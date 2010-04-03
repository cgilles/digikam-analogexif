#ifndef ANALOGEXIF_H
#define ANALOGEXIF_H

#include <QMainWindow>
#include <QFileSystemModel>
#include <QPixmap>
#include <QSqlDatabase>
#include <QSqlQueryModel>
#include <QSettings>

#include "ui_analogexif.h"

#include "dirsortfilterproxymodel.h"
#include "exiftreemodel.h"

class AnalogExif : public QMainWindow
{
	Q_OBJECT

public:
	AnalogExif(QWidget *parent = 0, Qt::WFlags flags = 0);
	~AnalogExif();

	bool initialize();

private:
	Ui::AnalogExifClass ui;
	QSettings settings;

	// directory model
	QFileSystemModel* fileViewModel;
	// pixmap to hold file preview
	QPixmap* filePreviewPixmap;
	// custom directory sorter
	DirSortFilterProxyModel* dirSorter;

	// Exif metadata tree model
	ExifTreeModel* exifTreeModel;

	// database
	QSqlDatabase db;

	// Sql query model
	QSqlQueryModel* sqlModel;

	// current version of the database
	static const int dbVersion = 1;

	// onResize
	virtual void resizeEvent(QResizeEvent * event);

private slots:
	// selection from folder viewer
	void fileView_clicked(const QModelIndex& sortIndex);
	// expansion of the tree
	void fileView_expanded (const QModelIndex& sortIndex);
};

#endif // ANALOGEXIF_H
