/*********************************************************************
 * MPKG packaging system: library interface
 * $Id: libmpkg.cpp,v 1.69 2007/12/04 18:48:34 i27249 Exp $
 * ******************************************************************/

#include "libmpkg.h"
#include "deltas.h"
#include "terminal.h"
#include <iostream>
mpkgDatabase *_globalDB = NULL;
int _globalDBUsers = 0;
string getErrorDescription(int errCode) { // FIXME: LOCALIZATION!!!
	switch(errCode) {
		case MPKGERROR_OK:
			return "OK";
		case MPKGERROR_NOPACKAGE:
			return "Нет такого пакета";
		case MPKGERROR_IMPOSSIBLE:
			return "Запрошенное действие невозможно";
		case MPKGERROR_NOSUCHVERSION:
			return "Нет такой версии";
		case MPKGERROR_COMMITERROR:
			return "Ошибка при установке или удалении пакетов";
		case MPKGERROR_UNRESOLVEDDEPS:
			return "Неразрешенные зависимости";
		case MPKGERROR_SQLQUERYERROR:
			return "Ошибка в SQL-запросе";
		case MPKGERROR_DOWNLOADERROR:
			return "Ошибка при закачке";
		case MPKGERROR_ABORTED:
			return "Отменено";
		case MPKGERROR_CRITICAL:
			return "Критическая ошибка";
		case MPKGERROR_AMBIGUITY:
			return "Неоднозначность";
		case MPKGERROR_INCORRECTDATA:
			return "Некорректные данные";
		case MPKGERROR_FILEOPERATIONS:
			return "Ошибка при работе с файлами";
		default:
			return "Неизвестный код ошибки: " + IntToStr(errCode);
	}
}


mpkg::mpkg(bool _loadDatabase)
{
	mDebug("creating core");
	currentStatus=_("Loading database...");
	char *home_dir = getenv("HOME");
	if (home_dir && FileExists(string(home_dir) + "/.mpkg", NULL) && !isDirectory(string(home_dir) + "/.mpkg")) {
		//printf("Home config exists\n");
		string tmp = ReadFile(string(home_dir) + "/.mpkg");
		if (tmp.find_first_of("\n\t")!=std::string::npos) {
			tmp = tmp.substr(0,tmp.find_first_of("\n\t"));
			if (FileExists(tmp)) {
				//printf("Custom config file found: %s\n", tmp.c_str());
				CONFIG_FILE = tmp;
				mConfig.configName = tmp;
			}
		}

	}
	loadGlobalConfig();
	db=NULL;
	DepTracker=NULL;
	if (_loadDatabase)
	{
		mDebug("Loading database");
		if (!_globalDB) {
			db = new mpkgDatabase();
			_globalDB = db;
		}
		else db = _globalDB;
		_globalDBUsers++;

		DepTracker = new DependencyTracker(db);
	}
	init_ok=true;
	currentStatus = _("Database loaded");
	// Initializing system mode
	if (mConfig.getValue("disable_dependencies")=="yes") ignoreDeps = true;
	if (mConfig.getValue("disable_dependencies").empty()) mConfig.setValue("disable_dependencies", "no");
}

mpkg::~mpkg()
{
	if (DepTracker!=NULL) delete DepTracker;
	if (db!=NULL) {
		_globalDBUsers--;
		if (_globalDBUsers==0) {
			delete db;
			_globalDB = NULL;
		}
	}
	delete_tmp_files();
	mpkgSys::clean_cache(true);
	mDebug("Deleting core");

}

string mpkg::current_status()
{
	return currentStatus;
}

void mpkg::clean_queue() {
	mpkgSys::clean_queue(db);
}

int mpkg::unqueue(int package_id)
{
	return mpkgSys::unqueue(package_id, db);
}

// Package building
int mpkg::build_package(string out_directory, bool source)
{
	return mpkgSys::build_package(out_directory, source);
}
int mpkg::convert_directory(string output_dir)
{
	return mpkgSys::convert_directory(output_dir);
}

void mpkg::get_available_tags(vector<string>* output)
{
	db->get_available_tags(output);
}
	
// Packages installation
int mpkg::install(vector<string> fname, vector<string> *p_version, vector<string> *p_build, vector<string> *eList) // This is the main function
{
	/*for (unsigned int i=0; i<fname.size(); ++i) {
		printf("[%s]\n", fname[i].c_str());
	}*/
	//if (p_version) printf("using versionz\n");	
	int ret=0;
	vector<string> version, build;
	if (p_version!=NULL) version = *p_version;
	else version.resize(fname.size());
	if (p_build!=NULL) build = *p_build;
	else build.resize(fname.size());
	if (mpkgSys::requestInstall(fname, version, build, db, DepTracker, eList)!=0) ret--;

	msay("", SAYMODE_NEWLINE);
	return ret;
}
int mpkg::installGroups(vector<string> groupName)
{
	for (unsigned int i=0; i<groupName.size(); i++)
	{
		mpkgSys::requestInstallGroup(groupName[i], db, DepTracker);
	}
	return 0;
}
int mpkg::install(string fname)
{
	currentStatus = _("Building queue: ")+fname;
	vector<string> queue, v, b;
	queue.push_back(fname);
	return mpkgSys::requestInstall(queue, v, b, db, DepTracker);
}
// Should be deprecated?
int mpkg::install(PACKAGE *pkg)
{
	vector<string> queue, v, b;
	queue.push_back(pkg->get_name());
	v.push_back(pkg->get_version());
	b.push_back(pkg->get_build());

	return mpkgSys::requestInstall(queue, v, b, db, DepTracker);
}

// DEFINITELY DEPRECATED
/*int mpkg::install(int package_id)
{
	return mpkgSys::requestInstall(package_id, db, DepTracker);
}*/

int mpkg::install(PACKAGE_LIST *pkgList)
{
	// New backend usage
	vector<string> name, version, build;
	for (unsigned int i=0; i<pkgList->size(); i++) {
		name.push_back(pkgList->at(i).get_name());
		version.push_back(pkgList->at(i).get_version());
		build.push_back(pkgList->at(i).get_build());
	}
	return mpkgSys::requestInstall(name, version, build, db, DepTracker);
}

// Packages removing
int mpkg::uninstall(vector<string> pkg_name)
{
	int ret=0;
	for (unsigned int i = 0; i < pkg_name.size(); i++)
	{
		currentStatus = _("Building queue: ")+IntToStr(i) + "/" +IntToStr(pkg_name.size()) +" ["+pkg_name[i]+"]";
		if (mpkgSys::requestUninstall(pkg_name[i], db, DepTracker)!=0) ret--;
	}
	return ret;
}
int mpkg::uninstall(int package_id) {
	return mpkgSys::requestUninstall(package_id, db, DepTracker);
}
int mpkg::uninstall(PACKAGE_LIST *pkgList) {
	return mpkgSys::requestUninstall(pkgList, db, DepTracker);
}
// Packages purging
int mpkg::purge(vector<string> pkg_name)
{
	int ret=0;
	for (unsigned int i = 0; i < pkg_name.size(); i++)
	{
		currentStatus = _("Building queue: ")+IntToStr(i) + "/" +IntToStr(pkg_name.size()) +" ["+pkg_name[i]+"]";
		if (mpkgSys::requestUninstall(pkg_name[i], db, DepTracker, true)!=0) ret--;
	}
	return ret;
}

int mpkg::purge(int package_id) {
	return mpkgSys::requestUninstall(package_id, db, DepTracker, true);
}
// Repository data updating

void mpkg::cleanCdromCache(string s_root)
{
	if (s_root.empty()) s_root = SYS_ROOT;
	system("rm -rf " + s_root + "/var/mpkg/index_cache/*");
}
int mpkg::update_repository_data()
{
	if (mpkgSys::update_repository_data(db) == 0 && db->sqlFlush()==0)
	{
		cleanCdromCache();
		currentStatus = _("Repository data updated");
		return 0;
	}
	else 
	{
		currentStatus = _("Repository data update failed");
		return -1;
	}
}

// Cache cleaning
int mpkg::clean_cache(bool clean_symlinks)
{
	currentStatus = _("Cleaning cache...");
	int ret = mpkgSys::clean_cache(clean_symlinks);
	if (ret == 0) currentStatus = _("Cache cleanup complete");
	else currentStatus = _("Error cleaning cache!");
	return ret;
}

// Package list retrieving
int mpkg::get_packagelist(SQLRecord& sqlSearch, PACKAGE_LIST *packagelist, bool ultraFast, bool needDescriptions)
{
	currentStatus = _("Retrieving package list...");
	int ret = db->get_packagelist(sqlSearch, packagelist, ultraFast, needDescriptions);
	if (ret == 0) currentStatus = _("Retriveal complete");
	else currentStatus = _("Failed retrieving package list!");
	return ret;
}

// Configuration and settings: retrieving
vector<string> mpkg::get_repositorylist()
{
	return mpkgconfig::get_repositorylist();
}
vector<string> mpkg::get_disabled_repositorylist()
{
	return mpkgconfig::get_disabled_repositorylist();
}

string mpkg::get_sysroot()
{
	return mpkgconfig::get_sysroot();
}
string mpkg::get_syscache()
{
	return mpkgconfig::get_syscache();
}
string mpkg::get_dburl()
{
	return mpkgconfig::get_dburl();
}

string mpkg::get_cdromdevice()
{
	return mpkgconfig::get_cdromdevice();
}

string mpkg::get_cdrommountpoint()
{
	return mpkgconfig::get_cdrommountpoint();
}

string mpkg::get_scriptsdir()
{
	return mpkgconfig::get_scriptsdir();
}
bool mpkg::get_runscripts()
{
	return mpkgconfig::get_runscripts();
}

unsigned int mpkg::get_checkFiles()
{
	return mpkgconfig::get_checkFiles();
}

int mpkg::set_checkFiles(unsigned int value)
{
	return mpkgconfig::set_checkFiles(value);
}

