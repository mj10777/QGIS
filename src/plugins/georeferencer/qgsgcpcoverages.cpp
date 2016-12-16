/***************************************************************************
               qgsgcpcoverages.cpp  - GcpCoverages
                             -------------------
    begin                : 2011-04-01
    copyright            : (C) 2011 Radim Blazek
    email                : radim dot blazek at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "qgsgcpcoverages.h"

#include <QAction>
#include <QDir>
#include <QDockWidget>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QPushButton>
#include <QKeySequence>
#include <QToolButton>
#include <QStatusBar>
#include <QMetaObject>
#include <QDebug>

#ifdef Q_OS_MACX
QgsGcpCoveragesDialog::QgsGcpCoveragesDialog( QWidget *parent, QgsSpatiaLiteProviderGcpUtils::GcpDbData* parms_GcpDbData )
    : QMainWindow( parent, Qt::WindowSystemMenuHint )  // Modeless dialog with close button only
#else
QgsGcpCoveragesDialog::QgsGcpCoveragesDialog( QWidget *parent, QgsSpatiaLiteProviderGcpUtils::GcpDbData* parms_GcpDbData )
    : QMainWindow( parent )
#endif
{
  mGcpDbData = parms_GcpDbData;
  i_selected_gcp_coverage = mGcpDbData->mId_gcp_coverage;
  setupUi( this );
  buttonBox->button( QDialogButtonBox::Reset )->setText( "Load Raster" );
  buttonBox->button( QDialogButtonBox::Reset )->setToolTip( QString( "Load Raster into %1, leaving this Dialog Open" ).arg( "Georeferencer" ) );
  buttonBox->button( QDialogButtonBox::Ok )->setToolTip( QString( "Close dialog, without loading Raster into %1." ).arg( "Georeferencer" ) );
  QSettings appSettings;
  restoreGeometry( appSettings.value( "/Windows/GcpCoverages/geometry" ).toByteArray() );
  init();
  QStringList myHeaders;
  myHeaders << tr( "Coverage Name / Column Value" ) << tr( "Title / Column-Names" ) << tr( "Abstract / Column-Desription" );
  treeWidget->setHeaderLabels( myHeaders );
  mLastDirSettingsName  = QLatin1String( "/UI/lastGcpCoveragesDir" );
  connect( buttonBox->button( QDialogButtonBox::Ok ), SIGNAL( clicked() ), this, SLOT( ok() ) );
  connect( buttonBox->button( QDialogButtonBox::Reset ), SIGNAL( clicked() ), this, SLOT( load_selected_coverage() ) );
}

QgsGcpCoveragesDialog::~QgsGcpCoveragesDialog()
{
  QSettings settings;
  settings.setValue( "/Windows/GcpCoverages/geometry", saveGeometry() );
}


void QgsGcpCoveragesDialog::load_selected_coverage()
{
  if ( mGcpDbData->mId_gcp_coverage != i_selected_gcp_coverage )
  { // Do not load a raster that is already loaded
    QString s_value = mGcpDbData->gcp_coverages.value( i_selected_gcp_coverage );
    QStringList sa_fields = s_value.split( mGcpDbData->mParseString );
    if ( sa_fields.count() >= 23 )
    {
      QFile raster_file( QString( "%1/%2" ).arg( sa_fields.at( 22 ) ).arg( sa_fields.at( 3 ) ) );
      if ( raster_file.exists() )
      { // Do not load a raster that does not exist
        emit selectedRasterCoverage( i_selected_gcp_coverage );
      }
    }
  }
}

void QgsGcpCoveragesDialog::ok()
{
  hide();
}

void QgsGcpCoveragesDialog::on_actionExpandAll_triggered( bool checked )
{
  Q_UNUSED( checked );
  treeWidget->expandAll();
}

void QgsGcpCoveragesDialog::on_actionCollapseAll_triggered( bool checked )
{
  Q_UNUSED( checked );
  treeWidget->collapseAll();
  for ( int i = 0; i < treeWidget->topLevelItemCount(); i++ )
  {
    treeWidget->expandItem( treeWidget->topLevelItem( i ) );
  }
}

void QgsGcpCoveragesDialog::init()
{
  QTreeWidgetItem * wi = createTreeItemCoverages();
  if ( wi )
  {
    treeWidget->insertTopLevelItem( 0, wi );
    treeWidget->expandItem( wi );
  }
  for ( int i = 0; i < treeWidget->topLevelItemCount(); i++ )
  {
    treeWidget->expandItem( treeWidget->topLevelItem( i ) );
  }
  for ( int i = 0; i < treeWidget->columnCount(); i++ )
  {
    treeWidget->resizeColumnToContents( i );
  }
  if ( item_selected_gcp_coverage )
  {
    treeWidget->expandItem( item_selected_gcp_coverage );
    buttonBox->button( QDialogButtonBox::Reset )->setText( s_selected_gcp_coverage_text );
    item_selected_gcp_coverage->setSelected( true );
    // item_selected_gcp_coverage->setExpanded( true );
  }
  connect( treeWidget, SIGNAL( itemClicked( QTreeWidgetItem*, int ) ), this , SLOT( tree_coverage_clicked( QTreeWidgetItem*, int ) ) );
}
void QgsGcpCoveragesDialog::tree_coverage_clicked( QTreeWidgetItem *item, int column )
{
  Q_UNUSED( column );
  if ( item->type() >= 1000 )
  {
    i_selected_gcp_coverage = item->type() - 1000;
    QString s_value = mGcpDbData->gcp_coverages.value( i_selected_gcp_coverage );
    QStringList sa_fields = s_value.split( mGcpDbData->mParseString );
    QString s_text = sa_fields.at( 5 ); // title
    if ( s_text.isEmpty() )
      s_text = sa_fields.at( 1 ); // coverage_name
    if ( item->childCount() > 0 )
    { // This is the Main-coverage Item
      item_selected_gcp_coverage = item;
      if ( !item_selected_gcp_coverage->isExpanded() )
      {
        item_selected_gcp_coverage->setExpanded( true );
      }
    }
    else
    { // This is one of the Conferage Information Items, use parent as selected Item
      item_selected_gcp_coverage = item->parent();
    }
    item_selected_gcp_coverage->setSelected( true );
    if ( mGcpDbData->mId_gcp_coverage != i_selected_gcp_coverage )
    {
      s_selected_gcp_coverage_text = QString( "Load Raster: '%1'" ).arg( s_text );
    }
    else
    {
      s_selected_gcp_coverage_text = QString( "Active Raster: '%1'" ).arg( s_text );
    }
    buttonBox->button( QDialogButtonBox::Reset )->setText( s_selected_gcp_coverage_text );
  }
}

QTreeWidgetItem * QgsGcpCoveragesDialog::createTreeItemCoverages()
{
  mGcpCoveragesDatabase->setText( mGcpDbData->mGCPdatabaseFileName );
  // qDebug() << QString( "QgsGcpCoveragesDialog::createTreeItemCoverages[%1] [%2]" ).arg( mGcpDbData->gcp_coverages.count() ).arg( mGcpDbData->mGCPdatabaseFileName );
  QStringList root_data( "Gcp Coverages" );
  root_data << QString( "List of maps for this project" );
  root_data << QString( "Each coverage will contain information about the image " );
  QTreeWidgetItem *root_Item = new QTreeWidgetItem( root_data, 0 );
  root_Item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
  // root_Item->setCheckState( 0, Qt::Checked );
  QMap<int, QString>::iterator it_map_coverages;
  for ( it_map_coverages = mGcpDbData->gcp_coverages.begin(); it_map_coverages != mGcpDbData->gcp_coverages.end(); ++it_map_coverages )
  {
    int id_gcp_coverage = it_map_coverages.key();
    QString s_value = it_map_coverages.value();
    QStringList sa_fields = s_value.split( mGcpDbData->mParseString );
    // qDebug() << QString( "QgsGcpCoveragesDialog::createTreeItemCoverages[%1] count_fileds[%2] value[%3]" ).arg( id_gcp_coverage ).arg(sa_fields.count()).arg(s_value);
    QTreeWidgetItem *coverage_Item = createTreeItemCoverage( id_gcp_coverage + 1000, sa_fields );
    root_Item->addChild( coverage_Item );
  }
  // Do not translate "Widgets", currently it is also used as path
  root_Item->setData( 0, Qt::DisplayRole, "Gcp Coverages" );
  return root_Item;
}
QTreeWidgetItem * QgsGcpCoveragesDialog::createTreeItemCoverage( int id_gcp_coverage, QStringList sa_fields )
{
  // myHeaders << tr( "Coverage Name" ) << tr( "Title" ) << tr( "Abstract" );
  QStringList coverage_data;
  QString s_id_gcp_coverage = QString( "id_gcp_coverage=%1" ).arg( sa_fields.at( 0 ) );
  QString s_coverage_name = sa_fields.at( 1 ); // coverage_name
  QString s_map_date = QString( "map_date=%1" ).arg( sa_fields.at( 2 ) );
  QString s_raster_file = QString( "raster_file=%1" ).arg( sa_fields.at( 3 ) );
  QString s_raster_gtif = QString( "raster_gtif=%1" ).arg( sa_fields.at( 4 ) );
  QString s_title = sa_fields.at( 5 ); // title
  QString s_abstract = sa_fields.at( 6 ); // abstract
  coverage_data << s_coverage_name << s_title << s_abstract;
  QTreeWidgetItem *coverage_Item = new QTreeWidgetItem( coverage_data, id_gcp_coverage );
  QString s_copyright = QString( "copyright=%1" ).arg( sa_fields.at( 7 ) );
  QString s_scale = QString( "scale=%1" ).arg( sa_fields.at( 8 ) );
  QString s_nodata = QString( "notada=%1" ).arg( sa_fields.at( 9 ) );
  QString s_srid = QString( "srid=%1" ).arg( sa_fields.at( 10 ) );
  QString s_id_cutline = QString( "id_cutline=%1" ).arg( sa_fields.at( 11 ) );
  QString s_gcp_count = QString( "gcp_count=%1" ).arg( sa_fields.at( 12 ) );
  // 0=ThinPlateSpline
  QString s_transform = "ThinPlateSpline";
  switch ( sa_fields.at( 13 ).toInt() )
  {
    case 1:
      s_transform = "PolynomialOrder1";
      break;
    case 2:
      s_transform = "PolynomialOrder2";
      break;
    case 3:
      s_transform = "PolynomialOrder3";
      break;
  }
  QString s_transformtype = QString( "transformtype=%1 [%2]" ).arg( sa_fields.at( 13 ) ).arg( s_transform );
  QString s_resampling = QString( "resampling=%1" ).arg( sa_fields.at( 14 ) );
  QString s_compression = QString( "compression=%1" ).arg( sa_fields.at( 15 ) );
  QString s_image = QString( "image=%1 x %2" ).arg( sa_fields.at( 16 ) ).arg( sa_fields.at( 17 ) );
  QString s_extent_ewkt = QString( "'SRID=%1;POLYGON((%2 %3,%4 %3,%4 %5,%2 %5,%2 %3))'" ).arg( sa_fields.at( 10 ) ).arg( sa_fields.at( 18 ) ).arg( sa_fields.at( 19 ) ).arg( sa_fields.at( 20 ) ).arg( sa_fields.at( 21 ) );
  QString s_raster_path = QString( "raster_path=%1" ).arg( sa_fields.at( 22 ) );
  for ( int i = 0;i < 5;i++ )
  {
    QStringList item_data;
    switch ( i )
    {
      case 0:
        item_data << s_id_gcp_coverage << s_map_date << s_copyright ;
        break;
      case 1:
        item_data << s_srid << s_scale << s_nodata ;
        break;
      case 2:
        item_data << s_id_cutline << s_gcp_count << QString( "%1 ; %2 ; %3" ).arg( s_transformtype ).arg( s_resampling ).arg( s_compression );
        break;
      case 3:
        item_data  << s_image << "Gcp-Points Extent as EWKT:" << s_extent_ewkt;
        break;
      case 4:
        item_data << s_raster_file << s_raster_gtif << s_raster_path;
        break;
    }
    QTreeWidgetItem *field_Item = new QTreeWidgetItem( item_data, id_gcp_coverage );
    coverage_Item->addChild( field_Item );
  }
  if (( mGcpDbData->mId_gcp_coverage == ( id_gcp_coverage - 1000 ) ) ||
       ( i_selected_gcp_coverage < 0 ))
  {
    QString s_text = sa_fields.at( 5 ); // title
    if ( s_text.isEmpty() )
      s_text = sa_fields.at( 1 ); // coverage_name
    if ( i_selected_gcp_coverage < 0 )
    { // first timed called, before any Raster was loaded [will take first]
      i_selected_gcp_coverage=id_gcp_coverage - 1000;
      s_selected_gcp_coverage_text = QString( "Load Raster: '%1'" ).arg( s_text );
    }
    else
    {
     s_selected_gcp_coverage_text = QString( "Active Raster: '%1'" ).arg( s_text );
    }
    item_selected_gcp_coverage = coverage_Item;
  }
  if ( !item_selected_gcp_coverage)
  { // Set the first found as the default selected
    QString s_text = sa_fields.at( 5 ); // title
    if ( s_text.isEmpty() )
      s_text = sa_fields.at( 1 ); // coverage_name
    i_selected_gcp_coverage=id_gcp_coverage - 1000;
    s_selected_gcp_coverage_text = QString( "Load Raster: '%1'" ).arg( s_text );
    item_selected_gcp_coverage = coverage_Item;
  }
  return coverage_Item;
}

QgsGcpCoverages *QgsGcpCoverages::pinstance = nullptr;
QgsGcpCoverages *QgsGcpCoverages::instance()
{
  if ( !pinstance )
  {
    pinstance = new QgsGcpCoverages();
  }
  return pinstance;
}

QgsGcpCoverages::QgsGcpCoverages()
    : pDialog( nullptr )
    , mGcpDbData( nullptr )
{
  id_gcp_coverage = -1;
}

QgsGcpCoverages::~QgsGcpCoverages()
{
  mGcpDbData = nullptr;
  pDialog = nullptr;
}

void QgsGcpCoverages::openDialog( QWidget *parent,  QgsSpatiaLiteProviderGcpUtils::GcpDbData* parms_GcpDbData )
{ // Check if anything has changed, which needs a reloading
  bool b_valid = true;
  if ( !parms_GcpDbData )
  {
    return;
  }
  if (( !pDialog ) || ( !mGcpDbData ) )
  {
    b_valid = false;
  }
  else
  { // different pointer, amount of coverages or Database-patch has changed
    if (( parms_GcpDbData != mGcpDbData ) ||
        ( parms_GcpDbData->gcp_coverages.count() != mGcpDbData->gcp_coverages.count() ) ||
        ( parms_GcpDbData->mGCPdatabaseFileName != mGcpDbData->mGCPdatabaseFileName ) )
    { // then rebuild the dialog
      b_valid = false;
      disconnect( pDialog, SIGNAL( selectedRasterCoverage( int ) ), this, SLOT( load_selected_coverage( int ) ) );
      pDialog = nullptr;
    }
  }
  if ( !b_valid )
  {
    mGcpDbData = parms_GcpDbData;
    pDialog = new QgsGcpCoveragesDialog( parent, parms_GcpDbData );
    connect( pDialog, SIGNAL( selectedRasterCoverage( int ) ), this, SLOT( load_selected_coverage( int ) ) );
  }
  // I am trying too enable switching widget status by clicking in main app, so I need non modal
  pDialog->show();
}

void QgsGcpCoverages::load_selected_coverage( int id_selected_coverage )
{
  if ( mGcpDbData->mId_gcp_coverage != id_selected_coverage )
  { // Do not load a raster that is already loaded
    QString s_value = mGcpDbData->gcp_coverages.value( id_selected_coverage );
    QStringList sa_fields = s_value.split( mGcpDbData->mParseString );
    if ( sa_fields.count() >= 23 )
    { // 22=path ; 3=file
      QFile raster_file( QString( "%1/%2" ).arg( sa_fields.at( 22 ) ).arg( sa_fields.at( 3 ) ) );
      if ( raster_file.exists() )
      { // Do not load a raster that does not exist
        emit loadRasterCoverage( id_selected_coverage );
      }
    }
  }
}
