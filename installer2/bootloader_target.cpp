/* New MOPSLinux setup: ncurses-based bootloader selection
 * This tool only chooses bootloader type from available options
 *
*/


#include <nwidgets/ncurses_if.h>
#include <mpkgsupport/mpkgsupport.h>
#include "default_paths.h"
//#include "helper_functions.h"
#include <mpkg-parted/mpkg-parted.h>
string getTargetMBR(CursesInterface &ncInterface) {
	vector<TagPair> devList = getDevList();
	if (devList.empty()) {
			ncInterface.showMsgBox(_("Error: no hard drive detected"));
			return "";
		}

	vector<MenuItem> devMenu;
	for (size_t i=0; i<devList.size(); ++i) {
		devMenu.push_back(MenuItem(devList[i].tag, devList[i].value));
	}
	
	return ncInterface.showMenu2(_("Choose hard drive to install boot loader on it's MBR. Select first HDD if unsure:"), devMenu);

}

string getTargetPartition(CursesInterface &ncInterface) {
	vector<pEntry> partList = getPartitionList();
	if (partList.empty()) {
		ncInterface.showMsgBox(_("No partitions found to install boot loader."));
		return "";
	}
	vector<MenuItem> menuItems;

	for (size_t i=0; i<partList.size(); ++i) {
		menuItems.push_back(MenuItem(partList[i].devname, partList[i].fstype + " (" + partList[i].size + "Mb)"));
	}
	
	return ncInterface.showMenu2(_("Choose partition on hard drive, in which you want to install boot loader:"), menuItems, ReadFile(SETUPCONFIG_ROOT));

}
int main(int, char **) {
	dialogMode = true;
	CursesInterface ncInterface;
	ncInterface.setTitle(_("MOPSLinux setup"));
	ncInterface.setSubtitle(_("Bootloader HDD selection"));
	vector<MenuItem> menuItems;

	menuItems.push_back(MenuItem(_("MBR"), _("Master boot record")));
	menuItems.push_back(MenuItem(_("Partition"), _("Hard drive partition")));

	string target_type;
	string target;
	do {
		target_type = ncInterface.showMenu2(_("Choose where do you want to install boot loader"),menuItems, "MBR");
		if (target_type.empty()) return 1;
		if (target_type==_("MBR")) target=getTargetMBR(ncInterface);
		else target=getTargetPartition(ncInterface);
	} while (target.empty());

	WriteFile(SETUPCONFIG_BOOTLOADER_TARGET, target);

	return 0;
}

