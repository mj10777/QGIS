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
  * \since QGIS 3.0
  */
class QgsSpatiaLiteProviderGcpUtils
{
  public:
    /**
     * ConvertUnits [Length unit conversion]
     *  - based in valued used in Spatialite [gg_const.h]
     * \see ConvertLength
     * \since QGIS 3.0
     */
    enum ConvertUnits
    {
      Kilometer=0,
      Meter=1,
      Decimeter=2,
      Centimeter=3,
      Millimeter=4,
      International_Nautical_Mile=5,
      Inch=6,
      Feet=7,
      Yard=8,
      Mile=9,
      Fathom=10,
      Chain=11,
      Link=12,
      US_Inch=13,
      US_Feet=14,
      US_Yard=15,
      US_Chain=16,
      US_Mile=17,
      Indian_Yard=18,
      Indian_Feet=19,
      Indian_Chain=20
    };
    /**
     * ConvertLenth [converting length from one unit to another]
     *  - based on Spatialite function gaiaConvertLength [gg_geodesic.c]
     * \see ConvertUnits
     * \since QGIS 3.0
     */
    static bool ConvertLength(double inputValue, QgsSpatiaLiteProviderGcpUtils::ConvertUnits unitFrom, QgsSpatiaLiteProviderGcpUtils::ConvertUnits unitTo, double* outputValue );

    /**
     * Structure to transfer Data between an using application and the static functions
     *  - for use the the creation and reading of Gcp-specific Databases
     * \note a long term Database connection is NOT intended
     *  When calling from an Application
     *  - the Database connection should be closed when returned to the Application
     *  -> the use of the connection is only indended for internal use
     * \since QGIS 3.0
     */
    enum GcpDatabases
    {
      GcpCoverages,
      GcpMaster
    };
    struct GcpDbData
    {
      GcpDbData( QString sDatabaseFilename, QString sCoverageName = QString::null, int i_srid = INT_MIN )
          : mGcpDatabaseFileName( sDatabaseFilename )
          , mGcpMasterDatabaseFileName( QString::null )
          , mGcpCoverageName( sCoverageName )
          , mGcpSrid( i_srid )
          , mGcpMasterSrid( -1 )
          , mGcpCoverageTitle( "" )
          , mGcpCoverageAbstract( "" )
          , mGcpCoverageCopyright( "" )
          , mGcpCoverageMapDate( "" )
          , mGcpPointsTableName( QString::null )
          , mLayer( nullptr )
          , mGcpEnabled( false )
          , mInputPoint( QgsPoint( 0.0, 0.0 ) )
          , mToPixel( false )
          , mOrder( 0 )
          , mReCompute( true )
          , mIdGcp( -1 )
          , mIdGcpCoverage( -1 )
          , mIdGcpCutline( -1 )
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
          , mGcpCoverageNameBase( QString::null )
          , mError( QString::null )
          , mSqlDump( false )
          , mDatabaseDump( false )
          , mDatabaseValid( false )
          , mParseString( ";#;#" )
          , spatialite_cache( nullptr )
          , db_handle( nullptr )
          , mUsedDatabaseFilename( QString::null )
          , mUsedDatabaseFilenameCount( 0 )
          , mGcpMasterArea( 5 )
      {}
      GcpDbData( QString sDatabaseFilename, QString sCoverageName, int i_srid, QString sPointsTableName,  QgsRasterLayer *rasterLayer, bool bGcpEnabled = false )
          : mGcpDatabaseFileName( sDatabaseFilename )
          , mGcpMasterDatabaseFileName( QString::null )
          , mGcpCoverageName( sCoverageName )
          , mGcpSrid( i_srid )
          , mGcpMasterSrid( -1 )
          , mGcpCoverageTitle( "" )
          , mGcpCoverageAbstract( "" )
          , mGcpCoverageCopyright( "" )
          , mGcpCoverageMapDate( "" )
          , mGcpPointsTableName( sPointsTableName )
          , mLayer( rasterLayer )
          , mGcpEnabled( bGcpEnabled )
          , mInputPoint( QgsPoint( 0.0, 0.0 ) )
          , mToPixel( false )
          , mOrder( 0 )
          , mReCompute( true )
          , mIdGcp( -1 )
          , mIdGcpCoverage( -1 )
          , mIdGcpCutline( -1 )
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
          , mGcpCoverageNameBase( QString::null )
          , mError( QString::null )
          , mSqlDump( false )
          , mDatabaseDump( false )
          , mDatabaseValid( false )
          , mParseString( ";#;#" )
          , spatialite_cache( nullptr )
          , db_handle( nullptr )
          , mUsedDatabaseFilename( QString::null )
          , mUsedDatabaseFilenameCount( 0 )
          , mGcpMasterArea( 5 )
      {}

