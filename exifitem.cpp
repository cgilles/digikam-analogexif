#include "exifitem.h"

// insert child
ExifItem* ExifItem::insertChild(const QString& tag, const QString& tagText, const QVariant& tagValue)
{
	ExifItem* child = new ExifItem(tag, tagText, tagValue, this);
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
