
/*************************************************************************************
 * 	SQL pool for MPKG packaging system
 * 	Currently supports SQLite only. Planning support for other database servers
 * 	in future (including networked)
 *	$Id: sql_pool.cpp,v 1.52 2007/11/22 15:32:57 i27249 Exp $
 ************************************************************************************/

#define SQLMATRIX_LIMIT 250
#include "sql_pool.h"
#include "dbstruct.h"
#include <mpkgsupport/mpkgsupport.h>
#include "errorhandler.h"
int b_t = 0;
int c_t = 0;
const string MPKGTableVersion="1.11";
bool SQLiteDB::canWriteDB() {
	if (access(db_filename.c_str(), R_OK|W_OK)==0) return true;
	return false;

}
bool SQLiteDB::CheckDatabaseIntegrity()
{
	if (\
			sql_exec("select dependency_id, packages_package_id, dependency_condition, dependency_type, dependency_package_name, dependency_package_version from dependencies limit 1;")!=0 || \
			sql_exec("select file_id, file_name, file_type, packages_package_id from files limit 1;")!=0 || \
			sql_exec("select conflict_id, conflict_file_name, backup_file, conflicted_package_id from conflicts limit 1;")!=0 || \
			sql_exec("select location_id, packages_package_id, server_url, location_path from locations limit 1;")!=0 || \
			sql_exec("select package_id, package_name, package_version, package_arch, package_build, package_compressed_size, package_installed_size, \
					package_short_description, package_description, package_changelog, package_packager, package_packager_email, \
					package_installed, package_configexist, package_action, package_md5, package_filename from packages limit 1;")!=0 || \
			sql_exec("select tags_id, tags_name from tags limit 1;")!=0 || \
			sql_exec("select tags_link_id, packages_package_id, tags_tag_id from tags_links limit 1;")!=0)
		/* || \
			sql_exec("select description_id, packages_package_id, description_language, description_text, short_description_text from descriptions limit 1;")!=0 || \
			sql_exec("select changelog_id, packages_package_id, changelog_language, changelog_text from changelogs limit 1;")!=0 || \
			sql_exec("select rating_id, rating_value, packages_package_name from ratings limit 1;")!=0 \
			)*/
	{
		return false;
	}
	else 
	{
		hideErrors = true;
		if (sql_exec("select version from mpkg_tableversion;")!=0)
		{
			hideErrors = false;
			// Mean we have an old 1.0 table version. Have to update!
			if (!canWriteDB() || _cmdOptions["sql_readonly"]=="yes") {
				mError(_("Your database need to be upgraded to new version ") + MPKGTableVersion +_(" (current version: 1.0).\nPlease run mpkg (e.g. 'mpkg list') as root to do this\nNote: it will be backward-compatible with all previous versions of mpkg"));
				return false;
			}
			sql_exec("create table mpkg_tableversion (version TEXT NOT NULL);");
			sql_exec("insert into mpkg_tableversion values ('"+MPKGTableVersion+"');");
			if (!dialogMode) say(_("Database format has been upgraded to version %s\n"), MPKGTableVersion.c_str());
		}
		hideErrors = true;
		if (sql_exec("select package_betarelease from packages limit 1;")!=0)
		{
			hideErrors = false;
			if (!canWriteDB() || _cmdOptions["sql_readonly"]=="yes") {
				mError(_("Your database need to be upgraded to new version ") + MPKGTableVersion +_(".\nPlease run mpkg (e.g. 'mpkg list') as root to do this\nNote: it will be backward-compatible with all previous versions of mpkg"));
				return false;
			}

			sql_exec("alter table packages add package_betarelease TEXT NULL;");
			//say(_("Added field package_betarelease to table packages\n"));
		}
		hideErrors = true;
		if (sql_exec("select package_installed_by_dependency from packages limit 1;")!=0)
		{
			hideErrors = false;
			if (!canWriteDB() || _cmdOptions["sql_readonly"]=="yes") {
				mError(_("Your database need to be upgraded to new version ") + MPKGTableVersion +_(".\nPlease run mpkg (e.g. 'mpkg list') as root to do this\nNote: it will be backward-compatible with all previous versions of mpkg"));
				return false;
			}

			sql_exec("alter table packages add package_installed_by_dependency INTEGER NOT NULL DEFAULT '0';");
			//say(_("Added field package_installed_by_dependency to table packages\n"));
		}
		hideErrors = true;
		if (sql_exec("select dependency_build_only from dependencies limit 1;")!=0)
		{
			hideErrors = false;
			if (!canWriteDB() || _cmdOptions["sql_readonly"]=="yes") {
				mError(_("Your database need to be upgraded to new version ") + MPKGTableVersion +_(".\nPlease run mpkg (e.g. 'mpkg list') as root to do this\nNote: it will be backward-compatible with all previous versions of mpkg"));
				return false;
			}

			sql_exec("alter table dependencies add dependency_build_only INTEGER NOT NULL DEFAULT '0';");
			//say(_("Added field dependency_build_only to table dependencies\n"));
		}
		hideErrors = true;
		if (sql_exec("select package_type from packages limit 1;")!=0)
		{
			hideErrors = false;
			if (!canWriteDB() || _cmdOptions["sql_readonly"]=="yes") {
				mError(_("Your database need to be upgraded to new version ") + MPKGTableVersion +_(".\nPlease run mpkg (e.g. 'mpkg list') as root to do this\nNote: it will be backward-compatible with all previous versions of mpkg"));
				return false;
			}

			sql_exec("alter table packages add package_type INTEGER NOT NULL DEFAULT '0';");
			//say(_("Added field package_type to table packages\n"));
		}
		hideErrors = true;
		if (sql_exec("select package_add_date from packages limit 1;")!=0)
		{
			hideErrors = false;
			if (!canWriteDB() || _cmdOptions["sql_readonly"]=="yes") {
				mError(_("Your database need to be upgraded to new version ") + MPKGTableVersion +_(".\nPlease run mpkg (e.g. 'mpkg list') as root to do this\nNote: it will be backward-compatible with all previous versions of mpkg"));
				return false;
			}

			sql_exec("alter table packages add package_add_date INTEGER NOT NULL DEFAULT '0';");
			//say(_("Added field package_add_date to table packages\n"));
		}
		hideErrors = true;
		if (sql_exec("select package_build_date from packages limit 1;")!=0)
		{
			hideErrors = false;
			if (!canWriteDB() || _cmdOptions["sql_readonly"]=="yes") {
				mError(_("Your database need to be upgraded to new version ") + MPKGTableVersion +_(".\nPlease run mpkg (e.g. 'mpkg list') as root to do this\nNote: it will be backward-compatible with all previous versions of mpkg"));
				return false;
			}

			sql_exec("alter table packages add package_build_date INTEGER NOT NULL DEFAULT '0';");
			//say(_("Added field package_build_date to table packages\n"));
		}
		hideErrors = true;
		if (sql_exec("select package_repository_tags from packages limit 1;")!=0)
		{
			hideErrors = false;
			if (!canWriteDB() || _cmdOptions["sql_readonly"]=="yes") {
				mError(_("Your database need to be upgraded to new version ") + MPKGTableVersion +_(".\nPlease run mpkg (e.g. 'mpkg list') as root to do this\nNote: it will be backward-compatible with all previous versions of mpkg"));
				return false;
			}

			sql_exec("alter table packages add package_repository_tags TEXT NULL;");
			//say(_("Added field package_build_date to table packages\n"));
		}
		if (sql_exec("select package_distro_version from packages limit 1;")!=0)
		{
			hideErrors = false;
			if (!canWriteDB() || _cmdOptions["sql_readonly"]=="yes") {
				mError(_("Your database need to be upgraded to new version ") + MPKGTableVersion +_(".\nPlease run mpkg (e.g. 'mpkg list') as root to do this\nNote: it will be backward-compatible with all previous versions of mpkg"));
				return false;
			}

			sql_exec("alter table packages add package_distro_version TEXT NULL;");
			//say(_("Added field package_build_date to table packages\n"));
		}

		if (sql_exec("select package_provides from packages limit 1;")!=0)
		{
			hideErrors = false;
			if (!canWriteDB() || _cmdOptions["sql_readonly"]=="yes") {
				mError(_("Your database need to be upgraded to new version ") + MPKGTableVersion +_(".\nPlease run mpkg (e.g. 'mpkg list') as root to do this\nNote: it will be backward-compatible with all previous versions of mpkg"));
				return false;
			}

			sql_exec("alter table packages add package_provides TEXT NULL;");
			//say(_("Added field package_build_date to table packages\n"));
		}

		if (sql_exec("select package_conflicts from packages limit 1;")!=0)
		{
			hideErrors = false;
			if (!canWriteDB() || _cmdOptions["sql_readonly"]=="yes") {
				mError(_("Your database need to be upgraded to new version ") + MPKGTableVersion +_(".\nPlease run mpkg (e.g. 'mpkg list') as root to do this\nNote: it will be backward-compatible with all previous versions of mpkg"));
				return false;
			}

			sql_exec("alter table packages add package_conflicts TEXT NULL;");
			//say(_("Added field package_build_date to table packages\n"));
		}


		hideErrors = true;
		if (sql_exec("select * from history limit 1;")!=0) {
			hideErrors = false;
			if (!canWriteDB() || _cmdOptions["sql_readonly"]=="yes") {
				mError(_("Your database need to be upgraded to new version ") + MPKGTableVersion +_(".\nPlease run mpkg (e.g. 'mpkg list') as root to do this\nNote: it will be backward-compatible with all previous versions of mpkg"));
				return false;
			}
			sql_exec("create table history (history_id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE, history_event INTEGER NOT NULL, history_data TEXT NULL);");
			//say(_("Added table history to database\n"));
		}
		hideErrors = true;
		if (sql_exec("select * from deltas limit 1;")!=0) {
			hideErrors=false;
			if (!canWriteDB() || _cmdOptions["sql_readonly"]=="yes") {
				mError(_("Your database need to be upgraded to new version ") + MPKGTableVersion +_(".\nPlease run mpkg (e.g. 'mpkg list') as root to do this\nNote: it will be backward-compatible with all previous versions of mpkg"));
				return false;
			}
			sql_exec("create table deltas (delta_id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE, packages_package_id INTEGER NOT NULL, delta_url TEXT NOT NULL, delta_md5 TEXT NOT NULL, delta_orig_filename TEXT NOT NULL, delta_orig_md5 TEXT NOT NULL);");
		}
		hideErrors = true;
		if (sql_exec("select delta_size from deltas limit 1;")!=0)
		{
			hideErrors = false;
			if (!canWriteDB() || _cmdOptions["sql_readonly"]=="yes") {
				mError(_("Your database need to be upgraded to new version ") + MPKGTableVersion +_(".\nPlease run mpkg (e.g. 'mpkg list') as root to do this\nNote: it will be backward-compatible with all previous versions of mpkg"));
				return false;
			}

			sql_exec("alter table deltas add delta_size TEXT NULL;");
			//say(_("Added field package_build_date to table packages\n"));
		}

		hideErrors = true;
		if (sql_exec("select package_id from config_files limit 1;")!=0)
		{
			hideErrors = false;
			if (!canWriteDB() || _cmdOptions["sql_readonly"]=="yes") {
				mError(_("Your database need to be upgraded to new version ") + MPKGTableVersion +_(".\nPlease run mpkg (e.g. 'mpkg list') as root to do this\nNote: it will be backward-compatible with all previous versions of mpkg"));
				return false;
			}

			sql_exec("CREATE TABLE config_files (id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,  package_id INTEGER, filename TEXT);");
		}
		hideErrors = true;
		if (sql_exec("select id from config_options limit 1;")!=0)
		{
			hideErrors = false;
			if (!canWriteDB() || _cmdOptions["sql_readonly"]=="yes") {
				mError(_("Your database need to be upgraded to new version ") + MPKGTableVersion +_(".\nPlease run mpkg (e.g. 'mpkg list') as root to do this\nNote: it will be backward-compatible with all previous versions of mpkg"));
				return false;
			}

			sql_exec("CREATE TABLE config_options (id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE, config_files_id INTEGER, name TEXT, value TEXT);");
		}

		hideErrors = true;
		if (sql_exec("select package_id from abuilds limit 1;")!=0)
		{
			hideErrors = false;
			if (!canWriteDB() || _cmdOptions["sql_readonly"]=="yes") {
				mError(_("Your database need to be upgraded to new version ") + MPKGTableVersion +_(".\nPlease run mpkg (e.g. 'mpkg list') as root to do this\nNote: it will be backward-compatible with all previous versions of mpkg"));
				return false;
			}

			sql_exec("CREATE TABLE abuilds (id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE, package_id INTEGER, url TEXT);");
		}


		hideErrors = true;
		if (sql_exec("select id from transactions limit 1;")!=0)
		{
			hideErrors = false;
			if (!canWriteDB() || _cmdOptions["sql_readonly"]=="yes") {
				mError(_("Your database need to be upgraded to new version ") + MPKGTableVersion +_(".\nPlease run mpkg (e.g. 'mpkg update') as root to do this\nNote: it will be backward-compatible with all previous versions of mpkg"));
				return false;
			}

			sql_exec("CREATE TABLE transactions (id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE, t_start TIMESTAMP, t_end TIMESTAMP, t_status INTEGER, custom_details TEXT);");
		}

		hideErrors = true;
		if (sql_exec("select id from transaction_details limit 1;")!=0)
		{
			hideErrors = false;
			if (!canWriteDB() || _cmdOptions["sql_readonly"]=="yes") {
				mError(_("Your database need to be upgraded to new version ") + MPKGTableVersion +_(".\nPlease run mpkg (e.g. 'mpkg update') as root to do this\nNote: it will be backward-compatible with all previous versions of mpkg"));
				return false;
			}

			sql_exec("CREATE TABLE transaction_details (id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE, transaction_id INTEGER, package_name TEXT, package_version TEXT, package_build TEXT, package_md5 TEXT, action_type INTEGER, action_date TIMESTAMP);");
		}



		hideErrors = false;
		return true;
	}
}

