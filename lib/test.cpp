#include "libmpkg.h"

int main(int argc, char **argv) {
	PACKAGE_LIST i_availablePackages;
	mpkg core;
	SQLRecord sqlSearch;
	string pkgname = "ffmpeg";
	sqlSearch.addField("package_name", pkgname);
	core.get_packagelist(sqlSearch, &i_availablePackages);
	PACKAGE nullPkg;
	PACKAGE *installed=&nullPkg, *maxv=&nullPkg;
	for (size_t i=0; i<i_availablePackages.size(); ++i) {
		cout << i_availablePackages[i].get_name() << " " << i_availablePackages[i].get_version() << " " << i_availablePackages[i].get_build() << endl;
		if (i_availablePackages[i].installed()) {
			installed = i_availablePackages.get_package_ptr(i);
		 	maxv = i_availablePackages.get_package_ptr(i);
		}
	}
	i_availablePackages.initVersioning();
	// Let's test max version stuff
	PACKAGE *pkg;
	pkg = (PACKAGE *) i_availablePackages.getInstalledOne();
	if (pkg) printf("getInstalledOne: STAGE1 PASSED\n");
	if (pkg==installed) printf("getInstalledOne: STAGE2 PASSED\n");
	pkg = i_availablePackages.getMaxVersion();
	if (pkg) printf("getMaxVersion: STAGE1 PASSED\n");
	if (pkg==maxv) printf("getMaxVersion: STAGE2 PASSED\n");
	else printf("pkg: %s, should be: %s\n", pkg->get_fullversion().c_str(), maxv->get_fullversion().c_str());
	return 0;

}
