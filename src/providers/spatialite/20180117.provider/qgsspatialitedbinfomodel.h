/***************************************************************************
                         qgsspatialitedbinfomodel.h  -  description
                         -------------------
    begin                : December 2017
    copyright            : (C) 2017 by Mark Johnson
    email                : mj10777@googlemail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSSPATIALITEDBINFOMODEL_H
#define QGSSPATIALITEDBINFOMODEL_H

#include <QAbstractItemModel>
#include <QStandardItem>
#include <QModelIndex>
#include <QVariant>
#include <QList>
#include <QVector>
#include "qgsspatialiteconnection.h"
// -- ---------------------------------- --
// Based on QT-Demo editabletreemodel [/qtbase/examples/widgets/itemviews/editabletreemodel/]
// qmake ; make ; ./editabletreemodel &
// -- ---------------------------------- --
class QgsSpatialiteDbInfo;
class QgsSpatialiteDbLayer;
class QgsSpatialiteDbInfoItem;

/**
 * This TreeView Modul will reflect the information stored in the QgsSpatialiteDbInfo class
  * - which contains all information collected from a Spatialite-Database
  * \note
  *  - The QgsSpatialiteDbInfo is contained in the QgsSqliteHandle class
  *  -> so that the Information must only be called once for each connection
  *  --> When shared, the QgsSqliteHandle class will used the same connection of each Layer in the same Database
  * \see QgsSqliteHandle
  * \see QgsSpatialiteDbInfo
  * \see QgsSpatialiteDbLayer
  * \since QGIS 3.0
 */
// bin/qgis': double free or corruption
class QgsSpatialiteDbInfoModel : public QAbstractItemModel
{
    Q_OBJECT

  public:
    QgsSpatialiteDbInfoModel( QObject *parent = nullptr );
    ~QgsSpatialiteDbInfoModel();

    /**
     * General function to retrieve Data from Item
     * \note
     *  - should be used by specific column functions [candidate to be moved to private]
    * \returns QVariant of column value
    * \since QGIS 3.0
    */
    QVariant data( const QModelIndex &index, int role ) const Q_DECL_OVERRIDE;

    /**
     * Retrieve Header Column Name
     * \note
     *  - may not be needed [candidate to be removed]
    * \returns QVariant of column value
    * \since QGIS 3.0
    */
    QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;

    QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const Q_DECL_OVERRIDE;
    QModelIndex parent( const QModelIndex &index ) const Q_DECL_OVERRIDE;

    int rowCount( const QModelIndex &parent = QModelIndex() ) const Q_DECL_OVERRIDE;

    /**
     * Returns amount of Columns of the Model of the rootItem
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns count of columns
    * \since QGIS 3.0
    */
    int columnCount( const QModelIndex &parent = QModelIndex() ) const Q_DECL_OVERRIDE;

