/***************************************************************************
    qgsgcplistmodel.h - Model implementation of GCPList Model/View
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
class QgsGCPList;
class QgsPoint;

class QgsGCPListModel : public QStandardItemModel
{
    Q_OBJECT

  public:
    explicit QgsGCPListModel( QObject *parent = nullptr, int i_LegacyMode=1 );

    void setLegacyMode( int i_LegacyMode );
    void setGCPList( QgsGCPList *theGCPList );
    void setGeorefTransform( QgsGeorefTransform *theGeorefTransform );
    /**
     * Update Modual
     *  - calls createGCPVectors to recreate List of enabled Points
     *  - rebuilds List calculating residual value
     * @see QgsGCPList::updatePoint
     * @see QgsGCPList::removePoint
     * @see QgsGCPList::createGCPVectors
     * @return true if the id was found, otherwise false
     */
    void updateModel();
    bool transformUpdated() { return bTransformUpdated; }
    bool mapUnitsPossible() { return bMapUnitsPossible; }
    /**
     * Calculates root mean squared error for the currently active
     * ground control points and transform method.
     * Note that the RMSE measure is adjusted for the degrees of freedom of the
     * used polynomial transform.
     * @param error out: the mean error
     * @return true in case of success
     */
    bool calculateMeanError( double& error ) const;

  public slots:
    void replaceDataPoint( QgsGeorefDataPoint *newDataPoint, int i );

    void onGCPListModified();
    void onTransformationModified();
   protected:
    /**
     * Builds List of Map/Pixel Point in List that are enabled
     * @param mapCoords to store the Map-points
     * @param pixelCoords to store the Pixel-points
     * @return nullptr if the id was not found, otherwise Point of the fiven id
     */
    void createGCPVectors( QVector<QgsPoint> &mapCoords, QVector<QgsPoint> &pixelCoords ) { return  mGCPList->createGCPVectors(mapCoords,pixelCoords); }
    /**
     * Retrieve Data-Point pair in List
     *  - added to insure that a specific Point can be retrieved
     * @param id (either id_gcp of the database OR a count whlie loading .points file) to search for and update
     * @return nullptr if the id was not found, otherwise Point of the fiven id
     */
    QgsGeorefDataPoint* getDataPoint( int id )  { return mGCPList->getDataPoint(id); }
    /**
     * Add a Data-Point pair to the List
     * @param data_point 
     * @return truealways
     */
    bool addDataPoint(QgsGeorefDataPoint* data_point ) { return mGCPList->addDataPoint(data_point); }
    /**
     * Update Data-Point pair in List
     *  - added to avoid reloading all the points again
     * @param id (either id_gcp of the database OR a count whlie loading .points file) to search for and update
     * @param b_point_map (either pixel.point [false] of map.point [true]) to update at give id
     * @param update_point the point to use
     * @return true if the id was found, otherwise false
     */
    bool updateDataPoint( int id, bool b_point_map, QgsPoint update_point ) { return  mGCPList->updateDataPoint(id, b_point_map,update_point); }
    /**
     * Remove Data-Point pair in List
     *  - added to avoid reloading all the points again
     * @param id (either id_gcp of the database OR a count whlie loading .points file) to search for and update
     * @return true if the id was found, otherwise false
     */
    bool removeDataPoint( int id ) { return  mGCPList->removeDataPoint(id); }
    /**
     * Informs the caller if any Update, Adding or Delete have been made since the last Update.
     * @return true or false (value of mIsDirty)
     */
    bool isDirty() { return mGCPList->isDirty(); }
    /**
     * Informs the caller if any changes have been made since the Initial loading of Point-Pairs.
     * @return true or false (value of mHasChanged)
     */
    bool hasChanged() { return mGCPList->hasChanged(); }
    QgsGCPList* getGCPList() { return mGCPList; }
    /**
     * Informs QgsGCPList that any Updating has been done
     *  - until next Update, Adding or removal: isDirty() = false
     */
    void setClean() { mGCPList->setClean(); };
    /**
     * Informs QgsGCPList (when true) that the Initial loading of Point-Pairs has been compleated
     * @param true or false
     */
    void setChanged(bool b_changed) { mGCPList->setChanged(b_changed); }
    /**
     * Set mGcpDbData for Spatialite Gcp-Support
     * @param parms_GcpDbData which contains all needed information to use Spatialite Gcp-Support
     * @param b_clear clear the list
     */
    void setGcpDbData(QgsSpatiaLiteProviderGcpUtils::GcpDbData* parms_GcpDbData, bool b_clear) { mGCPList->setGcpDbData(parms_GcpDbData,b_clear); };
    /**
     * Return mGcpDbData for Spatialite Gcp-Support
     * @return mGcpDbDatawhich contains all needed information to use Spatialite Gcp-Support
     */
    QgsSpatiaLiteProviderGcpUtils::GcpDbData* getGcpDbData() { return mGCPList->getGcpDbData(); }

  private:
    QgsGCPList         *mGCPList;
    QgsGeorefTransform *mGeorefTransform;
    int mLegacyMode;
    bool bTransformUpdated;
    bool bMapUnitsPossible;
    friend class QgsGCPListWidget; // in order to access createGCPVectors, getPoint, updatePoint and removePoint
};

#endif
