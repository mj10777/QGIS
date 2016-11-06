---------------
-- sqlite3 srs.db < qgis_srs_epsg_5664_5.sql
---------------
SELECT DateTime('now'),'-begin- INSERT EPSG:5664 + 5665 (DDR-Karten) in QGIS srs.db';
---------------
INSERT INTO tbl_srs
(description,projection_acronym,ellipsoid_acronym,parameters,srid,auth_name,auth_id,is_geo,deprecated)
VALUES('Pulkovo 1942(83) / Gauss-Kruger zone 2 (E-N)','tmerc','krass','+proj=tmerc +lat_0=0 +lon_0=9 +k=1 +x_0=2500000 +y_0=0 +ellps=krass +towgs84=26,-121,-78,0,0,0,0 +units=m +no_defs',5664,'EPSG','5664',0,0);
INSERT INTO tbl_srs
(description,projection_acronym,ellipsoid_acronym,parameters,srid,auth_name,auth_id,is_geo,deprecated)
VALUES('Pulkovo 1942(83) / Gauss-Kruger zone 3 (E-N)','tmerc','krass','+proj=tmerc +lat_0=0 +lon_0=15 +k=1 +x_0=3500000 +y_0=0 +ellps=krass +towgs84=26,-121,-78,0,0,0,0 +units=m +no_defs',5665,'EPSG','5665',0,0);
---------------
SELECT DateTime('now'),'-end- INSERT EPSG:5664 + 5665 (DDR-Karten) in QGIS srs.db';
---------------
