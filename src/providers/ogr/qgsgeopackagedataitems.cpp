/***************************************************************************
    qgsgeopackagedataitems.h
    ---------------------
    begin                : August 2017
    copyright            : (C) 2017 by Alessandro Pasotti
    email                : apasotti at boundlessgeo dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <sqlite3.h>

#include "qgssqliteutils.h"
#include "qgsgeopackagedataitems.h"
#include "qgsogrdbconnection.h"
#include "qgslogger.h"
#include "qgssettings.h"
#include "qgsproject.h"
#include "qgsvectorlayer.h"
#include "qgsrasterlayer.h"
#include "qgsogrprovider.h"
#include "qgsogrdataitems.h"
#ifdef HAVE_GUI
#include "qgsnewgeopackagelayerdialog.h"
#endif
#include "qgsmessageoutput.h"
#include "qgsvectorlayerexporter.h"
#include "qgsgeopackagerasterwritertask.h"
#include "qgstaskmanager.h"

#include <QAction>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>

QGISEXTERN bool deleteLayer( const QString &uri, const QString &errCause );

QgsDataItem *QgsGeoPackageDataItemProvider::createDataItem( const QString &path, QgsDataItem *parentItem )
{
  QgsDebugMsg( "path = " + path );
  if ( path.isEmpty() )
  {
    return new QgsGeoPackageRootItem( parentItem, QStringLiteral( "GeoPackage" ), QStringLiteral( "gpkg:" ) );
  }
  return nullptr;
}

QgsGeoPackageRootItem::QgsGeoPackageRootItem( QgsDataItem *parent, const QString &name, const QString &path )
  : QgsDataCollectionItem( parent, name, path )
{
  mCapabilities |= Fast;
  mIconName = QStringLiteral( "mGeoPackage.svg" );
  populate();
}

QVector<QgsDataItem *> QgsGeoPackageRootItem::createChildren()
{
  QVector<QgsDataItem *> connections;
  const QStringList connList( QgsOgrDbConnection::connectionList( QStringLiteral( "GPKG" ) ) );
  for ( const QString &connName : connList )
  {
    QgsOgrDbConnection connection( connName, QStringLiteral( "GPKG" ) );
    QgsDataItem *conn = new QgsGeoPackageConnectionItem( this, connection.name(), connection.uri().encodedUri() );

    connections.append( conn );
  }
  return connections;
}

#ifdef HAVE_GUI
QList<QAction *> QgsGeoPackageRootItem::actions( QWidget *parent )
{
  QList<QAction *> lst;

  QAction *actionNew = new QAction( tr( "New Connection..." ), parent );
  connect( actionNew, &QAction::triggered, this, &QgsGeoPackageRootItem::newConnection );
  lst.append( actionNew );

  QAction *actionCreateDatabase = new QAction( tr( "Create Database..." ), parent );
  connect( actionCreateDatabase, &QAction::triggered, this, &QgsGeoPackageRootItem::createDatabase );
  lst.append( actionCreateDatabase );

  return lst;
}

QWidget *QgsGeoPackageRootItem::paramWidget()
{
  return nullptr;
}

void QgsGeoPackageRootItem::onConnectionsChanged()
{
  refresh();
}

void QgsGeoPackageRootItem::newConnection()
{
  if ( QgsOgrDataCollectionItem::createConnection( QStringLiteral( "GeoPackage" ),  QStringLiteral( "GeoPackage Database (*.gpkg)" ),  QStringLiteral( "GPKG" ) ) )
  {
    refreshConnections();
  }
}

void QgsGeoPackageRootItem::createDatabase()
{
  QgsNewGeoPackageLayerDialog dialog( nullptr );
  dialog.setCrs( QgsProject::instance()->defaultCrsForNewLayers() );
  if ( dialog.exec() == QDialog::Accepted )
  {
    if ( QgsOgrDataCollectionItem::storeConnection( dialog.databasePath(), QStringLiteral( "GPKG" ) ) )
    {
      refreshConnections();
    }
  }
}
#endif


QgsGeoPackageCollectionItem::QgsGeoPackageCollectionItem( QgsDataItem *parent, const QString &name, const QString &path )
  : QgsDataCollectionItem( parent, name, path )
  , mPath( path )
{
  mToolTip = path;
  mCapabilities |= Collapse;
}



QVector<QgsDataItem *> QgsGeoPackageCollectionItem::createChildren()
{
  QVector<QgsDataItem *> children;
  const auto layers = QgsOgrLayerItem::subLayers( mPath, QStringLiteral( "GPKG" ) );
  for ( const QgsOgrDbLayerInfo *info : layers )
  {
    if ( info->layerType() == QgsLayerItem::LayerType::Raster )
    {
      children.append( new QgsGeoPackageRasterLayerItem( this, info->name(), info->path(), info->uri() ) );
    }
    else
    {
      children.append( new QgsGeoPackageVectorLayerItem( this, info->name(), info->path(), info->uri(), info->layerType( ) ) );
    }
  }
  qDeleteAll( layers );
  return children;
}

bool QgsGeoPackageCollectionItem::equal( const QgsDataItem *other )
{
  if ( type() != other->type() )
  {
    return false;
  }
  const QgsGeoPackageCollectionItem *o = dynamic_cast<const QgsGeoPackageCollectionItem *>( other );
  return o && mPath == o->mPath && mName == o->mName;

}

#ifdef HAVE_GUI

QList<QAction *> QgsGeoPackageCollectionItem::actions( QWidget *parent )
{
  QList<QAction *> lst;

  if ( QgsOgrDbConnection::connectionList( QStringLiteral( "GPKG" ) ).contains( mName ) )
  {
    QAction *actionDeleteConnection = new QAction( tr( "Remove Connection" ), parent );
    connect( actionDeleteConnection, &QAction::triggered, this, &QgsGeoPackageConnectionItem::deleteConnection );
    lst.append( actionDeleteConnection );
  }
  else
  {
    // Add to stored connections
    QAction *actionAddConnection = new QAction( tr( "Add Connection" ), parent );
    connect( actionAddConnection, &QAction::triggered, this, &QgsGeoPackageCollectionItem::addConnection );
    lst.append( actionAddConnection );
  }

  // Add table to existing DB
  QAction *actionAddTable = new QAction( tr( "Create a new layer or table..." ), parent );
  connect( actionAddTable, &QAction::triggered, this, &QgsGeoPackageCollectionItem::addTable );
  lst.append( actionAddTable );

  return lst;
}

bool QgsGeoPackageCollectionItem::handleDrop( const QMimeData *data, Qt::DropAction )
{

  if ( !QgsMimeDataUtils::isUriList( data ) )
    return false;

  QString uri;

  QStringList importResults;
  bool hasError = false;

  // Main task
  std::unique_ptr< QgsConcurrentFileWriterImportTask > mainTask( new QgsConcurrentFileWriterImportTask( tr( "GeoPackage import" ) ) );
  QgsTaskList importTasks;

  const auto lst = QgsMimeDataUtils::decodeUriList( data );
  for ( const QgsMimeDataUtils::Uri &dropUri : lst )
  {
    // Check that we are not copying over self
    if ( dropUri.uri.startsWith( mPath ) )
    {
      importResults.append( tr( "You cannot import layer %1 over itself!" ).arg( dropUri.name ) );
      hasError = true;

    }
    else
    {
      QgsMapLayer *srcLayer = nullptr;
      bool owner;
      bool isVector = false;
      QString error;
      // Common checks for raster and vector
      // aspatial is treated like vector
      if ( dropUri.layerType == QStringLiteral( "vector" ) )
      {
        // open the source layer
        srcLayer = dropUri.vectorLayer( owner, error );
        isVector = true;
      }
      else
      {
        srcLayer = dropUri.rasterLayer( owner, error );
      }
      if ( !srcLayer )
      {
        importResults.append( tr( "%1: %2" ).arg( dropUri.name, error ) );
        hasError = true;
        continue;
      }

      if ( srcLayer->isValid() )
      {
        uri = mPath;
        QgsDebugMsgLevel( "URI " + uri, 3 );

        // check if the destination layer already exists
        bool exists = false;
        const auto c( children() );
        for ( const QgsDataItem *child : c )
        {
          if ( child->name() == dropUri.name )
          {
            exists = true;
          }
        }
        if ( ! exists || QMessageBox::question( nullptr, tr( "Overwrite Layer" ),
                                                tr( "Destination layer <b>%1</b> already exists. Do you want to overwrite it?" ).arg( dropUri.name ), QMessageBox::Yes |  QMessageBox::No ) == QMessageBox::Yes )
        {
          if ( isVector ) // Import vectors and aspatial
          {
            QgsVectorLayer *vectorSrcLayer = qobject_cast < QgsVectorLayer * >( srcLayer );
            QVariantMap options;
            options.insert( QStringLiteral( "driverName" ), QStringLiteral( "GPKG" ) );
            options.insert( QStringLiteral( "update" ), true );
            options.insert( QStringLiteral( "overwrite" ), true );
            options.insert( QStringLiteral( "layerName" ), dropUri.name );
            QgsVectorLayerExporterTask *exportTask = new QgsVectorLayerExporterTask( vectorSrcLayer, uri, QStringLiteral( "ogr" ), vectorSrcLayer->crs(), options, owner );
            mainTask->addSubTask( exportTask, importTasks );
            importTasks << exportTask;
            // when export is successful:
            connect( exportTask, &QgsVectorLayerExporterTask::exportComplete, this, [ = ]()
            {
              // this is gross - TODO - find a way to get access to messageBar from data items
              QMessageBox::information( nullptr, tr( "Import to GeoPackage database" ), tr( "Import was successful." ) );
              refreshConnections();
            } );

            // when an error occurs:
            connect( exportTask, &QgsVectorLayerExporterTask::errorOccurred, this, [ = ]( int error, const QString & errorMessage )
            {
              if ( error != QgsVectorLayerExporter::ErrUserCanceled )
              {
                QgsMessageOutput *output = QgsMessageOutput::createMessageOutput();
                output->setTitle( tr( "Import to GeoPackage database" ) );
                output->setMessage( tr( "Failed to import some vector layers!\n\n" ) + errorMessage, QgsMessageOutput::MessageText );
                output->showMessage();
              }
            } );

          }
          else  // Import raster
          {
            QgsGeoPackageRasterWriterTask  *exportTask = new QgsGeoPackageRasterWriterTask( dropUri, mPath );
            mainTask->addSubTask( exportTask, importTasks );
            importTasks << exportTask;
            // when export is successful:
            connect( exportTask, &QgsGeoPackageRasterWriterTask::writeComplete, this, [ = ]()
            {
              // this is gross - TODO - find a way to get access to messageBar from data items
              QMessageBox::information( nullptr, tr( "Import to GeoPackage database" ), tr( "Import was successful." ) );
              refreshConnections();
            } );

            // when an error occurs:
            connect( exportTask, &QgsGeoPackageRasterWriterTask::errorOccurred, this, [ = ]( QgsGeoPackageRasterWriter::WriterError error, const QString & errorMessage )
            {
              if ( error != QgsGeoPackageRasterWriter::WriterError::ErrUserCanceled )
              {
                QgsMessageOutput *output = QgsMessageOutput::createMessageOutput();
                output->setTitle( tr( "Import to GeoPackage database" ) );
                output->setMessage( tr( "Failed to import some raster layers!\n\n" ) + errorMessage, QgsMessageOutput::MessageText );
                output->showMessage();
              }
              // Always try to delete the imported raster, in case the gpkg has been left
              // in an inconsistent status. Ignore delete errors.
              QString deleteErr;
              deleteGeoPackageRasterLayer( QStringLiteral( "GPKG:%1:%2" ).arg( mPath, dropUri.name ), deleteErr );
            } );

          }
        } // do not overwrite
      }
      else
      {
        importResults.append( tr( "%1: Not a valid layer!" ).arg( dropUri.name ) );
        hasError = true;
      }
    } // check for self copy
  } // for each

  if ( hasError )
  {
    QgsMessageOutput *output = QgsMessageOutput::createMessageOutput();
    output->setTitle( tr( "Import to GeoPackage database" ) );
    output->setMessage( tr( "Failed to import some layers!\n\n" ) + importResults.join( QStringLiteral( "\n" ) ), QgsMessageOutput::MessageText );
    output->showMessage();
  }
  if ( ! importTasks.isEmpty() )
  {
    QgsApplication::taskManager()->addTask( mainTask.release() );
  }
  return true;
}
#endif

bool QgsGeoPackageCollectionItem::deleteGeoPackageRasterLayer( const QString &uri, QString &errCause )
{
  bool result = false;
  // Better safe than sorry
  if ( ! uri.isEmpty( ) )
  {
    QStringList pieces( uri.split( ':' ) );
    if ( pieces.size() != 3 )
    {
      errCause = QStringLiteral( "Layer URI is malformed: layer <b>%1</b> cannot be deleted!" ).arg( uri );
    }
    else
    {
      QString baseUri = pieces.at( 1 );
      QString layerName = pieces.at( 2 );
      sqlite3_database_unique_ptr database;
      int status = database.open_v2( baseUri, SQLITE_OPEN_READWRITE, nullptr );
      if ( status != SQLITE_OK )
      {
        errCause = sqlite3_errmsg( database.get() );
      }
      else
      {
        // Remove table
        char *errmsg = nullptr;
        char *sql = sqlite3_mprintf(
                      "DROP table IF EXISTS \"%w\";"
                      "DELETE FROM gpkg_contents WHERE table_name = '%q';"
                      "DELETE FROM gpkg_tile_matrix WHERE table_name = '%q';"
                      "DELETE FROM gpkg_tile_matrix_set WHERE table_name = '%q';",
                      layerName.toUtf8().constData(),
                      layerName.toUtf8().constData(),
                      layerName.toUtf8().constData(),
                      layerName.toUtf8().constData() );
        status = sqlite3_exec(
                   database.get(),                              /* An open database */
                   sql,                                 /* SQL to be evaluated */
                   nullptr,                                /* Callback function */
                   nullptr,                                /* 1st argument to callback */
                   &errmsg                              /* Error msg written here */
                 );
        sqlite3_free( sql );
        // Remove from optional tables, may silently fail
        QStringList optionalTables;
        optionalTables << QStringLiteral( "gpkg_extensions" )
                       << QStringLiteral( "gpkg_metadata_reference" );
        for ( const QString &tableName : qgis::as_const( optionalTables ) )
        {
          char *sql = sqlite3_mprintf( "DELETE FROM %w WHERE table_name = '%q'",
                                       tableName.toUtf8().constData(),
                                       layerName.toUtf8().constData() );
          ( void )sqlite3_exec(
            database.get(),                              /* An open database */
            sql,                                 /* SQL to be evaluated */
            nullptr,                                /* Callback function */
            nullptr,                                /* 1st argument to callback */
            nullptr                                 /* Error msg written here */
          );
          sqlite3_free( sql );
        }
        // Other tables, ignore errors
        {
          char *sql = sqlite3_mprintf( "DELETE FROM gpkg_2d_gridded_coverage_ancillary WHERE tile_matrix_set_name = '%q'",
                                       layerName.toUtf8().constData() );
          ( void )sqlite3_exec(
            database.get(),                              /* An open database */
            sql,                                 /* SQL to be evaluated */
            nullptr,                                /* Callback function */
            nullptr,                                /* 1st argument to callback */
            nullptr                                 /* Error msg written here */
          );
          sqlite3_free( sql );
        }
        {
          char *sql = sqlite3_mprintf( "DELETE FROM gpkg_2d_gridded_tile_ancillary WHERE tpudt_name = '%q'",
                                       layerName.toUtf8().constData() );
          ( void )sqlite3_exec(
            database.get(),                              /* An open database */
            sql,                                 /* SQL to be evaluated */
            nullptr,                                /* Callback function */
            nullptr,                                /* 1st argument to callback */
            nullptr                                 /* Error msg written here */
          );
          sqlite3_free( sql );
        }
        // Vacuum
        {
          ( void )sqlite3_exec(
            database.get(),                              /* An open database */
            "VACUUM",                            /* SQL to be evaluated */
            nullptr,                                /* Callback function */
            nullptr,                                /* 1st argument to callback */
            nullptr                                 /* Error msg written here */
          );
        }

        if ( status == SQLITE_OK )
        {
          result = true;
        }
        else
        {
          errCause = tr( "There was an error deleting the layer %1: %2" ).arg( layerName, QString::fromUtf8( errmsg ) );
        }
        sqlite3_free( errmsg );
      }
    }
  }
  else
  {
    // This should never happen!
    errCause = tr( "Layer URI is empty: layer cannot be deleted!" );
  }
  return result;
}

