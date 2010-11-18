/********************************************************************************
 *
 * 			Central core for MPKG package system
 *			TODO: Should be reorganized to objects
 *	$Id: core.cpp,v 1.81 2007/12/10 03:12:58 i27249 Exp $
 *
 ********************************************************************************/

#include "core.h"
#include "debug.h"
#include "mpkg.h"
#include "terminal.h"

//------------------Library front-end--------------------------------
#define ENABLE_CONFLICT_CHECK
int mpkgDatabase::sql_exec(const string& query) {
	return db.sql_exec(query);
}
bool mpkgDatabase::checkVersion(const string& version1, const int& condition, const string& version2)
{
	//debug("checkVersion "+version1 + " vs " + version2);
	switch (condition)
	{
		case VER_MORE:
			if (strverscmp2(version1, version2)>0) return true;
			else return false;
			break;
		case VER_LESS:
			if (strverscmp2(version1, version2)<0) return true;
			else return false;
			break;
		case VER_EQUAL:
			if (strverscmp2(version1,version2)==0) return true;
			else return false;
			break;
		case VER_NOTEQUAL:
			if (strverscmp2(version1,version2)!=0) return true;
			else return false;
			break;
		case VER_XMORE:
			if (strverscmp2(version1,version2)>=0) return true;
			else return false;
			break;
		case VER_XLESS:
			if (strverscmp2(version1,version2)<=0) return true;
			else return false;
			break;
		default:
			mError("unknown condition " + IntToStr(condition));
			return true;
	}
	return true;
}

/*PACKAGE_LIST mpkgDatabase::get_other_versions(const string& package_name)
{
	PACKAGE_LIST pkgList;
	SQLRecord sqlSearch;
	sqlSearch.addField("package_name", package_name);
	get_packagelist(sqlSearch, &pkgList);
	return pkgList;
}*/	// Seems to be deprecated
PACKAGE* mpkgDatabase::get_max_version(PACKAGE_LIST& pkgList, const DEPENDENCY& dep)
{
	// Maybe optimization possible
	PACKAGE *candidate=NULL;
	string ver, build;
	for (unsigned int i=0; i<pkgList.size(); i++)
	{
		if (pkgList[i].reachable() \
				&& compareVersions(pkgList[i].get_version(), pkgList[i].get_build(), ver, build)>0 \
				/*&& strverscmp2(pkgList[i].get_version().c_str(), ver.c_str())>0 \*/
				&& mpkgDatabase::checkVersion(pkgList[i].get_version(), \
					atoi(dep.get_condition().c_str()), \
					dep.get_package_version())
		   )
		{
			candidate = pkgList.get_package_ptr(i);
			ver = pkgList[i].get_version();
			build = pkgList[i].get_build();
		}
	}
	if (candidate==NULL)
	{
		candidate = new PACKAGE;
		mError("max version not detected");
	}
	return candidate;
}

int mpkgDatabase::check_file_conflicts_new(const PACKAGE& package)
{
	if (verbose && !dialogMode) say(_("\nChecking file conflicts for package %s\n"), package.get_name().c_str());
	int package_id;
	SQLTable sqlTable;
	SQLRecord sqlFields;
	SQLRecord sqlSearch;
	sqlSearch.setSearchMode(SEARCH_IN);
	sqlFields.addField("packages_package_id");
	sqlFields.addField("file_name");
	if (package.get_files().size()==0) return 0; // If a package has no files, it cannot conflict =)
	for (unsigned int i=0;i<package.get_files().size(); ++i) {
		if (package.get_files().at(i).at(package.get_files().at(i).length()-1)!='/') {
			sqlSearch.addField("file_name", package.get_files().at(i));
		}
	}
	db.get_sql_vtable(sqlTable, sqlFields, "files", sqlSearch);
	int fPackages_package_id = sqlTable.getFieldIndex("packages_package_id");
	int fFile_name = sqlTable.getFieldIndex("file_name");
	vector<string *> fileNames;
	vector<int> package_ids;
	if (!sqlTable.empty()) {
		for (unsigned int k=0;k<sqlTable.getRecordCount() ;k++) {
			package_id=atoi(sqlTable.getValue(k, fPackages_package_id).c_str());
			if (package_id!=package.get_id()) {
				if (get_installed(package_id) || get_action(package_id)==ST_INSTALL) {
					fileNames.push_back(sqlTable.getValuePtr(k, fFile_name));
					package_ids.push_back(package_id);
				}
			}
		}
	}
	bool specialBackup = false;
	if (package.get_name()=="glibc" || package.get_name()=="glibc-solibs" || package.get_name()=="aaa_elflibs" || package.get_name()=="tar" || package.get_name()=="xz" || package.get_name()=="gzip") {
		specialBackup = true;
	}
	backupFiles(fileNames, package_ids, package.get_id(), specialBackup);
	return 0; // End of check_file_conflicts
}

bool mpkgDatabase::checkEssentialFile(const string& filename) {
	fillEssentialFiles();
	for (unsigned int i=0; i<essentialFiles.size(); ++i) {
		if (essentialFiles[i]==filename) return true;
	}
	return false;
	
}

void mpkgDatabase::fillEssentialFiles(bool force_update) {
	if (!essentialFiles.empty() && !force_update) return;
	//printf("Filling essential files...\n");
	SQLRecord sqlSearch;
	string pkgname = "aaa_elflibs";
	sqlSearch.addField("package_name", pkgname);
	sqlSearch.addField("package_installed", ST_INSTALLED);
	sqlSearch.setSearchMode(SEARCH_AND);
	PACKAGE_LIST p;
	get_packagelist(sqlSearch, &p, true);
	if (p.IsEmpty()) return;
	if (p.size()!=1) {
		printf("Multiple aaa_elflibs in database, fail!\n");
	}
	get_filelist(p[0].get_id(), &essentialFiles);
	//printf("Got %d essential files\n", essentialFiles.size());
}
bool mpkgDatabase::add_conflict_records(int conflicted_id, vector<int> overwritten_ids, vector<string *> file_names, PACKAGE_LIST& pkgList) {
	SQLRecord * sqlFill;
	SQLTable *sqlTable = new SQLTable;
	string tmp;
	PACKAGE *p;
	vector<int> bad_package_ids;
	bool bad_found;
	for (unsigned int i=0; i<overwritten_ids.size(); i++) {
		p=pkgList.getPackageByIDPtr(overwritten_ids[i], true);
		if (!p) {
			bad_found=false;
			for (unsigned int t=0; !bad_found && t<bad_package_ids.size(); ++t) {
				if (bad_package_ids[t]==overwritten_ids[i]) bad_found=true;
			}
			if (!bad_found) bad_package_ids.push_back(overwritten_ids[i]);
			continue;
		}

		sqlFill = new SQLRecord;
		sqlFill->addField("conflicted_package_id", conflicted_id);
		sqlFill->addField("conflict_file_name", *file_names[i]);
		tmp =p->get_name() + "_" + p->get_md5() + "/" + *file_names[i]; 
		sqlFill->addField("backup_file", tmp);
		sqlTable->addRecord(*sqlFill);
		delete sqlFill;
	}
	if (!bad_package_ids.empty()) {
		SQLRecord sqlDelete;
		sqlDelete.setSearchMode(SEARCH_IN);
		for (unsigned int i=0; i<bad_package_ids.size(); ++i) {
			sqlDelete.addField("packages_package_id", bad_package_ids[i]);
			printf("Clearing malformed file records for ID %d\n", bad_package_ids[i]);
		}
		db.sql_delete("files", sqlDelete);
	}
	int ret = db.sql_insert("conflicts", *sqlTable);
	delete sqlTable;
	return ret;
}

