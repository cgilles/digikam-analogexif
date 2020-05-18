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

#include "exifitem.h"
#include <QStringList>
#include <QDateTime>

#include "exifutils.h"
#include <cmath>

// insert child
ExifItem* ExifItem::insertChild(const QString& tag, const QString& tagText, const QVariant& tagValue, const QString& printFormat, TagType type, TagFlags flags, const QString& altTag)
{
	ExifItem* child = new ExifItem(tag, tagText, tagValue, printFormat, type, flags, altTag, this);
	childItems.append(child);
	return child;
}

// remove child
bool ExifItem::removeChild(int position)
{
	if((position < 0) || (position > childItems.size()))
		return false;

	delete childItems.takeAt(position);

	return true;
}

// return self child item, 0 for root
int ExifItem::childNumber() const
{
	if(parentItem)
		return parentItem->childItems.indexOf(const_cast<ExifItem*>(this));

	return 0;
}

// reset all tag values
void ExifItem::reset()
{
	dirty = false;
	if(metaTag != "")
	{
		metaValue = QVariant();
	}
	if(childCount() > 0)
	{
		for(int i = 0; i < childCount(); i++)
			child(i)->reset();
	}
}

// set value from string
void ExifItem::setValueFromString(const QVariant& value, bool setDirty, bool convertReal)
{
	if((value != QVariant()) && flags.testFlag(AsciiAlt))
	{
		QVariantList varList = value.toList();
		if(varList.count() > 1)
		{
			QVariantList listValue;
			listValue << valueFromString(varList.at(0).toString(), type, convertReal, flags) << valueFromString(varList.at(1).toString(), type, convertReal, flags);

			setValue(listValue, setDirty);
		}
		else
		{
			setValue(QVariant(), setDirty);
		}
	}
	else
	{
		setValue(valueFromString(value.toString(), type, convertReal, flags), setDirty);
	}
}

ExifItem* ExifItem::findTagByName(const QString& name)
{
	if(metaTag.contains(name))
		return this;

	for(int i = 0; i < childCount(); i++)
	{
		ExifItem* tag = child(i)->findTagByName(name);
		if(tag)
			return tag;
	}

	return NULL;
}

bool ExifItem::findSetTagValueFromString(const QString& tagName, const QVariant& value, bool setDirty)
{
	if(metaTag == tagName)
	{
		setValueFromString(value, setDirty);
		return true;
	}

	for(int i = 0; i < childCount(); i++)
	{
		if(child(i)->findSetTagValueFromString(tagName, value, setDirty))
			return true;
	}

	return false;
}

bool ExifItem::findSetTagValueFromTag(const ExifItem* tag, bool setDirty)
{
	if(metaTag == tag->metaTag)
	{
		metaValue = tag->metaValue;
		dirty = setDirty;
		return true;
	}

	for(int i = 0; i < childCount(); i++)
	{
		if(child(i)->findSetTagValueFromTag(tag, setDirty))
			return true;
	}

	return false;
}

QString ExifItem::typeName(TagType type)
{
	switch(type)
	{
		case TagString:
			return QT_TR_NOOP("string");
			break;
			
		case TagInteger:
			return QT_TR_NOOP("integer");
			break;
			
		case TagUInteger:
			return QT_TR_NOOP("uinteger");
			break;

		case TagRational:
			return QT_TR_NOOP("rational");
			break;

		case TagURational:
			return QT_TR_NOOP("urational");
			break;

		case TagFraction:
			return QT_TR_NOOP("fraction");
			break;

		case TagAperture:
			return QT_TR_NOOP("aperture");
			break;

		case TagApertureAPEX:
			return QT_TR_NOOP("apex aperture");
			break;

		case TagShutter:
			return QT_TR_NOOP("shutter");
			break;

		case TagISO:
			return QT_TR_NOOP("iso");
			break;

		case TagDateTime:
			return QT_TR_NOOP("time");
			break;

		case TagText:
			return QT_TR_NOOP("text");
			break;

		case TagGPS:
			return QT_TR_NOOP("gps");
			break;

		default:
			return QT_TR_NOOP("unknown");
			break;
	}
}

QString ExifItem::getValueAsString(bool convertReal)
{
	return valueToString(metaValue, type, QVariant(), convertReal);
}

