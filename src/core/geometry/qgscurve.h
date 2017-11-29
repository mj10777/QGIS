/***************************************************************************
                         qgscurve.h
                         ------------
    begin                : September 2014
    copyright            : (C) 2014 by Marco Hugentobler
    email                : marco at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSCURVEV2_H
#define QGSCURVEV2_H

#include "qgis_core.h"
#include "qgis.h"
#include "qgsabstractgeometry.h"
#include "qgsrectangle.h"

class QgsLineString;
class QPainterPath;

/**
 * \ingroup core
 * \class QgsCurve
 * \brief Abstract base class for curved geometry type
 * \since QGIS 2.10
 */
class CORE_EXPORT QgsCurve: public QgsAbstractGeometry
{
  public:

    /**
     * Constructor for QgsCurve.
     */
    QgsCurve() = default;

    virtual bool operator==( const QgsCurve &other ) const = 0;
    virtual bool operator!=( const QgsCurve &other ) const = 0;

    QgsCurve *clone() const override = 0 SIP_FACTORY;

    /**
     * Returns the starting point of the curve.
     * \see endPoint
     */
    virtual QgsPoint startPoint() const = 0;

    /**
     * Returns the end point of the curve.
     * \see startPoint
     */
    virtual QgsPoint endPoint() const = 0;

    /**
     * Returns true if the curve is closed.
     */
    virtual bool isClosed() const;

    /**
     * Returns true if the curve is a ring.
     */
    virtual bool isRing() const;

    /**
     * Returns a new line string geometry corresponding to a segmentized approximation
     * of the curve.
     * \param tolerance segmentation tolerance
     * \param toleranceType maximum segmentation angle or maximum difference between approximation and curve
     *
     * Uses a MaximumAngle tolerance of 1 degrees by default (360
     * segments in a full circle)
     */
    virtual QgsLineString *curveToLine( double tolerance = M_PI_2 / 90, SegmentationToleranceType toleranceType = MaximumAngle ) const = 0 SIP_FACTORY;

    /**
     * Adds a curve to a painter path.
     */
    virtual void addToPainterPath( QPainterPath &path ) const = 0;

    /**
     * Draws the curve as a polygon on the specified QPainter.
     * \param p destination QPainter
     */
    virtual void drawAsPolygon( QPainter &p ) const = 0;

    /**
     * Returns a list of points within the curve.
     */
    virtual void points( QgsPointSequence &pt SIP_OUT ) const = 0;

    /**
     * Returns the number of points in the curve.
     */
    virtual int numPoints() const = 0;

    /**
     * Sums up the area of the curve by iterating over the vertices (shoelace formula).
     */
    virtual void sumUpArea( double &sum SIP_OUT ) const = 0;

    QgsCoordinateSequence coordinateSequence() const override;
    bool nextVertex( QgsVertexId &id, QgsPoint &vertex SIP_OUT ) const override;
    void adjacentVertices( QgsVertexId vertex, QgsVertexId &previousVertex SIP_OUT, QgsVertexId &nextVertex SIP_OUT ) const override;
    int vertexNumberFromVertexId( QgsVertexId id ) const override;

    /**
     * Returns the point and vertex id of a point within the curve.
     * \param node node number, where the first node is 0
     * \param point will be set to point at corresponding node in the curve
     * \param type will be set to the vertex type of the node
     * \returns true if node exists within the curve
     */
    virtual bool pointAt( int node, QgsPoint &point SIP_OUT, QgsVertexId::VertexType &type SIP_OUT ) const = 0;

    /**
     * Returns a reversed copy of the curve, where the direction of the curve has been flipped.
     * \since QGIS 2.14
     */
    virtual QgsCurve *reversed() const = 0 SIP_FACTORY;

    QgsAbstractGeometry *boundary() const override SIP_FACTORY;

    /**
     * Returns a geometry without curves. Caller takes ownership
     * \param tolerance segmentation tolerance
     * \param toleranceType maximum segmentation angle or maximum difference between approximation and curve*/
    QgsCurve *segmentize( double tolerance = M_PI_2 / 90, SegmentationToleranceType toleranceType = MaximumAngle ) const override SIP_FACTORY;

    int vertexCount( int part = 0, int ring = 0 ) const override;
    int ringCount( int part = 0 ) const override;
    int partCount() const override;
    QgsPoint vertexAt( QgsVertexId id ) const override;
    QgsCurve *toCurveType() const override SIP_FACTORY;

    QgsRectangle boundingBox() const override;

    /**
     * Returns the x-coordinate of the specified node in the line string.
    * \param index index of node, where the first node in the line is 0
    * \returns x-coordinate of node, or 0.0 if index is out of bounds
    * \see setXAt()
    */
    virtual double xAt( int index ) const = 0;

    /**
     * Returns the y-coordinate of the specified node in the line string.
     * \param index index of node, where the first node in the line is 0
     * \returns y-coordinate of node, or 0.0 if index is out of bounds
     * \see setYAt()
     */
    virtual double yAt( int index ) const = 0;

    /**
     * Returns a QPolygonF representing the points.
     */
    QPolygonF asQPolygonF() const;

#ifndef SIP_RUN

    /**
     * Cast the \a geom to a QgsCurve.
     * Should be used by qgsgeometry_cast<QgsCurve *>( geometry ).
     *
     * \note Not available in Python. Objects will be automatically be converted to the appropriate target type.
     * \since QGIS 3.0
     */
    inline const QgsCurve *cast( const QgsAbstractGeometry *geom ) const
    {
      if ( !geom )
        return nullptr;

      QgsWkbTypes::Type type = geom->wkbType();
      if ( QgsWkbTypes::geometryType( type ) == QgsWkbTypes::LineGeometry && QgsWkbTypes::isSingleType( type ) )
      {
        return static_cast<const QgsCurve *>( geom );
      }
      return nullptr;
    }
#endif


  protected:

    void clearCache() const override;

    virtual int childCount() const override;
    virtual QgsPoint childPoint( int index ) const override;

#ifndef SIP_RUN

    /**
     * Helper function for QgsCurve subclasses to snap to grids.
     * \note Not available in Python bindings.
     */
    bool snapToGridPrivate( double hSpacing, double vSpacing, double dSpacing, double mSpacing,
                            const QVector<double> &srcX, const QVector<double> &srcY, const QVector<double> &srcZ, const QVector<double> &srcM,
                            QVector<double> &outX, QVector<double> &outY, QVector<double> &outZ, QVector<double> &outM ) const;
#endif

  private:

    mutable QgsRectangle mBoundingBox;
};

#endif // QGSCURVEV2_H
