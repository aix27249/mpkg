#ifndef SETUP_H_INCL
#define SETUP_H_INCL

#include <mpkg/libmpkg.h>
#include <sys/mount.h>
#include <mpkg/colors.h>
#include <mpkg/menu.h>

struct SysConfig
{
	string swapPartition;
	string rootPartition;
	string rootPartitionType;
	bool rootPartitionFormat;
	vector<TagPair>otherMounts;
	vector<string>otherMountFSTypes;
	vector<string>oldOtherFSTypes;
	vector<string>otherMountSizes;
	vector<bool>otherMountFormat;
	string rootMountPoint;
	string cdromDevice;
	string sourceName;
	unsigned int totalQueuedPackages, totalDependantPackages;
	unsigned long long totalDownloadSize, totalInstallSize;
	string setupMode;
	int setupModeI;
	string totalRequiredSpace;
	vector<TagPair> tmpMounts;
	string lang;
	bool tryPrelink;
	string kernelversion;
	bool tmpfs_tmp;
};

struct BootConfig
{
	string kernelOptions;
	string bootDevice;
	string videoModeName;
	unsigned int videoModeNumber;
	string rootFs;
	string loaderType;
};
struct OsRecord
{
	string label;
	string type;
	string kernel;
	string kernel_options;
	string root;
};

struct cmdConfig
{
	string sysroot;
	string sysroot_format;
	string pkgsource;
	string bootloader;
	string bootloader_root;
	string install_type;
	string swap_partition;
} sc;

struct CustomPkgSet {
	string name, desc, full;
};

int packageSelectionMenu(string predefined = "", bool simple=false);
string getLastError();
string doFormatString(bool input);
bool setPartitionMap();
void showGreeting();
int setSwapSpace(string predefined="");
int setRootPartition(string predefined="", string predefinedFormat = "", bool simple = false);
int setOtherPartitions(bool simple = false);
int formatPartitions();
int mountPartitions();
int autoInstall();
int manualInstall();
int initDatabaseStructure();
int moveDatabaseToHdd();
int mountMedia(string iso_image = "");
int selectInstallMethod();
void writeFstab();
int performConfig(bool simple = false);
void syncFS();
int packageSourceSelectionMenu(string predefinedSource="");
bool showFinish();
int setHDDSource(string predefined="");
int setNetworkSource(string predefined="");
int setCDSource(string predefined="");
// Internal configuration modules
int setupBootloaderOptions(bool simple = false);
int diskPartitioningMenu(bool simple=false);
bool liloconfig();
bool grubconfig();
bool grub2config();
bool msdosconfig(string device = "");
vector<OsRecord> getOsList();
StringMap getDeviceMap(string mapFile="/boot/grub/device.map");
int netConfig();
string ncGetMountPartition(string header);
int setSambaSource(string predefined, string username="", string password="");
int buildInitrd();
string getUUID(const string& dev);
string mapPart(StringMap devMap, string partName, int isGrubLegacy=1);
int commit(bool simple=false);
#endif
