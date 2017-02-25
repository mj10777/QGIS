/***************************************************************************
 *   Copyright (C) 2016 by Mark Johnson, Berlin Germany                                      *
 *   mj10777@googlemail.com                                                     *
 *                                                                         *
 *   This is a plugin generated from the QGIS plugin template              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef QGSSPATIALITEPROVIDERGCPUTILS_H
#define QGSSPATIALITEPROVIDERGCPUTILS_H

#include <limits>
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>

#include "qgsrasterlayer.h"

#include <sqlite3.h>
#include <spatialite.h>

class QgsPoint;
class QgsRasterLayer;

/**
  * Class to CREATE and read specific Spatialite Gcp-Databases
  *  - mainly indended for use with the Georeferencer
  *  -> but designed to be used elsewere
  * @note 'PROJECTNAME.gcp.db'
  *  Will be a Database that will contain TABLEs for each raster in a directory
  *  - that is be georeferenced (Gcp)
  *  -> they will contain (for each raster)
  *  --> 'map-points' of a specific srid
  *  --> 'pixel-points' of pixel positions (srid=-1)
  * @note 'gcp_master.db'
  *  Will be a Database that will contain a TABLE called 'gcp_master'
  *  - that will contain points of a specific srid
  *  - the TABLE will also contain metadata about each point
  *  --> name, notes, general text and 2 fields about to whome it belongs to
  *  --> DATEs valid_since, valid_until
  *  --> position and srid in readable form from the POINT geometry
  *  ---> that will autamaticly be UPDATEd by the TRIGGERs
  *  --> gcp_type: such as street-intersections/start and end points OR historial points
  *  ---> togeather with corresponding SpatialViews (with TRIGGERs)
  *  Goal: is to use these (Well Known) Points as a source for the georeferencer
  *  - togeather with the DATE (corresponding the the date of the map) and type
  *  - with a Spatialite version compiled with the Gcp-logic, the georeferencer can calulate the corresponding pixel-point
  */
class QgsSpatiaLiteProviderGcpUtils
{
  public:
    /**
     * Structure to transfer Data between an using application and the static functions
     *  - for use the the creation and reading of Gcp-specific Databases
     * @note a long term Database connection is NOT intended
     *  When calling from an Application
     *  - the Database connection should be closed when returned to the Application
     *  -> the use of the connection is only indended for internal use
     */
    enum GcpDatabases
    {
      GcpCoverages,
      GcpMaster
    };
    struct GcpDbData
    {
      GcpDbData( QString s_database_filename, QString s_coverage_name = QString::null, int i_srid = INT_MIN )
          : mGcpDatabaseFileName( s_database_filename )
          , mGcpMasterDatabaseFileName( QString::null )
          , mGcp_coverage_name( s_coverage_name )
          , mGcpSrid( i_srid )
          , mGcpMasterSrid( -1 )
          , mGcp_coverage_title( "" )
          , mGcp_coverage_abstract( "" )
          , mGcp_coverage_copyright( "" )
          , mGcp_coverage_map_date( "" )
          , mGcp_points_table_name( QString::null )
          , mLayer( nullptr )
          , mGcp_enabled( false )
          , mInputPoint( QgsPoint( 0.0, 0.0 ) )
          , mToPixel( false )
          , mOrder( 0 )
          , mReCompute( true )
          , mIdGcp( -1 )
          , mId_gcp_coverage( -1 )
          , mId_gcp_cutline( -1 )
          , mTransformParam( -1 )
          , mResamplingMethod( "Lanczos" )
          , mRasterYear( 0 )
          , mRasterScale( 0 )
          , mRasterNodata( -1 )
          , mCompressionMethod( "DEFLATE" )
          , mGcpPointsFileName( QString::null )
          , mGcpBaseFileName( QString::null )
          , mRasterFilePath( QString::null )
          , mRasterFileName( QString::null )
          , mModifiedRasterFileName( QString::null )
          , mGcp_coverage_name_base( QString::null )
          , mError( QString::null )
          , mSqlDump( false )
          , mDatabaseDump( false )
          , mDatabaseValid( false )
          , mParseString( ";#;#" )
          , spatialite_cache( nullptr )
          , db_handle( nullptr )
          , mUsed_database_filename( QString::null )
          , mUsed_database_filename_count( 0 )
      {}
      GcpDbData( QString s_database_filename, QString s_coverage_name, int i_srid, QString s_points_table_name,  QgsRasterLayer *raster_layer, bool b_Gcp_enabled = false )
          : mGcpDatabaseFileName( s_database_filename )
          , mGcpMasterDatabaseFileName( QString::null )
          , mGcp_coverage_name( s_coverage_name )
          , mGcpSrid( i_srid )
          , mGcpMasterSrid( -1 )
          , mGcp_coverage_title( "" )
          , mGcp_coverage_abstract( "" )
          , mGcp_coverage_copyright( "" )
          , mGcp_coverage_map_date( "" )
          , mGcp_points_table_name( s_points_table_name )
          , mLayer( raster_layer )
          , mGcp_enabled( b_Gcp_enabled )
          , mInputPoint( QgsPoint( 0.0, 0.0 ) )
          , mToPixel( false )
          , mOrder( 0 )
          , mReCompute( true )
          , mIdGcp( -1 )
          , mId_gcp_coverage( -1 )
          , mId_gcp_cutline( -1 )
          , mTransformParam( -1 )
          , mResamplingMethod( "Lanczos" )
          , mRasterYear( 0 )
          , mRasterScale( 0 )
          , mRasterNodata( -1 )
          , mCompressionMethod( "DEFLATE" )
          , mGcpPointsFileName( QString::null )
          , mGcpBaseFileName( QString::null )
          , mRasterFilePath( QString::null )
          , mRasterFileName( QString::null )
          , mModifiedRasterFileName( QString::null )
          , mGcp_coverage_name_base( QString::null )
          , mError( QString::null )
          , mSqlDump( false )
          , mDatabaseDump( false )
          , mDatabaseValid( false )
          , mParseString( ";#;#" )
          , spatialite_cache( nullptr )
          , db_handle( nullptr )
          , mUsed_database_filename( QString::null )
          , mUsed_database_filename_count( 0 )
      {}

