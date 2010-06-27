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

#include "exiftreemodel.h"
#include "exifutils.h"

#include <QSqlQuery>
#include <QStringList>
#include <QVariantList>
#include <QDateTime>
#include <QFont>
#include <QBrush>
#include <QMessageBox>

#include <QFile>
#include <QTextStream>

#include <cmath>

#include <exiv2/tags.hpp>
#include <exiv2/properties.hpp>
#include <exiv2/datasets.hpp>

#define CUSTOM_XMP_NAMESPACE_URI		("http://www.c41bytes.com/analogexif/ns")
#define ETAGS_START_MARKER_IN_COMMENTS	(QT_TR_NOOP("Photo information: \n"))
#define ETAGS_DELIMETER_IN_COMMENTS		(" \n\n")

ExifTreeModel::ExifTreeModel(QObject *parent) : QAbstractItemModel(parent)
{
	// create empty root item
	rootItem = new ExifItem("", "", QVariant());

	// register custom AnalogExif XMP namespace
	try
	{
		Exiv2::XmpProperties::registerNs(CUSTOM_XMP_NAMESPACE_URI, "AnalogExif");
	}
	catch(Exiv2::Error& exc)
	{
		QMessageBox::critical(NULL, tr("Error registering AnalogExif XMP schema"), tr("Unable to register AnalogExif XMP schema.\n\n%1.").arg(exc.what()), QMessageBox::Ok);
	}

	// create and read the values of the used metatags
	populateModel();
	editable = false;

	fillNotSupportedTags();
}

ExifTreeModel::~ExifTreeModel()
{
	delete rootItem;
}

// clear data
void ExifTreeModel::clear(bool deleteObj)
{
	reset();
	rootItem->reset();

	curExifData.clear();
	curIptcData.clear();
	curXmpData.clear();

	if(deleteObj)
		delete exifHandle.release();

	resetDirty();
}

static QString customUserNs;
static QString customUserNsPrefix;

bool ExifTreeModel::registerUserNs(QString userNs, QString userNsPrefix)
{
	try
	{
		Exiv2::XmpProperties::registerNs(userNs.toStdString(), userNsPrefix.toStdString());
	}
	catch(Exiv2::Error& exc)
	{
		customUserNs = customUserNsPrefix = "";

		return false;
	}

	customUserNs = userNs;
	customUserNsPrefix = userNsPrefix;

	return true;
}

bool ExifTreeModel::unregisterUserNs()
{
	if(customUserNs == "")
		return false;

	try
	{
		Exiv2::XmpProperties::unregisterNs(customUserNs.toStdString());

		customUserNs = customUserNsPrefix = "";
	}
	catch(Exiv2::Error& exc)
	{
		return false;
	}

	return true;
}

// open file and read metadata
bool ExifTreeModel::openFile(QString filename)
{
	// invalidate the model
	clear(true);

	try
	{
		// open file using Exiv2 library
#ifdef Q_WS_WIN
		// unicode paths supported only in windows version
		exifHandle = Exiv2::ImageFactory::open(filename.toStdWString());
#else
		// convert to UTF-8
		exifHandle = Exiv2::ImageFactory::open(filename.toUtf8().data());
#endif
		if(exifHandle.get() == 0)
			return false;
		// read metadata
		exifHandle->readMetadata();
	}
	catch(Exiv2::AnyError& err)
	{
		qDebug("AnalogExif: ExifTreeModel::openFile(%s) Exiv2 exception (%d) = %s", filename.toStdString().c_str(), err.code(), err.what());
		return false;
	}

	// read all tags from model
	if(readMetaValues())
	{
		// editable = true;
		return true;
	}

	return false;
}

// get item by its index
ExifItem *ExifTreeModel::getItem(const QModelIndex &index) const
{
	if (index.isValid())
	{
		ExifItem *item = static_cast<ExifItem*>(index.internalPointer());
		if (item) return item;
	}
	return rootItem;
}

// return number of rows
int ExifTreeModel::rowCount(const QModelIndex &parent) const
{
	ExifItem* parentItem = getItem(parent);

	return parentItem->childCount();
}

// return item flags for tree model
Qt::ItemFlags ExifTreeModel::flags(const QModelIndex &index) const
{
	if(!index.isValid())
		return 0;

	// caption or tag text - selectable, enabled, non-editable
	if(index.column() == 0)
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	if(editable)
		return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	else
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

// create index for the given position in the tree
QModelIndex ExifTreeModel::index(int row, int column, const QModelIndex &parent) const
{
	// no index for the extra columns
	if (parent.isValid() && parent.column() != 0)
		return QModelIndex();

	ExifItem *parentItem = getItem(parent);
	ExifItem *childItem = parentItem->child(row);

	if (childItem)
		return createIndex(row, column, childItem);
	else
		return QModelIndex();
}

// return index of a parent
QModelIndex ExifTreeModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	ExifItem *childItem = getItem(index);
	ExifItem *parentItem = childItem->parent();

	if (parentItem == rootItem)
		return QModelIndex();

	return createIndex(parentItem->childNumber(), 0, parentItem);
}

QVariant ExifTreeModel::getItemValue(const QVariant& itemValue, const QString& itemFormat, ExifItem::TagFlags tagFlags, ExifItem::TagType itemType, int role)
{
	// return value according to the tag type
	switch(itemType)
	{
	case ExifItem::TagString:
	case ExifItem::TagText:
	case ExifItem::TagGPS:
		if(role == Qt::DisplayRole)
		{
			// special care for alt-Ascii values
			if(tagFlags.testFlag(ExifItem::AsciiAlt))
			{
				QStringList strList = itemValue.toStringList();

				if((strList != QStringList()) && (strList.count() > 1))
				{
					QString text = QString(itemFormat).arg(strList.at(0)).replace(QRegExp("(\r|\n)"), " ").replace(QRegExp("(\\s)+"), " ");
					
					if((strList.at(1) != "") && (strList.at(1) != strList.at(0)))
						text += " (" + QString(itemFormat).arg(strList.at(1)).replace(QRegExp("(\r|\n)"), " ").replace(QRegExp("(\\s)+"), " ") + ")";

					return text;
				}
				else
				{
					return QVariant();
				}
			}
			else
			{
				return QString(itemFormat).arg(itemValue.toString()).replace(QRegExp("(\r|\n)"), " ").replace(QRegExp("(\\s)+"), " ");
			}
		}
		else if(role == Qt::EditRole)
			return itemValue;
		break;
	case ExifItem::TagInteger:
	case ExifItem::TagUInteger:
	case ExifItem::TagISO:
		if(role == Qt::DisplayRole)
			return QString(itemFormat).arg(itemValue.toInt());
		else if(role == Qt::EditRole)
			return itemValue;
		break;
	case ExifItem::TagRational:
	case ExifItem::TagURational:
	case ExifItem::TagAperture:
	case ExifItem::TagApertureAPEX:
		{
			double value = itemValue.toDouble();

			// adjust APEX values
			if(itemType == ExifItem::TagApertureAPEX)
				value = exp(value*log(2.0)*0.5);

			if(role == Qt::DisplayRole)
			{
				if((itemType == ExifItem::TagAperture) || (itemType == ExifItem::TagApertureAPEX))
					return itemFormat.arg(value, 0, 'f', 1);
				else
					return itemFormat.arg(ExifUtils::fancyPrintDouble(value));
			}
			else if(role == Qt::EditRole)
				return value;
			break;
		}
	case ExifItem::TagFraction:
	case ExifItem::TagShutter:
		{
			QVariantList rational = itemValue.toList();
			double value = (double)rational.at(0).toInt() / (double)rational.at(1).toInt();

			if(role == Qt::DisplayRole)
			{
				if((value >= 0.5) && (itemType == ExifItem::TagShutter))
				{
					return QString(itemFormat).arg(value);
				}
				else
				{
					QChar signChar = QChar();
					if((itemType == ExifItem::TagFraction) &&(value > 0))
						signChar = '+';

					int first = rational.at(0).toInt();
					int second = rational.at(1).toInt();

					if(abs(first) > abs(second))
					{
						int val = first / abs(second);
						int frac = abs(first) % abs(second);
						return QString(itemFormat).arg(QString("%1%2 %3/%4").arg(signChar).arg(val).arg(frac).arg(second));
					}

					return QString(itemFormat).arg(QString("%1%2/%3").arg(signChar).arg(first).arg(second));
				}
			}
			else if(role == Qt::EditRole)
				return rational;

			break;
		}
	case ExifItem::TagDateTime:
		{
			return QDateTime::fromString(itemValue.toString(), "yyyy:MM:dd HH:mm:ss");
		}
		break;
	default:
		break;
	}

	// just return item value
	return itemValue;
}

