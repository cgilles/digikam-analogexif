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

#include "analogexifoptions.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMenu>
#include <QNetworkProxy>
#include <QRegExp>
#include <QRegExpValidator>

#include "exiftreemodel.h"

AnalogExifOptions::AnalogExifOptions(QWidget* const parent)
    : QDialog(parent),
      dirty(false)
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

    connect(ui.gearTemplateView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(gearList_selectionChanged(const QItemSelection&, const QItemSelection&)));

    gearTempList->reload(0);
    ui.gearTemplateView->setColumnWidth(0, 25);
    ui.gearTemplateView->setColumnWidth(1, 125);
    ui.gearTemplateView->setColumnWidth(2, 125);
    ui.gearTemplateView->horizontalHeader()->setStretchLastSection(true);
    ui.gearTemplateView->setColumnHidden(0, true);

    tagTypeEditor = new TagTypeItemDelegate(this);
    ui.gearTemplateView->setItemDelegateForColumn(3, tagTypeEditor);

    tagNameEditor = new TagNameItemDelegate(this);
    ui.gearTemplateView->setItemDelegateForColumn(1, tagNameEditor);

    connect(tagNameEditor, SIGNAL(closeEditor(QWidget*, QAbstractItemDelegate::EndEditHint)), ui.gearTemplateView, SLOT(onCloseEditor(QWidget*, QAbstractItemDelegate::EndEditHint)));

    tagFormatEditor = new TagSelectValsItemDelegate(this);
    ui.gearTemplateView->setItemDelegateForColumn(4, tagFormatEditor);

    QAction* separator = new QAction(this);
    separator->setSeparator(true);

    tempContextMenu << ui.actionAdd_new_tag << ui.actionMove_up << ui.actionMove_down << separator << ui.actionDelete;

    ui.userNsEdit->setValidator(new QRegExpValidator(QRegExp("[a-zA-Z0-9$-_@\\.&+!*\"'(),=;/#?: %\\\\]*"), this));
    ui.userNsPrefix->setValidator(new QRegExpValidator(QRegExp("[a-zA-Z0-9]*"), this));

    loadOptions();

    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);

    ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

AnalogExifOptions::~AnalogExifOptions()
{
    delete tagTypeEditor;
    delete tagNameEditor;
    delete tagFormatEditor;
    delete gearTempList;
}

void AnalogExifOptions::on_gearTypesList_itemClicked(QListWidgetItem*)
{
    gearTempList->reload(ui.gearTypesList->currentRow());
    ui.gearTemplateView->setColumnWidth(0, 25);
    ui.gearTemplateView->setColumnWidth(1, 125);
    ui.gearTemplateView->setColumnWidth(2, 125);
    ui.gearTemplateView->horizontalHeader()->setStretchLastSection(true);
}

// close button
void AnalogExifOptions::on_buttonBox_rejected()
{
    reject();
}

