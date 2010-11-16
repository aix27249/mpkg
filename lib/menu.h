#include "libmpkg.h"
void showMainMenu(mpkg &core);
void actUpdate(mpkg &core);
void actPackageMenu(mpkg &core);
void actUpdatesMenu(mpkg &core);
void actUpgrade(mpkg &core, int action);
void manageRepositories(mpkg &core);
void actClean(mpkg &core);
void actIndex(mpkg &core, string path, bool index_filelist);
void actSearch(mpkg &core);
void show_package_info(mpkg *core, string name, string version="", string build="", bool showFilelist=false, int id=-1);
void list_pkglist(const PACKAGE_LIST& pkglist);
void list(mpkg *core, const vector<string>& search, const bool showOnlyAvailable, const bool showOnlyInstalled, const bool onlyQueue=false);
void actConvert(string filename, string tmpdir);
void actInstallFromList(mpkg &core, string filename, bool includeVersions=false, bool enqueueOnly=false);
void searchByFile(mpkg *core, string filename, bool strict=false);
void actListDependants(mpkg &core, string filename, bool includeNotInstalled=false);
void actSearchByDescription(mpkg &core, const vector<string> &query, bool showOnlyInstalled=false, bool showOnlyAvailable=false);
void actGetRepositorylist(string url="");
