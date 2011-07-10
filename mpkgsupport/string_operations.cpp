/***********************************************************
 * Standard C String helpful functions
 * $Id: string_operations.cpp,v 1.19 2007/12/10 03:12:59 i27249 Exp $
 * ********************************************************/

#include "string_operations.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include "debug.h"
#include "file_routines.h"
string getTagDescription(string tag)
{
	if (tag=="print") return "Система печати";
	if (tag=="utils") return "Различные утилиты";
	if (tag=="audio") return "Аудио";
	if (tag=="x11-fonts") return "Шрифты X11";
	if (tag=="virtual") return "Виртуальные пакеты";
	if (tag=="base-utils") return "Базовые утилиты";
	if (tag=="kde") return "Рабочий стол KDE";
	if (tag=="koffice") return "Офисный пакет KOffice";
	if (tag=="x11") return "Система X-Window";
	if (tag=="apple") return "Поддержка Apple Macintosh";
	if (tag=="openoffice") return "Офисный пакет OpenOffice.org";
	if (tag=="emacs") return "Редактор Emacs и его компоненты";
	if (tag=="tcl") return "Поддержка Tk/TCL";
	if (tag=="slackware") return "Сконвертированные пакеты";
	if (tag=="network") return "Программы для работы в сети";
	if (tag=="libs") return "Различные библиотеки";
	if (tag=="console") return "Консольные приложения";
	if (tag=="apps") return "Приложения";
	if (tag=="xapps") return "Графические приложения";
	if (tag=="devel") return "Инструменты для разработки";
	if (tag=="docs") return "Дополнительная документация";
	if (tag=="codec") return "Кодеки для проигрывания мультимедиа-файлов";
	if (tag=="kernel-source") return "Исходные тексты ядра";
	if (tag=="wine") return "Эмулятор Windows API";
	if (tag=="tex") return "Система верстки TeX";
	if (tag=="server") return "Серверные приложения";
	if (tag=="beryl") return "Трехмерный рабочий стол";
	if (tag=="themes") return "Различные темы";
	if (tag=="desktop") return "Набор для рабочей станции";
	if (tag=="science") return "Научные приложения";
	if (tag=="experimental") return "Экспериментальные пакеты";
	if (tag=="beta") return "Бета-версии";
	if (tag=="games") return "Игры";
	if (tag=="kde-i18n") return "Локализация KDE";
	if (tag=="proprietary") return "Проприетарные приложения";
	if (tag=="base") return "Базовая система";
	if (tag=="compiz") return "Трехмерный рабочий стол Compiz Fusion";
	if (tag=="doc") return "Документация";
	if (tag=="graphic") return "Инструменты для работы с графикой";
	if (tag=="multimedia") return "Мультимедийные приложения";
	if (tag=="perl") return "Модули perl";
	if (tag=="school") return "Образовательный набор";
	if (tag=="x11-base") return "Базовая часть X-Window";
	if (tag=="drivers") return "Драйверы оборудования";
	if (tag=="x11-skel") return "Структура X11";
	if (tag=="kde4") return "Рабочий стол KDE4";


	return tag;
}

TagPair::TagPair()
{
	checkState=false;
}
TagPair::~TagPair()
{
}

TagPair::TagPair(string tagV, string valueV, bool checkStateV)
{
	tag=tagV;
	value=valueV;
       	checkState=checkStateV;
}
void TagPair::clear()
{
	tag.clear();
	value.clear();
	checkState=false;
}


StringMap::StringMap() {}
StringMap::~StringMap() {}

const string& StringMap::getValue(const string& keyName) const {
	for (size_t i=0; i<key.size(); i++) {
		if (strcmp(key[i].c_str(), keyName.c_str())==0) return value[i];
	}
	return _empty;
}
const string& StringMap::getValue(size_t i) const {
	return value[i];
}
size_t StringMap::size() const {
	return key.size();
}

