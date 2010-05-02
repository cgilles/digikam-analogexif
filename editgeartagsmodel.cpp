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

// TODO: should be abstracted
QVariant EditGearTagsModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	// return only for Display, Edit and GetTypeRole role
	if (role != Qt::DisplayRole && role != Qt::EditRole && role != ExifTreeModel::GetTypeRole)
		return QSqlQueryModel::data(index, role);

	if(!query().seek(index.row()))
		return QVariant();

	// return tag text for column 0
	if((index.column() == 0)  && (role == Qt::DisplayRole))
		return query().value(0).toString();

	int tagType = query().value(2).toInt();

	// return tag type for GetTypeRole
	if(role == ExifTreeModel::GetTypeRole)
		return tagType;

	QVariant itemValue = query().value(1);
	if(itemValue == QVariant())
		return QVariant();

	QString formatString = query().value(3).toString();

	// return value according to the tag type
	switch(tagType)
	{
	case ExifItem::TagString:
		if(role == Qt::DisplayRole)
			return formatString.arg(itemValue.toString());
		else if(role == Qt::EditRole)
			return itemValue;
		break;
	case ExifItem::TagInteger:
	case ExifItem::TagUInteger:
	case ExifItem::TagISO:
		if(role == Qt::DisplayRole)
			return formatString.arg(itemValue.toInt());
		else if(role == Qt::EditRole)
			return itemValue;
		break;
	case ExifItem::TagRational:
	case ExifItem::TagURational:
	case ExifItem::TagAperture:
	case ExifItem::TagApertureAPEX:
		{
			QStringList ratioStr = itemValue.toString().split('/', QString::SkipEmptyParts);

			if(ratioStr.count() < 2)
				return QVariant();

			bool ok = false;

			int first = ratioStr.at(0).toInt(&ok);
			if(!ok)
				return QVariant();

			int second = ratioStr.at(1).toInt(&ok);
			if(!ok)
				return QVariant();

			double value = (double)first / (double)second;

			// adjust APEX values
			if(tagType == ExifItem::TagApertureAPEX)
				value = exp(value*log(2.0)*0.5);

			if(role == Qt::DisplayRole)
				return formatString.arg(value, 0, 'f', 1);
			else if(role == Qt::EditRole)
				return value;
			break;
		}
	case ExifItem::TagFraction:
	case ExifItem::TagShutter:
		{
			QStringList ratioStr = itemValue.toString().split('/', QString::SkipEmptyParts);

			if(ratioStr.count() < 2)
				return QVariant();
			
			bool ok = false;

			int first = ratioStr.at(0).toInt(&ok);
			if(!ok)
				return QVariant();

			int second = ratioStr.at(1).toInt(&ok);
			if(!ok)
				return QVariant();

			double value = (double)first / (double)second;

			if(role == Qt::DisplayRole)
			{
				if((value >= 0.5) && (tagType == ExifItem::TagShutter))
				{
					return formatString.arg(value);
				}
				else
				{
					return formatString.arg(QString("%1/%2").arg(first).arg(second));
				}
			}
			else if(role == Qt::EditRole)
			{
				QVariantList rational;
				rational << first << second;

				return rational;
			}

			break;
		}
	default:
		break;
	}

	// just return item value
	return itemValue;
}

// TODO: again, should be abstracted somewhere
bool EditGearTagsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role != Qt::EditRole)
		return false;

	if(!query().seek(index.row()))
		return false;

	QString updateValue;
	int tagType = query().value(2).toInt();

	// return value according to the tag type
	switch(tagType)
	{
	// strings and numbers are stored as text anyway
	case ExifItem::TagString:
	case ExifItem::TagInteger:
	case ExifItem::TagUInteger:
	case ExifItem::TagISO:
		updateValue = value.toString();
		break;
	// convert to fraction string
	case ExifItem::TagRational:
	case ExifItem::TagURational:
	case ExifItem::TagAperture:
	case ExifItem::TagApertureAPEX:
		{
			double val = value.toDouble();

			// correction for APEX values
			if(tagType == ExifItem::TagApertureAPEX)
			{
				val = 2*log(val)/log(2.0);
			}

			int first, second;

			if(query().value(1).toString() != "")
			{
				QStringList ratioStr = query().value(1).toString().split('/', QString::SkipEmptyParts);
				
				bool ok = false;

				first = ratioStr.at(0).toInt(&ok);
				if(!ok)
					return false;

				second = ratioStr.at(1).toInt(&ok);
				if(!ok)
					return false;

				// special check whether value is changed for rational numbers
				double oldValue = (double)first / (double)second;

				// compare APEX values up to the second digit after the decimal point
				if(tagType == ExifItem::TagApertureAPEX)
				{
					if((int)(oldValue*100) == (int)(val*100))
						return false;
				}
				else
				{
					if(oldValue == val)
						return false;
				}
			}

			ExifUtils::doubleToFraction(val, &first, &second);

			updateValue = QString("%1/%2").arg(first).arg(second);
			break;
		}
	case ExifItem::TagFraction:
	case ExifItem::TagShutter:
		{
			QVariantList rational = value.toList();

			if(rational == QVariantList())
				return false;

			updateValue = QString("%1/%2").arg(rational.at(0).toInt()).arg(rational.at(1).toInt());
			break;
		}
	default:
		return false;
		break;
	}

	emit dataChanged(index, index);

	// update the record
	QSqlQuery updQuery(QString("UPDATE UserGearProperties SET TagValue = '%1' WHERE id = %2").arg(updateValue).arg(query().value(4).toInt()));

	if(updQuery.lastError().isValid())
		return false;

	reload();

	return true;
}

void EditGearTagsModel::reload(int id)
{
	gearId = id;
	setQuery(QString("SELECT a.TagText, b.TagValue, a.TagType, a.PrintFormat, b.id FROM MetaTags a, UserGearProperties b WHERE b.GearId = %1 AND a.id = b.TagId ORDER BY b.OrderBy").arg(id));
}