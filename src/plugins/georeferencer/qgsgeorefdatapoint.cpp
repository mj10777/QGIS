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
                                        const QgsPoint& pixelCoords, const QgsPoint& mapCoords, int i_srid, int id_gcp,
                                        bool enable, int iLegacyMode )
    : mSrcCanvas( srcCanvas )
    , mDstCanvas( dstCanvas )
    , mPixelCoords( pixelCoords )
    , mMapCoords( mapCoords )
    , mPixelCoordsReverse( QgsPoint( 0.0, 0.0 ) )
    , mMapCoords_reverse( QgsPoint( 0.0, 0.0 ) )
    , mId( id_gcp )
    , mEnabled( enable )
    , mLegacyMode( iLegacyMode )
    , mTextInfo( "" )
    , mSrid( i_srid )
    , mParseString( ";#;#" )
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

QgsGeorefDataPoint::QgsGeorefDataPoint( const QgsPoint& pixelCoords, const QgsPoint& mapCoords, int i_srid, int id_gcp,
                                        int id_gcp_master, int iResultType, QString sTextInfo )
    : QObject()
    , mSrcCanvas( nullptr )
    , mDstCanvas( nullptr )
    , mGCPSourceItem( nullptr )
    , mGCPDestinationItem( nullptr )
    , mPixelCoords( pixelCoords )
    , mMapCoords( mapCoords )
    , mPixelCoordsReverse( QgsPoint( 0.0, 0.0 ) )
    , mMapCoords_reverse( QgsPoint( 0.0, 0.0 ) )
    , mId( id_gcp )
    , mLegacyMode( 1 )
    , mTextInfo( sTextInfo )
    , mIdMaster( id_gcp_master )
    , mResultType( iResultType )
    , mSrid( i_srid )
    , mParseString( ";#;#" )
{
  if ( mResultType == 1 )
  {
    mEnabled = true;
  }
}

QgsGeorefDataPoint::QgsGeorefDataPoint( const QgsGeorefDataPoint &dataPoint )
    : QObject()
    , mTextInfo( "" )
    , mParseString( ";#;#" )
{
  Q_UNUSED( dataPoint );
  // we share item representation on canvas between all points
//  mGCPSourceItem = new QgsGCPCanvasItem(p.srcCanvas(), p.pixelCoords(), p.mapCoords(), p.isEnabled());
//  mGCPDestinationItem = new QgsGCPCanvasItem(p.dstCanvas(), p.pixelCoords(), p.mapCoords(), p.isEnabled());
  mPixelCoords = dataPoint.pixelCoords();
  mPixelCoordsReverse = dataPoint.pixelCoordsReverse();
  mMapCoords = dataPoint.mapCoords();
  mMapCoords_reverse = dataPoint.mapCoordsReverse();
  mEnabled = dataPoint.isEnabled();
  mResidual = dataPoint.residual();
  mId = dataPoint.id();
  mSrid = dataPoint.getSrid();
}

QgsGeorefDataPoint::~QgsGeorefDataPoint()
{
  delete mGCPSourceItem;
  delete mGCPDestinationItem;
}

void QgsGeorefDataPoint::setPixelCoords( const QgsPoint &pixelPoint )
{
  mPixelCoords = pixelPoint;
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

bool QgsGeorefDataPoint::contains( QPoint searchPoint, bool isMapPlugin )
{
  if ( ! mGCPSourceItem )
    return false;
  if ( isMapPlugin )
  {
    QPointF pnt = mGCPSourceItem->mapFromScene( searchPoint );
    return mGCPSourceItem->shape().contains( pnt );
  }
  else
  {
    if ( ! mGCPDestinationItem )
      return false;
    QPointF pnt = mGCPDestinationItem->mapFromScene( searchPoint );
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

QString QgsGeorefDataPoint::AsEWKT( int iPointType, int i_srid ) const
{
  QString s_EWKT = QString( "SRID=%1;%2" );
  switch ( iPointType )
  {
    case 0:
      s_EWKT = QString( s_EWKT ).arg( -1 ).arg( mPixelCoords.wellKnownText() );
      break;
    case 2:
      s_EWKT = QString( s_EWKT ).arg( -1 ).arg( mPixelCoordsReverse.wellKnownText() );
      break;
    case 3:
    {
      if (( i_srid > 0 ) && ( i_srid != getSrid() ) )
      {
        s_EWKT = QString("ST_Transform(%1,%2)").arg(QString( s_EWKT ).arg( i_srid ).arg( mMapCoords_reverse.wellKnownText() )).arg(getSrid());
      }
      else
      {
        s_EWKT = QString( s_EWKT ).arg( getSrid() ).arg( mMapCoords_reverse.wellKnownText() );
      }
    }
    break;
    case 1:
    default:
    {
      if (( i_srid > 0 ) && ( i_srid != getSrid() ) )
      {
        s_EWKT = QString("ST_Transform(%1,%2)").arg(QString( s_EWKT ).arg( i_srid ).arg( mMapCoords.wellKnownText() )).arg(getSrid());
      }
      else
      {
        s_EWKT = QString( s_EWKT ).arg( getSrid() ).arg( mMapCoords.wellKnownText() );
      }
    }
    break;
  }
  return s_EWKT;
}
QString QgsGeorefDataPoint::sqlInsertPointsCoverage( int i_srid ) const
{
  QString s_sql_insert = "";
  QStringList sa_fields = getTextInfo().split( mParseString ); // 10
  if ( sa_fields.count() == 7 )
  { // SELECT id_gcp, id_gcp_coverage, name, notes, pixel_x, pixel_y, srid, point_x, point_y, order_selected, gcp_enable, gcp_text, gcp_pixel, gcp_point
    QString sPointsTableName = sa_fields.at( 2 );
    switch ( getResultType() )
    {
      case 3: // INSERT
        s_sql_insert  = QString( "INSERT INTO \"%1\"\n(id_gcp_coverage,name,notes,srid,gcp_enable,order_selected,gcp_text,gcp_pixel,gcp_point)\n " ).arg( sPointsTableName );
        s_sql_insert += QString( "SELECT %1 AS id_gcp_coverage," ).arg( sa_fields.at( 1 ) );
        s_sql_insert += QString( "'%1' AS name," ).arg( sa_fields.at( 3 ) );
        s_sql_insert += QString( "'%1' AS notes," ).arg( sa_fields.at( 4 ) );
        s_sql_insert += QString( "%1 AS srid," ).arg( getSrid() );
        s_sql_insert += QString( "%1 AS gcp_enable," ).arg(( int )isEnabled() );
        s_sql_insert += QString( "%1 AS order_selected," ).arg( sa_fields.at( 5 ) );
        s_sql_insert += QString( "'%1' AS gcp_text," ).arg( sa_fields.at( 6 ) );
        s_sql_insert += QString( "%1 AS gcp_pixel," ).arg( GeomFromEWKT( 0 ) );
        s_sql_insert += QString( "%1 AS gcp_point" ).arg( GeomFromEWKT( 1, i_srid ) );
        break;
      case 2: // UPDATE
        s_sql_insert  = QString( "UPDATE \"%1\" SET gcp_point=%2,name='%3', notes='%4', gcp_text='%5' WHERE (id_gcp=%6);" ).arg( sPointsTableName ).arg( GeomFromEWKT( 1 , i_srid) ).arg( sa_fields.at( 3 ) ).arg( sa_fields.at( 4 ) ).arg( sa_fields.at( 6 ) ).arg( sa_fields.at( 0 ) );
        break;
    }
  }
  return s_sql_insert;
}
