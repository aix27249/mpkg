#include "menu.h"
#include "terminal.h"
#define MENUITEM(z,y) menuItems.push_back(MenuItem(z,y))
#define MENUSPACE menuItems.push_back(MenuItem(" ", " "))
#define MENUCASE(z) if (ret==z)
void showMainMenu(mpkg &core) {
	dialogMode=true;
	vector<MenuItem> menuItems;
	
	MENUITEM("ACT_PACKAGEMENU", _("Package menu")); // TODO: improve, now: OK, actPackageMenu	
	MENUITEM("ACT_LATESTUPDATES", _("Show latest updates")); // OK
	MENUSPACE;
	MENUITEM("ACT_UPDATE", _("Update repository data")); // OK
	MENUITEM("ACT_LIST_REP", _("View/edit repositories")); // OK
	MENUITEM("ACT_GETONLINEREPOSITORYLIST", _("Get repository list from server")); // OK
	MENUSPACE;
	MENUITEM("ACT_SEARCH", _("Search packages by name")); // OK (needs improvements to UI)
	MENUITEM("ACT_SEARCHBYDESCRIPTION", _("Search package by description")); // OK	
	MENUITEM("ACT_FILESEARCH", _("Search file in packages by pattern")); // OK
	MENUITEM("ACT_WHICH", _("Search exact file in packages")); // OK
	MENUITEM("ACT_LISTDEPENDANTS", _("List packages who depends on selected")); // OK
	MENUSPACE;
	MENUITEM("ACT_INSTALLFROMLIST", _("Install packages from list")); // Ok	
	MENUITEM("ACT_EXPORTINSTALLED", _("Export list of installed packages to file")); // OK
	MENUSPACE;
	MENUITEM("ACT_SHOWQUEUE", _("Show queued actions")); // OK
	MENUITEM("ACT_RESETQUEUE", _("Reset action queue")); // OK
	MENUITEM("ACT_COMMIT", _("Commit queued actions")); // OK
	MENUSPACE;
	MENUITEM("ACT_CLEAN", _("Clean package cache")); // OK, actClean
	MENUSPACE;
	MENUITEM("ACT_INDEX", _("Build repository index")); // OK, actIndex
	MENUITEM("ACT_CONVERT", _("Convert packages to MPKG format")); //OK here, but requires fixes in core - in real it doesn't work.
	MENUITEM("ACT_TAG", _("Edit tags of package")); // OK
	MENUITEM("ACT_GENDEPS", _("Generate dependencies")); // OK
	MENUSPACE;
	MENUITEM("EXIT", _("Exit")); // OK :)
	
	string ret;
	do {
		ncInterface.setTitle(_("MPKG package system ") + (string) mpkgVersion);
		ncInterface.setSubtitle(_("Main menu"));
		ret = ncInterface.showMenu2(_("MPKG package system"), menuItems, ret);
		MENUCASE("ACT_CLEAN") actClean(core);
		MENUCASE("ACT_INDEX") {
			ncInterface.setSubtitle(_("Select directory to index:"));
			string path = ncInterface.ncGetOpenDir(getAbsolutePath("."));
			if (path.empty()) continue;
			bool index_filelist = ncInterface.showYesNo(_("Create filelist index?"));
			actIndex(core, path, index_filelist);
		}
		MENUCASE("ACT_PACKAGEMENU") actPackageMenu(core);
		MENUCASE("ACT_SEARCH") actSearch(core);
		MENUCASE("ACT_LIST_REP") manageRepositories(core);
		MENUCASE("ACT_UPDATE") actUpdate(core);
		MENUCASE("ACT_CONVERT") {
			string filename = ncInterface.ncGetOpenFile(getAbsolutePath("."));
			if (filename.empty()) continue;
			string tmpdir = get_tmp_dir();
			actConvert(getAbsolutePath(filename), tmpdir);
			delete_tmp_files();
		}
		MENUCASE("ACT_TAG") {
			ncInterface.setSubtitle(_("Choose package for tagging:"));
			string filename = ncInterface.ncGetOpenFile(getAbsolutePath("."));
			if (filename.empty()) continue;
			ncInterface.setSubtitle(_("Package tagging"));
			string tag = ncInterface.showInputBox(_("Enter new tag for package:"));
			if (tag.empty()) continue;
			bool clOther = ncInterface.showYesNo(_("Clear old tags?"));
			tag_package(filename, tag, clOther);
		}
		MENUCASE("ACT_COMMIT") {
			core.commit();
		}
		MENUCASE("ACT_SHOWQUEUE") {
			vector<string> _list_empty_;
			list(&core, _list_empty_, false, false, true);
		}
		MENUCASE("ACT_RESETQUEUE") {
			ncInterface.showInfoBox(_("Cleaning actions queue"));
			core.clean_queue();
			ncInterface.showMsgBox(_("Queue is clean"));
		}
		MENUCASE("ACT_INSTALLFROMLIST") {
			ncInterface.setSubtitle(_("Choose file with package list:"));
			string filename = ncInterface.ncGetOpenFile(getAbsolutePath("."));
			if (filename.empty()) continue;
			actInstallFromList(core, filename, false);
		}
		MENUCASE("ACT_GENDEPS") {
			ncInterface.setSubtitle(_("Choose package to generate dependencies:"));
			string filename = ncInterface.ncGetOpenFile(getAbsolutePath("."));
			if (filename.empty()) continue;
			ncInterface.showInfoBox(_("Generating dependencies for ") + filename);
			ncInterface.uninit();
			generateDeps_new(core, filename);
		}
		MENUCASE("ACT_FILESEARCH") {
			string pattern = ncInterface.showInputBox(_("Enter a part of filename:"));
			if (pattern.empty()) continue;
			searchByFile(&core, pattern);
		}
		MENUCASE("ACT_WHICH") {
			string filename = ncInterface.ncGetOpenFile("/");
			if (filename.empty()) continue;
			searchByFile(&core, filename, true);
		}
		MENUCASE("ACT_LISTDEPENDANTS") {
			string pkgname = ncInterface.showInputBox(_("Enter package name (without version, for example: zlib):"));
			if (pkgname.empty()) continue;
			actListDependants(core, pkgname);
		}
		MENUCASE("ACT_LATESTUPDATES") {
			actUpdatesMenu(core);
		}
		MENUCASE("ACT_SEARCHBYDESCRIPTION") {
			ncInterface.setSubtitle(_("Searching packages by description"));
			string pattern = ncInterface.showInputBox(_("Enter search pattern:"));
			if (pattern.empty()) continue;
			vector<string> query;
			query.push_back(pattern);
			actSearchByDescription(core, query);
		}
		MENUCASE("ACT_GETONLINEREPOSITORYLIST") {
			string url;
			url = "http://packages.agilialinux.ru/get_repository_list.php";
			url = ncInterface.showInputBox(_("Enter URL for repository list:"), url);
			if (url.empty()) continue;
			actGetRepositorylist(url);
		}
		MENUCASE("ACT_EXPORTINSTALLED") {
			ncInterface.setSubtitle(_("Export list of installed packages"));
			string output = ncInterface.showInputBox(_("Enter output filename:"));
			if (output.empty()) continue;
			bool includeVersions = ncInterface.showYesNo(_("Include version info?"));
			vector<string> data = core.getExportInstalled(includeVersions);
			WriteFileStrings(output, data);
			ncInterface.showMsgBox(_("List exported successfully"));
		}
		
	} while (ret != "EXIT" && !ret.empty());



}

