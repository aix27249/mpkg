/***********************************************************************
 * 	$Id: mpkg.cpp,v 1.132 2007/12/11 05:38:29 i27249 Exp $
 * 	MPKG packaging system
 * ********************************************************************/
// #define INSTALL_DEBUG // Enable this if you need to debug it fast

#include "mpkg.h"
#include "syscommands.h"
#include "DownloadManager.h"
#include <iostream>
#include "package.h"
#include "build.h"
#include "terminal.h"
#include "htmlcore.h"
#include <fstream>
#include <stdint.h>
#include "errorhandler.h"

// Two functions to install/remove configuration files
void pkgConfigInstall(const PACKAGE &package) {
	if (package.config_files.empty()) return;
       	
	bool sysconf_exists, orig_exists;
	string sysconf_name, orig_name, old_name;
	for (size_t i=0; i<package.config_files.size(); ++i) {
		sysconf_name = SYS_ROOT + "/" + package.config_files[i].name;
		orig_name = SYS_ROOT+"/var/mpkg/configs/" + package.get_corename() + "/" + package.config_files[i].name;
		old_name = SYS_ROOT+"/var/mpkg/configs/" + package.get_corename() + "/" + package.config_files[i].name + ".old";

		if (verbose) printf("Install: checking config file %s\n", sysconf_name.c_str());
 
		// Проверяется, есть ли уже такой конфиг в системе:
		sysconf_exists = FileExists(sysconf_name);
 
		// Проверяется, есть ли копия предыдущего оригинального конфига в var/mpkg/configs/$pkgname/$conf_path/$conf_file
		orig_exists = FileExists(orig_name);
 
		// Если системный конфиг существует, а так же существует оригинальный конфиг, и они совпадают - то старый системный конфиг перезаписывается новым:
		if ((sysconf_exists && ((orig_exists && get_file_md5(sysconf_name) == get_file_md5(orig_name)) || get_file_md5(sysconf_name)==get_file_md5(sysconf_name+".new")))|| !orig_exists) {
			system("mv " + sysconf_name + ".new " + sysconf_name);
			printf("\nUnmodified config: moving %s.new to %s\n", sysconf_name.c_str(), sysconf_name.c_str());
		}
 
		// Если системный конфиг существует, а так же существует оригинальный конфиг, но они НЕ совпадают, или же оригинального конфига нет - то:
		if ((sysconf_exists && orig_exists && get_file_md5(sysconf_name) != get_file_md5(orig_name)) || !orig_exists) {

			system("mkdir -p " + getDirectory(orig_name));
			// Если конфиг имеет флаг force_new, то старый системный конфиг копируется в var/mpkg/configs/$pkgname/$conf_path/$conf_file.old, а на его место записывается новый конфиг:
			if (package.config_files[i].hasAttribute("force_new", "true")) {

				printf("\nConfig has force_new flag: moving %s.new to %s and creating backup\n", sysconf_name.c_str(), sysconf_name.c_str());
				system("cp " + sysconf_name + " " + old_name);
				system("mv " + sysconf_name + ".new " + sysconf_name);
			}
 
			// После всего этого, в /var/mpkg/configs/$pkgname/$conf_path/$conf_file кладется копия оригинального конфига из пакета
			if (FileExists(sysconf_name + ".new")) {
				system("cp " + sysconf_name + ".new " + orig_name);
				printf("\nNEED ATTENTION: modified config detected: copying %s.new to %s, CHECK FOR CHANGES!\n", sysconf_name.c_str(), orig_name.c_str());
			}
			else {
				system("cp " + sysconf_name + " " + orig_name);
			}
		}
	}
}

void pkgConfigRemove(const PACKAGE &package) {
	// Check only one thing: if original file is the same as current one, we can remove it freely.
	if (package.config_files.empty()) return;
       	
	bool sysconf_exists, orig_exists;
	string sysconf_name, orig_name, old_name;
	for (size_t i=0; i<package.config_files.size(); ++i) {
		sysconf_name = SYS_ROOT + "/" + package.config_files[i].name;
		orig_name = SYS_ROOT+"/var/mpkg/configs/" + package.get_corename() + "/" + package.config_files[i].name;
		old_name = SYS_ROOT+"/var/mpkg/configs/" + package.get_corename() + "/" + package.config_files[i].name + ".old";
 
		// Проверяется, есть ли уже такой конфиг в системе:
		sysconf_exists = FileExists(sysconf_name);
 
		// Проверяется, есть ли копия предыдущего оригинального конфига в var/mpkg/configs/$pkgname/$conf_path/$conf_file
		orig_exists = FileExists(orig_name);

		if (sysconf_exists && orig_exists && get_file_md5(sysconf_name)==get_file_md5(orig_name)) {
			if (verbose) printf("Removing unmodified config file %s\n", sysconf_name.c_str());
			unlink(sysconf_name.c_str());
			unlink(orig_name.c_str());
		}
		else if (sysconf_exists) printf("Leaving modified config file %s in place\n", sysconf_name.c_str());
		else printf("Config file %s was wanished: perhaps it was removed by someone else.\n", sysconf_name.c_str());
	}

	
}

long double guessDeltaSize(const PACKAGE& p, const string workingDir) {
	if (p.deltaSources.empty()) return 0;
	if (_cmdOptions["enable_delta"]!="true") return 0;
	// Searching for suitable base file
	for (unsigned int i=0; i<p.deltaSources.size(); ++i) {
		if (!FileExists(workingDir + p.deltaSources[i].orig_filename)) continue;
		return strtod(p.deltaSources[i].dup_size.c_str(), NULL);
	}
	return 0;
}
string getRevision(string path, string type) {
	string tmp = get_tmp_file();
	vector<string> ret;
	if (type=="svn") {
		system(" ( cd " + path + " && svn info | grep Revi | cut -d \":\" -f2 | cut -c2- > " + tmp + " )");
		ret = ReadFileStrings(tmp);
	}
	if (type=="git") {
		system(" ( cd " + path + " && git describe | cut -f 2 -d \"-\" > " + tmp + " )");
		ret = ReadFileStrings(tmp);
	}
	unlink(tmp.c_str());
	if (!ret.empty()) return ret[0];
	else return "";
}
// Experimental function to apply patch between two txz packages
int applyXZPatch(const string& orig, const string& dest, const string& patchname, const string workingDir) {
	string tmp = get_tmp_dir();
	return system("( cd " + tmp + " || exit 1 ; xzcat " + workingDir + orig + " > " + orig + " || exit 1 ; bpatch " + orig + " " + dest + ".tar " + workingDir + patchname + " || exit 1 ; xz -zf " + dest + ".tar && mv " + dest + ".tar.xz " + workingDir + dest + " || exit 1 ; cd " + workingDir + " && rm -rf " + tmp + " )");
}

