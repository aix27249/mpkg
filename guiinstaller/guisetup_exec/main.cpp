#include <QApplication>
#include "mainwindow.h"
#include <QTextCodec>
#include <mpkg/libmpkg.h>
#include <QLocale>
#include <QTranslator>
int main(int argc, char *argv[]) {
	setupMode=true;
	simulate=false;
	forceSkipLinkMD5Checks=true;
	forceInInstallMD5Check = false;
	// We don't care of database integrity in case of installation failure half-way, so we can use fast mode. It has beed tested enough, and I think it is useful.
	//  Warp drive active!
	_cmdOptions["ramwarp"]="yes";
	_cmdOptions["warpmode"]="yes";
	noEject=true;


	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
	QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
	QApplication a(argc, argv);

	MainWindow w;
	w.show();
	return a.exec();
}

