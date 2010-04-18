/* New AgiliaLinux setup: ncurses-based root partition selection tool
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
	ncInterface.setSubtitle(_("Root partition selection"));
	vector<pEntry> rootList = getPartitionList();
	if (rootList.empty()) {
		ncInterface.showMsgBox(_("No partitions found for root filesystem. Create it first."));
		return 1;
	}

	string swapPartition = ReadFile(SETUPCONFIG_SWAP);
	vector<MenuItem> menuItems;
	int def_id=0;
	for (size_t i=0; i<rootList.size(); i++) {
		menuItems.push_back(MenuItem(rootList[i].devname, rootList[i].fstype + " (" + rootList[i].size + "Mb)"));
		if (rootList[i].devname==swapPartition) continue;
	}

	string rootPartition;
	int rootSelectNum;
	rootSelectNum = ncInterface.showMenu(_("Choose root partition for AgiliaLinux:"), menuItems, def_id);
		
	if (rootSelectNum<0) return 1;
	rootPartition = rootList[rootSelectNum].devname;
	
	WriteFile(SETUPCONFIG_ROOT, rootPartition);
	return 0;
}

