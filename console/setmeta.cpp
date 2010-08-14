#include <mpkg/libmpkg.h>
#include <mpkg/metaframe.h>
int print_usage() {
	fprintf(stderr, _("MPKG Package System: package metadata editor (CLI-based)\n"));
	fprintf(stderr, _("Usage: mpkg-setmeta FILENAME [OPTIONS]\n"));
	fprintf(stderr, _("Options:\n"));
	fprintf(stderr, _("-h   --help                      show this help\n"));
	fprintf(stderr, _("-n   --name=NAME                 set package name\n"));
	fprintf(stderr, _("-v   --version=VERSION           set package version\n"));
	fprintf(stderr, _("-a   --arch=ARCH                 set package architecture\n"));
	fprintf(stderr, _("-b   --build=BUILD               set package build\n"));
	fprintf(stderr, _("-B   --buildup                   increase package build\n"));
	fprintf(stderr, _("-N   --basename                  set name, version, arch and build by specified filename\n"));
	fprintf(stderr, _("-m   --maintainer-name=NAME      set maintainer name\n"));
	fprintf(stderr, _("-e   --maintainer-email=MAIL     set maintainer email\n"));
	fprintf(stderr, _("-t   --add-tag=TAG               add tag to package\n"));
	fprintf(stderr, _("-T   --remove-tag=TAG            remove tag from package\n"));
	fprintf(stderr, _("-d   --add-dep=DEP               add dependency (format: foo>=1.0)\n"));
	fprintf(stderr, _("-D   --remove-dep=DEP            remove dependency (format: foo)\n"));
	fprintf(stderr, _("-S   --shortdesc=FILE|TEXT       set short description\n"));
	fprintf(stderr, _("-l   --longdesc=FILE|TEXT        set long description\n"));
	fprintf(stderr, _("-c   --changelog=FILENAME|TEXT   set changelog\n"));
	fprintf(stderr, _("-X   --cleartags                 clear all previous tags\n"));
	fprintf(stderr, _("-Z   --cleardeps                 clear all previous dependencies\n"));
	fprintf(stderr, _("-p   --provides                  set provides value\n"));
	fprintf(stderr, _("-P   --clearprovides             clear provides value\n"));
	fprintf(stderr, _("-k   --conflicts                 set conflicts value\n"));
	fprintf(stderr, _("-K   --clearconflicts            clear conflicts value\n"));
	fprintf(stderr, _("-s   --keep-symlinks             keep symlinks in archive instead of moving it to doinst.sh\n"));
	fprintf(stderr, _("-f   --config-files=FILENAME     specify filename with list of configuration files and it's options. Replaces existing ones\n"));
	fprintf(stderr, _("-F   --clear-config-files        clears list of configuration files\n"));

	return 1;
}
void importConfigFiles(const vector<string> data, vector<ConfigFile> & ret) {
	// First: find delimiters
	// Second: split
	printf("Importing config files...\n");
	string filename;
	ConfigFile *cfile;
	string tmp, tmp_attr;
	size_t pos;
	for (size_t i=0; i<data.size(); ++i) {
		if (cutSpaces(data[i]).empty()) continue;
		cfile = new ConfigFile;
		pos = string::npos;
		for (size_t t=1; pos==string::npos && t<data[i].size(); ++t) {
			// Finding end of filename. Assuming first letter is not space.
			if (data[i][t]==' ' && data[i][t-1]!='\\') pos = t;
		}
		if (pos!=string::npos) {
			cfile->name = cutSpaces(data[i].substr(0, pos));
			tmp = cutSpaces(data[i].substr(pos));
		}
		else {
			cfile->name = cutSpaces(data[i]);
			tmp="";
		}
		// Cleaning leading slashes
		if (!cfile->name.empty() && cfile->name[0]=='/') {
			if (cfile->name.find_first_not_of("/")==std::string::npos) {
				printf("Bad config %s, skipping\n", cfile->name.c_str());
				delete cfile;
				continue;
			}
			cfile->name = cfile->name.substr(cfile->name.find_first_not_of("/"));
		}
		printf("Config file added: %s\n", cfile->name.c_str());
		while (!tmp.empty()) {
			pos = tmp.find_first_of(" ");
			if (pos!=string::npos) {
				tmp_attr = tmp.substr(0, pos);
				if (pos<tmp.size()-1) tmp = cutSpaces(tmp.substr(pos));
				else tmp.clear();
			}
			else {
				tmp_attr = tmp;
				tmp.clear();
			}
			
			pos = tmp_attr.find_first_of("=");
			if (pos!=string::npos && pos<tmp_attr.size()-1) cfile->addAttribute(tmp_attr.substr(0, pos), tmp_attr.substr(pos+1));
			else cfile->addAttribute(tmp_attr, "");
		}
		ret.push_back(*cfile);
		delete cfile;
		
	}
	printf("Config files import complete\n");
}

