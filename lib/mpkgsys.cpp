/*********************************************************
 * MPKG packaging system: general functions
 * $Id: mpkgsys.cpp,v 1.62 2007/12/11 11:52:59 i27249 Exp $
 * ******************************************************/

#include "mpkgsys.h"
#include "package.h"

#include "terminal.h"
string output_dir;

// Cleans system cache
int mpkgSys::clean_cache(bool symlinks_only)
{
	if (!dialogMode) {
		if (!symlinks_only) say(_("Cleaning package cache\n"));
		//else say(_("Cleaning orphaned symlinks in cache\n"));
	}
	if (!symlinks_only) ftw(SYS_CACHE.c_str(), _clean, 50);
	else ftw(SYS_CACHE.c_str(), _clean_symlinks, 50);
	return 0;
}

void mpkgSys::clean_queue(mpkgDatabase *db) {
	PACKAGE_LIST toInstall;
	SQLRecord sqlSearch;
	sqlSearch.setSearchMode(SEARCH_OR);
	sqlSearch.addField("package_action", ST_INSTALL);
	sqlSearch.addField("package_action", ST_REMOVE);
	sqlSearch.addField("package_action", ST_PURGE);
	sqlSearch.addField("package_action", ST_REPAIR);
	sqlSearch.addField("package_action", ST_UPDATE);
	db->get_packagelist(sqlSearch, &toInstall, true);
	for (size_t i=0; i<toInstall.size(); ++i) {
		db->set_action(toInstall[i].get_id(), ST_NONE, "clean");
	}
	db->clear_unreachable_packages();
}

int mpkgSys::unqueue(int package_id, mpkgDatabase *db)
{
	db->set_action(package_id, ST_NONE, "unqueue");
	return 0;
}


int mpkgSys::build_package(string out_directory, bool source)
{

	if (source) say("Building in [%s], source package\n", out_directory.c_str());
	if (!source) say("Building in [%s], binary package\n", out_directory.c_str());

	string pkgname;
	if (out_directory.empty()) out_directory = "./";
	out_directory+="/";
	string pkgType=mConfig.getValue("build_pkg_type");
	if (pkgType.empty()) pkgType="txz";
	printf("pkgType=[%s]\n", pkgType.c_str());
	if (FileNotEmpty("install/data.xml"))
    	{
	    	PackageConfig p("install/data.xml");
	    	if (!p.parseOk) 
		{
			mError("Parse error");
			return -100;
		}
		if (source)
		{
			pkgname =  p.getName()+"-"+p.getVersion()+"-"+p.getBuild();
	    		say(_("Packing source package to %s%s.spkg\n"), out_directory.c_str(), pkgname.c_str());
			string n = out_directory + p.getName()+"-"+p.getVersion()+"-"+p.getBuild()+".spkg";
			unlink(n.c_str());
			system("ls -la && tar -cf " + out_directory + p.getName()+"-"+p.getVersion()+"-"+p.getBuild()+".spkg *");
		}
		else {

			pkgname =  p.getName()+"-"+p.getVersion()+"-"+p.getArch()+"-"+p.getBuild();
			say(_("Packing binary package to %s%s.%s\n"), out_directory.c_str(), pkgname.c_str(), pkgType.c_str());
			string tmp_dir = get_tmp_dir();
			system(MAKEPKG_CMD + " " + tmp_dir + "/" + pkgname+"."+pkgType);
			system("mv " + tmp_dir + "/" + pkgname+"."+pkgType + " " + out_directory);
		}
	}
	else {
		mError("No XML data, cannot build package");
		return -1;
	}
	say(_("Package was built to %s/%s\n"), out_directory.c_str(), pkgname.c_str());
    	return 0;
}
int rsyncDescriptions(const string& path, const string& base_path) {
	cout << _("Retrieving descriptions from ") << path << "..." << endl;
	return system("rsync -arvh " + path + " " + base_path + "/ >/dev/null");
}

int wgetDescriptions(const string& path, const string& base_path) {
	cout << _("Retrieving descriptions from ") << path << "..." << endl;
	string filename = getFilename(path);
	int ret = system("wget " + path + " -O " + base_path + "/" + filename + " >/dev/null");
	if (ret != 0) return ret;
	ret = system("tar xf " + base_path + "/" + filename + " -C " + base_path);
	unlink(string(base_path + "/" + filename).c_str());
	return ret;
}

