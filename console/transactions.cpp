/* MPKG transactions module
 *
 * Shows transactions
 */

#include <mpkg/libmpkg.h>

int show_usage() {
	return 1;
}
string decodeStatus(string status) {
	if (status=="0") return  string(CL_RED) + "FAILED" + string(CL_WHITE);
	if (status=="1") return string(CL_GREEN) + "OK" + string(CL_WHITE);
	return "UNKNOWN";
}
int main(int argc, char **argv) {
	int ich;
	const char* short_opt = "c:h";
	const struct option long_options[] =  {
		{ "help",		0, NULL,	'h'},
		{ "count", 		1, NULL,	'c'},
		{ NULL, 		0, NULL, 	0}
	};

	int max_num=0;
	do {
		ich = getopt_long(argc, argv, short_opt, long_options, NULL);
		

		switch (ich) {
			case 'h':
					return show_usage();
			case 'c':
					max_num = atoi(optarg);
					break;
			default:
					break;
		}
	} while (ich != -1);

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
	for (size_t i=0; i<transactions.size(); ++i) {
		printf("%sTransaction ID:%s %s\nStarted: %s\nFinished at: %s\nStatus: %s\n", CL_GREEN, CL_WHITE, transactions.getValue(i, fTr_id).c_str(), transactions.getValue(i, fTr_date_start).c_str(), transactions.getValue(i, fTr_date_end).c_str(), decodeStatus(transactions.getValue(i, fTr_status)).c_str());

		printf("Installed:\n");
		for (size_t t=0; t<transaction_details.size(); ++t) {
			if (transaction_details.getValue(t, fTrd_tid)!=transactions.getValue(i, fTr_id)) continue;
			if (transaction_details.getValue(t, fTrd_action)!="0") continue; // Install only
			printf("+\t%s %s-%s, at %s\n", transaction_details.getValue(t, fTrd_pkgname).c_str(), transaction_details.getValue(t, fTrd_pkgver).c_str(), transaction_details.getValue(t, fTrd_pkgbuild).c_str(), transaction_details.getValue(t, fTrd_date).c_str());
		}

		printf("Removed:\n");
		for (size_t t=0; t<transaction_details.size(); ++t) {
			if (transaction_details.getValue(t, fTrd_tid)!=transactions.getValue(i, fTr_id)) continue;
			if (transaction_details.getValue(t, fTrd_action)!="1") continue; // Install only
			printf("-\t%s %s-%s, at %s\n", transaction_details.getValue(t, fTrd_pkgname).c_str(), transaction_details.getValue(t, fTrd_pkgver).c_str(), transaction_details.getValue(t, fTrd_pkgbuild).c_str(), transaction_details.getValue(t, fTrd_date).c_str());
		}
		printf("\n\n");
	}



}