int mpkg::add_repository(string repository_url) {
	// First of all, validate repository URL
	if (repository_url.empty()) return 0; // Drop empty ones
	if (repository_url[repository_url.size()-1]!='/') repository_url += "/"; // Automatically add closing slash
	if (repository_url[0]=='/') repository_url = "file://" + repository_url; // Automatically form file:// repositories
	vector<string> enabledRepositories, disabledRepositories, n1;
	enabledRepositories = get_repositorylist();
	disabledRepositories = get_disabled_repositorylist();
	// Check if it is already there
	for (unsigned int i=0; i<enabledRepositories.size(); i++) {
		if (enabledRepositories[i]==repository_url) {
			say(_("Repository %s is already in list\n"), repository_url.c_str());
			return 0; // Already there
		}
	}
	for (unsigned int i=0; i<disabledRepositories.size(); i++) {
		if (disabledRepositories[i]==repository_url) {
			enabledRepositories.push_back(repository_url);
			for (unsigned int t=0; t<disabledRepositories.size(); t++) {
				if (t!=i) n1.push_back(disabledRepositories[i]);
			}
			set_repositorylist(enabledRepositories, n1);
			return 0;
		}
	}
	enabledRepositories.push_back(repository_url);
	return set_repositorylist(enabledRepositories, disabledRepositories);
}

int mpkg::remove_repository(int repository_num)
{
	if (repository_num==0) {
		mError("No such repository"); 
		return -1;
	}
	repository_num--;
	vector<string> enabledRepositories, disabledRepositories, n1;
	enabledRepositories = get_repositorylist();
	disabledRepositories = get_disabled_repositorylist();
	if (repository_num >= (int) enabledRepositories.size())
	{
		repository_num = repository_num - enabledRepositories.size();
		if (repository_num>=(int) disabledRepositories.size()) {
			say(_("No such repository\n"));
			return -1;
		}
		for (unsigned int i=0; i<disabledRepositories.size(); i++)
		{
			if (i!=(unsigned int) repository_num) n1.push_back(disabledRepositories[i]);
		}
		return set_repositorylist(enabledRepositories, n1);
	}
	else
	{
		for (unsigned int i=0; i<enabledRepositories.size(); i++)
		{
			if (i!=(unsigned int) repository_num) n1.push_back(enabledRepositories[i]);
		}
		return set_repositorylist(n1, disabledRepositories);
	}
}
int mpkg::enable_repository(vector<int> rep_nums)
{
	vector<string> enabledRepositories, disabledRepositories, tmp;
	enabledRepositories = get_repositorylist();
	disabledRepositories = get_disabled_repositorylist();
	vector<bool> flags;

	for (unsigned int i=0; i<enabledRepositories.size(); ++i) {
		tmp.push_back(enabledRepositories[i]);
		flags.push_back(true);
	}
	for (unsigned int i=0; i<disabledRepositories.size(); ++i) {
		tmp.push_back(disabledRepositories[i]);
		flags.push_back(false);
	}
	for (unsigned int i=0; i<rep_nums.size(); ++i) {
		if (rep_nums[i]>=(int) flags.size() || rep_nums[i]<0) {
			mError(_("No such repository: ") + IntToStr(rep_nums[i] + 1));
			continue;
		}
		flags[rep_nums[i]]=true;
	}
	enabledRepositories.clear();
	disabledRepositories.clear();
	for (unsigned int i=0; i<flags.size(); ++i) {
		if (flags[i]) enabledRepositories.push_back(tmp[i]);
		else disabledRepositories.push_back(tmp[i]);
	}


	set_repositorylist(enabledRepositories, disabledRepositories);



	return 0;
}

int mpkg::disable_repository(vector<int> rep_nums)
{
	vector<string> enabledRepositories, disabledRepositories, tmp;
	enabledRepositories = get_repositorylist();
	disabledRepositories = get_disabled_repositorylist();
	vector<bool> flags;

	for (unsigned int i=0; i<enabledRepositories.size(); ++i) {
		tmp.push_back(enabledRepositories[i]);
		flags.push_back(true);
	}
	for (unsigned int i=0; i<disabledRepositories.size(); ++i) {
		tmp.push_back(disabledRepositories[i]);
		flags.push_back(false);
	}
	for (unsigned int i=0; i<rep_nums.size(); ++i) {
		if (rep_nums[i]>=(int) flags.size() || rep_nums[i]<0) {
			mError(_("No such repository: ") + IntToStr(rep_nums[i] + 1));
			continue;
		}
		flags[rep_nums[i]]=false;
	}
	enabledRepositories.clear();
	disabledRepositories.clear();
	for (unsigned int i=0; i<flags.size(); ++i) {
		if (flags[i]) enabledRepositories.push_back(tmp[i]);
		else disabledRepositories.push_back(tmp[i]);
	}


	set_repositorylist(enabledRepositories, disabledRepositories);

	return 0;
}
// Configuration and settings: setting
int mpkg::set_repositorylist(vector<string> newrepositorylist, vector<string> drList)
{
	return mpkgconfig::set_repositorylist(newrepositorylist, drList);
}
int mpkg::set_sysroot(string newsysroot)
{
	return mpkgconfig::set_sysroot(newsysroot);
}

int mpkg::set_syscache(string newsyscache)
{
	return mpkgconfig::set_syscache(newsyscache);
}
int mpkg::set_dburl(string newdburl)
{
	return mpkgconfig::set_dburl(newdburl);
}

int mpkg::set_cdromdevice(string cdromDevice)
{
	return mpkgconfig::set_cdromdevice(cdromDevice);
}

int mpkg::set_cdrommountpoint(string cdromMountPoint)
{
	return mpkgconfig::set_cdrommountpoint(cdromMountPoint);
}


int mpkg::set_scriptsdir(string newscriptsdir)
{
	return mpkgconfig::set_scriptsdir(newscriptsdir);
}
int mpkg::set_runscripts(bool dorun)
{
	return mpkgconfig::set_runscripts(dorun);
}

// Finalizing
int mpkg::commit(bool deferred)
{
	mDebug("committing");
	if (!dialogMode) msay(_("Building dependency tree"));
	else {
		ncInterface.showInfoBox(_("Building dependency tree"));
	}

	//currentStatus = _("Checking dependencies...");
	int errorCount;
	errorCount = DepTracker->renderData();
	if (errorCount !=0) {
	       if (!dialogMode) say("\n%s:\n%s", _("Dependency error"), depErrorTable.print().c_str());
	       else {
		       ncInterface.setSubtitle(_("Error: unresolved dependencies"));
		       string depFailList;
		       for (unsigned int i=0; i<DepTracker->get_failure_list().size(); ++i) {
			       depFailList += DepTracker->get_failure_list().at(i).get_name() + "-" + DepTracker->get_failure_list().at(i).get_fullversion() + "\n";
		       }
		       if (setupMode) {
			       system("cat /tmp/depErrTable.log > /dev/tty4 2>/dev/null");
		       }

		       ncInterface.showMsgBox(IntToStr(DepTracker->get_failure_list().size()) + "/" + IntToStr(depErrorTable.packageID.size()) + _(" packages has unresolvable dependencies. See full log in /tmp/depErrTable.log\n") + depFailList + _("\nDetails:\n") + depErrorTable.print());
	       }
	}
	if (errorCount==0)
	{
		if (!dialogMode) msay(_("Building queue"));
		else {
			ncInterface.showInfoBox(_("Building actions queue"));
		}
		mDebug("Tracking deps");
		
		if (!DepTracker->commitToDb()) {
			//mError("Impossible");
			return MPKGERROR_IMPOSSIBLE;
		}

		if (!dialogMode) msay(_("Committing..."));
		else {
			ncInterface.showInfoBox(_("Committing..."));
		}
		currentStatus = _("Committing changes...");
		if (deferred) {
			printf("deferred\n");
			return 0;
		}
		int ret = db->commit_actions();
		if (ret==0) {
			if (!dialogMode) say(_("Completed successfully\n"));
		}
		else 
		{
			switch (ret)
			{
				case MPKGERROR_ABORTED: 
					if (!dialogMode) say (_("Aborted\n"));
					break;
				case MPKGERROR_COMMITERROR:
				default:
					mError(_("Commit failed, error code: ") + getErrorDescription(ret));
					if (dialogMode) ncInterface.showMsgBox(_("Failed to execute requested operations: ") + getErrorDescription(ret));
			}
		}
		currentStatus = _("Complete.");
		ncInterface.showInfoBox(_("Complete."));
		return ret;
	}
	else
	{
	/*	if (!dialogMode) mError(_("Error in dependencies: ") + IntToStr(errorCount) + _(" failures"));
		else ncInterface.showMsgBox(_("Error in dependencies: ") + IntToStr(errorCount) + _(" failures"));*/
		currentStatus = _("Failed - depencency errors");
		mError(_("Failed - dependency errors"));
		return MPKGERROR_UNRESOLVEDDEPS;
	}
}

bool mpkg::checkPackageIntegrity(const string& pkgName)
{
	SQLRecord sqlSearch;
	sqlSearch.addField("package_name", pkgName);
	sqlSearch.addField("package_installed", ST_INSTALLED);
	PACKAGE_LIST table;
	//printf("SLOW GET_PACKAGELIST CALL: %s %d\n", __func__, __LINE__);
	get_packagelist(sqlSearch, &table, false, false);
	if (table.size()==0)
	{
		mError(_("Cannot check package \"") + pkgName + _("\": it isn't installed"));
		return false;
	}
	if (table.size()!=1)
	{
		mError(_("Received ") + IntToStr(table.size()) + _(" packages, ambiguity!"));
		return false;
	}
	return checkPackageIntegrity(table.get_package_ptr(0));
}

