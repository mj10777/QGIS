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
#include <QSettings>
#include <QTextStream>
#include <QPen>
#include <QStringList>
#include <QList>

#include "qgisinterface.h"
#include "qgslegendinterface.h"
#include "qgsapplication.h"

#include "qgscomposerlabel.h"
#include "qgscomposermap.h"
#include "qgscomposertexttable.h"
#include "qgscomposertablecolumn.h"
#include "qgscomposerframe.h"
#include "qgsmapcanvas.h"
#include "qgsmapcoordsdialog.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaprenderer.h"
#include "qgsmaptoolzoom.h"
#include "qgsmaptoolpan.h"

#include "qgsproject.h"
#include "qgsrasterlayer.h"
#include "../../app/qgsrasterlayerproperties.h"
#include "qgsproviderregistry.h"

#include "qgsgeorefdatapoint.h"
#include "qgsgeoreftooladdpoint.h"
#include "qgsgeoreftooldeletepoint.h"
#include "qgsgeoreftoolmovepoint.h"

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
#include "qgsrendererv2.h"
#include "qgssymbollayerv2.h"
#include "qgsmarkersymbollayerv2.h"
#include "qgslinesymbollayerv2.h"
#include "qgsfillsymbollayerv2.h"
#include "qgscategorizedsymbolrendererv2.h"
#include "qgsvectorcolorrampv2.h"
#include "qgslayerdefinition.h"
#include "qgsmaptooladdfeature.h"
#include "qgsmaptoolmovefeature.h"
#include "../../app/nodetool/qgsmaptoolnodetool.h"
#include <sqlite3.h>
#include <spatialite.h>

#include "qgslayertree.h"
#include "qgslayertreemodel.h"
#include "qgsgcpdatabasedialog.h"

QgsGeorefDockWidget::QgsGeorefDockWidget( const QString & title, QWidget * parent, Qt::WindowFlags flags )
    : QgsDockWidget( title, parent, flags )
{
  setObjectName( "GeorefDockWidget" ); // set object name so the position can be saved
}

QgsGeorefPluginGui::QgsGeorefPluginGui( QgisInterface* theQgisInterface, QWidget* parent, Qt::WindowFlags fl )
    : QMainWindow( parent, fl )
    , mMousePrecisionDecimalPlaces( 0 )
    , mTransformParam( QgsGeorefTransform::InvalidTransform )
    , mIface( theQgisInterface )
    , mLayer( nullptr )
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
  b_points_or_spatialite_gcp = false;
  mSpatialite_gcp_off = true;
  mLegacyMode = 1;
  mPointsPolygon = 1;
  layer_gcp_points = nullptr;
  layer_gcp_pixels = nullptr;
  mEvent_gcp_status = 0;
  mEvent_point_status = 0;
  mEvent_pixel_status = 0;
  layer_cutline_points = nullptr;
  layer_cutline_linestrings = nullptr;
  layer_cutline_polygons = nullptr;
  layer_mercator_polygons = nullptr;
  layer_mercator_polygons_transform = nullptr;
  mGcpDbData = nullptr;
  mGcpMasterDatabaseFileName = QString::null;
  mGcpMasterSrid = -1;
  mGcpSrid = -1;
  mGcpDatabaseFileName = QString::null;
  mGcpFileName = QString::null;
  mGcpBaseFileName = QString::null;
  /*
  mAddFeature = nullptr;
  mMoveFeature = nullptr;
  mNodeTool = nullptr;
  */
  // mj10777: when false: old behavior or no spatialite
  b_gdalscript_or_gcp_list = false;
  // Brings best results with spatialite Gps_Compute logic
  mTransformParam = QgsGeorefTransform::ThinPlateSpline;
  mResamplingMethod = QgsImageWarper::Lanczos;
  mCompressionMethod = "DEFLATE";
  bGcpFileName = false;
  bPointsFileName = true;
  mRasterYear = 1; // QDate starts with September 1752, bad for a map of 1748
  mRasterScale = 0;
  mRasterNodata = -1;
  b_gdalscript_or_gcp_list = true; // return only gcp-list instead of gdal commands-script
  group_georeferencer = nullptr;
  group_gcp_cutlines = nullptr;
  group_gcp_points = nullptr;
  group_gtif_raster = nullptr;
  group_cutline_mercator = nullptr;
  group_gcp_master = nullptr;
  layer_gcp_master = nullptr;
  // Circle 2.5 meters around MasterPoint
  mGcpMasterArea = 5.0;
  bAvoidUnneededUpdates = false;
  mLayer_gtif_raster = nullptr;
  mLoadGTifInQgis = 1;
  mGcp_label_type = 1;
  mGcpLabelExpression = "id_gcp"; // "id_gcp||'['||point_x||', '||point_y||']''"
  mCutline_points_layername = "create_cutline_points";
  mCutline_linestrings_layername = "create_cutline_linestrings";
  mCutline_polygons_layername = "create_cutline_polygons";
  mMercator_polygons_layername = "create_mercator_polygons";
  mMercator_polygons_transform_layername = QString( "%1(cutline_polygon)" ).arg( mMercator_polygons_layername );
  mMercator_polygons_layername = QString( "%1(mercator_polygon)" ).arg( mMercator_polygons_layername );
  mGcp_master_layername = "gcp_master";
  QFont appFont = QApplication::font();
  fontPointSize = ( double )appFont.pointSize();

  QSettings s;
  restoreGeometry( s.value( "/Plugin-GeoReferencer/Window/geometry" ).toByteArray() );

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

  connect( mIface, SIGNAL( currentThemeChanged( QString ) ), this, SLOT( updateIconTheme( QString ) ) );

  if ( s.value( "/Plugin-GeoReferencer/Config/ShowDocked" ).toBool() )
  {
    dockThisWindow( true );
  }
  if ( s.value( "/Plugin-GeoReferencer/Config/GDALScript" ).toString() == "Full" )
  {
    b_gdalscript_or_gcp_list = false;
  }
  else
  {
    b_gdalscript_or_gcp_list = true;
  }
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
  QSettings settings;
  settings.setValue( "/Plugin-GeoReferencer/Window/geometry", saveGeometry() );
  //will call clearGCPData() when needed and delete any old rasterlayers
  removeOldLayer();
  delete mToolZoomIn;
  delete mToolZoomOut;
  delete mToolPan;
  delete mToolAddPoint;
  delete mToolNodePoint;
  delete mToolMovePoint;
  delete mToolMovePointQgis;
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
  switch ( checkNeedGCPSave() )
  {
    case QgsGeorefPluginGui::GCPSAVE:
      if ( mLegacyMode == 0 )
      {
        if ( mGcpPointsFileName.isEmpty() )
          saveGCPsDialog();
        else
          saveGCPs();
      }
      writeSettings();
      //will call clearGCPData() when needed and delete any old rasterlayers
      removeOldLayer();
      mRasterFileName = "";
      e->accept();
      return;
    case QgsGeorefPluginGui::GCPSILENTSAVE:
      if ( mLegacyMode == 0 )
      {
        if ( !mGcpPointsFileName.isEmpty() )
          saveGCPs();
      }
      //will call clearGCPData() when needed and delete any old rasterlayers
      removeOldLayer();
      mRasterFileName = "";
      return;
    case QgsGeorefPluginGui::GCPDISCARD:
      writeSettings();
      //will call clearGCPData() when needed and delete any old rasterlayers
      removeOldLayer();
      mRasterFileName = "";
      e->accept();
      return;
    case QgsGeorefPluginGui::GCPCANCEL:
      e->ignore();
      return;
  }
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
    //will call clearGCPData() when needed and delete any old rasterlayers
    removeOldLayer();
  }
}

// -------------------------- private slots -------------------------------- //
// File slots
void QgsGeorefPluginGui::openRasterDialog()
{
  QSettings s;
  QString dir = s.value( "/Plugin-GeoReferencer/rasterdirectory" ).toString();
  if ( dir.isEmpty() )
    dir = '.';

  QString otherFiles = tr( "All other files (*)" );
  QString lastUsedFilter = s.value( "/Plugin-GeoReferencer/lastusedfilter", otherFiles ).toString();

  QString filters = QgsProviderRegistry::instance()->fileRasterFilters();
  filters.prepend( otherFiles + ";;" );
  filters.chop( otherFiles.size() + 2 );
  QString s_RasterFileName = QFileDialog::getOpenFileName( this, tr( "Open raster" ), dir, filters, &lastUsedFilter );
  QFileInfo raster_file( QString( "%1" ).arg( s_RasterFileName ) );
  if ( raster_file.exists() )
  {
    s.setValue( "/Plugin-GeoReferencer/lastusedfilter", lastUsedFilter );
    openRaster( raster_file );
  }
}
void QgsGeorefPluginGui::openRaster( QFileInfo raster_file )
{
  if ( ! raster_file.exists() )
  {
    return;
  }
  //  clearLog();
  switch ( checkNeedGCPSave() )
  {
    case QgsGeorefPluginGui::GCPSAVE:
      if ( mLegacyMode == 0 )
      {
        saveGCPsDialog();
      }
      break;
    case QgsGeorefPluginGui::GCPSILENTSAVE:
      if ( mLegacyMode == 0 )
      {
        if ( !mGcpPointsFileName.isEmpty() )
          saveGCPs();
      }
      break;
    case QgsGeorefPluginGui::GCPDISCARD:
      break;
    case QgsGeorefPluginGui::GCPCANCEL:
      return;
  }

  mRasterFileName = raster_file.canonicalFilePath();
  mModifiedRasterFileName = "";

  QString errMsg;
  if ( !QgsRasterLayer::isValidRasterFileName( mRasterFileName, errMsg ) )
  {
    QString msg = tr( "%1 is not a supported raster data source" ).arg( mRasterFileName );

    if ( !errMsg.isEmpty() )
      msg += '\n' + errMsg;

    QMessageBox::information( this, tr( "Unsupported Data Source" ), msg );
    return;
  }

  QDir parent_dir( raster_file.canonicalPath() );
  QSettings s;
  s.setValue( "/Plugin-GeoReferencer/rasterdirectory", raster_file.path() );

  mGeorefTransform.selectTransformParametrisation( mTransformParam );
  mGeorefTransform.setRasterChangeCoords( mRasterFileName );
  statusBar()->showMessage( tr( "Raster loading: %1" ).arg( mRasterFileName ) );
  setWindowTitle( tr( "Georeferencer - %1" ).arg( raster_file.fileName() ) );
  //will call clearGCPData() when needed and delete any old rasterlayers
  removeOldLayer();
  // mj10777
  mGcpFileName = QString( "%1.gcp.txt" ).arg( mRasterFileName );
  mGcpBaseFileName = raster_file.completeBaseName();
  if (( !mGcpDbData ) || ( mGcpDbData->mGcpDatabaseFileName.isEmpty() ) )
  { // if no Gcp-Database is active, use the Parent-Directory-Name as the default name, otherwise add it to the Active Database [which may be elsewhere]
    mGcpDatabaseFileName = QString( "%1/%2.maps.gcp.db" ).arg( raster_file.canonicalPath() ).arg( parent_dir.dirName() );
  }
  s_gcp_authid = mIface->mapCanvas()->mapSettings().destinationCrs().authid();
  s_gcp_description = mIface->mapCanvas()->mapSettings().destinationCrs().description();
  QStringList sa_authid = s_gcp_authid.split( ":" );
  QString s_srid = "-1";
  if ( sa_authid.length() == 2 )
    s_srid = sa_authid[1];
  mGcpSrid = s_srid.toInt();
  // destinationCrs: authid[EPSG:3068]  srsid[1031]  description[DHDN / Soldner Berlin]
  // s_srid=QString("destinationCrs[%1]: authid[%2]  srsid[%3]  description[%4] ").arg(mGcpSrid).arg(s_gcp_authid).arg(mIface->mapCanvas()->mapRenderer()->destinationCrs().srsid()).arg(s_gcp_description);
  // qDebug()<<s_srid;
  if ( mGcpSrid > 0 )
  { // with srid (of project), activate the buttons and set set output-filename
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
  mCanvas->setExtent( mLayer->extent() );
  mCanvas->refresh();
  mIface->mapCanvas()->refresh();
  // mj10777
  if ( mGcpSrid > 0 )
  { // with srid (of project), activate the buttons and set set output-filename
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
  statusBar()->showMessage( tr( "Raster loaded: %1" ).arg( mRasterFileName ) );
  // qDebug() << "QgsGeorefPluginGui::openRaster[z] - end";
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
      //      mGCPListWidget->setGeorefTransform(&mGeorefTransform);
      //      mTransformParamLabel->setText(tr("Transform: ") + convertTransformEnumToString(mTransformParam));

      mActionLinkGeorefToQGis->setEnabled( false );
      mActionLinkQGisToGeoref->setEnabled( false );
    }
  }
}

