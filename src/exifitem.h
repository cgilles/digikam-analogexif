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

#ifndef EXIFITEM_H
#define EXIFITEM_H

#include <QString>
#include <QVariant>
#include <QList>
#include <QFlags>

class ExifItem
{
public:
    enum TagType
    {
        TagString           = 0,
        TagInteger          = 1,
        TagUInteger         = 2,
        TagRational         = 3,
        TagURational        = 4,
        TagFraction         = 5,
        TagAperture         = 6,
        TagApertureAPEX     = 7,
        TagShutter          = 8,
        TagISO              = 9,
        TagDateTime         = 10,
        TagText             = 11,
        TagGPS              = 12,
        TagTypeLast
    };

    enum TagFlag
    {
        None        = 0,
        Protected   = 1,
        Extra       = 2,
        Choice      = 4,
        Multi       = 8,
        Ascii       = 16,
        AsciiAlt    = 32,
        Last
    };
    Q_DECLARE_FLAGS(TagFlags, TagFlag);

    ExifItem(const QString& tag, const QString& tagText, const QVariant& tagValue, const QString& printFormat = "%1", TagType type = TagString, TagFlags flags = None, const QString& altTag = "", ExifItem* parent = 0)
    {
        parentItem = parent;
        this->printFormat = printFormat;
        this->type = type;
        metaTag = tag;
        metaAltTag = altTag;
        metaTagText = tagText;
        metaValue = tagValue;
        this->flags = flags;
        dirty = false;
        checked = true;
        srcTagType = 0x1fffe;
    }

    ExifItem(const QString& tag, TagType type, const QString& tagValue, TagFlags flags = None, const QString& altTag = "")
    {
        parentItem = nullptr;
        this->type = type;
        this->flags = flags;
        metaTag = tag;
        metaAltTag = altTag;
        setValueFromString(tagValue);
        dirty = false;
        checked = true;
        srcTagType = 0x1fffe;
    }

    ExifItem(const ExifItem& item)
    {
        parentItem = nullptr;
        type = item.type;
        flags = item.flags;
        metaTag = item.metaTag;
        metaAltTag = item.metaAltTag;
        metaValue = item.metaValue;
        metaTagText = item.metaTagText;
        dirty = item.dirty;
        checked = item.checked;
        srcTagType = item.srcTagType;
        printFormat = item.printFormat;
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

    QString tagAltName() const
    {
        return metaAltTag;
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
    TagType tagType() const
    {
        return type;
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
    void setValueFromString(const QVariant& value, bool setDirty = false, bool convertReal = true);

    // set value ov the given tag for itself or children
    bool findSetTagValueFromString(const QString& tagName, const QVariant& value, bool setDirty = false);
    bool findSetTagValueFromTag(const ExifItem* tag, bool setDirty = false);

    // find tag by its EXIF name
    ExifItem* findTagByName(const QString& name);

    QString getValueAsString(bool convertReal = true);

    static QString valueToString(const QVariant& value, TagType type, const QVariant& oldValue = QVariant(), bool convertReal = true);
    static QVariant valueToString(const QVariant& value, TagType type, const QString formatString, int role);
    // DB conversions
    static QString valueToStringMulti(const QVariant& value, TagType type, TagFlags flags, const QVariant& oldValue);
    static QVariant valueFromString(const QString& value, TagType type, bool convertReal = true, TagFlags flags = None);

    // return dirty flag
    bool isDirty() const
    {
        return dirty;
    }
    // apply tag value
    void resetDirty()
    {
        dirty = false;
    }

    bool isChecked() const
    {
        return checked;
    }

    void setChecked(bool checked)
    {
        this->checked = checked;
    }

    // reset all tag values
    void reset();

    // insert child
    ExifItem* insertChild(const QString& tag, const QString& tagText, const QVariant& tagValue, const QString& printFormat = "%1", TagType type = TagString, TagFlags flags = None, const QString& altTag = "");

    // remove child at given position
    bool removeChild(int position);

    // remove all children
    void removeChildren()
    {
        qDeleteAll(childItems);
        childItems.clear();
    }

    void setSrcTagType(uint type)
    {
        srcTagType = type;
    }

    uint getSrcTagType() const
    {
        return srcTagType;
    }

    static QString typeName(TagType type);

    // flags
    TagFlags tagFlags() const
    {
        return flags;
    }

    void setFlags(TagFlags flags)
    {
        this->flags = flags;
    }

    //static bool testFlag(TagFlags flagValue, TagFlags flag);
    //static TagFlags clearFlag(TagFlags flagValue, TagFlags flag);
    static QString flagName(TagFlag flag);

    static QList<QVariantList> parseEncodedChoiceList(QString list, TagType dataType, TagFlags flags = None);
    static QString findChoiceTextByValue(QString list, QVariant value, TagType dataType, TagFlags flags = None);

private:
    // meta tag name
    QString metaTag;
    QString metaAltTag;
    // readable meta tag text
    QString metaTagText;
    // value - either tag value or caption
    QVariant metaValue;
    // print format in QString arg() style
    QString printFormat;
    // tag type
    TagType type;
    // has changed data
    bool dirty;
    // flags
    TagFlags flags;
    // checked
    bool checked;

    // source tag
    int srcTagType;

    // list of children
    QList<ExifItem*> childItems;
    // parent
    ExifItem* parentItem;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ExifItem::TagFlags);

#endif // EXIFITEM_H
