/***************************************************************************
     qgsgcpdatabasedialog.cpp
     --------------------------------------
    Date                 : 14-Feb-2010
    Copyright            : (C) 2010 by Jack R, Maxim Dubinin (GIS-Lab)
    Email                : sim@gis-lab.info
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include "qgsgcpdatabasedialog.h"
#include "qgscoordinatereferencesystem.h"
#include "qgssettings.h"

QgsGcpDatabaseDialog::QgsGcpDatabaseDialog( const QgsCoordinateReferenceSystem  &destCRS, QWidget *parent )
  : QDialog( parent )
  , mDestCRS( destCRS )
  , mCreateSqlDump( false )
  , mGcpDbType( QgsSpatiaLiteGcpUtils::GcpCoverages )
  , mDbSuffic( "db" )
{
  setupUi( this );
  QgsSettings s;
  QString s_gcp_authid = mDestCRS.authid();
  QStringList sa_authid = s_gcp_authid.split( ":" );
  QString s_srid = "-1";
  if ( sa_authid.length() == 2 )
    s_srid = sa_authid[1];
  mGcpSrid = s_srid.toInt();
  mGcpDbType = ( QgsSpatiaLiteGcpUtils::GcpDatabases )s.value( QStringLiteral( "/Plugin-GeoReferencer/lastdbtype" ), 0 ).toInt();
  if ( mGcpDbType == QgsSpatiaLiteGcpUtils::GcpCoverages )
  {
    mDbSuffic = "gcp.db";
  }
  mDatabaseDir = s.value( QStringLiteral( "/Plugin-GeoReferencer/gcpdb_directory" ) ).toString();
  QDir build_dir( mDatabaseDir );
  mDatabaseDir = build_dir.canonicalPath();
  leOutputDatabaseDir->setText( mDatabaseDir );
  cmbGcpDatabaseType->addItem( tr( "GcpCoverages" ), ( int )QgsSpatiaLiteGcpUtils::GcpCoverages );
  cmbGcpDatabaseType->addItem( tr( "GcpMaster" ), ( int )QgsSpatiaLiteGcpUtils::GcpMaster );
  cmbGcpDatabaseType->setCurrentIndex( ( int )mGcpDbType );
  mCrsSelector->setCrs( mDestCRS );
  // setGcpDatabaseName will be called, setting mDatabaseFileName and mOutputDatabaseFileEdit
  mDatabaseFile.setFile( QStringLiteral( "%1/%2" ).arg( mDatabaseDir ).arg( mDatabaseFileName ) );
}

QgsGcpDatabaseDialog::~QgsGcpDatabaseDialog()
{
}

void QgsGcpDatabaseDialog::changeEvent( QEvent *e )
{
  QDialog::changeEvent( e );
  switch ( e->type() )
  {
    case QEvent::LanguageChange:
      retranslateUi( this );
      break;
    default:
      break;
  }
}

void QgsGcpDatabaseDialog::accept()
{
  if ( !leOutputDatabaseDir->text().isEmpty() )
  {
    mGcpDbType = ( QgsSpatiaLiteGcpUtils::GcpDatabases )cmbGcpDatabaseType->itemData( cmbGcpDatabaseType->currentIndex() ).toInt();
    mDatabaseFileName = mOutputDatabaseFileEdit->text();
    if ( mDatabaseFileName.isEmpty() )
    {
      mDatabaseFileName = defaultGcpDatabaseName();
    }
    else
    {
      if ( !mDatabaseFileName.endsWith( mDbSuffic ) )
      {
        QFileInfo adapt_file( mDatabaseFileName );
        mDatabaseFileName = QStringLiteral( "%1.%2" ).arg( adapt_file.baseName() ).arg( mDbSuffic );
      }
    }
    mDatabaseDir = leOutputDatabaseDir->text();
    QFileInfo build_file( QStringLiteral( "%1/%2" ).arg( mDatabaseDir ).arg( mDatabaseFileName ) );
    // TODO: the that suffix is db or for coverage .gcp.db
    QDir build_dir( mDatabaseDir );
    mDatabaseDir = build_dir.canonicalPath();
    mDatabaseFile.setFile( QStringLiteral( "%1/%2" ).arg( mDatabaseDir ).arg( mDatabaseFileName ) );
    mCreateSqlDump = cbxCreateSqlDump->isChecked();
    // qDebug() << QString( "QgsGcpDatabaseDialog::accept() -zz- GcpMaster srid[%1] dump[%2] file[%3] dir[%4] canonicalFilePath[%5]" ).arg( mGcpSrid ).arg(mCreateSqlDump ).arg( mDatabaseFileName ).arg(mDatabaseDir).arg(mDatabaseFile.absoluteFilePath() );
  }
  QDialog::accept();
}

void QgsGcpDatabaseDialog::on_tbnOutputDatabaseDir_clicked()
{
  QString db_dir = QFileDialog::getExistingDirectory( this, tr( "Store in Directory" ),  leOutputDatabaseDir->text(),
                   QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );
  if ( db_dir.isEmpty() )
    return;
  mDatabaseDir = db_dir;
  leOutputDatabaseDir->setText( mDatabaseDir );
  leOutputDatabaseDir->setToolTip( mDatabaseDir );
}
void QgsGcpDatabaseDialog::on_mCrsSelector_crsChanged( const QgsCoordinateReferenceSystem &changed_Crs )
{
  QString s_gcp_authid = changed_Crs.authid();
  QStringList sa_authid = s_gcp_authid.split( ":" );
  QString s_srid = "-1";
  if ( sa_authid.length() == 2 )
    s_srid = sa_authid[1];
  setGcpDatabaseName( QString::null, s_srid.toInt() );
}
void QgsGcpDatabaseDialog::on_cmbGcpDatabaseType_currentIndexChanged( const QString &text )
{
  setGcpDatabaseName( text );
}
QString QgsGcpDatabaseDialog::setGcpDatabaseName( QString text, int i_srid )
{
  bool isDefaultName = false;
  if ( text.isNull() )
  {
    // Emulating result of on_cmbGcpDatabaseType_currentIndexChanged
    switch ( mGcpDbType )
    {
      case QgsSpatiaLiteGcpUtils::GcpCoverages:
        text = QStringLiteral( "GcpCoverages" );
        break;
      case QgsSpatiaLiteGcpUtils::GcpMaster:
        text = QStringLiteral( "GcpMaster" );
        break;
    }
  }
  if ( ( mOutputDatabaseFileEdit->text().isEmpty() ) || ( mOutputDatabaseFileEdit->text() == defaultGcpDatabaseName() ) )
  {
    // Previous members values must remain in place before the first call to defaultGcpDatabaseName()
    isDefaultName = true;
  }
  if ( i_srid != INT_MIN )
  {
    // Now we can change the (possibly) changed member values
    mGcpSrid = i_srid;
  }
  if ( text == "GcpCoverages" )
  {
    mDbSuffic = QStringLiteral( "gcp.db" );
    mGcpDbType = QgsSpatiaLiteGcpUtils::GcpCoverages;
    if ( isDefaultName )
    {
      mDatabaseFileName = defaultGcpDatabaseName();
    }
    else
    {
      QFileInfo adapt_file( mOutputDatabaseFileEdit->text() );
      mDatabaseFileName = QStringLiteral( "%1.%2" ).arg( adapt_file.baseName() ).arg( mDbSuffic );
    }
  }
  if ( text == "GcpMaster" )
  {
    // Previous values must remain in place before the first call to defaultGcpDatabaseName()
    mDbSuffic = QStringLiteral( "db" );
    mGcpDbType = QgsSpatiaLiteGcpUtils::GcpMaster;
    if ( isDefaultName )
    {
      mDatabaseFileName = defaultGcpDatabaseName();
    }
    else
    {
      QFileInfo adapt_file( mOutputDatabaseFileEdit->text() );
      mDatabaseFileName = QStringLiteral( "%1.%2" ).arg( adapt_file.baseName() ).arg( mDbSuffic );
    }
  }
  mOutputDatabaseFileEdit->setText( mDatabaseFileName );
  return mDatabaseFileName;
}