bool mpkg::checkPackageIntegrity(PACKAGE *package)
{
	bool checkIntegrity_includeSymlinks = false;
	string package_name = package->get_name();

	if (package->get_files().empty()) db->get_filelist(package->get_id(), package->get_files_ptr());
	// Function description: 
	// 	1. Check files for exist
	// 	2. Check files for integrity (md5 verify)
	// 	3. Access rights check (hm...)
	bool integrity_ok = true;
	bool broken_sym = false;
	for (unsigned int i=0; i<package->get_files().size(); i++)
	{
		if (package->get_files().at(i).rfind(".new")==package->get_files().at(i).length()-strlen(".new")) continue; // Skip config files
		if (package->get_files().at(i).find("dev/")==0) continue; // Skip /dev section - all of this files are managed by udev
		if (package_name.find("glibc")==0 && package->get_files().at(i).find("lib/incoming/")!=std::string::npos) continue;
		if (!FileExists(SYS_ROOT + "/" + package->get_files().at(i), &broken_sym)) {
			if (integrity_ok) { 
				if (!broken_sym || checkIntegrity_includeSymlinks) {
					mError(_("Package ") + (string) CL_YELLOW + package->get_name() + (string) CL_WHITE + _(" has broken files or symlinks:"));
					integrity_ok = false;
				}
			}
			if (!integrity_ok) {
				if (!broken_sym) 
					say(_("%s%s%s: /%s (file doesn't exist)\n"), CL_YELLOW, package->get_name().c_str(),CL_WHITE, package->get_files().at(i).c_str());
				else
					if (checkIntegrity_includeSymlinks) say(_("%s%s%s: /%s (broken symlink)\n"), CL_YELLOW, package->get_name().c_str(),CL_WHITE, package->get_files().at(i).c_str());
			}
		}
	}
	return integrity_ok;
}

bool mpkg::repair(string fname)
{
	SQLRecord sqlSearch;
	sqlSearch.addField("package_name", fname);
	sqlSearch.addField("package_installed", 1);
	PACKAGE_LIST p;
	//printf("SLOW GET_PACKAGELIST CALL: %s %d\n", __func__, __LINE__);
	get_packagelist(sqlSearch, &p, true, false);
	if (p.size()==1)
	{
		return repair(p.get_package_ptr(0));
	}
	else {
		say(_("Cannot repair or reinstall package %s: it is not installed\n"), fname.c_str());
		return false;
	}
}
bool mpkg::repair(PACKAGE *package)
{
	if (!package->available())
	{
		mError(_("Cannot repair ") + package->get_name() + _(": package is unavailable"));
		return false;
	}
	db->set_action(package->get_id(), ST_REPAIR, "repair");
	return true;
}
/*void mpkg::exportBase(string output_dir)
{
	say("Exporting data to %s directory\n",output_dir.c_str());
	// First of all, clear previous contents
	system("rm -rf " + output_dir+"; mkdir -p " + output_dir);
	PACKAGE_LIST allPackages;
	SQLRecord sqlSearch;
	sqlSearch.addField("package_installed", 1);
	//printf("SLOW GET_PACKAGELIST CALL: %s %d\n", __func__, __LINE__);
	get_packagelist(sqlSearch, &allPackages, true, true);
	say("Received %d packages\n",allPackages.size());
	db->get_full_filelist(&allPackages);
	for (unsigned int i=0; i<allPackages.size(); i++)
	{
		say("[%d/%d] Exporting package %s\n",i+1,allPackages.size(),allPackages[i].get_name().c_str());
		db->exportPackage(output_dir, *allPackages.get_package_ptr(i));
	}
}*/

void dumpPackage(PACKAGE *p, PackageConfig *pc, string filename)
{
	XMLNode node = XMLNode::createXMLTopNode("package");
	node.addChild("name");
	node.getChildNode("name").addText(p->get_name().c_str());
	node.addChild("version");
	node.getChildNode("version").addText(p->get_version().c_str());
	node.addChild("arch");
	node.getChildNode("arch").addText(p->get_arch().c_str());
	node.addChild("build");
	node.getChildNode("build").addText(p->get_build().c_str());
	if (!p->get_provides().empty()) {
		node.addChild("provides").addText(p->get_provides().c_str());
	}
	if (!p->get_conflicts().empty()) {
		node.addChild("conflicts").addText(p->get_conflicts().c_str());
	}

	node.addChild("description");
	node.getChildNode("description").addText(p->get_description().c_str());
	node.addChild("short_description");
	node.getChildNode("short_description",0).addText(p->get_short_description().c_str());
	
	node.addChild("dependencies");
	node.addChild("suggests");
	for (unsigned int i=0; i<p->get_dependencies().size(); i++)
	{
		node.getChildNode("dependencies").addChild("dep");
		node.getChildNode("dependencies").getChildNode("dep", i).addChild("name");
		node.getChildNode("dependencies").getChildNode("dep", i).getChildNode("name").addText(p->get_dependencies().at(i).get_package_name().c_str());
		node.getChildNode("dependencies").getChildNode("dep", i).addChild("condition");
		node.getChildNode("dependencies").getChildNode("dep", i).getChildNode("condition").addText(condition2xml(p->get_dependencies().at(i).get_condition()).c_str());
		node.getChildNode("dependencies").getChildNode("dep", i).addChild("version");
		node.getChildNode("dependencies").getChildNode("dep", i).getChildNode("version").addText(p->get_dependencies().at(i).get_package_version().c_str());
	}
	node.addChild("tags");
	node.addChild("changelog");

	for (unsigned int i=0; i<p->get_tags().size(); i++)
	{
		node.getChildNode("tags").addChild("tag");
		node.getChildNode("tags").getChildNode("tag",i).addText(p->get_tags().at(i).c_str());
	}

	node.getChildNode("changelog").addText(p->get_changelog().c_str());
	node.addChild("maintainer");
	node.getChildNode("maintainer").addChild("name");
	node.getChildNode("maintainer").getChildNode("name").addText(p->get_packager().c_str());
	node.getChildNode("maintainer").addChild("email");
	node.getChildNode("maintainer").getChildNode("email").addText(p->get_packager_email().c_str());
	
	node.addChild("mbuild");
	if (!pc->getBuildUrl().empty())
	{
		node.getChildNode("mbuild").addChild("url");
		node.getChildNode("mbuild").getChildNode("url").addText(pc->getBuildUrl().c_str());
	}
	if (!pc->getBuildPatchList().empty())
	{
		node.getChildNode("mbuild").addChild("patches");
		for (unsigned int i=0; i<pc->getBuildPatchList().size(); i++)
		{
			node.getChildNode("mbuild").getChildNode("patches").addChild("patch");
			node.getChildNode("mbuild").getChildNode("patches").getChildNode("patch",i).addText(pc->getBuildPatchList().at(i).c_str());
		}
	}
	if (!pc->getBuildSourceRoot().empty())
	{
		node.getChildNode("mbuild").addChild("sources_root_directory");
		node.getChildNode("mbuild").getChildNode("sources_root_directory").addText(pc->getBuildSourceRoot().c_str());
	}
	if (!pc->getBuildSystem().empty())
	{
		node.getChildNode("mbuild").addChild("build_system");
		{
			node.getChildNode("mbuild").getChildNode("build_system").addText(pc->getBuildSystem().c_str());
		}
	}
	if (!pc->getBuildMaxNumjobs().empty())
	{
		node.getChildNode("mbuild").addChild("max_numjobs");
		node.getChildNode("mbuild").getChildNode("max_numjobs").addText(pc->getBuildMaxNumjobs().c_str());
	}
		
	node.getChildNode("mbuild").addChild("optimization");
	if (!pc->getBuildOptimizationMarch().empty()) {
		node.getChildNode("mbuild").getChildNode("optimization").addChild("march");
		node.getChildNode("mbuild").getChildNode("optimization").getChildNode("march").addText(pc->getBuildOptimizationMarch().c_str());
	}
	if (!pc->getBuildOptimizationMtune().empty()) {
		node.getChildNode("mbuild").getChildNode("optimization").addChild("mtune");
		node.getChildNode("mbuild").getChildNode("optimization").getChildNode("mtune").addText(pc->getBuildOptimizationMtune().c_str());
	}
	if (!pc->getBuildOptimizationLevel().empty()) {
		node.getChildNode("mbuild").getChildNode("optimization").addChild("olevel");
		node.getChildNode("mbuild").getChildNode("optimization").getChildNode("olevel").addText(pc->getBuildOptimizationLevel().c_str());
	}
	if (!pc->getBuildOptimizationCustomGccOptions().empty()) {
		node.getChildNode("mbuild").getChildNode("optimization").addChild("custom_gcc_options");
		node.getChildNode("mbuild").getChildNode("optimization").getChildNode("custom_gcc_options").addText(pc->getBuildOptimizationCustomGccOptions().c_str());
	}

	node.getChildNode("mbuild").getChildNode("optimization").addChild("allow_change");
	if (!pc->getBuildOptimizationCustomizable()) {
		node.getChildNode("mbuild").getChildNode("optimization").getChildNode("allow_change").addText("true");
	}
	else {
		node.getChildNode("mbuild").getChildNode("optimization").getChildNode("allow_change").addText("false");
	}

	if (!pc->getBuildConfigureEnvOptions().empty())
	{
		node.getChildNode("mbuild").addChild("env_options");
		node.getChildNode("mbuild").getChildNode("env_options").addText(pc->getBuildConfigureEnvOptions().c_str());
	}
	if (!pc->getBuildKeyNames().empty())
	{
		node.getChildNode("mbuild").addChild("configuration");
	
		for (unsigned int i=0; i<pc->getBuildKeyNames().size(); i++)
		{
			node.getChildNode("mbuild").getChildNode("configuration").addChild("key");
			node.getChildNode("mbuild").getChildNode("configuration").getChildNode("key",i).addChild("name");
			node.getChildNode("mbuild").getChildNode("configuration").getChildNode("key",i).getChildNode("name").addText(pc->getBuildKeyNames().at(i).c_str());
			node.getChildNode("mbuild").getChildNode("configuration").getChildNode("key",i).addChild("value");
			node.getChildNode("mbuild").getChildNode("configuration").getChildNode("key",i).getChildNode("value").addText(pc->getBuildKeyValues().at(i).c_str());
		}
	}

	if (!pc->getBuildCmdConfigure().empty() || !pc->getBuildCmdMake().empty() || !pc->getBuildCmdMakeInstall().empty())
	{
		node.getChildNode("mbuild").addChild("custom_commands");
		if (!pc->getBuildCmdConfigure().empty())
		{
			node.getChildNode("mbuild").getChildNode("custom_commands").addChild("configure");
			node.getChildNode("mbuild").getChildNode("custom_commands").getChildNode("configure").addText(pc->getBuildCmdConfigure().c_str());
		}
		if (!pc->getBuildCmdMake().empty())
		{
			node.getChildNode("mbuild").getChildNode("custom_commands").addChild("make");
			node.getChildNode("mbuild").getChildNode("custom_commands").getChildNode("make").addText(pc->getBuildCmdMake().c_str());
		}
		if (!pc->getBuildCmdMakeInstall().empty())
		{
			node.getChildNode("mbuild").getChildNode("custom_commands").addChild("make_install");
			node.getChildNode("mbuild").getChildNode("custom_commands").getChildNode("make_install").addText(pc->getBuildCmdMakeInstall().c_str());
		}
	}


	node.writeToFile(filename.c_str());
}
void filterObjdumpOutput(vector<string>& ldd) {
	for (size_t i=0; i<ldd.size(); ++i) {
		strReplace(&ldd[i], "NEEDED", "");
		ldd[i] = "/" + cutSpaces(ldd[i]);
	}

}

