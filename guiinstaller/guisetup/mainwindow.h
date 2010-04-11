#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_
#include <QtGui>
#include <mpkg/libmpkg.h>
#include <mpkg-parted/mpkg-parted.h>
class QListWidgetItem;
namespace Ui {
	class MainWindowClass;
}
class QSettings;

struct CustomPkgSet {
	string name, desc, full;
};


class MountOptions {
	public:
		MountOptions(QTreeWidgetItem *item, QString _partition, QString _size, QString _currentfs, QString _mountpoint="", bool _format=false, QString _newfs="");
		~MountOptions();

		QTreeWidgetItem *itemPtr;
		QString partition;
		QString mountpoint;
		QString size;
		QString currentfs;
		bool format;
		QString newfs;
};

class MainWindow: public QMainWindow {
	Q_OBJECT
	public:
		MainWindow(QWidget *parent = 0);
		~MainWindow();
	private:
		Ui::MainWindowClass *ui;
		// Core
		mpkg *core;
		QSettings *settings;
		bool validatePageSettings(int index);
		bool validateBootloaderSettings();
		bool validateRootPassword();
		bool validateUserPasswords();
		bool validatePkgSources();
		vector<TagPair> drives;
		vector<pEntry> partitions;
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
		QString detectDVDDevice(QString isofile="");
		QString isopath;
		bool mountDVD(QString device="", bool iso=false);
		bool umountDVD();

		vector<CustomPkgSet> customPkgSetList;
		QString dvdDevice;
		string volname, rep_location;
		void getCustomSetupVariants(const vector<string>& rep_list);

		CustomPkgSet getCustomPkgSet(const string& name);
		void saveTimezone();
		void saveSetupVariant();
		void saveRootPassword();
		void saveUsers();



		
	public slots:
		void nextButtonClick();
		void backButtonClick();
		void updatePageData(int index);
		void storePageSettings(int index);

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

		void timezoneSearch(const QString &);
		void updateMountItemUI();

		void runInstaller();

	
};

enum {
	PAGE_LANGUAGE = 0,
	PAGE_LICENSE,
	PAGE_PARTITIONING,
	PAGE_MOUNTPOINTS,
	PAGE_BOOTLOADER,
	PAGE_PKGSOURCE,
	PAGE_INSTALLTYPE,
	PAGE_TIMEZONE,
	PAGE_ROOTPASSWORD,
	PAGE_USERS,
	PAGE_CONFIRMATION
};

#endif // MAINWINDOW_H_