int SQLiteDB::clear_table(const string &table_name)
{
	// Функция стирает всю информацию из указанной таблицы
	/*string exec = */return sql_exec(string("delete from ")+table_name+string(";"));
	//return sql_exec(exec);
}
int get_sql_table_counter=0;
int sqldump_counter=0;
int SQLiteDB::get_sql_table (const string &sql_query, char ***table, int *rows, int *cols)
{
	//WriteFile("/tmp/sqlquery" + IntToStr(sqldump_counter), sql_query);
	sqldump_counter++;
	get_sql_table_counter++;
	char *errmsg=0;
	int query_return=0;
	//const char *qqq = sql_query->c_str();
	query_return=sqlite3_get_table(db, sql_query.c_str(), table, rows, cols, &errmsg);
	if (query_return!=SQLITE_OK) // Means error
	{
		printf("query_return=%d, SQLITE_OK=%d\n", query_return, SQLITE_OK);
		printf("%s\n", sqlite3_errmsg(db));
		perror("HOLY SHIT OF SQLite INTERNAL ERROR");
		
		printf("The query was: %s\n", sql_query.c_str());
		if (errmsg) mError((string) "SQL error while querying database: " + errmsg);
		if (errmsg) sqlError=query_return;
		if (errmsg) sqlErrMsg=errmsg;
		//free(errmsg);
		mpkgErrorHandler.callError(MPKG_SUBSYS_SQLQUERY_ERROR);
		return query_return;
	}
	
	// If all ok, free memory
	free(errmsg);
	sqlError=0;
	sqlErrMsg.clear();
	return 0;
}	

