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
#include "exiftreemodel.h"

#include <QGroupBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QLineEdit>

QWidget* TagNameItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
	const QModelIndex &index) const
{
	QWidget* editor = new QWidget(parent);

	QVBoxLayout *vbox = new QVBoxLayout(editor);
	editor->setLayout(vbox);

	vbox->setContentsMargins(0, 0, 0, 0);
	vbox->setSpacing(0);

	// add tag name editor
	QLineEdit* edit = new QLineEdit(editor);
	edit->setPalette(option.palette);

	vbox->addWidget(edit);
	editor->setFocusProxy(edit);

	// add flags checkboxes
	QGroupBox* gbox = new QGroupBox(editor);
	QVBoxLayout *vbox1 = new QVBoxLayout(gbox);
	gbox->setAutoFillBackground(true);
	gbox->setPalette(option.palette);

	int startItem = 1;
	if(OptGearTemplateModel::ProtectBuiltInTags)
		startItem = 2;

	for(int i = startItem; i < ExifItem::Last; i *= 2)
		vbox1->addWidget(new QCheckBox(ExifItem::flagName((ExifItem::TagFlag)i), editor));

	vbox->addWidget(gbox);

	return editor;
}

void TagNameItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	// scan through checkboxes and set them according to the flags
	bool swappedLayout = false;
	QGroupBox* gbox = dynamic_cast<QGroupBox*>(editor->layout()->itemAt(1)->widget());

	if(!gbox)
	{
		// widgets may have been swapped
		swappedLayout = true;

		gbox = static_cast<QGroupBox*>(editor->layout()->itemAt(0)->widget());
	}

	ExifItem::TagFlags flags = index.data(OptGearTemplateModel::GetTagFlagsRole).toInt();

	if(flags)
	{
		int nItems = gbox->layout()->count();

		int startItem = 0;
		if(OptGearTemplateModel::ProtectBuiltInTags)
			startItem = 1;

		for(int i = 0; i < nItems; i++)
		{
			QCheckBox* cbx = static_cast<QCheckBox*>(gbox->layout()->itemAt(i)->widget());
			if(cbx)
			{
				if(flags.testFlag((ExifItem::TagFlag)(1 << (i + startItem))))
					cbx->setCheckState(Qt::Checked);
				else
					cbx->setCheckState(Qt::Unchecked);
			}
		}
	}

	// set edit text
	QLineEdit* lineEdit;
	if(swappedLayout)
		 lineEdit = static_cast<QLineEdit*>(editor->layout()->itemAt(1)->widget());
	else
		lineEdit = static_cast<QLineEdit*>(editor->layout()->itemAt(0)->widget());

	if(lineEdit)
	{
		lineEdit->setText(index.data(Qt::EditRole).toString());
	}
}

void TagNameItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
	const QModelIndex &index) const
{
	QVariantList data;

	// set edit text
	bool swappedLayout = false;
	QLineEdit* lineEdit = dynamic_cast<QLineEdit*>(editor->layout()->itemAt(0)->widget());

	if(!lineEdit)
	{
		swappedLayout = true;
		lineEdit = static_cast<QLineEdit*>(editor->layout()->itemAt(1)->widget());
	}

	if(lineEdit->text() == "")
		data << QVariant();
	else
		data << lineEdit->text();

	// scan through checkboxes and set them according to the flags
	QGroupBox* gbox;
	
	if(swappedLayout)
		gbox = static_cast<QGroupBox*>(editor->layout()->itemAt(0)->widget());
	else
		gbox = static_cast<QGroupBox*>(editor->layout()->itemAt(1)->widget());

	int nItems = gbox->layout()->count();
	ExifItem::TagFlags flags = 0;

	for(int i = 0; i < nItems; i++)
	{

		int startItem = 0;
		if(OptGearTemplateModel::ProtectBuiltInTags)
			startItem = 1;

		QCheckBox* cbx = static_cast<QCheckBox*>(gbox->layout()->itemAt(i)->widget());
		if(cbx)
		{
			if(cbx->checkState() == Qt::Checked)
				flags |= (ExifItem::TagFlag)(1 << (i + startItem));
		}
	}

	data << (int)flags;
	model->setData(index, data);
}

void TagNameItemDelegate::updateEditorGeometry(QWidget *editor,
	const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QLineEdit* lineEdit = static_cast<QLineEdit*>(editor->layout()->itemAt(0)->widget());

	if(lineEdit)
	{
		lineEdit->setMaximumHeight(option.rect.height());
		lineEdit->setMaximumWidth(option.rect.width());
	}

	QGroupBox* gbox = static_cast<QGroupBox*>(editor->layout()->itemAt(1)->widget());

	if(gbox)
	{
		int nItems = gbox->layout()->count();
		int maxWidth = 0;
		for(int i = 0; i < nItems; i++)
		{
			QCheckBox* cbx = static_cast<QCheckBox*>(gbox->layout()->itemAt(i)->widget());
			if(cbx)
			{
				int width = option.fontMetrics.width(cbx->text()) + gbox->layout()->contentsMargins().left() + gbox->layout()->contentsMargins().right() + option.fontMetrics.width("WWW");
				if(width > maxWidth)
					maxWidth = width;
			}
		}

		if(maxWidth > option.rect.width())
			maxWidth -= option.rect.width();
		else
			maxWidth = 0;

		QRect adjRect = option.rect.adjusted(0, 0, maxWidth, option.rect.height()*(nItems+3));
		int parentWidth = editor->parentWidget()->geometry().width();
		int parentHeight = editor->parentWidget()->geometry().height();

		// if exceeds parent view, move checkbox menu on top of the edit row
		if(adjRect.bottom() > parentHeight)
		{
			adjRect = adjRect.adjusted(0, -(adjRect.height() - option.rect.height()), 0, -(adjRect.height() - option.rect.height()));

			editor->layout()->addWidget(editor->layout()->takeAt(0)->widget());
		}
			
		editor->setGeometry(adjRect);
	}
}
