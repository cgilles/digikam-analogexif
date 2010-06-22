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
#include "editgear.h"
#include "analogexifoptions.h"
#include "autofillexpnum.h"
#include "progressdialog.h"
#include "copymetadatadialog.h"
#include "aboutdialog.h"

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

AnalogExif::AnalogExif(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);

	/// setup tree view
	fileViewModel = new QFileSystemModel(this);
	dirSorter = new DirSortFilterProxyModel(this);
	// show supported files only
	fileViewModel->setNameFilterDisables(false);
	fileViewModel->setNameFilters(QStringList() << "*.jpg" << "*.jpeg" << "*.tif" << "*.tiff");
	fileViewModel->setReadOnly(false);
	// connect selection and expansion events
	ui.fileView->setModel(dirSorter);
	// hide Type column
	ui.fileView->hideColumn(2);
	// resize Name and Size column
	ui.fileView->setColumnWidth(0, 150);
	ui.fileView->setColumnWidth(1, 50);
	// sort by filename 
	ui.fileView->setSortingEnabled(true);
	ui.fileView->sortByColumn(0, Qt::AscendingOrder);

	// set file preview
	// filePreviewPixmap = new QPixmap();
	ui.filePreview->setPixmap(filePreviewPixmap);

	exifTreeModel = NULL;
	filmsList = NULL;
	gearList = NULL;
	authorsList = NULL;
	developersList = NULL;

	// set context menus
	QList<QAction*> contextMenus;
	QAction* separator = new QAction(this);
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
	connect(this, SIGNAL(updatePreview()), this, SLOT(previewUpdate()), Qt::BlockingQueuedConnection);
#endif

	contextMenus.clear();

	contextMenus << ui.actionAuto_fill_exposure << ui.action_Copy_metadata << separator << ui.actionOpen_external << ui.actionRename << separator << ui.actionRemove;
	ui.fileView->addActions(contextMenus);

	verChecker = new OnlineVersionChecker(this);
	connect(verChecker, SIGNAL(newVersionAvailable(QString, QString, QDateTime, QString)), this, SLOT(newVersionAvailable(QString, QString, QDateTime, QString)));

#ifdef Q_WS_WIN
	if(QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA)
	{
		// For Vista/W7 style - disable alternating row colors on equipment views for better visibility
		ui.gearView->setAlternatingRowColors(false);
		ui.filmView->setAlternatingRowColors(false);
		ui.developerView->setAlternatingRowColors(false);
		ui.authorView->setAlternatingRowColors(false);
	}
