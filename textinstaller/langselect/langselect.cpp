#include <nwidgets/ncurses_if.h>
#include <cstdlib>
#include <mpkgsupport/mpkgsupport.h>
#include <agiliasetup.h>

int main() {
	CursesInterface ncInterface;
	setlocale(LC_ALL, "");
	ncInterface.setStrings();
	dialogMode = true;

	ncInterface.setTitle("AgiliaLinux installer");
	ncInterface.setSubtitle("Language selection");

	vector<MenuItem> menuItems;
	menuItems.push_back(MenuItem("ru", "Русский"));
	menuItems.push_back(MenuItem("uk", "Украинский"));
	menuItems.push_back(MenuItem("en", "English"));

	string lang = ncInterface.showMenu2("Please, select installation language:\nПожалуйста, выберите язык для установки:", menuItems);

	if (lang.empty()) return 1;

	string lc_locale;
	if (lang=="ru") lc_locale = "ru_RU.UTF-8";
	else if (lang=="uk") lc_locale = "uk_UA.UTF-8";
	else lc_locale = "en_US.UTF-8";

	map<string, string> settings;
	map<string, map<string, string> > partitions;
	vector<string> repositories;
	string home = getenv("HOME");
	if (!FileExists(home + "/.config")) system("mkdir -p " + home + "/.config");
	loadSettings(home + "/.config/agilia_installer.conf", settings, repositories, partitions);
	settings["language"] = lc_locale;
	saveSettings(home + "/.config/agilia_installer.conf", settings, repositories, partitions);

	ncInterface.uninit();
	return system(string("LC_ALL=" + lc_locale + " setup_config").c_str());
}
