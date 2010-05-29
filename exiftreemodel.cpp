#include "exiftreemodel.h"
#include "exifutils.h"

#include <QSqlQuery>
#include <QStringList>
#include <QVariantList>
#include <QDateTime>
#include <QFont>
#include <QBrush>
#include <QMessageBox>

#include <cmath>

#define CUSTOM_XMP_NAMESPACE_URI		("http://www.c41bytes.com/analogexif/ns")
#define ETAGS_START_MARKER_IN_COMMENTS	(QT_TR_NOOP("Photo information: \n"))
#define ETAGS_DELIMETER_IN_COMMENTS		(" \n\n")

ExifTreeModel::ExifTreeModel(QSqlDatabase& db, QObject *parent) : QAbstractItemModel(parent), dataBase(db)
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

	// register custom user schema
	QString userNs = settings.value("UserNs", "").toString();

	if(userNs != "")
	{
		QString userNsPrefix = settings.value("UserNsPrefix", "").toString();

		try
		{
			Exiv2::XmpProperties::registerNs(userNs.toStdString(), userNsPrefix.toStdString());
		}
		catch(Exiv2::Error& exc)
		{
			QMessageBox::critical(NULL, tr("Error registering user-defined XMP schema"), tr("Unable to register user-defined XMP schema.\nSchema: %1, prefix: %2.\n\n%3.").arg(userNs).arg(userNsPrefix).arg(exc.what()), QMessageBox::Ok);
		}
	}

	// create and read the values of the used metatags
	populateModel();
	editable = false;
}

ExifTreeModel::~ExifTreeModel()
{
	delete rootItem;
}

// clear data
void ExifTreeModel::clear()
{
	reset();
	editable = false;
	rootItem->reset();

	curExifData.clear();
	curIptcData.clear();
	curXmpData.clear();
}

// open file and read metadata
bool ExifTreeModel::openFile(QString filename)
{
	// invalidate the model
	clear();

	try
	{
		// open file using Exiv2 library
#ifdef _WIN32
		// unicode paths supported only in windows version
		exifHandle = Exiv2::ImageFactory::open(filename.toStdWString());
#else
		exifHandle = Exiv2::ImageFactory::open(filename.toStdString());
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

	ExifItem* item = getItem(index);

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

	// return tag type
	if(role == GetTypeRole)
		return item->tagType();

	// return tag flags
	if(role == GetFlagsRole)
		return item->tagFlags();

	// return tag format/choice
	if(role == GetChoiceRole)
		return item->format();

	if(item->value() == QVariant())
		return QVariant();

	if(ExifItem::testFlag(item->tagFlags(), ExifItem::TagFlagChoice) && (role == Qt::DisplayRole))
	{
		// special care for choice tags
		return ExifItem::findChoiceTextByValue(item->format(), item->value(), item->tagType());
	}

	// return value according to the tag type
	switch(item->tagType())
	{
	case ExifItem::TagString:
	case ExifItem::TagText:
	case ExifItem::TagGPS:
		if(role == Qt::DisplayRole)
			return QString(item->format()).arg(item->value().toString()).replace(QRegExp("(\r|\n)"), " ");
		else if(role == Qt::EditRole)
			return item->value();
		break;
	case ExifItem::TagInteger:
	case ExifItem::TagUInteger:
	case ExifItem::TagISO:
		if(role == Qt::DisplayRole)
			return QString(item->format()).arg(item->value().toInt());
		else if(role == Qt::EditRole)
			return item->value();
		break;
	case ExifItem::TagRational:
	case ExifItem::TagURational:
	case ExifItem::TagAperture:
	case ExifItem::TagApertureAPEX:
		{
			double value = item->value().toDouble();

			// adjust APEX values
			if(item->tagType() == ExifItem::TagApertureAPEX)
				value = exp(value*log(2.0)*0.5);

			if(role == Qt::DisplayRole)
				return QString(item->format()).arg(value, 0, 'f', 1);
			else if(role == Qt::EditRole)
				return value;
			break;
		}
	case ExifItem::TagFraction:
	case ExifItem::TagShutter:
		{
			QVariantList rational = item->value().toList();
			double value = (double)rational.at(0).toInt() / (double)rational.at(1).toInt();

			if(role == Qt::DisplayRole)
			{
				if((value >= 0.5) && (item->tagType() == ExifItem::TagShutter))
				{
					return QString(item->format()).arg(value);
				}
				else
				{
					return QString(item->format()).arg(QString("%1/%2").arg(rational.at(0).toInt()).arg(rational.at(1).toInt()));
				}
			}
			else if(role == Qt::EditRole)
				return rational;

			break;
		}
	case ExifItem::TagDateTime:
		{
			return QDateTime::fromString(item->value().toString(), "yyyy:MM:dd HH:mm:ss");
		}
		break;
	default:
		break;
	}

	// just return item value
	return item->value();
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
		// return value according to the tag type
		switch(item->tagType())
		{
		case ExifItem::TagString:
		case ExifItem::TagText:
		case ExifItem::TagGPS:
			item->setValue(value.toString(), true);
			break;
		case ExifItem::TagInteger:
		case ExifItem::TagUInteger:
		case ExifItem::TagISO:
			item->setValue(value.toInt(), true);
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

				if(item->value() != QVariant())
				{
					// special check whether value is changed for rational numbers
					double oldValue = item->value().toDouble();

					// compare APEX values up to the second digit after the decimal point
					if(item->tagType() == ExifItem::TagApertureAPEX)
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
				item->setValue(val, true);
				break;
			}
		case ExifItem::TagFraction:
		case ExifItem::TagShutter:
			item->setValue(value.toList(), true);
			break;
		case ExifItem::TagDateTime:
			item->setValue(value.toDateTime().toString("yyyy:MM:dd HH:mm:ss"));
			break;
		default:
			return false;
			break;
		}
	}

	emit dataChanged(index, index);

	return true;
}