bool StringMap::empty() const {
	return key.empty();
}
bool StringMap::deleteKey(size_t i) {
	if (i>=key.size()) return false;
	vector<string>::iterator a = key.begin();
	a += i;
	key.erase(a);
	a = value.begin();
	a += i;
	value.erase(a);
	return true;
}
void StringMap::clear() {
	key.clear();
	value.clear();
}
bool StringMap::deleteKey(const string& keyName) {
	vector<string>::iterator a;
	for (size_t i=0; i<key.size(); i++) {
		if (strcmp(key[i].c_str(), keyName.c_str())==0) {
			a = key.begin();
			a += i;
			key.erase(a);
			a = value.begin();
			a += i;
			value.erase(a);
			return true;
		}
	}
	return false;
}
const string& StringMap::getKeyName(size_t i) const {
	return key[i];
}

int StringMap::keyNum(const string& keyName) const {
	for (size_t i=0; i<key.size(); i++) {
		if (strcmp(key[i].c_str(), keyName.c_str())==0) return i;
	}
	return -1;
}
bool StringMap::setValue(const string& keyName, string *keyValue) {
	for (size_t i=0; i<key.size(); i++) {
		if (strcmp(key[i].c_str(), keyName.c_str())==0) {
			*keyValue = value[i];
			return true;
		}
	}
	keyValue->clear();
	return false;
}

void StringMap::setValue(const string& keyName, const string& keyValue) {
	for (size_t i=0; i<key.size(); i++) {
		if (strcmp(key[i].c_str(), keyName.c_str())==0) {
			value[i]=keyValue;
			return;
		}
	}
	key.push_back(keyName);
	value.push_back(keyValue);
	return;
}

void PrepareSql(string& str)
{
	if (str.empty())
	{
		str="0";
		return;
	}
	if (str.find("\'")==std::string::npos) return;
	size_t last_pos=0;
	size_t offset = 0;
	while (offset<str.length())
	{
		last_pos=str.substr(offset).find_first_of("\'");
		if (last_pos!=std::string::npos) str.insert(last_pos+offset, "\'");
		else return;
		offset += last_pos + 2;
	}
}

const string getAbsolutePath(const string& directory)
{
	string cwd = get_current_dir_name();
	cwd+="/";
	// Костыль-mode
	if (directory.find("/")==0) return directory; // Already absolute
	string ret = cwd;
       	if (directory!="." && directory!="./") ret += directory;
	return ret;
	
}

string getExtension(string filename)
{
	filename = getFilename(filename);
	if (filename.find(".")==std::string::npos || filename.find_last_of(".")==filename.length()-1) return ""; // No extension
	return filename.substr(filename.find_last_of(".")+1);
}

string getFilename(string fullpath)
{
	if (fullpath.find("/")==std::string::npos) return fullpath;
	if (fullpath.find_last_of("/")==fullpath.length()-1) return "";
	return fullpath.substr(fullpath.find_last_of("/")+1);
}

string getHostFromUrl(string url)
{
	if (url.find("://")!=std::string::npos) {
		url = url.substr(url.find("://")+strlen("://"));
	}
	if (url.find_first_of("/")!=std::string::npos) url = url.substr(0,url.find_first_of("/"));
	return url;


}

string getDirectory(string fullpath)
{
	if (fullpath.find("/")==std::string::npos) return "";
	return fullpath.substr(0, fullpath.find_last_of("/"));

}

// Helpful function ))
const string IntToStr(long long num)
{
  	char *s = (char *) malloc(100);
  	string ss;
  	if (s)
  	{
		sprintf(s,"%Ld",num);
	  	ss=s;
	  	free(s);
  	}
  	else 
  	{
		perror("Error while allocating memory");
	  	abort();
  	}
  	return ss;
}
bool parseSlackwareFilename(string tmp, string* name, string* version, string* arch, string* build) {
	size_t k;
	if (getExtension(tmp)!="tgz" && getExtension(tmp)!="tlz" && getExtension(tmp)!="txz" && getExtension(tmp)!="tbz") {
		printf(_("Malformed package name or unknown package extension: %s\n"), tmp.c_str());
		return false;
	}
	// Cutting extension
	tmp = tmp.substr(0, tmp.size()-4);

	// Build
	k=tmp.find_last_of("-");
	if (k==std::string::npos || k==tmp.size()-1) {
		printf(_("Malformed package name: %s\n"), tmp.c_str());
		return false;
	}
	*build = tmp.substr(k+1);
	tmp = tmp.substr(0,k);

	// Arch
	k = tmp.find_last_of("-");
	if (k==std::string::npos || k==tmp.size()-1) {
		printf(_("Malformed package name: %s\n"), tmp.c_str());
		return false;
	}
	*arch = tmp.substr(k+1);
	tmp = tmp.substr(0,k);
			
	// Version
	k = tmp.find_last_of("-");
	if (k==std::string::npos || k==tmp.size()-1) {
		printf(_("Malformed package name: %s\n"), tmp.c_str());
		return false;
	}
	*version = tmp.substr(k+1);
	tmp = tmp.substr(0,k);

	// Name: all that last.
	*name = tmp;
	return true;
}

