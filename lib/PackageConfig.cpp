/*
* XML parser of package config
* $Id: PackageConfig.cpp,v 1.43 2007/12/10 03:12:58 i27249 Exp $
*/
//#include "file_routines.h"
#include <mpkgsupport/mpkgsupport.h>
#include "PackageConfig.h"
#include "debug.h"
#include <libxml/parser.h>
#include <libxml/xpath.h>

using namespace std;

/**
 * 
 * @param _f path to file
 */
PackageConfig::PackageConfig(string _f)
{
	depCount=-1; suggestCount=-1;
    	this->errors = 0;
	// Because new libxml2 desn't understand leading byte-order bytes present in some (in real, most) packages, let's truncate them
	string xmlData = ReadFile(_f);
	unsigned long int xmlStart = xmlData.find("<?xml");
	if (xmlStart==std::string::npos) {
		mError("Failed to parse XML file " + _f + ": no XML data inside");
		this->errors++;
		this->parseOk = false;
		return;
	}
	doc = xmlParseMemory(xmlData.substr(xmlStart).c_str(), xmlData.substr(xmlStart).size());
	xmlData.clear();
	
    	if (doc == NULL) {
		mDebug("XML Load failed");
		this->errors++;
		this->parseOk = false;
		xmlFreeDoc(doc);
		doc=NULL;
		return;
	}

	curNode = xmlDocGetRootElement(doc);

	if (curNode == NULL) {
		mDebug("Failed to get root node");
		this->errors++;
		this->parseOk = false;
		xmlFreeDoc(doc);
		doc=NULL;
		return;
	} 
    	// checking for valid root node
	if (xmlStrcmp(curNode->name, (const xmlChar *) "package") ) {
		mDebug("Invalid root node definition");
		this->errors++;
		this->parseOk = false;
		xmlFreeDoc(doc);
		doc=NULL;
		return;

	}
	if (this->errors == 0) {
		this->parseOk = true;
		buildDepDef();
		buildSugDef();
	}
}

PackageConfig::PackageConfig(xmlChar * membuf, int bufsize)
{
	depCount = -1;
	suggestCount=-1;
    	this->errors = 0;
	doc = xmlParseMemory((const char *) membuf, bufsize);
	xmlFree(membuf);
	
	if (doc == NULL) {
		mDebug("XML Load failed");
		this->errors++;
		this->parseOk = false;
		xmlFreeDoc(doc);
		doc=NULL;
		return;
	}

	curNode = xmlDocGetRootElement(doc);

	if (curNode == NULL) {
		mDebug("Failed to get root node");
		this->errors++;
		this->parseOk = false;
		xmlFreeDoc(doc);
		doc=NULL;
		return;
	} 

	// checking for valid root node
	if (xmlStrcmp(curNode->name, (const xmlChar *) "package") ) {
		mDebug("Invalid root node definition");
		this->errors++;
		this->parseOk = false;
		xmlFreeDoc(doc);
		doc=NULL;
		return;
	} 

	if (this->errors == 0) {
		this->parseOk = true;
		buildDepDef();
		buildSugDef();
	}
}

/**
 * check if we have any errors during parsing
 * 
 * @return bool state
 */
bool PackageConfig::hasErrors() {
    	if (this->errors == 0) return false;
	else return true;
}

/**
 * WTF???
 * 
 * @param rootnode
 */
PackageConfig::PackageConfig(xmlNodePtr __rootXmlNodePtr)
{
	depCount = -1;
	suggestCount = -1;
	parseOk = true;
	//curNode = __rootXmlNodePtr;
	//curNode = xmlNewNode;
	*curNode=*__rootXmlNodePtr;
	this->doc=NULL;
	if (this->doc == NULL) {
		this->doc = xmlNewDoc((const xmlChar *)"1.0");
		if (this->doc == NULL) {
			parseOk=false;
		}
		xmlDocSetRootElement(this->doc, this->curNode);
	}
	buildDepDef();
	buildSugDef();
}

PackageConfig::~PackageConfig()
{
    	if (this->doc != NULL) {
		xmlFreeDoc(doc);
		doc=NULL;
    	}
	xmlCleanupMemory();
}

/**
 * evaluate XPath expression and return result nodes
 * 
 * @param exp XPath expression
 * 
 * @return xmlXPathObjectPtr node set
 */
