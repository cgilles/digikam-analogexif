#ifndef EXIFTREEMODEL_H
#define EXIFTREEMODEL_H

#include <QAbstractItemModel>
#include <QSqlDatabase>

// Exiv2 includes
#include <exiv2/image.hpp>

// metadata items
#include "exifitem.h"

// Exif data tree model
class ExifTreeModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	ExifTreeModel(QObject *parent);
	~ExifTreeModel();

	// open file for metadata manipulation
	bool openFile(QString filename);

	/// item model methods
	QVariant data(const QModelIndex &index, int role) const;

	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex &index) const;

	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const
	{
		// tag and its value
		return 2;
	}

     Qt::ItemFlags flags(const QModelIndex &index) const;
     bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

     /*bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex());
     bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex());*/

private:
	// return item by index
	ExifItem* getItem(const QModelIndex &index) const;

	// number of model items
	const static int ModelItemsMax = 5;

	// db to hold app settings
	QSqlDatabase db;

	// read metatags
	void populateModel();

	Exiv2::Image::AutoPtr exifHandle;

	ExifItem* rootItem;
};

#endif // EXIFTREEMODEL_H
