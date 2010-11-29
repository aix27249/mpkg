#include "transactions.h"

int startTransaction(const PACKAGE_LIST &install_list, const PACKAGE_LIST &remove_list, SQLProxy *db) {
	string to_install, to_remove;
	for (size_t i=0; i<install_list.size(); ++i) {
		to_install += install_list[i].get_name() + "-" + install_list[i].get_fullversion() + " ";
	}
	for (size_t i=0; i<remove_list.size(); ++i) {
		to_remove += remove_list[i].get_name() + "-" + remove_list[i].get_fullversion() + " ";
	}

	db->sql_exec("INSERT INTO transactions VALUES (NULL, datetime('now', 'localtime'), 0, 0, 'Install request: " + to_install + ", total: " + IntToStr(install_list.size()) + " packages, remove request: " + to_remove + ", total: " + IntToStr(remove_list.size()) + " packages');");
	return db->last_insert_id();
}


void endTransaction(int transaction_id, SQLProxy *db) {
	db->sql_exec("UPDATE TRANSACTIONS SET t_end=datetime('now', 'localtime'), t_status=1 WHERE id=" + IntToStr(transaction_id) + ";");
}

void recordPackageTransaction(const PACKAGE &pkg, int action, int transaction_id, SQLProxy *db) {
	db->sql_exec("INSERT INTO transaction_details VALUES (NULL, " + IntToStr(transaction_id) + ", '" + pkg.get_name() + "', '" + pkg.get_version() + "', '" + pkg.get_build() + "', '" + pkg.get_md5() + "', " + IntToStr(action) + ", datetime('now', 'localtime'));");
}
