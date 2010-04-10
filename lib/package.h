// $Id: package.h,v 1.7 2007/12/07 03:34:20 i27249 Exp $

#ifndef PACKAGE_H_
#define PACKAGE_H_
#include "libmpkg.h"
enum {
	DATATYPE_UNKNOWN = 0,
	DATATYPE_NEW,
	DATATYPE_BINARYPACKAGE,
	DATATYPE_SOURCEPACKAGE,
	DATATYPE_XML,
	DATATYPE_DIR
};
class BinaryPackage
{
	public:

		BinaryPackage();
		BinaryPackage(string in_file);
		~BinaryPackage();
		bool isExtracted();
		bool extracted;

		bool usingDirectory;
		bool importDirectory(string dirname);
		bool createNew();
		bool createWorkingDirectory();
		bool createFolderStructure();
		
		bool fileOk();
		string input_file;

		bool setInputFile(string in_file); // return true if file exists and readable, false if not

		string pkg_dir; // Extracted package root (some tmp if new
		bool unpackFile(); // Extract contents to a temp directory
	       	bool packFile(string output_directory="", string* full_output_dir=NULL); // Pack a structure.

		void clean();

		string getExtractedPath();
		
		string getDataFilename(); // Filename of data.xml
		
		bool setPreinstallScript(string script_text);
		bool setPostinstallScript(string script_text);
		bool setPreremoveScript(string script_text);
		bool setPostremoveScript(string script_text);


		string readPreinstallScript();
		string readPostinstallScript();
		string readPreremoveScript();
		string readPostremoveScript();

};

class SourcePackage: public BinaryPackage
{
	public:
		bool unpackFile();
		bool packFile(string output_directory="", string* full_output_filename=NULL);
		bool createFolderStructure();

		bool embedPatch(string filename); // Copy patch file to a package structure
		bool embedSource(string filename); // Copy sources for a package structure
		string source_filename;
		bool removeSource(string filename="");
		bool removeAllPatches();
		bool removePatch(string patch_name); // specify patch filename (without path) to remove from structure

		bool setBuildScript(string script_text);
		bool setPrebuildScript(string script_text);
		bool isSourceEmbedded(string url);
		bool isPatchEmbedded(string patch_name);
		string readBuildScript();
		string readPrebuildScript();

		vector<string> getEmbeddedPatchList();
		vector<string> getSourceFilenames();
		string getSourceDirectory();

};

enum {
	BUILDTYPE_AUTOTOOLS = 0,
	BUILDTYPE_SCONS,
	BUILDTYPE_CMAKE,
	BUILDTYPE_CUSTOM,
	BUILDTYPE_SCRIPT,
	BUILDTYPE_QMAKE4,
	BUILDTYPE_QMAKE3,
	BUILDTYPE_PYTHON,
	BUILDTYPE_MAKE,
	BUILDTYPE_PERL,
	BUILDTYPE_WAF
};

class SourceFile
{
	public:
		SourceFile();
		~SourceFile();
		void setUrl(string _url);
		bool download(bool *existFlag=NULL, bool useXterm=false);
		bool analyze(string *configure_help=NULL);
		
		string getType();
		int getBuildType();
		string getBuildTypeS();
		string getSourceDirectory();

	private:
		string url;
		string filepath;

		string type;
		int buildType;
		string sourceDirectory;
};

#endif
