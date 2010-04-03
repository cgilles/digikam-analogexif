#ifndef EXIFITEM_H
#define EXIFITEM_H

#include <QString>
#include <QVariant>
#include <QList>

class ExifItem
{
public:
	ExifItem(const QString& tag, const QString& tagText, const QVariant& tagValue, ExifItem* parent = 0)
	{
		parentItem = parent;
		metaTag = tag;
		metaTagText = tagText;
		metaValue = tagValue;
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
		return metaTag == "";
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

	// set caption or value
	void setValue(const QVariant& value)
	{
		metaValue = value;
	}

	// insert child
	ExifItem* insertChild(const QString& tag, const QString& tagText, const QVariant& tagValue);

	// remove child at given position
	bool removeChild(int position);

	// remove all children
	void removeChildren()
	{
		qDeleteAll(childItems);
		childItems.clear();
	}

private:
	// meta tag name
	QString metaTag;
	// readable meta tag text
	QString metaTagText;
	// value - either tag value or caption
	QVariant metaValue;

	// list of children
	QList<ExifItem*> childItems;
	// parent
	ExifItem* parentItem;
};

#endif // EXIFITEM_H