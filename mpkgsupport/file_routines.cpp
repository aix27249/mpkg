/*******************************************************
 * File operations
 * $Id: file_routines.cpp,v 1.51 2007/12/07 03:34:20 i27249 Exp $
 * ****************************************************/

#include "file_routines.h"
#include <assert.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <fstream>
#include <iostream>
#include "constants.h"
//#include "debug.h"
#include "globals.h"
//vector<string> temp_files;
TempFileController::TempFileController() {
}

TempFileController::~TempFileController() {
	clear_all();
}

int getProcessPid(const string& name) {
	string tmp = get_tmp_file();
	system("pidof " + name + " > " + tmp);
	string pid = ReadFile(tmp);
	unlink(tmp.c_str());
	if (pid.empty() || pid.find_first_not_of(" \t\n") == std::string::npos) return -1;
	else return atoi(pid.c_str());
}
string TempFileController::create() {
	string tmp_fname;
	char t[]="/tmp/mpkg-XXXXXX";
	int fd;
	fd=mkstemp(t);
	if ( t == NULL  ) {
		perror(_("Failed to create temp file"));
		abort();
	}
	tmp_fname=t;
	tFiles.push_back(tmp_fname);
	close(fd);
	return tmp_fname;

}
void TempFileController::clear_all() {
	for (size_t i=0; i<tFiles.size(); ++i) {
		if (access(tFiles[i].c_str(), F_OK)) unlink(tFiles[i].c_str()); // If plain file
		else if (access(string(tFiles[i]+".gz").c_str(), F_OK)) unlink(string(tFiles[i]+".gz").c_str()); // Sometimes we use gz extension
		system("rm -rf " + tFiles[i]); // And sometimes it is a directory
	}
	tFiles.clear(); // Clean-up list - for future use
}

void TempFileController::add(const string& external_tmp) {
	tFiles.push_back(external_tmp);
}
TempFileController tempFileController;
extern int errno;

// FIXME: Unitialized jump
vector<string> getDirectoryList(const string& directory_name, vector<int> *types, bool do_sort)
{
	vector<string> ret;
	vector<int> types_local;
	if (types) types->clear();
	DIR *patchDir = opendir(directory_name.c_str());
	if (!patchDir) {
		perror(_("Directory list"));
		//mWarning("Folder " + directory_name + " doesn't exist");
		return ret;
	}
	struct dirent *dEntry = readdir(patchDir);
	struct stat st;
	string d;
	string fullname;
	while (dEntry != NULL) {
		d = (string) dEntry->d_name;
		if (d!="." && d!="..") {
			fullname = directory_name + d;
			stat(fullname.c_str(), &st);
				
			if (S_ISDIR(st.st_mode)) types_local.push_back(TYPE_DIR);
			else  types_local.push_back(TYPE_FILE);
			ret.push_back(d);
		}
		dEntry = readdir(patchDir);
	}
	closedir(patchDir);
	if (do_sort) {
		vector<string> ret_dir, ret_reg;
		for (unsigned int i=0; i<types_local.size(); ++i) {
			if (types_local[i]==TYPE_DIR) {
				ret_dir.push_back(ret[i]);
				if (types) types->push_back(types_local[i]);
			}
		}
		for (unsigned int i=0; i<types_local.size(); ++i) {
			if (types_local[i]!=TYPE_DIR) {
				ret_reg.push_back(ret[i]);
				if (types) types->push_back(types_local[i]);
			}
		}
		sort(ret_dir.begin(), ret_dir.end());
		sort(ret_reg.begin(), ret_reg.end());
		ret = ret_dir;
		for (unsigned int i=0; i<ret_reg.size(); ++i) {
			ret.push_back(ret_reg[i]);
		}
	}
	else if (types) *types = types_local;
	return ret;
}

bool isProcessRunning(const string& pid)
{
	if (pid.empty() || pid.find_first_not_of("0123456789")!=std::string::npos) return false;
	string tmp_file = get_tmp_file();
	system("ps -p " + pid + " > " + tmp_file);
	string result = ReadFile(tmp_file);
	unlink(tmp_file.c_str());
	if (result.find(pid)!=std::string::npos)
	{
		// Process is running
		return true;
	}
	else return false;
}
bool isDirectory(const string& dir_name)
{
	struct stat fStat;
	int ret = stat(dir_name.c_str(), &fStat);
	if (ret!=0) { 
		//perror(dir_name.c_str());
		return false;
	}
	if (S_ISDIR(fStat.st_mode)) {
		return true;
	}
	else return false;
}