// populate model
void ExifTreeModel::populateModel()
{
	// connect to internal database
	QSqlQuery query("SELECT a.GearType, b.TagName, b.TagText, b.PrintFormat, b.TagType, b.Flags FROM GearTemplate a, MetaTags b WHERE b.id=a.TagId ORDER BY a.GearType, a.OrderBy");

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
		headerItem->insertChild(query.value(1).toString(), query.value(2).toString(), QVariant(), query.value(3).toString(), query.value(4).toInt(), query.value(5).toInt());
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

			gpsPosition += QString("%1° %2' %3\" ").arg(latStrs.at(0)).arg(latStrs.at(1)).arg(latStrs.at(2).left(latStrs.at(2).length() - 1));
		}
		else if(latStrs.count() == 2)
		{
			if(latStrs.at(1).at(latStrs.at(1).length() - 1) == 'N')
				gpsPosition = "+";
			else
				gpsPosition = "-";

			double minVal = latStrs.at(1).left(latStrs.at(1).length() - 1).toDouble();

			double secVal = modf(minVal, NULL);

			gpsPosition += QString("%1° %2' %3\" ").arg(latStrs.at(0)).arg((int)minVal, 2, 10, QChar('0')).arg(secVal, 2, 'f', 3, QChar('0'));
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

				gpsPosition += QString("%1° %2' %3\"").arg(longStrs.at(0)).arg(longStrs.at(1)).arg(longStrs.at(2).left(longStrs.at(2).length() - 1));
			}
			else if(longStrs.count() == 2)
			{
				if(longStrs.at(1).at(longStrs.at(1).length() - 1) == 'E')
					gpsPosition += "+";
				else
					gpsPosition += "-";

				double minVal = longStrs.at(1).left(longStrs.at(1).length() - 1).toDouble();

				double secVal = modf(minVal, NULL);

				gpsPosition += QString("%1° %2' %3\" ").arg(longStrs.at(0)).arg((int)minVal, 2, 10, QChar('0')).arg(secVal, 2, 'f', 3, QChar('0'));
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

			gpsPosition += QString("%1° %2' %3\" ").arg(deg.first, 2, 10, QChar('0')).arg(min.first, 2, 10, QChar('0')).arg(secDouble, 2, 'f', 3, QChar('0'));

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

					gpsPosition += QString("%1° %2' %3\"").arg(deg.first, 3, 10, QChar('0')).arg(min.first, 2, 10, QChar('0')).arg(secDouble, 2, 'f', 3, QChar('0'));

					return gpsPosition;
				}
			}
		}
	}

	return "";
}