QVariant ExifTreeModel::getItemData(const QVariant& itemValue, const QString& itemFormat, ExifItem::TagFlags itemFlags, ExifItem::TagType itemType, int role)
{
	// return tag type
	if(role == GetTypeRole)
		return itemType;

	// return tag flags
	if(role == GetFlagsRole)
		return (int)itemFlags;

	// return tag format/choice
	if(role == GetChoiceRole)
		return itemFormat;

	if((role != Qt::EditRole) && (role != Qt::DisplayRole))
		return QVariant();

	if(itemValue == QVariant())
		return QVariant();

	if(itemFlags.testFlag(ExifItem::Choice) && (role == Qt::DisplayRole))
	{
		// special care for choice tags
		return ExifItem::findChoiceTextByValue(itemFormat, itemValue, itemType, itemFlags);
	}

	if(!itemFlags.testFlag(ExifItem::Choice) && itemFlags.testFlag(ExifItem::Multi))
	{
		if(role == Qt::EditRole)
		{
			// for edit - process the whole list before giving up to editor (required for e.g. APEX adjustments)
			QVariantList processedList;
			foreach(QVariant value, itemValue.toList())
			{
				processedList << getItemValue(value, itemFormat, itemFlags, itemType, role);
			}

			return processedList;
		}
		else if(role == Qt::DisplayRole)
		{
			QString result;
			foreach(QVariant value, itemValue.toList())
			{
				result += getItemValue(value, itemFormat, itemFlags, itemType, role).toString() + "; ";
			}

			// erase last "; "
			return result.left(result.length() - 2);
		}
	}

	return getItemValue(itemValue, itemFormat, itemFlags, itemType, role);
}

// return item data
QVariant ExifTreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	// return only for Display and Edit role
	if (role != Qt::DisplayRole && role != Qt::EditRole && role != GetTypeRole && role != Qt::FontRole && role != GetFlagsRole && role != GetChoiceRole)
		return QVariant();

	ExifItem *item = getItem(index);

	// for caption and column 0 - return caption
	if(item->isCaption())
	{
		if((index.column() == 0) && (role == Qt::DisplayRole))
			return item->value();
		else
			return QVariant();
	}

	if((index.column() == 1) && (role == Qt::FontRole) && item->isDirty())
	{
		QFont f;
		f.setBold(true);

		return f;
	}

	// for meta data return tag name for column 0 and value for column 1
	if((index.column() == 0)  && (role == Qt::DisplayRole))
		return item->tagText();

	return getItemData(item->value(), item->format(), item->tagFlags(), (ExifItem::TagType)item->tagType(), role);
}

QVariant ExifTreeModel::processItemData(const ExifItem *item, const QVariant& value, bool& ok)
{
	ok = true;
	// return value according to the tag type
	switch(item->tagType())
	{
	case ExifItem::TagString:
	case ExifItem::TagText:
	case ExifItem::TagGPS:
	case ExifItem::TagInteger:
	case ExifItem::TagUInteger:
	case ExifItem::TagISO:
	case ExifItem::TagFraction:
	case ExifItem::TagShutter:
		return value;
		break;
	case ExifItem::TagRational:
	case ExifItem::TagURational:
	case ExifItem::TagAperture:
	case ExifItem::TagApertureAPEX:
		{
			double val = value.toDouble();

			// correction for APEX values
			if(item->tagType() == ExifItem::TagApertureAPEX)
			{
				val = 2*log(val)/log(2.0);
			}

			if(!item->tagFlags().testFlag(ExifItem::Multi) && (item->value() != QVariant()))
			{
				// special check whether value is changed for rational numbers
				double oldvalue = item->value().toDouble();

				if((int)(oldvalue*100) == (int)(val*100))
				{
					ok = false;
					return QVariant();
				}
			}
			return val;
			break;
		}
	case ExifItem::TagDateTime:
		return value.toDateTime().toString("yyyy:MM:dd HH:mm:ss");
		break;
        default:
                break;
	}

	ok = false;
	return QVariant();
}

// change the edited data
bool ExifTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role != Qt::EditRole)
		return false;

	ExifItem *item = getItem(index);

	// don't change the same values
	if(item->value() == value)
		return false;

	if(value == QVariant())
	{
		item->setValue(QVariant(), true);
	}
	else
	{
		bool ok = false;

		QVariant newValue;

		if(item->tagFlags().testFlag(ExifItem::Multi))
		{
			QVariantList valueList;

			// process each value in the list
			foreach(QVariant listValue, value.toList())
			{
				valueList << processItemData(item, listValue, ok);
			}

			newValue = valueList;
		}
		else
		{
			newValue = processItemData(item, value, ok);
		}

		if(!ok)
			return false;

		item->setValue(newValue, true);
	}

	emit dataChanged(index, index);

	return true;
}

// populate model
void ExifTreeModel::populateModel()
{
	// connect to internal database
	QSqlQuery query("SELECT a.GearType, b.TagName, b.TagText, b.PrintFormat, b.TagType, b.Flags, b.AltTag FROM GearTemplate a, MetaTags b WHERE b.id=a.TagId ORDER BY a.GearType, a.OrderBy");

	int curCategoryId = -1, nRows = 0;
	ExifItem* headerItem = NULL;

	while(query.next())
	{
		if(query.value(0).toInt() != curCategoryId)
		{
			// we have a new category - create a head element
			curCategoryId = query.value(0).toInt();

			QString categoryTitle;

			switch(curCategoryId)
			{
			case 0:
				categoryTitle = tr("Camera");
				break;
			case 1:
				categoryTitle = tr("Lens");
				break;
			case 2:
				categoryTitle = tr("Film");
				break;
			case 3:
				categoryTitle = tr("Developer");
				break;
			case 4:
				categoryTitle = tr("Author");
				break;
			case 5:
				categoryTitle = tr("Photo");
				break;
			}

			headerItem = rootItem->insertChild("", "", categoryTitle);
			nRows++;
			
		}

		// insert a tag
		headerItem->insertChild(query.value(1).toString(), query.value(2).toString(), QVariant(), query.value(3).toString(), (ExifItem::TagType)query.value(4).toInt(), (ExifItem::TagFlags)query.value(5).toInt(), query.value(6).toString());
		nRows++;
	}
	beginInsertRows(QModelIndex(), 0, nRows);
	endInsertRows();
}

