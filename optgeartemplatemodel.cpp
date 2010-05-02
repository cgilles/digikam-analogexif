#include "optgeartemplatemodel.h"
#include "exifitem.h"

#include <QFont>
#include <QSqlQuery>
#include <QSqlError>
#include <QStringList>

void OptGearTemplateModel::reload(int id)
{
	gearId = id;
	setQuery(QString("SELECT a.OrderBy, b.TagName, b.TagText, b.TagType, b.PrintFormat, b.Flags, b.id, a.id FROM GearTemplate a, MetaTags b WHERE a.GearType = %1 AND b.id = a.TagId ORDER BY a.OrderBy").arg(id));
}

Qt::ItemFlags OptGearTemplateModel::flags(const QModelIndex &index) const
{
	if(!index.isValid())
		return 0;

#ifndef DEVELOPMENT_VERSION

	// protect minimum db setup
	if(!query().seek(index.row()))
		return 0;

	if(query().value(5).toInt() & ExifItem::TagFlagProtected)
	{
		if((index.column() != 0) && (index.column() != 2))
			return Qt::ItemIsSelectable;
	}

#endif // DEVELOPMENT_VERSION

	if(index.column() == 1)
		return Qt::ItemIsUserCheckable | Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant OptGearTemplateModel::data(const QModelIndex &index, int role) const
{
	if(!index.isValid())
		return QVariant();

	if(role == Qt::TextAlignmentRole)
	{
		switch(index.column())
		{
		case 0:
		case 3:
			return Qt::AlignHCenter;
			break;
		}
	}

	if(role == GetTagId)
	{
		if(!query().seek(index.row()))
			return QVariant();

		return query().value(6).toInt();
	}

	if(role == GetTagFlagsRole)
	{
		if(!query().seek(index.row()))
			return QVariant();

		return query().value(5).toInt();
	}

	if((index.column() == 1) && (role == Qt::CheckStateRole))
	{
		// me does like copy-paste!
		if(!query().seek(index.row()))
			return QVariant();

		if(query().value(5).toInt() & ExifItem::TagFlagExtra)
			return Qt::Checked;
		else
			return Qt::Unchecked;
	}

	QVariant v = QSqlQueryModel::data(index, role);

	if(index.column() == 3)
	{
		// special care for tag type
		if(role == Qt::DisplayRole)
		{
			if(v != QVariant())
				return ExifItem::typeName((ExifItem::TagTypes)v.toInt());
		}
		else if(role == Qt::FontRole)
		{
			QFont f;
			f.setBold(true);

			return f;
		}
	}

	return v;
}

// header information
QVariant OptGearTemplateModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(((role != Qt::DisplayRole) && (role != Qt::FontRole)) || (orientation != Qt::Horizontal))
		return QSqlQueryModel::headerData(section, orientation, role);

	if(role == Qt::FontRole)
	{
		QFont f;
		f.setBold(false);
		return f;
	}

	// describe fields
	switch(section)
	{
	case 0:
		// Tag name
		return tr("#");
		break;
	case 1:
		// Tag name
		return tr("(Extra) Exif tag(s)");
		break;
	case 2:
		// Tag text
		return tr("Description");
		break;
	case 3:
		// Tag type
		return tr("Type");
		break;
	case 4:
		// print format
		return tr("Display format");
		break;

#ifdef DEVELOPMENT_VERSION
	case 5:
		// print format
		return tr("Flags");
		break;
#endif // DEVELOPMENT_VERSION
	}

	return QVariant();
}

bool OptGearTemplateModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if ((role != Qt::EditRole) && (role != Qt::CheckStateRole))
		return false;

	if(!query().seek(index.row()))
		return false;

	int id = query().value(6).toInt();

	if(value == QVariant())
		return false;

	// update the record
	QString queryStr;

	if(index.column() == 0)
	{
		queryStr = QString("UPDATE GearTemplate SET OrderBy = %1 WHERE id = %2").arg(value.toInt()).arg(query().value(7).toInt());
	}
	else if((index.column() == 1) && (role == Qt::CheckStateRole))
	{
		int flags = query().value(5).toInt();

		if(value.toInt() == Qt::Checked)
			flags = flags | 0x02;
		else
			flags = flags & ~0x02;

		queryStr = QString("UPDATE MetaTags SET Flags = %1 WHERE id = %2").arg(flags).arg(id);
	}
	else
	{
		queryStr = "UPDATE MetaTags SET ";

		switch(index.column())
		{
		case 1:
			queryStr += QString("TagName = '%1'").arg(value.toString());
			break;
		case 2:
			queryStr += QString("TagText = '%1'").arg(value.toString());
			break;
		case 3:
			queryStr += QString("TagType = %1").arg(value.toInt());
			break;
		case 4:
			queryStr += QString("PrintFormat = '") + value.toString() + +"'";
			break;

#ifdef DEVELOPMENT_VERSION
		case 5:
			queryStr += QString("Flags = %1").arg(value.toInt());
			break;
#endif // DEVELOPMENT_VERSION
		default:
			return false;
			break;
		}

		queryStr += QString(" WHERE id = ") + QString::number(id);
	}

	QSqlQuery updQuery(queryStr);

	if(updQuery.lastError().isValid())
		return false;

	emit dataChanged(index, index);

	reload();

	return true;
}

// find out which gear uses this tag
QStringList OptGearTemplateModel::getTagUsage(const QModelIndex& idx)
{
	QStringList strList;

	int tagId = idx.data(GetTagId).toInt();

	QSqlQuery query(QString("SELECT a.GearName FROM UserGearItems a, UserGearProperties b WHERE b.TagId = %1 AND a.id = b.GearId ORDER BY a.OrderBy").arg(tagId));

	while(query.next())
		strList << query.value(0).toString();

	return strList;
}

// remove tag
void OptGearTemplateModel::removeTag(const QModelIndex& idx)
{
	int tagId = idx.data(GetTagId).toInt();

	// remove user properties
	QSqlQuery query(QString("DELETE FROM UserGearProperties WHERE TagId = %1").arg(tagId));

	// remove from template
	query.exec(QString("DELETE FROM GearTemplate WHERE TagId = %1").arg(tagId));

	// remove tag
	query.exec(QString("DELETE FROM MetaTags WHERE id = %1").arg(tagId));

	reload();
}

// add new tag
int OptGearTemplateModel::insertTag(QString tagName, QString tagDesc, QString tagFormat, int tagType)
{
	QSqlQuery query(QString("INSERT INTO MetaTags(TagName, TagText, PrintFormat, TagType) VALUES('%1', '%2', '").arg(tagName).arg(tagDesc) + tagFormat + QString("', %1)").arg(tagType));

	// check validity
	if(!query.lastInsertId().isValid())
		return -1;

	int newId = query.lastInsertId().toInt();

	query.exec(QString("INSERT INTO GearTemplate(GearType, TagId, OrderBy) VALUES(%1, %2, %3)").arg(gearId).arg(newId).arg(rowCount()));

	// check validity
	if(!query.lastInsertId().isValid())
		return -1;

	reload();

	return query.lastInsertId().toInt();
}