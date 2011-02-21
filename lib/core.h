/********************************************************************************
 *					core.h
 * 			Central core for MPKG package system
 *					Headers
 *	$Id: core.h,v 1.14 2007/08/30 21:46:48 i27249 Exp $
 ********************************************************************************/
#ifndef CORE_H_
#define CORE_H_

#include "dataunits.h"
//#include "constants.h"
#include "string.h"
//string T="', '"; // Wery helpful element for SQL queries

//#define T "', '"   // Wery helpful element for SQL queries

#define SEARCH_OR 	0x01
#define SEARCH_AND 	0x02
#define SEARCH_IN	0x03
#define EQ_LIKE 0x01
#define EQ_EQUAL 0x02
#define EQ_CUSTOMLIKE 0x03

bool checkAcceptedArch(const PACKAGE *pkg);
typedef struct
{
	string fieldname;
	string value;
} SQLField;

class SQLRecord
{
	private:
		vector<SQLField> field;
		int search_type;
		int eq_type;

	public:
		unsigned int size() const;
		bool empty() const;
		void clear();
		vector<string> getRecordValues();
		const string& getFieldName(unsigned int num) const;
		const string& getValue(const string& fieldname) const;
		//string getValueX(const string&);
		vector<string> getValueVector(const string&);
		const string& getValueI(unsigned int num) const; // returns the indexed field value
	       	string* getValueIPtr(unsigned int num);
		int getFieldIndex(const string& fieldname) const;
		void setSearchMode(int mode);
		const int& getSearchMode() const;
		void setEqMode(int mode);
		const int& getEqMode() const;
		void addField(const string& fieldname, const char* value);
		void addField(const string& fieldname, const string& value);
		void addField(const string& fieldname, const int& value);
		void addField(const string& fieldname);
		bool setValue(const string& fieldname, const string& value);
		void setValue(unsigned int& field_index, const string& value);
		string orderBy;
		string groupBy;

		SQLRecord();
		~SQLRecord();
};

class SQLTable
{
	private:
		vector<SQLRecord> table;
	public:
		unsigned int getRecordCount() const; 	// returns record count
		unsigned int size() const;
		bool empty() const;		// returns TRUE if table is empty (record count = 0), otherwise returns false
		void clear();		// clears table
		const string& getValue (unsigned int num, const string& fieldname) const;	// returns value of field called fieldname in num record
		const string& getValueF (unsigned int num, const char* fieldname) const;
		const string& getValue(unsigned int num, const int& field_index) const; // returns the value of indexed field
		string * getValuePtr(unsigned int num, const int& field_index);
		int getFieldIndex(const string& fieldname) const; // returns the field index
		SQLRecord* getRecord(unsigned int num);

		void addRecord(const SQLRecord& record);
		SQLTable();
		~SQLTable();
};



#include "sql_pool.h"
#endif //CORE_H_

