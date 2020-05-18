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

#include "gearlistmodel.h"
#include "exifitem.h"

#include <QFont>
#include <QSqlQuery>
#include <QSqlRecord>

void GearListModel::reload()
{
	// invalidate selected index
	selected = QModelIndex();
	setQuery(QString("SELECT GearName, id FROM UserGearItems WHERE GearType=%1 ORDER BY OrderBy").arg(gearType));
}

void GearListModel::setApplicable(bool applicable)
{
    isApplicable = applicable;
    beginResetModel();
    endResetModel();
}

int GearListModel::rowCount(const QModelIndex &index) const
{
	int i = QSqlQueryModel::rowCount(index.parent());

	if(i == 0)
		return 1;
	else
		return i;
}

QVariant GearListModel::data(const QModelIndex &item, int role) const
{
	if(!item.isValid())
		return QVariant();

	int nRows = QSqlQueryModel::rowCount(item.parent());

	if(nRows == 0)
	{
		// return string for the empty list
		if(role == Qt::DisplayRole)
			return emptyMessage;
		if(role == Qt::FontRole)
		{
			QFont f;
			f.setStyle(QFont::StyleItalic);

			return f;
		}
	}

	if(selected.isValid() && (item == selected) && (role == Qt::FontRole))
	{
		QFont f;
		f.setBold(true);

		return f;
	}

	if(role == GetExifData)
	{
		if(!item.isValid() || !isApplicable)
			return QVariant();

		QSqlRecord curRecord = record(item.row());
		if(!curRecord.isEmpty())
		{
			QSqlQuery query(QString("SELECT b.TagName, a.TagValue, b.Flags, a.AltValue FROM UserGearProperties a, MetaTags b WHERE a.GearId = %1 AND b.id = a.TagId").arg(curRecord.value(1).toInt()));
			QVariantList properties;

		    while (query.next()) {
				QVariant value = query.value(1);

				if(((ExifItem::TagFlags)query.value(2).toInt()).testFlag(ExifItem::AsciiAlt))
				{
					QVariantList varList;
					varList << value << query.value(3);

					value = varList;
				}

				properties << query.value(0) << value;
			}

			if(properties.count())
				return properties;
		}

		return QVariant();
	}

	return QSqlQueryModel::data(item, role);
}

Qt::ItemFlags GearListModel::flags(const QModelIndex &index) const
{
	int nRows = QSqlQueryModel::rowCount(index.parent());

	if((nRows == 0) || !isApplicable)
	{
		return 0;
	}

	return QSqlQueryModel::flags(index);
}