bool tryGetDelta(PACKAGE *p, const string workingDir) {
	if (setupMode) return false;
	if (_cmdOptions["deltup"]!="true") {
		//cout << _("No delta utilities\n");
		return false;
	}
	if (p->deltaSources.empty()) {
		if (verbose && !dialogMode) say(_("\n\tNo deltas found for %s\n"), p->get_filename().c_str());
		return false;
	}
	if (_cmdOptions["enable_delta"]!="true") {
		return false;
	}
	bool dupOk=false;
	int deltupRet;
	// Searching for suitable base file
	string got_md5;
	string hideInDialog;
	if (dialogMode) hideInDialog = " >/dev/null 2>/dev/null ";
	for (unsigned int i=0; i<p->deltaSources.size(); ++i) {
		if (!FileExists(workingDir + p->deltaSources[i].orig_filename)) {
			if (verbose) say(_("No original file for delta %s\n"), string(workingDir + p->deltaSources[i].orig_filename).c_str());
			continue;
		}
		if (get_file_md5(workingDir + p->deltaSources[i].orig_filename)!=p->deltaSources[i].orig_md5) {
			if (verbose) say(_("MD5 of original file doesn't match for delta %s\n"), string(workingDir + p->deltaSources[i].orig_md5).c_str());
			continue;
		}
		// If file found and md5 are correct, check if dup is already has been downloaded. Also, say what we doing now...
		pData.setItemCurrentAction(p->itemID, _("Checking package delta"));
			
		msay(_("Checking delta and trying to download it: ") + p->get_name());

		if (FileExists(workingDir + getFilename(p->deltaSources[i].dup_url))) {
			if (get_file_md5(workingDir + getFilename(p->deltaSources[i].dup_url))==p->deltaSources[i].dup_md5) {
				dupOk=true;
			}
			else unlink(string(workingDir + getFilename(p->deltaSources[i].dup_url)).c_str()); // Bad dup is so bad...
		}
		DownloadResults dres;
		if (!dupOk) {
			dres = CommonGetFile(p->deltaSources[i].dup_url, workingDir + getFilename(p->deltaSources[i].dup_url));
			if (dres != DOWNLOAD_OK) {
				printf("Failed to download delta from %s\n", p->deltaSources[i].dup_url.c_str());
				return false;
			}
		}
		// Try to merge
		unlink(string(workingDir + p->get_filename()).c_str());
		// If package type is txz, apply dirty hack (maybe it is slow, but it works)
		if (getExtension(getFilename(p->deltaSources[i].orig_filename))=="txz" && getExtension(p->get_filename())=="txz") {
			deltupRet = applyXZPatch(getFilename(p->deltaSources[i].orig_filename), p->get_filename(), getFilename(p->deltaSources[i].dup_url), workingDir);
		}
		else {
			deltupRet = system("( cd " + workingDir + " " + hideInDialog + " || exit 1 ; deltup -p -d " + workingDir + " -D " + workingDir + " " + workingDir + getFilename(p->deltaSources[i].dup_url) + hideInDialog + " || exit 1 )");
		}
		if (deltupRet==0) {
			// Check md5 of result
			got_md5 = get_file_md5(workingDir + p->get_filename());
			if (got_md5 == p->get_md5()) {
				if (!dialogMode) say(_("Merged file MD5 OK\n"));
			}
			else {
				if (!dialogMode) say(_("Merge OK, but got wrong MD5!\n"));
				return false;
			}
			if (!dialogMode) say(_("Delta patch successfully merged in %s\n"), p->get_filename().c_str());
			// Deleting delta file, because we don't need it anymore:
			unlink(string(workingDir + getFilename(p->deltaSources[i].dup_url)).c_str());
			return true;
		}
		else {
			if (!dialogMode) {
				if (i+1<p->deltaSources.size()) say(_("%s patch failed to merge, trying next one\n"), p->deltaSources[i].dup_url.c_str());
				else say(_("%s patch failed to merge, going to download full package\n"), p->deltaSources[i].dup_url.c_str());
			}
		}
	}
	return false;
}
int emerge_package(string file_url, string *package_name, string march, string mtune, string olevel, string *builddir_name_ret)
{
#ifdef NEW_SPKG_ARCH
	PackageBuilder builder (file_url);
	if (builder.build()) return 0;
	else return -1;
#else
	string default_arch="i686";
#ifdef X86_64
	default_arch="x86_64";
#endif

	// march, mtune, olevel are passed from command line options
	string PACKAGE_OUTPUT = mConfig.getValue("package_output");
	if (PACKAGE_OUTPUT.empty()) {
		mConfig.setValue("package_output", "/var/mpkg/packages");
		PACKAGE_OUTPUT = mConfig.getValue("package_output");
	}
	*package_name="";
	SourcePackage spkg;
	if (!spkg.setInputFile(file_url))
	{
		mError(_("Source build: file not found"));
		return -1;
	}
	if (!spkg.unpackFile())
	{
		mError(_("Source build: error extracting package"));
		return -2;
	}

	PackageConfig p(spkg.getDataFilename());
	if (!p.parseOk) {
		mError(_("Source build: invalid XML data"));
		return -3;
	}

	bool canCustomize = p.getBuildOptimizationCustomizable();
	bool useCflags = p.getBuildUseCflags();
	string build_system = p.getBuildSystem();
	bool noSource = false;
	// Selecting appropriate march options
	// First we should check if any management is allowed.
	if (!canCustomize && mConfig.getValue("override_buildflags")!="yes") {
		mWarning(_("Build options are strictly defined by package and cannot be overrided"));
		march = p.getBuildOptimizationMarch();
		mtune = p.getBuildOptimizationMtune();
		olevel = p.getBuildOptimizationLevel();
	}
	else {
		printf(_("Customizable config, checking....\n"));
		// 1. Command line
		// 2. System config
		// 3. Package data
		// 4. Defaults
		
		// Checking for command line, if not - replacing by system config
		if (march.empty()) {
			march = mConfig.getValue("march");
			if (!march.empty()) printf("Arch set by system config: %s\n", march.c_str());
			else mConfig.setValue("march", default_arch);
		}
		else printf("Arch set by command line: %s\n", march.c_str());

		if (mtune.empty()) mtune = mConfig.getValue("mtune");
		if (mtune.empty()) mConfig.setValue("mtune", default_arch);
		if (olevel.empty()) olevel = mConfig.getValue("olevel");
		if (olevel.empty()) mConfig.setValue("olevel", "O2");

		// Checking for system config. If not - replacing by package data
		if (march.empty()) {
			march = p.getBuildOptimizationMarch();
			if (!march.empty()) printf("Arch set by package data: %s\n", march.c_str());
		}
		
		if (mtune.empty()) mtune = p.getBuildOptimizationMtune();
		if (olevel.empty()) olevel = p.getBuildOptimizationLevel();
		// If this still don't define an arch, define it by defaults
		if (march.empty()) {
			march=default_arch;
			printf("Arch set by defaults: %s\n", march.c_str());
		}
		if (march.empty()) mtune=default_arch;
		if (march.empty()) olevel="O2";

	}

	// Backup bullshit: if absolutely no march specified, let's specify default
	if (march.empty()) march=mConfig.getValue("march");
	if (march.empty()) march=default_arch;

	string gcc_options = p.getBuildOptimizationCustomGccOptions();
	string configure_cmd = p.getBuildCmdConfigure();
	string make_cmd = p.getBuildCmdMake();
	string make_install_cmd = p.getBuildCmdMakeInstall();
	string script_cmd, pre_script_cmd;

	// Setting input filename
	
	string filename, ext, tar_args;
	string cflags;
	if (useCflags)
	{
		printf("Using CFLAGS\n");
		cflags="'";
		if (!march.empty() && default_arch != "x86_64") {
			printf("Adding march option\n");
			cflags += " -march="+march;
		}
		if (!mtune.empty() && default_arch != "x86_64") {
			printf("Adding mtune option\n");
			cflags += " -mtune=" +mtune;
		}
		if (!olevel.empty()) cflags +=" -" + olevel;
		if (!gcc_options.empty()) cflags += " " + gcc_options;
		// Add here the ones from system config file
		string systemCFlags = mConfig.getValue("cflags");
		cflags+= " " + systemCFlags + " ";
		cflags +="'";
		if (cflags.length()<4) cflags.clear();
		else cflags = "CFLAGS=" + cflags + " CXXFLAGS=" + cflags;
	}
	printf("STAGE 1: cflags = %s\n", cflags.c_str());
	cflags = p.getBuildConfigureEnvOptions() + " " + cflags;
	printf("CFLAGZ: %s\n", cflags.c_str());

	printf("Build flags:\nArchitecture: %s, tuned for: %s, optimization level: %s\n", march.c_str(), mtune.c_str(), olevel.c_str());
	
	// Download & extract
	string extractCommand, dl_command;
	string url=p.getBuildUrl();
	StringMap urlMap = p.getBuildAdvancedUrlMap();

	string dldir=spkg.getExtractedPath(); // Directory, where will be downloaded sources
	
	// Print the URL list
	say(_("Main source url: [%s]\n"), url.c_str());
	for (unsigned int i=0; i<urlMap.size(); i++) {
		if (i==0) say(_("Advanced URL list:\n"));
		say("%s\n", urlMap.getKeyName(i).c_str());
	}

	// Parsing URLs
	// First, let's deal with main URL
	if (url.empty()) {
		noSource = true;
	}
	if (url.find("cvs ")!=0 && url.find("svn ")!=0 && url.find("git")!=0 && url.find("hg ")!=0 && url.find("bzr ")!=0 && !noSource) {
		filename=getFilename(url);
		ext = getExtension(url);
	
		if (ext=="bz2" || ext == "gz" || ext == "xz" || ext == "lzma" || ext == "tgz" || ext == "tbz" || ext == "txz" || ext == "tlz") {
			tar_args="xvf";
			extractCommand = "tar " + tar_args;
		}
		else if (ext=="zip") extractCommand = "unzip";
		else if (ext=="rar") extractCommand = "unrar e";
		else {
			mError("Unknown file extension " + ext);
			return 2;
		}
	}
	
	if (url.find("cvs")==0 || url.find("svn")==0 || url.find("git")==0 || url.find("hg ")==0 || url.find("bzr ")==0) dl_command = url;
	string wget_options;
	if (url.find("https://")==0) wget_options = " --no-check-certificate ";

	if (url.find("http://")==0 || url.find("https://")==0 || url.find("ftp://")==0) dl_command = "wget " + wget_options + " '" + url + "'";
	printf("dl_command = %s\n", dl_command.c_str());
	if (dl_command.empty() && !noSource) {
		if (url.find("/")==0) {
			if (FileExists(url)) dl_command="cp -v " + url + " .";
			else {
				mError(_("Source file doesn't exist, aborting"));
				return -5;
			}
		}
		else {
			if (FileExists(dldir+"/"+url)) dl_command="";
			else {
				mError(_("Source file doesn't exist"));
				return -5;
			}
		}
	}

	// Directories
	string currentdir=get_current_dir_name();	
	string pkgdir = "/tmp/package-"+p.getName();
	// Setting source directory
	bool noSubfolder = p.getBuildNoSubfolder();
	if (noSubfolder) printf("Nosubfolder mode\n");
	// Setting output package name
	string pkgType = mConfig.getValue("build_pkg_type");
	if (pkgType.empty()) pkgType="txz"; // Changed default to txz
	string pkgVersion = p.getVersion();
	string pkgBuild = p.getBuild();
	// In case if we have some sort of SVN build... let's replace DATE by current date
	time_t timet = time(NULL);
	struct tm *nowtime = localtime(&timet);
	char *timebuff=new char[10];
	strftime(timebuff, 10, "%Y%m%d", nowtime);
	printf("%s\n", timebuff);
	strReplace(&pkgVersion, "DATE", timebuff);
	strReplace(&pkgBuild, "DATE", timebuff);
	bool fixversion = false;
	bool fixbuild = false;
	if (pkgVersion != p.getVersion()) fixversion = true;
	if (pkgBuild != p.getBuild()) fixbuild = true;

	string pkg_name = p.getName()+"-"+pkgVersion+"-"+p.getArch()+"-"+pkgBuild+"."+pkgType;

	vector<string> key_names = p.getBuildKeyNames();
	vector<string> key_values = p.getBuildKeyValues();

	int max_numjobs = atoi(p.getBuildMaxNumjobs().c_str());
	string numjobs="4";
	if (max_numjobs<4 && max_numjobs !=0) numjobs = IntToStr(max_numjobs); // Temp solution


	string configure_options;
	for (unsigned int i=0; i<key_names.size(); i++)
	{
		configure_options += " " + key_names[i];
		if (!key_values[i].empty()) {
			configure_options += "=" + key_values[i];
		}
	}
	string libsuffix, sysarch;
	if (march=="x86_64") {
		strReplace(&configure_options, "$LIBSUFFIX", "64");
		libsuffix="64";
		sysarch="x86_64";
	}
	else {
		strReplace(&configure_options, "$LIBSUFFIX", "");
		sysarch="i686";
	}
	// Preparing environment. Clearing old directories
	if (pkgdir.empty() || pkgdir.find("/tmp/package-")!=0) {
		printf("Incorrect pkgdir: %s, dangerous to continue!\n", pkgdir.c_str());
		return -11;
	}
	system("rm -rf " + pkgdir);	
	system("mkdir " + pkgdir + "; cp -R " + dldir+"/install " + pkgdir+"/");
	bool fixarch = false;
	if (p.getArch().empty()) fixarch = true;
	if (march != p.getArch()) {
	       if (p.getArch()!="noarch" && p.getArch()!="fw") fixarch=true;
	}
	if (fixarch) {
		if (march.empty()) march = default_arch;
		// Writing new XML data (fixing arch)
		string xml_path = pkgdir+"/install/data.xml";
		string xmldata = ReadFile(xml_path);
		if (xmldata.find("<arch>")==std::string::npos || xmldata.find("</arch>")==std::string::npos) {
			mError(string(__func__) + ": parse error");
			return -1;
		}
		xmldata = xmldata.substr(0,xmldata.find("<arch>")+strlen("<arch>")) + march + xmldata.substr(xmldata.find("</arch>"));
		WriteFile(xml_path, xmldata);
		xmldata.clear();
	}



	
	

	string srcCacheDir = mConfig.getValue("source_cache_dir");
	if (srcCacheDir.empty()) {
		mConfig.setValue("source_cache_dir", "/var/mpkg/source_cache/");
		srcCacheDir = mConfig.getValue("source_cache_dir");
	}

	srcCacheDir = srcCacheDir+"/" + pkg_name;
	if (dl_command.find("wget ")==0 && url.find_last_of("/")!=url.length()-1 && FileExists(dldir+"/"+getFilename(url))) {
		printf("File exists, clearing download URL\n");
		dl_command.clear();
	}
	// Check for malformed pkg_name
	if (pkg_name.empty() || pkg_name.find("..")!=std::string::npos || pkg_name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-_.+~")!=std::string::npos) {
		printf("Malformed package name %s, will not continue\n", pkg_name.c_str());
		return -12;
	}
	if (dl_command.find("wget")!=0) useBuildCache = false;
	if (!useBuildCache) {
		system("rm -rf " + srcCacheDir);
	}
	if (useBuildCache && url.find_last_of("/")!=url.length()-1 && FileExists(srcCacheDir+"/"+getFilename(url))) dl_command.clear();
	// Downloading/copying sources
	// First, download in cache.
	system("mkdir -p " + srcCacheDir);
	if (!dl_command.empty() && !noSource) {
		printf("Retrieving sources via %s\n", dl_command.c_str());
		if (system("(cd " + srcCacheDir+" && " + dl_command+")")!=0) {
			mError("Error retrieving sources, build failed");
			system("rm -rf " + srcCacheDir);
			return -6;
		}
	}
	// Fix URL for slash
	while (!url.empty() && url[url.size()-1]=='/') url.resize(url.size()-1);
	if (!noSource && !FileExists(dldir+"/"+getFilename(url))) {
		// Copy everything to build directory
		cout << "cp -R " << srcCacheDir << "/* " << dldir << "/" << endl;
		if (system("cp -R " + srcCacheDir+"/* " + dldir+"/")!=0) {
			mError("Error getting sources from cache, failure");
			return -112;
		}
	}
	else {
		if (noSource) printf("DOESNT COPYING SRC: noSource flag is set\n");
		if (FileExists(dldir+"/"+getFilename(url))) printf("DOESNT COPYING SRC: file %s exists\n", string(dldir+"/"+getFilename(url)).c_str());
	}
	string extraCmd;
	if (noSubfolder)
	{
		extraCmd = "mkdir -p extracted_source; cd extracted_source || exit 1 ; ";
		filename = "../" + filename;
	}
	// Extracting the sources, if need so
	if (!noSource && url.find("cvs ")!=0 && url.find("svn ")!=0 && url.find("git")!=0 && url.find("hg ")!=0 && url.find("bzr ")!=0) {
		string xcmd = "( cd " + dldir+" && " + extraCmd + extractCommand + " " + filename+" )";
		if (system(xcmd)!=0) {
			printf("xcmd: [%s]\n", xcmd.c_str());
			printf("dldir = %s, filename was: %s\n", dldir.c_str(), filename.c_str());
			mError(_("Tar was failed to extract the received source package"));
			return -7;
		}
	}

	string srcdir;
	if (!noSubfolder)
	{

		srcdir=p.getBuildSourceRoot(); // Trying to get source directory from config
		if (srcdir.empty()) srcdir = spkg.getSourceDirectory(); // Trying to auto-detect directory by analyzing directory tree
		if (srcdir.empty()) { // If all of above was failed, try to determine directory name by package name
			if (ext=="bz2") srcdir=dldir+"/"+filename.substr(0,filename.length()-strlen(".tar.bz2"));
			if (ext=="gz") srcdir=dldir+"/"+filename.substr(0,filename.length()-strlen(".tar.gz"));
			if (ext=="xz") srcdir=dldir+"/"+filename.substr(0,filename.length()-strlen(".tar.xz"));
			if (ext=="lzma") srcdir=dldir+"/"+filename.substr(0,filename.length()-strlen(".tar.lzma"));
			if (ext=="tgz" || ext == "txz" || ext == "tlz" || ext == "tbz") srcdir=dldir+"/"+filename.substr(0,filename.length()-strlen(".tgz"));
			if (ext=="zip") srcdir=dldir+"/"+filename.substr(0,filename.length()-strlen(".zip"));
			if (ext=="rar") srcdir=dldir+"/"+filename.substr(0,filename.length()-strlen(".rar"));
		}
		else srcdir = dldir+"/"+srcdir;
	}
	else srcdir = dldir+"/extracted_source/";

	printf("Detected srcdir: %s\n", srcdir.c_str());
	if (srcdir.empty() || srcdir.find("/tmp/mpkg-")!=0) {
		printf(_("Failed to detect source directory, cannot continue\n"));
		return -10;
	}

	string rcsRevision;
	if (url.find("svn")==0) rcsRevision=getRevision(srcdir, "svn");
	if (url.find("git")==0) rcsRevision=getRevision(srcdir, "git");
	if (!rcsRevision.empty()) {
		if (pkgVersion.find("REVISION")!=std::string::npos) {
			strReplace(&pkgVersion, "REVISION", rcsRevision);
			fixversion = true;
		}
	
		if (pkgBuild.find("REVISION")!=std::string::npos) {
			strReplace(&pkgBuild, "REVISION", rcsRevision);
			fixbuild = true;
		}
	}

	if (fixversion) {
		// Writing new XML data (fixing version)
		string xml_path = pkgdir+"/install/data.xml";
		string xmldata = ReadFile(xml_path);
		if (xmldata.find("<version>")==std::string::npos || xmldata.find("</version>")==std::string::npos) {
			mError(string(__func__) + ": parse error");
			return -1;
		}
		xmldata = xmldata.substr(0,xmldata.find("<version>")+strlen("<version>")) + pkgVersion + xmldata.substr(xmldata.find("</version>"));
		WriteFile(xml_path, xmldata);
		xmldata.clear();
	}
	if (fixbuild) {
		// Writing new XML data (fixing version)
		string xml_path = pkgdir+"/install/data.xml";
		string xmldata = ReadFile(xml_path);
		if (xmldata.find("<build>")==std::string::npos || xmldata.find("</build>")==std::string::npos) {
			mError(string(__func__) + ": parse error");
			return -1;
		}
		xmldata = xmldata.substr(0,xmldata.find("<build>")+strlen("<build>")) + pkgBuild + xmldata.substr(xmldata.find("</build>"));
		WriteFile(xml_path, xmldata);
		xmldata.clear();
	}



	if (builddir_name_ret!=NULL) *builddir_name_ret = srcdir;

	if (FileExists(dldir+"/build_data/build.sh")) {
		script_cmd = "LIBSUFFIX=" + libsuffix + " ARCH=" + sysarch + " VERSION=" + pkgVersion + " NUMJOBS=" + numjobs + " DATADIR=" + dldir+"/build_data/" + " PKG="+pkgdir + " SRC=" + srcdir + " sh " + dldir+"/build_data/build.sh " + srcdir + " " + pkgdir + " " + march + " " + mtune + " " + olevel;
	}
	if (FileExists(dldir+"/build_data/prebuild.sh")) {
		pre_script_cmd = "LIBSUFFIX=" + libsuffix + " ARCH=" + sysarch + " NUMJOBS=" + numjobs + " VERSION=" + pkgVersion + " DATADIR=" + dldir+"/build_data/" + " PKG="+pkgdir + " SRC=" + srcdir + " sh " + dldir+"/build_data/prebuild.sh " + srcdir + " " + pkgdir + " " + march + " " + mtune + " " + olevel;
	}

	if (build_system=="autotools")
	{
		configure_cmd="./configure " + configure_options;
		make_cmd = "make";
		make_install_cmd="make install DESTDIR=$DESTDIR";
	}
	if (build_system=="cmake")
	{
		configure_cmd = "cmake " + configure_options + " ..";
		make_cmd = "make";
		make_install_cmd="make install DESTDIR=$DESTDIR";
	}
	if (build_system=="scons")
	{
		configure_cmd = "scons " + configure_options;
		//make_cmd = "make";
		make_install_cmd = "scons install FAKE_ROOT=$DESTDIR";
	}
	if (build_system=="qmake4") {
		configure_cmd = "qmake *.pro " + configure_options;
		make_cmd = "make";
		make_install_cmd = "make INSTALL_ROOT=$DESTDIR install";
	}
	if (build_system=="python") {
		//configure_cmd = "python setup.py configure";
		make_install_cmd = "python setup.py install --root=$DESTDIR " + configure_options;
	}
	if (build_system=="perl") {
		configure_cmd = "perl Makefile.PL " + configure_options;
		make_cmd = "make";
		make_install_cmd = "make install DESTDIR=$DESTDIR";
	}
	if (build_system=="waf") {
		configure_cmd = "waf configure " + configure_options;
		make_cmd = "waf build";
		make_install_cmd = "waf install --destdir=$DESTDIR";
	}
	if (build_system=="make") {
		make_cmd = "make";
		make_install_cmd = "make install DESTDIR=$DESTDIR";
	}
	if (build_system=="custom")
	{
		if (configure_cmd.find("$OPTIONS")!=std::string::npos) 
			configure_cmd=configure_cmd.substr(0,configure_cmd.find("$OPTIONS"))+ " " +configure_options + " " + configure_cmd.substr(configure_cmd.find("$OPTIONS")+strlen("$OPTIONS"));
		if (configure_cmd.find("$ENV")!=std::string::npos) 
			configure_cmd=configure_cmd.substr(0,configure_cmd.find("$ENV"))+ " " +cflags + " " + configure_cmd.substr(configure_cmd.find("$ENV")+strlen("$ENV"));

		printf("Using custom commands");
	}
	if (build_system=="script")
	{
		printf("Running script\n");
		configure_cmd.clear();
		make_cmd.clear();
		make_install_cmd.clear();
	}
	if (!pre_script_cmd.empty()) configure_cmd = pre_script_cmd + " && " + configure_cmd;
	if (make_cmd.find("make")!=std::string::npos || make_cmd.find("waf build")!=std::string::npos) {
	       if (!numjobs.empty()) make_cmd += " -j" + numjobs;
	}
	

	while (make_install_cmd.find("$DESTDIR")!=std::string::npos)
	{
		make_install_cmd=make_install_cmd.substr(0,make_install_cmd.find("$DESTDIR"))+ pkgdir+ make_install_cmd.substr(make_install_cmd.find("$DESTDIR")+strlen("$DESTDIR"));
	}
	while (make_install_cmd.find("$SRCDIR")!=std::string::npos)
	{
		make_install_cmd=make_install_cmd.substr(0,make_install_cmd.find("$SRCDIR"))+ srcdir+ make_install_cmd.substr(make_install_cmd.find("$SRCDIR")+strlen("$SRCDIR"));
	}
	while (make_install_cmd.find("$DATADIR")!=std::string::npos)
	{
		make_install_cmd=make_install_cmd.substr(0,make_install_cmd.find("$DATADIR")) + dldir+"/build_data/"+ make_install_cmd.substr(make_install_cmd.find("$DATADIR")+strlen("$DATADIR"));
	}

	if (!script_cmd.empty()) {
		if (!make_install_cmd.empty()) {
			make_install_cmd += " && " + script_cmd;
		}
		else make_install_cmd = script_cmd;
	}

	//printf("\n\n\n\n\nconfigure_cmd = [%s]\n\n\n\n\n\n", configure_cmd.c_str());

	// Fixing permissions (mozgmertv resistance)
	say(_("\nChecking and fixing permissions...\n"));
	system("(cd " + srcdir+" || exit 1 ; chown -R root:root .;	find . -perm 666 -exec chmod 644 {} \\;; find . -perm 664 -exec chmod 644 {} \\;; find . -perm 600 -exec chmod 644 {} \\;; find . -perm 444 -exec chmod 644 {} \\;; find . -perm 400 -exec chmod 644 {} \\;; find . -perm 440 -exec chmod 644 {} \\;; find . -perm 777 -exec chmod 755 {} \\;; find . -perm 775 -exec chmod 755 {} \\;; find . -perm 511 -exec chmod 755 {} \\;; find . -perm 711 -exec chmod 755 {} \\;; find . -perm 555 -exec chmod 755 {} \\;)");
	

	vector<string> patchList = p.getBuildPatchList();

	string compile_cmd;
	if (build_system=="script") make_cmd = cflags + " " + make_cmd;
	bool was_prev=false;
	if (build_system!="cmake")
	{
		compile_cmd = "(cd " + srcdir + "; ";
		if (!configure_cmd.empty()) 
		{
			if (build_system!="custom") compile_cmd += cflags + " " + configure_cmd;
			else compile_cmd += " " + configure_cmd;
			was_prev=true;
		}
	}
	else {
		compile_cmd = "(cd " + srcdir + " || exit 1 ; mkdir -p build; cd build || exit 1 ; "+cflags + " " + configure_cmd + " ; ";
	}
	//printf("1. compile_cmd = %s\n", compile_cmd.c_str());
	if (make_cmd.find_first_not_of(" ")!=std::string::npos)	{
		if (was_prev) compile_cmd += " && ";
		compile_cmd += " " +make_cmd;
		was_prev=true;
	}
	//printf("2. compile_cmd = %s\n", compile_cmd.c_str());

	if (!make_install_cmd.empty()) 
	{

		if (was_prev) {
			compile_cmd += " && ";
		}
		compile_cmd += " " + make_install_cmd;
		was_prev=true;
	}
	compile_cmd+=")";
	//printf("3. compile_cmd = %s\n", compile_cmd.c_str());

	// Patching if any
	for (unsigned int i=0; i<patchList.size(); i++)
	{
		system("(cd " + srcdir + "; zcat ../patches/" + patchList[i] + " | patch -p1 --verbose)");
	}
	// Compiling
	printf("Compilation command: %s\n", compile_cmd.c_str());
	if (system(compile_cmd)!=0) {
		mError("Build failed. Check the build config");
		return -7;
	}
	// Man compression and binary stripping
	string strip_cmd = "find . | xargs file | grep \"executable\" | grep ELF | cut -f 1 -d : | xargs strip --strip-unneeded 2> /dev/null; find . | xargs file | grep \"shared object\" | grep ELF | cut -f 1 -d : | xargs strip --strip-unneeded 2> /dev/null; ";
	if (p.getValue((const char *) GET_PKG_MBUILD_NOSTRIP)=="true") {
		strip_cmd.clear();
	}
	system("( cd " + pkgdir + " || exit 1 ; " + strip_cmd + " if [ -d usr/man ]; then gzip -9 usr/man/man?/*; fi )");
	// Copy spkg inside package
	system ("mkdir -p " + pkgdir + "/usr/src/SPKG && cp " + file_url + " " + pkgdir + "/usr/src/SPKG/");
	// packing
	string keeps;
	if (_cmdOptions["keep_symlinks"]=="true") keeps = " -s ";

	if (p.getArch()=="noarch") march="noarch";
	if (p.getArch()=="fw") march="fw";
	pkg_name = p.getName()+"-"+pkgVersion+"-"+march+"-"+pkgBuild+"." + pkgType;
	*package_name=(string) PACKAGE_OUTPUT+"/"+pkg_name;
	if (autogenDepsMode == ADMODE_MOZGMERTV) {
		mpkg core;
		_cmdOptions["preserve_deps"]="yes";
		generateDeps_new(core, pkgdir);
	}

	system("(cd " + pkgdir+"; mkdir -p " + (string) PACKAGE_OUTPUT + "; buildpkg " + keeps + (string) PACKAGE_OUTPUT +"/)");
	printf("Package built as: %s\n", package_name->c_str());
	// If all OK, clean up
	spkg.clean();
	return 0;
#endif
}