void filterLDDOutput(vector<string>& ldd) {
	size_t p, e;
	for (size_t i=0; i<ldd.size(); ++i) {
		p=ldd[i].find("/");
		if (p==std::string::npos) {
			ldd.erase(ldd.begin()+i);
			--i;
			continue;
		}
		ldd[i].erase(ldd[i].begin(), ldd[i].begin()+p);
		e=ldd[i].find_first_of(" \t");
		if (e==std::string::npos) continue;
		ldd[i].erase(ldd[i].begin()+e, ldd[i].end());
		//printf("[%s]\n", ldd[i].c_str());
	}

}


void generateDeps_new(mpkg &core, string tgz_filename) {
	string aaa = getAbsolutePath(tgz_filename); // aaa: absolute path of filename
	printf("Generating dependencies for %s (new algorithm)\n", tgz_filename.c_str());
	
	// Import exclusion list, if any
	vector<string> exclusionList;
	if (!_cmdOptions["exclusion_list"].empty()) {
		exclusionList = ReadFileStrings(_cmdOptions["exclusion_list"]);
		if (exclusionList.empty()) mWarning("Warning: specified exclusion list is empty");
	}
	// Checking if file exists
	string tmp_dir;
	LocalPackage *lp=NULL;
	PACKAGE *data=NULL;
	if (!FileExists(tgz_filename)) {
		mError(_("File or directory ") + tgz_filename + _(" doesn't exist"));
		return;
	}
	// Check if it is a directory
	if (isDirectory(tgz_filename)) {
		printf ("Package directory %s, importing metadata\n", aaa.c_str());
		data = new PACKAGE;
		bool canParse=false;
		if (FileExists(tgz_filename + "/install/data.xml")) {
			PackageConfig p(tgz_filename + "/install/data.xml");
			if (p.parseOk) {
				canParse = true;
				xml2package(p.getXMLDoc(), p.getXMLNode(), data);
			}
		}
		if (!canParse) {
			mError(_("Cannot work without data.xml, create it using mpkg-setmeta"));
			return;
		}
		// Now we should collect filelist and import it into data->get_files_ptr()
		// Retrieving regular files
		string fname=get_tmp_file();
		string tar_cmd="( cd " + aaa + " && find | grep -v '/install' | sed -e s/^.//g > " + fname + " 2>/dev/null )";
		system(tar_cmd);
		vector<string>file_names=ReadFileStrings(fname);
		unlink(fname.c_str());
		for (size_t i=0; i<file_names.size(); ++i) {
			if (!file_names[i].empty()) {
				data->get_files_ptr()->push_back(file_names[i].substr(1));
			}
		}
		tmp_dir = aaa;
	}
	else {
		printf ("Package file %s, importing metadata\n", aaa.c_str());
		// Importing package data
		lp = new LocalPackage(tgz_filename);
		if (!lp->isNative()) {
			mError(_("Cannot work with legacy packages, convert it first using mpkg-convert or mpkg-setmeta"));
			return;
		}
		lp->injectFile();

		data = &lp->data;
		lp->fill_filelist(data, true);

		// Creating temporary directory
		tmp_dir = get_tmp_dir();

		// Extracting package into it
		extractFiles(tgz_filename, "", tmp_dir);
	}

	printf("Import complete, tmpdir = %s\n", tmp_dir.c_str());

	
	vector<string> ldd;
	vector<string> global_ldd;
	vector<string> script_deps;
	bool found=false;
	string ext;
	vector<string> filecheck, lddcheck; // Vectors for script files and other files.

	//struct stat so_stat;	
	// Separating python, perl and ruby scripts from other files
	bool excludeThis=false;
	for (size_t i=0; i<data->get_files().size(); ++i) {
		excludeThis=false;
		for (size_t z=0; z<exclusionList.size(); ++z) {
			if (exclusionList[z].find("/")==0) exclusionList[z].erase(0, 1);
			if (exclusionList[z].find_last_of("*")!=exclusionList[z].size()-1 && exclusionList[z].find_last_of("/")!=exclusionList[z].size()-1) {
				if (exclusionList[z]==data->get_files().at(i)) {
					excludeThis=true;
					break;
				}
			}
			else {
				if (data->get_files().at(i).find(exclusionList[z].substr(0, exclusionList[z].size()-2))==0) {
					excludeThis=true;
					break;
				}
			}
		}
		if (excludeThis) continue;
		ext = getExtension(data->get_files().at(i));
		if (ext=="py" || ext=="pyc" || ext=="pl" || ext=="pm" || ext=="rb") {
			filecheck.push_back(data->get_files().at(i)); // scripts are filled in filecheck vector
			continue;
		}
		//stat(data->get_files().at(i).get_name().c_str(), &so_stat);
		//if (!S_ISREG(so_stat.st_mode)) continue;
		lddcheck.push_back(data->get_files().at(i)); // Other files what are regular are filled to lddcheck vector
	}

	string tmp_ldd_list = get_tmp_file(); // File for ldd scan input
	string tmp_file_list = get_tmp_file(); // File for filecheck scan int
	string tmp_parse_ldd_dir = get_tmp_dir(); // Directory for ldd output
	string tmp_parse_file_dir = get_tmp_dir(); // Directory for file check output

	// Writing queries for script
	WriteFileStrings(tmp_ldd_list, lddcheck); 
	WriteFileStrings(tmp_file_list, filecheck);
	
	// Running script. Default is objdump, unless ldd is forced
	if (_cmdOptions["gendeps_mode"]=="ldd") {
		system("/usr/libexec/mpkglddcheck.sh " + tmp_ldd_list + " " + tmp_parse_ldd_dir + " " + tmp_file_list + " " + tmp_parse_file_dir + " " + tmp_dir);
	}
	else {
		system("/usr/libexec/mpkg_objdump_check.sh " + tmp_ldd_list + " " + tmp_parse_ldd_dir + " " + tmp_file_list + " " + tmp_parse_file_dir + " " + tmp_dir);
	}
	unlink(tmp_ldd_list.c_str());
	unlink(tmp_file_list.c_str());
	
	// Checking for script files
	string file_str; // temp string for file utility report
	string this_script_dep; // temp string for dep type
	for (size_t i=0; i<filecheck.size(); ++i) {
		ext = getExtension(filecheck[i]);
		file_str=ReadFile(tmp_parse_file_dir+"/"+IntToStr(i));
		this_script_dep.clear();
		if (file_str.find("python")!=std::string::npos) this_script_dep="python";
		else if (file_str.find("ruby")!=std::string::npos) this_script_dep="ruby";
		else if (ext=="pm" && file_str.find("text")!=std::string::npos) this_script_dep="perl";
		if (!this_script_dep.empty()) {
			//printf("%s: %s script\n", filecheck[i].c_str(), this_script_dep.c_str());
			found=false;
			for (unsigned int s=0; !found && s<script_deps.size(); ++s) {
				if (script_deps[s]==this_script_dep) found=true;
			}
			if (!found) script_deps.push_back(this_script_dep);
		}
	}

	// Checking ldd output for queued files
	string tmplddfilename;
	for (size_t i=0; i<lddcheck.size(); ++i) {
		// Reading ldd output for i'th file
		ldd=ReadFileStrings(tmp_parse_ldd_dir+"/"+IntToStr(i));
		if (ldd.empty()) continue;
		if (_cmdOptions["gendeps_mode"]=="ldd") {
			filterLDDOutput(ldd); // Filtering output
		}
		else {
			filterObjdumpOutput(ldd);
		}
		
		for (size_t l=0; l<ldd.size(); ++l) {
			// In first, check if required library is inside package (internal dependency)
			
			found=false;
			tmplddfilename = getFilename(ldd[l]);
			for (size_t g=0; g<lddcheck.size(); ++g) {
				if (tmplddfilename == getFilename(lddcheck[g])) found=true;
			}
			if (found) {
				continue;
			}
			// Check if already in list
			found=false;
			for (unsigned int g=0; !found && g<global_ldd.size(); ++g) {
				if (global_ldd[g]==ldd[l]) {
					found=true;
					break;
				}
			}
			if (!found) {
				global_ldd.push_back(ldd[l]);
				printf("%s for %s\n", ldd[l].c_str(), lddcheck[i].c_str());
			}
		}
	}

	//printf("total size of global: %ld\n", global_ldd.size());
	//printf("Querying database..\n");
	SQLRecord sqlSearch, sqlFields, sqlPkgSearch;
	sqlSearch.setSearchMode(SEARCH_OR);
	sqlSearch.setEqMode(EQ_CUSTOMLIKE);
	size_t so=std::string::npos;
	string libfind;
	for (size_t i=0; i<global_ldd.size(); ++i) {
		so = global_ldd[i].find(".so");
		if (so!=std::string::npos) libfind = global_ldd[i].substr(1, so+2);
		else libfind = global_ldd[i].substr(1);
		//printf("Searching for library %s\n", libfind.c_str());
		sqlSearch.addField("file_name", "%/" + getFilename(libfind) + "%");
		//printf("global_ldd: %s\n", global_ldd[i].c_str());
	}
	//printf("creating query\n");
	sqlFields.addField("packages_package_id");
	sqlFields.addField("file_name");
	SQLTable results;
	//printf("querying files: %d\n", sqlSearch.size());
	if (global_ldd.size()>0) core.db->get_sql_vtable(results, sqlFields, "files", sqlSearch);
	//printf("results size=%d\n", results.size());

	int fPackages_package_id=results.getFieldIndex("packages_package_id");
	int fFile_name=results.getFieldIndex("file_name");
	PACKAGE_LIST pkgList;
	sqlPkgSearch.setSearchMode(SEARCH_IN);

	//printf("\n\n=============RESULTS DUMP:================\n\n");
	for (size_t i=0; i<results.size(); ++i) {
		sqlPkgSearch.addField("package_id", results.getValue(i,fPackages_package_id));
	}
	//printf("\n\nQuerying packages\n");
	//
	//printf("SLOW GET_PACKAGELIST CALL: %s %d\n", __func__, __LINE__);
	if (results.size()>0) {
		core.get_packagelist(sqlPkgSearch, &pkgList, true, false);
		core.db->get_full_taglist(&pkgList);
	}
	
	PACKAGE_LIST minimalPkgList;
	vector<PACKAGE *> candidates;
	PACKAGE *goodPkg;
	double maxSize;
	string lib1, lib2;
	bool isLibsResolvable;
	string tmp;
	vector<string> ldconf = ReadFileStrings("/etc/ld.so.conf");
	ldconf.push_back("/lib"); // Here and next, the ones ends with "lib" also matches "lib64", as filter only searches for beginning
	ldconf.push_back("/usr/lib");
	ldconf.push_back("/usr/local/lib");
	ldconf.push_back("/usr/i486-slackware-linux");
	ldconf.push_back("/usr/i686-slackware-linux");
	ldconf.push_back("/usr/x86_64-slackware-linux");
	ldconf.push_back("/lib/incoming");
	ldconf.push_back("/lib64/incoming");
	for (size_t i=0; i<results.size(); ++i) {
		if (results.getRecord(i)->empty()) {
		       	continue;
		}
		candidates.clear();
		goodPkg=NULL;
		isLibsResolvable=false;
	//	printf("Adding candidate %s as %s\n", results.getValue(i, fPackages_package_id).c_str(), pkgList.getPackageByIDPtr(atoi(results.getValue(i, fPackages_package_id).c_str()))->get_name().c_str());
		candidates.push_back(pkgList.getPackageByIDPtr(atoi(results.getValue(i, fPackages_package_id).c_str())));
		if (!isLibsResolvable && candidates[candidates.size()-1]->isTaggedBy("libs")) isLibsResolvable=true;
		if (results.getValue(i, fPackages_package_id).empty()) continue;
		so = results.getValue(i, fFile_name).find(".so");
		// Check for path. Valid library paths are: lib, usr/lib, usr/local/lib, usr/lib64, usr/local/lib64,  usr/i486-slackware-linux/lib, lib/incoming
		tmp = results.getValue(i, fFile_name);
		bool ldfound = false;
		for (size_t l=0; !ldfound && l<ldconf.size(); ++l) {
			if (ldconf[l].size()<2) continue;
			if (tmp.find(ldconf[l].substr(1))==0) ldfound = true;
		}
		if (!ldfound) continue;
		//if (tmp.find("lib")!=0 && tmp.find("lib64")!=0 && tmp.find("usr/lib")!=0 && tmp.find("usr/lib64")!=0 && tmp.find("usr/local/lib")!=0 && tmp.find("usr/local/lib64")!=0 && tmp.find("usr/i486-slackware-linux")!=0 && tmp.find("usr/x86_64-slackware-linux")!=0 && tmp.find("lib/incoming")!=0 && tmp.find("lib64/incoming")!=0) continue;
		if (so!=std::string::npos) lib1 = getFilename(results.getValue(i, fFile_name).substr(0, so+3));
		else continue;
		if (verbose) printf("lib1: %s, primary candidate: %s\n", lib1.c_str(), pkgList.getPackageByIDPtr(atoi(results.getValue(i, fPackages_package_id).c_str()))->get_name().c_str());
		for (size_t t=i+1; t<results.size(); ++t) {
			if (results.getRecord(t)->empty()) continue;
			lib2.clear();

			if (results.getValue(t, fPackages_package_id).empty()) continue;
			so = results.getValue(t, fFile_name).find(".so");
			if (so!=std::string::npos) lib2 = getFilename(results.getValue(t, fFile_name).substr(0, so+3));
			else continue;


			if (!lib2.empty() && lib1==lib2) {
				if (verbose) {
					printf("lib1==lib2==%s: Adding candidate %s as %s\n", lib1.c_str(), results.getValue(t, fPackages_package_id).c_str(), pkgList.getPackageByIDPtr(atoi(results.getValue(t, fPackages_package_id).c_str()))->get_name().c_str());
					printf("t size = %ld\n", results.getValue(t, fFile_name).size());
					printf("%s<==>%s\n", results.getValue(i, fFile_name).c_str(), results.getValue(t, fFile_name).c_str());
				}
				candidates.push_back(pkgList.getPackageByIDPtr(atoi(results.getValue(t, fPackages_package_id).c_str())));
				if (!isLibsResolvable && candidates[candidates.size()-1]->isTaggedBy("libs")) isLibsResolvable=true;
				results.getRecord(t)->clear();
			}
		}
//		printf("candidates for %s: %ld\n", results.getValue(i, fFile_name).c_str(), candidates.size());
		// Now filter results by ID. First, aaa_elflibs; second - any package from base (as they all installed anyway), and third - the package who has minimum size
		maxSize=0;
		for (size_t t=0; t<candidates.size(); ++t) {
			if (afraidAaaInDeps && candidates[t]->get_name()=="aaa_elflibs") continue;
			// Skip compat32 packages
			if (candidates[t]->isTaggedBy("compat32")) printf("%s is compat32, skipping\n", candidates[t]->get_name().c_str());
			if (data->get_name()!="skype" && data->get_name()!="wine" && candidates[t]->isTaggedBy("compat32") && !data->isTaggedBy("x86")) continue;
			if (verbose) printf("Checking candidate %s for %s\n", candidates[t]->get_name().c_str(), results.getValue(i, fFile_name).c_str());
			if (candidates[t]->get_name()=="aaa_elflibs") {
				if (verbose) printf("good as aaa: %s\n", candidates[t]->get_name().c_str());
				goodPkg=candidates[t];
				break;
			}
			if (candidates[t]->isTaggedBy("base")) {	
				if (verbose) printf("good as base: %s\n", candidates[t]->get_name().c_str());
				goodPkg=candidates[t];
				break;
			}
			if (data->get_arch()=="x86_64") {
			       if (candidates[t]->get_arch() != "noarch" && candidates[t]->get_arch()!="x86_64") continue; // How can we mix architectures, huh?
			}
			if (isLibsResolvable && !candidates[t]->isTaggedBy("libs")) continue; // Skip non-library candidates if possible
			//if (candidates[t]->get_corename()=="nvidia-driver") continue; // Skip nvidia driver, please!
			if (maxSize==0 || strtod(candidates[t]->get_installed_size().c_str(), NULL)<maxSize) {
				if (verbose) printf("possible good as maxsize %d<%d: %s\n", (int) strtod(candidates[t]->get_installed_size().c_str(), NULL), (int) maxSize, candidates[t]->get_name().c_str());
				goodPkg=candidates[t];
				maxSize=strtod(candidates[t]->get_installed_size().c_str(), NULL);
			}
		}
		if (goodPkg) minimalPkgList.add(*goodPkg);
		else {
		       	cout << _("FATAL: No good package found for item [") << i << "] " << tmp << _(", it means that you have some libs that are not packaged.\n") << endl;
			cout << _("Note: if you are building 32-bit binary for 64-bit system, you should add x86 tag to prevent this.") << endl;
			abort();
		}
		if (verbose) printf("%s found in %s package\n", results.getValue(i, fFile_name).c_str(), goodPkg->get_name().c_str());
		filterDupes(&minimalPkgList);
	}
	vector<DEPENDENCY> deps;
	if (_cmdOptions["preserve_deps"]=="yes") deps = data->get_dependencies();
	DEPENDENCY dep;
	for (size_t i=0; i<minimalPkgList.size(); ++i) {
		if (!minimalPkgList[i].installed()) continue;
		if (minimalPkgList[i].get_corename()==data->get_corename()) continue;
		if (minimalPkgList[i].get_name()=="glibc") dep.set_package_name("glibc-solibs");
		else if (minimalPkgList[i].get_name()=="openssl") dep.set_package_name("openssl-solibs");
		else if (minimalPkgList[i].get_corename()=="nvidia-driver") continue;
		else dep.set_package_name(minimalPkgList[i].get_corename());
		dep.set_condition(IntToStr(VER_XMORE));
		dep.set_package_version(minimalPkgList[i].get_version());
		if (_cmdOptions["preserve_deps"]=="yes") {
			bool alreadyDepped;
		        alreadyDepped = false;
			for (size_t z=0; !alreadyDepped && z<deps.size(); ++z) {
				if (deps[z].get_package_name()==dep.get_package_name()) {
					alreadyDepped = true;
				}
			}
			if (!alreadyDepped) deps.push_back(dep);
		}
		else deps.push_back(dep);
	}
	PACKAGE_LIST scriptPkgList;
	SQLRecord scriptPkgSearch;
	scriptPkgSearch.setSearchMode(SEARCH_IN);
	scriptPkgSearch.setEqMode(EQ_EQUAL);
	for (size_t i=0; i<script_deps.size(); ++i) {
		scriptPkgSearch.addField("package_name", script_deps[i]);
	}
	//printf("script_deps.size: %ld\n", script_deps.size());
	//printf("SLOW GET_PACKAGELIST CALL: %s %d\n", __func__, __LINE__);
	if (script_deps.size()>0) {
		core.get_packagelist(scriptPkgSearch, &scriptPkgList, true, false);
		core.db->get_full_taglist(&scriptPkgList);
	}
	for (size_t i=0; i<scriptPkgList.size(); ++i) {
		if (scriptPkgList[i].get_name()==data->get_name()) continue;
		if (!scriptPkgList[i].installed()) continue;
		found=false;
		for (size_t d=0; !found && d<deps.size(); ++d) {
			if (deps[d].get_package_name()==scriptPkgList[i].get_corename()) found=true;
		}
		if (found) continue;
		dep.set_package_name(scriptPkgList[i].get_corename());
		dep.set_condition(IntToStr(VER_XMORE));
		dep.set_package_version(scriptPkgList[i].get_version());
		deps.push_back(dep);
	}
	data->set_dependencies(deps);
	printf("\nDEPENDENCIES FOUND:\n");
	for (size_t i=0; i<deps.size(); ++i) {
		printf("[%d] %s\n", (int) i+1, deps[i].getDepInfo().c_str());
	}
	//printf("Saving data\n");
	PackageConfig pk(tmp_dir+"/install/data.xml");
	cout << "Writing " << data->get_dependencies().size() << " deps\n\n";
	dumpPackage(data, &pk, tmp_dir+"/install/data.xml");
	//printf("tmpdir: %s, aaa: %s\n", tmp_dir.c_str(), getDirectory(aaa).c_str());
	if (!isDirectory(tgz_filename)) {
		printf("Compressing package... please wait\n");
		string keeps;
		if (_cmdOptions["keep_symlinks"]=="true") keeps = " -s ";
		system ("cd " + tmp_dir + "; buildpkg " + keeps + getDirectory(aaa) + " >/dev/null 2>/dev/null");
		system("rm -rf " + tmp_dir);
	}
	if (lp) delete lp;
	else if (data) delete data;
	delete_tmp_files();

}

