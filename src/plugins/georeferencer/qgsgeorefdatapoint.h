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
                        const QgsPointXY &pixelCoords, const QgsPointXY &mapCoords, int i_srid, int id_gcp,
                        bool enable );
    QgsGeorefDataPoint( const QgsPointXY &pixelCoords, const QgsPointXY &mapCoords, int i_srid, int id_gcp,
                        int id_gcp_master, int iResultType, QString sTextInfo );
    QgsGeorefDataPoint( QgsGeorefDataPoint &dataPoint );
    ~QgsGeorefDataPoint();

    //! returns coordinates of the point
    QgsPointXY pixelCoords() const { return mPixelCoords; }
    void setPixelCoords( const QgsPointXY &pixelPoint );

    QgsPointXY pixelCoordsReverse() const { return mPixelCoordsReverse; }
    void setPixelCoordsReverse( const QgsPointXY &pixelPoint ) { mPixelCoordsReverse = pixelPoint; }
    double pixelDistanceReverse()  { return mPixelCoords.distance( mPixelCoordsReverse ); }
    double pixelAzimuthReverse()  { return fabs( mPixelCoords.azimuth( mPixelCoordsReverse ) ); }

    QgsPointXY mapCoords() const { return mMapCoords; }
    void setMapCoords( const QgsPointXY &map_point );

<<<<<<< HEAD
    QgsPointXY mapCoordsReverse() const { return mMapCoordsReverse; }
    void setMapCoordsReverse( const QgsPointXY &map_point )  { mMapCoordsReverse = map_point; }
=======
    /**
     * Update Point
     *  - added to avoid reloading all the points again
     * \param bPointMap (either pixel.point [false] of map.point [true]) to update at give id
     * \param updatePoint the point to use
     * \return true if the id was found, otherwise false
     */
    void updateDataPoint( bool bPointMap, QgsPoint updatePoint );

    QgsPoint mapCoordsReverse() const { return mMapCoordsReverse; }
    void setMapCoordsReverse( const QgsPoint &map_point )  { mMapCoordsReverse = map_point; }
