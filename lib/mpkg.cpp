/***********************************************************************
 * 	$Id: mpkg.cpp,v 1.132 2007/12/11 05:38:29 i27249 Exp $
 * 	MPKG packaging system
 * ********************************************************************/
// #define INSTALL_DEBUG // Enable this if you need to debug it fast

#include "mpkg.h"
#include "syscommands.h"
#include "DownloadManager.h"
#include <iostream>
#include "package.h"
#include "build.h"
#include "terminal.h"
#include "htmlcore.h"
#include <fstream>
#include <stdint.h>
#include "errorhandler.h"
#include "transactions.h"
#include "deltas.h"
#include "spkgsupport.h"
// Two functions to install/remove configuration files
void pkgConfigInstall(const PACKAGE &package) {
	if (package.config_files.empty()) {
		return;
	}
       	
	bool sysconf_exists, orig_exists;
	string sysconf_name, orig_name, old_name;
	for (size_t i=0; i<package.config_files.size(); ++i) {
		if (verbose) printf("Install: checking config file %s\n", sysconf_name.c_str());
		
		if (package.config_files[i].hasAttribute("class", "sample")) continue; // No need for manipulation with these files
		if (package.config_files[i].hasAttribute("class", "generated")) continue; // Nothing to do with this stuff
		
		sysconf_name = SYS_ROOT + "/" + package.config_files[i].name;
		orig_name = SYS_ROOT+"/var/mpkg/configs/" + package.get_corename() + "/" + package.config_files[i].name;
		old_name = SYS_ROOT+"/var/mpkg/configs/" + package.get_corename() + "/" + package.config_files[i].name + ".old";

 
		// Проверяется, есть ли уже такой конфиг в системе:
		sysconf_exists = FileExists(sysconf_name);
 
		// Проверяется, есть ли копия предыдущего оригинального конфига в var/mpkg/configs/$pkgname/$conf_path/$conf_file
		orig_exists = FileExists(orig_name);
 
		// Create a copy of new conf
		system("mkdir -p " + getDirectory(orig_name));
		system("cp " + sysconf_name + ".new " + orig_name); // Creating backup copy

		// If an existing config is identical with new, remove new
		if (sysconf_exists && get_file_md5(sysconf_name) == get_file_md5(sysconf_name + ".new")) {
			unlink(string(sysconf_name + ".new").c_str());
		}

		// If no old config, or has force_new flag, move new to sysconf_name
		if (!sysconf_exists) {
			system("mv " + sysconf_name + ".new " + sysconf_name);
		}

		// In case of force_new, force overwrite AND copy to old_name previous variant
		if (package.config_files[i].hasAttribute("force_new", "true")) {
			system("cp " + sysconf_name + " " + old_name);
			system("mv " + sysconf_name + ".new " + sysconf_name);
			printf(_("\n%s: Note: %s has forced to be new, old config backed up\n"), package.get_name().c_str(), sysconf_name.c_str());
		}

		// Note user if a .new file still exists.
		if (FileExists(sysconf_name + ".new")) printf(_("\n%s: Note: %s%s.new%s exists, %scheck for changes%s\n"), package.get_name().c_str(), CL_GREEN, sysconf_name.c_str(), CL_WHITE, CL_RED, CL_WHITE);
		
		// What to do with a .new one if it has to be leaved there? Let's leave it alone, until decided otherwise.
	}
}

void pkgConfigRemove(const PACKAGE &package) {
	// Check only one thing: if original file is the same as current one, we can remove it freely.
	if (package.config_files.empty()) return;
       	
	bool sysconf_exists, orig_exists;
	string sysconf_name, orig_name, old_name;
	for (size_t i=0; i<package.config_files.size(); ++i) {
		sysconf_name = SYS_ROOT + "/" + package.config_files[i].name;
		orig_name = SYS_ROOT+"/var/mpkg/configs/" + package.get_corename() + "/" + package.config_files[i].name;
		old_name = SYS_ROOT+"/var/mpkg/configs/" + package.get_corename() + "/" + package.config_files[i].name + ".old";
 
		// Проверяется, есть ли уже такой конфиг в системе:
		sysconf_exists = FileExists(sysconf_name);
 
		// Проверяется, есть ли копия предыдущего оригинального конфига в var/mpkg/configs/$pkgname/$conf_path/$conf_file
		orig_exists = FileExists(orig_name);

		if (sysconf_exists && orig_exists && get_file_md5(sysconf_name)==get_file_md5(orig_name)) {
			if (verbose) printf("Removing unmodified config file %s\n", sysconf_name.c_str());
			unlink(sysconf_name.c_str());
			unlink(orig_name.c_str());
		}
		else if (sysconf_exists) printf(_("Modified config: %s, leaving in place\n"), sysconf_name.c_str());
		else printf(_("Config file %s vanished: perhaps it was removed by someone else.\n"), sysconf_name.c_str());
	}

	
}


mpkgDatabase::mpkgDatabase()
{
	hasFileList=false;
	essentialFilesFilled=false;
}
mpkgDatabase::~mpkgDatabase(){}

int mpkgDatabase::sqlFlush()
{
	return db.sqlFlush();
}

PACKAGE mpkgDatabase::get_installed_package(const string& pkg_name)
{
	PACKAGE_LIST packagelist;
	SQLRecord sqlSearch;
	sqlSearch.addField("package_name", pkg_name);
	sqlSearch.addField("package_installed", ST_INSTALLED);

	get_packagelist(sqlSearch, &packagelist, true, false);
	// We do NOT allow multiple packages with same name to be installed, so, we simply get first package of list.
	
	if (packagelist.size()>0)
		return packagelist[0];
	else
	{
		PACKAGE ret;
		return ret;
	}
}
	

int mpkgDatabase::emerge_to_db(PACKAGE *package)
{
	int pkg_id;
	pkg_id=get_package_id(*package);
	//printf("add %s: %d\n", package->get_name().c_str(), pkg_id);
	if (pkg_id==0)
	{
		// New package, adding
		add_package_record(package);
		return 0;
	}
	if (pkg_id<0)
	{
		// Query error
		return MPKGERROR_CRITICAL;
	}
	
	// Раз пакет уже в базе (и в единственном числе - а иначе и быть не должно), сравниваем данные.
	// В случае необходимости, добавляем location.
	PACKAGE db_package;
	vector<LOCATION> new_locations;
	get_package(pkg_id, &db_package);
	package->set_id(pkg_id);
	//printf("pkg has %d locations already, avail flag: %d\n", db_package.get_locations().size(), package->available());
	for (size_t j=0; j<package->get_locations().size(); ++j) {
		for (size_t i=0; i<db_package.get_locations().size(); i++) {
			if (!package->get_locations().at(j).equalTo(db_package.get_locations().at(i)))
			{
				new_locations.push_back(package->get_locations().at(j));
			}
		}
	}
	if (!new_locations.empty()) add_locationlist_record(pkg_id, &new_locations);
	return 0;
}


bool mpkgDatabase::check_cache(PACKAGE *package, bool clear_wrong, ItemOrder itemOrder_mark) {
	string fname = SYS_CACHE + "/" + package->get_filename();
	//if (package->usedSource.find("cdrom://")!=std::string::npos && FileExists(fname)) return true; // WHAT THE FUCK IS THIS??!!
	string got_md5;

	bool broken_sym;
	if (FileExists(fname, &broken_sym) && !broken_sym) {
		if (forceSkipLinkMD5Checks) {
			printf("Skipping MD5 check for %s\n", package->get_name().c_str());
			return true;
		}

		// Checking MD5
		pData.setItemCurrentAction(package->itemID, _("Checking MD5"), itemOrder_mark);
		if (verbose) printf("Checking MD5 of %s/%s", SYS_CACHE.c_str(), package->get_filename().c_str());
		got_md5 = get_file_md5(SYS_CACHE + "/" + package->get_filename());

		// If all ok, returning true
	       	if (package->get_md5() == got_md5) return true;
		
		// And, if wrong, see if we should clear bad link. Note that if we use enableDownloadResume flag, clear_wrong is set to false always.
		if (!clear_wrong) {
			if (!dialogMode && !enableDownloadResume) {
				say(_("Incorrect MD5 for package %s: received %s, but should be %s\n"), package->get_name().c_str(), got_md5.c_str(), package->get_md5().c_str());
				mpkgErrorHandler.callError(MPKG_DOWNLOAD_ERROR, _("Invalid checksum in downloaded file ") + package->get_filename());
				printf("\n\n");
			}
		}
		else { // If clearing, just do it. Used in first check when no download resume enabled.
			if (!dialogMode && verbose) printf("Clearing orphaned file %s\n", fname.c_str());
			unlink(fname.c_str());
		}
	}
	return false;
}

bool needUpdateXFonts = false;
vector<string> iconCacheUpdates;
vector<string> gconfSchemas, gconfSchemasUninstall;

