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

#ifndef GEARLISTMODEL_H
#define GEARLISTMODEL_H

#include <QSqlQueryModel>

class GearListModel : public QSqlQueryModel
{
	Q_OBJECT

public:
	GearListModel(QObject *parent, int gType, QString emptyMsg = QT_TR_NOOP("empty")) :
		QSqlQueryModel(parent), isApplicable(false), gearType(gType),  emptyMessage(emptyMsg) { }

	// can user get data from the gear
	void setApplicable(bool applicable);

	// selects the gear
	void setSelectedIndex(const QModelIndex &index)
	{
		selected = index;

		emit dataChanged(index, index);
	}

	// data role to get gear properties
	static const int GetExifData = Qt::UserRole + 1;

	int rowCount(const QModelIndex &index) const;
	QVariant data(const QModelIndex &item, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

	// reloads the gear
	void reload();

protected:
	// can user get data from the gear
	bool isApplicable;

	// gear id
	int gearType;

	// message for the empty lists
	QString emptyMessage;

	// selected index
	QModelIndex selected;
};

#endif // GEARLISTMODEL_H
