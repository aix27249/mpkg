/*
Local package installation functions

$Id: local_package.cpp,v 1.77 2007/11/21 14:39:26 i27249 Exp $
*/

#include "local_package.h"
#include "mpkg.h"
//#include "oldstyle.h"
 
int xml2package(xmlNodePtr pkgNode, PACKAGE *data)
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
	if (!p.parseOk) 
	{
		//mDebug("PackageConfig init FAILED, returning -100");
		return -100;
	}
	//else mDebug("PackageConfig init OK");
	data->set_name(p.getName());
	data->set_version(p.getVersion());
	data->set_arch(p.getArch());
	data->set_build(p.getBuild());
	data->set_packager(p.getAuthorName());
	data->set_packager_email(p.getAuthorEmail());
	data->set_description(p.getDescription());
	data->set_short_description(p.getShortDescription());
	data->set_changelog(p.getChangelog());
	data->set_betarelease(p.getBetarelease());
	data->set_provides(p.getProvides());
	data->set_conflicts(p.getConflicts());
	if (p.getValue("//need_special_update")=="yes") data->needSpecialUpdate = true;
	else data->needSpecialUpdate = false;
	
	DEPENDENCY dep_tmp;
	DEPENDENCY suggest_tmp;

	string pkg_type = p.getPackageType();
	if (pkg_type == "source") {
		//printf("xml2package: source\n");
		data->set_type(PKGTYPE_SOURCE);
	}
	else {
		//printf("xml2package: binary ('%s')\n", pkg_type.c_str());
		data->set_type(PKGTYPE_BINARY);
	}

	vector<string> vec_tmp_names;
	vector<string> vec_tmp_conditions;
	vector<string> vec_tmp_versions;

	vector<bool> vec_tmp_buildflags;
	vec_tmp_names=p.getDepNames();
	vec_tmp_conditions=p.getDepConditions();
	vec_tmp_versions=p.getDepVersions();
	vec_tmp_buildflags = p.getDepBuildOnlyFlags();
	// Check sizes:
	if (vec_tmp_names.size()!=vec_tmp_buildflags.size())
	{
		mError("Vector dimensions mismatch!");
		//printf("vec_tmp_names size: %ld\nvec_tmp_buildflags.size: %ld\n", vec_tmp_names.size(), vec_tmp_buildflags.size());
		abort();
	}


	for (unsigned int i=0;i<vec_tmp_names.size();++i)
	{
		dep_tmp.set_package_name(vec_tmp_names[i]);
		dep_tmp.set_package_version(vec_tmp_versions[i]);
		dep_tmp.set_condition(IntToStr(condition2int(vec_tmp_conditions[i])));
		dep_tmp.set_type("DEPENDENCY");
		dep_tmp.setBuildOnly(vec_tmp_buildflags[i]);
		data->get_dependencies_ptr()->push_back(dep_tmp);
		dep_tmp.clear();
	}
	vec_tmp_names=p.getSuggestNames();
	vec_tmp_conditions=p.getSuggestConditions();
	vec_tmp_versions=p.getSuggestVersions();

	for (unsigned int i=0;i<vec_tmp_names.size();++i)
	{
		suggest_tmp.set_package_name(vec_tmp_names[i]);
		suggest_tmp.set_package_version(vec_tmp_versions[i]);
		suggest_tmp.set_condition(IntToStr(condition2int(vec_tmp_conditions[i])));
		suggest_tmp.set_type("SUGGEST");
		data->get_dependencies_ptr()->push_back(suggest_tmp);
		suggest_tmp.clear();
	}

	data->set_tags(p.getTags());

	vec_tmp_names.clear();
	vec_tmp_conditions.clear();
	vec_tmp_versions.clear();

	data->deltaSources=p.getBDeltas();
	LOCATION tmp_location;
	if (!p.getLocation().empty()) {
		tmp_location.set_path(p.getLocation());
		data->get_locations_ptr()->push_back(tmp_location);
	}

	data->set_filename(p.getFilename());
	data->set_md5(p.getMd5());
	data->set_compressed_size(p.getCompressedSize());
	data->set_installed_size(p.getInstalledSize());
	
	data->set_repository_tags(p.getRepositoryTags());
	data->package_distro_version = p.getValue((const char *) "//package/distro_version");
	//xmlFreeDoc(doc);
	return 0;
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

	xml2package(p.getXMLNode(), &data->pkg);
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