int mpkgDatabase::commit_actions()
{
	hookManager.reset();
	needUpdateXFonts = false;
	delete_tmp_files();
	sqlFlush();
	// Checking for utilities
	if (!checkUtilities()) return MPKGERROR_CRITICAL;
	if (checkUtility("deltup")) _cmdOptions["deltup"]="true";

	// Zero: purging required packages
	// First: removing required packages
	unsigned int installFailures=0;
	unsigned int removeFailures=0;
	PACKAGE_LIST remove_list;
	SQLRecord sqlSearch;
	sqlSearch.setSearchMode(SEARCH_IN);
	sqlSearch.addField("package_action", ST_REMOVE);
	sqlSearch.addField("package_action", ST_PURGE);
	if (dialogMode) ncInterface.setTitle("AgiliaLinux " + (string) DISTRO_VERSION);
	if (dialogMode) ncInterface.setSubtitle(_("Preparing to package installation"));
	if (dialogMode) {
		ncInterface.setProgressText(_("Requesting list of packages marked to remove"));
		ncInterface.setProgressMax(7); //TODO: поправить в соответствие с реальным количеством шагов
		ncInterface.setProgress(1);
	}
	//printf("SLOW GET_PACKAGELIST CALL: %s %d\n", __func__, __LINE__);
	if (get_packagelist(sqlSearch, &remove_list, false, false)!=0) return MPKGERROR_SQLQUERYERROR;
	if (!remove_list.IsEmpty()) {
		if (dialogMode) {
			ncInterface.setProgressText(_("Sorting list of packages marked to remove"));
			ncInterface.setProgress(2);
		}
		remove_list.sortByTags(true);
	}
	PACKAGE_LIST install_list;
	sqlSearch.clear();
	sqlSearch.setSearchMode(SEARCH_IN);
	sqlSearch.addField("package_action", ST_INSTALL);
	sqlSearch.addField("package_action", ST_REPAIR);
	if (dialogMode) {
		ncInterface.setProgressText(_("Requesting list of packages marked to install"));
		ncInterface.setProgress(3);
	}
	//printf("SLOW GET_PACKAGELIST CALL: %s %d\n", __func__, __LINE__);
	if (get_packagelist(sqlSearch, &install_list, false, false)!=0) return MPKGERROR_SQLQUERYERROR;
	
	// Sorting install order: from lowest priority to maximum one 
	if (dialogMode) {
		ncInterface.setProgressText(_("Detecting package installation order (this may take a while)..."));
		ncInterface.setProgress(4);
	}

	install_list.sortByLocations();
	install_list.sortByTags();
	// FIXME: Order should be direct in case of installation AND update, but it should be reverse if packages are being removed. 
	// Also, glibc should be updated first, always.
	if (mConfig.getValue("old_sort").empty()) install_list.sortByPriorityNew(); // Buggy?

	// Don't forget to sort remove priority too: it does not take much time, but may be useful in case of disaster.
	if (mConfig.getValue("old_sort").empty()) remove_list.sortByPriorityNew(true); // Buggy?
	
	// Checking available space
	long double rem_size=0;
	long double ins_size=0;
	long double dl_size = 0;
	long double delta_max_size = 0;
	//Checking for removes and updates
	if (dialogMode) {
		ncInterface.setProgressText(_("Looking if an installation queue contains updates"));
		ncInterface.setProgress(5);
	}
	for (size_t i=0; i<remove_list.size(); i++)
	{
		remove_list.get_package_ptr(i)->itemID = pData.addItem(remove_list[i].get_name(), 10);
		rem_size+=strtod(remove_list[i].get_installed_size().c_str(), NULL);
		// Also, checking for update
		for (size_t t=0; t<install_list.size(); ++t) {
			if (install_list[t].get_name() == remove_list[i].get_name()) {
				remove_list.get_package_ptr(i)->set_action(ST_UPDATE, "upgrade-" + remove_list[i].package_action_reason);
				remove_list.get_package_ptr(i)->updatingBy=install_list.get_package_ptr(t);
				install_list.get_package_ptr(t)->updatingBy = remove_list.get_package_ptr(i);
			}
		}
	}
	// From now on, all packages in remove group who will be updated, has action ST_UPDATE
	if (dialogMode) {
		ncInterface.setProgressText(_("Checking disk free space"));
		ncInterface.setProgress(6);
	}
	for (size_t i=0; i<install_list.size(); i++)
	{
		install_list.get_package_ptr(i)->itemID = pData.addItem(install_list[i].get_name(), atoi(install_list[i].get_compressed_size().c_str()));
		ins_size += strtod(install_list[i].get_installed_size().c_str(), NULL);
		dl_size += strtod(install_list[i].get_compressed_size().c_str(), NULL);
		delta_max_size += guessDeltaSize(install_list[i]);
	}
	long double freespace = get_disk_freespace(SYS_ROOT);
	long double required_space = (ins_size - rem_size)+(ins_size - rem_size)*0.2;
	
	if (freespace < required_space)
	{
		if (dialogMode)
		{
			if (!ncInterface.showYesNo(_("It seems to be that disk free space is not enough to install. Required: ") + humanizeSize(required_space) + _(", available: ") + humanizeSize(freespace) + _("\nNote that if you splitted your filesystem tree (e.g. separate /usr, and so on), this information may be incorrect and you can safely ignore this warning.\nContinue anyway?")))
			{
				return MPKGERROR_COMMITERROR;
			}
		}
		else mWarning(_("It seems to be that disk free space is not enough to install. Required: ") + humanizeSize(required_space) + _(", available: ") + humanizeSize(freespace));
	}
	string branch;
	string distro;
	string is_virtual;
	size_t currentItemNum = 0, totalItemCount = 0;
	// Let's show the summary for console and dialog users and ask for confirmation
	if (consoleMode)
	{
		size_t installCount = 0, removeCount = 0, purgeCount = 0, repairCount = 0, updateCount = 0;
		string dialogMsg;
		string pkgTypeStr;
		string reason;
		//msay(_("Action summary:\n"));

		// Install
		for (size_t i=0; i<install_list.size(); ++i) {
			if (verbose) reason = install_list[i].package_action_reason;
			branch=install_list[i].get_repository_tags();
			distro = install_list[i].package_distro_version;
			if (branch=="0") branch.clear();
			if (!branch.empty()) branch = "[" + branch + "]";
			if (install_list[i].action()==ST_INSTALL && install_list[i].updatingBy==NULL) {
				if (installCount==0) {
					if (!dialogMode) msay(_("Will be installed:\n"));
					else dialogMsg += _("Will be installed:\n");
				}
				installCount++;
				if (install_list[i].get_type()==PKGTYPE_SOURCE || _cmdOptions["abuild_links_only"]=="yes") pkgTypeStr=_("(source)");
				else pkgTypeStr=_("(binary)");
				if (!dialogMode) {
					say("  [%d] %s %s %s %s %s\n", installCount, \
						install_list[i].get_name().c_str(), \
						install_list[i].get_fullversion().c_str(), branch.c_str(), pkgTypeStr.c_str(), reason.c_str());
						if (!checkAcceptedArch(install_list.get_package_ptr(i))) mWarning(string(CL_RED) + _("ARCHITECTURE WARNING: ") + string(CL_WHITE) + _("Package ") + install_list[i].get_name() + "-" + install_list[i].get_version() + "-" + install_list[i].get_arch() + "-" + install_list[i].get_build() + _(" has non-native architecture (") + install_list[i].get_arch() + _("), are you sure it is what you wanted?"));
				}
				else dialogMsg += "  [" + IntToStr(installCount) + "] " + install_list[i].get_name() + " " + install_list[i].get_fullversion() + branch + "\n";

			}
		}
		// Remove
		for (size_t i=0; i<remove_list.size(); i++) {
			branch=remove_list[i].get_repository_tags();
			distro = remove_list[i].package_distro_version;
			if (branch=="0") branch.clear();
			if (distro == "0") distro.clear();
			if (!branch.empty()) branch = "[" + branch + "]";

			if (verbose) reason = remove_list[i].package_action_reason;
			if (remove_list[i].action()==ST_REMOVE) {
				if (removeCount==0) {
					if (!dialogMode) msay(_("Will be removed:\n"));
					else dialogMsg+=_("Will be removed:\n");
				}
				removeCount++;
				if (remove_list[i].isTaggedBy("virtual")) is_virtual = string(" (") + _("virtual package") + string(")");
				else is_virtual.clear();
				if (!dialogMode) say("  [%d] %s %s %s %s%s\n", removeCount, \
						remove_list[i].get_name().c_str(), \
						remove_list[i].get_fullversion().c_str(), branch.c_str(), reason.c_str(), is_virtual.c_str());
				else dialogMsg += "  [" + IntToStr(removeCount) + "] " + remove_list[i].get_name() + " " + remove_list[i].get_fullversion() + branch + is_virtual + "\n";
			}
		}
		// Purge
		for (size_t i=0; i<remove_list.size(); i++) {
			branch=remove_list[i].get_repository_tags();
			if (branch=="0") branch.clear();
			if (distro == "0") distro.clear();
			if (!branch.empty()) branch = "[" + branch + "]";
			if (verbose) reason = remove_list[i].package_action_reason;

			if (remove_list[i].action()==ST_PURGE) {
				if (purgeCount==0) {
					if (!dialogMode) msay(_("Will be purged:\n"));
					else dialogMsg += _("Will be purged:\n");
				}
				if (remove_list[i].isTaggedBy("virtual")) is_virtual = string(" (") + _("virtual package") + string(")");
				else is_virtual.clear();

				purgeCount++;
				if (!dialogMode) say("  [%d] %s %s %s %s%s\n", purgeCount, \
						remove_list[i].get_name().c_str(), \
						remove_list[i].get_fullversion().c_str(), branch.c_str(), reason.c_str(), is_virtual.c_str());
				else dialogMsg += "  [" + IntToStr(purgeCount) + "] " + remove_list[i].get_name() + " " + remove_list[i].get_fullversion() + branch + is_virtual + "\n";
			}
		}
		// Update
		for (size_t i=0; i<remove_list.size(); i++) {
			if (remove_list[i].action()==ST_UPDATE) {
				branch=remove_list[i].updatingBy->get_repository_tags();
				if (branch=="0") branch.clear();
				if (distro=="0") distro.clear();
				if (!branch.empty()) branch = "[" + branch + "]";
				if (verbose) reason = remove_list[i].package_action_reason + " => " + remove_list[i].updatingBy->package_action_reason;

				if (updateCount==0) {
					if (!dialogMode) msay(_("Will be updated:\n"));
					else dialogMsg += _("Will be updated:\n");
				}
				updateCount++;
				if (!dialogMode) {
					say("  [%d] %s %s%s%s%s%s%s%s %s%s %s\n", updateCount, \
						remove_list[i].get_name().c_str(), \
						CL_6, remove_list[i].get_fullversion().c_str(), CL_WHITE, _(" ==> "), 
						CL_GREEN, remove_list[i].updatingBy->get_fullversion().c_str(), CL_BLUE, branch.c_str(), CL_WHITE, reason.c_str());

					if (!checkAcceptedArch(remove_list[i].updatingBy)) mWarning(string(CL_RED) + \
							_("ARCHITECTURE WARNING: ") + \
							string(CL_WHITE) + \
							_("Package ") + \
							remove_list[i].updatingBy->get_name() + "-" + \
							remove_list[i].updatingBy->get_version() + "-" + \
							remove_list[i].updatingBy->get_arch() + "-" + \
							remove_list[i].updatingBy->get_build() + \
							_(" has non-native architecture (") + \
							remove_list[i].updatingBy->get_arch() + \
							_("), are you sure it is what you wanted?"));
				}
				else dialogMsg += "  [" + IntToStr(updateCount) + "] " + \
						remove_list[i].get_name() + " " + \
						remove_list[i].get_fullversion() + " ==> " + \
						remove_list[i].updatingBy->get_fullversion() + branch + "\n";
			}
		}
		// Repair
		for (size_t i=0; i<install_list.size(); i++) {
			if (install_list[i].action()==ST_REPAIR) {
				if (repairCount==0) {
					if (!dialogMode) msay(_("Will be repaired:\n"));
					else dialogMsg += _("Will be repaired:\n");
				}
				if (verbose) reason = install_list[i].package_action_reason;
				repairCount++;
				if (!dialogMode) say("  [%d] %s %s %s\n", repairCount, \
						install_list[i].get_name().c_str(), \
						install_list[i].get_fullversion().c_str(), reason.c_str());
				else dialogMsg += "  [" + IntToStr(repairCount) + "] " + install_list[i].get_name() + " " + install_list[i].get_fullversion() + "\n";
			}
		}
		// Calculate total queue
		totalItemCount = installCount + removeCount + purgeCount + updateCount + repairCount;

		if (install_list.size()>0 || remove_list.size()>0)
		{
			if (!dialogMode) say("\n");
			else dialogMsg += "\n";
			if (ins_size > rem_size) {
				if (!dialogMode) say("%s: %s\n", _("Required disk space"), humanizeSize(ins_size - rem_size).c_str());
				else dialogMsg += _("Required disk space") + (string) ": " + humanizeSize(ins_size - rem_size) + "\n";
			}
			else {
				if (!dialogMode) say("%s: %s\n", _("Disk space will be freed"), humanizeSize(rem_size - ins_size).c_str());
				else dialogMsg += _("Disk space will be freed") + (string) ": " + humanizeSize(rem_size - ins_size) + "\n";
			}
			if (!dialogMode) say("%s: %s\n", _("Approximate size of deltas which can be used"), humanizeSize(delta_max_size).c_str());
			else dialogMsg += _("Approximate size of deltas which can be used:") + (string) ": " + humanizeSize(delta_max_size);

			if (!dialogMode) say("%s: %s\n", _("Maximum download size"), humanizeSize(dl_size).c_str());
			else dialogMsg += _("Maximum download size") + (string) ": " + humanizeSize(dl_size);

			if (interactive_mode && !dialogMode && !getlinksOnly)
			{
				say("\n");
				say(_("Continue? [Y/n]\n"));
				string input;
				bool skipBad;
				while (input!="y" && input!="Y" && input!="yes" && input!="\n") {
					input.clear();
					input=cin.get();
					skipBad = false;
					for (size_t q=0; !skipBad && q<input.size(); ++q) {
						if ((unsigned int) input[q]>127) skipBad = true;
					}
					if (skipBad) continue;
					if (input=="n" || input=="N" || input == "no") return MPKGERROR_ABORTED;
					if (input!="y" && input!="Y" && input!="yes" && input!="\n") {
						say(_("Please answer Y (yes) or N (no)\n"));
						cin.get();
					}
				}
			}
			else if (dialogMode && !setupMode) {
				if (!ncInterface.showText(dialogMsg, _("Continue"), _("Cancel"))) return MPKGERROR_ABORTED;
			}
		}
		else 
		{
			if (!dialogMode) say (_("Nothing to do\n"));
			else ncInterface.showMsgBox(_("Nothing to do\n"));
			return 0;
		}

	} // if (consoleMode && !dialogMode)
	if (getlinksOnly) {
		vector<string> urls;
		for (size_t i=0; i<install_list.size(); ++i) {
			urls.push_back(install_list[i].get_locations().at(0).get_full_url() + install_list[i].get_filename());
			printf("%s\n", urls[i].c_str());
		}
		return 0;
	}

	if (_cmdOptions["abuild_links_only"]=="yes") {
		printf("Source URL requested, here it is:\n");
		vector<string> urls;
		for (size_t i=0; i<install_list.size(); ++i) {
			urls.push_back(install_list[i].abuild_url.c_str());
			printf("%s\n", install_list[i].abuild_url.c_str());
		}
		if (!_cmdOptions["abuild_links_output"].empty()) {
			WriteFileStrings(_cmdOptions["abuild_links_output"], urls);
		}
		return 0;
	}
	
	int transaction_id = startTransaction(install_list, remove_list, getSqlDb());
	msay(_("Looking for install queue"));
	vector<bool> needFullDownload(install_list.size());
	ItemOrder thisItemOrder;
	if (install_list.size()>0) {
		// Building download queue
		msay(_("Looking for package locations"));
		DownloadsList downloadQueue;
		DownloadItem tmpDownloadItem;
		vector<string> itemLocations;
		pData.resetItems(_("waiting"), 0, 1, ITEMSTATE_WAIT);
		pData.setCurrentAction(_("Checking cache"));
		bool skip=false;
		if (dialogMode) ncInterface.setProgressMax(install_list.size());
		for (size_t i=0; i<install_list.size(); ++i) {
			needFullDownload[i]=false;
			if (dialogMode)	{
				ncInterface.setProgressText("["+IntToStr(i+1)+"/"+IntToStr(install_list.size())+_("] Checking package cache and building installation queue: ") + install_list[i].get_name());
				ncInterface.setProgress(i);
			}
			delete_tmp_files();

			if (_abortActions) {
				sqlFlush();
				return MPKGERROR_ABORTED;
			}

			// Clear broken symlinks
			//clean_cache_symlinks();
			msay(_("Checking cache and building download queue: ") + install_list[i].get_name());
			
			// Item order mark. No last here.
			if (i==0) thisItemOrder = ITEMORDER_FIRST;
			else thisItemOrder = ITEMORDER_MIDDLE;
	

			if (skip || !check_cache(install_list.get_package_ptr(i), !enableDownloadResume, thisItemOrder)) {
				needFullDownload[i]=!tryGetDelta(install_list.get_package_ptr(i));
			}
			if (needFullDownload[i]) {

				itemLocations.clear();
				
				tmpDownloadItem.expectedSize=strtod(install_list[i].get_compressed_size().c_str(), NULL);
				tmpDownloadItem.file = SYS_CACHE + install_list[i].get_filename();
				tmpDownloadItem.name = install_list[i].get_name();
				tmpDownloadItem.priority = 0;
				tmpDownloadItem.status = DL_STATUS_WAIT;
				tmpDownloadItem.itemID = install_list[i].itemID;
				tmpDownloadItem.usedSource = (string *) &install_list[i].usedSource;
	
				install_list.get_package_ptr(i)->sortLocations();
				for (size_t k = 0; k < install_list[i].get_locations().size(); k++) {
					itemLocations.push_back(install_list[i].get_locations().at(k).get_server_url() \
						     + install_list[i].get_locations().at(k).get_path() \
						     + install_list[i].get_filename());
	
				}
				tmpDownloadItem.url_list = itemLocations;
				downloadQueue.push_back(tmpDownloadItem);
			}
	
			if (!setupMode) {
				pData.increaseItemProgress(install_list[i].itemID);
				pData.setItemState(install_list[i].itemID, ITEMSTATE_FINISHED);
			}
		}
		ncInterface.setProgress(0);
		bool do_download = true;
	
		pData.resetItems(_("waiting"), 0, 1, ITEMSTATE_WAIT);
download_process:
		// DOWNLOAD SECTION
		while(do_download)
		{
			do_download = false;

			if (CommonGetFileEx(downloadQueue, &currentItem) == DOWNLOAD_ERROR)
			{
				mError(_("Failed to download one or more files, process stopped"));
				if (!_abortActions) {
					return MPKGERROR_ABORTED;
				}
					
			}
		
		}
		pData.downloadAction=false;
		if (download_only) {
			say(_("Downloaded packages are stored in %s\n"), SYS_CACHE.c_str());
			mpkgSys::clean_queue(this);
			return 0;
		}
// installProcess:
	

		pData.resetItems(_("waiting"), 0, 1, ITEMSTATE_WAIT);
	
		skip=false;
		
		msay(_("Checking files (comparing MD5):"));
		pData.setCurrentAction(_("Checking md5"));
		if (dialogMode) {
			ncInterface.setProgressText(_("Checking packages integrity"));
			ncInterface.setProgress(0);
		}
		if (dialogMode) ncInterface.setProgressMax(install_list.size());
		for(size_t i=0; i<install_list.size(); i++)
		{
			if (_abortActions) {
				sqlFlush();
				return MPKGERROR_ABORTED;
			}
			if (forceSkipLinkMD5Checks) break;

			if (dialogMode)
			{
				ncInterface.setProgressText(_("Checking packages integrity: ") + install_list[i].get_name());
				ncInterface.setProgress((unsigned int) round(((double)(i)/(double) ((double) (install_list.size())/(double) (100)))));

			}
			msay(_("Checking md5 of downloaded files: ") + install_list[i].get_name());
	
			if (i==install_list.size()-1) thisItemOrder = ITEMORDER_LAST;
			else if (i==0) thisItemOrder = ITEMORDER_FIRST;
			else thisItemOrder = ITEMORDER_MIDDLE;

			if (!check_cache(install_list.get_package_ptr(i), false, thisItemOrder))
			{
				pData.setItemState(install_list[i].itemID, ITEMSTATE_FAILED);
	
				MpkgErrorReturn errRet = mpkgErrorHandler.callError(MPKG_DOWNLOAD_ERROR, _("Invalid checksum in downloaded file"));
				switch(errRet)
				{
					case MPKG_RETURN_IGNORE:
						say(_("Wrong checksum ignored, continuing...\n"));
						break;
					case MPKG_RETURN_RETRY:
						say(_("Re-downloading...\n")); // TODO: HEAVY CHECK!!!!
						do_download = true;
						goto download_process;
						break;
					case MPKG_RETURN_ABORT:
						say(_("Aborting installation\n"));
						return MPKGERROR_ABORTED;
						break;
					default:
						mError(_("Unknown reply, aborting"));
						return MPKGERROR_ABORTED;
						break;
				}
			}
			else if (!setupMode) pData.increaseItemProgress(install_list[i].itemID);
		}
		if (dialogMode) {
			ncInterface.setProgress(0);
			//dialogItem.closeGauge();
		}
	}

	if (remove_list.size()>0) {
		msay(_("Looking for remove queue"));
		msay(_("Removing ") + IntToStr(remove_list.size()) + _(" packages"));
		pData.setCurrentAction(_("Removing packages"));
	
		int removeItemID=0;
		for (size_t i=0; i<remove_list.size(); ++i) {
			removeItemID=remove_list[i].itemID;
			pData.setItemState(removeItemID, ITEMSTATE_WAIT);
			pData.setItemProgress(removeItemID, 0);
			pData.setItemProgressMaximum(removeItemID,8);
		}
		string _actionName;
		ncInterface.setProgressMax(remove_list.size());
		for (size_t i=0; i<remove_list.size(); ++i) {
			if (remove_list[i].action()==ST_UPDATE) {
				_actionName = _("Updating package");
				if (dialogMode) ncInterface.setSubtitle(_("Updating packages"));
			}
			else {
				_actionName = _("Removing package");
				if (dialogMode) ncInterface.setSubtitle(_("Removing packages"));
			}
			currentItemNum++;
			
			if (dialogMode) {
				ncInterface.setProgressText("[" + IntToStr(currentItemNum) + "/" + IntToStr(totalItemCount) + "] " + _actionName + " " + \
						remove_list[i].get_name() + "-" + \
						remove_list[i].get_fullversion());
			       	ncInterface.setProgress((unsigned int) round((double)(i)/(double)((double)(remove_list.size())/(double) (100))));
			}
			delete_tmp_files();
			if (_abortActions) {
				sqlFlush();
				return MPKGERROR_ABORTED;
			}
			pData.setItemState(remove_list[i].itemID, ITEMSTATE_INPROGRESS);

			msay(_actionName+" " + remove_list[i].get_name());

			if (remove_package(remove_list.get_package_ptr(i), currentItemNum, totalItemCount, transaction_id)!=0) {
				removeFailures++;
				pData.setItemCurrentAction(remove_list[i].itemID, _("Remove failed"));
				pData.setItemState(remove_list[i].itemID, ITEMSTATE_FAILED);
			}
			else {
				pData.setItemCurrentAction(remove_list[i].itemID, _("Removed"));
				pData.setItemState(remove_list[i].itemID, ITEMSTATE_FINISHED);
			}
		}
		sqlSearch.clear();

		clean_backup_directory();
	} // Done removing packages

	if (install_list.size()>0) {	
		// Actually installing

		pData.setCurrentAction(_("Installing packages"));
		uint64_t sz = 0;
		for(size_t i=0; i<install_list.size(); i++) {
			sz += (uint64_t) atol(install_list[i].get_installed_size().c_str());
		}
		pData.resetItems(_("waiting"), 0, 8, ITEMSTATE_WAIT);
	

		if (dialogMode) {
			ncInterface.setSubtitle(_("Installing packages"));
			ncInterface.setProgressMax(install_list.size());
		}
		vector<time_t> pkgInstallTime;
		vector<int64_t> pkgInstallSize;
		time_t pkgInstallStartTime=0, pkgInstallEndTime=0, pkgTotalInstallTime=0;
		int64_t pkgInstallCurrentSize=0;
		long double pkgInstallSpeed=0;
		time_t ETA_Time = 0;
		MpkgErrorCode install_result;
		for (size_t i=0;i<install_list.size(); ++i) {
			currentItemNum++;
			pkgInstallStartTime=time(NULL); // TIMER 1: mark package installation start
			
			if (_abortActions) {
				sqlFlush();
				return MPKGERROR_ABORTED;
			}
			pData.setItemCurrentAction(install_list[i].itemID, string("installing [") + IntToStr(currentItemNum) + "/" + IntToStr(totalItemCount) + ", " + humanizeSize(IntToStr(pkgInstallSpeed)) + _("/s, ETA: ") + IntToStr(ETA_Time/60) + _(" min") + string("]"));
			pData.setItemState(install_list[i].itemID, ITEMSTATE_INPROGRESS);
			msay(_("Installing package ") + install_list[i].get_name());

			if (dialogMode)
			{
				if (pkgInstallTime.size()>1 && pkgInstallSpeed!=0) {
					ncInterface.setSubtitle(_("Installing packages") + string(" [") + humanizeSize(IntToStr(pkgInstallSpeed)) + _("/s, ETA: ") + IntToStr((ETA_Time/60)+1) + _(" min") + string("]"));
				}
				ncInterface.setProgressText("[" + IntToStr(i+1) + "/" + IntToStr(install_list.size()) + _("] Installing: ") + \
						install_list[i].get_name() + "-" + \
						install_list[i].get_fullversion());
				ncInterface.setProgress(i+1);
				//csz += atol(install_list[i].get_installed_size().c_str());
				//ncInterface.setProgress(csz);
			}
			install_result = (MpkgErrorCode) install_package(install_list.get_package_ptr(i), currentItemNum, totalItemCount, transaction_id);
			if (install_result!=0)
			{
				mpkgErrorHandler.callError(install_result, _("Failed to install package ") + install_list[i].get_name() + " " + install_list[i].get_fullversion());
				installFailures++;
				pData.setItemCurrentAction(install_list[i].itemID, _("Installation failed"));
				pData.setItemState(install_list[i].itemID, ITEMSTATE_FAILED);
				
				// change in 0.12.9: will stop installation if previous brokes...
				mError(_("Failed to install package ") + install_list[i].get_name() + _(". Due to possible dependency issues, the installation procedure will stop now."));
				return MPKGERROR_COMMITERROR;
			}
			else
			{
				if (install_list[i].get_name()=="aaa_elflibs" || install_list[i].get_name().find("glibc")==0) {
					fillEssentialFiles(true); // Force update of essential files
					if (verbose && !dialogMode) printf("glibc update mode\n");
				}
				//pData.setItemCurrentAction(install_list[i].itemID, _("Installed"));
				pData.setItemState(install_list[i].itemID, ITEMSTATE_FINISHED);
			}
			pkgInstallEndTime=time(NULL); // TIMER 2: Mark end of package installation
			pkgInstallTime.push_back(pkgInstallEndTime-pkgInstallStartTime); // Store data
			pkgInstallSize.push_back(atoi(install_list[i].get_installed_size().c_str())); // Store more data :)
			
			// Approximate by total stats time, not so good
			pkgTotalInstallTime+=(pkgInstallEndTime-pkgInstallStartTime); // Adding summary time
			pkgInstallCurrentSize += (uint64_t) atol(install_list[i].get_installed_size().c_str()); // Adding completed size;
		
			if (pkgTotalInstallTime!=0) pkgInstallSpeed=(long double) pkgInstallCurrentSize/(long double) pkgTotalInstallTime; // Update pkgInstal speed
			ETA_Time = (time_t) ((long double) (sz-pkgInstallCurrentSize)/pkgInstallSpeed);
			if (i==install_list.size()-1) {
				pData.setItemCurrentAction(install_list[i].itemID, string(_("Installation finished, doing post-install actions")));
			}


		}
		msay(_("Installation complete."), SAYMODE_INLINE_END);
	}

	pData.setCurrentAction(_("Updating system files"));
	if (removeFailures!=0 && installFailures!=0) return MPKGERROR_COMMITERROR;
	sqlFlush(); // Teh fuckin' trouble place
	
	// NEW 25.08.08: cleanup db for local packages
	msay(_("Clearing unreachable packages"), SAYMODE_NEWLINE);
	clear_unreachable_packages();
	
	if (install_list.size()>0 || remove_list.size()>0) 
	{
		msay(_("Executing ldconfig"), SAYMODE_NEWLINE);
		if (FileExists("/usr/sbin/prelink") && mConfig.getValue("enable_prelink")=="yes") {
			system("/sbin/ldconfig 2> /dev/null");
			if (mConfig.getValue("enable_prelink_randomization")=="yes") {
				msay(_("Prelinking with randomization..."), SAYMODE_NEWLINE);
				system("/usr/sbin/prelink -amqR 2>/dev/null");
			}
			else {
				msay(_("Prelinking..."), SAYMODE_NEWLINE);
				system("/usr/sbin/prelink -amq 2>/dev/null");
			}

		}
		else system("/sbin/ldconfig 2> /dev/null"); // I prefer to update ldconfig in real time, seems that delayed jobs works bad.
		if (needUpdateXFonts) {
			msay(_("Updating font indexes"), SAYMODE_NEWLINE);
			system("chroot " + SYS_ROOT + " find /usr/share/fonts -type d -exec /usr/bin/mkfontdir {} \\; ");
			system("chroot " + SYS_ROOT + " find /usr/share/fonts -type d -exec /usr/bin/mkfontscale {} \\; ");
			system("chroot " + SYS_ROOT + " /usr/bin/fc-cache -f");
		}
		printf("Total icon paths to update: %d\n", (int) iconCacheUpdates.size());
		for (size_t i=0; i<iconCacheUpdates.size(); ++i) {
			printf(_("[%d/%d] Updating icon cache in %s\n"), (int) i+1, (int) iconCacheUpdates.size(), iconCacheUpdates[i].c_str());
			system("chroot " + SYS_ROOT + " /usr/bin/gtk-update-icon-cache -t -f " + iconCacheUpdates[i] + " 1> /dev/null 2> /dev/null");
		}
		printf("Total GConf schemas to remove: %d\n", (int) gconfSchemasUninstall.size());
		for (size_t i=0; i<gconfSchemasUninstall.size(); ++i) {
			printf(_("[%d/%d] Removing GConf schema %s\n"), (int) i+1, (int) gconfSchemasUninstall.size(), gconfSchemasUninstall[i].c_str());
			system("chroot " + SYS_ROOT + " /usr/sbin/gconfpkg --uninstall " + gconfSchemasUninstall[i] + " 1> /dev/null 2> /dev/null");
		}
	
		printf("Total GConf schemas to install: %d\n", (int) gconfSchemas.size());
		for (size_t i=0; i<gconfSchemas.size(); ++i) {
			printf(_("[%d/%d] Installing GConf schema %s\n"), (int) i+1, (int) gconfSchemas.size(), gconfSchemas[i].c_str());
			system("chroot " + SYS_ROOT + " /usr/sbin/gconfpkg --install " + gconfSchemas[i] + " 1> /dev/null 2> /dev/null");
		}

		// Always update mime database, it takes not much time but prevents lots of troubles
		msay(_("Updating icon cache and mime database"), SAYMODE_NEWLINE);
		system("chroot " + SYS_ROOT + " /usr/bin/update-all-caches >/dev/null");
		
		// Cleanup
		system("rm -rf " + SYS_ROOT + "/install");
		
		// Hooks
		hookManager.runHooks();


		msay(_("Syncing disks..."), SAYMODE_NEWLINE);
		system("sync &");
	}

	//system("echo `date +\%d.\%m.\%Y\\ \%H:\%M:\%S` FINISHED >> /var/log/mpkg-installation.log");
	endTransaction(transaction_id, getSqlDb());
	return 0;
}

