#include <QApplication>
#include "mainwindow.h"
#include <QTextCodec>
#include <mpkg/libmpkg.h>
#include <QLocale>
#include <QTranslator>
int main(int argc, char *argv[]) {
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
	
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
	QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
	QApplication a(argc, argv);
	QLocale lc;
	QTranslator translator;
	translator.load("pkgmanager2_" + lc.name(), "/usr/share/mpkg");
	a.installTranslator(&translator);

	MainWindow w;
	w.show();
	return a.exec();
}