char * strMerge(const char *part1, const char *part2)
{

	int p1=strlen(part1);
	int p2=strlen(part2);
	// Beginning dump
	char *ret=(char *) malloc(p1+p2+1);
	for (int i=0; i<p1; i++)
	{
		ret[i]=part1[i];
	}
	for (int i=p1; i<=p1+p2; i++)
	{
		ret[i]=part2[i-p1];
	}
	return ret;
}

const string cutSpaces(const string& str) {
	//printf("%s\n", str.c_str());
	size_t start = str.find_first_not_of(" \n\t");
	size_t end = str.find_last_not_of(" \n\t");
	string ret;
	if (start == std::string::npos || end == std::string::npos) return str;
	return str.substr(start, end-start+1);
}

void mstring::operator += (const string& str2)
{
	str = str + str2;
}

void mstring::operator += (const char *str2)
{
	str = str + (string) str2;
}

void mstring::operator += (char str2)
{
	str = str + str2;
}
void mstring::operator += (const mstring& str2)
{
	str = str + str2.str;
}

bool mstring::operator == (const string& str2)
{
	if (strcmp(str.c_str(), str2.c_str())==0) return true;
	return false;
}

bool mstring::operator == (const char *str2)
{
	if (strcmp(str.c_str(), str2)==0) return true;
	return false;
}

void mstring::operator = (const string& str2)
{
	str=str2;
}

void mstring::operator = (const char *str2)
{
	str = (string) str2;
}

bool mstring::operator != (const string& str2)
{
	if (strcmp(str.c_str(), str2.c_str())!=0) return true;
	return false;
}
void mstring::operator = (const mstring& str2)
{
	str = str2.str;
}
bool mstring::operator != (const char *str2)
{
	if (strcmp(str.c_str(), str2)!=0) return true;
	return false;
}

char mstring::operator [] (int i)
{
	return str[i];
}

void mstring::clear()
{
	str.clear();
}

bool mstring::empty()
{
	return str.empty();
}

const char * mstring::c_str()
{
	return str.c_str();
}

const string& mstring::s_str()
{
	return str;
}

size_t mstring::length()
{
	return str.length();
}

mstring::mstring(){}
mstring::~mstring(){}
unsigned int fl2ul(float input)
{
	unsigned int preout = (unsigned int) input;
	if (!(input - preout < .5))
	{
		preout++;
	}
	return preout;
}

const string humanizeSize(const string& size)
{
	return humanizeSize(strtod(size.c_str(), NULL));
}
const string humanizeSize(double size)
{
	long double new_size;
	string ret;
	string tmp;
	if (size < 1024) return IntToStr((long long) size) + _(" bytes");

	if (size >= 1024 && size < 1048576)
	{
		new_size = size/1024;
		ret = IntToStr((int) new_size) + _(" Kb");
		return ret;
	}
	if (size >= (1024*1024) && size < (1024*1024*1024))
	{
		new_size = size/(1024*1024);
		ret = IntToStr((int) new_size) + _(" Mb");
		return ret;
	}
	if (size >= (1024*1024*1024))
	{
		new_size = size/(1024*1024*1024);
		// Gigabytes matters, so show it with details
		tmp=IntToStr((int) (new_size*1000));
		if (tmp.length()>=4) tmp=tmp.substr(1,2);
		ret = IntToStr((int) new_size) + "." + tmp + _(" Gb");

		return ret;
	}
	return "0";
}

