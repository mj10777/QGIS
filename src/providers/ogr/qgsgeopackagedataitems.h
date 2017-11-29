/***************************************************************************
    qgsgeopackagedataitems.h
    ---------------------
    begin                : August 2017
    copyright            : (C) 2017 by Alessandro Pasotti
    email                : apasotti at boundlessgeo dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSGEOPACKAGEDATAITEMS_H
#define QGSGEOPACKAGEDATAITEMS_H

#include "qgsdataitem.h"
#include "qgsdataitemprovider.h"
#include "qgsdataprovider.h"
#include "qgstaskmanager.h"


/**
 * \brief The QgsGeoPackageAbstractLayerItem class is the base class for GeoPackage raster and vector layers
 */
class QgsGeoPackageAbstractLayerItem : public QgsLayerItem
{
    Q_OBJECT

  protected:
    QgsGeoPackageAbstractLayerItem( QgsDataItem *parent, const QString &name, const QString &path, const QString &uri, LayerType layerType, const QString &providerKey );

    /**
     * Subclasses need to implement this function with
     * the real deletion implementation
     */
    virtual bool executeDeleteLayer( QString &errCause );
#ifdef HAVE_GUI
    QList<QAction *> actions();
  public slots:
    virtual void deleteLayer();
#endif
};


class QgsGeoPackageRasterLayerItem : public QgsGeoPackageAbstractLayerItem
{
    Q_OBJECT

  public:
    QgsGeoPackageRasterLayerItem( QgsDataItem *parent, const QString &name, const QString &path, const QString &uri );
  protected:
    virtual bool executeDeleteLayer( QString &errCause ) override;
};


class QgsGeoPackageVectorLayerItem : public QgsGeoPackageAbstractLayerItem
{
    Q_OBJECT

  public:
    QgsGeoPackageVectorLayerItem( QgsDataItem *parent, const QString &name, const QString &path, const QString &uri, LayerType layerType );
  protected:
    virtual bool executeDeleteLayer( QString &errCause ) override;
};


/**
 * \brief The QgsGeoPackageCollectionItem class is the base class for
 *        GeoPackage container
 */
class QgsGeoPackageCollectionItem : public QgsDataCollectionItem
{
    Q_OBJECT

  public:
    QgsGeoPackageCollectionItem( QgsDataItem *parent, const QString &name, const QString &path );
    QVector<QgsDataItem *> createChildren() override;
    virtual bool equal( const QgsDataItem *other ) override;

#ifdef HAVE_GUI
    virtual bool acceptDrop() override { return true; }
    virtual bool handleDrop( const QMimeData *data, Qt::DropAction action ) override;
    QList<QAction *> actions( QWidget *parent ) override;
#endif

    //! Return the layer type from \a geometryType
    static QgsLayerItem::LayerType layerTypeFromDb( const QString &geometryType );

    //! Delete a geopackage layer
    static bool deleteGeoPackageRasterLayer( const QString &uri, QString &errCause );

  public slots:
#ifdef HAVE_GUI
    void addTable();
    void addConnection();
    void deleteConnection();
#endif

  protected:
    QString mPath;
};


/**
 * \brief The QgsGeoPackageConnectionItem class adds the stored
 *        connection management to QgsGeoPackageCollectionItem
 */
class QgsGeoPackageConnectionItem : public QgsGeoPackageCollectionItem
{
    Q_OBJECT

  public:
    QgsGeoPackageConnectionItem( QgsDataItem *parent, const QString &name, const QString &path );
    virtual bool equal( const QgsDataItem *other ) override;

#ifdef HAVE_GUI
    QList<QAction *> actions( QWidget *parent ) override;
#endif

  public slots:
#ifdef HAVE_GUI
    void editConnection();
#endif

};


class QgsGeoPackageRootItem : public QgsDataCollectionItem
{
    Q_OBJECT

  public:
    QgsGeoPackageRootItem( QgsDataItem *parent, const QString &name, const QString &path );

    QVector<QgsDataItem *> createChildren() override;

#ifdef HAVE_GUI
    virtual QWidget *paramWidget() override;
    QList<QAction *> actions( QWidget *parent ) override;

  public slots:
    void newConnection();
    void onConnectionsChanged();
    void createDatabase();
#endif
};


//! Provider for geopackage data item
class QgsGeoPackageDataItemProvider : public QgsDataItemProvider
{
  public:
    virtual QString name() override { return QStringLiteral( "GPKG" ); }
    virtual int capabilities() override { return QgsDataProvider::Database; }
    virtual QgsDataItem *createDataItem( const QString &path, QgsDataItem *parentItem ) override;
};


/**
 * \brief The QgsConcurrentFileWriterImportTask class is the parent task for
 * importing layers from a drag and drop operation in the browser.
 * Individual layers need to be added as individual substask.
 */
class QgsConcurrentFileWriterImportTask : public QgsTask
{
    Q_OBJECT

  public:

    QgsConcurrentFileWriterImportTask( const QString &desc = QString() ) : QgsTask( desc ) {}

    void emitProgressChanged( double progress ) { setProgress( progress ); }


  protected:

    bool run() override
    {
      return true;
    }

};

#endif // QGSGEOPACKAGEDATAITEMS_H