int system(const string& cmd)
{
/*	vector<string> log = ReadFileStrings("/tmp/mpkg_system.log");
	log.push_back(cmd);
	WriteFileStrings("/tmp/mpkg_system.log", log);*/
	int ret = system(cmd.c_str());
	return ret;
}
long double get_disk_freespace(const string& point)
{
	//mDebug("Checking free space");
	struct statfs buf;
	int s_ret = statfs(point.c_str(), &buf);
	if (s_ret!=0)
	{
		perror(string("Unable to determine FS parameters of " + point + ": ret = " + IntToStr(s_ret)).c_str());
		perror("get_disk_freespace");
		return 0;
	}
	long double dfree;
        dfree	= (long double) buf.f_bfree * (long double) buf.f_bsize;
	//mDebug("Free on " + point + ": " + IntToStr((long long) dfree));
	return (long double) dfree;
}

string get_file_md5(const string& filename) {
	string tmp_md5=get_tmp_file();
	system("md5sum '"+filename+"' > "+tmp_md5 + " 2>/dev/null");
	string ret = ReadFile(tmp_md5);
	unlink(tmp_md5.c_str());
	if (ret.find(" ")!=std::string::npos) return ret.substr(0, ret.find(" "));
	else return "";
}
/*
string get_file_md5(const string& filename)
{
	string tmp_md5=get_tmp_file();

	string sys="md5sum '"+filename+"' > "+tmp_md5 + " 2>/dev/null";
	system(sys.c_str());
	FILE* md5=fopen(tmp_md5.c_str(), "r");
	if (!md5)
	{
		perror("Unable to open md5 temp file");
		return "";
	}
	char *_c_md5 = (char *) malloc(1000);
	memset(_c_md5, 0, 1000);
	fscanf(md5, "%s", _c_md5);
	string md5str;
	md5str= (string) _c_md5;
	fclose(md5);
	free(_c_md5);
	unlink(tmp_md5.c_str());
	return md5str;
}
*/
int get_file_type_stat(string filename) {
	struct stat st;
	if (lstat(filename.c_str(), &st)) {
		return -1;
	}
	if (S_ISREG(st.st_mode)) return FTYPE_PLAIN;
	if (S_ISDIR(st.st_mode)) return FTYPE_DIR;
	if (S_ISCHR(st.st_mode)) return FTYPE_CHAR;
	if (S_ISLNK(st.st_mode)) return FTYPE_SYMLINK;
	if (S_ISBLK(st.st_mode)) return FTYPE_BLK;
	if (S_ISFIFO(st.st_mode)) return FTYPE_FIFO;
	if (S_ISSOCK(st.st_mode)) return FTYPE_SOCK;
	return -2;
}

string get_tmp_file() {
	return tempFileController.create();
}
void add_tmp_file(const string& ext_file) {
	tempFileController.add(ext_file);
}
string get_tmp_dir() {
	string ret = get_tmp_file();
	unlink(ret.c_str());
	system("mkdir " + ret);
	return ret;
}

void delete_tmp_files()
{
	// Disabled for now: let it clear all at exit
	//tempFileController.clear_all();
}

// FIXME: Unitialized jump
bool FileExists(const string& filename, bool *broken_symlink)
{
	
	//mDebug("checking the existance of " + filename);
	if (broken_symlink) *broken_symlink=false;
	if (access(filename.c_str(), F_OK)==0) {
		//mDebug("File exists, returning true"); 
		return true; 
	}
	else 
	{
		if (broken_symlink) {
			struct stat st;
			lstat(filename.c_str(), &st);
			if (S_ISLNK(st.st_mode)) {
				*broken_symlink=true;
			}
		}
	}
	//mDebug("File [" + filename + "] doesn't exist");
	return false;
}

long double getFileSize(const string& filename) {
	struct stat st;
	if (!FileExists(filename)) {
		//mWarning(filename + " doesn't exist");
		return -1;
	}
	lstat(filename.c_str(), &st);
	//printf("==============================================%d\n", st.st_size);
	return (long double) st.st_size;
}

