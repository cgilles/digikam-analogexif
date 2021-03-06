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

#include "analogexif.h"

// Qt includes

#include <QStringList>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QtConcurrentRun>
#include <QFileDialog>
#include <QTime>
#include <QDesktopServices>
#include <QUrl>
#include <QSysInfo>
#include <QTimer>
#include <QImageReader>

// digiKam includes

#include "dpluginaboutdlg.h"

// Local includes

#include "editgear.h"
#include "analogexifoptions.h"
#include "autofillexpnum.h"
#include "progressdialog.h"
#include "copymetadatadialog.h"

const QUrl AnalogExif::helpUrl("http://analogexif.sourceforge.net/help/");

AnalogExif::AnalogExif(DPluginGeneric* const tool, DInfoInterface* const iface)
    : QMainWindow(nullptr),
      m_tool(tool),
      m_iface(iface),
      m_fileIconProvider(nullptr)
{
    ui.setupUi(this);

    /// setup tree view
    dirViewModel = new QFileSystemModel(this);
    dirViewModel->setFilter(QDir::AllDirs | QDir::Drives | QDir::NoDotAndDotDot);
    dirViewModel->setReadOnly(false);
    dirSorter = new DirSortFilterProxyModel(this);

    ui.dirView->setModel(dirSorter);
    // resize Name and Size column
    ui.dirView->setColumnWidth(0, 150);
    ui.dirView->setColumnWidth(1, 50);
    // sort by filename
    ui.dirView->setSortingEnabled(true);
    ui.dirView->sortByColumn(0, Qt::AscendingOrder);

    fileViewModel = new QFileSystemModel(this);
    fileViewModel->setFilter(QDir::Files);
    // show supported files only
    fileViewModel->setNameFilterDisables(false);
    fileViewModel->setNameFilters(QStringList() << "*.jpg" << "*.jpeg" << "*.jpe" << "*.tif" << "*.tiff" << "*.dng" << "*.jp2" << "*.jpf" << "*.jpx" << "*.j2k" << "*.j2c" << "*.jpc" << "*.psd");
    fileViewModel->setReadOnly(false);
    fileSorter = new DirSortFilterProxyModel(this);
    fileSorter->setSourceModel(fileViewModel);

    ui.fileView->setModel(fileSorter);

    // set file preview
    // filePreviewPixmap = new QPixmap();
    ui.filePreview->setPixmap(filePreviewPixmap);

    exifTreeModel  = nullptr;
    filmsList      = nullptr;
    gearList       = nullptr;
    authorsList    = nullptr;
    developersList = nullptr;

    // set context menus
    QList<QAction*> contextMenus;
    QAction* const separator = new QAction(this);
    separator->setSeparator(true);

    contextMenus << ui.actionApply_gear << separator << ui.action_Undo << ui.action_Save << separator << ui.actionEdit_gear;
    ui.filmView->addActions(contextMenus);
    ui.gearView->addActions(contextMenus);
    ui.authorView->addActions(contextMenus);
    ui.developerView->addActions(contextMenus);

    contextMenus.clear();

    contextMenus << ui.action_Clear_tag_value << separator << ui.action_Undo << ui.action_Save;
    ui.metadataView->addActions(contextMenus);

    dirty = false;
    setDirty(false);

    // read previous window state
    if(settings.contains("WindowState")){
        restoreState(settings.value("WindowState").toByteArray());
    }

    if(settings.contains("WindowGeometry")){
        restoreGeometry(settings.value("WindowGeometry").toByteArray());
    }

    setWindowTitle(QCoreApplication::applicationName());

#ifndef Q_WS_MAC

    // connect preview updates
    connect(this, SIGNAL(updatePreview()), this,
            SLOT(previewUpdate()), Qt::BlockingQueuedConnection);

#endif

    contextMenus.clear();

    contextMenus << ui.actionAuto_fill_exposure << ui.action_Copy_metadata << separator << ui.actionOpen_external << ui.actionRename << separator << ui.actionRemove;
    ui.fileView->addActions(contextMenus);
    ui.dirView->addActions(contextMenus);

#ifdef Q_WS_WIN

    if (QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA)
    {
        // For Vista/W7 style - disable alternating row colors on equipment views for better visibility
        ui.gearView->setAlternatingRowColors(false);
        ui.filmView->setAlternatingRowColors(false);
        ui.developerView->setAlternatingRowColors(false);
        ui.authorView->setAlternatingRowColors(false);
    }

#endif

}

AnalogExif::~AnalogExif()
{
    if(exifTreeModel)
        delete exifTreeModel;

    if(filmsList)
        delete filmsList;

    if(gearList)
        delete gearList;

    if(authorsList)
        delete authorsList;

    if(developersList)
        delete developersList;

    delete dirSorter;
    delete dirViewModel;
    delete fileSorter;
    delete fileViewModel;
    delete m_fileIconProvider;

    // delete filePreviewPixmap;exifTreeModel
}

