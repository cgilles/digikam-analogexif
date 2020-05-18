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

#include "tagselectvalsitemdelegate.h"
#include "edittagselectvalues.h"
#include "optgeartemplatemodel.h"
#include "exifitem.h"

QWidget* TagSelectValsItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
    const QModelIndex &index) const
{
    ExifItem::TagFlags flags = (ExifItem::TagFlags)index.data(OptGearTemplateModel::GetTagFlagsRole).toInt();

    // use standard editor for the format field and open special form for selection values
    if(flags.testFlag(ExifItem::Choice))
        return new EditTagSelectValues((ExifItem::TagType)index.model()->index(index.row(), 3).data(Qt::EditRole).toInt(), flags, parent);
    else
        return QStyledItemDelegate::createEditor(parent, option, index);    
}

void TagSelectValsItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    ExifItem::TagFlags flags = (ExifItem::TagFlags)index.data(OptGearTemplateModel::GetTagFlagsRole).toInt();

    if(flags.testFlag(ExifItem::Choice))
    {
        EditTagSelectValues* editForm = static_cast<EditTagSelectValues*>(editor);

        // set the values
        if(editForm)
            editForm->setValues(index.data(Qt::EditRole).toString());
    }
    else
        QStyledItemDelegate::setEditorData(editor, index);
}

void TagSelectValsItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
    const QModelIndex &index) const
{
    ExifItem::TagFlags flags = (ExifItem::TagFlags)index.data(OptGearTemplateModel::GetTagFlagsRole).toInt();

    if(flags.testFlag(ExifItem::Choice))
    {
        EditTagSelectValues* editForm = static_cast<EditTagSelectValues*>(editor);

        if(editForm)
        {
            if(editForm->result() == QDialog::Accepted)
                model->setData(index, editForm->getValues());
        }
    }
    else
        QStyledItemDelegate::setModelData(editor, model, index);
}

void TagSelectValsItemDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    ExifItem::TagFlags flags = (ExifItem::TagFlags)index.data(OptGearTemplateModel::GetTagFlagsRole).toInt();

    if(!flags.testFlag(ExifItem::Choice))
        QStyledItemDelegate::updateEditorGeometry(editor, option, index);
}