int mpkgSys::updatePackageDescriptions(const vector< pair<string, string> > &descriptions) {
	
	if (mConfig.getValue("disable_description_sync")=="yes") return 0; // Allow user to disable this functionality

	// First of all: create directory. The path is: SYS_MPKG_VAR_DIRECTORY + "/descriptions";
	const string base_path = SYS_MPKG_VAR_DIRECTORY + "/descriptions";
	system("mkdir -p \"" + base_path + "\"");

	// Now download indexes. There are two methods: rsync and not rsync :) All these are configured via system config, and default is "rsync"
	// Also, user can choose language. Default is "all"
	string preferred_method = mConfig.getValue("description_preferred_method");
	if (preferred_method != "rsync" && preferred_method != "archive") preferred_method = "rsync";
	vector<string> methods;
	if (preferred_method == "rsync") {
		methods.push_back("rsync");
		methods.push_back("archive");
	}
	else if (preferred_method == "archive") {
		methods.push_back("archive");
		methods.push_back("rsync");
	}
	string _lang = mConfig.getValue("description_languages");
	vector<string> preferred_languages = splitString(_lang, " ");
	for (size_t i=0; i<preferred_languages.size(); ++i) {
		if (cutSpaces(preferred_languages[i]).empty()) {
			preferred_languages.erase(preferred_languages.begin() + i);
			i--;
		}
	}
	if (preferred_languages.empty()) preferred_languages.push_back("all");
	string this_method;
	bool lang_got;
	for (size_t l = 0; l < preferred_languages.size(); ++l) {
		lang_got = false;
		for (size_t m = 0; !lang_got && m < methods.size(); ++m) {
			for (size_t d=0; d<descriptions.size(); ++d) {
				if (descriptions[d].first != preferred_languages[l]) continue;
				if (descriptions[d].second.find("rsync://")==0) this_method = "rsync";
				else this_method = "archive";
				if (this_method == methods[m]) {
					if (this_method == "rsync") {
						rsyncDescriptions(descriptions[d].second, base_path);
						lang_got = true;
					}
					else if (this_method == "archive") {
						wgetDescriptions(descriptions[d].second, base_path);
						lang_got = true;
					}
				}
			}
		}
	}


	return 0;
}
int mpkgSys::update_repository_data(mpkgDatabase *db)//, DependencyTracker *DepTracker)
{
	// Функция, с которой начинается обновление данных.
	
	Repository *rep = new Repository;		// Объект репозиториев
	vector< pair<string, string> > package_descriptions;
	rep->package_descriptions = &package_descriptions; // Add pointer to package descriptions
	PACKAGE_LIST *availablePackages = new PACKAGE_LIST;		// Список пакетов, полученных из всех репозиториев...
	PACKAGE_LIST *tmpPackages = new PACKAGE_LIST;		// Список пакетов, полученных из текущего репозитория (временное хранилище)

	// А есть ли у нас вообще репозитории? Может нам и ловить-то нечего?...
	// Впрочем, надо все равно пойти на принцип и пометить все пакеты как недоступные. Ибо это действительно так.
	// Поэтому - проверка устранена.
	if (!dialogMode && !htmlMode) {
#ifdef X86_64
		say(_("Updating package data from %ld repository(s)...\n"), REPOSITORY_LIST.size());
#else
		say(_("Updating package data from %d repository(s)...\n"), REPOSITORY_LIST.size());
#endif

	}
	if (htmlMode) {
		newHtmlPage();
		printHtml("Updating package data from " + IntToStr(REPOSITORY_LIST.size()) + " repository(s)...\n");
	}
	int total_packages=0; // Счетчик полученных пакетов.

	// Поехали! Запрашиваем каждый репозиторий через функцию get_index()
	unsigned int cnt=1;
	pData.clear();
	for (size_t i=0; i<REPOSITORY_LIST.size(); ++i) {
		pData.addItem(REPOSITORY_LIST[i], 1, ITEMSTATE_INPROGRESS);
		pData.setItemCurrentAction(i, _("retrieving index"));
	}
	for (size_t i=0; i<REPOSITORY_LIST.size(); ++i) {
		delete tmpPackages;
		tmpPackages = new PACKAGE_LIST;					//Очищаем временный список.
		pData.setItemChanged(i);
		rep->get_index(REPOSITORY_LIST[i], tmpPackages);	// Получаем список пакетов.
		pData.increaseItemProgress(i);
		pData.setItemState(i, ITEMSTATE_FINISHED);
		cnt++;
		if (!tmpPackages->IsEmpty())				// Если мы таки получили что-то, добавляем это в список.
		{
			total_packages+=tmpPackages->size();		// Увеличим счетчик
			availablePackages->add_list(*tmpPackages);	// Прибавляем данные к общему списку.
		}
		cnt++;
	}
	delete rep;
	delete tmpPackages;
	// Вот тут-то и начинается самое главное. Вызываем фильтрацию пакетов (действие будет происходить в функции updateRepositoryData.
	int ret=db->updateRepositoryData(availablePackages);

	// Now - update package descriptions
	updatePackageDescriptions(package_descriptions);
	if (!dialogMode && !htmlMode) say(_("Update complete.\n"));
	pData.clear();
	return ret;
}
// V1 comments
// Установка (обновление)
// Делаются следующие проверки:
// 1) Проверяется есть ли такой пакет в базе
// 2) Проверяется его доступность
// 3) Проверяется факт обновления
// Если все ок, направляем в DepTracker

