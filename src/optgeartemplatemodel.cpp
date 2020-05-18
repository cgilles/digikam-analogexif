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

#include "optgeartemplatemodel.h"
#include "exiftreemodel.h"
#include "exifitem.h"

#include <QFont>
#include <QSqlQuery>
#include <QSqlError>
#include <QStringList>

void OptGearTemplateModel::reload(int id)
{
	gearId = id;
	setQuery(QString("SELECT a.OrderBy, b.TagName, b.TagText, b.TagType, b.PrintFormat, b.Flags, b.id, a.id, b.AltTag FROM GearTemplate a, MetaTags b WHERE a.GearType = %1 AND b.id = a.TagId ORDER BY a.OrderBy").arg(id));
}

Qt::ItemFlags OptGearTemplateModel::flags(const QModelIndex &index) const
{
	if(!index.isValid())
		return 0;

	// protect minimum db setup

	if(!query().seek(index.row()))
		return 0;

	if(ProtectBuiltInTags && ((ExifItem::TagFlags)query().value(5).toInt()).testFlag(ExifItem::Protected))
	{
		if((index.column() != 1) && (index.column() != 2) && (index.column() != 4))
			return Qt::ItemIsSelectable;

		// protect selection values from edit as well
		if(((ExifItem::TagFlags)query().value(5).toInt()).testFlag(ExifItem::Choice) && (index.column() == 4))
			return Qt::ItemIsSelectable;
	}

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

	if(role == GetAltTagRole)
	{
		if(!query().seek(index.row()))
			return QVariant();

		return query().value(8).toString();
	}

	if(role == GetTagTypeRole)
	{
		if(!query().seek(index.row()))
			return QVariant();

		return query().value(3).toInt();
	}

	QVariant v = QSqlQueryModel::data(index, role);

	if(((role == Qt::ToolTipRole) || (role == Qt::DisplayRole)) && (index.column() == 1))
	{
		if(!query().seek(index.row()))
			return QVariant();

		QString text = query().value(1).toString();

		if(((ExifItem::TagFlags)query().value(5).toInt()).testFlag(ExifItem::AsciiAlt))
		{
			if(query().value(8).toString() != "")
				text += " (" + query().value(8).toString() + ")";
		}

		return text;
	}

	if(index.column() == 3)
	{
		// special care for tag type
		if(role == Qt::DisplayRole)
		{
			if(v != QVariant())
				return ExifItem::typeName((ExifItem::TagType)v.toInt());
		}
		else if(role == Qt::FontRole)
		{
			QFont f;
			f.setBold(true);

			return f;
		}
	}

	if(index.column() == 4)
	{
		ExifItem::TagFlags flags = (ExifItem::TagFlags)data(index, GetTagFlagsRole).toInt();

		// special care for select values
		if(flags.testFlag(ExifItem::Choice))
		{
			if(role == Qt::DisplayRole)
			{
				return QString("...");
			}
		}
	}

	return v;
}

// header information
QVariant OptGearTemplateModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(((role != Qt::DisplayRole) && (role != Qt::FontRole) && (role != Qt::ToolTipRole)) || (orientation != Qt::Horizontal))
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
		if(role == Qt::ToolTipRole)
			return tr("Tag order");
		else
			return tr("#");
		break;
	case 1:
		// Tag name
		if(role == Qt::ToolTipRole)
			return tr("Tag name(s); check the item if you want to include it in the image comments.");
		else
			return tr("Exif tag(s)");
		break;
	case 2:
		// Tag text
		if(role == Qt::ToolTipRole)
			return tr("Tag description and display name");
		else
			return tr("Description");
		break;
	case 3:
		// Tag type
		if(role == Qt::ToolTipRole)
			return tr("Tag type; check the item if you want provide selection of the values");
		else
			return tr("Type");
		break;
	case 4:
		// print format
		if(role == Qt::ToolTipRole)
			return tr("For regular types - display format string, %1 will be replaced with tag value.\nFor selection types - selection text and values");
		else
			return tr("Display format");
		break;
	}

	return QVariant();
}

