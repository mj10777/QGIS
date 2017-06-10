/***************************************************************************
     QgsGeorefPluginGui.cpp
     --------------------------------------
    Date                 : Sun Sep 16 12:03:52 AKDT 2007
    Copyright            : (C) 2007 by Gary E. Sherman
    Email                : sherman at mrcc dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <QDesktopWidget>
#include <QDialogButtonBox>
#include <QClipboard>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPrinter>
#include <QProgressDialog>
#include <QPushButton>
#include <QTextStream>
#include <QPen>
#include <QStringList>
#include <QList>

#include "qgssettings.h"
#include "qgisinterface.h"
#include "qgsapplication.h"

#include "qgscomposition.h"
#include "qgscomposerlabel.h"
#include "qgscomposermap.h"
#include "qgscomposertexttable.h"
#include "qgscomposertablecolumn.h"
#include "qgscomposerframe.h"
#include "qgsmapcanvas.h"
#include "qgsmapcoordsdialog.h"
#include "qgsmaptoolzoom.h"
#include "qgsmaptoolpan.h"
#include "../../app/nodetool/qgsnodetool.h"

#include "qgsproject.h"
#include "qgsrasterlayer.h"
#include "../../app/qgsrasterlayerproperties.h"
#include "qgsproviderregistry.h"
#include "qgsadvanceddigitizingdockwidget.h"

#include "qgsgeorefdatapoint.h"

#include "qgsleastsquares.h"

#include "qgsgeorefconfigdialog.h"
#include "qgsgeorefdescriptiondialog.h"
#include "qgsresidualplotitem.h"
#include "qgstransformsettingsdialog.h"

#include "qgsgeorefplugingui.h"
#include "qgsmessagebar.h"

#include "qgsvectordataprovider.h"
#include "qgsvectorlayer.h"
#include "qgspallabeling.h"
#include "qgsvectorlayerlabeling.h"
#include "qgstextrenderer.h"
#include "qgsrenderer.h"
#include "qgssymbollayer.h"
#include "qgsmarkersymbollayer.h"
#include "qgslinesymbollayer.h"
#include "qgsfillsymbollayer.h"
#include "qgscategorizedsymbolrenderer.h"
#include "qgslayerdefinition.h"
#include "qgsmaptooladdfeature.h"
#include "qgsmaptoolmovefeature.h"
#include <sqlite3.h>
#include <spatialite.h>

#include "qgslayertree.h"
#include "qgslayertreemodel.h"
#include "qgsgcpdatabasedialog.h"

QgsGeorefDockWidget::QgsGeorefDockWidget( const QString &title, QWidget *parent, Qt::WindowFlags flags )
  : QgsDockWidget( title, parent, flags )
{
  setObjectName( QStringLiteral( "GeorefDockWidget" ) ); // set object name so the position can be saved
}

QgsGeorefPluginGui::QgsGeorefPluginGui( QgisInterface *qgisInterface, QWidget *parent, Qt::WindowFlags fl )
  : QMainWindow( parent, fl )
  , mMousePrecisionDecimalPlaces( 0 )
  , mTransformParam( QgsGeorefTransform::InvalidTransform )
  , mIface( qgisInterface )
  , mLayerRaster( nullptr )
  , mAgainAddRaster( false )
  , mMovingPoint( nullptr )
  , mMovingPointQgis( nullptr )
  , mMapCoordsDialog( nullptr )
  , mUseZeroForTrans( false )
  , mLoadInQgis( false )
  , mDock( nullptr )
{
  setupUi( this );
  // mj10777: when false: old behavior with points file
  mPointOrSpatialiteGcp = false;
  mSpatialiteGcpOff = true;
  mPointsPolygon = true;
  mEventGcpStatus = 0;
  mEventPointStatus = 0;
  mEventPixelStatus = 0;
  mGcpDbData = nullptr;
  mGcpMasterDatabaseFileName = QString::null;
  mGcpMasterSrid = -2; // Invalid
  mGcpSrid = -2; // Invalid
  mGcpDatabaseFileName = QString::null;
  mGcpFileName = QString::null;
  mGcpBaseFileName = QString::null;
  /*
  mAddFeature = nullptr;
  mMoveFeature = nullptr;
  mNodeTool = nullptr;
  */
  // mj10777: when false: old behavior or no spatialite
  mGdalScriptOrGcpList = false;
  // Brings best results with spatialite Gps_Compute logic
  mTransformParam = QgsGeorefTransform::ThinPlateSpline;
  mResamplingMethod = QgsImageWarper::Lanczos;
  mCompressionMethod = "DEFLATE";
  bGcpFileName = false;
  bPointsFileName = true;
  mRasterYear = 1; // QDate starts with September 1752, bad for a map of 1748
  mRasterScale = 0;
  mRasterNodata = -1;
  mGdalScriptOrGcpList = true; // return only gcp-list instead of gdal commands-script
  mLayerGcpMaster = nullptr;
  // Circle 2.5 meters around MasterPoint
  mGcpMasterArea = 5.0;
// Circle 1.0 meters around GcpPoint
  mGcpPointArea = 2.0;
// 1.0 meters Map-Unit based on srid being used in QGIS
  mMapPointUnit = 1.0;
  mAvoidUnneededUpdates = false;
  mLayerGtifRaster = nullptr;
  mLoadGTifInQgis = 1;
  mGcpLabelType = 1;
  mGcpLabelExpression = "id_gcp"; // "id_gcp||'['||point_x||', '||point_y||']''"
  QFont appFont = QApplication::font();
  fontPointSize = ( double )appFont.pointSize();

  QgsSettings s;
  restoreGeometry( s.value( QStringLiteral( "/Plugin-GeoReferencer/Window/geometry" ) ).toByteArray() );

  QWidget *centralWidget = this->centralWidget();
  mCentralLayout = new QGridLayout( centralWidget );
  centralWidget->setLayout( mCentralLayout );
  mCentralLayout->setContentsMargins( 0, 0, 0, 0 );

  createActions();
  createActionGroups();
  createMenus();
  createMapCanvas();
  createDockWidgets();
  createStatusBar();

  // a bar to warn the user with non-blocking messages
  mMessageBar = new QgsMessageBar( centralWidget );
  mMessageBar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed );
  mCentralLayout->addWidget( mMessageBar, 0, 0, 1, 1 );

  setAddPointTool();
  setupConnections();
  readSettings();

  mActionLinkGeorefToQGis->setEnabled( false );
  mActionLinkQGisToGeoref->setEnabled( false );

  mCanvas->clearExtentHistory(); // reset zoomnext/zoomlast

  connect( mIface, &QgisInterface::currentThemeChanged, this, &QgsGeorefPluginGui::updateIconTheme );

  if ( s.value( QStringLiteral( "/Plugin-GeoReferencer/Config/ShowDocked" ) ).toBool() )
  {
    dockThisWindow( true );
  }
  if ( s.value( QStringLiteral( "/Plugin-GeoReferencer/Config/GDALScript" ) ).toString() == "Full" )
  {
    mGdalScriptOrGcpList = true;
  }
  else
  {
    mGdalScriptOrGcpList = false;
  }
  // Will switch to the other Script-Type, setting text
  setGdalScriptType();
  if ( s.value( QStringLiteral( "/Plugin-GeoReferencer/Config/LayerProviderType" ) ).toString() == "ogr" )
  {
    mLayerProviderType = "spatialite";
  }
  else
  {
    mLayerProviderType = "ogr";
  }
  // Will switch to the other Provider, setting icons and text
  setLayerProviderType();
}

void QgsGeorefPluginGui::dockThisWindow( bool dock )
{
  if ( mDock )
  {
    setParent( mIface->mainWindow(), Qt::Window );
    show();

    mIface->removeDockWidget( mDock );
    mDock->setWidget( nullptr );
    delete mDock;
    mDock = nullptr;
  }

  if ( dock )
  {
    mDock = new QgsGeorefDockWidget( tr( "Georeferencer" ), mIface->mainWindow() );
    mDock->setWidget( this );
    mIface->addDockWidget( Qt::BottomDockWidgetArea, mDock );
  }
}

QgsGeorefPluginGui::~QgsGeorefPluginGui()
{
  QgsSettings settings;
  settings.setValue( QStringLiteral( "Plugin-GeoReferencer/Window/geometry" ), saveGeometry() );
  //will call clearGcpData() when needed and delete any old rasterlayers
  removeOldLayer();
  delete mToolZoomIn;
  delete mToolZoomOut;
  delete mToolPan;
  delete mToolAddPoint;
  delete mToolNodePoint;
  // delete QgsGcpCoverages::instance();
  // QgsGcpCoverages::instance()=nullptr;
  /*
   if (mAddFeature)
    delete mAddFeature;
   if (mMoveFeature)
    delete mMoveFeature;
   if (mNodeTool)
    delete mNodeTool;
  */
}

// ----------------------------- protected --------------------------------- //
void QgsGeorefPluginGui::closeEvent( QCloseEvent *e )
{
  Q_UNUSED( e );
  //will call clearGcpData() when needed and delete any old rasterlayers
  removeOldLayer();
  mRasterFileName = QLatin1String( "" );
}

void QgsGeorefPluginGui::reset()
{
  if ( QMessageBox::question( this,
                              tr( "Reset Georeferencer" ),
                              tr( "Reset georeferencer and clear all GCP points?" ),
                              QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel ) != QMessageBox::Cancel )
  {
    mRasterFileName.clear();
    mModifiedRasterFileName.clear();
    setWindowTitle( tr( "Georeferencer" ) );
    //will call clearGcpData() when needed and delete any old rasterlayers
    removeOldLayer();
  }
}

// -------------------------- private slots -------------------------------- //
// File slots
void QgsGeorefPluginGui::openRasterDialog()
{
  QgsSettings s;
  QString dir = s.value( QStringLiteral( "/Plugin-GeoReferencer/rasterdirectory" ) ).toString();
  if ( dir.isEmpty() )
    dir = '.';

  QString otherFiles = tr( "All other files (*)" );
  QString lastUsedFilter = s.value( QStringLiteral( "/Plugin-GeoReferencer/lastusedfilter" ), otherFiles ).toString();

  QString filters = QgsProviderRegistry::instance()->fileRasterFilters();
  filters.prepend( otherFiles + ";;" );
  filters.chop( otherFiles.size() + 2 );
  QString sRasterFileName = QFileDialog::getOpenFileName( this, tr( "Open raster" ), dir, filters, &lastUsedFilter );
  QFileInfo raster_file( QString( "%1" ).arg( sRasterFileName ) );
  if ( raster_file.exists() )
  {
    s.setValue( QStringLiteral( "/Plugin-GeoReferencer/lastusedfilter" ), lastUsedFilter );
    openRaster( raster_file );
  }
}
void QgsGeorefPluginGui::openRaster( QFileInfo raster_file )
{
  if ( ! raster_file.exists() )
  {
    return;
  }

  mRasterFileName = raster_file.canonicalFilePath();
  mModifiedRasterFileName = QLatin1String( "" );
  QString errMsg;
  if ( !QgsRasterLayer::isValidRasterFileName( mRasterFileName, errMsg ) )
  {
    QString msg = tr( "%1 is not a supported raster data source" ).arg( mRasterFileName );

    if ( !errMsg.isEmpty() )
      msg += '\n' + errMsg;

    QMessageBox::information( this, tr( "Unsupported Data Source" ), msg );
    return;
  }
  // mj10777
  mGcpFileName = QString( "%1.gcp.txt" ).arg( mRasterFileName );
  mGcpBaseFileName = raster_file.completeBaseName();
  QDir parent_dir( raster_file.canonicalPath() );
  QgsSettings s;
  s.setValue( QStringLiteral( "/Plugin-GeoReferencer/rasterdirectory" ), raster_file.path() );

  mGeorefTransform.selectTransformParametrisation( mTransformParam );
  mGeorefTransform.setRasterChangeCoords( mRasterFileName );
  statusBar()->showMessage( tr( "Raster loading: %1" ).arg( mGcpBaseFileName ) );
  setWindowTitle( tr( "Georeferencer - %1" ).arg( raster_file.fileName() ) );
  //will call clearGcpData() when needed and delete any old rasterlayers
  removeOldLayer();
  if ( ( !mGcpDbData ) || ( mGcpDbData->mGcpDatabaseFileName.isEmpty() ) )
  {
    // if no Gcp-Database is active, use the Parent-Directory-Name as the default name, otherwise add it to the Active Database [which may be elsewhere]
    mGcpDatabaseFileName = QString( "%1/%2.maps.gcp.db" ).arg( raster_file.canonicalPath() ).arg( parent_dir.dirName() );
  }
  mGcpAuthid = mIface->mapCanvas()->mapSettings().destinationCrs().authid();
  mGcpDescription = mIface->mapCanvas()->mapSettings().destinationCrs().description();
  QStringList sa_authid = mGcpAuthid.split( ":" );
  QString s_srid = "-1";
  if ( sa_authid.length() == 2 )
    s_srid = sa_authid[1];
  mGcpSrid = s_srid.toInt();
  // qDebug()<< QString("destinationCrs[%1]: authid[%2]  srsid[%3]  description[%4] ").arg(mGcpSrid).arg(mGcpAuthid).arg(mIface->mapCanvas()->mapSettings().destinationCrs().srsid()).arg(mGcpDescription);
  if ( mGcpSrid > 0 )
  {
    // with srid (of project), activate the buttons and set set output-filename
    // give this a default name
    mModifiedRasterFileName = QString( "%1.%2.map.%3" ).arg( mGcpBaseFileName ).arg( s_srid ).arg( raster_file.suffix() );
  }
  else
  {
    mModifiedRasterFileName = QString( "%1.0.map.%3" ).arg( mGcpBaseFileName ).arg( raster_file.suffix() );
  }
  // qDebug() << "QgsGeorefPluginGui::openRaster[1] - addRaster";
  // load previously added points
  mGcpPointsFileName = mRasterFileName + ".points";
  addRaster( mRasterFileName );
  // qDebug() << "QgsGeorefPluginGui::openRaster[2] - setExtent";
  // mj10777
  if ( mGcpSrid > 0 )
  {
    // with srid (of project), activate the buttons and set set output-filename
    mActionLinkGeorefToQGis->setChecked( false );
    mActionLinkQGisToGeoref->setChecked( false );
    mActionLinkGeorefToQGis->setEnabled( true );
    mActionLinkQGisToGeoref->setEnabled( true );
  }
  else
  {
    mModifiedRasterFileName = QString( "%1.0.map.%3" ).arg( mGcpBaseFileName ).arg( raster_file.suffix() );
  }

  mCanvas->clearExtentHistory(); // reset zoomnext/zoomlast
  mWorldFileName = guessWorldFileName( mRasterFileName );
  statusBar()->showMessage( tr( "Raster loaded: %1" ).arg( mGcpBaseFileName ) );
}

void QgsGeorefPluginGui::doGeoreference()
{
  if ( georeference() )
  {
    mMessageBar->pushMessage( tr( "Georeference Successful" ), tr( "Raster was successfully georeferenced." ), QgsMessageBar::INFO, messageTimeout() );
    if ( mLoadInQgis )
    {
      if ( mModifiedRasterFileName.isEmpty() )
      {
        // mIface->addRasterLayer( mRasterFileName );
        QFileInfo raster_file( QString( "%1" ).arg( mRasterFileName ) );
        loadGTifInQgis( raster_file.canonicalFilePath() );
      }
      else
      {
        QFileInfo gtif_file( QString( "%1/%2" ).arg( mRasterFilePath ).arg( mModifiedRasterFileName ) );
        loadGTifInQgis( gtif_file.canonicalFilePath() );
        // mIface->addRasterLayer( mModifiedRasterFileName );
      }

      //      showMessageInLog(tr("Modified raster saved in"), mModifiedRasterFileName);
      //      saveGCPs();

      //      mTransformParam = QgsGeorefTransform::InvalidTransform;
      //      mGeorefTransform.selectTransformParametrisation(mTransformParam);
      //      mGcpListWidget->setGeorefTransform(&mGeorefTransform);
      //      mTransformParamLabel->setText(tr("Transform: ") + convertTransformEnumToString(mTransformParam));

      mActionLinkGeorefToQGis->setEnabled( false );
      mActionLinkQGisToGeoref->setEnabled( false );
    }
  }
}

bool QgsGeorefPluginGui::getTransformSettings()
{
  QgsTransformSettingsDialog d( mRasterFileName, mModifiedRasterFileName, getGcpList()->countDataPointsEnabled() );
  if ( !d.exec() )
  {
    return false;
  }

  d.getTransformSettings( mTransformParam, mResamplingMethod, mCompressionMethod,
                          mModifiedRasterFileName, mProjection, mPdfOutputMapFile, mPdfOutputFile, mUseZeroForTrans, mLoadInQgis, mUserResX, mUserResY );
  mTransformParamLabel->setText( tr( "Transform: " ) + convertTransformEnumToString( mTransformParam ) );
  mGeorefTransform.selectTransformParametrisation( mTransformParam );
  mGcpListWidget->setGeorefTransform( &mGeorefTransform );
  mWorldFileName = guessWorldFileName( mRasterFileName );

  //  showMessageInLog(tr("Output raster"), mModifiedRasterFileName.isEmpty() ? tr("Non set") : mModifiedRasterFileName);
  //  showMessageInLog(tr("Target projection"), mProjection.isEmpty() ? tr("Non set") : mProjection);
  //  logTransformOptions();
  //  logRequaredGCPs();

  if ( QgsGeorefTransform::InvalidTransform != mTransformParam )
  {
    mActionLinkGeorefToQGis->setEnabled( true );
    mActionLinkQGisToGeoref->setEnabled( true );
  }
  else
  {
    mActionLinkGeorefToQGis->setEnabled( false );
    mActionLinkQGisToGeoref->setEnabled( false );
  }

  updateTransformParamLabel();
  return true;
}
void QgsGeorefPluginGui::generateGDALScript()
{
  if ( mLayerTreeView->currentLayer() == mLayerMercatorPolygons )
  {
    // this needs no gcp's, thus should run before checking for this is done [checkReadyGeoref]
    showGDALScript( QStringList() << generateGDALMercatorScript() );
    return;
  }
  if ( !checkReadyGeoref() )
    return;

  switch ( mTransformParam )
  {
    // for the spatialite/gisGrass logic wit presice POINTs:
    // Very poor results
    case QgsGeorefTransform::PolynomialOrder1:
    // Almost same (good)  results as ThinPlateSpline
    case QgsGeorefTransform::PolynomialOrder2:
    // Very poor results
    case QgsGeorefTransform::PolynomialOrder3:
    // Best results [set as default]
    case QgsGeorefTransform::ThinPlateSpline:
    {
      // CAVEAT: generateGDALwarpCommand() relies on some member variables being set
      // by generateGDALtranslateCommand(), so this method must be called before
      // gdalwarpCommand*()!
      if ( mGdalScriptOrGcpList )
      {
        // mj10777: for use when only gcp-list is to be returned
        showGDALScript( QStringList() << generateGDALGcpList() );
        break;
      }
      else
      {
        QString translateCommand = generateGDALtranslateCommand( false );
        QString gdalwarpCommand;
        QString resamplingStr = convertResamplingEnumToString( mResamplingMethod );

        int order = polynomialOrder( mTransformParam );
        if ( order != 0 )
        {
          gdalwarpCommand = generateGDALwarpCommand( resamplingStr, mCompressionMethod, mUseZeroForTrans, order,
                            mUserResX, mUserResY );
          showGDALScript( QStringList() << translateCommand << gdalwarpCommand );
          break;
        }
      }
    }
    FALLTHROUGH;
    default:
      mMessageBar->pushMessage( tr( "Invalid Transform" ), tr( "GDAL scripting is not supported for %1 transformation." )
                                .arg( convertTransformEnumToString( mTransformParam ) )
                                , QgsMessageBar::WARNING, messageTimeout() );
  }
}

// Edit slots
void QgsGeorefPluginGui::setSpatialiteGcpUsage()
{
  QString s_text = "Spatialite Gcp-Logic cannot be used";
  if ( isGcpEnabled() )
  {
    mSpatialiteGcpOff = !mSpatialiteGcpOff;
    if ( mSpatialiteGcpOff )
      s_text = "Spatialite Gcp-Logic will not be used when creating a new Gcp-Point";
    else
      s_text = "Spatialite Gcp-Logic will be used when creating a new Gcp-Point";
  }
  mActionSpatialiteGcp->setEnabled( true );
  mActionSpatialiteGcp->setText( s_text );
  mActionSpatialiteGcp->setToolTip( s_text );
  mActionSpatialiteGcp->setStatusTip( s_text );
  mActionSpatialiteGcp->setChecked( !mSpatialiteGcpOff );
  s_text = "Fetch Master-Gcp(s) from Map Extent and add as a new Gcp-Point, with Gcp-Master Point correctoons";
  if ( ( !isGcpEnabled() ) || ( mSpatialiteGcpOff ) )
  {
    s_text = "Gcp-Master Point corrections from Map Extent, without the Fetching of Master-Gcp(s) ";
  }
  mActionFetchMasterGcpExtent->setText( s_text );
  mActionFetchMasterGcpExtent->setToolTip( s_text );
  mActionFetchMasterGcpExtent->setStatusTip( s_text );
  mActionFetchMasterGcpExtent->setEnabled( true );
  // qDebug() << QString( "-I-> QgsGeorefPluginGui::setSpatialiteGcpUsage[%1,%2,%3]" ).arg(mSpatialiteGcpOff).arg(mActionSpatialiteGcp->isChecked()).arg(s_text);
}
void QgsGeorefPluginGui::setAddPointTool()
{
  if ( mToolAddPoint )
  {
    mCanvas->setMapTool( mToolAddPoint );
  }
  if ( mLayerGcpPixels )
  {
    if ( mLayerTreeView->currentLayer() == mLayerGcpPixels )
    {
      // mIface->layerTreeView()->setCurrentLayer( mLayerGcpPoints );
    }
  }
  if ( mLayerMercatorPolygons )
  {
    if ( mLayerTreeView->currentLayer() == mLayerMercatorPolygons )
    {
      // mIface->layerTreeView()->setCurrentLayer( mLayerMercatorCutlinePolygons );
    }
  }
}

void QgsGeorefPluginGui::setNodePointTool()
{
  if ( mToolNodePoint )
  {
    mCanvas->setMapTool( mToolNodePoint );
  }
}

void QgsGeorefPluginGui::setMovePointTool()
{
  if ( mLayerMercatorPolygons )
  {
    if ( mLayerTreeView->currentLayer() == mLayerMercatorPolygons )
    {
      // mIface->layerTreeView()->setCurrentLayer( mLayerMercatorCutlinePolygons );
    }
  }
}

// View slots
void QgsGeorefPluginGui::setPanTool()
{
  mCanvas->setMapTool( mToolPan );
}

void QgsGeorefPluginGui::setZoomInTool()
{
  mCanvas->setMapTool( mToolZoomIn );
}

void QgsGeorefPluginGui::setZoomOutTool()
{
  mCanvas->setMapTool( mToolZoomOut );
}

void QgsGeorefPluginGui::zoomToLayerTool()
{
  if ( mLayerRaster )
  {
    mCanvas->setExtent( mLayerRaster->extent() );
    mCanvas->refresh();
  }
}

void QgsGeorefPluginGui::zoomToLast()
{
  mCanvas->zoomToPreviousExtent();
}

void QgsGeorefPluginGui::zoomToNext()
{
  mCanvas->zoomToNextExtent();
}

void QgsGeorefPluginGui::linkQGisToGeoref( bool link )
{
  if ( link )
  {
    if ( QgsGeorefTransform::InvalidTransform != mTransformParam )
    {
      // Indicate that georeferencer canvas extent has changed
      extentsChangedGeorefCanvas();
    }
    else
    {
      mActionLinkGeorefToQGis->setEnabled( false );
    }
  }
}

void QgsGeorefPluginGui::linkGeorefToQGis( bool link )
{
  if ( link )
  {
    if ( QgsGeorefTransform::InvalidTransform != mTransformParam )
    {
      // Indicate that qgis main canvas extent has changed
      extentsChangedQGisCanvas();
    }
    else
    {
      mActionLinkQGisToGeoref->setEnabled( false );
    }
  }
}

// GCPs slots
void QgsGeorefPluginGui::addPoint( const QgsPointXY &pixelCoords, const QgsPointXY &mapCoords, int id_gcp, bool b_PixelMap,  bool enable, bool finalize )
{
  Q_UNUSED( finalize );
  QString s_map_points = QString( "pixel[%1] map[%2]" ).arg( pixelCoords.wellKnownText() ).arg( mapCoords.wellKnownText() );
  QString s_Event = QString( "Pixel-Point to Map-Point" );
  QgsVectorLayer *layer_gcp_update = mLayerGcpPoints;
  QgsPointXY added_point_update = mapCoords;
  if ( b_PixelMap )
  {
    s_Event = QString( "Map-Point to Pixel-Point" );
    layer_gcp_update = mLayerGcpPixels;
    added_point_update = pixelCoords;
  }
  s_Event = QString( "%1  id_gcp[%2] %3" ).arg( s_Event ).arg( id_gcp ).arg( s_map_points );
  QgsFeature fet_point_update;
  QgsFeatureRequest update_request = QgsFeatureRequest().setFilterExpression( QString( "id_gcp=%1" ).arg( id_gcp ) );
  if ( layer_gcp_update->getFeatures( update_request ).nextFeature( fet_point_update ) )
  {
    QgsGeometry geometry_update = QgsGeometry::fromPoint( added_point_update );
    fet_point_update.setGeometry( geometry_update );
    if ( layer_gcp_update->updateFeature( fet_point_update ) )
    {
      if ( saveEditsGcp( layer_gcp_update, true ) )
      {
        s_Event = QString( "%1 - %2" ).arg( s_Event ).arg( "saveEditsGcp was called." );
        QString sTextInfo = "";
        QgsGeorefDataPoint *dataPoint = new QgsGeorefDataPoint( mCanvas, mIface->mapCanvas(), pixelCoords, mapCoords, mGcpSrid, id_gcp, enable );
        dataPoint->setTextInfo( sTextInfo );
        mGcpListWidget->addDataPoint( dataPoint );
        // connect( mCanvas, &QgsMapCanvas::extentsChanged, dataPoint, &QgsGeorefDataPoint::updateCoords );
        updateGeorefTransform();
      }
    }
  }
  qDebug() << QString( "QgsGeorefPluginGui::addPoint[%1] -zz- " ).arg( s_Event );
}

// Settings slots
void QgsGeorefPluginGui::showRasterPropertiesDialog()
{
  if ( mLayerRaster )
  {
    mIface->showLayerProperties( mLayerRaster );
  }
  else
  {
    mMessageBar->pushMessage( tr( "Raster Properties" ), tr( "Please load raster to be georeferenced." ), QgsMessageBar::INFO, messageTimeout() );
  }
}

void QgsGeorefPluginGui::showGcpPixelsPropertiesDialog()
{
  if ( mLayerGcpPixels )
  {
    mIface->showLayerProperties( mLayerGcpPixels );
  }
}
void QgsGeorefPluginGui::zoomGcpPixelsExtent()
{
  if ( mLayerGcpPixels )
  {
    mCanvas->setExtent( mLayerGcpPixels->extent() );
    mCanvas->refresh();
  }
}
void QgsGeorefPluginGui::showMercatorPolygonPropertiesDialog()
{
  if ( mLayerMercatorPolygons )
  {
    mIface->showLayerProperties( mLayerMercatorPolygons );
  }
}
void QgsGeorefPluginGui::zoomMercatorPolygonExtent()
{
  if ( mLayerMercatorPolygons )
  {
    mCanvas->setExtent( mLayerMercatorPolygons->extent() );
    mCanvas->refresh();
  }
}

void QgsGeorefPluginGui::showGeorefConfigDialog()
{
  QgsGeorefConfigDialog config;
  if ( config.exec() == QDialog::Accepted )
  {
    mCanvas->refresh();
    mIface->mapCanvas()->refresh();
    QgsSettings s;
    //update dock state
    bool dock = s.value( QStringLiteral( "/Plugin-GeoReferencer/Config/ShowDocked" ) ).toBool();
    if ( dock && !mDock )
    {
      dockThisWindow( true );
    }
    else if ( !dock && mDock )
    {
      dockThisWindow( false );
    }
    //update gcp model
    if ( mGcpListWidget )
    {
      mGcpListWidget->updateGcpList();
    }
    //and status bar
    updateTransformParamLabel();
  }
}

// Histogram stretch slots
void QgsGeorefPluginGui::fullHistogramStretch()
{
  mLayerRaster->setContrastEnhancement( QgsContrastEnhancement::StretchToMinimumMaximum );
  mCanvas->refresh();
}

void QgsGeorefPluginGui::localHistogramStretch()
{
  QgsRectangle rectangle = mIface->mapCanvas()->mapSettings().outputExtentToLayerExtent( mLayerRaster, mIface->mapCanvas()->extent() );

  mLayerRaster->setContrastEnhancement( QgsContrastEnhancement::StretchToMinimumMaximum, QgsRasterMinMaxOrigin::MinMax, rectangle );
  mCanvas->refresh();
}

// Info slots
void QgsGeorefPluginGui::contextHelp()
{
  QgsGeorefDescriptionDialog dlg( this );
  dlg.exec();
}

// Comfort slots
void QgsGeorefPluginGui::jumpToGCP( QgsGeorefDataPoint *dataPoint )
{
  if ( dataPoint )
  {
    mExtentsChangedRecursionGuard = true;
    // true=Georeferencer-Canvas (isPixel)
    dataPoint->moveTo( true );
    if ( mActionLinkGeorefToQGis->isChecked() )
    {
      // false=QGis-Canvas (!isPixel)
      dataPoint->moveTo( false );
    }
    mExtentsChangedRecursionGuard = false;
  }
}

// This slot is called whenever the georeference canvas changes the displayed extent
void QgsGeorefPluginGui::extentsChangedGeorefCanvas()
{
  // Guard against endless recursion by ping-pong updates
  if ( mExtentsChangedRecursionGuard )
  {
    return;
  }

  if ( mActionLinkQGisToGeoref->isChecked() )
  {
    if ( !updateGeorefTransform() )
    {
      return;
    }

    // Reproject the georeference plugin canvas into world coordinates and fit axis aligned bounding box
    QgsRectangle rectMap = mGeorefTransform.hasCrs() ? mGeorefTransform.getBoundingBox( mCanvas->extent(), true ) : mCanvas->extent();
    QgsRectangle boundingBox = transformViewportBoundingBox( rectMap, mGeorefTransform, true );

    mExtentsChangedRecursionGuard = true;
    // Just set the whole extent for now
    // TODO: better fitting function which acounts for differing aspect ratios etc.
    mIface->mapCanvas()->setExtent( boundingBox );
    mIface->mapCanvas()->refresh();
    mExtentsChangedRecursionGuard = false;
  }
}

// This slot is called whenever the qgis main canvas changes the displayed extent
void QgsGeorefPluginGui::extentsChangedQGisCanvas()
{
  // Guard against endless recursion by ping-pong updates
  if ( mExtentsChangedRecursionGuard )
  {
    return;
  }

  if ( mActionLinkGeorefToQGis->isChecked() )
  {
    // Update transform if necessary
    if ( !updateGeorefTransform() )
    {
      return;
    }

    // Reproject the canvas into raster coordinates and fit axis aligned bounding box
    QgsRectangle boundingBox = transformViewportBoundingBox( mIface->mapCanvas()->extent(), mGeorefTransform, false );
    QgsRectangle rectMap = mGeorefTransform.hasCrs() ? mGeorefTransform.getBoundingBox( boundingBox, false ) : boundingBox;

    mExtentsChangedRecursionGuard = true;
    // Just set the whole extent for now
    // TODO: better fitting function which acounts for differing aspect ratios etc.
    mCanvas->setExtent( rectMap );
    mCanvas->refresh();
    mExtentsChangedRecursionGuard = false;
  }
}
// This slot is called whenever the qgis main Crs changes
void QgsGeorefPluginGui::onQGisCanvasCrsC2R()
{
  QString mGcpDescription = mIface->mapCanvas()->mapSettings().destinationCrs().description();
  QStringList sa_authid = mGcpAuthid.split( ":" );
  QString s_srid = "-1";
  if ( sa_authid.length() == 2 )
    s_srid = sa_authid[1];
  // int i_QGisSrid = s_srid.toInt();
}
// Canvas info slots (copy/pasted from QGIS :) )
void QgsGeorefPluginGui::showMouseCoords( const QgsPointXY &p )
{
  mCoordsLabel->setText( p.toString( mMousePrecisionDecimalPlaces ) );
  // Set minimum necessary width
  if ( mCoordsLabel->width() > mCoordsLabel->minimumWidth() )
  {
    mCoordsLabel->setMinimumWidth( mCoordsLabel->width() );
  }
}

void QgsGeorefPluginGui::updateMouseCoordinatePrecision()
{
  // Work out what mouse display precision to use. This only needs to
  // be when the s change or the zoom level changes. This
  // function needs to be called every time one of the above happens.

  // Get the display precision from the project s
  bool automatic = QgsProject::instance()->readBoolEntry( QStringLiteral( "PositionPrecision" ), QStringLiteral( "/Automatic" ) );
  int dp = 0;

  if ( automatic )
  {
    // Work out a suitable number of decimal places for the mouse
    // coordinates with the aim of always having enough decimal places
    // to show the difference in position between adjacent pixels.
    // Also avoid taking the log of 0.
    if ( mCanvas->mapUnitsPerPixel() != 0.0 )
      dp = static_cast<int>( ceil( -1.0 * log10( mCanvas->mapUnitsPerPixel() ) ) );
  }
  else
    dp = QgsProject::instance()->readNumEntry( QStringLiteral( "PositionPrecision" ), QStringLiteral( "/DecimalPlaces" ) );

  // Keep dp sensible
  if ( dp < 0 )
    dp = 0;

  mMousePrecisionDecimalPlaces = dp;
}