int mpkgSys::requestInstall(int package_id, mpkgDatabase *db, DependencyTracker *DepTracker, bool localInstall)
{
	mDebug("requested to install " + IntToStr(package_id));
	PACKAGE tmpPackage;
	int ret = db->get_package(package_id, &tmpPackage);
	SQLRecord sqlSearch;
	sqlSearch.addField("package_name", tmpPackage.get_name());
	PACKAGE_LIST candidates;
	//printf("SLOW GET_PACKAGELIST CALL: %s %d\n", __func__, __LINE__);
	db->get_packagelist(sqlSearch, &candidates, true, false);
	mDebug("received " + IntToStr(candidates.size()) + " candidates to update"); 
	for (unsigned int i=0; i<candidates.size(); i++)
	{
		mDebug("checking " + IntToStr(i));
		if (candidates[i].installed() && !candidates[i].equalTo(tmpPackage))
		{
			//if (!dialogMode) say(_("Updating package %s\n"), candidates[i].get_name().c_str());
			requestUninstall(candidates[i].get_name(), db, DepTracker);
		}
	}
	if (ret == 0)
	{
		if (tmpPackage.installed())
		{
			mWarning(_("Package ") + tmpPackage.get_name() + " " + tmpPackage.get_fullversion() + _(" is already installed"));
		}
		if (!tmpPackage.available(localInstall))
		{
			mError(_("Package ") + tmpPackage.get_name() + " " + tmpPackage.get_fullversion() + _(" is unavailable"));
		}
		if (tmpPackage.available(localInstall) && !tmpPackage.installed())
		{
			tmpPackage.set_action(ST_INSTALL, "new-request");
			if (!ignoreDeps) DepTracker->addToInstallQuery(tmpPackage);
			else db->set_action(tmpPackage.get_id(), ST_INSTALL, "new-request");
			return tmpPackage.get_id();
		}
		else
		{
			return MPKGERROR_IMPOSSIBLE;
		}
	}
	else
	{
		mError(_("get_package error: returned ") + IntToStr (ret));
		return ret;
	}
}
int mpkgSys::requestInstall(PACKAGE_LIST *pkgList, mpkgDatabase *db, DependencyTracker *DepTracker)
{
	for (unsigned int i=0; i<pkgList->size(); i++) {
		//printf("Adding to install: [%d] %s-%s\n", pkgList->at(i).get_id(), pkgList->at(i).get_name().c_str(), pkgList->at(i).get_fullversion().c_str());
		pkgList->get_package_ptr(i)->set_action(ST_INSTALL, "new-request");
		if (!ignoreDeps) DepTracker->addToInstallQuery(pkgList->at(i));
		else db->set_action(pkgList->at(i).get_id(), ST_INSTALL, "new-request");
	}
	return 0;
}
int mpkgSys::requestInstallGroup(string groupName, mpkgDatabase *db, DependencyTracker *DepTracker)
{
	mDebug("requesting data");
	SQLRecord sqlSearch;
	PACKAGE_LIST candidates;
	//printf("SLOW GET_PACKAGELIST CALL: %s %d\n", __func__, __LINE__);
	db->get_packagelist(sqlSearch, &candidates, true, false);
	db->get_full_taglist(&candidates);
	vector<string> install_list;
	for (unsigned int i=0; i<candidates.size(); i++)
	{
		for (unsigned int t=0; t<candidates[i].get_tags().size(); t++)
		{
			if (candidates[i].get_tags().at(t)==groupName)
			{
				install_list.push_back(candidates[i].get_name());
			}
		}
	}
	mDebug("Requesting to install " + IntToStr(install_list.size()) + " packages from group " + groupName);
	for (unsigned int i=0; i<install_list.size(); i++)
	{
		requestInstall(install_list[i],(string) "",(string) "", db, DepTracker);
	}
	return 0;
}

