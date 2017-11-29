/***************************************************************************
                          qgspatialitesourceselect.h  -  description
                             -------------------
    begin                : Dec 2008
    copyright            : (C) 2008 by Sandro Furieri
    email                : a.furieri@lqt.it
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSSPATIALITESOURCESELECT_H
#define QGSSPATIALITESOURCESELECT_H
#include "ui_qgsdbsourceselectbase.h"
#include "qgsguiutils.h"
#include "qgsdbfilterproxymodel.h"
#include "qgsspatialitetablemodel.h"
#include "qgshelp.h"
#include "qgsproviderregistry.h"
#include "qgsabstractdatasourcewidget.h"
#include "qgsspatialiteconnection.h"

#include <QThread>
#include <QMap>
#include <QList>
#include <QPair>
#include <QIcon>
#include <QFileDialog>

class QStringList;
class QTableWidgetItem;
class QPushButton;

/**
 * \class QgsSpatiaLiteSourceSelect
 * \brief Dialog to create connections and add tables from SpatiaLite.
 *
 * This dialog allows the user to define and save connection information
 * for SpatiaLite/SQLite databases. The user can then connect and add
 * tables from the database to the map canvas.
 */
// class QgsSpatiaLiteSourceSelect: public QDialog, private Ui::QgsDbSourceSelectBase
class QgsSpatiaLiteSourceSelect: public QgsAbstractDataSourceWidget, private Ui::QgsDbSourceSelectBase
{
    Q_OBJECT

  public:

    /* Open file selector to add new connection */
    static bool newConnection( QWidget *parent );

    //! Constructor
    QgsSpatiaLiteSourceSelect( QWidget *parent, Qt::WindowFlags fl = QgsGuiUtils::ModalDialogFlags, QgsProviderRegistry::WidgetMode widgetMode = QgsProviderRegistry::WidgetMode::None );

    ~QgsSpatiaLiteSourceSelect();
    //! Populate the connection list combo box
    void populateConnectionList();

    <<< <<< < HEAD

    /**
     * Determines the tables the user selected and closes the dialog
    =======

    /**
     * Determines the tables the user selected and closes the dialog
    >>>>>>> upstream_qgis/master32.spatialite_provider
     * \note
     *  - as url
     * \see collectSelectedTables()
     * \since QGIS 1.8
     */
    void addTables();

    <<< <<< < HEAD

    /**
     * List of selected Tabels
     * \note
     *  - as LayerName LayerName formatted as 'table_name(geometry_name)' or 'table_name'
     * \see collectSelectedTables()
     * \since QGIS 1.8
     */
    QString connectionInfo() {  return mTableModel.getDbConnectionInfo( ); }

    /**
     * Store the selected database
    =======

    /**
     * Connection info (DB-path) without table and geometry
     * - this will be called from classes using SpatialiteDbInfo
     * \note
     *  - to call for Database and Table/Geometry portion use: SpatialiteDbLayer::getLayerDataSourceUri()
    * \returns uri with Database only
    * \see SpatialiteDbLayer::getLayerDataSourceUri()
    * \since QGIS 3.0
    */
    QString getDatabaseUri() {  return mTableModel.getDatabaseUri(); }

    /**
     * Store the selected database
    >>>>>>> upstream_qgis/master32.spatialite_provider
     * \note
     *  - Remember which database was selected
     *  - in 'SpatiaLite/connections/selected'
     * \see collectSelectedTables()
     * \since QGIS 1.8
     */
    void dbChanged();
    void setSpatialiteDbInfo( SpatialiteDbInfo *spatialiteDbInfo, bool setConnectionInfo = false );

  public slots:

    <<< <<< < HEAD

    /**
     * Connects to the database using the stored connection parameters.
    =======

    /**
     * Connects to the database using the stored connection parameters.
    >>>>>>> upstream_qgis/master32.spatialite_provider
     * Once connected, available layers are displayed.
     */
    void on_btnConnect_clicked();
    void buildQuery();
    void addClicked();

    <<< <<< < HEAD

    /**
     * Calls UpdateLayerStatistics for a Spatialite Database
    =======

    /**
     * Calls UpdateLayerStatistics for a Spatialite Database
    >>>>>>> upstream_qgis/master32.spatialite_provider
     *  - for selected Layers ar the whole Database
     * \note
     *  - Uses the SpatialiteDbInfo interface
     *  -> UpdateLayerStatistics()
     * \see SpatialiteDbInfo::isDbSpatialite()
     * \since QGIS 3.0
     */
    void updateStatistics();

