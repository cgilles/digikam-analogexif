#include "analogexifoptions.h"
#include <QSqlQuery>
#include <QMenu>
#include <QMessageBox>

AnalogExifOptions::AnalogExifOptions(QWidget *parent)
	: QDialog(parent), dirty(false)
{
	ui.setupUi(this);

	// setup the custom gear templates tab
	ui.gearTypesList->addItem(tr("Camera body"));
	ui.gearTypesList->addItem(tr("Camera lens"));
	ui.gearTypesList->addItem(tr("Film"));
	ui.gearTypesList->addItem(tr("Developer"));
	ui.gearTypesList->addItem(tr("Author"));
	ui.gearTypesList->addItem(tr("Frame"));
	ui.gearTypesList->setCurrentRow(0);

	gearTempList = new OptGearTemplateModel(this);
	ui.gearTemplateView->setModel(gearTempList);
	connect(gearTempList, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(gearTempList_dataChanged(const QModelIndex&, const QModelIndex&)));

	gearTempList->reload(0);
	ui.gearTemplateView->setColumnWidth(0, 25);
#ifndef DEVELOPMENT_VERSION
	ui.gearTemplateView->setColumnWidth(1, 125);
	ui.gearTemplateView->setColumnWidth(2, 125);
#endif
	ui.gearTemplateView->horizontalHeader()->setStretchLastSection(true);

	tagTypeEditor = new TagTypeItemDelegate(this);
	ui.gearTemplateView->setItemDelegateForColumn(3, tagTypeEditor);

	QAction* separator = new QAction(this);
	separator->setSeparator(true);

	tempContextMenu << ui.actionAdd_new_tag << separator << ui.actionDelete;

	int etagsStorageOptions = settings.value("extraTagsStorage", 0x03).toInt();

	if(etagsStorageOptions & 0x01)
		ui.etagsCboxStorageUser->setCheckState(Qt::Checked);

	if(etagsStorageOptions & 0x02)
		ui.etagsCboxStorageXp->setCheckState(Qt::Checked);

	// start the transaction
	QSqlQuery query;
	//query.exec("BEGIN TRANSACTION");
	query.exec("SAVEPOINT EditOptionsStart");
}

AnalogExifOptions::~AnalogExifOptions()
{
	delete tagTypeEditor;
	delete gearTempList;
}

void AnalogExifOptions::on_gearTypesList_itemClicked(QListWidgetItem* item)
{
	gearTempList->reload(ui.gearTypesList->currentRow());
	ui.gearTemplateView->setColumnWidth(0, 25);
#ifndef DEVELOPMENT_VERSION
	ui.gearTemplateView->setColumnWidth(1, 125);
	ui.gearTemplateView->setColumnWidth(2, 125);
#endif
	ui.gearTemplateView->horizontalHeader()->setStretchLastSection(true);
}

// close button
void AnalogExifOptions::on_cancelButton_clicked()
{
	if(dirty)
	{
		QMessageBox::StandardButton result = QMessageBox::question(this, tr("Unsaved preferences"),
															tr("Some preferences were changed. Save changes?"),
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

	// apply transaction
	QSqlQuery query;
	//query.exec("COMMIT TRANSACTION");
	query.exec("ROLLBACK EditOptionsStart");

	reject();
}

// ok button
void AnalogExifOptions::on_okButton_clicked()
{
	saveOptions();

	accept();
}

// apply button
void AnalogExifOptions::on_applyButton_clicked()
{
	saveOptions();

	QSqlQuery query;
	query.exec("SAVEPOINT EditOptionsStart");

	setDirty(false);
}

// context menu
void AnalogExifOptions::on_gearTemplateView_customContextMenuRequested(const QPoint& pos)
{
	contextIndex = ui.gearTemplateView->indexAt(pos);

	if(contextIndex == QModelIndex())
	{
		ui.actionDelete->setEnabled(false);
	}
	else
	{
#ifndef DEVELOPMENT_VERSION
		if(contextIndex.data(OptGearTemplateModel::IsProtectedRole).toInt())
			ui.actionDelete->setEnabled(false);
		else
#endif
		ui.actionDelete->setEnabled(true);
	}

	QMenu menu(this);
	menu.addActions(tempContextMenu);

	menu.exec(ui.gearTemplateView->mapToGlobal(pos));
}

// delete tag
void AnalogExifOptions::on_actionDelete_triggered(bool checked)
{
	QMessageBox::StandardButton result = QMessageBox::question(this, tr("Delete tag"), tr("Are you sure you want to delete this tag?"),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

	if(result == QMessageBox::Yes)
	{
		// find out gear that uses this tag
		QStringList affectedGear = gearTempList->getTagUsage(contextIndex);
		
		if(affectedGear.count())
		{
			QMessageBox warning;
			warning.setWindowTitle(tr("Delete tag"));
			warning.setWindowIcon(windowIcon());
			warning.setText(tr("Some equipment still uses this tag.\nIf you delete this tag you will lose all associated information."));
			warning.setInformativeText(tr("Are you sure you want to delete this tag?"));
			
			QString affectedList = tr("The following equipment uses this tag:\n\n");
			foreach(QString str, affectedGear)
			{
				affectedList += "\t" + str + "\n";
			}
			warning.setDetailedText(affectedList);

			warning.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
			warning.setDefaultButton(QMessageBox::No);
			warning.setIcon(QMessageBox::Warning);

			if(warning.exec() == QMessageBox::No)
				return;
		}

		gearTempList->removeTag(contextIndex);
	}

	setDirty();
}

// add new tag
void AnalogExifOptions::on_actionAdd_new_tag_triggered(bool checked)
{
	int newId = gearTempList->insertTag("", tr("New EXIF tag"), "%1", 0);

	if(newId != -1)
	{
		ui.gearTemplateView->edit(gearTempList->index(gearTempList->rowCount() - 1, 1));
	}

	setDirty();
}

void AnalogExifOptions::gearTempList_dataChanged(const QModelIndex&, const QModelIndex&)
{
	setDirty();
}

void AnalogExifOptions::saveOptions()
{
	// save storage options
	int etagsStorageOptions = 0;

	if(ui.etagsCboxStorageUser->checkState() == Qt::Checked)
		etagsStorageOptions |= 0x01;

	if(ui.etagsCboxStorageXp->checkState() == Qt::Checked)
		etagsStorageOptions |= 0x02;

	settings.setValue("extraTagsStorage", etagsStorageOptions);

	// save database
	QSqlQuery query;
	//query.exec("COMMIT TRANSACTION");
	query.exec("RELEASE EditOptionsStart");
}