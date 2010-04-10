#include <mpkg/libmpkg.h>
#include <mpkg/dbstruct.h>
int main(int argc, char **argv) {
	string path;
	if (argc>1) path=argv[1];
	//WriteFile("sqlite_dump.sql", getDBStructure());
	if (path.find("://")==std::string::npos) {
		path = "./";
		if (!FileExists(path+"packages.xml.gz")) {
			mError(_("Index file ") + path+_("packages.xml.gz not found"));
			return 1;
		}
		path = "file://" + getAbsolutePath(path) + "/";
	}
	PACKAGE_LIST checkList;
	Repository rep;
	rep.get_index(path, &checkList);
	say(_("Checking %d packages in %s\n"), checkList.size(), path.c_str());
	for (size_t i=0; i<checkList.size(); ++i) {
		checkList.get_package_ptr(i)->set_action(ST_INSTALL, "check");
	}
	mpkg core;
	_cmdOptions["wtf"]="yes";
	DependencyTracker tracker(core.db);
	int rcount = tracker.renderDependenciesInPackageList(&checkList);
	if (rcount) {
		printf("Errors: %d\n", rcount);
		say("\n%s:\n%s", _("Dependency error"), depErrorTable.print().c_str());
	}

	return rcount;

}
