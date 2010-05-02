#include "gearlistmodel.h"

#include <QFont>
#include <QSqlQuery>
#include <QSqlRecord>

void GearListModel::reload()
{
	setQuery(QString("SELECT GearName, id FROM UserGearItems WHERE GearType=%1 ORDER BY OrderBy").arg(gearType));
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

	if((item == selected) && (role == Qt::FontRole))
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
			QSqlQuery query(QString("SELECT b.TagName, a.TagValue FROM UserGearProperties a, MetaTags b WHERE a.GearId = %1 AND b.id = a.TagId").arg(curRecord.value(1).toInt()));
			QVariantList properties;

		    while (query.next()) {
				properties << query.value(0) << query.value(1);
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