int mpkgDatabase::add_conflict_record(int conflicted_id, int overwritten_id, const string& file_name)
{
	PACKAGE pkg;
	get_package(overwritten_id, &pkg,true);

	SQLRecord sqlFill;
	sqlFill.addField("conflicted_package_id", conflicted_id);
	sqlFill.addField("conflict_file_name", file_name);
	string x = pkg.get_name() + "_" + pkg.get_md5() + "/" + file_name;
	sqlFill.addField("backup_file", x );
	return db.sql_insert("conflicts", sqlFill);
}

int mpkgDatabase::delete_conflict_record(int conflicted_id, const string& file_name)
{
	SQLRecord sqlFill;
	sqlFill.addField("conflicted_package_id", conflicted_id);
	sqlFill.addField("backup_file", file_name);
	return db.sql_delete("conflicts", sqlFill);
}

void mpkgDatabase::get_conflict_records(int conflicted_id, vector<string> *filenames, vector<FILE_EXTENDED_DATA> *ret)
{
	SQLRecord sqlSearch;
	sqlSearch.addField("conflicted_package_id", conflicted_id);
	SQLRecord sqlFields;
	sqlFields.addField("backup_file");
	sqlFields.addField("conflict_file_name");
	//sqlFields.addField("overwritten_package_id");
	SQLTable fTable;
	db.get_sql_vtable(fTable, sqlFields, "conflicts", sqlSearch);
	ret->clear();
	ret->resize(fTable.getRecordCount());
	filenames->clear();
	filenames->resize(fTable.getRecordCount());

	size_t fConflict_file_name = fTable.getFieldIndex("conflict_file_name");
	size_t fBackup_file = fTable.getFieldIndex("backup_file");
	for (size_t i=0; i<fTable.getRecordCount(); ++i) {
		filenames->at(i)=fTable.getValue(i, fConflict_file_name);
		ret->at(i).filename=&filenames->at(i);
		ret->at(i).backup_file=fTable.getValue(i, fBackup_file);
		ret->at(i).owner_id = -1;
		ret->at(i).overwriter_id = conflicted_id;
	}
}
void mpkgDatabase::get_backup_records(const PACKAGE& package, vector<string> *filenames, vector<FILE_EXTENDED_DATA> *ret)
{
	SQLRecord sqlSearch;
	sqlSearch.setEqMode(EQ_CUSTOMLIKE);
	string backupDir = package.get_name() + "_" + package.get_md5()+"%";
	sqlSearch.addField("backup_file", backupDir);
	SQLRecord sqlFields;
	sqlFields.addField("backup_file");
	sqlFields.addField("conflict_file_name");
	sqlFields.addField("conflicted_package_id");
	SQLTable fTable;
	db.get_sql_vtable(fTable, sqlFields, "conflicts", sqlSearch);
	ret->clear();
	ret->resize(fTable.getRecordCount());
	filenames->clear();
	filenames->resize(fTable.getRecordCount());


	size_t fConflict_file_name = fTable.getFieldIndex("conflict_file_name");
	size_t fBackup_file = fTable.getFieldIndex("backup_file");
	size_t fOverwriter_id = fTable.getFieldIndex("conflicted_package_id");
	for (size_t i=0; i<fTable.getRecordCount(); ++i) {
		filenames->at(i)=fTable.getValue(i, fConflict_file_name);
		ret->at(i).filename=&filenames->at(i);
		ret->at(i).backup_file=fTable.getValue(i, fBackup_file);
		ret->at(i).owner_id = package.get_id();
		ret->at(i).overwriter_id = atoi(fTable.getValue(i, fOverwriter_id).c_str());
	}
}
int mpkgDatabase::backupFiles(vector <string *> fileNames, vector<int> overwritten_package_ids, int conflicted_package_id, bool skipMove) {
	//if (dialogMode && fileNames.size()>0) ncProgressBar.SetText(_("Installing ") + IntToStr(conflicted_package_id) +_(": backing up ") + IntToStr(fileNames.size()) + _(" files"));

	bool hasErrors = false;
	SQLRecord *sqlSearch = new SQLRecord;
	PACKAGE_LIST * pkgList = new PACKAGE_LIST;
	vector<string *> fileNamesNew;
	vector<int> overwritten_package_ids_new;
	bool already = false;
	for (unsigned int i=0; i<fileNames.size(); i++) {
		already=false;
		if (FileExists(SYS_ROOT + *fileNames[i]) && !isDirectory(SYS_ROOT + *fileNames[i])) {
			for (unsigned int k=0; k<overwritten_package_ids_new.size(); k++) {
				if (overwritten_package_ids_new[k]==overwritten_package_ids[i]) {
					already=true;
					break;
				}
			}
			if (!already) {
				sqlSearch->addField("package_id", overwritten_package_ids[i]);
			}
			fileNamesNew.push_back(fileNames[i]);
			overwritten_package_ids_new.push_back(overwritten_package_ids[i]);
		}
	}
	sqlSearch->setSearchMode(SEARCH_IN);
	get_packagelist(*sqlSearch, pkgList, true);
	// Let's check received packagelist for missing packages
	add_conflict_records(conflicted_package_id, overwritten_package_ids_new, fileNamesNew, *pkgList);
	// Creating backup directories and backing up files
	string backupDir;
	string src, dest;
	vector<string> blacklist = ReadFileStrings("/etc/mpkg-unremovable");
	PACKAGE *p;
	for (unsigned int f=0; !skipMove && f<overwritten_package_ids_new.size(); f++) {
		p = (PACKAGE *) pkgList->getPackageByIDPtr(overwritten_package_ids_new[f], true);
		if (!p) continue; // WTFFAIL правда, но пусть.
		// Проверка на особые пакеты.
		// Первое: проверяем список mpkg-unremovable
		for (unsigned int z=0; z<blacklist.size(); ++z) {
			if (p->get_name()==blacklist[z]) continue;
		}
		if (p->get_name() == "aaa_elflibs") continue;
		// Второе: отдельная особая проверка для корневых библиотек aaa_elflibs:
		if (checkEssentialFile(*fileNamesNew[f])) continue; 
		
		// Если мы имеем дело с обычным пакетом, то можно работать.
		backupDir = SYS_BACKUP + p->get_name() + "_" + p->get_md5() + "/" + getDirectory(*fileNamesNew[f]);
		if (!isDirectory(backupDir)) system("mkdir -p '" + backupDir + "' 2>/dev/null >/dev/null");
	       	if (isDirectory(backupDir)) {
			src = SYS_ROOT + *fileNamesNew[f];
			dest = SYS_BACKUP + "/" + p->get_name() + "_" + p->get_md5() + "/" + *fileNamesNew[f];
			if (rename(src.c_str(), dest.c_str())) {
				if (errno==EXDEV && !system("mv " + src + " " + dest + "/ 2>/dev/null > /dev/null")) {
					hasErrors = true;
				}
			}
		}
		else {
			//mError("Error creating backup directory " + backupDir);
			hasErrors = true;
		}
	}
	
	
	delete sqlSearch;
	delete pkgList;
	return hasErrors;
}

int _cleanBackupCallback(const char *filename, const struct stat *file_status, int filetype)
{
	unsigned short x=0, y=0;

	if (file_status->st_ino!=0) x=y;
	if (filetype == FTW_D && strcmp(filename, SYS_BACKUP.c_str())!=0 && !simulate ) rmdir(filename);
	return 0;
}

void mpkgDatabase::clean_backup_directory()
{
	ftw(SYS_BACKUP.c_str(), _cleanBackupCallback, 20);
}

int mpkgDatabase::clean_package_filelist (PACKAGE *package)
{
	SQLRecord sqlSearch;
	sqlSearch.addField("packages_package_id", package->get_id());
	return db.sql_delete("files", sqlSearch);
}

