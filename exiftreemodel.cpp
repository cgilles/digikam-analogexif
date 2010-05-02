#include "exiftreemodel.h"
#include "exifutils.h"

#include <QSqlQuery>
#include <QStringList>
#include <QVariantList>
#include <QDateTime>
#include <QFont>
#include <QBrush>

#include <cmath>

#define CUSTOM_XMP_NAMESPACE_URI	("http://www.c41bytes.com/analogexif/ns")

ExifTreeModel::ExifTreeModel(QSqlDatabase& db, QObject *parent) : QAbstractItemModel(parent), dataBase(db)
{
	// create empty root item
	rootItem = new ExifItem("", "", QVariant());

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
		exifHandle = Exiv2::ImageFactory::open(filename.toStdString());
		if(exifHandle.get() == 0)
			return false;
		// read metadata
		exifHandle->readMetadata();
	}
	catch(Exiv2::AnyError&)
	{
		return false;
	}

	// read all tags from model
	if(readMetaValues())
	{
		editable = true;
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
	if (role != Qt::DisplayRole && role != Qt::EditRole && role != GetTypeRole && role != Qt::FontRole)
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

	if(item->value() == QVariant())
		return QVariant();

	// return value according to the tag type
	switch(item->tagType())
	{
	case ExifItem::TagString:
		if(role == Qt::DisplayRole)
			return QString(item->format()).arg(item->value().toString());
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
			QVariantList rational = item->value().toList();
			double value = (double)rational.at(0).toInt() / (double)rational.at(1).toInt();

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

	// return value according to the tag type
	switch(item->tagType())
	{
	case ExifItem::TagString:
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
			QVariantList rational;
			double val = value.toDouble();

			// correction for APEX values
			if(item->tagType() == ExifItem::TagApertureAPEX)
			{
				val = 2*log(val)/log(2.0);
			}

			rational = item->value().toList();
			if(rational != QVariantList())
			{
				// special check whether value is changed for rational numbers
				double oldValue = (double)rational.at(0).toInt() / (double)rational.at(1).toInt();

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
			rational.clear();

			int first, second;
			ExifUtils::doubleToFraction(val, &first, &second);

			rational << first << second;

			item->setValue(rational, true);
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

bool ExifTreeModel::readMetaValues()
{
	QString exifComments;

	// read comments for extra tags
	try
	{
		curExifData = exifHandle->exifData();

		int etagsStorageOptions = settings.value("extraTagsStorage", 0x03).toInt();

		if(etagsStorageOptions & 0x01)
		{
			Exiv2::Exifdatum tagValue = curExifData["Exif.Photo.UserComment"];

			if(tagValue.count() != 0)
				exifComments = QString::fromStdString(tagValue.toString());
		}

		if((etagsStorageOptions & 0x02) && (exifComments != ""))
		{
			Exiv2::Exifdatum tagValue = curExifData["Exif.Image.XPComment"];

			if(tagValue.count() != 0)
				exifComments = QString::fromStdString(tagValue.toString());
		}

	}
	catch(Exiv2::AnyError&)
	{
		return false;
	}

	// browse through all categories
	for(int i = 0; i < rootItem->childCount(); i++)
	{
		ExifItem* category = rootItem->child(i);
		for(int j = 0; j < category->childCount(); j++)
		{
			ExifItem* tag = category->child(j);

#if 0
			if(tag->tagFlags() & ExifItem::TagFlagExtra)
			{
				// special care for extra tags
				int tagPos = exifComments.indexOf(tag->tagText() + ": ");
				if(tagPos != -1)
				{
					// get value
					QString val = exifComments.mid(tagPos + tag->tagText().length() + 2, exifComments.indexOf(QChar('\n'), tagPos));
					tag->setValueFromString(val);
				}
				else
				{
					// TODO: Xmp schema etc.?
				}
				continue;
			}
#endif

			QStringList tags = tag->tagName().remove(QChar(' ')).split(",", QString::SkipEmptyParts);

			foreach(QString tagName, tags)
			{
				try
				{
					QString tagType = tagName.split(".").at(0);
					if(tagType == "Exif")
					{
						// Exif data
						
						Exiv2::Exifdatum tagValue = curExifData[tagName.toStdString()];

						if(tagValue.count() == 0)
							continue;

						Exiv2::TypeId typId = tagValue.typeId();
						switch(typId)
						{
						case Exiv2::asciiString:
						case Exiv2::comment:
							tag->setValue(QString::fromStdString(tagValue.toString()));
							break;
						case Exiv2::unsignedByte:
						case Exiv2::unsignedShort:
						case Exiv2::unsignedLong:
						case Exiv2::signedByte:
						case Exiv2::signedShort:
						case Exiv2::signedLong:
							tag->setValue(curExifData[tagName.toStdString()].toLong());
							break;
						case Exiv2::unsignedRational:
						case Exiv2::signedRational:
							{
								Exiv2::Rational val = curExifData[tagName.toStdString()].toRational();
								QVariantList rational;
								rational << val.first << val.second;
								tag->setValue(rational);
								break;
							}
						default:
							tag->setValue(QVariant());
							break;
						}
					}
					else if(tagType == "Iptc")
					{
						// IPTC tags

						curIptcData = exifHandle->iptcData();

						Exiv2::Iptcdatum tagValue = curIptcData[tagName.toStdString()];

						if(tagValue.count() == 0)
							continue;

						tag->setValue(QVariant());
					}
					else if(tagType == "Xmp")
					{
						// XMP tags

						curXmpData = exifHandle->xmpData();

						Exiv2::Xmpdatum tagValue = curXmpData[tagName.toStdString()];

						if(tagValue.count() == 0)
							continue;

						tag->setValue(QVariant());
					}
				}
				catch(Exiv2::AnyError&)
				{
					return false;
				}
			}
		}
	}

	return true;
}

void ExifTreeModel::reload()
 {
	 clear();

	 if(readMetaValues())
		editable = true;
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
	if(editable)
		reload();
}

bool ExifTreeModel::saveFile(QString filename)
{
	// register custom AnalogExif XMP namespace
	Exiv2::XmpProperties::registerNs(CUSTOM_XMP_NAMESPACE_URI, "AnalogExif");

	// browse through all categories
	for(int i = 0; i < rootItem->childCount(); i++)
	{
		ExifItem* category = rootItem->child(i);
		for(int j = 0; j < category->childCount(); j++)
		{
			ExifItem* tag = category->child(j);

			if(tag->isDirty())
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
							
							Exiv2::Exifdatum tagValue = curExifData[tagName.toStdString()];

							if(tagValue.count() == 0)
								continue;

							Exiv2::TypeId typId = tagValue.typeId();
							switch(typId)
							{
							case Exiv2::asciiString:
							case Exiv2::comment:
								tag->setValue(QString::fromStdString(tagValue.toString()));
								break;
							case Exiv2::unsignedByte:
							case Exiv2::unsignedShort:
							case Exiv2::unsignedLong:
							case Exiv2::signedByte:
							case Exiv2::signedShort:
							case Exiv2::signedLong:
								tag->setValue(curExifData[tagName.toStdString()].toLong());
								break;
							case Exiv2::unsignedRational:
							case Exiv2::signedRational:
								{
									Exiv2::Rational val = curExifData[tagName.toStdString()].toRational();
									QVariantList rational;
									rational << val.first << val.second;
									tag->setValue(rational);
									break;
								}
							default:
								tag->setValue(QVariant());
								break;
							}
						}
						else if(tagType == "Iptc")
						{
							// IPTC tags

							curIptcData = exifHandle->iptcData();

							Exiv2::Iptcdatum tagValue = curIptcData[tagName.toStdString()];

							if(tagValue.count() == 0)
								continue;

							tag->setValue(QVariant());
						}
						else if(tagType == "Xmp")
						{
							// XMP tags

							curXmpData = exifHandle->xmpData();

							Exiv2::Xmpdatum tagValue = curXmpData[tagName.toStdString()];

							if(tagValue.count() == 0)
								continue;

							tag->setValue(QVariant());
						}
					}
					catch(Exiv2::AnyError&)
					{
						return false;
					}
				}
			}
		}
	}


}
