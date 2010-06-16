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

#include "progressdialog.h"

ProgressDialog::ProgressDialog(QString windowTitle, QString labelText, QString buttonText, QWidget *parent, int min, int max)
	: QDialog(parent), cancelled(false)
{
	ui.setupUi(this);

	setWindowTitle(windowTitle);
	ui.label->setText(labelText);
	if(buttonText == "")
		ui.cancelBtn->hide();
	else
		ui.cancelBtn->setText(buttonText);

	ui.progressBar->setRange(min, max);

	setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
}
