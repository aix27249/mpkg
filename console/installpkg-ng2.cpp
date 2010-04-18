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
const char* program_name;
extern char* optarg;
extern int optind, opterr, optopt;
//string output_dir;
//static LoggerPtr rootLogger;
bool repair_damaged=false;
int setup_action(char* act);
int check_action(char* act);
int print_usage(FILE* stream=stdout, int exit_code=0);
void ShowBanner();
int list_rep(mpkg *core);
bool showOnlyAvailable=false;
bool showOnlyInstalled=false;
bool showFilelist=false;
bool showHeader = true;
bool enqueueOnly = false;
bool exportinstalled_includeversions = false;
bool index_filelist = false;
bool rssMode = false;
void ShowBanner()
{
	say("MPKG package system v.%s\n", mpkgVersion);
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
	const char* short_opt = "hvpdzfmkDLrailgyqNcwxHbFQVRZWPKCMt:GsE:S:A:";
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
		{ NULL, 		0, NULL, 	0}
	};

	program_name = argv[0];
	bool clear_other_tags = false;
	pid_t pid;
	if (!dialogMode) interactive_mode=true;
	do {
		ich = getopt_long(argc, argv, short_opt, long_options, NULL);
		

		switch (ich) {
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
					print_usage(stdout, 0);
					return 0;

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
					return print_usage(stderr, 1);

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
	if (string(argv[0]).find("buildpkg")==string(argv[0]).size()-strlen("buildpkg"))
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
	if ((string) argv[0] == "buildsrcpkg")
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
	//if (showHeader) ShowBanner();
	if ( optind < argc ) {
		if ( check_action( argv[optind++] ) == -1 )
		{
			return print_usage(stderr, 1);
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
			action == ACT_REMOVEGROUP)
	{
		if (do_reset) core.clean_queue();
	}
	if (htmlMode) {
		newHtmlPage();
		printHtml("MPKG запущен");
	}

	if ( action == ACT_NONE ) {
		return print_usage(stderr, 1);
	}

	if ( action == ACT_SHOW)
	{
		if (argc<=optind) return print_usage(stderr,1);
		
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
		if (argc!=optind) return print_usage(stderr,1);
		newHtmlPage();
		printHtml("Выполнение очереди намеченных ранее операций...");
		int ret = core.commit();
		unlockDatabase();
		newHtmlPage();
		if (ret) printHtml("При выполнении очереди возникли ошибки");
		else {
			printHtml("Очередь действий выполнена успешно");
			printHtmlRedirect();
		}
		return 0;
	}

	if (action == ACT_SHOWQUEUE)
	{
		if (argc!=optind) return print_usage(stderr,1);

		_cmdOptions["sql_readonly"]="yes";
		vector<string> list_empty;
		list(&core, list_empty,showOnlyAvailable, showOnlyInstalled, true);
		delete_tmp_files();
		return 0;
	}

	if (action == ACT_RESETQUEUE)
	{
		if (argc!=optind) return print_usage(stderr,1);

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
		if (argc!=optind) return print_usage(stderr,1);
		lockDatabase();
		dialogMode = true;
		actPackageMenu(core);
		unlockDatabase();
		return 0;
	}

	if (action == ACT_INSTALLFROMLIST)
	{
		if (argc<=optind) return print_usage(stderr,1);
		string filename = argv[optind];
		if (filename.find("http://")==0 || filename.find("ftp://")==0) {
			string tmp = get_tmp_file();
			unlink(tmp.c_str());
			system("wget " + filename + " -O " + tmp);
			filename = tmp;
		}
		if (!FileExists(filename)) {
			mError(_("File not found"));
			return 0;
		}
		lockDatabase();
		actInstallFromList(core, filename, exportinstalled_includeversions, enqueueOnly);
		delete_tmp_files();
		unlockDatabase();
		return 0;
	}				
	
	if (action == ACT_SHOWVERSION) {
		ShowBanner();
		return 0;
	}
	if (action == ACT_UPDATEALL || action == ACT_LISTUPGRADE)
	{
		actUpgrade(core, action);
		return 0;
	}

	if (action == ACT_BUILD)
	{
		if (argc<=optind) return print_usage(stderr,1);
		string s_pkg, build_dir_name;
		string march, mtune, olevel;
		for (int i=optind; i < argc; i++)
		{
			s_pkg = argv[i];
			if (s_pkg.find("march=")==0) {
				march=s_pkg.substr(strlen("march="));
				continue;
			}
			if (s_pkg.find("mtune=")==0) {
				mtune=s_pkg.substr(strlen("mtune="));
				continue;
			}

			if (s_pkg.find("olevel=")==0) {
				olevel=s_pkg.substr(strlen("olevel="));
				continue;
			}

			if (emerge_package(s_pkg, &s_pkg, march, mtune, olevel, &build_dir_name)!=0) {
				mError("Building of " + s_pkg + " was failed. Look into "+ build_dir_name + " for details");
				//delete_tmp_files();
				return 0;
			}
		}
		delete_tmp_files();
		return 0;
	}

	if (action == ACT_INSTALL || action == ACT_UPGRADE || action == ACT_REINSTALL)
	{

		vector<string> r_name, fname, p_version, p_build;
		string name, version, build;
		if (argc<=optind) return print_usage(stderr,1);
		lockDatabase();
		for (int i = optind; i < argc; i++) {
			r_name.push_back((string) argv[i]);
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
		if (action != ACT_REINSTALL) core.install(fname, &p_version, &p_build);
		else {
			for (unsigned int i=0; i<fname.size(); i++)
			{
				//printf("TODO: repair package %s\n", fname[i].c_str());
				core.repair(fname[i]);
			}
		}
		core.commit(enqueueOnly);
		if (!enqueueOnly) core.clean_queue();
		delete_tmp_files();

		if (usedCdromMount) system("umount " + CDROM_MOUNTPOINT + " 2>/dev/null >/dev/null");
		unlockDatabase();
		return 0;
	}

	if (action == ACT_CONFIG)
	{
		// TODO: Update to current state of possible config values
		if (argc!=optind) return print_usage(stderr,1);

		_cmdOptions["sql_readonly"]="yes";
		say(_("Current configuration:\n"));
		say(_("System root: %s\n"), SYS_ROOT.c_str());
		say(_("Package cache: %s\n"), SYS_CACHE.c_str());
		say(_("Run scripts: %d\n"), !DO_NOT_RUN_SCRIPTS);
		say(_("Database: %s\n"), DB_FILENAME.c_str());
		say(_("CD-ROM device: %s\n"), CDROM_DEVICE.c_str());
		say(_("CD-ROM mountpoint: %s\n"), CDROM_MOUNTPOINT.c_str());
		say(_("Scripts directory: %s\n"), SCRIPTS_DIR.c_str());
		list_rep(&core);
		return 0;
	}
	if (action == ACT_EXPORT)
	{

		_cmdOptions["sql_readonly"]="yes";
		string dest_dir=SYS_ROOT+"/"+legacyPkgDir;
		if (argc>optind) dest_dir=argv[optind];

		core.exportBase(dest_dir);
		return 0;
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
	if (action == ACT_TEST)
	{
		run_testing_facility(&core, "");
		/*
		if (argc<=optind) return print_usage(stderr,1);
		for (int i=optind; i<argc; ++i) {
			generateDeps_new(core, argv[i]);
		}*/
		return 0;
		
#ifdef RELEASE
		return print_usage(stderr,1);
#else
		return 0;
#endif
	}
	if (action == ACT_LISTDEPENDANTS)
	{
		_cmdOptions["sql_readonly"]="yes";
		if (argc<=optind) return print_usage(stderr, 1);
		if (!dialogMode) say(_("Searching for packages which depends on %s\n"),argv[optind]);
		actListDependants(core, argv[optind], showOnlyInstalled);
		return 0;
	}

	if (action == ACT_LISTGROUP)
	{
		_cmdOptions["sql_readonly"]="yes";
		if (argc<=optind) return print_usage(stderr,1);
		string group=argv[optind];
		PACKAGE_LIST pkgList1;
		PACKAGE_LIST pkgList2;
		SQLRecord sqlSearch;
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
		if (argc<=optind) return print_usage(stderr,1);
		lockDatabase();
		string group=argv[optind];
		PACKAGE_LIST pkgList1;
		vector<string> queue;
		SQLRecord sqlSearch;
		core.get_packagelist(sqlSearch, &pkgList1);
		for (unsigned int i=0; i<pkgList1.size(); i++)
		{
			if (pkgList1[i].isTaggedBy(group) && !pkgList1[i].installed()) queue.push_back(pkgList1[i].get_name());
		}
		core.install(queue);
		core.commit(enqueueOnly);
		if (!enqueueOnly) core.clean_queue();
		delete_tmp_files();

		unlockDatabase();
		

		return 0;
	}

	if (action == ACT_REMOVEGROUP)
	{
		if (argc<=optind) return print_usage(stderr,1);
		lockDatabase();
		string group=argv[optind];
		PACKAGE_LIST pkgList1;
		vector<string> queue;
		SQLRecord sqlSearch;
		core.get_packagelist(sqlSearch, &pkgList1);
		for (unsigned int i=0; i<pkgList1.size(); i++)
		{
			if (pkgList1[i].isTaggedBy(group) && pkgList1[i].installed()) queue.push_back(pkgList1[i].get_name());
		}
		core.uninstall(queue);

		hideErrors = false;
		int ret = core.commit(enqueueOnly);
		if (ret==MPKGERROR_IMPOSSIBLE) {
			printf("oops, ret = %d\n", ret);
		}
		if (!enqueueOnly) core.clean_queue();
		delete_tmp_files();
		unlockDatabase();
		return 0;

	}

	if (action == ACT_FILESEARCH)
	{
		_cmdOptions["sql_readonly"]="yes";
		if (argc<=optind) return print_usage(stderr,1);
		searchByFile(&core, argv[optind]);
		return 0;
	}


	if (action == ACT_WHICH)
	{
		_cmdOptions["sql_readonly"]="yes";
		if (argc<=optind) return print_usage(stderr,1);
		searchByFile(&core, argv[optind],true);
		return 0;
	}
	


	/*if (action == ACT_GENDEPS)
	{
		_cmdOptions["sql_readonly"]="yes";
		if (argc<=optind) return print_usage(stderr,1);
		for (int i=optind; i<argc; ++i) {
			generateDeps(argv[i]);
		}
		return 0;
	}*/
	if (action == ACT_GENDEPSNEW) {
		if (argc<=optind) return print_usage(stderr,1);

		_cmdOptions["sql_readonly"]="yes";
		for (int i=optind; i<argc; ++i) {
			generateDeps_new(core, argv[i]);
		}
		return 0;

	}
	if (action == ACT_CLEARDEPS) {
		_cmdOptions["sql_readonly"]="yes";
		if (argc<=optind) return print_usage(stderr,1);
		for (int i=optind; i<argc; ++i) {
			generateDeps(argv[i], false, true);
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
		core.get_packagelist(sqlSearch, &checkList);
		if (optind>=argc) core.db->get_full_filelist(&checkList);
		say(_("checking...\n"));
		string pkgname;
		if (optind>=argc)
		{
			//printf("mode 1\n");
			// Check entire system
			for (unsigned int i=0; i<checkList.size(); i++)
			{
				if (core.checkPackageIntegrity(checkList.get_package_ptr(i))) {
					if (verbose) {
						say("[%d/%d] ",i+1,checkList.size());
						say("%s: %sOK%s\n", checkList[i].get_name().c_str(), CL_GREEN, CL_WHITE);
					}
				}
				else 
				{
					//say("[%d/%d] ",i+1,checkList.size());
					//say(_("%s: %sDAMAGED%s\n"), checkList.get_package(i)->get_name()->c_str(), CL_RED, CL_WHITE);
					if (repair_damaged) 
					{
						//printf("Adding to repair queue\n");
						repairList.add(checkList[i]);
					}
				}
			}
		}
		else
		{
			int pkgIndex=-1;
			for (int i = optind; i<argc; i++)
			{
				pkgname = (string) argv[i];

				for (unsigned int t = 0; t<checkList.size(); t++)
				{
					if (checkList[t].get_name().find(pkgname)==0) {
						pkgIndex=t;
						break;
					}
				}
				
				if (core.checkPackageIntegrity(pkgname)) say("%s: %sOK%s\n", argv[i], CL_GREEN, CL_WHITE);
				else 
				{
					say(_("%s: %sDAMAGED%s\n"), argv[i], CL_RED, CL_WHITE);
					if (repair_damaged) 
					{
						//printf("Adding\n");
						repairList.add(checkList[pkgIndex]);
					}

				}
			}
		}
		for (unsigned int i=0; i<repairList.size(); i++)
		{
			//printf("repairing %d\n", i);
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
		if (argc<=optind) return print_usage(stderr,1);

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
		if (argc<=optind) return print_usage(stderr,1);
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
		if (argc!=optind) return print_usage(stderr,1);

		_cmdOptions["sql_readonly"]="yes";
		vector<string> list_empty;
		list(&core, list_empty, showOnlyAvailable, showOnlyInstalled);
		delete_tmp_files();
		return 0;
	}

	if ( action == ACT_UPDATE ) {
		if (argc!=optind) return print_usage(stderr,1);
		lockDatabase();
		actUpdate(core);
		unlockDatabase();
		return 0;
	}

	if ( action == ACT_CLEAN ) {
		if (argc!=optind) return print_usage(stderr,1);
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
		if (argc<=optind) return print_usage(stderr,1);

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
		if (action==ACT_REMOVE && do_purge==0) core.uninstall(r_name);
		else core.purge(r_name);
		core.commit(enqueueOnly);
		if (!enqueueOnly) core.clean_queue();
		delete_tmp_files();
		unlockDatabase();
		return 0;
	}

	if ( action == ACT_LIST_REP ) {
		_cmdOptions["sql_readonly"]="yes";
		if (argc!=optind) return print_usage(stderr,1);

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
		if (argc<=optind) return print_usage(stderr,1);
		for (int i=optind; i < argc; i++) {
			core.add_repository((string) argv[i]);
		}
		list_rep(&core);
		return 0;
	}
	if (action == ACT_REMOVE_REPOSITORY ) {
		_cmdOptions["sql_readonly"]="yes";
		if (argc<=optind) return print_usage(stderr,1);
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
		if (argc<=optind) return print_usage(stderr,1);
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
		if (argc<=optind) return print_usage(stderr,1);
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
			return print_usage(stderr,1);
		}
		
		if (!FileExists(argv[optind])) mError(_("Sync map not found\n"));
		core.syncronize_repositories((string) argv[optind]);
		return 0;
	}
	if (action == ACT_LATESTUPDATES) {
		_cmdOptions["sql_readonly"]="yes";
		int limit=-1;
		int skip = 0;
		if (argc>optind) limit = atoi(argv[optind]);
		if (argc>optind+1) skip = atoi(argv[optind+1]);
		if (htmlMode) {
			SQLRecord sqlSearch;
			sqlSearch.orderBy = "package_add_date";
			PACKAGE_LIST pkgList;
			core.get_packagelist(sqlSearch, &pkgList, true);
			if (limit == -1) limit = 0;
			else limit = pkgList.size() - limit;
			PACKAGE *pkg;
			string pkgChangelog, pkgShortDescription, pkgDescription, pkgRepTags;
			for (int i=pkgList.size()-1-skip; i>=0; --i) {
				pkg = pkgList.get_package_ptr(i);
				
				if (pkg->get_changelog()!="0") pkgChangelog = "<br><b><i>" + string(_("Changes:")) + " </i></b>" + toHtml(pkg->get_changelog());
				else pkgChangelog.clear();

				if (pkg->get_short_description()!="0") pkgShortDescription = ": " + pkg->get_short_description();
				else pkgShortDescription.clear();

				if (pkg->get_description()!="0") pkgDescription = toHtml(pkg->get_description());
				else pkgDescription.clear();
				if (!pkg->get_repository_tags().empty() && pkg->get_repository_tags()!="0") pkgRepTags = "<b><font color=\"blue\"> [" + pkg->get_repository_tags() + "] </b></font>";
				else pkgRepTags.clear();

				printf("<p><b>#%d</b> [%s]%s<br> <b><a href=\"show.php?id=%d\">%s %s</a>%s</b><br>%s%s</p>\n", i, getTimeString(pkg->add_date).c_str(), pkgRepTags.c_str(), pkg->get_id(), pkg->get_name().c_str(), pkg->get_fullversion().c_str(), pkgShortDescription.c_str(), pkgDescription.c_str(), pkgChangelog.c_str());
				if (i+skip==limit) break;
			}
		}
		if (rssMode) {
			printf("RSS support dropped from mpkg.");
		}
	}

	unlockDatabase();
	return 0;
}


int print_usage(FILE* stream, int exit_code)
{
	ShowBanner();
	fprintf(stream, _("\nUsage: %s [options] ACTION [package] [package] ...\n"), program_name);
	fprintf(stream,_("Options:\n"));
	fprintf(stream,_("\t-h    --help              show this help\n"));
	fprintf(stream,_("\t-v    --verbose           be verbose\n"));
	fprintf(stream,_("\t-g    --dialog            use dialog mode UI\n"));
	fprintf(stream,_("\t-d    --force-dep         interpret dependency errors as warnings\n"));
	fprintf(stream,_("\t-z    --no-dep            totally ignore dependencies existance\n"));
	fprintf(stream,_("\t-f    --force-conflicts   do not perform file conflict checking\n"));
	fprintf(stream,_("\t-m    --no-md5            do not check package integrity on install\n"));
	fprintf(stream,_("\t-k    --force-essential   allow removing essential packages\n"));
	fprintf(stream,_("\t-D    --download-only     just download packages, do not install\n"));
	fprintf(stream,_("\t-r    --repair            repair damaged packages (use with \"check\" keyword)\n"));
	fprintf(stream,_("\t-i    --installed         show only installed packages (use with \"list\" keyword)\n"));
	fprintf(stream,_("\t-a    --available         show only available packages (use with \"list\" keyword)\n"));
	fprintf(stream,_("\t-l    --filelist          show file list for package (with \"show\" keyword)\n"));
	fprintf(stream,_("\t-y    --noconfirm         don't ask confirmation\n"));
	fprintf(stream,_("\t-q    --noreset           don't reset queue at start\n"));
	fprintf(stream,_("\t-N    --nodepgen          don't generate dependencies on package building\n"));
	fprintf(stream,_("\t-w    --no_buildcache     don't use source cache when building packages\n")); 
	fprintf(stream,_("\t-c    --no_resume         disable download resuming\n"));
	fprintf(stream,_("\t-b    --cleartags         clear all other tags before tagging\n"));
	fprintf(stream,_("\t-F    --fork              detach from terminal and run in background\n"));
	fprintf(stream,_("\t-Q    --enqueue           only enqueue actions, do not perform\n"));
	fprintf(stream,_("\t-H    --html              switch to html mode (useful only for integrate into web)\n"));
	fprintf(stream,_("\t-V    --enableversions    enable versioning in install lists\n"));
	fprintf(stream,_("\t-R    --index-filelist    create file list while indexing\n"));
	fprintf(stream,_("\t-M    --md5index          add md5 sums for each file in filelist\n"));
	fprintf(stream,_("\t-S    --sysroot=<DIR>     set temporary system root\n"));
	fprintf(stream,_("\t-t    --conf=CONFIG_FILE  set temporary config file\n"));
	fprintf(stream,_("\t-A    --download-to=<DIR> download packages to specified directory\n"));
	fprintf(stream,_("\t-W    --enable-spkg-index enable spkg indexing (otherwise, it will be skipped)\n"));
	fprintf(stream,_("\t-L    --linksonly         on install/upgradeall, show only download links and exit\n"));
	fprintf(stream,_("\t-P    --noaaa             try to avoid aaa_elflibs in dependency generation (for gendeps2)\n"));
	fprintf(stream,_("\t-G    --keep-deps         do not replace deps in package, add to them instead (for gendeps2)\n"));
	fprintf(stream,_("\t-C    --enable-cache      enable cached indexing\n"));
	fprintf(stream,_("\t-K    --skip-deprecated   while sync, skip deprecated packages\n"));
	fprintf(stream,_("\t-s    --keep-symlinks     keep symlinks in archive instead of moving it to doinst.sh\n"));
	fprintf(stream,_("\t-E    --exclusion-list=FILENAME   Exclude paths from being checked by gendeps2\n"));

	
	fprintf(stream,_("\nActions:\n"));
	fprintf(stream,_("\tversion                            show mpkg version\n"));
	fprintf(stream,_("\tbuild [march=CPU] [mtune=CPU] [olevel={ O0 | O1 | O2 | O3 }] FILENAME     build source package(s)\n"));
	fprintf(stream,_("\tinstall PACKAGE_NAME | FILENAME    install package(s)\n"));
	fprintf(stream,_("\tupgrade PACKAGE_NAME | FILENAME    upgrade selected package\n"));
	fprintf(stream,_("\tupgradeall                         upgrade all installed packages\n"));
	fprintf(stream,_("\treinstall PACKAGE_NAME             reinstall package(s)\n"));
	fprintf(stream,_("\tremove PACKAGE_NAME                remove selected package(s)\n"));
	fprintf(stream,_("\tpurge PACKAGE_NAME                 purge selected package(s)\n"));
	fprintf(stream,_("\tinstallgroup GROUP_NAME            install all the packages from group\n"));
	fprintf(stream,_("\tremovegroup GROUP_NAME             remove all the packages from group\n"));

	fprintf(stream,_("\tlistupdates                        list available updates\n"));
	fprintf(stream,_("\tshow PACKAGE_NAME                  show info about package\n"));
	fprintf(stream,_("\tupdate                             update packages info\n"));
	fprintf(stream,_("\tlist                               show the list of all packages in database\n"));
	fprintf(stream,_("\tlistgroup GROUP_NAME               show the list of packages belonged to group\n"));
	fprintf(stream,_("\tlistgroups                         show the list of all existing groups\n"));
	fprintf(stream,_("\twhodepend PACKAGE_NAME             show what packages depends on this one\n"));
	
	fprintf(stream,_("\tfilesearch FILE_NAME               look for owner of the file in installed packages (LIKE mode).\n"));
	fprintf(stream,_("\twhich FILE_NAME                    look for owner of the file in installed packages (EQUAL mode).\n"));
	fprintf(stream,_("\tlist_rep                           list enabled repositories\n"));
	fprintf(stream,_("\tadd_rep URL                        add repository\n"));
	fprintf(stream,_("\tdelete_rep REPOSITORY_NUMBER       delete Nth repository\n"));
	fprintf(stream,_("\tenable_rep REPOSITORY_NUMBER       enable Nth repository\n"));
	fprintf(stream,_("\tdisable_rep REPOSITORY_NUMBER      disable Nth repository\n"));
	fprintf(stream,_("\tgetrepositorylist [LIST_URL]       get the list of repositories from server (by default, from rpunet.ru)\n"));
	fprintf(stream,_("\tinstallfromlist FILE_NAME          install using file with list of items\n"));
	fprintf(stream,_("\texportinstalled [FILE_NAME]        export list of installed packages to file or stdout\n"));
	fprintf(stream,_("\treset                              reset queue\n"));
	fprintf(stream,_("\tshow_queue                         show queue\n"));
	fprintf(stream,_("\tcommit                             commit queued actions\n"));
	fprintf(stream,_("\tsearch PATTERN                     search package by name containing PATTERN\n"));
	fprintf(stream,_("\tsearchdescription PATTERN          search package by description containing PATTERN\n"));
	fprintf(stream,_("\tclean                              remove all downloaded packages from cache\n"));
	fprintf(stream,_("\tcheck [package_name]               checks installed package(s) for damaged files. Use -r flag to to repair\n"));

	fprintf(stream,_("\nInteractive options:\n"));
	fprintf(stream,_("\tmenu                      shows the package selection menu\n"));
	
	fprintf(stream,_("\nRepository maintaining functions:\n"));
	fprintf(stream,_("\tindex                     create a repository index file \"packages.xml.gz\"\n"));
	fprintf(stream,_("\tconvert_dir <outp_dir>    convert whole directory (including sub-dirs) to MPKG format\n"));
	fprintf(stream,_("\tconvert <filename>        convert package to MPKG format\n"));
	fprintf(stream,_("\tnativize [dir]            search directory for non-native packages and convert it to MPKG format\n"));
	fprintf(stream,_("\texport [dir]              export database in slackware format to dir (by default, /var/log/packages/)\n"));
	fprintf(stream,_("\tgendeps <filename(s)>         generate dependencies and import it into package\n"));
	fprintf(stream,_("\tgendeps2 <filename(s)>         generate dependencies and import it into package (new algorithm\n"));
	fprintf(stream,_("\tcleardeps <filename>         clear package dependencies\n"));
	fprintf(stream,_("\tchecklist [dir]           check md5 sums of a package tree (requires a valid index in this dir)\n"));
	fprintf(stream,_("\tsync <syncmap file>       syncronize repositories by sync map\n"));
	fprintf(stream,_("\tbuildup <filename>        increase spkg/tgz build\n"));
#ifndef RELEASE
	fprintf(stream,_("\nDebug options:\n"));
	fprintf(stream,_("\ttest                      Executes unit test\n"));
#endif
	fprintf(stream,_("\nExtra options for command \"build\" (should be specified _before_ package name):\n"));
	fprintf(stream,_("\tmarch=                    CPU architecture\n"));
	fprintf(stream,_("\tmtune=                    CPU tuning\n"));
	fprintf(stream,_("\tolevel=                   Optimization level\n"));
	fprintf(stream,_("Example: mpkg build march=i686 mtune=prescott olevel=O3\n"));
	fprintf(stream, "\n");


	return exit_code;
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
		cout << " [" << i+rlist.size()+1 << "] " << dlist[i] << " (" << _("DISABLED") << ")\n" << endl;
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
	if ( _act == "lastupdates") return ACT_LATESTUPDATES;
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
	if (_act == "export")
		return ACT_EXPORT;
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


