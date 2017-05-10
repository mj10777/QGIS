/***************************************************************************
    qgsgcplist.h - GCP list class
     --------------------------------------
    Date                 : 27-Feb-2009
    Copyright            : (c) 2009 by Manuel Massing
    Email                : m.massing at warped-space.de
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGS_GCP_LIST_H
#define QGS_GCP_LIST_H

#include <QList>
#include <QVector>
#include "qgspointxy.h"
#include "qgsgeorefdatapoint.h"
#include "qgsspatialitegcputils.h"

class QgsSpatiaLiteGcpUtils;
class QgsGeorefDataPoint;
class QgsPointXY;

// what is better use inherid or agrigate QList?
class QgsGcpList : public QList<QgsGeorefDataPoint *>
{
  public:
    QgsGcpList();
    QgsGcpList( const QgsGcpList &list );

    QgsGcpList &operator =( const QgsGcpList &list );

    /**
     * Remove all Data-Points
     *  - set to isDirty
     */
    void clearDataPoints() {clear(); setDirty();  };

    /**
     * Retrieve Data-Point pair in List
     *  - added to insure that a specific Point can be retrieved
     * \param id_gcp (either id_gcp of the database OR a count whlie loading .points file) to search for and update
     * \return nullptr if the id was not found, otherwise Point of the fiven id
     */
    QgsGeorefDataPoint *getDataPoint( int id_gcp );

    /**
     * Add a Data-Point pair to the List
     * \param dataPoint
     * \return truealways
     */
    bool addDataPoint( QgsGeorefDataPoint *dataPoint );

    /**
     * Update Point in List
     *  - added to avoid reloading all the points again
     * \param id_gcp (either id_gcp of the database OR a count whlie loading .points file) to search for and update
     * \param bPointMap (either pixel.point [false] of map.point [true]) to update at give id
     * \param updatePoint the point to use
     * \return true if the id was found, otherwise false
     */
    bool updateDataPoint( int id_gcp, bool bPointMap, QgsPointXY updatePoint );

    /**
     * Remove Point in List
     *  - added to avoid reloading all the points again
     * \param id_gcp (either id_gcp of the database OR a count whlie loading .points file) to search for and update
     * \return true if the id was found, otherwise false
     */
    bool removeDataPoint( int id_gcp );

    /**
     * Return amount of DataPoints
     *  - Enabled and Disabled
     */
    int countDataPoints() {return sizeAll(); };

    /**
     * Return amount of enabled DataPoints
     *  - Enabled only
     */
    int countDataPointsEnabled() {return size(); };

    /**
     * Return amount of disabled DataPoints
     *  - Disabled only
     */
    int countDataPointsDisabled() {return ( sizeAll() - size() ); };

    /**
     * Return mGcpDbData for Spatialite Gcp-Support
     * \return mGcpDbDatawhich contains all needed information to use Spatialite Gcp-Support
     */
    QgsSpatiaLiteGcpUtils::GcpDbData *getGcpDbData() { return mGcpDbData; }

    /**
     * Search Point in List
     *  - find a QPoint inside the List
     * \note Goals:
     *  - avoid the adding of Points that exist already
     *  - determine the need to update existing Points that are near the search Point
     * \note result_type:
     *  - 0: not found [error, should never happen]
     *  - 1: result is exact [the Points are equal]
     *  - 2: result within parameters given [QgsPoint::compare == true], but not exact, but not exact
     *  - 3: outside area  [QgsPoint::compare == false]
     * \param searchPoint QPoint to use to search the List with
     * \param bPointMap (either pixel.point [false] of map.point [true]) to update at give id
     * \param epsilon area around point to search in [1 meter around point=0.5] - as used in QgsPoint::compare
     * \param id_gcp of nearest
     * \param distance to nearest point
     * \param iResultType to nearest point
     * \see QgsPoint::compare
     * \return true result is exact, otherwise false
     */
    bool searchDataPoint( QgsPointXY searchPoint, bool bPointMap = true, double epsilon = 0.5, int *id_gcp = nullptr, double *distance = nullptr, int *iResultType = nullptr );
  protected:

    /**
     * Set mGcpDbData for Spatialite Gcp-Support
     * \param parmsGcpDbData which contains all needed information to use Spatialite Gcp-Support
     * \param bClear clear the list
     */
    void setGcpDbData( QgsSpatiaLiteGcpUtils::GcpDbData *parmsGcpDbData, bool bClear ) { mGcpDbData = parmsGcpDbData; if ( bClear ) {clearDataPoints();} };

    /**
     * Builds List of Map/Pixel Point in List that are enabled
     * \param mapCoords to store the Map-points
     * \param pixelCoords to store the Pixel-points
     * \return nullptr if the id was not found, otherwise Point of the fiven id
     */
<<<<<<< HEAD
    void createGCPVectors( QVector<QgsPointXY> &mapCoords, QVector<QgsPointXY> &pixelCoords );
=======
    void createGcpVectors( QVector<QgsPoint> &mapCoords, QVector<QgsPoint> &pixelCoords );
>>>>>>> Last major problems resolved. Start test phase

    /**
     * Informs QgsGcpList that any Updating has been done
     *  - until next Update, Adding or Delete: isDirty() = false
     */
    void setClean() { mIsDirty = false; };

    /**
     * Informs the caller if any Update, Adding or Delete have been made since the last Update.
     * \return true or false (value of mIsDirty)
     */
    bool isDirty() { return mIsDirty; }

    /**
     * Set status if  Update, Adding or Delete have been made
     * \return true or false (value of mIsDirty)
     */
    bool setDirty() { mIsDirty = true; mHasChanged = true ; return mIsDirty; }

    /**
     * Informs the caller if any changes have been made since the Initial loading of Point-Pairs.
     * \return true or false (value of mHasChanged)
     */
    bool hasChanged() { return mHasChanged; }

    /**
     * Informs QgsGcpList (when true) that the Initial loading of Point-Pairs is completed
     * \param true or false
     */
    void setChanged( bool bChanged ) { mHasChanged = bChanged; };
    bool avoidUnneededUpdates() {return mAvoidUnneededUpdates;};
    void setAvoidUnneededUpdates( bool bAvoidUnneededUpdates ) {mAvoidUnneededUpdates = bAvoidUnneededUpdates;};
  private:

    int size() const;
    int mCountEnabled = 0;
    int sizeAll() const;
    bool mIsDirty = true;
    bool mHasChanged = false;

    /**
     * Project interact with static Functions to
     *  - create, read and administer Gcp-Coverage Tables
     *  -> also using Spatialite Gcp-Logic
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \see QgsSpatiaLiteGcpUtils::GcpDbData
     */
    QgsSpatiaLiteGcpUtils::GcpDbData *mGcpDbData = nullptr;
    friend class QgsGcpListModel; // in order to access createGcpVectors, getPoint, updatePoint and removePoint
    bool mAvoidUnneededUpdates = false;
};

#endif
