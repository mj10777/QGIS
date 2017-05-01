/***************************************************************************
    qgsgcplistmodel.cpp - Model implementation of GCPList Model/View
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

#include "qgsgcplist.h"
#include "qgsgcplistmodel.h"
#include "qgis.h"
#include "qgsgeorefdatapoint.h"
#include "qgsgeoreftransform.h"

#include <QSettings>
#include <cmath>

class QgsStandardItem : public QStandardItem
{
  public:
    explicit QgsStandardItem( const QString &text ) : QStandardItem( text )
    {
      // In addition to the DisplayRole, also set the user role, which is used for sorting.
      // This is needed for numerical sorting to work correctly (otherwise sorting is lexicographic).
      setData( QVariant( text ), Qt::UserRole );
      setData( QVariant( text ), Qt::EditRole );
      setData( QVariant( text ), Qt::DisplayRole );
      setData( QVariant( text ), Qt::ToolTipRole );
      setTextAlignment( Qt::AlignRight );
    }

    explicit QgsStandardItem( int value ) : QStandardItem( QString::number( value ) )
    {
      setData( QVariant( value ), Qt::UserRole );
      setTextAlignment( Qt::AlignCenter );
    }

    explicit QgsStandardItem( double value ) : QStandardItem( QString::number( value, 'f', 10 ) )
    {
      setData( QVariant( value ), Qt::UserRole );
      //show the full precision when editing points
      setData( QVariant( value ), Qt::EditRole );
      setData( QVariant( value ), Qt::DisplayRole );
      setData( QVariant( value ), Qt::ToolTipRole );
      setTextAlignment( Qt::AlignRight );
    }
};

QgsGCPListModel::QgsGCPListModel( QObject *parent, int iLegacyMode )
  : QStandardItemModel( parent )
  , mGCPList( new QgsGCPList() )
  , mLegacyMode( iLegacyMode )
{
  // Use data provided by Qt::UserRole as sorting key (needed for numerical sorting).
  setSortRole( Qt::UserRole );
}

void QgsGCPListModel::setLegacyMode( int iLegacyMode )
{
  mLegacyMode = iLegacyMode;
}

void QgsGCPListModel::setGCPList( QgsGCPList *theGCPList )
{
  mGCPList = theGCPList;
  updateModel();
}
// ------------------------------- public ---------------------------------- //
void QgsGCPListModel::setGeorefTransform( QgsGeorefTransform *theGeorefTransform )
{
  mGeorefTransform = theGeorefTransform;
  updateModel();
}

void QgsGCPListModel::updateModel()
{
  //clear();
  if ( !mGCPList )
    return;
  bMapUnitsPossible = false;
  bTransformUpdated = false;

  QVector<QgsPoint> mapCoords, pixelCoords;
  mGCPList->createGCPVectors( mapCoords, pixelCoords );

  //  // Setup table header
  QStringList itemLabels;
  QString unitType;
  QSettings s;

  if ( mGeorefTransform )
  {
    bTransformUpdated = mGeorefTransform->updateParametersFromGCPs( mapCoords, pixelCoords );
    bMapUnitsPossible = mGeorefTransform->providesAccurateInverseTransformation();
  }

  if ( s.value( "/Plugin-GeoReferencer/Config/ResidualUnits" ) == "mapUnits" && bMapUnitsPossible )
  {
    unitType = tr( "map units" );
  }
  else
  {
    unitType = tr( "pixels" );
  }

  itemLabels << tr( "Visible" )
             << tr( "ID" )
             << tr( "Source X" )
             << tr( "Source Y" )
             << tr( "Dest. X" )
             << tr( "Dest. Y" )
             << tr( "dX (%1)" ).arg( unitType )
             << tr( "dY (%1)" ).arg( unitType )
             << tr( "Residual (%1)" ).arg( unitType )
             << tr( "distance (%1)" ).arg( "pixels" )
             << tr( "azimuth (%1)" ).arg( "pixels" )
             << tr( "WKT (%1)" ).arg( "pixels" )
             << tr( "distance (%1)" ).arg( "map" )
             << tr( "azimuth (%1)" ).arg( "map" )
             << tr( "WKT (%1)" ).arg( "map" );

  setHorizontalHeaderLabels( itemLabels );
  setRowCount( mGCPList->size() );

  for ( int i = 0; i < mGCPList->countDataPoints(); ++i )
  {
    int j = 0;
    QgsGeorefDataPoint *dataPoint = mGCPList->at( i );

    if ( !dataPoint )
      continue;
    switch ( mLegacyMode )
    {
      case 0:
        dataPoint->setId( i );
        break;
    }

    QStandardItem *si = new QStandardItem();
    si->setTextAlignment( Qt::AlignCenter );
    si->setCheckable( true );
    if ( dataPoint->isEnabled() )
      si->setCheckState( Qt::Checked );
    else
      si->setCheckState( Qt::Unchecked );

    setItem( i, j++, si );
    setItem( i, j++, new QgsStandardItem( dataPoint-> id() ) );
    setItem( i, j++, new QgsStandardItem( QString::number( dataPoint->pixelCoords().x(), 'f', 10 ) ) );
    setItem( i, j++, new QgsStandardItem( QString::number( dataPoint->pixelCoords().y(), 'f', 10 ) ) );
    setItem( i, j++, new QgsStandardItem( QString::number( dataPoint->mapCoords().x(), 'f', 10 ) ) );
    setItem( i, j++, new QgsStandardItem( QString::number( dataPoint->mapCoords().y(), 'f', 10 ) ) );

    double residual;
    double dX = 0;
    double dY = 0;
    // Calculate residual if transform is available and up-to-date
    if ( mGeorefTransform && bTransformUpdated && mGeorefTransform->parametersInitialized() && !mGCPList->avoidUnneededUpdates() )
    {
      QgsPoint reverse_point_pixel;
      QgsPoint reverse_point_map;
      QgsPoint pixelPoint = mGeorefTransform->hasCrs() ? mGeorefTransform->toColumnLine( dataPoint->pixelCoords() ) : dataPoint->pixelCoords();
      if ( unitType == tr( "pixels" ) )
      {
        // Transform from world to raster coordinate:
        // This is the transform direction used by the warp operation.
        // As transforms of order >=2 are not invertible, we are only
        // interested in the residual in this direction
        // correct   : 23258.22999986090144375,20888.28000062370119849,4036.55863499999986743,-1720.50918799999999464,1
        // incorrect: 23258.22999986090144375,20888.28000062370119849,4039.69056077662253301,-2549.30078565706389782,1
        if ( mGeorefTransform->transformWorldToRaster( dataPoint->mapCoords(), reverse_point_pixel ) )
        {
          dX = ( reverse_point_pixel.x() - pixelPoint.x() );
          dY = -( reverse_point_pixel.y() - pixelPoint.y() );
          dataPoint->setPixelCoordsReverse( reverse_point_pixel );
          if ( mGeorefTransform->transformRasterToWorld( dataPoint->pixelCoords(), reverse_point_map ) )
          {
            dataPoint->setMapCoordsReverse( reverse_point_map );
          }
        }
      }
      else if ( unitType == tr( "map units" ) )
      {
        if ( mGeorefTransform->transformRasterToWorld( pixelPoint, reverse_point_map ) )
        {
          dX = ( reverse_point_map.x() - dataPoint->mapCoords().x() );
          dY = ( reverse_point_map.y() - dataPoint->mapCoords().y() );
          dataPoint->setMapCoordsReverse( reverse_point_map );
          if ( mGeorefTransform->transformWorldToRaster( dataPoint->mapCoords(), reverse_point_pixel ) )
          {
            dataPoint->setPixelCoordsReverse( reverse_point_pixel );
          }
        }
      }
    }
    residual = sqrt( dX * dX + dY * dY );

    dataPoint->setResidual( QPointF( dX, dY ) );

    if ( residual >= 0.f )
    {
      setItem( i, j++, new QgsStandardItem( QString::number( dX, 'f', 10 ) ) );
      setItem( i, j++, new QgsStandardItem( QString::number( dY, 'f', 10 ) ) );
      setItem( i, j++, new QgsStandardItem( QString::number( residual, 'f', 10 ) ) );
      setItem( i, j++, new QgsStandardItem( QString::number( dataPoint->pixelDistanceReverse(), 'f', 4 ) ) );
      setItem( i, j++, new QgsStandardItem( QString::number( dataPoint->pixelAzimuthReverse(), 'f', 4 ) ) );
      setItem( i, j++, new QgsStandardItem( dataPoint->pixelCoordsReverse().wellKnownText() ) );
      setItem( i, j++, new QgsStandardItem( QString::number( dataPoint->mapDistanceReverse(), 'f', 4 ) ) );
      setItem( i, j++, new QgsStandardItem( QString::number( dataPoint->mapAzimuthReverse(), 'f', 4 ) ) );
      setItem( i, j++, new QgsStandardItem( dataPoint->mapCoordsReverse().wellKnownText() ) );
    }
    else
    {
      setItem( i, j++, new QgsStandardItem( "n/a" ) );
      setItem( i, j++, new QgsStandardItem( "n/a" ) );
      setItem( i, j++, new QgsStandardItem( "n/a" ) );
      setItem( i, j++, new QgsStandardItem( "n/a" ) );
      setItem( i, j++, new QgsStandardItem( "n/a" ) );
      setItem( i, j++, new QgsStandardItem( "n/a" ) );
      setItem( i, j++, new QgsStandardItem( "n/a" ) );
      setItem( i, j++, new QgsStandardItem( "n/a" ) );
      setItem( i, j++, new QgsStandardItem( "n/a" ) );
    }
  }
  if ( bTransformUpdated )
  {
    setClean();
  }
}
bool QgsGCPListModel::calculateMeanError( double &error ) const
{
  if ( mGeorefTransform->transformParametrisation() == QgsGeorefTransform::InvalidTransform )
  {
    return false;
  }

  int nPointsEnabled = mGCPList->countDataPointsEnabled();
  if ( nPointsEnabled == mGeorefTransform->getMinimumGCPCount() )
  {
    error = 0;
    return true;
  }
  else if ( nPointsEnabled < mGeorefTransform->getMinimumGCPCount() )
  {
    return false;
  }

  double sumVxSquare = 0;
  double sumVySquare = 0;

  for ( int i = 0; i < mGCPList->countDataPoints(); ++i )
  {
    QgsGeorefDataPoint *dataPoint = mGCPList->at( i );
    if ( dataPoint->isEnabled() )
    {
      sumVxSquare += ( dataPoint->residual().x() * dataPoint->residual().x() );
      sumVySquare += ( dataPoint->residual().y() * dataPoint->residual().y() );
    }
  }

  // Calculate the root mean square error, adjusted for degrees of freedom of the transform
  // Caveat: The number of DoFs is assumed to be even (as each control point fixes two degrees of freedom).
  error = sqrt( ( sumVxSquare + sumVySquare ) / ( nPointsEnabled - mGeorefTransform->getMinimumGCPCount() ) );
  return true;
}

// --------------------------- public slots -------------------------------- //
void QgsGCPListModel::replaceDataPoint( QgsGeorefDataPoint *newDataPoint, int i )
{
  mGCPList->replace( i, newDataPoint );
}

void QgsGCPListModel::onGCPListModified()
{
}

void QgsGCPListModel::onTransformationModified()
{
}
