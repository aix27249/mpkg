#ifndef LIBAGILIASETUP_H__
#define LIBAGILIASETUP_H__

#include <mpkg/libmpkg.h>
struct CustomPkgSet {
	string name, desc, full;
};
struct PartConfig {
	string partition, mountpoint, fs, mount_options;
	bool format;
};

struct OsRecord
{
	string label;
	string type;
	string kernel;
	string kernel_options;
	string root;
};

class StatusNotifier {
	public:
		virtual void setDetailsTextCall(const string& text) = 0;
		virtual void setSummaryTextCall(const string& text) = 0;
		virtual void sendReportError(const string& text) = 0;
};

class AgiliaSetup {
	public:
		AgiliaSetup(StatusNotifier *_n = NULL);
		~AgiliaSetup();

		void registerStatusNotifier(StatusNotifier *);
		void unregisterStatusNotifier();

		void setDefaultXDM();
		void setDefaultRunlevels();
		void copyMPKGConfig();

		void setupNetwork(const string& netman, const string& hostname, const string& netname);
		void setTimezone(bool time_utc, const string& timezone);
		void umountFilesystems();

		bool createUsers(const vector<TagPair> &users);
		bool setRootPassword(const string& rootPassword);
		bool addUser(const string &username);
		bool setPasswd(const string& username, const string& passwd);
		bool createBaselayout();
		void getCustomSetupVariants(const vector<string>& rep_list);

		void setMpkgConfig(string pkgsource, const string& volname, const string& rep_location, const vector<string> additional_repositories);

		bool getRepositoryData();
		bool prepareInstallQueue(const string& setup_variant, const string& netman, const string& nvidia_driver);
		bool validateQueue();

		bool formatPartitions();
		bool mountPartitions();
		bool formatPartition(PartConfig pConfig);
		bool makeSwap(PartConfig pConfig);
		bool activateSwap(PartConfig pConfig);

		bool moveDatabase();

		void xorgSetLangConf(const string& language);
		bool processInstall(const string& pkgsource);
		void generateIssue();
		string getUUID(const string& dev); 

		void writeFstab(bool tmpfs_tmp);
		void buildInitrd(bool initrd_delay, const string& initrd_modules);

		void remapHdd(StringMap& devMap, const string& root_mbr);
		StringMap getDeviceMap(const string& mapFile);
		string getGfxPayload(const string& vgaMode);
		string mapPart(const StringMap& devMap, const string& partName, int isGrubLegacy);


		void setLocale(const string& lang);
		void setPartConfigs(const vector<PartConfig> &_partConfigs);


		vector<OsRecord> getOsList();

		bool grub2_install(const string& bootloader, const string& fbmode, const string& kernel_options);
		bool grub2_mkconfig(const string& bootloader, const string& fbmode, const string& kernel_options);
		bool grub2config(const string& bootloader, const string& fbmode, const string& kernel_options);



		bool postInstallActions(const string& language, const string& setup_variant, bool tmpfs_tmp, bool initrd_delay, const string& initrd_modules, const string& bootloader, const string& fbmode, const string& kernel_options, const string& netman, const string& hostname, const string& netname, bool time_utc, const string& timezone, const string& nvidia_driver);

		void setDefaultRunlevel(const string& lvl);
		void enablePlymouth(bool enable);
		void generateFontIndex();
		void setWM(const string& xinitrc);
		void setXwmConfig(const string& setup_variant);
		void generateLangSh(const string& lang, const string& dir="/tmp/new_sysroot/etc/profile.d/");
		void setConsoleKeymap(const string& lang);
	
		string rootPassword;
		vector<TagPair> users;



	private:
		bool setHostname(const string& hostname, const string& netname = "example.net");

		vector<CustomPkgSet> customPkgSetList;
		CustomPkgSet getCustomPkgSet(const string& name);
		StatusNotifier *notifier;
		string rootPartition, swapPartition, rootPartitionType, rootPartitionMountOptions, kernelversion;
		vector<PartConfig> partConfigs;

		string locale;
		string sysconf_lang;

};

#endif
