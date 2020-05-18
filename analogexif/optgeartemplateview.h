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

#ifndef OPTGEARTEMPLATEVIEW_H
#define OPTGEARTEMPLATEVIEW_H

#include <QTableView>
#include <QMouseEvent>

class OptGearTemplateView : public QTableView
{
	Q_OBJECT

public:
	OptGearTemplateView(QWidget *parent): QTableView(parent) { }

public slots:
	virtual void edit(const QModelIndex& index)
	{
		if(index.isValid() && (index.column() == 1))
		{
			curEditIndex = index;
		}

		QTableView::edit(index);
	}

	void onCloseEditor(QWidget*, QAbstractItemDelegate::EndEditHint)
	{
		curEditIndex = QModelIndex();
	}

protected:
	QModelIndex curEditIndex;

	virtual bool edit(const QModelIndex& index, EditTrigger trigger, QEvent* event)
	{
		if(index.isValid() && (index.column() == 1))
		{
			curEditIndex = index;
		}

		return QTableView::edit(index, trigger, event);
	}

	virtual void mousePressEvent(QMouseEvent * event)
	{
		QModelIndex clickIndex = indexAt(event->pos());

		QTableView::mousePressEvent(event);		

		if((curEditIndex != QModelIndex()) && (clickIndex != curEditIndex))
		{
			QWidget* widgeditor = indexWidget(curEditIndex);
			if(widgeditor)
			{
				commitData(widgeditor);
				closeEditor(widgeditor, QAbstractItemDelegate::NoHint);
			}
			curEditIndex = QModelIndex();
		}
	}
	
};

#endif // OPTGEARTEMPLATEVIEW_H
