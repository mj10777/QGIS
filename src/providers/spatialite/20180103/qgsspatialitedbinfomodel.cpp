/***************************************************************************
                        qgsspatialitedbinfomodel.cpp  -  description
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

#include <QtWidgets>
#include <QStringList>
#include <QList>
#include "qgslogger.h"

#include "qgsspatialitedbinfomodel.h"
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::QgsSpatialiteDbInfoModel
//-----------------------------------------------------------------
QgsSpatialiteDbInfoModel::QgsSpatialiteDbInfoModel( QObject *parent )
  : QAbstractItemModel( parent )
{
  // The rootItem [nullptr, ItemTypeRoot] will set the Column-Header text and store the positions
  mDbRootItem = new QgsSpatialiteDbInfoItem( nullptr, QgsSpatialiteDbInfoItem::ItemTypeRoot );
  // Store Column-Header positions numbers, which can be retrieved from the outside
  mColumnTable = mDbRootItem->getTableNameIndex();
  mColumnGeometryName = mDbRootItem->getGeometryNameIndex();
  mColumnGeometryType = mDbRootItem->getGeometryTypeIndex();
  mColumnSql = mDbRootItem->getSqlQueryIndex();
  mColumnSortHidden = mDbRootItem->getColumnSortHidden();
  // Connect willAdd/Added/Remove/Removed Children Events
  connectToRootItem();
  // Display text for the User, giving some information about what the Model is used for
  loadItemHelp();
  // QTableView {selection-background-color: #308cc6;selection-color: #ffffff;}
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::loadItemHelp
// Display text for the User, giving some information about what the Model is used for
// - Will be removed when the first Database is loaded
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoModel::loadItemHelp()
{
  // This can only be called after 'connectToRootItem' has run
  mDbHelpRoot = new QgsSpatialiteDbInfoItem( mDbRootItem, QgsSpatialiteDbInfoItem::ItemTypeHelpRoot );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::~QgsSpatialiteDbInfoModel
//-----------------------------------------------------------------
QgsSpatialiteDbInfoModel::~QgsSpatialiteDbInfoModel()
{
  delete mDbRootItem;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::getTableNameText
//-----------------------------------------------------------------
QString QgsSpatialiteDbInfoModel::getTableNameText() const
{
  return mDbRootItem->text( getTableNameIndex() );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::getGeometryNameText
//-----------------------------------------------------------------
QString QgsSpatialiteDbInfoModel::getGeometryNameText() const
{
  return mDbRootItem->text( getGeometryNameIndex() );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::getGeometryTypeText
//-----------------------------------------------------------------
QString QgsSpatialiteDbInfoModel::getGeometryTypeText() const
{
  return mDbRootItem->text( getGeometryTypeIndex() );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::getSqlQueryText
//-----------------------------------------------------------------
QString QgsSpatialiteDbInfoModel::getSqlQueryText() const
{
  return mDbRootItem->text( getSqlQueryIndex() );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::connectToRootItem()
//-----------------------------------------------------------------
// - Based on QgsLayerTreeModel::connectToRootNode
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoModel::connectToRootItem()
{
  Q_ASSERT( mDbRootItem );
  connect( mDbRootItem, &QgsSpatialiteDbInfoItem::willAddChildren, this, &QgsSpatialiteDbInfoModel::nodeWillAddChildren );
  connect( mDbRootItem, &QgsSpatialiteDbInfoItem::addedChildren, this, &QgsSpatialiteDbInfoModel::nodeAddedChildren );
  connect( mDbRootItem, &QgsSpatialiteDbInfoItem::willRemoveChildren, this, &QgsSpatialiteDbInfoModel::nodeWillRemoveChildren );
  connect( mDbRootItem, &QgsSpatialiteDbInfoItem::removedChildren, this, &QgsSpatialiteDbInfoModel::nodeRemovedChildren );
  // connect( mDbRootItem, &QgsSpatialiteDbInfoItem::visibilityChanged, this, &QgsSpatialiteDbInfoModel::nodeVisibilityChanged );
  // connect( mDbRootItem, &QgsSpatialiteDbInfoItem::nameChanged, this, &QgsSpatialiteDbInfoModel::nodeNameChanged );
  // connect( mDbRootItem, &QgsSpatialiteDbInfoItem::customPropertyChanged, this, &QgsSpatialiteDbInfoModel::nodeCustomPropertyChanged );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::nodeWillAddChildren
// - node is the Item the Children will be added to
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoModel::nodeWillAddChildren( QgsSpatialiteDbInfoItem *node, int indexFrom, int indexTo )
{
  if ( !node )
  {
    return;
  }
  beginInsertRows( node2index( node ), indexFrom, indexTo );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::nodeAddedChildren
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoModel::nodeAddedChildren( QgsSpatialiteDbInfoItem *node, int indexFrom, int indexTo )
{
  Q_ASSERT( node );
  Q_UNUSED( indexFrom );
  Q_UNUSED( indexTo );
  endInsertRows();
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::nodeWillRemoveChildren
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoModel::nodeWillRemoveChildren( QgsSpatialiteDbInfoItem *node, int indexFrom, int indexTo )
{
  Q_ASSERT( node );
  beginRemoveRows( node2index( node ), indexFrom, indexTo );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::index2node
//-----------------------------------------------------------------
QgsSpatialiteDbInfoItem *QgsSpatialiteDbInfoModel::index2node( const QModelIndex &index ) const
{
  if ( !index.isValid() )
    return mDbRootItem;
  QObject *obj = reinterpret_cast<QObject *>( index.internalPointer() );
  return qobject_cast<QgsSpatialiteDbInfoItem *>( obj );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::node2index
//-----------------------------------------------------------------
QModelIndex QgsSpatialiteDbInfoModel::node2index( QgsSpatialiteDbInfoItem *node ) const
{
  if ( ( !node )  || ( !node->parent() ) )
  {
    return QModelIndex(); // this is the only root item -> invalid index
  }
  QModelIndex parentIndex = node2index( node->parent() );
  int row = node->parent()->children().indexOf( node );
  // QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoModel::node2index type[%1] row[%2]" ).arg( node->getItemType() ).arg( row ), 7 );
  Q_ASSERT( row >= 0 );
  return index( row, 0, parentIndex );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::nodeRemovedChildren
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoModel::nodeRemovedChildren()
{
  endRemoveRows();
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::disconnectFromDbRootItem
//-----------------------------------------------------------------
// - Based on QgsLayerTreeModel::disconnectFromRootNode
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoModel::disconnectFromRootItem()
{
  disconnect( mDbRootItem, nullptr, this, nullptr );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::columnCount
//-----------------------------------------------------------------
int QgsSpatialiteDbInfoModel::columnCount( const QModelIndex & /* parent */ ) const
{
  return mDbRootItem->columnCount();
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::data
//-----------------------------------------------------------------
QVariant QgsSpatialiteDbInfoModel::data( const QModelIndex &index, int role ) const
{
  if ( !index.isValid() )
  {
    return QVariant();
  }
  if ( role == Qt::BackgroundRole )
  {
    //  if (condition1)
    //     return QColor(Qt::red);
    //  else
    //     return QColor(Qt::green);
  }
  switch ( role )
  {
    case Qt::DisplayRole:
    case Qt::DecorationRole:
    case Qt::EditRole:
    case Qt::ToolTipRole:
    case Qt::StatusTipRole:
    case Qt::TextAlignmentRole:
    case Qt::BackgroundRole:
    case Qt::ForegroundRole:
    // flags
    case ( Qt::UserRole-1 ):
    {
      // 0=Qt::DisplayRole ,1=Qt::DecorationRole, 2=Qt::EditRole, 3=Qt::ToolTipRole, 4=Qt::StatusTipRole
      // 7=Qt::TextAlignmentRole, 8=Qt::BackgroundRole,9=Qt::ForegroundRole, 255=Qt::UserRole-1
      QgsSpatialiteDbInfoItem *item = getItem( index );
      return item->data( index.column(), role );
    }
    break;
    case Qt::FontRole:
    case Qt::CheckStateRole:
    case Qt::SizeHintRole:
    default:
      // 6=Qt::FontRole, 10=Qt::CheckStateRole,13=Qt::SizeHintRole
      // QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoModel::data-3- index.row/column[%1,%2] role[%3] " ).arg( index.row() ).arg( index.column() ).arg( role ), 7 );
      break;
  }
  return QVariant();
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::flags
// will return the set flag for the given Item's column
// 0=Qt::NoItemFlags will be returned on error
//-----------------------------------------------------------------
Qt::ItemFlags QgsSpatialiteDbInfoModel::flags( const QModelIndex &index ) const
{
  if ( !index.isValid() )
  {
    return Qt::NoItemFlags;
  }
  // return Qt::ItemIsEditable | QAbstractItemModel::flags( index );
  QgsSpatialiteDbInfoItem *item = getItem( index );
  if ( item )
  {
    Qt::ItemFlags itemFlags = item->flags( index.column() );
#if 0
    bool bEditable = ( itemFlags & Qt::ItemIsEditable ) != 0;
    bool bSelectable = ( itemFlags & Qt::ItemIsSelectable ) != 0;
    bool bEnabled = ( itemFlags & Qt::ItemIsEnabled ) != 0;
    QgsDebugMsgLevel( QString( "-I-> QgsSpatialiteDbInfoModel::flags(%1,isEditable=%4,isSelectable=%5,isEnabled=%6) item.row/column[%2,%3]" ).arg( itemFlags ).arg( index.row() ).arg( index.column() ).arg( bEditable ).arg( bSelectable ).arg( bEnabled ), 7 );
#endif
    return itemFlags;
  }
  return Qt::NoItemFlags;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::getItem
//-----------------------------------------------------------------
QgsSpatialiteDbInfoItem *QgsSpatialiteDbInfoModel::getItem( const QModelIndex &index ) const
{
  if ( index.isValid() )
  {
    QgsSpatialiteDbInfoItem *item = static_cast<QgsSpatialiteDbInfoItem *>( index.internalPointer() );
    if ( item )
    {
      return item;
    }
  }
  return mDbRootItem;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::headerData
//-----------------------------------------------------------------
QVariant QgsSpatialiteDbInfoModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( ( orientation == Qt::Horizontal ) &&
       ( ( role == Qt::DisplayRole ) || ( role == Qt::EditRole ) || ( role == Qt::DecorationRole ) || ( role == Qt::TextAlignmentRole ) ||
         ( role == Qt::BackgroundRole ) || ( role == Qt::ForegroundRole ) ) )
  {
    // 0=Qt::DisplayRole ,1=Qt::DecorationRole, , 2=Qt::EditRole, 7=Qt::TextAlignmentRole, 8=Qt::BackgroundRole,9=Qt::ForegroundRole
    // QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoModel::headerData section[%1] role[%2] orientation[%3]" ).arg( section ).arg( role ).arg( orientation ), 7 );
    return mDbRootItem->data( section, role );
  }
  // QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoModel::headerData section[%1] role[%2] orientation[%3]" ).arg( section ).arg( role ).arg( orientation), 7 );
  // 2=Qt::EditRole, 6=Qt::FontRole,13=Qt::SizeHintRole
  return QVariant();
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::index
//-----------------------------------------------------------------
QModelIndex QgsSpatialiteDbInfoModel::index( int row, int column, const QModelIndex &parent ) const
{
  // QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoModel::index row[%1] column[%2] " ).arg( row ).arg( column ), 7 );
  // https://forum.qt.io/topic/41977/solved-how-to-find-a-child-in-a-qabstractitemmodel/5
  // if ( parent.isValid() && parent.column() != 0 )
  if ( !hasIndex( row, column, parent ) )
  {
    return QModelIndex();
  }
  QgsSpatialiteDbInfoItem *parentItem = getItem( parent );
  QgsSpatialiteDbInfoItem *childItem = parentItem->child( row );
  if ( childItem )
  {
    return createIndex( row, column, childItem );
  }
  else
  {
    return QModelIndex();
  }
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::insertColumns
//-----------------------------------------------------------------
bool QgsSpatialiteDbInfoModel::insertColumns( int position, int columns, const QModelIndex &parent )
{
  bool success;
  beginInsertColumns( parent, position, position + columns - 1 );
  success = mDbRootItem->insertColumns( position, columns );
  endInsertColumns();
  return success;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::insertRows
// Defaults: ItemType=ItemTypeUnknown, columnCount()
//-----------------------------------------------------------------
bool QgsSpatialiteDbInfoModel::insertRows( int position, int rows, const QModelIndex &parent )
{
  QgsSpatialiteDbInfoItem *parentItem = getItem( parent );
  bool success;
  beginInsertRows( parent, position, position + rows - 1 );
  // success = parentItem->insertChildren( position, rows, mDbRootItem->getItemType(), mDbRootItem->columnCount() );
  success = parentItem->insertChildren( position, rows );
  endInsertRows();
  return success;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::parent
//-----------------------------------------------------------------
QModelIndex QgsSpatialiteDbInfoModel::parent( const QModelIndex &index ) const
{
  if ( !index.isValid() )
  {
    return QModelIndex();
  }
  QgsSpatialiteDbInfoItem *childItem = getItem( index );
  QgsSpatialiteDbInfoItem *parentItem = childItem->parent();
  if ( parentItem == mDbRootItem )
  {
    return QModelIndex();
  }
  return createIndex( parentItem->childNumber(), 0, parentItem );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::removeColumns
//-----------------------------------------------------------------
bool QgsSpatialiteDbInfoModel::removeColumns( int position, int columns, const QModelIndex &parent )
{
  bool success;
  beginRemoveColumns( parent, position, position + columns - 1 );
  success = mDbRootItem->removeColumns( position, columns );
  endRemoveColumns();
  if ( mDbRootItem->columnCount() == 0 )
  {
    removeRows( 0, rowCount() );
  }
  return success;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::removeRows
// Remove the Children from Root
// - removeChildren will delete the Children
//-----------------------------------------------------------------
bool QgsSpatialiteDbInfoModel::removeRows( int position, int rows, const QModelIndex &parent )
{
  QgsSpatialiteDbInfoItem *parentItem = getItem( parent );
  bool success = true;
  beginRemoveRows( parent, position, position + rows - 1 );
  success = parentItem->removeChildren( position, rows );
  endRemoveRows();
  return success;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::rowCount
//-----------------------------------------------------------------
int QgsSpatialiteDbInfoModel::rowCount( const QModelIndex &parent ) const
{
  QgsSpatialiteDbInfoItem *parentItem = getItem( parent );
  return parentItem->childCount();
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::setData
//-----------------------------------------------------------------
bool QgsSpatialiteDbInfoModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
  bool bRc =  false;
  if ( ( role != Qt::DisplayRole ) && ( role != Qt::EditRole ) && ( role != Qt::DecorationRole ) && ( role != ( Qt::UserRole - 1 ) ) )
  {
    // Not: Text, QIcon or flags
    return bRc;
  }
  QgsSpatialiteDbInfoItem *item = getItem( index );
  if ( item )
  {
    bRc = item->setData( index.column(), value, role );
    QgsDebugMsgLevel( QString( "-I-> QgsSpatialiteDbInfoModel::setData(role=%1) item.row/column[%2,%3] result=%4" ).arg( role ).arg( index.row() ).arg( index.column() ).arg( bRc ), 7 );
    if ( bRc )
    {
      emit dataChanged( index, index );
    }
  }
  return bRc;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::setHeaderData
//-----------------------------------------------------------------
bool QgsSpatialiteDbInfoModel::setHeaderData( int section, Qt::Orientation orientation, const QVariant &value, int role )
{
  if ( role != Qt::EditRole || orientation != Qt::Horizontal )
  {
    // return false;
  }
  bool result = mDbRootItem->setData( section, value, role );
  if ( result )
  {
    emit headerDataChanged( orientation, section, section );
  }
  return result;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::setSqliteDb
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoModel::setSqliteDb( QgsSpatialiteDbInfo *spatialiteDbInfo )
{
  if ( ( spatialiteDbInfo ) && ( spatialiteDbInfo->isDbValid() ) )
  {
    if ( mSpatialiteDbInfo )
    {
      mTableCount = 0;
      if ( mDbItem )
      {
        // remove Children from mDbRootItem
        // - the children will be deleted
        removeRows( 0, rowCount() );
        if ( mDbItem )
        {
          mDbItem = nullptr;
        }
      }
      if ( mSpatialiteDbInfo->getQSqliteHandle() )
      {
        mSpatialiteDbInfo = nullptr;
      }
      else
      {
        delete spatialiteDbInfo;
        mSpatialiteDbInfo = nullptr;
      }
    }
    else
    {
      if ( mDbHelpRoot )
      {
        // remove Children from mDbRootItem
        // - the children will be deleted
        removeRows( 0, rowCount() );
        mDbHelpRoot = nullptr;
      }
    }
    mSpatialiteDbInfo = spatialiteDbInfo;
    mDbLayersDataSourceUris = getDataSourceUris();
    mTableCounter = 0;
    mDbItem = new QgsSpatialiteDbInfoItem( mDbRootItem, mSpatialiteDbInfo );
    if ( mDbItem )
    {
      mTableCounter = mDbRootItem->getTableCounter();
      mDbItem->insertPrepairedChildren();
    }
    return;
    addTableEntryTypes();
  }
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::addTableEntryTypes
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoModel::addTableEntryTypes()
{
  // addDatabaseEntry();
  QMap<QString, QString> mapLayers;
  if ( mSpatialiteDbInfo->dbVectorLayersCount() > 0 )
  {
    // - SpatialTables, SpatialViews and VirtualShapes [from the vector_layers View]
    // -- contains: Layer-Name as 'table_name(geometry_name)', Geometry-Type (with dimension) and Srid
    if ( mSpatialiteDbInfo->dbSpatialTablesLayersCount() > 0 )
    {
      addTableEntryType( mSpatialiteDbInfo->getDbLayersType( QgsSpatialiteDbInfo::SpatialTable ), QgsSpatialiteDbInfoModel::EntryTypeLayer, QgsSpatialiteDbInfo::SpatialTable );
    }
    if ( mSpatialiteDbInfo->dbSpatialViewsLayersCount() > 0 )
    {
      addTableEntryType( mSpatialiteDbInfo->getDbLayersType( QgsSpatialiteDbInfo::SpatialView ), QgsSpatialiteDbInfoModel::EntryTypeLayer, QgsSpatialiteDbInfo::SpatialView );
    }
    if ( mSpatialiteDbInfo->dbVirtualShapesLayersCount() > 0 )
    {
      addTableEntryType( mSpatialiteDbInfo->getDbLayersType( QgsSpatialiteDbInfo::VirtualShape ), QgsSpatialiteDbInfoModel::EntryTypeLayer, QgsSpatialiteDbInfo::VirtualShape );
    }
  }
  if ( mSpatialiteDbInfo->dbRasterCoveragesLayersCount() > 0 )
  {
    addTableEntryType( mSpatialiteDbInfo->getDbRasterCoveragesLayers(), QgsSpatialiteDbInfoModel::EntryTypeLayer, QgsSpatialiteDbInfo::RasterLite2Raster );
  }
  if ( mSpatialiteDbInfo->dbTopologyExportLayersCount() > 0 )
  {
    addTableEntryType( mSpatialiteDbInfo->getDbTopologyExportLayers(), QgsSpatialiteDbInfoModel::EntryTypeLayer, QgsSpatialiteDbInfo::TopologyExport );
  }
  if ( mSpatialiteDbInfo->dbRasterLite1LayersCount() > 0 )
  {
    addTableEntryType( mSpatialiteDbInfo->getDbRasterLite1Layers(), QgsSpatialiteDbInfoModel::EntryTypeLayer, QgsSpatialiteDbInfo::RasterLite1 );
  }
  if ( mSpatialiteDbInfo->dbMBTilesLayersCount() > 0 )
  {
    addTableEntryType( mSpatialiteDbInfo->getDbMBTilesLayers(), QgsSpatialiteDbInfoModel::EntryTypeLayer, QgsSpatialiteDbInfo::MBTilesTable );
  }
  if ( mSpatialiteDbInfo->dbGeoPackageLayersCount() > 0 )
  {
    addTableEntryType( mSpatialiteDbInfo->getDbGeoPackageLayers(), QgsSpatialiteDbInfoModel::EntryTypeLayer, QgsSpatialiteDbInfo::GeoPackageVector );
  }
  if ( mSpatialiteDbInfo->dbFdoOgrLayersCount() > 0 )
  {
    addTableEntryType( mSpatialiteDbInfo->getDbFdoOgrLayers(), QgsSpatialiteDbInfoModel::EntryTypeLayer, QgsSpatialiteDbInfo::GdalFdoOgr );
  }
  if ( mSpatialiteDbInfo->dbNonSpatialTablesCount() > 0 )
  {
    addTableEntryType( mSpatialiteDbInfo->getDbNonSpatialTables(), QgsSpatialiteDbInfoModel::EntryTypeMap, QgsSpatialiteDbInfo::NonSpatialTables );
#if 0
// TODO: resolve changed QMap-Type
    if ( mSpatialiteDbInfo->dbVectorStylesCount() > 0 )
    {
      addTableEntryType( mSpatialiteDbInfo->getDbVectorStylesInfo(), QgsSpatialiteDbInfoModel::EntryTypeMap, QgsSpatialiteDbInfo::StyleVector );
    }
    if ( mSpatialiteDbInfo->dbRasterStylesCount() > 0 )
    {
      addTableEntryType( mSpatialiteDbInfo->getDbRasterStylesInfo(), QgsSpatialiteDbInfoModel::EntryTypeMap, QgsSpatialiteDbInfo::StyleRaster );
    }
#endif
  }
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::addDatabaseEntry
//-----------------------------------------------------------------
bool QgsSpatialiteDbInfoModel::addDatabaseEntry()
{
  bool bRc = false;
  if ( ( mSpatialiteDbInfo ) && ( mSpatialiteDbInfo->isDbValid() ) )
  {
    QList < QgsSpatialiteDbInfoItem * >dbItems;
    //is there already a root item ?
    QgsSpatialiteDbInfoItem *dbItem = nullptr;
    // Only searches top level items
    // dbItems = findItems( mSpatialiteDbInfo->getFileName(), Qt::MatchExactly, 0 );
    // if ( !dbItems.isEmpty() )
    if ( dbItem )
    {
      // An Item of that name was found.
      dbItem = dbItems.at( 0 );
      bRc = true;
    }
    else  //create a new toplevel item
    {
      mDbItem = new QgsSpatialiteDbInfoItem( mDbRootItem, mSpatialiteDbInfo );
      dbItem = mDbItem;
      bRc = true;
    }
  }
  return bRc;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::addTableEntryType
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoModel::addTableEntryType( QMap<QString, QString> mapLayers, QgsSpatialiteDbInfoModel::EntryType entryType, QgsSpatialiteDbInfo::SpatialiteLayerType layerType )
{
  QString sGroupLayerType = QgsSpatialiteDbInfo::SpatialiteLayerTypeName( layerType );
  bool bLoadLayer = true;
  for ( QMap<QString, QString>::iterator itLayers = mapLayers.begin(); itLayers != mapLayers.end(); ++itLayers )
  {
    if ( !itLayers.key().isEmpty() )
    {
      switch ( entryType )
      {
        case QgsSpatialiteDbInfoModel::EntryTypeMap:
        {
          addTableEntryMap( itLayers.key(), itLayers.value(), layerType );
        }
        break;
        case QgsSpatialiteDbInfoModel::EntryTypeLayer:
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
// QgsSpatialiteDbInfoModel::addTableEntryLayer
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoModel::addTableEntryLayer( QgsSpatialiteDbLayer *dbLayer, int iLayersCount )
{
  Q_UNUSED( dbLayer );
  Q_UNUSED( iLayersCount );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::addTableEntryMap
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoModel::addTableEntryMap( QString sKey, QString sValue, QgsSpatialiteDbInfo::SpatialiteLayerType layerType )
{
  Q_UNUSED( sKey );
  Q_UNUSED( sValue );
  Q_UNUSED( layerType );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::findItems
// - taken from QStandardItemModel
// -> uses getItem() instead of 'itemFromIndex' as in QStandardItemModel
//-----------------------------------------------------------------
QList<QgsSpatialiteDbInfoItem *> QgsSpatialiteDbInfoModel::findItems( const QString &text, Qt::MatchFlags flags, int column ) const
{
  QModelIndexList indexes = match( index( 0, column, QModelIndex() ), Qt::EditRole, text, -1, flags );
  QList<QgsSpatialiteDbInfoItem *> items;
  const int numIndexes = indexes.size();
  items.reserve( numIndexes );
  for ( int i = 0; i < numIndexes; ++i )
  {
    items.append( getItem( indexes.at( i ) ) );
  }
  return items;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::getTableName
//-----------------------------------------------------------------
QString QgsSpatialiteDbInfoModel::getTableName( const QModelIndex &index ) const
{
  QgsSpatialiteDbInfoItem *item = getItem( index );
  if ( item )
  {
    return item->data( getTableNameIndex() ).toString();
  }
  return QString();
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::getGeometryName
//-----------------------------------------------------------------
QString QgsSpatialiteDbInfoModel::getGeometryName( const QModelIndex &index ) const
{
  QgsSpatialiteDbInfoItem *item = getItem( index );
  if ( item )
  {
    return item->data( getGeometryNameIndex() ).toString();
  }
  return QString();
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::getLayerName
//-----------------------------------------------------------------
QString QgsSpatialiteDbInfoModel::getLayerName( const QModelIndex &index ) const
{
  QString sLayerName = getTableName( index );
  QString sGeometryName = getGeometryName( index );
  if ( !sGeometryName.isEmpty() )
  {
    sLayerName = QString( "%1(%2)" ).arg( sLayerName ).arg( sGeometryName );
  }
  if ( mDbLayersDataSourceUris.contains( sLayerName ) )
  {
    return sLayerName;
  }
  return QString();
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::getLayerNameUris
//-----------------------------------------------------------------
QString QgsSpatialiteDbInfoModel::getLayerNameUris( const QModelIndex &index ) const
{
  QString sLayerUris = QString();
  QString sLayerName = getLayerName( index );
  if ( !sLayerName.isEmpty() )
  {
    sLayerUris = mDbLayersDataSourceUris.value( sLayerName );
  }
  return sLayerUris;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::getGeometryType
//-----------------------------------------------------------------
QString QgsSpatialiteDbInfoModel::getGeometryType( const QModelIndex &index ) const
{
  QgsSpatialiteDbInfoItem *item = getItem( index );
  if ( item )
  {
    return item->data( getGeometryTypeIndex() ).toString();
  }
  return QString();
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::getSqlQuery
//-----------------------------------------------------------------
QString QgsSpatialiteDbInfoModel::getSqlQuery( const QModelIndex &index ) const
{
  QgsSpatialiteDbInfoItem *item = getItem( index );
  if ( item )
  {
    return item->data( getSqlQueryIndex() ).toString();
  }
  return QString();
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::getDbName
//-----------------------------------------------------------------
QString QgsSpatialiteDbInfoModel::getDbName( bool withPath ) const
{
  QString sDatabaseName = QString();
  if ( ( mSpatialiteDbInfo ) && ( mSpatialiteDbInfo->isDbValid() ) )
  {
    if ( withPath )
    {
      sDatabaseName = mSpatialiteDbInfo->getDatabaseFileName();
    }
    else
    {
      sDatabaseName = mSpatialiteDbInfo->getFileName();
    }
  }
  return sDatabaseName;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::getDatabaseUri
//-----------------------------------------------------------------
QString QgsSpatialiteDbInfoModel::getDatabaseUri() const
{
  QString sDatabaseUri = QString();
  if ( ( mSpatialiteDbInfo ) && ( mSpatialiteDbInfo->isDbValid() ) )
  {
    sDatabaseUri = mSpatialiteDbInfo->getDatabaseUri();
  }
  return sDatabaseUri;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::setSql
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoModel::setSql( const QModelIndex &index, const QString &sql )
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
  QgsSpatialiteDbInfoItem *item = getItem( index );
  if ( item )
  {
    item->setData( index.column(), sql );
  }
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::UpdateLayerStatistics
//-----------------------------------------------------------------
bool QgsSpatialiteDbInfoModel::UpdateLayerStatistics( QStringList saLayers )
{
  if ( ( mSpatialiteDbInfo ) && ( mSpatialiteDbInfo->isDbValid() ) && ( mSpatialiteDbInfo->isDbSpatialite() ) )
  {
    return mSpatialiteDbInfo->UpdateLayerStatistics( saLayers );
  }
  return false;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::createDatabase
//-----------------------------------------------------------------
bool QgsSpatialiteDbInfoModel::createDatabase( QString sDatabaseFileName, QgsSpatialiteDbInfo::SpatialMetadata dbCreateOption )
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
        case QgsSpatialiteDbInfo::Spatialite45:
          switch ( spatialiteDbInfo->dbSpatialMetadata() )
          {
            case QgsSpatialiteDbInfo::SpatialiteLegacy:
            case QgsSpatialiteDbInfo::Spatialite40:
            case QgsSpatialiteDbInfo::Spatialite45:
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
      setSqliteDb( spatialiteDbInfo );
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
// -- ---------------------------------- --
// class QgsSpatialiteDbInfoItem
// - for Item Rows [with children columns]
// -- ---------------------------------- --
QgsSpatialiteDbInfoItem::QgsSpatialiteDbInfoItem( QgsSpatialiteDbInfoItem *parent, SpatialiteDbInfoItemType itemType )
{
  mParentItem = parent;
  mItemType = itemType;
  if ( !mParentItem )
  {
    if ( mItemType == ItemTypeRoot )
    {
      // QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem:: creating [%1]" ).arg( QString( "mDbRootItem" ) ), 7 );
    }
  }
  if ( onInit() )
  {
    if ( mItemType == ItemTypeHelpRoot )
    {
      addHelpInfo();
      insertPrepairedChildren();
    }
  }
}
// -- ---------------------------------- --
// class QgsSpatialiteDbInfoItem
// - for Item Rows [with children columns]
// -- ---------------------------------- --
QgsSpatialiteDbInfoItem::QgsSpatialiteDbInfoItem( QgsSpatialiteDbInfoItem *parent, QgsSpatialiteDbInfo *spatialiteDbInfo )
{
  mParentItem = parent;
  mSpatialiteDbInfo = spatialiteDbInfo;
  if ( mSpatialiteDbInfo )
  {
    mItemType = ItemTypeDb;
  }
  QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem:: ItemTypeDb FileName[%1]" ).arg( mSpatialiteDbInfo->getFileName() ), 7 );
  if ( onInit() )
  {
    // Column 0: getTableNameIndex()
    setText( getTableNameIndex(), mSpatialiteDbInfo->getFileName() );
    setIcon( getTableNameIndex(), mSpatialiteDbInfo->getSpatialMetadataIcon() );
    // Column 1: getGeometryNameIndex()
    setText( getGeometryNameIndex(), mSpatialiteDbInfo->dbSpatialMetadataString() );
    setIcon( getGeometryNameIndex(), mSpatialiteDbInfo->getSpatialMetadataIcon() );
    // Column 2: getGeometryTypeIndex()
    QString sLayerText = QStringLiteral( "%1 Layers" ).arg( mTableCounter );
    if ( mTableCounter == 1 )
    {
      sLayerText = QStringLiteral( "%1 Layer" ).arg( mTableCounter );
    }
    QgsDebugMsgLevel( QStringLiteral( "QgsSpatialiteDbInfoItem:: ItemTypeDb LayerText[%1]" ).arg( sLayerText ), 7 );
    setText( getGeometryTypeIndex(), sLayerText );
    QString sProvider = QStringLiteral( "QgsSpatiaLiteProvider" );
    QString sInfoText = "";
    switch ( mSpatialiteDbInfo->dbSpatialMetadata() )
    {
      case QgsSpatialiteDbInfo::SpatialiteFdoOgr:
        sProvider = QStringLiteral( "QgsOgrProvider" );
        sInfoText = QStringLiteral( "The 'SQLite' Driver is NOT active and cannot be displayed" );
        if ( mSpatialiteDbInfo->hasDbFdoOgrDriver() )
        {
          sInfoText = QStringLiteral( "The 'SQLite' Driver is active and can be displayed" );
        }
        break;
      case QgsSpatialiteDbInfo::SpatialiteGpkg:
        sProvider = QStringLiteral( "QgsOgrProvider" );
        sInfoText = QStringLiteral( "The 'GPKG' Driver is NOT active and cannot be displayed" );
        if ( mSpatialiteDbInfo->hasDbGdalGeoPackageDriver() )
        {
          sInfoText = QStringLiteral( "The 'GPKG' Driver is active and can be displayed" );
        }
        break;
      case QgsSpatialiteDbInfo::SpatialiteMBTiles:
        sProvider = QStringLiteral( "QgsGdalProvider" );
        sInfoText = QStringLiteral( "The 'MBTiles' Driver is NOT active and cannot be displayed" );
        if ( mSpatialiteDbInfo->hasDbGdalMBTilesDriver() )
        {
          sInfoText = QStringLiteral( "The 'MBTiles' Driver is active and can be displayed" );
        }
        break;
      case QgsSpatialiteDbInfo::SpatialiteLegacy:
        if ( mSpatialiteDbInfo->dbRasterLite1LayersCount() > 0 )
        {
          sProvider = QStringLiteral( "QgsGdalProvider" );
          sInfoText = QStringLiteral( "The 'RASTERLITE' Driver is NOT active and cannot be displayed" );
          if ( mSpatialiteDbInfo->hasDbGdalRasterLite1Driver() )
          {
            sInfoText = QStringLiteral( "The 'RASTERLITE' Driver is active and can be displayed" );
          }
        }
        break;
      default:
        break;
    }
    if ( !sInfoText.isEmpty() )
    {
      sProvider = QStringLiteral( "%1 [%2]" ).arg( sProvider ).arg( sInfoText );
    }
    // Column 3: getSqlQueryIndex()
    setText( getSqlQueryIndex(), sProvider );
    setIcon( getSqlQueryIndex(), QgsSpatialiteDbInfo::SpatialMetadataTypeIcon( mSpatialiteDbInfo->dbSpatialMetadata() ) );
    // Column 4: getColumnSortHidden()
    QString sSortTag = QStringLiteral( "AAAA_%1" ).arg( mSpatialiteDbInfo->getFileName() );
    setText( getColumnSortHidden(), sSortTag );
    QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem:: ItemTypeDb ProviderText[%1]" ).arg( sProvider ), 7 );
  }
}
QgsSpatialiteDbInfoItem::QgsSpatialiteDbInfoItem( QgsSpatialiteDbInfoItem *parent, QgsSpatialiteDbLayer *dbLayer )
{
  mParentItem = parent;
  mDbLayer = dbLayer;
  if ( mDbLayer )
  {
    mItemType = ItemTypeLayer;
  }
  if ( onInit() )
  {
  }
}
// -- ---------------------------------- --
// class QgsSpatialiteDbInfoItem
// - for Item Column with number
// TODO: this (with mColumnNr) may be removed
// -- ---------------------------------- --
QgsSpatialiteDbInfoItem::QgsSpatialiteDbInfoItem( QgsSpatialiteDbInfoItem *parent, QString sText, int column )
{
  mParentItem = parent;
  mColumnNr = column;
  mItemType = ItemTypeColumn;
  setText( column, sText );
  if ( onInit() )
  {
  }
}
// -- ---------------------------------- --
// class QgsSpatialiteDbInfoItem
// - for Item Column with Icon
// TODO: this (with mColumnNr) may be removed
// -- ---------------------------------- --
QgsSpatialiteDbInfoItem::QgsSpatialiteDbInfoItem( const QIcon &icon, QgsSpatialiteDbInfoItem *parent, QString sText, int column )
{
  mParentItem = parent;
  mColumnNr = column;
  setIcon( column, icon );
  setText( column, sText );
  mItemType = ItemTypeColumn;
  if ( onInit() )
  {
  }
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::~QgsSpatialiteDbInfoItem
//-----------------------------------------------------------------
QgsSpatialiteDbInfoItem::~QgsSpatialiteDbInfoItem()
{
  qDeleteAll( mChildItems );
  if ( mDbLayer )
  {
    // Do not delete, may be used elsewhere
    mDbLayer = nullptr;
  }
  if ( mSpatialiteDbInfo )
  {
    // Do not delete, may be used elsewhere
    mSpatialiteDbInfo = nullptr;
  }
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::onInit
// Common Tasks:
// - Set initial Values, such as Column-Headers and Positions [mDbRootItem]
// - retrieve values from Parent
// Only when bRc=true, will certain (mItemType specific tasks) will be done
//-----------------------------------------------------------------
bool QgsSpatialiteDbInfoItem::onInit()
{
  bool bRc = false;
  // Qt::ItemFlags flags = ( Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled );
  Qt::ItemFlags flags = ( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
  if ( !mParentItem )
  {
    if ( mItemType == ItemTypeRoot )
    {
      // This is the mDbRootItem
      mColumnTable = 0;
      mColumnGeometryName = getTableNameIndex() + 1;
      mColumnGeometryType = getGeometryNameIndex() + 1;
      mColumnSql = getGeometryTypeIndex() + 1;
      mColumnSortHidden = getSqlQueryIndex() + 1; // Must be the last, since (mColumnSortHidden+1) == columnCount
      // The 'mColumn*' values must be set before setText and setFlags can be used
      setText( getTableNameIndex(), QStringLiteral( "Table" ) );
      setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( QgsSpatialiteDbInfo::SpatialTable ) );
      setAlignment( getTableNameIndex(), Qt::AlignCenter ); //Qt::AlignJustify
      setText( getGeometryNameIndex(), QStringLiteral( "Geometry-Column" ) );
      setAlignment( getGeometryNameIndex(), Qt::AlignCenter );
      setText( getGeometryTypeIndex(), QStringLiteral( "Geometry-Type" ) );
      setIcon( getGeometryTypeIndex(), QgsSpatialiteDbInfo::SpatialGeometryTypeIcon( QgsWkbTypes::Polygon ) );
      setAlignment( getGeometryTypeIndex(), Qt::AlignCenter );
      setText( getSqlQueryIndex(), QStringLiteral( "Sql" ) );
      setAlignment( getSqlQueryIndex(), Qt::AlignCenter );
      setText( getColumnSortHidden(), QStringLiteral( "ColumnSortHidden" ) );
      setFlags( -1, flags );
    }
  }
  else
  {
    // Retrieve Column-Number Information from Parent [from mDbRootItem]
    mColumnTable = mParentItem->getTableNameIndex();
    mColumnGeometryName = mParentItem->getGeometryNameIndex();
    mColumnGeometryType = mParentItem->getGeometryTypeIndex();
    mColumnSql = mParentItem->getSqlQueryIndex();
    mColumnSortHidden = mParentItem->getColumnSortHidden();
    // The 'mColumn*' values must be set before setText and setFlags can be used
    // Set default values for Columns [Role may be ItemType specific for Qt::EditRole]
    int iRole = Qt::EditRole;
    // -1: set all columns with the given Text and Role
    setText( -1, QStringLiteral( "" ), iRole );
    setText( getColumnSortHidden(), QStringLiteral( "" ) ); // allways Qt::EditRole
    // Retrieve SpatialiteDbInfo from Parent, if exists and not done allready [there can only be one]
    if ( !mSpatialiteDbInfo )
    {
      mSpatialiteDbInfo = mParentItem->getSpatialiteDbInfo();
    }
    switch ( mItemType )
    {
      //   Qt::ItemFlags flags = ( Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled );
      case ItemTypeHelpRoot:
      case ItemTypeHelpText:
      case ItemTypeUnknown:
        flags &= ~Qt::ItemIsSelectable;
        break;
      case ItemTypeDb:
      case ItemTypeLayer:
      case ItemTypeColumn:
      case ItemTypeAdmin:
        break;
      default:
        break;
    }
    // -1: set all columns with the given Flags
    setFlags( -1, flags );
    // forward the signal towards the root
    connectToChildItem( this );
  }
  if ( mDbLayer )
  {
    if ( !mSpatialiteDbInfo )
    {
      // Retrieve SpatialiteDbInfo from Parent, if exists and not done allready [there can only be one]
      mSpatialiteDbInfo = mDbLayer->getSpatialiteDbInfo();
    }
  }
  if ( mSpatialiteDbInfo )
  {
    bRc = true;
    if ( mItemType == ItemTypeDb )
    {
      // Determin amount of Layers, and Layer-Types
      sniffTableInfo();
      if ( mParentItem )
      {
        mParentItem->setTableCounter( getTableCounter() );
      }
    }
    else
    {
      if ( mParentItem )
      {
        mTableCounter = mParentItem->getTableCounter();
      }
    }
  }
  else
  {
    switch ( mItemType )
    {
      case ItemTypeHelpRoot:
      case ItemTypeHelpText:
        bRc = true;
        break;
      default:
        break;
    }
  }
  // QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem::onInit bRc[%1] columns[%2] TableCounter[%3] ItemType[%4]" ).arg( bRc ).arg( columnCount() ).arg( getTableCounter() ).arg( getItemType() ), 7 );
  if ( mParentItem )
  {
    switch ( mItemType )
    {
      case ItemTypeHelpRoot:
      case ItemTypeDb:
        // Adding this Item to the Parent
        mParentItem->addChild( this, mParentItem->childCount() );
        break;
      // Must be done later, with insertPrepairedChildren [tack overflow in thread #1: can't grow stack to 0x1ffe801000]
      case ItemTypeHelpText:
      case ItemTypeUnknown:
      default:
        break;
    }
  }
  return bRc;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::connectToChildItem()
//-----------------------------------------------------------------
// - Based on QgsLayerTreeNode::insertChildrenPrivate
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoItem::connectToChildItem( QgsSpatialiteDbInfoItem *node )
{
  // forward the signal towards the root
  connect( node, &QgsSpatialiteDbInfoItem::willAddChildren, this, &QgsSpatialiteDbInfoItem::willAddChildren );
  connect( node, &QgsSpatialiteDbInfoItem::addedChildren, this, &QgsSpatialiteDbInfoItem::addedChildren );
  connect( node, &QgsSpatialiteDbInfoItem::willRemoveChildren, this, &QgsSpatialiteDbInfoItem::willRemoveChildren );
  connect( node, &QgsSpatialiteDbInfoItem::removedChildren, this, &QgsSpatialiteDbInfoItem::removedChildren );
  // connect( node, &QgsSpatialiteDbInfoItem::customPropertyChanged, this, &QgsSpatialiteDbInfoItem::customPropertyChanged );
  // connect( node, &QgsSpatialiteDbInfoItem::visibilityChanged, this, &QgsSpatialiteDbInfoItem::visibilityChanged );
  // connect( node, &QgsSpatialiteDbInfoItem::expandedChanged, this, &QgsSpatialiteDbInfoItem::expandedChanged );
  // connect( node, &QgsSpatialiteDbInfoItem::nameChanged, this, &QgsSpatialiteDbInfoItem::nameChanged );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::sniffTableInfo
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoItem::sniffTableInfo()
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
  }
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::child
// - 0 based index
//-----------------------------------------------------------------
QgsSpatialiteDbInfoItem *QgsSpatialiteDbInfoItem::child( int number )
{
  if ( number >= childCount() )
  {
    number = childCount() - 1;
  }
  if ( number < 0 )
  {
    number = 0;
  }
  return mChildItems.at( number );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::childCount
// - 1 based index
//-----------------------------------------------------------------
int QgsSpatialiteDbInfoItem::childCount() const
{
  return mChildItems.count();
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::columnCount
//-----------------------------------------------------------------
int QgsSpatialiteDbInfoItem::columnCount() const
{
  return ( mColumnSortHidden + 1 );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::addChild
// Inserting 1 child to the Parent at given 0-based position
// Note: the value of position and indexTo are the same (from/to)
// - emit willAddChildren will crash when used [reason unknown]
// -> works without [also not used in sample 'editabletreemodel']
//-----------------------------------------------------------------
bool QgsSpatialiteDbInfoItem::addChild( QgsSpatialiteDbInfoItem *child, int position, bool bWillAddChildren )
{
  if ( ( position < 0 ) || ( position > childCount() ) )
  {
    position = row(); // Insert at end;
  }
  int indexTo = position;
  if ( bWillAddChildren )
  {
    // connectToChildItem( child );
    emit willAddChildren( this, position, indexTo );
  }
  mChildItems.insert( position, child );
  if ( bWillAddChildren )
  {
    emit addedChildren( this, position, indexTo );
  }
  return true;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::insertPrepairedChildren
// Adding Childrent to this node/item
//-----------------------------------------------------------------
bool QgsSpatialiteDbInfoItem::insertPrepairedChildren()
{
  if ( mPrepairedChildItems.count() < 1 )
  {
    return false;
  }
  bool bWillAddChildren = false;
  int indexFrom = row(); // Insert at end
  int iPosition = indexFrom;
  for ( int i = 0; i < mPrepairedChildItems.count(); i++ )
  {
    QgsSpatialiteDbInfoItem *child = mPrepairedChildItems.at( i );
    if ( child )
    {
      addChild( child, ++iPosition, bWillAddChildren );
    }
  }
  mPrepairedChildItems.clear();
  return true;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::insertChildren
// this may not be needed
//-----------------------------------------------------------------
bool QgsSpatialiteDbInfoItem::insertChildren( int position, int count, SpatialiteDbInfoItemType itemType, int columns )
{
  if ( ( position < 0 ) || ( position > childCount() ) )
  {
    position = childCount();
  }
  if ( ( columns < 0 ) || ( columns > columnCount() ) )
  {
    columns = columnCount();
  }
  switch ( itemType )
  {
    case ItemTypeUnknown:
      break;
    // Type that call addChild in onInit, should not be supported
    case ItemTypeHelpRoot:
    case ItemTypeDb:
    default:
      itemType = ItemTypeUnknown;
      break;
  }
  int indexTo = ( position + count ) - 1;
  emit willAddChildren( this, position, indexTo );
  // TODO loop through layers and send
  for ( int row = 0; row < count; ++row )
  {
    QgsSpatialiteDbInfoItem *child = new QgsSpatialiteDbInfoItem( this, itemType );
    // connectToChildItem( child );
    mChildItems.insert( position, child );
  }
  emit addedChildren( this, position, indexTo );
  return true;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::insertColumns
//-----------------------------------------------------------------
bool QgsSpatialiteDbInfoItem::insertColumns( int position, int columns )
{
  if ( position < 0 || position > mItemData.size() )
  {
    return false;
  }
  for ( int column = 0; column < columns; ++column )
  {
    setText( column, QString() );
  }
  foreach ( QgsSpatialiteDbInfoItem *child, mChildItems )
  {
    child->insertColumns( position, columns );
  }
  return true;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::removeChildren
// - Children will be deleted
//-----------------------------------------------------------------
bool QgsSpatialiteDbInfoItem::removeChildren( int position, int count )
{
  QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem::removeChildren Item[type=%1,children=%2] position[%3] count[%4]" ).arg( getItemType() ).arg( childCount() ).arg( position ).arg( count ), 7 );
  if ( position < 0 || position + count > mChildItems.size() )
  {
    return false;
  }
  for ( int row = 0; row < count; ++row )
  {
    delete mChildItems.takeAt( position );
  }
  return true;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::removeColumns
//-----------------------------------------------------------------
bool QgsSpatialiteDbInfoItem::removeColumns( int position, int columns )
{
  if ( position < 0 || position + columns > mItemData.size() )
  {
    return false;
  }
  for ( int column = 0; column < columns; ++column )
  {
    mItemData.remove( position );
  }
  foreach ( QgsSpatialiteDbInfoItem *child, mChildItems )
  {
    child->removeColumns( position, columns );
  }
  return true;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::setData
// - Emulating QStandardItem
//-----------------------------------------------------------------
bool QgsSpatialiteDbInfoItem::setData( int column, const QVariant &value, int role )
{
  bool bRc = false; // when true dataChanged should be omitted
  if ( ( column < 0 ) || ( column >= columnCount() ) )
  {
    column = 0;
  }
  // Change any Qt::DisplayRole to Qt::EditRole
  role = ( role == Qt::EditRole ) ? Qt::DisplayRole : role;
  QVector<QgsSpatialiteDbInfoData>::iterator it;
  for ( it = mItemData.begin(); it != mItemData.end(); ++it )
  {
    // Replacing, if found
    if ( ( ( *it ).role == role ) && ( ( *it ).column == column ) )
    {
      if ( value.isValid() )
      {
        if ( ( *it ).value.type() == value.type() && ( *it ).value == value )
        {
          return bRc; // Value has not changed
        }
#if 0
        if ( ( value.typeName() == QString( "QString" ) ) || ( value.typeName() == QString( "int" ) ) )
        {
          QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem::setData -changed- column[%1,type=%2] role[%3] Value-Type[%5] Value-String[%4] " ).arg( column ).arg( mItemType ).arg( role ).arg( value.toString() ).arg( value.typeName() ), 7 );
        }
        else
        {
          QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem::setData -changed- column[%1,type=%2] role[%3] Value-Type[%4]" ).arg( column ).arg( mItemType ).arg( role ).arg( value.typeName() ), 7 );
        }
#endif
        ( *it ).value = value;
      }
      else
      {
        mItemData.erase( it );
      }
      bRc = true;
      return bRc;
    }
  }
  // Adding
  mItemData.append( QgsSpatialiteDbInfoData( role, value, column ) );
#if 0
  if ( ( value.typeName() == QString( "QString" ) ) || ( value.typeName() == QString( "int" ) ) )
  {
    if ( !value.toString().isEmpty() )
    {
      QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem::setData -added- column[%1,type=%2] role[%3] Value-Type[%5] Value-String[%4] " ).arg( column ).arg( mItemType ).arg( role ).arg( value.toString() ).arg( value.typeName() ), 7 );
    }
  }
  else
  {
    QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem::setData -added- column[%1,type=%2] role[%3] Value-Type[%4]" ).arg( column ).arg( mItemType ).arg( role ).arg( value.typeName() ), 7 );
  }
#endif
  bRc = true;
  return bRc;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::data
// - Emulating QStandardItem
// Used by:
// - text(), icon(), flags()
//-----------------------------------------------------------------
QVariant QgsSpatialiteDbInfoItem::data( int column, int role ) const
{
  if ( ( column < 0 ) || ( column >= columnCount() ) )
  {
    column = 0;
  }
  if ( ( column >= 0 ) && ( column < columnCount() ) )
  {
    // Change any Qt::DisplayRole to Qt::EditRole
    role = ( role == Qt::EditRole ) ? Qt::DisplayRole : role;
    QVector<QgsSpatialiteDbInfoData>::const_iterator it;
    for ( it = mItemData.begin(); it != mItemData.end(); ++it )
    {
      if ( ( *it ).role == role )
      {
        if ( ( *it ).column == column )
        {
          switch ( role )
          {
            case Qt::DisplayRole:
            case Qt::DecorationRole:
            case Qt::EditRole:
            case Qt::ToolTipRole:
            case Qt::StatusTipRole:
            case Qt::TextAlignmentRole:
            case Qt::BackgroundRole:
            case Qt::ForegroundRole:
            // flags
            case ( Qt::UserRole-1 ):
            {
              // 0=Qt::DisplayRole ,1=Qt::DecorationRole, 2=Qt::EditRole, 3=Qt::ToolTipRole, 4=Qt::StatusTipRole
              // 7=Qt::TextAlignmentRole, 8=Qt::BackgroundRole,9=Qt::ForegroundRole, 255=Qt::UserRole-1
              return ( *it ).value;
            }
            break;
            case Qt::FontRole:
            case Qt::CheckStateRole:
            case Qt::SizeHintRole:
            default:
              // 6=Qt::FontRole, 10=Qt::CheckStateRole,13=Qt::SizeHintRole
              break;
          }
        }
      }
    }
  }
  return QVariant();
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::setFlags
// - Emulating QStandardItem
// Used by:
// - setText, setIcon,setEnabled,setEditable,setSelectable
// - when column < 0: all columns will be set with the given value
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoItem::setFlags( int column, Qt::ItemFlags flags )
{
  if ( column < 0 )
  {
    // Insure all colums have a flag [called in onInit()]
    for ( column = 0; column < columnCount(); column++ )
    {
      setData( column, ( int )flags, Qt::UserRole - 1 );
    }
    return;
  }
  setData( column, ( int )flags, Qt::UserRole - 1 );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::setText
// - Emulating QStandardItem
// - when column < 0: all columns will be set with the given value
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoItem::setText( int column, const QString &sText, int role )
{
  if ( column < 0 )
  {
    // Insure all columns have a text [called in onInit()]
    for ( column = 0; column < columnCount(); column++ )
    {
      setData( column, sText, role );
    }
    return;
  }
  setData( column, sText, role );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::setFlags
// - Emulating QStandardItem
// Used by:
// - isEnabled,isEditable,isSelectable
// -> calling them here causes a loop
//-----------------------------------------------------------------
Qt::ItemFlags QgsSpatialiteDbInfoItem::flags( int column )  const
{
  QVariant v = data( column, Qt::UserRole - 1 );
  if ( !v.isValid() )
  {
    // Note: this should no loger happen, since setFlags(-1,...) is called in onInit()
    // 1=Qt::ItemIsSelectable, 2=Qt::ItemIsEditable, 32=Qt::ItemIsEnabled
    // 4=Qt::ItemIsDragEnabled, 8=Qt::ItemIsDropEnabled
    Qt::ItemFlags itemFlags = ( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
#if 1
    bool bEditable = ( itemFlags & Qt::ItemIsEditable ) != 0;
    bool bSelectable = ( itemFlags & Qt::ItemIsSelectable ) != 0;
    bool bEnabled = ( itemFlags & Qt::ItemIsEnabled ) != 0;
    QgsDebugMsgLevel( QString( "-W-> QgsSpatialiteDbInfoItem::flags(%1,isEditable=%4,isSelectable=%5,isEnabled=%6) item.row/column[%2,%3]" ).arg( itemFlags ).arg( row() ).arg( column ).arg( bEditable ).arg( bSelectable ).arg( bEnabled ), 7 );
#endif
    return  itemFlags;
  }
  Qt::ItemFlags itemFlags = Qt::ItemFlags( v.toInt() );
#if 0
  bool bEditable = ( itemFlags & Qt::ItemIsEditable ) != 0;
  bool bSelectable = ( itemFlags & Qt::ItemIsSelectable ) != 0;
  bool bEnabled = ( itemFlags & Qt::ItemIsEnabled ) != 0;
  QgsDebugMsgLevel( QString( "-I-> QgsSpatialiteDbInfoItem::flags(%1,isEditable=%4,isSelectable=%5,isEnabled=%6) item.row/column[%2,%3]" ).arg( itemFlags ).arg( row() ).arg( column ).arg( bEditable ).arg( bSelectable ).arg( bEnabled ), 7 );
#endif
  return  itemFlags;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::setEnabled
// - Emulating QStandardItem
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoItem::setEnabled( int column, bool value )
{
  Qt::ItemFlags flag = flags( column );
  if ( value )
  {
    flag |= Qt::ItemIsEditable;
  }
  else
  {
    flag &= ~Qt::ItemIsEditable;
  }
  setFlags( column, flag );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::setEditable
// - Emulating QStandardItem
// Item-Column must be Enabled to be Editable
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoItem::setEditable( int column, bool value )
{
  Qt::ItemFlags flag = flags( column );
  if ( value )
  {
    if ( !isEnabled( column ) )
    {
      setEnabled( column, true );
    }
    flag |= Qt::ItemIsEditable;
  }
  else
  {
    flag &= ~Qt::ItemIsEditable;
  }
  setFlags( column, flag );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::setSelectable
// - Emulating QStandardItem
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoItem::setSelectable( int column, bool value )
{
  Qt::ItemFlags flag = flags( column );
  if ( value )
  {
    flag |= Qt::ItemIsSelectable;
  }
  else
  {
    flag &= ~Qt::ItemIsSelectable;
  }
  setFlags( column, flag );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::childNumber
//-----------------------------------------------------------------
int QgsSpatialiteDbInfoItem::childNumber() const
{
  if ( mParentItem )
  {
    return mParentItem->mChildItems.indexOf( const_cast<QgsSpatialiteDbInfoItem *>( this ) );
  }
  return 0;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::position
// - Emulating QStandardItem
//-----------------------------------------------------------------
QPair<int, int> QgsSpatialiteDbInfoItem::position() const
{
  if ( parent() )
  {
    // int idx = parent()->childNumber(); // in Grand-Parent
    int idx = childNumber(); // within parent plus position Grand-Parent
    if ( idx == -1 )
    {
      return QPair<int, int>( -1, -1 );
    }
    // QgsSpatialiteDbInfoItem contains the column in 1 Item and not an Item for each column
    return QPair<int, int>( idx, parent()->columnCount() );
    // return QPair<int, int>( idx / parent()->columnCount(), idx % parent()->columnCount() );
  }
  // ### support header items?
  return QPair<int, int>( -1, -1 );
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::addHelpInfo
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoItem::addHelpInfo()
{
  // Create text for Help
  // Column 0: getTableNameIndex()
  int iSortCounter = 0;
  QString sText = QStringLiteral( "Help text for QgsSpatiaLiteSourceSelect" );
  setText( getTableNameIndex(), sText );
  setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialMetadataTypeIcon( QgsSpatialiteDbInfo::Spatialite45 ) );
  sText = QStringLiteral( "Browser for sqlite3 containers supported by" );
  // Column 1: getGeometryNameIndex()
  setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Spatialite, RasterLite2, Gdal and Ogr Providers" );
  setText( getGeometryTypeIndex(), sText );
  QString sSortTag = QStringLiteral( "AAAA_%1" ).arg( iSortCounter++ );
  setText( getColumnSortHidden(), sSortTag );
  // new Child to be added to Help Text
  QgsSpatialiteDbInfoItem *dbHelpItem = new QgsSpatialiteDbInfoItem( this, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  // Column 0: getTableNameIndex()
  sText = QStringLiteral( "Listing of Items" );
  dbHelpItem->setText( getTableNameIndex(), sText );
  // Column 1: getGeometryNameIndex()
  sText = QStringLiteral( "Tables/Views will be listed in Sub-Groups" );
  dbHelpItem->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "A Sub-Group will only be shown when valid entries exist" );
  dbHelpItem->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACA_%1" ).arg( iSortCounter++ );
  dbHelpItem->setText( getColumnSortHidden(), sSortTag );
  QgsSpatialiteDbInfoItem *dbHelpSubGroups = new QgsSpatialiteDbInfoItem( dbHelpItem, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  // Column 0: getTableNameIndex()
  sText = QStringLiteral( "Sub-Groups" );
  dbHelpSubGroups->setText( getTableNameIndex(), sText );
  sText = QStringLiteral( "based on Provider-Type" );
  dbHelpSubGroups->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Listing the Tables/Views togeather, with the amount found" );
  dbHelpSubGroups->setText( getGeometryTypeIndex(), sText );
  iSortCounter = 0;
  sSortTag = QStringLiteral( "AACA_%1" ).arg( iSortCounter++ );
  dbHelpSubGroups->setText( getColumnSortHidden(), sSortTag );
  QgsSpatialiteDbInfoItem *dbHelpSubGroup = new QgsSpatialiteDbInfoItem( dbHelpSubGroups, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "SpatialTables" );
  dbHelpSubGroup->setText( getTableNameIndex(), sText );
  dbHelpSubGroup->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( QgsSpatialiteDbInfo::SpatialTable ) );
  sText = QStringLiteral( "Spatialite and RasterLite2" );
  dbHelpSubGroup->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Showing the Table/Geometry-Names" );
  dbHelpSubGroup->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACA_%1" ).arg( iSortCounter++ );
  dbHelpSubGroup->setText( getColumnSortHidden(), sSortTag );
  // Add Sub-Group to SubGroups
  dbHelpSubGroups->addChild( dbHelpSubGroup );
  dbHelpSubGroup = new QgsSpatialiteDbInfoItem( dbHelpSubGroups, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "SpatialViews" );
  dbHelpSubGroup->setText( getTableNameIndex(), sText );
  dbHelpSubGroup->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( QgsSpatialiteDbInfo::SpatialView ) );
  sText = QStringLiteral( "Spatialite, RasterLite2 and RasterLite1" );
  dbHelpSubGroup->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Showing the View/Geometry-Names" );
  dbHelpSubGroup->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACA_%1" ).arg( iSortCounter++ );
  dbHelpSubGroup->setText( getColumnSortHidden(), sSortTag );
  // Add Sub-Group to SubGroups
  dbHelpSubGroups->addChild( dbHelpSubGroup );
  dbHelpSubGroup = new QgsSpatialiteDbInfoItem( dbHelpSubGroups, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "VirtualShapes" );
  dbHelpSubGroup->setText( getTableNameIndex(), sText );
  dbHelpSubGroup->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( QgsSpatialiteDbInfo::VirtualShape ) );
  sText = QStringLiteral( "Spatialite, RasterLite2" );
  dbHelpSubGroup->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Showing the VirtualShape-Names" );
  dbHelpSubGroup->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACA_%1" ).arg( iSortCounter++ );
  dbHelpSubGroup->setText( getColumnSortHidden(), sSortTag );
  // Add Sub-Group to SubGroups
  dbHelpSubGroups->addChild( dbHelpSubGroup );
  dbHelpSubGroup = new QgsSpatialiteDbInfoItem( dbHelpSubGroups, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "RasterLite2Rasters" );
  dbHelpSubGroup->setText( getTableNameIndex(), sText );
  dbHelpSubGroup->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( QgsSpatialiteDbInfo::RasterLite2Raster ) );
  sText = QStringLiteral( "RasterLite2" );
  dbHelpSubGroup->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Showing the RasterCoverage-Names" );
  dbHelpSubGroup->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACA_%1" ).arg( iSortCounter++ );
  dbHelpSubGroup->setText( getColumnSortHidden(), sSortTag );
  // Add Sub-Group to SubGroups
  dbHelpSubGroups->addChild( dbHelpSubGroup );
  dbHelpSubGroup = new QgsSpatialiteDbInfoItem( dbHelpSubGroups, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "GeoPackageVectors" );
  dbHelpSubGroup->setText( getTableNameIndex(), sText );
  dbHelpSubGroup->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( QgsSpatialiteDbInfo::GeoPackageVector ) );
  sText = QStringLiteral( "GeoPackage" );
  dbHelpSubGroup->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Showing the Table/Geometry-Names" );
  dbHelpSubGroup->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACA_%1" ).arg( iSortCounter++ );
  dbHelpSubGroup->setText( getColumnSortHidden(), sSortTag );
  // Add Sub-Group to SubGroups
  dbHelpSubGroups->addChild( dbHelpSubGroup );
  dbHelpSubGroup = new QgsSpatialiteDbInfoItem( dbHelpSubGroups, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "GeoPackageRasters" );
  dbHelpSubGroup->setText( getTableNameIndex(), sText );
  dbHelpSubGroup->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( QgsSpatialiteDbInfo::GeoPackageRaster ) );
  sText = QStringLiteral( "GeoPackage" );
  dbHelpSubGroup->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Showing the Table-Names" );
  dbHelpSubGroup->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACA_%1" ).arg( iSortCounter++ );
  dbHelpSubGroup->setText( getColumnSortHidden(), sSortTag );
  // Add Sub-Group to SubGroups
  dbHelpSubGroups->addChild( dbHelpSubGroup );
  dbHelpSubGroup = new QgsSpatialiteDbInfoItem( dbHelpSubGroups, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "OgrFdo" );
  dbHelpSubGroup->setText( getTableNameIndex(), sText );
  dbHelpSubGroup->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( QgsSpatialiteDbInfo::GdalFdoOgr ) );
  sText = QStringLiteral( "Ogr" );
  dbHelpSubGroup->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Showing the Table/Geometry-Names" );
  dbHelpSubGroup->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACA_%1" ).arg( iSortCounter++ );
  dbHelpSubGroup->setText( getColumnSortHidden(), sSortTag );
  // Add Sub-Group to SubGroups
  dbHelpSubGroups->addChild( dbHelpSubGroup );
  dbHelpSubGroup = new QgsSpatialiteDbInfoItem( dbHelpSubGroups, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "MBTiles" );
  dbHelpSubGroup->setText( getTableNameIndex(), sText );
  dbHelpSubGroup->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( QgsSpatialiteDbInfo::MBTilesTable ) );
  sText = QStringLiteral( "Gdal" );
  dbHelpSubGroup->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Showing the value entered in metadata.name" );
  dbHelpSubGroup->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACA_%1" ).arg( iSortCounter++ );
  dbHelpSubGroup->setText( getColumnSortHidden(), sSortTag );
  // Add Sub-Group to SubGroups
  dbHelpSubGroups->addChild( dbHelpSubGroup );
  dbHelpSubGroup = new QgsSpatialiteDbInfoItem( dbHelpSubGroups, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "RasterLite1" );
  dbHelpSubGroup->setText( getTableNameIndex(), sText );
  dbHelpSubGroup->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( QgsSpatialiteDbInfo::RasterLite1 ) );
  sText = QStringLiteral( "Gdal" );
  dbHelpSubGroup->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Showing the RasterCoverage-Names" );
  dbHelpSubGroup->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACA_%1" ).arg( iSortCounter++ );
  dbHelpSubGroup->setText( getColumnSortHidden(), sSortTag );
  // Add Sub-Group to SubGroups
  dbHelpSubGroups->addChild( dbHelpSubGroup );
  dbHelpSubGroup = new QgsSpatialiteDbInfoItem( dbHelpSubGroups, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "NonSpatialTables" );
  dbHelpSubGroup->setText( getTableNameIndex(), sText );
  dbHelpSubGroup->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( QgsSpatialiteDbInfo::NonSpatialTables ) );
  sText = QStringLiteral( "with Data, Administration Tables" );
  dbHelpSubGroup->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "These entries will always be shown, when any exist and are Provider specific" );
  dbHelpSubGroup->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACB_%1" ).arg( iSortCounter++ );
  dbHelpSubGroup->setText( getColumnSortHidden(), sSortTag );
  QgsSpatialiteDbInfoItem *dbHelpNonSpatialTable = new QgsSpatialiteDbInfoItem( dbHelpSubGroup, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "Table-Data" );
  dbHelpNonSpatialTable->setText( getTableNameIndex(), sText );
  dbHelpNonSpatialTable->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::NonSpatialTablesTypeIcon( sText ) );
  sText = QStringLiteral( "non-Spatiale Tables/Views" );
  dbHelpNonSpatialTable->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Showing the Table-Names" );
  dbHelpNonSpatialTable->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACC_%1" ).arg( iSortCounter++ );
  dbHelpNonSpatialTable->setText( getColumnSortHidden(), sSortTag );
  // Add NonSpatialTable to SubGroup NonSpatialTables
  dbHelpSubGroup->addChild( dbHelpNonSpatialTable );
  dbHelpNonSpatialTable = new QgsSpatialiteDbInfoItem( dbHelpSubGroup, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "RasterLite2-Metadata" );
  dbHelpNonSpatialTable->setText( getTableNameIndex(), sText );
  dbHelpNonSpatialTable->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::NonSpatialTablesTypeIcon( sText ) );
  sText = QStringLiteral( "non-Spatial Tables/Views" );
  dbHelpNonSpatialTable->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Administration Tables" );
  dbHelpNonSpatialTable->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACC_%1" ).arg( iSortCounter++ );
  dbHelpNonSpatialTable->setText( getColumnSortHidden(), sSortTag );
  // Add NonSpatialTable to SubGroup NonSpatialTables
  dbHelpSubGroup->addChild( dbHelpNonSpatialTable );
  dbHelpNonSpatialTable = new QgsSpatialiteDbInfoItem( dbHelpSubGroup, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "RasterLite2-Tiles" );
  dbHelpNonSpatialTable->setText( getTableNameIndex(), sText );
  dbHelpNonSpatialTable->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::NonSpatialTablesTypeIcon( sText ) );
  sText = QStringLiteral( "Tile-Tables" );
  dbHelpNonSpatialTable->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Tables containing Raster-Tiles" );
  dbHelpNonSpatialTable->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACC_%1" ).arg( iSortCounter++ );
  dbHelpNonSpatialTable->setText( getColumnSortHidden(), sSortTag );
  // Add NonSpatialTable to SubGroup NonSpatialTables
  dbHelpSubGroup->addChild( dbHelpNonSpatialTable );
  dbHelpNonSpatialTable = new QgsSpatialiteDbInfoItem( dbHelpSubGroup, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "RasterLite1-Metadata" );
  dbHelpNonSpatialTable->setText( getTableNameIndex(), sText );
  dbHelpNonSpatialTable->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::NonSpatialTablesTypeIcon( sText ) );
  sText = QStringLiteral( "non-Spatial Tables/Views" );
  dbHelpNonSpatialTable->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Administration Tables" );
  dbHelpNonSpatialTable->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACC_%1" ).arg( iSortCounter++ );
  dbHelpNonSpatialTable->setText( getColumnSortHidden(), sSortTag );
  // Add NonSpatialTable to SubGroup NonSpatialTables
  dbHelpSubGroup->addChild( dbHelpNonSpatialTable );
  dbHelpNonSpatialTable = new QgsSpatialiteDbInfoItem( dbHelpSubGroup, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "GeoPackage" );
  dbHelpNonSpatialTable->setText( getTableNameIndex(), sText );
  dbHelpNonSpatialTable->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::NonSpatialTablesTypeIcon( sText ) );
  sText = QStringLiteral( "non-Spatial Tables/Views" );
  dbHelpNonSpatialTable->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Administration Tables" );
  dbHelpNonSpatialTable->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACC_%1" ).arg( iSortCounter++ );
  dbHelpNonSpatialTable->setText( getColumnSortHidden(), sSortTag );
  dbHelpNonSpatialTable = new QgsSpatialiteDbInfoItem( dbHelpSubGroup, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "MBTiles" );
  dbHelpNonSpatialTable->setText( getTableNameIndex(), sText );
  dbHelpNonSpatialTable->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::NonSpatialTablesTypeIcon( sText ) );
  sText = QStringLiteral( "non-Spatial Tables/Views" );
  dbHelpNonSpatialTable->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Administration Tables" );
  dbHelpNonSpatialTable->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACC_%1" ).arg( iSortCounter++ );
  dbHelpNonSpatialTable->setText( getColumnSortHidden(), sSortTag );
  // Add NonSpatialTable to SubGroup NonSpatialTables
  dbHelpSubGroup->addChild( dbHelpNonSpatialTable );
  dbHelpNonSpatialTable = new QgsSpatialiteDbInfoItem( dbHelpSubGroup, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "RasterLite1-Tiles" );
  dbHelpNonSpatialTable->setText( getTableNameIndex(), sText );
  dbHelpNonSpatialTable->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::NonSpatialTablesTypeIcon( sText ) );
  sText = QStringLiteral( "Tile-Tables" );
  dbHelpNonSpatialTable->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Tables containing Raster-Tiles" );
  dbHelpNonSpatialTable->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACC_%1" ).arg( iSortCounter++ );
  dbHelpNonSpatialTable->setText( getColumnSortHidden(), sSortTag );
  dbHelpNonSpatialTable = new QgsSpatialiteDbInfoItem( dbHelpSubGroup, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "Data" );
  dbHelpNonSpatialTable->setText( getTableNameIndex(), sText );
  dbHelpNonSpatialTable->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::NonSpatialTablesTypeIcon( sText ) );
  sText = QStringLiteral( "non-Spatial Tables/Views" );
  dbHelpNonSpatialTable->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "used as Administration Tables" );
  dbHelpNonSpatialTable->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACC_%1" ).arg( iSortCounter++ );
  dbHelpNonSpatialTable->setText( getColumnSortHidden(), sSortTag );
  // Add NonSpatialTable to SubGroup NonSpatialTables
  dbHelpSubGroup->addChild( dbHelpNonSpatialTable );
  dbHelpNonSpatialTable = new QgsSpatialiteDbInfoItem( dbHelpSubGroup, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "Spatialite" );
  dbHelpNonSpatialTable->setText( getTableNameIndex(), sText );
  dbHelpNonSpatialTable->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( QgsSpatialiteDbInfo::SpatialTable ) );
  sText = QStringLiteral( "internal Administration Tables" );
  dbHelpNonSpatialTable->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "without SpatialIndex" );
  dbHelpNonSpatialTable->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACC_%1" ).arg( iSortCounter++ );
  dbHelpNonSpatialTable->setText( getColumnSortHidden(), sSortTag );
  // Add NonSpatialTable to SubGroup NonSpatialTables
  dbHelpSubGroup->addChild( dbHelpNonSpatialTable );
  dbHelpNonSpatialTable = new QgsSpatialiteDbInfoItem( dbHelpSubGroup, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "SpatialIndex" );
  dbHelpNonSpatialTable->setText( getTableNameIndex(), sText );
  dbHelpNonSpatialTable->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::NonSpatialTablesTypeIcon( sText ) );
  sText = QStringLiteral( "index Tables" );
  dbHelpNonSpatialTable->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "for SpatialIndex" );
  dbHelpNonSpatialTable->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACC_%1" ).arg( iSortCounter++ );
  dbHelpNonSpatialTable->setText( getColumnSortHidden(), sSortTag );
  // Add NonSpatialTable to SubGroup NonSpatialTables
  dbHelpSubGroup->addChild( dbHelpNonSpatialTable );
  dbHelpNonSpatialTable = new QgsSpatialiteDbInfoItem( dbHelpSubGroup, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "Geometries-Admin" );
  dbHelpNonSpatialTable->setText( getTableNameIndex(), sText );
  dbHelpNonSpatialTable->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::NonSpatialTablesTypeIcon( sText ) );
  sText = QStringLiteral( "internal Administration Tables" );
  dbHelpNonSpatialTable->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "for FdoOgr" );
  dbHelpNonSpatialTable->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACC_%1" ).arg( iSortCounter++ );
  dbHelpNonSpatialTable->setText( getColumnSortHidden(), sSortTag );
  // Add NonSpatialTable to SubGroup NonSpatialTables
  dbHelpSubGroup->addChild( dbHelpNonSpatialTable );
  dbHelpNonSpatialTable = new QgsSpatialiteDbInfoItem( dbHelpSubGroup, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "Sqlite3" );
  dbHelpNonSpatialTable->setText( getTableNameIndex(), sText );
  dbHelpNonSpatialTable->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::NonSpatialTablesTypeIcon( sText ) );
  sText = QStringLiteral( "internal Administration Tables" );
  dbHelpNonSpatialTable->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "for sqlite3" );
  dbHelpNonSpatialTable->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACC_%1" ).arg( iSortCounter++ );
  dbHelpNonSpatialTable->setText( getColumnSortHidden(), sSortTag );
  // Add NonSpatialTable to SubGroup NonSpatialTables
  dbHelpSubGroup->addChild( dbHelpNonSpatialTable );
  dbHelpNonSpatialTable = new QgsSpatialiteDbInfoItem( dbHelpSubGroup, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  sText = QStringLiteral( "EPSG-Admin" );
  dbHelpNonSpatialTable->setText( getTableNameIndex(), sText );
  dbHelpNonSpatialTable->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::NonSpatialTablesTypeIcon( sText ) );
  sText = QStringLiteral( "Spatial-Reference Tables" );
  dbHelpNonSpatialTable->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "for Spatialite, FdoOgr, GeoPackage" );
  dbHelpNonSpatialTable->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AACC_%1" ).arg( iSortCounter++ );
  dbHelpNonSpatialTable->setText( getColumnSortHidden(), sSortTag );
  // Add NonSpatialTable to SubGroup NonSpatialTables
  dbHelpSubGroup->addChild( dbHelpNonSpatialTable );
  // Add Sub-Group to SubGroups
  dbHelpSubGroups->addChild( dbHelpSubGroup );
  // Add Sub-Group to the main Item
  dbHelpItem->addChild( dbHelpSubGroups );
  // Column 3 [not used]: getSqlQueryIndex()
  // Add Text about Provider [will be shown as first child]
  mPrepairedChildItems.append( dbHelpItem );
  // Column 3 [not used]: getSqlQueryIndex()
  // new Child to be added to Help Text
  dbHelpItem = new QgsSpatialiteDbInfoItem( this, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  // Column 0: getTableNameIndex()
  sText = QStringLiteral( "Providers" );
  dbHelpItem->setText( getTableNameIndex(), sText );
  iSortCounter = 0;
  sSortTag = QStringLiteral( "AABA_%1" ).arg( iSortCounter++ );
  dbHelpItem->setText( getColumnSortHidden(), sSortTag );
  QgsSpatialiteDbInfoItem *dbHelpProviders = new QgsSpatialiteDbInfoItem( dbHelpItem, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  // Column 0: getTableNameIndex()
  sText = QStringLiteral( "Spatialite" );
  dbHelpProviders->setText( getTableNameIndex(), sText );
  dbHelpProviders->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialMetadataTypeIcon( QgsSpatialiteDbInfo::Spatialite45 ) );
  dbHelpProviders->setBackground( getTableNameIndex(), QColor( "mediumaquamarine" ) );
#ifndef QT_NO_TOOLTIP
  dbHelpProviders->setToolTip( getTableNameIndex(), QString( "QgsSpatialiteProvider" ) );
#endif
  // Column 1: getGeometryNameIndex()
  sText = QStringLiteral( "Versions 2.4, 3.* [Legacy] until present" );
  dbHelpProviders->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Vectors (Geometries)" );
  dbHelpProviders->setText( getGeometryTypeIndex(), sText );
  dbHelpProviders->setIcon( getGeometryTypeIndex(), QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( QgsSpatialiteDbInfo::SpatialTable ) );
  sSortTag = QStringLiteral( "AABA_%1" ).arg( iSortCounter++ );
  dbHelpProviders->setText( getColumnSortHidden(), sSortTag );
  // Column 3 [not used]: getSqlQueryIndex()
  dbHelpItem->addChild( dbHelpProviders );
  // new Child to be added to Provider
  dbHelpProviders = new QgsSpatialiteDbInfoItem( dbHelpItem, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  // Column 0: getTableNameIndex()
  sText = QStringLiteral( "RasterLite2" );
  dbHelpProviders->setText( getTableNameIndex(), sText );
  dbHelpProviders->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialMetadataTypeIcon( QgsSpatialiteDbInfo::Spatialite45 ) );
  dbHelpProviders->setBackground( getTableNameIndex(), QColor( "lightyellow" ) );
#ifndef QT_NO_TOOLTIP
  dbHelpProviders->setToolTip( getTableNameIndex(), QString( "QgsRasterite2Provider" ) );
#endif
  // Column 1: getGeometryNameIndex()
  sText = QStringLiteral( "Versions supported by QgsRasterite2Provider" );
  dbHelpProviders->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Rasters" );
  dbHelpProviders->setText( getGeometryTypeIndex(), sText );
  dbHelpProviders->setIcon( getGeometryTypeIndex(), QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( QgsSpatialiteDbInfo::RasterLite2Raster ) );
  sSortTag = QStringLiteral( "AABA_%1" ).arg( iSortCounter++ );
  dbHelpProviders->setText( getColumnSortHidden(), sSortTag );
  // Column 3 [not used]: getSqlQueryIndex()
  dbHelpItem->addChild( dbHelpProviders );
  // new Child to be added to Provider
  dbHelpProviders = new QgsSpatialiteDbInfoItem( dbHelpItem, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  // Column 0: getTableNameIndex()
  sText = QStringLiteral( "RasterLite1, GeoPackage-Rasters, MBTiles" );
  dbHelpProviders->setText( getTableNameIndex(), sText );
  dbHelpProviders->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialMetadataTypeIcon( QgsSpatialiteDbInfo::SpatialiteGpkg ) );
  dbHelpProviders->setBackground( getTableNameIndex(), QColor( "khaki" ) );
#ifndef QT_NO_TOOLTIP
  dbHelpProviders->setToolTip( getTableNameIndex(), QString( "QgsGdalProvider" ) );
  dbHelpProviders->setToolTip( getGeometryNameIndex(), QString( "The structure will be shown, even if the if the needed driver is not active and cannot be shown" ) );
  dbHelpProviders->setToolTip( getGeometryTypeIndex(), QString( "A message will be shown with the name of the needed driver, if it is active and can be shown" ) );
#endif
  // Column 1: getGeometryNameIndex()
  sText = QStringLiteral( "Versions supported by QgsGdalProvider" );
  dbHelpProviders->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Rasters, when the needed Driver is installed" );
  dbHelpProviders->setText( getGeometryTypeIndex(), sText );
  dbHelpProviders->setIcon( getGeometryTypeIndex(), QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( QgsSpatialiteDbInfo::RasterLite2Raster ) );
  dbHelpProviders->setBackground( getGeometryTypeIndex(), QColor( "indianred" ) );
  dbHelpProviders->setForeground( getGeometryTypeIndex(), QColor( "white" ) );
  // Column 3 [not used]: getSqlQueryIndex()
  sSortTag = QStringLiteral( "AABA_%1" ).arg( iSortCounter++ );
  dbHelpProviders->setText( getColumnSortHidden(), sSortTag );
  dbHelpItem->addChild( dbHelpProviders );
  // new Child to be added to Provider
  dbHelpProviders = new QgsSpatialiteDbInfoItem( dbHelpItem, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  // Column 0: getTableNameIndex()
  sText = QStringLiteral( "FdoOgr, GeoPackage-Vectors" );
  dbHelpProviders->setText( getTableNameIndex(), sText );
  dbHelpProviders->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialMetadataTypeIcon( QgsSpatialiteDbInfo::SpatialiteFdoOgr ) );
  dbHelpProviders->setBackground( getTableNameIndex(), QColor( "powderblue" ) );
#ifndef QT_NO_TOOLTIP
  dbHelpProviders->setToolTip( getTableNameIndex(), QString( "QgsOgrProvider" ) );
  dbHelpProviders->setToolTip( getGeometryNameIndex(), QString( "The structure will be shown, even if the if the needed driver is not active and cannot be shown" ) );
  dbHelpProviders->setToolTip( getGeometryTypeIndex(), QString( "A message will be shown with the name of the needed driver, if it is active and can be shown" ) );
#endif
  // Column 1: getGeometryNameIndex()
  sText = QStringLiteral( "Versions supported by QgsOgrProvider" );
  dbHelpProviders->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Vectors, when the needed Driver is installed" );
  dbHelpProviders->setText( getGeometryTypeIndex(), sText );
  dbHelpProviders->setIcon( getGeometryTypeIndex(), QgsSpatialiteDbInfo::SpatialMetadataTypeIcon( QgsSpatialiteDbInfo::SpatialiteFdoOgr ) );
  dbHelpProviders->setBackground( getGeometryTypeIndex(), QColor( "indianred" ) );
  dbHelpProviders->setForeground( getGeometryTypeIndex(), QColor( "white" ) );
  // Column 3 [not used]: getSqlQueryIndex()
  sSortTag = QStringLiteral( "AABA_%1" ).arg( iSortCounter++ );
  dbHelpProviders->setText( getColumnSortHidden(), sSortTag );
  dbHelpItem->addChild( dbHelpProviders );
  // Add Provider with Children [will be shown as second (and last) child]
  mPrepairedChildItems.append( dbHelpItem );
  // new Child to be added to Help Text
  dbHelpItem = new QgsSpatialiteDbInfoItem( this, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  // Column 0: getTableNameIndex()
  sText = QStringLiteral( "The Total amount of tables in the Database will be shown, " );
  dbHelpItem->setText( getTableNameIndex(), sText );
  // Column 1: getGeometryNameIndex()
  sText = QStringLiteral( "togeather with the Container-Type" );
  dbHelpItem->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "with information about the needed Driver (name and if active)" );
  dbHelpItem->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AAEA_%1" ).arg( iSortCounter++ );
  dbHelpItem->setText( getColumnSortHidden(), sSortTag );
  // Add Provider with Children [will be shown as second (and last) child]
  mPrepairedChildItems.append( dbHelpItem );
  // new Child to be added to Help Text
  dbHelpItem = new QgsSpatialiteDbInfoItem( this, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
  // Column 0: getTableNameIndex()
  sText = QStringLiteral( "When a Layer is added, the corresponding Provider will be used" );
  dbHelpItem->setText( getTableNameIndex(), sText );
  // Column 1: getGeometryNameIndex()
  sText = QStringLiteral( "Only sqlite3 containers are supported" );
  dbHelpItem->setText( getGeometryNameIndex(), sText );
  // Column 2: getGeometryTypeIndex()
  sText = QStringLiteral( "Raster and Vector Layers can be added togeather, when the needed Drivers are installed" );
  dbHelpItem->setText( getGeometryTypeIndex(), sText );
  sSortTag = QStringLiteral( "AADA_%1" ).arg( iSortCounter++ );
  dbHelpItem->setText( getColumnSortHidden(), sSortTag );
  // Add Provider with Children [will be shown as second (and last) child]
  mPrepairedChildItems.append( dbHelpItem );
}
