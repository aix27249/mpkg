/* New MOPSLinux setup: ncurses-based partition editor selection tool
*/

#include <nwidgets/ncurses_if.h>
#include <mpkgsupport/mpkgsupport.h>
#include "default_paths.h"
#include <mpkg-parted/mpkg-parted.h>
string getPartitionEditor(CursesInterface &ncInterface) {
	vector<string> partitionEditors = ReadFileStrings(PARTITION_EDITORS);
	vector<MenuItem> menuItems;
	string arg, value;
	for (size_t i=1; i<partitionEditors.size(); i=i+2) {
		arg=partitionEditors[i-1];
		value=partitionEditors[i];
		menuItems.push_back(MenuItem(arg, value));
	}
	if (menuItems.empty()) {
		return _("Continue");
	}
	menuItems.push_back(MenuItem(_("Continue"), _("All done")));

	return ncInterface.showMenu2(_("Choose partition editor:"), menuItems);

}


int main(int, char **) {
	dialogMode = true;
	CursesInterface ncInterface;
	ncInterface.setTitle(_("MOPSLinux setup"));
	ncInterface.setSubtitle(_("Disk partitioning"));

	// There are three options: auto, manual, and do_nothing.
	// Auto can be actually performed after confirmation, and requires:
	// 	HDD selection
	//	Partitioning model (e.g. desktop, server, laptop, etc - I don't know what to choose here)
	//	Utility that will perform all this stuff (still not implemented)
	//
	// Manual partitioning, in theory, also can be actually performed after confirmation, pushing the partition scheme to mparted.
	// But unfortunately there is no support for such nasty things neither in nwidgets, nor in mparted, so user have to perform partitioning
	// here and now. But he can choose partitioning tool :)
	//
	// Do nothing means do nothing. Here is very simple option.
	
	// NOTE: ALL THIS STUFF IS SUBJECT TO CHANGE TO CONFORM GREAT FUTURE CONCEPTS :)
	
	// For now, choose and run selected partitioning tool, or do nothing
	
	// First of all, get device list
	
	vector<TagPair> devList = getDevList();
	vector<MenuItem> devMenu;
	for (size_t i=0; i<devList.size(); ++i) {
		devMenu.push_back(MenuItem(devList[i].tag, devList[i].value));
	}
	devMenu.push_back(MenuItem(_("Continue"), _("All done")));

	// Loop menu
	string parteditor;
	string device;
       	do {
		parteditor = getPartitionEditor(ncInterface);
		if (parteditor.empty()) return 1;
		if (parteditor==_("Continue")) return 0;
		if (parteditor=="mparted") system("mparted");
		else {
			do {
				device = ncInterface.showMenu2(_("Select hard drive for partitioning:"), devMenu);
				if (device.empty()) break;
				if (device==_("Continue")) break;
				ncInterface.uninit();
				system(parteditor + " " + device);
			} while(!device.empty());

		}
	} while (parteditor!=_("Continue"));
       	return 0;
	
	

	



};
