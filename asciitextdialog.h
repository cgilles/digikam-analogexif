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

#ifndef ASCIITEXTDIALOG_H
#define ASCIITEXTDIALOG_H

#include <QDialog>

#ifdef Q_WS_MAC
#include "ui_asciitextdialog_mac.h"
#else
#include "ui_asciitextdialog.h"
#endif

class AsciiTextDialog : public QDialog
{
	Q_OBJECT

public:
	AsciiTextDialog(const QString& uValue, const QString& aValue, QWidget *parent = 0);

	void setUnicodeValue(const QString& uValue)
	{
		unicodeValue = uValue;
		ui.unicodeEdit->setPlainText(unicodeValue);
	}

	QString getUnicodeValue() const
	{
		return ui.unicodeEdit->toPlainText();
	}

	void setAsciiValue(const QString& aValue)
	{
		ui.asciiEdit->setPlainText(aValue);
	}

	QString getAsciiValue() const
	{
		return ui.asciiEdit->toPlainText();
	}

private:
	QString unicodeValue;
	Ui::AsciiTextDialogClass ui;

private slots:
	void on_copyBtn_clicked();
	void on_buttonBox_clicked(QAbstractButton* button);
};

#endif // ASCIITEXTDIALOG_H
