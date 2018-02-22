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
// QgsSpatialiteDbInfoModel::getExpandToDepth
// Based on the Item-Type, a different can be returned
// - ItemTypeHelpRoot: all level should be shown
// - ItemTypeDb: down to Table/View name, but not the Columns
//-----------------------------------------------------------------
int QgsSpatialiteDbInfoModel::getExpandToDepth()
{
  if ( ( mDbRootItem ) && ( mDbRootItem->childCount() == 1 ) )
  {
    switch ( mDbRootItem->child( 0 )->getItemType() )
    {
      case QgsSpatialiteDbInfoItem::ItemTypeHelpRoot:
        mExpandToDepth = 3;
        break;
      case QgsSpatialiteDbInfoItem::ItemTypeDb:
        mExpandToDepth = 1;
        break;
      default:
        mExpandToDepth = 3;
        break;
    }
  }
  return mExpandToDepth;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::~QgsSpatialiteDbInfoModel
//-----------------------------------------------------------------
QgsSpatialiteDbInfoModel::~QgsSpatialiteDbInfoModel()
{
  if ( mDbRootItem )
  {
    removeRows( 0, rowCount() );
    delete mDbRootItem;
  }
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
  QgsDebugMsgLevel( QString( "-I-> nodeWillAddChildren node.row/column[%1,%2] index[from=%3,to=%4]" ).arg( node->row() ).arg( node->column() ).arg( indexFrom ).arg( indexTo ), 7 );
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
  QgsDebugMsgLevel( QString( "-I-> QgsSpatialiteDbInfoModel::nodeWillRemoveChildren node.row/column[%1,%2] index[from=%3,to=%4]" ).arg( node->row() ).arg( node->column() ).arg( indexFrom ).arg( indexTo ), 7 );
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
  // int row = node->parent()->children().indexOf( node );
  int row = node->row();
  QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoModel::node2index ItemType[%1, row=%2] parentType[%3]" ).arg( node->getItemType() ).arg( node->row() ).arg( node->parent()->getItemType() ), 7 );
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
  // QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoModel::data -0- index.row/column[%1,%2] role[%3] index.valid[%4]" ).arg( index.row() ).arg( index.column() ).arg( role ).arg(index.isValid()), 7 );
  if ( !index.isValid() )
  {
    return QVariant();
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
// QgsSpatialiteDbInfoModel::setSpatialiteDbInfo
//-----------------------------------------------------------------
void QgsSpatialiteDbInfoModel::setSpatialiteDbInfo( QgsSpatialiteDbInfo *spatialiteDbInfo )
{
  if ( ( spatialiteDbInfo ) && ( spatialiteDbInfo->isDbValid() ) )
  {
    // Inform the TablesTreeView to stop painting, we are goint to rebuild everything
    beginResetModel();
    if ( getSpatialiteDbInfo() )
    {
      mTableCount = 0;
      if ( mDbItem )
      {
        // remove Children from mDbRootItem
        // - the children will be deleted
        removeRows( 0, rowCount() );
        mDbItem = nullptr;
      }
      if ( getSpatialiteDbInfo()->getQSqliteHandle() )
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
    mNonSpatialTablesCounter = 0;
    mDbItem = new QgsSpatialiteDbInfoItem( mDbRootItem, mSpatialiteDbInfo );
    if ( mDbItem )
    {
      if ( mDbRootItem->getSpatialiteDbInfo() )
      {
        mTableCounter = mDbRootItem->getTableCounter();
        mNonSpatialTablesCounter = mDbRootItem->getNonSpatialTablesCounter();
        QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoModel::setSpatialiteDbInfo root.childCount[%1, %2] counter[%3, %4]" ).arg( mDbRootItem->childCount() ).arg( mDbRootItem->child( 0 )->text( 0 ) ).arg( mTableCounter ).arg( mNonSpatialTablesCounter ), 7 );
        QMap<QString, QString> dbErrors = mDbRootItem->getSpatialiteDbInfoErrors( false ); // Errors [default]
        for ( QMap<QString, QString>::iterator itErrors = dbErrors.begin(); itErrors != dbErrors.end(); ++itErrors )
        {
          QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoModel::setSpatialiteDbInfo Database Errors: Key[%1] Value[%2]" ).arg( itErrors.key() ).arg( itErrors.value() ), 7 );
        }
        dbErrors = mDbRootItem->getSpatialiteDbInfoErrors( true ); // Warnings
        for ( QMap<QString, QString>::iterator itWarnings = dbErrors.begin(); itWarnings != dbErrors.end(); ++itWarnings )
        {
          QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoModel::setSpatialiteDbInfo Database Warnings: Key[%1] Value[%2]" ).arg( itWarnings.key() ).arg( itWarnings.value() ), 7 );
        }
      }
    }
    // We have finished rebuilding, inform the TablesTreeView to repaint everything
    endResetModel();
  }
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::findItems
// - taken from QStandardItemModel
// -> uses getItem() instead of 'itemFromIndex' as in QStandardItemModel
// TODO: theis may not be needed
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
  if ( ( getSpatialiteDbInfo() ) && ( getSpatialiteDbInfo()->isDbValid() ) )
  {
    if ( withPath )
    {
      sDatabaseName = getSpatialiteDbInfo()->getDatabaseFileName();
    }
    else
    {
      sDatabaseName = getSpatialiteDbInfo()->getFileName();
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
  if ( ( getSpatialiteDbInfo() ) && ( getSpatialiteDbInfo()->isDbValid() ) )
  {
    sDatabaseUri = getSpatialiteDbInfo()->getDatabaseUri();
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
  if ( ( getSpatialiteDbInfo() ) && ( getSpatialiteDbInfo()->isDbValid() ) && ( getSpatialiteDbInfo()->isDbSpatialite() ) )
  {
    return getSpatialiteDbInfo()->UpdateLayerStatistics( saLayers );
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
      setSpatialiteDbInfo( spatialiteDbInfo );
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
// QgsSpatialiteDbInfoItem constructor
// - to Build a Database
// OnInit will call sniffSpatialiteDbInfo
//-----------------------------------------------------------------
QgsSpatialiteDbInfoItem::QgsSpatialiteDbInfoItem( QgsSpatialiteDbInfoItem *parent, QgsSpatialiteDbInfo *spatialiteDbInfo )
{
  mParentItem = parent;
  mSpatialiteDbInfo = spatialiteDbInfo;
  if ( getSpatialiteDbInfo() )
  {
    mItemType = ItemTypeDb;
    if ( mParentItem )
    {
      mParentItem->setTableCounter( 0 );
      mParentItem->setNonSpatialTablesCounter( 0 );
    }
    QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem:: ItemTypeDb FileName[%1] DbType[%2]" ).arg( getSpatialiteDbInfo()->getFileName() ).arg( getSpatialiteDbInfo()->dbSpatialMetadataString() ), 7 );
  }
  if ( onInit() )
  {
    // Pre-Conditions for this Item-Type are fullfilled
    if ( mItemType == ItemTypeDb )
    {
      buildDbInfoItem();
    }
  }
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem constructor
// - to Build a SpatialLayer
//-----------------------------------------------------------------
QgsSpatialiteDbInfoItem::QgsSpatialiteDbInfoItem( QgsSpatialiteDbInfoItem *parent, QgsSpatialiteDbLayer *dbLayer )
{
  mParentItem = parent;
  mDbLayer = dbLayer;
  if ( getDbLayer() )
  {
    mItemType = ItemTypeLayer;
    mLayerType =  getDbLayer()->getLayerType();
  }
  if ( onInit() )
  {
    // Pre-Conditions for this Item-Type are fullfilled
    if ( mItemType == ItemTypeLayer )
    {
      buildDbLayerItem();
    }
  }
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
    // Pre-Conditions for this Item-Type are fullfilled
    if ( mItemType == ItemTypeHelpRoot )
    {
      buildHelpItem();
      // mPrepairedChildItems will be emptied
      insertPrepairedChildren();
    }
    if ( mItemType == ItemTypeGroupNonSpatialTables )
    {
      buildNonSpatialTablesGroups();
      // mPrepairedChildItems will be emptied
      insertPrepairedChildren();
    }
  }
}
// -- ---------------------------------- --
// class QgsSpatialiteDbInfoItem
// - for Item Rows [with children columns]
// -- ---------------------------------- --
QgsSpatialiteDbInfoItem::QgsSpatialiteDbInfoItem( QgsSpatialiteDbInfoItem *parent, SpatialiteDbInfoItemType itemType, QString sItemName, QStringList saItemInfos )
{
  mParentItem = parent;
  mItemType = itemType;
  if ( mParentItem )
  {
#if 0
    if ( mItemType == ItemTypeNonSpatialTablesGroup )
    {
      QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem:: creating [%1] GroupName[%2] sTableNames.count[%3]" ).arg( QString( "NonSpatialTablesGroup" ) ).arg( sItemName ).arg( saItemInfos .count() ), 7 );
    }
    if ( mItemType == ItemTypeAdmin )
    {
      QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem:: creating [%1] GroupName[%2] sTableNames.count[%3]" ).arg( QString( "ItemTypeAdmin" ) ).arg( sItemName ).arg( saItemInfos .count() ), 7 );
    }
#endif
  }
  if ( onInit() )
  {
    // Pre-Conditions for this Item-Type are fullfilled
    if ( mItemType == ItemTypeNonSpatialTablesGroup )
    {
      buildNonSpatialTablesGroup( sItemName, saItemInfos );
    }
    if ( mItemType == ItemTypeAdmin )
    {
      buildNonSpatialAdminTable( sItemName, saItemInfos );
      // mPrepairedChildItems will be emptied
      // insertPrepairedChildren();
    }
  }
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::setSpatialiteDbInfo
//-----------------------------------------------------------------
bool QgsSpatialiteDbInfoItem::setSpatialiteDbInfo( QgsSpatialiteDbInfo *spatialiteDbInfo )
{
  if ( ( spatialiteDbInfo ) && ( spatialiteDbInfo->isDbValid() ) )
  {
    mSpatialiteDbInfo = spatialiteDbInfo;
    mSpatialMetadata = getSpatialiteDbInfo()->dbSpatialMetadata();
    mSpatialMetadataString = getSpatialiteDbInfo()->dbSpatialMetadataString();
    getSpatialiteDbInfo()->isDbValid();
  }
  return false;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::getSpatialiteDbInfoErrors
//-----------------------------------------------------------------
// - only when geometryTypes
// - this does not include the geometry of the Layer
//-----------------------------------------------------------------
QMap<QString, QString> QgsSpatialiteDbInfoItem::getSpatialiteDbInfoErrors( bool bWarnings ) const
{
  QMap<QString, QString> dbErrors;
  if ( getSpatialiteDbInfo() )
  {
    if ( getDbLayer() )
    {
      if ( bWarnings )
      {
        dbErrors = getDbLayer()->getLayerWarnings();
      }
      else
      {
        dbErrors = getDbLayer()->getLayerErrors();
      }
    }
    else
    {
      if ( bWarnings )
      {
        dbErrors = getSpatialiteDbInfo()->getDbWarnings();
      }
      else
      {
        dbErrors = getSpatialiteDbInfo()->getDbErrors();
      }
    }
  }
  return dbErrors;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::addColumnItems()
//-----------------------------------------------------------------
// - only when geometryTypes
// - this does not include the geometry of the Layer
//-----------------------------------------------------------------
int QgsSpatialiteDbInfoItem::addColumnItems()
{
  int iColumsCount = 0;
  if ( getDbLayer() )
  {
    QColor fg_color = QColor( "yellow" ); // gold "khaki"
    QColor bg_color = QColor( "lightyellow" );
    QColor bg_color_Primary_Key = QColor( "darkcyan" );
    QgsFields attributeFields = getDbLayer()->getAttributeFields();
    QString sPrimaryKey = getDbLayer()->getPrimaryKey();
    for ( int i = 0; i < attributeFields.count(); i++ )
    {
      QString sName = attributeFields.at( i ).name();
      QString sType = attributeFields.at( i ).typeName();
      QgsWkbTypes::Type geometryType = QgsSpatialiteDbLayer::GetGeometryTypeLegacy( sType );
      QIcon iconType;
      if ( geometryType != QgsWkbTypes::Unknown )
      {
        iconType = QgsSpatialiteDbInfo::SpatialGeometryTypeIcon( geometryType );
      }
      else
      {
        iconType = QgsSpatialiteDbInfo::QVariantTypeIcon( attributeFields.at( i ).type() );
      }
      QString sComment = attributeFields.at( i ).comment();
      QString sSortTag = QString( "AZZC_%1_%2" ).arg( getDbLayer()->getLayerName() ).arg( i );
      QString sCommentGeometryType = QString();
      if ( getDbLayer()->getCapabilitiesString().startsWith( "SpatialView(" ) )
      {
        if ( getDbLayer()->getCapabilitiesString().startsWith( "SpatialView(ReadOnly)" ) )
        {
          sCommentGeometryType = QString( "Readonly-SpatialView: Default-Values from from CREATE TABLE-Sql of underling Table and WHERE conditions of View" );
        }
        else
        {
          sCommentGeometryType = QString( "Writable-SpatialView: Default-Values from CREATE TABLE-Sql of underling Table and defaults set in Triggers " );
        }
      }
      else if ( getDbLayer()->getCapabilitiesString().startsWith( "SpatialTable(" ) )
      {
        sCommentGeometryType = QString( "SpatialTable: Default-Values from CREATE TABLE-Sql " );
      }
      QgsSpatialiteDbInfoItem *dbColumnItem = new QgsSpatialiteDbInfoItem( this, QgsSpatialiteDbInfoItem::ItemTypeColumn );
      if ( dbColumnItem )
      {
        // Column 0: getTableNameIndex()
        dbColumnItem->setText( getTableNameIndex(), sName );
        dbColumnItem->setBackground( getTableNameIndex(), bg_color );
        // Column 1: getGeometryNameIndex()
        dbColumnItem->setText( getGeometryNameIndex(), sType );
        dbColumnItem->setIcon( getGeometryNameIndex(), iconType );
        dbColumnItem->setBackground( getGeometryNameIndex(), bg_color );
        // Column 2: getGeometryTypeIndex()
        if ( sName == sPrimaryKey )
        {
          dbColumnItem->setIcon( getGeometryTypeIndex(), QgsSpatialiteDbInfo::QVariantTypeIcon( QVariant::KeySequence ) );
          dbColumnItem->setBackground( getGeometryTypeIndex(), bg_color_Primary_Key );
          dbColumnItem->setForeground( getGeometryTypeIndex(), fg_color );
          if ( sComment.isEmpty() )
          {
            sComment = QString( "Primary-Key" );
          }
          else
          {
            if ( sComment !=  QStringLiteral( "PRIMARY KEY" ) )
            {
              sComment = QString( "[Primary-Key] %1" ).arg( sComment );
            }
          }
        }
        else
        {
          dbColumnItem->setBackground( getGeometryTypeIndex(), bg_color );
        }
        dbColumnItem->setText( getGeometryTypeIndex(), sComment );
        if ( !sCommentGeometryType.isEmpty() )
        {
          dbColumnItem->setToolTip( getGeometryTypeIndex(), sCommentGeometryType );
        }
        // Column 3 [not used]: getSqlQueryIndex()
        // Column 4: getColumnSortHidden()
        dbColumnItem->setText( getColumnSortHidden(), sSortTag );
        mPrepairedChildItems.append( dbColumnItem );
      }
    }
    iColumsCount = mPrepairedChildItems.count();
    // mPrepairedChildItems will be emptied
    insertPrepairedChildren();
    addCommonMetadataItems();
  }
  return iColumsCount;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::addCommonMetadataItems
//-----------------------------------------------------------------
// This List will be shown after the Columns [Vector-only]
// - not all Meta informaton is supported by all Layer-Types
//-----------------------------------------------------------------
int QgsSpatialiteDbInfoItem::addCommonMetadataItems()
{
  int iColumsCount = 0;
  if ( ( getSpatialiteDbInfo() ) && ( getDbLayer() ) )
  {
    QColor bg_color = QColor( "paleturquoise" );
    bool bMetadataExtended = false;
    if ( mIsRasterLite2 )
    {
      bMetadataExtended = true;
    }
    QgsLayerMetadata layerMetadata = getDbLayer()->getLayerMetadata();
    QString sToolTipText = QStringLiteral( "Metaddata [%1]" ).arg( getLayerTypeString() );
    QList<QPair<QString, QString>> metaData;
    metaData.append( qMakePair( QStringLiteral( "Capabilities" ), getDbLayer()->getCapabilitiesString() ) );
    if ( mIsGeometryType )
    {
      metaData.append( qMakePair( QStringLiteral( "Features" ), QString::number( getDbLayer()->getFeaturesCount() ) ) );
    }
    if ( ( !mIsMbTiles ) && ( !mIsRasterLite1 ) )
    {
      metaData.append( qMakePair( QStringLiteral( "Title" ), getDbLayer()->getTitle() ) );
      metaData.append( qMakePair( QStringLiteral( "Abstract" ), getDbLayer()->getAbstract() ) );
    }
    if ( bMetadataExtended )
    {
      metaData.append( qMakePair( QStringLiteral( "Copyright" ), getDbLayer()->getCopyright() ) );
    }
    metaData.append( qMakePair( QStringLiteral( "Srid" ), getDbLayer()->getSridEpsg() ) );
    metaData.append( qMakePair( QStringLiteral( "Extent" ), getDbLayer()->getLayerExtentEWKT() ) );
    metaData.append( qMakePair( QStringLiteral( "-> Extent - Min X" ), QString::number( getDbLayer()->getLayerExtentMinX(), 'f', 7 ) ) );
    metaData.append( qMakePair( QStringLiteral( "-> Extent - Min Y" ), QString::number( getDbLayer()->getLayerExtentMinY(), 'f', 7 ) ) );
    metaData.append( qMakePair( QStringLiteral( "-> Extent - Max X" ), QString::number( getDbLayer()->getLayerExtentMaxX(), 'f', 7 ) ) );
    metaData.append( qMakePair( QStringLiteral( "-> Extent - Max Y" ), QString::number( getDbLayer()->getLayerExtentMaxY(), 'f', 7 ) ) );
    metaData.append( qMakePair( QStringLiteral( "-> Extent - Width" ), QString::number( getDbLayer()->getLayerExtentWidth(), 'f', 7 ) ) );
    metaData.append( qMakePair( QStringLiteral( "-> Extent - Height" ), QString::number( getDbLayer()->getLayerExtentHeight(), 'f', 7 ) ) );
    if ( !mIsGeometryType )
    {
      metaData.append( qMakePair( QStringLiteral( "Resolution - X" ), QString::number( getDbLayer()->getLayerImageResolutionX(), 'f', 7 ) ) );
      metaData.append( qMakePair( QStringLiteral( "Resolution - Y" ), QString::number( getDbLayer()->getLayerImageResolutionY(), 'f', 7 ) ) );
      metaData.append( qMakePair( QStringLiteral( "Image-Size" ), QString( "%1x%2" ).arg( getDbLayer()->getLayerImageWidth() ).arg( getDbLayer()->getLayerImageHeight() ) ) );
      metaData.append( qMakePair( QStringLiteral( "Bands" ), QString::number( getDbLayer()->getLayerNumBands() ) ) );
      //if ( mIsRasterLite2 )
      metaData.append( qMakePair( QStringLiteral( "Data Type" ), getDbLayer()->getLayerRasterDataTypeString() ) );
      metaData.append( qMakePair( QStringLiteral( "Pixel Type" ), getDbLayer()->getLayerRasterPixelTypeString() ) );
      metaData.append( qMakePair( QStringLiteral( "Compression Type" ), getDbLayer()->getLayerRasterCompressionType() ) );
      metaData.append( qMakePair( QStringLiteral( "Tile Size" ), QString( "%1x%2" ).arg( getDbLayer()->getLayerTileWidth() ).arg( getDbLayer()->getLayerTileHeight() ) ) );
    }
    for ( int i = 0; i < metaData.count(); i++ )
    {
      QPair<QString, QString> pair = metaData.at( i );
      if ( !pair.first.isEmpty() )
      {
        QString sName = pair.first;
        QString sValue = pair.second;
        // Make sure that his does not get re-sorted when more that 10 entries ['0001']
        QString sSortTag = QString( "AZZM_%1_%2" ).arg( getDbLayer()->getLayerName() ).arg( QString( "%1" ).arg( i, 5, 10, QChar( '0' ) ) );
        QgsSpatialiteDbInfoItem *dbColumnItem = new QgsSpatialiteDbInfoItem( this, QgsSpatialiteDbInfoItem::ItemTypeMBTilesMetadata );
        if ( dbColumnItem )
        {
          // Column 0 [not used]: getTableNameIndex()
          // Column 1: getGeometryNameIndex()
          dbColumnItem->setText( getGeometryNameIndex(), sName );
          dbColumnItem->setToolTip( getGeometryNameIndex(), sToolTipText );
          dbColumnItem->setBackground( getGeometryNameIndex(), bg_color );
          // Column 2: getGeometryTypeIndex()
          dbColumnItem->setText( getGeometryTypeIndex(), sValue );
          // These can be long string descriptions, add ToolTip
          dbColumnItem->setToolTip( getGeometryTypeIndex(), sValue );
          // dbColumnItem->setEditable( getGeometryTypeIndex(), true );
          dbColumnItem->setBackground( getGeometryTypeIndex(), bg_color );

          // Column 3 [not used]: getSqlQueryIndex()
          // Column 4: getColumnSortHidden()
          dbColumnItem->setText( getColumnSortHidden(), sSortTag );
          mPrepairedChildItems.append( dbColumnItem );
        }
      }
    }
    metaData.clear();
    iColumsCount = mPrepairedChildItems.count();
    // mPrepairedChildItems will be emptied
    insertPrepairedChildren();
  }
  return iColumsCount;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoModel::addMbTilesMetadataItems
//-----------------------------------------------------------------
// - only when MBTilesType [getDbMBTileMetaData()]
//-----------------------------------------------------------------
int QgsSpatialiteDbInfoItem::addMbTilesMetadataItems()
{
  int iColumsCount = 0;
  if ( ( getSpatialiteDbInfo() ) && ( getDbLayer() ) )
  {
    QColor bg_color = QColor( "lightyellow" );
    QColor bg_edit = QColor( "darkseagreen" );
    QMap<QString, QString> metaData = getSpatialiteDbInfo()->getDbMBTileMetaData();
    for ( QMap<QString, QString>::iterator itLayers = metaData.begin(); itLayers != metaData.end(); ++itLayers )
    {
      if ( !itLayers.key().isEmpty() )
      {
        QString sName = itLayers.key();
        QString sValue = itLayers.value();
        QString sSortTag = QString( "AZZC_%1_%2" ).arg( getDbLayer()->getLayerName() ).arg( sName );
        QgsSpatialiteDbInfoItem *dbColumnItem = new QgsSpatialiteDbInfoItem( this, QgsSpatialiteDbInfoItem::ItemTypeMBTilesMetadata );
        if ( dbColumnItem )
        {
          // Column 0: getTableNameIndex()
          dbColumnItem->setText( getTableNameIndex(), sName );
          dbColumnItem->setToolTip( getTableNameIndex(), QStringLiteral( "Entry of MbTiles 'metadata' TABLE" ) );
          dbColumnItem->setBackground( getTableNameIndex(), bg_color );
          // Column 1: getGeometryNameIndex()
          dbColumnItem->setText( getGeometryNameIndex(), sValue );
          // These can be long string descriptions, add ToolTip
          dbColumnItem->setToolTip( getGeometryNameIndex(), sValue );
          dbColumnItem->setEditable( getGeometryNameIndex(), true );
          dbColumnItem->setBackground( getGeometryNameIndex(), bg_edit );
          // Column 2 [not used]: getGeometryTypeIndex()
          // Column 3 [not used]: getSqlQueryIndex()
          // Column 4: getColumnSortHidden()
          dbColumnItem->setText( getColumnSortHidden(), sSortTag );
          mPrepairedChildItems.append( dbColumnItem );
        }
      }
    }
    iColumsCount = mPrepairedChildItems.count();
    // mPrepairedChildItems will be emptied
    insertPrepairedChildren();
    addCommonMetadataItems();
  }
  return iColumsCount;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::~QgsSpatialiteDbInfoItem
//-----------------------------------------------------------------
QgsSpatialiteDbInfoItem::~QgsSpatialiteDbInfoItem()
{
  qDeleteAll( mChildItems );
  mItemData.clear();
  if ( getDbLayer() )
  {
    // Do not delete, may be used elsewhere
    mDbLayer = nullptr;
  }
  if ( getSpatialiteDbInfo() )
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
  bool bSpatialiteDbInfo = false;
  bool bDbLayer = false;
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
    if ( !getSpatialiteDbInfo() )
    {
      // This may be nullptr
      mSpatialiteDbInfo = mParentItem->getSpatialiteDbInfo();
    }
    // Retrieve DbLayer from Parent, if exists and not done allready [there can only be one]
    if ( !getDbLayer() )
    {
      // This may be nullptr
      mDbLayer = mParentItem->getDbLayer();
    }
    switch ( mItemType )
    {
      //   Qt::ItemFlags flags = ( Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled );
      case ItemTypeHelpRoot:
      case ItemTypeHelpText:
      case ItemTypeUnknown:
      case ItemTypeGroupSpatialTable:
      case ItemTypeGroupSpatialView:
      case ItemTypeGroupVirtualShape:
      case ItemTypeGroupRasterLite1:
      case ItemTypeGroupRasterLite2Vector:
      case ItemTypeGroupRasterLite2Raster:
      case ItemTypeGroupSpatialiteTopology:
      case ItemTypeGroupTopologyExport:
      case ItemTypeGroupStyleVector:
      case ItemTypeGroupStyleRaster:
      case ItemTypeGroupGdalFdoOgr:
      case ItemTypeGroupGeoPackageVector:
      case ItemTypeGroupGeoPackageRaster:
      case ItemTypeGroupMBTilesTable:
      case ItemTypeGroupMBTilesView:
      case ItemTypeGroupMetadata:
      case ItemTypeGroupAllSpatialLayers:
      case ItemTypeGroupNonSpatialTables:
      case ItemTypeMBTilesMetadata:
      case ItemTypeColumn:
      case ItemTypeAdmin:
      case ItemTypeNonSpatialTablesGroup:
        flags &= ~Qt::ItemIsSelectable;
        break;
      case ItemTypeDb:
      case ItemTypeLayer:
        break;
      default:
        break;
    }
    // -1: set all columns with the given Flags
    setFlags( -1, flags );
    // forward the signal towards the root
    // connectToChildItem( this );
  }
  if ( getDbLayer() )
  {
    if ( !getSpatialiteDbInfo() )
    {
      // Retrieve SpatialiteDbInfo from Parent, if exists and not done allready [there can only be one]
      mSpatialiteDbInfo = getDbLayer()->getSpatialiteDbInfo();
    }
    mLayerTypeString = getDbLayer()->getLayerTypeString();
    mLayerName = getDbLayer()->getLayerName();
    bDbLayer = true;
    switch ( mLayerType )
    {
      case QgsSpatialiteDbInfo::SpatialTable:
      case QgsSpatialiteDbInfo::SpatialView:
      case QgsSpatialiteDbInfo::TopologyExport:
      case QgsSpatialiteDbInfo::GeoPackageVector:
      case QgsSpatialiteDbInfo::GdalFdoOgr:
      case QgsSpatialiteDbInfo::VirtualShape:
        if ( mLayerType ==  QgsSpatialiteDbInfo::SpatialView )
        {
          mIsSpatialView = true;
        }
        if ( mLayerType ==  QgsSpatialiteDbInfo::GeoPackageVector )
        {
          mIsGeoPackage = true;
        }
        mIsGeometryType = true;
        break;
      case QgsSpatialiteDbInfo::MBTilesTable:
      case QgsSpatialiteDbInfo::MBTilesView:
        mIsMbTiles = true;
        break;
      case QgsSpatialiteDbInfo::RasterLite2Raster:
      case QgsSpatialiteDbInfo::RasterLite1:
      case QgsSpatialiteDbInfo::GeoPackageRaster:
        mIsGeometryType = false;
        if ( mLayerType ==  QgsSpatialiteDbInfo::RasterLite1 )
        {
          mIsRasterLite1 = true;
        }
        if ( mLayerType ==  QgsSpatialiteDbInfo::RasterLite2Raster )
        {
          mIsRasterLite2 = true;
        }
        if ( mLayerType ==  QgsSpatialiteDbInfo::GeoPackageRaster )
        {
          mIsGeoPackage = true;
        }
        break;
      default:
        mIsGeometryType = false;
        mIsGeoPackage = false;
        mIsMbTiles = false;
        mIsRasterLite2 = false;
        mIsRasterLite2 = false;
        mIsSpatialView = false;
        break;
    }
  }
  if ( getSpatialiteDbInfo() )
  {
    bSpatialiteDbInfo = true;
    mSpatialMetadata = getSpatialiteDbInfo()->dbSpatialMetadata();
    mSpatialMetadataString = getSpatialiteDbInfo()->dbSpatialMetadataString();
    if ( mItemType == ItemTypeDb )
    {
      // Determin amount of Layers, and Layer-Types
      // Add Groups and fill with Layers
      if ( sniffSpatialiteDbInfo() )
      {
        if ( mParentItem )
        {
          // Tell mRootItem the amount of Tables
          mParentItem->setTableCounter( getTableCounter() );
          mParentItem->setNonSpatialTablesCounter( getNonSpatialTablesCounter() );
          mParentItem->setSpatialiteDbInfo( getSpatialiteDbInfo() );
        }
        bRc = true;
      }
    }
    else
    {
      if ( mParentItem )
      {
        setTableCounter( mParentItem->getTableCounter() ) ;
        setNonSpatialTablesCounter( mParentItem->getNonSpatialTablesCounter() );
      }
    }
    switch ( mItemType )
    {
      case ItemTypeLayer:
      case ItemTypeNonSpatialTablesGroup:
      case ItemTypeGroupNonSpatialTables:
      case ItemTypeAdmin:
        // These Item-Types should call tasks after onInit()
        bRc = true;
        break;
      default:
        break;
    }
  }
  else
  {
    // do not have an active mSpatialiteDbInfo
    if ( mParentItem )
    {
      setTableCounter( mParentItem->getTableCounter() ) ;
      setNonSpatialTablesCounter( mParentItem->getNonSpatialTablesCounter() );
    }
    switch ( mItemType )
    {
      case ItemTypeHelpRoot:
      case ItemTypeHelpText:
        // These Item-Types should call tasks after onInit()
        bRc = true;
        break;
      default:
        break;
    }
  }
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
      case ItemTypeLayer:
      default:
        break;
    }
#if 0
    QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem::onInit bRc[%1 ColumnsCount[%2] TableCounter[%3] ItemType[this=%4,parent=%5] Parent-childCount[%6]  DbContainterType[%7] LayerType[%8] LayerName[%9] ,bSpatialiteDbInfo=%10,bDbLayer=%11]" ).arg( bRc ).arg( columnCount() ).arg( getTableCounter() ).arg( getItemType() ).arg( getLayerTypeString() ).arg( mParentItem->getItemTypeString() ).arg( mParentItem->childCount() ).arg( dbSpatialMetadataString() ).arg( getLayerName() ).arg( bSpatialiteDbInfo ).arg( bDbLayer ), 7 );
#endif
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
    connectToChildItem( child );
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
    connectToChildItem( child );
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
// -> starting at the given position, for the given count
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
// QgsSpatialiteDbInfoItem::sniffSpatialiteDbInfo
//-----------------------------------------------------------------
bool QgsSpatialiteDbInfoItem::sniffSpatialiteDbInfo()
{
  bool bRc = false;
  if ( ( getSpatialiteDbInfo() ) && ( getSpatialiteDbInfo()->isDbValid() ) )
  {
    mTableCounter = 0; // Count the Total amount of entries for all Spatial-Types.
    mNonSpatialTablesCounter = 0; // Count the Total amount of entries for all non-Spatial-Types.
    QgsSpatialiteDbInfoItem *dbGroupLayerItem = nullptr;
    if ( getSpatialiteDbInfo()->dbSpatialTablesLayersCount() > 0 )
    {
      dbGroupLayerItem = createGroupLayerItem( QgsSpatialiteDbInfo::SpatialTable );
      if ( dbGroupLayerItem )
      {
        mPrepairedChildItems.append( dbGroupLayerItem );
        mTableCounter += getSpatialiteDbInfo()->dbSpatialTablesLayersCount();
      }
    }
    if ( getSpatialiteDbInfo()->dbSpatialViewsLayersCount() > 0 )
    {
      dbGroupLayerItem = createGroupLayerItem( QgsSpatialiteDbInfo::SpatialView );
      if ( dbGroupLayerItem )
      {
        mPrepairedChildItems.append( dbGroupLayerItem );
        mTableCounter += getSpatialiteDbInfo()->dbSpatialViewsLayersCount();
      }
    }
    if ( getSpatialiteDbInfo()->dbVirtualShapesLayersCount() > 0 )
    {
      dbGroupLayerItem = createGroupLayerItem( QgsSpatialiteDbInfo::VirtualShape );
      if ( dbGroupLayerItem )
      {
        mPrepairedChildItems.append( dbGroupLayerItem );
        mTableCounter += getSpatialiteDbInfo()->dbVirtualShapesLayersCount();
      }
    }
    // these values will be -1 if no admin-tables were found
    if ( getSpatialiteDbInfo()->dbRasterCoveragesLayersCount() > 0 )
    {
      // RasterLite2
      dbGroupLayerItem = createGroupLayerItem( QgsSpatialiteDbInfo::RasterLite2Raster );
      if ( dbGroupLayerItem )
      {
        mPrepairedChildItems.append( dbGroupLayerItem );
        mTableCounter += getSpatialiteDbInfo()->dbRasterCoveragesLayersCount();
      }
    }
    if ( getSpatialiteDbInfo()->dbRasterLite1LayersCount() > 0 )
    {
      dbGroupLayerItem = createGroupLayerItem( QgsSpatialiteDbInfo::RasterLite1 );
      if ( dbGroupLayerItem )
      {
        mPrepairedChildItems.append( dbGroupLayerItem );
        mTableCounter += getSpatialiteDbInfo()->dbRasterLite1LayersCount();
      }
    }
    if ( getSpatialiteDbInfo()->dbMBTilesLayersCount() > 0 )
    {
      dbGroupLayerItem = createGroupLayerItem( QgsSpatialiteDbInfo::MBTilesTable );
      if ( dbGroupLayerItem )
      {
        mPrepairedChildItems.append( dbGroupLayerItem );
        mTableCounter += getSpatialiteDbInfo()->dbMBTilesLayersCount();
      }
    }
    if ( getSpatialiteDbInfo()->dbGeoPackageVectorsCount() > 0 )
    {
      dbGroupLayerItem = createGroupLayerItem( QgsSpatialiteDbInfo::GeoPackageVector );
      if ( dbGroupLayerItem )
      {
        mPrepairedChildItems.append( dbGroupLayerItem );
        mTableCounter += getSpatialiteDbInfo()->dbGeoPackageLayersCount();
      }
    }
    if ( getSpatialiteDbInfo()->dbGeoPackageRastersCount() > 0 )
    {
      dbGroupLayerItem = createGroupLayerItem( QgsSpatialiteDbInfo::GeoPackageRaster );
      if ( dbGroupLayerItem )
      {
        mPrepairedChildItems.append( dbGroupLayerItem );
        mTableCounter += getSpatialiteDbInfo()->dbGeoPackageLayersCount();
      }
    }
    if ( getSpatialiteDbInfo()->dbFdoOgrLayersCount() > 0 )
    {
      dbGroupLayerItem = createGroupLayerItem( QgsSpatialiteDbInfo::GdalFdoOgr );
      if ( dbGroupLayerItem )
      {
        mPrepairedChildItems.append( dbGroupLayerItem );
        mTableCounter += getSpatialiteDbInfo()->dbFdoOgrLayersCount();
      }
    }
    if ( getSpatialiteDbInfo()->dbTopologyExportLayersCount() > 0 )
    {
      dbGroupLayerItem = createGroupLayerItem( QgsSpatialiteDbInfo::TopologyExport );
      if ( dbGroupLayerItem )
      {
        mPrepairedChildItems.append( dbGroupLayerItem );
        mTableCounter += getSpatialiteDbInfo()->dbTopologyExportLayersCount();
      }
    }
    if ( getSpatialiteDbInfo()->dbNonSpatialTablesCount() > 0 )
    {
      dbGroupLayerItem = createGroupLayerItem( QgsSpatialiteDbInfo::NonSpatialTables );
      if ( dbGroupLayerItem )
      {
        mPrepairedChildItems.append( dbGroupLayerItem );
        mNonSpatialTablesCounter = getSpatialiteDbInfo()->dbNonSpatialTablesCount();
      }
#if 0
// TODO: resolve changed QMap-Type
      if ( getSpatialiteDbInfo()->dbVectorStylesCount() > 0 )
      {
        addTableEntryType( getSpatialiteDbInfo()->getDbVectorStylesInfo(), QgsSpatialiteDbInfoModel::EntryTypeMap, QgsSpatialiteDbInfo::StyleVector );
      }
      if ( getSpatialiteDbInfo()->dbRasterStylesCount() > 0 )
      {
        addTableEntryType( getSpatialiteDbInfo()->getDbRasterStylesInfo(), QgsSpatialiteDbInfoModel::EntryTypeMap, QgsSpatialiteDbInfo::StyleRaster );
      }
#endif
    }
    if ( mTableCounter > 0 )
    {
      bRc = true;
    }
    // QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem::sniffSpatialiteDbInfo bRc[%1] TableCounter[%2] [%3]" ).arg( bRc ).arg( getTableCounter() ).arg( mNonSpatialTablesCounter ), 7 );
  }
  return bRc;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::createGroupLayerItem
//-----------------------------------------------------------------
QgsSpatialiteDbInfoItem *QgsSpatialiteDbInfoItem::createGroupLayerItem( QgsSpatialiteDbInfo::SpatialiteLayerType layerType )
{
  QgsSpatialiteDbInfoItem::SpatialiteDbInfoItemType infoType = ItemTypeUnknown;
  QgsSpatialiteDbInfoItem *dbGroupLayerItem = nullptr;
  int iLayerOrMap = 0; // Map
  int iGroupCount = 0;
  QString sGroupCount = QString();
  QIcon iconGroup = QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( layerType );
  QString sTableType =  QString();
  QString sGroupName =  QString();
  QString sParentGroupName =  QString();
  QString sGroupSort =  QString();
  QString sTableName =  QString();
  QString sGroupFilter = QgsSpatialiteDbInfo::getSpatialiteTypesFilter( QgsSpatialiteDbInfo::SpatialiteLayerTypeName( layerType ) ) ;
  if ( QgsSpatialiteDbInfo::parseSpatialiteTableTypes( sGroupFilter, sTableType, sGroupName, sParentGroupName, sGroupSort, sTableName ) )
  {
    sGroupSort = QString( "%1_%2" ).arg( sGroupSort ).arg( sGroupName );
    switch ( layerType )
    {
      case QgsSpatialiteDbInfo::SpatialTable:
        infoType = QgsSpatialiteDbInfoItem::ItemTypeGroupSpatialTable;
        iLayerOrMap = 1; // Layer
        // sGroupSort = QString( "AAAA_%1" ).arg( sGroupName );
        break;
      case QgsSpatialiteDbInfo::SpatialView:
        infoType = QgsSpatialiteDbInfoItem::ItemTypeGroupSpatialView;
        iLayerOrMap = 1; // Layer
        // sGroupSort = QString( "AAAB_%1" ).arg( sGroupName );
#if 0
        if ( getSpatialiteDbInfo()->getListGroupNames().size() > 1 )
        {
          QIcon groupIconType = QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( QgsSpatialiteDbInfo::StyleVector );
          QMap<QString, int> mapGroupNames = getSpatialiteDbInfo()->getListGroupNames();
          for ( QMap<QString, int>::iterator itLayers = mapGroupNames.begin(); itLayers != mapGroupNames.end(); ++itLayers )
          {
            QString sSubGroupName =  itLayers.key();
            QString sSubGroupSort =  QString( "AZZZ_%1" ).arg( itLayers.key() );
            QString sSubGroupCount = QString( "%1 Layers" ).arg( itLayers.value() );
            QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem:createGroupLayerItem SpatialView[%1] Count[%2] " ).arg( sSubGroupName ).arg( sSubGroupCount ), 7 );
          }
        }
#endif
        break;
      case QgsSpatialiteDbInfo::VirtualShape:
        infoType = QgsSpatialiteDbInfoItem::ItemTypeGroupVirtualShape;
        iLayerOrMap = 1; // Layer
        // sGroupSort = QString( "AAAC_%1" ).arg( sGroupName );
        break;
      case QgsSpatialiteDbInfo::RasterLite1:
        infoType = QgsSpatialiteDbInfoItem::ItemTypeGroupRasterLite1;
        iLayerOrMap = 1; // Layer
        // sGroupSort = QString( "AAAD_%1" ).arg( sGroupName );
        break;
      case QgsSpatialiteDbInfo::RasterLite2Raster:
        infoType = QgsSpatialiteDbInfoItem::ItemTypeGroupRasterLite2Raster;
        iLayerOrMap = 1; // Layer
        // sGroupSort = QString( "AAAE_%1" ).arg( sGroupName );
        break;
      case QgsSpatialiteDbInfo::TopologyExport:
        infoType = QgsSpatialiteDbInfoItem::ItemTypeGroupTopologyExport;
        iLayerOrMap = 1; // Layer
        // sGroupSort = QString( "AAAF_%1" ).arg( sGroupName );
        break;
      case QgsSpatialiteDbInfo::MBTilesTable:
        sGroupName = QString( "MbTiles" ); // Instead of 'MBTilesTable'
        infoType = QgsSpatialiteDbInfoItem::ItemTypeGroupMBTilesTable;
        iLayerOrMap = 1; // Layer
        // sGroupSort = QString( "AABA_%1" ).arg( sGroupName );
        break;
      case QgsSpatialiteDbInfo::GeoPackageVector:
        infoType = QgsSpatialiteDbInfoItem::ItemTypeGroupGeoPackageVector;
        iLayerOrMap = 1; // Layer
        // sGroupSort = QString( "AABA_%1" ).arg( sGroupName );
        break;
      case QgsSpatialiteDbInfo::GeoPackageRaster:
        infoType = QgsSpatialiteDbInfoItem::ItemTypeGroupGeoPackageRaster;
        iLayerOrMap = 1; // Layer
        // sGroupSort = QString( "AABB_%1" ).arg( sGroupName );
        break;
      case QgsSpatialiteDbInfo::GdalFdoOgr:
        infoType = QgsSpatialiteDbInfoItem::ItemTypeGroupGdalFdoOgr;
        iLayerOrMap = 1; // Layer
        // sGroupSort = QString( "AABA_%1" ).arg( sGroupName );
        break;
      // Non-Spatialite-Types
      case QgsSpatialiteDbInfo::StyleVector:
      case QgsSpatialiteDbInfo::StyleRaster:
#if 0
        // TODO: add to NonSpatialTables
        // sGroupName = QString( "Se/Sld-Styles" );
        infoType = QgsSpatialiteDbInfoItem::ItemTypeGroupStyleRaster;
        iLayerOrMap = 0; // Map
        // sGroupSort = QString( "ZZCAA_%1" ).arg( sGroupName );
#endif
        break;
      case QgsSpatialiteDbInfo::NonSpatialTables:
        // when shown at end of list
        infoType = QgsSpatialiteDbInfoItem::ItemTypeGroupNonSpatialTables;
        iLayerOrMap = -1; // Dealt with internally when item is created
        // sGroupSort = QString( "AAYA_%1" ).arg( sGroupName );
        break;
      case QgsSpatialiteDbInfo::SpatialiteTopology:
      case QgsSpatialiteDbInfo::Metadata:
      case QgsSpatialiteDbInfo::RasterLite2Vector:
      case QgsSpatialiteDbInfo::MBTilesView:
      case QgsSpatialiteDbInfo::AllSpatialLayers:
      default:
        sGroupSort = QString( "AAZZ_%1" ).arg( sGroupName ); // Unknown at end
        break;
    }
    // QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem:createGroupLayerItem[%1] GoupeSort[%2]" ).arg( sGroupName ).arg( sGroupSort ), 7 );
    if ( infoType != QgsSpatialiteDbInfoItem::ItemTypeUnknown )
    {
      QMap<QString, QString> mapLayers = getSpatialiteDbInfo()->getDbLayersType( layerType );
      iGroupCount = mapLayers.count();
      if ( infoType != QgsSpatialiteDbInfoItem::ItemTypeGroupNonSpatialTables )
      {
        sGroupCount = QString( "%1 Layers" ).arg( iGroupCount );
        if ( iGroupCount < 2 )
        {
          sGroupCount = QString( "%1 Layer" ).arg( iGroupCount );
        }
      }
      else
      {
        sGroupCount = QString( "%1 Tables/Views" ).arg( iGroupCount );
      }
      for ( QMap<QString, QString>::iterator itLayers = mapLayers.begin(); itLayers != mapLayers.end(); ++itLayers )
      {
        // QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem:createGroupLayerItem.mapLayers Key[%1] Value[%2]" ).arg( itLayers.key() ).arg( itLayers.value() ), 7 );
        if ( !itLayers.key().isEmpty() )
        {
          if ( !dbGroupLayerItem )
          {
            dbGroupLayerItem = new QgsSpatialiteDbInfoItem( this, infoType );
            // Column 0: getTableNameIndex()
            dbGroupLayerItem->setText( getTableNameIndex(), sGroupName );
            dbGroupLayerItem->setIcon( getTableNameIndex(), iconGroup );
            // Column 1 [notused]: getGeometryNameIndex()
            // Column 2 [not used]: getGeometryTypeIndex()
            // Column 3: getSqlQueryIndex()
            dbGroupLayerItem->setText( getSqlQueryIndex(), sGroupCount );
            // Column 4: getColumnSortHidden()
            dbGroupLayerItem->setText( getColumnSortHidden(), sGroupSort );
            // QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem:createGroupLayerItem[%1] GroupCount[%2]" ).arg( sGroupName ).arg( sGroupCount ), 7 );
          }
          if ( iLayerOrMap  < 0 )
          {
            break;
          }
          switch ( iLayerOrMap )
          {
            case 0:
            {
              addTableItemMap( dbGroupLayerItem, itLayers.key(), itLayers.value(), layerType );
            }
            break;
            case 1:
            {
              bool bLoadLayer = true;
              QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem:createGroupLayerItem[%1] LayerName[%2]" ).arg( sGroupName ).arg( itLayers.key() ), 7 );
              QgsSpatialiteDbLayer *dbLayer = getSpatialiteDbInfo()->getQgsSpatialiteDbLayer( itLayers.key(), bLoadLayer );
              if ( ( dbLayer ) && ( dbLayer->isLayerValid() ) )
              {
                addTableItemLayer( dbGroupLayerItem, dbLayer );
              }
            }
            break;
            default:
              break;
          }
        }
      }
    }
    if ( dbGroupLayerItem )
    {
      // mPrepairedChildItems will be emptied
      dbGroupLayerItem->insertPrepairedChildren();
    }
  }
  return dbGroupLayerItem;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::addTableItemLayer
//-----------------------------------------------------------------
bool QgsSpatialiteDbInfoItem::addTableItemLayer( QgsSpatialiteDbInfoItem *dbGroupLayerItem, QgsSpatialiteDbLayer *dbLayer )
{
  bool bRc = false;
  if ( ( dbGroupLayerItem ) && ( dbLayer ) )
  {
    QgsSpatialiteDbInfoItem *dbLayerItem = new QgsSpatialiteDbInfoItem( dbGroupLayerItem, dbLayer );
    if ( dbLayerItem )
    {
      dbGroupLayerItem->mPrepairedChildItems.append( dbLayerItem );
    }
    bRc = true;
  }
  return  bRc;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::addTableItemMap
//-----------------------------------------------------------------
bool QgsSpatialiteDbInfoItem::addTableItemMap( QgsSpatialiteDbInfoItem *dbGroupLayerItem, QString sKey, QString sValue, QgsSpatialiteDbInfo::SpatialiteLayerType layerType )
{
  bool bRc = false;
  QgsSpatialiteDbInfoItem  *dbMapItem = nullptr;
  if ( ( dbGroupLayerItem ) && ( !sKey.isEmpty() ) && ( !sValue.isEmpty() ) )
  {
    switch ( layerType )
    {
      case QgsSpatialiteDbInfo::NonSpatialTables:
      {
        // dbMapItem = new QgsSpatialiteDbInfoItem( dbGroupLayerItem, ItemTypeNonSpatialTablesGroup, sKey, sValue );
        if ( dbMapItem )
        {
          dbGroupLayerItem->mPrepairedChildItems.append( dbMapItem );
        }
      }
      break;
      default:
        break;
    }
    if ( dbMapItem )
    {
      dbGroupLayerItem->mPrepairedChildItems.append( dbMapItem );
      bRc = true;
    }
  }
  return  bRc;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::buildDbInfoItem
// To Build the Database Item
// - setting Text and Icons
//-----------------------------------------------------------------
int QgsSpatialiteDbInfoItem::buildDbInfoItem()
{
  if ( ( getSpatialiteDbInfo() )  && ( mItemType == ItemTypeDb ) )
  {
    // Column 0: getTableNameIndex()
    setText( getTableNameIndex(), getSpatialiteDbInfo()->getFileName() );
    setIcon( getTableNameIndex(), getSpatialiteDbInfo()->getSpatialMetadataIcon() );
    setToolTip( getTableNameIndex(), QString( "in Directory: %1" ).arg( getSpatialiteDbInfo()->getDirectoryName() ) );
    // Column 1: getGeometryNameIndex()
    setText( getGeometryNameIndex(), getSpatialiteDbInfo()->dbSpatialMetadataString() );
    setIcon( getGeometryNameIndex(), getSpatialiteDbInfo()->getSpatialMetadataIcon() );
    setToolTip( getGeometryNameIndex(), QString( "the sqlite3 container type found was : %1" ).arg( getSpatialiteDbInfo()->dbSpatialMetadataString() ) );
    // Column 2: getGeometryTypeIndex()
    QString sProvider = QStringLiteral( "QgsSpatiaLiteProvider" );
    QString sInfoText = "";
    switch ( dbSpatialMetadata() )
    {
      case QgsSpatialiteDbInfo::SpatialiteFdoOgr:
        sProvider = QStringLiteral( "QgsOgrProvider" );
        sInfoText = QStringLiteral( "The 'SQLite' Driver is NOT active and cannot be displayed" );
        if ( getSpatialiteDbInfo()->hasDbFdoOgrDriver() )
        {
          sInfoText = QStringLiteral( "The 'SQLite' Driver is active and can be displayed" );
        }
        break;
      case QgsSpatialiteDbInfo::SpatialiteGpkg:
        sProvider = QStringLiteral( "QgsOgrProvider" );
        sInfoText = QStringLiteral( "The 'GPKG' Driver is NOT active and cannot be displayed" );
        if ( getSpatialiteDbInfo()->hasDbGdalGeoPackageDriver() )
        {
          sInfoText = QStringLiteral( "The 'GPKG' Driver is active and can be displayed" );
        }
        break;
      case QgsSpatialiteDbInfo::SpatialiteMBTiles:
        sProvider = QStringLiteral( "QgsGdalProvider" );
        sInfoText = QStringLiteral( "The 'MBTiles' Driver is NOT active and cannot be displayed" );
        if ( getSpatialiteDbInfo()->hasDbGdalMBTilesDriver() )
        {
          sInfoText = QStringLiteral( "The 'MBTiles' Driver is active and can be displayed" );
        }
        break;
      case QgsSpatialiteDbInfo::SpatialiteLegacy:
        if ( getSpatialiteDbInfo()->dbRasterLite1LayersCount() > 0 )
        {
          sProvider = QStringLiteral( "QgsGdalProvider" );
          sInfoText = QStringLiteral( "The 'RASTERLITE' Driver is NOT active and cannot be displayed" );
          if ( getSpatialiteDbInfo()->hasDbGdalRasterLite1Driver() )
          {
            sInfoText = QStringLiteral( "The 'RASTERLITE' Driver is active and can be displayed" );
          }
        }
        break;
      default:
        break;
    }
    QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem::buildDbInfoItem DbType[%1] Provider[%2] InfoText[%3]" ).arg( dbSpatialMetadataString() ).arg( sProvider ).arg( sInfoText ), 7 );
    if ( !sInfoText.isEmpty() )
    {
      sProvider = QStringLiteral( "%1 [%2]" ).arg( sProvider ).arg( sInfoText );
      setToolTip( getGeometryTypeIndex(), QString( "Driver information : %1" ).arg( sInfoText ) );
    }
    setText( getGeometryTypeIndex(), sProvider );
    setIcon( getGeometryTypeIndex(), QgsSpatialiteDbInfo::SpatialMetadataTypeIcon( getSpatialiteDbInfo()->dbSpatialMetadata() ) );
    // Column 3: getSqlQueryIndex()
    QString sLayerText = QStringLiteral( "%1 Spatial-Layers, %2 NonSpatial-Tables/Views" ).arg( getTableCounter() ).arg( getNonSpatialTablesCounter() );
    if ( mTableCounter == 1 )
    {
      sLayerText = QStringLiteral( "%1 Spatial-Layer, %2 NonSpatial-Tables/Views" ).arg( getTableCounter() ).arg( getNonSpatialTablesCounter() );
    }
    setText( getSqlQueryIndex(), sLayerText );
    // Column 4: getColumnSortHidden()
    QString sSortTag = QStringLiteral( "AAAA_%1" ).arg( getSpatialiteDbInfo()->getFileName() );
    setText( getColumnSortHidden(), sSortTag );
    QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem:: ItemTypeDb ProviderText[%1]" ).arg( sProvider ), 7 );
    if ( insertPrepairedChildren() )
    {
      QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem:: ItemTypeDb Children[%1]" ).arg( childCount() ), 7 );
    }
  }
  return childCount();
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::buildDbLayerItem
//-----------------------------------------------------------------
int QgsSpatialiteDbInfoItem::buildDbLayerItem()
{
  if ( ( getSpatialiteDbInfo() ) && ( getDbLayer() ) && ( mItemType == ItemTypeLayer ) )
  {
    QString sTableName = getDbLayer()->getTableName();
    QString sGeometryColumn = getDbLayer()->getGeometryColumn();
    QString sSortTag = QString( "AZZA_%1" ).arg( getDbLayer()->getLayerName() );
    QString sGeometryTypeString = getDbLayer()->getGeometryTypeString();
    QString sFeatureCount = QString();
    QString sViewTableName = QString();
    QString sTitle  = getDbLayer()->getTitle();
    QString sAbstract  = getDbLayer()->getAbstract();
    if ( mIsGeometryType )
    {
      // "SpatialView(Add, Insert, Delete | ReadOnly) 123 features"
      sFeatureCount = QString( "%1 %2 features" ).arg( getDbLayer()->getCapabilitiesString() ).arg( getDbLayer()->getFeaturesCount() );
      if ( mLayerType == QgsSpatialiteDbInfo::SpatialView )
      {
        sViewTableName = QString( " ViewTableName(%1)" ).arg( getDbLayer()->getViewTableName() );
      }
    }
    if ( !mIsGeometryType )
    {
      sGeometryColumn = sTitle;
      sGeometryTypeString = sAbstract;
      if ( mIsMbTiles )
      {
        sFeatureCount = QString( "Table-based MbTiles" );
        if ( mLayerType == QgsSpatialiteDbInfo::MBTilesView )
        {
          sFeatureCount = QString( "View-based MbTiles" );
        }
      }
    }
    // Column 0: getTableNameIndex()
    setText( getTableNameIndex(), sTableName );
    setIcon( getTableNameIndex(), getDbLayer()->getLayerTypeIcon() );
    // Column 1: getGeometryNameIndex()
    setText( getGeometryNameIndex(), sGeometryColumn );
    if ( mIsGeometryType )
    {
      // Add column information [this does not include the geometry of the Layer]
      int iColumsCount = addColumnItems();
      setToolTip( getTableNameIndex(), QString( "Fields count : %1%2" ).arg( ( iColumsCount + 1 ) ).arg( sViewTableName ) );
      setToolTip( getGeometryNameIndex(), sFeatureCount );
      setToolTip( getGeometryTypeIndex(), sTitle );
      setIcon( getGeometryTypeIndex(), getDbLayer()->getGeometryTypeIcon() );
    }
    else
    {
      // These can be long string descriptions, add ToolTip
      setText( getTableNameIndex(), sTableName );
      setToolTip( getTableNameIndex(), sTableName );
      setToolTip( getGeometryTypeIndex(), sGeometryTypeString );
      if ( mIsMbTiles )
      {
        addMbTilesMetadataItems();
        mParentItem->setText( getGeometryNameIndex(), sFeatureCount );
      }
      else
      {
        addCommonMetadataItems();
      }
    }
    // Column 2: getGeometryTypeIndex()
    setText( getGeometryTypeIndex(), sGeometryTypeString );
    // Column 3: getSqlQueryIndex()
    if ( mIsGeometryType )
    {
      setToolTip( getSqlQueryIndex(), sAbstract );
    }
    // Column 4: getColumnSortHidden()
    setText( getColumnSortHidden(), sSortTag );
    // QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem:: ItemTypeLayer LayerName[%1]" ).arg( getDbLayer()->getLayerName() ), 7 );
  }
  return childCount();
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::buildHelpItem
// Will create a 'help' text to explain the basic structure of
// what is shown in the QgsSpatiaLiteSourceSelect after a
// Database has been loaded.
// Will be remove after the first Database has been loaded.
// Note:
// If a 'double free or corruption (fasttop)' turns up when QGis is closed
// - look for a created Item that has NOT been added with 'addChild'
//-----------------------------------------------------------------
int QgsSpatialiteDbInfoItem::buildHelpItem()
{
  // Create text for Help
  if ( ( !getSpatialiteDbInfo() )  && ( mItemType == ItemTypeHelpRoot ) )
  {
    // Column 0: getTableNameIndex()
    int iSortCounter = 0;
    QString sText = QStringLiteral( "Help text for QgsSpatiaLiteSourceSelect" );
    setText( getTableNameIndex(), sText );
    setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialMetadataTypeIcon( QgsSpatialiteDbInfo::Spatialite50 ) );
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
    // Add NonSpatialTable to SubGroup NonSpatialTables
    dbHelpSubGroup->addChild( dbHelpNonSpatialTable );
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
    // Add NonSpatialTable to SubGroup NonSpatialTables
    dbHelpSubGroup->addChild( dbHelpNonSpatialTable );
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
    dbHelpProviders->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialMetadataTypeIcon( QgsSpatialiteDbInfo::Spatialite50 ) );
    dbHelpProviders->setBackground( getTableNameIndex(), QColor( "cadetblue" ) );
    dbHelpProviders->setForeground( getTableNameIndex(), QColor( "yellow" ) );
    dbHelpProviders->setToolTip( getTableNameIndex(), QString( "QgsSpatialiteProvider" ) );
    // Column 1: getGeometryNameIndex()
    sText = QStringLiteral( "Versions 2.4, 3.* [Legacy] until present" );
    dbHelpProviders->setText( getGeometryNameIndex(), sText );
    // Column 2: getGeometryTypeIndex()
    sText = QStringLiteral( "Vectors (Geometries)" );
    dbHelpProviders->setText( getGeometryTypeIndex(), sText );
    dbHelpProviders->setIcon( getGeometryTypeIndex(), QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( QgsSpatialiteDbInfo::SpatialTable ) );
    sSortTag = QStringLiteral( "AABA_%1" ).arg( iSortCounter++ );
    // Column 4: getColumnSortHidden()
    dbHelpProviders->setText( getColumnSortHidden(), sSortTag );
    dbHelpItem->addChild( dbHelpProviders );
    // new Child to be added to Provider
    dbHelpProviders = new QgsSpatialiteDbInfoItem( dbHelpItem, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
    // Column 0: getTableNameIndex()
    sText = QStringLiteral( "RasterLite2" );
    dbHelpProviders->setText( getTableNameIndex(), sText );
    dbHelpProviders->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialMetadataTypeIcon( QgsSpatialiteDbInfo::Spatialite50 ) );
    dbHelpProviders->setBackground( getTableNameIndex(), QColor( "lightyellow" ) );
    dbHelpProviders->setToolTip( getTableNameIndex(), QString( "QgsRasterite2Provider" ) );
    // Column 1: getGeometryNameIndex()
    sText = QStringLiteral( "Versions supported by QgsRasterite2Provider" );
    dbHelpProviders->setText( getGeometryNameIndex(), sText );
    // Column 2: getGeometryTypeIndex()
    sText = QStringLiteral( "Rasters" );
    dbHelpProviders->setText( getGeometryTypeIndex(), sText );
    dbHelpProviders->setIcon( getGeometryTypeIndex(), QgsSpatialiteDbInfo::SpatialiteLayerTypeIcon( QgsSpatialiteDbInfo::RasterLite2Raster ) );
    // Column 3 [not used]: getSqlQueryIndex()
    sSortTag = QStringLiteral( "AABA_%1" ).arg( iSortCounter++ );
    // Column 4: getColumnSortHidden()
    dbHelpProviders->setText( getColumnSortHidden(), sSortTag );
    dbHelpItem->addChild( dbHelpProviders );
    // new Child to be added to Provider
    dbHelpProviders = new QgsSpatialiteDbInfoItem( dbHelpItem, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
    // Column 0: getTableNameIndex()
    sText = QStringLiteral( "RasterLite1, GeoPackage-Rasters, MBTiles" );
    dbHelpProviders->setText( getTableNameIndex(), sText );
    dbHelpProviders->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialMetadataTypeIcon( QgsSpatialiteDbInfo::SpatialiteGpkg ) );
    dbHelpProviders->setBackground( getTableNameIndex(), QColor( "khaki" ) );
    dbHelpProviders->setToolTip( getTableNameIndex(), QString( "QgsGdalProvider" ) );
    dbHelpProviders->setToolTip( getGeometryNameIndex(), QString( "The structure will be shown, even if the if the needed driver is not active and cannot be shown" ) );
    dbHelpProviders->setToolTip( getGeometryTypeIndex(), QString( "A message will be shown with the name of the needed driver, if it is active and can be shown" ) );
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
    // Column 4: getColumnSortHidden()
    dbHelpProviders->setText( getColumnSortHidden(), sSortTag );
    dbHelpItem->addChild( dbHelpProviders );
    // new Child to be added to Provider
    dbHelpProviders = new QgsSpatialiteDbInfoItem( dbHelpItem, QgsSpatialiteDbInfoItem::ItemTypeHelpText );
    // Column 0: getTableNameIndex()
    sText = QStringLiteral( "FdoOgr, GeoPackage-Vectors" );
    dbHelpProviders->setText( getTableNameIndex(), sText );
    dbHelpProviders->setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::SpatialMetadataTypeIcon( QgsSpatialiteDbInfo::SpatialiteFdoOgr ) );
    dbHelpProviders->setBackground( getTableNameIndex(), QColor( "powderblue" ) );
    dbHelpProviders->setToolTip( getTableNameIndex(), QString( "QgsOgrProvider" ) );
    dbHelpProviders->setToolTip( getGeometryNameIndex(), QString( "The structure will be shown, even if the if the needed driver is not active and cannot be shown" ) );
    dbHelpProviders->setToolTip( getGeometryTypeIndex(), QString( "A message will be shown with the name of the needed driver, if it is active and can be shown" ) );
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
    // Column 4: getColumnSortHidden()
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
  return mPrepairedChildItems.count();
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::buildNonSpatialTablesGroup
// - Adding and filling NonSpatialTables Group and Sub-Groups
// -> placing the table into logical catagories
//-----------------------------------------------------------------
int QgsSpatialiteDbInfoItem::buildNonSpatialTablesGroups()
{
  int iGroupCount = 0;
  if ( ( getSpatialiteDbInfo() )  && ( mItemType == ItemTypeGroupNonSpatialTables ) )
  {
    QString sTableType;
    QString sGroupName;
    QString sParentGroupName;
    QString sGroupSort;
    QString sTableName;
    // Switch from a 'TableName' based QMap to a 'GroupName' based QMap
    bool bNonSpatialTablesGroups = true;
    // key: contains  GroupName; value: contains TableType,GroupName,ParentGroup-Name,SortKey,TableName
    QMultiMap<QString, QString> mapNonSpatialTables = getSpatialiteDbInfo()->getDbLayersType( QgsSpatialiteDbInfo::NonSpatialTables, bNonSpatialTablesGroups );
    // Extract the Unique List of Keys for create the Groups inside the 'NonSpatialTables' Group
    QStringList saListKeys = QStringList( mapNonSpatialTables.uniqueKeys() );
    QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem::buildNonSpatialTablesGroup: -1- count_list_keys[%1]" ).arg( saListKeys.count() ), 7 );
    for ( int i = 0; i < saListKeys.count(); i++ )
    {
      // Extract a a list of Tables belonging to the new Group
      QStringList saListValues = QStringList( mapNonSpatialTables.values( saListKeys.at( i ) ) );
      QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem::buildNonSpatialTablesGroup: -2- GroupName[%1] count_tables[%2]" ).arg( saListKeys.at( i ) ).arg( saListValues.size() ), 7 );
      if ( saListValues.size() > 0 )
      {
        if ( QgsSpatialiteDbInfo::parseSpatialiteTableTypes( saListValues.at( 0 ), sTableType, sGroupName, sParentGroupName, sGroupSort, sTableName ) )
        {
          if ( sParentGroupName.isEmpty() )
          {
            QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem::buildNonSpatialTablesGroup: -3a- Value[%1] GroupName[%3] ParentGroupName[%4] GroupSort[%5] TableType[%2] TableName[%6] saListValues.count[%7]" ).arg( saListValues.at( 0 ) ).arg( sTableType ).arg( sGroupName ).arg( sParentGroupName ).arg( sGroupSort ).arg( sTableName ).arg( saListValues.count() ), 7 );
            // will add a Group inside  the 'NonSpatialTables' Group, with a list of Tables belonging to that Group
            QgsSpatialiteDbInfoItem *dbGroupItem = new QgsSpatialiteDbInfoItem( this, ItemTypeNonSpatialTablesGroup, saListValues.at( 0 ), saListValues );
            if ( dbGroupItem )
            {
              mPrepairedChildItems.append( dbGroupItem );
            }
          }
          else
          {
            QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem::buildNonSpatialTablesGroup: -3b- Value[%1] GroupName[%3] ParentGroupName[%4] GroupSort[%5] TableType[%2]  TableName[%6] saListValues.count[%7]" ).arg( saListValues.at( 0 ) ).arg( sTableType ).arg( sGroupName ).arg( sParentGroupName ).arg( sGroupSort ).arg( sTableName ).arg( saListValues.count() ), 7 );
          }
        }
      }
    }
  }
  return iGroupCount;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::buildNonSpatialAdminTable
// - Adding Admin-Table with text and Icon
//-----------------------------------------------------------------
int QgsSpatialiteDbInfoItem::buildNonSpatialTablesGroup( QString sGroupInfo, QStringList saListValues )
{
  int iGroupCount = 0;
  if ( ( getSpatialiteDbInfo() )  && ( mItemType == ItemTypeNonSpatialTablesGroup ) )
  {
    QString sTableType;
    QString sGroupName;
    QString sParentGroupName;
    QString sGroupSort;
    QString sTableName;
    if ( QgsSpatialiteDbInfo::parseSpatialiteTableTypes( sGroupInfo, sTableType, sGroupName, sParentGroupName, sGroupSort, sTableName ) )
    {
      sGroupSort = QString( "%1_%2" ).arg( sGroupSort ).arg( sGroupName );
      QgsDebugMsgLevel( QString( "QgsSpatialiteDbInfoItem::buildNonSpatialTablesGroup:: building[%1] GroupName[%2] TableType[%3] saListValues.count[%4]" ).arg( sGroupInfo ).arg( sGroupName ).arg( sTableType ).arg( saListValues.count() ), 7 );
      setText( getTableNameIndex(), sGroupName );
      setIcon( getTableNameIndex(), QgsSpatialiteDbInfo::NonSpatialTablesTypeIcon( sGroupName ) );
      iGroupCount = saListValues.count(); // buildNonSpatialTablesGroup(sKey, sValue);
      // Column 1 [not used]: getGeometryNameIndex()
      // Column 2 [not used]: getGeometryTypeIndex()
      // Column 3 [not used]: getSqlQueryIndex()
      QString sGroupCount = QString( "%1 Tables/Views" ).arg( iGroupCount );
      setText( getSqlQueryIndex(), sGroupCount );
      // Column 4: getColumnSortHidden()
      // QString sGroupSort = QString( "AAYA_NonSpatialTables_%1" ).arg( sGroupName );
      setText( getColumnSortHidden(), sGroupSort );
      QStringList saParmValues;
      // saParmValues.append( sTableType );
      // saParmValues.append( sGroupSort );
      for ( int i = 0; i < saListValues.count(); i++ )
      {
        QgsSpatialiteDbInfoItem *dbAdminItem = new QgsSpatialiteDbInfoItem( this, ItemTypeAdmin, saListValues.at( i ), saParmValues );
        if ( dbAdminItem )
        {
          mPrepairedChildItems.append( dbAdminItem );
        }
      }
      // mPrepairedChildItems will be emptied
      insertPrepairedChildren();
    }
  }
  return iGroupCount;
}
//-----------------------------------------------------------------
// QgsSpatialiteDbInfoItem::buildNonSpatialAdminTable
// - Adding Admin-Table with text and Icon
//-----------------------------------------------------------------
int QgsSpatialiteDbInfoItem::buildNonSpatialAdminTable( QString sTableInfo, QStringList saListValues )
{
  int iGroupCount = 0;
  Q_UNUSED( saListValues );
  if ( ( getSpatialiteDbInfo() )  && ( mItemType == ItemTypeAdmin ) )
  {
    if ( saListValues.size() > 0 )
    {
      // for future use as extra parameter possiblity
    }
    QString sTableType;
    QString sGroupName;
    QString sParentGroupName;
    QString sGroupSort;
    QString sTableName;
    if ( QgsSpatialiteDbInfo::parseSpatialiteTableTypes( sTableInfo, sTableType, sGroupName, sParentGroupName, sGroupSort, sTableName ) )
    {
      sGroupSort = QString( "%1_%2_%3" ).arg( sGroupSort ).arg( sGroupName ).arg( sTableName );
      setText( getTableNameIndex(), sTableName );
      // Some of the Table-Names are long, use TooTip the see the name
      setToolTip( getTableNameIndex(), sTableName );
      // Column 1 [not used]: getGeometryNameIndex()
      setText( getGeometryNameIndex(), sTableType );
      setIcon( getGeometryNameIndex(), QgsSpatialiteDbInfo::NonSpatialTablesTypeIcon( sTableType ) );
      // Column 2 [not used]: getGeometryTypeIndex()
      // Column 3 [not used]: getSqlQueryIndex()
      // Column 4: getColumnSortHidden()
      setText( getColumnSortHidden(), sGroupSort );
      iGroupCount = 1;
    }
  }
  return iGroupCount;
}