const string adjustStringWide(string input, unsigned int char_width, string prefix)
{
	//if (char_width < 50) char_width = 50;
	vector<string> spaces;
	vector<string> chunks;
	int lspace=0;
	for (int i=0; i < (int) input.size(); i++)
	{
		if (input[i]==' ') lspace = i;
		if (utf8strlen(input.substr(0,i))>=char_width)
		{
			if (lspace==0) lspace = i;
			chunks.push_back(input.substr(0,lspace));
			input=input.substr(lspace);
			i=-1;
			lspace = 0;
		}
		if (i==(int) input.size()-1) chunks.push_back(input);

	}
	
	string ret;
	for (size_t i=0; i<chunks.size(); i++)
	{
		if (prefix.empty())
		{
			ret+=chunks[i];
		       	ret += "<br>";
		}
		else
		{	
			ret+=prefix+": " + chunks[i];
			ret+="\n";
		}

	}
	return ret;

}
vector<string> adjustStringWideEx(string input, unsigned int char_width)
{
	vector<string> spaces;
	vector<string> chunks;

	int lspace=0;
	for (int i=0; i < (int) input.size(); i++)
	{
		if (input[i]==' ') lspace = i;
		if (utf8strlen(input.substr(0,i))>=char_width)
		{
			if (lspace==0) lspace = i;
			chunks.push_back(input.substr(0,lspace));
			input=input.substr(lspace);
			i=-1;
			lspace = 0;
		}
		if (i==(int) input.size()-1) chunks.push_back(input);

	}
	for (size_t i=0; i<chunks.size(); i++) {
		if (chunks[i].find_first_of(" ")==0) chunks[i]=chunks[i].substr(1);
	}
	return chunks;
}




//--------------
size_t utf8strlen(string str) {
	size_t ret = str.length();
	unsigned char key;
	for (size_t i=0; i<str.length(); i++) {
		key = str[i];
		if (key>=194 && key<=223) { // 2-byte sequence
			ret--;
		}
		if (key>=224 && key<=239) { // 3-byte sequence
			ret -= 2;
		}
		if (key>=240 && key<=244) { // 4-byte sequence
			ret -= 3;
		}
	}
	return ret;
}

const vector<string> MakeStrings(string data) {
	vector<string> ret;
	if (data.empty()) return ret;
	if (data[data.size()-1]!='\n') data+="\n";
	vector<int> n_pos;
	int next_pos=0;
	n_pos.push_back(-1);
	for (size_t i=0; i<data.size(); i++) {
		if (data[i]=='\n') n_pos.push_back(i);
	}
	for (size_t i=0; i<n_pos.size()-1; i++) {
		if (i!=n_pos.size()-2) next_pos = n_pos[i+1]-n_pos[i]-1;
		else next_pos = data.size()-n_pos[i]-2;
		//printf("n_pos[i]=%d\n", n_pos[i]);
		ret.push_back(data.substr(n_pos[i]+1, next_pos));
	}
	return ret;
}
string utf8substr(string data, size_t start, int end) {
	//mLog("data = " + data);
	//mLog("start = " + IntToStr(start) + ", end = " + IntToStr(end));
	string ret;
	if (start>utf8strlen(data)) return ret;
	for (size_t i=0; utf8strlen(ret)<start; i++) {
		ret += data[i];
	}
	start = ret.size();
	ret.clear();
	if (end<0) end = data.size()-start;
	for (size_t i=start; utf8strlen(ret)<(size_t) end && i < data.size(); i++) {
		ret += data[i];
	}
	//mLog("ret = " + ret);
	return ret;
}
vector<string> disposeStr(string input, size_t w) {
	//mLog("Disposing str " + input + ", w = " + IntToStr(w));
	// Новый подход =)
	// Сначала создадим матрицу слов. Разделителями будем считать пробелы, символы \n и \t
	// При наличии нескольких пробелов они будут вырезаться.
	vector<string> wordMap;
	size_t a, b;
	while (input.find_first_of(" \t\n")!=std::string::npos) {
		a = input.find_first_of(" \t\n");
		b = input.find_first_not_of(" \t\n");
		if (b == std::string::npos) {
			input.clear();
			break;
		}
		if (a<b) input = input.substr(b);
		else {
			wordMap.push_back(input.substr(b, a-b));
			input = input.substr(a);
		}
	}
	if (!input.empty()) wordMap.push_back(input);

	// Получили карту слов. Располагаем ее по строкам соответствующего размера
	vector<string> ret;
	string str, test_str;
	size_t k = 0;

	//mLog("Composing strings from " + IntToStr(wordMap.size()) + " words");
	while (k<wordMap.size()) {
	//	mLog("k = " + IntToStr(k));
		if (str.empty() && utf8strlen(wordMap[k])>=w) { // Если у нас слово, которое ну никак не лезет - то режем его в наглую...
			test_str = wordMap[k];
			while (utf8strlen(test_str)>=w) {
	//			mLog("test_str = " + test_str);
				ret.push_back(utf8substr(test_str, 0, w-1));
				test_str = utf8substr(test_str, 0, w-1);
			}
			ret.push_back(test_str);
			test_str.clear();
			str.clear();
			k++;
			continue;
		}
		test_str = str + wordMap[k];
		if (utf8strlen(test_str) < w) {
			str = test_str + " ";
			k++;
		}
		else {
			ret.push_back(str);
			str.clear();
		}
	}
	if (!str.empty()) ret.push_back(str);
	//mLog("done");
	//
	for (size_t i=0; i<ret.size(); i++) {
		//mLog("[" + ret[i]+ "]");
	}
	return ret;
}

