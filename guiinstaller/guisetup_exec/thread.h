#ifndef GUISETUP_THREAD_H__
#define GUISETUP_THREAD_H__
#include <QtGui>
#include <QThread>
#include <mpkg/libmpkg.h>
#include <agiliasetup.h>

#include <mpkg/errorhandler.h>
struct OsRecord
{
	string label;
	string type;
	string kernel;
	string kernel_options;
	string root;
};




class SetupThread: public QThread, public StatusNotifier {
	Q_OBJECT
	public:
		void run();
		void setDetailsTextCallback(const string& msg);
		void sendReportError(const string& text);
	private:
		// Core
		mpkg *core;
		QSettings *settings;
		bool formatPartition(PartConfig pConfig);
		bool makeSwap(PartConfig pConfig);
		bool activateSwap(PartConfig pConfig);
		string sysconf_lang;

		string rootPartition, swapPartition, rootPartitionType, rootPartitionMountOptions, kernelversion;
		vector<PartConfig> partConfigs;

		QString rootPassword;
		vector<TagPair> users;
		MpkgErrorReturn errCode;
		AgiliaSetup agiliaSetup;


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
		bool fillPartConfigs();
		bool validateConfig();
		bool setMpkgConfig();
		bool getRepositoryData();
		bool prepareInstallQueue();
		bool validateQueue();
		bool formatPartitions();
		bool mountPartitions();
		bool moveDatabase();
		bool createBaselayout();
		bool processInstall();
		bool postInstallActions();
		void updateData(const ItemState& a);

		void xorgSetLangConf();
		void generateIssue();
		void writeFstab();
		void buildInitrd();
		bool grub2_install();
		bool grub2config();
		bool grub2_mkconfig();
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