int mpkgSys::requestInstall(PACKAGE *package, mpkgDatabase *db, DependencyTracker *DepTracker)
{
	if (!package->installedVersion.empty() && !package->installed()) requestUninstall(package->get_name(), db, DepTracker); 
	return requestInstall(package->get_id(), db, DepTracker);
}

// New, very very fast function. The only one who should be used, if fact
int mpkgSys::requestInstall(vector<string> package_name, vector<string> package_version, vector<string> package_build, mpkgDatabase *db, DependencyTracker *DepTracker, vector<string> *eList) {
	
	// First of all, check for local packages
	
	vector<int> localPackages;
	vector<bool> isLocal(package_name.size(), false);
	LocalPackage *_p;
	string pkgType;
	for (unsigned int i=0; i<package_name.size(); i++) {
		pkgType=getExtension(package_name[i]);
		if (pkgType=="txz" || pkgType == "tbz" || pkgType == "tlz" || pkgType=="tgz" || pkgType == "spkg") {
		       if (FileExists(package_name[i])) {
				_p = new LocalPackage(package_name[i]);
				_p->injectFile();
				db->emerge_to_db(&_p->data);
				package_name[i] = _p->data.get_name();
				package_version[i] = _p->data.get_version();
				package_build[i] = _p->data.get_build();
				isLocal[i]=true;
				//printf("\nDetected local package\nFilename: %s\nName:%s\nVersion:%s\n", _p->data.get_filename().c_str(), _p->data.get_name().c_str(), _p->data.get_version().c_str());
				localPackages.push_back(_p->data.get_id());
				delete _p;
			}
		}
	}
	vector<string> errorList;
	//printf("using error list\n");
	// Creating DB cache
	// 1. Creating a request for all packages which are in package_name vector.
	SQLRecord sqlSearch;
	sqlSearch.setSearchMode(SEARCH_IN);
	for (unsigned int i=0; i<package_name.size(); i++) {
		if (isLocal[i]) {
			continue;
		}
		sqlSearch.addField("package_name", package_name[i]);
	}
	// 2. Requesting database by search array
	PACKAGE_LIST pCache;
	//printf("SLOW GET_PACKAGELIST CALL: %s %d\n", __func__, __LINE__);
	int query_ret = db->get_packagelist(sqlSearch, &pCache, true, false);
	if (query_ret != 0) {
		errorList.push_back(mError("Error querying database"));
		if (eList) *eList = errorList;
		return MPKGERROR_SQLQUERYERROR;
	}
	// 3. Temporary matrix, temporary list (for each package), and result list
	vector<PACKAGE_LIST> tmpMatrix;
	PACKAGE_LIST *tmpList=new PACKAGE_LIST;
	PACKAGE_LIST resultList;
	PACKAGE_LIST uninstallList;
	// 4. Search by cache for installed ones, check for availability and select the appropriate versions
	// 4.1 Building matrix: one vector per list of packages with same name
	for (unsigned int i=0; i<package_name.size(); i++) {
		delete tmpList;
		tmpList = new PACKAGE_LIST;
		for (unsigned int t=0; t<pCache.size(); t++) {
			if (pCache.at(t).get_name() == package_name[i]) {
				if (isLocal[i] && pCache[t].get_id()!=localPackages[i]) continue;
				tmpList->add(pCache.at(t));
			}
		}
		tmpMatrix.push_back(*tmpList);
	}
	//printf("tmpMatrix[0] size = %d\n", tmpMatrix[0].size());
	// So, the matrix has been created.
	// In case of any error, collect all of them, and return MPKGERROR_IMPOSSIBLE
	
	// Sizes of tmpMatrix and input vectors are the same, so we can use any of them
	PACKAGE *outPackage = NULL, *installedOne = NULL;
	for (unsigned int i=0; i<tmpMatrix.size(); i++) {
		delete tmpList;
		tmpList = new PACKAGE_LIST;
		for (unsigned int t=0; t<tmpMatrix[i].size(); t++) {
			// Filling the tmpList with reachable (=installed or available) ones for each package
			if (tmpMatrix[i].at(t).available(true) || tmpMatrix[i].at(t).installed()) {
				if (package_version[i].empty() || tmpMatrix[i].at(t).get_version() == package_version[i]) {
					if (package_build[i].empty() || package_build[i]==tmpMatrix[i].at(t).get_build()) tmpList->add(tmpMatrix[i][t]);
				}
			}
		}
		// Now, we have a list of all good candidates. We will filter already installed ones separately for better UI output.
		tmpList->initVersioning();
		outPackage = tmpList->getMaxVersion();
		//if (outPackage) printf("outPackage VERSION: %s\n", outPackage->get_fullversion().c_str());
		installedOne = (PACKAGE *) tmpMatrix[i].getInstalledOne();
		if (outPackage == NULL) {
			string errorText = _("Requested package ") + package_name[i];
		       	if (!package_version[i].empty()) errorText += "-" + package_version[i];
			if (!package_build[i].empty()) errorText += "-" + package_build[i];
			if (!installedOne) errorList.push_back(mError(errorText + _(" cannot be found")));
			else errorList.push_back(mError(errorText + _(" is already installed")));
		}
		else {
			//printf("____________________________CHECK FOR UPDATE, installedOne: %p_____________________________\n", installedOne);
			// Check for update
			if (installedOne && outPackage->get_id() != installedOne->get_id()) {
				// This is update
				//printf("added to uninstall: %d\n", installedOne->get_id());
				uninstallList.add(*installedOne);
			}
			resultList.add(*outPackage);
		}
	}
	delete tmpList;
	// Special addition for local packages installed using -z key: check for installed one
	for (size_t i=0; i<isLocal.size(); ++i) {
		if (!isLocal[i]) continue;
		for (size_t t=0; t<pCache.size(); ++t) {
			if (pCache[t].installed() && pCache[t].get_id()!=localPackages[i] && pCache[t].get_name()==package_name[i]) {
				requestUninstall(pCache.get_package_ptr(t), db, DepTracker);
			}
		}
	}
	// Now, check resultList for installed ones and unavailable ones
	for (unsigned int i=0; i<resultList.size(); i++) {
		if (resultList[i].installed()) {
			mWarning(_("Package ") + resultList[i].get_name() + "-" + resultList[i].get_fullversion() + _(" is already installed"));
		} 
		else {
			if (!resultList[i].available(true)) {
				errorList.push_back(mError(_("Package ") + resultList[i].get_name() + "-" + resultList[i].get_fullversion() + _(" is unavailable")));
			}
		}
	}
	// NEW: ignore already installed packages
	tmpList = new PACKAGE_LIST;
	for (unsigned int i=0; i<resultList.size(); ++i) {
		if (!resultList[i].installed()) {
			tmpList->add(resultList[i]);
		}
	}
	resultList = *tmpList;
	delete tmpList;
	

	//printf("resultList size = %d\n", resultList.size());
	if (errorList.empty())	{
		// Push to database
		requestInstall(&resultList, db, DepTracker);
		for (unsigned int i=0; i<uninstallList.size(); i++) requestUninstall(uninstallList.get_package_ptr(i), db, DepTracker);

	}
	else {
		mError(_("Errors detected, cannot continue"));
		if (eList) *eList = errorList;
		return MPKGERROR_IMPOSSIBLE;
	}

	if (eList) *eList = errorList;
	return 0;
}
int mpkgSys::requestInstall(string package_name, string package_version, string package_build, mpkgDatabase *db, DependencyTracker *DepTracker)
{
	// Exclusion for here: package_name could be a filename of local placed package.
	bool tryLocalInstall=false;
	SQLRecord sqlSearch;
	sqlSearch.addField("package_name", package_name);
	if (!package_version.empty()) {
		sqlSearch.addField("package_version", package_version);
		//printf("adding package version [%s] to search\n", package_version.c_str());
	}
	if (!package_build.empty()) sqlSearch.addField("package_build", package_build);
	PACKAGE_LIST candidates;
	//printf("SLOW GET_PACKAGELIST CALL: %s %d\n", __func__, __LINE__);
	int ret = db->get_packagelist(sqlSearch, &candidates, true, false);
	mDebug("received " + IntToStr(candidates.size()) + " candidates for " + package_name);
	candidates.initVersioning();
	// TODO: let user know if multiple packages with same name, version and build are available, and let him choose.
	int id=-1;
	if (ret == 0)
	{
		mDebug("checking id...");
		id = candidates.getMaxVersionID(package_name);
		mDebug("id = " + IntToStr(id));
		if (id>=0)
		{
			mDebug("requesting to install " + candidates[candidates.getMaxVersionNumber(package_name)].get_fullversion());
			return requestInstall(id, db, DepTracker);
		}
		else 
		{

			mDebug("trying local install");
			if (FileExists(package_name)) tryLocalInstall=true;
		}
	}
	
	if (!tryLocalInstall || !FileExists(package_name))
	{
		string error_package = package_name;
		if (!package_version.empty()) error_package += _(" version ") + package_version;
		if (!package_build.empty()) error_package += _(" build ") + package_build;
		mError(_("No such package: ") + error_package);
		return MPKGERROR_NOPACKAGE;
	}

	else
	{
		/*
		if (getExtension(package_name)=="spkg")
		{
			if (emerge_package(package_name, &package_name)!=0 || !FileExists(package_name)) {
				mError(_("Cannot build this spkg"));
				return MPKGERROR_NOPACKAGE;
			}
		}*/
		msay(_("Inspecting local package ") + package_name);
		LocalPackage _p(package_name);
		_p.injectFile();
		db->emerge_to_db(&_p.data);
		requestInstall(_p.data.get_id(), db, DepTracker, true);
		return 0;
	}
}	
// Удаление
int mpkgSys::requestUninstall(PACKAGE *package, mpkgDatabase *db, DependencyTracker *DepTracker, bool purge)
{
	//printf("requestUninstall by PACKAGE FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF\n");
	return requestUninstall(package->get_id(), db, DepTracker, purge);
}
int mpkgSys::requestUninstall(PACKAGE_LIST *pkgList, mpkgDatabase *db, DependencyTracker *DepTracker, bool purge) {
	int ret=0;
	for (unsigned int i=0; i<pkgList->size(); ++i) {
		ret += requestUninstall(pkgList->get_package_ptr(i), db, DepTracker, purge);
	}
	return ret;
}
int mpkgSys::requestUninstall(int package_id, mpkgDatabase *db, DependencyTracker *DepTracker, bool purge)
{
	//printf("requestUninstall by ID, !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!ignoreDeps = %d\n", ignoreDeps);
	mDebug("requestUninstall\n");
	PACKAGE tmpPackage;
	int ret = db->get_package(package_id, &tmpPackage);
	mDebug("uninstall called for id " + IntToStr(package_id) + ", name = " + tmpPackage.get_name() + "-" + tmpPackage.get_fullversion());
	bool process=false;
	if (ret == 0)
	{
		if (tmpPackage.configexist())
		{
			if (purge)
			{
				mDebug("action set to purge");
				tmpPackage.set_action(ST_PURGE, "direct");
				process=true;
			}
			else if (tmpPackage.installed())
			{
				mDebug("action is remove");
				tmpPackage.set_action(ST_REMOVE, "direct");
				process=true;
			}
		}
		else
		{
			say(_("%s-%s doesn't present in the system\n"), tmpPackage.get_name().c_str(), tmpPackage.get_fullversion().c_str());
		}
		if (process)
		{
			mDebug("Processing deps");
			if (!ignoreDeps) {
				//printf("added to remove queue\n");
				DepTracker->addToRemoveQuery(tmpPackage);
			}
			else {
				if (purge) db->set_action(tmpPackage.get_id(), ST_PURGE, "direct");
				else db->set_action(tmpPackage.get_id(), ST_REMOVE, "direct");
			}

			return tmpPackage.get_id();
		}
		else
		{
			if (purge) mError(_("Package ") + tmpPackage.get_name() + " "+ tmpPackage.get_fullversion() + _(" is already purged"));
			else  mError(_("Package ") + tmpPackage.get_name() + " " + tmpPackage.get_fullversion() + _(" is already purged"));
;
			return MPKGERROR_IMPOSSIBLE;
		}
	}
	else
	{
		return ret;
	}
}
int mpkgSys::requestUninstall(string package_name, mpkgDatabase *db, DependencyTracker *DepTracker, bool purge)
{
	//printf("requestUninstall by name !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	mDebug("requestUninstall of " + package_name);
	SQLRecord sqlSearch;
	sqlSearch.addField("package_name", package_name);
	if (!purge) sqlSearch.addField("package_installed", 1);
	else sqlSearch.addField("package_configexist",1);
	PACKAGE_LIST candidates;
	//printf("SLOW GET_PACKAGELIST CALL: %s %d\n", __func__, __LINE__);
	int ret = db->get_packagelist(sqlSearch, &candidates, true, false);
	mDebug("candidates to uninstall size = " + IntToStr(candidates.size()));
	int id=-1;
	if (ret == 0)
	{
		if (candidates.size()>1)
		{
			mError(_("Ambiguity in uninstall: multiple packages with some name are installed"));
			return MPKGERROR_AMBIGUITY;
		}
		if (candidates.IsEmpty())
		{
			mError(_("Cannot remove package ") + package_name + _(": not installed"));
			return MPKGERROR_NOPACKAGE;
		}
		id = candidates[0].get_id();
		if (id>=0)
		{
			return requestUninstall(id, db, DepTracker, purge);
		}
		else return MPKGERROR_NOPACKAGE;
	}
	else return ret;
}



