#ifndef TEXTSETUP_H__
#define TEXTSETUP_H__

//#include <iniparser.h>
#include <mpkg/libmpkg.h>
#include "custompkgset.h"
#include "mechanics.h"

class TextSetup {
	public: 
		TextSetup(string _distro_version="8.0");
		~TextSetup();

		mpkg *core;
		int setPackageSource();
		int setInstallType();
		int setNvidiaDriver();
		int setPartitionEditor();
		int setMountPoints();
		int setBootLoader();
		int setRootPassword();
		int setCreateUser();
		int setNetworkSettings();
		int setTimezone();

		int saveSettings();

		string getISORepoPath();
		string getHDDRepoPath();
		string getCustomRepoPath();

		int run();

	private:
		map<string, string> settings;
		map<string, vector<string> > vectorSettings;
		map<string, map<string, string> > mapSettings;

		vector<CustomPkgSet> customPkgSetList;
		vector<MenuItem> getKnownFilesystems();
		TextSetupMechanics mech;

		string distro_version;

};


#endif
