/* Reads package names from specified file, and generates it's dependency order using data from mpkg database.
 * Note that the latest known packages will be always picked.
 */
#include <mpkg/libmpkg.h>
int print_usage() {
	printf("Usage: mpkg-deporder FILENAME, where FILENAME is file with package names\n");
	return 1;
}
bool canBeResolvedBy(const PACKAGE &pkg, const vector<PACKAGE *> pkgarray) {
	if (pkg.get_dependencies().empty()) return true;
	bool found;
	for (size_t i=0; i<pkg.get_dependencies().size(); ++i) {
		found = false;
		for (size_t t=0; !found && t<pkgarray.size(); ++t) {
			if (pkgarray[t]->get_name()==pkg.get_dependencies().at(i).get_package_name()) {
				fprintf(stderr, "RESOLV [%s]: %s resolved\n", pkg.get_name().c_str(), pkg.get_dependencies().at(i).get_package_name().c_str());
				found=true;
			}
		}
		if (!found) return false;
	}
	return true;
}
vector<PACKAGE *> buildDependencyOrderNew(const PACKAGE_LIST &packages) {
	vector<PACKAGE *> order;
	// Building zero ring
	for (size_t i=0; i<packages.size(); ++i) {
		if (packages[i].get_dependencies().empty()) {
			fprintf(stderr, "Zero-add: %s\n", packages[i].get_name().c_str());
			order.push_back((PACKAGE *) &packages[i]);
		}
	}
	fprintf (stderr, "Zero-ring: %d packages\n", (int) order.size());
	// Lurking thru package list
	size_t last_order_size;
	bool already_in_order;
	while(order.size()<packages.size()) {
		last_order_size=order.size();
		for (size_t i=0; i<packages.size(); ++i) {
			already_in_order=false;
			for (size_t t=0; !already_in_order && t<order.size(); ++t) {
				if (order[t]->get_name()==packages[i].get_name()) already_in_order=true;
			}
			if (already_in_order) continue;
			// If package can be satisfied by all deps in order, add them to one
			if (canBeResolvedBy(packages[i], order)) order.push_back((PACKAGE *) &packages[i]);
		}
		if (order.size()==last_order_size) {
			fprintf(stderr, "Leaved %d unresolved packages\n", (int) (packages.size() - order.size()));
			break;
		}
	}
	return order;

}
int main(int argc, char **argv) {
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
	PACKAGE_LIST filtered;
	for (size_t t=0; t<pkgList.size(); ++t) {
		if (pkgList[t].available()) filtered.add(pkgList[t]);
	}
	vector<PACKAGE *> order = buildDependencyOrderNew(filtered);
	for (size_t i=0; i<order.size(); ++i) {
		for (size_t t=0; t<pkgnames.size(); ++t) {
			if (order[i]->get_name()==pkgnames[t]) {
				printf("%s\n", pkgnames[t].c_str());
			}
		}
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
