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
    QgsGeorefDockWidget( const QString & title, QWidget * parent = nullptr, Qt::WindowFlags flags = nullptr );
};

class QgsGeorefPluginGui : public QMainWindow, private Ui::QgsGeorefPluginGuiBase
{
    Q_OBJECT

  public:
    QgsGeorefPluginGui( QgisInterface* theQgisInterface, QWidget* parent = nullptr, Qt::WindowFlags fl = nullptr );
    ~QgsGeorefPluginGui();

  protected:
    void closeEvent( QCloseEvent * ) override;

  private slots:
    // file
    void reset();
    /**
     * Reacts to mActionOpenRaster
     *  - portion of former 'openRaster'
     *  - reads settings for last raster Directory and file filter
     *  - calls QFileDialog::getOpenFileName
     *  -> calls 'openRaster' with QFileInfo raster_file
     * @see openRaster
     */
    void openRasterDialog();
    void doGeoreference();
    void generateGDALScript();
    bool getTransformSettings();

    // edit
    void setAddPointTool();
    void setDeletePointTool();
    void setMovePointTool();

    // view
    void setZoomInTool();
    void setZoomOutTool();
    void zoomToLayerTool();
    void zoomToLast();
    void zoomToNext();
    void setPanTool();
    void linkGeorefToQGis( bool link );
    void linkQGisToGeoref( bool link );
    void zoomMercatorPolygonExtent();
    /**
     * Changes the Selection of a Layer
     *  - MapTools will then react to these Layers
     * @note
     * QgsMapCanvasLayer displayed in Georeferencer
     *  - needed to store the visibility status of the Layers
     *  Contains 3 Layers
     *  - layer_gcp_pixels [pixel gcp]
     *  - layer_mercator_polygons [possible cutlines for raster ]
     *  - mCanvas [raster being shown]
     *  -> must be last of list
     * @note
     *  - only the first 2 are contained in 'group_cutline_mercator'
     *  -> to be turned on/off as needed
    * @see mMapCanvasLayers
     */
    void activeLayerTreeViewChanged( QgsMapLayer *layer );
    /**
     * Changes the Visablity of a Layer
     * @note
     * QgsMapCanvasLayer displayed in Georeferencer
     *  - needed to store the visibility status of the Layers
     *  Contains 3 Layers
     *  - layer_gcp_pixels [pixel gcp]
     *  - layer_mercator_polygons [possible cutlines for raster ]
     *  - mCanvas [raster being shown]
     *  -> must be last of list
     * @note
     *  - only the first 2 are contained in 'group_cutline_mercator'
     *  -> to be turned on/off as needed
    * @see mMapCanvasLayers
     */
    void visibilityLayerTreeViewChanged( QgsLayerTreeNode* layer_treenode, Qt::CheckState check_state );

    // gcps
    void addPoint( const QgsPoint& pixelCoords, const QgsPoint& mapCoords, int id_gcp = -1, bool b_PixelMap = true,
                   bool enable = true, bool refreshCanvas = true );
    void deleteDataPoint( QPoint pixelCoords );
    void deleteDataPoint( int index );
    void showCoordDialog( const QgsPoint &pixelCoords );

