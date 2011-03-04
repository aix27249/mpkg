#include "textsetup.h"
#include "mediachecker.h"
#include "mechanics.h"

TextSetup::TextSetup(string _distro_version) {
	distro_version = _distro_version;
}

TextSetup::~TextSetup() {
}

int TextSetup::run() {
	int ret;
	for (int i=0; i<9; ++i) {
		switch(i) {
			case 0: ret = setPackageSource(); break;
			case 1: ret = setInstallType(); break;
			case 2: ret = setPartitionEditor(); break;
			case 3: ret = setMountPoints(); break;
			case 4: ret = setBootLoader(); break;
			case 5: ret = setRootPassword(); break;
			case 6: ret = setCreateUser(); break;
			case 7: ret = setNetworkSettings(); break;
			case 8: ret = setTimezone(); break;
			default: return -1;
		}
		if (ret!=0) i=i-2;
		if (i<0) return -1;
	}
	return 0;
}

int TextSetup::setPackageSource() {
	vector<MenuItem> m;
	m.push_back(MenuItem("Disc", _("Install from installation DVD or USB flash")));
	m.push_back(MenuItem("Network", _("Install from official network repository")));
	m.push_back(MenuItem("ISO", _("Install from ISO image")));
	m.push_back(MenuItem("HDD", _("Install from directory on local hard drive")));
	m.push_back(MenuItem("Custom", _("Specify custom repository set")));
	
	settings["pkgsource"] = ncInterface.showMenu2(_("Please, choose package source from a list below:"), m, settings["pkgsource"]);
	
	string repo, ret_repo, volname, rep_location;
	if (settings["pkgsource"].empty()) return 1;
	if (settings["pkgsource"]=="Disc") repo="dvd";
	if (settings["pkgsource"]=="Network") repo="http://core.agilialinux.ru/" + distro_version + "/";
	if (settings["pkgsource"]=="ISO") repo=getISORepoPath();
	if (settings["pkgsource"]=="HDD") repo=getHDDRepoPath();
	if (settings["pkgsource"]=="Custom") repo=getCustomRepoPath();
	



	customPkgSetList = mech.getCustomPkgSetList(repo, &ret_repo, &volname, &rep_location);
	settings["pkgsource"] = ret_repo;
	settings["volname"] = volname;
	settings["rep_location"] = rep_location;
	
	if (customPkgSetList.empty()) {
		if (ncInterface.showYesNo(_("Failed to retrieve required data from specified repository. Select another one?"))) return setPackageSource();
		else return 2;
	}

	return 0;
}

int TextSetup::setInstallType() {
	vector<MenuItem> m;

	for (size_t i=0; i<customPkgSetList.size(); ++i) {
		m.push_back(MenuItem(customPkgSetList[i].name, customPkgSetList[i].desc));
	}

	settings["pkgset"] = ncInterface.showMenu2(_("Select installation type:"), m, settings["pkgset"]);

	if (settings["pkgset"].empty()) return 1;

	setNvidiaDriver();
	return 0;
}

int TextSetup::setNvidiaDriver() {
	// Check if we need it
	return 0;
}

int TextSetup::setPartitionEditor() {
	return 0;
}

int TextSetup::setMountPoints() {
	return 0;
}

int TextSetup::setBootLoader() {
	return 0;
}

int TextSetup::setRootPassword() {
	return 0;
}

int TextSetup::setCreateUser() {
	return 0;
}

int TextSetup::setNetworkSettings() {
	return 0;
}

int TextSetup::setTimezone() {
	return 0;
}

