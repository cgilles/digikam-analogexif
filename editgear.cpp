#include "editgear.h"

#include <QSqlQuery>
#include <QMenu>
#include <QMessageBox>

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
	connect(ui.gearView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(gear_selectionChanged(const QItemSelection&, const QItemSelection&)));
	ui.gearView->expandAll();

	filmList = new EditGearTreeModel(this, 2, false, tr("No films defined"));
	filmList->reload();
	ui.filmView->setModel(filmList);
	connect(filmList, SIGNAL(layoutChanged()), this, SLOT(gearList_layoutChanged()));
	connect(filmList, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(gearList_dataChanged(const QModelIndex &, const QModelIndex &)));
	connect(ui.filmView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(gearView_clicked(const QModelIndex &)));
	connect(ui.filmView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(gear_selectionChanged(const QItemSelection&, const QItemSelection&)));

	developerList = new EditGearTreeModel(this, 3, false, tr("No developers defined"));
	developerList->reload();
	ui.developerView->setModel(developerList);
	connect(developerList, SIGNAL(layoutChanged()), this, SLOT(gearList_layoutChanged()));
	connect(developerList, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(gearList_dataChanged(const QModelIndex &, const QModelIndex &)));
	connect(ui.developerView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(gearView_clicked(const QModelIndex &)));
	connect(ui.developerView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(gear_selectionChanged(const QItemSelection&, const QItemSelection&)));

	authorList = new EditGearTreeModel(this, 4, false, tr("No authors defined"));
	authorList->reload();
	ui.authorView->setModel(authorList);
	connect(authorList, SIGNAL(layoutChanged()), this, SLOT(gearList_layoutChanged()));
	connect(authorList, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(gearList_dataChanged(const QModelIndex &, const QModelIndex &)));
	connect(ui.authorView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(gearView_clicked(const QModelIndex &)));
	connect(ui.authorView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(gear_selectionChanged(const QItemSelection&, const QItemSelection&)));
	
	QAction* separator = new QAction(this);
	separator->setSeparator(true);

	gearContextMenu << ui.actionAdd_new_camera_body << ui.actionAdd_new_camera_lens << ui.actionDuplicate << separator << ui.actionDelete;
	filmContextMenu << ui.actionAdd_new_film << ui.actionDuplicate << separator << ui.actionDelete;
	authorContextMenu << ui.actionAdd_new_author << ui.actionDuplicate << separator << ui.actionDelete;
	developerContextMenu << ui.actionAdd_new_developer << ui.actionDuplicate << separator << ui.actionDelete;

	metadataList = new EditGearTagsModel(this);
	ui.metadataView->setModel(metadataList);
	connect(metadataList, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(metadataList_dataChanged(const QModelIndex&, const QModelIndex&)));

	exifItemDelegate = new ExifItemDelegate(this);
	ui.metadataView->setItemDelegateForColumn(1, exifItemDelegate);
}

EditGear::~EditGear()
{
	delete gearList;
	delete filmList;
	delete exifItemDelegate;
	delete metadataList;
}

