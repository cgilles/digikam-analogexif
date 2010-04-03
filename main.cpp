#include "analogexif.h"
#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationName("C-41 Bytes");
	QCoreApplication::setOrganizationDomain("c41bytes.com");
	QCoreApplication::setApplicationName("Analog Exif");

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
