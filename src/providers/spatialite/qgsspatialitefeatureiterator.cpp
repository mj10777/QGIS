/***************************************************************************
    qgsspatialitefeatureiterator.cpp
    ---------------------
    begin                : Juli 2012
    copyright            : (C) 2012 by Martin Dobias
    email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "qgsspatialitefeatureiterator.h"

#include "qgsspatialiteprovider.h"
#include "qgssqliteexpressioncompiler.h"

#include "qgsgeometry.h"
#include "qgslogger.h"
#include "qgsmessagelog.h"
#include "qgsjsonutils.h"
#include "qgssettings.h"
#include "qgsexception.h"
//-----------------------------------------------------------------
// QgsSpatiaLiteFeatureSource::QgsSpatiaLiteFeatureSource
//-----------------------------------------------------------------
QgsSpatiaLiteFeatureSource::QgsSpatiaLiteFeatureSource( const QgsSpatiaLiteProvider *p )
  : mQSqliteHandle( p->getQSqliteHandle() )
  , mSpatialiteDbInfo( p->getSpatialiteDbInfo() )
  , mDbLayer( p->getDbLayer() )
  , mGeometryColumn( p->getGeometryColumn() )
  , mAttributeFields( p->getAttributeFields() )
  , mPrimaryKey( p->getPrimaryKey() )
  , mPrimaryKeyCId( p->getPrimaryKeyCId() )
  , mLayerName( p->getLayerName() )
  , mGeometryTypeString( p->getGeometryTypeString() )
  , mSpatialIndexType( p->getSpatialIndexType() )
  , mValid( p->isValid() )
  , mSubsetString( p->mSubsetString )
  , mQuery( p->mQuery )
  , mIsQuery( p->mIsQuery )
  , mViewBased( p->mViewBased )
  , mVShapeBased( p->mVShapeBased )
  , mIndexTable( p->getIndexTable() )
  , mIndexGeometry( p->getIndexGeometry() )
  , mSpatialIndexRTree( p->mSpatialIndexRTree )
  , mSpatialIndexMbrCache( p->mSpatialIndexMbrCache )
  , mSqlitePath( p->mSqlitePath )
  , mCrs( p->crs() )
{
}
//-----------------------------------------------------------------
// QgsSpatiaLiteFeatureSource::getFeatures
//-----------------------------------------------------------------
QgsFeatureIterator QgsSpatiaLiteFeatureSource::getFeatures( const QgsFeatureRequest &request )
{
  return QgsFeatureIterator( new QgsSpatiaLiteFeatureIterator( this, false, request ) );
}
//-----------------------------------------------------------------
// QgsSpatiaLiteFeatureIterator::QgsSpatiaLiteFeatureIterator
//-----------------------------------------------------------------
QgsSpatiaLiteFeatureIterator::QgsSpatiaLiteFeatureIterator( QgsSpatiaLiteFeatureSource *source, bool ownSource, const QgsFeatureRequest &request )
  : QgsAbstractFeatureIteratorFromSource<QgsSpatiaLiteFeatureSource>( source, ownSource, request )
  , mExpressionCompiled( false )
{
  mHandle = mSource->getQSqliteHandle();
  <<< <<< < HEAD
  mFetchGeometry = !mSource->getGeometryColumn().isNull() && !( mRequest.flags() & QgsFeatureRequest::NoGeometry );
  mHasPrimaryKey = !mSource->getPrimaryKey().isEmpty();
  == == == =
    mFetchGeometry = !mSource->mGeometryColumn.isNull() && !( mRequest.flags() & QgsFeatureRequest::NoGeometry );
  mHasPrimaryKey = !mSource->mPrimaryKey.isEmpty();
  >>> >>> > upstream_qgis / master32.spatialite_provider
  mRowNumber = 0;
  QStringList whereClauses;
  bool useFallbackWhereClause = false;
  QString fallbackWhereClause;
  QString whereClause;

  if ( mRequest.destinationCrs().isValid() && mRequest.destinationCrs() != mSource->mCrs )
  {
    mTransform = QgsCoordinateTransform( mSource->mCrs, mRequest.destinationCrs() );
  }
  try
  {
    mFilterRect = filterRectToSourceCrs( mTransform );
  }
  catch ( QgsCsException & )
  {
    // can't reproject mFilterRect
    close();
    return;
  }

  //beware - limitAtProvider needs to be set to false if the request cannot be completely handled
  //by the provider (e.g., utilising QGIS expression filters)
  bool limitAtProvider = ( mRequest.limit() >= 0 );

  if ( !mFilterRect.isNull() && !mSource->getGeometryColumn() .isNull() )
  {
    // some kind of MBR spatial filtering is required
    whereClause = whereClauseRect();
    if ( ! whereClause.isEmpty() )
    {
      whereClauses.append( whereClause );
    }
  }

  if ( !mSource->mSubsetString.isEmpty() )
  {
    whereClause = "( " + mSource->mSubsetString + ')';
    if ( ! whereClause.isEmpty() )
    {
      whereClauses.append( whereClause );
    }
  }

  if ( request.filterType() == QgsFeatureRequest::FilterFid )
  {
    whereClause = whereClauseFid();
    if ( ! whereClause.isEmpty() )
    {
      whereClauses.append( whereClause );
    }
  }
  else if ( request.filterType() == QgsFeatureRequest::FilterFids )
  {
    if ( request.filterFids().isEmpty() )
    {
      close();
    }
    else
    {
      whereClauses.append( whereClauseFids() );
    }
  }
  //IMPORTANT - this MUST be the last clause added!
  else if ( request.filterType() == QgsFeatureRequest::FilterExpression )
  {
    // ensure that all attributes required for expression filter are being fetched
    if ( mRequest.flags() & QgsFeatureRequest::SubsetOfAttributes && request.filterType() == QgsFeatureRequest::FilterExpression )
    {
      QgsAttributeList attrs = request.subsetOfAttributes();
      //ensure that all fields required for filter expressions are prepared
      <<< <<< < HEAD
      QSet<int> attributeIndexes = request.filterExpression()->referencedAttributeIndexes( mSource->getAttributeFields() );
      == == == =
        QSet<int> attributeIndexes = request.filterExpression()->referencedAttributeIndexes( mSource->mAttributeFields );
      >>> >>> > upstream_qgis / master32.spatialite_provider
      attributeIndexes += attrs.toSet();
      mRequest.setSubsetOfAttributes( attributeIndexes.toList() );
    }
    if ( request.filterExpression()->needsGeometry() )
    {
      mFetchGeometry = true;
    }

    if ( QgsSettings().value( QStringLiteral( "qgis/compileExpressions" ), true ).toBool() )
    {
      <<< <<< < HEAD
      QgsSQLiteExpressionCompiler compiler = QgsSQLiteExpressionCompiler( mSource->getAttributeFields() );
      == == == =
        QgsSQLiteExpressionCompiler compiler = QgsSQLiteExpressionCompiler( source->mAttributeFields );
      >>> >>> > upstream_qgis / master32.spatialite_provider

      QgsSqlExpressionCompiler::Result result = compiler.compile( request.filterExpression() );

      if ( result == QgsSqlExpressionCompiler::Complete || result == QgsSqlExpressionCompiler::Partial )
      {
        whereClause = compiler.result();
        if ( !whereClause.isEmpty() )
        {
          useFallbackWhereClause = true;
          fallbackWhereClause = whereClauses.join( QStringLiteral( " AND " ) );
          whereClauses.append( whereClause );
          //if only partial success when compiling expression, we need to double-check results using QGIS' expressions
          mExpressionCompiled = ( result == QgsSqlExpressionCompiler::Complete );
          mCompileStatus = ( mExpressionCompiled ? Compiled : PartiallyCompiled );
        }
      }
      if ( result != QgsSqlExpressionCompiler::Complete )
      {
        //can't apply limit at provider side as we need to check all results using QGIS expressions
        limitAtProvider = false;
      }
    }
    else
    {
      limitAtProvider = false;
    }
  }

  if ( !mClosed )
  {
    whereClause = whereClauses.join( QStringLiteral( " AND " ) );

    // Setup the order by
    QStringList orderByParts;

    mOrderByCompiled = true;

    if ( QgsSettings().value( QStringLiteral( "qgis/compileExpressions" ), true ).toBool() )
    {
      <<< <<< < HEAD
      QgsSQLiteExpressionCompiler compiler = QgsSQLiteExpressionCompiler( mSource->getAttributeFields() );
      QgsExpression expression = clause.expression();
      if ( compiler.compile( &expression ) == QgsSqlExpressionCompiler::Complete )
        == == == =
          Q_FOREACH ( const QgsFeatureRequest::OrderByClause &clause, request.orderBy() )
            >>> >>> > upstream_qgis / master32.spatialite_provider
        {
          QgsSQLiteExpressionCompiler compiler = QgsSQLiteExpressionCompiler( source->mAttributeFields );
          QgsExpression expression = clause.expression();
          if ( compiler.compile( &expression ) == QgsSqlExpressionCompiler::Complete )
          {
            QString part;
            part = compiler.result();

            if ( clause.nullsFirst() )
              orderByParts << QStringLiteral( "%1 IS NOT NULL" ).arg( part );
            else
              orderByParts << QStringLiteral( "%1 IS NULL" ).arg( part );

            part += clause.ascending() ? " COLLATE NOCASE ASC" : " COLLATE NOCASE DESC";
            orderByParts << part;
          }
          else
          {
            // Bail out on first non-complete compilation.
            // Most important clauses at the beginning of the list
            // will still be sent and used to pre-sort so the local
            // CPU can use its cycles for fine-tuning.
            mOrderByCompiled = false;
            break;
          }
        }
    }
    else
    {
      mOrderByCompiled = false;
    }

    if ( !mOrderByCompiled )
      limitAtProvider = false;

    // also need attributes required by order by
    if ( !mOrderByCompiled && mRequest.flags() & QgsFeatureRequest::SubsetOfAttributes && !mRequest.orderBy().isEmpty() )
    {
      QSet<int> attributeIndexes;
      Q_FOREACH ( const QString &attr, mRequest.orderBy().usedAttributes() )
      {
        attributeIndexes << mSource->mAttributeFields.lookupField( attr );
      }
      attributeIndexes += mRequest.subsetOfAttributes().toSet();
      mRequest.setSubsetOfAttributes( attributeIndexes.toList() );
    }

    // preparing the SQL statement
    bool success = prepareStatement( whereClause, limitAtProvider ? mRequest.limit() : -1, orderByParts.join( QStringLiteral( "," ) ) );
    if ( !success && useFallbackWhereClause )
    {
      //try with the fallback where clause, e.g., for cases when using compiled expression failed to prepare
      mExpressionCompiled = false;
      success = prepareStatement( fallbackWhereClause, -1, orderByParts.join( QStringLiteral( "," ) ) );
    }

    if ( !success )
    {
      <<< <<< < HEAD
      attributeIndexes << mSource->getAttributeFields().lookupField( attr ); // mSource->getAttributeFields().lookupField( attr );
      == == == =
        // some error occurred
        sqliteStatement = nullptr;
      close();
      >>> >>> > upstream_qgis / master32.spatialite_provider
    }
  }
}
//-----------------------------------------------------------------
// QgsSpatiaLiteFeatureIterator::~QgsSpatiaLiteFeatureIterator
//-----------------------------------------------------------------
QgsSpatiaLiteFeatureIterator::~QgsSpatiaLiteFeatureIterator()
{
  close();
}
//-----------------------------------------------------------------
// QgsSpatiaLiteFeatureIterator::close
//-----------------------------------------------------------------
bool QgsSpatiaLiteFeatureIterator::close()
{
  if ( mClosed )
    return false;

  iteratorClosed();

  <<< <<< < HEAD
  // preparing the SQL statement
  // whereClause["id_geometry" IN (SELECT pkid FROM "idx_berlin_linestrings_soldner_linestring" WHERE xmin <= 24685.53663580332067795 AND xmax >= 24640.70353396030259319 AND ymin <= 22091.64312717200664338 AND ymax >= 22058.88187432484119199)]
  bool success = prepareStatement( whereClause, limitAtProvider ? mRequest.limit() : -1, orderByParts.join( QStringLiteral( "," ) ) );
  if ( !success && useFallbackWhereClause )
    == == == =
      if ( !mHandle )
        >>> >>> > upstream_qgis / master32.spatialite_provider
    {
      mClosed = true;
      return false;
    }

  if ( sqliteStatement )
  {
    sqlite3_finalize( sqliteStatement );
    sqliteStatement = nullptr;
  }
  // The Handle belongs to the provider, do not close!
  // QgsSpatiaLiteConnPool::instance()->releaseConnection( mHandle );
  // mHandle = nullptr;

  mClosed = true;
  return true;
}
//-----------------------------------------------------------------
// QgsSpatiaLiteFeatureIterator::fetchFeature
//-----------------------------------------------------------------
bool QgsSpatiaLiteFeatureIterator::fetchFeature( QgsFeature &feature )
{
  feature.setValid( false );
  if ( mClosed )
    return false;

  if ( !sqliteStatement )
  {
    QgsDebugMsg( "Invalid current SQLite statement" );
    close();
    return false;
  }

  if ( !getFeature( sqliteStatement, feature ) )
  {
    sqlite3_finalize( sqliteStatement );
    sqliteStatement = nullptr;
    close();
    return false;
  }
  feature.setValid( true );
  geometryToDestinationCrs( feature, mTransform );
  return true;
}
//-----------------------------------------------------------------
// QgsSpatiaLiteFeatureIterator::nextFeatureFilterExpression
//-----------------------------------------------------------------
bool QgsSpatiaLiteFeatureIterator::nextFeatureFilterExpression( QgsFeature &f )
{
  if ( !mExpressionCompiled )
    return QgsAbstractFeatureIterator::nextFeatureFilterExpression( f );
  else
    return fetchFeature( f );
}
//-----------------------------------------------------------------
// QgsSpatiaLiteFeatureIterator::rewind
//-----------------------------------------------------------------
bool QgsSpatiaLiteFeatureIterator::rewind()
{
  if ( mClosed )
    return false;

  if ( sqlite3_reset( sqliteStatement ) == SQLITE_OK )
  {
    mRowNumber = 0;
    return true;
  }
  else
  {
    return false;
  }
}
<<< <<< < HEAD

bool QgsSpatiaLiteFeatureIterator::close()
{
  if ( mClosed )
    return false;

  iteratorClosed();

  if ( !mHandle )
  {
    mClosed = true;
    return false;
  }

  if ( sqliteStatement )
  {
    sqlite3_finalize( sqliteStatement );
    sqliteStatement = nullptr;
  }
  // The Handle belongs to the provider, do not close!
  // QgsSpatiaLiteConnPool::instance()->releaseConnection( mHandle );
  // mHandle = nullptr;

  mClosed = true;
  return true;
}

////


== == == =
//-----------------------------------------------------------------
// QgsSpatiaLiteFeatureIterator::prepareStatement
//-----------------------------------------------------------------
  >>> >>> > upstream_qgis / master32.spatialite_provider
  bool QgsSpatiaLiteFeatureIterator::prepareStatement( const QString &whereClause, long limit, const QString &orderBy )
{
  if ( !mHandle )
    return false;

  try
  {
    QString sql = QStringLiteral( "SELECT %1" ).arg( mHasPrimaryKey ? quotedPrimaryKey() : QStringLiteral( "0" ) );
    int colIdx = 1; // column 0 is primary key

    if ( mRequest.flags() & QgsFeatureRequest::SubsetOfAttributes )
    {
      QgsAttributeList fetchAttributes = mRequest.subsetOfAttributes();
      for ( QgsAttributeList::const_iterator it = fetchAttributes.constBegin(); it != fetchAttributes.constEnd(); ++it )
      {
        <<< <<< < HEAD
        sql += ',' + fieldName( mSource->getAttributeFields().field( *it ) );
        == == == =
          sql += ',' + fieldName( mSource->mAttributeFields.field( *it ) );
        >>> >>> > upstream_qgis / master32.spatialite_provider
        colIdx++;
      }
    }
    else
    {
      // fetch all attributes
      <<< <<< < HEAD
      for ( int idx = 0; idx < mSource->getAttributeFields().count(); ++idx )
      {
        sql += ',' + fieldName( mSource->getAttributeFields().at( idx ) );
        == == == =
          for ( int idx = 0; idx < mSource->mAttributeFields.count(); ++idx )
        {
          sql += ',' + fieldName( mSource->mAttributeFields.at( idx ) );
          >>> >>> > upstream_qgis / master32.spatialite_provider
          colIdx++;
        }
      }

      if ( mFetchGeometry )
      {
        <<< <<< < HEAD
        sql += QStringLiteral( ", AsBinary(%1)" ).arg( QgsSpatiaLiteUtils::quotedIdentifier( mSource->getGeometryColumn() ) );
        == == == =
          sql += QStringLiteral( ", AsBinary(%1)" ).arg( QgsSpatiaLiteUtils::quotedIdentifier( mSource->mGeometryColumn ) );
        >>> >>> > upstream_qgis / master32.spatialite_provider
        mGeomColIdx = colIdx;
      }
      sql += QStringLiteral( " FROM %1" ).arg( QgsSpatiaLiteUtils::quotedIdentifier( mSource->mQuery ) );

      if ( !whereClause.isEmpty() )
        sql += QStringLiteral( " WHERE %1" ).arg( whereClause );

      if ( !orderBy.isEmpty() )
        sql += QStringLiteral( " ORDER BY %1" ).arg( orderBy );

      if ( limit >= 0 )
        sql += QStringLiteral( " LIMIT %1" ).arg( limit );
      <<< <<< < HEAD
      == == == =
        QgsDebugMsgLevel( QString( "-I-> QgsSpatiaLiteFeatureIterator::prepareStatement: sql[%1]" ).arg( sql ), 7 );
      >>> >>> > upstream_qgis / master32.spatialite_provider
      if ( sqlite3_prepare_v2( mSource->dbSqliteHandle(), sql.toUtf8().constData(), -1, &sqliteStatement, nullptr ) != SQLITE_OK )
      {
        // some error occurred
        QgsMessageLog::logMessage( QObject::tr( "SQLite error: %2\nSQL: %1" ).arg( sql, sqlite3_errmsg( mSource->dbSqliteHandle() ) ), QObject::tr( "SpatiaLite" ) );
        return false;
      }
    }
    catch ( QgsSpatiaLiteUtils::SLFieldNotFound )
    {
      rewind();
      return false;
    }
    return true;
  }
//-----------------------------------------------------------------
// QgsSpatiaLiteFeatureIterator::quotedPrimaryKey
//-----------------------------------------------------------------
  QString QgsSpatiaLiteFeatureIterator::quotedPrimaryKey()
  {
    <<< <<< < HEAD
    return mSource->getPrimaryKey().isEmpty() ? QStringLiteral( "ROWID" ) : QgsSpatiaLiteUtils::quotedIdentifier( mSource->getPrimaryKey() );
    == == == =
      return mSource->mPrimaryKey.isEmpty() ? QStringLiteral( "ROWID" ) : QgsSpatiaLiteUtils::quotedIdentifier( mSource->mPrimaryKey );
    >>> >>> > upstream_qgis / master32.spatialite_provider
  }
//-----------------------------------------------------------------
// QgsSpatiaLiteFeatureIterator::whereClauseFid
//-----------------------------------------------------------------
  QString QgsSpatiaLiteFeatureIterator::whereClauseFid()
  {
    return QStringLiteral( "%1=%2" ).arg( quotedPrimaryKey() ).arg( mRequest.filterFid() );
  }
//-----------------------------------------------------------------
// QgsSpatiaLiteFeatureIterator::whereClauseFids
//-----------------------------------------------------------------
  QString QgsSpatiaLiteFeatureIterator::whereClauseFids()
  {
    if ( mRequest.filterFids().isEmpty() )
      return QLatin1String( "" );

    QString expr = QStringLiteral( "%1 IN (" ).arg( quotedPrimaryKey() ), delim;
    Q_FOREACH ( const QgsFeatureId featureId, mRequest.filterFids() )
    {
      expr += delim + QString::number( featureId );
      delim = ',';
    }
    expr += ')';
    return expr;
  }
//-----------------------------------------------------------------
// QgsSpatiaLiteFeatureIterator::whereClauseRect
//-----------------------------------------------------------------
  QString QgsSpatiaLiteFeatureIterator::whereClauseRect()
  {
    QString whereClause;

    if ( mRequest.flags() & QgsFeatureRequest::ExactIntersect )
    {
      // we are requested to evaluate a true INTERSECT relationship
      <<< <<< < HEAD
      whereClause += QStringLiteral( "Intersects(%1, BuildMbr(%2)) AND " ).arg( QgsSpatiaLiteUtils::quotedIdentifier( mSource->getGeometryColumn() ), mbr( mFilterRect ) );
      == == == =
        whereClause += QStringLiteral( "Intersects(%1, BuildMbr(%2)) AND " ).arg( QgsSpatiaLiteUtils::quotedIdentifier( mSource->mGeometryColumn ), mbr( mFilterRect ) );
      >>> >>> > upstream_qgis / master32.spatialite_provider
    }
    if ( mSource->mVShapeBased )
    {
      // handling a VirtualShape layer
      <<< <<< < HEAD
      whereClause += QStringLiteral( "MbrIntersects(%1, BuildMbr(%2))" ).arg( QgsSpatiaLiteUtils::quotedIdentifier( mSource->getGeometryColumn() ), mbr( mFilterRect ) );
      == == == =
        whereClause += QStringLiteral( "MbrIntersects(%1, BuildMbr(%2))" ).arg( QgsSpatiaLiteUtils::quotedIdentifier( mSource->mGeometryColumn ), mbr( mFilterRect ) );
      >>> >>> > upstream_qgis / master32.spatialite_provider
    }
    else if ( mFilterRect.isFinite() )
    {
      if ( mSource->mSpatialIndexRTree )
      {
        // using the RTree spatial index
        QString mbrFilter = QStringLiteral( "xmin <= %1 AND " ).arg( qgsDoubleToString( mFilterRect.xMaximum() ) );
        mbrFilter += QStringLiteral( "xmax >= %1 AND " ).arg( qgsDoubleToString( mFilterRect.xMinimum() ) );
        mbrFilter += QStringLiteral( "ymin <= %1 AND " ).arg( qgsDoubleToString( mFilterRect.yMaximum() ) );
        mbrFilter += QStringLiteral( "ymax >= %1" ).arg( qgsDoubleToString( mFilterRect.yMinimum() ) );
        QString idxName = QStringLiteral( "idx_%1_%2" ).arg( mSource->mIndexTable, mSource->mIndexGeometry );
        whereClause += QStringLiteral( "%1 IN (SELECT pkid FROM %2 WHERE %3)" )
                       .arg( quotedPrimaryKey(),
                             QgsSpatiaLiteUtils::quotedIdentifier( idxName ),
                             mbrFilter );
      }
      else if ( mSource->mSpatialIndexMbrCache )
      {
        // using the MbrCache spatial index
        QString idxName = QStringLiteral( "cache_%1_%2" ).arg( mSource->mIndexTable, mSource->mIndexGeometry );
        whereClause += QStringLiteral( "%1 IN (SELECT rowid FROM %2 WHERE mbr = FilterMbrIntersects(%3))" )
                       .arg( quotedPrimaryKey(),
                             QgsSpatiaLiteUtils::quotedIdentifier( idxName ),
                             mbr( mFilterRect ) );
      }
      else
      {
        // using simple MBR filtering
        <<< <<< < HEAD
        whereClause += QStringLiteral( "MbrIntersects(%1, BuildMbr(%2))" ).arg( QgsSpatiaLiteUtils::quotedIdentifier( mSource->getGeometryColumn() ), mbr( mFilterRect ) );
        == == == =
          whereClause += QStringLiteral( "MbrIntersects(%1, BuildMbr(%2))" ).arg( QgsSpatiaLiteUtils::quotedIdentifier( mSource->mGeometryColumn ), mbr( mFilterRect ) );
        >>> >>> > upstream_qgis / master32.spatialite_provider
      }
    }
    else
    {
      whereClause = '1';
    }
    return whereClause;
  }
//-----------------------------------------------------------------
// QgsSpatiaLiteFeatureIterator::mbr
//-----------------------------------------------------------------
  QString QgsSpatiaLiteFeatureIterator::mbr( const QgsRectangle & rect )
  {
    return QStringLiteral( "%1, %2, %3, %4" )
           .arg( qgsDoubleToString( rect.xMinimum() ),
                 qgsDoubleToString( rect.yMinimum() ),
                 qgsDoubleToString( rect.xMaximum() ),
                 qgsDoubleToString( rect.yMaximum() ) );
  }
//-----------------------------------------------------------------
// QgsSpatiaLiteFeatureIterator::fieldName
//-----------------------------------------------------------------
  QString QgsSpatiaLiteFeatureIterator::fieldName( const QgsField & fld )
  {
    QString fieldname = QgsSpatiaLiteUtils::quotedIdentifier( fld.name() );
    const QString type = fld.typeName().toLower();
    if ( type.contains( QLatin1String( "geometry" ) ) || type.contains( QLatin1String( "point" ) ) ||
         type.contains( QLatin1String( "line" ) ) || type.contains( QLatin1String( "polygon" ) ) )
    {
      fieldname = QStringLiteral( "AsText(%1)" ).arg( fieldname );
    }
    return fieldname;
  }
//-----------------------------------------------------------------
// QgsSpatiaLiteFeatureIterator::getFeature
//-----------------------------------------------------------------
  bool QgsSpatiaLiteFeatureIterator::getFeature( sqlite3_stmt * stmt, QgsFeature & feature )
  {
    bool subsetAttributes = mRequest.flags() & QgsFeatureRequest::SubsetOfAttributes;

    int ret = sqlite3_step( stmt );
    if ( ret == SQLITE_DONE )
    {
      // there are no more rows to fetch
      return false;
    }
    if ( ret != SQLITE_ROW )
    {
      // some unexpected error occurred
      <<< <<< < HEAD
      QgsMessageLog::logMessage( QObject::tr( "SQLite error getting feature: LayerName[%1] GeomType[%2] -  %3" ).arg( mSource->getLayerName() ).arg( mSource->getGeometryTypeString() ).arg( QString::fromUtf8( sqlite3_errmsg( mSource->dbSqliteHandle() ) ) ) );
      == == == =
        QgsMessageLog::logMessage( QObject::tr( "SQLite error getting feature: LayerName[%1] GeomType[%2] -  %3" ).arg( mSource->mLayerName ).arg( mSource->mGeometryTypeString ).arg( QString::fromUtf8( sqlite3_errmsg( mSource->dbSqliteHandle() ) ) ) );
      >>> >>> > upstream_qgis / master32.spatialite_provider
      return false;
    }
    // one valid row has been fetched from the result set
    if ( !mFetchGeometry )
    {
      // no geometry was required
      feature.clearGeometry();
    }

    <<< <<< < HEAD
    feature.initAttributes( mSource->getAttributeFields().count() );
    feature.setFields( mSource->getAttributeFields() ); // allow name-based attribute lookups
    == == == =
      feature.initAttributes( mSource->mAttributeFields.count() );
    feature.setFields( mSource->mAttributeFields ); // allow name-based attribute lookups
    >>> >>> > upstream_qgis / master32.spatialite_provider
    int ic;
    int n_columns = sqlite3_column_count( stmt );
    for ( ic = 0; ic < n_columns; ic++ )
    {
      if ( ic == 0 )
      {
        if ( mHasPrimaryKey )
        {
          // first column always contains the ROWID (or the primary key)
          QgsFeatureId fid = sqlite3_column_int64( stmt, ic );
          QgsDebugMsgLevel( QString( "ic[%2,%3,%4] fid=%1" ).arg( fid ).arg( ic ).arg( mSource->mPrimaryKeyCId ).arg( mSource->mPrimaryKey ), 3 );
          feature.setId( fid );
        }
        else
        {
          // autoincrement a row number
          mRowNumber++;
          feature.setId( mRowNumber );
        }
      }
      else if ( mFetchGeometry && ic == mGeomColIdx )
      {
        getFeatureGeometry( stmt, ic, feature );
      }
      else
      {
        if ( subsetAttributes )
        {
          if ( ic <= mRequest.subsetOfAttributes().size() )
          {
            const int attrIndex = mRequest.subsetOfAttributes().at( ic - 1 );
            <<< <<< < HEAD
            const QgsField field = mSource->getAttributeFields().at( attrIndex );
            == == == =
              const QgsField field = mSource->mAttributeFields.at( attrIndex );
            >>> >>> > upstream_qgis / master32.spatialite_provider
            feature.setAttribute( attrIndex, getFeatureAttribute( stmt, ic, field.type(), field.subType() ) );
          }
        }
        else
        {
          const int attrIndex = ic - 1;
          <<< <<< < HEAD
          const QgsField field = mSource->getAttributeFields().at( attrIndex );
          == == == =
            const QgsField field = mSource->mAttributeFields.at( attrIndex );
          >>> >>> > upstream_qgis / master32.spatialite_provider
          feature.setAttribute( attrIndex, getFeatureAttribute( stmt, ic, field.type(), field.subType() ) );
        }
      }
    }

    return true;
  }
//-----------------------------------------------------------------
// QgsSpatiaLiteFeatureIterator::getFeatureAttribute
//-----------------------------------------------------------------
  QVariant QgsSpatiaLiteFeatureIterator::getFeatureAttribute( sqlite3_stmt * stmt, int ic, QVariant::Type type, QVariant::Type subType )
  {
    if ( sqlite3_column_type( stmt, ic ) == SQLITE_INTEGER )
    {
      if ( type == QVariant::Int )
      {
        // INTEGER value
        return sqlite3_column_int( stmt, ic );
      }
      else
      {
        // INTEGER value
        return ( qint64 ) sqlite3_column_int64( stmt, ic );
      }
    }

    if ( sqlite3_column_type( stmt, ic ) == SQLITE_FLOAT )
    {
      // DOUBLE value
      return sqlite3_column_double( stmt, ic );
    }

    if ( sqlite3_column_type( stmt, ic ) == SQLITE_TEXT )
    {
      // TEXT value
      const QString txt = QString::fromUtf8( ( const char * ) sqlite3_column_text( stmt, ic ) );
      if ( type == QVariant::List || type == QVariant::StringList )
      {
        // assume arrays are stored as JSON
        QVariant result = QVariant( QgsJsonUtils::parseArray( txt, subType ) );
        result.convert( type );
        return result;
      }
      return txt;
    }

    // assuming NULL
    return QVariant( type );
  }
//-----------------------------------------------------------------
// QgsSpatiaLiteFeatureIterator::getFeatureGeometry
//-----------------------------------------------------------------
  void QgsSpatiaLiteFeatureIterator::getFeatureGeometry( sqlite3_stmt * stmt, int ic, QgsFeature & feature )
  {
    if ( sqlite3_column_type( stmt, ic ) == SQLITE_BLOB )
    {
      unsigned char *featureGeom = nullptr;
      int geom_size = 0;
      const void *blob = sqlite3_column_blob( stmt, ic );
      int blob_size = sqlite3_column_bytes( stmt, ic );
      QgsSpatiaLiteUtils::convertToGeosWKB( ( const unsigned char * )blob, blob_size, &featureGeom, &geom_size );
      if ( featureGeom )
      {
        QgsGeometry g;
        g.fromWkb( featureGeom, geom_size );
        feature.setGeometry( g );
      }
      else
        feature.clearGeometry();
    }
    else
    {
      // NULL geometry
      feature.clearGeometry();
    }
  }
//-----------------------------------------------------------------
// QgsSpatiaLiteFeatureIterator::prepareOrderBy
//-----------------------------------------------------------------
  bool QgsSpatiaLiteFeatureIterator::prepareOrderBy( const QList<QgsFeatureRequest::OrderByClause> &orderBys )
  {
    Q_UNUSED( orderBys )
    // Preparation has already been done in the constructor, so we just communicate the result
    return mOrderByCompiled;
  }
  <<< <<< < HEAD


  QgsSpatiaLiteFeatureSource::QgsSpatiaLiteFeatureSource( const QgsSpatiaLiteProvider * p )
    : mHandle( p->getQSqliteHandle() )
    , mSpatialiteDbInfo( p->getSpatialiteDbInfo() )
    , mDbLayer( p->getDbLayer() )
    , mSubsetString( p->mSubsetString )
    , mQuery( p->mQuery )
    , mIsQuery( p->mIsQuery )
    , mViewBased( p->mViewBased )
    , mVShapeBased( p->mVShapeBased )
    , mIndexTable( p->getIndexTable() )
    , mIndexGeometry( p->getIndexGeometry() )
    , mSpatialIndexRTree( p->mSpatialIndexRTree )
    , mSpatialIndexMbrCache( p->mSpatialIndexMbrCache )
    , mSqlitePath( p->mSqlitePath )
    , mCrs( p->crs() )
  {
  }

  QgsFeatureIterator QgsSpatiaLiteFeatureSource::getFeatures( const QgsFeatureRequest & request )
  {
    return QgsFeatureIterator( new QgsSpatiaLiteFeatureIterator( this, false, request ) );
  }
  == == == =
    >>> >>> > upstream_qgis / master32.spatialite_provider
