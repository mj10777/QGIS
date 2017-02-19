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
                        const QgsPoint& pixelCoords, const QgsPoint& mapCoords, int i_srid, int id_gcp,
                        bool enable, int i_LegacyMode );
    QgsGeorefDataPoint( const QgsPoint& pixelCoords, const QgsPoint& mapCoords, int i_srid, int id_gcp,
                        int id_gcp_master, int i_ResultType, QString s_TextInfo );
    QgsGeorefDataPoint( const QgsGeorefDataPoint &data_point );
    ~QgsGeorefDataPoint();

    //! returns coordinates of the point
    QgsPoint pixelCoords() const { return mPixelCoords; }
    void setPixelCoords( const QgsPoint &pixel_point );

    QgsPoint pixelCoordsReverse() const { return mPixelCoords_reverse; }
    void setPixelCoordsReverse( const QgsPoint &pixel_point ) { mPixelCoords_reverse = pixel_point; }
    double pixelDistanceReverse()  { return mPixelCoords.distance( mPixelCoords_reverse ); }
    double pixelAzimuthReverse()  { return fabs( mPixelCoords.azimuth( mPixelCoords_reverse ) ); }

    QgsPoint mapCoords() const { return mMapCoords; }
    void setMapCoords( const QgsPoint &map_point );

    QgsPoint mapCoordsReverse() const { return mMapCoords_reverse; }
    void setMapCoordsReverse( const QgsPoint &map_point )  { mMapCoords_reverse = map_point; }
    double mapDistanceReverse()  { return mMapCoords.distance( mMapCoords_reverse ); }
    double mapAzimuthReverse()  { return fabs( mMapCoords.azimuth( mMapCoords_reverse ) ); }

    bool isEnabled() const { return mEnabled; }
    void setEnabled( bool enabled );

    int id() const { return mId; }
    void setId( int id );
    /**
     * getSrid()
     *  - Retrieve the Srid of the point
     * @see AsEWKT
     * @return  srid of point
     */
    int getSrid() const { return mSrid; };
    /**
     * setSrid()
     *  - Set the Srid of the point
     * @return  srid of point
     */
    void setSrid( int i_srid ) {mSrid = i_srid;};
    void setIdMaster( int id_master ) {mIdMaster = id_master;};
    int getIdMaster() const { return mIdMaster; };
    /**
     * getResultType()
     *  - How the point is being used
     *  -> UPDATE and INSERT should only be used on copys of the GCPListWidget entries
     * @note
     * Expected values:
     *  0: default value upon creation
     *  1: enabled GCP-Point
     *  2: is being used to UPDATE the Database with the present values
     *  3: is being used to INSERT into the Database with the present values
     * @see setResultType
     * @see sqlInsertPointsCoverage
     * @return  0-3
     */
    int getResultType() const { return mResultType; };
    /**
     * setResultType()
     *  - How the point is to be used
     *  -> UPDATE and INSERT should only be used on copiess of the GCPListWidget entries
     * @note
     * Expected values:
     *  0: default value upon creation
     *  1: enabled GCP-Point
     *  2: is being used to UPDATE the Database with the present values
     *  3: is being used to INSERT into the Database with the present values
     * @see getResultType
     * @return  0-3
     */
    void setResultType( int i_ResultType ) {mResultType = i_ResultType;};
    /**
     * getTextInfo()
     *  - Formatted String to be used as Meta-Data when creating a point from a GCP-Master entry
     *  -> UPDATE and INSERT Sql created will use these values
     * @note
     * UPDATE  will only use the following fields:
     *  3: name
     *  4: notes
     *  6: gcp_text
     * @see sqlInsertPointsCoverage
     */
    QString getTextInfo() const { return mTextInfo; };
    /**
     * setTextInfo()
     *  - Formatted String to be used as Meta-Data when creating a point from a GCP-Master entry
     *  -> UPDATE and INSERT Sql created will use these values
     * @note
     * Positions devided by 'mParseString':
     *  0: id_gcp
     *  1: id_gcp_coverage
     *  2: gcp_points_table_name
     *  3: name
     *  4: notes
     *  5: transformparm
     *  6: gcp_text
     * @see sqlInsertPointsCoverage
     * @see QgsGeorefPluginGui::fetchGcpMasterPointsExtent
     */
    void setTextInfo( QString s_TextInfo ) {mTextInfo = s_TextInfo;};
    /**
     * AsEWKT
     *  - Format point a EWKT for INSERT / UPDATE Sql-Statements
     *  -> using the set Srid of the point
     * @note
     * Expected values:
     *  0: Pixel-Value
     *  1: Map-Value
     *  2: reversed Pixel-Value
     *  3: reversed Map-Value
     * @see GeomFromEWKT
     * @return  Spatialite specific formated String as EWKT of point
     */
    QString AsEWKT( int i_point_type = 1 ) const;
    /**
     * sqlInsertPointsCoverage
     *  - Depending on how point is being used
     *  -> UPDATE and INSERT should only be used on copys of the GCPListWidget entries
     * @note
     * Used values of mResultType:
     *  2: is being used to UPDATE the Database with the present values
     *  3: is being used to INSERT into the Database with the present values
     * @see getResultType
     * @see setResultType
     * @see getTextInfo
     * @see QgsGeorefPluginGui::bulkGcpPointsInsert
     * @return valid SQL to UPDATE or INSERT the point
     */
    QString sqlInsertPointsCoverage() const;
    /**
     * GeomFromEWKT
     *  - Format point a EWKT for INSERT / UPDATE Sql-Statements
     *  -> using the set Srid of the point
     * @note
     * Expected values:
     *  0: Pixel-Value
     *  1: Map-Value
     *  2: reversed Pixel-Value
     *  3: reversed Map-Value
     * @see AsEWKT
     * @return  Spatialite specific GeomFromEWKT command, with formated String as EWKT of point
     */
    QString GeomFromEWKT( int i_point_type = 1 ) const { return QString( "GeomFromEWKT('%1')" ).arg( AsEWKT( i_point_type ) ); };
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
    QString mTextInfo;
    int mIdMaster;
    int mResultType;
    int mSrid;
    QString mParseString;
};

#endif //QGSGEOREFDATAPOINT_H
