/*
    Copyright (C) 2010 C-41 Bytes <contact@c41bytes.com>

    This file is part of AnalogExif.

    AnalogExif is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    AnalogExif is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with AnalogExif.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef EXIFTREEMODEL_H
#define EXIFTREEMODEL_H

#include <QAbstractItemModel>
#include <QSqlDatabase>
#include <QSettings>
#include <QStringList>

// Exiv2 includes
#include <exiv2/image.hpp>
#include <exiv2/preview.hpp>

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
    // fill the Exiv2 data structures with user data
    bool prepareMetadata();
    // save current metatada set in to the specified file
    bool saveFile(QString filename, bool overwrite = false);
    // set exposure number on the given file
    bool setExposureNumber(QString filename, int exposure);
    // merges the file's metadata with the given Exif metatags list
    bool mergeMetadata(QString filename, QVariantList metadata);

    // reset dirty flag
    void resetDirty();

    // clear data, if deleteObj = true, clear image object as well
    void clear(bool deleteObj = false);

    /// item model methods
    virtual QVariant data(const QModelIndex &index, int role) const;
    static QVariant getItemData(const QVariant& itemValue, const QString& itemFormat, ExifItem::TagFlags itemFlags, ExifItem::TagType itemType, int role = Qt::DisplayRole);

    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &index) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const
    {
                Q_UNUSED(parent);

        // tag and its value
        return 2;
    }

     virtual Qt::ItemFlags flags(const QModelIndex &index) const;
     virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

     /*bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex());
     bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex());*/

     static const int GetTypeRole = Qt::UserRole + 1;
     static const int GetFlagsRole = Qt::UserRole + 2;
     static const int GetChoiceRole = Qt::UserRole + 3;

     // reload data from file
     bool reload();

     // repopulate model with tags
     void repopulate();

     // get meta tag values from the list
     void setValues(QVariantList& values);

     // set editable
    void setReadonly(bool ro = true)
    {
        editable = !ro;
    }

     QByteArray* getPreview() const;

    // Exif UTF-QString conversion
    static Exiv2::Value::AutoPtr QStringToExifUtf(QString qstr, bool addUnicodeMarker = false, bool isUtf8 = false, Exiv2::TypeId typeId = Exiv2::unsignedByte);
    static void QStringToExifUtf(Exiv2::Value& v, QString qstr, bool addUnicodeMarker = false, bool isUtf8 = false, Exiv2::TypeId typeId = Exiv2::unsignedByte);
    static QString ExifUtfToQString(const Exiv2::Value& exifData, bool checkForMarker = false, bool isUtf8 = false);

    static bool tagSupported(const QString tagName);

    static bool registerUserNs(QString userNs, QString userNsPrefix);
    static bool unregisterUserNs();

protected:
    // return item by index
    ExifItem* getItem(const QModelIndex &index) const;

    // create data structure
    void populateModel();

    // read exif values into the model
    bool readMetaValues();
    bool readMetaValues(Exiv2::Image::AutoPtr& exivHandle);

    bool prepareMetadata(Exiv2::ExifData& exifData, Exiv2::IptcData& iptcData, Exiv2::XmpData& xmpData);

    bool storeTag(ExifItem* tag, Exiv2::ExifData& exifData, Exiv2::IptcData& iptcData, Exiv2::XmpData& xmpData);
    void processTag(ExifItem* tag, Exiv2::ExifData& exifData, Exiv2::IptcData& iptcData,  Exiv2::XmpData& xmpData);
    void eraseTag(ExifItem* tag, Exiv2::ExifData& exifData, Exiv2::IptcData& iptcData, Exiv2::XmpData& xmpData);
    void eraseTag(QString tagNames, Exiv2::ExifData& exifData, Exiv2::IptcData& iptcData, Exiv2::XmpData& xmpData);
    void tagValueToMetadata(QVariant value, ExifItem::TagType tagType, Exiv2::Value& v);

    // store extra tags values in the comments of the given Exiv2 ExifData, can throw Exiv2 exceptions
    // etags string should be prepared by prepareMetadata()
    void storeEtags(Exiv2::ExifData& exifData);

    // prepares etags string
    void prepareEtags();
    bool prepareEtagsAndErase(Exiv2::ExifData& exifData, Exiv2::IptcData& iptcData, Exiv2::XmpData& xmpData);

    // extract GPS data from Exif or Xmp
    QString getGPSfromExif();
    QString getGPSfromXmp();

    // store GPS data to Exif or Xmp
    bool parseGPSString(QString gpsStr, QString& latRef, int& latDeg, int& latMin, double& latSec, QString& lonRef, int& lonDeg, int& lonMin, double& lonSec);
    bool parseGPSString(QString gpsStr, QString& latRef, int& latDeg, double& latMin, QString& lonRef, int& lonDeg, double& lonMin);
    bool storeGPSInExif(QString gpsStr, Exiv2::ExifData& xmpData);
    bool storeGPSInXmp(QString gpsStr, Exiv2::XmpData& xmpData);

    // decode tag value from Exiv2 data
    QVariant getTagValueFromExif(ExifItem::TagType tagType, const Exiv2::Value& tagValue, int pos = 0) const;
    static QVariant getItemValue(const QVariant& itemValue, const QString& itemFormat, ExifItem::TagFlags itemFlags, ExifItem::TagType itemType, int role);
    QVariant processItemData(const ExifItem *item, const QVariant& value, bool& ok);

    QVariant readTagValue(QString tagNames, int& srcTagType, ExifItem::TagType tagType, ExifItem::TagFlags tagFlags, Exiv2::ExifData& exifData, Exiv2::IptcData& iptcData, Exiv2::XmpData& xmpData);
    void writeTagValue(QString tagNames, const QVariant& tagValue, ExifItem::TagType type, ExifItem::TagFlags tagFlags, Exiv2::ExifData& exifData, Exiv2::IptcData& iptcData, Exiv2::XmpData& xmpData);

    bool editable;
    // extra tags string
    QString etagsString;

    Exiv2::Image::AutoPtr exifHandle;

    Exiv2::ExifData curExifData;
    Exiv2::IptcData curIptcData;
    Exiv2::XmpData curXmpData;

    ExifItem* rootItem;

    QSettings settings;

    void fillNotSupportedTags();
};



#endif // EXIFTREEMODEL_H
