/*
    Copyright (C) 2010 C-41 Bytes <contact@c41bytes.com>

    This file is part of AnalogExif.

    AnalogExif is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    AnalogExif is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with AnalogExif.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ANALOGEXIF_H
#define ANALOGEXIF_H

#include <QMainWindow>
#include <QFileSystemModel>
#include <QPixmap>
#include <QSqlDatabase>
#include <QSettings>
#include <QCloseEvent>
#include <QCompleter>
#include <QMessageBox>
#include <QNetworkReply>

// digiKam includes

#include "dplugin.h"

#ifdef Q_WS_MAC
#include "ui_analogexif_mac.h"
#else
#include "ui_analogexif.h"
#endif

#include "dirsortfilterproxymodel.h"
#include "exiftreemodel.h"
#include "exifitemdelegate.h"
#include "gearlistmodel.h"
#include "geartreemodel.h"
#include "onlineversionchecker.h"

using namespace Digikam;

class AnalogExif : public QMainWindow
{
    Q_OBJECT

public:

    explicit AnalogExif(DPlugin* const tool);
    ~AnalogExif();

    bool initialize();

private:

    DPlugin* m_tool;

    Ui::AnalogExifClass ui;
    QSettings settings;

    // directory model
    QFileSystemModel* fileViewModel;
    QFileSystemModel* dirViewModel;
    // pixmap to hold file preview
    QPixmap filePreviewPixmap;
    // custom directory sorter
    DirSortFilterProxyModel* fileSorter;
    DirSortFilterProxyModel* dirSorter;
    // current filename
    QString curFileName;

    QCompleter dirCompleter;

    // Exif metadata tree model
    ExifTreeModel* exifTreeModel;

    // custom item editor
    ExifItemDelegate* exifItemDelegate;

    // film list model
    GearListModel* filmsList;

    // authors list model
    GearListModel* authorsList;

    // developers list model
    GearListModel* developersList;

    // equipment list model
    GearTreeModel* gearList;

    // database
    QSqlDatabase db;

    // current version of the database
    static const int dbVersion = 1;

    // onResize
    virtual void resizeEvent(QResizeEvent * event);

    // preview file index
    QModelIndex previewIndex;
    QModelIndex curDirIndex;

    // setup tree view
    void setupTreeView();

    // data is dirty
    void setDirty(bool isDirty)
    {
        if(previewIndex != QModelIndex())
        {
            dirty = isDirty;
            setWindowModified(isDirty);

            ui.applyChangesBtn->setEnabled(isDirty);
            ui.revertBtn->setEnabled(isDirty);
            ui.action_Undo->setEnabled(isDirty);
        }
        else
        {
            ui.revertBtn->setEnabled(false);
            ui.action_Undo->setEnabled(false);
        }

        ui.action_Save->setEnabled(isDirty);
        ui.applyChangesBtn->setEnabled(isDirty);
    }

    bool dirty;

    // create backup
    bool createBackup(QString filename, bool singleFile, QMessageBox::StandardButton& prevResult);

    // save data
    bool save();

    // open database
    bool open(QString name);

    // create new database
    QString createLibrary(QWidget* parent = 0, QString dir = QString());

    // background preview loader
    void loadPreview(QString filename);

    // open specified location
    void openLocation(QString path);

    // apply film settings
    void selectFilm(const QModelIndex& index);
    // apply gear settings
    void selectGear(const QModelIndex& index);
    // apply author settings
    void selectAuthor(const QModelIndex& index);
    // apply developer settings
    void selectDeveloper(const QModelIndex& index);

    // query user if dirty model, and save if asked
    bool checkForDirty();

    // open the file in the shell
    void openExternal(const QModelIndex& index);

    // async get file list
    QStringList getFileList(QModelIndexList selIdx, bool includeDirs = false, bool* cancelled = 0);

    void addFileNames(QStringList& fileNames, const QString& path, bool includeDirs = false);
    QStringList scanSubfolders(QModelIndexList selIdx, bool includeDirs = false);
    int filesFound;

    OnlineVersionChecker* verChecker;
    QMessageBox progressBox;

    static const QUrl helpUrl;

signals:

    void updatePreview();

private slots:

    // Apply changes clicked
    void on_applyChangesBtn_clicked();
    // Revert changes clicked
    void on_revertBtn_clicked();
    // equipment selected and applied
    void on_gearView_doubleClicked(const QModelIndex& index);
    // film selected and applied
    void on_filmView_doubleClicked(const QModelIndex& index);
    // author selected and applied
    void on_authorView_doubleClicked(const QModelIndex& index);
    // developer selected and applied
    void on_developerView_doubleClicked(const QModelIndex& index);
    // Apply action
    void on_actionApply_gear_triggered(bool checked = false);
    // Edit gear
    void on_actionEdit_gear_triggered(bool checked = false);
    // film selected
    void filmAndGearView_selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    // changed data in the model
    void modelDataChanged(const QModelIndex&, const QModelIndex&);
    // on close
    void closeEvent(QCloseEvent *event);
    // on options
    void on_actionPreferences_triggered(bool checked = false);
    // on return pressed while at directory line
    void on_directoryLine_returnPressed();
    // on open file
    void on_actionOpen_triggered(bool checked = false);
    // on clear tag value
    void on_action_Clear_tag_value_triggered(bool checked = false);
    // metadata tag selection changed
    void metadataView_selectionChanged(const QItemSelection&, const QItemSelection&);
    // open new gear database
    void on_actionOpen_library_triggered(bool checked = false);
    // create new gear database
    void on_actionNew_library_triggered(bool checked = false);
    // file browser selection changed
    void fileView_selectionChanged(const QItemSelection&, const QItemSelection&);
    void dirView_selectionChanged(const QItemSelection&, const QItemSelection&);
    // file and dir browser clicked
    void on_fileView_clicked(const QModelIndex& index);
    void on_dirView_clicked(const QModelIndex& index);
    // auto-fill exposure numbers
    void on_actionAuto_fill_exposure_triggered(bool checked = false);
    // double-click on file
    void on_fileView_doubleClicked(const QModelIndex& index);
    // open external
    void on_actionOpen_external_triggered(bool checked = false);
    // rename
    void on_actionRename_triggered(bool checked = false);
    // remove
    void on_actionRemove_triggered(bool checked = false);
    // copy metadata
    void on_action_Copy_metadata_triggered(bool checked = false);
    // about dialog
    void on_action_About_triggered(bool checked = false);
    // help
    void on_actionHelp_triggered(bool checked = false);

    // on update preview
    void previewUpdate();

    // new version available
    void newVersionAvailable(QString selfTag, QString newTag, QDateTime newTime, QString newSummary);
    void newVersionCheckError(QNetworkReply::NetworkError error);

    // cancel version check
    void cancelVersionCheck();

    // scroll to the selected directory
    void scrollToSelectedDir();
};

#endif // ANALOGEXIF_H
