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

#include "qgsgcplist.h"
#include "qgsmapcoordsdialog.h"
#include "qgsimagewarper.h"
#include "qgscoordinatereferencesystem.h"
#include "layertree/qgslayertreegroup.h"
#include "layertree/qgslayertreelayer.h"
#include "layertree/qgslayertreeview.h"
#include "qgsvectorlayereditbuffer.h"

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

class QgsGeorefDockWidget : public QgsDockWidget
{
    Q_OBJECT
  public:
    QgsGeorefDockWidget( const QString & title, QWidget * parent = nullptr, Qt::WindowFlags flags = nullptr );
};

class QgsSpatiaLiteProviderGcpUtils
{
  public:
    struct GcpDbData
    {
      GcpDbData( QString s_database_filename, QString s_coverage_name="gcp_cutline", int i_srid=3068)
          : mGCPdatabaseFileName( s_database_filename )
          , mGcp_coverage_name( s_coverage_name )
          , mGcp_srid(  i_srid )
          , mGcp_points_table_name( "" )
          , mLayer( nullptr )
          , mGcp_enabled( false )
          , mInputPoint( QgsPoint(0.0, 0.0) )
          , mToPixel( false )
          , mOrder( 0 )
          , mReCompute( true )
          , mIdGcp( -1 )
          , mId_gcp_coverage(-1 )
          , mId_gcp_cutline(-1 )
          , mTransformParam( -1 )
          , mResamplingMethod( "Lanczos" )
          , mRasterYear( 0 )
          , mRasterScale( 0 )
          , mRasterNodata( -1 )
          , mCompressionMethod( "DEFLATE" )
          , mGCPpointsFileName( "" )
          , mGCPbaseFileName( "" )
          , mRasterFilePath( "" )
          , mRasterFileName( "" )
          , mModifiedRasterFileName( "" )
          , mGcp_coverage_name_base( "" )
          , mError( "" )
          , mSqlDump( false )
          , mDatabaseDump( false )
          , mParseString( ";#;#" )
          , spatialite_cache(nullptr)
          , db_handle(nullptr)
          , mUsed_database_filename(QString::null)
          , mUsed_database_filename_count(0)
      {}
      GcpDbData( QString s_database_filename, QString s_coverage_name, int i_srid, QString s_points_table_name,  QgsRasterLayer *raster_layer, bool b_Gcp_enabled=false)
          : mGCPdatabaseFileName( s_database_filename )
          , mGcp_coverage_name( s_coverage_name )
          , mGcp_srid(  i_srid )
          , mGcp_points_table_name( s_points_table_name )
          , mLayer( raster_layer )
          , mGcp_enabled( b_Gcp_enabled )
          , mInputPoint( QgsPoint(0.0, 0.0) )
          , mToPixel( false )
          , mOrder( 0 )
          , mReCompute( true )
          , mIdGcp( -1 )
          , mId_gcp_coverage(-1 )
          , mId_gcp_cutline(-1 )
          , mTransformParam( -1 )
          , mResamplingMethod( "Lanczos" )
          , mRasterYear( 0 )
          , mRasterScale( 0 )
          , mRasterNodata( -1 )
          , mCompressionMethod( "DEFLATE" )
          , mGCPpointsFileName( "" )
          , mGCPbaseFileName( "" )
          , mRasterFilePath( "" )
          , mRasterFileName( "" )
          , mModifiedRasterFileName( "" )
          , mGcp_coverage_name_base( "" )
          , mError( "" )
          , mSqlDump( false )
          , mDatabaseDump( false )
          , mParseString( ";#;#" )
          , spatialite_cache(nullptr)
          , db_handle(nullptr)
          , mUsed_database_filename(QString::null)
          , mUsed_database_filename_count(0)
      {}