xmlXPathObjectPtr PackageConfig::getNodeSet(const xmlChar * exp) 
{
	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;
	
    	context = xmlXPathNewContext(doc);
    	if (context == NULL) 
	{
        	this->errors++;
        	XPATH_CTX_ERR;
        	return NULL;
    	}

    	result = xmlXPathEvalExpression(exp, context);
    	if (result == NULL) 
	{
        	this->errors++;
        	return NULL;
    	} 

	if(xmlXPathNodeSetIsEmpty(result->nodesetval))
	{
        	xmlXPathFreeObject(result);
		xmlXPathFreeContext(context);
        	return NULL;
    	}
       	else 
	{
		xmlXPathFreeContext(context);
		return result;
	}
}
const string& PackageConfig::getValue(string data_xpath) // Returns data from data_xpath
{
	string ret;
	if (dataCache.setValue(data_xpath, &ret)) return dataCache.getValue(data_xpath);;
    	xmlNodeSetPtr nodeset;
    	xmlXPathObjectPtr res;
    	res = getNodeSet((const xmlChar *) data_xpath.c_str());
    	if (res) {
		nodeset = res->nodesetval;
		xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
		const char * _result = (const char * )key;
		std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
		ret = strim(__r);
		dataCache.setValue(data_xpath, ret);
		return dataCache.getValue(data_xpath);
	} else {
		return EMPTY;
	}
}

/**
 * get /package/name from xml
 * 
 * @return string
 */
const string& PackageConfig::getProvides() {
	if (!pProvides.empty()) return pProvides;
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;
	res = getNodeSet(GET_PKG_PROVIDES);
	if (res) {
		nodeset = res->nodesetval;

		xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
		const char * _result = (const char * )key;
		std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
		pProvides = strim(__r);
		return pProvides;
	} 
	else {
		return EMPTY;
	}
}



const string& PackageConfig::getConflicts()
{
	if (!pConflicts.empty()) return pConflicts;
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;
	res = getNodeSet(GET_PKG_CONFLICTS);
	if (res) {
		nodeset = res->nodesetval;

		xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
		const char * _result = (const char * )key;
		std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
		pConflicts = strim(__r);
		return pConflicts;
	} 
	else {
		return EMPTY;
	}
}



const string& PackageConfig::getName()
{
	if (!pName.empty()) return pName;
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;
	res = getNodeSet(GET_PKG_NAME);
	if (res) {
		nodeset = res->nodesetval;

		xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
		const char * _result = (const char * )key;
		std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
		pName = strim(__r);
		return pName;
	} 
	else {
		return EMPTY;
	}
}

/**
 * get /package/version from xml
 * 
 * @return string
 */
const string& PackageConfig::getVersion()
{
	if (!pVersion.empty()) return pVersion;
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;
	res = getNodeSet(GET_PKG_VERSION);
	if (res) {
        
		nodeset = res->nodesetval;
		xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
		const char * _result = (const char * )key;
		std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
		pVersion = strim(__r);
		return pVersion;
	} else {
		return EMPTY;
	}
}
const string& PackageConfig::getBetarelease()
{
	if (!pBetarelease.empty()) return pBetarelease;
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;
	res = getNodeSet(GET_PKG_BETARELEASE);
	if (res) {

		nodeset = res->nodesetval;
		xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
		const char * _result = (const char * )key;
		std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
		pBetarelease = strim(__r);
		return pBetarelease;
	} else {
		return EMPTY;
    }
}
vector<DeltaSource> PackageConfig::getBDeltas() {
	vector<DeltaSource> ret;
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;
	string path;
	string delta_url, delta_md5, delta_size, orig_filename, orig_md5;
	xmlChar *result;
	for (unsigned int i=0; ; ++i) {
		delta_url.clear();
	       	delta_md5.clear();
		orig_filename.clear();
		orig_md5.clear();
		path = "//bdelta[" + IntToStr(i+1) + "]";
		res = getNodeSet((const xmlChar *) path.c_str());
		if (!res) {
			break;
		}
		nodeset = res->nodesetval;
		result = xmlGetProp(nodeset->nodeTab[0], (const xmlChar *)"dup");
		if (result) delta_url = string((const char *) result);
		result = xmlGetProp(nodeset->nodeTab[0], (const xmlChar *)"dup_md5");
		if (result) delta_md5 = string((const char *) result);
		result = xmlGetProp(nodeset->nodeTab[0], (const xmlChar *)"orig");
		if (result) orig_filename = string((const char *) result);
		result = xmlGetProp(nodeset->nodeTab[0], (const xmlChar *)"orig_md5");
		if (result) orig_md5 = string((const char *) result);
		result = xmlGetProp(nodeset->nodeTab[0], (const xmlChar *)"dup_size");
		if (result) delta_size = string((const char *) result);

		if (delta_url.empty() || delta_md5.empty() || orig_filename.empty() || orig_md5.empty() || delta_size.empty()) continue;
		ret.push_back(DeltaSource(delta_url, delta_md5, orig_filename, orig_md5, delta_size));
	}
	return ret;
}
string PackageConfig::getRepositoryTags()
{
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;
	res = getNodeSet(GET_PKG_REP_TAGS);
	if (res) {

		nodeset = res->nodesetval;
		xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
		const char * _result = (const char * )key;
		std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
		return strim(__r);
	} else {
		return EMPTY;
    }
}