bool FileNotEmpty(const string& filename)
{
	ifstream test(filename.c_str(), ios::in);
	ifstream::pos_type nullsize;
	if (test.is_open())
	{
		nullsize = test.tellg();
		test.close();
	}
	else {
		return false;
	}
	ifstream test2(filename.c_str(), ios::in | ios::ate);
	ifstream::pos_type endsize;
	if (test2.is_open())
	{
		endsize = test2.tellg();
		test2.close();
	}
	else {
       	       return false;
	}
	if (endsize == nullsize)
	{
		return false;
	}
	else
	{
		return true;
	}
}

string ReadFile(const string& filename) // Reads the text file
{
	ifstream filestr;
	long size;
	filebuf *pbuf;
	string sbuffer;
	filestr.open(filename.c_str());
	if (!filestr.is_open()) return "";
	
	pbuf=filestr.rdbuf();
	
	size=pbuf->pubseekoff(0,ios::end,ios::in);
	pbuf->pubseekpos(0,ios::in);
	
	sbuffer.resize(size+1);
	pbuf->sgetn(&sbuffer[0], size);
	
	filestr.close();
	
	// Terminating the string
	//sbuffer[size]=0;
	return sbuffer.c_str();
}
/*
string ReadFile(const string& filename) // Reads the text file
{
	ifstream filestr;
	long size;
	filebuf *pbuf;
	string sbuffer;
	filestr.open(filename.c_str());
	if (!filestr.is_open()) return "";
	pbuf=filestr.rdbuf();
	size=pbuf->pubseekoff(0,ios::end,ios::in);
	pbuf->pubseekpos(0,ios::in);
	sbuffer.resize(size+1);
	//char *zbuffer = (char *) malloc(size);
	//memset(zbuffer, 0, size);
	filestr.get(&sbuffer[0], size, 0);
	sbuffer.resize(sbuffer.size()-1);
//	for (size_t i=0; i<size; ++i) {
//		zbuffer[i]=filestr.get();
//	}
	
//	sbuffer = string(zbuffer);
	//free(zbuffer);
	filestr.close();
	
	// Terminating the string
	//sbuffer[size]=0;
	return sbuffer;
}
*/
int WriteFileStrings(const string& filename, const vector<string>& data) {
	ofstream filestr;
	filestr.open(filename.c_str());
	if (!filestr.is_open()) {
		return -1;
	}
	for (size_t i=0; i<data.size(); ++i) {
		filestr << data[i] << "\n";
	}
	filestr.close();
	return 0;

}
// FIXME: None of these functions works properly. FIX IT!!!!!!!
// THIS IS PRIORITY CRITICAL.
//
/*vector<string> ReadFileStrings(const string& filename) {
	string data = ReadFile(filename);
	vector<string> ret;
	if (data.empty()) return ret;
	size_t next_pos=0, prev_pos = 0;
	strReplace(&data, "\r\n", "\n");
	next_pos = data.find_first_of("\n");
	if (next_pos == std::string::npos) {
		ret.push_back(data);
		return ret;
	}
	do {
		ret.push_back(data.substr(prev_pos, next_pos));
		prev_pos += next_pos+1;
		if (prev_pos >= data.size()) next_pos = data.substr(prev_pos).find_first_of("\n");
		else return ret;
		if (next_pos==std::string::npos) {
			ret.push_back(data.substr(prev_pos));
			return ret;
		}
	} while (true);
	return ret;
}*/

vector<string> ReadFileStrings(const string& filename) {
	return MakeStrings(ReadFile(filename));
}
/*
vector<string> ReadFileStrings(const string& filename) {
	printf("EEEEEE\n");
	string data = ReadFile(filename);
	vector<string> ret;
	vector<size_t> n_pos;
	size_t next_pos=0;
	n_pos.push_back(-1);
	strReplace(&data, "\r\n", "\n");
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
}*/
int extractFiles(const string& filename, const string& files_to_extract, const string& output_dir, string file_type)
{
	if (!FileExists(output_dir)) {
		//mError("Cannot extract to %d: no such file or directory"); // FIXME
		return -1;
	}
	string tar_cmd;
	if (file_type.empty()) file_type = getExtension(filename);
	if (file_type=="tgz" || \
			file_type == "gz" || \
			file_type == "tbz" || \
			file_type == "txz" || \
			file_type == "tlz" || \
			file_type == "bz2" || \
			file_type == "xz" || \
			file_type == "lzma" || \
			file_type == "spkg" || \
			file_type == "tar") 
		tar_cmd = "tar xf ";

	return system("(cd " + output_dir+"; " + tar_cmd + getAbsolutePath(filename) + " " + files_to_extract + " 2>/dev/null);");
}


