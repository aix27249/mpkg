#ifndef MPKG_TRANSACTIONS_H__
#define MPKG_TRANSACTIONS_H__

#include "dataunits.h"
#include "sql_pool.h"

int startTransaction(const PACKAGE_LIST &install_list, const PACKAGE_LIST &remove_list, SQLProxy *db);
void endTransaction(int transaction_id, SQLProxy *db);
void recordPackageTransaction(const PACKAGE &pkg, int action, int transaction_id, SQLProxy *db);
#endif
