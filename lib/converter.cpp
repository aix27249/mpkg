/******************************************************
 * Data converter for legacy Slackware packages
 * $Id: converter.cpp,v 1.18 2007/11/28 02:24:25 i27249 Exp $
 * ***************************************************/

#include "converter.h"
#include "package.h"
#define GET_TXT_DESC
int slack_convert(const string& filename, string& xml_output)
{
	mDebug("Preparing to convert " + filename);
	PACKAGE package;
	package.set_filename(filename);
	// Resolving name, version, arch and build
	string tmp;
	string tmp_xml = get_tmp_file();
	extractFromTgz(filename, "install/data.xml", tmp_xml);
	if (FileNotEmpty(tmp_xml))
	{
		WriteFile(xml_output, ReadFile(tmp_xml));
		delete_tmp_files();
		return 0;
	}
	string ptrstr;
	// Guessing type
	string pkgType = getExtension(filename);
	if (pkgType!="tgz" && pkgType!="txz" && pkgType!="tlz" && pkgType!="tbz") {
		mError(_("Unknown package type ") + pkgType);
		return 1;
	}
	// Name-ver-arch-build parsing
	string name_tmp=filename.substr(0,filename.find("." + pkgType));
	name_tmp = name_tmp.substr(name_tmp.find_last_of("/")+1);
	ptrstr = name_tmp.substr(name_tmp.find_last_of("-")+1);
	package.set_build(ptrstr);
	name_tmp = name_tmp.substr(0,name_tmp.find_last_of("-"));
	ptrstr = name_tmp.substr(name_tmp.find_last_of("-")+1);
	package.set_arch(ptrstr);
	name_tmp = name_tmp.substr(0,name_tmp.find_last_of("-"));
	ptrstr=name_tmp.substr(name_tmp.find_last_of("-")+1);
	package.set_version(ptrstr);
	name_tmp = name_tmp.substr(0,name_tmp.find_last_of("-"));
	package.set_name(name_tmp);
	name_tmp.clear();


#define DESCRIPTION_PROCESS
#ifdef DESCRIPTION_PROCESS	
	//DESCRIPTION
	mDebug("Processing description");
	string desc_file= filename.substr(0,filename.length()-3)+"txt";
	bool can_read=false;
#ifdef GET_TXT_DESC
	if (access(desc_file.c_str(), R_OK)==0)
	{
		mDebug("Retrieving from txt");
		can_read=true;
	}
	else 
	{
#endif
		desc_file=get_tmp_file();
		string desc="tar xf "+filename+" install/slack-desc --to-stdout > "+desc_file;
		if (system(desc.c_str())==0)
		{

			mDebug("Retrieving from slack-desc");
			can_read=true;
		}
		else mDebug("Cannot find any description");
#ifdef GET_TXT_DESC
	}
#endif
	if (can_read)
	{
		string description=ReadFile(desc_file);
		delete_tmp_files();
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
			string pHead=package.get_name()+":";
			int spos=description.find(pHead,0);
			// Removing package: headers
			for (unsigned int i=spos; i<description.length(); i++)
			{
				//head+=description[i];
				if (i==0 || description[i-1]=='\n')
				{
					i=i+package.get_name().length()+1;
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
			package.set_short_description(short_description);
			package.set_description(description);
			mDebug("Description: " + description);
		}
	}
#endif
	XMLNode pkg=XMLNode::createXMLTopNode("package");
	pkg.addChild("name");
	pkg.getChildNode("name").addText(package.get_name().c_str());
	pkg.addChild("version");
	pkg.getChildNode("version").addText(package.get_version().c_str());
	pkg.addChild("arch");
	pkg.getChildNode("arch").addText(package.get_arch().c_str());
	pkg.addChild("build");
	pkg.getChildNode("build").addText(package.get_build().c_str());
#ifdef DESCRIPTION_PROCESS
	mDebug("Adding description to XML node");
	pkg.addChild("short_description");
	pkg.getChildNode("short_description").addText(package.get_short_description().c_str());
	pkg.addChild("description");
	pkg.getChildNode("description").addText(package.get_description().c_str());
#endif
#ifdef TAG_CONVERTED
	pkg.addChild("tags");
	pkg.getChildNode("tags").addChild("tag");
	pkg.getChildNode("tags").getChildNode("tag").addText("slackware");
#endif
	// Get maintainer name and email
	string mName = mConfig.getValue("maintainer_name");
	string mEmail = mConfig.getValue("maintainer_email");
	if (mName.empty()) mName = "Converted by anonymous maintainer";
	if (mEmail.empty()) mEmail = "No email specified";
	pkg.addChild("maintainer");
	pkg.getChildNode("maintainer").addChild("name");
	pkg.getChildNode("maintainer").getChildNode("name").addText(mName.c_str());
	pkg.getChildNode("maintainer").addChild("email");
	pkg.getChildNode("maintainer").getChildNode("email").addText(mEmail.c_str());
	pkg.writeToFile(xml_output.c_str(), "utf-8");
	return 0;
}

int convert_package(const string& filename, const string& output_dir)
{
//	char tmp[1000];
	say("converting package %s\n", filename.c_str());
	int name_start=0;
	for (int i=filename.length()-1; filename[i]!='/' && i>=0; i--)
	{
		name_start=i;
	}
	string real_filename;
	for (unsigned int i=name_start; i<filename.length(); i++)
	{
		real_filename+=filename[i];
	}
	string ext_outdir = filename.substr(0,filename.find_last_of("/"));
	string tmp_dir=get_tmp_file();
	string xml_output=tmp_dir+"/install/data.xml";
	string reasm="rm "+tmp_dir+" && mkdir -p "+tmp_dir+"/install >/dev/null && cp "+filename+" "+tmp_dir+"/ && cd "+tmp_dir+" &&  tar xf "+real_filename+" > /dev/null";
	system(reasm.c_str());	
	slack_convert(filename, xml_output);
	system("mkdir -p " + output_dir+"/"+ext_outdir);
	reasm="cd "+tmp_dir+" && rm "+real_filename+" && " + MAKEPKG_CMD +" "+output_dir+"/"+ext_outdir + "/" + real_filename+" 2>/dev/null >/dev/null && rm -rf "+tmp_dir;

	int ret = system(reasm.c_str());
	if (ret) say("An error occured while converting package %s\n", filename.c_str());
	else say("%s: OK\n", filename.c_str());
	delete_tmp_files();
	return 0;
}

int buildup_package(const string& filename)
{
	SourcePackage spkg;
	BinaryPackage pkg;
	bool binary_pkg = false;
	string xml_path;
	string pkgType = getExtension(filename);
	if (pkgType=="tgz" || pkgType=="tbz" || pkgType=="txz" || pkgType=="tlz") {
		binary_pkg = true;
		pkg.setInputFile(filename);
		pkg.unpackFile();
		xml_path = pkg.getDataFilename();
	}
	if (filename.find(".spkg")!=std::string::npos) {

		spkg.setInputFile(filename);
		spkg.unpackFile();
		xml_path = spkg.getDataFilename();
	}
	if (!FileExists(xml_path)) return -2;
	XMLResults xmlErrCode;
	XMLNode _node = XMLNode::parseFile(xml_path.c_str(), "package", &xmlErrCode);
	if (xmlErrCode.error != eXMLErrorNone)
	{
		mError("parse error");
		fprintf(stderr, "%s\n", _node.getError(xmlErrCode.error));
		return -1;
	}
	mDebug("File opened");
	if (_node.nChildNode("build")==0)
	{
		mError("Invalid package");
		return -1;
	}
	int build_num = atoi(_node.getChildNode("build").getText());
	_node.getChildNode("build").updateText(IntToStr(build_num+1).c_str());
	_node.writeToFile(xml_path.c_str());
	if (binary_pkg) pkg.packFile();
	else spkg.packFile();
	return 0;

}

int setver_package(const string& filename, const string& version) {
	SourcePackage spkg;
	BinaryPackage pkg;
	bool binary_pkg = false;
	string xml_path;
	string pkgType = getExtension(filename);
	if (pkgType=="tgz" || pkgType=="tbz" || pkgType=="txz" || pkgType=="tlz") {
		binary_pkg = true;
		pkg.setInputFile(filename);
		pkg.unpackFile();
		xml_path = pkg.getDataFilename();
	}
	if (filename.find(".spkg")!=std::string::npos) {

		spkg.setInputFile(filename);
		spkg.unpackFile();
		xml_path = spkg.getDataFilename();
	}
	if (!FileExists(xml_path)) return -2;
	XMLResults xmlErrCode;
	XMLNode _node = XMLNode::parseFile(xml_path.c_str(), "package", &xmlErrCode);
	if (xmlErrCode.error != eXMLErrorNone)
	{
		mError("parse error");
		fprintf(stderr, "%s\n", _node.getError(xmlErrCode.error));
		return -1;
	}
	mDebug("File opened");
	if (_node.nChildNode("version")==0)
	{
		mError("Invalid package");
		return -1;
	}
	string old_ver = _node.getChildNode("version").getText();
	_node.getChildNode("version").updateText(version.c_str());
	string url;
	if (_node.nChildNode("mbuild")!=0 && _node.getChildNode("mbuild").nChildNode("url")!=0) {
		url = _node.getChildNode("mbuild").getChildNode("url").getText();
		strReplace(&url, old_ver, version);
		_node.getChildNode("mbuild").getChildNode("url").updateText(url.c_str());
	}
	_node.writeToFile(xml_path.c_str());
	if (binary_pkg) pkg.packFile();
	else spkg.packFile();
	return 0;


}
int tag_package(const string& filename, const string& tag, bool delete_other)
{
	string pkgType = getExtension(filename);
	string redirect = " >/dev/null 2>/dev/null ";
	if (!dialogMode) redirect.clear();
	else ncInterface.showInfoBox(_("Tagging package ") + filename + _(" with tag ") + tag + "...");
	MetaPackage pkg(filename);
	if (!pkg.data) return 1;
	if (delete_other) pkg.data->get_tags_ptr()->clear();
	pkg.data->add_tag(tag);
	pkg.savePackage();

	return 0;
}