int mpkgDatabase::install_package(PACKAGE* package, size_t packageNum, size_t packagesTotal, int transaction_id)
{
	bool ultraFastMode = true;
#ifndef INSTALL_DEBUG
	if (setupMode && dialogMode) {
		system("echo Installing package " + package->get_name() + "-" + package->get_fullversion() + " >> /dev/tty4");
	}
	//system("echo `date +\%d.\%m.\%Y\\ \%H:\%M:\%S` Installing package " + package->get_name() + "-" + package->get_fullversion() + " >> /var/log/mpkg-installation.log");
#endif
	
	// Check if package already has been installed
	if (package->action()==ST_NONE) return 0;
	string sys_cache=SYS_CACHE;
	string sys_root=SYS_ROOT;
	string index_str, index_hole;
	if (packagesTotal>0) {
		index_str = "[" + IntToStr(packageNum) + "/"+IntToStr(packagesTotal)+"] ";
		for (size_t i=0; i<utf8strlen(index_str); ++i) {
			index_hole += " ";
		}
	}
	//if (!setupMode) pData.setItemCurrentAction(package->itemID, _("installing"));
	msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": initialization"), SAYMODE_INLINE_START);
	string statusHeader = _("Installing package ") + package->get_name()+": ";
	currentStatus = statusHeader + _("initialization");
	
	// NEW (04.10.2007): Check if package is source, and build if needed. Also, import to database the output binary package and prepare to install it.
	msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() +_( ": checking package type"));

	if (package->get_type()==PKGTYPE_SOURCE)
	{
		msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": building from source"));

		say(_("Package %s is source-based. Building...\n"), package->get_name().c_str());
		string binary_out;
		if (emerge_package(SYS_CACHE + package->get_filename(), &binary_out)!=0) {
			mError("Failed to build. Aborting...");
			return MPKG_INSTALL_META_ERROR;
		}
		say(_("Package was built. Filename: %s\n"), binary_out.c_str());
		// Now we have a new binary package with filename stored in variable binary_out. Import him into database and create a link to cache.
		
		if (!copyFile(binary_out, SYS_CACHE + getFilename(binary_out))) {
			mError("Error copying package, aborting...");
			return MPKG_INSTALL_META_ERROR;
		}
		say(_("Importing to database\n"));
		LocalPackage binpkg(SYS_CACHE + getFilename(binary_out));
		if (binpkg.injectFile()!=0) {
			mError("Error injecting binary package, cannot continue");
			return MPKG_INSTALL_META_ERROR;
		}
		PACKAGE binary_package = binpkg.data;
		binary_package.set_action(ST_INSTALL, "new-bin");
		emerge_to_db(&binary_package);
		binary_package.itemID = package->itemID;
		// Now replace record in install_list:
		*package = binary_package;
		say(_("Processing to install binary\n"));
		return install_package(package, packageNum, packagesTotal, transaction_id);
	}

	// First of all: EXTRACT file list and scripts!!!
	LocalPackage lp(SYS_CACHE + package->get_filename());
	if (forceInInstallMD5Check) {
		if (dialogMode) ncInterface.setProgressText(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + "\n" + index_hole + _("checking package integrity"));
		msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": checking package integrity"));

		string md5 = get_file_md5(SYS_CACHE + package->get_filename());
		if (package->get_md5() != md5) {
			mError(_("Error while installing package ") + package->get_name() + _(": MD5 checksum incorrect: got ") + md5 + _(", should be: ") + package->get_md5());
			return MPKG_INSTALL_META_ERROR;
		}
	}

	if (dialogMode) ncInterface.setProgressText(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + "\n" + index_hole + _("extracting metadata"));
	msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": extracting metadata"));


	int purge_id=0;
	if (_abortActions)
	{
		sqlFlush();
		return MPKGERROR_ABORTED;
	}
	
	pData.increaseItemProgress(package->itemID);
	if (!ultraFastMode) {
		if (dialogMode) ncInterface.setProgressText(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + "\n" + index_hole + _("extracting scripts")); 
		else msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": extracting scripts"));
		lp.fill_scripts(package);  // ultrafast mode: disabling scripts other than doinst.sh
	}

	currentStatus = statusHeader + _("extracting file list");
	printHtmlProgress();
	if (dialogMode) ncInterface.setProgressText(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + "\n" + index_hole + _("extracting file list"));
	else msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": extracting file list"));
	pData.increaseItemProgress(package->itemID);
	if (_abortActions) {
		sqlFlush();
		return MPKGERROR_ABORTED;
	}
