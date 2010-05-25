// $Id: package.cpp,v 1.9 2007/12/07 03:34:20 i27249 Exp $

#include "package.h"
BinaryPackage::BinaryPackage()
{
	extracted=false;

	usingDirectory=false;
}

BinaryPackage::BinaryPackage(string in_file)
{
	extracted=false;
	input_file = in_file;

	usingDirectory=false;
}
void BinaryPackage::clean()
{
	if (extracted)
	{
		printf("Cleaning up [%s]...\n", pkg_dir.c_str());
		system("rm -rf " + pkg_dir);
	}

}
BinaryPackage::~BinaryPackage()
{
}

bool BinaryPackage::isExtracted()
{
	return extracted;
}

bool BinaryPackage::fileOk()
{
	if (usingDirectory) return true;
	if (access(input_file.c_str(), R_OK)==0) return true;
	else return false;
}
bool BinaryPackage::createWorkingDirectory()
{
	//printf("Creating working dir\n");
	pkg_dir = get_tmp_file();
	//printf("dir created in %s\n", pkg_dir.c_str());
	unlink(pkg_dir.c_str());
	if (mkdir(pkg_dir.c_str(), 755)!=0) {
		extracted=false;
		return false;
	}	
	else {
		extracted=true;
	       	return true;
	}
}
bool BinaryPackage::importDirectory(string dirname)
{
	usingDirectory=true;
	pkg_dir = dirname;
	extracted=true;
	return true;
}
bool BinaryPackage::createFolderStructure()
{
	string prefix;
	if (extracted)
	{
		string _dinstall = pkg_dir + "/install";
		if (mkdir(_dinstall.c_str(), 755)!=0) return false;
		//if (sourceBased) prefix="/install";
	/*	_dinstall = pkg_dir + prefix+"/build_data";
		if (mkdir(_dinstall.c_str(), 755)!=0) return false;
		_dinstall = pkg_dir + prefix+ "/patches";
		if (mkdir(_dinstall.c_str(), 755)!=0) return false;*/
		return true;
	}
	else {
		mError("Mmmm... Where to create, yep?");
		return false;
	}
}
bool BinaryPackage::createNew()
{
	if (createWorkingDirectory() && createFolderStructure()) return true;
	else {
		mError("Unable to create");
		return false;
	}
	return true;
}

bool BinaryPackage::setInputFile(string in_file)
{
	if (extracted) {
		//printf("Warning: assigning a new filename while previous was extracted\n");
	}
	input_file = in_file;
	return fileOk();
}

bool BinaryPackage::unpackFile()
{
	if (usingDirectory) return true;
	if (extracted) {
		mError("Unable to extract package, because it is already extracted\n");
		return false;
	}
	if (!fileOk()) {
		mError("Unable to extract package: cannot find or read the archive");
		return false;
	}
	// All seems to be ok?
	
	createWorkingDirectory();
	if (system("tar xvf " + input_file + " -C " + pkg_dir)!=0) {
		mError("Error while extracting archive");
		return false;
	}
	extracted=true;
	return true;
}

bool BinaryPackage::packFile(string output_directory, string* full_output_dir)
{
	if (usingDirectory) return true;
	printf("Packing binary archive\n");
	string oldinput = input_file;
	if (output_directory.empty())
	{
		if (input_file.empty()) {
			mError("Don't know where to write");
			return false;
		}
		else output_directory = getDirectory(input_file);
	}
	
	if (full_output_dir) *full_output_dir=getAbsolutePath(output_directory);
	string keeps;
	if (_cmdOptions["keep_symlinks"]=="true") keeps = " -s ";

	if (system("(cd " + pkg_dir+"; buildpkg " + keeps + getAbsolutePath(output_directory)+")")==0) {
		//if (!oldinput.empty()) unlink(oldinput.c_str());
		return true;
	}
	else {
		mError("Failed to build a package");
		return false;
	}
}

