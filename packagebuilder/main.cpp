/****************************************************************
 *     MPKG packaging system
 *     Package builder - main file
 *     $Id: main.cpp,v 1.17 2007/11/23 01:01:45 i27249 Exp $
 ***************************************************************/

#include <QApplication>
 #include <QPushButton>
 #include <QListWidget>
 #include <QWidget>
#include "mainwindow.h"
#include <QtGui>
#include <string>
#include <QLocale>
using namespace std;
int main(int argc, char *argv[])
{
	_cmdOptions["sql_readonly"]="yes"; // We use database only in read-only, does we? :)
	// Now we can pass as argument:
	// 	package archive filename
	// 	package index data.xml
	
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

	QTranslator translator;
	translator.load("packagebuilder_" + QLocale::system().name(), "/usr/share/mpkg/");

	setlocale(LC_ALL, "");
	bindtextdomain( "mpkg", "/usr/share/locale");
	textdomain("mpkg");
	QApplication app(argc, argv);
	app.installTranslator(&translator);

	string arg;
	if (argc == 2) arg=argv[1];
	
     	Form *mw=new Form(0,arg);


	//mw->show();
	QObject::connect(mw->ui.addConfFileButton, SIGNAL(clicked()), mw, SLOT(addConfigFile()));
	QObject::connect(mw->ui.delConfFileButton, SIGNAL(clicked()), mw, SLOT(deleteConfigFile()));
	QObject::connect(mw->ui.conffileSearchButton, SIGNAL(clicked()), mw, SLOT(searchConfigFile()));
	QObject::connect(mw->ui.LoadButton, SIGNAL(clicked()), mw, SLOT(loadData()));
	QObject::connect(mw->ui.TagAddButton, SIGNAL(clicked()), mw, SLOT(addTag()));
	QObject::connect(mw->ui.TagDeleteButton, SIGNAL(clicked()), mw, SLOT(deleteTag()));
	QObject::connect(mw->ui.DepAddButton, SIGNAL(clicked()), mw, SLOT(addDependency()));
	QObject::connect(mw->ui.BuildButton, SIGNAL(clicked()), mw, SLOT(saveFile()));
	QObject::connect(mw->ui.saveAndBuildButton, SIGNAL(clicked()), mw, SLOT(saveAndBuild()));
	QObject::connect(mw->ui.buildInstallButton, SIGNAL(clicked()), mw, SLOT(buildAndInstall()));
	QObject::connect(mw->ui.DepDeleteButton, SIGNAL(clicked()), mw, SLOT(deleteDependency()));
	QObject::connect(mw->ui.NameEdit, SIGNAL(textChanged(const QString &)), mw, SLOT(changeHeader(const QString &)));
	QObject::connect(mw->ui.VersionEdit, SIGNAL(textChanged(const QString &)), mw, SLOT(changeHeader(const QString &)));
	QObject::connect(mw->ui.BuildEdit, SIGNAL(textChanged(const QString &)), mw, SLOT(changeHeader(const QString &)));
	QObject::connect(mw->ui.ArchComboBox, SIGNAL(currentIndexChanged(const QString &)), mw, SLOT(changeHeader()));
	QObject::connect(mw->ui.quitButton, SIGNAL(clicked()), mw, SLOT(quitApp()));
	//QObject::connect(mw->ui.DescriptionLanguageComboBox, SIGNAL(currentIndexChanged(const QString &)), mw, SLOT(swapLanguage()));
	QObject::connect(mw->ui.ShortDescriptionEdit, SIGNAL(textChanged(const QString &)), mw, SLOT(changeHeader(const QString &)));
	QObject::connect(mw->ui.DescriptionEdit, SIGNAL(textChanged()), mw, SLOT(changeHeader()));
	QObject::connect(mw->ui.ChangelogEdit, SIGNAL(textChanged()), mw, SLOT(changeHeader()));
//	QObject::connect(mw->ui.TagEdit, SIGNAL(textChanged(const QString &)), mw, SLOT(changeHeader(const QString &)));
	QObject::connect(mw->ui.DepAddButton, SIGNAL(clicked()), mw, SLOT(changeHeader()));
	QObject::connect(mw->ui.DepDeleteButton, SIGNAL(clicked()), mw, SLOT(changeHeader()));
	QObject::connect(mw->ui.TagAddButton, SIGNAL(clicked()), mw, SLOT(changeHeader()));
	QObject::connect(mw->ui.TagDeleteButton, SIGNAL(clicked()), mw, SLOT(changeHeader()));
	QObject::connect(mw->ui.MaintainerNameEdit, SIGNAL(textChanged(const QString &)), mw, SLOT(changeHeader()));
	QObject::connect(mw->ui.MaintainerMailEdit, SIGNAL(textChanged(const QString &)), mw, SLOT(changeHeader()));










     return app.exec();
 } 
