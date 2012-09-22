/**
 *	MPKG package system    
 *	CLI interface
 *	
 *	$Id: installpkg-ng2.cpp,v 1.89 2007/12/10 03:12:58 i27249 Exp $
 */
#include <mpkg/libmpkg.h>
#include <mpkg/converter.h>
#include <mpkg/mpkgsys.h>
#include <mpkg/terminal.h>
#include <mpkg/menu.h>
#include <mpkg/ncurses_if.h>
#include <time.h>
#include <mpkg/tests.h>
#include <mpkg/console-help.h>
extern char* optarg;
extern int optind, opterr, optopt;
//string output_dir;
//static LoggerPtr rootLogger;
bool repair_damaged=false;
int setup_action(char* act);
int check_action(char* act);
int list_rep(mpkg *core);
bool showOnlyAvailable=false;
bool showOnlyInstalled=false;
bool showFilelist=false;
bool showHeader = true;
bool enqueueOnly = false;
bool exportinstalled_includeversions = false;
bool index_filelist = false;
bool rssMode = false;
string program_name;
int print_usage(string cmd, bool is_error = false) {
	return showCmdHelp(cmd, is_error);
}

void cleanDebugFile()
{
	struct stat s;
	string file = (string) log_directory + "/mpkg-debug.log";
	if (stat(file.c_str(), &s)==0)
	{
		if (s.st_size >= 4000) unlink(file.c_str());
	}
	file = (string) log_directory + "/mpkg-errors.log";
	if (stat(file.c_str(), &s)==0)
	{
		if (s.st_size >= 4000) unlink(file.c_str());
	}

}

