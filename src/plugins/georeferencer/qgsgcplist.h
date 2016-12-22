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
#include "qgspoint.h"
#include "qgsgeorefdatapoint.h"
#include "qgsspatialiteprovidergcputils.h"

class QgsSpatiaLiteProviderGcpUtils;
class QgsGeorefDataPoint;
class QgsPoint;

// what is better use inherid or agrigate QList?
class QgsGCPList : public QList<QgsGeorefDataPoint *>
{
  public:
    QgsGCPList();
    QgsGCPList( const QgsGCPList &list );

    QgsGCPList &operator =( const QgsGCPList &list );
    /**
     * Remove all Data-Points
     *  - set to isDirty
     */
    void clearDataPoints() {clear(); setDirty();  };
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
    int countDataPointsDisabled() {return (sizeAll()-size()); };
    /**
     * Return mGcpDbData for Spatialite Gcp-Support
     * @return mGcpDbDatawhich contains all needed information to use Spatialite Gcp-Support
     */
    QgsSpatiaLiteProviderGcpUtils::GcpDbData* getGcpDbData() { return mGcpDbData; }
   protected:
    /**
     * Set mGcpDbData for Spatialite Gcp-Support
     * @param parms_GcpDbData which contains all needed information to use Spatialite Gcp-Support
     * @param b_clear clear the list
     */
    void setGcpDbData(QgsSpatiaLiteProviderGcpUtils::GcpDbData* parms_GcpDbData, bool b_clear) { mGcpDbData=parms_GcpDbData; if (b_clear) {clearDataPoints();} };
    /**
     * Builds List of Map/Pixel Point in List that are enabled
     * @param mapCoords to store the Map-points
     * @param pixelCoords to store the Pixel-points
     * @return nullptr if the id was not found, otherwise Point of the fiven id
     */
    void createGCPVectors( QVector<QgsPoint> &mapCoords, QVector<QgsPoint> &pixelCoords );
    /**
     * Retrieve Data-Point pair in List
     *  - added to insure that a specific Point can be retrieved
     * @param id (either id_gcp of the database OR a count whlie loading .points file) to search for and update
     * @return nullptr if the id was not found, otherwise Point of the fiven id
     */
    QgsGeorefDataPoint* getDataPoint( int id );
    /**
     * Add a Data-Point pair to the List
     * @param data_point 
     * @return truealways
     */
    bool addDataPoint(QgsGeorefDataPoint* data_point );
    /**
     * Update Point in List
     *  - added to avoid reloading all the points again
     * @param id (either id_gcp of the database OR a count whlie loading .points file) to search for and update
     * @param b_point_map (either pixel.point [false] of map.point [true]) to update at give id
     * @param update_point the point to use
     * @return true if the id was found, otherwise false
     */
    bool updateDataPoint( int id, bool b_point_map, QgsPoint update_point );
    /**
     * Remove Point in List
     *  - added to avoid reloading all the points again
     * @param id (either id_gcp of the database OR a count whlie loading .points file) to search for and update
     * @return true if the id was found, otherwise false
     */
    bool removeDataPoint( int id );
    /**
     * Informs QgsGCPList that any Updating has been done
     *  - until next Update, Adding or Delete: isDirty() = false
     */
    void setClean() { mIsDirty=false; };
    /**
     * Informs the caller if any Update, Adding or Delete have been made since the last Update.
     * @return true or false (value of mIsDirty)
     */
    bool isDirty() { return mIsDirty; }
    /**
     * Informs the caller if any changes have been made since the Initial loading of Point-Pairs.
     * @return true or false (value of mHasChanged)
     */
    bool hasChanged() { return mHasChanged; }
    /**
     * Informs QgsGCPList (when true) that the Initial loading of Point-Pairs is compleated
     * @param true or false
     */
    void setChanged(bool b_changed) { mHasChanged=b_changed; };
  private:

    int size();
    int i_count_enabled;
    int sizeAll() const;
    bool mIsDirty;
    bool mHasChanged;
    /**
     * Project interact with static Functions to
     *  - create, read and administer Gcp-Coverage Tables
     *  -> also using Spatialite Gcp-Logic
     * @note QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * @see QgsSpatiaLiteProviderGcpUtils::GcpDbData
     */
    QgsSpatiaLiteProviderGcpUtils::GcpDbData* mGcpDbData;
    bool setDirty() { mIsDirty=true; mHasChanged=true ; return mIsDirty; }
    friend class QgsGCPListModel; // in order to access createGCPVectors, getPoint, updatePoint and removePoint
};

#endif
