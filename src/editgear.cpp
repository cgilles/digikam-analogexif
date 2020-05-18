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

#include "editgear.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QMenu>
#include <QMessageBox>
#include <QDir>

EditGear::EditGear(QWidget *parent)
	: QDialog(parent), dirty(false)
{
	ui.setupUi(this);

	// start the transaction
	QSqlQuery query;
	//query.exec("BEGIN TRANSACTION");
	query.exec("SAVEPOINT EditGearStart");

	gearList = new EditGearTreeModel(this, 0, true, tr("No equipment defined"));
	gearList->reload();
	ui.gearView->setModel(gearList);
	connect(gearList, SIGNAL(layoutChanged()), this, SLOT(gearList_layoutChanged()));
	connect(gearList, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(gearList_dataChanged(const QModelIndex &, const QModelIndex &)));
	connect(ui.gearView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(gearView_clicked(const QModelIndex &)));
	connect(ui.gearView, SIGNAL(focused()), this, SLOT(gear_focused()));
	connect(ui.gearView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(gear_selectionChanged(const QItemSelection&, const QItemSelection&)));
	ui.gearView->expandAll();

	filmList = new EditGearTreeModel(this, 2, false, tr("No films defined"));
	filmList->reload();
	ui.filmView->setModel(filmList);
	connect(filmList, SIGNAL(layoutChanged()), this, SLOT(gearList_layoutChanged()));
	connect(filmList, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(gearList_dataChanged(const QModelIndex &, const QModelIndex &)));
	connect(ui.filmView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(gearView_clicked(const QModelIndex &)));
	connect(ui.filmView, SIGNAL(focused()), this, SLOT(film_focused()));
	connect(ui.filmView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(film_selectionChanged(const QItemSelection&, const QItemSelection&)));

	developerList = new EditGearTreeModel(this, 3, false, tr("No developers defined"));
	developerList->reload();
	ui.developerView->setModel(developerList);
	connect(developerList, SIGNAL(layoutChanged()), this, SLOT(gearList_layoutChanged()));
	connect(developerList, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(gearList_dataChanged(const QModelIndex &, const QModelIndex &)));
	connect(ui.developerView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(gearView_clicked(const QModelIndex &)));
	connect(ui.developerView, SIGNAL(focused()), this, SLOT(developer_focused()));
	connect(ui.developerView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(developer_selectionChanged(const QItemSelection&, const QItemSelection&)));

	authorList = new EditGearTreeModel(this, 4, false, tr("No authors defined"));
	authorList->reload();
	ui.authorView->setModel(authorList);
	connect(authorList, SIGNAL(layoutChanged()), this, SLOT(gearList_layoutChanged()));
	connect(authorList, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(gearList_dataChanged(const QModelIndex &, const QModelIndex &)));
	connect(ui.authorView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(gearView_clicked(const QModelIndex &)));
	connect(ui.authorView, SIGNAL(focused()), this, SLOT(author_focused()));
	connect(ui.authorView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(author_selectionChanged(const QItemSelection&, const QItemSelection&)));
	
	QAction* separator = new QAction(this);
	separator->setSeparator(true);

	gearContextMenu << ui.actionAdd_new_camera_body << ui.actionAdd_new_camera_lens << ui.actionDuplicate << separator << ui.actionDelete;
	filmContextMenu << ui.actionAdd_new_film << ui.actionDuplicate << separator << ui.actionDelete;
	authorContextMenu << ui.actionAdd_new_author << ui.actionDuplicate << separator << ui.actionDelete;
	developerContextMenu << ui.actionAdd_new_developer << ui.actionDuplicate << separator << ui.actionDelete;

	metadataList = new EditGearTagsModel(this);
	ui.metadataView->setModel(metadataList);
	connect(metadataList, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(metadataList_dataChanged(const QModelIndex&, const QModelIndex&)));
	connect(metadataList, SIGNAL(cleared()), this, SLOT(metadataList_cleared()));
	connect(ui.metadataView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(metadata_selectionChanged(const QItemSelection&, const QItemSelection&)));

	exifItemDelegate = new ExifItemDelegate(this);
	ui.metadataView->setItemDelegateForColumn(1, exifItemDelegate);

	metaTagsMenu = new QMenu(this);
	fillMetaTagsMenu();

	// set the database name in the title
	QString dbName = QDir::fromNativeSeparators(QSqlDatabase::database().databaseName());
	dbName = dbName.right(dbName.length() - dbName.lastIndexOf(QChar('/')) - 1);

	setWindowTitle(tr("Edit equipment (") + dbName + ")[*]");

	setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);

	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

