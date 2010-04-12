/*****************************************************
 * MOPSLinux packaging system
 * Package builder - header
 * $Id: mainwindow.h,v 1.29 2007/12/11 00:57:19 i27249 Exp $
 * ***************************************************/

#ifndef MV_H
#define MV_H
#include <QTextCodec>
#include <QtGui>
#include "ui_package_builder.h"
#include <QDir>
#include <mpkg/libmpkg.h>
#include <mpkg/errorhandler.h>
typedef enum {
       	TYPE_NONE=0,
	TYPE_TGZ,
	TYPE_XML
} TargetType;

typedef enum {
	FACT_COPY=0,
	FACT_MOVE,
	FACT_REMOVE
} FileAction;

using namespace std;
struct keys
{
	string name;
	string value;
};

class Form: public QWidget
{
	Q_OBJECT
	public:
		Form (QWidget *parent = 0, string arg="");
		~Form();
		QString _wTitle;
	public slots:

		MpkgErrorReturn errorHandler(ErrorDescription err, const string& details);
		void importFromDatabase();
		void showAbout();
		void loadData();
		void loadFile(QString filename);
		bool saveFile(bool saveAsMode=false, bool buildAfterSave=false, bool installAfterSave=false);
		void saveAndBuild();
		void buildAndInstall();
		bool saveAs();
		void importMetaFromFile();
		void addConfigFile();
		void deleteConfigFile();
		void searchConfigFile();
		void searchTempFile();
		void addTag();
		void addDependency();
		void deleteTag();
		void deleteDependency();
		void changeHeader(const QString &);
		void saveAndExit();
		void changeHeader();
		void swapLanguage();
		void storeCurrentDescription();
		void addDepsFromFiles();
		void quitApp();
		void browsePatch();
		void loadTemplateList();
		void applyTemplate();
		void saveTemplate();
		void saveTemplateNew();
		void switchConfigureField(int state);
		void switchEnvField(int state);
		void switchCompilationField(int state);
		void switchInstallField(int state);
		void switchSourceDirectoryField(int state);
		void switchBuildSystem(int index);
		void switchCpuArchField(int state);
		void switchCpuTuneField(int state);
		void switchOptimizationField(int state);
		void switchGccOptionsField(int state);
		void switchScript(int state);
		void switchPreBuildScript(int state);
		void displayKeys();
		void displayPatches();
		void addKey();
		void addPatch();
		void deletePatch();
		void deleteKey();

		void embedSources();
		void embedPatches();
		void analyzeSources();
		void analyzeName(bool force);
		void analyzeName_auto();
		void analyzeName_forced();
		//void switchAdvancedUrlMode();
		//void loadAdvancedUrlTable();
		//void addAdvancedUrl();
		//void deleteAdvancedUrl();
		//void switchExtractPath();

		void editBuildScriptWithGvim();
		void loadBuildScriptFromFile();
		void reloadPackageDirListing();
		void setCurrentPathLabel();
		void setNewPackageCurrentDirectory(QListWidgetItem *item);
		void goPackageHome();
		void goPackageUp();
		void reloadFilesystemDirListing();
		void setNewFilesystemCurrentDirectory(QListWidgetItem *item);
		void goFilesystemHome();
		void goFilesystemUp();

		void copyFiles();
		void moveFiles();
		void removeFiles();
		void renameFile();
		void viewFile();
		void makeDirectory();
		void editFile();
		void manageFiles(FileAction action);
		void setPanel1Focus();
		void setPanel2Focus();
		void execShellCommand();
		void addToResent(string filename);
		void openResent(int index);
		void runBuild(string filename, bool installAfterSave=false);

	public:
		Ui::Form ui;
		QLabel *debugLabel;
	private:
		bool sourcesAnalyzed;
		StringMap advancedUrlMap;
		int focusIndex;
		QString templateDirPath;
		QDir *packageDir;
		QDir *currentPackageDir;
		QDir *currentFilesystemDir;
		QString prevPackageDirName, prevFilesystemDirName;
		bool pBuilder_isStartup;
		string tempScriptEditFile;
		string _arg;
		std::vector<QString> short_description;
		std::vector<QString> description;
		bool modified;
		vector<string> patchList;
		vector<keys> keyList;

		BinaryPackage binaryPackage;
		SourcePackage sourcePackage;
		int dataType;
		mpkg *db;
		QString saveDir;
};
#endif