    /**
     * Returns the set flag for the given Item's column of the row
     * \note
     *  - set during construction of Item
    * \param index with row() and column() of Item
    * \returns flags for the given row and column of the Item
    * \see QgsSpatialiteDbInfoItem::flags
    * \since QGIS 3.0
    */
    Qt::ItemFlags flags( const QModelIndex &index ) const Q_DECL_OVERRIDE;
    bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole ) Q_DECL_OVERRIDE;
    bool setHeaderData( int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole ) Q_DECL_OVERRIDE;

    bool insertColumns( int position, int columns, const QModelIndex &parent = QModelIndex() ) Q_DECL_OVERRIDE;
    bool removeColumns( int position, int columns, const QModelIndex &parent = QModelIndex() ) Q_DECL_OVERRIDE;
    bool insertRows( int position, int rows, const QModelIndex &parent = QModelIndex() ) Q_DECL_OVERRIDE;
    bool removeRows( int position, int rows, const QModelIndex &parent = QModelIndex() ) Q_DECL_OVERRIDE;

    /**
     * Returns the Index of Column 'Hidden' from the selected Item
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
    * \see mColumnSortHidden
    * \see QgsSpatiaLiteSourceSelect::on_mSearchColumnComboBox_currentIndexChanged
    * \since QGIS 3.0
    */
    int getColumnSortHidden() const { return mColumnSortHidden; }

    /**
     * Returns the Index of Column 'Table' from the selected Item
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
    * \see mColumnTable
    * \see QgsSpatiaLiteSourceSelect::on_mSearchColumnComboBox_currentIndexChanged
    * \since QGIS 3.0
    */
    int getTableNameIndex() const { return mColumnTable; }

    /**
     * Returns the Text of Column 'Table' from the Header
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
    * \see mColumnTable
    * \see QgsSpatiaLiteSourceSelect::QgsSpatiaLiteSourceSelect
    * \since QGIS 3.0
    */
    QString getTableNameText() const;

    /**
     * Returns the Value of Column 'Table' from the selected Item
     * \note
     *  - Index value set during construction. Avoid manual use of numeric value in code
    * \returns QString-Value of column being used in QgsSpatiaLiteSourceSelect
    * \see getTableNameIndex
    * \see QgsSpatiaLiteSourceSelect::on_mSearchColumnComboBox_currentIndexChanged
    * \since QGIS 3.0
    */
    QString getTableName( const QModelIndex &index ) const;

    /**
     * Returns the Index of Column 'Geometry' from the selected Item
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
    * \see mColumnGeometryName
    * \see QgsSpatiaLiteSourceSelect::on_mSearchColumnComboBox_currentIndexChanged
    * \since QGIS 3.0
    */
    int getGeometryNameIndex() const { return mColumnGeometryName; }

    /**
     * Returns the Text of Column 'Geometry' from the Header
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
    * \see mColumnGeometryName
    * \see QgsSpatiaLiteSourceSelect::QgsSpatiaLiteSourceSelect
    * \since QGIS 3.0
    */
    QString getGeometryNameText() const;

    /**
     * Returns the Value of Column 'Geometry-Type' from the selected Item
     * \note
     *  - Index value set during construction. Avoid manual use of numeric value in code
    * \returns QString-Value of column being used in QgsSpatiaLiteSourceSelect
    * \see getTableNameIndex
    * \see QgsSpatiaLiteSourceSelect::on_mSearchColumnComboBox_currentIndexChanged
    * \since QGIS 3.0
    */
    QString getGeometryName( const QModelIndex &index ) const;

    /**
     * Returns the LayerName based on  Values of the Columns 'Table' and 'Geometry' from the selected Item
     * \note
     *  - checking is done to insure the the retured value exists
    * \returns QString Value of LayerName
    * \see getTableNameIndex
    * \see getLayerNameUris
    * \since QGIS 3.0
    */
    QString getLayerName( const QModelIndex &index ) const;

    /**
     * Returns the Urls based on  LayerName  from the selected Item
     * \note
     *  - checking is done to insure the the retured value exists
    * \returns QString Value of LayerName
    * \see getLayerName
    * \since QGIS 3.0
    */
    QString getLayerNameUris( const QModelIndex &index ) const;

    /**
     * Returns the Index of Column 'Geometry' from the selected Item
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
    * \see mColumnGeometryType
    * \see QgsSpatiaLiteSourceSelect::on_mSearchColumnComboBox_currentIndexChanged
    * \since QGIS 3.0
    */
    int getGeometryTypeIndex() const { return mColumnGeometryType; }

    /**
     * ExpandToDepth
     * \note
     *  - 3: ItemTypeHelpRoot: all level should be shown
     *  - 2: - ItemTypeDb: down to Table/View name, but not the Columns
     * \since QGIS 3.0
     */
    int getExpandToDepth();

    /**
     * Returns the Text of Column 'Geometry-Type' from the Header
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
    * \see mColumnGeometryType
    * \see QgsSpatiaLiteSourceSelect::QgsSpatiaLiteSourceSelect
    * \since QGIS 3.0
    */
    QString getGeometryTypeText() const;

    /**
     * Returns the GeometryType based on  Value of the Column 'GeometryType' from the selected Item
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns QString Value of GeometryType
    * \see getTableNameIndex
    * \see getLayerNameUris
    * \since QGIS 3.0
    */
    QString getGeometryType( const QModelIndex &index ) const;

    /**
     * Returns the Index of Column 'Sql' from the selected Item
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
    * \see getSqlQueryIndex
    * \see QgsSpatiaLiteSourceSelect::on_mSearchColumnComboBox_currentIndexChanged
    * \since QGIS 3.0
    */
    int getSqlQueryIndex() const { return mColumnSql; }

    /**
     * Returns the Text of Column 'SqlQuery' from the Header
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
    * \see mColumnSql
    * \see QgsSpatiaLiteSourceSelect::QgsSpatiaLiteSourceSelect
    * \since QGIS 3.0
    */
    QString getSqlQueryText() const;

    /**
     * Returns the Sql-Query based on  Value of the Column 'Sql' from the selected Item
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns QString Value of GeometryType
    * \see getTableNameIndex
    * \see getLayerNameUris
    * \since QGIS 3.0
    */
    QString getSqlQuery( const QModelIndex &index ) const;

    /**
     * Sets an sql statement that belongs to a cell specified by a model index
     * \note
     *  - checking is done that the Table and Geometry-Name are valid
    * \returns QString Value of GeometryType
    * \see getTableNameIndex
    * \see getGeometryNameIndex
    * \see QgsSpatiaLiteSourceSelect::on_mTablesTreeView_doubleClicked
    * \since QGIS 3.0
    */
    void setSql( const QModelIndex &index, const QString &sql );

    /**
     * Set the active Database
     * \note
     *  - this will build the Table-Model
    * \returns QString Value of Database file
    * \see QgsSpatiaLiteSourceSelect::setQgsSpatialiteDbInfo
    * \see QgsSpatialiteDbInfoModel::createDatabase
    * \since QGIS 3.0
    */
    void setSpatialiteDbInfo( QgsSpatialiteDbInfo *spatialiteDbInfo );

    /**
     * Returns the active Database file-path
     * \note
     *  - checking is done to insure the the retured value exists
    * \param withPath default: full path of file, otherwise base-name of file
    * \returns QString Value of Database file
    * \see QgsSpatiaLiteSourceSelect::updateStatistics
    * \since QGIS 3.0
    */
    QString getDbName( bool withPath = true ) const;

    /**
     * Connection info (DB-path) without table and geometry
     * - this will be called from classes using QgsSpatialiteDbInfo
     * \note
     *  - to call for Database and Table/Geometry portion use: QgsSpatialiteDbLayer::getLayerDataSourceUri()
    * \returns uri with Database only
    * \see QgsSpatialiteDbLayer::getLayerDataSourceUri()
    * \since QGIS 3.0
    */
    QString getDatabaseUri() const;

    /**
     * Retrieve QgsSpatialiteDbInfo
     * - containing all Information about Database file
     * \note
     * - isDbValid() return if the connection contains layers that are supported by
     * -- QgsSpatiaLiteProvider, QgsGdalProvider and QgsOgrProvider
     * \see QgsSpatialiteDbInfo::isDbValid()
     * \since QGIS 3.0
     */
    QgsSpatialiteDbInfo *getSpatialiteDbInfo() const { return mSpatialiteDbInfo; }

    /**
     * UpdateLayerStatistics for the Database or Layers
     * - this will be called from the QgsSpatialiteDbLayer::UpdateLayerStatistics
     * - this is also called with a selection of tables/geometries
     *  -> calls InvalidateLayerStatistics before UpdateLayerStatistics
     * \note
     *  - if the sent List is empty or containe 1 empty QString
     *  -> the whole Database will be done
     *  - when only a specific table or table with geometry is to be done
     *  -> format: 'table_name(geometry_name)' or only 'table_name', with all of its geometries
     *  - otherwise all tables with geometries
     *  - All commands within a TRANSACTION
     * \param saLayers List of LayerNames formatted as 'table_name(geometry_name)'
     * \returns result of the last returned rc
     * \see QgsSpatialiteDbInfo::::UpdateLayerStatistics
     * \since QGIS 3.0
     */
    bool UpdateLayerStatistics( QStringList saLayers );

    /**
     * Is the Container a Spatialite Database
     * \note
     *  - Spatialite specific functions should not be called when false
     *  -> UpdateLayerStatistics()
     * \see QgsSpatialiteDbInfo::isDbSpatialite()
     * \since QGIS 3.0
     */
    bool isSpatialite() const
    {
      if ( ( mSpatialiteDbInfo ) && ( mSpatialiteDbInfo->isDbValid() ) && ( mSpatialiteDbInfo->isDbSpatialite() ) )
      {
        return true;
      }
      return false;
    }

    /**
     * List of DataSourceUri of valid Layers
     * -  contains Layer-Name and DataSourceUri
     * \note
     * - Lists all Layer-Types (SpatialTable/View, RasterLite1/2, Topology and VirtualShape)
     * - and MbTiles, FdoOgr and Geopoackage
     * - Key: LayerName formatted as 'table_name(geometry)'
     * - Value: dependent on provider
     * -> Spatialite: 'PathToFile table="table_name" (geometry_name)'
     * -> RasterLite2: [TODO] 'PathToFile table="coverage_name"'
     * -> RasterLite1 [Gdal]: 'RASTERLITE:/long/path/to/database/ItalyRail.atlas,table=srtm'
     * -> MBTiles [Gdal]: 'PathToFile'
     * -> FdoOgr [Ogr]:  'PathToFile|layername=table_name(geometry)'
     * -> GeoPackage [Ogr]: 'PathToFile|layername=table_name'
     * \see mDbLayersDataSourceUris
     * \see mDbLayers
     * \see getQgsSpatialiteDbLayer
     * \see prepareDataSourceUris
     * \since QGIS 3.0
     */
    QMap<QString, QString> getDataSourceUris() const { return mSpatialiteDbInfo->getDataSourceUris(); }

    /**
     * Map of valid Selected Layers requested by the User
     * - only Uris that created a valid QgsVectorLayer/QgsRasterLayer
     * -> corresponding QgsMapLayer contained in  getSelectedDb??????Layers
     * \note
     * - Key: LayerName formatted as 'table_name(geometry)'
     * - Value: Uris dependent on provider
     * - can be for both QgsSpatiaLiteProvider or QgsGdalProvider
     * \returns mSelectedLayersUris  Map of LayerNames and  valid Layer-Uris entries
     * \see QgsSpatialiteDbInfo::setSelectedDbLayers
     * \see QgsSpatialiteDbInfo::getSelectedDbRasterLayers
     * \see QgsSpatialiteDbInfo::getSelectedDbVectorLayers
     * \since QGIS 3.0
     */
    QMap<QString, QString>  getSelectedLayersUris() const { return mSpatialiteDbInfo->getSelectedLayersUris(); }

    /**
     * Add a list of database layers to the map
     * - to fill a Map of QgsVectorLayers and/or QgsRasterLayers
     * -> can be for both QgsSpatiaLiteProvider or QgsOgr/GdalProvider
     * \note
     * - requested by User to add to main QGis
     * -> emit addDatabaseLayers for each supported Provider
     * \param saSelectedLayers formatted as 'table_name(geometry_name)'
     * \param saSelectedLayersSql Extra Sql-Query for selected Layers
     * \returns amount of Uris entries
     * \see getSelectedLayersUris
     * \since QGIS 3.0
     */
    int addDbMapLayers( QStringList saSelectedLayers, QStringList saSelectedLayersSql ) const {  return mSpatialiteDbInfo->addDbMapLayers( saSelectedLayers, saSelectedLayersSql );  }

    /**
     * Create a new Database
     *  - for use with QgsSpatialiteDbInfo::createDatabase
     * \note
     * - SpatialMetadata::Spatialite40: InitSpatialMetadata only
     * - SpatialMetadata::Spatialite50: also with Styles/Raster/VectorCoveragesTable's
     * - SpatialMetadata::SpatialiteGpkg: using spatialite 'gpkgCreateBaseTables' function
     * - SpatialMetadata::SpatialiteMBTiles: creating a view-base MbTiles file with grid tables
     * \returns true if the created Database is valid and of the Container-Type requested
     * \see QgsSpatialiteDbInfo::createDatabase
     * \since QGIS 3.0
     */
    bool createDatabase( QString sDatabaseFileName, QgsSpatialiteDbInfo::SpatialMetadata dbCreateOption = QgsSpatialiteDbInfo::Spatialite40 );

    /**
     * Returns the number of tables in the model
     * \note
     *  - set, but not not used [candidate to be removed]
    * \returns count of Tables in Database
    * \since QGIS 3.0
    */
    int tableCount() const
    {
      return mTableCount;
    }

    /**
     * Returns QgsSpatialiteDbInfoItem of the selected Item
     * \note
     *  - QgsSpatialiteDbInfoItem
     * Can be used in the same way as 'itemFromIndex' in QStandardItemModel
    * \returns QgsSpatialiteDbInfoItem of type QgsSpatialiteDbInfoItem
    * \since QGIS 3.0
    */
    QgsSpatialiteDbInfoItem *getItem( const QModelIndex &index ) const;

    /**
     * Return layer tree node for given index. Returns root node for invalid index.
     * Returns null pointer if index does not refer to a layer tree node (e.g. it is a legend node)
    * \since QGIS 3.0
     */
    QgsSpatialiteDbInfoItem *index2node( const QModelIndex &index ) const;

    /**
     * Return index for a given node. If the node does not belong to the layer tree, the result is undefined
    * \since QGIS 3.0
     */
    QModelIndex node2index( QgsSpatialiteDbInfoItem *node ) const;

    /**
     * Loads Help text after Contruction
     * - Display text for the User, giving some information about what the Model is used for
     * \note
     *  This can only be called after 'connectToRootItem' has run
     *  - Will be removed when the first Database is loaded
     * \see setSpatialiteDbInfo
    * \since QGIS 3.0
     */
    void loadItemHelp();

  private:

    enum EntryType
    {
      EntryTypeLayer = 0,
      EntryTypeMap = 1
    };

    /**
     * Placeholder for RootItem
     * - used as insert point of child Spatial Items when building
     * \note
     * - removes all entries when a new Database is being opened
     * \see setSpatialiteDbInfo
     * \see addDatabaseEntry
     * \since QGIS 3.0
     */
    QgsSpatialiteDbInfoItem *mDbRootItem = nullptr;

    /**
     * Placeholder for HelpItem
     * - used as insert point of child Spatial Items when building
     * \note
     * - removes all entries when a new Database is being opened
     * \see setSpatialiteDbInfo
     * \see addDatabaseEntry
     * \since QGIS 3.0
     */
    QgsSpatialiteDbInfoItem *mDbHelpRoot = nullptr;

    /**
     * Placeholder for RootItem
     * - used as insert point of child Spatial Items when building
     * \note
     * - removes all entries when a new Database is being opened
     * \see setSpatialiteDbInfo
     * \see addDatabaseEntry
     * \since QGIS 3.0
     */
    QgsSpatialiteDbInfoItem *mDbItem = nullptr;

    /**
     * Placeholder for NonSpatial Items
     * - used as insert point of child NonSpatial Items when building
     * \note
     * - removes all entries when a new Database is being opened
     * \see addTableItemMap
     * \since QGIS 3.0
     */
    QgsSpatialiteDbInfoItem *mNonSpatialItem = nullptr;

    /**
     * List of DataSourceUri of valid Layers
     * -  contains Layer-Name and DataSourceUri
     * \note
     * - Lists all Layer-Types (SpatialTable/View, RasterLite1/2, Topology and VirtualShape)
     * - and MbTiles, FdoOgr and Geopoackage
     * - Key: LayerName formatted as 'table_name(geometry)'
     * - Value: dependent on provider
     * -> Spatialite: 'PathToFile table="table_name" (geometry_name)'
     * -> RasterLite2: [TODO] 'PathToFile table="coverage_name"'
     * -> RasterLite1 [Gdal]: 'RASTERLITE:/long/path/to/database/ItalyRail.atlas,table=srtm'
     * -> MBTiles [Gdal]: 'PathToFile'
     * -> FdoOgr [Ogr]:  'PathToFile|layername=table_name(geometry)'
     * -> GeoPackage [Ogr]: 'PathToFile|layername=table_name'
     * \see mDbLayersDataSourceUris
     * \see mDbLayers
     * \see getQgsSpatialiteDbLayer
     * \see prepareDataSourceUris
     * \since QGIS 3.0
     */
    QMap<QString, QString> mDbLayersDataSourceUris;

    /**
     * Returns the Index of Column 'Hidden' from the selected Item
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
    * \see getColumnSortHidden
    * \see QgsSpatiaLiteSourceSelect::on_mSearchColumnComboBox_currentIndexChanged
    * \since QGIS 3.0
    */
    int mColumnSortHidden;

    /**
     * Returns the Index of Column 'Table' from the selected Item
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
    * \see getTableNameIndex
    * \see getTableName
    * \since QGIS 3.0
    */
    int mColumnTable;

    /**
     * Returns the Index of Column 'Geometry' from the selected Item
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
    * \see getGeometryTypeIndex
    * \see QgsSpatiaLiteSourceSelect::on_mSearchColumnComboBox_currentIndexChanged
    * \since QGIS 3.0
    */
    int mColumnGeometryType;

    /**
     * Returns the Index of Column 'Geometry' from the selected Item
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
    * \see getGeometryNameIndex
    * \see QgsSpatiaLiteSourceSelect::on_mSearchColumnComboBox_currentIndexChanged
    * \since QGIS 3.0
    */
    int mColumnGeometryName;

    /**
     * Returns the Index of Column 'Sql' from the selected Item
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
    * \see getSqlQueryIndex
    * \see getSqlQuery
    * \since QGIS 3.0
    */
    int mColumnSql;

    /**
     * QgsSpatialiteDbInfo Object
     * - containing all Information about Database file
     * \note
     * - isDbValid() return if the connection contains layers that are supported by
     * -- QgsSpatiaLiteProvider, QgsGdalProvider and QgsOgrProvider
     * \see QgsSpatialiteDbInfo::isDbValid()
     * \since QGIS 3.0
     */
    QgsSpatialiteDbInfo *mSpatialiteDbInfo = nullptr;
    QgsSpatialiteDbLayer *mDbLayer = nullptr;

    /**
     * Coulumn Names of Model
     * \note
     *  - set during construction.
    * \since QGIS 3.0
    */
    QVector<QVariant> mRootData;

    /**
     * Returns the number of tables in the model
     * \note
     *  - set, but not not used [candidate to be removed]
    * \returns count of Tables in Database
    * \since QGIS 3.0
    */
    int mTableCount;

    /**
     * Builds expected amout of Tables the number of tables in the model
     * \note
     *  - set and used for display
    * \returns count of Tables in Database
    * \see QgsSpatialiteDbInfo::getTableCounter
    * \since QGIS 3.0
    */
    int mTableCounter = 0;

    /**
     *  Total amount of non-SpatialTables/Views in Database
     * \note
     *  - Used to show the total amount of Layers
     * - calculated only ItemTypeDb from onInit, retrieved from parent for others
    * \see sniffSpatialiteDbInfo
    * \since QGIS 3.0
    */
    int mNonSpatialTablesCounter = 0;

    /**
     * Returns a list of items that match the given \a text, using the given \a flags, in the given \a column.
     * \note
     * - taken from QStandardItemModel
     * \since QGIS 3.0
     */
    QList<QgsSpatialiteDbInfoItem *> findItems( const QString &text, Qt::MatchFlags flags = Qt::MatchExactly, int column = 0 ) const;

    /**
     * Connect the RootNode to signels for creation/removel and changes to the QgsSpatialiteDbInfoItem
     * \note
     * - taken from QgsLayerTreeModel
     * \see mDbRootItem
     * \since QGIS 3.0
     */
    void connectToRootItem();

    /**
     * Disconnect the RootNode to signels for creation/removel and changes to the QgsSpatialiteDbInfoItem
     * - may not needed, since the mDbRootItem will remain constant
     * \note
     * - taken from QgsLayerTreeModel
     * \see mDbRootItem
     * \since QGIS 3.0
     */
    void disconnectFromRootItem();

    /**
     * ExpandToDepth
     * \note
     *  - for use with mTablesTreeView->expandToDepth
     * \since QGIS 3.9
     */
    int mExpandToDepth = 3;

  protected slots:
    void nodeWillAddChildren( QgsSpatialiteDbInfoItem *node, int indexFrom, int indexTo );
    void nodeAddedChildren( QgsSpatialiteDbInfoItem *node, int indexFrom, int indexTo );
    void nodeWillRemoveChildren( QgsSpatialiteDbInfoItem *node, int indexFrom, int indexTo );
    void nodeRemovedChildren();
};