int actInstallFromList(mpkg &core, string filename, bool includeVersions, bool enqueueOnly) {
	vector<string> installQuery, versionQuery;
       	parseInstallList(preprocessInstallList(filename), installQuery, versionQuery);
	int ret;
	vector<string> *v_ptr = NULL;
	if (includeVersions) v_ptr = &versionQuery;
	vector<string> errList;
	ret = core.install(installQuery, v_ptr, NULL, &errList);
	if (ret != 0) {
		if (!dialogMode) {
			for (unsigned int i=0; i<errList.size(); ++i) {
				mError(errList[i]);
			}
		}
		else {
			string err;
			for (unsigned int i=0; i<errList.size(); ++i) {
				err += errList[i]+"\n";
			}
			ncInterface.showMsgBox(err);
		}
		return ret;
	}
	return core.commit(enqueueOnly);
}

void actConvert(string filename, string tmpdir) {
	if (FileExists(filename)) {
		convert_package(filename, tmpdir);
		system("mv " + tmpdir + "/" + string(filename) + " " + string(filename));

//		convert_package(filename, tmpdir); // TODO: works like a shit
//		system("mv " + tmpdir + "/" + getFilename(filename) + " " + filename);
	}
	else mError(_("File ") + string(filename) + _(" doesn't exist"));
}
void actClean(mpkg &core) {
	if (dialogMode) ncInterface.showInfoBox(_("Cleaning package cache"));
	core.clean_cache(false);
}
void actIndex(mpkg &core, string path, bool index_filelist) {
	core.rep.build_index(path, index_filelist);

}
void actUpdatesMenu(mpkg &core) {
	vector<MenuItem> menuItems;
	menuItems.push_back(MenuItem("1", _("Show available updates")));
	menuItems.push_back(MenuItem("2", _("Install updates")));
	string ret;
	do {
		ret = ncInterface.showMenu2(_("Choose action"), menuItems);
		if (ret == "1") {
			actUpgrade(core, ACT_LISTUPGRADE);
		}
		if (ret == "2") {
			actUpgrade(core, ACT_UPDATEALL);
		}
	} while (!ret.empty());
}
void actUpgrade(mpkg &core, int action) {
		// New algorithm
		//if (dialogMode) ncInterface.showInfoBox(_("Searching for updates..."));
		//else fprintf(stderr, _("Searching for updates..."));

		PACKAGE_LIST resultList, uninstallList;
		vector<string> errorList = core.getLatestUpdates(&resultList, &uninstallList, true, dialogMode);
		vector<MenuItem> menuItems; // Vector for update list
		long double totalSizeC = 0, totalSizeI = 0;
		string branch;
		string distro;
		string size;
		if (errorList.empty() && resultList.size()>0)	{
			for (unsigned int i=0; i<resultList.size(); i++) {
				totalSizeC += atoi(resultList[i].get_compressed_size().c_str());
				totalSizeI += atoi(resultList[i].get_installed_size().c_str());
				size = " " + humanizeSize(atol(resultList[i].get_compressed_size().c_str()));
				if (resultList[i].get_repository_tags().empty() || resultList[i].get_repository_tags()=="0") branch.clear();
				else branch = "[" + resultList[i].get_repository_tags() + "]";
				if (resultList[i].package_distro_version.empty() || resultList[i].package_distro_version == "0") distro.clear();
				else distro = "[" + resultList[i].package_distro_version + "]";

				menuItems.push_back(MenuItem(IntToStr(i+1), resultList[i].get_name() + ": " + \
						uninstallList[i].get_fullversion() + " ==> " + resultList[i].get_fullversion() + branch + distro + size, resultList[i].get_description(), true));
			}
			PACKAGE_LIST uList;

			if (action==ACT_UPDATEALL) {
				if (dialogMode) {
					int ret = ncInterface.showExMenu(_("Found ") + IntToStr(resultList.size()) + _(" updates\n") + (string) _("Select packages you wish to be updated:\n"), menuItems);
					if (ret == -1) return;
					for (unsigned int i=0; i<menuItems.size(); i++) {
						if (menuItems[i].flag) uList.add(resultList[i]);
					}
				}
				else {
					uList = resultList;
					fprintf(stderr, string("\r" + string(_("Found ")) + IntToStr(resultList.size()) + _(" updates\n")).c_str());
					fprintf(stderr, _("Total: %s to download\n"), humanizeSize(totalSizeC).c_str());

				}
				core.install(&uList);
				core.commit();
				core.clean_queue();
			}
			if (action==ACT_LISTUPGRADE) {
				if (!dialogMode) {
					fprintf(stderr, _("Available updates:\n"));
					fprintf(stderr, "\n");
					for (unsigned int i=0; i<resultList.size(); i++) {
						if (_cmdOptions["machine_mode"]=="yes") {
							cout << resultList[i].get_name() << " " << uninstallList[i].get_fullversion() << " => " << resultList[i].get_fullversion() << endl;
							continue;
						}
						if (resultList[i].get_repository_tags().empty() || resultList[i].get_repository_tags()=="0") branch.clear();
						else branch = "[" + resultList[i].get_repository_tags() + "]";
						if (resultList[i].package_distro_version.empty() || resultList[i].package_distro_version == "0") distro.clear();
						else distro = "[" + resultList[i].package_distro_version + "]";
						size = " " + humanizeSize(atol(resultList[i].get_compressed_size().c_str()));
						
						if (!verbose) say("%s: %s%s%s ==> %s%s%s %s%s%s%s\n", resultList[i].get_name().c_str(), \
								CL_6, uninstallList[i].get_fullversion().c_str(), CL_WHITE, \
								CL_GREEN, resultList[i].get_fullversion().c_str(), \
								CL_BLUE, branch.c_str(), distro.c_str(), CL_WHITE, size.c_str());
						else for (unsigned int t=0; t<resultList[i].get_locations().size(); t++) {
							say("%s%s\n", resultList[i].get_locations().at(t).get_full_url().c_str(), resultList[i].get_filename().c_str());
						}
					}	

					fprintf(stderr, "\n");
					fprintf(stderr, string("\r" + string(_("Found ")) + IntToStr(resultList.size()) + _(" updates\n")).c_str());
					fprintf(stderr, _("Total: %s to download\n"), humanizeSize(totalSizeC).c_str());
				}
				else {
					ncInterface.showMenu2(_("Available updates:\n") + IntToStr(resultList.size()) + _(" packages, total size: ") + humanizeSize(totalSizeC) + " (" + humanizeSize(totalSizeI) + _(" installed)\n"), menuItems);
				}
			}
		}
		else {
			if (!dialogMode) fprintf(stderr, _("No updates available\n"));
			else ncInterface.showMsgBox(_("No updates available\n"));
		}
		//return 0;

}
void actUpdate(mpkg &core) {
	if (dialogMode) ncInterface.showInfoBox(_("Updating data from repositories..."));
	core.update_repository_data();
	delete_tmp_files();
	if (usedCdromMount) system("umount " + CDROM_MOUNTPOINT + " 2>/dev/null >/dev/null");
}

