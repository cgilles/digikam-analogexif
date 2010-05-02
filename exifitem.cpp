#include "exifitem.h"
#include <QStringList>

// insert child
ExifItem* ExifItem::insertChild(const QString& tag, const QString& tagText, const QVariant& tagValue, const QString& printFormat, int type, int flags)
{
	ExifItem* child = new ExifItem(tag, tagText, tagValue, printFormat, type, flags, this);
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
void ExifItem::setValueFromString(const QString& value, bool setDirty)
{
	QVariant val;
	bool ok = false;

	switch(type)
	{
	case TagString:
		val = value;
		break;
	case TagInteger:
	case TagUInteger:
	case TagISO:
		{
			val = value.toInt(&ok);
			if(!ok)
				return;
		}
		break;
	case TagRational:
	case TagURational:
	case TagAperture:
	case TagApertureAPEX:
	case TagFraction:
	case TagShutter:
		{
			if(value.contains('/'))
			{
				// x/y format
				QStringList ratioStr = value.split('/', QString::SkipEmptyParts);

				QVariantList rational;
				// all checks should be done by validator, but it doesn't work
				if(ratioStr.size() != 2)
					return;

				int first = ratioStr.at(0).toInt(&ok);
				if(!ok)
					return;

				int second = ratioStr.at(1).toInt(&ok);
				if(!ok)
					return;

				rational << first << second;
				val = rational;
			}
		}
		break;
	default:
		break;
	}

	setValue(val, setDirty);
}

bool ExifItem::findSetTagValueFromString(const QString& tagName, const QString& value, bool setDirty)
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

QString ExifItem::typeName(TagTypes type)
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

		default:
			return QT_TR_NOOP("unknown");
			break;
	}
}