/**
 * Class to contain all information stored in the mItemData member of the QgsSpatialiteDbInfoItem
  * \note
  *  One QgsSpatialiteDbInfoItem object will contain all Columns
  *  - helper functions exist, to set or extract specific Information-Types for a specific Column
  * \see column
  * \see value
  * \see role
  * \see QgsSpatialiteDbInfoItem::setData
  * \see QgsSpatialiteDbInfoItem::data
  * \see QgsSpatialiteDbInfoItem::setText
  * \see QgsSpatialiteDbInfoItem::text
  * \see QgsSpatialiteDbInfoItem::setFlags
  * \see QgsSpatialiteDbInfoItem::flags
  * \see QgsSpatialiteDbInfoItem::setIcon
  * \see QgsSpatialiteDbInfoItem::icon
  * \see QgsSpatialiteDbInfoItem::setBackground
  * \see QgsSpatialiteDbInfoItem::setForeground
  * \see QgsSpatialiteDbInfoItem::setAlignment
  * \see QgsSpatialiteDbInfoItem::setToolTip
  * \see QgsSpatialiteDbInfoItem::toolTip
  * \see QgsSpatialiteDbInfoItem::setStatusTip
  * \see QgsSpatialiteDbInfoItem::statusTip
  * \see QgsSpatialiteDbInfoItem::setFlags
  * \see QgsSpatialiteDbInfoItem::flags
  * \see QgsSpatialiteDbInfoItem::setSelectable
  * \see QgsSpatialiteDbInfoItem::isSelectable
  * \see QgsSpatialiteDbInfoItem::setEditable
  * \see QgsSpatialiteDbInfoItem::isEditable
  * \see QgsSpatialiteDbInfoItem::setEnabled
  * \see QgsSpatialiteDbInfoItem::isEnabled
  * \since QGIS 3.0
 */
