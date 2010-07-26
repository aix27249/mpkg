#ifndef GUISETUP_THREAD_H__
#define GUISETUP_THREAD_H__
#include <QtGui>
#include <QThread>
#include <mpkg/libmpkg.h>

#include <mpkg/errorhandler.h>
/*struct SysConfig
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
};*/
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
	string partition, mountpoint, fs, mount_options;
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

		string rootPartition, swapPartition, rootPartitionType, rootPartitionMountOptions, kernelversion;
		vector<PartConfig> partConfigs;

		QString rootPassword;
		vector<TagPair> users;
		MpkgErrorReturn errCode;

	signals:
		void setSummaryText(const QString &);
		void setDetailsText(const QString &);
		void setProgressMax(int);
		void setProgress(int);
		void reportError(const QString &);
		void reportFinish();
		void minimizeWindow();
		void maximizeWindow();
		void showMD5Button(bool);
		void enableMD5Button(bool);
		void sendErrorHandler(ErrorDescription, const QString &);

	public slots:
		void skipMD5();

		MpkgErrorReturn errorHandler(ErrorDescription err, const string& details);
		void receiveErrorResponce(MpkgErrorReturn);
		vector<OsRecord> getOsList();

		void setDefaultRunlevels();
		void setDefaultXDM();
		void getCustomSetupVariants(const vector<string>& rep_list);
		CustomPkgSet getCustomPkgSet(const string& name);
		bool fillPartConfigs();
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
		void xorgSetLangConf();
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
