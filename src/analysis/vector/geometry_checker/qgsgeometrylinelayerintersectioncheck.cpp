/***************************************************************************
    qgsgeometrylinelayerintersectioncheck.cpp
    ---------------------
    begin                : September 2017
    copyright            : (C) 2017 by Sandro Mani / Sourcepole AG
    email                : smani at sourcepole dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsgeometrylinelayerintersectioncheck.h"
#include "qgspolygon.h"
#include "qgslinestring.h"
#include "qgsfeaturepool.h"

void QgsGeometryLineLayerIntersectionCheck::collectErrors( QList<QgsGeometryCheckError *> &errors, QStringList &/*messages*/, QAtomicInt *progressCounter, const QMap<QString, QgsFeatureIds> &ids ) const
{
  QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds() : ids;
  featureIds.remove( mCheckLayer ); // Don't check layer against itself
  QgsGeometryCheckerUtils::LayerFeatures layerFeatures( mContext->featurePools, featureIds, mCompatibleGeometryTypes, progressCounter, true );
  for ( const QgsGeometryCheckerUtils::LayerFeature &layerFeature : layerFeatures )
  {
    const QgsAbstractGeometry *geom = layerFeature.geometry();
    for ( int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart )
    {
      const QgsLineString *line = dynamic_cast<const QgsLineString *>( QgsGeometryCheckerUtils::getGeomPart( geom, iPart ) );
      if ( !line )
      {
        // Should not happen
        continue;
      }

      // Check whether the line intersects with any other features of the specified layer
      QgsGeometryCheckerUtils::LayerFeatures checkFeatures( mContext->featurePools, QStringList() << mCheckLayer, line->boundingBox(), {QgsWkbTypes::LineGeometry, QgsWkbTypes::PolygonGeometry} );
      for ( const QgsGeometryCheckerUtils::LayerFeature &checkFeature : checkFeatures )
      {
        const QgsAbstractGeometry *testGeom = checkFeature.geometry();
        for ( int jPart = 0, mParts = testGeom->partCount(); jPart < mParts; ++jPart )
        {
          const QgsAbstractGeometry *part = QgsGeometryCheckerUtils::getGeomPart( testGeom, jPart );
          if ( const QgsLineString *testLine = dynamic_cast<const QgsLineString *>( part ) )
          {
            for ( const QgsPoint &inter : QgsGeometryCheckerUtils::lineIntersections( line, testLine, mContext->tolerance ) )
            {
              errors.append( new QgsGeometryCheckError( this, layerFeature, inter, QgsVertexId( iPart ), checkFeature.id() ) );
            }
          }
          else if ( const QgsPolygon *polygon = dynamic_cast<const QgsPolygon *>( part ) )
          {
            for ( const QgsLineString *ring : QgsGeometryCheckerUtils::polygonRings( polygon ) )
            {
              for ( const QgsPoint &inter : QgsGeometryCheckerUtils::lineIntersections( line, ring, mContext->tolerance ) )
              {
                errors.append( new QgsGeometryCheckError( this, layerFeature, inter, QgsVertexId( iPart ), checkFeature.id() ) );
              }
            }
          }
        }
      }
    }
  }
}

void QgsGeometryLineLayerIntersectionCheck::fixError( QgsGeometryCheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes & /*changes*/ ) const
{
  if ( method == NoChange )
  {
    error->setFixed( method );
  }
  else
  {
    error->setFixFailed( tr( "Unknown method" ) );
  }
}

QStringList QgsGeometryLineLayerIntersectionCheck::getResolutionMethods() const
{
  static QStringList methods = QStringList() << tr( "No action" );
  return methods;
}
