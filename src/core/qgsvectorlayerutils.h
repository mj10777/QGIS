/***************************************************************************
  qgsvectorlayerutils.h
  ---------------------
  Date                 : October 2016
  Copyright            : (C) 2016 by Nyall Dawson
  Email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSVECTORLAYERUTILS_H
#define QGSVECTORLAYERUTILS_H

#include "qgis_core.h"
#include "qgsvectorlayer.h"
#include "qgsgeometry.h"

/**
 * \ingroup core
 * \class QgsVectorLayerUtils
 * \brief Contains utility methods for working with QgsVectorLayers.
 *
 * \since QGIS 3.0
 */

class CORE_EXPORT QgsVectorLayerUtils
{
  public:

    /**
     * \ingroup core
     * \class QgsDuplicateFeatureContext
     * \brief Contains mainly the QMap with QgsVectorLayer and QgsFeatureIds do list all the duplicated features
     *
     * \since QGIS 3.0
     */
    class CORE_EXPORT QgsDuplicateFeatureContext
    {
      public:

        //! Constructor for QgsDuplicateFeatureContext
        QgsDuplicateFeatureContext() = default;

        /**
         * Returns all the layers on which features have been duplicated
         * \since QGIS 3.0
         */
        QList<QgsVectorLayer *> layers() const;

        /**
         * Returns the duplicated features in the given layer
         * \since QGIS 3.0
         */
        QgsFeatureIds duplicatedFeatures( QgsVectorLayer *layer ) const;


      private:
        QMap<QgsVectorLayer *, QgsFeatureIds> mDuplicatedFeatures;
        friend class QgsVectorLayerUtils;

        /**
         * To set info about duplicated features to the function feedback (layout and ids)
         * \since QGIS 3.0
         */
        void setDuplicatedFeatures( QgsVectorLayer *layer, QgsFeatureIds ids );
    };

    /**
     * Returns true if the specified value already exists within a field. This method can be used to test for uniqueness
     * of values inside a layer's attributes. An optional list of ignored feature IDs can be provided, if so, any features
     * with IDs within this list are ignored when testing for existence of the value.
     * \see createUniqueValue()
     */
    static bool valueExists( const QgsVectorLayer *layer, int fieldIndex, const QVariant &value, const QgsFeatureIds &ignoreIds = QgsFeatureIds() );

    /**
     * Returns a new attribute value for the specified field index which is guaranteed to be unique. The optional seed
     * value can be used as a basis for generated values.
     * \see valueExists()
     */
    static QVariant createUniqueValue( const QgsVectorLayer *layer, int fieldIndex, const QVariant &seed = QVariant() );

    /**
     * Tests an attribute value to check whether it passes all constraints which are present on the corresponding field.
     * Returns true if the attribute value is valid for the field. Any constraint failures will be reported in the errors argument.
     * If the strength or origin parameter is set then only constraints with a matching strength/origin will be checked.
     */
    static bool validateAttribute( const QgsVectorLayer *layer, const QgsFeature &feature, int attributeIndex, QStringList &errors SIP_OUT,
                                   QgsFieldConstraints::ConstraintStrength strength = QgsFieldConstraints::ConstraintStrengthNotSet,
                                   QgsFieldConstraints::ConstraintOrigin origin = QgsFieldConstraints::ConstraintOriginNotSet );

    /**
     * Creates a new feature ready for insertion into a layer. Default values and constraints
     * (e.g., unique constraints) will automatically be handled. An optional attribute map can be
     * passed for the new feature to copy as many attribute values as possible from the map,
     * assuming that they respect the layer's constraints. Note that the created feature is not
     * automatically inserted into the layer.
     */
    static QgsFeature createFeature( QgsVectorLayer *layer,
                                     const QgsGeometry &geometry = QgsGeometry(),
                                     const QgsAttributeMap &attributes = QgsAttributeMap(),
                                     QgsExpressionContext *context = nullptr );

    /**
     * Duplicates a feature and it's children (one level deep). It calls CreateFeature, so
     * default values and constraints (e.g., unique constraints) will automatically be handled.
     * The duplicated feature will be automatically inserted into the layer.
     * \a depth the higher this number the deeper the level - With depth > 0 the children of the feature are not duplicated
     * \a duplicateFeatureContext stores all the layers and the featureids of the duplicated features (incl. children)
     * \since QGIS 3.0
     */
    static QgsFeature duplicateFeature( QgsVectorLayer *layer, const QgsFeature &feature, QgsProject *project, int depth, QgsDuplicateFeatureContext &duplicateFeatureContext SIP_OUT );

};


#endif // QGSVECTORLAYERUTILS_H
