/***************************************************************************
                             qgscustomization.h  - Customization
                             -------------------
    begin                : 2011-04-01
    copyright            : (C) 2011 Radim Blazek
    email                : radim dot blazek at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSGCPCOVERAGES_H
#define QGSGCPCOVERAGES_H

#include "ui_qgsgcpcoveragesdialogbase.h"

#include <QDialog>
#include <QDomNode>
#include <QEvent>
#include <QMouseEvent>
#include <QSettings>
#include <QTreeWidgetItem>

#include "qgsspatialiteprovidergcputils.h"

class QString;
class QWidget;
class QTreeWidgetItem;
class QgsSpatiaLiteProviderGcpUtils;
class QgsGcpCoverages;

class QgsGcpCoveragesDialog : public QMainWindow, private Ui::QgsGcpCoveragesDialogBase
{
    Q_OBJECT
  public:
    QgsGcpCoveragesDialog( QWidget *parent,  QgsSpatiaLiteProviderGcpUtils::GcpDbData *parmsGcpDbData );
    ~QgsGcpCoveragesDialog();
  signals:
    void selectedRasterCoverage( int id_selected_coverage );
  private slots:
    // When Coverage has been selected
    void tree_coverage_clicked( QTreeWidgetItem *item, int column );
    // Close Dialog
    void ok();
    // Load selected coverage, when not alread loaded
    void load_selected_coverage();
    // Display Information for all Coverage
    void on_actionExpandAll_triggered( bool checked );
    // Display only Coverage Name, Title and Abstract
    void on_actionCollapseAll_triggered( bool checked );
  protected:
    // Create all Coverages
    QTreeWidgetItem *createTreeItemCoverages( QgsSpatiaLiteProviderGcpUtils::GcpDbData *parmsGcpDbData );
  private:
    void init();
    // Create all Gcp-Coverage [root]
    QTreeWidgetItem *createTreeItemGcpCoverage();
    // Formatting Tree after adding or rebuilding Coverages
    void init_TreeWidget();
    // Fill specific Coverage, parsing Information of column-Data
    QTreeWidgetItem *createTreeItemCoverage( int mIdGcpCoverage, QStringList sa_fields );
    // Coverage-Information sent by caller
    QgsSpatiaLiteProviderGcpUtils::GcpDbData *mGcpDbData = nullptr;
    // Last selected Coverage
    int i_selected_gcp_coverage;
    QTreeWidgetItem *item_selected_gcp_coverage = nullptr;
    QString  s_selected_gcp_coverage_text;
    QString mLastDirSettingsName;
    friend class QgsGcpCoverages; // in order to access call TreeItemCoverages
};

class QgsGcpCoverages : public QObject
{
    Q_OBJECT
  public:
    //! Returns the instance pointer, creating the object on the first call
    static QgsGcpCoverages *instance();
    void openDialog( QWidget *parent,  QgsSpatiaLiteProviderGcpUtils::GcpDbData *parmsGcpDbData );
  public slots:
    // Load selected coverage, when not alread loaded
    void load_selected_coverage( int id_selected_coverage );
  signals:
    void loadRasterCoverage( int id_selected_coverage );
  protected:
    QgsGcpCoverages();
    ~QgsGcpCoverages();
    QgsGcpCoveragesDialog *pDialog = nullptr;
    int mIdGcpCoverage = -1;
    QgsSpatiaLiteProviderGcpUtils::GcpDbData *mGcpDbData = nullptr;
  private slots:
  private:
    static QgsGcpCoverages *pinstance;
};
#endif // QGSGCPCOVERAGES_H