    <<< <<< < HEAD

    /**
     * Select File-name for new Database
    =======

    /**
     * Select File-name for new Database
    >>>>>>> upstream_qgis/master32.spatialite_provider
     *  - for use with SpatialiteDbInfo::createDatabase
     * \note
     * - SpatialMetadata::Spatialite40: InitSpatialMetadata only
     * - SpatialMetadata::Spatialite45: also with Styles/Raster/VectorCoveragesTable's
     * - SpatialMetadata::SpatialiteGpkg: using spatialite 'gpkgCreateBaseTables' function
     * - SpatialMetadata::SpatialiteMBTiles: creating a view-base MbTiles file with grid tables
     * \see SpatialiteDbInfo::createDatabase
     * \see cmbDbCreateOption
     * \since QGIS 3.0
     */
    void on_btnSave_clicked();
    //! Opens the create connection dialog to build a new connection
    void on_btnNew_clicked();
    //! Deletes the selected connection
    void on_btnDelete_clicked();
    void on_mSearchGroupBox_toggled( bool );
    void on_mSearchTableEdit_textChanged( const QString &text );
    void on_mSearchColumnComboBox_currentIndexChanged( const QString &text );
    void on_mSearchModeComboBox_currentIndexChanged( const QString &text );
    <<< <<< < HEAD
#if 0
    void on_cbxAllowGeometrylessTables_stateChanged( int );
#endif
    == == == =
      >>> >>> > upstream_qgis / master32.spatialite_provider
      void setSql( const QModelIndex &index );
    void on_cmbConnections_activated( int );
    void on_mTablesTreeView_clicked( const QModelIndex &index );
    void on_mTablesTreeView_doubleClicked( const QModelIndex &index );
    void treeWidgetSelectionChanged( const QItemSelection &selected, const QItemSelection &deselected );
    //!Sets a new regular expression to the model
    void setSearchExpression( const QString &regexp );

    void on_buttonBox_helpRequested() { QgsHelp::openHelp( QStringLiteral( "managing_data_source/opening_data.html#spatialite-layers" ) ); }

    <<< <<< < HEAD

    /**
     * Load selected Database-Source
    =======

    /**
     * Load selected Database-Source
    >>>>>>> upstream_qgis/master32.spatialite_provider
     *  - load Database an fill the QgsSpatiaLiteTableModel with the result
     * \note
     *  - only the first entry of 'paths' will be used
     * \param paths absolute path to file(s) to load
     * \param providerKey
     * \see on_btnConnect_clicked
     * \see addDatabaseLayers
     * \see QgisApp::openLayer
     * \since QGIS 3.0
     */
    void addDatabaseSource( QStringList const &paths, QString const &providerKey = QStringLiteral( "spatialite" ) );
  signals:

    /**
     * Emitted when the provider's connections have changed
     * This signal is normally forwarded the app and used to refresh browser items
     * \since QGIS 3.0
     */
    void connectionsChanged();

    <<< <<< < HEAD

    /**
     * Emitted when a DB layer has been selected for addition
    =======

    /**
     * Emitted when a DB layer has been selected for addition
    >>>>>>> upstream_qgis/master32.spatialite_provider
     * \note
     *  - this event is used to load a DB-Layer during a Drag and Drop
     * \see QgisApp::openLayer
     * \see addDatabaseSource
     * \since QGIS 3.0
     */
    void addDatabaseLayers( QStringList const &paths, QString const &providerKey );

  private:

    <<< <<< < HEAD

    /**
     * Set the position of the database connection list to the last used one.
    =======

    /**
     * Set the position of the database connection list to the last used one.
    >>>>>>> upstream_qgis/master32.spatialite_provider
     * \note
     *  - from 'SpatiaLite/connections/selected'
     * \see populateConnectionList
     * \since QGIS 1.8
     */
    void setConnectionListPosition();

    <<< <<< < HEAD

    /**
     * Combine the table and column data into a single string
    =======

    /**
     * Combine the table and column data into a single string
    >>>>>>> upstream_qgis/master32.spatialite_provider
     * \note
     *  - useful for display to the user
     * \since QGIS 1.8
     */
    QString fullDescription( const QString &table, const QString &column, const QString &type );

    <<< <<< < HEAD