string PackageConfig::getBuildConfigureEnvOptions()
{
	xmlNodeSetPtr nodeset;
    	xmlXPathObjectPtr res;
    	res = getNodeSet(GET_PKG_MBUILD_ENVOPTIONS);
    	if (res) {
        
        	nodeset = res->nodesetval;
	        xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
        	const char * _result = (const char * )key;
	        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
	        return strim(__r);
	    } else {
        	return EMPTY;
    	}

}

string PackageConfig::getBuildUrl()
{
	xmlNodeSetPtr nodeset;
    	xmlXPathObjectPtr res;
    	res = getNodeSet(GET_PKG_MBUILD_URL);
    	if (res) {
        
        	nodeset = res->nodesetval;
	        xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
        	const char * _result = (const char * )key;
	        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
	        return strim(__r);
	    } else {
        	return EMPTY;
    	}
}
string PackageConfig::getBuildSourceRoot()
{
	xmlNodeSetPtr nodeset;
    	xmlXPathObjectPtr res;
    	res = getNodeSet(GET_PKG_MBUILD_SOURCEROOT);
    	if (res) {
        
        	nodeset = res->nodesetval;
	        xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
        	const char * _result = (const char * )key;
	        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
	        return strim(__r);
	    } else {
        	return EMPTY;
    	}
}
string PackageConfig::getBuildSystem()
{
	xmlNodeSetPtr nodeset;
    	xmlXPathObjectPtr res;
    	res = getNodeSet(GET_PKG_MBUILD_BUILDSYSTEM);
    	if (res) {
        
        	nodeset = res->nodesetval;
	        xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
        	const char * _result = (const char * )key;
	        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
	        return strim(__r);
	    } else {
        	return EMPTY;
    	}
}
string PackageConfig::getBuildMaxNumjobs()
{
	xmlNodeSetPtr nodeset;
    	xmlXPathObjectPtr res;
    	res = getNodeSet(GET_PKG_MBUILD_MAX_NUMJOBS);
    	if (res) {
        
        	nodeset = res->nodesetval;
	        xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
        	const char * _result = (const char * )key;
	        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
	        return strim(__r);
	    } else {
        	return EMPTY;
    	}
}
bool PackageConfig::getBuildOptimizationCustomizable()
{
	xmlNodeSetPtr nodeset;
    	xmlXPathObjectPtr res;
    	res = getNodeSet(GET_PKG_MBUILD_OPTIMIZATION_ALLOW_CHANGE);
    	if (res) {
        
        	nodeset = res->nodesetval;

		xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode, 1);
        	const char * _result = (const char * )key;
	        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
	        if (strim(__r)=="true") return true;
		else return false;
	    } else {
        	return false;
    	}
}
bool PackageConfig::getBuildNoSubfolder()
{
	xmlNodeSetPtr nodeset;
    	xmlXPathObjectPtr res;
    	res = getNodeSet(GET_PKG_MBUILD_NO_SUBFOLDER);
    	if (res) {
        
        	nodeset = res->nodesetval;

		xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode, 1);
        	const char * _result = (const char * )key;
	        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
	        if (strim(__r)=="true") return true;
		else return false;
	    } else {
        	return false;
    	}
}

bool PackageConfig::getBuildUseCflags()
{
	xmlNodeSetPtr nodeset;
    	xmlXPathObjectPtr res;
    	res = getNodeSet(GET_PKG_MBUILD_USE_CFLAGS);
    	if (res) {
        
        	nodeset = res->nodesetval;

		xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode, 1);
        	const char * _result = (const char * )key;
	        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
	        if (strim(__r)=="false") return false;
		else return true;
	    } else {
        	return true;
    	}
}

