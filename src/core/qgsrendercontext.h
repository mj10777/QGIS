/***************************************************************************
                              qgsrendercontext.h
                              ------------------
  begin                : March 16, 2008
  copyright            : (C) 2008 by Marco Hugentobler
  email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSRENDERCONTEXT_H
#define QGSRENDERCONTEXT_H

#include "qgis_core.h"
#include "qgis_sip.h"
#include <QColor>
#include <memory>

#include "qgsabstractgeometry.h"
#include "qgscoordinatetransform.h"
#include "qgsexpressioncontext.h"
#include "qgsfeaturefilterprovider.h"
#include "qgsmaptopixel.h"
#include "qgsmapunitscale.h"
#include "qgsrectangle.h"
#include "qgsvectorsimplifymethod.h"
#include "qgsdistancearea.h"

class QPainter;
class QgsAbstractGeometry;
class QgsLabelingEngine;
class QgsMapSettings;


/**
 * \ingroup core
 * Contains information about the context of a rendering operation.
 * The context of a rendering operation defines properties such as
 * the conversion ratio between screen and map units, the extents
 * to be rendered etc.
 **/
class CORE_EXPORT QgsRenderContext
{
  public:
    QgsRenderContext();

    QgsRenderContext( const QgsRenderContext &rh );
    QgsRenderContext &operator=( const QgsRenderContext &rh );

    /**
     * Enumeration of flags that affect rendering operations.
     * \since QGIS 2.14
     */
    enum Flag
    {
      DrawEditingInfo          = 0x01,  //!< Enable drawing of vertex markers for layers in editing mode
      ForceVectorOutput        = 0x02,  //!< Vector graphics should not be cached and drawn as raster images
      UseAdvancedEffects       = 0x04,  //!< Enable layer opacity and blending effects
      UseRenderingOptimization = 0x08,  //!< Enable vector simplification and other rendering optimizations
      DrawSelection            = 0x10,  //!< Whether vector selections should be shown in the rendered map
      DrawSymbolBounds         = 0x20,  //!< Draw bounds of symbols (for debugging/testing)
      RenderMapTile            = 0x40,  //!< Draw map such that there are no problems between adjacent tiles
      Antialiasing             = 0x80,  //!< Use antialiasing while drawing
      RenderPartialOutput      = 0x100, //!< Whether to make extra effort to update map image with partially rendered layers (better for interactive map canvas). Added in QGIS 3.0
      RenderPreviewJob         = 0x200, //!< Render is a 'canvas preview' render, and shortcuts should be taken to ensure fast rendering
    };
    Q_DECLARE_FLAGS( Flags, Flag )

    /**
     * Set combination of flags that will be used for rendering.
     * \since QGIS 2.14
     */
    void setFlags( QgsRenderContext::Flags flags );

    /**
     * Enable or disable a particular flag (other flags are not affected)
     * \since QGIS 2.14
     */
    void setFlag( Flag flag, bool on = true );

    /**
     * Return combination of flags used for rendering.
     * \since QGIS 2.14
     */
    Flags flags() const;

    /**
     * Check whether a particular flag is enabled.
     * \since QGIS 2.14
     */
    bool testFlag( Flag flag ) const;

    /**
     * create initialized QgsRenderContext instance from given QgsMapSettings
     * \since QGIS 2.4
     */
    static QgsRenderContext fromMapSettings( const QgsMapSettings &mapSettings );

    /**
     * Creates a default render context given a pixel based QPainter destination.
     * If no painter is specified or the painter has no device, then a default
     * DPI of 88 will be assumed.
     * \since QGIS 3.0
     */
    static QgsRenderContext fromQPainter( QPainter *painter );

    //getters

    /**
     * Returns the destination QPainter for the render operation.
     * \see setPainter()
    */
    QPainter *painter() {return mPainter;}

    /**
     * Returns the current coordinate transform for the context, or an invalid
     * transform is no coordinate transformation is required.
     */
    QgsCoordinateTransform coordinateTransform() const {return mCoordTransform;}

    /**
     * A general purpose distance and area calculator, capable of performing ellipsoid based calculations.
     * \since QGIS 3.0
     */
    const QgsDistanceArea &distanceArea() const { return mDistanceArea; }

    const QgsRectangle &extent() const {return mExtent;}

    const QgsMapToPixel &mapToPixel() const {return mMapToPixel;}

    /**
     * Returns the scaling factor for the render to convert painter units
     * to physical sizes. This is usually equal to the number of pixels
     * per millimeter.
     * \see setScaleFactor()
     */
    double scaleFactor() const {return mScaleFactor;}

    bool renderingStopped() const {return mRenderingStopped;}

