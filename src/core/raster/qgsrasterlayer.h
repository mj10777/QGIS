/***************************************************************************
                        qgsrasterlayer.h  -  description
                              -------------------
 begin                : Fri Jun 28 2002
 copyright            : (C) 2004 by T.Sutton, Gary E.Sherman, Steve Halasz
 email                : tim@linfiniti.com
***************************************************************************/
/*
 * Peter J. Ersts - contributed to the refactoring and maintenance of this class
 * B. Morley - added functions to convert this class to a data provider interface
 * Frank Warmerdam - contributed bug fixes and migrated class to use all GDAL_C_API calls
 */
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSRASTERLAYER_H
#define QGSRASTERLAYER_H

#include "qgis_core.h"
#include "qgis_sip.h"
#include <QColor>
#include <QDateTime>
#include <QList>
#include <QMap>
#include <QPair>
#include <QVector>

#include "qgis.h"
#include "qgsmaplayer.h"
#include "qgsraster.h"
#include "qgsrasterdataprovider.h"
#include "qgsrasterpipe.h"
#include "qgsrasterviewport.h"
#include "qgsrasterminmaxorigin.h"
#include "qgscontrastenhancement.h"

class QgsMapToPixel;
class QgsRasterRenderer;
class QgsRectangle;
class QImage;
class QLibrary;
class QPixmap;
class QSlider;

typedef QList < QPair< QString, QColor > > QgsLegendColorList;

/**
 * \ingroup core
 *  This class provides qgis with the ability to render raster datasets
 *  onto the mapcanvas.
 *
 *  The qgsrasterlayer class makes use of gdal for data io, and thus supports
 *  any gdal supported format. The constructor attempts to infer what type of
 *  file (LayerType) is being opened - not in terms of the file format (tif, ascii grid etc.)
 *  but rather in terms of whether the image is a GRAYSCALE, PaletteD or Multiband,
 *
 *  Within the three allowable raster layer types, there are 8 permutations of
 *  how a layer can actually be rendered. These are defined in the DrawingStyle enum
 *  and consist of:
 *
 *  SingleBandGray -> a GRAYSCALE layer drawn as a range of gray colors (0-255)
 *  SingleBandPseudoColor -> a GRAYSCALE layer drawn using a pseudocolor algorithm
 *  PalettedSingleBandGray -> a PaletteD layer drawn in gray scale (using only one of the color components)
 *  PalettedSingleBandPseudoColor -> a PaletteD layer having only one of its color components rendered as pseudo color
 *  PalettedMultiBandColor -> a PaletteD image where the bands contains 24bit color info and 8 bits is pulled out per color
 *  MultiBandSingleBandGray -> a layer containing 2 or more bands, but using only one band to produce a grayscale image
 *  MultiBandSingleBandPseudoColor -> a layer containing 2 or more bands, but using only one band to produce a pseudocolor image
 *  MultiBandColor -> a layer containing 2 or more bands, mapped to the three RGBcolors. In the case of a multiband with only two bands, one band will have to be mapped to more than one color
 *
 *  Each of the above mentioned drawing styles is implemented in its own draw* function.
 *  Some of the drawing styles listed above require statistics about the layer such
 *  as the min / max / mean / stddev etc. statistics for a band can be gathered using the
 *  bandStatistics function. Note that statistics gathering is a slow process and
 *  every effort should be made to call this function as few times as possible. For this
 *  reason, qgsraster has a vector class member to store stats for each band. The
 *  constructor initializes this vector on startup, but only populates the band name and
 *  number fields.
 *
 *  Note that where bands are of gdal 'undefined' type, their values may exceed the
 *  renderable range of 0-255. Because of this a linear scaling histogram enhanceContrast is
 *  applied to undefined layers to normalise the data into the 0-255 range.
 *
 *  A qgsrasterlayer band can be referred to either by name or by number (base=1). It
 *  should be noted that band names as stored in datafiles may not be unique, and
 *  so the rasterlayer class appends the band number in brackets behind each band name.
 *
 *  Sample usage of the QgsRasterLayer class:
 *
 * \code
 *     QString myFileNameQString = "/path/to/file";
 *     QFileInfo myFileInfo(myFileNameQString);
 *     QString myBaseNameQString = myFileInfo.baseName();
 *     QgsRasterLayer *myRasterLayer = new QgsRasterLayer(myFileNameQString, myBaseNameQString);
 *
 * \endcode
 *
 *  In order to automate redrawing of a raster layer, you should like it to a map canvas like this :
 *
 * \code
 *     QObject::connect( myRasterLayer, SIGNAL(repaintRequested()), mapCanvas, SLOT(refresh()) );
 * \endcode
 *
 * Once a layer has been created you can find out what type of layer it is (GrayOrUndefined, Palette or Multiband):
 *
 * \code
 *    if (rasterLayer->rasterType()==QgsRasterLayer::Multiband)
 *    {
 *      //do something
 *    }
 *    else if (rasterLayer->rasterType()==QgsRasterLayer::Palette)
 *    {
 *      //do something
 *    }
 *    else // QgsRasterLayer::GrayOrUndefined
 *    {
 *      //do something.
 *    }
 * \endcode
 *
 *  Raster layers can also have an arbitrary level of transparency defined, and have their
 *  color palettes inverted using the setTransparency and setInvertHistogram methods.
 *
 *  Pseudocolor images can have their output adjusted to a given number of standard
 *  deviations using the setStandardDeviations method.
 *
 *  The final area of functionality you may be interested in is band mapping. Band mapping
 *  allows you to choose arbitrary band -> color mappings and is applicable only to Palette
 *  and Multiband rasters, There are four mappings that can be made: red, green, blue and gray.
 *  Mappings are non-exclusive. That is a given band can be assigned to no, some or all
 *  color mappings. The constructor sets sensible defaults for band mappings but these can be
 *  overridden at run time using the setRedBandName, setGreenBandName, setBlueBandName and setGrayBandName
 *  methods.
 */

