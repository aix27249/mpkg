/* Dependency tracking
$Id: dependencies.cpp,v 1.53 2007/11/23 01:01:46 i27249 Exp $
*/


#include "dependencies.h"
#include "debug.h"
//#include "constants.h"
#include <mpkgsupport/mpkgsupport.h>
const PACKAGE_LIST& DependencyTracker::get_install_list() const {
	return installList;
}

// Note: seems to be not used anywhere.
const PACKAGE_LIST& DependencyTracker::get_remove_list() const {
	return removeList;
}

const PACKAGE_LIST& DependencyTracker::get_failure_list() const {
	return failure_list;
}
void consoleLog(string data) {
	system("echo '" + data + "' >> /tmp/console.log");
}
void DependencyTracker::reset() {
	installQueryList.clear();
	removeQueryList.clear();
	installList.clear();
	removeList.clear();
	failure_list.clear();
	installedPackages.clear();
	packageCache.clear();
	cacheCreated=false;
	_tmpInstallStream=NULL;
	_tmpRemoveStream=NULL;
}
void DependencyTracker::addToInstallQuery(const PACKAGE& pkg)
{
	installQueryList.add(pkg);
}
void DependencyTracker::addToRemoveQuery(const PACKAGE& pkg)
{
	removeQueryList.add(pkg);
}

int findBrokenPackages(const PACKAGE_LIST& pkgList, PACKAGE_LIST *output)
{
	int counter=0;
	for (unsigned int i=0; i<pkgList.size(); i++)
	{
		if (pkgList[i].isBroken)
		{
			counter++;
			output->add(pkgList[i]);
		}
	}
	//printf("found %d broken packages\n", counter);
	return counter;
}

void filterDupes(PACKAGE_LIST *pkgList, bool removeEmpty) {
	PACKAGE_LIST output;
	
	// Comparing by MD5.
	vector<string> known_md5;
	bool dupe;
	for (size_t i=0; i<pkgList->size(); i++) {
		dupe=false;
		if (removeEmpty && pkgList->get_package_ptr(i)->IsEmpty()) dupe=true;
		else {
			for (size_t t=0; t<known_md5.size(); ++t) {
				if (pkgList->at(i).get_md5()==known_md5[t]) {
					dupe=true;
					break;
				}
			}
		}
		if (!dupe) {
			output.add(pkgList->at(i));
			known_md5.push_back(pkgList->at(i).get_md5());
		}
	}
	*pkgList=output;
}
int getMaxVersionCoreNumber(const PACKAGE_LIST &pkgList, const string& corename) {
	for (size_t i=0; i<pkgList.size(); ++i) {
		if (pkgList[i].get_corename() == corename && pkgList[i].hasMaxVersion) return i;
	}
	return MPKGERROR_NOPACKAGE;
}
// Filter different packages with same names. Max version will be reserved, all others - kicked
void filterDupeNames(PACKAGE_LIST *pkgList) {
	pkgList->initVersioning();
	PACKAGE_LIST ret;
	int num;
	for (unsigned int i=0; i<pkgList->size(); ++i) {
		num = getMaxVersionCoreNumber(*pkgList, pkgList->at(i).get_corename());
		if (num!=MPKGERROR_NOPACKAGE) ret.add(pkgList->at(num));
	}
	*pkgList=ret;
}
void DependencyTracker::createPackageCache()
{
	// Let's check if there is anything to do.
	SQLRecord sqlSearch;
	sqlSearch.setSearchMode(SEARCH_IN);
	sqlSearch.addField("package_action", ST_REMOVE);
	sqlSearch.addField("package_action", ST_PURGE);
	sqlSearch.addField("package_action", ST_INSTALL);
	sqlSearch.addField("package_action", ST_REPAIR);

	PACKAGE_LIST dummyList;
	db->get_packagelist(sqlSearch, &dummyList, true, false);

	if (dummyList.size()==0) {
		cacheCreated=true;
	       	return;
	}

	SQLRecord cacheSqlSearch;
	//printf("SLOW GET_PACKAGELIST CALL: %s %d\n", __func__, __LINE__);
	db->get_packagelist(cacheSqlSearch, &packageCache, true, false);	
	//printf("FULL DEPLIST FETCH\n");
	db->get_full_dependencylist(&packageCache);
	//printf("FULL DEPLIST FETCH END\n");
	cacheCreated=true;
}