mpkgDatabase::mpkgDatabase()
{
	hasFileList=false;
}
mpkgDatabase::~mpkgDatabase(){}

int mpkgDatabase::sqlFlush()
{
	return db.sqlFlush();
}

PACKAGE mpkgDatabase::get_installed_package(const string& pkg_name)
{
	PACKAGE_LIST packagelist;
	SQLRecord sqlSearch;
	sqlSearch.addField("package_name", pkg_name);
	sqlSearch.addField("package_installed", ST_INSTALLED);

	get_packagelist(sqlSearch, &packagelist);
	// We do NOT allow multiple packages with same name to be installed, so, we simply get first package of list.
	
	if (packagelist.size()>0)
		return packagelist[0];
	else
	{
		PACKAGE ret;
		return ret;
	}
}
	

int mpkgDatabase::emerge_to_db(PACKAGE *package)
{
	int pkg_id;
	pkg_id=get_package_id(*package);
	//printf("add %s: %d\n", package->get_name().c_str(), pkg_id);
	if (pkg_id==0)
	{
		// New package, adding
		add_package_record(package);
		return 0;
	}
	if (pkg_id<0)
	{
		// Query error
		return MPKGERROR_CRITICAL;
	}
	
	// Раз пакет уже в базе (и в единственном числе - а иначе и быть не должно), сравниваем данные.
	// В случае необходимости, добавляем location.
	PACKAGE db_package;
	vector<LOCATION> new_locations;
	get_package(pkg_id, &db_package);
	package->set_id(pkg_id);
	//printf("pkg has %d locations already, avail flag: %d\n", db_package.get_locations().size(), package->available());
	for (unsigned int j=0; j<package->get_locations().size(); j++)
	{
		for (unsigned int i=0; i<db_package.get_locations().size(); i++)
		{
			if (!package->get_locations().at(j).equalTo(db_package.get_locations().at(i)))
			{
				new_locations.push_back(package->get_locations().at(j));
			}
		}
	}
	if (!new_locations.empty()) add_locationlist_record(pkg_id, &new_locations);
	return 0;
}


bool mpkgDatabase::check_cache(PACKAGE *package, bool clear_wrong, bool) {
	string fname = SYS_CACHE + "/" + package->get_filename();
	//if (package->usedSource.find("cdrom://")!=std::string::npos && FileExists(fname)) return true; // WHAT THE FUCK IS THIS??!!
	string got_md5;
	bool broken_sym;
	if (FileExists(fname, &broken_sym) && !broken_sym) {
		if (forceSkipLinkMD5Checks) {
			printf("Skipping MD5 check for %s\n", package->get_name().c_str());
			return true;
		}

		pData.setItemCurrentAction(package->itemID, _("Checking MD5"));
		got_md5 = get_file_md5(SYS_CACHE + "/" + package->get_filename());
	       	if (package->get_md5() == got_md5) {
			//printf("MD5 CHECK %s: %sOK%s (%s == %s)\n", package->get_name().c_str(), CL_GREEN, CL_WHITE, package->get_md5().c_str(), got_md5.c_str());
			return true;
		}
		printf("MD5 CHECK %s: %sFAILED%s (%s != %s)\n", package->get_name().c_str(), CL_RED, CL_WHITE, package->get_md5().c_str(), got_md5.c_str());
		if (!dialogMode && !htmlMode) {
			say(_("Incorrect MD5 for package %s: received %s, but should be %s\n"), package->get_name().c_str(), got_md5.c_str(), package->get_md5().c_str());
			mpkgErrorHandler.callError(MPKG_DOWNLOAD_ERROR, _("Invalid checksum in downloaded file ") + package->get_filename());
		}
		if (clear_wrong) {
			printf("Clearing bad MD5\n");
			unlink(fname.c_str());
		}
		return false;
	}
	return false;
}

bool needUpdateXFonts = false;
vector<string> iconCacheUpdates;
vector<string> gconfSchemas, gconfSchemasUninstall;

