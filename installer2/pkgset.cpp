/* New MOPSLinux setup: ncurses-based package source selection tool
 *
*/


#include <nwidgets/ncurses_if.h>
#include <mpkg/libmpkg.h>
#include "default_paths.h"
#include "helper_functions.h"
#include "pkgset.h"
// Where we should get list of available sets?
// There are options:
// 1. Predefined in SETUP_BASIC_CONF dir
// 2. User-specified file
// 3. From package sources.
// First option is already available, but there should be only one variant: full installation.
// Second option requires to ask user what file to use. It is also very simple.
// Third option requires access to package sources. They may be precached in previous steps, but may not.
// I think we should implement a separate module which will download all available package source indices to cache.
//
// For now (development age), use only ones in SETUP_PKGSET_DIR and SETUP_PKGSET_CACHE_DIR
CustomPkgSet getCustomPkgSet(const string& name) {
	vector<string> data = ReadFileStrings(SETUP_PKGSET_CACHE_DIR + name + ".desc");
	CustomPkgSet ret;
	ret.name = name;
	string locale = ReadFile(SETUPCONFIG_LANGUAGE);
	if (locale.size()>2) locale = "[" + locale.substr(0,2) + "]";
	else locale = "";
	string gendesc, genfull;
	for (size_t i=0; i<data.size(); ++i) {
		if (data[i].find("desc" + locale + ": ")==0) ret.desc = getParseValue("desc" + locale + ": ", data[i], true);
		if (data[i].find("desc: ")==0) gendesc = getParseValue("desc: ", data[i], true);
		if (data[i].find("full" + locale + ": ")==0) ret.full = getParseValue("full" + locale + ": ", data[i], true);
		if (data[i].find("full: ")==0) genfull = getParseValue("full: ", data[i], true);
	}
	if (ret.desc.empty()) ret.desc = gendesc;
	if (ret.full.empty()) ret.full = genfull;
	return ret;
}

vector<CustomPkgSet> getCustomSetupVariants(const vector<string>& rep_list) {
	vector<CustomPkgSet> customPkgSetList;
	string tmpfile = get_tmp_file();
	string path;
	for (size_t z=0; z<rep_list.size(); ++z) {
		path = rep_list[z];
		CommonGetFile(path + "/setup_variants.list", tmpfile);
		vector<string> list = ReadFileStrings(tmpfile);
		vector<CustomPkgSet> ret;
		for (size_t i=0; i<list.size(); ++i) {
			CommonGetFile(path + "/setup_variants/" + list[i] + ".desc", SETUP_PKGSET_CACHE_DIR + list[i] + ".desc");
			CommonGetFile(path + "/setup_variants/" + list[i] + ".list", SETUP_PKGSET_CACHE_DIR + list[i] + ".list");
			customPkgSetList.push_back(getCustomPkgSet(list[i]));
		}
	}
	return customPkgSetList;
}


int main(int, char **) {
	dialogMode = true;
	vector<string> rep_list = ReadFileStrings(SETUPCONFIG_REPOSITORYLIST);
	vector<CustomPkgSet> customPkgSetList = getCustomSetupVariants(rep_list);

	vector<MenuItem> menuItems;
	ncInterface.setSubtitle(_("Package selection"));//"Выбор пакетов для установки");
	
	menuItems.push_back(MenuItem(_("FULL"), _("Full installation (recommended)"), _("Full package set for any purposes: workstation, office, home, education, development, and so on. You can remove unneeded components at any time after installation.")));
	menuItems.push_back(MenuItem(_("MINIMAL"), _("Minimal installation"), _("A minimal package set. It is enough to boot and install packages, but will be a very difficult process. Not recommended unless you know what you are doing")));
	// TODO: Insert here custom variants
	for (size_t i=0; i<customPkgSetList.size(); ++i) {
		menuItems.push_back(MenuItem(customPkgSetList[i].name, customPkgSetList[i].desc, customPkgSetList[i].full));
	}
	menuItems.push_back(MenuItem(_("FILE"), _("Package list from file..."), _("Install package set by list predefined in your file")));
	string predefinedSet = ncInterface.showMenu2(_("Choose package set to install"), menuItems, _("FULL"));

	if (predefinedSet.empty()) return 1;
	else WriteFile(SETUPCONFIG_PKGSET, predefinedSet);


}
