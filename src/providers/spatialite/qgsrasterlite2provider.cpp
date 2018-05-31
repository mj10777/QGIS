/***************************************************************************
     qgsrasterlite2provider.cpp
     --------------------------------------
    Date                 : 2017-08-29
    Copyright         : (C) 2017 by Mark Johnson, Berlin Germany
    Email                : mj10777 at googlemail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsrasterlite2provider.h"
#include "qgsdatasourceuri.h"
#include "qgsfeaturestore.h"
#include "qgsgeometry.h"
#include "qgslogger.h"
#include "qgsrasteridentifyresult.h"
#include "qgssettings.h"

#include <cstring>
#if 0
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPainter>
#endif
#include <qmath.h>
//-----------------------------------------------------------------
// RASTERLITE2_KEY, RASTERLITE2_DESCRIPTION
//-----------------------------------------------------------------
//-- Mandatory functions for each Provider
//--> each Provider must be created in an extra library [extra library]
//--> when creating inside a internal library [core library], create as class internal functions
//-----------------------------------------------------------------
const QString RASTERLITE2_KEY = QStringLiteral( "rasterlite2" );
const QString RASTERLITE2_DESCRIPTION = QStringLiteral( "Spatialite RasterLite2 data provider" );
//-----------------------------------------------------------------
// QGISEXTERN isProvider [Required Provider function]
//-----------------------------------------------------------------
// Used to determine if this shared library is a data provider plugin
//-----------------------------------------------------------------
QGISEXTERN bool isProvider()
{
  return true;
}
//-----------------------------------------------------------------
// QGISEXTERN providerKey [Required Provider function]
//-----------------------------------------------------------------
// Used to map the plugin to a data store type
//-----------------------------------------------------------------
QGISEXTERN QString providerKey()
{
  return RASTERLITE2_KEY;
}
//-----------------------------------------------------------------
// QGISEXTERN description [Required Provider function]
//-----------------------------------------------------------------
// Required description function
//-----------------------------------------------------------------
QGISEXTERN QString description()
{
  return RASTERLITE2_DESCRIPTION;
}
//-----------------------------------------------------------------
// QGISEXTERN classFactory [Required Provider function]
//-----------------------------------------------------------------
// Class factory to return a pointer to a newly created
// RasterLite2 object
//-----------------------------------------------------------------

/**
 * Class factory to return a pointer to a newly created
 * RasterLite2Provider object
 */
