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
#ifndef MAPCOORDSDIALOG_H
#define MAPCOORDSDIALOG_H

#include <QDialog>
#include <QMouseEvent>

#include "qgsmaptoolemitpoint.h"
#include "qgssnapindicator.h"
#include "qgssnappingutils.h"
#include "qgspointxy.h"
#include "qgsmapcanvas.h"

#include "ui_qgsmapcoordsdialogbase.h"

class QPushButton;

class QgsGeorefMapToolEmitPoint : public QgsMapTool
{
    Q_OBJECT

  public:
    explicit QgsGeorefMapToolEmitPoint( QgsMapCanvas *canvas )
      : QgsMapTool( canvas )
    {
      mSnapIndicator.reset( new QgsSnapIndicator( canvas ) );
    }

    void canvasMoveEvent( QgsMapMouseEvent *e ) override
    {
      mSnapIndicator->setMatch( mapPointMatch( e ) );
    }

    void canvasPressEvent( QgsMapMouseEvent *e ) override
    {
      QgsPointLocator::Match m = mapPointMatch( e );
      emit canvasClicked( m.isValid() ? m.point() : toMapCoordinates( e->pos() ), e->button() );
    }

    void canvasReleaseEvent( QgsMapMouseEvent *e ) override
    {
      QgsMapTool::canvasReleaseEvent( e );
      emit mouseReleased();
    }

    void deactivate() override
    {
      mSnapIndicator->setMatch( QgsPointLocator::Match() );

      QgsMapTool::deactivate();
    }

  signals:
    void canvasClicked( const QgsPointXY &point, Qt::MouseButton button );
    void mouseReleased();

  private:

    QgsPointLocator::Match mapPointMatch( QMouseEvent *e )
    {
      QgsPointXY pnt = toMapCoordinates( e->pos() );
      return canvas()->snappingUtils()->snapToMap( pnt );
    }

    <<< <<< < HEAD
    QgsVertexMarker *mSnappingMarker;
    == == == =
      std::unique_ptr<QgsSnapIndicator> mSnapIndicator;
    >>> >>> > upstream_qgis / master32.spatialite_provider
};

class QgsMapCoordsDialog : public QDialog, private Ui::QgsMapCoordsDialogBase
{
    Q_OBJECT

  public:
    QgsMapCoordsDialog( QgsMapCanvas *qgisCanvas, const QgsPointXY &pixelCoords, int id_gcp = -1, QWidget *parent = nullptr );
    ~QgsMapCoordsDialog();

  private slots:
    void buttonBox_accepted();

    void setToolEmitPoint( bool isEnable );

    void maybeSetXY( const QgsPointXY &, Qt::MouseButton );
    void updateOK();
    void setPrevTool();

  signals:
    void pointAdded( const QgsPointXY &, const QgsPointXY &, const int &, const bool & );

  private:
    double dmsToDD( const QString &dms );

    QPushButton *mPointFromCanvasPushButton;

    QgsGeorefMapToolEmitPoint *mToolEmitPoint = nullptr;
    QgsMapTool *mPrevMapTool = nullptr;
    QgsMapCanvas *mQgisCanvas = nullptr;

    QgsPointXY mPixelCoords;
    QgsPointXY mMapCoords;
    bool mPixelMap = false;
    int mIdGcp = 0;
};

#endif
