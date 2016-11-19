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
class QgsLayerTreeGroup;
class QgsLayerTreeLayer;

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
    void addPoint( const QgsPoint& pixelCoords, const QgsPoint& mapCoords,
                   bool enable = true, bool refreshCanvas = true, int id_gcp = -1 );
    void deleteDataPoint( QPoint pixelCoords );
    void deleteDataPoint( int index );
    void showCoordDialog( const QgsPoint &pixelCoords );

    void selectPoint( QPoint );
    void movePoint( QPoint );
    void releasePoint( QPoint );

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
    int i_gcp_srid;
    QString s_gcp_authid;
    QString s_gcp_description;
    QString s_gcp_points_table_name;
    int id_gcp_coverage;
    bool isGCPDb();
    bool createGCPDb();
    bool updateGCPDb( QString s_coverage_name );
    bool b_spatialite_gcp_enabled;
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
    bool exportLayerDefinition(QgsLayerTreeGroup *group_layer, QString file_path = QString::null);
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
    bool setGcpLayerSettings(QgsVectorLayer *layer_gcp);
    bool setCutlineLayerSettings(QgsVectorLayer *layer_cutline);
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
    QStringList createSvgColors(int i_method=0,bool b_reverse=false,bool b_grey_black=false);

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
    QString mCoverage_Name;
    QString mCoverage_Name_Base;
    int mRasterYear;
    int mRasterScale;
    QgsLayerTreeGroup *group_georeferencer;
    QgsLayerTreeGroup *group_gcp_points;
    QgsLayerTreeGroup *group_gcp_cutlines;
    QgsLayerTreeGroup *group_gtif_rasters;
    QgsLayerTreeLayer *group_gtif_raster;
    QgsRasterLayer *mLayer_gtif_raster;
    int mLoadGTifInQgis;

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
    QString mGcp_pixel_layername;
    QgsVectorLayer *layer_gcp_pixels;
    QString mCutline_points_layername;
    QgsVectorLayer *layer_cutline_points;
    QString mCutline_linestrings_layername;
    QgsVectorLayer *layer_cutline_linestrings;
    QString mCutline_polygons_layername;
    QgsVectorLayer *layer_cutline_polygons;
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
};

#endif
