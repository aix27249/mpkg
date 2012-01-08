#include <mpkg/libmpkg.h>

void check_conflicts(mpkg *core, const PACKAGE& package, vector<string> &fileNames, vector<int> &package_ids)
{
	if (verbose && !dialogMode) say(_("\nChecking file conflicts for package %s\n"), package.get_name().c_str());
	int package_id;
	SQLTable sqlTable;
	SQLRecord sqlFields;
	SQLRecord sqlSearch;
	sqlSearch.setSearchMode(SEARCH_IN);
	sqlFields.addField("packages_package_id");
	sqlFields.addField("file_name");
	if (package.get_files().size()==0) return; // If a package has no files, it cannot conflict =)
	for (unsigned int i=0;i<package.get_files().size(); ++i) {
		if (package.get_files().at(i).at(package.get_files().at(i).length()-1)!='/') {
			sqlSearch.addField("file_name", package.get_files().at(i));
		}
	}

	core->db->get_sql_vtable(sqlTable, sqlFields, "files", sqlSearch);
	int fPackages_package_id = sqlTable.getFieldIndex("packages_package_id");
	int fFile_name = sqlTable.getFieldIndex("file_name");
	if (!sqlTable.empty()) {
		for (size_t k=0;k<sqlTable.getRecordCount() ;k++) {
			package_id=atoi(sqlTable.getValue(k, fPackages_package_id).c_str());
			if (package_id!=package.get_id()) {
				if (core->db->get_installed(package_id) || core->db->get_action(package_id)==ST_INSTALL) {
					fileNames.push_back(sqlTable.getValue(k, fFile_name));
					package_ids.push_back(package_id);
				}
			}
		}
	}
}


int main(int argc, char **argv) {
	mpkg core;
	PACKAGE_LIST pkglist;
	SQLRecord sqlSearch;
	core.get_packagelist(sqlSearch, &pkglist, true, false);
	core.db->get_full_filelist(&pkglist);
	
	PACKAGE *p;
	vector<int> conflicting_packages;
	bool found;

	vector<string> fileNames; vector<int> package_ids;
	for (size_t i=0; i<pkglist.size(); ++i) {
		fileNames.clear();
		package_ids.clear();
		check_conflicts(&core, pkglist[i], fileNames, package_ids);

		for (size_t t=0; t<fileNames.size(); ++t) {
			p = pkglist.getPackageByIDPtr(package_ids[t]);
			cout << "CONFLICT: " << fileNames[t] << ", " + pkglist[i].get_name() << " => " << p->get_name() << endl;
			found = false;
			for (size_t z=0; !found && z<conflicting_packages.size(); ++z) {
				if (conflicting_packages[z]==package_ids[t]) found = true;
			}
			if (!found) conflicting_packages.push_back(package_ids[t]);
		}
		
	}

	for (size_t i=0; i<conflicting_packages.size(); ++i) {
		p = pkglist.getPackageByIDPtr(conflicting_packages[i]);
		cout << i+1 << ": " << p->get_name() << endl;
	}



	return 0;
}