#ifdef ENABLE_INTERNATIONAL
int mpkgDatabase::add_descriptionlist_record(int package_id, DESCRIPTION_LIST *desclist)
{
	SQLTable sqlTable;
	SQLRecord sqlValues;
	for (unsigned int i=0; i<desclist->size(); i++)
	{
		sqlValues.clear();
		sqlValues.addField("description_language", desclist->get_description(i)->get_language());
		sqlValues.addField("description_short_text", desclist->get_description(i)->get_shorttext());
		sqlValues.addField("description_text", desclist->get_description(i)->get_text());
		sqlValues.addField("packages_package_id", IntToStr(package_id));
		sqlTable.addRecord(sqlValues);
	}
	if (!sqlTable.empty())
	{
		return db.sql_insert("descriptions", &sqlTable);
	}
	else
	{
		 return 0;
	}

}
#endif
// Adds file list linked to package_id (usually for package adding)
int mpkgDatabase::add_filelist_record(int package_id, vector<string> *filelist)
{
	SQLTable *sqlTable = new SQLTable;
	SQLRecord sqlValues;
	for (unsigned int i=0;i<filelist->size();i++)
	{
		sqlValues.clear();
		sqlValues.addField("file_name", filelist->at(i));
		sqlValues.addField("packages_package_id", package_id);
		sqlValues.addField("file_type", 0);
		sqlTable->addRecord(sqlValues);
	}
	if (!sqlTable->empty())
	{
		int ret = db.sql_insert("files", *sqlTable);
		delete sqlTable;
		return ret;
	}
	else
	{
		delete sqlTable;
		return 0;
	}
}
int mpkgDatabase::add_abuild_record(int package_id, const string& abuild_url) {
	string a_url = abuild_url;
	PrepareSql(a_url);
	db.sql_exec("INSERT INTO abuilds VALUES(NULL," + IntToStr(package_id) + ",'" + a_url + "');");
	return 0;
}
// Adds location list linked to package_id
int mpkgDatabase::add_locationlist_record(int package_id, vector<LOCATION> *locationlist) // returns 0 if ok, anything else if failed.
{
	SQLTable *sqlLocations = new SQLTable;
	SQLRecord sqlLocation;
	for (unsigned int i=0;i<locationlist->size();i++)
	{
		sqlLocation.clear();
		sqlLocation.addField("server_url", locationlist->at(i).get_server_url());
		sqlLocation.addField("packages_package_id", package_id);
		sqlLocation.addField("location_path", locationlist->at(i).get_path());
		sqlLocations->addRecord(sqlLocation);
		
	}
	int ret=1;
	if (!sqlLocations->empty()) ret=db.sql_insert("locations", *sqlLocations);
	//printf("LOCATIONS ret: %d\n", ret);
	delete sqlLocations;
	return ret;
}
int mpkgDatabase::add_configfiles_record(const int package_id, const vector<ConfigFile>& config_files) {
	int64_t config_id;
	string name, val;
	for (size_t i=0; i<config_files.size(); ++i) {
		db.sql_exec("INSERT INTO config_files VALUES (NULL, " + IntToStr(package_id) + ", '" + config_files[i].name + "');");
		config_id = db.last_insert_id();
		for (size_t a=0; a<config_files[i].attr.size(); ++a) {
			name = config_files[i].attr[a].name;
			PrepareSql(name);
			val = config_files[i].attr[a].value;
			PrepareSql(val);
			db.sql_exec("INSERT INTO config_options VALUES(NULL, " + IntToStr(config_id) + ", '" + name + "', '" + val + "');");
		}
	}


	return 0;
}
int mpkgDatabase::add_delta_record(const int package_id, const vector<DeltaSource>& deltaSources) // returns 0 if ok, anything else if failed.
{
	SQLTable *sqlDeltas = new SQLTable;
	SQLRecord sqlDelta;
	for (unsigned int i=0;i<deltaSources.size();i++)
	{
		sqlDelta.clear();
		sqlDelta.addField("packages_package_id", package_id);
		sqlDelta.addField("delta_url", deltaSources[i].dup_url);
		sqlDelta.addField("delta_md5", deltaSources[i].dup_md5);
		sqlDelta.addField("delta_orig_filename", deltaSources[i].orig_filename);
		sqlDelta.addField("delta_orig_md5", deltaSources[i].orig_md5);
		sqlDelta.addField("delta_size", deltaSources[i].dup_size);
		sqlDeltas->addRecord(sqlDelta);
		
	}
	int ret=1;
	if (!sqlDeltas->empty()) ret=db.sql_insert("deltas", *sqlDeltas);
	delete sqlDeltas;
	return ret;
}

// Adds dependency list linked to package_id
int mpkgDatabase::add_dependencylist_record(int package_id, vector<DEPENDENCY> *deplist)
{
	SQLTable *sqlTable = new SQLTable;
	SQLRecord sqlInsert;
	for (unsigned int i=0;i<deplist->size();i++)
	{
		sqlInsert.clear();
		sqlInsert.addField("packages_package_id", package_id);
		sqlInsert.addField("dependency_condition",deplist->at(i).get_condition() );
		sqlInsert.addField("dependency_type", deplist->at(i).get_type());
		sqlInsert.addField("dependency_package_name", deplist->at(i).get_package_name());
		sqlInsert.addField("dependency_package_version", deplist->at(i).get_package_version());
		sqlInsert.addField("dependency_build_only", deplist->at(i).isBuildOnly());
		sqlTable->addRecord(sqlInsert);
	}
	int ret = db.sql_insert("dependencies", *sqlTable);
	delete sqlTable;
	return ret;
}

// Adds tag list linked to package_id. It checks existance of tag in tags table, and creates if not. Next, it calls add_tag_link() to link package and tags
int mpkgDatabase::add_taglist_record (int package_id, vector<string> *taglist)
{
	int tag_id=0;
	SQLRecord sqlInsert;
	SQLTable *sqlTable = new SQLTable;
	SQLRecord sqlSearch;
	SQLRecord sqlFields;
	sqlFields.addField("tags_id");
	int fTags_id = 0; //sqlTable->getFieldIndex("tags_id"); // WARNING: if you change the field numbering, change this too!!!
	for (unsigned int i=0; i<taglist->size();i++)
	{
		sqlSearch.clear();
		sqlSearch.addField("tags_name", taglist->at(i)); 
		db.get_sql_vtable(*sqlTable, sqlFields, "tags", sqlSearch);

		// If tag doesn't exist in database - adding it. We make it by one query per tag, because tag count is wery small
		if (sqlTable->empty())
		{
			sqlInsert.clear();
			sqlInsert.addField("tags_name", taglist->at(i));
			db.sql_insert("tags", sqlInsert);
			db.get_sql_vtable(*sqlTable, sqlFields, "tags", sqlSearch);
		}
		tag_id=atoi(sqlTable->getValue(0, fTags_id).c_str());
		add_tag_link(package_id, tag_id);
	}
	delete sqlTable;
	return 0;
}

// Creates a link between package_id and tag_id
int mpkgDatabase::add_tag_link(int package_id, int tag_id)
{
	SQLRecord sqlInsert;
	sqlInsert.addField("packages_package_id", package_id);
	sqlInsert.addField("tags_tag_id", tag_id);
	int ret = db.sql_insert("tags_links", sqlInsert);
	return ret;
}



