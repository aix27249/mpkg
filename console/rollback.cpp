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
	fprintf(stderr, _("mpkg-rollback: rolls back installed package set to specified state.\nUsage: \n\tmpkg-rollback TRANSACTION_ID\tRolls back up to specified transaction. All transactions with ID equal or greater than specified TRANSACTION_ID will be reverted.\n"));
	return 1;
}
int main(int argc, char **argv) {
	interactive_mode=true;

	if (argc<2) return print_usage();

	int rollback_to = atoi(argv[1]);
	if (rollback_to==0) return print_usage();


	SQLRecord sqlSearch;
	SQLTable transactions;
	SQLTable transaction_details;

	mpkg core;
	core.db->get_sql_vtable(transactions, sqlSearch, "transactions", sqlSearch);
	core.db->get_sql_vtable(transaction_details, sqlSearch, "transaction_details", sqlSearch);

	int fTr_id=0;
	int fTr_date_start=1;
	int fTr_date_end=2;
	int fTr_status=3;
	int fTrd_tid=1;
	int fTrd_pkgname=2;
	int fTrd_pkgver=3;
	int fTrd_pkgbuild=4;
	int fTrd_action=6;
	int fTrd_date=7;



	vector<string> ip_name, ip_version, ip_build, rp_name;


	for (size_t i=0; i<transactions.size(); ++i) {
		printf("%sRolling back transaction ID:%s %s\nStarted: %s\nFinished at: %s\nStatus: %s\n", CL_GREEN, CL_WHITE, transactions.getValue(i, fTr_id).c_str(), transactions.getValue(i, fTr_date_start).c_str(), transactions.getValue(i, fTr_date_end).c_str(), decodeStatus(transactions.getValue(i, fTr_status)).c_str());

		for (size_t t=0; t<transaction_details.size(); ++t) {
			if (atoi(transactions.getValue(i, fTr_id).c_str())<rollback_to) continue;
			if (transaction_details.getValue(t, fTrd_tid)!=transactions.getValue(i, fTr_id)) continue;
			if (transaction_details.getValue(t, fTrd_action)!="0") continue; // Install only

			rp_name.push_back(transaction_details.getValue(t, fTrd_pkgname));
			
			// Search for such package in ip_ vectors, and remove them if they are found there
			for (size_t q=0; q<ip_name.size(); ++q) {
				if (ip_name[q]==transaction_details.getValue(t, fTrd_pkgname)) {
					ip_name.erase(ip_name.begin()+q);
					ip_version.erase(ip_version.begin()+q);
					ip_build.erase(ip_build.begin()+q);
					break;
				}

			}
		}

		for (size_t t=0; t<transaction_details.size(); ++t) {
			if (atoi(transactions.getValue(i, fTr_id).c_str())<rollback_to) continue;
			if (transaction_details.getValue(t, fTrd_tid)!=transactions.getValue(i, fTr_id)) continue;
			if (transaction_details.getValue(t, fTrd_action)!="1") continue; // Remove only

			// Look in remove queue. If such package is there, remove it
			for (size_t q=0; q<rp_name.size(); ++q) {
				if (rp_name[q]==transaction_details.getValue(t, fTrd_pkgname)) {
					rp_name.erase(rp_name.begin()+q);
					break;
				}

			}

			// Look in install queue. If such package is already there, erase it
			for (size_t q=0; q<ip_name.size(); ++q) {
				if (ip_name[q]==transaction_details.getValue(t, fTrd_pkgname)) {
					ip_name.erase(ip_name.begin()+q);
					ip_version.erase(ip_version.begin()+q);
					ip_build.erase(ip_build.begin()+q);
					break;
				}

			}


			ip_name.push_back(transaction_details.getValue(t, fTrd_pkgname));
			ip_version.push_back(transaction_details.getValue(t, fTrd_pkgver));
			ip_build.push_back(transaction_details.getValue(t, fTrd_pkgbuild));
		}
	}
	
	// Committing queue.
	// First, reset it
	core.clean_queue();

	// Now, add purge and install queue
	core.purge(rp_name);
	core.install(ip_name, &ip_version, &ip_build);

	// And finally, commit actions
	return core.commit();


}