QgsGeoPackageConnectionItem::QgsGeoPackageConnectionItem( QgsDataItem *parent, const QString &name, const QString &path )
  : QgsGeoPackageCollectionItem( parent, name, path )
{

}

bool QgsGeoPackageConnectionItem::equal( const QgsDataItem *other )
{
  if ( type() != other->type() )
  {
    return false;
  }
  const QgsGeoPackageConnectionItem *o = dynamic_cast<const QgsGeoPackageConnectionItem *>( other );
  return o && mPath == o->mPath && mName == o->mName;

}

#ifdef HAVE_GUI
QList<QAction *> QgsGeoPackageConnectionItem::actions( QWidget *parent )
{
  QList<QAction *> lst;

  QAction *actionDeleteConnection = new QAction( tr( "Remove Connection" ), parent );
  connect( actionDeleteConnection, &QAction::triggered, this, &QgsGeoPackageConnectionItem::deleteConnection );
  lst.append( actionDeleteConnection );

  // Add table to existing DB
  QAction *actionAddTable = new QAction( tr( "Create a New Layer or Table..." ), parent );
  connect( actionAddTable, &QAction::triggered, this, &QgsGeoPackageConnectionItem::addTable );
  lst.append( actionAddTable );


  return lst;
}

void QgsGeoPackageCollectionItem::deleteConnection()
{
  QgsOgrDbConnection::deleteConnection( name(), QStringLiteral( "GPKG" ) );
  mParent->refreshConnections();
}