// Adds package - full structure (including files, locations, deps, and tags), returning package_id
int mpkgDatabase::add_package_record (PACKAGE *package)
{
	// INSERT INTO PACKAGES
	SQLRecord sqlInsert;
	
	sqlInsert.addField("package_name", package->get_name());
	sqlInsert.addField("package_version", package->get_version());
	sqlInsert.addField("package_arch", package->get_arch());
	sqlInsert.addField("package_build", package->get_build());
	sqlInsert.addField("package_compressed_size", package->get_compressed_size());
	sqlInsert.addField("package_installed_size", package->get_installed_size());
	sqlInsert.addField("package_short_description", package->get_short_description());
	sqlInsert.addField("package_description", package->get_description());
	sqlInsert.addField("package_changelog", package->get_changelog());
	sqlInsert.addField("package_packager", package->get_packager());
	sqlInsert.addField("package_packager_email", package->get_packager_email());
	sqlInsert.addField("package_installed", package->installed());
	sqlInsert.addField("package_configexist", package->configexist());
	sqlInsert.addField("package_action", package->action());

	sqlInsert.addField("package_md5", package->get_md5());
	sqlInsert.addField("package_filename", package->get_filename());
	sqlInsert.addField("package_betarelease", package->get_betarelease());
	//sqlInsert.addField("package_installed_by_dependency", package->get_installed_by_dependency());
	sqlInsert.addField("package_installed_by_dependency", package->package_action_reason);
	sqlInsert.addField("package_type", package->get_type());
	sqlInsert.addField("package_add_date", (int) time(NULL));
	sqlInsert.addField("package_build_date", package->build_date);
	sqlInsert.addField("package_repository_tags", package->get_repository_tags());
	sqlInsert.addField("package_distro_version", package->package_distro_version);
	sqlInsert.addField("package_provides", package->get_provides());
	sqlInsert.addField("package_conflicts", package->get_conflicts());

	db.sql_insert("packages", sqlInsert);
	
	// Retrieving package ID
	int package_id=db.getLastID();//get_last_id("packages", "package_id");
	package->set_id(package_id);
	if (package_id==0) exit(-100);
	
	// INSERT INTO FILES
	if (!package->get_files().empty()) add_filelist_record(package_id, package->get_files_ptr());
	
	// INSERT INTO LOCATIONS
	if (!package->get_locations().empty()) add_locationlist_record(package_id, package->get_locations_ptr());

	//INSERT INTO DEPENDENCIES
	if (!package->get_dependencies().empty()) add_dependencylist_record(package_id, package->get_dependencies_ptr());

	// INSERT INTO TAGS
	if (!package->get_tags().empty()) add_taglist_record(package_id, package->get_tags_ptr());

	// INSERT INTO DELTAS
	if (!package->deltaSources.empty()) add_delta_record(package_id, package->deltaSources);

	// INSERT INTO CONFIG_FILES AND CONFIG_OPTIONS
	if (!package->config_files.empty()) add_configfiles_record(package_id, package->config_files);

	// INSERT INTO ABUILDS
	if (!package->abuild_url.empty()) add_abuild_record(package_id, package->abuild_url);

	
#ifdef ENABLE_INTERNATIONAL
	// INSERT INTO DESCRIPTIONS
	add_descriptionlist_record(package_id, package->get_descriptions());
#endif
	return package_id;
}	



//--------------------Mid-level functions-------------------------

void mpkgDatabase::createDBCache()
{
	if (db.internalDataChanged)
	{
		SQLRecord sqlSearch;
		get_packagelist(sqlSearch, &packageDBCache);
		db.internalDataChanged=false;
	}
}

int mpkgDatabase::get_package(const int& package_id, PACKAGE *package, bool no_cache)//, bool GetExtraInfo)
{
	if (no_cache && db.internalDataChanged)
	{
		SQLRecord sqlSearch;
		PACKAGE_LIST pkgList;
		sqlSearch.addField("package_id", package_id);
		get_packagelist(sqlSearch, &pkgList);
		if (pkgList.size()==1) {
			*package = pkgList[0];
			return 0;
		}
		else return MPKGERROR_NOPACKAGE;

	}
	createDBCache();
	for (unsigned int i=0; i<packageDBCache.size(); i++)
	{
		if (packageDBCache[i].get_id()==package_id)
		{
			*package=packageDBCache[i];
			return 0;
		}
	}
	return MPKGERROR_NOPACKAGE;
}