string BinaryPackage::getExtractedPath()
{
	return pkg_dir;
}
string BinaryPackage::getDataFilename()
{
	//printf("pkg_dir=%s\n", pkg_dir.c_str());
	return pkg_dir+"/install/data.xml";
}

bool BinaryPackage::setPreinstallScript(string script_text)
{
	if (extracted) {

		createFolderStructure();
		if (script_text.empty())
		{
			string n=pkg_dir + "/install/preinst.sh";
			unlink(n.c_str());
			return true;
		}
		if (WriteFile(pkg_dir+"/install/preinst.sh", script_text)==0) return true;
		else {
			mError("Error writing script");
			return false;
		}
	}
	else {
		mError("No working directory");
		return false;
	}
}
bool BinaryPackage::setPostinstallScript(string script_text)
{
	if (extracted) {

		createFolderStructure();
		if (script_text.empty())
		{
			string n=pkg_dir + "/install/doinst.sh";
			unlink(n.c_str());
			return true;
		}

		if (WriteFile(pkg_dir+"/install/doinst.sh", script_text)==0) return true;
		else {
			mError("Error writing script");
			return false;
		}
	}
	else {
		mError("No working directory");
		return false;
	}
}
bool BinaryPackage::setPreremoveScript(string script_text)
{
	if (extracted) {

		createFolderStructure();
		if (script_text.empty())
		{
			string n=pkg_dir + "/install/preremove.sh";
			unlink(n.c_str());
			return true;
		}

		if (WriteFile(pkg_dir+"/install/preremove.sh", script_text)==0) return true;
		else {
			mError("Error writing script");
			return false;
		}
	}
	else {
		mError("No working directory");
		return false;
	}
}
bool BinaryPackage::setPostremoveScript(string script_text)
{
	if (extracted) {

		createFolderStructure();
		if (script_text.empty())
		{
			string n=pkg_dir + "/install/postremove.sh";
			unlink(n.c_str());
			return true;
		}

		if (WriteFile(pkg_dir+"/install/postremove.sh", script_text)==0) return true;
		else {
			mError("Error writing script");
			return false;
		}
	}
	else {
		mError("No working directory");
		return false;
	}
}

string BinaryPackage::readPreinstallScript()
{
	return ReadFile(pkg_dir+"/install/preinst.sh");
}

string BinaryPackage::readPostinstallScript()
{
	return ReadFile(pkg_dir+"/install/doinst.sh");
}
string BinaryPackage::readPreremoveScript()
{
	return ReadFile(pkg_dir+"/install/preremove.sh");
}
string BinaryPackage::readPostremoveScript()
{
	return ReadFile(pkg_dir+"/install/postremove.sh");
}


bool SourcePackage::createFolderStructure()
{
	if (extracted)
	{
		string _dinstall = pkg_dir + "/install";
		string _patchdir = pkg_dir + "/patches";
		string _builddir = pkg_dir + "/build_data";
		mkdir(_dinstall.c_str(), 755);
		mkdir(_patchdir.c_str(), 755);
		mkdir(_builddir.c_str(), 755);
	       	return true;
	}
	else {
		mError("Mmmm... Where to create, yep?");
		return false;
	}
}

bool SourcePackage::embedPatch(string filename)
{
	if (getFilename(filename)==filename && FileExists(pkg_dir+"/patches/"+filename)) {
		say("Patch %s is already there\n", filename.c_str());
		return true;
	}
	if (createFolderStructure()) {
		return copyFile(filename, pkg_dir + "/patches/");
	}
	else return false;
}

bool SourcePackage::embedSource(string filename)
{
	string wget_options;
	if (filename.find("https://")==0) wget_options = " --no-check-certificate ";
	if (createFolderStructure()) {
		if (!source_filename.empty()) removeSource();
		if (filename.find("http://")==0 || filename.find("https://")==0 || filename.find("ftp://")==0)
		{
			if (system("(cd " + pkg_dir + "/; wget " + wget_options + filename+")")==0) return true;
			else return false;
		}
			
		if (filename.find("/")==std::string::npos) source_filename = filename;
		else {
			if (filename.find_last_of("/")<filename.length()-1) source_filename = filename.substr(filename.find_last_of("/")+1);
			else {
				mError("Invalid filename");
				return false;
			}
		}
		return copyFile(filename, pkg_dir + "/");
	}
	else {
		mError("No structure!\n");
		return false;
	}
}