LocalPackage::LocalPackage(string _f, string _path_shift)
{
	path_shift = _path_shift;
	internal=false;
	this->filename=_f;
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


	if (__doc == NULL) {
		//mDebug("[get_xml] xml document == NULL");
	} else {
		//mDebug("[get_xml] xml docuemtn != NULL");
	}
	xml2package(p.getXMLNode(), &data);
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
	string fname=get_tmp_file();
	if (setupMode && FileExists(getAbsolutePath(getDirectory(filename))+"/.fcache/" + getFilename(filename) + "/flist")) {
		system("cat " + getAbsolutePath(getDirectory(filename))+"/.fcache/" + getFilename(filename) + "/flist > " + fname);
	}
	else {
		system("tar tf "+filename+" --exclude install " +" > "+fname + " 2>/dev/null");
	}
	// Parsing file list
	file_names=ReadFileStrings(fname);
	unlink(fname.c_str());
	file_names.erase(file_names.begin(), file_names.begin()+2);
	package->set_files(file_names);
	// Retrieving symlinks (from doinst.sh)
	string lnfname=get_tmp_file();
	string sed_cmd = "sed -n 's,^( *cd \\([^ ;][^ ;]*\\) *; *rm -rf \\([^ )][^ )]*\\) *) *$,\\1/\\2,p' < ";
	string dt = get_tmp_file();
	// Extracting file to the temp dir
	if (setupMode && FileExists(getAbsolutePath(getDirectory(filename))+"/.fcache/" + getFilename(filename) + "/doinst.sh")) {
		system("cat " + getAbsolutePath(getDirectory(filename))+"/.fcache/" + getFilename(filename) + "/doinst.sh > " + dt);
	}
	else extractFromTgz(filename, "install/doinst.sh", dt);

	sed_cmd += dt + " > " + lnfname;
	
	if (FileExists(dt)) {
		system(sed_cmd);
		vector<string>link_names=ReadFileStrings(lnfname);
		for (size_t i=0; i<link_names.size(); ++i) {
			if (!link_names[i].empty()) package->get_files_ptr()->push_back(link_names[i]);
		}
	}
	unlink(lnfname.c_str());
	unlink(dt.c_str());

	delete_tmp_files();
	return 0;
}


int LocalPackage::get_filelist(bool index)
{
	return fill_filelist(&data, index);
}
	
int LocalPackage::create_md5()
{
	
	//mDebug("create_md5 start");
	string tmp_md5=get_tmp_file();

	string sys="md5sum "+filename+" > "+tmp_md5;
	system(sys);
	string md5str=ReadFile(tmp_md5).substr(0,32);
	unlink(tmp_md5.c_str());
	//mDebug("MD5 = "+md5str);
	if (md5str.empty())
	{
		mError("Unable to read md5 temp file");
		return 1;
	}
	data.set_md5(md5str);
	xmlNodePtr __node;
	__node = xmlNewChild(_packageXMLNode, NULL, (const xmlChar *)"md5", (const xmlChar *)md5str.c_str());
	delete_tmp_files();
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
	xmlNodePtr __node;
	__node = xmlNewChild(_packageXMLNode, NULL, (const xmlChar *)"signature", (const xmlChar *)pgpstr.c_str());
	return 0;
}
void LocalPackage::set_repository_branch(const string &branch)
{
	data.set_repository_tags(branch);

	xmlNodePtr __node;
	__node = xmlNewTextChild(_packageXMLNode, NULL, (const xmlChar *)"repository_tags", (const xmlChar *)branch.c_str());
	return;
}