void AnalogExifOptions::reject()
{
    if(dirty)
    {
        QMessageBox::StandardButton result = QMessageBox::question(this, tr("Unsaved preferences"),
                                                            tr("Some preferences were changed. Save changes?"),
                                                            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                                            QMessageBox::Cancel);

        if(result == QMessageBox::Save)
        {
            saveAndExit();
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
    //query.exec("ROLLBACK TRANSACTION");
    query.exec("ROLLBACK TO EditOptionsStart");

    QDialog::reject();
}

// ok button
void AnalogExifOptions::on_buttonBox_accepted()
{
    saveAndExit();
}

void AnalogExifOptions::saveAndExit()
{
    if(!checkOptions())
        return;

    if(!saveOptions())
    {
        QMessageBox::StandardButtons result = QMessageBox::question(this, tr("Unsaved data"),
            tr("The data was not saved. Do you want to abandon changes?"),
            QMessageBox::Yes | QMessageBox::No);

        if(result == QMessageBox::Yes)
        {
            QDialog::reject();
        }

        return;
    }

    accept();
}

// apply button
void AnalogExifOptions::on_buttonBox_clicked(QAbstractButton* button)
{
    if(ui.buttonBox->standardButton(button) == QDialogButtonBox::Apply)
    {
        if(!checkOptions())
            return;

        if(saveOptions())
        {
            // start new transaction
            QSqlQuery query;
            query.exec("SAVEPOINT EditOptionsStart");

            setDirty(false);
        }
    }
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
        //if(contextIndex.data(OptGearTemplateModel::IsProtectedRole).toInt())
        //  ui.actionDelete->setEnabled(false);
        //else
        ui.actionDelete->setEnabled(true);
    }

    QMenu menu(this);
    menu.addActions(tempContextMenu);

    menu.exec(ui.gearTemplateView->mapToGlobal(pos));
}

// delete tag
void AnalogExifOptions::removeTag(QModelIndex idx)
{
    if(!idx.isValid())
        return;

    QMessageBox::StandardButton result = QMessageBox::question(this, tr("Delete tag"), tr("Are you sure you want to delete this tag?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if(result == QMessageBox::Yes)
    {
        // find out gear that uses this tag
        QStringList affectedGear = gearTempList->getTagUsage(idx);
        
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

        gearTempList->removeTag(idx);
    }
}

void AnalogExifOptions::on_actionDelete_triggered(bool)
{
    // check all selections 
    QModelIndexList idxs = ui.gearTemplateView->selectionModel()->selectedRows();

    foreach(QModelIndex idx, idxs)
    {
        removeTag(idx);
    }

    setDirty();
}

// add new tag
void AnalogExifOptions::on_actionAdd_new_tag_triggered(bool)
{
    int newId = gearTempList->insertTag("", tr("New tag"), "%1", ExifItem::TagString);

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

void AnalogExifOptions::loadOptions()
{
    initialState_userNsGBox = initialState_createBkpCbox = initialState_etagsCboxStorageXp = initialState_etagsCboxStorageUser = false;

    // load extra tags storage options
    int etagsStorageOptions = settings.value("ExtraTagsStorage", 0x03).toInt();

    if(etagsStorageOptions & 0x01)
    {
        initialState_etagsCboxStorageUser = true;
        ui.etagsCboxStorageUser->setCheckState(Qt::Checked);
    }

    if(etagsStorageOptions & 0x02)
    {
        initialState_etagsCboxStorageXp = true;
        ui.etagsCboxStorageXp->setCheckState(Qt::Checked);
    }

    // load backup options
    if(settings.value("CreateBackups", true).toBool())
    {
        initialState_createBkpCbox = true;
        ui.createBkpCbox->setChecked(true);
    }
    else
    {
        ui.createBkpCbox->setChecked(false);
    }

    // load user NS options
    originalNs = "";
    originalNsPrefix = "";
    QSqlQuery query("SELECT SetValueText FROM Settings WHERE SetId = 2");
    query.first();
    if(query.isValid())
    {
        originalNs = query.value(0).toString();
        query.exec("SELECT SetValueText FROM Settings WHERE SetId = 3");
        query.first();

        if(!query.isValid())
        {
            QMessageBox::critical(this, tr("Invalid database format"), tr("Invalid or corrupt database data."));
            originalNs = "";
        }
        else
        {
            originalNsPrefix = query.value(0).toString();
            initialState_userNsGBox = true;

            ui.userNsGBox->setChecked(true);

            ui.userNsEdit->setText(originalNs);

            ui.userNsPrefix->setText(originalNsPrefix);

            ui.userNsLabel1->show();
            ui.userNsLabel2->show();
            ui.userNsLabel2->setText(tr("Enter user-defined tags as Xmp.%1.<Tag Name>").arg(originalNsPrefix));
        }

    }

    if(originalNs == "")
    {
        ui.userNsGBox->setChecked(false);
        ui.userNsLabel1->hide();
        ui.userNsLabel2->hide();
    }

    //query.exec("BEGIN TRANSACTION");
    query.exec("SAVEPOINT EditOptionsStart");
}

bool AnalogExifOptions::saveOptions()
{
    // save storage options
    int etagsStorageOptions = 0;

    if(ui.etagsCboxStorageUser->checkState() == Qt::Checked)
        etagsStorageOptions |= 0x01;

    if(ui.etagsCboxStorageXp->checkState() == Qt::Checked)
        etagsStorageOptions |= 0x02;

    settings.setValue("ExtraTagsStorage", etagsStorageOptions);

    // save backup options
    settings.setValue("CreateBackups", ui.createBkpCbox->checkState() == Qt::Checked);

    // delete previous values
    QSqlQuery query("DELETE FROM Settings WHERE SetId = 2 OR SetId = 3");
    
    if((originalNs != "") && ((originalNs != ui.userNsEdit->text()) || (originalNsPrefix != ui.userNsPrefix->text())))
    {
        if(!ExifTreeModel::unregisterUserNs())
        {
            QMessageBox::critical(this, tr("Error unregistering user-defined XMP schema"), tr("Unable to unregister user-defined XMP schema.\nSchema: %1, prefix: %2.\n\n%3.").arg(ui.userNsEdit->text()).arg(ui.userNsPrefix->text()), QMessageBox::Ok);
            return false;
        }
    }

    if(ui.userNsGBox->isChecked())
    {
        query.exec(QString("INSERT INTO Settings(id, SetId, SetValueText) VALUES(1, 2, %1)").arg(ui.userNsEdit->text()));
        if(!query.lastError().isValid())
        {
            QMessageBox::critical(this, tr("Update error"), tr("Unable to store user-defined XMP schema"));
            return false;
        }

        query.exec(QString("INSERT INTO Settings(id, SetId, SetValueText) VALUES(2, 3, %1)").arg(ui.userNsPrefix->text()));
        if(!query.lastError().isValid())
        {
            QMessageBox::critical(this, tr("Update error"), tr("Unable to store user-defined XMP schema"));
            return false;
        }

        // register user ns
        if(!ExifTreeModel::registerUserNs(ui.userNsEdit->text(), ui.userNsPrefix->text()))
        {
            QMessageBox::critical(this, tr("Error registering user-defined XMP schema"), tr("Unable to register user-defined XMP schema.\nSchema: %1, prefix: %2.\n\n%3.").arg(ui.userNsEdit->text()).arg(ui.userNsPrefix->text()), QMessageBox::Ok);
            return false;
        }
    }

    settings.sync();

    initialState_userNsGBox = ui.userNsGBox->isChecked();
    initialState_createBkpCbox = ui.createBkpCbox->isChecked();
    initialState_etagsCboxStorageXp = ui.etagsCboxStorageXp->isChecked();
    initialState_etagsCboxStorageUser = ui.etagsCboxStorageUser->isChecked();

    //query.exec("COMMIT TRANSACTION");
    query.exec("RELEASE EditOptionsStart");

    return true;
}

void AnalogExifOptions::gearList_selectionChanged(const QItemSelection&, const QItemSelection&)
{
    QModelIndex curIndex = ui.gearTemplateView->currentIndex();
    if(curIndex.isValid())
    {
        ui.actionDelete->setEnabled(true);
        ui.delBtn->setEnabled(true);

        ui.upBtn->setEnabled((curIndex.row() != 0));
        ui.actionMove_up->setEnabled((curIndex.row() != 0));

        ui.downBtn->setEnabled((curIndex.row() != (gearTempList->rowCount() - 1)));
        ui.actionMove_down->setEnabled((curIndex.row() != (gearTempList->rowCount() - 1)));
    }
    else
    {
        ui.actionDelete->setEnabled(false);
        ui.delBtn->setEnabled(false);
        ui.downBtn->setEnabled(false);
        ui.actionMove_down->setEnabled(false);
        ui.upBtn->setEnabled(false);
        ui.actionMove_up->setEnabled(false);
    }
}

void AnalogExifOptions::on_userNsPrefix_textEdited(const QString& text)
{
    if(text != "")
    {
        ui.userNsLabel1->show();

        ui.userNsLabel2->setText(tr("Enter user-defined tags as Xmp.%1.<Tag Name>").arg(text));
        ui.userNsLabel2->show();
    }
    else
    {
        ui.userNsLabel1->hide();
        ui.userNsLabel2->hide();
    }

    setDirty();
}

bool AnalogExifOptions::checkOptions()
{
    // check options for validity
    if(ui.userNsGBox->isChecked())
    {
        // ns data should be properly filled
        if(ui.userNsEdit->text() == "")
        {
            QMessageBox::critical(this, tr("User-defined XMP schema data incorrect"), tr("User-defined XMP schema namespace missing.\n\nPlease set the URI of the schema namespace (e.g. http://purl.org/dc/elements/1.1/)."), QMessageBox::Ok);
            ui.tabWidget->setCurrentIndex(0);
            ui.userNsEdit->setFocus();
            return false;
        }
        
        if(ui.userNsPrefix->text() == "")
        {
            QMessageBox::critical(this, tr("User-defined XMP schema data incorrect"), tr("User-defined XMP schema prefix missing."), QMessageBox::Ok);
            ui.tabWidget->setCurrentIndex(0);
            ui.userNsPrefix->setFocus();
            return false;
        }
    }

    return true;
}

// UI interaction that dirty the form
void AnalogExifOptions::on_userNsEdit_textEdited(const QString &)
{
    setDirty();
}
void AnalogExifOptions::on_userNsGBox_toggled(bool state)
{
    if(state != initialState_userNsGBox)
    {
        if((initialState_userNsGBox == true) && (state == false))
        {
            // user disabled sutom namespace - check for the linked meta tags
            QSqlQuery query("SELECT TagText FROM MetaTags WHERE TagName LIKE '%." + ui.userNsPrefix->text() + ".%'");

            query.first();
            if(query.isValid())
            {
                // some tags still use custom namespace
                QMessageBox warning;
                warning.setWindowTitle(tr("Disable custom namespace"));
                warning.setWindowIcon(windowIcon());
                warning.setText(tr("Some tags still use custom namespace.\nIf you disable user namespace you will lose all corresponding metadata tags and associated data for the equipment."));
                warning.setInformativeText(tr("Are you sure you want to disable custom namespace?"));
                
                QString affectedList = tr("The following tags use custom namespace:\n\n");

                do
                {
                    affectedList += "\t" + query.value(0).toString() + "\n";
                } while(query.next());

                warning.setDetailedText(affectedList);

                warning.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                warning.setDefaultButton(QMessageBox::No);
                warning.setIcon(QMessageBox::Warning);

                if(warning.exec() == QMessageBox::No)
                {
                    // set checkbox to original state
                    ui.userNsGBox->setChecked(initialState_userNsGBox);
                    return;
                }

                // remove user properties
                query.exec("DELETE FROM UserGearProperties WHERE TagId IN (SELECT id FROM MetaTags WHERE TagName LIKE '%." + ui.userNsPrefix->text() + ".%')");

                // remove from template
                query.exec("DELETE FROM GearTemplate WHERE TagId IN (SELECT id FROM MetaTags WHERE TagName LIKE '%." + ui.userNsPrefix->text() + ".%')");

                // remove tag
                query.exec("DELETE FROM MetaTags WHERE id = IN (SELECT id FROM MetaTags WHERE TagName LIKE '%." + ui.userNsPrefix->text() + ".%')");
            }
        }

        setDirty();
    }
}
void AnalogExifOptions::on_createBkpCbox_stateChanged(int state)
{
    if((state == Qt::Checked) != initialState_createBkpCbox)
        setDirty();
}
void AnalogExifOptions::on_etagsCboxStorageXp_stateChanged(int state)
{
    if((state == Qt::Checked) != initialState_etagsCboxStorageXp)
        setDirty();
}
void AnalogExifOptions::on_etagsCboxStorageUser_stateChanged(int state)
{
    if((state == Qt::Checked) != initialState_etagsCboxStorageUser)
        setDirty();
}

void AnalogExifOptions::on_updProxyGBox_toggled(bool state)
{
    if(state != initialState_updProxyGBox)
        setDirty();
}

void AnalogExifOptions::on_proxyAddress_textEdited(const QString&)
{
    setDirty();
}

void AnalogExifOptions::on_proxyUsername_textEdited(const QString&)
{
    setDirty();
}

void AnalogExifOptions::on_proxyPassword_textEdited(const QString&)
{
    setDirty();
}

void AnalogExifOptions::on_proxyPort_valueChanged(int newValue)
{
    if(newValue != initialState_proxyPort)
        setDirty();
}

void AnalogExifOptions::on_proxyTypeCBox_currentIndexChanged(int newValue)
{
    if(newValue != initialState_proxyType)
        setDirty();
}

void AnalogExifOptions::on_updCheckInterval_currentIndexChanged(int newValue)
{
    if(newValue != initialState_updCheck)
        setDirty();
}

void AnalogExifOptions::on_actionMove_up_triggered(bool)
{
    QModelIndex curIdx = ui.gearTemplateView->currentIndex();
    if(curIdx.isValid())
    {
        int curRow = curIdx.row();

        // already on top
        if(curRow == 0)
            return;

        gearTempList->swapOrderBys(curIdx, curIdx.sibling(curRow - 1, 0));

        QModelIndex newIdx = gearTempList->index(curRow - 1, 0, curIdx.parent());
        ui.gearTemplateView->setCurrentIndex(newIdx);
        ui.gearTemplateView->selectRow(curRow - 1);
    }

    setDirty();
}

void AnalogExifOptions::on_actionMove_down_triggered(bool)
{
    QModelIndex curIdx = ui.gearTemplateView->currentIndex();
    if(curIdx.isValid())
    {
        int curRow = curIdx.row();

        // already on bottom
        if(curRow == (gearTempList->rowCount() - 1))
            return;

        gearTempList->swapOrderBys(curIdx, curIdx.sibling(curRow + 1, 0));

        QModelIndex newIdx = gearTempList->index(curRow + 1, 0, curIdx.parent());
        ui.gearTemplateView->setCurrentIndex(newIdx);
        ui.gearTemplateView->selectRow(curRow + 1);
    }

    setDirty();
}
