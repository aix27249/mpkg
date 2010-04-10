#include <QApplication>
#include <QtGui>
#include <QTextCodec>
#include "engine.h"
int main(int argc, char **argv) {
	if (getuid()) {
		// trying to obtain root UID
		setuid(0);
		if (getuid()) {
			string args;
			for (int i=1; i<argc; ++i) {
				args += string(argv[i]) + " ";
			}
			return system("xdg-su -c \"" + string(argv[0]) + " " + args + "\"");
		}
	}

	setlocale(LC_ALL, "");
	bindtextdomain( "mpkg", "/usr/share/locale");
	textdomain("mpkg");
	QApplication qapp(argc, argv);
	QLocale lc;


	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
	QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
	QTranslator translator;
	translator.load("guiinstaller_" + lc.name(), "/usr/share/mpkg");
	qapp.installTranslator(&translator);


	EngineWindow *engineWindow = new EngineWindow;	
	engineWindow->show();
	engineWindow->prepareData(argc, argv);
	return qapp.exec();
}