int extractFromTgz(const string& filename, const string& file_to_extract, const string& output, string file_type)
{
	string tar_cmd;
	if (file_type.empty()) file_type = getExtension(filename);
	if (file_type=="tgz" || \
			file_type == "gz" || \
			file_type == "tbz" || \
			file_type == "txz" || \
			file_type == "tlz" || \
			file_type == "bz2" || \
			file_type == "xz" || \
			file_type == "lzma" || \
			file_type == "spkg" || \
			file_type == "tar") 
		tar_cmd = "tar xf ";

	if (tar_cmd.empty()) {
		//mError("Unable to determine type of archive, trying tar autodetect..."); // FIXME
		tar_cmd = "tar xf ";
	}
	return system(tar_cmd + filename + " " + file_to_extract + " --to-stdout > " + output + " 2>/dev/null");
}
int extractFromTar(string filename, string file_to_extract, string output)
{
	return extractFromTgz(filename, file_to_extract, output, "tar");
}
// Deprecated: use extractFromTgz instead
/*int extractFromTbz2(string filename, string file_to_extract, string output)
{
	string cmd = "tar jxf "+filename+" "+ file_to_extract + " --to-stdout > " + output + " 2>/dev/null";
	return system(cmd.c_str());
}*/

// Old WriteFile: ugly and very slow
/*
int WriteFile(string filename, string data)
{
	mpkgErrorReturn errRet;
write_file:
	FILE* output=fopen(filename.c_str(),"w");
	if (output)
	{
		for (unsigned int i=0;i<data.length();i++)
		{
			fputc(data[i],output);
		}
		fclose(output);
		return 0;
	}
	else
	{
		mError("Unable to write file " + filename);
		errRet = waitResponce(MPKG_SUBSYS_FILE_WRITE_ERROR);
		if (errRet == MPKG_RETURN_RETRY)
			goto write_file;

		abort();
		return -1;
	}
}
*/
int WriteFile(const string& filename, const string& data) {
	ofstream filestr;
	filestr.open(filename.c_str());
	if (!filestr.is_open()) {
		abort();
	}
	filestr << data;
	filestr.close();
	return 0;
}


unsigned int CheckFileType(string fname)
{
	// Checking file existanse
	struct stat st;
	if (lstat(fname.c_str(), &st) != 0) {
		if ( errno == ENOENT ) {
			//mError("file "+fname+" not found"); // FIXME
			return PKGTYPE_UNKNOWN;
		}
	}
	
	if ( !S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode) ) {
		//mError("Not a regular file"); // FIXME
		return PKGTYPE_UNKNOWN;
	}

	// Checking extensions and (partially) contents
	string ext = getExtension(fname);
	if (ext=="tgz" || ext == "txz" || ext == "tlz" || ext == "tbz")	return PKGTYPE_BINARY;
	if (ext=="spkg") {
		//mDebug("Source MPKG package detected");
		return PKGTYPE_SOURCE;
	}
//	mDebug("Unknown package type "+ext);
	return PKGTYPE_UNKNOWN;
}
bool isMounted(string mountpoint) {
	if (_cdromMounted) return true;
	if (mountpoint.find_last_of("/")>=mountpoint.length()-1) mountpoint = mountpoint.substr(0,mountpoint.length()-1);
//	mountpoint = mountpoint.substr(0,find_last_of("/")-1);
//	mDebug("Checking if [" + mountpoint + "] is mounted");
#ifdef _MNTENTMCHECK
	// First, check if device mounted in proper directory. 
	bool mounted=false;
	struct mntent *mountList;
	FILE *mtab = fopen("/proc/mounts", "r");
	//char volname[2000];
	if (mtab)
	{
		mountList = getmntent(mtab);
		while ( !mounted && mountList != NULL )
		{
			if (strcmp(mountList->mnt_dir, mountpoint.c_str())==0)
			{
				/*if (strcmp(mountList->mnt_dir, CDROM_DEVICE.c_str())!=0)
				{
					umount(mountpoint.c_str());
				}
				else*/ mounted = true;
			}
			mountList = getmntent(mtab);
		}
		fclose(mtab);
	}
/*	if (mounted) mDebug(mountpoint + " is mounted");
	else mDebug(mountpoint + " isn't mounted");*/
	return mounted;
#else
	string out = get_tmp_file();
	system("cat /proc/mounts | grep " + mountpoint + " | wc -l >" + out );
	string ret = ReadFile(out);
	unlink(out.c_str());
	if (ret.empty() || ret[0]=='0') {
		//mDebug(mountpoint + " isn't mounted");
		return false;
	}
	else {
		//mDebug(mountpoint + " is already mounted");
		return true;
	}
#endif

}