    void selectPoint( QPoint );
    void movePoint( QPoint );
    void releasePoint( QPoint );
    /**
     * Reacts to a Spatialite-Database storing the Gcp-Points [gcp_point, gcp_pixel]
     *  - the TRIGGER will create an empty POINT (0,0) if not set
     *  -- which is what happens here
     * \note both geometries are in one TABLE
     *  an INSERT can therefor only be done once, the other must be UPDATEd if 'SELECT HasGCP()' returns true
     *  and enough gcp-points exist to make a calculation
     *  For this the primary key is needed for the INSERTed record, so that the POINT in the other Layer can be retrieved and UPDATEd
     * - this causes the Event to called during 'commitChanges' (the used 'fid' will be stored in 'mEvent_gcp_status')
     *  -- after 'commitChanges' has compleated, the value in 'mEvent_gcp_status' will be used to retrive the inserted primary-key
     * -- the record from the other gcp-layer will be retrieved with the primary-key and UPDATEd
     * \note 'mEvent_point_status' will be set to 1 (for 'Map-Point to Pixel-Point') and 2  (for 'Pixel-Point to Map-Point')
     * - this value will be read in the featureDeleted_gcp and geometryChanged_gcp and will return if > 0
     * \note the added geometry is added immediately to the Database
     * - which will also save any new position that were not commited after 'geometryChanged_gcp'
     * @see getGcpConvert
     * @see jumpToGcpConvert
     * @param fid (a minus number when inserting to the database, a positive number when adding to a layer)
     */
    void featureAdded_gcp( QgsFeatureId fid );
    /**
     * Reacts to a Spatialite-Database storing the Gcp-Points [gcp_point, gcp_pixel]
     *  - any change in the position will only effect the calling gcp-layer
     *  -- any activity should then only be done once
     * \note all 'fid' < 0 will be ignored
     *  - the changed position in NOT saved to the database
     * @param fid (a minus number when inserting to the database, a positive number when adding to a layer)
     * @param changed_geometry the changed geometry value must be updated in the gcp-list
     */
    void geometryChanged_gcp( QgsFeatureId fid, QgsGeometry& changed_geometry );
    /**
     * Reacts to a Spatialite-Database storing a cutline [cutline_points, linestrings, polygons and mecator_polygons]
     *  - save edit after each change
     *  -- so that the geometries can bee seen in the Canvos while moving around
     * \note some default values will be stored
     *  - id_gcp_coverage of the active coverage in georeferencer
     *  - belongs_to_01 the name of the active coverage in georeferencer
     *  - belongs_to_02 the raster_name of the active coverage in georeferencer
     * @param fid (a minus number when inserting to the database, a positive number when adding to a layer)
     */
    void featureAdded_cutline( QgsFeatureId fid );
    /**
     * Reacts to a Spatialite-Database storing a cutline [cutline_points, linestrings, polygons and mecator_polygons]
     *  - save edit after each change
     *  -- so that the geometries can bee seen in the Canvos while moving around
     * @param fis (a minus number when inserting to the database, a positive number when adding to a layer)
     * @param changed_geometry the changed geometry value must be updated in the gcp-list
     */
    void geometryChanged_cutline( QgsFeatureId fid, QgsGeometry& changed_geometry );

    void loadGCPsDialog();
    void saveGCPsDialog();

    // settings
    void showRasterPropertiesDialog();
    void showMercatorPolygonPropertiesDialog();
    void showGeorefConfigDialog();

    // GcpDatabase
    void setLegacyMode();
    void setPointsPolygon();
    void listGcpCoverages();
    // Load selected coverage, when not alread loaded
    /**
     * Result of QgsGcpCoveragesDialog
     *  - selection of Gcp-Coverage
     *  -> calls 'openRaster' with QFileInfo raster_file
     * @see openRaster
     * @param id_selected_coverage id of coverage from GcpDb
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
     * @see openRasterDialog
     * @see loadGcpCoverage
     * @see addRaster
     */
    void openRaster( QFileInfo raster_file );
    void openGcpDb();

    // plugin info
    void contextHelp();

    // comfort
    void jumpToGCP( uint theGCPIndex );
    void extentsChangedGeorefCanvas();
    void extentsChangedQGisCanvas();

    // canvas info
    void showMouseCoords( const QgsPoint &pt );
    void updateMouseCoordinatePrecision();

    // Histogram stretch
    void localHistogramStretch();
    void fullHistogramStretch();


    // when one Layer is removed
    void layerWillBeRemoved( const QString& theLayerId );
    void extentsChanged(); // Use for need add again Raster (case above)
    /**
     * Update Modual
     *  - calls createGCPVectors to recreate List of enabled Points
     *  - rebuilds List calculating residual value
     * @note this is also done in updateModel
     *  - then calls updateTransformParamLabel to set summery of results
     * @see updateTransformParamLabel
     * @see QgsGCPListWidget::updateModel
     * @return true if the id was found, otherwise false
     */
    bool updateGeorefTransform();

    void updateIconTheme( const QString& theme );

  private:
    enum SaveGCPs
    {
      GCPSAVE,
      GCPSILENTSAVE,
      GCPDISCARD,
      GCPCANCEL
    };

    // gui
    void createActions();
    void createActionGroups();
    void createMapCanvas();
    void createMenus();
    void createDockWidgets();
    QLabel* createBaseLabelStatus();
    void createStatusBar();
    void setupConnections();
    void removeOldLayer();

    // Mapcanvas Plugin
    /**
     * Adding of selected Raster to Georeference Canvas
     *  - creates QgsRasterLayer
     *  -> calls 'loadGCPs' with QFileInfo raster_file
     *  - adds created QgsRasterLayer to Canvas
     * @see openRaster
     * @see extentsChanged
     * @see loadGCPs
     * @param file name as string
     */
    void addRaster( const QString& file );
    void loadGTifInQgis( const QString& gtif_file );

    // settings
    void readSettings();
    void writeSettings();