    /**
     * The column labels
    =======

    /**
     * The column labels
    >>>>>>> upstream_qgis/master32.spatialite_provider
     * \note
     *  - not used
     * \since QGIS 1.8
     */
    QStringList mColumnLabels;

    <<< <<< < HEAD

    /**
     * Absolute Path of the Connection
    =======

    /**
     * Absolute Path of the Connection
    >>>>>>> upstream_qgis/master32.spatialite_provider
     * - 'SpatiaLite/connections'
     * \note
     *  - retrieved from QgsSpatiaLiteConnection
     * \returns name of connection
     * \see on_btnConnect_clicked
     * \since QGIS 1.8
     */
    QString mSqlitePath;

    <<< <<< < HEAD

    /**
     * Function to collect selected layers
    =======

    /**
     * Function to collect selected layers
    >>>>>>> upstream_qgis/master32.spatialite_provider
     * \note
     *  - fills m_selectedTables with uri
     *  - fills m_selectedLayers with LayerName formatted as 'table_name(geometry_name)' or 'table_name'
     * \see addTables()
     * \see updateStatistics()
     * \since QGIS 3.0
     */
    int collectSelectedTables();

    <<< <<< < HEAD

    /**
     * List of selected Tabels
    =======

    /**
     * List of selected Tabels
    >>>>>>> upstream_qgis/master32.spatialite_provider
     * \note
     *  - as LayerName LayerName formatted as 'table_name(geometry_name)' or 'table_name'
     * \see collectSelectedTables()
     * \since QGIS 3.0
     */
    QStringList m_selectedLayers;

    <<< <<< < HEAD

    /**
     * List of extra Sql-Query for selected Layers
    =======

    /**
     * List of extra Sql-Query for selected Layers
    >>>>>>> upstream_qgis/master32.spatialite_provider
     * \note
     *  - as extra Sql-Query
     * \see collectSelectedTables()
     * \since QGIS 1.8
     */
    QStringList m_selectedLayersSql;

    <<< <<< < HEAD

    /**
     * Add a list of database layers to the map
    =======

    /**
     * Add a list of database layers to the map
    >>>>>>> upstream_qgis/master32.spatialite_provider
     * - to fill a Map of QgsVectorLayers and/or QgsRasterLayers
     * -> can be for both QgsSpatiaLiteProvider or QgsOgr/GdalProvider
     * \note
     * - requested by User to add to main QGis
     * - collectSelectedTables must be called beforhand
     * -> emit addDatabaseLayers for each supported Provider
     * \param saSelectedLayers formatted as 'table_name(geometry_name)'
     * \returns amount of Uris entries
     * \see getSelectedLayersUris
     * \see collectSelectedTables()
     * \since QGIS 3.0
     */
    <<< <<< < HEAD
    int addDbMapLayers( ) const {  return mTableModel.addDbMapLayers( m_selectedLayers, m_selectedLayersSql );  }

    /**
     * Map of valid Selected Layers requested by the User
    =======
    int addDbMapLayers() const {  return mTableModel.addDbMapLayers( m_selectedLayers, m_selectedLayersSql );  }

    /**
     * Map of valid Selected Layers requested by the User
    >>>>>>> upstream_qgis/master32.spatialite_provider
     * - only Uris that created a valid QgsVectorLayer/QgsRasterLayer
     * -> filled during addDatabaseLayers
     * -> called through the SpatialiteDbInfo stored in QgsSpatiaLiteTableModel
     * \note
     * - Key: LayerName formatted as 'table_name(geometry)'
     * - Value: Uris dependent on provider
     * - can be for both QgsSpatiaLiteProvider or QgsGdalProvider
     * \returns mSelectedLayersUris  Map of LayerNames and  valid Layer-Uris entries
     * \see QgsSpatiaLiteTableModel:;getSpatialiteDbInfo
     * \see addDatabaseLayers
     * \since QGIS 3.0
     */
    QMap<QString, QString> getSelectedLayersUris() const { return mTableModel.getSelectedLayersUris(); }

    <<< <<< < HEAD

    /**
     * Storage for the range of layer type icons
    =======

    /**
     * Storage for the range of layer type icons
    >>>>>>> upstream_qgis/master32.spatialite_provider
     * \note
     *  - fills m_selectedTables with uri
     *  - fills m_selectedLayers with LayerName formatted as 'table_name(geometry_name)' or 'table_name'
     * \see addTables()
     * \see updateStatistics()
     * \since QGIS 3.0
     */
    QMap < QString, QPair < QString, QIcon > >mLayerIcons;