void actSearch(mpkg &core) {
	string search = ncInterface.showInputBox(_("Enter package name or part of it:"));
	if (search.empty()) return;
	vector<string> s;
	s.push_back(search);
	vector<MenuItem> menuItems;
	MENUITEM("A", _("Show only available"));
	MENUITEM("I", _("Show only installed"));
	if (ncInterface.showExMenu(_("Search options:"), menuItems)!=-1) {
		list(&core, s, menuItems[0].flag, menuItems[1].flag);
	}
}

void list_pkglist(const PACKAGE_LIST& pkglist)
{
	for (unsigned int i=0; i<pkglist.size(); i++)
	{
		say("[ %s ]\t", pkglist[i].get_vstatus(true).c_str());
		say("%s-%s-%s-%s\t(%s)\n", \
			pkglist[i].get_name().c_str(), \
			pkglist[i].get_version().c_str(), \
			pkglist[i].get_arch().c_str(), \
			pkglist[i].get_build().c_str(), \
			pkglist[i].get_short_description().c_str());
	}
}

void list(mpkg *core, const vector<string>& search, const bool showOnlyAvailable, const bool showOnlyInstalled, const bool onlyQueue)
{
	PACKAGE_LIST pkglist;
	SQLRecord sqlSearch;
	if (!search.empty())
	{
		sqlSearch.setEqMode(EQ_LIKE);
		for (unsigned int i=0; i<search.size(); i++)
		{
			sqlSearch.addField("package_name", search[i]);
		}
	}
	if (onlyQueue)
	{
		sqlSearch.setSearchMode(SEARCH_IN);
		sqlSearch.addField("package_action", ST_INSTALL);
		sqlSearch.addField("package_action", ST_REMOVE);
		sqlSearch.addField("package_action", ST_PURGE);
		sqlSearch.addField("package_action", ST_REPAIR);
		sqlSearch.addField("package_action", ST_UPDATE);
	}
	if (!dialogMode) say(_("Querying database...\n"));
	else ncInterface.showInfoBox(_("Querying database...\n"));
	
	core->db->sql_exec("PRAGMA case_sensitive_like = false;");
	core->get_packagelist(sqlSearch, &pkglist, true);
	core->db->sql_exec("PRAGMA case_sensitive_like = true;");
	if (pkglist.IsEmpty())
	{
		if (!dialogMode) say(_("Search attempt has no results\n"));
		else ncInterface.showMsgBox(_("Search attempt has no results\n"));

		return;
	}
	bool showThis;
	string id_str;
	vector<MenuItem> menuItems;
	string branch, distro;
	string taglist, extradata;
	// Create virtual pkglist
	PACKAGE_LIST *newPkgList = new PACKAGE_LIST;
	for (size_t i=0; i<pkglist.size(); ++i) {
		showThis=true;
		if (showOnlyAvailable && !pkglist[i].available()) showThis=false;
		if (showOnlyInstalled && !pkglist[i].installed()) showThis=false;
		if (onlyQueue && pkglist[i].action()!=ST_NONE) showThis=false;
		if (showThis) newPkgList->add(pkglist[i]);
	}
	pkglist = *newPkgList;
	delete newPkgList;
	for (unsigned int i=0; i<pkglist.size(); i++)
	{
		if (pkglist[i].get_repository_tags().empty() || pkglist[i].get_repository_tags()=="0") branch.clear();
		else branch="[" + pkglist[i].get_repository_tags() + "]";
		if (pkglist[i].package_distro_version.empty() || pkglist[i].package_distro_version == "0") distro.clear();
		else distro = "{" + pkglist[i].package_distro_version + "}";

		if (!dialogMode) {
			if (pkglist[i].isRemoveBlacklisted()) say("*");
			else say(" ");
		}
		id_str = IntToStr(pkglist[i].get_id());
		while (id_str.length()<4) id_str = " " + id_str;
		extradata.clear();
		if (verbose) {
			taglist.clear();
			for (size_t d=0; d<pkglist[i].get_tags().size(); ++d) {
				if (d) taglist += ", ";
				else taglist = _(", tags: ");
				taglist += pkglist[i].get_tags().at(d);
			}
			extradata = " [" + string(CL_5) + humanizeSize(atoi(pkglist[i].get_installed_size().c_str())) + string(CL_WHITE) + "] ";
		}
		if (!dialogMode) {
			say("[%s] [ %s ]%s\t", id_str.c_str(), pkglist[i].get_vstatus(true).c_str(), extradata.c_str());
			say("%s-%s-%s-%s\t(%s) %s%s%s%s%s\n", \
				pkglist[i].get_name().c_str(), \
				pkglist[i].get_version().c_str(), \
				pkglist[i].get_arch().c_str(), \
				pkglist[i].get_build().c_str(), \
				pkglist[i].get_short_description().c_str(), \
				CL_BLUE, branch.c_str(), distro.c_str(), CL_WHITE, taglist.c_str());
			
		}
		else {
			menuItems.push_back(MenuItem(IntToStr(pkglist[i].get_id()), string(pkglist[i].get_name() + " " + pkglist[i].get_fullversion() + " (" + pkglist[i].get_description() + ") " + branch + distro), pkglist[i].get_description(), pkglist[i].installed()));
		}
	}
	if (!dialogMode) say(_("Total: %d packages\n"), pkglist.size());
	else {
		int ret=0;
		while (ret!=-1) {
			ret = ncInterface.showExMenu(_("Search results:"), menuItems);
			if (ret!=-1) show_package_info(core, "", "", "", false, pkglist[ret].get_id());
		}
	}

	return;
}

