/***************************************************************************
    qgsgeometryholecheck.cpp
    ---------------------
    begin                : September 2015
    copyright            : (C) 2014 by Sandro Mani / Sourcepole AG
    email                : smani at sourcepole dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsgeometryholecheck.h"
#include "qgscurve.h"
#include "qgscurvepolygon.h"
#include "qgsfeaturepool.h"

void QgsGeometryHoleCheck::collectErrors( QList<QgsGeometryCheckError *> &errors, QStringList &/*messages*/, QAtomicInt *progressCounter, const QMap<QString, QgsFeatureIds> &ids ) const
{
  QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds() : ids;
  QgsGeometryCheckerUtils::LayerFeatures layerFeatures( mContext->featurePools, featureIds, mCompatibleGeometryTypes, progressCounter );
  for ( const QgsGeometryCheckerUtils::LayerFeature &layerFeature : layerFeatures )
  {
    const QgsAbstractGeometry *geom = layerFeature.geometry();
    for ( int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart )
    {
      const QgsCurvePolygon *poly = dynamic_cast<const QgsCurvePolygon *>( QgsGeometryCheckerUtils::getGeomPart( geom, iPart ) );
      if ( !poly )
      {
        continue;
      }
      // Rings after the first one are interiors
      for ( int iRing = 1, nRings = poly->ringCount( iPart ); iRing < nRings; ++iRing )
      {

        QgsPoint pos = poly->interiorRing( iRing - 1 )->centroid();
        errors.append( new QgsGeometryCheckError( this, layerFeature, pos, QgsVertexId( iPart, iRing ) ) );
      }
    }
  }
}

void QgsGeometryHoleCheck::fixError( QgsGeometryCheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes &changes ) const
{
  QgsFeaturePool *featurePool = mContext->featurePools[ error->layerId() ];
  QgsFeature feature;
  if ( !featurePool->get( error->featureId(), feature ) )
  {
    error->setObsolete();
    return;
  }
  QgsGeometry featureGeom = feature.geometry();
  const QgsAbstractGeometry *geom = featureGeom.constGet();
  QgsVertexId vidx = error->vidx();

  // Check if ring still exists
  if ( !vidx.isValid( geom ) )
  {
    error->setObsolete();
    return;
  }

  // Fix error
  if ( method == NoChange )
  {
    error->setFixed( method );
  }
  else if ( method == RemoveHoles )
  {
    deleteFeatureGeometryRing( error->layerId(), feature, vidx.part, vidx.ring, changes );
    error->setFixed( method );
  }
  else
  {
    error->setFixFailed( tr( "Unknown method" ) );
  }
}

QStringList QgsGeometryHoleCheck::getResolutionMethods() const
{
  static QStringList methods = QStringList() << tr( "Remove hole" ) << tr( "No action" );
  return methods;
}
