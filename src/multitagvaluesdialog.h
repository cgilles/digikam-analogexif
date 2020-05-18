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

#ifndef MULTITAGVALUESDIALOG_H
#define MULTITAGVALUESDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QStandardItemModel>
#include "ui_multitagvaluesdialog.h"

#include "exifitem.h"
#include "exifitemdelegate.h"

class MultiTagValuesItemModel : public QStandardItemModel
{
	Q_OBJECT

public:
	MultiTagValuesItemModel(ExifItem::TagType type, const QString& format, ExifItem::TagFlags flags = ExifItem::None, QObject *parent = 0) :
		QStandardItemModel(parent), dataType(type), tagFormat(format), tagFlags(flags & ~ExifItem::Multi) { }

	// get/set values
	void setValues(const QVariantList& data);
	QVariantList getValues() const;

	// add row
	QModelIndex addRow(const QModelIndex &item);

	QVariant data(const QModelIndex &item, int role) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

private:
	// data type
	ExifItem::TagType dataType;

	// tag print format or choice string
	QString tagFormat;

	// choice tag
	ExifItem::TagFlags tagFlags;
};

class MultiTagValuesDialog : public QDialog
{
	Q_OBJECT

public:
	MultiTagValuesDialog(ExifItem::TagType type, const QString& format, ExifItem::TagFlags flags = ExifItem::None, QWidget *parent = 0);
	~MultiTagValuesDialog();

	void setValues(const QVariantList& values);
	QVariantList getValues();

public slots:
	virtual void reject();

private slots:
	// changes in the selection
	void list_selectionChanged(const QItemSelection&, const QItemSelection&);
	// ok/cancel
	void on_buttonBox_accepted();
	void on_buttonBox_rejected();
	// add new value
	void on_actionAdd_new_value_triggered();
	// delete value
	void on_actionDelete_value_triggered();
	// move row up/down
	void on_actionMove_up_triggered();
	void on_actionMove_down_triggered();

private:
	Ui::MultiTagValuesDialogClass ui;

	// view model
	MultiTagValuesItemModel* itemModel;

	// item delegate for values
	ExifItemDelegate* itemDelegate;
};

#endif // MULTITAGVALUESDIALOG_H
