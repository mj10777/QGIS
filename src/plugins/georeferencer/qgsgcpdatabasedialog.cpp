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
#include <QSettings>

#include "qgsprojectionselector.h"

#include "qgsapplication.h"

#include "qgsgcpdatabasedialog.h"
#include "qgscoordinatereferencesystem.h"
#include "qgscrscache.h"

QgsGcpDatabaseDialog::QgsGcpDatabaseDialog( const QgsCoordinateReferenceSystem  &destCRS, QWidget *parent )
  : QDialog( parent )
  , mDestCRS( destCRS )
  , b_create_sqldump( false )
  , gcp_db_type( QgsSpatiaLiteProviderGcpUtils::GcpCoverages )
  , mDbSuffic( "db" )
{
  setupUi( this );
  QSettings s;
  QString s_gcp_authid = mDestCRS.authid();
  QStringList sa_authid = s_gcp_authid.split( ":" );
  QString s_srid = "-1";
  if ( sa_authid.length() == 2 )
    s_srid = sa_authid[1];
  mGcpSrid = s_srid.toInt();
  gcp_db_type = ( QgsSpatiaLiteProviderGcpUtils::GcpDatabases )s.value( "/Plugin-GeoReferencer/lastdbtype", 0 ).toInt();
  if ( gcp_db_type == QgsSpatiaLiteProviderGcpUtils::GcpCoverages )
  {
    mDbSuffic = "gcp.db";
  }
  mDatabaseDir = s.value( "/Plugin-GeoReferencer/gcpdb_directory" ).toString();
  QDir build_dir( mDatabaseDir );
  mDatabaseDir = build_dir.canonicalPath();
  leOutputDatabaseDir->setText( mDatabaseDir );
  cmbGcpDatabaseType->addItem( tr( "GcpCoverages" ), ( int )QgsSpatiaLiteProviderGcpUtils::GcpCoverages );
  cmbGcpDatabaseType->addItem( tr( "GcpMaster" ), ( int )QgsSpatiaLiteProviderGcpUtils::GcpMaster );
  cmbGcpDatabaseType->setCurrentIndex( ( int )gcp_db_type );
  mCrsSelector->setCrs( mDestCRS );
  // setGcpDatabaseName will be called, setting mDatabaseFile and mOutputDatabaseFileEdit
  database_file.setFile( QString( "%1/%2" ).arg( mDatabaseDir ).arg( mDatabaseFile ) );
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
    gcp_db_type = ( QgsSpatiaLiteProviderGcpUtils::GcpDatabases )cmbGcpDatabaseType->itemData( cmbGcpDatabaseType->currentIndex() ).toInt();
    mDatabaseFile = mOutputDatabaseFileEdit->text();
    if ( mDatabaseFile.isEmpty() )
    {
      mDatabaseFile = defaultGcpDatabaseName();
    }
    else
    {
      if ( !mDatabaseFile.endsWith( mDbSuffic ) )
      {
        QFileInfo adapt_file( mDatabaseFile );
        mDatabaseFile = QString( "%1.%2" ).arg( adapt_file.baseName() ).arg( mDbSuffic );
      }
    }
    mDatabaseDir = leOutputDatabaseDir->text();
    QFileInfo build_file( QString( "%1/%2" ).arg( mDatabaseDir ).arg( mDatabaseFile ) );
    // TODO: the that suffix is db or for coverage .gcp.db
    QDir build_dir( mDatabaseDir );
    mDatabaseDir = build_dir.canonicalPath();
    database_file.setFile( QString( "%1/%2" ).arg( mDatabaseDir ).arg( mDatabaseFile ) );
    b_create_sqldump = cbxCreateSqlDump->isChecked();
    // qDebug() << QString( "QgsGcpDatabaseDialog::accept() -zz- GcpMaster srid[%1] dump[%2] file[%3] dir[%4] canonicalFilePath[%5]" ).arg( mGcpSrid ).arg(b_create_sqldump ).arg( mDatabaseFile ).arg(mDatabaseDir).arg(database_file.absoluteFilePath() );
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
    switch ( gcp_db_type )
    {
      case QgsSpatiaLiteProviderGcpUtils::GcpCoverages:
        text = "GcpCoverages";
        break;
      case QgsSpatiaLiteProviderGcpUtils::GcpMaster:
        text = "GcpMaster";
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
    mDbSuffic = "gcp.db";
    gcp_db_type = QgsSpatiaLiteProviderGcpUtils::GcpCoverages;
    if ( isDefaultName )
    {
      mDatabaseFile = defaultGcpDatabaseName();
    }
    else
    {
      QFileInfo adapt_file( mOutputDatabaseFileEdit->text() );
      mDatabaseFile = QString( "%1.%2" ).arg( adapt_file.baseName() ).arg( mDbSuffic );
    }
  }
  if ( text == "GcpMaster" )
  {
    // Previous values must remain in place befor the first call to defaultGcpDatabaseName()
    mDbSuffic = "db";
    gcp_db_type = QgsSpatiaLiteProviderGcpUtils::GcpMaster;
    if ( isDefaultName )
    {
      mDatabaseFile = defaultGcpDatabaseName();
    }
    else
    {
      QFileInfo adapt_file( mOutputDatabaseFileEdit->text() );
      mDatabaseFile = QString( "%1.%2" ).arg( adapt_file.baseName() ).arg( mDbSuffic );
    }
  }
  mOutputDatabaseFileEdit->setText( mDatabaseFile );
  return mDatabaseFile;
}