bool SourcePackage::isSourceEmbedded(string url)
{
	if (!extracted) {
		mError("Package isn't extracted, cannot determine source existance");
		return false;
	}
	if (FileExists(pkg_dir+"/"+getFilename(url))) return true;
	else return false;
}
bool SourcePackage::isPatchEmbedded(string patch_name)
{
	if (!extracted) {
		mError("Package isn't extracted, cannot determine source existance");
		return false;
	}
	if (FileExists(pkg_dir+"/patches/"+getFilename(patch_name))) return true;
	else return false;

}
bool SourcePackage::removeSource(string filename)
{
	//printf("Removing source\n");
	if (extracted)
	{
		vector<string> sList = getSourceFilenames();
		if (sList.size()>1) mWarning("Multiple source filenames");

		if (filename.empty()) {
			string tmp;
			for (unsigned int i=0; i<sList.size(); i++)
			{
				tmp = pkg_dir+"/"+sList[i];
				unlink(tmp.c_str());
			}
			return true;
		}
		else {
			if (unlink(filename.c_str())==0) return true;
			else return false;
		}
	}
	else {
		mError("No structure");
		return false;
	}
}

bool SourcePackage::removeAllPatches()
{
	if (extracted)
	{
		vector<string> pList = getEmbeddedPatchList();
		string tmp;
		for (unsigned int i=0; i<pList.size(); i++)
		{
			tmp = pkg_dir + "/patches/" + pList[i];
			unlink(tmp.c_str());
		}
		return true;
	}
	else return false;
}

bool SourcePackage::removePatch(string patch_name)
{
	if (extracted)
	{
		string n = pkg_dir + "/patches/" + patch_name;
		if (!FileExists(n)) return false;
		if (unlink(n.c_str())==0) return true;
		else return false;
	}
	else return false;
}

bool SourcePackage::setBuildScript(string script_text)
{
	if (createFolderStructure()) {
		if (script_text.empty()) {
			string n = pkg_dir + "/build_data/build.sh";
			unlink(n.c_str());
			return true;
		}

		if (WriteFile(pkg_dir+"/build_data/build.sh", script_text)==0) return true;
		else return false;
	}
	else return true;
}

bool SourcePackage::setPrebuildScript(string script_text)
{
	if (createFolderStructure()) {
		if (script_text.empty()) {
			string n = pkg_dir + "/build_data/prebuild.sh";
			unlink(n.c_str());
			return true;
		}

		if (WriteFile(pkg_dir+"/build_data/prebuild.sh", script_text)==0) return true;
		else return false;
	}
	else return true;
}

string SourcePackage::readBuildScript()
{
	return ReadFile(pkg_dir+"/build_data/build.sh");
}
string SourcePackage::readPrebuildScript()
{
	return ReadFile(pkg_dir + "/build_data/prebuild.sh");
}
string SourcePackage::getSourceDirectory()
{
	if (!extracted) {
		if (!unpackFile()) 
		{
			mError("Failed to unpack!\n");
			return "";
		}
	}
	vector<string> dir_list = getDirectoryList(pkg_dir);
	vector<string> candidates;
/*	if (dir_list.size()>4) {
		mWarning(_("Cannot determine source directory: ambiguity"));
		return "";
	}*/
	if (dir_list.size()==3) {
		mWarning("Can't find any directory!\n");
	}
	for (unsigned int i=0; i<dir_list.size(); i++) {
		if (dir_list[i]!="install" &&
				dir_list[i]!="patches" &&
				dir_list[i]!="build_data" &&
				isDirectory(pkg_dir+"/"+dir_list[i])) candidates.push_back(dir_list[i]);
	}
	if (candidates.size()==1) return candidates[0];
	else {
		say(_("Cannot determine which directory to use. Seems that you have a package without a subfolder\nCandidates are: \n"));
		for (unsigned int i=0; i<candidates.size(); i++) {
			say("  %s\n", candidates[i].c_str());
		}
		return "";
	}
}
	
