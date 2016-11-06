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
#include "qgsgcplistwidget.h"

#include "qgsgeorefconfigdialog.h"
#include "qgsgeorefdescriptiondialog.h"
#include "qgsresidualplotitem.h"
#include "qgstransformsettingsdialog.h"

#include "qgsgeorefplugingui.h"
#include "qgsmessagebar.h"

#include "qgsvectordataprovider.h"
#include "qgsvectorlayer.h"
#include <sqlite3.h>
#include <spatialite.h>

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
  layer_gcp_points = NULL;
  layer_gcp_pixels = NULL;
  // mj10777: when false: old behavior or no spatialite
  b_gdalscript_or_gcp_list = false;
  mTransformParam = QgsGeorefTransform::PolynomialOrder1;
  mResamplingMethod = QgsImageWarper::Lanczos;
  mCompressionMethod = "DEFLATE";
  bGCPFileName = false;
  bPointsFileName = true;

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

  clearGCPData();

  removeOldLayer();

  delete mToolZoomIn;
  delete mToolZoomOut;
  delete mToolPan;
  delete mToolAddPoint;
  delete mToolDeletePoint;
  delete mToolMovePoint;
  delete mToolMovePointQgis;
}

// ----------------------------- protected --------------------------------- //
void QgsGeorefPluginGui::closeEvent( QCloseEvent *e )
{
  switch ( checkNeedGCPSave() )
  {
    case QgsGeorefPluginGui::GCPSAVE:
      if ( mGCPpointsFileName.isEmpty() )
        saveGCPsDialog();
      else
        saveGCPs();
      writeSettings();
      clearGCPData();
      removeOldLayer();
      mRasterFileName = "";
      e->accept();
      return;
    case QgsGeorefPluginGui::GCPSILENTSAVE:
      if ( !mGCPpointsFileName.isEmpty() )
        saveGCPs();
      clearGCPData();
      removeOldLayer();
      mRasterFileName = "";
      return;
    case QgsGeorefPluginGui::GCPDISCARD:
      writeSettings();
      clearGCPData();
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

    //delete old points
    clearGCPData();

    //delete any old rasterlayers
    removeOldLayer();
  }
}