string PackageConfig::getBuildOptimizationMarch()
{
	xmlNodeSetPtr nodeset;
    	xmlXPathObjectPtr res;
    	res = getNodeSet(GET_PKG_MBUILD_OPTIMIZATION_MARCH);
    	if (res) {
        
        	nodeset = res->nodesetval;
	        xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
        	const char * _result = (const char * )key;
	        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
	        return strim(__r);
	    } else {
        	return EMPTY;
    	}
}

string PackageConfig::getBuildOptimizationMtune()
{
	xmlNodeSetPtr nodeset;
    	xmlXPathObjectPtr res;
    	res = getNodeSet(GET_PKG_MBUILD_OPTIMIZATION_MTUNE);
    	if (res) {
        
        	nodeset = res->nodesetval;
	        xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
        	const char * _result = (const char * )key;
	        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
	        return strim(__r);
	    } else {
        	return EMPTY;
    	}
}

string PackageConfig::getBuildOptimizationLevel()
{
	xmlNodeSetPtr nodeset;
    	xmlXPathObjectPtr res;
    	res = getNodeSet(GET_PKG_MBUILD_OPTIMIZATION_LEVEL);
    	if (res) {
        
        	nodeset = res->nodesetval;
	        xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
        	const char * _result = (const char * )key;
	        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
	        return strim(__r);
	    } else {
        	return EMPTY;
    	}
}

string PackageConfig::getBuildOptimizationCustomGccOptions()
{
	xmlNodeSetPtr nodeset;
    	xmlXPathObjectPtr res;
    	res = getNodeSet(GET_PKG_MBUILD_OPTIMIZATION_CUSTOM_GCC_OPTIONS);
    	if (res) {
        
        	nodeset = res->nodesetval;
	        xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
        	const char * _result = (const char * )key;
	        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
	        return strim(__r);
	    } else {
        	return EMPTY;
    	}
}

string PackageConfig::getBuildCmdConfigure()
{
	xmlNodeSetPtr nodeset;
    	xmlXPathObjectPtr res;
    	res = getNodeSet(GET_PKG_MBUILD_CMD_CONFIGURE);
    	if (res) {
        
        	nodeset = res->nodesetval;
	        xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
        	const char * _result = (const char * )key;
	        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
	        return strim(__r);
	    } else {
        	return EMPTY;
    	}
}
const string& PackageConfig::getPackageType()
{
	if (!pPackageType.empty()) return pPackageType;
	xmlNodeSetPtr nodeset;
    	xmlXPathObjectPtr res;
    	res = getNodeSet(GET_PKG_TYPE);
    	if (res) {
        
        	nodeset = res->nodesetval;
	        xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
        	const char * _result = (const char * )key;
	        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
		pPackageType = strim(__r);
	        return pPackageType;
	    } else {
        	return EMPTY;
    	}
}

string PackageConfig::getBuildCmdMake()
{
	xmlNodeSetPtr nodeset;
    	xmlXPathObjectPtr res;
    	res = getNodeSet(GET_PKG_MBUILD_CMD_MAKE);
    	if (res) {
        
        	nodeset = res->nodesetval;
	        xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
        	const char * _result = (const char * )key;
	        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
	        return strim(__r);
	    } else {
        	return EMPTY;
    	}
}

string PackageConfig::getBuildCmdMakeInstall()
{
	xmlNodeSetPtr nodeset;
    	xmlXPathObjectPtr res;
    	res = getNodeSet(GET_PKG_MBUILD_CMD_MAKEINSTALL);
    	if (res) {
        
        	nodeset = res->nodesetval;
	        xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
        	const char * _result = (const char * )key;
	        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
	        return strim(__r);
	    } else {
        	return EMPTY;
    	}
}
vector<string> PackageConfig::getBuildPatchList()
{
	vector<string> a;
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;

	res = getNodeSet(GET_PKG_MBUILD_PATCH_LIST);
	if (res) {
		nodeset = res->nodesetval;
		for (int i=0; i<nodeset->nodeNr; ++i) {
			xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode,1);
			const char * _result = (const char * )key;
			std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
			a.push_back(strim(__r));
		}
		return a;
	} 
	return a;
}
StringMap PackageConfig::getBuildAdvancedUrlMap()
{
	StringMap a;
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;

	res = getNodeSet(GET_PKG_MBUILD_ADVANCED_URL_LIST);
	if (res) {

		nodeset = res->nodesetval;
		for (int i=0; i<nodeset->nodeNr; ++i) {
			xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode,1);
			const char * _result = (const char * )key;
			std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
			a.setValue(strim(__r), getValue("//mbuild/source_list/source["+IntToStr(i+1)+"]/@extract_path"));
		}
		return a;
	} 
	return a;
}