class QgsSpatialiteDbInfoData
{
  public:
    inline QgsSpatialiteDbInfoData() : role( -1 ) {}
    inline QgsSpatialiteDbInfoData( int r, const QVariant &v, int c ) : role( r ), value( v ), column( c ) {}
    int role;
    QVariant value;
    int column;
    inline bool operator==( const QgsSpatialiteDbInfoData &other ) const { return role == other.role && value == other.value && column == other.column; }
};
Q_DECLARE_TYPEINFO( QgsSpatialiteDbInfoData, Q_MOVABLE_TYPE );
// -- ---------------------------------- --

/**
 * Class to contain all information needed for a QgsSpatialiteDbInfo object
  * \note
  *  An Item-Column, is by default:
  *  -> Enabled, Selectable but not Editable but is set with Qt::EditRole
  *  --> setEditable must be called for each Column for which this is allowed and the TreeView must be properly set
  * TreeView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed)
  * \see QgsSpatialiteDbInfo
  * \see QgsSpatialiteDbLayer
  * \since QGIS 3.0
 */
class QgsSpatialiteDbInfoItem : public QObject
{
    Q_OBJECT

  public:
    enum SpatialiteDbInfoItemType
    {
      ItemTypeRoot = 0,
      ItemTypeDb = 1,
      ItemTypeLayer = 2,
      ItemTypeColumn = 3,
      ItemTypeMBTilesMetadata = 4,
      ItemTypeAdmin = 100,
      ItemTypeNonSpatialTablesGroup = 101,
      ItemTypeHelpText = 998,
      ItemTypeHelpRoot = 999,
      ItemTypeGroupSpatialTable = QgsSpatialiteDbInfo::SpatialTable + 10000,
      ItemTypeGroupSpatialView = QgsSpatialiteDbInfo::SpatialView + 10000,
      ItemTypeGroupVirtualShape = QgsSpatialiteDbInfo::VirtualShape + 10000,
      ItemTypeGroupRasterLite1 = QgsSpatialiteDbInfo::RasterLite1 + 10000,
      ItemTypeGroupRasterLite2Vector = QgsSpatialiteDbInfo::RasterLite2Vector + 10000,
      ItemTypeGroupRasterLite2Raster = QgsSpatialiteDbInfo::RasterLite2Raster + 10000,
      ItemTypeGroupSpatialiteTopology = QgsSpatialiteDbInfo::SpatialiteTopology + 10000,
      ItemTypeGroupTopologyExport = QgsSpatialiteDbInfo::TopologyExport + 10000,
      ItemTypeGroupStyleVector = QgsSpatialiteDbInfo::StyleVector + 10000,
      ItemTypeGroupStyleRaster = QgsSpatialiteDbInfo::StyleRaster + 10000,
      ItemTypeGroupGdalFdoOgr = QgsSpatialiteDbInfo::GdalFdoOgr + 10000,
      ItemTypeGroupGeoPackageVector = QgsSpatialiteDbInfo::GeoPackageVector + 10000,
      ItemTypeGroupGeoPackageRaster = QgsSpatialiteDbInfo::GeoPackageRaster + 10000,
      ItemTypeGroupMBTilesTable = QgsSpatialiteDbInfo::MBTilesTable + 10000,
      ItemTypeGroupMBTilesView = QgsSpatialiteDbInfo::MBTilesView + 10000,
      ItemTypeGroupMetadata = QgsSpatialiteDbInfo::Metadata + 10000,
      ItemTypeGroupAllSpatialLayers = QgsSpatialiteDbInfo::AllSpatialLayers + 10000,
      ItemTypeGroupNonSpatialTables = QgsSpatialiteDbInfo::NonSpatialTables + 10000,
      ItemTypeUnknown = 99999
    };

    /**
     * QgsSpatialiteDbInfoItem Object
     * - containing all Information about Database file
     * \note
     * - this constructor will call everything it needed to represent the Database
     * -> in sniffSpatialiteDbInfo
     * \param parent parent Item
     * \param spatialiteDbInfo QgsSpatialiteDbInfo to represent/display [may be nullptr]
     * \see buildDbInfoItem
     * \see sniffSpatialiteDbInfo
     * \since QGIS 3.0
     */
    explicit QgsSpatialiteDbInfoItem( QgsSpatialiteDbInfoItem *parent, QgsSpatialiteDbInfo *spatialiteDbInfo );

    /**
     * QgsSpatialiteDbInfoItem Object
     * - containing all Information about Database file
     * \note
     * - isDbValid() return if the connection contains layers that are supported by
     * -- QgsSpatiaLiteProvider, QgsGdalProvider and QgsOgrProvider
     * \param parent parent Item
     * \param dbLayer QgsSpatialiteDbLayer to represent/display [may be nullptr]
     * \see QgsSpatialiteDbInfo::isDbValid()
     * \since QGIS 3.0
     */
    explicit QgsSpatialiteDbInfoItem( QgsSpatialiteDbInfoItem *parent, QgsSpatialiteDbLayer *dbLayer );

    /**
     * QgsSpatialiteDbInfoItem Object
     * - creating a Sub-Group
     * \note
     * \param parent parent Item
     * \param itemType Type of Item-Group
     * \see createGroupLayerItem
     * \see createGroupLayerItem
     * \since QGIS 3.0
     */
    explicit QgsSpatialiteDbInfoItem( QgsSpatialiteDbInfoItem *parent, SpatialiteDbInfoItemType itemType );