int SQLiteDB::sqlBegin()
{
	b_t++;
	return sql_exec("begin transaction;");
}

int SQLiteDB::sqlCommit()
{
	c_t++;
	if (b_t!=c_t) fprintf(stderr, "\n\n\n%sWARNING WARNING TRANSACTION COUNTER MISMATCH! PLEASE FILL BUG REPORT! %s %d vs %d\n\n\n", CL_RED, CL_WHITE, b_t, c_t);
	return sql_exec("commit transaction;");
}

int SQLiteDB::sqlFlush()
{

	if (sqlCommit() == 0 && sqlBegin() == 0) return 0;
	else
	{
		mError("Error flushing to DB!");
		return -1;
	}
}

int SQLiteDB::init()
{
	// This code may be useful for debugging in future, so I leave it here.
	/*if (verbose) {
		int tsafe = sqlite3_threadsafe();
		if (!tsafe) printf("SQLite is NOT thread-safe, bad but will work\n");
		else printf("SQLite is thread-safe, good\n");
	}*/
	int ret;
	if (_cmdOptions["sql_readonly"]=="yes") {
		ret = sqlite3_open_v2(db_filename.c_str(), &db, SQLITE_OPEN_READONLY, NULL);
		if (ret!=SQLITE_OK) {
			mError(_("Cannot open database even in read-only mode:\n") + string(sqlite3_errmsg(db)));
			return 1;
		}
	}
	else do {
		// Try to open database until it can be opened. 
		ret = sqlite3_open_v2(db_filename.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
		if (ret==SQLITE_BUSY || ret==SQLITE_LOCKED) {
			if (!dialogMode) fprintf(stderr, _("Database is locked by another process, waiting for release...\nYou can press CTRL-C to break waiting and exit\n"));
			else ncInterface.showInfoBox(_("Database is locked, waiting for release...\nYou can press CTRL-C to break waiting and exit\n"));
			sleep(1);
		}
		else if (ret!=SQLITE_OK) {
		       mError(_("Cannot open database:\n") + string(sqlite3_errmsg(db)));
		       return 1;
		}
	}
	while (ret!=SQLITE_OK);
	// Check again to be sure:
	if (ret!=SQLITE_OK) {
		mError(_("Cannot open database:\n") + string(sqlite3_errmsg(db)));
		return 1;
	}

	sqlBegin();
	return ret;

}
int64_t SQLiteDB::last_insert_id() {
	return sqlite3_last_insert_rowid(db);
}

int SQLiteDB::sql_exec (const string &sql_query) {
	return sql_exec_c(sql_query.c_str());
}
int SQLiteDB::sql_exec_c (const char *sql_query)
{
	char *sql_errmsg=0;
	int query_return;
	size_t wait_counter = 0;
	do {
		query_return=sqlite3_exec(db,sql_query,NULL, NULL, &sql_errmsg);
		if (query_return!=SQLITE_OK && query_return!=SQLITE_LOCKED && query_return!=SQLITE_BUSY) break;
		if (query_return!=SQLITE_OK) {
			if (!dialogMode) {
				fprintf(stderr, "Waiting database to unlock\n");
				if (wait_counter>2) fprintf(stderr, "\rWaiting database to unlock: %d\n", (int) wait_counter);
			}
			
			if (wait_counter<5) sleep(wait_counter+1);
			else sleep(5);
		}
		wait_counter++;
	} while (query_return==SQLITE_BUSY || query_return==SQLITE_LOCKED);
	if (query_return!=SQLITE_OK) // Means error
	{
		
		if (sql_errmsg) {
			mError("Error executing query: " + (string) sql_errmsg);
			mError("The query was: " + string(sql_query));
		}
		if (initOk)
		{
			if (sql_errmsg) mError((string)"SQL error while querying database: " + sql_errmsg);
			WriteFile("/tmp/mpkg_sql_error", string(sql_query));
			mError("The query was: " + string(sql_query));
			sqlError=query_return;
			if (sql_errmsg) sqlErrMsg=sql_errmsg;
			//free(sql_errmsg);

			mpkgErrorHandler.callError(MPKG_SUBSYS_SQLQUERY_ERROR);
		}
		return query_return;
	}

	sqlError=0;
	sqlErrMsg.clear();
	return 0;
}
long long int SQLiteDB::getLastID()
{
	return sqlite3_last_insert_rowid(db);
}
long long int SQLProxy::getLastID()
{
	if (sqliteDB==NULL) sqliteDB = new SQLiteDB;
	return sqliteDB->getLastID();
}

int SQLiteDB::get_sql_vtable(SQLTable &output, const SQLRecord& fields, const string &table_name, const SQLRecord& search) {
	if (search.size()>900) {
		mWarning("WARNING: SEARCH IN " + table_name + " IS TOO BIG: " + IntToStr(search.size()));
	}
	//printf("data received, building query\n");
	string sql_query;
	string sql_action;
	string sql_fields;
	string sql_from;
	string sql_where;

	// Do what?
	sql_action="SELECT";
	vector<string> fieldList;
	SQLRecord templateRow;
	if (fields.empty()) {
		fieldList = getFieldNames(table_name);
		for (size_t i=0; i<fieldList.size(); ++i) {
			templateRow.addField(fieldList[i]);
		}
	}
	else {
		for (size_t i=0; i<fields.size(); ++i) {
			fieldList.push_back(fields.getFieldName(i));
		}
		templateRow = fields;
	}

	for (size_t i=0; i<fieldList.size(); ++i) {
		if (i<fieldList.size()-1) sql_fields = sql_fields + fieldList[i] + ",";
		else sql_fields = sql_fields + fieldList[i];
	}

	sql_from = "FROM " + table_name;
	if (!search.empty()) {
		if (search.getEqMode()!=EQ_EQUAL && search.getSearchMode()==SEARCH_IN) {
			mWarning("SEARCH_IN is not supported for like statements, result may be not as you expected");
		}
		sql_where="WHERE ";
		if (search.getSearchMode()==SEARCH_IN) {
			sql_where = sql_where + search.getFieldName(0) + " IN (";
			//Building IN statements
			for (size_t i=0; i<search.size(); ++i) {
				if (i<search.size()-1) sql_where = sql_where + "?,";
				else sql_where = sql_where + "?)";
			}
		}
		else { // LIKE mode
			for (size_t i=0; i<search.size(); ++i) {
				switch(search.getEqMode()) {
					case EQ_EQUAL:
						sql_where = sql_where + search.getFieldName(i) + "=?";
						break;
					case EQ_LIKE:
						sql_where = sql_where + search.getFieldName(i) + " LIKE '%' || ? || '%'";
						break;
					case EQ_CUSTOMLIKE:
						sql_where = sql_where + search.getFieldName(i) + " LIKE ?";
						break;
				}

				if (i<search.size()-1) {
					switch(search.getSearchMode()) {
						case SEARCH_AND: 
							sql_where = sql_where + " AND ";
							break;
						case SEARCH_OR: 
							sql_where = sql_where + " OR ";
							break;
					}
				}
			}
		}
	}
	string order, group;
	// Note here: leading space IS REQUIRED.
	if (table_name == "packages") order = " ORDER BY package_name";
	if (!search.orderBy.empty()) order = " ORDER BY " + search.orderBy;
	if (!search.groupBy.empty()) group = " GROUP BY " + search.groupBy;
	sql_query=sql_action + " " + sql_fields + " " + sql_from + " " + sql_where + group + order + ";";
	
	// Now, query is complete, bind values
	sqlite3_stmt *stmt;
	int p_ret = sqlite3_prepare_v2(db, sql_query.c_str(), -1, &stmt, NULL);
	if (p_ret!=SQLITE_OK) {
		printf("Prepare fails, ret: %d\n", p_ret);
		printf("query:\n%s\n", sql_query.c_str());
		return p_ret;
	}

	for (size_t i=0; i<search.size(); ++i) {
		sqlite3_bind_text(stmt, i+1, search.getValueI(i).c_str(), -1, SQLITE_STATIC); // NOTE: index in sqlite3_bind_* are counted from 1, not 0! It made me cry when I tried to get this work, btw...
	}

	// Exec!
	output.clear(); // Ensure that output is clean and empty
	SQLRecord row; 	// One row of data
	int sql_ret;
	const char *str;
	do {
		sql_ret = sqlite3_step(stmt);
		if (sql_ret!=SQLITE_ROW) {
			break;
		}
		row = templateRow;
		for (unsigned int i=0; i<(unsigned int) fieldList.size(); ++i) {
			str = (const char *) sqlite3_value_text(sqlite3_column_value(stmt, i));
			if (str) row.setValue(i, (const char *) sqlite3_column_text(stmt, i));
		}
		output.addRecord(row);
	}
	while (sql_ret==SQLITE_ROW);

	sqlite3_finalize(stmt);
	if (sql_ret==SQLITE_DONE) return SQLITE_OK;
	else return sql_ret;
}


/*int SQLiteDB::get_sql_vtable_old(SQLTable &output, const SQLRecord& fields, const string &table_name, const SQLRecord& search)
{
	//printf("data received, building query\n");
	string sql_query;
	string sql_action;
	mstring sql_fields;
	string sql_from;
	mstring sql_where;
	vector<mstring> sqlMatrix;
	int limiter = SQLMATRIX_LIMIT;
	char **table;
	int cols;
	int rows;
	// Do what?
	sql_action="select";
	SQLRecord tmp_fields;
	SQLRecord *fieldptr=(SQLRecord *) &fields;
	// Get what?
	if (fields.empty()) // If fields is empty, just get all fields
	{
		fieldptr=&tmp_fields;
		//mDebug("Fields empty");
		vector<string> fieldnames=getFieldNames(table_name); // Get field names to fill
		for (unsigned int i=0; i<fieldnames.size(); i++)
			fieldptr->addField(fieldnames[i]);
	}
	for (unsigned int i=0; i<fieldptr->size(); i++) // Otherwise, get special fields
	{
		sql_fields+=fieldptr->getFieldName(i);
		if (i!=fieldptr->size()-1) sql_fields+=", ";
	}
	

	sql_from = "from "+table_name;
	if (!search.empty())
	{
		if (search.getEqMode()!=EQ_EQUAL && search.getSearchMode()==SEARCH_IN) {
			mWarning("SEARCH_IN is not supported for like statements, result may be not as you expected");
		}
		sql_where="where ";
		if (search.getSearchMode()==SEARCH_IN)
		{
			sql_where += search.getFieldName(0) + " in (";
			//printf("Building IN statements\n");
			for (unsigned int i=0; i<search.size(); i++)
			{
				if (limiter == SQLMATRIX_LIMIT) {
					sqlMatrix.resize(sqlMatrix.size()+1);
					limiter = 0;
				}
				sqlMatrix[sqlMatrix.size()-1] += "'" + search.getValueI(i)+"'";
				if (i!=search.size()-1) sqlMatrix[sqlMatrix.size()-1] +=", ";
				limiter++;
			}
			//printf("composing matrix\n");
			for (unsigned int i=0; i<sqlMatrix.size(); i++) {
				sql_where += sqlMatrix[i];
				sqlMatrix[i].clear();
			}
			sqlMatrix.clear();
			sql_where += ")";
			//printf("IN statement done\n");
		}
		else
		{
			//printf("LIKE statements\n");
			for (unsigned int i=0; i<search.size(); i++)
			{
				if (limiter == SQLMATRIX_LIMIT) {
					sqlMatrix.resize(sqlMatrix.size()+1);
					limiter = 0;
				}

				if (search.getEqMode()==EQ_EQUAL) sqlMatrix[sqlMatrix.size()-1] += search.getFieldName(i) + "='" + search.getValueI(i)+"'";
				if (search.getEqMode()==EQ_LIKE) sqlMatrix[sqlMatrix.size()-1] += search.getFieldName(i) + " like '%" + search.getValueI(i)+"%'";
				if (search.getEqMode()==EQ_CUSTOMLIKE) sqlMatrix[sqlMatrix.size()-1] += search.getFieldName(i) + " like '" + search.getValueI(i)+"'";

				if (i!=search.size()-1) {
					if (search.getSearchMode()==SEARCH_AND) sqlMatrix[sqlMatrix.size()-1] += " and ";
					if (search.getSearchMode()==SEARCH_OR) sqlMatrix[sqlMatrix.size()-1] += " or ";
				}
				limiter++;
			}
			for (unsigned int i=0; i<sqlMatrix.size(); i++) {
				sql_where += sqlMatrix[i];
				sqlMatrix[i].clear();
			}
			sqlMatrix.clear();
			//printf("LIKE complete\n");
		}
	}
	string order, group;
	if (table_name == "packages") order = " order by package_name";
	if (!search.orderBy.empty()) order = " order by " + search.orderBy;
	if (!search.groupBy.empty()) group = " group by " + search.groupBy;
	sql_query=sql_action+" "+sql_fields.s_str()+" "+sql_from+" "+sql_where.s_str() + group + order + ";";
//	if (table_name == "packages" ) + " order by package_name;";
//	else sql_query = sql_action+" "+sql_fields.s_str()+" "+sql_from+" "+sql_where.s_str() + ";";
	//printf("Query complete, requesting\n");
#ifdef DEBUG
	lastSQLQuery=sql_query;
#endif
	int sql_ret=get_sql_table(sql_query, &table, &rows, &cols);
	//printf("SQL Request complete, parsing results\n");
	if (sql_ret==0)
	{
		output.clear(); // Ensure that output is clean and empty
		SQLRecord row; 	// One row of data
		unsigned int field_num=0;
		for (int current_row=1; current_row<=rows; current_row++)
		{
			field_num=0;
			row=*fieldptr;
			for (int value_pos=cols*current_row; value_pos<cols*(current_row+1); value_pos++)
			{
				if (table[value_pos]!=NULL) row.setValue(field_num, table[value_pos]);
				else row.getValueIPtr(field_num)->clear();
				
				field_num++;
			}

			output.addRecord(row);
		}
		//printf("Parse complete\n");
		sqlite3_free_table(table);
		return 0;
	}
	else return sql_ret;
}*/

int SQLiteDB::getLastError()
{
	return sqlError;
}

const string& SQLiteDB::getLastErrMsg() {
	return sqlErrMsg;
}


const vector<string> SQLiteDB::getFieldNames(const string& table_name) const {
	vector<string> fieldNames;
	if (table_name=="packages")
	{
		fieldNames.push_back("package_id"); // 0
		fieldNames.push_back("package_name"); // 1
		fieldNames.push_back("package_version"); // 2
		fieldNames.push_back("package_arch");// TEXT NOT NULL, //3
		fieldNames.push_back("package_build");// TEXT NULL, //4
		fieldNames.push_back("package_compressed_size");// TEXT NOT NULL, //5
		fieldNames.push_back("package_installed_size");// TEXT NOT NULL, //6
		fieldNames.push_back("package_short_description");// TEXT NULL, //7
		fieldNames.push_back("package_description");// TEXT NULL,  //8
		fieldNames.push_back("package_changelog");// TEXT NULL //9
		fieldNames.push_back("package_packager");// TEXT NULL, //10
		fieldNames.push_back("package_packager_email");// TEXT NULL, //11
		fieldNames.push_back("package_installed");// INTEGER NOT NULL, //12
		fieldNames.push_back("package_configexist");// INTEGER NOT NULL, //13
		fieldNames.push_back("package_action");// INTEGER NOT NULL, //14
		fieldNames.push_back("package_md5");// TEXT NOT NULL, //15
		fieldNames.push_back("package_filename");// TEXT NOT NULL //16
		fieldNames.push_back("package_betarelease");//TEXT NULL //17
		fieldNames.push_back("package_installed_by_dependency"); // INTEGER NOT NULL DEFAULT '0'    (0 - user-requested install, 1 - dependency-requested install, 2 - installed to build something//18
		fieldNames.push_back("package_type"); // INTEGER NOT NULL DEFAULT '0' //19
		fieldNames.push_back("package_add_date");//20
		fieldNames.push_back("package_build_date");//21
		fieldNames.push_back("package_repository_tags"); //22
		fieldNames.push_back("package_distro_version"); // 23
		fieldNames.push_back("package_provides");
		fieldNames.push_back("package_conflicts");
	}
	
	if(table_name=="files")
	{
		fieldNames.push_back("file_id");//  INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,
		fieldNames.push_back("file_name");//  TEXT NOT NULL,
		fieldNames.push_back("file_type");//  INTEGER NOT NULL,
		fieldNames.push_back("packages_package_id");//  INTEGER NOT NULL
	}

	
	if (table_name == "conflicts")
	{
		fieldNames.push_back("conflict_id");
		fieldNames.push_back("conflict_file_name");
		fieldNames.push_back("backup_file");
		fieldNames.push_back("conflicted_package_id");
	}
 	if(table_name=="locations")
	{
		fieldNames.push_back("location_id");//  INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,
		fieldNames.push_back("packages_package_id");//  INTEGER NOT NULL,
		fieldNames.push_back("server_url");//  INTEGER NOT NULL,
		fieldNames.push_back("location_path");//  TEXT NOT NULL
	}

 	if(table_name=="tags")
	{
		fieldNames.push_back("tags_id");//  INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,
		fieldNames.push_back("tags_name");//  TEXT NOT NULL
	}

 	if(table_name=="tags_links")
	{
		fieldNames.push_back("tags_link_id");//  INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,
		fieldNames.push_back("packages_package_id");//  INTEGER NOT NULL,
		fieldNames.push_back("tags_tag_id");//  INTEGER NOT NULL
	}

 	if(table_name=="dependencies")
	{
		fieldNames.push_back("dependency_id");//  INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,
		fieldNames.push_back("packages_package_id");//  INTEGER NOT NULL,
		fieldNames.push_back("dependency_condition");//  INTEGER NOT NULL DEFAULT '1',
		fieldNames.push_back("dependency_type");//  INTEGER NOT NULL DEFAULT '1',
		fieldNames.push_back("dependency_package_name");//  TEXT NOT NULL,
		fieldNames.push_back("dependency_package_version");//  TEXT NULL
		fieldNames.push_back("dependency_build_only");// INTEGER NOT NULL DEFAULT '0'
	}
	if (table_name=="history") {
		fieldNames.push_back("history_id");
		fieldNames.push_back("history_event");
		fieldNames.push_back("history_data");
	}
	if (table_name=="deltas") {
		fieldNames.push_back("delta_id");
		fieldNames.push_back("packages_package_id");
		fieldNames.push_back("delta_url");
		fieldNames.push_back("delta_md5");
		fieldNames.push_back("delta_orig_filename");
		fieldNames.push_back("delta_orig_md5");
		fieldNames.push_back("delta_size");
	}
	if (table_name=="config_files") {
		fieldNames.push_back("id");
		fieldNames.push_back("package_id");
		fieldNames.push_back("filename");
	}

	if (table_name=="config_options") {
		fieldNames.push_back("id");
		fieldNames.push_back("config_files_id");
		fieldNames.push_back("name");
		fieldNames.push_back("value");
	}

	if (table_name=="abuilds") {
		fieldNames.push_back("id");
		fieldNames.push_back("package_id");
		fieldNames.push_back("url");
	}

	if (table_name=="transactions") {
		fieldNames.push_back("id");
		fieldNames.push_back("t_start");
		fieldNames.push_back("t_end");
		fieldNames.push_back("t_status");
		fieldNames.push_back("custom_details");
	}


	if (table_name=="transaction_details") {
		fieldNames.push_back("id");
		fieldNames.push_back("transaction_id");
		fieldNames.push_back("package_name");
		fieldNames.push_back("package_version");
		fieldNames.push_back("package_build");
		fieldNames.push_back("package_md5");
		fieldNames.push_back("action_type");
		fieldNames.push_back("action_date");

	}


#ifdef ENABLE_INTERNATIONAL
	if (table_name=="descriptions") {
		fieldNames.push_back("description_id");
		fieldNames.push_back("packages_package_id");
		fieldNames.push_back("description_language");
		fieldNames.push_back("description_text");
		fieldNames.push_back("short_description_text");
	}
	
	if (table_name=="changelogs") {
		fieldNames.push_back("changelog_id");
		fieldNames.push_back("packages_package_id");
		fieldNames.push_back("changelog_language");
		fieldNames.push_back("changelog_text");
	}
	
	if (table_name=="ratings") {
		fieldNames.push_back("rating_id");
		fieldNames.push_back("rating_value");
		fieldNames.push_back("packages_package_name");
	}
#endif
	return fieldNames;
}

int SQLiteDB::sql_insert(const string& table_name, const SQLRecord& values) {
	string query_prepare = "INSERT INTO " + table_name + " VALUES(NULL,";
	vector<string> fieldNames=getFieldNames(table_name);
	// Note: algorithm requires that fieldNames.size()>=2
	for (size_t i=1; i<fieldNames.size(); ++i) {
		if (i<fieldNames.size()-1) query_prepare = query_prepare + "?,";
		else query_prepare = query_prepare + "?);";
	}
	sqlite3_stmt *stmt;
	int p_ret = sqlite3_prepare_v2(db, query_prepare.c_str(), -1, &stmt, NULL);
	if (p_ret!=SQLITE_OK) {
		printf("INSERT prepare fails, ret: %d\n", p_ret);
		printf("query: %s\n", query_prepare.c_str());
		return p_ret;
	}
	for (size_t i=1; i<fieldNames.size(); ++i) {
		sqlite3_bind_text(stmt, i, values.getValue(fieldNames[i]).c_str(), -1, SQLITE_STATIC); // NOTE: in sqlite3_bind_* indexes counts from 1, not from 0.
		//printf("BIND: %d, %s\n", i, values.getValue(fieldNames[i]).c_str());
	}
	int ret = sqlite3_step(stmt);
	if (ret==SQLITE_DONE) ret = SQLITE_OK;
	else {
		printf("FAILED TO INSERT: %d\n", ret);
		abort();
	}
	sqlite3_finalize(stmt);
	return ret;
}

// Table-based wrapper on sql_insert
int SQLiteDB::sql_insert(const string& table_name, const SQLTable& values) {
	if (values.empty()) return 0;
	int ret;
	for (size_t i=0; i<values.getRecordCount(); ++i) {
		ret = sql_insert(table_name, values[i]);
		if (ret!=SQLITE_OK) {
			mError("SQLite error: insert returned " + IntToStr(ret));
			return ret;
		}
	}
	return SQLITE_OK;
}

int SQLiteDB::sql_update(const string& table_name, const SQLRecord& fields, const SQLRecord& search) {
	if (fields.size() + search.size()>900) {
		mWarning("Too many fields in sql_update to " + table_name + ": " + IntToStr(fields.size() + search.size()));
	}
	string sql_query="UPDATE " + table_name + " SET ";
	for (size_t i=0; i<fields.size(); ++i) {
		if (i<fields.size()-1) sql_query = sql_query + fields.getFieldName(i)+"=?,";
		else sql_query = sql_query + fields.getFieldName(i)+"=?";
	}
	if (fields.empty()) {
		mDebug("Fields are empty, cannot update SQL data");
		return -1;
	}
	if (!search.empty()) {
		if (search.getSearchMode()==SEARCH_IN) {
			mError("SEARCH_IN is not supported in UPDATE!");
			return -1;
		}
		sql_query+=" WHERE ";
		for (size_t i=0; i<search.size(); i++) {
			sql_query+=search.getFieldName(i)+"=?";
		       	if (i!=search.size()-1 && search.getSearchMode()==SEARCH_AND) sql_query+=" AND ";
			if (i!=search.size()-1 && search.getSearchMode()==SEARCH_OR) sql_query+=" OR ";
		}
	}
	sql_query+=";";
	sqlite3_stmt *stmt;
	
	int p_ret = sqlite3_prepare_v2(db, sql_query.c_str(), -1, &stmt, NULL);
	if (p_ret!=SQLITE_OK) {
		printf("Prepare fails, ret: %d\n", p_ret);
		printf("query:\n%s\n", sql_query.c_str());
		return p_ret;
	}
	for (size_t i=0; i<fields.size(); ++i) {
		sqlite3_bind_text(stmt, i+1, fields.getValueI(i).c_str(), -1, SQLITE_STATIC);
	}

	for (size_t i=0; i<search.size(); ++i) {
		sqlite3_bind_text(stmt, fields.size()+i+1, search.getValueI(i).c_str(), -1, SQLITE_STATIC); // NOTE: index in sqlite3_bind_* are counted from 1, not 0! It made me cry when I tried to get this work, btw...
	}
	int ret = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (ret==SQLITE_DONE) return SQLITE_OK;
	return ret;
}

int SQLiteDB::sql_delete(const string& table_name, const SQLRecord& search) {
	string sql_query = "DELETE FROM "+table_name;
	if (!search.empty()) {
		sql_query = sql_query + " WHERE ";
		if (search.getSearchMode()==SEARCH_IN) {
			sql_query += search.getFieldName(0) + " IN (";
		}
		for (size_t i=0; i<search.size(); ++i) {
			switch(search.getSearchMode()) {
				case SEARCH_AND:
					sql_query = sql_query + search.getFieldName(i) + "=?";
		       			if (i<search.size()-1) sql_query = sql_query + " AND ";
					break;
				case SEARCH_OR:
					sql_query = sql_query + search.getFieldName(i) + "=?";
					if (i<search.size()-1) sql_query = sql_query + " OR ";
					break;
				case SEARCH_IN:
					if (i<search.size()-1) sql_query = sql_query + "?,";
					else sql_query = sql_query + "?)";
					break;
			}
		}
	}
	sql_query = sql_query + ";";
	sqlite3_stmt *stmt;
	
	int p_ret = sqlite3_prepare_v2(db, sql_query.c_str(), -1, &stmt, NULL);
	if (p_ret!=SQLITE_OK) {
		printf("Prepare fails, ret: %d\n", p_ret);
		printf("query:\n%s\n", sql_query.c_str());
		return p_ret;
	}
	for (size_t i=0; i<search.size(); ++i) {
		sqlite3_bind_text(stmt, i+1, search.getValueI(i).c_str(), -1, SQLITE_STATIC); // NOTE: index in sqlite3_bind_* are counted from 1, not 0! It made me cry when I tried to get this work, btw...
	}
	int ret = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (ret==SQLITE_DONE) return SQLITE_OK;
	return ret;
}

SQLiteDB::SQLiteDB(string filename, bool skip_integrity_check)
{
	initOk = false;
	db_filename=filename;
	sqlError=0;
	int sql_return;

	// backup db:
	backupDatabase();
opendb:
	sql_return=init();
	
	if (sql_return==1) // Means error
	{
		sqlError=sql_return;
		sqlErrMsg="Error opening database file "+db_filename+", aborting.";
		mError(sqlErrMsg);
	       	int close_result = sqlite3_close(db);
		if (close_result!=SQLITE_OK) printf("Failed to close DB handler %p, return code: %d\n", db, close_result); 
	       	if (mpkgErrorHandler.callError(MPKG_SUBSYS_SQLDB_OPEN_ERROR)==MPKG_RETURN_RETRY) goto opendb;
		abort();
       	}

check_integrity:
	if (!skip_integrity_check && !CheckDatabaseIntegrity())
	{
		// Try to reopen db as read-write and update structure
		if (_cmdOptions["sql_readonly"]=="yes") {
			_cmdOptions["sql_readonly"]="no";

			int close_result = sqlite3_close(db);
			if (close_result!=SQLITE_OK) printf("Failed to close DB handler %p, return code: %d\n", db, close_result); 
			sql_return = init();
			goto check_integrity;
		}
		if (mpkgErrorHandler.callError(MPKG_SUBSYS_SQLDB_INCORRECT)==MPKG_RETURN_REINIT) {
			say("reinitializing\n");
			initDatabaseStructure();
			goto check_integrity;
		}

		mError("Integrity check failed, aborting");
		int close_result = sqlite3_close(db);
		if (close_result!=SQLITE_OK) printf("Failed to close DB handler %p, return code: %d\n", db, close_result); 
		abort();
	}

	sql_exec("PRAGMA case_sensitive_like = true;");
	sqlite3_limit(db, SQLITE_LIMIT_VARIABLE_NUMBER, 1000000); // Limit possible variable number by one million.
	initOk = true;
}

int SQLiteDB::initDatabaseStructure()
{
	system((string) "mv -f " + db_filename + " " + db_filename + "_backup");
	unlink(db_filename.c_str());
	int ret;
	ret = sqlite3_open_v2(db_filename.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	sqlite3_extended_result_codes(db, 1);

	if (ret!=SQLITE_OK)
	{
		mError("Error opening database, cannot continue");
		return 1;
	}

	//WriteFile("test.sql", getDBStructure());
	sql_exec(getDBStructure());
	sqlBegin();
	return ret;
}


SQLiteDB::~SQLiteDB(){
	sqlCommit();	
	int close_result = sqlite3_close(db);
	if (close_result!=SQLITE_OK) printf("Failed to close DB handler %p, return code: %d\n", db, close_result); 
	if (simulate)
	{
		restoreDatabaseFromBackup();
	}
}

int SQLProxy::sqlCommit()
{
	if (sqliteDB==NULL) sqliteDB = new SQLiteDB;
	return sqliteDB->sqlCommit();
}

int SQLProxy::clear_table(const string& table_name)
{
	if (sqliteDB==NULL) sqliteDB = new SQLiteDB;

	internalDataChanged=true;
	return sqliteDB->clear_table(table_name);
}

int SQLProxy::sqlBegin()
{
	if (sqliteDB==NULL) sqliteDB = new SQLiteDB;
	int ret = sqliteDB->sqlBegin();
	return ret;
}

int SQLProxy::sqlFlush()
{
	if (sqliteDB==NULL) sqliteDB = new SQLiteDB;
	return sqliteDB->sqlFlush();
}
int SQLProxy::getLastError()
{
	if (sqliteDB==NULL) sqliteDB = new SQLiteDB;
	return sqliteDB->getLastError();
}

const string& SQLProxy::getLastErrMsg() {
	if (sqliteDB==NULL) sqliteDB = new SQLiteDB;
	return sqliteDB->getLastErrMsg();
}

int SQLProxy::get_sql_vtable(SQLTable& output, const SQLRecord& fields, const string& table_name, const SQLRecord& search)
{
	if (sqliteDB==NULL) sqliteDB = new SQLiteDB;
	
	// Check if search size exceeds limits and recombine request if possible
	if (search.size()<=900) return sqliteDB->get_sql_vtable(output, fields, table_name, search);
	else {
		printf("Search size: %d, RECOMBINE...\n", search.size());
		if (search.getSearchMode()!=SEARCH_IN) {
			mError("Query limit exceeded, but cannot be recomposed: use SEARCH_IN for long queries");
			return SQLITE_FAIL;
		}
		SQLRecord searchProxy;
		SQLTable outputProxy;
		searchProxy.setSearchMode(SEARCH_IN);
		size_t limit = 0;
		int ret = SQLITE_OK;
		for (size_t i=0; i<search.size(); ++i) {
			searchProxy.addField(search.getField(i));
			limit++;
			if (limit==900 || i==search.size()-1) {
				ret = sqliteDB->get_sql_vtable(outputProxy, fields, table_name, searchProxy);
				if (ret != SQLITE_OK) {
					printf("Step get_sql_vtable fails, ret: %d, searchProxy.size: %d, outputProxy.size: %d, output: %d\n", ret, searchProxy.size(), outputProxy.size(), output.size());
					return ret;
				}
				for (size_t t=0; t<outputProxy.size(); ++t) {
					output.addRecord(outputProxy[t]);
				}
				outputProxy.clear();
				searchProxy.clear();
				limit = 0;
			}
		}
		printf("Step get_sql_vtable OK, ret: %d, searchProxy.size: %d, outputProxy.size: %d, output: %d\n", ret, searchProxy.size(), outputProxy.size(), output.size());

		return ret;
	}

}

int SQLProxy::sql_insert(const string& table_name, const SQLRecord& values)
{
	if (sqliteDB==NULL) sqliteDB = new SQLiteDB;
	internalDataChanged=true;
	return sqliteDB->sql_insert(table_name, values);
}

int SQLProxy::sql_insert(const string& table_name, const SQLTable& values)
{
	if (sqliteDB==NULL) sqliteDB = new SQLiteDB;
	internalDataChanged=true;
	return sqliteDB->sql_insert(table_name, values);
}

int SQLProxy::sql_update(const string& table_name, const SQLRecord& fields, const SQLRecord& search)
{
	if (sqliteDB==NULL) sqliteDB = new SQLiteDB;
	internalDataChanged=true;
	if (search.size()<=900) return sqliteDB->sql_update(table_name, fields, search);
	else {
		if (search.getSearchMode()!=SEARCH_IN) {
			mError(string(__func__) + ": query limit exceeded, but cannot be recomposed: use SEARCH_IN for long queries");
			return SQLITE_FAIL;
		}
		SQLRecord searchProxy;
		searchProxy.setSearchMode(SEARCH_IN);
		size_t limit = 0;
		int ret = SQLITE_OK;
		for (size_t i=0; i<search.size(); ++i) {
			searchProxy.addField(search.getField(i));
			limit++;
			if (limit==900 || i==search.size()-1) {
				ret = sqliteDB->sql_update(table_name, fields, searchProxy);
				if (ret != SQLITE_OK) return ret;
				searchProxy.clear();
				limit = 0;
			}
		}
		return ret;
	}
}

int SQLProxy::sql_delete(const string& table_name, const SQLRecord& search)
{
	if (sqliteDB==NULL) sqliteDB = new SQLiteDB;
	internalDataChanged=true;
	if (search.size()<=900) return sqliteDB->sql_delete(table_name, search);
	else {
		if (search.getSearchMode()!=SEARCH_IN) {
			mError(string(__func__) + ": query limit exceeded, but cannot be recomposed: use SEARCH_IN for long queries");
			return SQLITE_FAIL;
		}
		SQLRecord searchProxy;
		searchProxy.setSearchMode(SEARCH_IN);
		size_t limit = 0;
		int ret = SQLITE_OK;
		for (size_t i=0; i<search.size(); ++i) {
			searchProxy.addField(search.getField(i));
			limit++;
			if (limit==900 || i==search.size()-1) {
				ret = sqliteDB->sql_delete(table_name, searchProxy);
				if (ret != SQLITE_OK) return ret;
				searchProxy.clear();
				limit = 0;
			}
		}
		return ret;
	}
}
int SQLProxy::sql_exec(const string& query) {
	if (sqliteDB==NULL) sqliteDB = new SQLiteDB;
	internalDataChanged=true;
	return sqliteDB->sql_exec(query);

}

int64_t SQLProxy::last_insert_id() {
	if (sqliteDB==NULL) sqliteDB = new SQLiteDB;
	internalDataChanged=true;
	return sqliteDB->last_insert_id();

}
SQLProxy::SQLProxy()
{
	sqliteDB=NULL;
	internalDataChanged=true;
}
SQLProxy::~SQLProxy()
{
	if (sqliteDB!=NULL) delete sqliteDB;
}

// Closes DB connection. Can be useful in install.
void SQLProxy::closeDBConnection() {
	if (sqliteDB!=NULL) {
		delete sqliteDB;
		sqliteDB=NULL;
	}
}

// DB Locking

bool lockDatabase()
{
	
	if (_cmdOptions["sql_readonly"]=="yes") return true; // No need to lock if we using read-only access
	// Lock file will be /var/run/mpkg.lock
	string var_directory = SYS_ROOT + "/var";
	if (!mConfig.getValue("var_directory").empty()) var_directory=mConfig.getValue("var_directory");
	if (FileExists(var_directory + "/run/mpkg.lock", NULL)) {
		if (!isProcessRunning(ReadFile(var_directory + "/run/mpkg.lock"))) {
			string lfile = var_directory + "/run/mpkg.lock";
			unlink(lfile.c_str());
		}
		else {
			mError("Database is already locked by process " + ReadFile(var_directory + "/run/mpkg.lock"));
			return false;
		}
	}
	system("mkdir -p " + var_directory + "/run");
	WriteFile(var_directory + "/run/mpkg.lock", IntToStr(getpid()));
	return true;
}

bool unlockDatabase()
{
	string var_directory = SYS_ROOT + "/var";
	if (!mConfig.getValue("var_directory").empty()) var_directory=mConfig.getValue("var_directory");

	if (FileExists(var_directory + "/run/mpkg.lock", NULL))
	{
		if (ReadFile(var_directory + "/run/mpkg.lock")==IntToStr(getpid()))
		{
			// Mean that the database is locked by this process
			string lfile = var_directory + "/run/mpkg.lock";
			unlink(lfile.c_str());
			return true;
		}
		else {
			//mError("Database is locked by another process, cannot unlock");
			return false;
		}
	}
	else {
		return true;
	}
}

bool isDatabaseLocked()
{
	string var_directory = SYS_ROOT + "/var";
	if (!mConfig.getValue("var_directory").empty()) var_directory=mConfig.getValue("var_directory");

	if (FileExists(var_directory + "/run/mpkg.lock", NULL))
	{
		// Check if the process created this id is alive
		if (!isProcessRunning(ReadFile(var_directory + "/run/mpkg.lock"))) {
			string lfile = var_directory + "/run/mpkg.lock";
			unlink(lfile.c_str());
			return false;
		}
		mError("Database is locked by process " + ReadFile(var_directory + "/run/mpkg.lock"));
		return true;
	}
	else return false;
}
bool backupDatabase()
{
	return true; // Backup is disabled for now

	string cmd = "cp -f " + DB_FILENAME + " " + DB_FILENAME+".backup 2>&1 >/dev/null";
	mDebug(cmd);
	if (system(cmd)==0) return true;
	else return false;
}

bool restoreDatabaseFromBackup()
{
	return true; // Backup is disabled for now
	mDebug("restoring");
	if (system("cp -f " + DB_FILENAME + ".backup " + DB_FILENAME + " 2>&1 >/dev/null")==0) return true;
	else return false;
}

