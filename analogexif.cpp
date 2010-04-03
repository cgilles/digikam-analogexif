#include "analogexif.h"

#include <QStringList>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>

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
	connect(ui.fileView, SIGNAL(clicked(const QModelIndex&)), this, SLOT(fileView_clicked(const QModelIndex&)));
	connect(ui.fileView, SIGNAL(expanded(const QModelIndex&)), this, SLOT(fileView_expanded(const QModelIndex&)));
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

	// set exif metadata model
	//exifTreeModel = new ExifTreeModel(this);
	//ui.metadataView->setModel(exifTreeModel);
	sqlModel = new QSqlQueryModel;
	//sqlModel->setQuery("SELECT b.Description, a.TagText FROM MetaTags a, MetaCategories b where a.CategoryId=b.id ORDER BY b.OrderBy, b.OrderBy");
	ui.metadataView->setModel(sqlModel);
}

AnalogExif::~AnalogExif()
{
	//delete exifTreeModel;
	delete sqlModel;
	delete dirSorter;
	delete fileViewModel;
	delete filePreviewPixmap;
}

// perform all initialization
bool AnalogExif::initialize()
{
	// open database
	QString dbName = settings.value("dbName", "analogexif.db").toString();

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

	sqlModel->setQuery("SELECT b.Description, a.TagText FROM MetaTags a, MetaCategories b where a.CategoryId=b.id ORDER BY b.OrderBy, b.OrderBy");

	// start scan
	fileViewModel->setRootPath(QDir::rootPath());

	return true;
}

// called when user clicks on item in the file view
void AnalogExif::fileView_clicked(const QModelIndex& sortIndex)
{
	// have to map indices
	QModelIndex& index = dirSorter->mapToSource(sortIndex);

	if(fileViewModel->isDir(index))
	{
		// update the directory line in case directory selected
		ui.directoryLine->setText(QDir::toNativeSeparators(fileViewModel->filePath(index)));
		ui.filePreview->setPixmap(NULL);
	}
	else
	{
		// show file preview and details
		ui.directoryLine->setText(QDir::toNativeSeparators(fileViewModel->fileInfo(index).absolutePath()));
		filePreviewPixmap->load(fileViewModel->filePath(index), 0, Qt::ThresholdDither);

		QSize previewSize = ui.filePreviewGroupBox->contentsRect().size();
		ui.filePreview->setPixmap(filePreviewPixmap->scaled(previewSize.width()-30, previewSize.height()-30, Qt::KeepAspectRatio));

		// load metadata
		exifTreeModel->openFile(QDir::toNativeSeparators(fileViewModel->filePath(index)));
		ui.metadataView->setFirstColumnSpanned(0, QModelIndex(), true);
		ui.metadataView->expandAll();
	}
}

// called when user expands the file browser tree
void AnalogExif::fileView_expanded (const QModelIndex& sortIndex)
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