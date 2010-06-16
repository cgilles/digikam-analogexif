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

#include "multitagvaluesdialog.h"
#include "exiftreemodel.h"

#include <QDateTime>
#include <cmath>

MultiTagValuesDialog::MultiTagValuesDialog(ExifItem::TagType type, const QString& format, ExifItem::TagFlags flags, QWidget *parent)
: QDialog(parent)
{
	ui.setupUi(this);

	// setup item view
	itemModel = new MultiTagValuesItemModel(type, format, flags, this);
	ui.valuesView->setModel(itemModel);

	// setup item delegate
	itemDelegate = new ExifItemDelegate(this);
	ui.valuesView->setItemDelegateForColumn(0, itemDelegate);

	// check the changes in the selection
	connect(ui.valuesView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(list_selectionChanged(const QItemSelection&, const QItemSelection&)));

	// setup context menu
	QAction* separator = new QAction(this);
	separator->setSeparator(true);

	ui.valuesView->addAction(ui.actionAdd_new_value);
	ui.valuesView->addAction(ui.actionMove_up);
	ui.valuesView->addAction(ui.actionMove_down);
	ui.valuesView->addAction(separator);
	ui.valuesView->addAction(ui.actionDelete_value);

	setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
}

MultiTagValuesDialog::~MultiTagValuesDialog()
{
	delete itemDelegate;
	delete itemModel;
}

// selection changed - update buttons
void MultiTagValuesDialog::list_selectionChanged(const QItemSelection&, const QItemSelection&)
{
	if(ui.valuesView->currentIndex().isValid())
	{
		ui.delBtn->setEnabled(true);
		ui.actionDelete_value->setEnabled(true);

		int curRow = ui.valuesView->currentIndex().row();
		if(curRow == 0)
		{
			ui.upBtn->setEnabled(false);
			ui.actionMove_up->setEnabled(false);
		}
		else
		{
			ui.upBtn->setEnabled(true);
			ui.actionMove_up->setEnabled(true);
		}

		if(curRow == (itemModel->rowCount() - 1))
		{
			ui.downBtn->setEnabled(false);
			ui.actionMove_down->setEnabled(false);
		}
		else
		{
			ui.downBtn->setEnabled(true);
			ui.actionMove_down->setEnabled(true);
		}
	}
	else
	{
		ui.delBtn->setEnabled(false);
		ui.upBtn->setEnabled(false);
		ui.downBtn->setEnabled(false);

		ui.actionDelete_value->setEnabled(false);
		ui.actionMove_down->setEnabled(false);
		ui.actionMove_up->setEnabled(false);
	}
}

// ok/cancel handlers
void MultiTagValuesDialog::on_buttonBox_rejected()
{
	setResult(QDialog::Rejected);
	reject();
}

void MultiTagValuesDialog::on_buttonBox_accepted()
{
	setResult(QDialog::Accepted);
	accept();
}

void MultiTagValuesDialog::setValues(const QVariantList& values)
{
	itemModel->setValues(values);
}

QVariantList MultiTagValuesDialog::getValues()
{
	return itemModel->getValues();
}

void MultiTagValuesDialog::on_actionAdd_new_value_triggered()
{
	QModelIndex idx = itemModel->addRow(ui.valuesView->currentIndex());
	if(idx.isValid())
	{
		ui.valuesView->setCurrentIndex(idx);
		ui.valuesView->edit(idx);
		ui.valuesView->selectRow(idx.row());
	}
}

// delete selected values
void MultiTagValuesDialog::on_actionDelete_value_triggered()
{
	if(ui.valuesView->currentIndex().isValid())
		itemModel->removeRow(ui.valuesView->currentIndex().row());
}

// move current row up
void MultiTagValuesDialog::on_actionMove_up_triggered()
{
	if(ui.valuesView->currentIndex().isValid())
	{
		int curRow = ui.valuesView->currentIndex().row();

		if(curRow == 0)
			return;

		QList<QStandardItem*> curRowItems = itemModel->takeRow(curRow);
		itemModel->insertRow(curRow - 1, curRowItems);
		ui.valuesView->selectRow(curRow - 1);
	}
}

// move current row down
void MultiTagValuesDialog::on_actionMove_down_triggered()
{
	if(ui.valuesView->currentIndex().isValid())
	{
		int curRow = ui.valuesView->currentIndex().row();

		if(curRow == (itemModel->rowCount() - 1))
			return;

		QList<QStandardItem*> curRowItems = itemModel->takeRow(curRow);
		itemModel->insertRow(curRow + 1, curRowItems);
		ui.valuesView->selectRow(curRow + 1);
	}
}

// set data for the model
void MultiTagValuesItemModel::setValues(const QVariantList& data)
{
	clear();

	foreach(QVariant value, data)
	{
		// add new row
		QStandardItem* newItem = new QStandardItem(ExifTreeModel::getItemData(value, tagFormat, tagFlags, (ExifItem::TagType)dataType).toString());
		newItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
		newItem->setData(value);

		appendRow(newItem);
	}
}

// get selected values
QVariantList MultiTagValuesItemModel::getValues() const
{
	QVariantList result;
	QStandardItem* valueItem;

	for(int i = 0; i < rowCount(); i++)
	{
		// get title
		valueItem = item(i);
		if(valueItem)
		{
			result << valueItem->data();
		}
		else
		{
			return QVariantList();
		}
	}

	return result;
}

// adds row after specified index or appends it to the list
QModelIndex MultiTagValuesItemModel::addRow(const QModelIndex &item)
{
	QStandardItem* valueItem = new QStandardItem("");
	valueItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
	valueItem->setData(QVariant());

	if(item.isValid())
		insertRow(item.row()+1, valueItem);
	else
		appendRow(valueItem);

	return indexFromItem(valueItem);
}

// get item data
QVariant MultiTagValuesItemModel::data(const QModelIndex &item, int role) const
{
	if (!item.isValid())
		return QVariant();

	QStandardItem* valueItem = itemFromIndex(item);

	if ((role == Qt::EditRole) || (role == Qt::DisplayRole) || (role == ExifTreeModel::GetFlagsRole) || (role == ExifTreeModel::GetChoiceRole) || (role == ExifTreeModel::GetTypeRole))
		return ExifTreeModel::getItemData(valueItem->data(), tagFormat, tagFlags, dataType, role);

	// leave unhandled roles for standard handler
	return QStandardItemModel::data(item, role);
}

// set item data
bool MultiTagValuesItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if(role == Qt::EditRole)
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