#ifndef INSTALL_DEBUG
	if (package->get_files().empty()) {
		lp.fill_filelist(package); // Extracting file list
	}
#endif
	
	if (!needUpdateXFonts) {
		msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": looking for X fonts"));

		for (size_t i=0; !needUpdateXFonts && i<package->get_files().size(); i++) {
			if (package->get_files().at(i).find("usr/share/fonts")!=std::string::npos) needUpdateXFonts = true;
		}
	}
	// Searching for icon cache updates
	bool hasIconCache = false;
	string *iconFilename;
	string iconDir;
	for (size_t i=0; i<package->get_files().size(); ++i) {
		iconFilename = (string *) &package->get_files().at(i);
		if (iconFilename->find("usr/share/icons/")!=std::string::npos && iconFilename->size()>strlen("usr/share/icons/") && iconFilename->at(iconFilename->size()-1)=='/') {
			hasIconCache=false;
			iconDir = iconFilename->substr(strlen("usr/share/icons/"));
			if (iconDir.find_first_of("/")!=std::string::npos) iconDir = iconDir.substr(0, iconDir.find_first_of("/"));
			iconDir = "usr/share/icons/" + iconDir;
			for (size_t t=0; !hasIconCache && t<iconCacheUpdates.size(); ++t) {
				if (iconCacheUpdates[t]==iconDir) hasIconCache = true;
			}
			if (!hasIconCache) iconCacheUpdates.push_back(iconDir);
		}
	}

	// Searching for gconf schemas
	bool hasGConfSchema = false;
	string *gconfFilename;
	string gconfSchemaName;
	for (size_t i=0; i<package->get_files().size(); ++i) {
		gconfFilename = (string *) &package->get_files().at(i);
		if (gconfFilename->find("usr/share/gconf/schemas/")!=std::string::npos && gconfFilename->size()>strlen("usr/share/gconf/schemas/") && getExtension(*gconfFilename)=="schemas") {
			hasGConfSchema=false;
			gconfSchemaName = gconfFilename->substr(strlen("usr/share/gconf/schemas/"));
			gconfSchemaName = gconfSchemaName.substr(0, gconfSchemaName.size()-strlen(".schemas"));
			for (size_t t=0; !hasGConfSchema && t<gconfSchemas.size(); ++t) {
				if (gconfSchemas[t]==gconfSchemaName) hasGConfSchema = true;
			}
			if (!hasGConfSchema) gconfSchemas.push_back(gconfSchemaName);
		}
	}

	msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": checking file conflicts"));
	if (dialogMode) ncInterface.setProgressText(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() +"\n" + index_hole + _("checking file conflicts"));
	if (fileConflictChecking==CHECKFILES_PREINSTALL) 
	{

		currentStatus = statusHeader + _("checking file conflicts");
		printHtmlProgress();
		pData.increaseItemProgress(package->itemID);

	}