int mpkgDatabase::commit_actions()
{
	needUpdateXFonts = false;
	delete_tmp_files();
	sqlFlush();
	// Checking for utilities
	if (!checkUtilities()) return MPKGERROR_CRITICAL;
	if (checkUtility("deltup")) _cmdOptions["deltup"]="true";

	// Zero: purging required packages
	// First: removing required packages
	unsigned int installFailures=0;
	unsigned int removeFailures=0;
	PACKAGE_LIST remove_list;
	SQLRecord sqlSearch;
	sqlSearch.setSearchMode(SEARCH_IN);
	sqlSearch.addField("package_action", ST_REMOVE);
	sqlSearch.addField("package_action", ST_PURGE);
	if (dialogMode) ncInterface.setTitle("AgiliaLinux " + (string) DISTRO_VERSION);
	if (dialogMode) ncInterface.setSubtitle(_("Preparing to package installation"));
	if (dialogMode) {
		ncInterface.setProgressText(_("Requesting list of packages marked to remove"));
		ncInterface.setProgressMax(7); //TODO: поправить в соответствие с реальным количеством шагов
		ncInterface.setProgress(1);
	}
	if (get_packagelist(sqlSearch, &remove_list)!=0) return MPKGERROR_SQLQUERYERROR;
	if (!remove_list.IsEmpty()) {
		if (dialogMode) {
			ncInterface.setProgressText(_("Sorting list of packages marked to remove"));
			ncInterface.setProgress(2);
		}
		remove_list.sortByTags(true);
	}
	PACKAGE_LIST install_list;
	sqlSearch.clear();
	sqlSearch.setSearchMode(SEARCH_IN);
	sqlSearch.addField("package_action", ST_INSTALL);
	sqlSearch.addField("package_action", ST_REPAIR);
	if (dialogMode) {
		ncInterface.setProgressText(_("Requesting list of packages marked to install"));
		ncInterface.setProgress(3);
	}
	if (get_packagelist(sqlSearch, &install_list)!=0) return MPKGERROR_SQLQUERYERROR;
	
	// Sorting install order: from lowest priority to maximum one 
	if (dialogMode) {
		ncInterface.setProgressText(_("Detecting package installation order (this may take a while)..."));
		ncInterface.setProgress(4);
	}

	install_list.sortByLocations();
	install_list.sortByTags();
	if (mConfig.getValue("old_sort").empty()) install_list.sortByPriorityNew(); // Buggy?

	// Don't forget to sort remove priority too: it does not take much time, but may be useful in case of disaster.
	if (mConfig.getValue("old_sort").empty()) remove_list.sortByPriorityNew(true); // Buggy?
	
	// Checking available space
	long double rem_size=0;
	long double ins_size=0;
	long double dl_size = 0;
	long double delta_max_size = 0;
	//Checking for removes and updates
	if (dialogMode) {
		ncInterface.setProgressText(_("Looking if an installation queue contains updates"));
		ncInterface.setProgress(5);
	}
	for (size_t i=0; i<remove_list.size(); i++)
	{
		remove_list.get_package_ptr(i)->itemID = pData.addItem(remove_list[i].get_name(), 10);
		rem_size+=strtod(remove_list[i].get_installed_size().c_str(), NULL);
		// Also, checking for update
		for (unsigned int t=0; t<install_list.size(); t++) {
			if (install_list[t].get_name() == remove_list[i].get_name()) {
				remove_list.get_package_ptr(i)->set_action(ST_UPDATE, "upgrade-" + remove_list[i].package_action_reason);
				remove_list.get_package_ptr(i)->updatingBy=install_list.get_package_ptr(t);
				install_list.get_package_ptr(t)->updatingBy = remove_list.get_package_ptr(i);
			}
		}
	}
	// From now on, all packages in remove group who will be updated, has action ST_UPDATE
	if (dialogMode) {
		ncInterface.setProgressText(_("Checking disk free space"));
		ncInterface.setProgress(6);
	}
	for (size_t i=0; i<install_list.size(); i++)
	{
		install_list.get_package_ptr(i)->itemID = pData.addItem(install_list[i].get_name(), atoi(install_list[i].get_compressed_size().c_str()));
		ins_size += strtod(install_list[i].get_installed_size().c_str(), NULL);
		dl_size += strtod(install_list[i].get_compressed_size().c_str(), NULL);
		delta_max_size += guessDeltaSize(install_list[i]);
	}
	long double freespace = get_disk_freespace(SYS_ROOT);
	
	if (freespace < (ins_size - rem_size))
	{
		if (dialogMode)
		{
			if (!ncInterface.showYesNo(_("It seems to be that disk free space is not enough to install. Required: ") + humanizeSize(ins_size - rem_size) + _(", available: ") + humanizeSize(freespace) + _("\nNote that if you splitted your filesystem tree (e.g. separate /usr, and so on), this information may be incorrect and you can safely ignore this warning.\nContinue anyway?")))
			{
				return MPKGERROR_COMMITERROR;
			}
		}
		else mWarning(_("It seems to be that disk free space is not enough to install. Required: ") + humanizeSize(ins_size - rem_size) + _(", available: ") + humanizeSize(freespace));
	}
	string branch;
	string distro;
	// Let's show the summary for console and dialog users and ask for confirmation
	if (consoleMode)
	{
		unsigned int installCount = 0, removeCount = 0, purgeCount = 0, repairCount = 0, updateCount = 0;
		string dialogMsg;
		string pkgTypeStr;
		string reason;
		//msay(_("Action summary:\n"));

		// Install
		for (unsigned int i=0; i<install_list.size(); i++) {
			if (verbose) reason = install_list[i].package_action_reason;
			branch=install_list[i].get_repository_tags();
			distro = install_list[i].package_distro_version;
			if (branch=="0") branch.clear();
			if (!branch.empty()) branch = "[" + branch + "]";
			if (install_list[i].action()==ST_INSTALL && install_list[i].updatingBy==NULL) {
				if (installCount==0) {
					if (!dialogMode) msay(_("Will be installed:\n"));
					else dialogMsg += _("Will be installed:\n");
				}
				installCount++;
				if (install_list[i].get_type()==PKGTYPE_SOURCE || _cmdOptions["abuild_links_only"]=="yes") pkgTypeStr=_("(source)");
				else pkgTypeStr=_("(binary)");
				if (!dialogMode) say("  [%d] %s %s %s %s %s\n", installCount, \
						install_list[i].get_name().c_str(), \
						install_list[i].get_fullversion().c_str(), branch.c_str(), pkgTypeStr.c_str(), reason.c_str());
				else dialogMsg += "  [" + IntToStr(installCount) + "] " + install_list[i].get_name() + " " + install_list[i].get_fullversion() + branch + "\n";
			}
		}
		// Remove
		for (unsigned int i=0; i<remove_list.size(); i++) {
			branch=remove_list[i].get_repository_tags();
			distro = remove_list[i].package_distro_version;
			if (branch=="0") branch.clear();
			if (distro == "0") distro.clear();
			if (!branch.empty()) branch = "[" + branch + "]";

			if (verbose) reason = remove_list[i].package_action_reason;
			if (remove_list[i].action()==ST_REMOVE) {
				if (removeCount==0) {
					if (!dialogMode) msay(_("Will be removed:\n"));
					else dialogMsg+=_("Will be removed:\n");
				}
				removeCount++;
				if (!dialogMode) say("  [%d] %s %s %s %s\n", removeCount, \
						remove_list[i].get_name().c_str(), \
						remove_list[i].get_fullversion().c_str(), branch.c_str(), reason.c_str());
				else dialogMsg += "  [" + IntToStr(removeCount) + "] " + remove_list[i].get_name() + " " + remove_list[i].get_fullversion() + branch + "\n";
			}
		}
		// Purge
		for (unsigned int i=0; i<remove_list.size(); i++) {
			branch=remove_list[i].get_repository_tags();
			if (branch=="0") branch.clear();
			if (distro == "0") distro.clear();
			if (!branch.empty()) branch = "[" + branch + "]";
			if (verbose) reason = remove_list[i].package_action_reason;

			if (remove_list[i].action()==ST_PURGE) {
				if (purgeCount==0) {
					if (!dialogMode) msay(_("Will be purged:\n"));
					else dialogMsg += _("Will be purged:\n");
				}
				purgeCount++;
				if (!dialogMode) say("  [%d] %s %s %s %s\n", purgeCount, \
						remove_list[i].get_name().c_str(), \
						remove_list[i].get_fullversion().c_str(), branch.c_str(), reason.c_str());
				else dialogMsg += "  [" + IntToStr(purgeCount) + "] " + remove_list[i].get_name() + " " + remove_list[i].get_fullversion() + branch + "\n";
			}
		}
		// Update
		for (unsigned int i=0; i<remove_list.size(); i++) {
			if (remove_list[i].action()==ST_UPDATE) {
				branch=remove_list[i].updatingBy->get_repository_tags();
				if (branch=="0") branch.clear();
				if (distro=="0") distro.clear();
				if (!branch.empty()) branch = "[" + branch + "]";
				if (verbose) reason = remove_list[i].package_action_reason + " => " + remove_list[i].updatingBy->package_action_reason;

				if (updateCount==0) {
					if (!dialogMode) msay(_("Will be updated:\n"));
					else dialogMsg += _("Will be updated:\n");
				}
				updateCount++;
				if (!dialogMode) say("  [%d] %s %s%s%s%s%s%s%s %s%s %s\n", updateCount, \
						remove_list[i].get_name().c_str(), \
						CL_6, remove_list[i].get_fullversion().c_str(), CL_WHITE, _(" ==> "), 
						CL_GREEN, remove_list[i].updatingBy->get_fullversion().c_str(), CL_BLUE, branch.c_str(), CL_WHITE, reason.c_str());
				else dialogMsg += "  [" + IntToStr(updateCount) + "] " + \
						remove_list[i].get_name() + " " + \
						remove_list[i].get_fullversion() + " ==> " + \
						remove_list[i].updatingBy->get_fullversion() + branch + "\n";
			}
		}
		// Repair
		for (unsigned int i=0; i<install_list.size(); i++) {
			if (install_list[i].action()==ST_REPAIR) {
				if (repairCount==0) {
					if (!dialogMode) msay(_("Will be repaired:\n"));
					else dialogMsg += _("Will be repaired:\n");
				}
				if (verbose) reason = install_list[i].package_action_reason;
				repairCount++;
				if (!dialogMode) say("  [%d] %s %s %s\n", repairCount, \
						install_list[i].get_name().c_str(), \
						install_list[i].get_fullversion().c_str(), reason.c_str());
				else dialogMsg += "  [" + IntToStr(repairCount) + "] " + install_list[i].get_name() + " " + install_list[i].get_fullversion() + "\n";
			}
		}

		if (install_list.size()>0 || remove_list.size()>0)
		{
			if (!dialogMode) say("\n");
			else dialogMsg += "\n";
			if (ins_size > rem_size) {
				if (!dialogMode) say("%s: %s\n", _("Required disk space"), humanizeSize(ins_size - rem_size).c_str());
				else dialogMsg += _("Required disk space") + (string) ": " + humanizeSize(ins_size - rem_size) + "\n";
			}
			else {
				if (!dialogMode) say("%s: %s\n", _("Disk space will be freed"), humanizeSize(rem_size - ins_size).c_str());
				else dialogMsg += _("Disk space will be freed") + (string) ": " + humanizeSize(rem_size - ins_size) + "\n";
			}
			if (!dialogMode) say("%s: %s\n", _("Approximate size of deltas which can be used"), humanizeSize(delta_max_size).c_str());
			else dialogMsg += _("Approximate size of deltas which can be used:") + (string) ": " + humanizeSize(delta_max_size);

			if (!dialogMode) say("%s: %s\n", _("Maximum download size"), humanizeSize(dl_size).c_str());
			else dialogMsg += _("Maximum download size") + (string) ": " + humanizeSize(dl_size);

			if (interactive_mode && !dialogMode && !getlinksOnly)
			{
				say("\n");
				say(_("Continue? [Y/n]\n"));
				string input;
				while (input!="y" && input!="Y" && input!="yes" && input!="\n") {
					input.clear();
					input=cin.get();
					if (input=="n" || input=="N" || input == "no") return MPKGERROR_ABORTED;
					if (input!="y" && input!="Y" && input!="yes" && input!="\n") {
						say(_("Please answer Y (yes) or N (no)\n"));
						cin.get();
					}
				}
			}
			else if (dialogMode && !setupMode) {
				if (!ncInterface.showText(dialogMsg, _("Continue"), _("Cancel"))) return MPKGERROR_ABORTED;
			}
		}
		else 
		{
			if (!dialogMode) say (_("Nothing to do\n"));
			else ncInterface.showMsgBox(_("Nothing to do\n"));
			return 0;
		}

	} // if (consoleMode && !dialogMode)
	if (getlinksOnly) {
		vector<string> urls;
		for (unsigned int i=0; i<install_list.size(); ++i) {
			urls.push_back(install_list[i].get_locations().at(0).get_full_url() + install_list[i].get_filename());
			printf("%s\n", urls[i].c_str());
		}
		return 0;
	}

	if (_cmdOptions["abuild_links_only"]=="yes") {
		printf("Source URL requested, here it is:\n");
		vector<string> urls;
		for (size_t i=0; i<install_list.size(); ++i) {
			urls.push_back(install_list[i].abuild_url.c_str());
			printf("%s\n", install_list[i].abuild_url.c_str());
		}
		if (!_cmdOptions["abuild_links_output"].empty()) {
			WriteFileStrings(_cmdOptions["abuild_links_output"], urls);
		}
		return 0;
	}

	
	// Building action list
	actionBus.clear();
	if (install_list.size()>0)
	{
		actionBus.addAction(ACTIONID_CACHECHECK);
		actionBus.setSkippable(ACTIONID_CACHECHECK, true);
		actionBus.setActionProgressMaximum(ACTIONID_CACHECHECK, install_list.size());
		actionBus.addAction(ACTIONID_DOWNLOAD);
		actionBus.setActionProgressMaximum(ACTIONID_DOWNLOAD, install_list.size());
		actionBus.addAction(ACTIONID_MD5CHECK);
		actionBus.setSkippable(ACTIONID_MD5CHECK, true);
		actionBus.setActionProgressMaximum(ACTIONID_MD5CHECK, install_list.size());
	}
	if (remove_list.size()>0)
	{
		actionBus.addAction(ACTIONID_REMOVE);
		actionBus.setActionProgressMaximum(ACTIONID_REMOVE, remove_list.size());
	}
	if (install_list.size()>0)
	{
		actionBus.addAction(ACTIONID_INSTALL);
		actionBus.setActionProgressMaximum(ACTIONID_INSTALL, install_list.size());

	}
	// Done

	msay(_("Looking for install queue"));
	vector<bool> needFullDownload(install_list.size());
	if (install_list.size()>0)
	{
		actionBus.setCurrentAction(ACTIONID_CACHECHECK);
		// Building download queue
		msay(_("Looking for package locations"));
		DownloadsList downloadQueue;
		DownloadItem tmpDownloadItem;
		vector<string> itemLocations;
		pData.resetItems(_("waiting"), 0, 1, ITEMSTATE_WAIT);
		pData.setCurrentAction(_("Checking cache"));
		bool skip=false;
		if (dialogMode) ncInterface.setProgressMax(install_list.size());
		for (unsigned int i=0; i<install_list.size(); i++)
		{
			needFullDownload[i]=false;
			if (dialogMode)
			{
				ncInterface.setProgressText("["+IntToStr(i+1)+"/"+IntToStr(install_list.size())+_("] Checking package cache and building installation queue: ") + install_list[i].get_name());
				ncInterface.setProgress(i);
			}
			delete_tmp_files();

			actionBus.setActionProgress(ACTIONID_CACHECHECK, i);
			if (actionBus._abortActions)
			{
				sqlFlush();
				actionBus._abortComplete=true;
				return MPKGERROR_ABORTED;
			}
			if (actionBus.skipped(ACTIONID_CACHECHECK))
			{
				skip=true;
			}

			// Clear broken symlinks
			//clean_cache_symlinks();
			msay(_("Checking cache and building download queue: ") + install_list[i].get_name());
	
	
			if (skip || !check_cache(install_list.get_package_ptr(i), false)) {
				needFullDownload[i]=!tryGetDelta(install_list.get_package_ptr(i));
			}
			if (needFullDownload[i]) {

				itemLocations.clear();
				
				tmpDownloadItem.expectedSize=strtod(install_list[i].get_compressed_size().c_str(), NULL);
				tmpDownloadItem.file = SYS_CACHE + install_list[i].get_filename();
				tmpDownloadItem.name = install_list[i].get_name();
				tmpDownloadItem.priority = 0;
				tmpDownloadItem.status = DL_STATUS_WAIT;
				tmpDownloadItem.itemID = install_list[i].itemID;
				tmpDownloadItem.usedSource = (string *) &install_list[i].usedSource;
	
				install_list.get_package_ptr(i)->sortLocations();
				for (unsigned int k = 0; k < install_list[i].get_locations().size(); k++) {
					itemLocations.push_back(install_list[i].get_locations().at(k).get_server_url() \
						     + install_list[i].get_locations().at(k).get_path() \
						     + install_list[i].get_filename());
	
				}
				tmpDownloadItem.url_list = itemLocations;
				downloadQueue.push_back(tmpDownloadItem);
			}
	
			if (!setupMode) {
				pData.increaseItemProgress(install_list[i].itemID);
				pData.setItemState(install_list[i].itemID, ITEMSTATE_FINISHED);
			}
		}
		ncInterface.setProgress(0);
		actionBus.setActionState(ACTIONID_CACHECHECK);
		actionBus.setCurrentAction(ACTIONID_DOWNLOAD);
		bool do_download = true;
	
		pData.resetItems(_("waiting"), 0, 1, ITEMSTATE_WAIT);
download_process:
		// DOWNLOAD SECTION
		while(do_download)
		{
			do_download = false;

			if (CommonGetFileEx(downloadQueue, &currentItem) == DOWNLOAD_ERROR)
			{
				mError(_("Failed to download one or more files, process stopped"));
				if (!actionBus._abortActions) {
					return MPKGERROR_ABORTED;
				}
					
			}
		
		}
		actionBus.setActionState(ACTIONID_DOWNLOAD);
		pData.downloadAction=false;
		if (download_only) {
			say(_("Downloaded packages are stored in %s\n"), SYS_CACHE.c_str());
			mpkgSys::clean_queue(this);
			return 0;
		}
// installProcess:
	
		actionBus.setCurrentAction(ACTIONID_MD5CHECK);
		pData.resetItems(_("waiting"), 0, 1, ITEMSTATE_WAIT);
	
		skip=false;
		
		msay(_("Checking files (comparing MD5):"));
		pData.setCurrentAction(_("Checking md5"));
		if (dialogMode) {
			ncInterface.setProgressText(_("Checking packages integrity"));
			ncInterface.setProgress(0);
		}
		if (dialogMode) ncInterface.setProgressMax(install_list.size());
		for(size_t i=0; i<install_list.size(); i++)
		{
			actionBus.setActionProgress(ACTIONID_MD5CHECK, i);
			if (actionBus._abortActions)
			{
				sqlFlush();
				actionBus._abortComplete=true;
				actionBus.setActionState(ACTIONID_MD5CHECK, ITEMSTATE_ABORTED);
				return MPKGERROR_ABORTED;
			}
			if (actionBus.skipped(ACTIONID_MD5CHECK) || forceSkipLinkMD5Checks) break;

			if (dialogMode)
			{
				ncInterface.setProgressText(_("Checking packages integrity: ") + install_list[i].get_name());
				ncInterface.setProgress((unsigned int) round(((double)(i)/(double) ((double) (install_list.size())/(double) (100)))));

			}
			msay(_("Checking md5 of downloaded files: ") + install_list[i].get_name());
	
			if (!check_cache(install_list.get_package_ptr(i), false, false))
			{
				pData.setItemState(install_list[i].itemID, ITEMSTATE_FAILED);
	
				MpkgErrorReturn errRet = mpkgErrorHandler.callError(MPKG_DOWNLOAD_ERROR, _("Invalid checksum in downloaded file"));
				switch(errRet)
				{
					case MPKG_RETURN_IGNORE:
						say(_("Wrong checksum ignored, continuing...\n"));
						break;
					case MPKG_RETURN_RETRY:
						say(_("Re-downloading...\n")); // TODO: HEAVY CHECK!!!!
						do_download = true;
						goto download_process;
						break;
					case MPKG_RETURN_ABORT:
						say(_("Aborting installation\n"));
						return MPKGERROR_ABORTED;
						break;
					default:
						mError(_("Unknown reply, aborting"));
						return MPKGERROR_ABORTED;
						break;
				}
			}
			else if (!setupMode) pData.increaseItemProgress(install_list[i].itemID);
		}
		if (dialogMode) {
			ncInterface.setProgress(0);
			//dialogItem.closeGauge();
		}
		actionBus.setActionState(ACTIONID_MD5CHECK);
	}

	if (remove_list.size()>0)
	{
		actionBus.setCurrentAction(ACTIONID_REMOVE);

		msay(_("Looking for remove queue"));
		msay(_("Removing ") + IntToStr(remove_list.size()) + _(" packages"));
		pData.setCurrentAction(_("Removing packages"));
	
		int removeItemID=0;
		for(unsigned int i=0; i<remove_list.size(); i++)
		{
			removeItemID=remove_list[i].itemID;
			pData.setItemState(removeItemID, ITEMSTATE_WAIT);
			pData.setItemProgress(removeItemID, 0);
			pData.setItemProgressMaximum(removeItemID,8);
		}
		string _actionName;
		ncInterface.setProgressMax(remove_list.size());
		for(unsigned int i=0;i<remove_list.size();i++)
		{
			if (remove_list[i].action()==ST_UPDATE) {
				_actionName = _("Updating package");
				if (dialogMode) ncInterface.setSubtitle(_("Updating packages"));
			}
			else {
				_actionName = _("Removing package");
				if (dialogMode) ncInterface.setSubtitle(_("Removing packages"));
			}
			
			if (dialogMode) {
				ncInterface.setProgressText("[" + IntToStr(i+1) + "/" + IntToStr(remove_list.size()) + "] " + _actionName + " " + \
						remove_list[i].get_name() + "-" + \
						remove_list[i].get_fullversion());
			       	ncInterface.setProgress((unsigned int) round((double)(i)/(double)((double)(remove_list.size())/(double) (100))));
			}
			delete_tmp_files();
			actionBus.setActionProgress(ACTIONID_REMOVE,i);
			if (actionBus._abortActions) {
				sqlFlush();
				actionBus._abortComplete=true;
				actionBus.setActionState(ACTIONID_REMOVE, ITEMSTATE_ABORTED);
				return MPKGERROR_ABORTED;
			}
			pData.setItemState(remove_list[i].itemID, ITEMSTATE_INPROGRESS);

			msay(_actionName+" " + remove_list[i].get_name());

			if (remove_package(remove_list.get_package_ptr(i))!=0) {
				removeFailures++;
				pData.setItemCurrentAction(remove_list[i].itemID, _("Remove failed"));
				pData.setItemState(remove_list[i].itemID, ITEMSTATE_FAILED);
			}
			else {
				pData.setItemCurrentAction(remove_list[i].itemID, _("Removed"));
				pData.setItemState(remove_list[i].itemID, ITEMSTATE_FINISHED);
			}
		}
		sqlSearch.clear();

		clean_backup_directory();
		actionBus.setActionState(ACTIONID_REMOVE);
	} // Done removing packages

	if (install_list.size()>0)
	{	
		// Actually installing

		pData.setCurrentAction(_("Installing packages"));
		uint64_t sz = 0;
		for(size_t i=0; i<install_list.size(); i++) {
			sz += (uint64_t) atol(install_list[i].get_installed_size().c_str());
		}
		actionBus.setCurrentAction(ACTIONID_INSTALL);
		pData.resetItems(_("waiting"), 0, 8, ITEMSTATE_WAIT);
	

		if (dialogMode) {
			ncInterface.setSubtitle(_("Installing packages"));
			ncInterface.setProgressMax(install_list.size());
		}
		vector<time_t> pkgInstallTime;
		vector<int64_t> pkgInstallSize;
		time_t pkgInstallStartTime=0, pkgInstallEndTime=0, pkgTotalInstallTime=0;
		int64_t pkgInstallCurrentSize=0;
		long double pkgInstallSpeed=0;
		time_t ETA_Time = 0;
		MpkgErrorCode install_result;
		for (size_t i=0;i<install_list.size(); ++i) {

			pkgInstallStartTime=time(NULL); // TIMER 1: mark package installation start
			
			actionBus.setActionProgress(ACTIONID_INSTALL, i);
			if (actionBus._abortActions)
			{
				sqlFlush();
				actionBus._abortComplete=true;
				actionBus.setActionState(ACTIONID_INSTALL, ITEMSTATE_ABORTED);
				return MPKGERROR_ABORTED;
			}
			pData.setItemCurrentAction(install_list[i].itemID, string("installing [") + humanizeSize(IntToStr(pkgInstallSpeed)) + _("/s, ETA: ") + IntToStr(ETA_Time/60) + _(" min") + string("]"));
			pData.setItemState(install_list[i].itemID, ITEMSTATE_INPROGRESS);
			msay(_("Installing package ") + install_list[i].get_name());

			if (dialogMode)
			{
				if (pkgInstallTime.size()>1 && pkgInstallSpeed!=0) {
					ncInterface.setSubtitle(_("Installing packages") + string(" [") + humanizeSize(IntToStr(pkgInstallSpeed)) + _("/s, ETA: ") + IntToStr(ETA_Time/60) + _(" min") + string("]"));
				}
				ncInterface.setProgressText("[" + IntToStr(i+1) + "/" + IntToStr(install_list.size()) + _("] Installing: ") + \
						install_list[i].get_name() + "-" + \
						install_list[i].get_fullversion());
				ncInterface.setProgress(i+1);
				//csz += atol(install_list[i].get_installed_size().c_str());
				//ncInterface.setProgress(csz);
			}
			install_result = (MpkgErrorCode) install_package(install_list.get_package_ptr(i),i,install_list.size());
			if (install_result!=0)
			{
				mpkgErrorHandler.callError(install_result, _("Failed to install package ") + install_list[i].get_name() + " " + install_list[i].get_fullversion());
				installFailures++;
				pData.setItemCurrentAction(install_list[i].itemID, _("Installation failed"));
				pData.setItemState(install_list[i].itemID, ITEMSTATE_FAILED);
				
				// change in 0.12.9: will stop installation if previous brokes...
				mError(_("Failed to install package ") + install_list[i].get_name() + _(". Due to possible dependency issues, the installation procedure will stop now."));
				return MPKGERROR_COMMITERROR;
			}
			else
			{
				if (install_list[i].get_name()=="aaa_elflibs" || install_list[i].get_name().find("glibc")==0) {
					fillEssentialFiles(true); // Force update of essential files
					if (verbose && !dialogMode) printf("glibc update mode\n");
				}
				//pData.setItemCurrentAction(install_list[i].itemID, _("Installed"));
				pData.setItemState(install_list[i].itemID, ITEMSTATE_FINISHED);
			}
			pkgInstallEndTime=time(NULL); // TIMER 2: Mark end of package installation
			pkgInstallTime.push_back(pkgInstallEndTime-pkgInstallStartTime); // Store data
			pkgInstallSize.push_back(atoi(install_list[i].get_installed_size().c_str())); // Store more data :)
			
			// Approximate by total stats time, not so good
			pkgTotalInstallTime+=(pkgInstallEndTime-pkgInstallStartTime); // Adding summary time
			pkgInstallCurrentSize += (uint64_t) atol(install_list[i].get_installed_size().c_str()); // Adding completed size;
		
			if (pkgTotalInstallTime!=0) pkgInstallSpeed=(long double) pkgInstallCurrentSize/(long double) pkgTotalInstallTime; // Update pkgInstal speed
			ETA_Time = (time_t) ((long double) (sz-pkgInstallCurrentSize)/pkgInstallSpeed);


		}
		msay(_("Installation complete."), SAYMODE_INLINE_END);
		actionBus.setActionState(ACTIONID_INSTALL);
	}
	if (removeFailures!=0 && installFailures!=0) return MPKGERROR_COMMITERROR;
	actionBus.clear();
	sqlFlush(); // Teh fuckin' trouble place
	
	// NEW 25.08.08: cleanup db for local packages
	msay(_("Clearing unreachable packages"), SAYMODE_NEWLINE);
	clear_unreachable_packages();
	
	if (install_list.size()>0 || remove_list.size()>0) 
	{
		msay(_("Executing ldconfig"), SAYMODE_NEWLINE);
		if (FileExists("/usr/sbin/prelink") && mConfig.getValue("enable_prelink")=="yes") {
			system("/sbin/ldconfig 2> /dev/null");
			if (mConfig.getValue("enable_prelink_randomization")=="yes") {
				msay(_("Prelinking with randomization..."), SAYMODE_NEWLINE);
				system("/usr/sbin/prelink -amqR 2>/dev/null");
			}
			else {
				msay(_("Prelinking..."), SAYMODE_NEWLINE);
				system("/usr/sbin/prelink -amq 2>/dev/null");
			}

		}
		else system("/sbin/ldconfig 2> /dev/null"); // I prefer to update ldconfig in real time, seems that delayed jobs works bad.
		if (needUpdateXFonts) {
			msay(_("Updating font indexes"), SAYMODE_NEWLINE);
			system("chroot " + SYS_ROOT + " find /usr/share/fonts -type d -exec /usr/bin/mkfontdir {} \\; ");
			system("chroot " + SYS_ROOT + " find /usr/share/fonts -type d -exec /usr/bin/mkfontscale {} \\; ");
			system("chroot " + SYS_ROOT + " /usr/bin/fc-cache -f");
		}
		printf("Total icon paths to update: %d\n", (int) iconCacheUpdates.size());
		for (size_t i=0; i<iconCacheUpdates.size(); ++i) {
			printf(_("[%d/%d] Updating icon cache in %s\n"), (int) i+1, (int) iconCacheUpdates.size(), iconCacheUpdates[i].c_str());
			system("chroot " + SYS_ROOT + " /usr/bin/gtk-update-icon-cache -t -f " + iconCacheUpdates[i] + " 1> /dev/null 2> /dev/null");
		}
		printf("Total GConf schemas to remove: %d\n", (int) gconfSchemasUninstall.size());
		for (size_t i=0; i<gconfSchemasUninstall.size(); ++i) {
			printf(_("[%d/%d] Removing GConf schema %s\n"), (int) i+1, (int) gconfSchemasUninstall.size(), gconfSchemasUninstall[i].c_str());
			system("chroot " + SYS_ROOT + " /usr/sbin/gconfpkg --uninstall " + gconfSchemasUninstall[i] + " 1> /dev/null 2> /dev/null");
		}
	
		printf("Total GConf schemas to install: %d\n", (int) gconfSchemas.size());
		for (size_t i=0; i<gconfSchemas.size(); ++i) {
			printf(_("[%d/%d] Installing GConf schema %s\n"), (int) i+1, (int) gconfSchemas.size(), gconfSchemas[i].c_str());
			system("chroot " + SYS_ROOT + " /usr/sbin/gconfpkg --install " + gconfSchemas[i] + " 1> /dev/null 2> /dev/null");
		}

		// Always update mime database, it takes not much time but prevents lots of troubles
		msay(_("Updating icon cache and mime database"), SAYMODE_NEWLINE);
		system("chroot " + SYS_ROOT + " /usr/bin/update-all-caches >/dev/null");
		
		// Cleanup
		system("rm -rf " + SYS_ROOT + "/install");


		msay(_("Syncing disks..."), SAYMODE_NEWLINE);
		system("sync &");
	}
	return 0;
}

