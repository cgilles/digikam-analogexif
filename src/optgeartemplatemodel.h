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

#ifndef OPTGEARTEMPLATEMODEL_H
#define OPTGEARTEMPLATEMODEL_H

#include <QSqlQueryModel>
#include "exifitem.h"

class OptGearTemplateModel : public QSqlQueryModel
{
	Q_OBJECT

public:
	OptGearTemplateModel(QObject *parent) : QSqlQueryModel(parent) { }
	
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

	int columnCount(const QModelIndex &parent = QModelIndex()) const
	{
                Q_UNUSED(parent);

		// order by, tag name, description, type and print format
		return 5;
	}

	void reload()
	{
		reload(gearId);
	}

	void reload(int id);

	// get gear that uses specified tag
	QStringList getTagUsage(const QModelIndex& idx);
	// delete tag
	void removeTag(const QModelIndex& idx);

	// insert new tag and return its id
	int insertTag(QString tagName, QString tagDesc, QString tagFormat, ExifItem::TagType tagType);

	// swap the orderby fields for the given indexes
	void swapOrderBys(const QModelIndex& idx1, const QModelIndex& idx2);

	// data role to get tag id
	static const int GetTagId = Qt::UserRole + 1;
	static const int GetTagFlagsRole = Qt::UserRole + 2;
	static const int GetAltTagRole = Qt::UserRole + 3;
	static const int GetTagTypeRole = Qt::UserRole + 4;

	// Protect built-in tags
	static const bool ProtectBuiltInTags = true;

private:
	int gearId;
};

#endif // OPTGEARTEMPLATEMODEL_H
