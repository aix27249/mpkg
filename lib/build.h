#ifndef BUILD_H_
#define BUILD_H_
#include "config.h"
#include "PackageConfig.h"
#include "package.h"

// Package building class
enum DownloadMethod {
	DLMETHOD_WGET = 0,
	DLMETHOD_CVS,
	DLMETHOD_COPY
};

class PackageBuilder {
	public:
		PackageBuilder(string _file_url="");
		~PackageBuilder();
		bool build();
		string getPackageName();
	private:
		// Variables

		SourcePackage spkg; // Source package
		PackageConfig *p; // Package data
		string fileName; // File name
		
		string build_system; // Building system

		string march, mtune, olevel, extraCFlags, envOptions; // Optimization and environment options

		string PACKAGE_OUTPUT;
		string mainUrl; // Main URL
		StringMap urlMap; // List of additional URLs
		vector<string> patchList;
		//string dldir; // spkg.getExtractedPath()
		string srcdir; // Extracted sources directory
		
		string script_cmd, configure_cmd, make_cmd, make_install_cmd;

		string cflags, gcc_options, numjobs;

		bool canCustomize, useCflags;
		bool noSubfolder;
		
		
		// Flags
		bool unpacked;

		// Methods
		void setFileUrl(string _file_url);
		void setCmdlineOptions(string c_march, string c_mtune, string c_olevel);
		bool unpackFile();
		bool analyzePackage();
		int selectCFlags();
		void selectPackageOutput();
		bool download();
		bool prepare();
		bool extract();
		bool compile();
		bool assemblePackages();
		bool buildDependencies();
		bool cleanup();
};


#endif