int main (int argc, char **argv)
{

	setlocale(LC_ALL, "");
	bindtextdomain( "mpkg", "/usr/share/locale");
	textdomain("mpkg");
	ncInterface.setStrings();

	cleanDebugFile();
	program_name = argv[0];

	/**
	 * remove everything?
	 */
	int do_purge = 0;

	/**
	 * selected action
	 * by default - ACT_NONE
	 */
	int action = ACT_NONE;

	uid_t uid, euid;

	uid = getuid();
	euid = geteuid();

		
	bool do_reset=true;
	int ich;
	const char* short_opt = "hvpdzfmkDLrailgyqNcwxHbFQVRZWPKCMt:GsE:S:A:B:Ye:j:X:I";
	//const char* short_opt = "abcdfghiklmpqrvzDLyNwxHFQVRZWPKCME"; // Try to sort this stuff...later :)
	const struct option long_options[] =  {
		{ "help",		0, NULL,	'h'},
		{ "verbose", 		0, NULL,	'v'},
		{ "purge", 		0, NULL,	'p'},
		{ "force-dep", 		0, NULL,	'd'},
		{ "no-dep",		0, NULL,	'z'},
		{ "force-conflicts",	0, NULL,	'f'},
		{ "no-md5",		0, NULL,	'm'},
		{ "force-essential",	0, NULL,	'k'},
		{ "keep-symlinks",	0, NULL,	's'},
		{ "download-only",	0, NULL,	'D'},
		{ "repair",		0, NULL,	'r'},
		{ "available",		0, NULL,	'a'},
		{ "installed",		0, NULL,	'i'},
		{ "filelist",		0, NULL,	'l'},
		{ "dialog",		0, NULL,	'g'},
		{ "noconfirm",		0, NULL,	'y'},
		{ "noreset",		0, NULL,	'q'},
		{ "nodepgen",		0, NULL,	'N'},
		{ "no-resume",		0, NULL,	'c'},
		{ "no-header",		0, NULL,	'x'},
		{ "html",		0, NULL,	'H'},
		{ "cleartags",		0, NULL,	'b'},
		{ "fork",		0, NULL,	'F'},
		{ "enqueue", 		0, NULL,	'Q'},
		{ "includeversions",	0, NULL,	'V'},
		{ "index-filelist",	0, NULL,	'R'},
		{ "sysroot",		1, NULL,	'S'},
		{ "download-to",	1, NULL,	'A'},
		{ "linksonly",		0, NULL,	'L'},
		{ "rssmode",		0, NULL,	'Z'},
		{ "enable-spkg-index",	0, NULL,	'W'},
		{ "noaaa",		0, NULL,	'P'},
		{ "skip-deprecated",	0, NULL,	'K'},
		{ "enable-cache",	0, NULL,	'C'},
		{ "keep-deps",		0, NULL,	'G'},
		{ "md5index",		0, NULL,	'M'},
		{ "conf",		1, NULL,	't'},
		{ "exclusion-list",	1, NULL,	'E'},
		{ "abuild",		1, NULL,	'B'},
		{ "resync",		0, NULL,	'Y'},
		{ "arch",		1, NULL,	'e'},
		{ "limit",		1, NULL,	'j'},
		{ "skip",		1, NULL,	'X'},
		{ "machinemode",	0, NULL,	'I'},
		{ NULL, 		0, NULL, 	0}
	};

	bool clear_other_tags = false;
	pid_t pid;
	if (!dialogMode) interactive_mode=true;
	do {
		ich = getopt_long(argc, argv, short_opt, long_options, NULL);
		

		switch (ich) {
			case 'I':
					_cmdOptions["machine_mode"]="yes";
					break;
			case 'X':
					_cmdOptions["skip_packages"]=string(optarg);
					break;
			case 'j':
					_cmdOptions["versionLimit"]=string(optarg);
					break;
			case 'e':
					_cmdOptions["arch"]=string(optarg);
					break;
			case 'Y':
					forceFullDBUpdate = true;
					break;
			case 'B':
					_cmdOptions["abuild_links_only"]="yes";
					_cmdOptions["abuild_links_output"]=string(optarg);
					break;
			case 'E':
					_cmdOptions["exclusion_list"]=string(optarg);
					break;
			case 'G':
					_cmdOptions["preserve_deps"]="yes";
					break;
			case 'C':
					_cmdOptions["index_cache"]="yes";
					break;
			case 'K':
					_cmdOptions["skip-deprecated"]="true";
					break;
			case 'M':
					_cmdOptions["md5fileindex"]="true";
					break;
			case 'P':
					afraidAaaInDeps=true;
					break;
			case 'W':
					enableSpkgIndexing=true;
					break;
			case 't':
					CONFIG_FILE=string(optarg);
					mConfig.configName=CONFIG_FILE;
					break;
			case 'S':
					TEMP_SYSROOT=string(optarg);
					SYS_ROOT=TEMP_SYSROOT;
					if (!dialogMode) printf(_("Temporarily reassigned system root: %s\n"), TEMP_SYSROOT.c_str());
					break;
			case 'A':
					TEMP_CACHE=string(optarg);
					SYS_CACHE = TEMP_CACHE;
					if (!dialogMode) printf(_("Temporarily reassigned system cache: %s\n"), TEMP_CACHE.c_str());

					break;
			case 'R':
					index_filelist = true;
					break;
			case 'V':
					exportinstalled_includeversions = true;
					_cmdOptions["pkglist_versioning"]="true";
					break;
			case 'Q':
					enqueueOnly = true;
					break;
			case 'F':	
					pid = fork();
					if (pid) return 0;
					break;
			case 'b':	
					clear_other_tags = true;
					break;
			case 'x':
					showHeader=false;
					break;
			case 'c':
					enableDownloadResume=false;
					break;
			case 'q':
					do_reset=false;
					break;
			case 'h':
					return print_usage(program_name);

			case 'v':
					verbose = true;
					break;

			case 'p':
					do_purge = 1;
					break;

			case 'd':
					force_dep = true;
					break;
			case 'f':
					force_skip_conflictcheck = true;
					break;

			case 'm':
					forceSkipLinkMD5Checks=true;
					break;

			case 'k':
					force_essential_remove=true;
					break;
			case 's':
					_cmdOptions["keep_symlinks"]="true";
					MAKEPKG_CMD = "/sbin/makepkg -l n -c n";
					break;
			case 'i':
					showOnlyInstalled=true;
					break;
			case 'a':
					showOnlyAvailable=true;
					break;
			case 'D':
					download_only=true;
					break;
			case 'L':
					getlinksOnly=true;
					break;

			case 'r':
					repair_damaged=true;
					break;

			case 'l':
					showFilelist=true;
					break;
			case 'g':
					dialogMode=true;
					break;
	
			case 'z':
					ignoreDeps=true;
					break;
			case 'y':
					interactive_mode=false;
					break;
			case 'N':
					autogenDepsMode=ADMODE_OFF;
					break;
			case 'w':
					useBuildCache = false;
					break;
			case 'H':
					htmlMode = true;
					interactive_mode = false;
					break;
			case 'Z':
					rssMode=true;
					interactive_mode = false;
					break;
			case '?':
					return print_usage(program_name, true);

			case -1:
					break;
					

			default:
					abort();
		}
	
	}  while ( ich != -1 );


	// Check database lock
	mpkg core;
	if (!core.init_ok)
	{
		mError(_("Error initializing CORE"));
		abort();
	}
	if (string(program_name).find("buildpkg")==string(program_name).size()-strlen("buildpkg"))
	{
		if (argc-optind==1) core.build_package((string) argv[optind],false);
		if (argc-optind==0) core.build_package((string) "", false);
		if (argc-optind>2) {
			fprintf(stderr,_("Too many arguments\n"));
			fprintf(stderr,_("Usage: buildpkg [output_directory]\n"));
			return -1;
		}
	    	return 0;
	}
	if ((string) program_name == "buildsrcpkg")
	{
		if (argc==2) core.build_package((string) argv[1],true);
		if (argc==1) core.build_package((string) "", true);
		if (argc>2) {
			fprintf(stderr,_("Too many arguments\n"));
			fprintf(stderr,_("Usage: buildsrcpkg [output_directory]\n"));
			return -1;
		}
	    	return 0;
	}
	if ( optind < argc ) {
		if ( check_action( argv[optind++] ) == -1 )
		{
			return print_usage(program_name, true);
		}
		
		action = setup_action( argv[optind-1] );
	}

	if (action==ACT_SHOW || \
			action == ACT_SEARCH || \
			action == ACT_SEARCHBYDESCRIPTION || \
			action == ACT_SEARCHBYLOCATION || \
			action == ACT_SHOWQUEUE || \
			action == ACT_LIST || \
			action == ACT_INDEX || \
			action == ACT_FILESEARCH || \
			action == ACT_LISTUPGRADE || \
			action == ACT_LISTDEPENDANTS || \
			action == ACT_LIST_REP || \
			action == ACT_WHICH || \
			action == ACT_LISTGROUPS || \
			action == ACT_LISTGROUP || \
			action == ACT_SHOWVERSION || \
			action == ACT_TEST || \
			action == ACT_CHECKLIST || \
			action == ACT_EXPORTINSTALLED || \
			action == ACT_SYNC || \
			action == ACT_NONE) 
		require_root=false;
	if (mConfig.getValue("require_root")=="false") require_root = false;
	if (require_root && uid != 0 ) {
		string arg_string;
		arg_string = (string) argv[0] + " -x ";
		for (int i=1; i<argc; i++) {
			arg_string += (string) argv[i] + " ";
		}
		if (getenv("SKIP") && !cutSpaces(getenv("SKIP")).empty()) {
			vector<string> tmp_skip = splitString(getenv("SKIP"), " ");
			arg_string += " -X ";
			for (size_t s=0; s<tmp_skip.size(); ++s) {
				arg_string += tmp_skip[s];
				if (s<tmp_skip.size()-1) arg_string+= ",";
			}
		}
		//say("%s\n", _("You must login as root to run this program"));
		if (system("sudo " + arg_string)!=0) {
			return 1;
		}
		return 0;
	}
	if (htmlMode && isDatabaseLocked()) {
		newHtmlPage();
		printHtmlError("Database is locked, cannot process");
		exit(0);
	}
	if (require_root && isDatabaseLocked())
	{
		mError(_("Error: database is locked. Please close all other programs that use this"));
		exit(1);
	}

	if (action == ACT_INSTALL ||
			action == ACT_INSTALLFROMLIST ||
			action == ACT_PACKAGEMENU ||
			action == ACT_PURGE ||
			action == ACT_REMOVE ||
			action == ACT_INSTALLGROUP ||
			action == ACT_REMOVEGROUP ||
			action == ACT_UPDATEALL ||
			action == ACT_REINSTALL )
	{
		if (do_reset) core.clean_queue();
	}

	if ( action == ACT_NONE ) {
		return print_usage(program_name, true);
	}

	if ( action == ACT_SHOW)
	{
		if (argc<=optind) return print_usage(program_name, true);
		
		_cmdOptions["sql_readonly"]="yes";
		//------
		vector<string> r_name;
		string name, version, build;
		name = argv[optind];
		version.clear();
		build.clear();
		if (name.find_first_of("=")!=std::string::npos) {
			version = name.substr(name.find_first_of("="));
			if (version.length()>1) version=version.substr(1);
			name = name.substr(0, name.find_first_of("="));
			if (version.find_first_of("=")!=std::string::npos) {
				build = version.substr(version.find_first_of("="));
				if (build.length()>1) build=build.substr(1);
				version = version.substr(0, version.find_first_of("="));
			}
		}
		
		//------
		if (!htmlMode) show_package_info(&core, name, version, build, showFilelist);
		else {
			show_package_info(&core, "", "", "", showFilelist, atoi(argv[optind]));
		}
	}
	vector<string> pname;
	if (action == ACT_COMMIT)
	{
		if (argc!=optind) return print_usage(program_name, true);
		core.commit();
		unlockDatabase();
		return 0;
	}

	if (action == ACT_SHOWQUEUE)
	{
		if (argc!=optind) return print_usage(program_name, true);

		_cmdOptions["sql_readonly"]="yes";
		vector<string> list_empty;
		list(&core, list_empty,showOnlyAvailable, showOnlyInstalled, true);
		delete_tmp_files();
		return 0;
	}

	if (action == ACT_RESETQUEUE)
	{
		if (argc!=optind) return print_usage(program_name, true);

		lockDatabase();
		core.clean_queue();
		delete_tmp_files();
		unlockDatabase();
		return 0;
	}
	if (action == ACT_MENU) {
		lockDatabase();
		hideErrors = false;
		showMainMenu(core);
		unlockDatabase();
		delete_tmp_files();
		return 0;
	}

	if (action == ACT_PACKAGEMENU)
	{
		if (argc!=optind) return print_usage(program_name, true);
		lockDatabase();
		dialogMode = true;
		actPackageMenu(core);
		unlockDatabase();
		return 0;
	}

	if (action == ACT_INSTALLFROMLIST)
	{
		if (argc<=optind) return print_usage(program_name, true);
		string filename = argv[optind];
		if (filename.find("http://")==0 || filename.find("ftp://")==0) {
			string tmp = get_tmp_file();
			unlink(tmp.c_str());
			system("wget " + filename + " -O " + tmp);
			filename = tmp;
		}
		if (!FileExists(filename)) {
			mError(_("File not found"));
			return -1;
		}
		lockDatabase();
		int ret = actInstallFromList(core, filename, exportinstalled_includeversions, enqueueOnly);
		delete_tmp_files();
		unlockDatabase();
		return ret;
	}				
	
	if (action == ACT_SHOWVERSION) {
		showBanner();
		return 0;
	}
	if (action == ACT_UPDATEALL || action == ACT_LISTUPGRADE)
	{
		actUpgrade(core, action);
		return 0;
	}

	if (action == ACT_BUILD)
	{
		fprintf(stderr, _("mpkg-build has been deprecated many time ago, and it's support was dropped. Please convert your SPKG files to ABUILD using mpkg-spkg2abuild."));
		return 1;
	}

	if (action == ACT_INSTALL || action == ACT_UPGRADE || action == ACT_REINSTALL)
	{

		vector<string> r_name, fname, p_version, p_build;
		string name, version, build;
		if (argc<=optind) return print_usage(program_name, true);
		lockDatabase();
		string http_file_tmp;
		for (int i = optind; i < argc; i++) {
			if (string(argv[i]).find("http://")==0 || string(argv[i]).find("ftp://")==0) {
				CommonGetFile(argv[i], SYS_CACHE + getFilename(argv[i]));
				r_name.push_back(SYS_CACHE + getFilename(argv[i]));

			}
			else if (string(argv[i]).find("file:///")==0) r_name.push_back(string(argv[i]).substr(strlen("file://"))); // #1460: added handler for file:// urls.
			else r_name.push_back((string) argv[i]);
		}

		// Check for wildcards
		bool hasWilds=false;
		for (unsigned int i=0; i<r_name.size(); i++)
		{
			if (r_name[i].find("*")!=std::string::npos)
			{
				hasWilds=true;
				break;
			}
		}
		if (hasWilds)
		{
			SQLTable pl;
			SQLRecord sqlr, sqlfields;
			sqlr.setEqMode(EQ_CUSTOMLIKE);
			vector<string> srch=r_name;
			sqlfields.addField("package_name");
			for (unsigned int i=0; i<srch.size(); i++)
			{
				while(srch[i].find_first_of("*")!=std::string::npos)
				{
					srch[i][srch[i].find_first_of("*")]='%';
				}
				sqlr.addField("package_name", srch[i]);
			}
			core.db->get_sql_vtable(pl, sqlfields, (string) "packages", sqlr);
			r_name.clear();
			for (unsigned int i=0; i<pl.size(); i++)
			{
				r_name.push_back(pl.getValue(i,"package_name"));
			}
		}

		for (unsigned int i = 0; i < r_name.size(); i++)
		{
			name = r_name[i];
			version.clear();
			build.clear();
			if (name.find_first_of("=")!=std::string::npos) {
				version = name.substr(name.find_first_of("="));
				if (version.length()>1) version=version.substr(1);
				name = name.substr(0, name.find_first_of("="));
				if (version.find_first_of("=")!=std::string::npos) {
					build = version.substr(version.find_first_of("="));
					if (build.length()>1) build=build.substr(1);
					version = version.substr(0, version.find_first_of("="));
				}
			}
			//printf("adding [%s]-[%s]-[%s]\n", name.c_str(), version.c_str(), build.c_str());
			fname.push_back(name);
			p_version.push_back(version);
			p_build.push_back(build);
		}
		hideErrors = false;
		int ret = 0;
		if (action != ACT_REINSTALL) ret = core.install(fname, &p_version, &p_build);
		else {
			for (size_t i=0; i<fname.size(); ++i) {
				if (!core.repair(fname[i])) ret = -1;
			}
		}
		if (ret!=0) {
			if (!enqueueOnly) core.clean_queue();
			delete_tmp_files();
			return ret;
		}
		ret = core.commit(enqueueOnly);
		if (!enqueueOnly) core.clean_queue();
		delete_tmp_files();

		if (usedCdromMount) system("umount " + CDROM_MOUNTPOINT + " 2>/dev/null >/dev/null");
		unlockDatabase();
		return ret;
	}

	if (action == ACT_LISTGROUPS)
	{

		_cmdOptions["sql_readonly"]="yes";
		vector<string> availableTags;
		core.get_available_tags(&availableTags);
		for (unsigned int i=0; i<availableTags.size(); i++)
		{
			say("%s\n", availableTags[i].c_str());
		}
		return 0;
	}
	if (action == ACT_LISTDEPENDANTS)
	{
		_cmdOptions["sql_readonly"]="yes";
		if (argc<=optind) return print_usage(program_name, true);
		if (!dialogMode) fprintf(stderr, _("Searching for packages which depends on %s\n"), argv[optind]);
		actListDependants(core, argv[optind], showOnlyAvailable);
		return 0;
	}

	if (action == ACT_LISTGROUP)
	{
		_cmdOptions["sql_readonly"]="yes";
		if (argc<=optind) return print_usage(program_name, true);
		string group=argv[optind];
		PACKAGE_LIST pkgList1;
		PACKAGE_LIST pkgList2;
		SQLRecord sqlSearch;
		//printf("SLOW GET_PACKAGELIST CALL: %s %d\n", __func__, __LINE__);
		core.get_packagelist(sqlSearch, &pkgList1);
		for (unsigned int i=0; i<pkgList1.size(); i++)
		{
			if (pkgList1[i].isTaggedBy(group)) pkgList2.add(pkgList1[i]);
		}
		list_pkglist(pkgList2);
		return 0;
	}

	if (action == ACT_INSTALLGROUP)
	{
		if (argc<=optind) return print_usage(program_name, true);
		lockDatabase();
		string group=argv[optind];
		PACKAGE_LIST pkgList1;
		vector<string> queue;
		SQLRecord sqlSearch;
		//printf("SLOW GET_PACKAGELIST CALL: %s %d\n", __func__, __LINE__);
		core.get_packagelist(sqlSearch, &pkgList1);
		for (unsigned int i=0; i<pkgList1.size(); i++)
		{
			if (pkgList1[i].isTaggedBy(group) && !pkgList1[i].installed()) queue.push_back(pkgList1[i].get_name());
		}
		int ret = core.install(queue);
		if (ret!=0) {
			if (!enqueueOnly) core.clean_queue();
			delete_tmp_files();
			unlockDatabase();
			return ret;
		}
		ret = core.commit(enqueueOnly);

		if (!enqueueOnly) core.clean_queue();
		delete_tmp_files();
		unlockDatabase();
		

		return 0;
	}

	if (action == ACT_REMOVEGROUP)
	{
		if (argc<=optind) return print_usage(program_name, true);
		lockDatabase();
		string group=argv[optind];
		PACKAGE_LIST pkgList1;
		vector<string> queue;
		SQLRecord sqlSearch;
		//printf("SLOW GET_PACKAGELIST CALL: %s %d\n", __func__, __LINE__);
		core.get_packagelist(sqlSearch, &pkgList1);
		for (unsigned int i=0; i<pkgList1.size(); i++)
		{
			if (pkgList1[i].isTaggedBy(group) && pkgList1[i].installed()) queue.push_back(pkgList1[i].get_name());
		}
		int ret = core.uninstall(queue);

		hideErrors = false;
		if (ret != 0) {
			if (!enqueueOnly) core.clean_queue();
			delete_tmp_files();
			unlockDatabase();
			return ret;
		}
		ret = core.commit(enqueueOnly);
		if (!enqueueOnly) core.clean_queue();
		delete_tmp_files();
		unlockDatabase();
		return ret;

	}

	if (action == ACT_FILESEARCH)
	{
		_cmdOptions["sql_readonly"]="yes";
		if (argc<=optind) return print_usage(program_name, true);
		searchByFile(&core, argv[optind]);
		return 0;
	}


	if (action == ACT_WHICH)
	{
		_cmdOptions["sql_readonly"]="yes";
		if (argc<=optind) return print_usage(program_name, true);
		searchByFile(&core, argv[optind],true);
		return 0;
	}
	
	if (action == ACT_GENDEPSNEW) {
		if (argc<=optind) return print_usage(program_name, true);

		_cmdOptions["sql_readonly"]="yes";
		for (int i=optind; i<argc; ++i) {
			generateDeps_new(core, argv[i]);
		}
		return 0;

	}
	if (action == ACT_CLEARDEPS) {
		_cmdOptions["sql_readonly"]="yes";
		if (argc<=optind) return print_usage(program_name, true);
		for (int i=optind; i<argc; ++i) {
			system("mpkg-setmeta " + string(argv[i]) + " -Z");
		}
		return 0;
	}



	if (action == ACT_CHECKDAMAGE)
	{
		if (!repair_damaged) _cmdOptions["sql_readonly"]="yes";
		else lockDatabase();
		say(_("Retrieving package data, it may take a while...\n"));
		PACKAGE_LIST repairList;
		PACKAGE_LIST checkList;
		SQLRecord sqlSearch;
		sqlSearch.addField("package_installed",ST_INSTALLED);
		//printf("SLOW GET_PACKAGELIST CALL: %s %d\n", __func__, __LINE__);
		core.get_packagelist(sqlSearch, &checkList);
		if (optind>=argc) core.db->get_full_filelist(&checkList);
		say(_("checking...\n"));
		string pkgname;
		if (optind>=argc) {
			// Check entire system
			for (size_t i=0; i<checkList.size(); ++i) {
				if (core.checkPackageIntegrity(checkList.get_package_ptr(i))) {
					if (verbose) {
						say("[%d/%d] ",i+1,checkList.size());
						say("%s: %sOK%s\n", checkList[i].get_name().c_str(), CL_GREEN, CL_WHITE);
					}
				}
				else if (repair_damaged) {
					repairList.add(checkList[i]);
				}
			}
		}
		else {
			int pkgIndex=-1;
			for (int i = optind; i<argc; i++) {
				pkgname = (string) argv[i];
				for (size_t t=0; t<checkList.size(); ++t) {
					if (checkList[t].get_name().find(pkgname)==0) {
						pkgIndex=t;
						break;
					}
				}
				
				if (core.checkPackageIntegrity(pkgname)) say("%s: %sOK%s\n", argv[i], CL_GREEN, CL_WHITE);
				else {
					say(_("%s: %sDAMAGED%s\n"), argv[i], CL_RED, CL_WHITE);
					if (repair_damaged) repairList.add(checkList[pkgIndex]);
				}
			}
		}
		for (size_t i=0; i<repairList.size(); ++i) {
			core.repair(repairList.get_package_ptr(i));
		}
		if (repairList.size()>0)
		{
			say(_("\n\n----------Repairing damaged packages----------\n"));
			core.commit(enqueueOnly);
		}

		if (repair_damaged) unlockDatabase();
		return 0;
	}
	
	if ( action == ACT_QUERY ) {
		_cmdOptions["sql_readonly"]="yes";
		PACKAGE_LIST pkgList;
		SQLRecord sqlSearch;
		string q;
		string showUrl;
		for (int i=optind; i<argc; ++i) {
			q = (string) argv[i];
			// Filter: replace * by %
			
			//printf("Q:[%s]\n", q.c_str());
			for (unsigned int s=0; s<q.size(); ++s) {
				if (q[s]=='*') q[s]='%';
			}
			if (q.find("id=")==0) {
				sqlSearch.addField("package_id", atoi(q.substr(q.find("id=")+strlen("id=")).c_str()));
				continue;
			}
			if (q.find("name=")==0) {
				sqlSearch.addField("package_name", q.substr(q.find("name=")+strlen("name=")));
				continue;
			}
			if (q.find("version=")==0) {
				sqlSearch.addField("package_version", q.substr(q.find("version=")+strlen("version=")));
				continue;
			}
			if (q.find("build=")==0) {
				sqlSearch.addField("package_build", q.substr(q.find("build=")+strlen("build=")));
				continue;
			}
			if (q.find("description=")==0) {
				sqlSearch.addField("package_description", q.substr(q.find("description=")+strlen("description=")));
				continue;
			}
			if (q.find("short_description=")==0) {
				sqlSearch.addField("package_short_description", q.substr(q.find("short_description=")+strlen("short_description=")));
				continue;
			}
			if (q.find("EQMODE=")==0) {
				if (q.substr(q.find("EQMODE=")+strlen("EQMODE=")) == "CUSTOMLIKE") sqlSearch.setEqMode(EQ_CUSTOMLIKE);
				else if (q.substr(q.find("EQMODE=")+strlen("EQMODE=")) == "LIKE") sqlSearch.setEqMode(EQ_LIKE);
				else if (q.substr(q.find("EQMODE=")+strlen("EQMODE=")) == "EQUAL") sqlSearch.setEqMode(EQ_EQUAL);
				else {
					printf("Unknown EQMODE. Valid: EQUAL, LIKE, CUSTOMLIKE\n");
				}
				continue;
			}
			if (htmlMode && q.find("SHOW_URL=")==0) {
				showUrl = q.substr(q.find("SHOW_URL=")+strlen("SHOW_URL="));
				continue;
			}
			printf("Invalid query: [%s]\n", argv[i]);
			return 0;
		}
		if (sqlSearch.empty()) {
			printf("no query\n");
			return 0;
		}
		
		core.db->sql_exec("PRAGMA case_sensitive_like = false;");
		core.get_packagelist(sqlSearch, &pkgList, true);
		core.db->sql_exec("PRAGMA case_sensitive_like = true;");

		if (htmlMode) {
			string htmlData;
			PACKAGE *pkg;
			for (unsigned int i=0; i<pkgList.size(); i++) {
				pkg = pkgList.get_package_ptr(i);
				htmlData += "<a href=\"" + showUrl + IntToStr(pkg->get_id()) + "\">" + pkg->get_name() + " " + pkg->get_fullversion() + ": " + pkg->get_short_description()+"</a><br>";
			}
			if (pkgList.IsEmpty()) {
				htmlData = "<p>Nothing found</p>";
			}
			printf("%s\n", htmlData.c_str());
		}
		return 0;
	}

	if ( action == ACT_SEARCH ) {
		if (argc<=optind) return print_usage(program_name, true);

		_cmdOptions["sql_readonly"]="yes";
		vector<string> list_search;
		for (int i = optind; i < argc; i++)
		{
			list_search.push_back((string) argv[i]);
		}
		list(&core, list_search, showOnlyAvailable, showOnlyInstalled);
		delete_tmp_files();
		return 0;
	}
	if ( action == ACT_SEARCHBYDESCRIPTION ) {
		if (argc<=optind) return print_usage(program_name, true);
		_cmdOptions["sql_readonly"]="yes";
		vector<string> query;
		for (int i=optind; i < argc; i++) {
			query.push_back(string(argv[i]));
		}
		actSearchByDescription(core, query, showOnlyInstalled, showOnlyAvailable);
		return 0;

	}
	
	if ( action == ACT_CONVERT  ) {
		_cmdOptions["sql_readonly"]="yes";
		string tmpdir = get_tmp_dir();
		for (int i = optind; i < argc; i++) {
			actConvert(getAbsolutePath(argv[i]), tmpdir);
		}
		delete_tmp_files();
		return 0;
	
	}
	if ( action == ACT_NAVIVIZE) {
		_cmdOptions["sql_readonly"]="yes";
		string path="./";
		if (argc>optind) {
			path=argv[optind];
		}
		say(_("Nativizing in %s\n"), path.c_str());
		mpkgSys::nativize_directory(path);
		delete_tmp_files();
		return 0;
	}

	if ( action == ACT_TAG )
	{
		_cmdOptions["sql_readonly"]="yes";
		if (argc > optind+1)
		{
			say(_("tagging %s as %s...\n"), argv[optind+1], argv[optind]);
			tag_package(argv[optind+1], argv[optind], clear_other_tags);
		}
		delete_tmp_files();
		return 0;
	}
	if ( action == ACT_BUILDUP )
	{
		_cmdOptions["sql_readonly"]="yes";
		if (argc > optind)
		{
			say(_("increasing build of %s...\n"), argv[optind]);
			buildup_package(argv[optind]);
		}
		delete_tmp_files();
		return 0;
	}
	if ( action == ACT_SETVER ) {
		_cmdOptions["sql_readonly"]="yes";
		if (argc > optind + 1)
		{
			say(_("Setting version of %s to %s...\n"), argv[optind+1], argv[optind]);
			setver_package(argv[optind+1], argv[optind]);
		}
		delete_tmp_files();
		return 0;
	}


	if ( action == ACT_CONVERT_DIR ) {
		_cmdOptions["sql_readonly"]="yes";
		if (optind < argc )
		{
			core.convert_directory((string) argv[optind]);
		}
		else 
		{
			mError(_("Please define output directory"));
		}
		delete_tmp_files();

		return 0;
	}

	if ( action == ACT_LIST ) {
		if (argc!=optind) return print_usage(program_name, true);

		_cmdOptions["sql_readonly"]="yes";
		vector<string> list_empty;
		list(&core, list_empty, showOnlyAvailable, showOnlyInstalled);
		delete_tmp_files();
		return 0;
	}

	if ( action == ACT_UPDATE ) {
		if (argc!=optind) return print_usage(program_name, true);
		lockDatabase();
		actUpdate(core);
		unlockDatabase();
		return 0;
	}

	if ( action == ACT_CLEAN ) {
		if (argc!=optind) return print_usage(program_name, true);
		actClean(core);
		return 0;
	
	}

	if ( action == ACT_INDEX ) {
		_cmdOptions["sql_readonly"]="yes";
		string path = "./";
		if (argc>optind) path = string(argv[optind]);
		actIndex(core, path, index_filelist);
		delete_tmp_files();
		mDebug("Exiting from INDEX");
		//xmlFree();
	
		return 0;
	}

	if ( action == ACT_PURGE  || action==ACT_REMOVE) {
		if (argc<=optind) return print_usage(program_name, true);

		lockDatabase();
		vector<string> r_name;
		for (int i = optind; i < argc; i++) {
			r_name.push_back((string) argv[i]);
		}

		bool hasWilds=false;
		for (unsigned int i=0; i<r_name.size(); ++i) {
			if (r_name[i].find("*")!=std::string::npos){
				hasWilds=true;
				break;
			}
		}
		if (hasWilds) {
			SQLTable pl;
			SQLRecord sqlr, sqlfields;
			sqlr.setEqMode(EQ_CUSTOMLIKE);
			vector<string> srch=r_name;
			sqlfields.addField("package_name");
			sqlr.addField("package_installed", 1);
			for (unsigned int i=0; i<srch.size(); i++) {
				while(srch[i].find_first_of("*")!=std::string::npos) {
					srch[i][srch[i].find_first_of("*")]='%';
				}
				sqlr.addField("package_name", srch[i]);
			}
			core.db->get_sql_vtable(pl, sqlfields, (string) "packages", sqlr);
			r_name.clear();
			for (unsigned int i=0; i<pl.size(); i++) {
				r_name.push_back(pl.getValue(i,"package_name"));
			}
		}


		hideErrors = false;
		int ret = 0;
		if (action==ACT_REMOVE && do_purge==0) ret = core.uninstall(r_name);
		else ret = core.purge(r_name);
		if (ret!=0) {
			delete_tmp_files();
			if (!enqueueOnly) core.clean_queue();
			unlockDatabase();
			return ret;
		}
		ret = core.commit(enqueueOnly);
		if (!enqueueOnly) core.clean_queue();
		delete_tmp_files();
		unlockDatabase();
		return ret;
	}

	if ( action == ACT_LIST_REP ) {
		_cmdOptions["sql_readonly"]="yes";
		if (argc!=optind) return print_usage(program_name, true);

		list_rep(&core);
		delete_tmp_files();
		return 0;
	}
	if ( action == ACT_GETONLINEREPOSITORYLIST ) {
		string url;
		if (argc>optind) url = (string) argv[optind];
		actGetRepositorylist(url);
	}
	if ( action == ACT_CHECKLIST) {
		_cmdOptions["sql_readonly"]="yes";
		string path = "./";
		if (argc>optind) path = argv[optind];
/*		if (!FileExists(path+"packages.xml.gz")) {
			mError(_("Index file ") + path+_("packages.xml.gz not found"));
			return 1;
		}*/

		PACKAGE_LIST checkList;
		Repository rep;
		rep.get_index("file://" + getAbsolutePath(path) + "/", &checkList);
		say(_("Checking %d packages in %s\n"), checkList.size(), path.c_str());
		string md5, filename;
		unsigned int broken=0;
		vector<PACKAGE*> brokenPackages;
		vector<string> brokenPackagesStr;
		for (unsigned int i=0; i<checkList.size(); ++i) {
			filename = filter_slashes(path + checkList[i].get_locations().at(0).get_path() + checkList[i].get_filename());

			printf("Checking %s: ", filename.c_str());
			fflush(stdout);
			md5 = get_file_md5(filename);
			if (md5 == checkList[i].get_md5()) printf("%sOK%s\n", CL_GREEN, CL_WHITE);
			else {
				printf("%sBAD:%s md5 is %s, but should be %s\n", CL_RED, CL_WHITE, md5.c_str(), checkList[i].get_md5().c_str());
				brokenPackages.push_back(checkList.get_package_ptr(i));
				brokenPackagesStr.push_back(filename);
				broken++;
			}
		}
		printf("Summary:\nTotal: %d packages\nBad packages: %d\n", checkList.size(), broken);
		if (broken) WriteFileStrings("broken_packages.log", brokenPackagesStr);
		else unlink("broken_packages.log");

		delete_tmp_files();
		return 0;


	}
	if ( action == ACT_ADD_REPOSITORY ) {
		_cmdOptions["sql_readonly"]="yes";
		if (argc<=optind) return print_usage(program_name, true);
		string url;
		for (int i=optind; i < argc; i++) {
			// Check URL for validity
			url = argv[i];
			if (url.empty()) continue;
			if (url[0]=='/') url="file://" + url; // Convert absolute paths to file:// URLs
			if (url.find("http://")!=0 && url.find("ftp://")!=0 && url.find("https://")!=0 && url.find("ftps://")!=0 && url.find("cdrom://")!=0 && url.find("file:///")!=0) {
				// Invalid URL: warn user and skip it
				mWarning(_("Invalid repository URL ") + url);
				continue;
			}
			// Add ending slash if missing
			if (url[url.size()-1]!='/') url += "/";

			// Check file:// repo existance, and warn user if it doesn't exist or doesn't contains index
			if (url.find("file:///")==0) {
				string path = url.substr(strlen("file://"));
				if (!FileExists(path)) {
					mWarning(_("Directory ") + path + _(" doesn't exist"));
				}
				else if (!FileExists(path+"packages.xml.xz") && !FileExists(path+"packages.xml.gz")) {
					mWarning(_("Directory ") + path + _(" missing any package index"));
				}
			}
			
			core.add_repository(url);
		}
		list_rep(&core);
		return 0;
	}
	if (action == ACT_REMOVE_REPOSITORY ) {
		_cmdOptions["sql_readonly"]="yes";
		if (argc<=optind) return print_usage(program_name, true);
		int shift = 0;
		for (int i=optind; i<argc; i++) {
			core.remove_repository(atoi(argv[i])-shift);
			shift++;
		}
		list_rep(&core);
		return 0;
	}
	if (action == ACT_EXPORTINSTALLED) {
		_cmdOptions["sql_readonly"]="yes";
		string output;
		if (argc > optind) output = string(argv[optind]);
		vector<string> data = core.getExportInstalled(exportinstalled_includeversions);
		if (output.empty()) {
			for (unsigned int i=0; i<data.size(); ++i) {
				printf("%s\n", data[i].c_str());
			}
		}
		else WriteFileStrings(output, data);
		return 0;
	}

	if (action == ACT_ENABLE_REPOSITORY) {
		_cmdOptions["sql_readonly"]="yes";
		if (argc<=optind) return print_usage(program_name, true);
		vector<int> en;
		for (int i=optind; i<argc; i++) {
			en.push_back(atoi(argv[i])-1);
		}
		core.enable_repository(en);
		list_rep(&core);
		return 0;
	}
	if (action == ACT_DISABLE_REPOSITORY) {
		_cmdOptions["sql_readonly"]="yes";
		if (argc<=optind) return print_usage(program_name, true);
		vector<int> en;
		for (int i=optind; i<argc; i++) {
			en.push_back(atoi(argv[i])-1);
		}
		core.disable_repository(en);
		list_rep(&core);
		return 0;
	}
	if (action == ACT_SYNC) {
		_cmdOptions["sql_readonly"]="yes";
		if (argc<=optind) {
			return print_usage(program_name, true);
		}
		
		if (!FileExists(argv[optind])) mError(_("Sync map not found\n"));
		core.syncronize_repositories((string) argv[optind]);
		return 0;
	}

	unlockDatabase();
	return 0;
}



