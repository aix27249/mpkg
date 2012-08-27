#include <mpkg/libmpkg.h>
#include <mpkg/terminal.h>
#include <mpkg/checklibdeps.h>
int print_usage() {
	printf(_("mpkg-checklibdeps: checks binary dependencies using ldd for missing libraries\n"));
	printf(_("Usage: mpkg-checklibdeps [PKGNAME] [OPTIONS] - checks specified package for errors\n"));
	printf(_("Running mpkg-checklibdeps without parameters will check whole system.\n"));
	printf(_("Available options:"));
	printf(_("\t-h\t--help\tShow this help\n"));
	printf(_("\t-f\t--fast\tFast mode: skip directories which rarely contains anything that does make sence\n"));
	printf(_("\t-l\t--filter-lib=LIBRARY\tFilter errors for specified library. Example: -l libgmp.so.1\n"));
	printf(_("\t-s\t--filter-sym=SYMBOL\tFilter errors for specified symbol. Example: -s PythonUnicode_UCS2_FromLatin1\n"));
	printf(_("\t-c\t--compact\tCompact mode: makes output more machine-readable\n"));
	printf(_("\t-R\t--rebuild\tRequest rebuild for packages with errors\n"));
	printf(_("\t-d\t--interactive\tInteractive mode for rebuild: you can choose which packages to rebuild\n"));
	printf(_("\t-r\t--resolve\tCheck symbols for resolving. NOTE: errors may be false positive in case of runtime linking\n"));
	printf(_("\t-v\t--verbose\tVerbose mode: shows names of missing libs and symbols\n"));
	printf(_("\t-V\t--very-verbose\tVery verbose mode: shows maximum details\n"));
	printf(_("\t-C\t--recheck\tCheck only packages that produced errors in previous run\n"));
	return 0;
}