DEPENDENCY parseDependency(const string& dep) {
	DEPENDENCY ret;
	string tmp = cutSpaces(dep);
	size_t p = tmp.find_first_of(" <>!=\t");
	if (p==string::npos) {
		ret.set_package_name(tmp);
		ret.set_condition(IntToStr(VER_ANY));
		ret.set_type("DEPENDENCY");
		return ret;
	}
	else ret.set_package_name(tmp.substr(0, p));
	if (p>=tmp.size()-1) {
		mError(dep + _(" is incorrect, cannot resolve as dependency"));
		return ret;
	}
	tmp.erase(tmp.begin(), tmp.begin()+p);
	tmp = cutSpaces(tmp);
	p = tmp.find_first_not_of(" <>!=\t");
	if (p==std::string::npos || p>=tmp.size()-1) {
		mError(dep + _(" is incorrect, cannot resolve as dependency"));
		return ret;
	}
	ret.set_condition(IntToStr(condition2int(hcondition2xml(tmp.substr(0, p)))));
	tmp.erase(tmp.begin(), tmp.begin()+p);
	tmp = cutSpaces(tmp);
	ret.set_package_version(tmp);
	ret.set_type("DEPENDENCY");
	return ret;
	
}
int main(int argc, char **argv) {
	if (argc<2) return print_usage();
	if (string(argv[1])=="-h" || string(argv[1])=="--help") {
		print_usage();
		return 0;
	}
	MetaPackage *pkg = new MetaPackage(argv[1]);
	if (!pkg->data) {
		mError(_("Failed to open package, exiting"));
		delete pkg;
		return 1;
	}
	PACKAGE *data = pkg->data;
	//extern int optind, opterr, optopt;

	string n, v, a, b;
	extern char* optarg;
	int ich;
	const char* short_opt = "nN:v:a:b:Bm:e:t:T:d:D:sl:c:XZp:Pk:KFf:";
	const struct option long_options[] =  {
		{ "help",		0, NULL,	'h'},
		{ "name",		1, NULL,	'n'},
		{ "basename",		1, NULL,	'N'},
		{ "version",		1, NULL,	'v'},
		{ "arch",		1, NULL,	'a'},
		{ "build",		1, NULL,	'b'},
		{ "buildup",		0, NULL,	'B'},
		{ "maintainer-name",	1, NULL,	'm'},
		{ "maintainer-email",	1, NULL,	'e'},
		{ "add-tag",		1, NULL,	't'},
		{ "remove-tag",		1, NULL,	'T'},
		{ "add-dep",		1, NULL,	'd'},
		{ "remove-dep",		1, NULL,	'D'},
		{ "shortdesc",		1, NULL,	'S'},
		{ "longdesc",		1, NULL,	'l'},
		{ "changelog",		1, NULL,	'c'},
		{ "cleartags",		0, NULL,	'X'},
		{ "cleardeps",		0, NULL,	'Z'},
		{ "keep-symlinks",	0, NULL,	's'},
		{ "provides",		1, NULL,	'p'},
		{ "conflicts",		1, NULL,	'k'},
		{ "clearconflicts",	0, NULL,	'K'},
		{ "clearprovides",	0, NULL,	'P'},
		{ "config-files",	1, NULL,	'f'},
		{ "clear-config-files", 0, NULL,	'F'},
		{ NULL, 		0, NULL, 	0}
	};

	vector<string> newtags, removetags, newdeps, removedeps, config_files;
	do {
		ich = getopt_long(argc, argv, short_opt, long_options, NULL);
		switch (ich) {
			case 'h':
					print_usage();
					delete pkg;
					return 0;
					break;
			case 'n':
					data->set_name(string(optarg));
					break;
			case 'N':
					if (splitSlackwareFilename(string(optarg), &n, &v, &a, &b)) {
						data->set_name(n);
						data->set_version(v);
						data->set_arch(a);
						data->set_build(b);
					}
					else mError("Failed to parse basename");
					break;

			case 'v':
					data->set_version(string(optarg));
					break;
			case 'a':
					data->set_arch(string(optarg));
					break;
			case 'b':
					data->set_build(string(optarg));
					break;
			case 'B':
					data->set_build(IntToStr(atoi(data->get_build().c_str())+1));
					break;
			case 'm':
					data->set_packager(string(optarg));
					break;
			case 'e':
					data->set_packager_email(string(optarg));
					break;
			case 't':
					newtags.push_back(string(optarg));
					break;
			case 'T':
					removetags.push_back(string(optarg));
					break;
			case 'd':
					newdeps.push_back(string(optarg));
					break;
			case 'D':
					removedeps.push_back(string(optarg));
					break;
			case 'S':
					if (FileExists(string(optarg))) data->set_short_description(ReadFile(string(optarg)));
					else data->set_short_description(string(optarg));
					break;
			case 'l':
					if (FileExists(string(optarg))) data->set_description(ReadFile(string(optarg)));
					else data->set_description(string(optarg));
					break;
			case 'c':
					if (FileExists(string(optarg))) data->set_changelog(ReadFile(string(optarg)));
					else data->set_changelog(string(optarg));
					break;
			case 'X':
					data->get_tags_ptr()->clear();
					break;
			case 'Z':
					data->get_dependencies_ptr()->clear();
					break;
			case 'p':
					printf("Settings provides %s\n", optarg);
					data->set_provides(string(optarg));
					break;
			case 'k':
					data->set_conflicts(string(optarg));
					break;
			case 'P':
					data->set_provides("");
					break;
			case 'K':
					data->set_conflicts("");
					break;
			case 's':
					_cmdOptions["keep_symlinks"]="true";
					MAKEPKG_CMD = "/sbin/makepkg -l n -c n";
					break;
			case 'f':
					if (!FileExists(string(optarg))) {
						mWarning("File with configuration file list not found, check your command line.");
						break;
					}
					config_files = ReadFileStrings(optarg);
					importConfigFiles(config_files, data->config_files);
					break;
			case 'F':
					data->config_files.clear();
					break;

			case -1:
					break;
			default:
					delete pkg;
					return print_usage();
		}
	} while (ich!=-1);

	// Merging tags
	for (unsigned int i=0; i<data->get_tags().size(); ++i) {
		newtags.push_back(data->get_tags()[i]);
	}
	data->get_tags_ptr()->clear();
	sort(newtags.begin(), newtags.end());
	bool addThis;
	for (unsigned int i=0; i<newtags.size(); ++i) {
		addThis=true;
		for (unsigned int t=0; addThis && t<removetags.size(); ++t) {
			if (newtags[i]==removetags[t]) addThis=false;
		}
		for (unsigned int t=0; addThis && t<data->get_tags().size(); ++t) {
			if (newtags[i]==data->get_tags()[t]) addThis=false;
		}
		if (addThis) data->add_tag(newtags[i]);
	}
	// Merging dependencies
	vector<DEPENDENCY> deps = data->get_dependencies();
	data->get_dependencies_ptr()->clear();
	DEPENDENCY *tmpDep = NULL;
	for (unsigned int i=0; i<newdeps.size(); ++i) {
		if (tmpDep) delete tmpDep;
		tmpDep = new DEPENDENCY;
		*tmpDep = parseDependency(newdeps[i]);
		if (tmpDep->IsEmpty()) continue;
		deps.push_back(*tmpDep);
	}
	if (tmpDep) delete tmpDep;
	for (unsigned int i=0; i<deps.size(); ++i) {
		addThis = true;
		for (unsigned int t=0; t<removedeps.size(); ++t) {
			if (removedeps[t]==deps[i].get_package_name()) addThis = false;
		}
		for (unsigned int t=0; addThis && t<data->get_dependencies().size(); ++t) {
			if (deps[i].equalTo(data->get_dependencies()[t])) addThis = false;
		}
		if (addThis) data->get_dependencies_ptr()->push_back(deps[i]);
	}
	
	pkg->savePackage();
	delete pkg;
	return 0;
}
