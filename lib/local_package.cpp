/*
Local package installation functions

$Id: local_package.cpp,v 1.77 2007/11/21 14:39:26 i27249 Exp $
*/

#include "local_package.h"
#include "mpkg.h"
#include "xml2pkglist.h"
 
int xml2package(xmlDocPtr doc, xmlNodePtr cur, PACKAGE *data) {
	return xml2pkg(doc, cur, *data);
}


int xml2spkg(xmlNodePtr pkgNode, SPKG *data)
{

	//mDebug("reading package node");
#ifdef XPTR_MODE // Init using xmlNodePtr
	PackageConfig p(pkgNode);
#else		// Init using string dump

	xmlDocPtr doc = xmlNewDoc((const xmlChar *) "1.0");
	xmlDocSetRootElement(doc,pkgNode);
/*	FILE *__dump = fopen(TEMP_XML_DOC,"w");
	xmlDocDump(__dump, doc);
	fclose(__dump);
*/
	xmlChar * membuff;
	int bufsize;
	xmlDocDumpMemory(doc, &membuff, &bufsize);
	//xmlFreeDoc(doc);
	//PackageConfig p(TEMP_XML_DOC);
	PackageConfig p(membuff, bufsize);
#endif
	if (!p.parseOk) return -100;

	
	// Now, import spkg-related things
	data->buildsystem = p.getBuildSystem();
	data->url = p.getBuildUrl();
	data->configure_keys = p.getBuildKeyNames();
	data->configure_values = p.getBuildKeyValues();
	data->patches = p.getBuildPatchList();
	data->source_root = p.getBuildSourceRoot();
	data->max_numjobs = p.getBuildMaxNumjobs();
	data->allow_change_cflags = p.getBuildOptimizationCustomizable();
	data->march = p.getBuildOptimizationMarch();
	data->mtune = p.getBuildOptimizationMtune();
	data->olevel = p.getBuildOptimizationLevel();
	data->custom_gcc_options = p.getBuildOptimizationCustomGccOptions();
	data->use_cflags = p.getBuildUseCflags();
	data->custom_configure = p.getBuildCmdConfigure();
	data->custom_make = p.getBuildCmdMake();
	data->custom_makeinstall = p.getBuildCmdMakeInstall();
	data->no_subfolder = p.getBuildNoSubfolder();
	data->env_options = p.getBuildConfigureEnvOptions();
	data->nostrip = p.getValue((const char *) GET_PKG_MBUILD_NOSTRIP)=="true";

	xml2package(p.getXMLDoc(), p.getXMLNode(), &data->pkg);
	return 0;
}

// In future, this function should replace all instances of code that attempts to split slackware filename to parts. This code was got from convert_package function, as it seems to be the most correct one.
bool splitSlackwareFilename(string filename, string *name, string *version, string *arch, string *build) {
	mDebug("Preparing to convert " + filename);
	string tmp;
	string ptrstr;
	// Guessing type
	string pkgType = getExtension(filename);
	if (pkgType!="tgz" && pkgType!="txz" && pkgType!="tlz" && pkgType!="tbz") {
		mError(_("Unknown package type ") + pkgType);
		return false;
	}
	// Name-ver-arch-build parsing
	string name_tmp=filename.substr(0,filename.find("." + pkgType));
	name_tmp = name_tmp.substr(name_tmp.find_last_of("/")+1);
	ptrstr = name_tmp.substr(name_tmp.find_last_of("-")+1);
	*build=ptrstr;
	name_tmp = name_tmp.substr(0,name_tmp.find_last_of("-"));
	ptrstr = name_tmp.substr(name_tmp.find_last_of("-")+1);
	*arch = ptrstr;
	name_tmp = name_tmp.substr(0,name_tmp.find_last_of("-"));
	ptrstr=name_tmp.substr(name_tmp.find_last_of("-")+1);
	*version = ptrstr;
	name_tmp = name_tmp.substr(0,name_tmp.find_last_of("-"));
	*name = name_tmp;
	return true;
}

