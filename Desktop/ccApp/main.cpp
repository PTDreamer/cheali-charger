#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationName("JB");
	QCoreApplication::setOrganizationDomain("jbtecnologia.com");
	QCoreApplication::setApplicationName("cheali controller");
	QApplication a(argc, argv);
	MainWindow w;
	w.show();

	return a.exec();
}
