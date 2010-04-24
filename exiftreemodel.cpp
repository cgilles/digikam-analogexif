#include "exiftreemodel.h"
#include "exifutils.h"

#include <QSqlQuery>
#include <QStringList>
#include <QVariantList>
#include <QFont>
#include <QBrush>

#include <cmath>

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
}

// open file and read metadata
bool ExifTreeModel::openFile(QString filename)
{
	// invalidate the model
	reset();

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
	// QSqlQuery query("SELECT b.id, b.Description, a.TagName, a.TagText, a.PrintFormat, a.TagType FROM MetaTags a, MetaCategories b where a.CategoryId=b.id ORDER BY b.OrderBy, a.OrderBy");
	QSqlQuery query("SELECT a.GearType, b.TagName, b.TagText, b.PrintFormat, b.TagType FROM GearTemplate a, MetaTags b WHERE b.id=a.TagId ORDER BY a.GearType, a.OrderBy, b.OrderBy");

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
			case 3:
				categoryTitle = tr("Frame");
				break;
			}

			headerItem = rootItem->insertChild("", "", curCategoryId);
			nRows++;
			
		}

		// insert a tag
		headerItem->insertChild(query.value(2).toString(), query.value(3).toString(), QVariant(), query.value(4).toString(), query.value(5).toInt());
		nRows++;
	}
	beginInsertRows(QModelIndex(), 0, nRows);
	endInsertRows();
}

bool ExifTreeModel::readMetaValues()
{
	Exiv2::ExifData &exifData = exifHandle->exifData();

	// browse through all categories
	for(int i = 0; i < rootItem->childCount(); i++)
	{
		ExifItem* category = rootItem->child(i);
		for(int j = 0; j < category->childCount(); j++)
		{
			ExifItem* tag = category->child(j);

			try
			{
				QString tagType = tag->tagName().split(".").at(0);
				if(tagType == "Exif")
				{
					// Exif data
					Exiv2::Exifdatum tagValue = exifData[tag->tagName().toStdString()];

					Exiv2::TypeId typId = tagValue.typeId();
					switch(typId)
					{
					case Exiv2::asciiString:
					case Exiv2::comment:
						tag->setValue(QString::fromStdString(exifData[tag->tagName().toStdString()].toString()));
						break;
					case Exiv2::unsignedByte:
					case Exiv2::unsignedShort:
					case Exiv2::unsignedLong:
					case Exiv2::signedByte:
					case Exiv2::signedShort:
					case Exiv2::signedLong:
						tag->setValue(exifData[tag->tagName().toStdString()].toLong());
						break;
					case Exiv2::unsignedRational:
					case Exiv2::signedRational:
						{
							Exiv2::Rational val = exifData[tag->tagName().toStdString()].toRational();
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
					tag->setValue(QVariant());
				}
				else if(tagType == "Xmp")
				{
					// XMP tags
					tag->setValue(QVariant());
				}
			}
			catch(Exiv2::AnyError&)
			{
				return false;
			}
		}
	}

	return true;
}

void ExifTreeModel::reload()
 {
	 reset();

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