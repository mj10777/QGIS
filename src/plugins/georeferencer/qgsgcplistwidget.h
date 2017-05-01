/***************************************************************************
    qgsgcplistwidget.h - Widget for GCP list display
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
#ifndef QGS_GCP_LIST_WIDGET_H
#define QGS_GCP_LIST_WIDGET_H

#include <QTableView>
#include "qgsgcplistmodel.h"

class QgsDoubleSpinBoxDelegate;
class QgsNonEditableDelegate;
class QgsDmsAndDdDelegate;
class QgsCoordDelegate;

class QgsGCPList;
class QgsGCPListModel;
class QgsGeorefTransform;
class QgsGeorefDataPoint;
class QgsPoint;

/**
  * Class to to list, change and remove point read
  *  - OGR source or the original .points file
  * @note In the older version it seemed to be that:
  *  - is being done to often (for some single changes called multible times)
  *  - often replcing all the point when one was changed, added or removed
  * @note QgsGCPList [as a member of QgsGCPListModel]
  *  - will set itsself as 'isDirty' after:
  * -> Adding, Changing or Removing a Data-Point pair
  *  - mGCPListModel->updateModel
  * -> will now only be called if isDirty() returns true
  * --> avoiding re-calculations when nothing has changed
  * -> when compleated, will call setClean(); [i.e not isDirty]
  * @note
  *  - setChanged(true) should be called after the Initial loading of Point-Pairs
  * -> After any further Adding, Changing or Removing a Data-Point pairs
  * -->  hasChanged() would then return true, even if isDirty() returns false
  * --->  saving to a .points file is then only needed when hasChanged() returns true
  * @note Goal was to avoid 3 copies of the GcpPoint-Data
  *  - 'mPoints', 'mInitialPoints' and the version inside 'mGCPListModel'
  *  - 'QgsResidualPlotItem' will not recieve a pointer to the version inside 'mGCPListModel'
  * --> avoiding, yet again, another copy
  * @note Precision of 10 is now used in QgsStandardItem, avoiding the use of exponential formating in
  * - 'mGCPListModel->updateModel' is now done
  * --> however reverts back to 2 after changes are made
  */
class QgsGCPListWidget : public QTableView
{
    Q_OBJECT
  public:
    explicit QgsGCPListWidget( QWidget *parent = nullptr, int iLegacyMode = 1 );

    void setLegacyMode( int iLegacyMode );
    void setGCPList( QgsGCPList *theGCPList );
    void setGeorefTransform( QgsGeorefTransform *theGeorefTransform );
    QgsGCPList *getGCPList() { return mGCPListModel->getGCPList(); }
    void updateGCPList();
    void closeEditors();

    /**
     * Informs the caller if any Update, Adding or Delete have been made since the last Update.
     * \return true or false (value of mIsDirty)
     */
    bool isDirty() { return mGCPListModel->isDirty(); }

    /**
     * Informs the caller if any changes have been made since the Initial loading of Point-Pairs.
     * \return true or false (value of mHasChanged)
     */
    bool hasChanged() { return mGCPListModel->hasChanged(); }

    /**
     * Informs QgsGCPList (when true) that the Initial loading of Point-Pairs has been compleated
     * \param true or false
     */
    void setChanged( bool bChanged ) { mGCPListModel->setChanged( bChanged ); }
    bool transformUpdated() { return mGCPListModel->transformUpdated(); }
    bool mapUnitsPossible() { return mGCPListModel->mapUnitsPossible(); }

    /**
     * Builds List of Map/Pixel Point in List that are enabled
     * \param mapCoords to store the Map-points
     * \param pixelCoords to store the Pixel-points
     * \return nullptr if the id was not found, otherwise Point of the fiven id
     */
    void createGCPVectors( QVector<QgsPoint> &mapCoords, QVector<QgsPoint> &pixelCoords ) { return  mGCPListModel->createGCPVectors( mapCoords, pixelCoords ); }

    /**
     * Retrieve Data-Point pair in List
     *  - added to insure that a specific Point can be retrieved
     * \param id_gcp (either id_gcp of the database OR a count whlie loading .points file) to search for and update
     * \return nullptr if the id was not found, otherwise Point of the given id
     */
    QgsGeorefDataPoint *getDataPoint( int id_gcp )   { return mGCPListModel->getDataPoint( id_gcp ); }

    /**
     * Add a Data-Point pair to the List
     * \param dataPoint
     * \return truealways
     */
    bool addDataPoint( QgsGeorefDataPoint *dataPoint ) { return mGCPListModel->addDataPoint( dataPoint ); }