/*int mpkgDatabase::get_fast_packagelist (string query, PACKAGE_LIST *packagelist, bool needTags, bool needDeps, bool needFilelist, bool needAbuilds, bool needConfigs) {
	
}*/
int gpkglist_counter=0, e_gpkglist_counter=0, gpkg_chit=0;
int mpkgDatabase::get_packagelist (const SQLRecord& sqlSearch, PACKAGE_LIST *packagelist, bool ultraFast, bool needDescriptions)//, bool GetExtraInfo, bool ultraFast)
{
	gpkglist_counter++;
	//printf("\n\nCOUNT: %d, fast: %d\n", gpkglist_counter, ultraFast);
	// Ultrafast will request only basic info about package
	
	if (sqlSearch.empty() && !db.internalDataChanged)
	{
		gpkg_chit++;
		//printf("Cache hit: %d\n", gpkg_chit);
		*packagelist=packageDBCache;
		return 0;
	}

	SQLTable *sqlTable = new SQLTable;
	SQLRecord sqlFields;
	PACKAGE package;

	int sql_ret=db.get_sql_vtable(*sqlTable, sqlFields, string("packages"), sqlSearch);
	if (sql_ret!=0)
	{
		return MPKGERROR_SQLQUERYERROR;
	}
	packagelist->clear(sqlTable->getRecordCount());
	
	// Creating field index
	int fPackage_id = sqlTable->getFieldIndex("package_id");
	int fPackage_name = sqlTable->getFieldIndex("package_name");
	int fPackage_version = sqlTable->getFieldIndex("package_version");
	int fPackage_arch = sqlTable->getFieldIndex("package_arch");
	int fPackage_build = sqlTable->getFieldIndex("package_build");
	int fPackage_compressed_size = sqlTable->getFieldIndex("package_compressed_size");
	int fPackage_installed_size = sqlTable->getFieldIndex("package_installed_size");
	int fPackage_short_description = sqlTable->getFieldIndex("package_short_description");
	int fPackage_description = sqlTable->getFieldIndex("package_description");
	int fPackage_changelog = sqlTable->getFieldIndex("package_changelog");
	int fPackage_packager = sqlTable->getFieldIndex("package_packager");
	int fPackage_packager_email = sqlTable->getFieldIndex("package_packager_email");
	int fPackage_installed = sqlTable->getFieldIndex("package_installed");
	int fPackage_configexist = sqlTable->getFieldIndex("package_configexist");
	int fPackage_action = sqlTable->getFieldIndex("package_action");
	int fPackage_md5 = sqlTable->getFieldIndex("package_md5");
	int fPackage_filename = sqlTable->getFieldIndex("package_filename");
	//int fPackage_betarelease =sqlTable->getFieldIndex("package_betarelease");
	int fPackage_installed_by_dependency = sqlTable->getFieldIndex("package_installed_by_dependency");
	int fPackage_type = sqlTable->getFieldIndex("package_type");
	int fPackage_add_date = sqlTable->getFieldIndex("package_add_date");
	int fPackage_build_date = sqlTable->getFieldIndex("package_build_date");
	
	int fPackage_repository_tags = sqlTable->getFieldIndex("package_repository_tags");
	int fPackage_distro_version = sqlTable->getFieldIndex("package_distro_version");
	int fPackage_provides = sqlTable->getFieldIndex("package_provides");
	int fPackage_conflicts = sqlTable->getFieldIndex("package_conflicts");

	PACKAGE *p;
	for (unsigned int i=0; i<sqlTable->getRecordCount(); ++i) {
		p = packagelist->get_package_ptr(i);
		p->set_id(atoi(sqlTable->getValue(i, fPackage_id).c_str()));
		p->set_name(sqlTable->getValue(i, fPackage_name));
		p->set_version(sqlTable->getValue(i, fPackage_version));
		p->set_arch(sqlTable->getValue(i, fPackage_arch));
		p->set_build(sqlTable->getValue(i, fPackage_build));
		p->set_compressed_size(sqlTable->getValue(i, fPackage_compressed_size));
		p->set_installed_size(sqlTable->getValue(i, fPackage_installed_size));
		if (needDescriptions) p->set_short_description(sqlTable->getValue(i, fPackage_short_description));
		if (needDescriptions) p->set_description(sqlTable->getValue(i, fPackage_description));
		if (needDescriptions) p->set_changelog(sqlTable->getValue(i, fPackage_changelog));
		if (needDescriptions) p->set_packager(sqlTable->getValue(i, fPackage_packager));
		if (needDescriptions) p->set_packager_email(sqlTable->getValue(i, fPackage_packager_email));
		p->set_installed(atoi(sqlTable->getValue(i,fPackage_installed).c_str()));
		p->set_configexist(atoi(sqlTable->getValue(i, fPackage_configexist).c_str()));
		p->set_action(atoi(sqlTable->getValue(i, fPackage_action).c_str()), sqlTable->getValue(i, fPackage_installed_by_dependency));

		p->set_md5(sqlTable->getValue(i, fPackage_md5));
		p->set_filename(sqlTable->getValue(i, fPackage_filename));
		//p->set_betarelease(sqlTable->getValue(i, fPackage_betarelease));
		p->set_installed_by_dependency(atoi(sqlTable->getValue(i, fPackage_installed_by_dependency).c_str()));
		p->set_type(atoi(sqlTable->getValue(i, fPackage_type).c_str()));
		if (needDescriptions) p->add_date = atoi(sqlTable->getValue(i, fPackage_add_date).c_str());
		if (needDescriptions) p->build_date = atoi(sqlTable->getValue(i, fPackage_build_date).c_str());
		if (needDescriptions) p->set_repository_tags(sqlTable->getValue(i, fPackage_repository_tags));
		if (needDescriptions) p->package_distro_version = sqlTable->getValue(i, fPackage_distro_version);
		p->set_provides(sqlTable->getValue(i, fPackage_provides));
		p->set_conflicts(sqlTable->getValue(i, fPackage_conflicts));
#ifdef ENABLE_INTERNATIONAL
		get_descriptionlist(p->get_id(), packagelist[i].get_descriptions());
#endif
	}
	if (!ultraFast && !packagelist->IsEmpty()) 
	{
		get_full_taglist(packagelist);
		get_full_dependencylist(packagelist);
		get_full_config_files_list(packagelist);
		if (needDescriptions) get_full_abuildlist(packagelist);
	}
	if (!packagelist->IsEmpty()) get_full_locationlist(packagelist);
	//get_full_deltalist(packagelist);
	if (!ultraFast && sqlSearch.empty())
	{
		packageDBCache=*packagelist;
		db.internalDataChanged=false;
	}

	delete sqlTable;
	e_gpkglist_counter++;
	//printf("\n\nEND COUNT: %d\n", e_gpkglist_counter);
	return 0;

}
#ifdef ENABLE_INTERNATIONAL
int mpkgDatabase::get_descriptionlist(int package_id, DESCRIPTION_LIST *desclist, string language)
{
	SQLTable sqlTable;
	SQLRecord sqlFields;
	sqlFields.addField("description_language");
	sqlFields.addField("short_description_text");
	sqlFields.addField("description_text");
	SQLRecord sqlSearch;
	sqlSearch.addField("packages_package_id", IntToStr(package_id));
	if (!language.empty()) sqlSearch.addField("description_language", language);
	
	DESCRIPTION desc;
	
	int sql_ret=db.get_sql_vtable(&sqlTable, sqlFields, "descriptions", sqlSearch);
	if (sql_ret!=0)
	{
		return -1;
	}
	
	for (int row=0; row<sqlTable.getRecordCount(); row++)
	{
		desc.set_language(sqlTable.getValue(row, "description_language"));
		desc.set_text(sqlTable.getValue(row, "description_text"));
		desc.set_shorttext(sqlTable.getValue(row, "short_description_text"));
		desclist->add(desc);
		desc.clear();
	}
	return 0;
}
#endif

int mpkgDatabase::get_filelist(const int& package_id, vector<string> *filelist)
{
	SQLTable *sqlTable = new SQLTable;
	SQLRecord sqlFields;
	sqlFields.addField("file_name");
	SQLRecord sqlSearch;
	sqlSearch.addField("packages_package_id", package_id);

	db.get_sql_vtable(*sqlTable, sqlFields, "files", sqlSearch);
	filelist->clear();
	filelist->resize(sqlTable->getRecordCount());
	size_t fFile_name = sqlTable->getFieldIndex("file_name");
	for (size_t row=0; row<sqlTable->getRecordCount(); ++row) {
		filelist->at(row)=sqlTable->getValue(row, fFile_name);
	}
	delete sqlTable;
	return 0;
}
int mpkgDatabase::get_sql_vtable(SQLTable& output, const SQLRecord& fields, const string& table_name, const SQLRecord& search)
{
	return db.get_sql_vtable(output, fields, table_name, search);
}

void mpkgDatabase::get_full_filelist(PACKAGE_LIST *pkgList)
{
	if (pkgList->IsEmpty()) return;
	SQLTable *sqlTable = new SQLTable;
	SQLRecord sqlSearch;
	SQLRecord sqlFields;
	sqlFields.addField("file_name");
	sqlFields.addField("packages_package_id");
	sqlSearch.setSearchMode(SEARCH_IN);
	for (size_t i=0; i<pkgList->size(); ++i) {
		sqlSearch.addField("packages_package_id", pkgList->at(i).get_id());
	}

	// Create pkg-id map
	map<int, size_t> pkgmap;
	for (size_t i=0; i<pkgList->size(); ++i) {
		pkgmap[pkgList->at(i).get_id()]=i;
	}
	size_t pkgnum;


	db.get_sql_vtable(*sqlTable, sqlFields, "files", sqlSearch);
	int package_id;
	size_t fPackages_package_id=sqlTable->getFieldIndex("packages_package_id");
	size_t fFile_name=sqlTable->getFieldIndex("file_name");
	for (size_t i=0; i<sqlTable->size(); ++i) {
		package_id=atoi(sqlTable->getValue(i, fPackages_package_id).c_str());
		pkgnum = pkgmap[package_id];
		if (pkgnum >= pkgList->size()) {
			continue;
		}

		pkgList->get_package_ptr(pkgnum)->get_files_ptr()->push_back(sqlTable->getValue(i, fFile_name));
	}
	delete sqlTable;
}

