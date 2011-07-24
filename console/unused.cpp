#include <mpkg/libmpkg.h>

int print_usage() {
	printf(_("mpkg-unused: find packages that is not a dependency for any other packages installed in system.\n"));
	printf(_("Usage: mpkg-unused [TAG]\n"));
	printf(_("Running mpkg-unused with no argument will check all installed packages.\n"));
	printf(_("Available options:"));
	printf(_("\t-h\t--help\tShow this help\n"));
	printf(_("\t-v\t--verbose\tVerbose mode: shows installed size of each package\n"));
	return 0;

}

int main(int argc, char **argv) {
	setlocale(LC_ALL, "");
	bindtextdomain( "mpkg", "/usr/share/locale");
	textdomain("mpkg");
	
	int ich;
	const char* program_name;
	extern int optind;
	const char* short_opt = "hv";
	const struct option long_options[] =  {
		{ "help",		0, NULL,	'h'},
		{ NULL, 		0, NULL, 	0}
	};

	_cmdOptions["sql_readonly"] = "yes";
	program_name = argv[0];
	do {
		ich = getopt_long(argc, argv, short_opt, long_options, NULL);
		

		switch (ich) {
			case 'h':
			case '?':
					return print_usage();
			case 'v':
					verbose = true;
					break;
		
			case -1:
					break;
			default:
					abort();
		}
	
	}  while ( ich != -1 );

	mpkg core;
	PACKAGE_LIST packages;
	SQLRecord sqlSearch;
	sqlSearch.addField("package_installed", 1);
	core.get_packagelist(sqlSearch, &packages);
	string tag;
       	if (argc>optind) tag	= argv[optind];


	vector<PACKAGE *> list;
	bool used;
	for (size_t z=0; z<packages.size(); ++z) {
		used = false;
		for (size_t i=0; !used && i<packages.size(); ++i) {
			for (size_t t=0; !used && t<packages[i].get_dependencies().size(); ++t) {
				if (packages[i].get_dependencies().at(t).get_package_name()==packages[z].get_corename()) {
					used = true;
				}
			}
		}
		if (!used) list.push_back(packages.get_package_ptr(z));
	}


	vector<PACKAGE *> selected_list;
	// Now, we have list of unused packages in list variable.
	for (size_t i=0; i<list.size(); ++i) {
		if (tag.empty() || list[i]->isTaggedBy(tag)) selected_list.push_back(list[i]);
	}


	// Output results
	fprintf(stderr, _("Found %d unused packages:\n"), (int) selected_list.size());
	long double total_size = 0;
	for (size_t i=0; i<selected_list.size(); ++i) {
		cout << selected_list[i]->get_name();
	       	if (verbose) cout << " (" << humanizeSize(selected_list[i]->get_installed_size()) << ")";
		total_size += atol(selected_list[i]->get_installed_size().c_str());
	       	cout << endl;
	}
	fprintf(stderr, _("Total: %s\n"), humanizeSize(total_size).c_str());

	return 0;
	
}