int mpkgSys::_clean(const char *filename, const struct stat *file_status, int filetype)
{
	unsigned short x=0, y=0;

	if (file_status->st_ino!=0) x=y;
	if (verbose && !dialogMode) say(_("Removing %s\n"), filename);
	switch(filetype)
	{
		case FTW_F:
			unlink(filename);
			break;
		case FTW_D:
			rmdir(filename);
			break;
		default:
			unlink(filename);
	}
	return 0;
}

int mpkgSys::_clean_symlinks(const char *filename, const struct stat *, int ) {
	if (get_file_type_stat(filename)==FTYPE_SYMLINK) {
		if (verbose && !dialogMode) say(_("Removing symlink %s\n"), filename);
		unlink(filename);
	}
	return 0;
}


int mpkgSys::_conv_dir(const char *filename, const struct stat *, int filetype) {
	if (filetype!=FTW_F) return 0;
       	string ext=getExtension(filename);
	if (ext!="tgz" && ext!="tbz" && ext!="txz" && ext!="tlz") return 0;
	convert_package(string(filename), output_dir);
	return 0;

}

int mpkgSys::_nativize_dir(const char *filename, const struct stat *, int filetype) {
	if (filetype!=FTW_F) return 0;
       	string ext=getExtension(filename);
	if (ext!="tgz" && ext!="tbz" && ext!="txz" && ext!="tlz") return 0;

	LocalPackage lp(filename);
	if (lp.isNative()) return 0;
	convert_package(string(filename), output_dir);
	system("mv -v " + output_dir + "/" + string(filename) + " " + string(filename));
	return 0;
}

int mpkgSys::convert_directory(string out_dir) {
	output_dir=getAbsolutePath(out_dir);
	ftw("./", _conv_dir, 100);
	return 0;
}

int mpkgSys::nativize_directory(string input_dir) {
	output_dir = get_tmp_dir();
	ftw(input_dir.c_str(), _nativize_dir, 100);
	return 0;
}