EditGear::~EditGear()
{
	delete gearList;
	delete filmList;
	delete authorList;
	delete developerList;
	delete exifItemDelegate;
	delete metadataList;
	delete metaTagsMenu;
}

void EditGear::on_buttonBox_rejected()
{
	reject();
}

void EditGear::reject()
{
	if(dirty)
	{
		QMessageBox::StandardButton result = QMessageBox::question(this, tr("Unsaved data"),
															tr("User equipment was changed. Save changes?"),
															QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
															QMessageBox::Cancel);

		if(result == QMessageBox::Save)
		{
			saveAndClose();
			return;
		}
		else if (result == QMessageBox::Cancel)
		{
			// just return for cancel
			return;
		}
	}


	// rollback transaction
	QSqlQuery query;
	//query.exec("ROLLBACK TRANSACTION");
	query.exec("ROLLBACK TO EditGearStart");

	QDialog::reject();
}

void EditGear::on_buttonBox_accepted()
{
	saveAndClose();
}

void EditGear::saveAndClose()
{
	// apply transaction
	QSqlQuery query;
	//query.exec("COMMIT TRANSACTION");
	query.exec("RELEASE EditGearStart");

	accept();
}

void EditGear::on_buttonBox_clicked(QAbstractButton* button)
{
	if(ui.buttonBox->standardButton(button) == QDialogButtonBox::Apply)
	{
		QSqlQuery query;
		//query.exec("COMMIT TRANSACTION");
		query.exec("RELEASE EditGearStart");
		query.exec("SAVEPOINT EditGearStart");

		setDirty(false);
	}
}

void EditGear::gearList_layoutChanged()
{
	ui.gearView->expandAll();
	setDirty();
}

void EditGear::on_gearView_customContextMenuRequested(const QPoint &pos)
{
	contextIndex = ui.gearView->indexAt(pos);

	ui.actionAdd_new_camera_body->setEnabled(true);

	if(contextIndex == QModelIndex())
	{
		ui.actionAdd_new_camera_lens->setEnabled(false);
		ui.actionDelete->setEnabled(false);
		ui.actionDuplicate->setEnabled(false);
	}
	else
	{
		int gearType = contextIndex.data(EditGearTreeModel::GetGearTypeRole).toInt();

		// clicked on the lens
		if(gearType == 1)
		{
			ui.actionAdd_new_camera_body->setEnabled(false);
		}

		// enable all potentially disabled
		ui.actionAdd_new_camera_lens->setEnabled(true);
		ui.actionDelete->setEnabled(true);
		ui.actionDuplicate->setEnabled(true);

	}
	QMenu menu(this);
	menu.addActions(gearContextMenu);

	menu.exec(ui.gearView->mapToGlobal(pos));
}

void EditGear::on_filmView_customContextMenuRequested(const QPoint &pos)
{
	contextIndex = ui.filmView->indexAt(pos);

	if(contextIndex == QModelIndex())
	{
		ui.actionDelete->setEnabled(false);
		ui.actionDuplicate->setEnabled(false);
	}
	else
	{
		ui.actionDelete->setEnabled(true);
		ui.actionDuplicate->setEnabled(true);
	}

	QMenu menu(this);
	menu.addActions(filmContextMenu);

	menu.exec(ui.filmView->mapToGlobal(pos));
}

void EditGear::on_authorView_customContextMenuRequested(const QPoint &pos)
{
	contextIndex = ui.authorView->indexAt(pos);

	if(contextIndex == QModelIndex())
	{
		ui.actionDelete->setEnabled(false);
		ui.actionDuplicate->setEnabled(false);
	}
	else
	{
		ui.actionDelete->setEnabled(true);
		ui.actionDuplicate->setEnabled(true);
	}

	QMenu menu(this);
	menu.addActions(authorContextMenu);

	menu.exec(ui.authorView->mapToGlobal(pos));
}

