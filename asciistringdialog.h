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

#ifndef ASCIISTRINGDIALOG_H
#define ASCIISTRINGDIALOG_H

#include <QDialog>

#ifdef Q_WS_MAC
#include "ui_asciistringdialog_mac.h"
#else
#include "ui_asciistringdialog.h"
#endif

class AsciiStringDialog : public QDialog
{
	Q_OBJECT

public:
	AsciiStringDialog(const QString& uValue, const QString& aValue, QWidget *parent = 0);

	void setUnicodeValue(const QString& uValue)
	{
		unicodeValue = uValue;
		ui.unicodeEdit->setText(unicodeValue);
	}

	QString getUnicodeValue() const
	{
		return ui.unicodeEdit->text();
	}

	void setAsciiValue(const QString& aValue)
	{
		ui.asciiEdit->setText(aValue);
	}

	QString getAsciiValue() const
	{
		return ui.asciiEdit->text();
	}

private:
	QString unicodeValue;
	Ui::AsciiStringDialogClass ui;

private slots:
	void on_copyBtn_clicked();
	void on_buttonBox_clicked(QAbstractButton* button);
};

#endif // ASCIISTRINGDIALOG_H
