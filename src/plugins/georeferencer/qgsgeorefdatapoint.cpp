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

#include "qgsgeorefdatapoint.h"

QgsGeorefDataPoint::QgsGeorefDataPoint( QgsMapCanvas *srcCanvas, QgsMapCanvas *dstCanvas,
                                        const QgsPointXY &pixelCoords, const QgsPointXY &mapCoords, int i_srid, int id_gcp,
                                        bool enable )
  : mSrcCanvas( srcCanvas )
  , mDstCanvas( dstCanvas )
  , mPixelCoords( pixelCoords )
  , mMapCoords( mapCoords )
  , mPixelCoordsReverse( QgsPointXY( 0.0, 0.0 ) )
  , mMapCoordsReverse( QgsPointXY( 0.0, 0.0 ) )
  , mId( id_gcp )
  , mEnabled( enable )
  , mTextInfo( "" )
  , mSrid( i_srid )
  , mParseString( ";#;#" )
{
}

QgsGeorefDataPoint::QgsGeorefDataPoint( const QgsPointXY &pixelCoords, const QgsPointXY &mapCoords, int i_srid, int id_gcp,
                                        int id_gcp_master, int iResultType, QString sTextInfo )
  : QObject()
  , mSrcCanvas( nullptr )
  , mDstCanvas( nullptr )
  , mPixelCoords( pixelCoords )
  , mMapCoords( mapCoords )
  , mPixelCoordsReverse( QgsPointXY( 0.0, 0.0 ) )
  , mMapCoordsReverse( QgsPointXY( 0.0, 0.0 ) )
  , mId( id_gcp )
  , mTextInfo( sTextInfo )
  , mName( "" )
  , mNotes( "" )
  , mIdMaster( id_gcp_master )
  , mResultType( iResultType )
  , mSrid( i_srid )
  , mParseString( ";#;#" )
{
  if ( mResultType == 1 )
  {
    mEnabled = true;
  }
  // parse and set mName and mNotes
  setTextInfo( sTextInfo );
}

QgsGeorefDataPoint::QgsGeorefDataPoint( QgsGeorefDataPoint &dataPoint )
  : QObject()
  , mTextInfo( "" )
  , mName( "" )
  , mNotes( "" )
  , mParseString( ";#;#" )
{
  // we share item representation on canvas between all points
  mPixelCoords = dataPoint.pixelCoords();
  mPixelCoordsReverse = dataPoint.pixelCoordsReverse();
  mMapCoords = dataPoint.mapCoords();
  mMapCoordsReverse = dataPoint.mapCoordsReverse();
  mEnabled = dataPoint.isEnabled();
  mResidual = dataPoint.residual();
  mId = dataPoint.id();
  mSrid = dataPoint.getSrid();
  mTextInfo = dataPoint.getTextInfo();
  mName = dataPoint.name();
  mNotes = dataPoint.notes();
}


QgsGeorefDataPoint::~QgsGeorefDataPoint()
{
}
QString QgsGeorefDataPoint::getTextInfo()
{
  if ( mTextInfo.isEmpty() )
  {
    mTextInfo = QString( "%2%1%1%1%3%1%4%1" ).arg( mParseString ).arg( mId ).arg( mName ).arg( mNotes );
  }
  return mTextInfo;
}
void QgsGeorefDataPoint::setTextInfo( QString sTextInfo )
{
  mTextInfo = sTextInfo;
  QStringList sa_fields = sTextInfo.split( mParseString ); // 10
  if ( sa_fields.count() >= 5 )
  {
    mName = sa_fields.at( 3 ).trimmed();
    mNotes = sa_fields.at( 4 ).trimmed();
  }
}

void QgsGeorefDataPoint::setPixelCoords( const QgsPointXY &pixelPoint )
{
  mPixelCoords = pixelPoint;
}

void QgsGeorefDataPoint::setMapCoords( const QgsPointXY &map_point )
{
  mMapCoords = map_point;
}

void QgsGeorefDataPoint::updateDataPoint( bool bPointMap, QgsPoint updatePoint )
{
  // iPointType: 0=pixel ; 1 = map
  if ( bPointMap )
  {
    setMapCoords( updatePoint );
  }
  else
  {
    setPixelCoords( updatePoint );
  }
}

void QgsGeorefDataPoint::setEnabled( bool enabled )
{
  mEnabled = enabled;
}

void QgsGeorefDataPoint::setId( int id )
{
  mId = id;
}

void QgsGeorefDataPoint::setResidual( QPointF r )
{
  mResidual = r;
}