int slack2xml(string filename, string xml_output)
{
	string slackDescFile = get_tmp_file();
	//string slackRequiredFile = get_tmp_file();
	//string slackSuggestsFile = get_tmp_file();
	extractFromTgz(filename, "install/slack-desc", slackDescFile);
	string description=ReadFile(slackDescFile);
	unlink(slackDescFile.c_str());
	//extractFromTgz(filename, "install/slack-required", slackRequiredFile);
	//extractFromTgz(filename, "install/slack-suggest", slackSuggestsFile);
	XMLNode _node = XMLNode::createXMLTopNode("package");
	_node.addChild("name");
	string pkgName;
	_node.addChild("version");
	_node.addChild("arch");
	_node.addChild("build");
	_node.addChild("short_description");
	_node.addChild("description");
	_node.addChild("dependencies");
	_node.addChild("suggests");
	_node.addChild("filename");
	int pos=0;
	int name_start=0;
	string tmp;
	for (int i=filename.length()-1; filename[i]!='/' && i>=0; i--)
	{
		name_start=i;
	}
	for (unsigned int i=name_start; i<filename.length()-1; i++)
	{
		if (filename[i]=='-')
		{
			if (filename[i+1]=='0' || \
				filename[i+1] == '1' || \
				filename[i+1] == '2' || \
				filename[i+1] == '3' || \
				filename[i+1] == '4' || \
				filename[i+1] == '5' || \
				filename[i+1] == '6' || \
				filename[i+1] == '7' || \
				filename[i+1] == '8' || \
				filename[i+1] == '9')
			{
				pkgName = tmp;
				_node.getChildNode("name").addText(tmp.c_str());
				pos=i+2;
				break;
			}
		}
		tmp+=filename[i];
	}
	tmp.clear();
	//VERSION
	for (unsigned int i=pos-1; i< filename.length(); i++)
	{
		if (filename[i]=='-')
		{
			_node.getChildNode("version").addText(tmp.c_str());
			pos=i+2;
			break;
		}
		tmp+=filename[i];
	}
	tmp.clear();
	//ARCH
	for (unsigned int i=pos-1; i< filename.length(); i++)
	{
		if (filename[i]=='-')
		{
			_node.getChildNode("arch").addText(tmp.c_str());
			pos=i+2;
			break;
		}
		tmp+=filename[i];
	}
	tmp.clear();
	//BUILD
	for (unsigned int i=pos-1; i<filename.length()-4; i++)
	{
		tmp+=filename[i];
	}
	_node.getChildNode("build").addText(tmp.c_str());
	tmp.clear();
	// Step 1. Skip comments
	unsigned int dpos=0;
	//unsigned int strLen=0;
	string comment;
	string head;
	string short_description;
	//bool str1=true;
	if (!description.empty())
	{
		// Cleaning out comments
		for (unsigned int i=0; i<description.length(); i++)
		{
			if (description[i]=='#')
			{
				while (description[i]!='\n') i++;
			}
			if (i<description.length()) tmp+=description[i];
		}
		description=tmp;
		tmp.clear();
		string pHead=pkgName+":";
		int spos=description.find(pHead,0);
		// Removing package: headers
		for (unsigned int i=spos; i<description.length(); i++)
		{
			//head+=description[i];
			if (i==0 || description[i-1]=='\n')
			{
				i=i+pkgName.length()+1;
				//if (description[i-1]=='\n') i=i+package.get_name().length()+2;
				//head.clear();
			}
			if (i<description.length()) tmp+=description[i];
		}
		description=tmp;
		tmp.clear();
		// Splitting short and long descriptions
		for (unsigned int i=0; i<description.length() && description[i]!='\n'; i++)
		{
			tmp+=description[i];
			dpos=i+1;
		}
		short_description=tmp;
		tmp.clear();
		for (unsigned int i=dpos; i<description.length(); i++)
		{
			if (i==dpos && description[i]=='\n')
			{
				while (description[i]=='\n' || description[i]==' ') i++;
				if (i>=description.length()) break;
			}
			if (i<description.length()) tmp+=description[i];
		}
		description=tmp;
		tmp.clear();
		mDebug("Description: " + description);
	}
	_node.getChildNode("short_description").addText(short_description.c_str());
	_node.getChildNode("description").addText(description.c_str());
	// Get maintainer name and email
	string mName = mConfig.getValue("maintainer_name");
	string mEmail = mConfig.getValue("maintainer_email");
	if (mName.empty()) mName = "Converted by anonymous maintainer";
	if (mEmail.empty()) mEmail = "No email specified";
	_node.addChild("maintainer");
	_node.getChildNode("maintainer").addChild("name");
	_node.getChildNode("maintainer").getChildNode("name").addText(mName.c_str());
	_node.getChildNode("maintainer").addChild("email");
	_node.getChildNode("maintainer").getChildNode("email").addText(mEmail.c_str());


	_node.writeToFile(xml_output.c_str());
	//delete_tmp_files();
	return 0;
}