int list_rep(mpkg *core)
{
	say(_("Repository list:\n"));
	vector <string> rlist=core->get_repositorylist();
	vector<string> dlist = core->get_disabled_repositorylist();
	for (unsigned int i=0; i<rlist.size(); i++)
	{
		say("* [%d] %s\n", i+1, rlist[i].c_str());
	}
	for (unsigned int i=0; i<dlist.size(); i++)
	{
		cout << " [" << i+rlist.size()+1 << "] " << dlist[i] << " (" << _("DISABLED") << ")" << endl;
	}
	
	return 0;
}



int check_action(char* act)
{
	std::string _act(act);	
	int res = 0;

	mDebug((string) "action = " + act);


	if ( _act != "install"
		&& _act != "remove"
	  	&& _act != "update"
	  	&& _act != "upgrade"
	  	&& _act != "list"
	  	&& _act != "search"
		&& _act != "index"
		&& _act != "purge"
		&& _act != "convert"
		&& _act != "convert_dir"
		&& _act != "list_rep"
		&& _act != "tag"
		&& _act != "nativize"
		&& _act != "buildup"
		&& _act != "setver"
	  	&& _act != "clean"
		&& _act != "reset"
		&& _act != "commit"
		&& _act != "show_queue"
	  	&& _act != "show"
		&& _act != "installfromlist"
		&& _act != "test"
		&& _act != "check"
	  	&& _act != "menu"
	        && _act != "config"
		&& _act != "export"
		&& _act != "gendeps"
		&& _act != "gendeps2"
		&& _act != "cleardeps"
		&& _act != "filesearch"
		&& _act != "which"
		&& _act != "listgroup"
		&& _act != "installgroup"
		&& _act != "removegroup"
		&& _act != "whodepend"
		&& _act != "reinstall"
		&& _act != "upgradeall"
		&& _act != "listgroups"
		&& _act != "build"
		&& _act != "add_rep"
		&& _act != "delete_rep"
		&& _act != "enable_rep"
		&& _act != "disable_rep"
		&& _act != "listupdates"
		&& _act != "version"
		&& _act != "searchdescription"
		&& _act != "getrepositorylist"
		&& _act != "query"
		&& _act != "sync"
		&& _act != "lastupdates"
		&& _act != "checklist"
		&& _act != "exportinstalled"
		//&& _act != "edit_rep"
		) {
		res = -1;
	}
	else program_name = "mpkg-" + _act;

	mDebug("res = " + IntToStr(res));

	return res;
}

