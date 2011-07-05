/*****************************************************************
 * Repository working tools: creating index, decrypts data, etc.
 * 
 * Repository types supported:
 * 	Native MPKG (fully supported)
 * 	Legacy Slackware (generic format is fully supported, extensions support has limitations)
 *
 * $Id: repository.h,v 1.14 2007/10/20 10:34:50 i27249 Exp $
 *****************************************************************/
#ifndef REPOSITORY_H_
#define REPOSITORY_H_

#include "DownloadManager.h"
#include "local_package.h"
#include <ftw.h>

#define TYPE_AUTO 0
#define TYPE_MPKG 1
#define TYPE_SLACK 2
#define TYPE_DEBIAN 3
#define TYPE_RPM 4
// Packages & repositories type: mpkg

extern vector<string>* pkgListPtr;
class PkgList {
	public:
		PkgList(const vector<string>& paths);
		~PkgList();
		vector<string> getBaseFor(const string& filename);
		vector<string> pkglist;
};
class Repository
{
	public:
		int build_index(string _path=".", bool make_filelist = false); // builds index of packages (creates packages.xml), consuming REPOSITORY_ROOT is current dir by default
		int get_index(string server_url, PACKAGE_LIST *packages, unsigned int type=TYPE_AUTO);
		Repository();
		~Repository();

		vector< pair<string, string> > *package_descriptions;
		
	private:
	//	int ProcessPackage(const char *filename, const struct stat *file_status, int filetype);
		PACKAGE_LIST data;
		string path;
};
//void analyzeFTree(XMLNode *node);
int ProcessPackage(const char *filename, const struct stat *file_status, int filetype);
int slackpackages2list (string *packageslist, string *md5list, PACKAGE_LIST *pkglist, string server_url);
bool validateRepStr(const string &);


#endif


