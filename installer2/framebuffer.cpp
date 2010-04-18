/* New AgiliaLinux setup: ncurses-based bootloader selection
 * This tool only chooses bootloader type from available options
 *
*/


#include <nwidgets/ncurses_if.h>
#include <mpkgsupport/mpkgsupport.h>
#include "default_paths.h"

int main(int, char **) {
	dialogMode = true;
	CursesInterface ncInterface;
	ncInterface.setTitle(_("AgiliaLinux setup"));
	ncInterface.setSubtitle(_("Framebuffer"));
	vector<MenuItem> menuItems;
	string vgaMode;
	menuItems.push_back(MenuItem("normal", _("Generic 80x25 text console. Recommended for KMS")));
	
	menuItems.push_back(MenuItem("257", _("640x480, 256 colors")));
	menuItems.push_back(MenuItem("259", _("800x600, 256 colors")));
	menuItems.push_back(MenuItem("261", _("1024x768, 256 colors")));
	menuItems.push_back(MenuItem("263", _("1280x1024, 256 colors")));
	
	menuItems.push_back(MenuItem("273", _("640x480, 16-bit color")));
	menuItems.push_back(MenuItem("276", _("800x600, 16-bit color")));
	menuItems.push_back(MenuItem("279", _("1024x768, 16-bit color (default)")));
	menuItems.push_back(MenuItem("282", _("1280x1024, 16-bit color")));
	
	menuItems.push_back(MenuItem("274", _("640x480, 24-bit color")));
	menuItems.push_back(MenuItem("277", _("800x600, 24-bit color")));
	menuItems.push_back(MenuItem("280", _("1024x768, 24-bit color")));
	menuItems.push_back(MenuItem("283", _("1280x1024, 24-bit color")));
	
	menuItems.push_back(MenuItem(_("Other"), _("Define custom framebuffer mode")));
	do {
		vgaMode = ncInterface.showMenu2(_("Choose framebuffer graphics mode\nTo enable splash screen, you should choose 16-bit or 24-bit color mode\n"),menuItems, "279");
		if (vgaMode.empty()) return 1;
		if (vgaMode == _("Other")) {
			vgaMode = ncInterface.showInputBox(_("Enter VESA graphics mode (decimal):"));
		}
	} while (vgaMode.empty());
	WriteFile(SETUPCONFIG_FRAMEBUFFER, vgaMode);
	return 0;
}

