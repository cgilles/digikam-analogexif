#ifndef ANALOGEXIF_H
#define ANALOGEXIF_H

#include <QMainWindow>
#include <QFileSystemModel>
#include <QPixmap>
#include <QSqlDatabase>
#include <QSettings>
#include <QCloseEvent>

#include "ui_analogexif.h"

#include "dirsortfilterproxymodel.h"
#include "exiftreemodel.h"
#include "exifitemdelegate.h"
#include "gearlistmodel.h"
#include "geartreemodel.h"

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

	// setup tree view
	void setupTreeView();

	// data is dirty
	bool dirty;

	// save data
	void save() {}

	// background preview loader
	void loadPreview();

	// apply film settings
	void selectFilm(const QModelIndex& index);
	// apply gear settings
	void selectGear(const QModelIndex& index);
	// apply author settings
	void selectAuthor(const QModelIndex& index);
	// apply developer settings
	void selectDeveloper(const QModelIndex& index);

private slots:
	// selection from folder viewer
	void on_fileView_clicked(const QModelIndex& sortIndex);
	// expansion of the tree
	void on_fileView_expanded(const QModelIndex& sortIndex);
	// Apply changes clicked
	void on_applyChangesBtn_clicked();
	// Revert changes clicked
	void on_revertBtn_clicked();
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
};

#endif // ANALOGEXIF_H