      QString mGcpDatabaseFileName;
      QString mGcpMasterDatabaseFileName;
      QString mGcp_coverage_name; // file without extention Lower-Case
      int mGcpSrid;
      int mGcpMasterSrid;
      QString mGcp_coverage_title;
      QString mGcp_coverage_abstract;
      QString mGcp_coverage_copyright;
      QString mGcp_coverage_map_date; 
      QString mGcp_points_table_name;
      QgsRasterLayer *mLayer;
      bool mGcp_enabled;
      QgsPoint mInputPoint;
      bool mToPixel;
      int mOrder;
      bool mReCompute;
      int mIdGcp;
      int mId_gcp_coverage;
      int mId_gcp_cutline;
      int mTransformParam;
      QString mResamplingMethod;
      int mRasterYear;
      int mRasterScale;
      int mRasterNodata;
      QString mCompressionMethod;
      QString mGcpPointsFileName;
      QString mGcpBaseFileName;
      QString mRasterFilePath; // Path to the file, without file-name
      QString mRasterFileName; // file-name without path
      QString mModifiedRasterFileName; // Georeferenced File-Name
      QString mGcp_coverage_name_base; // Original Spelling of mGcp_coverage_name
      QString mError;
      bool mSqlDump;
      bool mDatabaseDump;
      bool mDatabaseValid;
      QString mParseString;
      QMap<int, QString> gcp_coverages;
      void *spatialite_cache;
      sqlite3* db_handle;
      QString mUsed_database_filename;
      int mUsed_database_filename_count;
      QMap<QString, QString> map_providertags;
    };
    /**
     * Creates a Spatialite-Database storing the Gcp-Points
     *  A 'gcp_convert' TABLE can also be created to use the Spatialite GCP_Transform/Compute logic
     * @note the Spatialite running must be compiled with './configure --enable-gcp=yes'
     *  QGIS can be compiled without this setting since only SQL-Queries are being used
     *  The Tables will only be created/updated after checking that 'SELECT HasGCP()' returns true
     * <a href="https://www.gaia-gis.it/fossil/libspatialite/wiki?name=Ground+Control+Points </a>
     * @note only when 'gcp_enable' is true, will the POINT be used
     *  Based on the value of id_order, only those points will be used
     * @note
     *  - Conditions to set mDatabaseValid to true:
     *  -> if a Database is being created only (and has no coverages) : will be set to true
     *  -> if a Database is being created with a new coverages, and the coverage was created : will be set to true
     *  -> if a Database is being read (without request for a specific coverage) and a gcp_coverage TABLE exists : will be set to true
     *  -> if a Database is being read for a specific coverage and  that coverage exists : will be set to true
     * @see getGcpConvert
     * @param s_coverage_name name of map to use
     * @return true if database exists
     */
    static bool createGcpDb( GcpDbData* parms_GcpDbData );
    /**
     * Updates the gcp_coverage TABLE
     *  - General Data about the coverage will be stored
     * @note the Spatialite running must be compiled with './configure --enable-gcp=yes'
     *  - Data such as the extent of the enable POINTS will be read and stored
     *  - The srid of the POINTs TABLE will be (again) stored
     *  - Title, Abstract and Copyright will be stored
     *  - General settings will be stored
     * @note
     *  A calling Application should set thes values in the 'GcpDbData' structure
     *  - when any settings have changed
     * @note
     *  - 'GcpDbData' structure will be filled with the values found in the Database
     *  -> and should be 'transfered' to the member variablen of the Application
     * @note
     *  this function will be called
     *  -> durring an open, change or close
     * @see createGcpDb
     * @param parms_GcpDbData  with full path the the file to be created and srid to use
     * @return true if database exists
     */
    static bool updateGcpDb( GcpDbData* parms_GcpDbData );
    /**
     * Stores the Thin Plate Spline/Polynomial Coefficients of the Gcp-Points
     *  using Spatialite GCP_Transform/Compute
     * @note the Spatialite running must be compiled with './configure --enable-gcp=yes'
     *  QGIS can be compiled without this setting since only SQL-Queries are being used
     *  The Tables will only be created/updated after checking that 'SELECT HasGCP()' returns true
     * <a href="https://www.gaia-gis.it/fossil/libspatialite/wiki?name=Ground+Control+Points </a>
     * @note only when 'gcp_enable' is true, will the POINT be used
     *  Based on the value of id_order, only those points will be used
     * @see getGcpConvert
     * @param s_coverage_name name of map to use
     * @return true if database exists
     */
    static bool updateGcpCompute( GcpDbData* parms_GcpDbData );
    /**
     * Converts a Geometry based on the values given by Polynomial Coefficients of the Gcp-Points
     *  using Spatialite GCP_Transform/Compute
     * @note the Spatialite running must be compiled with './configure --enable-gcp=yes'
     *  QGIS can be compiled without this setting since only SQL-Queries are being used
     *  The Tables will only be created/updated after checking that 'SELECT HasGCP()' returns true
     * <a href="https://www.gaia-gis.it/fossil/libspatialite/wiki?name=Ground+Control+Points </a>
     * @note only when 'gcp_enable' is true, will the POINT be used
     *  Based on the value of id_order, only those points will be used
     * @see getGcpConvert
     * @param s_coverage_name name of map to use
     * @return true if database exists
     */
    static bool updateGcpTranslate( GcpDbData* parms_GcpDbData );
    /**
     * Convert Pixel/Map value to Map/Pixel value
     *  using Spatialite GCP_Transform/Compute
     * @note the Spatialite running must be compiled with './configure --enable-gcp=yes'
     *  QGIS can be compiled without this setting since only SQL-Queries are being used
     *  The SQL-Queries will only be called after checking that 'SELECT HasGCP()' returns true
     * <a href="https://www.gaia-gis.it/fossil/libspatialite/wiki?name=Ground+Control+Points </a>
     * @note only PolynomialOrder1, PolynomialOrder2, PolynomialOrder3 and ThinPlateSpline aresupported
     * @see getGcpTransformParam
     * @see updateGcpCompute
     * @param s_coverage_name name of map to search for
     * @param input_point point to convert [not used when id_gcp > 0]
     * @param b_toPixel true=convert Map-Point to Pixel Point ; false: Convert Pixel-Point to Map-Point
     * @param i_order 0-3 [ThinPlateSpline, PolynomialOrder1, PolynomialOrder2, PolynomialOrder3]
     * @param b_reCompute re-calculate value by reading al enable points, othewise read stored values in gcp_convert.
     * @param id_gcp read value from specific gcp point [input_point will be ignored]
     * @return QgsPoint of result (0,0 when invalid)
     */
    static QgsPoint getGcpConvert( GcpDbData* parms_GcpDbData );
    /**
     * Create an empty Gcp-Master Database
     *  - a Database to be used to reference POINTs to assist while Georeferencing
     *  -> all POINTs must be of the same srid
     *  --> that are listed in the central 'gcp_coverages' table
     *  The intention a generaly reusable Database for Well-Known-Points
     *  - also readable for non-spatial applications
     * @note Fields
     *  - id_gcp : should be unique
     *  - name : should be a discriptive name for the POINT
     *  - notes : placeholder for any other information that might ge useful
     *  - text : placeholder for any other information that might ge useful
     *  - belongs_to_01 / 02 : possible (text) placeholder to where the POINT belongs to
     *  - valid_since / until : the timeframe where this POINT is valid (usable)
     *  - map_x / y, srid : the position in a non-spatial envoirment
     *  - gcp_type: a general classification of what type
     * @note TRIGGERS
     *  - will be created to set fields with default (readable) values from the given POINT
     *  -> map_x: result of ST_X(gcp_point)
     *  -> map_y: result of ST_Y(gcp_point)
     *  -> srid: result of ST_SRID(gcp_point)
     *  -> order_selected: 0 if nothing is given [0-4]
     *  ---> see the CREATE Statement of 'gcp_master' for the supported values
     *  -> valid_since: '0001-01-01' if nothing is given
     *  -> valid_until: '3000-01-01' if nothing is given
     *  -> gcp_type: 0 if nothing is given
     *  --> many SpatialView's are created to SELECT only specfic gcp_points based on the 'gcp_type'
     *  ---> see the CREATE Statement of 'gcp_master' for the supported values
     * @note
     *  - the 'GcpDbData->mGcp_coverage_name' SHOULD remain 'QString::null' so that the default name
     *  -> 'gcp_master' will be used
     *  --> this application will assume that the default name is being used
     *  --> POINTs from this 'gcp_master' Database can then be 'imported'
     * @note
     *  - When the Spatialite Gcp-Logic is being used and returns usefull results
     *  -> i.e. when adding new gcp-points and the created pixel-points are very near where they are expected
     * @param parms_GcpDbData  with full path the the file to be created and srid to use
     * @return true if database exists
     */
    static bool createGcpMasterDb( GcpDbData* parms_GcpDbData );
    /**
     * Bulk INSERT (or UPDATE) of Gcp-coverages Points TABLE
     *  - execute INSERT (or UPDATE)   commands stored in 
     *  -- retrieving created id_gcp [INSERT only]
     * @note       
     * QMap<int, QString> gcp_coverages of the parms_GcpDbData structure
     *  - the QString will contain the sql-command
     *  -- it will be replaced with the returned id_gcp
     *  - the 'int' will be used by the calling function
     *  -- to identify to what the created id_gcp belongs to
     * @note
     * mOrder of the parms_GcpDbData structure
     *  - will contain what type of Sql-Command will be used
     *  -- 1=INSERT (will call getSqliteSequence after each INSERT)
     *  -- 2=UPDATE (will not call SqliteSequence)
     * @see getSqliteSequence
     * @param parms_GcpDbData  which will contain a spatialite connection (with mParseString to use and if Spatialite Gcp-logic is active)
     * @return true if all commands where executed correctly
     */
    static bool bulkGcpPointsInsert( GcpDbData* parms_GcpDbData );
  private:
    /**
     * Creates sql to CREATE a 'gcp_points_||gcp_coverage' TABLE, TRIGGERS and VIEWs for the coresponding gcp_coverage entry
     *  - goal is the maintainence of the field-name syntax is in one place
     *  -- any changes in the column-names are done here
     * @note Format of each sa_tables entry: 'id_gcp_coverage+mParseString+srid+mParseString+coverage_name'
     *  - this is used both for a single entry OR when
     * @param parms_GcpDbData  which will contain a spatialite connection (but otherwise not used)
     * @param sa_tables  list of formatted entries
     * @return sa_sql_commands QStringList of all sql-commands to be executed
     */
    static QStringList createGcpSqlPointsCommands( GcpDbData* parms_GcpDbData, QStringList sa_tables );
    /**
     * Creates sql for gcp_coverage, gcp_compute and cutline TABLEs
     *  - goal is the maintainence of the field-name syntax is in one place
     *  -- any changes in the column-names are done here
     * @note 'CREATE_COVERAGE' 'CREATE_COVERAGES+mParseString+srid'
     *  - builds CREATE Statement for 'gcp_coverages' TABLE
     *  -> when Spatialite Gcp-logic is active: also CREATE Statment for 'gcp_compute' TABLE
     * @note 'UPDATE_COVERAGE_EXTENT': 'UPDATE_COVERAGE_EXTENT+mParseString+id_gcp_coverage+mParseString+coverage_name"
     *  - builds UPDATE Statement for 'gcp_coverages' TABLE
     *  -> will build Sql-Subqueries to the corresponding 'gcp_points_' TABLE returning extent min/max  x+y values
     *  -> will build Sql-Subquery to the corresponding 'create_cutline_polygons' TABLE returning id_cutline
     *  --> WHERE cutline_type = 77 for matching id_gcp_coverage
     *  ---> when more than 1 is found: max(id_cutline) is returned
     *  ---> when  none are found: -1 is returned
     * @note 'SELECT_COVERAGES': 'SELECT_COVERAGES+mParseString+coverage_name'
     *  - builds SELECT Statement for all columns of 'gcp_coverages' TABLE
     *  -> 2 results returned: 'id_gcp_coverage' and all values (including 'id_gcp_coverage') seperated with 'parms_GcpDbData->mParseString'
     *  --> used to build 'parms_GcpDbData->gcp_coverages' [QMap<int, QString>]
     *  -> when 'coverage_name' is not empty: only tha coverage will be returned, otherwise all coverages
     * @note 'INSERT_COVERAGES': 'SELECT_COVERAGES+mParseString+VALUES([values of all columns properly formatted])'
     *  - builds INSERT Statement for all 'gcp_coverages' entries for Sql-Dump
     * @note 'INSERT_COVERAGE': 'SELECT_COVERAGES+mParseString+VALUES([values of all columns (without id_gcp_coverage) properly formatted])'
     *  - builds INSERT Statement for new 'gcp_coverages' entry
     * @note 'CREATE_CUTLINES': 'CREATE_CUTLINES+mParseString+srid'
     * @note 'INSERT_COMPUTE': INSERT_COMPUTE+mParseString+id_gcp_coverage+mParseString+srid+mParseString+coverage_name+mParseString+image_max_x+mParseString+image_max_y'
     *  - builds INSERT Statement for new 'gcp_compute' entry
     * @note 'UPDATE_COMPUTE' : 'UPDATE_COMPUTE+mParseString+id_gcp_coverage+mParseString+srid+mParseString+coverage_name+mParseString+image_max_x+mParseString+image_max_y'
     *  -> when Spatialite Gcp-logic is active
     * @param parms_GcpDbData  which will contain a spatialite connection (with mParseString to use and if Spatialite Gcp-logic is active)
     * @return sa_sql_commands QStringList of all sql-commands to be executed
     */
    static QStringList createGcpSqlCoveragesCommands( GcpDbData* parms_GcpDbData, QStringList sa_tables );
    /**
     * Creates sql for gcp_coverage, gcp_compute and cutline TABLEs
     *  - goal is the maintainence of the field-name syntax is in one place
     *  -- any changes in the column-names are done here
     * @note 'CREATE_MASTER_TABLE' 'CREATE_MASTER_TABLE+mParseString+gcp_master+mParseString+srid'
     *  - builds CREATE Statement for 'gcp_master' TABLE
     * @note 'CREATE_MASTER_VIEWS' 'CREATE_MASTER_VIEWS+mParseString+srid'
     *  - builds CREATE Statement for 'gcp_master' TABLE
     * @param parms_GcpDbData  which will contain a spatialite connection (with mParseString to use and if Spatialite Gcp-logic is active)
     * @return sa_sql_commands QStringList of all sql-commands to be executed
     */
    static QStringList createGcpSqlMasterCommands( GcpDbData* parms_GcpDbData, QStringList sa_tables );
    /**
     * Write Sql-Script to file
     *  - dealing with common errors that may occur
     * @note
     *  - Original use was to replace '.db' with '.sql'
     *  -> when the directory also contained a '.db', that too was replaced
     *  --> resulting in a QFile::OpenError [5] during open
     *  - Since this functionality is often being used [4 times at the time of writing]
     *  -> a single version with proper replacement (file-name only) and Error controls was made
     * @note
     *  - This Function will add the following to the Sql-Statements:
     *  -> a header [in sql comments] showing how the script should be called
     *  -> 'BEGIN;' and 'COMMIT;' Statement inserted around the Sql-Statements
     *  -> A general Start/End Message with a now time Statement around the Sql-Statements
     * @param s_source_filename source file (with path) to use as a base for the output file
     * @param s_sql_output collection of Sql-Statments to be dumped to file
     * @param s_extention the extention to use to replace the source file extention with
     * @return file_result result of QFile error() by open/output commands
     */
    static QFile::FileError writeGcpSqlDump( QString s_source_filename, QString s_sql_output = QString::null, QString s_extention = "sql" );
    /**
     * Retrieve last use Segquence Number of TABLE
     *  - retrieve last used Primary-Key value after INSERT
     *  -- SELECT seq FROM sqlite_sequence WHERE (name="gcp_points_1909.straube_blatt_iii_a.4000")
     * @param parms_GcpDbData  which will contain a spatialite connection (with mParseString to use and if Spatialite Gcp-logic is active)
     * @param s_tablename TABLE name to retieve 
     * @return id_seq  last used Primary-Key value for the given TABLE
     */
    static int getSqliteSequence( GcpDbData* parms_GcpDbData, QString s_tablename );
    /**
     * Creates a new Spatialite-Database connection to a specific Database
     *  - opens sqlite3 connection with OPEN READWRITE and CREATE
     *  --> stores handle pointer in GcpDbData->db_handle
     *  - creating Spatialite cache
     *  --> stores cache pointer in GcpDbData->spatialite_cache
     *  - calls spatialite_init_ex using db_handle and spatialite_cache
     * @note stores Database-file name in GcpDbData->mUsed_database_filename
     *  - will return SQLITE_OK when
     *  -> a new connection has been made
     *  -> a connection exists for the given Database-file [will NOT create new connection]
     *  --> stores count of requested connections in GcpDbData->mUsed_database_filename_count
     *  - will return SQLITE_MISUSE when
     *  -> a connection exists for another Database-file [will NOT create new connection]
     * @note a long term connection is NOT intended
     *  - when the static function(s) are compleated, the connection should be closed
     *  - when one static function calls another static function (using the same database)
     *  --> it will use the same connection
     *  - each static function will (attempt) to open and close a connection
     *  --> only the first (for open) and last (for close) will be done
     *  - while open an connection to another Databse file is NOT permitted with the same parms_GcpDbData
     *  --> will return with SQLITE_MISUSE and the static function returns with error
     * @see spatialiteShutdown
     * @param parms_GcpDbData to store Database file-name, count, handle and cache
     * @param s_database_filename  Database file-name to open
     * @return SQLITE_OK or SQLITE_MISUSE see above
     */
    static int spatialiteInitEx( GcpDbData* parms_GcpDbData, QString s_database_filename );
    /**
     * Closes a Spatialite-Database connection to a specific Database
     *  - close sqlite3 connection will be called
     *  --> sets handle pointer to nullptr in GcpDbData->db_handle
     *  --> sets the Database-file name to QString::null in GcpDbData->mUsed_database_filename
     *  --> sets the Database-file counter to 0 in GcpDbData->mUsed_database_filename_count
     *  - spatialite_cleanup_ex is called to free the Spatialite cache
     *  --> sets cache pointer to nullptr in GcpDbData->spatialite_cache
     *  - calls spatialite_shutdown()
     * @note if more than one connection request exists (mUsed_database_filename_count > 1)
     *  - the close request will be ignored
     *  -> mUsed_database_filename_count will be reduced by 1 and returns true
     * @see spatialiteInitEx
     * @param parms_GcpDbData  to store Database file-name, count, handle and cache
     * @return true when  parms_GcpDbData is not NULL
     */
    static bool spatialiteShutdown( GcpDbData* parms_GcpDbData );
};

#endif