QString ExifTreeModel::getGPSfromXmp()
{
	QString gpsPosition = "";

	Exiv2::XmpData::iterator pos = curXmpData.findKey(Exiv2::XmpKey("Xmp.exif.GPSLatitude"));
	if(pos != curXmpData.end())
	{
		QString latitude = QString::fromStdString(curXmpData["Xmp.exif.GPSLatitude"].toString());

		QStringList latStrs = latitude.split(QChar(','));
		if(latStrs.count() == 3)
		{
			if(latStrs.at(2).at(latStrs.at(2).length() - 1) == 'N')
				gpsPosition = "+";
			else
				gpsPosition = "-";

			gpsPosition += QString("%1\u00B0 %2' %3\" ").arg(latStrs.at(0)).arg(latStrs.at(1)).arg(latStrs.at(2).left(latStrs.at(2).length() - 1));
		}
		else if(latStrs.count() == 2)
		{
			if(latStrs.at(1).at(latStrs.at(1).length() - 1) == 'N')
				gpsPosition = "+";
			else
				gpsPosition = "-";

			double minVal = latStrs.at(1).left(latStrs.at(1).length() - 1).toDouble();

			double secVal = modf(minVal, NULL);

			gpsPosition += QString("%1\u00B0 %2' %3\" ").arg(latStrs.at(0)).arg((int)minVal, 2, 10, QChar('0')).arg(secVal, 2, 'f', 3, QChar('0'));
		}
		else
			return "";

		pos = curXmpData.findKey(Exiv2::XmpKey("Xmp.exif.GPSLongitude"));
		if(pos != curXmpData.end())
		{
			QString longitude = QString::fromStdString(curXmpData["Xmp.exif.GPSLongitude"].toString());

			QStringList longStrs = longitude.split(QChar(','));
			if(longStrs.count() == 3)
			{
				if(longStrs.at(2).at(longStrs.at(2).length() - 1) == 'E')
					gpsPosition += "+";
				else
					gpsPosition += "-";

				gpsPosition += QString("%1\u00B0 %2' %3\"").arg(longStrs.at(0)).arg(longStrs.at(1)).arg(longStrs.at(2).left(longStrs.at(2).length() - 1));
			}
			else if(longStrs.count() == 2)
			{
				if(longStrs.at(1).at(longStrs.at(1).length() - 1) == 'E')
					gpsPosition += "+";
				else
					gpsPosition += "-";

				double minVal = longStrs.at(1).left(longStrs.at(1).length() - 1).toDouble();

				double secVal = modf(minVal, NULL);

				gpsPosition += QString("%1\u00B0 %2' %3\" ").arg(longStrs.at(0)).arg((int)minVal, 2, 10, QChar('0')).arg(secVal, 2, 'f', 3, QChar('0'));
			}
			else
				return "";

			return gpsPosition;
		}
	}

	return "";
}

QString ExifTreeModel::getGPSfromExif()
{
	QString gpsPosition = "";

	Exiv2::ExifData::iterator pos = curExifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLatitudeRef"));

	if(pos != curExifData.end())
	{
		if(curExifData["Exif.GPSInfo.GPSLatitudeRef"].toString() == "N")
			gpsPosition = "+";
		else
			gpsPosition = "-";

		pos = curExifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLatitude"));
		if(pos != curExifData.end())
		{
			Exiv2::Exifdatum& latitude = curExifData["Exif.GPSInfo.GPSLatitude"];

			Exiv2::Rational deg = latitude.toRational(0);
			Exiv2::Rational min = latitude.toRational(1);
			Exiv2::Rational sec = latitude.toRational(2);

			double secDouble = (double)sec.first / (double)sec.second;

			if(min.second != 1)
			{
				double minDouble, frac;
				
				frac = modf((double)min.first / (double)min.second, &minDouble);

				min.first = minDouble;
				min.second = 1;

				secDouble += frac * 60.0;
			}

			gpsPosition += QString("%1\u00B0 %2' %3\" ").arg(deg.first, 2, 10, QChar('0')).arg(min.first, 2, 10, QChar('0')).arg(secDouble, 2, 'f', 3, QChar('0'));

			pos = curExifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLongitudeRef"));
			if(pos != curExifData.end())
			{
				if(curExifData["Exif.GPSInfo.GPSLongitudeRef"].toString() == "E")
					gpsPosition += "+";
				else
					gpsPosition += "-";

				pos = curExifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLongitude"));
				if(pos != curExifData.end())
				{
					Exiv2::Exifdatum& longitude = curExifData["Exif.GPSInfo.GPSLongitude"];
					Exiv2::Rational deg = longitude.toRational(0);
					Exiv2::Rational min = longitude.toRational(1);
					Exiv2::Rational sec = longitude.toRational(2);

					double secDouble = (double)sec.first / (double)sec.second;

					if(min.second != 1)
					{
						double minDouble, frac;
						
						frac = modf((double)min.first / (double)min.second, &minDouble);

						min.first = minDouble;
						min.second = 1;

						secDouble += frac * 60.0;
					}

					gpsPosition += QString("%1\u00B0 %2' %3\"").arg(deg.first, 3, 10, QChar('0')).arg(min.first, 2, 10, QChar('0')).arg(secDouble, 2, 'f', 3, QChar('0'));

					return gpsPosition;
				}
			}
		}
	}

	return "";
}

bool ExifTreeModel::readMetaValues()
{
	return readMetaValues(exifHandle);
}

// reads the value from Exif, may throw Exiv2 exceptions
QVariant ExifTreeModel::getTagValueFromExif(ExifItem::TagType tagType, const Exiv2::Value& tagValue, int pos) const
{
	switch(tagValue.typeId())
	{
	case Exiv2::asciiString:
	case Exiv2::string:
	case Exiv2::comment:
	case Exiv2::date:
	case Exiv2::time:
		// Local 8bit instead of Unicode
		return ExifItem::valueFromString(QString::fromLocal8Bit(tagValue.toString(pos).c_str()), tagType);
		break;
	case Exiv2::unsignedByte:
	case Exiv2::unsignedShort:
	case Exiv2::unsignedLong:
	case Exiv2::signedByte:
	case Exiv2::signedShort:
	case Exiv2::signedLong:
	case Exiv2::undefined:
		return QVariant::fromValue(tagValue.toLong(pos));
		break;
	case Exiv2::unsignedRational:
	case Exiv2::signedRational:
		{
			Exiv2::Rational val = tagValue.toRational(pos);
			if((tagType == ExifItem::TagFraction) || (tagType == ExifItem::TagShutter))
			{
				QVariantList rational;
				rational << val.first << val.second;
				return rational;
			}
			else
			{
				return tagValue.toFloat(pos);
			}
			break;
		}
        default:
                break;
	}

	return QVariant();
}