class CORE_EXPORT QgsRasterLayer : public QgsMapLayer
{
    Q_OBJECT
  public:

    //! \brief Default sample size (number of pixels) for estimated statistics/histogram calculation
    static const double SAMPLE_SIZE;

    //! \brief Default enhancement algorithm for single band raster
    static const QgsContrastEnhancement::ContrastEnhancementAlgorithm SINGLE_BAND_ENHANCEMENT_ALGORITHM;

    //! \brief Default enhancement algorithm for multiple band raster of type Byte
    static const QgsContrastEnhancement::ContrastEnhancementAlgorithm MULTIPLE_BAND_SINGLE_BYTE_ENHANCEMENT_ALGORITHM;

    //! \brief Default enhancement algorithm for multiple band raster of type different from Byte
    static const QgsContrastEnhancement::ContrastEnhancementAlgorithm MULTIPLE_BAND_MULTI_BYTE_ENHANCEMENT_ALGORITHM;

    //! \brief Default enhancement limits for single band raster
    static const QgsRasterMinMaxOrigin::Limits SINGLE_BAND_MIN_MAX_LIMITS;

    //! \brief Default enhancement limits for multiple band raster of type Byte
    static const QgsRasterMinMaxOrigin::Limits MULTIPLE_BAND_SINGLE_BYTE_MIN_MAX_LIMITS;

    //! \brief Default enhancement limits for multiple band raster of type different from Byte
    static const QgsRasterMinMaxOrigin::Limits MULTIPLE_BAND_MULTI_BYTE_MIN_MAX_LIMITS;

    //! \brief Constructor. Provider is not set.
    QgsRasterLayer();

    /**
     * Setting options for loading raster layers.
     * \since QGIS 3.0
     */
    struct LayerOptions
    {

      /**
       * Constructor for LayerOptions.
       */
      explicit LayerOptions( bool loadDefaultStyle = true )
        : loadDefaultStyle( loadDefaultStyle )
      {}

      //! Set to true if the default layer style should be loaded
      bool loadDefaultStyle = true;
    };

    /**
     * \brief This is the constructor for the RasterLayer class.
     *
     * The main tasks carried out by the constructor are:
     *
     * -Load the rasters default style (.qml) file if it exists
     *
     * -Populate the RasterStatsVector with initial values for each band.
     *
     * -Calculate the layer extents
     *
     * -Determine whether the layer is gray, paletted or multiband.
     *
     * -Assign sensible defaults for the red, green, blue and gray bands.
     *
     * -
     * */
    explicit QgsRasterLayer( const QString &uri,
                             const QString &baseName = QString(),
                             const QString &providerKey = "gdal",
                             const QgsRasterLayer::LayerOptions &options = QgsRasterLayer::LayerOptions() );

    ~QgsRasterLayer();

    /**
     * Returns a new instance equivalent to this one. A new provider is
     *  created for the same data source and renderer is cloned too.
     * \returns a new layer instance
     * \since QGIS 3.0
     */
    virtual QgsRasterLayer *clone() const override SIP_FACTORY;

    //! \brief This enumerator describes the types of shading that can be used
    enum ColorShadingAlgorithm
    {
      UndefinedShader,
      PseudoColorShader,
      FreakOutShader,
      ColorRampShader,
      UserDefinedShader
    };

