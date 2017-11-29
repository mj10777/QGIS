/***************************************************************************
    qgsappbrowserproviders.h
    -------------------------
    begin                : September 2017
    copyright            : (C) 2017 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSAPPBROWSERPROVIDERS_H
#define QGSAPPBROWSERPROVIDERS_H

#include "qgsdataitemprovider.h"
#include "qgsdataprovider.h"
#include "qgscustomdrophandler.h"

/**
 * Custom data item for QLR files.
 */
class QgsQlrDataItem : public QgsLayerItem
{
    Q_OBJECT

  public:

    QgsQlrDataItem( QgsDataItem *parent, const QString &name, const QString &path );
    bool hasDragEnabled() const override;
    QgsMimeDataUtils::Uri mimeUri() const override;

};

/**
 * Data item provider for showing QLR layer files in the browser.
 */
class QgsQlrDataItemProvider : public QgsDataItemProvider
{
  public:
    QString name() override;
    int capabilities() override;
    QgsDataItem *createDataItem( const QString &path, QgsDataItem *parentItem ) override;
};

/**
 * Handles drag and drop of QLR files to app.
 */
class QgsQlrDropHandler : public QgsCustomDropHandler
{
  public:

    QString customUriProviderKey() const override;
    void handleCustomUriDrop( const QgsMimeDataUtils::Uri &uri ) const override;
};

/**
 * Custom data item for QPT print template files.
 */
class QgsQptDataItem : public QgsDataItem
{
    Q_OBJECT

  public:

    QgsQptDataItem( QgsDataItem *parent, const QString &name, const QString &path );
    bool hasDragEnabled() const override;
    QgsMimeDataUtils::Uri mimeUri() const override;
    bool handleDoubleClick() override;
    QList< QAction * > actions( QWidget *parent ) override;


};

/**
 * Data item provider for showing QPT print templates in the browser.
 */
class QgsQptDataItemProvider : public QgsDataItemProvider
{
  public:
    QString name() override;
    int capabilities() override;
    QgsDataItem *createDataItem( const QString &path, QgsDataItem *parentItem ) override;
};

/**
 * Handles drag and drop of QPT print templates to app.
 */
class QgsQptDropHandler : public QgsCustomDropHandler
{
  public:

    QString customUriProviderKey() const override;
    void handleCustomUriDrop( const QgsMimeDataUtils::Uri &uri ) const override;
    bool handleFileDrop( const QString &file ) override;
};



/**
 * Custom data item for py Python scripts.
 */
class QgsPyDataItem : public QgsDataItem
{
    Q_OBJECT

  public:

    QgsPyDataItem( QgsDataItem *parent, const QString &name, const QString &path );
    bool hasDragEnabled() const override;
    QgsMimeDataUtils::Uri mimeUri() const override;
    bool handleDoubleClick() override;
    QList< QAction * > actions( QWidget *parent ) override;


};

/**
 * Data item provider for showing Python py scripts in the browser.
 */
class QgsPyDataItemProvider : public QgsDataItemProvider
{
  public:
    QString name() override;
    int capabilities() override;
    QgsDataItem *createDataItem( const QString &path, QgsDataItem *parentItem ) override;
};

/**
 * Handles drag and drop of Python py scripts to app.
 */
class QgsPyDropHandler : public QgsCustomDropHandler
{
  public:

    QString customUriProviderKey() const override;
    void handleCustomUriDrop( const QgsMimeDataUtils::Uri &uri ) const override;
    bool handleFileDrop( const QString &file ) override;
};

#endif // QGSAPPBROWSERPROVIDERS_H