    /**
     * QgsSpatialiteDbInfoItem Object
     * - creating a Sub-Group inside a tem-Group
     * \note
     * \param parent parent Item
     * \param itemType Type of SubGroup
     * \param sGroupName Name of SubGroup
     * \param saTableNames List of TableName to add
     * \see addTableItemMap
     * \since QGIS 3.0
     */
    explicit QgsSpatialiteDbInfoItem( QgsSpatialiteDbInfoItem *parent, SpatialiteDbInfoItemType itemType, QString sItemName, QStringList saItemInfos );

    /**
     * QgsSpatialiteDbInfoItem distructor
     * \note
     * - imDbLayer /QgsSpatialiteDbLayer) will be set to nullptr
     * -> must never be deleted, may be used elsewhere
     * \since QGIS 3.0
     */
    ~QgsSpatialiteDbInfoItem();

    /**
     * Connect the Node to signels for creation/removel and changes to the QgsSpatialiteDbInfoItem
     * - forward the signal towards the root
     * \note
     * - taken from QgsLayerTreeModel
     * \see mDbRootItem
     * \since QGIS 3.0
     */
    void connectToChildItem( QgsSpatialiteDbInfoItem *node );

    /**
     * Add Columns Items to Geometry-Table entry
     * \note
     * - mDbLayer must be set
     * -> displaying information about the columns
     * --> this does not include the geometry of the Layer
    * \returns count of colums the geometry belongs to
     * \since QGIS 3.0
     */
    int addColumnItems();

    /**
     * Add Raster Metadata Items to RasterLite1, RasterLite2 and GeoPackage-Raster-Table entry
     * \note
     * - must be a MBTiles-Container
     * -> displaying name, value of the metadata-Table
    * \returns count of entries found
     * \since QGIS 3.0
     */
    int addCommonMetadataItems();

    /**
     * Add Columns Items to MBTiles-Table entry
     * \note
     * - must be a MBTiles-Container
     * -> displaying name, value of the metadata-Table
    * \returns count of entries found
     * \since QGIS 3.0
     */
    int addMbTilesMetadataItems();

    /**
     * Common task during construction
     * \note
     * - mDbLayer /QgsSpatialiteDbLayer) will be set to nullptr
     * -> must never be deleted, may be used elsewhere
     * \since QGIS 3.0
     */
    bool onInit();

    /**
     * Resolve unset settings
     * Goal: to (semi) Automate unresolved settings when needed
     * \since QGIS 3.0
     */
    QgsSpatialiteDbInfoItem *child( int number );

    /**
     * How many children does this Item contain
     * \since QGIS 3.0
     */
    int childCount() const;

    /**
     * How many columns does this Item contain
     * \note
     *  default: mColumnSortHidden + 1
     * \since QGIS 3.0
     */
    int columnCount() const;

    /**
     * Add a child to this Item
     * \note
     *  willAddChildren and addedChildren will be emitted when bWillAddChildren=true
     *  - calling connectToChildItem to the child
     *  Using willAddChildren crashes the Application and is works without it
     * \param child Child to add
     * \param position At which position to insert child
     * \param bWillAddChildren emit willAddChildren  [default=false]
     * \see connectToChildItem
     * \since QGIS 3.0
     */
    bool addChild( QgsSpatialiteDbInfoItem *child, int position = -1, bool bWillAddChildren = false );

    /**
     * Add x-amount of children to this Item
     * \note
     *  willAddChildren and addedChildren will be emitted
     *  - calling connectToChildItem to each child
     * \param count amount of Children to add [default=1]
     * \param itemType ItemType to set [default=ItemTypeUnknown]
     * \param columns amount of Columns to add [default=-1 (takes value of  columnCount())]
     * \see connectToChildItem
     * \see columnCount()
     * \since QGIS 3.0
     */
    bool insertChildren( int position, int count = 1, SpatialiteDbInfoItemType itemType = ItemTypeUnknown, int columns = -1 );

    /**
     * Add Prepaired children to this Item
     * \note
     *  willAddChildren and addedChildren will be emitted
     *  - calling connectToChildItem to each child
     * \see connectToChildItem
     * \since QGIS 3.0
     */
    bool insertPrepairedChildren();

    /**
     * Insert columns into Item
     * \note
     *  presently not used
     *  canditate to be removed
     * \since QGIS 3.0
     */
    bool insertColumns( int position, int columns );

    /**
     * Retrieve Parent Item
     * \since QGIS 3.0
     */
    QgsSpatialiteDbInfoItem *parent() const { return mParentItem; }

    /**
     * Insert columns into Item
     * \note
     *  presently not used
     *  canditate to be removed
     * \since QGIS 3.0
     */
    bool removeChildren( int position, int count );
    bool removeColumns( int position, int columns );

    /**
     * Position of this Item in the Parent Item's Children
     * \since QGIS 3.0
     */
    int childNumber() const;

    /**
     * Retrieve QgsSpatialiteDbInfo
     * - containing all Information about Database file
     * \note
     * - isDbValid() return if the connection contains layers that are supported by
     * -- QgsSpatiaLiteProvider, QgsGdalProvider and QgsOgrProvider
     * \see QgsSpatialiteDbInfo::isDbValid()
     * \since QGIS 3.0
     */
    QgsSpatialiteDbInfo *getSpatialiteDbInfo() const { return mSpatialiteDbInfo; }

    /**
     * Set the active Database
     * \note
     *  - this will build the Table-Model
    * \returns QString Value of Database file
    * \see QgsSpatiaLiteSourceSelect::setQgsSpatialiteDbInfo
    * \see QgsSpatialiteDbInfoModel::createDatabase
    * \since QGIS 3.0
    */
    bool setSpatialiteDbInfo( QgsSpatialiteDbInfo *spatialiteDbInfo );

    /**
     * Collection of reasons for the Database-Layer being invalid
     * \note
     *  -> format: 'table_name(geometry_name)', Error or Warning text
     * \param bWarnings default:false return Errors, else Warnings Messages
     * \returns QMape of Strings with function name and Text of Warning/Error
     * \since QGIS 3.0
     */
    QMap<QString, QString> getSpatialiteDbInfoErrors( bool bWarnings = false ) const;

    /**
     * Retrieve Layer of Item
     * - contains Layer-Name and a QgsSpatialiteDbLayer-Pointer
     * \note
     * - child Item will receive Layer from Parent
     * -- when  getDbLoadLayers is true, getQgsSpatialiteDbLayer will load all layers
     * \returns mDbLayer of active Layer
     *  \see onInit
     * \since QGIS 3.0
     */
    QgsSpatialiteDbLayer *getDbLayer() const { return mDbLayer; }

    /**
     * The Spatialite Layer-Type of the Layer
     * - representing mLayerType
     *  \see setGeometryType
     *  \see mLayerTypeString
     * \since QGIS 3.0
     */
    QgsSpatialiteDbInfo::SpatialiteLayerType getLayerType() const { return mLayerType; }

    /**
     * Is the Layer-Type a Geometry or Raster
     * - only with mLayerType
     *  \see setGeometryType
     *  \see addCommonMetadataItems
     *  \see mLayerTypeString
     * \since QGIS 3.0
     */
    bool mIsGeometryType = false;

    /**
     * Is the Layer-Type a RasterLite2 Layer
     * - only with mLayerType
     *  \see addCommonMetadataItems
     *  \see mLayerTypeString
     * \since QGIS 3.0
     */
    bool mIsSpatialView = false;

    /**
     * Is the Layer-Type a RasterLite2 Layer
     * - only with mLayerType
     *  \see addCommonMetadataItems
     *  \see mLayerTypeString
     * \since QGIS 3.0
     */
    bool mIsRasterLite2 = false;

    /**
     * Is the Layer-Type a RasterLite1 Layer
     * - Without title, Abstract and copywrite Information
     *  \see addCommonMetadataItems
     *  \see mLayerTypeString
     * \since QGIS 3.0
     */
    bool mIsRasterLite1 = false;