// This function is used to create a virtual package cache. It requires to do analytics work on fake database without making deal with real one.
void DependencyTracker::setFakePackageCache(const PACKAGE_LIST &fakePkgCache) {
	packageCache = fakePkgCache;
	cacheCreated = true;

}
void DependencyTracker::fillInstalledPackages()
{
	if (!cacheCreated) createPackageCache();
	installedPackages.clear();
	for (unsigned int i=0; i<packageCache.size(); i++)
	{
		if (packageCache[i].installed()) installedPackages.add(packageCache[i]);
	}
}
//#define DEPDEBUG
int DependencyTracker::renderDependenciesInPackageList(PACKAGE_LIST *pkgList)
{
	//printf("%s\n", __func__);
	// A *very* special case: need to compute dependencies within single package list, with no access to database
	packageCache = *pkgList;
	cacheCreated=true;
	fillInstalledPackages();
	int failureCounter = 0;
	// Forming pseudo-queue
	for (size_t i=0; i<packageCache.size(); i++)
	{
		if (packageCache[i].action()==ST_INSTALL) installQueryList.add(packageCache[i]);
		else if (packageCache[i].action()==ST_REMOVE || \
				packageCache[i].action()==ST_PURGE) removeQueryList.add(packageCache[i]);
	}
	// From this to $END, debug code
	size_t avCount=0;
	for (size_t i=0; i<packageCache.size(); ++i) {
		if (packageCache[i].isBroken) avCount++;
		
	}
	PACKAGE_LIST installStream;
        _tmpInstallStream = &installStream;
	installStream = renderRequiredList(&installQueryList);
#ifdef DEPDEBUG
	vector<string> depLog;
	for (size_t i=0; i<installStream.size(); ++i) {
		depLog.push_back(installStream[i].get_name());
	}
	WriteFileStrings("/tmp/installStream.log", depLog);
#endif
	PACKAGE_LIST removeStream;
        _tmpRemoveStream = &removeStream;
	removeStream = renderRemoveQueue(removeQueryList);
	filterDupes(&installStream);
	filterDupes(&removeStream);
	PACKAGE_LIST fullWillBeList = installedPackages;
	fullWillBeList.add(installStream);
#ifdef DEPDEBUG
	depLog.clear();
	for (size_t i=0; i<fullWillBeList.size(); ++i) {
		depLog.push_back(fullWillBeList[i].get_name());
	}
	WriteFileStrings("/tmp/fullWillBeList.log", depLog);
#endif

	bool requested = false;
	for (unsigned int i=0; i<removeStream.size(); i++)
	{
		requested=false;
		for (unsigned int t=0; t<removeQueryList.size(); t++)
		{
			if (removeStream[i].equalTo(removeQueryList[t]))
			{
				requested=true;
				break;
			}
		}
		if (!requested && checkBrokenDeps(removeStream.get_package_ptr(i), fullWillBeList))
		{
			mDebug("Rolling back " + removeStream[i].get_name());
			installStream.add(removeStream[i]);
		}
	}
	muxStreams(installStream, removeStream);
#ifdef DEPDEBUG
	depLog.clear();
	for (size_t i=0; i<installList.size(); ++i) {
		depLog.push_back(installList[i].get_name());
	}
	WriteFileStrings("/tmp/instalList.log", depLog);
#endif


	failureCounter = findBrokenPackages(installList, &failure_list);

	// Summarize and push back
	
	// Reset
	for (unsigned int i=0; i<pkgList->size(); i++)
	{
		pkgList->get_package_ptr(i)->set_action(ST_NONE, "reset");
	}
	int iC=0;
	vector<int> i_ids;
	bool alreadyThere;
	for (unsigned int i=0; i<installList.size(); i++)
	{
		if (!installList[i].installed())
		{
			alreadyThere=false;
			for (unsigned int v=0; v<i_ids.size(); v++)
			{
				if (i_ids[v]==installList[i].get_id())
				{
					alreadyThere=true;
					break;
				}
			}
			if (!alreadyThere)
			{
				iC++;
				pkgList->getPackageByIDPtr(installList[i].get_id())->set_action(ST_INSTALL, "inlist_new");
				i_ids.push_back(installList[i].get_id());
			}
		}
	}
	int rC=0;
	vector<int> r_ids;
	
	for (unsigned int i=0; i<removeList.size(); i++)
	{
		if (removeList[i].configexist())
		{
			alreadyThere=false;
			for (unsigned int v=0; v<r_ids.size(); v++)
			{
				if (r_ids[v]==removeList[i].get_id()) {
					alreadyThere=true;
					break;
				}
			}
			if (!alreadyThere)
			{
				rC++;
				pkgList->getPackageByIDPtr(removeList[i].get_id())->set_action(ST_REMOVE, "direct");
				r_ids.push_back(removeList[i].get_id());
			}
		}
	}
#ifdef DEPDEBUG
	depLog.clear();
	for (size_t i=0; i<installList.size(); ++i) {
		depLog.push_back(installList[i].get_name());
	}
	WriteFileStrings("/tmp/installList2.log", depLog);
#endif


	filterDupes(&installList);
	filterDupes(&removeList);
#ifdef DEPDEBUG
	depLog.clear();
	for (size_t i=0; i<installList.size(); ++i) {
		depLog.push_back(installList[i].get_name());
	}
	WriteFileStrings("/tmp/installList3.log", depLog);
#endif


	filterDupeNames(&installList);
	filterDupeNames(&removeList);

#ifdef DEPDEBUG
	depLog.clear();
	for (size_t i=0; i<installList.size(); ++i) {
		depLog.push_back(installList[i].get_name());
	}
	WriteFileStrings("/tmp/installList4_final.log", depLog);
#endif




	_tmpRemoveStream = NULL;
	_tmpInstallStream = NULL;
	if (!force_dep) return failureCounter;
	else return 0;

}

