/************************************************************
 *
 *	MPKG packaging system
 *	XML parsing helper: reads XML, creates XML for
 *	packages and whole repository
 *
 *	$Id: PackageConfig.h,v 1.27 2007/12/10 03:12:58 i27249 Exp $
 *
 * **********************************************************/


// Package type: mpkg

#ifndef PACKAGECONFIG_H_
#define PACKAGECONFIG_H_

#include <string>
#include <vector>
#include <cassert>

#include "xmlParser.h"
#include "dataunits.h"
#include <libxml/parser.h>
#include <libxml/xpath.h>
using namespace std;

struct depTree { bool name, version, condition, build_only; } ;

#define XPATH_CTX_ERR (mDebug("Failed to create XPath context"))
#define XPATH_EVAL_ERR (mDebug("XPath eval error"))
#define EMPTYVECTOR ( new std::vector<std::string>() )


// Binary package XML paths
#define GET_PKG_PROVIDES ((const xmlChar *)"//package/provides")
#define GET_PKG_CONFLICTS ((const xmlChar *)"//package/conflicts")
#define GET_PKG_NAME  ((const xmlChar *)"//package/name")
#define GET_PKG_VERSION ((const xmlChar *)"//package/version")
#define GET_PKG_ARCH ((const xmlChar *)"//package/arch")
#define GET_PKG_BUILD ((const xmlChar *)"//package/build")
#define GET_PKG_SHORT_DESCRIPTION ((const xmlChar *)"//package/short_description")
#define GET_PKG_DESCRIPTION ((const xmlChar *)"//package/description")
#define GET_PKG_MAINT_NAME ((const xmlChar *)"//package/maintainer/name")
#define GET_PKG_MAINT_EMAIL ((const xmlChar *)"//package/maintainer/email")
#define GET_PKG_LOCATION ((const xmlChar *)"//package/location")
#define GET_PKG_FILENAME ((const xmlChar *)"//package/filename")
#define GET_PKG_BETARELEASE ((const xmlChar *)"//package/betarelease")
#define GET_PKG_MD5 ((const xmlChar *)"//package/md5")
#define GET_PKG_CHANGELOG ((const xmlChar *)"//package/changelog")
#define GET_PKG_COMP_SIZE ((const xmlChar *)"//package/compressed_size")
#define GET_PKG_INST_SIZE ((const xmlChar *)"//package/installed_size")
#define GET_PKG_TAGS ((const xmlChar *)"//package/tags/tag")
#define GET_PKG_TYPE ((const xmlChar *)"//package/type")
#define GET_PKG_FILE_LIST ((const xmlChar *)"//package/files/file")
#define GET_PKG_SUG ((const xmlChar *) "//suggests/suggest")
#define GET_PKG_SUG_COND ((const xmlChar *) "//suggests/suggest/condition")
#define GET_PKG_SUG_NAME ((const xmlChar *) "//suggests/suggest/name")
#define GET_PKG_SUG_VERSION ((const xmlChar *) "//suggests/suggest/version")
#define GET_PKG_CONFIG_FILE_LIST ((const xmlChar *)"//package/configfiles/conffile")
#define GET_PKG_TEMP_FILE_LIST ((const xmlChar *)"//package/tempfiles/tempfile")
#define GET_PKG_DEP ((const xmlChar *)"//package/dependencies/dep")
#define GET_PKG_DEP_NAME ((const xmlChar *)"//dependencies/dep/name")
#define GET_PKG_DEP_COND ((const xmlChar *)"//dependencies/dep/condition")
#define GET_PKG_DEP_VERSION ((const xmlChar *)"//dependencies/dep/version")
#define GET_PKG_DEP_BUILDONLY ((const xmlChar *)"//dependencies/dep/build_only")

#define GET_PKG_ENUM_DEP_NAME ((const xmlChar *)"//dependencies/dep[name]")
#define GET_PKG_ENUM_DEP_VERSION ((const xmlChar *)"//dependencies/dep[version]")
#define GET_PKG_ENUM_DEP_COND ((const xmlChar *)"//dependencies/dep[condition]")
#define GET_PKG_ENUM_DEP_BUILDONLY ((const xmlChar *)"//dependencies/dep[build_only]")
#define GET_PKG_BDELTA ((const xmlChar*)"//bdelta")
// Repository extensions
#define GET_PKG_REP_TAGS ((const xmlChar *)"//package/repository_tags")

