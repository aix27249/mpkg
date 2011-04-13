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
class HelpForm;


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
		map<string, string> settings;
		map<string, map<string, string> > partSettings;
		vector<string> repositories;

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
		void saveTimezone();
		void saveSetupVariant();
		void saveRootPassword();
		void saveUsers();
		void saveNetworking();
		void saveNvidia();
		int hasNvidia;
		map<int, bool> skipPages;
		QTranslator *translator;
		HelpForm *helpWindow;
		void initSetupVariantButtons();
		void hideAllSetupVariantButtons();
		void loadSetupVariantButton(QPushButton *, int);
		QVector<QPushButton *> setupVariantButtons; // Technically useless, but good for code compaction
		QMap<QPushButton *, int> setupVariantMap;
		size_t selectedSetupVariant;
		QString customSetupVariant;
		bool mergeCustomSetupVariant;
		QButtonGroup *svButtonGroup;
		string resolveSetupVariant(const string& s_v);

		void realShowSetupVariantDescription(const CustomPkgSet &customPkgSet);



		
	public slots:
		void showHelp();
		void loadHelp();
		void askQuit();
		void nextButtonClick();
		void backButtonClick();
		void updatePageData(int index);
		void storePageSettings(int index);
		void closeEvent(QCloseEvent *event);

		void saveConfigAndExit();
		void loadPartitioningDriveList();
		void loadMountsTree();
		void runPartitioningTool();
		void updateMountsGUI(QTreeWidgetItem *nextItem, QTreeWidgetItem *prevItem);
		void processMountEdit(QTreeWidgetItem *item);
		void loadBootloaderTree();
		void loadSetupVariants();
		void loadNetworking();
		void loadTimezones();
		void loadConfirmationData();
		bool checkNvidiaLoad();
		bool checkLoad(int page);
		void showHideReleaseNotes();

		void receiveLoadSetupVariants(bool success, const vector<CustomPkgSet> &);

		void timezoneSearch(const QString &);
		void updateMountItemUI();

		void runInstaller();
		MpkgErrorReturn errorHandler(ErrorDescription err, const string& details);
		
		void showSetupVariantDescription(QAbstractButton *);

		void mountFilterNoFormat(bool);
		void mountFilterCustom(bool);
		void mountFilterSwap(bool);
		void mountFilterRoot(bool);
		void mountFilterDontUse(bool);

		void getLoadText(const QString &);
		void getLoadProgress(int);
	
		void openCustomEdit();
	
};

enum {
	PAGE_WELCOME = 0,
	PAGE_PKGSOURCE,
	PAGE_WAITPKGSOURCE,
	PAGE_INSTALLTYPE,
	PAGE_NVIDIA,
	PAGE_PARTITIONING,
	PAGE_MOUNTPOINTS,
	PAGE_BOOTLOADER,
	PAGE_ROOTPASSWORD,
	PAGE_USERS,
	PAGE_NETWORKING,
	PAGE_TIMEZONE,
	PAGE_CONFIRMATION
};

#endif // MAINWINDOW_H_
