/* MPKG package system
 * Broken dependency fixing tool
 * (c) 2010 Bogdan "aix27249" Kokorev, <i27249@gmail.com>
 * Code licensed under GPL version 3 or later
 */

#include <mpkg/libmpkg.h>

int checkRes(const string& packageName, const PACKAGE_LIST& pkgList) {
	bool hasNamed = false;
	for (size_t i=0; i<pkgList.size(); ++i) {
		if (pkgList[i].get_corename()==packageName) {
			if (pkgList[i].installed()) return 0;
			else hasNamed=true;
		}
	}
	if (hasNamed) return 1;
	return -1;
}

int print_usage() {
	fprintf(stderr, "mpkg-fixdeps: fixing broken dependencies of installed packages.\n");
	fprintf(stderr, "USAGE: mpkg-fixdeps\n");
	return 1;
}

int main(int argc, char **argv) {

	setlocale(LC_ALL, "");
	bindtextdomain( "mpkg", "/usr/share/locale");
	textdomain("mpkg");

	if (argc>1) return print_usage();

	interactive_mode=true;
	// Everything is easy. Look for broken dependencies in installed packages and try to install it.
	PACKAGE_LIST pkgList;
	SQLRecord sqlSearch;
	mpkg core;
	core.get_packagelist(sqlSearch, &pkgList);
	vector<string> installList;
	int res;
	int uid = getuid();
	if (uid != 0 ) {
		string arg_string;
		arg_string = (string) argv[0] + " ";
		for (int i=1; i<argc; i++) {
			arg_string += (string) argv[i] + " ";
		}
		if (system("sudo " + arg_string)!=0) {
			return 1;
		}
		return 0;
	}


	for (size_t i=0; i<pkgList.size(); ++i) {
		if (!pkgList[i].installed()) continue;
		for (size_t d=0; d<pkgList[i].get_dependencies().size(); ++d) {
			res = checkRes(pkgList[i].get_dependencies().at(d).get_package_name(), pkgList);
			if (res==0) continue;
			else if (res==-1) {
				mWarning(pkgList[i].get_name() + ": dependency " + pkgList[i].get_dependencies().at(d).get_package_name() + " unresolvable");
				continue;
			}
			printf("%s has broken dep: %s, resolving\n", pkgList[i].get_name().c_str(), pkgList[i].get_dependencies().at(d).get_package_name().c_str());
			installList.push_back(pkgList[i].get_dependencies().at(d).get_package_name());
		}
	}

	
	if (installList.size()>0) {
		core.install(installList, NULL, NULL);
		core.commit();
	}

	core.clean_queue();
	return 0;




}