void actPackageMenu(mpkg &core) {
	ncInterface.setSubtitle(_("Add/remove packages"));
	SQLRecord sqlSearch;
	PACKAGE_LIST packageList;
	core.get_packagelist(sqlSearch, &packageList);
	vector<bool> state;
	vector<int> id;
	for (unsigned int i=0; i<packageList.size(); i++) {
		state.push_back(packageList[i].installed());
		id.push_back(packageList[i].get_id());
	}
	vector<string> groups;
       	core.get_available_tags(&groups);
	vector<MenuItem> groupList, packageMenu;
	sort(groups.begin(), groups.end());
	groupList.push_back(MenuItem("ALL", _("All packages")));
	for (unsigned int i=0; i<groups.size(); i++) {
		groupList.push_back(MenuItem(groups[i],getTagDescription(groups[i])));
	}

	int ex_ret=0;
	//groupList.push_back(MenuItem("ALL", _("All packages")));
	groupList.push_back(MenuItem("OK", _("Apply changes and exit")));
	string menu_select;
	do {
		menu_select.clear();
		menu_select = ncInterface.showMenu2(_("Choose group or action"), groupList);
		if (!menu_select.empty() && menu_select != "OK") {
			packageMenu.clear();
			for (unsigned int i=0; i<packageList.size(); i++) {
				if ( menu_select == "ALL" || packageList[i].isTaggedBy(menu_select) ) {
					packageMenu.push_back(
							MenuItem(IntToStr(i), 
								packageList[i].get_name() + " " + packageList[i].get_fullversion() + " (" + packageList[i].get_short_description() + ")", 
								packageList[i].get_description(), state[i])
							);
				}
			}
			ex_ret = ncInterface.showExMenu(_("Mark packages you want to be installed"), packageMenu);
			if (ex_ret!=-1) {
				for (unsigned int m=0; m<packageMenu.size(); m++) {
					state[atoi(packageMenu[m].tag.c_str())] = packageMenu[m].flag;
				}
			}
		}

	} while (!menu_select.empty() && menu_select!="OK");
	// Подвод итогов
	string will_be_installed;
	string will_be_removed;
	if (menu_select.empty()) return; // Нечего делать - все отменили
	vector<string> pname, pversion, pbuild, remove_pname;
	for (unsigned int i=0; i<packageList.size(); i++) {
		if (packageList[i].installed() != state[i]) {
			if (state[i]) {
				will_be_installed += "[" + IntToStr(id[i]) + "] " + packageList[i].get_name() + "\n";
				pname.push_back(packageList[i].get_name());
				pversion.push_back(packageList[i].get_version());
				pbuild.push_back(packageList[i].get_build());
			}
			else {
				will_be_removed += "[" + IntToStr(id[i]) + "] " + packageList[i].get_name() + "\n";
				remove_pname.push_back(packageList[i].get_name());
			}
		}
	}
	string message;
	if (!will_be_installed.empty()) {
		message +=  _("Packages to be installed:\n") + will_be_installed + "\n";
		core.install(pname, &pversion, &pbuild);
	}
	if (!will_be_removed.empty()) {
		message +=  _("Packages to be removed:\n") + will_be_removed;
		core.uninstall(remove_pname);
	}
	core.commit();
	//ncInterface.showText(message, "Применить", "Отмена");

}


void manageRepositories(mpkg &core) {
	vector<string> rep_list = core.get_repositorylist();
	vector<string> disabled_list = core.get_disabled_repositorylist();
	vector<MenuItem> menuItems;
	int ret = 0;
	int def_select = 0;
	int key = 0;
	int skip_item = -1;
	string tmp;
	do {
		skip_item = -1;
		menuItems.clear();
		for (unsigned int i=0; i<rep_list.size(); i++) {
			menuItems.push_back(MenuItem(IntToStr(i), rep_list[i], "", true));
		}
		for (unsigned int i=0; i<disabled_list.size(); i++) {
			menuItems.push_back(MenuItem(IntToStr(i+rep_list.size()), disabled_list[i], "", false));
		}
		menuItems.push_back(MenuItem("-", _("Apply")));
		key = 0;
		ret = ncInterface.showExMenu(_("Edit repository list. Keys: SPACE - enable/disable, ENTER - edit, INSERT - add, DELETE - remove. When ready, select Apply"), menuItems, def_select, &key);
		rep_list.clear();
		disabled_list.clear();

		switch (key) {
			case 331: // Insert
				tmp = ncInterface.showInputBox(_("Enter repository URL:"), tmp);
				if (!tmp.empty()) rep_list.push_back(tmp);
				tmp.clear();
				break;
			case 330:
				skip_item = ret;
				break;

		}
		if (key==0 && ret != -1 && ret != (int) menuItems.size()-1) {
			tmp = ncInterface.showInputBox(_("Edit repository URL:"), menuItems[ret].value);
			if (!tmp.empty()) menuItems[ret].value = tmp;
			tmp.clear();
		}
		for (unsigned int i=0; i<menuItems.size()-1; i++) {
			if (i==(unsigned int) skip_item) continue;
			if (menuItems[i].flag) rep_list.push_back(menuItems[i].value);
			else disabled_list.push_back(menuItems[i].value);
		}

	} while (ret != (int) menuItems.size()-1 && ret != -1);
	if (ret != -1) core.set_repositorylist(rep_list, disabled_list);
//	core.set_disabled_repositorylist(disabled_list);
}
string getInstallDate(const PACKAGE *pkg) {
	mpkg core;
	SQLRecord sqlSearch, sqlFields;
	sqlSearch.addField("action_type", 0);
	sqlSearch.addField("package_md5", pkg->get_md5());
	sqlFields.addField("action_date");
	SQLTable results;
	core.db->get_sql_vtable(results, sqlFields, "transaction_details", sqlSearch);
	if (results.empty()) return _("unknown date");
	return results.getValue(0,0);
	

}

