/* New AgiliaLinux setup: ncurses-based swap selection tool
 * 
*/

#include <nwidgets/ncurses_if.h>
#include <mpkgsupport/mpkgsupport.h>
#include "default_paths.h"
#include <mpkg-parted/mpkg-parted.h>

int main(int, char **) {
	dialogMode = true;
	CursesInterface ncInterface;
	ncInterface.setTitle(_("AgiliaLinux setup"));
	ncInterface.setSubtitle(_("Swap partition selection"));
	vector<pEntry> swapList = getPartitionList();
	if (swapList.empty()) {
		ncInterface.showMsgBox(_("No partitions found for swap space. Create it first."));
		return 1;
	}

	vector<MenuItem> menuItems;
	int def_id=0;
	for (size_t i=0; i<swapList.size(); i++) {
		menuItems.push_back(MenuItem(swapList[i].devname, swapList[i].fstype + " (" + swapList[i].size + "Mb)"));
		if (swapList[i].fstype.find("swap")!=std::string::npos) def_id=i;
	}
	menuItems.push_back(MenuItem(_("Skip"), _("Continue without swap partition"), _("If you have lots of RAM (2Gb or more), and you sure that you will never use all of this - you can continue without swap. Please note, that you will also loose ability of suspending to disk, because it uses swapspace for data storage.")));

	string swapPartition;
	int swapSelectNum;
	bool repeat;
	do {
		repeat = false;
		swapSelectNum = ncInterface.showMenu(_("Please, choose swap partition"), menuItems, def_id);
		
		if (swapSelectNum<0) return 1;
		if (swapSelectNum>=(int)swapList.size()) swapPartition="";
		if (swapList[swapSelectNum].fstype.find("linux-swap")==std::string::npos && swapList[swapSelectNum].fstype!="unformatted") {
			// Seems that we are trying to overwrite some filesystem, maybe important...
			if (!ncInterface.showYesNo(_("You are going to create swapspace on partition ") + \
				swapList[swapSelectNum].devname + ", which already has a filesystem " + swapList[swapSelectNum].fstype + \
				". Are you sure you really want to do this?"))	repeat = true;
		}
		if (!repeat) swapPartition = swapList[swapSelectNum].devname;
	} while (repeat);
	
	WriteFile(SETUPCONFIG_SWAP, swapPartition);
	return 0;
}

