#include "metaframe.h"
#include "package.h"
void parseSlackDesc(const string& slackDescFile, string *shortdesc, string *longdesc) {
	string description=ReadFile(slackDescFile);
	if (description.empty()) {
		mError("Cannot read slack-desc");
		return;
	}
	string tmp;
	// Step 1. Skip comments
	unsigned int dpos=0;
	//unsigned int strLen=0;
	string comment;
	string head;
	string short_description;
	//bool str1=true;
	if (!description.empty()) {
		// Cleaning out comments
		for (size_t i=0; i<description.length(); i++) {
			if (description[i]=='#') {
				while (description[i]!='\n') i++;
			} if (i<description.length()) tmp+=description[i];
		}
		// try to autodetect pkgName
		string pkgName;
		vector<string> vdesc = MakeStrings(tmp);
		for (size_t i=0; i<vdesc.size(); ++i) {
			if (vdesc[i].find(": ")!=std::string::npos) {
				pkgName = vdesc[i].substr(0, vdesc[i].find(": "));
			}
		}
		if (pkgName.empty()) {
			mError(string(__func__) + ": Failed to autodetect package name, exiting");
			return;
		}
		else printf("Found pkgName: [%s]\n", pkgName.c_str());
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
	printf("SHORT: %s\nLONG: %s\n", short_description.c_str(), description.c_str());
	*shortdesc = short_description;
	*longdesc = description;
}
MetaPackage::MetaPackage(const string& _f) {
	lp = NULL;
	data = NULL;
	if (!FileExists(_f)) {
		mError(_("File or directory ") + _f + _(" doesn't exist"));
		return;
	}
	if (isDirectory(_f)) {
		if (!FileExists(_f+"/install")) {
			mError(_("No install directory exists here! Create it manually."));
			return;
		}
		data = new PACKAGE;
		bool canParse=false;
		if (FileExists(_f + "/install/data.xml")) {
			PackageConfig p(_f + "/install/data.xml");
			if (p.parseOk) {
				canParse = true;
				xml2package(p.getXMLNode(), data);
				data->sync();
			}
		}
		if (data->get_description().empty() && data->get_short_description().empty() && FileExists(_f+"/install/slack-desc")) {
			printf("Parsing slack-desc\n");
			string sdesc, ldesc;
			parseSlackDesc(_f+"/install/slack-desc", &sdesc, &ldesc);
			data->set_description(ldesc);
			data->set_short_description(sdesc);
		}
		if (data->get_packager().empty()) {
			string mName = mConfig.getValue("maintainer_name");
			string mEmail = mConfig.getValue("maintainer_email");
			if (mName.empty()) {
				mName = "Anonymous maintainer";
				mWarning(_("No maintainer name specified in configuration file! Please specify it if you going to public your packages."));
			}
			if (mEmail.empty()) {
				mEmail = "No email specified";
				mWarning(_("No maintainer e-mail specified in configuration file! Please specify it if you going to public your packages."));
			}
			data->set_packager(mName);
			data->set_packager_email(mEmail);
		}
	}
	else {
		lp = new LocalPackage(_f);
		lp->injectFile();
		data = &lp->data;
	}
	pkgFilename = _f;
	
}

MetaPackage::~MetaPackage() {
	if (lp) delete lp;
	else {
		if (data) delete data;
	}
}

bool MetaPackage::saveToXML(const string& xmlFilename) {
	// Let's generate new xmlTree
	XMLNode node;
	node = XMLNode::createXMLTopNode("package");
	if (data->needSpecialUpdate) {
		node.addChild("need_special_update");
		node.getChildNode("need_special_update").addText("yes");
	}
	node.addChild("name");
	node.getChildNode("name").addText(data->get_name().c_str());
	node.addChild("version");
	node.getChildNode("version").addText(data->get_version().c_str());
	if (!data->get_betarelease().empty()) {
		node.addChild("betarelease");
		node.getChildNode("betarelease").addText(data->get_betarelease().c_str());
	}
	node.addChild("arch");
	node.getChildNode("arch").addText(data->get_arch().c_str());
	node.addChild("build");
	node.getChildNode("build").addText(data->get_build().c_str());

	if (!data->get_provides().empty()){
		node.addChild("provides").addText(data->get_provides().c_str());
	}
	if (!data->get_conflicts().empty()){
		node.addChild("conflicts").addText(data->get_conflicts().c_str());
	}

	node.addChild("short_description");
	node.getChildNode("short_description").addText(data->get_short_description().c_str());
	node.addChild("description");
	node.getChildNode("description").addText(data->get_description().c_str());
	node.addChild("dependencies");
	node.addChild("suggests");
	unsigned int dep_id=0, sug_id=0;
	for (unsigned int i=0; i<data->get_dependencies().size(); ++i) {
		if (data->get_dependencies()[i].get_type()=="DEPENDENCY") {
			node.getChildNode("dependencies").addChild("dep");
			
			node.getChildNode("dependencies").getChildNode("dep", dep_id).addChild("name").addText(data->get_dependencies()[i].get_package_name().c_str());
			node.getChildNode("dependencies").getChildNode("dep", dep_id).addChild("condition").addText(condition2xml(data->get_dependencies()[i].get_condition()).c_str());
			node.getChildNode("dependencies").getChildNode("dep", dep_id).addChild("version").addText(data->get_dependencies()[i].get_package_version().c_str());

			dep_id++;
		}
		if (data->get_dependencies()[i].get_type()=="SUGGEST") {
			node.getChildNode("suggests").addChild("suggest");

			node.getChildNode("suggests").getChildNode("suggest", sug_id).addChild("name").addText(data->get_dependencies()[i].get_package_name().c_str());
			node.getChildNode("suggests").getChildNode("suggest", sug_id).addChild("condition").addText(condition2xml(data->get_dependencies()[i].get_condition()).c_str());
			node.getChildNode("suggests").getChildNode("suggest", sug_id).addChild("version").addText(data->get_dependencies()[i].get_package_version().c_str());
			sug_id++;
		}
	}
	if (!data->get_changelog().empty() && data->get_changelog().find_first_not_of("0 \n\t")!=std::string::npos) node.addChild("changelog").addText(data->get_changelog().c_str());
	node.addChild("maintainer").addChild("name").addText(data->get_packager().c_str());
	node.getChildNode("maintainer").addChild("email").addText(data->get_packager_email().c_str());
	if (!data->get_tags().empty()) node.addChild("tags");
	for (unsigned int i=0; i<data->get_tags().size(); ++i) {
		node.getChildNode("tags").addChild("tag").addText(data->get_tags()[i].c_str());
	}

	if (!data->get_config_files().empty()) node.addChild("configfiles");
	for (unsigned int i=0; i<data->get_config_files().size(); ++i) {
		node.getChildNode("configfiles").addChild("conffile").addText(data->get_config_files()[i].get_name().c_str());
	}

	if (!data->get_temp_files().empty()) node.addChild("tempfiles");
	for (unsigned int i=0; i<data->get_temp_files().size(); ++i) {
		node.getChildNode("tempfiles").addChild("tempfile").addText(data->get_temp_files()[i].get_name().c_str());
	}

	return node.writeToFile(xmlFilename.c_str())==eXMLErrorNone;
}

bool MetaPackage::savePackage() {
	if (!isDirectory(pkgFilename)) {
		BinaryPackage pkg(pkgFilename);
		pkg.unpackFile();
		saveToXML(pkg.getDataFilename());
		bool ret = pkg.packFile();
		pkg.clean();
		return ret;
	}
	else {
		saveToXML(pkgFilename + "/install/data.xml");
		return true;
	}
}

MetaSrcPackage::MetaSrcPackage(string _f) {
	data = NULL;
	sp = NULL;
	if (_f.empty()) {
		// Creating an empty package
		data = new SPKG;
		sp = new SourcePackage;
		sp->createNew();
		// Set some defaults:
		data->allow_change_cflags = true;
		data->use_cflags = true;
		data->pkg.set_packager(mConfig.getValue("maintainer_name"));
		data->pkg.set_packager_email(mConfig.getValue("maintainer_email"));
		return;
	}
	if (!FileExists(_f)) {
		mError(_("File or directory ") + _f + _(" doesn't exist"));
		return;
	}
	if (isDirectory(_f)) {
		if (!FileExists(_f+"/install")) {
			mError(_("No install directory exists here! Create it manually."));
			return;
		}
		data = new SPKG;
		bool canParse=false;
		if (FileExists(_f + "/install/data.xml")) {
			PackageConfig p(_f + "/install/data.xml");
			if (p.parseOk) {
				canParse = true;
				xml2spkg(p.getXMLNode(), data);
				data->pkg.sync();
			}
		}
		if (data->pkg.get_description().empty() && data->pkg.get_short_description().empty() && FileExists(_f+"/install/slack-desc")) {
			printf("Parsing slack-desc\n");
			string sdesc, ldesc;
			parseSlackDesc(_f+"/install/slack-desc", &sdesc, &ldesc);
			data->pkg.set_description(ldesc);
			data->pkg.set_short_description(sdesc);
		}
		if (data->pkg.get_packager().empty()) {
			string mName = mConfig.getValue("maintainer_name");
			string mEmail = mConfig.getValue("maintainer_email");
			if (mName.empty()) {
				mName = "Anonymous maintainer";
				mWarning(_("No maintainer name specified in configuration file! Please specify it if you going to public your packages."));
			}
			if (mEmail.empty()) {
				mEmail = "No email specified";
				mWarning(_("No maintainer e-mail specified in configuration file! Please specify it if you going to public your packages."));
			}
			data->pkg.set_packager(mName);
			data->pkg.set_packager_email(mEmail);
		}
	}
	else {
		data = new SPKG;
		sp = new SourcePackage;
		sp->setInputFile(_f);
		sp->unpackFile();
		PackageConfig p(sp->getDataFilename());
		if (p.parseOk) {
			xml2spkg(p.getXMLNode(), data);
			data->pkg.sync();
		}
	}
	pkgFilename = _f;
	
}
MetaSrcPackage::~MetaSrcPackage() {
	if (sp) delete sp;
	if (data) delete data;
}

bool MetaSrcPackage::saveToXML(const string& xmlFilename) {
	// Let's generate new xmlTree
	XMLNode node;
	node = XMLNode::createXMLTopNode("package");
	if (data->pkg.needSpecialUpdate) {
		node.addChild("need_special_update");
		node.getChildNode("need_special_update").addText("yes");
	}
	node.addChild("name");
	node.getChildNode("name").addText(data->pkg.get_name().c_str());
	
	if (!data->pkg.get_provides().empty()){
		node.addChild("provides").addText(data->pkg.get_provides().c_str());
	}
	if (!data->pkg.get_conflicts().empty()){
		node.addChild("conflicts").addText(data->pkg.get_conflicts().c_str());
	}
	node.addChild("version");
	node.getChildNode("version").addText(data->pkg.get_version().c_str());
	if (!data->pkg.get_betarelease().empty()) {
		node.addChild("betarelease");
		node.getChildNode("betarelease").addText(data->pkg.get_betarelease().c_str());
	}
	node.addChild("arch");
	node.getChildNode("arch").addText(data->pkg.get_arch().c_str());
	node.addChild("build");
	node.getChildNode("build").addText(data->pkg.get_build().c_str());

	node.addChild("short_description");
	node.getChildNode("short_description").addText(data->pkg.get_short_description().c_str());
	node.addChild("description");
	node.getChildNode("description").addText(data->pkg.get_description().c_str());
	node.addChild("dependencies");
	node.addChild("suggests");
	size_t dep_id=0, sug_id=0;
	for (size_t i=0; i<data->pkg.get_dependencies().size(); ++i) {
		if (data->pkg.get_dependencies()[i].get_type()=="DEPENDENCY") {
			node.getChildNode("dependencies").addChild("dep");
			
			node.getChildNode("dependencies").getChildNode("dep", dep_id).addChild("name").addText(data->pkg.get_dependencies()[i].get_package_name().c_str());
			node.getChildNode("dependencies").getChildNode("dep", dep_id).addChild("condition").addText(condition2xml(data->pkg.get_dependencies()[i].get_condition()).c_str());
			node.getChildNode("dependencies").getChildNode("dep", dep_id).addChild("version").addText(data->pkg.get_dependencies()[i].get_package_version().c_str());

			dep_id++;
		}
		if (data->pkg.get_dependencies()[i].get_type()=="SUGGEST") {
			node.getChildNode("suggests").addChild("suggest");

			node.getChildNode("suggests").getChildNode("suggest", sug_id).addChild("name").addText(data->pkg.get_dependencies()[i].get_package_name().c_str());
			node.getChildNode("suggests").getChildNode("suggest", sug_id).addChild("condition").addText(condition2xml(data->pkg.get_dependencies()[i].get_condition()).c_str());
			node.getChildNode("suggests").getChildNode("suggest", sug_id).addChild("version").addText(data->pkg.get_dependencies()[i].get_package_version().c_str());
			sug_id++;
		}
	}
	if (!data->pkg.get_changelog().empty() && data->pkg.get_changelog().find_first_not_of("0 \n\t")!=std::string::npos) node.addChild("changelog").addText(data->pkg.get_changelog().c_str());
	node.addChild("maintainer").addChild("name").addText(data->pkg.get_packager().c_str());
	node.getChildNode("maintainer").addChild("email").addText(data->pkg.get_packager_email().c_str());
	if (!data->pkg.get_tags().empty()) node.addChild("tags");
	for (unsigned int i=0; i<data->pkg.get_tags().size(); ++i) {
		node.getChildNode("tags").addChild("tag").addText(data->pkg.get_tags()[i].c_str());
	}

	if (!data->pkg.get_config_files().empty()) node.addChild("configfiles");
	for (unsigned int i=0; i<data->pkg.get_config_files().size(); ++i) {
		node.getChildNode("configfiles").addChild("conffile").addText(data->pkg.get_config_files()[i].get_name().c_str());
	}

	if (!data->pkg.get_temp_files().empty()) node.addChild("tempfiles");
	for (unsigned int i=0; i<data->pkg.get_temp_files().size(); ++i) {
		node.getChildNode("tempfiles").addChild("tempfile").addText(data->pkg.get_temp_files()[i].get_name().c_str());
	}

	// Mbuild-related
		node.addChild("mbuild");
		if (!data->url.empty()) {
			node.getChildNode("mbuild").addChild("url");
			node.getChildNode("mbuild").getChildNode("url").addText(data->url.c_str());
		}
		if (!data->patches.empty()) {
			node.getChildNode("mbuild").addChild("patches");
			for (unsigned int i=0; i<data->patches.size(); ++i) {
				node.getChildNode("mbuild").getChildNode("patches").addChild("patch");
				node.getChildNode("mbuild").getChildNode("patches").getChildNode("patch",i).addText(getFilename(data->patches[i]).c_str());
			}
		}
		if (!data->source_root.empty()) node.getChildNode("mbuild").addChild("sources_root_directory").addText(data->source_root.c_str());
		node.getChildNode("mbuild").addChild("build_system").addText(data->buildsystem.c_str());
		if (data->no_subfolder) node.getChildNode("mbuild").addChild("no_subfolder").addText("true");
		
		node.getChildNode("mbuild").addChild("max_numjobs").addText(data->max_numjobs.c_str());
		
		node.getChildNode("mbuild").addChild("optimization");
		if (!data->march.empty()) {
			node.getChildNode("mbuild").getChildNode("optimization").addChild("march").addText(data->march.c_str());
		}
		if (!data->mtune.empty()) {
			node.getChildNode("mbuild").getChildNode("optimization").addChild("mtune").addText(data->mtune.c_str());
		}
		if (!data->olevel.empty()) {
			node.getChildNode("mbuild").getChildNode("optimization").addChild("olevel").addText(data->olevel.c_str());
		}
		if (!data->custom_gcc_options.empty()) {
			node.getChildNode("mbuild").getChildNode("optimization").addChild("custom_gcc_options").addText(data->custom_gcc_options.c_str());
		}

		node.getChildNode("mbuild").getChildNode("optimization").addChild("allow_change");
		if (data->allow_change_cflags)
			node.getChildNode("mbuild").getChildNode("optimization").getChildNode("allow_change").addText("true");
		else   	
			node.getChildNode("mbuild").getChildNode("optimization").getChildNode("allow_change").addText("false");
	
		node.getChildNode("mbuild").addChild("use_cflags");	
		if (data->use_cflags)
			node.getChildNode("mbuild").getChildNode("use_cflags").addText("false");
		else
			node.getChildNode("mbuild").getChildNode("use_cflags").addText("true");

		if (!data->env_options.empty())	{
			node.getChildNode("mbuild").addChild("env_options").addText(data->env_options.c_str());
		}
		node.getChildNode("mbuild").addChild("configuration");
		for (unsigned int i=0; i<data->configure_keys.size(); ++i) {
			node.getChildNode("mbuild").getChildNode("configuration").addChild("key").addChild("name").addText(data->configure_keys[i].c_str());
			node.getChildNode("mbuild").getChildNode("configuration").getChildNode("key",i).addChild("value").addText(data->configure_values[i].c_str());
		}
		if (!data->custom_configure.empty() || !data->custom_make.empty() || !data->custom_makeinstall.empty()) {
			node.getChildNode("mbuild").addChild("custom_commands");
			if (!data->custom_configure.empty()) {
				node.getChildNode("mbuild").getChildNode("custom_commands").addChild("configure").addText(data->custom_configure.c_str());
			}
			if (!data->custom_make.empty()) {
				node.getChildNode("mbuild").getChildNode("custom_commands").addChild("make").addText(data->custom_make.c_str());
			}
			if (!data->custom_makeinstall.empty()) {
				node.getChildNode("mbuild").getChildNode("custom_commands").addChild("make_install").addText(data->custom_makeinstall.c_str());
			}
		}
		if (data->nostrip) {
			node.getChildNode("mbuild").addChild("nostrip").addText("true");
		}
	
	return node.writeToFile(xmlFilename.c_str())==eXMLErrorNone;
}

bool MetaSrcPackage::savePackage(string saveTo) {
	if (sp) {
		saveToXML(sp->getDataFilename());
		return sp->packFile(saveTo);
	}
	/*if (!isDirectory(pkgFilename)) {
		BinaryPackage pkg(pkgFilename);
		pkg.unpackFile();
		saveToXML(pkg.getDataFilename());
		bool ret = pkg.packFile();
		pkg.clean();
		return ret;
	}*/
	else {
		saveToXML(pkgFilename + "/install/data.xml");
		return true;
	}
}