bool SourcePackage::unpackFile()
{
	if (usingDirectory) return true;
	if (extracted) {
		mWarning("Already extracted\n");
		return true;
	}
	if (!fileOk()) {
		mError("Unable to extract package: cannot find or read the archive");
		return false;
	}
	// All seems to be ok?
	printf("SOURCE_PACKAGE/unpack: input_file='%s'\n", input_file.c_str());
	createWorkingDirectory();
	if (system("tar xvf " + input_file + " --same-owner -C " + pkg_dir)!=0) {
		mError("Error while extracting archive");
		return false;
	}
	extracted=true;
	return true;
}
bool SourcePackage::packFile(string output_directory, string *full_output_dir)
{
	if (usingDirectory) return true;

	printf("SOURCE_PACKAGE/pack: input_file='%s'\n", input_file.c_str());
	string oldinput = input_file;
	if (output_directory.empty())
	{
		if (input_file.empty()) {
			mError(string(__func__) + " Dunno know where to write, outdir = " + output_directory);
			return false;
		}
		else output_directory = getDirectory(input_file);
	}
	if (full_output_dir) *full_output_dir=getAbsolutePath(output_directory);
	if (system("(cd " + pkg_dir+"; buildsrcpkg " + getAbsolutePath(output_directory)+")")==0) {
		//if (!oldinput.empty()) unlink(oldinput.c_str());
		return true;
	}
	else {
		mError(string(__func__) + " Failed to build a package");
		return false;
	}
}

SourceFile::SourceFile() {}
SourceFile::~SourceFile() {}

void SourceFile::setUrl(string _url)
{
	url = _url;
}

bool SourceFile::download(bool *existFlag, bool useXterm)
{
	string wget_options;
	if (url.find("https://")==0) wget_options = " --no-check-certificate ";

	filepath = mConfig.getValue("source_cache_dir") + getFilename(url);
	if (filepath.empty()) filepath = get_tmp_file();
	if (existFlag!=NULL && FileExists(filepath + "/" + getFilename(url)) && !*existFlag) {
		*existFlag = true;
		return true;
	}
	string wget_cmd = "wget";
	if (useXterm) wget_cmd = "xterm -e " + wget_cmd;
	if (url.find("http://")==0 || url.find("https://")==0 || url.find("ftp://")==0) system("(mkdir -p " + filepath + "; cd " + filepath + "; " + wget_cmd + " " + wget_options + url + ")");
	system("(cd " + filepath + "; cp " + url + " .)");
	filepath += "/" + getFilename(url);
	return true;
}
vector<string> SourcePackage::getEmbeddedPatchList()
{
	vector<string> dlist = getDirectoryList(pkg_dir+"/patches");
	vector<string> ret;
	for (unsigned int i=0; i<dlist.size(); i++)
	{
		if (dlist[i].find(".diff.gz")!=dlist[i].length()-strlen(".diff.gz")) {
			mWarning("Unknown patch filetype: " + dlist[i]);
		}
		else ret.push_back(dlist[i]);
	}
	return ret;
}
vector<string> SourcePackage::getSourceFilenames()
{
	vector<string> dlist = getDirectoryList(pkg_dir);
	vector<string> ret;
	for (unsigned int i=0; i<dlist.size(); i++) {
		if (getExtension(dlist[i])!="gz" && 
				getExtension(dlist[i])!="bz2" &&
				getExtension(dlist[i])!="zip" &&
				getExtension(dlist[i])!="rar")
		{
			mWarning("Unknown source filetype: " + dlist[i]);
		}
		else ret.push_back(dlist[i]);
	}

	return ret;
}

