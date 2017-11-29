/***************************************************************************
 *  qgsgeometrycheckersetuptab.cpp                                         *
 *  -------------------                                                    *
 *  begin                : Jun 10, 2014                                    *
 *  copyright            : (C) 2014 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsgeometrycheckersetuptab.h"
#include "qgsgeometrycheckerresulttab.h"
#include "qgsgeometrychecker.h"
#include "qgsgeometrycheckfactory.h"
#include "qgsgeometrycheck.h"
#include "qgsfeaturepool.h"

#include "qgsfeatureiterator.h"
#include "qgisinterface.h"
#include "qgscrscache.h"
#include "qgsproject.h"
#include "qgsvectorlayer.h"
#include "qgsmapcanvas.h"
#include "qgsvectorfilewriter.h"
#include "qgsvectordataprovider.h"

#include <QAction>
#include <QEventLoop>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QMessageBox>
#include <QPushButton>
#include <QtConcurrentMap>

static const int LayerIdRole = Qt::UserRole + 1;

QgsGeometryCheckerSetupTab::QgsGeometryCheckerSetupTab( QgisInterface *iface, QDialog *checkerDialog, QWidget *parent )
  : QWidget( parent )
  , mIface( iface )
  , mCheckerDialog( checkerDialog )

{
  ui.setupUi( this );
  ui.progressBar->hide();
  ui.labelStatus->hide();
  mRunButton = ui.buttonBox->addButton( tr( "Run" ), QDialogButtonBox::ActionRole );
  mAbortButton = new QPushButton( tr( "Abort" ) );
  mRunButton->setEnabled( false );

  const auto filterFormatMap = QgsVectorFileWriter::supportedFiltersAndFormats( QgsVectorFileWriter::SortRecommended | QgsVectorFileWriter::SkipNonSpatialFormats );
  for ( const QgsVectorFileWriter::FilterFormatDetails &filter : filterFormatMap )
  {
    QString driverName = filter.driverName;
    ui.comboBoxOutputFormat->addItem( driverName );
    if ( driverName == QLatin1String( "ESRI Shapefile" ) )
    {
      ui.comboBoxOutputFormat->setCurrentIndex( ui.comboBoxOutputFormat->count() - 1 );
    }
  }
  ui.listWidgetInputLayers->setIconSize( QSize( 16, 16 ) );

  ui.lineEditFilenamePrefix->setText( QSettings().value( "/geometry_checker/previous_values/filename_prefix", tr( "checked_" ) ).toString() );

  connect( mRunButton, &QAbstractButton::clicked, this, &QgsGeometryCheckerSetupTab::runChecks );
  connect( ui.listWidgetInputLayers, &QListWidget::itemChanged, this, &QgsGeometryCheckerSetupTab::validateInput );
  connect( QgsProject::instance(), &QgsProject::layersAdded, this, &QgsGeometryCheckerSetupTab::updateLayers );
  connect( QgsProject::instance(), static_cast<void ( QgsProject::* )( const QStringList & )>( &QgsProject::layersRemoved ), this, &QgsGeometryCheckerSetupTab::updateLayers );
  connect( ui.radioButtonOutputNew, &QAbstractButton::toggled, ui.frameOutput, &QWidget::setEnabled );
  connect( ui.buttonGroupOutput, static_cast<void ( QButtonGroup::* )( int )>( &QButtonGroup::buttonClicked ), this, &QgsGeometryCheckerSetupTab::validateInput );
  connect( ui.pushButtonOutputDirectory, &QAbstractButton::clicked, this, &QgsGeometryCheckerSetupTab::selectOutputDirectory );
  connect( ui.lineEditOutputDirectory, &QLineEdit::textChanged, this, &QgsGeometryCheckerSetupTab::validateInput );
  connect( ui.checkBoxSliverPolygons, &QAbstractButton::toggled, ui.widgetSliverThreshold, &QWidget::setEnabled );
  connect( ui.checkBoxSliverArea, &QAbstractButton::toggled, ui.doubleSpinBoxSliverArea, &QWidget::setEnabled );
  connect( ui.checkLineLayerIntersection, &QAbstractButton::toggled, ui.comboLineLayerIntersection, &QComboBox::setEnabled );
  connect( ui.checkBoxFollowBoundaries, &QAbstractButton::toggled, ui.comboBoxFollowBoundaries, &QComboBox::setEnabled );

  for ( const QgsGeometryCheckFactory *factory : QgsGeometryCheckFactoryRegistry::getCheckFactories() )
  {
    factory->restorePrevious( ui );
  }
  updateLayers();
}

QgsGeometryCheckerSetupTab::~QgsGeometryCheckerSetupTab()
{
  delete mAbortButton;
}

void QgsGeometryCheckerSetupTab::updateLayers()
{
  QStringList prevCheckedLayers;
  for ( int row = 0, nRows = ui.listWidgetInputLayers->count(); row < nRows; ++row )
  {
    QListWidgetItem *item = ui.listWidgetInputLayers->item( row );
    if ( item->checkState() == Qt::Checked )
    {
      prevCheckedLayers.append( item->data( LayerIdRole ).toString() );
    }
  }
  ui.listWidgetInputLayers->clear();
  ui.comboLineLayerIntersection->clear();
  ui.comboBoxFollowBoundaries->clear();

  // Collect layers
  for ( QgsVectorLayer *layer : QgsProject::instance()->layers<QgsVectorLayer *>() )
  {
    QListWidgetItem *item = new QListWidgetItem( layer->name() );
    bool supportedGeometryType = true;
    if ( layer->geometryType() == QgsWkbTypes::PointGeometry )
    {
      item->setIcon( QgsApplication::getThemeIcon( "/mIconPointLayer.svg" ) );
    }
    else if ( layer->geometryType() == QgsWkbTypes::LineGeometry )
    {
      item->setIcon( QgsApplication::getThemeIcon( "/mIconLineLayer.svg" ) );
      ui.comboLineLayerIntersection->addItem( layer->name(), layer->id() );
    }
    else if ( layer->geometryType() == QgsWkbTypes::PolygonGeometry )
    {
      item->setIcon( QgsApplication::getThemeIcon( "/mIconPolygonLayer.svg" ) );
      ui.comboLineLayerIntersection->addItem( layer->name(), layer->id() );
      ui.comboBoxFollowBoundaries->addItem( layer->name(), layer->id() );
    }
    else
    {
      supportedGeometryType = false;
    }
    item->setToolTip( layer->dataProvider()->dataSourceUri() );
    item->setData( LayerIdRole, layer->id() );
    if ( supportedGeometryType )
    {
      if ( mCheckerDialog->isVisible() )
      {
        // If dialog is visible, only set item to checked if it previously was
        item->setCheckState( prevCheckedLayers.contains( layer->id() ) ? Qt::Checked : Qt::Unchecked );
      }
      else
      {
        // Otherwise, set item to checked
        item->setCheckState( Qt::Checked );
      }
    }
    else
    {
      item->setCheckState( Qt::Unchecked );
      item->setFlags( item->flags() & ~( Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable ) );
    }
    ui.listWidgetInputLayers->addItem( item );
  }
  validateInput();
}

QList<QgsVectorLayer *> QgsGeometryCheckerSetupTab::getSelectedLayers()
{
  QList<QgsVectorLayer *> layers;
  for ( int row = 0, nRows = ui.listWidgetInputLayers->count(); row < nRows; ++row )
  {
    QListWidgetItem *item = ui.listWidgetInputLayers->item( row );
    if ( item->checkState() == Qt::Checked )
    {
      QString layerId = item->data( LayerIdRole ).toString();
      QgsVectorLayer *layer = qobject_cast<QgsVectorLayer *>( QgsProject::instance()->mapLayer( layerId ) );
      if ( layer )
      {
        layers.append( layer );
      }
    }
  }
  return layers;
}

void QgsGeometryCheckerSetupTab::validateInput()
{
  QStringList layerCrs = QStringList() << mIface->mapCanvas()->mapSettings().destinationCrs().authid();
  QList<QgsVectorLayer *> layers = getSelectedLayers();
  int nApplicable = 0;
  int nPoint = 0;
  int nLineString = 0;
  int nPolygon = 0;
  if ( !layers.isEmpty() )
  {
    for ( QgsVectorLayer *layer : layers )
    {
      QgsWkbTypes::GeometryType geomType = layer->geometryType();
      if ( geomType == QgsWkbTypes::PointGeometry )
      {
        ++nPoint;
      }
      else if ( geomType == QgsWkbTypes::LineGeometry )
      {
        ++nLineString;
      }
      else if ( geomType == QgsWkbTypes::PolygonGeometry )
      {
        ++nPolygon;
      }
      layerCrs.append( layer->crs().authid() );
    }
  }
  for ( const QgsGeometryCheckFactory *factory : QgsGeometryCheckFactoryRegistry::getCheckFactories() )
  {
    nApplicable += factory->checkApplicability( ui, nPoint, nLineString, nPolygon );
  }

  bool outputOk = ui.radioButtonOutputModifyInput->isChecked() || !ui.lineEditOutputDirectory->text().isEmpty();
  mRunButton->setEnabled( !layers.isEmpty() && nApplicable > 0 && outputOk );
}

void QgsGeometryCheckerSetupTab::selectOutputDirectory()
{
  QString filterString = QgsVectorFileWriter::filterForDriver( QStringLiteral( "GPKG" ) );
  const auto filterFormatMap = QgsVectorFileWriter::supportedFiltersAndFormats( QgsVectorFileWriter::SortRecommended | QgsVectorFileWriter::SkipNonSpatialFormats );
  for ( const QgsVectorFileWriter::FilterFormatDetails &filter : filterFormatMap )
  {
    QString driverName = filter.driverName;
    if ( driverName != QLatin1String( "ESRI Shapefile" ) ) // Default entry, first in list (see above)
    {
      filterString += ";;" + filter.filterString;
    }
  }
  QString initialdir = ui.lineEditOutputDirectory->text();
  if ( initialdir.isEmpty() || !QDir( initialdir ).exists() )
  {
    for ( const QgsVectorLayer *layer : getSelectedLayers() )
    {
      QDir dir = QFileInfo( layer->dataProvider()->dataSourceUri() ).dir();
      if ( dir.exists() )
      {
        initialdir = dir.absolutePath();
        break;
      }
    }
  }
  if ( initialdir.isEmpty() || !QDir( initialdir ).exists() )
  {
    initialdir = QDir::homePath();
  }
  QString dir = QFileDialog::getExistingDirectory( this, tr( "Select Output Directory" ), initialdir );
  if ( !dir.isEmpty() )
  {
    ui.lineEditOutputDirectory->setText( dir );
  }
}

void QgsGeometryCheckerSetupTab::runChecks()
{
  // Get selected layer
  QList<QgsVectorLayer *> layers = getSelectedLayers();
  if ( layers.isEmpty() )
    return;

  if ( ui.radioButtonOutputNew->isChecked() )
  {
    for ( QgsVectorLayer *layer : layers )
    {
      if ( layer->dataProvider()->dataSourceUri().startsWith( ui.lineEditOutputDirectory->text() ) )
      {
        QMessageBox::critical( this, tr( "Invalid Output Directory" ), tr( "The chosen output directory contains one or more input layers." ) );
        return;
      }
    }
  }
  QgsVectorLayer *lineLayerCheckLayer = ui.comboLineLayerIntersection->isEnabled() ? dynamic_cast<QgsVectorLayer *>( QgsProject::instance()->mapLayer( ui.comboLineLayerIntersection->currentData().toString() ) ) : nullptr;
  QgsVectorLayer *followBoundaryCheckLayer = ui.comboBoxFollowBoundaries->isEnabled() ? dynamic_cast<QgsVectorLayer *>( QgsProject::instance()->mapLayer( ui.comboBoxFollowBoundaries->currentData().toString() ) ) : nullptr;
  if ( layers.contains( lineLayerCheckLayer ) || layers.contains( followBoundaryCheckLayer ) )
  {
    QMessageBox::critical( this, tr( "Error" ), tr( "The test layer set contains a layer selected for a topology check." ) );
    return;
  }

  for ( QgsVectorLayer *layer : layers )
  {
    if ( layer->isEditable() )
    {
      QMessageBox::critical( this, tr( "Editable Input Layer" ), tr( "Input layer '%1' is not allowed to be in editing mode." ).arg( layer->name() ) );
      return;
    }
  }
  bool selectedOnly = ui.checkBoxInputSelectedOnly->isChecked();

  // Set window busy
  setCursor( Qt::WaitCursor );
  mRunButton->setEnabled( false );
  ui.labelStatus->setText( tr( "<b>Preparing output...</b>" ) );
  ui.labelStatus->show();
  QApplication::processEvents( QEventLoop::ExcludeUserInputEvents );

  QList<QgsVectorLayer *> processLayers;
  if ( ui.radioButtonOutputNew->isChecked() )
  {
    // Get output directory and file extension
    QDir outputDir = QDir( ui.lineEditOutputDirectory->text() );
    QString outputDriverName = ui.comboBoxOutputFormat->currentText();
    QgsVectorFileWriter::MetaData metadata;
    if ( !QgsVectorFileWriter::driverMetadata( outputDriverName, metadata ) )
    {
      QMessageBox::critical( this, tr( "Unknown Output Format" ), tr( "The specified output format cannot be recognized." ) );
      mRunButton->setEnabled( true );
      ui.labelStatus->hide();
      unsetCursor();
      return;
    }
    QString outputExtension = metadata.ext;

    // List over input layers, check which existing project layers need to be removed and create output layers
    QString filenamePrefix = ui.lineEditFilenamePrefix->text();
    QSettings().setValue( "/geometry_checker/previous_values/filename_prefix", filenamePrefix );
    QStringList toRemove;
    QStringList createErrors;
    for ( QgsVectorLayer *layer : layers )
    {
      QString outputPath = outputDir.absoluteFilePath( filenamePrefix + layer->name() + "." + outputExtension );

      // Remove existing layer with same uri from project
      for ( QgsVectorLayer *projectLayer : QgsProject::instance()->layers<QgsVectorLayer *>() )
      {
        if ( projectLayer->dataProvider()->dataSourceUri().startsWith( outputPath ) )
        {
          toRemove.append( projectLayer->id() );
        }
      }

      // Create output layer
      QString errMsg;
      QgsVectorFileWriter::WriterError err =  QgsVectorFileWriter::writeAsVectorFormat( layer, outputPath, layer->dataProvider()->encoding(), layer->crs(), outputDriverName, selectedOnly, &errMsg );
      if ( err != QgsVectorFileWriter::NoError )
      {
        createErrors.append( errMsg );
        continue;
      }

      QgsVectorLayer *newlayer = new QgsVectorLayer( outputPath, QFileInfo( outputPath ).completeBaseName(), QStringLiteral( "ogr" ) );
      if ( selectedOnly )
      {
        QgsFeature feature;

        // Get features to select (only selected features were written up to this point)
        QgsFeatureIds selectedFeatures = newlayer->allFeatureIds();

        // Write non-selected feature ids
        QgsFeatureList features;
        QgsFeatureIterator it = layer->getFeatures();
        while ( it.nextFeature( feature ) )
        {
          if ( !layer->selectedFeatureIds().contains( feature.id() ) )
          {
            features.append( feature );
          }
        }
        newlayer->dataProvider()->addFeatures( features );

        // Set selected features
        newlayer->selectByIds( selectedFeatures );
      }
      processLayers.append( newlayer );
    }

    //  Remove layers from project
    if ( !toRemove.isEmpty() )
    {
      QgsProject::instance()->removeMapLayers( toRemove );
    }

    // Error if an output layer could not be created
    if ( !createErrors.isEmpty() )
    {
      QMessageBox::critical( this, tr( "Layer Creation Failed" ), tr( "Failed to create one or more output layers:\n%1" ).arg( createErrors.join( "\n" ) ) );
      mRunButton->setEnabled( true );
      ui.labelStatus->hide();
      unsetCursor();
      return;
    }
  }
  else
  {
    processLayers = layers;
  }

  // Check if output layers are editable
  QList<QgsVectorLayer *> nonEditableLayers;
  for ( QgsVectorLayer *layer : processLayers )
  {
    if ( ( layer->dataProvider()->capabilities() & QgsVectorDataProvider::ChangeGeometries ) == 0 )
    {
      nonEditableLayers.append( layer );
    }
  }
  if ( !nonEditableLayers.isEmpty() )
  {
    QStringList nonEditableLayerNames;
    for ( QgsVectorLayer *layer : nonEditableLayers )
    {
      nonEditableLayerNames.append( layer->name() );
    }
    if ( QMessageBox::Yes != QMessageBox::question( this, tr( "Non-editable Output Layers" ), tr( "The following output layers are in a format that does not support editing features:\n%1\n\nThe geometry check can be performed, but it will not be possible to fix any errors. Do you want to continue?" ).arg( nonEditableLayerNames.join( "\n" ) ), QMessageBox::Yes, QMessageBox::No ) )
    {
      if ( ui.radioButtonOutputNew->isChecked() )
      {
        for ( QgsVectorLayer *layer : processLayers )
        {
          QString layerPath = layer->dataProvider()->dataSourceUri();
          delete layer;
          if ( ui.comboBoxOutputFormat->currentText() == QLatin1String( "ESRI Shapefile" ) )
          {
            QgsVectorFileWriter::deleteShapeFile( layerPath );
          }
          else
          {
            QFile( layerPath ).remove();
          }
        }
        mRunButton->setEnabled( true );
        ui.labelStatus->hide();
        unsetCursor();
      }
      return;
    }
  }

  // Setup checker
  ui.labelStatus->setText( tr( "<b>Building spatial index...</b>" ) );
  QApplication::processEvents( QEventLoop::ExcludeUserInputEvents );
  QMap<QString, QgsFeaturePool *> featurePools;
  for ( QgsVectorLayer *layer : processLayers )
  {
    double layerToMapUntis = mIface->mapCanvas()->mapSettings().layerToMapUnits( layer );
    QgsCoordinateTransform layerToMapTransform = QgsCoordinateTransformCache::instance()->transform( layer->crs().authid(), mIface->mapCanvas()->mapSettings().destinationCrs().authid() );
    featurePools.insert( layer->id(), new QgsFeaturePool( layer, layerToMapUntis, layerToMapTransform, selectedOnly ) );
  }
  // LineLayerIntersection check is enabled, make sure there is also a feature pool for that layer
  if ( ui.checkLineLayerIntersection->isChecked() && !featurePools.keys().contains( ui.comboLineLayerIntersection->currentData().toString() ) )
  {
    QgsVectorLayer *layer = dynamic_cast<QgsVectorLayer *>( QgsProject::instance()->mapLayer( ui.comboLineLayerIntersection->currentData().toString() ) );
    Q_ASSERT( layer );
    double layerToMapUntis = mIface->mapCanvas()->mapSettings().layerToMapUnits( layer );
    QgsCoordinateTransform layerToMapTransform = QgsCoordinateTransformCache::instance()->transform( layer->crs().authid(), mIface->mapCanvas()->mapSettings().destinationCrs().authid() );
    featurePools.insert( layer->id(), new QgsFeaturePool( layer, layerToMapUntis, layerToMapTransform, selectedOnly ) );
  }

  QgsGeometryCheckerContext *context = new QgsGeometryCheckerContext( ui.spinBoxTolerance->value(), mIface->mapCanvas()->mapSettings().destinationCrs().authid(), featurePools );

  QList<QgsGeometryCheck *> checks;
  for ( const QgsGeometryCheckFactory *factory : QgsGeometryCheckFactoryRegistry::getCheckFactories() )
  {
    QgsGeometryCheck *check = factory->createInstance( context, ui );
    if ( check )
    {
      checks.append( check );
    }
  }
  QgsGeometryChecker *checker = new QgsGeometryChecker( checks, context );

  emit checkerStarted( checker );

  if ( ui.radioButtonOutputNew->isChecked() )
  {
    QList<QgsMapLayer *> addLayers;
    for ( QgsVectorLayer *layer : processLayers )
    {
      addLayers.append( layer );
    }
    QgsProject::instance()->addMapLayers( addLayers );
  }

  // Run
  ui.buttonBox->addButton( mAbortButton, QDialogButtonBox::ActionRole );
  mRunButton->hide();
  ui.progressBar->setRange( 0, 0 );
  ui.labelStatus->hide();
  ui.progressBar->show();
  ui.widgetInputs->setEnabled( false );
  QEventLoop evLoop;
  QFutureWatcher<void> futureWatcher;
  connect( checker, &QgsGeometryChecker::progressValue, ui.progressBar, &QProgressBar::setValue );
  connect( &futureWatcher, &QFutureWatcherBase::finished, &evLoop, &QEventLoop::quit );
  connect( mAbortButton, &QAbstractButton::clicked, &futureWatcher, &QFutureWatcherBase::cancel );
  connect( mAbortButton, &QAbstractButton::clicked, this, &QgsGeometryCheckerSetupTab::showCancelFeedback );

  int maxSteps = 0;
  futureWatcher.setFuture( checker->execute( &maxSteps ) );
  ui.progressBar->setRange( 0, maxSteps );
  evLoop.exec();

  // Restore window
  unsetCursor();
  mAbortButton->setEnabled( true );
  ui.buttonBox->removeButton( mAbortButton );
  mRunButton->setEnabled( true );
  mRunButton->show();
  ui.progressBar->hide();
  ui.labelStatus->hide();
  ui.widgetInputs->setEnabled( true );

  // Show result
  emit checkerFinished( !futureWatcher.isCanceled() );
}

void QgsGeometryCheckerSetupTab::showCancelFeedback()
{
  mAbortButton->setEnabled( false );
  ui.labelStatus->setText( tr( "<b>Waiting for running checks to finish...</b>" ) );
  ui.labelStatus->show();
  ui.progressBar->hide() ;
}