int setup_action(char* act)
{
	std::string _act(act);
	if (_act == "exportinstalled") return ACT_EXPORTINSTALLED;
	if (_act == "searchdescription") return ACT_SEARCHBYDESCRIPTION;
	if (_act == "version") return ACT_SHOWVERSION;
	if ( _act == "listupdates") return ACT_LISTUPGRADE;
	if ( _act == "getrepositorylist") return ACT_GETONLINEREPOSITORYLIST;
	if ( _act == "query") return ACT_QUERY;
	if ( _act == "sync") return ACT_SYNC;
	if ( _act == "listgroups")
		return ACT_LISTGROUPS;
	if ( _act == "upgradeall")
		return ACT_UPDATEALL;
	if ( _act == "reinstall")
			return ACT_REINSTALL;
	if ( _act == "whodepend")
			return ACT_LISTDEPENDANTS;
	if ( _act == "check")
			return ACT_CHECKDAMAGE;
	if ( _act == "test" )
			return ACT_TEST;

	if ( _act == "installfromlist" )
			return ACT_INSTALLFROMLIST;

	if ( _act == "install" )
			return ACT_INSTALL;

	if ( _act == "remove" )
			return ACT_REMOVE;

	if ( _act == "search" )
		   return ACT_SEARCH;

	if ( _act == "list")
		return 	ACT_LIST;

	if ( _act == "update" )
		return ACT_UPDATE;

	if ( _act == "upgrade" )
		return ACT_UPGRADE;

	if ( _act == "clean" )
		return ACT_CLEAN;

	if (_act == "index" )
		return ACT_INDEX;

	if (_act == "purge" )
		return ACT_PURGE;

	if (_act == "convert")
		return ACT_CONVERT;
	if (_act == "nativize")
		return ACT_NAVIVIZE;
	if (_act == "tag")
		return ACT_TAG;
	if (_act=="buildup") return ACT_BUILDUP;
	if (_act=="setver") return ACT_SETVER;

	if (_act == "convert_dir")
		return ACT_CONVERT_DIR;

	if (_act == "list_rep")
		return ACT_LIST_REP;

	if (_act == "reset")
		return ACT_RESETQUEUE;

	if (_act == "show_queue")
		return ACT_SHOWQUEUE;

	if (_act == "commit")
		return ACT_COMMIT;
	if (_act == "show")
		return ACT_SHOW;
	if (_act == "menu")
		return ACT_MENU;
	if (_act == "config")
		return ACT_CONFIG;
	/*if (_act == "export")
		return ACT_EXPORT;*/
	if (_act == "gendeps2" || _act == "gendeps") // From now on, old gendeps algorithm is removed.
		return ACT_GENDEPSNEW;
	if (_act == "cleardeps")
		return ACT_CLEARDEPS;
	if (_act == "filesearch")
		return ACT_FILESEARCH;
	if (_act == "which")
		return ACT_WHICH;
	if (_act == "listgroup")
		return ACT_LISTGROUP;
	if (_act == "installgroup")
		return ACT_INSTALLGROUP;
	if (_act == "removegroup")
		return ACT_REMOVEGROUP;
	if (_act == "build")
		return ACT_BUILD;
	if (_act == "add_rep") return ACT_ADD_REPOSITORY;
	if (_act == "delete_rep") return ACT_REMOVE_REPOSITORY;
	if (_act == "enable_rep") return ACT_ENABLE_REPOSITORY;
	if (_act == "disable_rep") return ACT_DISABLE_REPOSITORY;
	//if (_act == "edit_rep") return ACT_EDIT_REPOSITORY;
	if (_act == "checklist") return ACT_CHECKLIST;

	return ACT_NONE;
}