      QString mGcpDatabaseFileName;
      QString mGcpMasterDatabaseFileName;
      QString mGcpCoverageName; // file without extention Lower-Case
      int mGcpSrid;
      int mGcpMasterSrid;
      QString mGcpCoverageTitle;
      QString mGcpCoverageAbstract;
      QString mGcpCoverageCopyright;
      QString mGcpCoverageMapDate;
      QString mGcpPointsTableName;
      QgsRasterLayer *mLayer;
      bool mGcpEnabled;
      QgsPoint mInputPoint;
      bool mToPixel;
      int mOrder;
      bool mReCompute;
      int mIdGcp;
      int mIdGcpCoverage;
      int mIdGcpCutline;
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
      QString mGcpCoverageNameBase; // Original Spelling of mGcpCoverageName
      QString mError;
      bool mSqlDump;
      bool mDatabaseDump;
      bool mDatabaseValid;
      QString mParseString;
      QMap<int, QString> gcp_coverages;
      void *spatialite_cache;
      sqlite3* db_handle;
      QString mUsedDatabaseFilename;
      int mUsedDatabaseFilenameCount;
      QMap<QString, QString> map_providertags;
      double mGcpMasterArea;
    };
    /**
     * Creates a Spatialite-Database storing the Gcp-Points
     *  A 'gcp_convert' TABLE can also be created to use the Spatialite GCP_Transform/Compute logic
     * \note the Spatialite running must be compiled with './configure --enable-gcp=yes'
     *  QGIS can be compiled without this setting since only SQL-Queries are being used
     *  The Tables will only be created/updated after checking that 'SELECT HasGCP()' returns true
     * <a href="https://www.gaia-gis.it/fossil/libspatialite/wiki?name=Ground+Control+Points </a>
     * \note only when 'gcp_enable' is true, will the POINT be used
     *  Based on the value of id_order, only those points will be used
     * \note
     *  - Conditions to set mDatabaseValid to true:
     *  -> if a Database is being created only (and has no coverages) : will be set to true
     *  -> if a Database is being created with a new coverages, and the coverage was created : will be set to true
     *  -> if a Database is being read (without request for a specific coverage) and a gcp_coverage TABLE exists : will be set to true
     *  -> if a Database is being read for a specific coverage and  that coverage exists : will be set to true
     * \see getGcpConvert
     * \param sCoverageName name of map to use
     * \return true if database exists
     * \since QGIS 3.0
     */
    static bool createGcpDb( GcpDbData* parmsGcpDbData );
    /**
     * Updates the gcp_coverage TABLE
     *  - General Data about the coverage will be stored
     * \note the Spatialite running must be compiled with './configure --enable-gcp=yes'
     *  - Data such as the extent of the enable POINTS will be read and stored
     *  - The srid of the POINTs TABLE will be (again) stored
     *  - Title, Abstract and Copyright will be stored
     *  - General settings will be stored
     * \note
     *  A calling Application should set thes values in the 'GcpDbData' structure
     *  - when any settings have changed
     * \note
     *  - 'GcpDbData' structure will be filled with the values found in the Database
     *  -> and should be 'transfered' to the member variablen of the Application
     * \note
     *  this function will be called
     *  -> durring an open, change or close
     * \see createGcpDb
     * \param parmsGcpDbData  with full path the the file to be created and srid to use
     * \return true if database exists
     * \since QGIS 3.0
     */
    static bool updateGcpDb( GcpDbData* parmsGcpDbData );
    /**
     * Stores the Thin Plate Spline/Polynomial Coefficients of the Gcp-Points
     *  using Spatialite GCP_Transform/Compute
     * \note the Spatialite running must be compiled with './configure --enable-gcp=yes'
     *  QGIS can be compiled without this setting since only SQL-Queries are being used
     *  The Tables will only be created/updated after checking that 'SELECT HasGCP()' returns true
     * <a href="https://www.gaia-gis.it/fossil/libspatialite/wiki?name=Ground+Control+Points </a>
     * \note only when 'gcp_enable' is true, will the POINT be used
     *  Based on the value of id_order, only those points will be used
     * \see getGcpConvert
     * \param sCoverageName name of map to use
     * \return true if database exists
     * \since QGIS 3.0
     */
    static bool updateGcpCompute( GcpDbData* parmsGcpDbData );
    /**
     * Converts a Geometry based on the values given by Polynomial Coefficients of the Gcp-Points
     *  using Spatialite GCP_Transform/Compute
     * \note the Spatialite running must be compiled with './configure --enable-gcp=yes'
     *  QGIS can be compiled without this setting since only SQL-Queries are being used
     *  The Tables will only be created/updated after checking that 'SELECT HasGCP()' returns true
     * <a href="https://www.gaia-gis.it/fossil/libspatialite/wiki?name=Ground+Control+Points </a>
     * \note only when 'gcp_enable' is true, will the POINT be used
     *  Based on the value of id_order, only those points will be used
     * \see getGcpConvert
     * \param sCoverageName name of map to use
     * \return true if database exists
     * \since QGIS 3.0
     */
    static bool updateGcpTranslate( GcpDbData* parmsGcpDbData );
    /**
     * Convert Pixel/Map value to Map/Pixel value
     *  using Spatialite GCP_Transform/Compute
     * \note the Spatialite running must be compiled with './configure --enable-gcp=yes'
     *  QGIS can be compiled without this setting since only SQL-Queries are being used
     *  The SQL-Queries will only be called after checking that 'SELECT HasGCP()' returns true
     * <a href="https://www.gaia-gis.it/fossil/libspatialite/wiki?name=Ground+Control+Points </a>
     * \note only PolynomialOrder1, PolynomialOrder2, PolynomialOrder3 and ThinPlateSpline aresupported
     * \see getGcpTransformParam
     * \see updateGcpCompute
     * \param sCoverageName name of map to search for
     * \param inputPoint point to convert [not used when id_gcp > 0]
     * \param bToPixel true=convert Map-Point to Pixel Point ; false: Convert Pixel-Point to Map-Point
     * \param i_order 0-3 [ThinPlateSpline, PolynomialOrder1, PolynomialOrder2, PolynomialOrder3]
     * \param bReCompute re-calculate value by reading all enable points, othewise read stored values in gcp_convert.
     * \param id_gcp read value from specific gcp point [inputPoint will be ignored]
     * \return QgsPoint of result (0,0 when invalid)
     * \since QGIS 3.0
     */
    static QgsPoint getGcpConvert( GcpDbData* parmsGcpDbData );
    /**
     * Create an empty Gcp-Master Database
     *  - a Database to be used to reference POINTs to assist while Georeferencing
     *  -> all POINTs must be of the same srid
     *  --> that are listed in the central 'gcp_coverages' table
     *  The intention a generaly reusable Database for Well-Known-Points
     *  - also readable for non-spatial applications
     * \note Fields
     *  - id_gcp : should be unique
     *  - name : should be a discriptive name for the POINT
     *  - notes : placeholder for any other information that might ge useful
     *  - text : placeholder for any other information that might ge useful
     *  - belongs_to_01 / 02 : possible (text) placeholder to where the POINT belongs to
     *  - valid_since / until : the timeframe where this POINT is valid (usable)
     *  - map_x / y, srid : the position in a non-spatial environment
     *  - gcp_type: a general classification of what type
     * \note TRIGGERS
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
     * \note
     *  - the 'GcpDbData->mGcpCoverageName' SHOULD remain 'QString::null' so that the default name
     *  -> 'gcp_master' will be used
     *  --> this application will assume that the default name is being used
     *  --> POINTs from this 'gcp_master' Database can then be 'imported'
     * \note
     *  - When the Spatialite Gcp-Logic is being used and returns usefull results
     *  -> i.e. when adding new gcp-points and the created pixel-points are very near where they are expected
     * \param parmsGcpDbData  with full path the the file to be created and srid to use
     * \return true if database exists
     * \since QGIS 3.0
     */
    static bool createGcpMasterDb( GcpDbData* parmsGcpDbData );
    /**
     * Bulk INSERT (or UPDATE) of Gcp-coverages Points TABLE
     *  - execute INSERT (or UPDATE)   commands stored in
     *  -- retrieving created id_gcp [INSERT only]
     * \note
     * QMap<int, QString> gcp_coverages of the parmsGcpDbData structure
     *  - the QString will contain the sql-command
     *  -- it will be replaced with the returned id_gcp
     *  - the 'int' will be used by the calling function
     *  -- to identify to what the created id_gcp belongs to
     * \note
     * mOrder of the parmsGcpDbData structure
     *  - will contain what type of Sql-Command will be used
     *  -- 1=INSERT (will call getSqliteSequence after each INSERT)
     *  -- 2=UPDATE (will not call SqliteSequence)
     * \see getSqliteSequence
     * \param parmsGcpDbData  which will contain a spatialite connection (with mParseString to use and if Spatialite Gcp-logic is active)
     * \return true if all commands where executed correctly
     * \since QGIS 3.0
     */
    static bool bulkGcpPointsInsert( GcpDbData* parmsGcpDbData );
    /**
     * Given a meters Map-Unit value
     *  - a realistic Map-Unit value for the srid used will be returned
     * meters value: parmsGcpDbData->mGcpMasterArea
     * srid value:  parmsGcpDbData->mGcpSrid
     * \note
     *  - for Degrees a start position must be set
     *  -> parmsGcpDbData->mInputPoint
     * \see spatialiteInitEx
     * \param parmsGcpDbData  to store Database file-name, count, handle and cache
     * \return Map-Unit for the given srid
     * \since QGIS 3.0
     */
    static double metersToMapPoint( GcpDbData* parmsGcpDbData );
  private:
    /**
     * Creates sql to CREATE a 'gcp_points_||gcp_coverage' TABLE, TRIGGERS and VIEWs for the coresponding gcp_coverage entry
     *  - goal is the maintainence of the field-name syntax is in one place
     *  -- any changes in the column-names are done here
     * \note Format of each listTables entry: 'id_gcp_coverage+mParseString+srid+mParseString+coverage_name'
     *  - this is used both for a single entry OR when
     * \param parmsGcpDbData  which will contain a spatialite connection (but otherwise not used)
     * \param listTables  list of formatted entries
     * \return sa_sql_commands QStringList of all sql-commands to be executed
     * \since QGIS 3.0
     */
    static QStringList createGcpSqlPointsCommands( GcpDbData* parmsGcpDbData, QStringList listTables );
    /**
     * Creates sql for gcp_coverage, gcp_compute and cutline TABLEs
     *  - goal is the maintainence of the field-name syntax is in one place
     *  -- any changes in the column-names are done here
     * \note 'CREATE_COVERAGE' 'CREATE_COVERAGES+mParseString+srid'
     *  - builds CREATE Statement for 'gcp_coverages' TABLE
     *  -> when Spatialite Gcp-logic is active: also CREATE Statment for 'gcp_compute' TABLE
     * \note 'UPDATE_COVERAGE_EXTENT': 'UPDATE_COVERAGE_EXTENT+mParseString+id_gcp_coverage+mParseString+coverage_name"
     *  - builds UPDATE Statement for 'gcp_coverages' TABLE
     *  -> will build Sql-Subqueries to the corresponding 'gcp_points_' TABLE returning extent min/max  x+y values
     *  -> will build Sql-Subquery to the corresponding 'create_cutline_polygons' TABLE returning id_cutline
     *  --> WHERE cutline_type = 77 for matching id_gcp_coverage
     *  ---> when more than 1 is found: max(id_cutline) is returned
     *  ---> when  none are found: -1 is returned
     * \note 'SELECT_COVERAGES': 'SELECT_COVERAGES+mParseString+coverage_name'
     *  - builds SELECT Statement for all columns of 'gcp_coverages' TABLE
     *  -> 2 results returned: 'id_gcp_coverage' and all values (including 'id_gcp_coverage') seperated with 'parmsGcpDbData->mParseString'
     *  --> used to build 'parmsGcpDbData->gcp_coverages' [QMap<int, QString>]
     *  -> when 'coverage_name' is not empty: only tha coverage will be returned, otherwise all coverages
     * \note 'INSERT_COVERAGES': 'SELECT_COVERAGES+mParseString+VALUES([values of all columns properly formatted])'
     *  - builds INSERT Statement for all 'gcp_coverages' entries for Sql-Dump
     * \note 'INSERT_COVERAGE': 'SELECT_COVERAGES+mParseString+VALUES([values of all columns (without id_gcp_coverage) properly formatted])'
     *  - builds INSERT Statement for new 'gcp_coverages' entry
     * \note 'CREATE_CUTLINES': 'CREATE_CUTLINES+mParseString+srid'
     * \note 'INSERT_COMPUTE': INSERT_COMPUTE+mParseString+id_gcp_coverage+mParseString+srid+mParseString+coverage_name+mParseString+image_max_x+mParseString+image_max_y'
     *  - builds INSERT Statement for new 'gcp_compute' entry
     * \note 'UPDATE_COMPUTE' : 'UPDATE_COMPUTE+mParseString+id_gcp_coverage+mParseString+srid+mParseString+coverage_name+mParseString+image_max_x+mParseString+image_max_y'
     *  -> when Spatialite Gcp-logic is active
     * \param parmsGcpDbData  which will contain a spatialite connection (with mParseString to use and if Spatialite Gcp-logic is active)
     * \return sa_sql_commands QStringList of all sql-commands to be executed
     * \since QGIS 3.0
     */
    static QStringList createGcpSqlCoveragesCommands( GcpDbData* parmsGcpDbData, QStringList listTables );
    /**
     * Creates sql for gcp_coverage, gcp_compute and cutline TABLEs
     *  - goal is the maintainence of the field-name syntax is in one place
     *  -- any changes in the column-names are done here
     * \note 'CREATE_MASTER_TABLE' 'CREATE_MASTER_TABLE+mParseString+gcp_master+mParseString+srid'
     *  - builds CREATE Statement for 'gcp_master' TABLE
     * \note 'CREATE_MASTER_VIEWS' 'CREATE_MASTER_VIEWS+mParseString+srid'
     *  - builds CREATE Statement for 'gcp_master' TABLE
     * \param parmsGcpDbData  which will contain a spatialite connection (with mParseString to use and if Spatialite Gcp-logic is active)
     * \return sa_sql_commands QStringList of all sql-commands to be executed
     * \since QGIS 3.0
     */
    static QStringList createGcpSqlMasterCommands( GcpDbData* parmsGcpDbData, QStringList listTables );
    /**
     * Write Sql-Script to file
     *  - dealing with common errors that may occur
     * \note
     *  - Original use was to replace '.db' with '.sql'
     *  -> when the directory also contained a '.db', that too was replaced
     *  --> resulting in a QFile::OpenError [5] during open
     *  - Since this functionality is often being used [4 times at the time of writing]
     *  -> a single version with proper replacement (file-name only) and Error controls was made
     * \note
     *  - This Function will add the following to the Sql-Statements:
     *  -> a header [in sql comments] showing how the script should be called
     *  -> 'BEGIN;' and 'COMMIT;' Statement inserted around the Sql-Statements
     *  -> A general Start/End Message with a now time Statement around the Sql-Statements
     * \param sSourceFilename source file (with path) to use as a base for the output file
     * \param sSqlOutput collection of Sql-Statments to be dumped to file
     * \param sFileExtention the extention to use to replace the source file extention with
     * \return file_result result of QFile error() by open/output commands
     * \since QGIS 3.0
     */
    static QFile::FileError writeGcpSqlDump( QString sSourceFilename, QString sSqlOutput = QString::null, QString sFileExtention = "sql" );
    /**
     * Retrieve last use Segquence Number of TABLE
     *  - retrieve last used Primary-Key value after INSERT
     *  -- SELECT seq FROM sqlite_sequence WHERE (name="gcp_points_1909.straube_blatt_iii_a.4000")
     * \param parmsGcpDbData  which will contain a spatialite connection (with mParseString to use and if Spatialite Gcp-logic is active)
     * \param sTablename TABLE name to retieve
     * \return id_seq  last used Primary-Key value for the given TABLE
     * \since QGIS 3.0
     */
    static int getSqliteSequence( GcpDbData* parmsGcpDbData, QString sTablename );
    /**
     * Creates a new Spatialite-Database connection to a specific Database
     *  - opens sqlite3 connection with OPEN READWRITE and CREATE
     *  --> stores handle pointer in GcpDbData->db_handle
     *  - creating Spatialite cache
     *  --> stores cache pointer in GcpDbData->spatialite_cache
     *  - calls spatialite_init_ex using db_handle and spatialite_cache
     * \note stores Database-file name in GcpDbData->mUsedDatabaseFilename
     *  - will return SQLITE_OK when
     *  -> a new connection has been made
     *  -> a connection exists for the given Database-file [will NOT create new connection]
     *  --> stores count of requested connections in GcpDbData->mUsedDatabaseFilenameCount
     *  - will return SQLITE_MISUSE when
     *  -> a connection exists for another Database-file [will NOT create new connection]
     * \note a long term connection is NOT intended
     *  - when the static function(s) are compleated, the connection should be closed
     *  - when one static function calls another static function (using the same database)
     *  --> it will use the same connection
     *  - each static function will (attempt) to open and close a connection
     *  --> only the first (for open) and last (for close) will be done
     *  - while open an connection to another Databse file is NOT permitted with the same parmsGcpDbData
     *  --> will return with SQLITE_MISUSE and the static function returns with error
     * \see spatialiteShutdown
     * \param parmsGcpDbData to store Database file-name, count, handle and cache
     * \param sDatabaseFilename  Database file-name to open
     * \return SQLITE_OK or SQLITE_MISUSE see above
     * \since QGIS 3.0
     */
    static int spatialiteInitEx( GcpDbData* parmsGcpDbData, QString sDatabaseFilename );
    /**
     * Closes a Spatialite-Database connection to a specific Database
     *  - close sqlite3 connection will be called
     *  --> sets handle pointer to nullptr in GcpDbData->db_handle
     *  --> sets the Database-file name to QString::null in GcpDbData->mUsedDatabaseFilename
     *  --> sets the Database-file counter to 0 in GcpDbData->mUsedDatabaseFilenameCount
     *  - spatialite_cleanup_ex is called to free the Spatialite cache
     *  --> sets cache pointer to nullptr in GcpDbData->spatialite_cache
     *  - calls spatialite_shutdown()
     * \note if more than one connection request exists (mUsedDatabaseFilenameCount > 1)
     *  - the close request will be ignored
     *  -> mUsedDatabaseFilenameCount will be reduced by 1 and returns true
     * \see spatialiteInitEx
     * \param parmsGcpDbData  to store Database file-name, count, handle and cache
     * \return true when  parmsGcpDbData is not NULL
     * \since QGIS 3.0
     */
    static bool spatialiteShutdown( GcpDbData* parmsGcpDbData );
    /**
     * Retrieve TIFFTAG Information from a tiff file and can store the result
     *  TIFFTAG_DOCUMENTNAME, IMAGEDESCRIPTION and COPYWRITE
     *  - can be used to set Title, Abstract and KeywordsList
     * \note will only be called when provider == "gdal"
     * \note a 'copywrite' should be added to store the value of TIFFTAG_COPYWRITE
     *  - which is often a requirement of OpenData sources
     * @see setDataProvider
     * @param rasterLayerbeing created
     * @param b_SetTags when true: will set Title, Abstract and KeywordsList from TIFFTAG_DOCUMENTNAME, IMAGEDESCRIPTION and COPYWRITE if found
     * @return QMap<QString, QString> TIFFTAG-Parm, TIFFTAG-Value
     * \since QGIS 3.0
     */
    static QMap<QString, QString> readProvidertags( QgsRasterLayer *rasterLayer, bool bSetTags = false );
};

#endif