int mpkgDatabase::install_package(PACKAGE* package, unsigned int packageNum, unsigned int packagesTotal)
{
	bool ultraFastMode = true;
#ifndef INSTALL_DEBUG
	if (setupMode) {
		system("echo Installing package " + package->get_name() + "-" + package->get_fullversion() + " >> /dev/tty4");
	}
	system("echo Installing package " + package->get_name() + "-" + package->get_fullversion() + " >> /var/log/mpkg-installation.log");
#endif
	
	// Check if package already has been installed
	if (package->action()==ST_NONE) return 0;
	string sys_cache=SYS_CACHE;
	string sys_root=SYS_ROOT;
	string index_str, index_hole;
	if (packagesTotal>0) {
		index_str = "[" + IntToStr(packageNum+1) + "/"+IntToStr(packagesTotal)+"] ";
		for (unsigned int i=0; i<utf8strlen(index_str); i++) {
			index_hole += " ";
		}
	}
	//if (!setupMode) pData.setItemCurrentAction(package->itemID, _("installing"));
	msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": initialization"), SAYMODE_INLINE_START);
	string statusHeader = "["+IntToStr((int) actionBus.progress())+"/"+IntToStr((int)actionBus.progressMaximum())+"] "+_("Installing package ") + package->get_name()+": ";
	currentStatus = statusHeader + _("initialization");
	
	// Checking if it is a symlink. If it is broken, and package installs from CD, ask to insert and mount
	//bool broken_sym=false;
	msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": checking package source"));

	// Если ставим с CD/DVD, попытаемся молча смонтировать диск.
	// UPD: а зачем на этом этапе это делать, если диск либо уже вставлен, либо уже не вставлен? о_О
	
	
	/***************** BEGINNING OF WEIRD CODE *******************************
	
	if (package->usedSource.find("cdrom://")!=std::string::npos) {
		if (!FileExists(sys_cache + package->get_filename(), &broken_sym) || broken_sym) {
			system("mount " + CDROM_DEVICE + " " + CDROM_MOUNTPOINT + " 2>/dev/null >/dev/null");
		}
	}
	broken_sym = false;

	if (!FileExists(sys_cache + package->get_filename(), &broken_sym) || broken_sym) // If file not found
	{
		//printf("Link is broken. Looking for the package source\n");
		// Let's see what source is used
		if (package->usedSource.find("cdrom://")!=std::string::npos)
		{
			msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": mounting CD-ROM"));

			// Yeah, we used CD and there are symlink. Let's ask for appropriate disc
			// First, determine Volume ID
			string source;
		        source = package->usedSource.substr(strlen("cdrom://"));
			//printf("source created\n");
			string cdromVolName;
		        cdromVolName = source.substr(0,source.find_first_of("/"));
			//printf("cdromVolName created\n");
			bool mountedOk=false, abortMount=false;
			string recv_volname;
			while (!mountedOk)
			{
				//printf("Trying to mount\n");
				if (dialogMode) 
				{
					string oldtitle = ncInterface.subtitle;
					ncInterface.setSubtitle(_("Installing package ") + package->get_name() + _(": mounting CD/DVD..."));
					//printf("Yes, we are installing package from CD. Ejecting, mounting and checking the volume\n");
					if (!noEject) {
						system("eject " + CDROM_DEVICE + " 2>/dev/null >/dev/null");
					}

					if (ncInterface.showYesNo(_("Please insert DVD with label ") + cdromVolName + _(" in drive ") + CDROM_DEVICE, "OK", "Cancel"))
					{
						system("umount " + CDROM_MOUNTPOINT + " 2>/dev/null >/dev/null");
						system("mount " + CDROM_DEVICE + " " + CDROM_MOUNTPOINT + " 2>/dev/null >/dev/null");
						recv_volname = getCdromVolname();
						if (recv_volname == cdromVolName)
						{
							mountedOk = true;
						}
						else {
							if (noEject) system("umount " + CDROM_MOUNTPOINT + " 2>/dev/null >/dev/null");
							else {
								system("eject " + CDROM_DEVICE + " 2>/dev/null >/dev/null");
								if (setupMode) system("echo EJECTING mpkg:1182 > /dev/tty4");
							}
							ncInterface.showMsgBox(_("You have inserted a wrong disk.\nRequired: [")+cdromVolName+_("]\nInserted: [") + recv_volname + "]\n");
						}
					}
					else abortMount = true;
					ncInterface.setSubtitle(oldtitle);
				}
				else {
					if (mpkgErrorHandler.callError(MPKG_CDROM_MOUNT_ERROR)==MPKG_RETURN_ABORT) abortMount=true;
				}
				if (abortMount) mountedOk=true;
			}
			if (abortMount) return MPKGERROR_ABORTED;
		}
		else {
			mError(_("Installation error: file not found"));
			mError(_("Filename was: ") + SYS_CACHE + package->get_filename());
			return MPKG_INSTALL_EXTRACT_ERROR;
		}
	}*/

	// NEW (04.10.2007): Check if package is source, and build if needed. Also, import to database the output binary package and prepare to install it.
	msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() +_( ": checking package type"));

	if (package->get_type()==PKGTYPE_SOURCE)
	{
		msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": building from source"));

		say(_("Package %s is source-based. Building...\n"), package->get_name().c_str());
		string binary_out;
		if (emerge_package(SYS_CACHE + package->get_filename(), &binary_out)!=0) {
			mError("Failed to build. Aborting...");
			return MPKG_INSTALL_META_ERROR;
		}
		say(_("Package was built. Filename: %s\n"), binary_out.c_str());
		// Now we have a new binary package with filename stored in variable binary_out. Import him into database and create a link to cache.
		
		if (!copyFile(binary_out, SYS_CACHE + getFilename(binary_out))) {
			mError("Error copying package, aborting...");
			return MPKG_INSTALL_META_ERROR;
		}
		say(_("Importing to database\n"));
		LocalPackage binpkg(SYS_CACHE + getFilename(binary_out));
		if (binpkg.injectFile()!=0) {
			mError("Error injecting binary package, cannot continue");
			return MPKG_INSTALL_META_ERROR;
		}
		PACKAGE binary_package = binpkg.data;
		binary_package.set_action(ST_INSTALL, "new-bin");
		emerge_to_db(&binary_package);
		binary_package.itemID = package->itemID;
		// Now replace record in install_list:
		*package = binary_package;
		say(_("Processing to install binary\n"));
		return install_package(package, packageNum, packagesTotal );
	}

	// First of all: EXTRACT file list and scripts!!!
	LocalPackage lp(SYS_CACHE + package->get_filename());
	if (forceInInstallMD5Check) {
		if (dialogMode) ncInterface.setProgressText(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + "\n" + index_hole + _("checking package integrity"));
		msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": checking package integrity"));

		string md5 = get_file_md5(SYS_CACHE + package->get_filename());
		if (package->get_md5() != md5) {
			mError(_("Error while installing package ") + package->get_name() + _(": MD5 checksum incorrect: got ") + md5 + _(", should be: ") + package->get_md5());
			return MPKG_INSTALL_META_ERROR;
		}
	}

	if (dialogMode) ncInterface.setProgressText(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + "\n" + index_hole + _("extracting metadata"));
	msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": extracting metadata"));


	int purge_id=0;
	if (actionBus._abortActions)
	{
		sqlFlush();
		actionBus._abortComplete=true;
		actionBus.setActionState(ACTIONID_INSTALL, ITEMSTATE_ABORTED);
		return MPKGERROR_ABORTED;
	}
	
	pData.increaseItemProgress(package->itemID);
	if (!ultraFastMode) {
		if (dialogMode) ncInterface.setProgressText(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + "\n" + index_hole + _("extracting scripts")); 
		else msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": extracting scripts"));
		lp.fill_scripts(package);  // ultrafast mode: disabling scripts other than doinst.sh
	}

	currentStatus = statusHeader + _("extracting file list");
	printHtmlProgress();
	if (dialogMode) ncInterface.setProgressText(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + "\n" + index_hole + _("extracting file list"));
	else msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": extracting file list"));
	pData.increaseItemProgress(package->itemID);
	if (actionBus._abortActions)
	{
		sqlFlush();
		actionBus._abortComplete=true;
		actionBus.setActionState(ACTIONID_INSTALL, ITEMSTATE_ABORTED);
		return MPKGERROR_ABORTED;
	}
