/* Dependency tracking - header
$Id: dependencies.h,v 1.17 2007/11/23 01:01:46 i27249 Exp $
*/



#ifndef DEPENDENCIES_H_
#define DEPENDENCIES_H_
#include "mpkg.h"
#include "core.h"
#include "conditions.h"
//#include "mpkg.h"
#define DEP_OK 0
#define DEP_NOTFOUND 1
#define DEP_CONFLICT 2
#define DEP_VERSION 3
#define DEP_CHILD 4
#define DEP_BROKEN 5
#define DEP_UNAVAILABLE 6
#define DEP_DBERROR 7
#define DEP_FILECONFLICT 8

void filterDupes(PACKAGE_LIST *pkgList, bool removeEmpty=true);
void filterDupeNames(PACKAGE_LIST *pkgList);
class DependencyTracker
{
	private:
		PACKAGE_LIST installList;
		PACKAGE_LIST removeList;
		PACKAGE_LIST failure_list;

		PACKAGE_LIST installQueryList;
		PACKAGE_LIST removeQueryList;

		PACKAGE_LIST installedPackages;

		PACKAGE_LIST packageCache;

		void createPackageCache(bool only_if_queue_exists=false);
		void fillInstalledPackages();
		void fillByName(const string& name, PACKAGE_LIST *p, PACKAGE_LIST *testPackages=NULL);
		void fillByAction(int ACTION, PACKAGE_LIST *p);
		bool cacheCreated;
		mpkgDatabase *db;

		PACKAGE_LIST *_tmpInstallStream;
		PACKAGE_LIST *_tmpRemoveStream;
	public:
		bool check_deps(PACKAGE *package, PACKAGE_LIST *pList);
		int renderDependenciesInPackageList(PACKAGE_LIST *pkgList);
		void addToInstallQuery(const PACKAGE& pkg);
		void addToRemoveQuery(const PACKAGE& pkg);
		const PACKAGE_LIST& get_install_list() const;
		const PACKAGE_LIST& get_remove_list() const;
		const PACKAGE_LIST& get_failure_list() const;
		int renderData();	// Returns 0 if all ok, failure count if something fails (broken dependencies, etc)
		bool commitToDb();
		void reset(); // Clears all internal lists
		
		PACKAGE_LIST renderRequiredList(PACKAGE_LIST *installationQueue);
		PACKAGE_LIST get_required_packages(PACKAGE *package);
		int get_dep_package(DEPENDENCY *dep, PACKAGE *returnPackage, PACKAGE_LIST *pList=NULL);
		PACKAGE_LIST renderRemoveQueue(const PACKAGE_LIST& removeQueue);
		PACKAGE_LIST get_dependant_packages(const PACKAGE& package);
		void muxStreams(const PACKAGE_LIST& installStream, const PACKAGE_LIST& removeStream);
		bool checkBrokenDeps(PACKAGE *pkg, PACKAGE_LIST searchList);

		void setFakePackageCache(const PACKAGE_LIST &);

		DependencyTracker(mpkgDatabase *mpkgDB);
		~DependencyTracker();

};
#endif //DEPENDENCIES_H_

