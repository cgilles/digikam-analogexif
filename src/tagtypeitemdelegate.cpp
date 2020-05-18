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

#include "tagtypeitemdelegate.h"
#include "exifitem.h"

#include <QComboBox>
#include <QStringList>

QWidget* TagTypeItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &,
        const QModelIndex &) const
{
    QComboBox* combo = new QComboBox(parent);

    QStringList items;

    for(int i = ExifItem::TagString; i < ExifItem::TagTypeLast; i++)
        items << ExifItem::typeName((ExifItem::TagType)i);

    combo->addItems(items);
    // combo->setFrame(false);

    QFont f;
    f.setBold(true);
    combo->setFont(f);

    return combo;
}

void TagTypeItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox* combo = static_cast<QComboBox*>(editor);

    combo->setCurrentIndex(index.data(Qt::EditRole).toInt());
}

void TagTypeItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
    const QModelIndex &index) const
{
    QComboBox* combo = static_cast<QComboBox*>(editor);

    model->setData(index, combo->currentIndex());
}

void TagTypeItemDelegate::updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &) const
{
    editor->setGeometry(option.rect.adjusted(0, -2, 0, 2));
}
