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

#ifndef GEARTREEVIEW_H
#define GEARTREEVIEW_H

#include <QTreeView>
#include <QMouseEvent>
#include <QFocusEvent>

class GearTreeView : public QTreeView
{
    Q_OBJECT

public:
    GearTreeView(QWidget *parent) : QTreeView(parent) { }

Q_SIGNALS:
    void focused();

protected:
    virtual void mousePressEvent(QMouseEvent * event)
    {
        QModelIndex clickIndex = indexAt(event->pos());

        QTreeView::mousePressEvent(event);      

        if(!clickIndex.isValid() && (event->button() != Qt::RightButton))
            clearSelection();
    }

    virtual void focusInEvent(QFocusEvent * event)
    {
        QTreeView::focusInEvent(event);

        emit focused();
    }

    virtual bool focusNextPrevChild(bool next)
    {
        // TODO/BUG: need to prohibit selection of the next child when model is clear()ed.
        if(model()->rowCount() == 0)
            return false;
        else
            return QTreeView::focusNextPrevChild(next);
    }
};

#endif // GEARTREEVIEW_H
