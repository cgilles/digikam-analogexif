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

#include "analogexif.h"
#include <QtGui/QApplication>

#ifdef Q_WS_WIN32
#include <QtPlugin>

Q_IMPORT_PLUGIN(qjpeg)
Q_IMPORT_PLUGIN(qsqlite)
#endif

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationName("C-41 Bytes");
	QCoreApplication::setOrganizationDomain("c41bytes.com");
	QCoreApplication::setApplicationName("AnalogExif");

	QApplication a(argc, argv);
	AnalogExif w;

	// initialize and run main window
	if(w.initialize())
	{
		w.show();
		return a.exec();
	}

	// error otherwise
	return -1;
}