void mpkgDatabase::get_full_abuildlist(PACKAGE_LIST *pkgList) {
	if (pkgList->IsEmpty()) return;
	SQLTable *abuilds = new SQLTable;
	SQLRecord fields;
	SQLRecord search;

	db.get_sql_vtable(*abuilds, fields, "abuilds", search);
	
	int fPackage_id = abuilds->getFieldIndex("package_id");
	int fURL = abuilds->getFieldIndex("url");

	int package_id;
	for (size_t i=0; i<abuilds->size(); ++i) {
		package_id = atoi(abuilds->getValue(i, fPackage_id).c_str());
		for (size_t p=0; p<pkgList->size(); ++p) {
			if (package_id!=pkgList->at(p).get_id()) continue;
			pkgList->get_package_ptr(p)->abuild_url = abuilds->getValue(i, fURL);
		}
	}
}
void mpkgDatabase::get_full_config_files_list(PACKAGE_LIST *pkgList) {
	if (pkgList->IsEmpty()) return;
	SQLTable *configs = new SQLTable;
	SQLTable *c_options = new SQLTable;
	SQLRecord fields;
	SQLRecord search;
	db.get_sql_vtable(*configs, fields, "config_files", search);
	
	int fConfig_id = configs->getFieldIndex("id");
	int fPackage_id = configs->getFieldIndex("package_id");
	int fFilename = configs->getFieldIndex("filename");

	db.get_sql_vtable(*c_options, fields, "config_options", search);

	int fAttrConfig_id = c_options->getFieldIndex("config_files_id");
	int fAttrName = c_options->getFieldIndex("name");
	int fAttrValue = c_options->getFieldIndex("value");
	int config_id, package_id;
	ConfigFile *cfile = NULL;
	for (size_t i=0; i<configs->size(); ++i) {
		config_id = atoi(configs->getValue(i, fConfig_id).c_str());
		package_id = atoi(configs->getValue(i, fPackage_id).c_str());
		for (size_t p=0; p<pkgList->size(); ++p) {
			if (package_id!=pkgList->at(p).get_id()) continue;
			cfile = new ConfigFile;
			cfile->name = configs->getValue(i, fFilename);
			for (size_t a=0; a<c_options->size(); ++a) {
				if (config_id!=atoi(c_options->getValue(a, fAttrConfig_id).c_str())) continue;
				cfile->addAttribute(c_options->getValue(a, fAttrName), c_options->getValue(a, fAttrValue));
			}
			pkgList->get_package_ptr(p)->config_files.push_back(*cfile);
			delete cfile;
		}
	}
	delete configs;
	delete c_options;

}


void mpkgDatabase::get_full_dependencylist(PACKAGE_LIST *pkgList) //TODO: incomplete
{
	if (pkgList->IsEmpty()) return;
	SQLRecord fields;
	SQLRecord search;
	SQLTable deplist;
	
	search.setSearchMode(SEARCH_IN);
	for (size_t i=0; i<pkgList->size(); ++i) {
		search.addField("packages_package_id", pkgList->at(i).get_id());
	}

	db.get_sql_vtable(deplist, fields, "dependencies", search); // Emerging the list of all existing dependencies
	string pid_str;
	DEPENDENCY dep_tmp;

	// Creating index
	int fPackages_package_id = deplist.getFieldIndex("packages_package_id");
	int fDependency_condition = deplist.getFieldIndex("dependency_condition");
	int fDependency_package_name = deplist.getFieldIndex("dependency_package_name");
	int fDependency_type = deplist.getFieldIndex("dependency_type");
	int fDependency_package_version = deplist.getFieldIndex("dependency_package_version");
	int fDependency_build_only = deplist.getFieldIndex("dependency_build_only");
	
	// Create pkg-id map
	map<int, size_t> pkgmap;
	for (size_t i=0; i<pkgList->size(); ++i) {
		pkgmap[pkgList->at(i).get_id()]=i;
	}

	// Processing
	int currentDepID;
	size_t dsize, pkgnum;
	DEPENDENCY *dep;
	for (size_t i=0; i<deplist.size(); ++i) {
		currentDepID = atoi(deplist.getValue(i, fPackages_package_id).c_str());
		pkgnum = pkgmap[currentDepID];
		if (pkgnum >= pkgList->size()) {
			continue;
		}
		pkgList->get_package_ptr(pkgnum)->get_dependencies_ptr()->push_back(dep_tmp);
		dsize = pkgList->get_package_ptr(pkgnum)->get_dependencies().size();
		dep = &pkgList->get_package_ptr(pkgnum)->get_dependencies_ptr()->at(dsize-1);
		dep->set_condition(deplist.getValue(i, fDependency_condition));
		dep->set_type(deplist.getValue(i, fDependency_type));
		dep->set_package_name(deplist.getValue(i, fDependency_package_name));
		dep->set_package_version(deplist.getValue(i, fDependency_package_version));
		dep->setBuildOnly(atoi(deplist.getValue(i, fDependency_build_only).c_str()));
	}
}

void mpkgDatabase::get_full_taglist(PACKAGE_LIST *pkgList)
{
	if (pkgList->IsEmpty()) return;
	SQLRecord fields;
	SQLRecord search;
	SQLTable tags;
	SQLTable links;

	db.get_sql_vtable(tags, fields, "tags", search);
	
	fields.addField("packages_package_id");
	fields.addField("tags_tag_id");

	search.setSearchMode(SEARCH_IN);
	for (size_t i=0; i<pkgList->size(); ++i) {
		search.addField("packages_package_id", pkgList->at(i).get_id());
	}

	db.get_sql_vtable(links, fields, "tags_links", search);
	
	// Index
	int fLinksPackages_package_id = links.getFieldIndex("packages_package_id");
	int fLinksTags_tag_id = links.getFieldIndex("tags_tag_id");

	int fTagsTags_id = tags.getFieldIndex("tags_id");
	int fTagsTags_name = tags.getFieldIndex("tags_name");

	// Create pkg-id map
	map<int, size_t> pkgmap;
	for (size_t i=0; i<pkgList->size(); ++i) {
		pkgmap[pkgList->at(i).get_id()]=i;
	}

	// Create tag-id map
	map<int, size_t> tagmap;
	for (size_t i=0; i<tags.size(); ++i) {
		tagmap[atoi(tags.getValue(i, fTagsTags_id).c_str())]=i;
	}

	
	int currentLinkPackageID;
	size_t pkgnum, tagnum;
	
	for (size_t i=0; i<links.size(); ++i) {
		currentLinkPackageID = atoi(links.getValue(i, fLinksPackages_package_id).c_str());
		pkgnum = pkgmap[currentLinkPackageID];
		if (pkgnum >= pkgList->size()) {
			continue;
		}
		tagnum = tagmap[atoi(links.getValue(i, fLinksTags_tag_id).c_str())];
		if (tagnum >= tags.size()) {
			continue;
		}

		pkgList->get_package_ptr(pkgnum)->get_tags_ptr()->push_back(tags.getValue(tagnum, fTagsTags_name));
	}
}

void mpkgDatabase::get_available_tags(vector<string> *output)
{
	SQLTable sqlTable;
	SQLRecord sqlSearch;
	SQLRecord sqlFields;
	sqlFields.addField("tags_name");
	db.get_sql_vtable(sqlTable, sqlFields, "tags", sqlSearch);
	output->clear();
	output->resize(sqlTable.size());
	int fTags_name = sqlTable.getFieldIndex("tags_name");
	for (unsigned int i=0; i<sqlTable.size(); i++)
	{
		output->at(i)=sqlTable.getValue(i,fTags_name);
	}
}

