/***************************************************************************
     qgsgeorefdatapoint.cpp
     --------------------------------------
    Date                 : Sun Sep 16 12:02:45 AKDT 2007
    Copyright            : (C) 2007 by Gary E. Sherman
    Email                : sherman at mrcc dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <QPainter>

#include "qgsmapcanvas.h"
#include "qgsgcpcanvasitem.h"

#include "qgsgeorefdatapoint.h"

QgsGeorefDataPoint::QgsGeorefDataPoint( QgsMapCanvas* srcCanvas, QgsMapCanvas *dstCanvas,
                                        const QgsPoint& pixelCoords, const QgsPoint& mapCoords, int id_gcp,
                                        bool enable, int i_LegacyMode )
    : mSrcCanvas( srcCanvas )
    , mDstCanvas( dstCanvas )
    , mPixelCoords( pixelCoords )
    , mMapCoords( mapCoords )
    , mPixelCoords_reverse( QgsPoint(0.0, 0.0) )
    , mMapCoords_reverse( QgsPoint(0.0, 0.0) )
    , mId( id_gcp )
    , mEnabled( enable )
    , mLegacyMode( i_LegacyMode )
{
  mGCPSourceItem = nullptr;
  mGCPDestinationItem = nullptr;
  switch ( mLegacyMode )
  {
    case 0:
      mGCPSourceItem = new QgsGCPCanvasItem( srcCanvas, this, true, mLegacyMode );
      mGCPDestinationItem = new QgsGCPCanvasItem( dstCanvas, this, false, mLegacyMode );
      mGCPSourceItem->setEnabled( enable );
      mGCPDestinationItem->setEnabled( enable );
      mGCPSourceItem->show();
      mGCPDestinationItem->show();
      break;
    case 1:
      mGCPSourceItem = new QgsGCPCanvasItem( srcCanvas, this, true, mLegacyMode );
      mGCPSourceItem->setEnabled( enable );
      mGCPSourceItem->show();
      break;
  }

}

QgsGeorefDataPoint::QgsGeorefDataPoint( const QgsGeorefDataPoint &data_point )
    : QObject()
    , mSrcCanvas( nullptr )
    , mDstCanvas( nullptr )
    , mGCPSourceItem( nullptr )
    , mGCPDestinationItem( nullptr )
{
  Q_UNUSED( data_point );
  // we share item representation on canvas between all points
//  mGCPSourceItem = new QgsGCPCanvasItem(p.srcCanvas(), p.pixelCoords(), p.mapCoords(), p.isEnabled());
//  mGCPDestinationItem = new QgsGCPCanvasItem(p.dstCanvas(), p.pixelCoords(), p.mapCoords(), p.isEnabled());
  mPixelCoords = data_point.pixelCoords();
  mPixelCoords_reverse = data_point.pixelCoordsReverse();
  mMapCoords = data_point.mapCoords();
  mMapCoords_reverse = data_point.mapCoordsReverse();
  mEnabled = data_point.isEnabled();
  mResidual = data_point.residual();
  mId = data_point.id();
}

QgsGeorefDataPoint::~QgsGeorefDataPoint()
{
  delete mGCPSourceItem;
  delete mGCPDestinationItem;
}

void QgsGeorefDataPoint::setPixelCoords( const QgsPoint &pixel_point )
{
  mPixelCoords = pixel_point;
  if ( mGCPSourceItem )
  {
    mGCPSourceItem->update();
  }
  if ( mGCPDestinationItem )
  {
    mGCPDestinationItem->update();
  }
}

void QgsGeorefDataPoint::setMapCoords( const QgsPoint &map_point )
{
  mMapCoords = map_point;
  if ( mGCPSourceItem )
  {
    mGCPSourceItem->update();
  }
  if ( mGCPDestinationItem )
  {
    mGCPDestinationItem->update();
  }
}

void QgsGeorefDataPoint::setEnabled( bool enabled )
{
  mEnabled = enabled;
  if ( mGCPSourceItem )
  {
    mGCPSourceItem->update();
  }
}

void QgsGeorefDataPoint::setId( int id )
{
  mId = id;
  if ( mGCPSourceItem )
  {
    mGCPSourceItem->update();
  }
  if ( mGCPDestinationItem )
  {
    mGCPDestinationItem->update();
  }
}

void QgsGeorefDataPoint::setResidual( QPointF r )
{
  mResidual = r;
  if ( mGCPSourceItem )
  {
    mGCPSourceItem->checkBoundingRectChange();
  }
}

void QgsGeorefDataPoint::updateCoords()
{
  if ( mGCPSourceItem )
  {
    mGCPSourceItem->updatePosition();
    mGCPSourceItem->update();
  }
  if ( mGCPDestinationItem )
  {
    mGCPDestinationItem->updatePosition();
    mGCPDestinationItem->update();
  }
}

bool QgsGeorefDataPoint::contains( QPoint search_point, bool isMapPlugin )
{
  if ( ! mGCPSourceItem )
    return false;
  if ( isMapPlugin )
  {
    QPointF pnt = mGCPSourceItem->mapFromScene( search_point );
    return mGCPSourceItem->shape().contains( pnt );
  }
  else
  {
    if ( ! mGCPDestinationItem )
      return false;
    QPointF pnt = mGCPDestinationItem->mapFromScene( search_point );
    return mGCPDestinationItem->shape().contains( pnt );
  }
}

void QgsGeorefDataPoint::moveTo( QPoint p, bool isMapPlugin )
{
  if ( ! mGCPSourceItem )
    return;
  if ( isMapPlugin )
  {
    QgsPoint pnt = mGCPSourceItem->toMapCoordinates( p );
    mPixelCoords = pnt;
  }
  else
  {
    if ( mGCPDestinationItem )
    {
      QgsPoint pnt = mGCPDestinationItem->toMapCoordinates( p );
      mMapCoords = pnt;
    }
  }
  mGCPSourceItem->update();
  if ( mGCPDestinationItem )
  {
    mGCPDestinationItem->update();
  }
  updateCoords();
}