QVariant ExifTreeModel::readTagValue(QString& tagNames, int& srcTagType, ExifItem::TagType type, ExifItem::TagFlags tagFlags, Exiv2::ExifData& exifData, Exiv2::IptcData& iptcData, Exiv2::XmpData& xmpData)
{
	// get list of tags
	QStringList tags = tagNames.remove(QChar(' ')).split(",", QString::SkipEmptyParts);

	foreach(QString tagName, tags)
	{
		QString tagType = tagName.split(".").at(0);
		if(tagType == "Exif")
		{
			// Exif data
			
			// search for the key
			Exiv2::ExifKey exifKey(tagName.toStdString());

			Exiv2::ExifData::const_iterator pos = exifData.findKey(exifKey);

			if(pos == exifData.end())
			{
				continue;
			}

			const Exiv2::Value& tagValue = pos->value();

			Exiv2::TypeId typId = tagValue.typeId();

			srcTagType = (int)typId;

			// need special care for comments
			if(tagName == "Exif.Photo.UserComment")
			{
				// check for charset marker, UTF-16
				QString commentValue = ExifUtfToQString(tagValue, true);

				// strip tags values
				int etagsStartIndex = commentValue.indexOf(ETAGS_START_MARKER_IN_COMMENTS);

				if(etagsStartIndex != -1)
				{
					int delimeterLen = QString(ETAGS_DELIMETER_IN_COMMENTS).length();
					if(commentValue.indexOf(ETAGS_DELIMETER_IN_COMMENTS, etagsStartIndex - delimeterLen) == (etagsStartIndex - delimeterLen))
						etagsStartIndex -= delimeterLen;

					commentValue = commentValue.left(etagsStartIndex);
				}

				return commentValue.replace(" \n", "\n");
			}
			else if(tagName == "Exif.Image.XPComment")
			{
				// UTF-16, no charset marker
				QString commentValue = ExifUtfToQString(tagValue);

				// strip tags values
				int etagsStartIndex = commentValue.indexOf(ETAGS_START_MARKER_IN_COMMENTS);

				if(etagsStartIndex != -1)
				{
					int delimeterLen = QString(ETAGS_DELIMETER_IN_COMMENTS).length();
					if(commentValue.indexOf(ETAGS_DELIMETER_IN_COMMENTS, etagsStartIndex - delimeterLen) == (etagsStartIndex - delimeterLen))
						etagsStartIndex -= delimeterLen;

					commentValue = commentValue.left(etagsStartIndex);
				}

				return commentValue.replace(" \n", "\n");
			}
			else if((tagName == "Exif.Image.XPTitle") || (tagName == "Exif.Image.XPAuthor") || (tagName == "Exif.Image.XPKeywords") || (tagName == "Exif.Image.XPSubject"))
			{
				// special care for XP* tags - they are stored in UTF-8
				return ExifUtfToQString(tagValue);
			}
			else
			{
				// special care for multivalue tag
				if(tagFlags.testFlag(ExifItem::Multi))
				{
					QVariantList valueList;

					// Read all components
					for(int i = 0; i < tagValue.count(); i++)
					{
						valueList << getTagValueFromExif(type, tagValue, i);
					}

					return valueList;
				}
				else
				{
					return getTagValueFromExif(type, tagValue);
				}
			}
		}
		else if(tagType == "Iptc")
		{
			// IPTC tags

			Exiv2::IptcKey iptcKey(tagName.toStdString());

			Exiv2::IptcData::iterator pos = iptcData.findKey(iptcKey);

			if(pos == iptcData.end())
			{
				continue;
			}

			int tagId = iptcKey.tag();

			const Exiv2::Value& tagValue = pos->value();

			srcTagType = (int)tagValue.typeId();

			// special care for multivalue tag
			if(tagFlags.testFlag(ExifItem::Multi))
			{
				QVariantList valueList;

				// Iptc metadata is sorted by metadata key, so all tags should be following each other
				Exiv2::IptcData::const_iterator i = pos;
				while((i != iptcData.end()) && (i->tag() == tagId))
				{
					valueList << getTagValueFromExif(type, i->value());
					i++;
				}

				return valueList;
			}
			else
			{
				return getTagValueFromExif(type, tagValue);
			}
		}
		else if(tagType == "Xmp")
		{
			// XMP tags

			Exiv2::XmpData::iterator pos = xmpData.findKey(Exiv2::XmpKey(tagName.toStdString()));

			if(pos == xmpData.end())
			{
				continue;
			}

			const Exiv2::Value& tagValue = pos->value();

			Exiv2::TypeId typId = tagValue.typeId();

			// for multi-values from AnalogExif and user-defined namespaces use XMP seq type, since order is set when editing
			if(tagFlags.testFlag(ExifItem::Multi) &&
				((pos->groupName() == "AnalogExif") || (pos->groupName() == settings.value("UserNsPrefix", "").toString().toStdString())))
			{
				typId = Exiv2::xmpSeq;
			}

			srcTagType = (int)typId;

			switch(typId)
			{
			case Exiv2::xmpText:
				{
					std::string str = tagValue.toString();
					// do not convert real numbers to Exif fraction format
					return ExifItem::valueFromString(QString::fromUtf8(str.data(), str.length()), type, false, false);
				}
				break;
			case Exiv2::langAlt:	// LangAlt gets the first value only
				{
					std::string str = tagValue.toString();
					// do not convert real numbers to Exif fraction format
					return ExifItem::valueFromString(QString::fromUtf8(str.data(), str.length()).remove(QRegExp("^lang=(\\\")?.*(\\\")? ")), type, false, false);
				}
				break;
			case Exiv2::xmpAlt:
			case Exiv2::xmpBag:
			case Exiv2::xmpSeq:
				// XMP bag, seq and alt are supported only for multi-value tags
				if(tagFlags.testFlag(ExifItem::Multi))
				{
					QVariantList valueList;

					for(int i = 0; i < pos->count(); i++)
					{
						std::string str = tagValue.toString(i);
						// do not convert real numbers to Exif fraction format
						valueList << ExifItem::valueFromString(QString::fromUtf8(str.data(), str.length()), type, false);
					}

					return valueList;
				}
				break;
			default:
				return QVariant();
				break;
			}
		}
	}

	return QVariant();
}

// process passed tag with the given Exiv2 containers
void ExifTreeModel::processTag(ExifItem* tag, Exiv2::ExifData& exifData, Exiv2::IptcData& iptcData, Exiv2::XmpData& xmpData)
{
	// special care for GPS tag
	if(tag->tagType() == ExifItem::TagGPS)
	{
		// try to get GPS position from EXIF
		QString gpsPosition = getGPSfromExif();

		// if failed, try with XMP
		if(gpsPosition == "")
			gpsPosition = getGPSfromXmp();

		// no GPS data found - clear the tag
		if(gpsPosition == "")
		{
			tag->setChecked(false);
			tag->setValue(QVariant());
		}
		else
		{
			tag->setChecked(true);
			tag->setValue(gpsPosition);
		}

		return;
	}

	int srcTagType;

	// get tag value
	QVariant tagValue = readTagValue(tag->tagName(), srcTagType, tag->tagType(), tag->tagFlags(), exifData, iptcData, xmpData);
	tag->setSrcTagType(srcTagType);

	// get alt tag value
	if(tag->tagFlags().testFlag(ExifItem::AsciiAlt))
	{
		// get alt value
		QVariant altTagValue = readTagValue(tag->tagAltName(), srcTagType, tag->tagType(), tag->tagFlags() & ~ExifItem::AsciiAlt, exifData, iptcData, xmpData);

		// if alt value exists
		if(tagValue == QVariant())
		{
			if(altTagValue != QVariant())
				tagValue = altTagValue;
		}

		if(tagValue != QVariant())
		{
			QVariantList listValue;
			listValue << tagValue << altTagValue;
			tagValue = listValue;
		}
	}

	// check only existing values
	if(tagValue != QVariant())
	{
		tag->setChecked(true);
	}
	else
	{
		tag->setChecked(false);
	}

	tag->setValue(tagValue);
}

bool ExifTreeModel::readMetaValues(Exiv2::Image::AutoPtr& exivHandle)
{
	// check for empty image
	if(!exivHandle.get())
		return true;

	// re-read metadata
	curExifData = exivHandle->exifData();
	curIptcData = exivHandle->iptcData();
	curXmpData = exivHandle->xmpData();

	// sort values
	// curExifData.sortByTag();
	curIptcData.sortByTag();
	// curXmpData.sortByKey();

	// browse through all categories
	for(int i = 0; i < rootItem->childCount(); i++)
	{
		ExifItem* category = rootItem->child(i);
		for(int j = 0; j < category->childCount(); j++)
		{
			ExifItem* tag = category->child(j);

			try
			{
				processTag(tag, curExifData, curIptcData, curXmpData);
			}
			catch(Exiv2::AnyError& err)
			{
				qDebug("AnalogExif: ExifTreeModel::readMetaValues() Exiv2 exception (%d) = %s", err.code(), err.what());
				return false;
			}
		}
	}

	return true;
}

