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
#include "exiftreemodel.h"
#include <QCheckBox>
#include <QPushButton>
#include <QMessageBox>
#include <QRegExp>
#include <QRegExpValidator>

TagNameEditDialog::TagNameEditDialog(QWidget *parent, const QString& tagNames, ExifItem::TagFlags, const QString& altTags)
	: QDialog(parent), altTagNames(NULL)
{
	ui.setupUi(this);

	ui.tagNamesEdit->setText(tagNames);
	ui.tagNamesEdit->setValidator(new QRegExpValidator(QRegExp("(((Exif|Iptc|Xmp)\\.([a-zA-Z0-9])+\\.([a-zA-Z0-9])+)\\s*(\\,?\\s*)?)+"), this));

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
			altTagNames->setValidator(new QRegExpValidator(QRegExp("(((Exif|Iptc|Xmp)\\.([a-zA-Z0-9])+\\.([a-zA-Z0-9])+)\\s*(\\,?\\s*)?)+"), this));

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

	setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);

	ui.tagNamesEdit->setFocus(Qt::OtherFocusReason);
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
	// check for empty string
	if(ui.tagNamesEdit->text().isEmpty())
	{
		QMessageBox::critical(this, tr("Empty tag list"), tr("Please specify correct tag in the tag list."));
		return;
	}

	// browse through tags and verify them
	QStringList tags = ui.tagNamesEdit->text().remove(QRegExp("(\\s?)")).split(",", QString::SkipEmptyParts);
	foreach(QString tag, tags)
	{
		if(!ExifTreeModel::tagSupported(tag))
		{
			QMessageBox::critical(this, tr("Unsupported tag"), tr("Unsupported or unknown metadata tag %1.\nPlease correct the tag name.").arg(tag));
			ui.tagNamesEdit->selectAll();
			ui.tagNamesEdit->setFocus(Qt::OtherFocusReason);

			return;
		}
	}

	// if alternative tag set, verify them as well
	if(altTagNames && altTagNames->isEnabled())
	{
		tags = altTagNames->text().remove(QRegExp("(\\s?)")).split(",", QString::SkipEmptyParts);
		foreach(QString tag, tags)
		{
			if(!ExifTreeModel::tagSupported(tag))
			{
				QMessageBox::critical(this, tr("Unsupported tag"), tr("Unsupported or unknown metadata tag %1.\nPlease correct the tag name.").arg(tag));
				altTagNames->selectAll();
				altTagNames->setFocus(Qt::OtherFocusReason);

				return;
			}
		}
	}

	setResult(QDialog::Accepted);
	accept();
}

void TagNameEditDialog::on_buttonBox_rejected()
{
	reject();
}

void TagNameEditDialog::reject()
{
	setResult(QDialog::Rejected);
	QDialog::reject();
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

void TagNameEditDialog::disableEdit()
{
	// disable all editing functions
	ui.tagNamesEdit->setEnabled(false);
	ui.optionsBox->setEnabled(false);
	ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}