#include <mpkg/libmpkg.h>
string fatalSummary, blockingSummary, warningSummary, okSummary;
int print_usage(int err=0) {
	printf(_("Usage: mpkg-validate FILENAME\n"));
	return err;
}
int fatalCount=0, blockingCount=0, warningCount=0, okCount=0;
void fatal(string msg) {
	printf(_("[FATAL] Error: %s\n"), msg.c_str());
	if (fatalSummary.empty()) fatalSummary += "\n================FATAL ERRORS================\n";
	fatalSummary+="FATAL:" + msg + "\n";
	fatalCount++;
}

void blocking(string msg) {
	printf(_("[BLOCKING] Error: %s\n"), msg.c_str());
	if (blockingSummary.empty()) blockingSummary += "\n================BLOCKING ERRORS================\n";
	blockingSummary+="BLOCKING: " + msg+"\n";
	blockingCount++;
}

void warning(string msg) {
	printf(_("[NOT GOOD] Warning: %s\n"), msg.c_str());
	if (warningSummary.empty()) warningSummary += "\n================WARNINGS================\n";
	warningSummary+="WARNING: " + msg+"\n";
	warningCount++;
}

void ok(string msg) {
	printf(_("[PASSED] OK: %s\n"), msg.c_str());
	okSummary+=msg+"\n";
	okCount++;
}
string moveFatalsTo, moveBlockingTo;
int checkPackage(string filename) {
	printf("==================================>>>>>>>>CHECKING: %s\n", filename.c_str());
	fatalCount=0;
	blockingCount=0;
	warningCount=0;
	okCount=0;
	fatalSummary.clear();
	blockingSummary.clear();
	warningSummary.clear();
	okSummary.clear();
	// First: check package integrity
	int etar = system("tar tf " + filename + " >/dev/null");
	if (etar) {
		fatal(_("Extraction failed"));
		return 0;
	}
	LocalPackage lp(filename);
	lp.injectFile();
	if (!lp._parseOk) {
		fatal(_("Failed to parse package: invalid data.xml"));
	}
	else ok(_("data.xml parsed"));
	PACKAGE *p = &lp.data;
	lp.fill_filelist(p);
	delete_tmp_files();
	if (lp.legacyPackage) blocking(_("No data.xml found, legacy packages are not acceptable. Convert it using mpkg convert"));
	PACKAGE_LIST pkgList;
	mpkg core;
	SQLRecord sqlSearch;
	core.get_packagelist(sqlSearch, &pkgList);
	const PACKAGE *installedPackage=NULL;
	for (unsigned int i=0; i<pkgList.size(); ++i) {
		if (pkgList[i].get_name()!=p->get_name()) continue;
		if (pkgList[i].installed()) {
			installedPackage=&pkgList[i];
			break;
		}
	}
	// Check filename
	if (p->get_filename() != p->get_name() + "-" + p->get_version() + "-" + p->get_arch() + "-" + p->get_build() + "." + getExtension(p->get_filename())) {
		warning(_("Filename doesn't conform slackware guidelines: should be ") + p->get_name() + "-" + p->get_version() + "-" + p->get_arch() + "-" + p->get_build() + "." + getExtension(p->get_filename()) + _(" ; to fix this - rename package"));
	}
	else ok(_("Filename conforms slackware guidelines"));
	// Check conflicts
	SQLTable sqlTable;
	SQLRecord sqlFields;
	sqlSearch.setSearchMode(SEARCH_IN);
	sqlFields.addField("packages_package_id");
	sqlFields.addField("file_name");
	for (unsigned int i=0;i<p->get_files().size(); ++i) {
		if (p->get_files().at(i).get_name().at(p->get_files().at(i).get_name().length()-1)!='/') {
			sqlSearch.addField("file_name", p->get_files().at(i).get_name());
		}
	}

	core.db->get_sql_vtable(sqlTable, sqlFields, "files", sqlSearch);
	int fPackages_package_id = sqlTable.getFieldIndex("packages_package_id");
	int fFile_name = sqlTable.getFieldIndex("file_name");

	vector<string> fileNames;
	vector<int> package_ids;
	int package_id;
	if (!sqlTable.empty() && installedPackage) {
		for (unsigned int k=0;k<sqlTable.getRecordCount() ;k++) {
			package_id=atoi(sqlTable.getValue(k, fPackages_package_id).c_str());
			if (package_id!=installedPackage->get_id()) {
				fileNames.push_back(sqlTable.getValue(k, fFile_name));
				package_ids.push_back(package_id);
			}
		}
	}
	if (!fileNames.empty()) {
		warning("File conflicts detected");
		warningSummary += "CONFLICTS:\n---------------\n";
	       	for (unsigned int i=0; i<fileNames.size(); ++i) {
			warningSummary += fileNames[i] + "\n";
		}
		warningSummary += "----------------\n";
		WriteFileStrings("/tmp/mpkg-validate-conflicts.log", fileNames);
	}
	else ok(_("No conflicts detected"));
	fileNames.clear();
	// Check dependencies resolvable
	core.DepTracker->addToInstallQuery(*p);
	core.DepTracker->renderData();
	if (!core.DepTracker->get_failure_list().IsEmpty()) {
		blocking(_("Unresolvable dependencies"));
		blockingSummary += depErrorTable.print().c_str();
	}
	else ok(_("Dependencies are resolvable using installed or available packages"));
	// Checking descriptions
	if (p->get_short_description().empty() || p->get_short_description()=="0") {
		warning(_("No short description"));
	}
	else ok(_("Short description is good"));

	if (p->get_description().empty() || p->get_description()=="0") {
		warning(_("No description"));
	}
	else ok(_("Description is good"));
	// Maintainer
	if (p->get_packager().empty() || p->get_packager()=="0") {
		warning(_("No maintainer name specified"));
	}
	else ok(_("Maintainer name is good"));
	if (p->get_packager_email().empty() || p->get_packager_email()=="0") {
		warning(_("No maintainer email specified"));
	}
	else ok(_("Maintainer email is good"));
	if (p->get_tags().empty()) {
		warning(_("No tags specified"));
	}
	else ok(_("Package tagged"));
	// Check version
	PACKAGE_LIST plist;
	for (unsigned int i=0; i<pkgList.size(); ++i) {
		if (pkgList[i].get_name()==p->get_name()) plist.add(pkgList[i]);
	}
	plist.add(*p);
	plist.initVersioning();
	PACKAGE *maxV = plist.getMaxVersion();
	if (maxV) {
		if (maxV->get_md5() == p->get_md5() && maxV->get_id() == p->get_id()) {
			ok(_("Version ok"));
		}
		else {
			if (maxV->get_md5()==p->get_md5()) {
				warning(_("Package already in repository"));
			}
			else {
				if (strverscmp2(maxV->get_fullversion(), p->get_fullversion())>0) {
					warning(_("Deprecated version: there is already ") + maxV->get_fullversion());
				}
				else warning(_("Can't understand what with versioning"));
			}
		}
	}
	printf("\n\nSUMMARY for %s-%s [%s]\n\nFatal errors: %d\nBlocking errors: %d\nWarnings: %d\nPASSED: %d\n", p->get_name().c_str(), p->get_fullversion().c_str(), filename.c_str(), fatalCount, blockingCount, warningCount, okCount);
	fprintf(stderr, "%s: FATAL: %d, BLOCK: %d, WARN: %d, OK: %d\n", filename.c_str(), fatalCount, blockingCount, warningCount, okCount);
	string pkgSummary = fatalSummary + blockingSummary + warningSummary;
	if (pkgSummary.empty()) pkgSummary = "All tests passed OK";
	WriteFile(filename+".validate", pkgSummary);
	if (!moveFatalsTo.empty()) {
		if (fatalCount) {
			moveFile(filename, moveFatalsTo+"/");
			moveFile(filename+".validate", moveFatalsTo+"/");
		}
	}
	if (!moveBlockingTo.empty()) {
		if (blockingCount) {
			moveFile(filename, moveBlockingTo+"/");
			moveFile(filename+".validate", moveBlockingTo+"/");
		}
	}


	return 0;
}
int checkTreeCallback(const char *filename, const struct stat *file_status, int filetype) {
	unsigned short x=0, y=0;

	if (file_status->st_ino!=0) x=y;

       	string ext = getExtension(filename);
	bool pkgOk = false;
	if (filetype==FTW_F) {
		if (ext=="spkg" && !enableSpkgIndexing) return 0;
		if (ext == "tgz" || ext == "tlz" || ext == "spkg" || ext == "txz" || ext == "tbz" ) pkgOk=true;
	}
	vector<string> tmp;
	if (getFilename(filename)==".mpkg_skip") {
		printf("%s: requested to skip\n", filename);
		pkgOk=false;
	}
	if (pkgOk) checkPackage(filename);

	return 0;

}

int checkTree(string path=".") {
	ftw(path.c_str(), checkTreeCallback, 600);
	return 0;
}

int main(int argc, char **argv) {
	bool treeMode=false;
	string filename;
	if (string(argv[0])=="mpkg-validate-tree") treeMode=true;
	else {
		if (argc<2) return print_usage(1);
		filename=string(argv[1]);
	}
	if (argc>3-treeMode) {
		moveFatalsTo=argv[2-treeMode];
		moveBlockingTo=argv[3-treeMode];
	}
	printf("Options get\n");
	if (!treeMode) return checkPackage(filename);
	else return checkTree();
}