void QgsGeorefPluginGui::extentsChanged()
{
  if ( mAgainAddRaster )
  {
    qDebug() << QString( "-I-> QgsGeorefPluginGui::extentsChanged mAgainAddRaster[%1]" ).arg( mAgainAddRaster );
    if ( QFile::exists( mRasterFileName ) )
    {
      addRaster( mRasterFileName );
    }
    else
    {
      mLayerRaster = nullptr;
      mGcpDbData = nullptr;
      mAgainAddRaster = false;
    }
    qDebug() << QString( "-I-> QgsGeorefPluginGui::extentsChangedr [%1]" ).arg( "end" );
  }
}

// Registry layer QGis
void QgsGeorefPluginGui::layerWillBeRemoved( const QString &layerId )
{
  mAgainAddRaster = mLayerRaster && mLayerRaster->id().compare( layerId ) == 0;
}

// ------------------------------ private ---------------------------------- //
// Gui
void QgsGeorefPluginGui::createActions()
{
  // File actions
  mActionReset->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mIconClear.svg" ) ) );
  connect( mActionReset, &QAction::triggered, this, &QgsGeorefPluginGui::reset );

  mActionOpenRaster->setIcon( getThemeIcon( QStringLiteral( "/mActionAddRasterLayer.svg" ) ) );
  connect( mActionOpenRaster, &QAction::triggered, this, &QgsGeorefPluginGui::openRasterDialog );

  mActionStartGeoref->setIcon( getThemeIcon( QStringLiteral( "/mActionStartGeoref.png" ) ) );
  connect( mActionStartGeoref, &QAction::triggered, this, &QgsGeorefPluginGui::doGeoreference );

  mActionGDALScript->setIcon( getThemeIcon( QStringLiteral( "/mActionGDALScript.png" ) ) );
  connect( mActionGDALScript, &QAction::triggered, this, &QgsGeorefPluginGui::generateGDALScript );

  mActionTransformSettings->setIcon( getThemeIcon( QStringLiteral( "/mActionTransformSettings.png" ) ) );
  connect( mActionTransformSettings, &QAction::triggered, this, &QgsGeorefPluginGui::getTransformSettings );

  // Edit actions
  mActionAddFeature->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionCapturePoint.svg" ) ) );
  connect( mActionAddFeature, &QAction::triggered, this, &QgsGeorefPluginGui::setAddPointTool );
  mActionSpatialiteGcp->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionNewSpatiaLiteLayer.svg" ) ) );
  mActionSpatialiteGcp->setEnabled( false );
  connect( mActionSpatialiteGcp, &QAction::triggered, this, &QgsGeorefPluginGui::setSpatialiteGcpUsage );

  mActionFetchMasterGcpExtent->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionEditNodesItem.svg" ) ) );
  mActionFetchMasterGcpExtent->setEnabled( false );
  connect( mActionFetchMasterGcpExtent, &QAction::triggered, this, &QgsGeorefPluginGui::fetchGcpMasterPointsExtent );

  mActionNodeFeature->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionNodeTool.svg" ) ) );
  connect( mActionNodeFeature, &QAction::triggered, this, &QgsGeorefPluginGui::setNodePointTool );

  // View actions
  mActionPan->setIcon( getThemeIcon( QStringLiteral( "/mActionPan.svg" ) ) );
  connect( mActionPan, &QAction::triggered, this, &QgsGeorefPluginGui::setPanTool );

  mActionZoomIn->setIcon( getThemeIcon( QStringLiteral( "/mActionZoomIn.svg" ) ) );
  connect( mActionZoomIn, &QAction::triggered, this, &QgsGeorefPluginGui::setZoomInTool );

  mActionZoomOut->setIcon( getThemeIcon( QStringLiteral( "/mActionZoomOut.svg" ) ) );
  connect( mActionZoomOut, &QAction::triggered, this, &QgsGeorefPluginGui::setZoomOutTool );

  mActionZoomToLayer->setIcon( getThemeIcon( QStringLiteral( "/mActionZoomToLayer.svg" ) ) );
  connect( mActionZoomToLayer, &QAction::triggered, this, &QgsGeorefPluginGui::zoomToLayerTool );

  mActionZoomLast->setIcon( getThemeIcon( QStringLiteral( "/mActionZoomLast.svg" ) ) );
  connect( mActionZoomLast, &QAction::triggered, this, &QgsGeorefPluginGui::zoomToLast );

  mActionZoomNext->setIcon( getThemeIcon( QStringLiteral( "/mActionZoomNext.svg" ) ) );
  connect( mActionZoomNext, &QAction::triggered, this, &QgsGeorefPluginGui::zoomToNext );

  mActionLinkGeorefToQGis->setIcon( getThemeIcon( QStringLiteral( "/mActionLinkGeorefToQGis.png" ) ) );
  connect( mActionLinkGeorefToQGis, &QAction::triggered, this, &QgsGeorefPluginGui::linkGeorefToQGis );

  mActionLinkQGisToGeoref->setIcon( getThemeIcon( QStringLiteral( "/mActionLinkQGisToGeoref.png" ) ) );
  connect( mActionLinkQGisToGeoref, &QAction::triggered, this, &QgsGeorefPluginGui::linkQGisToGeoref );

  // Settings actions
  mActionRasterProperties->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionPropertyItem.svg" ) ) );
  connect( mActionRasterProperties, &QAction::triggered, this, &QgsGeorefPluginGui::showRasterPropertiesDialog );
  mActionGcpPixelsProperties->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionPropertyItem.svg" ) ) );
  connect( mActionGcpPixelsProperties, &QAction::triggered, this, &QgsGeorefPluginGui::showGcpPixelsPropertiesDialog );
  mActionGcpPixelsExtent->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionZoomToLayer.svg" ) ) );
  connect( mActionGcpPixelsExtent, &QAction::triggered, this, &QgsGeorefPluginGui::zoomGcpPixelsExtent );
  mActionMercatorPolygonProperties->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionPropertyItem.svg" ) ) );
  connect( mActionMercatorPolygonProperties, &QAction::triggered, this, &QgsGeorefPluginGui::showMercatorPolygonPropertiesDialog );
  mActionMercatorPolygonExtent->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionZoomToLayer.svg" ) ) );
  connect( mActionMercatorPolygonExtent, &QAction::triggered, this, &QgsGeorefPluginGui::zoomMercatorPolygonExtent );

  mActionGeorefConfig->setIcon( getThemeIcon( QStringLiteral( "/mActionGeorefConfig.png" ) ) );
  connect( mActionGeorefConfig, &QAction::triggered, this, &QgsGeorefPluginGui::showGeorefConfigDialog );

  // GcpDatabase

  mActionOpenRaster->setIcon( getThemeIcon( QStringLiteral( "/mActionAddRasterLayer.svg" ) ) );
  mActionGdalScriptType->setCheckable( true );
  mActionGdalScriptType->setChecked( mGdalScriptOrGcpList );
  QString s_text = "Switch to Gdal-Script [Full Script]";
  if ( mGdalScriptOrGcpList )
  {
    s_text = "Switch to Gdal-Script Gcp-List only]";
  }
  mActionGdalScriptType->setText( s_text );
  mActionGdalScriptType->setToolTip( s_text );
  mActionGdalScriptType->setStatusTip( s_text );
  mActionGdalScriptType->setIcon( QgsApplication::getThemeIcon( "/mActionRotatePointSymbols.svg" ) );
  connect( mActionGdalScriptType, &QAction::triggered, this, &QgsGeorefPluginGui::setGdalScriptType );
  mActionLayerProviderType->setCheckable( true );
  connect( mActionLayerProviderType, &QAction::triggered, this, &QgsGeorefPluginGui::setLayerProviderType );
  mActionPointsPolygon->setIcon( getThemeIcon( QStringLiteral( "/mActionTransformSettings.png" ) ) );
  mActionPointsPolygon->setCheckable( true );
  mActionPointsPolygon->setChecked( mPointsPolygon );
  connect( mActionPointsPolygon, &QAction::triggered, this, &QgsGeorefPluginGui::setPointsPolygon );

  mListGcpCoverages->setIcon( getThemeIcon( QStringLiteral( "/mActionTransformSettings.png" ) ) );
  connect( mListGcpCoverages, &QAction::triggered, this, &QgsGeorefPluginGui::listGcpCoverages );
  mListGcpCoverages->setEnabled( false );

  mOpenGcpCoverages->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddOgrLayer.svg" ) ) );
  connect( mOpenGcpCoverages, &QAction::triggered, this, &QgsGeorefPluginGui::openGcpCoveragesDb );
  mActionOpenGcpMaster->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddOgrLayer.svg" ) ) );
  connect( mActionOpenGcpMaster, &QAction::triggered, this, &QgsGeorefPluginGui::openGcpMasterDb );
  mActionCreateGcpDatabase->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionNewSpatiaLiteLayer.svg" ) ) );
  connect( mActionCreateGcpDatabase, &QAction::triggered, this, &QgsGeorefPluginGui::createGcpDatabaseDialog );
  mActionSqlDumpGcpCoverage->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionNewSpatiaLiteLayer.svg" ) ) );
  connect( mActionSqlDumpGcpCoverage, &QAction::triggered, this, &QgsGeorefPluginGui::createSqlDumpGcpCoverage );
  mActionSqlDumpGcpMaster->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionNewSpatiaLiteLayer.svg" ) ) );
  connect( mActionSqlDumpGcpMaster, &QAction::triggered, this, &QgsGeorefPluginGui::createSqlDumpGcpMaster );
  mActionGcpCoverageAsGcpMaster->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/symbologyAdd.svg" ) ) );
  connect( mActionGcpCoverageAsGcpMaster, &QAction::triggered, this, &QgsGeorefPluginGui::exportGcpCoverageAsGcpMaster );

  // Histogram stretch
  mActionLocalHistogramStretch->setIcon( getThemeIcon( QStringLiteral( "/mActionLocalHistogramStretch.svg" ) ) );
  connect( mActionLocalHistogramStretch, &QAction::triggered, this, &QgsGeorefPluginGui::localHistogramStretch );
  mActionLocalHistogramStretch->setEnabled( false );

  mActionFullHistogramStretch->setIcon( getThemeIcon( QStringLiteral( "/mActionFullHistogramStretch.svg" ) ) );
  connect( mActionFullHistogramStretch, &QAction::triggered, this, &QgsGeorefPluginGui::fullHistogramStretch );
  mActionFullHistogramStretch->setEnabled( false );

  // Help actions
  mActionHelp = new QAction( tr( "Help" ), this );
  connect( mActionHelp, &QAction::triggered, this, &QgsGeorefPluginGui::contextHelp );

  mActionQuit->setIcon( getThemeIcon( QStringLiteral( "/mActionFileExit.png" ) ) );
  mActionQuit->setShortcuts( QList<QKeySequence>() << QKeySequence( Qt::CTRL + Qt::Key_Q )
                             << QKeySequence( Qt::Key_Escape ) );
  connect( mActionQuit, &QAction::triggered, this, &QWidget::close );
}

void QgsGeorefPluginGui::createActionGroups()
{
  QActionGroup *mapToolGroup = new QActionGroup( this );
  mActionPan->setCheckable( true );
  mapToolGroup->addAction( mActionPan );
  mActionZoomIn->setCheckable( true );
  mapToolGroup->addAction( mActionZoomIn );
  mActionZoomOut->setCheckable( true );
  mapToolGroup->addAction( mActionZoomOut );

  mActionAddFeature->setCheckable( true );
  mapToolGroup->addAction( mActionAddFeature );
  mActionNodeFeature->setCheckable( true );
  mapToolGroup->addAction( mActionNodeFeature );
}

void QgsGeorefPluginGui::createMapCanvas()
{
  // set up the canvas
  mCanvas = new QgsMapCanvas( this->centralWidget() );
  mCanvas->setObjectName( QStringLiteral( "georefCanvas" ) );
  mCanvas->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
  // mCanvas->setCanvasColor( QColor(170,170,127) ); // aaaa7f
  QgsSettings settings;
  // Retrieve default values for main application (hoping that is not QColor::White, so that the image edges can be seen)
  int appRed = settings.value( QStringLiteral( "qgis/default_canvas_color_red" ), 255 ).toInt();
  int appGreen = settings.value( QStringLiteral( "qgis/default_canvas_color_green" ), 255 ).toInt();
  int appBlue = settings.value( QStringLiteral( "qgis/default_canvas_color_blue" ), 255 ).toInt();
  mCanvas->setCanvasColor( QColor( appRed, appGreen, appBlue ) );
  mCanvas->setMinimumWidth( 400 );
  mCentralLayout->addWidget( mCanvas, 0, 0, 2, 1 );

  // set up map tools
  mToolZoomIn = new QgsMapToolZoom( mCanvas, false /* zoomOut */ );
  mToolZoomIn->setAction( mActionZoomIn );

  mToolZoomOut = new QgsMapToolZoom( mCanvas, true /* zoomOut */ );
  mToolZoomOut->setAction( mActionZoomOut );

  mToolPan = new QgsMapToolPan( mCanvas );
  mToolPan->setAction( mActionPan );

  mToolAddPoint = new QgsMapToolAddFeature( mCanvas );
  mToolAddPoint->setAction( mActionAddFeature );
  // TODO mAdvancedDigitizingDockWidget
  QgsAdvancedDigitizingDockWidget *mAdvancedDigitizingDockWidget = nullptr;
  mAdvancedDigitizingDockWidget = new QgsAdvancedDigitizingDockWidget( mCanvas, this );
  mAdvancedDigitizingDockWidget->setObjectName( QStringLiteral( "AdvancedDigitizingToolsGeoRef" ) );
  mAdvancedDigitizingDockWidget->hide();
  mToolNodePoint =  new QgsNodeTool( mCanvas, mAdvancedDigitizingDockWidget );
  mToolNodePoint->setAction( mActionNodeFeature );

  QgsSettings s;
  double zoomFactor = s.value( QStringLiteral( "/qgis/zoom_factor" ), 2 ).toDouble();
  mCanvas->setWheelFactor( zoomFactor );

  mExtentsChangedRecursionGuard = false;

  mGeorefTransform.selectTransformParametrisation( QgsGeorefTransform::Linear );
  // Connect main canvas and georef canvas signals so we are aware if any of the viewports change
  // (used by the map follow mode)
  connect( mCanvas, &QgsMapCanvas::extentsChanged, this, &QgsGeorefPluginGui::extentsChangedGeorefCanvas );
  connect( mIface->mapCanvas(), &QgsMapCanvas::extentsChanged, this, &QgsGeorefPluginGui::extentsChangedQGisCanvas );
  connect( mIface->mapCanvas(), &QgsMapCanvas::destinationCrsChanged, this, &QgsGeorefPluginGui::onQGisCanvasCrsC2R );
}

void QgsGeorefPluginGui::createMenus()
{
  // Get platform for menu layout customization (Gnome, Kde, Mac, Win)
  QDialogButtonBox::ButtonLayout layout =
    QDialogButtonBox::ButtonLayout( style()->styleHint( QStyle::SH_DialogButtonLayout, nullptr, this ) );

  mPanelMenu = new QMenu( tr( "Panels" ) );
  mPanelMenu->setObjectName( QStringLiteral( "mPanelMenu" ) );
  mPanelMenu->addAction( dockWidgetGCPpoints->toggleViewAction() );
  //  mPanelMenu->addAction(dockWidgetLogView->toggleViewAction());

  mToolbarMenu = new QMenu( tr( "Toolbars" ) );
  mToolbarMenu->setObjectName( QStringLiteral( "mToolbarMenu" ) );
  mToolbarMenu->addAction( toolBarFile->toggleViewAction() );
  mToolbarMenu->addAction( toolBarEdit->toggleViewAction() );
  mToolbarMenu->addAction( toolBarView->toggleViewAction() );

  QgsSettings s;
  int size = s.value( QStringLiteral( "/IconSize" ), 32 ).toInt();
  toolBarFile->setIconSize( QSize( size, size ) );
  toolBarEdit->setIconSize( QSize( size, size ) );
  toolBarView->setIconSize( QSize( size, size ) );
  toolBarHistogramStretch->setIconSize( QSize( size, size ) );

  // View menu
  if ( layout != QDialogButtonBox::KdeLayout )
  {
    menuView->addSeparator();
    menuView->addMenu( mPanelMenu );
    menuView->addMenu( mToolbarMenu );
  }
  else // if ( layout == QDialogButtonBox::KdeLayout )
  {
    menuSettings->addSeparator();
    menuSettings->addMenu( mPanelMenu );
    menuSettings->addMenu( mToolbarMenu );
  }
}

void QgsGeorefPluginGui::createDockWidgets()
{
  //  mLogViewer = new QPlainTextEdit;
  //  mLogViewer->setReadOnly(true);
  //  mLogViewer->setWordWrapMode(QTextOption::NoWrap);
  //  dockWidgetLogView->setWidget(mLogViewer);

  mGcpListWidget = new QgsGcpListWidget( this );
  mGcpListWidget->setGeorefTransform( &mGeorefTransform );
  dockWidgetGCPpoints->setWidget( mGcpListWidget );

  connect( mGcpListWidget, &QgsGcpListWidget::jumpToGCP, this, &QgsGeorefPluginGui::jumpToGCP );
#if 0
  // connect( mGcpListWidget, &QgsGcpListWidget::replaceDataPoint, this, &QgsGeorefPluginGui::replaceDataPoint );
// Warning: Object::connect: No such signal QgsGcpListWidget::replaceDataPoint( QgsGeorefDataPoint*, int )
#endif
  connect( mGcpListWidget, &QgsGcpListWidget::pointEnabled, this, &QgsGeorefPluginGui::updateGeorefTransform );
  initLayerTreeView();
}

void QgsGeorefPluginGui::initLayerTreeView()
{
  // Unique names must be used to avoid confusion with the Main-Application
  // mRootLayerTreeGroup = new QgsLayerTreeGroup( tr( "Georeferencer LayersTreeGroupe" ) );
  mRootLayerTree = new QgsLayerTree();
  mLayerTreeDock = new QgsDockWidget( tr( "Georeferencer Layers Panel" ), this );
  mLayerTreeDock->setObjectName( QStringLiteral( "Georeferencer Layers" ) );
  mLayerTreeDock->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
  mLayerTreeView = new QgsLayerTreeView( this );
  mLayerTreeView->setObjectName( QStringLiteral( "GeoreferencerLayerTreeView" ) ); // "theLayerTreeView" used to find this canonical instance later
  QgsLayerTreeModel *model = new QgsLayerTreeModel( mRootLayerTree, this );
  // QgsLayerTreeModel *model = new QgsLayerTreeModel( QgsProject::instance()->layerTreeRoot(), this );
#ifdef ENABLE_MODELTEST
  new ModelTest( model, this );
#endif
  // model->setFlag( QgsLayerTreeModel::AllowNodeReorder );
  // model->setFlag( QgsLayerTreeModel::AllowNodeRename );
  model->setFlag( QgsLayerTreeModel::AllowNodeChangeVisibility );
  // model->setFlag( QgsLayerTreeModel::ShowLegendAsTree );
  //model->setFlag( QgsLayerTreeModel::UseEmbeddedWidgets );
  model->setAutoCollapseLegendNodes( 10 );
  mLayerTreeView->setModel( model );
  QVBoxLayout *vboxLayout = new QVBoxLayout;
  vboxLayout->setMargin( 0 );
  vboxLayout->setContentsMargins( 0, 0, 0, 0 );
  vboxLayout->setSpacing( 0 );
  // vboxLayout->addWidget( toolbar );
  vboxLayout->addWidget( mLayerTreeView );
  QWidget *w = new QWidget;
  w->setLayout( vboxLayout );
  mLayerTreeDock->setWidget( w );
  addDockWidget( Qt::LeftDockWidgetArea, mLayerTreeDock );
  mPanelMenu->addAction( mLayerTreeDock->toggleViewAction() );
  connect( mLayerTreeView, &QgsLayerTreeView::currentLayerChanged, this, &QgsGeorefPluginGui::activeLayerTreeViewChanged );
  connect( mLayerTreeView->layerTreeModel()->rootGroup(), &QgsLayerTreeNode::visibilityChanged, this, &QgsGeorefPluginGui::visibilityLayerTreeViewChanged );
}
void QgsGeorefPluginGui::visibilityLayerTreeViewChanged( QgsLayerTreeNode *layerTreenode )
{
  if ( layerTreenode )
  {
    bool b_visible = true;
    if ( layerTreenode->itemVisibilityChecked() == Qt::Unchecked )
    {
      b_visible = false;
    }
    if ( QgsLayerTree::isLayer( layerTreenode ) )
    {
      QgsMapLayer *map_layer = QgsLayerTree::toLayer( layerTreenode )->layer();
      if ( map_layer )
      {
        QList<QgsMapLayer *> buildMapCanvasLayers;
        for ( int i = 0; i < mMapCanvasLayers.size(); i++ )
        {
          if ( ( mMapCanvasLayers[i] != map_layer ) || ( ( mMapCanvasLayers[i] == map_layer ) && ( b_visible ) ) )
          {
            // we may have to rebuild this QList<QgsMapLayer *>, since QgsMapCanvasLayer.setvisable() is not possible
            // TODO: keep track of previous changes
            buildMapCanvasLayers.append( map_layer );
          }
        }
        mCanvas->freeze( true );
        mCanvas->setLayers( buildMapCanvasLayers );
        mCanvas->freeze( false );
        mCanvas->refresh();
      }
    }
  }
}
void QgsGeorefPluginGui::activeLayerTreeViewChanged( QgsMapLayer *map_layer )
{
  if ( mCanvas )
  {
    QgsVectorLayer *layer_vector = ( QgsVectorLayer * ) map_layer;
    if ( layer_vector )
    {
      if ( layer_vector == mLayerGcpPixels )
      {
        mCanvas->setCurrentLayer( map_layer );
        QString s_text = "Add a Gcp-Point";
        mActionAddFeature->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionCapturePoint.svg" ) ) );
        mActionAddFeature->setText( s_text );
        mActionAddFeature->setToolTip( s_text );
        mActionAddFeature->setStatusTip( s_text );
        mActionNodeFeature->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionNodeTool.svg" ) ) );
        s_text = "Change or Delete selected Gcp-Point";
        mActionNodeFeature->setText( s_text );
        mActionNodeFeature->setToolTip( s_text );
        mActionNodeFeature->setStatusTip( s_text );
        mActionSpatialiteGcp->setEnabled( !mSpatialiteGcpOff );
        mActionFetchMasterGcpExtent->setEnabled( !mSpatialiteGcpOff );
        mPointsPolygon = true;
      }
      if ( layer_vector == mLayerMercatorPolygons )
      {
        mCanvas->setCurrentLayer( map_layer );
        mActionAddFeature->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddPolygon.svg" ) ) );
        QString s_text = "Add a Raster-Cutline";
        mActionAddFeature->setText( s_text );
        mActionAddFeature->setToolTip( s_text );
        mActionAddFeature->setStatusTip( s_text );
        mActionNodeFeature->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionEditNodesItem.svg" ) ) );
        s_text = "Change selected Raster-Cutline";
        mActionNodeFeature->setText( s_text );
        mActionNodeFeature->setToolTip( s_text );
        mActionNodeFeature->setStatusTip( s_text );
        mActionFetchMasterGcpExtent->setEnabled( false );
        mActionSpatialiteGcp->setEnabled( false );
        mPointsPolygon = false;
      }
      mActionPointsPolygon->setChecked( mPointsPolygon );
    }
  }
}
QLabel *QgsGeorefPluginGui::createBaseLabelStatus()
{
  QLabel *label = new QLabel( statusBar() );
  label->setFont( QApplication::font() );
  label->setMinimumWidth( 10 );
  label->setMaximumHeight( 20 );
  label->setMargin( 3 );
  label->setAlignment( Qt::AlignCenter );
  label->setFrameStyle( QFrame::NoFrame );
  return label;
}

void QgsGeorefPluginGui::createStatusBar()
{
  mTransformParamLabel = createBaseLabelStatus();
  mTransformParamLabel->setText( tr( "Transform: " ) + convertTransformEnumToString( mTransformParam ) );
  mTransformParamLabel->setToolTip( tr( "Current transform parametrisation" ) );
  statusBar()->addPermanentWidget( mTransformParamLabel, 0 );

  mCoordsLabel = createBaseLabelStatus();
  mCoordsLabel->setMaximumWidth( 100 );
  mCoordsLabel->setText( tr( "Coordinate: " ) );
  mCoordsLabel->setToolTip( tr( "Current map coordinate" ) );
  statusBar()->addPermanentWidget( mCoordsLabel, 0 );

  mEPSG = createBaseLabelStatus();
  mEPSG->setText( QStringLiteral( "EPSG:" ) );
  statusBar()->addPermanentWidget( mEPSG, 0 );
}

void QgsGeorefPluginGui::setupConnections()
{
  connect( mCanvas, &QgsMapCanvas::xyCoordinates, this, &QgsGeorefPluginGui::showMouseCoords );
  connect( mCanvas, &QgsMapCanvas::scaleChanged, this, &QgsGeorefPluginGui::updateMouseCoordinatePrecision );

  // Connect status from ZoomLast/ZoomNext to corresponding action
  connect( mCanvas, &QgsMapCanvas::zoomLastStatusChanged, mActionZoomLast, &QAction::setEnabled );
  connect( mCanvas, &QgsMapCanvas::zoomNextStatusChanged, mActionZoomNext, &QAction::setEnabled );
  // Connect when one Layer is removed - Case where change the Projetct in QGIS
  connect( QgsProject::instance(), static_cast<void ( QgsProject::* )( const QString & )>( &QgsProject::layerWillBeRemoved ), this, &QgsGeorefPluginGui::layerWillBeRemoved );

  // Connect extents changed - Use for need add again Raster
  connect( mCanvas, &QgsMapCanvas::extentsChanged, this, &QgsGeorefPluginGui::extentsChanged );
}

void QgsGeorefPluginGui::removeOldLayer()
{
  // delete layer (and don't signal it as it's our private layer)
  if ( mLayerRaster )
  {
    clearGcpData();
  }
}

void QgsGeorefPluginGui::updateIconTheme( const QString &theme )
{
  Q_UNUSED( theme );
  // File actions
  mActionOpenRaster->setIcon( getThemeIcon( QStringLiteral( "/mActionAddRasterLayer.svg" ) ) );
  mActionStartGeoref->setIcon( getThemeIcon( QStringLiteral( "/mActionStartGeoref.png" ) ) );
  mActionGDALScript->setIcon( getThemeIcon( QStringLiteral( "/mActionGDALScript.png" ) ) );
  mActionTransformSettings->setIcon( getThemeIcon( QStringLiteral( "/mActionTransformSettings.png" ) ) );

  // Edit actions
  mActionAddFeature->setIcon( getThemeIcon( QStringLiteral( "/mActionCapturePoint.svg" ) ) );
  mActionNodeFeature->setIcon( getThemeIcon( QStringLiteral( "/mActionNodeTool.svg" ) ) );
  mActionSpatialiteGcp->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionNewSpatiaLiteLayer.svg" ) ) );
  mActionFetchMasterGcpExtent->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionEditNodesItem.svg" ) ) );

  // View actions
  mActionPan->setIcon( getThemeIcon( QStringLiteral( "/mActionPan.svg" ) ) );
  mActionZoomIn->setIcon( getThemeIcon( QStringLiteral( "/mActionZoomIn.svg" ) ) );
  mActionZoomOut->setIcon( getThemeIcon( QStringLiteral( "/mActionZoomOut.svg" ) ) );
  mActionZoomToLayer->setIcon( getThemeIcon( QStringLiteral( "/mActionZoomToLayer.svg" ) ) );
  mActionZoomLast->setIcon( getThemeIcon( QStringLiteral( "/mActionZoomLast.svg" ) ) );
  mActionZoomNext->setIcon( getThemeIcon( QStringLiteral( "/mActionZoomNext.svg" ) ) );
  mActionLinkGeorefToQGis->setIcon( getThemeIcon( QStringLiteral( "/mActionLinkGeorefToQGis.png" ) ) );
  mActionLinkQGisToGeoref->setIcon( getThemeIcon( QStringLiteral( "/mActionLinkQGisToGeoref.png" ) ) );

  // Settings actions
  mActionRasterProperties->setIcon( getThemeIcon( QStringLiteral( "/mActionRasterProperties.png" ) ) );
  mActionGeorefConfig->setIcon( getThemeIcon( QStringLiteral( "/mActionGeorefConfig.png" ) ) );
  mActionRasterProperties->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionPropertyItem.svg" ) ) );
  mActionMercatorPolygonProperties->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionPropertyItem.svg" ) ) );
  mActionMercatorPolygonExtent->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionZoomToLayer.svg" ) ) );

  // GcpDatabase
  mActionOpenRaster->setIcon( getThemeIcon( QStringLiteral( "/mActionAddRasterLayer.svg" ) ) );
  mActionGdalScriptType->setIcon( QgsApplication::getThemeIcon( "/mActionRotatePointSymbols.svg" ) );
  mActionPointsPolygon->setIcon( getThemeIcon( QStringLiteral( "/mActionTransformSettings.png" ) ) );
  mListGcpCoverages->setIcon( getThemeIcon( QStringLiteral( "/mActionTransformSettings.png" ) ) );
  mOpenGcpCoverages->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddOgrLayer.svg" ) ) );
  mActionOpenGcpMaster->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddOgrLayer.svg" ) ) );
  mActionCreateGcpDatabase->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionNewSpatiaLiteLayer.svg" ) ) );
  mActionSqlDumpGcpCoverage->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionNewSpatiaLiteLayer.svg" ) ) );
  mActionSqlDumpGcpMaster->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionNewSpatiaLiteLayer.svg" ) ) );

  // Histogram stretch
  mActionLocalHistogramStretch->setIcon( getThemeIcon( QStringLiteral( "/mActionLocalHistogramStretch.svg" ) ) );
  mActionFullHistogramStretch->setIcon( getThemeIcon( QStringLiteral( "/mActionFullHistogramStretch.svg" ) ) );

  mActionQuit->setIcon( getThemeIcon( QStringLiteral( "/mActionFileExit.png" ) ) );

}

// Mapcanvas Plugin
void QgsGeorefPluginGui::addRaster( const QString &rasterFileName )
{
  QDateTime functionDuration = QDateTime::currentDateTime();
  mCanvas->freeze( true );
  mIface->mapCanvas()->freeze( true );
  if ( mLayerRaster )
  {
    clearGcpData();
  }
  mLayerRaster = new QgsRasterLayer( rasterFileName, mGcpBaseFileName );
  mCanvas->setDestinationCrs( mLayerRaster->crs() );
  // the Layer should be loaded just before loadGCPs is called - will read metadata from file and set layer
  if ( loadGcpData() )
  {
    // so layer is not added to legend
    QgsProject::instance()->addMapLayers( QList<QgsMapLayer *>() << mLayerRaster, false, false );
    // add layer to map canvas
    mMapCanvasLayers.clear();
    if ( isGcpDb() )
    {
      if ( mLayerMercatorPolygons )
      {
        mMapCanvasLayers.append( mLayerMercatorPolygons );
      }
      if ( mLayerGcpPixels )
      {
        mMapCanvasLayers.append( mLayerGcpPixels );
      }
      if ( isGcpEnabled() )
      {
        // mLayerRaster width and height are needed
        updateGcpCompute( mGcpCoverageName );
      }
    }
    // map layer must be the last
    mMapCanvasLayers.append( mLayerRaster );
    mCanvas->setLayers( mMapCanvasLayers );

#if 0
    QStringList list_layers;
    for ( int i_list = 0; i_list < mCanvas->mapSettings().layers().size(); i_list++ )
    {
      // the first is on top of the second, map layer must be the last
      list_layers.append( mCanvas->mapSettings().layers()[ i_list]->originalName() );
      qDebug() << QString( "-II-> QgsGeorefPluginGui::addRaster[%1,%2] " ).arg( i_list ).arg( mCanvas->mapSettings().layers()[ i_list]->originalName() );
    }
#endif

    mAgainAddRaster = false;

    mActionLocalHistogramStretch->setEnabled( true );
    mActionFullHistogramStretch->setEnabled( true );

    // Status Bar
    if ( mGeorefTransform.hasCrs() )
    {
      QString authid = mLayerRaster->crs().authid();
      mEPSG->setText( authid );
      mEPSG->setToolTip( mLayerRaster->crs().toProj4() );
    }
    else
    {
      mEPSG->setText( tr( "None" ) );
      mEPSG->setToolTip( tr( "Coordinate of image(column/line)" ) );
    }

    // mExtentsChangedRecursionGuard = true;
    if ( mActionLinkQGisToGeoref->isChecked() )
    {
      // Reproject the canvas into raster coordinates and fit axis aligned bounding box
      QgsRectangle boundingBox = transformViewportBoundingBox( mIface->mapCanvas()->extent(), mGeorefTransform, false );
      QgsRectangle rectMap = mGeorefTransform.hasCrs() ? mGeorefTransform.getBoundingBox( boundingBox, false ) : boundingBox;
      // Just set the whole extent for now
      // TODO: better fitting function which acounts for differing aspect ratios etc.
      mCanvas->setExtent( rectMap );
    }
    else
    {
      mCanvas->setExtent( mLayerRaster->extent() );
      // Reproject the georeference plugin canvas into world coordinates and fit axis aligned bounding box
      QgsRectangle rectMap = mGeorefTransform.hasCrs() ? mGeorefTransform.getBoundingBox( mCanvas->extent(), true ) : mCanvas->extent();
      QgsRectangle boundingBox = transformViewportBoundingBox( rectMap, mGeorefTransform, true );
      // Just set the whole extent for now
      // TODO: better fitting function which acounts for differing aspect ratios etc.
      mIface->mapCanvas()->setExtent( boundingBox );
    }
    mCanvas->freeze( false );
    mIface->mapCanvas()->freeze( false );
    mCanvas->refresh();
    mIface->mapCanvas()->refresh();
    qDebug() << QString( "-I-> QgsGeorefPluginGui::addRaster[%1] Raster[%2] duration[%3]" ).arg( mLayerProviderType ).arg( mGcpBaseFileName ).arg( functionDuration.msecsTo( QDateTime::currentDateTime() ) );
    // -I-> QgsGeorefPluginGui::addRaster[ogr.01] Raster[1910.Straube_Blatt_Uebersicht.26500] duration[4015]
    // -I-> QgsGeorefPluginGui::addRaster[ogr.01] Raster[1909.Straube_Blatt_III_A.4000] duration[59434]
    // -I-> QgsGeorefPluginGui::addRaster[ogr.02] Raster[1910.Straube_Blatt_Uebersicht.26500] duration[4843]
    // -I-> QgsGeorefPluginGui::addRaster[ogr] Raster[1909.Straube_Blatt_III_A.4000] duration[59452]
    // -I-> QgsGeorefPluginGui::addRaster[spatialite.01] Raster[1910.Straube_Blatt_Uebersicht.26500] duration[3237]
    // -I-> QgsGeorefPluginGui::addRaster[spatialite.01] Raster[1909.Straube_Blatt_III_A.4000] duration[59794]
  }
  else
  {
    mCanvas->freeze( false );
    mIface->mapCanvas()->freeze( false );
  }
}

