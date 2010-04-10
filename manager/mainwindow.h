/*****************************************************
 * MOPSLinux packaging system
 * Package manager UI - header
 * $Id: mainwindow.h,v 1.57 2007/09/09 23:24:08 i27249 Exp $
 * ***************************************************/

#ifndef MV_H
#define MV_H
#include "ui_pkgmanager.h"
#include "ui_aboutbox.h"
#include "ui_preferencesbox.h"
#include "preferencesbox.h"
#include "ui_loading.h"
#include <mpkg/libmpkg.h>
#include "loading.h"
#include "db.h"
#include "corethread.h"

#include <QThread>
#define PT_INSTALLCHECK 0
#define PT_NAME 1
#define PT_ID 2
class PrevState {
	
	public:
	PrevState();
	~PrevState();
	void resize(unsigned int newSize);
	unsigned int size();
	ProgressData data;
	vector<QTableWidgetItem *> label;
	vector<QTableWidgetItem *> bar;
	vector<QTableWidgetItem *> state;
};
class MainWindow: public QMainWindow
{
	Q_OBJECT
	public:

		bool isCategoryComplain(int package_num, int category_id);
		double installQueueSize;
		bool nameComplain(int package_num, QString text);
		bool initializeOk;
		MainWindow (QMainWindow *parent = 0);
		~MainWindow();
		coreThread *thread;
		statusThread *StatusThread;
		errorBus *ErrorBus;
		PreferencesBox *prefBox;

		bool subsysOk;

	signals:
		void fillReady();
		void imReady();
		void requestPackages(vector<bool> _showMask);
		void getAvailableTags();
		void redrawReady(bool flag);
		void loadPackageDatabase();
		void startThread();
		void startStatusThread();
		void startErrorBus();
		void sendUserReply(QMessageBox::StandardButton reply);
		void syncData();
		void updateDatabase();
		void quitThread();
		void callCleanCache();
		void commit(vector<int> nStatus);
		void backsync(vector<int> nStatus);
		void getRequiredPackages(unsigned int package_num);
		void getDependantPackages(unsigned int package_num);
	public slots:
		void loadDefaultData();
		void highlightCategoryList();
		void generateStat(vector<int> newStatusData);
		void receiveRequiredPackages(unsigned int package_num, PACKAGE_LIST req);
		void receiveDependantPackages(unsigned int pacakge_num, PACKAGE_LIST dep);
		void showMessageBox(QString header, QString body);
		void receiveAvailableTags(vector<string> tags);
		void setSkipButton(bool flag);
		void setIdleButtons(bool flag);
		void abortActions();
		void skipAction();
		void resetProgressBar();
		void setInitOk(bool flag);
		void updateProgressData_new(); // New progressData: sometimes fails, so disabled for now.
		void updateProgressData();
		void showProgressWindow(bool flag);
		void hideEntireTable();
		void showErrorMessage(QString headerText, QString bodyText, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton);
		void initCategories(bool initial=false);
		void filterCategory(int category_id);
		void applyPackageFilter();
		void setTableSize();
		void filterCloneItems();

		void receivePackageList(PACKAGE_LIST pkgList, vector<int> nStatus);
		void setStatus(QString status);
		void loadData();
		void errorLoadingDatabase();
		void sqlQueryBegin();
		void sqlQueryEnd();
		void loadingStarted();
		void loadingFinished();
		
		// Progress bar
		void enableProgressBar();
		void disableProgressBar();
		void setProgressBarValue(unsigned int value);
		void initProgressBar(unsigned int stepCount = 100);
		
		void enableProgressBar2();
		void disableProgressBar2();
		void setProgressBarValue2(unsigned int value);
		void initProgressBar2(unsigned int stepCount = 100);
	
		// Table operations
		void clearTable();
		void selectAll();
		void deselectAll();
		void setTableSize(unsigned int size);
		void setTableItem(unsigned int row, int packageNum, bool checkState, string cellItemText);
		void setTableItemVisible(unsigned int row, bool visible);

		void showPreferences();
		void showAbout();
		void quitApp();
		void showCoreSettings();
		void commitChanges();
		void resetChanges();
		void resetQueue();
		void cleanCache();
		void showAddRemoveRepositories();
//		void setInstalledFilter();
		void clearForm();
		void updateData();
		void execMenu();
		void showPackageInfo();
		void markChanges(int x, Qt::CheckState state, int force_state=-1);
		void quickPackageSearch();

	public:
		Ui::MainWindow ui;
		Ui::aboutBox _aboutBox;
		LoadingBox *loadBox;
		DatabaseBox *dbBox;
		QMenu *tableMenu;
		QAction *installPackageAction;
		QAction *removePackageAction;
		QAction *purgePackageAction;

		// Toolbar
		QAction *applyChangesAction;
		QAction *resetChangesAction;
		QAction *showConfigAction;
		QAction *showRepositoryConfigAction;
		QAction *updateRepositoryDataAction;
		QAction *applyInstalledFilterAction;
		QAction *applyAvailableFilterAction;
		QAction *applyNotPurgedFilterAction;
		QAction *applyDeprecatedFilterAction;
		QAction *applyUnavailableFilterAction;

		XMLNode _categories;
		
	private:
		void lockPackageList(bool state);
		void waitUnlock();
		map <string, bool> highlightMap;
		double totalInstalledSize;
		double totalAvailableSize;
		unsigned int totalAvailableCount;
		unsigned int installedCount;
		unsigned int installQueueCount;
		unsigned int removeQueueCount;
		unsigned int updateQueueCount;
		double willBeFreed;
		double willBeOccupied;
		bool __pkgLock;
		string bool2str(bool data);
		QMovie *movie; 
		mpkg *mDb;
		void setBarValue(QProgressBar *Bar, int stepValue);
		PACKAGE_LIST *packagelist;
		vector<string> install_queue;
		vector<string> remove_queue;
		vector<string> purge_queue;
		vector<bool>highlightState;
		vector<bool>stateChanged;
		vector<int>newStatus;
		vector<string> availableTags;
		void initPackageTable();
		int currentCategoryID;

};

class CheckBox: public QCheckBox
{
	Q_OBJECT
	public:
		CheckBox(MainWindow *parent);
	public slots:
		void markChanges();
	public:
	int row;
	MainWindow *mw;
	
};
#endif