string getWeekDay(int day) {
	switch (day) {
		case 1:
			return _("Monday");
		case 2:
			return _("Tuesday");
		case 3:
			return _("Wednesday");
		case 4:
			return _("Thursday");
		case 5:
			return _("Friday");
		case 6:
			return _("Saturday");
		case 0:
			return _("Sunday");
		default:
			return "UNKNOWN_DAY "+ IntToStr(day);
	}
}
string getMonthName(int month) {
	switch (month) {
		case 0: return _("january");
		case 1: return _("february");
		case 2: return _("march");
		case 3:	return _("april");
		case 4:	return _("may");
		case 5:	return _("june");
		case 6:	return _("july");
		case 7:	return _("august");
		case 8:	return _("september");
		case 9:	return _("october");
		case 10: return _("november");
		case 11: return _("december");
		default: return "UNKNOWN_MONTH "+IntToStr(month);
	}
}

string getTimeString(time_t unixtime) {
	string time_str, hr, mn, sc, date_str;
	struct tm *time_struct = NULL;
	time_struct = gmtime(&unixtime);
	hr = IntToStr(time_struct->tm_hour); 
	if (hr.length() < 2) hr = "0" + hr;
	mn = IntToStr(time_struct->tm_min);
	if (mn.length() < 2) mn = "0" + mn;
	sc = IntToStr(time_struct->tm_sec);
	if (sc.length() < 2) sc = "0" + sc;

	time_str = hr + ":" + mn;
	date_str = IntToStr(time_struct->tm_mday) + " " + getMonthName(time_struct->tm_mon) + " "+IntToStr(1900 + time_struct->tm_year);
	time_str = date_str + " " + time_str;
	return time_str;


}
vector<string> adjustStringWide2(const string& input, size_t w) {
	vector<string> ret, tmp;
	vector< vector<string> > matrix;
	tmp = MakeStrings(input);
	for (size_t i=0; i<tmp.size(); i++) {
		matrix.push_back(disposeStr(tmp[i], w));
	}
	for (size_t i=0; i<matrix.size(); i++) {
		for (size_t y=0; y<matrix[i].size(); y++) {
			ret.push_back(matrix[i][y]);
		}
	}
	return ret;
	
}


//---------------

string truncateString(string data, size_t max_length, bool edge) {
	if (utf8strlen(data)<=max_length) return data;
	string ret;
	if (edge) return utf8substr(data, 0, max_length-3) + "...";
	else return utf8substr(data, 0, max_length/2-5) + "...."+utf8substr(data, max_length/2+5);
}

/*unsigned int getOptimalWidth(string str) {
	vector<string> a = MakeStrings(str);
	unsigned int ret=0;
	for (unsigned int i=0; i<a.size(); i++) {
		if (utf8strlen(a[i])>ret) ret = utf8strlen(a[i]);
	}
	return ret;

}*/

