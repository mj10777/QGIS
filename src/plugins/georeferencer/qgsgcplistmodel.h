/***************************************************************************
    qgsgcplistmodel.h - Model implementation of GcpList Model/View
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

#ifndef QGSGCP_LIST_TABLE_VIEW_H
#define QGSGCP_LIST_TABLE_VIEW_H

#include <QTreeView>
#include <QStandardItemModel>
#include "qgsgcplist.h"

class QgsGeorefDataPoint;
class QgsGeorefTransform;
<<<<<<< HEAD
class QgsGCPList;
class QgsPointXY;
=======
class QgsGcpList;
class QgsPoint;
>>>>>>> Last major problems resolved. Start test phase

class QgsGcpListModel : public QStandardItemModel
{
    Q_OBJECT

  public:
    explicit QgsGcpListModel( QObject *parent = nullptr );

    void setGcpList( QgsGcpList *gcpList );
    void setGeorefTransform( QgsGeorefTransform *georefTransform );

    /**
     * Update Modual
     *  - calls createGcpVectors to recreate List of enabled Points
     *  - rebuilds List calculating residual value
     * \see QgsGcpList::updatePoint
     * \see QgsGcpList::removePoint
     * \see QgsGcpList::createGcpVectors
     * \return true if the id was found, otherwise false
     */
    void updateModel();
    bool transformUpdated() { return bTransformUpdated; }
    bool mapUnitsPossible() { return bMapUnitsPossible; }

    /**
     * Calculates root mean squared error for the currently active
     * ground control points and transform method.
     * Note that the RMSE measure is adjusted for the degrees of freedom of the
     * used polynomial transform.
     * \param error out: the mean error
     * \return true in case of success
     */
    bool calculateMeanError( double &error ) const;

  public slots:
    void replaceDataPoint( QgsGeorefDataPoint *newDataPoint, int i );

    void onGcpListModified();
    void onTransformationModified();
  protected:

    /**
     * Builds List of Map/Pixel Point in List that are enabled
     * \param mapCoords to store the Map-points
     * \param pixelCoords to store the Pixel-points
     * \return nullptr if the id was not found, otherwise Point of the fiven id
     */
<<<<<<< HEAD
    void createGCPVectors( QVector<QgsPointXY> &mapCoords, QVector<QgsPointXY> &pixelCoords ) { return  mGCPList->createGCPVectors( mapCoords, pixelCoords ); }
=======
    void createGcpVectors( QVector<QgsPoint> &mapCoords, QVector<QgsPoint> &pixelCoords ) { return  mGcpList->createGcpVectors( mapCoords, pixelCoords ); }
>>>>>>> Last major problems resolved. Start test phase

    /**
     * Retrieve Data-Point pair in List
     *  - added to insure that a specific Point can be retrieved
     * \param id_gcp (either id_gcp of the database OR a count whlie loading .points file) to search for and update
     * \return nullptr if the id was not found, otherwise Point of the fiven id
     */
    QgsGeorefDataPoint *getDataPoint( int id_gcp )  { return mGcpList->getDataPoint( id_gcp ); }

    /**
     * Add a Data-Point pair to the List
     * \param dataPoint
     * \return truealways
     */
    bool addDataPoint( QgsGeorefDataPoint *dataPoint ) { return mGcpList->addDataPoint( dataPoint ); }

    /**
     * Update Data-Point pair in List
     *  - added to avoid reloading all the points again
     * \param id_gcp (either id_gcp of the database OR a count whlie loading .points file) to search for and update
     * \param bPointMap (either pixel.point [false] of map.point [true]) to update at give id
     * \param updatePoint the point to use
     * \return true if the id was found, otherwise false
     */
<<<<<<< HEAD
    bool updateDataPoint( int id_gcp, bool bPointMap, QgsPointXY updatePoint ) { return  mGCPList->updateDataPoint( id_gcp, bPointMap, updatePoint ); }
=======
    bool updateDataPoint( int id_gcp, bool bPointMap, QgsPoint updatePoint ) { return  mGcpList->updateDataPoint( id_gcp, bPointMap, updatePoint ); }
