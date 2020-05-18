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

#include "edittagselectvalues.h"
#include "exifitem.h"
#include "exiftreemodel.h"

#include <QDateTime>
#include <cmath>

EditTagSelectValues::EditTagSelectValues(ExifItem::TagType dataType, ExifItem::TagFlags flags, QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    selValsModel = new TagSelectValsItemModel(this, dataType, flags);
    ui.selValsView->setModel(selValsModel);

    exifItemDelegate = new ExifItemDelegate(ui.selValsView);
    ui.selValsView->setItemDelegateForColumn(1, exifItemDelegate);

    connect(ui.selValsView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(list_selectionChanged(const QItemSelection&, const QItemSelection&)));

    QAction* separator = new QAction(this);
    separator->setSeparator(true);

    ui.selValsView->addAction(ui.actionAdd_new_row);
    ui.selValsView->addAction(ui.actionMove_row_up);
    ui.selValsView->addAction(ui.actionMove_row_down);
    ui.selValsView->addAction(separator);
    ui.selValsView->addAction(ui.actionDelete_current_row);

    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
}

EditTagSelectValues::~EditTagSelectValues()
{
    delete selValsModel;
    delete exifItemDelegate;
}

// load values
void TagSelectValsItemModel::setValues(const QString& data)
{
    clear();

    QList<QVariantList> values = ExifItem::parseEncodedChoiceList(data, dataType, tagFlags);

    if(values == QList<QVariantList>())
        return;

    foreach(QVariantList valuePair, values)
    {
        QList<QStandardItem*> itemList;

        QStandardItem* valueItem1 = new QStandardItem(valuePair.at(0).toString());
        valueItem1->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);

        QStandardItem* valueItem2 = new QStandardItem("");
        valueItem2->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
        // store value as user data
        valueItem2->setData(valuePair.at(1));
        itemList << valueItem1 << valueItem2;
        // add row
        appendRow(itemList);
    }
}

// return data
QVariant TagSelectValsItemModel::data(const QModelIndex &item, int role) const
{
    if (!item.isValid())
        return QVariant();

    QStandardItem* valueItem = itemFromIndex(item);

    if (role == ExifTreeModel::GetTypeRole)
    {
        return dataType;
    }
    else if (item.column() == 1)
    {
        if ((role == Qt::EditRole) || (role == Qt::DisplayRole) || (role == ExifTreeModel::GetFlagsRole) || (role == ExifTreeModel::GetChoiceRole) || (role == ExifTreeModel::GetTypeRole))
            return ExifTreeModel::getItemData(valueItem->data(), "%1", tagFlags, dataType, role);
    }

    return QStandardItemModel::data(item, role);
}

// construct final string
QString TagSelectValsItemModel::getValues() const
{
    QString result;
    QStandardItem* valueItem;

    for(int i = 0; i < rowCount(); i++)
    {
        // get title
        valueItem = item(i, 0);
        if(valueItem)
        {
            result += (valueItem->text().remove("||").remove(";;") + "||");

            // get type
            valueItem = item(i, 1);

            if(valueItem)
            {
                result += (ExifItem::valueToStringMulti(valueItem->data(), dataType, tagFlags, QVariant()).remove("||").remove(";;") + ";;");
            }
            else
                return "";

        }
        else
            return "";
    }

    return result;
}

bool TagSelectValsItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if((role != Qt::EditRole) || !index.isValid())
        return false;

    if(index.column() == 1)
    {
        // return value according to the tag type
        switch(dataType)
        {
        case ExifItem::TagApertureAPEX:
            {
                double val = value.toDouble();

                val = 2*log(val)/log(2.0);

                itemFromIndex(index)->setData(val);
                break;
            }
        case ExifItem::TagDateTime:
            itemFromIndex(index)->setData(value.toDateTime().toString("yyyy:MM:dd HH:mm:ss"));
            break;
        default:
            itemFromIndex(index)->setData(value);
            break;
        }
        return true;
    }

    return QStandardItemModel::setData(index, value, role);
}