void QgsGeoPackageCollectionItem::addTable()
{
  QgsNewGeoPackageLayerDialog dialog( nullptr );
  dialog.setDatabasePath( mPath );
  dialog.setCrs( QgsProject::instance()->defaultCrsForNewLayers() );
  dialog.setOverwriteBehavior( QgsNewGeoPackageLayerDialog::AddNewLayer );
  dialog.lockDatabasePath();
  if ( dialog.exec() == QDialog::Accepted )
  {
    refreshConnections();
  }
}

void QgsGeoPackageCollectionItem::addConnection()
{
  QgsOgrDbConnection connection( mName, QStringLiteral( "GPKG" ) );
  connection.setPath( mPath );
  connection.save();
  mParent->refreshConnections();
}

#endif

#ifdef HAVE_GUI
QList<QAction *> QgsGeoPackageAbstractLayerItem::actions()
{
  QList<QAction *> lst;
  QAction *actionDeleteLayer = new QAction( tr( "Delete Layer '%1'..." ).arg( mName ), this );
  connect( actionDeleteLayer, &QAction::triggered, this, &QgsGeoPackageAbstractLayerItem::deleteLayer );
  lst.append( actionDeleteLayer );
  return lst;
}

void QgsGeoPackageAbstractLayerItem::deleteLayer()
{
  // Check if the layer is in the registry
  const QgsMapLayer *projectLayer = nullptr;
  const auto mapLayers( QgsProject::instance()->mapLayers() );
  for ( const QgsMapLayer *layer :  mapLayers )
  {
    if ( layer->publicSource() == mUri )
    {
      projectLayer = layer;
    }
  }
  if ( ! projectLayer )
  {
    if ( QMessageBox::question( nullptr, QObject::tr( "Delete Layer" ),
                                QObject::tr( "Are you sure you want to delete layer <b>%1</b> from GeoPackage?" ).arg( mName ),
                                QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) != QMessageBox::Yes )
      return;

    QString errCause;
    bool res = executeDeleteLayer( errCause );
    if ( !res )
    {
      QMessageBox::warning( nullptr, tr( "Delete Layer" ), errCause );
    }
    else
    {
      QMessageBox::information( nullptr, tr( "Delete Layer" ), tr( "Layer <b>%1</b> deleted successfully." ).arg( mName ) );
      if ( mParent )
        mParent->refreshConnections();
    }
  }
  else
  {
    QMessageBox::warning( nullptr, QObject::tr( "Delete Layer" ), QObject::tr( "The layer <b>%1</b> cannot be deleted because it is in the current project as <b>%2</b>,"
                          " remove it from the project and retry." ).arg( mName, projectLayer->name() ) );
  }

}
#endif