#endif

	// set application proxy
	AnalogExifOptions::setupProxy();
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
	delete fileViewModel;
	delete verChecker;
	// delete filePreviewPixmap;
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

		if(msgBox.clickedButton() == cancelBtn)
		{
			QCoreApplication::exit(-1);
			return false;
		}
		else if(msgBox.clickedButton() == openBtn)
		{
			dbName = QFileDialog::getOpenFileName(NULL, tr("Open equipment library"), QString(), tr("AnalogExif library files (*.ael);;All files (*.*)"));
		}
		else if(msgBox.clickedButton() == newBtn)
		{
			dbName = createLibrary();
		}
	}

	db = QSqlDatabase::addDatabase("QSQLITE");

	if(!open(dbName))
		return false;

	// set exif metadata model
	exifTreeModel = new ExifTreeModel(this);
	exifItemDelegate = new ExifItemDelegate(this);

	ui.metadataView->setModel(exifTreeModel);
	ui.metadataView->setItemDelegateForColumn(1, exifItemDelegate);

	// connect to data changed signal
	connect(exifTreeModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(modelDataChanged(const QModelIndex&, const QModelIndex&)));

	connect(ui.metadataView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(metadataView_selectionChanged(const QItemSelection&, const QItemSelection&)));
	
	// span the categories to full row
	setupTreeView();

	// films view
	filmsList = new GearListModel(this, 2, tr("No film defined"));
	filmsList->reload();

	ui.filmView->setModel(filmsList);
	connect(ui.filmView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(filmAndGearView_selectionChanged(const QItemSelection&, const QItemSelection&)));

	// authors view
	authorsList = new GearListModel(this, 3, tr("No authors defined"));
	authorsList->reload();

	ui.authorView->setModel(authorsList);
	connect(ui.authorView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(filmAndGearView_selectionChanged(const QItemSelection&, const QItemSelection&)));

	// developers view
	developersList = new GearListModel(this, 4, tr("No developers defined"));
	developersList->reload();

	ui.developerView->setModel(developersList);
	connect(ui.developerView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(filmAndGearView_selectionChanged(const QItemSelection&, const QItemSelection&)));

	// gear view
	gearList = new GearTreeModel(this);
	gearList->reload();
	ui.gearView->setModel(gearList);
	connect(ui.gearView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(filmAndGearView_selectionChanged(const QItemSelection&, const QItemSelection&)));

	filmsList->setApplicable(true);
	gearList->setApplicable(true);
	developersList->setApplicable(true);
	authorsList->setApplicable(true);

	if(gearList->bodyCount() == 0)
		ui.gearView->setRootIsDecorated(false);
	ui.gearView->expandAll();

	// start scan
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        fileViewModel->setRootPath(QDir::rootPath());
	QApplication::restoreOverrideCursor();

	dirSorter->setSourceModel(fileViewModel);

	connect(ui.fileView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(fileView_selectionChanged(const QItemSelection&, const QItemSelection&)));

	QModelIndex lastFolder = dirSorter->mapFromSource(fileViewModel->index(settings.value("lastFolder", QDir::homePath()).toString()));
	if(lastFolder != QModelIndex())
	{
		ui.directoryLine->setText(QDir::toNativeSeparators(settings.value("lastFolder", QDir::homePath()).toString()));
		ui.fileView->scrollTo(lastFolder, QAbstractItemView::PositionAtTop);
		ui.fileView->setCurrentIndex(lastFolder);
		ui.fileView->setExpanded(lastFolder, true);
	}

	// check for the new version
	verChecker->checkForNewVersion();

	return true;
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
	else
	{
		ui.action_Copy_metadata->setEnabled(false);
	}

	// selected single item
	if(selIdx.count() == 1)
	{
		QModelIndex index = dirSorter->mapToSource(selIdx.at(0));

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
			ui.filePreview->setPixmap(NULL);
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
		else
		{
			// selected directory - set proper directory string
			ui.directoryLine->setText(QDir::toNativeSeparators(fileViewModel->filePath(index)));
		}
	}
	else
	{
		// multiple selection - clear directory string
		ui.directoryLine->setText("");
	}

	// enable auto-fill exposure numbers for several files or directory(ies)
	ui.actionAuto_fill_exposure->setEnabled(true);

	ui.actionOpen_external->setEnabled(false);

	// clear file preview
	ui.filePreview->setPixmap(NULL);

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
		ui.filePreview->setPixmap(NULL);

		// clear metatags
		exifTreeModel->clear(true);

		setWindowTitle(QCoreApplication::applicationName());
		setWindowModified(false);

		// clear current file name
		curFileName = "";
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
		ui.filePreview->setPixmap(NULL);
#ifdef Q_WS_MAC
                // Background loading doesn't work properly for Mac
                QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                loadPreview(curFileName);
                QApplication::restoreOverrideCursor();
#else
                QFuture<void> future = QtConcurrent::run(this, &AnalogExif::loadPreview, curFileName);
#endif
        }

	ui.gearView->expandAll();

	setDirty(false);

	// setup tree
	setupTreeView();

	// scroll and select
	QModelIndex idx = dirSorter->mapFromSource(fileViewModel->index(path));
	ui.fileView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
	ui.fileView->selectionModel()->select(idx, QItemSelectionModel::SelectCurrent);
}

