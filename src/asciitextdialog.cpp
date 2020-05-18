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

#include "asciitextdialog.h"
#include <QMessageBox>

AsciiTextDialog::AsciiTextDialog(const QString& uValue, const QString& aValue, QWidget *parent)
	: QDialog(parent), unicodeValue(uValue)
{
	ui.setupUi(this);

	ui.buttonBox->addButton(tr("Set Ascii value"), QDialogButtonBox::AcceptRole);
	ui.buttonBox->addButton(tr("Ignore Ascii value"), QDialogButtonBox::DestructiveRole);

	ui.unicodeEdit->setPlainText(uValue);

	if(aValue.isEmpty())
	{
		// for some reason Local8Bit on Mac still Unicode
#ifdef Q_WS_MAC
		ui.asciiEdit->setPlainText(QString::fromLatin1(uValue.toLatin1().constData()));
#else
		ui.asciiEdit->setPlainText(QString::fromLocal8Bit(uValue.toLocal8Bit().constData()));
#endif
	}
	else
	{
		ui.asciiEdit->setPlainText(aValue);
	}

	setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
}

void AsciiTextDialog::on_copyBtn_clicked()
{
	// for some reason Local8Bit on Mac still Unicode
#ifdef Q_WS_MAC
	ui.asciiEdit->setPlainText(QString::fromLatin1(ui.unicodeEdit->toPlainText().toLatin1().constData()));
#else
	ui.asciiEdit->setPlainText(QString::fromLocal8Bit(ui.unicodeEdit->toPlainText().toLocal8Bit().constData()));
#endif
}

void AsciiTextDialog::on_buttonBox_clicked(QAbstractButton* button)
{
	int buttonRole = ui.buttonBox->buttonRole(button);

	if((buttonRole == QDialogButtonBox::DestructiveRole)
		|| ((buttonRole == QDialogButtonBox::AcceptRole) && (ui.asciiEdit->toPlainText() == "")))
	{
		QMessageBox::warning(this, tr("No Ascii alternative entered"),
			tr("No Ascii alternative text was entered for this field.\n"
			   "Ascii-only metadata tags would be empty."));
	}

	setResult(buttonRole);
	done(buttonRole);
}

void AsciiTextDialog::reject()
{
	setResult(QDialogButtonBox::RejectRole);
	done(QDialogButtonBox::RejectRole);
}