int DependencyTracker::renderData()
{
	//printf("renderData(): installList size = %d\n", installList.size());
	createPackageCache();
	fillInstalledPackages();

	mDebug("Rendering installations");
	int failureCounter = 0;
	//printf("rendering required list\n");
	PACKAGE_LIST installStream;
       _tmpInstallStream = &installStream;
	installStream = renderRequiredList(&installQueryList);
	

	mDebug("Rendering removing");
	//printf("Rendering remove queue\n");
	PACKAGE_LIST removeStream;
       _tmpRemoveStream = &removeStream;
	removeStream = renderRemoveQueue(removeQueryList);

	mDebug("Filtering dupes: install");
	currentStatus=_("Checking dependencies: filtering (stage 1: installation queue dupes");
	//printf("Filtering dupes\n");
	filterDupes(&installStream);
	currentStatus=_("Checking dependencies: filtering (stage 2: remove queue dupes");
	mDebug("Filtering dupes: remove");
	filterDupes(&removeStream);
	//printf("preparing rollback\n");
	currentStatus=_("Checking dependencies: filtering (stage 3: rollback)");
	mDebug("Rolling back the items who was dropped on update");
	PACKAGE_LIST fullWillBeList = installedPackages;
	fullWillBeList.add(installStream);
	bool requested=false;
	//printf("Starting a loop of rollbacking\n");
	for (unsigned int i=0; i<removeStream.size(); i++)
	{
		requested=false;
		for (unsigned int t=0; t<removeQueryList.size(); t++)
		{
			if (removeStream[i].equalTo(removeQueryList[t]))
			{
				// Package is requested to remove by user, we shouldn't roll back
				requested=true;
				break;
			}
		}
		if (!requested && checkBrokenDeps(removeStream.get_package_ptr(i), fullWillBeList))
		{
			// If package:
			//    - isn't requested to remove by user, and
			//    - some of packages requested to install depends on it
			// then we should cancel it's removal

			mDebug("Rolling back " + removeStream[i].get_name());
			installStream.add(removeStream[i]);
		}

	}
	//printf("rollback end\n");
	// END OF ROLLBACK MECHANISM

	mDebug("Muxing streams");
	//printf("muxing streams\n");
	currentStatus=_("Checking dependencies: muxing queues");
	muxStreams(installStream, removeStream);
	
	// Имеем:
	// installList
	// removeList
#ifndef NO_FIND_UPDATE
	currentStatus=_("Advanced search for updates");
	// Fill in fullWillBeList
	fullWillBeList = installList;
	/*printf("STEP 2: installList size = %d\n", installList.size());
	for (int i=0; i<installList.size(); i++) {
		printf("STEP 2 LIST: %d: ID=%d\n", i, installList[i].get_id());
	}*/

	bool skip;
	for (size_t i=0; i<installedPackages.size(); i++) {
		skip=false;
		for (size_t t=0; t<removeList.size(); t++) {
			if (removeList[t].get_id()==installedPackages[i].get_id()) {
				skip=true;
				break;
			}
		}
		if (!skip) fullWillBeList.add(installedPackages[i]);
	}
	//printf("removeList.size = %d, removeQueryList.size = %d\n", removeList.size(), removeQueryList.size());
	/*PACKAGE *checkPackage; // Temporary package, I don't wish to risk packageCache integrity
	for (unsigned int i=0; i<removeList.size(); i++) {
		requested=false;
		for (unsigned int t=0; t<removeQueryList.size(); t++) {
			if (removeList[i].equalTo(removeQueryList[t])) {
				// Package is requested to remove by user, we shouldn't roll back
				requested=true;
				break;
			}
		}

		if (!requested) {
			printf("Not requested, but broken deps: %s %s\n", removeList[i].get_name().c_str(), removeList[i].get_fullversion().c_str());
			// Here is a place to try to find replacement with new dependencies
			// Поищем среди доступных версии пакета, способные заменить сломанный пакет. По нахождении добавляем в installList
			//printf("packageCache size = %d\n", packageCache.size());
			for (unsigned int t=0; t<packageCache.size(); t++) {
				checkPackage = packageCache.get_package_ptr(t);
				if (checkPackage->get_name()==removeList[i].get_name()) { // Нашли пакет с таким же именем, смотрим что у него там с зависимостями
					if (check_deps(checkPackage, &fullWillBeList)) { // Подходит
						printf("adding package %s %s\n", checkPackage->get_name().c_str(), checkPackage->get_fullversion().c_str());
						installList.add(*checkPackage);
						printf("Added %s-%s for update broken package %s-%s\n", checkPackage->get_name().c_str(), 
								checkPackage->get_fullversion().c_str(),
								removeList[i].get_name().c_str(), removeList[i].get_fullversion().c_str());
						break; // Раз нашли один подходящий, нафиг что-либо еще?
					}
				}
			}
		}
	}
	*/
#endif
	currentStatus=_("Checking dependencies: searching for broken packages");
	mDebug("Searching for broken packages");
	failureCounter = findBrokenPackages(installList, &failure_list);
	mDebug("done");
	currentStatus=_("Dependency check completed, error count: ") + IntToStr(failureCounter);
	_tmpRemoveStream = NULL;
	_tmpInstallStream = NULL;

	filterDupeNames(&installList);
	filterDupeNames(&removeList);
	filterDupes(&installList);
	filterDupes(&removeList);
	if (!force_dep) return failureCounter;
	else return 0;
}
// Tree
PACKAGE_LIST DependencyTracker::renderRequiredList(PACKAGE_LIST *installationQueue)
{
	currentStatus=_("Checking dependencies: rendering requirements");
	mDebug("Rendering required list\n");
	// installationQueue - user-composed request for installation
	// outStream - result, including all required packages.
	PACKAGE_LIST outStream;
	PACKAGE_LIST req;
	outStream.add(*installationQueue);
	bool skipThis;
	for (unsigned int i=0; i<outStream.size(); i++)
	{
		currentStatus = _("Checking dependencies: rendering requirements") + (string) " (" + IntToStr(i) + "/"+IntToStr(outStream.size()) + ")";
		//printf("%s: cycle %d\n",__func__, i);
		req=get_required_packages(outStream.get_package_ptr(i));
		//printf("Package was get\n");
		// Check if this package is already in stream
		for (unsigned int t=0; t<req.size(); t++)
		{
			skipThis=false;
			//printf("Cheking cycle\n");
			// Will check and add by one
			for (unsigned int c=0; c<outStream.size(); c++)
			{
				if (req[t].installed() || req[t].get_id()==outStream[c].get_id()) 
				{
					skipThis=true;
					break;
				}
			}
			//printf("cycle complter\n");
			if (!skipThis) outStream.add(req[t]);
		}
		//outStream.add(&req);
	}
	//printf("end\n");
	return outStream;
}

