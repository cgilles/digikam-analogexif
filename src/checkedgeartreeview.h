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

#ifndef CHECKEDGEARTREEVIEW_H
#define CHECKEDGEARTREEVIEW_H

#include <QTreeView>

class CheckedGearTreeView : public QTreeView
{
    Q_OBJECT

public:
    CheckedGearTreeView(QWidget *parent) : QTreeView(parent) { }

public slots:
    void edit(const QModelIndex& index)
    {
        QTreeView::edit(index);
    }

protected:
    virtual bool edit(const QModelIndex &index, EditTrigger trigger, QEvent *event)
    {
        if(index.isValid() && (index.column() == 1) && (trigger == QAbstractItemView::EditKeyPressed))
            emit doubleClicked(index);

        return QTreeView::edit(index, trigger, event);
    }
};

#endif // CHECKEDGEARTREEVIEW_H