bool ExifTreeModel::reload()
 {
	 clear();

	 return readMetaValues();
 }

void ExifTreeModel::setValues(QVariantList& values)
{
	if(values.count() < 2)
		return;

	for(int i = 0; i < values.count(); i += 2)
	{
		if(rootItem->findSetTagValueFromString(values.at(i).toString(), values.at(i+1), true))
			emit dataChanged(QModelIndex(), QModelIndex());
	}
}

void ExifTreeModel::repopulate()
{
	rootItem->removeChildren();
	reset();
	populateModel();
	// reload information if file was open
	/*if(editable)
		reload();*/
}

bool ExifTreeModel::parseGPSString(QString gpsStr, QString& latRef, int& latDeg, int& latMin, double& latSec, QString& lonRef, int& lonDeg, int& lonMin, double& lonSec)
{
	QRegExp regEx("(\\+|\\-)?(\\d{1,2})\u00B0\\s*(\\d{1,2})'\\s*(\\d{1,2}(?:\\.\\d{1,3})?)\" (\\+|\\-)?(\\d{1,3})\u00B0\\s*(\\d{1,2})'\\s*(\\d{1,2}(?:\\.\\d{1,3})?)\"");

	QStringList caps;

	if(regEx.indexIn(gpsStr) != -1)
	{
		caps = regEx.capturedTexts();

		if(caps.size() == 9)
		{
			// parse latitude reference
			if(regEx.cap(1) == "+")
				latRef = "N";
			else
				latRef = "S";

			// parse latitude
			latDeg = regEx.cap(2).toInt();
			latMin = regEx.cap(3).toInt();
			latSec = regEx.cap(4).toDouble();

			// parse longitude reference
			if(regEx.cap(5) == "+")
				lonRef = "E";
			else
				lonRef = "W";

			// parse latitude
			lonDeg = regEx.cap(6).toInt();
			lonMin = regEx.cap(7).toInt();
			lonSec = regEx.cap(8).toDouble();

			return true;
		}
	}

	return false;
}

bool ExifTreeModel::parseGPSString(QString gpsStr, QString& latRef, int& latDeg, double& latMin, QString& lonRef, int& lonDeg, double& lonMin)
{
	int latMinInt, lonMinInt;
	double latSec, lonSec;

	if(!parseGPSString(gpsStr, latRef, latDeg, latMinInt, latSec, lonRef, lonDeg, lonMinInt, lonSec))
		return false;

	latMin = (double)latMinInt + latSec / 60.0;
	lonMin = (double)lonMinInt + lonSec / 60.0;

	return true;
}

bool ExifTreeModel::storeGPSInExif(QString gpsStr, Exiv2::ExifData& exifData)
{
	if(gpsStr == "")
		return false;

	QString latRef;
	int latDeg, latMin;
	double latSec;

	QString lonRef;
	int lonDeg, lonMin;
	double lonSec;

	if(!parseGPSString(gpsStr, latRef, latDeg, latMin, latSec, lonRef, lonDeg, lonMin, lonSec))
		return false;

	try
	{
		exifData["Exif.GPSInfo.GPSLatitudeRef"] = latRef.toStdString();

		Exiv2::RationalValue::AutoPtr rv(new Exiv2::RationalValue);
		rv->value_.push_back(std::make_pair(latDeg,1));
		rv->value_.push_back(std::make_pair(latMin,1));

		rv->value_.push_back(std::make_pair(latSec*1000,1000));

		exifData["Exif.GPSInfo.GPSLatitude"] = *rv;

		exifData["Exif.GPSInfo.GPSLongitudeRef"] = lonRef.toStdString();

		Exiv2::RationalValue::AutoPtr rv1(new Exiv2::RationalValue);
		rv1->value_.push_back(std::make_pair(lonDeg,1));
		rv1->value_.push_back(std::make_pair(lonMin,1));

		rv1->value_.push_back(std::make_pair(lonSec*1000,1000));

		exifData["Exif.GPSInfo.GPSLongitude"] = *rv1;
	}
	catch(Exiv2::AnyError& err)
	{
		qDebug("AnalogExif: ExifTreeModel::storeGPSInExif() Exiv2 exception (%d) = %s", err.code(), err.what());
		return false;
	}

	return true;
}

bool ExifTreeModel::storeGPSInXmp(QString gpsStr, Exiv2::XmpData& xmpData)
{
	if(gpsStr == "")
		return false;

	QString latRef;
	int latDeg;
	double latMin;

	QString lonRef;
	int lonDeg;
	double lonMin;

	if(!parseGPSString(gpsStr, latRef, latDeg, latMin, lonRef, lonDeg, lonMin))
		return false;

	try
	{
		// TODO: mins precision 4 is out of spec, but ok for ExifTool
		QString latStr = QString("%1,%2%3").arg(latDeg, 3, 10, QChar('0')).arg(latMin, 2, 'f', 4, QChar('0')).arg(latRef);
		xmpData["Xmp.exif.GPSLatitude"] = latStr.toStdString();

		QString lonStr = QString("%1,%2%3").arg(lonDeg, 3, 10, QChar('0')).arg(lonMin, 2, 'f', 4, QChar('0')).arg(lonRef);
		xmpData["Xmp.exif.GPSLongitude"] = lonStr.toStdString();
	}
	catch(Exiv2::AnyError& err)
	{
		qDebug("AnalogExif: ExifTreeModel::storeGPSInXmp() Exiv2 exception (%d) = %s", err.code(), err.what());
		return false;
	}

	return true;
}

void ExifTreeModel::tagValueToMetadata(QVariant value, ExifItem::TagType tagType, Exiv2::Value& v)
{
	switch(v.typeId())
	{
	case Exiv2::asciiString:
	case Exiv2::string:
	case Exiv2::comment:
	case Exiv2::date:
	case Exiv2::time:
		{
			// store as Local 8bit
			QByteArray localStr = ExifItem::valueToString(value, tagType).toLocal8Bit();

			v.read((Exiv2::byte*)localStr.data(), localStr.length(), Exiv2::littleEndian);
		}
		break;
	default:
		v.read(ExifItem::valueToString(value, tagType).toStdString());
		break;
	}
}

