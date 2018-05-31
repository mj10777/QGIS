/***************************************************************************
    qgsspatialitefeatureiterator.h
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
#ifndef QGSSPATIALITEFEATUREITERATOR_H
#define QGSSPATIALITEFEATUREITERATOR_H

#include "qgsfeatureiterator.h"
#include "qgsfields.h"
#include "qgsspatialiteconnpool.h"

extern "C"
{
//#include <sys/types.h>
#include <sqlite3.h>
}

class QgsSpatiaLiteProvider;

class QgsSpatiaLiteFeatureSource : public QgsAbstractFeatureSource
{
  public:
    explicit QgsSpatiaLiteFeatureSource( const QgsSpatiaLiteProvider *p );

    virtual QgsFeatureIterator getFeatures( const QgsFeatureRequest &request ) override;

  private:

    /**
     * The class allowing to reuse the same sqlite handle for more layers
     * - containing all Information about Database file
     * \note
     * - isDbValid() return if the connection contains layers that are supported by
     * -- QgsSpatiaLiteProvider, QgsGdalProvider and QgsOgrProvider
     * \see QgsSpatialiteDbInfo::isDbValid()
     * \since QGIS 3.0
     */
    QgsSqliteHandle *mQSqliteHandle = nullptr;

    /**
     * The class allowing to reuse the same sqlite handle for more layers
     * - containing all Information about Database file
     * \note
     * - isDbValid() return if the connection contains layers that are supported by
     * -- QgsSpatiaLiteProvider, QgsGdalProvider and QgsOgrProvider
     * \see QgsSpatialiteDbInfo::isDbValid()
     * \since QGIS 3.0
     */
    QgsSqliteHandle *getQSqliteHandle() const { return mQSqliteHandle; }

    /**
     * The sqlite handler
     * - contained in the QgsSqliteHandle class being used
     * \note
     * - isDbValid() return if the connection contains layers that are supported by
     * -- QgsSpatiaLiteProvider, QgsGdalProvider and QgsOgrProvider
     * \see QgsSpatialiteDbInfo::isDbValid()
     * \since QGIS 3.0
     */
    sqlite3 *dbSqliteHandle() const { return getQSqliteHandle()->handle(); }

    /**
     * Retrieve QgsSpatialiteDbInfo
     * - containing all Information about Database file
     * \note
     * - isDbValid() return if the connection contains layers that are supported by
     * -- QgsSpatiaLiteProvider, QgsGdalProvider and QgsOgrProvider
     * \see QgsSpatialiteDbInfo::isDbValid()
     * \since QGIS 3.0
     */
    QgsSpatialiteDbInfo *mSpatialiteDbInfo = nullptr;

    /**
     * The active Layer
     * - being read by the Provider
     * \note
     * - isLayerValid() return true if everything is considered correct
     * \see QgsSpatialiteDbLayer::isLayerValid
     * \see setDbLayer
     * \since QGIS 3.0
     */
    QgsSpatialiteDbLayer *mDbLayer = nullptr;

    /**
     * The active Layer
     * - being read by the Provider
     * \note
     * - isLayerValid() return true if everything is considered correct
     * \see QgsSpatialiteDbLayer::isLayerValid
     * \see setDbLayer
     * \since QGIS 3.0
     */
    QgsSpatialiteDbLayer *getDbLayer() const { return mDbLayer; }

    /**
     * Retrieve SpatialIndex Query from DbLayer based on given Rectangle
     * \note
     *
     * \see QgsSpatiaLiteFeatureIterator::whereClauseRect
     * \since QGIS 3.0
     */
    QString getSpatialIndexWhereClause( QgsRectangle filterRect ) const
    {
      QString sSpatialIndexQuery = QString();
      if ( getDbLayer() )
      {
        sSpatialIndexQuery = getDbLayer()->getSpatialIndexWhereClause( filterRect );
      }
      return sSpatialIndexQuery;
    }

    /**
     * Build extent string for SpatialIndex
     *  - for use with Spatialite  SpatialIndex
     * \note
     *   BuildMbr Syntax will be used when srid > -2
     * \param filterRect Rectangle value to use
     * \param  iSrid -2 [default]: x,y,Min,Max only ; srid=BuildMbr(x,y,Min,Max,srid)
     * \see QgsSpatialiteDbLayer::getSpatialIndexWhereClause
    */
    QString mbr( const QgsRectangle &filterRect, bool bBuildMbr = false )
    {
      QString sBuildMbr = QString();
      if ( getDbLayer() )
      {
        int iSrid = -2;
        if ( bBuildMbr )
        {
          iSrid = getDbLayer()->getSrid();
        }
        sBuildMbr = QgsSpatiaLiteUtils::mbr( filterRect, iSrid );
      }
      return sBuildMbr;
    }

    /**
     * Name of the geometry column in the table
     * \note
     *  Vector: should always be filled
     *  Raster: may be empty
     * \since QGIS 3.0
     */
    QString mGeometryColumn;

    /**
     * List of layer fields in the table
     * \note
     *  - from PRAGMA table_info()
     * \see QgsSpatialiteDbLayer::getAttributeFields
     * \since QGIS 3.0
     */
    QgsFields mAttributeFields;

    /**
     * Name of the primary key column in the table
     * \note
     *  -> SpatialView: entry of view_rowid of views_geometry_columns
     * \see QgsSpatialiteDbLayer::getPrimaryKey
     * \since QGIS 3.0
     */
    QString mPrimaryKey;

    /**
     * Column-Number of the primary key
     * \note
     *  - SpatialView: name from entry of view_rowid of views_geometry_columns
     *  -> position from inside the PRAGMA table_info('name')
     * \see QgsSpatialiteDbLayer::getPrimaryKeyCId
     * \since QGIS 3.0
     */
    int mPrimaryKeyCId;

    /**
     * Layer-Name
     * \note
     *  Vector: Layer format: 'table_name(geometry_name)'
     *  Raster: Layer format: 'coverage_name'
     * \see QgsSpatialiteDbLayer::getLayerName
     * \since QGIS 3.0
     */
    QString mLayerName;

    /**
     * The Spatialite Geometry-Type of the Layer (as String)
     * - representing mGeometryType
     * \note
     *  - QgsWkbTypes::displayString(mGeometryType)
     * \see QgsSpatialiteDbLayer::getGeometryType
     * \since QGIS 3.0
     */
    QString mGeometryTypeName;

    /**
     * The SpatialIndex-Type used for the Geometry
     * \note
     *  Uses the same numbering as Spatialite [0,1,2]
     * \see QgsSpatialiteDbLayer::getSpatialIndexType
     * \since QGIS 3.0
     */
    int mSpatialIndexType;

    /**
     * Flag indicating if the layer data source is a valid SpatiaLite layer
     * \note
     *  result of setSqliteHandle and setDbLayer
     * otherwise not used
     * \see setSqliteHandle
     * \since QGIS 3.0
     */
    bool mValid;

    /**
     * String used to define a subset of the layer
     * \note
     *  QgsDataSourceUri is reset base on this value
     * \see setSubsetString
     * \since QGIS 3.0
     */
    QString mSubsetString;

    /**
     * Name of the table or subquery
     * \note
     *
     * \see checkQuery
     * \since QGIS 3.0
     */
    QString mQuery;

    /**
     * Flag indicating if the layer data source is based on a query
     * \note
     *
     * \see checkQuery
     * \since QGIS 3.0
     */
    bool mIsQuery;

    /**
     * DB full path
     * extracted from dataSourceUri()
     * \note
     * The Uri is based on the ogr version, which is NOT supported by QgsDataSourceUri
     * \since QGIS 3.0
     */
    QString mSqlitePath;
    QgsCoordinateReferenceSystem mCrs;

    friend class QgsSpatiaLiteFeatureIterator;
    friend class QgsSpatiaLiteExpressionCompiler;
};