void EditGear::on_developerView_customContextMenuRequested(const QPoint &pos)
{
	contextIndex = ui.developerView->indexAt(pos);

	if(contextIndex == QModelIndex())
	{
		ui.actionDelete->setEnabled(false);
		ui.actionDuplicate->setEnabled(false);
	}
	else
	{
		ui.actionDelete->setEnabled(true);
		ui.actionDuplicate->setEnabled(true);
	}

	QMenu menu(this);
	menu.addActions(developerContextMenu);

	menu.exec(ui.developerView->mapToGlobal(pos));
}

void EditGear::on_actionAdd_new_camera_body_triggered(bool)
{
	// add new camera body
	int newId = gearList->createNewGear(-1, -1, 0, tr("New camera body"), gearList->invisibleRootItem()->rowCount());

	if(newId == -1)
	{
		// error
		QMessageBox::critical(this, tr("Error creating new camera body"), tr("Unable to create a new camera body.\nPlease check that equipment library file is writeable and retry the operation."));
		return;
	}

	QModelIndex newIdx = gearList->reload(newId);
	ui.gearView->expandAll();
	metadataList->reload(newId);
	setDirty();

	ui.gearView->edit(newIdx);
}

void EditGear::on_actionAdd_new_camera_lens_triggered(bool)
{
	int orderBy = gearList->itemFromIndex(contextIndex)->rowCount();

	// add new lens for the selected camera
	int parentId = contextIndex.data(EditGearTreeModel::GetGearIdRole).toInt();
	if(contextIndex.data(EditGearTreeModel::GetGearTypeRole).toInt() == 1)
	{
		parentId = contextIndex.parent().data(EditGearTreeModel::GetGearIdRole).toInt();
		orderBy = gearList->itemFromIndex(contextIndex.parent())->rowCount();
	}

	int newId = gearList->createNewGear(-1, parentId, 1, tr("New camera lens"), orderBy);

	if(newId == -1)
	{
		// error
		QMessageBox::critical(this, tr("Error creating new lens"), tr("Unable to create new lens.\nPlease check that equipment library file is writeable and retry the operation."));
		return;
	}

	QModelIndex newIdx = gearList->reload(newId);
	ui.gearView->expandAll();
	metadataList->reload(newId);
	setDirty();

	ui.gearView->scrollTo(newIdx, QAbstractItemView::EnsureVisible);
	ui.gearView->edit(newIdx);
}

void EditGear::on_actionAdd_new_film_triggered(bool)
{
	// add new film
	int newId = filmList->createNewGear(-1, -1, 2, tr("New film"), filmList->invisibleRootItem()->rowCount());

	if(newId == -1)
	{
		// error
		QMessageBox::critical(this, tr("Error creating new film"), tr("Unable to create a new film.\nPlease check that equipment library file is writeable and retry the operation."));
		return;
	}

	QModelIndex newIdx = filmList->reload(newId);
	metadataList->reload(newId);
	setDirty();

	ui.filmView->scrollTo(newIdx, QAbstractItemView::EnsureVisible);
	ui.filmView->edit(newIdx);
}

void EditGear::on_actionAdd_new_developer_triggered(bool)
{
	// add new film
	int newId = filmList->createNewGear(-1, -1, 3, tr("New developer"), developerList->invisibleRootItem()->rowCount());

	if(newId == -1)
	{
		// error
		QMessageBox::critical(this, tr("Error creating new developer"), tr("Unable to create a new developer.\nPlease check that equipment library file is writeable and retry the operation."));
		return;
	}

	QModelIndex newIdx = developerList->reload(newId);
	metadataList->reload(newId);
	setDirty();

	ui.developerView->scrollTo(newIdx, QAbstractItemView::EnsureVisible);
	ui.developerView->edit(newIdx);
}

void EditGear::on_actionAdd_new_author_triggered(bool)
{
	// add new film
	int newId = authorList->createNewGear(-1, -1, 4, tr("New author"), authorList->invisibleRootItem()->rowCount());

	if(newId == -1)
	{
		// error
		QMessageBox::critical(this, tr("Error creating new author"), tr("Unable to create a new author.\nPlease check that equipment library file is writeable and retry the operation."));
		return;
	}

	QModelIndex newIdx = authorList->reload(newId);
	metadataList->reload(newId);
	setDirty();

	ui.authorView->scrollTo(newIdx, QAbstractItemView::EnsureVisible);
	ui.authorView->edit(newIdx);
}

