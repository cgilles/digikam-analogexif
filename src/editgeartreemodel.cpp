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

#include "editgeartreemodel.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardItem>
#include <QMimeData>

void EditGearTreeModel::reload()
{
	reload(gearType);
}

QModelIndex EditGearTreeModel::reload(int id)
{
	QStandardItem* resultIndex = NULL;
	clear();

	invisibleRootItem()->setFlags(Qt::ItemIsDropEnabled);
	invisibleRootItem()->setData(-1, GetGearIdRole);
	invisibleRootItem()->setData(-1, GetGearTypeRole);


	QSqlQuery query(QString("SELECT GearName, id, OrderBy FROM UserGearItems WHERE GearType = %1 ORDER BY OrderBy").arg(gearType)), subquery;

	int querySize = 0;

	QStandardItem* parent, *child;

	while(query.next())
	{
		querySize++;

		parent = new QStandardItem(query.value(0).toString()/* + " (" + query.value(2).toString() + ")"*/);
		
		if((id != -1) && (query.value(1).toInt() == id))
			resultIndex = parent;

		parent->setData(query.value(1).toInt(), GetGearIdRole);
		parent->setData(gearType, GetGearTypeRole);
		parent->setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsEditable);
		if(treeView)
			parent->setFlags(parent->flags() | Qt::ItemIsDropEnabled);

		invisibleRootItem()->appendRow(parent);

		subquery.exec(QString("SELECT GearName, id, GearType, OrderBy FROM UserGearItems WHERE ParentId = %1 ORDER BY OrderBy").arg(query.value(1).toInt()));

		while(subquery.next())
		{
			querySize++;

			child = new QStandardItem(subquery.value(0).toString()/* + " (" + subquery.value(3).toString() + ")"*/);

			if((id != -1) && (subquery.value(1).toInt() == id))
				resultIndex = child;

			child->setData(subquery.value(1).toInt(), GetGearIdRole);
			child->setData(subquery.value(2).toInt(), GetGearTypeRole);
			child->setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsEditable);

			parent->appendRow(child);
		}
	}

	if(querySize == 0)
	{
		resultIndex = new QStandardItem(emptyMessage);
		resultIndex->setData(-1, GetGearIdRole);
		resultIndex->setData(-1, GetGearTypeRole);

		QFont f;
		f.setStyle(QFont::StyleItalic);

		resultIndex->setData(f, Qt::FontRole);

		resultIndex->setFlags(0);

		invisibleRootItem()->appendRow(resultIndex);
	}

	if(resultIndex)
		return indexFromItem(resultIndex);
	else
		return QModelIndex();
}

QStringList EditGearTreeModel::mimeTypes() const
{
	QStringList types;
	types << "application/analogexif.gearlist";
	return types;
}

QMimeData* EditGearTreeModel::mimeData(const QModelIndexList &indexes) const
{
	QMimeData *mimeData = new QMimeData();
	QByteArray encodedData;

	QDataStream stream(&encodedData, QIODevice::WriteOnly);

	foreach (QModelIndex index, indexes) {
		if (index.isValid()) {
			int parentId = -1;
			if(index.parent().isValid())
				parentId = index.parent().data(GetGearIdRole).toInt();
			stream << index.data(GetGearIdRole).toInt() << index.data(GetGearTypeRole).toInt();
		}
	}

	mimeData->setData("application/analogexif.gearlist", encodedData);
	return mimeData;
}

// TODO: fix it
// NB: I know this is really terrible way to implement but Qt drag-n-drop was driving me crazy
// and db itself is not that big, so this'll do the trick
bool EditGearTreeModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	if (action == Qt::IgnoreAction)
		return true;

	if (!data->hasFormat("application/analogexif.gearlist"))
		return false;

	if (column > 0)
		return false;

	int parentGearType = -1, parentId = -1;

	if(parent.isValid())
	{
		parentGearType = parent.data(GetGearTypeRole).toInt();
		parentId = parent.data(GetGearIdRole).toInt();
	}

	QByteArray encodedData = data->data("application/analogexif.gearlist");
	QDataStream stream(&encodedData, QIODevice::ReadOnly);

	QSqlQuery query;

	int beginRow = row, startRow = row;

	if(row == -1)
	{
		startRow = beginRow = rowCount(parent);
	}

	int nItems = 0;

	QList<int> gearIds;

	while (!stream.atEnd()) {
		QString itemName;
		int gearId, gearType;

		stream >> gearId >> gearType;

		if(action == Qt::CopyAction)
		{
			// create new item
			gearId = createNewGear(gearId, parentId, gearType, "Copy of ");

			if(gearId == -1)
				continue;
		}

		if((gearType == 1) && (parentGearType == 0))
		{
			// assign new order by and possibly parent
			query.exec(QString("UPDATE UserGearItems SET ParentId = %1, OrderBy = %2 WHERE id = %3").arg(parent.data(GetGearIdRole).toInt()).arg(beginRow).arg(gearId));
		}
		else if(((gearType == 0) || (gearType == 2)) && (parentGearType == -1))
		{
			// update orderby
			query.exec(QString("UPDATE UserGearItems SET OrderBy = %1 WHERE id = %2").arg(beginRow).arg(gearId));
		}
		else
		{
			// ignore incorrect drops
			continue;
		}

		gearIds << gearId;

		beginRow++;
		nItems++;
	}

	// update neighbours
	QStandardItem* parentItem = invisibleRootItem();

	if(parent.isValid())
		parentItem = itemFromIndex(parent);

	int newOrderBy = 0;
	for(int i = 0; i < rowCount(parent); i++)
	{
		QStandardItem* sibling = parentItem->child(i, column);
		if(sibling)
		{
			int siblingId = sibling->data(GetGearIdRole).toInt();

			// not in list
			if(gearIds.indexOf(siblingId) == -1)
			{
				if(newOrderBy == startRow)
					newOrderBy += nItems;
				query.exec(QString("UPDATE UserGearItems SET OrderBy = %1 WHERE id = %2").arg(newOrderBy).arg(siblingId));
				newOrderBy++;
			}
		}
	}

	reload();

	emit layoutChanged();

	return true;
}