vector<string> PackageConfig::getBuildKeyNames()
{
	vector<string> a;
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;

	res = getNodeSet(GET_PKG_MBUILD_CONFIGURATION_KEY_NAME);
	if (res) {
		nodeset = res->nodesetval;
		for (int i=0; i<nodeset->nodeNr; ++i) {
			xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode,1);
			const char * _result = (const char * )key;
			std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
			a.push_back(strim(__r));
		}
		return a;
	} 
	return a;
}

vector<string> PackageConfig::getBuildKeyValues()
{
	vector<string> a;
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;

	res = getNodeSet(GET_PKG_MBUILD_CONFIGURATION_KEY_VALUE);
	if (res) {
		nodeset = res->nodesetval;
		for (int i=0; i<nodeset->nodeNr; ++i) {
			xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode,1);
			const char * _result = (const char * )key;
			std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
			a.push_back(strim(__r));
		}
		return a;
	} 
	return a;
}

string PackageConfig::getPGPSignature()
{
	xmlNodeSetPtr nodeset;
    	xmlXPathObjectPtr res;
    	res = getNodeSet(GET_PKG_PGP_SIGNATURE);
    	if (res) {
        
        	nodeset = res->nodesetval;
	        xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
        	const char * _result = (const char * )key;
	        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
	        return strim(__r);
	    } else {
        	return EMPTY;
    	}
}


/**
 * return /package/arch
 * 
 * @return string
 */
const string& PackageConfig::getArch()
{
	if (!pArch.empty()) return pArch;
	xmlNodeSetPtr nodeset;
    xmlXPathObjectPtr res;
    res = getNodeSet(GET_PKG_ARCH);
    if (res) {
        
        nodeset = res->nodesetval;
        xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
        const char * _result = (const char * )key;
        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;

        pArch = strim(__r);
	return pArch;
    } else {
        return EMPTY;
    }
}

/**
 * return /package/build
 * 
 * @return string
 */
const string& PackageConfig::getBuild()
{
	if (!pBuild.empty()) return pBuild;
	xmlNodeSetPtr nodeset;
    xmlXPathObjectPtr res;
    res = getNodeSet(GET_PKG_BUILD);
    if (res) {
        
        nodeset = res->nodesetval;
        xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
        const char * _result = (const char * )key;
        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
        //return (std::string)_result;
	pBuild = strim(__r);
	return pBuild;
	//	return "1";
    } else {
        return EMPTY;
    }
}

/**
 * return /package/maintainer/name
 * @return string
 */
const string& PackageConfig::getAuthorName()
{
	if (!pAuthorName.empty()) return pAuthorName;
	xmlNodeSetPtr nodeset;
    xmlXPathObjectPtr res;
    res = getNodeSet(GET_PKG_MAINT_NAME);
    if (res) {
        
        nodeset = res->nodesetval;
        xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
        const char * _result = (const char * )key;
        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
        pAuthorName = strim(__r);
	return pAuthorName;
    } else {
        return EMPTY;
    }
}

/**
 * return /package/maintainer/email
 * 
 * @return string
 */
const string& PackageConfig::getAuthorEmail()
{
	if (!pAuthorEmail.empty()) return pAuthorEmail;
	xmlNodeSetPtr nodeset;
    xmlXPathObjectPtr res;
    res = getNodeSet(GET_PKG_MAINT_EMAIL);
    if (res) {
        
        nodeset = res->nodesetval;
        xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
        const char * _result = (const char * )key;
        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
        pAuthorEmail = strim(__r);
	return pAuthorEmail;
    } else {
        return EMPTY;
    }
}

/**
 * return /package/changelog
 * 
 * @return string
 */
const string& PackageConfig::getChangelog()
{
	if (!pChangelog.empty()) return pChangelog;
    xmlNodeSetPtr nodeset;
    xmlXPathObjectPtr res;
    res = getNodeSet(GET_PKG_CHANGELOG);
    if (res != NULL) {
        nodeset = res->nodesetval;

		xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
		
        const char * _result = (const char * )key;
        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
        pChangelog = strim(__r);
	return pChangelog;
    } else {
        return EMPTY;
    }
}

/**
 * return description
 * really return only 'en' description by
 * /package/description[@lang='en']
 * 
 * @param lang description language
 * 
 * @return string
 */