void showPackageInfoCLI(const PACKAGE *pkg, const string &hasUpdate) {
	string vstatus;
	vstatus.clear();
	if (pkg->available()) vstatus = _("Available");
	if (pkg->installed()) {
		if (!vstatus.empty()) vstatus += ", ";
		vstatus += _("Installed");
	}
	if (vstatus.empty()) vstatus = _("Doesn't exist");
	vstatus = pkg->get_vstatus(true) + " (" + vstatus + ")";

	string tags;
	for (size_t i=0; i<pkg->get_tags().size(); ++i) {
		tags += pkg->get_tags().at(i);
		if (i<pkg->get_tags().size()-1) tags+= ", ";
	}

	string depends;
	for (size_t i=0; i<pkg->get_dependencies().size(); ++i) {
		depends += pkg->get_dependencies().at(i).get_package_name();
	       	if (atoi(pkg->get_dependencies().at(i).get_condition().c_str())!=VER_ANY) depends += pkg->get_dependencies().at(i).get_vcondition() + pkg->get_dependencies().at(i).get_package_version();
		if (i<pkg->get_dependencies().size()-1) depends+= ", ";
	}


	// FIXME: Split this code into separate function(s) like getShortDescriptionTranslated(string pkgname, string lang)
	// Note: think twice about it's signature

	string shortDescription = pkg->get_short_description();
	string lang = setlocale(LC_MESSAGES, NULL);
	if (lang.size()<2) lang = "en";
	else lang = lang.substr(0, 2);
	// HARDCORE
	string short_translated = SYS_MPKG_VAR_DIRECTORY + "/descriptions/" + lang + string("/shortdesc/") + strToLower(pkg->get_name()) + ".txt";
	if (FileExists(short_translated)) {
		shortDescription = ReadFile(short_translated);
		strReplace(&shortDescription, "\t", "");
		strReplace(&shortDescription, "\n", "");
		strReplace(&shortDescription, "\r", "");
	}

	cout << pkg->get_name() << " " << pkg->get_version() << "-" << pkg->get_build() << " (" << pkg->get_arch() << "): " << shortDescription << endl;
	if (pkg->installed()) cout << _("Installed:") << "\t" << getInstallDate(pkg) << endl;
	else cout << _("Available in:") << "\t" << pkg->package_distro_version.c_str() << endl;
	if (!pkg->get_provides().empty()) cout << _("Provides:") << "\t" << pkg->get_provides() << endl;
	if (!pkg->get_conflicts().empty()) cout << _("Conflicts with:") << "\t"	<< pkg->get_conflicts() << endl;
	cout << _("Size:") << "\t\t" << humanizeSize(atoi(pkg->get_compressed_size().c_str())) << _(" compressed") << endl << "\t\t" << humanizeSize(atoi(pkg->get_installed_size().c_str())) << _(" installed") << endl;
	cout << _("Tags:") << "\t\t" << tags << endl;
	cout << _("Built by:") << "\t" << pkg->get_packager();
	if (verbose) cout << " <" << pkg->get_packager_email() << ">" << endl;
	else cout << endl;
	cout << _("Depends:") << "\t" << depends << endl;
	if (verbose) {
		cout << _("MD5:") << "\t\t" << pkg->get_md5() << endl;
		cout << _("Package ID:") << "\t" << pkg->get_id() << endl;
		if (pkg->available()) {
			cout << _("Locations:") << "\t";
			for (size_t i=0; i<pkg->get_locations().size(); ++i) {
				if (i!=0) cout << "\t\t";
				cout << pkg->get_locations().at(i).get_full_url() << endl;
			}
		}
		if (!pkg->config_files.empty()) {
			cout << _("Config files:") << "\t";
			for (size_t i=0; i<pkg->config_files.size(); ++i) {
				if (i!=0) cout << "\t\t";
				cout << pkg->config_files[i].name << endl;
			}
		}
	}
	
	
	   
	/*   say(_("%sID:%s %d\n"), CL_GREEN, CL_WHITE, pkg->get_id());
	say(_("%sName:%s %s\n"), CL_GREEN, CL_WHITE, pkg->get_name().c_str());
	say(_("%sVersion:%s %s\n"), CL_GREEN, CL_WHITE, pkg->get_version().c_str());
	say(_("%sArch:%s %s\n"), CL_GREEN, CL_WHITE, pkg->get_arch().c_str());
	say(_("%sBuild:%s %s\n"), CL_GREEN, CL_WHITE, pkg->get_build().c_str());
	if (!pkg->get_provides().empty()) say(_("%sProvides:%s %s\n"), CL_GREEN, CL_WHITE, pkg->get_provides().c_str());
	if (!pkg->get_conflicts().empty()) say(_("%sConflicts:%s %s\n"), CL_GREEN, CL_WHITE, pkg->get_conflicts().c_str());
	say(_("%sBranch:%s %s\n"), CL_GREEN, CL_WHITE, pkg->package_distro_version.c_str());
	say(_("%sPackage size:%s %s\n"), CL_GREEN, CL_WHITE, humanizeSize(atoi(pkg->get_compressed_size().c_str())).c_str());
	say(_("%sInstalled size:%s %s\n"), CL_GREEN, CL_WHITE, humanizeSize(atoi(pkg->get_installed_size().c_str())).c_str());
	say(_("%sBuilt by:%s %s\n"), CL_GREEN, CL_WHITE, pkg->get_packager().c_str());
	say(_("%sBuilder e-mail:%s %s\n"), CL_GREEN, CL_WHITE, pkg->get_packager_email().c_str());
	say(_("%sStatus:%s %s\n"), CL_GREEN, CL_WHITE, vstatus.c_str());
	say(_("%sMD5:%s %s\n"), CL_GREEN, CL_WHITE, pkg->get_md5().c_str());
	say(_("%sFilename:%s %s\n"), CL_GREEN, CL_WHITE, pkg->get_filename().c_str());
	say(_("%sShort description:%s %s\n"), CL_GREEN, CL_WHITE, pkg->get_short_description().c_str());
		
	if (pkg->available()) {
		say(_("%sLocations:%s\n"), CL_GREEN, CL_WHITE);
		for (unsigned int t=0; t<pkg->get_locations().size(); t++)
		{
			say("\t\t    %s\n", pkg->get_locations().at(t).get_full_url().c_str());
		}
	}
	else say(_("%sLocations:%s %s\n"), CL_GREEN, CL_WHITE, _("none"));
	if (!pkg->deltaSources.empty()) {
		say(_("%sDelta patches:%s\n"), CL_GREEN, CL_WHITE);
		for (unsigned int t=0; t<pkg->deltaSources.size(); ++t) {
			say("\t\t    %s (%s)\n", pkg->deltaSources[t].dup_url.c_str(), humanizeSize(pkg->deltaSources[t].dup_size).c_str());
		}
	}
	//else say(_("%sDelta patches:%s %s\n"), CL_GREEN, CL_WHITE, _("none"));
	if (!pkg->abuild_url.empty()) {
		say(_("%sABUILD:%s %s\n"), CL_GREEN, CL_WHITE, pkg->abuild_url.c_str());
	}
	else say(_("%sABUILD:%s %s\n"), CL_GREEN, CL_WHITE, _("none"));

	if (!pkg->config_files.empty()) {
		say(_("%sConfig files:%s\n"), CL_GREEN, CL_WHITE);
		for (size_t i=0; i<pkg->config_files.size(); ++i) {
			say("\t\t%s\n", pkg->config_files[i].name.c_str());
		}
	}

		
	say(_("%sTags:%s "), CL_GREEN, CL_WHITE);
	if (pkg->get_tags().empty())
	{
		say(_("none\n"));
	}
	else
	{
		for (unsigned int t=0; t<pkg->get_tags().size(); t++)
		{
			say ("%s", pkg->get_tags().at(t).c_str());
			if (t<pkg->get_tags().size()-1) say(", ");
			else say("\n");
		}
	}
	if (!pkg->get_dependencies().empty())
	{
		say(_("%sDepends on:%s "), CL_GREEN, CL_WHITE);
		for (unsigned int t=0; t<pkg->get_dependencies().size(); t++)
		{
			say("%s", pkg->get_dependencies().at(t).getDepInfo().c_str());
			if (t<pkg->get_dependencies().size()-1) say(", ");
		}
		say("\n");
	}

	if (!pkg->get_changelog().empty() && pkg->get_changelog()!="0") say(_("%sChangelog:%s          \n%s\n"), CL_GREEN, CL_WHITE, pkg->get_changelog().c_str());
	if (!pkg->get_description().empty() && pkg->get_description()!="0") say(_("%sDescription:%s        \n%s\n"), CL_GREEN, CL_WHITE, pkg->get_description().c_str());
	*/
}

