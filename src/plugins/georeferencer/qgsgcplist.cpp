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
{
}

QgsGCPList::QgsGCPList( const QgsGCPList &list )
  : QList<QgsGeorefDataPoint *>()
  , mHasChanged( true )
{
  clear();
  QgsGCPList::const_iterator it = list.constBegin();
  for ( ; it != list.constEnd(); ++it )
  {
    QgsGeorefDataPoint *dataPoint = new QgsGeorefDataPoint( **it );
    append( dataPoint );
  }
}

void QgsGCPList::createGCPVectors( QVector<QgsPoint> &mapCoords, QVector<QgsPoint> &pixelCoords )
{
  mapCoords   = QVector<QgsPoint>( size() );
  pixelCoords = QVector<QgsPoint>( size() );
  for ( int i = 0, j = 0; i < sizeAll(); i++ )
  {
    QgsGeorefDataPoint *dataPoint = at( i );
    if ( dataPoint->isEnabled() )
    {
      mapCoords[j] = dataPoint->mapCoords();
      pixelCoords[j] = dataPoint->pixelCoords();
      j++;
    }
  }
}
QgsGeorefDataPoint *QgsGCPList::getDataPoint( int id_gcp )
{
  for ( int i = 0; i < sizeAll(); i++ )
  {
    QgsGeorefDataPoint *dataPoint = at( i );
    if ( dataPoint->id() == id_gcp )
    {
      return dataPoint;
    }
  }
  return nullptr;
}
bool QgsGCPList::addDataPoint( QgsGeorefDataPoint *dataPoint )
{
  append( dataPoint );
  return setDirty();
}
bool QgsGCPList::removeDataPoint( int id_gcp )
{
  int i_position_at = -1;
  bool b_hit = false;
  for ( int i = 0; i < sizeAll(); i++ )
  {
    QgsGeorefDataPoint *dataPoint = at( i );
    if ( dataPoint->id() == id_gcp )
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
bool QgsGCPList::updateDataPoint( int id_gcp, bool bPointMap, QgsPoint updatePoint )
{
  // iPointType: 0=pixel ; 1 = map
  bool b_hit = false;
  QgsGeorefDataPoint *update_dataPoint = getDataPoint( id_gcp );
  if ( update_dataPoint )
  {
    if ( bPointMap )
    {
      update_dataPoint->setMapCoords( updatePoint );
    }
    else
    {
      update_dataPoint->setPixelCoords( updatePoint );
    }
    b_hit = true;
    setDirty();
  }
  return b_hit;
}
bool QgsGCPList::searchDataPoint( QgsPoint searchPoint, bool bPointMap, double epsilon, int *id_gcp, double *distance, int *iResultType )
{
  // iPointType: 0=pixel ; 1 = map
  bool b_hit = false;
  int i_search_id_gcp = 0;
  double search_distance = DBL_MAX;
  if ( epsilon < 0.0 )
    epsilon = 0.0;
  int i_search_result_type = 0; // 0=not found ; 1=exact ; 2: inside area ; 3=outside area
  for ( int i = 0; i < sizeAll(); i++ )
  {
    QgsGeorefDataPoint *dataPoint = at( i );
    QgsPoint check_point;
    int i_check_id = dataPoint->id();
    int i_check_result_type = 0;
    if ( bPointMap )
    {
      check_point = dataPoint->mapCoords();
    }
    else
    {
      check_point = dataPoint->pixelCoords();
    }
    double check_distance = check_point.distance( searchPoint );
    if ( check_point == searchPoint )
    {
      i_check_result_type = 1;
      b_hit = true;
    }
    else
    {
      i_check_result_type = 3;
      if ( check_point.compare( searchPoint, epsilon ) )
      {
        i_check_result_type = 3;
      }
    }
    if ( check_distance < search_distance )
    {
      // The shortest distance will be returned
      if ( i_search_result_type != 1 )
      {
        // the first exact match will be returned
        search_distance = check_distance;
        i_search_result_type = i_check_result_type;
        i_search_id_gcp = i_check_id;
      }
    }
  }
  if ( id_gcp )
  {
    *id_gcp = i_search_id_gcp;
  }
  if ( distance )
  {
    *distance = search_distance;
  }
  if ( iResultType )
  {
    *iResultType = i_search_result_type;
  }
  return b_hit;
}

int QgsGCPList::size()
{
  if ( QList<QgsGeorefDataPoint *>::isEmpty() )
  {
    mCountEnabled = 0;
    return mCountEnabled;
  }
  if ( !mIsDirty )
  {
    //  no need to be recalulated since 'mIsDirty' is not true
    return mCountEnabled;
  }
  mCountEnabled = 0;
  const_iterator it = begin();
  while ( it != end() )
  {
    if ( ( *it )->isEnabled() )
      mCountEnabled++;
    ++it;
  }
  return mCountEnabled;
}

int QgsGCPList::sizeAll() const
{
  return QList<QgsGeorefDataPoint *>::size();
}

QgsGCPList &QgsGCPList::operator =( const QgsGCPList &list )
{
  clear();
  QgsGCPList::const_iterator it = list.constBegin();
  for ( ; it != list.constEnd(); ++it )
  {
    QgsGeorefDataPoint *dataPoint = new QgsGeorefDataPoint( **it );
    append( dataPoint );
  }
  setDirty();
  return *this;
}
