#include "zhwindow.h"
#include <QFile>
#include <QMessageBox>
#include <QTranslator>
#include <QSettings>
#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
	QSettings settings("zhelper.ini", QSettings::IniFormat);

	QApplication app(argc, argv);
	
	if (settings.value("client_id", "0").toUInt() < 1) {
		settings.setValue("client_id", "0");
		QMessageBox::critical( NULL, QObject::tr("Error"), QObject::tr("You client id value is incorrect"), QMessageBox::Ok); 
		return 1;
	}

	QFile keyFile(QApplication::applicationDirPath() + "/key.private");

	if (!keyFile.exists()) {
		QMessageBox::critical( NULL, QObject::tr("Error"), QObject::tr("You key file is not found"), QMessageBox::Ok); 
		return 1;
	}

	settings.sync();

	QTranslator appTranslator;

	if (settings.value("Language", "").toString() != "en") {
		appTranslator.load(settings.value("Language", "").toString() + ".qm", app.applicationDirPath());
		app.installTranslator(&appTranslator);
	}

	ZHelperWindow window;
	window.show();
	return app.exec();
}
