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

#ifndef COPYMETADATADIALOG_H
#define COPYMETADATADIALOG_H

#include <QDialog>
#include <QTreeView>
#include "ui_copymetadatadialog.h"
#include "exiftreemodel.h"
#include "exifitemdelegate.h"

class CheckedExifTreeModel : public ExifTreeModel
{
    Q_OBJECT

public:
    CheckedExifTreeModel(QObject *parent) : ExifTreeModel(parent) { }

    // modify item flags to include checkboxes
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    // change the checked state of all items
    void setChecked(int checked);

    // get checked tag values
    QVariantList getCheckedTags();
};

class CopyMetadataDialog : public QDialog
{
    Q_OBJECT

public:
    CopyMetadataDialog(const QString& fname, QWidget *parent = 0);
    ~CopyMetadataDialog();

    QVariantList getMetadata();

private:
    Ui::CopyMetadataDialogClass ui;

    CheckedExifTreeModel* exifTreeModel;
    ExifItemDelegate* exifItemDelegate;

private slots:
    // on edit double click
    void on_metadataView_doubleClicked(const QModelIndex &);
    // check/uncheck/check not null checkbox state change
    void on_checkUncheckCBox_stateChanged(int state);
    void exifTreeModel_dataChanged(const QModelIndex&, const QModelIndex&);
};

#endif // COPYMETADATADIALOG_H
