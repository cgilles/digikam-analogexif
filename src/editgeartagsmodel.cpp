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

#include "editgeartagsmodel.h"
#include "exiftreemodel.h"
#include "exifitem.h"
#include "exifutils.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QStringList>

#include <cmath>

Qt::ItemFlags EditGearTagsModel::flags(const QModelIndex &index) const
{
	if(!index.isValid())
		return 0;

	if(index.column() == 1)
		return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant EditGearTagsModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if(!query().seek(index.row()))
		return QVariant();

	// return tag text for column 0
	if((index.column() == 0)  && (role == Qt::DisplayRole))
		return query().value(0).toString();

	ExifItem::TagType tagType = (ExifItem::TagType)query().value(2).toInt();
	QString formatString = query().value(3).toString();
	ExifItem::TagFlags tagFlags = (ExifItem::TagFlags)query().value(5).toInt();

	// return tag id
	if(role == GetTagIdRole)
		return query().value(4);

	if(index.column() == 1)
	{
		QVariant itemValue = ExifItem::valueFromString(query().value(1).toString(), tagType, true, tagFlags);

		if(tagFlags.testFlag(ExifItem::AsciiAlt))
		{
			QVariantList varList;
			varList << itemValue << ExifItem::valueFromString(query().value(6).toString(), tagType, true, tagFlags);

			itemValue = varList;
		}

		if ((index.column() == 1) && (
			(role == Qt::EditRole) || (role == Qt::DisplayRole) || (role == ExifTreeModel::GetFlagsRole) ||
			(role == ExifTreeModel::GetChoiceRole) || (role == ExifTreeModel::GetTypeRole)
			))
			return ExifTreeModel::getItemData(itemValue, formatString, tagFlags, tagType, role);
	}

	return QSqlQueryModel::data(index, role);
}

// TODO: again, should be abstracted somewhere
bool EditGearTagsModel::setData(const QModelIndex &index, const QVariant &dataValue, int role)
{
	if (role != Qt::EditRole)
		return false;

	if(!query().seek(index.row()))
		return false;

	ExifItem::TagType tagType = (ExifItem::TagType)query().value(2).toInt();
	ExifItem::TagFlags tagFlags = (ExifItem::TagFlags)query().value(5).toInt();

	// return value according to the tag type
	QString updateValue;
	QString updateAltValue;

	QVariant oldValue = query().value(1);
	QVariant value = dataValue;

	QVariantList varList;

	if(tagFlags.testFlag(ExifItem::AsciiAlt) && (dataValue != QVariant()))
	{
		varList = dataValue.toList();
		if(!varList.isEmpty())
			value = varList.at(0);
	}

	// special consideration for APEX-adjusted values
	if((value != QVariant()) && (tagType == ExifItem::TagApertureAPEX))
	{
		QVariant apexValue = 2*log(value.toDouble())/log(2.0);
		updateValue = ExifItem::valueToStringMulti(apexValue, tagType, tagFlags, oldValue);

		if(tagFlags.testFlag(ExifItem::AsciiAlt) && (varList.count() > 1))
		{
			QVariant apexAltValue = 2*log(varList.at(1).toDouble())/log(2.0);
			updateAltValue = ExifItem::valueToStringMulti(apexAltValue, tagType, tagFlags, oldValue);
		}
	}
	else
	{
		updateValue = ExifItem::valueToStringMulti(value, tagType, tagFlags, oldValue);

		if(tagFlags.testFlag(ExifItem::AsciiAlt) && (varList.count() > 1))
		{
			updateAltValue = ExifItem::valueToStringMulti(varList.at(1), tagType, tagFlags, oldValue);
		}
	}

	if(updateValue.isNull())
		return false;

	// update the record
	QSqlQuery updQuery(QString("UPDATE UserGearProperties SET TagValue = '%1', AltValue = '%2' WHERE id = %3").arg(updateValue.replace("'", "''")).arg(updateAltValue.replace("'", "''")).arg(query().value(4).toInt()));

	if(updQuery.lastError().isValid())
		return false;

	reload();
	emit dataChanged(index, index);

	return true;
}

void EditGearTagsModel::reload(int id)
{
	gearId = id;
	setQuery(QString("SELECT a.TagText, b.TagValue, a.TagType, a.PrintFormat, b.id, a.Flags, b.AltValue FROM MetaTags a, UserGearProperties b WHERE b.GearId = %1 AND a.id = b.TagId ORDER BY b.OrderBy").arg(id));
}

bool EditGearTagsModel::addNewTag(int tagId, int orderBy)
{
	QSqlQuery updQuery(QString("INSERT INTO UserGearProperties(GearId, TagId, OrderBy) VALUES(%1, %2, %3)").arg(gearId).arg(tagId).arg(orderBy));
	if(updQuery.lastError().isValid())
		return false;

	reload();

	return true;
}

bool EditGearTagsModel::deleteTag(int tagId)
{
	QSqlQuery updQuery(QString("DELETE FROM UserGearProperties WHERE id = %1").arg(tagId));
	if(updQuery.lastError().isValid())
		return false;

	reload();

	return true;
}