/******************************************************
* File operations
* $Id: file_routines.h,v 1.27 2007/11/21 01:57:54 i27249 Exp $
* ****************************************************/
#ifndef FILE_ROUTINES_H_
#define FILE_ROUTINES_H_
#include <string>
#include <vector>
#include <math.h>
//#include "config.h"
//#include "dataunits.h"
#include "string_operations.h"
using namespace std;
#define PKGTYPE_UNKNOWN -1
#define PKGTYPE_BINARY 0
#define PKGTYPE_SOURCE 1


#include <unistd.h>
#include <errno.h>
#include <sys/dir.h>
#include <sys/types.h>
#include <sys/param.h>

#define TYPE_DIR 0
#define TYPE_FILE 1
int getProcessPid(const string& name);
vector<string> getDirectoryList(const string& directory_name, vector<int> *types = NULL, bool sort=true);
bool isDirectory(const string& dir_name);
bool isMounted(string mountpoint);
int system(const string& cmd);
long double get_disk_freespace(const string& point="/");
long double getFileSize(const string& filename);
string get_file_md5(const string& filename);
string get_tmp_file();
string get_tmp_dir();
void delete_tmp_files();
bool FileExists(const string& filename, bool *broken_symlink=NULL);
bool FileNotEmpty(const string& filename);
string ReadFile(const string& filename);
int WriteFile(const string& filename, const string& data);
int extractFiles(const string& filename, const string& files_to_extract, const string& output_dir, string file_type="");
int extractFromTgz(const string& filename, const string& file_to_extract, const string& output, string file_type="");
int extractFromTar(string filename, string file_to_extract, string output); // Should be replaced by extractFiles in most cases
int extractFromTbz2(string filename, string file_to_extract, string output);// Should be replaced by extractFiles in most cases

string getCdromVolname(string *rep_location=NULL);
bool cacheCdromIndex(string vol_id, string rep_location);
// Package type definition (also defines repository type)
//vector<string> ReadFileStringsN(string filename);
vector<string> ReadFileStrings(const string& filename);
int WriteFileStrings(const string& filename, const vector<string>& data);
unsigned int CheckFileType(const string& fname);
bool copyFile(string source, string destination);
bool moveFile(string source, string destination);
bool removeFile(string source);
int get_file_type_stat(string filename);
class TempFileController {
	public:
		TempFileController();
		~TempFileController();
		string create();
		void clear_all();
		//string clear(string name);
	private:
		vector<string> tFiles;
};

bool isProcessRunning(const string& pid);
// Some advanced interface functions
//string ncGetOpenFile(string defaultDir="/");
//string ncGetOpenDir(string defaultDir="/");
#endif