    bool forceVectorOutput() const;

    /**
     * Returns true if advanced effects such as blend modes such be used
     */
    bool useAdvancedEffects() const;

    /**
     * Used to enable or disable advanced effects such as blend modes
     */
    void setUseAdvancedEffects( bool enabled );

    bool drawEditingInformation() const;

    /**
     * Returns the renderer map scale. This will match the desired scale denominator
     * for the rendered map, eg 1000.0 for a 1:1000 map render.
     * \see setRendererScale()
     */
    double rendererScale() const {return mRendererScale;}

    /**
     * Get access to new labeling engine (may be nullptr)
     * \note not available in Python bindings
     */
    QgsLabelingEngine *labelingEngine() const { return mLabelingEngine; } SIP_SKIP

    QColor selectionColor() const { return mSelectionColor; }

    /**
     * Returns true if vector selections should be shown in the rendered map
     * \returns true if selections should be shown
     * \see setShowSelection
     * \see selectionColor
     * \since QGIS v2.4
     */
    bool showSelection() const;

    //setters

    //! Sets coordinate transformation.
    void setCoordinateTransform( const QgsCoordinateTransform &t );
    void setMapToPixel( const QgsMapToPixel &mtp ) {mMapToPixel = mtp;}
    void setExtent( const QgsRectangle &extent ) {mExtent = extent;}

    void setDrawEditingInformation( bool b );

    void setRenderingStopped( bool stopped ) {mRenderingStopped = stopped;}

    /**
     * A general purpose distance and area calculator, capable of performing ellipsoid based calculations.
     * Will be used to convert meter distances to active MapUnit values for QgsUnitTypes::RenderMetersInMapUnits
     * \since QGIS 3.0
     */
    void setDistanceArea( const QgsDistanceArea &distanceArea ) {mDistanceArea = distanceArea ;}

    /**
     * Sets the scaling factor for the render to convert painter units
     * to physical sizes. This should usually be equal to the number of pixels
     * per millimeter.
     * \see scaleFactor()
     */
    void setScaleFactor( double factor ) {mScaleFactor = factor;}

    /**
     * Sets the renderer map scale. This should match the desired scale denominator
     * for the rendered map, eg 1000.0 for a 1:1000 map render.
     * \see rendererScale()
     */
    void setRendererScale( double scale ) {mRendererScale = scale;}

    /**
     * Sets the destination QPainter for the render operation. Ownership of the painter
     * is not transferred and the QPainter destination must stay alive for the duration
     * of any rendering operations.
     * \see painter()
     */
    void setPainter( QPainter *p ) {mPainter = p;}

    void setForceVectorOutput( bool force );

    /**
     * Assign new labeling engine
     * \note not available in Python bindings
     */
    void setLabelingEngine( QgsLabelingEngine *engine2 ) { mLabelingEngine = engine2; } SIP_SKIP
    void setSelectionColor( const QColor &color ) { mSelectionColor = color; }

    /**
     * Sets whether vector selections should be shown in the rendered map
     * \param showSelection set to true if selections should be shown
     * \see showSelection
     * \see setSelectionColor
     * \since QGIS v2.4
     */
    void setShowSelection( const bool showSelection );

    /**
     * Returns true if the rendering optimization (geometry simplification) can be executed
     */
    bool useRenderingOptimization() const;

    void setUseRenderingOptimization( bool enabled );

    //! Added in QGIS v2.4
    const QgsVectorSimplifyMethod &vectorSimplifyMethod() const { return mVectorSimplifyMethod; }
    void setVectorSimplifyMethod( const QgsVectorSimplifyMethod &simplifyMethod ) { mVectorSimplifyMethod = simplifyMethod; }

    /**
     * Sets the expression context. This context is used for all expression evaluation
     * associated with this render context.
     * \see expressionContext()
     * \since QGIS 2.12
     */
    void setExpressionContext( const QgsExpressionContext &context ) { mExpressionContext = context; }

    /**
     * Gets the expression context. This context should be used for all expression evaluation
     * associated with this render context.
     * \see setExpressionContext()
     * \since QGIS 2.12
     */
    QgsExpressionContext &expressionContext() { return mExpressionContext; }

    /**
     * Gets the expression context (const version). This context should be used for all expression evaluation
     * associated with this render context.
     * \see setExpressionContext()
     * \since QGIS 2.12
     * \note not available in Python bindings
     */
    const QgsExpressionContext &expressionContext() const { return mExpressionContext; } SIP_SKIP