string toHtml(string data) {
	for (size_t i=0; i<data.size(); ++i) {
		if (data[i]=='\n' || data[i] == '\r') {
			data.insert(i, string("<br>"));
			i = i+4;
		}
	}
	return data;
}

string filter_slashes(string data) {
	size_t pos;
	while (data.find("/./")!=std::string::npos) {
		pos = data.find("/./");
		data.erase(pos, 2);
	}
	while (data.find("//")!=std::string::npos) {
		pos = data.find("//");
		data.erase(pos, 1);
	}
	return data;
}
bool strToBool(string data) {
	if (data == "true" || data == "yes" || data=="1" || data=="TRUE" || data=="YES") return true;
	return false;
}

string boolToStr(bool value) {
	if (value) return "yes";
	return "no";
}

void strReplace(string *data, string pattern, string value) {
	if (!data || data->empty()) return;
	if (pattern.empty()) return;
	if (pattern == value) return;
	if (value.find(pattern)!=std::string::npos) {
		string proxy;
		while(proxy.empty() || data->find(proxy)!=std::string::npos) {
			proxy=get_tmp_file();
			unlink(proxy.c_str());
		}
		for (size_t pos = data->find(pattern); pos != std::string::npos; pos = data->find(pattern)) data->replace(pos, pattern.size(), proxy);
		pattern=proxy;
	}
	for (size_t pos = data->find(pattern); pos != std::string::npos; pos = data->find(pattern)) data->replace(pos, pattern.size(), value);
}