void EditGear::on_actionDuplicate_triggered(bool)
{
	// duplicate selected gear
	QModelIndexList selectedItems;
	QItemSelectionModel* selModel;

	EditGearTreeModel* treemodel = static_cast<EditGearTreeModel*>((QAbstractItemModel*)contextIndex.model());
	if(treemodel == NULL)
		return;

	int gearType = treemodel->getGearType();

	switch(gearType)
	{
	case 0:
		selModel = ui.gearView->selectionModel();
		break;
	case 2:
		selModel = ui.filmView->selectionModel();
		break;
	case 3:
		selModel = ui.developerView->selectionModel();
		break;
	case 4:
		selModel = ui.authorView->selectionModel();
		break;
	default:
		return;
		break;
	}

	if(selModel)
		selectedItems = selModel->selectedRows();
	else
		return;


	int newId = -1;

    foreach(QModelIndex index, selectedItems)
	{
		switch(gearType)
		{
		case 0:
			{
				int orderBy = gearList->invisibleRootItem()->rowCount();
				int parentId = -1;

				if(index.data(EditGearTreeModel::GetGearTypeRole).toInt() == 1)
				{
					orderBy = gearList->itemFromIndex(index.parent())->rowCount();
					parentId =index.parent().data(EditGearTreeModel::GetGearIdRole).toInt();
				}
				newId = gearList->createNewGear(index.data(EditGearTreeModel::GetGearIdRole).toInt(), parentId, index.data(EditGearTreeModel::GetGearTypeRole).toInt(), tr("Copy of "), orderBy);
			}
			break;
		case 2:
			newId = filmList->createNewGear(index.data(EditGearTreeModel::GetGearIdRole).toInt(), -1, index.data(EditGearTreeModel::GetGearTypeRole).toInt(), tr("Copy of "), filmList->invisibleRootItem()->rowCount());
			break;
		case 3:
			newId = filmList->createNewGear(index.data(EditGearTreeModel::GetGearIdRole).toInt(), -1, index.data(EditGearTreeModel::GetGearTypeRole).toInt(), tr("Copy of "), developerList->invisibleRootItem()->rowCount());
			break;
		case 4:
			newId = filmList->createNewGear(index.data(EditGearTreeModel::GetGearIdRole).toInt(), -1, index.data(EditGearTreeModel::GetGearTypeRole).toInt(), tr("Copy of "), authorList->invisibleRootItem()->rowCount());
			break;
		}
	}

	if(newId == -1)
	{
		// error
		QMessageBox::critical(this, tr("Error duplicating equipment"), tr("Unable to duplicate equipment.\nPlease check that equipment library file is writeable and retry the operation."));
		return;
	}

	treemodel->reload();
	selModel->clearSelection();

	if(gearType == 0)
	{
		ui.gearView->expandAll();
	}

	setDirty();
}

