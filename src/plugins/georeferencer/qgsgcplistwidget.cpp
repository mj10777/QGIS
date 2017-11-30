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

QgsGcpListWidget::QgsGcpListWidget( QWidget *parent )
  : QTableView( parent )
  , mGcpListModel( new QgsGcpListModel( this ) )
  , mNonEditableDelegate( new QgsNonEditableDelegate( this ) )
  , mDmsAndDdDelegate( new QgsDmsAndDdDelegate( this ) )
  , mCoordDelegate( new QgsCoordDelegate( this ) )
{
  // Create a proxy model, which will handle dynamic sorting
  QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel( this );
  proxyModel->setSourceModel( mGcpListModel );
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


  connect( this, &QgsGcpListWidget::replaceDataPoint,
           mGcpListModel, &QgsGcpListModel::replaceDataPoint );

  connect( this, &QAbstractItemView::doubleClicked,
           this, &QgsGcpListWidget::itemDoubleClicked );
  connect( this, &QAbstractItemView::clicked,
           this, &QgsGcpListWidget::itemClicked );
  connect( this, &QWidget::customContextMenuRequested,
           this, &QgsGcpListWidget::showContextMenu );

  // connect( mDmsAndDdDelegate, &QAbstractItemDelegate::closeEditor,  this, &QgsGcpListWidget::updateItemCoords );
  // connect( mCoordDelegate, &QAbstractItemDelegate::closeEditor, this, &QgsGcpListWidget::updateItemCoords );
}


void QgsGcpListWidget::setGcpList( QgsGcpList *gcpList )
{
  mGcpListModel->setGcpList( gcpList );
  adjustTableContent();
}

void QgsGcpListWidget::setGeorefTransform( QgsGeorefTransform *theGeorefTransform )
{
  mGcpListModel->setGeorefTransform( theGeorefTransform );
  adjustTableContent();
}

void QgsGcpListWidget::updateGcpList()
{
  if ( isDirty() )
  {
    mGcpListModel->updateModel();
    adjustTableContent();
  }
}

void QgsGcpListWidget::closeEditors()
{
  Q_FOREACH ( const QModelIndex &index, selectedIndexes() )
  {
    closePersistentEditor( index );
  }
}

void QgsGcpListWidget::itemDoubleClicked( QModelIndex index )
{
  index = static_cast<const QSortFilterProxyModel *>( model() )->mapToSource( index );
  QgsGeorefDataPoint *dataPoint = getGcpList()->at( index.row() );
  if ( dataPoint )
  {
    emit jumpToGCP( dataPoint );
  }
}

void QgsGcpListWidget::itemClicked( QModelIndex index )
{
  index = static_cast<const QSortFilterProxyModel *>( model() )->mapToSource( index );
  QStandardItem *item = mGcpListModel->item( index.row(), index.column() );
  if ( item->isCheckable() )
  {
    QgsGeorefDataPoint *dataPoint = getGcpList()->at( index.row() );
    if ( item->checkState() == Qt::Checked )
    {
      dataPoint->setEnabled( true );
    }
    else // Qt::Unchecked
    {
      dataPoint->setEnabled( false );
    }

    mGcpListModel->updateModel();
    emit pointEnabled( dataPoint, index.row() );
    adjustTableContent();
  }

  mPrevRow = index.row();
  mPrevColumn = index.column();
}

void QgsGcpListWidget::updateItemCoords( QWidget *editor )
{
  Q_UNUSED( editor );
  return;
  // editing of Points no longer supported.
  QLineEdit *lineEdit = qobject_cast<QLineEdit *>( editor );
  QgsGeorefDataPoint *dataPoint = getGcpList()->at( mPrevRow );
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

  updateGcpList();
}

void QgsGcpListWidget::showContextMenu( QPoint p )
{
  if ( !getGcpList() || 0 == getGcpList()->count() )
    return;

  QMenu m;// = new QMenu(this);
  QModelIndex index = indexAt( p );
  if ( index == QModelIndex() )
    return;

  // Select the right-clicked item
  setCurrentIndex( index );

  QAction *jumpToPointAction = new QAction( tr( "Recenter" ), this );
  connect( jumpToPointAction, &QAction::triggered, this, &QgsGcpListWidget::jumpToPoint );
  m.addAction( jumpToPointAction );

  QAction *removeAction = new QAction( tr( "Remove" ), this );
  connect( removeAction, &QAction::triggered, this, &QgsGcpListWidget::removeRow );
  m.addAction( removeAction );
  m.exec( QCursor::pos(), removeAction );

  index = static_cast<const QSortFilterProxyModel *>( model() )->mapToSource( index );
  mPrevRow = index.row();
  mPrevColumn = index.column();
}

void QgsGcpListWidget::removeRow()
{
  QModelIndex index = static_cast<const QSortFilterProxyModel *>( model() )->mapToSource( currentIndex() );
  emit deleteDataPoint( index.row() );
}

void QgsGcpListWidget::editCell()
{
  edit( currentIndex() );
}

void QgsGcpListWidget::jumpToPoint()
{
  QModelIndex index = static_cast<const QSortFilterProxyModel *>( model() )->mapToSource( currentIndex() );
  QgsGeorefDataPoint *dataPoint = getGcpList()->at( index.row() );
  if ( dataPoint )
  {
    emit jumpToGCP( dataPoint );
  }
}

void QgsGcpListWidget::adjustTableContent()
{
  resizeColumnsToContents();
  resizeRowsToContents();
}