void LocalPackage::set_distro_version(const string &distro)
{
	data.package_distro_version = distro;

	xmlNodePtr __node;
	__node = xmlNewTextChild(_packageXMLNode, NULL, (const xmlChar *)"distro_version", (const xmlChar *)distro.c_str());
	return;
}
void setXmlDeltaSources(const vector<DeltaSource>& deltasources, xmlNodePtr _packageXMLNode) {
	//printf("setting delta sources, count: %d\n", deltasources.size());
	xmlNodePtr __node;
	for (unsigned int i=0; i<deltasources.size(); ++i) {
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
	if (getExtension(filename)=="spkg" || getExtension(filename) == "tlz" || getExtension(filename)=="txz" || getExtension(filename)=="tbz")
	{
		struct stat fstat;
		stat(filename.c_str(), &fstat);
		isize = "0";
		data.set_installed_size(isize);
		csize = IntToStr(fstat.st_size);
		data.set_compressed_size(csize);
	}
	if (getExtension(filename)!="spkg") {
		//mDebug("get_size start");
		string tmp=get_tmp_file();
		string sys;
		if (getExtension(filename)=="tgz") sys="gzip -l "+filename+" > "+tmp + " 2>/dev/null";
		if (getExtension(filename)=="tbz") sys="bzcat " + filename + " | wc -c > " + tmp + " 2>/dev/null";
		if (getExtension(filename)=="tlz") sys="lzcat " + filename + " | wc -c > " + tmp + " 2>/dev/null";
		if (getExtension(filename)=="txz") sys="xzcat " + filename + " | wc -c > " + tmp + " 2>/dev/null";
		if (system(sys)!=0)
		{
			unlink(tmp.c_str());
			delete_tmp_files();
			mError("Zero-length package " + filename);
			return -2;
		}
		FILE *zdata=fopen(tmp.c_str(), "r");
		if (!zdata)
		{
			unlink(tmp.c_str());
			mError("Unable to extract size of package");
			return -1;
		}
		char *c_size = (char *) malloc(40000); //FIXME: Overflow are possible here
		char *i_size = (char *) malloc(40000); //FIXME: Same problem
		//mDebug("reading file...");
	
		if (getExtension(filename)=="tgz") {
			for (int i=1; i<=5; i++) {
				if (fscanf(zdata, "%s", c_size)==EOF)
				{
					fclose(zdata);
					unlink(tmp.c_str());
					delete_tmp_files();
					mError("Unexcepted EOF while reading gzip data");
					free(c_size);
					free(i_size);
					return -1;
				}
			}
			fscanf(zdata, "%s", i_size);
			if (c_size) {
				csize=c_size;
				free(c_size);
			}
		}
		else {
			fscanf(zdata, "%s", i_size);
		}
		fclose(zdata);
		unlink(tmp.c_str());
		if (i_size) {
			isize=i_size;
			free(i_size);
		}
		data.set_compressed_size(csize);
		data.set_installed_size(isize);
		//mDebug(" Sizes: C: " + csize + ", I: " + isize);
	}

	xmlNodePtr __node;
	__node = xmlNewTextChild(_packageXMLNode, NULL, (const xmlChar *)"compressed_size", (const xmlChar *)csize.c_str());
	__node = xmlNewTextChild(_packageXMLNode, NULL, (const xmlChar *)"installed_size", (const xmlChar *)isize.c_str());
	//mDebug("get_size end");
	delete_tmp_files();
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
	xmlNodePtr __node;
	__node = xmlNewTextChild(_packageXMLNode, NULL, (const xmlChar *)"filename", (const xmlChar *)data.get_filename().c_str());
	__node = xmlNewTextChild(_packageXMLNode, NULL, (const xmlChar *)"location", (const xmlChar *)fpath.c_str());
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
	xmlDocDumpMemory(this->__doc, &membuf, bufsize);
	return membuf;
}