// perform all initialization
bool AnalogExif::initialize()
{
    // open database
    QString dbName = settings.value("dbName", "AnalogExif.ael").toString();

    // check for the database to exist
    while(!QFile::exists(dbName))
    {
        QMessageBox msgBox(QMessageBox::Critical, tr("Open library error"),
            tr("Unable to find previously used library file:\n\n") + dbName +
            tr("\n\nDo you want to open existing library file or create new library?"));

        if(dbName.isNull())
        {
            msgBox.setText(tr("Unable to find previously used library file") +
            tr("\n\nDo you want to open existing library file or create new library?"));
        }

        QPushButton* openBtn = msgBox.addButton(tr("Open..."), QMessageBox::ActionRole);
        QPushButton* newBtn = msgBox.addButton(tr("New..."), QMessageBox::ActionRole);
                msgBox.addButton(QMessageBox::Retry);
        QPushButton* cancelBtn = msgBox.addButton(QMessageBox::Cancel);

        msgBox.setWindowIcon(this->windowIcon());

        msgBox.exec();

        if      (msgBox.clickedButton() == cancelBtn)
        {
            return false;
        }
        else if (msgBox.clickedButton() == openBtn)
        {
            dbName = QFileDialog::getOpenFileName(nullptr, tr("Open equipment library"), QString(), tr("AnalogExif library files (*.ael);;All files (*.*)"));
        }
        else if (msgBox.clickedButton() == newBtn)
        {
            dbName = createLibrary();
        }
    }

    db = QSqlDatabase::addDatabase("QSQLITE");

    if(!open(dbName))
        return false;

    // set exif metadata model
    exifTreeModel      = new ExifTreeModel(this);
    exifItemDelegate   = new ExifItemDelegate(this);
    m_fileIconProvider = new FileIconProvider(exifTreeModel);
    fileViewModel->setIconProvider(m_fileIconProvider);
    
    ui.metadataView->setModel(exifTreeModel);
    ui.metadataView->setItemDelegateForColumn(1, exifItemDelegate);

    // connect to data changed signal
    connect(exifTreeModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
            this, SLOT(modelDataChanged(const QModelIndex&, const QModelIndex&)));

    connect(ui.metadataView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(metadataView_selectionChanged(const QItemSelection&, const QItemSelection&)));

    // span the categories to full row
    setupTreeView();

    // films view
    filmsList = new GearListModel(this, 2, tr("No film defined"));
    filmsList->reload();

    ui.filmView->setModel(filmsList);
    connect(ui.filmView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(filmAndGearView_selectionChanged(const QItemSelection&, const QItemSelection&)));

    // authors view
    authorsList = new GearListModel(this, 4, tr("No authors defined"));
    authorsList->reload();

    ui.authorView->setModel(authorsList);
    connect(ui.authorView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(filmAndGearView_selectionChanged(const QItemSelection&, const QItemSelection&)));

    // developers view
    developersList = new GearListModel(this, 3, tr("No developers defined"));
    developersList->reload();

    ui.developerView->setModel(developersList);
    connect(ui.developerView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(filmAndGearView_selectionChanged(const QItemSelection&, const QItemSelection&)));

    // gear view
    gearList = new GearTreeModel(this);
    gearList->reload();
    ui.gearView->setModel(gearList);
    connect(ui.gearView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(filmAndGearView_selectionChanged(const QItemSelection&, const QItemSelection&)));

    filmsList->setApplicable(true);
    gearList->setApplicable(true);
    developersList->setApplicable(true);
    authorsList->setApplicable(true);

    if (gearList->bodyCount() == 0)
        ui.gearView->setRootIsDecorated(false);

    ui.gearView->expandAll();

    // start scan
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    dirViewModel->setRootPath(QDir::rootPath());
    QApplication::restoreOverrideCursor();

    dirSorter->setSourceModel(dirViewModel);

    for(int i = 0; i < dirSorter->columnCount(); i++)
        ui.dirView->hideColumn(i);

    ui.dirView->showColumn(0);

    connect(ui.dirView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(dirView_selectionChanged(const QItemSelection&, const QItemSelection&)));

    connect(ui.fileView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(fileView_selectionChanged(const QItemSelection&, const QItemSelection&)));

    QString currFolder = QDir::homePath();

    if (m_iface)
    {
        QList<QUrl> urls = m_iface->currentAlbumItems();

        if (!urls.isEmpty())
        {
            currFolder = urls.first().adjusted(QUrl::RemoveFilename).toLocalFile();
        }
    }

    curDirIndex = dirSorter->mapFromSource(dirViewModel->index(currFolder));

    if (curDirIndex != QModelIndex())
    {
        ui.directoryLine->setText(QDir::toNativeSeparators(currFolder));
        ui.dirView->setCurrentIndex(curDirIndex);
        ui.dirView->setExpanded(curDirIndex, true);

#ifndef Q_WS_MAC
        QTimer::singleShot(200, this, SLOT(scrollToSelectedDir()));
#endif

        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        fileViewModel->setRootPath(currFolder);
        QApplication::restoreOverrideCursor();

        fileSorter->setSourceModel(fileViewModel);
        ui.fileView->setRootIndex(fileSorter->mapFromSource(fileViewModel->index(currFolder)));
    }

    return true;
}

void AnalogExif::scrollToSelectedDir()
{
    if(curDirIndex.isValid())
        ui.dirView->scrollTo(curDirIndex, QAbstractItemView::PositionAtCenter);
}