string getParseValue(string key, string data, bool toEndStr) {
	string ret = data.substr(data.find(key)+key.length());
	if (!toEndStr) ret = ret.substr(0, ret.find_first_of(" \t"));
	return ret;

}
vector<size_t> __strversSplitChunks(const string& version) {
	//printf("ver: %s\n", version.c_str());
	vector<size_t> d;
	if (cutSpaces(version).empty()) return d;
	d.push_back(0);
	for (size_t i=1; i<version.size(); ++i) {
		if (version[i-1]=='.' || version[i-1]=='_' || version[i-1]=='-') d.push_back(i);
		else {
			if (version[i]<'a') {
				if (version[i-1]>'9') d.push_back(i);
			}
			else {
				if (version[i-1]<'a') d.push_back(i);
			}
		}
	}
	return d;

}
int strverscmp2(const string& v1, const string& v2) {
	//printf("ver: %s\n", v1.c_str());
	//fprintf(stderr, "INPUT: '%s' '%s'\n", v1.c_str(), v2.c_str());
	//return strverscmp(v1.c_str(), v2.c_str());
	vector<size_t> d1=__strversSplitChunks(v1);
	vector<size_t> d2=__strversSplitChunks(v2);
	size_t chunks1 = d1.size();
	size_t chunks2 = d2.size();
	//printf("RECEIVED %d vs %d chunks\n", chunks1, chunks2);
	string s1, s2;
	int i1, i2;
	for (size_t i=1; i<chunks1+2 || i<chunks2+2; ++i) {
		if (i<chunks1) {
			s1 = v1.substr(d1[i-1], d1[i]-d1[i-1]);
		}
		else if (i==chunks1) s1 = v1.substr(d1[i-1]);
		else s1="0";
		strReplace(&s1, ".", "");
		strReplace(&s1, "_", "");
		strReplace(&s1, "-", "");
	
		if (i<chunks2) {
			s2 = v2.substr(d2[i-1], d2[i]-d2[i-1]);
		}
		else if (i==chunks2) s2 = v2.substr(d2[i-1]);
		else s2="0";
		strReplace(&s2, ".", "");
		strReplace(&s2, "_", "");
		strReplace(&s2, "-", "");
		for (size_t k=0; k<s1.size(); ++k) s1[k] = tolower(s1[k]);
		for (size_t k=0; k<s2.size(); ++k) s2[k] = tolower(s2[k]);
		if (strcmp(s1.c_str(), s2.c_str())==0) continue;

		if (strcmp(s1.c_str(), "rc")==0) i1=-1;
		else if (strcmp(s1.c_str(), "pre")==0) i1=-2;
		else if (strcmp(s1.c_str(), "beta")==0) i1=-3;
		else if (strcmp(s1.c_str(), "alpha")==0) i1=-4;
		else if (strcmp(s1.c_str(), "prealpha")==0) i1=-5;
		else if (strcmp(s1.c_str(), "git")==0 || strcmp(s1.c_str(), "svn")==0 || strcmp(s1.c_str(), "hg")==0 || strcmp(s1.c_str(), "r")==0 || strcmp(s1.c_str(), "rev")==0 || strcmp(s1.c_str(), "cvs")==0) i1=-6;
		else i1=atoi(s1.c_str());

		if (strcmp(s2.c_str(), "rc")==0) i2=-1;
		else if (strcmp(s2.c_str(), "pre")==0) i2=-2;
		else if (strcmp(s2.c_str(), "beta")==0) i2=-3;
		else if (strcmp(s2.c_str(), "alpha")==0) i2=-4;
		else if (strcmp(s2.c_str(), "prealpha")==0) i2=-5;
		else if (strcmp(s2.c_str(), "git")==0 || strcmp(s2.c_str(), "svn")==0 || strcmp(s2.c_str(), "hg")==0 || strcmp(s2.c_str(), "r")==0 || strcmp(s2.c_str(), "rev")==0 || strcmp(s2.c_str(), "cvs")==0) i2=-6;
		else i2=atoi(s2.c_str());
		if (i1<i2) {
		       //fprintf(stderr, "INPUT: %s %s, returning -1 as %s (as %d) < %s (as %d)\n", v1.c_str(), v2.c_str(), s1.c_str(), i1, s2.c_str(), i2);
		       return -1;
		}
		if (i1>i2) {
		       //fprintf(stderr, "INPUT: %s %s, returning 1 as %s (as %d) > %s (as %d)\n", v1.c_str(), v2.c_str(), s1.c_str(), i1, s2.c_str(), i2);
			return 1;
		}
		
		if (i1==0 && IntToStr(i1)!=s1) {
			if (i2==0 && IntToStr(i2)!=s2) {
				int scomp = strcmp(s1.c_str(), s2.c_str());
				if (scomp) {
		       			//fprintf(stderr, "INPUT: %s %s, returning -1 as %s (as string) < %s (as string)\n", v1.c_str(), v2.c_str(), s1.c_str(), s2.c_str());
					return scomp;
				}
			}
			else return 1;
		}
		else {
			if (i2==0 && IntToStr(i2)!=s2) {
		       		//fprintf(stderr, "INPUT: %s %s, returning -1 as %s (as %d) < %s (as string)\n", v1.c_str(), v2.c_str(), s1.c_str(), i1, s2.c_str());
				return -1;
			}
		}
	}

	return 0;
}

int compareVersions(const string& version1, const string& build1, const string& version2, const string& build2) // 0 == equal, 1 == v1>v2, -1 == v1<v2
{
	//printf("ver: %s\n", version1.c_str());
	//printf("ver: %s\n", version2.c_str());
	int ret;
	ret = strverscmp2(version1, version2);
	if (ret==0) {
		/*if (build2=="0" || build2.empty()) {
			if (build1=="0" || build1.empty()) return ret;
		}*/
		ret = strverscmp2(build1, build2);
	}
	return ret;
}

vector<string> splitString(const string& data, const string& delim) {
	vector<string> ret;
	size_t lastpos=0, prev_lastpos=0;
	do {
		prev_lastpos = lastpos;
		lastpos = data.substr(lastpos).find(delim);
		if (lastpos==std::string::npos) {
			ret.push_back(data.substr(prev_lastpos));
			return ret;
		}
		ret.push_back(data.substr(prev_lastpos, lastpos - prev_lastpos));
		lastpos+=delim.size() + prev_lastpos;
	} while (lastpos<data.size());
	return ret;
}


vector<string> mergeVectors(const vector<string>& vector1, const vector<string>& vector2) {
	vector<string> ret = vector1;
	for (size_t i=0; i<vector2.size(); ++i) {
		ret.push_back(vector2[i]);
	}
	return ret;
}