void ExifTreeModel::writeTagValue(QString& tagNames, const QVariant& tagValue, ExifItem::TagType type, ExifItem::TagFlags tagFlags, Exiv2::ExifData& exifData, Exiv2::IptcData& iptcData, Exiv2::XmpData& xmpData)
{
	QStringList tags = tagNames.remove(QChar(' ')).split(",", QString::SkipEmptyParts);

	foreach(QString tagName, tags)
	{
		QString tagType = tagName.split(".").at(0);
		if(tagType == "Exif")
		{
			// Exif data

			Exiv2::ExifKey exifKey(tagName.toStdString());

			// erase tag if it is empty
			if(tagValue != QVariant())
			{
				Exiv2::Value::AutoPtr v;

				// set tag data according to its Exiv2 type
				Exiv2::TypeId typId = Exiv2::ExifTags::tagType(exifKey.tag(), exifKey.ifdId());

				// special care for multivalue tag
				if(tagFlags.testFlag(ExifItem::Multi))
				{
					v = Exiv2::Value::create(typId);
					QVariantList valueList = tagValue.toList();

					// add each value to the tag
					foreach(QVariant value, valueList)
					{
						tagValueToMetadata(value, type, *v);
					}
				}
				else
				{
					// special care for comments
					if(tagName == "Exif.Photo.UserComment")
					{
						// UTF-16, add charset marker
						v = QStringToExifUtf(ExifItem::valueToString(tagValue, type).replace('\n', " \n"), true, false, typId);
					}
					else if(tagName == "Exif.Image.XPComment")
					{
						// UTF-16, no charset markers
						v = QStringToExifUtf(ExifItem::valueToString(tagValue, type).replace('\n', " \n"));
					}
					else if((tagName == "Exif.Image.XPTitle") || (tagName == "Exif.Image.XPAuthor") || (tagName == "Exif.Image.XPKeywords") || (tagName == "Exif.Image.XPSubject"))
					{
						// UTF-16, no charset markers
						v = QStringToExifUtf(ExifItem::valueToString(tagValue, type));
					}
					else
					{
						v = Exiv2::Value::create(typId);
						tagValueToMetadata(tagValue, type, *v);
					}
				}

				exifData[tagName.toStdString()] = *v;
				v.reset();
			}

		}
		else if(tagType == "Iptc")
		{
			// IPTC tags
			Exiv2::IptcKey iptcKey(tagName.toStdString());

			// erase tag if it is empty
			if(tagValue != QVariant())
			{
				// set tag data according to its Exiv2 type
				Exiv2::TypeId typId = Exiv2::IptcDataSets::dataSetType(iptcKey.tag(), iptcKey.record());

				// special care for multivalue tag
				if(tagFlags.testFlag(ExifItem::Multi))
				{
					// erase all existing tags
					Exiv2::IptcData::iterator i = iptcData.findKey(iptcKey);
					while(i != iptcData.end())
					{
						iptcData.erase(i);
						i = iptcData.findKey(iptcKey);
					}

					QVariantList valueList = tagValue.toList();

					// store each value as separate tag
					foreach(QVariant value, valueList)
					{
						Exiv2::Value::AutoPtr v = Exiv2::Value::create(typId);
						tagValueToMetadata(value, type, *v);
						iptcData.add(iptcKey, v.get());
						v.reset();
					}
				}
				else
				{
					Exiv2::Value::AutoPtr v = Exiv2::Value::create(typId);
					tagValueToMetadata(tagValue, type, *v);
					iptcData[tagName.toStdString()] = *v;
					v.reset();
				}
			}
		}
		else if(tagType == "Xmp")
		{
			// XMP tags

			Exiv2::XmpKey xmpKey(tagName.toStdString());

			// erase tag if it is empty
			if(tagValue != QVariant())
			{
				Exiv2::TypeId typId = Exiv2::XmpProperties::propertyType(xmpKey);

				// for multi-values from AnalogExif and user-defined namespaces use XMP seq type, since order is set when editing
				if(tagFlags.testFlag(ExifItem::Multi) &&
					((xmpKey.groupName() == "AnalogExif") || (xmpKey.groupName() == settings.value("UserNsPrefix", "").toString().toStdString())))
				{
					typId = Exiv2::xmpSeq;
				}

				Exiv2::Value::AutoPtr v;
				
				switch(typId)
				{
				case Exiv2::xmpText:
				case Exiv2::langAlt:	// LangAlt gets the first value only
					v = QStringToExifUtf(ExifItem::valueToString(tagValue, type, QVariant(), false), false, true, typId);
					break;
				case Exiv2::xmpAlt:
				case Exiv2::xmpBag:
				case Exiv2::xmpSeq:
					// XMP bag, seq and alt are supported only for multi-value tags
					if(tagFlags.testFlag(ExifItem::Multi))
					{
						v = Exiv2::Value::create(typId);

						foreach(QVariant val, tagValue.toList())
						{
							QStringToExifUtf(*v, ExifItem::valueToString(val, type, QVariant(), false), false, true, typId);
						}
					}
					else
					{
						v = QStringToExifUtf(ExifItem::valueToString(tagValue, type, QVariant(), false), false, true, typId);
					}
					break;
				default:
					break;
				}

				if(v.get())
					xmpData[tagName.toStdString()] = *v;

				v.reset();
			}
		}
	}
}

bool ExifTreeModel::storeTag(ExifItem* tag, Exiv2::ExifData& exifData, Exiv2::IptcData& iptcData, Exiv2::XmpData& xmpData)
{
	// special care for GPS tag
	if(tag->tagType() == ExifItem::TagGPS)
	{
		if(!storeGPSInExif(tag->value().toString(), exifData))
			return false;

		// TODO: Optional store in Xmp?
		if(!storeGPSInXmp(tag->value().toString(), xmpData))
			return false;

	}
	else
	{
		// for alt tags store value in different set of tags
		if(tag->tagFlags().testFlag(ExifItem::AsciiAlt))
		{
			if(tag->value() == QVariant())
			{
				// erase both sets of values
				writeTagValue(tag->tagName(), QVariant(), tag->tagType(), tag->tagFlags(), exifData, iptcData, xmpData);
				writeTagValue(tag->tagAltName(), QVariant(), tag->tagType(), tag->tagFlags(), exifData, iptcData, xmpData);
			}
			else
			{
				// extract value, first item - original value, second item - alt value
				QVariantList tagValue = tag->value().toList();
				if(tagValue.isEmpty() || (tagValue.count() < 2))
					return false;

				writeTagValue(tag->tagName(), tagValue.at(0), tag->tagType(), tag->tagFlags(), exifData, iptcData, xmpData);
				writeTagValue(tag->tagAltName(), tagValue.at(1), tag->tagType(), tag->tagFlags(), exifData, iptcData, xmpData);
			}
		}
		else
		{
			writeTagValue(tag->tagName(), tag->value(), tag->tagType(), tag->tagFlags(), exifData, iptcData, xmpData);
		}
	}

	return true;
}

bool ExifTreeModel::prepareMetadata()
{
	// clear Exiv2 containers so they contain only updated tags
	curExifData.clear();
	curXmpData.clear();
	curIptcData.clear();

	return prepareMetadata(curExifData, curIptcData, curXmpData);
}

bool ExifTreeModel::prepareMetadata(Exiv2::ExifData& exifData, Exiv2::IptcData& iptcData, Exiv2::XmpData& xmpData)
{
	etagsString = "";

	int etagsStorageOptions = settings.value("ExtraTagsStorage", 0x03).toInt();

	// browse through all categories and fill Exiv2 structures
	for(int i = 0; i < rootItem->childCount(); i++)
	{
		ExifItem* category = rootItem->child(i);
		for(int j = 0; j < category->childCount(); j++)
		{
			ExifItem* tag = category->child(j);

			if(tag->isDirty())
			{
				try
				{
					if(!storeTag(tag, exifData, iptcData, xmpData))
						return false;
				}
				catch(Exiv2::AnyError& err)
				{
					qDebug("AnalogExif: ExifTreeModel::prepareMetadata() Exiv2 exception (%d) = %s", err.code(), err.what());
					return false;
				}
			}
			// store extra tags in comments, if required
			if((tag->value() != QVariant()) && (etagsStorageOptions != 0 ) && tag->tagFlags().testFlag(ExifItem::Extra))
			{
				etagsString += "\t" + tag->tagText() + ": " + getItemData(tag->value(), tag->format(), tag->tagFlags(), (ExifItem::TagType)tag->tagType()).toString() + ". \n";
			}
		}
	}

	return true;
}

// convert QString to UTF-16 Exif byte string
Exiv2::Value::AutoPtr ExifTreeModel::QStringToExifUtf(QString qstr, bool addUnicodeMarker, bool isUtf8, Exiv2::TypeId typeId)
{
	// create as Exif byte value
	Exiv2::Value::AutoPtr v;
	
	v = Exiv2::Value::create(typeId);

	QStringToExifUtf(*v, qstr, addUnicodeMarker, isUtf8, typeId);

	return v;
}