QGISEXTERN QgsRasterLite2Provider *classFactory( const QString *uri )
{
  return new QgsRasterLite2Provider( *uri );
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::QgsRasterLite2Provider
//-----------------------------------------------------------------
QgsRasterLite2Provider::QgsRasterLite2Provider( const QString &uri )
  : QgsRasterDataProvider( uri )
  , mValid( false )
  , mUriTableName( QString::null )
  , mUriSqlitePath( QString::null )
  , mMetadata( QString() )
  , mDefaultImageBackground( QStringLiteral( "#ffffff" ) )
  , mImageBandsViewExtent( QgsRectangle() )
{
  if ( dataSourceUri().startsWith( QStringLiteral( "RASTERLITE2:" ) ) )
  {
    QStringList sa_list_info = dataSourceUri().split( QgsSpatialiteDbInfo::ParseSeparatorUris );
    if ( sa_list_info.count() == 3 )
    {
      // Note: the gdal notation will be retained, so that only one connection string exists. QgsDataSourceUri will fail.
      if ( sa_list_info.at( 0 ) == QStringLiteral( "RASTERLITE2" ) )
      {
        mUriSqlitePath = sa_list_info.at( 1 );
        mUriTableName = sa_list_info.at( 2 );
      }
    }
  }
  else
  {
    QgsDataSourceUri anUri = QgsDataSourceUri( dataSourceUri() );
    mUriTableName = anUri.table();
    mUriSqlitePath = anUri.database();
  }
  if ( ( !mUriSqlitePath.isEmpty() ) && ( !mUriSqlitePath.isEmpty() ) )
  {
    // trying to open the SQLite DB
    bool bShared = true;
    bool bLoadLayers = true;
    if ( setSqliteHandle( QgsSqliteHandle::openDb( mUriSqlitePath, bShared, mUriTableName, bLoadLayers ) ) )
    {
      mValid = true;
    }
  }
  mTimestamp = QDateTime::currentDateTime();
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::~QgsRasterLite2Provider
//-----------------------------------------------------------------
QgsRasterLite2Provider::~QgsRasterLite2Provider()
{
  // QgsDebugMsgLevel( QStringLiteral( "-I-> QgsRasterLite2Provider:~QgsRasterLite2Provider Bands[%1]" ).arg( mImageBands.count() ), 5 );
  if ( mImageBands.count() > 0 )
  {
    qDeleteAll( mImageBands.begin(), mImageBands.end() );
    mImageBands.clear();
  }
  closeDb();
  invalidateConnections( mUriSqlitePath );
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::name
//-----------------------------------------------------------------
QString QgsRasterLite2Provider::name() const
{
  return RASTERLITE2_KEY;
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::description
//-----------------------------------------------------------------
QString QgsRasterLite2Provider::description() const
{
  return RASTERLITE2_DESCRIPTION;
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::closeDb
//-----------------------------------------------------------------
void QgsRasterLite2Provider::closeDb()
{
  if ( isDbValid() )
  {
    if ( !mSpatialiteDbInfo->checkConnectionNeeded() )
    {
      // Delete only if not being used elsewhere, Connection will be closed
      delete mSpatialiteDbInfo;
    }
  }
  mSpatialiteDbInfo = nullptr;
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::invalidateConnections
//-----------------------------------------------------------------
void QgsRasterLite2Provider::invalidateConnections( const QString &connection )
{
  QgsSpatiaLiteConnPool::instance()->invalidateConnections( connection );
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::capabilities
//-----------------------------------------------------------------
int QgsRasterLite2Provider::capabilities() const
{
  int capability = QgsRasterDataProvider::Identify
                   | QgsRasterDataProvider::IdentifyValue
                   | QgsRasterDataProvider::Size
                   | QgsRasterDataProvider::BuildPyramids
                   | QgsRasterDataProvider::Create
                   | QgsRasterDataProvider::Remove;
  return capability;
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::setSqliteHandle
//-----------------------------------------------------------------
bool QgsRasterLite2Provider::setSqliteHandle( QgsSqliteHandle *qSqliteHandle )
{
  bool bRc = false;
  mQSqliteHandle = qSqliteHandle;
  if ( getQSqliteHandle() )
  {
    mSpatialiteDbInfo = getQSqliteHandle()->getSpatialiteDbInfo();
    if ( getSpatialiteDbInfo() )
    {
      //-----------------------------------------------------------------
      // Setting needed values from Database
      //-----------------------------------------------------------------
      mIsDbValid = getSpatialiteDbInfo()->isDbValid();
      if ( ( isDbValid() ) && ( isDbRasterLite2() ) )
      {
        // Calls 'mod_spatialite' and 'mod_rasterlite2', if not done allready
        if ( dbHasSpatialite() )
        {
          // -- ---------------------------------- --
          // The combination isDbValid() and isDbRasterLite2()
          //  - means that the given Layer is supported by the QgsRasterLite2Provider
          //  --> i.e. not GeoPackage, MBTiles etc.
          //  - RasterLite1 will return isDbGdalOgr() == 1
          //  -> which renders the RasterLayers with gdal
          //  --> so a check must be done later
          //  ---> that the layer is not a RasterLite1-Layer
          // -- ---------------------------------- --
          bool bLoadLayer = true;
          bRc = setDbLayer( getSpatialiteDbInfo()->getQgsSpatialiteDbLayer( mUriTableName, bLoadLayer ) );
        }
        else
        {
          QgsDebugMsgLevel( QStringLiteral( "QgsRasterLite2Provider failed: Spatialite and/or RasterLite2-Drivers are not active Spatialite[%1] RasterLite2[%2] LayerName[%3]" ).arg( isDbSpatialiteActive() ).arg( isDbRasterLite2Active() ).arg( mUriTableName ), 7 );
        }
      }
      else
      {
        if ( isDbValid() )
        {
          if ( !isDbRasterLite2() )
          {
            QgsDebugMsgLevel( QStringLiteral( "QgsRasterLite2Provider failed: Database not supported by QgsRasterLite2Provider LayerName[%1]" ).arg( mUriTableName ), 7 );
          }
        }
        else
        {
          QgsDebugMsgLevel( QStringLiteral( "QgsRasterLite2Provider failed: Database is invalid LayerName[%1]" ).arg( mUriTableName ), 7 );
        }
      }
    }
    else
    {
      QgsDebugMsgLevel( QStringLiteral( "QgsRasterLite2Provider failed QgsSpatialiteDbInfo invalid LayerName[%1] " ).arg( mUriTableName ), 7 );
    }
  }
  else
  {
    QgsDebugMsgLevel( QStringLiteral( "QgsRasterLite2Provider failed invalid QgsSqliteHandle[%1] " ).arg( mUriSqlitePath ), 7 );
  }
  return bRc;
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::setDbLayer
//-----------------------------------------------------------------
bool QgsRasterLite2Provider::setDbLayer( QgsSpatialiteDbLayer *dbLayer )
{
  bool bRc = false;
  if ( ( dbLayer ) && ( dbLayer->isLayerValid() ) && ( dbLayer->isLayerRasterLite2() ) )
  {
    mDbLayer = dbLayer;
    mIsLayerValid = isLayerValid();
    // Start the RasterLite-connection, if not already done. [calls rl2_init}
    if ( isDbRasterLite2Active() )
    {
      //-----------------------------------------------------------------
      // Setting needed values from Layer
      //-----------------------------------------------------------------
      mLayerName = getDbLayer()->getLayerName();
      mCopyright = getDbLayer()->getCopyright();
      mAbstract = getDbLayer()->getAbstract();
      mTitle = getDbLayer()->getTitle();
      mSrid = getDbLayer()->getSrid();
      mWidth = getDbLayer()->getLayerImageWidth();
      mHeight = getDbLayer()->getLayerImageHeight();
      mBandCount = getDbLayer()->getLayerNumBands();
      mXBlockSize = getDbLayer()->getLayerTileWidth();
      mYBlockSize = getDbLayer()->getLayerTileHeight();
      // Set collected Metadata for the Layer
      mCrs = QgsCoordinateReferenceSystem::fromOgcWmsCrs( mDbLayer->getSridEpsg() );
      setLayerBandsInfo( mDbLayer->getLayerGetBandStatistics(), mDbLayer->getLayerBandsHistograms() );
      mDefaultRasterStyle = mDbLayer->getLayerStyleSelected();
      mDefaultImageBackground = mDbLayer->getLayerDefaultImageBackground();
      QgsSettings *userSettings = new QgsSettings();
      // Set the default Image-Backround to the color used for the canvas background
      mDefaultImageBackground = QStringLiteral( "#%1%2%3" )
                                .arg( QString::number( userSettings->value( QStringLiteral( "/qgis/default_canvas_color_red" ), 255 ).toInt(), 16 ) )
                                .arg( QString::number( userSettings->value( QStringLiteral( "/qgis/default_canvas_color_green" ), 255 ).toInt(), 16 ) )
                                .arg( QString::number( userSettings->value( QStringLiteral( "/qgis/default_canvas_color_blue" ), 255 ).toInt(), 16 ) );
      setLayerMetadata( getDbLayer()->getLayerMetadata() );
      createLayerMetadata( 1 );
      bRc = true;
    }
    else
    {
      QgsDebugMsgLevel( QStringLiteral( "QgsRasterLite2Provider failed while loading Layer : Spatialite and/or RasterLite2-Drivers are not active Spatialite[%1] RasterLite2[%2] LayerName[%3]" ).arg( isDbSpatialiteActive() ).arg( isDbRasterLite2Active() ).arg( getLayerName() ), 7 );
    }
  }
  else
  {
    if ( dbLayer )
    {
      QgsDebugMsgLevel( QStringLiteral( " QgsRasterLite2Provider setting Layer failed: isLayerValid[%1] isDbRasterLite2[%2] LayerName[%3]" ).arg( dbLayer->isLayerValid() ).arg( dbLayer->isLayerRasterLite2() ).arg( mUriTableName ), 7 );
    }
  }
  return bRc;
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::setLayerBandsInfo
//-----------------------------------------------------------------
void QgsRasterLite2Provider::setLayerBandsInfo( QStringList layerBandsInfo, QMap<int, QImage> layerBandsHistograms )
{
  mLayerBandsInfo = layerBandsInfo;
  mLayerBandsHistograms = layerBandsHistograms;
  for ( int i = 0; i < mLayerBandsInfo.count(); i++ )
  {
    QStringList sa_list_info = mLayerBandsInfo.at( i ).split( QgsSpatialiteDbInfo::ParseSeparatorGeneral );
    if ( sa_list_info.count() == 8 )
    {
      mLayerBandsNodata.insert( i, sa_list_info.at( 0 ).toDouble() );
      mSrcNoDataValue.append( sa_list_info.at( 0 ).toDouble() );
      // We want to inform the world of the values of these Nodata pixels
      mSrcHasNoDataValue.append( true );
      // Nodata is returned from RasterLite2 when supported by the given format
      mUseSrcNoDataValue.append( true );
      mLayerBandsPixelMin.insert( i, sa_list_info.at( 1 ).toDouble() );
      mLayerBandsPixelMax.insert( i, sa_list_info.at( 2 ).toDouble() );
      mLayerBandsPixelAverage.insert( i, sa_list_info.at( 3 ).toDouble() );
      mLayerBandsPixelVariance.insert( i, sa_list_info.at( 4 ).toDouble() );
      mLayerBandsPixelStandardDeviation.insert( i, sa_list_info.at( 5 ).toDouble() );
      mLayerBandsPixelValidPixelsCount.insert( i, sa_list_info.at( 6 ).toInt() );
    }
  }
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::createLayerMetadata
//-----------------------------------------------------------------
int QgsRasterLite2Provider::createLayerMetadata( int i_debug )
{
  int i_status = 1;
  if ( mMetadata.isEmpty() )
  {
    i_status = 0;
    if ( !isLayerValid() )
    {
      return i_status;
    }
    //-----------------------------------------------------------
    QString htmlTrTdStart = QStringLiteral( "<tr><td class=\"highlight\">" );
    QString htmlTrTdEnd = QStringLiteral( "</td></tr>\n" );
    QString htmlTdEndTdStart = QStringLiteral( "</td><td>" );
    //-----------------------------------------------------------
    QString sMetadataBuild = QString();
    QString sMetadata = QString();
    QString sMetadataBands = QString();
    QString sHtmlMetadata = QString();
    QString sHtmlMetadataBands = QString();
    // TODO: add header information
    // GDAL Driver description
    //  myMetadata += QStringLiteral( "<tr><td class=\"highlight\">" ) + tr( "GDAL Driver Description" ) + QStringLiteral( "</td><td>" ) + QString( GDALGetDescription( GDALGetDatasetDriver( mGdalDataset ) ) ) + QStringLiteral( "</td></tr>\n" );
    // GDAL Driver Metadata
    // myMetadata += QStringLiteral( "<tr><td class=\"highlight\">" ) + tr( "GDAL Driver Metadata" ) + QStringLiteral( "</td><td>" ) + QString( GDALGetMetadataItem( GDALGetDatasetDriver( mGdalDataset ), GDAL_DMD_LONGNAME, nullptr ) ) + QStringLiteral( "</td></tr>\n" );
    //-----------------------------------------------------------
    sMetadataBuild = QStringLiteral( "Layer-Name: %1\n" ).arg( getLayerName() );
    sMetadataBands += sMetadataBuild;
    sHtmlMetadataBands += QString( "%1%2%3" ).arg( htmlTrTdStart ).arg( sMetadataBuild.replace( "\n", "" ).replace( ": ", htmlTdEndTdStart ) ).arg( htmlTrTdEnd );
    sMetadataBuild = QStringLiteral( "Title     : %1\n" ).arg( getTitle() );
    sMetadataBands += sMetadataBuild;
    sHtmlMetadataBands += QString( "%1%2%3" ).arg( htmlTrTdStart ).arg( sMetadataBuild.replace( "\n", "" ).replace( ": ", htmlTdEndTdStart ) ).arg( htmlTrTdEnd );
    sMetadataBuild = QStringLiteral( "Abstract  : %1\n" ).arg( getAbstract() );
    sMetadataBands += sMetadataBuild;
    sHtmlMetadataBands += QString( "%1%2%3" ).arg( htmlTrTdStart ).arg( sMetadataBuild.replace( "\n", "" ).replace( ": ", htmlTdEndTdStart ) ).arg( htmlTrTdEnd );
    sMetadataBuild = QStringLiteral( "Copyright : %1\n" ).arg( getCopyright() );
    sMetadataBands += sMetadataBuild;
    sHtmlMetadataBands += QString( "%1%2%3" ).arg( htmlTrTdStart ).arg( sMetadataBuild.replace( "\n", "" ).replace( ": ", htmlTdEndTdStart ) ).arg( htmlTrTdEnd );
    sMetadataBuild = QStringLiteral( "Srid\t  : %1\n" ).arg( getSridEpsg() );
    sMetadataBands += sMetadataBuild;
    sHtmlMetadataBands += QString( "%1%2%3" ).arg( htmlTrTdStart ).arg( sMetadataBuild.replace( "\n", "" ).replace( ": ", htmlTdEndTdStart ) ).arg( htmlTrTdEnd );
    sMetadataBuild = QStringLiteral( "Extent\t  : %1\n" ).arg( getDbLayer()->getLayerExtentEWKT() );
    sMetadataBands += sMetadataBuild;
    sHtmlMetadataBands += QString( "%1%2%3" ).arg( htmlTrTdStart ).arg( sMetadataBuild.replace( "\n", "" ).replace( ": ", htmlTdEndTdStart ) ).arg( htmlTrTdEnd );
    sMetadataBuild = QStringLiteral( "\tExtent - Width : %1\n" ).arg( QString::number( getDbLayer()->getLayerExtentWidth(), 'f', 7 ) );
    sMetadataBands += sMetadataBuild;
    sHtmlMetadataBands += QString( "%1%2%3" ).arg( htmlTrTdStart ).arg( sMetadataBuild.replace( "\n", "" ).replace( ": ", htmlTdEndTdStart ) ).arg( htmlTrTdEnd );
    sMetadataBuild = QStringLiteral( "\tExtent - Height: %1\n" ).arg( QString::number( getDbLayer()->getLayerExtentHeight(), 'f', 7 ) );
    sMetadataBands += sMetadataBuild;
    sHtmlMetadataBands += QString( "%1%2%3" ).arg( htmlTrTdStart ).arg( sMetadataBuild.replace( "\n", "" ).replace( ": ", htmlTdEndTdStart ) ).arg( htmlTrTdEnd );
    sMetadataBuild = QStringLiteral( "Image-Size: %1x%2\n" ).arg( getDbLayer()->getLayerImageWidth() ).arg( getDbLayer()->getLayerImageHeight() );
    sMetadataBands += sMetadataBuild;
    sHtmlMetadataBands += QString( "%1%2%3" ).arg( htmlTrTdStart ).arg( sMetadataBuild.replace( "\n", "" ).replace( ": ", htmlTdEndTdStart ) ).arg( htmlTrTdEnd );
    sMetadataBuild = QStringLiteral( "\tPixels - Total  : %1\n" ).arg( getDbLayer()->getLayerCountImagePixels() );
    sMetadataBands += sMetadataBuild;
    sHtmlMetadataBands += QString( "%1%2%3" ).arg( htmlTrTdStart ).arg( sMetadataBuild.replace( "\n", "" ).replace( ": ", htmlTdEndTdStart ) ).arg( htmlTrTdEnd );
    sMetadataBuild = QStringLiteral( "\tPixels - Valid  : %1\n" ).arg( getDbLayer()->getLayerCountImageValidPixels() );
    sMetadataBands += sMetadataBuild;
    sHtmlMetadataBands += QString( "%1%2%3" ).arg( htmlTrTdStart ).arg( sMetadataBuild.replace( "\n", "" ).replace( ": ", htmlTdEndTdStart ) ).arg( htmlTrTdEnd );
    sMetadataBuild = QStringLiteral( "\tPixels - NoData : %1\n" ).arg( getDbLayer()->getLayerCountImageNodataPixels() );
    sMetadataBuild = QStringLiteral( "Bands\t  : %1\n" ).arg( getDbLayer()->getLayerNumBands() );
    sMetadataBands += sMetadataBuild;
    sHtmlMetadataBands += QString( "%1%2%3" ).arg( htmlTrTdStart ).arg( sMetadataBuild.replace( "\n", "" ).replace( ": ", htmlTdEndTdStart ) ).arg( htmlTrTdEnd );
    // 3319x2701 = 8964619-7917158=1047461
    //-----------------------------------------------------------
    for ( int i = 0; i < getDbLayer()->getLayerNumBands(); i++ )
    {
      // [generateBandName is 1 based, thus adding '+1'] RL2_GetPixelValue for nodata
      QString sBandInfo = QStringLiteral( " %1\t: nodata[%2] " ).arg( generateBandName( i + 1 ) ).arg( ( int )mLayerBandsNodata.value( i ) );
      // RL2_GetBandStatistics_Min, RL2_GetBandStatistics_Max
      sBandInfo += QStringLiteral( "Min[%1] Max[%2] " ).arg( ( int )mLayerBandsPixelMin.value( i ) ).arg( ( int )mLayerBandsPixelMax.value( i ) );
      // RL2_GetRasterStatistics_ValidPixelsCount, range (max-min)
      sBandInfo += QStringLiteral( "Range[%1] " ).arg( ( int )( mLayerBandsPixelMax.value( i ) - mLayerBandsPixelMin.value( i ) ) );
      // RL2_GetBandStatistics_Avg, RL2_GetBandStatistics_StdDev
      sBandInfo += QStringLiteral( "Average/Mean[%1] StandardDeviation[%2] " ).arg( ( int )mLayerBandsPixelAverage.value( i ) ).arg( ( int )mLayerBandsPixelStandardDeviation.value( i ) );
      // RL2_GetBandStatistics_Var [estimated Variance value as double]
      sBandInfo += QStringLiteral( "Variance[%1] " ).arg( ( int )mLayerBandsPixelVariance.value( i ) );
      sMetadataBuild += QStringLiteral( "\t%1\n" ).arg( sBandInfo );
      sMetadataBands += sMetadataBuild;
      sHtmlMetadataBands += QString( "%1%2%3" ).arg( htmlTrTdStart ).arg( sMetadataBuild.replace( "\n", "" ).replace( ": ", htmlTdEndTdStart ) ).arg( htmlTrTdEnd );
    }
    sMetadata = QStringLiteral( "%1\n" ).arg( sMetadataBands );
    sHtmlMetadata = QStringLiteral( "%1\n" ).arg( sHtmlMetadataBands );
    sMetadataBuild = QStringLiteral( "Data-Type   : %1 [%2]\n" ).arg( getDbLayer()->getLayerRasterSampleType() ).arg( getDbLayer()->getLayerRasterDataTypeString() );
    sMetadata += sMetadataBuild;
    sHtmlMetadata += QString( "%1%2%3" ).arg( htmlTrTdStart ).arg( sMetadataBuild.replace( "\n", "" ).replace( ": ", htmlTdEndTdStart ) ).arg( htmlTrTdEnd );
    sMetadataBuild = QStringLiteral( "Pixel-Type  : %1\n" ).arg( getDbLayer()->getLayerRasterPixelTypeString() );
    sMetadata += sMetadataBuild;
    sHtmlMetadata += QString( "%1%2%3" ).arg( htmlTrTdStart ).arg( sMetadataBuild.replace( "\n", "" ).replace( ": ", htmlTdEndTdStart ) ).arg( htmlTrTdEnd );
    sMetadataBuild = QStringLiteral( "Compression : %1\n" ).arg( getDbLayer()->getLayerRasterCompressionType() );
    sMetadata += sMetadataBuild;
    sHtmlMetadata += QString( "%1%2%3" ).arg( htmlTrTdStart ).arg( sMetadataBuild.replace( "\n", "" ).replace( ": ", htmlTdEndTdStart ) ).arg( htmlTrTdEnd );
    sMetadataBuild = QStringLiteral( "Tile-Size   : %1,%2\n" ).arg( getDbLayer()->getLayerTileWidth() ).arg( getDbLayer()->getLayerTileHeight() );
    sMetadata += sMetadataBuild;
    sHtmlMetadata += QString( "%1%2%3" ).arg( htmlTrTdStart ).arg( sMetadataBuild.replace( "\n", "" ).replace( ": ", htmlTdEndTdStart ) ).arg( htmlTrTdEnd );
    sMetadataBuild = QStringLiteral( "Resolution  : horz=%1 vert=%2\n" ).arg( QString::number( getDbLayer()->getLayerImageResolutionX(), 'f', 7 ) ).arg( QString::number( getDbLayer()->getLayerImageResolutionY(), 'f', 7 ) );
    sMetadata += sMetadataBuild;
    sHtmlMetadata += QString( "%1%2%3" ).arg( htmlTrTdStart ).arg( sMetadataBuild.replace( "\n", "" ).replace( ": ", htmlTdEndTdStart ) ).arg( htmlTrTdEnd );
    sMetadataBuild = QStringLiteral( "User Canvas background    : '%1'\n" ).arg( mDefaultImageBackground );
    sMetadata += sMetadataBuild;
    sHtmlMetadata += QString( "%1%2%3" ).arg( htmlTrTdStart ).arg( sMetadataBuild.replace( "\n", "" ).replace( ": ", htmlTdEndTdStart ) ).arg( htmlTrTdEnd );
    sMetadataBuild = QStringLiteral( "NoData ImageBackground : '%1'\n" ).arg( mDbLayer->getLayerDefaultImageBackground() );
    sMetadata += sMetadataBuild;
    sHtmlMetadata += QString( "%1%2%3" ).arg( htmlTrTdStart ).arg( sMetadataBuild.replace( "\n", "" ).replace( ": ", htmlTdEndTdStart ) ).arg( htmlTrTdEnd );
    sMetadataBuild = QStringLiteral( "DefaultRasterStyle : '%1' of %2 Styles\n" ).arg( mDbLayer->getLayerStyleSelected() ).arg( mDbLayer->getLayerCoverageStylesInfo().count() );
    sMetadata += sMetadataBuild;
    sHtmlMetadata += QString( "%1%2%3" ).arg( htmlTrTdStart ).arg( sMetadataBuild.replace( "\n", "" ).replace( ": ", htmlTdEndTdStart ) ).arg( htmlTrTdEnd );
    //-----------------------------------------------------------
    sHtmlMetadata.replace( "\t", "" );
    //-----------------------------------------------------------
    if ( i_debug > 0 )
    {
      QgsDebugMsgLevel( sMetadata, 5 );
    }
    //-----------------------------------------------------------
    mMetadata = sMetadata;
    mHtmlMetadata = sHtmlMetadata;
    i_status = 1;
  }
  //-----------------------------------------------------------
  return i_status;
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::metadata
//-----------------------------------------------------------------
QString QgsRasterLite2Provider::htmlMetadata()
{
  return mHtmlMetadata;
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::subLayerStyles
//-----------------------------------------------------------------
QStringList QgsRasterLite2Provider::subLayerStyles() const
{
  QStringList styles;
  for ( int i = 0, n = mSubLayers.count(); i < n; ++i )
  {
    styles.append( QString() );
  }
  return styles;
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::setLayerOrder
//-----------------------------------------------------------------
void QgsRasterLite2Provider::setLayerOrder( const QStringList &layers )
{
  QStringList oldSubLayers = mSubLayers;
  QList<bool> oldSubLayerVisibilities = mSubLayerVisibilities;
  mSubLayers.clear();
  mSubLayerVisibilities.clear();
  foreach ( const QString &layer, layers )
  {
    // Search for match
    for ( int i = 0, n = oldSubLayers.count(); i < n; ++i )
    {
      if ( oldSubLayers[i] == layer )
      {
        mSubLayers.append( layer );
        oldSubLayers.removeAt( i );
        mSubLayerVisibilities.append( oldSubLayerVisibilities[i] );
        oldSubLayerVisibilities.removeAt( i );
        break;
      }
    }
  }
  // Add remaining at bottom
  mSubLayers.append( oldSubLayers );
  mSubLayerVisibilities.append( oldSubLayerVisibilities );
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::setSubLayerVisibility
//-----------------------------------------------------------------
void QgsRasterLite2Provider::setSubLayerVisibility( const QString &name, bool vis )
{
  for ( int i = 0, n = mSubLayers.count(); i < n; ++i )
  {
    if ( mSubLayers[i] == name )
    {
      mSubLayerVisibilities[i] = vis;
      break;
    }
  }
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::reloadData
//-----------------------------------------------------------------
void QgsRasterLite2Provider::reloadData()
{
  mCachedImage = QImage();
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::clone
//-----------------------------------------------------------------
QgsRasterInterface *QgsRasterLite2Provider::clone() const
{
  QgsRasterLite2Provider *provider = new QgsRasterLite2Provider( dataSourceUri() );
  provider->copyBaseSettings( *this );
  return provider;
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::identify
//-----------------------------------------------------------------
QgsRasterIdentifyResult QgsRasterLite2Provider::identify( const QgsPointXY &point, QgsRaster::IdentifyFormat format, const QgsRectangle &extent, int width, int height, int dpi )
{
  Q_UNUSED( point );
  Q_UNUSED( format );
  Q_UNUSED( extent );
  Q_UNUSED( width );
  Q_UNUSED( height );
  Q_UNUSED( dpi ); // for now
  QMap<int, QVariant> entries;
#if 0
  // http://resources.arcgis.com/en/help/rest/apiref/identify.html
  QgsDataSourceUri dataSource( dataSourceUri() );
  QUrl queryUrl( dataSource.param( QStringLiteral( "url" ) ) + "/identify" );
  queryUrl.addQueryItem( QStringLiteral( "f" ), QStringLiteral( "json" ) );
  queryUrl.addQueryItem( QStringLiteral( "geometryType" ), QStringLiteral( "esriGeometryPoint" ) );
  queryUrl.addQueryItem( QStringLiteral( "geometry" ), QStringLiteral( "{x: %1, y: %2}" ).arg( point.x(), 0, 'f' ).arg( point.y(), 0, 'f' ) );
//  queryUrl.addQueryItem( "sr", mCrs.postgisSrid() );
  queryUrl.addQueryItem( QStringLiteral( "layers" ), QStringLiteral( "all:%1" ).arg( dataSource.param( QStringLiteral( "layer" ) ) ) );
  queryUrl.addQueryItem( QStringLiteral( "imageDisplay" ), QStringLiteral( "%1,%2,%3" ).arg( width ).arg( height ).arg( dpi ) );
  queryUrl.addQueryItem( QStringLiteral( "mapExtent" ), QStringLiteral( "%1,%2,%3,%4" ).arg( extent.xMinimum(), 0, 'f' ).arg( extent.yMinimum(), 0, 'f' ).arg( extent.xMaximum(), 0, 'f' ).arg( extent.yMaximum(), 0, 'f' ) );
  queryUrl.addQueryItem( QStringLiteral( "tolerance" ), QStringLiteral( "10" ) );
  QVariantList queryResults = QgsArcGisRestUtils::queryServiceJSON( queryUrl, mErrorTitle, mError ).value( QStringLiteral( "results" ) ).toList();

  if ( format == QgsRaster::IdentifyFormatText )
  {
    foreach ( const QVariant &result, queryResults )
    {
      QVariantMap resultMap = result.toMap();
      QVariantMap attributesMap = resultMap[QStringLiteral( "attributes" )].toMap();
      QString valueStr;
      foreach ( const QString &attribute, attributesMap.keys() )
      {
        valueStr += QStringLiteral( "%1 = %2\n" ).arg( attribute, attributesMap[attribute].toString() );
      }
      entries.insert( entries.count(), valueStr );
    }
  }
  else if ( format == QgsRaster::IdentifyFormatFeature )
  {
    foreach ( const QVariant &result, queryResults )
    {
      QVariantMap resultMap = result.toMap();

      QgsFields fields;
      QVariantMap attributesMap = resultMap[QStringLiteral( "attributes" )].toMap();
      QgsAttributes featureAttributes;
      foreach ( const QString &attribute, attributesMap.keys() )
      {
        fields.append( QgsField( attribute, QVariant::String, QStringLiteral( "string" ) ) );
        featureAttributes.append( attributesMap[attribute].toString() );
      }
      QgsCoordinateReferenceSystem crs;
      QgsAbstractGeometry *geometry = QgsArcGisRestUtils::parseEsriGeoJSON( resultMap[QStringLiteral( "geometry" )].toMap(), resultMap[QStringLiteral( "geometryType" )].toString(), false, false, &crs );
      QgsFeature feature( fields );
      feature.setGeometry( QgsGeometry( geometry ) );
      feature.setAttributes( featureAttributes );
      feature.setValid( true );
      QgsFeatureStore store( fields, crs );
      QMap<QString, QVariant> params;
      params[QStringLiteral( "sublayer" )] = resultMap[QStringLiteral( "layerName" )].toString();
      params[QStringLiteral( "featureType" )] = attributesMap[resultMap[QStringLiteral( "displayFieldName" )].toString()].toString();
      store.setParams( params );
      store.addFeature( feature );
      entries.insert( entries.coun t(), qVariantFromValue( QList<QgsFeatureStore>() << store ) );
    }
  }
#endif
  return QgsRasterIdentifyResult( format, entries );
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::readBlock
//-----------------------------------------------------------------
void QgsRasterLite2Provider::readBlock( int bandNo, const QgsRectangle &viewExtent, int width, int height, void *data, QgsRasterBlockFeedback *feedback )
{
  Q_UNUSED( feedback );  // TODO: make use of the feedback object
  if ( isValid() )
  {
    // QgsSpatialiteDbLayer must not be nullptr [possibly deleted]
    int bandNo_ZeroBased = bandNo - 1;
    // viewExtent uses the srid of the given QgsCoordinateReferenceSystem.
    // Note Style: registered style used in the coverage if this remain empty.
    // Otherwise: set to 'default' or to another registered Style, otherwise 'default' will be used
    if ( mImageBandsViewExtent != viewExtent )
    {
      QgsDebugMsgLevel( QStringLiteral( "QgsRasterLite2Provider::readBlock Retrieving Data band[%1,%2]" ).arg( bandNo ).arg( bandNo_ZeroBased ), 7 );
      mImageBandsViewExtent = viewExtent;
      qDeleteAll( mImageBands.begin(), mImageBands.end() );
      mImageBands.clear();
      mImageBands = getDbLayer()->getMapBandsFromRasterLite2( width, height, viewExtent, mDefaultRasterStyle, mDefaultImageBackground, mError );
      QgsDebugMsgLevel( QStringLiteral( "-I-->QgsRasterLite2Provider::readBlock Retrieved Data band[%1,%2]" ).arg( bandNo ).arg( bandNo_ZeroBased ).arg( mImageBands.count() ), 7 );
    }
    else
    {
      QgsDebugMsgLevel( QStringLiteral( "QgsRasterLite2Provider::readBlock Have Data band[%1,%2]" ).arg( bandNo ).arg( bandNo_ZeroBased ), 7 );
    }
    //bandNo=bandNo_ZeroBased;
    QgsDebugMsgLevel( QStringLiteral( "-I--> QgsRasterLite2Provider::readBlock Have Data band[%1,%2] count[%3]" ).arg( bandNo ).arg( bandNo_ZeroBased ).arg( mImageBands.count() ), 7 );
    if ( bandNo_ZeroBased < mImageBands.count() )
    {
      QgsDebugMsgLevel( QStringLiteral( "QgsRasterLite2Provider::readBlock Setting Data band[%1,%2] size[%3]" ).arg( bandNo ).arg( bandNo_ZeroBased ).arg( mImageBands.at( bandNo_ZeroBased )->size() ), 7 );
      char *arrayData = mImageBands.at( bandNo_ZeroBased )->data();
      if ( arrayData )
      {
        std::memcpy( data, arrayData, mImageBands.at( bandNo_ZeroBased )->size() );
      }
    }
#if 0
    if ( !getDbLayer()->getMapImageFromRasterLite2( width, height, viewExtent, mDefaultRasterStyle, mimeType, mDefaultImageBackground, data, mError ) )
    {
      QgsDebugMsgLevel( QStringLiteral( "Retrieving image from RasterLite2 failed: [%1]" ).arg( mError ), 3 );
      return;
    }
#endif
  }
  return;
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::generateBandName
//-----------------------------------------------------------------
QString QgsRasterLite2Provider::generateBandName( int bandNumber ) const
{
  QString sLayerPixelType = QString();
  if ( isValid() )
  {
    sLayerPixelType = getDbLayer()->getLayerRasterPixelTypeString();
    if ( sLayerPixelType == QLatin1String( "RGB" ) )
    {
      switch ( bandNumber )
      {
        case 1:
          sLayerPixelType = QStringLiteral( "%2 %1" ).arg( bandNumber ).arg( QStringLiteral( "Red  " ) );
          break;
        case 2:
          sLayerPixelType = QStringLiteral( "%2 %1" ).arg( bandNumber ).arg( QStringLiteral( "Green" ) );
          break;
        case 3:
          sLayerPixelType = QStringLiteral( "%2 %1" ).arg( bandNumber ).arg( QStringLiteral( "Blue " ) );
          break;
        default:
          sLayerPixelType = QStringLiteral( "Band %1" ).arg( bandNumber );
          break;
      }
    }
    else
    {
      if ( sLayerPixelType == QLatin1String( "MULTIBAND" ) )
      {
        sLayerPixelType = QStringLiteral( "%2-Band %1" ).arg( bandNumber ).arg( sLayerPixelType );
      }
      else
      {
        sLayerPixelType = QStringLiteral( "%1-Band" ).arg( sLayerPixelType );
      }
    }
  }
  return sLayerPixelType;
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::hasStatistics
//-----------------------------------------------------------------
bool  QgsRasterLite2Provider::hasStatistics( int bandNo,
    int stats,
    const QgsRectangle &boundingBox,
    int sampleSize )
{
  // First check if cached in mStatistics
  if ( QgsRasterDataProvider::hasStatistics( bandNo, stats, boundingBox, sampleSize ) )
  {
    return true;
  }
  // The gdal band number (starts at 1) - ours are 0 based, thus '(bandNo-1)'
  QgsRasterBandStats rasterBandStats;
  initStatistics( rasterBandStats, bandNo, stats, boundingBox, sampleSize );
  rasterBandStats.elementCount = mLayerBandsPixelValidPixelsCount.value( ( bandNo - 1 ) ); // RL2_GetRasterStatistics_ValidPixelsCount
  rasterBandStats.minimumValue = mLayerBandsPixelMin.value( ( bandNo - 1 ) ); // RL2_GetBandStatistics_Min
  rasterBandStats.maximumValue = mLayerBandsPixelMax.value( ( bandNo - 1 ) ); // RL2_GetBandStatistics_Max
  rasterBandStats.range = rasterBandStats.maximumValue - rasterBandStats.minimumValue;
  rasterBandStats.mean = mLayerBandsPixelAverage.value( ( bandNo - 1 ) ); // RL2_GetBandStatistics_Avg
  rasterBandStats.stdDev = mLayerBandsPixelStandardDeviation.value( ( bandNo - 1 ) ); // RL2_GetBandStatistics_StdDev
  // The sum of all cells in the band. NO_DATA values are excluded
  rasterBandStats.sum = 0; // info[QStringLiteral( "SUM" )].toDouble();
  // The sum of the squares. Used to calculate standard deviation.
  rasterBandStats.sumOfSquares = 0; // info[QStringLiteral( "SQSUM" )].toDouble();
  // RL2_GetBandStatistics_Var [estimated Variance value as double]
  int supportedStats = QgsRasterBandStats::Min | QgsRasterBandStats::Max
                       | QgsRasterBandStats::Range | QgsRasterBandStats::Mean
                       | QgsRasterBandStats::StdDev;
  rasterBandStats.statsGathered = supportedStats;
  QgsDebugMsgLevel( QStringLiteral( "************ STATS Band %1**************" ).arg( rasterBandStats.bandNumber ), 3 );
  QgsDebugMsgLevel( QStringLiteral( "COUNT %1" ).arg( rasterBandStats.elementCount ), 3 );
  QgsDebugMsgLevel( QStringLiteral( "MIN %1" ).arg( rasterBandStats.minimumValue ), 3 );
  QgsDebugMsgLevel( QStringLiteral( "MAX %1" ).arg( rasterBandStats.maximumValue ), 3 );
  QgsDebugMsgLevel( QStringLiteral( "RANGE %1" ).arg( rasterBandStats.range ), 3 );
  QgsDebugMsgLevel( QStringLiteral( "MEAN %1" ).arg( rasterBandStats.mean ), 3 );
  QgsDebugMsgLevel( QStringLiteral( "STDDEV %1" ).arg( rasterBandStats.stdDev ), 3 );
  if ( rasterBandStats.extent != extent() ||
       ( stats & ( ~supportedStats ) ) )
  {
    QgsDebugMsgLevel( QStringLiteral( "Not supported by RasterLite2." ), 3 );
    return false;
  }
  return true;
}
//-----------------------------------------------------------------
// QgsRasterLite2Provider::bandStatistics
//-----------------------------------------------------------------
QgsRasterBandStats QgsRasterLite2Provider::bandStatistics( int bandNo, int stats, const QgsRectangle &boundingBox, int sampleSize, QgsRasterBlockFeedback * )
{
  QgsRasterBandStats rasterBandStats;
  initStatistics( rasterBandStats, bandNo, stats, boundingBox, sampleSize );
  // First check if cached in mStatistics
  Q_FOREACH ( const QgsRasterBandStats &stats, mStatistics )
  {
    if ( stats.contains( rasterBandStats ) )
    {
      QgsDebugMsgLevel( QStringLiteral( "Using cached statistics." ), 3 );
      return stats;
    }
  }
  // The gdal band number (starts at 1) - ours are 0 based, thus '(bandNo-1)'
  initStatistics( rasterBandStats, bandNo, stats, boundingBox, sampleSize );
  rasterBandStats.elementCount = mLayerBandsPixelValidPixelsCount.value( ( bandNo - 1 ) ); // RL2_GetRasterStatistics_ValidPixelsCount
  rasterBandStats.minimumValue = mLayerBandsPixelMin.value( ( bandNo - 1 ) ); // RL2_GetBandStatistics_Min
  rasterBandStats.maximumValue = mLayerBandsPixelMax.value( ( bandNo - 1 ) ); // RL2_GetBandStatistics_Max
  rasterBandStats.range = rasterBandStats.maximumValue - rasterBandStats.minimumValue;
  rasterBandStats.mean = mLayerBandsPixelAverage.value( ( bandNo - 1 ) ); // RL2_GetBandStatistics_Avg
  rasterBandStats.stdDev = mLayerBandsPixelStandardDeviation.value( ( bandNo - 1 ) ); // RL2_GetBandStatistics_StdDev
  // The sum of all cells in the band. NO_DATA values are excluded
  rasterBandStats.sum = 0; // info[QStringLiteral( "SUM" )].toDouble();
  // The sum of the squares. Used to calculate standard deviation.
  rasterBandStats.sumOfSquares = 0; // info[QStringLiteral( "SQSUM" )].toDouble();
  // RL2_GetBandStatistics_Var [estimated Variance value as double]
  int supportedStats = QgsRasterBandStats::Min | QgsRasterBandStats::Max
                       | QgsRasterBandStats::Range | QgsRasterBandStats::Mean
                       | QgsRasterBandStats::StdDev;
  rasterBandStats.statsGathered = supportedStats;
  QgsDebugMsgLevel( QStringLiteral( "************ STATS Band %1 **************" ).arg( rasterBandStats.bandNumber ), 3 );
  QgsDebugMsgLevel( QStringLiteral( "COUNT %1" ).arg( rasterBandStats.elementCount ), 3 );
  QgsDebugMsgLevel( QStringLiteral( "MIN %1" ).arg( rasterBandStats.minimumValue ), 3 );
  QgsDebugMsgLevel( QStringLiteral( "MAX %1" ).arg( rasterBandStats.maximumValue ), 3 );
  QgsDebugMsgLevel( QStringLiteral( "RANGE %1" ).arg( rasterBandStats.range ), 3 );
  QgsDebugMsgLevel( QStringLiteral( "MEAN %1" ).arg( rasterBandStats.mean ), 3 );
  QgsDebugMsgLevel( QStringLiteral( "STDDEV %1" ).arg( rasterBandStats.stdDev ), 3 );

  mStatistics.append( rasterBandStats );
  return rasterBandStats;
}