const string& PackageConfig::getDescription(string lang)
{
	if (!pDescription.empty()) return pDescription;
    if (!lang.empty()) say("warning: languaged descriptions disabled\n");
    xmlNodeSetPtr nodeset;
    xmlXPathObjectPtr res;
    res = getNodeSet(GET_PKG_DESCRIPTION);
    if (res) {
        nodeset = res->nodesetval;
        xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
        const char * _result = (const char * )key;
        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
        pDescription = strim(__r);
	return pDescription;
    } else {
        return EMPTY;
    }
}

/**
 * return short description
 * really return only 'en' description by
 * /package/short_description[@lang='en']
 * 
 * @param lang description language
 * 
 * @return string
 */
const string& PackageConfig::getShortDescription(string lang)
{
	if (!pShortDescription.empty()) return pShortDescription;
	if (!lang.empty()) say("warning: languaged descriptions disabled\n");

	xmlNodeSetPtr nodeset;
    xmlXPathObjectPtr res;
    res = getNodeSet(GET_PKG_SHORT_DESCRIPTION);
    if (res) {
        
        nodeset = res->nodesetval;
        xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
        const char * _result = (const char * )key;
        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
        pShortDescription = strim(__r);
	return pShortDescription;
    } else {
        return EMPTY;
    }
}

void PackageConfig::buildSugDef()
{
	suggestTreeDef.clear();
	xmlXPathObjectPtr res;
	string path;
	suggestCount=0;
	for (size_t i=1; ; ++i) {
		path = "//package/suggests/suggest[" + IntToStr(i)+"]";
		res = getNodeSet((const xmlChar *) path.c_str());
		if (res) {
			suggestCount++;
		}
		else break;
	}
	suggestTreeDef.resize(suggestCount);
	for (size_t i=1; i<=suggestCount; ++i) {
		path = "//suggests/suggest[" + IntToStr(i) + "]/name[1]";
		res = getNodeSet((const xmlChar *) path.c_str());
		if (res) {
			suggestTreeDef[i-1].name=true;
//			printf("found name in %d suggest\n", i);
		}
		else {
//			printf("NOT FOUND name in %d suggest\n", i);
			suggestTreeDef[i-1].name=false;
		}
	}
	for (size_t i=1; i<=suggestCount; ++i) {
		path = "//suggests/suggest[" + IntToStr(i) + "]/condition[1]";
		res = getNodeSet((const xmlChar *) path.c_str());
		if (res) suggestTreeDef[i-1].condition=true;
		else suggestTreeDef[i-1].condition=false;
	}
	for (size_t i=1; i<=suggestCount; ++i) {
		path = "//suggests/suggest[" + IntToStr(i) + "]/version[1]";
		res = getNodeSet((const xmlChar *) path.c_str());
		if (res) suggestTreeDef[i-1].version=true;
		else suggestTreeDef[i-1].version=false;
	}
	for (size_t i=1; i<=suggestCount; ++i) {
		path = "//suggests/suggest[" + IntToStr(i) + "]/build_only[1]";
		res = getNodeSet((const xmlChar *) path.c_str());
		if (res) suggestTreeDef[i-1].build_only=true;
		else suggestTreeDef[i-1].build_only=false;
	}
}



