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

#include "tagnameeditdialog.h"
#include "optgeartemplatemodel.h"
#include <QCheckBox>

TagNameEditDialog::TagNameEditDialog(QWidget *parent, const QString& tagNames, ExifItem::TagFlags flags, const QString& altTags)
	: QDialog(parent), altTagNames(NULL)
{
	ui.setupUi(this);

	ui.tagNamesEdit->setText(tagNames);

	for(int i = 1; i < ExifItem::Last; i *= 2)
	{
		QCheckBox* cbx = new QCheckBox(ExifItem::flagName((ExifItem::TagFlag)i), ui.optionsBox);
		ui.optionsBox->layout()->addWidget(cbx);

		if((i == ExifItem::Protected) && OptGearTemplateModel::ProtectBuiltInTags)
			cbx->setEnabled(false);

		if(i == ExifItem::AsciiAlt)
		{
			altTagNames = new QLineEdit(ui.optionsBox);
			altTagNames->setEnabled(false);
			altTagNames->setText(altTags);

			altTagCbox = cbx;

			ui.optionsBox->layout()->addWidget(altTagNames);

			connect(cbx, SIGNAL(stateChanged(int)), this, SLOT(altTag_stateChanged(int)));
		}

		if(i == ExifItem::Ascii)
		{
			asciiTagCbox = cbx;
			connect(cbx, SIGNAL(stateChanged(int)), this, SLOT(ascii_stateChanged(int)));
		}
	}
}

void TagNameEditDialog::setFlags(ExifItem::TagFlags flags)
{
	for(int i = 1, j = 0; i < ExifItem::Last; i *= 2, j++)
	{
		QCheckBox* cbx = static_cast<QCheckBox*>(ui.optionsBox->layout()->itemAt(j)->widget());
		cbx->setChecked(flags.testFlag((ExifItem::TagFlag)i));

		if(i == ExifItem::AsciiAlt)
			altTagNames->setEnabled(cbx->isChecked());
	}
}

ExifItem::TagFlags TagNameEditDialog::getFlags() const
{
	ExifItem::TagFlags flags = 0;

	for(int i = 1, j = 0; i < ExifItem::Last; i *= 2, j++)
	{
		QCheckBox* cbx = static_cast<QCheckBox*>(ui.optionsBox->layout()->itemAt(j)->widget());
		if(cbx->isChecked())
			flags |= (ExifItem::TagFlag)i;
	}

	return flags;
}

void TagNameEditDialog::on_buttonBox_accepted()
{
	setResult(QDialog::Accepted);
	accept();
}

void TagNameEditDialog::on_buttonBox_rejected()
{
	setResult(QDialog::Rejected);
	reject();
}

void TagNameEditDialog::altTag_stateChanged(int state)
{
	if(altTagNames)
		altTagNames->setEnabled(state == Qt::Checked);

	if(state == Qt::Checked)
		asciiTagCbox->setChecked(false);
}

void TagNameEditDialog::ascii_stateChanged(int state)
{
	if(state == Qt::Checked)
		altTagCbox->setChecked(false);
}

void TagNameEditDialog::setFlagEnabled(ExifItem::TagFlag flag, bool isEnabled)
{
	for(int i = 1, j = 0; i < ExifItem::Last; i *= 2, j++)
	{
		if(i == flag)
		{
			QCheckBox* cbx = static_cast<QCheckBox*>(ui.optionsBox->layout()->itemAt(j)->widget());
			cbx->setEnabled(isEnabled);

			if((i == ExifItem::AsciiAlt) && !isEnabled && cbx->isChecked())
				altTagNames->setEnabled(false);

			break;
		}
	}
}