int main(int argc, char **argv) {

	setlocale(LC_ALL, "");
	bindtextdomain( "mpkg", "/usr/share/locale");
	textdomain("mpkg");

	int ich;
	const char* program_name;
	extern int optind;
	extern char* optarg;
	const char* short_opt = "fhrRdvVcs:l:LC";
	const struct option long_options[] =  {
		{ "help",		0, NULL,	'h'},
		{ "fast", 		0, NULL,	'f'},
		{ "resolve",		0, NULL,	'r'},
		{ "rebuild",		0, NULL,	'R'},
		{ "localtree",		0, NULL,	'L'},
		{ "interactive",	0, NULL,	'd'},
		{ "get-xml",		0, NULL,	'x'},
		{ "verbose",		0, NULL,	'v'},
		{ "very-verbose",	0, NULL,	'V'},
		{ "compact",		0, NULL,	'c'},
		{ "filter-lib",		1, NULL,	'l'},
		{ "filter-symbol",	1, NULL,	's'},
		{ "recheck",		1, NULL,	'C'},
		{ NULL, 		0, NULL, 	0}
	};

	bool fast = false;
	bool compact = false;
	bool skip_symbols = true;
	bool rebuild = false;
	bool interactive = false;
	bool local_tree = false;
	bool recheck = false;

	vector<string> symFilter, libFilter;


	int verbose_level = 0;
	_cmdOptions["sql_readonly"] = "yes";
	program_name = argv[0];
	do {
		ich = getopt_long(argc, argv, short_opt, long_options, NULL);
		

		switch (ich) {
			case 'h':
			case '?':
					return print_usage();
			case 'f':
					fast = true;
					break;
			case 'r':
					skip_symbols = false;
					break;
			case 'R':
					rebuild = true;
					break;
			case 'd':
					interactive = true;
					break;
			case 'L':
					local_tree = true;
					break;
			case 'v':
					verbose_level = 1;
					break;
			case 'V':
					verbose_level = 2;
					verbose = true;
					break;
			case 'c':
					compact = true;
					break;

			case 'l':
					libFilter.push_back((string) optarg);
					break;

			case 's':
					symFilter.push_back((string) optarg);
					break;
			case 'C':
					recheck = true;
					break;

			case -1:
					break;
					

			default:
					abort();
		}
	
	}  while ( ich != -1 );


	PACKAGE_LIST pkgList;
	mpkg *core = new mpkg;
	SQLRecord sqlSearch;
	const char *homeDir = getenv("HOME");
	if (!homeDir) {
		mError(_("Could not determine home directory"));
		return 1;
	}
	const string lastbrokenFile = (string) homeDir + "/.mpkg-checklibdeps-lastbroken";
	
	if (recheck) {
		vector<string> lastChecked = ReadFileStrings(lastbrokenFile);
		for (size_t i=0; i<lastChecked.size(); ++i) {
			if (cutSpaces(lastChecked[i])=="") continue;
			sqlSearch.addField("package_name", cutSpaces(lastChecked[i]));
		}
		if (sqlSearch.empty()) {
			fprintf(stderr, _("Nothing to recheck, running full check instead"));
		}
	}
	else {
		for (int i=optind; i<argc; ++i) {
			printf("[%d] optind: %d, argc: %d, adding %s\n", i, optind, argc, argv[i]);
			sqlSearch.addField("package_name", (string) argv[i]);
		}
	}
	sqlSearch.setSearchMode(SEARCH_IN);

	
	if (sqlSearch.empty()) sqlSearch.addField("package_installed", 1);
	core->get_packagelist(sqlSearch, &pkgList);
	core->db->get_full_filelist(&pkgList);
	delete core;

	map<const PACKAGE *, PkgScanResults> scanResults = checkRevDeps(pkgList, fast, skip_symbols);

	// NOTE: due to STL map implementation, it's size may increase with read-only operations, so please use errorCount variable if you wanna know initial count of packages affected by troubles.
	size_t errorCount = scanResults.size();

	if (!compact) printf(_("Scanned: %d packages\n"), (int) errorCount);

	PkgScanResults res;
	vector<PACKAGE *> rebuild_queue;
	vector<string> brokenPackageNames;
	//vector<string> errorList;
	//string tmpErrList;
	for (size_t i=0; i<pkgList.size(); ++i) {
		if (!pkgList[i].installed()) continue;
		res = scanResults[&pkgList[i]];
		if (res.size()==0) continue;
		if (!symFilter.empty() || !libFilter.empty()) {
			if (res.filteredSize(symFilter, libFilter)==0) continue;
		}
		// If we reach here, pkgList[i] has errors.
		brokenPackageNames.push_back(pkgList[i].get_name());
		if (!compact) printf(_("%s-%s: %d errors\n"), pkgList[i].get_name().c_str(), pkgList[i].get_fullversion().c_str(), (int) res.size());
		else printf("%s\n", pkgList[i].get_name().c_str());
		if (verbose_level>0 && !compact) {
			vector<string> sE = res.getLostSymbols(symFilter);
			vector<string> lE = res.getLostLibs(libFilter);
			//tmpErrList.clear();
			printf(_("\tSymbol errors: %d\n"), (int) sE.size());
			for (size_t t=0; t<sE.size(); ++t) {
				cout << "\t\t" << sE[t] << endl;
				//tmpErrList += sE[t] + "\n";
			}
			printf(_("\tLibrary errors: %d\n"), (int) lE.size());
			for (size_t t=0; t<lE.size(); ++t) {
				cout << "\t\t" << lE[t] << endl;
				//tmpErrList += lE[t] + "\n";
			}
			//errorList.push_back(tmpErrList);
		}
		if (verbose_level>1) {
			if (!compact) printf(_("\tDetails:\n"));
			for (size_t t=0; t<res.symbolErrors.size(); ++t) {
				cout << "\t\tUNRESOLVED: " << res.symbolErrors[t].symbol << " (" << res.symbolErrors[t].filename << ")" << endl;
			}
			for (size_t t=0; t<res.notFoundErrors.size(); ++t) {
				cout << "\t\tNOT FOUND: " << res.notFoundErrors[t].libname << " (" << res.notFoundErrors[t].filename << ")" << endl;
			}
		}
		if (rebuild) {
			rebuild_queue.push_back(pkgList.get_package_ptr(i));
		}
	}
	// Store last broken
	WriteFileStrings(lastbrokenFile, brokenPackageNames);
	// Check if we need to rebuild
	if (interactive) {
		vector<MenuItem> m;
		for (size_t i=0; i<rebuild_queue.size() /*&& i<errorList.size()*/; ++i) {
			// TODO: showExMenu currently ignores errorList entry, so we should either implement callback for some key to show errors, or implement showing errorList in showExMenu.
			m.push_back(MenuItem(rebuild_queue[i]->get_name(), rebuild_queue[i]->get_fullversion(), "", false));
		}
		dialogMode = true;
		if (ncInterface.showExMenu(_("Select packages you want to rebuild:"), m)!=0) return 0;
		ncInterface.uninit();
		dialogMode = false;
		vector<PACKAGE *> r_queue;
		for (size_t i=0; i<m.size() && i<rebuild_queue.size(); ++i) {
			if (m[i].flag) r_queue.push_back(rebuild_queue[i]);
		}
		rebuild_queue = r_queue;
	}

	string bt_source, new_build;
	int b_res;
	for (size_t i=0; i<rebuild_queue.size(); ++i) {
		if (!local_tree) {
			bt_source = "api:" + rebuild_queue[i]->get_name();
		}
		else {
			bt_source = "/usr/src/BuildTrees/" + rebuild_queue[i]->get_name() + "-" + rebuild_queue[i]->get_version() + ".build_tree.tar.xz";
		}
		new_build = IntToStr(atoi(rebuild_queue[i]->get_build().c_str()) + 1);

		b_res = system("mkpkg -bt " + bt_source + " -si -bb " + new_build);
		if (b_res!=0 && getenv("SKIP_REBUILD_ERRORS")==NULL) {
			mError("Failed to rebuild " + rebuild_queue[i]->get_name());
			return 1;
		}
	}
	return 0;
}

