#ifndef EDITGEAR_H
#define EDITGEAR_H

#include <QDialog>
#include <QAction>
#include "ui_editgear.h"

#include "editgeartreemodel.h"
#include "exifitemdelegate.h"
#include "editgeartagsmodel.h"

class EditGear : public QDialog
{
	Q_OBJECT

public:
	EditGear(QWidget *parent = 0);
	~EditGear();

private:
	Ui::EditGearClass ui;

	EditGearTreeModel* gearList;
	EditGearTreeModel* filmList;
	EditGearTreeModel* authorList;
	EditGearTreeModel* developerList;

	EditGearTagsModel* metadataList;
	
	// custom item editor
	ExifItemDelegate* exifItemDelegate;

	QList<QAction*> gearContextMenu, filmContextMenu, authorContextMenu, developerContextMenu;

	QModelIndex contextIndex;

	bool dirty;

	void setDirty(bool isDirty = true)
	{
		dirty = isDirty;
		ui.applyButton->setEnabled(isDirty);
		setWindowModified(isDirty);
	}

private slots:
	// close button
	void on_cancelButton_clicked();
	// ok button
	void on_okButton_clicked();
	// apply button
	void on_applyButton_clicked();
	// layout changed
	void gearList_layoutChanged();
	// data changed
	void gearList_dataChanged(const QModelIndex &, const QModelIndex &);
	// gear view context menu
	void on_gearView_customContextMenuRequested(const QPoint& pos);
	// film view context menu
	void on_filmView_customContextMenuRequested(const QPoint& pos);
	// developer view context menu
	void on_developerView_customContextMenuRequested(const QPoint& pos);
	// author view context menu
	void on_authorView_customContextMenuRequested(const QPoint& pos);
	// gear selected
	void gearView_clicked(const QModelIndex& index);
	// data changed
	void metadataList_dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
	// selection changed
	void gear_selectionChanged(const QItemSelection&, const QItemSelection&);

	// add gear mini-button
	void on_addLensBtn_clicked();
	// delete mini-buttons
	void on_delGearBtn_clicked();
	void on_delFilmBtn_clicked();
	void on_delDevBtn_clicked();
	void on_delAuthorBtn_clicked();
	// duplicate mini-buttons
	void on_dupGearBtn_clicked();
	void on_dupFilmBtn_clicked();
	void on_dupDevBtn_clicked();
	void on_dupAuthorBtn_clicked();

	// menu actions
	// add new camera
	void on_actionAdd_new_camera_body_triggered(bool checked = false);
	// add new lens
	void on_actionAdd_new_camera_lens_triggered(bool checked = false);
	// add new film
	void on_actionAdd_new_film_triggered(bool checked = false);
	// add new developer
	void on_actionAdd_new_developer_triggered(bool checked = false);
	// add new author
	void on_actionAdd_new_author_triggered(bool checked = false);
	// duplicate
	void on_actionDuplicate_triggered(bool checked = false);
	// delete
	void on_actionDelete_triggered(bool checked = false);
};

#endif // EDITGEAR_H
