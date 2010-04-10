/* New MOPSLinux setup: ncurses-based package source selection tool
 *
*/


#include <nwidgets/ncurses_if.h>
#include <mpkgsupport/mpkgsupport.h>
#include "default_paths.h"
#include "helper_functions.h"

int main(int, char **) {
	dialogMode = true;
	CursesInterface ncInterface;
	ncInterface.setTitle(_("MOPSLinux setup"));
	ncInterface.setSubtitle(_("Package source selection"));
	vector<MenuItem> menuItems;
	string ret;
	vector<string> repository_paths;
	menuItems.push_back(MenuItem("DVD", _("Install from DVD")));
	menuItems.push_back(MenuItem("HDD", _("Install from hard drive")));
	menuItems.push_back(MenuItem("Network", _("Install from network repository (FTP, HTTP)")));
	menuItems.push_back(MenuItem("ISO", _("Install from ISO image on HDD")));
	//menuItems.push_back(MenuItem("Samba", _("Install from Samba (Windows network)"))); // In theory, it can work, but it is better to mount this manually.
	ret = ncInterface.showMenu2(_("Select a package source to install system from it:"), menuItems);
	if (ret.empty()) return 1;
	// Let's ask user for some data
	if (ret=="DVD") repository_paths.push_back("cdrom://"); // Note that we do not provide full path to CDROM packages here, it is intended to do by setup itself.
	else if (ret=="HDD") {
		// We need to ask if user wants to mount some partition, and then ask for path.
		string base;
		if (ncInterface.showYesNo(_("Do you want to mount partition with packages?"))) base = ncGetMountPartition(ncInterface);
		if (base.empty()) base="/";
		ret = ncInterface.ncGetOpenDir(base);
		if (ret.empty()) return 1;
		repository_paths.push_back("file://" + ret);
	}
	else if (ret=="Network") {
		#ifdef X86_64
			string default_url="http://core64.mopspackages.ru/";
		#else
			string default_url="http://core32.mopspackages.ru/";
		#endif
		repository_paths.push_back(default_url);
		ncInterface.showMsgBox(_("Set package source to: ") + default_url);
	}
	else if (ret=="ISO") {
		string base;
		if (ncInterface.showYesNo(_("Do you want to mount partition with ISO image?"))) base = ncGetMountPartition(ncInterface);
		if (base.empty()) base="/";
		ret = ncInterface.ncGetOpenFile(base);
		if (ret.empty()) return 1;
		repository_paths.push_back("iso://" + ret);
	}
	
	WriteFileStrings(SETUPCONFIG_PKGSOURCE, repository_paths);
	return 0;

}
