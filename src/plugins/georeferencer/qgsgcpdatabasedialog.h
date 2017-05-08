/***************************************************************************
     qgsgcpdatabasedialog.h
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

#ifndef QGSGCPDATABASESDIALOG_H
#define QGSGCPDATABASESDIALOG_H

#include <QDialog>

#include "qgsgeorefplugingui.h"

#include "ui_qgsgcpdatabasedialogbase.h"

class QgsGcpDatabaseDialog : public QDialog, private Ui::QgsGcpDatabaseDialog
{
    Q_OBJECT

  public:
    QgsGcpDatabaseDialog( const QgsCoordinateReferenceSystem &destCRS, QWidget *parent = nullptr );

    ~QgsGcpDatabaseDialog();

    /**
     * A QFileInfo built out of mDatabaseDir and mDatabaseFileName
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \note
     *  -> will be used to build the QFileInfo that the Dialog will return
     * \see mDatabaseDir
     * \see mDatabaseFileName
     * \see mDatabaseFile
     */
    QFileInfo GcpDatabaseFile() { return mDatabaseFile; }

    /**
     * Which type of Gcp Database should be created
     *  -> will determine which function will be called
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \note
     *  -> will be used to the srid of the Database
     * \see QgsGeorefPluginGui::createGcpCoverageDb
     * \see QgsGeorefPluginGui::createGcpMasterDb
     * \see mGcpDbType
     */
    QgsSpatiaLiteGcpUtils::GcpDatabases GcpDatabaseType() { return mGcpDbType; }

    /**
     * Srid of the Database
     *  ->default value will be the present setting of the QGIS-Application
     * -> the srid will be extracted from the authid()
     * -> 2nd value of authid.split( ":" );
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \note
     *  -> will be used to the srid of the Database
     * \see mGcpSrid
     */
    int GcpDatabaseSrid() { return mGcpSrid; }

    /**
     * Create a Sql-Dump of the Database
     *  -> during creation
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \note
     *  -> will be used to the srid of the Database
     * \see QgsGeorefPluginGui::createGcpCoverageDb
     * \see QgsGeorefPluginGui::createGcpMasterDb
     * \see GcpDatabaseDump()
     */
    bool GcpDatabaseDump() { return mCreateSqlDump; }

  protected:
    void changeEvent( QEvent *e ) override;

    /**
     * The final values will be prepareed for the results of the dialog
     *  -> the suffix/file extetion will be enforsed of the File-Name
     * \note
     *  - 'gcp_coverage'
     *  -> should have the 'gcp.db' suffix/file-extension to be recognised by this application
     * \note
     *  - 'gcp_master'
     *  -> should have the 'db' suffix/file-extension to be recognised by this application
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \see GcpDatabaseFile
     * \see GcpDatabaseType
     * \see GcpDatabaseSrid
     * \see GcpDatabaseDump
     * \see QgsGeorefPluginGui::createGcpDatabaseDialog
     */
    void accept() override;

  private slots:
    void on_tbnOutputDatabaseDir_clicked();
    void on_cmbGcpDatabaseType_currentIndexChanged( const QString &text );
    void on_mCrsSelector_crsChanged( const QgsCoordinateReferenceSystem &changed_Crs );

  private:

    /**
     * Query an default name for the Gcp-Database (Coverage/Master)
     * \note
     *  - 'gcp_coverage'
     *  -> should have the 'gcp.db' suffix/file-extension to be recognised by this application
     * \note
     *  - 'gcp_master'
     *  -> should start with 'gcp_master' to be recognised by this application
     *  -> should also have the 'db' suffix/file-extension to be recognised by this application
     * \note
     *  -> setGcpDatabaseName will call this to see if the User has renamed the input
     *  --> if NOT, it will setGcpDatabaseName replace the value of mOutputDatabaseFileEdit with the returned value
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \see setGcpDatabaseName
     * \return mDatabaseFileName the default name for mOutputDatabaseFileEdit
     */
    QString defaultGcpDatabaseName() { if ( mGcpDbType == QgsSpatiaLiteGcpUtils::GcpCoverages ) return QString( "gcp_coverages.%1.%2" ).arg( mGcpSrid ).arg( mDbSuffic ); else return QString( "gcp_master.%1.%2" ).arg( mGcpSrid ).arg( mDbSuffic ); }

    /**
     * Create an default name for the Gcp-Database (Coverage/Master)
     * \note
     *  - 'gcp_coverage'
     *  -> should have the 'gcp.db' suffix/file-extension to be recognised by this application
     * \note
     *  - 'gcp_master'
     *  -> should start with 'gcp_master' to be recognised by this application
     *  -> should also have the 'db' suffix/file-extension to be recognised by this application
     * \note
     *  -> Adding the srid is only intended as a convenience for creating default empty Databases
     *  --> it is NOT needed to be recognised by this application
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \see defaultGcpDatabaseName
     * \param text if QString::null: will be read from  mOutputDatabaseFileEdit->text()
     * \param i_srid if not INT_MIN: will set the member mGcpSrid
     * \return mDatabaseFileName at its present setting (not used)
     */
    QString setGcpDatabaseName( QString text = QString::null, int i_srid = INT_MIN );

    /**
     * The Directory to store the Database
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \note
     *  -> will be used to build the QFileInfo that the Dialog will return
     * \see GcpDatabaseFile
     */
    QString mDatabaseDir;

    /**
     * The Filename of the Database
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \note
     *  -> will be used to build the QFileInfo that the Dialog will return
     * \see GcpDatabaseFile
     */
    QString mDatabaseFileName;

    /**
     * A QFileInfo built out of mDatabaseDir and mDatabaseFileName
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \note
     *  -> will be used to build the QFileInfo that the Dialog will return
     * \see mDatabaseDir
     * \see mDatabaseFileName
     * \see GcpDatabaseFile
     */
    QFileInfo mDatabaseFile;

    /**
     * CoordinateReferenceSystem to retrieve the desired srid of the Database
     *  - default value will be the present setting of the QGIS-Application
     * -> the srid will be extracted from the authid()
     * -> 2nd value of authid.split( ":" );
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \note
     *  -> will be used to the srid of the Database
     * \see GcpDatabaseSrid()
     */
    QgsCoordinateReferenceSystem mDestCRS;

    /**
     * Create a Sql-Dump of the Database
     *  -> during creation
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \note
     *  -> will be used to the srid of the Database
     * \see QgsGeorefPluginGui::createGcpCoverageDb
     * \see QgsGeorefPluginGui::createGcpMasterDb
     * \see GcpDatabaseDump()
     */
    bool mCreateSqlDump;

    /**
     * Srid of the Database
     *  ->default value will be the present setting of the QGIS-Application
     * -> the srid will be extracted from the authid()
     * -> 2nd value of authid.split( ":" );
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \note
     *  -> will be used to the srid of the Database
     * \see GcpDatabaseSrid()
     */
    int mGcpSrid;

    /**
     * Which type of Gcp Database should be created
     *  -> will determine which function will be called
     * \since QGIS 3.0
     * New for ( mLegacyMode == 1 )
     *  - therefore is needed
     * \note
     *  -> will be used to the srid of the Database
     * \see GcpDatabaseType()
     * \see QgsGeorefPluginGui::createGcpCoverageDb
     * \see QgsGeorefPluginGui::createGcpMasterDb
     */
    QgsSpatiaLiteGcpUtils::GcpDatabases mGcpDbType;

    /**
     * Store the default suffix/File extension based on the Gcp-Database Type
     * \note
     *  - 'gcp_coverage'
     *  -> should have the 'gcp.db' suffix/file-extension to be recognised by this application
     * \note
     *  - 'gcp_master'
     *  -> should also have the 'db' suffix/file-extension to be recognised by this application
     */
    QString mDbSuffic;
};

#endif // QGSGCPDATABASESDIALOG_H
