/* New MOPSLinux setup: ncurses-based language selection tool
*/

#include <nwidgets/ncurses_if.h>
#include <mpkgsupport/mpkgsupport.h>
#include "default_paths.h"
int main(int, char **) {
	dialogMode = true;
	CursesInterface ncInterface;
	ncInterface.setTitle(_("MOPSLinux setup"));
	ncInterface.setSubtitle(_("Language selection"));
	vector<string> availableLanguages = ReadFileStrings(SETUP_LANGUAGES);
	vector<MenuItem> menuItems;
	string arg, value;
	for (size_t i=1; i<availableLanguages.size(); i=i+2) {
		arg=availableLanguages[i-1];
		value=availableLanguages[i];
		menuItems.push_back(MenuItem(arg, value));
	}
	if (menuItems.empty()) {
		ncInterface.showMsgBox(_("No languages available to select. Choosing english (en_US) as default option"));
		WriteFile(SETUP_LANGUAGES, "en_US");
		return 0;
	}
	string language = ncInterface.showMenu2(_("Select language:"), menuItems);
	if (language.empty()) return 1;
	WriteFile(SETUPCONFIG_LANGUAGE, language);
	return 0;
}