LocalPackage::LocalPackage(string _f, string _path_shift) {
	path_shift = _path_shift;
	internal=false;
	this->filename=_f;
	
	// Reset pointers
	__doc = NULL;
	_packageXMLNode = NULL;
	_packageFListNode = NULL;
	
}

LocalPackage::~LocalPackage(){}

string LocalPackage::files2xml(string input)
{
	mstring output;
	output="<?xml version=\"1.0\" encoding=\"utf-8\"?><package><filelist><file>";
	for (unsigned int i=0; i<input.length(); i++)
	{
		if (input[i]=='\n') 
		{
			output+="</file>\n";
			if (i<input.length()-1) output+="<file>";
		}
		else output+=input[i];
	}
	output+="</filelist></package>";
	return output.s_str();
}

int LocalPackage::fill_scripts(PACKAGE *package)
{
	string scripts_dir=SCRIPTS_DIR+"/" + package->get_filename() + "_" + package->get_md5() + "/";
	string tmp_preinstall=scripts_dir+"preinstall.sh";
	string tmp_postinstall=scripts_dir+"doinst.sh";
	string tmp_preremove=scripts_dir+"preremove.sh";
	string tmp_postremove=scripts_dir+"postremove.sh";
	string mkdir_pkg="mkdir -p "+scripts_dir+" 2>/dev/null";
	if (!simulate) system(mkdir_pkg);
	string sys_cache=SYS_CACHE;
	string filename=sys_cache + package->get_filename();
	if (!simulate)
	{
		//mDebug("extracting scripts");
		extractFiles(filename, "install/preinstall.sh install/doinst.sh install/preremove.sh install/postremove.sh", scripts_dir);
		if (FileExists(scripts_dir + "/install")) {
			system("(cd " + scripts_dir + "/install; mv * ..; cd ..; rmdir install)");
		}
	}

	return 0;
}
bool LocalPackage::isNative() {
	string tmp_xml = get_tmp_file();
	extractFromTgz(filename, "install/data.xml", tmp_xml);
	if (!FileNotEmpty(tmp_xml)) {
		unlink(tmp_xml.c_str());
		return false;
	}
	unlink(tmp_xml.c_str());
	return true;
}
int LocalPackage::get_xml()
{
	legacyPackage=false;
	_parseOk=false;
	//mDebug("get_xml start");
	string tmp_xml=get_tmp_file();
	//mDebug("Extracting XML");
	extractFromTgz(filename, "install/data.xml", tmp_xml);

	if (!FileNotEmpty(tmp_xml))
	{
		//mDebug("XML empty, parsing as legacy");
		say(_("%s: No XML, converting from legacy\n"), filename.c_str());
		legacyPackage=true;
		
		FILE *legacy = fopen("/var/log/mpkg-legacy.log", "a");
		if (legacy)
		{
			fprintf(legacy, "%s\n", filename.c_str());
			fclose(legacy);
		}
		// In most cases it means that we have legacy Slackware package.
		// Trying to convert:
		//mDebug("Converting from legacy to XML");
		if (slack2xml(filename, tmp_xml) != 0)
		{
			mError("Infernally invalid package! Cannot work with it at all");
			unlink(tmp_xml.c_str());
			delete_tmp_files();
			return -1;
		}
		//mDebug("Converted");
	}
	//mDebug("Parsing XML");
	PackageConfig p(tmp_xml);
	unlink(tmp_xml.c_str());
	if (!p.parseOk)
	{
		//mDebug("Error parsing XML");
		delete_tmp_files();
		return -100;
	}

	int bufsize;
	xmlChar * membuff = p.getXMLNodeXPtr(&bufsize);
	__doc = xmlParseMemory((const char *) membuff, bufsize);
	xmlFree(membuff);
	_packageXMLNode = xmlDocGetRootElement(__doc);

	xml2package(p.getXMLDoc(), p.getXMLNode(), &data);
	//mDebug("get_xml end");
	delete_tmp_files();
	_parseOk=true;
	return 0;
}

