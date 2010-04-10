/* New MOPSLinux setup: ncurses-based bootloader selection
 * This tool only chooses bootloader type from available options
 *
*/


#include <nwidgets/ncurses_if.h>
#include <mpkgsupport/mpkgsupport.h>
#include "default_paths.h"

int main(int, char **) {
	dialogMode = true;
	CursesInterface ncInterface;
	ncInterface.setTitle(_("MOPSLinux setup"));
	ncInterface.setSubtitle(_("Bootloader selection"));
	vector<MenuItem> menuItems;
	menuItems.push_back(MenuItem("GRUB2", _("GRand Unified Bootloader 2 (default)")));
	menuItems.push_back(MenuItem("LILO", "LInux LOader"));
	menuItems.push_back(MenuItem("None", _("Do not install boot loader")));
	string bootLoader = ncInterface.showMenu2(_("Choose boot loader type.\nIf unsure - choose default:"), menuItems);
	if (bootLoader.empty()) return 1;
	WriteFile(SETUPCONFIG_BOOTLOADER, bootLoader);
	return 0;
}

