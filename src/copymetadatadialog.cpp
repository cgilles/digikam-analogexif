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

#include "copymetadatadialog.h"

CopyMetadataDialog::CopyMetadataDialog(const QString& fname, QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    exifTreeModel = new CheckedExifTreeModel(this);
    connect(exifTreeModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(exifTreeModel_dataChanged(const QModelIndex&, const QModelIndex&)));

    exifItemDelegate = new ExifItemDelegate(this);

    ui.metadataView->setModel(exifTreeModel);
    ui.metadataView->setItemDelegateForColumn(1, exifItemDelegate);

    exifTreeModel->openFile(fname);

    for(int i = 0; i < exifTreeModel->rowCount(); i++)
    {
        ui.metadataView->setFirstColumnSpanned(i, QModelIndex(), true);
    }
    ui.metadataView->header()->resizeSection(0, 200);
    ui.metadataView->expandAll();

    ui.checkUncheckCBox->setCheckState(Qt::PartiallyChecked);

    // enable editing
    exifTreeModel->setReadonly(false);  

    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
}

CopyMetadataDialog::~CopyMetadataDialog()
{
    delete exifItemDelegate;
    delete exifTreeModel;
}

QVariantList CopyMetadataDialog::getMetadata()
{
    return exifTreeModel->getCheckedTags();
}

void CopyMetadataDialog::on_metadataView_doubleClicked(const QModelIndex& index)
{
    if(index.isValid() && (index.column() == 1))
    {
        ui.metadataView->edit(index);
    }
}

void CopyMetadataDialog::on_checkUncheckCBox_stateChanged(int state)
{
    exifTreeModel->setChecked(state);
}

void CopyMetadataDialog::exifTreeModel_dataChanged(const QModelIndex& topLeft, const QModelIndex&)
{
    if(topLeft.isValid())
    {
        exifTreeModel->setData(topLeft, Qt::Checked, Qt::CheckStateRole);
    }
}

// add checkboxes on items
Qt::ItemFlags CheckedExifTreeModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = ExifTreeModel::flags(index);

    if((flags != 0) && (index.column() == 0))
        flags |= Qt::ItemIsUserCheckable | Qt::ItemIsEditable;

    return flags;
}

QVariant CheckedExifTreeModel::data(const QModelIndex &index, int role) const
{
    // ignore "dirty" font decorations
    if(role == Qt::FontRole)
        return QVariant();

    // pass to ExifTreeModel if not check state
    if (role != Qt::CheckStateRole)
        return ExifTreeModel::data(index, role);

    if (!index.isValid())
        return QVariant();

    ExifItem *item = getItem(index);

    // for caption and column 0 - return caption
    if((index.column() == 0) && !item->isCaption())
    {
        if(item->isChecked())
            return Qt::Checked;
        else
            return Qt::Unchecked;
    }

    return QVariant();
}

bool CheckedExifTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::CheckStateRole)
        return ExifTreeModel::setData(index, value, role);

    if (!index.isValid())
        return false;

    ExifItem *item = getItem(index);

    // for caption and column 0 - return caption
    if(/*(index.column() == 0) && */!item->isCaption())
    {
        item->setChecked(value.toInt() == Qt::Checked);
        return true;
    }

    return false;
}

void CheckedExifTreeModel::setChecked(int checked)
{
    for(int i = 0; i < rootItem->childCount(); i++)
    {
        ExifItem* category = rootItem->child(i);
        for(int j = 0; j < category->childCount(); j++)
        {
            ExifItem* tag = category->child(j);

            if(checked == Qt::PartiallyChecked)
            {
                if(tag->value() == QVariant())
                    tag->setChecked(false);
                else
                    tag->setChecked(true);
            }
            else
            {
                tag->setChecked(checked);
            }
        }
    }

    emit dataChanged(QModelIndex(), QModelIndex());
}

QVariantList CheckedExifTreeModel::getCheckedTags()
{
    QVariantList checkedValues;

    for(int i = 0; i < rootItem->childCount(); i++)
    {
        ExifItem* category = rootItem->child(i);
        for(int j = 0; j < category->childCount(); j++)
        {
            ExifItem* tag = category->child(j);

            if(tag->isChecked())
                checkedValues << qVariantFromValue((void*)tag);
        }
    }

    return checkedValues;
}