int mpkg::syncronize_repositories(string sync_map) {
	// So, we have:
	// 1. List of remote repositories, which should be mapped to defined destination directories
	// 2. These directories should be indexed after sync, and become a mirrors of requested repositories
	// 3. If set by syncmap, no extra packages should be in destination after sync: they should be backed up to deprecated branch
	// 4. Only changed/new packages should be downloaded
	// 5. Directory structure should be the same as in source storage
	// 6. Log activity in the way in which it can be readed for site news.
	// 7. Check package integrity by comparing md5 after sync

	// Step 1: read sync map
	bool deprecateOld = false;
	bool reindexBeforeSync = false;
	bool deleteDeprecated = false;
	string deprecatedDir;
	vector<string> mapData = ReadFileStrings(sync_map); // Syncmap config
	vector<string> serverUrl; // Data from config: URLs
	vector<string> localPath; // Data from config: PATHs
	vector<string> tmpQueryDestination, queryDestination; // Per-repository destinations
	vector<PACKAGE *> tmpQuery, query; // Per-repository sources
	DownloadsList *downloadQueue; // Per-repository download queue
	DownloadItem tmpDownloadItem;
	vector<string> itemLocations; // Per-item locations
	bool forceRebuild = false;
	bool skipDeprecated=false;
	bool noRebuildAfterSync = false;
	if (_cmdOptions["skip-deprecated"]=="true") skipDeprecated=true;
	string trash;
	PACKAGE_LIST *remoteList = NULL, *localList = NULL; // Per-repository package lists
	vector<string> dropPackages;
	bool noValidate = false;
	// 1. Parsing config
	for (unsigned int i=0; i<mapData.size(); i++) {
		if (mapData[i].find("DROP=")==0 && mapData[i].size()>5) {
			printf("%s\n", mapData[i].substr(5).c_str());
			dropPackages.push_back(mapData[i].substr(5));
			continue;
		}
		if (mapData[i].find("DEPRECATE_OLD")==0 && mapData[i].find("false")==std::string::npos) {
			printf("Old packages will be deprecated\n");
			deprecateOld = true;
			continue;
		}

		if (mapData[i].find("DELETE_DEPRECATED")==0 && mapData[i].find("false")==std::string::npos) {
			deleteDeprecated = true;
			continue;
		}

		if (mapData[i].find("REBUILD_INDEX_BEFORE_SYNC")==0 && mapData[i].find("false")==std::string::npos) {
			reindexBeforeSync = true;
			continue;
		}
		if (mapData[i].find("DEPRECATED_DIR=")==0) {
			deprecatedDir = mapData[i].substr(strlen("DEPRECATED_DIR="));
			continue;
		}
		if (mapData[i].find("NO_REBUILD_AFTER_SYNC")==0 && mapData[i].find("false")==std::string::npos) {
			noRebuildAfterSync=true;
			continue;
		}
		if (mapData[i].find("NO_VALIDATE")==0 && mapData[i].find("false")==std::string::npos) {
			noValidate = true;
			continue;
		}
		
		if (mapData[i].find("#")==0) continue; // allow comments

		if (mapData[i].find("://")!=std::string::npos) {
			// I think it is URL.
			size_t s = mapData[i].find_first_of(" \t");
			if (s==std::string::npos) {
				mError(_("Error in ") + sync_map + (" on line ") + IntToStr(i));
				return 1;
			}
			serverUrl.push_back(mapData[i].substr(0,s));
			localPath.push_back(mapData[i].substr(s+1));
		}
	}

	// Checking if deprecating options have been specified incorrectly
	if (deprecateOld && !deleteDeprecated && deprecatedDir.empty()) {
		mError(_("You didn't specified directory for deprecated packages"));
		return 1;
	}
	
	// Rebuilding index if nessecary
	if (reindexBeforeSync) {
		bool treeFound;
		for (unsigned int i=0; i<localPath.size(); ++i) {
			treeFound = false;
			for (size_t z=i+1; !treeFound && z<localPath.size(); ++z) {
				if (localPath[i]==localPath[z]) treeFound = true;
			}
			if (!treeFound) rep.build_index(localPath[i]);
		}
	}
	// Reading data from all repositories and syncronize by pair

	bool found = false;
	for (size_t i=0; i<serverUrl.size(); ++i) {
		// Creating objects
		remoteList = new PACKAGE_LIST;
		localList = new PACKAGE_LIST;
		tmpQueryDestination.clear();
		queryDestination.clear();
		tmpQuery.clear();
		query.clear();
		
		// Get indexes
		rep.get_index(serverUrl[i], remoteList);
		if (remoteList->IsEmpty()) {
			// Error in connection, or empty repository - anyway, better to skip
			printf(_("Error connecting to %s, skipping\n"), serverUrl[i].c_str());
			delete remoteList;
			delete localList;
			continue;
		}
		rep.get_index("file://" + localPath[i], localList);
		
		// Searching for differences by md5
		for (size_t r=0; r<remoteList->size(); ++r) {
			found = false;
			for (size_t l=0; l<dropPackages.size(); ++l) {
				if (remoteList->at(r).get_name()==dropPackages[l]) {
					fprintf(stderr, _("Skipping %s-%s: package dropped in config\n"), remoteList->at(r).get_name().c_str(), remoteList->at(r).get_fullversion().c_str());
					found=true;
					continue;
				}
			}
			if (found) continue;
			found = false;
			for (size_t l=0; l<localList->size(); l++) {
				if (remoteList->at(r).get_md5() == localList->at(l).get_md5()) {
					found = true;
					break;
				}
			}
			if (found) continue;
			found=false;
			if (skipDeprecated) {
				for (size_t l=0; l<localList->size(); ++l) {
					if (localList->at(l).get_name()!=remoteList->at(r).get_name()) continue;
					if (compareVersions(localList->at(l).get_version(), 
								localList->at(l).get_build(), \
								remoteList->at(r).get_version(), \
								remoteList->at(r).get_build())>=0) 
					{
						fprintf(stderr, _("Skipping %s %s as deprecated: %s already in mirror\n"), \
								remoteList->at(r).get_name().c_str(), \
								remoteList->at(r).get_fullversion().c_str(), \
								localList->at(l).get_fullversion().c_str());
						found=true;
						break;
					}
				}
				for (size_t l=0; l<remoteList->size(); ++l) {
					if (r==l) continue;
					if (remoteList->at(r).get_name()!=remoteList->at(l).get_name()) continue;
					if (compareVersions(remoteList->at(l).get_version(), \
								remoteList->at(l).get_build(), \
								remoteList->at(r).get_version(), \
								remoteList->at(r).get_build())>=0) 
					{
						fprintf(stderr, _("Skipping %s %s as deprecated: %s already in mirror\n"), \
								remoteList->at(r).get_name().c_str(), \
								remoteList->at(r).get_fullversion().c_str(), \
								remoteList->at(l).get_fullversion().c_str());
						found=true;
						break;
					}
				}
				if (found) continue;
				printf("%s %s: new package\n", remoteList->at(r).get_name().c_str(), remoteList->at(r).get_fullversion().c_str());
			}
			// If not found, we have to download it. Add to queue
			tmpQuery.push_back(remoteList->get_package_ptr(r));
			tmpQueryDestination.push_back(localPath[i]+ remoteList->at(r).get_locations().at(0).get_path() + remoteList->at(r).get_filename());
		}

		if (tmpQuery.empty()) {
			printf(_("Repository are up to date\n"));
		}
		else {
			if (verbose && !dialogMode) {
				for (unsigned int i=0; i<tmpQuery.size(); ++i) {
					printf("[%d] %s-%s\n", i+1, tmpQuery[i]->get_name().c_str(), tmpQuery[i]->get_fullversion().c_str());
				}
			}
			if (!interactive_mode) {
				cout << tmpQuery.size() << _(" packages enqueued to sync. Processing") << endl;
			}
			else {
				cout << tmpQuery.size() << _(" packages enqueued to sync. Proceed? [Y/n]") << endl;
				string input;
				int cnt = 0;
				while (input!="y" && input!="Y" && input!="yes" && input!="\n" && cnt < 10) {
					input.clear();
					input=cin.get();
					if (input=="n" || input=="N" || input == "no") return MPKGERROR_ABORTED;
					if (input!="y" && input!="Y" && input!="yes" && input!="\n") {
						say(_("Please answer Y (yes) or N (no)\n"));
						cin.get();
					}
					cnt++;
				}
				if (cnt>=10) {
					mError(_("Failed to receive answer from user"));
					return MPKGERROR_ABORTED;
				}

			}
		}
		// Creating output directories
		for (unsigned int d=0; d<tmpQueryDestination.size(); ++d) {
			system("mkdir -p " + getDirectory(tmpQueryDestination[d]));
		}
		
		// Trying to use deltas
		for (unsigned int l=0; l<tmpQuery.size(); ++l) {
			//printf("Trying to get delta for %s, directory: %s\n", tmpQuery[l]->get_filename().c_str(), getDirectory(tmpQueryDestination[l]).c_str());
			if (tryGetDelta(tmpQuery[l], getDirectory(tmpQueryDestination[l])+"/")) {
				continue;
			}
			else {
				query.push_back(tmpQuery[l]);
				queryDestination.push_back(tmpQueryDestination[l]);
			}
			
		}
		
		// Checking for deprecated
		for (unsigned int l=0; deprecateOld && l<localList->size(); ++l) {
			found = false;
			for (unsigned int r=0; r<remoteList->size(); r++) {
				if (remoteList->at(r).get_md5() == localList->at(l).get_md5()) {
					found = true;
					break;
				}
			}
			if (found) continue;
			// If not found, we should move it to deprecated dir.

			trash = localPath[i] + localList->at(l).get_locations().at(0).get_path() + localList->at(l).get_filename();
			if (deleteDeprecated) {
				say(_("Deleting deprecated package %s-%s\n"), localList->at(l).get_name().c_str(), localList->at(l).get_fullversion().c_str());
				unlink(trash.c_str());
			}
			else {
				say(_("Moving deprecated package %s-%s to %s\n"), localList->at(l).get_name().c_str(), localList->at(l).get_fullversion().c_str(), deprecatedDir.c_str());
				system("mkdir -p " + deprecatedDir);
				system("mv " + trash + " " + deprecatedDir + "/");
			}
			forceRebuild = true;
		}

		
		// Profit!
		downloadQueue = new DownloadsList;


		for (size_t q=0; q<query.size(); ++q) {
			itemLocations.clear();
			
			tmpDownloadItem.expectedSize=strtod(query[q]->get_compressed_size().c_str(), NULL);
			tmpDownloadItem.file = queryDestination[q];
			system("mkdir -p " + getDirectory(queryDestination[q]));
			tmpDownloadItem.name = query[q]->get_name();
			tmpDownloadItem.priority = 0;
			tmpDownloadItem.status = DL_STATUS_WAIT;
			tmpDownloadItem.itemID = q;
			tmpDownloadItem.usedSource = &query[q]->usedSource;
	
			query[q]->sortLocations();
			for (unsigned int k = 0; k < query[q]->get_locations().size(); k++) {
				itemLocations.push_back(query[q]->get_locations().at(k).get_server_url() \
					     + query[q]->get_locations().at(k).get_path() \
					     + query[q]->get_filename());
			}
			tmpDownloadItem.url_list = itemLocations;
			downloadQueue->push_back(tmpDownloadItem);
		}


		// Downloading files
		if (CommonGetFileEx(*downloadQueue, &currentItem)==DOWNLOAD_ERROR) {
			printf("Error while sync: cannot download some files\n");
			delete remoteList;
			delete localList;
			delete downloadQueue;
			return 1;
		}
		// Validating input
		string val;
		for (unsigned int q=0; !noValidate && q<tmpQuery.size(); ++q) {
			system("mpkg-validate " + tmpQueryDestination[q]);
			val = ReadFile(tmpQueryDestination[q]+".validate");
			if (val.find("BLOCK")!=std::string::npos || val.find("FATAL")!=std::string::npos) {
				mError(tmpQueryDestination[q] + ": BLOCKING OR FATAL ERRORS IN PACKAGE!");
			}
		}
		if (!tmpQuery.empty() || forceRebuild) {
			if (!noRebuildAfterSync) {
				printf("Building new local index\n");
				rep.build_index(localPath[i]);
			}
		}

		// Deleting package lists
		delete remoteList;
		delete localList;
		delete downloadQueue;

	}
	if (deprecateOld && !deleteDeprecated && !deprecatedDir.empty() && forceRebuild) {
		say(_("Building index for deprecated directory\n"));
		rep.build_index(deprecatedDir);
	}

	
	return 0;
}