    // gcp points
    /**
     * Loading of Gcp-Points
     * - clears Gcp-Points when needed
     * - calls createGcpDb
     * -> will return false when 'mLegacyMode == 0'
     * @note
     *  - LegacyMode
     *  -> when a Gcp-Database has been read (createGcpDb returns true)
     *  --> Gcp-Points will read from the OGR-Datasource
     *  -> when no Gcp-Database was found
     *  --> Gcp-Points will read from the '.points' file
     * - both methods will:
     * -> add the QgsGeorefDataPoint
     * - calls 'setGCPList' AFTER all points have been added
     * @see openRaster
     * @see extentsChanged
     * @see createGcpDb
     * @see QgsGCPListWidget::setGCPList
     * @param file name as string
     */
    bool loadGCPs( /*bool verbose = true*/ );
    void saveGCPs();
    QgsGeorefPluginGui::SaveGCPs checkNeedGCPSave();

    // mj10777
    int mGcp_srid;
    QString s_gcp_authid;
    QString s_gcp_description;
    QString mGcp_coverage_name;
    QString mGcp_coverage_name_base;
    QString mGcp_points_table_name;
    int mId_gcp_coverage;
    int mId_gcp_cutline;

    void jumpToGcpConvert( QgsPoint input_point, bool b_toPixel = false );
    /**
     * Translate QgsGeorefTransform::TransformParametrisation numbering to Spatialite numbering
     * use when calling getGcpConvert
     * @see getGcpConvert
     * Note only ThinPlateSpline, PolynomialOrder1, PolynomialOrder2, PolynomialOrder3 and  are supported
     * @param mTransformParam: as used in project
     * @return 0-3, otherwise 0 for ThinPlateSpline
     */
    int getGcpTransformParam( QgsGeorefTransform::TransformParametrisation i_TransformParam );
    /**
     * Translate QgsGeorefTransform::TransformParametrisation numbering to Spatialite numbering
     * use when calling getGcpConvert
     * @see getGcpConvert
     * Note only ThinPlateSpline, PolynomialOrder1, PolynomialOrder2, PolynomialOrder3 and  are supported
     * @param mTransformParam: as used in project
     * @return 0-3, otherwise 0 for ThinPlateSpline
     */
    QgsGeorefTransform::TransformParametrisation setGcpTransformParam( int i_TransformParam );
    /**
     * Translate to and from a readable form for the gcp_points Database
     * - GRA_NearestNeighbour, GRA_Bilinear, GRA_Cubic, GRA_CubicSpline and GRA_Lanczos are supported
     * @see getGcpConvert
     * Note only ThinPlateSpline, PolynomialOrder1, PolynomialOrder2, PolynomialOrder3 and  are supported
     * @param mResamplingMethod: as used in project
     * @return  above, otherwise 'Lanczos' for GRA_Lanczos
     */
    QString getGcpResamplingMethod( QgsImageWarper::ResamplingMethod i_ResamplingMethod );
    /**
     * Translate to and from a readable form for the gcp_points Database
     * - GRA_NearestNeighbour, GRA_Bilinear, GRA_Cubic, GRA_CubicSpline and GRA_Lanczos are supported
     * @param s_ResamplingMethod: as used in project
     * @return above, otherwise 'Lanczos' for GRA_Lanczos
     */
    QgsImageWarper::ResamplingMethod setGcpResamplingMethod( QString s_ResamplingMethod );
    bool mSpatialite_gcp_enabled;
    bool isGcpEnabled();
    // mj10777: add gui logic for this and store in setting
    bool b_gdalscript_or_gcp_list;
    // mj10777: add gui logic for this and store in setting
    bool b_points_or_spatialite_gcp;
    QString mError;
    QgsGeorefTransform mTransformType;
    QString mGcpLabelExpression;
    int mGcp_label_type;
    int mLegacyMode;
    int mPointsPolygon;
    double fontPointSize;
    bool exportLayerDefinition( QgsLayerTreeGroup *group_layer, QString file_path = QString::null );
    // QgsMapTool *mAddFeature;
    // QgsMapTool *mMoveFeature;
    // QgsMapTool *mNodeTool;

    // georeference
    bool georeference();
    bool writeWorldFile( const QgsPoint& origin, double pixelXSize, double pixelYSize, double rotation );
    bool writePDFReportFile( const QString& fileName, const QgsGeorefTransform& transform );
    bool writePDFMapFile( const QString& fileName, const QgsGeorefTransform& transform );
    void updateTransformParamLabel();

