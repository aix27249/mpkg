/* Helper functions */

#include "helper_functions.h"
#include <mpkg-parted/mpkg-parted.h>
#include <nwidgets/ncurses_if.h>
#include "default_paths.h"
// FIXME: this function could cause pain in the ass. We need to provide some methods
string ncGetMountPartition(CursesInterface &ncInterface) {
	// Selects the partition
	vector<pEntry> pList = getPartitionList();
	vector<MenuItem> menuItems;
	for (size_t i=0; i<pList.size(); i++)
	{
		menuItems.push_back(MenuItem(pList[i].devname, pList[i].fstype + " (" + pList[i].size + "Mb)"));
	}
	int menu_ret=0;
	vector<string> tempMounts = ReadFileStrings(SETUPCONFIG_TEMPMOUNTS);
	while (menu_ret != -1) {
		menu_ret = ncInterface.showMenu(_("Choose partition to mount:"), menuItems);
		if (menu_ret == -1) return "";
		// TODO: check if partition is already mounted, and return it's mount point
		string mountcmd = "mount";
		// Creating directory
		string mPoint = "/tmp/mpkgmount/" + string(pList[menu_ret].devname);
		if (pList[menu_ret].fstype=="ntfs") mountcmd = "ntfs-3g";
		system("umount " + mPoint + " 2>/dev/null >/dev/null");
		system("mkdir -p " + mPoint);
		if (system(mountcmd + " " + string(pList[menu_ret].devname) + " " + mPoint + " > /dev/null 2>/dev/tty4")==0) {
			// TODO: check if tempMounts already contains this path
			tempMounts.push_back(pList[menu_ret].devname);
		       	tempMounts.push_back(mPoint);
			tempMounts.push_back(mountcmd);
			WriteFileStrings(SETUPCONFIG_TEMPMOUNTS, tempMounts);
			return mPoint;
		}
		else if (!ncInterface.showYesNo(_("Failed to mount partition ") + pList[menu_ret].devname + _(". Try another?"))) return "";
       }
       return "";
}