void PackageConfig::buildDepDef()
{
	dependencyTreeDef.clear();
	xmlXPathObjectPtr res;
	string path;
	depCount=0;
	for (size_t i=1; ; ++i) {
		path = "//package/dependencies/dep[" + IntToStr(i)+"]";
		res = getNodeSet((const xmlChar *) path.c_str());
		if (res) {
			depCount++;
		}
		else break;
	}
	dependencyTreeDef.resize(depCount);
	for (size_t i=1; i<=depCount; ++i) {
		path = "//dependencies/dep[" + IntToStr(i) + "]/name[1]";
		res = getNodeSet((const xmlChar *) path.c_str());
		if (res) {
			dependencyTreeDef[i-1].name=true;
		}
		else {
			dependencyTreeDef[i-1].name=false;
		}
	}
	for (size_t i=1; i<=depCount; ++i) {
		path = "//dependencies/dep[" + IntToStr(i) + "]/condition[1]";
		res = getNodeSet((const xmlChar *) path.c_str());
		if (res) dependencyTreeDef[i-1].condition=true;
		else dependencyTreeDef[i-1].condition=false;
	}
	for (size_t i=1; i<=depCount; ++i) {
		path = "//dependencies/dep[" + IntToStr(i) + "]/version[1]";
		res = getNodeSet((const xmlChar *) path.c_str());
		if (res) dependencyTreeDef[i-1].version=true;
		else dependencyTreeDef[i-1].version=false;
	}
	for (size_t i=1; i<=depCount; ++i) {
		path = "//dependencies/dep[" + IntToStr(i) + "]/build_only[1]";
		res = getNodeSet((const xmlChar *) path.c_str());
		if (res) dependencyTreeDef[i-1].build_only=true;
		else dependencyTreeDef[i-1].build_only=false;
	}
	for (size_t i=0; i<dependencyTreeDef.size(); ++i)
	{
		//printf("[%d] %d %d %d %d\n", i, dependencyTreeDef[i].name, dependencyTreeDef[i].condition, dependencyTreeDef[i].version, dependencyTreeDef[i].build_only);
	}


}
vector<bool> PackageConfig::getDepBuildOnlyFlags()
{
	vector<bool> a;
	xmlNodeSetPtr nodeset;
    	xmlXPathObjectPtr res;
    	res = getNodeSet(GET_PKG_DEP_BUILDONLY);
    	if (res) {
        
        	nodeset = res->nodesetval;
		for (size_t i=0; i<depCount; ++i) {
			if (!dependencyTreeDef[i].build_only) {
				a.push_back(false); 
				continue;
			}

			xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode, 1);

	        	const char * _result = (const char * )key;
		        std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
		        if (strim(__r)=="true") a.push_back(true);
			else a.push_back(false);
	    	}
        	return a;
    	}
	else {
		for (size_t i=0; i<depCount; ++i) {
			a.push_back(false);
		}
	}
	return a;
}

vector<string> PackageConfig::getDepNames()
{
	vector<string> a;
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;

	res = getNodeSet(GET_PKG_DEP_NAME);
	if (res) {

		nodeset = res->nodesetval;
		for (size_t i=0; i<depCount; ++i) {
			if (!dependencyTreeDef[i].name) {
				a.push_back(""); 
				continue;
			}
			xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode,1);
			const char * _result = (const char * )key;
			std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
			a.push_back(strim(__r));
		}
		return a;
	}
	else {
		for (size_t i=0; i<depCount; ++i) {
			a.push_back("");
		}
	}
	
	return a;
}
vector<string> PackageConfig::getDepConditions()
{
	vector<string> a;
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;

	res = getNodeSet(GET_PKG_DEP_COND);
	if (res) {

		nodeset = res->nodesetval;
		for (size_t i=0; i<depCount; ++i) {
			if (!dependencyTreeDef[i].condition) {
				a.push_back(""); 
				continue;
			}

			xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode,1);
			const char * _result = (const char * )key;
			std::string __r = (_result != NULL) ? ((std::string)_result) : "OMGWTF!!!!";
			a.push_back(strim(__r));
		}
		return a;
	}
	else {
		for (size_t i=0; i<depCount; ++i) {
			a.push_back("");
		}
	}

	return a;
}

vector<string> PackageConfig::getDepVersions()
{
	vector<string> a;
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;

	res = getNodeSet(GET_PKG_DEP_VERSION);
	if (res) {

		nodeset = res->nodesetval;
		for (size_t i=0; i<depCount; ++i) {
			if (!dependencyTreeDef[i].version) {
				a.push_back(""); 
				continue;
			}

			xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode,1);
			const char * _result = (const char * )key;
			std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
			a.push_back(strim(__r));
		}
		return a;
	} 
	else {
		for (size_t i=0; i<depCount; ++i)	{
			a.push_back("");
		}
	}

	return a;
}

vector<string> PackageConfig::getSuggestNames()
{
	vector<string> a;
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;

	res = getNodeSet(GET_PKG_SUG_NAME);
	if (res) {
		nodeset = res->nodesetval;
		for (size_t i=0; i<suggestCount; ++i) {
			if (!suggestTreeDef[i].name) {
				a.push_back("");
				continue;
			}
			xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode,1);
			const char * _result = (const char * )key;
			std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
			a.push_back(strim(__r));
		}
		return a;
	}
	else {
		a.resize(suggestCount);
		return a;
	}
}
vector<string> PackageConfig::getSuggestConditions()
{
	vector<string> a;
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;

	res = getNodeSet(GET_PKG_SUG_COND);
	if (res) {

		nodeset = res->nodesetval;
		for (size_t i=0; i<suggestCount; ++i) {
			if (!suggestTreeDef[i].condition) {
				a.push_back("");
				continue;
			}

			xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode,1);
			const char * _result = (const char * )key;
			std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
			a.push_back(strim(__r));
		}
		return a;
	}
	else {
		a.resize(suggestCount);
		return a;
	}
}

