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

#ifndef GEARTREEMODEL_H
#define GEARTREEMODEL_H

#include <QStandardItemModel>

class GearTreeModel : public QStandardItemModel
{
	Q_OBJECT

public:
	GearTreeModel(QObject *parent) : QStandardItemModel(parent), isApplicable(false)
	{
		reload();
	}

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

	QVariant data(const QModelIndex &item, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

	// reloads the gear
	void reload();

	int bodyCount() const;

private:
	// can user get data from the gear
	bool isApplicable;

	// selected index
	QModelIndex selected;

};

#endif // GEARTREEMODEL_H