PACKAGE_LIST DependencyTracker::get_required_packages(PACKAGE *package)
{
	// Returns a list of required packages. Broken ones is marked internally
	PACKAGE_LIST requiredPackages;
	PACKAGE tmpPackage;
	int ret;
	for (size_t i=0; i<package->get_dependencies().size(); ++i) {
		// First, check if it can be resolved using installed packages, next - using installQueryList, and next using everything
		ret = get_dep_package((DEPENDENCY *) &package->get_dependencies().at(i), &tmpPackage, &installedPackages);
		if (ret) {
			ret = get_dep_package((DEPENDENCY *) &package->get_dependencies().at(i), &tmpPackage, &installQueryList);
		}
		if (ret) {
			ret = get_dep_package((DEPENDENCY *) &package->get_dependencies().at(i), &tmpPackage);
		}
		if (ret!=0) {
			if (ret == MPKGERROR_NOPACKAGE) ret = DEP_NOTFOUND;
			else if (ret == MPKGERROR_NOSUCHVERSION) ret = DEP_VERSION;
			depErrorTable.add(package->get_id(), package->get_name(), package->get_fullversion(), package->get_dependencies().at(i).get_package_name(), package->get_dependencies().at(i).getDepInfo(), ret);
			mDebug(_("package ") + package->get_name() + " " + package->get_fullversion() + _(" is broken")); package->set_broken();
		}
		else requiredPackages.add(tmpPackage);
	}
	return requiredPackages;
}

