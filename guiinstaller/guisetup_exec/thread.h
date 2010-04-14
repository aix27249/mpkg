#ifndef GUISETUP_THREAD_H__
#define GUISETUP_THREAD_H__
#include <QtGui>
#include <QThread>
#include <mpkg/libmpkg.h>

struct SysConfig
{
	string swapPartition;
	string rootPartition;
	string rootPartitionType;
	bool rootPartitionFormat;
	vector<TagPair>otherMounts;
	vector<string>otherMountFSTypes;
	vector<string>oldOtherFSTypes;
	vector<string>otherMountSizes;
	vector<bool>otherMountFormat;
	string rootMountPoint;
	string cdromDevice;
	string sourceName;
	unsigned int totalQueuedPackages, totalDependantPackages;
	unsigned long long totalDownloadSize, totalInstallSize;
	string setupMode;
	int setupModeI;
	string totalRequiredSpace;
	vector<TagPair> tmpMounts;
	string lang;
	bool tryPrelink;
	string kernelversion;
	bool tmpfs_tmp;
};

struct BootConfig
{
	string kernelOptions;
	string bootDevice;
	string videoModeName;
	unsigned int videoModeNumber;
	string rootFs;
	string loaderType;
};
struct OsRecord
{
	string label;
	string type;
	string kernel;
	string kernel_options;
	string root;
};

struct CustomPkgSet {
	string name, desc, full;
};

struct PartConfig {
	string partition, mountpoint, fs;
	bool format;
};

class SetupThread: public QThread {
	Q_OBJECT
	public:
		void run();
	private:
		// Core
		mpkg *core;
		QSettings *settings;
		vector<CustomPkgSet> customPkgSetList;
		bool formatPartition(PartConfig pConfig);
		bool makeSwap(PartConfig pConfig);
		bool activateSwap(PartConfig pConfig);
		string sysconf_lang;

		string rootPartition, swapPartition, rootPartitionType, kernelversion;

		QString rootPassword;
		vector<TagPair> users;

	signals:
		void setSummaryText(const QString &);
		void setDetailsText(const QString &);
		void setProgressMax(int);
		void setProgress(int);
		void reportError(const QString &);
		void reportFinish();
		void minimizeWindow();
		void maximizeWindow();
	public slots:
		void getCustomSetupVariants(const vector<string>& rep_list);
		CustomPkgSet getCustomPkgSet(const string& name);
		bool validateConfig();
		bool setMpkgConfig();
		bool getRepositoryData();
		bool prepareInstallQueue();
		bool validateQueue();
		bool formatPartitions();
		bool mountPartitions();
		bool moveDatabase();
		bool processInstall();
		bool postInstallActions();
		void updateData(const ItemState& a);

		void xorgSetLangHALEx();
		void generateIssue();
		void writeFstab();
		void buildInitrd();
		bool grub2config();
		bool setHostname();
		void setDefaultRunlevel(const string &);
		void enablePlymouth(bool);
		void generateFontIndex();
		void setXwmConfig();

		void setRootPassword();
		void createUsers();
		void setTimezone();
		void setupNetwork();

		void copyMPKGConfig();

		void umountFilesystems();

};

#endif