    /**
     * Update Data-Point pait in List
     *  - added to avoid reloading all the points again
     * \param id_gcp (either id_gcp of the database OR a count whlie loading .points file) to search for and update
     * \param bPointMap (either pixel.point [false] of map.point [true]) to update at give id
     * \param updatePoint the point to use
     * \return true if the id was found, otherwise false
     */
    bool updateDataPoint( int id_gcp, bool bPointMap, QgsPoint updatePoint )  { return  mGCPListModel->updateDataPoint( id_gcp, bPointMap, updatePoint ); }

    /**
     * Remove Data-Point pair in List
     *  - added to avoid reloading all the points again
     * \param id_gcp (either id_gcp of the database OR a count whlie loading .points file) to search for and update
     * \return true if the id was found, otherwise false
     */
    bool removeDataPoint( int id_gcp )  { return  mGCPListModel->removeDataPoint( id_gcp ); }

    /**
     * Calculates root mean squared error for the currently active
     * ground control points and transform method.
     * Note that the RMSE measure is adjusted for the degrees of freedom of the
     * used polynomial transform.
     * \param error out: the mean error
     * \return true in case of success
     */
    bool calculateMeanError( double &error ) const { return mGCPListModel->calculateMeanError( error ); }

    /**
     * Set mGcpDbData for Spatialite Gcp-Support
     * \param parmsGcpDbData which contains all needed information to use Spatialite Gcp-Support
     * \param bClear clear the list
     */
    void setGcpDbData( QgsSpatiaLiteProviderGcpUtils::GcpDbData *parmsGcpDbData, bool bClear ) { mGCPListModel->setGcpDbData( parmsGcpDbData, bClear ); };

    /**
     * Return mGcpDbData for Spatialite Gcp-Support
     * \return mGcpDbDatawhich contains all needed information to use Spatialite Gcp-Support
     */
    QgsSpatiaLiteProviderGcpUtils::GcpDbData *getGcpDbData() { return mGCPListModel->getGcpDbData(); }

    /**
     * Remove all Data-Points
     *  - set to isDirty
     */
    void clearDataPoints() {mGCPListModel->clearDataPoints(); };

    /**
     * Return amount of DataPoints
     *  - Enabled and Disabled
     */
    int countDataPoints() {return mGCPListModel->countDataPoints(); };

    /**
     * Return amount of enabled DataPoints
     *  - Enabled only
     */
    int countDataPointsEnabled() {return mGCPListModel->countDataPointsEnabled(); };

    /**
     * Return amount of disabled DataPoints
     *  - Disabled only
     */
    int countDataPointsDisabled() {return mGCPListModel->countDataPointsDisabled(); };

    /**
     * Search Point in List
     *  - find a QPoint inside the List
     * \note Goals:
     *  - avoid the adding of Points that exist allready
     *  - determine the need to update existing Points that are near the search Point
     * \note result_type:
     *  - 0: not found
     *  - 1: result is exact [the Points are equal]
     *  - 2: result within parameters given [QgsPoint::compare == true], but not exact
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
    bool searchDataPoint( QgsPoint searchPoint, bool bPointMap = true, double epsilon = 0.5, int *id_gcp = nullptr, double *distance = nullptr, int *iResultType = nullptr )  {return mGCPListModel->searchDataPoint( searchPoint, bPointMap, epsilon, id_gcp, distance, iResultType ); };
    bool avoidUnneededUpdates() {return mGCPListModel->avoidUnneededUpdates();};
    void setAvoidUnneededUpdates( bool bAvoidUnneededUpdates ) {mGCPListModel->setAvoidUnneededUpdates( bAvoidUnneededUpdates );};
  public slots:
    // This slot is called by the list view if an item is double-clicked
    void itemDoubleClicked( QModelIndex index );
    void itemClicked( QModelIndex index );

  signals:
    void jumpToGCP( uint theGCPIndex );
    void replaceDataPoint( QgsGeorefDataPoint *pnt, int row );
    void pointEnabled( QgsGeorefDataPoint *pnt, int i );
    void deleteDataPoint( int index );

  private slots:
    void updateItemCoords( QWidget *editor );
    void showContextMenu( QPoint );
    void removeRow();
    void editCell();
    void jumpToPoint();

  private:
    void createActions();
    void createItemContextMenu();
    void adjustTableContent();

    QgsGCPListModel          *mGCPListModel = nullptr;

    QgsNonEditableDelegate   *mNonEditableDelegate = nullptr;
    QgsDmsAndDdDelegate      *mDmsAndDdDelegate = nullptr;
    QgsCoordDelegate         *mCoordDelegate = nullptr;

    int mPrevRow = 0;
    int mPrevColumn = 0;
    int mLegacyMode = 0;
};

#endif