int LocalPackage::fill_filelist(PACKAGE *package, bool)
{
	if (!package) package=&data;
	//mDebug("fill_filelist start");
	// Retrieving regular files
	// For setup mode, we can try to enable cached filelists
	vector<string> file_names;
	string fname;
	bool fname_temp = false;
	if (setupMode && FileExists(getAbsolutePath(getDirectory(filename))+"/.fcache/" + getFilename(filename) + "/flist")) {
		fname = getAbsolutePath(getDirectory(filename))+"/.fcache/" + getFilename(filename) + "/flist";
	}
	else {
		fname = get_tmp_file();
		fname_temp = true;
		system("tar tf "+filename+" --exclude install " +" > "+fname + " 2>/dev/null");
	}
	// Parsing file list
	file_names=ReadFileStrings(fname);
	if (fname_temp) unlink(fname.c_str());
	if (file_names.size()>2) file_names.erase(file_names.begin(), file_names.begin()+2);
	else {
		mWarning("Empty file list in package " + package->get_name());
		file_names.clear();
	}
	package->set_files(file_names);
	// Retrieving symlinks (from doinst.sh).
	string dt;
	bool dt_temp=false;
	// Extracting file to the temp dir
	if (setupMode && FileExists(getAbsolutePath(getDirectory(filename))+"/.fcache/" + getFilename(filename) + "/doinst.sh")) {
		dt = getAbsolutePath(getDirectory(filename))+"/.fcache/" + getFilename(filename) + "/doinst.sh";
	}
	else if (!setupMode || !FileExists(getAbsolutePath(getDirectory(filename))+"/.fcache/" + getFilename(filename) + "/flist")) { // Assuming that doinst.sh isn't present if flist is cached
		dt = get_tmp_file();
		extractFromTgz(filename, "install/doinst.sh", dt);
		dt_temp = true;
	}

	
	if (!dt.empty() && FileExists(dt)) {
		string lnfname=get_tmp_file();
		string sed_cmd = "sed -n 's,^( *cd \\([^ ;][^ ;]*\\) *; *rm -rf \\([^ )][^ )]*\\) *) *$,\\1/\\2,p' < ";
		sed_cmd += dt + " > " + lnfname;
		system(sed_cmd);
		vector<string>link_names=ReadFileStrings(lnfname);
		for (size_t i=0; i<link_names.size(); ++i) {
			if (!link_names[i].empty()) package->get_files_ptr()->push_back(link_names[i]);
		}
		if (dt_temp) unlink(dt.c_str());
		unlink(lnfname.c_str());
	}

	delete_tmp_files();
	return 0;
}


int LocalPackage::get_filelist(bool index)
{
	return fill_filelist(&data, index);
}
	
int LocalPackage::create_md5() {
	string sys="md5sum "+filename;
	string md5str=psystem(sys);
	if (md5str.empty()) {
		mError("Unable to get MD5 from " + filename);
		return 1;
	}
	md5str = md5str.substr(0,32);
	data.set_md5(md5str);
	xmlNewChild(_packageXMLNode, NULL, (const xmlChar *)"md5", (const xmlChar *)md5str.c_str());
	return 0;
}

