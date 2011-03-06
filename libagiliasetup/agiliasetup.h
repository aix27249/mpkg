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


class StatusNotifier {
	public:
		virtual void setDetailsTextCallback(const string& text) = 0;
		virtual void sendReportError(const string& text) = 0;
};

class AgiliaSetup {
	public:
		AgiliaSetup();
		~AgiliaSetup();

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
		void getCustomSetupVariants(const vector<string>& rep_list, StatusNotifier *notifier = NULL);

		void setMpkgConfig(string pkgsource, const string& volname, const string& rep_location, const vector<string> additional_repositories);

		bool getRepositoryData(StatusNotifier *notifier = NULL);
		bool prepareInstallQueue(const string& setup_variant, const string& netman, const string& nvidia_driver, StatusNotifier *notifier = NULL);
		bool validateQueue(StatusNotifier *notifier = NULL);

		bool formatPartition(PartConfig pConfig, StatusNotifier *notifier = NULL);
		bool makeSwap(PartConfig pConfig, StatusNotifier *notifier = NULL);
		bool activateSwap(PartConfig pConfig);



		void setLocale(const string& lang);

	private:
		bool setHostname(const string& hostname, const string& netname = "example.net");

		vector<CustomPkgSet> customPkgSetList;
		CustomPkgSet getCustomPkgSet(const string& name);

		string locale;
};

#endif
