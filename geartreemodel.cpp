#include "geartreemodel.h"

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
			QSqlQuery query(QString("SELECT b.TagName, a.TagValue FROM UserGearProperties a, MetaTags b WHERE a.GearId = %1 AND b.id = a.TagId").arg(selItem->data().toInt()));

		    while (query.next()) {
				properties << query.value(0) << query.value(1);
			}
		}

		selItem = itemFromIndex(item);

		// get item properties
		if(selItem)
		{
			QSqlQuery query(QString("SELECT b.TagName, a.TagValue FROM UserGearProperties a, MetaTags b WHERE a.GearId = %1 AND b.id = a.TagId").arg(selItem->data().toInt()));

		    while (query.next()) {
				properties << query.value(0) << query.value(1);
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