int LocalPackage::create_signature(string) {
	string tmp_sig=get_tmp_file();

	system("gpg -ba -o " + tmp_sig + " " + filename);
	string pgpstr=ReadFile(tmp_sig);	
	unlink(tmp_sig.c_str());
	if (pgpstr.empty())
	{
		mError("Unable to read signature temp file");
		return 1;
	}
	data.pgp_signature = pgpstr;
	xmlNewChild(_packageXMLNode, NULL, (const xmlChar *)"signature", (const xmlChar *)pgpstr.c_str());
	return 0;
}

void LocalPackage::set_repository_branch(const string &branch) {
	data.set_repository_tags(branch);
	xmlNewTextChild(_packageXMLNode, NULL, (const xmlChar *)"repository_tags", (const xmlChar *)branch.c_str());
	return;
}

void LocalPackage::set_distro_version(const string &distro) {
	data.package_distro_version = distro;

	xmlNewTextChild(_packageXMLNode, NULL, (const xmlChar *)"distro_version", (const xmlChar *)distro.c_str());
	return;
}
void setXmlDeltaSources(const vector<DeltaSource>& deltasources, xmlNodePtr _packageXMLNode) {
	xmlNodePtr __node;
	for (size_t i=0; i<deltasources.size(); ++i) {
		__node = xmlNewChild(_packageXMLNode, NULL, (const xmlChar *)"bdelta", NULL);
		xmlNewProp(__node, (const xmlChar *) "orig", (const xmlChar *) deltasources[i].orig_filename.c_str());
		xmlNewProp(__node, (const xmlChar *) "orig_md5", (const xmlChar *) deltasources[i].orig_md5.c_str());
		xmlNewProp(__node, (const xmlChar *) "dup", (const xmlChar *) deltasources[i].dup_url.c_str());
		xmlNewProp(__node, (const xmlChar *) "dup_md5", (const xmlChar *) deltasources[i].dup_md5.c_str());
		xmlNewProp(__node, (const xmlChar *) "dup_size", (const xmlChar *) deltasources[i].dup_size.c_str());
	}


}
void LocalPackage::set_deltasources(const vector<DeltaSource>& deltasources) {
	data.deltaSources=deltasources;
	setXmlDeltaSources(deltasources, _packageXMLNode);

}

int LocalPackage::get_size()
{
	string csize, isize;
	struct stat fstat;
	stat(filename.c_str(), &fstat);
	isize = getExtractedSize(filename);
	data.set_installed_size(isize);
	csize = IntToStr(fstat.st_size);
	data.set_compressed_size(csize);

	xmlNewTextChild(_packageXMLNode, NULL, (const xmlChar *)"compressed_size", (const xmlChar *)csize.c_str());
	xmlNewTextChild(_packageXMLNode, NULL, (const xmlChar *)"installed_size", (const xmlChar *)isize.c_str());
	return 0;
}
	
int LocalPackage::set_additional_data()
{
	LOCATION location;
	location.set_local();
	string pwd = get_current_dir_name();
	string fpath;
	string fname;
	int fname_start=0;
	// OMG@@@!!!
	for(int i=data.get_filename().length()-1;i>=0 && data.get_filename().at(i)!='/'; i--)
	{
		fname_start=i;
	}
	if (fname_start!=1)
	{

		for (int i=0;i<fname_start;i++)
		{
			fpath+=data.get_filename().at(i);
		}
		for (unsigned int i=fname_start;i<data.get_filename().length();i++)
		{
			fname+=data.get_filename().at(i);
		}
	}
	//mDebug("filename: "+fname);
	string ffname;
	if (!path_shift.empty()) {
		fpath = "./" + fpath.substr(path_shift.size());
	}
	if (fpath[0]!='/' && path_shift.empty()) {
		ffname=pwd + "/" + path_shift;
		ffname+="/";
		ffname+=fpath;
	}
	else ffname=fpath;
	
	//mDebug("file path: "+ffname);
	data.set_filename(fname);
	location.set_server_url(string("local://"));
	location.set_path(ffname);
	data.get_locations_ptr()->push_back(location);
	xmlNewTextChild(_packageXMLNode, NULL, (const xmlChar *)"filename", (const xmlChar *)data.get_filename().c_str());
	xmlNewTextChild(_packageXMLNode, NULL, (const xmlChar *)"location", (const xmlChar *)fpath.c_str());
	return 0;
}