    /**
     * Is the Layer-Type a MbTiles Layer
     * - only with mLayerType
     *  \see addCommonMetadataItems
     *  \see mLayerTypeString
     * \since QGIS 3.0
     */
    bool mIsMbTiles = false;

    /**
     * Is the Layer-Type a GeoPackage
     * - only with mLayerType
     *  \see addCommonMetadataItems
     *  \see mLayerTypeString
     * \since QGIS 3.0
     */
    bool mIsGeoPackage = false;

    /**
     * Layer-Name
     * \note
     *  Vector: Layer format: 'table_name(geometry_name)'
     *  Raster: Layer format: 'coverage_name'
     * \since QGIS 3.0
     */
    QString getLayerName() const { return mLayerName; }

    /**
     * Set the Spatialite Layer-Type String
     * - representing mLayerType
     *  \see setLayerType
     * \see SpatialiteLayerTypeName
     * \since QGIS 3.0
     */
    QString getLayerTypeString() const { return mLayerTypeString; }

    /**
     * The Spatialite internal Database structure being read
     * \note
     *  -  based on result of CheckSpatialMetaData
     * \see getSniffDatabaseType
    * \since QGIS 3.0
    */
    QgsSpatialiteDbInfo::SpatialMetadata dbSpatialMetadata() const { return mSpatialMetadata; }

    /**
     * The Spatialite internal Database structure being read (as String)
     * \note
     *  -  based on result of CheckSpatialMetaData
     * \see getSniffDatabaseType
    * \since QGIS 3.0
    */
    QString dbSpatialMetadataString() const { return mSpatialMetadataString; }

    /**
     * Returns the Index of Column 'Table' from the Header
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
    * \see mColumnTable
    * \see QgsSpatiaLiteSourceSelect::on_mSearchColumnComboBox_currentIndexChanged
    * \since QGIS 3.0
    */
    int getItemType() const { return mItemType; }

    /**
     * Returns the Index of Column 'Table' from the Header
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
    * \see mColumnTable
    * \see QgsSpatiaLiteSourceSelect::on_mSearchColumnComboBox_currentIndexChanged
    * \since QGIS 3.0
    */
    int getTableNameIndex() const { return mColumnTable; }

    /**
     * Returns the Index of Column 'Geometry' from the selected Item
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
    * \see mColumnGeometryName
    * \see QgsSpatiaLiteSourceSelect::on_mSearchColumnComboBox_currentIndexChanged
    * \since QGIS 3.0
    */
    int getGeometryNameIndex() const { return mColumnGeometryName; }

    /**
     * Returns the Index of Column 'Geometry-Type' from the selected Item
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
    * \see mColumnGeometryType
    * \see QgsSpatiaLiteSourceSelect::on_mSearchColumnComboBox_currentIndexChanged
    * \since QGIS 3.0
    */
    int getGeometryTypeIndex() const { return mColumnGeometryType; }

    /**
     * Returns the Index of Column 'Sql' from the selected Item
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
    * \see getSqlQueryIndex
    * \see QgsSpatiaLiteSourceSelect::on_mSearchColumnComboBox_currentIndexChanged
    * \since QGIS 3.0
    */
    int getSqlQueryIndex() const { return mColumnSql; }

    /**
     * Returns the Index of Column 'Hidden' from the selected Item
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
    * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
    * \see mColumnSortHidden
    * \see QgsSpatiaLiteSourceSelect::on_mSearchColumnComboBox_currentIndexChanged
    * \since QGIS 3.0
    */
    int getColumnSortHidden() const { return mColumnSortHidden; }
    //-----------------------------------------------------------------
    // Emulating QStandardItem
    //-----------------------------------------------------------------

    /**
     * Sets the item's data for the given \a role to the specified \a value.
     * \note
     *  - The default implementation treats Qt::EditRole and Qt::DisplayRole as referring to the same data.
     * Based on logic used in QStandardItem
     * \see data
     * \see setFlags
     * \see mItemData
     * \param column Column to set Value
     * \param value Value to set in Column
     * \param role Role-Tpe to set in Column
     * \since QGIS 3.0
    */
    bool setData( int column, const QVariant &value, int role = Qt::EditRole );

    /**
     * Returns the item's data for the given \a role, or an invalid QVariant if there is no data for the role.
     * \note
     *  - The default implementation treats Qt::EditRole and Qt::DisplayRole as referring to the same data.
     * Based on logic used in QStandardItem
     * \see data
     * \see setFlags
     * \see mItemData
     * \param column Column to retrieve Value from [default=0]
     * \since QGIS 3.0
    */
    QVariant data( int column, int role = Qt::EditRole ) const;

    /**
     * Returns the set flag for the given Item's column
     * \note
     *  - set during construction of Item
    * \param column of the Item [default=0]
    * \returns flags for the given column of the Item
    * \see QgsSpatialiteDbInfoModel::flags
    * \since QGIS 3.0
    */
    Qt::ItemFlags flags( int column = 0 ) const;

    /**
     * Sets the item flags for the item to \a flags.
     *  - The item flags determine how the user can interact with the item.
     * \note
     *  - when column < 0: all columns will be set with the given value
     * \param column Column to set Value to
     * \param flags Value to set in Column
    * \since QGIS 3.0
    */
    void setFlags( int column, Qt::ItemFlags flags );

    /**
     * Sets the item's accessible description to the string specified by \a  accessibleDescription.
     *  - The accessible description is used by assistive technologies (i.e. for users who cannot use conventional means of interaction).
     * \note
     *  - default:  [Qt::ItemIsEnabled]
     * \param column Column to retrieve Value from [default=0]
    * \since QGIS 3.0
    */
    inline bool isEnabled( int column = 0 ) const { return ( flags( column ) & Qt::ItemIsEnabled ) != 0; }

    /**
     * Sets whether the item is enabled.
     *  - If \a enabled is true, the item is enabled, meaning that the user can interact with the item; if \a enabled is false, the user cannot interact with the item.
     * \note
     *  This flag takes precedence over the other item flags; e.g. if an item is not enabled, it cannot be selected by the user, even if the Qt::ItemIsSelectable flag has been set.
     *  - default:  [Qt::ItemIsEnabled]
     * \param column Column to set Value to [default=0]
     * \param value Value to set to True or False
    * \since QGIS 3.0
    */
    void setEnabled( int column, bool value );

    /**
     * Returns whether the item can be edited by the user.
     *  -When an item is editable (and enabled), the user can edit the item by invoking one of the view's edit triggers;
     * \note
     *  - default:  [false]
     * \param column Column to retrieve Value from [default=0]
    * \since QGIS 3.0
    */
    inline bool isEditable( int column ) const { return ( flags( column ) & Qt::ItemIsEditable ) != 0; }

    /**
     * Sets whether the Item-Column is editable.
     *  - If \a editable is true, the Item-Column can be edited by the user; otherwise, the user cannot edit the item.
     *  - since the Item-Column must be enabled, setEnabled will be called if not already enabled
     * \note
     *  - How the user can edit items in a view is determined by the view's edit triggers;
     *  - default:  [false]
     *  - mTablesTreeView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
     * \param column Column to set Value to [default=0]
     * \param value Value to set to True or False
     * \see setEnabled
     * \since QGIS 3.0
    */
    void setEditable( int column, bool value );

    /**
     * Returns whether the item is selectable by the user.
     *  -The default value is true
     * \note
     *  - default:  [Qt::ItemIsSelectable]
     * \param column Column to retrieve Value from [default=0]
    * \since QGIS 3.0
    */
    inline bool isSelectable( int column = 0 ) const { return ( flags( column ) & Qt::ItemIsSelectable ) != 0; }

    /**
     * Sets whether the item is selectable
     *  - If \a selectable is true, the item can be selected by the user; otherwise, the user cannot select the item.
     * \note
     *  -   You can control the selection behavior and mode by manipulating their view properties
     * \param column Column to set Value to [default=0]
     * \param value Value to set to True or False
     * \since QGIS 3.0
    */
    void setSelectable( int column, bool value );