// Settings
void QgsGeorefPluginGui::readSettings()
{
  QgsSettings s;
  QRect georefRect = QApplication::desktop()->screenGeometry( mIface->mainWindow() );
  resize( s.value( QStringLiteral( "/Plugin-GeoReferencer/size" ), QSize( georefRect.width() / 2 + georefRect.width() / 5,
                   mIface->mainWindow()->height() ) ).toSize() );
  move( s.value( QStringLiteral( "/Plugin-GeoReferencer/pos" ), QPoint( parentWidget()->width() / 2 - width() / 2, 0 ) ).toPoint() );
  restoreState( s.value( QStringLiteral( "/Plugin-GeoReferencer/uistate" ) ).toByteArray() );

  // warp options
  mResamplingMethod = ( QgsImageWarper::ResamplingMethod )s.value( QStringLiteral( "/Plugin-GeoReferencer/resamplingmethod" ),
                      QgsImageWarper::NearestNeighbour ).toInt();
  mCompressionMethod = s.value( QStringLiteral( "/Plugin-GeoReferencer/compressionmethod" ), "NONE" ).toString();
  mUseZeroForTrans = s.value( QStringLiteral( "/Plugin-GeoReferencer/usezerofortrans" ), false ).toBool();
}

void QgsGeorefPluginGui::writeSettings()
{
  QgsSettings s;
  s.setValue( QStringLiteral( "/Plugin-GeoReferencer/pos" ), pos() );
  s.setValue( QStringLiteral( "/Plugin-GeoReferencer/size" ), size() );
  s.setValue( QStringLiteral( "/Plugin-GeoReferencer/uistate" ), saveState() );

  // warp options
  s.setValue( QStringLiteral( "/Plugin-GeoReferencer/transformparam" ), mTransformParam );
  s.setValue( QStringLiteral( "/Plugin-GeoReferencer/resamplingmethod" ), mResamplingMethod );
  s.setValue( QStringLiteral( "/Plugin-GeoReferencer/compressionmethod" ), mCompressionMethod );
  s.setValue( QStringLiteral( "/Plugin-GeoReferencer/usezerofortrans" ), mUseZeroForTrans );
}
QString QgsGeorefPluginGui::formatUrl( const QString &sGcpDatabaseFileName, const QString &sGcpTableName, const QString &sGcpGeometryName )
{
  if ( mFormatUrl == "" )
  {
    if ( mLayerProviderType == "ogr" )
    {
      mFormatUrl = QString( "%1|layername=%2|geometryname=%3" );
      mFormatUrl = QString( "%1|layername=%2(%3)|ogrtype=0" );
      mFormatUrl = QString( "%1|layername=%2(%3)" );
    }
    else
    {
      mFormatUrl = QString( "dbname=%1 table=%2 (%3)" );
    }
  }
  if ( mCutlinePointsLayername.isNull() )
  {
    mCutlinePointsLayername = QString( "%1(%2)" ).arg( mCutlinePointsTableName ).arg( mCutlinePointsGeometryName );
    mCutlineLinestringsLayername = QString( "%1(%2)" ).arg( mCutlineLinestringsTableName ).arg( mCutlineLinestringsGeometryName );
    mCutlinePolygonsLayername = QString( "%1(%2)" ).arg( mCutlinePolygonsTableName ).arg( mCutlinePolygonsGeometryName );
    mGcpMasterLayername = QString( "%1(%2)" ).arg( mGcpMasterTableName ).arg( mGcpMasterGeometryName );
    mMercatorCutlinePolygonsLayername = QString( "%1(%2)" ).arg( mMercatorPolygonsTableName ).arg( mMercatorCutlinePolygonsGeometryName );
    mMercatorPolygonsLayername = QString( "%1(%2)" ).arg( mMercatorPolygonsTableName ).arg( mMercatorPolygonsGeometryName );
  }
  return QString( mFormatUrl ).arg( sGcpDatabaseFileName ).arg( sGcpTableName ).arg( sGcpGeometryName );
}

// GCP points
bool QgsGeorefPluginGui::loadGcpData( /*bool verbose*/ )
{
  QFile points_file( mGcpPointsFileName );
  // will create needed gcp_database, if it does not exist
  // - also import an existing '.point' file
  // check if the used spatialite has GCP-enabled
  bool bDatabaseExists = createGcpDb();
  if ( bDatabaseExists )
  {
    if ( ( ( mGcpDatabaseFileName.isNull() ) || ( mGcpDatabaseFileName.isEmpty() ) ) &&
         ( !mGcpDbData->mGcpDatabaseFileName.isEmpty() ) )
    {
      // Should never happen, but lets make sure
      mGcpDatabaseFileName = mGcpDbData->mGcpDatabaseFileName;
    }
    mGcpListWidget->setGcpDbData( mGcpDbData, true );
    mGcpPointsLayername = QString( "%1(%2)" ).arg( mGcpPointsTableName ).arg( mGcpPointsGeometryName );
    QString sLayerFormatUrl = formatUrl( mGcpDatabaseFileName, mGcpPointsTableName, mGcpPointsGeometryName );
    mLayerGcpPoints = new QgsVectorLayer( sLayerFormatUrl, mGcpPointsLayername,  mLayerProviderType );
    if ( ( ( !mLayerGcpPoints ) || ( !mLayerGcpPoints->isValid() ) ) && ( mLayerProviderType == "ogr" ) )
    {
      qDebug() << QString( "-W-> QgsGeorefPluginGui::loadGCPs[%1] GcpPoints not loaded  with Provider[%2]" ).arg( mGcpPointsLayername ).arg( mLayerProviderType );
      // This Ogr-Provider-Version cannot read a Spatialite-Table with more than 1 Geometry, revert to Spatialite-Provider
      setLayerProviderType();
      sLayerFormatUrl = formatUrl( mGcpDatabaseFileName, mGcpPointsTableName, mGcpPointsGeometryName );
      mLayerGcpPoints = new QgsVectorLayer( sLayerFormatUrl, mGcpPointsLayername,  mLayerProviderType );
    }
    if ( setGcpLayerSettings( mLayerGcpPoints ) )
    {
      // pointer and isValid == true ; default Label settings done
      mGcpPixelLayername = QString( "%1(%2)" ).arg( mGcpPointsTableName ).arg( mGcpPixelGeometryName );
      sLayerFormatUrl = formatUrl( mGcpDatabaseFileName, mGcpPointsTableName, mGcpPixelGeometryName );
      mLayerGcpPixels = new QgsVectorLayer( sLayerFormatUrl, mGcpPixelLayername, mLayerProviderType );
      if ( setGcpLayerSettings( mLayerGcpPixels ) )
      {
        // add pixels-layer to georeferencer-map canvas and add points-layer to georeferencergroup in application QgsLayerTree
        QgsProject::instance()->addMapLayers( QList<QgsMapLayer *>()  << mLayerGcpPixels, false );
        if ( !mLayerCutlinePoints )
        {
          // general geometry type as helpers during georeferencing, but also as cutline
          sLayerFormatUrl = formatUrl( mGcpDatabaseFileName, mCutlinePointsTableName, mCutlinePointsGeometryName );
          mLayerCutlinePoints = new QgsVectorLayer( sLayerFormatUrl, mCutlinePointsTableName, mLayerProviderType );
          sLayerFormatUrl = formatUrl( mGcpDatabaseFileName, mCutlineLinestringsTableName, mCutlineLinestringsGeometryName );
          mLayerCutlineLinestrings = new QgsVectorLayer( sLayerFormatUrl, mCutlineLinestringsLayername,  mLayerProviderType );
          sLayerFormatUrl = formatUrl( mGcpDatabaseFileName, mCutlinePolygonsTableName, mCutlinePolygonsGeometryName );
          mLayerCutlinePolygons = new QgsVectorLayer( sLayerFormatUrl, mCutlinePolygonsLayername,  mLayerProviderType );
          sLayerFormatUrl = formatUrl( mGcpDatabaseFileName, mMercatorPolygonsTableName, mMercatorPolygonsGeometryName );
          mLayerMercatorPolygons = new QgsVectorLayer( sLayerFormatUrl, mMercatorPolygonsLayername,  mLayerProviderType );
          sLayerFormatUrl = formatUrl( mGcpDatabaseFileName, mMercatorPolygonsTableName, mMercatorCutlinePolygonsGeometryName );
          mLayerMercatorCutlinePolygons = new QgsVectorLayer( sLayerFormatUrl, mMercatorCutlinePolygonsLayername,  mLayerProviderType );
        }
        QList<QgsMapLayer *> layers_points_cutlines;
        QList<QgsMapLayer *> layers_points_mercator;
        layers_points_mercator << mLayerGcpPixels;
        layers_points_cutlines << mLayerGcpPoints;
        // layerTreeRoot=QgsLayerTreeGroup
        mTreeGroupGeoreferencer = QgsProject::instance()->layerTreeRoot()->insertGroup( 0, QString( "Georeferencer" ) );
        int i_group_position = 0;
        mTreeGroupGcpPoints = mTreeGroupGeoreferencer->insertGroup( i_group_position++, mGcpCoverageName );
        if ( setCutlineLayerSettings( mLayerCutlinePoints ) )
        {
          // Add only valid layers
          layers_points_cutlines << mLayerCutlinePoints;
          if ( setCutlineLayerSettings( mLayerCutlineLinestrings ) )
          {
            // Add only valid layers
            layers_points_cutlines << mLayerCutlineLinestrings;
            if ( setCutlineLayerSettings( mLayerCutlinePolygons ) )
            {
              // Add only valid layers
              layers_points_cutlines << mLayerCutlinePolygons;
              mTreeGroupGcpCutlines = mTreeGroupGeoreferencer->insertGroup( i_group_position++,  "Gcp_Cutlines" );
              if ( setCutlineLayerSettings( mLayerMercatorPolygons ) )
              {
                // Add only valid layers
                // layers_points_cutlines << mLayerMercatorPolygons;
                layers_points_mercator << mLayerMercatorPolygons;
                loadGcpMaster(); // Will be added if exists
                if ( mTreeGroupGcpMaster )
                {
                  // Insure that 'mTreeGroupGcpMaster' comes before the 'mTreeGroupGtifRasters'
                  i_group_position++;
                }
                if ( setCutlineLayerSettings( mLayerMercatorCutlinePolygons ) )
                {
                  // Add only valid layers
                  layers_points_cutlines << mLayerMercatorCutlinePolygons;
                }
              }
            }
          }
        }
        // Note: since they are being placed in our groups, 'false' must be used, avoiding the automatic inserting into the layerTreeRoot.
        QgsProject::instance()->addMapLayers( layers_points_cutlines, false );
        mTreeGroupGcpPoints->insertLayer( 0, mLayerGcpPoints );
        if ( mTreeGroupGcpCutlines )
        {
          // must be added AFTER being registered !
          mTreeGroupGcpCutlines->insertLayer( 0, mLayerCutlinePoints );
          connect( mLayerCutlinePoints, &QgsVectorLayer::featureAdded, this, &QgsGeorefPluginGui::featureAdded_cutline );
          connect( mLayerCutlinePoints, &QgsVectorLayer::geometryChanged, this, &QgsGeorefPluginGui::geometryChanged_cutline );
          mLayerCutlinePoints->startEditing();
          mTreeGroupGcpCutlines->insertLayer( 1, mLayerCutlineLinestrings );
          connect( mLayerCutlineLinestrings, &QgsVectorLayer::featureAdded, this, &QgsGeorefPluginGui::featureAdded_cutline );
          connect( mLayerCutlineLinestrings, &QgsVectorLayer::geometryChanged, this, &QgsGeorefPluginGui::geometryChanged_cutline );
          mLayerCutlineLinestrings->startEditing();
          mTreeGroupGcpCutlines->insertLayer( 2, mLayerCutlinePolygons );
          connect( mLayerCutlinePolygons, &QgsVectorLayer::featureAdded, this, &QgsGeorefPluginGui::featureAdded_cutline );
          connect( mLayerCutlinePolygons, &QgsVectorLayer::geometryChanged, this, &QgsGeorefPluginGui::geometryChanged_cutline );
          mLayerCutlinePolygons->startEditing();
          if ( mLayerMercatorCutlinePolygons )
          {
            // Will be shown in the QGIS Application
            mTreeGroupGcpCutlines->insertLayer( 3, mLayerMercatorCutlinePolygons );
#if 0
            connect( mLayerMercatorCutlinePolygons, &QgsVectorLayer::featureAdded, this, &QgsGeorefPluginGui::featureAdded_cutline );
            connect( mLayerMercatorCutlinePolygons, &QgsVectorLayer::geometryChanged, this, &QgsGeorefPluginGui::geometryChanged_cutline );
#endif
            mLayerMercatorCutlinePolygons->startEditing();
          }
          if ( mLayerMercatorPolygons )
          {
            // Will be shown in the Georeferencer
            QgsProject::instance()->addMapLayers( QList<QgsMapLayer *>() << layers_points_mercator, false );
            mTreeGroupCutlineMercator = mRootLayerTree->insertGroup( 0,  "Gcp_Mercator" );
            mTreeGroupCutlineMercator->insertLayer( 0, mLayerMercatorPolygons );
            mTreeGroupCutlineMercator->insertLayer( 1, mLayerGcpPixels );
            connect( mLayerMercatorPolygons, &QgsVectorLayer::featureAdded, this, &QgsGeorefPluginGui::featureAdded_cutline );
            connect( mLayerMercatorPolygons, &QgsVectorLayer::geometryChanged, this, &QgsGeorefPluginGui::geometryChanged_cutline );
            mLayerMercatorPolygons->startEditing();
          }
        }
        mTreeGroupGtifRasters = mTreeGroupGeoreferencer->insertGroup( i_group_position++,  "Gcp_GTif-Rasters" );
        if ( mTreeGroupGtifRasters )
        {
          // If configured to do so and if file exists, will be loaded
          QFileInfo gtif_file( QString( "%1/%2" ).arg( mRasterFilePath ).arg( mModifiedRasterFileName ) );
          loadGTifInQgis( gtif_file.canonicalFilePath() );
        }
#if 0
        exportLayerDefinition( mTreeGroupGcpCutlines );
#endif
        // Shown in the Georeferencer
        mLayerGcpPixels->startEditing();
        connect( mLayerGcpPixels, &QgsVectorLayer::featureAdded, this, &QgsGeorefPluginGui::featureAdded_gcp );
        connect( mLayerGcpPixels, &QgsVectorLayer::geometryChanged, this, &QgsGeorefPluginGui::geometryChanged_gcp );
        connect( mLayerGcpPixels, &QgsVectorLayer::attributeValueChanged, this, &QgsGeorefPluginGui::attributeChanged_gcp );
        // Shown in the QGIS Application
        mLayerGcpPoints->startEditing();
        connect( mLayerGcpPoints, &QgsVectorLayer::featureAdded, this, &QgsGeorefPluginGui::featureAdded_gcp );
        connect( mLayerGcpPoints, &QgsVectorLayer::geometryChanged, this, &QgsGeorefPluginGui::geometryChanged_gcp );
        connect( mLayerGcpPoints, &QgsVectorLayer::attributeValueChanged, this, &QgsGeorefPluginGui::attributeChanged_gcp );
        QgsAttributeList lst_needed_fields;
        lst_needed_fields.append( 0 ); // id_gcp
        lst_needed_fields.append( 2 ); // name
        lst_needed_fields.append( 3 ); // notes
        lst_needed_fields.append( 10 ); // gcp_enable
        QgsFeatureIterator fit_pixels = mLayerGcpPixels->getFeatures( QgsFeatureRequest().setSubsetOfAttributes( lst_needed_fields ) );
        QgsFeature fet_pixel;
        QgsFeatureIterator fit_points = mLayerGcpPoints->getFeatures( QgsFeatureRequest().setSubsetOfAttributes( QgsAttributeList() ) );
        QgsFeature fet_point;
        int i_count = 0;
        while ( fit_pixels.nextFeature( fet_pixel ) )
        {
          if ( fet_pixel .geometry() && fet_pixel.geometry().type() == QgsWkbTypes::PointGeometry )
          {
            fit_points.nextFeature( fet_point );
            if ( fet_point .geometry() && fet_point.geometry().type() == QgsWkbTypes::PointGeometry )
            {
              int id_gcp = fet_pixel.attribute( QString( "id_gcp" ) ).toInt();
              int enable = fet_pixel.attribute( QString( "gcp_enable" ) ).toInt();
              QgsPointXY pt_pixel = fet_pixel.geometry().vertexAt( 0 );
              QgsPointXY pt_point = fet_point.geometry().vertexAt( 0 );
              QString sTextInfo  = QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( id_gcp );
              sTextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( mIdGcpCoverage );
              sTextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( mGcpPointsTableName );
              sTextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( fet_pixel.attribute( QString( "name" ) ).toString() );
              sTextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( fet_pixel.attribute( QString( "notes" ) ).toString() );
              sTextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( getGcpTransformParam( mTransformParam ) );
              i_count++;
              // id_gcp=fet_point.id();
              if ( fet_pixel.id() != fet_point.id() )
                qDebug() << QString( "QgsGeorefPluginGui::loadGCPs[%1] pixel.id[%2] fet_point.id[%3]" ).arg( id_gcp ).arg( fet_pixel.id() ).arg( fet_point.id() );
              QgsGeorefDataPoint *dataPoint = new QgsGeorefDataPoint( mCanvas, mIface->mapCanvas(), pt_pixel, pt_point, mGcpSrid, id_gcp, enable );
              dataPoint->setTextInfo( sTextInfo );
              mGcpListWidget->addDataPoint( dataPoint );
            }
          }
        }
      }
      else
      {
        mLayerGcpPoints = nullptr;
      }
    }
    else
    {
      qDebug() << QString( "-W-> QgsGeorefPluginGui::loadGCPs[%1] GcpPoints not loaded  (mLayerGcpPoints == NULL)" ).arg( mGcpPointsLayername );
    }
  }
  if ( mGcpListWidget->isDirty() )
  {
    // Informs QgsGcpList that the Initial loading of Point-Pairs has been completed
    mGcpListWidget->setChanged( true );
    updateGeorefTransform();
    if ( mLayerGcpPoints )
    {
      mIface->layerTreeView()->setCurrentLayer( mLayerGcpPoints );
      mLayerTreeView->setCurrentLayer( mLayerGcpPixels );
      mPointsPolygon = true;
      mActionPointsPolygon->setChecked( mPointsPolygon );
      mActionFetchMasterGcpExtent->setEnabled( true );
      mSpatialiteGcpOff = false;
      if ( ( !isGcpOff() ) && ( mGcpListWidget->countDataPointsEnabled() > 3 ) )
      {
        mSpatialiteGcpOff = true;
      }
      // Will set the opposite and all the text
      setSpatialiteGcpUsage();
    }
  }
  return true;
}
bool QgsGeorefPluginGui::loadGcpMaster( QString  sDatabaseFilename )
{
  bool b_rc = false;
  if ( sDatabaseFilename.isNull() )
  {
    if ( !mGcpMasterDatabaseFileName.isNull() )
    {
      sDatabaseFilename = mGcpMasterDatabaseFileName;
    }
    else
    {
      QgsSettings s;
      QString s_gcp_master = s.value( QStringLiteral( "/Plugin-GeoReferencer/gcp_master_db" ), "" ).toString();
      if ( ( !s_gcp_master.isEmpty() ) && ( QFile::exists( s_gcp_master ) ) )
      {
        mGcpMasterDatabaseFileName = s_gcp_master;
        sDatabaseFilename = mGcpMasterDatabaseFileName;
      }
    }
  }
  if ( !sDatabaseFilename.isNull() )
  {
    QFileInfo database_file( sDatabaseFilename );
    if ( database_file.exists() )
    {
      int i_return_srid = createGcpMasterDb( database_file.canonicalFilePath() );
      if ( i_return_srid != INT_MIN )
      {
        // will return srid being used in the gcp_master, otherwise INT_MIN if not found
        mGcpMasterDatabaseFileName = database_file.canonicalFilePath();
        mGcpMasterSrid = i_return_srid;
        if ( mTreeGroupGeoreferencer )
        {
          if ( !mTreeGroupGcpMaster )
          {
            mTreeGroupGcpMaster = mTreeGroupGeoreferencer->addGroup( "Gcp_Master" );
          }
          QString sLayerFormatUrl = formatUrl( database_file.canonicalFilePath(), mGcpMasterTableName, mGcpMasterGeometryName );
          mLayerGcpMaster = new QgsVectorLayer( sLayerFormatUrl, mGcpMasterLayername,  mLayerProviderType );
          if ( setCutlineLayerSettings( mLayerGcpMaster ) )
          {
            QgsProject::instance()->addMapLayers( QList<QgsMapLayer *>() << mLayerGcpMaster, false );
            mTreeGroupGcpMaster->insertLayer( 0, mLayerGcpMaster );
            connect( mLayerGcpMaster, &QgsVectorLayer::featureAdded, this, &QgsGeorefPluginGui::featureAdded_cutline );
            connect( mLayerGcpMaster, &QgsVectorLayer::geometryChanged, this, &QgsGeorefPluginGui::geometryChanged_cutline );
            mLayerGcpMaster->startEditing();
            b_rc = true;
          }
        }
      }
    }
  }
  return b_rc;
}

void QgsGeorefPluginGui::saveGCPs()
{
  if ( isGcpDb() )
  {
    updateGcpDb( mGcpCoverageName );
    if ( isGcpEnabled() )
    {
      // mLayerRaster width and height are needed
      updateGcpCompute( mGcpCoverageName );
    }
  }
  if ( bGcpFileName )
  {
    QFile gcpFile( mGcpFileName );
    if ( gcpFile.open( QIODevice::WriteOnly ) )
    {
      QTextStream gcps( &gcpFile );
      gcps << generateGDALGcpList();
    }
  }
  //  showMessageInLog(tr("GCP points saved in"), mGcpPointsFileName);
}

// Georeference
bool QgsGeorefPluginGui::georeference()
{
  if ( !checkReadyGeoref() )
    return false;

  if ( mModifiedRasterFileName.isEmpty() && ( QgsGeorefTransform::Linear == mGeorefTransform.transformParametrisation() ||
       QgsGeorefTransform::Helmert == mGeorefTransform.transformParametrisation() ) )
  {
    QgsPointXY origin;
    double pixelXSize, pixelYSize, rotation;
    if ( !mGeorefTransform.getOriginScaleRotation( origin, pixelXSize, pixelYSize, rotation ) )
    {
      mMessageBar->pushMessage( tr( "Transform Failed" ), tr( "Failed to calculate linear transform parameters." ), QgsMessageBar::WARNING, messageTimeout() );
      return false;
    }

    if ( !mWorldFileName.isEmpty() )
    {
      if ( QFile::exists( mWorldFileName ) )
      {
        int r = QMessageBox::question( this, tr( "World file exists" ),
                                       tr( "<p>The selected file already seems to have a "
                                           "world file! Do you want to replace it with the "
                                           "new world file?</p>" ),
                                       QMessageBox::Yes | QMessageBox::Default,
                                       QMessageBox::No | QMessageBox::Escape );
        if ( r == QMessageBox::No )
          return false;
        else
          QFile::remove( mWorldFileName );
      }
    }
    else
    {
      return false;
    }

    if ( !writeWorldFile( origin, pixelXSize, pixelYSize, rotation ) )
    {
      return false;
    }
    if ( !mPdfOutputFile.isEmpty() )
    {
      writePDFReportFile( mPdfOutputFile, mGeorefTransform );
    }
    if ( !mPdfOutputMapFile.isEmpty() )
    {
      writePDFMapFile( mPdfOutputMapFile, mGeorefTransform );
    }
    return true;
  }
  else // Helmert, Polinom 1, Polinom 2, Polinom 3
  {
    QgsImageWarper warper( this );
    int res = warper.warpFile( mRasterFileName, mModifiedRasterFileName, mGeorefTransform,
                               mResamplingMethod, mUseZeroForTrans, mCompressionMethod, mProjection, mUserResX, mUserResY );
    if ( res == 0 ) // fault to compute GCP transform
    {
      //TODO: be more specific in the error message
      mMessageBar->pushMessage( tr( "Transform Failed" ), tr( "Failed to compute GCP transform: Transform is not solvable." ), QgsMessageBar::WARNING, messageTimeout() );
      return false;
    }
    else if ( res == -1 ) // operation canceled
    {
      QFileInfo modified_file( mModifiedRasterFileName );
      modified_file.dir().remove( mModifiedRasterFileName );
      return false;
    }
    else // 1 all right
    {
      if ( !mPdfOutputFile.isEmpty() )
      {
        writePDFReportFile( mPdfOutputFile, mGeorefTransform );
      }
      if ( !mPdfOutputMapFile.isEmpty() )
      {
        writePDFMapFile( mPdfOutputMapFile, mGeorefTransform );
      }
      return true;
    }
  }
}

bool QgsGeorefPluginGui::writeWorldFile( const QgsPointXY &origin, double pixelXSize, double pixelYSize, double rotation )
{
  // write the world file
  QFile file( mWorldFileName );
  if ( !file.open( QIODevice::WriteOnly ) )
  {
    mMessageBar->pushMessage( tr( "Error" ), tr( "Could not write to %1." ).arg( mWorldFileName ), QgsMessageBar::CRITICAL, messageTimeout() );
    return false;
  }

  double rotationX = 0;
  double rotationY = 0;

  if ( !qgsDoubleNear( rotation, 0.0 ) )
  {
    rotationX = pixelXSize * sin( rotation );
    rotationY = pixelYSize * sin( rotation );
    pixelXSize *= cos( rotation );
    pixelYSize *= cos( rotation );
  }

  QTextStream stream( &file );
  stream << qgsDoubleToString( pixelXSize ) << endl
         << rotationX << endl
         << rotationY << endl
         << qgsDoubleToString( -pixelYSize ) << endl
         << qgsDoubleToString( origin.x() ) << endl
         << qgsDoubleToString( origin.y() ) << endl;
  return true;
}

bool QgsGeorefPluginGui::writePDFMapFile( const QString &fileName, const QgsGeorefTransform &transform )
{
  Q_UNUSED( transform );
  if ( !mCanvas )
  {
    return false;
  }

  QgsRasterLayer *rlayer = ( QgsRasterLayer * ) mCanvas->layer( 0 );
  if ( !rlayer )
  {
    return false;
  }
  double mapRatio =  rlayer->extent().width() / rlayer->extent().height();

  QPrinter printer;
  printer.setOutputFormat( QPrinter::PdfFormat );
  printer.setOutputFileName( fileName );

  QgsSettings s;
  double paperWidth = s.value( QStringLiteral( "/Plugin-GeoReferencer/Config/WidthPDFMap" ), "297" ).toDouble();
  double paperHeight = s.value( QStringLiteral( "/Plugin-GeoReferencer/Config/HeightPDFMap" ), "420" ).toDouble();

  //create composition
  QgsComposition *composition = new QgsComposition( QgsProject::instance() );
  if ( mapRatio >= 1 )
  {
    composition->setPaperSize( paperHeight, paperWidth );
  }
  else
  {
    composition->setPaperSize( paperWidth, paperHeight );
  }
  composition->setPrintResolution( 300 );
  printer.setPaperSize( QSizeF( composition->paperWidth(), composition->paperHeight() ), QPrinter::Millimeter );

  double leftMargin = 8;
  double topMargin = 8;
  double contentWidth = composition->paperWidth() - 2 * leftMargin;
  double contentHeight = composition->paperHeight() - 2 * topMargin;

  //composer map
  QgsComposerMap *composerMap = new QgsComposerMap( composition, leftMargin, topMargin, contentWidth, contentHeight );
  composerMap->setKeepLayerSet( true );
  QgsMapLayer *firstLayer = mCanvas->mapSettings().layers()[0];
  composerMap->setLayers( QList<QgsMapLayer *>() << firstLayer );
  composerMap->zoomToExtent( rlayer->extent() );
  composition->addItem( composerMap );
  printer.setFullPage( true );
  printer.setColorMode( QPrinter::Color );

  QString residualUnits;
  if ( s.value( QStringLiteral( "/Plugin-GeoReferencer/Config/ResidualUnits" ) ) == "mapUnits" && mGeorefTransform.providesAccurateInverseTransformation() )
  {
    residualUnits = tr( "map units" );
  }
  else
  {
    residualUnits = tr( "pixels" );
  }

  //residual plot
  QgsResidualPlotItem *resPlotItem = new QgsResidualPlotItem( composition );
  composition->addItem( resPlotItem );
  resPlotItem->setSceneRect( QRectF( leftMargin, topMargin, contentWidth, contentHeight ) );
  resPlotItem->setExtent( composerMap->extent() );
  resPlotItem->setGcpList( getGcpList() );
  resPlotItem->setConvertScaleToMapUnits( residualUnits == tr( "map units" ) );

  printer.setResolution( composition->printResolution() );
  QPainter p( &printer );
  composition->setPlotStyle( QgsComposition::Print );
  QRectF paperRectMM = printer.pageRect( QPrinter::Millimeter );
  QRectF paperRectPixel = printer.pageRect( QPrinter::DevicePixel );
  composition->render( &p, paperRectPixel, paperRectMM );

  delete resPlotItem;
  delete composerMap;
  delete composition;

  return true;
}

