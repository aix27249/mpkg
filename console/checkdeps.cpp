#include <mpkg/libmpkg.h>
#include <mpkg/dbstruct.h>

int print_usage(int ret = 0) {
	FILE *fd = stdout;
	if (ret!=0) fd = stderr;
	fprintf(fd, _("mpkg-checkdeps: checks dependency tree integrity within specified repositories. Used to find out unresolvable dependencies\n"));
	fprintf(fd, _("USAGE: mpkg-checkdeps REPOSITORY_URL [REPOSITORY_URL] [...]\n"));
	fprintf(fd, _("\nNote: Due to internal limitations, currently mpkg-checkdeps can check only repositores that match system architecture. For example, you cannot check x86_64 repository from i686 system\n"));
	return ret;
}

int main(int argc, char **argv) {
	string path;
	PACKAGE_LIST checkList;
	PACKAGE_LIST *tmpList;
	Repository rep;
	if (argc==1) return print_usage(-1);

	for (int i=1; i<argc; ++i) {
		path=argv[i];
		//WriteFile("sqlite_dump.sql", getDBStructure());
		if (path.find("://")==std::string::npos) {
			path = "./";
			if (!FileExists(path + "packages.xml.xz") && !FileExists(path+"packages.xml.gz")) {
				mError(_("Index file ") + path+_("packages.xml.xz or packages.xml.gz not found"));
				return 1;
			}
			path = "file://" + getAbsolutePath(path) + "/";
		}
		tmpList = new PACKAGE_LIST;
		rep.get_index(path, tmpList);
		checkList.add(*tmpList);
		delete tmpList;
	}
	fprintf(stderr, _("Checking %d packages\n"), checkList.size());
	for (size_t i=0; i<checkList.size(); ++i) {
		checkList.get_package_ptr(i)->set_action(ST_INSTALL, "check");
	}
	mpkg core;
	DependencyTracker tracker(core.db);
	int rcount = tracker.renderDependenciesInPackageList(&checkList);
	if (rcount) {
		printf("Errors: %d\n", rcount);
		say("\n%s:\n%s", _("Dependency error"), depErrorTable.print().c_str());
	}

	return rcount;

}