    //! \brief This enumerator describes the type of raster layer
    enum LayerType
    {
      GrayOrUndefined,
      Palette,
      Multiband,
      ColorLayer
    };

    /**
     * This helper checks to see whether the file name appears to be a valid
     *  raster file name.  If the file name looks like it could be valid,
     *  but some sort of error occurs in processing the file, the error is
     *  returned in retError.
     */
    static bool isValidRasterFileName( const QString &fileNameQString, QString &retError );
    static bool isValidRasterFileName( const QString &fileNameQString );

    //! Return time stamp for given file name
    static QDateTime lastModified( const QString   &name );

    //! [ data provider interface ] Set the data provider
    void setDataProvider( const QString &provider );

    //! \brief  Accessor for raster layer type (which is a read only property)
    LayerType rasterType() { return mRasterType; }

    //! Set raster renderer. Takes ownership of the renderer object
    void setRenderer( QgsRasterRenderer *renderer SIP_TRANSFER );
    QgsRasterRenderer *renderer() const { return mPipe.renderer(); }

    //! Set raster resample filter. Takes ownership of the resample filter object
    QgsRasterResampleFilter *resampleFilter() const { return mPipe.resampleFilter(); }

    QgsBrightnessContrastFilter *brightnessFilter() const { return mPipe.brightnessFilter(); }
    QgsHueSaturationFilter *hueSaturationFilter() const { return mPipe.hueSaturationFilter(); }

    //! Get raster pipe
    QgsRasterPipe *pipe() { return &mPipe; }

    //! \brief Accessor that returns the width of the (unclipped) raster
    int width() const;

    //! \brief Accessor that returns the height of the (unclipped) raster
    int height() const;

    //! \brief Get the number of bands in this layer
    int bandCount() const;

    //! \brief Get the name of a band given its number
    QString bandName( int bandNoInt ) const;

    QgsRasterDataProvider *dataProvider() override;

    /**
     * Returns the data provider in a const-correct manner
      \note available in Python bindings as constDataProvider()
     */
    const QgsRasterDataProvider *dataProvider() const SIP_PYNAME( constDataProvider ) override;

    //! Synchronises with changes in the datasource
    virtual void reload() override;

    /**
     * Return new instance of QgsMapLayerRenderer that will be used for rendering of given context
     * \since QGIS 2.4
     */
    virtual QgsMapLayerRenderer *createMapRenderer( QgsRenderContext &rendererContext ) override SIP_FACTORY;

    //! \brief This is an overloaded version of the draw() function that is called by both draw() and thumbnailAsPixmap
    void draw( QPainter *theQPainter,
               QgsRasterViewPort *myRasterViewPort,
               const QgsMapToPixel *qgsMapToPixel = nullptr );

    //! Returns a list with classification items (Text and color)
    QgsLegendColorList legendSymbologyItems() const;

    virtual bool isSpatial() const override { return true; }

    QString htmlMetadata() const override;

    //! \brief Get an 100x100 pixmap of the color palette. If the layer has no palette a white pixmap will be returned
    QPixmap paletteAsPixmap( int bandNumber = 1 );

    //! \brief [ data provider interface ] Which provider is being used for this Raster Layer?
    QString providerType() const;

    //! \brief Returns the number of raster units per each raster pixel in X axis. In a world file, this is normally the first row (without the sign)
    double rasterUnitsPerPixelX() const;
    //! \brief Returns the number of raster units per each raster pixel in Y axis. In a world file, this is normally the first row (without the sign)
    double rasterUnitsPerPixelY() const;

    /**
     * \brief Set contrast enhancement algorithm
     *  \param algorithm Contrast enhancement algorithm
     *  \param limits Limits
     *  \param extent Extent used to calculate limits, if empty, use full layer extent
     *  \param sampleSize Size of data sample to calculate limits, if 0, use full resolution
     *  \param generateLookupTableFlag Generate lookup table. */


    void setContrastEnhancement( QgsContrastEnhancement::ContrastEnhancementAlgorithm algorithm,
                                 QgsRasterMinMaxOrigin::Limits limits = QgsRasterMinMaxOrigin::MinMax,
                                 const QgsRectangle &extent = QgsRectangle(),
                                 int sampleSize = QgsRasterLayer::SAMPLE_SIZE,
                                 bool generateLookupTableFlag = true );

    /**
     * \brief Refresh contrast enhancement with new extent.
     *  \note not available in Python bindings
     */
    // Used by QgisApp::legendLayerStretchUsingCurrentExtent()
    void refreshContrastEnhancement( const QgsRectangle &extent ) SIP_SKIP;

