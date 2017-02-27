/***************************************************************************
     QgsSpatiaLiteProviderGcpUtils.cpp
     --------------------------------------
    Date                 : 2016-12-14
    Copyright            : (C) 2016 by Mark Johnson, Berlin Germany
    Email                : mj10777@googlemail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QStringList>
#include <QDebug>

#include "qgsspatialiteprovidergcputils.h"

//-----------------------------------------------------------
//  QgsSpatiaLiteProviderGcpUtils functions
//-----------------------------------------------------------
bool QgsSpatiaLiteProviderGcpUtils::createGcpDb( GcpDbData* parms_GcpDbData )
{
  if ( !parms_GcpDbData )
    return false;
  parms_GcpDbData->mDatabaseValid = false;
  if (( parms_GcpDbData->mGcpDatabaseFileName.isEmpty() ) && ( !parms_GcpDbData->mRasterFileName.isEmpty() ) )
  { // if no Gcp-Database is given, use the Parent-Directory-Name of the Raster as the default name and store it in that Directory
    QFileInfo raster_file( parms_GcpDbData->mRasterFileName );
    parms_GcpDbData->mGcpDatabaseFileName = QString( "%1/%2.maps.gcp.db" ).arg( raster_file.canonicalPath() ).arg( raster_file.absoluteDir().dirName() );
  }
  QString  s_database_filename = parms_GcpDbData->mGcpDatabaseFileName;
  QString s_coverage_name = parms_GcpDbData->mGcp_coverage_name;
  bool b_create_empty_database = false;
  bool b_database_exists = QFile( s_database_filename ).exists();
  if (( s_coverage_name.isNull() ) || ( s_coverage_name == "gcp_coverage" ) )
  { // Read existing Database and collect all coverages
    if ( !b_database_exists )
    { // Create new Database
      b_create_empty_database = true;
    }
    s_coverage_name = QString::null;
  }
  if ( s_coverage_name.isEmpty() )
  {
    s_coverage_name = QString::null;
  }
  int i_srid = parms_GcpDbData->mGcpSrid;
  QString s_points_table_name = parms_GcpDbData->mGcp_points_table_name;
  QString s_points_file_name = parms_GcpDbData->mGcpPointsFileName;
  QString s_RasterFileName = parms_GcpDbData->mRasterFileName;
  QString s_GCPbaseFileName = parms_GcpDbData->mGcpBaseFileName;
  QString s_RasterFilePath = parms_GcpDbData->mRasterFilePath;
  QString s_ModifiedRasterFileName = parms_GcpDbData->mModifiedRasterFileName;
  bool b_gcp_enabled = parms_GcpDbData->mGcp_enabled;
  QgsRasterLayer *raster_layer = parms_GcpDbData->mLayer;
  int i_TransformParam = parms_GcpDbData->mTransformParam;
  QString s_ResamplingMethod = parms_GcpDbData->mResamplingMethod;
  QString s_CompressionMethod = parms_GcpDbData->mCompressionMethod;
  int i_RasterNoData = parms_GcpDbData->mRasterNodata;
  int i_RasterScale = parms_GcpDbData->mRasterScale;
  QFile points_file;
  int id_gcp_coverage = -1;
  int id_cutline = -1;
  // qDebug() << QString( "-I-> QgsSpatiaLiteProviderGcpUtils::createGcpDb -0- GCPdatabaseFileName[%1] " ).arg( s_database_filename );
  // After being opened (with CREATE), the file will exist, so store if it exist now
  //qDebug() << QString( "-I-> QgsSpatiaLiteProviderGcpUtils::createGcpDb[%6] dump[%1] empty[%2] create[%3] coverage[%4] database[%5]" ).arg( parms_GcpDbData->mDatabaseDump ).arg( b_create_empty_database ).arg( b_database_exists ).arg( s_coverage_name ).arg( s_database_filename ).arg( i_srid );
  if (( b_database_exists ) && ( b_create_empty_database ) )
  {
    parms_GcpDbData->mError = QString( "Failed to create database [file exists]: rc=%1 database[%2]" ).arg( 1 ).arg( s_database_filename );
    return false;
  }
  QString s_sql_newline = QString( "---------------" );
  QString s_sql_output = QString::null;
  b_gcp_enabled = false;
  bool b_cutline_geometries_create = false;
  bool b_spatial_ref_sys_create = false;
  bool b_gcp_coverage = false;
  bool b_gcp_compute = false;
  bool b_gcp_coverage_create = false;
  bool b_import_points = false;
  QString s_coverage_title = "";
  QString s_coverage_abstract = "";
  QString s_coverage_copyright = "";
  QString s_map_date = "";
  QString s_sql_select_coverages = QString::null;
  int i_image_max_x = 0;
  int i_image_max_y = 0;
  double d_extent_minx = 0.0;
  double d_extent_miny = 0.0;
  double d_extent_maxx = 0.0;
  double d_extent_maxy = 0.0;
  if ( !s_coverage_name.isNull() )
  { // Reading a gcp_points TABLE
    s_points_table_name = QString( "gcp_points_%1" ).arg( s_coverage_name );
    points_file.setFileName( s_points_file_name );
    if ( raster_layer )
    {
      parms_GcpDbData->map_providertags = QgsRasterLayer::read_providertags( raster_layer, true );
      i_image_max_x = raster_layer->width();
      i_image_max_y = raster_layer->height();
      s_coverage_title = raster_layer->title();
      s_coverage_abstract = raster_layer->abstract();
      s_coverage_copyright = raster_layer->keywordList();
      // qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -1- isSpatial[%1] " ).arg( raster_layer->isSpatial() );
      // if (!raster_layer->isSpatial()) [isSpatial() returns true ??]
      if ( !parms_GcpDbData->map_providertags.value( "TIFFTAG_DOCUMENTNAME" ).isEmpty() )
      {
        // qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -1- TIFFTAG_DOCUMENTNAME[%1] " ).arg( parms_GcpDbData->map_providertags.value( "TIFFTAG_DOCUMENTNAME" ) );
      }
      if ( !parms_GcpDbData->map_providertags.value( "TIFFTAG_IMAGEDESCRIPTION" ).isEmpty() )
      {
        // qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -2- TIFFTAG_IMAGEDESCRIPTION[%1] " ).arg( parms_GcpDbData->map_providertags.value( "TIFFTAG_IMAGEDESCRIPTION" ) );
      }
      QFileInfo raster_file( QString( "%1" ).arg( s_RasterFileName ) );
      if ( raster_file.exists() )
      {
        QFile world_file( QString( "%1/%2" ).arg( s_RasterFilePath ).arg( QString( "%1.%2" ).arg( raster_file.completeBaseName() ).arg( "tfw" ) ) );
        if ( !world_file.exists() )
        {
          world_file.open( QIODevice::WriteOnly );
          QTextStream  world_file_out( &world_file );
          // A World file to emulate EPSG:3395 (WGS 84 / World Mercator) for gdal cutlines using create_mercator_polygons
          world_file_out << QString( "%1\n%2\n%2\n-%1\n%2\n%2" ).arg( "1.00000000000000" ).arg( "0.00000000000000" );
          world_file_out.flush();
          world_file.close();
        }
      }
    }
  }
  else
  { // Reading only the Database gcp-coverages
    if ( !b_create_empty_database )
    {
      s_points_table_name = QString::null;
      s_points_file_name = QString::null;
      s_RasterFileName = QString::null;
      s_GCPbaseFileName = QString::null;
      s_RasterFilePath = QString::null;
      s_ModifiedRasterFileName = QString::null;
      b_gcp_enabled = false;
      raster_layer = nullptr;
      i_TransformParam = 0;
      s_ResamplingMethod = QString::null;
      s_CompressionMethod = QString::null;
    }
    if ( !b_database_exists )
    { // Database will be created
      // return b_database_exists;
    }
  }
  if (( !b_create_empty_database ) && ( !b_database_exists ) && ( parms_GcpDbData->mDatabaseDump ) )
  { // Cannot dump a Databse that does not exist
    return b_database_exists;
  }
  // base on code found in 'QgsOSMDatabase::createSpatialTable'
  sqlite3* db_handle;
  char *errMsg = nullptr;
  // open database
  int ret = QgsSpatiaLiteProviderGcpUtils::spatialiteInitEx( parms_GcpDbData, s_database_filename );
  if ( ret != SQLITE_OK )
  {
    parms_GcpDbData->mError = QString( "Failed to open database: rc=%1 database[%2]" ).arg( ret ).arg( s_database_filename );
    if ( ret != SQLITE_MISUSE )
    { // 'SQLITE_MISUSE': in use with another file, do not close
      QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
    }
    return false;
  }
  // To simplfy the useage.
  db_handle = parms_GcpDbData->db_handle;
  sqlite3_stmt* stmt;
  // Start TRANSACTION
  ret = sqlite3_exec( db_handle, "BEGIN", NULL, NULL, 0 );
  Q_ASSERT( ret == SQLITE_OK );
  Q_UNUSED( ret );
  // SELECT HasGCP(), (SELECT count(name) FROM sqlite_master WHERE ((type='table') AND (name='gcp_coverages'))) AS has_gcp_coverages;
  QString s_sql = QString( "SELECT HasGCP(), (SELECT count(name) FROM sqlite_master WHERE ((type='table') AND (name='gcp_coverages'))) AS has_gcp_coverages," );
  s_sql += QString( " (SELECT count(name) FROM sqlite_master WHERE ((type='table') AND (name='gcp_compute'))) AS has_gcp_compute," );
  s_sql += QString( " (SELECT count(name) FROM sqlite_master WHERE ((type='table') AND (name='create_cutline_polygons'))) AS has_cutline_geometries," );
  s_sql += QString( " (SELECT count(name) FROM sqlite_master WHERE ((type='table') AND (name='spatial_ref_sys'))) AS has_spatial_ref_sys" );
  ret = sqlite3_prepare_v2( db_handle, s_sql.toUtf8().constData(), -1, &stmt, NULL );
  if ( ret != SQLITE_OK )
  { //  rc=1 [no such function: HasGCP] is not an error [older spatialite version]
    if ( QString( "no such function: HasGCP" ).compare( QString( "%1" ).arg( sqlite3_errmsg( db_handle ) ) ) != 0 )
    {
      parms_GcpDbData->mError = QString( "Unable to SELECT HasGCP(): rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql );
      qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -1- [%1] " ).arg( parms_GcpDbData->mError );
    }
  }
  int i_spatialite_gcp_enabled = 0;
  int i_gcp_coverage = -1;
  int i_gcp_compute = -1;
  int i_cutline_geometries = -1;
  int i_spatial_ref_sys_create = -1;
  while ( sqlite3_step( stmt ) == SQLITE_ROW )
  {
    if ( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL )
      i_spatialite_gcp_enabled = sqlite3_column_int( stmt, 0 );
    if ( sqlite3_column_type( stmt, 1 ) != SQLITE_NULL )
      i_gcp_coverage = sqlite3_column_int( stmt, 1 );
    if ( sqlite3_column_type( stmt, 2 ) != SQLITE_NULL )
      i_gcp_compute = sqlite3_column_int( stmt, 2 );
    if ( sqlite3_column_type( stmt, 3 ) != SQLITE_NULL )
      i_cutline_geometries = sqlite3_column_int( stmt, 3 );
    if ( sqlite3_column_type( stmt, 4 ) != SQLITE_NULL )
      i_spatial_ref_sys_create = sqlite3_column_int( stmt, 4 );
  }
  sqlite3_finalize( stmt );
  if ( i_spatialite_gcp_enabled > 0 )
    b_gcp_enabled = true;
  if ( i_gcp_coverage > 0 )
    b_gcp_coverage = true;
  if ( i_gcp_compute > 0 )
    b_gcp_compute = true;
  if ( i_cutline_geometries > 0 )
    b_cutline_geometries_create = true;
  if ( i_spatial_ref_sys_create > 0 )
    b_spatial_ref_sys_create = true;
  parms_GcpDbData->mGcp_enabled = b_gcp_enabled;
  if ( !b_spatial_ref_sys_create )
  { // no spatial_ref_sys, call InitSpatialMetadata
    s_sql = QString( "SELECT InitSpatialMetadata(0)" );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      parms_GcpDbData->mError = QString( "Unable to InitSpatialMetadata: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -2- [%1] " ).arg( parms_GcpDbData->mError );
      sqlite3_free( errMsg );
      // End TRANSACTION
      ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
      Q_ASSERT( ret == SQLITE_OK );
      Q_UNUSED( ret );
      QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
      return false;
    }
    b_spatial_ref_sys_create = true;
  }
  // End TRANSACTION
  ret = sqlite3_exec( db_handle, "COMMIT", NULL, NULL, 0 );
  Q_ASSERT( ret == SQLITE_OK );
  Q_UNUSED( ret );
  if ( !b_gcp_coverage )
  { // TABLE gcp_coverage does not exist, create it
    QStringList sa_tables; // Format: CREATE_COVERAGE;#;#i_srid
    sa_tables.append( QString( "%2%1%3" ).arg( parms_GcpDbData->mParseString ).arg( "CREATE_COVERAGE" ).arg( i_srid ) );
    if ( sa_tables.size() > 0 )
    {
      QStringList sa_sql_commands = QgsSpatiaLiteProviderGcpUtils::createGcpSqlCoveragesCommands( parms_GcpDbData, sa_tables );
      if ( sa_sql_commands.size() > 0 )
      {
        ret = sqlite3_exec( db_handle, "BEGIN", NULL, NULL, 0 );
        Q_ASSERT( ret == SQLITE_OK );
        Q_UNUSED( ret );
        for ( int i_list = 0;i_list < sa_sql_commands.size();i_list++ )
        { // raster entry has been added to gcp_covarage, create corresponding tables
          s_sql = sa_sql_commands.at( i_list );
          if ( parms_GcpDbData->mSqlDump )
          {
            s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
          }
          ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
          if ( ret != SQLITE_OK )
          {
            parms_GcpDbData->mError = QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb: Failed while creating gcp_coverages table for [%1]: rc=%2 [%3] sql[%4]\n" ).arg( i_list ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
            qDebug() << parms_GcpDbData->mError;
            sqlite3_free( errMsg );
            // End TRANSACTION
            ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
            Q_ASSERT( ret == SQLITE_OK );
            Q_UNUSED( ret );
            QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
            return false;
          }
        }
        // End TRANSACTION
        ret = sqlite3_exec( db_handle, "COMMIT", NULL, NULL, 0 );
        Q_ASSERT( ret == SQLITE_OK );
        Q_UNUSED( ret );
        b_database_exists = true;
        b_gcp_coverage = b_database_exists;
        // If a gcp_coverage exists, the Database is considered Valid, but may remain empty
        parms_GcpDbData->mDatabaseValid = b_database_exists;
        sa_sql_commands.clear();
      }
      sa_tables.clear();
    }
  }
  else
  { // gcp_coverages exists, check for for raster entry [gcp_compute should exist]
    if ( parms_GcpDbData->mSqlDump )
    { // Should only run when creating a new Database
      parms_GcpDbData->mSqlDump = false;
    }
    if (( b_gcp_enabled ) && ( !b_gcp_compute ) )
    { // Database was created without gcp_compute, turn off support
      b_gcp_enabled = false;
    }
    // If a gcp_coverage exists, the Database is considered Valid
    parms_GcpDbData->mDatabaseValid = b_gcp_coverage;
    if ( !s_coverage_name.isNull() )
    {
      s_sql = QString( "SELECT id_gcp_coverage,srid,transformtype,resampling, compression,nodata,id_cutline FROM gcp_coverages WHERE coverage_name='%1'" ).arg( s_coverage_name );
      ret = sqlite3_exec( db_handle, "BEGIN", NULL, NULL, 0 );
      Q_ASSERT( ret == SQLITE_OK );
      Q_UNUSED( ret );
      ret = sqlite3_prepare_v2( db_handle, s_sql.toUtf8().constData(), -1, &stmt, NULL );
      if ( ret != SQLITE_OK )
      { //  rc=1 [no such table: vector_layers_statistics or gcp_points, 'no such column' for gcp_pixel and gcp_ppoint] is an error
        parms_GcpDbData->mError = QString( "Unable to retrieve id_gcp_covarage: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql );
        qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -5- [%1] " ).arg( parms_GcpDbData->mError );
        // End TRANSACTION
        ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
        Q_ASSERT( ret == SQLITE_OK );
        Q_UNUSED( ret );
        QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
        return false;
      }
      while ( sqlite3_step( stmt ) == SQLITE_ROW )
      {
        if ( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL )
          id_gcp_coverage = sqlite3_column_int( stmt, 0 );
        if ( sqlite3_column_type( stmt, 1 ) != SQLITE_NULL )
          i_srid = sqlite3_column_int( stmt, 1 );
        if ( sqlite3_column_type( stmt, 2 ) != SQLITE_NULL )
          i_TransformParam = sqlite3_column_int( stmt, 2 );
        if ( sqlite3_column_type( stmt, 3 ) != SQLITE_NULL )
          s_ResamplingMethod = QString::fromUtf8(( const char * ) sqlite3_column_text( stmt, 3 ) );
        if ( sqlite3_column_type( stmt, 4 ) != SQLITE_NULL )
          s_CompressionMethod = QString::fromUtf8(( const char * ) sqlite3_column_text( stmt, 4 ) );
        if ( sqlite3_column_type( stmt, 5 ) != SQLITE_NULL )
          i_RasterNoData = sqlite3_column_int( stmt, 5 );
        if ( sqlite3_column_type( stmt, 6 ) != SQLITE_NULL )
          id_cutline = sqlite3_column_int( stmt, 6 );
      }
      sqlite3_finalize( stmt );
      // End TRANSACTION
      ret = sqlite3_exec( db_handle, "COMMIT", NULL, NULL, 0 );
      Q_ASSERT( ret == SQLITE_OK );
      Q_UNUSED( ret );
    }
  }
  if ( !s_coverage_name.isNull() )
  { // reading a specific gcp_points TABLE
    // qDebug() << QString( "-I-> QgsSpatiaLiteProviderGcpUtils::createGcpDb -01- id_gcp_coverage[%2] b_gcp_coverage_create[%3] gcp_points_table_name[%1] " ).arg( s_points_table_name ).arg( id_gcp_coverage ).arg( b_gcp_coverage_create );
    ret = sqlite3_exec( db_handle, "BEGIN", NULL, NULL, 0 );
    Q_ASSERT( ret == SQLITE_OK );
    Q_UNUSED( ret );
    if ( id_gcp_coverage < 0 )
    { // INSERT raster entry [title, abstract and copyright are taken from the TIFFTAGS (if they exist)]
      b_import_points = true; // import of .points file only when first created
      s_map_date = QString( "%1-01-01" ).arg( parms_GcpDbData->mRasterYear, 4, 10, QChar( '0' ) );
      s_sql_select_coverages  = QString( "INSERT_COVERAGE%1%2" ).arg( parms_GcpDbData->mParseString ).arg( "VALUES(" );
      s_sql_select_coverages += QString( "'%1'," ).arg( s_coverage_name );
      s_sql_select_coverages += QString( "'%1'," ).arg( s_map_date );
      s_sql_select_coverages += QString( "'%1'," ).arg( s_RasterFileName );
      s_sql_select_coverages += QString( "'%1'," ).arg( s_ModifiedRasterFileName );
      s_sql_select_coverages += QString( "'%1'," ).arg( s_coverage_title );
      s_sql_select_coverages += QString( "'%1'," ).arg( s_coverage_abstract );
      s_sql_select_coverages += QString( "'%1'," ).arg( s_coverage_copyright );
      s_sql_select_coverages += QString( "%1," ).arg( i_RasterScale ); // scale
      s_sql_select_coverages += QString( "%1," ).arg( i_RasterNoData ); // gdal-nodata
      s_sql_select_coverages += QString( "%1," ).arg( i_srid );
      s_sql_select_coverages += QString( "%1," ).arg( id_cutline );
      s_sql_select_coverages += QString( "%1," ).arg( 0 ); // gcp_count
      s_sql_select_coverages += QString( "%1," ).arg( i_TransformParam );
      s_sql_select_coverages += QString( "'%1'," ).arg( s_ResamplingMethod );
      s_sql_select_coverages += QString( "'%1'," ).arg( s_CompressionMethod );
      s_sql_select_coverages += QString( "%1," ).arg( i_image_max_x );
      s_sql_select_coverages += QString( "%1," ).arg( i_image_max_y );
      s_sql_select_coverages += QString( "%1," ).arg( d_extent_minx );
      s_sql_select_coverages += QString( "%1," ).arg( d_extent_miny );
      s_sql_select_coverages += QString( "%1," ).arg( d_extent_maxx );
      s_sql_select_coverages += QString( "%1," ).arg( d_extent_maxy );
      s_sql_select_coverages += QString( "'%1'" ).arg( s_RasterFilePath );
      s_sql_select_coverages += QString( "%1" ).arg( ");" );
      QStringList sa_tables; // Format: INSERT_COVERAGE;#;#VALUES(...)
      sa_tables.append( s_sql_select_coverages );
      if ( sa_tables.size() > 0 )
      {
        QStringList sa_sql_commands = QgsSpatiaLiteProviderGcpUtils::createGcpSqlCoveragesCommands( parms_GcpDbData, sa_tables );
        if ( sa_sql_commands.size() > 0 )
        {
          for ( int i_list = 0;i_list < sa_sql_commands.size();i_list++ )
          { // raster entry is being added to gcp_covarage, create corresponding tables
            s_sql = sa_sql_commands.at( i_list );
            if ( parms_GcpDbData->mSqlDump )
            {
              s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
            }
            ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
            if ( ret != SQLITE_OK )
            {
              parms_GcpDbData->mError = QString( "Unable to INSERT INTO gcp_covarage: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
              qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -6- [%1] " ).arg( parms_GcpDbData->mError );
              sqlite3_free( errMsg );
            }
          }
          sa_sql_commands.clear();
        }
        sa_tables.clear();
      }
      s_sql = QString( "SELECT id_gcp_coverage, srid FROM gcp_coverages WHERE (coverage_name='%1')" ).arg( s_coverage_name );
      ret = sqlite3_prepare_v2( db_handle, s_sql.toUtf8().constData(), -1, &stmt, NULL );
      if ( ret != SQLITE_OK )
      { //  rc=1 [no such table: vector_layers_statistics or gcp_points, 'no such column' for gcp_pixel and gcp_ppoint] is an error
        parms_GcpDbData->mError = QString( "Unable to retrieve id_gcp_covarage: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql );
        qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -7a- [%1] " ).arg( parms_GcpDbData->mError );
        // End TRANSACTION
        ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
        Q_ASSERT( ret == SQLITE_OK );
        Q_UNUSED( ret );
        QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
        return false;
      }
      while ( sqlite3_step( stmt ) == SQLITE_ROW )
      {
        if ( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL )
        {
          id_gcp_coverage = sqlite3_column_int( stmt, 0 );
          b_gcp_coverage_create = true; // create needed tables, views
        }
        if ( sqlite3_column_type( stmt, 1 ) != SQLITE_NULL )
        {
          i_srid = sqlite3_column_int( stmt, 1 );
        }
      }
      sqlite3_finalize( stmt );
    }
    else
    { // the gcp_coverages exists, retrieve metadata and update the layer.
      s_sql = QString( "SELECT id_gcp_coverage,srid, title, abstract, copyright, map_date FROM gcp_coverages WHERE (coverage_name='%1')" ).arg( s_coverage_name );
      ret = sqlite3_prepare_v2( db_handle, s_sql.toUtf8().constData(), -1, &stmt, NULL );
      if ( ret != SQLITE_OK )
      { //  rc=1 [no such table: vector_layers_statistics or gcp_points, 'no such column' for gcp_pixel and gcp_ppoint] is an error
        parms_GcpDbData->mError = QString( "Unable to retrieve id_gcp_covarage: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql );
        qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -7b- [%1] " ).arg( parms_GcpDbData->mError );
        // End TRANSACTION
        ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
        Q_ASSERT( ret == SQLITE_OK );
        Q_UNUSED( ret );
        QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
        return false;
      }
      while ( sqlite3_step( stmt ) == SQLITE_ROW )
      {
        if ( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL )
          id_gcp_coverage = sqlite3_column_int( stmt, 0 );
        if ( sqlite3_column_type( stmt, 1 ) != SQLITE_NULL )
          i_srid = sqlite3_column_int( stmt, 1 );
        if ( sqlite3_column_type( stmt, 2 ) != SQLITE_NULL )
          s_coverage_title = QString::fromUtf8(( const char * ) sqlite3_column_text( stmt, 2 ) );
        if ( sqlite3_column_type( stmt, 3 ) != SQLITE_NULL )
          s_coverage_abstract = QString::fromUtf8(( const char * ) sqlite3_column_text( stmt, 3 ) );
        if ( sqlite3_column_type( stmt, 4 ) != SQLITE_NULL )
          s_coverage_copyright = QString::fromUtf8(( const char * ) sqlite3_column_text( stmt, 4 ) );
        if ( sqlite3_column_type( stmt, 5 ) != SQLITE_NULL )
          s_map_date = QString::fromUtf8(( const char * ) sqlite3_column_text( stmt, 5 ) );
      }
      sqlite3_finalize( stmt );
      if ( raster_layer )
      {
        raster_layer->setTitle( s_coverage_title );
        raster_layer->setAbstract( s_coverage_abstract );
        raster_layer->setKeywordList( s_coverage_copyright );
      }
    }
    // End TRANSACTION
    ret = sqlite3_exec( db_handle, "COMMIT", NULL, NULL, 0 );
    Q_ASSERT( ret == SQLITE_OK );
    Q_UNUSED( ret );
    if (( b_gcp_coverage_create ) && ( id_gcp_coverage > 0 ) )
    { // raster entry has been added to gcp_covarage, create corresponding tables
      qDebug() << QString( "-I-> QgsSpatiaLiteProviderGcpUtils::createGcpDb creating gcp tables and views for[%1] seid[%2]" ).arg( s_points_table_name ).arg( i_srid );
      // Start TRANSACTION
      QStringList sa_tables; // Format: id_gcp_coverage;#;#i_srid;#;#s_coverage_name
      sa_tables.append( QString( "%2%1%3%1%4" ).arg( parms_GcpDbData->mParseString ).arg( id_gcp_coverage ).arg( i_srid ).arg( s_coverage_name ) );
      QStringList sa_sql_commands = QgsSpatiaLiteProviderGcpUtils::createGcpSqlPointsCommands( parms_GcpDbData, sa_tables );
      if ( sa_sql_commands.size() > 0 )
      {
        ret = sqlite3_exec( db_handle, "BEGIN", NULL, NULL, 0 );
        Q_ASSERT( ret == SQLITE_OK );
        Q_UNUSED( ret );
        for ( int i_list = 0;i_list < sa_sql_commands.size();i_list++ )
        { // raster entry has been added to gcp_covarage, create corresponding tables
          s_sql = sa_sql_commands.at( i_list );
          if ( parms_GcpDbData->mSqlDump )
          {
            if ( s_sql.contains( QString( "SELECT AddGeometryColumn(" ) ) )
            {
              s_sql_output += QString( "%1\n" ).arg( s_sql );
            }
            else
            {
              s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
            }
          }
          ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
          if ( ret != SQLITE_OK )
          {
            parms_GcpDbData->mError = QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb: Failed while creating gcp_points table for [%1]: rc=%2 [%3] sql[%4]\n" ).arg( s_coverage_name ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
            qDebug() << parms_GcpDbData->mError;
            sqlite3_free( errMsg );
            // End TRANSACTION
            ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
            Q_ASSERT( ret == SQLITE_OK );
            Q_UNUSED( ret );
            QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
            return false;
          }
        }
        // End TRANSACTION
        ret = sqlite3_exec( db_handle, "COMMIT", NULL, NULL, 0 );
        Q_ASSERT( ret == SQLITE_OK );
        Q_UNUSED( ret );
      }
    }
    if ( id_gcp_coverage <= 0 )
    { // We have not found the requested Coverage, assume the Database is invalid
      parms_GcpDbData->mDatabaseValid = false;
    }
  }
  if (( b_database_exists ) && ( b_gcp_coverage ) )
  { // if the database already exists, check that '"gcp_pixel' and 'gcp_point' geometries in 'gcp_points' exist and correct the statistics if needed
    // Sanity checks to insure that this is a table we can work with, otherwise return 'db' not found
    // Start TRANSACTION
    ret = sqlite3_exec( db_handle, "BEGIN", NULL, NULL, 0 );
    Q_ASSERT( ret == SQLITE_OK );
    Q_UNUSED( ret );
    int i_point_statistics = 0;
    int i_pixel_statistics = 0;
    int i_gcp_coverages = 0;
    s_sql = QString( "SELECT " );
    s_sql += QString( "count((SELECT extent_min_x FROM vector_layers_statistics WHERE ((extent_min_x IS NOT NULL) AND " );
    s_sql += QString( "(table_name='%1') AND (geometry_column='%2')))) AS pixel_statistics," ).arg( s_points_table_name ).arg( "gcp_pixel" );
    s_sql += QString( "count((SELECT extent_min_x FROM vector_layers_statistics WHERE ((extent_min_x IS NOT NULL) AND " );
    s_sql += QString( "(table_name='%1') AND (geometry_column='%2')))) AS points_statistics," ).arg( s_points_table_name ).arg( "gcp_point" );
    s_sql += QString( "(SELECT count(id_gcp_coverage) FROM '%1' WHERE gcp_count > 0) AS count_gcp" ).arg( "gcp_coverages" );
    ret = sqlite3_prepare_v2( db_handle, s_sql.toUtf8().constData(), -1, &stmt, NULL );
    if ( ret != SQLITE_OK )
    { //  rc=1 [no such table: vector_layers_statistics or gcp_points, 'no such column' for gcp_pixel and gcp_ppoint] is an error
      parms_GcpDbData->mError = QString( "Unable to check vector_layers_statistics, table gcp_points or geometries: gcp_pixel and gcp_point: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql );
      qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -22- [%1] " ).arg( parms_GcpDbData->mError );
      // End TRANSACTION
      ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
      Q_ASSERT( ret == SQLITE_OK );
      Q_UNUSED( ret );
      QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
      return false;
    }
    while ( sqlite3_step( stmt ) == SQLITE_ROW )
    {
      if ( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL )
        i_pixel_statistics = sqlite3_column_int( stmt, 0 );
      if ( sqlite3_column_type( stmt, 1 ) != SQLITE_NULL )
        i_point_statistics = sqlite3_column_int( stmt, 1 );
      if ( sqlite3_column_type( stmt, 2 ) != SQLITE_NULL )
        i_gcp_coverages = sqlite3_column_int( stmt, 2 );
    }
    sqlite3_finalize( stmt );
    if ( i_gcp_coverages < 1 )
    { // gcp_points is empty, don't bother about the statistics
      i_pixel_statistics = 1;
      i_point_statistics = 1;
    }
    if ( i_pixel_statistics == 0 )
    { // should be 1, if not empty
      s_sql = QString( "SELECT UpdateLayerStatistics('%1','gcp_pixel')" ).arg( s_points_table_name );
      ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
      if ( ret != SQLITE_OK )
      {
        parms_GcpDbData->mError = QString( "Unable to execute UpdateLayerStatistics('%1','gcp_pixel'): rc=%2 [%3] sql[%4]\n" ).arg( s_points_table_name ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
        qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -23- [%1] " ).arg( parms_GcpDbData->mError );
        sqlite3_free( errMsg );
        // End TRANSACTION
        ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
        Q_ASSERT( ret == SQLITE_OK );
        Q_UNUSED( ret );
        QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
        return false;
      }
    }
    if ( i_point_statistics == 0 )
    {  // should be 1, if not empty
      s_sql = QString( "SELECT UpdateLayerStatistics('%1','gcp_point')" ).arg( s_points_table_name );
      ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
      if ( ret != SQLITE_OK )
      {
        parms_GcpDbData->mError = QString( "Unable to execute UpdateLayerStatistics('%1','gcp_point'): rc=%2 [%3] sql[%4]\n" ).arg( s_points_table_name ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
        qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -24- [%1] " ).arg( parms_GcpDbData->mError );
        sqlite3_free( errMsg );
        // End TRANSACTION
        ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
        Q_ASSERT( ret == SQLITE_OK );
        Q_UNUSED( ret );
        QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
        return false;
      }
    }
    QStringList sa_tables; // Format: id_gcp_coverage;#;#List of all fields [if 2nd argument is empty: all records, otherwise specific coverage_name]
    sa_tables.append( QString( "SELECT_COVERAGES%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "" ) );
    QStringList sa_sql_commands = QgsSpatiaLiteProviderGcpUtils::createGcpSqlCoveragesCommands( parms_GcpDbData, sa_tables );
    if ( sa_sql_commands.size() == 1 )
    {
      s_sql = sa_sql_commands.at( 0 );
      ret = sqlite3_prepare_v2( db_handle, s_sql.toUtf8().constData(), -1, &stmt, NULL );
      if ( ret != SQLITE_OK )
      { //  rc=1 [no such table: vector_layers_statistics or gcp_points, 'no such column' for gcp_pixel and gcp_point] is an error
        parms_GcpDbData->mError = QString( "Unable to query gcp_coverages: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql );
        qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -22- [%1] " ).arg( parms_GcpDbData->mError );
        // End TRANSACTION
        ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
        Q_ASSERT( ret == SQLITE_OK );
        Q_UNUSED( ret );
        QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
        return false;
      }
      parms_GcpDbData->gcp_coverages.clear();
      while ( sqlite3_step( stmt ) == SQLITE_ROW )
      {
        if (( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL ) && ( sqlite3_column_type( stmt, 1 ) != SQLITE_NULL ) )
        {
          int i_id = sqlite3_column_int( stmt, 0 );
          QString s_value = QString::fromUtf8(( const char * ) sqlite3_column_text( stmt, 1 ) );
          parms_GcpDbData->gcp_coverages.insert( i_id, s_value );
        }
      }
      if ( parms_GcpDbData->gcp_coverages.size() == 0 )
      {
        parms_GcpDbData->mError = QString( "Query of gcp_coverages returned no results:  gcp_coverages.size=%1 cause[%2] sql[%3]\n" ).arg( parms_GcpDbData->gcp_coverages.size() ).arg( "possible NULL values in ROW [correction: set to 0]" ).arg( s_sql );
      }
      else
      {
        parms_GcpDbData->mDatabaseValid = true;
      }
      sa_sql_commands.clear();
    }
    sa_tables.clear();
    // End TRANSACTION
    ret = sqlite3_exec( db_handle, "COMMIT", NULL, NULL, 0 );
    Q_ASSERT( ret == SQLITE_OK );
    Q_UNUSED( ret );
  }
  // --- Cutline common sql for TABLE creation
  if (( !b_cutline_geometries_create ) )
  { // no cutline geometries, create them
    // qDebug() << QString( "-I-> QgsSpatiaLiteProviderGcpUtils::createGcpDb creating cutline geomtries for[%1] " ).arg( "points, linestrings, polygons" );
    // Start TRANSACTION
    QStringList sa_tables; // Format: CREATE_CUTLINES;#;#i_srid
    sa_tables.append( QString( "%2%1%3" ).arg( parms_GcpDbData->mParseString ).arg( "CREATE_CUTLINES" ).arg( i_srid ) );
    if ( sa_tables.size() > 0 )
    {
      QStringList sa_sql_commands = QgsSpatiaLiteProviderGcpUtils::createGcpSqlCoveragesCommands( parms_GcpDbData, sa_tables );
      if ( sa_sql_commands.size() > 0 )
      {
        ret = sqlite3_exec( db_handle, "BEGIN", NULL, NULL, 0 );
        Q_ASSERT( ret == SQLITE_OK );
        Q_UNUSED( ret );
        for ( int i_list = 0;i_list < sa_sql_commands.size();i_list++ )
        { // raster entry has been added to gcp_covarage, create corresponding tables
          s_sql = sa_sql_commands.at( i_list );
          if ( s_sql.contains( QString( "SELECT AddGeometryColumn(" ) ) )
          {
            s_sql_output += QString( "%1\n" ).arg( s_sql );
          }
          else
          {
            s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
          }
          ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
          if ( ret != SQLITE_OK )
          {
            parms_GcpDbData->mError = QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb: Failed while creating [%1] tables for project : rc=%2 [%3] sql[%4]\n" ).arg( "cutline_*" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
            qDebug() << parms_GcpDbData->mError;
            sqlite3_free( errMsg );
            // End TRANSACTION
            ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
            Q_ASSERT( ret == SQLITE_OK );
            Q_UNUSED( ret );
            QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
            return false;
          }
        }
        // End TRANSACTION
        ret = sqlite3_exec( db_handle, "COMMIT", NULL, NULL, 0 );
        Q_ASSERT( ret == SQLITE_OK );
        Q_UNUSED( ret );
        b_gcp_coverage = true;
        sa_sql_commands.clear();
      }
      sa_tables.clear();
    }
  }
  if ( !s_coverage_name.isNull() )
  { // reading a specific gcp_points TABLE
    // qDebug() << QString( "-I-> QgsSpatiaLiteProviderGcpUtils::createGcpDb[%1,%2,%3] s_points_file_name[%4] " ).arg( id_gcp_coverage ).arg( b_import_points ).arg( points_file.exists() ).arg( s_points_file_name );
    if ((( b_import_points ) && ( points_file.exists() ) ) && ( id_gcp_coverage >= 0 ) )
    { // Reading of '.points' file [for QGIS 3.0: this should NOT not removed]
      // qDebug() << QString( "-I-> QgsSpatiaLiteProviderGcpUtils::createGcpDb s_points_file_name[%1] " ).arg( s_points_file_name );
      if ( points_file.open( QIODevice::ReadOnly ) )
      {
        // Start TRANSACTION
        ret = sqlite3_exec( db_handle, "BEGIN", NULL, NULL, 0 );
        Q_ASSERT( ret == SQLITE_OK );
        Q_UNUSED( ret );
        QTextStream stream_points( &points_file );
        QString line = stream_points.readLine();
        while ( !stream_points.atEnd() )
        {
          line = stream_points.readLine();
          QStringList ls;
          if ( line.contains( QRegExp( "," ) ) ) // in previous format "\t" is delimiter of points in new - ","
          {  // points from new georeferencer
            ls = line.split( "," );
          }
          else
          {  // points from prev georeferencer
            ls = line.split( "\t" );
          }
          // QgsPoint mapCoords( ls.at( 0 ).toDouble(), ls.at( 1 ).toDouble() ); // map x,y
          // QgsPoint pixelCoords( ls.at( 2 ).toDouble(), ls.at( 3 ).toDouble() ); // pixel x,y
          s_sql = QString( "INSERT INTO \"%1\"\n(name,id_gcp_coverage,srid,pixel_x,pixel_y,point_x,point_y,gcp_enable)\n " ).arg( s_points_table_name );
          s_sql += QString( "SELECT '%1' AS name," ).arg( s_GCPbaseFileName );
          s_sql += QString( "%1 AS id_gcp_coverage," ).arg( id_gcp_coverage );
          s_sql += QString( "%1 AS srid,\n " ).arg( i_srid );
          s_sql += QString( "%1 AS pixel_x," ).arg( ls.at( 2 ).simplified() );
          s_sql += QString( "%1 AS pixel_y,\n " ).arg( ls.at( 3 ).simplified() );
          s_sql += QString( "%1 AS point_x," ).arg( ls.at( 0 ).simplified() );
          s_sql += QString( "%1 AS point_y," ).arg( ls.at( 1 ).simplified() );
          s_sql += QString( "%1 AS gcp_enable;" ).arg( ls.at( 4 ).simplified() );
          if ( parms_GcpDbData->mSqlDump )
          {
            s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
          }
          ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
          if ( ret != SQLITE_OK )
          {
            parms_GcpDbData->mError = QString( "Unable to INSERT INTO '%1': rc=%2 [%3] sql[%4]\n" ).arg( s_points_table_name ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
            qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -34- [%1] " ).arg( parms_GcpDbData->mError );
            sqlite3_free( errMsg );
          }
        }
        // Note: since we are not inserting the geometry directly [is created with the TRIGGER], CheckSpatialIndex will return 0
        // - RecoverSpatialIndex is needed to build the index properly, otherwise OGR will not read the records properly
        s_sql = QString( "SELECT RecoverSpatialIndex('%1','gcp_pixel');" ).arg( s_points_table_name );
        if ( parms_GcpDbData->mSqlDump )
        {
          s_sql_output += QString( "%1\n" ).arg( s_sql );
        }
        ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
        if ( ret != SQLITE_OK )
        {
          parms_GcpDbData->mError = QString( "Unable to execute RecoverSpatialIndex('%1','gcp_pixel'): rc=%2 [%3] sql[%4]\n" ).arg( s_points_table_name ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
          qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -35- [%1] " ).arg( parms_GcpDbData->mError );
          sqlite3_free( errMsg );
          // End TRANSACTION
          ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
          Q_ASSERT( ret == SQLITE_OK );
          Q_UNUSED( ret );
          QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
          return false;
        }
        s_sql = QString( "SELECT UpdateLayerStatistics('%1','gcp_pixel');" ).arg( s_points_table_name );
        if ( parms_GcpDbData->mSqlDump )
        {
          s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
        }
        ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
        if ( ret != SQLITE_OK )
        {
          parms_GcpDbData->mError = QString( "Unable to execute UpdateLayerStatistics('%1','gcp_pixel'): rc=%2 [%3] sql[%4]\n" ).arg( s_points_table_name ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
          qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -36- [%1] " ).arg( parms_GcpDbData->mError );
          sqlite3_free( errMsg );
          // End TRANSACTION
          ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
          Q_ASSERT( ret == SQLITE_OK );
          Q_UNUSED( ret );
          QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
          return false;
        }
        // Note: since we are not inserting the geometry directly [is created with the TRIGGER], CheckSpatialIndex will return 0
        // - RecoverSpatialIndex is needed to build the index properly, otherwise OGR will not read the records properly
        s_sql = QString( "SELECT RecoverSpatialIndex('%1','gcp_point');" ).arg( s_points_table_name );
        if ( parms_GcpDbData->mSqlDump )
        {
          s_sql_output += QString( "%1\n" ).arg( s_sql );
        }
        ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
        if ( ret != SQLITE_OK )
        {
          parms_GcpDbData->mError = QString( "Unable to execute RecoverSpatialIndex('%1','gcp_point'): rc=%2 [%3] sql[%4]\n" ).arg( s_points_table_name ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
          qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -37- [%1] " ).arg( parms_GcpDbData->mError );
          sqlite3_free( errMsg );
          // End TRANSACTION
          ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
          Q_ASSERT( ret == SQLITE_OK );
          Q_UNUSED( ret );
          QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
          return false;
        }
        s_sql = QString( "SELECT UpdateLayerStatistics('%1','gcp_point');" ).arg( s_points_table_name );
        if ( parms_GcpDbData->mSqlDump )
        {
          s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
        }
        ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
        if ( ret != SQLITE_OK )
        {
          parms_GcpDbData->mError = QString( "Unable to execute UpdateLayerStatistics('%1','gcp_point'): rc=%2 [%3] sql[%4]\n" ).arg( s_points_table_name ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
          qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -38- [%1] " ).arg( parms_GcpDbData->mError );
          sqlite3_free( errMsg );
          // End TRANSACTION
          ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
          Q_ASSERT( ret == SQLITE_OK );
          Q_UNUSED( ret );
          QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
          return false;
        }
        // End TRANSACTION
        ret = sqlite3_exec( db_handle, "COMMIT", NULL, NULL, 0 );
        Q_ASSERT( ret == SQLITE_OK );
        Q_UNUSED( ret );
        points_file.close();
      }
    }
  }
  if (( b_gcp_coverage ) && ( parms_GcpDbData->mDatabaseDump ) && ( parms_GcpDbData->mDatabaseValid ) )
  { // Dump Database after creation, when requested
    // Prevent both DatabaseDump and SqlDump running
    s_sql_output = QString::null;
    parms_GcpDbData->mSqlDump = false;
    parms_GcpDbData->mDatabaseValid = false;
    QStringList sa_tables; // Format: CREATE_COVERAGE;#;#i_srid
    QStringList sa_points_tables;
    QStringList sa_create_points_tables;
    QStringList sa_gcp_compute;
    QStringList sa_sql_commands;
    QStringList sa_cutline_tables;
    // Start TRANSACTION
    ret = sqlite3_exec( db_handle, "BEGIN", NULL, NULL, 0 );
    Q_ASSERT( ret == SQLITE_OK );
    Q_UNUSED( ret );
    s_sql_select_coverages = QString( "SELECT\n " );
    s_sql_select_coverages += QString( "'UPDATE_COVERAGE_EXTENT" );
    s_sql_select_coverages += QString( "%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "id_gcp_coverage" );
    s_sql_select_coverages += QString( "||'%1'||%2\n" ).arg( parms_GcpDbData->mParseString ).arg( "coverage_name" );
    s_sql_select_coverages += QString( "FROM '%1' ORDER BY id_gcp_coverage;" ).arg( "gcp_coverages" );
    ret = sqlite3_prepare_v2( db_handle, s_sql_select_coverages.toUtf8().constData(), -1, &stmt, NULL );
    // qDebug() << QString( "sql_select_coverages[%1] " ).arg( s_sql_select_coverages );
    if ( ret != SQLITE_OK )
    {
      parms_GcpDbData->mError = QString( "Unable to retrieve gcp_coverages for UPDATE: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql_select_coverages );
      qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -gcp_coverages- [%1] " ).arg( parms_GcpDbData->mError );
      // End TRANSACTION
      ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
      Q_ASSERT( ret == SQLITE_OK );
      Q_UNUSED( ret );
      QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
      return false;
    }
    while ( sqlite3_step( stmt ) == SQLITE_ROW )
    {
      if ( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL )
      {
        s_sql = QString::fromUtf8(( const char * ) sqlite3_column_text( stmt, 0 ) );
        sa_tables.append( s_sql );
      }
    }
    sqlite3_finalize( stmt );
    if ( sa_tables.size() > 0 )
    {
      sa_sql_commands = QgsSpatiaLiteProviderGcpUtils::createGcpSqlCoveragesCommands( parms_GcpDbData, sa_tables );
      if ( sa_sql_commands.size() > 0 )
      {
        for ( int i_list = 0;i_list < sa_sql_commands.size();i_list++ )
        { // We have no other fields to UPDATE, so fill with an empty string [every 2nd command has no formatting needs]
          if ( sa_sql_commands.at( i_list ).contains( "%1" ) )
            s_sql = QString( "%1" ).arg( QString( sa_sql_commands.at( i_list ) ).arg( "" ) );
          else
            s_sql = sa_sql_commands.at( i_list );
          ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
          if ( ret != SQLITE_OK )
          {
            parms_GcpDbData->mError = QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb: Failed while UPDATEing [%1] table entries for project : rc=%2 [%3] sql[%4]\n" ).arg( "gcp_coverages" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
            qDebug() << parms_GcpDbData->mError;
            sqlite3_free( errMsg );
            // End TRANSACTION
            ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
            Q_ASSERT( ret == SQLITE_OK );
            Q_UNUSED( ret );
            QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
            return false;
          }
        }
        sa_sql_commands.clear();
      }
      sa_tables.clear();
    }
    //-------
    // sa_tables.append( s_update_extent );
    //-- create gcp_coverages
    sa_tables.append( QString( "%2%1%3" ).arg( parms_GcpDbData->mParseString ).arg( "CREATE_COVERAGE" ).arg( i_srid ) );
    if ( sa_tables.size() > 0 )
    {
      sa_sql_commands = QgsSpatiaLiteProviderGcpUtils::createGcpSqlCoveragesCommands( parms_GcpDbData, sa_tables );
      if ( sa_sql_commands.size() > 0 )
      {
        for ( int i_list = 0;i_list < sa_sql_commands.size();i_list++ )
        { // raster entry has been added to gcp_covarage, create corresponding tables
          s_sql = sa_sql_commands.at( i_list );
          s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
        }
        sa_sql_commands.clear();
      }
      sa_tables.clear();
    }
    //-- create cutlines_ [points, linestrings,polygons]
    sa_tables.append( QString( "%2%1%3" ).arg( parms_GcpDbData->mParseString ).arg( "CREATE_CUTLINES" ).arg( i_srid ) );
    if ( sa_tables.size() > 0 )
    {
      sa_sql_commands = QgsSpatiaLiteProviderGcpUtils::createGcpSqlCoveragesCommands( parms_GcpDbData, sa_tables );
      if ( sa_sql_commands.size() > 0 )
      {
        for ( int i_list = 0;i_list < sa_sql_commands.size();i_list++ )
        { // raster entry has been added to gcp_covarage, create corresponding tables
          s_sql = sa_sql_commands.at( i_list );
          if ( s_sql.contains( QString( "SELECT AddGeometryColumn(" ) ) )
          {
            s_sql_output += QString( "%1\n" ).arg( s_sql );
          }
          else
          {
            s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
          }
        }
        sa_sql_commands.clear();
      }
      sa_tables.clear();
    }
    //-------
    //-- read gcp_coverages
    s_sql_select_coverages = QString( "SELECT\n " );
    s_sql_select_coverages += QString( "'gcp_points_'||%1 AS points_name,\n " ).arg( "coverage_name" );
    // Format: id_gcp_coverage;#;#i_srid;#;#s_coverage_name
    s_sql_select_coverages += QString( "%2||'%1'||%3||'%1'||%4 AS create_points,\n " ).arg( parms_GcpDbData->mParseString ).arg( "id_gcp_coverage" ).arg( "srid" ).arg( "coverage_name" );
    // Format: INSERT_COVERAGES;#;#VALUES(...)
    // returns the INSERT VALUE portion for each entry in gcp_coverages
    s_sql_select_coverages += QString( "'INSERT_COVERAGES'||'%1'||'%2'" ).arg( parms_GcpDbData->mParseString ).arg( "VALUES(" );
    s_sql_select_coverages += QString( "||%1" ).arg( "id_gcp_coverage" );
    s_sql_select_coverages += QString( "||','''||%1" ).arg( "coverage_name" );
    s_sql_select_coverages += QString( "||''','''||%1" ).arg( "map_date" );
    s_sql_select_coverages += QString( "||''','''||%1" ).arg( "raster_file" );
    s_sql_select_coverages += QString( "||''','''||%1" ).arg( "raster_gtif" );
    s_sql_select_coverages += QString( "||''','''||%1" ).arg( "title" );
    s_sql_select_coverages += QString( "||''','''||%1" ).arg( "abstract" );
    s_sql_select_coverages += QString( "||''','''||%1" ).arg( "copyright" );
    s_sql_select_coverages += QString( "||''','||%1" ).arg( "scale" );
    s_sql_select_coverages += QString( "||','||%1" ).arg( "nodata" );
    s_sql_select_coverages += QString( "||','||%1" ).arg( "srid" );
    s_sql_select_coverages += QString( "||','||%1" ).arg( "id_cutline" );
    s_sql_select_coverages += QString( "||','||%1" ).arg( "gcp_count" );
    s_sql_select_coverages += QString( "||','||%1" ).arg( "transformtype" );
    s_sql_select_coverages += QString( "||','''||%1" ).arg( "resampling" );
    s_sql_select_coverages += QString( "||''','''||%1" ).arg( "compression" );
    s_sql_select_coverages += QString( "||''','||%1" ).arg( "image_max_x" );
    s_sql_select_coverages += QString( "||','||%1" ).arg( "image_max_y" );
    s_sql_select_coverages += QString( "||','||%1" ).arg( "extent_minx" );
    s_sql_select_coverages += QString( "||','||%1" ).arg( "extent_miny" );
    s_sql_select_coverages += QString( "||','||%1" ).arg( "extent_maxx" );
    s_sql_select_coverages += QString( "||','||%1" ).arg( "extent_maxy" );
    s_sql_select_coverages += QString( "||','''||%1" ).arg( "raster_path" );
    s_sql_select_coverages += QString( "||'''%1" ).arg( ");' AS coverage_value,\n " );
    s_sql_select_coverages += QString( "'INSERT_COMPUTE'||'%1'||%2||'%1'||%3||'%1'||%4||'%1'||%5||'%1'||%6 AS insert_compute\n" ).arg( parms_GcpDbData->mParseString ).arg( "id_gcp_coverage" ).arg( "srid" ).arg( "coverage_name" ).arg( "image_max_x" ).arg( "image_max_y" );
    s_sql_select_coverages += QString( "FROM '%1'\nORDER BY id_gcp_coverage;" ).arg( "gcp_coverages" );
    ret = sqlite3_prepare_v2( db_handle, s_sql_select_coverages.toUtf8().constData(), -1, &stmt, NULL );
    // qDebug() << QString( "sql_select_coverages[%1] " ).arg( s_sql_select_coverages );
    if ( ret != SQLITE_OK )
    { //  rc=1 [no such table: vector_layers_statistics or gcp_points, 'no such column' for gcp_pixel and gcp_ppoint] is an error
      parms_GcpDbData->mError = QString( "Unable to retrieve gcp_coverages: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql_select_coverages );
      qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -22- [%1] " ).arg( parms_GcpDbData->mError );
      // End TRANSACTION
      ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
      Q_ASSERT( ret == SQLITE_OK );
      Q_UNUSED( ret );
      QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
      return false;
    }
    while ( sqlite3_step( stmt ) == SQLITE_ROW )
    {
      if (( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL ) && ( sqlite3_column_type( stmt, 1 ) != SQLITE_NULL ) && ( sqlite3_column_type( stmt, 2 ) != SQLITE_NULL ) && ( sqlite3_column_type( stmt, 3 ) != SQLITE_NULL ) )
      {
        s_sql = QString::fromUtf8(( const char * ) sqlite3_column_text( stmt, 0 ) );
        sa_points_tables.append( s_sql );
        s_sql = QString::fromUtf8(( const char * ) sqlite3_column_text( stmt, 1 ) );
        sa_create_points_tables.append( s_sql );
        s_sql = QString::fromUtf8(( const char * ) sqlite3_column_text( stmt, 2 ) );
        sa_tables.append( s_sql );
        s_sql = QString::fromUtf8(( const char * ) sqlite3_column_text( stmt, 3 ) );
        sa_gcp_compute.append( s_sql );
      }
    }
    sqlite3_finalize( stmt );
    if ( sa_tables.size() > 0 )
    { // gcp_coverage
      sa_sql_commands = QgsSpatiaLiteProviderGcpUtils::createGcpSqlCoveragesCommands( parms_GcpDbData, sa_tables );
      if ( sa_sql_commands.size() > 0 )
      {
        s_sql_output += QString( "-- adding coverage entries into '%2'\n%1\n" ).arg( "---------------" ).arg( "gcp_coverage" );
        for ( int i_list = 0;i_list < sa_sql_commands.size();i_list++ )
        { // raster entry has been added to gcp_covarage, create corresponding tables
          s_sql = sa_sql_commands.at( i_list );
          s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
        }
        sa_sql_commands.clear();
      }
      sa_tables.clear();
    }
    else
    {
      s_sql_output += QString( "-- no coverage entries found in '%2'\n%1\n" ).arg( "---------------" ).arg( "gcp_coverages" );
    }
    if ( sa_gcp_compute.size() > 0 )
    {
      sa_sql_commands = QgsSpatiaLiteProviderGcpUtils::createGcpSqlCoveragesCommands( parms_GcpDbData, sa_gcp_compute );
      if ( sa_sql_commands.size() > 0 )
      {
        s_sql_output += QString( "-- adding compute entries into '%2'\n%1\n" ).arg( "---------------" ).arg( "gcp_compute" );
        for ( int i_list = 0;i_list < sa_sql_commands.size();i_list++ )
        { // raster entry has been added to gcp_covarage, create corresponding tables
          s_sql = sa_sql_commands.at( i_list );
          s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
        }
        sa_sql_commands.clear();
      }
      else
      {
        s_sql_output += QString( "-- no compute entries found in '%2'\n%1\n" ).arg( "---------------" ).arg( "gcp_compute" );
      }
    }
    // end of reading gcp_coverages
    // create point-tables
    if ( sa_create_points_tables.size() > 0 )
    {
      sa_sql_commands = QgsSpatiaLiteProviderGcpUtils::createGcpSqlPointsCommands( parms_GcpDbData, sa_create_points_tables );
      if ( sa_sql_commands.size() > 0 )
      {
        for ( int i_list = 0;i_list < sa_sql_commands.size();i_list++ )
        {
          s_sql = sa_sql_commands.at( i_list );
          if ( s_sql.contains( QString( "CREATE TABLE IF NOT EXISTS" ) ) )
          {
            QStringList sa_list = s_sql.split( "'" );
            s_sql_output += QString( "-- creating gcp_points TABLEs, TRIGGERs and VIEWs for '%2'\n%1\n" ).arg( "---------------" ).arg( sa_list.at( 1 ) );
          }
          if ( s_sql.contains( QString( "SELECT AddGeometryColumn(" ) ) )
          {
            s_sql_output += QString( "%1\n" ).arg( s_sql );
          }
          else
          {
            s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
          }
        }
        sa_sql_commands.clear();
      }
      sa_create_points_tables.clear();
    }
    // loop read point_tables
    if ( sa_points_tables.size() > 0 )
    {
      QString s_fields = QString( "id_gcp, id_gcp_coverage, name, notes, pixel_x, pixel_y, srid, point_x, point_y, order_selected, gcp_enable, gcp_text, gcp_pixel, gcp_point" );
      QString s_values = QString( "'('||id_gcp||','||id_gcp_coverage||','''||name||''','''|| notes||''','||pixel_x||','|| pixel_y||','||srid||','||point_x||','||point_y||','||order_selected||','||gcp_enable||','''||gcp_text||''','" );
      s_values += QString( "||'\n  GeomFromEWKT('''||AsEWKT(%1)||'''),'" ).arg( "gcp_pixel" );
      s_values += QString( "||'\n  GeomFromEWKT('''||AsEWKT(%1)||'''))'" ).arg( "gcp_point" );
      for ( int i_list = 0;i_list < sa_points_tables.size();i_list++ )
      {
        QString s_point_table = sa_points_tables.at( i_list );
        s_sql = QString( "SELECT\n %1 AS row_value\nFROM '%2'\nORDER BY %3" ).arg( s_values ).arg( s_point_table ).arg( "id_gcp" );
        ret = sqlite3_prepare_v2( db_handle, s_sql.toUtf8().constData(), -1, &stmt, NULL );
        if ( ret != SQLITE_OK )
        { //  rc=1 [no such table: vector_layers_statistics or gcp_points, 'no such column' for gcp_pixel and gcp_ppoint] is an error
          parms_GcpDbData->mError = QString( "Unable to check vector_layers_statistics, table gcp_points or geometries: gcp_pixel and gcp_point: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql );
          qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -22- [%1] " ).arg( parms_GcpDbData->mError );
          // End TRANSACTION
          ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
          Q_ASSERT( ret == SQLITE_OK );
          Q_UNUSED( ret );
          QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
          return false;
        }
        QString s_sql_insert = QString( "-- gcp_points of '%2'\n%1\n" ).arg( "---------------" ).arg( s_point_table );
        s_sql_insert += QString( "INSERT INTO '%1'\n (%2)" ).arg( s_point_table ).arg( s_fields );
        int i_count = 0;
        QString s_new_line = "\n VALUES";
        while ( sqlite3_step( stmt ) == SQLITE_ROW )
        {
          if ( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL )
          {
            s_sql = QString::fromUtf8(( const char * ) sqlite3_column_text( stmt, 0 ) );
            s_sql_insert += QString( "%1%2" ).arg( s_new_line ).arg( s_sql );
            if ( i_count == 0 )
            {
              s_new_line = ",\n ";
              i_count++;
            }
          }
        }
        s_sql_insert += QString( ";" );
        s_sql_output += QString( "%1\n%2\n" ).arg( s_sql_insert ).arg( s_sql_newline );
        s_sql_output += QString( "SELECT UpdateLayerStatistics('%1','gcp_pixel');\n" ).arg( s_point_table );
        s_sql_output += QString( "SELECT UpdateLayerStatistics('%1','gcp_point');\n%2\n" ).arg( s_point_table ).arg( s_sql_newline );
      }
      sa_points_tables.clear();
    }
    sa_gcp_compute.replaceInStrings( QString( "INSERT_COMPUTE" ), QString( "UPDATE_COMPUTE" ) );
    sa_sql_commands = QgsSpatiaLiteProviderGcpUtils::createGcpSqlCoveragesCommands( parms_GcpDbData, sa_gcp_compute );
    if ( sa_sql_commands.size() > 0 )
    {
      s_sql_output += QString( "-- updating coverage entries into '%2'\n%1\n" ).arg( "---------------" ).arg( "gcp_compute" );
      for ( int i_list = 0;i_list < sa_sql_commands.size();i_list++ )
      { // raster entry has been added to gcp_covarage, create corresponding tables
        s_sql = sa_sql_commands.at( i_list );
        if ( s_sql.contains( QString( "SELECT GCP_Compute(gcp_point, gcp_pixel, 0 ) FROM" ) ) )
        {
          QStringList sa_list = s_sql.split( "'" );
          s_sql_output += QString( "-- updating coverage entries for '%2'\n%1\n" ).arg( "---------------" ).arg( sa_list.at( 3 ) );
        }
        s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
      }
      sa_sql_commands.clear();
    }
    sa_gcp_compute.clear();
    //-- read cutlines_ [points, linestrings,polygons]
    QString s_cutline_fields = QString( "(id_cutline, id_gcp_coverage, cutline_type, name, notes, text, belongs_to_01, belongs_to_02, valid_since, valid_until, cutline_%1)" );
    QString s_cutline_values = QString( "VALUES('" );
    s_cutline_values += QString( "||%1" ).arg( "id_cutline" );
    s_cutline_values += QString( "||','||%1" ).arg( "id_gcp_coverage" );
    s_cutline_values += QString( "||','||%1" ).arg( "cutline_type" );
    s_cutline_values += QString( "||','''||%1" ).arg( "name" );
    s_cutline_values += QString( "||''','''||%1" ).arg( "notes" );
    s_cutline_values += QString( "||''','''||%1" ).arg( "text" );
    s_cutline_values += QString( "||''','''||%1" ).arg( "belongs_to_01" );
    s_cutline_values += QString( "||''','''||%1" ).arg( "belongs_to_02" );
    s_cutline_values += QString( "||''','''||%1" ).arg( "valid_since" );
    s_cutline_values += QString( "||''','''||%1" ).arg( "valid_until" );
    s_cutline_values += QString( "||''',\n  GeomFromEWKT('''||AsEWKT(cutline_%1)||'''));'\n" );
    s_cutline_values += QString( "FROM 'create_cutline_%1s' ORDER BY id_gcp_coverage,id_cutline;" );
    QString s_sql_select_insert = QString( "SELECT\n " );
    s_sql_select_insert += QString( "'INSERT INTO ''create_cutline_%1s''\n" );
    s_sql_select_insert += QString( "%1\n " ).arg( QString( s_cutline_fields ) );
    s_sql_select_insert += QString( "%1\n " ).arg( QString( s_cutline_values ) );
    QString s_update_layerstatistics = QString( "SELECT UpdateLayerStatistics('create_cutline_%1s','cutline_%1');" );
    //-------
    s_sql_select_coverages = QString( "%1" ).arg( QString( s_sql_select_insert ).arg( "point" ) );
    ret = sqlite3_prepare_v2( db_handle, s_sql_select_coverages.toUtf8().constData(), -1, &stmt, NULL );
    // qDebug() << QString( "sql_select_coverages[%1] " ).arg( s_sql_select_coverages );
    if ( ret != SQLITE_OK )
    { //  rc=1 [no such table: vector_layers_statistics or gcp_points, 'no such column' for gcp_pixel and gcp_ppoint] is an error
      parms_GcpDbData->mError = QString( "Unable to retrieve create_cutline_points: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql_select_coverages );
      qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -cutline_points- [%1] " ).arg( parms_GcpDbData->mError );
      // End TRANSACTION
      ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
      Q_ASSERT( ret == SQLITE_OK );
      Q_UNUSED( ret );
      QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
      return false;
    }
    while ( sqlite3_step( stmt ) == SQLITE_ROW )
    {
      if ( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL )
      {
        s_sql = QString::fromUtf8(( const char * ) sqlite3_column_text( stmt, 0 ) );
        sa_cutline_tables.append( s_sql );
      }
    }
    sqlite3_finalize( stmt );
    if ( sa_cutline_tables.size() > 0 )
    {
      s_sql_output += QString( "-- adding %3 cutline entries into '%2'\n%1\n" ).arg( "---------------" ).arg( "create_cutline_points" ).arg( sa_cutline_tables.size() );
      for ( int i_list = 0;i_list < sa_cutline_tables.size();i_list++ )
      { // raster entry has been added to gcp_covarage, create corresponding tables
        s_sql = sa_cutline_tables.at( i_list );
        s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
      }
      s_sql = QString( "%1 " ).arg( QString( s_update_layerstatistics ).arg( "point" ) );
      s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
      sa_cutline_tables.clear();
    }
    else
    {
      s_sql_output += QString( "-- no cutline rows found in '%2'\n%1\n" ).arg( "---------------" ).arg( "create_cutline_points" );
    }
    //-------
    s_sql_select_coverages = QString( "%1" ).arg( QString( s_sql_select_insert ).arg( "linestring" ) );
    ret = sqlite3_prepare_v2( db_handle, s_sql_select_coverages.toUtf8().constData(), -1, &stmt, NULL );
    // qDebug() << QString( "sql_select_coverages[%1] " ).arg( s_sql_select_coverages );
    if ( ret != SQLITE_OK )
    { //  rc=1 [no such table: vector_layers_statistics or gcp_points, 'no such column' for gcp_pixel and gcp_ppoint] is an error
      parms_GcpDbData->mError = QString( "Unable to retrieve create_cutline_linestrings: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql_select_coverages );
      qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -cutline_linestrings- [%1] " ).arg( parms_GcpDbData->mError );
      // End TRANSACTION
      ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
      Q_ASSERT( ret == SQLITE_OK );
      Q_UNUSED( ret );
      QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
      return false;
    }
    while ( sqlite3_step( stmt ) == SQLITE_ROW )
    {
      if ( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL )
      {
        s_sql = QString::fromUtf8(( const char * ) sqlite3_column_text( stmt, 0 ) );
        sa_cutline_tables.append( s_sql );
      }
      s_sql = QString( "%1 " ).arg( QString( s_update_layerstatistics ).arg( "linestring" ) );
      s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
    }
    sqlite3_finalize( stmt );
    if ( sa_cutline_tables.size() > 0 )
    {
      s_sql_output += QString( "-- adding %3 cutline entries into '%2'\n%1\n" ).arg( "---------------" ).arg( "create_cutline_linestrings" ).arg( sa_cutline_tables.size() );
      for ( int i_list = 0;i_list < sa_cutline_tables.size();i_list++ )
      { // raster entry has been added to gcp_covarage, create corresponding tables
        s_sql = sa_cutline_tables.at( i_list );
        s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
      }
      sa_cutline_tables.clear();
    }
    else
    {
      s_sql_output += QString( "-- no cutline rows found in '%2'\n%1\n" ).arg( "---------------" ).arg( "create_cutline_linestrings" );
    }
    //-------
    s_sql_select_coverages = QString( "%1" ).arg( QString( s_sql_select_insert ).arg( "polygon" ) );
    //-------
    ret = sqlite3_prepare_v2( db_handle, s_sql_select_coverages.toUtf8().constData(), -1, &stmt, NULL );
    // qDebug() << QString( "sql_select_coverages[%1] " ).arg( s_sql_select_coverages );
    if ( ret != SQLITE_OK )
    { //  rc=1 [no such table: vector_layers_statistics or gcp_points, 'no such column' for gcp_pixel and gcp_ppoint] is an error
      parms_GcpDbData->mError = QString( "Unable to retrieve create_cutline_polygons: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql_select_coverages );
      qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -cutline_polygons- [%1] " ).arg( parms_GcpDbData->mError );
      // End TRANSACTION
      ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
      Q_ASSERT( ret == SQLITE_OK );
      Q_UNUSED( ret );
      QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
      return false;
    }
    while ( sqlite3_step( stmt ) == SQLITE_ROW )
    {
      if ( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL )
      {
        s_sql = QString::fromUtf8(( const char * ) sqlite3_column_text( stmt, 0 ) );
        sa_cutline_tables.append( s_sql );
      }
    }
    sqlite3_finalize( stmt );
    if ( sa_cutline_tables.size() > 0 )
    {
      s_sql_output += QString( "-- adding %3 cutline entries into '%2'\n%1\n" ).arg( "---------------" ).arg( "create_cutline_polygons" ).arg( sa_cutline_tables.size() );
      for ( int i_list = 0;i_list < sa_cutline_tables.size();i_list++ )
      { // raster entry has been added to gcp_covarage, create corresponding tables
        s_sql = sa_cutline_tables.at( i_list );
        s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
      }
      s_sql = QString( "%1 " ).arg( QString( s_update_layerstatistics ).arg( "polygon" ) );
      s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
      sa_cutline_tables.clear();
    }
    else
    {
      s_sql_output += QString( "-- no cutline rows found in '%2'\n%1\n" ).arg( "---------------" ).arg( "create_cutline_polygons" );
    }
    //-------
    s_sql_select_coverages = QString( "%1" ).arg( QString( s_sql_select_insert ).arg( "polygon" ) );
    s_sql_select_coverages.replace( "cutline_", "mercator_" );
    s_sql_select_coverages.replace( "mercator_type", "cutline_type" );
    //-------
    ret = sqlite3_prepare_v2( db_handle, s_sql_select_coverages.toUtf8().constData(), -1, &stmt, NULL );
    // qDebug() << QString( "sql_select_coverages[%1] " ).arg( s_sql_select_coverages );
    if ( ret != SQLITE_OK )
    { //  rc=1 [no such table: vector_layers_statistics or gcp_points, 'no such column' for gcp_pixel and gcp_ppoint] is an error
      parms_GcpDbData->mError = QString( "Unable to retrieve create_mercator_polygons: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql_select_coverages );
      qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpDb -mercator_polygons- [%1] " ).arg( parms_GcpDbData->mError );
      // End TRANSACTION
      ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
      Q_ASSERT( ret == SQLITE_OK );
      Q_UNUSED( ret );
      QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
      return false;
    }
    while ( sqlite3_step( stmt ) == SQLITE_ROW )
    {
      if ( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL )
      {
        s_sql = QString::fromUtf8(( const char * ) sqlite3_column_text( stmt, 0 ) );
        sa_cutline_tables.append( s_sql );
      }
    }
    sqlite3_finalize( stmt );
    if ( sa_cutline_tables.size() > 0 )
    {
      s_sql_output += QString( "-- adding %3 mercator_polygon cutline entries into '%2'\n%1\n" ).arg( "---------------" ).arg( "create_mercator_polygons" ).arg( sa_cutline_tables.size() );
      for ( int i_list = 0;i_list < sa_cutline_tables.size();i_list++ )
      { // raster entry has been added to gcp_covarage, create corresponding tables
        s_sql = sa_cutline_tables.at( i_list );
        s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
      }
      s_sql = QString( "%1 " ).arg( QString( s_update_layerstatistics ).arg( "polygon" ) );
      s_sql.replace( "cutline_", "mercator_" );
      s_sql.replace( "mercator_type", "cutline_type" );
      s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
      sa_cutline_tables.clear();
    }
    else
    {
      s_sql_output += QString( "-- no cutline rows found in '%2' for mercator_polygon\n%1\n" ).arg( "---------------" ).arg( "create_mercator_polygons" );
    }
    //-------
    QString s_transform_compute = QString( "TRANSFORM_COMPUTE%1%2" ).arg( parms_GcpDbData->mParseString ).arg( "create_mercator_polygons" );
    s_transform_compute += QString( "%1%2" ).arg( parms_GcpDbData->mParseString ).arg( "mercator_polygon" );
    s_transform_compute += QString( "%1%2" ).arg( parms_GcpDbData->mParseString ).arg( "cutline_polygon" );
    s_transform_compute += QString( "%1%2" ).arg( parms_GcpDbData->mParseString ).arg( 0 ); // 0=Dump all coverages
    s_transform_compute += QString( "%1%2" ).arg( parms_GcpDbData->mParseString ).arg( 0 );
    QStringList sa_transform_compute;
    sa_transform_compute.append( s_transform_compute );
    sa_sql_commands = QgsSpatiaLiteProviderGcpUtils::createGcpSqlCoveragesCommands( parms_GcpDbData, sa_transform_compute );
    if ( sa_sql_commands.size() > 0 )
    {
      for ( int i_list = 0;i_list < sa_sql_commands.size();i_list++ )
      { // raster entry has been added to gcp_covarage, create corresponding tables
        s_sql = sa_sql_commands.at( i_list );
        s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
      }
      sa_sql_commands.clear();
    }
    sa_transform_compute.clear();
    //-------
    // End TRANSACTION
    ret = sqlite3_exec( db_handle, "COMMIT", NULL, NULL, 0 );
    Q_ASSERT( ret == SQLITE_OK );
    Q_UNUSED( ret );
    if ( !s_sql_output.isNull() )
    { // writing of Sql-Dump
      QFile::FileError file_result = QgsSpatiaLiteProviderGcpUtils::writeGcpSqlDump( s_database_filename, s_sql_output, "sql" );
      if ( file_result != QFile::NoError )
      {
        b_gcp_coverage = false;
        parms_GcpDbData->mError = QString( "Failed to create Gcp-Coverage Sql-Dump [expected output > %1 bytes]: FileError=%2 database[%3]" ).arg( s_sql_output.length() ).arg( file_result ).arg( s_database_filename );
        s_sql_output = QString::null;
        QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
        return b_gcp_coverage;
      }
      else
      {
        parms_GcpDbData->mDatabaseValid = true;
      }
      s_sql_output = QString::null;
    }
    else
    { // Dump failed, set as error
      b_gcp_coverage = false;
      parms_GcpDbData->mError = QString( "Failed to create Gcp-Coverage Sql-Dump [file exists]: rc=%1 database[%2]" ).arg( 1 ).arg( s_database_filename );
      QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
      return b_gcp_coverage;
    }
  }
  if ( !parms_GcpDbData->mDatabaseDump )
  {
    parms_GcpDbData->mGcpSrid = i_srid;
    parms_GcpDbData->mGcp_points_table_name = s_points_table_name;
    parms_GcpDbData->mId_gcp_coverage = id_gcp_coverage;
    parms_GcpDbData->mId_gcp_cutline = id_cutline;
    parms_GcpDbData->mTransformParam = i_TransformParam;
    parms_GcpDbData->mResamplingMethod = s_ResamplingMethod;
    parms_GcpDbData->mCompressionMethod = s_CompressionMethod;
    parms_GcpDbData->mRasterNodata = i_RasterNoData;
    parms_GcpDbData->mGcp_coverage_title = s_coverage_title;
    parms_GcpDbData->mGcp_coverage_abstract = s_coverage_abstract;
    parms_GcpDbData->mGcp_coverage_copyright = s_coverage_copyright;
    parms_GcpDbData->mGcp_coverage_map_date = s_map_date;
  }
  else
  { // when dumping the Database, no need for Update
    parms_GcpDbData->mGcpSrid = i_srid;
    QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
    return b_database_exists;
  }
  if ( parms_GcpDbData->mSqlDump )
  {
    if ( !s_sql_output.isNull() )
    { // writing of Sql-Dump
      QFile::FileError file_result = QgsSpatiaLiteProviderGcpUtils::writeGcpSqlDump( s_database_filename, s_sql_output, "sql" );
      if ( file_result != QFile::NoError )
      {
        b_gcp_coverage = false;
        parms_GcpDbData->mError = QString( "Failed to create Gcp-Master Sql-Dump [expected output > %1 bytes]: FileError=%2 database[%3]" ).arg( s_sql_output.length() ).arg( file_result ).arg( s_database_filename );
        s_sql_output = QString::null;
        return b_gcp_coverage;
      }
      s_sql_output = QString::null;
    }
    else
    { // Dump failed, set as error
      b_gcp_coverage = false;
      parms_GcpDbData->mError = QString( "Failed to create Gcp-Coverage Sql-Dump [file exists]: rc=%1 database[%2]" ).arg( 1 ).arg( s_database_filename );
      return b_gcp_coverage;
    }
  }
  // Update extent of active raster [will resuse the connection]
  // With a new Database, will return true if Database was found
  b_database_exists = QgsSpatiaLiteProviderGcpUtils::updateGcpDb( parms_GcpDbData );
  QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
  return b_database_exists;
}
bool QgsSpatiaLiteProviderGcpUtils::updateGcpDb( GcpDbData* parms_GcpDbData )
{
  if ( !parms_GcpDbData )
    return false;
  QString  s_database_filename = parms_GcpDbData->mGcpDatabaseFileName;
  bool b_database_exists = QFile( s_database_filename ).exists();
  QString s_coverage_name = parms_GcpDbData->mGcp_coverage_name;
  if (( s_coverage_name.isEmpty() ) || ( s_coverage_name.isNull() ) )
  { // The Database is being opened to only to retrieve a list of coverages.
    return b_database_exists;
  }
  int i_srid = parms_GcpDbData->mGcpSrid;
  int i_gcp_coverage = parms_GcpDbData->mId_gcp_coverage;
  QgsRasterLayer *raster_layer = parms_GcpDbData->mLayer;
  QString s_GCPbaseFileName = parms_GcpDbData->mGcpBaseFileName;
  QString s_ModifiedRasterFileName = parms_GcpDbData->mModifiedRasterFileName;
  int i_TransformParam = parms_GcpDbData->mTransformParam;
  QString s_ResamplingMethod = parms_GcpDbData->mResamplingMethod;
  int i_RasterYear = parms_GcpDbData->mRasterYear;
  int i_RasterScale = parms_GcpDbData->mRasterScale;
  int i_RasterNoData = parms_GcpDbData->mRasterNodata;
  QString s_CompressionMethod = parms_GcpDbData->mCompressionMethod;
  if ( b_database_exists )
  {
    s_coverage_name = s_coverage_name.toLower();
    QString s_gcp_points_table_name = QString( "gcp_points_%1" ).arg( s_coverage_name );
    s_ModifiedRasterFileName = QString( "%1.%2.map.%3" ).arg( s_GCPbaseFileName ).arg( i_srid ).arg( "tif" );
    QString s_coverage_title = "";
    QString s_coverage_abstract = "";
    QString s_coverage_copyright = "";
    int i_image_max_x = 0;
    int i_image_max_y = 0;
    int i_cutline_type = 77;
    // search id_cutline from create_cutline_polygons, where cutline_type=77 with same id_gcp_coverage OR return -1
    // - if more than 1: then the latest 'max(id_cutline)'
    QString s_cutline_type_77 = QString( "SELECT\n    max(id_cutline)\n   FROM\n    '%1'\n   WHERE\n   ((id_gcp_coverage=%2) AND (cutline_type=%3))" ).arg( "create_cutline_polygons" ).arg( i_gcp_coverage ).arg( i_cutline_type );
    QString s_result_id = QString( " SELECT\n  (\n   %1\n  ) AS result_id" ).arg( s_cutline_type_77 );
    QString s_result_cutline = QString( " FROM\n (\n %1\n ) AS result_cutline" ).arg( s_result_id );
    QString s_result_case = QString( "CASE WHEN result_cutline.result_id IS NULL OR result_cutline.result_id < 0 THEN -1 ELSE NEW.result_cutline.result_id END id_result" );
    QString s_query_id_cutline = QString( "\n(\n SELECT\n  %1\n%2\n)" ).arg( s_result_case ).arg( s_result_cutline );
    //----
    sqlite3* db_handle;
    char *errMsg = nullptr;
    // open database
    // Note: this will not repopen the connection when called from another function such as createGcpDb
    int ret = QgsSpatiaLiteProviderGcpUtils::spatialiteInitEx( parms_GcpDbData, s_database_filename );
    if ( ret != SQLITE_OK )
    {
      parms_GcpDbData->mError = QString( "Failed to open database: rc=%1 database[%2]" ).arg( ret ).arg( s_database_filename );
      if ( ret != SQLITE_MISUSE )
      { // 'SQLITE_MISUSE': in use with another file, do not close
        QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
      }
      return false;
    }
    // To simplfy the useage.
    db_handle = parms_GcpDbData->db_handle;
    QString s_sql_fields = QString( " transformtype=%1," ).arg( i_TransformParam );
    s_sql_fields += QString( " resampling='%1'," ).arg( s_ResamplingMethod );
    s_sql_fields += QString( " compression='%1'," ).arg( s_CompressionMethod );
    s_sql_fields += QString( " raster_gtif='%1'," ).arg( s_ModifiedRasterFileName );
    s_sql_fields += QString( " map_date='%1'," ).arg( QString( "%1-01-01" ).arg( i_RasterYear, 4, 10, QChar( '0' ) ) );
    s_sql_fields += QString( " scale=%1," ).arg( i_RasterScale );
    s_sql_fields += QString( " nodata=%1," ).arg( i_RasterNoData );
    if ( raster_layer )
    {
      i_image_max_x = raster_layer->width();
      i_image_max_y = raster_layer->height();
      s_coverage_title = raster_layer->title();
      s_coverage_abstract = raster_layer->abstract();
      s_coverage_copyright = raster_layer->keywordList();
    }
    if ( i_image_max_x > 0 )
    {
      s_sql_fields += QString( "image_max_x=%1," ).arg( i_image_max_x );
      s_sql_fields += QString( "image_max_y=%1," ).arg( i_image_max_y );
    }
    if ( ! s_coverage_title.isEmpty() )
    {
      s_sql_fields += QString( "title='%1'," ).arg( s_coverage_title );
    }
    if ( ! s_coverage_abstract.isEmpty() )
    {
      s_sql_fields += QString( "abstract='%1'," ).arg( s_coverage_abstract );
    }
    if ( ! s_coverage_copyright.isEmpty() )
    {
      s_sql_fields += QString( "copyright='%1'," ).arg( s_coverage_copyright );
    }
    s_sql_fields += QString( "\n" ); // Making created Subqueries more readable
    QString s_update_extent = QString( "UPDATE_COVERAGE_EXTENT%1%2%1%3" ).arg( parms_GcpDbData->mParseString ).arg( i_gcp_coverage ).arg( s_coverage_name );
    QStringList sa_tables;
    sa_tables.append( s_update_extent );
    QStringList sa_sql_commands;
    // Creates UPDATE command with Subqueries to external table [avoiding creation of the same Subqueries in different functions]
    sa_sql_commands = QgsSpatiaLiteProviderGcpUtils::createGcpSqlCoveragesCommands( parms_GcpDbData, sa_tables );
    if ( sa_sql_commands.size() == 2 )
    { // UPDATE extent with gcp-points count
      QString s_sql_commands = sa_sql_commands.at( 0 );
      QString s_sql = QString( s_sql_commands ).arg( s_sql_fields );
      ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
      if ( ret != SQLITE_OK )
      {
        parms_GcpDbData->mError = QString( "Unable to execute Update of gcp_coverages for '%1': rc=%2 [%3] sql[%4]\n" ).arg( s_gcp_points_table_name ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
        qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::updateGcpDb -01- [%1] " ).arg( parms_GcpDbData->mError );
        sqlite3_free( errMsg );
      }
      // UPDATE extent_min/max x/y to 0 where gcp-points count == 0 [prevent NULL values]
      s_sql = sa_sql_commands.at( 1 );
      ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
      if ( ret != SQLITE_OK )
      {
        parms_GcpDbData->mError = QString( "Unable to execute Update of gcp_coverages for '%1': rc=%2 [%3] sql[%4]\n" ).arg( "all coverages preventing NULL values" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
        qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::updateGcpDb -02- [%1] " ).arg( parms_GcpDbData->mError );
        sqlite3_free( errMsg );
      }
    }
    parms_GcpDbData->mModifiedRasterFileName = s_ModifiedRasterFileName;
    // Note: this will not close the connection when called from another function such as createGcpDb
    QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
  }
  else
  {
    parms_GcpDbData->mError = QString( "QgsSpatiaLiteProviderGcpUtils::updateGcpDb: Database error: rc=%1 database[%2] filename[%3]" ).arg( 1 ).arg( b_database_exists ).arg( s_database_filename );
  }
  return b_database_exists;
}
bool QgsSpatiaLiteProviderGcpUtils::updateGcpCompute( GcpDbData* parms_GcpDbData )
{
  if ( !parms_GcpDbData )
    return false;
  QString  s_database_filename = parms_GcpDbData->mGcpDatabaseFileName;
  QString s_coverage_name = parms_GcpDbData->mGcp_coverage_name;
  QgsRasterLayer *raster_layer = parms_GcpDbData->mLayer;
  bool b_database_exists = QFile( s_database_filename ).exists();
  s_coverage_name = s_coverage_name.toLower();
  QString s_gcp_points_table_name = QString( "gcp_points_%1" ).arg( s_coverage_name );
  QString s_pixel_point_to_map_point = "Pixel-Point to Map-Point";
  QString s_map_point_to_pixel_point = "Map-Point to Pixel-Point";
  if ( b_database_exists )
  {
    sqlite3* db_handle;
    char *errMsg = nullptr;
    sqlite3_stmt* stmt;
    // open database
    int ret = QgsSpatiaLiteProviderGcpUtils::spatialiteInitEx( parms_GcpDbData, s_database_filename );
    if ( ret != SQLITE_OK )
    {
      parms_GcpDbData->mError = QString( "Failed to open database: rc=%1 database[%2]" ).arg( ret ).arg( s_database_filename );
      if ( ret != SQLITE_MISUSE )
      { // 'SQLITE_MISUSE': in use with another file, do not close
        QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
      }
      return false;
    }
    // To simplfy the useage.
    db_handle = parms_GcpDbData->db_handle;
    // check if gcp_compute contains entries for this coverage
    QString s_sql = QString( "SELECT (SELECT count(coverage_name) FROM gcp_compute WHERE (coverage_name='%1')) AS has_gcp_compute," ).arg( s_coverage_name );
    s_sql += QString( " (SELECT count(coverage_name) FROM gcp_coverages WHERE (coverage_name='%1')) AS has_gcp_coverage," ).arg( s_coverage_name );
    s_sql += QString( " (SELECT id_gcp_coverage FROM gcp_coverages WHERE (coverage_name='%1')) AS id_gcp_coverage," ).arg( s_coverage_name );
    s_sql += QString( " (SELECT srid FROM gcp_coverages WHERE (coverage_name='%1')) AS id_coverage_srid" ).arg( s_coverage_name );
    ret = sqlite3_prepare_v2( db_handle, s_sql.toUtf8().constData(), -1, &stmt, NULL );
    if ( ret != SQLITE_OK )
    { //  rc=1 [no such function: HasGCP] is not an error [older spatialite version]
      parms_GcpDbData->mError = QString( "Unable to execute reading of gcp_coverages for '%1': rc=%2 [%3] sql[%4]\n" ).arg( s_coverage_name ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql );
      qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::updateGcpCompute -metadata- [%1] " ).arg( parms_GcpDbData->mError );
    }
    int i_gcp_compute = -1;
    int i_gcp_coverage = -1;
    int i_gcp_coverage_id = -1;
    int i_gcp_coverage_srid = -1;
    while ( sqlite3_step( stmt ) == SQLITE_ROW )
    {
      if ( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL )
        i_gcp_compute = sqlite3_column_int( stmt, 0 );
      if ( sqlite3_column_type( stmt, 1 ) != SQLITE_NULL )
        i_gcp_coverage = sqlite3_column_int( stmt, 1 );
      if ( sqlite3_column_type( stmt, 2 ) != SQLITE_NULL )
        i_gcp_coverage_id = sqlite3_column_int( stmt, 2 );
      if ( sqlite3_column_type( stmt, 3 ) != SQLITE_NULL )
        i_gcp_coverage_srid = sqlite3_column_int( stmt, 3 );
    }
    sqlite3_finalize( stmt );
    QString s_coverage_title = "";
    QString s_coverage_abstract = "";
    QString s_coverage_copyright = "";
    int i_image_max_x = 0;
    int i_image_max_y = 0;
    if ( raster_layer )
    {
      i_image_max_x = raster_layer->width();
      i_image_max_y = raster_layer->height();
      s_coverage_title = raster_layer->title();
      s_coverage_abstract = raster_layer->abstract();
      s_coverage_copyright = raster_layer->keywordList();
    }
    QString s_gcp_compute = QString( "INSERT_COMPUTE%1%2" ).arg( parms_GcpDbData->mParseString ).arg( i_gcp_coverage_id );
    s_gcp_compute += QString( "%1%2" ).arg( parms_GcpDbData->mParseString ).arg( i_gcp_coverage_srid );
    s_gcp_compute += QString( "%1%2" ).arg( parms_GcpDbData->mParseString ).arg( s_coverage_name );
    s_gcp_compute += QString( "%1%2" ).arg( parms_GcpDbData->mParseString ).arg( i_image_max_x );
    s_gcp_compute += QString( "%1%2" ).arg( parms_GcpDbData->mParseString ).arg( i_image_max_y );
    QStringList sa_gcp_compute;
    if (( i_gcp_compute < 1 ) && ( i_gcp_coverage > 0 ) )
    { //  "INSERT_COMPUTE;#;#1;#;#3068;#;#1909.straube_blatt_i_a.4000;#;#5629;#;#4472"
      sa_gcp_compute.append( s_gcp_compute );
      QStringList sa_sql_commands = QgsSpatiaLiteProviderGcpUtils::createGcpSqlCoveragesCommands( parms_GcpDbData, sa_gcp_compute );
      if ( sa_sql_commands.size() > 0 )
      {
        for ( int i_list = 0;i_list < sa_sql_commands.size();i_list++ )
        { // raster entry has been added to gcp_covarage, create corresponding tables
          s_sql = sa_sql_commands.at( i_list );
          ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
          if ( ret != SQLITE_OK )
          {
            parms_GcpDbData->mError = QString( "Unable to INSERT INTO '%1': rc=%2 [%3] sql[%4]\n" ).arg( "gcp_compute" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
            qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::updateGcpCompute - Order 0-3 - %1 - [%2] " ).arg( s_map_point_to_pixel_point ).arg( parms_GcpDbData->mError );
            sqlite3_free( errMsg );
          }
        }
        sa_sql_commands.clear();
      }
      sa_gcp_compute.clear();
    }
    s_gcp_compute.replace( QString( "INSERT_COMPUTE" ), QString( "UPDATE_COMPUTE" ) );
    sa_gcp_compute.append( s_gcp_compute );
    QStringList sa_sql_commands = QgsSpatiaLiteProviderGcpUtils::createGcpSqlCoveragesCommands( parms_GcpDbData, sa_gcp_compute );
    if ( sa_sql_commands.size() > 0 )
    {
      for ( int i_list = 0;i_list < sa_sql_commands.size();i_list++ )
      { // raster entry has been added to gcp_covarage, create corresponding tables
        s_sql = sa_sql_commands.at( i_list );
        ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
        if ( ret != SQLITE_OK )
        {
          parms_GcpDbData->mError = QString( "Unable to UPDATE  '%1': rc=%2 [%3] sql[%4]\n" ).arg( "gcp_compute" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
          qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::updateGcpCompute - Order 0-3 - %1 - [%2] " ).arg( s_map_point_to_pixel_point ).arg( parms_GcpDbData->mError );
          sqlite3_free( errMsg );
        }
      }
      sa_sql_commands.clear();
    }
    sa_gcp_compute.clear();
    //----------------
    QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
  }
  return b_database_exists;
}
bool QgsSpatiaLiteProviderGcpUtils::updateGcpTranslate( GcpDbData* parms_GcpDbData )
{
  if ( !parms_GcpDbData )
    return false;
  QString  s_database_filename = parms_GcpDbData->mGcpDatabaseFileName;
  QString s_coverage_name = parms_GcpDbData->mGcp_coverage_name;
  bool b_database_exists = QFile( s_database_filename ).exists();
  s_coverage_name = s_coverage_name.toLower();
  QString s_gcp_points_table_name = QString( "gcp_points_%1" ).arg( s_coverage_name );
  QString s_pixel_point_to_map_point = "Pixel-Point to Map-Point";
  QString s_map_point_to_pixel_point = "Map-Point to Pixel-Point";
  if ( b_database_exists )
  {
    sqlite3* db_handle;
    char *errMsg = nullptr;
    sqlite3_stmt* stmt;
    // open database
    int ret = QgsSpatiaLiteProviderGcpUtils::spatialiteInitEx( parms_GcpDbData, s_database_filename );
    if ( ret != SQLITE_OK )
    {
      parms_GcpDbData->mError = QString( "Failed to open database: rc=%1 database[%2]" ).arg( ret ).arg( s_database_filename );
      if ( ret != SQLITE_MISUSE )
      { // 'SQLITE_MISUSE': in use with another file, do not close
        QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
      }
      return false;
    }
    // To simplfy the useage.
    db_handle = parms_GcpDbData->db_handle;
    // check if gcp_compute contains entries for this coverage
    QString s_sql = QString( "SELECT (SELECT count(coverage_name) FROM gcp_compute WHERE (coverage_name='%1')) AS has_gcp_compute," ).arg( s_coverage_name );
    s_sql += QString( " (SELECT count(coverage_name) FROM gcp_coverages WHERE (coverage_name='%1')) AS has_gcp_coverage," ).arg( s_coverage_name );
    s_sql += QString( " (SELECT id_gcp_coverage FROM gcp_coverages WHERE (coverage_name='%1')) AS id_gcp_coverage," ).arg( s_coverage_name );
    s_sql += QString( " (SELECT srid FROM gcp_coverages WHERE (coverage_name='%1')) AS id_coverage_srid" ).arg( s_coverage_name );
    ret = sqlite3_prepare_v2( db_handle, s_sql.toUtf8().constData(), -1, &stmt, NULL );
    if ( ret != SQLITE_OK )
    { //  rc=1 [no such function: HasGCP] is not an error [older spatialite version]
      parms_GcpDbData->mError = QString( "Unable to execute reading of gcp_coverages for '%1': rc=%2 [%3] sql[%4]\n" ).arg( s_coverage_name ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql );
      qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::updateGcpTranslate -metadata- [%1] " ).arg( parms_GcpDbData->mError );
    }
    int i_gcp_compute = -1;
    int i_gcp_coverage = -1;
    int i_gcp_coverage_id = -1;
    int i_gcp_coverage_srid = -1;
    while ( sqlite3_step( stmt ) == SQLITE_ROW )
    {
      if ( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL )
        i_gcp_compute = sqlite3_column_int( stmt, 0 );
      if ( sqlite3_column_type( stmt, 1 ) != SQLITE_NULL )
        i_gcp_coverage = sqlite3_column_int( stmt, 1 );
      if ( sqlite3_column_type( stmt, 2 ) != SQLITE_NULL )
        i_gcp_coverage_id = sqlite3_column_int( stmt, 2 );
      if ( sqlite3_column_type( stmt, 3 ) != SQLITE_NULL )
        i_gcp_coverage_srid = sqlite3_column_int( stmt, 3 );
    }
    sqlite3_finalize( stmt );
    if (( i_gcp_compute > 0 ) && ( i_gcp_coverage > 0 ) )
    {
      // "TRANSFORM_COMPUTE;#;#create_mercator_polygons;#;#mercator_polygon;#;#cutline_polygon#;#i_gcp_coverage_id#;#i_gcp_coverage_srid"
      QString s_transform_compute = QString( "TRANSFORM_COMPUTE%1%2" ).arg( parms_GcpDbData->mParseString ).arg( "create_mercator_polygons" );
      s_transform_compute += QString( "%1%2" ).arg( parms_GcpDbData->mParseString ).arg( "mercator_polygon" );
      s_transform_compute += QString( "%1%2" ).arg( parms_GcpDbData->mParseString ).arg( "cutline_polygon" );
      s_transform_compute += QString( "%1%2" ).arg( parms_GcpDbData->mParseString ).arg( i_gcp_coverage_id );
      s_transform_compute += QString( "%1%2" ).arg( parms_GcpDbData->mParseString ).arg( i_gcp_coverage_srid );
      QStringList sa_transform_compute;
      sa_transform_compute.append( s_transform_compute );
      QStringList sa_sql_commands = QgsSpatiaLiteProviderGcpUtils::createGcpSqlCoveragesCommands( parms_GcpDbData, sa_transform_compute );
      if ( sa_sql_commands.size() > 0 )
      {
        for ( int i_list = 0;i_list < sa_sql_commands.size();i_list++ )
        { // raster entry has been added to gcp_covarage, create corresponding tables
          s_sql = sa_sql_commands.at( i_list );
          // qDebug() << QString( "updateGcpTranslate : fgcp_coverage_id  [%1] srid[%3] sql[%2]" ).arg( i_gcp_coverage_id ).arg( s_sql ).arg( i_gcp_coverage_srid );
          ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
          if ( ret != SQLITE_OK )
          {
            parms_GcpDbData->mError = QString( "Unable to UPDATE  '%1': rc=%2 [%3] sql[%4]\n" ).arg( "gcp_compute" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
            qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::updateGcpTranslate - Order %3 - %1 - [%2] " ).arg( s_map_point_to_pixel_point ).arg( parms_GcpDbData->mError ).arg( parms_GcpDbData->mOrder );
            sqlite3_free( errMsg );
          }
        }
        sa_sql_commands.clear();
      }
      sa_transform_compute.clear();
    }
    //----------------
    QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
  }
  return b_database_exists;
}
QgsPoint QgsSpatiaLiteProviderGcpUtils::getGcpConvert( GcpDbData* parms_GcpDbData )
{
  double point_x = 0;
  double point_y = 0;
  QgsPoint convert_point( point_x, point_y );
  if ( !parms_GcpDbData )
    return convert_point;
  QString  s_database_filename = parms_GcpDbData->mGcpDatabaseFileName;
  QString s_coverage_name = parms_GcpDbData->mGcp_coverage_name;
  QgsPoint input_point = parms_GcpDbData->mInputPoint;
  bool b_toPixel = parms_GcpDbData->mToPixel;
  int i_order = parms_GcpDbData->mOrder;
  bool b_reCompute = parms_GcpDbData->mReCompute;
  int id_gcp = parms_GcpDbData->mIdGcp;
  bool b_database_exists = QFile( s_database_filename ).exists();
  s_coverage_name = s_coverage_name.toLower();
  QString s_gcp_points_table_name = QString( "gcp_points_%1" ).arg( s_coverage_name );
  QString s_pixel_point_to_map_point = "Pixel-Point to Map-Point";
  QString s_map_point_to_pixel_point = "Map-Point to Pixel-Point";
  if ( b_database_exists )
  {
    // qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::getGcpConvert start - 01 - toPixel[%3] point[%1,%2]  " ).arg( input_point.x() ).arg( input_point.y() ).arg( b_toPixel );
    sqlite3* db_handle;
    sqlite3_stmt* stmt;
    // open database
    int ret = QgsSpatiaLiteProviderGcpUtils::spatialiteInitEx( parms_GcpDbData, s_database_filename );
    if ( ret != SQLITE_OK )
    {
      parms_GcpDbData->mError = QString( "Failed to open database: rc=%1 database[%2]" ).arg( ret ).arg( s_database_filename );
      if ( ret != SQLITE_MISUSE )
      { // 'SQLITE_MISUSE': in use with another file, do not close
        QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
      }
      return convert_point;
    }
    // To simplfy the useage.
    db_handle = parms_GcpDbData->db_handle;
    QString s_srid_request = QString( "srid=-1" );
    if ( b_toPixel )
    { // QgsPoint contains no srid information
      s_srid_request = QString( "srid<>-1" );
    }
    if (( i_order < 0 ) || ( i_order > 3 ) )
    {
      i_order = 0;
    }
    // check if gcp_compute contains entries for this coverage
    QString s_sql = QString( "SELECT HasGCP(), (SELECT count(coverage_name) FROM gcp_compute WHERE ((coverage_name='%1') AND (id_order=%2) AND (%3))) AS has_gcp_compute," ).arg( s_coverage_name ).arg( i_order ).arg( s_srid_request );
    s_sql += QString( " (SELECT count(coverage_name) FROM gcp_coverages WHERE (coverage_name='%1')) AS has_gcp_coverags," ).arg( s_coverage_name );
    s_sql += QString( " (SELECT id_gcp_coverage FROM gcp_coverages WHERE (coverage_name='%1')) AS id_gcp_coverage," ).arg( s_coverage_name );
    s_sql += QString( " (SELECT srid FROM gcp_coverages WHERE (coverage_name='%1')) AS srid_gcp_coverage," ).arg( s_coverage_name );
    s_sql += QString( " (SELECT count(id_gcp) FROM \"%1\" WHERE (gcp_enable=1)) AS count_gcp" ).arg( s_gcp_points_table_name );
    ret = sqlite3_prepare_v2( db_handle, s_sql.toUtf8().constData(), -1, &stmt, NULL );
    if ( ret != SQLITE_OK )
    { //  rc=1 [no such function: HasGCP] is not an error [older spatialite version]
      parms_GcpDbData->mError = QString( "Unable to execute reading of gcp_coverages for '%1': rc=%2 [%3] sql[%4]\n" ).arg( s_coverage_name ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql );
      qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::getGcpConvert -metadata- [%1] " ).arg( parms_GcpDbData->mError );
    }
    int i_spatialite_gcp_enabled = 0;
    int i_gcp_compute = -1;
    int i_gcp_coverage = -1;
    int i_gcp_coverage_id = -1;
    int i_gcp_coverage_srid = -1;
    int i_gcp_count = -1;
    while ( sqlite3_step( stmt ) == SQLITE_ROW )
    {
      i_spatialite_gcp_enabled = sqlite3_column_int( stmt, 0 );
      if ( sqlite3_column_type( stmt, 1 ) != SQLITE_NULL )
        i_gcp_compute = sqlite3_column_int( stmt, 1 );
      if ( sqlite3_column_type( stmt, 2 ) != SQLITE_NULL )
        i_gcp_coverage = sqlite3_column_int( stmt, 2 );
      if ( sqlite3_column_type( stmt, 3 ) != SQLITE_NULL )
        i_gcp_coverage_id = sqlite3_column_int( stmt, 3 );
      if ( sqlite3_column_type( stmt, 4 ) != SQLITE_NULL )
        i_gcp_coverage_srid = sqlite3_column_int( stmt, 4 );
      if ( sqlite3_column_type( stmt, 5 ) != SQLITE_NULL )
        i_gcp_count = sqlite3_column_int( stmt, 5 );
    }
    sqlite3_finalize( stmt );
    if ( i_spatialite_gcp_enabled == 1 )
    { // https://www.gaia-gis.it/fossil/libspatialite/wiki?name=Ground+Control+Points
      // 0: the GCP problem will be solved by applying the Thin Plate Spline (aka TPS) method.
      // 1: (default) the GCP problem will be solved by applying the RMSE regression method and will return a set of polynomial coefficients of the 1st order.
      // 2: the GCP problem will be solved by applying the RMSE regression method and will return a set of polynomial coefficients of the 2nd order.
      // 3: the GCP problem will be solved by applying the RMSE regression method and will return a set of polynomial coefficients of the 3rd order.
      bool b_valid = true;
      if (( i_gcp_count < 4 ) || ( i_gcp_coverage <= 0 ) || ( i_gcp_coverage_id <= 0 ) )
      {
        b_valid = false;
      }
      if ( b_valid )
      {
        if ( i_gcp_compute != 1 )
        { // Either: no gcp_convert TABLE was found OR no row for this map with srid and order was found [TABLE cannot be queried]
          b_reCompute = true;
        }
        QString s_pixel_point_to_map_point = "Pixel-Point to Map-Point";
        QString s_map_point_to_pixel_point = "Map-Point to Pixel-Point";
        QString s_transform_type = QString( "Order %1 - " ).arg( i_order );
        // A postion is being requested (not yet in the gcp-list) ['id_gcp' is ignored]
        QString s_sql_base = QString( "SELECT \n TRANSFORM\nFROM\n COMPUTE\nLIMIT 1;" );
        if ( id_gcp >= 0 )
        { // A specific gcp is being requested ['input_point' is ignored]
          s_sql_base = QString( "SELECT \n TRANSFORM\nFROM\n \"%1\" AS gcp, \n COMPUTE\nWHERE (id_gcp=%2);" ).arg( s_gcp_points_table_name ).arg( id_gcp );
        }
        QString s_sql_transform_base = "";
        QString s_sql_transform = "";
        QString s_sql_compute = "";
        // Thin Plate Spline will fail if any of the points are 0
        QString s_recompute_where = "((gcp_enable=1) AND (((ST_X(gcp_point)<>0) AND (ST_Y(gcp_point)<>0)) AND ((ST_X(gcp_pixel)<>0) AND (ST_Y(gcp_pixel)<>0))))";
        if ( b_toPixel )
        { // Map-Point to Pixel-Point
          s_transform_type += s_map_point_to_pixel_point;
          if ( id_gcp >= 0 )
          { // A specific gcp is being requested ['input_point' is ignored]
            s_sql_transform_base = QString( "GCP_Transform(gcp.gcp_point , g_pixel.matrix,-1)" );
            s_sql_transform   = QString( "ST_X(%1) AS gcp_pixel_x,\n " ).arg( s_sql_transform_base );
            s_sql_transform += QString( "ST_Y(%1) AS gcp_pixel_y" ).arg( s_sql_transform_base );
          }
          else
          { // A postion is being requested (not yet in the gcp-list) ['id_gcp' is ignored]
            s_sql_transform_base = QString( "GCP_Transform(MakePoint(%1,%2,%3) , g_pixel.matrix,-1)" ).arg( input_point.x() ).arg( input_point.y() ).arg( i_gcp_coverage_srid );
          }
          s_sql_transform   = QString( "ST_X(%1) AS gcp_pixel_x,\n " ).arg( s_sql_transform_base );
          s_sql_transform += QString( "ST_Y(%1) AS gcp_pixel_y," ).arg( s_sql_transform_base );
          s_sql_transform += QString( "GCP_IsValid(g_pixel.matrix) AS valid_matrix" );
          if ( b_reCompute )
          { // If there is no TABLE 'gcp_compute' OR requested directly, then calculate directly from the POINTs
            s_sql_compute = QString( "(SELECT GCP_Compute(gcp_point,gcp_pixel,%1) AS matrix FROM  \"%2\"  WHERE %3) AS g_pixel" ).arg( i_order ).arg( s_gcp_points_table_name ).arg( s_recompute_where );
          }
          else
          { // The stored 'Affine Transformation Matrix' in TABLE 'gcp_compute' will be read [should be swifter than recalulating]
            s_sql_compute = QString( "(SELECT gcp_compute AS matrix FROM \"%1\"  WHERE ((coverage_name='%1') AND (srid=%3) AND (id_order=%4))) AS g_pixel" ).arg( "gcp_compute" ).arg( s_coverage_name ).arg( -1 ).arg( i_order ).arg( s_gcp_points_table_name );
          }
        }
        else
        { // Pixel-Point to Map-Point
          s_transform_type += s_pixel_point_to_map_point;
          if ( id_gcp >= 0 )
          { // A specific gcp is being requested ['input_point' is ignored]
            s_sql_transform_base = QString( "GCP_Transform(gcp.gcp_pixel , g_point.matrix,-1)" );
          }
          else
          { // A postion is being requested (not yet in the gcp-list) ['id_gcp' is ignored]
            s_sql_transform_base = QString( "GCP_Transform(MakePoint(%1,%2,-1) , g_point.matrix,%3)" ).arg( input_point.x() ).arg( input_point.y() ).arg( i_gcp_coverage_srid );
          }
          s_sql_transform   = QString( "ST_X(%1) AS gcp_map_x,\n " ).arg( s_sql_transform_base );
          s_sql_transform += QString( "ST_Y(%1) AS gcp_map_y," ).arg( s_sql_transform_base );
          // GCP_IsValid(g_pixel.matrix) AS valid_matrix
          s_sql_transform += QString( "GCP_IsValid(g_point.matrix) AS valid_matrix" );
          if ( b_reCompute )
          { // If there is no TABLE 'gcp_compute' OR requested directly, then calculate directly from the POINTs
            QString s_where_condition = " (gcp_enable=1)";
            s_sql_compute = QString( "(SELECT GCP_Compute(gcp_pixel,gcp_point,%1) AS matrix FROM  \"%2\"  WHERE %3) AS g_point" ).arg( i_order ).arg( s_gcp_points_table_name ).arg( s_recompute_where );
          }
          else
          { // The stored 'Affine Transformation Matrix' in TABLE 'gcp_compute' will be read [should be swifter than recalulating]
            s_sql_compute = QString( "(SELECT gcp_compute AS matrix FROM \"%1\"  WHERE ((coverage_name='%1') AND (srid=%3) AND (id_order=%4))) AS g_point" ).arg( "gcp_compute" ).arg( s_coverage_name ).arg( i_gcp_coverage_srid ).arg( i_order ).arg( s_gcp_points_table_name );
          }
        }
        s_sql = QString( s_sql_base ).replace( "TRANSFORM", s_sql_transform ).replace( "COMPUTE", s_sql_compute );
        ret = sqlite3_prepare_v2( db_handle, s_sql.toUtf8().constData(), -1, &stmt, NULL );
        if ( ret != SQLITE_OK )
        { //  typically: too few GCP pairs were passed, or the GCP pairs are of a so poor quality that finding any valid solution was impossible
          parms_GcpDbData->mError = QString( "Unable to execute %1 for '%2': rc=%3 [%4] sql[%5]\n" ).arg( s_transform_type ).arg( s_gcp_points_table_name ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql );
          qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::getGcpConvert - GCP_Transform/Compute - [%1] " ).arg( parms_GcpDbData->mError );
          // -- src/control_points/gaia_control_points.c
          // Poorly placed control points. - Can not generate the transformation equation.
        }
        int i_valid_matrix = -1;
        while ( sqlite3_step( stmt ) == SQLITE_ROW )
        {
          if ( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL )
            point_x = sqlite3_column_double( stmt, 0 );
          if ( sqlite3_column_type( stmt, 1 ) != SQLITE_NULL )
            point_y = sqlite3_column_double( stmt, 1 );
          if ( sqlite3_column_type( stmt, 2 ) != SQLITE_NULL )
            i_valid_matrix = sqlite3_column_double( stmt, 2 );
        }
        if ( i_valid_matrix > 0 )
        {
          convert_point.setX( point_x );
          convert_point.setY( point_y );
        }
        // qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::getGcpConvert valid_matrix[%1] - GCP_Transform/Compute - point[%3,%4] sql[%2] " ).arg( i_valid_matrix ).arg( s_sql ).arg( point_x ).arg( point_y );
      }
    }
    else
    {
      parms_GcpDbData->mError = QString( "QgsSpatiaLiteProviderGcpUtils::getGcpConvert: Gcp not enabled: rc=%1 database[%2] filename[%3]" ).arg( 2 ).arg( b_database_exists ).arg( s_database_filename );
    }
    //----------------
    QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
  }
  else
  {
    parms_GcpDbData->mError = QString( "QgsSpatiaLiteProviderGcpUtils::getGcpConvert: Database error: rc=%1 database[%2] filename[%3]" ).arg( 1 ).arg( b_database_exists ).arg( s_database_filename );
  }
  return convert_point;
}
bool QgsSpatiaLiteProviderGcpUtils::createGcpMasterDb( GcpDbData* parms_GcpDbData )
{
  if ( !parms_GcpDbData )
    return false;
  parms_GcpDbData->mDatabaseValid = false;
  QString  s_database_filename = parms_GcpDbData->mGcpDatabaseFileName;
  QString s_gcp_master = parms_GcpDbData->mGcp_coverage_name.toLower();
  bool b_create_empty_database = false;
  bool b_database_exists = QFile( s_database_filename ).exists();
  if (( s_gcp_master.isEmpty() ) || ( s_gcp_master.isNull() ) )
  {
    s_gcp_master = "gcp_master";
    if ( !b_database_exists )
      b_create_empty_database = true;
  }
  int i_srid = parms_GcpDbData->mGcpSrid;
  int i_gcp_enabled = parms_GcpDbData->mGcp_enabled;
  int i_gcp_count = 0;
  bool b_gcp_master = false;
  bool b_spatial_ref_sys_create;
  int i_sql_counter = 1;
  QString s_sql_newline = QString( "---------------" );
  QString s_sql_output = QString::null;
  if (( !b_create_empty_database ) && ( !b_database_exists ) && ( parms_GcpDbData->mDatabaseDump ) )
  { // Cannot dump a Database that does not exist
    parms_GcpDbData->mError = QString( "Failed to create database [file exists]: rc=%1 database[%2]" ).arg( 1 ).arg( s_database_filename );
    return b_database_exists;
  }
  // base on code found in 'QgsOSMDatabase::createSpatialTable'
  sqlite3* db_handle;
  char *errMsg = nullptr;
  // open database
  int ret = QgsSpatiaLiteProviderGcpUtils::spatialiteInitEx( parms_GcpDbData, s_database_filename );
  if ( ret != SQLITE_OK )
  {
    parms_GcpDbData->mError = QString( "Failed to open database: rc=%1 database[%2]" ).arg( ret ).arg( s_database_filename );
    if ( ret != SQLITE_MISUSE )
    { // 'SQLITE_MISUSE': in use with another file, do not close
      QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
    }
    return false;
  }
  // To simplfy the useage.
  db_handle = parms_GcpDbData->db_handle;
  sqlite3_stmt* stmt;
  // Start TRANSACTION
  ret = sqlite3_exec( db_handle, "BEGIN", NULL, NULL, 0 );
  Q_ASSERT( ret == SQLITE_OK );
  Q_UNUSED( ret );
  // SELECT HasGCP(), (SELECT count(name) FROM sqlite_master WHERE ((type='table') AND (name='gcp_coverages'))) AS has_gcp_coverages;
  QString s_sql = QString( "SELECT HasGCP(), (SELECT count(name) FROM sqlite_master WHERE ((type='table') AND (name='%1'))) AS has_gcp_master," ).arg( s_gcp_master );
  s_sql += QString( " (SELECT count(name) FROM sqlite_master WHERE ((type='table') AND (name='spatial_ref_sys'))) AS has_spatial_ref_sys" );
  ret = sqlite3_prepare_v2( db_handle, s_sql.toUtf8().constData(), -1, &stmt, NULL );
  if ( ret != SQLITE_OK )
  { //  rc=1 [no such function: HasGCP] is not an error [older spatialite version]
    if ( QString( "no such function: HasGCP" ).compare( QString( "%1" ).arg( sqlite3_errmsg( db_handle ) ) ) != 0 )
    {
      parms_GcpDbData->mError = QString( "Unable to SELECT HasGCP(): rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql );
      qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpMasterDb -%1- [%2] " ).arg( i_sql_counter ).arg( parms_GcpDbData->mError );
    }
  }
  i_sql_counter++;
  int i_gcp_master = -1;
  int i_spatial_ref_sys_create = -1;
  while ( sqlite3_step( stmt ) == SQLITE_ROW )
  {
    if ( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL )
      i_gcp_enabled = sqlite3_column_int( stmt, 0 );
    if ( sqlite3_column_type( stmt, 1 ) != SQLITE_NULL )
      i_gcp_master = sqlite3_column_int( stmt, 1 );
    if ( sqlite3_column_type( stmt, 2 ) != SQLITE_NULL )
      i_spatial_ref_sys_create = sqlite3_column_int( stmt, 2 );
  }
  sqlite3_finalize( stmt );
  if ( i_gcp_master > 0 )
    b_gcp_master = true;
  if ( i_spatial_ref_sys_create > 0 )
    b_spatial_ref_sys_create = true;
  if ( !b_spatial_ref_sys_create )
  { // no spatial_ref_sys, call InitSpatialMetadata
    s_sql = QString( "SELECT InitSpatialMetadata(0)" );
    ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
    if ( ret != SQLITE_OK )
    {
      parms_GcpDbData->mError = QString( "Unable to InitSpatialMetadata: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
      qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpMasterDb -%1- [%2] " ).arg( i_sql_counter ).arg( parms_GcpDbData->mError );
      sqlite3_free( errMsg );
      // End TRANSACTION
      ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
      Q_ASSERT( ret == SQLITE_OK );
      Q_UNUSED( ret );
      QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
      return false;
    }
    b_spatial_ref_sys_create = true;
  }
  i_sql_counter++;
  b_database_exists = QFile( s_database_filename ).exists();
  if ( b_database_exists )
  {
    if ( !b_gcp_master )
    {
      QStringList sa_tables; // Format: CREATE_MASTER;#;#gcp_master;#;#i_srid
      sa_tables.append( QString( "%2%1%3%1%4" ).arg( parms_GcpDbData->mParseString ).arg( "CREATE_MASTER" ).arg( s_gcp_master ).arg( i_srid ) );
      if ( sa_tables.size() > 0 )
      {
        QStringList sa_sql_commands = QgsSpatiaLiteProviderGcpUtils::createGcpSqlMasterCommands( parms_GcpDbData, sa_tables );
        if ( sa_sql_commands.size() > 0 )
        {
          for ( int i_list = 0;i_list < sa_sql_commands.size();i_list++ )
          {
            s_sql = sa_sql_commands.at( i_list );
            if ( parms_GcpDbData->mSqlDump )
            {
              s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
            }
            ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
            if ( ret != SQLITE_OK )
            {
              parms_GcpDbData->mError = QString( "Sql failed %1: rc=%2 [%3] sql[%4]\n" ).arg( s_gcp_master ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql );
              qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpMasterDb -%1- [%2] " ).arg( i_sql_counter ).arg( parms_GcpDbData->mError );
              sqlite3_free( errMsg );
              // End TRANSACTION
              ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
              Q_ASSERT( ret == SQLITE_OK );
              Q_UNUSED( ret );
              QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
              return false;
            }
            i_sql_counter++;
          }
          sa_sql_commands.clear();
        }
      }
      sa_tables.clear();
    }
    // End TRANSACTION
    ret = sqlite3_exec( db_handle, "COMMIT", NULL, NULL, 0 );
    Q_ASSERT( ret == SQLITE_OK );
    Q_UNUSED( ret );
    s_sql = QString( "SELECT (SELECT srid FROM geometry_columns WHERE (f_table_name='%1')) AS srid_gcp_master," ).arg( s_gcp_master );
    s_sql += QString( " (SELECT count(id_gcp) FROM '%1') AS count_gcp_master" ).arg( s_gcp_master );
    ret = sqlite3_prepare_v2( db_handle, s_sql.toUtf8().constData(), -1, &stmt, NULL );
    if ( ret != SQLITE_OK )
    { //  typically: too few GCP pairs were passed, or the GCP pairs are of a so poor quality that finding any valid solution was impossible
      parms_GcpDbData->mError = QString( "Unable to retrieve %1 for '%2': rc=%3 [%4] sql[%5]\n" ).arg( "srid" ).arg( s_gcp_master ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql );
      qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpMasterDb -%1- [%2] " ).arg( i_sql_counter ).arg( parms_GcpDbData->mError );
    }
    while ( sqlite3_step( stmt ) == SQLITE_ROW )
    {
      if ( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL )
      {
        i_srid = sqlite3_column_int( stmt, 0 );
        b_gcp_master = true;
      }
      if ( sqlite3_column_type( stmt, 1 ) != SQLITE_NULL )
      {
        i_gcp_count = sqlite3_column_int( stmt, 1 );
      }
    }
    sqlite3_finalize( stmt );
    i_sql_counter++;
  }
  if (( b_gcp_master ) && ( parms_GcpDbData->mDatabaseDump ) )
  { // Dump Database after creation, when requested
    // Prevent both DatabaseDump and SqlDump running
    s_sql_output = QString::null;
    parms_GcpDbData->mSqlDump = false;
    QStringList sa_tables; // Format: CREATE_MASTER;#;#gcp_master;#;#i_srid
    sa_tables.append( QString( "%2%1%3%1%4" ).arg( parms_GcpDbData->mParseString ).arg( "CREATE_MASTER" ).arg( s_gcp_master ).arg( i_srid ) );
    if ( sa_tables.size() > 0 )
    {
      QStringList sa_sql_commands = QgsSpatiaLiteProviderGcpUtils::createGcpSqlMasterCommands( parms_GcpDbData, sa_tables );
      if ( sa_sql_commands.size() > 0 )
      {
        for ( int i_list = 0;i_list < sa_sql_commands.size();i_list++ )
        {
          s_sql = sa_sql_commands.at( i_list );
          if ( s_sql.contains( QString( "SELECT AddGeometryColumn(" ) ) )
          {
            s_sql_output += QString( "%1\n" ).arg( s_sql );
          }
          else
          {
            if ( s_sql.contains( QString( "CREATE TRIGGER IF NOT EXISTS 'tb_ins" ) ) )
            {
              s_sql_output += QString( "-- adding TRIGGERs\n%1\n" ).arg( s_sql_newline );
            }
            if ( s_sql.contains( QString( "CREATE VIEW IF NOT EXISTS " ) ) )
            {
              s_sql_output += QString( "-- adding SpatialView, with INSERT into views_geometry_columns with needed TRIGGERs \n%1\n" ).arg( s_sql_newline );
            }
            s_sql_output += QString( "%1\n%2\n" ).arg( s_sql ).arg( s_sql_newline );
          }
        }
        sa_sql_commands.clear();
      }
      sa_tables.clear();
    }
    if ( i_gcp_count > 0 )
    { // Only if rows exist, otherwise message
      sa_tables.append( QString( "%2%1%3" ).arg( parms_GcpDbData->mParseString ).arg( "SELECT_MASTER" ).arg( s_gcp_master ) );
      if ( sa_tables.size() > 0 )
      { // Format: SELECT_MASTER;#;#gcp_master
        QStringList sa_sql_commands = QgsSpatiaLiteProviderGcpUtils::createGcpSqlMasterCommands( parms_GcpDbData, sa_tables );
        if ( sa_sql_commands.size() == 3 )
        { // 3 results: select_insert (with select_fields),select_values, UpdateLayerStatistics
          QString s_sql_select_insert = sa_sql_commands.at( 0 );
          s_sql = sa_sql_commands.at( 1 );
          int i_rows_counter = 0;
          int i_rows_compound = 0;
          int i_max_compound = 100; // Max 500 ? ['too many terms in compound SELECT' when running Sql-Script]
          QString s_value_insert;
          QString s_update_layerstatistics = sa_sql_commands.at( 2 );
          s_sql_output += QString( "-- adding %2 rows into '%3' \n%1\n" ).arg( s_sql_newline ).arg( i_gcp_count ).arg( s_gcp_master );
          ret = sqlite3_prepare_v2( db_handle, s_sql.toUtf8().constData(), -1, &stmt, NULL );
          if ( ret != SQLITE_OK )
          { //  rc=1 [no such table: vector_layers_statistics or gcp_points, 'no such column' for gcp_pixel and gcp_ppoint] is an error
            parms_GcpDbData->mError = QString( "Unable to retrieve gcp_master rows : rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( db_handle ) ) ).arg( s_sql );
            qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::createGcpMasterDb -gcp_master- [%1] " ).arg( parms_GcpDbData->mError );
            // End TRANSACTION
            ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
            Q_ASSERT( ret == SQLITE_OK );
            Q_UNUSED( ret );
            QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
            return false;
          }
          while ( sqlite3_step( stmt ) == SQLITE_ROW )
          {
            if ( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL )
            {
              s_sql = QString::fromUtf8(( const char * ) sqlite3_column_text( stmt, 0 ) );
              if ( i_rows_compound == 0 )
              { // start the INSERT Statement ... VALUES
                s_sql_output += QString( s_sql_select_insert ).arg( s_sql );
                i_rows_compound++;
              }
              else
              { // continue the INSERT Statement  ( ... )
                i_rows_compound++;
                s_value_insert = QString( ",\n %1" ).arg( s_sql );
                if ( i_rows_compound > i_max_compound )
                { // close the INSERT Statement
                  i_rows_compound = 0;
                  s_value_insert += QString( ";\n%1\n" ).arg( s_sql_newline );
                }
                s_sql_output += s_value_insert;
              }
              i_rows_counter++;
            }
          }
          sqlite3_finalize( stmt );
          if ( i_rows_counter > 0 )
          {
            QString s_message = QString( "SELECT DateTime('now'),'UpdateLayerStatistics[for ''%1'' (%2 rows) and all SpatialViews]';" ).arg( s_gcp_master ).arg( i_rows_counter );
            // close the INSERT Statement
            s_sql_output += QString( ";\n%2\n%3\n%1\n" ).arg( s_update_layerstatistics ).arg( s_sql_newline ).arg( s_message );
          }
          sa_sql_commands.clear();
        }
        sa_tables.clear();
      }
    }
    else
    {
      QString s_message = QString( "SELECT DateTime('now'),'''%1'' no rows found.';" ).arg( s_gcp_master );
      s_sql_output += QString( "%2\n%1\n" ).arg( s_sql_newline ).arg( s_message );
    }
    if ( !s_sql_output.isNull() )
    { // writing of Sql-Dump
      QFile::FileError file_result = QgsSpatiaLiteProviderGcpUtils::writeGcpSqlDump( s_database_filename, s_sql_output, "sql" );
      if ( file_result != QFile::NoError )
      {
        b_gcp_master = false;
        parms_GcpDbData->mError = QString( "Failed to create Gcp-Master Sql-Dump [expected output of alt lease %1 bytes]: FileError=%1 database[%2]" ).arg( s_sql_output.length() ).arg( file_result ).arg( s_database_filename );
        s_sql_output = QString::null;
        return b_gcp_master;
      }
      s_sql_output = QString::null;
    }
    else
    { // Dump failed, set as error
      b_gcp_master = false;
      parms_GcpDbData->mError = QString( "Failed to create Gcp-Master Sql-Dump [file exists]: rc=%1 database[%2]" ).arg( 1 ).arg( s_database_filename );
      return b_gcp_master;
    }
  }
  QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
  if ( b_gcp_master )
  { // If we arrive here, a gcp_master TABLE exist and the used srid is known - thus valid
    parms_GcpDbData->mDatabaseValid = true;
    parms_GcpDbData->mGcpSrid = i_srid;
    parms_GcpDbData->mGcp_enabled = i_gcp_enabled;
    if ( parms_GcpDbData->mSqlDump )
    {
      if ( !s_sql_output.isNull() )
      { // writing of Sql-Dump
        QFile::FileError file_result = QgsSpatiaLiteProviderGcpUtils::writeGcpSqlDump( s_database_filename, s_sql_output, "sql" );
        if ( file_result != QFile::NoError )
        {
          b_gcp_master = false;
          parms_GcpDbData->mError = QString( "Failed to create Gcp-Master Sql-Dump [expected output of alt lease %1 bytes]: FileError=%1 database[%2]" ).arg( s_sql_output.length() ).arg( file_result ).arg( s_database_filename );
          s_sql_output = QString::null;
          return b_gcp_master;
        }
        s_sql_output = QString::null;
      }
      else
      { // Dump failed, set as error
        b_gcp_master = false;
        parms_GcpDbData->mError = QString( "Failed to create Gcp-Master Sql-Dump [file exists]: rc=%1 database[%2]" ).arg( 1 ).arg( s_database_filename );
        return b_gcp_master;
      }
    }
  }
  else
  {
    parms_GcpDbData->mError = QString( "Failed to create Gcp-Master -- reason unknown -- [file exists]: rc=%1 database[%2]" ).arg( 1 ).arg( s_database_filename );
    return b_gcp_master;
  }
  return b_database_exists;
}
QFile::FileError QgsSpatiaLiteProviderGcpUtils::writeGcpSqlDump( QString s_source_filename, QString s_sql_output, QString s_extention )
{
  QFile::FileError file_result = QFile::NoError;
  if ( !s_sql_output.isNull() )
  { // writing of Sql-Dump
    QFileInfo db_file( s_source_filename );
    QString s_db_name = db_file.fileName(); // file-name without path
    QString s_sql_name = db_file.fileName().replace( db_file.suffix(), s_extention );
    QString s_db_path = db_file.canonicalPath();
    if ( s_db_path.isEmpty() )
    { // File does not exist, use absolutePath [synlinks and relative paths may not be resolved]
      s_db_path = db_file.absolutePath();
    }
    QString s_sql_filename = QString( "%1/%2" ).arg( s_db_path ).arg( s_sql_name );
    QString s_command_text = QString( "rm %1 ; spatialite %1 < %2;" ).arg( s_db_name ).arg( s_sql_name );
    QFile sql_file( s_sql_filename );
    if ( sql_file.exists() )
    {
      sql_file.remove();
    }
    sql_file.open( QIODevice::WriteOnly );
    file_result = sql_file.error();
    if ( file_result == QFile::NoError )
    {
      QString s_sql_newline = QString( "---------------" );
      QTextStream  sql_out( &sql_file );
      // The command to execute the sql-script
      sql_out << QString( "%1\n-- %2\n%1\n%3\n%1\nBEGIN;\n%1\n" ).arg( s_sql_newline ).arg( s_command_text ).arg( QString( "SELECT DateTime('now'),'%1 [start]';" ).arg( s_sql_name ) );
      sql_out << s_sql_output << QString( "%1\n%2\n%3\n%2" ).arg( "COMMIT;" ).arg( s_sql_newline ).arg( QString( "SELECT DateTime('now'),'%1 [finished]';" ).arg( s_sql_name ) );
      sql_out.flush();
      sql_file.close();
      file_result = sql_file.error();
      // qDebug() << QString( "-I-> QgsSpatiaLiteProviderGcpUtils::QgsSpatiaLiteProviderGcpUtils::writeGcpSqlDump writing sql-script sql_file[%1] error[%2]" ).arg( s_sql_filename ).arg( sql_file.error() );
    }
    else
    { // QFile::OpenError [5] : Directory contained a '.db', thus was also replaced [Directory did not exist]
    }
  }
  return file_result;
}
QStringList QgsSpatiaLiteProviderGcpUtils::createGcpSqlPointsCommands( GcpDbData* parms_GcpDbData, QStringList sa_tables )
{ // Format: id_gcp_coverage;#;#i_srid;#;#s_coverage_name
  QStringList sa_sql_commands;
  if ( !parms_GcpDbData )
  {
    return sa_sql_commands;
  }
  for ( int i_list = 0;i_list < sa_tables.size();i_list++ )
  { // raster entry has been added to gcp_covarage, create corresponding tables
    QStringList sa_fields = sa_tables.at( i_list ).split( parms_GcpDbData->mParseString );
    QString s_coverage_name = QString::null;
    QString s_points_table_name = QString::null;
    int i_srid = parms_GcpDbData->mGcpSrid;
    int id_gcp_coverage = -1;
    qDebug() << QString( "createGcpSqlPointsCommands: srid[%3] fields_count[%1] line[%2]" ).arg( sa_fields.count() ).arg( sa_tables.at( i_list ) ).arg( i_srid );
    if ( sa_fields.count() == 3 )
    {
      id_gcp_coverage = sa_fields.at( 0 ).toInt();
      i_srid = sa_fields.at( 1 ).toInt();
      s_coverage_name = sa_fields.at( 2 );
      s_points_table_name = QString( "%1_%2" ).arg( "gcp_points" ).arg( s_coverage_name );
    }
    else
    { // Entry is invalid, skip
      continue;
    }
    if ( !s_points_table_name.isNull() )
    {
      QString s_sql = QString( "CREATE TABLE IF NOT EXISTS '%1'\n(\n" ).arg( s_points_table_name );
      s_sql += QString( " %1 INTEGER PRIMARY KEY AUTOINCREMENT,\n" ).arg( "id_gcp" );
      s_sql += QString( " %1 INTEGER DEFAULT %2,\n" ).arg( "id_gcp_coverage" ).arg( id_gcp_coverage );
      s_sql += QString( " %1 TEXT DEFAULT '%2',\n" ).arg( "name" ).arg( s_coverage_name );
      s_sql += QString( " %1 TEXT DEFAULT '',\n" ).arg( "notes" );
      s_sql += QString( " %1 DOUBLE DEFAULT 0,\n" ).arg( "pixel_x" );
      s_sql += QString( " %1 DOUBLE DEFAULT 0,\n" ).arg( "pixel_y" );
      s_sql += QString( " %1 INTEGER DEFAULT %2,\n" ).arg( "srid" ).arg( i_srid );
      s_sql += QString( " %1 DOUBLE DEFAULT 0,\n" ).arg( "point_x" );
      s_sql += QString( " %1 DOUBLE DEFAULT 0,\n" ).arg( "point_y" );
      s_sql += QString( " -- 0=tpc (Thin Plate Spline)\n" );
      s_sql += QString( " -- 1=1st order(affine) [precise]\n" );
      s_sql += QString( " -- 2=2nd order [less exact]\n" );
      s_sql += QString( " -- 3=3rd order [poor]\n" );
      s_sql += QString( " %1 INTEGER DEFAULT 0,\n" ).arg( "order_selected" );
      s_sql += QString( " %1 INTEGER DEFAULT 1,\n" ).arg( "gcp_enable" );
      s_sql += QString( " %1 TEXT DEFAULT ''\n);" ).arg( "gcp_text" );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "SELECT AddGeometryColumn('%1','gcp_pixel',-1,'POINT','XY');" ).arg( s_points_table_name );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "SELECT CreateSpatialIndex('%1','gcp_pixel');" ).arg( s_points_table_name );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "SELECT AddGeometryColumn('%1','gcp_point',%2,'POINT','XY');" ).arg( s_points_table_name ).arg( i_srid );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "SELECT CreateSpatialIndex('%1','gcp_point');" ).arg( s_points_table_name );
      sa_sql_commands.append( s_sql );
      // If while INSERTING, no geometries are supplied (only valid pixel/point x+y values), the geometries will be created
      s_sql = QString( "CREATE TRIGGER IF NOT EXISTS 'tb_ins_%1'\n" ).arg( s_points_table_name );
      s_sql += QString( " AFTER INSERT ON  \"%1\"\nBEGIN\n" ).arg( s_points_table_name );
      s_sql += QString( " UPDATE \"%1\" \n" ).arg( s_points_table_name );
      s_sql += QString( " -- general checking for and setting of NULL values done in 'tb_upd_gcp_points'\n SET\n" );
      s_sql += QString( "  name =  NEW.name, notes = NEW.notes,\n" );
      s_sql += QString( "  pixel_x = NEW.pixel_x, pixel_y = NEW.pixel_y,\n" );
      s_sql += QString( "  srid = NEW.srid,\n" );
      s_sql += QString( "  point_x = NEW.point_x, point_y = NEW.point_y,\n" );
      s_sql += QString( "  order_selected = NEW.order_selected,\n" );
      s_sql += QString( "  gcp_enable = NEW.gcp_enable,\n" );
      s_sql += QString( "  gcp_text = NEW.gcp_text,\n" );
      s_sql += QString( "  gcp_pixel = CASE WHEN NEW.gcp_pixel IS NULL THEN MakePoint(NEW.pixel_x,NEW.pixel_y,-1) ELSE NEW.gcp_pixel END,\n" );
      s_sql += QString( "  gcp_point = CASE WHEN NEW.gcp_point IS NULL THEN MakePoint(NEW.point_x,NEW.point_y,NEW.srid) ELSE NEW.gcp_point END\n" );
      s_sql += QString( " WHERE id_gcp = NEW.id_gcp;\nEND;" );
      sa_sql_commands.append( s_sql );
      // AFTER (not BEFORE) an UPDATE of the geometries,  the pixel/point x+y values will be updated
      s_sql = QString( "CREATE TRIGGER IF NOT EXISTS 'tb_upd_%1'\n" ).arg( s_points_table_name );
      s_sql += QString( " AFTER UPDATE OF\n" );
      s_sql += QString( "  id_gcp,name,notes,pixel_x,pixel_y,srid,point_x,point_y,order_selected,gcp_enable,gcp_text,gcp_pixel,gcp_point\n" );
      s_sql += QString( " ON \"%1\" \nBEGIN\n" ).arg( s_points_table_name );
      s_sql += QString( " UPDATE \"%1\" \n" ).arg( s_points_table_name );
      s_sql += QString( " -- general checking for and setting of NULL values\n SET\n" );
      s_sql += QString( "  name =  CASE WHEN NEW.name IS NULL THEN '' ELSE NEW.name END,\n" );
      s_sql += QString( "  notes = CASE WHEN NEW.notes IS NULL THEN '' ELSE NEW.notes END,\n" );
      s_sql += QString( "  pixel_x = ST_X(NEW.gcp_pixel),\n" );
      s_sql += QString( "  pixel_y = ST_Y(NEW.gcp_pixel),\n" );
      s_sql += QString( "  srid = ST_SRID(NEW.gcp_point),\n" );
      s_sql += QString( "  point_x = ST_X(NEW.gcp_point),\n" );
      s_sql += QString( "  point_y = ST_Y(NEW.gcp_point),\n" );
      s_sql += QString( "  order_selected = CASE WHEN NEW.order_selected IS NULL THEN 0 ELSE NEW.order_selected END,\n" );
      s_sql += QString( "  gcp_enable = CASE WHEN NEW.gcp_enable IS NULL THEN 1 ELSE NEW.gcp_enable END,\n" );
      s_sql += QString( "  gcp_text = CASE WHEN ((NEW.gcp_text = NULL) OR (NEW.gcp_text = '')) THEN CAST(NEW.id_gcp AS TEXT) ELSE NEW.gcp_text END,\n" );
      s_sql += QString( "  gcp_pixel = NEW.gcp_pixel,\n" );
      s_sql += QString( "  gcp_point = NEW.gcp_point\n" );
      s_sql += QString( " WHERE id_gcp = OLD.id_gcp;\nEND;" );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "CREATE VIEW IF NOT EXISTS '%1_%2' AS\n" ).arg( "list_gcp" ).arg( s_coverage_name );
      s_sql += QString( " -- list gcp''s as gdal_gcp and qgis-points\n SELECT id_gcp,\n " );
      s_sql += QString( "  '-gcp '||pixel_x||' '||-(pixel_y)||' '||point_x||' '||point_y AS gcp_gdal,\n" );
      s_sql += QString( "  point_x||','||point_y||','||pixel_x||','||pixel_y||','||gcp_enable AS point_qgis\n" );
      s_sql += QString( " FROM \"%1\"\n ORDER BY point_x,point_y;" ).arg( s_points_table_name );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "CREATE VIEW IF NOT EXISTS '%1_%2' AS\n" ).arg( "list_gcp_disabeld" ).arg( s_coverage_name );
      s_sql += QString( " -- gcp is disabled\n SELECT id_gcp,\n " );
      s_sql += QString( "  '-gcp '||pixel_x||' '||-(pixel_y)||' '||point_x||' '||point_y AS gcp_gdal,\n" );
      s_sql += QString( "  point_x||','||point_y||','||pixel_x||','||pixel_y||','||gcp_enable AS point_qgis\n" );
      s_sql += QString( " FROM \"%1\"\n %2\n ORDER BY point_x,point_y;" ).arg( s_points_table_name ).arg( "WHERE (gcp_enable=0)" );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "CREATE VIEW IF NOT EXISTS '%1_%2' AS\n" ).arg( "list_gcp_enabled" ).arg( s_coverage_name );
      s_sql += QString( " -- gcp is enabled\n SELECT id_gcp,\n " );
      s_sql += QString( "  '-gcp '||pixel_x||' '||-(pixel_y)||' '||point_x||' '||point_y AS gcp_gdal,\n" );
      s_sql += QString( "  point_x||','||point_y||','||pixel_x||','||pixel_y||','||gcp_enable AS point_qgis\n" );
      s_sql += QString( " FROM \"%1\"\n %2\n ORDER BY point_x,point_y;" ).arg( s_points_table_name ).arg( "WHERE (gcp_enable=1)" );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "CREATE VIEW IF NOT EXISTS '%1_%2' AS\n" ).arg( "list_gcp_enabled_order_0" ).arg( s_coverage_name );
      s_sql += QString( " -- 0=tpc (Thin Plate Spline)\n SELECT id_gcp,\n" );
      s_sql += QString( "  '-gcp '||pixel_x||' '||-(pixel_y)||' '||point_x||' '||point_y AS gcp_gdal,\n" );
      s_sql += QString( "  point_x||','||point_y||','||pixel_x||','||pixel_y||','||gcp_enable AS point_qgis" );
      s_sql += QString( " FROM \"%1\"\n %3\n ORDER BY point_x,point_y;" ).arg( s_points_table_name ).arg( "WHERE ((gcp_enable=1) AND (order_selected=0))" );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "CREATE VIEW IF NOT EXISTS '%1_%2' AS\n" ).arg( "list_gcp_enabled_order_1" ).arg( s_coverage_name );
      s_sql += QString( " -- 1=1st order(affine) [precise]\n SELECT id_gcp,\n" );
      s_sql += QString( "  '-gcp '||pixel_x||' '||-(pixel_y)||' '||point_x||' '||point_y AS gcp_gdal,\n" );
      s_sql += QString( "  point_x||','||point_y||','||pixel_x||','||pixel_y||','||gcp_enable AS point_qgis\n" );
      s_sql += QString( " FROM \"%1\"\n %3\n ORDER BY point_x,point_y;" ).arg( s_points_table_name ).arg( "WHERE ((gcp_enable=1) AND (order_selected=1))" );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "CREATE VIEW IF NOT EXISTS '%1_%2' AS\n" ).arg( "list_gcp_enabled_order_2" ).arg( s_coverage_name );
      s_sql += QString( " -- 2=2nd order [less exact]\n SELECT id_gcp,\n" );
      s_sql += QString( "  '-gcp '||pixel_x||' '||-(pixel_y)||' '||point_x||' '||point_y AS gcp_gdal,\n" );
      s_sql += QString( "  point_x||','||point_y||','||pixel_x||','||pixel_y||','||gcp_enable AS point_qgis\n" );
      s_sql += QString( " FROM \"%1\"\n %2\n ORDER BY point_x,point_y;" ).arg( s_points_table_name ).arg( "WHERE ((gcp_enable=1) AND (order_selected=2))" );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "CREATE VIEW IF NOT EXISTS '%1_%2' AS\n" ).arg( "list_gcp_enabled_order_3" ).arg( s_coverage_name );
      s_sql += QString( " -- 3=3rd order [poor]\n SELECT id_gcp,\n" );
      s_sql += QString( "  '-gcp '||pixel_x||' '||-(pixel_y)||' '||point_x||' '||point_y AS gcp_gdal,\n" );
      s_sql += QString( "  point_x||','||point_y||','||pixel_x||','||pixel_y||','||gcp_enable AS point_qgis\n" );
      s_sql += QString( " FROM \"%1\"\n %2\n ORDER BY point_x,point_y;" ).arg( s_points_table_name ).arg( "WHERE ((gcp_enable=1) AND (order_selected=3))" );
      sa_sql_commands.append( s_sql );
    }
  }
  return sa_sql_commands;
}
QStringList QgsSpatiaLiteProviderGcpUtils::createGcpSqlCoveragesCommands( GcpDbData* parms_GcpDbData, QStringList sa_tables )
{ // Format: id_gcp_coverage;#;#i_srid;#;#s_coverage_name
  QStringList sa_sql_commands;
  if ( !parms_GcpDbData )
  {
    return sa_sql_commands;
  }
  for ( int i_list = 0;i_list < sa_tables.size();i_list++ )
  { // raster entry has been added to gcp_covarage, create corresponding tables
    QStringList sa_fields = sa_tables.at( i_list ).split( parms_GcpDbData->mParseString );
    QString s_sql = QString::null;
    QString s_command_type = "";
    QString s_coverage_values = QString::null;
    QString s_coverage_name = QString::null;
    QString s_points_table_name = "";
    int i_gcp_coverage_id = -1;
    int i_srid = parms_GcpDbData->mGcpSrid;
    int i_image_max_x = -1;
    int i_image_max_y = -1;
    bool b_toPixel = parms_GcpDbData->mToPixel;
    int i_order = parms_GcpDbData->mOrder;
    if (( i_order < 0 ) || ( i_order > 3 ) )
    {
      i_order = 0;
    }
    QString s_pixel_point_to_map_point = "Pixel-Point to Map-Point";
    QString s_map_point_to_pixel_point = "Map-Point to Pixel-Point";
    QString s_update_table = QString::null;
    QString s_update_field = QString::null;
    QString s_source_field = QString::null;
    // qDebug() << QString( "createGcpSqlCoveragesCommands:srid[%3] fields_count[%1] line[%2]" ).arg( sa_fields.count() ).arg( sa_tables.at( i_list ) ).arg( i_srid );
    if ( sa_fields.count() > 1 )
    {
      s_command_type = sa_fields.at( 0 );
      if ( s_command_type.toUpper().startsWith( "INSERT_" ) )
      {
        if ( s_command_type.toUpper() ==  "INSERT_COMPUTE" )
        { //  "INSERT_COMPUTE;#;#1;#;#3068;#;#1909.straube_blatt_i_a.4000;#;#5629;#;#4472"
          if ( sa_fields.count() != 6 )
          {
            continue; // invalid input, avoid crash
          }
          i_gcp_coverage_id = sa_fields.at( 1 ).toInt();
          i_srid = sa_fields.at( 2 ).toInt();
          s_coverage_name = sa_fields.at( 3 );
          i_image_max_x = sa_fields.at( 4 ).toInt();
          i_image_max_y = sa_fields.at( 5 ).toInt();
        }
        else
        {
          s_coverage_values = sa_fields.at( 1 );
        }
      }
      if ( s_command_type.toUpper() ==  "UPDATE_COMPUTE" )
      { //  "UPDATE_COMPUTE;#;#1;#;#3068;#;#1909.straube_blatt_i_a.4000;#;#5629;#;#4472"
        if ( sa_fields.count() != 6 )
        {
          continue; // invalid input, avoid crash
        }
        i_gcp_coverage_id = sa_fields.at( 1 ).toInt();
        i_srid = sa_fields.at( 2 ).toInt();
        s_coverage_name = sa_fields.at( 3 );
        s_points_table_name = QString( "%1_%2" ).arg( "gcp_points" ).arg( s_coverage_name );
        i_image_max_x = sa_fields.at( 4 ).toInt();
        i_image_max_y = sa_fields.at( 5 ).toInt();
        //qDebug() << QString( "-I-> UPDATE_COMPUTE[%1] points_table_name[%2] " ).arg( sa_tables.at( i_list ) ).arg(s_points_table_name);
      }
      if ( s_command_type.toUpper() ==  "UPDATE_COVERAGE_EXTENT" )
      { //  "UPDATE_COVERAGE_EXTENT;#;#1;;#;#1909.straube_blatt_i_a.4000"
        if ( sa_fields.count() != 3 )
        {
          continue; // invalid input, avoid crash
        }
        i_gcp_coverage_id = sa_fields.at( 1 ).toInt();
        s_coverage_name = sa_fields.at( 2 );
        s_points_table_name = QString( "%1_%2" ).arg( "gcp_points" ).arg( s_coverage_name );
        //qDebug() << QString( "-I-> UPDATE_COMPUTE[%1] points_table_name[%2] " ).arg( sa_tables.at( i_list ) ).arg(s_points_table_name);
      }
      if ( s_command_type.toUpper().startsWith( "CREATE_" ) )
      { // True for both 'COVERAGE' and 'CUTLINE'
        i_srid = sa_fields.at( 1 ).toInt();
      }
      if ( s_command_type.toUpper().startsWith( "SELECT_COVERAGES" ) )
      { // id_gcp_coverage and All fields, if coverage_name is empty, all coverages, othewise with WHERE
        if ( !sa_fields.at( 1 ).isEmpty() )
        {
          s_coverage_name = sa_fields.at( 1 );
        }
      }
      if ( s_command_type.toUpper().startsWith( "TRANSFORM_COMPUTE" ) )
      {
        if ( sa_fields.count() != 6 )
        { // "TRANSFORM_COMPUTE;#;#create_mercator_polygons;#;#mercator_polygon;#;#cutline_polygon#;#i_gcp_coverage_id#;#i_gcp_coverage_srid"
          continue; // invalid input, avoid crash
        }
        s_update_table = sa_fields.at( 1 );
        s_source_field = sa_fields.at( 2 );
        s_update_field = sa_fields.at( 3 );
        i_gcp_coverage_id = sa_fields.at( 4 ).toInt();
        i_srid = sa_fields.at( 5 ).toInt();
        if ( s_update_table == "create_mercator_polygons" )
        {
          if (( s_source_field == "mercator_polygon" ) && ( s_update_field == "cutline_polygon" ) )
          {
            b_toPixel = false;
          }
          if (( s_update_field == "mercator_polygon" ) && ( s_source_field == "cutline_polygon" ) )
          {
            b_toPixel = true;
          }
        }
      }
    }
    else
    { // Entry is invalid, skip
      continue;
    }
    if ( s_command_type.toUpper() == "CREATE_COVERAGE" )
    {
      s_sql = QString( "CREATE TABLE IF NOT EXISTS gcp_coverages\n( --collection of maps for georeferencing\n" );
      s_sql += QString( " %1 INTEGER PRIMARY KEY AUTOINCREMENT,\n" ).arg( "id_gcp_coverage" );
      s_sql += QString( " %1 TEXT DEFAULT '',\n" ).arg( "coverage_name" );
      s_sql += QString( " %1 DATE DEFAULT '0001-01-01',\n" ).arg( "map_date" );
      s_sql += QString( " %1 TEXT DEFAULT '',\n" ).arg( "raster_file" );
      s_sql += QString( " %1 TEXT DEFAULT '',\n" ).arg( "raster_gtif" );
      s_sql += QString( " %1 TEXT DEFAULT '',\n" ).arg( "title" );
      s_sql += QString( " %1 TEXT DEFAULT '',\n" ).arg( "abstract" );
      s_sql += QString( " %1 TEXT DEFAULT '',\n" ).arg( "copyright" );
      s_sql += QString( " %1 INTEGER DEFAULT 0,\n -- Gdal Nodata-Value\n" ).arg( "scale" );
      s_sql += QString( " %1 INTEGER DEFAULT -1,\n" ).arg( "nodata" );
      s_sql += QString( " %1 INTEGER DEFAULT %2,\n -- create_cutline_polygons.cutline_type=77\n -- AND same id_gcp_coverage\n" ).arg( "srid" ).arg( i_srid );
      s_sql += QString( " %1 INTEGER DEFAULT %2,\n" ).arg( "id_cutline" ).arg( -1 );
      s_sql += QString( " %1 INTEGER DEFAULT 0,\n -- ThinPlateSpline\n" ).arg( "gcp_count" );
      s_sql += QString( " %1 INTEGER DEFAULT 0,\n -- GRA_Lanczos\n" ).arg( "transformtype" );
      s_sql += QString( " %1 TEXT DEFAULT 'Lanczos', \n" ).arg( "resampling" );
      s_sql += QString( " %1 TEXT DEFAULT 'DEFLATE',\n -- Image Size X/Width\n" ).arg( "compression" );
      s_sql += QString( " %1 INTEGER DEFAULT 0,\n -- Image Size Y/Height\n" ).arg( "image_max_x" );
      s_sql += QString( " %1 INTEGER DEFAULT 0,\n" ).arg( "image_max_y" );
      s_sql += QString( " %1 DOUBLE DEFAULT 0.0,\n" ).arg( "extent_minx" );
      s_sql += QString( " %1 DOUBLE DEFAULT 0.0,\n" ).arg( "extent_miny" );
      s_sql += QString( " %1 DOUBLE DEFAULT 0.0,\n" ).arg( "extent_maxx" );
      s_sql += QString( " %1 DOUBLE DEFAULT 0.0,\n" ).arg( "extent_maxy" );
      s_sql += QString( " %1 TEXT DEFAULT ''\n);" ).arg( "raster_path" );
      sa_sql_commands.append( s_sql );
      if ( parms_GcpDbData->mGcp_enabled )
      {
        s_sql = QString( "CREATE TABLE IF NOT EXISTS gcp_compute\n( -- Spatialite GCP module\n" );
        s_sql += QString( " %1 INTEGER PRIMARY KEY AUTOINCREMENT,\n" ).arg( "id_gcp_compute" );
        s_sql += QString( " %1 INTEGER  DEFAULT 0,\n" ).arg( "id_gcp_coverage" );
        s_sql += QString( " %1 TEXT DEFAULT '',\n -- 'Pixel-Point to Map-Point - srid=-1' or\n -- 'Map-Point to Pixel-Point'  - srid<>-1\n" ).arg( "coverage_name" );
        s_sql += QString( " %1 TEXT DEFAULT 'Pixel-Point to Map-Point',\n -- id_order (0-3) used with GCP_Compute\n" ).arg( "notes" );
        s_sql += QString( " %1 INTEGER DEFAULT 0,\n -- srid (-1=pixel or srid of Map-Point)\n" ).arg( "id_order" );
        s_sql += QString( " %1 INTEGER DEFAULT %2,\n -- Image Size X/Width\n" ).arg( "srid" ).arg( -1 );
        s_sql += QString( " %1 INTEGER DEFAULT 0,\n -- Image Size Y/Height\n" ).arg( "image_max_x" );
        s_sql += QString( " %1 INTEGER DEFAULT 0,\n -- GCP_Compute result from 'gcp_points'\n" ).arg( "image_max_y" );
        s_sql += QString( " %1 BLOB\n);" ).arg( "gcp_compute" );
        sa_sql_commands.append( s_sql );
      }
    }
    if ( s_command_type.toUpper() == "UPDATE_COVERAGE_EXTENT" )
    {
      int i_cutline_type = 77;
      // search id_cutline from create_cutline_polygons, where cutline_type=77 with same id_gcp_coverage OR return -1
      // - if more than 1: then the latest 'max(id_cutline)'
      QString s_cutline_type_77 = QString( "SELECT\n    max(id_cutline)\n   FROM\n    '%1'\n   WHERE\n   ((id_gcp_coverage=%2) AND (cutline_type=%3))" ).arg( "create_cutline_polygons" ).arg( i_gcp_coverage_id ).arg( i_cutline_type );
      QString s_result_id = QString( " SELECT\n  (\n   %1\n  ) AS result_id" ).arg( s_cutline_type_77 );
      QString s_result_cutline = QString( " FROM\n (\n %1\n ) AS result_cutline" ).arg( s_result_id );
      QString s_result_case = QString( "CASE WHEN result_cutline.result_id IS NULL OR result_cutline.result_id < 0 THEN -1 ELSE NEW.result_cutline.result_id END id_result" );
      QString s_query_id_cutline = QString( "\n(\n SELECT\n  %1\n%2\n)" ).arg( s_result_case ).arg( s_result_cutline );
      QString s_sql = QString( "UPDATE '%1' SET\n" ).arg( "gcp_coverages" );
      s_sql += QString( "%1" ); // Placeholder for other UPDATE fields, where needed [!!: MUST end with ',' !!]
      s_sql += QString( " id_cutline=%1," ).arg( s_query_id_cutline );
      s_sql += QString( " gcp_count=(SELECT count(id_gcp) FROM \"%1\" WHERE ((gcp_point IS NOT NULL) AND (gcp_enable <> 0)))," ).arg( s_points_table_name );
      s_sql += QString( " extent_minx=(SELECT min(ST_X(gcp_point)) FROM \"%1\" WHERE ((gcp_point IS NOT NULL) AND (gcp_enable <> 0)))," ).arg( s_points_table_name );
      s_sql += QString( " extent_miny=(SELECT min(ST_Y(gcp_point)) FROM \"%1\" WHERE ((gcp_point IS NOT NULL) AND (gcp_enable <> 0)))," ).arg( s_points_table_name );
      s_sql += QString( " extent_maxx=(SELECT max(ST_X(gcp_point)) FROM \"%1\" WHERE ((gcp_point IS NOT NULL) AND (gcp_enable <> 0)))," ).arg( s_points_table_name );
      s_sql += QString( " extent_maxy=(SELECT max(ST_Y(gcp_point)) FROM \"%1\" WHERE ((gcp_point IS NOT NULL) AND (gcp_enable <> 0)))," ).arg( s_points_table_name );
      // srid is sometimes getting overwritten with '1'
      s_sql += QString( " srid=(SELECT srid FROM \"%1\" WHERE ((f_table_name = '%2') AND (f_geometry_column = 'gcp_point')))" ).arg( "geometry_columns" ).arg( s_points_table_name );
      s_sql += QString( " WHERE (coverage_name='%1')" ).arg( s_coverage_name );
      sa_sql_commands.append( s_sql );
      // correct possible NULL values for extents, where points TABLE is empty [gcp_count < 1 or no enabled]
      s_sql = QString( "UPDATE '%1' SET\n" ).arg( "gcp_coverages" );
      s_sql += QString( " extent_minx=0,extent_miny=0,extent_maxx=0,extent_maxy=0\n" );
      s_sql += QString( "WHERE (extent_minx IS NULL);" );
      sa_sql_commands.append( s_sql );
    }
    if ( s_command_type.toUpper() == "SELECT_COVERAGES" )
    { // 2 fields: id_gcp_coverage and all Fields (including id_gcp_coverage)
      // s_sql += QString( "SELECT_COVERAGES%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "" );
      s_sql = QString( "SELECT\n id_gcp_coverage," );
      s_sql += QString( "%1" ).arg( "id_gcp_coverage" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "coverage_name" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "map_date" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "raster_file" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "raster_gtif" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "title" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "abstract" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "copyright" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "scale" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "nodata" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "srid" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "id_cutline" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "gcp_count" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "transformtype" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "resampling" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "compression" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "image_max_x" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "image_max_y" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "extent_minx" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "extent_miny" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "extent_maxx" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "extent_maxy" );
      s_sql += QString( "||'%1'||%2" ).arg( parms_GcpDbData->mParseString ).arg( "raster_path" );
      s_sql += QString( "%1" ).arg( " AS coverage_value\n" );
      s_sql += QString( "FROM '%1'" ).arg( "gcp_coverages" );
      if ( !s_coverage_values.isNull() )
      {
        s_sql += QString( " WHERE (coverage_name='%1')" ).arg( s_coverage_name );
      }
      s_sql += QString( "\nORDER BY %1;" ).arg( "coverage_name" );
      // qDebug() << QString( "-I-> SELECT_COVERAGES sql[%1] " ).arg( s_sql);
      sa_sql_commands.append( s_sql );
    }
    if ( s_command_type.toUpper() == "CREATE_CUTLINES" )
    {
      // --- Cutline common sql for TABLE creation
      QString s_sql_cutline = QString( "CREATE TABLE IF NOT EXISTS 'create_cutline_%1'\n( --%2: to build %3 to assist the georeferencing\n" );
      s_sql_cutline += QString( " %1 INTEGER PRIMARY KEY AUTOINCREMENT,\n" ).arg( "id_cutline" );
      s_sql_cutline += QString( " %1 INTEGER DEFAULT 0,\n" ).arg( "id_gcp_coverage" );
      s_sql_cutline += QString( " %1 INTEGER DEFAULT 0,\n" ).arg( "cutline_type" );
      s_sql_cutline += QString( " %1 TEXT DEFAULT '',\n" ).arg( "name" );
      s_sql_cutline += QString( " %1 TEXT DEFAULT '',\n" ).arg( "notes" );
      s_sql_cutline += QString( " %1 TEXT DEFAULT '',\n" ).arg( "text" );
      s_sql_cutline += QString( " %1 TEXT DEFAULT '',\n" ).arg( "belongs_to_01" );
      s_sql_cutline += QString( " %1 TEXT DEFAULT '',\n" ).arg( "belongs_to_02" );
      s_sql_cutline += QString( " %1 DATE DEFAULT '0001-01-01',\n" ).arg( "valid_since" );
      s_sql_cutline += QString( " %1 DATE DEFAULT '3000-01-01'\n);" ).arg( "valid_until" );
      // --- Cutline POINTs to assist georeferencing
      s_sql = QString( s_sql_cutline ).arg( "points" ).arg( "POINT" ).arg( "POINTs" );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "SELECT AddGeometryColumn('create_cutline_points','cutline_point',%1,'POINT','XY');" ).arg( i_srid );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "SELECT CreateSpatialIndex('create_cutline_points','cutline_point');" );
      sa_sql_commands.append( s_sql );
      // --- Cutline LINESTRINGs to assist georeferencing
      s_sql = QString( s_sql_cutline ).arg( "linestrings" ).arg( "LINESTRING" ).arg( "LINESTRINGs" );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "SELECT AddGeometryColumn('create_cutline_linestrings','cutline_linestring',%1,'LINESTRING','XY');" ).arg( i_srid );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "SELECT CreateSpatialIndex('create_cutline_linestrings','cutline_linestring');" );
      sa_sql_commands.append( s_sql );
      // --- Cutline POLYGONs to assist georeferencing
      s_sql = QString( s_sql_cutline ).arg( "polygons" ).arg( "POLYGON" ).arg( "POLYGONs" );
      s_sql.replace( QString( " id_gcp_coverage INTEGER DEFAULT 0,\n" ), QString( " id_gcp_coverage INTEGER DEFAULT 0,\n -- cutline_type=77: use as gdal cutline for id_gcp_coverage\n" ) );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "SELECT AddGeometryColumn('create_cutline_polygons','cutline_polygon',%1,'POLYGON','XY');" ).arg( i_srid );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "SELECT CreateSpatialIndex('create_cutline_polygons','cutline_polygon');" );
      sa_sql_commands.append( s_sql );
      // --- Mercator POLYGONs to assist georeferencing
      s_sql = QString( s_sql_cutline ).arg( "polygons" ).arg( "POLYGON" ).arg( "POLYGONs" );
      s_sql.replace( QString( "create_cutline_polygons" ), QString( "create_mercator_polygons" ) );
      s_sql.replace( QString( " id_gcp_coverage INTEGER DEFAULT 0,\n" ), QString( " id_gcp_coverage INTEGER DEFAULT 0,\n -- cutline_type=77: use as gdal cutline for id_gcp_coverage\n" ) );
      // s_sql.replace( QString( " notes TEXT DEFAULT '',\n" ), QString( " notes TEXT DEFAULT 'EPSG:3395;WGS 84 / World Mercator',\n" ) );
      // sa_sql_commands.append( s_sql );
      // s_sql = QString( "SELECT AddGeometryColumn('create_mercator_polygons','mercator_polygon',%1,'POLYGON','XY');" ).arg( 3395 );
      s_sql.replace( QString( " notes TEXT DEFAULT '',\n" ), QString( " notes TEXT DEFAULT '',\n" ) );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "SELECT AddGeometryColumn('create_mercator_polygons','mercator_polygon',%1,'POLYGON','XY');" ).arg( -1 );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "SELECT CreateSpatialIndex('create_mercator_polygons','mercator_polygon');" );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "SELECT AddGeometryColumn('create_mercator_polygons','cutline_polygon',%1,'POLYGON','XY');" ).arg( i_srid );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "SELECT CreateSpatialIndex('create_mercator_polygons','cutline_polygon');" );
      sa_sql_commands.append( s_sql );
    }
    if (( s_command_type.toUpper() == "INSERT_COVERAGES" ) && ( !s_coverage_values.isNull() ) )
    { // With 'id_gcp_coverage'
      QString s_sql_fields = QString( "id_gcp_coverage,coverage_name,map_date,raster_file,raster_gtif,title,abstract,copyright," );
      s_sql_fields += QString( "scale,nodata,srid,id_cutline,gcp_count,transformtype, resampling, compression, image_max_x,image_max_y," );
      s_sql_fields += QString( "extent_minx,extent_miny,extent_maxx,extent_maxy,raster_path" );
      s_sql = QString( "INSERT INTO gcp_coverages\n(%1)\n%2" ).arg( s_sql_fields ).arg( s_coverage_values );
      sa_sql_commands.append( s_sql );
    }
    if (( s_command_type.toUpper() == "INSERT_COVERAGE" ) && ( !s_coverage_values.isNull() ) )
    { // Without 'id_gcp_coverage'
      QString s_sql_fields = QString( "coverage_name,map_date,raster_file,raster_gtif,title,abstract,copyright," );
      s_sql_fields += QString( "scale,nodata,srid,id_cutline,gcp_count,transformtype, resampling, compression, image_max_x,image_max_y," );
      s_sql_fields += QString( "extent_minx,extent_miny,extent_maxx,extent_maxy,raster_path" );
      s_sql = QString( "INSERT INTO gcp_coverages\n(%1)\n%2" ).arg( s_sql_fields ).arg( s_coverage_values );
      sa_sql_commands.append( s_sql );
    }
    if (( s_command_type.toUpper() == "INSERT_COMPUTE" ) && ( !s_coverage_name.isNull() ) )
    {
      if ( parms_GcpDbData->mGcp_enabled )
      {
        QString s_sql = QString( "INSERT INTO '%1'\n (coverage_name,id_gcp_coverage, image_max_x, image_max_y, notes, srid, id_order)\n VALUES" ).arg( "gcp_compute" );
        for ( int i_order = 0;i_order < 4;i_order++ )
        { // Order 0-3 - Map-Point to Pixel-Point
          if ( i_order > 0 )
          {
            s_sql += QString( ",\n " );
          }
          s_sql += QString( "('%1'," ).arg( s_coverage_name );
          s_sql += QString( "%1," ).arg( i_gcp_coverage_id );
          s_sql += QString( "%1," ).arg( i_image_max_x );
          s_sql += QString( "%1," ).arg( i_image_max_y );
          s_sql += QString( "'%1'," ).arg( s_map_point_to_pixel_point );
          s_sql += QString( "%1," ).arg( i_srid );
          s_sql += QString( "%1),\n " ).arg( i_order );
          // Order 0-3 -  Pixel-Point to Map-Point
          s_sql += QString( "('%1'," ).arg( s_coverage_name );
          s_sql += QString( "%1," ).arg( i_gcp_coverage_id );
          s_sql += QString( "%1," ).arg( i_image_max_x );
          s_sql += QString( "%1," ).arg( i_image_max_y );
          s_sql += QString( "'%1'," ).arg( s_pixel_point_to_map_point );
          s_sql += QString( "%1," ).arg( -1 );
          s_sql += QString( "%1)" ).arg( i_order );
        }
        s_sql += QString( ";" );
        sa_sql_commands.append( s_sql );
      }
    }
    if (( s_command_type.toUpper() == "UPDATE_COMPUTE" ) && ( !s_coverage_name.isNull() ) )
    {
      if ( parms_GcpDbData->mGcp_enabled )
      {
        for ( int i_order = 0;i_order < 4;i_order++ )
        { // Order 0-3 - Map-Point to Pixel-Point
          s_sql = QString( "UPDATE '%1' SET gcp_compute=\n" ).arg( "gcp_compute" );
          s_sql += QString( " (SELECT GCP_Compute(gcp_point, gcp_pixel, %1 ) FROM '%2' WHERE ((gcp_point IS NOT NULL) AND (gcp_enable <> 0) AND (order_selected=%1)))\n" ).arg( i_order ).arg( s_points_table_name );
          s_sql += QString( "WHERE ((coverage_name='%1') AND (id_order=%2) AND (notes='%3'));" ).arg( s_coverage_name ).arg( i_order ).arg( s_map_point_to_pixel_point );
          sa_sql_commands.append( s_sql );
          // Order 0-3 -  Pixel-Point to Map-Point
          s_sql = QString( "UPDATE '%1' SET gcp_compute=\n" ).arg( "gcp_compute" );
          s_sql += QString( " (SELECT GCP_Compute(gcp_pixel, gcp_point, %1 ) FROM '%2' WHERE ((gcp_pixel IS NOT NULL) AND (gcp_enable <> 0) AND (order_selected=%1)))\n" ).arg( i_order ).arg( s_points_table_name );
          s_sql += QString( "WHERE ((coverage_name='%1') AND (id_order=%2) AND (notes='%3'));" ).arg( s_coverage_name ).arg( i_order ).arg( s_pixel_point_to_map_point );
          sa_sql_commands.append( s_sql );
          // qDebug() << QString( "-I-> UPDATE_COMPUTE[%1] sql[%2] " ).arg(sa_sql_commands.size()).arg( s_sql);
        }
      }
    }
    if (( s_command_type.toUpper() == "TRANSFORM_COMPUTE" ) && ( !s_update_table.isNull() ) )
    { //  "TRANSFORM_COMPUTE;#;#create_mercator_polygons;#;#mercator_polygon;#;#cutline_polygon#;#i_gcp_coverage_id#;#i_gcp_coverage_srid"
      if ( parms_GcpDbData->mGcp_enabled )
      { // Update all of the Pixel-based Polygons to Polygongs of the Srid using the Polynomial Coefficients of the Gcp-Points
        QString s_where_coverage_id = "";
        if ( i_gcp_coverage_id > 0 )
        {
          s_where_coverage_id = QString( "\nWHERE\n(\n (%1=%2)\n)" ).arg( "id_gcp_coverage" ).arg( i_gcp_coverage_id );
        }
        QString s_select_id_order = QString( " (id_order = %1) AND" ).arg( i_order );
        QString s_select_toPixel = " (srid=-1)";
        if ( b_toPixel )
          s_select_toPixel = " (srid<>-1)";
        QString s_srid_query = QString( "SELECT srid FROM %1 WHERE (%3.%2=%1.%2)" ).arg( "gcp_coverages" ).arg( "id_gcp_coverage" ).arg( s_update_table );
        QString s_select_transform = QString( " SELECT\n  GCP_Transform\n  (\n   %1,\n   g_geometry.matrix,\n   (%2)\n  ) AS result_polygon\n" ).arg( s_source_field ).arg( s_srid_query );
        QString s_where_compute = QString( "  WHERE\n  (\n   (%1.%2 = %3.%2) AND\n  %4\n  %5\n  )" ).arg( "gcp_compute" ).arg( "id_gcp_coverage" ).arg( s_update_table ).arg( s_select_id_order ).arg( s_select_toPixel );
        QString s_select_compute = QString( " SELECT\n   %1 AS matrix\n  FROM\n   %1\n%2" ).arg( "gcp_compute" ).arg( s_where_compute );
        QString s_from_transform = QString( " FROM\n (\n %1\n ) AS g_geometry" ).arg( s_select_compute );
        QString s_subquery_transform = QString( "%1%2" ).arg( s_select_transform ).arg( s_from_transform );
        QString s_message = QString( " -- Update all of the Pixel-based Polygons to Polygons of the Srid using the Polynomial Coefficients of the Gcp-Points" );
        s_sql = QString( "UPDATE '%1' SET %2=\n(%3\n%4\n)%5;" ).arg( s_update_table ).arg( s_update_field ).arg( s_message ).arg( s_subquery_transform ).arg( s_where_coverage_id );
        sa_sql_commands.append( s_sql );
        s_sql = QString( "SELECT UpdateLayerStatistics('%1','%2');" ).arg( s_update_table ).arg( s_update_field );
        sa_sql_commands.append( s_sql );
      }
    }
  }
  return sa_sql_commands;
}
QStringList QgsSpatiaLiteProviderGcpUtils::createGcpSqlMasterCommands( GcpDbData* parms_GcpDbData, QStringList sa_tables )
{ // Format: id_gcp_coverage;#;#i_srid;#;#s_coverage_name
  QStringList sa_sql_commands;
  if ( !parms_GcpDbData )
  {
    return sa_sql_commands;
  }
  for ( int i_list = 0;i_list < sa_tables.size();i_list++ )
  { // raster entry has been added to gcp_covarage, create corresponding tables
    QStringList sa_fields = sa_tables.at( i_list ).split( parms_GcpDbData->mParseString );
    QString s_sql = QString::null;
    QString s_command_type = "";
    QString s_gcp_master = "gcp_master";
    int i_srid = parms_GcpDbData->mGcpSrid;
    qDebug() << QString( "createGcpSqlMasterCommands: srid[%3] fields_count[%1] line[%2]" ).arg( sa_fields.count() ).arg( sa_tables.at( i_list ) ).arg( i_srid );
    // --
    QString s_coverage_values = QString::null;
    QString s_coverage_name = QString::null;
    QString s_points_table_name = "";
    if ( sa_fields.count() > 1 )
    {
      s_command_type = sa_fields.at( 0 );
      if ( sa_fields.count() > 2 )
      {
        if ( s_command_type.toUpper().startsWith( "CREATE_MASTER" ) )
        { //  "CREATE_MASTER;#;#gcp_master;#;#3068"
          s_gcp_master = sa_fields.at( 1 );
          i_srid = sa_fields.at( 2 ).toInt();
        }
      }
      if ( s_command_type.toUpper() ==  "SELECT_MASTER" )
      { //  "SELECT_MASTER;#;#gcp_master"
        s_gcp_master = sa_fields.at( 1 );
      }
    }
    else
    { // Entry is invalid, skip
      continue;
    }
    if ( s_command_type.toUpper() == "CREATE_MASTER" )
    {
      s_sql = QString( "CREATE TABLE IF NOT EXISTS '%1'\n(\n" ).arg( s_gcp_master );
      s_sql += QString( " %1 INTEGER ,\n" ).arg( "id_gcp" );
      s_sql += QString( " %1 TEXT DEFAULT '',\n" ).arg( "name" );
      s_sql += QString( " %1 TEXT DEFAULT '',\n" ).arg( "notes" );
      s_sql += QString( " %1 TEXT DEFAULT '',\n" ).arg( "text" );
      s_sql += QString( " %1 TEXT DEFAULT '',\n" ).arg( "belongs_to_01" );
      s_sql += QString( " %1 TEXT DEFAULT '',\n" ).arg( "belongs_to_02" );
      s_sql += QString( " %1 DATE DEFAULT '0001-01-01',\n" ).arg( "valid_since" );
      s_sql += QString( " %1 DATE DEFAULT '3000-01-01',\n" ).arg( "valid_until" );
      s_sql += QString( " --> %1 \n" ).arg( "will be set by INSERT and UPDATE TRIGGERs" );
      s_sql += QString( " %1 DOUBLE DEFAULT 0,\n" ).arg( "map_x" );
      s_sql += QString( " --> %1 \n" ).arg( "will be set by INSERT and UPDATE TRIGGERs" );
      s_sql += QString( " %1 DOUBLE DEFAULT 0,\n" ).arg( "map_y" );
      s_sql += QString( " %1 INTEGER DEFAULT %2,\n" ).arg( "srid" ).arg( i_srid );
      s_sql += QString( " -- %1 \n" ).arg( "0=general point" );
      s_sql += QString( " -- %1 \n" ).arg( "1=Known Point" );
      s_sql += QString( " -- %1 \n" ).arg( "20=bridge" );
      s_sql += QString( " -- %1 \n" ).arg( "100-199=street interface type" );
      s_sql += QString( " --> %1 \n" ).arg( "100=general" );
      s_sql += QString( " --> %1 \n" ).arg( "101 = street_start" );
      s_sql += QString( " --> %1 \n" ).arg( "102 = street end" );
      s_sql += QString( " --> %1 \n" ).arg( "109 = nearest point" );
      s_sql += QString( " --> %1 \n" ).arg( "110 = street intersection (street continues)" );
      s_sql += QString( " --> %1 \n" ).arg( "111 = street corner intersection (NW,NE,SE,SW)" );
      s_sql += QString( " -- %1 \n" ).arg( "maps: Top/Left Corner of Map, these maps share common points" );
      s_sql += QString( " --> %1 \n" ).arg( "201 = scale 1:1000 (K1)" );
      s_sql += QString( " --> %1 \n" ).arg( "203 = scale 1:4000 (Straube)" );
      s_sql += QString( " --> %1 \n" ).arg( "204 = scale 1:4000 (K4)" );
      s_sql += QString( " --> %1 \n" ).arg( "205 = scale 1:5000 (K5)" );
      s_sql += QString( " --> %1 \n" ).arg( "210 = scale 1:10000 (K10)" );
      s_sql += QString( " --> %1 \n" ).arg( "700 = Historical Points" );
      s_sql += QString( " -- %1 \n" ).arg( "9999=no text/unknown" );
      s_sql += QString( " -- %1 \n" ).arg( "9998=delete" );
      s_sql += QString( " %1 INTEGER DEFAULT 0,\n" ).arg( "gcp_type" );
      s_sql += QString( " -- %1 \n" ).arg( "0=tpc (Thin Plate Spline)" );
      s_sql += QString( " -- %1 \n" ).arg( "1=1st order(affine) [precise]" );
      s_sql += QString( " -- %1 \n" ).arg( "2nd order [less exact]" );
      s_sql += QString( " -- %1 \n" ).arg( "3rd order [poor]" );
      s_sql += QString( " %1 INTEGER DEFAULT 0,\n" ).arg( "order_selected" );
      s_sql += QString( " %1 INTEGER DEFAULT 1,\n" ).arg( "gcp_enable" );
      s_sql += QString( " %1 TEXT DEFAULT '',\n" ).arg( "gcp_text" );
      s_sql += QString( " PRIMARY KEY (%1)\n);" ).arg( "id_gcp" );
      sa_sql_commands.append( s_sql );
      // -------
      s_sql = QString( "SELECT AddGeometryColumn('%1','gcp_point',%2,'POINT','XY');" ).arg( s_gcp_master ).arg( i_srid );
      sa_sql_commands.append( s_sql );
      s_sql = QString( "SELECT CreateSpatialIndex('%1','gcp_point');" ).arg( s_gcp_master );
      sa_sql_commands.append( s_sql );
      // -------
      QString s_TRIGGER_FIELDS = QString( "id_gcp,name,notes,text,belongs_to_01,belongs_to_02,valid_since,valid_until,gcp_type,map_x,map_y,srid,order_selected,gcp_point" );
      // If while INSERTING, no geometries are supplied (only valid pixel/point x+y values), the geometries will be created
      QString s_TRIGGER_INSERT_BASE = QString( "CREATE TRIGGER IF NOT EXISTS 'tb_ins_%1'\n" ).arg( s_gcp_master );
      s_TRIGGER_INSERT_BASE += QString( " AFTER INSERT ON \"%1\"\nBEGIN\n" ).arg( s_gcp_master );
      s_TRIGGER_INSERT_BASE += QString( " UPDATE \"%1\" \n" ).arg( s_gcp_master );
      s_sql = QString( "%1 -- general checking for and setting of NULL values done in 'tb_upd_gcp_points'\n SET\n" ).arg( s_TRIGGER_INSERT_BASE );
      s_sql += QString( "  name = CASE WHEN NEW.name IS NULL THEN '' ELSE NEW.name END,\n" );
      s_sql += QString( "  notes = CASE WHEN NEW.notes IS NULL THEN '' ELSE NEW.notes END,\n" );
      s_sql += QString( "  text = CASE WHEN NEW.text IS NULL THEN '' ELSE NEW.text END,\n" );
      s_sql += QString( "  belongs_to_01 = CASE WHEN NEW.belongs_to_01 IS NULL THEN '' ELSE NEW.belongs_to_01 END,\n" );
      s_sql += QString( "  belongs_to_02 = CASE WHEN NEW.belongs_to_02 IS NULL THEN '' ELSE NEW.belongs_to_02 END,\n" );
      s_sql += QString( "  valid_since = CASE WHEN NEW.valid_since IS NULL THEN '0001-01-01' ELSE NEW.valid_since END,\n" );
      s_sql += QString( "  valid_until = CASE WHEN NEW.valid_until IS NULL THEN '3000-01-01' ELSE NEW.valid_until END,\n" );
      s_sql += QString( "  gcp_type = CASE WHEN NEW.gcp_type IS NULL THEN 0 ELSE NEW.gcp_type END,\n" );
      s_sql += QString( "  map_x = CASE WHEN NEW.gcp_point IS NOT NULL THEN ST_X(NEW.gcp_point) ELSE 0 END,\n" );
      s_sql += QString( "  map_y = CASE WHEN NEW.gcp_point IS NOT NULL THEN ST_Y(NEW.gcp_point) ELSE 0 END,\n" );
      s_sql += QString( "  srid = CASE WHEN NEW.gcp_point IS NOT NULL THEN ST_SRID(NEW.gcp_point) ELSE NEW.srid END,\n" );
      s_sql += QString( "  order_selected = CASE WHEN NEW.order_selected IS NULL THEN 0 ELSE NEW.order_selected END,\n" );
      s_sql += QString( "  gcp_point = NEW.gcp_point\n" );
      s_sql += QString( " WHERE id_gcp = NEW.id_gcp;\nEND;" );
      sa_sql_commands.append( s_sql );
      QString s_TRIGGER_INSERT = s_TRIGGER_INSERT_BASE;
      s_TRIGGER_INSERT = s_TRIGGER_INSERT.replace( QString( "tb_ins_%1'" ).arg( s_gcp_master ), QString( "vw_ins_%1'" ).arg( s_gcp_master ) );
      s_TRIGGER_INSERT = s_TRIGGER_INSERT.replace( QString( "AFTER INSERT ON" ), QString( "INSTEAD OF INSERT ON" ) );
      s_TRIGGER_INSERT = s_TRIGGER_INSERT.replace( QString( "UPDATE \"%1\" \n" ).arg( s_gcp_master ), QString( "INSERT OR REPLACE INTO \"%1\" \n" ).arg( s_gcp_master ) );
      s_TRIGGER_INSERT += QString( " (%1)\n VALUES\n ( -- sanity checks will be done in 'tb_ins_%2'\n" ).arg( s_TRIGGER_FIELDS ).arg( s_gcp_master );
      s_TRIGGER_INSERT += QString( "  NEW.id_gcp,\n" );
      s_TRIGGER_INSERT += QString( "  NEW.name,\n" );
      s_TRIGGER_INSERT += QString( "  NEW.notes,\n" );
      s_TRIGGER_INSERT += QString( "  NEW.text,\n" );
      s_TRIGGER_INSERT += QString( "  NEW.belongs_to_01,\n" );
      s_TRIGGER_INSERT += QString( "  NEW.belongs_to_02,\n" );
      s_TRIGGER_INSERT += QString( "  NEW.valid_since,\n" );
      s_TRIGGER_INSERT += QString( "  NEW.valid_until,\n" );
      s_TRIGGER_INSERT += QString( "  CASE WHEN NEW.gcp_type IS NULL THEN 0 ELSE NEW.gcp_type END,\n" );
      s_TRIGGER_INSERT += QString( "  NEW.map_x,\n" );
      s_TRIGGER_INSERT += QString( "  NEW.map_y,\n" );
      s_TRIGGER_INSERT += QString( "  NEW.srid,\n" );
      s_TRIGGER_INSERT += QString( "  NEW.order_selected,\n" );
      s_TRIGGER_INSERT += QString( "  NEW.gcp_point\n" );
      s_TRIGGER_INSERT += QString( " );\nEND;" );
      // -------
      // AFTER (not BEFORE) an UPDATE of the geometries,  the pixel/point x+y values will be updated
      QString s_TRIGGER_UPDATE_BASE = QString( "CREATE TRIGGER IF NOT EXISTS 'tb_upd_%1'\n" ).arg( s_gcp_master );
      s_TRIGGER_UPDATE_BASE += QString( " AFTER UPDATE OF\n" );
      s_TRIGGER_UPDATE_BASE += QString( "  %1\n" ).arg( s_TRIGGER_FIELDS );
      s_TRIGGER_UPDATE_BASE += QString( " ON \"%1\" \nBEGIN\n" ).arg( s_gcp_master );
      s_TRIGGER_UPDATE_BASE += QString( " UPDATE \"%1\" \n" ).arg( s_gcp_master );
      s_sql = QString( "%1 -- general checking for and setting of NULL values\n SET\n" ).arg( s_TRIGGER_UPDATE_BASE );
      s_sql += QString( "  name = CASE WHEN NEW.name IS NULL THEN '' ELSE NEW.name END,\n" );
      s_sql += QString( "  notes = CASE WHEN NEW.notes IS NULL THEN '' ELSE NEW.notes END,\n" );
      s_sql += QString( "  text = CASE WHEN NEW.text IS NULL THEN '' ELSE NEW.text END,\n" );
      s_sql += QString( "  belongs_to_01 = CASE WHEN NEW.belongs_to_01 IS NULL THEN '' ELSE NEW.belongs_to_01 END,\n" );
      s_sql += QString( "  belongs_to_02 = CASE WHEN NEW.belongs_to_02 IS NULL THEN '' ELSE NEW.belongs_to_02 END,\n" );
      s_sql += QString( "  valid_since = CASE WHEN NEW.valid_since IS NULL THEN '0001-01-01' ELSE NEW.valid_since END,\n" );
      s_sql += QString( "  valid_until = CASE WHEN NEW.valid_until IS NULL THEN '3000-01-01' ELSE NEW.valid_until END,\n" );
      s_sql += QString( "  gcp_type = CASE WHEN NEW.gcp_type IS NULL THEN 0 ELSE NEW.gcp_type END,\n" );
      s_sql += QString( "  order_selected = CASE WHEN NEW.order_selected IS NULL THEN 0 ELSE NEW.order_selected END,\n" );
      s_sql += QString( "  map_x = CASE WHEN NEW.gcp_point IS NOT NULL THEN ST_X(NEW.gcp_point) ELSE 0 END,\n" );
      s_sql += QString( "  map_y = CASE WHEN NEW.gcp_point IS NOT NULL THEN ST_Y(NEW.gcp_point) ELSE 0 END,\n" );
      s_sql += QString( "  srid = CASE WHEN NEW.gcp_point IS NOT NULL THEN ST_SRID(NEW.gcp_point) ELSE NEW.srid END,\n" );
      s_sql += QString( "  order_selected = CASE WHEN NEW.order_selected IS NULL THEN 0 ELSE NEW.order_selected END,\n" );
      s_sql += QString( "  gcp_point = NEW.gcp_point\n" );
      s_sql += QString( " WHERE id_gcp = OLD.id_gcp;\nEND;" );
      sa_sql_commands.append( s_sql );
      // -------
      QString s_TRIGGER_UPDATE = s_TRIGGER_UPDATE_BASE;
      s_TRIGGER_UPDATE = s_TRIGGER_UPDATE.replace( QString( "tb_upd_%1'" ).arg( s_gcp_master ), QString( "vw_upd_%1'" ).arg( s_gcp_master ) );
      s_TRIGGER_UPDATE = s_TRIGGER_UPDATE.replace( QString( "AFTER UPDATE OF" ), QString( "INSTEAD OF UPDATE OF" ) );
      s_TRIGGER_UPDATE += QString( " -- sanity checks will be done in 'tb_upd_%1'\n SET\n" ).arg( s_gcp_master );
      s_TRIGGER_UPDATE += QString( "  name = NEW.name,\n" );
      s_TRIGGER_UPDATE += QString( "  notes = NEW.notes,\n" );
      s_TRIGGER_UPDATE += QString( "  text = NEW.text,\n" );
      s_TRIGGER_UPDATE += QString( "  belongs_to_01 = NEW.belongs_to_01,\n" );
      s_TRIGGER_UPDATE += QString( "  belongs_to_02 = NEW.belongs_to_02,\n" );
      s_TRIGGER_UPDATE += QString( "  valid_since = NEW.valid_since,\n" );
      s_TRIGGER_UPDATE += QString( "  valid_until = NEW.valid_until,\n" );
      s_TRIGGER_UPDATE += QString( "  gcp_type = CASE WHEN NEW.gcp_type IS NULL THEN 0 ELSE NEW.gcp_type END,\n" );
      s_TRIGGER_UPDATE += QString( "  map_x = NEW.map_x,\n" );
      s_TRIGGER_UPDATE += QString( "  map_y = NEW.map_y,\n" );
      s_TRIGGER_UPDATE += QString( "  srid = NEW.srid,\n" );
      s_TRIGGER_UPDATE += QString( "  order_selected = NEW.order_selected,\n" );
      s_TRIGGER_UPDATE += QString( "  gcp_point = NEW.gcp_point\n" );
      s_TRIGGER_UPDATE += QString( " WHERE id_gcp = OLD.id_gcp;\nEND;" );
      // -------
      // Create SpatialView: (INSERT, UPDATE and DELETE)
      QString s_TRIGGER_DELETE = QString( "CREATE TRIGGER IF NOT EXISTS 'vw_del_%1' \n" ).arg( s_gcp_master );
      s_TRIGGER_DELETE += QString( " INSTEAD OF DELETE ON %1 \nBEGIN\n %2\n" ).arg( s_gcp_master ).arg( "-- the primary key known to the view must be used !" );
      s_TRIGGER_DELETE += QString( " DELETE FROM %1 WHERE id_gcp = OLD.id_gcp;\nEND;" ).arg( s_gcp_master );
      // -------
      QStringList sa_view_types;
      QMap<int, QString> map_views;
      map_views.insert( 0, QString( "%1;%2" ).arg( "general_points_general" ).arg( "0=general point" ) );
      map_views.insert( 1, QString( "%1;%2" ).arg( "points_known" ).arg( "1 Known Point" ) );
      map_views.insert( 700, QString( "%1;%2" ).arg( "points_historical" ).arg( "700=Historical Points" ) );
      map_views.insert( 109, QString( "%1;%2" ).arg( "points_nearest" ).arg( "109=nearest point" ) );
      map_views.insert( 20, QString( "%1;%2" ).arg( "bridges" ).arg( "20=bridge" ) );
      map_views.insert( 100, QString( "%1;%2" ).arg( "general" ).arg( "100=general" ) );
      map_views.insert( 101, QString( "%1;%2" ).arg( "street_start" ).arg( "101=street start point" ) );
      map_views.insert( 102, QString( "%1;%2" ).arg( "street_end" ).arg( "102=street end point" ) );
      map_views.insert( 110, QString( "%1;%2" ).arg( "street_intersection" ).arg( "110=street intersection (street continues)" ) );
      map_views.insert( 111, QString( "%1;%2" ).arg( "street_corners" ).arg( "111=street corner intersection (NW,NE,SE,SW)" ) );
      map_views.insert( 201, QString( "%1;%2" ).arg( "maps_scale_1000" ).arg( "201=scale 1:1000 (K1)" ) );
      map_views.insert( 203, QString( "%1;%2" ).arg( "maps_scale_4000_straube" ).arg( "203=scale 1:4000 (Straube)" ) );
      map_views.insert( 204, QString( "%1;%2" ).arg( "maps_scale_4000" ).arg( "204=scale 1:4000 (K4)" ) );
      map_views.insert( 205, QString( "%1;%2" ).arg( "maps_scale_5000" ).arg( "205=scale 1:5000 (K5)" ) );
      map_views.insert( 210, QString( "%1;%2" ).arg( "maps_scale_10000" ).arg( "210=scale 1:10000 (K10)" ) );
      QMap<int, QString>::iterator it_map_views;
      for ( it_map_views = map_views.begin(); it_map_views != map_views.end(); ++it_map_views )
      {
        int i_gcp_type = it_map_views.key();
        QString s_value = it_map_views.value();
        QStringList sa_list = s_value.split( ";" );
        QString s_gcp_type_name = sa_list[0];
        QString s_gcp_type_info = sa_list[1];
        s_sql = QString( "CREATE VIEW IF NOT EXISTS '%1_%2' AS\n" ).arg( s_gcp_master ).arg( s_gcp_type_name );
        s_sql += QString( " SELECT \n  %1 \n" ).arg( "id_gcp,name,notes,text,belongs_to_01,belongs_to_02,valid_since,valid_until,gcp_type,map_x,map_y,srid,order_selected,gcp_point" );
        s_sql += QString( " FROM \"%1\"\n WHERE \n ( -- %2\n  gcp_type=%3\n )\n ORDER BY name;" ).arg( s_gcp_master ).arg( s_gcp_type_info ).arg( i_gcp_type );
        sa_sql_commands.append( s_sql );
        // -------
        s_sql = QString( "INSERT INTO views_geometry_columns\n%1\n" ).arg( " (view_name,view_geometry,view_rowid,f_table_name,f_geometry_column,read_only)" );
        s_sql += QString( " VALUES ('%1_%2%3);" ).arg( s_gcp_master ).arg( s_gcp_type_name ).arg( "','gcp_point','id_gcp','gcp_master','gcp_point',0" );
        sa_sql_commands.append( s_sql );
        // -------
        s_sql = s_TRIGGER_INSERT;
        s_sql = s_sql.replace( QString( "vw_ins_%1'" ).arg( s_gcp_master ), QString( "vw_ins_%1_%2'" ).arg( s_gcp_master ).arg( s_gcp_type_name ) );
        s_sql = s_sql.replace( QString( "INSTEAD OF INSERT ON \"%1\"" ).arg( s_gcp_master ), QString( "INSTEAD OF INSERT ON \"%1_%2\"" ).arg( s_gcp_master ).arg( s_gcp_type_name ) );
        s_sql = s_sql.replace( QString( "CASE WHEN NEW.gcp_type IS NULL THEN 0 ELSE NEW.gcp_type END," ), QString( "CASE WHEN NEW.gcp_type IS NULL THEN %1 ELSE NEW.gcp_type END," ).arg( i_gcp_type ) );
        sa_sql_commands.append( s_sql );
        // -------
        s_sql = s_TRIGGER_UPDATE;
        s_sql = s_sql.replace( QString( "vw_upd_%1'" ).arg( s_gcp_master ), QString( "vw_upd_%1_%2'" ).arg( s_gcp_master ).arg( s_gcp_type_name ) );
        s_sql = s_sql.replace( QString( " ON \"%1\" \nBEGIN\n" ).arg( s_gcp_master ), QString( " ON \"%1_%2\" \nBEGIN\n" ).arg( s_gcp_master ).arg( s_gcp_type_name ) );
        s_sql = s_sql.replace( QString( " CASE WHEN NEW.gcp_type IS NULL THEN 0 ELSE NEW.gcp_type END," ), QString( " CASE WHEN NEW.gcp_type IS NULL THEN %1 ELSE NEW.gcp_type END," ).arg( i_gcp_type ) );
        sa_sql_commands.append( s_sql );
        // -------
        s_sql = s_TRIGGER_DELETE;
        s_sql = s_sql.replace( QString( "vw_del_%1'" ).arg( s_gcp_master ), QString( "vw_del_%1_%2'" ).arg( s_gcp_master ).arg( s_gcp_type_name ) );
        s_sql = s_sql.replace( QString( "INSTEAD OF DELETE ON %1" ).arg( s_gcp_master ), QString( "INSTEAD OF DELETE ON %1_%2" ).arg( s_gcp_master ).arg( s_gcp_type_name ) );
        sa_sql_commands.append( s_sql );
      }
    }
    if ( s_command_type.toUpper() == "SELECT_MASTER" )
    { // 3 results: select_insert (with select_fields),select_values, UpdateLayerStatistics
      QString s_select_fields = QString( "(id_gcp, name, notes, text, belongs_to_01, belongs_to_02, valid_since, valid_until,map_x,map_y,srid,gcp_type,order_selected,gcp_enable,gcp_text, gcp_point)" );
      s_sql = QString( "INSERT INTO '%1'\n" ).arg( s_gcp_master );
      s_sql += QString( "%1\n " ).arg( s_select_fields );
      s_sql += QString( "VALUES%1" ); // select_values [from result set]
      sa_sql_commands.append( s_sql );
      //-- read gcp_master
      s_sql = QString( "SELECT\n " );
      // Format: INSERT_MASTER;#;#VALUES(...)
      // returns the INSERT VALUE portion for each entry in gcp_master
      s_sql += QString( "'%1'" ).arg( "(" );
      s_sql += QString( "||%1" ).arg( "id_gcp" );
      s_sql += QString( "||','''||%1" ).arg( "name" );
      s_sql += QString( "||''','''||%1" ).arg( "notes" );
      s_sql += QString( "||''','''||%1" ).arg( "text" );
      s_sql += QString( "||''','''||%1" ).arg( "belongs_to_01" );
      s_sql += QString( "||''','''||%1" ).arg( "belongs_to_02" );
      s_sql += QString( "||''','''||%1" ).arg( "valid_since" );
      s_sql += QString( "||''','''||%1" ).arg( "valid_until" );
      s_sql += QString( "||''',\n '||%1" ).arg( "map_x" );
      s_sql += QString( "||','||%1" ).arg( "map_y" );
      s_sql += QString( "||','||%1" ).arg( "srid" );
      s_sql += QString( "||','||%1" ).arg( "gcp_type" );
      s_sql += QString( "||','||%1" ).arg( "order_selected" );
      s_sql += QString( "||','||%1" ).arg( "gcp_enable" );
      s_sql += QString( "||','''||%1" ).arg( "gcp_text" );
      s_sql += QString( "||''',\n  GeomFromEWKT('''||AsEWKT(gcp_point)||'''))'AS master_value\n" );
      s_sql += QString( "FROM '%1'" ).arg( s_gcp_master );
      s_sql += QString( "\nORDER BY %1;" ).arg( "id_gcp,valid_since" );
      sa_sql_commands.append( s_sql );
      // Runs after the creation of SpatialViews, therefore everything will be updated
      s_sql = QString( "SELECT UpdateLayerStatistics();" );
      sa_sql_commands.append( s_sql );
    }
  }
  return sa_sql_commands;
}
bool QgsSpatiaLiteProviderGcpUtils::bulkGcpPointsInsert( GcpDbData* parms_GcpDbData )
{
  if ( !parms_GcpDbData )
    return false;
  if ( parms_GcpDbData->gcp_coverages.size() <= 0 )
    return false;
  QString  s_database_filename = parms_GcpDbData->mGcpDatabaseFileName;
  bool b_database_exists = QFile( s_database_filename ).exists();
  if ( !b_database_exists )
    return false;
  int i_sql_command_type = parms_GcpDbData->mOrder;
  if (( i_sql_command_type < 1 ) || ( i_sql_command_type > 2 ) )
  { // 1=INSERT, 2=UPDATE
    return false;
  }
  parms_GcpDbData->mDatabaseValid = false;
  QString s_sql_type = "INSERT";
  if ( i_sql_command_type == 2 )
    s_sql_type = "UPDATE";
  QString s_coverage_name = parms_GcpDbData->mGcp_coverage_name;
  s_coverage_name = s_coverage_name.toLower();
  QString s_gcp_points_table_name = QString( "gcp_points_%1" ).arg( s_coverage_name );
  if ( b_database_exists )
  {
    sqlite3* db_handle;
    char *errMsg = nullptr;
    int i_sql_counter = 0;
    // open database
    int ret = QgsSpatiaLiteProviderGcpUtils::spatialiteInitEx( parms_GcpDbData, s_database_filename );
    if ( ret != SQLITE_OK )
    {
      parms_GcpDbData->mError = QString( "Failed to open database: rc=%1 database[%2]" ).arg( ret ).arg( s_database_filename );
      if ( ret != SQLITE_MISUSE )
      { // 'SQLITE_MISUSE': in use with another file, do not close
        QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
      }
      return false;
    }
    // To simplfy the useage.
    db_handle = parms_GcpDbData->db_handle;
    // check if gcp_compute contains entries for this coverage
    ret = sqlite3_exec( db_handle, "BEGIN", NULL, NULL, 0 );
    Q_ASSERT( ret == SQLITE_OK );
    Q_UNUSED( ret );
    QMap<int, QString>::iterator it_map_coverages;
    for ( it_map_coverages = parms_GcpDbData->gcp_coverages.begin(); it_map_coverages != parms_GcpDbData->gcp_coverages.end(); ++it_map_coverages )
    {
      int id_gcp_master = it_map_coverages.key();
      QString s_sql = it_map_coverages.value();
      ret = sqlite3_exec( db_handle, s_sql.toUtf8().constData(), NULL, NULL, &errMsg );
      i_sql_counter++;
      if ( ret != SQLITE_OK )
      {
        parms_GcpDbData->mError = QString( "Unable to %5 INTO [%4]: rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( errMsg ) ).arg( s_sql ).arg( s_gcp_points_table_name ).arg( s_sql_type );
        qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::bulkGcpPointsInsert -%1- [%2] " ).arg( i_sql_counter ).arg( parms_GcpDbData->mError );
        sqlite3_free( errMsg );
        // End TRANSACTION
        ret = sqlite3_exec( db_handle, "ROLLBACK", NULL, NULL, 0 );
        Q_ASSERT( ret == SQLITE_OK );
        Q_UNUSED( ret );
        QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
        return false;
      }
      switch ( i_sql_command_type )
      {
        case 1:
        { // INSERT
          int id_gcp = getSqliteSequence( parms_GcpDbData, s_gcp_points_table_name );
          if ( id_gcp != SQLITE_ERROR )
          { // Replace the Sql-Statement with the id_gcp or the INSERTEd row
            parms_GcpDbData->gcp_coverages[id_gcp_master] = QString( "%1" ).arg( id_gcp );
          }
        }
        break;
        case 2:
        { // UPDATE
        }
        break;
      }
    }
    //-------
    // End TRANSACTION
    ret = sqlite3_exec( db_handle, "COMMIT", NULL, NULL, 0 );
    Q_ASSERT( ret == SQLITE_OK );
    Q_UNUSED( ret );
    //----------------
    QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
  }
  parms_GcpDbData->mDatabaseValid = true;
  return b_database_exists;
}
int QgsSpatiaLiteProviderGcpUtils::getSqliteSequence( GcpDbData* parms_GcpDbData, QString s_tablename )
{
  int id_sequence = SQLITE_ERROR;
  if ( parms_GcpDbData )
  { // check if this is active
    if ( parms_GcpDbData->db_handle )
    {
      sqlite3_stmt* stmt;
      QString s_sql = QString( "SELECT seq FROM sqlite_sequence WHERE (name='%1')" ).arg( s_tablename );
      int ret = sqlite3_prepare_v2( parms_GcpDbData->db_handle, s_sql.toUtf8().constData(), -1, &stmt, NULL );
      if ( ret != SQLITE_OK )
      {
        parms_GcpDbData->mError = QString( "Unable to retrieve seq FROM sqlite_sequence for '%4' : rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( parms_GcpDbData->db_handle ) ) ).arg( s_sql ).arg( s_tablename );
        qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::getSqliteSequence -sqlite_sequence- [%1] " ).arg( parms_GcpDbData->mError );
        return id_sequence;
      }
      while ( sqlite3_step( stmt ) == SQLITE_ROW )
      {
        if ( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL )
          id_sequence = sqlite3_column_int( stmt, 0 );
      }
      sqlite3_finalize( stmt );
    }
  }
  return id_sequence;
}
int QgsSpatiaLiteProviderGcpUtils::spatialiteInitEx( GcpDbData* parms_GcpDbData, QString s_database_filename )
{
  int i_rc = SQLITE_ERROR;
  if ( parms_GcpDbData )
  { // check if this is in use with the same file
    if ( parms_GcpDbData->db_handle )
    {
      if ( parms_GcpDbData->mUsed_database_filename == s_database_filename )
      { // this is in use with the same file in a called function, do no reopen and close
        i_rc = SQLITE_OK;
        parms_GcpDbData->mUsed_database_filename_count++;
      }
      else
      { // this is in use with another file
        return SQLITE_MISUSE;
      }
    }
    if ( i_rc == SQLITE_ERROR )
    {
      i_rc = sqlite3_open_v2( s_database_filename.toUtf8().data(), &parms_GcpDbData->db_handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL );
      if ( i_rc == SQLITE_OK )
      { // needed by spatialite_init_ex (multi-threading)
        parms_GcpDbData->spatialite_cache = spatialite_alloc_connection();
        // load spatialite extension
        spatialite_init_ex( parms_GcpDbData->db_handle, parms_GcpDbData->spatialite_cache, 0 );
        parms_GcpDbData->mUsed_database_filename = s_database_filename;
        parms_GcpDbData->mUsed_database_filename_count = 1;
      }
    }
  }
  return i_rc;
}
bool QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( GcpDbData* parms_GcpDbData )
{
  bool b_rc = false;
  if ( parms_GcpDbData )
  {
    if ( parms_GcpDbData->db_handle )
    {
      if ( parms_GcpDbData->mUsed_database_filename_count == 1 )
      {
        sqlite3_close( parms_GcpDbData->db_handle );
        parms_GcpDbData->mUsed_database_filename_count = 0;
        parms_GcpDbData->mUsed_database_filename = QString::null;
        parms_GcpDbData->db_handle = nullptr;
        if ( parms_GcpDbData->spatialite_cache != nullptr )
        {
          spatialite_cleanup_ex( parms_GcpDbData->spatialite_cache );
          parms_GcpDbData->spatialite_cache = nullptr;
          spatialite_shutdown();
        }
      }
      else
      { // this is in use with the same file in a called function, do no reopen and close
        parms_GcpDbData->mUsed_database_filename_count--;
      }
    }
    b_rc = true;
  }
  return b_rc;
}
double QgsSpatiaLiteProviderGcpUtils::metersToMapPoint( GcpDbData* parms_GcpDbData )
{
  int i_isDegree = 0;
  QString s_UnitName = "";
  //----
  QString  s_database_filename = parms_GcpDbData->mGcpDatabaseFileName;
  bool b_database_exists = QFile( s_database_filename ).exists();
  sqlite3* db_handle;
  char *errMsg = nullptr;
  // open database
  // Note: this will not repopen the connection when called from another function such as createGcpDb
  int ret = QgsSpatiaLiteProviderGcpUtils::spatialiteInitEx( parms_GcpDbData, s_database_filename );
  if ( ret != SQLITE_OK )
  {
    parms_GcpDbData->mError = QString( "Failed to open database: rc=%1 database[%2]" ).arg( ret ).arg( s_database_filename );
    if ( ret != SQLITE_MISUSE )
    { // 'SQLITE_MISUSE': in use with another file, do not close
      QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
    }
    return false;
  }
  if ( parms_GcpDbData )
  {
    if ( parms_GcpDbData->db_handle )
    {
      sqlite3_stmt* stmt;
      QString s_sql = QString( "SELECT SridIsGeographic(%1), SridGetUnit(%1);" ).arg( parms_GcpDbData->mGcpSrid );
      // SELECT SridIsGeographic(3068), SridGetUnit(3068),SridIsGeographic(4326), SridGetUnit(4326); : 0 metre 1 degree
      int ret = sqlite3_prepare_v2( parms_GcpDbData->db_handle, s_sql.toUtf8().constData(), -1, &stmt, NULL );
      if ( ret != SQLITE_OK )
      {
        parms_GcpDbData->mError = QString( "Unable to retrieve SridIsGeographic(%4), SridGetUnit(%4) : rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( parms_GcpDbData->db_handle ) ) ).arg( s_sql ).arg( parms_GcpDbData->mGcpSrid );
        qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::metersToMapPoint -UnitName and Type - [%1] " ).arg( parms_GcpDbData->mError );
        QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
        return parms_GcpDbData->mGcpMasterArea;
      }
      while ( sqlite3_step( stmt ) == SQLITE_ROW )
      {
        if ( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL )
          i_isDegree = sqlite3_column_int( stmt, 0 );
        if ( sqlite3_column_type( stmt, 1 ) != SQLITE_NULL )
          s_UnitName = QString::fromUtf8(( const char * ) sqlite3_column_text( stmt, 1 ) ).toLower();
      }
      sqlite3_finalize( stmt );
      s_sql = "";
      if (( i_isDegree == 0 ) && (( s_UnitName.contains( QString( "metre" ) ) || ( s_UnitName.contains( QString( "meter" ) ) ) ) ) )
      {
        return parms_GcpDbData->mGcpMasterArea;
      }
      else
      {
        QString s_cvt_command = "";
        if ( i_isDegree == 1 )
        { // SELECT ST_Transform(MakePoint(24250.41,21182.19,3068),4326); SRID=4326;POINT(13.3934162138589 52.51751576264239)
          QString s_GeomFromEWKT = QString( "GeomFromEWKT('SRID=%1;%2')" ).arg( parms_GcpDbData->mGcpSrid).arg( parms_GcpDbData->mInputPoint.wellKnownText() );
          // SELECT ST_Transform(MakePoint(13.3934162138589,52.51751576264239,4326),3395)
          // SELECT ST_Length(MakeLine(MakePoint(13.3934162138589,52.51751576264239,4326),ST_Project(MakePoint(13.3934162138589,52.51751576264239,4326),5,PI()/2)))
          // 0.000074
          s_sql = QString( "SELECT ST_Length(MakeLine(%1,ST_Project(%1,%2,PI()/2));" ).arg( s_GeomFromEWKT ).arg( parms_GcpDbData->mGcpMasterArea );
        }
        else
        { // SELECT DISTINCT unit FROM spatial_ref_sys_aux
          if ( s_UnitName.contains( QString( "foot" ) ) )
          {
            s_cvt_command = "CvtToFt";
            if ( s_UnitName.contains( QString( "us" ) ) )
            {
              s_cvt_command = "CvtToUsFt";
            }
            if ( s_UnitName.contains( QString( "indian" ) ) )
            {
              s_cvt_command = "CvtToIndFt";
            }
          }
          if ( s_UnitName.contains( QString( "yard" ) ) )
          {
            s_cvt_command = "CvtToYd";
            if ( s_UnitName.contains( QString( "us" ) ) )
            {
              s_cvt_command = "CvtToUsYd";
            }
            if ( s_UnitName.contains( QString( "indian" ) ) )
            {
              s_cvt_command = "CvtToIndYd";
            }
          }
          if ( s_UnitName.contains( QString( "chain" ) ) )
          {
            s_cvt_command = "CvtToCh";
            if ( s_UnitName.contains( QString( "us" ) ) )
            {
              s_cvt_command = "CvtToUsCh";
            }
            if ( s_UnitName.contains( QString( "indian" ) ) )
            {
              s_cvt_command = "CvtToIndCh";
            }
          }
          if ( s_UnitName.contains( QString( "link" ) ) )
          {
            s_cvt_command = "CvtToLink";
          }
          if ( s_UnitName.contains( QString( "fath" ) ) )
          {
            s_cvt_command = "CvtToFath";
          }
          s_sql = QString( "SELECT %1(%2);" ).arg( s_cvt_command ).arg( parms_GcpDbData->mGcpMasterArea );
        }
        if ( s_sql.isEmpty() )
        {
          return parms_GcpDbData->mGcpMasterArea;
        }
        ret = sqlite3_prepare_v2( parms_GcpDbData->db_handle, s_sql.toUtf8().constData(), -1, &stmt, NULL );
        if ( ret != SQLITE_OK )
        {
          parms_GcpDbData->mError = QString( "Unable to retrieve MapUnits for %4 failed. : rc=%1 [%2] sql[%3]\n" ).arg( ret ).arg( QString::fromUtf8( sqlite3_errmsg( parms_GcpDbData->db_handle ) ) ).arg( s_sql ).arg( parms_GcpDbData->mGcpSrid );
          qDebug() << QString( "QgsSpatiaLiteProviderGcpUtils::metersToMapPoint -UnitName and Type - [%1] " ).arg( parms_GcpDbData->mError );
          QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
          return parms_GcpDbData->mGcpMasterArea;
        }
        while ( sqlite3_step( stmt ) == SQLITE_ROW )
        {
          if ( sqlite3_column_type( stmt, 0 ) != SQLITE_NULL )
            parms_GcpDbData->mGcpMasterArea = sqlite3_column_double( stmt, 0 );
        }
        sqlite3_finalize( stmt );
      }
    }
  }
  //----------------
  QgsSpatiaLiteProviderGcpUtils::spatialiteShutdown( parms_GcpDbData );
  return parms_GcpDbData->mGcpMasterArea;
}
