/**********************************************************
 * Standard C String helpful functions - header file
 * $Id: string_operations.h,v 1.16 2007/12/10 03:12:59 i27249 Exp $
 * ********************************************************/
#ifndef _STRING_OPERATIONS_H_
#define _STRING_OPERATIONS_H_
#include <string>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <libintl.h>
#include <locale.h>

#define _(string) gettext(string)
using namespace std;
const string IntToStr(long long num);
const string getAbsolutePath(const string& directory);
void PrepareSql(string& str);
char * strMerge(const char *part1, const char *part2); // Merges part1 and part2, and returns the result. Note: you should free() the result at the end.
const string cutSpaces(const string& str);
const string humanizeSize(const string& size);
const string humanizeSize(double size);
unsigned int fl2ul(float input);
const string adjustStringWide(string input, unsigned int char_width, string prefix="");
vector<string> adjustStringWide2(const string&, size_t);
vector<string> adjustStringWideEx(const string& input, unsigned int char_width);
class TagPair
{
	public:
	TagPair(string tagV, string valueV, bool checkState=false);
	TagPair();
	~TagPair();
	string tag;
	string value;
	bool checkState;
	void clear();
};

string getTagDescription(string tag);
string getWeekDay(int);
string getMonthName(int);
string getTimeString(time_t);
string getExtension(string filename);
string getFilename(string fullpath);
string getDirectory(string fullpath);
size_t utf8strlen(string str);
string utf8substr(string data, size_t start, int end=-1);
string truncateString(string data, size_t max_length, bool edge=true);
string getHostFromUrl(string url);
bool strToBool(string data);
string boolToStr(bool value);
string toHtml(string data);

// TODO: get rid of this. We should use std::map instead.
class StringMap {
	public:
		StringMap();
		~StringMap();
		size_t size() const;
		string _empty;
		bool deleteKey(const string& keyName);
		bool empty() const;
		void clear();
		bool deleteKey(size_t i);
		const string& getValue(const string& keyName) const;
		const string& getValue(size_t i) const;
		const string& getKeyName(size_t i) const;
		int keyNum(const string& keyName) const;
		void setValue(const string& keyName, const string& keyValue);
		bool setValue(const string& keyName, string *keyValue); // More complex function, returns false if key not found
		

	private:
		vector<string> value;
		vector<string> key;
};
class mstring
{
	public:
		void operator += (const string& str2);
		void operator += (const char *str2);
		void operator += (char str2);
		void operator += (const mstring& str2);
		bool operator == (const string& str2);
		bool operator == (const char *str2);
		bool operator != (const string& str2);
		bool operator != (const char *str2);
		void operator + (const string& str2);
		void operator + (const char *str2);
		void operator = (const mstring& str2);
		void operator = (const string& str2);
		void operator = (const char *str2);


		char operator [] (int i);
		void clear();
		bool empty();
		const char * c_str();
		size_t length();
		const string& s_str();
		mstring();
		~mstring();

		string str;
};

const vector<string> MakeStrings(string data);
string filter_slashes(string data);
void strReplace(string *data, string pattern, string value);
bool parseSlackwareFilename(string tmp, string* name, string* version, string* arch, string* build);
string getParseValue(string key, string data, bool toEndStr=false);
vector<string> splitString(const string& data, const string& delim);
int strverscmp2(const string& v1, const string& v2);
int compareVersions(const string& version1, const string& build1, const string& version2, const string& build2); // 0 == equal, 1 == v1>v2, -1 == v1<v2

vector<string> mergeVectors(const vector<string>& vector1, const vector<string>& vector2);
#endif