    <<< <<< < HEAD

    /**
     * Model that acts as datasource for mTableTreeWidget
    =======

    /**
     * Model that acts as datasource for mTableTreeWidget
    >>>>>>> upstream_qgis/master32.spatialite_provider
     * \note
     *  - filled by values contained in SpatialiteDbInfo
     * \since QGIS 1.8
     */
    QgsSpatiaLiteTableModel mTableModel;

    <<< <<< < HEAD

    /**
     * QgsDatabaseFilterProxyMode
    =======

    /**
     * QgsDatabaseFilterProxyMode
    >>>>>>> upstream_qgis/master32.spatialite_provider
     * \note
     *  - does what ever needs to be done
     * \since QGIS 1.8
     */
    QgsDatabaseFilterProxyModel mProxyModel;

    <<< <<< < HEAD

    /**
     * Creates extra Sql-Query for selected Layers
    =======

    /**
     * Creates extra Sql-Query for selected Layers
    >>>>>>> upstream_qgis/master32.spatialite_provider
     * \note
     *  - retrieves Table and Geometry Names with Sql from TableModel
     *  -> which know the needed index-number
     *  TODO for QGIS 3.0
     *  - TableModel should retrieve the Url from SpatialiteDbInfo
     *  -> adding the Sql-Query to that result
     * \see QgsSpatiaLiteTableModel::getTableName
     * \see QgsSpatiaLiteTableModel::getGeometryName
     * \see QgsSpatiaLiteTableModel::getSqlQuery
     * \see m_selectedLayersSql
     * \since QGIS 1.8
     */
    QString layerUriSql( const QModelIndex &index );

    <<< <<< < HEAD

    /**
     * Build Sql-Query
    =======

    /**
     * Build Sql-Query
    >>>>>>> upstream_qgis/master32.spatialite_provider
     * \note
     *  - calls setSql
     *  TODO for QGIS 3.0
     *  - TableModel should the Layer-Type (Raster/Vector)
     *  - TableModel should the Provider-Type (Spatialite/Gdal/Oge)
     * \see buildQuery
     * \see setSql
     * \since QGIS 1.8
     */
    QPushButton *mBuildQueryButton = nullptr;

    <<< <<< < HEAD

    /**
     * Add selected Entries to Qgis
    =======

    /**
     * Add selected Entries to Qgis
    >>>>>>> upstream_qgis/master32.spatialite_provider
     * \note
     *  - calls addTables
     * \see addClicked
     * \see addTables
     * \since QGIS 1.8
     */
    QPushButton *mAddButton = nullptr;

    <<< <<< < HEAD

    /**
     * Button Interface for UpdateLayerStatitics
    =======

    /**
     * Button Interface for UpdateLayerStatitics
    >>>>>>> upstream_qgis/master32.spatialite_provider
     * \note
     *  - UpdateLayerStatitics is done through SpatialiteDbInfo
     * \see updateStatistics
     * \since QGIS 1.8
     */
    QPushButton *mStatsButton = nullptr;

    <<< <<< < HEAD

    /**
     * Will contain values to represent option to create a new Database
    =======

    /**
     * Will contain values to represent option to create a new Database
    >>>>>>> upstream_qgis/master32.spatialite_provider
     *  - for use with SpatialiteDbInfo::createDatabase
     * \note
     * - SpatialMetadata::Spatialite40: InitSpatialMetadata only
     * - SpatialMetadata::Spatialite45: also with Styles/Raster/VectorCoveragesTable's
     * - SpatialMetadata::SpatialiteGpkg: using spatialite 'gpkgCreateBaseTables' function
     * - SpatialMetadata::SpatialiteMBTiles: creating a view-base MbTiles file with grid tables
     * \returns true if the Database was created
     * \see mDbCreateOption
     * \see SpatialiteDbInfo::createDatabase
     * \since QGIS 3.0
     */
    QComboBox *cmbDbCreateOption = nullptr;

    <<< <<< < HEAD

    /**
     * QgsProviderRegistry::WidgetMode
    =======

    /**
     * QgsProviderRegistry::WidgetMode
    >>>>>>> upstream_qgis/master32.spatialite_provider
     * \note
     *  - does what ever needs to be done
     * \since QGIS 1.8
     */
    QgsProviderRegistry::WidgetMode mWidgetMode = QgsProviderRegistry::WidgetMode::None;
};

#endif // QGSSPATIALITESOURCESELECT_H