bool QgsGeorefPluginGui::writePDFReportFile( const QString &fileName, const QgsGeorefTransform &transform )
{
  if ( !mCanvas )
  {
    return false;
  }

  //create composition A4 with 300 dpi
  QgsComposition *composition = new QgsComposition( QgsProject::instance() );
  composition->setPaperSize( 210, 297 ); //A4
  composition->setPrintResolution( 300 );
  composition->setNumPages( 2 );

  QFont titleFont;
  titleFont.setPointSize( 9 );
  titleFont.setBold( true );
  QFont tableHeaderFont;
  tableHeaderFont.setPointSize( 9 );
  tableHeaderFont.setBold( true );
  QFont tableContentFont;
  tableContentFont.setPointSize( 9 );

  QgsSettings s;
  double leftMargin = s.value( QStringLiteral( "/Plugin-GeoReferencer/Config/LeftMarginPDF" ), "2.0" ).toDouble();
  double rightMargin = s.value( QStringLiteral( "/Plugin-GeoReferencer/Config/RightMarginPDF" ), "2.0" ).toDouble();
  double contentWidth = 210 - ( leftMargin + rightMargin );

  //title
  QFileInfo rasterFi( mRasterFileName );
  QgsComposerLabel *titleLabel = new QgsComposerLabel( composition );
  titleLabel->setFont( titleFont );
  titleLabel->setText( rasterFi.fileName() );
  composition->addItem( titleLabel );
  titleLabel->setSceneRect( QRectF( leftMargin, 5, contentWidth, 8 ) );
  titleLabel->setFrameEnabled( false );

  //composer map
  QgsRasterLayer *rLayer = ( QgsRasterLayer * ) mCanvas->layer( 0 );
  if ( !rLayer )
  {
    return false;
  }
  QgsRectangle layerExtent = rLayer->extent();
  //calculate width and height considering extent aspect ratio and max Width 206, maxHeight 70
  double widthExtentRatio = contentWidth / layerExtent.width();
  double heightExtentRatio = 70 / layerExtent.height();
  double mapWidthMM = 0;
  double mapHeightMM = 0;
  if ( widthExtentRatio < heightExtentRatio )
  {
    mapWidthMM = contentWidth;
    mapHeightMM = contentWidth / layerExtent.width() * layerExtent.height();
  }
  else
  {
    mapHeightMM = 70;
    mapWidthMM = 70 / layerExtent.height() * layerExtent.width();
  }

  QgsComposerMap *composerMap = new QgsComposerMap( composition, leftMargin, titleLabel->rect().bottom() + titleLabel->pos().y(), mapWidthMM, mapHeightMM );
  composerMap->setLayers( mCanvas->mapSettings().layers() );
  composerMap->zoomToExtent( layerExtent );
  composition->addItem( composerMap );

  QgsComposerTextTableV2 *parameterTable = nullptr;
  double scaleX, scaleY, rotation;
  QgsPointXY origin;

  QgsComposerLabel *parameterLabel = nullptr;
  //transformation that involves only scaling and rotation (linear or helmert) ?
  bool wldTransform = transform.getOriginScaleRotation( origin, scaleX, scaleY, rotation );

  QString residualUnits;
  if ( s.value( QStringLiteral( "/Plugin-GeoReferencer/Config/ResidualUnits" ) ) == "mapUnits" && mGeorefTransform.providesAccurateInverseTransformation() )
  {
    residualUnits = tr( "map units" );
  }
  else
  {
    residualUnits = tr( "pixels" );
  }

  QGraphicsRectItem *previousItem = composerMap;
  if ( wldTransform )
  {
    QString parameterTitle = tr( "Transformation parameters" ) + QStringLiteral( " (" ) + convertTransformEnumToString( transform.transformParametrisation() ) + QStringLiteral( ")" );
    parameterLabel = new QgsComposerLabel( composition );
    parameterLabel->setFont( titleFont );
    parameterLabel->setText( parameterTitle );
    parameterLabel->adjustSizeToText();
    composition->addItem( parameterLabel );
    parameterLabel->setSceneRect( QRectF( leftMargin, composerMap->rect().bottom() + composerMap->pos().y() + 5, contentWidth, 8 ) );
    parameterLabel->setFrameEnabled( false );

    //calculate mean error
    double meanError = 0;
    calculateMeanError( meanError );

    parameterTable = new QgsComposerTextTableV2( composition, false );
    parameterTable->setHeaderFont( tableHeaderFont );
    parameterTable->setContentFont( tableContentFont );

    QgsComposerTableColumns columns;
    columns << new QgsComposerTableColumn( tr( "Translation x" ) )
            << new QgsComposerTableColumn( tr( "Translation y" ) )
            << new QgsComposerTableColumn( tr( "Scale x" ) )
            << new QgsComposerTableColumn( tr( "Scale y" ) )
            << new QgsComposerTableColumn( tr( "Rotation [degrees]" ) )
            << new QgsComposerTableColumn( tr( "Mean error [%1]" ).arg( residualUnits ) );

    parameterTable->setColumns( columns );
    QStringList row;
    row << QString::number( origin.x(), 'f', 3 ) << QString::number( origin.y(), 'f', 3 ) << QString::number( scaleX ) << QString::number( scaleY ) << QString::number( rotation * 180 / M_PI ) << QString::number( meanError );
    parameterTable->addRow( row );

    QgsComposerFrame *tableFrame = new QgsComposerFrame( composition, parameterTable, leftMargin, parameterLabel->rect().bottom() + parameterLabel->pos().y() + 5, contentWidth, 12 );
    parameterTable->addFrame( tableFrame );

    composition->addItem( tableFrame );
    parameterTable->setGridStrokeWidth( 0.1 );

    previousItem = tableFrame;
  }

  QgsComposerLabel *residualLabel = new QgsComposerLabel( composition );
  residualLabel->setFont( titleFont );
  residualLabel->setText( tr( "Residuals" ) );
  composition->addItem( residualLabel );
  residualLabel->setSceneRect( QRectF( leftMargin, previousItem->rect().bottom() + previousItem->pos().y() + 5, contentWidth, 6 ) );
  residualLabel->setFrameEnabled( false );

  //residual plot
  QgsResidualPlotItem *resPlotItem = new QgsResidualPlotItem( composition );
  composition->addItem( resPlotItem );
  resPlotItem->setSceneRect( QRectF( leftMargin, residualLabel->rect().bottom() + residualLabel->pos().y() + 5, contentWidth, composerMap->rect().height() ) );
  resPlotItem->setExtent( composerMap->extent() );
  resPlotItem->setGcpList( getGcpList() );

  //necessary for the correct scale bar unit label
  resPlotItem->setConvertScaleToMapUnits( residualUnits == tr( "map units" ) );

  QgsComposerTextTableV2 *gcpTable = new QgsComposerTextTableV2( composition, false );
  gcpTable->setHeaderFont( tableHeaderFont );
  gcpTable->setContentFont( tableContentFont );
  gcpTable->setHeaderMode( QgsComposerTableV2::AllFrames );
  QgsComposerTableColumns columns;
  columns << new QgsComposerTableColumn( tr( "ID" ) )
          << new QgsComposerTableColumn( tr( "Enabled" ) )
          << new QgsComposerTableColumn( tr( "Pixel X" ) )
          << new QgsComposerTableColumn( tr( "Pixel Y" ) )
          << new QgsComposerTableColumn( tr( "Map X" ) )
          << new QgsComposerTableColumn( tr( "Map Y" ) )
          << new QgsComposerTableColumn( tr( "Res X (%1)" ).arg( residualUnits ) )
          << new QgsComposerTableColumn( tr( "Res Y (%1)" ).arg( residualUnits ) )
          << new QgsComposerTableColumn( tr( "Res Total (%1)" ).arg( residualUnits ) );

  gcpTable->setColumns( columns );

  QgsGcpList::const_iterator gcpIt = getGcpList()->constBegin();
  QList< QStringList > gcpTableContents;
  for ( ; gcpIt != getGcpList()->constEnd(); ++gcpIt )
  {
    QStringList currentGCPStrings;
    QPointF residual = ( *gcpIt )->residual();
    double residualTot = sqrt( residual.x() * residual.x() +  residual.y() * residual.y() );

    currentGCPStrings << QString::number( ( *gcpIt )->id() );
    if ( ( *gcpIt )->isEnabled() )
    {
      currentGCPStrings << tr( "yes" );
    }
    else
    {
      currentGCPStrings << tr( "no" );
    }
    currentGCPStrings << QString::number( ( *gcpIt )->pixelCoords().x(), 'f', 0 ) << QString::number( ( *gcpIt )->pixelCoords().y(), 'f', 0 ) << QString::number( ( *gcpIt )->mapCoords().x(), 'f', 3 )
                      <<  QString::number( ( *gcpIt )->mapCoords().y(), 'f', 3 ) <<  QString::number( residual.x() ) <<  QString::number( residual.y() ) << QString::number( residualTot );
    gcpTableContents << currentGCPStrings ;
  }

  gcpTable->setContents( gcpTableContents );

  double firstFrameY = resPlotItem->rect().bottom() + resPlotItem->pos().y() + 5;
  double firstFrameHeight = 287 - firstFrameY;
  QgsComposerFrame *gcpFirstFrame = new QgsComposerFrame( composition, gcpTable, leftMargin, firstFrameY, contentWidth, firstFrameHeight );
  gcpTable->addFrame( gcpFirstFrame );
  composition->addItem( gcpFirstFrame );

  QgsComposerFrame *gcpSecondFrame = new QgsComposerFrame( composition, gcpTable, leftMargin, 10, contentWidth, 277.0 );
  gcpSecondFrame->setItemPosition( leftMargin, 10, QgsComposerItem::UpperLeft, 2 );
  gcpSecondFrame->setHidePageIfEmpty( true );
  gcpTable->addFrame( gcpSecondFrame );
  composition->addItem( gcpSecondFrame );

  gcpTable->setGridStrokeWidth( 0.1 );
  gcpTable->setResizeMode( QgsComposerMultiFrame::RepeatUntilFinished );

  composition->exportAsPDF( fileName );

  delete titleLabel;
  delete parameterLabel;
  delete residualLabel;
  delete resPlotItem;
  delete composerMap;
  delete composition;
  return true;
}

void QgsGeorefPluginGui::updateTransformParamLabel()
{
  if ( !mTransformParamLabel )
  {
    return;
  }

  QString transformName = convertTransformEnumToString( mGeorefTransform.transformParametrisation() );
  QString labelString = tr( "Transform: " ) + transformName;

  QgsPointXY origin;
  double scaleX, scaleY, rotation;
  if ( mGeorefTransform.getOriginScaleRotation( origin, scaleX, scaleY, rotation ) )
  {
    labelString += ' ';
    labelString += tr( "Translation (%1, %2)" ).arg( origin.x() ).arg( origin.y() );
    labelString += ' ';
    labelString += tr( "Scale (%1, %2)" ).arg( scaleX ).arg( scaleY );
    labelString += ' ';
    labelString += tr( "Rotation: %1" ).arg( rotation * 180 / M_PI );
  }

  double meanError = 0;
  if ( calculateMeanError( meanError ) )
  {
    labelString += ' ';
    labelString += tr( "Mean error: %1" ).arg( meanError );
  }
  mTransformParamLabel->setText( labelString );
}

// Gdal script
void QgsGeorefPluginGui::showGDALScript( const QStringList &commands )
{
  QString script = commands.join( QStringLiteral( "\n" ) ) + '\n';

  // create window to show gdal script
  QDialogButtonBox *bbxGdalScript = new QDialogButtonBox( QDialogButtonBox::Cancel, Qt::Horizontal, this );
  QPushButton *pbnCopyInClipBoard = new QPushButton( getThemeIcon( QStringLiteral( "/mActionEditPaste.svg" ) ),
      tr( "Copy to Clipboard" ), bbxGdalScript );
  bbxGdalScript->addButton( pbnCopyInClipBoard, QDialogButtonBox::AcceptRole );

  QPlainTextEdit *pteScript = new QPlainTextEdit();
  pteScript->setReadOnly( true );
  pteScript->setWordWrapMode( QTextOption::NoWrap );
  pteScript->setPlainText( tr( "%1" ).arg( script ) );

  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget( pteScript );
  layout->addWidget( bbxGdalScript );

  QDialog *dlgShowGdalScript = new QDialog( this );
  dlgShowGdalScript->setWindowTitle( tr( "GDAL script" ) );
  dlgShowGdalScript->setLayout( layout );

  connect( bbxGdalScript, &QDialogButtonBox::accepted, dlgShowGdalScript, &QDialog::accept );
  connect( bbxGdalScript, &QDialogButtonBox::rejected, dlgShowGdalScript, &QDialog::reject );

  if ( dlgShowGdalScript->exec() == QDialog::Accepted )
  {
    QApplication::clipboard()->setText( pteScript->toPlainText() );
  }
}

QString QgsGeorefPluginGui::generateGDALtranslateCommand( bool generateTFW )
{
  QStringList gdalCommand;
  gdalCommand << QStringLiteral( "gdal_translate" ) << QStringLiteral( "-of GTiff" ) << generateGDALTifftags( 0 );
  if ( generateTFW )
  {
    // say gdal generate associated ESRI world file
    gdalCommand << QStringLiteral( "-co TFW=YES" );
  }

  // mj10777: replaces Q_FOREACH loop
  gdalCommand << generateGDALGcpList();

  QFileInfo raster_file( mRasterFileName );
  mTranslatedRasterFileName = QDir::tempPath() + '/' + raster_file.fileName();
  gdalCommand << QString( "\"%1\"" ).arg( mRasterFileName ) << QString( "\"%1\"" ).arg( mTranslatedRasterFileName );

  return gdalCommand.join( " " );
}

QString QgsGeorefPluginGui::generateGDALwarpCommand( const QString &resampling, const QString &compress,
    bool useZeroForTrans, int order, double targetResX, double targetResY )
{
  QStringList gdalCommand;
  gdalCommand << QStringLiteral( "gdalwarp -r" ) << resampling << generateGDALTifftags( 1 ) << generateGDALNodataCutlineParms();

  if ( order > 0 && order <= 3 )
  {
    // Let gdalwarp use polynomial warp with the given degree
    gdalCommand << QStringLiteral( "-order" ) << QString::number( order );
  }
  else
  {
    // Otherwise, use thin plate spline interpolation
    gdalCommand << QStringLiteral( "-tps" );
  }
  gdalCommand << QStringLiteral( "-co COMPRESS=" ) + compress << ( useZeroForTrans ? QStringLiteral( "-dstalpha" ) : "" );

  if ( targetResX != 0.0 && targetResY != 0.0 )
  {
    gdalCommand << QStringLiteral( "-tr" ) << QString::number( targetResX, 'f' ) << QString::number( targetResY, 'f' );
  }

  gdalCommand << QStringLiteral( "\"%1\"" ).arg( mTranslatedRasterFileName ) << QStringLiteral( "\"%1\"" ).arg( mModifiedRasterFileName );

  return gdalCommand.join( " " );
}

// Log
//void QgsGeorefPluginGui::showMessageInLog(const QString &description, const QString &msg)
//{
//  QString logItem = QString("<code>%1: %2</code>").arg(description).arg(msg);
//
//  mLogViewer->appendHtml(logItem);
//}
//
//void QgsGeorefPluginGui::clearLog()
//{
//  mLogViewer->clear();
//}

// Helpers
bool QgsGeorefPluginGui::checkReadyGeoref()
{
  if ( mRasterFileName.isEmpty() )
  {
    mMessageBar->pushMessage( tr( "No Raster Loaded" ), tr( "Please load raster to be georeferenced" ), QgsMessageBar::WARNING, messageTimeout() );
    return false;
  }

  if ( QgsGeorefTransform::InvalidTransform == mTransformParam )
  {
    QMessageBox::information( this, tr( "Info" ), tr( "Please set transformation type" ) );
    getTransformSettings();
    return false;
  }

  //MH: helmert transformation without warping disabled until qgis is able to read rotated rasters efficiently
  if ( mModifiedRasterFileName.isEmpty() && QgsGeorefTransform::Linear != mTransformParam /*&& QgsGeorefTransform::Helmert != mTransformParam*/ )
  {
    QMessageBox::information( this, tr( "Info" ), tr( "Please set output raster name" ) );
    getTransformSettings();
    return false;
  }

  if ( getGcpList()->count() < ( int )mGeorefTransform.getMinimumGCPCount() )
  {
    mMessageBar->pushMessage( tr( "Not Enough GCPs" ), tr( "%1 transformation requires at least %2 GCPs. Please define more." )
                              .arg( convertTransformEnumToString( mTransformParam ) ).arg( mGeorefTransform.getMinimumGCPCount() )
                              , QgsMessageBar::WARNING, messageTimeout() );
    return false;
  }

  // Update the transform if necessary
  if ( !updateGeorefTransform() )
  {
    mMessageBar->pushMessage( tr( "Transform Failed" ), tr( "Failed to compute GCP transform: Transform is not solvable." ), QgsMessageBar::WARNING, messageTimeout() );
    //    logRequaredGCPs();
    return false;
  }

  return true;
}

bool QgsGeorefPluginGui::updateGeorefTransform()
{
  if ( !mAvoidUnneededUpdates )
  {
    mGcpListWidget->updateGcpList(); // will only do something if isDirty()
    if ( !mGcpListWidget->transformUpdated() )
    {
      return false;
    }
    updateTransformParamLabel();
  }
  return true;
}

// Samples the given rectangle at numSamples per edge.
// Returns an axis aligned bounding box which contains the transformed samples.
QgsRectangle QgsGeorefPluginGui::transformViewportBoundingBox( const QgsRectangle &canvasExtent,
    QgsGeorefTransform &t,
    bool rasterToWorld, uint numSamples )
{
  double minX, minY;
  double maxX, maxY;
  minX = minY =  std::numeric_limits<double>::max();
  maxX = maxY = -std::numeric_limits<double>::max();

  double oX = canvasExtent.xMinimum();
  double oY = canvasExtent.yMinimum();
  double dX = canvasExtent.xMaximum();
  double dY = canvasExtent.yMaximum();
  double stepX = numSamples ? ( dX - oX ) / ( numSamples - 1 ) : 0.0;
  double stepY = numSamples ? ( dY - oY ) / ( numSamples - 1 ) : 0.0;
  for ( uint s = 0u;  s < numSamples; s++ )
  {
    for ( uint edge = 0; edge < 4; edge++ )
    {
      QgsPointXY src, raster;
      switch ( edge )
      {
        case 0:
          src = QgsPointXY( oX + ( double )s * stepX, oY );
          break;
        case 1:
          src = QgsPointXY( oX + ( double )s * stepX, dY );
          break;
        case 2:
          src = QgsPointXY( oX, oY + ( double )s * stepY );
          break;
        case 3:
          src = QgsPointXY( dX, oY + ( double )s * stepY );
          break;
      }
      t.transform( src, raster, rasterToWorld );
      minX = qMin( raster.x(), minX );
      maxX = qMax( raster.x(), maxX );
      minY = qMin( raster.y(), minY );
      maxY = qMax( raster.y(), maxY );
    }
  }
  return QgsRectangle( minX, minY, maxX, maxY );
}

QString QgsGeorefPluginGui::convertTransformEnumToString( QgsGeorefTransform::TransformParametrisation transform )
{
  switch ( transform )
  {
    case QgsGeorefTransform::Linear:
      return tr( "Linear" );
    case QgsGeorefTransform::Helmert:
      return tr( "Helmert" );
    case QgsGeorefTransform::PolynomialOrder1:
      return tr( "Polynomial 1" );
    case QgsGeorefTransform::PolynomialOrder2:
      return tr( "Polynomial 2" );
    case QgsGeorefTransform::PolynomialOrder3:
      return tr( "Polynomial 3" );
    case QgsGeorefTransform::ThinPlateSpline:
      return tr( "Thin plate spline (TPS)" );
    case QgsGeorefTransform::Projective:
      return tr( "Projective" );
    default:
      return tr( "Not set" );
  }
}

QString QgsGeorefPluginGui::convertResamplingEnumToString( QgsImageWarper::ResamplingMethod resampling )
{
  switch ( resampling )
  {
    case QgsImageWarper::NearestNeighbour:
      return QStringLiteral( "near" );
    case QgsImageWarper::Bilinear:
      return QStringLiteral( "bilinear" );
    case QgsImageWarper::Cubic:
      return QStringLiteral( "cubic" );
    case QgsImageWarper::CubicSpline:
      return QStringLiteral( "cubicspline" );
    case QgsImageWarper::Lanczos:
      return QStringLiteral( "lanczos" );
  }
  return QLatin1String( "" );
}

int QgsGeorefPluginGui::polynomialOrder( QgsGeorefTransform::TransformParametrisation transform )
{
  switch ( transform )
  {
    case QgsGeorefTransform::PolynomialOrder1:
      return 1;
    case QgsGeorefTransform::PolynomialOrder2:
      return 2;
    case QgsGeorefTransform::PolynomialOrder3:
      return 3;
    case QgsGeorefTransform::ThinPlateSpline:
      return -1;

    default:
      return 0;
  }
}

QString QgsGeorefPluginGui::guessWorldFileName( const QString &rasterFileName )
{
  QString worldFileName = QLatin1String( "" );
  int point = rasterFileName.lastIndexOf( '.' );
  if ( point != -1 && point != rasterFileName.length() - 1 )
    worldFileName = rasterFileName.left( point + 1 ) + "wld";

  return worldFileName;
}

// Note this code is duplicated from qgisapp.cpp because
// I didn't want to make plugins on qgsapplication [TS]
QIcon QgsGeorefPluginGui::getThemeIcon( const QString &name )
{
  if ( QFile::exists( QgsApplication::activeThemePath() + name ) )
  {
    return QIcon( QgsApplication::activeThemePath() + name );
  }
  else if ( QFile::exists( QgsApplication::defaultThemePath() + name ) )
  {
    return QIcon( QgsApplication::defaultThemePath() + name );
  }
  else
  {
    QgsSettings settings;
    QString themePath = ":/icons/" + settings.value( QStringLiteral( "Themes" ) ).toString() + name;
    if ( QFile::exists( themePath ) )
    {
      return QIcon( themePath );
    }
    else
    {
      return QIcon( ":/icons/default" + name );
    }
  }
}

bool QgsGeorefPluginGui::checkFileExisting( const QString &fileName, const QString &title, const QString &question )
{
  if ( !fileName.isEmpty() )
  {
    if ( QFile::exists( fileName ) )
    {
      int r = QMessageBox::question( this, title, question,
                                     QMessageBox::Yes | QMessageBox::Default,
                                     QMessageBox::No | QMessageBox::Escape );
      if ( r == QMessageBox::No )
        return false;
      else
        QFile::remove( fileName );
    }
  }

  return true;
}

bool QgsGeorefPluginGui::equalGCPlists( const QgsGcpList *list1, const QgsGcpList *list2 )
{
  // posibly no longer needed
  if ( list1->count() != list2->count() )
    return false;

  int count = list1->count();
  int j = 0;
  for ( int i = 0; i < count; ++i, ++j )
  {
    QgsGeorefDataPoint *p1 = list1->at( i );
    QgsGeorefDataPoint *p2 = list2->at( j );
    if ( p1->pixelCoords() != p2->pixelCoords() )
      return false;

    if ( p1->mapCoords() != p2->mapCoords() )
      return false;
  }

  return true;
}

//void QgsGeorefPluginGui::logTransformOptions()
//{
//  showMessageInLog(tr("Interpolation"), convertResamplingEnumToString(mResamplingMethod));
//  showMessageInLog(tr("Compression method"), mCompressionMethod);
//  showMessageInLog(tr("Zero for transparency"), mUseZeroForTrans ? "true" : "false");
//}
//
//void QgsGeorefPluginGui::logRequaredGCPs()
//{
//  if (mGeorefTransform.getMinimumGCPCount() != 0)
//  {
//    if ((uint)mPoints.size() >= mGeorefTransform.getMinimumGCPCount())
//      showMessageInLog(tr("Info"), tr("For georeferencing requared at least %1 GCP points")
//                       .arg(mGeorefTransform.getMinimumGCPCount()));
//    else
//      showMessageInLog(tr("Critical"), tr("For georeferencing requared at least %1 GCP points")
//                       .arg(mGeorefTransform.getMinimumGCPCount()));
//  }
//}

void QgsGeorefPluginGui::clearGcpData()
{
  //force all list widget editors to close before removing data points
  //otherwise the editors try to update deleted data points when they close
  mGcpListWidget->closeEditors();
  if ( mLayerGcpPoints )
  {
    if ( mTreeGroupGeoreferencer )
    {
      // In QGIS--Canvas
#if 0
      exportLayerDefinition( mTreeGroupGeoreferencer );
#endif
      if ( mTreeGroupGcpCutlines )
      {
        // Order: disconnect ; save ; group_remove ; MapLayerRemove
        // mTreeGroupGcpCutlines->removeLayer( mLayerMercatorPolygons );
        // Warning: Object::disconnect: No such signal QObject::featureAdded( QgsFeatureId )
        // Warning: Object::disconnect:  (receiver name: 'QgsGeorefPluginGuiBase')
        // ?? No idea why ',( QgsGeorefPluginGui* )this' is needed. Otherwise crash ??
        saveEditsGcp( mLayerCutlinePoints, false );
        mTreeGroupGcpCutlines->removeLayer( mLayerCutlinePoints );
        QgsProject::instance()->removeMapLayers( ( QStringList() << mLayerCutlinePoints->id() ) );
        mLayerCutlinePoints = nullptr;
        //-------
        saveEditsGcp( mLayerCutlineLinestrings, false );
        mTreeGroupGcpCutlines->removeLayer( mLayerCutlineLinestrings );
        QgsProject::instance()->removeMapLayers( ( QStringList() << mLayerCutlineLinestrings->id() ) );
        mLayerCutlineLinestrings = nullptr;
        //-------
        saveEditsGcp( mLayerCutlinePolygons, false );
        mTreeGroupGcpCutlines->removeLayer( mLayerCutlinePolygons );
        QgsProject::instance()->removeMapLayers( ( QStringList() << mLayerCutlinePolygons->id() ) );
        mLayerCutlinePolygons = nullptr;
        //-------
        saveEditsGcp( mLayerMercatorCutlinePolygons, false );
        mTreeGroupGcpCutlines->removeLayer( mLayerMercatorCutlinePolygons );
        QgsProject::instance()->removeMapLayers( ( QStringList() << mLayerMercatorCutlinePolygons->id() ) );
        mLayerMercatorCutlinePolygons = nullptr;
        //-------
        mTreeGroupGeoreferencer->removeChildNode( ( QgsLayerTreeNode * )mTreeGroupGcpCutlines );
        mTreeGroupGcpCutlines = nullptr;
      }
      if ( mTreeGroupGcpPoints )
      {
        // Order: disconnect ; save ; group_remove ; MapLayerRemove
        saveEditsGcp( mLayerGcpPoints, false );
        if ( mTreeGroupGcpPoints->findLayer( mLayerGcpPoints->id() ) )
        {
          // Remove only if layer is still contained in the main-group, may have been moved elsewhere by the user
          mTreeGroupGcpPoints->removeLayer( mLayerGcpPoints );
          QgsProject::instance()->removeMapLayers( ( QStringList() << mLayerGcpPoints->id() ) );
        }
        mLayerGcpPoints = nullptr;
        //-------
        mTreeGroupGcpPoints = nullptr;
        mTreeGroupGeoreferencer->removeChildNode( ( QgsLayerTreeNode * )mTreeGroupGcpPoints );
      }
      if ( mTreeGroupGtifRasters )
      {
        // Order:  group_remove ; MapLayerRemove
        if ( ( mLayerGtifRaster ) && ( mTreeLayerGtifRaster ) )
        {
          // Check if layer is still contained in group
          if ( mTreeGroupGtifRasters->findLayer( mTreeLayerGtifRaster->layerId() ) )
          {
            // Remove only if layer is still contained in the main-group, may have been moved elsewhere by the user
            mTreeGroupGtifRasters->removeLayer( mLayerGtifRaster );
            QgsProject::instance()->removeMapLayers( ( QStringList() << mLayerGtifRaster->id() ) );
            mLayerGtifRaster = nullptr;
          }
        }
        mTreeGroupGeoreferencer->removeChildNode( ( QgsLayerTreeNode * )mTreeGroupGtifRasters );
        mTreeGroupGtifRasters = nullptr;
      }
      if ( mTreeGroupGcpMaster )
      {
        // Order: disconnect ; save ; group_remove ; MapLayerRemove
        saveEditsGcp( mLayerGcpMaster, false );
        mTreeGroupGcpMaster->removeLayer( mLayerGcpMaster );
        mTreeGroupGcpMaster = nullptr;
        QgsProject::instance()->removeMapLayers( ( QStringList() << mLayerGcpMaster->id() ) );
        mLayerGcpMaster = nullptr;
      }
      QgsProject::instance()->layerTreeRoot()->removeChildNode( ( QgsLayerTreeNode * )mTreeGroupGeoreferencer );
      mTreeGroupGeoreferencer = nullptr;
    }

    if ( mTreeGroupCutlineMercator )
    {
      // Georeferencer-Canvas
      if ( mLayerMercatorPolygons )
      {
        // Order: disconnect ; save ; group_remove ; MapLayerRemove
        saveEditsGcp( mLayerMercatorPolygons, false );
        mTreeGroupCutlineMercator->removeLayer( mLayerMercatorPolygons );
        QgsProject::instance()->removeMapLayers( ( QStringList() << mLayerMercatorPolygons->id() ) );
        mLayerMercatorPolygons = nullptr;
      }
      if ( mLayerGcpPixels )
      {
        // Order: disconnect ; save ; group_remove ; MapLayerRemove
        saveEditsGcp( mLayerGcpPixels, false );
        QgsProject::instance()->removeMapLayers( ( QStringList() << mLayerGcpPixels->id() ) );
        mTreeGroupCutlineMercator->removeLayer( mLayerGcpPixels );
        mLayerGcpPixels = nullptr;
      }
      mRootLayerTree->removeChildNode( ( QgsLayerTreeNode * )mTreeGroupCutlineMercator );
      mTreeGroupCutlineMercator = nullptr;
    }
  }
  if ( mGcpListWidget->countDataPoints() > 0 )
  {
    mGcpListWidget->clearDataPoints();
    mGcpListWidget->updateGcpList();
  }
  if ( mLayerRaster )
  {
    QgsProject::instance()->removeMapLayers( ( QStringList() << mLayerRaster->id() ) );
    mLayerRaster = nullptr;
  }
  mCanvas->setLayers( QList<QgsMapLayer *>() );
  mCanvas->refresh();
  mIface->mapCanvas()->refresh();
  mSpatialiteGcpOff = false;
  mActionSpatialiteGcp->setEnabled( !mSpatialiteGcpOff );
  mActionFetchMasterGcpExtent->setEnabled( !mSpatialiteGcpOff );
  // qDebug() << QString( "-I-> QgsGeorefPluginGui::clearGcpData[%1]" ).arg( "finished" );
  //-------------------------------------------------------------------------------------------------------------------
}

int QgsGeorefPluginGui::messageTimeout()
{
  QgsSettings settings;
  return settings.value( QStringLiteral( "/qgis/messageTimeout" ), 5 ).toInt();
}