void EditGear::on_cancelButton_clicked()
{
	if(dirty)
	{
		QMessageBox::StandardButton result = QMessageBox::question(this, tr("Unsaved data"),
															tr("User equipment was changed. Save changes?"),
															QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
															QMessageBox::Cancel);

		if(result == QMessageBox::Save)
		{
			// NB: Baaaad!
			on_okButton_clicked();
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

	reject();
}

void EditGear::on_okButton_clicked()
{
	// apply transaction
	QSqlQuery query;
	//query.exec("COMMIT TRANSACTION");
	query.exec("RELEASE EditGearStart");

	accept();
}

void EditGear::on_applyButton_clicked()
{
	QSqlQuery query;
	//query.exec("COMMIT TRANSACTION");
	query.exec("RELEASE EditGearStart");
	query.exec("SAVEPOINT EditGearStart");

	setDirty(false);
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

void EditGear::on_actionAdd_new_camera_body_triggered(bool checked)
{
	// add new camera body
	int newId = gearList->createNewGear(-1, -1, 0, tr("New camera body"), gearList->invisibleRootItem()->rowCount());

	QModelIndex newIdx = gearList->reload(newId);
	ui.gearView->expandAll();
	metadataList->reload(newId);
	setDirty();

	ui.gearView->edit(newIdx);
}

void EditGear::on_actionAdd_new_camera_lens_triggered(bool checked)
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

	QModelIndex newIdx = gearList->reload(newId);
	ui.gearView->expandAll();
	metadataList->reload(newId);
	setDirty();

	ui.gearView->edit(newIdx);
}

void EditGear::on_actionAdd_new_film_triggered(bool checked)
{
	// add new film
	int newId = filmList->createNewGear(-1, -1, 2, tr("New film"), filmList->invisibleRootItem()->rowCount());

	QModelIndex newIdx = filmList->reload(newId);
	metadataList->reload(newId);
	setDirty();

	ui.filmView->edit(newIdx);
}

void EditGear::on_actionAdd_new_developer_triggered(bool checked)
{
	// add new film
	int newId = filmList->createNewGear(-1, -1, 3, tr("New developer"), developerList->invisibleRootItem()->rowCount());

	QModelIndex newIdx = developerList->reload(newId);
	metadataList->reload(newId);
	setDirty();

	ui.developerView->edit(newIdx);
}

void EditGear::on_actionAdd_new_author_triggered(bool checked)
{
	// add new film
	int newId = authorList->createNewGear(-1, -1, 4, tr("New author"), authorList->invisibleRootItem()->rowCount());

	QModelIndex newIdx = authorList->reload(newId);
	metadataList->reload(newId);
	setDirty();

	ui.authorView->edit(newIdx);
}

void EditGear::on_actionDuplicate_triggered(bool checked)
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
		selectedItems = selModel->selectedIndexes();
	else
		return;


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
				gearList->createNewGear(index.data(EditGearTreeModel::GetGearIdRole).toInt(), parentId, index.data(EditGearTreeModel::GetGearTypeRole).toInt(), tr("Copy of "), orderBy);
			}
			break;
		case 2:
			filmList->createNewGear(index.data(EditGearTreeModel::GetGearIdRole).toInt(), -1, index.data(EditGearTreeModel::GetGearTypeRole).toInt(), tr("Copy of "), filmList->invisibleRootItem()->rowCount());
			break;
		case 3:
			filmList->createNewGear(index.data(EditGearTreeModel::GetGearIdRole).toInt(), -1, index.data(EditGearTreeModel::GetGearTypeRole).toInt(), tr("Copy of "), developerList->invisibleRootItem()->rowCount());
			break;
		case 4:
			filmList->createNewGear(index.data(EditGearTreeModel::GetGearIdRole).toInt(), -1, index.data(EditGearTreeModel::GetGearTypeRole).toInt(), tr("Copy of "), authorList->invisibleRootItem()->rowCount());
			break;
		}
	}

	treemodel->reload();
	selModel->clearSelection();

	if(gearType == 0)
	{
		ui.gearView->expandAll();
	}

	setDirty();
}

void EditGear::on_actionDelete_triggered(bool checked)
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
		selectedItems = selModel->selectedIndexes();
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
	    foreach(QModelIndex index, selectedItems) {
			treemodel->deleteGear(index.data(EditGearTreeModel::GetGearIdRole).toInt(), index.data(EditGearTreeModel::GetGearTypeRole).toInt());
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
}

void EditGear::metadataList_dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
	setDirty();
}

void EditGear::gearList_dataChanged(const QModelIndex &, const QModelIndex &)
{
	setDirty();
}

void EditGear::gear_selectionChanged(const QItemSelection&, const QItemSelection&)
{
	ui.delGearBtn->setEnabled(ui.gearView->selectionModel()->hasSelection());
	ui.addLensBtn->setEnabled(ui.gearView->selectionModel()->hasSelection());
	ui.dupGearBtn->setEnabled(ui.gearView->selectionModel()->hasSelection());

	ui.delFilmBtn->setEnabled(ui.filmView->selectionModel()->hasSelection());
	ui.dupFilmBtn->setEnabled(ui.filmView->selectionModel()->hasSelection());

	ui.delDevBtn->setEnabled(ui.developerView->selectionModel()->hasSelection());
	ui.dupDevBtn->setEnabled(ui.developerView->selectionModel()->hasSelection());

	ui.delAuthorBtn->setEnabled(ui.authorView->selectionModel()->hasSelection());
	ui.dupAuthorBtn->setEnabled(ui.authorView->selectionModel()->hasSelection());
}

void EditGear::on_delGearBtn_clicked()
{
	// double check
	if(ui.gearView->selectionModel()->hasSelection())
	{
		contextIndex = ui.gearView->selectionModel()->currentIndex();
		ui.actionDelete->trigger();
	}
}

void EditGear::on_delFilmBtn_clicked()
{
	// double check
	if(ui.filmView->selectionModel()->hasSelection())
	{
		contextIndex = ui.filmView->selectionModel()->currentIndex();
		ui.actionDelete->trigger();
	}
}

void EditGear::on_delDevBtn_clicked()
{
	// double check
	if(ui.developerView->selectionModel()->hasSelection())
	{
		contextIndex = ui.developerView->selectionModel()->currentIndex();
		ui.actionDelete->trigger();
	}
}

void EditGear::on_delAuthorBtn_clicked()
{
	// double check
	if(ui.authorView->selectionModel()->hasSelection())
	{
		contextIndex = ui.authorView->selectionModel()->currentIndex();
		ui.actionDelete->trigger();
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
