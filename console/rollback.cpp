/* MPKG package system
 * System rollback tool
 *
 */

#include <mpkg/libmpkg.h>
string decodeStatus(string status) {
	if (status=="0") return  string(CL_RED) + "FAILED" + string(CL_WHITE);
	if (status=="1") return string(CL_GREEN) + "OK" + string(CL_WHITE);
	return "UNKNOWN";
}
int print_usage() {
	fprintf(stderr, _("mpkg-rollback: rolls back installed package set to specified state.\nUsage: \n\tmpkg-rollback [OPTIONS] TRANSACTION_ID\tRolls back up to specified transaction. All transactions with ID equal or greater than specified TRANSACTION_ID will be reverted.\n\nAvailable options:\n\t-v\t--verbose\tVerbose info\n"));
	return 1;
}

void printWarning(string type, string name, string ver, string build) {
	mWarning("Cannot find " + name + " " + ver + "-" + build + " within available packages. Rollback may be incomplete.");
}
int main(int argc, char **argv) {
	interactive_mode=true;
	setlocale(LC_ALL, "");
	bindtextdomain( "mpkg", "/usr/share/locale");
	textdomain("mpkg");


	if (argc<1) return print_usage();
	string rollback_package;
	bool verbose = false;
	int rollback_to = 0;
	for (int i=1; i<argc; ++i) {
		if (string(argv[i])=="-z" || string(argv[i])=="--no-dep") ignoreDeps = true;
		else if (string(argv[i])=="-v" || string(argv[i])=="--verbose") verbose = true;
		else if (atoi(argv[i])>0) {
			rollback_to = atoi(argv[i]);
		}
		else if (IntToStr(atoi(argv[i]))==string(argv[i])) return print_usage();
		else rollback_package = argv[i];
	}
	
	if (getuid()>0) {
		mError("This program should be run as root.");
		return 1;
	}

	printf(_("Gathering data\n"));
	SQLRecord sqlSearch;
	SQLTable transactions;
	SQLTable transaction_details;

	mpkg core;

	core.clean_queue();
	core.db->get_sql_vtable(transactions, sqlSearch, "transactions", sqlSearch);
	core.db->get_sql_vtable(transaction_details, sqlSearch, "transaction_details", sqlSearch);

	PACKAGE_LIST pkgListModified;
	core.get_packagelist(sqlSearch, &pkgListModified);

	printf(_("Searching for transactions\n"));
	
	int fTr_id=0;
	//int fTr_date_start=1;
	int fTr_date_end=2;
	//int fTr_status=3;
	int fTrd_tid=1;
	int fTrd_pkgname=2;
	int fTrd_pkgver=3;
	int fTrd_pkgbuild=4;
	int fTrd_md5=5;
	int fTrd_action=6;
	//int fTrd_date=7;



	vector<string> ip_name, ip_version, ip_build, rp_name;
	int pkgNum;

	if (rollback_to==0) {
		// Get latest transaction number
		if (!rollback_package.empty()) {
			printf(_("Searching for latest transaction with %s\n"), rollback_package.c_str());
			for (size_t t=transaction_details.size()-1; (int) t>=0; --t) {
				if (transaction_details.getValue(t, fTrd_pkgname)!=rollback_package) continue;
				rollback_to = atoi(transaction_details.getValue(t, fTrd_tid).c_str());
				printf(_("Found transaction %d\n"), rollback_to);
				break;
			}
			if (rollback_to==0) {
				printf(_("No actions with package %s in history\n"), rollback_package.c_str());
				return 0;
			}
		}
		if (rollback_to==0) rollback_to = atoi(transactions.getValue(transactions.size()-1, fTr_id).c_str());
	}

	// Going thru transactions from the end.
	
	for (size_t i=transactions.size()-1; (int) i>=0; i--) {
		// If ID is less than required, skip it
		if (atoi(transactions.getValue(i, fTr_id).c_str())<rollback_to) continue;
		// Print info
		printf("%sRolling back transaction %s%s (%s)\n", CL_GREEN, CL_WHITE, transactions.getValue(i, fTr_id).c_str(), transactions.getValue(i, fTr_date_end).c_str());

		// Packages that was installed
		for (size_t t=transaction_details.size()-1; (int) t>=0; --t) {
			if (transaction_details.getValue(t, fTrd_tid)!=transactions.getValue(i, fTr_id)) continue; // Skip other transaction details
		
			// If package name specified, skip others
			if (!rollback_package.empty() && transaction_details.getValue(t, fTrd_pkgname)!=rollback_package) continue;
			// Now look by MD5.
			pkgNum = pkgListModified.getPackageNumberByMD5(transaction_details.getValue(t, fTrd_md5));
			if (pkgNum==-1) {
				// This is not fatal, but can cause incomplete rollback in some cases. Warn user about this.
				printWarning(transaction_details.getValue(t, fTrd_action), transaction_details.getValue(t, fTrd_pkgname), transaction_details.getValue(t, fTrd_pkgver), transaction_details.getValue(t, fTrd_pkgbuild));
				continue;
			}
			if (transaction_details.getValue(t, fTrd_action)=="0") {
				if (!pkgListModified[pkgNum].installed()) pkgListModified.get_package_ptr(pkgNum)->set_action(ST_NONE, "");
				else pkgListModified.get_package_ptr(pkgNum)->set_action(ST_REMOVE, "rollback");
				if (verbose) printf("--- %s %s\n", pkgListModified[pkgNum].get_name().c_str(), pkgListModified[pkgNum].get_fullversion().c_str());
			}
			else {
				if (pkgListModified[pkgNum].installed()) pkgListModified.get_package_ptr(pkgNum)->set_action(ST_NONE, "");
				else pkgListModified.get_package_ptr(pkgNum)->set_action(ST_INSTALL, "rollback");
				if (verbose) printf("+++ %s %s\n", pkgListModified[pkgNum].get_name().c_str(), pkgListModified[pkgNum].get_fullversion().c_str());
			}
			

		}
	}	
	// Committing queue.

	for (size_t i=0; i<pkgListModified.size(); ++i) {
		if (pkgListModified[i].action()==ST_REMOVE) rp_name.push_back(pkgListModified[i].get_name());
		if (pkgListModified[i].action()==ST_INSTALL) {
			ip_name.push_back(pkgListModified[i].get_name());
			ip_version.push_back(pkgListModified[i].get_version());
			ip_build.push_back(pkgListModified[i].get_build());
		}
	}
	// Now, add purge and install queue
	core.purge(rp_name);
	core.install(ip_name, &ip_version, &ip_build);

	// And finally, commit actions
	return core.commit();


}