#ifdef INSTALL_DEBUG
	force_skip_conflictcheck=true;
#endif
	if (!force_skip_conflictcheck)
	{
		if (fileConflictChecking == CHECKFILES_PREINSTALL && check_file_conflicts_new(*package)!=0)
		{
			currentStatus = _("Error: Unresolved file conflict on package ") + package->get_name();
			mError(_("Unresolved file conflict on package ") + package->get_name() + _(", it will be skipped!"));
			return MPKG_INSTALL_FILE_CONFLICT;
		}
	}
	
	currentStatus = statusHeader + _("installing...");
	pData.increaseItemProgress(package->itemID);

	printHtmlProgress();

// Filtering file list...

	if (dialogMode) ncInterface.setProgressText(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() +"\n" + index_hole + _("merging file lists into database"));
	msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": merging file lists into database"));
#ifndef INSTALL_DEBUG
	add_filelist_record(package->get_id(), package->get_files_ptr());
#endif
	
	string sys;

	// Extracting package
	currentStatus = statusHeader + _("extracting...");
	if (dialogMode) ncInterface.setProgressText(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + "\n" + index_hole + _("extracting ") + IntToStr(package->get_files().size()) + _(" files"));
	msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": extracting ") + IntToStr(package->get_files().size()) + _(" files"));
	pData.increaseItemProgress(package->itemID);


	sys="(cd "+sys_root+" && tar xf '"+sys_cache + package->get_filename() + "'";
	//If previous version isn't purged, do not overwrite config files
	if (_cmdOptions["skip_doc_installation"]=="true") {
		sys += " --exclude usr/doc";
	}
	if (_cmdOptions["skip_man_installation"]=="true") {
		sys += " --exclude usr/man";
	}
	if (_cmdOptions["skip_dev_installation"]=="true") {
		sys += " --exclude usr/include";
	}
	if (_cmdOptions["skip_static_a_installation"]=="true") {
		sys += " --exclude 'lib/*.a' --exclude 'lib64/*.a'";
	}
	if (setupMode && dialogMode) sys+=" > /dev/tty4 2>/dev/tty4 )";
	else if (dialogMode) sys+=" >/dev/null 2>>/var/log/mpkg-errors.log )";
	else sys+= " )";

	if (!simulate) {
#ifdef INSTALL_DEBUG
		if (true)
#else
		if (system(sys.c_str()) == 0 /* || package->get_name()=="aaa_base"*/) // Somebody, TELL ME WHAT THE HELL IS THIS???! WHY SUCH EXCEPTION?!
#endif
		{
			if (ultraFastMode) {
				if (_cmdOptions["preseve_doinst"]=="true" && FileExists(SYS_ROOT + "/install/doinst.sh") ) system("mkdir -p " + package->get_scriptdir() + " && cp " + SYS_ROOT+"/install/doinst.sh " + package->get_scriptdir()); // Please note that this stuff will not work in real world.
				if (FileExists(SYS_ROOT + "/install/postremove.sh")) system("mkdir -p " + package->get_scriptdir() + " && mv " + SYS_ROOT+"/install/postremove.sh " + package->get_scriptdir());
				if (FileExists(SYS_ROOT + "/install/preremove.sh")) system("mkdir -p " + package->get_scriptdir() + " && mv " + SYS_ROOT+"/install/preremove.sh " + package->get_scriptdir());
			}
			printHtmlProgress();
			currentStatus = statusHeader + _("executing post-install scripts...");
		}
		else {
			
			printHtmlProgress();
			currentStatus = _("Failed to extract!");
			mError(_("Error while extracting package ") + package->get_name());
			return MPKG_INSTALL_EXTRACT_ERROR;
		}
	}

	pData.increaseItemProgress(package->itemID);

/*#ifdef INSTALL_DEBUG
	DO_NOT_RUN_SCRIPTS = true;
#endif*/

	// Managing config files
	pkgConfigInstall(*package);	

	hookManager.addInstalled(package);
	package->get_files_ptr()->clear();
	// Creating and running POST-INSTALL script
	if (!DO_NOT_RUN_SCRIPTS)
	{
		msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": executing post-install script"));
		if (dialogMode) ncInterface.setProgressText(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + "\n" + index_hole + _("executing post-install script"));
		//if (FileExists(package->get_scriptdir() + "doinst.sh"))
		if (FileExists(SYS_ROOT + "/install/doinst.sh"))
		{
			string postinst;
			string tmpdoinst = "/tmp/mpkgtmp_" + package->get_name() + ".sh";
			add_tmp_file(SYS_ROOT + "/" + tmpdoinst);
			system("mv " + SYS_ROOT + "/install/doinst.sh " + SYS_ROOT + "/" + tmpdoinst);
			postinst="cd " + SYS_ROOT + " && bash " + SYS_ROOT + "/" + tmpdoinst; // New fast mode: we don't care much about script run ordering, and parallel run is MUCH faster.
			if (setupMode && dialogMode) postinst += " 2>/dev/tty4 >/dev/tty4";
			else if (dialogMode) postinst += " 2>/dev/null > /dev/null";
			//cout << "Running: '" << postinst << endl;
			system_threaded(postinst);
			//system(postinst);
		}
	}

	if (dialogMode) ncInterface.setProgressText(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + "\n" + index_hole + _("finishing installation"));
	msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": finishing installation"));