#ifndef INSTALL_DEBUG
	if (package->get_files().empty()) lp.fill_filelist(package); // Extracting file list
#endif
	
	if (!needUpdateXFonts) {
		msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": looking for X fonts"));

		for (size_t i=0; !needUpdateXFonts && i<package->get_files().size(); i++) {
			if (package->get_files().at(i).find("usr/share/fonts")!=std::string::npos) needUpdateXFonts = true;
		}
	}
	// Searching for icon cache updates
	bool hasIconCache = false;
	string *iconFilename;
	string iconDir;
	for (size_t i=0; i<package->get_files().size(); ++i) {
		iconFilename = (string *) &package->get_files().at(i);
		if (iconFilename->find("usr/share/icons/")!=std::string::npos && iconFilename->size()>strlen("usr/share/icons/") && iconFilename->at(iconFilename->size()-1)=='/') {
			hasIconCache=false;
			iconDir = iconFilename->substr(strlen("usr/share/icons/"));
			if (iconDir.find_first_of("/")!=std::string::npos) iconDir = iconDir.substr(0, iconDir.find_first_of("/"));
			iconDir = "usr/share/icons/" + iconDir;
			for (size_t t=0; !hasIconCache && t<iconCacheUpdates.size(); ++t) {
				if (iconCacheUpdates[t]==iconDir) hasIconCache = true;
			}
			if (!hasIconCache) iconCacheUpdates.push_back(iconDir);
		}
	}

	// Searching for gconf schemas
	bool hasGConfSchema = false;
	string *gconfFilename;
	string gconfSchemaName;
	for (size_t i=0; i<package->get_files().size(); ++i) {
		gconfFilename = (string *) &package->get_files().at(i);
		if (gconfFilename->find("usr/share/gconf/schemas/")!=std::string::npos && gconfFilename->size()>strlen("usr/share/gconf/schemas/") && getExtension(*gconfFilename)=="schemas") {
			hasGConfSchema=false;
			gconfSchemaName = gconfFilename->substr(strlen("usr/share/gconf/schemas/"));
			gconfSchemaName = gconfSchemaName.substr(0, gconfSchemaName.size()-strlen(".schemas"));
			for (size_t t=0; !hasGConfSchema && t<gconfSchemas.size(); ++t) {
				if (gconfSchemas[t]==gconfSchemaName) hasGConfSchema = true;
			}
			if (!hasGConfSchema) gconfSchemas.push_back(gconfSchemaName);
		}
	}

	msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": checking file conflicts"));
	if (dialogMode) ncInterface.setProgressText(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() +"\n" + index_hole + _("checking file conflicts"));
	if (fileConflictChecking==CHECKFILES_PREINSTALL) 
	{

		currentStatus = statusHeader + _("checking file conflicts");
		printHtmlProgress();
		pData.increaseItemProgress(package->itemID);

	}
#ifdef INSTALL_DEBUG
	force_skip_conflictcheck=true;
#endif
	if (!force_skip_conflictcheck)
	{
		if (fileConflictChecking == CHECKFILES_PREINSTALL && check_file_conflicts_new(*package)!=0)
		{
			currentStatus = _("Error: Unresolved file conflict on package ") + package->get_name();
			mError(_("Unresolved file conflict on package ") + package->get_name() + _(", it will be skipped!"));
			return MPKG_INSTALL_FILE_CONFLICT;
		}
	}
	
	currentStatus = statusHeader + _("installing...");
	pData.increaseItemProgress(package->itemID);

	printHtmlProgress();

// Filtering file list...

	if (dialogMode) ncInterface.setProgressText(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() +"\n" + index_hole + _("merging file lists into database"));
	msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": merging file lists into database"));
#ifndef INSTALL_DEBUG
	add_filelist_record(package->get_id(), package->get_files_ptr());
#endif
	
	string sys;
	pData.increaseItemProgress(package->itemID);
	if (actionBus._abortActions)
	{
		sqlFlush();
		actionBus._abortComplete=true;
		actionBus.setActionState(ACTIONID_INSTALL, ITEMSTATE_ABORTED);
		return MPKGERROR_ABORTED;
	}

	printHtmlProgress();
/*	if (!DO_NOT_RUN_SCRIPTS)
	{
		printHtmlProgress();
		currentStatus = statusHeader + _("executing pre-install scripts");
		if (dialogMode) ncInterface.setProgressText(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + "\n" + index_hole + _("executing pre-install script"));
		msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": executing pre-install script"));
		if (FileExists(package->get_scriptdir() + "preinst.sh"))
		{
			string preinst="cd " + SYS_ROOT + " ; sh "+package->get_scriptdir() + "preinst.sh";
			if (setupMode) preinst += " 2>> /dev/tty4";
			if (!simulate) system(preinst.c_str());
		}
	}

	printHtmlProgress();*/
	// Extracting package
	currentStatus = statusHeader + _("extracting...");
	if (dialogMode) ncInterface.setProgressText(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + "\n" + index_hole + _("extracting ") + IntToStr(package->get_files().size()) + _(" files"));
	msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": extracting ") + IntToStr(package->get_files().size()) + _(" files"));
	pData.increaseItemProgress(package->itemID);

	// UPD: I don't think that creating root here is useful thing
	/*string create_root="mkdir -p '"+sys_root+"' 2>/dev/null";
	if (!simulate) system(create_root.c_str());*/

	sys="(cd "+sys_root+" && tar xf '"+sys_cache + package->get_filename() + "'";
	//If previous version isn't purged, do not overwrite config files
	if (_cmdOptions["skip_doc_installation"]=="true") {
		sys += " --exclude usr/doc";
	}
	if (_cmdOptions["skip_man_installation"]=="true") {
		sys += " --exclude usr/man";
	}
	if (_cmdOptions["skip_dev_installation"]=="true") {
		sys += " --exclude usr/include";
	}
	if (_cmdOptions["skip_static_a_installation"]=="true") {
		sys += " --exclude 'lib/*.a' --exclude 'lib64/*.a'";
	}
	package->get_files_ptr()->clear();
	if (setupMode && dialogMode) sys+=" > /dev/tty4 2>/dev/tty4 )";
	else if (dialogMode) sys+=" >/dev/null 2>>/var/log/mpkg-errors.log )";
	else sys+= " )";

	if (!simulate) {
#ifdef INSTALL_DEBUG
		if (true)
#else
		if (system(sys.c_str()) == 0 /* || package->get_name()=="aaa_base"*/) // Somebody, TELL ME WHAT THE HELL IS THIS???! WHY SUCH EXCEPTION?!
#endif
		{
			if (ultraFastMode) {
				if (_cmdOptions["preseve_doinst"]=="true" && FileExists(SYS_ROOT + "/install/doinst.sh") ) system("mkdir -p " + package->get_scriptdir() + " && cp " + SYS_ROOT+"/install/doinst.sh " + package->get_scriptdir()); // Please note that this stuff will not work in real world.
				if (FileExists(SYS_ROOT + "/install/postremove.sh")) system("mkdir -p " + package->get_scriptdir() + " && mv " + SYS_ROOT+"/install/postremove.sh " + package->get_scriptdir());
				if (FileExists(SYS_ROOT + "/install/preremove.sh")) system("mkdir -p " + package->get_scriptdir() + " && mv " + SYS_ROOT+"/install/preremove.sh " + package->get_scriptdir());
			}
			printHtmlProgress();
			currentStatus = statusHeader + _("executing post-install scripts...");
		}
		else {
			
			printHtmlProgress();
			currentStatus = _("Failed to extract!");
			mError(_("Error while extracting package ") + package->get_name());
			return MPKG_INSTALL_EXTRACT_ERROR;
		}
	}

	pData.increaseItemProgress(package->itemID);

/*#ifdef INSTALL_DEBUG
	DO_NOT_RUN_SCRIPTS = true;
#endif*/

	// Managing config files
	pkgConfigInstall(*package);	

	// Creating and running POST-INSTALL script
	if (!DO_NOT_RUN_SCRIPTS)
	{
		msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": executing post-install script"));
		if (dialogMode) ncInterface.setProgressText(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + "\n" + index_hole + _("executing post-install script"));
		//if (FileExists(package->get_scriptdir() + "doinst.sh"))
		if (FileExists(SYS_ROOT + "/install/doinst.sh"))
		{
			//string postinst="cd " + SYS_ROOT + " ; sh "+package->get_scriptdir() + "doinst.sh";
			string postinst;
			string tmpdoinst = "/tmp/mpkgtmp_" + package->get_name() + ".sh";
			add_tmp_file(SYS_ROOT + tmpdoinst);
			system("mv " + SYS_ROOT + "/install/doinst.sh " + SYS_ROOT + tmpdoinst);
			postinst="cd " + SYS_ROOT + " && bash " + SYS_ROOT + tmpdoinst; // New fast mode: we don't care much about script run ordering, and parallel run is MUCH faster.
			if (setupMode && dialogMode) postinst += " 2>/dev/tty4 >/dev/tty4";
			else if (dialogMode) postinst += " 2>/dev/null > /dev/null";
			//cout << "Running: '" << postinst << endl;
			system_threaded(postinst);
			//system(postinst);
		}
	}

	if (dialogMode) ncInterface.setProgressText(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + "\n" + index_hole + _("finishing installation"));
	msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": finishing installation"));
#ifndef INSTALL_DEBUG
	// UPD: we don't care about whole dir, we care only about doinst.sh one
	//system("rm -rf " + SYS_ROOT+"/install"); // Cleanup. Be aware of placing anything important to this directory