// background preview loader
void AnalogExif::loadPreview(QString filename)
{
	// show file preview and details

	// try to load preview
	QByteArray* preview = exifTreeModel->getPreview();
	if(preview)
	{
		filePreviewPixmap.loadFromData(*preview);
		delete preview;
	}
	else
	{
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

// called when user expands the file browser tree
void AnalogExif::on_fileView_expanded (const QModelIndex& sortIndex)
{
	// have to map indices
	QModelIndex index = dirSorter->mapToSource(sortIndex);
	ui.directoryLine->setText(QDir::toNativeSeparators(fileViewModel->filePath(index)));
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
	QModelIndexList selIdx = ui.fileView->selectionModel()->selectedRows();
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
	filmsList->setSelectedIndex(QModelIndex());
	gearList->setSelectedIndex(QModelIndex());
	authorsList->setSelectedIndex(QModelIndex());
	developersList->setSelectedIndex(QModelIndex());

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

	QModelIndex curFolder = ui.fileView->selectionModel()->currentIndex();
	if(curFolder != QModelIndex())
	{
		if(!fileViewModel->isDir(dirSorter->mapToSource(curFolder)))
			settings.setValue("lastFolder", QDir::toNativeSeparators(fileViewModel->fileInfo(dirSorter->mapToSource(curFolder)).canonicalPath()));
		else
			settings.setValue("lastFolder", QDir::toNativeSeparators(fileViewModel->filePath(dirSorter->mapToSource(curFolder))));
	}

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

	EditGear edit(this);
	edit.setModal(true);

	if(edit.exec())
	{
		gearList->reload();
		ui.gearView->expandAll();
		filmsList->reload();
		authorsList->reload();
		developersList->reload();
	}
}

// Preferences
void AnalogExif::on_actionPreferences_triggered(bool)
{
	if(!checkForDirty())
		return;

	AnalogExifOptions options(this);
	options.setModal(true);

	if(options.exec())
	{
		exifTreeModel->repopulate();
		ui.metadataView->expandAll();
	}
}

// directory line enter pressed
void AnalogExif::on_directoryLine_returnPressed()
{
	if(!checkForDirty())
		return;

	// try to open specified path
	QFileInfo fileInfo(ui.directoryLine->text());

	// TODO: show warning message box?
	if(!fileInfo.exists())
		return;

	openLocation(QDir::fromNativeSeparators(ui.directoryLine->text()));
}

// open file
void AnalogExif::on_actionOpen_triggered(bool)
{
	if(!checkForDirty())
		return;

	QString filename = QFileDialog::getOpenFileName(this, tr("Select file to open..."), QDir::fromNativeSeparators(ui.directoryLine->text()), tr("JPEG images (*.jpg *.jpeg);;TIFF images (*.tif *.tiff);;All files (*.*)"));

	if(!filename.isNull())
		openLocation(filename);
}

void AnalogExif::on_gearView_doubleClicked(const QModelIndex& index)
{
	selectGear(index);
	ui.gearView->expandAll();	// to redraw
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
		exifTreeModel->setData(idx, QVariant());
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
		if(!QFile::copy(":/database/AnalogExif.ael", newDb))
		{
			QMessageBox::critical(this, tr("Library create error"), tr("Unable to create new library ")+QDir::toNativeSeparators(newDb));
			return QString();
		}
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

	// browse through all selected indexes
	foreach(QModelIndex idx, selIdx)
	{
		addFileNames(fileNames, fileViewModel->filePath(dirSorter->mapToSource(idx)), includeDirs);
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
	// determine the number of selected files
	QModelIndexList selIdx = ui.fileView->selectionModel()->selectedRows();

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
		exifTreeModel->clear(true);
		setupTreeView();
	}
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

	QModelIndex idx = dirSorter->mapToSource(index);

	if(!fileViewModel->isDir(idx))
		QDesktopServices::openUrl(QUrl::fromLocalFile(fileViewModel->filePath(idx)));
}

void AnalogExif::on_actionRename_triggered(bool)
{
	ui.fileView->edit(ui.fileView->selectionModel()->currentIndex());
}

void AnalogExif::on_actionRemove_triggered(bool)
{
	// determine the number of selected files
	QModelIndexList selIdx = ui.fileView->selectionModel()->selectedRows();

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
	// determine the number of selected files
	QModelIndexList selIdx = ui.fileView->selectionModel()->selectedRows();

	QStringList fileNames = getFileList(selIdx);

	if(fileNames.isEmpty())
		return;

	// get source filename
	QString fileName = QFileDialog::getOpenFileName(this, tr("Select source file for metadata..."), QDir::fromNativeSeparators(ui.directoryLine->text()), tr("JPEG images (*.jpg *.jpeg);;TIFF images (*.tif *.tiff);;All files (*.*)"));

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
				QMessageBox::critical(this, tr("File save error"), tr("Unable to set metadata for %1.").arg(fileName));

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

		if((selIdx.count() == 1) && (!fileViewModel->isDir(dirSorter->mapToSource(selIdx.at(0)))))
		{
			// if only one file selected, reselect it and trigger metadata reload
			ui.fileView->clearSelection();
			ui.fileView->selectionModel()->select(selIdx.at(0), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
		}
		else
		{
			// restore view
			setupTreeView();
		}
	}
}

void AnalogExif::on_action_About_triggered(bool)
{
	AboutDialog a(this);
	a.exec();
}

void AnalogExif::newVersionAvailable(QString selfTag, QString newTag, QDateTime newTime, QString newSummary)
{
    QMessageBox info(QMessageBox::Question, tr("New program version available"), tr("New version of AnalogExif is available.\n"
               "Current version is %1, new version is %2 from %3.\n\n"
               "Do you want to open the program website?").arg(selfTag).arg(newTag).arg(newTime.toString("dd.MM.yyyy")), QMessageBox::Yes | QMessageBox::No, this);
    info.setDetailedText(newSummary);

    if(info.exec() == QMessageBox::Yes)
    {
            OnlineVersionChecker::openDownloadPage();
    }
}