bool OptGearTemplateModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role != Qt::EditRole)
		return false;

	if(!query().seek(index.row()))
		return false;

	int id = query().value(6).toInt();

	if(value == QVariant())
	{
		if((index.column() == 1) && query().value(1).toString().isEmpty())
		{
			// remove cancelled tag
			removeTag(index);
		}

		return false;
	}

	// update the record
	QString queryStr;

	if(index.column() == 0)
	{
		if(query().value(0).toInt() == value.toInt())
			return false;

		queryStr = QString("UPDATE GearTemplate SET OrderBy = %1 WHERE id = %2").arg(value.toInt()).arg(query().value(7).toInt());
	}
	else
	{
		queryStr = "UPDATE MetaTags SET ";

		switch(index.column())
		{
		case 1:
			{
				QVariantList vallist = value.toList();
				if(vallist.count() != 3)
					return false;

				if(vallist.at(0) == QVariant())
					return false;

				// TODO: bad, should be checked in item delegate
				if((query().value(1).toString() == vallist.at(0).toString()) && (query().value(5).toInt() == vallist.at(1).toInt()))
				{
					if(((ExifItem::TagFlags)vallist.at(1).toInt()).testFlag(ExifItem::AsciiAlt))
					{
						if(query().value(8).toString() == vallist.at(2).toString())
							return false;
					}
					else
					{
						return false;
					}
				}

				queryStr += QString("TagName = '%1', Flags = %2").arg(vallist.at(0).toString().remove(QRegExp("(\\s?)")).replace("'", "''")).arg(vallist.at(1).toInt());

				if(((ExifItem::TagFlags)vallist.at(1).toInt()).testFlag(ExifItem::AsciiAlt))
				{
					queryStr += QString(", AltTag = '%1'").arg(vallist.at(2).toString().remove(QRegExp("(\\s?)")).replace("'", "''"));
				}
				else
				{
					// empty alt tag otherwise
					queryStr += QString(", AltTag = ''");
				}
			}
			break;
		case 2:
			{
				if(query().value(2).toString() == value.toString())
					return false;

				queryStr += QString("TagText = '%1'").arg(value.toString().replace("'", "''"));
			}
			break;
		case 3:
			{
				if(query().value(3).toInt() == value.toInt())
					return false;

				queryStr += QString("TagType = %1").arg(value.toInt());
			}
			break;
		case 4:
			{
				if(query().value(4).toString() == value.toString())
					return false;

				queryStr += QString("PrintFormat = '") + value.toString().replace("'", "''") +"'";
			}
			break;
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
int OptGearTemplateModel::insertTag(QString tagName, QString tagDesc, QString tagFormat, ExifItem::TagType tagType)
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

void OptGearTemplateModel::swapOrderBys(const QModelIndex& idx1, const QModelIndex& idx2)
{
	// check for validity
	if(!idx1.isValid() || !idx2.isValid())
		return;

	// get id and orderby values of the first index
	if(!query().seek(idx1.row()))
		return;

	int tagId1 = query().value(7).toInt();
	int orderBy1 = query().value(0).toInt();

	// get id and orderby values of the second index
	if(!query().seek(idx2.row()))
		return;

	int tagId2 = query().value(7).toInt();
	int orderBy2 = query().value(0).toInt();

	// update database
	QSqlQuery updQuery(QString("UPDATE GearTemplate SET OrderBy = %1 WHERE id = %2").arg(orderBy2).arg(tagId1));

	if(updQuery.lastError().isValid())
		return;

	updQuery.exec(QString("UPDATE GearTemplate SET OrderBy = %1 WHERE id = %2").arg(orderBy1).arg(tagId2));

	if(updQuery.lastError().isValid())
		return;

	// notify the view
	reload();
}