    // gdal script
    void showGDALScript( const QStringList& commands );
    QString generateGDALtranslateCommand( bool generateTFW = true );
    /* Generate command-line for gdalwarp based on current GCPs and given parameters.
     * For values in the range 1 to 3, the parameter "order" prescribes the degree of the interpolating polynomials to use,
     * a value of -1 indicates that thin plate spline interpolation should be used for warping.*/
    QString generateGDALwarpCommand( const QString& resampling, const QString& compress, bool useZeroForTrans, int order,
                                     double targetResX, double targetResY );

    // mj10777
    QString generateGDALgcpCommand();

    // utils
    bool checkReadyGeoref();
    QgsRectangle transformViewportBoundingBox( const QgsRectangle &canvasExtent, QgsGeorefTransform &t,
        bool rasterToWorld = true, uint numSamples = 4 );
    QString convertTransformEnumToString( QgsGeorefTransform::TransformParametrisation transform );
    QString convertResamplingEnumToString( QgsImageWarper::ResamplingMethod resampling );
    int polynomialOrder( QgsGeorefTransform::TransformParametrisation transform );
    QString guessWorldFileName( const QString &rasterFileName );
    QIcon getThemeIcon( const QString &theName );
    bool checkFileExisting( const QString& fileName, const QString& title, const QString& question );
    bool equalGCPlists( const QgsGCPList *list1, const QgsGCPList *list2 );
    void logTransformOptions();
    void logRequaredGCPs();
    void clearGCPData();
    bool setGcpLayerSettings( QgsVectorLayer *layer_gcp );
    bool setCutlineLayerSettings( QgsVectorLayer *layer_cutline );
    /**
     * createSvgColors
     * @param i_method  filling logic to use
     * @param b_reverse  use Dark to Light instead of Light to Dark
     * @param b_grey_black  include Color-Groups White, Grey and Black
     * @returns  list_SvgColors QStringList list containing distinct list of svg color names for use with QColor
     * @note
     *
     *  SVG color keyword names, of which there are 146 (138 unique colors)
     * <a href="https://www.w3.org/TR/SVG/types.html#ColorKeywords </a>
     *
     * X11 color names, listed by color groups, of which there are 11
     * <a href="https://en.wikipedia.org/wiki/Web_colors#Css_colors">Colors listed by Color-Groups </a>
     * @note
     * Gray/Grey are interchnagable (7 colors) 'grey' will be used
     * Alternative names: 2 colors) 'aqua / cyan' and 'magenta / fuchsia',   cyan and magenta will be used
     *
     * Used to create QgsCategorizedSymbolRendererV2 Symbols for Polygons
    * @see setCutlineLayerSettings
     */
    QStringList createSvgColors( int i_method = 0, bool b_reverse = false, bool b_grey_black = false );

    /**
     * Calculates root mean squared error for the currently active
     * ground control points and transform method.
     * Note that the RMSE measure is adjusted for the degrees of freedom of the
     * used polynomial transform.
     * @param error out: the mean error
     * @return true in case of success
     */
    bool calculateMeanError( double& error ) const { return mGCPListWidget->calculateMeanError(error); }

    /** Docks / undocks this window*/
    void dockThisWindow( bool dock );

    QGridLayout* mCentralLayout;

    QgsMessageBar* mMessageBar;
    QMenu *mPanelMenu;
    QMenu *mToolbarMenu;

    QAction *mActionHelp;

    QgsGCPListWidget *mGCPListWidget;
    QLineEdit *mScaleEdit;
    QLabel *mScaleLabel;
    QLabel *mCoordsLabel;
    QLabel *mTransformParamLabel;
    QLabel *mEPSG;
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

    QgisInterface *mIface;

    QgsGCPList* getGCPList() { return mGCPListWidget->getGCPList(); }
    // QgsGCPList* mInitialPoints;
    QgsMapCanvas *mCanvas;
    QgsRasterLayer *mLayer;
    bool mAgainAddRaster;

    // mj10777

    int mEvent_gcp_status;
    int mEvent_point_status;
    int mEvent_pixel_status;

