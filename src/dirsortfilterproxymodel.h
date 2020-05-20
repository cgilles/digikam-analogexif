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

#ifndef DIRSORTFILTERPROXYMODEL_H
#define DIRSORTFILTERPROXYMODEL_H

// Qt includes

#include <QSortFilterProxyModel>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QIcon>

class ExifTreeModel;

/**
 * sorts the files/directory view
 */
class DirSortFilterProxyModel : public QSortFilterProxyModel 
{
    Q_OBJECT

public:

    DirSortFilterProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {
    }

    bool lessThan(const QModelIndex &left, const QModelIndex &right)            const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
};

// -------------------------------------------------------------------------------------

class FileIconProvider : public QFileIconProvider
{
public:

    FileIconProvider(ExifTreeModel* const model);
    
    QIcon icon(const QFileInfo& info) const;
    
private:
    
    ExifTreeModel* m_model;
};

#endif // DIRSORTFILTERPROXYMODEL_H