      QString mGCPdatabaseFileName;
      QString mGcp_coverage_name; // file without extention Lower-Case
      int mGcp_srid;
      QString mGcp_points_table_name;
      QgsRasterLayer *mLayer;
      bool mGcp_enabled;
      QgsPoint mInputPoint;
      bool mToPixel;
      int mOrder;
      bool mReCompute;
      int mIdGcp;
      int mId_gcp_coverage;
      int mId_gcp_cutline;
      int mTransformParam;
      QString mResamplingMethod;
      int mRasterYear;
      int mRasterScale;
      int mRasterNodata;
      QString mCompressionMethod;
      QString mGCPpointsFileName;
      QString mGCPbaseFileName;
      QString mRasterFilePath; // Path to the file, without file-name
      QString mRasterFileName; // file-name without path
      QString mModifiedRasterFileName; // Georeferenced File-Name
      QString mGcp_coverage_name_base; // Original Spelling of mGcp_coverage_name
      QString mError;
      bool mSqlDump;
      bool mDatabaseDump;
      QString mParseString;
      QMap<int, QString> gcp_coverages;
      void *spatialite_cache;
      sqlite3* db_handle;
      QString mUsed_database_filename;
      int mUsed_database_filename_count;
    };
    /**
     * Creates a Spatialite-Database storing the Gcp-Points
     *  A 'gcp_convert' TABLE can also be created to use the Spatialite GCP_Transform/Compute logic
     * \note the Spatialite running must be compiled with './configure --enable-gcp=yes'
     *  QGIS can be compiled without this setting since only SQL-Queries are being used
     *  The Tables will only be created/updated after checking that 'SELECT HasGCP()' returns true
     * <a href="https://www.gaia-gis.it/fossil/libspatialite/wiki?name=Ground+Control+Points </a>
     * \note only when 'gcp_enable' is true, will the POINT be used
     *  Based on the value of id_order, only those points will be used
     * @see getGcpConvert
     * @param s_coverage_name name of map to use
     * @return true if database exists
     */
    static bool createGcpDb(GcpDbData* parms_GcpDbData);
    static bool updateGcpDb( GcpDbData* parms_GcpDbData );
    /**
     * Stores the Thin Plate Spline/Polynomial Coefficients of the Gcp-Points
     *  using Spatialite GCP_Transform/Compute
     * \note the Spatialite running must be compiled with './configure --enable-gcp=yes'
     *  QGIS can be compiled without this setting since only SQL-Queries are being used
     *  The Tables will only be created/updated after checking that 'SELECT HasGCP()' returns true
     * <a href="https://www.gaia-gis.it/fossil/libspatialite/wiki?name=Ground+Control+Points </a>
     * \note only when 'gcp_enable' is true, will the POINT be used
     *  Based on the value of id_order, only those points will be used
     * @see getGcpConvert
     * @param s_coverage_name name of map to use
     * @return true if database exists
     */
    static bool updateGcpCompute( GcpDbData* parms_GcpDbData );
    /**
     * Convert Pixel/Map value to Map/Pixel value
     *  using Spatialite GCP_Transform/Compute
     * \note the Spatialite running must be compiled with './configure --enable-gcp=yes'
     *  QGIS can be compiled without this setting since only SQL-Queries are being used
     *  The SQL-Queries will only be called after checking that 'SELECT HasGCP()' returns true
     * <a href="https://www.gaia-gis.it/fossil/libspatialite/wiki?name=Ground+Control+Points </a>
     * \note only PolynomialOrder1, PolynomialOrder2, PolynomialOrder3 and ThinPlateSpline aresupported
     * @see getGcpTransformParam
     * @see updateGcpCompute
     * @param s_coverage_name name of map to search for
     * @param input_point point to convert [not used when id_gcp > 0]
     * @param b_toPixel true=convert Map-Point to Pixel Point ; false: Convert Pixel-Point to Map-Point
     * @param i_order 0-3 [ThinPlateSpline, PolynomialOrder1, PolynomialOrder2, PolynomialOrder3]
     * @param b_reCompute re-calculate value by reading al enable points, othewise read stored values in gcp_convert.
     * @param id_gcp read value from specfic gcp point [input_point will be ignored]
     * @return QgsPoint of result (0,0 when invalid)
     */
    static QgsPoint getGcpConvert( GcpDbData* parms_GcpDbData );
    /**
     * Creates a Spatialite-Database storing the Gcp-Mater-Points
     *  A 'gcp_master' TABLE will be created based on the current srid being used
     *  - this will only contain Map-Points
     * \note a list of known position can be stored in the database
     *  - goal is to make it possible to import these known points into the 'gcp_points' TABLE
     *  When using the Spatialite GCP_Transform/Compute logic is being used
     *  - the Map-Points can be converted to Pixel-Points
     * @param parms_GcpDbData  with full path the the file to be created and srid to use
     * @return true if database exists
     */
    static bool createGcpMasterDb(GcpDbData* parms_GcpDbData);
  private:
    static QStringList createGcpSqlPointsCommands(GcpDbData* parms_GcpDbData, QStringList sa_tables);
    static QStringList createGcpSqlCoveragesCommands(GcpDbData* parms_GcpDbData, QStringList sa_tables);
    static int spatialiteInitEx(GcpDbData* parms_GcpDbData, QString s_database_filename);
    static bool spatialiteShutdown(GcpDbData* parms_GcpDbData);
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
    void openRaster();
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