QgsGeoPackageAbstractLayerItem::QgsGeoPackageAbstractLayerItem( QgsDataItem *parent, const QString &name, const QString &path, const QString &uri, QgsLayerItem::LayerType layerType, const QString &providerKey )
  : QgsLayerItem( parent, name, path, uri, layerType, providerKey )
{
  mToolTip = uri;
  setState( Populated ); // no children are expected
}

bool QgsGeoPackageAbstractLayerItem::executeDeleteLayer( QString &errCause )
{
  errCause = QObject::tr( "The layer <b>%1</b> cannot be deleted because this feature is not yet implemented for this kind of layers." ).arg( mName );
  return false;
}


QgsGeoPackageVectorLayerItem::QgsGeoPackageVectorLayerItem( QgsDataItem *parent, const QString &name, const QString &path, const QString &uri, LayerType layerType )
  : QgsGeoPackageAbstractLayerItem( parent, name, path, uri, layerType, QStringLiteral( "ogr" ) )
{

}


QgsGeoPackageRasterLayerItem::QgsGeoPackageRasterLayerItem( QgsDataItem *parent, const QString &name, const QString &path, const QString &uri )
  : QgsGeoPackageAbstractLayerItem( parent, name, path, uri, QgsLayerItem::LayerType::Raster, QStringLiteral( "gdal" ) )
{

}

bool QgsGeoPackageRasterLayerItem::executeDeleteLayer( QString &errCause )
{
  return QgsGeoPackageCollectionItem::deleteGeoPackageRasterLayer( mUri, errCause );
}


bool QgsGeoPackageVectorLayerItem::executeDeleteLayer( QString &errCause )
{
  return ::deleteLayer( mUri, errCause );
}

