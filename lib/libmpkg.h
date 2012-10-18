/********************************************************************************
 * MPKG packaging system: core API
 * $Id: libmpkg.h,v 1.38 2007/12/04 18:48:34 i27249 Exp $
 *
 * Use this header to access the library. No other headers are needed :)
 * *****************************************************************************/

#ifndef LIBMPKG_H_
#define LIBMPKG_H_

// Includes: some overhead, but enough.
#include "version.h"
#include "config.h"
#include "local_package.h"
#include "debug.h"
#include "mpkg.h"
#include "spkgsupport.h"
#include "repository.h"
#include "actions.h"
#include "converter.h"
#include "mpkgsys.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include "package.h"
#include "metaframe.h"
// Package working tool =)
// Main class - use only this one while using the library
class mpkg
{
	public:
		// Constructor & destructor
		mpkg(bool _loadDatabase=true);
		~mpkg();

		// Cleaning
		void clean_queue();
		int unqueue(int package_id);
		// Error checking
		
		bool init_ok;
		/* Boolean value:
		 * TRUE if all initialized OK (database is opened and passed integrity check, etc)
		 * FALSE if something goes wrong
		 @@*/

		// Interface transports - current status messages
		string current_status();
		/* Contains current status. Used to make user output more informative.
		 * @@*/

		// Package building
		int build_package(string out_directory="", bool source=false);
		/* build_package:
		 * Builds package in current directory
		 * It gets data from install/data.xml, and runs makepkg based on package information.
		 * Returns 0 if all ok, -1 if fails.
		 * @@*/

		int convert_directory(string output_dir);
		/* convert_directory(string output_dir):
		 * Converts current directory (including subdirs) from old Slackware format to new.
		 * It collects data from package and generates install/data.xml, and repackages the
		 * package with extended data. Output will be placed in path defined by output_dir variable.
		 * Returns 0 if all ok, <0 (e.g. -1) if something failed.
		 * @@*/

		// Repository building
		Repository rep;
		/* rep:
		 * Object containing repository processing functions.
		 * Try to not use it directly, in future, should be moved to private.
		 * @@*/
		
		// Package installation, removing, upgrade
		int install(vector<string> fname, vector<string> *p_version=NULL, vector<string> *p_build=NULL, vector<string> *eList = NULL);
		int install(string fname);
		int install(PACKAGE *pkg);
		int install(PACKAGE_LIST *pkgList);
		//int install(int package_id);
		int installGroups(vector<string>groupName);
		/* install(vector<string> fname):
		 * Prepares packages with names defined in fname vector to install.
		 * It checks availability, chooses newest version, builds dependency tree, and
		 * marks all required packages to install/remove/upgrade.
		 * Note: this function does NOT actually install the package, it just places it on the queue.
		 * Returns 0 if successful, <0 if fails.
		 * @@*/

		int uninstall(vector<string> pkg_name);
		int uninstall(int package_id);
		int uninstall(PACKAGE_LIST *pkgList);
		/* uninstall(vector<string> pkg_name):
		 * Prepares packages with names defined by pkg_name vector to uninstall.
		 * It builds dependency tree, and marks all required packages to remove.
		 * Returns 0 if successful, <0 if fails.
		 * @@*/
		/*
		int upgrade (vector<string> pkgname);
		int upgrade (PACKAGE *pkg);
		int upgrade (PACKAGE_LIST *pkg);*/
		/* upgrade (vector<string> pkgname):
		 * Upgrades packages defined by pkgname vector.
		 * It builds dependency tree, and marks all required packages to remove and install.
		 * Returns 0 if successful, <0 if fails.
		 * @@*/

		int purge(vector<string> pkg_name);
		int purge(int package_id);
		/* purge (vector<string> pkg_name):
		 * Purges package. Similar to uninstall, but mark to remove package completely (including config files).
		 * If package is already uninstalled, it marks configuration files to be removed.
		 * Returns 0 if OK, <0 if fails.
		 * @@*/
		
		// Repository data updating
		int update_repository_data();
		void cleanCdromCache(string s_root = "");
		/* update_repository_data():
		 * Updates a list of available packages from the repositories defined in configuration file.
		 * Returns 0 of OK, <0 if something fails.
		 * @@*/

		// Cache cleaning
		int clean_cache(bool clean_symlinks=false);
		/* clean_cache():
		 * Removes all packages from download cache (or only symlinks if clean_symlinks = true).
		 * Returns 0 if OK, <0 if fails.
		 * @@*/