void mpkg::get_queue(PACKAGE_LIST *pkgList, int filter) {
	pkgList->clear();
	SQLRecord sqlSearch;

	sqlSearch.setSearchMode(SEARCH_IN);
	if (filter == QUEUEFILTER_NONE || filter == QUEUEFILTER_INSTALL) sqlSearch.addField("package_action", ST_INSTALL);
	if (filter == QUEUEFILTER_NONE || filter == QUEUEFILTER_REMOVE || filter == QUEUEFILTER_REMOVEPURGE) sqlSearch.addField("package_action", ST_REMOVE);
	if (filter == QUEUEFILTER_NONE || filter == QUEUEFILTER_PURGE || filter == QUEUEFILTER_REMOVEPURGE) sqlSearch.addField("package_action", ST_PURGE);
	if (filter == QUEUEFILTER_NONE || filter == QUEUEFILTER_REPAIR) sqlSearch.addField("package_action", ST_REPAIR);
	if (filter == QUEUEFILTER_NONE || filter == QUEUEFILTER_UPDATE) sqlSearch.addField("package_action", ST_UPDATE);

	get_packagelist(sqlSearch, pkgList, true);
}

vector<string> mpkg::getLatestUpdates(PACKAGE_LIST *pkgList, PACKAGE_LIST *uninstList, bool fast, bool needDescriptions) {
	// Yet another new algorithm
	if (dialogMode) ncInterface.showInfoBox(_("Searching for updates..."));
	else fprintf(stderr, _("Searching for updates..."));
	vector<string> errorList;
	//vector<MenuItem> menuItems; // Vector for update list
	// Creating DB cache
	// 1. Creating a request for all packages which are in package_name vector.
	SQLRecord sqlSearch;
	// 2. Requesting database by search array
	PACKAGE_LIST pCache;
	//printf("SLOW=%d GET_PACKAGELIST CALL: %s %d\n", fast, __func__, __LINE__);
	sqlSearch.orderBy="package_id";
	int query_ret = get_packagelist(sqlSearch, &pCache, fast, needDescriptions);

	vector<string> blackList = ReadFileStrings("/etc/mpkg-update-blacklist");
	vector<string> env_blacklist_v, opt_blacklist_v;
       	if (getenv("SKIP")) {
		env_blacklist_v = splitString(getenv("SKIP"), " ");
	}
	if (!_cmdOptions["skip_packages"].empty()) {
		opt_blacklist_v = splitString(_cmdOptions["skip_packages"], ",");
		for (size_t i=0; i<opt_blacklist_v.size(); ++i) {
			env_blacklist_v.push_back(opt_blacklist_v[i]);
		}
	}

	for (size_t i=0; i<env_blacklist_v.size(); ++i) {
		if (cutSpaces(env_blacklist_v[i]).empty()) continue;
		printf("SKIP: %s\n", cutSpaces(env_blacklist_v[i]).c_str());
		blackList.push_back(cutSpaces(env_blacklist_v[i]));
	}

	if (query_ret != 0) {
		if (dialogMode) ncInterface.showMsgBox(_("Error querying database"));
		else errorList.push_back(mError(string(_("Error querying database"))));
		return errorList;// MPKGERROR_SQLQUERYERROR;
	}
	map<string, vector<size_t> > nameMap;
	vector<PACKAGE *> updateCandidates, removeCandidates;
	bool blacklisted;
	bool addedToRemove;

	for (size_t i=0; i<pCache.size(); ++i) {
		if (!pCache[i].installed()) continue;
		addedToRemove = false;
		blacklisted = false;
		for (size_t t=0; !blacklisted && t<blackList.size(); ++t) {
			if (pCache[i].get_corename()==blackList[t] || pCache[i].get_name()==blackList[t]) {
				if (!dialogMode && verbose) printf(_("Skipping %s: BLACKLISTED\n"), pCache[i].get_name().c_str());
				blacklisted = true;
			}
		}
		if (blacklisted) continue;
		for (size_t t=0; t<pCache.size(); ++t) {
			if (strcmp(pCache[i].get_name().c_str(), pCache[t].get_name().c_str())!=0) continue;
			//if (!pCache[t].available()) continue;
			if (compareVersions(pCache[i].get_version(), pCache[i].get_build(), pCache[t].get_version(), pCache[t].get_build())<0) {
				updateCandidates.push_back(pCache.get_package_ptr(t));
				if (!addedToRemove) {
					removeCandidates.push_back(pCache.get_package_ptr(i));
					addedToRemove = true;
				}
				nameMap[pCache[i].get_name()].push_back(t);
			}
		}
	}

	// Now let's find max versions for candidates to update
	vector<size_t> updVector;
	size_t max_id;
	string maxv, maxb;
	for (size_t i=0; i<removeCandidates.size(); ++i) {
		maxv.clear();
		maxb.clear();
		updVector = nameMap[removeCandidates[i]->get_name()];
		if (updVector.empty()) continue;
		maxb = pCache[updVector[0]].get_build();
		maxv = pCache[updVector[0]].get_version();
		max_id = updVector[0];
		for (size_t t=1; t<updVector.size(); ++t) {
			if (compareVersions(maxv, maxb, pCache[updVector[t]].get_version(), pCache[updVector[t]].get_build())<0) {
				max_id = updVector[t];
				maxv = pCache[updVector[t]].get_version();
				maxb = pCache[updVector[t]].get_build();
			}
		}
		pkgList->add(pCache[max_id]);
		uninstList->add(*removeCandidates[i]);
	}

	return errorList;
}

