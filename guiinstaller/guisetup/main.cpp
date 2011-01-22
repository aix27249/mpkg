#include <QApplication>
#include "mainwindow.h"
#include <QTextCodec>
#include <mpkg/libmpkg.h>
#include <QLocale>
#include <QTranslator>
int main(int argc, char *argv[]) {
	// This should be run as root since libparted requires it.
	if (getuid()) {
		// trying to obtain root UID
		setuid(0);
		if (getuid()) {
			string args;
			for (int i=1; i<argc; ++i) {
				args += string(argv[i]) + " ";
			}
			// Check if we can run via sudo
			if (system("[\"`sudo -l | grep " + string(argv[0]) | grep NOPASSWD`\" = \"\" ]")) return system("sudo " + string(argv[0]) + " " + args);
			else return system("xdg-su -c \"" + string(argv[0]) + " " + args + "\"");
		}
	}
	if (FileExists("/var/run/guisetup_exec.pid")) {
		string pid_locked = ReadFile("/var/run/guisetup_exec.pid").c_str();
		if (isProcessRunning(pid_locked)) {
			fprintf(stderr, "Another setup process %s is alrealy running.\n", pid_locked.c_str());
			return 1;
		}
	}
	pid_t pid = getpid();
	
	
	if (FileExists("/var/run/guisetup.pid")) {
		string pid_locked = ReadFile("/var/run/guisetup.pid").c_str();
		if (isProcessRunning(pid_locked)) {
			fprintf(stderr, "Another setup process %s is alrealy running.\n", pid_locked.c_str());
			return 1;
		}
	}
	WriteFile("/var/run/guisetup.pid", IntToStr(pid));


	setlocale(LC_ALL, "");
	bindtextdomain( "mpkg", "/usr/share/locale");
	textdomain("mpkg");
	
	// For mpkg, note that we copy config to temp directory	
	CONFIG_FILE="/tmp/mpkg.xml";
	mConfig.configName=CONFIG_FILE;
	unlink("/tmp/packages.db");
	unlink("/tmp/mpkg.xml");
	if (!FileExists("/usr/share/setup/packages.db")) {
		mError("Oops, no database template in /usr/share/setup/packages.db!");
		return 1;
	}
	if (!FileExists("/usr/share/setup/mpkg-setup.xml")) {
		mError("Oops, no config template in /usr/share/setup/mpkg-setup.xml!");
		return 1;
	}
	system("cp /usr/share/setup/packages.db /tmp/packages.db");
	system("cp /usr/share/setup/mpkg-setup.xml /tmp/mpkg.xml");

	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
	QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
	QApplication a(argc, argv);
	QLocale lc;
	QTranslator translator;
	translator.load("guisetup_" + lc.name(), "/usr/share/setup/l10n");
	a.installTranslator(&translator);

	MainWindow w;
	w.show();
	return a.exec();
}