//	unlink(string(SYS_ROOT+"/doinst.sh").c_str()); // It does not exists, but in case of move errors...
//	unlink(string(SYS_ROOT+"/preremove.sh").c_str()); // *NOW* 
//	unlink(string(SYS_ROOT+"/postremove.sh").c_str()); // It does not exists, but in case of move errors...

	set_installed(package->get_id(), ST_INSTALLED);
	set_configexist(package->get_id(), ST_CONFIGEXIST);
	set_action(package->get_id(), ST_NONE, package->package_action_reason);
#endif
	if (purge_id!=0){
		set_configexist(purge_id, ST_CONFIGNOTEXIST); // Clear old purge status
		cleanFileList(purge_id);
	}
	msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": updating database"));
	if (_cmdOptions["warpmode"]!="yes") sqlFlush();
	msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": exporting legacy data"));
	if (!setupMode) exportPackage(SYS_ROOT+"/"+legacyPkgDir, *package);
	pData.increaseItemProgress(package->itemID);
	msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": complete"), SAYMODE_INLINE_END);
	package->set_action(ST_NONE, "install_complete");
	return 0;
}	//End of install_package

// New optimized exportPackage function.
void mpkgDatabase::exportPackage(const string& output_dir, PACKAGE& p) {
	ofstream filestr;
	filestr.open(string(output_dir+"/"+p.get_name()+"-"+p.get_version()+"-"+p.get_arch()+"-"+p.get_build()).c_str());
	if (!filestr.is_open()) {
		return;
	}
	filestr << "PACKAGE NAME:\t" << p.get_name()  << "-" << p.get_version() << "-" << p.get_arch() << "-" << p.get_build() << \
		"\nCOMPRESSED PACKAGE SIZE:\t" << p.get_compressed_size() << \
		"\nUNCOMPRESSED PACKAGE SIZE:\t" << p.get_installed_size() << \
		"\nPACKAGE LOCATION:\t/var/log/mount/" << p.get_filename() << \
		"\nPACKAGE DESCRIPTION:\n" << p.get_name() << ":  " << p.get_short_description() << \
		"\nFILE LIST:\n";
	
	if (p.get_files().size()==0) get_filelist(p.get_id(), p.get_files_ptr());
	for (size_t f=0; f<p.get_files().size(); f++) {
		filestr << p.get_files().at(f) << "\n";
	}
	filestr << "\n";
	filestr.close();
	return;
}

void mpkgDatabase::unexportPackage(const string& output_dir, const PACKAGE& p)
{
	string victim = output_dir+"/"+p.get_name()+"-"+p.get_version()+"-"+p.get_arch()+"-"+p.get_build();
	unlink(victim.c_str());
}

int mpkgDatabase::remove_package(PACKAGE* package, unsigned int packageNum, unsigned int packagesTotal)
{
	string index_str, action_str, by_str;
	if (packagesTotal>0) {
		index_str = "[" + IntToStr(packageNum+1) + "/"+IntToStr(packagesTotal)+"] ";
	}
	bool needSpecialUpdate = false;
	bool dontRemove = false;
	vector<string> unremovable = ReadFileStrings("/etc/mpkg-unremovable"); // List of packages, which will NEVER be removed physically
	for (unsigned int i=0; i<unremovable.size(); i++) {
		if (package->get_name() == unremovable[i]) {
//			msay("Package " + package->get_name() + " is unremovable, so it's files will not be deleted");
			dontRemove = true;
			break;
		}
	}
	if (package->get_name()=="glibc" || package->get_name()=="glibc-solibs" || package->get_name()=="aaa_elflibs" || package->get_name()=="tar" || package->get_name()=="xz" || package->get_name()=="aaa_base" || package->get_name()=="gzip") {
		dontRemove = true;
		needSpecialUpdate = true;
	}

	if (package->action()==ST_UPDATE) {
		action_str=_("Updating");
		by_str = _(" ==> ") + package->updatingBy->get_fullversion();
		if (package->needSpecialUpdate || package->updatingBy->needSpecialUpdate) {
			msay("Package " + package->get_name() + " needs a special update method");
			needSpecialUpdate = true;
		}
		if (needSpecialUpdate || package->isTaggedBy("base") || package->updatingBy->isTaggedBy("base")) needSpecialUpdate=true;
		if (needSpecialUpdate || package->get_name().find("aaa_")==0 || package->get_name().find("glibc")==0) needSpecialUpdate=true;
		if (needSpecialUpdate || package->get_name().find("tar")==0 || package->get_name().find("coreutils")==0 || package->get_name().find("sed")==0 || package->get_name().find("bash")==0 || \
				package->get_name().find("grep")==0 || package->get_name().find("gzip")==0 || package->get_name().find("which")==0) needSpecialUpdate=true;
	}
	if (package->action()==ST_REMOVE) action_str = _("Removing");
	if (package->action()==ST_PURGE) action_str = _("Purging");
	if (mConfig.getValue("always_special_update")=="yes") needSpecialUpdate = true; // Transitional purposes
	get_filelist(package->get_id(), package->get_files_ptr());
	pData.setItemProgressMaximum(package->itemID, package->get_files().size()+8);

	pData.setItemCurrentAction(package->itemID, action_str);
	msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str, SAYMODE_INLINE_START);

	string statusHeader = "["+IntToStr((int)actionBus.progress())+"/"+IntToStr((int)actionBus.progressMaximum()) + "] " + _("Removing package ") + package->get_name()+": ";
	currentStatus = statusHeader + _("initialization");
	
	printHtmlProgress();
	if (package->action()==ST_REMOVE || package->action()==ST_PURGE || package->action()==ST_UPDATE)
	{
		// Checking if package is updating, if so, get the files of new package already
		if (package->action()==ST_UPDATE) {
			msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str + _(": extracting file list"));
			LocalPackage *lp = new LocalPackage(SYS_CACHE + package->updatingBy->get_filename());
			lp->fill_filelist(package->updatingBy);
			delete lp;
		}


		// Running pre-remove scripts
		//printf("Processing\n");
		if(!DO_NOT_RUN_SCRIPTS)
		{
			if (FileExists(package->get_scriptdir() + "preremove.sh"))
			{
				msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str + _(": executing pre-remove script"));
				printHtmlProgress();
				currentStatus = statusHeader + _("executing pre-remove scripts");
				string prerem="cd " + SYS_ROOT + " ; sh "+package->get_scriptdir() + "preremove.sh";
				if (!simulate) system(prerem.c_str());
			}
		}
		
		pData.increaseItemProgress(package->itemID);
		
		// removing package
		string sys_cache=SYS_CACHE;
		string sys_root=SYS_ROOT;
		string fname;

		// Checking backups
		msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str + _(": checking for backups"));

		vector<FILE_EXTENDED_DATA> backups;
		vector<string> backups_filenames;
		get_backup_records(*package, &backups_filenames, &backups);



		// Purge is now implemented here; checking all
		currentStatus = statusHeader + _("building file list");
		vector<string> *remove_files = package->get_files_ptr(); // Note: no need to delete remove_files, because it will be deleted together with package object
		vector<string> *new_files = package->updatingBy->get_files_ptr();

		// Check for special procedures
		if (!needUpdateXFonts) {
			msay(index_str + _("Installing ") + package->get_name() + " " + package->get_fullversion() + _(": looking for X fonts"));

			for (size_t i=0; !needUpdateXFonts && i<package->get_files().size(); i++) {
				if (package->get_files().at(i).find("usr/share/fonts")!=std::string::npos) needUpdateXFonts = true;
			}
		}
		// Searching for icon cache updates
		bool hasIconCache = false;
		string *iconFilename;
		string iconDir;
		for (size_t i=0; i<package->get_files().size(); ++i) {
			iconFilename = (string *) &package->get_files().at(i);
			if (iconFilename->find("usr/share/icons/")!=std::string::npos && iconFilename->size()>strlen("usr/share/icons/") && iconFilename->at(iconFilename->size()-1)=='/') {
				hasIconCache=false;
				iconDir = iconFilename->substr(strlen("usr/share/icons/"));
				if (iconDir.find_first_of("/")!=std::string::npos) iconDir = iconDir.substr(0, iconDir.find_first_of("/"));
				iconDir = "usr/share/icons/" + iconDir;
				for (size_t t=0; !hasIconCache && t<iconCacheUpdates.size(); ++t) {
					if (iconCacheUpdates[t]==iconDir) hasIconCache = true;
				}
				if (!hasIconCache) iconCacheUpdates.push_back(iconDir);
			}
		}

		// Searching for gconf schemas
		bool hasGConfSchema = false;
		string *gconfFilename;
		string gconfSchemaName;
		for (size_t i=0; i<package->get_files().size(); ++i) {
			gconfFilename = (string *) &package->get_files().at(i);
			if (gconfFilename->find("usr/share/gconf/schemas/")!=std::string::npos && gconfFilename->size()>strlen("usr/share/gconf/schemas/") && getExtension(*gconfFilename)=="schemas") {
				hasGConfSchema=false;
				gconfSchemaName = gconfFilename->substr(strlen("usr/share/gconf/schemas/"));
				gconfSchemaName = gconfSchemaName.substr(0, gconfSchemaName.size()-strlen(".schemas"));
				for (size_t t=0; !hasGConfSchema && t<gconfSchemasUninstall.size(); ++t) {
					if (gconfSchemasUninstall[t]==gconfSchemaName) hasGConfSchema = true;
				}
				if (!hasGConfSchema) gconfSchemasUninstall.push_back(gconfSchemaName);
			}
		}

		printHtmlProgress();
		currentStatus = statusHeader + _("removing files...");
		bool removeThis;
		int unlink_ret;
		for (size_t i=0; i<remove_files->size(); ++i) {
			if (i==0 || i==remove_files->size() || i%10==0) msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str + _(": removing files [") + IntToStr(i) + "/"+IntToStr(remove_files->size())+"]");
			fname=sys_root + remove_files->at(i);
			for (size_t t=0; t<backups.size(); ++t) {
				if (remove_files->at(i)==*backups[t].filename) {
					fname = (string) SYS_BACKUP+"/"+backups[t].backup_file;
					delete_conflict_record(backups[t].overwriter_id, backups[t].backup_file);
					break;
				}
			}
		

			// Checking for: 
			// - if file is a configuration file
			// - if file will be overwritten by new package
			//
			
			removeThis = false;
			
			if (package->action()!=ST_UPDATE) {
				removeThis = true;
			}
			else {
				removeThis=true;
				if (needSpecialUpdate) {
					for (size_t t=0; removeThis && t<new_files->size(); ++t) {
						if (new_files->at(t)==remove_files->at(i)) {
							removeThis=false;
							break;
						}
					}
				}
			}
			if (checkEssentialFile(remove_files->at(i))) removeThis=false; 
			// Actually removing files
			if (removeThis && fname[fname.length()-1]!='/')
			{
				if (!simulate && !dontRemove) {
					if (verbose && !dialogMode) say("[%d] %s %s: ", (unsigned int) i, _("Removing file"), fname.c_str());
					unlink_ret = unlink(fname.c_str());
					if (verbose && !dialogMode) {
						if (unlink_ret==0) say("%sOK%s\n", CL_GREEN, CL_WHITE);
						else say(_("%sFAILED%s\n"), CL_RED, CL_WHITE);
					}
				}
			}
			pData.increaseItemProgress(package->itemID);
		}
		msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str+_(": removing empty directories"));

		printHtmlProgress();
		currentStatus = statusHeader + _("removing empty directories...");
	
		// Run 2: clearing empty directories
		vector<string>empty_dirs;
		string edir;
		
		pData.increaseItemProgress(package->itemID);
		
		for (unsigned int i=0; i<remove_files->size(); i++)
		{
			fname=sys_root + remove_files->at(i);
			for (unsigned int d=0; d<fname.length(); d++)
			{
				edir+=fname[d];
				if (fname[d]=='/')
				{
					empty_dirs.resize(empty_dirs.size()+1);
					empty_dirs[empty_dirs.size()-1]=edir;
				}
			}

			for (int x=empty_dirs.size()-1;x>=0; x--)
			{
				if (!simulate && !dontRemove) {
					unlink_ret = rmdir(empty_dirs[x].c_str());
					if (verbose) {
						if (unlink_ret == 0) say("[%d] %s %s\n", i, _("Removing empty directory"), fname.c_str());
					}

				}
			}
			edir.clear();
			empty_dirs.clear();
		}
	
		printHtmlProgress();
		// Creating and running POST-REMOVE script
		if (!DO_NOT_RUN_SCRIPTS)
		{
			if (FileExists(package->get_scriptdir() + "postremove.sh"))
			{
				msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str+_(": executing post-remove script"));
				currentStatus = statusHeader + _("executing post-remove scripts");
				string postrem="cd " + SYS_ROOT + " ; sh " + package->get_scriptdir() + "postremove.sh";
				if (!simulate) system(postrem.c_str());
			}
		}
		// Clearing scripts
		msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str + _(": clearing script directory"));
		system("rm -rf " + package->get_scriptdir() + " 2>/dev/null >/dev/null");
	
		// Restoring backups. NOTE: if there is an updating package, manage backups anyway. TODO: minimize file operation by processing more data
		
		msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str+_(": restoring backups"));

		printHtmlProgress();
		
		// Creating restore lists. Note that both objects are linked inside.
		vector<string> restore_fnames;
		vector<FILE_EXTENDED_DATA> restore;

		get_conflict_records(package->get_id(), &restore_fnames, &restore);
		int ret;
		if (!restore.empty()) {
			string cmd;
			string tmpName;
			for (size_t i=0; i<restore.size(); ++i) {
				if (restore[i].filename->find_last_of("/")!=std::string::npos)	{
					cmd = "mkdir -p ";
					cmd += SYS_ROOT + restore[i].filename->substr(0, restore[i].filename->find_last_of("/")) + " 2>/dev/null >/dev/null";
					if (!simulate) system(cmd.c_str());
				}
				cmd = "mv ";
			        cmd += SYS_BACKUP+restore[i].backup_file + " ";
				tmpName = restore[i].backup_file.substr(SYS_BACKUP.length());
				tmpName = tmpName.substr(tmpName.find("/"));
			        cmd += SYS_ROOT + tmpName.substr(0,tmpName.find_last_of("/"))+"/ 2>/dev/null >/dev/null";
				if (!simulate) ret = system(cmd);
				delete_conflict_record(package->get_id(), restore[i].backup_file);
			}
		}
		
		// Calling remove for package configs
		pkgConfigRemove(*package);

		msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str + _(": finishing"));
		pData.increaseItemProgress(package->itemID);
		set_installed(package->get_id(), ST_NOTINSTALLED);
		if (package->action()==ST_PURGE) set_configexist(package->get_id(), 0);
		set_action(package->get_id(), ST_NONE, package->package_action_reason);
		currentStatus = statusHeader + _("cleaning file list");
		pData.increaseItemProgress(package->itemID);
		cleanFileList(package->get_id());
		pData.increaseItemProgress(package->itemID);
		msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str + _(": updating database"));

		sqlFlush();
		currentStatus = statusHeader + _("remove complete");
		package->get_files_ptr()->clear();
		unexportPackage(SYS_ROOT+"/"+legacyPkgDir, *package);
		pData.setItemProgress(package->itemID, pData.getItemProgressMaximum(package->itemID));
		if (package->action()==ST_UPDATE) msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str + _(": done"));
		else msay(index_str + action_str + " " + package->get_name() + " " + package->get_fullversion() + by_str+": done", SAYMODE_INLINE_END);
		
		printHtmlProgress();
		// If update: run install now (for fault tolerance)
		if (package->action()==ST_UPDATE) {
			return install_package(package->updatingBy, packageNum, packagesTotal);
		}
		return 0;
	}
	else
	{
		mError(_("Weird status of package, i'm afraid to remove this..."));
		return -1;
	}
}	// End of remove_package