void showPackageFilelist(PACKAGE *pkg) {
	say(_("%sFilelist:%s\n"), CL_GREEN, CL_WHITE);
	if (pkg->installed()) {
		if (pkg->get_files().size()==0)
		{
			say(_("\tPackage contains no files\n"));
		}
		else for (size_t t=0; t<pkg->get_files().size(); ++t) {
			say ("\t    %s\n", pkg->get_files().at(t).c_str());
		}
	}
	else say (_("\tPackage is not installed\n"));
}

void showPackageInfoDialog(const PACKAGE *pkg, const string &hasUpdate) {
	if (!dialogMode) showPackageInfoCLI(pkg, hasUpdate);

	ncInterface.setSubtitle(_("Information about package ") + string(" ") + pkg->get_name() + " (ID: " + IntToStr(pkg->get_id()) + ")");
	if (dialogMode) {
		string data;
		string locationsInfo;
		string url;
		string depData;
		string deltaInfo;
		string hasABUILD;
	//	for (unsigned int i=0; i<pkgList.size(); ++i) {
	//		pkg = pkgList.get_package_ptr(i);
			mstring taglist;
			for (unsigned int t=0; t< pkg->get_tags().size(); ++t) {
				taglist+="  " + pkg->get_tags().at(t)+"\n";
			}
			locationsInfo.clear();
			for (unsigned int t=0; t<pkg->get_locations().size(); t++) {
				url = pkg->get_locations().at(t).get_full_url();
			       	if (pkg->get_filename()!=url) url += pkg->get_filename();
				if (url.find("/./")) url = url.substr(0, url.find("/./")) + url.substr(url.find("/./")+2);
				locationsInfo += "  " + url + "\n";
			}
			deltaInfo.clear();
			for (unsigned int t=0; t<pkg->deltaSources.size(); t++) {
				deltaInfo += pkg->deltaSources[t].dup_url + "\n";
			}
			depData.clear();
			for (unsigned int t=0; t<pkg->get_dependencies().size(); t++) {
				// Sorry for hardcoding search.php here, it will be replaced by cmdline and/or config option later.
				depData += pkg->get_dependencies().at(t).getDepInfo()+"\n";
			}

			hasABUILD.clear();
			if (!pkg->abuild_url.empty()) {
				hasABUILD = "\nABUILD: " + pkg->abuild_url;
			}

			data = _("Name: ") + pkg->get_name() \
				+ (string) _("\nVersion: ") + pkg->get_version() \
			       	+ (string) _("\nBeta release: ") + pkg->get_betarelease() \
			       	+ (string) _("\nArch: ") + pkg->get_arch() \
			       	+ (string) _("\nBuild: ") + pkg->get_build() \
			       	+ (string) _("\nProvides: ") + pkg->get_provides() \
			       	+ (string) _("\nConflicts: ") + pkg->get_conflicts() \
				+ (string) _("\nBranch: ") + pkg->get_repository_tags() \
				+ (string) _("\nShort description: ") + pkg->get_short_description() \
				+ (string) _("\nFull description: ") + pkg->get_description() \
			       	+ (string) _("\nCompressed size: ") + humanizeSize(pkg->get_compressed_size()) \
			       	+ (string) _("\nInstalled size: ") + humanizeSize(pkg->get_installed_size()) \
			       	+ (string) _("\nFile name: ") + pkg->get_filename() \
			       	+ (string) _("\nMD5 checksum: ") + pkg->get_md5() \
			       	+ (string) _("\nMaintainer: ") + pkg->get_packager() \
			       	+ (string) " (" + pkg->get_packager_email() + (string)")" \
			       	+ (string) _("\n\nTags: \n") \
			       	+ taglist.s_str() \
				+ (string) _("\nAdded to repository: ") + getTimeString(pkg->add_date) \
				+ (string) _("\nLocations: \n") + locationsInfo \
				+ (string) _("\nDeltas: \n") + deltaInfo \
				+ (string) _("\nDependencies: \n") + depData \
				+ hasABUILD \
				+ hasUpdate;
				;


	//	}
		ncInterface.showMsgBox(data);
		return;
	}
}