bool QgsGeorefDataPoint::contains( QPointXY searchPoint, bool isPixelPoint )
{
  if ( isPixelPoint )
  {
    if ( mSrcCanvas )
    {
      return mPixelCoords.compare( QgsPointXY( searchPoint ) );
    }
    return false;
  }
  else
  {
    if ( mDstCanvas )
    {
      return mMapCoords.compare( QgsPointXY( searchPoint ) );
    }
    return false;
  }
}

void QgsGeorefDataPoint::moveTo( bool isPixelPoint )
{
  QgsMapCanvas *useCanvas = mSrcCanvas;
  QgsPointXY new_center = mPixelCoords;
  if ( !isPixelPoint )
  {
    useCanvas = mDstCanvas;
    new_center = mMapCoords;
  }
  if ( useCanvas )
  {
    QgsRectangle ext = useCanvas->extent();
    QgsPointXY center = ext.center();
    QgsPointXY diff( new_center.x() - center.x(), new_center.y() - center.y() );
    QgsRectangle new_extent( ext.xMinimum() + diff.x(), ext.yMinimum() + diff.y(),
                             ext.xMaximum() + diff.x(), ext.yMaximum() + diff.y() );
    useCanvas->setExtent( new_extent );
    useCanvas->refresh();
  }
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
      if ( ( i_srid > 0 ) && ( i_srid != getSrid() ) )
      {
        s_EWKT = QString( "ST_Transform(%1,%2)" ).arg( QString( s_EWKT ).arg( i_srid ).arg( mMapCoordsReverse.wellKnownText() ) ).arg( getSrid() );
      }
      else
      {
        s_EWKT = QString( s_EWKT ).arg( getSrid() ).arg( mMapCoordsReverse.wellKnownText() );
      }
    }
    break;
    case 1:
    default:
    {
      if ( ( i_srid > 0 ) && ( i_srid != getSrid() ) )
      {
        s_EWKT = QString( "ST_Transform(%1,%2)" ).arg( QString( s_EWKT ).arg( i_srid ).arg( mMapCoords.wellKnownText() ) ).arg( getSrid() );
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
QString QgsGeorefDataPoint::sqlInsertPointsCoverage( int i_srid )
{
  QString s_sql_insert = "";
  QStringList sa_fields = getTextInfo().split( mParseString ); // 10
  if ( sa_fields.count() == 7 )
  {
    // SELECT id_gcp, id_gcp_coverage, name, notes, pixel_x, pixel_y, srid, point_x, point_y, order_selected, gcp_enable, gcp_text, gcp_pixel, gcp_point
    QString sPointsTableName = sa_fields.at( 2 );
    switch ( getResultType() )
    {
      case 3: // INSERT
        s_sql_insert  = QString( "INSERT INTO \"%1\"\n(id_gcp_coverage,name,notes,srid,gcp_enable,order_selected,gcp_text,gcp_pixel,gcp_point)\n " ).arg( sPointsTableName );
        s_sql_insert += QString( "SELECT %1 AS id_gcp_coverage," ).arg( sa_fields.at( 1 ) );
        s_sql_insert += QString( "'%1' AS name," ).arg( sa_fields.at( 3 ) );
        s_sql_insert += QString( "'%1' AS notes," ).arg( sa_fields.at( 4 ) );
        s_sql_insert += QString( "%1 AS srid," ).arg( getSrid() );
        s_sql_insert += QString( "%1 AS gcp_enable," ).arg( ( int )isEnabled() );
        s_sql_insert += QString( "%1 AS order_selected," ).arg( sa_fields.at( 5 ) );
        s_sql_insert += QString( "'%1' AS gcp_text," ).arg( sa_fields.at( 6 ) );
        s_sql_insert += QString( "%1 AS gcp_pixel," ).arg( GeomFromEWKT( 0 ) );
        s_sql_insert += QString( "%1 AS gcp_point" ).arg( GeomFromEWKT( 1, i_srid ) );
        break;
      case 2: // UPDATE
        s_sql_insert  = QString( "UPDATE \"%1\" SET gcp_point=%2,name='%3', notes='%4', gcp_text='%5' WHERE (id_gcp=%6);" ).arg( sPointsTableName ).arg( GeomFromEWKT( 1, i_srid ) ).arg( sa_fields.at( 3 ) ).arg( sa_fields.at( 4 ) ).arg( sa_fields.at( 6 ) ).arg( sa_fields.at( 0 ) );
        break;
    }
  }
  return s_sql_insert;
}