		// Package list retrieving
		int get_packagelist(SQLRecord& sqlSearch, PACKAGE_LIST *packagelist, bool ultraFast=false, bool needDescriptions=true);
		/* get_packagelist(SQLRecord, PACKAGE_LIST *, bool):
		 * Fills the packagelist by packages who meets sqlSearch requirements.
		 * To use this, you should define sqlSearch first. See documentation about an SQLRecord object.
		 * If GetExtraInfo is true (default value), the function retrieves a complete info about packages,
		 * otherwise only basic (name, ver, arch, build, sizes, descriptions and changelog) will be retrieved
		 * (which is little faster). For now on, I don't know any computer who was so slow so it requires this speedup.
		 * Returns 0 if all ok, <0 if fails.
		 * @@*/

		// Dependency tree
		int getDependencyTree(PACKAGE_LIST *tree);

		// Configuration and settings: retrieving
		vector<string> get_repositorylist();
		vector<string> get_disabled_repositorylist();
		//int set_disabled_repositorylist(vector<string>drList);
		/* get_repositorylist():
		 * returns a vector with URLs of currently enabled repositories.
		 * @@*/
		//SERVER_LIST get_fullrepositorylist();
		string get_sysroot(); // Returns system root path (in normal sys, should be '/')

		string get_cdromdevice();
		string get_cdrommountpoint();
		string get_syscache(); // Returns package cache path
		string get_dburl(); // Returns database URL
		string get_scriptsdir(); // Returns package scripts dir path
		bool get_runscripts(); // True if configured to run scripts, false otherwise.
		unsigned int get_checkFiles();

		// Configuration and settings: setting
		int add_repository(string repository_url);
		int remove_repository(int repository_num);
		int enable_repository(vector<int> repository_num);
		int disable_repository(vector<int> repository_num);
		int set_repositorylist(vector<string> newrepositorylist, vector<string> drList); // Sets new repository list to config
		//int set_fullrepositorylist(SERVER_LIST newrepositorylist);
		int set_sysroot(string newsysroot); // Sets new system root to config
		int set_syscache(string newsyscache); // Sets new system cache to config
		int set_dburl(string newdburl); // Sets new database to use
		int set_scriptsdir(string newscriptsdir); // Sets a scripts directory
		int set_runscripts(bool dorun); // Enables or disables a script running
		int set_cdromdevice(string cdromDevice);
		int set_cdrommountpoint(string cdromMountPoint);
		int set_checkFiles(unsigned int value);
		
		void get_available_tags(vector<string>* output);
		// Error bus


		// Finalizing
		int commit(bool deferred=false); 
		/* Committing changes:
		 * this function ACTUALLY perform all queued actions: install, upgrade, remove, etc.
		 * @@*/

		bool checkPackageIntegrity(PACKAGE *package);
		bool checkPackageIntegrity(const string& pkgName);
	
		bool repair(PACKAGE *package);
		bool repair(string fname);
		// Database export
		

		void exportBase(string output_dir="/var/log/packages");
		// Objects to work with. Normally should be private, so try to avoid using it directly.
		mpkgDatabase *db;
		DependencyTracker *DepTracker;

		int syncronize_repositories(string sync_map);

		// Useful :)
#define QUEUEFILTER_NONE 0
#define QUEUEFILTER_INSTALL 1
#define QUEUEFILTER_REMOVE 2
#define QUEUEFILTER_REMOVEPURGE 3
#define QUEUEFILTER_PURGE 4
#define QUEUEFILTER_REPAIR 5
#define QUEUEFILTER_UPDATE 6
		void get_queue(PACKAGE_LIST *, int filter=QUEUEFILTER_NONE);
		vector<string> getLatestUpdates(PACKAGE_LIST *resultList, PACKAGE_LIST *uninstList, bool fast=false, bool needDescriptions = true);
		vector<string> getExportInstalled(bool include_versions);


};

void generateDeps_new(mpkg& core, string tgz_filename);

vector<string> preprocessInstallList(const string &filename);
void parseInstallList(const vector<string> &data, vector<string> &installQuery, vector<string> &versionQuery);
bool checkUtility(const string &);
bool checkUtilities();

void calculatePkgSetSize(CustomPkgSet &set, const string& file_name, bool merge);
CustomPkgSet getCustomPkgSet(const string& name, const string& locale);
CustomPkgSet getUserCustomPkgSet(const string &path, const string& base_name, bool merge, const string& locale);

PACKAGE_LIST buildInstallQueue(const vector<string> &list);

#endif //LIBMPKG_H_