#ifndef INSTALL_DEBUG

	set_installed(package->get_id(), ST_INSTALLED);
	set_configexist(package->get_id(), ST_CONFIGEXIST);
	set_action(package->get_id(), ST_NONE, package->package_action_reason);
#endif
	if (purge_id!=0){
		set_configexist(purge_id, ST_CONFIGNOTEXIST); // Clear old purge status
		cleanFileList(purge_id);
	}
	msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": updating database"));
	if (_cmdOptions["warpmode"]!="yes") sqlFlush();
	msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": exporting legacy data"));
	pData.increaseItemProgress(package->itemID);
	msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": complete"), SAYMODE_INLINE_END);
	if (package->action()==ST_REPAIR) recordPackageTransaction(*package, 1, transaction_id, getSqlDb());
	package->set_action(ST_NONE, "install_complete");
	
	recordPackageTransaction(*package, 0, transaction_id, getSqlDb());
	return 0;
}	//End of install_package

int mpkgDatabase::remove_package(PACKAGE* package, size_t packageNum, size_t packagesTotal, int transaction_id)
{
	//system("echo `date +\%d.\%m.\%Y\\ \%H:\%M:\%S`  Removing package " + package->get_name() + "-" + package->get_fullversion() + " >> /var/log/mpkg-installation.log");
	string index_str, action_str, by_str;
	if (packagesTotal>0) {
		index_str = "[" + IntToStr(packageNum) + "/"+IntToStr(packagesTotal)+"] ";
	}
	bool needSpecialUpdate = false;
	bool dontRemove = false;
	vector<string> unremovable = ReadFileStrings("/etc/mpkg-unremovable"); // List of packages, which will NEVER be removed physically
	for (size_t i=0; i<unremovable.size(); ++i) {
		if (package->get_name() == unremovable[i]) {
//			msay("Package " + package->get_name() + " is unremovable, so it's files will not be deleted");
			dontRemove = true;
			break;
		}
	}
	if (package->get_name()=="glibc" || package->get_name()=="glibc-solibs" || package->get_name()=="aaa_elflibs" || package->get_name()=="tar" || package->get_name()=="xz" || package->get_name()=="aaa_base" || package->get_name()=="gzip") {
		dontRemove = true;
		needSpecialUpdate = true;
	}

	if (package->action()==ST_UPDATE) {
		action_str=_("Updating");
		by_str = _(" ==> ") + package->updatingBy->get_fullversion();
		if (package->needSpecialUpdate || package->updatingBy->needSpecialUpdate) {
			msay("Package " + package->get_name() + " needs a special update method");
			needSpecialUpdate = true;
		}
		if (needSpecialUpdate || package->isTaggedBy("base") || package->updatingBy->isTaggedBy("base")) needSpecialUpdate=true;
		if (needSpecialUpdate || package->get_name().find("aaa_")==0 || package->get_name().find("glibc")==0) needSpecialUpdate=true;
		if (needSpecialUpdate || package->get_name().find("tar")==0 || package->get_name().find("coreutils")==0 || package->get_name().find("sed")==0 || package->get_name().find("bash")==0 || \
				package->get_name().find("grep")==0 || package->get_name().find("gzip")==0 || package->get_name().find("which")==0) needSpecialUpdate=true;
	}
	if (package->action()==ST_REMOVE) action_str = _("Removing");
	if (package->action()==ST_PURGE) action_str = _("Purging");
	if (mConfig.getValue("always_special_update")=="yes") needSpecialUpdate = true; // Transitional purposes
	get_filelist(package->get_id(), package->get_files_ptr());
	pData.setItemProgressMaximum(package->itemID, package->get_files().size()+8);

	pData.setItemCurrentAction(package->itemID, action_str);
	msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str, SAYMODE_INLINE_START);

	string statusHeader = _("Removing package ") + package->get_name()+": ";
	currentStatus = statusHeader + _("initialization");
	
	printHtmlProgress();
	if (package->action()==ST_REMOVE || package->action()==ST_PURGE || package->action()==ST_UPDATE)
	{
		// Checking if package is updating, if so, get the files of new package already
		if (package->action()==ST_UPDATE) {
			msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str + _(": extracting file list"));
			LocalPackage *lp = new LocalPackage(SYS_CACHE + package->updatingBy->get_filename());
			lp->fill_filelist(package->updatingBy);
			delete lp;
		}


		// Running pre-remove scripts
		//printf("Processing\n");
		if(!DO_NOT_RUN_SCRIPTS)
		{
			if (FileExists(package->get_scriptdir() + "preremove.sh"))
			{
				msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str + _(": executing pre-remove script"));
				printHtmlProgress();
				currentStatus = statusHeader + _("executing pre-remove scripts");
				string prerem="cd " + SYS_ROOT + " ; sh "+package->get_scriptdir() + "preremove.sh";
				if (!simulate) system(prerem.c_str());
			}
		}
		
		pData.increaseItemProgress(package->itemID);
		
		// removing package
		string sys_cache=SYS_CACHE;
		string sys_root=SYS_ROOT;
		string fname;

		// Checking backups
		msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str + _(": checking for backups"));

		vector<FILE_EXTENDED_DATA> backups;
		vector<string> backups_filenames;
		get_backup_records(*package, &backups_filenames, &backups);



		// Purge is now implemented here; checking all
		currentStatus = statusHeader + _("building file list");
		vector<string> *remove_files = package->get_files_ptr(); // Note: no need to delete remove_files, because it will be deleted together with package object
		vector<string> *new_files = package->updatingBy->get_files_ptr();

		// Check for special procedures
		if (!needUpdateXFonts) {
			msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": looking for X fonts"));

			for (size_t i=0; !needUpdateXFonts && i<package->get_files().size(); i++) {
				if (package->get_files().at(i).find("usr/share/fonts")!=std::string::npos) needUpdateXFonts = true;
			}
		}
		// Searching for icon cache updates
		bool hasIconCache = false;
		string *iconFilename;
		string iconDir;
		for (size_t i=0; i<package->get_files().size(); ++i) {
			iconFilename = (string *) &package->get_files().at(i);
			if (iconFilename->find("usr/share/icons/")!=std::string::npos && iconFilename->size()>strlen("usr/share/icons/") && iconFilename->at(iconFilename->size()-1)=='/') {
				hasIconCache=false;
				iconDir = iconFilename->substr(strlen("usr/share/icons/"));
				if (iconDir.find_first_of("/")!=std::string::npos) iconDir = iconDir.substr(0, iconDir.find_first_of("/"));
				iconDir = "usr/share/icons/" + iconDir;
				for (size_t t=0; !hasIconCache && t<iconCacheUpdates.size(); ++t) {
					if (iconCacheUpdates[t]==iconDir) hasIconCache = true;
				}
				if (!hasIconCache) iconCacheUpdates.push_back(iconDir);
			}
		}

		// Searching for gconf schemas
		bool hasGConfSchema = false;
		string *gconfFilename;
		string gconfSchemaName;
		for (size_t i=0; i<package->get_files().size(); ++i) {
			gconfFilename = (string *) &package->get_files().at(i);
			if (gconfFilename->find("usr/share/gconf/schemas/")!=std::string::npos && gconfFilename->size()>strlen("usr/share/gconf/schemas/") && getExtension(*gconfFilename)=="schemas") {
				hasGConfSchema=false;
				gconfSchemaName = gconfFilename->substr(strlen("usr/share/gconf/schemas/"));
				gconfSchemaName = gconfSchemaName.substr(0, gconfSchemaName.size()-strlen(".schemas"));
				for (size_t t=0; !hasGConfSchema && t<gconfSchemasUninstall.size(); ++t) {
					if (gconfSchemasUninstall[t]==gconfSchemaName) hasGConfSchema = true;
				}
				if (!hasGConfSchema) gconfSchemasUninstall.push_back(gconfSchemaName);
			}
		}

		printHtmlProgress();
		currentStatus = statusHeader + _("removing files...");
		bool removeThis;
		int unlink_ret;
		for (size_t i=0; i<remove_files->size(); ++i) {
			if (i==0 || i==remove_files->size() || i%20==0) msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str + _(": removing files [") + IntToStr(i) + "/"+IntToStr(remove_files->size())+"]");
			fname=sys_root + "/" + remove_files->at(i);
			for (size_t t=0; t<backups.size(); ++t) {
				if (remove_files->at(i)==*backups[t].filename) {
					fname = (string) SYS_BACKUP+"/"+backups[t].backup_file;
					delete_conflict_record(backups[t].overwriter_id, backups[t].backup_file);
					break;
				}
			}
		

			// Checking for: 
			// - if file is a configuration file
			// - if file will be overwritten by new package
			//
			
			removeThis = false;
			
			if (package->action()!=ST_UPDATE) {
				removeThis = true;
			}
			else {
				removeThis=true;
				if (needSpecialUpdate) {
					for (size_t t=0; removeThis && t<new_files->size(); ++t) {
						if (new_files->at(t)==remove_files->at(i)) {
							removeThis=false;
							break;
						}
					}
				}
			}
			if (checkEssentialFile(remove_files->at(i))) removeThis=false; 
			// Actually removing files
			if (removeThis && fname[fname.length()-1]!='/')
			{
				if (!simulate && !dontRemove) {
					if (verbose && !dialogMode) say("[%d] %s %s: ", (size_t) i, _("Removing file"), fname.c_str());
					unlink_ret = unlink(fname.c_str());
					if (verbose && !dialogMode) {
						if (unlink_ret==0) say("%sOK%s\n", CL_GREEN, CL_WHITE);
						else say(_("%sFAILED%s\n"), CL_RED, CL_WHITE);
					}
				}
			}
			// DISABLED FOR SPEED DEBUG
			//pData.increaseItemProgress(package->itemID);
		}
		msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str+_(": removing empty directories"));

		printHtmlProgress();
		currentStatus = statusHeader + _("removing empty directories...");
	
		// Run 2: clearing empty directories
		vector<string>empty_dirs;
		string edir;
		
		pData.increaseItemProgress(package->itemID);
		
		for (size_t i=0; i<remove_files->size(); ++i) {
			fname=sys_root + "/" + remove_files->at(i);
			for (size_t d=0; d<fname.length(); ++d)	{
				edir+=fname[d];
				if (fname[d]=='/') {
					empty_dirs.resize(empty_dirs.size()+1);
					empty_dirs[empty_dirs.size()-1]=edir;
				}
			}

			for (int x=empty_dirs.size()-1; x>=0; x--) {
				if (!simulate && !dontRemove) {
					unlink_ret = rmdir(empty_dirs[x].c_str());
					if (verbose) {
						if (unlink_ret == 0) say("[%d] %s %s\n", i, _("Removing empty directory"), fname.c_str());
					}

				}
			}
			edir.clear();
			empty_dirs.clear();
		}
	
		printHtmlProgress();
		// Creating and running POST-REMOVE script
		if (!DO_NOT_RUN_SCRIPTS)
		{
			if (FileExists(package->get_scriptdir() + "postremove.sh"))
			{
				msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str+_(": executing post-remove script"));
				currentStatus = statusHeader + _("executing post-remove scripts");
				string postrem="cd " + SYS_ROOT + " ; sh " + package->get_scriptdir() + "postremove.sh";
				if (!simulate) system(postrem.c_str());
			}
		}
		// Clearing scripts
		msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str + _(": clearing script directory"));
		system("rm -rf " + package->get_scriptdir() + " 2>/dev/null >/dev/null");
	
		// Restoring backups. NOTE: if there is an updating package, manage backups anyway. TODO: minimize file operation by processing more data
		
		msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str+_(": restoring backups"));

		printHtmlProgress();
		
		// Creating restore lists. Note that both objects are linked inside.
		vector<string> restore_fnames;
		vector<FILE_EXTENDED_DATA> restore;

		get_conflict_records(package->get_id(), &restore_fnames, &restore);
		int ret;
		if (!restore.empty()) {
			string cmd;
			string tmpName;
			for (size_t i=0; i<restore.size(); ++i) {
				if (restore[i].filename->find_last_of("/")!=std::string::npos)	{
					cmd = "mkdir -p ";
					cmd += SYS_ROOT + "/" + restore[i].filename->substr(0, restore[i].filename->find_last_of("/")) + " 2>/dev/null >/dev/null";
					if (!simulate) system(cmd.c_str());
				}
				cmd = "mv ";
			        cmd += SYS_BACKUP+restore[i].backup_file + " ";
				tmpName = restore[i].backup_file.substr(SYS_BACKUP.length());
				tmpName = tmpName.substr(tmpName.find("/"));
			        cmd += SYS_ROOT + "/" + tmpName.substr(0,tmpName.find_last_of("/"))+"/ 2>/dev/null >/dev/null";
				if (!simulate) ret = system(cmd);
				if (verbose) cout << "RESTORE: " << cmd << endl;
				delete_conflict_record(package->get_id(), restore[i].backup_file);
			}
		}
		
		// Calling remove for package configs
		pkgConfigRemove(*package);

		msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str + _(": finishing"));
		pData.increaseItemProgress(package->itemID);
		set_installed(package->get_id(), ST_NOTINSTALLED);
		if (package->action()==ST_PURGE) set_configexist(package->get_id(), 0);
		set_action(package->get_id(), ST_NONE, package->package_action_reason);
		currentStatus = statusHeader + _("cleaning file list");
		pData.increaseItemProgress(package->itemID);
		cleanFileList(package->get_id());
		pData.increaseItemProgress(package->itemID);
		msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str + _(": updating database"));

		if (_cmdOptions["warpmode"]!="yes") sqlFlush();
		currentStatus = statusHeader + _("remove complete");
		hookManager.addRemoved(package);
		package->get_files_ptr()->clear();
		pData.setItemProgress(package->itemID, pData.getItemProgressMaximum(package->itemID));
		if (package->action()==ST_UPDATE) msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str + _(": done"));
		else msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str+": done", SAYMODE_INLINE_END);
		
		printHtmlProgress();
		recordPackageTransaction(*package, 1, transaction_id, getSqlDb());
		// If update: run install now (for fault tolerance)
		if (package->action()==ST_UPDATE) {
			return install_package(package->updatingBy, packageNum, packagesTotal, transaction_id);
		}
		return 0;
	}
	else
	{
		mError(_("Weird status of package, i'm afraid to remove this..."));
		return -1;
	}
}	// End of remove_package

