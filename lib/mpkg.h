/***********************************************************************************
 * 	$Id: mpkg.h,v 1.43 2007/12/07 03:34:20 i27249 Exp $
 * 	MPKG Package System
 * ********************************************************************************/

#ifndef MPKG_H_
#define MPKG_H_

#include <math.h>
//#include "constants.h"
#include "debug.h"
#include "config.h"
#include "sql_pool.h"
#include "htmlcore.h"
#include <ftw.h>
class mpkgDatabase
{
	public:
		int get_sql_vtable(SQLTable& output, const SQLRecord& fields, const string& table_name, const SQLRecord& search);
		void exportPackage(const string& output_dir, PACKAGE& p);
		void unexportPackage(const string& output_dir, const PACKAGE& p);
		// Functions to get data
		int get_package(const int& package_id, PACKAGE *package, bool no_cache=false);
		int get_packagelist(const SQLRecord& sqlSearch, PACKAGE_LIST *packagelist, bool ultraFast=false);
#ifdef ENABLE_INTERNATIONAL
		int get_descriptionlist(const int& package_id, vector<DESCRIPTION> *desclist, string language="");
#endif
		int get_filelist (const int& package_id, vector<FILES> *filelist, bool config_only=false);
		void get_full_filelist(PACKAGE_LIST *pkgList);
		int get_dependencylist(int package_id, vector<DEPENDENCY> *deplist);
		void get_full_dependencylist(PACKAGE_LIST *pkgList);
		void get_available_tags(vector<string>* output);
		//int get_taglist(int package_id, vector<string> *taglist);
		void get_full_taglist(PACKAGE_LIST *pkgList);
//		int get_server_tag_list(int server_id, SERVER_TAG_LIST *server_tag_list);
//		int get_server(int server_id, SERVER *server);
		int get_locationlist(int package_id, vector<LOCATION> *location_list);
		void get_full_locationlist(PACKAGE_LIST *pkgList);
		void get_full_deltalist(PACKAGE_LIST *pkgList);
//		int get_last_id(/*string table_name, string id_field=""*/);
		int get_package_id(const PACKAGE& package);
		int get_installed(int package_id);
		int get_available(int package_id);
		int get_action(int package_id);
		int get_configexist(int package_id);

//		int get_scripts(int package_id, SCRIPTS *scripts);
		PACKAGE_LIST get_other_versions(const string& package_name);
		PACKAGE* get_max_version(PACKAGE_LIST& pkgList, const DEPENDENCY& dep);
		bool checkVersion(const string& version1, const int& condition, const string& version2);
		int sqlFlush();
		
		// Checking functions
		//int check_file_conflicts (const PACKAGE& package);
		int check_file_conflicts_new (const PACKAGE& package);
		//int backupFile(string *filename, int overwritten_package_id, int conflicted_package_id);
		int backupFiles(vector <string *> fileNames, vector<int> overwritten_package_ids, int conflicted_package_id, bool skipMove=false);
		void clean_backup_directory();
		int add_conflict_record(int conflicted_id, int overwritten_id, const string& file_name);
		bool add_conflict_records(int conflicted_id, vector<int> overwritten_ids, vector<string *> file_names, PACKAGE_LIST& pkgList);
		int delete_conflict_record(int conflicted_id, const string& file_name);
		void get_conflict_records(int conflicted_id, vector<FILES> *ret); // Returns list of files overwritten by conflicted_id
		void get_backup_records(const PACKAGE& package, vector<FILES> *ret); // Returns a list of files for package which has been overwritten
		bool checkEssentialFile(const string& f);
		void fillEssentialFiles(bool force_update=false);

		vector<FILES> essentialFiles;



		// Action functions
		int add_filelist_record(int package_id, vector<FILES> *filelist);
#ifdef ENABLE_INTERNATIONAL
		int add_descriptionlist_record(int package_id, vector<DESCRIPTION> *desclist);
#endif
		int add_locationlist_record(int package_id, vector<LOCATION> *locationlist);
		int add_delta_record(const int package_id, const vector<DeltaSource>& deltaSources);
		int add_dependencylist_record(int package_id, vector<DEPENDENCY> *deplist);
		int add_taglist_record(int package_id, vector<string> *taglist);
		int add_tag_link(int package_id, int tag_id);
		int add_package_record(PACKAGE *package);
		int update_package_data(int package_id, PACKAGE *package);
		int update_package_record(int package_id, const SQLRecord& sqlUpdate);

		// Purge functions
		int get_purge(const string& package_name);
	
		// Cleanup functions
		int delete_packages(PACKAGE_LIST *pkgList);
		int cleanFileList(int package_id);
		int clean_package_filelist (PACKAGE *package);
		int clear_unreachable_packages();
		int sql_exec(const string&);

	private:
		SQLProxy db;
		PACKAGE_LIST packageDBCache;
		bool hasFileList;
	public:
		void createDBCache();
		bool check_cache(PACKAGE *package, bool clear_wrong=true, bool quiet = true);
		int commit_actions(); // Commits ALL actions planned in database
		//int set_available(int package_id, int status);
		int set_action(int package_id, int status, const string& reason);
		int set_configexist(int package_id, int status);
		int set_installed(int package_id, int status);

		int emerge_to_db(PACKAGE *package);	// Adds new package to database, filtering data
		int install_package(PACKAGE *package, unsigned int packageNum=0, unsigned int packagesTotal=0); // PHYSICALLY install package (extract, execute scripts)
		int remove_package(PACKAGE *package, unsigned int packageNum=0, unsigned int packagesTotal=0); // PHYSICALLY remove package (delete files, execute remove scripts)
		int uninstall(vector<string>* pkgnames); // Wrapper, uninstalls all packages with given names.
		int updateRepositoryData(PACKAGE_LIST *newPackages);
		int syncronize_data(PACKAGE_LIST *pkgList);
		PACKAGE get_installed_package(const string& pkg_name); // Returns installed package

		SQLProxy *getSqlDb();
		mpkgDatabase();
		~mpkgDatabase();
};

bool tryGetDelta(PACKAGE *p, const string workingDir="/var/mpkg/cache/");
long double guessDeltaSize(const PACKAGE& p, const string workingDir="/var/mpkg/cache/");
int applyXZPatch(const string& orig, const string& dest, const string& patchname, const string workingDir="/var/mpkg/cache/");
int emerge_package(string file_url, string *package_name, string march="", string mtune="", string olevel="", string *builddir_name_ret=NULL); // Assembly from source
#include "local_package.h"
#include "core.h"
#include "dependencies.h"
#endif //MPKG_H_


