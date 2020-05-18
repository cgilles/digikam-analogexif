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

#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>
#include "ui_progressdialog.h"

class ProgressDialog : public QDialog
{
	Q_OBJECT

public:
	ProgressDialog(QString windowTitle, QString labelText, QString buttonText = tr("Cancel"), QWidget *parent = 0, int min = 0, int max = 100);

	void setLabelText(const QString& text)
	{
		ui.label->setText(text);
	}

	bool wasCanceled() const
	{
		return cancelled;
	}

	void resetCanceled()
	{
		cancelled = false;
	}

	void setRange(int min, int max)
	{
		ui.progressBar->setRange(min, max);
	}

public slots:
	void setValue(int i)
	{
		ui.progressBar->setValue(i);
	}

	void setValue(qint64 i)
	{
		ui.progressBar->setValue(i / 1024);
	}

	virtual void reject()
	{
		// handle Escape key properly
		cancelled = true;
		QDialog::reject();
	}

private:
	Ui::ProgressDialogClass ui;

	bool cancelled;

private slots:
	// on cancel button clicked
	void on_cancelBtn_clicked(bool)
	{
		cancelled = true;
	}
};

#endif // PROGRESSDIALOG_H