    /**
     * Simplify the commit of the gcp-layers, with checking and commiting as needed
     * @note
     * used during events and Gcp-Layer unloading in clearGCPData()
     * @param gcp_layer either gcp_points or gcp_pixels
     * @param leaveEditable to call startEditing() and set set proper QgsVectorLayerEditBuffer member
     * @return true in case of success of commitChanges()
     */
    bool saveEditsGcp( QgsVectorLayer *gcp_layer, bool leaveEditable );
    /**
     * Georeferencer LayerTreeGroup displayed in QGIS-Application
     * @note
     *  Contains 3 Sub-Groups:
     *  - group_gcp_points
     *  -> layer_gcp_points [Map-Points of active Gcp-Points]
     *  - group_gcp_cutlines [Helper ceometries for Georeferencing]
     *  -> layer_cutline_points
     *  -> layer_cutline_linestrings
     *  -> layer_cutline_polygons [for gdal cutlines 'cutline_type=77' of id_gcp_coverage]
     * - group_gtif_rasters [for loadGTifInQgis]
     *  -- group_gtif_raster [to store georeferenced result (if exists)]
     *  --> mLayer_gtif_raster
     */
    QgsLayerTreeGroup *group_georeferencer;
    QgsLayerTreeGroup *group_gcp_points;
    // Pixel-Points of active Gcp-Points
    QString mGcp_points_layername;
    QgsVectorLayer *layer_gcp_points;
    // group_gcp_cutlines
    // - contains cutline_points, linestrings and polygons
    QgsLayerTreeGroup *group_gcp_cutlines;
    QString mCutline_points_layername;
    QgsVectorLayer *layer_cutline_points;
    QString mCutline_linestrings_layername;
    QgsVectorLayer *layer_cutline_linestrings;
    QString mCutline_polygons_layername;
    QgsVectorLayer *layer_cutline_polygons;
    QgsLayerTreeGroup *group_gtif_rasters;
    QgsLayerTreeLayer *group_gtif_raster;
    QgsRasterLayer *mLayer_gtif_raster;
    /**
     * Georeferencer LayerTreeGroup displayed in Georeferencer
     * @note
     *  Contains 1 Sub-Group:
     *  - group_cutline_mercator
     *  -> layer_mercator_polygons [for gdal cutlines 'cutline_type=77' of id_gcp_coverage]
     *  --> for use with non-georeference images [sample: maps with folds]
     * @note
     *  - layer_gcp_pixels [Pixel-Points of active Gcp-Points]
     *  -> Does not belong to a group [should not be seen in the Layer Panel]
     */
    QgsLayerTreeGroup *group_cutline_mercator;
    QString mMercator_polygons_layername;
    QgsVectorLayer *layer_mercator_polygons;
    // Pixel-Points of active Gcp-Points]
    QString mGcp_pixel_layername;
    QgsVectorLayer *layer_gcp_pixels;
    /**
     * QgsMapCanvasLayer displayed in Georeferencer
     *  - needed to store the visibility status of the Layers
     * @note
     *  Contains 3 Layers
     *  - layer_gcp_pixels [pixel gcp]
     *  - layer_mercator_polygons [possible cutlines for raster ]
     *  - mCanvas [raster being shown]
     *  -> must be last of list
     * @note
     *  - only the first 2 are contained in 'group_cutline_mercator'
     *  -> to be turned on/off as needed
    * @see visibilityLayerTreeViewChanged
     */
    QList<QgsMapCanvasLayer> mMapCanvasLayers;

    QStringList mSvgColors;

    QgsMapTool *mToolZoomIn;
    QgsMapTool *mToolZoomOut;
    QgsMapTool *mToolPan;
    QgsMapTool *mToolAddPoint;
    QgsMapTool *mToolDeletePoint;
    QgsMapTool *mToolMovePoint;
    QgsMapTool *mToolMovePointQgis;

    QgsGeorefDataPoint *mMovingPoint;
    QgsGeorefDataPoint *mMovingPointQgis;
    QPointer<QgsMapCoordsDialog> mMapCoordsDialog;

    bool mUseZeroForTrans;
    bool mExtentsChangedRecursionGuard;
    bool mLoadInQgis;

    QgsDockWidget* mDock;
    int messageTimeout();
    QgsSpatiaLiteProviderGcpUtils::GcpDbData* mGcpDbData;
    bool isGcpDb();
    bool createGcpDb( bool b_DatabaseDump = false );
    bool readGcpDb( QString  s_database_filename, bool b_DatabaseDump = false );
    bool updateGcpDb( QString s_coverage_name );
    bool updateGcpCompute( QString s_coverage_name );
    QgsPoint getGcpConvert( QString s_coverage_name, QgsPoint input_point, bool b_toPixel = false, int i_order = 0, bool b_reCompute = true, int id_gcp = -1 );
    bool createGcpMasterDb( QString  s_database_filename, QString  s_gcp_master_tablename = "gcp_master", int i_srid = 3068 );
    // docks ------------------------------------------
    QgsLayerTreeGroup* mRootLayerTreeGroup;
    // QgsLayerTreeRegistryBridge* mLayerTreeRegistryBridge;
    QgsDockWidget *mLayerTreeDock;
    QgsLayerTreeView* mLayerTreeView;
    void initLayerTreeView();
};

#endif