int mpkgDatabase::delete_packages(PACKAGE_LIST *pkgList)
{
	if (pkgList->IsEmpty())
	{
		return 0;
	}
	SQLRecord sqlSearch;
	sqlSearch.setSearchMode(SEARCH_IN);
	for(unsigned int i=0; i<pkgList->size(); i++)
	{
		sqlSearch.addField("package_id", pkgList->at(i).get_id());
	}
	db.sql_delete("packages", sqlSearch);
	sqlSearch.clear();
	for(unsigned int i=0; i<pkgList->size(); i++)
	{
		sqlSearch.addField("packages_package_id", pkgList->at(i).get_id());
	}
	db.sql_delete("tags_links", sqlSearch);
	db.sql_delete("dependencies", sqlSearch);
	db.sql_delete("deltas", sqlSearch);
#ifdef ENABLE_INTERNATIONAL
	db.sql_delete("descriptions", sqlSearch);
	db.sql_delete("changelogs", sqlSearch);
	db.sql_delete("ratings", sqlSearch);
#endif

	// Removing unused tags
	sqlSearch.clear();
	SQLTable available_tags;
	SQLRecord fields;

	db.get_sql_vtable(available_tags, sqlSearch, "tags", fields);
	SQLTable used_tags;
	db.get_sql_vtable(used_tags, sqlSearch, "tags_links", fields);
	vector<string> toDelete;
	bool used;

	// Index
	//int fAvailTags_name = available_tags.getFieldIndex("tags_name");
	int fAvailTags_id = available_tags.getFieldIndex("tags_id");
	int fUsedTags_tag_id = used_tags.getFieldIndex("tags_tag_id");
	if (available_tags.size()>0) {
		for(unsigned int i=0; i<available_tags.size(); i++) {
			used=false;
			for (unsigned int u=0; u<used_tags.size(); u++) {
				if (used_tags.getValue(u, fUsedTags_tag_id)==available_tags.getValue(i, fAvailTags_id)) {
					used=true;
				}
			}
			if (!used) {
				//say(_("Deleting tag %s as unused\n"), available_tags.getValue(i, fAvailTags_name).c_str());
				toDelete.push_back(available_tags.getValue(i,"tags_id"));
			}
		}
		available_tags.clear();
		sqlSearch.clear();
		sqlSearch.setSearchMode(SEARCH_IN);
		if (toDelete.size()>0)
		{
			for (unsigned int i=0; i<toDelete.size(); i++)
			{
				sqlSearch.addField("tags_id", toDelete[i]);
			}
			db.sql_delete("tags", sqlSearch);
		}
	}
	return 0;
}



int mpkgDatabase::cleanFileList(int package_id)
{
	SQLRecord sqlSearch;
	sqlSearch.addField("packages_package_id", package_id);
	int ret = db.sql_delete("files", sqlSearch);
	if (ret!=0) return ret;
	string pkg_id = IntToStr(package_id);
	if (pkg_id.empty()) return -1;
	db.sql_exec("DELETE FROM config_options WHERE config_files_id IN (SELECT id FROM config_files WHERE package_id=" + pkg_id + ");");
	db.sql_exec("DELETE FROM config_files WHERE package_id=" + pkg_id + ";");

	set_configexist(package_id,0);
	return ret;
}

int mpkgDatabase::update_package_data(int package_id, PACKAGE *package)
{
	PACKAGE old_package;
	if (get_package(package_id, &old_package)!=0)
	{
		return -1;
	}
	
	SQLRecord sqlUpdate;
	SQLRecord sqlSearch;
	sqlSearch.addField("package_id", package_id);

	// 1. Updating direct package data
	if (package->get_md5()!=old_package.get_md5())
	{
		sqlUpdate.addField("package_description", package->get_description());
		sqlUpdate.addField("package_short_description", package->get_short_description());
		sqlUpdate.addField("package_compressed_size", package->get_compressed_size());
		sqlUpdate.addField("package_installed_size", package->get_installed_size());
		sqlUpdate.addField("package_changelog", package->get_changelog());
		sqlUpdate.addField("package_packager", package->get_packager());
		sqlUpdate.addField("package_packager_email", package->get_packager_email());
		sqlUpdate.addField("package_md5", package->get_md5());
	}

	// 2. Updating filename
	if (package->get_filename()!=old_package.get_filename())
	{
		sqlUpdate.addField("package_filename", package->get_filename());
	}

	// 3. Updating status. Seems that somewhere here is an error causing double scan is required
	sqlUpdate.addField("package_available", package->available());


	// 4. Updating locations
	
	// Note about method used for updating locations:
	// If locations are not identical:
	// 	Step 1. Remove all existing package locations from "locations" table. Servers are untouched.
	// 	Step 2. Add new locations.
	// Note: after end of updating procedure for all packages, it will be good to do servers cleanup - delete all servers who has no locations.
	//"mpkg.cpp: update_package_data(): checking locations"	
	if (!package->locationsEqualTo(old_package))
	{
		SQLRecord loc_sqlDelete;
		loc_sqlDelete.addField("packages_package_id", package_id);
		int sql_del=db.sql_delete("locations", loc_sqlDelete);
		if (sql_del!=0)
		{
			return -2;
		}
		if (add_locationlist_record(package_id, package->get_locations_ptr())!=0)
		{
			return -3;
		}
		if (package->get_locations().empty())
		{
			sqlUpdate.addField("package_available", ST_NOTAVAILABLE);
		}
	}

	// 5. Updating tags
	if (!package->tagsEqualTo(old_package))
	{
		SQLRecord taglink_sqlDelete;
		taglink_sqlDelete.addField("packages_package_id", package_id);

		if (db.sql_delete("tags_links", taglink_sqlDelete)!=0) return -4;
		if (add_taglist_record(package_id, package->get_tags_ptr())!=0) return -5;
	}

	// 6. Updating dependencies
	if (!package->depsEqualTo(old_package))
	{
		SQLRecord dep_sqlDelete;
		dep_sqlDelete.addField("packages_package_id", package_id);

		if(db.sql_delete("dependencies", dep_sqlDelete)!=0) return -6;
		if (add_dependencylist_record(package_id, package->get_dependencies_ptr())!=0) return -7;
	}

	// 7, 8 - update scripts and file list. It is skipped for now, because we don't need this here (at least, for now).
	if (!sqlUpdate.empty())
	{
		if (db.sql_update("packages", sqlUpdate, sqlSearch)!=0)
		{
			return -8;
		}
	}
	return 0;
}

bool compareLocations(const vector<LOCATION>& location1, const vector<LOCATION>& location2) {
	if (location1.size()!=location2.size()) return false;
	// Note: **NOT** ignoring order
	for (size_t i=0; i<location1.size(); ++i) {
		if (!location1[i].equalTo(location2[i])) return false;
	}
	return true;
}


int mpkgDatabase::updateRepositoryData(PACKAGE_LIST *newPackages)
{
	// Одна из самых страшных функций в этой программе.
	// Попытаемся применить принципиально новый алгоритм.
	// 
	// Для этого введем некоторые тезисы:
	// 1. Пакет однозначно идентифицируется по его контрольной сумме.
	// 2. Пакет без контрольной суммы - не пакет а мусор, выкидываем такое нахрен.
	// 
	// Алгоритм:
	// 1. Стираем все записи о locations и servers в базе.
	// 2. Забираем из базы весь список пакетов. Этот список и будет рабочим.
	// 3. Для каждого пакета из нового списка ищем соответствие в старой базе.
	// 	3а. В случае если такое соответствие найдено, вписываем в список записи о locations. Остальное остается неизменным, ибо MD5 та же.
	// 	3б. В случае если соответствие не найдено, пакет добавляется в конец рабочего списка с флагом new = true
	// 4. Вызывается синхронизация рабочего списка и данных в базе (это уже отдельная тема).
	//
	// ////////////////////////////////////////////////
	//"Retrieving current package list, clearing tables"
	// Стираем locations и servers
	db.clear_table("locations");
	db.clear_table("deltas");
	db.clear_table("abuilds");
	//if (forceFullDBUpdate) db.clear_table("dependencies");  // Teh WTF!
	

	// Забираем текущий список пакетов
	PACKAGE_LIST *pkgList = new PACKAGE_LIST;
	SQLRecord sqlSearch;
	get_packagelist(sqlSearch, pkgList);
	
	//say("Merging data\n");
	// Ищем соответствия // TODO: надо б тут что-нить прооптимизировать
	int pkgNumber;
	size_t new_pkgs=0;
	vector<bool> needUpdateRepositoryTags;
	vector<bool> needUpdateDistroVersion;
	for(size_t i=0; i<newPackages->size(); i++)
	{
		pkgNumber = pkgList->getPackageNumberByMD5(newPackages->at(i).get_md5());
		
		if (pkgNumber!=-1)	// Если соответствие найдено...
		{
			pkgList->get_package_ptr(pkgNumber)->set_locations(newPackages->at(i).get_locations());	// Записываем locations
			pkgList->get_package_ptr(pkgNumber)->deltaSources=newPackages->at(i).deltaSources; // TEMP DISABLED
			pkgList->get_package_ptr(pkgNumber)->abuild_url=newPackages->at(i).abuild_url; // One ABUILD record will be enough.
			
			if (pkgList->get_package_ptr(pkgNumber)->get_repository_tags()!=newPackages->at(i).get_repository_tags()) {
				pkgList->get_package_ptr(pkgNumber)->set_repository_tags(newPackages->at(i).get_repository_tags());
				needUpdateRepositoryTags.push_back(true);
			}
			else needUpdateRepositoryTags.push_back(false);
			
			if (pkgList->get_package_ptr(pkgNumber)->package_distro_version==newPackages->at(i).package_distro_version) {
				pkgList->get_package_ptr(pkgNumber)->package_distro_version = newPackages->at(i).package_distro_version;
				needUpdateDistroVersion.push_back(true);
			}
			else needUpdateDistroVersion.push_back(false);
			
			//pkgList->get_package_ptr(pkgNumber)->set_dependencies(newPackages->at(i).get_dependencies());
			//db.sql_exec("DELETE FROM dependencies WHERE packages_package_id='" + IntToStr(pkgList->get_package_ptr(pkgNumber)->get_id()) + "';");
		}
		else			// Если соответствие НЕ найдено...
		{
			newPackages->get_package_ptr(i)->newPackage=true;
			pkgList->add(newPackages->at(i));
			needUpdateRepositoryTags.push_back(true);
			needUpdateDistroVersion.push_back(true);
			new_pkgs++;
		}
	}

	//say("Clean up...\n");
	// Вызываем синхронизацию данных.
	// Вообще говоря, ее можно было бы делать прямо здесь, но пусть таки будет универсальность.
	delete newPackages;//->clear();
	syncronize_data(pkgList, needUpdateRepositoryTags, needUpdateDistroVersion);
	if (!dialogMode && new_pkgs>0) say(_("New packages in repositories: %d\n"), (unsigned int) new_pkgs);
	return 0;
}
int mpkgDatabase::syncronize_data(PACKAGE_LIST *pkgList, vector<bool> needUpdateRepositoryTags, vector<bool> needUpdateDistroVersion)
{
	// Идея:
	// Добавить в базу пакеты, у которых флаг newPackage
	// Добавить locations к тем пакетам, которые такого флага не имеют
	// 
	// Алгоритм:
	// Бежим по списку пакетов.
	// 	Если пакет имеет влаг newPackage, то сразу добавляем его в базу функцией add_package_record()
	//	Если флага нету, то сразу добавляем ему locations функцией add_locationlist_record()
	// 
	
	SQLRecord *sqlUpdate = NULL;
	for(size_t i=0; i<pkgList->size(); i++)
	{
		if (pkgList->at(i).newPackage) {
			add_package_record(pkgList->get_package_ptr(i));
		}
		else {
			add_locationlist_record(pkgList->at(i).get_id(), pkgList->get_package_ptr(i)->get_locations_ptr());
			if (!pkgList->at(i).deltaSources.empty()) add_delta_record(pkgList->at(i).get_id(), pkgList->at(i).deltaSources);
			if (!pkgList->at(i).abuild_url.empty()) add_abuild_record(pkgList->at(i).get_id(), pkgList->at(i).abuild_url);
			if (needUpdateDistroVersion[i] || needUpdateRepositoryTags[i]) {
				sqlUpdate = new SQLRecord;
				if (needUpdateRepositoryTags[i]) sqlUpdate->addField("package_repository_tags", pkgList->at(i).get_repository_tags());
				if (needUpdateDistroVersion[i]) sqlUpdate->addField("package_distro_version", pkgList->at(i).package_distro_version);
				update_package_record(pkgList->at(i).get_id(), *sqlUpdate);
				delete sqlUpdate;
			}
			//add_dependencylist_record(pkgList->at(i).get_id(), pkgList->get_package_ptr(i)->get_dependencies_ptr());
		}
	}
	delete pkgList;

	// Дополнение от 10 мая 2007 года: сносим нафиг все недоступные пакеты, которые не установлены. Нечего им болтаться в базе.
	clear_unreachable_packages();
	return 0;

}

int mpkgDatabase::clear_unreachable_packages() {
	PACKAGE_LIST *allList = new PACKAGE_LIST;
	SQLRecord sqlSearch;
	sqlSearch.addField("package_installed", ST_NOTINSTALLED);
	sqlSearch.addField("package_configexist", ST_CONFIGNOTEXIST);
	get_packagelist(sqlSearch, allList);
	PACKAGE_LIST deleteQueue;
	unsigned int rm_pkgs = 0;
	for(unsigned int i=0; i<allList->size(); i++)
	{
		if (allList->at(i).installed()) continue;
		if (!allList->at(i).reachable(true)) {
			deleteQueue.add(allList->at(i));
		}
		else {
			if (allList->at(i).available(false)!=allList->at(i).available(true)) {
				if (!allList->at(i).configexist()) {
					deleteQueue.add(allList->at(i));
					rm_pkgs++;
				}
			}
		}
	}
	if (!deleteQueue.IsEmpty()) delete_packages(&deleteQueue);
	delete allList;
	if (!dialogMode && rm_pkgs>0) say(_("Packages removed from repositories: %d\n"),rm_pkgs);
	return 0;

}