string getCdromVolname(string *rep_location)
{
	//mDebug("checking in location " + CDROM_MOUNTPOINT);
	bool hasMountedHere=false;
	if (!isMounted(CDROM_MOUNTPOINT))
	{
		system("mount " + CDROM_DEVICE + " " + CDROM_MOUNTPOINT + " 2>/dev/null >/dev/null");
		hasMountedHere=true;
	}
	string Svolname, repLoc;
	// check_volname:
	if (FileExists(CDROM_MOUNTPOINT + "/.volume_id")) {
		vector<string> tmp = ReadFileStrings(CDROM_MOUNTPOINT + "/.volume_id");
		if (!tmp.empty()) {
			Svolname = cutSpaces(tmp[0]);
		}
	}
	if (rep_location!=NULL)
	{
		if (FileExists(CDROM_MOUNTPOINT + "/.repository")) {
			vector<string> tmp = ReadFileStrings(CDROM_MOUNTPOINT + "/.repository");
			if (!tmp.empty()) {
				repLoc = cutSpaces(tmp[0]);
			}
		}
	}
	if (hasMountedHere) system("umount " + CDROM_MOUNTPOINT + " 2>/dev/null >/dev/null");
	// Validating
	if (Svolname.find_first_of("\n\t/><| !@#$%%^&*()`\"\'")!=std::string::npos)
	{
		/*if (dialogMode) mError("Invalid volname [" + Svolname+"]"); */
		//else ncInterface.showMsgBox(_("Invalid volname [") + Svolname + "]"); // FIXME: no backlinks to ncInterface
		return "";
	}
	if (rep_location!=NULL)
	{
		/*if (repLoc.find_first_of("<>|!@#$%^&*()`\"\'")!=std::string::npos)
		{
			if (dialogMode) mError("Invalid repository path");
			else ncInterface.showMsgBox(_("Invalid repository path ") + repLoc);
			return "";
		}*/
		*rep_location = repLoc;
	}
	
	return Svolname;

}

bool cacheCdromIndex(string vol_id, string rep_location)
{
	//mDebug("Caching index for ["+vol_id+"] in with location [" + rep_location+"]");
	string rep_location_id = rep_location;
	strReplace(&rep_location_id, "/", "_");
	bool ret = false;
	if (system("mkdir -p /var/mpkg/index_cache/"+vol_id + "~" + rep_location_id)==0 && system("cp -f /var/log/mount/" +rep_location+"/packages.xml.gz /var/mpkg/index_cache/"+vol_id + "~" + rep_location_id + "/")==0) ret = true;
	if (setupMode && FileExists("/var/log/mount/" +rep_location+"/setup_variants.list")) {
		system("cp -f /var/log/mount/" +rep_location+"/setup_variants.list /var/mpkg/index_cache/"+vol_id + "~" + rep_location_id + "/");
		system("mkdir -p /var/mpkg/index_cache/"+vol_id + "~" + rep_location_id + "/setup_variants");
		system("cp -f /var/log/mount/" +rep_location+"/setup_variants/* /var/mpkg/index_cache/"+vol_id + "~" + rep_location_id + "/setup_variants/");
	}
	return ret;
}

bool copyFile(string source, string destination)
{
	if (system("cp -Rf " + source + " " + destination)==0) return true;
	else return false;
}

bool moveFile(string source, string destination)
{
	if (system("mv " + source + " " + destination)==0) return true;
	else return false;
}

bool removeFile(string source)
{
	if (system("rm -rf " + source)==0) return true;
	else return false;
}
/*bool copyDir(string source, string destination)
{
	if (system("cp -Rf " + source + " " + destination)==0) return true;
	else return false;
}*/




