/*************************************************************************************
 * 	SQL pool for MPKG packaging system
 * 	Currently supports SQLite only.
 * 	
 *	$Id: sql_pool.h,v 1.21 2007/08/24 06:20:52 i27249 Exp $
 ************************************************************************************/



#ifndef SQL_POOL_H_
#define SQL_POOL_H_
#include <sqlite3.h>
#include "debug.h"
#include "core.h"
#include "config.h"
class SQLiteDB
{
	private:
		string db_filename;
		int sqlError;
		bool initOk;
		string sqlErrMsg;
		string lastSQLQuery;
		int sql_exec_c (const char* sql_query);
		int initDatabaseStructure();
		bool CheckDatabaseIntegrity(); // Checks database integrity
		int get_sql_table (const string& sql_query, char ***table, int *rows, int *cols); // fills table
		sqlite3 *db; //Database is open all the time during work (added: aix27249, for optimization reasons)
	public:

		int sql_exec (const string &sql_query);
		const vector<string> getFieldNames(const string& table_name) const;
		int getLastError();
		const string& getLastErrMsg();
		int get_sql_vtable(SQLTable &output, const SQLRecord& fields, const string &table_name, const SQLRecord& search);
		//int get_sql_vtable_by_query(string *query, SQLTable *output);
		int init();
		int sqlBegin();
		int sqlCommit();
		int sqlFlush();
		long long int getLastID();

		int clear_table(const string& table_name);
		//int sql_insert(string table_name, SQLRecord values);
		int sql_insert(const string& table_name, const SQLRecord& values);
		//int sql_insert(string table_name, SQLTable values);
		int sql_insert(const string& table_name, const SQLTable& values);
		int sql_update(const string& table_name, const SQLRecord& fields, const SQLRecord& search);
		int sql_delete(const string& table_name, const SQLRecord& search);

		int64_t last_insert_id();
		bool canWriteDB();
		
		SQLiteDB(string filename=DB_FILENAME, bool skip_integrity_check = false);
		~SQLiteDB();
};

class SQLProxy
{
	private:
		SQLiteDB *sqliteDB;
	public:

		bool internalDataChanged;
		long long int getLastID();
		int getLastError();
		const string& getLastErrMsg();
		int clear_table(const string& table_name);
		int get_sql_vtable(SQLTable& output, const SQLRecord& fields, const string& table_name, const SQLRecord& search);
		//int get_sql_vtable_by_query(string *query, SQLTable *output);
		int sql_insert(const string& table_name, const SQLRecord& values);
		int sql_insert(const string& table_name, const SQLTable& values);
		int sql_update(const string& table_name, const SQLRecord& fields, const SQLRecord& search);
		int sql_delete(const string& table_name, const SQLRecord& search);
		int sql_exec(const string& query);
		int sqlCommit();
		int sqlBegin();
		int sqlFlush();
		int64_t last_insert_id();

		void closeDBConnection();


		SQLProxy();
		~SQLProxy();
};

bool lockDatabase();
bool unlockDatabase();
bool isDatabaseLocked();
bool backupDatabase();
bool restoreDatabaseFromBackup();

#endif