bool DependencyTracker::check_deps(PACKAGE *package, PACKAGE_LIST *pList) // Checks if package deps can be resolved using pList
{
	// Returns a list of required packages. Broken ones is marked internally
	PACKAGE tmpPackage;
	for (unsigned int i=0; i<package->get_dependencies().size(); i++)
	{
		//printf("%s-%s: checking dependency %s\n", package->get_name()->c_str(), package->get_fullversion().c_str(), package->get_dependencies()->at(i).getDepInfo().c_str());
		if (get_dep_package((DEPENDENCY *) &package->get_dependencies().at(i), &tmpPackage, pList)!=0) {
			return false;
		}
	}
	//printf("Package %s-%s meets requirements\n", package->get_name()->c_str(), package->get_fullversion().c_str());
	return true;
}

void DependencyTracker::fillByName(const string& name, PACKAGE_LIST *p, PACKAGE_LIST *testPackages)
{
	PACKAGE_LIST *list;
	if (testPackages==NULL) {
		if (!cacheCreated) createPackageCache();
		list = &packageCache;
	}
	else list = testPackages;
	p->clear();
	for (size_t i=0; i<list->size(); ++i) {
		if (list->at(i).get_corename()==name) p->add(list->at(i));
	}
}

/*bool hasWillBeInstalled(const PACKAGE_LIST &pkgList) {
	for (size_t i=0; i<pkgList.size(); ++i) {
		if (pkgList[i].action()==ST_INSTALL) return true;
	}
	return false;
}*/

PACKAGE* getWillBeInstalled(PACKAGE_LIST &pkgList) {
	for (size_t i=0; i<pkgList.size(); ++i) {
		if (pkgList[i].action()==ST_INSTALL) return pkgList.get_package_ptr(i);
	}
	return NULL;


}