bool ExifTreeModel::readMetaValues()
{
	// re-read metadata
	curExifData = exifHandle->exifData();
	curIptcData = exifHandle->iptcData();
	curXmpData = exifHandle->xmpData();

	// browse through all categories
	for(int i = 0; i < rootItem->childCount(); i++)
	{
		ExifItem* category = rootItem->child(i);
		for(int j = 0; j < category->childCount(); j++)
		{
			ExifItem* tag = category->child(j);

			QStringList tags = tag->tagName().remove(QChar(' ')).split(",", QString::SkipEmptyParts);

			try
			{
				// special care for GPS tag
				if(tag->tagType() == ExifItem::TagGPS)
				{
					QString gpsPosition = getGPSfromExif();

					if(gpsPosition == "")
						gpsPosition = getGPSfromXmp();

					tag->setValue(gpsPosition);
					continue;
				}

				foreach(QString tagName, tags)
				{
					QString tagType = tagName.split(".").at(0);
					if(tagType == "Exif")
					{
						// Exif data
						
						// search for the key
						Exiv2::ExifData::iterator pos = curExifData.findKey(Exiv2::ExifKey(tagName.toStdString()));

						if(pos == curExifData.end())
							continue;

						Exiv2::Value::AutoPtr tagValue = pos->getValue();

						Exiv2::TypeId typId = tagValue->typeId();

						tag->setSrcTagType((int)typId);

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

							tag->setValueFromString(commentValue.replace(" \n", "\n"));
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

							tag->setValueFromString(commentValue.replace(" \n", "\n"));
						}
						else
						{
							switch(typId)
							{
							case Exiv2::asciiString:
							case Exiv2::comment:
								// Local 8bit instead of Unicode
								tag->setValueFromString(QString::fromLocal8Bit(tagValue->toString().c_str()));
								break;
							case Exiv2::unsignedByte:
							case Exiv2::unsignedShort:
							case Exiv2::unsignedLong:
							case Exiv2::signedByte:
							case Exiv2::signedShort:
							case Exiv2::signedLong:
							case Exiv2::undefined:
								tag->setValue(tagValue->toLong());
								break;
							case Exiv2::unsignedRational:
							case Exiv2::signedRational:
								{
									Exiv2::Rational val = tagValue->toRational();
									if((tag->tagType() == ExifItem::TagFraction) || (tag->tagType() == ExifItem::TagShutter))
									{
										QVariantList rational;
										rational << val.first << val.second;
										tag->setValue(rational);
									}
									else
									{
										tag->setValue(tagValue->toFloat());
									}
									break;
								}
							default:
								tag->setValue(QVariant());
								break;
							}
						}
					}
					else if(tagType == "Iptc")
					{
						// IPTC tags

						Exiv2::IptcData::iterator pos = curIptcData.findKey(Exiv2::IptcKey(tagName.toStdString()));

						if(pos == curIptcData.end())
							continue;

						Exiv2::Value::AutoPtr tagValue = pos->getValue();

						// TODO: not ignore Iptc tags

						// tag->setSrcTagType((int)tagValue->typeId());

						// tag->setValue(QVariant());
					}
					else if(tagType == "Xmp")
					{
						// XMP tags

						curXmpData = exifHandle->xmpData();

						Exiv2::XmpData::iterator pos = curXmpData.findKey(Exiv2::XmpKey(tagName.toStdString()));

						if(pos == curXmpData.end())
							continue;

						Exiv2::Value::AutoPtr tagValue = pos->getValue();

						tag->setSrcTagType((int)tagValue->typeId());

						if(tagValue->typeId() == Exiv2::xmpText)
						{
							tag->setValueFromString(ExifUtfToQString(pos->getValue(), false, true));
						}
						else
						{
							// TODO: handle XMP complex types
							tag->setValueFromString(QString::fromStdString(tagValue->toString()));
						}
					}

					// value found, continue with the next tag
					break;
				}
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

void ExifTreeModel::reload()
 {
	 clear();

	 readMetaValues();
 }

void ExifTreeModel::setValues(QVariantList& values)
{
	if(values.count() < 2)
		return;

	for(int i = 0; i < values.count(); i += 2)
	{
		if(rootItem->findSetTagValueFromString(values.takeAt(i).toString(), values.takeAt(i+1).toString(), true))
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
	QRegExp regEx("(\\+|\\-)?(\\d{1,2})°\\s*(\\d{1,2})'\\s*(\\d{1,2}(?:\\.\\d{1,3})?)\" (\\+|\\-)?(\\d{1,3})°\\s*(\\d{1,2})'\\s*(\\d{1,2}(?:\\.\\d{1,3})?)\"");

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

bool ExifTreeModel::storeGPSInExif(QString gpsStr)
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
		curExifData["Exif.GPSInfo.GPSLatitudeRef"] = latRef.toStdString();

		Exiv2::RationalValue::AutoPtr rv(new Exiv2::RationalValue);
		rv->value_.push_back(std::make_pair(latDeg,1));
		rv->value_.push_back(std::make_pair(latMin,1));

		rv->value_.push_back(std::make_pair(latSec*1000,1000));

		curExifData["Exif.GPSInfo.GPSLatitude"] = *rv;

		curExifData["Exif.GPSInfo.GPSLongitudeRef"] = lonRef.toStdString();

		Exiv2::RationalValue::AutoPtr rv1(new Exiv2::RationalValue);
		rv1->value_.push_back(std::make_pair(lonDeg,1));
		rv1->value_.push_back(std::make_pair(lonMin,1));

		rv1->value_.push_back(std::make_pair(lonSec*1000,1000));

		curExifData["Exif.GPSInfo.GPSLongitude"] = *rv1;
	}
	catch(Exiv2::AnyError& err)
	{
		qDebug("AnalogExif: ExifTreeModel::storeGPSInExif() Exiv2 exception (%d) = %s", err.code(), err.what());
		return false;
	}

	return true;
}

bool ExifTreeModel::storeGPSInXmp(QString gpsStr)
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
		curXmpData["Xmp.exif.GPSLatitude"] = latStr.toStdString();

		QString lonStr = QString("%1,%2%3").arg(lonDeg, 3, 10, QChar('0')).arg(lonMin, 2, 'f', 4, QChar('0')).arg(lonRef);
		curXmpData["Xmp.exif.GPSLongitude"] = lonStr.toStdString();
	}
	catch(Exiv2::AnyError& err)
	{
		qDebug("AnalogExif: ExifTreeModel::storeGPSInXmp() Exiv2 exception (%d) = %s", err.code(), err.what());
		return false;
	}

	return true;
}

bool ExifTreeModel::prepareTags()
{
	// clear Exiv2 containers so they contain only updated tags
	curExifData.clear();
	curXmpData.clear();
	curIptcData.clear();

	QString etagsString = "";
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
				// special care for GPS tag
				if(tag->tagType() == ExifItem::TagGPS)
				{
					if(!storeGPSInExif(tag->value().toString()))
						return false;

					// TODO: Optional store in Xmp?
					if(!storeGPSInXmp(tag->value().toString()))
						return false;

				}
				else
				{

					QStringList tags = tag->tagName().remove(QChar(' ')).split(",", QString::SkipEmptyParts);

					foreach(QString tagName, tags)
					{
						try
						{
							QString tagType = tagName.split(".").at(0);
							if(tagType == "Exif")
							{
								// Exif data

								Exiv2::ExifKey exifKey(tagName.toStdString());

								// erase tag if it is empty
								if(tag->value() == QVariant())
								{
									// find tag
									Exiv2::ExifData::iterator pos = curExifData.findKey(exifKey);
									if(pos != curExifData.end())
										curExifData.erase(pos);
								}
								else
								{
									// set tag data according to its Exiv2 type
									Exiv2::TypeId typId = Exiv2::ExifTags::tagType(exifKey.tag(), exifKey.ifdId());

									Exiv2::Value::AutoPtr v = Exiv2::Value::create(typId);

									// special care for comments
									if(tagName == "Exif.Photo.UserComment")
									{
										// UTF-16, add charset marker
										v = QStringToExifUtf(tag->getValueAsString().replace('\n', " \n"), true, false, typId);
									}
									else if(tagName == "Exif.Image.XPComment")
									{
										// UTF-16, no charset markers
										v = QStringToExifUtf(tag->getValueAsString().replace('\n', " \n"));
									}
									else if(typId == Exiv2::asciiString)
									{
										// store as Local 8bit
										v = Exiv2::Value::create(typId);
										QByteArray localStr = tag->getValueAsString().toLocal8Bit();

										v->read((Exiv2::byte*)localStr.data(), localStr.length(), Exiv2::littleEndian);
									}
									else
									{
										v = Exiv2::Value::create(typId);
										v->read(tag->getValueAsString().toStdString());
									}

									curExifData[tagName.toStdString()] = *v;
								}

							}
							else if(tagType == "Iptc")
							{
								// IPTC tags

								Exiv2::IptcKey iptcKey(tagName.toStdString());

								// erase tag if it is empty
								if(tag->value() == QVariant())
								{
									// find tag
									Exiv2::IptcData::iterator pos = curIptcData.findKey(iptcKey);
									if(pos != curIptcData.end())
										curIptcData.erase(pos);
								}
								else
								{
									// set tag data according to its Exiv2 type
									// curIptcData[tagName.toStdString()] = *v;
								}
							}
							else if(tagType == "Xmp")
							{
								// XMP tags

								Exiv2::XmpKey xmpKey(tagName.toStdString());

								// erase tag if it is empty
								if(tag->value() == QVariant())
								{
									// find tag
									Exiv2::XmpData::iterator pos = curXmpData.findKey(xmpKey);
									if(pos != curXmpData.end())
										curXmpData.erase(pos);
								}
								else
								{
									Exiv2::TypeId typId = Exiv2::XmpProperties::propertyType(xmpKey);

									Exiv2::Value::AutoPtr v;
									
									if(typId == Exiv2::xmpText)
									{
										v = QStringToExifUtf(tag->getValueAsString(), false, true, typId);
									}
									else
									{
										v = Exiv2::Value::create(typId);
										v->read(tag->getValueAsString().toStdString());
									}

									curXmpData[tagName.toStdString()] = *v;
								}

							}
						}
						catch(Exiv2::AnyError& err)
						{
							qDebug("AnalogExif: ExifTreeModel::prepareTags() Exiv2 exception (%d) = %s", err.code(), err.what());
							return false;
						}
					}
				}

				// clear dirty flag
				tag->resetDirty();
			}

			// store extra tags in comments, if required
			if((tag->value() != QVariant()) && (etagsStorageOptions != 0 ) && (tag->tagFlags() & (1 << ExifItem::TagFlagExtra)))
			{
				// add extra tag value
				etagsString += "\t" + tag->tagText() + ": " + tag->getValueAsString() + ". \n";
			}
		}
	}

	// store extra tags in comment fields
	if(etagsString != "")
	{
		try
		{
			if(etagsStorageOptions & 0x01)
			{
				// store in Exif.Photo.UserComment
				Exiv2::ExifKey exifKey("Exif.Photo.UserComment");

				QString commentValue = "";

				Exiv2::ExifData::iterator pos = curExifData.findKey(exifKey);
				if(pos != curExifData.end())
				{
					commentValue = ExifUtfToQString(pos->getValue(), true);

					if(commentValue != "")
						commentValue += QString(ETAGS_DELIMETER_IN_COMMENTS);

					// strip etags if stored
					int etagsStartIndex = commentValue.indexOf(ETAGS_START_MARKER_IN_COMMENTS);

					if(etagsStartIndex != -1)
						commentValue = commentValue.left(etagsStartIndex);
				}

				commentValue += (QString(ETAGS_START_MARKER_IN_COMMENTS) + etagsString);

				curExifData["Exif.Photo.UserComment"] = *QStringToExifUtf(commentValue, true, false, Exiv2::comment);
			}
			if(etagsStorageOptions & 0x02)
			{
				// store in Exif.Image.XPComment
				Exiv2::ExifKey exifKey("Exif.Image.XPComment");

				QString commentValue = "";

				Exiv2::ExifData::iterator pos = curExifData.findKey(exifKey);
				if(pos != curExifData.end())
				{
					commentValue = ExifUtfToQString(pos->getValue());

					if(commentValue != "")
						commentValue += QString(ETAGS_DELIMETER_IN_COMMENTS);

					// strip etags if stored
					int etagsStartIndex = commentValue.indexOf(ETAGS_START_MARKER_IN_COMMENTS);

					if(etagsStartIndex != -1)
						commentValue = commentValue.left(etagsStartIndex);
				}

				commentValue += (QString(ETAGS_START_MARKER_IN_COMMENTS) + etagsString);

				curExifData["Exif.Image.XPComment"] = *QStringToExifUtf(commentValue);
			}
		}
		catch(Exiv2::AnyError& err)
		{
			qDebug("AnalogExif: ExifTreeModel::prepareTags() Exiv2 exception (%d) = %s", err.code(), err.what());
			return false;
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
	v->read((Exiv2::byte*)utfData.data(), utfData.length(), Exiv2::littleEndian);

	return v;
}

// convert Exif UTF-16 byte string to QString
QString ExifTreeModel::ExifUtfToQString(Exiv2::Value::AutoPtr exifData, bool checkForMarker, bool isUtf8)
{
	int strSize = exifData->size();
	if(strSize == 0)
		return "";

	unsigned char* data = new unsigned char[strSize];

	exifData->copy(data, Exiv2::littleEndian);

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

bool ExifTreeModel::saveFile(QString filename, bool overwrite)
{
	if(!prepareTags())
		return false;

	try
	{

#ifdef _WIN32
		// unicode paths are supported only in Windows verison
	    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename.toStdWString());
#else
		Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename.toStdString());
#endif

	    if((image.get() == 0) || (!image->good()))
			return false;

		// replace image metadata
		if(overwrite)
		{
		    image->setExifData(curExifData);
			image->setIptcData(curIptcData);
			image->setXmpData(curXmpData);
		}
		else
		{
			// read meta data
			image->readMetadata();

			Exiv2::ExifData imgExifData = image->exifData();

			// add/replace Exif data
			Exiv2::ExifData::const_iterator exifEnd = curExifData.end();
			for(Exiv2::ExifData::const_iterator i = curExifData.begin(); i != exifEnd; ++i)
			{
				imgExifData[i->key()] = *i;
			}

			Exiv2::IptcData imgIptcData = image->iptcData();

			// add/replace Iptc data
			Exiv2::IptcData::const_iterator iptcEnd = curIptcData.end();
			for(Exiv2::IptcData::const_iterator i = curIptcData.begin(); i != iptcEnd; ++i)
			{
				imgIptcData[i->key()] = *i;
			}

			Exiv2::XmpData imgXmpData = image->xmpData();

			// add/replace Xmp data
			Exiv2::XmpData::const_iterator xmpEnd = curXmpData.end();
			for(Exiv2::XmpData::const_iterator i = curXmpData.begin(); i != xmpEnd; ++i)
			{
				imgXmpData[i->key()] = *i;
			}
		}

	    image->writeMetadata();
	}
	catch(Exiv2::AnyError& err)
	{
		qDebug("AnalogExif: ExifTreeModel::saveFile(%s) Exiv2 exception (%d) = %s", filename.toStdString().c_str(), err.code(), err.what());
		return false;
	}

	return true;
}

bool ExifTreeModel::save()
{
	if(!prepareTags())
		return false;

	try
	{
	    exifHandle->setExifData(curExifData);
		exifHandle->setIptcData(curIptcData);
		exifHandle->setXmpData(curXmpData);

	    exifHandle->writeMetadata();
	}
	catch(Exiv2::AnyError& err)
	{
		qDebug("AnalogExif: ExifTreeModel::save() Exiv2 exception (%d) = %s", err.code(), err.what());
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