void EditGear::on_actionDelete_triggered(bool)
{
	QModelIndexList selectedItems;
	QItemSelectionModel* selModel;
	QString title, gearTitle;

	EditGearTreeModel* treemodel = static_cast<EditGearTreeModel*>((QAbstractItemModel*)contextIndex.model());
	if(treemodel == NULL)
		return;

	int gearType = treemodel->getGearType();

	switch(gearType)
	{
	case 0:
		title = tr("Delete equipment");
		gearTitle = tr("Are you sure you want to delete this equipment?");
		selModel = ui.gearView->selectionModel();
		break;
	case 2:
		gearTitle = tr("Are you sure you want to delete selected films?");
		selModel = ui.filmView->selectionModel();
		title = tr("Delete films");
		break;
	case 3:
		gearTitle = tr("Are you sure you want to delete selected developers?");
		selModel = ui.developerView->selectionModel();
		title = tr("Delete developers");
		break;
	case 4:
		gearTitle = tr("Are you sure you want to delete selected authors?");
		selModel = ui.authorView->selectionModel();
		title = tr("Delete authors");
		break;
	default:
		return;
		break;
	}

	if(selModel)
		selectedItems = selModel->selectedRows();
	else
		return;

	if(selectedItems.count() == 1)
	{
		gearTitle = tr("Are you sure you want to delete ") + contextIndex.data(Qt::DisplayRole).toString() + "?";

		if(gearType != 0)
			title = title.left(title.length()-1);	// remove 's'
	}

	QMessageBox::StandardButton result = QMessageBox::question(this, title, gearTitle, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

	if(result == QMessageBox::Yes)
	{
		bool deleted;
	    foreach(QModelIndex index, selectedItems) {
			if(!treemodel->deleteGear(index.data(EditGearTreeModel::GetGearIdRole).toInt(), index.data(EditGearTreeModel::GetGearTypeRole).toInt()))
			{
				// error
				QMessageBox::critical(this, tr("Error deleting equipment"), tr("Unable to delete equipment.\nPlease check that equipment library file is writeable and retry the operation."));
				if(deleted)
				{
					// something was deleted - need to update view
					break;
				}
				else
				{
					return;
				}
			}
			// at least one operation was successful
			deleted = true;
		}

		treemodel->reload();

		if(gearType == 0)
		{
			ui.gearView->expandAll();
		}
	}

	selModel->clearSelection();
	metadataList->clear();
	setDirty();
}

void EditGear::gearView_clicked(const QModelIndex& index)
{
	metadataList->reload(index.data(EditGearTreeModel::GetGearIdRole).toInt());
	ui.addTagBtn->setEnabled(true);
	metaTagsMenu->setEnabled(true);

	// clear selection from other views
	// could be done with pointer comparison, but non-illustrative
	EditGearTreeModel* treemodel = static_cast<EditGearTreeModel*>((QAbstractItemModel*)index.model());
	if(treemodel == NULL)
		return;

	int gearType = treemodel->getGearType();

	switch(gearType)
	{
	case 0:
		ui.filmView->clearSelection();
		ui.developerView->clearSelection();
		ui.authorView->clearSelection();
		break;
	case 2:
		ui.gearView->clearSelection();
		ui.developerView->clearSelection();
		ui.authorView->clearSelection();
		break;
	case 3:
		ui.gearView->clearSelection();
		ui.filmView->clearSelection();
		ui.authorView->clearSelection();
		break;
	case 4:
		ui.gearView->clearSelection();
		ui.filmView->clearSelection();
		ui.developerView->clearSelection();
		break;
	}
}

void EditGear::metadataList_dataChanged(const QModelIndex&, const QModelIndex&)
{
	setDirty();
	ui.metadataView->setFocus();
}

void EditGear::gearList_dataChanged(const QModelIndex &, const QModelIndex &)
{
	setDirty();
}

void EditGear::gear_selectionChanged(const QItemSelection&, const QItemSelection&)
{
	bool hasSelection = ui.gearView->selectionModel()->hasSelection();
	ui.delGearBtn->setEnabled(hasSelection);
	ui.addLensBtn->setEnabled(hasSelection);
	ui.dupGearBtn->setEnabled(hasSelection);

	if(!ui.gearView->selectionModel()->hasSelection() && !ui.filmView->selectionModel()->hasSelection() && !ui.developerView->selectionModel()->hasSelection()
		&& !ui.authorView->selectionModel()->hasSelection())
	{
		metadataList->clear();
	}
}

void EditGear::gear_focused()
{
	// clear selection from other categories
	ui.filmView->clearSelection();
	ui.developerView->clearSelection();
	ui.authorView->clearSelection();
}

void EditGear::film_selectionChanged(const QItemSelection&, const QItemSelection&)
{
	bool hasSelection = ui.filmView->selectionModel()->hasSelection();
	ui.delFilmBtn->setEnabled(hasSelection);
	ui.dupFilmBtn->setEnabled(hasSelection);

	if(!ui.gearView->selectionModel()->hasSelection() && !ui.filmView->selectionModel()->hasSelection() && !ui.developerView->selectionModel()->hasSelection()
		&& !ui.authorView->selectionModel()->hasSelection())
	{
		metadataList->clear();
	}
}

void EditGear::film_focused()
{
	// clear selection from other categories
	ui.gearView->clearSelection();
	ui.developerView->clearSelection();
	ui.authorView->clearSelection();
}

void EditGear::developer_selectionChanged(const QItemSelection&, const QItemSelection&)
{
	bool hasSelection = ui.developerView->selectionModel()->hasSelection();
	ui.delDevBtn->setEnabled(hasSelection);
	ui.dupDevBtn->setEnabled(hasSelection);

	if(!ui.gearView->selectionModel()->hasSelection() && !ui.filmView->selectionModel()->hasSelection() && !ui.developerView->selectionModel()->hasSelection()
		&& !ui.authorView->selectionModel()->hasSelection())
	{
		metadataList->clear();
	}
}

void EditGear::developer_focused()
{
	// clear selection from other categories
	ui.gearView->clearSelection();
	ui.filmView->clearSelection();
	ui.authorView->clearSelection();
}

void EditGear::author_selectionChanged(const QItemSelection&, const QItemSelection&)
{
	bool hasSelection = ui.authorView->selectionModel()->hasSelection();
	ui.delAuthorBtn->setEnabled(hasSelection);
	ui.dupAuthorBtn->setEnabled(hasSelection);

	if(!ui.gearView->selectionModel()->hasSelection() && !ui.filmView->selectionModel()->hasSelection() && !ui.developerView->selectionModel()->hasSelection()
		&& !ui.authorView->selectionModel()->hasSelection())
	{
		metadataList->clear();
	}
}

void EditGear::author_focused()
{
	// clear selection from other categories
	ui.gearView->clearSelection();
	ui.filmView->clearSelection();
	ui.developerView->clearSelection();
}

void EditGear::on_delGearBtn_clicked()
{
	// double check
	if(ui.gearView->selectionModel()->hasSelection())
	{
		contextIndex = ui.gearView->selectionModel()->currentIndex();
		ui.actionDelete->trigger();

		ui.gearView->clearSelection();
		ui.delGearBtn->setEnabled(false);
		ui.dupGearBtn->setEnabled(false);
	}
}

void EditGear::on_delFilmBtn_clicked()
{
	// double check
	if(ui.filmView->selectionModel()->hasSelection())
	{
		contextIndex = ui.filmView->selectionModel()->currentIndex();
		ui.actionDelete->trigger();
		
		ui.filmView->clearSelection();
		ui.delFilmBtn->setEnabled(false);
		ui.dupFilmBtn->setEnabled(false);
	}
}

void EditGear::on_delDevBtn_clicked()
{
	// double check
	if(ui.developerView->selectionModel()->hasSelection())
	{
		contextIndex = ui.developerView->selectionModel()->currentIndex();
		ui.actionDelete->trigger();

		ui.developerView->clearSelection();
		ui.delDevBtn->setEnabled(false);
		ui.dupDevBtn->setEnabled(false);
	}
}

void EditGear::on_delAuthorBtn_clicked()
{
	// double check
	if(ui.authorView->selectionModel()->hasSelection())
	{
		contextIndex = ui.authorView->selectionModel()->currentIndex();
		ui.actionDelete->trigger();

		ui.authorView->clearSelection();
		ui.delAuthorBtn->setEnabled(false);
		ui.dupAuthorBtn->setEnabled(false);
	}
}

void EditGear::on_addLensBtn_clicked()
{
	if(ui.gearView->selectionModel()->hasSelection())
	{
		contextIndex = ui.gearView->selectionModel()->currentIndex();
		ui.actionAdd_new_camera_lens->trigger();
	}
}

void EditGear::on_dupGearBtn_clicked()
{
	// double check
	if(ui.gearView->selectionModel()->hasSelection())
	{
		contextIndex = ui.gearView->selectionModel()->currentIndex();
		ui.actionDuplicate->trigger();
	}
}

void EditGear::on_dupFilmBtn_clicked()
{
	// double check
	if(ui.filmView->selectionModel()->hasSelection())
	{
		contextIndex = ui.filmView->selectionModel()->currentIndex();
		ui.actionDuplicate->trigger();
	}
}

void EditGear::on_dupDevBtn_clicked()
{
	// double check
	if(ui.developerView->selectionModel()->hasSelection())
	{
		contextIndex = ui.developerView->selectionModel()->currentIndex();
		ui.actionDuplicate->trigger();
	}
}

void EditGear::on_dupAuthorBtn_clicked()
{
	// double check
	if(ui.authorView->selectionModel()->hasSelection())
	{
		contextIndex = ui.authorView->selectionModel()->currentIndex();
		ui.actionDuplicate->trigger();
	}
}

void EditGear::fillMetaTagsMenu()
{
	metaTagsMenu->setTitle(tr("&Add meta tag..."));

	QMenu* menu = metaTagsMenu->addMenu(tr("Camera body"));
	addMetaTags(menu, 0);
	menu = metaTagsMenu->addMenu(tr("Camera lens"));
	addMetaTags(menu, 1);
	menu = metaTagsMenu->addMenu(tr("Film"));
	addMetaTags(menu, 2);
	menu = metaTagsMenu->addMenu(tr("Developer"));
	addMetaTags(menu, 3);
	menu = metaTagsMenu->addMenu(tr("Author"));
	addMetaTags(menu, 4);
	menu = metaTagsMenu->addMenu(tr("Frame"));
	addMetaTags(menu, 5);

	metaTagsMenu->setEnabled(false);
}

void EditGear::addMetaTags(QMenu* menu, int category)
{
	QSqlQuery query(QString("SELECT b.TagText, b.id FROM GearTemplate a, MetaTags b WHERE a.GearType = %1 AND b.id = a.TagId ORDER BY a.OrderBy").arg(category));

	if(query.lastError().isValid())
		return;

	while(query.next())
	{
		QAction* action = new QAction(query.value(0).toString(), menu);
		// store tag id
		action->setData(query.value(1));

		menu->addAction(action);
	}
}

void EditGear::on_addTagBtn_clicked()
{
	QAction* action = metaTagsMenu->exec(QCursor::pos());

	// if tag selected
	if(action)
	{
		// append new tag to the end of the list
		if(metadataList->addNewTag(action->data().toInt(), metadataList->rowCount()))
		{
			setDirty();
			// select last row (new) and edit it
			QModelIndex idx = metadataList->index(metadataList->rowCount() - 1, 1);
			ui.metadataView->setCurrentIndex(idx);
			ui.metadataView->selectRow(idx.row());
			ui.metadataView->edit(idx);
		}
	}
}

void EditGear::metadata_selectionChanged(const QItemSelection&, const QItemSelection&)
{
	bool enabled = ui.metadataView->selectionModel()->hasSelection();
	ui.delTagBtn->setEnabled(enabled);
	ui.actionDelete_meta_tag->setEnabled(enabled);
}

void EditGear::metadataList_cleared()
{
	ui.metadataView->reset();
	ui.delTagBtn->setEnabled(false);
	ui.addTagBtn->setEnabled(false);
	ui.actionDelete_meta_tag->setEnabled(false);
	metaTagsMenu->setEnabled(false);
}

void EditGear::on_actionDelete_meta_tag_triggered(bool)
{
	QModelIndexList idxList = ui.metadataView->selectionModel()->selectedRows();
	if(idxList.count() == 0)
		return;

	QList<int> idList;

	// collect all ids
	foreach(QModelIndex idx, idxList)
	{
		// get tag id
		QVariant tagId = idx.data(EditGearTagsModel::GetTagIdRole);
		if(tagId != QVariant())
		{
			idList << tagId.toInt();
		}
	}

	// delete accordingly
	foreach(int i, idList)
	{
		// delete and set dirty on success
		if(!metadataList->deleteTag(i))
			return;
	}

	setDirty();
}

void EditGear::on_metadataView_customContextMenuRequested(const QPoint& pos)
{
	QMenu menu(this);
	menu.addMenu(metaTagsMenu);

	menu.addAction(ui.actionDelete_meta_tag);

	QAction* action = menu.exec(ui.metadataView->mapToGlobal(pos));

	// if tag selected
	if(action)
	{
		// append new tag to the end of the list
		if(metadataList->addNewTag(action->data().toInt(), metadataList->rowCount()))
		{
			setDirty();
			// select last row (new) and edit it
			QModelIndex idx = metadataList->index(metadataList->rowCount() - 1, 1);
			ui.metadataView->setCurrentIndex(idx);
			ui.metadataView->selectRow(idx.row());
			ui.metadataView->edit(idx);
		}
	}
}