    /**
     * Returns the item's icon.
     * \note
     *  - default:  [Qt::DecorationRole, QIcon]
     * \param column Column to retrieve Value from [default=0]
     * \since QGIS 3.0
    */
    inline QIcon icon( int column = 0 ) const { return qvariant_cast<QIcon>( data( column, Qt::DecorationRole ) ); }

    /**
     * Sets the item's icon to the \a icon specified.
     * \note
     *  - default:  [Qt::DecorationRole, QIcon]
     * \param column Column to set Value to [default=0]
     * \param icon Value to set in Column
     * \since QGIS 3.0
    */
    inline void setIcon( int column,  const QIcon &icon )  { setData( column, icon, Qt::DecorationRole ); }

    /**
     * Sets the item's text to the \a text specified.
     * \note
     *  - default:  [Qt::EditRole, Edit]
     *  - when column < 0: all columns will be set with the given value
     * \param column Column to set Value to [default=0]
     * \param sText Value to set in Column
     * \since QGIS 3.0
    */
    inline void setText( int column, const QString &sText, int role = Qt::EditRole );

    /**
     * Returns the item's text. This is the text that's presented to the user in a view.
     * \note
     *  - default:  [Qt::EditRole, Edit]
     * \param column Column to retrieve Value from [default=0]
     * \since QGIS 3.0
    */
    inline QString text( int column = 0, int role = Qt::EditRole ) const { return qvariant_cast<QString>( data( column, role ) ); }

    /**
     * Sets the item's text Background to the color specified.
     * \note
     *  - default:  [lightyellow, #FFFFE0, rgb(255, 255, 224)]
     * \param column Column to set Value to [default=0]
     * \param value QColor background value to set in Column
     * \since QGIS 3.0
    */
    inline void setBackground( int column = 0, QColor value = QColor( "lightyellow" ) )  { setData( column, value, Qt::BackgroundRole ); }

    /**
     * Sets the item's text Background to the color specified.
     * \note
     *  - default:  [black, #000000, rgb(0, 0, 0)]
     * \param column Column to set Value to [default=0]
     * \param value QColor foreground value to set in Column
     * \since QGIS 3.0
    */
    inline void setForeground( int column = 0, QColor value = QColor( "black" ) )  { setData( column, value, Qt::ForegroundRole ); }

    /**
     * Sets the item's text Alignment to the Alignment specified.
     * \note
     *  - default:  [Qt::AlignLeft, Browse] can be [Qt::AlignCenter,Qt::AlignRight,Qt::AlignJustify etc.]
     * \param column Column to set Value to [default=0]
     * \param sText Value to set in Column
     * \since QGIS 3.0
    */
    inline void setAlignment( int column = 0, int value = Qt::AlignLeft )  { setData( column, value, Qt::TextAlignmentRole ); }

    /**
     * Sets the item's tooltip to the string specified by \a toolTip.
     * \note
     *  - default:  [Qt::ToolTipRole]
     * \param column Column to set Value to [default=0]
     * \param sText Value to set in Column
     * \since QGIS 3.0
    */
    inline void setToolTip( int column, const QString &sText )  { setData( column, sText, Qt::ToolTipRole ); }

    /**
     * Returns the item's tooltip.
     * \note
     *  - default:  [Qt::ToolTipRole]
     * \param column Column to retrieve Value from [default=0]
     * \since QGIS 3.0
    */
    inline QString toolTip( int column = 0 ) const { return qvariant_cast<QString>( data( column, Qt::ToolTipRole ) ); }

    /**
     * Sets the item's status tip to the string specified by \a statusTip.
     * \note
     *  -  [Qt::StatusTipRole]
     * \param column Column to set Value to [default=0]
     * \param sText Value to set in Column
     * \since QGIS 3.0
    */
    inline void setStatusTip( int column, const QString &sText )  { setData( column, sText, Qt::StatusTipRole ); }

    /**
     * Returns the item's tooltip.
     * \note
     *  -  [Qt::StatusTipRole]
     * \param column Column to retrieve Value from [default=0]
     * \since QGIS 3.0
    */
    inline QString statusTip( int column = 0 ) const { return qvariant_cast<QString>( data( column, Qt::StatusTipRole ) ); }

    /**
     * Returns the row where the item is located in its parent's child table
     *  -   -1 if the item has no parent.
     * \note
     *  -  returns 'first' value from result of position() pair
     * \since QGIS 3.0
    */
    inline int row() const { return position().first; }

    /**
     *  Returns the column where the item is located in its parent's child table,
     *  -   -1 if the item has no parent.
     * \note
     *  -  returns 'second' value from result of position() pair
     * \since QGIS 3.0
    */
    inline int column() const { return position().second; }

    /**
     *  Total amount of Layers in Database
     * \note
     * - Used to show the total amount of Layers
     * - calculated only ItemTypeDb from onInit, retrieved from parent for others
    * \see sniffSpatialiteDbInfo
    * \since QGIS 3.0
    */
    int getTableCounter() { return mTableCounter; }

    /**
     *  Total amount of non-SpatialTables/Views in Database
     * \note
     *  - Used to show the total amount of Layers
     * - calculated only ItemTypeDb from onInit, retrieved from parent for others
    * \see sniffSpatialiteDbInfo
    * \since QGIS 3.0
    */
    int getNonSpatialTablesCounter() { return mNonSpatialTablesCounter; }

    /**
     *  Set Total amount of Layers in Database
     * - Used mainly to set in value in mDbRootItem
     * \note
     * - Used to show the total amount of Layers
     * - calculated only ItemTypeDb from onInit, retrieved from parent for others
    * \see sniffSpatialiteDbInfo
    * \since QGIS 3.0
    */
    void setTableCounter( int iTableCounter ) { mTableCounter = iTableCounter; }

    /**
     *  Set Total amount of non-SpatialTables/Views  in Database
     * - Used mainly to set in value in mDbRootItem
     * \note
     * - Used to show the total amount of Layers
     * - calculated only ItemTypeDb from onInit, retrieved from parent for others
    * \see sniffSpatialiteDbInfo
    * \since QGIS 3.0
    */
    void setNonSpatialTablesCounter( int iNonSpatialTablesCounter ) { mNonSpatialTablesCounter = iNonSpatialTablesCounter; }

  signals:
    // - Based on QgsLayerTreeModel / QgsLayerTreeNode
    //! Emitted when one or more nodes will be added to a node within the tree
    void willAddChildren( QgsSpatialiteDbInfoItem *node, int indexFrom, int indexTo );
    //! Emitted when one or more nodes have been added to a node within the tree
    void addedChildren( QgsSpatialiteDbInfoItem *node, int indexFrom, int indexTo );
    //! Emitted when one or more nodes will be removed from a node within the tree
    void willRemoveChildren( QgsSpatialiteDbInfoItem *node, int indexFrom, int indexTo );
    //! Emitted when one or more nodes has been removed from a node within the tree
    void removedChildren( QgsSpatialiteDbInfoItem *node, int indexFrom, int indexTo );
    //! Emitted when check state of a node within the tree has been changed
    void visibilityChanged( QgsSpatialiteDbInfoItem *node );
    //! Emitted when a custom property of a node within the tree has been changed or removed
    void customPropertyChanged( QgsSpatialiteDbInfoItem *node, const QString &key );
    //! Emitted when the collapsed/expanded state of a node within the tree has been changed
    // void expandedChanged( QgsSpatialiteDbInfoItem *node, bool expanded );

  private:

    /**
     * The Item-Type
     * - Item-Type being used
     * \since QGIS 3.0
     *
     */
    SpatialiteDbInfoItemType mItemType = SpatialiteDbInfoItemType::ItemTypeUnknown;

    /**
     * The Spatialite Layer-Type of the Layer
     * - representing mLayerType
     * \note
     *  - The Layer-Type of a set mDblayer, otherwise SpatialiteUnknown
     * \since QGIS 3.0
     */
    QgsSpatialiteDbInfo::SpatialiteLayerType mLayerType = QgsSpatialiteDbInfo::SpatialiteUnknown;

