/* Temporary config - until a full-functional config will be implemented
    $Id: config.h,v 1.56 2007/12/10 03:12:58 i27249 Exp $
*/


#ifndef CONFIG_H_
#define CONFIG_H_
#include "build_counter.h"
#include "errorcodes.h"
#include "bus.h"
//#include "string_operations.h"
//#include "interface.h"
#include <vector>
#include <map>
#include <nwidgets/ncurses_if.h>
using namespace std;
#define CHECKFILES_PREINSTALL 1
#define CHECKFILES_POSTINSTALL 2
#define CHECKFILES_DISABLE 0
extern map<string, string> _cmdOptions;
extern bool enableSpkgIndexing;
extern bool afraidAaaInDeps;
extern bool getlinksOnly;
extern bool _cdromMounted;
extern bool forceFullDBUpdate;
extern int TERM_COLS;
class DepErrorTable {
	public:
		DepErrorTable();
		~DepErrorTable();
		void add(int package_id, string package_name, string package_version, string required_name, string required_version, int failure_type);
		vector<string> packageName, packageVersion, requiredName, requiredVersion;
		vector<int> packageID, failureType;
		string print(int num=-1);
};
extern string htmlPage;
extern string htmlNextLink;
extern bool htmlMode;
extern bool serverMode;
extern bool noEject;
extern bool FORCE_CDROM_CACHE;
extern DepErrorTable depErrorTable;
extern string SYS_BACKUP; //TODO
// Database type definitions
#define DB_SQLITE_LOCAL 0x01
#define DB_REMOTE 0x02
// Global error code definition

#ifndef HTTP_LIB
enum adModes {
	ADMODE_MOZGMERTV=0,
	ADMODE_OFF,
	ADMODE_REPLACE,
	ADMODE_UPDATE,
	ADMODE_ADD
};
// Global configuration and message bus

//extern interface ncProgressBar;
extern CursesInterface ncInterface;
extern bool msayState;
extern string MAKEPKG_CMD;
extern bool verbose;
extern bool useBuildCache;
extern int autogenDepsMode;
extern bool enableDownloadResume;
extern bool dont_export;
extern string legacyPkgDir;
extern bool setupMode;
extern bool interactive_mode;
extern bool forceSkipLinkMD5Checks;
extern bool forceInInstallMD5Check;
extern bool simulate;
extern bool force_dep;
extern bool force_skip_conflictcheck;
extern bool force_essential_remove;
extern bool download_only;
//#define log_directory "/var/log/"
extern string log_directory;
extern bool require_root;
extern bool DO_NOT_RUN_SCRIPTS;	// Prevent executing of scripts - it may be dangerous
extern unsigned int fileConflictChecking;

extern string CONFIG_FILE;
extern string SYS_MPKG_VAR_DIRECTORY;
//extern string CDROM_DEVICE;
//extern string CDROM_MOUNTPOINT;
extern string CDROM_VOLUMELABEL; // For QT GUI
extern string CDROM_DEVICENAME;  // For QT GUI
extern vector<string> removeBlacklist;
// System configuration
extern bool ignoreDeps;
extern bool consoleMode;
extern bool dialogMode;
extern string SYS_ROOT;		// "/root/development/sys_root/"
extern string TEMP_SYSROOT;
extern string TEMP_CACHE;
extern string SYS_CACHE; 	//"/root/development/sys_cache/"
extern string SCRIPTS_DIR;
extern unsigned int DATABASE;
extern string DB_FILENAME;
extern vector<string> REPOSITORY_LIST;
extern vector<string> DISABLED_REPOSITORY_LIST;
#include <libintl.h>
#include <locale.h>
#include "xmlParser.h"
#include "debug.h"
#endif //HTTP_LIB
#ifdef HTTP_LIB
extern string DL_CDROM_DEVICE;
extern string DL_CDROM_MOUNTPOINT;
#endif
int loadGlobalConfig(string config_file=CONFIG_FILE);
//void consoleEventResolver();

void initDirectoryStructure();


namespace mpkgconfig
{
	int initConfig();
	XMLNode getXMLConfig(string conf_file=CONFIG_FILE);
	int setXMLConfig(XMLNode xmlConfig, string conf_file=CONFIG_FILE);

	vector<string> get_repositorylist();
	vector<string> get_disabled_repositorylist();
	string get_sysroot();
	string get_syscache();
	string get_dburl();
	string get_scriptsdir();
	string get_cdromdevice();
	string get_cdrommountpoint();
	bool get_runscripts();
	unsigned int get_checkFiles();

	int set_repositorylist(vector<string> newrepositorylist, vector<string> drList);
	int set_disabled_repositorylist(vector<string> newrepositorylist);
	int set_sysroot(string newsysroot);
	int set_syscache(string newsyscache);
	int set_dburl(string newdburl);
	int set_scriptsdir(string newscriptsdir);
	int set_cdromdevice(string cdromDevice);
	int set_cdrommountpoint(string cdromMountPoint);
	int set_runscripts(bool dorun);
	int set_checkFiles(unsigned int value);
}

class Config
{
	public:
	Config(string _configName=CONFIG_FILE);
	~Config();
	string getValue(string attribute, int value_id = 0);
	bool setValue(string attribute, string value, int value_id = 0);	
	string configName;
	private:
	XMLNode node;
	bool readXml();
	bool writeXml();
};


extern Config mConfig;

class TimeCounter {
	time_t zero_time;
	time_t current_time;
	void startCounter();
	void stopCounter();
	void pauseCounter();
	time_t elapsedTime();
};
#endif
