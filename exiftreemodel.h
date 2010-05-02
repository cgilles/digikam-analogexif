#ifndef EXIFTREEMODEL_H
#define EXIFTREEMODEL_H

#include <QAbstractItemModel>
#include <QSqlDatabase>
#include <QSettings>

// Exiv2 includes
#include <exiv2/image.hpp>

// metadata items
#include "exifitem.h"

// Exif data tree model
class ExifTreeModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	ExifTreeModel(QSqlDatabase& db, QObject *parent);
	~ExifTreeModel();

	// open file for metadata manipulation
	bool openFile(QString filename);
	// save current metatada set in to the specified file
	bool saveFile(QString filename);

	// clear data
	void clear();

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

	 static const int GetTypeRole = Qt::UserRole + 1;

	 // reload data from file
	 void reload();

	 // repopulate model with tags
	 void repopulate();

	 // get meta tag values from the list
	 void setValues(QVariantList& values);

private:
	// return item by index
	ExifItem* getItem(const QModelIndex &index) const;

	// db to hold app settings
	QSqlDatabase& dataBase;

	// create data structure
	void populateModel();

	// read exif values into the model
	bool readMetaValues();

	bool editable;

	Exiv2::Image::AutoPtr exifHandle;

	Exiv2::ExifData curExifData;
	Exiv2::IptcData curIptcData;
	Exiv2::XmpData curXmpData;

	ExifItem* rootItem;

	QSettings settings;
};

#endif // EXIFTREEMODEL_H
