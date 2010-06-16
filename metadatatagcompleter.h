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

#ifndef METADATATAGCOMPLETER_H
#define METADATATAGCOMPLETER_H

#include <QCompleter>
#include <QStringList>

class MetadataTagCompleter : public QCompleter
{
	Q_OBJECT
	Q_PROPERTY(QString separator READ separator WRITE setSeparator)

public:
	MetadataTagCompleter(QObject *parent = 0) : QCompleter(parent) { }
	MetadataTagCompleter(QAbstractItemModel *model, QObject *parent = 0) : QCompleter(model, parent) { }
	MetadataTagCompleter(const QStringList& list, QObject* parent = 0) : QCompleter(list, parent) { }

	QString separator() const;
public slots:
	void setSeparator(const QString &separator);

protected:
	QStringList splitPath(const QString &path) const;
	QString pathFromIndex(const QModelIndex &index) const;

private:
	QString sep;
};

#endif // METADATATAGCOMPLETER_H
