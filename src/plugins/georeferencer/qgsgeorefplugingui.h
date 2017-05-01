/***************************************************************************
 *   Copyright (C) 2003 by Tim Sutton                                      *
 *   tim@linfiniti.com                                                     *
 *                                                                         *
 *   This is a plugin generated from the QGIS plugin template              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef QGSGEOREFPLUGINGUI_H
#define QGSGEOREFPLUGINGUI_H

#include "ui_qgsgeorefpluginguibase.h"
#include "qgsgeoreftransform.h"

#include "qgsgcplistwidget.h"
#include "qgsmapcoordsdialog.h"
#include "qgsimagewarper.h"
#include "qgscoordinatereferencesystem.h"
#include "layertree/qgslayertreegroup.h"
#include "layertree/qgslayertreelayer.h"
#include "layertree/qgslayertreeview.h"
#include "qgsvectorlayereditbuffer.h"
#include "qgsspatialiteprovidergcputils.h"
#include "qgsgcpcoverages.h"

#include <QPointer>

class QAction;
class QActionGroup;
class QIcon;
class QPlainTextEdit;
class QLabel;

class QgisInterface;
class QgsGeorefDataPoint;
class QgsGCPListWidget;
class QgsMapTool;
class QgsMapCanvas;
class QgsMapCoordsDialog;
class QgsPoint;
class QgsRasterLayer;
class QgsRectangle;
class QgsMessageBar;
// mj10777
class QgsVectorLayer;
class QgsVectorLayerEditBuffer;
class QgsLayerTreeGroup;
class QgsLayerTreeLayer;
class QgsLayerTreeView;
class QgsSpatiaLiteProviderGcpUtils;

class QgsGeorefDockWidget : public QgsDockWidget
{
    Q_OBJECT
  public:
    QgsGeorefDockWidget( const QString &title, QWidget *parent = nullptr, Qt::WindowFlags flags = nullptr );
};

class QgsGeorefPluginGui : public QMainWindow, private Ui::QgsGeorefPluginGuiBase
{
    Q_OBJECT

  public:
    QgsGeorefPluginGui( QgisInterface *qgisInterface, QWidget *parent = nullptr, Qt::WindowFlags fl = nullptr );
    ~QgsGeorefPluginGui();

  protected:

    /**
     * closeEvent
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void closeEvent( QCloseEvent * ) override;

  private slots:
    // file

    /**
     * reset
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void reset();

    /**
     * Reacts to mActionOpenRaster
     *  - portion of former 'openRaster'
     *  - reads settings for last raster Directory and file filter
     *  - calls QFileDialog::getOpenFileName
     *  -> calls 'openRaster' with QFileInfo raster_file
     * \see openRaster
     */
    void openRasterDialog();

    /**
     * doGeoreference
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void doGeoreference();

    /**
     * generateGDALScript
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void generateGDALScript();

    /**
     * generateMercatorScript
     *  - generates a script base on POLYGONs found in 'create_mercator_polygon'
     * Script will create images out of the source image based on the cutline
     *  - Goal is to cutout portions of Maps with folds into single images without the Fold portions
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     */
    QString generateGDALMercatorScript();

    /**
     * getTransformSettings
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    bool getTransformSettings();

    // edit

    /**
     * setAddPointTool
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void setAddPointTool();

    /**
     * setNodePointTool
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void setNodePointTool();

    /**
     * setMovePointTool
     * \since QGIS 3.0
     * Not needed, used only in ( mLegacyMode == 0 )
     *  - therefore is not needed
     */
    void setMovePointTool();

    /**
     * setSpatialiteGcpUsage
     *  - turn Spatialite Gcp Logic on/off
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void setSpatialiteGcpUsage();

    /**
     * Fetch from the GcpMaster Database, Gcp-Point within the present MapExtent
     *  - assumes Active GcpMaster and Coverge
     *  -> also using Spatialite Gcp-Logic
     * \note
     *  - will query QGis-Map-Extent
     *  -- search for Gcp-Points inside that extent that are NOT contained in the Coverage-Gcps
     *  --> add these Points using Spatialite Gcp-Logic, when NOT turned off or if NOT available
     * \note
     * Gcp-Master Point correctoons of existing Gcp-Points
     *  - when a existing Gcp-Points is within the 'mGcpMasterArea' Circle
     *  -> the existing Gcp-Map point will be replaced with the Gcp-Master Point
     *  --> this functionality is available, when the Spatialite Gcp-Logic has been NOT turned off or if NOT available
     *  --> this combination also allows for the corrections of Gcp-Points WITHOUT the fetching of other Gcp-Master Points
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     */
    void fetchGcpMasterPointsExtent( );

    // view
    void setZoomInTool();

    /**
     * setZoomOutTool
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void setZoomOutTool();

    /**
     * zoomToLayerTool
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void zoomToLayerTool();

    /**
     * zoomToLast
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void zoomToLast();

    /**
     * zoomToNext
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void zoomToNext();

    /**
     * setPanTool
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void setPanTool();

    /**
     * linkGeorefToQGis
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void linkGeorefToQGis( bool link );

    /**
     * linkQGisToGeoref
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void linkQGisToGeoref( bool link );

    /**
     * zoomMercatorPolygonExtent
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void zoomMercatorPolygonExtent();

    /**
     * Changes the Selection of a Layer
     *  - MapTools will then react to these Layers
     * \note
     * QgsMapCanvasLayer displayed in Georeferencer
     *  - needed to store the visibility status of the Layers
     *  Contains 3 Layers
     *  - mLayerGcpPixels [pixel gcp]
     *  - mLayerMercatorPolygons [possible cutlines for raster ]
     *  - mCanvas [raster being shown]
     *  -> must be last of list
     * \note
     *  - only the first 2 are contained in 'mTreeGroupCutlineMercator'
     *  -> to be turned on/off as needed
    * @see mMapCanvasLayers
     */
    void activeLayerTreeViewChanged( QgsMapLayer *layer );

    /**
     * Changes the Visablity of a Layer
     * \note
     * QgsMapCanvasLayer displayed in Georeferencer
     *  - needed to store the visibility status of the Layers
     *  Contains 3 Layers
     *  - mLayerGcpPixels [pixel gcp]
     *  - mLayerMercatorPolygons [possible cutlines for raster ]
     *  - mCanvas [raster being shown]
     *  -> must be last of list
     * \note
     *  - only the first 2 are contained in 'mTreeGroupCutlineMercator'
     *  -> to be turned on/off as needed
    * @see mMapCanvasLayers
     */
    void visibilityLayerTreeViewChanged( QgsLayerTreeNode *layerTreenode, Qt::CheckState check_state );
    // gcps

    /**
     * Adding a Point
     *  - reaction to emit pointAdded
     * \since QGIS 3.0
     *  - the 'if (mLegacyMode == 1)' portion is needed
     *  -> when the Spatialite-Gcp-Logic is not active
     *  --> not enough Points or Spatialite is not compiled with Gcp-Logic
     * \since QGIS 3.0
     *  - the 'if (mLegacyMode == 0)' portion can be removed
     * \see showCoordDialog
     * \see featureAdded_gcp
     */
    void addPoint( const QgsPoint &pixelCoords, const QgsPoint &mapCoords, int id_gcp = -1, bool b_PixelMap = true,
                   bool enable = true, bool finalize = true );

    /**
     * Delete Data-Point
     * \since QGIS 3.0
     * Only when ( mLegacyMode == 0 )
     *  - therefore not needed if mLegacyMode is not supported
     * \param QPoint
     */
    void deleteDataPoint( QPoint pixelCoords );

    /**
     * Delete Data-Point
     * \since QGIS 3.0
     * Only when ( mLegacyMode == 0 )
     *  - therefore not needed if mLegacyMode is not supported
     * \param id_gcp
     */
    void deleteDataPoint( int id_gcp );

    /**
     * Show CoordDialog
     * \since QGIS 3.0
     * Only when ( mLegacyMode == 0 )
     *  - therefore not needed if mLegacyMode is not supported
     * \param QgsPoint
     */
    void showCoordDialog( const QgsPoint &pixelCoords );

    /**
     * Select Data-Point
     * \since QGIS 3.0
     * Only when ( mLegacyMode == 0 )
     *  - therefore not needed if mLegacyMode is not supported
     * \param QPoint
     */
    void selectPoint( QPoint coords );

    /**
     * Move Data-Point
     * \since QGIS 3.0
     * Only when ( mLegacyMode == 0 )
     *  - therefore not needed if mLegacyMode is not supported
     * \param QPoint
     */
    void movePoint( QPoint coords );

    /**
     * Move Data-Point
     * \since QGIS 3.0
     * Only when ( mLegacyMode == 0 )
     *  - therefore not needed if mLegacyMode is not supported
     * \param QPoint
     */
    void releasePoint( QPoint coords );

    /**
     * Reacts to a Spatialite-Database storing the Gcp-Points [gcp_point, gcp_pixel]
     *  - the TRIGGER will create an empty POINT (0,0) if not set
     *  -- which is what happens here
     * \note both geometries are in one TABLE
     *  an INSERT can therefor only be done once, the other must be UPDATEd if 'SELECT HasGCP()' returns true
     *  and enough gcp-points exist to make a calculation
     *  For this the primary key is needed for the INSERTed record, so that the POINT in the other Layer can be retrieved and UPDATEd
     * - this causes the Event to called during 'commitChanges' (the used 'fid' will be stored in 'mEventGcpStatus')
     *  -- after 'commitChanges' has compleated, the value in 'mEventGcpStatus' will be used to retrive the inserted primary-key
     * -- the record from the other gcp-layer will be retrieved with the primary-key and UPDATEd
     * \note 'mEventPointStatus' will be set to 1 (for 'Map-Point to Pixel-Point') and 2  (for 'Pixel-Point to Map-Point')
     * - this value will be read in the featureDeleted_gcp and geometryChanged_gcp and will return if > 0
     * \note the added geometry is added immediately to the Database
     * - which will also save any new position that were not commited after 'geometryChanged_gcp'
     * \see getGcpConvert
     * \see jumpToGcpConvert
     * \see saveEditsGcp
     * \param fid (a minus number when inserting to the database, a positive number when adding to a layer)
     */
    void featureAdded_gcp( QgsFeatureId fid );

    /**
     * Reacts to a Spatialite-Database storing the Gcp-Points [gcp_point, gcp_pixel]
     *  - any change in the position will only effect the calling gcp-layer
     *  -- any activity should then only be done once
     * \note all 'fid' < 0 will be ignored
     *  - the changed position in NOT saved to the database
     * \see saveEditsGcp
     * \param fid (a minus number when inserting to the database, a positive number when adding to a layer)
     * \param changed_geometry the changed geometry value must be updated in the gcp-list
     */
    void geometryChanged_gcp( QgsFeatureId fid, QgsGeometry &changed_geometry );

    /**
     * Reacts to a Spatialite-Database storing a cutline [cutline_points, linestrings, polygons and mecator_polygons]
     *  - save edit after each change
     *  -- so that the geometries can bee seen in the Canvos while moving around
     * \note some default values will be stored
     *  - id_gcp_coverage of the active coverage in georeferencer
     *  - belongs_to_01 the name of the active coverage in georeferencer
     *  - belongs_to_02 the raster_name of the active coverage in georeferencer
     * \see saveEditsGcp
     * \param fid (a minus number when inserting to the database, a positive number when adding to a layer)
     */
    void featureAdded_cutline( QgsFeatureId fid );

    /**
     * Reacts to a Spatialite-Database storing a cutline [cutline_points, linestrings, polygons and mecator_polygons]
     *  - save edit after each change
     *  -- so that the geometries can bee seen in the Canvos while moving around
     * \see saveEditsGcp
     * \param fis (a minus number when inserting to the database, a positive number when adding to a layer)
     * \param changed_geometry the changed geometry value must be updated in the gcp-list
     */
    void geometryChanged_cutline( QgsFeatureId fid, QgsGeometry &changed_geometry );

    /**
     * loadGCPsDialog
     * \since QGIS 3.0
     * Only when ( mLegacyMode == 0 )
     *  - therefore not needed if mLegacyMode is not supported
     */
    void loadGCPsDialog();

    /**
     * saveGCPsDialog(
     * \since QGIS 3.0
     * Only when ( mLegacyMode == 0 )
     *  - therefore not needed if mLegacyMode is not supported
     */
    void saveGCPsDialog();

    // settings

    /**
     * showRasterPropertiesDialog
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void showRasterPropertiesDialog();

    /**
     * showMercatorPolygonPropertiesDialog
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void showMercatorPolygonPropertiesDialog();

    /**
     * createGcpDatabaseDialog
     *  - create an empty Gcp-Database
     *  -> option will be offered to create a Sql-Dump of the created Database
     * \note
     *  - 'gcp_coverage'
     *  -> should have the 'gcp.db' suffix/file-extention to be recognised by this application
     * \note
     *  - 'gcp_master'
     *  -> should have the 'db' suffix/file-extention to be recognised by this application
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \see QgsGcpDatabaseDialog::accept
     * \see createGcpCoverageDb
     * \see createGcpMasterDb
     */
    void createGcpDatabaseDialog();

    /**
     * createSqlDumpGcpCoverage
     *  -> Create a Sql-Dump of the active Gcp-Coverages Database
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     * \see createGcpCoverageDb
     */
    void createSqlDumpGcpCoverage();

    /**
     * createSqlDumpGcpMaster
     *  -> Create a Sql-Dump of the active Gcp-Master Database
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     * \see createGcpMasterDb
     */
    void createSqlDumpGcpMaster();

    /**
     * showGeorefConfigDialog
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void showGeorefConfigDialog();
    // GcpDatabase

    /**
     * setLegacyMode
     * \since QGIS 3.0
     * Only when ( mLegacyMode == 0 )
     *  - therefore not needed if mLegacyMode is not supported
     */
    void setLegacyMode();

    /**
     * setLegacyMode
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void setPointsPolygon();

    /**
     * listGcpCoverages
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void listGcpCoverages();
    // Load selected coverage, when not alread loaded

    /**
     * Result of QgsGcpCoveragesDialog
     *  - selection of Gcp-Coverage
     *  -> calls 'openRaster' with QFileInfo raster_file
     * \see openRaster
     * \param id_selected_coverage id of coverage from GcpDb
     */
    void loadGcpCoverage( int id_selected_coverage );

    /**
     * Reaction to result of 'loadGcpCoverage' or 'openRasterdialog'
     *  - major portion of former 'openRaster' (without OpenFile-Dialog)
     *  - save GCP when needed
     *  - calls QgsRasterLayer::isValidRasterFileName
     *  - stores in Settings: raster_file path and used filter
     *  - sets GeorefTransform settings
     *  - clears GCP when needed
     *  - remove old Layer when needed
     *  - prepairs auxiliary file names (.point etc)
     *  - calls addRaster
     *  - set Canvas extents and QgsMapTool settings
     * \since QGIS 3.0
     *  - the 'if (mLegacyMode == 0)' portion can be removed
     * \see openRasterDialog
     * \see loadGcpCoverage
     * \see addRaster
     */
    void openRaster( QFileInfo raster_file );

    /**
     * openGcpCoveragesDb
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void openGcpCoveragesDb();

    /**
     * openGcpCMasterDb
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void openGcpMasterDb();

    // plugin info

    /**
     * contextHelp
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void contextHelp();

    // comfort

    /**
     * jumpToGCP
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void jumpToGCP( uint theGCPIndex );

    /**
     * extentsChangedGeorefCanvas
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void extentsChangedGeorefCanvas();

    /**
     * extentsChangedQGisCanvas
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void extentsChangedQGisCanvas();
    void onQGisCanvasCrsC2R();

    // canvas info

    /**
     * showMouseCoords
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void showMouseCoords( const QgsPoint &pt );

    /**
     * updateMouseCoordinatePrecision
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void updateMouseCoordinatePrecision();

    // Histogram stretch

    /**
     * localHistogramStretch
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void localHistogramStretch();

    /**
     * fullHistogramStretch
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void fullHistogramStretch();

    // when one Layer is removed

    /**
     * layerWillBeRemoved
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void layerWillBeRemoved( const QString &layerId );

    /**
     * extentsChanged
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void extentsChanged(); // Use for need add again Raster (case above)

    /**
     * Update Modell
     *  - calls createGCPVectors to recreate List of enabled Points
     *  - rebuilds List calculating residual value
     * \note this is also done in updateModel
     *  - then calls updateTransformParamLabel to set summery of results
     * \see updateTransformParamLabel
     * \see QgsGCPListWidget::updateModel
     * \return true if the id was found, otherwise false
     */
    bool updateGeorefTransform();

    /**
     * updateIconTheme
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void updateIconTheme( const QString &theme );

  private:

    /**
     * SaveGCPs
     * \since QGIS 3.0
     * Possibly not needed for ( mLegacyMode == 1 )
     *  - to be determined
     *  - TODO: document
     */
    enum SaveGCPs
    {
      GCPSAVE,
      GCPSILENTSAVE,
      GCPDISCARD,
      GCPCANCEL
    };

    // gui

    /**
     * createActions
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void createActions();

    /**
     * createActionGroups
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void createActionGroups();

    /**
     * createMapCanvas
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void createMapCanvas();

    /**
     * createMenus
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void createMenus();

    /**
     * createDockWidgets
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void createDockWidgets();

    /**
     * createBaseLabelStatus
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    QLabel *createBaseLabelStatus();

    /**
     * createStatusBar
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void createStatusBar();

    /**
     * setupConnections
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void setupConnections();

    /**
     * removeOldLayer
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void removeOldLayer();

    // Mapcanvas Plugin

    /**
     * Adding of selected Raster to Georeference Canvas
     *  - creates QgsRasterLayer
     *  -> calls 'loadGCPs' with QFileInfo raster_file
     *  - adds created QgsRasterLayer to Canvas
     * \see openRaster
     * \see extentsChanged
     * \see loadGCPs
     * \param file name as string
     */
    void addRaster( const QString &file );

    /**
     * loadGTifInQgis
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void loadGTifInQgis( const QString &gtif_file );

    // settings

    /**
     * readSettings
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void readSettings();

    /**
     * writeSettings
     * \since QGIS 3.0
     * Common for both  ( mLegacyMode == 0+1 )
     *  - therefore is needed
     *  - TODO: document
     */
    void writeSettings();

    // gcp points

    /**
     * Loading of Gcp-Points
     * - clears Gcp-Points when needed
     * - calls createGcpDb
     * -> will return false when 'mLegacyMode == 0'
     * \note
     *  - LegacyMode
     *  -> when a Gcp-Database has been read (createGcpDb returns true)
     *  --> Gcp-Points will read from the OGR-Datasource
     *  -> when no Gcp-Database was found
     *  --> Gcp-Points will read from the '.points' file
     * - both methods will:
     * -> add the QgsGeorefDataPoint
     * - calls 'setGCPList' AFTER all points have been added
     * \since QGIS 3.0
     *  - the 'if (mLegacyMode == 0)' portion can be removed
     *  -> reading of the .points file
     *  --> the Gcp-Database will still read an existing '.points' file upon creation if the gcp_points TABLE does not exist
     * \see openRaster
     * \see extentsChanged
     * \see createGcpDb
     * \see QgsGCPListWidget::setGCPList
     * \see QgsSpatiaLiteProviderGcpUtils::createGcpDb
     * \param file name as string
     */
    bool loadGCPs( /*bool verbose = true*/ );

    /**
     * saveGCPs
     * \since QGIS 3.0
     *  - the 'if (mLegacyMode == 1)' portion is needed
     *  -> update extent of Database
     * \since QGIS 3.0
     *  - the 'if (mLegacyMode == 0)' portion can be removed
     *  -> .points file is save when needed
     */
    void saveGCPs();

    /**
     * SaveGCPs and checkNeedGCPSave
     *  ->  ?? weird construction ??
     * \since QGIS 3.0
     *  - for 'if (mLegacyMode == 1)'
     *  -> not really needed
     * \since QGIS 3.0
     *  - the 'if (mLegacyMode == 0)' portion can be removed
     *  -> based on enum SaveGCPs how to save ,points file
     * \see SaveGCPs
     */
    QgsGeorefPluginGui::SaveGCPs checkNeedGCPSave();

    // mj10777

    /**
     * mGcpSrid
     *  - srid of Gcp (to be stored or retrieved from Gcp-Database
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     */
    int mGcpSrid;

    /**
     * Srid of read MasterDB
     *  - srid of Gcp (to be stored or retrieved from Gcp-Database
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     */
    int mGcpMasterSrid;

    /**
     * Circle around MasterDB Gcp-Point
     *  - default: 5.0 (2.5 Map-Units [meters] around (left/right/top/bottom of point)
     * \note
     *  - Assumption
     *  -> the Gcp-Master Point is a precise position that is valid for all maps
     *  -> checking is done (from older projects) during fetchGcpMasterPointsExtent
     *  --> if existing Gcp-Points exist within the given  Circle
     *  ---> if found, the former Gcp-Point will be replaced with the Gcp-Master Point
     *  ----> the Pixel-Value will NOT be changed
     * \note
     *  The User can change this value, which will be retrieved before being used in fetchGcpMasterPointsExtent
     *  - a (possibly changed) value is NOT stored in the QSettings
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \see setCutlineGcpMasterArea
     * \see fetchGcpMasterPointsExtent
     */
    double mGcpMasterArea;

    /**
     * Circle around MasterDB Gcp-Point
     *  - default: 2.0 (1.0 Map-Units [meters] around (left/right/top/bottom of point)
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \see setCutlineGcpMasterArea
     * \see fetchGcpMasterPointsExtent
     */
    double mGcpPointArea;

    /**
     * Map-Point Unit based on 1 meter
     *  - 1.0 meters Map-Unit based on srid being used in QGIS
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \see setCutlineGcpMasterArea
     * \see fetchGcpMasterPointsExtent
     */
    double mMapPointUnit;

    /**
     * Project projection being used
     *  - from mIface->mapCanvas()->mapSettings().destinationCrs()
     * \since QGIS 3.0
     */
    QString mGcpAuthid;

    /**
     * Project projection being used
     *  - from mIface->mapCanvas()->mapSettings().destinationCrs()
     * \since QGIS 3.0
     */
    QString mGcpDescription;

    /**
     * jumpToGcpConvert
     *  - After adding a new Point to canvas_X
     *  -> move canvas_Y to the point created by the Spatialite-Gcp-logic
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     */
    void jumpToGcpConvert( QgsPoint inputPoint, bool bToPixel = false );

    /**
     * Translate QgsGeorefTransform::TransformParametrisation numbering to Spatialite numbering
     * use when calling getGcpConvert
     * \see getGcpConvert
     * Note only ThinPlateSpline, PolynomialOrder1, PolynomialOrder2, PolynomialOrder3 and  are supported
     * \param mTransformParam: as used in project
     * \return 0-3, otherwise 0 for ThinPlateSpline
     */
    int getGcpTransformParam( QgsGeorefTransform::TransformParametrisation iTransformParam );

    /**
     * Translate QgsGeorefTransform::TransformParametrisation numbering to Spatialite numbering
     * use when calling getGcpConvert
     * \see getGcpConvert
     * Note only ThinPlateSpline, PolynomialOrder1, PolynomialOrder2, PolynomialOrder3 and  are supported
     * \param mTransformParam: as used in project
     * \return 0-3, otherwise 0 for ThinPlateSpline
     */
    QgsGeorefTransform::TransformParametrisation setGcpTransformParam( int iTransformParam );

    /**
     * Translate to and from a readable form for the gcp_points Database
     * - GRA_NearestNeighbour, GRA_Bilinear, GRA_Cubic, GRA_CubicSpline and GRA_Lanczos are supported
     * \see getGcpConvert
     * Note only ThinPlateSpline, PolynomialOrder1, PolynomialOrder2, PolynomialOrder3 and  are supported
     * \param mResamplingMethod: as used in project
     * \return  above, otherwise 'Lanczos' for GRA_Lanczos
     */
    QString getGcpResamplingMethod( QgsImageWarper::ResamplingMethod i_ResamplingMethod );

    /**
     * Translate to and from a readable form for the gcp_points Database
     * - GRA_NearestNeighbour, GRA_Bilinear, GRA_Cubic, GRA_CubicSpline and GRA_Lanczos are supported
     * \param sResamplingMethod: as used in project
     * \return above, otherwise 'Lanczos' for GRA_Lanczos
     */
    QgsImageWarper::ResamplingMethod setGcpResamplingMethod( QString sResamplingMethod );

    /**
     * mSpatialite_gcp_enabled
     *  - Has the Spatialite being used been compiled with the Gcp-Logic
     *  -> if not the Legacy QgsMapCoordsDialog will be called
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \see SaveGCPs
     */
    bool mSpatialite_gcp_enabled;

    /**
     * isGcpEnabled
     *  - Has the Spatialite being used been compiled with the Gcp-Logic
     *  -> if not the Legacy QgsMapCoordsDialog will be called
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \see mSpatialite_gcp_enabled
     * \return  mSpatialite_gcp_enabled
     */
    bool isGcpEnabled() { return mSpatialite_gcp_enabled; }

    /**
     * isGcpOff
     *  - Has the User turned off the Spatialite the Gcp-Logic ?
     *  -> if yes the Legacy QgsMapCoordsDialog will be called
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \see mSpatialite_gcp_enabled
     * \return  mSpatialite_gcp_enabled
     */
    bool isGcpOff() { if ( isGcpEnabled() ) return mSpatialiteGcpOff; else return false; }
    // mj10777: add gui logic for this and store in setting
    bool mGdalScriptOrGcpList;
    // mj10777: add gui logic for this and store in setting
    bool mPointOrSpatialiteGcp;
    // mj10777: Prevent usage of Spatialite-Gcp logic when desired
    bool mSpatialiteGcpOff;
    QString mError;
    QgsGeorefTransform mTransformType;
    QString mGcpLabelExpression;
    int mGcp_label_type;
    int mLegacyMode;
    int mPointsPolygon;
    double fontPointSize;
    bool exportLayerDefinition( QgsLayerTreeGroup *group_layer, QString file_path = QString::null );

    // georeference
    bool georeference();
    bool writeWorldFile( const QgsPoint &origin, double pixelXSize, double pixelYSize, double rotation );
    bool writePDFReportFile( const QString &fileName, const QgsGeorefTransform &transform );
    bool writePDFMapFile( const QString &fileName, const QgsGeorefTransform &transform );
    void updateTransformParamLabel();

    // gdal script
    void showGDALScript( const QStringList &commands );
    QString generateGDALtranslateCommand( bool generateTFW = true );
    /* Generate command-line for gdalwarp based on current GCPs and given parameters.
     * For values in the range 1 to 3, the parameter "order" prescribes the degree of the interpolating polynomials to use,
     * a value of -1 indicates that thin plate spline interpolation should be used for warping.*/
    QString generateGDALwarpCommand( const QString &resampling, const QString &compress, bool useZeroForTrans, int order,
                                     double targetResX, double targetResY );

    // mj10777

    /**
     * generateGcpList
     *  - for gdal_translate or as seperate list
     * \note
     * Based on settings in QgsGeorefConfigDialog
     *  - mGdalScriptOrGcpList will be set
     *  - false: Full Gdal-Script will be returned
     *  - true : Only Gcp-List will be returned
     * \see generateGDALtranslateCommand
     * \see generateGDALScript()
     * \return  QString with Gcp-List
     */
    QString generateGDALGcpList();

    /**
     * generateGDALTifftags
     *  - for gdal_translate or gdalwarp
     * \note
     * Following TIFFTAGS are supported
     *  - TIFFTAG_DOCUMENTNAME [gcp_coverages.title]
     *  - TIFFTAG_IMAGEDESCRIPTION [gcp_coverages.abstract]
     *  - TIFFTAG_COPYRIGHT [gcp_coverages.copyright]
     *  - TIFFTAG_DATETIME [gcp_coverages.map_date]
     *  - TIFFTAG_ARTIST
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \param i_gdal_command : 0=gdal_translate, 1=gdalwarp
     * \see generateGDALtranslateCommand
     * \see generateGDALwarpCommand
     * \return  QString with -to or -mo parameters
     */
    QString generateGDALTifftags( int i_gdal_command );

    /**
     * generateGDALNodataCutlineParms
     *  - for gdalwarp
     * \note
     * Nodata value stored in [gcp_coverages.nodata]
     *  - TODO: add Diealog to set value
     * Cutline, stored in create_cutline_polygon
     *  - The last found id_cutline that fullfills the following conditions
     *  -- id_gcp_coverage of the active coverage
     *  -- cutline_type=77
     *  - will create a Statement that looks like this:
     *  -> -crop_to_cutline -cutline /abosult_directory/Berlin_Straube.maps.gcp.db -csql "SELECT cutline_polygon FROM create_cutline_polygons WHERE (id_cutline = 20);"
     *  - The id_cutline will be searched for and set when the gcp_coverages is updated
     * \see generateGDALScript()
     * \see QgsSpatiaLiteProviderGcpUtils::updateGcpDb
     * \return  QString with Gdal-Parameters
     */
    QString generateGDALNodataCutlineParms();

    // utils
    bool checkReadyGeoref();
    QgsRectangle transformViewportBoundingBox( const QgsRectangle &canvasExtent, QgsGeorefTransform &t,
        bool rasterToWorld = true, uint numSamples = 4 );
    QString convertTransformEnumToString( QgsGeorefTransform::TransformParametrisation transform );
    QString convertResamplingEnumToString( QgsImageWarper::ResamplingMethod resampling );
    int polynomialOrder( QgsGeorefTransform::TransformParametrisation transform );
    QString guessWorldFileName( const QString &rasterFileName );
    QIcon getThemeIcon( const QString &name );
    bool checkFileExisting( const QString &fileName, const QString &title, const QString &question );
    bool equalGCPlists( const QgsGCPList *list1, const QgsGCPList *list2 );
    void logTransformOptions();
    void logRequaredGCPs();

    /**
     * Removal of Gcp-Layers
     * - this function will be called:
     * -> when a new Raster is loaded
     * -> when the Georeferencer ends
     * \note
     * - the order of removal is important
     * -> disconnect SLOT/SIGNAL
     * -> save Layer-Data when needed
     * -> remove layer from group
     * -> remove layer from MapLayerRegistry
     * -> set layer-pointer to nullptr
     * \note Display in QGIS-Cavas: mTreeGroupGeoreferencer
     * - mTreeGroupGcpCutlines
     * -> mLayerCutlinePoints/linestrings/polygons
     * - mTreeGroupGcpPoints
     * -> mLayerGcpPoints
     * - mTreeGroupGtifRasters
     * -> mLayerGtifRaster
     * - mTreeGroupGcpMaster
     * -> mLayerGcpMaster
     * \note Display in Georeferencer-Canvas: mTreeGroupCutlineMercator
     * -> mLayerGcpPixels
     * -> mLayerMercatorPolygons
     * \note GCPListWidget
     * - Data-Points will be cleared
     * - Georeferencer-Canvas will be removed and refreshed
     */
    void clearGCPData();

    /**
     * Gcp-Layers: Gcp-Points
     *  - Settiing of Labels and Symbols:
     *  -> which will be loaded in the corresponding QgsLayerTreeGroup
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \see loadGCPs
     * \return  true íf usable result have been made
     */
    bool setGcpLayerSettings( QgsVectorLayer *layer_gcp );

    /**
     * Gcp-Layers: Gcp-Master and Cutline
     *  - Settiing of Labels and Symbols:
     *  -> which will be loaded in the corresponding QgsLayerTreeGroup
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \see loadGCPs
     * \see loadGcpMaster
     * \return  true íf usable result have been made
     */
    bool setCutlineLayerSettings( QgsVectorLayer *layer_cutline );

    /**
     * Retrieve value used for Circle around MasterDB Gcp-Point
     *  - default: 5.0 (2.5 meter around (lef/right/top/bottom point)
     * \note
     * The User may change this value befor calling 'fetchGcpMasterPointsExtent'
     *  - therefore the, possibly changed value, must be retrieved befor first usage
     * \see mGcpMasterArea
     * \see fetchGcpMasterPointsExtent
     */
    double getCutlineGcpMasterArea( QgsVectorLayer *layer_cutline = nullptr );

    /**
     * Set value used for Circle around MasterDB Gcp-Point
     *  - default: 5.0 (2.5 meter around (lef/right/top/bottom point)
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \see mGcpMasterArea
     */
    bool setCutlineGcpMasterArea( double d_area = 2.0, QgsVectorLayer *layer_cutline = nullptr );

    /**
     * createSvgColors
     * \param i_method  filling logic to use
     * \param b_reverse  use Dark to Light instead of Light to Dark
     * \param b_grey_black  include Color-Groups White, Grey and Black
     * \returns  list_SvgColors QStringList list containing distinct list of svg color names for use with QColor
     * \note
     *
     *  SVG color keyword names, of which there are 146 (138 unique colors)
     * <a href="https://www.w3.org/TR/SVG/types.html#ColorKeywords </a>
     *
     * X11 color names, listed by color groups, of which there are 11
     * <a href="https://en.wikipedia.org/wiki/Web_colors#Css_colors">Colors listed by Color-Groups </a>
     * \note
     * Gray/Grey are interchnagable (7 colors) 'grey' will be used
     * Alternative names: 2 colors) 'aqua / cyan' and 'magenta / fuchsia',   cyan and magenta will be used
     *
     * Used to create QgsCategorizedSymbolRendererV2 Symbols for Polygons
     * \see setCutlineLayerSettings
     */
    QStringList createSvgColors( int i_method = 0, bool b_reverse = false, bool b_grey_black = false );

    /**
     * Calculates root mean squared error for the currently active
     * ground control points and transform method.
     * Note that the RMSE measure is adjusted for the degrees of freedom of the
     * used polynomial transform.
     * \param error out: the mean error
     * \return true in case of success
     */
    bool calculateMeanError( double &error ) const { return mGCPListWidget->calculateMeanError( error ); }

    //! Docks / undocks this window
    void dockThisWindow( bool dock );

    QGridLayout *mCentralLayout = nullptr;

    QgsMessageBar *mMessageBar = nullptr;
    QMenu *mPanelMenu = nullptr;
    QMenu *mToolbarMenu = nullptr;

    QAction *mActionHelp = nullptr;

    QgsGCPListWidget *mGCPListWidget = nullptr;
    QLineEdit *mScaleEdit = nullptr;
    QLabel *mScaleLabel = nullptr;
    QLabel *mCoordsLabel = nullptr;
    QLabel *mTransformParamLabel = nullptr;
    QLabel *mEPSG = nullptr;
    unsigned int mMousePrecisionDecimalPlaces;

    QString mRasterFilePath; // Path to the file
    QString mRasterFileName; // Absolute-FilePath of input raster
    QString mModifiedRasterFileName; // Output geo-tiff
    QString mWorldFileName;
    QString mTranslatedRasterFileName; // Input File-Name in temp-dir
    QString mGcpPointsFileName;
    QgsCoordinateReferenceSystem mProjection;
    QString mPdfOutputFile;
    QString mPdfOutputMapFile;
    double  mUserResX, mUserResY;  // User specified target scale

    // mj10777
    bool bGcpFileName; // Save GCP-Text to a text-file
    bool bPointsFileName; // Save old Points-file to a text-file
    QString mGcpFileName;
    QString mGcpDatabaseFileName;
    QString mGcpMasterDatabaseFileName;
    int mMasterSrid;
    QString mGcpBaseFileName;
    int mRasterYear;
    int mRasterScale;
    int mRasterNodata;

    int mLoadGTifInQgis;
    QMap<int, QString> mGcp_coverages;
    QMap<QString, QString> map_providertags;

    QgsGeorefTransform::TransformParametrisation mTransformParam;
    QgsImageWarper::ResamplingMethod mResamplingMethod;
    QgsGeorefTransform mGeorefTransform;
    QString mCompressionMethod;

    QgisInterface *mIface = nullptr;

    QgsGCPList *getGCPList() { return mGCPListWidget->getGCPList(); }
    QgsMapCanvas *mCanvas = nullptr;
    QgsRasterLayer *mLayer = nullptr;
    bool mAgainAddRaster = false;

    // mj10777

    int mEventGcpStatus = 0;
    int mEventPointStatus = 0;
    int mEventPixelStatus = 0;

    /**
     * Simplify the commit of the gcp-layers, with checking and commiting as needed
     * \note
     * For the Spatialite Gcp-Logic to work correctly
     * - added, changed or deleted data must be saved by OGR to the Database-Source
     * ->  since the 'GCP_Compute' reads the stored data to create the needed matrix
     * --> uncommited data in the 'VectorLayerEditBuffer' cannot be evaluated
     * this is the main reason for commiting the data after each change
     * - which is contrary to the normal behavour in the QGIS-Application
     * \note
     * The compleate process is
     * - After loading the Data: startEditing()
     * -> After any Adding, Changing of Delete: editBuffer()->commitChanges
     * --> During unloading: endEditCommand()
     * \note
     * - used during events and Gcp-Layer unloading in clearGCPData()
     * -> triggerRepaint will be called after commit
     * \note
     * - this function cause Appllication crashes in its intiallial form
     * ->  'gcp_layer->commitChanges' instead of 'editBuffer()->commitChanges'
     * --> Warning: QUndoStack::endMacro(): no matching beginMacro()
     * ---> possibly from QgsVectorLayer::endEditCommand() : undoStack()->endMacro();
     * \param gcp_layer either gcp_points or gcp_pixels
     * \param leaveEditable to call startEditing() and set set proper QgsVectorLayerEditBuffer member
     * \return true in case of success of commitChanges()
     */
    bool saveEditsGcp( QgsVectorLayer *gcp_layer, bool leaveEditable );

    /**
     * Georeferencer LayerTreeGroup displayed in QGIS-Application
     * \note
     *  Contains 3 Sub-Groups:
     *  - mTreeGroupGcpPoints
     *  -> mLayerGcpPoints [Map-Points of active Gcp-Points]
     *  - mTreeGroupGcpCutlines [Helper ceometries for Georeferencing]
     *  -> mLayerCutlinePoints
     *  -> mLayerCutlineLinestrings
     *  -> mLayerCutlinePolygons [for gdal cutlines 'cutline_type=77' of id_gcp_coverage]
     * - mTreeGroupGtifRasters [for loadGTifInQgis]
     *  -- mTreeLayerGtifRaster [to store georeferenced result (if exists)]
     *  --> mLayerGtifRaster
     */
    QgsLayerTreeGroup *mTreeGroupGeoreferencer = nullptr;
    QgsLayerTreeGroup *mTreeGroupGcpPoints = nullptr;
    //! Raster file without extention Lower-Case from Database since QGIS 3.0
    QString mGcpCoverageName;
    //! Original Spelling of mGcpCoverageName from Database since QGIS 3.0
    QString mGcpCoverageNameBase;

    /**
     * Gcp-Points/Pixel selected name from Database
     * \see mGcpDbData->mGcpPointsTableName;
     * \since QGIS 3.0
     */
    QString mGcpPointsTableName;

    /**
     * Gcp-Points Geometry-Name from Gcp-Points/Pixel Table
     * \since QGIS 3.0
     */
    const QString mGcpPointsGeometryName = "gcp_point";

    /**
     * Gcp-Pixel Geometry-Name from Gcp-Points/Pixel Table
     * \since QGIS 3.0
     */
    const QString mGcpPixelGeometryName = "gcp_pixel";
    //! id of Gcp-Points/Pixel selected GcpPointsTable from Database since QGIS 3.0
    int mIdGcpCoverage;
    //! id of Cutline selected GcpPointsTable from Database since QGIS 3.0
    int mIdGcpCutline;

    /**
     * Active GcpPoints Layername as used in the TreeGroup
     *  - Format: tablename(geometryname)
     * \since QGIS 3.0
     */
    QString mGcpPointsLayername;

    /**
     * Active GcpPoints VectorLayer
     * \since QGIS 3.0
     */
    QgsVectorLayer *mLayerGcpPoints = nullptr;

    /**
     * Active GcpPixels Layername as used in the TreeGroup
     *  - Format: tablename(geometryname)
     * \since QGIS 3.0
     */
    QString mGcpPixelLayername;

    /**
     * Active GcpPixels VectorLayer
     * \since QGIS 3.0
     */
    QgsVectorLayer *mLayerGcpPixels = nullptr;

    /**
     * TreeGroup in QGIS
     *  - contains cutline_points, linestrings and polygons
     * \since QGIS 3.0
     * \see mLayerCutlinePoints
     * \see mLayerCutlineLinestrings
     * \see mLayerCutlinePolygons
     */
    QgsLayerTreeGroup *mTreeGroupGcpCutlines = nullptr;

    /**
     * Cutline-Points Table from Database
     * \since QGIS 3.0
     */
    const QString mCutlinePointsTableName = "create_cutline_points";

    /**
     * Cutline-Points Geometry-Name from Database
     * \since QGIS 3.0
     */
    const QString mCutlinePointsGeometryName = "cutline_point";

    /**
     * Active Cutline-Points Layername as used in the TreeGroup
     *  - Format: tablename(geometryname)
     * \since QGIS 3.0
     * \see mCutlinePointsTableName
     * \see mCutlinePointsGeometryName
     */
    QString mCutlinePointsLayername = QString::null;

    /**
     * Active Cutline-Points VectorLayer
     * \since QGIS 3.0
     */
    QgsVectorLayer *mLayerCutlinePoints = nullptr;

    /**
     * Cutline-Linestrings Table from Database
     * \since QGIS 3.0
     */
    const QString mCutlineLinestringsTableName = "create_cutline_linestrings";

    /**
     * Cutline-Points Geometry-Name from Database
     * \since QGIS 3.0
     */
    const QString mCutlineLinestringsGeometryName = "cutline_linestring";

    /**
     * Active Cutline-Linestrings Layername as used in the TreeGroup
     *  - Format: tablename(geometryname)
     * \since QGIS 3.0
     * \see mCutlineLinestringsTableName
     * \see mCutlineLinestringsGeometryName
     */
    QString mCutlineLinestringsLayername = QString::null;

    /**
     * Active Cutline-Linestrings VectorLayer
     * \since QGIS 3.0
     */
    QgsVectorLayer *mLayerCutlineLinestrings = nullptr;

    /**
     * Cutline-Polygons Table from Database
     * \since QGIS 3.0
     */
    const QString mCutlinePolygonsTableName = "create_cutline_polygons";

    /**
     * Cutline-Polygons Geometry-Name from Database
     * \since QGIS 3.0
     */
    const QString mCutlinePolygonsGeometryName = "cutline_polygon";

    /**
     * Active Cutline-Polygons Layername as used in the TreeGroup
     *  - Format: tablename(geometryname)
     * \since QGIS 3.0
     * \see mCutlinePolygonsTableName
     * \see mCutlinePolygonsGeometryName
     */
    QString mCutlinePolygonsLayername = QString::null;

    /**
     * Active Cutline-Polygons VectorLayer
     * \since QGIS 3.0
     */
    QgsVectorLayer *mLayerCutlinePolygons = nullptr;

    /**
     * TreeGroup in QGIS
     *  - contains georeferenced RasterLayer
     * \since QGIS 3.0
     * \see mTreeLayerGtifRaster
     * \see mLayerGtifRaster
     */
    QgsLayerTreeGroup *mTreeGroupGtifRasters = nullptr;

    /**
     * TreeLayer in QGIS
     *  - contains georeferenced RasterLayer
     * \since QGIS 3.0
     * \see mTreeLayerGtifRaster
     * \see mLayerGtifRaster
     */
    QgsLayerTreeLayer *mTreeLayerGtifRaster = nullptr;

    /**
     * Active georeferenced RasterLayer
     * \since QGIS 3.0
     */
    QgsRasterLayer *mLayerGtifRaster = nullptr;

    /**
     * TreeGroup in QGIS
     *  - contains GcpMaster VectorLayer
     * \since QGIS 3.0
     * \see mLayerGcpMaster
     */
    QgsLayerTreeGroup *mTreeGroupGcpMaster = nullptr;

    /**
     * Gcp-Master Points Table from Database
     * \since QGIS 3.0
     */
    const QString mGcpMasterTableName = "gcp_master";

    /**
     * Gcp-Master Points Geometry-Name from Database
     * \since QGIS 3.0
     */
    const QString mGcpMasterGeometryName = "gcp_point";

    /**
     * Active Gcp-Master Layername as used in the TreeGroup
     *  - Format: tablename(geometryname)
     * \since QGIS 3.0
     * \see mGcpMasterTableName
     * \see mGcpMasterGeometryName
     */
    QString mGcpMasterLayername = QString::null;

    /**
     * Active GcpMater-Points VectorLayer
     * \since QGIS 3.0
     */
    QgsVectorLayer *mLayerGcpMaster = nullptr;

    /**
     * Georeferencer LayerTreeGroup displayed in Georeferencer
     * \note
     *  Contains 1 Sub-Group:
     *  - mTreeGroupCutlineMercator
     *  -> mLayerMercatorPolygons [for gdal cutlines 'cutline_type=77' of id_gcp_coverage]
     *  --> for use with non-georeference images [sample: maps with folds]
     * \note
     *  - mLayerGcpPixels [Pixel-Points of active Gcp-Points]
     *  -> Does not belong to a group [should not be seen in the Layer Panel]
     * \since QGIS 3.0
     */
    QgsLayerTreeGroup *mTreeGroupCutlineMercator = nullptr;

    /**
     * Mercator Polygons Table from Database
     * \since QGIS 3.0
     */
    const QString mMercatorPolygonsTableName = "create_mercator_polygons";

    /**
     * Mercator Polygons Map-Points Geometry-Name from Database
     * \since QGIS 3.0
     */
    const QString mMercatorCutlinePolygonsGeometryName = "cutline_polygon";

    /**
     * Active Gcp-Master Layername as used in the TreeGroup
     *  - Format: tablename(geometryname)
     * \since QGIS 3.0
     * \see mMercatorPolygonsTablename
     * \see mMercatorCutlinePolygonsGeometryName
     */
    QString mMercatorCutlinePolygonsLayername = QString::null;

    /**
     * Mercator Polygons Pixel-Points Geometry-Name from Database
     * \since QGIS 3.0
     */
    const QString mMercatorPolygonsGeometryName = "mercator_polygon";

    /**
     * Active Gcp-Master Layername as used in the TreeGroup
     *  - Format: tablename(geometryname)
     * \since QGIS 3.0
     * \see mMercatorPolygonsTablename
     * \see mMercatorPolygonsGeometryName
     */
    QString mMercatorPolygonsLayername = QString::null;

    /**
     * Active Mercator-Polygons VectorLayer
     * \since QGIS 3.0
     */
    QgsVectorLayer *mLayerMercatorPolygons = nullptr;

    /**
     * Active Mercator-Transform-Polygons VectorLayer
     * \since QGIS 3.0
     */
    QgsVectorLayer *mLayerMercatorCutlinePolygons = nullptr;

    /**
     * Provider to use
     *  - 'spatialite': default
     *  - 'ogr'
     * \note
     *  Until QgsOgrProvider supports Spatialite with tables that containe more than 1 geometry properly
     *  - 'ogr' cannot be used.
     * \since QGIS 3.0
     */
    QString mLayerProviderType;

    /**
     * Connection string to use, depending on Provider to use
     *  - 'spatialite': 'dbname=%1 table=%2 (%3)'
     *  - 'ogr': '%1|layername=%2|geometryname=%3'
     * \note
     *  Until QgsOgrProvider supports Spatialite with tables that containe more than 1 geometry properly
     *  - 'ogr' cannot be used.
     * \since QGIS 3.0
     * \see formatUrl
     */
    QString mFormatUrl;

    /**
     * Create Connection string to use, depending on Provider to use
     *  - 'spatialite': 'dbname=%1 table=%2 (%3)'
     *  - 'ogr': '%1|layername=%2|geometryname=%3'
     * \note
     *  When first used, mFormatUrl and the Layername strings will be set.
     * \since QGIS 3.0
     * \see mFormatUrl
     * \see mCutlinePointsLayername
     * \see mCutlineLinestringsLayername
     * \see mCutlinePolygonsLayername
     * \see mGcpMasterLayername
     * \see mMercatorPolygonsLayername
     * \see mMercatorCutlinePolygonsLayername
     */
    QString formatUrl( const QString &sGcpDatabaseFileName, const QString &sGcpTableName, const QString &sGcpGeometryName );

    /**
     * Load Gcp-Master Database as LayerTreeGroup to be displayed in QGIS
     * \note
     *  Contains 1 Sub-Group:
     *  - mTreeGroupCutlineMercator
     *  -> mLayerMercatorPolygons [for gdal cutlines 'cutline_type=77' of id_gcp_coverage]
     *  --> for use with non-georeference images [sample: maps with folds]
     * \note
     *  - mLayerGcpPixels [Pixel-Points of active Gcp-Points]
     *  -> Does not belong to a group [should not be seen in the Layer Panel]
     */
    bool loadGcpMaster( QString  sDatabaseFilename = QString::null );

    /**
     * QgsMapCanvasLayer displayed in Georeferencer
     *  - needed to store the visibility status of the Layers
     * \note
     *  Contains 3 Layers
     *  - mLayerGcpPixels [pixel gcp]
     *  - mLayerMercatorPolygons [possible cutlines for raster ]
     *  - mCanvas [raster being shown]
     *  -> must be last of list
     * \note
     *  - only the first 2 are contained in 'mTreeGroupCutlineMercator'
     *  -> to be turned on/off as needed
    * @see visibilityLayerTreeViewChanged
     */
    QList<QgsMapCanvasLayer> mMapCanvasLayers;

    /**
     * Create list of unique colors
     *  - used for cutline polygons
     * \since QGIS 3.0
    * @see createSvgColors
    * @see setCutlineLayerSettings
     */
    QStringList mSvgColors;

    QgsMapTool *mToolZoomIn = nullptr;
    QgsMapTool *mToolZoomOut = nullptr;
    QgsMapTool *mToolPan = nullptr;
    QgsMapTool *mToolAddPoint = nullptr;
    QgsMapTool *mToolNodePoint = nullptr;
    QgsMapTool *mToolMovePoint = nullptr;
    QgsMapTool *mToolMovePointQgis = nullptr;

    QgsGeorefDataPoint *mMovingPoint = nullptr;
    QgsGeorefDataPoint *mMovingPointQgis = nullptr;
    QPointer<QgsMapCoordsDialog> mMapCoordsDialog = nullptr;

    bool mUseZeroForTrans;
    bool mExtentsChangedRecursionGuard;
    bool mLoadInQgis;

    QgsDockWidget *mDock = nullptr;
    int messageTimeout();

    /**
     * Project interaction with static Functions to
     *  - create, read and administer Gcp-Coverage Tables
     *  -> also using Spatialite Gcp-Logic
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \see QgsSpatiaLiteProviderGcpUtils::GcpDbData
     */
    QgsSpatiaLiteProviderGcpUtils::GcpDbData *mGcpDbData = nullptr;

    /**
     * isGcpDb
     *  - Has the Spatialite been made and is usable
     *  -> true Gcp-Points have been read
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \see mLayerGcpPixels
     * \return  true íf usable result have been made
     */
    bool isGcpDb() {if ( mLayerGcpPixels != nullptr ) return true; else return false;};

    /**
     * Project Functions to interact with
     *  - create, read and administer Gcp-Coverage Tables
     *  -> also using Spatialite Gcp-Logic
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \see QgsSpatiaLiteProviderGcpUtils::GcpDbData
     */
    bool readGcpDb( QString  sDatabaseFilename, bool b_DatabaseDump = false );

    /**
     * Create or read a points TABLE for a Gcp-Coverage Database
     *  - using the 'Add Raster' Dialog
     *  -> that Raster will be added to the gcp-coverages
     *  --> and a 'gcp_points_*' TABLE created
     *  - the file path (without file.name) will be stored in the column 'raster_path'
     *  -> if raster-file is moved somewhere else, this entry must be UPDATEd
     *  - the file-name will be stored in the column: 'raster_file'
     *  -> if raster-file is renamed to something else, this entry must be UPDATEd
     *  The column 'coverage_name' is based on the lowercase (original) file-name WITHOUT path and and extention
     *  The created  'gcp_points' TABLE is based on 'gcp_points_'+'coverage_name'
     *  - TODO: continue documentation
     * \note
     *  -> see Function documentation for more details
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \see QgsSpatiaLiteProviderGcpUtils::createGcpDb
     * \param sDatabaseFilename Database file name to create [can exist]
     * \param b_dump create and '.sql' file with the used Sql-Statements that can be run as a script
     */
    bool createGcpDb( bool b_DatabaseDump = false );

    /**
     * Update a points TABLE for a Gcp-Coverage Database
     * \param sCoverageName Gcp-Points TABLE to update
     * \see QgsSpatiaLiteProviderGcpUtils::updateGcpDb
     * \since QGIS 3.0
     */
    bool updateGcpDb( QString sCoverageName );

    /**
     * Update a Gcp-Compute TABLE for a Gcp-Coverage points TABLE
     * \param sCoverageName Gcp-Points TABLE to update
     * \since QGIS 3.0
     * \see QgsSpatiaLiteProviderGcpUtils::updateGcpCompute
     */
    bool updateGcpCompute( QString sCoverageName );

    /**
     * Update a Gcp-Translate TABLE for a Gcp-Coverage points TABLE
     * \param sCoverageName Gcp-Points TABLE to update
     * \since QGIS 3.0
     * \see QgsSpatiaLiteProviderGcpUtils::updateGcpTranslate
     */
    bool updateGcpTranslate( QString sCoverageName );

    /**
     * Converts a Pixel or Map-Point to Map or Pixel-Point
     * \param sCoverageName name of map to search for
     * \param inputPoint point to convert [not used when id_gcp > 0]
     * \param bToPixel true=convert Map-Point to Pixel Point ; false: Convert Pixel-Point to Map-Point
     * \param i_order 0-3 [ThinPlateSpline, PolynomialOrder1, PolynomialOrder2, PolynomialOrder3]
     * \param bReCompute re-calculate value by reading all enable points, othewise read stored values in gcp_convert.
     * \param id_gcp read value from specific gcp point [inputPoint will be ignored]
     * \return QgsPoint of result (0,0 when invalid)
     * \since QGIS 3.0
     * \see QgsSpatiaLiteProviderGcpUtils::getGcpConvert
     */
    QgsPoint getGcpConvert( QString sCoverageName, QgsPoint inputPoint, bool bToPixel = false, int i_order = 0, bool bReCompute = true, int id_gcp = -1 );

    /**
     * Given a meters Map-Unit value
     *  - a realistic Map-Unit value for the srid used will be returned
     * meters value: parmsGcpDbData->mGcpMasterArea
     * srid value:  parmsGcpDbData->mGcpSrid
     * \note
     *  - for Degrees a start position must be set
     *  -> parmsGcpDbData->mInputPoint
     * \see spatialiteInitEx
     * \param parmsGcpDbData  to store Database file-name, count, handle and cache
     * \return Map-Unit for the given srid
     */
    double getMetersToMapPoint( double d_GcpMasterArea = 5.0, int i_srid = -1,  QgsPoint inputPoint = QgsPoint( 0.0, 0.0 ) );

    /**
     * Create an empty Gcp-Master Database
     *  - a Database to be used to reference POINTs to assist while Georeferencing
     *  -> all POINTs must be of the same srid
     *  --> that are listed in the central 'gcp_coverages' table
     * \note
     *  - this can also be used to test if the Database is valid
     *  -> i.e. contains a 'gcp_master' TABLE
     *  --> Warning: test that the file exist beforehand, otherwise the Database will be created !
     *  --> srid being used will be returned if valid, othewise INT_MIN
     * \note
     *  -> see Function documentation for more details
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \see QgsSpatiaLiteProviderGcpUtils::createGcpDb
     * \param sDatabaseFilename Database file name to create [can exist]
     * \param srid to use for cutline TABLEs that will be created
     * \param b_dump create and '.sql' file with the used Sql-Statements that can be run as a script
     * \return return_srid INT_MIN if invalid, otherwise the srid being used will be returned
     */
    int createGcpMasterDb( QString  sDatabaseFilename,  int i_srid = 3068, bool b_dump = false );

    /**
     * Create an empty Gcp-Coverage Database
     *  - a Database to be used to add Rasters that will be Georeferenced
     *  -> containing 'gcp_points_*' for each Raster
     *  --> that are listed in the central 'gcp_coverages' table
     * \note
     *  - the 'GcpDbData->mGcpCoverageName' MUST be set to
     *  -> 'gcp_coverage' for the createGcpDb Function to know that a new, empty Database shouöld be created
     *  --> if the given Database-File exist: the function will return false
     * \note
     *  -> see Function documentation for more details
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \see QgsSpatiaLiteProviderGcpUtils::createGcpDb
     * \param sDatabaseFilename Database file name to create (must not exist)
     * \param srid to use for cutline TABLEs that will be created
     * \param b_dump create and '.sql' file with the used Sql-Statements that can be run as a script
     * \return return_srid INT_MIN if invalid, otherwise the srid being used will be returned
     */
    int createGcpCoverageDb( QString  sDatabaseFilename, int i_srid = 3068, bool b_dump = false );

    /**
     * Bulk INSERT (or UPDATE) of Gcp-coverages Points TABLE
     *  - execute INSERT (or UPDATE)   commands stored in
     *  -- retrieving created id_gcp [INSERT only]
     * \note
     * QMap<int, QString> gcp_coverages of the parmsGcpDbData structure
     *  - the QString will contain the sql-command
     *  -- it will be replaced with the returned id_gcp
     *  - the 'int' will be used by the calling function
     *  -- to identify to what the created id_gcp belongs to
     * \note
     * mOrder of the parmsGcpDbData structure
     *  - will contain what type of Sql-Command will be used
     *  -- 1=INSERT (will call getSqliteSequence after each INSERT)
     *  -- 2=UPDATE (will not call SqliteSequence)
     * \see getSqliteSequence
     * \param parmsGcpDbData  which will contain a spatialite connection (with mParseString to use and if Spatialite Gcp-logic is active)
     * \return true if all commands where executed correctly
     */
    int bulkGcpPointsInsert( QgsGCPList *master_GcpList );
    // docks ------------------------------------------
    QgsLayerTreeGroup *mRootLayerTreeGroup = nullptr;
    QgsDockWidget *mLayerTreeDock = nullptr;
    QgsLayerTreeView *mLayerTreeView = nullptr;
    void initLayerTreeView();
    bool mAvoidUnneededUpdates = false;
};

#endif