    //! Returns pointer to the unsegmentized geometry
    const QgsAbstractGeometry *geometry() const { return mGeometry; }
    //! Sets pointer to original (unsegmentized) geometry
    void setGeometry( const QgsAbstractGeometry *geometry ) { mGeometry = geometry; }

    /**
     * Set a filter feature provider used for additional filtering of rendered features.
     * \param ffp the filter feature provider
     * \since QGIS 2.14
     * \see featureFilterProvider()
     */
    void setFeatureFilterProvider( const QgsFeatureFilterProvider *ffp );

    /**
     * Get the filter feature provider used for additional filtering of rendered features.
     * \returns the filter feature provider
     * \since QGIS 2.14
     * \see setFeatureFilterProvider()
     */
    const QgsFeatureFilterProvider *featureFilterProvider() const;

    /**
     * Sets the segmentation tolerance applied when rendering curved geometries
    \param tolerance the segmentation tolerance*/
    void setSegmentationTolerance( double tolerance ) { mSegmentationTolerance = tolerance; }
    //! Gets the segmentation tolerance applied when rendering curved geometries
    double segmentationTolerance() const { return mSegmentationTolerance; }

    /**
     * Sets segmentation tolerance type (maximum angle or maximum difference between curve and approximation)
    \param type the segmentation tolerance typename*/
    void setSegmentationToleranceType( QgsAbstractGeometry::SegmentationToleranceType type ) { mSegmentationToleranceType = type; }
    //! Gets segmentation tolerance type (maximum angle or maximum difference between curve and approximation)
    QgsAbstractGeometry::SegmentationToleranceType segmentationToleranceType() const { return mSegmentationToleranceType; }

    // Conversions

    /**
     * Converts a size from the specified units to painter units (pixels). The conversion respects the limits
     * specified by the optional scale parameter.
     * \since QGIS 3.0
     * \see convertToMapUnits()
     */
    double convertToPainterUnits( double size, QgsUnitTypes::RenderUnit unit, const QgsMapUnitScale &scale = QgsMapUnitScale() ) const;

    /**
     * Converts a size from the specified units to map units. The conversion respects the limits
     * specified by the optional scale parameter.
     * \since QGIS 3.0
     * \see convertToPainterUnits()
     */
    double convertToMapUnits( double size, QgsUnitTypes::RenderUnit unit, const QgsMapUnitScale &scale = QgsMapUnitScale() ) const;

    /**
     * Converts a size from map units to the specified units.
     * \since QGIS 3.0
     * \see convertToMapUnits()
     */
    double convertFromMapUnits( double sizeInMapUnits, QgsUnitTypes::RenderUnit outputUnit ) const;

    /**
     * Convert meter distances to active MapUnit values for QgsUnitTypes::RenderMetersInMapUnits
     * \note
      * When the sourceCrs() is geographic, the center of the Extent will be used
     * \since QGIS 3.0
     */
    double convertMetersToMapUnits( double meters ) const;

  private:

    Flags mFlags;

    //! Painter for rendering operations
    QPainter *mPainter = nullptr;

    //! For transformation between coordinate systems. Can be invalid if on-the-fly reprojection is not used
    QgsCoordinateTransform mCoordTransform;

    /**
     * A general purpose distance and area calculator, capable of performing ellipsoid based calculations.
     * Will be used to convert meter distances to active MapUnit values for QgsUnitTypes::RenderMetersInMapUnits
     * \since QGIS 3.0
     */
    QgsDistanceArea mDistanceArea;

    QgsRectangle mExtent;

    QgsMapToPixel mMapToPixel;

    //! True if the rendering has been canceled
    bool mRenderingStopped = false;

    //! Factor to scale line widths and point marker sizes
    double mScaleFactor = 1.0;

    //! Map scale
    double mRendererScale = 1.0;

    //! Newer labeling engine implementation (can be nullptr)
    QgsLabelingEngine *mLabelingEngine = nullptr;

    //! Color used for features that are marked as selected
    QColor mSelectionColor;

    //! Simplification object which holds the information about how to simplify the features for fast rendering
    QgsVectorSimplifyMethod mVectorSimplifyMethod;

    //! Expression context
    QgsExpressionContext mExpressionContext;

    //! Pointer to the (unsegmentized) geometry
    const QgsAbstractGeometry *mGeometry = nullptr;

    //! The feature filter provider
    std::unique_ptr< QgsFeatureFilterProvider > mFeatureFilterProvider;

    double mSegmentationTolerance = M_PI_2 / 90;

    QgsAbstractGeometry::SegmentationToleranceType mSegmentationToleranceType = QgsAbstractGeometry::MaximumAngle;
};

Q_DECLARE_OPERATORS_FOR_FLAGS( QgsRenderContext::Flags )

#endif
