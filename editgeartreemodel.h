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

#ifndef EDITGEARTREEMODEL_H
#define EDITGEARTREEMODEL_H

#include <QStandardItemModel>

class EditGearTreeModel : public QStandardItemModel
{
	Q_OBJECT

public:
	EditGearTreeModel(QObject *parent, int type, bool tree = false, QString emptyMsg = QT_TR_NOOP("empty")) : 
		QStandardItemModel(parent), gearType(type), treeView(tree), emptyMessage(emptyMsg)
	{
		reload();
	}

	int getGearType() const
	{
		return gearType;
	}

	void reload();
	QModelIndex reload(int id);

	// drag'n'drop routines
	QStringList mimeTypes() const;
	QMimeData* mimeData(const QModelIndexList &indexes) const;
	bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

	// data roles
	static const int GetGearIdRole		= Qt::UserRole + 1;
	static const int GetGearTypeRole	= Qt::UserRole + 2;

	// create new item and optionally copy properties from existing one
	int createNewGear(int copyId, int parentId, int gearType, QString prefix = "", int orderBy = 0);

	// delete specified gear
	bool deleteGear(int gearId, int gearType);

private:
	int gearType;
	bool treeView;
	QString emptyMessage;
};

#endif // EDITGEARTREEMODEL_H
