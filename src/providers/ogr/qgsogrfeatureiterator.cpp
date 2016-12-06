/***************************************************************************
    qgsogrfeatureiterator.cpp
    ---------------------
    begin                : Juli 2012
    copyright            : (C) 2012 by Martin Dobias
    email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "qgsogrfeatureiterator.h"

#include "qgsogrprovider.h"
#include "qgsogrexpressioncompiler.h"
#include "qgssqliteexpressioncompiler.h"

#include "qgsogrutils.h"
#include "qgsapplication.h"
#include "qgsgeometry.h"
#include "qgslogger.h"
#include "qgsmessagelog.h"

#include <QTextCodec>
#include <QFile>

// using from provider:
// - setRelevantFields(), mRelevantFieldsForNextFeature
// - ogrLayer
// - mFetchFeaturesWithoutGeom
// - mAttributeFields
// - mEncoding


QgsOgrFeatureIterator::QgsOgrFeatureIterator( QgsOgrFeatureSource* source, bool ownSource, const QgsFeatureRequest& request )
    : QgsAbstractFeatureIteratorFromSource<QgsOgrFeatureSource>( source, ownSource, request )
    , mFeatureFetched( false )
    , mConn( nullptr )
    , ogrLayer( nullptr )
    , mSubsetStringSet( false )
    , mFetchGeometry( false )
    , mExpressionCompiled( false )
    , mFilterFids( mRequest.filterFids() )
    , mFilterFidsIt( mFilterFids.constBegin() )
{
  mConn = QgsOgrConnPool::instance()->acquireConnection( mSource->mProvider->dataSourceUri() );
  if ( !mConn->ds )
  {
    return;
  }

  if ( !mSource->mSubLayerString.isNull() )
  {
    ogrLayer = QgsOgrProviderUtils::OGRGetSubLayerStringWrapper( mConn->ds, mSource-> mSubLayerString );
  }
  if ( !ogrLayer )
  {
    return;
  }
  if ( !mSource->mSubsetString.isEmpty() )
  {
    ogrLayer = QgsOgrProviderUtils::setSubsetString( ogrLayer, mConn->ds, mSource->mEncoding, mSource->mSubsetString );
    if ( !ogrLayer )
    {
      return;
    }
    mSubsetStringSet = true;
  }
  mFetchGeometry = ( !mRequest.filterRect().isNull() ) || !( mRequest.flags() & QgsFeatureRequest::NoGeometry );
  QgsAttributeList attrs = ( mRequest.flags() & QgsFeatureRequest::SubsetOfAttributes ) ? mRequest.subsetOfAttributes() : mSource->mFields.allAttributesList();
  // ensure that all attributes required for expression filter are being fetched
  if ( mRequest.flags() & QgsFeatureRequest::SubsetOfAttributes && request.filterType() == QgsFeatureRequest::FilterExpression )
  {
    Q_FOREACH ( const QString& field, request.filterExpression()->referencedColumns() )
    {
      int attrIdx = mSource->mFields.fieldNameIndex( field );
      if ( !attrs.contains( attrIdx ) )
        attrs << attrIdx;
    }
    mRequest.setSubsetOfAttributes( attrs );
  }
  if ( request.filterType() == QgsFeatureRequest::FilterExpression && request.filterExpression()->needsGeometry() )
  {
    mFetchGeometry = true;
  }

  // make sure we fetch just relevant fields
  // unless it's a VRT data source filtered by geometry as we don't know which
  // attributes make up the geometry and OGR won't fetch them to evaluate the
  // filter if we choose to ignore them (fixes #11223)
  if (( mSource->mDriverName != "VRT" && mSource->mDriverName != "OGR_VRT" ) || mRequest.filterRect().isNull() )
  {
    QgsOgrProviderUtils::setRelevantFields( ogrLayer, mSource->mFields.count(), mFetchGeometry, attrs, mSource->mFirstFieldIsFid );
  }

  // spatial query to select features
  if ( !mRequest.filterRect().isNull() )
  {
    const QgsRectangle& rect = mRequest.filterRect();

    OGR_L_SetSpatialFilterRect( ogrLayer, rect.xMinimum(), rect.yMinimum(), rect.xMaximum(), rect.yMaximum() );
  }
  else
  {
    OGR_L_SetSpatialFilter( ogrLayer, nullptr );
  }

  if ( request.filterType() == QgsFeatureRequest::FilterExpression
       && QSettings().value( "/qgis/compileExpressions", true ).toBool() )
  {
    QgsSqlExpressionCompiler* compiler;
    if ( source->mDriverName == "SQLite" || source->mDriverName == "GPKG" )
    {
      compiler = new QgsSQLiteExpressionCompiler( source->mFields );
    }
    else
    {
      compiler = new QgsOgrExpressionCompiler( source );
    }

    QgsSqlExpressionCompiler::Result result = compiler->compile( request.filterExpression() );
    if ( result == QgsSqlExpressionCompiler::Complete || result == QgsSqlExpressionCompiler::Partial )
    {
      QString whereClause = compiler->result();
      if ( OGR_L_SetAttributeFilter( ogrLayer, mSource->mEncoding->fromUnicode( whereClause ).constData() ) == OGRERR_NONE )
      {
        //if only partial success when compiling expression, we need to double-check results using QGIS' expressions
        mExpressionCompiled = ( result == QgsSqlExpressionCompiler::Complete );
        mCompileStatus = ( mExpressionCompiled ? Compiled : PartiallyCompiled );
      }
    }
    else
    {
      OGR_L_SetAttributeFilter( ogrLayer, nullptr );
    }

    delete compiler;
  }
  else
  {
    OGR_L_SetAttributeFilter( ogrLayer, nullptr );
  }

  //start with first feature
  rewind();
}

QgsOgrFeatureIterator::~QgsOgrFeatureIterator()
{
  close();
}

bool QgsOgrFeatureIterator::nextFeatureFilterExpression( QgsFeature& f )
{
  if ( !mExpressionCompiled )
    return QgsAbstractFeatureIterator::nextFeatureFilterExpression( f );
  else
    return fetchFeature( f );
}

bool QgsOgrFeatureIterator::fetchFeatureWithId( QgsFeatureId id, QgsFeature& feature ) const
{
  feature.setValid( false );
  OGRFeatureH fet = OGR_L_GetFeature( ogrLayer, FID_TO_NUMBER( id ) );
  if ( !fet )
  {
    return false;
  }

  if ( readFeature( fet, feature ) )
    OGR_F_Destroy( fet );
  feature.setValid( true );
  // qDebug() << QString( "QgsOgrFeatureIterator::fetchFeatureWithId[%1,%2]  returning name[%3] - size=%4 value[%5] " ).arg( id ).arg( feature.isValid() ).arg(mSource->mLayerName).arg(feature.attributes().size()).arg( feature.attribute(0).toString() );
  return true;
}

bool QgsOgrFeatureIterator::fetchFeature( QgsFeature& feature )
{
  feature.setValid( false );

  if ( mClosed || !ogrLayer )
    return false;
  // qDebug() << QString( "QgsOgrFeatureIterator::fetchFeature[%1] FilterType[%2,%3] " ).arg( feature.id() ).arg(mRequest.filterType()).arg(mRequest.filterFid());
  if ( mRequest.filterType() == QgsFeatureRequest::FilterFid )
  {
    bool result = fetchFeatureWithId( mRequest.filterFid(), feature );
    close(); // the feature has been read or was not found: we have finished here
    // qDebug() << QString( "QgsOgrFeatureIterator::fetchFeature[%1,%2]  -1- name[%3] size=%4 value[%5] " ).arg( feature.id() ).arg( feature.isValid() ).arg(mSource->mLayerName).arg(feature.attributes().size()).arg( feature.attribute(0).toString() );
    return result;
  }
  else if ( mRequest.filterType() == QgsFeatureRequest::FilterFids )
  {
    while ( mFilterFidsIt != mFilterFids.constEnd() )
    {
      QgsFeatureId nextId = *mFilterFidsIt;
      mFilterFidsIt++;
      if ( fetchFeatureWithId( nextId, feature ) )
        return true;
    }
    close();
    return false;
  }

  OGRFeatureH fet;
  while (( fet = OGR_L_GetNextFeature( ogrLayer ) ) )
  {
    if ( !readFeature( fet, feature ) )
      continue;
    else
      OGR_F_Destroy( fet );

    if ( !mRequest.filterRect().isNull() && !feature.constGeometry() )
      continue;

    // we have a feature, end this cycle
    feature.setValid( true );
    // qDebug() << QString( "QgsOgrFeatureIterator::fetchFeature[%1,%2]  -3- name[%3] size=%4 value[%5] " ).arg( feature.id() ).arg( feature.isValid() ).arg(mSource->mLayerName).arg(feature.attributes().size()).arg( feature.attribute(0).toString() );
    return true;

  } // while

  close();
  return false;
}


bool QgsOgrFeatureIterator::rewind()
{
  if ( mClosed || !ogrLayer )
    return false;

  OGR_L_ResetReading( ogrLayer );

  mFilterFidsIt = mFilterFids.constBegin();

  return true;
}


bool QgsOgrFeatureIterator::close()
{
  if ( !mConn )
    return false;

  iteratorClosed();

  // Will for example release SQLite3 statements
  if ( ogrLayer )
  {
    OGR_L_ResetReading( ogrLayer );
  }

  if ( mSubsetStringSet )
  {
    OGR_DS_ReleaseResultSet( mConn->ds, ogrLayer );
  }

  if ( mConn )
    QgsOgrConnPool::instance()->releaseConnection( mConn );

  mConn = nullptr;
  ogrLayer = nullptr;

  mClosed = true;
  return true;
}


void QgsOgrFeatureIterator::getFeatureAttribute( OGRFeatureH ogrFet, QgsFeature & f, int attindex ) const
{
  if ( mSource->mFirstFieldIsFid && attindex == 0 )
  {
    f.setAttribute( 0, static_cast<qint64>( OGR_F_GetFID( ogrFet ) ) );
    // qDebug() << QString( "QgsOgrFeatureIterator::getFeatureAttribute[%1,%2] -1- mFirstFieldIsFid=%3 value[%4] " ).arg( attindex ).arg(mSource->mLayerName).arg(mSource->mFirstFieldIsFid).arg( f.attribute(0).toString() );
    return;
  }

  int attindexWithoutFid = ( mSource->mFirstFieldIsFid ) ? attindex - 1 : attindex;
  bool ok = false;
  QVariant value = QgsOgrUtils::getOgrFeatureAttribute( ogrFet, mSource->mFieldsWithoutFid, attindexWithoutFid, mSource->mEncoding, &ok );
  // qDebug() << QString( "QgsOgrFeatureIterator::getFeatureAttribute[%1,%2] -2- rc=%3 value[%4] " ).arg( attindex ).arg(mSource->mLayerName).arg(ok).arg( value.toString() );
  if ( !ok )
    return;

  f.setAttribute( attindex, value );
}

bool QgsOgrFeatureIterator::readFeature( OGRFeatureH fet, QgsFeature& feature ) const
{
  feature.setFeatureId( OGR_F_GetFID( fet ) );
  feature.initAttributes( mSource->mFields.count() );
  feature.setFields( mSource->mFields ); // allow name-based attribute lookups
  bool useIntersect = mRequest.flags() & QgsFeatureRequest::ExactIntersect;
  bool geometryTypeFilter = mSource->mOgrGeometryTypeFilter != wkbUnknown;
  if ( mFetchGeometry || useIntersect || geometryTypeFilter )
  {
    OGRGeometryH geom = OGR_F_GetGeometryRef( fet );

    if ( geom )
    {
      feature.setGeometry( QgsOgrUtils::ogrGeometryToQgsGeometry( geom ) );
      // qDebug() << QString( "QgsOgrFeatureIterator::readFeature[%1,%2] -1a- AttributeList.size=%3  " ).arg( feature.id() ).arg(mSource->mLayerName).arg(mRequest.subsetOfAttributes().size());
    }
    else
      feature.setGeometry( nullptr );

    if ( mSource->mOgrGeometryTypeFilter == wkbGeometryCollection &&
         geom && wkbFlatten( OGR_G_GetGeometryType( geom ) ) == wkbGeometryCollection )
    {
      // OK
      // qDebug() << QString( "QgsOgrFeatureIterator::readFeature[%1,%2] -1b- AttributeList.size=%3  " ).arg( feature.id() ).arg(mSource->mLayerName).arg(mRequest.subsetOfAttributes().size());
    }
    else if (( useIntersect && ( !feature.constGeometry() || !feature.constGeometry()->intersects( mRequest.filterRect() ) ) )
             || ( geometryTypeFilter && ( !feature.constGeometry() || QgsOgrProviderUtils::wkbSingleFlattenWrapper(( OGRwkbGeometryType )feature.constGeometry()->wkbType() ) != mSource->mOgrGeometryTypeFilter ) ) )
    {
      bool b_condition_00=false;
      bool b_condition_001=false;
      bool b_condition_002=false;
      bool b_condition_01=false;
      bool b_condition_02=false;
      bool b_condition_021=false;
      bool b_condition_022=false;
      if ( !mRequest.filterRect().isNull() )
       b_condition_001=true;
      if ( !( mRequest.flags() & QgsFeatureRequest::NoGeometry ) )
       b_condition_002=true;
      QString s_condition_error_00(QString("-W-> condition_00[%1] condition_001[%2] condition_002[%3]").arg(mFetchGeometry).arg(b_condition_001).arg(b_condition_002));
      // qDebug() << QString( "QgsOgrFeatureIterator::readFeature[%1,%2] -E-> -1ca- Geometry_conditions=[%3]" ).arg( feature.id() ).arg(mSource->mLayerName).arg(s_condition_error_00);
      if ( useIntersect && ( !feature.constGeometry() || !feature.constGeometry()->intersects( mRequest.filterRect() ) ) )
       b_condition_01=true;
      if ( geometryTypeFilter && ( !feature.constGeometry() || QgsOgrProviderUtils::wkbSingleFlattenWrapper(( OGRwkbGeometryType )feature.constGeometry()->wkbType() ) != mSource->mOgrGeometryTypeFilter ) )
       b_condition_02=true;
      if (b_condition_02)
      {
       if ( !feature.constGeometry())
        b_condition_021=true;
       if (!b_condition_021)
       {
         if ( QgsOgrProviderUtils::wkbSingleFlattenWrapper(( OGRwkbGeometryType )feature.constGeometry()->wkbType() ) != mSource->mOgrGeometryTypeFilter)
          b_condition_022=true;
       }
      }
      QString s_condition_error(QString("-W-> condition_01[%1,%2] condition_02[%3,%4] condition_021+2[%5,%6]").arg(b_condition_01).arg(useIntersect).arg(b_condition_02).arg(geometryTypeFilter).arg( b_condition_021).arg(b_condition_022));
      // qDebug() << QString( "QgsOgrFeatureIterator::readFeature[%1,%2] -E-> -1c- AttributeList.size=%3  " ).arg( feature.id() ).arg(mSource->mLayerName).arg(s_condition_error);
      OGR_F_Destroy( fet );
      return false;
    }
  }

  if ( !mFetchGeometry )
  {
    feature.setGeometry( nullptr );
  }
  // qDebug() << QString( "QgsOgrFeatureIterator::readFeature[%1,%2] -1d- AttributeList.size=%3  " ).arg( feature.id() ).arg(mSource->mLayerName).arg(mRequest.subsetOfAttributes().size());
  // fetch attributes
  if ( mRequest.flags() & QgsFeatureRequest::SubsetOfAttributes )
  { // with the new QgsOgrProvider attrs.size() is sometimes 0
    QgsAttributeList attrs = mRequest.subsetOfAttributes();
    // qDebug() << QString( "QgsOgrFeatureIterator::readFeature[%1,%2] -2- AttributeList.size=%3  " ).arg( feature.id() ).arg(mSource->mLayerName).arg(mRequest.subsetOfAttributes().size());
    for ( QgsAttributeList::const_iterator it = attrs.begin(); it != attrs.end(); ++it )
    {
      getFeatureAttribute( fet, feature, *it );
    }
  }
  else
  {
    // all attributes
    // qDebug() << QString( "QgsOgrFeatureIterator::readFeature[%1,%2] -3- size=%3  " ).arg( feature.id() ).arg(mSource->mLayerName).arg(feature.attributes().size());
    for ( int idx = 0; idx < mSource->mFields.count(); ++idx )
    {
      getFeatureAttribute( fet, feature, idx );
    }
  }
  return true;
}


QgsOgrFeatureSource::QgsOgrFeatureSource( const QgsOgrProvider* p )
    : mProvider( p )
{
  mDataSource = p->dataSourceUri();
  mLayerName = p->layerName();
  mLayerIndex = p->layerIndex();
  mSubsetString = p->mSubsetString;
  mEncoding = p->mEncoding; // no copying - this is a borrowed pointer from Qt
  mFields = p->mAttributeFields;
  for ( int i = ( p->mFirstFieldIsFid ) ? 1 : 0; i < mFields.size(); i++ )
    mFieldsWithoutFid.append( mFields.at( i ) );
  mDriverName = p->ogrDriverName;
  mFirstFieldIsFid = p->mFirstFieldIsFid;
  mOgrGeometryTypeFilter = QgsOgrProviderUtils::wkbSingleFlattenWrapper( p->mOgrGeometryTypeFilter );
  mSubLayerString = p->SubLayerString();
  QgsOgrConnPool::instance()->ref( mDataSource );
}

QgsOgrFeatureSource::~QgsOgrFeatureSource()
{
  QgsOgrConnPool::instance()->unref( mDataSource );
}

QgsFeatureIterator QgsOgrFeatureSource::getFeatures( const QgsFeatureRequest& request )
{
  return QgsFeatureIterator( new QgsOgrFeatureIterator( this, false, request ) );
}