int mpkgDatabase::get_locationlist(int package_id, vector<LOCATION> *location_list)
{
	SQLTable *sqlTable = new SQLTable;
	SQLRecord sqlFields;
	sqlFields.addField("location_id");
	sqlFields.addField("server_url");
	sqlFields.addField("location_path");
	SQLRecord sqlSearch;
	sqlSearch.addField("packages_package_id", package_id);
	db.get_sql_vtable(*sqlTable, sqlFields, "locations", sqlSearch);
	location_list->clear();
	location_list->resize(sqlTable->getRecordCount());
	unsigned int fLocation_id=sqlTable->getFieldIndex("location_id");
	unsigned int fServer_url=sqlTable->getFieldIndex("server_url");
	unsigned int fLocation_path=sqlTable->getFieldIndex("location_path");
	for (unsigned int i=0; i<sqlTable->getRecordCount(); i++)
	{
		location_list->at(i).set_id(atoi(sqlTable->getValue(i, fLocation_id).c_str()));
		location_list->at(i).set_server_url(sqlTable->getValue(i, fServer_url));
		location_list->at(i).set_path(sqlTable->getValue(i, fLocation_path));
	}
	delete sqlTable;
	return 0;
}
void mpkgDatabase::get_full_deltalist(PACKAGE_LIST *pkgList)
{
	SQLTable *sqlTable = new SQLTable;
	SQLRecord sqlFields;
	SQLRecord sqlSearch;
	
	sqlFields.addField("delta_id");
	sqlFields.addField("packages_package_id");
	sqlFields.addField("delta_url");
	sqlFields.addField("delta_md5");
	sqlFields.addField("delta_orig_filename");
	sqlFields.addField("delta_orig_md5");
	sqlFields.addField("delta_size");


	db.get_sql_vtable(*sqlTable, sqlFields, "deltas", sqlSearch);

	int package_id;

	// Index
	int fPackages_package_id = sqlTable->getFieldIndex("packages_package_id");
//	int fDelta_id = sqlTable->getFieldIndex("delta_id");
	int fDelta_url = sqlTable->getFieldIndex("delta_url");
	int fDelta_md5 = sqlTable->getFieldIndex("delta_md5");
	int fOrig_filename = sqlTable->getFieldIndex("delta_orig_filename");
	int fOrig_md5 = sqlTable->getFieldIndex("delta_orig_md5");
	int fDelta_size = sqlTable->getFieldIndex("delta_size");

	for (unsigned int i=0; i<sqlTable->size(); ++i) {
		package_id = atoi(sqlTable->getValue(i, fPackages_package_id).c_str());
		for (unsigned int t=0; t<pkgList->size(); ++t) {
			if (pkgList->get_package_ptr(t)->get_id()==package_id) {
				pkgList->get_package_ptr(t)->deltaSources.push_back(DeltaSource(sqlTable->getValue(i, fDelta_url), sqlTable->getValue(i, fDelta_md5), sqlTable->getValue(i, fOrig_filename), sqlTable->getValue(i, fOrig_md5), sqlTable->getValue(i, fDelta_size)));
			}
		}
	}
	delete sqlTable;
}

void mpkgDatabase::get_full_locationlist(PACKAGE_LIST *pkgList)
{
	if (pkgList->IsEmpty()) return;
	SQLTable *sqlTable = new SQLTable;
	SQLRecord sqlFields;
	SQLRecord sqlSearch;
	sqlFields.addField("location_path");
	sqlFields.addField("location_id");
	sqlFields.addField("server_url");
	sqlFields.addField("packages_package_id");

	sqlSearch.setSearchMode(SEARCH_IN);
	for (size_t i=0; i<pkgList->size(); ++i) {
		sqlSearch.addField("packages_package_id", pkgList->at(i).get_id());
	}

	db.get_sql_vtable(*sqlTable, sqlFields, "locations", sqlSearch);

	int package_id;
	LOCATION tmp;

	// Index
	int fPackages_package_id = sqlTable->getFieldIndex("packages_package_id");
	int fLocation_id = sqlTable->getFieldIndex("location_id");
	int fServer_url = sqlTable->getFieldIndex("server_url");
	int fLocation_path = sqlTable->getFieldIndex("location_path");

	for (unsigned int i=0; i<sqlTable->size(); i++)
	{
		package_id = atoi(sqlTable->getValue(i, fPackages_package_id).c_str());
		for (unsigned int t=0; t<pkgList->size(); t++)
		{
			if (pkgList->get_package_ptr(t)->get_id()==package_id)
			{
				tmp.set_id(atoi(sqlTable->getValue(i, fLocation_id).c_str()));
				tmp.set_server_url(sqlTable->getValue(i, fServer_url));
				tmp.set_path(sqlTable->getValue(i, fLocation_path));
				pkgList->get_package_ptr(t)->get_locations_ptr()->push_back(tmp);
			}
		}
	}
	delete sqlTable;
}

int mpkgDatabase::get_package_id(const PACKAGE& package)
{
	SQLTable *sqlTable = new SQLTable;
	SQLRecord sqlFields;
	sqlFields.addField("package_id");
	SQLRecord sqlSearch;
	sqlSearch.addField("package_md5", package.get_md5());
	db.get_sql_vtable(*sqlTable, sqlFields, "packages", sqlSearch);

	if (sqlTable->empty())
	{
		delete sqlTable;
		return 0;
	}

	if (sqlTable->getRecordCount()==1)
	{
		int ret = atoi(sqlTable->getValue(0, sqlTable->getFieldIndex("package_id")).c_str());
		delete sqlTable;
		return ret;
	}
	if (sqlTable->getRecordCount()>1)
	{
		delete sqlTable;
		mError("Multiple package records, internal error!");
		return -1;
	}

	return -1;
}
int mpkgDatabase::update_package_record(int package_id, const SQLRecord& sqlUpdate) {
	SQLRecord sqlSearch;
	sqlSearch.addField("package_id", package_id);
	return db.sql_update("packages", sqlUpdate, sqlSearch);
}
int mpkgDatabase::set_installed(int package_id, int status)
{
       	SQLRecord sqlUpdate;
	sqlUpdate.addField("package_installed", status);
	SQLRecord sqlSearch;
	sqlSearch.addField("package_id", package_id);
	return db.sql_update("packages", sqlUpdate, sqlSearch);
}
int mpkgDatabase::set_configexist(int package_id, int status)
{
       	SQLRecord sqlUpdate;
	sqlUpdate.addField("package_configexist", status);
	SQLRecord sqlSearch;
	sqlSearch.addField("package_id", package_id);
	return db.sql_update("packages", sqlUpdate, sqlSearch);
}
int mpkgDatabase::set_action(int package_id, int status, const string& reason)
{

       	SQLRecord sqlUpdate;
	sqlUpdate.addField("package_action", status);
	sqlUpdate.addField("package_installed_by_dependency", reason);
	SQLRecord sqlSearch;
	sqlSearch.addField("package_id", package_id);
	return db.sql_update("packages", sqlUpdate, sqlSearch);
}


int mpkgDatabase::get_installed(int package_id)
{
	SQLTable *sqlTable = new SQLTable;
	SQLRecord sqlFields;
	sqlFields.addField("package_installed");
	SQLRecord sqlSearch;
	sqlSearch.addField("package_id", package_id);
	db.get_sql_vtable(*sqlTable, sqlFields, "packages", sqlSearch);

	if (sqlTable->empty())
	{
		delete sqlTable;
		return -1;
	}
	if (sqlTable->getRecordCount()==1)
	{
		int ret = atoi(sqlTable->getValue(0, sqlTable->getFieldIndex("package_installed")).c_str());
		delete sqlTable;
		return ret;
	}
	if (sqlTable->getRecordCount()>1)
	{
		delete sqlTable;
		return -3;
	}
	
	return -100;
}

int mpkgDatabase::get_action(int package_id)
{
	SQLTable *sqlTable = new SQLTable;
	SQLRecord sqlFields;
	sqlFields.addField("package_action");
	SQLRecord sqlSearch;
	sqlSearch.addField("package_id", package_id);
	db.get_sql_vtable(*sqlTable, sqlFields, "packages", sqlSearch);

	if (sqlTable->empty())
	{
		delete sqlTable;
		return -1;
	}
	if (sqlTable->getRecordCount()==1)
	{
		int ret = atoi(sqlTable->getValue(0, sqlTable->getFieldIndex("package_action")).c_str());
		delete sqlTable;
		return ret;
	}
	if (sqlTable->getRecordCount()>1)
	{
		delete sqlTable;
		return -3;
	}
	mError("shouldn't be here");
	return -100;
}

