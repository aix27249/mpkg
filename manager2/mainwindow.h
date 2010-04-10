#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_
#include <QtGui/QMainWindow>
#include <QtGui/QListWidgetItem>
#include <mpkg/libmpkg.h>
enum {
	ICONSTATE_UNKNOWN = 0,
	ICONSTATE_INSTALLED,
	ICONSTATE_INSTALL,
	ICONSTATE_AVAILABLE,
	ICONSTATE_REMOVE,
	ICONSTATE_INSTALLED_DEPRECATED,
	ICONSTATE_AVAILABLE_DEPRECATED,
	ICONSTATE_UPDATE
};
class QListWidgetItem;
class LoadTags;
class LoadPackages;
class LoadUpdateRepositoryData;
class CommitActions;
class CommitDialog;
class SettingsDialog;
class ProgressWidget;
namespace Ui {
	class MainWindowClass;
}


class MainWindow: public QMainWindow {
	Q_OBJECT
	public:
		MainWindow(QWidget *parent = 0);
		~MainWindow();
	private:
		Ui::MainWindowClass *ui;
		void connectSlots();
		void disconnectSlots();
		void connectThreadSlots();
		// Threads
		LoadTags *loadTagsThread;
		LoadPackages *loadPackagesThread;
		LoadUpdateRepositoryData *updateRepositoryDataThread;
		CommitActions *commitActionsThread;

		// Core
		mpkg *core;
		bool slotsConnected;

		// Additional windows
		CommitDialog *commitDialog;
		SettingsDialog *settingsDialog;

		bool checkSearchFilter(const QString&);
		PACKAGE_LIST globalPkgList;
		QVector<QPixmap> pixmapList;
		void loadPixmapList();
		QListWidgetItem * createItemForPkg(const PACKAGE& p);
		int getIconState(const PACKAGE& p);

	public slots:
		void tagsLoadingComplete();
		void packagesLoadingComplete();
		void showPackageInfo(int);
		void receiveCategoryFilter(QListWidgetItem *current, QListWidgetItem *previous);
		void showSettings();
		void updateRepositoryData();
		void doneUpdateRepositoryData();
		void requestCommit();
		void loadPackageList();
		void commitActions();
		void commitFinished();
		void applySearchFilter();
		void markAllVisible();
		void unmarkAllVisible();
		void resetQueue();
		void cleanCache();
		void getRepositoryList();
		void loadSettings();
		void saveSettings();
		void quit();
};

#endif // MAINWINDOW_H_