    // gcps
    void addPoint( const QgsPoint& pixelCoords, const QgsPoint& mapCoords, int id_gcp = -1, bool b_PixelMap=true,
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
     * @param fis (a minus number when inserting to the database, a positive number when adding to a layer)
     */
    void featureAdded_gcp( QgsFeatureId fid );
    /**
     * Reacts to a Spatialite-Database storing the Gcp-Points [gcp_point, gcp_pixel]
     *  - any change in the position will only effect the calling gcp-layer
     *  -- any activity should then only be done once
     * \note all 'fid' < 0 will be ignored
     *  - the changed position in NOT saved to the database
     * @param fis (a minus number when inserting to the database, a positive number when adding to a layer)
     * @param changed_geometry the changed geometry value must be updated in the gcp-list
     */
    void geometryChanged_gcp( QgsFeatureId fid, QgsGeometry& changed_geometry );

    void loadGCPsDialog();
    void saveGCPsDialog();

    // settings
    void showRasterPropertiesDialog();
    void showGeorefConfigDialog();

    // GCPDatabase
    void setLegacyMode();
    void readGCBDb();

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
    void addRaster( const QString& file );
    void loadGTifInQgis( const QString& gtif_file );

    // settings
    void readSettings();
    void writeSettings();

    // gcp points
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
    bool equalGCPlists( const QgsGCPList &list1, const QgsGCPList &list2 );
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
    bool calculateMeanError( double& error ) const;

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
    QString mGCPpointsFileName;
    QgsCoordinateReferenceSystem mProjection;
    QString mPdfOutputFile;
    QString mPdfOutputMapFile;
    double  mUserResX, mUserResY;  // User specified target scale

    // mj10777
    bool bGCPFileName; // Save GCP-Text to a text-file
    bool bPointsFileName; // Save old Points-file to a text-file
    QString mGCPFileName;
    QString mGCPdatabaseFileName;
    QString mGCPbaseFileName;
    int mRasterYear;
    int mRasterScale;
    int mRasterNodata;
    QgsLayerTreeGroup *group_georeferencer;
    QgsLayerTreeGroup *group_gcp_points;
    QgsLayerTreeGroup *group_gcp_cutlines;
    QgsLayerTreeGroup *group_gtif_rasters;
    QgsLayerTreeLayer *group_gtif_raster;
    QgsRasterLayer *mLayer_gtif_raster;
    int mLoadGTifInQgis;
    QMap<int, QString> mGcp_coverages;

    QgsGeorefTransform::TransformParametrisation mTransformParam;
    QgsImageWarper::ResamplingMethod mResamplingMethod;
    QgsGeorefTransform mGeorefTransform;
    QString mCompressionMethod;

    QgisInterface *mIface;

    QgsGCPList mPoints;
    QgsGCPList mInitialPoints;
    QgsMapCanvas *mCanvas;
    QgsRasterLayer *mLayer;
    bool mAgainAddRaster;

    // mj10777
    QString mGcp_points_layername;
    QgsVectorLayer *layer_gcp_points;
    int mEvent_gcp_status;
    int mEvent_point_status;
    int mEvent_pixel_status;
    QString mGcp_pixel_layername;
    QgsVectorLayer *layer_gcp_pixels;
    /**
     * Simplify the commit of the gcp-layers, with checking and commiting as needed
     * @note
     * used during events and Gcp-Layer unloading in clearGCPData()
     * @param gcp_layer either gcp_points or gcp_pixels
     * @param leaveEditable to call startEditing() and set set proper QgsVectorLayerEditBuffer member
     * @return true in case of success of commitChanges()
     */
    bool saveEditsGcp( QgsVectorLayer *gcp_layer, bool leaveEditable );
    QString mCutline_points_layername;
    QgsVectorLayer *layer_cutline_points;
    QString mCutline_linestrings_layername;
    QgsVectorLayer *layer_cutline_linestrings;
    QString mCutline_polygons_layername;
    QgsVectorLayer *layer_cutline_polygons;
    QString mMercator_polygons_layername;
    QgsVectorLayer *layer_mercator_polygons;
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
    bool mGCPsDirty;
    bool mLoadInQgis;

    QgsDockWidget* mDock;
    int messageTimeout();
    QgsSpatiaLiteProviderGcpUtils::GcpDbData* mGcpDbData;
    bool isGcpDb();
    bool createGcpDb(bool b_DatabaseDump=false);
    bool readGcpDb(QString  s_database_filename,bool b_DatabaseDump=false);
    bool updateGcpDb( QString s_coverage_name );
    bool updateGcpCompute( QString s_coverage_name );
    QgsPoint getGcpConvert( QString s_coverage_name, QgsPoint input_point, bool b_toPixel = false, int i_order = 0, bool b_reCompute = true, int id_gcp = -1 );
    bool createGcpMasterDb(QString  s_database_filename, QString  s_gcp_master_tablename="gcp_master", int i_srid=3068);
};

#endif