QString ExifItem::flagName(TagFlag flag)
{
	switch(flag)
	{
	case Extra:
			return QT_TR_NOOP("Save value in comments");
			break;
			
	case Protected:
			return QT_TR_NOOP("Built-in");
			break;
			
	case Choice:
			return QT_TR_NOOP("Choice of values");
			break;

	case Multi:
			return QT_TR_NOOP("Multiple values");
			break;

	case Ascii:
			return QT_TR_NOOP("7-bit Ascii-only value");
			break;

	case AsciiAlt:
			return QT_TR_NOOP("Ascii-only alternative value");
			break;

	default:
			return QT_TR_NOOP("Unknown");
			break;
	}
}

QString ExifItem::valueToStringMulti(const QVariant& value, TagType type, TagFlags flags, const QVariant& oldValue)
{
	if(flags.testFlag(Multi))
	{
		QString result;

		foreach(QVariant val, value.toList())
		{
			result += valueToString(val, type, oldValue).remove("&&") + "&&";
		}

		// strip last ';'
		return result.left(result.length() - 2);
	}

	return valueToString(value, type, oldValue);
}

QString ExifItem::valueToString(const QVariant& value, TagType type, const QVariant& oldValue, bool convertReal)
{
	if(value == QVariant())
		return "";

	QString valStr;

	switch(type)
	{
	// strings and numbers are stored as text anyway
	case TagString:
	case TagText:
	case TagInteger:
	case TagUInteger:
	case TagISO:
	case TagGPS:
		valStr = value.toString();
		break;
	case TagDateTime:
		valStr = value.toDateTime().toString("yyyy:MM:dd HH:mm:ss");
		break;
	// convert to fraction string
	case TagRational:
	case TagURational:
	case TagAperture:
	case TagApertureAPEX:
		{
			double val = value.toDouble();

			if(!convertReal)
			{
				return QString::number(val);
			}

			int first, second;

			if((oldValue != QVariant()) && (oldValue.toString() != ""))
			{
				QStringList ratioStr = oldValue.toString().split('/', QString::SkipEmptyParts);
				
				bool ok = false;

				first = ratioStr.at(0).toInt(&ok);
				if(!ok)
					return QString();

				second = ratioStr.at(1).toInt(&ok);
				if(!ok)
					return QString();

				// special check whether value is changed for rational numbers
				double oldValue = (double)first / (double)second;

				// compare APEX values up to the second digit after the decimal point
				if(type == ExifItem::TagApertureAPEX)
				{
					if((int)(oldValue*100) == (int)(val*100))
						return QString();
				}
				else
				{
					if(oldValue == val)
						return QString();
				}
			}

			ExifUtils::doubleToFraction(val, &first, &second);

			valStr = QString("%1/%2").arg(first).arg(second);
			break;
		}
	case ExifItem::TagFraction:
	case ExifItem::TagShutter:
		{
			QVariantList rational = value.toList();

			if(rational == QVariantList())
				return QString();

			valStr = QString("%1/%2").arg(rational.at(0).toInt()).arg(rational.at(1).toInt());
			break;
		}
	default:
		return QString();
		break;
	}

	return valStr;
}

QVariant ExifItem::valueToString(const QVariant& value, TagType type, const QString formatString, int role)
{
	switch(type)
	{
	case TagString:
	case TagText:
	case TagGPS:
		if(role == Qt::DisplayRole)
			return formatString.arg(value.toString()).replace(QRegExp("(\r|\n)"), " ");
		else if(role == Qt::EditRole)
			return value;
		break;
	case TagInteger:
	case TagUInteger:
	case TagISO:
		if(role == Qt::DisplayRole)
			return formatString.arg(value.toInt());
		else if(role == Qt::EditRole)
			return value;
		break;
	case TagRational:
	case TagURational:
	case TagAperture:
	case TagApertureAPEX:
		{
			QVariantList rational = value.toList();
			double valueDbl = (double)rational.at(0).toInt() / (double)rational.at(1).toInt();

			// adjust APEX values
			/*if(type == TagApertureAPEX)
				valueDbl = exp(valueDbl*log(2.0)*0.5);*/

			if(role == Qt::DisplayRole)
			{
				if((type == TagAperture) || (type == TagApertureAPEX))
					return formatString.arg(valueDbl, 0, 'f', 1);
				else
					return formatString.arg(ExifUtils::fancyPrintDouble(valueDbl));
			}
			else if(role == Qt::EditRole)
				return valueDbl;
			break;
		}
	case TagFraction:
	case TagShutter:
		{
			QVariantList rational = value.toList();
			double valueDbl = (double)rational.at(0).toInt() / (double)rational.at(1).toInt();

			if(role == Qt::DisplayRole)
			{
				if((valueDbl >= 0.5) && (type == ExifItem::TagShutter))
				{
					return formatString.arg(valueDbl);
				}
				else
				{
					return formatString.arg(QString("%1/%2").arg(rational.at(0).toInt()).arg(rational.at(1).toInt()));
				}
			}
			else if(role == Qt::EditRole)
				return rational;

			break;
		}
	case ExifItem::TagDateTime:
		{
			return QDateTime::fromString(value.toString(), "yyyy:MM:dd HH:mm:ss");
		}
		break;
	default:
		break;
	}

	return value;
}