vector<string> PackageConfig::getSuggestVersions()
{
	vector<string> a;
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;

	res = getNodeSet(GET_PKG_SUG_VERSION);
	if (res) {
		nodeset = res->nodesetval;
		for (size_t i=0; i<suggestCount; ++i) {
			if (!suggestTreeDef[i].version) {
				a.push_back("");
				continue;
			}

			xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode,1);
			const char * _result = (const char * )key;
			std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
			a.push_back(strim(__r));
		}
		return a;
	}
	else {
		a.resize(suggestCount);
	}
	return a;
}

/**
 * return tags /package/tags/tag
 * 
 * @return vector<string>
 */
vector<string> PackageConfig::getTags()
{
	vector<string> a;
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;

	res = getNodeSet(GET_PKG_TAGS);
	if (res) {
		nodeset = res->nodesetval;
		for (int i=0; i<nodeset->nodeNr; ++i) {
			xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode,1);
			const char * _result = (const char * )key;
			std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
			a.push_back(strim(__r));
		}
	} 
	return a;
}

vector<string> PackageConfig::getFilelist()
{
	vector<string> a;

	return a;
}

vector<string> PackageConfig::getConfigFilelist()
{
	// SEEMS NOT TO WORK!!!!!!!!!!!!!!!!!
	vector<string> a;

	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;

	res = getNodeSet(GET_PKG_CONFIG_FILE_LIST);
	if (res) {
		nodeset = res->nodesetval;
		for (int i=0; i<nodeset->nodeNr; ++i) {
			xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode,1);
			const char * _result = (const char * )key;
			std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
			a.push_back(strim(__r));
		}
		return a;
	}


	return a;
}
vector<string> PackageConfig::getTempFilelist() {
	vector<string> a;

	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;

	res = getNodeSet(GET_PKG_TEMP_FILE_LIST);
	if (res) {
		nodeset = res->nodesetval;
		for (int i=0; i<nodeset->nodeNr; ++i) {
			xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode,1);
			const char * _result = (const char * )key;
			std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
			a.push_back(strim(__r));
		}
	}
	return a;
}

xmlNodePtr PackageConfig::getXMLNode() {
	return this->curNode;
}

xmlChar * PackageConfig::getXMLNodeXPtr(int *bufsize) {
	xmlChar *membuf;
	xmlDocDumpMemory(this->doc, &membuf, bufsize);
	return membuf;
}
	
xmlDocPtr PackageConfig::getXMLDoc() {
	return this->doc;
}

/**
 * return /package/md5
 * 
 * @return string
 */
string PackageConfig::getMd5() {
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;
	res = getNodeSet(GET_PKG_MD5);
	if (res) {
		nodeset = res->nodesetval;
		xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
		const char * _result = (const char * )key;
		std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
		return strim(__r);
	}
	else {
		return EMPTY;
	}
}

string PackageConfig::getCompressedSize() {
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;
	res = getNodeSet(GET_PKG_COMP_SIZE);
	if (res) {
		nodeset = res->nodesetval;
		xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
		const char * _result = (const char * )key;
		std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
		return strim(__r);
	}
	else {
		return EMPTY;
	}
}

string PackageConfig::getInstalledSize() {
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;
	res = getNodeSet(GET_PKG_INST_SIZE);
	if (res) {
		nodeset = res->nodesetval;
		xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
		const char * _result = (const char * )key;
		std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
		return strim(__r);
	} else {
		return EMPTY;
	}
}

string PackageConfig::getFilename() {
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;
	res = getNodeSet(GET_PKG_FILENAME);
	if (res) {
		nodeset = res->nodesetval;
		xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
		const char * _result = (const char * )key;
		std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
		return strim(__r);
	} else {
		return EMPTY;
	}
}

string PackageConfig::getLocation() {
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr res;
	res = getNodeSet(GET_PKG_LOCATION);
	if (res) {
		nodeset = res->nodesetval;
		xmlChar * key = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode,1);
		const char * _result = (const char * )key;
		std::string __r = (_result != NULL) ? ((std::string)_result) : EMPTY;
		return strim(__r);
	} else {
		return EMPTY;
	}
}