void ExifTreeModel::QStringToExifUtf(Exiv2::Value& v, QString qstr, bool addUnicodeMarker, bool isUtf8, Exiv2::TypeId)
{
	// get UTF-16 byte string
	QByteArray utfData;
	
	if(isUtf8)
	{
		utfData = qstr.toUtf8();
	}
	else
	{
		utfData = QByteArray::fromRawData((char*)qstr.utf16(), qstr.length() * 2);
	}

	if(addUnicodeMarker)
	{
		// prepend with Exif charset marker
		utfData.prepend('\0');
		utfData.prepend("UNICODE");
	}

	// append with end-of-string null
	// utfData.append(QByteArray(2, '\0'));

	// read the value into Exiv2::Value
	v.read((Exiv2::byte*)utfData.data(), utfData.length(), Exiv2::littleEndian);
}

// convert Exif UTF-16 byte string to QString
QString ExifTreeModel::ExifUtfToQString(const Exiv2::Value& exifData, bool checkForMarker, bool isUtf8)
{
	int strSize = exifData.size();
	if(strSize == 0)
		return "";

	unsigned char* data = new unsigned char[strSize];

	exifData.copy(data, Exiv2::littleEndian);

	QString qstr;

	if(checkForMarker)
	{
		if(strcmp((char*)data, "UNICODE") == 0)
			qstr = QString::fromUtf16((const ushort*)(data + 8), (strSize - 8) / 2);
		else 
			qstr = QString::fromAscii((char*)(data + 8), strSize - 8);
	}
	else
	{
		if(isUtf8)
		{
			qstr = QString::fromUtf8((char*)data, strSize);
		}
		else
		{
			qstr = QString::fromUtf16((const ushort*)data, strSize / 2);
		}
	}

	delete data;

	return qstr;
}

// store extra tags string in the given Exiv2 ExifData, can throw Exiv2 exceptions
void ExifTreeModel::storeEtags(Exiv2::ExifData& exifData)
{
	int etagsStorageOptions = settings.value("ExtraTagsStorage", 0x03).toInt();

	// store extra tags in comment fields
	if(etagsString != "")
	{
		if(etagsStorageOptions & 0x01)
		{
			// store in Exif.Photo.UserComment
			Exiv2::ExifKey exifKey("Exif.Photo.UserComment");

			QString commentValue = "";

			Exiv2::ExifData::iterator pos = exifData.findKey(exifKey);
			if(pos != exifData.end())
			{
				commentValue = ExifUtfToQString(pos->value(), true);

				if(commentValue != "")
					commentValue += QString(ETAGS_DELIMETER_IN_COMMENTS);

				// strip etags if stored
				int etagsStartIndex = commentValue.indexOf(ETAGS_START_MARKER_IN_COMMENTS);

				if(etagsStartIndex != -1)
					commentValue = commentValue.left(etagsStartIndex);
			}

			commentValue += (QString(ETAGS_START_MARKER_IN_COMMENTS) + etagsString);

			exifData["Exif.Photo.UserComment"] = *QStringToExifUtf(commentValue, true, false, Exiv2::comment);
		}
		if(etagsStorageOptions & 0x02)
		{
			// store in Exif.Image.XPComment
			Exiv2::ExifKey exifKey("Exif.Image.XPComment");

			QString commentValue = "";

			Exiv2::ExifData::iterator pos = exifData.findKey(exifKey);
			if(pos != exifData.end())
			{
				commentValue = ExifUtfToQString(pos->value());

				if(commentValue != "")
					commentValue += QString(ETAGS_DELIMETER_IN_COMMENTS);

				// strip etags if stored
				int etagsStartIndex = commentValue.indexOf(ETAGS_START_MARKER_IN_COMMENTS);

				if(etagsStartIndex != -1)
					commentValue = commentValue.left(etagsStartIndex);
			}

			commentValue += (QString(ETAGS_START_MARKER_IN_COMMENTS) + etagsString);

			exifData["Exif.Image.XPComment"] = *QStringToExifUtf(commentValue);
		}
	}
}

bool ExifTreeModel::saveFile(QString filename, bool overwrite)
{
	try
	{

#ifdef Q_WS_WIN
		// unicode paths are supported only in Windows verison
	    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename.toStdWString());
#else
		// use UTF-8
		Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename.toUtf8().data());
#endif

	    if((image.get() == 0) || (!image->good()))
			return false;

		// replace image metadata
		if(overwrite)
		{
			storeEtags(curExifData);
		    image->setExifData(curExifData);
			image->setIptcData(curIptcData);
			image->setXmpData(curXmpData);
		}
		else
		{
			// read meta data
			image->readMetadata();

			Exiv2::ExifData& imgExifData = image->exifData();
			Exiv2::IptcData& imgIptcData = image->iptcData();
			Exiv2::XmpData& imgXmpData = image->xmpData();

			// sort
			// imgExifData.sortByTag();
			imgIptcData.sortByTag();
			// imgXmpData.sortByKey();

			if(!prepareEtagsAndErase(imgExifData, imgIptcData, imgXmpData))
				return false;

			// add/replace Exif data
			Exiv2::ExifData::const_iterator exifEnd = curExifData.end();
			for(Exiv2::ExifData::const_iterator i = curExifData.begin(); i != exifEnd; ++i)
			{
				imgExifData.add(*i);
			}

			// add/replace Iptc data
			Exiv2::IptcData::const_iterator iptcEnd = curIptcData.end();
			for(Exiv2::IptcData::const_iterator i = curIptcData.begin(); i != iptcEnd; ++i)
			{
				imgIptcData.add(*i);
			}

			// add/replace Xmp data
			Exiv2::XmpData::const_iterator xmpEnd = curXmpData.end();
			for(Exiv2::XmpData::const_iterator i = curXmpData.begin(); i != xmpEnd; ++i)
			{
				imgXmpData[i->key()] = *i;
			}

			storeEtags(imgExifData);
		}

	    image->writeMetadata();

		delete image.release();
	}
	catch(Exiv2::AnyError& err)
	{
		qDebug("AnalogExif: ExifTreeModel::saveFile(%s) Exiv2 exception (%d) = %s", filename.toStdString().c_str(), err.code(), err.what());
		return false;
	}

	return true;
}

QByteArray* ExifTreeModel::getPreview() const
{
	if(!exifHandle->good())
		return NULL;

	// load preview
	Exiv2::PreviewManager preview(*exifHandle);

	Exiv2::PreviewPropertiesList previews = preview.getPreviewProperties();

	if(previews.empty())
		return NULL;

	// choose the largest required preview
	return new QByteArray((const char*)preview.getPreviewImage(previews.back()).pData(), previews.back().size_);
}

// clears dirty flag from all tags
void ExifTreeModel::resetDirty()
{
	for(int i = 0; i < rootItem->childCount(); i++)
	{
		ExifItem* category = rootItem->child(i);
		for(int j = 0; j < category->childCount(); j++)
		{
			ExifItem* tag = category->child(j);
			tag->resetDirty();
		}
	}
}

static QStringList notSupportedTags;

void ExifTreeModel::fillNotSupportedTags()
{
	QFile file(":/not-supported.txt");

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	QTextStream in(&file);
	while (!in.atEnd()) {
		notSupportedTags << in.readLine();
	}
}

