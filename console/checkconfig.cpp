#include <mpkg/libmpkg.h>

int print_usage() {
	printf(_("mpkg-checkconfigs: checks all .new files if they are properly renamed and exists\n"));
	printf(_("Usage: mpkg-checkconfigs [PKGNAME] [OPTIONS] - checks specified package for errors\n"));
	printf(_("Running mpkg-checkconfigs without specifying package name will check whole system.\n"));
	printf(_("Available options:"));
	printf(_("\t-h\t--help\tShow this help\n"));
	printf(_("\t-r\t--repair\tRepair errors. Default action is to rename .new to original\n"));
	printf(_("\t-c\t--copy\tSame as previous, but Instead of moving, do copying\n"));
	return 0;

}

int main(int argc, char **argv) {

	setlocale(LC_ALL, "");
	bindtextdomain( "mpkg", "/usr/share/locale");
	textdomain("mpkg");
	
	int ich;
	const char* program_name;
	extern int optind;
	const char* short_opt = "rhcv";
	const struct option long_options[] =  {
		{ "help",		0, NULL,	'h'},
		{ "repair",		0, NULL,	'r'},
		{ "copy",		0, NULL,	'c'},
		{ NULL, 		0, NULL, 	0}
	};

	_cmdOptions["sql_readonly"] = "yes";
	program_name = argv[0];
	bool repair;
	bool copy;
	do {
		ich = getopt_long(argc, argv, short_opt, long_options, NULL);
		

		switch (ich) {
			case 'h':
			case '?':
					return print_usage();
			case 'r':
					repair = true;
					break;
			case 'c':
					repair = true;
					copy = true;
					break;
		
			case -1:
					break;
					

			default:
					abort();
		}
	
	}  while ( ich != -1 );




	SQLRecord sqlSearch;
	sqlSearch.addField("file_name", string("%%.new"));
	sqlSearch.setEqMode(EQ_CUSTOMLIKE);
	SQLTable results;
	SQLRecord sqlFields;
	sqlFields.addField("packages_package_id");
	sqlFields.addField("file_name");

	PACKAGE_LIST pkgList;
	mpkg core;
	SQLRecord pkgSearch;
	
	for (int i=optind; i<argc; ++i) {
		pkgSearch.addField("package_name", (string) argv[i]);
	}
	if (pkgSearch.empty()) pkgSearch.addField("package_installed", 1);



	core.get_packagelist(pkgSearch, &pkgList);
	core.db->get_sql_vtable(results, sqlFields, "files", sqlSearch);

	int fId = results.getFieldIndex("packages_package_id");
	int fName = results.getFieldIndex("file_name");
	string tmp, orig;
	for (size_t i=0; i<pkgList.size(); ++i) {
		if (!pkgList[i].installed()) continue;
		for (size_t t=0; t<results.size(); ++t) {
			if (atoi(results.getValue(t, fId).c_str())!=pkgList[i].get_id()) continue;
			tmp = "/" + results.getValue(t, fName);
			orig = tmp.substr(0, tmp.size()-strlen(".new"));
			if (!FileExists(orig)) {
				if (!FileExists(tmp)) fprintf(stderr, "%s missing both:\t%s and .new\n", pkgList[i].get_name().c_str(), orig.c_str());
				else {
					printf("%s:\t%s\n", pkgList[i].get_name().c_str(), orig.c_str());
					if (repair) {
						if (copy) system("cp -v '" + tmp + "' '" + orig + "'");
						else system("mv -v '" + tmp + "' '" + orig + "'");

					}
				}
			}
		}
	}
	return 0;
}
