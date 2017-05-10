/***************************************************************************
    qgsgcplistmodel.cpp - Model implementation of GcpList Model/View
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
#include "qgssettings.h"

#include <cmath>

class QgsStandardItem : public QStandardItem
{
  public:
    explicit QgsStandardItem( const QString &text ) : QStandardItem( text )
    {
      // In addition to the DisplayRole, also set the user role, which is used for sorting.
      // This is needed for numerical sorting to work correctly (otherwise sorting is lexicographic).
      setData( QVariant( text ), Qt::UserRole );
      // setData( QVariant( text ), Qt::EditRole );
      setData( QVariant( text ), Qt::DisplayRole );
      setData( QVariant( text ), Qt::ToolTipRole );
      setTextAlignment( Qt::AlignLeft );
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
      // setData( QVariant( value ), Qt::EditRole );
      setData( QVariant( value ), Qt::DisplayRole );
      setData( QVariant( value ), Qt::ToolTipRole );
      setTextAlignment( Qt::AlignRight );
    }
};

QgsGcpListModel::QgsGcpListModel( QObject *parent )
  : QStandardItemModel( parent )
  , mGcpList( new QgsGcpList() )
{
  // Use data provided by Qt::UserRole as sorting key (needed for numerical sorting).
  setSortRole( Qt::UserRole );
}

void QgsGcpListModel::setGcpList( QgsGcpList *gcpList )
{
  mGcpList = gcpList;
  updateModel();
}
// ------------------------------- public ---------------------------------- //
void QgsGcpListModel::setGeorefTransform( QgsGeorefTransform *georefTransform )
{
  mGeorefTransform = georefTransform;
  updateModel();
}

void QgsGcpListModel::updateModel()
{
  //clear();
  if ( !mGcpList )
    return;
  bMapUnitsPossible = false;
  bTransformUpdated = false;

<<<<<<< HEAD
  QVector<QgsPointXY> mapCoords, pixelCoords;
  mGCPList->createGCPVectors( mapCoords, pixelCoords );
=======
  QVector<QgsPoint> mapCoords, pixelCoords;
  mGcpList->createGcpVectors( mapCoords, pixelCoords );
>>>>>>> Last major problems resolved. Start test phase

  //  // Setup table header
  QStringList itemLabels;
  QString unitType;
  QgsSettings s;

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
             << tr( "id_gcp" )
             << tr( "dX (%1)" ).arg( unitType )
             << tr( "dY (%1)" ).arg( unitType )
             << tr( "Residual (%1)" ).arg( unitType )
             << tr( "azimuth (%1)" ).arg( "pixels" )
             << tr( "azimuth (%1)" ).arg( "map" )
             << tr( "Notes" )
             << tr( "WKT (%1)" ).arg( "pixels" )
             << tr( "WKT (%1)" ).arg( "map" );

  setHorizontalHeaderLabels( itemLabels );
  setRowCount( mGcpList->size() );

  for ( int i = 0; i < mGcpList->countDataPoints(); ++i )
  {
    int j = 0;
    QgsGeorefDataPoint *dataPoint = mGcpList->at( i );

    if ( !dataPoint )
      continue;

    QStandardItem *si = new QStandardItem();
    si->setTextAlignment( Qt::AlignCenter );
    si->setCheckable( true );
    if ( dataPoint->isEnabled() )
      si->setCheckState( Qt::Checked );
    else
      si->setCheckState( Qt::Unchecked );

    setItem( i, j++, si );
    setItem( i, j++, new QgsStandardItem( dataPoint-> id() ) );
    QString sNotes = dataPoint->name();

    double residual;
    double dX = 0;
    double dY = 0;
    // Calculate residual if transform is available and up-to-date
    if ( mGeorefTransform && bTransformUpdated && mGeorefTransform->parametersInitialized() && !mGcpList->avoidUnneededUpdates() )
    {
      QgsPointXY reverse_point_pixel;
      QgsPointXY reverse_point_map;
      QgsPointXY pixelPoint = mGeorefTransform->hasCrs() ? mGeorefTransform->toColumnLine( dataPoint->pixelCoords() ) : dataPoint->pixelCoords();
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
      setItem( i, j++, new QgsStandardItem( QString::number( dataPoint->pixelAzimuthReverse(), 'f', 4 ) ) );
      setItem( i, j++, new QgsStandardItem( QString::number( dataPoint->mapAzimuthReverse(), 'f', 4 ) ) );
    }
    else
    {
      setItem( i, j++, new QgsStandardItem( "n/a" ) );
      setItem( i, j++, new QgsStandardItem( "n/a" ) );
      setItem( i, j++, new QgsStandardItem( "n/a" ) );
      setItem( i, j++, new QgsStandardItem( "n/a" ) );
      setItem( i, j++, new QgsStandardItem( "n/a" ) );
    }
    if ( !dataPoint->notes().isEmpty() )
    {
      sNotes = QString( "%1, %2" ).arg( sNotes ).arg( dataPoint->notes() );
    }
    setItem( i, j++, new QgsStandardItem( sNotes ) );
    setItem( i, j++, new QgsStandardItem( dataPoint->pixelCoordsReverse().wellKnownText() ) );
    setItem( i, j++, new QgsStandardItem( dataPoint->mapCoordsReverse().wellKnownText() ) );
  }
  if ( bTransformUpdated )
  {
    setClean();
  }
}
bool QgsGcpListModel::calculateMeanError( double &error ) const
{
  if ( mGeorefTransform->transformParametrisation() == QgsGeorefTransform::InvalidTransform )
  {
    return false;
  }

  int nPointsEnabled = mGcpList->countDataPointsEnabled();
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

  for ( int i = 0; i < mGcpList->countDataPoints(); ++i )
  {
    QgsGeorefDataPoint *dataPoint = mGcpList->at( i );
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
void QgsGcpListModel::replaceDataPoint( QgsGeorefDataPoint *newDataPoint, int i )
{
  mGcpList->replace( i, newDataPoint );
}

void QgsGcpListModel::onGcpListModified()
{
}

void QgsGcpListModel::onTransformationModified()
{
}
