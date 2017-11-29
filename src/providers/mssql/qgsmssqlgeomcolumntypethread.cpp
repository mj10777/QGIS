#include "qgsmssqlgeomcolumntypethread.h"

#include <QSqlDatabase>
#include <QSqlQuery>

#include "qgslogger.h"
#include "qgsmssqlprovider.h"

QgsMssqlGeomColumnTypeThread::QgsMssqlGeomColumnTypeThread( const QString &connectionName, bool useEstimatedMetadata )
  : mConnectionName( connectionName )
  , mUseEstimatedMetadata( useEstimatedMetadata )
  , mStopped( false )
{
  qRegisterMetaType<QgsMssqlLayerProperty>( "QgsMssqlLayerProperty" );
}

void QgsMssqlGeomColumnTypeThread::addGeometryColumn( const QgsMssqlLayerProperty &layerProperty )
{
  layerProperties << layerProperty;
}

void QgsMssqlGeomColumnTypeThread::stop()
{
  mStopped = true;
}

void QgsMssqlGeomColumnTypeThread::run()
{
  mStopped = false;

  for ( QList<QgsMssqlLayerProperty>::iterator it = layerProperties.begin(),
        end = layerProperties.end();
        it != end; ++it )
  {
    QgsMssqlLayerProperty &layerProperty = *it;

    if ( !mStopped )
    {
      QString table;
      table = QStringLiteral( "%1[%2]" )
              .arg( layerProperty.schemaName.isEmpty() ? QLatin1String( "" ) : QStringLiteral( "[%1]." ).arg( layerProperty.schemaName ),
                    layerProperty.tableName );

      QString query = QString( "SELECT %3"
                               " UPPER([%1].STGeometryType()),"
                               " [%1].STSrid"
                               " FROM %2"
                               " WHERE [%1] IS NOT NULL %4"
                               " GROUP BY [%1].STGeometryType(), [%1].STSrid" )
                      .arg( layerProperty.geometryColName,
                            table,
                            mUseEstimatedMetadata ? "TOP 1" : "",
                            layerProperty.sql.isEmpty() ? QLatin1String( "" ) : QStringLiteral( " AND %1" ).arg( layerProperty.sql ) );

      // issue the sql query
      QSqlDatabase db = QSqlDatabase::database( mConnectionName );
      if ( !QgsMssqlProvider::OpenDatabase( db ) )
      {
        QgsDebugMsg( db.lastError().text() );
        continue;
      }

      QSqlQuery q = QSqlQuery( db );
      q.setForwardOnly( true );
      if ( !q.exec( query ) )
      {
        QgsDebugMsg( q.lastError().text() );
      }

      QString type;
      QString srid;

      if ( q.isActive() )
      {
        QStringList types;
        QStringList srids;

        while ( q.next() )
        {
          QString type = q.value( 0 ).toString().toUpper();
          QString srid = q.value( 1 ).toString();

          if ( type.isEmpty() )
            continue;

          types << type;
          srids << srid;
        }

        type = types.join( QStringLiteral( "," ) );
        srid = srids.join( QStringLiteral( "," ) );
      }

      layerProperty.type = type;
      layerProperty.srid = srid;
    }
    else
    {
      layerProperty.type.clear();
      layerProperty.srid.clear();
    }

    // Now tell the layer list dialog box...
    emit setLayerType( layerProperty );
  }
}