int DependencyTracker::get_dep_package(DEPENDENCY *dep, PACKAGE *returnPackage, PACKAGE_LIST *pList)
{
	returnPackage->clear();
	returnPackage->isRequirement=true;
	returnPackage->set_name(dep->get_package_name());
	returnPackage->set_broken(true);
	returnPackage->set_requiredVersion(dep->get_version_data());
	PACKAGE_LIST reachablePackages;
	PACKAGE *tmpRetPackage;
	fillByName(dep->get_package_name(), &reachablePackages, pList);
	
	if (reachablePackages.IsEmpty())
	{
		if (pList==NULL) mDebug(_("Required package ") + dep->get_package_name() + _(" not found"));
		return MPKGERROR_NOPACKAGE;
	}
	
	PACKAGE_LIST candidates;
	// First of all, let's determine what alt is installed at this moment.
	/*
	string altname;
	for (size_t i=0; i<reachablePackages.size(); ++i) {
		if (reachablePackages[i].installed()) {
			altname = reachablePackages[i].get_name();
			break;
		}
	}
	if (!altname.empty()) {
		fprintf(stderr, "altname: %s\n", altname.c_str());
		// Round zero: let's find candidates with same altversion
		for (size_t i=0; i<reachablePackages.size(); ++i) {
			if (reachablePackages[i].get_name()!=altname) continue;
				if (reachablePackages[i].reachable() && meetVersion(dep->get_version_data(), reachablePackages[i].get_version())) {
				candidates.add(reachablePackages.at(i));
				fprintf(stderr, "Added candidate (round 0): %s %s\n", reachablePackages[i].get_name().c_str(), reachablePackages[i].get_fullversion().c_str());
			}
		}
	}
	
	if (altname.find("cairo")!=std::string::npos) fprintf(stderr, "cairo altname check complete: %d\n", candidates.size());
	// If nothing found - let's search another ones
	// Round one: same name (no alts)
	if (candidates.IsEmpty()) {
		for (size_t i=0; i<reachablePackages.size(); ++i) {
			if (reachablePackages[i].get_name()!=reachablePackages[i].get_corename()) continue;
			if (reachablePackages[i].reachable() && meetVersion(dep->get_version_data(), reachablePackages[i].get_version())) {
				candidates.add(reachablePackages.at(i));
				fprintf(stderr, "Added candidate (round 1): %s %s\n", reachablePackages[i].get_name().c_str(), reachablePackages[i].get_fullversion().c_str());
			}
		}
	}
	
	if (altname.find("cairo")!=std::string::npos) fprintf(stderr, "cairo corename check complete: %d\n", candidates.size()); */
	// Finally, if no unaltered ones found - let's pick any
	if (candidates.IsEmpty()) {
		for (size_t i=0; i<reachablePackages.size(); i++) {
			if (reachablePackages[i].reachable() && meetVersion(dep->get_version_data(), reachablePackages[i].get_version())) {
				candidates.add(reachablePackages.at(i));
				//fprintf(stderr, "Added candidate (round 2): %s %s\n", reachablePackages[i].get_name().c_str(), reachablePackages[i].get_fullversion().c_str());
			}
		}
	}


	//if (altname.find("cairo")!=std::string::npos) fprintf(stderr, "cairo anyname check complete: %d\n", candidates.size());
	if (candidates.IsEmpty())
	{
		if (pList==NULL) mDebug(dep->getDepInfo() + _(" is required, but no suitable version was found"));
		return MPKGERROR_NOSUCHVERSION;
	}

	//if (candidates.hasInstalledOnes()) *returnPackage = *candidates.getInstalledOne();
	tmpRetPackage = (PACKAGE *) candidates.getInstalledOne();
	if (tmpRetPackage) *returnPackage = *tmpRetPackage;
	else {
		tmpRetPackage = getWillBeInstalled(candidates);
		if (tmpRetPackage) *returnPackage = *tmpRetPackage;
		else {
			candidates.initVersioning();
			*returnPackage = *candidates.getMaxVersion();
		}
	}
	return MPKGERROR_OK;
}

PACKAGE_LIST DependencyTracker::renderRemoveQueue(const PACKAGE_LIST& removeQueue) // Построение очереди на удаление
{
	//printf("%s: removeQueue size = %d\n", __func__, removeQueue->size());
	currentStatus=_("Checking dependencies: rendering remove queue");
	// removeQueue - user-composed remove queue
	// removeStream - result. Filtered.
	PACKAGE_LIST removeStream;
	PACKAGE_LIST tmp;
	removeStream.add(removeQueue);
	bool skipThis;
	for (unsigned int i=0; i<removeStream.size(); i++) {
		currentStatus=_("Checking dependencies: rendering remove queue") + (string) " (" + IntToStr(i) + "/" + IntToStr(removeStream.size())+")";
		tmp = get_dependant_packages(removeStream.at(i));
		for (unsigned int t=0; t<tmp.size(); t++) {
			skipThis=false;
			for (unsigned int c=0; c<removeStream.size(); c++) {
				if (!tmp[t].installed() || removeStream[c].get_id()==tmp[t].get_id()) {
					skipThis=true;
					break;
				}
			}
			if (!skipThis) {
				removeStream.add(tmp[t]);
			}
		}
	}
	return removeStream;
}

