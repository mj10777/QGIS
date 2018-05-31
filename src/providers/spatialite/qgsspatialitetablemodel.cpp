/***************************************************************************
                         qgsspatialitetablemodel.cpp  -  description
                         -------------------
    begin                : Dec 2008
    copyright            : (C) 2008 by Sandro Furieri
    email                : a.furieri@lqt.it
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsspatialitetablemodel.h"
#include "qgsapplication.h"
#include "qgsdataitem.h" // for icons
#include "qgslogger.h"
//-----------------------------------------------------------------
// QgsSpatiaLiteTableModel::QgsSpatiaLiteTableModel
//-----------------------------------------------------------------
QgsSpatiaLiteTableModel::QgsSpatiaLiteTableModel(): QStandardItemModel(), mTableCount( 0 ), mDbRootItem( nullptr ), mLoadGeometrylessTables( true )
{
  headerLabels << tr( "Table" );
  mColumnTable = 0;
  headerLabels << tr( "Geometry column" );
  mColumnGeometryName = mColumnTable + 1;
  headerLabels << tr( "Geometry-Type" );
  mColumnGeometryType = mColumnGeometryName + 1;
  headerLabels << tr( "Sql" );
  mColumnSql = mColumnGeometryType + 1;
  headerLabels << tr( "ColumnSortHidden" );
  mColumnSortHidden = mColumnSql + 1;
  setHorizontalHeaderLabels( headerLabels );
}
//-----------------------------------------------------------------
// QgsSpatiaLiteTableModel::createDatabase
//-----------------------------------------------------------------
bool QgsSpatiaLiteTableModel::createDatabase( QString sDatabaseFileName, QgsSpatialiteDbInfo::SpatialMetadata dbCreateOption )
{
  bool bRc = false;
  bool bLoadLayers = false;
  bool bShared = false;
  //----------------------------------------------------------
  QgsSpatiaLiteConnection connectionInfo( sDatabaseFileName );
  QgsSpatialiteDbInfo *spatialiteDbInfo = connectionInfo.CreateSpatialiteConnection( QString(), bLoadLayers, bShared, dbCreateOption );
  if ( spatialiteDbInfo )
  {
    if ( spatialiteDbInfo->isDbSqlite3() )
    {
      switch ( dbCreateOption )
      {
        case QgsSpatialiteDbInfo::Spatialite40:
        case QgsSpatialiteDbInfo::Spatialite50:
          switch ( spatialiteDbInfo->dbSpatialMetadata() )
          {
            case QgsSpatialiteDbInfo::SpatialiteLegacy:
            case QgsSpatialiteDbInfo::Spatialite40:
            case QgsSpatialiteDbInfo::Spatialite50:
              // this is a Database that can be used for QgsSpatiaLiteProvider
              bRc = true;
              break;
            default:
              break;
          }
          break;
        case QgsSpatialiteDbInfo::SpatialiteGpkg:
          switch ( spatialiteDbInfo->dbSpatialMetadata() )
          {
            case QgsSpatialiteDbInfo::SpatialiteGpkg:
              // this is a Database that can be used for QgsOgrProvider or QgsGdalProvider
              bRc = true;
              break;
            default:
              break;
          }
          break;
        case QgsSpatialiteDbInfo::SpatialiteMBTiles:
          switch ( spatialiteDbInfo->dbSpatialMetadata() )
          {
            case QgsSpatialiteDbInfo::SpatialiteMBTiles:
              // this is a Database that can be used for QgsGdalProvider
              bRc = true;
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }
    }
    if ( bRc )
    {
      setSpatialiteDbInfo( spatialiteDbInfo, true );
    }
    else
    {
      delete spatialiteDbInfo;
      spatialiteDbInfo = nullptr;
    }
  }
  //----------------------------------------------------------
  return bRc;
}
//-----------------------------------------------------------------
// QgsSpatiaLiteTableModel::setSpatialiteDbInfo
//-----------------------------------------------------------------
void QgsSpatiaLiteTableModel::setSpatialiteDbInfo( QgsSpatialiteDbInfo *spatialiteDbInfo, bool loadGeometrylessTables )
{
  if ( ( spatialiteDbInfo ) && ( spatialiteDbInfo->isDbValid() ) )
  {
    if ( mSpatialiteDbInfo )
    {
      if ( mSpatialiteDbInfo->getQSqliteHandle() )
      {
        // TODO QgsSqliteHandle::closeDb( mSpatialiteDbInfo->getQSqliteHandle() );
        mSpatialiteDbInfo = nullptr;
      }
      else
      {
        delete spatialiteDbInfo;
        mSpatialiteDbInfo = nullptr;
      }
      mTableCount = 0;
      if ( mDbRootItem )
      {
        QModelIndex rootItemIndex = indexFromItem( invisibleRootItem() );
        removeRows( 0, rowCount( rootItemIndex ), rootItemIndex );
        mDbRootItem = nullptr;
      }
    }
    mSpatialiteDbInfo = spatialiteDbInfo;
    mDbLayersDataSourceUris = getDataSourceUris();
    mTableCounter = 0;
    mLoadGeometrylessTables = loadGeometrylessTables;
    addTableEntryTypes();
  }
}
//-----------------------------------------------------------------
// QgsSpatiaLiteTableModel::createLayerTypeEntry
//-----------------------------------------------------------------
QList < QStandardItem * > QgsSpatiaLiteTableModel::createLayerTypeEntry( QgsSpatialiteDbInfo::SpatialiteLayerType layerType, int amountLayers )
{
  QString sTypeName = QgsSpatialiteDbInfo::SpatialiteLayerTypeName( layerType );
  if ( sTypeName == "MBTilesTable" )
  {
    sTypeName = "MBTiles";
  }
  QIcon iconType = QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( layerType );
  int i_typeCount = amountLayers;
  QString sLayerText = QString( "%1 Layers" ).arg( i_typeCount );
  if ( i_typeCount == 1 )
  {
    sLayerText = QString( "%1 Layer" ).arg( i_typeCount );
  }
  QString sSortTag = QString();
  switch ( layerType )
  {
    case QgsSpatialiteDbInfo::SpatialTable:
      sSortTag = QString( "AAAA_%1" ).arg( sTypeName );
      break;
    case QgsSpatialiteDbInfo::SpatialView:
      sSortTag = QString( "AAAB_%1" ).arg( sTypeName );
      break;
    case QgsSpatialiteDbInfo::VirtualShape:
      sSortTag = QString( "AAAC_%1" ).arg( sTypeName );
      break;
    case QgsSpatialiteDbInfo::RasterLite1:
      sSortTag = QString( "AAAD_%1" ).arg( sTypeName );
      break;
    case QgsSpatialiteDbInfo::RasterLite2Raster:
      sSortTag = QString( "AAAE_%1" ).arg( sTypeName );
      break;
    case QgsSpatialiteDbInfo::SpatialiteTopology:
      sSortTag = QString( "AAAF_%1" ).arg( sTypeName );
      break;
    // Non-spatialite-Types
    case QgsSpatialiteDbInfo::MBTilesTable:
      sSortTag = QString( "AABA%1" ).arg( sTypeName );
      break;
    case QgsSpatialiteDbInfo::GeoPackageVector:
      sSortTag = QString( "AABB_%1" ).arg( sTypeName );
      break;
    case QgsSpatialiteDbInfo::GdalFdoOgr:
      sSortTag = QString( "AABB_%1" ).arg( sTypeName );
      break;
    case QgsSpatialiteDbInfo::NonSpatialTables:
      // when shown at end of list
      sSortTag = QString( "AAYA_%1" ).arg( sTypeName );
      sLayerText = QString( "%1 Tables/Views" ).arg( i_typeCount );
      if ( i_typeCount == 1 )
      {
        sLayerText = QString( "%1 Tables/Views" ).arg( i_typeCount );
      }
    default:
      sSortTag = QString( "AAZZ_%1" ).arg( sTypeName ); // Unknown at end
      break;
  }
  QStandardItem *tbTypeItem = new QStandardItem( iconType, sTypeName );
  tbTypeItem->setFlags( Qt::ItemIsEnabled );
  QStandardItem *tbLayersItem = new QStandardItem( sLayerText );
  tbLayersItem->setFlags( Qt::ItemIsEnabled );
  //  Ever Columns must be filled (even if empty)
  QStandardItem *emptyItem_01 = new QStandardItem();
  emptyItem_01->setFlags( Qt::ItemIsEnabled );
  QStandardItem *emptyItem_02 = new QStandardItem();
  emptyItem_02->setFlags( Qt::ItemIsEnabled );
  QStandardItem *sortHiddenItem = new QStandardItem( sSortTag );
  sortHiddenItem->setFlags( Qt::NoItemFlags );
  // this must be done in the same order as the Header
  QList < QStandardItem *> tbItemList;
  tbItemList.push_back( tbTypeItem );
  tbItemList.push_back( emptyItem_01 );
  tbItemList.push_back( emptyItem_02 );
  tbItemList.push_back( tbLayersItem );
  tbItemList.push_back( sortHiddenItem );
  return tbItemList;
}
//-----------------------------------------------------------------
// QgsSpatiaLiteTableModel::addRootEntry
//-----------------------------------------------------------------
void QgsSpatiaLiteTableModel::addRootEntry()
{
  if ( ( mSpatialiteDbInfo ) && ( mSpatialiteDbInfo->isDbValid() ) )
  {
    mTableCounter = 0;
    mTableCounter += mSpatialiteDbInfo->dbSpatialTablesLayersCount();
    mTableCounter += mSpatialiteDbInfo->dbSpatialViewsLayersCount();
    mTableCounter += mSpatialiteDbInfo->dbVirtualShapesLayersCount();
    // these values will be -1 if no admin-tables were found
    if ( mSpatialiteDbInfo->dbRasterCoveragesLayersCount() > 0 )
    {
      mTableCounter += mSpatialiteDbInfo->dbRasterCoveragesLayersCount();
    }
    if ( mSpatialiteDbInfo->dbRasterLite1LayersCount() > 0 )
    {
      mTableCounter += mSpatialiteDbInfo->dbRasterLite1LayersCount();
    }
    if ( mSpatialiteDbInfo->dbMBTilesLayersCount() > 0 )
    {
      mTableCounter += mSpatialiteDbInfo->dbMBTilesLayersCount();
    }
    if ( mSpatialiteDbInfo->dbGeoPackageLayersCount() > 0 )
    {
      mTableCounter += mSpatialiteDbInfo->dbGeoPackageLayersCount();
    }
    if ( mSpatialiteDbInfo->dbFdoOgrLayersCount() > 0 )
    {
      mTableCounter += mSpatialiteDbInfo->dbFdoOgrLayersCount();
    }
    if ( mSpatialiteDbInfo->dbTopologyExportLayersCount() > 0 )
    {
      mTableCounter += mSpatialiteDbInfo->dbTopologyExportLayersCount();
    }
    QList < QStandardItem * >dbItems;
    //is there already a root item ?
    QStandardItem *dbItem = nullptr;
    // Only searches top level items
    dbItems = findItems( mSpatialiteDbInfo->getFileName(), Qt::MatchExactly, 0 );
    //there is already an item
    if ( !dbItems.isEmpty() )
    {
      dbItem = dbItems.at( 0 );
    }
    else  //create a new toplevel item
    {
      QStandardItem *dbTypeItem = new QStandardItem( mSpatialiteDbInfo->getSpatialMetadataIcon(), mSpatialiteDbInfo->dbSpatialMetadataString() );
      dbTypeItem->setFlags( Qt::ItemIsEnabled );
      QString sLayerText = QString( "%1 Layers" ).arg( mTableCounter );
      if ( mTableCounter == 1 )
      {
        sLayerText = QString( "%1 Layer" ).arg( mTableCounter );
      }
      QStandardItem *layersItem = new QStandardItem( sLayerText );
      layersItem->setFlags( Qt::ItemIsEnabled );
      QString sProvider = QStringLiteral( "QgsSpatiaLiteProvider" );
      QString sInfoText = "";
      switch ( mSpatialiteDbInfo->dbSpatialMetadata() )
      {
        case QgsSpatialiteDbInfo::SpatialiteFdoOgr:
          sProvider = QStringLiteral( "QgsOgrProvider" );
          sInfoText = "The 'SQLite' Driver is NOT active and cannot be displayed";
          if ( mSpatialiteDbInfo->hasDbFdoOgrDriver() )
          {
            sInfoText = "The 'SQLite' Driver is active and can be displayed";
          }
          break;
        case QgsSpatialiteDbInfo::SpatialiteGpkg:
          sProvider = QStringLiteral( "QgsOgrProvider" );
          sInfoText = "The 'GPKG' Driver is NOT active and cannot be displayed";
          if ( mSpatialiteDbInfo->hasDbGdalGeoPackageDriver() )
          {
            sInfoText = "The 'GPKG' Driver is active and can be displayed";
          }
          break;
        case QgsSpatialiteDbInfo::SpatialiteMBTiles:
          sProvider = QStringLiteral( "QgsGdalProvider" );
          sInfoText = "The 'MBTiles' Driver is NOT active and cannot be displayed";
          if ( mSpatialiteDbInfo->hasDbGdalMBTilesDriver() )
          {
            sInfoText = "The 'MBTiles' Driver is active and can be displayed";
          }
          break;
        case QgsSpatialiteDbInfo::SpatialiteLegacy:
          if ( mSpatialiteDbInfo->dbRasterLite1LayersCount() > 0 )
          {
            sProvider = QStringLiteral( "QgsGdalProvider" );
            sInfoText = "The 'RASTERLITE' Driver is NOT active and cannot be displayed";
            if ( mSpatialiteDbInfo->hasDbGdalRasterLite1Driver() )
            {
              sInfoText = "The 'RASTERLITE' Driver is active and can be displayed";
            }
          }
          break;
        default:
          break;
      }
      QStandardItem *providerItem = new QStandardItem( sProvider );
      providerItem->setFlags( Qt::ItemIsEnabled );
      QStandardItem *textInfoItem = new QStandardItem( sInfoText );
      textInfoItem->setFlags( Qt::ItemIsEnabled );
      QStandardItem *sortHiddenItem = new QStandardItem( "ZZBB_Metadata" );
      sortHiddenItem->setFlags( Qt::NoItemFlags );
      QList < QStandardItem * >rootItemList;
      rootItemList.push_back( dbTypeItem );
      rootItemList.push_back( layersItem );
      rootItemList.push_back( providerItem );
      rootItemList.push_back( textInfoItem );
      rootItemList.push_back( sortHiddenItem );
      // this must be done in the same order as the Header
      dbItem = new QStandardItem( mSpatialiteDbInfo->getSpatialMetadataIcon(), mSpatialiteDbInfo->getFileName() );
      dbItem->setFlags( Qt::ItemIsEnabled );
      if ( invisibleRootItem()->hasChildren() )
      {
        invisibleRootItem()->removeRows( 0, invisibleRootItem()->rowCount() );
      }
      invisibleRootItem()->setChild( invisibleRootItem()->rowCount(), dbItem );
      dbItems = findItems( mSpatialiteDbInfo->getFileName(), Qt::MatchExactly, 0 );
      //there is already an item
      if ( !dbItems.isEmpty() )
      {
        mDbRootItem = dbItems.at( 0 );
        QStandardItem *dbMetaDataItem = nullptr;
        QIcon iconType = mSpatialiteDbInfo->getSpatialMetadataIcon();
        QStandardItem *tbTypeItem = new QStandardItem( iconType, "Metadata" );
        tbTypeItem->setFlags( Qt::ItemIsEnabled );
        QStandardItem *emptyItem_01 = new QStandardItem();
        emptyItem_01->setFlags( Qt::ItemIsEnabled );
        //  Every Column must be filled (even if empty)
        QStandardItem *emptyItem_02 = new QStandardItem();
        emptyItem_02->setFlags( Qt::ItemIsEnabled );
        QStandardItem *emptyItem_03 = new QStandardItem();
        emptyItem_03->setFlags( Qt::ItemIsEnabled );
        QStandardItem *sortHiddenItem = new QStandardItem( "ZZBB_Metadata" );
        sortHiddenItem->setFlags( Qt::NoItemFlags );
        // this must be done in the same order as the Header
        QList < QStandardItem *> tbItemList;
        tbItemList.push_back( tbTypeItem );
        tbItemList.push_back( emptyItem_01 );
        tbItemList.push_back( emptyItem_02 );
        tbItemList.push_back( emptyItem_03 );
        tbItemList.push_back( sortHiddenItem );
        mDbRootItem->appendRow( tbItemList );
        dbItems = findItems( "Metadata", Qt::MatchExactly | Qt::MatchRecursive, 0 );
        if ( !dbItems.isEmpty() )
        {
          dbMetaDataItem = dbItems.at( 0 );
        }
        dbMetaDataItem->appendRow( rootItemList );
        if ( mSpatialiteDbInfo->dbSpatialTablesLayersCount() > 0 )
        {
          QList < QStandardItem *> tbItemList = createLayerTypeEntry( QgsSpatialiteDbInfo::SpatialTable, mSpatialiteDbInfo->dbSpatialTablesLayersCount() );
          mDbRootItem->appendRow( tbItemList );
        }
        if ( mSpatialiteDbInfo->dbSpatialViewsLayersCount() > 0 )
        {
          QList < QStandardItem *> tbItemList = createLayerTypeEntry( QgsSpatialiteDbInfo::SpatialView, mSpatialiteDbInfo->dbSpatialViewsLayersCount() );
          mDbRootItem->appendRow( tbItemList );
          if ( mSpatialiteDbInfo->getMapGroupNames().size() > 0 )
          {
            QStandardItem *dbSpatialViewItem = nullptr;
            dbItems = findItems( "SpatialView", Qt::MatchExactly | Qt::MatchRecursive, 0 );
            if ( !dbItems.isEmpty() )
            {
              dbSpatialViewItem = dbItems.at( 0 );
              QIcon groupIconType = QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( QgsSpatialiteDbInfo::StyleVector );
              QMap<QString, int> mapGroupNames = mSpatialiteDbInfo->getMapGroupNames();
              for ( QMap<QString, int>::iterator itLayers = mapGroupNames.begin(); itLayers != mapGroupNames.end(); ++itLayers )
              {
                QStandardItem *tbTypeItem = new QStandardItem( groupIconType, itLayers.key() );
                tbTypeItem->setFlags( Qt::ItemIsEnabled );
                QStandardItem *sortHiddenItem = new QStandardItem( QString( "AZZZ_%1" ).arg( itLayers.key() ) );
                sortHiddenItem->setFlags( Qt::ItemIsEnabled );
                QStandardItem *emptyItem_01 = new QStandardItem();
                emptyItem_01->setFlags( Qt::ItemIsEnabled );
                QStandardItem *emptyItem_02 = new QStandardItem();
                emptyItem_02->setFlags( Qt::ItemIsEnabled );
                QStandardItem *emptyItem_03 = new QStandardItem( QString( "%1 Layers" ).arg( itLayers.value() ) );
                emptyItem_03->setFlags( Qt::ItemIsEnabled );
                QList < QStandardItem *> tbGroupItemList;
                tbGroupItemList.push_back( tbTypeItem );
                tbGroupItemList.push_back( emptyItem_01 );
                tbGroupItemList.push_back( emptyItem_02 );
                tbGroupItemList.push_back( emptyItem_03 );
                tbGroupItemList.push_back( sortHiddenItem );
                dbSpatialViewItem->appendRow( tbGroupItemList );
              }
            }
          }
        }
        if ( mSpatialiteDbInfo->dbVirtualShapesLayersCount() > 0 )
        {
          QList < QStandardItem *> tbItemList = createLayerTypeEntry( QgsSpatialiteDbInfo::VirtualShape, mSpatialiteDbInfo->dbSpatialViewsLayersCount() );
          mDbRootItem->appendRow( tbItemList );
        }
        if ( mSpatialiteDbInfo->dbRasterCoveragesLayersCount() > 0 )
        {
          QList < QStandardItem *> tbItemList = createLayerTypeEntry( QgsSpatialiteDbInfo::RasterLite2Raster, mSpatialiteDbInfo->dbRasterCoveragesLayersCount() );
          mDbRootItem->appendRow( tbItemList );
        }
        if ( mSpatialiteDbInfo->dbRasterLite1LayersCount() > 0 )
        {
          QList < QStandardItem *> tbItemList = createLayerTypeEntry( QgsSpatialiteDbInfo::RasterLite1, mSpatialiteDbInfo->dbRasterLite1LayersCount() );
          mDbRootItem->appendRow( tbItemList );
        }
        if ( mSpatialiteDbInfo->dbMBTilesLayersCount() > 0 )
        {
          QList < QStandardItem *> tbItemList = createLayerTypeEntry( QgsSpatialiteDbInfo::MBTilesTable, mSpatialiteDbInfo->dbMBTilesLayersCount() );
          mDbRootItem->appendRow( tbItemList );
        }
        if ( mSpatialiteDbInfo->dbGeoPackageVectorsCount() > 0 )
        {
          QList < QStandardItem *> tbItemList = createLayerTypeEntry( QgsSpatialiteDbInfo::GeoPackageVector, mSpatialiteDbInfo->dbGeoPackageVectorsCount() );
          mDbRootItem->appendRow( tbItemList );
        }
        if ( mSpatialiteDbInfo->dbGeoPackageRastersCount() > 0 )
        {
          QList < QStandardItem *> tbItemList = createLayerTypeEntry( QgsSpatialiteDbInfo::GeoPackageRaster, mSpatialiteDbInfo->dbGeoPackageRastersCount() );
          mDbRootItem->appendRow( tbItemList );
        }
        if ( mSpatialiteDbInfo->dbTopologyExportLayersCount() > 0 )
        {
          QList < QStandardItem *> tbItemList = createLayerTypeEntry( QgsSpatialiteDbInfo::SpatialiteTopology, mSpatialiteDbInfo->dbTopologyExportLayersCount() );
          mDbRootItem->appendRow( tbItemList );
        }
        if ( mSpatialiteDbInfo->dbFdoOgrLayersCount() > 0 )
        {
          QList < QStandardItem *> tbItemList = createLayerTypeEntry( QgsSpatialiteDbInfo::GdalFdoOgr, mSpatialiteDbInfo->dbFdoOgrLayersCount() );
          mDbRootItem->appendRow( tbItemList );
        }
        if ( mSpatialiteDbInfo->dbNonSpatialTablesCount() > 0 )
        {
          QList < QStandardItem *> tbItemList = createLayerTypeEntry( QgsSpatialiteDbInfo::NonSpatialTables, mSpatialiteDbInfo->dbNonSpatialTablesCount() );
          mDbRootItem->appendRow( tbItemList );
        }
      }
    }
  }
}
//-----------------------------------------------------------------
// QgsSpatiaLiteTableModel::addTableEntryTypes
//-----------------------------------------------------------------
void QgsSpatiaLiteTableModel::addTableEntryTypes()
{
  addRootEntry();
  QMap<QString, QString> mapLayers;
  if ( mSpatialiteDbInfo->dbVectorLayersCount() > 0 )
  {
    // - SpatialTables, SpatialViews and VirtualShapes [from the vector_layers View]
    // -- contains: Layer-Name as 'table_name(geometry_name)', Geometry-Type (with dimension) and Srid
    if ( mSpatialiteDbInfo->dbSpatialTablesLayersCount() > 0 )
    {
      addTableEntryType( mSpatialiteDbInfo->getDbLayersType( QgsSpatialiteDbInfo::SpatialTable ), QgsSpatiaLiteTableModel::EntryTypeLayer, QgsSpatialiteDbInfo::SpatialTable );
    }
    if ( mSpatialiteDbInfo->dbSpatialViewsLayersCount() > 0 )
    {
      addTableEntryType( mSpatialiteDbInfo->getDbLayersType( QgsSpatialiteDbInfo::SpatialView ), QgsSpatiaLiteTableModel::EntryTypeLayer, QgsSpatialiteDbInfo::SpatialView );
    }
    if ( mSpatialiteDbInfo->dbVirtualShapesLayersCount() > 0 )
    {
      addTableEntryType( mSpatialiteDbInfo->getDbLayersType( QgsSpatialiteDbInfo::VirtualShape ), QgsSpatiaLiteTableModel::EntryTypeLayer, QgsSpatialiteDbInfo::VirtualShape );
    }
  }
  if ( mSpatialiteDbInfo->dbRasterCoveragesLayersCount() > 0 )
  {
    addTableEntryType( mSpatialiteDbInfo->getDbRasterCoveragesLayers(), QgsSpatiaLiteTableModel::EntryTypeLayer, QgsSpatialiteDbInfo::RasterLite2Raster );
  }
  if ( mSpatialiteDbInfo->dbTopologyExportLayersCount() > 0 )
  {
    addTableEntryType( mSpatialiteDbInfo->getDbTopologyExportLayers(), QgsSpatiaLiteTableModel::EntryTypeLayer, QgsSpatialiteDbInfo::TopologyExport );
  }
  if ( mSpatialiteDbInfo->dbRasterLite1LayersCount() > 0 )
  {
    addTableEntryType( mSpatialiteDbInfo->getDbRasterLite1Layers(), QgsSpatiaLiteTableModel::EntryTypeLayer, QgsSpatialiteDbInfo::RasterLite1 );
  }
  if ( mSpatialiteDbInfo->dbMBTilesLayersCount() > 0 )
  {
    addTableEntryType( mSpatialiteDbInfo->getDbMBTilesLayers(), QgsSpatiaLiteTableModel::EntryTypeLayer, QgsSpatialiteDbInfo::MBTilesTable );
  }
  if ( mSpatialiteDbInfo->dbGeoPackageLayersCount() > 0 )
  {
    addTableEntryType( mSpatialiteDbInfo->getDbGeoPackageLayers(), QgsSpatiaLiteTableModel::EntryTypeLayer, QgsSpatialiteDbInfo::GeoPackageVector );
  }
  if ( mSpatialiteDbInfo->dbFdoOgrLayersCount() > 0 )
  {
    addTableEntryType( mSpatialiteDbInfo->getDbFdoOgrLayers(), QgsSpatiaLiteTableModel::EntryTypeLayer, QgsSpatialiteDbInfo::GdalFdoOgr );
  }
  if ( ( mLoadGeometrylessTables ) && ( mSpatialiteDbInfo->dbNonSpatialTablesCount() > 0 ) )
  {
    addTableEntryType( mSpatialiteDbInfo->getDbNonSpatialTables(), QgsSpatiaLiteTableModel::EntryTypeMap, QgsSpatialiteDbInfo::NonSpatialTables );
#if 0
// TODO: resolve changed QMap-Type
    if ( mSpatialiteDbInfo->dbVectorStylesCount() > 0 )
    {
      addTableEntryType( mSpatialiteDbInfo->getDbVectorStylesInfo(), QgsSpatiaLiteTableModel::EntryTypeMap, QgsSpatialiteDbInfo::StyleVector );
    }
    if ( mSpatialiteDbInfo->dbRasterStylesCount() > 0 )
    {
      addTableEntryType( mSpatialiteDbInfo->getDbRasterStylesInfo(), QgsSpatiaLiteTableModel::EntryTypeMap, QgsSpatialiteDbInfo::StyleRaster );
    }
#endif
  }
}
//-----------------------------------------------------------------
// QgsSpatiaLiteTableModel::addTableEntryType
//-----------------------------------------------------------------
void QgsSpatiaLiteTableModel::addTableEntryType( QMap<QString, QString> mapLayers, QgsSpatiaLiteTableModel::EntryType entryType, QgsSpatialiteDbInfo::SpatialiteLayerType layerType )
{
  QString sGroupLayerType = QgsSpatialiteDbInfo::SpatialiteLayerTypeName( layerType );
  bool bLoadLayer = true;
  for ( QMap<QString, QString>::iterator itLayers = mapLayers.begin(); itLayers != mapLayers.end(); ++itLayers )
  {
    if ( !itLayers.key().isEmpty() )
    {
      switch ( entryType )
      {
        case QgsSpatiaLiteTableModel::EntryTypeMap:
        {
          addTableEntryMap( itLayers.key(), itLayers.value(), layerType );
        }
        break;
        case QgsSpatiaLiteTableModel::EntryTypeLayer:
        default:
        {
          QgsSpatialiteDbLayer *dbLayer = mSpatialiteDbInfo->getQgsSpatialiteDbLayer( itLayers.key(), bLoadLayer );
          if ( ( dbLayer ) && ( dbLayer->isLayerValid() ) )
          {
            addTableEntryLayer( dbLayer, mapLayers.count() );
          }
        }
        break;
      }
    }
  }
}
//-----------------------------------------------------------------
// QgsSpatiaLiteTableModel::addTableEntryMap
//-----------------------------------------------------------------
void QgsSpatiaLiteTableModel::addTableEntryMap( QString sKey, QString sValue, QgsSpatialiteDbInfo::SpatialiteLayerType layerType )
{
  QString sGroupLayerType = QgsSpatialiteDbInfo::SpatialiteLayerTypeName( layerType );
  switch ( layerType )
  {
    case QgsSpatialiteDbInfo::StyleVector:
    case QgsSpatialiteDbInfo::StyleRaster:
    {
      QString sSeSldStylesGroup = "Se/Sld-Styles";
      QString sGroupNonSpatialTables = QgsSpatialiteDbInfo::SpatialiteLayerTypeName( QgsSpatialiteDbInfo::NonSpatialTables );
      QList < QStandardItem * >dbItems = findItems( sGroupNonSpatialTables, Qt::MatchExactly | Qt::MatchRecursive, 0 );
      if ( !dbItems.isEmpty() )
      {
        mNonSpatialItem = dbItems.at( 0 );
      }
      else
      {
        return;
      }
      // Key[fromosm_tiles] Value[GeoPackageRaster;3857]
      QStringList sa_style_info = sValue.split( QgsSpatialiteDbInfo::ParseSeparatorCoverage );
      QString sStyleName; // = sKey;
      QString sStyleType;
      QString sStyleTitle;
      QString sStyleAbstract;
      QString sStyleValid;
      if ( sa_style_info.size() == 4 )
      {
        sStyleName = sa_style_info.at( 0 );
        sStyleTitle = sa_style_info.at( 1 );
        sStyleAbstract = sa_style_info.at( 2 );
        sStyleValid = sa_style_info.at( 3 );
      }
      else
      {
        return;
      }
      QString sSortTag;
      QStandardItem *styleSeSdlTypeItem = nullptr;
      QIcon iconType = QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( layerType );
      dbItems = findItems( sSeSldStylesGroup, Qt::MatchExactly | Qt::MatchRecursive, 0 );
      if ( !dbItems.isEmpty() )
      {
        styleSeSdlTypeItem = dbItems.at( 0 );
      }
      else
      {
        sSortTag = QString( "ZZCAA_%1" ).arg( sSeSldStylesGroup );
        QIcon groupIconType = QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( QgsSpatialiteDbInfo::StyleVector );
        QStandardItem *tbTypeItem = new QStandardItem( groupIconType, sSeSldStylesGroup );
        tbTypeItem->setFlags( Qt::ItemIsEnabled );
        QStandardItem *emptyItem_01 = new QStandardItem();
        emptyItem_01->setFlags( Qt::ItemIsEnabled );
        //  Ever Columns must be filled (even if empty)
        QStandardItem *emptyItem_02 = new QStandardItem();
        emptyItem_02->setFlags( Qt::ItemIsEnabled );
        QStandardItem *emptyItem_03 = new QStandardItem();
        emptyItem_03->setFlags( Qt::ItemIsEnabled );
        QStandardItem *sortHiddenItem = new QStandardItem( sSortTag );
        sortHiddenItem->setFlags( Qt::NoItemFlags );
        // this must be done in the same order as the Header
        QList < QStandardItem *> tbItemList;
        tbItemList.push_back( tbTypeItem );
        tbItemList.push_back( emptyItem_01 );
        tbItemList.push_back( emptyItem_02 );
        tbItemList.push_back( emptyItem_03 );
        tbItemList.push_back( sortHiddenItem );
        mNonSpatialItem->appendRow( tbItemList );
        dbItems = findItems( sSeSldStylesGroup, Qt::MatchExactly | Qt::MatchRecursive, 0 );
        if ( !dbItems.isEmpty() )
        {
          styleSeSdlTypeItem = dbItems.at( 0 );
        }
        else
        {
          return;
        }
      }
      QStandardItem *styleTypeItem = nullptr;
      iconType = QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( QgsSpatialiteDbInfo::SpatialiteLayerTypeFromName( sStyleType ) );
      dbItems = findItems( sStyleType, Qt::MatchExactly | Qt::MatchRecursive, 0 );
      if ( !dbItems.isEmpty() )
      {
        styleTypeItem = dbItems.at( 0 );
      }
      else
      {
        sSortTag = QString( "ZZCBA_%1" ).arg( sStyleType );
        QStandardItem *tbTypeItem = new QStandardItem( iconType, sStyleType );
        tbTypeItem->setFlags( Qt::ItemIsEnabled );
        QStandardItem *emptyItem_01 = new QStandardItem();
        emptyItem_01->setFlags( Qt::ItemIsEnabled );
        //  Ever Columns must be filled (even if empty)
        QStandardItem *emptyItem_02 = new QStandardItem();
        emptyItem_02->setFlags( Qt::ItemIsEnabled );
        QStandardItem *emptyItem_03 = new QStandardItem();
        emptyItem_03->setFlags( Qt::ItemIsEnabled );
        QStandardItem *sortHiddenItem = new QStandardItem( sSortTag );
        sortHiddenItem->setFlags( Qt::NoItemFlags );
        // this must be done in the same order as the Header
        QList < QStandardItem *> tbItemList;
        tbItemList.push_back( tbTypeItem );
        tbItemList.push_back( emptyItem_01 );
        tbItemList.push_back( emptyItem_02 );
        tbItemList.push_back( emptyItem_03 );
        tbItemList.push_back( sortHiddenItem );
        styleSeSdlTypeItem->appendRow( tbItemList );
        dbItems = findItems( sStyleType, Qt::MatchExactly | Qt::MatchRecursive, 0 );
        if ( !dbItems.isEmpty() )
        {
          styleTypeItem = dbItems.at( 0 );
        }
        else
        {
          return;
        }
      }
      sSortTag = QString( "AZZZ_%1" ).arg( sStyleName );
      QList < QStandardItem * >childItemList;
      QStandardItem *styleNameItem = new QStandardItem( iconType, sStyleName );
      styleNameItem->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
      QStandardItem *styleTitleItem = new QStandardItem( sStyleTitle );
      styleTitleItem->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
      QStandardItem *styleAbstractItem = new QStandardItem( sStyleAbstract );
      styleAbstractItem->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
      QStandardItem *emptyItem_03 = new QStandardItem();
      emptyItem_03->setFlags( Qt::ItemIsEnabled );
      QStandardItem *sortHiddenItem = new QStandardItem( sSortTag );
      sortHiddenItem->setFlags( Qt::NoItemFlags );
      // this must be done in the same order as the Header
      childItemList.push_back( styleNameItem );
      childItemList.push_back( styleTitleItem );
      childItemList.push_back( styleAbstractItem );
      childItemList.push_back( emptyItem_03 );
      childItemList.push_back( sortHiddenItem );
      styleTypeItem->appendRow( childItemList );
    }
    break;
    case QgsSpatialiteDbInfo::NonSpatialTables:
    default:
    {
      QList < QStandardItem * >dbItems = findItems( sGroupLayerType, Qt::MatchExactly | Qt::MatchRecursive, 0 );
      if ( !dbItems.isEmpty() )
      {
        mNonSpatialItem = dbItems.at( 0 );
      }
      else
      {
        return;
      }
      QStandardItem *nonSpatialGroupItem = nullptr;
      QString sGroup = sValue;
      QString sSubGroup = sValue;
      QStringList sa_list_type = sValue.split( "-" );
      if ( sa_list_type.size() == 2 )
      {
        sGroup = sa_list_type[0];
        sSubGroup = sa_list_type[1];
      }
      QString sSortTag = QString( "ZZCA_%1" ).arg( sGroup );
      if ( ( sGroup.startsWith( "RasterLite1" ) ) || ( sGroup.startsWith( "RasterLite2" ) ) )
      {
        // Avoid using the same names as used for the Group of Layer-Names [RasterLite1 has no Internals ]
        if ( sValue != QString( "%1-Internal" ).arg( "RasterLite2" ) )
        {
          sGroup = QString( "%1-Metatdata" ).arg( sGroup );
          sSortTag = QString( "ZZBA_%1" ).arg( sGroup );
        }
      }
      if ( ( sGroup.startsWith( "Table" ) ) || ( sGroup.startsWith( "View" ) ) )
      {
        // Normal Data-Tables and Views should come first
        sGroup = QString( "%1-Data" ).arg( sGroup );
        sSortTag = QString( "ZZBA_%1" ).arg( sGroup );
      }
      if ( sValue.startsWith( "RasterLite2-I" ) )
      {
        // Spatial-Admin-Tables and Views should come first
        sGroup = QString( "%1-Admin" ).arg( sGroup );
        sSortTag = QString( "ZZBB_%1" ).arg( sGroup );
      }
      if ( sGroup.startsWith( "MBTiles" ) )
      {
        sGroup = QString( "%1-Admin" ).arg( sGroup );
        sSortTag = QString( "ZZBB_%1" ).arg( sGroup );
      }
      if ( sGroup.startsWith( "Topology" ) )
      {
        QString sTopologyAdmin = "Topology-Admin";
        if ( sValue == QString( "%1-Internal" ).arg( "Topology" ) )
        {
          sGroup = sTopologyAdmin;
          sSortTag = QString( "ZZBCA_%1" ).arg( sGroup );
        }
        else
        {
          dbItems = findItems( sTopologyAdmin, Qt::MatchExactly | Qt::MatchRecursive, 0 );
          if ( !dbItems.isEmpty() )
          {
            nonSpatialGroupItem = dbItems.at( 0 );
          }
          if ( sValue.startsWith( "Topology-View-" ) )
          {
            sGroup = QString( "Topology-Views" );
            sSortTag = QString( "ZZBCZ_%1" ).arg( sGroup );
            sSubGroup.remove( "Topology-View-" );
          }
          else
          {
            if ( sValue.startsWith( "Topology-Export" ) )
            {
              sGroup = QString( "Topology-Export-Data" );
              sSortTag = QString( "ZZBCA_%1" ).arg( sGroup );
              sSubGroup.remove( "Topology-" );
            }
            sGroup = QString( "%1-Tables" ).arg( sGroup );
            sSortTag = QString( "ZZBCY_%1" ).arg( sGroup );
          }
        }
      }
      if ( ( sGroup.startsWith( "SpatialTable" ) ) || ( sGroup.startsWith( "SpatialView" ) )  || ( sGroup.startsWith( "VirtualShapes" ) ) )
      {
        // Spatial-Admin-Tables and Views should come first
        sGroup = QString( "%1-Admin" ).arg( sGroup );
        sSortTag = QString( "ZZMA_%1" ).arg( sGroup );
      }
      if ( sGroup.startsWith( "Styles" ) )
      {
        sGroup = QString( "%1-Admin" ).arg( sGroup );
        sSortTag = QString( "ZZMB_%1" ).arg( sGroup );
      }
      if ( sGroup.startsWith( "Geometries" ) )
      {
        sGroup = QString( "%1-Admin" ).arg( sGroup );
        sSortTag = QString( "ZZMC_%1" ).arg( sGroup );
      }
      if ( sGroup.startsWith( "EPSG" ) )
      {
        sGroup = QString( "%1-Admin" ).arg( sGroup );
        sSortTag = QString( "ZZMD_%1" ).arg( sGroup );
      }
      if ( ( sGroup.startsWith( "Virtual" ) ) && ( !sGroup.startsWith( "VirtualShapes" ) ) )
      {
        sGroup = QString( "%1-Interfaces" ).arg( sGroup );
        sSortTag = QString( "ZZME_%1" ).arg( sGroup );
      }
      dbItems = findItems( sGroup, Qt::MatchExactly | Qt::MatchRecursive, 0 );
      if ( !dbItems.isEmpty() )
      {
        nonSpatialGroupItem = dbItems.at( 0 );
      }
      else
      {
        // Everything else as sorted
        QIcon iconType = QgsSpatialiteDbInfo::NonSpatialTablesTypeIcon( sGroup );
        QStandardItem *tbTypeItem = new QStandardItem( iconType, sGroup );
        tbTypeItem->setFlags( Qt::ItemIsEnabled );
        QStandardItem *emptyItem_01 = new QStandardItem();
        emptyItem_01->setFlags( Qt::ItemIsEnabled );
        //  Ever Columns must be filled (even if empty)
        QStandardItem *emptyItem_02 = new QStandardItem();
        emptyItem_02->setFlags( Qt::ItemIsEnabled );
        QStandardItem *emptyItem_03 = new QStandardItem();
        emptyItem_03->setFlags( Qt::ItemIsEnabled );
        QStandardItem *sortHiddenItem = new QStandardItem( sSortTag );
        sortHiddenItem->setFlags( Qt::NoItemFlags );
        // this must be done in the same order as the Header
        QList < QStandardItem *> tbItemList;
        tbItemList.push_back( tbTypeItem );
        tbItemList.push_back( emptyItem_01 );
        tbItemList.push_back( emptyItem_02 );
        tbItemList.push_back( emptyItem_03 );
        tbItemList.push_back( sortHiddenItem );
        mNonSpatialItem->appendRow( tbItemList );
        dbItems = findItems( sGroup, Qt::MatchExactly | Qt::MatchRecursive, 0 );
        if ( !dbItems.isEmpty() )
        {
          nonSpatialGroupItem = dbItems.at( 0 );
        }
        else
        {
          return;
        }
      }
      sSortTag = QString( "ZZZA_%1" ).arg( sKey );
      QIcon iconType = QgsSpatialiteDbInfo::NonSpatialTablesTypeIcon( sValue );
      QStandardItem *tbTypeItem = new QStandardItem( sKey );
      tbTypeItem->setFlags( Qt::ItemIsEnabled );
      QStandardItem *tbSubTypeItem = new QStandardItem( iconType, sValue );
      tbSubTypeItem->setFlags( Qt::ItemIsEnabled );
      //  Ever Columns must be filled (even if empty)
      QStandardItem *emptyItem_01 = new QStandardItem();
      emptyItem_01->setFlags( Qt::ItemIsEnabled );
      QStandardItem *emptyItem_03 = new QStandardItem();
      emptyItem_03->setFlags( Qt::ItemIsEnabled );
      QStandardItem *sortHiddenItem = new QStandardItem( sSortTag );
      sortHiddenItem->setFlags( Qt::NoItemFlags );
      // this must be done in the same order as the Header
      QList < QStandardItem *> tbItemList;
      tbItemList.push_back( tbTypeItem );
      tbItemList.push_back( emptyItem_01 );
      tbItemList.push_back( tbSubTypeItem );
      tbItemList.push_back( emptyItem_03 );
      tbItemList.push_back( sortHiddenItem );
      nonSpatialGroupItem->appendRow( tbItemList );
    }
    break;
  }
}
//-----------------------------------------------------------------
// QgsSpatiaLiteTableModel::addTableEntryLayer
//-----------------------------------------------------------------
void QgsSpatiaLiteTableModel::addTableEntryLayer( QgsSpatialiteDbLayer *dbLayer, int iLayersCount )
{
  if ( ( dbLayer ) && ( dbLayer->isLayerValid() ) )
  {
    //is there already a root item ?
    QStandardItem *dbItem = nullptr;
    QString sSortTag = "";
    QString sSearchName = dbLayer->getLayerTypeString();
    if ( sSearchName == "TopologyExport" )
    {
      sSearchName = dbLayer->getTableName();
    }
    if ( sSearchName.startsWith( "MBTiles" ) )
    {
      // Could be 'MBTilesTable' or 'MBTilesView'
      sSearchName = "MBTiles";
    }
    if ( ( sSearchName == "SpatialView" ) && ( !dbLayer->getLayerGroupName().isEmpty() ) )
    {
      sSearchName = dbLayer->getLayerGroupName();
    }
    // also searches lower level items
    QList < QStandardItem * >dbItems = findItems( sSearchName, Qt::MatchExactly | Qt::MatchRecursive, 0 );
    //there is already an item
    if ( !dbItems.isEmpty() )
    {
      dbItem = dbItems.at( 0 );
    }
    else  //create a new toplevel item
    {
      if ( ( dbLayer->getLayerTypeString() == "TopologyExport" ) || ( dbLayer->getLayerTypeString() == "GeoPackageRaster" ) )
      {
        QString sSearchGroupName = dbLayer->getLayerTypeString();
        QIcon iconType = dbLayer->getLayerTypeIcon();
        if ( dbLayer->getLayerTypeString() == "TopologyExport" )
        {
          sSearchGroupName = "SpatialiteTopology";
        }
        QString sLayerText = QString( "%1 Layers" ).arg( iLayersCount );
        if ( mTableCounter == 1 )
        {
          sLayerText = QString( "%1 Layer" ).arg( iLayersCount );
        }
        dbItems = findItems( sSearchGroupName, Qt::MatchExactly | Qt::MatchRecursive, 0 );
        if ( !dbItems.isEmpty() )
        {
          dbItem = dbItems.at( 0 );
        }
        else
        {
          // For 'TopologyExport' the extra query is possibly needed
          sSortTag = QString( "AAFA_%1" ).arg( sSearchGroupName );
          QStandardItem *tbTypeItem = new QStandardItem( iconType, sSearchGroupName );
          tbTypeItem->setFlags( Qt::ItemIsEnabled );
          QStandardItem *layerItem = new QStandardItem( sLayerText );
          layerItem->setFlags( Qt::ItemIsEnabled );
          //  Ever Columns must be filled (even if empty)
          QStandardItem *emptyItem_02 = new QStandardItem();
          emptyItem_02->setFlags( Qt::ItemIsEnabled );
          QStandardItem *emptyItem_03 = new QStandardItem();
          emptyItem_03->setFlags( Qt::ItemIsEnabled );
          QStandardItem *sortHiddenItem = new QStandardItem( sSortTag );
          sortHiddenItem->setFlags( Qt::NoItemFlags );
          // this must be done in the same order as the Header
          QList < QStandardItem *> tbItemList;
          tbItemList.push_back( tbTypeItem );
          tbItemList.push_back( layerItem );
          tbItemList.push_back( emptyItem_02 );
          tbItemList.push_back( emptyItem_03 );
          tbItemList.push_back( sortHiddenItem );
          mDbRootItem->appendRow( tbItemList );
          dbItems = findItems( sSearchGroupName, Qt::MatchExactly | Qt::MatchRecursive, 0 );
          if ( !dbItems.isEmpty() )
          {
            dbItem = dbItems.at( 0 );
          }
          else
          {
            return;
          }
        }
      }
      else
      {
        return;
      }
    }
    sSortTag = QString( "AZZZ_%1" ).arg( dbLayer->getLayerName() );
    QList < QStandardItem * >childItemList;
    QString sTableName = dbLayer->getTableName();
    QString sGeomItemText = dbLayer->getGeometryColumn();
    QString sGeometryTypeString = dbLayer->getGeometryTypeString();
    if ( sSearchName == "MBTiles" )
    {
      if ( !dbLayer->getTitle().isEmpty() )
      {
        // if Title is set, show it instead of the file-name [which TableName contains]
        sTableName = dbLayer->getTitle();
      }
      sGeomItemText = ""; // Must remain empty
      sGeometryTypeString = dbLayer->getAbstract();
    }
    if ( sSearchName == "RasterLite2Raster" )
    {
      sGeomItemText = ""; // Must remain empty
      sGeometryTypeString = dbLayer->getTitle();
    }
    QStandardItem *tableNameItem = new QStandardItem( dbLayer->getLayerTypeIcon(), sTableName );
    tableNameItem->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
    QStandardItem *geomItem = new QStandardItem( sGeomItemText );
    geomItem->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
    QIcon iconType;
    if ( sGeometryTypeString != "Unknown" )
    {
      iconType = dbLayer->getGeometryTypeIcon();
    }
    else
    {
      QString sGroup = dbLayer->getLayerTypeString();
      if ( sGroup == "RasterLite1" )
      {
        sGroup = "RasterLite1-Tiles";
        sGeometryTypeString = sGroup;
      }
      if ( sGroup == "RasterLite2Raster" )
      {
        sGroup = "RasterLite2-Tiles";
        sGeometryTypeString = sGroup;
      }
      if ( sGroup == "GeoPackageRaster" )
      {
        sGroup = "GeoPackage-Tiles";
        sGeometryTypeString = sGroup;
      }
      if ( sGroup == "GeoPackageVector" )
      {
        sGroup = "GeoPackage-Vector";
        sGeometryTypeString = sGroup;
      }
      iconType = QgsSpatialiteDbInfo::NonSpatialTablesTypeIcon( sGroup );
    }
    QStandardItem *geomTypeItem = nullptr;
    QStandardItem *sqlItem  = nullptr;
    if ( sSearchName == "MBTiles" )
    {
      geomTypeItem = new QStandardItem( sGeometryTypeString );
      sqlItem = new QStandardItem( dbLayer->getLayerQuery() );
    }
    else
    {
      geomTypeItem = new QStandardItem( iconType, sGeometryTypeString );
      geomTypeItem->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
      sqlItem = new QStandardItem( dbLayer->getLayerQuery() );
      sqlItem->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
    }
    QStandardItem *sortHiddenItem = new QStandardItem( sSortTag );
    sortHiddenItem->setFlags( Qt::NoItemFlags );
    // this must be done in the same order as the Header
    childItemList.push_back( tableNameItem );
    childItemList.push_back( geomItem );
    childItemList.push_back( geomTypeItem );
    childItemList.push_back( sqlItem );
    childItemList.push_back( sortHiddenItem );
    dbItem->appendRow( childItemList );
    ++mTableCount;
  }
}
//-----------------------------------------------------------------
// QgsSpatiaLiteTableModel::setSql
//-----------------------------------------------------------------
void QgsSpatiaLiteTableModel::setSql( const QModelIndex &index, const QString &sql )
{
  if ( !index.isValid() || !index.parent().isValid() )
  {
    return;
  }

  //find out table name
  QModelIndex tableSibling = index.sibling( index.row(), getTableNameIndex() );
  QModelIndex geomSibling = index.sibling( index.row(), getGeometryNameIndex() );

  if ( !tableSibling.isValid() || !geomSibling.isValid() )
  {
    return;
  }

  QModelIndex sqlIndex = index.sibling( index.row(), getSqlQueryIndex() );
  if ( sqlIndex.isValid() )
  {
    itemFromIndex( sqlIndex )->setText( sql );
  }
}
