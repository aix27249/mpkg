#include <mpkg/libmpkg.h>
#include <mpkg/terminal.h>
#include <mpkg/checklibdeps.h>
int print_usage() {
	printf(_("mpkg-checklibdeps: checks binary dependencies using ldd for missing libraries\n"));
	printf(_("Usage: mpkg-checklibdeps [PKGNAME] - checks package for errors\n"));
	printf(_("Running mpkg-checklibdeps without parameters will check whole system.\n"));
	printf(_("Output will be written to /var/log/mpkg-checklibdeps.log and /var/log/mpkg-checklibdeps-extended.log"));
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
	const char* short_opt = "fhxvVcs:l:";
	const struct option long_options[] =  {
		{ "help",		0, NULL,	'h'},
		{ "fast", 		0, NULL,	'f'},
		{ "get-xml",		0, NULL,	'x'},
		{ "verbose",		0, NULL,	'v'},
		{ "very-verbose",	0, NULL,	'V'},
		{ "compact",		0, NULL,	'c'},
		{ "filter-lib",		1, NULL,	'l'},
		{ "filter-symbol",	1, NULL,	's'},
		{ NULL, 		0, NULL, 	0}
	};

	bool fast = false;
	bool get_xml = false;
	bool compact = false;

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
			case 'x':
					get_xml = true;
					break;
			case 'v':
					verbose_level = 1;
					break;
			case 'V':
					verbose_level = 2;
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

			case -1:
					break;
					

			default:
					abort();
		}
	
	}  while ( ich != -1 );


	PACKAGE_LIST pkgList;
	mpkg *core = new mpkg;
	SQLRecord sqlSearch;
	for (int i=optind; i<argc; ++i) {
		sqlSearch.addField("package_name", (string) argv[i]);
	}
	if (sqlSearch.empty()) sqlSearch.addField("package_installed", 1);
	core->get_packagelist(sqlSearch, &pkgList);
	core->db->get_full_filelist(&pkgList);
	delete core;

	map<const PACKAGE *, PkgScanResults> scanResults = checkRevDeps(pkgList, fast);

	// NOTE: due to STL map implementation, it's size may increase with read-only operations, so please use errorCount variable if you wanna know initial count of packages affected by troubles.
	size_t errorCount = scanResults.size();

	printf("Total: %d possibly broken packages\n", (int) errorCount);

	if (get_xml) {
	}
	else {
		PkgScanResults res;
		for (size_t i=0; i<pkgList.size(); ++i) {
			if (!pkgList[i].installed()) continue;
			res = scanResults[&pkgList[i]];
			if (res.size()==0) continue;
			printf("%s-%s: %d errors\n", pkgList[i].get_name().c_str(), pkgList[i].get_fullversion().c_str(), (int) res.size());
			if (verbose_level>0) {
				vector<string> sE = res.getLostSymbols(symFilter);
				vector<string> lE = res.getLostLibs(libFilter);
				printf("\tSymbol errors: %d\n", (int) sE.size());
				for (size_t t=0; t<sE.size(); ++t) {
					cout << "\t\t" << sE[t] << endl;
				}
				printf("\tLibrary errors: %d\n", (int) lE.size());
				for (size_t t=0; t<lE.size(); ++t) {
					cout << "\t\t" << lE[t] << endl;
				}
			}
			if (verbose_level>1) {
				printf("\tDetails:\n");
				for (size_t t=0; t<res.symbolErrors.size(); ++t) {
					cout << "\t\tUNRESOLVED: " << res.symbolErrors[t].symbol << " (" << res.symbolErrors[t].filename << ")" << endl;
				}
				for (size_t t=0; t<res.notFoundErrors.size(); ++t) {
					cout << "\t\tNOT FOUND: " << res.notFoundErrors[t].libname << " (" << res.notFoundErrors[t].filename << ")" << endl;
				}
			}

		}
	}

	
	

}
/*


int main_old(int argc, char **argv) {
	mpkg core;
	SQLTable files;
	SQLRecord fields;
	fields.addField("packages_package_id");
	fields.addField("file_name");
	int fId = 0;
	int fName = 1;
	bool fast=false;
	if (argc>1 && strcmp(argv[1], "--fast")==0) fast = true;
	if (argc>1 && strcmp(argv[1], "--help")==0) return print_usage();
	SQLRecord search;
	if (!fast && argc>1) {
		SQLRecord pkgSearchInit;
		pkgSearchInit.addField("package_name", string(argv[1]));
		pkgSearchInit.addField("package_installed", ST_INSTALLED);
		PACKAGE_LIST checkList;
		core.get_packagelist(pkgSearchInit, &checkList);
		if (checkList.size()!=1) {
			mError("Package not found");
			return 1;
		}
		search.addField("packages_package_id", checkList[0].get_id());
	}
	core.db->get_sql_vtable(files, fields, "files", search);
	string tmpfile = "/dev/shm/mpkg-checklibdeps.log";
	string data;
	

	SQLRecord pkgSearch;
	pkgSearch.setSearchMode(SEARCH_IN);
	map<int, string> errorList;
	string fname;
	for (size_t i=0; i<files.size(); ++i) {
		fname = files.getValue(i, fName);
		if (fast) {
			if (fname.find("usr/lib")!=0 && fname.find("usr/bin/")!=0 && fname.find("bin/")!=0 && fname.find("sbin/")!=0 && fname.find("usr/sbin/")!=0) continue;
		}
		if (fname.empty() || fname[fname.size()-1]=='/' || fname.find("etc/")==0 || fname.find("dev/")==0 || fname.find("lib/modules/")==0 || fname.find("usr/share/")==0 || fname.find("usr/man/")==0 || fname.find("usr/include/")==0 || fname.find("usr/doc/")==0 || fname.find("usr/lib/locale/")==0 || fname.find("usr/lib64/locale/")==0 || fname.find("opt/")==0) continue;
		if (access(string("/" + fname).c_str(), X_OK)) continue;
		msay("[Errors found: " + IntToStr(pkgSearch.size()) + "] [" + IntToStr(i+1) + "/" + IntToStr(files.size()) + "] Checking file /" + fname);
		system("ldd -r '/" + files.getValue(i, fName) + "' 2>&1 | grep -P 'undefined symbol|not found' > " + tmpfile);
		data = ReadFile(tmpfile);
		if (data.empty()) continue;
		pkgSearch.addField("package_id", files.getValue(i, fId));
		errorList[atoi(files.getValue(i, fId).c_str())]+="/" + fname + ": " + data;
	}
	unlink(tmpfile.c_str());
	PACKAGE_LIST pkgList;
	if (pkgSearch.size()==0) {
		printf("\n\nCheck finished, no errors detected\n");
		return 0;
	}
	core.get_packagelist(pkgSearch, &pkgList);
	printf("Errors detected in %d packages:\n", pkgList.size());
	vector<string> pkgErrList, ePkgErrList;
	string elog;
	for (size_t i=0; i<pkgList.size(); ++i) {
		elog = pkgList[i].get_name() + " " + pkgList[i].get_fullversion() + ":\n" + errorList[pkgList[i].get_id()] + "\n\n";
		pkgErrList.push_back(pkgList[i].get_name());
		ePkgErrList.push_back(elog);
		printf("%s\n", elog.c_str());
		//printf("%s %s:\n%s\n\n", pkgList[i].get_name().c_str(), pkgList[i].get_fullversion().c_str(), errorList[pkgList[i].get_id()].c_str());
	}
	WriteFileStrings("/var/log/mpkg-checklibdeps.log", pkgErrList);
	WriteFileStrings("/var/log/mpkg-checklibdeps_extended.log", ePkgErrList);
	return 0;
}

*/