vector<string> mpkg::getExportInstalled(bool include_versions) {
	vector<string> ret;
	PACKAGE_LIST pkgList;
	SQLRecord sqlSearch;
	sqlSearch.addField("package_installed", ST_INSTALLED);
	get_packagelist(sqlSearch, &pkgList, true);
	ret.push_back("# Installed package list generated by mpkg. Total packages: ");
	ret.push_back("# You can install packages by this list by using command: mpkg-installfromlist FILE_NAME");
	ret.push_back("# ");
	// Let's group packages using short tags. It will be compatible with parser, and much more readable
	vector<string> shorttags;
	bool tagfound;
	size_t tagcount;
	for (size_t i=0; i<pkgList.size(); ++i) {
		tagcount=0;
		for (size_t z=0; z<pkgList[i].get_tags().size(); ++z) {
			if (pkgList[i].get_tags().at(z).find("-")!=std::string::npos) continue;
			tagcount++;
			tagfound = false;
			for (size_t t=0; !tagfound && t<shorttags.size(); ++t) {
				if (shorttags[t]==pkgList[i].get_tags().at(z)) tagfound = true;
			}
			if (!tagfound) {
				shorttags.push_back(pkgList[i].get_tags().at(z));
				break;
			}
		}
		if (tagcount==0) {
			if (include_versions) ret.push_back(pkgList[i].get_name() + " " + pkgList[i].get_fullversion());
			else ret.push_back(pkgList[i].get_name());
		}
	}
	vector<size_t> dumped_packages;
	sort(shorttags.begin(), shorttags.end());
	bool dumped;
	for (size_t z=0; z<shorttags.size(); ++z) {
		ret.push_back("");
		ret.push_back("# " + shorttags[z]);
		for (size_t i=0; i<pkgList.size(); ++i) {
			dumped = false;
			if (!pkgList[i].isTaggedBy(shorttags[z])) continue;
			for (size_t t=0; !dumped && t<dumped_packages.size(); ++t) {
				if (dumped_packages[t]==i) dumped = true;
			}
			if (!dumped) {
				dumped_packages.push_back(i);
				if (include_versions) ret.push_back(pkgList[i].get_name() + " " + pkgList[i].get_fullversion());
				else ret.push_back(pkgList[i].get_name());
			}
		}
	}

	/*for (unsigned int i=0; i<pkgList.size(); ++i) {
		if (include_versions) ret.push_back(pkgList[i].get_name() + " " + pkgList[i].get_fullversion());
		else ret.push_back(pkgList[i].get_name());
	}*/
	ret[0] += IntToStr(pkgList.size());
	return ret;
}

