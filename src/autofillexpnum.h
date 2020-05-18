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

#ifndef AUTOFILLEXPNUM_H
#define AUTOFILLEXPNUM_H

#include <QDialog>
#include <QStringList>
#include <QStandardItemModel>
#include "ui_autofillexpnum.h"

class AutoFillExpNum : public QDialog
{
	Q_OBJECT

public:
	AutoFillExpNum(QStringList& fileList, QWidget *parent = 0);
	~AutoFillExpNum();

	QVariantList resultFileNames();

private:
	Ui::AutoFillExpNumClass ui;
	QStandardItemModel* files;

private slots:
	// changed data in the model
	void files_dataChanged(const QModelIndex&, const QModelIndex&);
	// changed first exposure number
	void on_firstExpNum_valueChanged(int i);
};

#endif // AUTOFILLEXPNUM_H
