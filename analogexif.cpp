#include "analogexif.h"
#include "editgear.h"
#include "analogexifoptions.h"

#include <QStringList>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QtConcurrentRun>

AnalogExif::AnalogExif(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);

	/// setup tree view
	fileViewModel = new QFileSystemModel(this);
	dirSorter = new DirSortFilterProxyModel(this);
	dirSorter->setSourceModel(fileViewModel);
	// show JPEG only
	fileViewModel->setNameFilterDisables(false);
	fileViewModel->setNameFilters(QStringList() << "*.jpg" << "*.jpeg");
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
	filePreviewPixmap = new QPixmap();
	ui.filePreview->setPixmap(*filePreviewPixmap);

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

	dirty = false;

	// read previous window state
	if(settings.contains("WindowState")){
		restoreState(settings.value("WindowState").toByteArray());
	}

	// Qt for Linux does not work very well with re/storing window geometry
#if defined(__linux__)
	QPoint pos = settings.value("WindowPos", QPoint(0,0)).toPoint();
	QSize size = settings.value("WindowSize", QSize(701, 576)).toSize();
	resize(size);
	move(pos);
#else
	if(settings.contains("WindowGeometry")){
		restoreGeometry(settings.value("WindowGeometry").toByteArray());
	}
#endif

	setWindowTitle(QCoreApplication::applicationName());
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
	delete filePreviewPixmap;
}

// perform all initialization
bool AnalogExif::initialize()
{
	// open database
	QString dbName = settings.value("dbName", "AnalogExif.db").toString();

	db = QSqlDatabase::addDatabase("QSQLITE");
	db.setDatabaseName(dbName);
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
			QMessageBox::critical(this, tr("Critical error"), tr("Unsupported AnalogExif database version:\n"
																 "Required version %1, found version %2 (%3)").arg(dbVersion).arg(query.value(0).toInt()).arg(query.value(1).toString()));
			return false;
		}
	}
	else
	{
		QMessageBox::critical(this, tr("Critical error"), tr("Unable to query database version.\nCorrupt or invalid database file?"));
		return false;
	}

	// set exif metadata model
	exifTreeModel = new ExifTreeModel(db, this);
	exifItemDelegate = new ExifItemDelegate(this);

	ui.metadataView->setModel(exifTreeModel);
	ui.metadataView->setItemDelegateForColumn(1, exifItemDelegate);

	// connect to data changed signal
	connect(exifTreeModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(modelDataChanged(const QModelIndex&, const QModelIndex&)));
	
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
	ui.gearView->setModel(gearList);
	connect(ui.gearView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(filmAndGearView_selectionChanged(const QItemSelection&, const QItemSelection&)));

	if(gearList->bodyCount() == 0)
		ui.gearView->setRootIsDecorated(false);
	ui.gearView->expandAll();

	// start scan
	fileViewModel->setRootPath(QDir::rootPath());

	QModelIndex& lastFolder = dirSorter->mapFromSource(fileViewModel->index(settings.value("lastFolder", QDir::homePath()).toString()));
	if(lastFolder != QModelIndex())
	{
		ui.directoryLine->setText(QDir::toNativeSeparators(settings.value("lastFolder", QDir::homePath()).toString()));
		ui.fileView->scrollTo(lastFolder, QAbstractItemView::PositionAtTop);
		ui.fileView->setCurrentIndex(lastFolder);
		ui.fileView->setExpanded(lastFolder, true);
	}
	return true;
}

// called when user clicks on item in the file view
void AnalogExif::on_fileView_clicked(const QModelIndex& sortIndex)
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
			// select previous file
			ui.fileView->selectionModel()->select(dirSorter->mapFromSource(previewIndex), QItemSelectionModel::SelectCurrent);
			return;
		}

		if(result == QMessageBox::Save)
			exifTreeModel->submit();
	}

	dirty = false;

	// have to map indices
	QModelIndex& index = dirSorter->mapToSource(dirSorter->index(sortIndex.row(), 0, sortIndex.parent()));
	previewIndex = index;

	if(fileViewModel->isDir(index))
	{
		// update the directory line in case directory selected
		ui.directoryLine->setText(QDir::toNativeSeparators(fileViewModel->filePath(index)));
		setWindowTitle(QCoreApplication::applicationName());
		ui.filePreview->setPixmap(NULL);
		exifTreeModel->clear();

		filmsList->setApplicable(false);
		gearList->setApplicable(false);
		developersList->setApplicable(false);
		authorsList->setApplicable(false);

		ui.gearView->expandAll();

		ui.actionApply_gear->setEnabled(false);
		ui.applyGearBtn->setEnabled(false);

		dirty = false;
		setWindowModified(false);
	}
	else
	{
		// load metadata
		exifTreeModel->openFile(QDir::toNativeSeparators(fileViewModel->filePath(index)));

		filmsList->setApplicable(true);
		gearList->setApplicable(true);
		developersList->setApplicable(true);
		authorsList->setApplicable(true);

		ui.gearView->expandAll();

		bool validSelection = ui.gearView->selectionModel()->hasSelection() | ui.filmView->selectionModel()->hasSelection() | ui.developerView->selectionModel()->hasSelection() | ui.authorView->selectionModel()->hasSelection();
		ui.actionApply_gear->setEnabled(validSelection);
		ui.applyGearBtn->setEnabled(validSelection);

		dirty = false;
		setWindowModified(false);

		setWindowTitle("");
		setWindowFilePath(fileViewModel->fileName(index));

		// load preview in the background
		ui.filePreview->setPixmap(NULL);
		QFuture<void> future = QtConcurrent::run(this, &AnalogExif::loadPreview);
	}
	setupTreeView();
}