vector<string> preprocessInstallList(const string &filename) {
	vector<string> filenames;
	filenames.push_back(filename);
	vector<string> ret;
       	ret = ReadFileStrings(filename);
	string include_name;
	bool processed;
	for (size_t i=0; i<ret.size(); ++i) {
		if (ret[i].find("@include ")==0 && ret[i].size()>strlen("@include ")) {
			include_name = getDirectory(filename) + "/" + ret[i].substr(strlen("@include "));
			// Check if already processed
			processed = false;
			for (size_t t=0; !processed && t<filenames.size(); ++t) {
				if (filenames[t]==include_name) processed = true;
			}
			if (processed) continue;
			filenames.push_back(include_name);
			// Missing includes should be fatal
			if (!FileExists(include_name)) {
				mError(_("File ") + include_name + _(", included in setup list, not found"));
				vector<string> a;
				return a;
			}
			if (!dialogMode && verbose) {
				printf("Merging with %s\n", include_name.c_str());
			}
			ret = mergeVectors(ret, ReadFileStrings(include_name));
		}
	}
	
	return ret;
}

void parseInstallList(const vector<string> &data, vector<string> &installQuery, vector<string> &versionQuery) {
	installQuery = data;
	versionQuery.clear();
	versionQuery.resize(installQuery.size());
	vector<string> rm_diff;

	// Filter versions and comments if exists
	for (size_t i=0; i<installQuery.size(); ++i) {
		while (installQuery[i].find_first_of("#")!=std::string::npos) {
			installQuery[i] = installQuery[i].substr(0, installQuery[i].find_first_of("#"));
		}
		if (installQuery[i].find_first_of("#")!=std::string::npos) {
			installQuery.erase(installQuery.begin()+i);
			versionQuery.erase(versionQuery.begin()+i);
			i--;
			continue;
		}
		if (installQuery[i].find_first_of(" ")!=std::string::npos) {
		versionQuery[i] = installQuery[i].substr(installQuery[i].find_first_of(" "));
			installQuery[i] = installQuery[i].substr(0, installQuery[i].find_first_of(" "));
		}
		versionQuery[i]=cutSpaces(versionQuery[i]);
		installQuery[i]=cutSpaces(installQuery[i]);
		if (installQuery[i].find_first_not_of(" ")==std::string::npos) {
			installQuery.erase(installQuery.begin()+i);
			versionQuery.erase(versionQuery.begin()+i);
			i--;
			continue;
		}
		if (installQuery[i][0]=='-') {
			rm_diff.push_back(installQuery[i]);
			installQuery.erase(installQuery.begin()+i);
			versionQuery.erase(versionQuery.begin()+i);
			i--;
			continue;
		}
	}
	// Diff processing
	for (size_t i=0; i<rm_diff.size(); ++i) {
		for (size_t t=0; t<installQuery.size(); ++t) {
			if (installQuery[t]==rm_diff[i]) {
				installQuery.erase(installQuery.begin()+t);
				versionQuery.erase(versionQuery.begin()+t);
				t--;
				continue;
			}
		}
	}
}

bool checkUtility(const string& util) {
	string tmp = get_tmp_file();
	system("which " + util + " > " + tmp + " 2>/dev/null");
	string res = ReadFile(tmp);
	unlink(tmp.c_str());
	if (res.empty()) return false;
	return true;
}

// This function checks if all required utilities present in system:
// tar, gzip, xz, sed
bool checkUtilities() {
	return checkUtility("tar") && checkUtility("gzip") && checkUtility("xz") && checkUtility("sed");
}


CustomPkgSet getUserCustomPkgSet(const string &path, const string& base_name, bool merge, const string& locale) {
	CustomPkgSet ret;
	
	PACKAGE_LIST pkgList;
	SQLRecord record;
	mpkg *core = new mpkg;
	core->get_packagelist(record, &pkgList, true, false);
	delete core;

	if (merge) ret = getCustomPkgSet(base_name, locale, pkgList);
	else {
		ret.name = getFilename(path);
		ret.desc = _("Custom list");
		ret.full = _("Custom package list from ") + path;
		ret.hasX11 = "N/A";
		ret.hasDM = "N/A";
		ret.hw = "N/A";
	}
	
	string finalPath;
	if (path.find("http://")==0 || path.find("ftp://")==0) {
		string t = get_tmp_file();
		CommonGetFile(path, t);
		finalPath = t;
	}
	else finalPath = path;

	calculatePkgSetSize(ret, finalPath, pkgList, merge);
	return ret;
}

CustomPkgSet getCustomPkgSet(const string& name, const string& locale, const PACKAGE_LIST &pkgList) {
	vector<string> data = ReadFileStrings("/tmp/setup_variants/" + name + ".desc");
	CustomPkgSet ret;
	ret.hasX11 = false;
	ret.hasDM = false;
	ret.name = name;
	printf("Processing %s\n", name.c_str());
	string c_locale = locale;
	if (c_locale.size()>2) c_locale = "[" + c_locale.substr(0,2) + "]";
	else c_locale = "";
	string gendesc, genfull;
	for (size_t i=0; i<data.size(); ++i) {
		if (data[i].find("desc" + c_locale + ": ")==0) ret.desc = getParseValue("desc" + c_locale + ": ", data[i], true);
		else if (data[i].find("desc: ")==0) gendesc = getParseValue("desc: ", data[i], true);
		else if (data[i].find("full" + c_locale + ": ")==0) ret.full = getParseValue("full" + c_locale + ": ", data[i], true);
		else if (data[i].find("full: ")==0) genfull = getParseValue("full: ", data[i], true);
		else if (data[i].find("hasX11")==0) ret.hasX11 = true;
		else if (data[i].find("hasDM")==0) ret.hasDM = true;
		else if (data[i].find("hardware" + c_locale + ": ")==0) ret.hw = getParseValue("hardware" + c_locale + ": ", data[i], true);
		else if (data[i].find("hardware: ")==0) if (ret.hw.empty()) ret.hw = getParseValue("hardware: ", data[i], true);
	}
	if (ret.desc.empty()) ret.desc = gendesc;
	if (ret.full.empty()) ret.full = genfull;
	calculatePkgSetSize(ret, "/tmp/setup_variants/" + ret.name + ".list", pkgList, false);
	return ret;
}

void calculatePkgSetSize(CustomPkgSet &set, const string& file_path, const PACKAGE_LIST &pkgList, bool merge) {
	vector<string> list = preprocessInstallList(file_path);
	int64_t csize = 0, isize = 0;
	size_t count = 0;
	vector<string> was;
	bool pkgWas;
	for (size_t i=0; i<list.size(); ++i) {
		if (list[i].find("#")!=std::string::npos) continue;
		if (cutSpaces(list[i]).empty()) continue;
		pkgWas = false;
		for (size_t w=0; !pkgWas && w<was.size(); ++w) {
			if (was[w]==cutSpaces(list[i])) pkgWas = true;
		}
		if (pkgWas) continue;
		for (size_t t=0; t<pkgList.size(); ++t) {
			if (pkgList[t].get_name()!=cutSpaces(list[i])) continue;
			was.push_back(cutSpaces(list[i]));
			csize += atol(pkgList[t].get_compressed_size().c_str());
			isize += atol(pkgList[t].get_installed_size().c_str());
			count++;
		}
	}
	if (!merge) {
		set.isize = 0;
		set.csize = 0;
		set.count = 0;
	}
	set.isize += isize;
	set.csize += csize;
	set.count += count;

}


