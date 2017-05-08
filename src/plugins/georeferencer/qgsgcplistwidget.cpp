/***************************************************************************
    qgsgcplistwidget.cpp - Widget for GCP list display
     --------------------------------------
    Date                 : 27-Feb-2009
    Copyright            : (c) 2009 by Manuel Massing
    Email                : m.massing at warped-space.de
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QHeaderView>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QMenu>
#include <QSortFilterProxyModel>

#include "qgsgeorefdelegates.h"
#include "qgsgeorefdatapoint.h"
#include "qgsgcplist.h"
#include "qgsgcplistwidget.h"
#include "qgsgcplistmodel.h"

QgsGCPListWidget::QgsGCPListWidget( QWidget *parent )
  : QTableView( parent )
  , mGCPListModel( new QgsGCPListModel( this ) )
  , mNonEditableDelegate( new QgsNonEditableDelegate( this ) )
  , mDmsAndDdDelegate( new QgsDmsAndDdDelegate( this ) )
  , mCoordDelegate( new QgsCoordDelegate( this ) )
{
  // Create a proxy model, which will handle dynamic sorting
  QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel( this );
  proxyModel->setSourceModel( mGCPListModel );
  proxyModel->setDynamicSortFilter( true );
  proxyModel->setSortRole( Qt::UserRole );
  setModel( proxyModel );
  setSortingEnabled( true );

  setContextMenuPolicy( Qt::CustomContextMenu );
  setFocusPolicy( Qt::NoFocus );

  verticalHeader()->hide();
  setAlternatingRowColors( true );

  // set delegates for items
  setItemDelegateForColumn( 1, mNonEditableDelegate ); // id
  setItemDelegateForColumn( 2, mNonEditableDelegate ); // dX
  setItemDelegateForColumn( 3, mNonEditableDelegate ); // dY
  setItemDelegateForColumn( 4, mNonEditableDelegate ); // residual
  setItemDelegateForColumn( 5, mNonEditableDelegate ); // pixel azimuth
  setItemDelegateForColumn( 6, mNonEditableDelegate ); // map azimuth
  setItemDelegateForColumn( 7, mNonEditableDelegate ); // name+notes
  setItemDelegateForColumn( 8, mNonEditableDelegate ); // pixel WKT
  setItemDelegateForColumn( 9, mNonEditableDelegate ); // map WKT
  //setItemDelegateForColumn( 2, mCoordDelegate ); // srcX
  //setItemDelegateForColumn( 3, mCoordDelegate ); // srcY
  //setItemDelegateForColumn( 4, mDmsAndDdDelegate ); // dstX
  //setItemDelegateForColumn( 5, mDmsAndDdDelegate ); // dstY
  // setItemDelegateForColumn( 9, mNonEditableDelegate ); // pixel distance
  //setItemDelegateForColumn( 12, mNonEditableDelegate ); // map distance


  connect( this, &QgsGCPListWidget::replaceDataPoint,
           mGCPListModel, &QgsGCPListModel::replaceDataPoint );

  connect( this, &QAbstractItemView::doubleClicked,
           this, &QgsGCPListWidget::itemDoubleClicked );
  connect( this, &QAbstractItemView::clicked,
           this, &QgsGCPListWidget::itemClicked );
  connect( this, &QWidget::customContextMenuRequested,
           this, &QgsGCPListWidget::showContextMenu );

  // connect( mDmsAndDdDelegate, &QAbstractItemDelegate::closeEditor,  this, &QgsGCPListWidget::updateItemCoords );
  // connect( mCoordDelegate, &QAbstractItemDelegate::closeEditor, this, &QgsGCPListWidget::updateItemCoords );
}


void QgsGCPListWidget::setGCPList( QgsGCPList *gcpList )
{
  mGCPListModel->setGCPList( gcpList );
  adjustTableContent();
}

void QgsGCPListWidget::setGeorefTransform( QgsGeorefTransform *theGeorefTransform )
{
  mGCPListModel->setGeorefTransform( theGeorefTransform );
  adjustTableContent();
}

void QgsGCPListWidget::updateGCPList()
{
  if ( isDirty() )
  {
    mGCPListModel->updateModel();
    adjustTableContent();
  }
}

void QgsGCPListWidget::closeEditors()
{
  Q_FOREACH ( const QModelIndex &index, selectedIndexes() )
  {
    closePersistentEditor( index );
  }
}

void QgsGCPListWidget::itemDoubleClicked( QModelIndex index )
{
  index = static_cast<const QSortFilterProxyModel *>( model() )->mapToSource( index );
  QgsGeorefDataPoint *dataPoint = getGCPList()->at( index.row() );
  if ( dataPoint )
  {
    emit jumpToGCP( dataPoint );
  }
}

void QgsGCPListWidget::itemClicked( QModelIndex index )
{
  index = static_cast<const QSortFilterProxyModel *>( model() )->mapToSource( index );
  QStandardItem *item = mGCPListModel->item( index.row(), index.column() );
  if ( item->isCheckable() )
  {
    QgsGeorefDataPoint *dataPoint = getGCPList()->at( index.row() );
    if ( item->checkState() == Qt::Checked )
    {
      dataPoint->setEnabled( true );
    }
    else // Qt::Unchecked
    {
      dataPoint->setEnabled( false );
    }

    mGCPListModel->updateModel();
    emit pointEnabled( dataPoint, index.row() );
    adjustTableContent();
  }

  mPrevRow = index.row();
  mPrevColumn = index.column();
}

void QgsGCPListWidget::updateItemCoords( QWidget *editor )
{
  Q_UNUSED( editor );
  return;
  // editing of Points no longer supported.
  QLineEdit *lineEdit = qobject_cast<QLineEdit *>( editor );
  QgsGeorefDataPoint *dataPoint = getGCPList()->at( mPrevRow );
  if ( lineEdit )
  {
    double value = lineEdit->text().toDouble();
    QgsPointXY newMapCoords( dataPoint->mapCoords() );
    QgsPointXY newPixelCoords( dataPoint->pixelCoords() );
    if ( mPrevColumn == 2 ) // srcX
    {
      newPixelCoords.setX( value );
    }
    else if ( mPrevColumn == 3 ) // srcY
    {
      newPixelCoords.setY( value );
    }
    else if ( mPrevColumn == 4 ) // dstX
    {
      newMapCoords.setX( value );
    }
    else if ( mPrevColumn == 5 ) // dstY
    {
      newMapCoords.setY( value );
    }
    else
    {
      return;
    }

    dataPoint->setPixelCoords( newPixelCoords );
    dataPoint->setMapCoords( newMapCoords );
  }

  updateGCPList();
}

void QgsGCPListWidget::showContextMenu( QPoint p )
{
  if ( !getGCPList() || 0 == getGCPList()->count() )
    return;

  QMenu m;// = new QMenu(this);
  QModelIndex index = indexAt( p );
  if ( index == QModelIndex() )
    return;

  // Select the right-clicked item
  setCurrentIndex( index );

  QAction *jumpToPointAction = new QAction( tr( "Recenter" ), this );
  connect( jumpToPointAction, &QAction::triggered, this, &QgsGCPListWidget::jumpToPoint );
  m.addAction( jumpToPointAction );

  QAction *removeAction = new QAction( tr( "Remove" ), this );
  connect( removeAction, &QAction::triggered, this, &QgsGCPListWidget::removeRow );
  m.addAction( removeAction );
  m.exec( QCursor::pos(), removeAction );

  index = static_cast<const QSortFilterProxyModel *>( model() )->mapToSource( index );
  mPrevRow = index.row();
  mPrevColumn = index.column();
}

void QgsGCPListWidget::removeRow()
{
  QModelIndex index = static_cast<const QSortFilterProxyModel *>( model() )->mapToSource( currentIndex() );
  emit deleteDataPoint( index.row() );
}

void QgsGCPListWidget::editCell()
{
  edit( currentIndex() );
}

void QgsGCPListWidget::jumpToPoint()
{
  QModelIndex index = static_cast<const QSortFilterProxyModel *>( model() )->mapToSource( currentIndex() );
  QgsGeorefDataPoint *dataPoint = getGCPList()->at( index.row() );
  if ( dataPoint )
  {
    emit jumpToGCP( dataPoint );
  }
}

void QgsGCPListWidget::adjustTableContent()
{
  resizeColumnsToContents();
  resizeRowsToContents();
}