// background preview loader
void AnalogExif::loadPreview()
{
	// show file preview and details
	ui.directoryLine->setText(QDir::toNativeSeparators(fileViewModel->fileInfo(previewIndex).absolutePath()));
	filePreviewPixmap->load(fileViewModel->filePath(previewIndex), 0, Qt::ThresholdDither);

	QSize previewSize = ui.filePreviewGroupBox->contentsRect().size();
	ui.filePreview->setPixmap(filePreviewPixmap->scaled(previewSize.width()-30, previewSize.height()-30, Qt::KeepAspectRatio));
}

// called when user expands the file browser tree
void AnalogExif::on_fileView_expanded (const QModelIndex& sortIndex)
{
	// have to map indices
	QModelIndex& index = dirSorter->mapToSource(sortIndex);

	ui.directoryLine->setText(QDir::toNativeSeparators(fileViewModel->filePath(index)));
}

// on main window resize event
void AnalogExif::resizeEvent(QResizeEvent * event)
{
	// rescale the pixmap
	if(!ui.filePreview->pixmap()->isNull())
	{
		QSize previewSize = ui.filePreviewGroupBox->contentsRect().size();
		ui.filePreview->setPixmap(filePreviewPixmap->scaled(previewSize.width()-30, previewSize.height()-30, Qt::KeepAspectRatio));
	}
}

// apply changes
void AnalogExif::on_applyChangesBtn_clicked()
{
	// save data
	exifTreeModel->submit();
	dirty = false;
	setWindowModified(false);
	setupTreeView();
	filmsList->setSelectedIndex(QModelIndex());
	gearList->setSelectedIndex(QModelIndex());

	ui.applyChangesBtn->setEnabled(false);
	ui.revertBtn->setEnabled(false);
	ui.action_Save->setEnabled(false);
	ui.action_Undo->setEnabled(false);

	ui.metadataView->setFocus(Qt::OtherFocusReason);
}

// revert changes
void AnalogExif::on_revertBtn_clicked()
{
	exifTreeModel->reload();
	dirty = false;
	setWindowModified(false);
	setupTreeView();
	filmsList->setSelectedIndex(QModelIndex());
	gearList->setSelectedIndex(QModelIndex());

	ui.applyChangesBtn->setEnabled(false);
	ui.revertBtn->setEnabled(false);
	ui.action_Save->setEnabled(false);
	ui.action_Undo->setEnabled(false);

	ui.metadataView->setFocus(Qt::OtherFocusReason);
}

// data model changed signal
void AnalogExif::modelDataChanged(const QModelIndex&, const QModelIndex&)
{
	// data changed - enable apply/revert buttons
	dirty = true;
	setWindowModified(true);
	ui.applyChangesBtn->setEnabled(true);
	ui.revertBtn->setEnabled(true);
	ui.action_Save->setEnabled(true);
	ui.action_Undo->setEnabled(true);
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
	// save required settings
	
	// do not save database name - use it only for defaults override
	// settings.setValue("dbName", db.databaseName());

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

	// Qt for Linux does not work very well with re/storing window geometry
#if defined(__linux__)
	settings.setValue("WindowPos", pos());
	settings.setValue("WindowSize", size());
#else
	settings.setValue("WindowGeometry", saveGeometry());
#endif

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
		exifTreeModel->setValues(filmData.toList());
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
		exifTreeModel->setValues(gearData.toList());
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
		exifTreeModel->setValues(authorData.toList());
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
		exifTreeModel->setValues(devData.toList());
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
void AnalogExif::filmAndGearView_selectionChanged(const QItemSelection& selected, const QItemSelection&)
{
	bool validSelection = ui.gearView->selectionModel()->hasSelection() | ui.filmView->selectionModel()->hasSelection() | ui.developerView->selectionModel()->hasSelection() | ui.authorView->selectionModel()->hasSelection();
	ui.actionApply_gear->setEnabled(validSelection);
	ui.applyGearBtn->setEnabled(validSelection);
}

// Apply action triggered
void AnalogExif::on_actionApply_gear_triggered(bool checked)
{
	QModelIndex curFilm = ui.filmView->currentIndex();
	QModelIndex curGear = ui.gearView->currentIndex();
	QModelIndex curAuthor = ui.authorView->currentIndex();
	QModelIndex curDeveloper = ui.developerView->currentIndex();

	if(curFilm.isValid())
		selectFilm(curFilm);

	if(curGear.isValid())
		selectGear(curGear);

	if(curAuthor.isValid())
		selectAuthor(curAuthor);

	if(curDeveloper.isValid())
		selectDeveloper(curDeveloper);
}

// Edit gear action
void AnalogExif::on_actionEdit_gear_triggered(bool checked)
{
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
void AnalogExif::on_actionPreferences_triggered(bool checked)
{
	AnalogExifOptions options(this);
	options.setModal(true);

	if(options.exec())
	{
		exifTreeModel->repopulate();
		ui.metadataView->expandAll();
	}
}