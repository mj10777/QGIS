/***************************************************************************
  qgsdemterraintilegeometry_p.cpp
  --------------------------------------
  Date                 : July 2017
  Copyright            : (C) 2017 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsdemterraintilegeometry_p.h"
#include <Qt3DRender/qattribute.h>
#include <Qt3DRender/qbuffer.h>
#include <Qt3DRender/qbufferdatagenerator.h>
#include <limits>

///@cond PRIVATE

using namespace Qt3DRender;


static QByteArray createPlaneVertexData( int res, float skirtHeight, const QByteArray &heights )
{
  Q_ASSERT( res >= 2 );
  Q_ASSERT( heights.count() == res * res * ( int )sizeof( float ) );

  const float *zData = ( const float * ) heights.constData();
  const float *zBits = zData;

  const int nVerts = ( res + 2 ) * ( res + 2 );

  // Populate a buffer with the interleaved per-vertex data with
  // vec3 pos, vec2 texCoord, vec3 normal, vec4 tangent
  const quint32 elementSize = 3 + 2 + 3;
  const quint32 stride = elementSize * sizeof( float );
  QByteArray bufferBytes;
  bufferBytes.resize( stride * nVerts );
  float *fptr = reinterpret_cast<float *>( bufferBytes.data() );

  float w = 1, h = 1;
  QSize resolution( res, res );
  const float x0 = -w / 2.0f;
  const float z0 = -h / 2.0f;
  const float dx = w / ( resolution.width() - 1 );
  const float dz = h / ( resolution.height() - 1 );
  const float du = 1.0 / ( resolution.width() - 1 );
  const float dv = 1.0 / ( resolution.height() - 1 );

  // Iterate over z
  for ( int j = -1; j <= resolution.height(); ++j )
  {
    int jBound = qBound( 0, j, resolution.height() - 1 );
    const float z = z0 + static_cast<float>( jBound ) * dz;
    const float v = static_cast<float>( jBound ) * dv;

    // Iterate over x
    for ( int i = -1; i <= resolution.width(); ++i )
    {
      int iBound = qBound( 0, i, resolution.width() - 1 );
      const float x = x0 + static_cast<float>( iBound ) * dx;
      const float u = static_cast<float>( iBound ) * du;

      float height;
      if ( i == iBound && j == jBound )
        height = *zBits++;
      else
        height = zData[ jBound * resolution.width() + iBound ] - skirtHeight;

      // position
      *fptr++ = x;
      *fptr++ = height;
      *fptr++ = z;

      // texture coordinates
      *fptr++ = u;
      *fptr++ = v;

      // TODO: compute correct normals based on neighboring pixels
      // normal
      *fptr++ = 0.0f;
      *fptr++ = 1.0f;
      *fptr++ = 0.0f;
    }
  }

  return bufferBytes;
}


static QByteArray createPlaneIndexData( int res )
{
  QSize resolution( res, res );
  int numVerticesX = resolution.width() + 2;
  int numVerticesZ = resolution.height() + 2;

  // Create the index data. 2 triangles per rectangular face
  const int faces = 2 * ( numVerticesX - 1 ) * ( numVerticesZ - 1 );
  const quint32 indices = 3 * faces;
  Q_ASSERT( indices < std::numeric_limits<quint32>::max() );
  QByteArray indexBytes;
  indexBytes.resize( indices * sizeof( quint32 ) );
  quint32 *indexPtr = reinterpret_cast<quint32 *>( indexBytes.data() );

  // Iterate over z
  for ( int j = 0; j < numVerticesZ - 1; ++j )
  {
    const int rowStartIndex = j * numVerticesX;
    const int nextRowStartIndex = ( j + 1 ) * numVerticesX;

    // Iterate over x
    for ( int i = 0; i < numVerticesX - 1; ++i )
    {
      // Split quad into two triangles
      *indexPtr++ = rowStartIndex + i;
      *indexPtr++ = nextRowStartIndex + i;
      *indexPtr++ = rowStartIndex + i + 1;

      *indexPtr++ = nextRowStartIndex + i;
      *indexPtr++ = nextRowStartIndex + i + 1;
      *indexPtr++ = rowStartIndex + i + 1;
    }
  }

  return indexBytes;
}



//! Generates vertex buffer for DEM terrain tiles
class PlaneVertexBufferFunctor : public QBufferDataGenerator
{
  public:
    explicit PlaneVertexBufferFunctor( int resolution, float skirtHeight, const QByteArray &heightMap )
      : mResolution( resolution )
      , mSkirtHeight( skirtHeight )
      , mHeightMap( heightMap )
    {}

    QByteArray operator()() final
    {
      return createPlaneVertexData( mResolution, mSkirtHeight, mHeightMap );
    }

    bool operator ==( const QBufferDataGenerator &other ) const final
    {
      const PlaneVertexBufferFunctor *otherFunctor = functor_cast<PlaneVertexBufferFunctor>( &other );
      if ( otherFunctor != nullptr )
        return ( otherFunctor->mResolution == mResolution &&
                 otherFunctor->mSkirtHeight == mSkirtHeight &&
                 otherFunctor->mHeightMap == mHeightMap );
      return false;
    }

    QT3D_FUNCTOR( PlaneVertexBufferFunctor )

  private:
    int mResolution;
    float mSkirtHeight;
    QByteArray mHeightMap;
};


//! Generates index buffer for DEM terrain tiles
class PlaneIndexBufferFunctor : public QBufferDataGenerator
{
  public:
    explicit PlaneIndexBufferFunctor( int resolution )
      : mResolution( resolution )
    {}

    QByteArray operator()() final
    {
      return createPlaneIndexData( mResolution );
    }

    bool operator ==( const QBufferDataGenerator &other ) const final
    {
      const PlaneIndexBufferFunctor *otherFunctor = functor_cast<PlaneIndexBufferFunctor>( &other );
      if ( otherFunctor != nullptr )
        return ( otherFunctor->mResolution == mResolution );
      return false;
    }

    QT3D_FUNCTOR( PlaneIndexBufferFunctor )

  private:
    int mResolution;
};


// ------------


DemTerrainTileGeometry::DemTerrainTileGeometry( int resolution, float skirtHeight, const QByteArray &heightMap, DemTerrainTileGeometry::QNode *parent )
  : QGeometry( parent )
  , mResolution( resolution )
  , mSkirtHeight( skirtHeight )
  , mHeightMap( heightMap )
{
  init();
}

void DemTerrainTileGeometry::init()
{
  mPositionAttribute = new QAttribute( this );
  mNormalAttribute = new QAttribute( this );
  mTexCoordAttribute = new QAttribute( this );
  mIndexAttribute = new QAttribute( this );
  mVertexBuffer = new Qt3DRender::QBuffer( Qt3DRender::QBuffer::VertexBuffer, this );
  mIndexBuffer = new Qt3DRender::QBuffer( Qt3DRender::QBuffer::IndexBuffer, this );

  int nVertsX = mResolution + 2;
  int nVertsZ = mResolution + 2;
  const int nVerts = nVertsX * nVertsZ;
  const int stride = ( 3 + 2 + 3 ) * sizeof( float );
  const int faces = 2 * ( nVertsX - 1 ) * ( nVertsZ - 1 );

  mPositionAttribute->setName( QAttribute::defaultPositionAttributeName() );
  mPositionAttribute->setVertexBaseType( QAttribute::Float );
  mPositionAttribute->setVertexSize( 3 );
  mPositionAttribute->setAttributeType( QAttribute::VertexAttribute );
  mPositionAttribute->setBuffer( mVertexBuffer );
  mPositionAttribute->setByteStride( stride );
  mPositionAttribute->setCount( nVerts );

  mTexCoordAttribute->setName( QAttribute::defaultTextureCoordinateAttributeName() );
  mTexCoordAttribute->setVertexBaseType( QAttribute::Float );
  mTexCoordAttribute->setVertexSize( 2 );
  mTexCoordAttribute->setAttributeType( QAttribute::VertexAttribute );
  mTexCoordAttribute->setBuffer( mVertexBuffer );
  mTexCoordAttribute->setByteStride( stride );
  mTexCoordAttribute->setByteOffset( 3 * sizeof( float ) );
  mTexCoordAttribute->setCount( nVerts );

  mNormalAttribute->setName( QAttribute::defaultNormalAttributeName() );
  mNormalAttribute->setVertexBaseType( QAttribute::Float );
  mNormalAttribute->setVertexSize( 3 );
  mNormalAttribute->setAttributeType( QAttribute::VertexAttribute );
  mNormalAttribute->setBuffer( mVertexBuffer );
  mNormalAttribute->setByteStride( stride );
  mNormalAttribute->setByteOffset( 5 * sizeof( float ) );
  mNormalAttribute->setCount( nVerts );

  mIndexAttribute->setAttributeType( QAttribute::IndexAttribute );
  mIndexAttribute->setVertexBaseType( QAttribute::UnsignedInt );
  mIndexAttribute->setBuffer( mIndexBuffer );

  // Each primitive has 3 vertives
  mIndexAttribute->setCount( faces * 3 );

  mVertexBuffer->setDataGenerator( QSharedPointer<PlaneVertexBufferFunctor>::create( mResolution, mSkirtHeight, mHeightMap ) );
  mIndexBuffer->setDataGenerator( QSharedPointer<PlaneIndexBufferFunctor>::create( mResolution ) );

  addAttribute( mPositionAttribute );
  addAttribute( mTexCoordAttribute );
  addAttribute( mNormalAttribute );
  addAttribute( mIndexAttribute );
}

/// @endcond