class QgsSpatiaLiteFeatureIterator : public QgsAbstractFeatureIteratorFromSource<QgsSpatiaLiteFeatureSource>
{
  public:
    QgsSpatiaLiteFeatureIterator( QgsSpatiaLiteFeatureSource *source, bool ownSource, const QgsFeatureRequest &request );

    ~QgsSpatiaLiteFeatureIterator();
    virtual bool rewind() override;
    virtual bool close() override;

  protected:

    virtual bool fetchFeature( QgsFeature &feature ) override;
    bool nextFeatureFilterExpression( QgsFeature &f ) override;

  private:

    QString whereClauseRect();
    QString whereClauseFid();
    QString whereClauseFids();
    bool prepareStatement( const QString &whereClause, long limit = -1, const QString &orderBy = QString() );
    QString quotedPrimaryKey();
    bool getFeature( sqlite3_stmt *stmt, QgsFeature &feature );
    QString fieldName( const QgsField &fld );
    QVariant getFeatureAttribute( sqlite3_stmt *stmt, int ic, QVariant::Type type, QVariant::Type subType );
    void getFeatureGeometry( sqlite3_stmt *stmt, int ic, QgsFeature &feature );

    //! wrapper of the SQLite database connection
    QgsSqliteHandle *mHandle = nullptr;

    /**
      * SQLite statement handle
     */
    sqlite3_stmt *sqliteStatement = nullptr;

    //! Geometry column index used when fetching geometry
    int mGeomColIdx;

    //! Sets to true, if geometry is in the requested columns
    bool mFetchGeometry = true;

    bool mHasPrimaryKey;
    QgsFeatureId mRowNumber;

    bool prepareOrderBy( const QList<QgsFeatureRequest::OrderByClause> &orderBys ) override;

    bool mOrderByCompiled;
    bool mExpressionCompiled;

    QgsRectangle mFilterRect;
    QgsCoordinateTransform mTransform;
};

#endif // QGSSPATIALITEFEATUREITERATOR_H
