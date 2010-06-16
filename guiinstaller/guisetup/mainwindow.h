#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_
#include <QtGui>
#include <mpkg/libmpkg.h>
#include <mpkg/errorhandler.h>
#include <mpkg-parted/mpkg-parted.h>
#include <mpkg-parted/raidtool.h>
#include <mpkg-parted/lvmtool.h>
#include "thread.h"
class QListWidgetItem;
class QTranslator;
namespace Ui {
	class MainWindowClass;
}
class QSettings;

struct CustomPkgSet {
	string name, desc, full;
	uint64_t csize, isize;
	size_t count;
};


class MountOptions {
	public:
		MountOptions(QTreeWidgetItem *item, QString _partition, uint64_t _psize, QString _size, QString _currentfs, QString _mountpoint="", bool _format=false, QString _newfs="");
		~MountOptions();

		QTreeWidgetItem *itemPtr;
		QString partition;
		QString mountpoint;
		uint64_t psize;
		QString size;
		QString currentfs;
		bool format;
		QString newfs;
		QString mount_options;
};

class MainWindow: public QMainWindow {
	Q_OBJECT
	public:
		MainWindow(QWidget *parent = 0);
		~MainWindow();
	private:
		Ui::MainWindowClass *ui;
		// Core
		//mpkg *core;
		LoadSetupVariantsThread *loadSetupVariantsThread;
		QSettings *settings;
		bool validatePageSettings(int index);
		bool validateBootloaderSettings();
		bool validateRootPassword();
		bool validateUserPasswords();
		bool validatePkgSources();
		bool validateMountPoints();
		bool validateNetworking();
		vector<TagPair> drives;
		vector<pEntry> partitions;
		vector<LVM_VG> lvm_groups;
		QString lastPart;

		void updatePartitionLists();
		bool lockUIUpdate;
		bool mountsReady;
		vector<MountOptions> mountOptions;
		vector<QTreeWidgetItem *> bootLoaderItems;
		vector<QString> bootLoaderPartitions;

		void saveMountSettings();
		void saveBootloaderSettings();
		void savePkgsourceSettings();
		QString isopath, hddpath, urlpath;

		vector<CustomPkgSet> customPkgSetList;
		void getCustomSetupVariants(const vector<string>& rep_list);

		CustomPkgSet getCustomPkgSet(const string& name);
		void saveTimezone();
		void saveSetupVariant();
		void saveRootPassword();
		void saveUsers();
		void saveNetworking();
		void saveNvidia();
		void saveAlternatives();
		int hasNvidia;
		map<int, bool> skipPages;
		QTranslator *translator;



		
	public slots:
		void showHelp();
		void askQuit();
		void nextButtonClick();
		void backButtonClick();
		void updatePageData(int index);
		void storePageSettings(int index);
		void closeEvent(QCloseEvent *event);

		void saveConfigAndExit();
		void loadLicense();
		void loadPartitioningDriveList();
		void loadMountsTree();
		void runPartitioningTool();
		void updateMountsGUI(QTreeWidgetItem *nextItem, QTreeWidgetItem *prevItem);
		void processMountEdit(QTreeWidgetItem *item);
		void loadBootloaderTree();
		void loadSetupVariants();
		void loadTimezones();
		void loadConfirmationData();
		bool checkNvidiaLoad();
		bool checkLoad(int page);
		void showHideReleaseNotes();

		void receiveLoadSetupVariants(bool success);

		void timezoneSearch(const QString &);
		void updateMountItemUI();

		void runInstaller();
		MpkgErrorReturn errorHandler(ErrorDescription err, const string& details);
		
		void showSetupVariantDescription(int);
		void calculatePkgSetSize(CustomPkgSet &set);

		void mountFilterNoFormat(bool);
		void mountFilterCustom(bool);
		void mountFilterSwap(bool);
		void mountFilterRoot(bool);
		void mountFilterDontUse(bool);
	
};

enum {
	PAGE_WELCOME = 0,
	PAGE_LICENSE,
	PAGE_NETWORKING,
	PAGE_PKGSOURCE,
	PAGE_WAITPKGSOURCE,
	PAGE_INSTALLTYPE,
	PAGE_ALTERNATIVES,
	PAGE_NVIDIA,
	PAGE_PARTITIONING,
	PAGE_MOUNTPOINTS,
	PAGE_BOOTLOADER,
	PAGE_ROOTPASSWORD,
	PAGE_USERS,
	PAGE_TIMEZONE,
	PAGE_CONFIRMATION
};

#endif // MAINWINDOW_H_