int mpkgDatabase::delete_packages(PACKAGE_LIST *pkgList)
{
	if (pkgList->IsEmpty())
	{
		return 0;
	}
	SQLRecord sqlSearch;
	sqlSearch.setSearchMode(SEARCH_IN);
	for(size_t i=0; i<pkgList->size(); ++i) {
		sqlSearch.addField("package_id", pkgList->at(i).get_id());
	}
	db.sql_delete("packages", sqlSearch);
	sqlSearch.clear();
	for(size_t i=0; i<pkgList->size(); ++i) {
		sqlSearch.addField("packages_package_id", pkgList->at(i).get_id());
	}
	db.sql_delete("tags_links", sqlSearch);
	db.sql_delete("dependencies", sqlSearch);
	db.sql_delete("deltas", sqlSearch);
#ifdef ENABLE_INTERNATIONAL
	db.sql_delete("descriptions", sqlSearch);
	db.sql_delete("changelogs", sqlSearch);
	db.sql_delete("ratings", sqlSearch);
#endif

	// Removing unused tags
	sqlSearch.clear();
	SQLTable available_tags;
	SQLRecord fields;

	db.get_sql_vtable(available_tags, sqlSearch, "tags", fields);
	SQLTable used_tags;
	db.get_sql_vtable(used_tags, sqlSearch, "tags_links", fields);
	vector<string> toDelete;
	bool used;

	// Index
	//int fAvailTags_name = available_tags.getFieldIndex("tags_name");
	int fAvailTags_id = available_tags.getFieldIndex("tags_id");
	int fUsedTags_tag_id = used_tags.getFieldIndex("tags_tag_id");
	if (available_tags.size()>0) {
		for(size_t i=0; i<available_tags.size(); ++i) {
			used=false;
			for (size_t u=0; u<used_tags.size(); ++u) {
				if (used_tags.getValue(u, fUsedTags_tag_id)==available_tags.getValue(i, fAvailTags_id)) {
					used=true;
				}
			}
			if (!used) {
				//say(_("Deleting tag %s as unused\n"), available_tags.getValue(i, fAvailTags_name).c_str());
				toDelete.push_back(available_tags.getValue(i,"tags_id"));
			}
		}
		available_tags.clear();
		sqlSearch.clear();
		sqlSearch.setSearchMode(SEARCH_IN);
		if (toDelete.size()>0) {
			for (size_t i=0; i<toDelete.size(); ++i) {
				sqlSearch.addField("tags_id", toDelete[i]);
			}
			db.sql_delete("tags", sqlSearch);
		}
	}
	return 0;
}



int mpkgDatabase::cleanFileList(int package_id) {
	SQLRecord sqlSearch;
	sqlSearch.addField("packages_package_id", package_id);
	int ret = db.sql_delete("files", sqlSearch);
	if (ret!=0) return ret;
	string pkg_id = IntToStr(package_id);
	if (pkg_id.empty()) return -1;
	db.sql_exec("DELETE FROM config_options WHERE config_files_id IN (SELECT id FROM config_files WHERE package_id=" + pkg_id + ");");
	db.sql_exec("DELETE FROM config_files WHERE package_id=" + pkg_id + ";");

	set_configexist(package_id,0);
	return ret;
}

int mpkgDatabase::update_package_data(int package_id, PACKAGE *package)
{
	PACKAGE old_package;
	if (get_package(package_id, &old_package)!=0)
	{
		return -1;
	}
	
	SQLRecord sqlUpdate;
	SQLRecord sqlSearch;
	sqlSearch.addField("package_id", package_id);

	// 1. Updating direct package data
	if (package->get_md5()!=old_package.get_md5())
	{
		sqlUpdate.addField("package_description", package->get_description());
		sqlUpdate.addField("package_short_description", package->get_short_description());
		sqlUpdate.addField("package_compressed_size", package->get_compressed_size());
		sqlUpdate.addField("package_installed_size", package->get_installed_size());
		sqlUpdate.addField("package_changelog", package->get_changelog());
		sqlUpdate.addField("package_packager", package->get_packager());
		sqlUpdate.addField("package_packager_email", package->get_packager_email());
		sqlUpdate.addField("package_md5", package->get_md5());
	}

	// 2. Updating filename
	if (package->get_filename()!=old_package.get_filename())
	{
		sqlUpdate.addField("package_filename", package->get_filename());
	}

	// 3. Updating status. Seems that somewhere here is an error causing double scan is required
	sqlUpdate.addField("package_available", package->available());


	// 4. Updating locations
	
	// Note about method used for updating locations:
	// If locations are not identical:
	// 	Step 1. Remove all existing package locations from "locations" table. Servers are untouched.
	// 	Step 2. Add new locations.
	// Note: after end of updating procedure for all packages, it will be good to do servers cleanup - delete all servers who has no locations.
	//"mpkg.cpp: update_package_data(): checking locations"	
	if (!package->locationsEqualTo(old_package))
	{
		SQLRecord loc_sqlDelete;
		loc_sqlDelete.addField("packages_package_id", package_id);
		int sql_del=db.sql_delete("locations", loc_sqlDelete);
		if (sql_del!=0)
		{
			return -2;
		}
		if (add_locationlist_record(package_id, package->get_locations_ptr())!=0)
		{
			return -3;
		}
		if (package->get_locations().empty())
		{
			sqlUpdate.addField("package_available", ST_NOTAVAILABLE);
		}
	}

	// 5. Updating tags
	if (!package->tagsEqualTo(old_package))
	{
		SQLRecord taglink_sqlDelete;
		taglink_sqlDelete.addField("packages_package_id", package_id);

		if (db.sql_delete("tags_links", taglink_sqlDelete)!=0) return -4;
		if (add_taglist_record(package_id, package->get_tags_ptr())!=0) return -5;
	}

	// 6. Updating dependencies
	if (!package->depsEqualTo(old_package))
	{
		SQLRecord dep_sqlDelete;
		dep_sqlDelete.addField("packages_package_id", package_id);

		if(db.sql_delete("dependencies", dep_sqlDelete)!=0) return -6;
		if (add_dependencylist_record(package_id, package->get_dependencies_ptr())!=0) return -7;
	}

	// 7, 8 - update scripts and file list. It is skipped for now, because we don't need this here (at least, for now).
	if (!sqlUpdate.empty())
	{
		if (db.sql_update("packages", sqlUpdate, sqlSearch)!=0)
		{
			return -8;
		}
	}
	return 0;
}

bool compareLocations(const vector<LOCATION>& location1, const vector<LOCATION>& location2) {
	if (location1.size()!=location2.size()) return false;
	// Note: **NOT** ignoring order
	for (size_t i=0; i<location1.size(); ++i) {
		if (!location1[i].equalTo(location2[i])) return false;
	}
	return true;
}


