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

#include "dirsortfilterproxymodel.h"

// Qt includes

#include <QFileSystemModel>
#include <QPixmap>

// Local includes

#include "exiftreemodel.h"

bool DirSortFilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    QFileSystemModel* const srcModel = dynamic_cast<QFileSystemModel*>(sourceModel());

    if (!left.isValid() || !right.isValid())
    {
        return false;
    }

    // if compared to directory file is always less than
    if (srcModel->isDir(left) && !srcModel->isDir(right))
    {
        return true;
    }

    if (!srcModel->isDir(left) && srcModel->isDir(right))
    {
        return false;
    }

    // else compare by name
    return (QString::compare(srcModel->fileName(left), srcModel->fileName(right), Qt::CaseInsensitive) < 0);
}

QVariant DirSortFilterProxyModel::data(const QModelIndex& index, int role) const
{
    QFileSystemModel* const srcModel = dynamic_cast<QFileSystemModel*>(sourceModel());

    if (!index.isValid())
    {
        return QVariant();
    }

    if (role == Qt::ToolTipRole)
    {
        return srcModel->fileName(index);
    }

    return QSortFilterProxyModel::data(index, role);
}

// -----------------------------------------------------------------------------------------------------------

FileIconProvider::FileIconProvider(ExifTreeModel* const model)
{
    m_model = model;
}
    
QIcon FileIconProvider::icon(const QFileInfo& info) const
{
    if (!QFileInfo(info.filePath()).isDir())
    {
        // if the file is image

        QImage preview = m_model->getPreview(info.filePath());
        QIcon myIcon(QPixmap::fromImage(preview));

        return myIcon;
    }

    // if the file is not image, return default icon

    return QFileIconProvider::icon(info);
}
