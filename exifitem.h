#ifndef EXIFITEM_H
#define EXIFITEM_H

#include <QString>
#include <QVariant>
#include <QList>

class ExifItem
{
public:
	ExifItem(const QString& tag, const QString& tagText, const QVariant& tagValue, const QString& printFormat = "%1", int type = TagString, int flags = TagFlagNone, ExifItem* parent = 0)
	{
		parentItem = parent;
		this->printFormat = printFormat;
		this->type = type;
		metaTag = tag;
		metaTagText = tagText;
		metaValue = tagValue;
		this->flags = flags;
		dirty = false;
	}

	ExifItem(const QString& tag, int type, const QString& tagValue, int flags = TagFlagNone)
	{
		parentItem = NULL;
		this->type = type;
		this->flags = flags;
		metaTag = tag;
		setValueFromString(tagValue);
		dirty = false;
	}

	~ExifItem(void)
	{
		// free all children
		qDeleteAll(childItems);
	}

	// return parent item
	ExifItem* parent()
	{
		return parentItem;
	}

	// return child by number
	ExifItem* child(int number)
	{
		return childItems.value(number);
	}

	// return child count
	int childCount() const
	{
		return childItems.size();
	}

	// return self child item, 0 for root
	int childNumber() const;

	// check whether caption item (i.e. no tag associated)
	bool isCaption() const
	{
		if(flags == 0)
			return metaTag == "";
		else
			return false;
	}

	// return associated tag
	QString tagName() const
	{
		return metaTag;
	}

	// return associated tag
	QString tagText() const
	{
		return metaTagText;
	}

	// return either caption or tag value
	QVariant value() const
	{
		return metaValue;
	}

	// print format
	QString format() const
	{
		return printFormat;
	}

	// tag type
	int tagType() const
	{
		return type;
	}

	// flags
	int tagFlags() const
	{
		return flags;
	}

	void setFlags(int flags)
	{
		this->flags = flags;
	}


	// set caption or value
	void setValue(const QVariant& value, bool setDirty = false)
	{
		if(metaValue != value)
		{
			metaValue = value;
			dirty = setDirty;
		}
	}



	// set caption or value
	void setValueFromString(const QString& value, bool setDirty = false);

	// set value ov the given tag for itself or children
	bool findSetTagValueFromString(const QString& tagName, const QString& value, bool setDirty = false);

	// return dirty flag
	bool isDirty() const
	{
		return dirty;
	}

	// reset all tag values
	void reset();

	// insert child
	ExifItem* insertChild(const QString& tag, const QString& tagText, const QVariant& tagValue, const QString& printFormat = "%1", int type = 0, int flags = TagFlagNone);

	// remove child at given position
	bool removeChild(int position);

	// remove all children
	void removeChildren()
	{
		qDeleteAll(childItems);
		childItems.clear();
	}

	enum TagTypes
	{
		TagString			= 0,
		TagInteger			= 1,
		TagUInteger			= 2,
		TagRational			= 3,
		TagURational		= 4,
		TagFraction			= 5,
		TagAperture			= 6,
		TagApertureAPEX		= 7,
		TagShutter			= 8,
		TagISO				= 9,
		TagDateTime			= 10,
		TagTypeLast
	};

	static QString typeName(TagTypes type);

	enum TagFlags
	{
		TagFlagNone			= 0,
		TagFlagProtected	= 1,
		TagFlagExtra		= 2,
		TagFlagLast
	};

private:
	// meta tag name
	QString metaTag;
	// readable meta tag text
	QString metaTagText;
	// value - either tag value or caption
	QVariant metaValue;
	// print format in QString arg() style
	QString printFormat;
	// tag type
	int type;
	// has changed data
	bool dirty;
	// flags
	int flags;

	// list of children
	QList<ExifItem*> childItems;
	// parent
	ExifItem* parentItem;
};

#endif // EXIFITEM_H