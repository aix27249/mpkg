/*
Local package installation functions
$Id: local_package.h,v 1.23 2007/11/02 20:19:45 i27249 Exp $
*/


#ifndef LOCAL_PACKAGE_H_
#define LOCAL_PACKAGE_H_

#include <sys/param.h>

#include "dependencies.h"
#include "PackageConfig.h"
#include "debug.h"
//#include "file_routines.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
void setXmlDeltaSources(const vector<DeltaSource>& deltasources, xmlNodePtr _packageXMLNode);
int xml2package(xmlDocPtr doc, xmlNodePtr cur, PACKAGE *data);
int xml2spkg(xmlNodePtr pkgnode, SPKG *data);
bool splitSlackwareFilename(string filename, string *name, string *version, string *arch, string *build);
// Package type: mpkg
class LocalPackage
{
	public:
		LocalPackage(string _f, string _path_shift = "");
		~LocalPackage();
		PACKAGE data;
		int injectFile();
		xmlNode getPackageXMLNode();
		//std::string getPackageXMLNodeEx();
		xmlChar * getPackageXMLNodeXPtr(int * bufsize);
		xmlNode _nd;
		//XMLNode getPackageFListNode();
		int fill_filelist(PACKAGE *package=NULL, bool index=false);
		int fill_scripts(PACKAGE *package);
		int fill_configfiles(PACKAGE *package);
		int create_signature(string passwd=""); // Creates PGP signature for package
		bool isNative();
		void set_repository_branch(const string &branch);
		void set_distro_version(const string &distro);
		void set_deltasources(const vector<DeltaSource>& deltasources);
		bool legacyPackage;
		bool _parseOk;
	private:
		bool native;
		bool internal;
		string files2xml(string input);
		string filename;
		//XMLNode _packageXMLNode, _packageFListNode;
		xmlNodePtr _packageXMLNode, _packageFListNode;
		xmlDocPtr __doc;
		int create_md5();
		int get_size();
		int get_xml();
		int get_filelist(bool index=false);
		int set_additional_data();
		int CreateFlistNode(string fname, string tmp_xml);
		string path_shift;

};

#endif //LOCAL_PACKAGE_H_

