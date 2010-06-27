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

#include "geartreemodel.h"
#include "exifitem.h"

#include <QSqlQuery>
#include <QStandardItem>
#include <QFont>

void GearTreeModel::reload()
{
	clear();

	if(bodyCount() == 0)
	{
		QFont f;
		f.setStyle(QFont::StyleItalic);

		QStandardItem* item = new QStandardItem(tr("No equipment defined"));
		item->setFont(f);

		insertRow(0, item);
	}
	else
	{
		QSqlQuery query("SELECT a.GearName, a.id, b.GearName, b.id FROM UserGearItems a, UserGearItems b WHERE a.GearType = 0 AND b.ParentId = a.id ORDER BY a.OrderBy, b.OrderBy");
		int previousParentId = -1;
		QStandardItem* previousParent = NULL;
		QStandardItem* child;

		while(query.next())
		{
			if(query.value(1).toInt() != previousParentId)
			{
				previousParentId = query.value(1).toInt();
				previousParent = new QStandardItem(query.value(0).toString());
				previousParent->setData(previousParentId);

				invisibleRootItem()->appendRow(previousParent);
			}

			child = new QStandardItem(query.value(2).toString());
			child->setData(query.value(3).toInt());

			previousParent->appendRow(child);
		}
	}
}

// get number of camera bodies (gearType = 0)
int GearTreeModel::bodyCount() const
{
	QSqlQuery query("SELECT COUNT(*) FROM UserGearItems WHERE GearType=0");
	query.first();

	if(query.isValid())
		return query.value(0).toInt();
	else
		return 0;
}

QVariant GearTreeModel::data(const QModelIndex &item, int role) const
{
	if(((item == selected) || (item == selected.parent())) && (role == Qt::FontRole))
	{
		QFont f;
		f.setBold(true);

		return f;
	}

	if(role == GetExifData)
	{
		if(!item.isValid() || !isApplicable)
			return QVariant();

		QVariantList properties;

		QStandardItem* selItem = itemFromIndex(item.parent());

		// get item parent properties
		if(selItem)
		{
			QSqlQuery query(QString("SELECT b.TagName, a.TagValue, b.Flags, a.AltValue FROM UserGearProperties a, MetaTags b WHERE a.GearId = %1 AND b.id = a.TagId").arg(selItem->data().toInt()));

		    while (query.next()) {
				QVariant value = query.value(1);

				if(((ExifItem::TagFlags)query.value(2).toInt()).testFlag(ExifItem::Ascii))
				{
					QVariantList varList;
					varList << value << query.value(3);

					value = varList;
				}

				properties << query.value(0) << value;
			}
		}

		selItem = itemFromIndex(item);

		// get item properties
		if(selItem)
		{
			QSqlQuery query(QString("SELECT b.TagName, a.TagValue, b.Flags, a.AltValue FROM UserGearProperties a, MetaTags b WHERE a.GearId = %1 AND b.id = a.TagId").arg(selItem->data().toInt()));

		    while (query.next()) {
				QVariant value = query.value(1);

				if(((ExifItem::TagFlags)query.value(2).toInt()).testFlag(ExifItem::Ascii))
				{
					QVariantList varList;
					varList << value << query.value(3);

					value = varList;
				}

				properties << query.value(0) << value;
			}
		}

		if(properties.count())
			return properties;

		return QVariant();
	}

	return QStandardItemModel::data(item, role);
}

Qt::ItemFlags GearTreeModel::flags(const QModelIndex &index) const
{
	if((bodyCount() == 0) || !isApplicable)
	{
		return 0;
	}

	return QStandardItemModel::flags(index);
}