int LocalPackage::injectFile()
{
	if (!dialogMode && verbose) say(string(_("Injecting file") + filename + "\n").c_str());

	internal=true;
	// Injecting data from file!
	// If any of functions fails (e.g. return!=0) - break process and return failure code (!=0);
	//int ret=0;
	//mDebug("local_package.cpp: injectFile(): start");
	//mDebug("get_xml");
	if (get_xml()!=0)
	{
		//mDebug("local_package.cpp: injectFile(): get_xml FAILED");
		return -3;
	}
	if (getExtension(filename)=="spkg") {
		//printf("Source package detected, adding to XML\n");
		xmlNewTextChild(_packageXMLNode, NULL, (const xmlChar *)"type", (const xmlChar *)"source");
		data.set_type(PKGTYPE_SOURCE);
	}
	if (getExtension(filename)=="tgz" || getExtension(filename) == "txz" || getExtension(filename) == "tlz" ||getExtension(filename) == "tbz" ) {
		xmlNewTextChild(_packageXMLNode, NULL, (const xmlChar *)"type", (const xmlChar *)"binary");
		data.set_type(PKGTYPE_BINARY);
	}

	


	//mDebug("get_size()\n");
	if (get_size()!=0)
	{
		mDebug("local_package.cpp: injectFile(): get_size() FAILED");
		return -1;
	}
	//mDebug("create_md5\n");
	if (create_md5()!=0)
	{
		mDebug("local_package.cpp: injectFile(): create_md5 FAILED");
		return -2;
	}
	//mDebug("set_additional_data\n");
	//mDebug("local_packaige.cpp: injectFile(): filename is "+ filename);
	data.set_filename(filename);
	
	if (set_additional_data()!=0)
	{
		mDebug("local_package.cpp: injectFile(): set_additional_data FAILED");
		return -6;
	}
	delete_tmp_files();	
	//mDebug("local_package.cpp: injectFile(): end");
	return 0;
}

int LocalPackage::CreateFlistNode(string fname, string tmp_xml)
{
	if (getExtension(filename)=="spkg") return 0;
	//mDebug("local_package.cpp: CreateFlistNode(): begin");
	string tar_cmd;
	//mDebug("flist tmpfile: "+fname);
	if (getExtension(filename)=="tgz" || getExtension(filename) == "txz" ||getExtension(filename) == "tlz" ||getExtension(filename) == "tbz" ) tar_cmd="tar tf "+filename+" --exclude install " +" > "+fname + " 2>/dev/null";
	if (system(tar_cmd)!=0)
	{
		mError("Unable to get file list");
		return -1;
	}
#ifdef USE_INTERNAL_SED
	WriteFile(tmp_xml, files2xml(ReadFile(fname)));
#else
	string sed_cmd;
	sed_cmd="echo '<?xml version=\"1.0\" encoding=\"utf-8\"?><package><filelist><file>file_list_header' > "+tmp_xml+" && cat "+ fname +" | sed -e i'</file>\\n<file>'  >> "+tmp_xml+" && echo '</file></filelist></package>' >> "+tmp_xml;
	if (system(sed_cmd)!=0)
	{
		mError("Parsing using sed failed");
		return -1;
	}
#endif
	//mDebug("local_package.cpp: CreateFlistNode(): end");
	return 0;
}

xmlNode LocalPackage::getPackageXMLNode()
{
	return *_packageXMLNode;
}

xmlChar * LocalPackage::getPackageXMLNodeXPtr(int * bufsize)
{
	xmlChar *membuf;
	if (!__doc) fprintf(stderr, "OOPS: NULL __doc in getPackageXMLNodeXPtr\n");
	xmlDocDumpMemory(this->__doc, &membuf, bufsize);
	return membuf;
}

