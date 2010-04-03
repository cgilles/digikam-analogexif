#include "exiftreemodel.h"

ExifTreeModel::ExifTreeModel(QObject *parent) : QAbstractItemModel(parent)
{
	// create empty root item
	rootItem = new ExifItem("", "", QVariant());

	db = QSqlDatabase::addDatabase("QSQLITE", ":/analogexif.db");
	db.open();
}

ExifTreeModel::~ExifTreeModel()
{
	delete rootItem;
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

	// clear all
	rootItem->removeChildren();

	// create and read the values of the used metatags
	populateModel();

	return true;
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

	return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
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
	if (role != Qt::DisplayRole && role != Qt::EditRole)
		return QVariant();

	ExifItem *item = getItem(index);

	// for caption and column 0 - return caption
	if(item->isCaption())
	{
		if(index.column() == 0)
			return item->value();
		else
			return QVariant();
	}

	// for meta data return tag name for column 0 and value for column 1
	if(index.column() == 0)
		return item->tagText();

	return item->value();
}

// change the edited data
bool ExifTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role != Qt::EditRole)
		return false;

	ExifItem *item = getItem(index);
	item->setValue(value);

	emit dataChanged(index, index);

	return true;
}

// populate model
void ExifTreeModel::populateModel()
{

	// connect to internal database
//	QSqlQuery query = new QSqlQuery("SELECT 

	beginInsertRows(QModelIndex(), 0, ModelItemsMax);
	ExifItem* headerItem = rootItem->insertChild("", "", tr("Camera description"));



	headerItem->insertChild("Exif.Maker", "Maker", "Canon");
	headerItem->insertChild("Exif.Model", "Model", "Canon EOS 30");

	headerItem = rootItem->insertChild("", "", "Lens");
	headerItem->insertChild("Exif.Model", "Model", "Canon EF 50mm f/1.8 II");
	endInsertRows();
}
