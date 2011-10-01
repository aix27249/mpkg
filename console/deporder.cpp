/* Reads package names from specified file, and generates it's dependency order using data from mpkg database.
 * Note that the latest known packages will be always picked.
 */
#include <mpkg/libmpkg.h>
int print_usage() {
	// FIXME: this is a shit-like usage output.
	fprintf(stderr, "Reads package names from specified file, and generates it's dependency order using data from mpkg database. Note that the latest known packages will be always picked.\n");
	fprintf(stderr, "Usage:\n\tmpkg-deporder FILENAME, where FILENAME is file with package names\n");
	return 1;
}
int main(int argc, char **argv) {
	if (argc<2) return print_usage();
	string filename = argv[1];
	vector<string> pkgnames = ReadFileStrings(filename);
	mpkg core;
	PACKAGE_LIST pkgList;
	SQLRecord sqlSearch;
	core.get_packagelist(sqlSearch, &pkgList);
	for (size_t i=0; i<pkgList.size(); ++i) {
		pkgList.get_package_ptr(i)->set_installed(0);
		pkgList.get_package_ptr(i)->set_configexist(0);
	}
	for (size_t i=0; i<pkgnames.size(); ++i) {
		for (size_t t=0; t<pkgList.size(); ++t) {
			if (pkgList[t].get_name()==pkgnames[i]) pkgList.get_package_ptr(t)->set_action(ST_INSTALL, "install-direct");
		}
	}
	DependencyTracker tracker(core.db);
	tracker.renderDependenciesInPackageList(&pkgList);
	for (size_t i=0; i<pkgList.size(); ++i) {
		if (pkgList[i].action()==ST_INSTALL) printf("%s\n", pkgList[i].get_name().c_str());
	}
	
	return 0;

}
/*
int main_2(int argc, char **argv) {
	if (argc<1) return print_usage();
	string filename = argv[1];
	vector<string> pkgnames = ReadFileStrings(filename);
	mpkg core;
	PACKAGE_LIST pkgList;
	SQLRecord sqlSearch;
	core.get_packagelist(sqlSearch, &pkgList);
	for (size_t i=0; i<pkgList.size(); ++i) {
		pkgList.get_package_ptr(i)->set_installed(0);
		pkgList.get_package_ptr(i)->set_configexist(0);
	}
	core.DepTracker->setFakePackageCache(pkgList);
	for (size_t i=0; i<pkgList.size(); ++i) {
		for (size_t t=0; t<pkgnames.size(); ++t) {
			if (pkgList[i].get_name()==pkgnames[t]) core.DepTracker->addToInstallQuery(pkgList[i]);
		}
	}
	core.DepTracker->renderData();
	PACKAGE_LIST query = core.DepTracker->get_install_list();
	for (size_t i=0; i<query.size(); ++i) {
		//printf("%d: %s %s\n", i, query[i].get_name().c_str(), query[i].get_fullversion().c_str());
		printf("%s\n", query[i].get_name().c_str());
	}
	return 0;

}*/
