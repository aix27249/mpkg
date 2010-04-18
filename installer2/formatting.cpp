/* New AgiliaLinux setup: ncurses-based partition formatting tool
 *
*/


#include <nwidgets/ncurses_if.h>
#include <mpkgsupport/mpkgsupport.h>
#include "default_paths.h"
#include <mpkg-parted/mpkg-parted.h>
string chooseFormat(CursesInterface &ncInterface, const vector<pEntry> &partList, const int partNum) {
	vector<MenuItem> formatOptions;
	formatOptions.push_back(MenuItem("ext4", _("4th version of standard linux filesystem"), _("New version of EXT, developed to superseed EXT3. The fastest journaling filesystem in most cases.")));
	formatOptions.push_back(MenuItem("ext3", _("Standard journaling Linux filesystem"), _("Seems to be a best balance between reliability and performance")));
	formatOptions.push_back(MenuItem("ext2", _("Standard Linux filesystem (without journaling)."), _("Old, stable, very fast (it doesn't use jounal), but less reliable")));
	formatOptions.push_back(MenuItem("xfs", _("Fast filesystem developed by SGI"), _("The best choise to store data (especially large files). Supports online defragmentation and other advanced features. Not recommended to use as root FS or /home")));
	formatOptions.push_back(MenuItem("jfs", _("Journaling filesystem by IBM"), _("Another good filesystem for general use.")));
	formatOptions.push_back(MenuItem("reiserfs", _("Journaling filesystem developed by Hans Reiser"), _("ReiserFS is a general-purpose, journaled computer file system designed and implemented by a team at Namesys led by Hans Reiser. ReiserFS is currently supported on Linux. Introduced in version 2.4.1 of the Linux kernel, it was the first journaling file system to be included in the standard kernel")));
	formatOptions.push_back(MenuItem("btrfs", _("B-Tree FS, new EXPERIMENTAL filesystem similar to ZFS. Separate /boot partition required to use it as root filesystem.")));
	formatOptions.push_back(MenuItem("nilfs2", _("NILFS2, new EXPERIMENTAL snapshot-based filesystem. Separate /boot partition required to use it as root filesystem.")));
	formatOptions.push_back(MenuItem("---", _("Do not format (use as is and keep data)")));
	return ncInterface.showMenu2(_("Choose how to format partition ") + partList[partNum].devname + ":", formatOptions);

}
int main(int, char **) {
	dialogMode = true;
	CursesInterface ncInterface;
	ncInterface.setTitle(_("AgiliaLinux setup"));
	ncInterface.setSubtitle(_("Partition formatting"));
	vector<pEntry> partList = getPartitionList();
	if (partList.empty()) {
		ncInterface.showMsgBox(_("No partitions found to format."));
		return 1;
	}
	vector<MenuItem> menuItems;
	vector<string> format;

	for (size_t i=0; i<partList.size(); ++i) {
		menuItems.push_back(MenuItem(partList[i].devname, partList[i].fstype + " (" + partList[i].size + "Mb)"));
		format.push_back("");
	}

	menuItems.push_back(MenuItem(_("Continue"), _("All done, continue")));
	
	int partNum;
	string formatOption;
	do {
		partNum = ncInterface.showMenu(_("Choose partition and select formatting options"), menuItems);
		if (partNum>= (int) format.size()) break;
		if (partNum==-1) return 1;

		formatOption = chooseFormat(ncInterface, partList, partNum);
		if (formatOption.empty()) continue;
		if (formatOption=="---") formatOption.clear();
		format[partNum]=formatOption;
		if (format[partNum].empty()) menuItems[partNum].value = partList[partNum].fstype + " (" + partList[partNum].size + "Mb)";
		else menuItems[partNum].value=partList[partNum].fstype + " (" + partList[partNum].size + string("Mb), ") + _("format to: ") + format[partNum];
	} while (true);

	vector<string> data;
	for (size_t i=0; i<partList.size(); ++i) {
		if (!format[i].empty()) {
			data.push_back(partList[i].devname);
			data.push_back(format[i]);
		}
	}
	WriteFileStrings(SETUPCONFIG_FORMATTING, data);
	return 0;
}