QVariant ExifItem::valueFromString(const QString& value, TagType type, bool convertReal, TagFlags flags)
{
	// special care for multi tags
	if(flags.testFlag(Multi))
	{
		QVariantList values;

		// split the values
		QStringList strValues = value.split("&&", QString::SkipEmptyParts);

		// convert each value individually
		foreach(QString strVal, strValues)
		{
			values << valueFromString(strVal, type, convertReal);
		}

		return values;
	}

	switch(type)
	{
	case TagString:
	case TagText:
		return value;
		break;
	case TagGPS:
		if(value == "")
			return QVariant();
		else
			return value;
		break;
	case TagInteger:
	case TagUInteger:
	case TagISO:
		{
			bool ok = false;
			QVariant val = value.toInt(&ok);
			if(!ok)
				return QVariant();
			else
				return val;
		}
		break;
	case TagRational:
	case TagURational:
	case TagAperture:
	case TagApertureAPEX:
		{
			double val;
			bool ok = false;

			if(!convertReal)
			{
				val = value.toDouble(&ok);

				if(ok)
					return val;
				else
					return QVariant();
			}

			QStringList ratioStr = value.split('/', QString::SkipEmptyParts);

			if(ratioStr.count() < 2)
				return QVariant();

			int first = ratioStr.at(0).toInt(&ok);
			if(!ok)
				return QVariant();

			int second = ratioStr.at(1).toInt(&ok);
			if(!ok)
				return QVariant();

			val = (double)first / (double)second;

			return val;
		}
		break;
	case TagFraction:
	case TagShutter:
		{
			QStringList ratioStr = value.split('/', QString::SkipEmptyParts);

			if(ratioStr.count() < 2)
				return QVariant();
			
			bool ok = false;

			int first = ratioStr.at(0).toInt(&ok);
			if(!ok)
				return QVariant();

			int second = ratioStr.at(1).toInt(&ok);
			if(!ok)
				return QVariant();

			QVariantList rational;
			rational << first << second;

			return rational;
		}
		break;
        case TagDateTime:
		return QDateTime::fromString(value, "yyyy:MM:dd HH:mm:ss");
		break;
        default:
                break;
	}

	return QVariant();
}

QList<QVariantList> ExifItem::parseEncodedChoiceList(QString list, TagType dataType, TagFlags flags)
{
	QList<QVariantList> res;

	QStringList items = list.split(";;", QString::SkipEmptyParts);

	foreach(QString itemPair, items)
	{
		QStringList values = itemPair.split("||", QString::SkipEmptyParts);
		if(values.count() != 2)
			return QList<QVariantList>();

		QVariantList varList;
		varList << values.at(0) << valueFromString(values.at(1), dataType, true, flags);

		res << varList;
	}

	return res;
}

QString ExifItem::findChoiceTextByValue(QString list, QVariant value, TagType dataType, TagFlags flags)
{
	QStringList items = list.split(";;", QString::SkipEmptyParts);

	foreach(QString itemPair, items)
	{
		QStringList values = itemPair.split("||", QString::SkipEmptyParts);
		if(values.count() != 2)
			return QString();

		if(valueFromString(values.at(1), dataType, true, flags) == value)
			return values.at(0);
	}

	return QString();
}
