/***************************************************************************
    qgsgeometrymultipartcheck.h
    ---------------------
    begin                : September 2015
    copyright            : (C) 2014 by Sandro Mani / Sourcepole AG
    email                : smani at sourcepole dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define SIP_NO_FILE

#ifndef QGS_GEOMETRY_MULTIPART_CHECK_H
#define QGS_GEOMETRY_MULTIPART_CHECK_H

#include "qgsgeometrycheck.h"

class ANALYSIS_EXPORT QgsGeometryMultipartCheck : public QgsGeometryCheck
{
    Q_OBJECT

  public:
    explicit QgsGeometryMultipartCheck( QgsGeometryCheckerContext *context )
      : QgsGeometryCheck( FeatureCheck, {QgsWkbTypes::PointGeometry, QgsWkbTypes::LineGeometry, QgsWkbTypes::PolygonGeometry}, context ) {}
    void collectErrors( QList<QgsGeometryCheckError *> &errors, QStringList &messages, QAtomicInt *progressCounter = nullptr, const QMap<QString, QgsFeatureIds> &ids = QMap<QString, QgsFeatureIds>() ) const override;
    void fixError( QgsGeometryCheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes ) const override;
    QStringList getResolutionMethods() const override;
    QString errorDescription() const override { return tr( "Multipart object with only one feature" ); }
    QString errorName() const override { return QStringLiteral( "QgsGeometryMultipartCheck" ); }

    enum ResolutionMethod { ConvertToSingle, RemoveObject, NoChange };
};

#endif // QGS_GEOMETRY_MULTIPART_CHECK_H