>>>>>>> Last major problems resolved. Start test phase

    /**
     * Remove Data-Point pair in List
     *  - added to avoid reloading all the points again
     * \param id_gcp (either id_gcp of the database OR a count whlie loading .points file) to search for and update
     * \return true if the id was found, otherwise false
     */
    bool removeDataPoint( int id_gcp ) { return  mGcpList->removeDataPoint( id_gcp ); }

    /**
     * Informs the caller if any Update, Adding or Delete have been made since the last Update.
     * \return true or false (value of mIsDirty)
     */
    bool isDirty() { return mGcpList->isDirty(); }

    /**
     * Set status if  Update, Adding or Delete have been made, possibly externaly
     * \return true or false (value of mIsDirty)
     */
    bool setDirty() { return mGcpList->setDirty(); }

    /**
     * Informs the caller if any changes have been made since the Initial loading of Point-Pairs.
     * \return true or false (value of mHasChanged)
     */
    bool hasChanged() { return mGcpList->hasChanged(); }
    QgsGcpList *getGcpList() { return mGcpList; }

    /**
     * Informs QgsGcpList that any Updating has been done
     *  - until next Update, Adding or removal: isDirty() = false
     */
    void setClean() { mGcpList->setClean(); };

    /**
     * Informs QgsGcpList (when true) that the Initial loading of Point-Pairs has been completed
     * \param true or false
     */
    void setChanged( bool bChanged ) { mGcpList->setChanged( bChanged ); }

    /**
     * Set mGcpDbData for Spatialite Gcp-Support
     * \param parmsGcpDbData which contains all needed information to use Spatialite Gcp-Support
     * \param bClear clear the list
     */
    void setGcpDbData( QgsSpatiaLiteGcpUtils::GcpDbData *parmsGcpDbData, bool bClear ) { mGcpList->setGcpDbData( parmsGcpDbData, bClear ); };

    /**
     * Return mGcpDbData for Spatialite Gcp-Support
     * \return mGcpDbDatawhich contains all needed information to use Spatialite Gcp-Support
     */
    QgsSpatiaLiteGcpUtils::GcpDbData *getGcpDbData() { return mGcpList->getGcpDbData(); }

    /**
     * Remove all Data-Points
     *  - set to isDirty
     */
    void clearDataPoints() {mGcpList->clearDataPoints(); };

    /**
     * Return amount of DataPoints
     *  - Enabled and Disabled
     */
    int countDataPoints() {return mGcpList->countDataPoints(); };

    /**
     * Return amount of enabled DataPoints
     *  - Enabled only
     */
    int countDataPointsEnabled() {return mGcpList->countDataPointsEnabled(); };

    /**
     * Return amount of disabled DataPoints
     *  - Disabled only
     */
    int countDataPointsDisabled() {return mGcpList->countDataPointsDisabled(); };

    /**
     * Search Point in List
     *  - find a QPoint inside the List
     * \note Goals:
     *  - avoid the adding of Points that exist already
     *  - determine the need to update existing Points that are near the search Point
     * \note result_type:
     *  - 0: not found
     *  - 1: result is exact [the Points are equal]
     *  - 2: result within parameters given [QgsPointXY::compare == true], but not exact
     *  - 3: outside area  [QgsPointXY::compare == false]
     * \param searchPoint QPoint to use to search the List with
     * \param bPointMap (either pixel.point [false] of map.point [true]) to update at give id
     * \param epsilon area around point to search in [1 meter around point=0.5] - as used in QgsPointXY::compare
     * \param id_gcp of nearest
     * \param distance to nearest point
     * \param iResultType to nearest point
     * \see QgsPointXY::compare
     * \return true result is exact, otherwise false
     */
<<<<<<< HEAD
    bool searchDataPoint( QgsPointXY searchPoint, bool bPointMap = true, double epsilon = 0.5, int *id_gcp = nullptr, double *distance = nullptr, int *iResultType = nullptr )  {return mGCPList->searchDataPoint( searchPoint, bPointMap, epsilon, id_gcp, distance, iResultType ); };
    bool avoidUnneededUpdates() {return mGCPList->avoidUnneededUpdates();};
    void setAvoidUnneededUpdates( bool bAvoidUnneededUpdates ) {mGCPList->setAvoidUnneededUpdates( bAvoidUnneededUpdates );};
=======
    bool searchDataPoint( QgsPoint searchPoint, bool bPointMap = true, double epsilon = 0.5, int *id_gcp = nullptr, double *distance = nullptr, int *iResultType = nullptr )  {return mGcpList->searchDataPoint( searchPoint, bPointMap, epsilon, id_gcp, distance, iResultType ); };
    bool avoidUnneededUpdates() {return mGcpList->avoidUnneededUpdates();};
    void setAvoidUnneededUpdates( bool bAvoidUnneededUpdates ) {mGcpList->setAvoidUnneededUpdates( bAvoidUnneededUpdates );};
>>>>>>> Last major problems resolved. Start test phase
  private:
    QgsGcpList         *mGcpList = nullptr;
    QgsGeorefTransform *mGeorefTransform = nullptr;
    bool bTransformUpdated = false;
    bool bMapUnitsPossible = false;
    friend class QgsGcpListWidget; // in order to access createGcpVectors, getPoint, updatePoint and removePoint
};

#endif