// MPKG-SRC extensions
#define GET_PKG_MBUILD_URL ((const xmlChar *)"//mbuild/url")
#define GET_PKG_MBUILD_ADVANCED_URL_LIST ((const xmlChar *)"//mbuild/source_list/source")
#define GET_PKG_MBUILD_PATCH_LIST ((const xmlChar *)"//mbuild/patches/patch")
#define GET_PKG_MBUILD_SOURCEROOT ((const xmlChar *)"//mbuild/sources_root_directory")
#define GET_PKG_MBUILD_BUILDSYSTEM ((const xmlChar *)"//mbuild/build_system")
#define GET_PKG_MBUILD_MAX_NUMJOBS ((const xmlChar *)"//mbuild/max_numjobs")
#define GET_PKG_MBUILD_OPTIMIZATION_ALLOW_CHANGE ((const xmlChar *)"//mbuild/optimization/allow_change")
#define GET_PKG_MBUILD_OPTIMIZATION_MARCH ((const xmlChar *)"//mbuild/optimization/march")
#define GET_PKG_MBUILD_OPTIMIZATION_MTUNE ((const xmlChar *)"//mbuild/optimization/mtune")
#define GET_PKG_MBUILD_OPTIMIZATION_LEVEL ((const xmlChar *)"//mbuild/optimization/olevel")
#define GET_PKG_MBUILD_OPTIMIZATION_CUSTOM_GCC_OPTIONS ((const xmlChar *)"//mbuild/optimization/custom_gcc_options")
#define GET_PKG_MBUILD_CONFIGURATION_KEY_NAME ((const xmlChar *)"//mbuild/configuration/key/name")
#define GET_PKG_MBUILD_CONFIGURATION_KEY_VALUE ((const xmlChar *)"//mbuild/configuration/key/value")
#define GET_PKG_MBUILD_USE_CFLAGS ((const xmlChar *)"//mbuild/use_cflags")
#define GET_PKG_MBUILD_CMD_CONFIGURE ((const xmlChar *)"//mbuild/custom_commands/configure")
#define GET_PKG_MBUILD_CMD_MAKE ((const xmlChar *)"//mbuild/custom_commands/make")
#define GET_PKG_MBUILD_CMD_MAKEINSTALL ((const xmlChar *)"//mbuild/custom_commands/make_install")
#define GET_PKG_MBUILD_NO_SUBFOLDER ((const xmlChar *)"//mbuild/no_subfolder")
#define GET_PKG_MBUILD_ENVOPTIONS ((const xmlChar *)"//mbuild/env_options")
#define GET_PKG_MBUILD_NOSTRIP ((const xmlChar *)"//mbuild/nostrip")
// PGP signature specific
#define GET_PKG_PGP_SIGNATURE ((const xmlChar *)"//package/signature")

class PackageConfig
{
public:
	PackageConfig(string _f);
	PackageConfig(xmlChar * membuf, int bufsize);
	StringMap dataCache;
	PackageConfig(xmlNodePtr __rootXmlNodePtr);
	~PackageConfig();
	const string& getValue(string data_xpath);
	void buildDepDef(void);
	void buildSugDef(void);
	const string& getName(void);
	string pName;
	const string& getVersion(void);
	string pVersion;
	const string& getBetarelease(void);
	string pBetarelease;
	const string& getArch(void);
	string pArch;
	const string& getBuild(void);
	string pBuild;
	
	const string& getAuthorName(void);
	string pAuthorName;
	const string& getAuthorEmail(void);
	string pAuthorEmail;
	const string& getPackageFileName(void);
	string pPackageFileName;
	const string& getPackageType(void);
	string pPackageType;

	const string& getProvides(void);
	string pProvides;

	const string& getConflicts(void);
	string pConflicts;

	const string& getDescription(string lang="");
	string pDescription;
	//string getDescriptionI(int num=0);
	const string& getShortDescription(string lang="");
	string pShortDescription;
	//string getShortDescriptionI(int num=0);

//	string getDependencyName(int dep_num);
//	string getDependencyCondition(int dep_num);
//	string getDependencyVersion(int dep_num);
	//string getTag(int tag_num);

	const string& getChangelog(void);
	string pChangelog;

	//string getFile(int file_num);
	//string getConfigFile(int file_num);
	vector <string> getDepNames(void);
	vector <string> getDepConditions(void);
	vector <string> getDepVersions(void);
	vector <bool> getDepBuildOnlyFlags(void);
	vector <string> getSuggestNames(void);
	vector <string> getSuggestConditions(void);
	vector <string> getSuggestVersions(void);
	vector <string> getTags(void);
	vector <string> getFilelist(void);
	vector <string> getConfigFilelist(void);
	vector <string> getTempFilelist(void);
	
	string getBuildUrl(void);
	StringMap getBuildAdvancedUrlMap(void);
	string getBuildSourceRoot(void);
	string getBuildSystem(void);
	string getBuildMaxNumjobs(void);
	bool getBuildOptimizationCustomizable(void);
	bool getBuildNoSubfolder(void);
	string getBuildOptimizationMarch(void);
	string getBuildOptimizationMtune(void);
	string getBuildOptimizationLevel(void);
	string getBuildOptimizationCustomGccOptions(void);
	string getBuildConfigureEnvOptions(void);
	string getBuildCmdConfigure(void);
	string getBuildCmdMake(void);
	string getBuildCmdMakeInstall(void);
	bool getBuildUseCflags(void);
	vector<string> getBuildKeyNames(void);
	vector<string> getBuildKeyValues(void);
	vector<string> getBuildPatchList(void);
	string getRepositoryTags(void);
	string getPGPSignature(void);

	xmlNodePtr getXMLNode(void);
	xmlDocPtr getXMLDoc(void);

	// Repository-related functions
	string getMd5(void);
	string getCompressedSize(void);
	string getInstalledSize(void);
	string getFilename(void);
	string getLocation(void);
	xmlChar * getXMLNodeXPtr(int * bufsize);
	vector<DeltaSource> getBDeltas(void);
	//std::string getXMLNodeEx();

	bool parseOk;

	bool hasErrors();

private:
	vector<depTree> dependencyTreeDef, suggestTreeDef;
	string fileName;
	XMLNode _node;
	XMLNode tmp;
	string EMPTY;
    size_t errors;
    size_t depCount, suggestCount;
    xmlDocPtr doc;
    xmlNodePtr curNode;

    xmlXPathObjectPtr getNodeSet(const xmlChar * exp);
};

#endif /*PACKAGECONFIG_H_*/
