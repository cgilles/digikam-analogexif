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

#include "tagnameitemdelegate.h"
#include "optgeartemplatemodel.h"
#include "exifitem.h"
#include "tagnameeditdialog.h"
#include "exiftreemodel.h"

#include <QGroupBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QLineEdit>

QWidget* TagNameItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &,
        const QModelIndex &) const
{

	TagNameEditDialog* tagNameEditDialog = new TagNameEditDialog(parent);

	return tagNameEditDialog;
}

void TagNameItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	TagNameEditDialog* tagNameEditDialog = static_cast<TagNameEditDialog*>(editor);

	tagNameEditDialog->setTagNames(index.data(Qt::EditRole).toString());

	ExifItem::TagFlags flags = (ExifItem::TagFlags)index.data(OptGearTemplateModel::GetTagFlagsRole).toInt();

	ExifItem::TagType tagType = (ExifItem::TagType)index.data(OptGearTemplateModel::GetTagTypeRole).toInt();
	if((tagType != ExifItem::TagString) && (tagType != ExifItem::TagText))
	{
		if(flags.testFlag(ExifItem::AsciiAlt))
			flags &= ~ExifItem::AsciiAlt;

		if(flags.testFlag(ExifItem::Ascii))
			flags &= ~ExifItem::Ascii;

		tagNameEditDialog->setFlagEnabled(ExifItem::AsciiAlt, false);
		tagNameEditDialog->setFlagEnabled(ExifItem::Ascii, false);
	}

	tagNameEditDialog->setFlags(flags);

	if(flags.testFlag(ExifItem::AsciiAlt))
		tagNameEditDialog->setAltTagNames(index.data(OptGearTemplateModel::GetAltTagRole).toString());

	if(OptGearTemplateModel::ProtectBuiltInTags && flags.testFlag(ExifItem::Protected))
		tagNameEditDialog->disableEdit();
}

void TagNameItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
	const QModelIndex &index) const
{
	TagNameEditDialog* tagNameEditDialog = static_cast<TagNameEditDialog*>(editor);

	if(tagNameEditDialog->result() == QDialog::Accepted)
	{
		QVariantList data;
		data << tagNameEditDialog->getTagNames() << (int)tagNameEditDialog->getFlags() << tagNameEditDialog->getAltTagNames();

		model->setData(index, data);
	}
	else
	{
		model->setData(index, QVariant());
	}
}