// -------------------------- private slots -------------------------------- //
// File slots
void QgsGeorefPluginGui::openRaster()
{
  //  clearLog();
  switch ( checkNeedGCPSave() )
  {
    case QgsGeorefPluginGui::GCPSAVE:
      saveGCPsDialog();
      break;
    case QgsGeorefPluginGui::GCPSILENTSAVE:
      if ( !mGCPpointsFileName.isEmpty() )
        saveGCPs();
      break;
    case QgsGeorefPluginGui::GCPDISCARD:
      break;
    case QgsGeorefPluginGui::GCPCANCEL:
      return;
  }

  QSettings s;
  QString dir = s.value( "/Plugin-GeoReferencer/rasterdirectory" ).toString();
  if ( dir.isEmpty() )
    dir = '.';

  QString otherFiles = tr( "All other files (*)" );
  QString lastUsedFilter = s.value( "/Plugin-GeoReferencer/lastusedfilter", otherFiles ).toString();

  QString filters = QgsProviderRegistry::instance()->fileRasterFilters();
  filters.prepend( otherFiles + ";;" );
  filters.chop( otherFiles.size() + 2 );
  mRasterFileName = QFileDialog::getOpenFileName( this, tr( "Open raster" ), dir, filters, &lastUsedFilter );
  mModifiedRasterFileName = "";

  if ( mRasterFileName.isEmpty() )
    return;

  QString errMsg;
  if ( !QgsRasterLayer::isValidRasterFileName( mRasterFileName, errMsg ) )
  {
    QString msg = tr( "%1 is not a supported raster data source" ).arg( mRasterFileName );

    if ( !errMsg.isEmpty() )
      msg += '\n' + errMsg;

    QMessageBox::information( this, tr( "Unsupported Data Source" ), msg );
    return;
  }

  QFileInfo fileInfo( mRasterFileName );
  QDir parent_dir( fileInfo.canonicalPath() );
  s.setValue( "/Plugin-GeoReferencer/rasterdirectory", fileInfo.path() );
  s.setValue( "/Plugin-GeoReferencer/lastusedfilter", lastUsedFilter );

  mGeorefTransform.selectTransformParametrisation( mTransformParam );
  mGeorefTransform.setRasterChangeCoords( mRasterFileName );
  statusBar()->showMessage( tr( "Raster loaded: %1" ).arg( mRasterFileName ) );
  setWindowTitle( tr( "Georeferencer - %1" ).arg( fileInfo.fileName() ) );

  //  showMessageInLog(tr("Input raster"), mRasterFileName);

  //delete old points
  clearGCPData();

  //delete any old rasterlayers
  removeOldLayer();

  // mj10777
  mGCPFileName = QString( "%1.gcp.txt" ).arg( mRasterFileName );
  mGCPbaseFileName = fileInfo.completeBaseName();
  mGCPdatabaseFileName = QString( "%1/%2.maps.gcp.db" ).arg( fileInfo.canonicalPath() ).arg( parent_dir.dirName() );
  s_gcp_authid = mIface->mapCanvas()->mapSettings().destinationCrs().authid();
  s_gcp_description = mIface->mapCanvas()->mapSettings().destinationCrs().description();
  QStringList sa_authid = s_gcp_authid.split( ":" );
  QString s_srid = "-1";
  if ( sa_authid.length() == 2 )
    s_srid = sa_authid[1];
  i_gcp_srid = s_srid.toInt();
  // destinationCrs: authid[EPSG:3068]  srsid[1031]  description[DHDN / Soldner Berlin]
  // s_srid=QString("destinationCrs[%1]: authid[%2]  srsid[%3]  description[%4] ").arg(i_gcp_srid).arg(s_gcp_authid).arg(mIface->mapCanvas()->mapRenderer()->destinationCrs().srsid()).arg(s_gcp_description);
  // qDebug()<<s_srid;
  if ( i_gcp_srid > 0 )
  { // with srid (of project), activate the buttons and set set output-filename
    // give this a default name
    mModifiedRasterFileName = QString( "%1.%2.map.%3" ).arg( mGCPbaseFileName ).arg( s_srid ).arg( fileInfo.suffix() );
  }
  else
  {
    mModifiedRasterFileName = QString( "%1.0.map.%3" ).arg( mGCPbaseFileName ).arg( fileInfo.suffix() );
  }
  qDebug() << "QgsGeorefPluginGui::openRaster[1] - addRaster";
  // Add raster
  addRaster( mRasterFileName );

  // load previously added points
  mGCPpointsFileName = mRasterFileName + ".points";
  ( void )loadGCPs();

  qDebug() << "QgsGeorefPluginGui::openRaster[2] - setExtent";
  mCanvas->setExtent( mLayer->extent() );
  mCanvas->refresh();
  mIface->mapCanvas()->refresh();
  // mj10777
  if ( i_gcp_srid > 0 )
  { // with srid (of project), activate the buttons and set set output-filename
    mActionLinkGeorefToQGis->setChecked( false );
    mActionLinkQGisToGeoref->setChecked( false );
    mActionLinkGeorefToQGis->setEnabled( true );
    mActionLinkQGisToGeoref->setEnabled( true );
  }
  else
  {
    mModifiedRasterFileName = QString( "%1.0.map.%3" ).arg( mGCPbaseFileName ).arg( fileInfo.suffix() );
  }

  mCanvas->clearExtentHistory(); // reset zoomnext/zoomlast
  mWorldFileName = guessWorldFileName( mRasterFileName );
  qDebug() << "QgsGeorefPluginGui::openRaster[z] - end";
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
        mIface->addRasterLayer( mRasterFileName );
      }
      else
      {
        mIface->addRasterLayer( mModifiedRasterFileName );
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
  QgsTransformSettingsDialog d( mRasterFileName, mModifiedRasterFileName, mPoints.size() );
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
  if ( !checkReadyGeoref() )
    return;

  switch ( mTransformParam )
  {
    case QgsGeorefTransform::PolynomialOrder1:
    case QgsGeorefTransform::PolynomialOrder2:
    case QgsGeorefTransform::PolynomialOrder3:
    case QgsGeorefTransform::ThinPlateSpline:
    {
      // CAVEAT: generateGDALwarpCommand() relies on some member variables being set
      // by generateGDALtranslateCommand(), so this method must be called before
      // gdalwarpCommand*()!
      if ( b_gdalscript_or_gcp_list )
      {
        // mj10777: for use when only gcp-list is to be returned
        QString gcpCommand = generateGDALgcpCommand();
        showGDALScript( QStringList() << gcpCommand );
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
void QgsGeorefPluginGui::setAddPointTool()
{
  mCanvas->setMapTool( mToolAddPoint );
}

void QgsGeorefPluginGui::setDeletePointTool()
{
  mCanvas->setMapTool( mToolDeletePoint );
}

void QgsGeorefPluginGui::setMovePointTool()
{
  mCanvas->setMapTool( mToolMovePoint );
  mIface->mapCanvas()->setMapTool( mToolMovePointQgis );
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
void QgsGeorefPluginGui::addPoint( const QgsPoint& pixelCoords, const QgsPoint& mapCoords,
                                   bool enable, bool refreshCanvas/*, bool verbose*/ )
{
  QgsGeorefDataPoint* pnt = new QgsGeorefDataPoint( mCanvas, mIface->mapCanvas(),
      pixelCoords, mapCoords, enable );
  mPoints.append( pnt );
  mGCPsDirty = true;
  mGCPListWidget->setGCPList( &mPoints );
  if ( refreshCanvas )
  {
    mCanvas->refresh();
    mIface->mapCanvas()->refresh();
  }

  connect( mCanvas, SIGNAL( extentsChanged() ), pnt, SLOT( updateCoords() ) );
  updateGeorefTransform();

  //  if (verbose)
  //    logRequaredGCPs();
}

void QgsGeorefPluginGui::deleteDataPoint( QPoint coords )
{
  for ( QgsGCPList::iterator it = mPoints.begin(); it != mPoints.end(); ++it )
  {
    QgsGeorefDataPoint* pt = *it;
    if ( /*pt->pixelCoords() == coords ||*/ pt->contains( coords, true ) ) // first operand for removing from GCP table
    {
      delete *it;
      mPoints.erase( it );
      mGCPListWidget->updateGCPList();

      mCanvas->refresh();
      break;
    }
  }
  updateGeorefTransform();
}

void QgsGeorefPluginGui::deleteDataPoint( int theGCPIndex )
{
  Q_ASSERT( theGCPIndex >= 0 );
  delete mPoints.takeAt( theGCPIndex );
  mGCPListWidget->updateGCPList();
  updateGeorefTransform();
}

void QgsGeorefPluginGui::selectPoint( QPoint p )
{
  // Get Map Sender
  bool isMapPlugin = sender() == mToolMovePoint;
  QgsGeorefDataPoint *&mvPoint = isMapPlugin ? mMovingPoint : mMovingPointQgis;

  for ( QgsGCPList::const_iterator it = mPoints.constBegin(); it != mPoints.constEnd(); ++it )
  {
    if (( *it )->contains( p, isMapPlugin ) )
    {
      mvPoint = *it;
      break;
    }
  }
}

void QgsGeorefPluginGui::movePoint( QPoint p )
{
  // Get Map Sender
  bool isMapPlugin = sender() == mToolMovePoint;
  QgsGeorefDataPoint *mvPoint = isMapPlugin ? mMovingPoint : mMovingPointQgis;

  if ( mvPoint )
  {
    mvPoint->moveTo( p, isMapPlugin );
    mGCPListWidget->updateGCPList();
  }

}

void QgsGeorefPluginGui::releasePoint( QPoint p )
{
  Q_UNUSED( p );
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

void QgsGeorefPluginGui::showCoordDialog( const QgsPoint &pixelCoords )
{
  if ( mLayer && !mMapCoordsDialog )
  {
    mMapCoordsDialog = new QgsMapCoordsDialog( mIface->mapCanvas(), pixelCoords, this );
    connect( mMapCoordsDialog, SIGNAL( pointAdded( const QgsPoint &, const QgsPoint & ) ),
             this, SLOT( addPoint( const QgsPoint &, const QgsPoint & ) ) );
    mMapCoordsDialog->show();
  }
}

void QgsGeorefPluginGui::loadGCPsDialog()
{
  QString selectedFile = mRasterFileName.isEmpty() ? "" : mRasterFileName + ".points";
  mGCPpointsFileName = QFileDialog::getOpenFileName( this, tr( "Load GCP points" ),
                       selectedFile, tr( "GCP file" ) + " (*.points)" );
  if ( mGCPpointsFileName.isEmpty() )
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

void QgsGeorefPluginGui::saveGCPsDialog()
{
  if ( mPoints.isEmpty() )
  {
    mMessageBar->pushMessage( tr( "No GCP Points" ), tr( "No GCP points are available to save." ), QgsMessageBar::WARNING, messageTimeout() );
    return;
  }

  QString selectedFile = mRasterFileName.isEmpty() ? "" : mRasterFileName + ".points";
  mGCPpointsFileName = QFileDialog::getSaveFileName( this, tr( "Save GCP points" ),
                       selectedFile,
                       tr( "GCP file" ) + " (*.points)" );

  if ( mGCPpointsFileName.isEmpty() )
    return;

  if ( mGCPpointsFileName.right( 7 ) != ".points" )
    mGCPpointsFileName += ".points";

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
  if (( int )theGCPIndex >= mPoints.size() )
  {
    return;
  }

  // qgsmapcanvas doesn't seem to have a method for recentering the map
  QgsRectangle ext = mCanvas->extent();

  QgsPoint center = ext.center();
  QgsPoint new_center = mPoints[theGCPIndex]->pixelCoords();

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
  connect( mActionReset, SIGNAL( triggered() ), this, SLOT( reset() ) );

  mActionOpenRaster->setIcon( getThemeIcon( "/mActionAddRasterLayer.svg" ) );
  connect( mActionOpenRaster, SIGNAL( triggered() ), this, SLOT( openRaster() ) );

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
  mActionAddPoint->setIcon( getThemeIcon( "/mActionAddGCPPoint.png" ) );
  connect( mActionAddPoint, SIGNAL( triggered() ), this, SLOT( setAddPointTool() ) );

  mActionDeletePoint->setIcon( getThemeIcon( "/mActionDeleteGCPPoint.png" ) );
  connect( mActionDeletePoint, SIGNAL( triggered() ), this, SLOT( setDeletePointTool() ) );

  mActionMoveGCPPoint->setIcon( getThemeIcon( "/mActionMoveGCPPoint.png" ) );
  connect( mActionMoveGCPPoint, SIGNAL( triggered() ), this, SLOT( setMovePointTool() ) );

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
  mActionRasterProperties->setIcon( getThemeIcon( "/mActionRasterProperties.png" ) );
  connect( mActionRasterProperties, SIGNAL( triggered() ), this, SLOT( showRasterPropertiesDialog() ) );

  mActionGeorefConfig->setIcon( getThemeIcon( "/mActionGeorefConfig.png" ) );
  connect( mActionGeorefConfig, SIGNAL( triggered() ), this, SLOT( showGeorefConfigDialog() ) );

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

  mActionAddPoint->setCheckable( true );
  mapToolGroup->addAction( mActionAddPoint );
  mActionDeletePoint->setCheckable( true );
  mapToolGroup->addAction( mActionDeletePoint );
  mActionMoveGCPPoint->setCheckable( true );
  mapToolGroup->addAction( mActionMoveGCPPoint );
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

  mToolAddPoint = new QgsGeorefToolAddPoint( mCanvas );
  mToolAddPoint->setAction( mActionAddPoint );
  connect( mToolAddPoint, SIGNAL( showCoordDialog( const QgsPoint & ) ),
           this, SLOT( showCoordDialog( const QgsPoint & ) ) );

  mToolDeletePoint = new QgsGeorefToolDeletePoint( mCanvas );
  mToolDeletePoint->setAction( mActionDeletePoint );
  connect( mToolDeletePoint, SIGNAL( deleteDataPoint( const QPoint & ) ),
           this, SLOT( deleteDataPoint( const QPoint& ) ) );

  mToolMovePoint = new QgsGeorefToolMovePoint( mCanvas );
  mToolMovePoint->setAction( mActionMoveGCPPoint );
  connect( mToolMovePoint, SIGNAL( pointPressed( const QPoint & ) ),
           this, SLOT( selectPoint( const QPoint & ) ) );
  connect( mToolMovePoint, SIGNAL( pointMoved( const QPoint & ) ),
           this, SLOT( movePoint( const QPoint & ) ) );
  connect( mToolMovePoint, SIGNAL( pointReleased( const QPoint & ) ),
           this, SLOT( releasePoint( const QPoint & ) ) );

  // Point in Qgis Map
  mToolMovePointQgis = new QgsGeorefToolMovePoint( mIface->mapCanvas() );
  mToolMovePointQgis->setAction( mActionMoveGCPPoint );
  connect( mToolMovePointQgis, SIGNAL( pointPressed( const QPoint & ) ),
           this, SLOT( selectPoint( const QPoint & ) ) );
  connect( mToolMovePointQgis, SIGNAL( pointMoved( const QPoint & ) ),
           this, SLOT( movePoint( const QPoint & ) ) );
  connect( mToolMovePointQgis, SIGNAL( pointReleased( const QPoint & ) ),
           this, SLOT( releasePoint( const QPoint & ) ) );

  QSettings s;
  double zoomFactor = s.value( "/qgis/zoom_factor", 2 ).toDouble();
  mCanvas->setWheelFactor( zoomFactor );

  mExtentsChangedRecursionGuard = false;

  mGeorefTransform.selectTransformParametrisation( QgsGeorefTransform::Linear );
  mGCPsDirty = true;

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

  mGCPListWidget = new QgsGCPListWidget( this );
  mGCPListWidget->setGeorefTransform( &mGeorefTransform );
  dockWidgetGCPpoints->setWidget( mGCPListWidget );

  connect( mGCPListWidget, SIGNAL( jumpToGCP( uint ) ), this, SLOT( jumpToGCP( uint ) ) );
#if 0
  connect( mGCPListWidget, SIGNAL( replaceDataPoint( QgsGeorefDataPoint*, int ) ),
           this, SLOT( replaceDataPoint( QgsGeorefDataPoint*, int ) ) );
// Warning: Object::connect: No such signal QgsGCPListWidget::replaceDataPoint( QgsGeorefDataPoint*, int )
#endif
  connect( mGCPListWidget, SIGNAL( deleteDataPoint( int ) ),
           this, SLOT( deleteDataPoint( int ) ) );
  connect( mGCPListWidget, SIGNAL( pointEnabled( QgsGeorefDataPoint*, int ) ), this, SLOT( updateGeorefTransform() ) );
}

QLabel* QgsGeorefPluginGui::createBaseLabelStatus()
{
  QFont myFont( "Arial", 9 );
  QLabel* label = new QLabel( statusBar() );
  label->setFont( myFont );
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
    QgsMapLayerRegistry::instance()->removeMapLayers(
      ( QStringList() << mLayer->id() ) );
    mLayer = nullptr;
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
  mActionAddPoint->setIcon( getThemeIcon( "/mActionAddGCPPoint.png" ) );
  mActionDeletePoint->setIcon( getThemeIcon( "/mActionDeleteGCPPoint.png" ) );
  mActionMoveGCPPoint->setIcon( getThemeIcon( "/mActionMoveGCPPoint.png" ) );

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
  mLayer = new QgsRasterLayer( file, "Raster" );

  // so layer is not added to legend
  QgsMapLayerRegistry::instance()->addMapLayers(
    QList<QgsMapLayer *>() << mLayer, false, false );

  // add layer to map canvas
  QList<QgsMapCanvasLayer> layers;
  if ( isGCPDb() )
  {
    // root = QgsProject.instance().layerTreeRoot()
    // myGroup1 = root.addGroup("My Group 1")
    layers.append( QgsMapCanvasLayer( layer_gcp_pixels ) );
  }
  layers.append( QgsMapCanvasLayer( mLayer ) );
  mCanvas->setLayerSet( layers );

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
  QFile pointFile( mGCPpointsFileName );
  // will create needed gcp_database, if it does not exist
  // - also import an existing '.point' file
  // check if the used spatialite has GCP-enabled
  bool b_database_exists = createGCPDb();
  bool b_load_points_file = false;
  mGcp_points_layername = QString( "%1(gcp_point)" ).arg( s_gcp_points_table_name );
  if ( b_database_exists )
  {
    QString s_gcp_points_layer = QString( "%1|layername=%2" ).arg( mGCPdatabaseFileName ).arg( mGcp_points_layername );
    // qDebug()<< QString("QgsGeorefPluginGui::loadGCPs[1] layer[%1] geom[%2]").arg(s_gcp_points_layer).arg(s_gcp_points_base);
    layer_gcp_points = new QgsVectorLayer( s_gcp_points_layer, mGcp_points_layername,  "ogr" );
    Q_CHECK_PTR( layer_gcp_points );
    if ( layer_gcp_points->isValid() )
    {
      mGcp_pixel_layername = QString( "%1(gcp_pixel)" ).arg( s_gcp_points_table_name );
      s_gcp_points_layer = QString( "%1|layername=%2" ).arg( mGCPdatabaseFileName ).arg( mGcp_pixel_layername );
      // qDebug()<< QString("QgsGeorefPluginGui::loadGCPs[2] layer[%1] geom[%2]").arg(s_gcp_points_layer).arg(s_gcp_points_base);
      layer_gcp_pixels = new QgsVectorLayer( s_gcp_points_layer, mGcp_pixel_layername, "ogr" );
      Q_CHECK_PTR( layer_gcp_pixels );
      if ( layer_gcp_pixels->isValid() )
      {
        // add layer to map canvas
        // QList<QgsMapCanvasLayer> layers_pixels;
        // layers_pixels.append( QgsMapCanvasLayer( layer_gcp_pixels ) );
        // mCanvas->setLayerSet( layers_pixels );
        QgsMapLayerRegistry::instance()->addMapLayers( QList<QgsMapLayer *>() << layer_gcp_pixels, false );
        //mCanvas->refresh();
        QList<QgsMapCanvasLayer> layers_points;
        layers_points.append( QgsMapCanvasLayer( layer_gcp_points ) );
        // mIface->mapCanvas()->setLayerSet( layers_points );
        // so layer is not added to legend
        QgsMapLayerRegistry::instance()->addMapLayers( QList<QgsMapLayer *>() << layer_gcp_points, true );
        layer_gcp_pixels->startEditing();
        layer_gcp_points->startEditing();
        QgsFeatureIterator fit_pixels = layer_gcp_pixels->getFeatures( QgsFeatureRequest().setSubsetOfAttributes( QgsAttributeList() ) );
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
              QgsPoint pt_pixel = fet_pixel.geometry()->vertexAt( 0 );
              QgsPoint pt_point = fet_point.geometry()->vertexAt( 0 );
              i_count++;
              addPoint( pt_pixel, pt_point, true, false );
            }
          }
        }
        mInitialPoints = mPoints;
        qDebug() << QString( "QgsGeorefPluginGui::loadGCPs[2] count[%1] geom[%2]" ).arg( layer_gcp_pixels->featureCount() ).arg( mGcp_pixel_layername );
      }
      else
        layer_gcp_points = NULL;
    }
  }
  if ( layer_gcp_pixels == NULL )
    b_load_points_file = true;
  // 201603-19: override for the moment.
  b_load_points_file = true;
  if ( b_load_points_file )
  {
    if ( pointFile.open( QIODevice::ReadOnly ) )
    {
      qDebug() << QString( "-I-> QgsGeorefPluginGui::loadGCPs[%1]  loading point_file  (b_load_points_file = true)" ).arg( mGcp_pixel_layername );
      clearGCPData();
      QTextStream points( &pointFile );
      QString line = points.readLine();
      int i = 0;
      while ( !points.atEnd() )
      {
        line = points.readLine();
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
        if ( ls.count() == 5 )
        {
          bool enable = ls.at( 4 ).toInt();
          addPoint( pixelCoords, mapCoords, enable, false/*, verbose*/ ); // enabled
        }
        else
          addPoint( pixelCoords, mapCoords, true, false );

        ++i;
      }
      mInitialPoints = mPoints;
      //    showMessageInLog(tr("GCP points loaded from"), mGCPpointsFileName);
      mCanvas->refresh();
    }
  }
  else
  {
    qDebug() << QString( "-W-> QgsGeorefPluginGui::loadGCPs[%1] not loading point_file  (layer_gcp_pixels != NULL)" ).arg( mGcp_pixel_layername );
  }

  return true;
}

void QgsGeorefPluginGui::saveGCPs()
{
  if ( isGCPDb() )
  {
    updateGCPDb( mGCPbaseFileName.toLower() );
  }
  if ( bGCPFileName )
  {
    QFile gcpFile( mGCPFileName );
    if ( gcpFile.open( QIODevice::WriteOnly ) )
    {
      QTextStream gcps( &gcpFile );
      gcps << generateGDALgcpCommand();
    }
  }
  if ( bPointsFileName )
  {
    QFile pointFile( mGCPpointsFileName );
    if ( pointFile.open( QIODevice::WriteOnly ) )
    {
      QTextStream points( &pointFile );
      points << "mapX,mapY,pixelX,pixelY,enable" << endl;
      Q_FOREACH ( QgsGeorefDataPoint *pt, mPoints )
      {
        points << QString( "%1,%2,%3,%4,%5" )
        .arg( qgsDoubleToString( pt->mapCoords().x() ),
              qgsDoubleToString( pt->mapCoords().y() ),
              qgsDoubleToString( pt->pixelCoords().x() ),
              qgsDoubleToString( pt->pixelCoords().y() ) )
        .arg( pt->isEnabled() ) << endl;
      }
    }

    mInitialPoints = mPoints;
  }
  else
  {
    mMessageBar->pushMessage( tr( "Write Error" ), tr( "Could not write to GCP points file %1." ).arg( mGCPpointsFileName ), QgsMessageBar::WARNING, messageTimeout() );
    return;
  }

  //  showMessageInLog(tr("GCP points saved in"), mGCPpointsFileName);
}

QgsGeorefPluginGui::SaveGCPs QgsGeorefPluginGui::checkNeedGCPSave()
{
  if ( 0 == mPoints.count() )
    return QgsGeorefPluginGui::GCPDISCARD;

  if ( !equalGCPlists( mInitialPoints, mPoints ) )
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
      QFileInfo fi( mModifiedRasterFileName );
      fi.dir().remove( mModifiedRasterFileName );
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

bool QgsGeorefPluginGui::calculateMeanError( double& error ) const
{
  if ( mGeorefTransform.transformParametrisation() == QgsGeorefTransform::InvalidTransform )
  {
    return false;
  }

  int nPointsEnabled = 0;
  QgsGCPList::const_iterator gcpIt = mPoints.constBegin();
  for ( ; gcpIt != mPoints.constEnd(); ++gcpIt )
  {
    if (( *gcpIt )->isEnabled() )
    {
      ++nPointsEnabled;
    }
  }

  if ( nPointsEnabled == mGeorefTransform.getMinimumGCPCount() )
  {
    error = 0;
    return true;
  }
  else if ( nPointsEnabled < mGeorefTransform.getMinimumGCPCount() )
  {
    return false;
  }

  double sumVxSquare = 0;
  double sumVySquare = 0;

  gcpIt = mPoints.constBegin();
  for ( ; gcpIt != mPoints.constEnd(); ++gcpIt )
  {
    if (( *gcpIt )->isEnabled() )
    {
      sumVxSquare += (( *gcpIt )->residual().x() * ( *gcpIt )->residual().x() );
      sumVySquare += (( *gcpIt )->residual().y() * ( *gcpIt )->residual().y() );
    }
  }

  // Calculate the root mean square error, adjusted for degrees of freedom of the transform
  // Caveat: The number of DoFs is assumed to be even (as each control point fixes two degrees of freedom).
  error = sqrt(( sumVxSquare + sumVySquare ) / ( nPointsEnabled - mGeorefTransform.getMinimumGCPCount() ) );
  return true;
}

bool QgsGeorefPluginGui::writePDFMapFile( const QString& fileName, const QgsGeorefTransform& transform )
{
  Q_UNUSED( transform );
  if ( !mCanvas )
  {
    return false;
  }

  QgsRasterLayer *rlayer = ( QgsRasterLayer* ) mCanvas->layer( 0 );
  if ( !rlayer )
  {
    return false;
  }
  double mapRatio =  rlayer->extent().width() / rlayer->extent().height();

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

  //composer map
  QgsComposerMap* composerMap = new QgsComposerMap( composition, leftMargin, topMargin, contentWidth, contentHeight );
  composerMap->setKeepLayerSet( true );
  QStringList list;
  list.append( mCanvas->mapSettings().layers()[0] );
  composerMap->setLayerSet( list );
  composerMap->zoomToExtent( rlayer->extent() );
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

  //residual plot
  QgsResidualPlotItem* resPlotItem = new QgsResidualPlotItem( composition );
  composition->addItem( resPlotItem );
  resPlotItem->setSceneRect( QRectF( leftMargin, topMargin, contentWidth, contentHeight ) );
  resPlotItem->setExtent( composerMap->extent() );
  resPlotItem->setGCPList( mPoints );
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

  //title
  QFileInfo rasterFi( mRasterFileName );
  QgsComposerLabel* titleLabel = new QgsComposerLabel( composition );
  titleLabel->setFont( titleFont );
  titleLabel->setText( rasterFi.fileName() );
  composition->addItem( titleLabel );
  titleLabel->setSceneRect( QRectF( leftMargin, 5, contentWidth, 8 ) );
  titleLabel->setFrameEnabled( false );

  //composer map
  QgsRasterLayer *rLayer = ( QgsRasterLayer* ) mCanvas->layer( 0 );
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
  resPlotItem->setGCPList( mPoints );

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

  QgsGCPList::const_iterator gcpIt = mPoints.constBegin();
  QList< QStringList > gcpTableContents;
  for ( ; gcpIt != mPoints.constEnd(); ++gcpIt )
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
  gdalCommand << "gdal_translate" << "-of GTiff";
  if ( generateTFW )
  {
    // say gdal generate associated ESRI world file
    gdalCommand << "-co TFW=YES";
  }

  // mj10777: replaces Q_FOREACH loop
  gdalCommand << generateGDALgcpCommand();
  /*
  Q_FOREACH ( QgsGeorefDataPoint *pt, mPoints )
  {
    gdalCommand << QString( "-gcp %1 %2 %3 %4" ).arg( pt->pixelCoords().x() ).arg( -pt->pixelCoords().y() )
    .arg( pt->mapCoords().x() ).arg( pt->mapCoords().y() );
  }
  */

  QFileInfo rasterFileInfo( mRasterFileName );
  mTranslatedRasterFileName = QDir::tempPath() + '/' + rasterFileInfo.fileName();
  gdalCommand << QString( "\"%1\"" ).arg( mRasterFileName ) << QString( "\"%1\"" ).arg( mTranslatedRasterFileName );

  return gdalCommand.join( " " );
}

QString QgsGeorefPluginGui::generateGDALwarpCommand( const QString& resampling, const QString& compress,
    bool useZeroForTrans, int order, double targetResX, double targetResY )
{
  QStringList gdalCommand;
  gdalCommand << "gdalwarp" << "-r" << resampling;

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

  if ( mPoints.count() < ( int )mGeorefTransform.getMinimumGCPCount() )
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

  mGCPsDirty = false;
  updateTransformParamLabel();
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

bool QgsGeorefPluginGui::equalGCPlists( const QgsGCPList &list1, const QgsGCPList &list2 )
{
  if ( list1.count() != list2.count() )
    return false;

  int count = list1.count();
  int j = 0;
  for ( int i = 0; i < count; ++i, ++j )
  {
    QgsGeorefDataPoint *p1 = list1.at( i );
    QgsGeorefDataPoint *p2 = list2.at( j );
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

  qDeleteAll( mPoints );
  mPoints.clear();
  mGCPListWidget->updateGCPList();

  mIface->mapCanvas()->refresh();
}

int QgsGeorefPluginGui::messageTimeout()
{
  QSettings settings;
  return settings.value( "/qgis/messageTimeout", 5 ).toInt();
}

// mj10777: added functions
QString QgsGeorefPluginGui::generateGDALgcpCommand()
{
  QStringList gdalCommand;

  foreach ( QgsGeorefDataPoint *pt, mPoints )
  {
    gdalCommand << QString( "-gcp %1 %2 %3 %4" ).arg( pt->pixelCoords().x() ).arg( -pt->pixelCoords().y() )
    .arg( pt->mapCoords().x() ).arg( pt->mapCoords().y() );
  }
  return gdalCommand.join( " " );
}
bool QgsGeorefPluginGui::isGCPDb()
{
  if ( layer_gcp_pixels != NULL )
    return true;
  else
    return false;
}
bool QgsGeorefPluginGui::createGCPDb()
{
  QFile pointFile( mGCPpointsFileName );
  qDebug() << QString( "-I-> QgsGeorefPluginGui::createGCPDb -0- GCPdatabaseFileName[%1] " ).arg( mGCPdatabaseFileName );
  // After being opened (with CREATE), the file will exist, so store if it exist now
  bool b_database_exists = QFile( mGCPdatabaseFileName ).exists();
  void *cache;
  char *errMsg = NULL;
  b_spatialite_gcp_enabled = false;
  s_gcp_points_table_name = QString( "gcp_points_%1" ).arg( mGCPbaseFileName.toLower() );
  id_gcp_coverage = -1;
  bool b_cutline_geometries_create = false;
  bool b_spatial_ref_sys_create = false;
  bool b_gcp_coverage = false;
  bool b_gcp_coverage_create = false;
  bool b_import_points = false;
  // base on code found in 'QgsOSMDatabase::createSpatialTable'
  sqlite3* db_handle;
  // open database
  int ret = sqlite3_open_v2( mGCPdatabaseFileName.toUtf8().data(), &db_handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL );
  if ( ret != SQLITE_OK )
  {
    mError = QString( "Failed to open database: rc=%1 database[%2]" ).arg( ret ).arg( mGCPdatabaseFileName );
    sqlite3_close( db_handle );
    db_handle = NULL;
    return false;
  }
  // needed by spatialite_init_ex (multi-threading)
  cache = spatialite_alloc_connection();
  // load spatialite extension
  spatialite_init_ex( db_handle, cache, 0 );
  sqlite3_stmt* stmt;
  // Start TRANSACTION
  ret = sqlite3_exec( db_handle, "BEGIN", NULL, NULL, 0 );
  Q_ASSERT( ret == SQLITE_OK );
  Q_UNUSED( ret );
  // SELECT HasGCP(), (SELECT count(name) FROM sqlite_master WHERE ((type='table') AND (name='gcp_coverages'))) AS has_gcp_coverages;
  QString s_sql = QString( "SELECT HasGCP(), (SELECT count(name) FROM sqlite_master WHERE ((type='table') AND (name='gcp_coverages'))) AS has_gcp_coverages," );
  s_sql += QString( " (SELECT count(name) FROM sqlite_master WHERE ((type='table') AND (name='create_cutline_polygons'))) AS has_cutline_geometries," );
  s_sql += QString( " (SELECT count(name) FROM sqlite_master WHERE ((type='table') AND (name='spatial_ref_sys'))) AS has_spatial_ref_sys" );
  ret = sqlite3_prepare_v2( db_handle, s_sql.toUtf8().constData(), -1, &stmt, NULL );
  if ( ret != SQLITE_OK )
  { //  rc=1 [no such function: HasGCP] is not an error [older spatialite version]
    if ( QString( "no such function: HasGCP" ).compare( QString( "%1" ).arg( sqlite3_errmsg( db_handle ) ) ) != 0 )
    {
      mError = QString( "Unable to SELECT HasGCP(): rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -2- [%1] " ).arg( mError );
    }
  }
  int i_spatialite_gcp_enabled = 0;
  int i_gcp_coverage = -1;
  int i_cutline_geometries = -1;
  int i_spatial_ref_sys_create = -1;
  while ( sqlite3_step( stmt ) == SQLITE_ROW )
  {
    i_spatialite_gcp_enabled = sqlite3_column_int( stmt, 0 );
    i_gcp_coverage = sqlite3_column_int( stmt, 1 );
    i_cutline_geometries = sqlite3_column_int( stmt, 2 );
    i_spatial_ref_sys_create = sqlite3_column_int( stmt, 3 );
  }
  sqlite3_finalize( stmt );
  if ( i_spatialite_gcp_enabled > 0 )
    b_spatialite_gcp_enabled = true;
  if ( i_gcp_coverage > 0 )
    b_gcp_coverage = true;
  if ( i_cutline_geometries > 0 )
    b_cutline_geometries_create = true;
  if ( i_spatial_ref_sys_create > 0 )
    b_spatial_ref_sys_create = true;
  qDebug() << QString( "-I-> QgsGeorefPluginGui::createGCPDb spatialite_gcp_enabled[%1] gcp_coverage[%2] cutline_geometries_create[%3] spatial_ref_sys_create[%4]" ).arg( b_spatialite_gcp_enabled ).arg( b_gcp_coverage ).arg( b_cutline_geometries_create ).arg( b_spatial_ref_sys_create );
  if ( !b_spatial_ref_sys_create )
  { // no spatial_ref_sys, call InitSpatialMetadata
    s_sql = QString( "SELECT InitSpatialMetadata(0)" );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to InitSpatialMetadata: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -3- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    b_spatial_ref_sys_create = true;
  }
  if ( !b_gcp_coverage )
  {
    qDebug() << QString( "-I-> QgsGeorefPluginGui::createGCPDb creating [%1] " ).arg( "gcp_coverages" );
    QString s_sql_gcp = QString( "CREATE TABLE IF NOT EXISTS gcp_coverages\n( --collection of maps for georeferencing\n" );
    s_sql_gcp += QString( " %1 INTEGER PRIMARY KEY AUTOINCREMENT,\n" ).arg( "id_gcp_coverage" );
    s_sql_gcp += QString( " %1 TEXT DEFAULT '',\n" ).arg( "coverage_name" );
    s_sql_gcp += QString( " %1 DATE DEFAULT '0001-01-01',\n" ).arg( "year" );
    s_sql_gcp += QString( " %1 TEXT DEFAULT '',\n" ).arg( "path_image" );
    s_sql_gcp += QString( " %1 TEXT DEFAULT '',\n" ).arg( "path_gtif" );
    s_sql_gcp += QString( " %1 TEXT DEFAULT '',\n" ).arg( "title" );
    s_sql_gcp += QString( " %1 TEXT DEFAULT '',\n" ).arg( "abstract" );
    s_sql_gcp += QString( " %1 TEXT DEFAULT '',\n" ).arg( "copyright" );
    s_sql_gcp += QString( " %1 INTEGER DEFAULT %2,\n" ).arg( "srid" ).arg( i_gcp_srid );
    s_sql_gcp += QString( " %1 INTEGER DEFAULT 0,\n" ).arg( "gcp_count" );
    s_sql_gcp += QString( " %1 INTEGER DEFAULT 2, -- QgsGeorefTransform::PolynomialOrder1\n" ).arg( "transformtype" );
    s_sql_gcp += QString( " %1 INTEGER DEFAULT 4, -- GRA_Lanczos\n" ).arg( "resampling" );
    s_sql_gcp += QString( " %1 TEXT DEFAULT 'DEFLATE',\n" ).arg( "compression" );
    s_sql_gcp += QString( " %1 DOUBLE DEFAULT 0.0,\n" ).arg( "extent_minx" );
    s_sql_gcp += QString( " %1 DOUBLE DEFAULT 0.0,\n" ).arg( "extent_miny" );
    s_sql_gcp += QString( " %1 DOUBLE DEFAULT 0.0,\n" ).arg( "extent_maxx" );
    s_sql_gcp += QString( " %1 DOUBLE DEFAULT 0.0)\n" ).arg( "extent_maxy" );
    ret = sqlite3_exec( db_handle, s_sql_gcp.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to CREATE gcp_coverages: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql_gcp );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -4- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    else
      b_gcp_coverage = true;
  }
  else
  { // gcp_coverages exists, check for for raster entry
    s_sql = QString( "SELECT id_gcp_coverage,transformtype,resampling, compression FROM gcp_coverages WHERE coverage_name='%1'" ).arg( mGCPbaseFileName );
    ret = sqlite3_prepare_v2( db_handle, s_sql.toUtf8().constData(), -1, &stmt, NULL );
    if ( ret != SQLITE_OK )
    { //  rc=1 [no such table: vector_layers_statistics or gcp_points, 'no such column' for gcp_pixel and gcp_ppoint] is an error
      mError = QString( "Unable to retrieve id_gcp_covarage: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -5- [%1] " ).arg( mError );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    while ( sqlite3_step( stmt ) == SQLITE_ROW )
    {
      id_gcp_coverage = sqlite3_column_int( stmt, 0 );
      mTransformParam = ( QgsGeorefTransform::TransformParametrisation )sqlite3_column_int( stmt, 1 );
      mResamplingMethod = ( QgsImageWarper::ResamplingMethod )sqlite3_column_int( stmt, 2 );
      mCompressionMethod = QString::fromUtf8(( const char * ) sqlite3_column_text( stmt, 3 ) );
    }
    sqlite3_finalize( stmt );
  }
  if ( id_gcp_coverage < 0 )
  { // INSERT raster entry
    b_import_points = true; // import of .points file only when first created
    s_sql = QString( "INSERT INTO gcp_coverages (coverage_name,srid, transformtype, resampling, compression,path_image,path_gtif)\n" );
    s_sql += QString( "SELECT '%1' AS coverage_name," ).arg( mGCPbaseFileName.toLower() );
    s_sql += QString( "%1 AS srid," ).arg( i_gcp_srid );
    s_sql += QString( "%1 AS transformtype," ).arg(( int )mTransformParam );
    s_sql += QString( "%1 AS resampling," ).arg(( int )mResamplingMethod );
    s_sql += QString( "'%1' AS compression," ).arg( mCompressionMethod );
    s_sql += QString( "'%1' AS path_image," ).arg( mRasterFileName );
    s_sql += QString( "'%1' AS path_gtif" ).arg( mModifiedRasterFileName );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to INSERT INTO gcp_covarage: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -6- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
    }
    s_sql = QString( "SELECT id_gcp_coverage FROM gcp_coverages WHERE coverage_name='%1'" ).arg( mGCPbaseFileName.toLower() );
    ret = sqlite3_prepare_v2( db_handle, s_sql.toUtf8().constData(), -1, &stmt, NULL );
    if ( ret != SQLITE_OK )
    { //  rc=1 [no such table: vector_layers_statistics or gcp_points, 'no such column' for gcp_pixel and gcp_ppoint] is an error
      mError = QString( "Unable to retrieve id_gcp_covarage: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -7- [%1] " ).arg( mError );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    while ( sqlite3_step( stmt ) == SQLITE_ROW )
    {
      id_gcp_coverage = sqlite3_column_int( stmt, 0 );
      b_gcp_coverage_create = true; // create needed tables, views
    }
    sqlite3_finalize( stmt );
  }
  // End TRANSACTION
  ret = sqlite3_exec( db_handle, "COMMIT", NULL, NULL, 0 );
  Q_ASSERT( ret == SQLITE_OK );
  Q_UNUSED( ret );
  qDebug() << QString( "-I-> QgsGeorefPluginGui::createGCPDb gcp_points_table_name[%1] id_gcp_coverage[%2] b_gcp_coverage_create[%3]" ).arg( s_gcp_points_table_name ).arg( id_gcp_coverage ).arg( b_gcp_coverage_create );
  if (( b_gcp_coverage_create ) && ( id_gcp_coverage > 0 ) )
  { // raster entry has been added to gcp_covarage, create corresponding tables
    qDebug() << QString( "-I-> QgsGeorefPluginGui::createGCPDb creating gcp tables and views for[%1] " ).arg( s_gcp_points_table_name );
    // Start TRANSACTION
    ret = sqlite3_exec( db_handle, "BEGIN", NULL, NULL, 0 );
    Q_ASSERT( ret == SQLITE_OK );
    Q_UNUSED( ret );
    s_sql = QString( "CREATE TABLE IF NOT EXISTS '%1'\n(\n" ).arg( s_gcp_points_table_name );
    s_sql += QString( " %1 INTEGER PRIMARY KEY AUTOINCREMENT,\n" ).arg( "id_gcp" );
    s_sql += QString( " %1 INTEGER DEFAULT 0,\n" ).arg( "id_gcp_coverage" );
    s_sql += QString( " %1 TEXT DEFAULT '',\n" ).arg( "name" );
    s_sql += QString( " %1 TEXT DEFAULT '',\n" ).arg( "notes" );
    s_sql += QString( " %1 DOUBLE DEFAULT 0,\n" ).arg( "pixel_x" );
    s_sql += QString( " %1 DOUBLE DEFAULT 0,\n" ).arg( "pixel_y" );
    s_sql += QString( " %1 INTEGER DEFAULT %2,\n" ).arg( "srid" ).arg( i_gcp_srid );
    s_sql += QString( " %1 DOUBLE DEFAULT 0,\n" ).arg( "point_x" );
    s_sql += QString( " %1 DOUBLE DEFAULT 0,\n" ).arg( "point_y" );
    s_sql += QString( " -- 0=tpc (Thin Plate Spline)\n" );
    s_sql += QString( " -- 1=1st order(affine) [precise]\n" );
    s_sql += QString( " -- 2=2nd order [less exact]\n" );
    s_sql += QString( " -- 3=3rd order [poor]\n" );
    s_sql += QString( " %1 INTEGER DEFAULT 0,\n" ).arg( "order_selected" );
    s_sql += QString( " %1 INTEGER DEFAULT 1,\n" ).arg( "gcp_enable" );
    s_sql += QString( " %1 TEXT DEFAULT ''\n)" ).arg( "gcp_text" );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create table gcp_points: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -8- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    s_sql = QString( "SELECT AddGeometryColumn('%1','gcp_pixel',-1,'POINT','XY')" ).arg( s_gcp_points_table_name );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create geometry column gcp_pixel: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -9- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    s_sql = QString( "SELECT CreateSpatialIndex('%1','gcp_pixel')" ).arg( s_gcp_points_table_name );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create CreateSpatialIndex for gcp_point: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -10- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    s_sql = QString( "SELECT AddGeometryColumn('%1','gcp_point',%2,'POINT','XY')" ).arg( s_gcp_points_table_name ).arg( i_gcp_srid );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create geometry column gcp_point: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -11- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    s_sql = QString( "SELECT CreateSpatialIndex('%1','gcp_point')" ).arg( s_gcp_points_table_name );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create CreateSpatialIndex for gcp_point: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -12- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    // If while INSERTING, no geometries are supplied (only valid pixel/point x+y values), the geometries will be created
    s_sql = QString( "CREATE TRIGGER IF NOT EXISTS 'tb_ins_%1'\n" ).arg( s_gcp_points_table_name );
    s_sql += QString( " AFTER INSERT ON  \"%1\"\nBEGIN\n" ).arg( s_gcp_points_table_name );
    s_sql += QString( " UPDATE \"%1\" \n" ).arg( s_gcp_points_table_name );
    s_sql += QString( " -- general checking for and setting of NULL values done in 'tb_upd_gcp_points'\n SET\n" );
    s_sql += QString( "  name =  NEW.name, notes = NEW.notes,\n" );
    s_sql += QString( "  pixel_x = NEW.pixel_x, pixel_y = NEW.pixel_y,\n" );
    s_sql += QString( "  srid = NEW.srid,\n" );
    s_sql += QString( "  point_x = NEW.point_x, point_y = NEW.point_y,\n" );
    s_sql += QString( "  order_selected = NEW.order_selected,\n" );
    s_sql += QString( "  gcp_enable = NEW.gcp_enable,\n" );
    s_sql += QString( "  gcp_text = NEW.gcp_text,\n" );
    s_sql += QString( "  gcp_pixel = CASE WHEN NEW.gcp_pixel IS NULL THEN MakePoint(NEW.pixel_x,NEW.pixel_y,-1) ELSE NEW.gcp_pixel END,\n" );
    s_sql += QString( "  gcp_point = CASE WHEN NEW.gcp_point IS NULL THEN MakePoint(NEW.point_x,NEW.point_y,NEW.srid) ELSE NEW.gcp_point END\n" );
    s_sql += QString( " WHERE id_gcp = NEW.id_gcp;\nEND;" );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create AFTER INSERT TRIGGER for %1: rc=%2 [%3] sql[%4]\n" ).arg( s_gcp_points_table_name ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -13- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    // AFTER (not BEFORE) an UPDATE of the geometries,  the pixel/point x+y values will be updated
    s_sql = QString( "CREATE TRIGGER IF NOT EXISTS 'tb_upd_%1'\n" ).arg( s_gcp_points_table_name );
    s_sql += QString( " AFTER UPDATE OF\n" );
    s_sql += QString( "  id_gcp,name,notes,pixel_x,pixel_y,srid,point_x,point_y,order_selected,gcp_enable,gcp_text,gcp_pixel,gcp_point\n" );
    s_sql += QString( " ON \"%1\" \nBEGIN\n" ).arg( s_gcp_points_table_name );
    s_sql += QString( " UPDATE \"%1\" \n" ).arg( s_gcp_points_table_name );
    s_sql += QString( " -- general checking for and setting of NULL values\n SET\n" );
    s_sql += QString( "  name =  CASE WHEN NEW.name IS NULL THEN '' ELSE NEW.name END,\n" );
    s_sql += QString( "  notes = CASE WHEN NEW.notes IS NULL THEN '' ELSE NEW.notes END,\n" );
    s_sql += QString( "  pixel_x = ST_X(NEW.gcp_pixel),\n" );
    s_sql += QString( "  pixel_y = ST_Y(NEW.gcp_pixel),\n" );
    s_sql += QString( "  srid = ST_SRID(NEW.gcp_point),\n" );
    s_sql += QString( "  point_x = ST_X(NEW.gcp_point),\n" );
    s_sql += QString( "  point_y = ST_Y(NEW.gcp_point),\n" );
    s_sql += QString( "  order_selected = CASE WHEN NEW.order_selected IS NULL THEN 0 ELSE NEW.order_selected END,\n" );
    s_sql += QString( "  gcp_enable = CASE WHEN NEW.gcp_enable IS NULL THEN 1 ELSE NEW.gcp_enable END,\n" );
    s_sql += QString( "  gcp_text = CASE WHEN ((NEW.gcp_text = NULL) OR (NEW.gcp_text = '')) THEN CAST(NEW.id_gcp AS TEXT) ELSE NEW.gcp_text END,\n" );
    s_sql += QString( "  gcp_pixel = NEW.gcp_pixel,\n" );
    s_sql += QString( "  gcp_point = NEW.gcp_point\n" );
    s_sql += QString( " WHERE id_gcp = OLD.id_gcp;\nEND;" );
    // EXPLAIN UPDATE gcp_points SET notes='test_update' WHERE id_gcp=1;
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create BEFORE UPDATE TRIGGER for %1: rc=%2 [%3] sql[%4]\n" ).arg( s_gcp_points_table_name ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -14- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    s_sql = QString( "CREATE VIEW IF NOT EXISTS '%1_%2' AS\n" ).arg( "list_gcp" ).arg( mGCPbaseFileName.toLower() );
    s_sql += QString( " SELECT id_gcp,\n  " );
    s_sql += QString( "  '-gcp '||pixel_x||' '||-(pixel_y)||' '||point_x||' '||point_y AS gcp_gdal,\n" );
    s_sql += QString( "  point_x||','||point_y||','||pixel_x||','||pixel_y||','||gcp_enable AS point_qgis\n" );
    s_sql += QString( " FROM \"%1\"\n ORDER BY point_x,point_y;\n" ).arg( s_gcp_points_table_name );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create CREATE VIEW for list_gcp_%1: rc=%2 [%3] sql[%4]\n" ).arg( mGCPbaseFileName.toLower() ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -15- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    s_sql = QString( "CREATE VIEW IF NOT EXISTS '%1_%2' AS\n" ).arg( "list_gcp_disabeld" ).arg( mGCPbaseFileName.toLower() );
    s_sql += QString( " SELECT id_gcp,\n  " );
    s_sql += QString( "  '-gcp '||pixel_x||' '||-(pixel_y)||' '||point_x||' '||point_y AS gcp_gdal,\n" );
    s_sql += QString( "  point_x||','||point_y||','||pixel_x||','||pixel_y||','||gcp_enable AS point_qgis\n" );
    s_sql += QString( " FROM \"%1\"\n %2\n ORDER BY point_x,point_y;\n" ).arg( s_gcp_points_table_name ).arg( "WHERE (gcp_enable=0)" );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create CREATE VIEW for list_gcp_disabeld_%1: rc=%2 [%3] sql[%4]\n" ).arg( mGCPbaseFileName.toLower() ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -16- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    s_sql = QString( "CREATE VIEW IF NOT EXISTS '%1_%2' AS\n" ).arg( "list_gcp_enabled" ).arg( mGCPbaseFileName.toLower() );
    s_sql += QString( " SELECT id_gcp,\n  " );
    s_sql += QString( "  '-gcp '||pixel_x||' '||-(pixel_y)||' '||point_x||' '||point_y AS gcp_gdal,\n" );
    s_sql += QString( "  point_x||','||point_y||','||pixel_x||','||pixel_y||','||gcp_enable AS point_qgis\n" );
    s_sql += QString( " FROM \"%1\"\n %2\n ORDER BY point_x,point_y;\n" ).arg( s_gcp_points_table_name ).arg( "WHERE (gcp_enable=1)" );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create CREATE VIEW for list_gcp_enabled_%1: rc=%2 [%3] sql[%4]\n" ).arg( mGCPbaseFileName.toLower() ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -17- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    s_sql = QString( "CREATE VIEW IF NOT EXISTS '%1_%2' AS\n" ).arg( "list_gcp_enabled_order_0" ).arg( mGCPbaseFileName.toLower() );
    s_sql += QString( " -- 0=tpc (Thin Plate Spline)\n SELECT\n" );
    s_sql += QString( "  id_gcp,'-gcp '||pixel_x||' '||-(pixel_y)||' '||point_x||' '||point_y AS gcp_gdal,\n" );
    s_sql += QString( "  point_x||','||point_y||','||pixel_x||','||pixel_y||','||gcp_enable AS point_qgis\n" );
    s_sql += QString( " FROM \"%1\"\n %2\n ORDER BY point_x,point_y;\n" ).arg( s_gcp_points_table_name ).arg( "WHERE ((gcp_enable=1) AND (order_selected=0))" );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create CREATE VIEW for list_gcp_enabled_order_0_%1: rc=%2 [%3] sql[%4]\n" ).arg( mGCPbaseFileName.toLower() ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -18- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    s_sql = QString( "CREATE VIEW IF NOT EXISTS '%1_%2' AS\n" ).arg( "list_gcp_enabled_order_1" ).arg( mGCPbaseFileName.toLower() );
    s_sql += QString( " -- 1=1st order(affine) [precise]\n SELECT id_gcp,\n  " );
    s_sql += QString( "  '-gcp '||pixel_x||' '||-(pixel_y)||' '||point_x||' '||point_y AS gcp_gdal,\n" );
    s_sql += QString( "  point_x||','||point_y||','||pixel_x||','||pixel_y||','||gcp_enable AS point_qgis\n" );
    s_sql += QString( " FROM \"%1\"\n %3\n ORDER BY point_x,point_y;\n" ).arg( s_gcp_points_table_name ).arg( "WHERE ((gcp_enable=1) AND (order_selected=1))" );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create CREATE VIEW for list_gcp_enabled_order_1_%1: rc=%2 [%3] sql[%4]\n" ).arg( mGCPbaseFileName.toLower() ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -19- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    s_sql = QString( "CREATE VIEW IF NOT EXISTS '%1_%2' AS\n" ).arg( "list_gcp_enabled_order_2" ).arg( mGCPbaseFileName.toLower() );
    s_sql += QString( " -- 2=2nd order [less exact]\n SELECT id_gcp,\n  " );
    s_sql += QString( "  '-gcp '||pixel_x||' '||-(pixel_y)||' '||point_x||' '||point_y AS gcp_gdal,\n" );
    s_sql += QString( "  point_x||','||point_y||','||pixel_x||','||pixel_y||','||gcp_enable AS point_qgis\n" );
    s_sql += QString( " FROM \"%1\"\n %2\n ORDER BY point_x,point_y;\n" ).arg( s_gcp_points_table_name ).arg( "WHERE ((gcp_enable=1) AND (order_selected=2))" );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create CREATE VIEW for list_gcp_enabled_order_2_%1: rc=%2 [%3] sql[%4]\n" ).arg( mGCPbaseFileName.toLower() ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -20- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    s_sql = QString( "CREATE VIEW IF NOT EXISTS '%1_%2' AS\n" ).arg( "list_gcp_enabled_order_3" ).arg( mGCPbaseFileName.toLower() );
    s_sql += QString( " -- 3=3rd order [poor]\n SELECT id_gcp,\n  " );
    s_sql += QString( "  '-gcp '||pixel_x||' '||-(pixel_y)||' '||point_x||' '||point_y AS gcp_gdal,\n" );
    s_sql += QString( "  point_x||','||point_y||','||pixel_x||','||pixel_y||','||gcp_enable AS point_qgis\n" );
    s_sql += QString( " FROM \"%1\"\n %2\n ORDER BY point_x,point_y;\n" ).arg( s_gcp_points_table_name ).arg( "WHERE ((gcp_enable=1) AND (order_selected=3))" );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create CREATE VIEW for list_gcp_enabled_order_3_%1: rc=%2 [%3] sql[%4]\n" ).arg( mGCPbaseFileName.toLower() ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -21- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    // End TRANSACTION
    ret = sqlite3_exec( db_handle, "COMMIT", NULL, NULL, 0 );
    Q_ASSERT( ret == SQLITE_OK );
    Q_UNUSED( ret );
  }
  if (( b_database_exists ) && ( b_gcp_coverage ) )
  { // if the database already exists, check that '"gcp_pixel' and 'gcp_point' geometries in 'gcp_points' exist and correct the statistics if needed
    // Sanity checks to insure that this is a table we can work with, otherwise return 'db' not found
    // Start TRANSACTION
    ret = sqlite3_exec( db_handle, "BEGIN", NULL, NULL, 0 );
    Q_ASSERT( ret == SQLITE_OK );
    Q_UNUSED( ret );
    int i_point_statistics = 0;
    int i_pixel_statistics = 0;
    int i_gcp_coverages = 0;
    // SELECT count((SELECT extent_min_x FROM vector_layers_statistics WHERE ((extent_min_x IS NOT NULL) AND (table_name='gcp_points')  AND (geometry_column='gcp_pixel')))) AS pixel_statistics,count((SELECT extent_min_x FROM vector_layers_statistics WHERE ((extent_min_x IS NOT NULL) AND (table_name='gcp_points')  AND (geometry_column='gcp_point')))) AS points_statistics, (SELECT count(id_gcp) FROM 'gcp_points') AS points_geometries, (SELECT count(gcp_pixel) FROM 'gcp_points' WHERE gcp_pixel IS NOT NULL) AS count_pixel, (SELECT count(gcp_point) FROM 'gcp_points' WHERE gcp_point IS NOT NULL) AS count_point
    s_sql = QString( "SELECT " );
    s_sql += QString( "count((SELECT extent_min_x FROM vector_layers_statistics WHERE ((extent_min_x IS NOT NULL) AND " );
    s_sql += QString( "(table_name='%1') AND (geometry_column='%2')))) AS pixel_statistics," ).arg( s_gcp_points_table_name ).arg( "gcp_pixel" );
    s_sql += QString( "count((SELECT extent_min_x FROM vector_layers_statistics WHERE ((extent_min_x IS NOT NULL) AND " );
    s_sql += QString( "(table_name='%1') AND (geometry_column='%2')))) AS points_statistics," ).arg( s_gcp_points_table_name ).arg( "gcp_point" );
    // s_sql += QString("(SELECT count(id_gcp) FROM '%1') AS points_table,").arg(s_gcp_points_table_name);
    // s_sql += QString("(SELECT count(gcp_pixel) FROM '%1' WHERE gcp_pixel IS NOT NULL) AS count_pixel,").arg(s_gcp_points_table_name);
    // s_sql += QString("(SELECT count(gcp_point) FROM '%1' WHERE gcp_point IS NOT NULL) AS count_point").arg(s_gcp_points_table_name);
    s_sql += QString( "(SELECT count(id_gcp_coverage) FROM '%1' WHERE gcp_count > 0) AS count_gcp" ).arg( "gcp_coverages" );
    ret = sqlite3_prepare_v2( db_handle, s_sql.toUtf8().constData(), -1, &stmt, NULL );
    if ( ret != SQLITE_OK )
    { //  rc=1 [no such table: vector_layers_statistics or gcp_points, 'no such column' for gcp_pixel and gcp_ppoint] is an error
      mError = QString( "Unable to check vector_layers_statistics, table gcp_points or geometries: gcp_pixel and gcp_point: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -22- [%1] " ).arg( mError );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    while ( sqlite3_step( stmt ) == SQLITE_ROW )
    {
      i_pixel_statistics = sqlite3_column_int( stmt, 0 );
      i_point_statistics = sqlite3_column_int( stmt, 1 );
      i_gcp_coverages = sqlite3_column_int( stmt, 2 );
    }
    sqlite3_finalize( stmt );
    if ( i_gcp_coverages < 1 )
    { // gcp_points is empty, don't bother about the statistics
      i_pixel_statistics = 1;
      i_point_statistics = 1;
    }
    if ( i_pixel_statistics == 0 )
    { // should be 1, if not empty
      s_sql = QString( "SELECT UpdateLayerStatistics('%1','gcp_pixel')" ).arg( s_gcp_points_table_name );
      ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
      if ( ret != SQLITE_OK )
      {
        mError = QString( "Unable to execute UpdateLayerStatistics('%1','gcp_pixel'): rc=%2 [%3] sql[%4]\n" ).arg( s_gcp_points_table_name ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
        qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -23- [%1] " ).arg( mError );
        sqlite3_free( errMsg );
        sqlite3_close( db_handle );
        db_handle = NULL;
        if ( cache != NULL )
          spatialite_cleanup_ex( cache );
        spatialite_shutdown();
        return false;
      }
    }
    if ( i_point_statistics == 0 )
    {  // should be 1, if not empty
      s_sql = QString( "SELECT UpdateLayerStatistics('%1','gcp_point')" ).arg( s_gcp_points_table_name );
      ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
      if ( ret != SQLITE_OK )
      {
        mError = QString( "Unable to execute UpdateLayerStatistics('%1','gcp_point'): rc=%2 [%3] sql[%4]\n" ).arg( s_gcp_points_table_name ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
        qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -24- [%1] " ).arg( mError );
        sqlite3_free( errMsg );
        sqlite3_close( db_handle );
        db_handle = NULL;
        if ( cache != NULL )
          spatialite_cleanup_ex( cache );
        spatialite_shutdown();
        return false;
      }
    }
    // End TRANSACTION
    ret = sqlite3_exec( db_handle, "COMMIT", NULL, NULL, 0 );
    Q_ASSERT( ret == SQLITE_OK );
    Q_UNUSED( ret );
  }
  // --- Cutline common sql for TABLE creation
  if (( !b_cutline_geometries_create ) )
  { // no cutline geometries, create them
    qDebug() << QString( "-I-> QgsGeorefPluginGui::createGCPDb creating cutline geomtries for[%1] " ).arg( "points, linestrings, polygons" );
    // Start TRANSACTION
    ret = sqlite3_exec( db_handle, "BEGIN", NULL, NULL, 0 );
    Q_ASSERT( ret == SQLITE_OK );
    Q_UNUSED( ret );
    // --- Cutline common sql for TABLE creation
    QString s_sql_cutline = QString( "CREATE TABLE IF NOT EXISTS 'create_cutline_%1'\n( --%2: to build %3 to assist georeferencing\n" );
    s_sql_cutline += QString( " %1 INTEGER PRIMARY KEY AUTOINCREMENT,\n" ).arg( "id_cutline" );
    s_sql_cutline += QString( " %1 INTEGER DEFAULT 0,\n" ).arg( "id_gcp_coverage" );
    s_sql_cutline += QString( " %1 TEXT DEFAULT '',\n" ).arg( "name" );
    s_sql_cutline += QString( " %1 TEXT DEFAULT '',\n" ).arg( "notes" );
    s_sql_cutline += QString( " %1 TEXT DEFAULT '',\n" ).arg( "text" );
    s_sql_cutline += QString( " %1 TEXT DEFAULT '',\n" ).arg( "belongs_to_01" );
    s_sql_cutline += QString( " %1 TEXT DEFAULT '',\n" ).arg( "belongs_to_02" );
    s_sql_cutline += QString( " %1 DATE DEFAULT '0001-01-01',\n" ).arg( "valid_since" );
    s_sql_cutline += QString( " %1 DATE DEFAULT '3000-01-01')\n" ).arg( "valid_until" );
    // --- Cutline POINTs to assist georeferencing
    s_sql = QString( s_sql_cutline ).arg( "points" ).arg( "POINT" ).arg( "POINTs" );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create TABLE create_cutline_points: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -25- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    s_sql = QString( "SELECT AddGeometryColumn('create_cutline_points','cutline_point',%1,'POINT','XY')" ).arg( i_gcp_srid );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create geometry column cutline_point: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -26- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    s_sql = QString( "SELECT CreateSpatialIndex('create_cutline_points','cutline_point')" );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create CreateSpatialIndex for cutline_point: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -27- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    // --- Cutline LINESTRINGs to assist georeferencing
    s_sql = QString( s_sql_cutline ).arg( "linestrings" ).arg( "LINESTRING" ).arg( "LINESTRINGs" );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create TABLE create_cutline_linestrings: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -28- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    s_sql = QString( "SELECT AddGeometryColumn('create_cutline_linestrings','cutline_linestring',%1,'LINESTRING','XY')" ).arg( i_gcp_srid );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create geometry column cutline_linestring: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -29- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    s_sql = QString( "SELECT CreateSpatialIndex('create_cutline_linestrings','cutline_linestring')" );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create CreateSpatialIndex for cutline_linestring: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -30- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    // --- Cutline POLYGONs to assist georeferencing
    s_sql = QString( s_sql_cutline ).arg( "polygons" ).arg( "POLYGON" ).arg( "POLYGONs" );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create TABLE create_cutline_polygons: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -31- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    s_sql = QString( "SELECT AddGeometryColumn('create_cutline_polygons','cutline_polygon',%1,'POLYGON','XY')" ).arg( i_gcp_srid );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create geometry column cutline_polygon: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -32- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    s_sql = QString( "SELECT CreateSpatialIndex('create_cutline_polygons','cutline_polygon')" );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to create CreateSpatialIndex for cutline_polygon: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -33- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
      sqlite3_close( db_handle );
      db_handle = NULL;
      if ( cache != NULL )
        spatialite_cleanup_ex( cache );
      spatialite_shutdown();
      return false;
    }
    // End TRANSACTION
    ret = sqlite3_exec( db_handle, "COMMIT", NULL, NULL, 0 );
    Q_ASSERT( ret == SQLITE_OK );
    Q_UNUSED( ret );
  }
  if (( b_import_points ) && ( pointFile.exists() ) )
  {
    qDebug() << QString( "-I-> QgsGeorefPluginGui::createGCPDb mGCPpointsFileName[%1] " ).arg( mGCPpointsFileName );
    if ( pointFile.open( QIODevice::ReadOnly ) )
    {
      // Start TRANSACTION
      ret = sqlite3_exec( db_handle, "BEGIN", NULL, NULL, 0 );
      Q_ASSERT( ret == SQLITE_OK );
      Q_UNUSED( ret );
      QTextStream points( &pointFile );
      QString line = points.readLine();
      while ( !points.atEnd() )
      {
        line = points.readLine();
        QStringList ls;
        if ( line.contains( QRegExp( "," ) ) ) // in previous format "\t" is delimiter of points in new - ","
        {  // points from new georeferencer
          ls = line.split( "," );
        }
        else
        {  // points from prev georeferencer
          ls = line.split( "\t" );
        }
        // QgsPoint mapCoords( ls.at( 0 ).toDouble(), ls.at( 1 ).toDouble() ); // map x,y
        // QgsPoint pixelCoords( ls.at( 2 ).toDouble(), ls.at( 3 ).toDouble() ); // pixel x,y
        s_sql = QString( "INSERT INTO \"%1\" (name,id_gcp_coverage,srid,pixel_x,pixel_y,point_x,point_y,gcp_enable)\n" ).arg( s_gcp_points_table_name );
        s_sql += QString( "SELECT '%1' AS name," ).arg( mGCPbaseFileName );
        s_sql += QString( "%1 AS id_gcp_coverage," ).arg( id_gcp_coverage );
        s_sql += QString( "%1 AS srid," ).arg( i_gcp_srid );
        s_sql += QString( "%1 AS pixel_x," ).arg( ls.at( 2 ).simplified() );
        s_sql += QString( "%1 AS pixel_y," ).arg( ls.at( 3 ).simplified() );
        s_sql += QString( "%1 AS point_x," ).arg( ls.at( 0 ).simplified() );
        s_sql += QString( "%1 AS point_y," ).arg( ls.at( 1 ).simplified() );
        s_sql += QString( "%1 AS gcp_enable" ).arg( ls.at( 4 ).simplified() );
        ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
        if ( ret != SQLITE_OK )
        {
          mError = QString( "Unable to INSERT INTO '%1': rc=%2 [%3] sql[%4]\n" ).arg( s_gcp_points_table_name ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
          qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -34- [%1] " ).arg( mError );
          sqlite3_free( errMsg );
        }
      }
      s_sql = QString( "SELECT UpdateLayerStatistics('%1','gcp_pixel')" ).arg( s_gcp_points_table_name );
      ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
      if ( ret != SQLITE_OK )
      {
        mError = QString( "Unable to execute UpdateLayerStatistics('%1','gcp_pixel'): rc=%2 [%3] sql[%4]\n" ).arg( s_gcp_points_table_name ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
        qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -35- [%1] " ).arg( mError );
        sqlite3_free( errMsg );
        sqlite3_close( db_handle );
        db_handle = NULL;
        if ( cache != NULL )
          spatialite_cleanup_ex( cache );
        spatialite_shutdown();
        return false;
      }
      s_sql = QString( "SELECT UpdateLayerStatistics('%1','gcp_point')" ).arg( s_gcp_points_table_name );
      ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
      if ( ret != SQLITE_OK )
      {
        mError = QString( "Unable to execute UpdateLayerStatistics('%1','gcp_point'): rc=%2 [%3] sql[%4]\n" ).arg( s_gcp_points_table_name ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
        qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -36- [%1] " ).arg( mError );
        sqlite3_free( errMsg );
        sqlite3_close( db_handle );
        db_handle = NULL;
        if ( cache != NULL )
          spatialite_cleanup_ex( cache );
        spatialite_shutdown();
        return false;
      }
      // End TRANSACTION
      ret = sqlite3_exec( db_handle, "COMMIT", NULL, NULL, 0 );
      Q_ASSERT( ret == SQLITE_OK );
      Q_UNUSED( ret );
    }
  }
  sqlite3_close( db_handle );
  db_handle = NULL;
  if ( cache != NULL )
    spatialite_cleanup_ex( cache );
  spatialite_shutdown();
  // Update extent of active raster
  return updateGCPDb( mGCPbaseFileName.toLower() );
}
bool QgsGeorefPluginGui::updateGCPDb( QString s_coverage_name )
{
  bool b_database_exists = QFile( mGCPdatabaseFileName ).exists();
  if ( b_database_exists )
  {
    void *cache;
    char *errMsg = NULL;
    sqlite3* db_handle;
    // open database
    int ret = sqlite3_open_v2( mGCPdatabaseFileName.toUtf8().data(), &db_handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Failed to open database: rc=%1 database[%2]" ).arg( ret ).arg( mGCPdatabaseFileName );
      sqlite3_close( db_handle );
      db_handle = NULL;
      return false;
    }
    // needed by spatialite_init_ex (multi-threading)
    cache = spatialite_alloc_connection();
    // load spatialite extension
    spatialite_init_ex( db_handle, cache, 0 );
    QString s_gcp_points_table_name = QString( "gcp_points_%1" ).arg( s_coverage_name.toLower() );
    QString s_sql = QString( "UPDATE \"gcp_coverages\" SET\n" );
    s_sql += QString( " gcp_count=(SELECT count(id_gcp) FROM \"%1\" WHERE ((gcp_point IS NOT NULL) AND (gcp_enable <> 0)))," ).arg( s_gcp_points_table_name );
    s_sql += QString( " extent_minx=(SELECT min(ST_X(gcp_point)) FROM \"%1\" WHERE ((gcp_point IS NOT NULL) AND (gcp_enable <> 0)))," ).arg( s_gcp_points_table_name );
    s_sql += QString( " extent_miny=(SELECT min(ST_Y(gcp_point)) FROM \"%1\" WHERE ((gcp_point IS NOT NULL) AND (gcp_enable <> 0)))," ).arg( s_gcp_points_table_name );
    s_sql += QString( " extent_maxx=(SELECT max(ST_X(gcp_point)) FROM \"%1\" WHERE ((gcp_point IS NOT NULL) AND (gcp_enable <> 0)))," ).arg( s_gcp_points_table_name );
    s_sql += QString( " extent_maxy=(SELECT max(ST_Y(gcp_point)) FROM \"%1\" WHERE ((gcp_point IS NOT NULL) AND (gcp_enable <> 0)))" ).arg( s_gcp_points_table_name );
    s_sql += QString( " WHERE (coverage_name='%1')" ).arg( s_coverage_name.toLower() );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      mError = QString( "Unable to execute Update of gcp_coverages for '%1': rc=%2 [%3] sql[%4]\n" ).arg( s_gcp_points_table_name ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsGeorefPluginGui::createGCPDb -37- [%1] " ).arg( mError );
      sqlite3_free( errMsg );
    }
    sqlite3_close( db_handle );
    db_handle = NULL;
    if ( cache != NULL )
      spatialite_cleanup_ex( cache );
    spatialite_shutdown();
  }
  return b_database_exists;
}
