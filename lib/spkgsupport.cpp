#include "spkgsupport.h"
#include "package.h"

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

