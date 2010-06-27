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

#ifndef TAGNAMEEDITDIALOG_H
#define TAGNAMEEDITDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include "ui_tagnameeditdialog.h"
#include "exifitem.h"

class TagNameEditDialog : public QDialog
{
	Q_OBJECT

public:
	TagNameEditDialog(QWidget *parent = 0, const QString& tagNames = "", ExifItem::TagFlags flags = ExifItem::None, const QString& altTags = "");
	
	ExifItem::TagFlags getFlags() const;
	QString getTagNames() const
	{
		return ui.tagNamesEdit->text();
	}

	QString getAltTagNames() const
	{
		if(altTagNames && altTagNames->isEnabled())
		{
			return altTagNames->text();
		}

		return QString();
	}

	void setFlags(ExifItem::TagFlags flags);
	void setTagNames(const QString& tagNames)
	{
		ui.tagNamesEdit->setText(tagNames);
	}

	void setAltTagNames(const QString& altTags)
	{
		if(altTagNames)
		{
			altTagNames->setText(altTags);
		}
	}

private:
	Ui::TagNameEditDialogClass ui;

	QLineEdit* altTagNames;

private slots:
	// ok/cancel
	void on_buttonBox_accepted();
	void on_buttonBox_rejected();

	// alt tag enabled/disabled
	void altTag_stateChanged(int);
};

#endif // TAGNAMEEDITDIALOG_H