bool SourceFile::analyze(string *configure_help)
{
	type = getExtension(filepath);
	string tar_args = " xf ";

	string tmp_analyze_dir = get_tmp_file();
	unlink(tmp_analyze_dir.c_str());
	system("mkdir " + tmp_analyze_dir);
	printf("filedir = %s\n", getDirectory(filepath).c_str());
	system("(cd " + tmp_analyze_dir + "; tar " + tar_args + filepath+")");
	DIR *dir = opendir(tmp_analyze_dir.c_str());
	if (!dir) return false;
	vector<string> dir_containers;
	struct dirent *dentry = readdir(dir);
	string d;
	while (dentry != NULL)
	{
		d = (string) dentry->d_name;
		if (d!="." && d!=".." && d!=getFilename(filepath)) {
			dir_containers.push_back((string)dentry->d_name);
		}
		dentry = readdir(dir);
	}
	closedir(dir);
	if (dir_containers.size()>1) {
		cout << "contains too much elements (" << dir_containers.size() << "), ambiguity\n" << endl;
	}

	if (dir_containers.size()==1) sourceDirectory = dir_containers[0];

	string sdir = tmp_analyze_dir +"/"+ sourceDirectory;
	dir = opendir(sdir.c_str());
	dir_containers.clear();
	dentry = readdir(dir);
	while (dentry!=NULL)
	{
		dir_containers.push_back((string)dentry->d_name);
		dentry = readdir(dir);

	}

	string tmp = get_tmp_file();
	buildType = BUILDTYPE_SCRIPT;
	for (unsigned int i=0; i<dir_containers.size(); i++)
	{
		if (dir_containers[i].find("CMakeLists")!=std::string::npos)
		{
			buildType = BUILDTYPE_CMAKE;
			break;
		}
		if (dir_containers[i].find("SConstruct")!=std::string::npos)
		{
			buildType = BUILDTYPE_SCONS;
			break;
		}
		if (dir_containers[i].find("configure")!=std::string::npos)
		{
			buildType = BUILDTYPE_AUTOTOOLS;
			system("(cd " + sdir + "; ./configure --help > " + tmp + " )");
			if (configure_help!=NULL) *configure_help = ReadFile(tmp);
			break;
		}
		if (dir_containers[i].find("setup.py")!=std::string::npos) {
			buildType = BUILDTYPE_PYTHON;
			break;
		}
		if (dir_containers[i].find(".pro")!=std::string::npos) {
			// Maybe, qmake one
			buildType = BUILDTYPE_QMAKE4;
		}
		if (dir_containers[i].find("Makefile")!=std::string::npos) {
			// Maybe, just make?
			buildType = BUILDTYPE_MAKE;
		}
		if (dir_containers[i].find("Makefile.PL")!=std::string::npos) {
			buildType = BUILDTYPE_PERL;
			break;
		}
		if (dir_containers[i].find("wscript")!=std::string::npos) {
			buildType = BUILDTYPE_WAF;
			break;
		}
	}
	if (configure_help && configure_help->empty()) {
		system("(cd " + sdir + " ; ls -l > " + tmp + " )");
		*configure_help = ReadFile(tmp);
	}
	//system("rm -rf " + tmp_analyze_dir);
	return true;
}
string SourceFile::getType()
{
	return type;
}

int SourceFile::getBuildType()
{
	return buildType;
}
string SourceFile::getBuildTypeS()
{
	switch(buildType)
	{
		case BUILDTYPE_SCRIPT: return "Script";
		case BUILDTYPE_AUTOTOOLS: return "Autotools";
		case BUILDTYPE_CMAKE: return "CMake";
		case BUILDTYPE_SCONS: return "SCons";
		case BUILDTYPE_CUSTOM: return "Custom";
	};
	return "Unknown";
}
string SourceFile::getSourceDirectory()
{
	return sourceDirectory;
}


