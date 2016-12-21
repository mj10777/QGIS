/***************************************************************************
     qgsgeorefconfigdialog.h
     --------------------------------------
    Date                 : 14-Feb-2010
    Copyright            : (C) 2010 by Jack R, Maxim Dubinin (GIS-Lab)
    Email                : sim@gis-lab.info
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsgcplist.h"

QgsGCPList::QgsGCPList()
    : QList<QgsGeorefDataPoint *>()
    , mIsDirty( true )
    , mHasChanged( false )
    , mGcpDbData( nullptr )
{
}

QgsGCPList::QgsGCPList( const QgsGCPList &list )
    : QList<QgsGeorefDataPoint *>()
    , mIsDirty( true )
    , mHasChanged( false )
    , mGcpDbData( nullptr )
{
  clear();
  QgsGCPList::const_iterator it = list.constBegin();
  for ( ; it != list.constEnd(); ++it )
  {
    QgsGeorefDataPoint *data_point = new QgsGeorefDataPoint( **it );
    append( data_point );
  }
}

void QgsGCPList::createGCPVectors( QVector<QgsPoint> &mapCoords, QVector<QgsPoint> &pixelCoords )
{
  mapCoords   = QVector<QgsPoint>( size() );
  pixelCoords = QVector<QgsPoint>( size() );
  for ( int i = 0, j = 0; i < sizeAll(); i++ )
  {
    QgsGeorefDataPoint *data_point = at( i );
    if ( data_point->isEnabled() )
    {
      mapCoords[j] = data_point->mapCoords();
      pixelCoords[j] = data_point->pixelCoords();
      j++;
    }
  }
}
QgsGeorefDataPoint* QgsGCPList::getDataPoint( int id )
{
  for ( int i = 0; i < sizeAll(); i++ )
  {
    QgsGeorefDataPoint *data_point = at( i );
    if ( data_point->id() == id )
    {
      return data_point;
    }
  }
  return nullptr;
}
bool QgsGCPList::addDataPoint( QgsGeorefDataPoint* data_point )
{
  append(data_point);
  return setDirty();
}
bool QgsGCPList::removeDataPoint( int id )
{
  int i_position_at = -1;
  bool b_hit = false;
  for ( int i = 0; i < sizeAll(); i++ )
  {
    QgsGeorefDataPoint *data_point = at( i );
    if ( data_point->id() == id )
    {
      i_position_at = i;
      break;
    }
  }
  if ( i_position_at >= 0 )
  {
    removeAt( i_position_at );
    b_hit = true;
    setDirty();
  }
  return b_hit;
}
bool QgsGCPList::updateDataPoint( int id, bool b_point_map, QgsPoint update_point )
{ // i_point_type0=pixel ; 1 = map
  bool b_hit = false;
  QgsGeorefDataPoint *update_data_point = getDataPoint( id );
  if ( update_data_point )
  {
    if ( b_point_map )
    {
      update_data_point->setMapCoords( update_point );
    }
    else
    {
      update_data_point->setPixelCoords( update_point );
    }
    b_hit = true;
    setDirty();
  }
  return b_hit;
}

int QgsGCPList::size() const
{
  if ( QList<QgsGeorefDataPoint *>::isEmpty() )
    return 0;

  int s = 0;
  const_iterator it = begin();
  while ( it != end() )
  {
    if (( *it )->isEnabled() )
      s++;
    ++it;
  }
  return s;
}

int QgsGCPList::sizeAll() const
{
  return QList<QgsGeorefDataPoint *>::size();
}

QgsGCPList &QgsGCPList::operator =( const QgsGCPList & list )
{
  clear();
  QgsGCPList::const_iterator it = list.constBegin();
  for ( ; it != list.constEnd(); ++it )
  {
    QgsGeorefDataPoint *data_point = new QgsGeorefDataPoint( **it );
    append( data_point );
  }
  return *this;
}