PACKAGE_LIST DependencyTracker::get_dependant_packages(const PACKAGE& package)
{
	if (!cacheCreated) { fillInstalledPackages(); }
	PACKAGE_LIST dependantPackages;
	bool updating;
	for (unsigned int i=0; i<installedPackages.size(); i++)
	{
		if (installedPackages[i].isItRequired(package))
		{
			updating=false;
			// Check if it can be replaced by any from install queue
			if (_tmpInstallStream != NULL)
			{
				for (unsigned int t=0; t<_tmpInstallStream->size(); t++)
				{
					if (package.get_name() == _tmpInstallStream->at(t).get_name() && installedPackages[i].isItRequired(_tmpInstallStream->at(t)))
					{
						updating = true;
						break;
					}
				}
			}
			if (!updating) dependantPackages.add(installedPackages[i]);
		}
	}
	// Setting appropriary actions
	for (unsigned int i=0; i<dependantPackages.size(); i++)
	{
		dependantPackages.get_package_ptr(i)->set_action(ST_REMOVE, "indirect-getdep");
	}
	return dependantPackages;
}

void DependencyTracker::fillByAction(int action, PACKAGE_LIST *p)
{
	if (!cacheCreated) createPackageCache();
	for (unsigned int i=0; i<packageCache.size(); i++)
	{
		if (packageCache[i].action()==action) p->add(packageCache[i]);
	}
}

void DependencyTracker::muxStreams(const PACKAGE_LIST& installStream, const PACKAGE_LIST& removeStream)
{
	PACKAGE_LIST install_list;
	PACKAGE_LIST remove_list;
	PACKAGE_LIST conflict_list;
	PACKAGE_LIST installQueuedList;
	PACKAGE_LIST removeQueuedList;

	fillByAction(ST_INSTALL, &installQueuedList);

#ifdef EXTRACHECK_REMOVE_QUEUE
	fillByAction(ST_REMOVE, &removeQueuedList);
	fillByAction(ST_PURGE, &removeQueuedList);
#endif
	bool found;
	// What we should do?
	// 1. Remove from removeStream all items who are in installStream.
	// 2. Add to remove_list resulting removeStream;

	mDebug("Stage 1: removing items from removeStream which is required by installStream");

	for (unsigned int i=0; i<removeStream.size(); ++i)
	{
		found=false;
		for (unsigned int t=0;t<installStream.size();++t)
		{
			if (installStream[t].equalTo(removeStream[i])) 
			{
				found=true;
				break;
			}
		}
		if (!found) remove_list.add(removeStream[i]);
	}

	mDebug("Add to remove_stream all conflicting packages");

	// 3. Add to remove_list all installed packages who conflicts with installStream (means have the same name and other md5). Also, filter conflict list
	PACKAGE_LIST proxyinstalledList = installedPackages;
       	proxyinstalledList.add(installQueuedList);
	found=false;
	for (size_t i=0; i<installStream.size(); ++i) {
		for (size_t t=0; t<proxyinstalledList.size(); ++t) {
			if (proxyinstalledList[t].installed() && \
					proxyinstalledList[t].get_corename() == installStream[i].get_corename() && \
					proxyinstalledList[t].get_md5() != installStream[i].get_md5()) {
				proxyinstalledList.get_package_ptr(t)->set_action(ST_REMOVE, "conflict");

				conflict_list.add(proxyinstalledList[t]);
				break;
			}
			else if (!installStream[i].get_conflicts().empty() && proxyinstalledList[t].get_corename()==installStream[i].get_conflicts()) {
				proxyinstalledList.get_package_ptr(t)->set_action(ST_REMOVE, "conflict");
				conflict_list.add(proxyinstalledList[t]);
				break;
			}
		}
		
	}
	mDebug("Filtering conflict list");
	// 3.1 Filter conflict_list. Search for packages who required anything in conflict_list and it cannot be replaced by anything in installStream.
	remove_list.add(conflict_list);
#ifdef BACKTRACE_DEPS
	PACKAGE_LIST removeCandidates;
	PACKAGE_LIST removeQueue2;
	PACKAGE_LIST willInstalled = installStream + installedPackages;
	removeCandidates = conflict_list;
	for (int i=0; i<conflict_list.size(); i++)
	{
		removeCandidates = get_dependant_packages(conflict_list.get_package(i));
		for (int t=0; t<removeCandidates.size(); t++)
		{
			if (checkBrokenDeps(removeCandidates.get_package(t), willInstalled)) removeQueue2.add(removeCandidates.get_package(t));
		}
	}
	removeQueue2 = renderRemoveQueue (&removeQueue2);
	remove_list += removeQueue2;
#endif
	mDebug("Putting install_list");
	
	// 4. Put in install_list all installStream
	install_list = installStream;
	
	// 5. Return results.
	installList = install_list;
	removeList = remove_list;

	mDebug("Done");
}

