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

#ifndef EDITGEARTAGSMODEL_H
#define EDITGEARTAGSMODEL_H

#include <QSqlQueryModel>

class EditGearTagsModel : public QSqlQueryModel
{
	Q_OBJECT

public:
	EditGearTagsModel(QObject *parent) : QSqlQueryModel(parent), gearId(-1) { }

	QVariant data(const QModelIndex &index, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

	static const int GetTagIdRole = Qt::UserRole + 4;

	int columnCount(const QModelIndex &parent = QModelIndex()) const
	{
		// tag and its value
		return 2;
	}

	void reload()
	{
		reload(gearId);
	}

	void reload(int id);

	virtual void clear()
	{
		QSqlQueryModel::clear();
		emit cleared();
	}

	bool addNewTag(int tagId, int orderBy = 0);
	bool deleteTag(int tagId);

signals:
	void cleared();

private:
	int gearId;
};

#endif // EDITGEARTAGSMODEL_H