bool QgsGeorefPluginGui::getTransformSettings()
{
  QgsTransformSettingsDialog d( mRasterFileName, mModifiedRasterFileName, getGCPList()->countDataPointsEnabled() );
  if ( !d.exec() )
  {
    return false;
  }

  d.getTransformSettings( mTransformParam, mResamplingMethod, mCompressionMethod,
                          mModifiedRasterFileName, mProjection, mPdfOutputMapFile, mPdfOutputFile, mUseZeroForTrans, mLoadInQgis, mUserResX, mUserResY );
  mTransformParamLabel->setText( tr( "Transform: " ) + convertTransformEnumToString( mTransformParam ) );
  mGeorefTransform.selectTransformParametrisation( mTransformParam );
  mGCPListWidget->setGeorefTransform( &mGeorefTransform );
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
  if ( mLayerTreeView->currentLayer() == layer_mercator_polygons )
  { // this needs no gcp's, thus should run before checking for this is done [checkReadyGeoref]
    showGDALScript( QStringList() << generateGDALMercatorScript() );
    return;
  }
  if ( !checkReadyGeoref() )
    return;
  QSettings s;
  if ( s.value( "/Plugin-GeoReferencer/Config/GDALScript" ).toString() == "Full" )
  {
    b_gdalscript_or_gcp_list = false;
  }
  else
  {
    b_gdalscript_or_gcp_list = true;
  }

  switch ( mTransformParam )
  { // for the spatialite/gisGrass logic wit presice POINTs:
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
      if ( b_gdalscript_or_gcp_list )
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
    mSpatialite_gcp_off = !mSpatialite_gcp_off;
    if ( mSpatialite_gcp_off )
      s_text = "Spatialite Gcp-Logic will not be used when creating a new Gcp-Point";
    else
      s_text = "Spatialite Gcp-Logic will be used when creating a new Gcp-Point";
  }
  mActionSpatialiteGcp->setEnabled( true );
  mActionSpatialiteGcp->setText( s_text );
  mActionSpatialiteGcp->setToolTip( s_text );
  mActionSpatialiteGcp->setStatusTip( s_text );
  mActionSpatialiteGcp->setChecked( !mSpatialite_gcp_off );
  s_text = "Fetch Master-Gcp(s) from Map Extent and add as a new Gcp-Point, with Gcp-Master Point correctoons";
  if (( !isGcpEnabled() ) || ( mSpatialite_gcp_off ) )
  {
    s_text = "Gcp-Master Point correctoons from Map Extent, without the Fetching of Master-Gcp(s) ";
  }
  mActionFetchMasterGcpExtent->setText( s_text );
  mActionFetchMasterGcpExtent->setToolTip( s_text );
  mActionFetchMasterGcpExtent->setStatusTip( s_text );
  mActionFetchMasterGcpExtent->setEnabled( true );
  // qDebug() << QString( "-I-> QgsGeorefPluginGui::setSpatialiteGcpUsage[%1,%2,%3]" ).arg(mSpatialite_gcp_off).arg(mActionSpatialiteGcp->isChecked()).arg(s_text);
}
void QgsGeorefPluginGui::setAddPointTool()
{
  if ( mToolAddPoint )
  {
    mCanvas->setMapTool( mToolAddPoint );
  }
  if ( layer_gcp_pixels )
  {
    if ( mLayerTreeView->currentLayer() == layer_gcp_pixels )
    {
      // mIface->layerTreeView()->setCurrentLayer( layer_gcp_points );
    }
  }
  if ( layer_mercator_polygons )
  {
    if ( mLayerTreeView->currentLayer() == layer_mercator_polygons )
    {
      // mIface->layerTreeView()->setCurrentLayer( layer_mercator_polygons_transform );
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
  if ( mToolMovePoint )
  {
    mCanvas->setMapTool( mToolMovePoint );
  }
  if ( layer_mercator_polygons )
  {
    if ( mLayerTreeView->currentLayer() == layer_mercator_polygons )
    {
      // mIface->layerTreeView()->setCurrentLayer( layer_mercator_polygons_transform );
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
  if ( mLayer )
  {
    mCanvas->setExtent( mLayer->extent() );
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
void QgsGeorefPluginGui::addPoint( const QgsPoint& pixelCoords, const QgsPoint& mapCoords, int id_gcp, bool b_PixelMap,  bool enable, bool refreshCanvas )
{
  switch ( mLegacyMode )
  {
    case 0:
    {
      QgsGeorefDataPoint* data_point = new QgsGeorefDataPoint( mCanvas, mIface->mapCanvas(), pixelCoords, mapCoords, mGcpSrid, getGCPList()->countDataPoints(), enable, mLegacyMode );
      mGCPListWidget->addDataPoint( data_point );
      // mGCPListWidget->setGCPList( &mPoints );
      if ( refreshCanvas )
      {
        mCanvas->refresh();
        mIface->mapCanvas()->refresh();
      }
      connect( mCanvas, SIGNAL( extentsChanged() ), data_point, SLOT( updateCoords() ) );
      updateGeorefTransform();
    }
    break;
    case 1:
    {
      QString s_map_points = QString( "pixel[%1] map[%2]" ).arg( pixelCoords.wellKnownText() ).arg( mapCoords.wellKnownText() );
      QString s_Event = QString( "Pixel-Point to Map-Point" );
      QgsVectorLayer *layer_gcp_update = layer_gcp_points;
      QgsPoint added_point_update = mapCoords;
      if ( b_PixelMap )
      {
        s_Event = QString( "Map-Point to Pixel-Point" );
        layer_gcp_update = layer_gcp_pixels;
        added_point_update = pixelCoords;
      }
      s_Event = QString( "%1  id_gcp[%2] %3" ).arg( s_Event ).arg( id_gcp ).arg( s_map_points );
      QgsFeature fet_point_update;
      QgsFeatureRequest update_request = QgsFeatureRequest().setFilterExpression( QString( "id_gcp=%1" ).arg( id_gcp ) );
      if ( layer_gcp_update->getFeatures( update_request ).nextFeature( fet_point_update ) )
      {
        QgsGeometry* geometry_update = QgsGeometry::fromPoint( added_point_update );
        fet_point_update.setGeometry( geometry_update );
        if ( layer_gcp_update->updateFeature( fet_point_update ) )
        {
          if ( saveEditsGcp( layer_gcp_update, true ) )
          {
            s_Event = QString( "%1 - %2" ).arg( s_Event ).arg( "saveEditsGcp was called." );
            QgsGeorefDataPoint* data_point = new QgsGeorefDataPoint( mCanvas, mIface->mapCanvas(), pixelCoords, mapCoords, mGcpSrid, id_gcp, enable, mLegacyMode );
            mGCPListWidget->addDataPoint( data_point );
            connect( mCanvas, SIGNAL( extentsChanged() ), data_point, SLOT( updateCoords() ) );
            updateGeorefTransform();
          }
        }
      }
      qDebug() << QString( "QgsGeorefPluginGui::addPoint[%1] -zz- " ).arg( s_Event );
    }
    break;
  }
  //  if (verbose)
  //    logRequaredGCPs();
}

void QgsGeorefPluginGui::deleteDataPoint( QPoint coords )
{  // TODO: insure that the unique id is used
  if ( mLegacyMode == 0 )
  {
    for ( QgsGCPList::iterator it = getGCPList()->begin(); it != getGCPList()->end(); ++it )
    {
      QgsGeorefDataPoint* data_point = *it;
      if ( /*pt->pixelCoords() == coords ||*/ data_point->contains( coords, true ) ) // first operand for removing from GCP table
      {
        mGCPListWidget->removeDataPoint( data_point->id() );
        mGCPListWidget->updateGCPList();
        mCanvas->refresh();
        break;
      }
    }
    updateGeorefTransform();
  }
}

void QgsGeorefPluginGui::deleteDataPoint( int id_gcp )
{ // TODO: insure that the unique id is used
  if ( mLegacyMode == 0 )
  {
    Q_ASSERT( id_gcp >= 0 );
    mGCPListWidget->removeDataPoint( id_gcp );
    // delete mPoints.takeAt( theGCPIndex );
    // mGCPListWidget->updateGCPList();
    updateGeorefTransform();
  }
}

void QgsGeorefPluginGui::selectPoint( QPoint coords )
{
  if (( mLegacyMode == 0 ) && ( mToolMovePoint ) )
  {
    // Get Map Sender
    bool isMapPlugin = sender() == mToolMovePoint;
    QgsGeorefDataPoint *&mvPoint = isMapPlugin ? mMovingPoint : mMovingPointQgis;

    for ( QgsGCPList::const_iterator it = getGCPList()->constBegin(); it != getGCPList()->constEnd(); ++it )
    {
      if (( *it )->contains( coords, isMapPlugin ) )
      {
        mvPoint = *it;
        break;
      }
    }
  }
}

void QgsGeorefPluginGui::movePoint( QPoint coords )
{
  if (( mLegacyMode == 0 ) && ( mToolMovePoint ) )
  {
    // Get Map Sender
    bool isMapPlugin = sender() == mToolMovePoint;
    QgsGeorefDataPoint *mvPoint = isMapPlugin ? mMovingPoint : mMovingPointQgis;

    if ( mvPoint )
    {
      mvPoint->moveTo( coords, isMapPlugin );
      mGCPListWidget->updateGCPList();
    }
  }
}

void QgsGeorefPluginGui::releasePoint( QPoint coords )
{
  Q_UNUSED( coords );
  if (( mLegacyMode == 0 ) && ( mToolMovePoint ) )
  {
    // Get Map Sender
    if ( sender() == mToolMovePoint )
    {
      mMovingPoint = nullptr;
    }
    else
    {
      mMovingPointQgis = nullptr;
    }
  }
}

void QgsGeorefPluginGui::showCoordDialog( const QgsPoint &pixelCoords )
{
  if ( mLegacyMode == 0 )
  {
    if ( mLayer && !mMapCoordsDialog )
    {
      mMapCoordsDialog = new QgsMapCoordsDialog( mIface->mapCanvas(), pixelCoords, -1, this, mLegacyMode );
      connect( mMapCoordsDialog, SIGNAL( pointAdded( const QgsPoint &, const QgsPoint & , const int &, const bool & ) ), this, SLOT( addPoint( const QgsPoint &, const QgsPoint & , const int &, const bool & ) ) );
      mMapCoordsDialog->show();
    }
  }
}

void QgsGeorefPluginGui::loadGCPsDialog()
{
  if ( mLegacyMode == 0 )
  {
    QString selectedFile = mRasterFileName.isEmpty() ? "" : mRasterFileName + ".points";
    mGcpPointsFileName = QFileDialog::getOpenFileName( this, tr( "Load GCP points" ),
                         selectedFile, tr( "GCP file" ) + " (*.points)" );
    if ( mGcpPointsFileName.isEmpty() )
      return;

    if ( !loadGCPs() )
    {
      mMessageBar->pushMessage( tr( "Invalid GCP file" ), tr( "GCP file could not be read." ), QgsMessageBar::WARNING, messageTimeout() );
    }
    else
    {
      mMessageBar->pushMessage( tr( "GCPs loaded" ), tr( "GCP file successfully loaded." ), QgsMessageBar::INFO, messageTimeout() );
    }
  }
}

void QgsGeorefPluginGui::saveGCPsDialog()
{
  if ( mLegacyMode == 0 )
  {
    if ( getGCPList()->isEmpty() )
    {
      mMessageBar->pushMessage( tr( "No GCP Points" ), tr( "No GCP points are available to save." ), QgsMessageBar::WARNING, messageTimeout() );
      return;
    }

    QString selectedFile = mRasterFileName.isEmpty() ? "" : mRasterFileName + ".points";
    mGcpPointsFileName = QFileDialog::getSaveFileName( this, tr( "Save GCP points" ),
                         selectedFile,
                         tr( "GCP file" ) + " (*.points)" );

    if ( mGcpPointsFileName.isEmpty() )
      return;

    if ( mGcpPointsFileName.right( 7 ) != ".points" )
      mGcpPointsFileName += ".points";
  }
  saveGCPs();
}

// Settings slots
void QgsGeorefPluginGui::showRasterPropertiesDialog()
{
  if ( mLayer )
  {
    mIface->showLayerProperties( mLayer );
  }
  else
  {
    mMessageBar->pushMessage( tr( "Raster Properties" ), tr( "Please load raster to be georeferenced." ), QgsMessageBar::INFO, messageTimeout() );
  }
}
void QgsGeorefPluginGui::showMercatorPolygonPropertiesDialog()
{
  if ( layer_mercator_polygons )
  {
    mIface->showLayerProperties( layer_mercator_polygons );
  }
}
void QgsGeorefPluginGui::zoomMercatorPolygonExtent()
{
  if ( layer_mercator_polygons )
  {
    mCanvas->setExtent( layer_mercator_polygons->extent() );
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
    QSettings s;
    //update dock state
    bool dock = s.value( "/Plugin-GeoReferencer/Config/ShowDocked" ).toBool();
    if ( dock && !mDock )
    {
      dockThisWindow( true );
    }
    else if ( !dock && mDock )
    {
      dockThisWindow( false );
    }
    //update gcp model
    if ( mGCPListWidget )
    {
      mGCPListWidget->updateGCPList();
    }
    //and status bar
    updateTransformParamLabel();
  }
}

// Histogram stretch slots
void QgsGeorefPluginGui::fullHistogramStretch()
{
  mLayer->setContrastEnhancement( QgsContrastEnhancement::StretchToMinimumMaximum );
  mCanvas->refresh();
}

void QgsGeorefPluginGui::localHistogramStretch()
{
  QgsRectangle rectangle = mIface->mapCanvas()->mapSettings().outputExtentToLayerExtent( mLayer, mIface->mapCanvas()->extent() );

  mLayer->setContrastEnhancement( QgsContrastEnhancement::StretchToMinimumMaximum, QgsRaster::ContrastEnhancementMinMax, rectangle );
  mCanvas->refresh();
}

// Info slots
void QgsGeorefPluginGui::contextHelp()
{
  QgsGeorefDescriptionDialog dlg( this );
  dlg.exec();
}

// Comfort slots
void QgsGeorefPluginGui::jumpToGCP( uint theGCPIndex )
{
  if (( int )theGCPIndex >= getGCPList()->countDataPointsEnabled() )
  {
    return;
  }

  // qgsmapcanvas doesn't seem to have a method for recentering the map
  QgsRectangle ext = mCanvas->extent();

  QgsPoint center = ext.center();
  QgsPoint new_center = mGCPListWidget->getDataPoint(( int )theGCPIndex )->pixelCoords();

  QgsPoint diff( new_center.x() - center.x(), new_center.y() - center.y() );
  QgsRectangle new_extent( ext.xMinimum() + diff.x(), ext.yMinimum() + diff.y(),
                           ext.xMaximum() + diff.x(), ext.yMaximum() + diff.y() );
  mCanvas->setExtent( new_extent );
  mCanvas->refresh();
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

// Canvas info slots (copy/pasted from QGIS :) )
void QgsGeorefPluginGui::showMouseCoords( const QgsPoint &p )
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
  bool automatic = QgsProject::instance()->readBoolEntry( "PositionPrecision", "/Automatic" );
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
    dp = QgsProject::instance()->readNumEntry( "PositionPrecision", "/DecimalPlaces" );

  // Keep dp sensible
  if ( dp < 0 )
    dp = 0;

  mMousePrecisionDecimalPlaces = dp;
}

void QgsGeorefPluginGui::extentsChanged()
{
  if ( mAgainAddRaster )
  {
    if ( QFile::exists( mRasterFileName ) )
    {
      addRaster( mRasterFileName );
    }
    else
    {
      mLayer = nullptr;
      mGcpDbData = nullptr;
      mAgainAddRaster = false;
    }
  }
}

// Registry layer QGis
void QgsGeorefPluginGui::layerWillBeRemoved( const QString& theLayerId )
{
  mAgainAddRaster = mLayer && mLayer->id().compare( theLayerId ) == 0;
}

// ------------------------------ private ---------------------------------- //
// Gui
void QgsGeorefPluginGui::createActions()
{
  // File actions
  mActionReset->setIcon( QgsApplication::getThemeIcon( "/mIconClear.svg" ) );
  connect( mActionReset, SIGNAL( triggered() ), this, SLOT( reset() ) );

  mActionOpenRaster->setIcon( getThemeIcon( "/mActionAddRasterLayer.svg" ) );
  connect( mActionOpenRaster, SIGNAL( triggered() ), this, SLOT( openRasterDialog() ) );

  mActionStartGeoref->setIcon( getThemeIcon( "/mActionStartGeoref.png" ) );
  connect( mActionStartGeoref, SIGNAL( triggered() ), this, SLOT( doGeoreference() ) );

  mActionGDALScript->setIcon( getThemeIcon( "/mActionGDALScript.png" ) );
  connect( mActionGDALScript, SIGNAL( triggered() ), this, SLOT( generateGDALScript() ) );

  mActionLoadGCPpoints->setIcon( getThemeIcon( "/mActionLoadGCPpoints.png" ) );
  connect( mActionLoadGCPpoints, SIGNAL( triggered() ), this, SLOT( loadGCPsDialog() ) );

  mActionSaveGCPpoints->setIcon( getThemeIcon( "/mActionSaveGCPpointsAs.png" ) );
  connect( mActionSaveGCPpoints, SIGNAL( triggered() ), this, SLOT( saveGCPsDialog() ) );

  mActionTransformSettings->setIcon( getThemeIcon( "/mActionTransformSettings.png" ) );
  connect( mActionTransformSettings, SIGNAL( triggered() ), this, SLOT( getTransformSettings() ) );

  // Edit actions
  // mActionAddFeature->setIcon( getThemeIcon( "/mActionAddGCPPoint.png" ) );
  mActionAddFeature->setIcon( QgsApplication::getThemeIcon( "/mActionCapturePoint.svg" ) );
  connect( mActionAddFeature, SIGNAL( triggered() ), this, SLOT( setAddPointTool() ) );
  // mActionNodeTool->setIcon( QgsApplication::getThemeIcon( "/mActionNodeTool.svg" ) );
  mActionSpatialiteGcp->setIcon( QgsApplication::getThemeIcon( "/mActionNewSpatiaLiteLayer.svg" ) );
  mActionSpatialiteGcp->setEnabled( false );
  connect( mActionSpatialiteGcp, SIGNAL( triggered() ), this, SLOT( setSpatialiteGcpUsage() ) );

  mActionFetchMasterGcpExtent->setIcon( QgsApplication::getThemeIcon( "/mActionEditNodesItem.svg" ) );
  mActionFetchMasterGcpExtent->setEnabled( false );
  connect( mActionFetchMasterGcpExtent, SIGNAL( triggered() ), this, SLOT( fetchGcpMasterPointsExtent() ) );

  // mActionNodeFeature->setIcon( getThemeIcon( "/mActionDeleteGCPPoint.png" ) );
  mActionNodeFeature->setIcon( QgsApplication::getThemeIcon( "/mActionNodeTool.svg" ) );
  connect( mActionNodeFeature, SIGNAL( triggered() ), this, SLOT( setNodePointTool() ) );

  // mActionMoveFeature->setIcon( getThemeIcon( "/mActionMoveFeature.png" ) );
  mActionMoveFeature->setIcon( getThemeIcon( "/mActionMoveFeaturePoint.svg" ) );
  connect( mActionMoveFeature, SIGNAL( triggered() ), this, SLOT( setMovePointTool() ) );

  // View actions
  mActionPan->setIcon( getThemeIcon( "/mActionPan.svg" ) );
  connect( mActionPan, SIGNAL( triggered() ), this, SLOT( setPanTool() ) );

  mActionZoomIn->setIcon( getThemeIcon( "/mActionZoomIn.svg" ) );
  connect( mActionZoomIn, SIGNAL( triggered() ), this, SLOT( setZoomInTool() ) );

  mActionZoomOut->setIcon( getThemeIcon( "/mActionZoomOut.svg" ) );
  connect( mActionZoomOut, SIGNAL( triggered() ), this, SLOT( setZoomOutTool() ) );

  mActionZoomToLayer->setIcon( getThemeIcon( "/mActionZoomToLayer.svg" ) );
  connect( mActionZoomToLayer, SIGNAL( triggered() ), this, SLOT( zoomToLayerTool() ) );

  mActionZoomLast->setIcon( getThemeIcon( "/mActionZoomLast.svg" ) );
  connect( mActionZoomLast, SIGNAL( triggered() ), this, SLOT( zoomToLast() ) );

  mActionZoomNext->setIcon( getThemeIcon( "/mActionZoomNext.svg" ) );
  connect( mActionZoomNext, SIGNAL( triggered() ), this, SLOT( zoomToNext() ) );

  mActionLinkGeorefToQGis->setIcon( getThemeIcon( "/mActionLinkGeorefToQGis.png" ) );
  connect( mActionLinkGeorefToQGis, SIGNAL( triggered( bool ) ), this, SLOT( linkGeorefToQGis( bool ) ) );

  mActionLinkQGisToGeoref->setIcon( getThemeIcon( "/mActionLinkQGisToGeoref.png" ) );
  connect( mActionLinkQGisToGeoref, SIGNAL( triggered( bool ) ), this, SLOT( linkQGisToGeoref( bool ) ) );

  // Settings actions
  // mActionRasterProperties->setIcon( getThemeIcon( "/mActionRasterProperties.png" ) );
  mActionNodeFeature->setIcon( QgsApplication::getThemeIcon( "/mActionPropertyItem.svg" ) );
  connect( mActionRasterProperties, SIGNAL( triggered() ), this, SLOT( showRasterPropertiesDialog() ) );
  mActionMercatorPolygonProperties->setIcon( QgsApplication::getThemeIcon( "/mActionPropertyItem.svg" ) );
  connect( mActionMercatorPolygonProperties, SIGNAL( triggered() ), this, SLOT( showMercatorPolygonPropertiesDialog() ) );
  mActionMercatorPolygonExtent->setIcon( QgsApplication::getThemeIcon( "/mActionZoomToLayer.svg" ) );
  connect( mActionMercatorPolygonExtent, SIGNAL( triggered() ), this, SLOT( zoomMercatorPolygonExtent() ) );

  mActionGeorefConfig->setIcon( getThemeIcon( "/mActionGeorefConfig.png" ) );
  connect( mActionGeorefConfig, SIGNAL( triggered() ), this, SLOT( showGeorefConfigDialog() ) );

  // GcpDatabase
  mActionLegacyMode->setCheckable( true );
  mActionLegacyMode->setChecked( !( bool )mLegacyMode );
  mActionLegacyMode->setIcon( QgsApplication::getThemeIcon( "/mActionRotatePointSymbols.svg" ) );
  connect( mActionLegacyMode, SIGNAL( triggered() ), this, SLOT( setLegacyMode() ) );

  mActionOpenRaster->setIcon( getThemeIcon( "/mActionAddRasterLayer.svg" ) );

  mActionPointsPolygon->setIcon( getThemeIcon( "/mActionTransformSettings.png" ) );
  mActionPointsPolygon->setCheckable( true );
  mActionPointsPolygon->setChecked(( bool )mPointsPolygon );
  connect( mActionPointsPolygon, SIGNAL( triggered() ), this, SLOT( setPointsPolygon() ) );

  mListGcpCoverages->setIcon( getThemeIcon( "/mActionTransformSettings.png" ) );
  connect( mListGcpCoverages, SIGNAL( triggered() ), this, SLOT( listGcpCoverages() ) );
  mListGcpCoverages->setEnabled( false );

  mOpenGcpCoverages->setIcon( QgsApplication::getThemeIcon( "/mActionAddOgrLayer.svg" ) );
  connect( mOpenGcpCoverages, SIGNAL( triggered() ), this, SLOT( openGcpCoveragesDb() ) );
  mActionOpenGcpMaster->setIcon( QgsApplication::getThemeIcon( "/mActionAddOgrLayer.svg" ) );
  connect( mActionOpenGcpMaster, SIGNAL( triggered() ), this, SLOT( openGcpMasterDb() ) );
  mActionCreateGcpDatabase->setIcon( QgsApplication::getThemeIcon( "/mActionNewSpatiaLiteLayer.svg" ) );
  connect( mActionCreateGcpDatabase, SIGNAL( triggered() ), this, SLOT( createGcpDatabaseDialog() ) );
  mActionSqlDumpGcpCoverage->setIcon( QgsApplication::getThemeIcon( "/mActionNewSpatiaLiteLayer.svg" ) );
  connect( mActionSqlDumpGcpCoverage, SIGNAL( triggered() ), this, SLOT( createSqlDumpGcpCoverage() ) );
  mActionSqlDumpGcpMaster->setIcon( QgsApplication::getThemeIcon( "/mActionNewSpatiaLiteLayer.svg" ) );
  connect( mActionSqlDumpGcpMaster, SIGNAL( triggered() ), this, SLOT( createSqlDumpGcpMaster() ) );

  // Histogram stretch
  mActionLocalHistogramStretch->setIcon( getThemeIcon( "/mActionLocalHistogramStretch.svg" ) );
  connect( mActionLocalHistogramStretch, SIGNAL( triggered() ), this, SLOT( localHistogramStretch() ) );
  mActionLocalHistogramStretch->setEnabled( false );

  mActionFullHistogramStretch->setIcon( getThemeIcon( "/mActionFullHistogramStretch.svg" ) );
  connect( mActionFullHistogramStretch, SIGNAL( triggered() ), this, SLOT( fullHistogramStretch() ) );
  mActionFullHistogramStretch->setEnabled( false );

  // Help actions
  mActionHelp = new QAction( tr( "Help" ), this );
  connect( mActionHelp, SIGNAL( triggered() ), this, SLOT( contextHelp() ) );

  mActionQuit->setIcon( getThemeIcon( "/mActionQuit.png" ) );
  mActionQuit->setShortcuts( QList<QKeySequence>() << QKeySequence( Qt::CTRL + Qt::Key_Q )
                             << QKeySequence( Qt::Key_Escape ) );
  connect( mActionQuit, SIGNAL( triggered() ), this, SLOT( close() ) );
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
  mActionMoveFeature->setCheckable( true );
  mapToolGroup->addAction( mActionMoveFeature );
}

void QgsGeorefPluginGui::createMapCanvas()
{
  // set up the canvas
  mCanvas = new QgsMapCanvas( this->centralWidget(), "georefCanvas" );
  mCanvas->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
  mCanvas->setCanvasColor( Qt::white );
  mCanvas->setMinimumWidth( 400 );
  mCentralLayout->addWidget( mCanvas, 0, 0, 2, 1 );

  // set up map tools
  mToolZoomIn = new QgsMapToolZoom( mCanvas, false /* zoomOut */ );
  mToolZoomIn->setAction( mActionZoomIn );

  mToolZoomOut = new QgsMapToolZoom( mCanvas, true /* zoomOut */ );
  mToolZoomOut->setAction( mActionZoomOut );

  mToolPan = new QgsMapToolPan( mCanvas );
  mToolPan->setAction( mActionPan );

  switch ( mLegacyMode )
  {
    case 0:
      mToolAddPoint = new QgsGeorefToolAddPoint( mCanvas );
      mToolAddPoint->setAction( mActionAddFeature );
      connect( mToolAddPoint, SIGNAL( showCoordDialog( const QgsPoint & ) ), this, SLOT( showCoordDialog( const QgsPoint & ) ) );

      mToolNodePoint = new QgsGeorefToolDeletePoint( mCanvas );
      mToolNodePoint->setAction( mActionNodeFeature );
      connect( mToolNodePoint, SIGNAL( deleteDataPoint( const QPoint & ) ), this, SLOT( deleteDataPoint( const QPoint& ) ) );

      mToolMovePoint = new QgsGeorefToolMovePoint( mCanvas );
      mToolMovePoint->setAction( mActionMoveFeature );
      connect( mToolMovePoint, SIGNAL( pointPressed( const QPoint & ) ), this, SLOT( selectPoint( const QPoint & ) ) );
      connect( mToolMovePoint, SIGNAL( pointMoved( const QPoint & ) ), this, SLOT( movePoint( const QPoint & ) ) );
      connect( mToolMovePoint, SIGNAL( pointReleased( const QPoint & ) ), this, SLOT( releasePoint( const QPoint & ) ) );

      // Point in Qgis Map
      mToolMovePointQgis = new QgsGeorefToolMovePoint( mIface->mapCanvas() );
      mToolMovePointQgis->setAction( mActionMoveFeature );
      connect( mToolMovePointQgis, SIGNAL( pointPressed( const QPoint & ) ), this, SLOT( selectPoint( const QPoint & ) ) );
      connect( mToolMovePointQgis, SIGNAL( pointMoved( const QPoint & ) ), this, SLOT( movePoint( const QPoint & ) ) );
      connect( mToolMovePointQgis, SIGNAL( pointReleased( const QPoint & ) ), this, SLOT( releasePoint( const QPoint & ) ) );
      break;
    default:
      mToolAddPoint = new QgsMapToolAddFeature( mCanvas );
      mToolAddPoint->setAction( mActionAddFeature );
      mToolNodePoint =  new QgsMapToolNodeTool( mCanvas );
      mToolNodePoint->setAction( mActionNodeFeature );
      mActionMoveFeature->setCheckable( false );
      mActionMoveFeature->setEnabled( false );
      // mToolMovePoint = new QgsMapToolMoveFeature( mCanvas );
      // mToolMovePoint->setAction( mActionMoveFeature );
      // mToolMovePointQgis = nullptr;
      break;
  }

  QSettings s;
  double zoomFactor = s.value( "/qgis/zoom_factor", 2 ).toDouble();
  mCanvas->setWheelFactor( zoomFactor );

  mExtentsChangedRecursionGuard = false;

  mGeorefTransform.selectTransformParametrisation( QgsGeorefTransform::Linear );
  // Connect main canvas and georef canvas signals so we are aware if any of the viewports change
  // (used by the map follow mode)
  connect( mCanvas, SIGNAL( extentsChanged() ), this, SLOT( extentsChangedGeorefCanvas() ) );
  connect( mIface->mapCanvas(), SIGNAL( extentsChanged() ), this, SLOT( extentsChangedQGisCanvas() ) );
}

void QgsGeorefPluginGui::createMenus()
{
  // Get platform for menu layout customization (Gnome, Kde, Mac, Win)
  QDialogButtonBox::ButtonLayout layout =
    QDialogButtonBox::ButtonLayout( style()->styleHint( QStyle::SH_DialogButtonLayout, nullptr, this ) );

  mPanelMenu = new QMenu( tr( "Panels" ) );
  mPanelMenu->setObjectName( "mPanelMenu" );
  mPanelMenu->addAction( dockWidgetGCPpoints->toggleViewAction() );
  //  mPanelMenu->addAction(dockWidgetLogView->toggleViewAction());

  mToolbarMenu = new QMenu( tr( "Toolbars" ) );
  mToolbarMenu->setObjectName( "mToolbarMenu" );
  mToolbarMenu->addAction( toolBarFile->toggleViewAction() );
  mToolbarMenu->addAction( toolBarEdit->toggleViewAction() );
  mToolbarMenu->addAction( toolBarView->toggleViewAction() );

  QSettings s;
  int size = s.value( "/IconSize", 32 ).toInt();
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

  mGCPListWidget = new QgsGCPListWidget( this, mLegacyMode );
  mGCPListWidget->setGeorefTransform( &mGeorefTransform );
  dockWidgetGCPpoints->setWidget( mGCPListWidget );

  connect( mGCPListWidget, SIGNAL( jumpToGCP( uint ) ), this, SLOT( jumpToGCP( uint ) ) );
#if 0
  // connect( mGCPListWidget, SIGNAL( replaceDataPoint( QgsGeorefDataPoint*, int ) ), this, SLOT( replaceDataPoint( QgsGeorefDataPoint*, int ) ) );
// Warning: Object::connect: No such signal QgsGCPListWidget::replaceDataPoint( QgsGeorefDataPoint*, int )
#endif
  connect( mGCPListWidget, SIGNAL( deleteDataPoint( int ) ), this, SLOT( deleteDataPoint( int ) ) );
  connect( mGCPListWidget, SIGNAL( pointEnabled( QgsGeorefDataPoint*, int ) ), this, SLOT( updateGeorefTransform() ) );
  initLayerTreeView();
}

void QgsGeorefPluginGui::initLayerTreeView()
{ // Unique names must be used to avoid confusion with the Maon-Application
  mRootLayerTreeGroup = new QgsLayerTreeGroup( tr( "Georeferencer LayersTree" ) );
  mLayerTreeDock = new QgsDockWidget( tr( "Georeferencer Layers Panel" ), this );
  mLayerTreeDock->setObjectName( "Georeferencer Layers" );
  mLayerTreeDock->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
  mLayerTreeView = new QgsLayerTreeView( this );
  mLayerTreeView->setObjectName( "GeoreferencerLayerTreeView" ); // "theLayerTreeView" used to find this canonical instance later
  QgsLayerTreeModel* model = new QgsLayerTreeModel( mRootLayerTreeGroup, this );
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
  QVBoxLayout* vboxLayout = new QVBoxLayout;
  vboxLayout->setMargin( 0 );
  vboxLayout->setContentsMargins( 0, 0, 0, 0 );
  vboxLayout->setSpacing( 0 );
  // vboxLayout->addWidget( toolbar );
  vboxLayout->addWidget( mLayerTreeView );
  QWidget* w = new QWidget;
  w->setLayout( vboxLayout );
  mLayerTreeDock->setWidget( w );
  addDockWidget( Qt::LeftDockWidgetArea, mLayerTreeDock );
  mPanelMenu->addAction( mLayerTreeDock->toggleViewAction() );
  // dockWidgetGCPpoints->setWidget( mLayerTreeView );
  // mLayerTreeView->setMenuProvider( new QgsAppLayerTreeViewMenuProvider( mLayerTreeView, mCanvas ) );
  // setupLayerTreeViewFromSettings();
  // connect( mLayerTreeView, SIGNAL( doubleClicked( QModelIndex ) ), this, SLOT( layerTreeViewDoubleClicked( QModelIndex ) ) );
  connect( mLayerTreeView, SIGNAL( currentLayerChanged( QgsMapLayer* ) ), this, SLOT( activeLayerTreeViewChanged( QgsMapLayer* ) ) );
  // connect( mLayerTreeView->selectionModel(), SIGNAL( currentChanged( QModelIndex, QModelIndex ) ), this, SLOT( updateNewLayerInsertionPoint() ) );
  // connect( QgsProject::instance()->layerTreeRegistryBridge(), SIGNAL( addedLayersToLayerTree( QList<QgsMapLayer*> ) ),this, SLOT( autoSelectAddedLayer( QList<QgsMapLayer*> ) ) );
  connect( mLayerTreeView->layerTreeModel()->rootGroup(), SIGNAL( visibilityChanged( QgsLayerTreeNode*, Qt::CheckState ) ), this, SLOT( visibilityLayerTreeViewChanged( QgsLayerTreeNode*, Qt::CheckState ) ) );
}
void QgsGeorefPluginGui::visibilityLayerTreeViewChanged( QgsLayerTreeNode* layer_treenode, Qt::CheckState check_state )
{
  if ( layer_treenode )
  {
    if ( QgsLayerTree::isLayer( layer_treenode ) )
    {
      QgsMapLayer* map_layer = QgsLayerTree::toLayer( layer_treenode )->layer();
      if ( map_layer )
      {
        bool b_visible = true;
        if ( check_state == Qt::Unchecked )
        {
          b_visible = false;
        }
        for ( int i = 0; i < mMapCanvasLayers.size(); i++ )
        {
          if ( mMapCanvasLayers[i].layer() == map_layer )
          {
            mMapCanvasLayers[i].setVisible( b_visible );
          }
        }
        mCanvas->freeze( true );
        mCanvas->setLayerSet( mMapCanvasLayers );
        mCanvas->freeze( false );
        mCanvas->refresh();
      }
    }
  }
}
void QgsGeorefPluginGui::activeLayerTreeViewChanged( QgsMapLayer* map_layer )
{
  if ( mCanvas )
  {
    QgsVectorLayer *layer_vector = ( QgsVectorLayer* ) map_layer;
    if ( layer_vector )
    {
      if ( layer_vector == layer_gcp_pixels )
      {
        mCanvas->setCurrentLayer( map_layer );
        QString s_text = "Add a Gcp-Point";
        mActionAddFeature->setIcon( QgsApplication::getThemeIcon( "/mActionCapturePoint.svg" ) );
        mActionAddFeature->setText( s_text );
        mActionAddFeature->setToolTip( s_text );
        mActionAddFeature->setStatusTip( s_text );
        mActionNodeFeature->setIcon( QgsApplication::getThemeIcon( "/mActionNodeTool.svg" ) );
        s_text = "Change or Delete selected Gcp-Point";
        mActionNodeFeature->setText( s_text );
        mActionNodeFeature->setToolTip( s_text );
        mActionNodeFeature->setStatusTip( s_text );
        mActionSpatialiteGcp->setEnabled( !mSpatialite_gcp_off );
        mActionFetchMasterGcpExtent->setEnabled( !mSpatialite_gcp_off );
      }
      if ( layer_vector == layer_mercator_polygons )
      {
        mCanvas->setCurrentLayer( map_layer );
        mActionAddFeature->setIcon( QgsApplication::getThemeIcon( "/mActionAddPolygon.svg" ) );
        QString s_text = "Add a Raster-Cutline";
        mActionAddFeature->setText( s_text );
        mActionAddFeature->setToolTip( s_text );
        mActionAddFeature->setStatusTip( s_text );
        mActionNodeFeature->setIcon( QgsApplication::getThemeIcon( "/mActionEditNodesItem.svg" ) );
        s_text = "Change selected Raster-Cutline";
        mActionNodeFeature->setText( s_text );
        mActionNodeFeature->setToolTip( s_text );
        mActionNodeFeature->setStatusTip( s_text );
        mActionFetchMasterGcpExtent->setEnabled( false );
        mActionSpatialiteGcp->setEnabled( false );
      }
    }
  }
}
QLabel* QgsGeorefPluginGui::createBaseLabelStatus()
{
  QLabel* label = new QLabel( statusBar() );
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
  mEPSG->setText( "EPSG:" );
  statusBar()->addPermanentWidget( mEPSG, 0 );
}

void QgsGeorefPluginGui::setupConnections()
{
  connect( mCanvas, SIGNAL( xyCoordinates( QgsPoint ) ), this, SLOT( showMouseCoords( QgsPoint ) ) );
  connect( mCanvas, SIGNAL( scaleChanged( double ) ), this, SLOT( updateMouseCoordinatePrecision() ) );

  // Connect status from ZoomLast/ZoomNext to corresponding action
  connect( mCanvas, SIGNAL( zoomLastStatusChanged( bool ) ), mActionZoomLast, SLOT( setEnabled( bool ) ) );
  connect( mCanvas, SIGNAL( zoomNextStatusChanged( bool ) ), mActionZoomNext, SLOT( setEnabled( bool ) ) );
  // Connect when one Layer is removed - Case where change the Projetct in QGIS
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layerWillBeRemoved( QString ) ), this, SLOT( layerWillBeRemoved( QString ) ) );

  // Connect extents changed - Use for need add again Raster
  connect( mCanvas, SIGNAL( extentsChanged() ), this, SLOT( extentsChanged() ) );

}

void QgsGeorefPluginGui::removeOldLayer()
{
  // delete layer (and don't signal it as it's our private layer)
  if ( mLayer )
  {
    if ( mGCPListWidget->countDataPoints() > 0 )
    {
      clearGCPData();
    }
    QgsMapLayerRegistry::instance()->removeMapLayers(( QStringList() << mLayer->id() ) );
    mLayer = nullptr;
    disconnect( QgsGcpCoverages::instance(), SIGNAL( loadRasterCoverage( int ) ), this, SLOT( loadGcpCoverage( int ) ) );
    mGcpDbData = nullptr;
  }
  mCanvas->refresh();
}

void QgsGeorefPluginGui::updateIconTheme( const QString& theme )
{
  Q_UNUSED( theme );
  // File actions
  mActionOpenRaster->setIcon( getThemeIcon( "/mActionAddRasterLayer.svg" ) );
  mActionStartGeoref->setIcon( getThemeIcon( "/mActionStartGeoref.png" ) );
  mActionGDALScript->setIcon( getThemeIcon( "/mActionGDALScript.png" ) );
  mActionLoadGCPpoints->setIcon( getThemeIcon( "/mActionLoadGCPpoints.png" ) );
  mActionSaveGCPpoints->setIcon( getThemeIcon( "/mActionSaveGCPpointsAs.png" ) );
  mActionTransformSettings->setIcon( getThemeIcon( "/mActionTransformSettings.png" ) );

  // Edit actions
  mActionAddFeature->setIcon( getThemeIcon( "/mActionAddGCPPoint.png" ) );
  mActionNodeFeature->setIcon( getThemeIcon( "/mActionDeleteGCPPoint.png" ) );
  mActionMoveFeature->setIcon( getThemeIcon( "/mActionMoveFeature.png" ) );

  // View actions
  mActionPan->setIcon( getThemeIcon( "/mActionPan.svg" ) );
  mActionZoomIn->setIcon( getThemeIcon( "/mActionZoomIn.svg" ) );
  mActionZoomOut->setIcon( getThemeIcon( "/mActionZoomOut.svg" ) );
  mActionZoomToLayer->setIcon( getThemeIcon( "/mActionZoomToLayer.svg" ) );
  mActionZoomLast->setIcon( getThemeIcon( "/mActionZoomLast.svg" ) );
  mActionZoomNext->setIcon( getThemeIcon( "/mActionZoomNext.svg" ) );
  mActionLinkGeorefToQGis->setIcon( getThemeIcon( "/mActionLinkGeorefToQGis.png" ) );
  mActionLinkQGisToGeoref->setIcon( getThemeIcon( "/mActionLinkQGisToGeoref.png" ) );

  // Settings actions
  mActionRasterProperties->setIcon( getThemeIcon( "/mActionRasterProperties.png" ) );
  mActionGeorefConfig->setIcon( getThemeIcon( "/mActionGeorefConfig.png" ) );

  mActionQuit->setIcon( getThemeIcon( "/mActionQuit.png" ) );
}

// Mapcanvas Plugin
void QgsGeorefPluginGui::addRaster( const QString& file )
{
  mLayer = new QgsRasterLayer( file, mGcpBaseFileName );
  // the Layer should be loaded just before loadGCPs is called - will read metadata from file and set layer
  ( void )loadGCPs();
  // so layer is not added to legend
  QgsMapLayerRegistry::instance()->addMapLayers( QList<QgsMapLayer *>() << mLayer, false, false );

  // add layer to map canvas
  mMapCanvasLayers.clear();
  if ( isGcpDb() )
  {
    switch ( mLegacyMode )
    {
      case 1:
        if ( layer_mercator_polygons )
        {
          mMapCanvasLayers.append( QgsMapCanvasLayer( layer_mercator_polygons, true, false ) );
        }
        if ( layer_gcp_pixels )
        {
          mMapCanvasLayers.append( QgsMapCanvasLayer( layer_gcp_pixels, true, false ) );
        }
        break;
    }
    if ( isGcpEnabled() )
    { // mLayer width and height are needed
      updateGcpCompute( mGcp_coverage_name );
    }
  }
  // map layer must be the last
  mMapCanvasLayers.append( QgsMapCanvasLayer( mLayer, true, false ) );
  mCanvas->setLayerSet( mMapCanvasLayers );
#if 0
  QStringList list_layers;
  for ( int i_list = 0;i_list < mCanvas->mapSettings().layers().size();i_list++ )
  { // the first is on top of the second, map layer must be the last
    list_layers.append( mCanvas->mapSettings().layers().at( i_list ) );
    qDebug() << QString( "QgsGeorefPluginGui::addRaster[%1,%2] [%3]" ).arg( i_list ).arg( mCanvas->mapSettings().layers().at( i_list ) );
  }
#endif

  mAgainAddRaster = false;

  mActionLocalHistogramStretch->setEnabled( true );
  mActionFullHistogramStretch->setEnabled( true );

  // Status Bar
  if ( mGeorefTransform.hasCrs() )
  {
    QString authid = mLayer->crs().authid();
    mEPSG->setText( authid );
    mEPSG->setToolTip( mLayer->crs().toProj4() );
  }
  else
  {
    mEPSG->setText( tr( "None" ) );
    mEPSG->setToolTip( tr( "Coordinate of image(column/line)" ) );
  }
}

// Settings
void QgsGeorefPluginGui::readSettings()
{
  QSettings s;
  QRect georefRect = QApplication::desktop()->screenGeometry( mIface->mainWindow() );
  resize( s.value( "/Plugin-GeoReferencer/size", QSize( georefRect.width() / 2 + georefRect.width() / 5,
                   mIface->mainWindow()->height() ) ).toSize() );
  move( s.value( "/Plugin-GeoReferencer/pos", QPoint( parentWidget()->width() / 2 - width() / 2, 0 ) ).toPoint() );
  restoreState( s.value( "/Plugin-GeoReferencer/uistate" ).toByteArray() );

  // warp options
  mResamplingMethod = ( QgsImageWarper::ResamplingMethod )s.value( "/Plugin-GeoReferencer/resamplingmethod",
                      QgsImageWarper::NearestNeighbour ).toInt();
  mCompressionMethod = s.value( "/Plugin-GeoReferencer/compressionmethod", "NONE" ).toString();
  mUseZeroForTrans = s.value( "/Plugin-GeoReferencer/usezerofortrans", false ).toBool();
}

void QgsGeorefPluginGui::writeSettings()
{
  QSettings s;
  s.setValue( "/Plugin-GeoReferencer/pos", pos() );
  s.setValue( "/Plugin-GeoReferencer/size", size() );
  s.setValue( "/Plugin-GeoReferencer/uistate", saveState() );

  // warp options
  s.setValue( "/Plugin-GeoReferencer/transformparam", mTransformParam );
  s.setValue( "/Plugin-GeoReferencer/resamplingmethod", mResamplingMethod );
  s.setValue( "/Plugin-GeoReferencer/compressionmethod", mCompressionMethod );
  s.setValue( "/Plugin-GeoReferencer/usezerofortrans", mUseZeroForTrans );
}

// GCP points
bool QgsGeorefPluginGui::loadGCPs( /*bool verbose*/ )
{
  clearGCPData();
  QFile points_file( mGcpPointsFileName );
  // will create needed gcp_database, if it does not exist
  // - also import an existing '.point' file
  // check if the used spatialite has GCP-enabled
  bool b_database_exists = createGcpDb();
  if ( b_database_exists )
  {  // Note: if (mLegacyMode == 0), b_database_exists will be false
    if ((( mGcpDatabaseFileName.isNull() ) || ( mGcpDatabaseFileName.isEmpty() ) ) &&
        ( !mGcpDbData->mGcpDatabaseFileName.isEmpty() ) )
    { // Should never happen, but lets make sure
      mGcpDatabaseFileName = mGcpDbData->mGcpDatabaseFileName;
    }
    mGCPListWidget->setGcpDbData( mGcpDbData, true );
    mGcp_points_layername = QString( "%1(gcp_point)" ).arg( mGcp_points_table_name );
    QString s_gcp_points_layer = QString( "%1|layername=%2" ).arg( mGcpDatabaseFileName ).arg( mGcp_points_layername );
    layer_gcp_points = new QgsVectorLayer( s_gcp_points_layer, mGcp_points_layername,  "ogr" );
    if ( setGcpLayerSettings( layer_gcp_points ) )
    { // pointer and isValid == true ; default Label settings done
      mGcp_pixel_layername = QString( "%1(gcp_pixel)" ).arg( mGcp_points_table_name );
      QString s_gcp_pixel_layer = QString( "%1|layername=%2" ).arg( mGcpDatabaseFileName ).arg( mGcp_pixel_layername );
      layer_gcp_pixels = new QgsVectorLayer( s_gcp_pixel_layer, mGcp_pixel_layername, "ogr" );
      if ( setGcpLayerSettings( layer_gcp_pixels ) )
      {
        // add pixels-layer to georeferencer-map canvas and add points-layer to georeferencergroup in application QgsLayerTree
        switch ( mLegacyMode )
        {
          case 1:
            QgsMapLayerRegistry::instance()->addMapLayers( QList<QgsMapLayer *>()  << layer_gcp_pixels, false );
            break;
        }
        if ( !layer_cutline_points )
        {
          // general geometry type as helpers during georeferencing, but also as cutline
          QString s_cutline_points_layer = QString( "%1|layername=%2" ).arg( mGcpDatabaseFileName ).arg( mCutline_points_layername );
          layer_cutline_points = new QgsVectorLayer( s_cutline_points_layer, mCutline_points_layername,  "ogr" );
          QString s_cutline_linestrings_layer = QString( "%1|layername=%2" ).arg( mGcpDatabaseFileName ).arg( mCutline_linestrings_layername );
          layer_cutline_linestrings = new QgsVectorLayer( s_cutline_linestrings_layer, mCutline_linestrings_layername,  "ogr" );
          QString s_cutline_polygons_layer = QString( "%1|layername=%2" ).arg( mGcpDatabaseFileName ).arg( mCutline_polygons_layername );
          layer_cutline_polygons = new QgsVectorLayer( s_cutline_polygons_layer, mCutline_polygons_layername,  "ogr" );
          QString mGcp_pixel_layername = QString( "%1(gcp_pixel)" ).arg( mGcp_points_table_name );
          QString s_mercator_polygons_layer = QString( "%1|layername=%2" ).arg( mGcpDatabaseFileName ).arg( mMercator_polygons_layername );
          layer_mercator_polygons = new QgsVectorLayer( s_mercator_polygons_layer, mMercator_polygons_layername,  "ogr" );
          QString s_mercator_polygons_layer_transform = QString( "%1|layername=%2" ).arg( mGcpDatabaseFileName ).arg( mMercator_polygons_transform_layername );
          layer_mercator_polygons_transform = new QgsVectorLayer( s_mercator_polygons_layer_transform, mMercator_polygons_transform_layername,  "ogr" );
        }
        QList<QgsMapLayer *> layers_points_cutlines;
        QList<QgsMapLayer *> layers_points_mercator;
        layers_points_mercator << layer_gcp_pixels;
        layers_points_cutlines << layer_gcp_points;
        // layerTreeRoot=QgsLayerTreeGroup
        group_georeferencer = QgsProject::instance()->layerTreeRoot()->insertGroup( 0, QString( "Georeferencer" ) );
        int i_group_position = 0;
        group_gcp_points = group_georeferencer->insertGroup( i_group_position++, mGcp_coverage_name );
        if ( setCutlineLayerSettings( layer_cutline_points ) )
        { // Add only valid layers
          layers_points_cutlines << layer_cutline_points;
          if ( setCutlineLayerSettings( layer_cutline_linestrings ) )
          { // Add only valid layers
            layers_points_cutlines << layer_cutline_linestrings;
            if ( setCutlineLayerSettings( layer_cutline_polygons ) )
            { // Add only valid layers
              layers_points_cutlines << layer_cutline_polygons;
              group_gcp_cutlines = group_georeferencer->insertGroup( i_group_position++,  "Gcp_Cutlines" );
              if ( setCutlineLayerSettings( layer_mercator_polygons ) )
              { // Add only valid layers
                // layers_points_cutlines << layer_mercator_polygons;
                layers_points_mercator << layer_mercator_polygons;
                loadGcpMaster(); // Will be added if exists
                if ( group_gcp_master )
                { // Insure that 'group_gcp_master' comes before the 'group_gtif_rasters'
                  i_group_position++;
                }
                if ( setCutlineLayerSettings( layer_mercator_polygons_transform ) )
                { // Add only valid layers
                  layers_points_cutlines << layer_mercator_polygons_transform;
                }
              }
            }
          }
        }
        // Note: since they are being placed in our groups, 'false' must be used, avoiding the automatic inserting into the layerTreeRoot.
        QgsMapLayerRegistry::instance()->addMapLayers( layers_points_cutlines, false );
        group_gcp_points->insertLayer( 0, layer_gcp_points );
        if ( group_gcp_cutlines )
        { // must be added AFTER being registered !
          group_gcp_cutlines->insertLayer( 0, layer_cutline_points );
          // ?? No idea why ',( QgsGeorefPluginGui* )this' is needed. Otherwise crash ??
          connect( layer_cutline_points, SIGNAL( featureAdded( QgsFeatureId ) ), ( QgsGeorefPluginGui* )this, SLOT( featureAdded_cutline( QgsFeatureId ) ) );
          connect( layer_cutline_points, SIGNAL( geometryChanged( QgsFeatureId, QgsGeometry& ) ), ( QgsGeorefPluginGui* )this, SLOT( geometryChanged_cutline( QgsFeatureId, QgsGeometry& ) ) );
          layer_cutline_points->startEditing();
          group_gcp_cutlines->insertLayer( 1, layer_cutline_linestrings );
          connect( layer_cutline_linestrings, SIGNAL( featureAdded( QgsFeatureId ) ), ( QgsGeorefPluginGui* )this, SLOT( featureAdded_cutline( QgsFeatureId ) ) );
          connect( layer_cutline_linestrings, SIGNAL( geometryChanged( QgsFeatureId, QgsGeometry& ) ), ( QgsGeorefPluginGui* )this, SLOT( geometryChanged_cutline( QgsFeatureId, QgsGeometry& ) ) );
          layer_cutline_linestrings->startEditing();
          group_gcp_cutlines->insertLayer( 2, layer_cutline_polygons );
          connect( layer_cutline_polygons, SIGNAL( featureAdded( QgsFeatureId ) ), ( QgsGeorefPluginGui* )this, SLOT( featureAdded_cutline( QgsFeatureId ) ) );
          connect( layer_cutline_polygons, SIGNAL( geometryChanged( QgsFeatureId, QgsGeometry& ) ), ( QgsGeorefPluginGui* )this, SLOT( geometryChanged_cutline( QgsFeatureId, QgsGeometry& ) ) );
          layer_cutline_polygons->startEditing();
          if ( layer_mercator_polygons_transform )
          { // Will be shown in the QGIS Application
            group_gcp_cutlines->insertLayer( 3, layer_mercator_polygons_transform );
#if 0
            connect( layer_mercator_polygons_transform, SIGNAL( featureAdded( QgsFeatureId ) ), ( QgsGeorefPluginGui* )this, SLOT( featureAdded_cutline( QgsFeatureId ) ) );
            connect( layer_mercator_polygons_transform, SIGNAL( geometryChanged( QgsFeatureId, QgsGeometry& ) ), ( QgsGeorefPluginGui* )this, SLOT( geometryChanged_cutline( QgsFeatureId, QgsGeometry& ) ) );
#endif
            layer_mercator_polygons_transform->startEditing();
          }
          if ( layer_mercator_polygons )
          { // Will be shown in the Georeferencer
            QgsMapLayerRegistry::instance()->addMapLayers( QList<QgsMapLayer *>() << layers_points_mercator, false );
            group_cutline_mercator = mRootLayerTreeGroup->insertGroup( 0,  "Gcp_Mercator" );
            group_cutline_mercator->insertLayer( 0, layer_mercator_polygons );
            group_cutline_mercator->insertLayer( 1, layer_gcp_pixels );
            connect( layer_mercator_polygons, SIGNAL( featureAdded( QgsFeatureId ) ), ( QgsGeorefPluginGui* )this, SLOT( featureAdded_cutline( QgsFeatureId ) ) );
            connect( layer_mercator_polygons, SIGNAL( geometryChanged( QgsFeatureId, QgsGeometry& ) ), ( QgsGeorefPluginGui* )this, SLOT( geometryChanged_cutline( QgsFeatureId, QgsGeometry& ) ) );
            layer_mercator_polygons->startEditing();
          }
        }
        group_gtif_rasters = group_georeferencer->insertGroup( i_group_position++,  "Gcp_GTif-Rasters" );
        if ( group_gtif_rasters )
        { // If configured to do so and if file exists, will be loaded
          QFileInfo gtif_file( QString( "%1/%2" ).arg( mRasterFilePath ).arg( mModifiedRasterFileName ) );
          loadGTifInQgis( gtif_file.canonicalFilePath() );
        }
#if 0
        exportLayerDefinition( group_gcp_cutlines );
#endif
        // Shown in the Georeferencer
        layer_gcp_pixels->startEditing();
        connect( layer_gcp_pixels, SIGNAL( featureAdded( QgsFeatureId ) ), ( QgsGeorefPluginGui* )this, SLOT( featureAdded_gcp( QgsFeatureId ) ) );
        connect( layer_gcp_pixels, SIGNAL( geometryChanged( QgsFeatureId, QgsGeometry& ) ), ( QgsGeorefPluginGui* )this, SLOT( geometryChanged_gcp( QgsFeatureId, QgsGeometry& ) ) );
        // Shown in the QGIS Application
        layer_gcp_points->startEditing();
        connect( layer_gcp_points, SIGNAL( featureAdded( QgsFeatureId ) ), ( QgsGeorefPluginGui* )this, SLOT( featureAdded_gcp( QgsFeatureId ) ) );
        connect( layer_gcp_points, SIGNAL( geometryChanged( QgsFeatureId, QgsGeometry& ) ), ( QgsGeorefPluginGui* )this, SLOT( geometryChanged_gcp( QgsFeatureId, QgsGeometry& ) ) );
        QgsAttributeList lst_needed_fields;
        lst_needed_fields.append( 0 ); // id_gcp
        lst_needed_fields.append( 10 ); // gcp_enable
        QgsFeatureIterator fit_pixels = layer_gcp_pixels->getFeatures( QgsFeatureRequest().setSubsetOfAttributes( lst_needed_fields ) );
        QgsFeature fet_pixel;
        QgsFeatureIterator fit_points = layer_gcp_points->getFeatures( QgsFeatureRequest().setSubsetOfAttributes( QgsAttributeList() ) );
        QgsFeature fet_point;
        int i_count = 0;
        while ( fit_pixels.nextFeature( fet_pixel ) )
        {
          if ( fet_pixel .geometry() && fet_pixel.geometry()->type() == QGis::Point )
          {
            fit_points.nextFeature( fet_point );
            if ( fet_point .geometry() && fet_point.geometry()->type() == QGis::Point )
            {
              int id_gcp = fet_pixel.attribute( QString( "id_gcp" ) ).toInt();
              int enable = fet_pixel.attribute( QString( "gcp_enable" ) ).toInt();
              QgsPoint pt_pixel = fet_pixel.geometry()->vertexAt( 0 );
              QgsPoint pt_point = fet_point.geometry()->vertexAt( 0 );
              i_count++;
              // id_gcp=fet_point.id();
              if ( fet_pixel.id() != fet_point.id() )
                qDebug() << QString( "QgsGeorefPluginGui::loadGCPs[%1] pixel.id[%2] fet_point.id[%3]" ).arg( id_gcp ).arg( fet_pixel.id() ).arg( fet_point.id() );
              QgsGeorefDataPoint* data_point = new QgsGeorefDataPoint( mCanvas, mIface->mapCanvas(), pt_pixel, pt_point, mGcpSrid, id_gcp, enable, mLegacyMode );
              mGCPListWidget->addDataPoint( data_point );
              connect( mCanvas, SIGNAL( extentsChanged() ), data_point, SLOT( updateCoords() ) );
            }
          }
        }
      }
      else
      {
        layer_gcp_points = nullptr;
      }
    }
  }
  if ( mLegacyMode == 0 )
  {
    if ( points_file.open( QIODevice::ReadOnly ) )
    {
      qDebug() << QString( "-I-> QgsSpatiaLiteProviderGcpUtils::loadGCPs[%1]  loading point_file  (b_load_points_file = true)" ).arg( mGcp_pixel_layername );
      clearGCPData();
      QTextStream stream_points( &points_file );
      QString line = stream_points.readLine();
      int id_gcp = 1;
      while ( !stream_points.atEnd() )
      {
        line = stream_points.readLine();
        QStringList ls;
        if ( line.contains( QRegExp( "," ) ) ) // in previous format "\t" is delimiter of points in new - ","
        {
          // points from new georeferencer
          ls = line.split( "," );
        }
        else
        {
          // points from prev georeferencer
          ls = line.split( "\t" );
        }
        QgsPoint mapCoords( ls.at( 0 ).toDouble(), ls.at( 1 ).toDouble() ); // map x,y
        QgsPoint pixelCoords( ls.at( 2 ).toDouble(), ls.at( 3 ).toDouble() ); // pixel x,y
        QgsGeorefDataPoint* data_point;
        if ( ls.count() == 5 )
        {
          bool enable = ls.at( 4 ).toInt();
          data_point = new QgsGeorefDataPoint( mCanvas, mIface->mapCanvas(), pixelCoords, mapCoords, mGcpSrid, id_gcp++, enable, mLegacyMode );
        }
        else
          data_point = new QgsGeorefDataPoint( mCanvas, mIface->mapCanvas(), pixelCoords, mapCoords, mGcpSrid, id_gcp++, true, mLegacyMode );
        mGCPListWidget->addDataPoint( data_point );
        connect( mCanvas, SIGNAL( extentsChanged() ), data_point, SLOT( updateCoords() ) );
      }
      points_file.close();
    }
  }
  else
  {
    qDebug() << QString( "-W-> QgsGeorefPluginGui::loadGCPs[%1] not loading point_file  (layer_gcp_pixels != NULL)" ).arg( mGcp_pixel_layername );
  }
  if ( mGCPListWidget->isDirty() )
  {
    // Informs QgsGCPList that the Initial loading of Point-Pairs has been compleated
    mGCPListWidget->setChanged( true );
    updateGeorefTransform();
    if ( !mActionLinkGeorefToQGis->isChecked() )
    {
      QgsRectangle rectMap = mGeorefTransform.hasCrs() ? mGeorefTransform.getBoundingBox( mCanvas->extent(), true ) : mCanvas->extent();
      QgsRectangle boundingBox = transformViewportBoundingBox( rectMap, mGeorefTransform, true );
      mIface->mapCanvas()->setExtent( boundingBox );
    }
    if ( layer_gcp_points )
    {
      mIface->layerTreeView()->setCurrentLayer( layer_gcp_points );
      mLayerTreeView->setCurrentLayer( layer_gcp_pixels );
      mActionFetchMasterGcpExtent->setEnabled( true );
      mSpatialite_gcp_off = false;
      if (( !isGcpOff() ) && (mGCPListWidget->countDataPointsEnabled() > 3))
      {
        mSpatialite_gcp_off = true;
      }
      // Will set the opposite and all the text
      setSpatialiteGcpUsage();
    }
    mCanvas->refresh();
    mIface->mapCanvas()->refresh();
  }
  return true;
}
bool QgsGeorefPluginGui::loadGcpMaster( QString  s_database_filename )
{
  bool b_rc = false;
  if ( s_database_filename.isNull() )
  {
    if ( !mGcpMasterDatabaseFileName.isNull() )
    {
      s_database_filename = mGcpMasterDatabaseFileName;
    }
    else
    {
      QSettings s;
      QString s_gcp_master = s.value( "/Plugin-GeoReferencer/gcp_master_db", "" ).toString();
      if (( !s_gcp_master.isEmpty() ) && ( QFile::exists( s_gcp_master ) ) )
      {
        mGcpMasterDatabaseFileName = s_gcp_master;
        s_database_filename = mGcpMasterDatabaseFileName;
      }
    }
  }
  if ( !s_database_filename.isNull() )
  {
    QFileInfo database_file( s_database_filename );
    if ( database_file.exists() )
    {
      int i_return_srid = createGcpMasterDb( database_file.canonicalFilePath() );
      if ( i_return_srid != INT_MIN )
      { // will return srid being used in the gcp_master, otherwise INT_MIN if not found
        mGcpMasterDatabaseFileName = database_file.canonicalFilePath();
        mGcpMasterSrid = i_return_srid;
        if ( group_georeferencer )
        {
          if ( !group_gcp_master )
          {
            group_gcp_master = group_georeferencer->addGroup( "Gcp_Master" );
          }
          QString s_gcp_master_layer = QString( "%1|layername=%2(gcp_point)" ).arg( database_file.canonicalFilePath() ).arg( mGcp_master_layername );
          layer_gcp_master = new QgsVectorLayer( s_gcp_master_layer, mGcp_master_layername,  "ogr" );
          if ( setCutlineLayerSettings( layer_gcp_master ) )
          {
            QgsMapLayerRegistry::instance()->addMapLayers( QList<QgsMapLayer *>() << layer_gcp_master, false );
            group_gcp_master->insertLayer( 0, layer_gcp_master );
            connect( layer_gcp_master, SIGNAL( featureAdded( QgsFeatureId ) ), ( QgsGeorefPluginGui* )this, SLOT( featureAdded_cutline( QgsFeatureId ) ) );
            connect( layer_gcp_master, SIGNAL( geometryChanged( QgsFeatureId, QgsGeometry& ) ), ( QgsGeorefPluginGui* )this, SLOT( geometryChanged_cutline( QgsFeatureId, QgsGeometry& ) ) );
            layer_gcp_master->startEditing();
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
    updateGcpDb( mGcp_coverage_name );
    if ( isGcpEnabled() )
    { // mLayer width and height are needed
      updateGcpCompute( mGcp_coverage_name );
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
  if ( mLegacyMode == 0 )
  {
    if ( bPointsFileName )
    {
      QFile points_file( mGcpPointsFileName );
      if ( points_file.open( QIODevice::WriteOnly ) )
      {
        QTextStream points( &points_file );
        points << "mapX,mapY,pixelX,pixelY,enable" << endl;
        Q_FOREACH ( QgsGeorefDataPoint *pt, *getGCPList() )
        {
          points << QString( "%1,%2,%3,%4,%5" )
          .arg( qgsDoubleToString( pt->mapCoords().x() ),
                qgsDoubleToString( pt->mapCoords().y() ),
                qgsDoubleToString( pt->pixelCoords().x() ),
                qgsDoubleToString( pt->pixelCoords().y() ) )
          .arg( pt->isEnabled() ) << endl;
        }
      }
    }
    else
    {
      mMessageBar->pushMessage( tr( "Write Error" ), tr( "Could not write to GCP points file %1." ).arg( mGcpPointsFileName ), QgsMessageBar::WARNING, messageTimeout() );
      return;
    }
  }
  //  showMessageInLog(tr("GCP points saved in"), mGcpPointsFileName);
}

QgsGeorefPluginGui::SaveGCPs QgsGeorefPluginGui::checkNeedGCPSave()
{
  if ( mLegacyMode == 0 )
  {
    if ( 0 == getGCPList()->count() )
      return QgsGeorefPluginGui::GCPDISCARD;

    // if ( !equalGCPlists( mInitialPoints, getGCPList() ) )
    if ( !mGCPListWidget->hasChanged() )
    {
      QMessageBox::StandardButton a = QMessageBox::information( this, tr( "Save GCPs" ),
                                      tr( "Save GCP points?" ),
                                      QMessageBox::Save | QMessageBox::Discard
                                      | QMessageBox::Cancel );
      if ( a == QMessageBox::Save )
      {
        return QgsGeorefPluginGui::GCPSAVE;
      }
      else if ( a == QMessageBox::Cancel )
      {
        return QgsGeorefPluginGui::GCPCANCEL;
      }
      else if ( a == QMessageBox::Discard )
      {
        return QgsGeorefPluginGui::GCPDISCARD;
      }
    }
  }

  return QgsGeorefPluginGui::GCPSILENTSAVE;
}

// Georeference
bool QgsGeorefPluginGui::georeference()
{
  if ( !checkReadyGeoref() )
    return false;

  if ( mModifiedRasterFileName.isEmpty() && ( QgsGeorefTransform::Linear == mGeorefTransform.transformParametrisation() ||
       QgsGeorefTransform::Helmert == mGeorefTransform.transformParametrisation() ) )
  {
    QgsPoint origin;
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

bool QgsGeorefPluginGui::writeWorldFile( const QgsPoint& origin, double pixelXSize, double pixelYSize, double rotation )
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

bool QgsGeorefPluginGui::writePDFMapFile( const QString& fileName, const QgsGeorefTransform& transform )
{
  Q_UNUSED( transform );
  if ( !mCanvas )
  {
    return false;
  }

  QgsRasterLayer *raster_layer;
  if ( mLayer )
  {
    raster_layer = mLayer;
    qDebug() << QString( "QgsGeorefPluginGui::writePDFMapFile -0- title[%1] " ).arg( "mLayer" );
  }
  else
  {
    raster_layer = ( QgsRasterLayer* ) mCanvas->layer( 0 );
  }
  if ( !raster_layer )
  {
    return false;
  }
  double mapRatio =  raster_layer->extent().width() / raster_layer->extent().height();

  QPrinter printer;
  printer.setOutputFormat( QPrinter::PdfFormat );
  printer.setOutputFileName( fileName );

  QSettings s;
  double paperWidth = s.value( "/Plugin-GeoReferencer/Config/WidthPDFMap", "297" ).toDouble();
  double paperHeight = s.value( "/Plugin-GeoReferencer/Config/HeightPDFMap", "420" ).toDouble();

  //create composition
  QgsComposition* composition = new QgsComposition( mCanvas->mapSettings() );
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
  QString s_title = raster_layer->title();
  if (( s_title.isEmpty() ) && ( !raster_layer->abstract().isEmpty() ) )
    s_title = raster_layer->abstract();
  if ( s_title.isEmpty() )
  {
    QFileInfo raster_file( mRasterFileName );
    s_title = raster_file.fileName();
  }
  /*
   QFont titleFont=QApplication::font();;
   titleFont.setPointSize( 9 );
   titleFont.setBold( true );
   QgsComposerLabel* titleLabel = new QgsComposerLabel( composition );
   titleLabel->setFont( titleFont );
   titleLabel->setText( s_title );
   composition->addItem( titleLabel );
   titleLabel->setSceneRect( QRectF( leftMargin, 5, contentWidth, 8 ) );
   titleLabel->setFrameEnabled( false );
   */

  //composer map
  QgsComposerMap* composerMap = new QgsComposerMap( composition, leftMargin, topMargin, contentWidth, contentHeight );
  // QgsComposerMap* composerMap = new QgsComposerMap( composition, leftMargin, titleLabel->rect().bottom() + titleLabel->pos().y(), contentWidth, contentHeight );
  composerMap->setKeepLayerSet( true );
  QStringList list_layers;
  for ( int i_list = 0;i_list < mCanvas->mapSettings().layers().size();i_list++ )
  { // the first is on top of the second
    list_layers.append( mCanvas->mapSettings().layers().at( i_list ) );
  }
  composerMap->setLayerSet( list_layers );
  composerMap->zoomToExtent( raster_layer->extent() );
  composition->addItem( composerMap );
  printer.setFullPage( true );
  printer.setColorMode( QPrinter::Color );

  QString residualUnits;
  if ( s.value( "/Plugin-GeoReferencer/Config/ResidualUnits" ) == "mapUnits" && mGeorefTransform.providesAccurateInverseTransformation() )
  {
    residualUnits = tr( "map units" );
  }
  else
  {
    residualUnits = tr( "pixels" );
  }
  qDebug() << QString( "QgsGeorefPluginGui::writePDFMapFile -1- title[%1] residualUnits[%2] " ).arg( s_title ).arg( residualUnits );

  //residual plot
  QgsResidualPlotItem* resPlotItem = new QgsResidualPlotItem( composition );
  composition->addItem( resPlotItem );
  resPlotItem->setSceneRect( QRectF( leftMargin, topMargin, contentWidth, contentHeight ) );
  resPlotItem->setExtent( composerMap->extent() );
  resPlotItem->setGCPList( getGCPList() );
  resPlotItem->setConvertScaleToMapUnits( residualUnits == tr( "map units" ) );

  printer.setResolution( composition->printResolution() );
  QPainter painter( &printer );
  composition->setPlotStyle( QgsComposition::Print );
  QRectF paperRectMM = printer.pageRect( QPrinter::Millimeter );
  QRectF paperRectPixel = printer.pageRect( QPrinter::DevicePixel );
  composition->render( &painter, paperRectPixel, paperRectMM );
  // composition->exportAsPDF( fileName );

  delete resPlotItem;
  delete composerMap;
  delete composition;

  return true;
}

bool QgsGeorefPluginGui::writePDFReportFile( const QString& fileName, const QgsGeorefTransform& transform )
{
  if ( !mCanvas )
  {
    return false;
  }

  //create composition A4 with 300 dpi
  QgsComposition* composition = new QgsComposition( mCanvas->mapSettings() );
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

  QSettings s;
  double leftMargin = s.value( "/Plugin-GeoReferencer/Config/LeftMarginPDF", "2.0" ).toDouble();
  double rightMargin = s.value( "/Plugin-GeoReferencer/Config/RightMarginPDF", "2.0" ).toDouble();
  double contentWidth = 210 - ( leftMargin + rightMargin );

  //composer map
  QgsRasterLayer *raster_layer;
  if ( mLayer )
  {
    raster_layer = mLayer;
    qDebug() << QString( "QgsGeorefPluginGui::writePDFReportFile -0- title[%1] " ).arg( "mLayer" );
  }
  else
  {
    raster_layer = ( QgsRasterLayer* ) mCanvas->layer( 0 );
  }

  if ( !raster_layer )
  {
    return false;
  }
  //title [Attempt to read possible metadata - better than 'my_file.tif']
  QString s_title = raster_layer->title();
  if (( s_title.isEmpty() ) && ( !raster_layer->abstract().isEmpty() ) )
    s_title = raster_layer->abstract();
  if ( s_title.isEmpty() )
  {
    QFileInfo raster_file( mRasterFileName );
    s_title = raster_file.fileName();
  }
  QgsComposerLabel* titleLabel = new QgsComposerLabel( composition );
  titleLabel->setFont( titleFont );
  titleLabel->setText( s_title );
  composition->addItem( titleLabel );
  titleLabel->setSceneRect( QRectF( leftMargin, 5, contentWidth, 8 ) );
  titleLabel->setFrameEnabled( false );
  qDebug() << QString( "QgsGeorefPluginGui::writePDFReportFile -1- title[%1] " ).arg( s_title );

  QgsRectangle layerExtent = raster_layer->extent();
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

  QgsComposerMap* composerMap = new QgsComposerMap( composition, leftMargin, titleLabel->rect().bottom() + titleLabel->pos().y(), mapWidthMM, mapHeightMM );
  composerMap->setLayerSet( mCanvas->mapSettings().layers() );
  composerMap->zoomToExtent( layerExtent );
  composerMap->setMapCanvas( mCanvas );
  composition->addItem( composerMap );

  QgsComposerTextTableV2* parameterTable = nullptr;
  double scaleX, scaleY, rotation;
  QgsPoint origin;

  QgsComposerLabel* parameterLabel = nullptr;
  //transformation that involves only scaling and rotation (linear or helmert) ?
  bool wldTransform = transform.getOriginScaleRotation( origin, scaleX, scaleY, rotation );

  QString residualUnits;
  if ( s.value( "/Plugin-GeoReferencer/Config/ResidualUnits" ) == "mapUnits" && mGeorefTransform.providesAccurateInverseTransformation() )
  {
    residualUnits = tr( "map units" );
  }
  else
  {
    residualUnits = tr( "pixels" );
  }

  QGraphicsRectItem* previousItem = composerMap;
  if ( wldTransform )
  {
    QString parameterTitle = tr( "Transformation parameters" ) + QLatin1String( " (" ) + convertTransformEnumToString( transform.transformParametrisation() ) + QLatin1String( ")" );
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

    QgsComposerFrame* tableFrame = new QgsComposerFrame( composition, parameterTable, leftMargin, parameterLabel->rect().bottom() + parameterLabel->pos().y() + 5, contentWidth, 12 );
    parameterTable->addFrame( tableFrame );

    composition->addItem( tableFrame );
    parameterTable->setGridStrokeWidth( 0.1 );

    previousItem = tableFrame;
  }

  QgsComposerLabel* residualLabel = new QgsComposerLabel( composition );
  residualLabel->setFont( titleFont );
  residualLabel->setText( tr( "Residuals" ) );
  composition->addItem( residualLabel );
  residualLabel->setSceneRect( QRectF( leftMargin, previousItem->rect().bottom() + previousItem->pos().y() + 5, contentWidth, 6 ) );
  residualLabel->setFrameEnabled( false );

  //residual plot
  QgsResidualPlotItem* resPlotItem = new QgsResidualPlotItem( composition );
  composition->addItem( resPlotItem );
  resPlotItem->setSceneRect( QRectF( leftMargin, residualLabel->rect().bottom() + residualLabel->pos().y() + 5, contentWidth, composerMap->rect().height() ) );
  resPlotItem->setExtent( composerMap->extent() );
  resPlotItem->setGCPList( getGCPList() );

  //necessary for the correct scale bar unit label
  resPlotItem->setConvertScaleToMapUnits( residualUnits == tr( "map units" ) );

  QgsComposerTextTableV2* gcpTable = new QgsComposerTextTableV2( composition, false );
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

  QgsGCPList::const_iterator gcpIt = getGCPList()->constBegin();
  QList< QStringList > gcpTableContents;
  for ( ; gcpIt != getGCPList()->constEnd(); ++gcpIt )
  {
    QStringList currentGCPStrings;
    QPointF residual = ( *gcpIt )->residual();
    double residualTot = sqrt( residual.x() * residual.x() +  residual.y() * residual.y() );

    currentGCPStrings << QString::number(( *gcpIt )->id() );
    if (( *gcpIt )->isEnabled() )
    {
      currentGCPStrings << tr( "yes" );
    }
    else
    {
      currentGCPStrings << tr( "no" );
    }
    currentGCPStrings << QString::number(( *gcpIt )->pixelCoords().x(), 'f', 0 ) << QString::number(( *gcpIt )->pixelCoords().y(), 'f', 0 ) << QString::number(( *gcpIt )->mapCoords().x(), 'f', 3 )
    <<  QString::number(( *gcpIt )->mapCoords().y(), 'f', 3 ) <<  QString::number( residual.x() ) <<  QString::number( residual.y() ) << QString::number( residualTot );
    gcpTableContents << currentGCPStrings ;
  }

  gcpTable->setContents( gcpTableContents );

  double firstFrameY = resPlotItem->rect().bottom() + resPlotItem->pos().y() + 5;
  double firstFrameHeight = 287 - firstFrameY;
  QgsComposerFrame* gcpFirstFrame = new QgsComposerFrame( composition, gcpTable, leftMargin, firstFrameY, contentWidth, firstFrameHeight );
  gcpTable->addFrame( gcpFirstFrame );
  composition->addItem( gcpFirstFrame );

  QgsComposerFrame* gcpSecondFrame = new QgsComposerFrame( composition, gcpTable, leftMargin, 10, contentWidth, 277.0 );
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

  QgsPoint origin;
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
void QgsGeorefPluginGui::showGDALScript( const QStringList& commands )
{
  QString script = commands.join( "\n" ) + '\n';

  // create window to show gdal script
  QDialogButtonBox *bbxGdalScript = new QDialogButtonBox( QDialogButtonBox::Cancel, Qt::Horizontal, this );
  QPushButton *pbnCopyInClipBoard = new QPushButton( getThemeIcon( "/mActionEditPaste.svg" ),
      tr( "Copy to Clipboard" ), bbxGdalScript );
  bbxGdalScript->addButton( pbnCopyInClipBoard, QDialogButtonBox::AcceptRole );

  QPlainTextEdit *pteScript = new QPlainTextEdit();
  pteScript->setReadOnly( true );
  pteScript->setWordWrapMode( QTextOption::NoWrap );
  pteScript->setPlainText( tr( "%1" ).arg( script ) );

  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget( pteScript );
  layout->addWidget( bbxGdalScript );

  QDialog *dlgShowGdalScrip = new QDialog( this );
  dlgShowGdalScrip->setWindowTitle( tr( "GDAL script" ) );
  dlgShowGdalScrip->setLayout( layout );

  connect( bbxGdalScript, SIGNAL( accepted() ), dlgShowGdalScrip, SLOT( accept() ) );
  connect( bbxGdalScript, SIGNAL( rejected() ), dlgShowGdalScrip, SLOT( reject() ) );

  if ( dlgShowGdalScrip->exec() == QDialog::Accepted )
  {
    QApplication::clipboard()->setText( pteScript->toPlainText() );
  }
}

QString QgsGeorefPluginGui::generateGDALtranslateCommand( bool generateTFW )
{
  QStringList gdalCommand;
  gdalCommand << "gdal_translate" << "-of GTiff" << generateGDALTifftags( 0 );
  if ( generateTFW )
  {
    // say gdal generate associated ESRI world file
    gdalCommand << "-co TFW=YES";
  }

  // mj10777: replaces Q_FOREACH loop
  gdalCommand << generateGDALGcpList();

  QFileInfo raster_file( mRasterFileName );
  mTranslatedRasterFileName = QDir::tempPath() + '/' + raster_file.fileName();
  gdalCommand << QString( "\"%1\"" ).arg( mRasterFileName ) << QString( "\"%1\"" ).arg( mTranslatedRasterFileName );

  return gdalCommand.join( " " );
}

QString QgsGeorefPluginGui::generateGDALwarpCommand( const QString& resampling, const QString& compress,
    bool useZeroForTrans, int order, double targetResX, double targetResY )
{
  QStringList gdalCommand;
  gdalCommand << "gdalwarp" << "-r" << resampling << generateGDALTifftags( 1 ) << generateGDALNodataCutlineParms();

  if ( order > 0 && order <= 3 )
  {
    // Let gdalwarp use polynomial warp with the given degree
    gdalCommand << "-order" << QString::number( order );
  }
  else
  {
    // Otherwise, use thin plate spline interpolation
    gdalCommand << "-tps";
  }
  gdalCommand << "-co COMPRESS=" + compress << ( useZeroForTrans ? "-dstalpha" : "" );

  if ( targetResX != 0.0 && targetResY != 0.0 )
  {
    gdalCommand << "-tr" << QString::number( targetResX, 'f' ) << QString::number( targetResY, 'f' );
  }

  gdalCommand << QString( "\"%1\"" ).arg( mTranslatedRasterFileName ) << QString( "\"%1\"" ).arg( mModifiedRasterFileName );

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

  if ( getGCPList()->count() < ( int )mGeorefTransform.getMinimumGCPCount() )
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
  /*
   QVector<QgsPoint> mapCoords, pixelCoords;
   if ( mGCPListWidget->gcpList() )
     mGCPListWidget->gcpList()->createGCPVectors( mapCoords, pixelCoords );
   else
     return false;

   // Parametrize the transform with GCPs
   if ( !mGeorefTransform.updateParametersFromGCPs( mapCoords, pixelCoords ) )
   {
     return false;
   }
   */
  if ( !bAvoidUnneededUpdates )
  {
    mGCPListWidget->updateGCPList(); // will ony do something isDirty()
    if ( !mGCPListWidget->transformUpdated() )
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
      QgsPoint src, raster;
      switch ( edge )
      {
        case 0:
          src = QgsPoint( oX + ( double )s * stepX, oY );
          break;
        case 1:
          src = QgsPoint( oX + ( double )s * stepX, dY );
          break;
        case 2:
          src = QgsPoint( oX, oY + ( double )s * stepY );
          break;
        case 3:
          src = QgsPoint( dX, oY + ( double )s * stepY );
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
      return "near";
    case QgsImageWarper::Bilinear:
      return "bilinear";
    case QgsImageWarper::Cubic:
      return "cubic";
    case QgsImageWarper::CubicSpline:
      return "cubicspline";
    case QgsImageWarper::Lanczos:
      return "lanczos";
  }
  return "";
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
  QString worldFileName = "";
  int point = rasterFileName.lastIndexOf( '.' );
  if ( point != -1 && point != rasterFileName.length() - 1 )
    worldFileName = rasterFileName.left( point + 1 ) + "wld";

  return worldFileName;
}

// Note this code is duplicated from qgisapp.cpp because
// I didnt want to make plugins on qgsapplication [TS]
QIcon QgsGeorefPluginGui::getThemeIcon( const QString &theName )
{
  if ( QFile::exists( QgsApplication::activeThemePath() + theName ) )
  {
    return QIcon( QgsApplication::activeThemePath() + theName );
  }
  else if ( QFile::exists( QgsApplication::defaultThemePath() + theName ) )
  {
    return QIcon( QgsApplication::defaultThemePath() + theName );
  }
  else
  {
    QSettings settings;
    QString themePath = ":/icons/" + settings.value( "/Themes" ).toString() + theName;
    if ( QFile::exists( themePath ) )
    {
      return QIcon( themePath );
    }
    else
    {
      return QIcon( ":/icons/default" + theName );
    }
  }
}

bool QgsGeorefPluginGui::checkFileExisting( const QString& fileName, const QString& title, const QString& question )
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

bool QgsGeorefPluginGui::equalGCPlists( const QgsGCPList *list1, const QgsGCPList *list2 )
{ // posibly no longer needed
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

void QgsGeorefPluginGui::clearGCPData()
{
  //force all list widget editors to close before removing data points
  //otherwise the editors try to update deleted data points when they close
  mGCPListWidget->closeEditors();
  if ( layer_gcp_points )
  {
    if ( group_georeferencer )
    { // In QGIS--Canvas
#if 0
      exportLayerDefinition( group_georeferencer );
#endif
      if ( group_gcp_cutlines )
      { // Order: disconnect ; save ; group_remove ; MapLayerRemove
        // group_gcp_cutlines->removeLayer( layer_mercator_polygons );
        // Warning: Object::disconnect: No such signal QObject::featureAdded( QgsFeatureId )
        // Warning: Object::disconnect:  (receiver name: 'QgsGeorefPluginGuiBase')
        // ?? No idea why ',( QgsGeorefPluginGui* )this' is needed. Otherwise crash ??
        // qDebug() << QString( "-I-> QgsGeorefPluginGui::clearGCPData[%1]" ).arg( "layer_cutline_points" );
        disconnect( layer_cutline_points, SIGNAL( featureAdded( QgsFeatureId ) ), ( QgsGeorefPluginGui* )this, SLOT( featureAdded_cutline( QgsFeatureId ) ) );
        disconnect( layer_cutline_points, SIGNAL( geometryChanged( QgsFeatureId, QgsGeometry& ) ), ( QgsGeorefPluginGui* )this, SLOT( geometryChanged_cutline( QgsFeatureId, QgsGeometry& ) ) );
        saveEditsGcp( layer_cutline_points, false );
        group_gcp_cutlines->removeLayer( layer_cutline_points );
        QgsMapLayerRegistry::instance()->removeMapLayers(( QStringList() << layer_cutline_points->id() ) );
        layer_cutline_points = nullptr;
        //-------
        // qDebug() << QString( "-I-> QgsGeorefPluginGui::clearGCPData[%1]" ).arg( "layer_cutline_linestrings" );
        disconnect( layer_cutline_linestrings, SIGNAL( featureAdded( QgsFeatureId ) ), ( QgsGeorefPluginGui* )this, SLOT( featureAdded_cutline( QgsFeatureId ) ) );
        disconnect( layer_cutline_linestrings, SIGNAL( geometryChanged( QgsFeatureId, QgsGeometry& ) ), ( QgsGeorefPluginGui* )this, SLOT( geometryChanged_cutline( QgsFeatureId, QgsGeometry& ) ) );
        saveEditsGcp( layer_cutline_linestrings, false );
        group_gcp_cutlines->removeLayer( layer_cutline_linestrings );
        QgsMapLayerRegistry::instance()->removeMapLayers(( QStringList() << layer_cutline_linestrings->id() ) );
        layer_cutline_linestrings = nullptr;
        //-------
        // qDebug() << QString( "-I-> QgsGeorefPluginGui::clearGCPData[%1]" ).arg( "layer_cutline_polygons" );
        disconnect( layer_cutline_polygons, SIGNAL( featureAdded( QgsFeatureId ) ), ( QgsGeorefPluginGui* )this, SLOT( featureAdded_cutline( QgsFeatureId ) ) );
        disconnect( layer_cutline_polygons, SIGNAL( geometryChanged( QgsFeatureId, QgsGeometry& ) ), ( QgsGeorefPluginGui* )this, SLOT( geometryChanged_cutline( QgsFeatureId, QgsGeometry& ) ) );
        saveEditsGcp( layer_cutline_polygons, false );
        group_gcp_cutlines->removeLayer( layer_cutline_polygons );
        QgsMapLayerRegistry::instance()->removeMapLayers(( QStringList() << layer_cutline_polygons->id() ) );
        layer_cutline_polygons = nullptr;
        //-------
        // qDebug() << QString( "-I-> QgsGeorefPluginGui::clearGCPData[%1]" ).arg( "layer_mercator_polygons_transform" );
#if 0
        disconnect( layer_mercator_polygons_transform, SIGNAL( featureAdded( QgsFeatureId ) ), ( QgsGeorefPluginGui* )this, SLOT( featureAdded_cutline( QgsFeatureId ) ) );
        disconnect( layer_mercator_polygons_transform, SIGNAL( geometryChanged( QgsFeatureId, QgsGeometry& ) ), ( QgsGeorefPluginGui* )this, SLOT( geometryChanged_cutline( QgsFeatureId, QgsGeometry& ) ) );
#endif
        saveEditsGcp( layer_mercator_polygons_transform, false );
        group_gcp_cutlines->removeLayer( layer_mercator_polygons_transform );
        QgsMapLayerRegistry::instance()->removeMapLayers(( QStringList() << layer_mercator_polygons_transform->id() ) );
        layer_mercator_polygons_transform = nullptr;
        //-------
        group_georeferencer->removeChildNode(( QgsLayerTreeNode * )group_gcp_cutlines );
        group_gcp_cutlines = nullptr;
      }
      if ( group_gcp_points )
      { // Order: disconnect ; save ; group_remove ; MapLayerRemove
        // qDebug() << QString( "-I-> QgsGeorefPluginGui::clearGCPData[%1]" ).arg( "layer_gcp_points" );
        disconnect( layer_gcp_points, SIGNAL( featureAdded( QgsFeatureId ) ), ( QgsGeorefPluginGui* )this, SLOT( featureAdded_gcp( QgsFeatureId ) ) );
        disconnect( layer_gcp_points, SIGNAL( geometryChanged( QgsFeatureId, QgsGeometry& ) ), ( QgsGeorefPluginGui* )this, SLOT( geometryChanged_gcp( QgsFeatureId, QgsGeometry& ) ) );
        saveEditsGcp( layer_gcp_points, false );
        group_gcp_points->removeLayer( layer_gcp_points );
        QgsMapLayerRegistry::instance()->removeMapLayers(( QStringList() << layer_gcp_points->id() ) );
        layer_gcp_points = nullptr;
        //-------
        group_gcp_points = nullptr;
        group_georeferencer->removeChildNode(( QgsLayerTreeNode * )group_gcp_points );
      }
      if ( group_gtif_rasters )
      { // Order:  group_remove ; MapLayerRemove
        if (( mLayer_gtif_raster ) && ( group_gtif_raster ) )
        { // Check if layer is still contained in group
          if ( group_gtif_rasters->findLayer( group_gtif_raster->layerId() ) )
          { // Remove only if layer is still contained in the main-group, may have been movwd elsewhere by the user
            group_gtif_rasters->removeLayer( mLayer_gtif_raster );
            QgsMapLayerRegistry::instance()->removeMapLayers(( QStringList() << mLayer_gtif_raster->id() ) );
            mLayer_gtif_raster = nullptr;
          }
        }
        group_georeferencer->removeChildNode(( QgsLayerTreeNode * )group_gtif_rasters );
        group_gtif_rasters = nullptr;
      }
      if ( group_gcp_master )
      { // Order: disconnect ; save ; group_remove ; MapLayerRemove
        // qDebug() << QString( "-I-> QgsGeorefPluginGui::clearGCPData[%1]" ).arg( "layer_gcp_master" );
        disconnect( layer_gcp_master, SIGNAL( featureAdded( QgsFeatureId ) ), ( QgsGeorefPluginGui* )this, SLOT( featureAdded_cutline( QgsFeatureId ) ) );
        disconnect( layer_gcp_master, SIGNAL( geometryChanged( QgsFeatureId, QgsGeometry& ) ), ( QgsGeorefPluginGui* )this, SLOT( geometryChanged_cutline( QgsFeatureId, QgsGeometry& ) ) );
        saveEditsGcp( layer_gcp_master, false );
        group_gcp_master->removeLayer( layer_gcp_master );
        group_gcp_master = nullptr;
        QgsMapLayerRegistry::instance()->removeMapLayers(( QStringList() << layer_gcp_master->id() ) );
        layer_gcp_master = nullptr;
      }
      QgsProject::instance()->layerTreeRoot()->removeChildNode(( QgsLayerTreeNode * )group_georeferencer );
      group_georeferencer = nullptr;
    }

    if ( group_cutline_mercator )
    { // Georeferencer-Canvas
      if ( layer_mercator_polygons )
      { // Order: disconnect ; save ; group_remove ; MapLayerRemove
        // qDebug() << QString( "-I-> QgsGeorefPluginGui::clearGCPData[%1]" ).arg( "layer_mercator_polygons" );
        disconnect( layer_mercator_polygons, SIGNAL( featureAdded( QgsFeatureId ) ), ( QgsGeorefPluginGui* )this, SLOT( featureAdded_cutline( QgsFeatureId ) ) );
        disconnect( layer_mercator_polygons, SIGNAL( geometryChanged( QgsFeatureId, QgsGeometry& ) ), ( QgsGeorefPluginGui* )this, SLOT( geometryChanged_cutline( QgsFeatureId, QgsGeometry& ) ) );
        saveEditsGcp( layer_mercator_polygons, false );
        group_cutline_mercator->removeLayer( layer_mercator_polygons );
        QgsMapLayerRegistry::instance()->removeMapLayers(( QStringList() << layer_mercator_polygons->id() ) );
        layer_mercator_polygons = nullptr;
      }
      if ( layer_gcp_pixels )
      { // Order: disconnect ; save ; group_remove ; MapLayerRemove
        // qDebug() << QString( "-I-> QgsGeorefPluginGui::clearGCPData[%1]" ).arg( "layer_gcp_pixels" );
        disconnect( layer_gcp_pixels, SIGNAL( featureAdded( QgsFeatureId ) ), ( QgsGeorefPluginGui* )this, SLOT( featureAdded_gcp( QgsFeatureId ) ) );
        disconnect( layer_gcp_pixels, SIGNAL( geometryChanged( QgsFeatureId, QgsGeometry& ) ), ( QgsGeorefPluginGui* )this, SLOT( geometryChanged_gcp( QgsFeatureId, QgsGeometry& ) ) );
        saveEditsGcp( layer_gcp_pixels, false );
        QgsMapLayerRegistry::instance()->removeMapLayers(( QStringList() << layer_gcp_pixels->id() ) );
        group_cutline_mercator->removeLayer( layer_gcp_pixels );
        layer_gcp_pixels = nullptr;
      }
      mRootLayerTreeGroup->removeChildNode(( QgsLayerTreeNode * )group_cutline_mercator );
      group_cutline_mercator = nullptr;
    }
  }
  if ( mGCPListWidget->countDataPoints() > 0 )
  {
    mGCPListWidget->clearDataPoints();
    mGCPListWidget->updateGCPList();
    QList<QgsMapCanvasLayer> sa_empty;
    mCanvas->setLayerSet( sa_empty );
    mIface->mapCanvas()->refresh();
    mSpatialite_gcp_off = false;
    mActionSpatialiteGcp->setEnabled( !mSpatialite_gcp_off );
    mActionFetchMasterGcpExtent->setEnabled( !mSpatialite_gcp_off );
    qDebug() << QString( "-I-> QgsGeorefPluginGui::clearGCPData[%1]" ).arg( "finished" );
    if ( mGCPListWidget->countDataPoints() > 300 )
    {
      bAvoidUnneededUpdates = true;
      mGCPListWidget->setAvoidUnneededUpdates( bAvoidUnneededUpdates );
    }
    //-------------------------------------------------------------------------------------------------------------------
    // -- Possible cause of the following messages (after this point):
    // -->  layer was set to nullptr before being properly disconnected / removed
    //-------------------------------------------------------------------------------------------------------------------
    // Warning: Object::connect: No such signal QObject::featureAdded( QgsFeatureId )
    // Warning: Object::connect: No such signal QObject::featureDeleted( QgsFeatureId )
    // Warning: Object::connect: No such signal QObject::geometryChanged( QgsFeatureId, QgsGeometry& )
    // Warning: Object::connect: No such signal QObject::dataChanged()
    //-------------------------------------------------------------------------------------------------------------------
  }
}

int QgsGeorefPluginGui::messageTimeout()
{
  QSettings settings;
  return settings.value( "/qgis/messageTimeout", 5 ).toInt();
}

// mj10777: added functions
QString QgsGeorefPluginGui::generateGDALGcpList()
{
  QStringList gdalCommand;

  for ( int i = 0; i < getGCPList()->countDataPoints(); ++i )
  {
    QgsGeorefDataPoint *data_point = getGCPList()->at( i );
    if ( data_point->isEnabled() )
    {
      gdalCommand << QString( "-gcp %1 %2 %3 %4" ).arg( data_point->pixelCoords().x() ).arg( -data_point->pixelCoords().y() ).arg( data_point->mapCoords().x() ).arg( data_point->mapCoords().y() );
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
    if ( layer_mercator_polygons )
    {
      QgsAttributeList lst_needed_fields;
      lst_needed_fields.append( 0 ); // id_cutline
      lst_needed_fields.append( 2 ); // cutline_type
      lst_needed_fields.append( 3 ); // name
      QgsFeature fet_polygon;
      QgsFeatureRequest retrieve_request = QgsFeatureRequest().setFilterExpression( QString( "id_gcp_coverage=%1" ).arg( mGcpDbData->mId_gcp_coverage ) );
      QgsFeatureIterator fit_polygons = layer_mercator_polygons->getFeatures( retrieve_request.setSubsetOfAttributes( lst_needed_fields ) );
      while ( fit_polygons.nextFeature( fet_polygon ) )
      {
        if ( fet_polygon .geometry() && fet_polygon.geometry()->type() == QGis::Polygon )
        {
          int i_cutline_type = fet_polygon.attribute( QString( "cutline_type" ) ).toInt();
          if ( i_cutline_type == 77 )
          { // Ignore any other Polygons [77=default value]
            if ( gdalCommand.size() == 0 )
            { // Create a World-file (one) for the input file [emulating epsg:3395 'WGS 84 / World Mercator']
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
  if ( mGcpDbData->mGcp_coverage_title.isEmpty() )
  { //  -to 'TIFFTAG_DOCUMENTNAME=1700 Grundriss der Friedrichstadt und Dorotheenstadt'
    gdal_tifftags << QString( "%1 'TIFFTAG_DOCUMENTNAME=%2'" ).arg( s_parm ).arg( mGcpDbData->mGcp_coverage_title );
  }
  if ( !mGcpDbData->mGcp_coverage_abstract.isEmpty() )
  { // -to 'TIFFTAG_IMAGEDESCRIPTION=1700 Grundriss der Friedrichstadt und Dorotheenstadt, Federzeichnung um 1700.'
    gdal_tifftags << QString( "%1 'TIFFTAG_IMAGEDESCRIPTION=%2'" ).arg( s_parm ).arg( mGcpDbData->mGcp_coverage_abstract );
  }
  if ( !mGcpDbData->mGcp_coverage_copyright.isEmpty() )
  { // -to 'TIFFTAG_COPYRIGHT=https://de.wikipedia.org/wiki/Datei:Friedrichstadt_dorotheensta.jpg'
    gdal_tifftags << QString( "%1 'TIFFTAG_COPYRIGHT=%2'" ).arg( s_parm ).arg( mGcpDbData->mGcp_coverage_copyright );
  }
  if ( !mGcpDbData->mGcp_coverage_map_date.isEmpty() )
  { // -to 'TIFFTAG_DATETIME=1700'
    gdal_tifftags << QString( "%1 'TIFFTAG_DATETIME=%2'" ).arg( s_parm ).arg( mGcpDbData->mGcp_coverage_map_date );
  }
  if ( !mGcpDbData->map_providertags.value( "TIFFTAG_ARTIST" ).isEmpty() )
  { // -to 'TIFFTAG_ARTIST=mj10777.de.eu'
    gdal_tifftags << QString( "%1 'TIFFTAG_ARTIST=%2'" ).arg( s_parm ).arg( mGcpDbData->map_providertags.value( "TIFFTAG_ARTIST" ) );
  }
  return gdal_tifftags.join( " " );
}
QString QgsGeorefPluginGui::generateGDALNodataCutlineParms()
{
  QStringList gdal_nodata_cutline;
  if ( mGcpDbData->mRasterNodata >= 0 )
  { // -srcnodata 255 -dstnodata 255
    gdal_nodata_cutline << QString( "-srcnodata %1 -dstnodata %1 " ).arg( mGcpDbData->mRasterNodata );
  }
  if ( mGcpDbData->mId_gcp_cutline >= 0 )
  { // -crop_to_cutline -cutline qgis/cutline.gcp.db -csql "SELECT cutline_polygon FROM create_cutline_polygons WHERE (id_cutline = 20);"
    QString s_cutline_sql = QString( "SELECT cutline_polygon FROM create_cutline_polygons WHERE (id_cutline = %1);" ).arg( mGcpDbData->mId_gcp_cutline );
    gdal_nodata_cutline << QString( "-crop_to_cutline -cutline %1 -csql \"%2\"" ).arg( mGcpDbData->mGcpDatabaseFileName ).arg( s_cutline_sql );
  }
  return gdal_nodata_cutline.join( " " );
}
int  QgsGeorefPluginGui::getGcpTransformParam( QgsGeorefTransform::TransformParametrisation i_TransformParam )
{ // Use when calling QgsGeorefPluginGui::getGcpConvert
  int i_GpsTransformParam = 0;
  switch ( i_TransformParam )
  { // The Spatialite GCP_Transform/Compute uses a different numbering than QgsGeorefTransform::TransformParametrisation
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
QgsGeorefTransform::TransformParametrisation  QgsGeorefPluginGui::setGcpTransformParam( int i_TransformParam )
{ // Use when calling QgsGeorefPluginGui::getGcpConvert
  QgsGeorefTransform::TransformParametrisation i_GpsTransformParam = QgsGeorefTransform::ThinPlateSpline;
  switch ( i_TransformParam )
  { // The Spatialite GCP_Transform/Compute uses a different numbering than QgsGeorefTransform::TransformParametrisation
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
{ // Use when calling QgsGeorefPluginGui::getGcpConvert
  QString s_GpsResamplingMethod = "Lanczos";
  switch ( i_ResamplingMethod )
  { // In a readable form for the Database
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
QgsImageWarper::ResamplingMethod QgsGeorefPluginGui::setGcpResamplingMethod( QString s_ResamplingMethod )
{
  QgsImageWarper::ResamplingMethod i_ResamplingMethod = QgsImageWarper::Lanczos;
  if ( s_ResamplingMethod == "NearestNeighbour" )
  {
    i_ResamplingMethod = QgsImageWarper::NearestNeighbour;
  }
  if ( s_ResamplingMethod == "Bilinear" )
  {
    i_ResamplingMethod = QgsImageWarper::Bilinear;
  }
  if ( s_ResamplingMethod == "Cubic" )
  {
    i_ResamplingMethod = QgsImageWarper::Cubic;
  }
  if ( s_ResamplingMethod == "CubicSpline" )
  {
    i_ResamplingMethod = QgsImageWarper::CubicSpline;
  }
  return i_ResamplingMethod;
}

bool QgsGeorefPluginGui::setGcpLayerSettings( QgsVectorLayer *layer_gcp )
{ // Note: these are 'Point'-Layers
  Q_CHECK_PTR( layer_gcp );
  if ( mLegacyMode > 0 )
  {
    QColor color_red( "red" );
    QColor color_yellow( "yellow" );
    QColor color_cyan( "cyan" );
    QColor color_background = color_red;
    QColor color_forground = color_yellow;
    QColor color_shadow = color_cyan;
    switch ( mGcp_label_type )
    {
      case 1:
        mGcpLabelExpression = "id_gcp";
        break;
      case 2:
        if ( layer_gcp->name() == mGcp_points_layername )
          mGcpLabelExpression = "id_gcp||' ['||point_x||', '||point_y||']'";
        else
          mGcpLabelExpression = "id_gcp||' ['||pixel_x||', '||pixel_y||']'";
        break;
      default:
        mGcp_label_type = 0;
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
    if (( layer_gcp->isValid() ) && ( mGcp_label_type > 0 ) )
    { // see: core/qgspallabeling.cpp/h and  app/qgslabelingwidget.cpp
      // fontPointSize=15 ; _size_font=12
      color_background = color_yellow;
      color_shadow = color_cyan;
      QgsPalLayerSettings gcp_layer_settings;
      gcp_layer_settings.readFromLayer( layer_gcp );
      gcp_layer_settings.enabled = true;
      gcp_layer_settings.drawLabels = true;
      gcp_layer_settings.fieldName = mGcpLabelExpression;
      gcp_layer_settings.isExpression = true;
      // Text
      gcp_layer_settings.textColor = Qt::black;
      gcp_layer_settings.fontSizeInMapUnits = false;
      gcp_layer_settings.textFont.setPointSizeF( d_size_font );
      // Wrap
      if ( mGcp_label_type == 2 )
      {
        // gcp_layer_settings.wrapChar = QString( "[" ); // removes character
        gcp_layer_settings.multilineHeight = 1.0;
        gcp_layer_settings.multilineAlign = QgsPalLayerSettings::MultiLeft;
      }
      // Background [shape]
      gcp_layer_settings.shapeDraw = true;
      if ( mGcp_label_type == 1 )
      {
        gcp_layer_settings.shapeType = QgsPalLayerSettings::ShapeCircle;
      }
      else
      {
        gcp_layer_settings.shapeType = QgsPalLayerSettings::ShapeRectangle;
      }
      gcp_layer_settings.shapeFillColor = color_background;
      gcp_layer_settings.shapeBorderColor = Qt::black;
      gcp_layer_settings.shapeTransparency = 0;
      gcp_layer_settings.shapeBorderWidth = 1;
      gcp_layer_settings.shapeBorderWidthUnits = QgsPalLayerSettings::MM;
      // Placement
      gcp_layer_settings.placement = QgsPalLayerSettings::OrderedPositionsAroundPoint;
      // gcp_layer_settings.placementFlags = QgsPalLayerSettings::AboveLine | QgsPalLayerSettings::MapOrientation;
      gcp_layer_settings.placementFlags = QgsPalLayerSettings::AboveLine;
      gcp_layer_settings.dist = 5;
      gcp_layer_settings.distInMapUnits = false;
      // Drop shadow
      gcp_layer_settings.shadowDraw = true;
      gcp_layer_settings.shadowUnder = QgsPalLayerSettings::ShadowLowest;
      gcp_layer_settings.shadowOffsetAngle = 135;
      gcp_layer_settings.shadowOffsetDist = 0;
      gcp_layer_settings.shadowOffsetUnits = QgsPalLayerSettings::MM;
      gcp_layer_settings.shadowOffsetMapUnitScale = false;
      gcp_layer_settings.shadowOffsetGlobal = true;
      gcp_layer_settings.shadowRadius = 1.50;
      gcp_layer_settings.shadowRadiusUnits = QgsPalLayerSettings::MM;
      gcp_layer_settings.shadowRadiusMapUnitScale = false;
      gcp_layer_settings.shadowRadiusAlphaOnly = false;
      gcp_layer_settings.shadowTransparency = 30.0;
      gcp_layer_settings.shadowScale = 100;
      gcp_layer_settings.shadowColor = color_shadow;
      gcp_layer_settings.shadowBlendMode = QPainter::CompositionMode_Multiply;
      // Write settings to layer
      gcp_layer_settings.writeToLayer( layer_gcp );
    }
    if (( layer_gcp->isValid() ) && ( layer_gcp->editFormConfig()->suppress() !=  QgsEditFormConfig::SuppressOn ) )
    { // Turn off 'Feature Attibutes' Form - QgsAttributeDialog
      layer_gcp->editFormConfig()->setSuppress( QgsEditFormConfig::SuppressOn );
    }
    // Symbols:
    if (( layer_gcp->isValid() ) && ( layer_gcp->rendererV2() ) )
    {  // see: gui/symbology-ng/qgsrendererv2propertiesdialog.cpp, qgslayerpropertieswidget.cpp and  app/qgsvectorlayerproperties.cpp
      QgsFeatureRendererV2 *gcp_layer_renderer = layer_gcp->rendererV2();
      if ( gcp_layer_renderer )
      {
        QgsRenderContext gcp_layer_symbols_context;
        QgsSymbolV2List gcp_layer_symbols = gcp_layer_renderer->symbols( gcp_layer_symbols_context );
        if ( gcp_layer_symbols.count() == 1 )
        { // Being a Point, there should only be 1
          bool isDirty = false;
          QgsSymbolV2 *gcp_layer_symbol_container = gcp_layer_symbols.at( 0 );
          if ( gcp_layer_symbol_container )
          {
            if (( gcp_layer_symbol_container->type() == QgsSymbolV2::Marker ) && ( gcp_layer_symbol_container->symbolLayerCount() == 1 ) )
            { // remove the only QgsSymbolV2 from the list [will be replaced with a big red star containing a small red circle]
              QgsSimpleMarkerSymbolLayerV2 *gcp_layer_symbol_star =  static_cast<QgsSimpleMarkerSymbolLayerV2*>( gcp_layer_symbol_container->takeSymbolLayer( 0 ) );
              if ( gcp_layer_symbol_star )
              { // prepair the second QgsSymbolV2 from the first
                QgsSimpleMarkerSymbolLayerV2 *gcp_layer_symbol_circle = static_cast<QgsSimpleMarkerSymbolLayerV2*>( gcp_layer_symbol_star->clone() );
                if ( gcp_layer_symbol_circle )
                { // see core/symbology-ng/QgsSymbolV2.h, qgsmarkersymbollayerv2.h
                  // Star
                  color_background = color_red;
                  color_forground = color_yellow;
                  gcp_layer_symbol_star->setFillColor( color_background );
                  gcp_layer_symbol_star->setOutlineColor( Qt::black );
                  gcp_layer_symbol_star->setOutputUnit( QgsSymbolV2::MM );
                  gcp_layer_symbol_star->setSize( d_size_symbol_big );
                  gcp_layer_symbol_star->setMapUnitScale( 0 );
                  gcp_layer_symbol_star->setShape( QgsSimpleMarkerSymbolLayerBase::Star );
                  gcp_layer_symbol_star->setOutlineStyle( Qt::DotLine );
                  gcp_layer_symbol_star->setPenJoinStyle( Qt::RoundJoin );
                  // Circle
                  // gcp_layer_symbol_circle->setFillColor( Qt::transparent );
                  gcp_layer_symbol_circle->setFillColor( color_forground );
                  gcp_layer_symbol_circle->setOutlineColor( Qt::black );
                  gcp_layer_symbol_circle->setOutputUnit( QgsSymbolV2::MM );
                  gcp_layer_symbol_circle->setSize( d_size_symbol_small );
                  gcp_layer_symbol_circle->setMapUnitScale( 0 );
                  gcp_layer_symbol_circle->setShape( QgsSimpleMarkerSymbolLayerBase::Circle );
                  gcp_layer_symbol_circle->setAngle( 0.0 );
                  gcp_layer_symbol_circle->setOutlineStyle( Qt::DashDotLine );
                  gcp_layer_symbol_circle->setPenJoinStyle( Qt::RoundJoin );
                  // Set transparency for both and add Star, Circle
                  gcp_layer_symbol_container->setAlpha( 1.0 );
                  gcp_layer_symbol_container->appendSymbolLayer( gcp_layer_symbol_star );
                  gcp_layer_symbol_container->appendSymbolLayer( gcp_layer_symbol_circle );
                  if (( gcp_layer_symbol_container->type() == QgsSymbolV2::Marker ) && ( gcp_layer_symbol_container->symbolLayerCount() == 2 ) )
                  { // checking that we have what is desired
                    isDirty = true;
                  }
                }
              }
            }
          }
          if ( isDirty )
          {
            layer_gcp->setRendererV2( gcp_layer_renderer );
          }
        }
      }
    }
  }
  return layer_gcp->isValid();
}
double QgsGeorefPluginGui::getCutlineGcpMasterArea( QgsVectorLayer *layer_cutline )
{
  if (( !layer_cutline ) && ( layer_gcp_master ) )
  {
    layer_cutline = layer_gcp_master;
  }
  if ( !layer_cutline )
  {
    return mGcpMasterArea;
  }
  if (( layer_cutline->isValid() ) && ( layer_cutline->rendererV2() ) )
  {
    QgsFeatureRendererV2 *cutline_layer_renderer = layer_cutline->rendererV2();
    if ( cutline_layer_renderer )
    {
      QgsRenderContext cutline_layer_symbols_context;
      QgsSymbolV2List cutline_layer_symbols = cutline_layer_renderer->symbols( cutline_layer_symbols_context );
      if ( cutline_layer_symbols.count() == 1 )
      {
        QgsSymbolV2 *cutline_layer_symbol_container = cutline_layer_symbols.at( 0 );
        if ( cutline_layer_symbol_container )
        {
          if (( cutline_layer_symbol_container->type() == QgsSymbolV2::Marker ) && ( cutline_layer_symbol_container->symbolLayerCount() == 3 ) )
          {
            QgsSimpleMarkerSymbolLayerV2 *cutline_layer_symbol_circle =  static_cast<QgsSimpleMarkerSymbolLayerV2*>( cutline_layer_symbol_container->symbolLayer( 0 )->clone() );
            if ( cutline_layer_symbol_circle )
            {
              mGcpMasterArea = cutline_layer_symbol_circle->size( );
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
  if (( !layer_cutline ) && ( layer_gcp_master ) )
  {
    layer_cutline = layer_gcp_master;
  }
  if ( !layer_cutline )
  {
    return b_rc;
  }
  if (( layer_cutline->isValid() ) && ( layer_cutline->rendererV2() ) )
  {
    QgsFeatureRendererV2 *cutline_layer_renderer = layer_cutline->rendererV2();
    if ( cutline_layer_renderer )
    {
      QgsRenderContext cutline_layer_symbols_context;
      QgsSymbolV2List cutline_layer_symbols = cutline_layer_renderer->symbols( cutline_layer_symbols_context );
      if ( cutline_layer_symbols.count() == 1 )
      {
        QgsSymbolV2 *cutline_layer_symbol_container = cutline_layer_symbols.at( 0 );
        if ( cutline_layer_symbol_container )
        {
          if (( cutline_layer_symbol_container->type() == QgsSymbolV2::Marker ) && ( cutline_layer_symbol_container->symbolLayerCount() == 3 ) )
          {
            QgsSimpleMarkerSymbolLayerV2 *cutline_layer_symbol_circle =  static_cast<QgsSimpleMarkerSymbolLayerV2*>( cutline_layer_symbol_container->takeSymbolLayer( 0 ) );
            if ( cutline_layer_symbol_circle )
            {
              cutline_layer_symbol_circle->setSize( mGcpMasterArea );
              cutline_layer_symbol_container->insertSymbolLayer( 0, cutline_layer_symbol_circle );
              layer_cutline->setRendererV2( cutline_layer_renderer );
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
{ // Note: these are 'Point'-Layers
  Q_CHECK_PTR( layer_cutline );
  if ( mLegacyMode > 0 )
  {
    int i_table_type = 0; // cutline-tables
    QString s_CutlineLabelExpression = "name||' ['||id_gcp_coverage||', '||id_cutline||']'";
    QString s_id_name = "id_cutline";
    if ( layer_cutline->originalName() == "gcp_master" )
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
    QColor color_forground = color_khaki;
    QColor color_shadow = color_cyan;
    int layer_type = -1;
    // Symbols:
    if (( layer_cutline->isValid() ) && ( layer_cutline->rendererV2() ) )
    {  // see: gui/symbology-ng/qgsrendererv2propertiesdialog.cpp, qgslayerpropertieswidget.cpp and  app/qgsvectorlayerproperties.cpp
      QgsFeatureRendererV2 *cutline_layer_renderer = layer_cutline->rendererV2();
      if ( cutline_layer_renderer )
      {
        QgsRenderContext cutline_layer_symbols_context;
        QgsSymbolV2List cutline_layer_symbols = cutline_layer_renderer->symbols( cutline_layer_symbols_context );
        if ( cutline_layer_symbols.count() == 1 )
        { // For the default seetings, there should only be 1
          bool isDirty = false;
          QgsSymbolV2 *cutline_layer_symbol_container = cutline_layer_symbols.at( 0 );
          if ( cutline_layer_symbol_container )
          { // Can be used for all types
            // cutline_points
            if (( cutline_layer_symbol_container->type() == QgsSymbolV2::Marker ) && ( cutline_layer_symbol_container->symbolLayerCount() == 1 ) )
            { // remove the only QgsSymbolV2 from the list [will be replaced with a big green pentagon containing a small red cross]
              layer_type = 0;
              QgsSimpleMarkerSymbolLayerV2 *cutline_layer_symbol_pentagon =  static_cast<QgsSimpleMarkerSymbolLayerV2*>( cutline_layer_symbol_container->takeSymbolLayer( 0 ) );
              if ( cutline_layer_symbol_pentagon )
              { // prepair the second QgsSymbolV2 from the first, insuring that it has the correct default attributes
                QgsSimpleMarkerSymbolLayerV2 *cutline_layer_symbol_cross = static_cast<QgsSimpleMarkerSymbolLayerV2*>( cutline_layer_symbol_pentagon->clone() );
                if ( cutline_layer_symbol_cross )
                { // see core/symbology-ng/QgsSymbolV2.h, qgsmarkersymbollayerv2.h
                  int i_valid_layer_count = 2;
                  color_background = color_darkgreen;
                  color_forground = color_khaki;
                  // https://www.w3.org/TR/SVG/types.html#ColorKeywords
                  // Pentagon [darkgreen #006400]
                  cutline_layer_symbol_pentagon->setFillColor( color_background );
                  cutline_layer_symbol_pentagon->setOutlineColor( Qt::black );
                  cutline_layer_symbol_pentagon->setOutputUnit( QgsSymbolV2::MM );
                  cutline_layer_symbol_pentagon->setSize( d_size_symbol_big );
                  cutline_layer_symbol_pentagon->setMapUnitScale( 0 );
                  cutline_layer_symbol_pentagon->setShape( QgsSimpleMarkerSymbolLayerBase::Star );
                  cutline_layer_symbol_pentagon->setOutlineStyle( Qt::DotLine );
                  cutline_layer_symbol_pentagon->setPenJoinStyle( Qt::RoundJoin );
                  // Cross
                  cutline_layer_symbol_cross->setFillColor( color_forground );
                  cutline_layer_symbol_cross->setOutlineColor( Qt::black );
                  cutline_layer_symbol_cross->setOutputUnit( QgsSymbolV2::MM );
                  cutline_layer_symbol_cross->setSize( d_size_symbol_small );
                  cutline_layer_symbol_cross->setMapUnitScale( 0 );
                  cutline_layer_symbol_cross->setShape( QgsSimpleMarkerSymbolLayerBase::Circle );
                  cutline_layer_symbol_cross->setAngle( 0.0 );
                  cutline_layer_symbol_cross->setOutlineStyle( Qt::DashDotLine );
                  cutline_layer_symbol_cross->setPenJoinStyle( Qt::RoundJoin );
                  // Set transparency for both and add Pentagon, Cross
                  cutline_layer_symbol_container->setAlpha( 1.0 );
                  if ( i_table_type == 1 )
                  { // prepair a (possible) third [gcp_master] QgsSymbolV2 from the first, insuring that it has the correct default attributes
                    QgsSimpleMarkerSymbolLayerV2 *cutline_layer_symbol_circle = static_cast<QgsSimpleMarkerSymbolLayerV2*>( cutline_layer_symbol_cross->clone() );
                    if ( cutline_layer_symbol_circle )
                    { // Will show area of 2.5 meters around the Point [only noticeable when zoomed in]
                      cutline_layer_symbol_circle->setFillColor( color_red );
                      cutline_layer_symbol_circle->setMapUnitScale( 0 );
                      cutline_layer_symbol_circle->setOutputUnit( QgsSymbolV2::MapUnit );
                      cutline_layer_symbol_circle->setSize( mGcpMasterArea );
                      cutline_layer_symbol_container->appendSymbolLayer( cutline_layer_symbol_circle );
                      cutline_layer_symbol_container->setAlpha( 0.75 );
                      i_valid_layer_count++;
                    }
                  }
                  cutline_layer_symbol_container->appendSymbolLayer( cutline_layer_symbol_pentagon );
                  cutline_layer_symbol_container->appendSymbolLayer( cutline_layer_symbol_cross );
                  if (( cutline_layer_symbol_container->type() == QgsSymbolV2::Marker ) && ( cutline_layer_symbol_container->symbolLayerCount() == i_valid_layer_count ) )
                  { // checking that we have what is desired
                    isDirty = true;
                  }
                }
              }
            }
            // cutline_linestrings
            if (( cutline_layer_symbol_container->type() == QgsSymbolV2::Line ) && ( cutline_layer_symbol_container->symbolLayerCount() == 1 ) )
            { // remove the only QgsSymbolV2 from the list [will be replaced with a wide dark-cyan solid line containing a slim Khaki dotted  line]
              layer_type = 1;
              QgsSimpleLineSymbolLayerV2 *cutline_layer_symbol_outline =  static_cast<QgsSimpleLineSymbolLayerV2*>( cutline_layer_symbol_container->takeSymbolLayer( 0 ) );
              if ( cutline_layer_symbol_outline )
              { // prepair the second QgsSymbolV2 from the first, insuring that it has the correct default attributes
                QgsSimpleLineSymbolLayerV2 *cutline_layer_symbol_innerline = static_cast<QgsSimpleLineSymbolLayerV2*>( cutline_layer_symbol_outline->clone() );
                if ( cutline_layer_symbol_innerline )
                { // see core/symbology-ng/QgsSymbolV2.h, qgslinesymbollayerv2.h
                  // https://www.w3.org/TR/SVG/types.html#ColorKeywords
                  // Outerline [darkcyan #008b8b]
                  color_background = color_darkcyan;
                  color_forground = color_khaki;
                  cutline_layer_symbol_outline->setColor( color_background );
                  cutline_layer_symbol_outline->setOutlineColor( Qt::black );
                  cutline_layer_symbol_outline->setOutputUnit( QgsSymbolV2::MM );
                  cutline_layer_symbol_outline->setWidth( d_size_symbol_big );
                  cutline_layer_symbol_outline->setMapUnitScale( 0 );
                  cutline_layer_symbol_outline->setPenStyle( Qt::SolidLine );
                  cutline_layer_symbol_outline->setPenJoinStyle( Qt::RoundJoin );
                  // Innerline [khaki #f0e68c]
                  cutline_layer_symbol_innerline->setColor( color_forground );
                  cutline_layer_symbol_innerline->setOutlineColor( Qt::black );
                  cutline_layer_symbol_innerline->setOutputUnit( QgsSymbolV2::MM );
                  cutline_layer_symbol_innerline->setWidth( d_size_symbol_small );
                  cutline_layer_symbol_innerline->setMapUnitScale( 0 );
                  cutline_layer_symbol_innerline->setPenStyle( Qt::DashLine );
                  cutline_layer_symbol_innerline->setPenJoinStyle( Qt::RoundJoin );
                  // Set transparency for both and add Inner and Outer Linestrings
                  cutline_layer_symbol_container->setAlpha( 0.80 );
                  cutline_layer_symbol_container->appendSymbolLayer( cutline_layer_symbol_outline );
                  cutline_layer_symbol_container->appendSymbolLayer( cutline_layer_symbol_innerline );
                  if (( cutline_layer_symbol_container->type() == QgsSymbolV2::Line ) && ( cutline_layer_symbol_container->symbolLayerCount() == 2 ) )
                  { // checking that we have what is desired
                    isDirty = true;
                  }
                }
              }
            }
            // cutline_polygons
            if (( cutline_layer_symbol_container->type() == QgsSymbolV2::Fill ) && ( cutline_layer_symbol_container->symbolLayerCount() == 1 ) )
            {  // remove the only QgsSymbolV2 from the list [will be replaced with a yellow background containing a red grid]
              layer_type = 2;
              // QgsSimpleFillSymbolLayerV2 *cutline_layer_symbol_grid =  static_cast<QgsSimpleFillSymbolLayerV2*>( cutline_layer_symbol_container->symbolLayer( 0 )->clone() );
              QgsSimpleFillSymbolLayerV2 *cutline_layer_symbol_grid =  static_cast<QgsSimpleFillSymbolLayerV2*>( cutline_layer_symbol_container->takeSymbolLayer( 0 ) );
              if ( cutline_layer_symbol_grid )
              { // Being a different type (QgsGradientFillSymbolLayerV2 instead of QgsSimpleFillSymbolLayerV2) must be created from scratch
                QgsGradientFillSymbolLayerV2 *cutline_layer_symbol_background = new QgsGradientFillSymbolLayerV2();
                if ( cutline_layer_symbol_background )
                { // see core/symbology-ng/QgsSymbolV2.h, qgsfillsymbollayerv2.h
                  // Grid
                  // http://planet.qgis.org/planet/tag/gradient/
                  // shapeburst fills, Using a data defined expression for random colours
                  // https://www.w3.org/TR/SVG/types.html#ColorKeywords
                  // orangered #ff4500 rgb(255, 69, 0)
                  color_background = color_orangered;
                  color_forground = color_crimson;
                  cutline_layer_symbol_grid->setFillColor( color_background );
                  cutline_layer_symbol_grid->setBrushStyle( Qt::DiagCrossPattern );
                  // crimson #dc143c rgb(220, 20, 60) ; #f40101
                  cutline_layer_symbol_grid->setOutlineColor( color_crimson );
                  cutline_layer_symbol_grid->setOutputUnit( QgsSymbolV2::MM );
                  cutline_layer_symbol_grid->setBorderWidth( d_size_symbol_small );
                  cutline_layer_symbol_grid->setMapUnitScale( 0 );
                  cutline_layer_symbol_grid->setBorderStyle( Qt::DashDotLine );
                  cutline_layer_symbol_grid->setPenJoinStyle( Qt::RoundJoin );
                  // Background
                  QgsGradientFillSymbolLayerV2::GradientColorType colorType = QgsGradientFillSymbolLayerV2::SimpleTwoColor;
                  QgsGradientFillSymbolLayerV2::GradientType type = QgsGradientFillSymbolLayerV2::Linear;
                  QgsGradientFillSymbolLayerV2::GradientCoordinateMode coordinateMode = QgsGradientFillSymbolLayerV2::Feature;
                  QgsGradientFillSymbolLayerV2::GradientSpread gradientSpread = QgsGradientFillSymbolLayerV2::Pad;
                  QPointF point_01( 0.50, 0.00 );
                  QPointF point_02( 0.50, 1.00 );
                  double d_rotation = 45.0;
                  cutline_layer_symbol_background->setGradientColorType( colorType );
                  cutline_layer_symbol_background->setColor( color_palegoldenrod );
                  cutline_layer_symbol_background->setColor2( color_cadetblue );
                  cutline_layer_symbol_background->setGradientType( type );
                  cutline_layer_symbol_background->setCoordinateMode( coordinateMode );
                  cutline_layer_symbol_background->setGradientSpread( gradientSpread );
                  cutline_layer_symbol_background->setReferencePoint1( point_01 );
                  cutline_layer_symbol_background->setReferencePoint2( point_02 );
                  cutline_layer_symbol_background->setAngle( d_rotation );
                  // Set transparency for grid and background
                  cutline_layer_symbol_container->setAlpha( 0.5 );
                  cutline_layer_symbol_container->appendSymbolLayer( cutline_layer_symbol_grid );
                  cutline_layer_symbol_container->appendSymbolLayer( cutline_layer_symbol_background );
                  // qDebug() << QString( "QgsGeorefPluginGui::setCutlineLayerSettings -y- type[%1] " ).arg( cutline_layer_renderer->type() );
                  if (( cutline_layer_symbol_container->type() == QgsSymbolV2::Fill ) && ( cutline_layer_symbol_container->symbolLayerCount() == 2 ) )
                  { // checking that we have what is desired
                    isDirty = true;
                    QgsCategorizedSymbolRendererV2* cutline_layer_renderer_categorized = QgsCategorizedSymbolRendererV2::convertFromRenderer( cutline_layer_renderer );
                    if ( cutline_layer_renderer_categorized )
                    { // see core/symbology-ng/qgscategorizedsymbolrendererv2.h
                      // Note: not using '.clone()' is a 'No,no'
                      cutline_layer_renderer_categorized->setSourceSymbol( cutline_layer_symbol_container->clone() );
                      if ( mSvgColors.isEmpty() )
                      {
                        mSvgColors = createSvgColors( 0, true, false );
                      }
                      cutline_layer_renderer_categorized->setClassAttribute( QString( "%1 \% %2" ).arg( s_id_name ).arg( mSvgColors.size() ) );
                      for ( int i = 0;i < mSvgColors.size();i++ )
                      {
                        QVariant value( i );
                        QgsSymbolV2* new_container = cutline_layer_symbol_container->clone();
                        QgsGradientFillSymbolLayerV2 *new_symbol_background = static_cast<QgsGradientFillSymbolLayerV2*>( new_container->takeSymbolLayer( 1 ) );
                        QColor color_svg( mSvgColors.at( i ) );
                        new_symbol_background->setColor2( color_svg );
                        new_container->appendSymbolLayer( new_symbol_background );
                        // qDebug() << QString( "QgsGeorefPluginGui::setCutlineLayerSettings loop[%1,%2] -2-" ).arg( i ).arg( mSvgColors.at( i ) );
                        QgsRendererCategoryV2 category( value, new_container, value.toString(), true );
                        cutline_layer_renderer_categorized->addCategory( category );
                      }
                      isDirty = false;
                      layer_cutline->setRendererV2( cutline_layer_renderer_categorized );
                    }
                  }
                }
              }
            }
          }
          if ( isDirty )
          {
            layer_cutline->setRendererV2( cutline_layer_renderer );
            // qDebug() << QString( "QgsGeorefPluginGui::setCutlineLayerSettings -za- [%1] " ).arg( isDirty );
          }
        }
      }
    }
    // Labels:
    if ( layer_cutline->isValid() )
    { // see: core/qgspallabeling.cpp/h and  app/qgslabelingwidget.cpp
      // fontPointSize=15 ; _size_font=12
      color_background = color_lightblue;
      int i_transparency = 50;
      if ( i_table_type == 1 )
      {
        color_background = color_palegoldenrod;
        color_shadow = color_khaki;
        i_transparency = 25;
      }
      QgsPalLayerSettings cutline_layer_settings;
      cutline_layer_settings.readFromLayer( layer_cutline );
      cutline_layer_settings.enabled = true;
      cutline_layer_settings.drawLabels = true;
      cutline_layer_settings.fieldName = s_CutlineLabelExpression;
      cutline_layer_settings.isExpression = true;
      // Text
      cutline_layer_settings.textColor = Qt::black;
      cutline_layer_settings.fontSizeInMapUnits = false;
      cutline_layer_settings.textFont.setPointSizeF( d_size_font );
      // Wrap
      // cutline_layer_settings.wrapChar = QString( "[" ); // removes character
      cutline_layer_settings.multilineHeight = 1.0;
      cutline_layer_settings.multilineAlign = QgsPalLayerSettings::MultiCenter;
      // Background [shape]
      cutline_layer_settings.shapeDraw = true;
      cutline_layer_settings.shapeType = QgsPalLayerSettings::ShapeEllipse;
      cutline_layer_settings.shapeFillColor = color_background;
      cutline_layer_settings.shapeBorderColor = color_darkgreen;
      cutline_layer_settings.shapeTransparency = i_transparency;
      cutline_layer_settings.shapeBorderWidth = 1;
      cutline_layer_settings.shapeBorderWidthUnits = QgsPalLayerSettings::MM;
      // Placement
      cutline_layer_settings.dist = 5;
      cutline_layer_settings.distInMapUnits = false;
      switch ( layer_type )
      { // 0=point ; 1= linestring ; 2=polygon
        case 1:
          cutline_layer_settings.placement = QgsPalLayerSettings::OrderedPositionsAroundPoint;
          cutline_layer_settings.placementFlags = QgsPalLayerSettings::AboveLine;
          break;
        case 2:
          // Visible Polygon, Force point inside polygon, QuadrantOver, Offset from centroid
          cutline_layer_settings.centroidWhole = false;
          cutline_layer_settings.centroidInside = true;
          cutline_layer_settings.fitInPolygonOnly = false;
          cutline_layer_settings.quadOffset = QgsPalLayerSettings::QuadrantOver;
          cutline_layer_settings.placement = QgsPalLayerSettings::OverPoint;
          break;
        case 0:
        default:
          cutline_layer_settings.placement = QgsPalLayerSettings::OrderedPositionsAroundPoint;
          cutline_layer_settings.placementFlags = QgsPalLayerSettings::AboveLine;
          break;
      }
      // cutline_layer_settings.placementFlags = QgsPalLayerSettings::AboveLine | QgsPalLayerSettings::MapOrientation;
      // Drop shadow
      cutline_layer_settings.shadowDraw = true;
      cutline_layer_settings.shadowUnder = QgsPalLayerSettings::ShadowLowest;
      cutline_layer_settings.shadowOffsetAngle = 135;
      cutline_layer_settings.shadowOffsetDist = 0;
      cutline_layer_settings.shadowOffsetUnits = QgsPalLayerSettings::MM;
      cutline_layer_settings.shadowOffsetMapUnitScale = false;
      cutline_layer_settings.shadowOffsetGlobal = true;
      cutline_layer_settings.shadowRadius = 1.50;
      cutline_layer_settings.shadowRadiusUnits = QgsPalLayerSettings::MM;
      cutline_layer_settings.shadowRadiusMapUnitScale = false;
      cutline_layer_settings.shadowRadiusAlphaOnly = false;
      cutline_layer_settings.shadowTransparency = 30.0;
      cutline_layer_settings.shadowScale = 100;
      cutline_layer_settings.shadowColor = color_shadow;
      cutline_layer_settings.shadowBlendMode = QPainter::CompositionMode_Multiply;
      // Write settings to layer
      cutline_layer_settings.writeToLayer( layer_cutline );
    }
  }
  return layer_cutline->isValid();
}
// GcpDatabase slots
void QgsGeorefPluginGui::setLegacyMode()
{
  switch ( mLegacyMode )
  {
    case 1:
      mLegacyMode = 0;
      break;
    default:
      mLegacyMode = 1;
      break;
  }
  mActionLegacyMode->setChecked( !( bool )mLegacyMode );
  createMapCanvas();
  if ( mGCPListWidget )
  {
    mGCPListWidget->setLegacyMode( mLegacyMode );
  }
}
void QgsGeorefPluginGui::setPointsPolygon()
{ // Do nothing for now
  switch ( mPointsPolygon )
  {
    case 1:
      mPointsPolygon = 0;
      break;
    default:
      mPointsPolygon = 1;
      break;
  }
  mActionPointsPolygon->setChecked(( bool )mPointsPolygon );
}
void QgsGeorefPluginGui::openGcpCoveragesDb()
{ // Do nothing for now [Dialog select Database]
  QSettings s;
  QString dir = s.value( "/Plugin-GeoReferencer/gcpdb_directory" ).toString();
  if ( dir.isEmpty() )
    dir = '.';
  // Gcp-Database (*.db);; Tiff (*.tif)
  QString otherFiles = tr( "Gcp-Database (*.gcp.db);;" );
  QString lastUsedFilter = s.value( "/Plugin-GeoReferencer/lastuseddb", otherFiles ).toString();

  QString filters; //  = QgsProviderRegistry::instance()->fileRasterFilters();
  filters.prepend( otherFiles + ";;" );
  filters.chop( otherFiles.size() + 2 );
  QString s_GcpCoverages_FileName = QFileDialog::getOpenFileName( this, tr( "Open GcpCoverages-Database" ), dir, otherFiles, &lastUsedFilter );
  // mModifiedRasterFileName = "";

  if ( s_GcpCoverages_FileName.isEmpty() )
    return;

  QFileInfo database_file( s_GcpCoverages_FileName );
  if ( database_file.exists() )
  {
    QDir parent_dir( database_file.canonicalPath() );
    s.setValue( "/Plugin-GeoReferencer/gcpdb_directory", database_file.canonicalPath() );
    s.setValue( "/Plugin-GeoReferencer/lastusedb", lastUsedFilter );
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
  QSettings s;
  QString dir = s.value( "/Plugin-GeoReferencer/gcpdb_directory" ).toString();
  if ( dir.isEmpty() )
    dir = '.';
  // Gcp-Database (*.db);; Tiff (*.tif)
  QString otherFiles = tr( "Gcp-Database (*gcp_master*.db);;" );
  QString lastUsedFilter = s.value( "/Plugin-GeoReferencer/gcp_master_db", otherFiles ).toString();

  QString filters; //  = QgsProviderRegistry::instance()->fileRasterFilters();
  filters.prepend( otherFiles + ";;" );
  filters.chop( otherFiles.size() + 2 );
  QString s_GcpMaster_FileName = QFileDialog::getOpenFileName( this, tr( "Open GcpMaster-Database" ), dir, otherFiles, &lastUsedFilter );
  // mModifiedRasterFileName = "";

  if ( s_GcpMaster_FileName.isEmpty() )
    return;

  QFileInfo database_file( s_GcpMaster_FileName );
  if ( database_file.exists() )
  {
    int i_return_srid = createGcpMasterDb( database_file.canonicalFilePath() );
    if ( i_return_srid != INT_MIN )
    { // will return srid being used in the gcp_master, otherwise INT_MIN if not found
      mGcpMasterDatabaseFileName = database_file.canonicalFilePath();
      mGcpMasterSrid = i_return_srid;
      QDir parent_dir( database_file.canonicalPath() );
      s.setValue( "/Plugin-GeoReferencer/gcpdb_directory", database_file.canonicalPath() );
      s.setValue( "/Plugin-GeoReferencer/gcp_master_db", mGcpMasterDatabaseFileName );
      loadGcpMaster( mGcpMasterDatabaseFileName ); // Will be added if group_georeferencer exists and is loadable
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
    QgsSpatiaLiteProviderGcpUtils::GcpDatabases gcp_db_type = create_gcpdb.GcpDatabaseType();
    if ( ! database_file.exists() )
    { // since the file does not exist, canonicalFilePath() cannot be used [use: absoluteFilePath()]
      switch ( gcp_db_type )
      {
        case QgsSpatiaLiteProviderGcpUtils::GcpCoverages:
        {
          qDebug() << QString( "QgsGeorefPluginGui::createGcpDatabaseDialog -1- GcpCoverages srid[%1] dump[%2] file[%3]" ).arg( i_srid ).arg( b_sqlDump ).arg( database_file.absoluteFilePath() );
          if ( createGcpCoverageDb( database_file.absoluteFilePath(), i_srid, b_sqlDump ) == i_srid )
          { // will return srid being used in the gcp_master, otherwise INT_MIN if not found
            statusBar()->showMessage( tr( "Created GcpCoverages: srid[%1] dump[%2] in %3" ).arg( i_srid ).arg( b_sqlDump ).arg( database_file.absoluteFilePath() ) );
          }
          else
          {
            statusBar()->showMessage( tr( "Creation failed: GcpCoverages: srid[%1] dump[%2] in %3" ).arg( i_srid ).arg( b_sqlDump ).arg( database_file.absoluteFilePath() ) );
          }
        }
        break;
        case QgsSpatiaLiteProviderGcpUtils::GcpMaster:
        {
          qDebug() << QString( "QgsGeorefPluginGui::createGcpDatabaseDialog -2- GcpMaster srid[%1] dump[%2] file[%3]" ).arg( i_srid ).arg( b_sqlDump ).arg( database_file.absoluteFilePath() );
          if ( createGcpMasterDb( database_file.absoluteFilePath(), i_srid, b_sqlDump ) == i_srid )
          { // will return srid being used in the gcp_master, otherwise INT_MIN if not found
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
        statusBar()->showMessage( tr( "Creation failed: GcpCoverages Sql-Dump: srid[%1] recieved[%2]in %3" ).arg( mGcpDbData->mGcpSrid ).arg( return_srid ).arg( mGcpDbData->mGcpDatabaseFileName ) );
      }
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
        statusBar()->showMessage( tr( "Creation failed: GcpMaster Sql-Dump: srid[%1], recieved[%2] in %3" ).arg( mGcpMasterSrid ).arg( return_srid ).arg( mGcpMasterDatabaseFileName ) );
      }
    }
  }
}
void QgsGeorefPluginGui::listGcpCoverages()
{
  if ( mGcpDbData )
  {
    if ( mGcpDbData->gcp_coverages.count() > 0 )
    { // Will build list of Coverages read from Gcp-Database
      QgsGcpCoverages::instance()->openDialog( this, mGcpDbData );
    }
  }
}
void QgsGeorefPluginGui::loadGcpCoverage( int id_selected_coverage )
{ // Result of Coverage selection from QgsGcpCoverages
  if ( mGcpDbData->mId_gcp_coverage != id_selected_coverage )
  { // Do not load a raster that is already loaded
    QString s_value = mGcpDbData->gcp_coverages.value( id_selected_coverage );
    QStringList sa_fields = s_value.split( mGcpDbData->mParseString );
    if ( sa_fields.count() >= 23 )
    { // 22=path ; 3=file
      QFile raster_file( QString( "%1/%2" ).arg( sa_fields.at( 22 ) ).arg( sa_fields.at( 3 ) ) );
      if ( raster_file.exists() )
      { // Do not load a raster that does not exist
        QString s_title = sa_fields.at( 5 ); // title
        if ( s_title.isEmpty() )
          s_title = sa_fields.at( 1 ); // coverage_name
        qDebug() << QString( "QgsGeorefPluginGui::loadGcpCoverage title[%1] file[%2]  " ).arg( s_title ).arg( raster_file.fileName() );
        openRaster( raster_file );
      }
    }
  }
}
// Mapcanvas QGIS
void QgsGeorefPluginGui::loadGTifInQgis( const QString& gtif_file )
{
  if ( mLoadGTifInQgis )
  {
    QFileInfo raster_file( gtif_file );
    if ( raster_file.exists() )
    {
      mLayer_gtif_raster = new QgsRasterLayer( raster_file.canonicalFilePath(), raster_file.completeBaseName() );
      if ( mLayer_gtif_raster )
      {
        if ( group_gtif_rasters )
        {  // so that layer is not added to legend
          QgsMapLayerRegistry::instance()->addMapLayers( QList<QgsMapLayer *>() << mLayer_gtif_raster, false, false );
          // must be added AFTER being registered !
          group_gtif_raster = group_gtif_rasters->addLayer( mLayer_gtif_raster );
        }
        else
        { // so that layer is added to legend
          QgsMapLayerRegistry::instance()->addMapLayers( QList<QgsMapLayer *>() << mLayer_gtif_raster, true, false );
        }
      }
    }
  }
}
bool QgsGeorefPluginGui::exportLayerDefinition( QgsLayerTreeGroup *group_layer, QString file_path )
{ // Note: this is intended more to help in debugging styling structures created in setCutlineLayerSettings and setGcpLayerSettings
  bool saved = false;
  if ( group_layer )
  {
    if (( file_path.isNull() ) || ( file_path.isEmpty() ) )
    { //  General valid file, pathnames: 0-9, a-z, A-Z, '.', '_', and '-' / "[^[:alnum:]._-[:space:]]" [replace space with '_']
      file_path = QString( "%1/%2.qlr" ).arg( mRasterFilePath ).arg( group_layer->name().replace( QRegExp( "[^[:alnum:]._-]" ), "_" ) );
    }
    QString errorMessage = QString( "" );
    saved = QgsLayerDefinition::exportLayerDefinition( file_path, group_layer->children(), errorMessage );
    QFile definition_file( file_path );
    if (( definition_file.exists() ) && ( definition_file.size() == 0 ) )
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
    { // for(int k=0, s=list_white.size(), max=(s/2); k<max; k++) list_white.swap(k,s-(1+k));
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
  for ( int i = 0,  loop = 0;i < i_max_colors;loop++ )
  {
    if ( loop >  i_max_colors )
    { // should never happen, but just in case ...
      break;
    }
    if (( i_max_colors_pink >= 0 ) && ( i_max_colors_pink < list_pink.size() ) )
    {
      // qDebug() << QString( "SvgColors[%1,%4] pink[%2,%5] [%3] " ).arg( i ).arg( i_max_colors_pink ).arg( list_pink.at( i_max_colors_pink ) ).arg( i_max_colors ).arg( list_pink.size() - 1 );
      list_SvgColors.append( list_pink.at( i_max_colors_pink ) );
      i_max_colors_pink += i_entry_addition;
      i++;
    }
    if (( i_max_colors_red >= 0 ) && ( i_max_colors_red < list_red.size() ) )
    {
      list_SvgColors.append( list_red.at( i_max_colors_red ) );
      i_max_colors_red += i_entry_addition;
      i++;
    }
    if (( i_max_colors_orange >= 0 ) && ( i_max_colors_orange < list_orange.size() ) )
    {
      list_SvgColors.append( list_orange.at( i_max_colors_orange ) );
      i_max_colors_orange += i_entry_addition;
      i++;
    }
    if (( i_max_colors_yellow >= 0 ) && ( i_max_colors_yellow < list_yellow.size() ) )
    {
      list_SvgColors.append( list_yellow.at( i_max_colors_yellow ) );
      i_max_colors_yellow += i_entry_addition;
      i++;
    }
    if (( i_max_colors_brown >= 0 ) && ( i_max_colors_brown < list_brown.size() ) )
    {
      list_SvgColors.append( list_brown.at( i_max_colors_brown ) );
      i_max_colors_brown += i_entry_addition;
      i++;
    }
    if (( i_max_colors_green >= 0 ) && ( i_max_colors_green < list_green.size() ) )
    {
      list_SvgColors.append( list_green.at( i_max_colors_green ) );
      i_max_colors_green += i_entry_addition;
      i++;
    }
    if (( i_max_colors_cyan >= 0 ) && ( i_max_colors_cyan < list_cyan.size() ) )
    {
      list_SvgColors.append( list_cyan.at( i_max_colors_cyan ) );
      i_max_colors_cyan += i_entry_addition;
      i++;
    }
    if (( i_max_colors_blue >= 0 ) && ( i_max_colors_blue < list_blue.size() ) )
    {
      list_SvgColors.append( list_blue.at( i_max_colors_blue ) );
      i_max_colors_blue += i_entry_addition;
      i++;
    }
    if (( i_max_colors_purple >= 0 ) && ( i_max_colors_purple < list_purple.size() ) )
    {
      list_SvgColors.append( list_purple.at( i_max_colors_purple ) );
      i_max_colors_purple += i_entry_addition;
      i++;
    }
    if (( i_max_colors_white >= 0 ) && ( i_max_colors_white < list_white.size() ) )
    {
      list_SvgColors.append( list_white.at( i_max_colors_white ) );
      i_max_colors_white += i_entry_addition;
      i++;
    }
    if (( i_max_colors_gray_black >= 0 ) && ( i_max_colors_gray_black < list_gray_black.size() ) )
    {
      list_SvgColors.append( list_gray_black.at( i_max_colors_gray_black ) );
      i_max_colors_gray_black += i_entry_addition;
      i++;
    }
  }
  // -------
  return list_SvgColors;
}
void QgsGeorefPluginGui::jumpToGcpConvert( QgsPoint input_point, bool b_toPixel )
{
  QgsRectangle canvas_extent;
  if ( b_toPixel )
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
  QgsPoint canvas_center = canvas_extent.center();
  QgsPoint canvas_diff( input_point.x() - canvas_center.x(), input_point.y() - canvas_center.y() );
  QgsRectangle canvas_extent_new( canvas_extent.xMinimum() + canvas_diff.x(), canvas_extent.yMinimum() + canvas_diff.y(),
                                  canvas_extent.xMaximum() + canvas_diff.x(), canvas_extent.yMaximum() + canvas_diff.y() );
  mExtentsChangedRecursionGuard = true;
  if ( b_toPixel )
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
  if (( gcp_layer ) && ( !gcp_layer->isValid() ) )
  { // If not valid can cause a crash of the application
    qDebug() << QString( "-W-> QgsGeorefPluginGui::saveEditsGcp: layer_name[%1] Invalid error[%2]" ).arg( gcp_layer->name() ).arg( gcp_layer->error().message( QgsErrorMessage::Text ) );
  }
#endif
  QStringList sa_errors;
  if (( gcp_layer ) && ( gcp_layer->isValid() ) && ( gcp_layer->isEditable() ) && ( gcp_layer->editBuffer()->isModified() ) )
  { //  [When used with 'gcp_layer->commitChanges' instead of 'editBuffer()->commitChanges', caused crashes] Warning: QUndoStack::endMacro(): no matching beginMacro()
    b_rc = gcp_layer->editBuffer()->commitChanges( sa_errors );
  }
  else
  {
    if (( gcp_layer ) && ( gcp_layer->isValid() ) && ( gcp_layer->isEditable() ) && ( !gcp_layer->isModified() ) )
    {  // [Hack] After a gcp_layer->addFeature(..) in fetchGcpMasterPointsExtent: isModified() returns false, which is not true
      if ( leaveEditable )
      { // - so we will force a commit when we are NOT shutting down
        b_rc = gcp_layer->editBuffer()->commitChanges( sa_errors );
      }
    }
  }
  if ( b_rc )
  {
    if ( !leaveEditable )
    {
      gcp_layer->commitChanges();
      gcp_layer->endEditCommand();
      // qDebug() << QString( "-I-> QgsGeorefPluginGui::saveEditsGcp[%2,%3]: layer_name[%1] isModified[%4]" ).arg( gcp_layer->name() ).arg( "After endEditCommand" ).arg( b_rc ).arg( gcp_layer->isModified() );
    }
    // without 'triggerRepaint' crashes occur
    gcp_layer->triggerRepaint();
  }
  else
  {
    for ( int i = 0;i < sa_errors.size();i++ )
    {
      qDebug() << QString( "-E-> QgsGeorefPluginGui::saveEditsGcp[%1] error[%2]" ).arg( gcp_layer->name() ).arg( sa_errors.at( i ) );
    }
  }
  return b_rc;
}
void QgsGeorefPluginGui::featureAdded_gcp( QgsFeatureId fid )
{
  if (( layer_gcp_points ) && ( layer_gcp_pixels ) )
  { // Default assumtion: a map-point has been added and the pixel-point must be updated
    bool b_toPixel = true;
    QString s_Event = "Map-Point to Pixel-Point";
    QgsVectorLayer *layer_gcp_event = layer_gcp_points;
    QgsVectorLayer *layer_gcp_update = layer_gcp_pixels;
    if ( layer_gcp_update->editBuffer()->isModified() )
    { // A pixel-point has been added and the map-point must be updated, switch pointers
      b_toPixel = false;
      s_Event = "Pixel-Point to Map-Point";
      layer_gcp_event = layer_gcp_pixels;
      layer_gcp_update = layer_gcp_points;
    }
    if ( fid < 0 )
    { //  fid is negative for new features
      if ( b_toPixel )
        mEvent_point_status = 1;
      else
        mEvent_point_status = 2;
      QgsFeatureMap& addedFeatures = const_cast<QgsFeatureMap&>( layer_gcp_event->editBuffer()->addedFeatures() );
      if ( addedFeatures[fid].attribute( QString( "id_gcp_coverage" ) ).toInt() <= 0 )
      { // do this only when the id_gcp_coverage is not known (importing from Gcp-Master) [default values are allready set]
        addedFeatures[fid].setAttribute( "name", mGcpBaseFileName );
        addedFeatures[fid].setAttribute( "id_gcp_coverage", mId_gcp_coverage );
        addedFeatures[fid].setAttribute( "order_selected", getGcpTransformParam( mTransformParam ) );
      }
      mEvent_gcp_status = fid;
      if ( saveEditsGcp( layer_gcp_event, true ) )
      { // Note: during the commitChanges, this function will run again with a positive 'fid', which will be stored in 'mEvent_gcp_status'
        QgsFeature fet_point_event;
        QgsAttributeList lst_needed_fields;
        lst_needed_fields.append( 0 ); // id_gcp
        if ( layer_gcp_event->getFeatures( QgsFeatureRequest( mEvent_gcp_status ).setSubsetOfAttributes( lst_needed_fields ) ).nextFeature( fet_point_event ) )
        { // Retrieve the newly added record and extract the primary key
          QgsPoint added_point_event = fet_point_event.geometry()->asPoint();
          int id_gcp = fet_point_event.attribute( QString( "id_gcp" ) ).toInt();
          // qDebug() << QString( "QgsGeorefPluginGui::featureAdded_gcp[%1]  mEvent_point_status[%2] id_gcp[%3]" ).arg( fid ).arg( mEvent_point_status ).arg( id_gcp );
          if ( id_gcp > 0 )
          { // Both geometries are in the same table
            QgsFeature fet_point_update;
            QgsFeatureRequest update_request = QgsFeatureRequest().setFilterExpression( QString( "id_gcp=%1" ).arg( id_gcp ) );
            if ( layer_gcp_update->getFeatures( update_request ).nextFeature( fet_point_update ) )
            { // Retrieve the (now added) other-geometry which by default is 0 0
              QgsPoint map_point;
              QgsPoint pixel_point;
              QgsPoint added_point_update = getGcpConvert( mGcp_coverage_name, added_point_event, b_toPixel, mTransformParam );
              if ( b_toPixel )
              {
                pixel_point = added_point_update;
                map_point = added_point_event;
              }
              else
              {
                pixel_point = added_point_event;
                map_point = added_point_update;
              }
              QString s_map_points = QString( "pixel[%1] map[%2]" ).arg( pixel_point.wellKnownText() ).arg( map_point.wellKnownText() );
              s_Event = QString( "%1 id_gcp[%2] - %3[%4]" ).arg( s_Event ).arg( id_gcp ).arg( "Adding Point" ).arg( s_map_points );
              // qDebug() << QString( "QgsGeorefPluginGui::featureAdded_gcp[%1] -zz- after commit update_fid[%2]  event_modified[%3] update_modified[%4]" ).arg( s_Event ).arg( fet_point_update.id() ).arg( layer_gcp_event->isModified() ).arg( layer_gcp_update->isModified() );
              if ( fet_point_update.geometry()->asPoint() != added_point_update )
              { // We  have a usable result
                QgsGeometry* geometry_update = QgsGeometry::fromPoint( added_point_update );
                fet_point_update.setGeometry( geometry_update );
                if ( layer_gcp_update->updateFeature( fet_point_update ) )
                {
                  if ( saveEditsGcp( layer_gcp_update, true ) )
                  {  // ?? isModified() returning 0 for both, but when applications end request to save pixels
                    mEvent_point_status = 0;
                    // Update 'layer_mercator_polygons_transform' if any 'layer_mercator_polygons' exist
                    updateGcpTranslate( mGcp_coverage_name );
                  }
                  else
                  { // an error has accured
                    id_gcp = -2;
                    s_Event = QString( "-E-> QgsGeorefPluginGui::featureAdded_gcp -gcp_update- saveEditsGcp [%1}" ).arg( s_Event );
                    qDebug() << s_Event << layer_gcp_update->commitErrors();
                  }
                }
                // qDebug() << QString( "QgsGeorefPluginGui::featureAdded_gcp[%1] -zz- after commit update_fid[%2]  event_modified[%3] update_modified[%4]" ).arg( s_Event ).arg( fet_point_update.id() ).arg( layer_gcp_event->isModified() ).arg( layer_gcp_update->isModified() );
                mEvent_gcp_status = 0;
                mEvent_point_status = 0;
                QgsGeorefDataPoint* data_point = new QgsGeorefDataPoint( mCanvas, mIface->mapCanvas(), pixel_point, map_point, mGcpSrid, id_gcp, 1, mLegacyMode );
                mGCPListWidget->addDataPoint( data_point );
                connect( mCanvas, SIGNAL( extentsChanged() ), data_point, SLOT( updateCoords() ) );
                updateGeorefTransform();
                jumpToGcpConvert( added_point_update, b_toPixel );
              }
              else
              { // call dialog to set point
                mEvent_gcp_status = id_gcp;
                if ( b_toPixel )
                { // Map-Point has been created, enter a Pixel-Point
                  mMapCoordsDialog = new QgsMapCoordsDialog( mCanvas, map_point, id_gcp, this, mLegacyMode );
                }
                else
                { // Pixel-Point has been created, enter a Map-Point
                  mMapCoordsDialog = new QgsMapCoordsDialog( mIface->mapCanvas(), pixel_point, id_gcp , this, mLegacyMode );
                }
                mEvent_point_status = 0;
                qDebug() << QString( "QgsGeorefPluginGui::featureAdded_gcp[%1] -calling dialog- update_fid[%2]  event_modified[%3] update_modified[%4]" ).arg( s_Event ).arg( mEvent_gcp_status ).arg( layer_gcp_event->isModified() ).arg( layer_gcp_update->isModified() );
                connect( mMapCoordsDialog, SIGNAL( pointAdded( const QgsPoint &, const QgsPoint &  , const int &, const bool & ) ), this, SLOT( addPoint( const QgsPoint &, const QgsPoint &  , const int &, const bool & ) ) );
                mMapCoordsDialog->show();
              }
            }
            else
            {
              qDebug() << QString( "QgsGeorefPluginGui::featureAdded_gcp[%1]  -layer_gcp_update- could not fetch feature mEvent_point_status[%2] id_gcp[%3]" ).arg( fid ).arg( mEvent_point_status ).arg( id_gcp );
              qDebug() << s_Event << layer_gcp_event->commitErrors();
            }
          }
        }
      }
      else
      { // an error has accured
        s_Event = QString( "-E-> QgsGeorefPluginGui::featureAdded_gcp -gcp_event- saveEditsGcp[%1}" ).arg( s_Event );
        qDebug() << s_Event << layer_gcp_event->commitErrors();
      }
      mEvent_gcp_status = 0;
      mEvent_point_status = 0;
      return;
    }
    if ( fid > 0 )
    { //  fid, when positive, will be returned during commitChanges for the new feature
      mEvent_gcp_status = fid;
      return;
    }
  }
}
void QgsGeorefPluginGui::geometryChanged_gcp( QgsFeatureId fid, QgsGeometry& changed_geometry )
{
  if ( mEvent_point_status > 0 )
    return;
  if (( layer_gcp_points ) && ( layer_gcp_pixels ) )
  { // Default assumtion: a map-point has been added and the pixel-point must be updated
    bool b_toPixel = true;
    QString s_Event = "Map-Point to Pixel-Point";
    QgsVectorLayer *layer_gcp_event = layer_gcp_points;
    QgsVectorLayer *layer_gcp_update = layer_gcp_pixels;
    // int i_features_delete_event = layer_gcp_event->editBuffer()->deletedFeatureIds().size();
    // int i_features_delete_update = layer_gcp_update->editBuffer()->deletedFeatureIds().size();
    // qDebug() << QString( "QgsGeorefPluginGui::geometryChanged_gcp[%1] -1- delete event[%2] update[%3]" ).arg( fid ).arg( i_features_delete_event ).arg( i_features_delete_update );
    if ( layer_gcp_update->editBuffer()->changedGeometries().size() > 0 )
    { // A pixel-point has been changed and the map-point must be updated, switch pointers
      b_toPixel = false;
      s_Event = "Pixel-Point to Map-Point";
      layer_gcp_event = layer_gcp_pixels;
      layer_gcp_update = layer_gcp_points;
    }
    int i_features_changed_event = layer_gcp_event->editBuffer()->changedGeometries().size();
    //qDebug() << QString( "QgsGeorefPluginGui::geometryChanged_gcp[%1] -1- changedGeometries().size[%2] b_toPixel/type[%3,%4]" ).arg( fid ).arg( i_features_changed_event ).arg( b_toPixel ).arg( s_Event );
    if ( fid > 0 )
    { //  fid, when positive, will be returned during commitChanges for the new feature
      mEvent_gcp_status = fid;
      int id_gcp = -1;
      QgsAttributeList lst_needed_fields;
      lst_needed_fields.append( 0 ); // id_gcp
      QgsFeature fet_point_event;
      // QgsFeatureRequest request_point_event( fid );
      QgsFeatureRequest request_point_event;
      // request_point_event.setFlags( QgsFeatureRequest::NoGeometry );
      request_point_event.setSubsetOfAttributes( lst_needed_fields );
      // if ( layer_gcp_event->getFeatures( QgsFeatureRequest( QgsFeatureId(mEvent_gcp_status) ).setSubsetOfAttributes( lst_needed_fields ) ).nextFeature( fet_point_event ) )
      QgsFeatureIterator it = layer_gcp_event->getFeatures( request_point_event );
#if 1
      // int i_count = 0;
      while ( it.nextFeature( fet_point_event ) )
      {
        /*
        i_count++;
        QString output_line = QString( "-I-%1-> f_id[%2]" ).arg( i_count ).arg( fet_point_event.id() );
        for ( int j = 0;j < fet_point_event.fields()->size();j++ )
        {
          output_line += QString( " ; %1[%2]" ).arg( fet_point_event.fields()->at( j ).name() ).arg( fet_point_event.attribute( j ).toString() );
        }
        qDebug() << output_line << "\n";
         */
        if ( fet_point_event.id() ==  fid )
          break;
      }
      it.rewind();
      if ( fet_point_event.id() ==  fid )
#else
      if ( layer_gcp_event->getFeatures( request_point_event ).nextFeature( fet_point_event ) )
#endif
      {
        if ( fet_point_event.isValid() )
        {
          id_gcp = fet_point_event.attribute( QString( "id_gcp" ) ).toInt();
          if ( id_gcp > 0 )
          {
            int i_updateGCPList = 0;
            if ( changed_geometry.type() == QGis::Point )
            {  // The Point has being moved [TODO: check if it has really moved, if possible]
              s_Event = QString( "%1 id_gcp[%2] - %3[%4]" ).arg( s_Event ).arg( id_gcp ).arg( "Moving Point" ).arg( changed_geometry.exportToWkt() );
              i_updateGCPList = 1;
            }
            else
            { // The Point is being deleted with the Node-MapTool
              s_Event = QString( "%1 id_gcp[%2] - %3[%4]" ).arg( s_Event ).arg( id_gcp ).arg( "Deleting Point" ).arg( changed_geometry.exportToWkt() );
              layer_gcp_event->deleteFeature( fid );
              i_updateGCPList = 2;
            }
            if ( i_updateGCPList > 0 )
            { // Common task for both update and delete
#if 1
              // TODO: 20161220: still causes crashes
              if ( saveEditsGcp( layer_gcp_event, true ) )
              { // Commited and continue editing, with triggerRepaint()
                saveEditsGcp( layer_gcp_update, true ); // will save only isModified
              }
              else
              { // an error has accured
                id_gcp = -1;
                s_Event = QString( "-E-> QgsGeorefPluginGui::geometryChanged_gcp saveEditsGcp[%1] %2" ).arg( i_features_changed_event ).arg( s_Event );
                qDebug() << s_Event << layer_gcp_event->commitErrors();
              }
#endif
              switch ( i_updateGCPList )
              {
                case 1:
                  if ( ! mGCPListWidget->updateDataPoint( id_gcp, b_toPixel, changed_geometry.asPoint() ) )
                  {
                    i_updateGCPList = 0;
                  }
                  break;
                case 2:
                  if ( !mGCPListWidget->removeDataPoint( id_gcp ) )
                  {
                    i_updateGCPList = 0;
                  }
                  else
                  {
                    if ( b_toPixel )
                    { // Map-Point has been deleted, refresh the Georef Map so that point will also be removed
                      mCanvas->refresh();
                    }
                    else
                    { // Pixel-Point has been deleted, refresh the QGIS Map so that point will also be removed
                      mIface->mapCanvas()->refresh();
                    }
                  }
                  break;
              }
              qDebug() <<  QString( "%1 id_gcp[%2] - %3[%4]" ).arg( s_Event ).arg( id_gcp ).arg( s_Event ).arg( changed_geometry.exportToWkt() );
              if ( i_updateGCPList > 0 )
              { // 'updateGCPList' will rebuild the list and recalulate the 'residual' values.
                updateGeorefTransform();
              }
            }
            qDebug() << QString( "QgsGeorefPluginGui::geometryChanged_gcp[%1]\n\t\t -zz- after commit fid[%2] event_modified[%3] update_modified[%4]" ).arg( s_Event ).arg( mEvent_gcp_status ).arg( layer_gcp_event->isModified() ).arg( layer_gcp_update->isModified() );
            // Warning: Object::connect: No such slot QObject::addError( QgsGeometry::Error )
            // Warning: Object::connect: No such slot QObject::validationFinished()
            // Warning: QUndoStack::endMacro(): no matching beginMacro()
          }
          else
          {
            QString output_line = QString( "-I-> f_id[%1]" ).arg( fet_point_event.id() );
            for ( int j = 0;j < fet_point_event.fields()->size();j++ )
            {
              output_line += QString( " ; %1[%2]" ).arg( fet_point_event.fields()->at( j ).name() ).arg( fet_point_event.attribute( j ).toString() );
            }
            qDebug() << output_line << "\n";
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
  if (( !layer_cutline_event ) && ( layer_cutline_points ) && ( layer_cutline_points->isModified() ) )
  {
    layer_cutline_event = layer_cutline_points;
    i_cutline_type = 100;
  }
  if (( !layer_cutline_event ) && ( layer_cutline_linestrings ) && ( layer_cutline_linestrings->isModified() ) )
  {
    layer_cutline_event = layer_cutline_linestrings;
    i_cutline_type = 101;
  }
  if (( !layer_cutline_event ) && ( layer_cutline_polygons ) && ( layer_cutline_polygons->isModified() ) )
  {
    layer_cutline_event = layer_cutline_polygons;
    i_cutline_type = 102;
  }
  if (( !layer_cutline_event ) && ( layer_mercator_polygons ) && ( layer_mercator_polygons->isModified() ) )
  {
    layer_cutline_event = layer_mercator_polygons;
    i_cutline_type = 77; // to be used as a cutline
  }
  if (( !layer_cutline_event ) && ( layer_gcp_master ) && ( layer_gcp_master->isModified() ) )
  { // gcp_master does not belong to the cutline groupe, save asis
    if ( saveEditsGcp( layer_gcp_master, true ) )
    { // Commited and continue editing, with triggerRepaint()
      qDebug() << QString( "QgsGeorefPluginGui::featureAdded_cutline[%1]  featureAdded[%3]" ).arg( fid ).arg( mGcp_master_layername );
    }
  }
  if ( layer_cutline_event )
  {
    QgsFeatureMap& addedFeatures = const_cast<QgsFeatureMap&>( layer_cutline_event->editBuffer()->addedFeatures() );
    addedFeatures[fid].setAttribute( "id_gcp_coverage", mId_gcp_coverage );
    addedFeatures[fid].setAttribute( "cutline_type",  i_cutline_type );
    addedFeatures[fid].setAttribute( "belongs_to_01",  mGcpBaseFileName );
    addedFeatures[fid].setAttribute( "belongs_to_02", mRasterFileName );
    if ( saveEditsGcp( layer_cutline_event, true ) )
    { // Commited and continue editing, with triggerRepaint()
      qDebug() << QString( "QgsGeorefPluginGui::featureAdded_cutline[%1]  featureAdded[%3]" ).arg( fid ).arg( mGcpBaseFileName );
    }
  }
}
void QgsGeorefPluginGui::geometryChanged_cutline( QgsFeatureId fid, QgsGeometry& changed_geometry )
{
  Q_UNUSED( fid );
  Q_UNUSED( changed_geometry );
  QgsVectorLayer *layer_cutline_event = nullptr;
  if (( !layer_cutline_event ) && ( layer_cutline_points ) && ( layer_cutline_points->editBuffer()->changedGeometries().size() > 0 ) )
  {
    layer_cutline_event = layer_cutline_points;
  }
  if (( !layer_cutline_event ) && ( layer_cutline_linestrings ) && ( layer_cutline_linestrings->editBuffer()->changedGeometries().size() > 0 ) )
  {
    layer_cutline_event = layer_cutline_linestrings;
  }
  if (( !layer_cutline_event ) && ( layer_cutline_polygons ) && ( layer_cutline_polygons->editBuffer()->changedGeometries().size() > 0 ) )
  {
    layer_cutline_event = layer_cutline_polygons;
  }
  if (( !layer_cutline_event ) && ( layer_mercator_polygons ) && ( layer_mercator_polygons->editBuffer()->changedGeometries().size() > 0 ) )
  {
    layer_cutline_event = layer_mercator_polygons;
  }
  if (( !layer_cutline_event ) && ( layer_gcp_master ) && ( layer_gcp_master->editBuffer()->changedGeometries().size() > 0 ) )
  { // gcp_master does not belong to the cutline group, save anyway
    layer_cutline_event = layer_gcp_master;
  }
  if ( layer_cutline_event )
  { // To delete a point must be selected
    if ( saveEditsGcp( layer_cutline_event, true ) )
    { // Commited and continue editing, with triggerRepaint()
      // qDebug() << QString( "QgsGeorefPluginGui::geometryChanged_cutline[%1]  changed_geometry[%2]" ).arg( fid ).arg( changed_geometry.exportToWkt() );
    }
  }
  else
  {
    qDebug() << QString( "QgsGeorefPluginGui::geometryChanged_cutline[%1]  unresolved changed_geometry[%2]" ).arg( fid ).arg( changed_geometry.exportToWkt() );
  }
}
//-----------------------------------------------------------
//  Wrappers for QgsSpatiaLiteProviderGcpUtils functions
//-----------------------------------------------------------
bool QgsGeorefPluginGui::readGcpDb( QString  s_database_filename, bool b_DatabaseDump )
{
  bool b_rc = false;
  if ( mLegacyMode == 0 )
    return b_rc;
  QgsSpatiaLiteProviderGcpUtils::GcpDbData *parms_GcpDbData = nullptr;
  parms_GcpDbData = new QgsSpatiaLiteProviderGcpUtils::GcpDbData( s_database_filename, QString::null, INT_MIN );
  if ( QgsSpatiaLiteProviderGcpUtils::createGcpDb( parms_GcpDbData ) )
  {
    if ( parms_GcpDbData->mDatabaseValid )
    {
      if ( parms_GcpDbData->gcp_coverages.size() > 0 )
      {
        qDebug() << QString( "QgsGeorefPluginGui::readGcpDb -replacing GcpDbData-pointer- old[%1] new[%2]" ).arg( QString().sprintf( "%8p", mGcpDbData ) ).arg( QString().sprintf( "%8p", parms_GcpDbData ) );
        mGcpDbData = parms_GcpDbData;
        mGcp_coverages = mGcpDbData->gcp_coverages;
        mGcpDatabaseFileName = mGcpDbData->mGcpDatabaseFileName;
        mListGcpCoverages->setEnabled( true );
        connect( QgsGcpCoverages::instance(), SIGNAL( loadRasterCoverage( int ) ), this, SLOT( loadGcpCoverage( int ) ) );
        if ( b_DatabaseDump )
        {
          parms_GcpDbData->mSqlDump = true;
          QgsSpatiaLiteProviderGcpUtils::createGcpDb( parms_GcpDbData );
        }
        b_rc = true;
      }
      else
      {
        qDebug() << QString( "QgsGeorefPluginGui::readGcpDb - no Gcp-Coverages found - db[%1] error[%2]" ).arg( parms_GcpDbData->mGcpDatabaseFileName ).arg( parms_GcpDbData->mError );
      }
    }
    else
    {
      qDebug() << QString( "QgsGeorefPluginGui:readGcpDb -  Database considered invalid - db[%1] error[%2]" ).arg( parms_GcpDbData->mGcpDatabaseFileName ).arg( parms_GcpDbData->mError );
    }
  }
  else
  {
    qDebug() << QString( "QgsGeorefPluginGui::readGcpDb - open of Database failed - db[%1] error[%2]" ).arg( parms_GcpDbData->mGcpDatabaseFileName ).arg( parms_GcpDbData->mError );
  }
  parms_GcpDbData = nullptr;
  return b_rc;
}
bool QgsGeorefPluginGui::createGcpDb( bool b_DatabaseDump )
{
  if ( mLegacyMode == 0 )
    return false;
  QFileInfo raster_file( mRasterFileName );
  mRasterFilePath = raster_file.canonicalPath(); // Path to the file, without file-name
  QString s_RasterFileName = raster_file.fileName(); // file-name without path
  mGcp_coverage_name_base = raster_file.completeBaseName(); // file without extention
  mGcp_coverage_name = mGcp_coverage_name_base.toLower(); // file without extention (Lower-Case)
  QStringList sa_list_id_fields = mGcp_coverage_name_base.split( "." );
  if ( sa_list_id_fields.size() > 2 )
  { // Possilbly a format is being used: 'year.name.scale.*': 1986.n-33-123-b-c-2_30.5000
    QString s_year = sa_list_id_fields[0];
    QString s_scale = sa_list_id_fields[2];
    bool ok;
    int i_year = sa_list_id_fields[0].toInt( &ok, 10 );
    if (( ok ) && ( sa_list_id_fields[0].length() == 4 ) )
    {
      int i_scale = sa_list_id_fields[2].toInt( &ok, 10 );
      if ( ok )
      {
        mRasterScale = i_scale;
        mRasterYear = i_year;
        mGcp_coverage_name_base = sa_list_id_fields[1];
        QString s_build = QString( "%1.%2.%3" ).arg( QString( "%1" ).arg( mRasterYear, 4, 10, QChar( '0' ) ) ).arg( mGcp_coverage_name_base ).arg( mRasterScale );
        if ( mGcpSrid > 0 )
        { // give this a default name
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
    mGcpDbData = new QgsSpatiaLiteProviderGcpUtils::GcpDbData( mGcpDatabaseFileName, mGcp_coverage_name, mGcpSrid, mGcp_points_table_name, mLayer, mSpatialite_gcp_enabled );
  }
  mGcpDbData->mId_gcp_coverage = mId_gcp_coverage;
  mGcpDbData->mId_gcp_cutline = mId_gcp_cutline;
  mGcpDbData->mTransformParam = getGcpTransformParam( mTransformParam );
  mGcpDbData->mResamplingMethod = getGcpResamplingMethod( mResamplingMethod );
  mGcpDbData->mCompressionMethod = mCompressionMethod;
  mGcpDbData->mGcpPointsFileName = mGcpPointsFileName;
  mGcpDbData->mGcpBaseFileName = mGcpBaseFileName;
  mGcpDbData->mRasterFilePath = mRasterFilePath;
  mGcpDbData->mRasterFileName = s_RasterFileName;
  mGcpDbData->mGcp_coverage_name = mGcp_coverage_name;
  mGcpDbData->mGcp_coverage_name_base = mGcp_coverage_name_base;
  mGcpDbData->mModifiedRasterFileName = mModifiedRasterFileName;
  mGcpDbData->mRasterYear = mRasterYear;
  mGcpDbData->mRasterScale = mRasterScale;
  mGcpDbData->mRasterNodata = mRasterNodata;
  if ( b_DatabaseDump )
    mGcpDbData->mDatabaseDump = true;
  if ( QgsSpatiaLiteProviderGcpUtils::createGcpDb( mGcpDbData ) )
  {
    if ( mGcpDbData->mDatabaseValid )
    {
      mGcpSrid = mGcpDbData->mGcpSrid;
      mGcp_points_table_name = mGcpDbData->mGcp_points_table_name;
      mSpatialite_gcp_enabled = mGcpDbData->mGcp_enabled;
      mId_gcp_coverage = mGcpDbData->mId_gcp_coverage;
      mId_gcp_cutline = mGcpDbData->mId_gcp_cutline;
      mTransformParam = setGcpTransformParam( mGcpDbData->mTransformParam );
      mCompressionMethod = mGcpDbData->mCompressionMethod;
      mResamplingMethod = setGcpResamplingMethod( mGcpDbData->mResamplingMethod );
      mModifiedRasterFileName = mGcpDbData->mModifiedRasterFileName;
      mRasterNodata = mGcpDbData->mRasterNodata;
      mError = mGcpDbData->mError;
      if ( mGcpDbData->gcp_coverages.size() > 0 )
      {
        mGcp_coverages = mGcpDbData->gcp_coverages;
        mListGcpCoverages->setEnabled( true );
        connect( QgsGcpCoverages::instance(), SIGNAL( loadRasterCoverage( int ) ), this, SLOT( loadGcpCoverage( int ) ) );
      }
      else
      {
        qDebug() << QString( "QgsGeorefPluginGui::createGcpDb  - Gcp-Coverage[%3] not found - db[%1] error[%2]" ).arg( mGcpDbData->mGcpDatabaseFileName ).arg( mGcpDbData->mError ).arg( mGcp_coverage_name );
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
bool QgsGeorefPluginGui::updateGcpDb( QString s_coverage_name )
{
  if ( mLegacyMode == 0 )
    return false;
  if ( !mGcpDbData )
  {
    mGcpDbData = new QgsSpatiaLiteProviderGcpUtils::GcpDbData( mGcpDatabaseFileName, mGcp_coverage_name, mGcpSrid, mGcp_points_table_name, mLayer, mSpatialite_gcp_enabled );
  }
  mGcpDbData->mGcp_coverage_name = s_coverage_name;
  if ( QgsSpatiaLiteProviderGcpUtils::updateGcpDb( mGcpDbData ) )
  {
    mError = mGcpDbData->mError;
    mModifiedRasterFileName = mGcpDbData->mModifiedRasterFileName;
    return true;
  }
  mError = mGcpDbData->mError;
  return false;
}
QgsPoint QgsGeorefPluginGui::getGcpConvert( QString s_coverage_name, QgsPoint input_point, bool b_toPixel, int i_order, bool b_reCompute, int id_gcp )
{
  QgsPoint convert_point( 0.0, 0.0 );
  if (( mLegacyMode == 0 ) || ( ! isGcpEnabled() ) || ( isGcpOff() ) )
  { // Spatialite-Gcp-Logic is not available or has been turned off by the User [also LegacyMode==0]
    return convert_point;
  }
  if ( !mGcpDbData )
  { // mGcpDatabaseFileName
    mGcpDbData = new QgsSpatiaLiteProviderGcpUtils::GcpDbData( mGcpDatabaseFileName, mGcp_coverage_name, mGcpSrid, mGcp_points_table_name, mLayer, mSpatialite_gcp_enabled );
  }
  mGcpDbData->mGcp_coverage_name = s_coverage_name;
  mGcpDbData->mInputPoint = input_point;
  mGcpDbData->mToPixel = b_toPixel;
  mGcpDbData->mOrder = i_order;
  mGcpDbData->mReCompute = b_reCompute;
  mGcpDbData->mIdGcp = id_gcp;
  mError = mGcpDbData->mError = "";
  convert_point = QgsSpatiaLiteProviderGcpUtils::getGcpConvert( mGcpDbData );
  mError = mGcpDbData->mError;
  return convert_point;
}
bool QgsGeorefPluginGui::updateGcpCompute( QString s_coverage_name )
{
  if (( mLegacyMode == 0 ) || ( ! isGcpEnabled() ) || ( ! mLayer ) )
  { // Preconditions
    return false;
  }
  if ( !mGcpDbData )
  {
    mGcpDbData = new QgsSpatiaLiteProviderGcpUtils::GcpDbData( mGcpDatabaseFileName, mGcp_coverage_name, mGcpSrid, mGcp_points_table_name, mLayer, mSpatialite_gcp_enabled );
  }
  mGcpDbData->mGcp_coverage_name = s_coverage_name;
  mError = mGcpDbData->mError = "";
  if ( QgsSpatiaLiteProviderGcpUtils::updateGcpCompute( mGcpDbData ) )
  {
    mError = mGcpDbData->mError;
    return true;
  }
  mError = mGcpDbData->mError;
  return false;
}
bool QgsGeorefPluginGui::updateGcpTranslate( QString s_coverage_name )
{
  if (( mLegacyMode == 0 ) || ( ! isGcpEnabled() ) || ( ! mLayer ) )
  { // Preconditions
    return false;
  }
  if ( !mGcpDbData )
  {
    mGcpDbData = new QgsSpatiaLiteProviderGcpUtils::GcpDbData( mGcpDatabaseFileName, mGcp_coverage_name, mGcpSrid, mGcp_points_table_name, mLayer, mSpatialite_gcp_enabled );
  }
  mGcpDbData->mGcp_coverage_name = s_coverage_name;
  mError = mGcpDbData->mError = "";
  if ( QgsSpatiaLiteProviderGcpUtils::updateGcpTranslate( mGcpDbData ) )
  {
    mError = mGcpDbData->mError;
    return true;
  }
  mError = mGcpDbData->mError;
  return false;
}
int QgsGeorefPluginGui::createGcpMasterDb( QString  s_database_filename,  int i_srid, bool b_dump )
{
  int return_srid = INT_MIN; // Invalid
  if ( s_database_filename.isNull() )
  { // Create default Databas-File name, when not given
    if ( mRasterFilePath.isNull() )
    {
      return return_srid;
    }
    s_database_filename = QString( "%1/%2.db" ).arg( mRasterFilePath ).arg( "gcp_master.db" );
  }
  QgsSpatiaLiteProviderGcpUtils::GcpDbData *parms_GcpDbData = nullptr;
  parms_GcpDbData = new QgsSpatiaLiteProviderGcpUtils::GcpDbData( s_database_filename,  QString::null, i_srid );
  parms_GcpDbData->mDatabaseDump = b_dump;
  if ( QgsSpatiaLiteProviderGcpUtils::createGcpMasterDb( parms_GcpDbData ) )
  {
    if ( parms_GcpDbData->mDatabaseValid )
    {
      return_srid = parms_GcpDbData->mGcpSrid;
    }
    else
    {
      qDebug() << QString( "QgsGeorefPluginGui:createGcpCoverageDb -  Database considered invalid-  db[%1] error[%2]" ).arg( parms_GcpDbData->mGcpDatabaseFileName ).arg( parms_GcpDbData->mError );
    }
  }
  else
  {
    qDebug() << QString( "QgsGeorefPluginGui::createGcpMasterDb - creation of Database failed - db[%1] error[%2]" ).arg( parms_GcpDbData->mGcpDatabaseFileName ).arg( parms_GcpDbData->mError );
  }
  parms_GcpDbData = nullptr;
  return return_srid;
}
int QgsGeorefPluginGui::createGcpCoverageDb( QString  s_database_filename, int i_srid, bool b_dump )
{
  int return_srid = INT_MIN; // Invalid
  if ( s_database_filename.isNull() )
  { // Create default Databas-File name, when not given
    if ( mRasterFilePath.isNull() )
    {
      return return_srid;
    }
    s_database_filename = QString( "%1/%2.db" ).arg( mRasterFilePath ).arg( "gcp_coverage.gcp.db" );
  }
  QgsSpatiaLiteProviderGcpUtils::GcpDbData *parms_GcpDbData = nullptr;
  parms_GcpDbData = new QgsSpatiaLiteProviderGcpUtils::GcpDbData( s_database_filename, QString::null, i_srid );
  parms_GcpDbData->mDatabaseDump = b_dump;
  if ( QgsSpatiaLiteProviderGcpUtils::createGcpDb( parms_GcpDbData ) )
  {
    if ( parms_GcpDbData->mDatabaseValid )
    {
      return_srid = parms_GcpDbData->mGcpSrid;
    }
    else
    {
      qDebug() << QString( "QgsGeorefPluginGui:createGcpCoverageDb -  Database considered invalid - db[%1] error[%2]" ).arg( parms_GcpDbData->mGcpDatabaseFileName ).arg( parms_GcpDbData->mError );
    }
  }
  else
  {
    qDebug() << QString( "QgsGeorefPluginGui:createGcpCoverageDb - creation of Database failed - db[%1] error[%2]" ).arg( parms_GcpDbData->mGcpDatabaseFileName ).arg( parms_GcpDbData->mError );
  }
  parms_GcpDbData = nullptr;
  return return_srid;
}
void QgsGeorefPluginGui::fetchGcpMasterPointsExtent( )
{
  if (( mLegacyMode == 0 ) || ( !mLayer ) || ( !mGcpDbData ) || ( !mIface->mapCanvas() ) || ( !mCanvas ) || ( !mGCPListWidget ) ||
      ( !layer_gcp_master ) || ( !layer_gcp_points ) || ( !layer_gcp_points->isEditable() ) )
  { // Preconditions [everything we need to fullfill the task must be active]
    mActionFetchMasterGcpExtent->setChecked( false );
    return;
  }
  if ( !mActionLinkGeorefToQGis->isChecked() )
  { // if Georeference is not linked to QGis and
    if ( !mActionLinkQGisToGeoref->isChecked() )
    { // ... Qgis is not linked to Georeferencer: then link Georeference to QGis
      mActionLinkQGisToGeoref->setChecked( true );
      // this will insure that only points inside usable area are taken
      extentsChangedGeorefCanvas();
      mIface->mapCanvas()->refresh();
    }
  }
  int i_search_id_gcp = 0;
  double search_distance = DBL_MAX;
  int i_search_result_type = 0; // 0=not found ; 1=exact ; 2: inside area ; 3: outside area
  bool b_point_map = true;
  bool b_toPixel = true;
  QString s_searchDataPoint;
  bool b_searchDataPoint;
  bool b_valid = false;
  bool b_permit_update = true;
  // first: Collect the Information and store in an array
  QgsGCPList* master_GcpList = new QgsGCPList();
  QStringList sa_gcp_master_features;
  QgsFeatureIterator layer_gcp_master_fit = layer_gcp_master->getFeatures( QgsFeatureRequest().setFilterRect( mIface->mapCanvas()->extent() ) );
  QgsFeature fet_master_point;
  int i_count = 0;
  int id_gcp_master = 0;
  QString s_master_name = "";
  QString s_master_notes = "";
  QString s_master_gcp_text = "";
  QgsPoint pt_point;
  QgsPoint pt_pixel;
  QString s_master_TextInfo;
  QMap<int, QString> sql_insert;
  // The User may have reset value, so this value needs to be retrieved beforhand
  mGcpMasterArea = getCutlineGcpMasterArea( layer_gcp_master );
  while ( layer_gcp_master_fit.nextFeature( fet_master_point ) )
  {
    if ( fet_master_point.geometry() && fet_master_point.geometry()->type() == QGis::Point )
    {
      i_count++;
      b_valid = false;
      pt_point = fet_master_point.geometry()->vertexAt( 0 );
      b_searchDataPoint=false;
      if (mGcpMasterSrid == mGcpSrid)
      { // If not the same srid, will never be found without transformation
       b_searchDataPoint = mGCPListWidget->searchDataPoint( pt_point, b_point_map, mGcpMasterArea, &i_search_id_gcp, &search_distance, &i_search_result_type );
      }
      // TODO: Transform Point
      // s_searchDataPoint = QString( "-I-> searchDataPoint[%1] [gcp-exists] result_type[%2] distance[%3] id_gcp[%4]" ).arg( b_searchDataPoint ).arg( i_search_result_type ).arg( search_distance ).arg( i_search_id_gcp );
      if ( !b_searchDataPoint )
      { // Only id_gcp was when not found [not an exact match]
        if (( search_distance <= ( mGcpMasterArea / 2 ) ) && (mGcpMasterSrid == mGcpSrid))
        { // POINT is near the GCP (left/right/top/bottom of point) [update with Master-Gcp Position]
          if ( mGCPListWidget->updateDataPoint( i_search_id_gcp, b_point_map, pt_point ) )
          { // Retrieve the UPDATEd QgsGeorefDataPoint
            QgsGeorefDataPoint* data_point = mGCPListWidget->getDataPoint( i_search_id_gcp );
            if ( data_point )
            {
              data_point->setResultType( 2 ); // UPDATE
              s_master_TextInfo  = QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( i_search_id_gcp );
              s_master_TextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( mId_gcp_coverage );
              s_master_TextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( mGcp_points_table_name );
              s_master_TextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( fet_master_point.attribute( QString( "name" ) ).toString() );
              s_master_TextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( fet_master_point.attribute( QString( "notes" ) ).toString() );
              s_master_TextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( getGcpTransformParam( mTransformParam ) );
              id_gcp_master = fet_master_point.attribute( QString( "id_gcp" ) ).toInt();
              s_master_TextInfo += QString( "%1" ).arg( QString( "GcpMaster[%1]" ).arg( id_gcp_master ) );
              data_point->setTextInfo( s_master_TextInfo ); // UPDATE Meta-Data [only name,notes and gcp_text will be used]
              master_GcpList->append( data_point );
              s_searchDataPoint = QString( "-I-> searchDataPoint[%1] [gcp-UPDATE] result_type[%2] distance[%3,%5] id_gcp[%4]" ).arg( b_searchDataPoint ).arg( i_search_result_type ).arg( search_distance ).arg( i_search_id_gcp ).arg( mGcpMasterArea );
              qDebug() << s_searchDataPoint;
            }
          }
        }
        else
        {
          if ( !isGcpOff() )
          { // Only when the User has NOT turned off the Spatialite the Gcp-Logic and the Spatialite being used been compiled with the Gcp-Logic
            b_valid = true;
            id_gcp_master = fet_master_point.attribute( QString( "id_gcp" ) ).toInt();
            s_master_TextInfo  = QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( id_gcp_master );
            s_master_TextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( mId_gcp_coverage );
            s_master_TextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( mGcp_points_table_name );
            s_master_TextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( fet_master_point.attribute( QString( "name" ) ).toString() );
            s_master_TextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( fet_master_point.attribute( QString( "notes" ) ).toString() );
            s_master_TextInfo += QString( "%2%1" ).arg( mGcpDbData->mParseString ).arg( getGcpTransformParam( mTransformParam ) );
            s_master_TextInfo += QString( "%1" ).arg( QString( "GcpMaster[%1]" ).arg( id_gcp_master ) );
            // qDebug() << QString( "master_TextInfo[%1] " ).arg( s_master_TextInfo );
            pt_pixel = getGcpConvert( mGcp_coverage_name, pt_point, b_toPixel, mTransformParam );
            if (( pt_pixel.x() == 0.0 ) && ( pt_pixel.x() == 0.0 ) )
            {
              b_valid = false;
            }
            if (( i_search_result_type < 2 ) || ( i_search_result_type > 3 ) )
            { // 2=possible update ; 3 new point
              b_valid = false;
            }
            if (( !b_permit_update ) && ( i_search_result_type == 2 ) )
            { // We may, at some time, offer a option to turn off automatic UPDATEs [b_permit_update=false]
              b_valid = false;
            }
            if ( b_valid )
            {
              QgsGeorefDataPoint* data_point = new QgsGeorefDataPoint( pt_pixel, pt_point, mGcpSrid, id_gcp_master, id_gcp_master, i_search_result_type, s_master_TextInfo );
              data_point->setEnabled( true );
              master_GcpList->append( data_point );
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
int QgsGeorefPluginGui::bulkGcpPointsInsert( QgsGCPList* master_GcpList )
{
  int return_count = INT_MIN; // Invalid
  if (( !mGcpDbData ) || ( !master_GcpList ) || ( master_GcpList->countDataPoints() <= 0 ) )
  {
    return return_count;
  }
  QMap<int, QString> sql_update;
  QgsSpatiaLiteProviderGcpUtils::GcpDbData *parms_GcpDbData = nullptr;
  parms_GcpDbData = new QgsSpatiaLiteProviderGcpUtils::GcpDbData( mGcpDatabaseFileName, mGcp_coverage_name, mGcpSrid, mGcp_points_table_name, mLayer, mSpatialite_gcp_enabled );
  parms_GcpDbData->mOrder = 1; // INSERT
  for ( int i = 0; i < master_GcpList->countDataPoints(); i++ )
  {
    QgsGeorefDataPoint *data_point = master_GcpList->at( i );
    switch ( data_point->getResultType() )
    {
      case 3: // INSERT
        // qDebug() << QString( "QgsGeorefPluginGui::bulkGcpPointsInsert[%1] INSERT  id_gcp=%3 sql[%2]" ).arg( i ).arg( data_point->sqlInsertPointsCoverage() ).arg(data_point->id());
        parms_GcpDbData->gcp_coverages.insert( data_point->getIdMaster(), data_point->sqlInsertPointsCoverage(mGcpMasterSrid) );
        break;
      case 2: // UPDATE
        // qDebug() << QString( "QgsGeorefPluginGui::bulkGcpPointsInsert[%1] UPDATE id_gcp=%3 sql[%2]" ).arg( i ).arg( data_point->sqlInsertPointsCoverage() ).arg( data_point->id() );
        sql_update.insert( data_point->id(), data_point->sqlInsertPointsCoverage(mGcpMasterSrid) );
        break;
    }
  }
  if ( parms_GcpDbData->gcp_coverages.count() > 0 )
  {
    if ( QgsSpatiaLiteProviderGcpUtils::bulkGcpPointsInsert( parms_GcpDbData ) )
    { // No errors
      if ( parms_GcpDbData->mDatabaseValid )
      { // Also no errors
        return_count = parms_GcpDbData->gcp_coverages.count();
        return_count = 0;
        QMap<int, QString>::iterator it_map_coverages;
        for ( it_map_coverages = parms_GcpDbData->gcp_coverages.begin(); it_map_coverages != parms_GcpDbData->gcp_coverages.end(); ++it_map_coverages )
        { // Add the new Gcp-Points Pair to the GCPListWidget
          int id_gcp_master = it_map_coverages.key();
          int id_gcp = it_map_coverages.value().toInt();
          QgsGeorefDataPoint* get_point = master_GcpList->getDataPoint( id_gcp_master );
          if ( get_point )
          { // insure that the screen is refreshed afterwards by adding to return_count
            return_count++;
            QgsGeorefDataPoint* data_point = new QgsGeorefDataPoint( mCanvas, mIface->mapCanvas(), get_point->pixelCoords(), get_point->mapCoords(), get_point->getSrid(), id_gcp, get_point->isEnabled(), mLegacyMode );
            mGCPListWidget->addDataPoint( data_point );
            connect( mCanvas, SIGNAL( extentsChanged() ), data_point, SLOT( updateCoords() ) );
          }
        }
      }
      else
      {
        qDebug() << QString( "QgsGeorefPluginGui::bulkGcpPointsInsert -  Database invalid -  db[%1] error[%2]" ).arg( parms_GcpDbData->mGcpDatabaseFileName ).arg( parms_GcpDbData->mError );
      }
    }
    else
    {
      qDebug() << QString( "QgsGeorefPluginGui::bulkGcpPointsInsert -  INSERTing of Gcp failed -  db[%1] error[%2]" ).arg( parms_GcpDbData->mGcpDatabaseFileName ).arg( parms_GcpDbData->mError );
    }
  }
  if ( sql_update.count() > 0 )
  { // There may be no INSERTs, so  do the  UPDATEs here
    parms_GcpDbData->mOrder = 2; // UPDATE
    parms_GcpDbData->gcp_coverages = sql_update;
    if ( QgsSpatiaLiteProviderGcpUtils::bulkGcpPointsInsert( parms_GcpDbData ) )
    { // insure that the screen is refreshed afterwards by adding to return_count
      return_count += sql_update.count();
      // qDebug() << QString( "QgsGeorefPluginGui::bulkGcpPointsInsert[%1] -I-> UPDATE compleated" ).arg( sql_update.count() );
    }
  }
  parms_GcpDbData = nullptr;
  // When return_count > 0, the screens will be refreshed
  return return_count;
}