int mpkgDatabase::updateRepositoryData(PACKAGE_LIST *newPackages)
{
	if (verbose) printf("Update...\n");
	// Одна из самых страшных функций в этой программе.
	// Попытаемся применить принципиально новый алгоритм.
	// 
	// Для этого введем некоторые тезисы:
	// 1. Пакет однозначно идентифицируется по его контрольной сумме.
	// 2. Пакет без контрольной суммы - не пакет а мусор, выкидываем такое нахрен.
	// 
	// Алгоритм:
	// 1. Стираем все записи о locations и servers в базе.
	// 2. Забираем из базы весь список пакетов. Этот список и будет рабочим.
	// 3. Для каждого пакета из нового списка ищем соответствие в старой базе.
	// 	3а. В случае если такое соответствие найдено, вписываем в список записи о locations. Остальное остается неизменным, ибо MD5 та же.
	// 	3б. В случае если соответствие не найдено, пакет добавляется в конец рабочего списка с флагом new = true
	// 4. Вызывается синхронизация рабочего списка и данных в базе (это уже отдельная тема).
	//
	// ////////////////////////////////////////////////
	//"Retrieving current package list, clearing tables"
	// Стираем locations и servers
	
	if (verbose) printf("Pre-cleanup...\n");
	db.clear_table("locations");
	db.clear_table("deltas");
	db.clear_table("abuilds");
	// In case of repository bugs, full resync may be needed.
	if (forceFullDBUpdate) {
		db.clear_table("dependencies");
		db.clear_table("tags_links");
		db.clear_table("config_files");
		db.clear_table("config_options");
	}
	

	if (verbose) printf("Data load...\n");
	// Забираем текущий список пакетов
	PACKAGE_LIST *pkgList = new PACKAGE_LIST;
	SQLRecord sqlSearch;
	//printf("SLOW GET_PACKAGELIST CALL: %s %d\n", __func__, __LINE__);
	get_packagelist(sqlSearch, pkgList, false, true);
	
	if (verbose) printf("Scanning for changes...\n");
	//say("Merging data\n");
	// Ищем соответствия // TODO: надо б тут что-нить прооптимизировать
	int pkgNumber;
	size_t new_pkgs=0;
	vector<bool> needUpdateRepositoryTags;
	vector<bool> needUpdateDistroVersion;
	for(size_t i=0; i<newPackages->size(); ++i) {
		pkgNumber = pkgList->getPackageNumberByMD5(newPackages->at(i).get_md5());
		
		if (pkgNumber!=-1)	// Если соответствие найдено...
		{
			pkgList->get_package_ptr(pkgNumber)->set_locations(newPackages->at(i).get_locations());	// Записываем locations
			pkgList->get_package_ptr(pkgNumber)->deltaSources=newPackages->at(i).deltaSources; // TEMP DISABLED
			pkgList->get_package_ptr(pkgNumber)->abuild_url=newPackages->at(i).abuild_url; // One ABUILD record will be enough.
			
			if (pkgList->get_package_ptr(pkgNumber)->get_repository_tags()!=newPackages->at(i).get_repository_tags()) {
				pkgList->get_package_ptr(pkgNumber)->set_repository_tags(newPackages->at(i).get_repository_tags());
				needUpdateRepositoryTags.push_back(true);
			}
			else needUpdateRepositoryTags.push_back(false);
			
			if (strcmp(pkgList->get_package_ptr(pkgNumber)->package_distro_version.c_str(), newPackages->at(i).package_distro_version.c_str())==0) {
				pkgList->get_package_ptr(pkgNumber)->package_distro_version = newPackages->at(i).package_distro_version;
				needUpdateDistroVersion.push_back(true);
			}
			else needUpdateDistroVersion.push_back(false);
			
			pkgList->get_package_ptr(pkgNumber)->set_dependencies(newPackages->at(i).get_dependencies());
			pkgList->get_package_ptr(pkgNumber)->set_tags(newPackages->at(i).get_tags());

			if (forceFullDBUpdate) pkgList->get_package_ptr(pkgNumber)->config_files=newPackages->at(i).config_files;
			//db.sql_exec("DELETE FROM dependencies WHERE packages_package_id='" + IntToStr(pkgList->get_package_ptr(pkgNumber)->get_id()) + "';");
		}
		else			// Если соответствие НЕ найдено...
		{
			newPackages->get_package_ptr(i)->newPackage=true;
			pkgList->add(newPackages->at(i));
			needUpdateRepositoryTags.push_back(true);
			needUpdateDistroVersion.push_back(true);
			new_pkgs++;
		}
	}

	//say("Clean up...\n");
	// Вызываем синхронизацию данных.
	// Вообще говоря, ее можно было бы делать прямо здесь, но пусть таки будет универсальность.
	delete newPackages;//->clear();
	syncronize_data(pkgList, needUpdateRepositoryTags, needUpdateDistroVersion);
	if (!dialogMode && new_pkgs>0) say(_("New packages in repositories: %d\n"), (size_t) new_pkgs);
	return 0;
}
int mpkgDatabase::syncronize_data(PACKAGE_LIST *pkgList, vector<bool> needUpdateRepositoryTags, vector<bool> needUpdateDistroVersion)
{
	if (verbose) printf("Sync...\n");
	// Идея:
	// Добавить в базу пакеты, у которых флаг newPackage
	// Добавить locations к тем пакетам, которые такого флага не имеют
	// 
	// Алгоритм:
	// Бежим по списку пакетов.
	// 	Если пакет имеет влаг newPackage, то сразу добавляем его в базу функцией add_package_record()
	//	Если флага нету, то сразу добавляем ему locations функцией add_locationlist_record()
	// 
	
	SQLRecord *sqlUpdate = NULL;
	for(size_t i=0; i<pkgList->size(); i++)
	{
		if (pkgList->at(i).newPackage) {
			add_package_record(pkgList->get_package_ptr(i));
		}
		else {
			add_locationlist_record(pkgList->at(i).get_id(), pkgList->get_package_ptr(i)->get_locations_ptr());
			if (!pkgList->at(i).deltaSources.empty()) add_delta_record(pkgList->at(i).get_id(), pkgList->at(i).deltaSources);
			if (!pkgList->at(i).abuild_url.empty()) add_abuild_record(pkgList->at(i).get_id(), pkgList->at(i).abuild_url);
			if (needUpdateDistroVersion[i] || needUpdateRepositoryTags[i]) {
				sqlUpdate = new SQLRecord;
				if (needUpdateRepositoryTags[i]) sqlUpdate->addField("package_repository_tags", pkgList->at(i).get_repository_tags());
				if (needUpdateDistroVersion[i]) sqlUpdate->addField("package_distro_version", pkgList->at(i).package_distro_version);
				update_package_record(pkgList->at(i).get_id(), *sqlUpdate);
				delete sqlUpdate;
			}
			if (forceFullDBUpdate) {
				add_dependencylist_record(pkgList->at(i).get_id(), pkgList->get_package_ptr(i)->get_dependencies_ptr());
				add_taglist_record(pkgList->get_package_ptr(i)->get_id(), pkgList->get_package_ptr(i)->get_tags_ptr());
				add_configfiles_record(pkgList->get_package_ptr(i)->get_id(), pkgList->get_package_ptr(i)->config_files);
			}
		}
	}
	delete pkgList;

	if (verbose) printf("Cleanup...\n");
	// Дополнение от 10 мая 2007 года: сносим нафиг все недоступные пакеты, которые не установлены. Нечего им болтаться в базе.
	clear_unreachable_packages();
	if (verbose) printf("Done.\n");
	return 0;

}

int mpkgDatabase::clear_unreachable_packages() {
	PACKAGE_LIST *allList = new PACKAGE_LIST;
	SQLRecord sqlSearch;
	sqlSearch.addField("package_installed", ST_NOTINSTALLED);
	sqlSearch.addField("package_configexist", ST_CONFIGNOTEXIST);
	get_packagelist(sqlSearch, allList, true, false);
	PACKAGE_LIST deleteQueue;
	unsigned int rm_pkgs = 0;
	for(size_t i=0; i<allList->size(); ++i) {
		if (allList->at(i).installed()) continue;
		if (!allList->at(i).reachable(true)) {
			deleteQueue.add(allList->at(i));
		}
		else {
			if (allList->at(i).available(false)!=allList->at(i).available(true)) {
				if (!allList->at(i).configexist()) {
					deleteQueue.add(allList->at(i));
					rm_pkgs++;
				}
			}
		}
	}
	if (!deleteQueue.IsEmpty()) delete_packages(&deleteQueue);
	delete allList;
	if (!dialogMode && rm_pkgs>0) say(_("Packages removed from repositories: %d\n"),rm_pkgs);
	return 0;

}


HookManager::HookManager() {
}

HookManager::~HookManager() {
}

void HookManager::addInstalled(PACKAGE *pkg) {
	pkgInstalled.push_back(pkg->get_name());
	FILE *f = fopen(string(SYS_ROOT + "/var/mpkg/last_installed_files").c_str(), "a");
	if (!f) return;
	for (size_t i=0; i<pkg->get_files().size(); ++i) {
		fprintf(f, "%s\n", pkg->get_files().at(i).c_str());
	}
	fclose(f);
}

void HookManager::addRemoved(PACKAGE *pkg) {
	pkgRemoved.push_back(pkg->get_name());
	FILE *f = fopen(string(SYS_ROOT + "/var/mpkg/last_removed_files").c_str(), "a");
	if (!f) return;
	for (size_t i=0; i<pkg->get_files().size(); ++i) {
		fprintf(f, "%s\n", pkg->get_files().at(i).c_str());
	}
	fclose(f);

}

void HookManager::runHooks() {
	if (!FileExists(SYS_ROOT + "/etc/mpkg/hooks")) return;
	string pkgInstalledStr, pkgRemovedStr, pkgUpdatedStr;
	for (size_t i=0; i<pkgInstalled.size(); ++i) {
		pkgInstalledStr = pkgInstalledStr + pkgInstalled[i];
		if (i<pkgInstalled.size()-1) pkgInstalledStr+= " ";
	}
	for (size_t i=0; i<pkgRemoved.size(); ++i) {
		pkgRemovedStr = pkgRemovedStr + pkgRemoved[i];
		if (i<pkgRemoved.size()-1) pkgRemovedStr+= " ";
	}

	for (size_t i=0; i<pkgInstalled.size(); ++i) {
		for (size_t t=0; t<pkgRemoved.size(); ++t) {
			if (pkgInstalled[i]!=pkgRemoved[t]) continue;
			if (!pkgUpdatedStr.empty()) pkgUpdatedStr+= " ";
			pkgUpdatedStr = pkgUpdatedStr + pkgInstalled[i];
		}
	}
	printf(_("Executing post-install hooks...\n"));
	system("( cd " + SYS_ROOT + " ; for i in `find etc/mpkg/hooks -name '*.sh' -type f -perm 755` ; do echo " + string(_("Starting")) + " /$i ; PKG_INSTALLED='" + pkgInstalledStr + "' PKG_REMOVED='" + pkgRemovedStr + "' PKG_UPDATED='" + pkgUpdatedStr + "' $i ; echo /$i " + string(_("finished")) + " ; done )");
	printf(_("Post-install hooks completed\n"));


}

void HookManager::reset() {
	pkgInstalled.clear();
	pkgRemoved.clear();
	unlink(string(SYS_ROOT + "/var/mpkg/last_installed_files").c_str());
	unlink(string(SYS_ROOT + "/var/mpkg/last_removed_files").c_str());
	
}

