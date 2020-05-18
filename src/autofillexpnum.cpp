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

#include "autofillexpnum.h"

AutoFillExpNum::AutoFillExpNum(QStringList& fileList, QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	files = new QStandardItemModel(this);

	QStringList header;
	header << tr("Filename") << tr("Exposure");
	files->setHorizontalHeaderLabels(header);

	//ui.fileExposures->setColumnWidth(1, 50);

	int nExposures = 1;

	foreach(QString fileName, fileList)
	{
		QList<QStandardItem*> row;

		QStandardItem* fileNameItem = new QStandardItem(fileName);
		fileNameItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		QStandardItem* fileNumItem = new QStandardItem(QString::number(nExposures));
		fileNumItem->setData(nExposures, Qt::EditRole);

		row << fileNameItem << fileNumItem;

		files->appendRow(row);

		nExposures++;
	}

	ui.fileExposures->setModel(files);
	ui.fileExposures->setColumnWidth(0, 175);

	connect(files, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(files_dataChanged(const QModelIndex&, const QModelIndex&)));

	setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
}

AutoFillExpNum::~AutoFillExpNum()
{
	delete files;
}

QVariantList AutoFillExpNum::resultFileNames()
{
	QVariantList values;

	for(int i = 0; i < files->rowCount(); i++)
	{
		values << files->item(i, 0)->text() << files->item(i, 1)->data(Qt::EditRole).toInt();
	}

	return values;
}

// model data changed - resort the list
void AutoFillExpNum::files_dataChanged(const QModelIndex&, const QModelIndex&)
{
	files->sort(1);
}

// increased first exposure number - correct other values
void AutoFillExpNum::on_firstExpNum_valueChanged(int i)
{
	QList<QStandardItem*> children;
	
	for(int j = 0; j < files->rowCount(); j++)
	{
		children << files->item(j, 1);
	}

	int numExp = i;

	foreach(QStandardItem* item, children)
	{
		item->setData(numExp, Qt::EditRole);
		numExp++;
	}
}