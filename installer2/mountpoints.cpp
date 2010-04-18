/* New AgiliaLinux setup: ncurses-based partition mountting tool
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
	ncInterface.setSubtitle(_("Partition mountting"));
	vector<pEntry> partList = getPartitionList();
	if (partList.empty()) {
		ncInterface.showMsgBox(_("No partitions found to mount."));
		return 1;
	}
	vector<MenuItem> menuItems;
	vector<string> mount;

	for (size_t i=0; i<partList.size(); ++i) {
		menuItems.push_back(MenuItem(partList[i].devname, partList[i].fstype + " (" + partList[i].size + "Mb)"));
		mount.push_back("");
	}

	menuItems.push_back(MenuItem(_("Continue"), _("All done, continue")));
	string swapPartition = ReadFile(SETUPCONFIG_SWAP);
	string rootPartition = ReadFile(SETUPCONFIG_ROOT);
	int partNum;
	string mountPoint;
	do {
		partNum = ncInterface.showMenu(_("Choose partition to set mount point"), menuItems);
		if (partNum>= (int) mount.size()) break;
		if (partNum==-1) return 1;
		if (partList[partNum].devname==swapPartition) {
			continue;
		}
		if (partList[partNum].devname==rootPartition) {
			continue;
		}

		mountPoint = ncInterface.showInputBox(_("Choose mount point for partition ") + partList[partNum].devname + ":");
		if (mountPoint.empty()) continue;
		mount[partNum]=mountPoint;
		if (mount[partNum].empty()) menuItems[partNum].value = partList[partNum].fstype + " (" + partList[partNum].size + "Mb)";
		else menuItems[partNum].value=partList[partNum].fstype + " (" + partList[partNum].size + string("Mb), ") + _("mount to: ") + mount[partNum];
	} while (true);

	vector<string> data;
	for (size_t i=0; i<partList.size(); ++i) {
		if (!mount[i].empty()) {
			data.push_back(partList[i].devname);
			data.push_back(mount[i]);
		}
	}
	WriteFileStrings(SETUPCONFIG_MOUNT, data);
	return 0;
}