void TagSelectValsItemModel::addRow(const QModelIndex &item)
{
    QList<QStandardItem*> itemList;

    QStandardItem* valueItem1 = new QStandardItem("");
    valueItem1->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
    // store value as user data
    QStandardItem* valueItem2 = new QStandardItem("");
    valueItem2->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
    valueItem2->setData(QVariant());
    itemList << valueItem1 << valueItem2;

    if(item.isValid())
        insertRow(item.row()+1, itemList);
    else
        appendRow(itemList);
}


void EditTagSelectValues::setValues(QString valueList)
{
    selValsModel->setValues(valueList);
}

QString EditTagSelectValues::getValues() const
{
    return selValsModel->getValues();
}

void EditTagSelectValues::on_buttonBox_rejected()
{
    setResult(QDialog::Rejected);
    reject();
}

void EditTagSelectValues::on_buttonBox_accepted()
{
    setResult(QDialog::Accepted);
    accept();
}

void EditTagSelectValues::on_actionAdd_new_row_triggered()
{
    QModelIndex curRow = ui.selValsView->currentIndex();
    selValsModel->addRow(curRow);
    if(curRow.isValid())
        curRow = curRow.sibling(curRow.row()+1, 0);
    else
        curRow = selValsModel->index(selValsModel->rowCount() - 1, 0);

    ui.selValsView->setCurrentIndex(curRow);
    ui.selValsView->edit(curRow);
}

// delete selected values
void EditTagSelectValues::on_actionDelete_current_row_triggered()
{
    if(ui.selValsView->currentIndex().isValid())
        selValsModel->removeRow(ui.selValsView->currentIndex().row());
}

// selection changed - update buttons
void EditTagSelectValues::list_selectionChanged(const QItemSelection&, const QItemSelection&)
{
    if(ui.selValsView->currentIndex().isValid())
    {
        ui.delBtn->setEnabled(true);
        ui.actionDelete_current_row->setEnabled(true);

        int curRow = ui.selValsView->currentIndex().row();
        if(curRow == 0)
        {
            ui.upBtn->setEnabled(false);
            ui.actionMove_row_up->setEnabled(false);
        }
        else
        {
            ui.upBtn->setEnabled(true);
            ui.actionMove_row_up->setEnabled(true);
        }

        if(curRow == (selValsModel->rowCount() - 1))
        {
            ui.downBtn->setEnabled(false);
            ui.actionMove_row_down->setEnabled(false);
        }
        else
        {
            ui.downBtn->setEnabled(true);
            ui.actionMove_row_down->setEnabled(true);
        }
    }
    else
    {
        ui.delBtn->setEnabled(false);
        ui.upBtn->setEnabled(false);
        ui.downBtn->setEnabled(false);

        ui.actionDelete_current_row->setEnabled(false);
        ui.actionMove_row_down->setEnabled(false);
        ui.actionMove_row_up->setEnabled(false);
    }
}

// move current row up
void EditTagSelectValues::on_actionMove_row_up_triggered()
{
    if(ui.selValsView->currentIndex().isValid())
    {
        int curRow = ui.selValsView->currentIndex().row();

        if(curRow == 0)
            return;

        QList<QStandardItem*> curRowItems = selValsModel->takeRow(curRow);
        selValsModel->insertRow(curRow - 1, curRowItems);
        ui.selValsView->selectRow(curRow - 1);
    }
}

// move current row down
void EditTagSelectValues::on_actionMove_row_down_triggered()
{
    if(ui.selValsView->currentIndex().isValid())
    {
        int curRow = ui.selValsView->currentIndex().row();

        if(curRow == (selValsModel->rowCount() - 1))
            return;

        QList<QStandardItem*> curRowItems = selValsModel->takeRow(curRow);
        selValsModel->insertRow(curRow + 1, curRowItems);
        ui.selValsView->selectRow(curRow + 1);
    }
}