    /**
     * Set the Spatialite Layer-Type String
     * - representing mLayerType
     *  \see setLayerType
     * \see SpatialiteLayerTypeName
     * \since QGIS 3.0
     */
    QString mLayerTypeString = QString( "SpatialiteUnknown" );

    /**
     * Layer-Name
     * \note
     *  Vector: Layer format: 'table_name(geometry_name)'
     *  Raster: Layer format: 'coverage_name'
     * \since QGIS 3.0
     */
    QString mLayerName = QString();

    /**
     * The Spatialite internal Database structure being read
     * \note
     *  -  based on result of CheckSpatialMetaData
     * \see getSniffDatabaseType
    * \since QGIS 3.0
    */
    QgsSpatialiteDbInfo::SpatialMetadata mSpatialMetadata = QgsSpatialiteDbInfo::SpatialUnknown;

    /**
     * The Spatialite internal Database structure being read (as String)
     * \note
     *  -  based on result of CheckSpatialMetaData
     * \see getSniffDatabaseType
    * \since QGIS 3.0
    */
    QString mSpatialMetadataString = QString( "SpatialUnknown" );

    /**
     * Returns the Index of Column 'Hidden' from the selected Item
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
     * - must be the last, since (mColumnSortHidden+1) == columnCount
     * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
     * \see getColumnSortHidden
     * \see QgsSpatiaLiteSourceSelect::on_mSearchColumnComboBox_currentIndexChanged
     * \since QGIS 3.0
    */
    int mColumnSortHidden;

    /**
     * Returns the Index of Column 'Table' from the selected Item
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
     * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
     * \see getTableNameIndex
     * \see getTableName
     * \since QGIS 3.0
    */
    int mColumnTable;

    /**
     * Returns the Index of Column 'Geometry' from the selected Item
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
     * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
     * \see getGeometryTypeIndex
     * \see QgsSpatiaLiteSourceSelect::on_mSearchColumnComboBox_currentIndexChanged
     * \since QGIS 3.0
    */
    int mColumnGeometryType;

    /**
     * Returns the Index of Column 'Geometry' from the selected Item
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
     * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
     * \see getGeometryNameIndex
     * \see QgsSpatiaLiteSourceSelect::on_mSearchColumnComboBox_currentIndexChanged
     * \since QGIS 3.0
    */
    int mColumnGeometryName;

    /**
     * Returns the Index of Column 'Sql' from the selected Item
     * \note
     *  - set during construction. Avoid manual use of numeric value in code
     * \returns index-number of column being used in QgsSpatiaLiteSourceSelect
     * \see getSqlQueryIndex
     * \see getSqlQuery
     * \since QGIS 3.0
    */
    int mColumnSql;

    /**
     * Store the Item-Children
     * \note
     *
     * \since QGIS 3.0
    */
    QList<QgsSpatialiteDbInfoItem *> mChildItems;

    /**
     * Store the Item-Children
     * \note
     *
     * \since QGIS 3.0
    */
    QList<QgsSpatialiteDbInfoItem *> mPrepairedChildItems;

    /**
     * Store the Item-Data
     * \note
     *  - for RootItem, this will contain the Column-Names
     *  - Text [Qt::EditRole]
     *  - Icon  [Qt::DecorationRole]
     *  - Flags  [255: Qt::UserRole - 1 ]
     * The default implementation treats Qt::EditRole and Qt::DisplayRole as referring to the same data.
     * \since QGIS 3.0
    */
    QVector<QgsSpatialiteDbInfoData> mItemData;

    /**
     * Store the Item-Parent
     * \note
     *  Set during construction
     * \since QGIS 3.0
    */
    QgsSpatialiteDbInfoItem *mParentItem = nullptr;

    /**
     * QgsSpatialiteDbInfo Object
     * - containing all Information about Database file
     * \note
     * - isDbValid() return if the connection contains layers that are supported by
     * -- QgsSpatiaLiteProvider, QgsGdalProvider and QgsOgrProvider
     * \see QgsSpatialiteDbInfo::isDbValid()
     * \since QGIS 3.0
    */
    QgsSpatialiteDbInfo *mSpatialiteDbInfo = nullptr;
    QgsSpatialiteDbLayer *mDbLayer = nullptr;

    /**
     * Helper Function to implement row() and column()
     * \note
     *  - Internal function
     *  - Emulating QStandardItem
     * \since QGIS 3.0
    */
    QPair<int, int> position() const;
    int mColumnNr = -1;

    /**
     *  Total amount of Layers in Database
     * \note
     *  - Used to show the total amount of Layers
     * - calculated only ItemTypeDb from onInit, retrieved from parent for others
    * \see sniffSpatialiteDbInfo
    * \since QGIS 3.0
    */
    int mTableCounter = 0;

    /**
     *  Total amount of non-SpatialTables/Views in Database
     * \note
     *  - Used to show the total amount of Layers
     * - calculated only ItemTypeDb from onInit, retrieved from parent for others
    * \see sniffSpatialiteDbInfo
    * \since QGIS 3.0
    */
    int mNonSpatialTablesCounter = 0;

    /**
     * Sets the mTableCounter
     * \note
     * - Used to show the total amount of Layers
     * - runs only when ItemTypeDb from onInit
     * \since QGIS 3.0
    */
    bool sniffSpatialiteDbInfo();

    /**
     * Add Items to display as HelpText
     * \note
     * - called during creation of Model
     * -> removed during first Database loading
     * \since QGIS 3.0
    */
    int buildDbInfoItem();

    /**
     * Add Items to display as HelpText
     * \note
     * - called during creation of Model
     * -> removed during first Database loading
     * \since QGIS 3.0
    */
    int buildDbLayerItem();

    /**
     * Add Items to display as HelpText
     * \note
     * - called during creation of Model
     * -> removed during first Database loading
     * \since QGIS 3.0
    */
    int buildHelpItem();

    /**
     * Adding and filling NonSpatialTables Sub-Groups
     * \note
     * - called during creation of Model
     * -> removed during first Database loading
     * \param sKey table name
     * \param sValue Name of Sub-Group
     * \returns count of Items added
     * \see sniffSpatialiteDbInfo()
     * \see QgsSpatialiteDbInfo::dbNonSpatialTablesCount
     * \see createGroupLayerItem
     * \see QgsSpatialiteDbInfo::getDbLayersType
     * \see addTableItemMap
     * \since QGIS 3.0
    */
    int buildNonSpatialTablesGroups();

    /**
     * Add Items to display as HelpText
     * \note
     * - called during creation of Model
     * -> removed during first Database loading
     * \see buildNonSpatialTablesGroups
     * \since QGIS 3.0
    */
    int buildNonSpatialTablesGroup( QString sGroupInfo, QStringList saListValues );

    /**
     * Add Items to display as HelpText
     * \note
     * - called during creation of Model
     * -> removed during first Database loading
     * \since QGIS 3.0
    */
    int buildNonSpatialAdminTable( QString sTableInfo, QStringList saListValues );

    /**
     * Retrieve the  Layer-Type entries
     * \see addTableEntry
     * \since QGIS 3.0
     */
    QgsSpatialiteDbInfoItem *createGroupLayerItem( QgsSpatialiteDbInfo::SpatialiteLayerType layerType = QgsSpatialiteDbInfo::SpatialiteUnknown );

    /**
     * Add the Item based on a Spatial-Layer
     * \since QGIS 3.0
     */
    bool addTableItemLayer( QgsSpatialiteDbInfoItem *dbGroupLayerItem, QgsSpatialiteDbLayer *dbLayer );

    /**
     * Add the Item based on a non-Spatial-Layer
     * \since QGIS 3.0
     */
    bool addTableItemMap( QgsSpatialiteDbInfoItem *dbGroupLayerItem, QString sKey, QString sValue, QgsSpatialiteDbInfo::SpatialiteLayerType layerType = QgsSpatialiteDbInfo::SpatialiteUnknown );

};
#endif // QGSSPATIALITEDBINFOMODEL_H
