/***************************************************************************
     mapcoordsdialog.cpp
     --------------------------------------
    Date                 : 2005
    Copyright            : (C) 2005 by Lars Luthman
    Email                : larsl at users dot sourceforge dot net
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <QValidator>
#include <QPushButton>

#include "qgsmapcanvas.h"

#include "qgsgeorefvalidators.h"
#include "qgsmapcoordsdialog.h"
#include "qgssettings.h"

QgsMapCoordsDialog::QgsMapCoordsDialog( QgsMapCanvas *qgisCanvas, const QgsPointXY &pixelCoords, int id_gcp, QWidget *parent )
  : QDialog( parent, Qt::Dialog )
  , mQgisCanvas( qgisCanvas )
  , mPixelCoords( pixelCoords )
  , mIdGcp( id_gcp )
{
  mMapCoords.setX( 0.0 );
  mMapCoords.setY( 0.0 );
  if ( qgisCanvas->objectName() == "georefCanvas" )
  {
    // Map-Point to Pixel-Point [receiving a Map-Point and a Pixel-Canvas to select a Pixel-Point from]
    mPixelMap = true;
    mMapCoords = mPixelCoords;
    mPixelCoords.setX( 0.0 );
    mPixelCoords.setY( 0.0 );
  }
  qDebug() << QString( "QgsMapCoordsDialog[%1]  dialog- mPixelMap[%2] " ).arg( qgisCanvas->objectName() ).arg( mPixelMap );
  setupUi( this );
  connect( buttonBox, &QDialogButtonBox::accepted, this, &QgsMapCoordsDialog::buttonBox_accepted );

  QgsSettings s;
  restoreGeometry( s.value( QStringLiteral( "/Plugin-GeoReferencer/MapCoordsWindow/geometry" ) ).toByteArray() );

  setAttribute( Qt::WA_DeleteOnClose );
  if ( qgisCanvas->objectName() == "georefCanvas" )
  {
    mPointFromCanvasPushButton = new QPushButton( QIcon( ":/icons/default/mPushButtonPencil.png" ), tr( "From Georef canvas" ) );
  }
  else
  {
    mPointFromCanvasPushButton = new QPushButton( QIcon( ":/icons/default/mPushButtonPencil.png" ), tr( "From map canvas" ) );
  }
  mPointFromCanvasPushButton->setCheckable( true );
  buttonBox->addButton( mPointFromCanvasPushButton, QDialogButtonBox::ActionRole );

  // User can input either DD or DMS coords (from QGis mapcanav we take DD coords)
  QgsDMSAndDDValidator *validator = new QgsDMSAndDDValidator( this );
  leXCoord->setValidator( validator );
  leYCoord->setValidator( validator );

  mToolEmitPoint = new QgsGeorefMapToolEmitPoint( qgisCanvas );
  mToolEmitPoint->setButton( mPointFromCanvasPushButton );

  connect( mPointFromCanvasPushButton, &QAbstractButton::clicked, this, &QgsMapCoordsDialog::setToolEmitPoint );

  connect( mToolEmitPoint, &QgsGeorefMapToolEmitPoint::canvasClicked,
           this, &QgsMapCoordsDialog::maybeSetXY );
  connect( mToolEmitPoint, &QgsGeorefMapToolEmitPoint::mouseReleased, this, &QgsMapCoordsDialog::setPrevTool );

  connect( leXCoord, &QLineEdit::textChanged, this, &QgsMapCoordsDialog::updateOK );
  connect( leYCoord, &QLineEdit::textChanged, this, &QgsMapCoordsDialog::updateOK );
  updateOK();
}

QgsMapCoordsDialog::~QgsMapCoordsDialog()
{
  delete mToolEmitPoint;

  QgsSettings settings;
  settings.setValue( QStringLiteral( "/Plugin-GeoReferencer/MapCoordsWindow/geometry" ), saveGeometry() );
}

void QgsMapCoordsDialog::updateOK()
{
  bool enable = ( leXCoord->text().size() != 0 && leYCoord->text().size() != 0 );
  QPushButton *okPushButton = buttonBox->button( QDialogButtonBox::Ok );
  okPushButton->setEnabled( enable );
}

void QgsMapCoordsDialog::setPrevTool()
{
  mQgisCanvas->setMapTool( mPrevMapTool );
}

void QgsMapCoordsDialog::buttonBox_accepted()
{
  bool ok;
  double x = leXCoord->text().toDouble( &ok );
  if ( !ok )
    x = dmsToDD( leXCoord->text() );

  double y = leYCoord->text().toDouble( &ok );
  if ( !ok )
    y = dmsToDD( leYCoord->text() );
  if ( mPixelMap )
  {
    mPixelCoords.setX( x );
    mPixelCoords.setY( y );
  }
  else
  {
    mMapCoords.setX( x );
    mMapCoords.setY( y );
  }

  emit pointAdded( mPixelCoords, mMapCoords, mIdGcp, mPixelMap );
  close();
}

void QgsMapCoordsDialog::maybeSetXY( const QgsPointXY &xy, Qt::MouseButton button )
{
  // Only LeftButton should set point
  if ( Qt::LeftButton == button )
  {
    QgsPointXY mapCoordPoint = xy;

    leXCoord->clear();
    leYCoord->clear();
    leXCoord->setText( qgsDoubleToString( mapCoordPoint.x() ) );
    leYCoord->setText( qgsDoubleToString( mapCoordPoint.y() ) );
  }

  parentWidget()->showNormal();
  parentWidget()->activateWindow();
  parentWidget()->raise();

  mPointFromCanvasPushButton->setChecked( false );
  buttonBox->button( QDialogButtonBox::Ok )->setFocus();
  activateWindow();
  raise();
}

void QgsMapCoordsDialog::setToolEmitPoint( bool isEnable )
{
  if ( isEnable )
  {
    parentWidget()->showMinimized();

    Q_ASSERT( parentWidget()->parentWidget() );
    parentWidget()->parentWidget()->activateWindow();
    parentWidget()->parentWidget()->raise();

    mPrevMapTool = mQgisCanvas->mapTool();
    mQgisCanvas->setMapTool( mToolEmitPoint );
  }
  else
  {
    mQgisCanvas->setMapTool( mPrevMapTool );
  }
}

double QgsMapCoordsDialog::dmsToDD( const QString &dms )
{
  QStringList list = dms.split( ' ' );
  QString tmpStr = list.at( 0 );
  double res = std::fabs( tmpStr.toDouble() );

  tmpStr = list.value( 1 );
  if ( !tmpStr.isEmpty() )
    res += tmpStr.toDouble() / 60;

  tmpStr = list.value( 2 );
  if ( !tmpStr.isEmpty() )
    res += tmpStr.toDouble() / 3600;

  if ( dms.startsWith( '-' ) )
    return -res;
  else
    return res;
}