>>>>>>> Last major problems resolved. Start test phase
    double mapDistanceReverse()  { return mMapCoords.distance( mMapCoordsReverse ); }
    double mapAzimuthReverse()  { return fabs( mMapCoords.azimuth( mMapCoordsReverse ) ); }

    bool isEnabled() const { return mEnabled; }
    void setEnabled( bool enabled );

    /**
     * Unique id of Point
     *  - corresponds to the id_gcp of the Database
     * \return  id of point
     * \since QGIS 3.0
     */
    int id() const { return mId; }

    /**
     * Name
     *  - corresponds to the name of the Database
     * \note
     * Positions divided by 'mParseString' from mTextInfo:
     *  3: name
     * \return  name of point
     * \since QGIS 3.0
     */
    QString name() const { return mName; }

    /**
     * Notes
     *  - corresponds to the notes of the Database
     * \note
     * Positions divided by 'mParseString' from mTextInfo:
     *  4: notes
     * \return  name of point
     * \since QGIS 3.0
     */
    QString notes() const { return mNotes; }

    /**
     * Set unique id of Point
     *  - corresponds to the id_gcp of the Database
     * \return  id of point
     * \since QGIS 3.0
     */
    void setId( int id );

    /**
     * getSrid()
     *  - Retrieve the Srid of the point
     * \see AsEWKT
     * \return  srid of point
     * \since QGIS 3.0
     */
    int getSrid() const { return mSrid; };

    /**
     * setSrid()
     *  - Set the Srid of the point
     * \return  srid of point
     * \since QGIS 3.0
     */
    void setSrid( int i_srid ) {mSrid = i_srid;};

    /**
     * Set Unique id of Gcp-Master Point used to create this point
     *  - for external use only
     * \return  id of point
     * \since QGIS 3.0
     */
    void setIdMaster( int id_master ) {mIdMaster = id_master;};

    /**
     * Unique id of Gcp-Master Point used to create this point
     *  - for external use only
     * \return  id of point
     * \since QGIS 3.0
     */
    int getIdMaster() const { return mIdMaster; };

    /**
     * getResultType()
     *  - How the point is being used
     *  -> UPDATE and INSERT should only be used on copys of the GcpListWidget entries
     * \note
     * Expected values:
     *  0: default value upon creation
     *  1: enabled GCP-Point
     *  2: is being used to UPDATE the Database with the present values
     *  3: is being used to INSERT into the Database with the present values
     * \see setResultType
     * \see sqlInsertPointsCoverage
     * \return  0-3
     * \since QGIS 3.0
     */
    int getResultType() const { return mResultType; };

    /**
     * setResultType()
     *  - How the point is to be used
     *  -> UPDATE and INSERT should only be used on copiess of the GcpListWidget entries
     * \note
     * Expected values:
     *  0: default value upon creation
     *  1: enabled GCP-Point
     *  2: is being used to UPDATE the Database with the present values
     *  3: is being used to INSERT into the Database with the present values
     * \see getResultType
     * \return  0-3
     * \since QGIS 3.0
     */
    void setResultType( int iResultType ) {mResultType = iResultType;};

    /**
     * getTextInfo()
     *  - Formatted String to be used as Meta-Data when creating a point from a GCP-Master entry
     *  -> UPDATE and INSERT Sql created will use these values
     * \note
     * UPDATE  will only use the following fields:
     *  3: name
     *  4: notes
     *  6: gcp_text
     * \see sqlInsertPointsCoverage
     * \since QGIS 3.0
     */
    QString getTextInfo();

    /**
     * setTextInfo()
     *  - Formatted String to be used as Meta-Data when creating a point from a GCP-Master entry
     *  -> UPDATE and INSERT Sql created will use these values
     * \note
     * Positions divided by 'mParseString':
     *  0: id_gcp
     *  1: id_gcp_coverage
     *  2: gcp_points_table_name
     *  3: name
     *  4: notes
     *  5: transformparm
     *  6: gcp_text
     * \see sqlInsertPointsCoverage
     * \see QgsGeorefPluginGui::fetchGcpMasterPointsExtent
     * \since QGIS 3.0
     */
    void setTextInfo( QString sTextInfo );

    /**
     * AsEWKT
     *  - Format point a EWKT for INSERT / UPDATE Sql-Statements
     *  -> using the set Srid of the point
     * \note
     * Expected values for iPointType  pixel:
     *  0: Pixel-Value
     *  1: Map-Value
     *  2: reversed Pixel-Value
     *  3: reversed Map-Value
     * \note
     * Gcp-Master : when contains another srid:
     * - the srid of the master is given and transformed into the srid of the project
     * \see GeomFromEWKT
     * \param iPointType  pixel Pixel/Map/Reversed
     * \param i_srid srid to use for ST_Transform
     * \return  Spatialite specific formatted String as EWKT of point
     * \since QGIS 3.0
     */
    QString AsEWKT( int iPointType = 1, int i_srid = -1 ) const;

    /**
     * sqlInsertPointsCoverage
     *  - Depending on how point is being used
     *  -> UPDATE and INSERT should only be used on copys of the GcpListWidget entries
     * \note
     * Used values of mResultType:
     *  2: is being used to UPDATE the Database with the present values
     *  3: is being used to INSERT into the Database with the present values
     * \see getResultType
     * \see setResultType
     * \see getTextInfo
     * \see AsEWKT
     * \see QgsGeorefPluginGui::bulkGcpPointsInsert
     * \param i_srid srid to use for ST_Transform during AsEWKT
     * \return valid SQL to UPDATE or INSERT the point
     * \since QGIS 3.0
     */
    QString sqlInsertPointsCoverage( int i_srid = -1 );

    /**
     * GeomFromEWKT
     *  - Format point a EWKT for INSERT / UPDATE Sql-Statements
     *  -> using the set Srid of the point
     * \note
     * Expected values:
     *  0: Pixel-Value
     *  1: Map-Value
     *  2: reversed Pixel-Value
     *  3: reversed Map-Value
     * \see AsEWKT
     * \param iPointType  pixel Pixel/Map/Reversed
     * \param i_srid srid to use for ST_Transform
     * \return  Spatialite specific GeomFromEWKT command, with formatted String as EWKT of point
     * \since QGIS 3.0
     */
    QString GeomFromEWKT( int iPointType = 1, int i_srid = -1 ) const { return QString( "GeomFromEWKT('%1')" ).arg( AsEWKT( iPointType, i_srid ) ); };

    /**
     * Search for a Point in list
     * \param searchPoint  Point to compare
     * \param isPixelPoint PixelPoint[true], MapPoint[false]
     * \return true if QgsPointXY.compare returns true
     * \see moveTo
     * \since QGIS 3.0
     */
    bool contains( QPointXY searchPoint, bool isPixelPoint );

    /**
     * Returns pixel (Georeferencer) Canvas
     * \return mSrcCanvas pixel (Georeferencer) Canvas
     * \since QGIS 3.0
     */
    QgsMapCanvas *srcCanvas() const { return mSrcCanvas; }

    /**
     * Returns map (QGis) Canvas mLegacyMode=0 ;
     * \return mDstCanvas map (QGis) Canvas
     * \since QGIS 3.0
     */
    QgsMapCanvas *dstCanvas() const { return mDstCanvas; }

    QPointF residual() const { return mResidual; }
    void setResidual( QPointF r );

  public slots:

    /**
     * Move to the given Point in te given Canvas
     * \param isPixelPoint PixelPoint[true], MapPoint[false]
     * \see QgsGeorefPluginGui::jumpToGCP
     * \since QGIS 3.0
     */
    void moveTo( bool isPixelPoint );

  private:

    /**
     * Pixel (Georeferencer) Canvas
     */
    QgsMapCanvas *mSrcCanvas = nullptr;

    /**
     * Map (QGis) Canvas
     */
    QgsMapCanvas *mDstCanvas = nullptr;
    QgsPointXY mPixelCoords;
    QgsPointXY mMapCoords;
    QgsPointXY mPixelCoordsReverse;
    QgsPointXY mMapCoordsReverse;

    int mId = 0;
    bool mEnabled = false;
    QPointF mResidual;
    QString mTextInfo;
    QString mName;
    QString mNotes;
    int mIdMaster = 0;
    int mResultType = 0;
    int mSrid = -1;
    QString mParseString;
};

#endif //QGSGEOREFDATAPOINT_H