void show_package_info(mpkg *core, string name, string version, string build, bool showFilelist, int id) {
	PACKAGE_LIST pkgList;
	SQLRecord sqlSearch;
	if (id>=0) {
		sqlSearch.addField("package_id", id);
	}
	if (!name.empty()) {
		sqlSearch.addField("package_name", name);
	}
	if (!version.empty()) {
		sqlSearch.addField("package_version", version);
	}
	if (!build.empty()) {
		sqlSearch.addField("package_build", build);
	}

	core->get_packagelist(sqlSearch, &pkgList);
	// Let's change behavior a little: show only one package: installed if any, or maxVersion if not.
	// We should add some key to show all packages, but... I don't know what to choose :) TODO: do it, later.
	pkgList.initVersioning();
	PACKAGE *installedPkg = (PACKAGE *) pkgList.getInstalledOne();
	PACKAGE *maxVersionPkg = pkgList.getMaxVersion();
	string hasUpdate;
	if (installedPkg && maxVersionPkg && installedPkg!=maxVersionPkg) hasUpdate = _("\nUpdate available: ") + maxVersionPkg->get_fullversion() + "\n";
	PACKAGE *pkg = installedPkg;
	if (!pkg) pkg=maxVersionPkg;
	if (!pkg || pkgList.IsEmpty()) {
		if (!dialogMode) say(_("No such package %s %s %s\n"), name.c_str(), version.c_str(), build.c_str());
		else ncInterface.showMsgBox(_("No such package\n"));
		return;
	}
	PACKAGE_LIST newList;
	newList.add(*pkg);
	pkg = newList.get_package_ptr(0);
	pkgList.clear();
	if (showFilelist) core->db->get_full_filelist(&newList);
	if (verbose) core->db->get_full_config_files_list(&newList);

	// At this point, pkg contains pointer to package to be shown.
	if (showFilelist) {
		showPackageFilelist(pkg);
	}
	else {
		if (dialogMode) showPackageInfoDialog(pkg, hasUpdate);
		else showPackageInfoCLI(pkg, hasUpdate);
	}
}


void searchByFile(mpkg *core, string filename, bool strict)
{
	ncInterface.setSubtitle(_("Searching for file ") + filename);
	string filename_orig=filename;
	if (filename.length()==0) {
		say(_("No filename specified\n"));
		return;
	}

	if (filename[0]=='/') filename=filename.substr(1);
	if (!strict) {
		if (!dialogMode) say(_("Searching by filename with pattern \"%s\"...\n"), filename_orig.c_str());
		else ncInterface.showInfoBox(_("Searching by filename with pattern \"") + filename_orig + "\"...");
	}
	else {
		if (!dialogMode) say(_("Searching package who contains \"/%s\"\n"), filename_orig.c_str());
		else ncInterface.showInfoBox(_("Searching package who contains \"") + filename_orig + "\"");
	}

	SQLRecord sqlSearch, sqlFields, sqlPkgSearch;
	
	if (!strict) sqlSearch.setEqMode(EQ_LIKE);
	else sqlSearch.setEqMode(EQ_EQUAL);
	sqlSearch.addField("file_name", filename);
	sqlFields.addField("packages_package_id");
	sqlFields.addField("file_name");
	SQLTable results;
	core->db->get_sql_vtable(results, sqlFields, "files", sqlSearch);
	PACKAGE_LIST pkgList;
	if (results.size()==0) {
		if (!dialogMode) say(_("File %s doesn't belong to any installed package\n"),filename_orig.c_str());
		else ncInterface.showMsgBox(_("File ") + filename_orig + _(" doesn't belong to any of installed packages"));
		return;
	}
	sqlPkgSearch.setSearchMode(SEARCH_IN);
	bool dupFound;
	unsigned int fPackages_package_id=results.getFieldIndex("packages_package_id");
	for (unsigned int i=0; i<results.size(); i++) {
		dupFound=false;
		for (unsigned int t=0; !dupFound && t<i; ++t) {
			if (results.getValue(i, fPackages_package_id)==results.getValue(t, fPackages_package_id)) dupFound=true;
		}
		if (!dupFound) sqlPkgSearch.addField("package_id", results.getValue(i,fPackages_package_id));
	}
	core->get_packagelist(sqlPkgSearch, &pkgList);
	if (pkgList.size()==0) {
		mWarning(_("Hm... no package was returned. Seems to be malformed SQL query"));
		return;
	}
	string data;
	if (!dialogMode) say(_("File %s found in %d package(s):\n"),filename_orig.c_str(), pkgList.size());
	else data = _("File ") + filename_orig + _(" found in ") + IntToStr(pkgList.size()) + _(" package(s):\n");
	string pattern;
	bool showIt;
	vector<int> usedIDs;
	unsigned int fFile_name = results.getFieldIndex("file_name");
	for (unsigned int i=0; i<results.size(); i++) {
		for (unsigned int t=0; t<pkgList.size(); t++) {
			if (results.getValue(i,fPackages_package_id)==IntToStr(pkgList[t].get_id())) {
				if (verbose) {
					pattern=results.getValue(i,fFile_name);
					if (!dialogMode) {
						say("/%s: %s %s\n", pattern.c_str(), pkgList[t].get_name().c_str(), \
							pkgList[t].get_fullversion().c_str());
					}
					else data += "/" + pattern + ": " + pkgList[t].get_name() + " " + pkgList[t].get_fullversion() + "\n";
				}
				else {
					showIt=true;
					for (unsigned int u = 0; u < usedIDs.size(); u++) {
						if (usedIDs[u]==pkgList[t].get_id()) { 
							showIt=false;
							break;
						}
					}
					if (showIt) {
						if (!dialogMode) {
							say(_("%s-%s\n"), pkgList[t].get_name().c_str(), \
								pkgList[t].get_fullversion().c_str());
						}
						else data += pkgList[t].get_name() + " " + pkgList[t].get_fullversion() + "\n";
					}

				}
				usedIDs.push_back(pkgList[t].get_id());

			}
		}
	}
	if (dialogMode) ncInterface.showMsgBox(data);
}

