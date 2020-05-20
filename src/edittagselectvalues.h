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

#ifndef EDITTAGSELECTVALUES_H
#define EDITTAGSELECTVALUES_H

#include <QDialog>
#include <QStandardItemModel>
#include "ui_edittagselectvalues.h"

#include "exifitem.h"
#include "exifitemdelegate.h"

class TagSelectValsItemModel : public QStandardItemModel
{
    Q_OBJECT

public:
    TagSelectValsItemModel(QObject *parent, ExifItem::TagType type, ExifItem::TagFlags flags) :
        QStandardItemModel(parent), dataType(type), tagFlags(flags & ~ExifItem::Choice) { }

    // get/set values
    void setValues(const QString& data);
    QString getValues() const;

    // add row
    void addRow(const QModelIndex &item);

    QVariant data(const QModelIndex &item, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

private:
    // data type
    ExifItem::TagType dataType;
    // tag flags
    ExifItem::TagFlags tagFlags;
};

class EditTagSelectValues : public QDialog
{
    Q_OBJECT

public:
    EditTagSelectValues(ExifItem::TagType dataType, ExifItem::TagFlags flags, QWidget *parent = 0);
    ~EditTagSelectValues();

    // set/get data
    void setValues(QString valueList);
    QString getValues() const;

private:
    Ui::EditTagSelectValuesClass ui;

    // item selection values model
    TagSelectValsItemModel* selValsModel;
    // Exif item edit delegate
    ExifItemDelegate* exifItemDelegate;

private Q_SLOTS:
    // ok button
    void on_buttonBox_accepted();
    // cancel button
    void on_buttonBox_rejected();
    // add button
    void on_actionAdd_new_row_triggered();
    // delete button
    void on_actionDelete_current_row_triggered();
    // up button
    void on_actionMove_row_up_triggered();
    // down button
    void on_actionMove_row_down_triggered();
    // selection changed
    void list_selectionChanged(const QItemSelection&, const QItemSelection&);
};

#endif // EDITTAGSELECTVALUES_H