    /**
     * \brief Refresh renderer with new extent, if needed
     *  \note not available in Python bindings
     */
    // Used by QgsRasterLayerRenderer
    void refreshRendererIfNeeded( QgsRasterRenderer *rasterRenderer, const QgsRectangle &extent ) SIP_SKIP;

    /**
     * \brief Return default contrast enhancemnt settings for that type of raster.
     *  \note not available in Python bindings
     */
    bool defaultContrastEnhancementSettings(
      QgsContrastEnhancement::ContrastEnhancementAlgorithm &myAlgorithm,
      QgsRasterMinMaxOrigin::Limits &myLimits ) const SIP_SKIP;

    //! \brief Set default contrast enhancement
    void setDefaultContrastEnhancement();

    //! \brief Returns the sublayers of this layer - Useful for providers that manage their own layers, such as WMS
    virtual QStringList subLayers() const override;

    /**
     * \brief Draws a preview of the rasterlayer into a QImage
     \since QGIS 2.4 */
    QImage previewAsImage( QSize size, const QColor &bgColor = Qt::white,
                           QImage::Format format = QImage::Format_ARGB32_Premultiplied );

    /**
     * Reorders the *previously selected* sublayers of this layer from bottom to top
     *
     * (Useful for providers that manage their own layers, such as WMS)
     *
     */
    virtual void setLayerOrder( const QStringList &layers ) override;

    /**
     * Set the visibility of the given sublayer name
     */
    virtual void setSubLayerVisibility( const QString &name, bool vis ) override;

    //! Time stamp of data source in the moment when data/metadata were loaded by provider
    virtual QDateTime timestamp() const override;

  public slots:
    void showStatusMessage( const QString &message );

  protected:
    //! \brief Read the symbology for the current layer from the Dom node supplied
    bool readSymbology( const QDomNode &node, QString &errorMessage, const QgsReadWriteContext &context ) override;

    //! \brief Read the style information for the current layer from the Dom node supplied
    bool readStyle( const QDomNode &node, QString &errorMessage, const QgsReadWriteContext &context ) override;

    //! \brief Reads layer specific state from project file Dom node
    bool readXml( const QDomNode &layer_node, const QgsReadWriteContext &context ) override;

    //! \brief Write the symbology for the layer into the docment provided
    bool writeSymbology( QDomNode &, QDomDocument &doc, QString &errorMessage, const QgsReadWriteContext &context ) const override;

    //! \brief Write the style for the layer into the docment provided
    bool writeStyle( QDomNode &node, QDomDocument &doc, QString &errorMessage, const QgsReadWriteContext &context ) const override;

    //! \brief Write layer specific state to project file Dom node
    bool writeXml( QDomNode &layer_node, QDomDocument &doc, const QgsReadWriteContext &context ) const override;

  private:
    //! \brief Initialize default values
    void init();

    //! \brief Close data provider and clear related members
    void closeDataProvider();

    //! \brief Update the layer if it is outdated
    bool update();

    //! Sets corresponding renderer for style
    void setRendererForDrawingStyle( QgsRaster::DrawingStyle drawingStyle );

    void setContrastEnhancement( QgsContrastEnhancement::ContrastEnhancementAlgorithm algorithm,
                                 QgsRasterMinMaxOrigin::Limits limits,
                                 const QgsRectangle &extent,
                                 int sampleSize,
                                 bool generateLookupTableFlag,
                                 QgsRasterRenderer *rasterRenderer );

    void computeMinMax( int band,
                        const QgsRasterMinMaxOrigin &mmo,
                        QgsRasterMinMaxOrigin::Limits limits,
                        const QgsRectangle &extent,
                        int sampleSize,
                        double &min, double &max );

    //! \brief  Constant defining flag for XML and a constant that signals property not used
    const QString QSTRING_NOT_SET;
    const QString TRSTRING_NOT_SET;

    //! Pointer to data provider
    QgsRasterDataProvider *mDataProvider = nullptr;

    //! [ data provider interface ] Timestamp, the last modified time of the data source when the layer was created
    QDateTime mLastModified;

    QgsRasterViewPort mLastViewPort;

    //! [ data provider interface ] Data provider key
    QString mProviderKey;

    LayerType mRasterType;

    QgsRasterPipe mPipe;

    //! To save computations and possible infinite cycle of notifications
    QgsRectangle mLastRectangleUsedByRefreshContrastEnhancementIfNeeded;
};

// clazy:excludeall=qstring-allocations

#endif