bool ExifTreeModel::tagSupported(const QString tagName)
{
	QSettings settings;
	QStringList strList = tagName.split(QChar('.'), QString::SkipEmptyParts);

	if(strList.count() != 3)
		return false;

	try {
		// determine the metadata type
		if(strList.at(0) == "Exif")
		{
			// create EXIF key
			Exiv2::ExifKey exifKey(tagName.toStdString());
		}
		else if(strList.at(0) == "Iptc")
		{
			Exiv2::IptcKey iptcKey(tagName.toStdString());
		}
		else if(strList.at(0) == "Xmp")
		{
			Exiv2::XmpKey xmpKey(tagName.toStdString());

			// do not verify AnalogExif and custom user ns
			if((strList.at(1) != "AnalogExif") && (strList.at(1) != customUserNs))
			{
				if(!Exiv2::XmpProperties::propertyInfo(xmpKey))
					return false;
			}
		}
	}
	catch (Exiv2::Error&)
	{
		// Exiv2 exception - no tag exists
		return false;
	}

	// if no exception was thrown so far - tag exists

	// check in non-supported taglist
	if(notSupportedTags.contains(tagName))
		return false;

	// verify AnalogExif namespace
	return true;
}

void ExifTreeModel::prepareEtags()
{
	int etagsStorageOptions = settings.value("ExtraTagsStorage", 0x03).toInt();

	if(etagsStorageOptions)
	{
		etagsString = "";

		// browse through all categories and fill Exiv2 structures
		for(int i = 0; i < rootItem->childCount(); i++)
		{
			ExifItem* category = rootItem->child(i);
			for(int j = 0; j < category->childCount(); j++)
			{
				ExifItem* tag = category->child(j);

				// store extra tags in comments, if required
				if((tag->value() != QVariant()) && tag->tagFlags().testFlag(ExifItem::Extra))
				{
					// add extra tag value
					etagsString += "\t" + tag->tagText() + ": " + getItemData(tag->value(), tag->format(), tag->tagFlags(), tag->tagType()).toString() + ". \n";
				}
			}
		}
	}
}

void ExifTreeModel::eraseTag(ExifItem* tag, Exiv2::ExifData& exifData, Exiv2::IptcData& iptcData, Exiv2::XmpData& xmpData)
{
	eraseTag(tag->tagName(), exifData, iptcData, xmpData);
	eraseTag(tag->tagAltName(), exifData, iptcData, xmpData);
}

void ExifTreeModel::eraseTag(QString& tagNames, Exiv2::ExifData& exifData, Exiv2::IptcData& iptcData, Exiv2::XmpData& xmpData)
{
	QStringList tags = tagNames.remove(QChar(' ')).split(",", QString::SkipEmptyParts);

	foreach(QString tagName, tags)
	{
		QString tagType = tagName.split(".").at(0);
		if(tagType == "Exif")
		{
			// Exif data

			Exiv2::ExifKey exifKey(tagName.toStdString());

			Exiv2::ExifData::iterator i = exifData.findKey(exifKey);
			while((i != exifData.end()) && (i->tag() == exifKey.tag()))
				i = exifData.erase(i);
		}
		else if(tagType == "Iptc")
		{
			Exiv2::IptcKey iptcKey(tagName.toStdString());

			Exiv2::IptcData::iterator i = iptcData.findKey(iptcKey);
			while((i != iptcData.end()) && (i->tag() == iptcKey.tag()))
				i = iptcData.erase(i);
		}
		else if(tagType == "Xmp")
		{
			Exiv2::XmpKey xmpKey(tagName.toStdString());

			Exiv2::XmpData::iterator pos = xmpData.findKey(xmpKey);
			if(pos != xmpData.end())
				xmpData.erase(pos);
		}
	}
}

bool ExifTreeModel::prepareEtagsAndErase(Exiv2::ExifData& exifData, Exiv2::IptcData& iptcData, Exiv2::XmpData& xmpData)
{
	int etagsStorageOptions = settings.value("ExtraTagsStorage", 0x03).toInt();

	if(etagsStorageOptions)
	{
		etagsString = "";
	}

	// browse through all categories and fill Exiv2 structures
	for(int i = 0; i < rootItem->childCount(); i++)
	{
		ExifItem* category = rootItem->child(i);
		for(int j = 0; j < category->childCount(); j++)
		{
			ExifItem* tag = category->child(j);

			try
			{
				// store extra tags in comments, if required
				if(etagsStorageOptions && tag->tagFlags().testFlag(ExifItem::Extra))
				{
					ExifItem bkpTag = *tag;

					if(!tag->isDirty())
					{
						// try to get etag value from the file
						processTag(&bkpTag, exifData, iptcData, xmpData);
					}
					// add extra tag value
					if(bkpTag.value() != QVariant())
						etagsString += "\t" + bkpTag.tagText() + ": " + getItemData(bkpTag.value(), bkpTag.format(), bkpTag.tagFlags(), bkpTag.tagType()).toString() + ". \n";
				}

				if(tag->isDirty())
				{
					// erase tags
					eraseTag(tag, exifData, iptcData, xmpData);
				}
			}
			catch (Exiv2::Error& err)
			{
				qDebug("AnalogExif: ExifTreeModel::prepareEtags() Exiv2 exception (%d) = %s", err.code(), err.what());
				return false;
			}
		}
	}

	return true;
}

bool ExifTreeModel::setExposureNumber(QString filename, int exposure)
{
	int etagsStorageOptions = settings.value("ExtraTagsStorage", 0x03).toInt();

	try
	{
#ifdef Q_WS_WIN
		// unicode paths are supported only in Windows verison
	    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename.toStdWString());
#else
		// use UTF-8
		Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename.toUtf8().data());
#endif

	    if((image.get() == 0) || (!image->good()))
			return false;

		// read meta data
		image->readMetadata();

		Exiv2::XmpData& imgXmpData = image->xmpData();

		imgXmpData["Xmp.AnalogExif.ExposureNumber"] = exposure;

		// if store etags in comments - read meta values
		if(etagsStorageOptions)
		{
			if(!readMetaValues(image))
				return false;

			prepareEtags();

			Exiv2::ExifData& imgExifData = image->exifData();

			storeEtags(imgExifData);
		}

		image->writeMetadata();

		delete image.release();
	}
	catch (Exiv2::Error& err)
	{
		qDebug("AnalogExif: ExifTreeModel::setExposureNumber(%s) Exiv2 exception (%d) = %s", filename.toStdString().c_str(), err.code(), err.what());
		return false;
	}

	return true;
}

bool ExifTreeModel::mergeMetadata(QString filename, QVariantList metadata)
{
	int etagsStorageOptions = settings.value("ExtraTagsStorage", 0x03).toInt();

	try
	{
#ifdef Q_WS_WIN
		// unicode paths are supported only in Windows verison
	    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename.toStdWString());
#else
		// use UTF-8
		Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename.toUtf8().data());
#endif

	    if((image.get() == 0) || (!image->good()))
			return false;

		// read meta data
		image->readMetadata();

		if(etagsStorageOptions)
		{
			if(!readMetaValues(image))
				return false;
		}

		foreach(QVariant value, metadata)
		{
			ExifItem* item = static_cast<ExifItem*>(qVariantValue<void*>(value));

			if(!item)
				return false;

			// merge values
			if(!rootItem->findSetTagValueFromTag(item, true))
				return false;
		}

		// read metadata
		Exiv2::ExifData& exifData = image->exifData();
		Exiv2::IptcData& iptcData = image->iptcData();
		Exiv2::XmpData& xmpData = image->xmpData();

		// sort
		// exifData.sortByTag();
		iptcData.sortByTag();
		// xmpData.sortByKey();

		if(!prepareMetadata(exifData, iptcData, xmpData))
			return false;

		// if store etags in comments - read meta values
		if(etagsStorageOptions)
		{
			storeEtags(image->exifData());
		}

		image->writeMetadata();

		delete image.release();
	}
	catch (Exiv2::Error& err)
	{
		qDebug("AnalogExif: ExifTreeModel::setExposureNumber(%s) Exiv2 exception (%d) = %s", filename.toStdString().c_str(), err.code(), err.what());
		return false;
	}

	return true;
}
