/***************************************************************************
  qgsvector.h - QgsVector

 ---------------------
 begin                : 24.2.2017
 copyright            : (C) 2017 by Matthias Kuhn
 email                : matthias@opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSVECTOR_H
#define QGSVECTOR_H

#include "qgis_core.h"

/**
 * \ingroup core
 * A class to represent a vector.
 * Currently no Z axis / 2.5D support is implemented.
 */

class CORE_EXPORT QgsVector
{

  public:

    /**
     * Default constructor for QgsVector. Creates a vector with length of 0.0.
     */
    QgsVector() = default;

    /**
     * Constructor for QgsVector taking x and y component values.
     * \param x x-component
     * \param y y-component
     */
    QgsVector( double x, double y );

    //! Swaps the sign of the x and y components of the vector.
    QgsVector operator-() const;

    /**
     * Returns a vector where the components have been multiplied by a scalar value.
     * \param scalar factor to multiply by
     */
    QgsVector operator*( double scalar ) const;

    /**
     * Returns a vector where the components have been divided by a scalar value.
     * \param scalar factor to divide by
     */
    QgsVector operator/( double scalar ) const;

    /**
     * Returns the dot product of two vectors, which is the sum of the x component
     *  of this vector multiplied by the x component of another
     *  vector plus the y component of this vector multiplied by the y component of another vector.
     */
    double operator*( QgsVector v ) const;

    /**
     * Adds another vector to this vector.
     * \since QGIS 3.0
     */
    QgsVector operator+( QgsVector other ) const;

    /**
     * Adds another vector to this vector in place.
     * \since QGIS 3.0
     */
    QgsVector &operator+=( const QgsVector other );

    /**
     * Subtracts another vector to this vector.
     * \since QGIS 3.0
     */
    QgsVector operator-( const QgsVector other ) const;

    /**
     * Subtracts another vector to this vector in place.
     * \since QGIS 3.0
     */
    QgsVector &operator-=( const QgsVector other );

    /**
     * Returns the length of the vector.
     */
    double length() const;

    /**
     * Returns the vector's x-component.
     * \see y()
     */
    double x() const;

    /**
     * Returns the vector's y-component.
     * \see x()
     */
    double y() const;

    /**
     * Returns the perpendicular vector to this vector (rotated 90 degrees counter-clockwise)
     */
    QgsVector perpVector() const;

    /**
     * Returns the angle of the vector in radians.
     */
    double angle() const;

    /**
     * Returns the angle between this vector and another vector in radians.
     */
    double angle( QgsVector v ) const;

    /**
     * Rotates the vector by a specified angle.
     * \param rot angle in radians
     */
    QgsVector rotateBy( double rot ) const;

    /**
     * Returns the vector's normalized (or "unit") vector (ie same angle but length of 1.0).
     * Will throw a QgsException if called on a vector with length of 0.
     */
    QgsVector normalized() const;

    //! Equality operator
    bool operator==( QgsVector other ) const;

    //! Inequality operator
    bool operator!=( QgsVector other ) const;

  private:
    double mX = 0.0, mY = 0.0;

};

#endif // QGSVECTOR_H