int EditGearTreeModel::createNewGear(int copyId, int parentId, int gearType, QString prefix, int orderBy)
{
	QSqlQuery query, subquery;

	// start inner transaction
	query.exec("SAVEPOINT InsertGear");

	if(copyId == -1)
	{
		// insert new row
		query.exec(QString("INSERT INTO UserGearItems(ParentId, GearType, GearName, OrderBy) VALUES(%1, %2, '%3', %4)").arg(parentId).arg(gearType).arg(prefix).arg(orderBy));
	}
	else
	{
		// copy from existing one
		if(orderBy == -1)
			query.exec(QString("INSERT INTO UserGearItems(ParentId, GearType, GearName, OrderBy) SELECT %1, GearType, '%2' || GearName, OrderBy FROM UserGearItems WHERE id = %3").arg(parentId).arg(prefix).arg(copyId));
		else
			query.exec(QString("INSERT INTO UserGearItems(ParentId, GearType, GearName, OrderBy) SELECT %1, GearType, '%2' || GearName, %3 FROM UserGearItems WHERE id = %4").arg(parentId).arg(prefix).arg(orderBy).arg(copyId));
	}

	// check validity
	if(!query.lastInsertId().isValid())
	{
		query.exec("ROLLBACK TO InsertGear");
		return -1;
	}

	if(query.lastError().isValid())
	{
		query.exec("ROLLBACK TO InsertGear");
		return -1;
	}

	int newId = query.lastInsertId().toInt();

	// set properties
	if(copyId == -1)
	{
		// insert properties from template
		query.exec(QString("INSERT INTO UserGearProperties(GearId, TagId, OrderBy) SELECT %1, TagId, OrderBy FROM GearTemplate WHERE GearType = %2").arg(newId).arg(gearType));

		if(query.lastError().isValid())
		{
			query.exec("ROLLBACK TO InsertGear");
			return -1;
		}
	}
	else
	{
		// copy properties
		query.exec(QString("INSERT INTO UserGearProperties(GearId, TagId, TagValue, OrderBy) SELECT %1, TagId, TagValue, OrderBy FROM UserGearProperties WHERE GearId = %2").arg(newId).arg(copyId));

		if(query.lastError().isValid())
		{
			query.exec("ROLLBACK TO InsertGear");
			return -1;
		}

		// copy children
		if(gearType == 0)
		{
			subquery.exec(QString("SELECT id, GearType FROM UserGearItems WHERE ParentId = %1 ORDER BY OrderBy").arg(copyId));
			while(subquery.next())
			{
				if(createNewGear(subquery.value(0).toInt(), newId, subquery.value(1).toInt(), "", -1) == -1)
				{
					query.exec("ROLLBACK TO InsertGear");
					return -1;
				}
			}
		}
	}

	// "commit" inner transaction
	query.exec("RELEASE InsertGear");

	return newId;
}

bool EditGearTreeModel::deleteGear(int gearId, int gearType)
{
	QSqlQuery query;

	// start inner transaction
	query.exec("SAVEPOINT DeleteGear");

	// delete children
	if(gearType == 0)
	{
		query.exec(QString("SELECT id, GearType FROM UserGearItems WHERE ParentId = %1 ORDER BY OrderBy").arg(gearId));

		while(query.next())
		{
			deleteGear(query.value(0).toInt(), query.value(1).toInt());
		}
	}

	// delete properties
	query.exec(QString("DELETE FROM UserGearProperties WHERE GearId = %1").arg(gearId));

	if(query.lastError().isValid())
	{
		query.exec("ROLLBACK TO DeleteGear");
		return false;
	}

	// delete gear
	query.exec(QString("DELETE FROM UserGearItems WHERE id = %1").arg(gearId));

	if(query.lastError().isValid())
	{
		query.exec("ROLLBACK TO DeleteGear");
		return false;
	}

	// "commit" transaction
	query.exec("RELEASE DeleteGear");

	return true;
}

bool EditGearTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if(role == Qt::EditRole)
	{
		if(index.isValid() && (value != QVariant()))
		{
			int gearId = index.data(GetGearIdRole).toInt();

			if(index.data(Qt::EditRole) == value)
				return false;

			QSqlQuery query(QString("UPDATE UserGearItems SET GearName = '%1' WHERE id = %2").arg(value.toString()).arg(gearId));

			if(query.lastError().isValid())
				return false;

			reload();

			emit layoutChanged();

			return true;
		}
		else
		{
			return false;
		}
	}
	return QStandardItemModel::setData(index, value, role);
}
