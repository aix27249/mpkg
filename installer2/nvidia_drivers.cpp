#include <mpkgsupport/mpkgsupport.h>
#include <nwidgets/ncurses_if.h>
#include "default_paths.h"
void loadAltDescriptions(vector<MenuItem> *altMenu) {
	for (size_t i=0; i<altMenu->size(); ++i) {
		if (altMenu->at(i).tag=="bfs") altMenu->at(i).value=_("BFS kernel");
		else if (altMenu->at(i).tag=="cleartype") altMenu->at(i).value=_("Cleartype-patched fonts");
	}
}

int driverSelectionMenu(CursesInterface &ncInterface) {
	// First, detect hardware for which we have to install drivers.
	string tmp_hw = get_tmp_file();
	system("lspci > " + tmp_hw);
	vector<string> hw = ReadFileStrings(tmp_hw);
	// Check for VirtualBox
	bool hasVirtualBox=false;
	int hasNvidia=-1;
	int hasRadeon=-1;
	for(size_t i=0; i<hw.size(); ++i) {
		if (hw[i].find("VirtualBox")!=std::string::npos) hasVirtualBox = true;
		if (hw[i].find("VGA compatible controller")!=std::string::npos && hw[i].find("nVidia")!=std::string::npos) hasNvidia = i;
		if (hw[i].find("VGA compatible controller")!=std::string::npos && hw[i].find("Radeon")!=std::string::npos) hasRadeon = i;
	}
	if (hasNvidia==-1) return 1;
	vector<MenuItem> menuItems;
	menuItems.push_back(MenuItem(_("Latest"), _("Latest GeForce (series 6xxx and later)")));
	menuItems.push_back(MenuItem("173", _("GeForce FX series")));
	menuItems.push_back(MenuItem("96", _("GeForce 4 series")));
	menuItems.push_back(MenuItem(_("Skip"), _("Do not install driver")));

	string ret = ncInterface.showMenu2(_("Videocard detected: ") + hw[hasNvidia] + _("\nChoose driver for it:"), menuItems);
	if (!ret.empty() && ret != _("Skip")) {
		if (ret!=_("Latest")) {
			WriteFile(SETUPCONFIG_NVIDIA, string("legacy" + ret));
		}
		else {
			WriteFile(SETUPCONFIG_NVIDIA, "generic");
			
		}
	}
	return 0;
}




int main() {
	dialogMode = true;
	CursesInterface ncInterface;
	driverSelectionMenu(ncInterface);

}
