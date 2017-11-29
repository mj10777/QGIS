/***************************************************************************
    qgsgeometryareacheck.cpp
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

#include "qgsgeometryengine.h"
#include "qgsgeometrycollection.h"
#include "qgsgeometryareacheck.h"
#include "qgsfeaturepool.h"

void QgsGeometryAreaCheck::collectErrors( QList<QgsGeometryCheckError *> &errors, QStringList &/*messages*/, QAtomicInt *progressCounter, const QMap<QString, QgsFeatureIds> &ids ) const
{
  QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds() : ids;
  QgsGeometryCheckerUtils::LayerFeatures layerFeatures( mContext->featurePools, featureIds, mCompatibleGeometryTypes, progressCounter );
  for ( const QgsGeometryCheckerUtils::LayerFeature &layerFeature : layerFeatures )
  {
    double layerToMapUnits = layerFeature.layerToMapUnits();
    const QgsAbstractGeometry *geom = layerFeature.geometry();
    for ( int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart )
    {
      double value;
      const QgsAbstractGeometry *part = QgsGeometryCheckerUtils::getGeomPart( geom, iPart );
      if ( checkThreshold( layerToMapUnits, part, value ) )
      {
        errors.append( new QgsGeometryCheckError( this, layerFeature, part->centroid(), QgsVertexId( iPart ), value * layerToMapUnits * layerToMapUnits, QgsGeometryCheckError::ValueArea ) );
      }
    }
  }
}

void QgsGeometryAreaCheck::fixError( QgsGeometryCheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes ) const
{
  QgsFeaturePool *featurePool = mContext->featurePools[ error->layerId() ];
  QgsFeature feature;
  if ( !featurePool->get( error->featureId(), feature ) )
  {
    error->setObsolete();
    return;
  }
  double layerToMapUnits = featurePool->getLayerToMapUnits();
  QgsGeometry g = feature.geometry();
  const QgsAbstractGeometry *geom = g.constGet();
  QgsVertexId vidx = error->vidx();

  // Check if polygon still exists
  if ( !vidx.isValid( geom ) )
  {
    error->setObsolete();
    return;
  }

  // Check if error still applies
  double value;
  if ( !checkThreshold( layerToMapUnits, QgsGeometryCheckerUtils::getGeomPart( geom, vidx.part ), value ) )
  {
    error->setObsolete();
    return;
  }

  // Fix with selected method
  if ( method == NoChange )
  {
    error->setFixed( method );
  }
  else if ( method == Delete )
  {
    deleteFeatureGeometryPart( error->layerId(), feature, vidx.part, changes );
    error->setFixed( method );
  }
  else if ( method == MergeLongestEdge || method == MergeLargestArea || method == MergeIdenticalAttribute )
  {
    QString errMsg;
    if ( mergeWithNeighbor( error->layerId(), feature,  vidx.part, method, mergeAttributeIndices[error->layerId()], changes, errMsg ) )
    {
      error->setFixed( method );
    }
    else
    {
      error->setFixFailed( tr( "Failed to merge with neighbor: %1" ).arg( errMsg ) );
    }
  }
  else
  {
    error->setFixFailed( tr( "Unknown method" ) );
  }
}

bool QgsGeometryAreaCheck::checkThreshold( double layerToMapUnits, const QgsAbstractGeometry *geom, double &value ) const
{
  value = geom->area();
  double threshold = mThresholdMapUnits / ( layerToMapUnits * layerToMapUnits );
  return value < threshold;
}

bool QgsGeometryAreaCheck::mergeWithNeighbor( const QString &layerId, QgsFeature &feature, int partIdx, int method, int mergeAttributeIndex, Changes &changes, QString &errMsg ) const
{
  QgsFeaturePool *featurePool = mContext->featurePools[ layerId ];

  double maxVal = 0.;
  QgsFeature mergeFeature;
  int mergePartIdx = -1;
  bool matchFound = false;
  QgsGeometry featureGeometry = feature.geometry();
  const QgsAbstractGeometry *geom = featureGeometry.constGet();

  // Search for touching neighboring geometries
  for ( QgsFeatureId testId : featurePool->getIntersects( featureGeometry.boundingBox() ) )
  {
    QgsFeature testFeature;
    if ( !featurePool->get( testId, testFeature ) )
    {
      continue;
    }
    QgsGeometry testFeatureGeom = testFeature.geometry();
    const QgsAbstractGeometry *testGeom = testFeatureGeom.constGet();
    for ( int testPartIdx = 0, nTestParts = testGeom->partCount(); testPartIdx < nTestParts; ++testPartIdx )
    {
      if ( testId == feature.id() && testPartIdx == partIdx )
      {
        continue;
      }
      double len = QgsGeometryCheckerUtils::sharedEdgeLength( QgsGeometryCheckerUtils::getGeomPart( geom, partIdx ), QgsGeometryCheckerUtils::getGeomPart( testGeom, testPartIdx ), mContext->reducedTolerance );
      if ( len > 0. )
      {
        if ( method == MergeLongestEdge || method == MergeLargestArea )
        {
          double val;
          if ( method == MergeLongestEdge )
          {
            val = len;
          }
          else
          {
            if ( dynamic_cast<const QgsGeometryCollection *>( testGeom ) )
              val = static_cast<const QgsGeometryCollection *>( testGeom )->geometryN( testPartIdx )->area();
            else
              val = testGeom->area();
          }
          if ( val > maxVal )
          {
            maxVal = val;
            mergeFeature = testFeature;
            mergePartIdx = testPartIdx;
          }
        }
        else if ( method == MergeIdenticalAttribute )
        {
          if ( testFeature.attribute( mergeAttributeIndex ) == feature.attribute( mergeAttributeIndex ) )
          {
            mergeFeature = testFeature;
            mergePartIdx = testPartIdx;
            matchFound = true;
            break;
          }
        }
      }
    }
    if ( matchFound )
    {
      break;
    }
  }

  if ( !matchFound && maxVal == 0. )
  {
    return method == MergeIdenticalAttribute;
  }

  // Merge geometries
  QgsGeometry mergeFeatureGeom = mergeFeature.geometry();
  const QgsAbstractGeometry *mergeGeom = mergeFeatureGeom.constGet();
  std::unique_ptr<QgsGeometryEngine> geomEngine( QgsGeometryCheckerUtils::createGeomEngine( QgsGeometryCheckerUtils::getGeomPart( mergeGeom, mergePartIdx ), mContext->reducedTolerance ) );
  QgsAbstractGeometry *combinedGeom = geomEngine->combine( QgsGeometryCheckerUtils::getGeomPart( geom, partIdx ), &errMsg );
  if ( !combinedGeom || combinedGeom->isEmpty() || !QgsWkbTypes::isSingleType( combinedGeom->wkbType() ) )
  {
    return false;
  }

  // Replace polygon in merge geometry
  if ( mergeFeature.id() == feature.id() && mergePartIdx > partIdx )
  {
    --mergePartIdx;
  }
  replaceFeatureGeometryPart( layerId, mergeFeature, mergePartIdx, combinedGeom, changes );
  // Remove polygon from source geometry
  deleteFeatureGeometryPart( layerId, feature, partIdx, changes );

  return true;
}

QStringList QgsGeometryAreaCheck::getResolutionMethods() const
{
  static QStringList methods = QStringList()
                               << tr( "Merge with neighboring polygon with longest shared edge" )
                               << tr( "Merge with neighboring polygon with largest area" )
                               << tr( "Merge with neighboring polygon with identical attribute value, if any, or leave as is" )
                               << tr( "Delete feature" )
                               << tr( "No action" );
  return methods;
}