void AnalogExif::dirView_selectionChanged(const QItemSelection& selected, const QItemSelection&)
{
    // map selection to original
    QModelIndexList selIdx;

    if(selected.count())
    {
        int prevRow = -1;
        foreach(QModelIndex idx, selected.indexes())
        {
            if(idx.row() != prevRow)
            {
                selIdx.append(idx);
                prevRow = idx.row();
            }
        }
    }

    // selIdx = ui.dirView->selectionModel()->selectedRows();

    if(selIdx.count() == 0)
    {
        curDirIndex = QModelIndex();
        return;
    }

    if(dirty)
    {
        QMessageBox::StandardButton result = QMessageBox::question(this, tr("Unsaved data"),
                                                            tr("Save changes in the current image?"),
                                                            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                                            QMessageBox::Cancel);

        if(result == QMessageBox::Cancel)
        {
            // select previous file
            ui.dirView->selectionModel()->select(curDirIndex, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
            ui.dirView->selectionModel()->setCurrentIndex(curDirIndex, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);

            return;
        }

        if(result == QMessageBox::Save)
            if(!save())
                return;

        setDirty(false);
    }

    setDirty(false);

    if(selIdx.count() == 1)
    {
        // selected single folder - reflect this in the file view
        curDirIndex = selIdx.at(0);

        if(curDirIndex != QModelIndex())
        {
            QString selFolderName = dirViewModel->filePath(dirSorter->mapToSource(curDirIndex));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            fileViewModel->setFilter(0);
            ui.fileView->setRootIndex(fileSorter->mapFromSource(fileViewModel->setRootPath(selFolderName)));
            fileViewModel->setFilter(QDir::Files);
            QApplication::restoreOverrideCursor();

            ui.directoryLine->setText(QDir::toNativeSeparators(selFolderName));
        }

        ui.dirView->setCurrentIndex(curDirIndex);
        ui.dirView->setExpanded(curDirIndex, true);
    }
    else
    {
        // multiple selection, clear files view
        fileViewModel->setFilter(0);
    }

    // enable copy metadata if anything selected
    // BUG/TODO: should be disabled when no files found
    ui.action_Copy_metadata->setEnabled(true);

    // enable auto-fill exposure numbers for several files or directory(ies)
    ui.actionAuto_fill_exposure->setEnabled(true);

    ui.actionOpen_external->setEnabled(false);

    // clear file preview
    ui.filePreview->setPixmap(QPixmap());

    // clear metatags
    exifTreeModel->clear(true);

    // enable editing
    exifTreeModel->setReadonly(false);  

    // expand and fix the metadata tree view
    setupTreeView();

    // clear filename from the window title
    setWindowTitle(QCoreApplication::applicationName());
    setWindowModified(false);

    // clear current file name
    curFileName = "";
    previewIndex = QModelIndex();
}

// changed selection of the file browser
void AnalogExif::fileView_selectionChanged(const QItemSelection&, const QItemSelection&)
{
    // map selection to original
    QModelIndexList selIdx = ui.fileView->selectionModel()->selectedRows();

    if(dirty && (previewIndex != QModelIndex()))
    {
        if((selIdx.count() == 1) && (previewIndex != selIdx.at(0)))
        {
            QMessageBox::StandardButton result = QMessageBox::question(this, tr("Unsaved data"),
                                                                tr("Save changes in the current image?"),
                                                                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                                                QMessageBox::Cancel);

            if(result == QMessageBox::Cancel)
            {
                // select previous file
                ui.fileView->selectionModel()->select(previewIndex, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
                ui.fileView->selectionModel()->setCurrentIndex(previewIndex, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);

                return;
            }

            if(result == QMessageBox::Save)
                if(!save())
                    return;

            setDirty(false);
        }
        else if ((selIdx.count() == 1) && (previewIndex == selIdx.at(0)))
        {
            // ignore back select
            return;
        }
    }

    setDirty(false);

    // enable copy metadata if anything selected
    if(selIdx.count())
    {
        ui.action_Copy_metadata->setEnabled(true);
    }
    else if(!ui.dirView->selectionModel()->hasSelection())
    {
        ui.action_Copy_metadata->setEnabled(false);
    }

    // selected single item
    if(selIdx.count() == 1)
    {
        QModelIndex index = fileSorter->mapToSource(selIdx.at(0));

        // check whether it is file
        if(!fileViewModel->isDir(index))
        {
            QString path = fileViewModel->filePath(index);

            // load metadata
            if(!exifTreeModel->openFile(QDir::toNativeSeparators(path)))
            {
                QMessageBox::critical(this, tr("Read file error"), tr("Unable to load metadata from %1.").arg(QDir::toNativeSeparators(path)));
                ui.fileView->clearSelection();
                exifTreeModel->setReadonly();

                return;
            }

            setupTreeView();

            setWindowTitle("");
            setWindowFilePath(fileViewModel->fileName(index));

            setWindowModified(false);

            ui.actionAuto_fill_exposure->setEnabled(false);
            ui.actionOpen_external->setEnabled(true);

            curFileName = path;
            ui.directoryLine->setText(QDir::toNativeSeparators(fileViewModel->fileInfo(index).absolutePath()));

            // load preview in the background
            ui.filePreview->setPixmap(QPixmap());
#ifdef Q_WS_MAC
            // Background loading doesn't work properly for Mac
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            loadPreview(curFileName);
            QApplication::restoreOverrideCursor();
#else
            QFuture<void> future = QtConcurrent::run(this, &AnalogExif::loadPreview, curFileName);
#endif
            exifTreeModel->setReadonly(false);

            previewIndex = selIdx.at(0);

            return;
        }
    }

    // enable auto-fill exposure numbers for several files or directory(ies)
    if((selIdx.count() == 0) && !ui.dirView->selectionModel()->hasSelection())
    {
        ui.actionAuto_fill_exposure->setEnabled(false);
    }
    else
    {
        ui.actionAuto_fill_exposure->setEnabled(true);
    }

    ui.actionOpen_external->setEnabled(false);

    // clear file preview
    ui.filePreview->setPixmap(QPixmap());

    // clear metatags
    exifTreeModel->clear(true);

    // enable editing
    exifTreeModel->setReadonly(false);  

    // expand and fix the metadata tree view
    setupTreeView();

    // clear filename from the window title
    setWindowTitle(QCoreApplication::applicationName());
    setWindowModified(false);

    // clear current file name
    curFileName = "";
    previewIndex = QModelIndex();

}

void AnalogExif::on_fileView_clicked(const QModelIndex&)
{
    ui.dirView->selectionModel()->clearSelection();
}

void AnalogExif::on_dirView_clicked(const QModelIndex& index)
{
    ui.fileView->selectionModel()->clearSelection();

    if(index.isValid())
    {
        // update directory line
        ui.directoryLine->setText(QDir::toNativeSeparators(dirViewModel->filePath(dirSorter->mapToSource(index))));
    }
}

// open file for editing
void AnalogExif::openLocation(QString path)
{
    // if valid selection - enable equipment apply
    bool validSelection = ui.gearView->selectionModel()->hasSelection() | ui.filmView->selectionModel()->hasSelection() | ui.developerView->selectionModel()->hasSelection() | ui.authorView->selectionModel()->hasSelection();
    ui.actionApply_gear->setEnabled(validSelection);
    ui.applyGearBtn->setEnabled(validSelection);

    QFileInfo fileInfo(path);

    if(fileInfo.isDir())
    {
        // update the directory line in case directory selected
        ui.directoryLine->setText(QDir::toNativeSeparators(path));

        // clear file preview
        ui.filePreview->setPixmap(QPixmap());

        // clear metatags
        exifTreeModel->clear(true);

        setWindowTitle(QCoreApplication::applicationName());
        setWindowModified(false);

        // clear current file name
        curFileName = "";

        // current directory index
        curDirIndex = dirSorter->mapFromSource(dirViewModel->index(fileInfo.filePath()));
    }
    else
    {
        // load metadata
        if(!exifTreeModel->openFile(QDir::toNativeSeparators(path)))
            return;

        setWindowTitle("");
        setWindowFilePath(fileInfo.fileName());

        setWindowModified(false);

        curFileName = path;
        ui.directoryLine->setText(QDir::toNativeSeparators(fileInfo.absolutePath()));

        // load preview in the background
        ui.filePreview->setPixmap(QPixmap());
#ifdef Q_WS_MAC
        // Background loading doesn't work properly for Mac
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        loadPreview(curFileName);
        QApplication::restoreOverrideCursor();
#else
        QFuture<void> future = QtConcurrent::run(this, &AnalogExif::loadPreview, curFileName);
#endif
        // directory index
        curDirIndex = dirSorter->mapFromSource(dirViewModel->index(fileInfo.path()));
    }

    // scroll to the directory
    ui.dirView->scrollTo(curDirIndex, QAbstractItemView::PositionAtCenter);
    ui.dirView->selectionModel()->select(curDirIndex, QItemSelectionModel::SelectCurrent);
    ui.dirView->setCurrentIndex(curDirIndex);
    ui.dirView->setExpanded(curDirIndex, true);

    ui.gearView->expandAll();

    setDirty(false);

    // setup tree
    setupTreeView();

    // scroll and select
    previewIndex = fileSorter->mapFromSource(fileViewModel->index(path));
    ui.fileView->selectionModel()->select(previewIndex, QItemSelectionModel::SelectCurrent);
    ui.fileView->setCurrentIndex(previewIndex);
    ui.fileView->scrollTo(previewIndex, QAbstractItemView::PositionAtCenter);
}

// background preview loader
void AnalogExif::loadPreview(const QString& filename)
{
    // show file preview and details

    // try to load preview
    QImage preview = exifTreeModel->getPreview(filename);

    if (!preview.isNull())
    {
        filePreviewPixmap = QPixmap::fromImage(preview);
    }
    else
    {
        // check whether image is supported
        QList<QByteArray> supportedImgs = QImageReader::supportedImageFormats();

        if(!supportedImgs.contains(filename.section(".", -1).toLatin1()))
            return;

        // show the full image otherwise
        if(!filePreviewPixmap.load(filename, 0, Qt::ThresholdDither))
            return;
    }

    QSize previewSize = ui.filePreviewGroupBox->contentsRect().size();

    // ui.filePreview->setPixmap(filePreviewPixmap->scaled(previewSize.width()-30, previewSize.height()-30, Qt::KeepAspectRatio));
    filePreviewPixmap = filePreviewPixmap.scaled(previewSize.width()-30, previewSize.height()-30, Qt::KeepAspectRatio);

#ifdef Q_WS_MAC

    // preview is loaded on the same thread under Mac
    previewUpdate();

#else

    emit updatePreview();

#endif
}

void AnalogExif::previewUpdate()
{
    ui.filePreview->setPixmap(filePreviewPixmap);
}

// on main window resize event
void AnalogExif::resizeEvent(QResizeEvent *)
{
    // rescale the pixmap
    if(!ui.filePreview->pixmap()->isNull())
    {
        QSize previewSize = ui.filePreviewGroupBox->contentsRect().size();
        ui.filePreview->setPixmap(filePreviewPixmap.scaled(previewSize.width()-30, previewSize.height()-30, Qt::KeepAspectRatio));
    }
}

// apply changes
void AnalogExif::on_applyChangesBtn_clicked()
{
    save();
}

bool AnalogExif::createBackup(QString filename, bool singleFile, QMessageBox::StandardButton& prevResult)
{
    // save data
    if(settings.value("CreateBackups", true).toBool())
    {
        QMessageBox::StandardButton res = QMessageBox::Yes;
        QMessageBox::StandardButtons btns = QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel;

        if(!singleFile)
        {
            btns |= QMessageBox::YesToAll | QMessageBox::NoToAll;
        }

        // create backup
        if(QFile::exists(filename + ".bak"))
        {
            // skip question if Yes/NoToAll was previously selected
            if((prevResult != QMessageBox::YesToAll) && (prevResult != QMessageBox::NoToAll))
            {
                // otherwise - set previous choice as default
                res = QMessageBox::question(this, tr("Backup file exists"),
                    tr("Backup file %1 already exists.\n\nOverwrite backup file?").arg(QDir::toNativeSeparators(filename + ".bak")),
                    btns, prevResult);
            }
            else
            {
                res = prevResult;
            }

            if(res == QMessageBox::Cancel)
                return false;

            if((res == QMessageBox::Yes) || (res == QMessageBox::YesToAll))
            {
                if(!QFile::remove(filename + ".bak"))
                {
                    QMessageBox::critical(this, tr("Save error"), tr("Unable to remove backup file %1.").arg(QDir::toNativeSeparators(filename + ".bak")));
                    return false;
                }
            }
        }
        if((res == QMessageBox::Yes) || (res == QMessageBox::YesToAll))
        {
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

            QTime timer;
            ProgressDialog progress(tr("Creating backup"), "Please wait...", "", this, 0, 100);
            QFuture<bool> future = QtConcurrent::run(&QFile::copy, filename, filename + ".bak");
            progress.setValue(0);

            while(!future.isFinished())
            {
                if((timer.elapsed() > 500) && (!progress.isVisible()))
                    progress.show();

                progress.setValue(timer.elapsed() / 1000);

                QCoreApplication::processEvents();
                QCoreApplication::sendPostedEvents();
            }

            QApplication::restoreOverrideCursor();

            if(!future)
            {
                QMessageBox::critical(this, tr("Save error"), tr("Unable to create backup file:\n%1.").arg(QDir::toNativeSeparators(filename + ".bak")));
    
                return false;
            }
        }
        prevResult = res;
    }

    return true;
}

bool AnalogExif::save()
{
    // determine the number of selected files
    QModelIndexList selIdx;

    if(ui.fileView->selectionModel()->hasSelection())
    {
        selIdx = ui.fileView->selectionModel()->selectedRows();
    }
    else
    {
        selIdx = ui.dirView->selectionModel()->selectedRows();
    }

    // save backup answer
    QMessageBox::StandardButton saveBkp = QMessageBox::No;
    // single selection
    bool singleFile = false;

    // prepare metadata to save
    if(!exifTreeModel->prepareMetadata())
    {
        QMessageBox::critical(this, tr("Save error"), tr("Unable to prepare metadata for saving."));
        return false;
    }

    // browse through all selected indexes
    QStringList fileNames = getFileList(selIdx);

    if(fileNames.count() < 1)
        return false;

    ProgressDialog progress(tr("Saving metadata..."), "", tr("Cancel"), this, 0, fileNames.count());
    QTime time;
    time.start();

    int filesProcessed = 0;

    if(fileNames.count() == 1)
        singleFile = true;

    foreach(QString fName, fileNames)
    {
        progress.setValue(filesProcessed);
        progress.setLabelText(tr("Saving %1...").arg(fName));

        // check elapsed time, show progress dialog if required
        if((time.elapsed() > 500) && (!progress.isVisible()))
            progress.show();

        // create backup
        if(!createBackup(fName, singleFile, saveBkp))
            return false;

        // save metadata in another thread, do not replace
        QFuture<bool> future = QtConcurrent::run(exifTreeModel, &ExifTreeModel::saveFile, fName, false);

        while(!future.isFinished())
        {
            QCoreApplication::processEvents();
            QCoreApplication::sendPostedEvents();

            if(progress.wasCanceled())
                return false;
        }

        if(!future.result())
        {
            QMessageBox::critical(this, tr("Save error"), tr("Unable to save %1.").arg(QDir::toNativeSeparators(fName)));
            return false;
        }

        filesProcessed++;
    }

    // clear dirty flags
    exifTreeModel->resetDirty();
    setDirty(false);

    // re-setup metadata view
    setupTreeView();

    // deselect applied equipment
    filmsList->setSelectedIndex(QModelIndex());
    gearList->setSelectedIndex(QModelIndex());
    authorsList->setSelectedIndex(QModelIndex());
    developersList->setSelectedIndex(QModelIndex());

    ui.gearView->selectionModel()->clearSelection();
    ui.filmView->selectionModel()->clearSelection();
    ui.developerView->selectionModel()->clearSelection();
    ui.authorView->selectionModel()->clearSelection();

    // focus on metadata view
    ui.metadataView->setFocus(Qt::OtherFocusReason);

    return true;
}

// revert changes
void AnalogExif::on_revertBtn_clicked()
{
    exifTreeModel->reload();
    setDirty(false);
    setupTreeView();

    // clear selection
    filmsList->setSelectedIndex(QModelIndex());
    gearList->setSelectedIndex(QModelIndex());
    authorsList->setSelectedIndex(QModelIndex());
    developersList->setSelectedIndex(QModelIndex());

    ui.gearView->selectionModel()->clearSelection();
    ui.filmView->selectionModel()->clearSelection();
    ui.developerView->selectionModel()->clearSelection();
    ui.authorView->selectionModel()->clearSelection();

    ui.metadataView->setFocus(Qt::OtherFocusReason);
}

// data model changed signal
void AnalogExif::modelDataChanged(const QModelIndex&, const QModelIndex&)
{
    setDirty(true);
}

// set up tree view
void AnalogExif::setupTreeView()
{
    for(int i = 0; i < exifTreeModel->rowCount(); i++)
    {
        ui.metadataView->setFirstColumnSpanned(i, QModelIndex(), true);
    }
    ui.metadataView->header()->resizeSection(0, 200);
    ui.metadataView->expandAll();
}

// close event
void AnalogExif::closeEvent(QCloseEvent *event)
{
    if(!checkForDirty())
    {
        event->ignore();
        return;
    }

    // save required settings
    settings.setValue("dbName", db.databaseName());

    // save window state and geometry
    settings.setValue("WindowState", saveState());
    settings.setValue("WindowGeometry", saveGeometry());

    // sync settings
    settings.sync();

    event->accept();
}

// apply film settings
void AnalogExif::selectFilm(const QModelIndex& index)
{
    if(!index.isValid())
        return;

    QVariant filmData = filmsList->data(index, GearListModel::GetExifData);
    if(filmData != QVariant())
    {
        QVariantList list = filmData.toList();
        exifTreeModel->setValues(list);
    }

    filmsList->setSelectedIndex(index);
}

void AnalogExif::selectGear(const QModelIndex& index)
{
    if(!index.isValid())
        return;

    QVariant gearData = gearList->data(index, GearTreeModel::GetExifData);
    if(gearData != QVariant())
    {
        QVariantList list = gearData.toList();
        exifTreeModel->setValues(list);
    }

    gearList->setSelectedIndex(index);
}

void AnalogExif::selectAuthor(const QModelIndex& index)
{
    if(!index.isValid())
        return;

    QVariant authorData = authorsList->data(index, GearListModel::GetExifData);
    if(authorData != QVariant())
    {
        QVariantList list = authorData.toList();
        exifTreeModel->setValues(list);
    }

    authorsList->setSelectedIndex(index);
}

void AnalogExif::selectDeveloper(const QModelIndex& index)
{
    if(!index.isValid())
        return;

    QVariant devData = developersList->data(index, GearListModel::GetExifData);
    if(devData != QVariant())
    {
        QVariantList list = devData.toList();
        exifTreeModel->setValues(list);
    }

    developersList->setSelectedIndex(index);
}

// film double clicked - apply
void AnalogExif::on_filmView_doubleClicked(const QModelIndex& index)
{
    selectFilm(index);
}

// author double clicked - apply
void AnalogExif::on_authorView_doubleClicked(const QModelIndex& index)
{
    selectAuthor(index);
}

// author double clicked - apply
void AnalogExif::on_developerView_doubleClicked(const QModelIndex& index)
{
    selectDeveloper(index);
}

// film or gear selected - enable "apply gear" button
void AnalogExif::filmAndGearView_selectionChanged(const QItemSelection&, const QItemSelection&)
{
    bool validSelection = ui.gearView->selectionModel()->hasSelection() | ui.filmView->selectionModel()->hasSelection() | ui.developerView->selectionModel()->hasSelection() | ui.authorView->selectionModel()->hasSelection();
    ui.actionApply_gear->setEnabled(validSelection);
    ui.applyGearBtn->setEnabled(validSelection);

    if(!ui.gearView->selectionModel()->hasSelection())
        gearList->setSelectedIndex(QModelIndex());

    if(!ui.filmView->selectionModel()->hasSelection())
        filmsList->setSelectedIndex(QModelIndex());

    if(!ui.developerView->selectionModel()->hasSelection())
        developersList->setSelectedIndex(QModelIndex());

    if(!ui.authorView->selectionModel()->hasSelection())
        authorsList->setSelectedIndex(QModelIndex());
}

// Apply action triggered
void AnalogExif::on_actionApply_gear_triggered(bool)
{
    QModelIndexList curFilm = ui.filmView->selectionModel()->selectedIndexes();
    QModelIndexList curGear = ui.gearView->selectionModel()->selectedIndexes();
    QModelIndexList curAuthor = ui.authorView->selectionModel()->selectedIndexes();
    QModelIndexList curDeveloper = ui.developerView->selectionModel()->selectedIndexes();

    if(curFilm.count())
        selectFilm(curFilm.at(0));

    if(curGear.count())
        selectGear(curGear.at(0));

    if(curAuthor.count())
        selectAuthor(curAuthor.at(0));

    if(curDeveloper.count())
        selectDeveloper(curDeveloper.at(0));
}

// Edit gear action
void AnalogExif::on_actionEdit_gear_triggered(bool)
{
    if(!checkForDirty())
        return;

    // get currently selected filename - indexes may not be valid after changes
    QString curSelected;
    if(ui.fileView->selectionModel()->hasSelection() && (ui.fileView->selectionModel()->selectedRows().count() == 1))
    {
        curSelected = fileViewModel->filePath(fileSorter->mapToSource(ui.fileView->currentIndex()));
    }

    EditGear edit(this);
    edit.setModal(true);

    if(edit.exec())
    {
        // reload equipment
        gearList->reload();
        ui.gearView->expandAll();
        filmsList->reload();
        authorsList->reload();
        developersList->reload();
    }

    // repopulate metadata
    if(!curSelected.isEmpty())
    {
        QModelIndex idx = fileSorter->mapFromSource(fileViewModel->index(curSelected));
        ui.fileView->selectionModel()->clearSelection();
        ui.fileView->setCurrentIndex(QModelIndex());

        if(idx.isValid())
        {
            // reselect and reload
            ui.fileView->selectionModel()->select(idx, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
            ui.fileView->setCurrentIndex(idx);
        }
    }
    else
    {
        // multiple or incorrect selection - clear the metadata view
        exifTreeModel->reload();
        ui.metadataView->expandAll();
    }
}

// Preferences
void AnalogExif::on_actionPreferences_triggered(bool)
{
    if(!checkForDirty())
        return;

    // get currently selected filename - indexes may not be valid after changes
    QString curSelected;
    if(ui.fileView->selectionModel()->hasSelection() && (ui.fileView->selectionModel()->selectedRows().count() == 1))
    {
        curSelected = fileViewModel->filePath(fileSorter->mapToSource(ui.fileView->currentIndex()));
    }

    AnalogExifOptions options(this);
    options.setModal(true);

    if(options.exec())
    {
        exifTreeModel->repopulate();
        ui.metadataView->expandAll();
    }

    // repopulate metadata
    if(!curSelected.isEmpty())
    {
        QModelIndex idx = fileSorter->mapFromSource(fileViewModel->index(curSelected));
        ui.fileView->selectionModel()->clearSelection();
        ui.fileView->setCurrentIndex(QModelIndex());

        if(idx.isValid())
        {
            // reselect and reload
            ui.fileView->selectionModel()->select(idx, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
            ui.fileView->setCurrentIndex(idx);
        }
    }
    else
    {
        // multiple or incorrect selection - clear the metadata view
        exifTreeModel->reload();
        ui.metadataView->expandAll();
    }
}

// directory line enter pressed
void AnalogExif::on_directoryLine_returnPressed()
{
    if(!checkForDirty())
        return;

    // try to open specified path
    QString filePath = ui.directoryLine->text();
    QFileInfo fileInfo(filePath);

    if(!fileInfo.exists() && !fileInfo.isDir())
    {
        QMessageBox::warning(this, tr("Path does not exist"), tr("The specified path (%1) does not exist.\nPlease enter correct path.").arg(filePath));
        return;
    }

    openLocation(QDir::fromNativeSeparators(filePath));
}

// open file
void AnalogExif::on_actionOpen_triggered(bool)
{
    if(!checkForDirty())
        return;

    QString filename = QFileDialog::getOpenFileName(this, tr("Select file to open..."), QDir::fromNativeSeparators(ui.directoryLine->text()), tr("JPEG images (*.jpg *.jpeg *jpe);;JPEG2000 images (*.jpf *.jpx *.jp2 *.j2c *.j2k *.jpc);;TIFF images (*.tif *.tiff);;DNG images (*.dng);;Photoshop PSD images (*.psd);;Camera raw images (*.cr2 *.nef *.pef *.rw2 *.arw *.sr2 *.orf *.raf *.mrw);;All files (*.*)"));

    if(!filename.isNull())
        openLocation(filename);
}

void AnalogExif::on_gearView_doubleClicked(const QModelIndex& index)
{
    selectGear(index);
    ui.gearView->expandAll();   // to redraw
}

bool AnalogExif::checkForDirty()
{
    // dirty data - notify user
    if(dirty)
    {
        QMessageBox::StandardButton result = QMessageBox::question(this, tr("Unsaved data"),
                                                            tr("Save changes in the current image?"),
                                                            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                                            QMessageBox::Cancel);

        if(result == QMessageBox::Cancel)
        {
            return false;
        }

        if(result == QMessageBox::Save)
            if(!save())
                return false;

    }

    setDirty(false);

    // invalidate selected equipment
    gearList->setSelectedIndex(QModelIndex());
    filmsList->setSelectedIndex(QModelIndex());
    developersList->setSelectedIndex(QModelIndex());
    authorsList->setSelectedIndex(QModelIndex());

    // deselect equipment
    ui.gearView->selectionModel()->clearSelection();
    ui.filmView->selectionModel()->clearSelection();
    ui.developerView->selectionModel()->clearSelection();
    ui.authorView->selectionModel()->clearSelection();

    return true;
}

void AnalogExif::on_action_Clear_tag_value_triggered(bool)
{
    // get the list of selected items
    QModelIndexList idxList = ui.metadataView->selectionModel()->selectedIndexes();
    if(idxList.count() == 0)
        return;

    // clear their values
    foreach(QModelIndex idx, idxList)
    {
        if(exifTreeModel->index(idx.row(), 1, idx.parent()).data(Qt::DisplayRole) != QVariant())
        {
            exifTreeModel->setData(idx, QVariant());
            }
    }

    ui.metadataView->expandAll();
}

void AnalogExif::metadataView_selectionChanged(const QItemSelection& selected, const QItemSelection&)
{
    if(selected.count() != 0)
    {
        // check for the data to clear
        foreach(QModelIndex idx, selected.indexes())
        {
            if(exifTreeModel->index(idx.row(), 1, idx.parent()).data(Qt::DisplayRole) != QVariant())
            {
                ui.action_Clear_tag_value->setEnabled(true);
                return;
            }
        }
    }

    // disable when nothing is selected or no data to clear
    ui.action_Clear_tag_value->setEnabled(false);
}

// open new library
void AnalogExif::on_actionOpen_library_triggered(bool)
{
    if(!checkForDirty())
        return;

    QString newName = QFileDialog::getOpenFileName(this, tr("Open equipment library"), QDir::fromNativeSeparators(ui.directoryLine->text()), tr("AnalogExif library files (*.ael);;All files (*.*)"));

    if(!newName.isNull())
    {
        // save previous database, in case of failure
        QString previousName = db.databaseName();

        if(!open(QDir::toNativeSeparators(newName)))
        {
            // failed to open new library - revert to previous one
            if(!open(previousName))
            {
                // failed to open previous as well - panic
                QCoreApplication::exit(-1);
            }
        }

        gearList->reload();
        filmsList->reload();
        developersList->reload();
        authorsList->reload();

        exifTreeModel->repopulate();

        // setup tree
        setupTreeView();

        ui.gearView->expandAll();

    }
}

// create new library
void AnalogExif::on_actionNew_library_triggered(bool)
{
    if(!checkForDirty())
        return;

    QString newDb = createLibrary(this, QDir::fromNativeSeparators(ui.directoryLine->text()));

    if(!newDb.isNull())
    {
        // save previous database, in case of failure
        QString previousName = db.databaseName();

        if(!open(QDir::toNativeSeparators(newDb)))
        {
            // failed to open new library - revert to previous one
            if(!open(previousName))
            {
                // failed to open previous as well - panic
                QCoreApplication::exit(-1);
            }
        }

        gearList->reload();
        filmsList->reload();
        developersList->reload();
        authorsList->reload();

        exifTreeModel->repopulate();

        // setup tree
        setupTreeView();

        ui.gearView->expandAll();
    }
}

// open new database
bool AnalogExif::open(QString dbName)
{
    // close if open
    if(db.isOpen())
        db.close();

    // set database name
    db.setDatabaseName(dbName);

    // try to open
    if(!db.open())
    {
        QSqlError error = db.lastError();
        QMessageBox::critical(this, tr("Critical error"), tr("Unable to open database (")+dbName+")");
        return false;
    }

    // check database version
    QSqlQuery query ("SELECT setValue, setValueText FROM Settings WHERE setId=1");
    query.first();
    if(query.isValid())
    {
        if(query.value(0).toInt() != dbVersion)
        {
            QMessageBox::critical(this, tr("Critical error"), tr("Unsupported AnalogExif library version:\n"
                                                                 "Required version %1, found version %2 (%3)").arg(dbVersion).arg(query.value(0).toInt()).arg(query.value(1).toString()));
            return false;
        }
    }
    else
    {
        QMessageBox::critical(this, tr("Critical error"), tr("Unable to query library version.\nCorrupt or invalid library file?"));
        return false;
    }

    // load user-defined ns

    // ignore the result
    ExifTreeModel::unregisterUserNs();

    query.exec("SELECT SetValueText FROM Settings WHERE SetId = 2");
    query.first();
    if(query.isValid())
    {
        QString userNs = query.value(0).toString();
        query.exec("SELECT SetValueText FROM Settings WHERE SetId = 3");
        query.first();

        if(!query.isValid())
        {
            QMessageBox::critical(this, tr("Critical error"), tr("Custom user-defined XMP schema data is not found.\nInvalid or corrupt database data."));
            return false;
        }
        else
        {
            if(!ExifTreeModel::registerUserNs(userNs, query.value(0).toString()))
            {
                QMessageBox::critical(this, tr("Critical error"), tr("Unable to register user-defined XMP schema."));
                return false;
            }
        }
    }

    return true;
}

// create new database file
QString AnalogExif::createLibrary(QWidget* parent, QString dir)
{
    QString newDb = QFileDialog::getSaveFileName(parent, tr("New equipment library"), dir, tr("AnalogExif library files (*.ael);;All files (*.*)"));

    if(!newDb.isNull())
    {
        // save previous database, in case of failure
        QString previousName = db.databaseName();

        // remove old file
        if(QFile::exists(newDb))
        {
            if(!QFile::remove(newDb))
            {
                QMessageBox::critical(this, tr("Library create error"), tr("Unable to overwrite library file ")+QDir::toNativeSeparators(newDb));
                return QString();
            }
        }

        // copy from the resource file
        if(!QFile::copy(":/database/NewDb.ael", newDb))
        {
            QMessageBox::critical(this, tr("Library create error"), tr("Unable to create new library ")+QDir::toNativeSeparators(newDb));
            return QString();
        }

        // fix file access
#ifdef Q_WS_WIN
        // use file attributes under Win32
        if(!SetFileAttributes((LPCTSTR)newDb.toStdWString().c_str(), FILE_ATTRIBUTE_NORMAL))
        {
            QMessageBox::critical(this, tr("Library create error"), tr("Unable to create new library ")+QDir::toNativeSeparators(newDb));
            return QString();
        }
#else
        // fix access permissions anywhere else
        QFile::Permissions filePermissions = QFile::permissions(newDb);

        if(!QFile::setPermissions(newDb, filePermissions | QFile::WriteOwner))
        {
            QMessageBox::critical(this, tr("Library create error"), tr("Unable to create new library ")+QDir::toNativeSeparators(newDb));
            return QString();
        }
#endif
    }

    return newDb;
}

void AnalogExif::addFileNames(QStringList& fileNames, const QString& path, bool includeDirs)
{
    QFileInfo fInfo(path);

    if(!fInfo.exists())
        return;

    // if file - just add the file to the list
    if(!fInfo.isDir())
    {
        fileNames << QDir::toNativeSeparators(path);
        filesFound++;
        return;
    }

    QStringList subDirs = QDir(path).entryList(QStringList() << "*.jpg" << "*.jpeg" << "*.tif" << "*.tiff", QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDir::Name | QDir::DirsLast | QDir::LocaleAware);
    foreach(QString dirName, subDirs)
    {
        addFileNames(fileNames, path + "/" + dirName, includeDirs);
    }

    if(includeDirs)
    {
        fileNames << QDir::toNativeSeparators(path);
        filesFound++;
    }
}

QStringList AnalogExif::scanSubfolders(QModelIndexList selIdx, bool includeDirs)
{
    QStringList fileNames;

    if(selIdx.count())
    {
        DirSortFilterProxyModel* sortModel = (DirSortFilterProxyModel*)selIdx.at(0).model();
        // browse through all selected indexes
        foreach(QModelIndex idx, selIdx)
        {
            addFileNames(fileNames, ((QFileSystemModel*)sortModel->sourceModel())->filePath(sortModel->mapToSource(idx)), includeDirs);
        }
    }

    return fileNames;
}

QStringList AnalogExif::getFileList(QModelIndexList selIdx, bool includeDirs, bool* cancelled)
{
    if(cancelled)
        *cancelled = false;

    filesFound = 0;

    ProgressDialog progress(tr("Scanning subfolders..."), tr("Files found: 0"), tr("Cancel"), this, 0, 500);
    QTime timer;

    timer.start();

    QFuture<QStringList> future = QtConcurrent::run(this, &AnalogExif::scanSubfolders, selIdx, includeDirs);

    int newFilesFound = 0;
    progress.setValue(0);

    while(!future.isFinished())
    {
        if(filesFound != newFilesFound)
        {
            newFilesFound = filesFound;
            progress.setValue(newFilesFound);
            progress.setLabelText(tr("Files found: %1").arg(newFilesFound));
        }

        if((timer.elapsed() > 500) && (!progress.isVisible()))
            progress.show();

        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents();

        if(progress.wasCanceled())
        {
            if(cancelled)
                *cancelled = true;
            return QStringList();
        }
    }

    progress.close();

    return future.result();
}

// auto-fill exposure
void AnalogExif::on_actionAuto_fill_exposure_triggered(bool)
{
    if(!checkForDirty())
        return;

    // determine the number of selected files
    QModelIndexList selIdx;

    if(ui.fileView->selectionModel()->hasSelection())
    {
        selIdx = ui.fileView->selectionModel()->selectedRows();
    }
    else
    {
        selIdx = ui.dirView->selectionModel()->selectedRows();
    }

    bool cancelled = false;

    QStringList fileNames = getFileList(selIdx, false, &cancelled);

    if(cancelled)
        return;

    if(fileNames.count() < 2)
    {
        QMessageBox::warning(this, tr("Less than two files in the folder"), tr("Selected folder contains less than two files.\nPlease use auto-fill exposure on at least two files."));
        return;
    }

    AutoFillExpNum autoFillDialog(fileNames, this);

    // get assigned exposure numbers
    if(autoFillDialog.exec() == QDialog::Accepted)
    {
        QVariantList sortedFiles = autoFillDialog.resultFileNames();
        QMessageBox::StandardButton saveBkp = QMessageBox::No;

        ProgressDialog progress(tr("Updating files..."), "", tr("Cancel"), this, 0, sortedFiles.count() / 2);
        progress.show();

        // mute metadata model to supress model changes
        ui.metadataView->blockSignals(true);

        // browse through all files
        for(int i = 0; i < sortedFiles.count(); i += 2)
        {
            QString fileName = sortedFiles.at(i).toString();

            progress.setValue(i / 2);
            progress.setLabelText(tr("Updating %1...").arg(fileName));

            // create backup, if required
            if(!createBackup(fileName, false, saveBkp))
            {
                ui.metadataView->blockSignals(false);
                exifTreeModel->clear(true);
                setupTreeView();
                return;
            }

            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

            QFuture<bool> future = QtConcurrent::run(exifTreeModel, &ExifTreeModel::setExposureNumber, fileName, sortedFiles.at(i+1).toInt());

            while(!future.isFinished())
            {
                QCoreApplication::processEvents();
                QCoreApplication::sendPostedEvents();
            }

            QApplication::restoreOverrideCursor();

            if(progress.wasCanceled())
            {
                ui.metadataView->blockSignals(false);
                exifTreeModel->clear(true);
                setupTreeView();

                return;
            }

            if(!future.result())
            {
                QMessageBox::critical(this, tr("File save error"), tr("Unable to set exposure number for %1.").arg(fileName));

                ui.metadataView->blockSignals(false);
                exifTreeModel->clear(true);
                setupTreeView();

                return;
            }
        }

        ui.metadataView->blockSignals(false);
    }
    exifTreeModel->clear(true);
    setupTreeView();
}

// double-click - launch file
void AnalogExif::on_fileView_doubleClicked(const QModelIndex& index)
{
    openExternal(index);
}

void AnalogExif::on_actionOpen_external_triggered(bool)
{
    // determine the number of selected files
    QModelIndexList selIdx = ui.fileView->selectionModel()->selectedRows();

    foreach(QModelIndex idx, selIdx)
    {
        openExternal(idx);
    }
}

void AnalogExif::openExternal(const QModelIndex& index)
{
    if(!index.isValid())
        return;

    QModelIndex idx = fileSorter->mapToSource(index);

    if(!fileViewModel->isDir(idx))
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileViewModel->filePath(idx)));
}

void AnalogExif::on_actionRename_triggered(bool)
{
    if(ui.fileView->selectionModel()->hasSelection())
    {
        // triggered on file view
        ui.fileView->edit(ui.fileView->selectionModel()->currentIndex());
    }
    else
    {
        // triggered on dir view
        ui.dirView->edit(ui.dirView->selectionModel()->currentIndex());
    }
}

void AnalogExif::on_actionRemove_triggered(bool)
{
    // determine the number of selected files
    QModelIndexList selIdx;

    if(ui.fileView->selectionModel()->hasSelection())
    {
        selIdx = ui.fileView->selectionModel()->selectedRows();
    }
    else
    {
        selIdx = ui.dirView->selectionModel()->selectedRows();
    }

    bool cancelled = false;

    QStringList fileNames = getFileList(selIdx, true, &cancelled);

    if(cancelled)
        return;

    QMessageBox warning;
    warning.setWindowTitle(tr("Delete files"));
    warning.setWindowIcon(windowIcon());
    if(fileNames.count() > 1)
    {
        warning.setText(tr("Are you sure you want to delete these files?"));
    
        QString affectedList = tr("The following files will be deleted:\n\n");
        foreach(QString str, fileNames)
        {
            affectedList += str + "\n";
        }
        warning.setDetailedText(affectedList);
    }
    else
    {
        warning.setText(tr("Are you sure you want to delete this file?"));
    }

    warning.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    warning.setDefaultButton(QMessageBox::No);
    warning.setIcon(QMessageBox::Warning);

    if(warning.exec() == QMessageBox::No)
        return;

    ProgressDialog progress(tr("Deleting files..."), tr("Deleting"), tr("Cancel"), this, 0, fileNames.count());
    QTime timer;

    timer.start();

    int filesDeleted = 0;
    progress.setValue(0);

    foreach(QString str, fileNames)
    {
        progress.setLabelText(tr("Deleting %1...").arg(str));

        QFileInfo fInfo(str);
        if(fInfo.isDir())
        {
            QDir dir(str);
            if(!dir.rmdir(str))
            {
                QMessageBox::critical(this, tr("Remove error"), tr("Unable to delete folder %1.").arg(str));
                return;
            }
        }
        else
        {
            if(!QFile::remove(str))
            {
                QMessageBox::critical(this, tr("Remove error"), tr("Unable to delete file %1.").arg(str));
                return;
            }
         }

        if((timer.elapsed() > 500) && (!progress.isVisible()))
            progress.show();

        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents();

        if(progress.wasCanceled())
            return;

        filesDeleted++;
        progress.setValue(filesDeleted);
    }
}

// copy metadata from another file
void AnalogExif::on_action_Copy_metadata_triggered(bool)
{
    if(!checkForDirty())
        return;

    // determine the number of selected files
    QModelIndexList selIdx;
    QString selectedFname;

    if(ui.fileView->selectionModel()->hasSelection())
    {
        selIdx = ui.fileView->selectionModel()->selectedRows();

        // if only one file selected - store its filename to reload later
        if(selIdx.count() == 1)
        {
            if(selIdx.at(0).isValid())
                selectedFname = fileViewModel->filePath(fileSorter->mapToSource(selIdx.at(0)));
        }
    }
    else
    {
        selIdx = ui.dirView->selectionModel()->selectedRows();
    }

    QStringList fileNames = getFileList(selIdx);

    if(fileNames.isEmpty())
        return;

    // get source filename
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select source file for metadata..."), QDir::fromNativeSeparators(ui.directoryLine->text()), tr("JPEG images (*.jpg *.jpeg *jpe);;JPEG2000 images (*.jpf *.jpx *.jp2 *.j2c *.j2k *.jpc);;TIFF images (*.tif *.tiff);;DNG images (*.dng);;Photoshop PSD images (*.psd);;Camera raw images (*.cr2 *.nef *.pef *.rw2 *.arw *.sr2 *.orf *.raf *.mrw);;All files (*.*)"));

    if(fileName.isNull())
        return;

    // show the metadata selection dialog
    CopyMetadataDialog copyMetadata(fileName, this);

    if(copyMetadata.exec() == QDialog::Accepted)
    {
        QVariantList data = copyMetadata.getMetadata();
        QMessageBox::StandardButton saveBkp = QMessageBox::No;

        ProgressDialog progress(tr("Updating files..."), "", tr("Cancel"), this, 0, fileNames.count());
        progress.show();

        int nFiles = 0;
        progress.setValue(nFiles);

        exifTreeModel->blockSignals(true);

        // browse through all files
        foreach(QString fName, fileNames)
        {
            progress.setLabelText(tr("Updating %1...").arg(fName));

            // create backup, if required
            if(!createBackup(fName, false, saveBkp))
            {
                exifTreeModel->blockSignals(false);
                exifTreeModel->clear(true);
                setupTreeView();
                return;
            }

            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            QFuture<bool> future = QtConcurrent::run(exifTreeModel, &ExifTreeModel::mergeMetadata, fName, data);

            while(!future.isFinished())
            {
                QCoreApplication::processEvents();
                QCoreApplication::sendPostedEvents();
            }

            QApplication::restoreOverrideCursor();

            if(progress.wasCanceled())
            {
                exifTreeModel->blockSignals(false);
                exifTreeModel->clear(true);
                setupTreeView();

                return;
            }

            if(!future.result())
            {
                QMessageBox::critical(this, tr("File save error"), tr("Unable to set metadata for %1.").arg(fName));

                exifTreeModel->blockSignals(false);
                exifTreeModel->clear(true);
                setupTreeView();

                return;
            }

            nFiles++;
            progress.setValue(nFiles);
        }

        exifTreeModel->blockSignals(false);
        exifTreeModel->clear(true);
    }

    if(!selectedFname.isEmpty())
    {
        QModelIndex idx = fileSorter->mapFromSource(fileViewModel->index(selectedFname));
        ui.fileView->selectionModel()->clearSelection();
        ui.fileView->setCurrentIndex(QModelIndex());

        if(idx.isValid())
        {
            // reselect and reload
            ui.fileView->selectionModel()->select(idx, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
            ui.fileView->setCurrentIndex(idx);
        }
    }
    else
    {
        // multiple or incorrect selection - clear the metadata view
        exifTreeModel->reload();
        ui.metadataView->expandAll();
    }
}

void AnalogExif::on_action_About_triggered(bool)
{
    QPointer<DPluginAboutDlg> dlg = new DPluginAboutDlg(m_tool);
    dlg->exec();
    delete dlg;
}

void AnalogExif::on_actionHelp_triggered(bool)
{
    QDesktopServices::openUrl(helpUrl);
}