// mj10777: added functions
QString QgsGeorefPluginGui::generateGDALGcpList()
{
  QStringList gdalCommand;

  for ( int i = 0; i < getGcpList()->countDataPoints(); ++i )
  {
    QgsGeorefDataPoint *dataPoint = getGcpList()->at( i );
    if ( dataPoint->isEnabled() )
    {
      gdalCommand << QString( "-gcp %1 %2 %3 %4" ).arg( dataPoint->pixelCoords().x() ).arg( -dataPoint->pixelCoords().y() ).arg( dataPoint->mapCoords().x() ).arg( dataPoint->mapCoords().y() );
    }
  }
  return gdalCommand.join( " " );
}
QString QgsGeorefPluginGui::generateGDALMercatorScript()
{
  QStringList gdalCommand;
  QString s_parm = "-mo"; // gdal_edit.py
  int i_nodata = 192;
  if ( mGcpDbData->mRasterNodata >= 0 )
    i_nodata = mGcpDbData->mRasterNodata;
  QString s_gdalwarp_base = QString( "gdalwarp -s_srs epsg:%1 -t_srs epsg:%1 -srcnodata %2 -dstnodata %2 -crop_to_cutline -cutline %3" ).arg( 3395 ).arg( i_nodata ).arg( mGcpDbData->mGcpDatabaseFileName );
  // Avoid gdal warnings about missing srid
  QString s_sql_base = QString( "-csql \"SELECT SetSRID(mercator_polygon,3395) FROM create_mercator_polygons WHERE (id_cutline = %1);\"" );
  // Path to the file, without file-name : file-name without path
  QFileInfo raster_file( QString( "%1/%2" ).arg( mGcpDbData->mRasterFilePath ).arg( mGcpDbData->mRasterFileName ) );
  if ( raster_file.exists() )
  {
    if ( mLayerMercatorPolygons )
    {
      QgsAttributeList lst_needed_fields;
      lst_needed_fields.append( 0 ); // id_cutline
      lst_needed_fields.append( 2 ); // cutline_type
      lst_needed_fields.append( 3 ); // name
      QgsFeature fet_polygon;
      QgsFeatureRequest retrieve_request = QgsFeatureRequest().setFilterExpression( QString( "id_gcp_coverage=%1" ).arg( mGcpDbData->mIdGcpCoverage ) );
      QgsFeatureIterator fit_polygons = mLayerMercatorPolygons->getFeatures( retrieve_request.setSubsetOfAttributes( lst_needed_fields ) );
      while ( fit_polygons.nextFeature( fet_polygon ) )
      {
        if ( fet_polygon .geometry() && fet_polygon.geometry().type() == QgsWkbTypes::PolygonGeometry )
        {
          int i_cutline_type = fet_polygon.attribute( QString( "cutline_type" ) ).toInt();
          if ( i_cutline_type == 77 )
          {
            // Ignore any other Polygons [77=default value]
            if ( gdalCommand.size() == 0 )
            {
              // Create a World-file (one) for the input file [emulating epsg:3395 'WGS 84 / World Mercator']
              QFileInfo world_file( QString( "%1/%2" ).arg( mGcpDbData->mRasterFilePath ).arg( QString( "%1.%2" ).arg( raster_file.completeBaseName() ).arg( "tfw" ) ) );
              gdalCommand << QString( "echo -e \"%1\n%2\n%2\n-%1\n%2\n%2\" > %3;" ).arg( "1.00000000000000" ).arg( "0.00000000000000" ).arg( world_file.absoluteFilePath() );
            }
            int id_cutline = fet_polygon.attribute( QString( "id_cutline" ) ).toInt();
            QString s_name = fet_polygon.attribute( QString( "name" ) ).toString();
            //  -to 'TIFFTAG_DOCUMENTNAME=1750 Stadtplan von Berlin, Schmettau - NW - 1:4333'
            QString s_tifftag = QString( "%1 'TIFFTAG_DOCUMENTNAME=%2'" ).arg( s_parm ).arg( s_name );
            QFileInfo output_file( QString( "%1/%2" ).arg( mGcpDbData->mRasterFilePath ).arg( QString( "%1.%2.%3" ).arg( raster_file.completeBaseName() ).arg( id_cutline ).arg( raster_file.suffix() ) ) );
            gdalCommand << QString( "%1 %2 %3 %4;" ).arg( s_gdalwarp_base ).arg( QString( s_sql_base ).arg( id_cutline ) ).arg( raster_file.absoluteFilePath() ).arg( output_file.absoluteFilePath() );
            // Set the TIFFTAG and remove georeference information
            gdalCommand << QString( "gdal_edit.py %1 -unsetgt %2;" ).arg( s_tifftag ).arg( output_file.absoluteFilePath() );
          }
        }
      }
    }
  }
  return gdalCommand.join( " " );
}
QString QgsGeorefPluginGui::generateGDALTifftags( int i_gdal_command )
{
  QStringList gdal_tifftags;
  QString s_parm = "-mo"; // gdal_translate
  if ( i_gdal_command == 1 )
  {
    s_parm = "-to"; // gdalwarp
  }
  if ( mGcpDbData->mGcpCoverageTitle.isEmpty() )
  {
    //  -to 'TIFFTAG_DOCUMENTNAME=1700 Grundriss der Friedrichstadt und Dorotheenstadt'
    gdal_tifftags << QString( "%1 'TIFFTAG_DOCUMENTNAME=%2'" ).arg( s_parm ).arg( mGcpDbData->mGcpCoverageTitle );
  }
  if ( !mGcpDbData->mGcpCoverageAbstract.isEmpty() )
  {
    // -to 'TIFFTAG_IMAGEDESCRIPTION=1700 Grundriss der Friedrichstadt und Dorotheenstadt, Federzeichnung um 1700.'
    gdal_tifftags << QString( "%1 'TIFFTAG_IMAGEDESCRIPTION=%2'" ).arg( s_parm ).arg( mGcpDbData->mGcpCoverageAbstract );
  }
  if ( !mGcpDbData->mGcpCoverageCopyright.isEmpty() )
  {
    // -to 'TIFFTAG_COPYRIGHT=https://de.wikipedia.org/wiki/Datei:Friedrichstadt_dorotheensta.jpg'
    gdal_tifftags << QString( "%1 'TIFFTAG_COPYRIGHT=%2'" ).arg( s_parm ).arg( mGcpDbData->mGcpCoverageCopyright );
  }
  if ( !mGcpDbData->mGcpCoverageMapDate.isEmpty() )
  {
    // -to 'TIFFTAG_DATETIME=1700'
    gdal_tifftags << QString( "%1 'TIFFTAG_DATETIME=%2'" ).arg( s_parm ).arg( mGcpDbData->mGcpCoverageMapDate );
  }
  if ( !mGcpDbData->map_providertags.value( "TIFFTAG_ARTIST" ).isEmpty() )
  {
    // -to 'TIFFTAG_ARTIST=mj10777.de.eu'
    gdal_tifftags << QString( "%1 'TIFFTAG_ARTIST=%2'" ).arg( s_parm ).arg( mGcpDbData->map_providertags.value( "TIFFTAG_ARTIST" ) );
  }
  return gdal_tifftags.join( " " );
}
QString QgsGeorefPluginGui::generateGDALNodataCutlineParms()
{
  QStringList gdal_nodata_cutline;
  if ( mGcpDbData->mRasterNodata >= 0 )
  {
    // -srcnodata 255 -dstnodata 255
    gdal_nodata_cutline << QString( "-srcnodata %1 -dstnodata %1 " ).arg( mGcpDbData->mRasterNodata );
  }
  if ( mGcpDbData->mIdGcpCutline >= 0 )
  {
    // -crop_to_cutline -cutline qgis/cutline.gcp.db -csql "SELECT cutline_polygon FROM create_cutline_polygons WHERE (id_cutline = 20);"
    QString s_cutline_sql = QString( "SELECT cutline_polygon FROM create_cutline_polygons WHERE (id_cutline = %1);" ).arg( mGcpDbData->mIdGcpCutline );
    gdal_nodata_cutline << QString( "-crop_to_cutline -cutline %1 -csql \"%2\"" ).arg( mGcpDbData->mGcpDatabaseFileName ).arg( s_cutline_sql );
  }
  return gdal_nodata_cutline.join( " " );
}
int  QgsGeorefPluginGui::getGcpTransformParam( QgsGeorefTransform::TransformParametrisation iTransformParam )
{
  // Use when calling QgsGeorefPluginGui::getGcpConvert
  int i_GpsTransformParam = 0;
  switch ( iTransformParam )
  {
    // The Spatialite GCP_Transform/Compute uses a different numbering than QgsGeorefTransform::TransformParametrisation
    case QgsGeorefTransform::PolynomialOrder1:
      i_GpsTransformParam = 1;
      break;
    case QgsGeorefTransform::PolynomialOrder2:
      i_GpsTransformParam = 2;
      break;
    case QgsGeorefTransform::PolynomialOrder3:
      i_GpsTransformParam = 3;
      break;
    case QgsGeorefTransform::ThinPlateSpline:
    default:
      i_GpsTransformParam = 0;
      break;
  }
  return i_GpsTransformParam;
}
QgsGeorefTransform::TransformParametrisation  QgsGeorefPluginGui::setGcpTransformParam( int iTransformParam )
{
  // Use when calling QgsGeorefPluginGui::getGcpConvert
  QgsGeorefTransform::TransformParametrisation i_GpsTransformParam = QgsGeorefTransform::ThinPlateSpline;
  switch ( iTransformParam )
  {
    // The Spatialite GCP_Transform/Compute uses a different numbering than QgsGeorefTransform::TransformParametrisation
    case 1:
      i_GpsTransformParam = QgsGeorefTransform::PolynomialOrder1;
      break;
    case 2:
      i_GpsTransformParam = QgsGeorefTransform::PolynomialOrder2;
      break;
    case 3:
      i_GpsTransformParam = QgsGeorefTransform::PolynomialOrder3;
      break;
    case 0:
    default:
      i_GpsTransformParam = QgsGeorefTransform::ThinPlateSpline;
      break;
  }
  return i_GpsTransformParam;
}

QString  QgsGeorefPluginGui::getGcpResamplingMethod( QgsImageWarper::ResamplingMethod i_ResamplingMethod )
{
  // Use when calling QgsGeorefPluginGui::getGcpConvert
  QString s_GpsResamplingMethod = "Lanczos";
  switch ( i_ResamplingMethod )
  {
    // In a readable form for the Database
    case QgsImageWarper::NearestNeighbour:
      s_GpsResamplingMethod = "NearestNeighbour";
      break;
    case QgsImageWarper::Bilinear:
      s_GpsResamplingMethod = "Bilinear";
      break;
    case QgsImageWarper::Cubic:
      s_GpsResamplingMethod = "Cubic";
      break;
    case QgsImageWarper::CubicSpline:
      s_GpsResamplingMethod = "CubicSpline";
      break;
    case QgsImageWarper::Lanczos:
    default:
      s_GpsResamplingMethod = "Lanczos";
      break;
  }
  return s_GpsResamplingMethod;
}
QgsImageWarper::ResamplingMethod QgsGeorefPluginGui::setGcpResamplingMethod( QString sResamplingMethod )
{
  QgsImageWarper::ResamplingMethod i_ResamplingMethod = QgsImageWarper::Lanczos;
  if ( sResamplingMethod == "NearestNeighbour" )
  {
    i_ResamplingMethod = QgsImageWarper::NearestNeighbour;
  }
  if ( sResamplingMethod == "Bilinear" )
  {
    i_ResamplingMethod = QgsImageWarper::Bilinear;
  }
  if ( sResamplingMethod == "Cubic" )
  {
    i_ResamplingMethod = QgsImageWarper::Cubic;
  }
  if ( sResamplingMethod == "CubicSpline" )
  {
    i_ResamplingMethod = QgsImageWarper::CubicSpline;
  }
  return i_ResamplingMethod;
}