int mpkgDatabase::get_configexist(int package_id)
{
	SQLTable *sqlTable = new SQLTable;
	SQLRecord sqlFields;
	sqlFields.addField("package_configexist");
	SQLRecord sqlSearch;
	sqlSearch.addField("package_id", package_id);
	db.get_sql_vtable(*sqlTable, sqlFields, "packages", sqlSearch);

	if (sqlTable->empty())
	{
		delete sqlTable;
		return -1;
	}
	if (sqlTable->getRecordCount()==1)
	{
		int ret = atoi(sqlTable->getValue(0, sqlTable->getFieldIndex("package_configexist")).c_str());
		delete sqlTable;
		return ret;
	}
	if (sqlTable->getRecordCount()>1)
	{
		delete sqlTable;
		return -3;
	}
	
	return -100;
}

int mpkgDatabase::get_available(int package_id)
{
	mError("Deprecated");
	SQLTable *sqlTable = new SQLTable;
	SQLRecord sqlFields;
	sqlFields.addField("packages_package_id");
	SQLRecord sqlSearch;
	sqlSearch.addField("packages_package_id", package_id);
	db.get_sql_vtable(*sqlTable, sqlFields, "locations", sqlSearch);

	if (sqlTable->empty())
	{
		delete sqlTable;
		return 0;
	}
	else
	{
		delete sqlTable;
		return sqlTable->size();
		//int ret = atoi(sqlTable->getValue(0, "package_available").c_str());
	}
}

int mpkgDatabase::get_purge(const string& package_name)
{
	SQLRecord sqlSearch;
	sqlSearch.addField("package_name", package_name);
	SQLTable sqlTable;
	SQLTable sqlTableFiles;
	SQLRecord sqlFields;
	sqlFields.addField("package_id");
	sqlFields.addField("package_configexist");
	if (db.get_sql_vtable(sqlTable, sqlFields, "packages", sqlSearch)!=0)
	{
		return -1;
	}
	if (sqlTable.empty())
	{
		return 0;
	}
	int id=0;
	int fPackage_configexist = sqlTable.getFieldIndex("package_configexist");
	int fPackage_id = sqlTable.getFieldIndex("package_id");
	for (unsigned int i=0; i<sqlTable.getRecordCount(); i++)
	{
		if (sqlTable.getValue(i, fPackage_configexist)==IntToStr(ST_CONFIGEXIST))
		{
			id=atoi(sqlTable.getValue(i, fPackage_id).c_str());
			break;
		}
	}
	return id;
}
//--------------------------------------SQL PART----------------------------------------
SQLProxy* mpkgDatabase::getSqlDb()
{
	return &db;
}
// class SQLRecord
vector<string> SQLRecord::getRecordValues()
{
	vector<string> output;
	for (unsigned int i=0;i<field.size();i++)
	{
		output.resize(i+1);
		output[i]=field[i].value;
	}
	return output;

}
unsigned int SQLRecord::size() const {
	return field.size();
}

bool SQLRecord::empty() const {
	if (field.empty()) return true;
	return false;
}
const string& SQLRecord::getFieldName(unsigned int num) const {
	return field[num].fieldname;
}

vector<string> SQLRecord::getValueVector(const string& fieldname) {
	vector<string> ret;
	for (unsigned int i=0; i<field.size(); ++i) {
		if (field[i].fieldname==fieldname) {
			ret.push_back(field[i].value);
		}
	}
	return ret;
}

/*string SQLRecord::getValueX(const string& fieldname) {
	for (unsigned int i=0; i<field.size(); ++i) {
		if (field[i].fieldname==fieldname) {
			return field[i].value;
		}
	}
	return "";
}*/



const string& SQLRecord::getValue(const string& fieldname) const {
	for (unsigned int i=0;i<field.size();i++)
	{
		if (field[i].fieldname==fieldname)
		{
			//PrepareSql(&field[i].value);
			return field[i].value;
		}
	}
	mError("No such field " + fieldname);
	mError("Available fields:");
	for (unsigned int i=0; i<field.size(); i++)
	{
		mError(field[i].fieldname);
	}
	abort();
}

bool SQLRecord::setValue(const string& fieldname, const string& value) {
	for (unsigned int i=0; i<field.size(); i++) {
		if (field[i].fieldname==fieldname) {
			field[i].value=value;
			PrepareSql(field[i].value);
			return true;
		}
	}
	return false;
}

void SQLRecord::setValue(unsigned int& field_index, const string& value) {
	field[field_index].value = value;
	PrepareSql(field[field_index].value);
}

void SQLRecord::addField(const string& fieldname, const string& value) {
	SQLField tmp;
	tmp.fieldname=fieldname;
	tmp.value=value;
	PrepareSql(tmp.value);
	field.push_back(tmp);
}
/*void SQLRecord::addField(const string& fieldname, const string& value) {
	addField(fieldname, &value);
}*/
void SQLRecord::addField(const string& fieldname)
{
	string nullstr;
	addField(fieldname, nullstr);
}

void SQLRecord::addField(const string& fieldname, const int& value)
{
	//string istr = IntToStr(value);
	addField(fieldname, IntToStr(value));
}
const string& SQLRecord::getValueI(unsigned int num) const {
	if (field.size()<=num) {
		cout << __func__ << ": field size " << field.size() << " < " << num << "\n" << endl;
		abort();
	}
		//PrepareSql(&field[num].value);
	return field[num].value;
}
string* SQLRecord::getValueIPtr(unsigned int num) {
	//PrepareSql(&field[num].value);
	if (field.size()<=num) {
		cout << __func__ << ": field size " << field.size() << " < " << num << "\n" << endl;
		abort();
	}

	return &field[num].value;
}

int SQLRecord::getFieldIndex(const string& fieldname) const {
	for (unsigned int i=0;i<field.size();++i) {
		if (field[i].fieldname==fieldname) {
			return i;
		}
	}
	return -1;
}


void SQLRecord::clear()
{
	field.clear();
}
void SQLRecord::setSearchMode(int mode)
{
	search_type=mode;
}

const int& SQLRecord::getSearchMode() const {
	return search_type;
}

void SQLRecord::setEqMode(int mode)
{
	eq_type=mode;
}

const int& SQLRecord::getEqMode() const {
	return eq_type;
}

SQLRecord::SQLRecord()
{
	search_type=SEARCH_AND;
	eq_type=EQ_EQUAL;
}
SQLRecord::~SQLRecord(){}

unsigned int SQLTable::size() const {
	return table.size();
}
unsigned int SQLTable::getRecordCount() const {
	return table.size();
}

bool SQLTable::empty() const {
	if (table.size()==0) return true;
	return false;
}

void SQLTable::clear()
{
	table.clear();
}

const string& SQLTable::getValue(unsigned int num, const string& fieldname) const {
	if (num<table.size()) {
		return table[num].getValue(fieldname);
	}
	else {
		mError("Cannot find field " + fieldname+ " with ID " + IntToStr(num) + ": no such ID. Aborting...");
		abort();
	}
}

const string& SQLTable::getValue(unsigned int num, const int& field_index) const {
	if (table.size()<=num) {
		cout << __func__ << ": field size " << table.size() << " < " << num << "\n" << endl;
		abort();
	}
	return table[num].getValueI(field_index);
}

string* SQLTable::getValuePtr(unsigned int num, const int& field_index) {
	if (table.size()<=num) {
		cout << __func__ << ": field size " << table.size() << " < " << num << "\n" << endl;
		abort();
	}

	return table[num].getValueIPtr(field_index);
}

int SQLTable::getFieldIndex(const string& fieldname) const
{
	if (table.size()==0) return -2;
	else return table[0].getFieldIndex(fieldname);
}
SQLRecord* SQLTable::getRecord(unsigned int num)
{
	if (num<table.size()) return &table[num];
	else
	{
		mError("core.cpp: SQLTable::getRecord():  record number " + IntToStr(num) + "is out of range");
		abort();
		return NULL;
	}
}

void SQLTable::addRecord(const SQLRecord& record) {
	table.push_back(record);
}


SQLTable::SQLTable(){}
SQLTable::~SQLTable(){}



