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

#ifndef EXIFITEMDELEGATE_H
#define EXIFITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QLineEdit>
#include <QToolTip>
#include <QKeyEvent>
#include <QApplication>

class GpsLineEdit: public QLineEdit
{
	Q_OBJECT

public:
	GpsLineEdit(QWidget* parent = 0);
	GpsLineEdit(const QString& contents, QWidget* parent = 0);

	QString getDefaultValue() const
	{
		return defaultValue;
	}

public Q_SLOTS:
	void paste();

private:
	QString defaultValue;
};

class AsciiLineEdit: public QLineEdit
{
	Q_OBJECT

public:
	AsciiLineEdit(QWidget* parent = 0) : QLineEdit(parent) { }
	AsciiLineEdit(const QString& contents, QWidget* parent = 0) : QLineEdit(contents, parent) { }

protected:
	virtual void keyPressEvent(QKeyEvent* event)
	{
		QString keys = event->text();
		if(!keys.isEmpty())
		{
			QToolTip::hideText();
			for(int i = 0; i < keys.length(); i++)
			{
				char curChar = keys.at(i).toAscii();
				if((curChar >= 0x80) || (curChar ==0))
				{
					// not supported character
#ifdef Q_WS_WIN
					QToolTip::showText(mapToGlobal(cursorRect().topRight()), tr("Only 7-bit ASCII characters are allowed in this field.\nLocal characters may not be displayed properly in the other applications."));
#else
					QToolTip::showText(mapToGlobal(cursorRect().topRight()), tr("Only 7-bit ASCII characters are allowed in the field."));
					QApplication::beep();
					event->ignore();
					return;
#endif
				}
			}
		}

		QLineEdit::keyPressEvent(event);
	}
};

class ExifItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	ExifItemDelegate(QObject *parent) : QStyledItemDelegate(parent) { }

	virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
		const QModelIndex &index) const;

	virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;
	virtual void setModelData(QWidget *editor, QAbstractItemModel *model,
		const QModelIndex &index) const;

	virtual void updateEditorGeometry(QWidget *editor,
		const QStyleOptionViewItem &option, const QModelIndex &index) const;

	virtual bool eventFilter(QObject *object, QEvent *event);
};

#endif // EXIFITEMDELEGATE_H