bool DependencyTracker::checkBrokenDeps(PACKAGE *pkg, PACKAGE_LIST searchList) // Tests if all items in searchList can be installed without pkg
{
	// Returns true if it is required by someone in searchList, or it is already in searchList himself
	// False if no package depends on it
	bool hasdependant;


	for (unsigned int t=0; t<searchList.size(); t++) {
		hasdependant=false;
		for (unsigned int i=0; i<searchList[t].get_dependencies().size(); i++) {
			if (searchList[t].get_dependencies().at(i).get_package_name() == pkg->get_corename() && \
					meetVersion(searchList[t].get_dependencies().at(i).get_version_data(), pkg->get_version()))
			{
				hasdependant=true;
				break;
			}
		}
		if (!hasdependant)
		{
			//printf("%s: All clean, we don't remove this\n",__func__);
			return false; // We can remove it
		}
	}
	
	//printf("%s: No packages to complain\n",__func__);
	return false; // No packages to complain - it's alone one :)
	
}


bool DependencyTracker::commitToDb()
{
	mDebug("Tracking and committing to database");
	int iC=0;
	vector<int> i_ids;
	bool alreadyThere;
	PACKAGE_LIST iList, rList;
	for (unsigned int i=0; i<installList.size(); i++)
	{
		if (!installList[i].installed())
		{
			alreadyThere=false;
			for (unsigned int v=0; v<i_ids.size(); v++)
			{
				if (i_ids[v]==installList[i].get_id())
				{
					alreadyThere=true;
					break;
				}
			}
			if (!alreadyThere)
			{
				iC++;
				iList.add(installList[i]);
				i_ids.push_back(installList[i].get_id());
			}
		}
	}
	installList = iList;
	int rC=0;
	vector<int> r_ids;

	bool essentialFound=false;
	for (unsigned int i=0; i<removeList.size(); i++)
	{
		if (removeList[i].configexist())
		{
			alreadyThere=false;
			for (unsigned int v=0; v<r_ids.size(); v++)
			{
				if (r_ids[v]==removeList[i].get_id()) {
					alreadyThere=true;
					break;
				}
			}
			if (!alreadyThere)
			{
				rC++;
				rList.add(removeList[i]);
				r_ids.push_back(removeList[i].get_id());
			}
		}
	}
	removeList = rList;

	bool attemptRemoveEssential;
	// Queue are fully formed, check for essentials
	for (unsigned int i=0; i<removeList.size(); i++)
	{
		if (removeList[i].isRemoveBlacklisted()) {
			// Checking if it is replaced by something...
			attemptRemoveEssential = true;
			for (unsigned int t=0; t<installList.size(); t++) {
				if (installList[t].get_corename() == removeList[i].get_corename()) {
					attemptRemoveEssential = false; // It's updating, all ok
					break;
				}
			}
			if (attemptRemoveEssential) {
				if (!force_essential_remove) {
					mError(_("Cannot remove package ") + \
						removeList[i].get_name() + \
						_(", because it is an important system component."));
					essentialFound = true;
				}
				else  mWarning(_("Removing essential package ") + \
						removeList[i].get_name());
			}
		}
	}

	if (essentialFound) {
		mError(_("Found essential packages, cannot continue"));
		return false;
	}
	
	// Commit actions to database if all OK
	for (unsigned int i=0; i<installList.size(); i++) db->set_action(installList[i].get_id(), ST_INSTALL, installList[i].package_action_reason);
	for (unsigned int i=0; i<removeList.size(); i++) db->set_action(removeList[i].get_id(), removeList[i].action(), removeList[i].package_action_reason);
	mDebug("finished");
	return true;
}

DependencyTracker::DependencyTracker(mpkgDatabase *mpkgDB)
{
	_tmpRemoveStream = NULL;
	_tmpInstallStream = NULL;
	cacheCreated=false;
	db=mpkgDB;
}
DependencyTracker::~DependencyTracker(){}

