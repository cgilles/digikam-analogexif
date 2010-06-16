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

#include "metadatatagcompleter.h"

QString MetadataTagCompleter::separator() const
{
	return sep;
}

void MetadataTagCompleter::setSeparator(const QString &separator)
{
	sep = separator;
}

QStringList MetadataTagCompleter::splitPath(const QString &path) const
{
	if (sep.isNull()) {
		return QCompleter::splitPath(path);
	}

	return path.split(sep);
}

QString MetadataTagCompleter::pathFromIndex(const QModelIndex &index) const
{
	if (sep.isNull()) {
		return QCompleter::pathFromIndex(index);
	}

	// navigate up and accumulate data
	QStringList dataList;
	for (QModelIndex i = index; i.isValid(); i = i.parent()) {
		dataList.prepend(model()->data(i, completionRole()).toString());
	}

	return dataList.join(sep);
}