void actListDependants(mpkg &core, string pkgname, bool includeNotInstalled) {
	PACKAGE_LIST packages;
	SQLRecord sqlSearch;
	if (!includeNotInstalled) sqlSearch.addField("package_installed", 1);
	core.get_packagelist(sqlSearch, &packages);

	string core_name, data;
	// We should distinguish installed and not installed packages, as available packages may be different, including malformed ones.
	// First, check if such package exists within installed packages
	for (size_t i=0; i<packages.size(); ++i) {
		if (packages[i].installed() && packages[i].get_name()==pkgname) {
			core_name = packages[i].get_corename();
			break;
		}
	}

	if (core_name.empty()) {
		for (size_t i=0; i<packages.size(); ++i) {
			if (!packages[i].installed() && packages[i].get_name()==pkgname) {
				core_name = packages[i].get_corename();
				break;
			}
		}

	}

	if (core_name.empty()) {
		mWarning(_("Package ") + pkgname + _(" not found, but we will try to find if some package still depends on it"));
		core_name = pkgname;
	}
	if (core_name!=pkgname) fprintf(stderr, _("Looking for package who depends on %s, as %s provides it\n"), core_name.c_str(), pkgname.c_str());

	vector<PACKAGE *> list;
	bool found;
	for (size_t i=0; i<packages.size(); ++i) {
		for (size_t t=0; t<packages[i].get_dependencies().size(); ++t) {
			if (packages[i].get_dependencies().at(t).get_package_name()==core_name) {
				found = false;
				for (size_t z=0; !found && z<list.size(); ++z) {
					if (list[z]==packages.get_package_ptr(i)) found=true;
				}
				if (!found) list.push_back(packages.get_package_ptr(i));
			}
		}
	}
	// Output
	
	
	if (!dialogMode) fprintf(stderr, _("Next packages depends on %s: \n"), pkgname.c_str());
	else {
		ncInterface.setSubtitle(_("Searching for packages depending on ") + pkgname);
		data = _("Next packages depends on ") + pkgname + "\n";
	}

	if (list.empty()) {
		if (dialogMode) data = _("No package depends on ") + pkgname;
		else fprintf(stderr, _("No package depends on %s\n"), pkgname.c_str());
	}
	string vcond, pver, vlimit = _cmdOptions["versionLimit"];

	for (size_t i=0; i<list.size(); ++i) {
		if (!dialogMode) {
			for (size_t t=0; t<list[i]->get_dependencies().size(); ++t) {
				if (list[i]->get_dependencies().at(t).get_package_name()==core_name) {
					vcond = list[i]->get_dependencies().at(t).get_vcondition();
					pver = list[i]->get_dependencies().at(t).get_package_version();
					if (vcond=="(any)") {
						vcond.clear();
						pver.clear();
					}

					if (!vlimit.empty()) {
						if (pver.empty()) break;
						if (strverscmp2(vlimit, pver)<=0) break;
					}

					if (verbose) {
						say("%s%s%s\n", list[i]->get_name().c_str(), vcond.c_str(), pver.c_str());
						break;
					}
					else {
						say("%s\n", list[i]->get_name().c_str());
						
					}
				}
			}
		}
		else {
			data += list[i]->get_name() + " " + list[i]->get_fullversion() + "\n";
		}

	}

	if (dialogMode) ncInterface.showMsgBox(data);
}

void actSearchByDescription(mpkg &core, const vector<string> &query, bool showOnlyInstalled, bool showOnlyAvailable) {
	SQLRecord sqlSearch;
	for (unsigned int i=0; i<query.size(); ++i) {
		sqlSearch.addField("package_description", query[i]);
	}
	sqlSearch.setEqMode(EQ_LIKE);
	sqlSearch.setSearchMode(SEARCH_AND);
	PACKAGE_LIST pkglist;
	
	core.db->sql_exec("PRAGMA case_sensitive_like = false;");
	core.get_packagelist(sqlSearch, &pkglist);
	core.db->sql_exec("PRAGMA case_sensitive_like = true;");
	bool showThis;
	int counter=0;
	string id_str;
	for (unsigned int i=0; i<pkglist.size(); i++)
	{
		showThis=true;
		if (showOnlyAvailable && !pkglist[i].available()) showThis=false;
		if (showOnlyInstalled && !pkglist[i].installed()) showThis=false;
		if (showThis)
		{
			counter++;
			if (pkglist[i].isRemoveBlacklisted()) say("*");
			else say(" ");
			id_str = IntToStr(pkglist[i].get_id());
			while (id_str.length()<4) id_str = " " + id_str;
			say("[%s] [ %s ]\t", id_str.c_str(), pkglist[i].get_vstatus(true).c_str());
			say("%s-%s-%s-%s\t(%s)\n", \
				pkglist[i].get_name().c_str(), \
				pkglist[i].get_version().c_str(), \
				pkglist[i].get_arch().c_str(), \
				pkglist[i].get_build().c_str(), \
				pkglist[i].get_short_description().c_str());
		}
	}
	say(_("Total: %d packages\n"), counter);
}

void actGetRepositorylist(string url) {
	string out = get_tmp_file();
	if (url.empty()) {
		url = "http://packages.agilialinux.ru/get_repository_list.php";
	}
	if (CommonGetFile(url, out)==DOWNLOAD_OK) {
		vector<string> draftList = ReadFileStrings(out);
		vector<string> repList;
		for (unsigned int i=0; i<draftList.size(); i++) {
			if (draftList[i].find("http://")==0 ||
					draftList[i].find("ftp://")==0 ||
					draftList[i].find("fish://")==0 ||
					draftList[i].find("https://")==0 ||
					draftList[i].find("file://")==0 ||
					draftList[i].find("cdrom://")==0)
				repList.push_back(draftList[i]);
		}
		if (repList.size()==0) {
			mError(_("Cannot get valid repository list from url ") + url);
			return;
		}
		vector<string> disabled_rep_list = mpkgconfig::get_disabled_repositorylist();
		mpkgconfig::set_repositorylist(repList, disabled_rep_list);
		delete_tmp_files();
		return;
	}
	mError(_("Cannot download repository list from ") + url);
	return;
}
