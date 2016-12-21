/***************************************************************************
     qgsgeorefdatapoint.h
     --------------------------------------
    Date                 : Sun Sep 16 12:02:56 AKDT 2007
    Copyright            : (C) 2007 by Gary E. Sherman
    Email                : sherman at mrcc dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSGEOREFDATAPOINT_H
#define QGSGEOREFDATAPOINT_H

#include "qgsmapcanvasitem.h"

class QgsGCPCanvasItem;

class QgsGeorefDataPoint : public QObject
{
    Q_OBJECT

  public:
    //! constructor
    QgsGeorefDataPoint( QgsMapCanvas *srcCanvas, QgsMapCanvas *dstCanvas,
                        const QgsPoint& pixelCoords, const QgsPoint& mapCoords, int id_gcp,
                        bool enable, int i_LegacyMode );
    QgsGeorefDataPoint( const QgsGeorefDataPoint &data_point );
    ~QgsGeorefDataPoint();

    //! returns coordinates of the point
    QgsPoint pixelCoords() const { return mPixelCoords; }
    void setPixelCoords( const QgsPoint &pixel_point );

    QgsPoint pixelCoordsReverse() const { return mPixelCoords_reverse; }
    void setPixelCoordsReverse( const QgsPoint &pixel_point ) { mPixelCoords_reverse=pixel_point; }
    double pixelDistanceReverse()  { return mPixelCoords.distance(mPixelCoords_reverse); }
    double pixelAzimuthReverse()  { return fabs(mPixelCoords.azimuth(mPixelCoords_reverse)); }

    QgsPoint mapCoords() const { return mMapCoords; }
    void setMapCoords( const QgsPoint &map_point );

    QgsPoint mapCoordsReverse() const { return mMapCoords_reverse; }
    void setMapCoordsReverse( const QgsPoint &map_point )  { mMapCoords_reverse=map_point; }
    double mapDistanceReverse()  { return mMapCoords.distance(mMapCoords_reverse); }
    double mapAzimuthReverse()  { return fabs(mMapCoords.azimuth(mMapCoords_reverse)); }

    bool isEnabled() const { return mEnabled; }
    void setEnabled( bool enabled );

    int id() const { return mId; }
    void setId( int id );

    bool contains( QPoint p, bool isMapPlugin );
    /**
     * Returns pixel (Georeferencer) Canvas
     * @return mSrcCanvas pixel (Georeferencer) Canvas
     */
    QgsMapCanvas *srcCanvas() const { return mSrcCanvas; }
    /**
     * Returns map (QGis) Canvas
     * @return mDstCanvas map (QGis) Canvas
     */
    QgsMapCanvas *dstCanvas() const { return mDstCanvas; }

    QPointF residual() const { return mResidual; }
    void setResidual( QPointF r );

  public slots:
    void moveTo( QPoint, bool isMapPlugin );
    void updateCoords();

  private:
    /**
     * Pixel (Georeferencer) Canvas
     */
    QgsMapCanvas *mSrcCanvas;
    /**
     * Pixel (Georeferencer) Canvas
     */
    QgsMapCanvas *mDstCanvas;
    QgsGCPCanvasItem *mGCPSourceItem;
    QgsGCPCanvasItem *mGCPDestinationItem;
    QgsPoint mPixelCoords;
    QgsPoint mMapCoords;
    QgsPoint mPixelCoords_reverse;
    QgsPoint mMapCoords_reverse;

    int mId;
    bool mEnabled;
    int mLegacyMode;
    QPointF mResidual;
};

#endif //QGSGEOREFDATAPOINT_H
