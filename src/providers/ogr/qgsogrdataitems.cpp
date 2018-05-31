/***************************************************************************
                             qgsogrdataitems.cpp
                             -------------------
    begin                : 2011-04-01
    copyright            : (C) 2011 Radim Blazek
    email                : radim dot blazek at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsogrdataitems.h"
#include "qgsogrdbconnection.h"

#include "qgslogger.h"
#include "qgsmessagelog.h"
#include "qgssettings.h"
#include "qgsproject.h"
#include "qgssqlitehandle.h"
#include "qgsvectorlayer.h"
#include "qgsrasterlayer.h"
#include "qgsgeopackagedataitems.h"
#include "qgsogrutils.h"

#include <QFileInfo>
#include <QTextStream>
#include <QAction>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>

#include <ogr_srs_api.h>
#include <cpl_error.h>
#include <cpl_conv.h>
#include <gdal.h>

// these are defined in qgsogrprovider.cpp
QGISEXTERN QStringList fileExtensions();
QGISEXTERN QStringList directoryExtensions();
QGISEXTERN QStringList wildcards();

QGISEXTERN bool deleteLayer( const QString &uri, const QString &errCause );


QgsOgrLayerItem::QgsOgrLayerItem( QgsDataItem *parent,
                                  const QString &name, const QString &path, const QString &uri, LayerType layerType, bool isSubLayer )
  : QgsLayerItem( parent, name, path, uri, layerType, QStringLiteral( "ogr" ) )
{
  mIsSubLayer = isSubLayer;
  mToolTip = uri;
  setState( Populated ); // children are not expected

  if ( mPath.endsWith( QLatin1String( ".shp" ), Qt::CaseInsensitive ) )
  {
    if ( OGRGetDriverCount() == 0 )
    {
      OGRRegisterAll();
    }
    gdal::dataset_unique_ptr hDataSource( GDALOpenEx( mPath.toUtf8().constData(), GDAL_OF_VECTOR | GDAL_OF_UPDATE, nullptr, nullptr, nullptr ) );
    if ( hDataSource )
    {
      mCapabilities |= SetCrs;
    }

    // It it is impossible to assign a crs to an existing layer
    // No OGR_L_SetSpatialRef : http://trac.osgeo.org/gdal/ticket/4032
  }
}


bool QgsOgrLayerItem::setCrs( const QgsCoordinateReferenceSystem &crs )
{
  if ( !( mCapabilities & SetCrs ) )
    return false;

  QString layerName = mPath.left( mPath.indexOf( QLatin1String( ".shp" ), Qt::CaseInsensitive ) );
  QString wkt = crs.toWkt();

  // save ordinary .prj file
  OGRSpatialReferenceH hSRS = OSRNewSpatialReference( wkt.toLocal8Bit().data() );
  OSRMorphToESRI( hSRS ); // this is the important stuff for shapefile .prj
  char *pszOutWkt = nullptr;
  OSRExportToWkt( hSRS, &pszOutWkt );
  QFile prjFile( layerName + ".prj" );
  if ( prjFile.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
  {
    QTextStream prjStream( &prjFile );
    prjStream << pszOutWkt << endl;
    prjFile.close();
  }
  else
  {
    QgsMessageLog::logMessage( tr( "Couldn't open file %1.prj" ).arg( layerName ), tr( "OGR" ) );
    return false;
  }
  OSRDestroySpatialReference( hSRS );
  CPLFree( pszOutWkt );

  // save qgis-specific .qpj file (maybe because of better wkt compatibility?)
  QFile qpjFile( layerName + ".qpj" );
  if ( qpjFile.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
  {
    QTextStream qpjStream( &qpjFile );
    qpjStream << wkt.toLocal8Bit().data() << endl;
    qpjFile.close();
  }
  else
  {
    QgsMessageLog::logMessage( tr( "Couldn't open file %1.qpj" ).arg( layerName ), tr( "OGR" ) );
    return false;
  }

  return true;
}

QgsLayerItem::LayerType QgsOgrLayerItem::layerTypeFromDb( const QString &geometryType )
{
  if ( geometryType.contains( QStringLiteral( "Point" ), Qt::CaseInsensitive ) )
  {
    return QgsLayerItem::LayerType::Point;
  }
  else if ( geometryType.contains( QStringLiteral( "Polygon" ), Qt::CaseInsensitive ) )
  {
    return QgsLayerItem::LayerType::Polygon;
  }
  else if ( geometryType.contains( QStringLiteral( "LineString" ), Qt::CaseInsensitive ) )
  {
    return QgsLayerItem::LayerType::Line;
  }
  else if ( geometryType.contains( QStringLiteral( "Collection" ), Qt::CaseInsensitive ) )
  {
    return QgsLayerItem::LayerType::Vector;
  }
  // To be moved in a parent class that would also work for gdal and rasters
  else if ( geometryType.contains( QStringLiteral( "Raster" ), Qt::CaseInsensitive ) )
  {
    return QgsLayerItem::LayerType::Raster;
  }

  // fallback - try parsing as a WKT type string
  switch ( QgsWkbTypes::geometryType( QgsWkbTypes::parseType( geometryType ) ) )
  {
    case QgsWkbTypes::PointGeometry:
      return QgsLayerItem::LayerType::Point;
    case QgsWkbTypes::LineGeometry:
      return QgsLayerItem::LayerType::Line;
    case QgsWkbTypes::PolygonGeometry:
      return QgsLayerItem::LayerType::Polygon;
    case QgsWkbTypes::UnknownGeometry:
    case QgsWkbTypes::NullGeometry:
      break;
  }

  return QgsLayerItem::LayerType::TableLayer;
}

QList<QgsOgrDbLayerInfo *> QgsOgrLayerItem::subLayers( const QString &path, const QString &driver )
{

  QList<QgsOgrDbLayerInfo *> children;

  // Vector layers
  QgsVectorLayer layer( path, QStringLiteral( "ogr_tmp" ), QStringLiteral( "ogr" ) );
  if ( ! layer.isValid( ) )
  {
    QgsDebugMsgLevel( tr( "Layer is not a valid %1 Vector layer %2" ).arg( path ), 3 );
  }
  else
  {
    // Collect mixed-geom layers
    QMultiMap<int, QStringList> subLayersMap;
    const QStringList subLayersList( layer.dataProvider()->subLayers( ) );
    QMap< QString, int > mapLayerNameToCount;
    bool uniqueNames = true;
    int prevIdx = -1;
    for ( const QString &descriptor : subLayersList )
    {
      QStringList pieces = descriptor.split( QgsDataProvider::SUBLAYER_SEPARATOR );
      int idx = pieces[0].toInt();
      subLayersMap.insert( idx, pieces );
      if ( pieces.count() >= 4 && idx != prevIdx )
      {
        QString layerName = pieces[1];
        int count = ++mapLayerNameToCount[layerName];
        if ( count > 1 || layerName.isEmpty() )
          uniqueNames = false;
      }
      prevIdx = idx;
    }
    prevIdx = -1;
    const auto subLayerKeys = subLayersMap.keys( );
    for ( const int &idx : subLayerKeys )
    {
      if ( idx == prevIdx )
      {
        continue;
      }
      prevIdx = idx;
      QList<QStringList> values = subLayersMap.values( idx );
      for ( int i = 0; i < values.size(); ++i )
      {
        QStringList pieces = values.at( i );
        QString layerId = pieces[0];
        QString name = pieces[1];
        // QString featuresCount = pieces[2]; // Not used
        QString geometryType = pieces[3];
        QString geometryColumn = pieces[4];
        QgsLayerItem::LayerType layerType;
        layerType = QgsOgrLayerItem::layerTypeFromDb( geometryType );
        // example URI for mixed-geoms geoms:    '/path/gdal_sample_v1.2_no_extensions.gpkg|layerid=7|geometrytype=Point'
        // example URI for mixed-geoms attr table:    '/path/gdal_sample_v1.2_no_extensions.gpkg|layername=MyLayer|layerid=7'
        // example URI for single geoms:    '/path/gdal_sample_v1.2_no_extensions.gpkg|layerid=6'
        QString uri;
        if ( layerType != QgsLayerItem::LayerType::NoType )
        {
          if ( geometryType.contains( QStringLiteral( "Collection" ), Qt::CaseInsensitive ) )
          {
            QgsDebugMsgLevel( QStringLiteral( "Layer %1 is a geometry collection: skipping %2" ).arg( name, path ), 3 );
          }
          else
          {
            if ( uniqueNames )
              uri = QStringLiteral( "%1|layername=%2" ).arg( path, name );
            else
              uri = QStringLiteral( "%1|layerid=%2" ).arg( path, layerId );
            if ( values.size() > 1 )
            {
              uri += QStringLiteral( "|geometrytype=" ) + geometryType;
            }
            QgsDebugMsgLevel( QStringLiteral( "Adding %1 Vector item %2 %3 %4" ).arg( driver, name, uri, geometryType ), 3 );
            children.append( new QgsOgrDbLayerInfo( path, uri, name, geometryColumn, geometryType, layerType ) );
          }
        }
        else
        {
          QgsDebugMsgLevel( QStringLiteral( "Layer type is not a supported %1 Vector layer %2" ).arg( driver, path ), 3 );
          uri = QStringLiteral( "%1|layerid=%2|layername=%3" ).arg( path, layerId, name );
          children.append( new QgsOgrDbLayerInfo( path, uri, name, geometryColumn, geometryType, QgsLayerItem::LayerType::TableLayer ) );
        }
        QgsDebugMsgLevel( QStringLiteral( "Adding %1 Vector item %2 %3 %4" ).arg( driver, name, uri, geometryType ), 3 );
      }
    }
  }
  // Raster layers
  QgsRasterLayer::LayerOptions options;
  options.loadDefaultStyle = false;
  QgsRasterLayer rlayer( path, QStringLiteral( "gdal_tmp" ), QStringLiteral( "gdal" ), options );
  if ( !rlayer.dataProvider()->subLayers( ).empty() )
  {
    const QStringList layers( rlayer.dataProvider()->subLayers( ) );
    for ( const QString &uri : layers )
    {
      // Split on ':' since this is what comes out from the provider
      QStringList pieces = uri.split( ':' );
      QString name = pieces.value( pieces.length() - 1 );
      QgsDebugMsgLevel( QStringLiteral( "Adding GeoPackage Raster item %1 %2 %3" ).arg( name, uri ), 3 );
      children.append( new QgsOgrDbLayerInfo( path, uri, name, QStringLiteral( "" ), QStringLiteral( "Raster" ), QgsLayerItem::LayerType::Raster ) );
    }
  }
  else if ( rlayer.isValid( ) )
  {
    // Get the identifier
    GDALAllRegister();
    // do not print errors, but write to debug
    CPLPushErrorHandler( CPLQuietErrorHandler );
    CPLErrorReset();
    gdal::dataset_unique_ptr hDS( GDALOpen( path.toUtf8().constData(), GA_ReadOnly ) );
    CPLPopErrorHandler();

    if ( ! hDS )
    {
      QgsDebugMsg( QString( "GDALOpen error # %1 : %2 " ).arg( CPLGetLastErrorNo() ).arg( CPLGetLastErrorMsg() ) );

    }
    else
    {
      QString uri( QStringLiteral( "%1:%2" ).arg( driver, path ) );
      QString name = GDALGetMetadataItem( hDS.get(), "IDENTIFIER", nullptr );
      hDS.reset();
      // Fallback: will not be able to delete the table
      if ( name.isEmpty() )
      {
        name = QFileInfo( path ).fileName();
      }
      else
      {
        uri += QStringLiteral( ":%1" ).arg( name );
      }

      QgsDebugMsgLevel( QStringLiteral( "Adding %1 Raster item %2 %3" ).arg( driver, name, path ), 3 );
      children.append( new QgsOgrDbLayerInfo( path, uri, name, QStringLiteral( "" ), QStringLiteral( "Raster" ), QgsLayerItem::LayerType::Raster ) );
    }
  }
  return children;
}

QString QgsOgrLayerItem::layerName() const
{
  QFileInfo info( name() );
  if ( info.suffix() == QLatin1String( "gz" ) )
    return info.baseName();
  else
    return info.completeBaseName();
}

#ifdef HAVE_GUI
QList<QAction *> QgsOgrLayerItem::actions( QWidget *parent )
{
  QList<QAction *> lst;
  // Messages are different for files and tables
  QString message = mIsSubLayer ? QObject::tr( "Delete Layer '%1'…" ).arg( mName ) : QObject::tr( "Delete File '%1'…" ).arg( mUri );
  QAction *actionDeleteLayer = new QAction( message, parent );
  connect( actionDeleteLayer, &QAction::triggered, this, &QgsOgrLayerItem::deleteLayer );
  lst.append( actionDeleteLayer );
  return lst;
}

void QgsOgrLayerItem::deleteLayer()
{
  // Messages are different for files and tables
  QString title = mIsSubLayer ? QObject::tr( "Delete Layer" ) : QObject::tr( "Delete File" );
  // Check if the layer is in the registry
  const QgsMapLayer *projectLayer = nullptr;
  Q_FOREACH ( const QgsMapLayer *layer, QgsProject::instance()->mapLayers() )
  {
    if ( layer->publicSource() == mUri )
    {
      projectLayer = layer;
    }
  }
  if ( ! projectLayer )
  {
    QString confirmMessage;
    if ( mIsSubLayer )
    {
      confirmMessage = QObject::tr( "Are you sure you want to delete layer '%1' from datasource?" ).arg( mName );
    }
    else
    {
      confirmMessage = QObject::tr( "Are you sure you want to delete file '%1'?" ).arg( mUri );
    }
    if ( QMessageBox::question( nullptr, title,
                                confirmMessage,
                                QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) != QMessageBox::Yes )
      return;

    QString errCause;
    bool res = ::deleteLayer( mUri, errCause );
    if ( !res )
    {
      QMessageBox::warning( nullptr, title, errCause );
    }
    else
    {
      QMessageBox::information( nullptr, title, mIsSubLayer ? tr( "Layer deleted successfully." ) :  tr( "File deleted successfully." ) );
      if ( mParent )
        mParent->refresh();
    }
  }
  else
  {
    QMessageBox::warning( nullptr, title, QObject::tr( "The layer '%1' cannot be deleted because it is in the current project as '%2',"
                          " remove it from the project and retry." ).arg( mName, projectLayer->name() ) );
  }
}
#endif

// -------

static QgsOgrLayerItem *dataItemForLayer( QgsDataItem *parentItem, QString name, QString path, GDALDatasetH hDataSource, int layerId, bool isSubLayer = false )
{
  OGRLayerH hLayer = GDALDatasetGetLayer( hDataSource, layerId );
  OGRFeatureDefnH hDef = OGR_L_GetLayerDefn( hLayer );

  QgsLayerItem::LayerType layerType = QgsLayerItem::Vector;
  OGRwkbGeometryType ogrType = QgsOgrProvider::getOgrGeomType( hLayer );
  QgsWkbTypes::Type wkbType = QgsOgrProviderUtils::qgisTypeFromOgrType( ogrType );
  switch ( QgsWkbTypes::geometryType( wkbType ) )
  {
    case QgsWkbTypes::UnknownGeometry:
      break;
    case QgsWkbTypes::NullGeometry:
      layerType = QgsLayerItem::TableLayer;
      break;
    case QgsWkbTypes::PointGeometry:
      layerType = QgsLayerItem::Point;
      break;
    case QgsWkbTypes::LineGeometry:
      layerType = QgsLayerItem::Line;
      break;
    case QgsWkbTypes::PolygonGeometry:
      layerType = QgsLayerItem::Polygon;
      break;
  }

  QgsDebugMsgLevel( QString( "ogrType = %1 layertype = %2" ).arg( ogrType ).arg( layerType ), 2 );

  QString layerUri = path;

  if ( name.isEmpty() )
  {
    // we are in a collection
    name = QString::fromUtf8( OGR_FD_GetName( hDef ) );
    QgsDebugMsg( "OGR layer name : " + name );

    layerUri += "|layerid=" + QString::number( layerId );

    path += '/' + name;
  }

  QgsDebugMsgLevel( "OGR layer uri : " + layerUri, 2 );

  return new QgsOgrLayerItem( parentItem, name, path, layerUri, layerType, isSubLayer );
}

// ----

QgsOgrDataCollectionItem::QgsOgrDataCollectionItem( QgsDataItem *parent, const QString &name, const QString &path )
  : QgsDataCollectionItem( parent, name, path )
{
}

QVector<QgsDataItem *> QgsOgrDataCollectionItem::createChildren()
{
  QVector<QgsDataItem *> children;

  gdal::dataset_unique_ptr hDataSource( GDALOpenEx( mPath.toUtf8().constData(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr ) );
  if ( !hDataSource )
    return children;
  int numLayers = GDALDatasetGetLayerCount( hDataSource.get() );

  children.reserve( numLayers );
  for ( int i = 0; i < numLayers; ++i )
  {
    QgsOgrLayerItem *item = dataItemForLayer( this, QString(), mPath, hDataSource.get(), i, true );
    children.append( item );
  }

  return children;
}

bool QgsOgrDataCollectionItem::storeConnection( const QString &path, const QString &ogrDriverName )
{
  QFileInfo fileInfo( path );
  QString connName = fileInfo.fileName();
  if ( ! path.isEmpty() )
  {
    bool ok = true;
    while ( ok && ! QgsOgrDbConnection( connName, ogrDriverName ).path( ).isEmpty( ) )
    {

      connName = QInputDialog::getText( nullptr, tr( "Cannot add connection '%1'" ).arg( connName ),
                                        tr( "A connection with the same name already exists,\nplease provide a new name:" ), QLineEdit::Normal,
                                        QLatin1String( "" ), &ok );
    }
    if ( ok && ! connName.isEmpty() )
    {
      QgsOgrDbConnection connection( connName, ogrDriverName );
      connection.setPath( path );
      connection.save();
      return true;
    }
  }
  return false;
}

bool QgsOgrDataCollectionItem::createConnection( const QString &name, const QString &extensions, const QString &ogrDriverName )
{
  QString path = QFileDialog::getOpenFileName( nullptr, tr( "Open %1" ).arg( name ), QLatin1String( "" ), extensions );
  return storeConnection( path, ogrDriverName );
}


// ---------------------------------------------------------------------------

QGISEXTERN int dataCapabilities()
{
  return  QgsDataProvider::File | QgsDataProvider::Dir;
}

QGISEXTERN QgsDataItem *dataItem( QString path, QgsDataItem *parentItem )
{
  if ( path.isEmpty() )
    return nullptr;
  // Check if supported by QgsSpatiaLiteProvider or QgsRasterLite2Provider and some Mime-Types
  bool isSupportedFormat = false;
  bool bIsGeoPackage = false;
  bool isOgr = true; // this is not Gdal
  // QgsSpatialiteDbInfo can load Gdal/Ogr Sqlite3-formats using the QgsGdal/OgrProviders
  bool allowOgrGdal = true; // allow the Spatialite::dataItem version to deal with Gdal/Ogr Sqlite3-formats
  if ( QgsSpatiaLiteUtils::CheckOgrGdalDataItem( path, isOgr, allowOgrGdal, &isSupportedFormat, &bIsGeoPackage ) )
  {
    // either delt with elsewhere or contains a Magic-Header that is not supported by Ogr/Gdal
    return nullptr;
  }
  if ( ( isSupportedFormat ) || ( bIsGeoPackage ) )
  {
    QgsDataItem *item = nullptr;
    QFileInfo info( path );
    QString name = info.fileName();
    if ( isSupportedFormat )
    {
      item = new QgsOgrDataCollectionItem( parentItem, name, path );
    }
    if ( bIsGeoPackage )
    {
      item = new QgsGeoPackageCollectionItem( parentItem, name, path );
    }
    if ( item )
    {
      QgsDebugMsgLevel( QString( "OGR starting[%1]" ).arg( name ), 4 );
      return item;
    }
  }

  QgsDebugMsgLevel( "thePath: " + path, 2 );

// zip settings + info
  QgsSettings settings;
  QString scanZipSetting = settings.value( QStringLiteral( "qgis/scanZipInBrowser2" ), "basic" ).toString();
  QString vsiPrefix = QgsZipItem::vsiPrefix( path );
  bool is_vsizip = ( vsiPrefix == QLatin1String( "/vsizip/" ) );
  bool is_vsigzip = ( vsiPrefix == QLatin1String( "/vsigzip/" ) );
  bool is_vsitar = ( vsiPrefix == QLatin1String( "/vsitar/" ) );

// should we check ext. only?
// check if scanItemsInBrowser2 == extension or parent dir in scanItemsFastScanUris
// TODO - do this in dir item, but this requires a way to inform which extensions are supported by provider
// maybe a callback function or in the provider registry?
  bool scanExtSetting = false;
  if ( ( settings.value( QStringLiteral( "qgis/scanItemsInBrowser2" ),
                         "extension" ).toString() == QLatin1String( "extension" ) ) ||
       ( parentItem && settings.value( QStringLiteral( "qgis/scanItemsFastScanUris" ),
                                       QStringList() ).toStringList().contains( parentItem->path() ) ) ||
       ( ( is_vsizip || is_vsitar ) && parentItem && parentItem->parent() &&
         settings.value( QStringLiteral( "qgis/scanItemsFastScanUris" ),
                         QStringList() ).toStringList().contains( parentItem->parent()->path() ) ) )
  {
    scanExtSetting = true;
  }

// get suffix, removing .gz if present
  QString tmpPath = path; //path used for testing, not for layer creation
  if ( is_vsigzip )
    tmpPath.chop( 3 );
  QFileInfo info( tmpPath );
  QString suffix = info.suffix().toLower();
// extract basename with extension
  info.setFile( path );
  QString name = info.fileName();

// If a .tab exists, then the corresponding .map/.dat is very likely a
// side-car file of the .tab
  if ( suffix == QLatin1String( "map" ) || suffix == QLatin1String( "dat" ) )
  {
    if ( QFileInfo( QDir( info.path() ), info.baseName() + ".tab" ).exists() )
      return nullptr;
  }

  QgsDebugMsgLevel( "thePath= " + path + " tmpPath= " + tmpPath + " name= " + name
                    + " suffix= " + suffix + " vsiPrefix= " + vsiPrefix, 3 );

  QStringList myExtensions = fileExtensions();
  QStringList dirExtensions = directoryExtensions();

// allow only normal files, supported directories, or VSIFILE items to continue
  bool isOgrSupportedDirectory = info.isDir() && dirExtensions.contains( suffix );
  if ( !isOgrSupportedDirectory && !info.isFile() && vsiPrefix.isEmpty() )
    return nullptr;

// skip *.aux.xml files (GDAL auxiliary metadata files),
// *.shp.xml files (ESRI metadata) and *.tif.xml files (TIFF metadata)
// unless that extension is in the list (*.xml might be though)
  if ( path.endsWith( QLatin1String( ".aux.xml" ), Qt::CaseInsensitive ) &&
       !myExtensions.contains( QStringLiteral( "aux.xml" ) ) )
    return nullptr;
  if ( path.endsWith( QLatin1String( ".shp.xml" ), Qt::CaseInsensitive ) &&
       !myExtensions.contains( QStringLiteral( "shp.xml" ) ) )
    return nullptr;
  if ( path.endsWith( QLatin1String( ".tif.xml" ), Qt::CaseInsensitive ) &&
       !myExtensions.contains( QStringLiteral( "tif.xml" ) ) )
    return nullptr;

// We have to filter by extensions, otherwise e.g. all Shapefile files are displayed
// because OGR drive can open also .dbf, .shx.
  if ( myExtensions.indexOf( suffix ) < 0 && !dirExtensions.contains( suffix ) )
  {
    bool matches = false;
    Q_FOREACH ( const QString &wildcard, wildcards() )
    {
      QRegExp rx( wildcard, Qt::CaseInsensitive, QRegExp::Wildcard );
      if ( rx.exactMatch( info.fileName() ) )
      {
        matches = true;
        break;
      }
    }
    if ( !matches )
      return nullptr;
  }

// .dbf should probably appear if .shp is not present
  if ( suffix == QLatin1String( "dbf" ) )
  {
    QString pathShp = path.left( path.count() - 4 ) + ".shp";
    if ( QFileInfo::exists( pathShp ) )
      return nullptr;
  }

// fix vsifile path and name
  if ( !vsiPrefix.isEmpty() )
  {
    // add vsiPrefix to path if needed
    if ( !path.startsWith( vsiPrefix ) )
      path = vsiPrefix + path;
    // if this is a /vsigzip/path_to_zip.zip/file_inside_zip remove the full path from the name
    // no need to change the name I believe
#if 0
    if ( ( is_vsizip || is_vsitar ) && ( path != vsiPrefix + parentItem->path() ) )
    {
      name = path;
      name = name.replace( vsiPrefix + parentItem->path() + '/', "" );
    }
#endif
  }

// Filters out the OGR/GDAL supported formats that can contain multiple layers
// and should be treated like a DB: GeoPackage and SQLite
// NOTE: this formats are scanned for rasters too and they must
//       be skipped by "gdal" provider or the rasters will be listed
//       twice. ogrSupportedDbLayersExtensions must be kept in sync
//       with the companion variable (same name) in the gdal provider
//       class
// TODO: add more OGR supported multiple layers formats here!
  QStringList ogrSupportedDbLayersExtensions;
  ogrSupportedDbLayersExtensions << QStringLiteral( "gpkg" ) << QStringLiteral( "sqlite" ) << QStringLiteral( "db" ) << QStringLiteral( "gdb" );
  QStringList ogrSupportedDbDriverNames;
  ogrSupportedDbDriverNames << QStringLiteral( "GPKG" ) << QStringLiteral( "db" ) << QStringLiteral( "gdb" );

// Fast track: return item without testing if:
// scanExtSetting or zipfile and scan zip == "Basic scan"
// netCDF files can be both raster or vector, so fallback to opening
  if ( ( scanExtSetting ||
         ( ( is_vsizip || is_vsitar ) && scanZipSetting == QLatin1String( "basic" ) ) ) &&
       suffix != QLatin1String( "nc" ) )
  {
    // if this is a VRT file make sure it is vector VRT to avoid duplicates
    if ( suffix == QLatin1String( "vrt" ) )
    {
      CPLPushErrorHandler( CPLQuietErrorHandler );
      CPLErrorReset();
      GDALDriverH hDriver = GDALIdentifyDriver( path.toUtf8().constData(), nullptr );
      CPLPopErrorHandler();
      if ( !hDriver || GDALGetDriverShortName( hDriver ) == QLatin1String( "VRT" ) )
      {
        QgsDebugMsgLevel( "Skipping VRT file because root is not a OGR VRT", 2 );
        return nullptr;
      }
    }
    // Handle collections
    // Check if the layer has sublayers by comparing the extension
    QgsDataItem *item = nullptr;
    if ( ! ogrSupportedDbLayersExtensions.contains( suffix ) )
    {
      item = new QgsOgrLayerItem( parentItem, name, path, path, QgsLayerItem::Vector );
    }
    else if ( suffix.compare( QLatin1String( "gpkg" ), Qt::CaseInsensitive ) == 0 )
    {
      item = new QgsGeoPackageCollectionItem( parentItem, name, path );
    }
    else
    {
      item = new QgsOgrDataCollectionItem( parentItem, name, path );
    }

    if ( item )
      return item;
  }

// Slow track: scan file contents
  QgsDataItem *item = nullptr;

// test that file is valid with OGR
  if ( OGRGetDriverCount() == 0 )
  {
    OGRRegisterAll();
  }
// do not print errors, but write to debug
  CPLPushErrorHandler( CPLQuietErrorHandler );
  CPLErrorReset();
  gdal::dataset_unique_ptr hDS( GDALOpenEx( path.toUtf8().constData(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr ) );
  CPLPopErrorHandler();

  if ( ! hDS )
  {
    QgsDebugMsg( QString( "GDALOpen error # %1 : %2 on %3" ).arg( CPLGetLastErrorNo() ).arg( CPLGetLastErrorMsg() ).arg( path ) );
    return nullptr;
  }

  GDALDriverH hDriver = GDALGetDatasetDriver( hDS.get() );
  QString driverName = GDALGetDriverShortName( hDriver );
  QgsDebugMsgLevel( QString( "GDAL Driver : %1" ).arg( driverName ), 2 );
  int numLayers = GDALDatasetGetLayerCount( hDS.get() );

// GeoPackage needs a specialized data item, mainly because of raster deletion not
// yet implemented in GDAL (2.2.1)
  if ( driverName == QLatin1String( "GPKG" ) )
  {
    item = new QgsGeoPackageCollectionItem( parentItem, name, path );
  }
  else if ( numLayers > 1 || ogrSupportedDbDriverNames.contains( driverName ) )
  {
    item = new QgsOgrDataCollectionItem( parentItem, name, path );
  }
  else
  {
    item = dataItemForLayer( parentItem, name, path, hDS.get(), 0 );
  }
  return item;
}

QGISEXTERN bool handlesDirectoryPath( const QString &path )
{
  QFileInfo info( path );
  QString suffix = info.suffix().toLower();

  QStringList dirExtensions = directoryExtensions();
  return dirExtensions.contains( suffix );
}