bool QgsGeorefPluginGui::setGcpLayerSettings( QgsVectorLayer *layer_gcp )
{
  // Note: these are 'Point'-Layers
  Q_CHECK_PTR( layer_gcp );
  QColor color_red( "red" );
  QColor color_yellow( "yellow" );
  QColor color_goldenrod( "goldenrod" );
  QColor color_cyan( "cyan" );
  QColor color_forestgreen( "forestgreen" );
  QColor color_background = color_red;
  QColor color_foreground = color_yellow;
  QColor color_shadow = color_cyan;
  QColor color_area = color_forestgreen;
  bool bToPixel = true;
  if ( layer_gcp->name() == mGcpPointsLayername )
  {
    bToPixel = false;
  }
  switch ( mGcpLabelType )
  {
    case 1:
      mGcpLabelExpression = "id_gcp";
      break;
    case 2:
      if ( layer_gcp->name() == mGcpPointsLayername )
      {
        mGcpLabelExpression = "id_gcp||' ['||point_x||', '||point_y||']'";
        bToPixel = false;
      }
      else
        mGcpLabelExpression = "id_gcp||' ['||pixel_x||', '||pixel_y||']'";
      break;
    default:
      mGcpLabelType = 0;
      break;
  }
  double d_size_base = fontPointSize;
  if ( d_size_base < 10.0 )
    d_size_base = 15.0;
  double d_size_font = d_size_base * 0.8;
  d_size_base = d_size_base / 3; // 6.0
  double d_size_symbol_big = d_size_base * 1.5; // 7.5
  double d_size_symbol_small = d_size_base * 0.50;   // 2.50
  // Labels:
  if ( ( layer_gcp->isValid() ) && ( mGcpLabelType > 0 ) )
  {
    // see: core/qgspallabeling.cpp/h and  app/qgslabelingwidget.cpp
    // fontPointSize=15 ; _size_font=12
    color_background = color_yellow;
    color_shadow = color_cyan;
    QgsPalLayerSettings pal_layer_settings;
    // Access to labeling configuration, may be null if labeling is not used [default]
    if ( layer_gcp->labeling() )
    {
      QgsAbstractVectorLayerLabeling *vlLabeling = layer_gcp->labeling()->clone();
      if ( vlLabeling )
      {
        pal_layer_settings = vlLabeling->settings( layer_gcp->dataProvider()->name() );
      }
    }
    pal_layer_settings.drawLabels = true;
    pal_layer_settings.fieldName = mGcpLabelExpression;
    pal_layer_settings.isExpression = true;
    // Placement
    pal_layer_settings.placement = QgsPalLayerSettings::OrderedPositionsAroundPoint;
    // pal_layer_settings.placementFlags = QgsPalLayerSettings::AboveLine | QgsPalLayerSettings::MapOrientation;
    pal_layer_settings.placementFlags = QgsPalLayerSettings::AboveLine;
    pal_layer_settings.dist = 5;
    pal_layer_settings.distInMapUnits = false;
    // Wrap
    if ( mGcpLabelType == 2 )
    {
      // pal_layer_settings.wrapChar = QString( "[" ); // removes character
      //?? pal_layer_settings.multilineHeight = 1.0;
      pal_layer_settings.multilineAlign = QgsPalLayerSettings::MultiLeft;
    }
    // Retrieve Text-Format Settings
    QgsTextFormat pal_layer_textFormat = pal_layer_settings.format();
    pal_layer_textFormat.setFont( QApplication::font() );
    pal_layer_textFormat.setSize( d_size_font );
    pal_layer_textFormat.setSizeUnit( QgsUnitTypes::RenderPixels );
    pal_layer_textFormat.setColor( Qt::black );
#if 0
    // Retrieve Buffer [around Label-Text] Settings
    QgsTextBufferSettings pal_layer_buffer = pal_layer_textFormat.buffer();
    pal_layer_buffer.setEnabled( false );
    pal_layer_buffer.setColor( Qt::black );
    pal_layer_buffer.setSizeUnit( QgsUnitTypes::RenderPoints );
    pal_layer_buffer.setSize( d_size_font );
    // Replace Buffer [around Label-Text] Settings
    pal_layer_textFormat.setBuffer( pal_layer_buffer );
#endif
    // Retrieve Background [Shape]  Settings
    QgsTextBackgroundSettings pal_layer_background = pal_layer_textFormat.background();
    pal_layer_background.setEnabled( true );
    if ( mGcpLabelType == 1 )
    {
      pal_layer_background.setType( QgsTextBackgroundSettings::ShapeCircle );
    }
    else
    {
      pal_layer_background.setType( QgsTextBackgroundSettings::ShapeRectangle );
    }
    pal_layer_background.setSizeType( QgsTextBackgroundSettings::SizeBuffer );
    pal_layer_background.setSizeUnit( QgsUnitTypes::RenderMillimeters );
    // opacity as a double value between 0 (fully transparent) and 1 (totally opaque)
    pal_layer_background.setOpacity( 1.0 );
    pal_layer_background.setFillColor( color_background );
    pal_layer_background.setStrokeColor( Qt::black );
    pal_layer_background.setStrokeWidth( 1.0 );
    // Replace Background [Shape] to Settings
    pal_layer_textFormat.setBackground( pal_layer_background );
    // Retrieve  Shadow [Drop] Settings
    QgsTextShadowSettings pal_layer_shadow = pal_layer_textFormat.shadow();
    pal_layer_shadow.setEnabled( true );
    pal_layer_shadow.setShadowPlacement( QgsTextShadowSettings::ShadowLowest );
    pal_layer_shadow.setOffsetAngle( 135 );
    pal_layer_shadow.setOffsetDistance( 0 );
    pal_layer_shadow.setOffsetUnit( QgsUnitTypes::RenderMillimeters );
    pal_layer_shadow.setOffsetGlobal( true );
    pal_layer_shadow.setBlurRadius( 1.50 );
    pal_layer_shadow.setBlurRadiusUnit( QgsUnitTypes::RenderMillimeters );
    pal_layer_shadow.setBlurAlphaOnly( false );
    // opacity as a double value between 0 (fully transparent) and 1 (totally opaque)
    pal_layer_shadow.setOpacity( 70.0 );
    pal_layer_shadow.setScale( 100 );
    pal_layer_shadow.setColor( color_shadow );
    pal_layer_shadow.setBlendMode( QPainter::CompositionMode_Multiply );
    // Replace Shadow [Drop] to Settings
    pal_layer_textFormat.setShadow( pal_layer_shadow );
    // Replace Format to Settings
    pal_layer_settings.setFormat( pal_layer_textFormat );
    // Write settings to layer
    // pal_layer_settings.writeToLayer( layer_gcp );
    layer_gcp->setLabeling( new QgsVectorLayerSimpleLabeling( pal_layer_settings ) );
  }
  if ( ( layer_gcp->isValid() ) && ( layer_gcp->editFormConfig().suppress() !=  QgsEditFormConfig::SuppressOn ) )
  {
    // Turn off 'Feature Attributes' Form - QgsAttributeDialog
    layer_gcp->editFormConfig().setSuppress( QgsEditFormConfig::SuppressOn );
  }
  // Symbols:
  if ( ( layer_gcp->isValid() ) && ( layer_gcp->renderer() ) )
  {
    // see: gui/symbology-ng/qgsrendererv2propertiesdialog.cpp, qgslayerpropertieswidget.cpp and  app/qgsvectorlayerproperties.cpp
    QgsFeatureRenderer *feature_layer_renderer = layer_gcp->renderer();
    if ( feature_layer_renderer )
    {
      QgsRenderContext feature_layer_symbols_context;
      QgsSymbolList feature_layer_symbols_list = feature_layer_renderer->symbols( feature_layer_symbols_context );
      if ( feature_layer_symbols_list.count() == 1 )
      {
        // Being a Point, there should only be 1
        bool isDirty = false;
        QgsSymbol *feature_layer_symbol_container = feature_layer_symbols_list.at( 0 );
        if ( feature_layer_symbol_container )
        {
          if ( ( feature_layer_symbol_container->type() == QgsSymbol::Marker ) && ( feature_layer_symbol_container->symbolLayerCount() == 1 ) )
          {
            // remove the only QgsSymbol from the list [will be replaced with a big red star containing a small red circle]
            QgsSimpleMarkerSymbolLayer *feature_layer_symbol_first =  static_cast<QgsSimpleMarkerSymbolLayer *>( feature_layer_symbol_container->takeSymbolLayer( 0 ) );
            if ( feature_layer_symbol_first )
            {
              // prepare the second QgsSymbol from the first
              QgsSimpleMarkerSymbolLayer *feature_layer_symbol_first_clone_circle = static_cast<QgsSimpleMarkerSymbolLayer *>( feature_layer_symbol_first->clone() );
              if ( feature_layer_symbol_first_clone_circle )
              {
                // see core/symbology-ng/QgsSymbol.h, qgsmarkersymbollayerv2.h
                int i_valid_layer_count = 2;
                // Star
                color_background = color_red;
                color_foreground = color_yellow;
                feature_layer_symbol_first->setFillColor( color_background );
                feature_layer_symbol_first->setStrokeColor( Qt::black );
                feature_layer_symbol_first->setOutputUnit( QgsUnitTypes::RenderMillimeters );
                feature_layer_symbol_first->setSize( d_size_symbol_big );
                feature_layer_symbol_first->setStrokeWidthMapUnitScale( QgsMapUnitScale() );
                feature_layer_symbol_first->setShape( QgsSimpleMarkerSymbolLayerBase::Star );
                feature_layer_symbol_first->setStrokeStyle( Qt::DotLine );
                feature_layer_symbol_first->setPenJoinStyle( Qt::RoundJoin );
                // Circle
                // feature_layer_symbol_first_clone->setFillColor( Qt::transparent );
                feature_layer_symbol_first_clone_circle->setFillColor( color_foreground );
                feature_layer_symbol_first_clone_circle->setStrokeColor( Qt::black );
                feature_layer_symbol_first_clone_circle->setOutputUnit( QgsUnitTypes::RenderMillimeters );
                feature_layer_symbol_first_clone_circle->setSize( d_size_symbol_small );
                feature_layer_symbol_first_clone_circle->setStrokeWidthMapUnitScale( QgsMapUnitScale() );
                feature_layer_symbol_first_clone_circle->setShape( QgsSimpleMarkerSymbolLayerBase::Circle );
                feature_layer_symbol_first_clone_circle->setAngle( 0.0 );
                feature_layer_symbol_first_clone_circle->setStrokeStyle( Qt::DashDotLine );
                feature_layer_symbol_first_clone_circle->setPenJoinStyle( Qt::RoundJoin );
                // Set transparency for both and add Star, Circle
                feature_layer_symbol_container->setOpacity( 1.0 ); // was setAlpha( 1.0 );
                if ( !bToPixel )
                {
                  // prepare a (possible) third [gcp_master] QgsSymbol from the first, insuring that it has the correct default attributes
                  QgsSimpleMarkerSymbolLayer *feature_layer_symbol_first_clone_area = static_cast<QgsSimpleMarkerSymbolLayer *>( feature_layer_symbol_first->clone() );
                  if ( feature_layer_symbol_first_clone_area )
                  {
                    // Will show area of 2.5 meters around the Point [only noticeable when zoomed in]
                    // feature_layer_symbol_first_clone_area->setOutputUnit( QgsUnitTypes::RenderMetersToMapUnits );
                    feature_layer_symbol_first_clone_area->setOutputUnit( QgsUnitTypes::RenderMapUnits );
                    feature_layer_symbol_first_clone_area->setFillColor( color_area );
                    feature_layer_symbol_first_clone_area->setStrokeColor( color_goldenrod );
                    feature_layer_symbol_first_clone_area->setSize( mGcpPointArea );
                    feature_layer_symbol_first_clone_area->setStrokeWidth( ( mGcpPointArea / 20 ) );
                    feature_layer_symbol_first_clone_area->setStrokeWidthMapUnitScale( QgsMapUnitScale() );
                    feature_layer_symbol_container->appendSymbolLayer( feature_layer_symbol_first_clone_area );
                    feature_layer_symbol_container->setOpacity( 0.75 ); // was setAlpha( 0.75 );
                    i_valid_layer_count++;
                  }
                }
                feature_layer_symbol_container->appendSymbolLayer( feature_layer_symbol_first );
                feature_layer_symbol_container->appendSymbolLayer( feature_layer_symbol_first_clone_circle );
                if ( ( feature_layer_symbol_container->type() == QgsSymbol::Marker ) && ( feature_layer_symbol_container->symbolLayerCount() == i_valid_layer_count ) )
                {
                  // checking that we have what is desired
                  isDirty = true;
                }
              }
            }
          }
        }
        if ( isDirty )
        {
          layer_gcp->setRenderer( feature_layer_renderer );
          QgsSnappingConfig config( QgsProject::instance() );
          config.setEnabled( true );
          config.setMode( QgsSnappingConfig::AdvancedConfiguration );
          config.setIntersectionSnapping( false );  // only snap to layers
          config.setIndividualLayerSettings( layer_gcp, QgsSnappingConfig::IndividualLayerSettings(
                                               layer_gcp->isEditable(), QgsSnappingConfig::VertexAndSegment, mGcpPointArea, QgsTolerance::ProjectUnits ) );
        }
      }
    }
  }
  return layer_gcp->isValid();
}
double QgsGeorefPluginGui::getCutlineGcpMasterArea( QgsVectorLayer *layer_cutline )
{
  if ( ( !layer_cutline ) && ( mLayerGcpMaster ) )
  {
    layer_cutline = mLayerGcpMaster;
  }
  if ( !layer_cutline )
  {
    return mGcpMasterArea;
  }
  if ( ( layer_cutline->isValid() ) && ( layer_cutline->renderer() ) )
  {
    QgsFeatureRenderer *feature_layer_renderer = layer_cutline->renderer();
    if ( feature_layer_renderer )
    {
      QgsRenderContext feature_layer_symbols_context;
      QgsSymbolList feature_layer_symbols_list = feature_layer_renderer->symbols( feature_layer_symbols_context );
      if ( feature_layer_symbols_list.count() == 1 )
      {
        QgsSymbol *feature_layer_symbol_container = feature_layer_symbols_list.at( 0 );
        if ( feature_layer_symbol_container )
        {
          if ( ( feature_layer_symbol_container->type() == QgsSymbol::Marker ) && ( feature_layer_symbol_container->symbolLayerCount() == 3 ) )
          {
            QgsSimpleMarkerSymbolLayer *feature_layer_symbol_first =  static_cast<QgsSimpleMarkerSymbolLayer *>( feature_layer_symbol_container->symbolLayer( 0 )->clone() );
            if ( feature_layer_symbol_first )
            {
              mGcpMasterArea = feature_layer_symbol_first->size( );
            }
          }
        }
      }
    }
  }
  return mGcpMasterArea;
}
bool QgsGeorefPluginGui::setCutlineGcpMasterArea( double d_area, QgsVectorLayer *layer_cutline )
{
  bool b_rc = false;
  mGcpMasterArea = d_area;   // meter
  if ( ( !layer_cutline ) && ( mLayerGcpMaster ) )
  {
    layer_cutline = mLayerGcpMaster;
  }
  if ( !layer_cutline )
  {
    return b_rc;
  }
  if ( ( layer_cutline->isValid() ) && ( layer_cutline->renderer() ) )
  {
    QgsFeatureRenderer *feature_layer_renderer = layer_cutline->renderer();
    if ( feature_layer_renderer )
    {
      QgsRenderContext feature_layer_symbols_context;
      QgsSymbolList feature_layer_symbols_list = feature_layer_renderer->symbols( feature_layer_symbols_context );
      if ( feature_layer_symbols_list.count() == 1 )
      {
        QgsSymbol *feature_layer_symbol_container = feature_layer_symbols_list.at( 0 );
        if ( feature_layer_symbol_container )
        {
          if ( ( feature_layer_symbol_container->type() == QgsSymbol::Marker ) && ( feature_layer_symbol_container->symbolLayerCount() == 3 ) )
          {
            QgsSimpleMarkerSymbolLayer *feature_layer_symbol_first =  static_cast<QgsSimpleMarkerSymbolLayer *>( feature_layer_symbol_container->takeSymbolLayer( 0 ) );
            if ( feature_layer_symbol_first )
            {
              feature_layer_symbol_first->setSize( mGcpMasterArea );
              feature_layer_symbol_container->insertSymbolLayer( 0, feature_layer_symbol_first );
              layer_cutline->setRenderer( feature_layer_renderer );
              QgsSnappingConfig config( QgsProject::instance() );
              config.setEnabled( true );
              config.setMode( QgsSnappingConfig::AdvancedConfiguration );
              config.setIntersectionSnapping( false );  // only snap to layers
              config.setIndividualLayerSettings( layer_cutline, QgsSnappingConfig::IndividualLayerSettings(
                                                   layer_cutline->isEditable(), QgsSnappingConfig::VertexAndSegment, mGcpMasterArea, QgsTolerance::ProjectUnits ) );
              b_rc = true;
            }
          }
        }
      }
    }
  }
  return b_rc;
}
bool QgsGeorefPluginGui::setCutlineLayerSettings( QgsVectorLayer *layer_cutline )
{
  // Note: these are 'Point'-Layers
  Q_CHECK_PTR( layer_cutline );
  int i_table_type = 0; // cutline-tables
  QString s_CutlineLabelExpression = "name||' ['||id_gcp_coverage||', '||id_cutline||']'";
  QString s_id_name = "id_cutline";
  if ( layer_cutline->originalName() == mGcpMasterLayername )
  {
    i_table_type = 1; // gcp_master-tables
    s_CutlineLabelExpression = "name||' ['||valid_since||']'";
  }
  double d_size_base = fontPointSize;
  if ( d_size_base < 10.0 )
    d_size_base = 15.0;
  double d_size_font = d_size_base * 0.8;
  d_size_base = d_size_base / 3; // 6.0
  double d_size_symbol_big = d_size_base * 1.5; // 7.5
  double d_size_symbol_small = d_size_base * 0.50;   // 2.50
  // palegoldenrod #eee8aa rgb(238, 232, 170) ; near // "#dce3aa" 220, 227,170
  QColor color_palegoldenrod( "palegoldenrod" );
  QColor color_khaki( "khaki" );
  // cadetblue#5f9ea0 rgb( 95, 158, 160) ; near "#adc6cb" 173,198,203
  QColor color_cadetblue( "cadetblue" );
  QColor color_lightblue( "lightblue" );
  QColor color_darkgreen( "darkgreen" );
  QColor color_darkcyan( "darkcyan" );
  QColor color_cyan( "cyan" );
  QColor color_orangered( "orangered" );
  QColor color_crimson( "crimson" );
  QColor color_red( "red" );
  QColor color_background = color_darkgreen;
  QColor color_foreground = color_khaki;
  QColor color_shadow = color_cyan;
  int layer_type = -1;
  // Symbols:
  if ( ( layer_cutline->isValid() ) && ( layer_cutline->renderer() ) )
  {
    // see: gui/symbology-ng/qgsrendererv2propertiesdialog.cpp, qgslayerpropertieswidget.cpp and  app/qgsvectorlayerproperties.cpp
    QgsFeatureRenderer *feature_layer_renderer = layer_cutline->renderer();
    if ( feature_layer_renderer )
    {
      QgsRenderContext feature_layer_symbols_context;
      QgsSymbolList feature_layer_symbols_list = feature_layer_renderer->symbols( feature_layer_symbols_context );
      if ( feature_layer_symbols_list.count() == 1 )
      {
        // For the default seetings, there should only be 1
        bool isDirty = false;
        QgsSymbol *feature_layer_symbol_container = feature_layer_symbols_list.at( 0 );
        if ( feature_layer_symbol_container )
        {
          // Can be used for all types
          // cutline_points
          if ( ( feature_layer_symbol_container->type() == QgsSymbol::Marker ) && ( feature_layer_symbol_container->symbolLayerCount() == 1 ) )
          {
            // remove the only QgsSymbol from the list [will be replaced with a big green pentagon containing a small red cross]
            layer_type = 0;
            QgsSimpleMarkerSymbolLayer *feature_layer_symbol_first =  static_cast<QgsSimpleMarkerSymbolLayer *>( feature_layer_symbol_container->takeSymbolLayer( 0 ) );
            if ( feature_layer_symbol_first )
            {
              // prepare the second QgsSymbol from the first, insuring that it has the correct default attributes
              QgsSimpleMarkerSymbolLayer *feature_layer_symbol_first_clone_circle = static_cast<QgsSimpleMarkerSymbolLayer *>( feature_layer_symbol_first->clone() );
              if ( feature_layer_symbol_first_clone_circle )
              {
                // see core/symbology-ng/QgsSymbol.h, qgsmarkersymbollayerv2.h
                int i_valid_layer_count = 2;
                color_background = color_darkgreen;
                color_foreground = color_khaki;
                // https://www.w3.org/TR/SVG/types.html#ColorKeywords
                // Pentagon [darkgreen #006400]
                feature_layer_symbol_first->setFillColor( color_background );
                feature_layer_symbol_first->setStrokeColor( Qt::black );
                feature_layer_symbol_first->setOutputUnit( QgsUnitTypes::RenderMillimeters );
                feature_layer_symbol_first->setSize( d_size_symbol_big );
                feature_layer_symbol_first->setStrokeWidthMapUnitScale( QgsMapUnitScale() );
                feature_layer_symbol_first->setShape( QgsSimpleMarkerSymbolLayerBase::Star );
                feature_layer_symbol_first->setStrokeStyle( Qt::DotLine );
                feature_layer_symbol_first->setPenJoinStyle( Qt::RoundJoin );
                // Circle
                feature_layer_symbol_first_clone_circle->setFillColor( color_foreground );
                feature_layer_symbol_first_clone_circle->setStrokeColor( Qt::black );
                feature_layer_symbol_first_clone_circle->setOutputUnit( QgsUnitTypes::RenderMillimeters );
                feature_layer_symbol_first_clone_circle->setSize( d_size_symbol_small );
                feature_layer_symbol_first_clone_circle->setStrokeWidthMapUnitScale( QgsMapUnitScale() );
                feature_layer_symbol_first_clone_circle->setShape( QgsSimpleMarkerSymbolLayerBase::Circle );
                feature_layer_symbol_first_clone_circle->setAngle( 0.0 );
                feature_layer_symbol_first_clone_circle->setStrokeStyle( Qt::DashDotLine );
                feature_layer_symbol_first_clone_circle->setPenJoinStyle( Qt::RoundJoin );
                // Set transparency for both and add Pentagon, Cross
                feature_layer_symbol_container->setOpacity( 1.0 ); // was setAlpha( 1.0 );
                if ( i_table_type == 1 )
                {
                  // prepare a (possible) third [gcp_master] QgsSymbol from the first, insuring that it has the correct default attributes
                  QgsSimpleMarkerSymbolLayer *feature_layer_symbol_first_clone_area = static_cast<QgsSimpleMarkerSymbolLayer *>( feature_layer_symbol_first->clone() );
                  if ( feature_layer_symbol_first_clone_area )
                  {
                    // Will show area of 2.5 meters around the Point [only noticeable when zoomed in]
                    feature_layer_symbol_first_clone_area->setFillColor( color_red );
                    feature_layer_symbol_first_clone_area->setStrokeWidthMapUnitScale( QgsMapUnitScale() );
                    // feature_layer_symbol_first_clone_area->setOutputUnit( QgsUnitTypes::RenderMetersToMapUnits );
                    feature_layer_symbol_first_clone_area->setOutputUnit( QgsUnitTypes::RenderMapUnits );
                    feature_layer_symbol_first_clone_area->setSize( mGcpMasterArea );
                    feature_layer_symbol_container->appendSymbolLayer( feature_layer_symbol_first_clone_area );
                    feature_layer_symbol_container->setOpacity( 0.75 ); // was setAlpha( 0.75 );
                    i_valid_layer_count++;
                  }
                }
                feature_layer_symbol_container->appendSymbolLayer( feature_layer_symbol_first );
                feature_layer_symbol_container->appendSymbolLayer( feature_layer_symbol_first_clone_circle );
                if ( ( feature_layer_symbol_container->type() == QgsSymbol::Marker ) && ( feature_layer_symbol_container->symbolLayerCount() == i_valid_layer_count ) )
                {
                  // checking that we have what is desired
                  isDirty = true;
                }
              }
            }
          }
          // cutline_linestrings
          if ( ( feature_layer_symbol_container->type() == QgsSymbol::Line ) && ( feature_layer_symbol_container->symbolLayerCount() == 1 ) )
          {
            // remove the only QgsSymbol from the list [will be replaced with a wide dark-cyan solid line containing a slim Khaki dotted  line]
            layer_type = 1;
            QgsSimpleLineSymbolLayer *feature_layer_symbol_first =  static_cast<QgsSimpleLineSymbolLayer *>( feature_layer_symbol_container->takeSymbolLayer( 0 ) );
            if ( feature_layer_symbol_first )
            {
              // prepare the second QgsSymbol from the first, insuring that it has the correct default attributes
              QgsSimpleLineSymbolLayer *feature_layer_symbol_first_clone_dashline = static_cast<QgsSimpleLineSymbolLayer *>( feature_layer_symbol_first->clone() );
              if ( feature_layer_symbol_first_clone_dashline )
              {
                // see core/symbology-ng/QgsSymbol.h, qgslinesymbollayerv2.h
                // https://www.w3.org/TR/SVG/types.html#ColorKeywords
                // Outerline [darkcyan #008b8b]
                color_background = color_darkcyan;
                color_foreground = color_khaki;
                feature_layer_symbol_first->setColor( color_background );
                feature_layer_symbol_first->setStrokeColor( Qt::black );
                feature_layer_symbol_first->setOutputUnit( QgsUnitTypes::RenderMillimeters );
                feature_layer_symbol_first->setWidth( d_size_symbol_big );
                feature_layer_symbol_first->setMapUnitScale( QgsMapUnitScale() );
                feature_layer_symbol_first->setPenStyle( Qt::SolidLine );
                feature_layer_symbol_first->setPenJoinStyle( Qt::RoundJoin );
                // Innerline [khaki #f0e68c]
                feature_layer_symbol_first_clone_dashline->setColor( color_foreground );
                feature_layer_symbol_first_clone_dashline->setStrokeColor( Qt::black );
                feature_layer_symbol_first_clone_dashline->setOutputUnit( QgsUnitTypes::RenderMillimeters );
                feature_layer_symbol_first_clone_dashline->setWidth( d_size_symbol_small );
                feature_layer_symbol_first_clone_dashline->setMapUnitScale( QgsMapUnitScale() );
                feature_layer_symbol_first_clone_dashline->setPenStyle( Qt::DashLine );
                feature_layer_symbol_first_clone_dashline->setPenJoinStyle( Qt::RoundJoin );
                // Set transparency for both and add Inner and Outer Linestrings
                feature_layer_symbol_container->setOpacity( 0.80 ); // was setAlpha( 0.80 );
                feature_layer_symbol_container->appendSymbolLayer( feature_layer_symbol_first );
                feature_layer_symbol_container->appendSymbolLayer( feature_layer_symbol_first_clone_dashline );
                if ( ( feature_layer_symbol_container->type() == QgsSymbol::Line ) && ( feature_layer_symbol_container->symbolLayerCount() == 2 ) )
                {
                  // checking that we have what is desired
                  isDirty = true;
                }
              }
            }
          }
          // cutline_polygons
          if ( ( feature_layer_symbol_container->type() == QgsSymbol::Fill ) && ( feature_layer_symbol_container->symbolLayerCount() == 1 ) )
          {
            // remove the only QgsSymbol from the list [will be replaced with a yellow background containing a red grid]
            layer_type = 2;
            // QgsSimpleFillSymbolLayer *feature_layer_symbol_first =  static_cast<QgsSimpleFillSymbolLayer*>( feature_layer_symbol_container->symbolLayer( 0 )->clone() );
            QgsSimpleFillSymbolLayer *feature_layer_symbol_first =  static_cast<QgsSimpleFillSymbolLayer *>( feature_layer_symbol_container->takeSymbolLayer( 0 ) );
            if ( feature_layer_symbol_first )
            {
              // Being a different type (QgsGradientFillSymbolLayer instead of QgsSimpleFillSymbolLayer) must be created from scratch
              QgsGradientFillSymbolLayer *feature_layer_symbol_GradientFill = new QgsGradientFillSymbolLayer();
              if ( feature_layer_symbol_GradientFill )
              {
                // see core/symbology-ng/QgsSymbol.h, qgsfillsymbollayerv2.h
                // Grid
                // http://planet.qgis.org/planet/tag/gradient/
                // shapeburst fills, Using a data defined expression for random colors
                // https://www.w3.org/TR/SVG/types.html#ColorKeywords
                // orangered #ff4500 rgb(255, 69, 0)
                color_background = color_orangered;
                color_foreground = color_crimson;
                feature_layer_symbol_first->setFillColor( color_background );
                feature_layer_symbol_first->setBrushStyle( Qt::DiagCrossPattern );
                // crimson #dc143c rgb(220, 20, 60) ; #f40101
                feature_layer_symbol_first->setStrokeColor( color_crimson );
                feature_layer_symbol_first->setOutputUnit( QgsUnitTypes::RenderMillimeters );
                feature_layer_symbol_first->setStrokeWidth( d_size_symbol_small );
                feature_layer_symbol_first->setStrokeWidthMapUnitScale( QgsMapUnitScale() );
                feature_layer_symbol_first->setStrokeStyle( Qt::DashDotLine );
                feature_layer_symbol_first->setPenJoinStyle( Qt::RoundJoin );
                // Background
                QgsGradientFillSymbolLayer::GradientColorType colorType = QgsGradientFillSymbolLayer::SimpleTwoColor;
                QgsGradientFillSymbolLayer::GradientType type = QgsGradientFillSymbolLayer::Linear;
                QgsGradientFillSymbolLayer::GradientCoordinateMode coordinateMode = QgsGradientFillSymbolLayer::Feature;
                QgsGradientFillSymbolLayer::GradientSpread gradientSpread = QgsGradientFillSymbolLayer::Pad;
                QPointF point_01( 0.50, 0.00 );
                QPointF point_02( 0.50, 1.00 );
                double d_rotation = 45.0;
                feature_layer_symbol_GradientFill->setGradientColorType( colorType );
                feature_layer_symbol_GradientFill->setColor( color_palegoldenrod );
                feature_layer_symbol_GradientFill->setColor2( color_cadetblue );
                feature_layer_symbol_GradientFill->setGradientType( type );
                feature_layer_symbol_GradientFill->setCoordinateMode( coordinateMode );
                feature_layer_symbol_GradientFill->setGradientSpread( gradientSpread );
                feature_layer_symbol_GradientFill->setReferencePoint1( point_01 );
                feature_layer_symbol_GradientFill->setReferencePoint2( point_02 );
                feature_layer_symbol_GradientFill->setAngle( d_rotation );
                // Set transparency for grid and background
                feature_layer_symbol_container->setOpacity( 0.5 ); // was setAlpha( 0.5 );
                feature_layer_symbol_container->appendSymbolLayer( feature_layer_symbol_first );
                feature_layer_symbol_container->appendSymbolLayer( feature_layer_symbol_GradientFill );
                // qDebug() << QString( "QgsGeorefPluginGui::setCutlineLayerSettings -y- type[%1] " ).arg( feature_layer_renderer->type() );
                if ( ( feature_layer_symbol_container->type() == QgsSymbol::Fill ) && ( feature_layer_symbol_container->symbolLayerCount() == 2 ) )
                {
                  // checking that we have what is desired
                  isDirty = true;
                  QgsCategorizedSymbolRenderer *feature_layer_renderer_categorized = QgsCategorizedSymbolRenderer::convertFromRenderer( feature_layer_renderer );
                  if ( feature_layer_renderer_categorized )
                  {
                    // see core/symbology-ng/qgscategorizedsymbolrendererv2.h
                    // Note: not using '.clone()' is a 'No,no'
                    feature_layer_renderer_categorized->setSourceSymbol( feature_layer_symbol_container->clone() );
                    if ( mSvgColors.isEmpty() )
                    {
                      mSvgColors = createSvgColors( 0, true, false );
                    }
                    feature_layer_renderer_categorized->setClassAttribute( QString( "%1 \% %2" ).arg( s_id_name ).arg( mSvgColors.size() ) );
                    for ( int i = 0; i < mSvgColors.size(); i++ )
                    {
                      QVariant value( i );
                      QgsSymbol *new_container = feature_layer_symbol_container->clone();
                      QgsGradientFillSymbolLayer *new_symbol_background = static_cast<QgsGradientFillSymbolLayer *>( new_container->takeSymbolLayer( 1 ) );
                      QColor color_svg( mSvgColors.at( i ) );
                      new_symbol_background->setColor2( color_svg );
                      new_container->appendSymbolLayer( new_symbol_background );
                      // qDebug() << QString( "QgsGeorefPluginGui::setCutlineLayerSettings loop[%1,%2] -2-" ).arg( i ).arg( mSvgColors.at( i ) );
                      QgsRendererCategory category( value, new_container, value.toString(), true );
                      feature_layer_renderer_categorized->addCategory( category );
                    }
                    isDirty = false;
                    layer_cutline->setRenderer( feature_layer_renderer_categorized );
                  }
                }
              }
            }
          }
        }
        if ( isDirty )
        {
          layer_cutline->setRenderer( feature_layer_renderer );
          QgsSnappingConfig config( QgsProject::instance() );
          config.setEnabled( true );
          config.setMode( QgsSnappingConfig::AdvancedConfiguration );
          config.setIntersectionSnapping( false );  // only snap to layers
          config.setIndividualLayerSettings( layer_cutline, QgsSnappingConfig::IndividualLayerSettings(
                                               layer_cutline->isEditable(), QgsSnappingConfig::VertexAndSegment, mGcpMasterArea, QgsTolerance::ProjectUnits ) );
          // qDebug() << QString( "QgsGeorefPluginGui::setCutlineLayerSettings -za- [%1] " ).arg( isDirty );
        }
      }
    }
  }
  // Labels:
  if ( layer_cutline->isValid() )
  {
    // see: core/qgspallabeling.cpp/h and  app/qgslabelingwidget.cpp
    // fontPointSize=15 ; _size_font=12
    color_background = color_lightblue;
    // opacity as a double value between 0 (fully transparent) and 1 (totally opaque)
    int i_opacity = 50;
    if ( i_table_type == 1 )
    {
      color_background = color_palegoldenrod;
      color_shadow = color_khaki;
      i_opacity = 75;
    }
    QgsPalLayerSettings pal_layer_settings;
    // Access to labeling configuration, may be null if labeling is not used [default]
    if ( layer_cutline->labeling() )
    {
      QgsAbstractVectorLayerLabeling *vlLabeling = layer_cutline->labeling()->clone();
      if ( vlLabeling )
      {
        pal_layer_settings = vlLabeling->settings( layer_cutline->dataProvider()->name() );
      }
    }
    pal_layer_settings.drawLabels = true;
    pal_layer_settings.fieldName = s_CutlineLabelExpression;
    pal_layer_settings.isExpression = true;
    // Placement
    pal_layer_settings.dist = 5;
    pal_layer_settings.distInMapUnits = false;
    switch ( layer_type )
    {
      // 0=point ; 1= linestring ; 2=polygon
      case 1:
        pal_layer_settings.placement = QgsPalLayerSettings::OrderedPositionsAroundPoint;
        pal_layer_settings.placementFlags = QgsPalLayerSettings::AboveLine;
        break;
      case 2:
        // Visible Polygon, Force point inside polygon, QuadrantOver, Offset from centroid
        pal_layer_settings.centroidWhole = false;
        pal_layer_settings.centroidInside = true;
        pal_layer_settings.fitInPolygonOnly = false;
        pal_layer_settings.quadOffset = QgsPalLayerSettings::QuadrantOver;
        pal_layer_settings.placement = QgsPalLayerSettings::OverPoint;
        break;
      case 0:
      default:
        pal_layer_settings.placement = QgsPalLayerSettings::OrderedPositionsAroundPoint;
        pal_layer_settings.placementFlags = QgsPalLayerSettings::AboveLine;
        break;
    }
    // Wrap
    // pal_layer_settings.wrapChar = QString( "[" ); // removes character
    // pal_layer_settings.multilineHeight = 1.0;
    pal_layer_settings.multilineAlign = QgsPalLayerSettings::MultiCenter;
    // Retrieve Text-Format Settings
    QgsTextFormat pal_layer_textFormat = pal_layer_settings.format();
    pal_layer_textFormat.setFont( QApplication::font() );
    pal_layer_textFormat.setSize( d_size_font );
    pal_layer_textFormat.setSizeUnit( QgsUnitTypes::RenderPixels );
    pal_layer_textFormat.setColor( Qt::black );
#if 0
    // Retrieve Buffer [around Label-Text] Settings
    QgsTextBufferSettings pal_layer_buffer = pal_layer_textFormat.buffer();
    pal_layer_buffer.setEnabled( false );
    pal_layer_buffer.setColor( Qt::black );
    pal_layer_buffer.setSizeUnit( QgsUnitTypes::RenderPoints );
    pal_layer_buffer.setSize( d_size_font );
    // Replace Buffer [around Label-Text] Settings
    pal_layer_textFormat.setBuffer( pal_layer_buffer );
#endif
    // Retrieve Background [Shape]  Settings
    QgsTextBackgroundSettings pal_layer_background = pal_layer_textFormat.background();
    pal_layer_background.setEnabled( true );
    pal_layer_background.setType( QgsTextBackgroundSettings::ShapeEllipse );
    pal_layer_background.setSizeType( QgsTextBackgroundSettings::SizeBuffer );
    pal_layer_background.setSizeUnit( QgsUnitTypes::RenderMillimeters );
    pal_layer_background.setOpacity( i_opacity );
    pal_layer_background.setFillColor( color_background );
    pal_layer_background.setStrokeColor( color_darkgreen );
    pal_layer_background.setStrokeWidth( 1.0 );
    // Replace Background [Shape] to Settings
    pal_layer_textFormat.setBackground( pal_layer_background );
    // Retrieve  Shadow [Drop] Settings
    QgsTextShadowSettings pal_layer_shadow = pal_layer_textFormat.shadow();
    pal_layer_shadow.setEnabled( true );
    pal_layer_shadow.setShadowPlacement( QgsTextShadowSettings::ShadowLowest );
    pal_layer_shadow.setOffsetAngle( 135 );
    pal_layer_shadow.setOffsetDistance( 0 );
    pal_layer_shadow.setOffsetUnit( QgsUnitTypes::RenderMillimeters );
    pal_layer_shadow.setOffsetGlobal( true );
    pal_layer_shadow.setBlurRadius( 1.50 );
    pal_layer_shadow.setBlurRadiusUnit( QgsUnitTypes::RenderMillimeters );
    pal_layer_shadow.setBlurAlphaOnly( false );
    // opacity as a double value between 0 (fully transparent) and 1 (totally opaque)
    pal_layer_shadow.setOpacity( 70.0 );
    pal_layer_shadow.setScale( 100 );
    pal_layer_shadow.setColor( color_shadow );
    pal_layer_shadow.setBlendMode( QPainter::CompositionMode_Multiply );
    // Replace Shadow [Drop] to Settings
    pal_layer_textFormat.setShadow( pal_layer_shadow );
    // Replace Text-Format to Settings
    pal_layer_settings.setFormat( pal_layer_textFormat );
    // Write settings to layer
    layer_cutline->setLabeling( new QgsVectorLayerSimpleLabeling( pal_layer_settings ) );
  }
  return layer_cutline->isValid();
}
// GcpDatabase slots
void QgsGeorefPluginGui::setGdalScriptType()
{
  mGdalScriptOrGcpList = !mGdalScriptOrGcpList;
  mActionGdalScriptType->setChecked( mGdalScriptOrGcpList );
  QString s_icon = "/mActionRotatePointSymbols.svg";
  QString s_text = "Switch to Gdal-Script [Full Script]";
  if ( mGdalScriptOrGcpList )
  {
    s_text = "Switch to Gdal-Script [Gcp-List only]";
    s_icon = "/mActionCalculateField.svg"; // providerGdal.svg
  }
  mActionGdalScriptType->setText( s_text );
  mActionGdalScriptType->setToolTip( s_text );
  mActionGdalScriptType->setStatusTip( s_text );
  mActionGdalScriptType->setIcon( QgsApplication::getThemeIcon( s_icon ) );
  QString sGDALScript = "Full";
  if ( mGdalScriptOrGcpList )
    sGDALScript = "GcpList";
  QgsSettings settings;
  settings.setValue( QStringLiteral( "/Plugin-GeoReferencer/Config/GDALScript" ), sGDALScript );
}
void QgsGeorefPluginGui::setLayerProviderType()
{
  bool isSpatialite = true;
  QString s_text = "Switch to Ogr-Provider [from Spatialite]";
  QString s_icon = "/mActionAddSpatiaLiteLayer.svg";
  if ( mLayerProviderType.isNull() )
  {
    mLayerProviderType = "spatialite";
  }
  else
  {
    if ( mLayerProviderType == "spatialite" )
    {
      mLayerProviderType = "ogr";
      isSpatialite = false;
      s_text = "Switch to Spatialite-Provider [from Ogr]";
      s_icon = "/mActionAddOgrLayer.svg";
    }
    else
    {
      mLayerProviderType = "spatialite";
      isSpatialite = true;
    }
  }
  mActionLayerProviderType->setIcon( QgsApplication::getThemeIcon( s_icon ) );
  mActionLayerProviderType->setChecked( isSpatialite );
  mActionLayerProviderType->setText( s_text );
  mActionLayerProviderType->setToolTip( s_text );
  mActionLayerProviderType->setStatusTip( s_text );
  QgsSettings settings;
  settings.setValue( QStringLiteral( "/Plugin-GeoReferencer/Config/LayerProviderType" ), mLayerProviderType );
}
void QgsGeorefPluginGui::setPointsPolygon()
{
  if ( mPointsPolygon )
  {
    if ( mLayerMercatorPolygons )
    {
      mLayerTreeView->setCurrentLayer( mLayerMercatorPolygons );
    }
  }
  else
  {
    if ( mLayerGcpPixels )
    {
      mLayerTreeView->setCurrentLayer( mLayerGcpPixels );
    }
  }
}
void QgsGeorefPluginGui::openGcpCoveragesDb()
{
  QgsSettings s;
  QString dir = s.value( QStringLiteral( "/Plugin-GeoReferencer/gcpdb_directory" ) ).toString();
  if ( dir.isEmpty() )
    dir = '.';
  // Gcp-Database (*.db);; Tiff (*.tif)
  QString otherFiles = tr( "Gcp-Database (*.gcp.db);;" );
  QString lastUsedFilter = s.value( QStringLiteral( "/Plugin-GeoReferencer/lastuseddb" ), otherFiles ).toString();

  QString filters; //  = QgsProviderRegistry::instance()->fileRasterFilters();
  filters.prepend( otherFiles + ";;" );
  filters.chop( otherFiles.size() + 2 );
  QString s_GcpCoverages_FileName = QFileDialog::getOpenFileName( this, tr( "Open GcpCoverages-Database" ), dir, otherFiles, &lastUsedFilter );
  // mModifiedRasterFileName = QLatin1String( "" );

  if ( s_GcpCoverages_FileName.isEmpty() )
    return;

  QFileInfo database_file( s_GcpCoverages_FileName );
  if ( database_file.exists() )
  {
    QDir parent_dir( database_file.canonicalPath() );
    s.setValue( QStringLiteral( "/Plugin-GeoReferencer/gcpdb_directory" ), database_file.canonicalPath() );
    s.setValue( QStringLiteral( "/Plugin-GeoReferencer/lastusedb" ), lastUsedFilter );
    readGcpDb( s_GcpCoverages_FileName, false );
    if ( mGcpDbData )
    {
      if ( mGcpDbData->gcp_coverages.count() > 0 )
      {
        mListGcpCoverages->setEnabled( true );
      }
    }
    listGcpCoverages();
  }
}
void QgsGeorefPluginGui::openGcpMasterDb()
{
  QgsSettings s;
  QString dir = s.value( QStringLiteral( "/Plugin-GeoReferencer/gcpdb_directory" ) ).toString();
  if ( dir.isEmpty() )
    dir = '.';
  // Gcp-Database (*.db);; Tiff (*.tif)
  QString otherFiles = tr( "Gcp-Database (*gcp_master*.db);;" );
  QString lastUsedFilter = s.value( QStringLiteral( "/Plugin-GeoReferencer/gcp_master_db" ), otherFiles ).toString();

  QString filters; //  = QgsProviderRegistry::instance()->fileRasterFilters();
  filters.prepend( otherFiles + ";;" );
  filters.chop( otherFiles.size() + 2 );
  QString s_GcpMaster_FileName = QFileDialog::getOpenFileName( this, tr( "Open GcpMaster-Database" ), dir, otherFiles, &lastUsedFilter );
  // mModifiedRasterFileName = QLatin1String( "" );

  if ( s_GcpMaster_FileName.isEmpty() )
    return;

  QFileInfo database_file( s_GcpMaster_FileName );
  if ( database_file.exists() )
  {
    int i_return_srid = createGcpMasterDb( database_file.canonicalFilePath() );
    if ( i_return_srid != INT_MIN )
    {
      // will return srid being used in the gcp_master, otherwise INT_MIN if not found
      mGcpMasterDatabaseFileName = database_file.canonicalFilePath();
      mGcpMasterSrid = i_return_srid;
      QDir parent_dir( database_file.canonicalPath() );
      s.setValue( QStringLiteral( "/Plugin-GeoReferencer/gcpdb_directory" ), database_file.canonicalPath() );
      s.setValue( QStringLiteral( "/Plugin-GeoReferencer/gcp_master_db" ), mGcpMasterDatabaseFileName );
      loadGcpMaster( mGcpMasterDatabaseFileName ); // Will be added if mTreeGroupGeoreferencer exists and is loadable
    }
  }
}
void QgsGeorefPluginGui::createGcpDatabaseDialog()
{
  QgsGcpDatabaseDialog create_gcpdb( mIface->mapCanvas()->mapSettings().destinationCrs() );
  if ( create_gcpdb.exec() == QDialog::Accepted )
  {
    QFileInfo database_file = create_gcpdb.GcpDatabaseFile();
    bool b_sqlDump = create_gcpdb.GcpDatabaseDump();
    int i_srid = create_gcpdb.GcpDatabaseSrid();
    QgsSpatiaLiteGcpUtils::GcpDatabases gcp_db_type = create_gcpdb.GcpDatabaseType();
    if ( ! database_file.exists() )
    {
      // since the file does not exist, canonicalFilePath() cannot be used [use: absoluteFilePath()]
      switch ( gcp_db_type )
      {
        case QgsSpatiaLiteGcpUtils::GcpCoverages:
        {
          // qDebug() << QString( "QgsGeorefPluginGui::createGcpDatabaseDialog -1- GcpCoverages srid[%1] dump[%2] file[%3]" ).arg( i_srid ).arg( b_sqlDump ).arg( database_file.absoluteFilePath() );
          if ( createGcpCoverageDb( database_file.absoluteFilePath(), i_srid, b_sqlDump ) == i_srid )
          {
            // will return srid being used in the gcp_master, otherwise INT_MIN if not found
            statusBar()->showMessage( tr( "Created GcpCoverages: srid[%1] dump[%2] in %3" ).arg( i_srid ).arg( b_sqlDump ).arg( database_file.absoluteFilePath() ) );
          }
          else
          {
            statusBar()->showMessage( tr( "Creation failed: GcpCoverages: srid[%1] dump[%2] in %3" ).arg( i_srid ).arg( b_sqlDump ).arg( database_file.absoluteFilePath() ) );
          }
        }
        break;
        case QgsSpatiaLiteGcpUtils::GcpMaster:
        {
          // qDebug() << QString( "QgsGeorefPluginGui::createGcpDatabaseDialog -2- GcpMaster srid[%1] dump[%2] file[%3]" ).arg( i_srid ).arg( b_sqlDump ).arg( database_file.absoluteFilePath() );
          if ( createGcpMasterDb( database_file.absoluteFilePath(), i_srid, b_sqlDump ) == i_srid )
          {
            // will return srid being used in the gcp_master, otherwise INT_MIN if not found
            statusBar()->showMessage( tr( "Created GcpMaster: srid[%1] dump[%2] in %3" ).arg( i_srid ).arg( b_sqlDump ).arg( database_file.absoluteFilePath() ) );
          }
          else
          {
            statusBar()->showMessage( tr( "Creation failed: GcpMaster: srid[%1] dump[%2] in %3" ).arg( i_srid ).arg( b_sqlDump ).arg( database_file.absoluteFilePath() ) );
          }
        }
        break;
      }
    }
  }
}
void QgsGeorefPluginGui::createSqlDumpGcpCoverage()
{
  if ( mGcpDbData )
  {
    if ( QFile::exists( mGcpDbData->mGcpDatabaseFileName ) )
    {
      int return_srid = createGcpCoverageDb( mGcpDbData->mGcpDatabaseFileName, mGcpDbData->mGcpSrid, true );
      if ( return_srid == mGcpDbData->mGcpSrid )
      {
        statusBar()->showMessage( tr( "Created GcpCoverages Sql-Dump: srid[%1] in %2" ).arg( mGcpDbData->mGcpSrid ).arg( mGcpDbData->mGcpDatabaseFileName ) );
      }
      else
      {
        statusBar()->showMessage( tr( "Creation failed: GcpCoverages Sql-Dump: srid[%1] received[%2]in %3" ).arg( mGcpDbData->mGcpSrid ).arg( return_srid ).arg( mGcpDbData->mGcpDatabaseFileName ) );
      }
    }
  }
}
void QgsGeorefPluginGui::exportGcpCoverageAsGcpMaster()
{
  if ( mGcpDbData )
  {
    if ( QFile::exists( mGcpDbData->mGcpDatabaseFileName ) )
    {
      // the GcpMasterDb (with this name) will be created if it does not exsist.
      QString sDatabaseFilename = QString( "%1/%2.db" ).arg( mRasterFilePath ).arg( QString( "gcp_master_%1" ).arg( mGcpDbData->mGcpCoverageName ) );
      QgsSpatiaLiteGcpUtils::GcpDbData *parmsGcpDbData = nullptr;
      parmsGcpDbData = new QgsSpatiaLiteGcpUtils::GcpDbData( mGcpDbData->mGcpDatabaseFileName, mGcpDbData->mGcpCoverageName, mGcpDbData->mGcpSrid );
      parmsGcpDbData->mIdGcpCoverage = mGcpDbData->mIdGcpCoverage;
      parmsGcpDbData->mGcpMasterDatabaseFileName = sDatabaseFilename;
      parmsGcpDbData->mGcpMasterSrid = mGcpDbData->mGcpSrid;
      parmsGcpDbData->mSqlDump = true;
      if ( QgsSpatiaLiteGcpUtils::exportGcpCoverageAsGcpMaster( parmsGcpDbData ) )
      {
        statusBar()->showMessage( tr( "Exported '%1': to in %2" ).arg( mGcpDbData->mGcpCoverageName ).arg( parmsGcpDbData->mGcpMasterDatabaseFileName ) );
      }
      else
      {
        statusBar()->showMessage( tr( "Export of '%1' as GcpMaster failed:  %2" ).arg( mGcpDbData->mGcpCoverageName ).arg( sDatabaseFilename ) );
      }
      parmsGcpDbData = nullptr;
    }
    else
    {
      statusBar()->showMessage( tr( "GcpCoverageDb for '%2' not found:  %1" ).arg( mGcpDbData->mGcpCoverageName ).arg( mGcpDbData->mGcpDatabaseFileName ) );
    }
  }
}
void QgsGeorefPluginGui::createSqlDumpGcpMaster()
{
  if ( mGcpDbData )
  {
    if ( QFile::exists( mGcpMasterDatabaseFileName ) )
    {
      int return_srid = createGcpMasterDb( mGcpMasterDatabaseFileName, mGcpMasterSrid, true );
      if ( return_srid == mGcpMasterSrid )
      {
        statusBar()->showMessage( tr( "Created GcpMaster Sql-Dump: srid[%1] in %2" ).arg( mGcpMasterSrid ).arg( mGcpMasterDatabaseFileName ) );
      }
      else
      {
        statusBar()->showMessage( tr( "Creation failed: GcpMaster Sql-Dump: srid[%1], received[%2] in %3" ).arg( mGcpMasterSrid ).arg( return_srid ).arg( mGcpMasterDatabaseFileName ) );
      }
    }
  }
}
void QgsGeorefPluginGui::listGcpCoverages()
{
  if ( mGcpDbData )
  {
    if ( mGcpDbData->gcp_coverages.count() > 0 )
    {
      // Will build list of Coverages read from Gcp-Database
      QgsGcpCoverages::instance()->openDialog( this, mGcpDbData );
    }
  }
}
void QgsGeorefPluginGui::loadGcpCoverage( int id_selected_coverage )
{
  // Result of Coverage selection from QgsGcpCoverages
  if ( mGcpDbData->mIdGcpCoverage != id_selected_coverage )
  {
    // Do not load a raster that is already loaded
    QString s_value = mGcpDbData->gcp_coverages.value( id_selected_coverage );
    QStringList sa_fields = s_value.split( mGcpDbData->mParseString );
    if ( sa_fields.count() >= 23 )
    {
      // 22=path ; 3=file
      QFile raster_file( QString( "%1/%2" ).arg( sa_fields.at( 22 ) ).arg( sa_fields.at( 3 ) ) );
      if ( raster_file.exists() )
      {
        // Do not load a raster that does not exist
        QString s_title = sa_fields.at( 5 ); // title
        if ( s_title.isEmpty() )
          s_title = sa_fields.at( 1 ); // coverage_name
        openRaster( raster_file );
      }
    }
  }
}
// Mapcanvas QGIS
void QgsGeorefPluginGui::loadGTifInQgis( const QString &gtif_file )
{
  if ( mLoadGTifInQgis )
  {
    QFileInfo raster_file( gtif_file );
    if ( raster_file.exists() )
    {
      mLayerGtifRaster = new QgsRasterLayer( raster_file.canonicalFilePath(), raster_file.completeBaseName() );
      if ( mLayerGtifRaster )
      {
        if ( mTreeGroupGtifRasters )
        {
          // so that layer is not added to legend
          QgsProject::instance()->addMapLayers( QList<QgsMapLayer *>() << mLayerGtifRaster, false, false );
          // must be added AFTER being registered !
          mTreeLayerGtifRaster = mTreeGroupGtifRasters->addLayer( mLayerGtifRaster );
        }
        else
        {
          // so that layer is added to legend
          QgsProject::instance()->addMapLayers( QList<QgsMapLayer *>() << mLayerGtifRaster, true, false );
        }
      }
    }
  }
}
bool QgsGeorefPluginGui::exportLayerDefinition( QgsLayerTreeGroup *group_layer, QString file_path )
{
  // Note: this is intended more to help in debugging styling structures created in setCutlineLayerSettings and setGcpLayerSettings
  bool saved = false;
  if ( group_layer )
  {
    if ( ( file_path.isNull() ) || ( file_path.isEmpty() ) )
    {
      //  General valid file, pathnames: 0-9, a-z, A-Z, '.', '_', and '-' / "[^[:alnum:]._-[:space:]]" [replace space with '_']
      file_path = QString( "%1/%2.qlr" ).arg( mRasterFilePath ).arg( group_layer->name().replace( QRegExp( "[^[:alnum:]._-]" ), "_" ) );
    }
    QString errorMessage = QString( "" );
    saved = QgsLayerDefinition::exportLayerDefinition( file_path, group_layer->children(), errorMessage );
    QFile definition_file( file_path );
    if ( ( definition_file.exists() ) && ( definition_file.size() == 0 ) )
    {
      saved = false;
    }
    if ( !saved )
    {
      if ( definition_file.exists() )
      {
        definition_file.remove();
      }
    }
  }
  return saved;
}
QStringList QgsGeorefPluginGui::createSvgColors( int i_method, bool b_reverse, bool b_grey_black )
{
  Q_UNUSED( i_method );
  QStringList list_SvgColors;
  int i_max_colors_pink = 6;
  int i_max_colors_red = 9;
  int i_max_colors_orange = 5;
  int i_max_colors_yellow = 11;
  int i_max_colors_brown = 17;
  int i_max_colors_green = 20;
  int i_max_colors_cyan = 11;
  int i_max_colors_blue = 15;
  int i_max_colors_purple = 17; // (Purple, violet, and magenta)
  int i_max_colors_white = 17;
  int i_max_colors_gray_black = 10;
  int i_max_colors = 0;
  int i_max_groups = 9;
  QStringList list_pink;
  QStringList list_red;
  QStringList list_orange;
  QStringList list_yellow;
  QStringList list_brown;
  QStringList list_green;
  QStringList list_cyan;
  QStringList list_blue;
  QStringList list_purple;
  QStringList list_white;
  QStringList list_gray_black;
  list_pink.reserve( i_max_colors_pink );
  list_red.reserve( i_max_colors_red );
  list_orange.reserve( i_max_colors_orange );
  list_yellow.reserve( i_max_colors_yellow );
  list_brown.reserve( i_max_colors_brown );
  list_green.reserve( i_max_colors_green );
  list_cyan.reserve( i_max_colors_cyan );
  list_blue.reserve( i_max_colors_blue );
  list_purple.reserve( i_max_colors_purple );
  if ( b_grey_black )
  {
    list_white.reserve( i_max_colors_white );
    list_gray_black.reserve( i_max_colors_gray_black );
    // ------- [light to dark]
    list_white << "white" << "honeydew"  << "mintcream"  << "azure"  << "aliceblue" ;
    list_white << "ghostwhite"   << "whitesmoke"   << "ivory" << "snow"  << "floralwhite"  ;
    list_white << "seashell" << "oldlace"   << "linen"  << "lavenderblush" << "beige" ;
    list_white << "antiquewhite"  << "mistyrose" ;
    i_max_colors_white = list_white.size() - 1;
    // ------- 'grey' will be used (same rgb value as 'gray')
    list_gray_black << "gainsboro" << "lightgrey"  << "silver"  << "darkgrey"  << "lightslategrey" ;
    list_gray_black << "slategrey"  << "grey" << "dimgrey" << " darkslategrey"  << "black" ;
    i_max_colors_gray_black = list_gray_black.size() - 1;
    // -------
    if ( b_reverse )
    {
      // for(int k=0, s=list_white.size(), max=(s/2); k<max; k++) list_white.swap(k,s-(1+k));
      // std::reverse( list_white.begin(), list_white.end() );
      // std::reverse( list_gray_black.begin(), list_gray_black.end() );
    }
    i_max_colors += list_white.size() + list_gray_black.size();
    i_max_groups += 2;
  }
  else
  {
    i_max_colors_white = -1;
    i_max_colors_gray_black = -1;
  }
  // -------  [light to dark]
  list_pink << "pink" << "lightpink"  << "hotpink"  << "deeppink"  << "palevioletred"  << "mediumvioletred";
  i_max_colors_pink = list_pink.size() - 1;
  i_max_colors += list_pink.size();
  // -------
  list_red << "lightsalmon" << "salmon"  << "darksalmon"  << "lightcoral" << "indianred";
  list_red << "red" << "crimson" << "firebrick"  << "darkred";
  i_max_colors_red = list_red.size() - 1;
  i_max_colors += list_red.size();
  // -------
  list_orange << "orange" << "darkorange"  << "coral"  << "tomato"  << "orangered";
  i_max_colors_orange = list_orange.size() - 1;
  i_max_colors += list_orange.size();
  // -------
  list_yellow << "lightyellow" << "lemonchiffon"  << "lightgoldenrodyellow"  << "papayawhip"  << "moccasin"  << "peachpuff";
  list_yellow << "palegoldenrod" << "khaki"  << "yellow"  << "gold"  << "darkkhaki";
  i_max_colors_yellow = list_yellow.size() - 1;
  i_max_colors += list_yellow.size();
  // -------
  list_brown << "cornsilk" << "blanchedalmond"  << "bisque"  << "navajowhite"  << "wheat"  << "burlywood";
  list_brown << "tan" << "rosybrown"  << "sandybrown"  << "goldenrod"  << "darkgoldenrod"  << "peru";
  list_brown << "chocolate" << "saddlebrown"  << "sienna"  << "brown"  << "maroon";
  i_max_colors_brown = list_brown.size() - 1;
  i_max_colors += list_brown.size();
  // -------
  list_green << "lightgreen" << "palegreen"  << "greenyellow"  << "chartreuse"  << "lawngreen";
  list_green << "lime" << "springgreen"  << "mediumspringgreen"  << "yellowgreen"  << "limegreen";
  list_green << "darkseagreen" << "mediumaquamarine"  << "mediumseagreen"  << "olivedrab"  << "olive";
  list_green << "seagreen" << "forestgreen"  << "green"  << "darkolivegreen"  << "darkgreen";
  i_max_colors_green = list_green.size() - 1;
  i_max_colors += list_green.size();
  // ------- agua has the same rgb value as cyan
  list_cyan << "lightcyan" << "paleturquoise"  << "aquamarine"  << "cyan"  << "turquoise" << "mediumturquoise";
  list_cyan << "darkturquoise"  << "lightseagreen"  << "cadetblue"  << "darkcyan" << "teal";
  i_max_colors_cyan = list_cyan.size() - 1;
  i_max_colors += list_cyan.size();
  // -------
  list_blue << "powderblue" << "lightblue"  << "lightsteelblue"  << "skyblue"  << "lightskyblue";
  list_blue << "deepskyblue" << "cornflowerblue"  << "steelblue"  << "dodgerblue"  << "royalblue";
  list_blue << "blue" << "mediumblue"  << "darkblue"  << "navy"  << "midnightblue";
  i_max_colors_blue = list_blue.size() - 1;
  i_max_colors += list_blue.size();
  // ------- Purple, violet, and magenta colors [magenta has the same rgb value as fuchsia]
  list_purple << "lavender" << "thistle"  << "plum"  << "violet"  << "orchid"  << "magenta";
  list_purple << "mediumorchid" << "mediumpurple"   << "darkslateblue"  << "slateblue" << "blueviolet"  << "darkviolet";
  list_purple  << "darkorchid"  << "darkmagenta" << "purple" << "indigo"   << "mediumslateblue";
  i_max_colors_purple = list_purple.size() - 1;
  i_max_colors += list_purple.size();
  // -------
  //SvgColors[138,11] or SvgColors[111,9]
  list_SvgColors.reserve( i_max_colors );
  // -------
  int i_entry_addition = -1;
  if ( !b_reverse )
  {
    i_entry_addition = 1;
    i_max_colors_pink = 0;
    i_max_colors_red = 0;
    i_max_colors_orange = 0;
    i_max_colors_yellow = 0;
    i_max_colors_brown = 0;
    i_max_colors_green = 0;
    i_max_colors_cyan = 0;
    i_max_colors_blue = 0;
    i_max_colors_purple = 0;
    if ( b_grey_black )
    {
      i_max_colors_white = 0;
      i_max_colors_gray_black = 0;
    }
  }
  for ( int i = 0,  loop = 0; i < i_max_colors; loop++ )
  {
    if ( loop >  i_max_colors )
    {
      // should never happen, but just in case ...
      break;
    }
    if ( ( i_max_colors_pink >= 0 ) && ( i_max_colors_pink < list_pink.size() ) )
    {
      // qDebug() << QString( "SvgColors[%1,%4] pink[%2,%5] [%3] " ).arg( i ).arg( i_max_colors_pink ).arg( list_pink.at( i_max_colors_pink ) ).arg( i_max_colors ).arg( list_pink.size() - 1 );
      list_SvgColors.append( list_pink.at( i_max_colors_pink ) );
      i_max_colors_pink += i_entry_addition;
      i++;
    }
    if ( ( i_max_colors_red >= 0 ) && ( i_max_colors_red < list_red.size() ) )
    {
      list_SvgColors.append( list_red.at( i_max_colors_red ) );
      i_max_colors_red += i_entry_addition;
      i++;
    }
    if ( ( i_max_colors_orange >= 0 ) && ( i_max_colors_orange < list_orange.size() ) )
    {
      list_SvgColors.append( list_orange.at( i_max_colors_orange ) );
      i_max_colors_orange += i_entry_addition;
      i++;
    }
    if ( ( i_max_colors_yellow >= 0 ) && ( i_max_colors_yellow < list_yellow.size() ) )
    {
      list_SvgColors.append( list_yellow.at( i_max_colors_yellow ) );
      i_max_colors_yellow += i_entry_addition;
      i++;
    }
    if ( ( i_max_colors_brown >= 0 ) && ( i_max_colors_brown < list_brown.size() ) )
    {
      list_SvgColors.append( list_brown.at( i_max_colors_brown ) );
      i_max_colors_brown += i_entry_addition;
      i++;
    }
    if ( ( i_max_colors_green >= 0 ) && ( i_max_colors_green < list_green.size() ) )
    {
      list_SvgColors.append( list_green.at( i_max_colors_green ) );
      i_max_colors_green += i_entry_addition;
      i++;
    }
    if ( ( i_max_colors_cyan >= 0 ) && ( i_max_colors_cyan < list_cyan.size() ) )
    {
      list_SvgColors.append( list_cyan.at( i_max_colors_cyan ) );
      i_max_colors_cyan += i_entry_addition;
      i++;
    }
    if ( ( i_max_colors_blue >= 0 ) && ( i_max_colors_blue < list_blue.size() ) )
    {
      list_SvgColors.append( list_blue.at( i_max_colors_blue ) );
      i_max_colors_blue += i_entry_addition;
      i++;
    }
    if ( ( i_max_colors_purple >= 0 ) && ( i_max_colors_purple < list_purple.size() ) )
    {
      list_SvgColors.append( list_purple.at( i_max_colors_purple ) );
      i_max_colors_purple += i_entry_addition;
      i++;
    }
    if ( ( i_max_colors_white >= 0 ) && ( i_max_colors_white < list_white.size() ) )
    {
      list_SvgColors.append( list_white.at( i_max_colors_white ) );
      i_max_colors_white += i_entry_addition;
      i++;
    }
    if ( ( i_max_colors_gray_black >= 0 ) && ( i_max_colors_gray_black < list_gray_black.size() ) )
    {
      list_SvgColors.append( list_gray_black.at( i_max_colors_gray_black ) );
      i_max_colors_gray_black += i_entry_addition;
      i++;
    }
  }
  // -------
  return list_SvgColors;
}
void QgsGeorefPluginGui::jumpToGcpConvert( QgsPointXY inputPoint, bool bToPixel )
{
  QgsRectangle canvas_extent;
  if ( bToPixel )
  {
    if ( updateGeorefTransform() )
    {
      QgsRectangle boundingBox = transformViewportBoundingBox( mIface->mapCanvas()->extent(), mGeorefTransform, false );
      canvas_extent = mGeorefTransform.hasCrs() ? mGeorefTransform.getBoundingBox( boundingBox, false ) : boundingBox;
    }
    else
    {
      canvas_extent = mCanvas->extent();
    }
  }
  else
  {
    if ( updateGeorefTransform() )
    {
      QgsRectangle rectMap = mGeorefTransform.hasCrs() ? mGeorefTransform.getBoundingBox( mCanvas->extent(), true ) : mCanvas->extent();
      canvas_extent = transformViewportBoundingBox( rectMap, mGeorefTransform, true );
    }
    else
    {
      canvas_extent = mIface->mapCanvas()->extent();
    }
  }
  QgsPointXY canvas_center = canvas_extent.center();
  QgsPointXY canvas_diff( inputPoint.x() - canvas_center.x(), inputPoint.y() - canvas_center.y() );
  QgsRectangle canvas_extent_new( canvas_extent.xMinimum() + canvas_diff.x(), canvas_extent.yMinimum() + canvas_diff.y(),
                                  canvas_extent.xMaximum() + canvas_diff.x(), canvas_extent.yMaximum() + canvas_diff.y() );
  mExtentsChangedRecursionGuard = true;
  if ( bToPixel )
  {
    mCanvas->setExtent( canvas_extent_new );
    mCanvas->refresh();
  }
  else
  {
    mIface->mapCanvas()->setExtent( canvas_extent_new );
    mIface->mapCanvas()->refresh();
  }
  mExtentsChangedRecursionGuard = false;
}
bool QgsGeorefPluginGui::saveEditsGcp( QgsVectorLayer *gcp_layer, bool leaveEditable )
{
  bool b_rc = false;
#if 0
  if ( ( gcp_layer ) && ( !gcp_layer->isValid() ) )
  {
    // If not valid can cause a crash of the application
    qDebug() << QString( "-W-> QgsGeorefPluginGui::saveEditsGcp: layer_name[%1] Invalid error[%2]" ).arg( gcp_layer->name() ).arg( gcp_layer->error().message( QgsErrorMessage::Text ) );
  }
#endif
  if ( !leaveEditable )
  {
    if ( ( gcp_layer ) && ( gcp_layer->isValid() ) && ( gcp_layer->isEditable() ) && ( gcp_layer->editBuffer()->isModified() ) )
    {
      gcp_layer->commitChanges();
      gcp_layer->endEditCommand();
      qDebug() << QString( "-I-> QgsGeorefPluginGui::saveEditsGcp[%2,%3]: layer_name[%1] isModified[%4]" ).arg( gcp_layer->name() ).arg( "After endEditCommand" ).arg( b_rc ).arg( gcp_layer->isModified() );
    }
    return true;
  }
  QStringList sa_errors;
  if ( ( gcp_layer ) && ( gcp_layer->isValid() ) && ( gcp_layer->isEditable() ) && ( gcp_layer->editBuffer()->isModified() ) )
  {
    //  [When used with 'gcp_layer->commitChanges' instead of 'editBuffer()->commitChanges', caused crashes] Warning: QUndoStack::endMacro(): no matching beginMacro()
    b_rc = gcp_layer->editBuffer()->commitChanges( sa_errors );
    if ( ( gcp_layer->editBuffer()->isModified() ) && ( sa_errors.size() ) )
    {
      QString s_error = sa_errors.join( ";" );
      // gcp_layer->error().message( QgsErrorMessage::Text )
      // Warning: QUndoStack::setIndex(): cannot set index in the middle of a macro
      if ( ! s_error.startsWith( "SUCCESS" ) )
      {
        qDebug() << QString( "-W-> QgsGeorefPluginGui::saveEditsGcp: layer_name[%1] Invalid error[%2] b_rc=%3" ).arg( gcp_layer->name() ).arg( s_error ).arg( b_rc );
        gcp_layer->rollBack( false ); // clear for the next AddPoint
      }
    }
  }
  else
  {
    if ( ( gcp_layer ) && ( gcp_layer->isValid() ) && ( gcp_layer->isEditable() ) && ( !gcp_layer->isModified() ) )
    {
      // [Hack] After a gcp_layer->addFeature(..) in fetchGcpMasterPointsExtent: isModified() returns false, which is not true
      if ( leaveEditable )
      {
        // - so we will force a commit when we are NOT shutting down
        b_rc = gcp_layer->editBuffer()->commitChanges( sa_errors );
      }
    }
  }
  if ( b_rc )
  {
    // without 'triggerRepaint' crashes occur
    gcp_layer->triggerRepaint();
  }
  else
  {
    for ( int i = 0; i < sa_errors.size(); i++ )
    {
      qDebug() << QString( "-E-> QgsGeorefPluginGui::saveEditsGcp[%1] error[%2]" ).arg( gcp_layer->name() ).arg( sa_errors.at( i ) );
    }
  }
  return b_rc;
}
void QgsGeorefPluginGui::featureAdded_gcp( QgsFeatureId fid )
{
  qDebug() << QString( "QgsGeorefPluginGui::featureAdded_gcp[%1]" ).arg( fid );
  if ( ( mLayerGcpPoints ) && ( mLayerGcpPixels ) )
  {
    // Default assumtion: a map-point has been added and the pixel-point must be updated
    bool bToPixel = false;
    QString s_Event = "Pixel-Point to Map-Point";
    QgsVectorLayer *layer_gcp_event = mLayerGcpPixels;
    QgsVectorLayer *layer_gcp_update = mLayerGcpPoints;
    QgsFeature fet_point_test = layer_gcp_event->getFeature( fid );
    if ( ( fet_point_test.id() !=  fid ) || ( !fet_point_test.isValid() ) )
    {
      // A Map-point has been added and the pixel-point must be updated, switch pointers
      bToPixel = true;
      s_Event = "Map-Point to Pixel-Point";
      layer_gcp_event = mLayerGcpPoints;
      layer_gcp_update = mLayerGcpPixels;
    }
    qDebug() << QString( "QgsGeorefPluginGui::featureAdded_gcp[%1] [%2,%3] srid=%4" ).arg( fid ).arg( bToPixel ).arg( s_Event ).arg( mGcpSrid );
    if ( fid < 0 )
    {
      //  fid is negative for new features
      if ( bToPixel )
        mEventPointStatus = 1;
      else
        mEventPointStatus = 2;
      // Search for (highest) unique id_gcp
      int id_gcp_next = 0;
      QgsAttributeList lst_search_gcp;
      lst_search_gcp.append( 0 ); // id_gcp
      QgsFeature fet_gcp;
      QgsFeatureIterator fit_gcp = layer_gcp_event->getFeatures( QgsFeatureRequest().setSubsetOfAttributes( lst_search_gcp ) );
      while ( fit_gcp.nextFeature( fet_gcp ) )
      {
        int id_gcp_search = fet_gcp.attribute( QString( "id_gcp" ) ).toInt();
        if ( id_gcp_search > id_gcp_next )
        {
          id_gcp_next = id_gcp_search;
        }
      }
      id_gcp_next++;
      QgsFeatureMap addedFeatures = layer_gcp_event->editBuffer()->addedFeatures();
      // Avoid setting this in the Add Dialog
      addedFeatures[fid].setAttribute( "id_gcp", id_gcp_next );
      // qDebug() << QString( "QgsGeorefPluginGui::featureAdded_gcp[%1] [%2,%3] mEventPointStatus[%4]" ).arg( fid ).arg( bToPixel ).arg( s_Event ).arg( mEventPointStatus );
      if ( addedFeatures[fid].attribute( QString( "id_gcp_coverage" ) ).toInt() <= 0 )
      {
        // do this only when the id_gcp_coverage is not known (importing from Gcp-Master) [default values are already set]
        addedFeatures[fid].setAttribute( "name", mGcpBaseFileName );
        addedFeatures[fid].setAttribute( "id_gcp_coverage", mIdGcpCoverage );
        addedFeatures[fid].setAttribute( "order_selected", getGcpTransformParam( mTransformParam ) );
      }
      mEventGcpStatus = fid;
      if ( saveEditsGcp( layer_gcp_event, true ) )
      {
        // Note: during the commitChanges, this function will run again with a positive 'fid', which will be stored in 'mEventGcpStatus'
        QgsFeature fet_point_event;
        QgsAttributeList lst_needed_fields;
        lst_needed_fields.append( 0 ); // id_gcp
        lst_needed_fields.append( 2 ); // name
        lst_needed_fields.append( 3 ); // notes
        if ( layer_gcp_event->getFeatures( QgsFeatureRequest( mEventGcpStatus ).setSubsetOfAttributes( lst_needed_fields ) ).nextFeature( fet_point_event ) )
        {
          // Retrieve the newly added record and extract the primary key
          QgsPointXY added_point_event = fet_point_event.geometry().asPoint();
          int id_gcp = fet_point_event.attribute( QString( "id_gcp" ) ).toInt(); // Should now be the same as id_gcp_next
          QString sName = fet_point_event.attribute( QString( "name" ) ).toString();
          QString sNotes = fet_point_event.attribute( QString( "notes" ) ).toString();
          // qDebug() << QString( "QgsGeorefPluginGui::featureAdded_gcp[%1]  mEventPointStatus[%2] id_gcp[%3]" ).arg( fid ).arg( mEventPointStatus ).arg( id_gcp );
          if ( id_gcp > 0 )
          {
            // Both geometries are in the same table
            QgsFeature fet_point_update;
            QgsFeatureRequest update_request = QgsFeatureRequest().setFilterExpression( QString( "id_gcp=%1" ).arg( id_gcp ) );
            if ( layer_gcp_update->getFeatures( update_request ).nextFeature( fet_point_update ) )
            {
              // Retrieve the (now added) other-geometry which by default is 0 0
              QgsPointXY map_point;
              QgsPointXY pixelPoint;
              QgsPointXY added_point_update = getGcpConvert( mGcpCoverageName, added_point_event, bToPixel, mTransformParam );
              if ( bToPixel )
              {
                pixelPoint = added_point_update;
                map_point = added_point_event;
              }
              else
              {
                pixelPoint = added_point_event;
                map_point = added_point_update;
              }
              QString s_map_points = QString( "pixel[%1] map[%2] srid=%3" ).arg( pixelPoint.wellKnownText() ).arg( map_point.wellKnownText() ).arg( mGcpSrid );
              s_Event = QString( "%1 id_gcp[%2] - %3[%4]" ).arg( s_Event ).arg( id_gcp ).arg( "Adding Point" ).arg( s_map_points );
              // qDebug() << QString( "QgsGeorefPluginGui::featureAdded_gcp[%1] -yy- after commit update_fid[%2]  event_modified[%3] update_modified[%4]" ).arg( s_Event ).arg( fet_point_update.id() ).arg( layer_gcp_event->isModified() ).arg( layer_gcp_update->isModified() );
              if ( fet_point_update.geometry().asPoint() != added_point_update )
              {
                // We  have a usable result
                QgsGeometry geometry_update = QgsGeometry::fromPoint( added_point_update );
                fet_point_update.setGeometry( geometry_update );
                if ( layer_gcp_update->updateFeature( fet_point_update ) )
                {
                  if ( saveEditsGcp( layer_gcp_update, true ) )
                  {
                    // TODO: resove problem layer_gcp_event->isModified() == true
                    mEventPointStatus = 0;
                    qDebug() << QString( "QgsGeorefPluginGui::featureAdded_gcp[%1] [%2] gcp_next[%3] SpatialiteGcpEnabled[%4] SpatialiteGcpOff[%5]" ).arg( fid ).arg( bToPixel ).arg( id_gcp_next ).arg( mSpatialiteGcpEnabled ).arg( mSpatialiteGcpOff );
                    updateGcpTranslate( mGcpCoverageName );
                  }
                  else
                  {
                    // an error has accured
                    id_gcp = -2;
                    s_Event = QString( "-E-> QgsGeorefPluginGui::featureAdded_gcp -gcp_update- saveEditsGcp [%1]" ).arg( s_Event );
                    qDebug() << s_Event << layer_gcp_update->commitErrors();
                  }
                }
                mEventGcpStatus = 0;
                mEventPointStatus = 0;
                QgsGeorefDataPoint *dataPoint = new QgsGeorefDataPoint( mCanvas, mIface->mapCanvas(), pixelPoint, map_point, mGcpSrid, id_gcp, 1 );
                QString  sTextInfo  = QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( id_gcp );
                sTextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( mIdGcpCoverage );
                sTextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( mGcpPointsTableName );
                sTextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( sName );
                sTextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( sNotes );
                sTextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( getGcpTransformParam( mTransformParam ) );
                dataPoint->setTextInfo( sTextInfo );
                mGcpListWidget->addDataPoint( dataPoint );
                updateGeorefTransform();
                jumpToGcpConvert( added_point_update, bToPixel );
                // qDebug() << QString( "QgsGeorefPluginGui::featureAdded_gcp[%1] -zz- after commit update_fid[%2]  event_modified[%3] update_modified[%4] TextInfo[%5]" ).arg( s_Event ).arg( fet_point_update.id() ).arg( layer_gcp_event->isModified() ).arg( layer_gcp_update->isModified() ).arg( sTextInfo );
              }
              else
              {
                // call dialog to set point
                mEventGcpStatus = id_gcp;
                if ( bToPixel )
                {
                  // Map-Point has been created, enter a Pixel-Point
                  mMapCoordsDialog = new QgsMapCoordsDialog( mCanvas, map_point, id_gcp, this );
                }
                else
                {
                  // Pixel-Point has been created, enter a Map-Point
                  mMapCoordsDialog = new QgsMapCoordsDialog( mIface->mapCanvas(), pixelPoint, id_gcp, this );
                }
                mEventPointStatus = 0;
                qDebug() << QString( "QgsGeorefPluginGui::featureAdded_gcp[%1] -calling dialog- update_fid[%2]  event_modified[%3] update_modified[%4]" ).arg( s_Event ).arg( mEventGcpStatus ).arg( layer_gcp_event->isModified() ).arg( layer_gcp_update->isModified() );
                // connect( mMapCoordsDialog, &QgsMapCoordsDialog::pointAdded, this, &QgsGeorefPluginGui::addPoint );
                connect( mMapCoordsDialog, SIGNAL( pointAdded( const QgsPointXY &, const QgsPointXY &, const int &, const bool & ) ), this, SLOT( addPoint( const QgsPointXY &, const QgsPointXY &, const int &, const bool & ) ) );
                mMapCoordsDialog->show();
              }
            }
            else
            {
              qDebug() << QString( "QgsGeorefPluginGui::featureAdded_gcp[%1]  -layer_gcp_update- could not fetch feature mEventPointStatus[%2] id_gcp[%3]" ).arg( fid ).arg( mEventPointStatus ).arg( id_gcp );
              qDebug() << s_Event << layer_gcp_event->commitErrors();
            }
          }
        }
      }
      else
      {
        // an error has accured
        s_Event = QString( "-E-> QgsGeorefPluginGui::featureAdded_gcp -gcp_event- saveEditsGcp[%1]" ).arg( s_Event );
        qDebug() << s_Event << layer_gcp_event->commitErrors();
      }
      mEventGcpStatus = 0;
      mEventPointStatus = 0;
      return;
    }
    if ( fid > 0 )
    {
      //  fid, when positive, will be returned during commitChanges for the new feature
      mEventGcpStatus = fid;
      return;
    }
  }
}
void QgsGeorefPluginGui::attributeChanged_gcp( QgsFeatureId fid, int idx, const QVariant &value )
{
  Q_UNUSED( value );
  if ( ( mLayerGcpPoints ) && ( mLayerGcpPixels ) )
  {
    if ( ( idx == 2 ) || ( idx == 3 ) )
    {
      QgsVectorLayer *layer_gcp_event = mLayerGcpPoints;
      if ( mLayerGcpPixels->editBuffer()->changedGeometries().size() > 0 )
      {
        layer_gcp_event = mLayerGcpPixels;
      }
      int id_gcp = -1;
      QgsFeature fet_point_event;
      fet_point_event = layer_gcp_event->getFeature( fid );
      if ( fet_point_event.id() ==  fid )
      {
        id_gcp = fet_point_event.attribute( QString( "id_gcp" ) ).toInt();
        if ( id_gcp > 0 )
        {
          QgsGeorefDataPoint *dataPoint = mGcpListWidget->getDataPoint( id_gcp );
          if ( dataPoint )
          {
            QString  sTextInfo  = QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( id_gcp );
            sTextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( mIdGcpCoverage );
            sTextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( mGcpPointsTableName );
            sTextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( fet_point_event.attribute( QString( "name" ) ).toString() );
            sTextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( fet_point_event.attribute( QString( "notes" ) ).toString() );
            sTextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( getGcpTransformParam( mTransformParam ) );
            dataPoint->setTextInfo( sTextInfo );
            mGcpListWidget->setDirty(); // Inform the model, that changes have been made.
            // Update any changes made to the Meta-Data
            mGcpListWidget->updateGcpList();
          }
        }
      }
    }
  }
}
void QgsGeorefPluginGui::geometryChanged_gcp( QgsFeatureId fid, const QgsGeometry &changed_geometry )
{
  if ( mEventPointStatus > 0 )
    return;
  if ( ( mLayerGcpPoints ) && ( mLayerGcpPixels ) )
  {
    // Default assumtion: a map-point has been added and the pixel-point must be updated
    bool bToPixel = true;
    QString s_Event = "Map-Point to Pixel-Point";
    QgsVectorLayer *layer_gcp_event = mLayerGcpPoints;
    QgsVectorLayer *layer_gcp_update = mLayerGcpPixels;
    // int i_features_delete_event = layer_gcp_event->editBuffer()->deletedFeatureIds().size();
    // int i_features_delete_update = layer_gcp_update->editBuffer()->deletedFeatureIds().size();
    // qDebug() << QString( "QgsGeorefPluginGui::geometryChanged_gcp[%1] -1- delete event[%2] update[%3]" ).arg( fid ).arg( i_features_delete_event ).arg( i_features_delete_update );
    if ( layer_gcp_update->editBuffer()->changedGeometries().size() > 0 )
    {
      // A pixel-point has been changed and the map-point must be updated, switch pointers
      bToPixel = false;
      s_Event = "Pixel-Point to Map-Point";
      layer_gcp_event = mLayerGcpPixels;
      layer_gcp_update = mLayerGcpPoints;
    }
    int i_features_changed_event = layer_gcp_event->editBuffer()->changedGeometries().size();
    //qDebug() << QString( "QgsGeorefPluginGui::geometryChanged_gcp[%1] -1- changedGeometries().size[%2] bToPixel/type[%3,%4]" ).arg( fid ).arg( i_features_changed_event ).arg( bToPixel ).arg( s_Event );
    if ( fid > 0 )
    {
      //  fid, when positive, will be returned during commitChanges for the new feature
      mEventGcpStatus = fid;
      int id_gcp = -1;
      QgsFeature fet_point_event;
      fet_point_event = layer_gcp_event->getFeature( fid );
      if ( fet_point_event.id() ==  fid )
      {
        if ( fet_point_event.isValid() )
        {
          id_gcp = fet_point_event.attribute( QString( "id_gcp" ) ).toInt();
          if ( id_gcp > 0 )
          {
            int i_updateGcpList = 0;
            if ( changed_geometry.type() == QgsWkbTypes::PointGeometry )
            {
              // The Point has being moved [TODO: check if it has really moved, if possible]
              s_Event = QString( "%1 id_gcp[%2] - %3[%4]" ).arg( s_Event ).arg( id_gcp ).arg( "Moving Point" ).arg( changed_geometry.exportToWkt() );
              i_updateGcpList = 1;
            }
            else
            {
              // The Point is being deleted with the Node-MapTool
              s_Event = QString( "%1 id_gcp[%2] - %3[%4]" ).arg( s_Event ).arg( id_gcp ).arg( "Deleting Point" ).arg( changed_geometry.exportToWkt() );
              layer_gcp_event->deleteFeature( fid );
              i_updateGcpList = 2;
            }
            if ( i_updateGcpList > 0 )
            {
              // Common task for both update and delete
              if ( saveEditsGcp( layer_gcp_event, true ) )
              {
                // Commit and continue editing, with triggerRepaint()
                saveEditsGcp( layer_gcp_update, true ); // will save only isModified
              }
              else
              {
                // an error has accured
                id_gcp = -1;
                s_Event = QString( "-E-> QgsGeorefPluginGui::geometryChanged_gcp saveEditsGcp[%1] %2" ).arg( i_features_changed_event ).arg( s_Event );
                qDebug() << s_Event << layer_gcp_event->commitErrors();
              }
              switch ( i_updateGcpList )
              {
                case 1:
                {
                  QgsGeorefDataPoint *dataPoint = mGcpListWidget->getDataPoint( id_gcp );
                  if ( dataPoint )
                  {
                    dataPoint->updateDataPoint( bToPixel, changed_geometry.asPoint() );
                    // Update any changes made to the Meta-Data
                    QString  sTextInfo  = QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( id_gcp );
                    sTextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( mIdGcpCoverage );
                    sTextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( mGcpPointsTableName );
                    sTextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( fet_point_event.attribute( QString( "name" ) ).toString() );
                    sTextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( fet_point_event.attribute( QString( "notes" ) ).toString() );
                    sTextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( getGcpTransformParam( mTransformParam ) );
                    dataPoint->setTextInfo( sTextInfo );
                    mGcpListWidget->setDirty(); // Inform the model, that changes have been made.
                  }
                }
                break;
                case 2:
                  // When a Pixel or Map-Point has been deleted both points will be removed from their Canvases.
                  mGcpListWidget->removeDataPoint( id_gcp );
                  break;
              }
              if ( mGcpListWidget->isDirty() )
              {
                // 'updateGcpList' will rebuild the list and recalculate the 'residual' values.
                updateGeorefTransform();
              }
            }
            // qDebug() << QString( "QgsGeorefPluginGui::geometryChanged_gcp[%1]\n\t\t -zz- after commit fid[%2] event_modified[%3] update_modified[%4]" ).arg( s_Event ).arg( mEventGcpStatus ).arg( layer_gcp_event->isModified() ).arg( layer_gcp_update->isModified() );
          }
        }
      }
    }
  }
}
void QgsGeorefPluginGui::featureAdded_cutline( QgsFeatureId fid )
{
  QgsVectorLayer *layer_cutline_event = nullptr;
  if ( fid > 0 )
  {
    return;
  }
  int i_cutline_type = 0;
  if ( ( !layer_cutline_event ) && ( mLayerCutlinePoints ) && ( mLayerCutlinePoints->isModified() ) )
  {
    layer_cutline_event = mLayerCutlinePoints;
    i_cutline_type = 100;
  }
  if ( ( !layer_cutline_event ) && ( mLayerCutlineLinestrings ) && ( mLayerCutlineLinestrings->isModified() ) )
  {
    layer_cutline_event = mLayerCutlineLinestrings;
    i_cutline_type = 101;
  }
  if ( ( !layer_cutline_event ) && ( mLayerCutlinePolygons ) && ( mLayerCutlinePolygons->isModified() ) )
  {
    layer_cutline_event = mLayerCutlinePolygons;
    i_cutline_type = 102;
  }
  if ( ( !layer_cutline_event ) && ( mLayerMercatorPolygons ) && ( mLayerMercatorPolygons->isModified() ) )
  {
    layer_cutline_event = mLayerMercatorPolygons;
    i_cutline_type = 77; // to be used as a cutline
  }
  if ( ( !layer_cutline_event ) && ( mLayerGcpMaster ) && ( mLayerGcpMaster->isModified() ) )
  {
    // gcp_master does not belong to the cutline groupe, save asis
    if ( saveEditsGcp( mLayerGcpMaster, true ) )
    {
      // Commit and continue editing, with triggerRepaint()
      qDebug() << QString( "QgsGeorefPluginGui::featureAdded_cutline[%1]  featureAdded[%3]" ).arg( fid ).arg( mGcpMasterLayername );
    }
  }
  if ( layer_cutline_event )
  {
    //QgsFeatureMap &addedFeatures = const_cast<QgsFeatureMap &>( layer_cutline_event->editBuffer()->addedFeatures() );
    QgsFeatureMap addedFeatures = layer_cutline_event->editBuffer()->addedFeatures();
    addedFeatures[fid].setAttribute( "id_gcp_coverage", mIdGcpCoverage );
    addedFeatures[fid].setAttribute( "cutline_type",  i_cutline_type );
    addedFeatures[fid].setAttribute( "belongs_to_01",  mGcpBaseFileName );
    addedFeatures[fid].setAttribute( "belongs_to_02", mRasterFileName );
    if ( saveEditsGcp( layer_cutline_event, true ) )
    {
      // Commit and continue editing, with triggerRepaint()
      qDebug() << QString( "QgsGeorefPluginGui::featureAdded_cutline[%1]  featureAdded[%3]" ).arg( fid ).arg( mGcpBaseFileName );
    }
  }
}
void QgsGeorefPluginGui::geometryChanged_cutline( QgsFeatureId fid, const QgsGeometry &changed_geometry )
{
  Q_UNUSED( fid );
  Q_UNUSED( changed_geometry );
  QgsVectorLayer *layer_cutline_event = nullptr;
  if ( ( !layer_cutline_event ) && ( mLayerCutlinePoints ) && ( mLayerCutlinePoints->editBuffer()->changedGeometries().size() > 0 ) )
  {
    layer_cutline_event = mLayerCutlinePoints;
  }
  if ( ( !layer_cutline_event ) && ( mLayerCutlineLinestrings ) && ( mLayerCutlineLinestrings->editBuffer()->changedGeometries().size() > 0 ) )
  {
    layer_cutline_event = mLayerCutlineLinestrings;
  }
  if ( ( !layer_cutline_event ) && ( mLayerCutlinePolygons ) && ( mLayerCutlinePolygons->editBuffer()->changedGeometries().size() > 0 ) )
  {
    layer_cutline_event = mLayerCutlinePolygons;
  }
  if ( ( !layer_cutline_event ) && ( mLayerMercatorPolygons ) && ( mLayerMercatorPolygons->editBuffer()->changedGeometries().size() > 0 ) )
  {
    layer_cutline_event = mLayerMercatorPolygons;
  }
  if ( ( !layer_cutline_event ) && ( mLayerGcpMaster ) && ( mLayerGcpMaster->editBuffer()->changedGeometries().size() > 0 ) )
  {
    // gcp_master does not belong to the cutline group, save anyway
    layer_cutline_event = mLayerGcpMaster;
  }
  if ( layer_cutline_event )
  {
    // To delete a point must be selected
    if ( saveEditsGcp( layer_cutline_event, true ) )
    {
      // Commit and continue editing, with triggerRepaint()
      // qDebug() << QString( "QgsGeorefPluginGui::geometryChanged_cutline[%1]  changed_geometry[%2]" ).arg( fid ).arg( changed_geometry.exportToWkt() );
    }
  }
  else
  {
    qDebug() << QString( "QgsGeorefPluginGui::geometryChanged_cutline[%1]  unresolved changed_geometry[%2]" ).arg( fid ).arg( changed_geometry.exportToWkt() );
  }
}
//-----------------------------------------------------------
//  Wrappers for QgsSpatiaLiteGcpUtils functions
//-----------------------------------------------------------
bool QgsGeorefPluginGui::readGcpDb( QString  sDatabaseFilename, bool b_DatabaseDump )
{
  bool b_rc = false;
  if ( mGcpSrid < 0 )
  {
    mGcpSrid = readProjectSrid();
  }
  QgsSpatiaLiteGcpUtils::GcpDbData *parmsGcpDbData = nullptr;
  parmsGcpDbData = new QgsSpatiaLiteGcpUtils::GcpDbData( sDatabaseFilename, QString::null, mGcpSrid );
  if ( QgsSpatiaLiteGcpUtils::createGcpDb( parmsGcpDbData ) )
  {
    if ( parmsGcpDbData->mDatabaseValid )
    {
      if ( parmsGcpDbData->gcp_coverages.size() > 0 )
      {
        // qDebug() << QString( "QgsGeorefPluginGui::readGcpDb -replacing GcpDbData-pointer- old[%1] new[%2]" ).arg( QString().sprintf( "%8p", mGcpDbData ) ).arg( QString().sprintf( "%8p", parmsGcpDbData ) );
        mGcpDbData = parmsGcpDbData;
        mGcp_coverages = mGcpDbData->gcp_coverages;
        mGcpDatabaseFileName = mGcpDbData->mGcpDatabaseFileName;
        mListGcpCoverages->setEnabled( true );
        connect( QgsGcpCoverages::instance(), &QgsGcpCoverages::loadRasterCoverage, this, &QgsGeorefPluginGui::loadGcpCoverage );
        if ( b_DatabaseDump )
        {
          parmsGcpDbData->mSqlDump = true;
          QgsSpatiaLiteGcpUtils::createGcpDb( parmsGcpDbData );
        }
        b_rc = true;
      }
      else
      {
        qDebug() << QString( "QgsGeorefPluginGui::readGcpDb - no Gcp-Coverages found - db[%1] error[%2]" ).arg( parmsGcpDbData->mGcpDatabaseFileName ).arg( parmsGcpDbData->mError );
      }
    }
    else
    {
      qDebug() << QString( "QgsGeorefPluginGui:readGcpDb -  Database considered invalid - db[%1] error[%2]" ).arg( parmsGcpDbData->mGcpDatabaseFileName ).arg( parmsGcpDbData->mError );
    }
  }
  else
  {
    qDebug() << QString( "QgsGeorefPluginGui::readGcpDb - open of Database failed - db[%1] error[%2]" ).arg( parmsGcpDbData->mGcpDatabaseFileName ).arg( parmsGcpDbData->mError );
  }
  parmsGcpDbData = nullptr;
  return b_rc;
}
bool QgsGeorefPluginGui::createGcpDb( bool b_DatabaseDump )
{
  QFileInfo raster_file( mRasterFileName );
  mRasterFilePath = raster_file.canonicalPath(); // Path to the file, without file-name
  QString sRasterFileName = raster_file.fileName(); // file-name without path
  if ( mGcpSrid < 0 )
  {
    mGcpSrid = readProjectSrid();
  }
  mGcpCoverageNameBase = raster_file.completeBaseName(); // file without extension
  mGcpCoverageName = mGcpCoverageNameBase.toLower(); // file without extension (Lower-Case)
  QStringList sa_list_id_fields = mGcpCoverageNameBase.split( "." );
  if ( sa_list_id_fields.size() > 2 )
  {
    // Possilbly a format is being used: 'year.name.scale.*': 1986.n-33-123-b-c-2_30.5000
    QString s_year = sa_list_id_fields[0];
    QString s_scale = sa_list_id_fields[2];
    bool ok;
    int i_year = sa_list_id_fields[0].toInt( &ok, 10 );
    if ( ( ok ) && ( sa_list_id_fields[0].length() == 4 ) )
    {
      int i_scale = sa_list_id_fields[2].toInt( &ok, 10 );
      if ( ok )
      {
        mRasterScale = i_scale;
        mRasterYear = i_year;
        mGcpCoverageNameBase = sa_list_id_fields[1];
        QString s_build = QString( "%1.%2.%3" ).arg( QString( "%1" ).arg( mRasterYear, 4, 10, QChar( '0' ) ) ).arg( mGcpCoverageNameBase ).arg( mRasterScale );
        if ( mGcpSrid > 0 )
        {
          // give this a default name
          mModifiedRasterFileName = QString( "%1.%2.map.%3" ).arg( s_build ).arg( mGcpSrid ).arg( raster_file.suffix() );
        }
        else
        {
          mModifiedRasterFileName = QString( "%1.0.map.%3" ).arg( s_build ).arg( raster_file.suffix() );
        }
      }
    }
  }
  if ( !mGcpDbData )
  {
    mGcpDbData = new QgsSpatiaLiteGcpUtils::GcpDbData( mGcpDatabaseFileName, mGcpCoverageName, mGcpSrid, mGcpPointsTableName, mLayerRaster, mSpatialiteGcpEnabled );
  }
  mGcpDbData->mIdGcpCoverage = mIdGcpCoverage;
  mGcpDbData->mIdGcpCutline = mIdGcpCutline;
  mGcpDbData->mTransformParam = getGcpTransformParam( mTransformParam );
  mGcpDbData->mResamplingMethod = getGcpResamplingMethod( mResamplingMethod );
  mGcpDbData->mCompressionMethod = mCompressionMethod;
  mGcpDbData->mGcpPointsFileName = mGcpPointsFileName;
  mGcpDbData->mGcpBaseFileName = mGcpBaseFileName;
  mGcpDbData->mRasterFilePath = mRasterFilePath;
  mGcpDbData->mRasterFileName = sRasterFileName;
  mGcpDbData->mGcpCoverageName = mGcpCoverageName;
  mGcpDbData->mGcpCoverageNameBase = mGcpCoverageNameBase;
  mGcpDbData->mModifiedRasterFileName = mModifiedRasterFileName;
  mGcpDbData->mRasterYear = mRasterYear;
  mGcpDbData->mRasterScale = mRasterScale;
  mGcpDbData->mRasterNodata = mRasterNodata;
  mGcpDbData->mGcpMasterArea = mGcpMasterArea;
  if ( mLayerGcpMaster )
  {
    mGcpDbData->mInputPoint = mLayerGcpMaster->extent().center();
  }
  // mInputPoint
  if ( b_DatabaseDump )
    mGcpDbData->mDatabaseDump = true;
  if ( QgsSpatiaLiteGcpUtils::createGcpDb( mGcpDbData ) )
  {
    if ( mGcpDbData->mDatabaseValid )
    {
      mGcpSrid = mGcpDbData->mGcpSrid;
      mGcpPointsTableName = mGcpDbData->mGcpPointsTableName;
      mSpatialiteGcpEnabled = mGcpDbData->mGcpEnabled;
      mIdGcpCoverage = mGcpDbData->mIdGcpCoverage;
      mIdGcpCutline = mGcpDbData->mIdGcpCutline;
      mTransformParam = setGcpTransformParam( mGcpDbData->mTransformParam );
      mCompressionMethod = mGcpDbData->mCompressionMethod;
      mResamplingMethod = setGcpResamplingMethod( mGcpDbData->mResamplingMethod );
      mModifiedRasterFileName = mGcpDbData->mModifiedRasterFileName;
      mRasterNodata = mGcpDbData->mRasterNodata;
      mError = mGcpDbData->mError;
      if ( mGcpMasterArea != mGcpDbData->mGcpMasterArea )
      {
        mGcpMasterArea = mGcpDbData->mGcpMasterArea;
        if ( mLayerGcpMaster )
        {
          setCutlineGcpMasterArea( mGcpDbData->mGcpMasterArea, mLayerGcpMaster );
        }
      }
      if ( mGcpDbData->gcp_coverages.size() > 0 )
      {
        mGcp_coverages = mGcpDbData->gcp_coverages;
        mListGcpCoverages->setEnabled( true );
        connect( QgsGcpCoverages::instance(), &QgsGcpCoverages::loadRasterCoverage, this, &QgsGeorefPluginGui::loadGcpCoverage );
      }
      else
      {
        qDebug() << QString( "QgsGeorefPluginGui::createGcpDb  - Gcp-Coverage[%3] not found - db[%1] error[%2]" ).arg( mGcpDbData->mGcpDatabaseFileName ).arg( mGcpDbData->mError ).arg( mGcpCoverageName );
      }
    }
    else
    {
      qDebug() << QString( "QgsGeorefPluginGui:createGcpDb -  Database considered invalid - db[%1] error[%2]" ).arg( mGcpDbData->mGcpDatabaseFileName ).arg( mGcpDbData->mError );
    }
    return true;
  }
  else
  {
    qDebug() << QString( "QgsGeorefPluginGui::createGcpDb - open of Database failed - db[%1] error[%2]" ).arg( mGcpDbData->mGcpDatabaseFileName ).arg( mGcpDbData->mError );
  }
  // createGcpMasterDb( s_gcp_master_db, s_gcp_master_tablename, mGcpSrid );
  mError = mGcpDbData->mError;
  return false;
}
bool QgsGeorefPluginGui::updateGcpDb( QString sCoverageName )
{
  if ( mGcpSrid < 0 )
  {
    mGcpSrid = readProjectSrid();
  }
  if ( !mGcpDbData )
  {
    mGcpDbData = new QgsSpatiaLiteGcpUtils::GcpDbData( mGcpDatabaseFileName, mGcpCoverageName, mGcpSrid, mGcpPointsTableName, mLayerRaster, mSpatialiteGcpEnabled );
  }
  mGcpDbData->mGcpCoverageName = sCoverageName;
  if ( QgsSpatiaLiteGcpUtils::updateGcpDb( mGcpDbData ) )
  {
    mError = mGcpDbData->mError;
    mModifiedRasterFileName = mGcpDbData->mModifiedRasterFileName;
    return true;
  }
  mError = mGcpDbData->mError;
  return false;
}
double QgsGeorefPluginGui::getMetersToMapPoint( double d_GcpMasterArea, int i_srid,  QgsPointXY inputPoint )
{
  QgsSpatiaLiteGcpUtils::GcpDbData *parmsGcpDbData = nullptr;
  if ( i_srid <= 0 )
  {
    QString mGcpAuthid = mIface->mapCanvas()->mapSettings().destinationCrs().authid();
    QStringList sa_authid = mGcpAuthid.split( ":" );
    QString s_srid = "-1";
    if ( sa_authid.length() == 2 )
    {
      s_srid = sa_authid[1];
      i_srid = s_srid.toInt();
    }
    else
    {
      i_srid = mGcpSrid;
    }
    inputPoint = mIface->mapCanvas()->extent().center();
  }
  parmsGcpDbData = new QgsSpatiaLiteGcpUtils::GcpDbData( mGcpDatabaseFileName,  QString::null, i_srid );
  parmsGcpDbData->mGcpMasterArea = d_GcpMasterArea;
  parmsGcpDbData->mInputPoint = inputPoint;
  if ( QgsSpatiaLiteGcpUtils::metersToMapPoint( parmsGcpDbData ) )
  {
    if ( d_GcpMasterArea != parmsGcpDbData->mGcpMasterArea )
    {
      d_GcpMasterArea = parmsGcpDbData->mGcpMasterArea;
      if ( mLayerGcpMaster )
      {
        setCutlineGcpMasterArea( d_GcpMasterArea, mLayerGcpMaster );
      }
    }
  }
  return d_GcpMasterArea;
}
QgsPointXY QgsGeorefPluginGui::getGcpConvert( QString sCoverageName, QgsPointXY inputPoint, bool bToPixel, int i_order, bool bReCompute, int id_gcp )
{
  QgsPointXY convertPoint( 0.0, 0.0 );
  if ( ( ! isGcpEnabled() ) || ( isGcpOff() ) )
  {
    // Spatialite-Gcp-Logic is not available or has been turned off by the User [also LegacyMode==0]
    qDebug() << QString( "-W-> QgsGeorefPluginGui:ugetGcpConvert - SpatialiteGcpEnabled[%1] SpatialiteGcpOff[%2]" ).arg( mSpatialiteGcpEnabled ).arg( mSpatialiteGcpOff );
    return convertPoint;
  }
  if ( !mGcpDbData )
  {
    // mGcpDatabaseFileName
    mGcpDbData = new QgsSpatiaLiteGcpUtils::GcpDbData( mGcpDatabaseFileName, mGcpCoverageName, mGcpSrid, mGcpPointsTableName, mLayerRaster, mSpatialiteGcpEnabled );
  }
  mGcpDbData->mGcpCoverageName = sCoverageName;
  mGcpDbData->mInputPoint = inputPoint;
  mGcpDbData->mToPixel = bToPixel;
  mGcpDbData->mOrder = i_order;
  mGcpDbData->mReCompute = bReCompute;
  mGcpDbData->mIdGcp = id_gcp;
  mError = mGcpDbData->mError = QLatin1String( "" );
  convertPoint = QgsSpatiaLiteGcpUtils::getGcpConvert( mGcpDbData );
  mError = mGcpDbData->mError;
  return convertPoint;
}
bool QgsGeorefPluginGui::updateGcpCompute( QString sCoverageName )
{
  if ( ( ! isGcpEnabled() ) || ( ! mLayerRaster ) )
  {
    // Preconditions
    return false;
  }
  if ( !mGcpDbData )
  {
    mGcpDbData = new QgsSpatiaLiteGcpUtils::GcpDbData( mGcpDatabaseFileName, mGcpCoverageName, mGcpSrid, mGcpPointsTableName, mLayerRaster, mSpatialiteGcpEnabled );
  }
  mGcpDbData->mGcpCoverageName = sCoverageName;
  mError = mGcpDbData->mError = QLatin1String( "" );
  if ( QgsSpatiaLiteGcpUtils::updateGcpCompute( mGcpDbData ) )
  {
    mError = mGcpDbData->mError;
    return true;
  }
  mError = mGcpDbData->mError;
  return false;
}
bool QgsGeorefPluginGui::updateGcpTranslate( QString sCoverageName )
{
  if ( ( ! isGcpEnabled() ) || ( ! mLayerRaster ) )
  {
    // Preconditions
    qDebug() << QString( "-W-> QgsGeorefPluginGui:updateGcpTranslate - SpatialiteGcpEnabled[%1] SpatialiteGcpOff[%2]" ).arg( mSpatialiteGcpEnabled ).arg( mSpatialiteGcpOff );
    return false;
  }
  if ( !mGcpDbData )
  {
    mGcpDbData = new QgsSpatiaLiteGcpUtils::GcpDbData( mGcpDatabaseFileName, mGcpCoverageName, mGcpSrid, mGcpPointsTableName, mLayerRaster, mSpatialiteGcpEnabled );
  }
  mGcpDbData->mGcpCoverageName = sCoverageName;
  mError = mGcpDbData->mError = QLatin1String( "" );
  if ( QgsSpatiaLiteGcpUtils::updateGcpTranslate( mGcpDbData ) )
  {
    mError = mGcpDbData->mError;
    return true;
  }
  mError = mGcpDbData->mError;
  return false;
}
int QgsGeorefPluginGui::createGcpMasterDb( QString  sDatabaseFilename,  int i_srid, bool b_dump )
{
  int return_srid = INT_MIN; // Invalid
  if ( sDatabaseFilename.isNull() )
  {
    // Create default Databas-File name, when not given
    if ( mRasterFilePath.isNull() )
    {
      return return_srid;
    }
    sDatabaseFilename = QString( "%1/%2.db" ).arg( mRasterFilePath ).arg( "gcp_master.db" );
  }
  QgsSpatiaLiteGcpUtils::GcpDbData *parmsGcpDbData = nullptr;
  parmsGcpDbData = new QgsSpatiaLiteGcpUtils::GcpDbData( sDatabaseFilename,  QString::null, i_srid );
  parmsGcpDbData->mDatabaseDump = b_dump;
  if ( QgsSpatiaLiteGcpUtils::createGcpMasterDb( parmsGcpDbData ) )
  {
    if ( parmsGcpDbData->mDatabaseValid )
    {
      return_srid = parmsGcpDbData->mGcpSrid;
    }
    else
    {
      qDebug() << QString( "QgsGeorefPluginGui:ucreateGcpMasterDb -  Database considered invalid-  db[%1] error[%2]" ).arg( parmsGcpDbData->mGcpDatabaseFileName ).arg( parmsGcpDbData->mError );
    }
  }
  else
  {
    qDebug() << QString( "QgsGeorefPluginGui::createGcpMasterDb - creation of Database failed - db[%1] error[%2]" ).arg( parmsGcpDbData->mGcpDatabaseFileName ).arg( parmsGcpDbData->mError );
  }
  parmsGcpDbData = nullptr;
  return return_srid;
}
int QgsGeorefPluginGui::readProjectSrid()
{
  int return_srid = INT_MIN; // Invalid
  if ( mIface )
  {
    QString sGcpAuthid = mIface->mapCanvas()->mapSettings().destinationCrs().authid();
    QString sGcpDescription = mIface->mapCanvas()->mapSettings().destinationCrs().description();
    QStringList sa_authid = sGcpAuthid.split( ":" );
    QString s_srid = "-2";
    if ( sa_authid.length() == 2 )
      s_srid = sa_authid[1];
    return_srid = s_srid.toInt();
  }
  return return_srid;
}
int QgsGeorefPluginGui::createGcpCoverageDb( QString  sDatabaseFilename, int i_srid, bool b_dump )
{
  int return_srid = INT_MIN; // Invalid
  if ( sDatabaseFilename.isNull() )
  {
    // Create default Databas-File name, when not given
    if ( mRasterFilePath.isNull() )
    {
      return return_srid;
    }
    sDatabaseFilename = QString( "%1/%2.db" ).arg( mRasterFilePath ).arg( "gcp_coverage.gcp.db" );
  }
  if ( i_srid < 0 )
  {
    if ( mGcpSrid < 0 )
    {
      mGcpSrid = readProjectSrid();
    }
    i_srid = mGcpSrid;
  }
  if ( i_srid < 0 )
  {
    return return_srid;
  }
  QgsSpatiaLiteGcpUtils::GcpDbData *parmsGcpDbData = nullptr;
  parmsGcpDbData = new QgsSpatiaLiteGcpUtils::GcpDbData( sDatabaseFilename, QString::null, i_srid );
  parmsGcpDbData->mDatabaseDump = b_dump;
  if ( QgsSpatiaLiteGcpUtils::createGcpDb( parmsGcpDbData ) )
  {
    if ( parmsGcpDbData->mDatabaseValid )
    {
      return_srid = parmsGcpDbData->mGcpSrid;
    }
    else
    {
      qDebug() << QString( "QgsGeorefPluginGui:createGcpCoverageDb -  Database considered invalid - db[%1] error[%2]" ).arg( parmsGcpDbData->mGcpDatabaseFileName ).arg( parmsGcpDbData->mError );
    }
  }
  else
  {
    qDebug() << QString( "QgsGeorefPluginGui:createGcpCoverageDb - creation of Database failed - db[%1] error[%2]" ).arg( parmsGcpDbData->mGcpDatabaseFileName ).arg( parmsGcpDbData->mError );
  }
  parmsGcpDbData = nullptr;
  return return_srid;
}
void QgsGeorefPluginGui::fetchGcpMasterPointsExtent( )
{
  if ( ( !mLayerRaster ) || ( !mGcpDbData ) || ( !mIface->mapCanvas() ) || ( !mCanvas ) || ( !mGcpListWidget ) ||
       ( !mLayerGcpMaster ) || ( !mLayerGcpPoints ) || ( !mLayerGcpPoints->isEditable() ) )
  {
    // Preconditions [everything we need to fulfill the task must be active]
    mActionFetchMasterGcpExtent->setChecked( false );
    return;
  }
  if ( !mActionLinkGeorefToQGis->isChecked() )
  {
    // if Georeference is not linked to QGis and
    if ( !mActionLinkQGisToGeoref->isChecked() )
    {
      // ... Qgis is not linked to Georeferencer: then link Georeference to QGis
      mActionLinkQGisToGeoref->setChecked( true );
      // this will insure that only points inside usable area are taken
      extentsChangedGeorefCanvas();
      mIface->mapCanvas()->refresh();
    }
  }
  int i_search_id_gcp = 0;
  double search_distance = DBL_MAX;
  int i_search_result_type = 0; // 0=not found ; 1=exact ; 2: inside area ; 3: outside area
  bool bPointMap = true;
  bool bToPixel = true;
  QString s_searchDataPoint;
  bool b_searchDataPoint;
  bool b_valid = false;
  bool b_permit_update = true;
  // first: Collect the Information and store in an array
  QgsGcpList *master_GcpList = new QgsGcpList();
  QStringList sa_gcp_master_features;
  QgsFeatureIterator mLayerGcpMaster_fit = mLayerGcpMaster->getFeatures( QgsFeatureRequest().setFilterRect( mIface->mapCanvas()->extent() ) );
  QgsFeature fet_master_point;
  int i_count = 0;
  int id_gcp_master = 0;
  QString s_master_name = QLatin1String( "" );
  QString s_master_notes = QLatin1String( "" );
  QString s_master_gcp_text = QLatin1String( "" );
  QgsPointXY pt_point;
  QgsPointXY pt_pixel;
  QString s_master_TextInfo;
  QMap<int, QString> sql_insert;
  // fetchGcpMasterPoint only within 110% of image area
  QgsRectangle image_extent = mLayerRaster->extent();
  double scale_area = 1.10;
  image_extent.scale( scale_area );
  // The User may have reset value, so this value needs to be retrieved beforhand
  mGcpMasterArea = getCutlineGcpMasterArea( mLayerGcpMaster );
  while ( mLayerGcpMaster_fit.nextFeature( fet_master_point ) )
  {
    if ( fet_master_point.geometry() && fet_master_point.geometry().type() == QgsWkbTypes::PointGeometry )
    {
      i_count++;
      b_valid = false;
      pt_point = fet_master_point.geometry().vertexAt( 0 );
      b_searchDataPoint = false;
      if ( mGcpMasterSrid == mGcpSrid )
      {
        // If not the same srid, will never be found without transformation
        b_searchDataPoint = mGcpListWidget->searchDataPoint( pt_point, bPointMap, mGcpMasterArea, &i_search_id_gcp, &search_distance, &i_search_result_type );
      }
      // TODO: Transform Point
      // s_searchDataPoint = QString( "-I-> searchDataPoint[%1] [gcp-exists] result_type[%2] distance[%3] id_gcp[%4]" ).arg( b_searchDataPoint ).arg( i_search_result_type ).arg( search_distance ).arg( i_search_id_gcp );
      if ( !b_searchDataPoint )
      {
        // Only id_gcp was when not found [not an exact match]
        if ( ( search_distance <= ( mGcpMasterArea / 2 ) ) && ( mGcpMasterSrid == mGcpSrid ) )
        {
          // POINT is near the GCP (left/right/top/bottom of point) [update with Master-Gcp Position]
          if ( mGcpListWidget->updateDataPoint( i_search_id_gcp, bPointMap, pt_point ) )
          {
            // Retrieve the UPDATEd QgsGeorefDataPoint
            QgsGeorefDataPoint *dataPoint = mGcpListWidget->getDataPoint( i_search_id_gcp );
            if ( dataPoint )
            {
              dataPoint->setResultType( 2 ); // UPDATE
              s_master_TextInfo  = QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( i_search_id_gcp );
              s_master_TextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( mIdGcpCoverage );
              s_master_TextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( mGcpPointsTableName );
              s_master_TextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( fet_master_point.attribute( QString( "name" ) ).toString() );
              s_master_TextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( fet_master_point.attribute( QString( "notes" ) ).toString() );
              s_master_TextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( getGcpTransformParam( mTransformParam ) );
              id_gcp_master = fet_master_point.attribute( QString( "id_gcp" ) ).toInt();
              s_master_TextInfo += QString( "%1" ).arg( QString( "GcpMaster[%1,%2,%3]" ).arg( id_gcp_master ) ).arg( fet_master_point.attribute( QString( "valid_since" ) ).toString() ).arg( fet_master_point.attribute( QString( "valid_until" ) ).toString() );
              dataPoint->setTextInfo( s_master_TextInfo ); // UPDATE Meta-Data [only name,notes and gcp_text will be used]
              master_GcpList->append( dataPoint );
              s_searchDataPoint = QString( "-I-> searchDataPoint[%1] [gcp-UPDATE] result_type[%2] distance[%3,%5] id_gcp[%4]" ).arg( b_searchDataPoint ).arg( i_search_result_type ).arg( search_distance ).arg( i_search_id_gcp ).arg( mGcpMasterArea );
              qDebug() << s_searchDataPoint;
            }
          }
        }
        else
        {
          if ( !isGcpOff() )
          {
            // Only when the User has NOT turned off the Spatialite the Gcp-Logic and the Spatialite being used been compiled with the Gcp-Logic
            b_valid = true;
            id_gcp_master = fet_master_point.attribute( QString( "id_gcp" ) ).toInt();
            s_master_TextInfo  = QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( id_gcp_master );
            s_master_TextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( mIdGcpCoverage );
            s_master_TextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( mGcpPointsTableName );
            s_master_TextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( fet_master_point.attribute( QString( "name" ) ).toString() );
            s_master_TextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( fet_master_point.attribute( QString( "notes" ) ).toString() );
            s_master_TextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( getGcpTransformParam( mTransformParam ) );
            s_master_TextInfo += QString( "%1" ).arg( QString( "GcpMaster[%1,%2,%3]" ).arg( id_gcp_master ) ).arg( fet_master_point.attribute( QString( "valid_since" ) ).toString() ).arg( fet_master_point.attribute( QString( "valid_until" ) ).toString() );
            // qDebug() << QString( "master_TextInfo[%1] " ).arg( s_master_TextInfo );
            pt_pixel = getGcpConvert( mGcpCoverageName, pt_point, bToPixel, mTransformParam );
            if ( !image_extent.contains( pt_pixel ) )
            {
              // fetchGcpMasterPoint only within 110% of image area [prevent fetching of GcpMasterPoint outside of image area]
              b_valid = false;
            }
            if ( ( pt_pixel.x() == 0.0 ) && ( pt_pixel.x() == 0.0 ) )
            {
              b_valid = false;
            }
            if ( ( i_search_result_type < 2 ) || ( i_search_result_type > 3 ) )
            {
              // 2=possible update ; 3 new point
              b_valid = false;
            }
            if ( ( !b_permit_update ) && ( i_search_result_type == 2 ) )
            {
              // We may, at some time, offer a option to turn off automatic UPDATEs [b_permit_update=false]
              b_valid = false;
            }
            if ( b_valid )
            {
              QgsGeorefDataPoint *dataPoint = new QgsGeorefDataPoint( pt_pixel, pt_point, mGcpSrid, id_gcp_master, id_gcp_master, i_search_result_type, s_master_TextInfo );
              dataPoint->setEnabled( true );
              master_GcpList->append( dataPoint );
              // s_searchDataPoint = QString( "-I-> searchDataPoint[%1] [gcp-INSERT] result_type[%2] distance[%3] id_gcp[%4]" ).arg( b_searchDataPoint ).arg( i_search_result_type ).arg( search_distance ).arg( i_search_id_gcp );
            }
          }
        }
      }
    }
  }
  // second: INSERT or UPDATE what is known to be valid  with the Information stored in the array
  if ( master_GcpList->count() > 0 )
  {
    statusBar()->showMessage( tr( "Importing %1 Gcp-Points from Gcp-Master ..." ).arg( master_GcpList->count() ) );
    int i_imported = bulkGcpPointsInsert( master_GcpList );
    if ( i_imported != INT_MIN )
    {
      updateGeorefTransform();
      mCanvas->refresh();
      mIface->mapCanvas()->refresh();
      statusBar()->showMessage( tr( "Gcp-Points imported from Gcp-Master: %1" ).arg( i_imported ) );
    }
    else
    {
      statusBar()->showMessage( tr( "Importing of %1 Gcp-Points from Gcp-Master failed" ).arg( master_GcpList->count() ) );
    }
  }
  master_GcpList->clearDataPoints();
  master_GcpList = nullptr;
  mActionFetchMasterGcpExtent->setChecked( false );
  return;
}
int QgsGeorefPluginGui::bulkGcpPointsInsert( QgsGcpList *master_GcpList )
{
  int return_count = INT_MIN; // Invalid
  if ( ( !mGcpDbData ) || ( !master_GcpList ) || ( master_GcpList->countDataPoints() <= 0 ) )
  {
    return return_count;
  }
  QMap<int, QString> sql_update;
  QgsSpatiaLiteGcpUtils::GcpDbData *parmsGcpDbData = nullptr;
  parmsGcpDbData = new QgsSpatiaLiteGcpUtils::GcpDbData( mGcpDatabaseFileName, mGcpCoverageName, mGcpSrid, mGcpPointsTableName, mLayerRaster, mSpatialiteGcpEnabled );
  parmsGcpDbData->mOrder = 1; // INSERT
  for ( int i = 0; i < master_GcpList->countDataPoints(); i++ )
  {
    QgsGeorefDataPoint *dataPoint = master_GcpList->at( i );
    switch ( dataPoint->getResultType() )
    {
      case 3: // INSERT
        // qDebug() << QString( "QgsGeorefPluginGui::bulkGcpPointsInsert[%1] INSERT  id_gcp=%3 sql[%2]" ).arg( i ).arg( dataPoint->sqlInsertPointsCoverage() ).arg(dataPoint->id());
        parmsGcpDbData->gcp_coverages.insert( dataPoint->getIdMaster(), dataPoint->sqlInsertPointsCoverage( mGcpMasterSrid ) );
        break;
      case 2: // UPDATE
        // qDebug() << QString( "QgsGeorefPluginGui::bulkGcpPointsInsert[%1] UPDATE id_gcp=%3 sql[%2]" ).arg( i ).arg( dataPoint->sqlInsertPointsCoverage() ).arg( dataPoint->id() );
        sql_update.insert( dataPoint->id(), dataPoint->sqlInsertPointsCoverage( mGcpMasterSrid ) );
        break;
    }
  }
  if ( parmsGcpDbData->gcp_coverages.count() > 0 )
  {
    // during INSERTing, block all events [valid until end of if-statement]
    // - wil hopfully rid ourselves of the 'Warning: QUndoStack::endMacro(): no matching beginMacro()' messages
    const QSignalBlocker blockerPoints( mLayerGcpPoints );
    const QSignalBlocker blockerPixels( mLayerGcpPixels );
    if ( QgsSpatiaLiteGcpUtils::bulkGcpPointsInsert( parmsGcpDbData ) )
    {
      // No errors
      if ( parmsGcpDbData->mDatabaseValid )
      {
        // Also no errors
        return_count = parmsGcpDbData->gcp_coverages.count();
        return_count = 0;
        QMap<int, QString>::iterator itMapCoverages;
        for ( itMapCoverages = parmsGcpDbData->gcp_coverages.begin(); itMapCoverages != parmsGcpDbData->gcp_coverages.end(); ++itMapCoverages )
        {
          // Add the new Gcp-Points Pair to the GcpListWidget
          int id_gcp_master = itMapCoverages.key();
          int id_gcp = itMapCoverages.value().toInt();
          QgsGeorefDataPoint *get_point = master_GcpList->getDataPoint( id_gcp_master );
          if ( get_point )
          {
            // insure that the screen is refreshed afterwards by adding to return_count
            return_count++;
            QgsGeorefDataPoint *dataPoint = new QgsGeorefDataPoint( mCanvas, mIface->mapCanvas(), get_point->pixelCoords(), get_point->mapCoords(), get_point->getSrid(), id_gcp, get_point->isEnabled() );
            dataPoint->setTextInfo( get_point->getTextInfo() );
            mGcpListWidget->addDataPoint( get_point );
          }
        }
      }
      else
      {
        qDebug() << QString( "QgsGeorefPluginGui::bulkGcpPointsInsert -  Database invalid - db[%1] error[%2]" ).arg( parmsGcpDbData->mGcpDatabaseFileName ).arg( parmsGcpDbData->mError );
      }
    }
    else
    {
      qDebug() << QString( "QgsGeorefPluginGui::bulkGcpPointsInsert -  INSERTing of Gcp failed - db[%1] error[%2]" ).arg( parmsGcpDbData->mGcpDatabaseFileName ).arg( parmsGcpDbData->mError );
    }
  }
  if ( sql_update.count() > 0 )
  {
    // during UPDATEs, block all events [valid until end of if-statement]
    const QSignalBlocker blockerPoints( mLayerGcpPoints );
    const QSignalBlocker blockerPixels( mLayerGcpPixels );
    // There may be no INSERTs, so  do the  UPDATEs here
    parmsGcpDbData->mOrder = 2; // UPDATE
    parmsGcpDbData->gcp_coverages = sql_update;
    if ( QgsSpatiaLiteGcpUtils::bulkGcpPointsInsert( parmsGcpDbData ) )
    {
      // insure that the screen is refreshed afterwards by adding to return_count
      return_count += sql_update.count();
      // qDebug() << QString( "QgsGeorefPluginGui::bulkGcpPointsInsert[%1] -I-> UPDATE completed" ).arg( sql_update.count() );
    }
  }
  parmsGcpDbData = nullptr;
  // When return_count > 0, the screens will be refreshed
  return return_count;
}
