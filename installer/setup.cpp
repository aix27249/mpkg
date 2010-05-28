/****************************************************
 * AgiliaLinux: system setup (new generation)
 * $Id: setup.cpp,v 1.55 2007/11/20 00:41:50 i27249 Exp $
 *
 * Required libraries:
 * libparted
 * libmpkg
 * 
 * 
 * Required tools:
 * mpkg
 * mkfs.*
 * mkswap
 * swapon/swapoff
 *
 * *************************************************/
#define NTFS3G // We want to use NTFS-3G to handle NTFS volumes
//#define ALPHA // Alpha mode installer

#include "setup.h"
#include "composite_setup.h"
#include "raidtool.h"
/*
#define PKGSET_MINIMAL 1
#define PKGSET_SERVER 2
#define PKGSET_FULL 3
#define PKGSET_EXPERT 4
#define PKGSET_FILE 5
#define PKGSET_XFCE 6
*/
#define USE_X11_HAL 

vector<CustomPkgSet> customPkgSetList;

bool enable_addons = false;
bool enable_contrib = false;
bool askForKDE = false;
string i_menuHead = _("AgiliaLinux ") + (string) string(DISTRO_VERSION);// + " installation";// + " [mpkg  v." + mpkgVersion + " build " + mpkgBuild + "]";
bool licenseAccepted = false;
bool unattended = false;
SysConfig systemConfig;
BootConfig bootConfig;
bool enable_setup_debug=false;
bool use_initrd = true;
PACKAGE_LIST i_availablePackages;
vector<string> i_tagList;
mpkg *core=NULL;
bool realtimeTrackingEnabled=true;
bool onlineDeps=true;
string kde_branch;
string rootdev_wait = "1";
bool ext4_supported=false;
void generateLangSh(string dir="/mnt/etc/profile.d/") {
	string lang_sh="#!/bin/sh\n\
export LANG=$L\n\
export LC_CTYPE=$L\n\
export LC_NUMERIC=$L\n\
export LC_TIME=$L\n\
export LC_COLLATE=C\n\
export LC_MONETARY=$L\n\
export LC_MESSAGES=$L\n\
export LC_PAPER=$L\n\
export LC_NAME=$L\n\
export LC_ADDRESS=$L\n\
export LC_TELEPHONE=$L\n\
export LC_MEASUREMENT=$L\n\
export LC_IDENTIFICATION=$L\n\
export LESSCHARSET=UTF-8\n";
	strReplace(&lang_sh, "$L", systemConfig.lang);
	WriteFile(dir+"lang.sh", lang_sh);
	strReplace(&lang_sh, "export", "setenv");
	strReplace(&lang_sh, "/bin/sh", "/bin/csh");
	WriteFile(dir+"lang.csh", lang_sh);
}
vector<MenuItem> getAvailableXkbLayouts() {
	vector<MenuItem> ret;
	ret.push_back(MenuItem("us", _("English"), ""));
	ret.push_back(MenuItem("ru", _("Russian"), "winkeys"));
	ret.push_back(MenuItem("ru", _("Russian (UNIX)"), ""));
	ret.push_back(MenuItem("ua", _("Ukrainian"), "winkeys"));
	ret.push_back(MenuItem("ua", _("Ukrainian (UNIX)"), ""));
	ret.push_back(MenuItem("il", _("Hebrew"), ""));
	ret.push_back(MenuItem("il", _("Hebrew (phonetic)"), "phonetic"));
	return ret;
}
vector<MenuItem> askXkbLayout() {
	vector<MenuItem> al = getAvailableXkbLayouts();
	for (size_t i=0; i<al.size(); ++i) {
		if (al[i].tag=="us") al[i].flag = true;
		if (al[i].tag=="ru" && al[i].description=="winkeys" && systemConfig.lang.find("ru")==0) al[i].flag=true;
		if (al[i].tag=="ua" && al[i].description=="winkeys" && systemConfig.lang.find("uk")==0) al[i].flag=true;
	}
	ncInterface.showExMenu(_("Select X11 keyboard layouts. If not sure, leave this as is"), al);
	return al;
	

}
void xorgSetLang() {
	string lang;
	if (systemConfig.lang.find("ru")==0) lang="us,ru(winkeys)";
	if (systemConfig.lang.find("uk")==0) lang="us,uk(winkeys)";
	if (systemConfig.lang.find("en")==0) lang="us";
	string data = ReadFile(systemConfig.rootMountPoint + "/etc/X11/xorg.original");
	strReplace(&data, "us,ru(winkeys)", lang);
	WriteFile(systemConfig.rootMountPoint + "/etc/X11/xorg.original", data);
}

void xorgSetLangHALEx() {
	string lang, varstr;
	vector<MenuItem> langmenu = askXkbLayout();
	size_t cnt=0;
	for (size_t i=0; i<langmenu.size(); ++i) {
		if (!langmenu[i].flag) continue;
		if (cnt!=0) {
			lang += ",";
			varstr+=",";
		}
		lang += langmenu[i].tag;
		varstr += langmenu[i].description;
		cnt++;
	}
	string variant = "<merge key=\"input.xkb.variant\" type=\"string\">" + varstr + "</merge>\n";
	
	string fdi = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><!-- -*- SGML -*- -->\n\
<match key=\"info.capabilities\" contains=\"input.keyboard\">\n\
  <merge key=\"input.xkb.layout\" type=\"string\">" + lang + "</merge>\n" + variant + \
"  <merge key=\"input.xkb.options\" type=\"string\">terminate:ctrl_alt_bksp,grp:ctrl_shift_toggle,grp_led:scroll</merge>\n\
</match>\n";
	system("mkdir -p " + systemConfig.rootMountPoint + "/etc/hal/fdi/policy 2>/dev/null >/dev/null");
	WriteFile(systemConfig.rootMountPoint + "/etc/hal/fdi/policy/10-x11-input.fdi", fdi);
}

bool setupListCheck(const PACKAGE_LIST& pkgList) {
	vector<string> corenames;
	string corename;
	bool found;
	vector<MenuItem> dupes;
	for (size_t i=0; i<pkgList.size(); ++i) {
		if (pkgList[i].action()!=ST_INSTALL) continue;
		corename = pkgList[i].get_corename();
		found = false;
		for (size_t z=0; z<corenames.size(); ++z) {
			if (corenames[z]==corename) {
				found = true;
				break;
			}
		}
		if (!found) corenames.push_back(corename);
		else dupes.push_back(MenuItem(pkgList[i].get_name(), "corename: " + pkgList[i].get_corename() + " (" + pkgList[i].get_fullversion() + ")"));
	}
	if (dupes.empty()) {
		ncInterface.showMsgBox("List OK");
		return true;
	}
	else {
		ncInterface.showMenu("Dupes found:", dupes);
		return false;
	}
}

void xorgSetLangHAL() {
	string lang;
	string variant = "<merge key=\"input.xkb.variant\" type=\"string\">,winkeys</merge>\n";
	
	if (systemConfig.lang.find("ru")==0) lang="us,ru";
	if (systemConfig.lang.find("uk")==0) lang="us,uk";
	if (systemConfig.lang.find("en")==0) lang="us";
	
	


	string fdi = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><!-- -*- SGML -*- -->\n\
<match key=\"info.capabilities\" contains=\"input.keyboard\">\n\
  <merge key=\"input.xkb.layout\" type=\"string\">" + lang + "</merge>\n" + variant + \
"  <merge key=\"input.xkb.options\" type=\"string\">terminate:ctrl_alt_bksp,grp:ctrl_shift_toggle,grp_led:scroll</merge>\n\
</match>\n";
	system("mkdir -p " + systemConfig.rootMountPoint + "/etc/hal/fdi/policy 2>/dev/null >/dev/null");
	WriteFile(systemConfig.rootMountPoint + "/etc/hal/fdi/policy/10-x11-input.fdi", fdi);
}
void generateIssue(string dir="/mnt/etc/") {
	if (!FileExists(dir+"issue_" + systemConfig.lang)) {
		return;
	}
	system("(cd " + dir + " 2>/dev/tty4 >/dev/null ; rm issue > /dev/null 2>/dev/tty4 ; ln -s issue_" + systemConfig.lang + " issue > /dev/null 2>/dev/tty4 )");
}
bool checkForExt4() {
	// We need two conditions: kernel support and mkfs.ext4
	string kver = get_tmp_file();
	system("uname -r > " + kver);
	if (strverscmp("2.6.29", ReadFile(kver).c_str())>0) {
		unlink(kver.c_str());
		return false;
	}
	string mkfs = get_tmp_file();
	system("which mkfs.ext4 2>/dev/null >" + mkfs);
	if (ReadFile(mkfs).empty()) {
		unlink(kver.c_str());
		unlink(mkfs.c_str());
		return false;
	}
	unlink(kver.c_str());
	unlink(mkfs.c_str());
	return true;
}


void setDefaultRunlevel(const string& lvl) {
	// Can change runlevels 3 and 4 to lvl
	//
	string data = ReadFile(systemConfig.rootMountPoint + "/etc/inittab");
	strReplace(&data, "id:4:initdefault", "id:" + lvl + ":initdefault");
	strReplace(&data, "id:3:initdefault", "id:" + lvl + ":initdefault");
	WriteFile(systemConfig.rootMountPoint + "/etc/inittab", data);
	if (lvl=="4") {
		if (FileExists("/mnt/etc/rc.d/init.d/kdm")) system("chroot /mnt rc-update add kdm default");
		else if (FileExists("/mnt/etc/rc.d/init.d/gdm")) system("chroot /mnt rc-update add gdm default");
		else if (FileExists("/mnt/etc/rc.d/init.d/lxdm")) system("chroot /mnt rc-update add lxdm default");
		else if (FileExists("/mnt/etc/rc.d/init.d/slim")) system("chroot /mnt rc-update add slim default");
		else if (FileExists("/mnt/etc/rc.d/init.d/xdm")) system("chroot /mnt rc-update add xdm default");
	}
}
string getUUID(const string& dev) {
	string tmp_file = get_tmp_file();
	system("blkid -s UUID " + dev + " > " + tmp_file);
	string data = ReadFile(tmp_file);
	size_t a = data.find_first_of("\"");
	if (a==std::string::npos || a==data.size()-1) return "";
	data.erase(data.begin(), data.begin()+a+1);
	a = data.find_first_of("\"");
	if (a==std::string::npos || a==0) return "";
	return data.substr(0, a);
}

// Repository list
vector<string> rep_locations;
bool list_initialized=false;
void createCore() {
	if (core==NULL) core=new mpkg;
}

void deleteCore() {
	if (core!=NULL)	{
		delete core;
		core=NULL;
	}
}

void showGreeting() {
	ncInterface.setSubtitle(_("Welcome!"));
	ncInterface.showMsgBox(_("Welcome to AgiliaLinux!\n\nSetup program will install the system on your computer and perform initial configuration.\n"));
}

int setSwapSpace(string predefined) {
	// Selects the partition to use as a swap space
	ncInterface.setSubtitle(_("Swap partition selection"));
	vector<string> gp;
	vector<pEntry> swapList = getGoodPartitions(gp);
	if (swapList.empty()) {
		mDebug("no partitions detected");
		ncInterface.showMsgBox(_("No partitions found on your hard drive. Create it first."));
		return -3;
	}
	vector<MenuItem> sList;
	int def_id=0;
	for (size_t i=0; i<swapList.size(); i++)
	{
		sList.push_back(MenuItem(swapList[i].devname, swapList[i].fstype + " (" + swapList[i].size + "Mb)"));
		if (swapList[i].fstype.find("swap")!=std::string::npos) def_id=i;//swapList[i].devname;
	}
	sList.push_back(MenuItem(_("Skip"), _("Continue without swap partition"), _("If you have lots of RAM (2Gb or more), and you sure that you will never use all of this - you can continue without swap. Please note, that you will also loose ability of suspending to disk, because it uses swapspace for data storage.")));
	string swapPartition;
selectSwapPartition:
	if (predefined.empty()) {
		int menu_ret = ncInterface.showMenu(_("Please, choose swap partition"), sList, def_id);
		if (menu_ret == (int) sList.size()-1) return 0;
		if (menu_ret == -1) return -1;
		else swapPartition = sList[menu_ret].tag;
	}
	else {
		bool valid=false;
		// Validate predefined data
		if (predefined=="auto") {
			valid=true;
			for (size_t i=0; i<swapList.size(); ++i) {
				if (swapList[i].fstype.find("linux-swap")!=std::string::npos) {
					swapPartition=predefined;
					break;
				}
			}
		}
		for (size_t i=0; !valid && i<swapList.size(); i++) {
			if (predefined==swapList[i].devname) valid=true;
		}
		if (valid) swapPartition = predefined;
		else ncInterface.showMsgBox(_("You have defined swap partition that doesn't exists: ") + predefined);
	}
	
	if (swapPartition.empty()) return -1;
	
	for (size_t i=0; i<swapList.size(); ++i) {
		if (swapPartition==swapList[i].devname)
		{
			// We found it! Check it out
			if (swapList[i].fstype.find("linux-swap")==std::string::npos && swapList[i].fstype!="unformatted")
			{
				// Seems that we are trying to overwrite some filesystem, maybe important...
				if (!ncInterface.showYesNo(_("You are going to create swapspace on partition ") + \
							swapList[i].devname + ", which already has a filesystem " + swapList[i].fstype + \
							". Are you sure you really want to do this?"))
				{
					goto selectSwapPartition;
				}
			}
			systemConfig.swapPartition = swapList[i].devname;
		}
	}
	return 0;
}

int activateSwapSpace()
{
	// activates the swap space
	if (!systemConfig.swapPartition.empty())
	{
		string swapoff="swapoff " + systemConfig.swapPartition + " 2>/dev/null >/dev/null";
		system(swapoff);
		string swapMake = "mkswap " + systemConfig.swapPartition + " 2>/var/log/mpkg-lasterror.log >/dev/null";
		if (system(swapMake)!=0) {
			mDebug("error creating swap on " + systemConfig.swapPartition + ", reason: " + getLastError());
			ncInterface.showMsgBox(_("An error occured while creating swap: ") + getLastError());
			return -1;
		}
		else {
			mDebug("swap created on " + systemConfig.swapPartition);
			swapMake = "swapon " + systemConfig.swapPartition + " 2>/dev/null >/dev/null";
			if (system(swapMake)!=0)
			{
				ncInterface.showMsgBox(_("An error occured while enabling swap space: ") + ReadFile("/var/log/mpkg-lasterror.log"));
			}
		}
	}
	return 0;
}


int setRootPartition(string predefined, string predefinedFormat, bool simple)
{
	// Selects the root partition
	vector<string> gp;
	vector<pEntry> pList;
	pList= getGoodPartitions(gp);
	ncInterface.setSubtitle(_("Root partition setup"));
	vector<MenuItem> menuItems;
	for (size_t i=0; i<pList.size(); i++)
	{
		if (simple && pList[i].devname == systemConfig.swapPartition) continue;
		menuItems.push_back(MenuItem(pList[i].devname, pList[i].fstype + " (" + pList[i].size + "Mb)"));
	}
	string rootPartition;
	if (predefined.empty()) {
		int menu_ret = ncInterface.showMenu(_("Choose root partition for AgiliaLinux:"), menuItems);
		if (menu_ret == -1) return -1;
		else rootPartition = menuItems[menu_ret].tag;
	}
	else {
		bool valid=false;
		// Validate predefined data
		for (size_t i=0; i<pList.size(); i++) {
			if (pList[i].devname==predefined) {
				valid=true;
			}
		}
		if (valid) rootPartition = predefined;
		else {
			ncInterface.showMsgBox(_("Predefined root partition doesn't exist: ") + predefined);
			return -1;
		}
	}
	if (rootPartition.empty()) {
		mDebug("nothing selected");
		return -1;
	}
	for (size_t i=0; i<pList.size(); ++i) {
		if (pList[i].devname == rootPartition) {
			systemConfig.rootPartitionType = pList[i].fstype;
		}
	}
	mDebug("selecting FS type");
	vector<MenuItem> formatOptions;
	if (ext4_supported) formatOptions.push_back(MenuItem("ext4", _("4th version of standard linux filesystem"), _("New version of EXT, developed to superseed EXT3. The fastest journaling filesystem in most cases.")));
	formatOptions.push_back(MenuItem("ext3", _("Standard journaling Linux filesystem"), _("Seems to be a best balance between reliability and performance")));
	formatOptions.push_back(MenuItem("ext2", _("Standard Linux filesystem (without journaling)."), _("Old, stable, very fast (it doesn't use jounal), but less reliable")));
	formatOptions.push_back(MenuItem("xfs", _("Fast filesystem developed by SGI"), _("The best choise to store data (especially large files). Supports online defragmentation and other advanced features. Not recommended to use as root FS or /home")));
	formatOptions.push_back(MenuItem("jfs", _("Journaling filesystem by IBM"), _("Another good filesystem for general use.")));
	formatOptions.push_back(MenuItem("reiserfs", _("Journaling filesystem developed by Hans Reiser"), _("ReiserFS is a general-purpose, journaled computer file system designed and implemented by a team at Namesys led by Hans Reiser. ReiserFS is currently supported on Linux. Introduced in version 2.4.1 of the Linux kernel, it was the first journaling file system to be included in the standard kernel")));
	formatOptions.push_back(MenuItem("btrfs", _("B-Tree FS, new EXPERIMENTAL filesystem similar to ZFS. Separate /boot partition required to use it as root filesystem.")));
	formatOptions.push_back(MenuItem("nilfs2", _("NILFS2, new EXPERIMENTAL snapshot-based filesystem. Separate /boot partition required to use it as root filesystem.")));
	formatOptions.push_back(MenuItem("---", _("Do not format (use as is and keep data)")));
	string formatOption;

rootFormatMenu:
	if (predefinedFormat.empty()) {
		int menu_ret = ncInterface.showMenu(_("You have choosed ") + rootPartition + _(" as a root filesystem.\nWhich filesystem type do you want to format it?"), formatOptions);
		if (menu_ret==-1) return -1;
		else formatOption = formatOptions[menu_ret].tag;
	}
	else {
		bool valid=false;
		if (predefinedFormat == "noformat") {
			valid=true;
			predefinedFormat = "---";
		}
		for (size_t i=0; !valid && i<formatOptions.size(); i++) {
			if (predefinedFormat == formatOptions[i].tag) {
				valid=true;
			}
		}
		if (valid) formatOption = predefinedFormat;
		else ncInterface.showMsgBox(_("Predefined format options are incorrect: ") + predefinedFormat);
		
	}
	if (formatOption.empty())
	{
		return -1;
	}
	
	systemConfig.rootPartition = rootPartition;

	if (formatOption=="xfs")
	{
		if (!ncInterface.showYesNo(_("You have choosed XFS as root filesystem format.\nIt is not recommended, AND It is NOT SUPPORTED by GRUB bootloader. Also, if you install any bootloader on this partition, it will cause of TOTAL DATA LOSS on it.\nYou have to choose another option, or create a separate /boot partition, or use LILO as boot loader in MBR.\n\nAre you REALLY SURE you want to continue with this option?"))) goto rootFormatMenu;
	}
	if (formatOption!="---")
	{
		systemConfig.rootPartitionFormat=true;
		systemConfig.rootPartitionType = formatOption;
		mDebug("set to format " + rootPartition + " as " + formatOption);
		system("umount -l " + rootPartition + " 2>/dev/null >/dev/null");
	}

	else
	{
		systemConfig.rootPartitionFormat=false;
	}
		
	mDebug("end");
	return 0;

}

int setOtherPartitionItems()
{
	// Initializes partition list.
	vector<string> gp;
	vector<pEntry> pList_raw = getGoodPartitions(gp);
	systemConfig.otherMountFSTypes.clear();
	systemConfig.otherMounts.clear();
	systemConfig.otherMountFormat.clear();
	systemConfig.otherMountSizes.clear();
	for (size_t i=0; i<pList_raw.size(); i++)
	{
		systemConfig.otherMounts.push_back(TagPair(pList_raw[i].devname, pList_raw[i].size + "Mb, " + pList_raw[i].fstype + _(", not mounted")));
		systemConfig.otherMountSizes.push_back(pList_raw[i].size);
		systemConfig.otherMountFSTypes.push_back(pList_raw[i].fstype);
		systemConfig.otherMountFormat.push_back(false);
	}
	systemConfig.oldOtherFSTypes=systemConfig.otherMountFSTypes;
	return 0;
}

int setOtherOptions(string devName)
{
	// Sets an options for other partitions
	ncInterface.setSubtitle(_("Filesystem type and mountpoint options for ") + devName);
	string fstype_ret;
	string mountpoint_ret;
	vector<MenuItem> formatOptions;
	if (ext4_supported) formatOptions.push_back(MenuItem("ext4", _("4th version of standard linux filesystem"), _("New version of EXT, developed to superseed EXT3. The fastest journaling filesystem in most cases.")));
	formatOptions.push_back(MenuItem("ext3", _("Standard journaling Linux filesystem"), _("Seems to be a best balance between reliability and performance")));
	formatOptions.push_back(MenuItem("ext2", _("Standard Linux filesystem (without journaling)."), _("Old, stable, very fast (it doesn't use jounal), but less reliable")));
	formatOptions.push_back(MenuItem("xfs", _("Fast filesystem developed by SGI"), _("The best choise to store data (especially large files). Supports online defragmentation and other advanced features. Not recommended to use as root FS or /home")));
	formatOptions.push_back(MenuItem("jfs", _("Journaling filesystem by IBM"), _("Another good filesystem for general use.")));
	formatOptions.push_back(MenuItem("reiserfs", _("Journaling filesystem developed by Hans Reiser"), _("ReiserFS is a general-purpose, journaled computer file system designed and implemented by a team at Namesys led by Hans Reiser. ReiserFS is currently supported on Linux. Introduced in version 2.4.1 of the Linux kernel, it was the first journaling file system to be included in the standard kernel")));
	formatOptions.push_back(MenuItem("btrfs", _("B-Tree FS, new EXPERIMENTAL filesystem similar to ZFS. Separate /boot partition required to use it as root filesystem.")));
	formatOptions.push_back(MenuItem("nilfs2", _("NILFS2, new EXPERIMENTAL snapshot-based filesystem. Separate /boot partition required to use it as root filesystem.")));

	formatOptions.push_back(MenuItem("---", _("Do not format (use as is and keep data)")));

	for (size_t i=0; i<systemConfig.otherMounts.size(); i++)
	{
		if (systemConfig.otherMounts[i].tag == devName)
		{
			if (systemConfig.otherMounts[i].value[0]== '/') mountpoint_ret = systemConfig.otherMounts[i].value; 
			fstype_ret = systemConfig.otherMountFSTypes[i];
select_mp:
			mountpoint_ret = ncInterface.showInputBox(_("Enter mount point for ") + systemConfig.otherMounts[i].tag + _(", for example, /mnt/win_c"), mountpoint_ret);
			if (mountpoint_ret.empty())
			{
				// clearing
				systemConfig.otherMounts[i].value=systemConfig.otherMountSizes[i] + "Mb, " + systemConfig.oldOtherFSTypes[i] + _(", not mounted");
				systemConfig.otherMountFormat[i]=false;
				systemConfig.otherMountFSTypes[i]=systemConfig.oldOtherFSTypes[i];
				return -1;
			}
			if (mountpoint_ret[0]!='/')
			{
				ncInterface.showMsgBox(_("The mount point you specified ") + mountpoint_ret + _(" is incorrect. It should be an absolute path (it means, beginning with slash /), for example: /mnt/win_c"));
				goto select_mp;
			}
			systemConfig.otherMounts[i].value = mountpoint_ret;

			fstype_ret = ncInterface.showMenu2(_("Choose filesystem type for ") + systemConfig.otherMounts[i].tag,formatOptions, "---");
			if (fstype_ret.empty()) return -1;
			if (fstype_ret == "---")
			{
				systemConfig.otherMountFormat[i]=false;
			}
			else
			{
				systemConfig.otherMountFormat[i]=true;
				systemConfig.otherMountFSTypes[i]=fstype_ret;
			}
			return 0;
		}
	}
	// If we reach this point - this mean that nothing was found...
	mError("No such device " + devName);
	return -2;

}
void setTmpfsOption() {
	systemConfig.tmpfs_tmp = !ncInterface.showYesNo(_("If you have lots of RAM (at least 2 Gb), you can enable tmpfs for temporary files (/tmp partition). Enabling this may significantly increase performance in most cases.\nDo you want to mount /tmp as tmpfs?"), _("No"), _("Yes"));
}
int setOtherPartitions(bool simple)
{

	setOtherPartitionItems();
	ncInterface.setSubtitle(_("Other partitions mount options"));
	vector<MenuItem> menuItems;
	string ret;
	string jump;
	//TagPair tmpTag;
	MenuItem tmpTag;
other_part_menu_main:
	menuItems.clear();
	for (size_t i=0; i<systemConfig.otherMounts.size(); i++)
	{
		if (systemConfig.otherMounts[i].tag!=systemConfig.rootPartition && systemConfig.otherMounts[i].tag!=systemConfig.swapPartition)
		{
			tmpTag.tag = systemConfig.otherMounts[i].tag;
			tmpTag.value = systemConfig.otherMounts[i].value;
			if (tmpTag.value[0]=='/') 
				tmpTag.value += ", " + systemConfig.otherMountSizes[i] + "Mb, " + systemConfig.otherMountFSTypes[i] + _(", formatting: ") + doFormatString(systemConfig.otherMountFormat[i]);
			menuItems.push_back(tmpTag);
		}
	}
	if (menuItems.empty()) {
		if (!simple) ncInterface.showMsgBox(_("All partitions already used as swap or as root filesystem"));
		setTmpfsOption();
		return 0;
	}
	menuItems.push_back(MenuItem(_("All OK"), _("Continue")));
	ret = ncInterface.showMenu2(_("Select other partitions you want to mount:"),menuItems, jump);
	if (ret.empty()) return -1;
	if (ret == _("All OK")) {
		setTmpfsOption();
		return 0;
	}
	for (size_t i=0; i<menuItems.size(); i++)
	{
		if (ret==menuItems[i].tag)
		{
			if (i+1<menuItems.size()) 
			{ 
				jump=menuItems[i+1].tag;
			}
			else jump=ret;
			break;
		}
	}
	setOtherOptions(ret);
	goto other_part_menu_main;
}

string getLastError()
{
	string ret = ReadFile("/var/log/mpkg-lasterror.log");
	return ret;
}
bool formatPartition(string devname, string fstype)
{
	string fs_options;
	if (fstype=="jfs") fs_options="-q";
	if (fstype=="xfs") fs_options="-f -q";
	if (fstype=="reiserfs") fs_options="-q";
	if (!simulate)
	{
		string cmd = "umount -l " + devname +  " 2>/var/log/mpkg-lasterror.log ; mkfs -t " + fstype + " " + fs_options + " " + devname + " 2>> /var/log/mpkg-lasterror.log 1>>/var/log/mkfs.log";
		if (system(cmd)==0) return true;
		else return false;
	}
	return true;
}
int formatPartitions()
{
	ncInterface.setSubtitle(_("Formatting partitions"));
	ncInterface.showInfoBox(_("Formatting filesystems..."));
	// Do the actual format
	if (systemConfig.rootPartitionFormat)
	{
		ncInterface.showInfoBox(_("Formatting root partition ") + systemConfig.rootPartition + \
				_("\nFilesystem type: ") + systemConfig.rootPartitionType);
		if (!formatPartition(systemConfig.rootPartition, systemConfig.rootPartitionType)) 
		{
			ncInterface.showMsgBox(("An error occured while formatting root partition: \n")+getLastError());
			return -1;
		}
	}
	for (size_t i=0; i<systemConfig.otherMounts.size(); i++)
	{
		if (systemConfig.otherMountFormat[i])
		{
			ncInterface.showInfoBox(_("Formatting partition ") + systemConfig.otherMounts[i].tag + \
					_(", mount point: ") + systemConfig.otherMounts[i].value + _("\nFilesystem type: ") + systemConfig.otherMountFSTypes[i]);
			if (!formatPartition(systemConfig.otherMounts[i].tag, systemConfig.otherMountFSTypes[i]))
			{
				ncInterface.showMsgBox(_("An error occured while formatting partition ") + systemConfig.otherMounts[i].tag + ":\n" + getLastError());
				return -1;
			}
		}
	}
	return 0;

}



int mountPartitions()
{
	// Mount all HDD partitions
	mDebug("start");
	ncInterface.setSubtitle(_("Mounting partitions"));
	string mount_cmd;
	string mkdir_cmd;
	ncInterface.showInfoBox(_("Mounting root partition (")+systemConfig.rootPartition + ")");

	mkdir_cmd = "mkdir -p " + systemConfig.rootMountPoint + " 2>/var/log/mpkg-lasterror.log >/dev/null";
	mount_cmd = "mount " + systemConfig.rootPartition + " " + systemConfig.rootMountPoint + " 2>>/var/log/mpkg-lasterror.log >/dev/null";
	mDebug("creating root mount point: " + mkdir_cmd);
	mDebug("and mounting: " + mount_cmd);
	if (system(mkdir_cmd) !=0 || system(mount_cmd)!=0)
	{
		mDebug("mkdir or mount failed");
		ncInterface.showMsgBox(_("An error occured while mounting root filesystem:\n")+getLastError());
		abort();
	}
	else mDebug("root mkdir and mount OK");

	// Sorting mount points
	vector<int> mountOrder, mountPriority;
	for (size_t i=0; i<systemConfig.otherMounts.size(); i++)
	{
		mountPriority.push_back(0);
	}
	for (size_t i=0; i<systemConfig.otherMounts.size(); i++)
	{
		for (size_t t=0; t<systemConfig.otherMounts.size(); t++)
		{
			if (systemConfig.otherMounts[i].value.find(systemConfig.otherMounts[t].value)==0)
			{
				mountPriority[i]++;
			}
			if (systemConfig.otherMounts[i].value[0]!='/') mountPriority[i]=-1;
		}
	}
	for (size_t i=0; i<mountPriority.size(); i++)
	{
		for (size_t t=0; t<mountPriority.size(); t++)
		{
			if (mountPriority[t]==(int) i)
			{
				mountOrder.push_back(t);
			}
		}
	}
	if (mountPriority.size()!=systemConfig.otherMounts.size()) 
	{
		mError("mount code error, size mismatch: priority size = " + \
			IntToStr(mountPriority.size()) + \
			"queue size: " + IntToStr(systemConfig.otherMounts.size())); 
		abort();
	}

	// Mounting others...
	
	for (size_t i=0; i<mountOrder.size(); i++)
	{
		ncInterface.showInfoBox(_("Mounting other partitions: ") + systemConfig.otherMounts[mountOrder[i]].tag + " ["+ \
				systemConfig.otherMounts[mountOrder[i]].value+"]");
		mkdir_cmd = "mkdir -p " + systemConfig.rootMountPoint+systemConfig.otherMounts[mountOrder[i]].value + " 2>/var/log/mpkg-lasterror.log >/dev/null";
		if (systemConfig.otherMountFSTypes[mountOrder[i]]=="ntfs")  mount_cmd = "ntfs-3g -o force " + systemConfig.otherMounts[mountOrder[i]].tag + " " + systemConfig.rootMountPoint+systemConfig.otherMounts[mountOrder[i]].value + " 2>>/var/log/mpkg-lasterror.log >/dev/null";
		else mount_cmd = "mount " + systemConfig.otherMounts[mountOrder[i]].tag + " " + systemConfig.rootMountPoint+systemConfig.otherMounts[mountOrder[i]].value + " 2>>/var/log/mpkg-lasterror.log >/dev/null";
		if (systemConfig.otherMountFSTypes[mountOrder[i]]=="jfs") mount_cmd = "fsck " + systemConfig.otherMounts[mountOrder[i]].tag + " 2>>/var/log/mpkg-lasterror.log >/dev/null && " + mount_cmd;

		mDebug("Attempting to mkdir: " + mkdir_cmd);
	       	mDebug("and Attempting to mount: " + mount_cmd);	
		if (system(mkdir_cmd)!=0 || system(mount_cmd)!=0)
		{
			mDebug("error while mount");
			ncInterface.showMsgBox(_("An error occured while mounting partition ") + \
					systemConfig.otherMounts[mountOrder[i]].tag + "\n:"+getLastError());
			return -1;
		}
		else mDebug("mount ok");
	}
	ncInterface.showInfoBox(_("Moving package database to hard drive"));
	moveDatabaseToHdd();

	mDebug("end");
	return 0;
}



int initDatabaseStructure()
{
	deleteCore();
	if (system("rm -rf /var/mpkg >/dev/tty4 2>/dev/tty4; mkdir -p /var/mpkg >/dev/tty4 2>/dev/tty4 && mkdir -p /var/mpkg/scripts >/dev/tty4 2>/dev/tty4 && mkdir -p /var/mpkg/backup 2>/dev/tty4 >/dev/tty4 && mkdir -p /var/mpkg/cache 2>/dev/tty4 >/dev/tty4 && cp -f /packages.db /var/mpkg/packages.db 2>/dev/tty4 >/dev/tty4")!=0)
	{
		ncInterface.showMsgBox(_("An error occured while creating database structure"));
		return -1;
	}
	return 0;
}

int moveDatabaseToHdd()
{
	deleteCore();
	log_directory = "/mnt/var/log/";
	if (system("rm -rf /mnt/var/mpkg 2>/dev/tty4 >/dev/tty4; mkdir -p /mnt/var/log 2>/dev/tty4 >/dev/tty4; cp -R /var/mpkg /mnt/var/ 2>/dev/tty4 >/dev/tty4 && rm -rf /var/mpkg 2>/dev/tty4 >/dev/tty4 && ln -s /mnt/var/mpkg /var/mpkg 2>/dev/tty4 >/dev/tty4")!=0)
	{
		ncInterface.showMsgBox(_("An error occured while moving database to hard drive"));
		return -1;
	}
	if (_cmdOptions["ramwarp"]=="yes") {
		system("mkdir -p /mnt/.installer && mount -t tmpfs none /mnt/.installer && mv /mnt/var/mpkg/packages.db /mnt/.installer/packages.db && ln -s /mnt/.installer/packages.db /mnt/var/mpkg/packages.db");
	}
	return 0;
}

int mountMedia(string iso_image)
{
	mDebug("start");
	if (system("umount -l /var/log/mount  2>/dev/tty4 >/dev/tty4")==0) 
	{
		mDebug("successfully unmounted old mount");
		usedCdromMount = false;
	}
	else mDebug("cd wasn't mounted or unmount error. Processing to detection");
	
	ncInterface.setSubtitle(_("Searching for CD/DVD drive"));
	if (iso_image.empty()) ncInterface.showInfoBox(_("Performing CD/DVD drive autodetection..."));
	vector<string> devList = getCdromList();

	// Trying to mount
	if (!iso_image.empty()) {
		systemConfig.cdromDevice = iso_image;
		return 0;
	}
	string cmd;
	for (size_t i=0; i<devList.size(); i++)
	{
		mDebug("searching DVD at " + devList[i]);
		cmd = "mount -t iso9660 " + devList[i]+" /var/log/mount 2>/dev/tty4 >/dev/tty4";
		if (system(cmd.c_str())==0)
		{
			mDebug("Successfully found at " + devList[i]);
			ncInterface.showInfoBox(_("CD/DVD drive found at ") + devList[i]);
			system("rm -f /dev/cdrom 2>/dev/tty4 >/dev/tty4; ln -s " + devList[i] + " /dev/cdrom 2>/dev/tty4 >/dev/tty4");
			systemConfig.cdromDevice=devList[i];
			system("echo Found DVD-ROM drive: " + systemConfig.cdromDevice + " >/dev/tty4");
			createCore();
			core->set_cdromdevice(devList[i]);
			core->set_cdrommountpoint("/var/log/mount/");
			deleteCore();
			mDebug("end");
			return 0;
		}
	}
	mDebug("Drive not found. Proceeding to manual selection");
	// Failed? Select manually please
	if (!ncInterface.showYesNo(_("CD/DVD drive autodetection failed. Specify device name manually?"))) {
		mDebug("aborted");
		return -1;
	}
manual:
	mDebug("manual selection");
	string manualMount = ncInterface.showInputBox(_("Please, enter device name (for example, /dev/scd0):"), "/dev/");
	if (manualMount.empty())
	{
		return -1;
	}
	cmd = "mount -t iso9660 " + manualMount + " /var/log/mount 2>/var/log/mpkg-lasterror.log >/dev/tty4";
	if (system(cmd)!=0)
	{
		mDebug("manual mount error");
		ncInterface.showMsgBox(_("The drive you have specified has failed to mount:\n") + getLastError());
		goto manual;
	}

	mDebug("end");
	return 0;
}
bool showLicense()
{
	if (licenseAccepted) return true;
	ncInterface.setSubtitle(_("License agreement"));
	return ncInterface.showText(ReadFile("/license." + systemConfig.lang));
	
}
void writeFstab()
{
	mDebug("start");
	string data;
	if (!systemConfig.swapPartition.empty()) data = "# " + systemConfig.swapPartition + "\nUUID="+getUUID(systemConfig.swapPartition) + "\tswap\tswap\tdefaults\t0 0\n";

	data+= "# " + systemConfig.rootPartition + "\nUUID=" + getUUID(systemConfig.rootPartition) + "\t/\t" + systemConfig.rootPartitionType + "\tdefaults\t1 1\n";
	/*
	data += (string) "devpts\t/dev/pts\tdevpts\tgid=5,mode=620\t0 0\n" + \
	(string) "proc\t/proc\tproc\tdefaults\t0 0\n";
	*/
	string options="defaults";
	string fstype="auto";
	
	// Sorting
	vector<int> mountOrder, mountPriority;
	for (size_t i=0; i<systemConfig.otherMounts.size(); i++)
	{
		mountPriority.push_back(0);
	}
	for (size_t i=0; i<systemConfig.otherMounts.size(); i++)
	{
		for (size_t t=0; t<systemConfig.otherMounts.size(); t++)
		{
			if (systemConfig.otherMounts[i].value.find(systemConfig.otherMounts[t].value)==0)
			{
				mountPriority[i]++;
			}
			if (systemConfig.otherMounts[i].value[0]!='/') mountPriority[i]=-1;
		}
	}
	for (size_t i=0; i<mountPriority.size(); i++)
	{
		for (size_t t=0; t<mountPriority.size(); t++)
		{
			if (mountPriority[t]== (int) i)
			{
				mountOrder.push_back(t);
			}
		}
	}
	if (mountPriority.size()!=systemConfig.otherMounts.size()) 
	{
		mError("mount code error, size mismatch: priority size = " + \
			IntToStr(mountPriority.size()) + \
			"queue size: " + IntToStr(systemConfig.otherMounts.size())); 
		abort();
	}
	// Storing data
	for (size_t i=0; i<mountOrder.size(); i++)
	{
		options="defaults";
#ifdef AUTO_FS_DETECT
		fstype="auto";
#else
		fstype=systemConfig.otherMountFSTypes[mountOrder[i]];
#endif
		if (fstype=="hfs+") fstype = "hfsplus";
		if (systemConfig.otherMountFSTypes[mountOrder[i]].find("fat")!=std::string::npos)
		{
			options="rw,codepage=866,iocharset=utf8,umask=000,showexec,quiet";
			fstype="vfat";
		}
		if (systemConfig.otherMountFSTypes[mountOrder[i]].find("ntfs")!=std::string::npos)
		{
			options="locale=ru_RU.utf8,umask=000";
			fstype="ntfs-3g";
//			options="nls=utf8,umask=000,rw";
//			fstype="ntfs";
		}
		data += "# " + systemConfig.otherMounts[mountOrder[i]].tag + "\nUUID="+getUUID(systemConfig.otherMounts[mountOrder[i]].tag) + "\t" + systemConfig.otherMounts[mountOrder[i]].value + "\t"+fstype+"\t" + options + "\t1 1\n";
	}
	mDebug("data is:\n"+data);
	data += "\n# Required for X shared memory\nnone\t/dev/shm\ttmpfs\tdefaults\t0 0\n";
	if (systemConfig.tmpfs_tmp) data += "\n# /tmp as tmpfs\nnone\t/tmp\ttmpfs\tdefaults\t0 0\n";
	
	if (!simulate) WriteFile(systemConfig.rootMountPoint + "/etc/fstab", data);
	mDebug("end");
}

bool setPasswd(string username, string passwd) {
	string tmp_file = "/mnt/tmp/wtf";
	string data = passwd + "\n" + passwd + "\n";
	WriteFile(tmp_file, data);
	string passwd_cmd = "#!/bin/sh\ncat /tmp/wtf | passwd " + username+" > /dev/null 2>/dev/tty4\n";
	WriteFile("/mnt/tmp/run_passwd", passwd_cmd);
	int ret = system("chroot /mnt sh /tmp/run_passwd");
	for (size_t i=0; i<data.size(); i++) {
		data[i]=' ';
	}
	WriteFile(tmp_file, data);
	unlink(tmp_file.c_str());
	unlink("/mnt/tmp/run_passwd");
	if (ret == 0) return true;
	return false;
}

bool addUser(string username) {
	//string extgroup="audio,cdrom,disk,floppy,lp,scanner,video,wheel"; // Old default groups
	string extgroup="audio,cdrom,floppy,video,netdev,plugdev,power"; // New default groups, which conforms current guidelines
	system("chroot /mnt /usr/sbin/useradd -d /home/" + username + " -m -g users -G " + extgroup + " -s /bin/bash " + username + " 2>/dev/tty4 >/dev/tty4");
	system("chroot /mnt chown -R " + username+":users /home/"+username + " 2>/dev/tty4 >/dev/tty4");
	system("chmod 711 /mnt/home/" + username + " 2>/dev/tty4 >/dev/tty4");
	return true;
}

bool setUserPasswordPwd(string username) {
	ncInterface.uninit();
	system("echo '" + string(_("Setting new password for user ")) + username + "'");
	while (system("LC_ALL=" + systemConfig.lang + " chroot /mnt passwd " + username)!=0) {

	}
	return 0;
}
bool setUserPassword(string username) {
	ncInterface.setSubtitle(_("Password input"));
	string passwd;
	string passwd2;
get_root_passwd:
	passwd.clear();
	passwd2.clear();
	if (ncInterface.showInputBox(_("Enter password for user ") + username + ":", &passwd, true)) {
		if (ncInterface.showInputBox(_("Repeat password for user ") + username + _(" again:"), &passwd2, true)) {
			if (passwd == passwd2) {
				if (!setPasswd(username, passwd)) {
					ncInterface.showMsgBox(_("Error: failed to set password, internal error"));
					return false;
				}
			}
			else {
				ncInterface.showMsgBox(_("Passwords doesn't match"));
				goto get_root_passwd;
			}
		}
		else return false;
	}
	return false;
}

bool setRootPassword()
{
	//return setUserPassword("root");
	return setUserPasswordPwd("root");
}

bool addNewUsers() {
	string username;
	string add_else_user;
add_new_user:
	username.clear();
	ncInterface.setSubtitle(_("Creating users"));
	while (ncInterface.showYesNo(_("Add ") + add_else_user + _("user?"))) {
		if (!ncInterface.showInputBox(_("Enter user name:"), &username)) goto add_new_user;
		else {
			if (username.find_first_of("1234567890")==0) {
				ncInterface.showMsgBox(_("User name cannot start from number, enter correct name."));
				goto add_new_user;
			}
			addUser(username);
			//setUserPassword(username);
			setUserPasswordPwd(username);
			username.clear();
			add_else_user = _("another ");
		}
	}
	return true;
	
}
void installPatchedATI(const string& f) {
	ncInterface.showMsgBox(_("After installing this driver, you will see an error message due to incompatibility with newer kernels. Ignore this, and setup will apply patch automatically after driver installation"));
	system("chroot /mnt sh /usr/src/drivers/" + f);
	system("cp -R /var/log/mount/drivers/ati/ati-patch /mnt/usr/src/drivers/");
	system("chroot /mnt sh /usr/src/drivers/ati-patch/install_patch.sh");
}
void installAdditionalDrivers() {
	// First, detect hardware for which we have to install drivers.
	string tmp_hw = get_tmp_file();
	system("lspci > " + tmp_hw);
	vector<string> hw = ReadFileStrings(tmp_hw);
	// Check for VirtualBox
	bool hasVirtualBox=false;
	//int hasNvidia=-1;
	int hasRadeon=-1;
	for(size_t i=0; i<hw.size(); ++i) {
		if (hw[i].find("VirtualBox")!=std::string::npos) hasVirtualBox = true;
	//	if (hw[i].find("VGA compatible controller")!=std::string::npos && hw[i].find("nVidia")!=std::string::npos) hasNvidia = i;
		if (hw[i].find("VGA compatible controller")!=std::string::npos && hw[i].find("Radeon")!=std::string::npos) hasRadeon = i;
	}
	string driverDir = "/var/log/mount/drivers/";
	ncInterface.setSubtitle(_("Installing additional drivers"));
	if (hasVirtualBox /* || hasNvidia!=-1 */ || hasRadeon!=-1) {
		system("mount /dev/cdrom /var/log/mount  2>/dev/null >/dev/null");
		//bool tryFoundDrivers = true;
		if (!FileExists(driverDir)) return;
		/*while (!FileExists(driverDir) && tryFoundDrivers) {
			string text;
		        if (hasVirtualBox) text = _("You are using VirtualBox, but guest additions not found in '") + driverDir + _("' . Do you want to specify directory manually?");
			else text = _("Next hardware detected:\n");
//			if (hasNvidia!=-1) text += hw[hasNvidia] + "\n";
			if (hasRadeon!=-1) text += hw[hasRadeon] + "\n";
			text += _("but no drivers detected in '") + driverDir + _("' . Do you want to specify directory manually?");
			if (ncInterface.showYesNo(text)) driverDir = ncGetOpenDir(driverDir);
			else tryFoundDrivers = false;
		}*/
	}
	if (hasVirtualBox) {
		ncInterface.uninit();
#ifndef X86_64
		system("mkdir -p /mnt/usr/src/drivers && cp /var/log/mount/drivers/VBoxLinuxAdditions-x86.run /mnt/usr/src/drivers");
#else
		system("mkdir -p /mnt/usr/src/drivers && cp /var/log/mount/drivers/VBoxLinuxAdditions-amd64.run /mnt/usr/src/drivers");
#endif

		system("chroot /mnt mount -t proc none /proc");

#ifndef X86_64
		system("chroot /mnt sh /usr/src/drivers/VBoxLinuxAdditions-x86.run");
#else
		system("chroot /mnt sh /usr/src/drivers/VBoxLinuxAdditions-amd64.run");
#endif
	}
/*	if (hasNvidia!=-1) {
		system("mount /dev/cdrom /var/log/mount 2>/dev/null >/dev/null");
		vector<string> nvlist = getDirectoryList("/var/log/mount/drivers/nvidia/");
		vector<MenuItem> menuItems;
		for (size_t i=0; i<nvlist.size(); ++i) {
#ifndef X86_64
			if (nvlist[i].find("NVIDIA-Linux-x86-")==std::string::npos) continue;
#else
			if (nvlist[i].find("NVIDIA-Linux-x86_64")==std::string::npos) continue;
#endif

			menuItems.push_back(MenuItem(IntToStr(i), nvlist[i]));
		}
		menuItems.push_back(MenuItem(_("Skip"), _("Do not install driver")));
		string ret = ncInterface.showMenu2(_("Videocard detected: ") + hw[hasNvidia] + _("\nChoose driver for it:\n185.хх and 190.xx - GeForce 6 and newer\n173.xx - GeForce FX\n96.xx - GeForce 2MX, 3, 4, 4MX\n7х - GeForce2 and older"), menuItems);
		if (!ret.empty() && ret != _("Skip")) {
			ncInterface.uninit();
			system("mkdir -p /mnt/usr/src/drivers && cp /var/log/mount/drivers/nvidia/" + nvlist[atoi(ret.c_str())] + " /mnt/usr/src/drivers/");
		       	system("chroot /mnt mount -t proc none /mnt");
		      	system("chroot /mnt sh /usr/src/drivers/" + nvlist[atoi(ret.c_str())] + " -qNX --no-runlevel-check");
		}
	}
*/
	// We disable ATI proprietary driver installation, because this piece of crap doesn't work with xorg-server >= 1.7.x
	/*if (hasRadeon!=-1) {
		system("mount /dev/cdrom /var/log/mount 2>/dev/null >/dev/null");
		vector<string> rvlist = getDirectoryList("/var/log/mount/drivers/ati/");
		vector<MenuItem> menuItems;
		for (size_t i=0; i<rvlist.size(); ++i) {
			if (rvlist[i].find("ati-driver-installer-")==std::string::npos) continue;
			menuItems.push_back(MenuItem(IntToStr(i), rvlist[i]));
		}
		if (!rvlist.empty()) {
			menuItems.push_back(MenuItem(_("Skip"), _("Do not install driver")));
			string ret = ncInterface.showMenu2("Videocard detected: " + hw[hasRadeon] + _("\nChoose driver: for cards based prior to R600 (9xxx, X300, X550, X600, X700, X1xxx, Xpress, X2100), select 9-3, for newer cards - select latest version:"), menuItems);
			if (!ret.empty() && ret != _("Skip")) {
				//system("mount /dev/cdrom /var/log/mount");
				ncInterface.uninit();
				system("mkdir -p /mnt/usr/src/drivers && cp /var/log/mount/drivers/ati/" + rvlist[atoi(ret.c_str())] + " /mnt/usr/src/drivers/");
				system("chroot /mnt mount -t proc none /mnt");
				// If we have a driver version prior 9-3, apply special installation procedure
				if (rvlist[atoi(ret.c_str())].find("9-3")!=std::string::npos) installPatchedATI(rvlist[atoi(ret.c_str())]);
				else system("chroot /mnt sh /usr/src/drivers/" + rvlist[atoi(ret.c_str())]);
			}
		}
	}*/

}
bool setHostname() {
	string hostname = ncInterface.showInputBox(_("Enter hostname (for example, agilia):"));
	if (hostname.empty()) return false;
	string netname = ncInterface.showInputBox(_("Enter network name (for example, example.net):"), "example.net");
	if (netname.empty()) return false;
	WriteFile(systemConfig.rootMountPoint + "/etc/HOSTNAME", hostname + "." + netname + "\n");
	string hosts = ReadFile(systemConfig.rootMountPoint + "/etc/hosts");
	strReplace(&hosts, "darkstar", hostname);
	strReplace(&hosts, "example.net", netname);
	WriteFile(systemConfig.rootMountPoint + "/etc/hosts", hosts);
	return true;
}
void enablePlymouth(bool enable) {
	if (enable) system("chroot /mnt rc-update add plymouth default");
	else system("chroot /mnt rc-update del plymouth plymouth");
}

void askPlymouth() {
	// Asking for plymouth
	if (FileExists(systemConfig.rootMountPoint + "/etc/rc.d/rc.plymouth")) {
		enablePlymouth(ncInterface.showYesNo(_("AgiliaLinux has EXPERIMENTAL support for graphical boot splash implemented using Plymouth.\nIf your hardware supports KMS (kernel modesetting), you can try to enable it now.\nIt seems to work fine, but it wasn't tested well and may cause problems.\nIn any case, you can change your choice later by changing permissions on /etc/rc.d/rc.plymouth file.\n\nDo you want to enable experimental splash screen?")));
	}
}

void setDefaultRunlevels() {
// We don't know which of them are in system in real, but let's try them all
	system("chroot /mnt rc-update add sysfs sysinit 2>/dev/tty4 >/dev/tty4");
	system("chroot /mnt rc-update add udev sysinit 2>/dev/tty4 >/dev/tty4");
	system("chroot /mnt rc-update add consolefont default 2>/dev/tty4 >/dev/tty4");
	system("chroot /mnt rc-update add hald default 2>/dev/tty4 >/dev/tty4");
	system("chroot /mnt rc-update add sysklogd default 2>/dev/tty4 >/dev/tty4");
	system("chroot /mnt rc-update add dbus default 2>/dev/tty4 >/dev/tty4");
	system("chroot /mnt rc-update add sshd default 2>/dev/tty4 >/dev/tty4");
	system("chroot /mnt rc-update add alsasound default 2>/dev/tty4 >/dev/tty4");
	system("chroot /mnt rc-update add acpid default 2>/dev/tty4 >/dev/tty4");
	system("chroot /mnt rc-update add cupsd default 2>/dev/tty4 >/dev/tty4");
}



int performConfig(bool simple)
{
	ncInterface.setSubtitle(_("Initial system configuration"));
	generateLangSh();
#ifdef USE_X11_HAL
	//xorgSetLangHAL();
	xorgSetLangHALEx();
#else
	xorgSetLang();
#endif
	generateIssue();
	writeFstab();
	buildInitrd();
	setDefaultRunlevels();
	vector<MenuItem> networkManagers;
	string selectedNetworkManager;
	if (FileExists(systemConfig.rootMountPoint + "/usr/sbin/NetworkManager")) {
		networkManagers.push_back(MenuItem("NetworkManager", _("NetworkManager is default network connection manager for any desktop.")));
		system("chmod -x " + systemConfig.rootMountPoint + "/etc/rc.d/rc.networkmanager >/dev/tty4 2>/dev/tty4");
	}
	if (FileExists(systemConfig.rootMountPoint+"/usr/sbin/wicd")) {
		networkManagers.push_back(MenuItem("wicd", _("Wicd is an open source wired and wireless network manager")));
		system("chmod -x " + systemConfig.rootMountPoint + "/etc/rc.d/rc.wicd >/dev/tty4 2>/dev/tty4");
	}
	if (FileExists(systemConfig.rootMountPoint + "/sbin/netconfig")) networkManagers.push_back(MenuItem("netconfig", _("Generic netconfig, Slackware default network settings manager")));


	if (!bootConfig.loaderType.empty()) ncInterface.showInfoBox(_("Installing bootloader: ") + bootConfig.loaderType);
	vector<MenuItem> menuItems;
	menuItems.push_back(MenuItem("0", _("See installation log")));
	menuItems.push_back(MenuItem("1", _("Change settings")));
	menuItems.push_back(MenuItem("2", _("Edit configuration file manually")));
	menuItems.push_back(MenuItem("3", _("Open chroot console")));
	menuItems.push_back(MenuItem("4", _("Continue without installing bootloader")));
try_install_boot:
	if (bootConfig.loaderType == "GRUB2") {
		while (!grub2config()) {
			string ret;
		       	while (ret != "4") {
				ret = ncInterface.showMenu2(_("GRUB2 installation failed. Your system will NOT boot without bootloader"), menuItems);
				if (ret == "0") {
					ncInterface.showText(ReadFile("/tmp/grubinstall-logfile"), "ОК", "ОК");
				}
				if (ret == "1") {
					setupBootloaderOptions();
					goto try_install_boot;
					break;
				}
				if (ret == "2") {
					ncInterface.uninit();
					system("chroot /mnt vi /boot/grub/grub.cfg");
				}
				if (ret == "3") {
					ncInterface.uninit();
					system("chroot /mnt");
				}
			}
			if (ret == "4") break;
		}
	}

	if (bootConfig.loaderType == "GRUB") {
		while (!grubconfig()) {
			string ret;
		       	while (ret != "4") {
				ret = ncInterface.showMenu2(_("GRUB installation failed. Your system will NOT boot without bootloader"), menuItems);
				if (ret == "0") {
					ncInterface.showText(ReadFile("/tmp/grubinstall-logfile"), "ОК", "ОК");
				}
				if (ret == "1") {
					setupBootloaderOptions();
					goto try_install_boot;
					break;
				}
				if (ret == "2") {
					ncInterface.uninit();
					system("chroot /mnt vi /boot/grub/menu.lst");
				}
				if (ret == "3") {
					ncInterface.uninit();
					system("chroot /mnt");
				}
			}
			if (ret == "4") break;
		}
	}
	if (bootConfig.loaderType == "LILO") {
		while (!liloconfig()) {
			string ret;
		       	while (ret != "4") {
				ret = ncInterface.showMenu2(_("Installation of LILO failed. Your system will NOT boot without bootloader"), menuItems);
				if (ret == "0") {
					ncInterface.showText(ReadFile("/tmp/liloinstall_logfile"), "OK", "OK");
				}
				if (ret == "1") {
					setupBootloaderOptions();
					goto try_install_boot;
					break;
				}
				if (ret == "2") {
					ncInterface.uninit();
					system("chroot /mnt vi /etc/lilo.conf");
				}
				if (ret == "3") {
					ncInterface.uninit();
					system("chroot /mnt");
				}
			}
			if (ret == "4") break;

		}
	}
	if (bootConfig.loaderType == "MSDOS") msdosconfig();
	

	mDebug("start");
	setenv("ROOT_DEVICE", systemConfig.rootPartition.c_str(), 1);
	setenv("COLOR", "on",1);
	WriteFile("/var/log/setup/tmp/SeTT_PX", systemConfig.rootMountPoint+"\n");
	WriteFile("/var/log/setup/tmp/SeTrootdev", systemConfig.rootPartition+"\n");
	WriteFile("/var/log/setup/tmp/SeTcolor","on\n");
	
	if (!networkManagers.empty()) {
		ncInterface.setSubtitle(_("Network setup"));
	selectNetManagerMode:
		selectedNetworkManager = ncInterface.showMenu2(_("Please select preferred network manager:"), networkManagers);
		
		if (selectedNetworkManager == "NetworkManager") {
			system("chmod +x " + systemConfig.rootMountPoint + "/etc/rc.d/rc.networkmanager >/dev/tty4 2>/dev/tty4");
			system("chroot /mnt rc-update add networkmanager default 2>/dev/tty4 >/dev/tty4");
			if (!setHostname()) goto selectNetManagerMode;
		}
		if (selectedNetworkManager == "wicd") {
			system("chmod +x " + systemConfig.rootMountPoint + "/etc/rc.d/rc.wicd >/dev/tty4 2>/dev/tty4");
			system("chroot /mnt rc-update add wicd default 2>/dev/tty4 >/dev/tty4");
			if (!setHostname()) goto selectNetManagerMode;
		}
		if (selectedNetworkManager == "netconfig") {
			ncInterface.uninit();
			system("LC_ALL=" + systemConfig.lang +  " chroot " + systemConfig.rootMountPoint + " /sbin/netconfig");
		}
	}
	ncInterface.setSubtitle(_("Initial system configuration"));
	ncInterface.showInfoBox(_("Configuring your system..."));
	// Skip some question in simple mode
	
	ncInterface.uninit(); // Disable ncurses

	system("chmod -x " + systemConfig.rootMountPoint + "/etc/rc.d/rc.pcmcia 2>/dev/null >/dev/null"); // skip PCMCIA
	if (simple) {
		system("AUTOCONFIG=imps2 LC_ALL=" + systemConfig.lang + " " + systemConfig.rootMountPoint + "/var/log/setup/setup.mouse " + systemConfig.rootMountPoint); // Set defaults for GDM
		system("chmod -x " + systemConfig.rootMountPoint + "/var/log/setup/setup.mouse"); // skip GPM config in future: already configured
		system("chmod -x " + systemConfig.rootMountPoint + "/var/log/setup/setup.services"); // skip services config. Defaults are ok for 90% of users, and another 10% can set this manually later or use advanced install mode
		system("chmod -x " + systemConfig.rootMountPoint + "/var/log/setup/setup.80.make-bootdisk"); // We never shipped USB boot creation by default, neither will now. Let it for advanced users and ones who failed to install bootloader

	}

	if (system("LC_ALL=" + systemConfig.lang + " /usr/lib/setup/SeTconfig")==0)
	{
		mDebug("Config seems to be ok");
	}
	else 
	{
		ncInterface.setSubtitle(_("Initial system configuration"));
		ncInterface.showMsgBox(_("An error occured while performing initial system configuration. Find out why it happened, and restart installation."));
		mDebug("Config seems to be failed");
		return -1;
	}
	setRootPassword();
	addNewUsers();
	installAdditionalDrivers();
#ifndef USE_X11_HAL
	enableComposite("/mnt/etc/X11/xorg.conf");
#endif
	/*if (systemConfig.setupModeI==PKGSET_XFCE) {
		system("( cd /mnt/etc/X11/xinit ; rm xinitrc ; ln -s xinitrc.xfce xinitrc )");
	}*/
	if (FileExists(systemConfig.rootMountPoint + "/usr/bin/X")) {
		if (FileExists(systemConfig.rootMountPoint + "/usr/bin/kdm") ||
		    FileExists(systemConfig.rootMountPoint + "/usr/bin/xdm") ||
    		    FileExists(systemConfig.rootMountPoint + "/usr/sbin/gdm") ||
    		    FileExists(systemConfig.rootMountPoint + "/usr/sbin/lxdm") ||
		    FileExists(systemConfig.rootMountPoint + "/usr/bin/slim")) {
			if (ncInterface.showYesNo(_("Enable X11 login by default?"))) {
				setDefaultRunlevel("4");
				askPlymouth();
			}
			else enablePlymouth(false);
		}
	}
	// If prelink is installed, let's prelink all libraries and binaries to speed up the system:
	if (systemConfig.tryPrelink && FileExists(systemConfig.rootMountPoint + "/usr/sbin/prelink")) {
		ncInterface.showInfoBox(_("Prelinking...\nIt will take about 3-5 minutes, depending on your hardware"));
		system("chroot " + systemConfig.rootMountPoint + " prelink -am > /dev/tty4 2>/dev/tty4");
	}
	mDebug("end");
	return 0;
}

void syncFS()
{
	mDebug("start");
	system("sync >/dev/tty4 2>/dev/tty4");
	// FIXME: should be called ONLY in case if installing from CD/DVD!
	if (noEject) system("umount " + systemConfig.cdromDevice + " >/dev/null 2>/dev/null");
	else {
		system("eject " + systemConfig.cdromDevice + " 2>/dev/null >/dev/null");
		system("echo EJECTING in syncFS > /dev/tty4");
	}
	usedCdromMount = false;
	mDebug("end");
}

void cleanup()
{
	createCore();
	core->clean_cache(true);
}

bool showFinish(time_t totalTime, time_t pkgTotalTime)
{
	unlockDatabase();
	deleteCore();
	ncInterface.setSubtitle(_("Installation finished"));
	if (verbose) {
		ncInterface.showMsgBox(_("Setup finished. Time:\n") + IntToStr(totalTime/60) + "/" + IntToStr(pkgTotalTime/60));
	}
	return ncInterface.showYesNo(_("AgiliaLinux installation finished successfully. Reboot now?"));
}



CustomPkgSet getCustomPkgSet(const string& name) {
	vector<string> data = ReadFileStrings("/tmp/setup_variants/" + name + ".desc");
	CustomPkgSet ret;
	ret.name = name;
	string locale = systemConfig.lang;
	if (locale.size()>2) locale = "[" + locale.substr(0,2) + "]";
	else locale = "";
	string gendesc, genfull;
	for (size_t i=0; i<data.size(); ++i) {
		if (data[i].find("desc" + locale + ": ")==0) ret.desc = getParseValue("desc" + locale + ": ", data[i], true);
		if (data[i].find("desc: ")==0) gendesc = getParseValue("desc: ", data[i], true);
		if (data[i].find("full" + locale + ": ")==0) ret.full = getParseValue("full" + locale + ": ", data[i], true);
		if (data[i].find("full: ")==0) genfull = getParseValue("full: ", data[i], true);
	}
	if (ret.desc.empty()) ret.desc = gendesc;
	if (ret.full.empty()) ret.full = genfull;
	return ret;
}

// Get setup variants
void getCustomSetupVariants(const vector<string>& rep_list) {
	string tmpfile = get_tmp_file();
	system("rm -rf /tmp/setup_variants");
	system("mkdir -p /tmp/setup_variants");
	string path;
	customPkgSetList.clear();
	for (size_t z=0; z<rep_list.size(); ++z) {
		path = rep_list[z];
		CommonGetFile(path + "/setup_variants.list", tmpfile);
		vector<string> list = ReadFileStrings(tmpfile);
		vector<CustomPkgSet> ret;
		for (size_t i=0; i<list.size(); ++i) {
			CommonGetFile(path + "/setup_variants/" + list[i] + ".desc", "/tmp/setup_variants/" + list[i] + ".desc");
			CommonGetFile(path + "/setup_variants/" + list[i] + ".list", "/tmp/setup_variants/" + list[i] + ".list");
			customPkgSetList.push_back(getCustomPkgSet(list[i]));
		}
	}
}
int packageSelectionMenu(string predefined, bool simple)
{
	// Let's check package dependencies here.
	PACKAGE_LIST *checkList = new PACKAGE_LIST;
	SQLRecord sqlS;
	createCore();
	core->get_packagelist(sqlS, checkList);
	for (size_t i=0; i<checkList->size(); ++i) {
		checkList->get_package_ptr(i)->set_action(ST_INSTALL, "check");
	}
	DependencyTracker *tracker = new DependencyTracker(core->db);
	int rcount = tracker->renderDependenciesInPackageList(checkList);
	if (rcount) {
		ncInterface.setSubtitle(_("Depencency errors"));
		ncInterface.showInfoBox(_("Rendering error list: ") + IntToStr(rcount) + " errors");
		ncInterface.showMsgBox(_("Unresolved dependencies: ") + IntToStr(rcount) + "\n" + depErrorTable.print());
		delete tracker;
		delete checkList;
		deleteCore();
		return 1;

	}
	delete tracker;
	delete checkList;
	deleteCore();

	vector<MenuItem> menuItems;
	string ret;
	bool mark;
	PACKAGE *p;
	string ins_type;

	/*
	// BFS kernel set
	bool hasGenericKernel=false;
	bool hasBFSKernel=false;
	//int num;
	int numAlt;
	vector<string> altKernelPkgList;
	altKernelPkgList.push_back("kernel");
	altKernelPkgList.push_back("kernel-modules");
	altKernelPkgList.push_back("kernel-source");*/

	string installset_filename;
	vector<string> installset_contains;
pkg_select_init:
	menuItems.clear();
	installset_contains.clear();
	installset_filename.clear();
	ncInterface.setSubtitle(_("Package selection"));//"Выбор пакетов для установки");
	ins_type=systemConfig.setupMode;
	if (systemConfig.setupMode==_("Advanced setup")) menuItems.push_back(MenuItem("0", _("Edit current package set")));
	
	
	menuItems.push_back(MenuItem(_("FULL"), _("Full installation (recommended)"), _("Full package set for any purposes: workstation, office, home, education, development, and so on. You can remove unneeded components at any time after installation.")));
	if (!simple || enable_setup_debug) menuItems.push_back(MenuItem(_("MINIMAL"), _("Minimal installation"), _("A minimal package set. It is enough to boot and install packages, but will be a very difficult process. Not recommended unless you know what you are doing")));
	// TODO: Insert here custom variants
	for (size_t i=0; i<customPkgSetList.size(); ++i) {
		menuItems.push_back(MenuItem(customPkgSetList[i].name, customPkgSetList[i].desc, customPkgSetList[i].full));
	}
	if (!simple) menuItems.push_back(MenuItem(_("CUSTOM"), _("Custom installation"), _("Custom installation. You can manually select what packages to install.")));
	menuItems.push_back(MenuItem(_("FILE"), _("Package list from file..."), _("Install package set by list predefined in your file")));
	if (predefined.empty())	ret = ncInterface.showMenu2(_("Choose package set to install"), menuItems, _("FULL"));
	else ret = predefined;
	
	if (ret.empty()) return 0;

	if (ret == _("FILE")) {
		if (ncInterface.showYesNo(_("Mount partition?"))) ncGetMountPartition(_("Select partition which contains package list file:"));
		ncInterface.setSubtitle(_("Select file with package list:"));
		installset_filename = ncInterface.ncGetOpenFile();
		if (installset_filename.empty()) goto pkg_select_init;
	}
	ncInterface.showInfoBox(_("Importing package list for installation..."));
	if (ret == _("MINIMAL")) {
		ins_type = _("Minimal installation");
		installset_filename = "/usr/lib/setup/predefined/minimal";
	}
	else if (ret == _("FULL")) {
			ins_type = _("Full installation");
	}
	else if (ret == _("CUSTOM")) {
			ins_type = _("Custom installation");
	}
	else if (ret == _("FILE")) {
			ins_type = _("Package list from file: ") + getFilename(installset_filename);
	}
	else {
		// Some custom variant, get parsed data
		size_t num=customPkgSetList.size();
		for (size_t i=0; i<customPkgSetList.size(); ++i) {
			if (ret==customPkgSetList[i].name) {
				num = i;
				break;
			}
		}
		if (num>=customPkgSetList.size()) {
			ncInterface.showMsgBox(_("Cannot find predefined set"));
			return 1;
		}
		installset_filename = "/tmp/setup_variants/" + customPkgSetList[num].name + ".list";
		ins_type = customPkgSetList[num].name;
	
	}
	systemConfig.setupMode=ins_type;
	if (ret == _("FILE") && !FileExists(installset_filename)) {
		ncInterface.showMsgBox(_("Package list file ") + installset_filename + _(" not found"));
		goto pkg_select_init;
	}

	if (!installset_filename.empty() && FileExists(installset_filename)) {
		vector<string> versionz; // It'll be lost after, but we can't carry about it here: only one version is available.
		parseInstallList(ReadFileStrings(installset_filename), installset_contains, versionz);
		if (installset_contains.empty()) {
			ncInterface.showMsgBox(_("Package list ") + installset_filename + _(" contains 0 (zero) packages, I cannot install by this list"));
			goto pkg_select_init;
		}
	}
	createCore();
	SQLRecord sqlSearch;
	core->clean_queue();
	if (!list_initialized) 
	{
		core->get_packagelist(sqlSearch, &i_availablePackages);
		list_initialized=true;
	}
	core->get_available_tags(&i_tagList);
	/*if (i_availablePackages.getPackageNumberByName("kernel")!=-1) hasGenericKernel=true;
	if (i_availablePackages.getPackageNumberByName("kernel-bfs")!=-1) hasBFSKernel=true;

	string alternateKernel;
	if (!hasGenericKernel && hasBFSKernel) alternateKernel = "bfs";
	if (hasGenericKernel && hasBFSKernel) {
		if (ncInterface.showYesNo(_("Do you want to try a BFS kernel?\nThis kernel has patched to use Con Kolivas's BFS scheduler, which significantly improves desktop performance. It also improves server performance as well in some cases, if your server contains no more than 16 cores. This is a  experimental feature, so it is not recommended to use this in mission-critial cases."))) alternateKernel="bfs";
	}*/


	// Now, two methods of package selection exists: by file or by tag mask. File is preferred if available
	if (!installset_contains.empty()) {
		vector<string> errorList;
		int set_ret = core->install(installset_contains, NULL, NULL, &errorList);
		if (set_ret) {
			string errText;
			for (size_t i=0; i<errorList.size(); ++i) {
				errText += errorList[i] + "\n";
			}
			if (!ncInterface.showText(_("Note: some of requested packages are unavailable and will not be installed:\n\n") + errText, "Continue", _("Select another list"))) goto pkg_select_init;
		}
		else {
			core->commit(true);
			core->get_packagelist(sqlSearch, &i_availablePackages);
		}

		
	}
	// Predefined groups preselect
	for (size_t i=0; installset_contains.empty() && i<i_availablePackages.size(); ++i)
	{
		p = i_availablePackages.get_package_ptr(i);
		p->set_action(ST_NONE, "reset");
		mark=false;

		// Выбираем наборы пакетов
		// 1. Для всех-всех-всех вариантов нужна база и mopscripts
		if (p->isTaggedBy("base")) mark=true;
		if (p->isTaggedBy("mopscripts")) mark=true;
		// 2. Для различных групп выбираем пакеты по тегам
		
		if (ret == _("MINIMAL")) {
		       	// Минимальная установка: только база, которая уже выбрана: дополнительно ничего не требуется
			// Add i18n glibc, since everybody needs this
			if (p->get_name()=="glibc-i18n") mark=true;
		}
		if (ret == _("FULL")) { // Полная установка: выбираем все пакеты
			mark = true;
		}
		if (ret == _("CUSTOM")) {
		// Экспертная установка: по умолчанию выбираем все пакеты
			mark = true;
		}
		
		if (p->get_repository_tags()=="additional-drivers") {
			mark = false;
		}
		if (p->get_corename()=="nvidia-kernel" || p->get_corename()=="nvidia-driver") mark=false;
		
		if (mark) p->set_action(ST_INSTALL, "marked");
	}

	/*for (size_t i=0; i<altKernelPkgList.size(); ++i) {
		num = i_availablePackages.getPackageNumberByName(altKernelPkgList[i]);
		numAlt = i_availablePackages.getPackageNumberByName(altKernelPkgList[i]+"-bfs");
		if (num==-1 || numAlt==-1) continue;
		if (i_availablePackages[num].action()==ST_INSTALL || i_availablePackages[numAlt].action()==ST_INSTALL) {
			if (alternateKernel.empty()) {
				i_availablePackages.get_package_ptr(numAlt)->set_action(ST_NONE);
				i_availablePackages.get_package_ptr(num)->set_action(ST_INSTALL);
			}
			else {
				i_availablePackages.get_package_ptr(numAlt)->set_action(ST_INSTALL);
				i_availablePackages.get_package_ptr(num)->set_action(ST_NONE);
			}
		}
	}*/
	bool showThisItem;
	if (ret==_("CUSTOM")) goto group_adjust_menu;
	else goto pkgcounter_finalize;
group_mark_menu:
	menuItems.clear();
	for (size_t i=0; i<i_tagList.size(); i++)
	{
		if (ret == _("CUSTOM") || i_tagList[i]!="base")
		{
			menuItems.push_back(MenuItem(i_tagList[i],getTagDescription(i_tagList[i])));
		}
	}
	if (ncInterface.showExMenu(_("Edit list of groups which will be installed. Note that all changes done in previous menu will be lost."), menuItems)==-1) goto group_adjust_menu;
	for (size_t i=0; i<menuItems.size(); i++)
	{
		for (size_t t=0; t<i_availablePackages.size(); t++)
		{
			if (i_availablePackages.at(t).isTaggedBy(menuItems[i].tag))
			{
				if (menuItems[i].flag) i_availablePackages.get_package_ptr(t)->set_action(ST_INSTALL, "groupmark");
				else i_availablePackages.get_package_ptr(t)->set_action(ST_NONE, "reset");
			}
			if (i_availablePackages.at(t).get_tags().size()==0) i_availablePackages.get_package_ptr(t)->set_action(ST_NONE, "reset");
		}
	}
	goto group_adjust_menu;
	// Executing adjustment
group_adjust_menu:
	menuItems.clear();
	menuItems.push_back(MenuItem("OK", _("All OK, continue")));
	menuItems.push_back(MenuItem(_("Groups"), _("Edit by groups")));
	for (size_t i=0; i<i_tagList.size(); i++)
	{
		if (i_tagList[i]!="base")
		{
			menuItems.push_back(MenuItem(i_tagList[i], getTagDescription(i_tagList[i])));
		}
	}
	menuItems.push_back(MenuItem("extras", _("Packages which doesn't belong to any group")));
	menuItems.push_back(MenuItem("everything", _("All packages in one list")));
	
	ret = ncInterface.showMenu2(_("Select group and edit package list if needed. When ready, select [OK]"), menuItems);
	if (ret == "OK") goto pkgcounter_finalize;
	if (ret == _("Groups")) goto group_mark_menu;
	if (ret.empty()) goto pkg_select_init;
	else
	{
		menuItems.clear();
		for (size_t i=0; i<i_availablePackages.size(); i++)
		{
			showThisItem=false;
			p = i_availablePackages.get_package_ptr(i);
			if (ret == "everything") showThisItem=true;
			if (p->isTaggedBy(ret)) showThisItem=true;
			if (ret == "extras" && p->get_tags().empty()) showThisItem=true;
			
			if (showThisItem)
			{
				menuItems.push_back(MenuItem(IntToStr(p->get_id()), p->get_name() + " (" + p->get_short_description() + ")", p->get_description(), p->action()==ST_INSTALL));
			}
		}
		if (ncInterface.showExMenu(_("Mark packages to install (group ") + ret + _(")\nTo select, use SPACE key"), menuItems)>=0)
		{
			for (size_t i=0; i<menuItems.size(); i++)
			{
				for (size_t t=0; t<i_availablePackages.size(); t++)
				{
					if (atoi(menuItems[i].tag.c_str()) == i_availablePackages[t].get_id())
					{
						if (menuItems[i].flag) i_availablePackages.get_package_ptr(t)->set_action(ST_INSTALL, "manual select");
						else i_availablePackages.get_package_ptr(t)->set_action(ST_NONE, "reset");
					}
				}
			}


		}
		goto group_adjust_menu;
	}
pkgcounter_finalize:
	// Render deps and calculate summary
	systemConfig.totalQueuedPackages=0;
	systemConfig.totalDependantPackages=0;
	
	PACKAGE_LIST i_dependantPackages = i_availablePackages;
	if (onlineDeps) 
	{
		deleteCore();
		createCore();
		core->DepTracker->renderDependenciesInPackageList(&i_dependantPackages);
	}
	systemConfig.totalQueuedPackages=0;
	systemConfig.totalDownloadSize=0;
	systemConfig.totalInstallSize=0;
	for (size_t i=0; i<i_availablePackages.size(); i++) {
		if (i_availablePackages[i].action()==ST_INSTALL) {
			systemConfig.totalQueuedPackages++;
			systemConfig.totalDownloadSize += atol(i_availablePackages[i].get_compressed_size().c_str());
			systemConfig.totalInstallSize += atol(i_availablePackages[i].get_installed_size().c_str());
		}
	}
	for (size_t i=0; i<i_dependantPackages.size(); i++)
	{
		if (i_dependantPackages[i].action()==ST_INSTALL) systemConfig.totalDependantPackages++;
	}
	return 0;

}
int dbConfig()
{
	mDebug("Writing the data");
	// Goals: create an appropriate mpkg configuration for installed system
	system("cp /etc/mpkg.xml /etc/mpkg.xml.backup");
	createCore();
	core->set_sysroot("/");
	
	// Adding "addons" repositories from DVD or CD in default installation
	vector<string> repositoryList = core->get_repositorylist();
	string tmp_location;
	size_t rep_id=0;
	for (size_t i=0; !rep_locations.empty() && i<repositoryList.size(); i++) {
		if (rep_id<rep_locations.size()) {
			if (repositoryList[i].find("cdrom://")==0 && repositoryList[i].find(rep_locations[rep_id])!=std::string::npos) {
				core->add_repository(repositoryList[i].substr(0, repositoryList[i].find(rep_locations[rep_id++])) + "/addons/");
			}
		}
	} // Done
	
	deleteCore();
	system("cp /etc/mpkg.xml /mnt/etc/");
	system("cp /etc/mpkg.xml.backup /etc/mpkg.xml");
	return 0;
}
int commit(bool simple)
{
	string summary;
	string errors;
	bool has_queue=false;
	if (systemConfig.rootPartition.empty()) errors += _("You didn't specified a root partition\n");
	else if (systemConfig.rootPartition==systemConfig.swapPartition) errors += _("Root and swap partition cannot be the same. Please fix this.\n");
	if (systemConfig.sourceName.empty()) errors += _("You didn't specified a package source\n");
	else
	{
		if (i_availablePackages.IsEmpty()) errors += _("No available packages found\n");
		for (size_t i=0; i<i_availablePackages.size(); i++)
		{
			if (i_availablePackages[i].action()==ST_INSTALL) {
				has_queue=true;
				break;
			}
		}
		if (!has_queue) errors += _("No package set selected\n");
	}

	if (!errors.empty())
	{
		ncInterface.setSubtitle(_("Some errors has been detected in configuration"));
		ncInterface.showMsgBox(errors);
		return -1;
	}

	if (_cmdOptions["no_bootloader"]!="yes" && bootConfig.loaderType.empty() && !ncInterface.showYesNo(_("WARNING! You didn't configured a boot loader.\nYou can install system without this, but remember that in this case you should provide system booting manually (for example, using another, already installed boot loader).\nCONTINUE ONLY IF YOU *REALLY KNOW* WHAT YOU ARE DOING!"), _("I CONFIRM"), _("Cancel"))) return -1;
	ncInterface.setSubtitle(_("Confirm installation parameters"));
	summary = _("Check carefully all settings before continuing!\n\n") + (string) \
		   _("Root partition: ") + systemConfig.rootPartition + ", " + systemConfig.rootPartitionType + "\n" + \
		   _("Swap partition: ");
       if (systemConfig.swapPartition.empty()) summary += _("no\n");
       else summary += systemConfig.swapPartition + "\n";
       summary += _("Other partitions: ");
	if (systemConfig.otherMounts.empty()) summary += _("no\n");
	else
	{
		for (size_t i=0; i<systemConfig.otherMounts.size(); i++)
		{
			if (systemConfig.otherMounts[i].value[0]=='/')
				summary += "                " + \
				    systemConfig.otherMounts[i].tag + \
				    " (" + systemConfig.otherMounts[i].value + "), " + systemConfig.otherMountFSTypes[i] + _(", format: ") + doFormatString(systemConfig.otherMountFormat[i])+"\n";
		}
	}
	// Check deps before any action
	
	ncInterface.showInfoBox(_("Creating installation list"));
	PACKAGE_LIST commitList;
	
	/*// debug
	vector<string> depLog;
	for (size_t i=0; i<i_availablePackages.size(); ++i) {
		if (i_availablePackages[i].action()==ST_INSTALL) depLog.push_back(i_availablePackages[i].get_name());
	}
	WriteFileStrings("/tmp/i_availablePackages_before_commit.log", depLog);
	// enddebug*/

	for (size_t i=0; i<i_availablePackages.size(); i++) {
		if (i_availablePackages[i].action()==ST_INSTALL) {
			commitList.add(i_availablePackages[i]);
			//commitList.get_package_ptr(commitList.size()-1)->set_action(ST_INSTALL);
		}
	}
	
	/*
	// debug
	depLog.clear();
	for (size_t i=0; i<commitList.size(); ++i) {
		if (commitList[i].action()==ST_INSTALL) depLog.push_back(commitList[i].get_name());
	}
	WriteFileStrings("/tmp/commitList1.log", depLog);
	// enddebug
	*/

	/*createCore();
	DependencyTracker *tracker = new DependencyTracker(core->db);

	ncInterface.showInfoBox(_("Checking dependencies"));
	int rcount = tracker->renderDependenciesInPackageList(&commitList);
	ncInterface.showInfoBox(_("Checking results"));
	if (rcount) {
		ncInterface.setSubtitle(_("Depencency errors"));
		ncInterface.showInfoBox(_("Rendering error list: ") + IntToStr(rcount) + " errors");
		ncInterface.showMsgBox(_("Unresolved dependencies: ") + IntToStr(rcount) + "\n" + depErrorTable.print());
		delete tracker;
		return 1;
	}
	delete tracker;
	deleteCore();*/

	
	//setupListCheck(commitList);

	summary += _("Packages source: ") + systemConfig.sourceName + "\n" + \
		    _("Setup mode: ") + systemConfig.setupMode + "\n" + \
		    _("Packages to install: ") + IntToStr(systemConfig.totalDependantPackages) + \
		    	_(" (explicitly choosed: ") + IntToStr(systemConfig.totalQueuedPackages) + _(", added by dependencies: ") + IntToStr(systemConfig.totalDependantPackages - systemConfig.totalQueuedPackages) + ")\n" + \
			_("\nCompressed packages size: ") + humanizeSize(systemConfig.totalDownloadSize) + \
			_("\nDisk space required by packages: ") + humanizeSize(systemConfig.totalInstallSize*1.2f) + \
			_("\nBoot loader will be installed in: ") + bootConfig.bootDevice + \
		    _("\nProceed with installation?");
	if (ncInterface.showYesNo(summary))
	{
		time_t installStartTime = time(NULL);  // Let's measure time :)
		time_t pkgInstallTime = 0;
		ncInterface.setSubtitle(_("Preparing with system installation"));
		if (!simulate)
		{
			if (activateSwapSpace()!=0) return -1;
			if (formatPartitions()!=0) return -1;
			if (mountPartitions()!=0) return -1;
			createCore();
			ncInterface.showInfoBox(_("Creating installation queue"));
			core->clean_queue();
			/*depLog.clear();
			for (size_t i=0; i<commitList.size(); ++i) {
				if (commitList[i].action()==ST_INSTALL) depLog.push_back(commitList[i].get_name());
			}
			WriteFileStrings("/tmp/commitList2.log", depLog);*/
			// New: link cache from CD-ROM
			if (FileExists("/var/log/mount/cache")) system("ln -s /var/log/mount/cache /var/mpkg/cache/.fcache");
	

			core->install(&commitList);
			core->commit(true);
			PACKAGE_LIST tmpList;
			SQLRecord tmpSql;
			core->get_packagelist(tmpSql, &tmpList);
			vector<string> depLog;
			for (size_t i=0; i<tmpList.size(); ++i) {
				if (tmpList[i].action()==ST_INSTALL) depLog.push_back(tmpList[i].get_name());
			}
			WriteFileStrings("/tmp/installation_list.log", depLog);

			/*if (ncInterface.showYesNo(_("Show queue?"))) {
				deleteCore();
				ncInterface.uninit();
				system("mpkg -g show_queue");
				createCore();
			}*/
			ncInterface.showInfoBox(_("Starting package installation"));
			time_t pkgInstallStartTime=time(NULL);
			if (core->commit()!=0) return -1;
			/*PACKAGE_LIST kernelCheck;
			SQLRecord search;
			search.addField("package_name", string("kernel"));
			search.addField("package_provides", string("kernel"));
			search.setSearchMode(SEARCH_OR);
			core->db->get_packagelist(search, &kernelCheck);
			if (kernelCheck.size()>0) {
				for (size_t i=0; i<kernelCheck.size(); ++i) {
					if (kernelCheck[i].installed()) systemConfig.kernelversion = kernelCheck[0].get_version();
				}
			}*/
			// Do installed kernel version check
			vector<string> dirList = getDirectoryList("/mnt/lib/modules");
			for (size_t i=0; i<dirList.size(); ++i) {
				if (dirList[i]!="." && dirList[i]!="..") {
					systemConfig.kernelversion = dirList[i];
					break;
				}
			}
			deleteCore();
			if (_cmdOptions["ramwarp"]=="yes") {
				system("rm /mnt/var/mpkg/packages.db && mv /mnt/.installer/packages.db /mnt/var/mpkg/packages.db && umount /mnt/.installer && rmdir /mnt/.installer");
			}
			pkgInstallTime = time(NULL) - pkgInstallStartTime;
			if (performConfig(simple)!=0) return -1;
			if (dbConfig()!=0) return -1;
			syncFS();
		}
		time_t installTime = time(NULL) - installStartTime; // Installation end time
		if (showFinish(installTime, pkgInstallTime)) return 2;
	}
	else return -1;
	return 0;
}

bool requestNextDisc(int disc_number, string last_indexed_cd, bool prompt_index, bool index_complete) // OMGWTF хитрющая функция для опроса юзера для вставки следующего диска.
{
	if (index_complete) return false;
	if (prompt_index) return ncInterface.showYesNo(_("Insert another installation disc to index.\nIf all discs are already indexed, press [Next]\n\nDiscs indexed: ") +\
				IntToStr(disc_number) + _("\nLast indexed: ") + last_indexed_cd, _("Index"), _("Continue"));
	return true;
}

int setSambaSource(string, string username, string password) {
	if (ncInterface.showYesNo(_("Do you wish to configure network?"))) netConfig();
	string ip = ncInterface.showInputBox(_("Enter server's hostname or IP address:"));
	if (username.empty()) {
		username = ncInterface.showInputBox(_("Login (leave blank if none):"));
		if (!username.empty() && password.empty()) password = ncInterface.showInputBox(_("Password:"));
	}
	system("mkdir -p /tmp/mpkgmount/cifs");
	// List the resources
	string auth;
	//string lookup = "smbclient -g -L " + ip;
	if (!username.empty()) auth = "username = " + username + "\n";
	if (!password.empty()) auth += "password = " + password + "\n";
	if (!auth.empty()) {
		WriteFile("/tmp/cifs_auth", auth);
	//	lookup += " -A auth";
	}
	/*
	system(lookup + " | grep Disk > /tmp/cifs_lookup");
	vector<string> data = ReadFileStrings("/tmp/cifs_lookup");
	vector<MenuItem> items;
	for (size_t i=0; i<data.size(); ++i) {
		data[i] = data[i].substr(6);
		items.push_back(MenuItem(data[i].substr(0, data[i].find_first_of("|")), ""));
	}*/
	string selectShare = ncInterface.showInputBox(_("Select share to mount:"));
	if (selectShare.empty()) return -1;
	//system("mkdir -p /mnt/mpkgmount/cifs")
	string mnt = "mount -t cifs //" + ip + "/" + selectShare + " /tmp/mpkgmount/cifs ";
	if (!auth.empty()) mnt += " -o credentials=/tmp/cifs_auth";
	mnt += " >/tmp/mounterr";
//	ncInterface.showMsgBox(mnt);
	ncInterface.uninit();
	int ret = system(mnt);
	if (ret) {
		ncInterface.showMsgBox(_("Failed to connect to Samba server:\n") + ReadFile("/tmp/mounterr"));
		return -1;
	}
	string iso = ncInterface.ncGetOpenFile("/mnt/mpkgmount/cifs/");
	return setCDSource(iso);

}

int setCDSource(string predefined)
{

	if (predefined =="dvd") ncInterface.setSubtitle(_("Installation from DVD"));
	else if (predefined.find("iso")!=std::string::npos) ncInterface.setSubtitle(_("Installation from ISO image"));
	else ncInterface.setSubtitle("Installation from CD/DVD");
	vector<MenuItem> kdeOptions;
	kdeOptions.push_back(MenuItem("kde3", "KDE 3.5.10", _("Very old and very stable. If you need \"Just works\" - it's your choice")));
	kdeOptions.push_back(MenuItem("kde4", "KDE 4.2.x", _("Current version of KDE. Good choice if you have a modern computer with good videocard and want to go forward with new technologies")));
	string kde_version;
       	if (askForKDE) kde_version = ncInterface.showMenu2(_("Please, choose KDE branch which you prefer. Both of them are stable enough today in most cases. If you wish to use another desktop environment (XFCE, Fluxbox, etc) - your choice should be based on programs which you will use from these environments\n"), kdeOptions);
	else kde_version = "kde4";
	if (kde_version.empty()) kde_version = "kde3";
	kde_branch = kde_version;

#ifdef ENABLE_ADDONS_QUESTION
	enable_addons=ncInterface.showYesNo(_("Enable addons?"));
#endif
	if (predefined.find("/")!=std::string::npos) {
		if (FileExists(predefined)) systemConfig.cdromDevice = predefined;
		predefined = "iso";
	}
	else {
		if (predefined == "iso") {
			ncInterface.setSubtitle(_("Select ISO image"));
			string base;
			if (ncInterface.showYesNo(_("Do you want to mount partition which contains an ISO image?"))) base = ncGetMountPartition(_("Mounting partition with ISO image"));
			else base="/";
			string iso_file = ncInterface.ncGetOpenFile(base);
			if (iso_file.empty()) return 1;
			ncInterface.setSubtitle(_("Installation from ISO image"));
			mountMedia(iso_file);
		}
		else mountMedia();
	}
	system("umount " + systemConfig.cdromDevice + " 2>/dev/tty4 >/dev/tty4");
	usedCdromMount = false;

	string last_indexed_cd=_("<none>");
	int disc_number=0;
	vector<string> nullList,rList;
	if (predefined!="dvd" && predefined != "iso") ncInterface.showMsgBox(_("Please, prepare all installation discs.\nIt is required to index them all."));
	createCore();
	string volname;
	rep_locations.clear();
	string rep_location;
	mkdir(CDROM_MOUNTPOINT.c_str(), 755);
	bool prompt_index=true, index_complete=false;
	if (predefined=="dvd" || predefined == "iso") prompt_index=false;
	while(requestNextDisc(disc_number, last_indexed_cd, prompt_index, index_complete))
	{
		prompt_index=true;
		mDebug("Mounting and retrieving index)");
		ncInterface.showInfoBox(_("Mounting CD/DVD..."));
		if (predefined != "iso") system("mount " + systemConfig.cdromDevice + " /var/log/mount 2>/dev/tty4 >/dev/tty4");
		else {
			system("mount -o loop " + systemConfig.cdromDevice + " /var/log/mount 2>/dev/tty4 >/dev/tty4");
		}

		volname = getCdromVolname(&rep_location);
		if (rep_location.find("$KDEBRANCH")!=std::string::npos) strReplace(&rep_location, "$KDEBRANCH", kde_version);
		if (!volname.empty() && !rep_location.empty())
		{
			ncInterface.showInfoBox(_("Loading data from disc ") + volname + _("\nData path: ") + rep_location);
			if (volname!=last_indexed_cd)
			{
				if (cacheCdromIndex(volname, rep_location))
				{
					if (enable_addons && FileExists("/var/log/mount/.addons")) {
						string addons_rep = cutSpaces(ReadFile("/var/log/mount/.addons"));
						strReplace(&addons_rep, "$KDEBRANCH", kde_version);
						cacheCdromIndex(volname, addons_rep);
						mDebug("Caching addons ok");
						rList.push_back("cdrom://" + volname + "/" + addons_rep);
						rep_locations.push_back(addons_rep);
					}
					if (enable_contrib && FileExists("/var/log/mount/.contrib")) {
						string contrib_rep = cutSpaces(ReadFile("/var/log/mount/.contrib"));
						strReplace(&contrib_rep, "$KDEBRANCH", kde_version);
						cacheCdromIndex(volname, contrib_rep);
						mDebug("Caching contrib ok");
						rList.push_back("cdrom://" + volname + "/" + contrib_rep);
						rep_locations.push_back(contrib_rep);
					}


					mDebug("Caching OK");
					rList.push_back("cdrom://"+volname+"/"+rep_location);
					rep_locations.push_back(rep_location);

					last_indexed_cd=volname;
					disc_number++;
					ncInterface.showInfoBox(_("Data loaded successfully.\nDisc label: ") + volname + _("\nPackage path: ") + rep_location);
					if (predefined=="dvd" || predefined == "iso") {
						index_complete=true;
					}

				}
				else ncInterface.showMsgBox(_("Failed to read repository index from this disc"));
			}
			else ncInterface.showMsgBox(_("This disc already indexed"));
		}
		else ncInterface.showMsgBox(_("This disc doesn't recognized as AgiliaLinux installation disc"));
		system("umount " + systemConfig.cdromDevice + " 2>/dev/tty4 >/dev/tty4");
		if (!noEject && predefined !="dvd") {
			system("eject " + systemConfig.cdromDevice + " 2>/dev/tty4 >/dev/tty4");
			system("echo Ejecting > /dev/tty4");
		}
		usedCdromMount=false;
	}
	// Commit
	if (rList.empty())
	{
		systemConfig.sourceName.clear();
		deleteCore();
		return -1;
	}

	core->set_repositorylist(rList, nullList);
	core->update_repository_data();
	getCustomSetupVariants(rList);
	// Clean index cache
	core->cleanCdromCache("/");
	deleteCore();
	
	if (predefined!="iso") {
		if (rList.size()==1) systemConfig.sourceName=_("Single CD/DVD");
		if (rList.size()>1) systemConfig.sourceName = _("Set of ") + IntToStr(rList.size()) + " CD/DVD";
	}
	else systemConfig.sourceName = _("ISO image");
	return 0;
}


int setHDDSource(string predefined)
{
	vector<string> rList, nullList;
	ncInterface.setSubtitle(_("Looking for partitions on hard drives"));
	string ret="/", index_name;

	string base;
	if (predefined.find("file://")==0) {
		ret = predefined;
		goto hdd_unattended;
	}
	/*if (ncInterface.showYesNo("Сначала вы должны смонтировать нужный раздел в любую папку. Открыть консоль?")) 
	{
		ncInterface.uninit();
		say("Смонтируйте нужный диск в любую папку, перейдите в директорию с пакетами, и наберите this\n");
		system("/bin/bash");
	}*/
enter_path:
	/*
	ret = ReadFile("/var/log/hdddir");
	if (ret.find_first_of("\n\r")!=std::string::npos) ret = ret.substr(0,ret.find_first_of("\n\r"));
	if (ret.empty()) ret = "/";
	//ret = ncInterface.showInputBox("Укажите АБСОЛЮТНЫЙ путь к каталогу с пакетами:", ret);*/
	if (ncInterface.showYesNo(_("Do you want to mount partition with packages?"))) base = ncGetMountPartition(_("Select partition with packages:"));
	ret = ncInterface.ncGetOpenDir(base);
	if (ret.empty()) { 
		packageSourceSelectionMenu(); return 0;
	}
hdd_unattended:
	index_name = ret + "/packages.xml.gz";

	if (access(index_name.c_str(), R_OK)==0)
	{
		rList.clear();
		rList.push_back("file://" + ret + "/");
		ncInterface.showInfoBox(_("Updating package list..."));
		createCore();
		core->set_repositorylist(rList, nullList);
		if (core->update_repository_data()!=0)
		{
			mDebug("update failed");
			deleteCore();
			ncInterface.showMsgBox(_("An error occured while loading package list. Check if package source specified correctly."));
			goto enter_path;
		}
		else
		{
			getCustomSetupVariants(rList);
			mDebug("ok");

			deleteCore();
		}
		systemConfig.sourceName="file://" + ret + "/";
	}
	else 
	{
		ncInterface.showMsgBox(_("You have specified directory (") + ret + _("), but no repository found there.\nCheck path and try again."));
		goto enter_path;
	}
	return 0;
}
int netConfig()
{
	// TODO: fault tolerance
	ncInterface.setSubtitle(_("Network setup"));
	string eth_name, eth_ip, eth_netmask, eth_gateway, eth_dns;
	eth_name = ncInterface.showInputBox(_("Enter network card name:"),"eth0");
	if (eth_name.empty()) return -1;
	bool use_dhcp = ncInterface.showYesNo(_("Use automatic configuration (DHCP)?"));
	if (use_dhcp) {
		ncInterface.uninit();
		system("dhcpcd -n " + eth_name);
		return 0;
	}
	eth_ip = ncInterface.showInputBox(_("IP address:"),"");
	if (eth_ip.empty()) return -1;
	eth_netmask = ncInterface.showInputBox(_("Network mask:"),"255.255.255.0");
	if (eth_netmask.empty()) return -1;
	eth_gateway = ncInterface.showInputBox(_("Gateway (leave blank if none):"),"");
	eth_dns = ncInterface.showInputBox(_("Primary DNS (leave blank if none):"), eth_gateway);


	system("ifconfig " + eth_name + " " + eth_ip + " netmask " + eth_netmask + " up");
	if (!eth_gateway.empty()) { system("route del default"); system("route add default gw " + eth_gateway); }
	if (!eth_dns.empty()) WriteFile("/etc/resolv.conf", "nameserver " + eth_dns);

	return 0;
}
int setNetworkSource(string predefined)
{
	if (ncInterface.showYesNo(_("Configure network?"))) netConfig();
	ncInterface.setSubtitle(_("Looking for packages in network"));
	vector<string> nullList, rList;
	string ret;
start:
	createCore();
	if (systemConfig.sourceName.find(_("From network"))==0 && !REPOSITORY_LIST.empty()) ret = REPOSITORY_LIST[0];

	rList.clear();
#ifdef X86_64
	string default_url="http://core64.mopspackages.ru/";
#else
	string default_url="http://core32.mopspackages.ru/";
#endif
	if (predefined.empty()) ret = ncInterface.showInputBox(_("Enter repository URL:"), default_url);
	else ret = predefined;
	if (!ret.empty()) rList.push_back(ret);
	
	//d.execAddableList("Введите URL дополнительных сетевых репозиториев. Можете указать несколько.", &rList, "://");
	// Redefine repository list
	if (rList.empty())
	{
		return -1;
	}
	if (rList.size()==1) systemConfig.sourceName=_("From network: ") + rList[0];
	else systemConfig.sourceName=_("From network (multiple sources)");
	ncInterface.showInfoBox(_("Updating package list"));
	core->set_repositorylist(rList, nullList);
	PACKAGE_LIST pkgList;// = new PACKAGE_LIST;
	SQLRecord sqlSearch;// = new SQLRecord;
	core->get_packagelist(sqlSearch, &pkgList);
	if (core->update_repository_data()!=0 && pkgList.size()==0)
	{
		mDebug("update failed");
		deleteCore();
		if (ncInterface.showYesNo(_("Failed to load package list from network. Maybe, you didn't configured network properly. Try to (re)configure it?")))
		{
			if (netConfig()!=0) return -1;
		}
		goto start;
	}
	else
	{
		getCustomSetupVariants(rList);
		mDebug("ok");
		deleteCore();
	}

	return 0;
}

int packageSourceSelectionMenu(string predefinedSource)
{

	ncInterface.setSubtitle(_("Package source selection"));
	vector<MenuItem> menuItems;
	string ret;
pkgSrcSelect:
	menuItems.clear();
	menuItems.push_back(MenuItem("DVD-ROM", _("Install from DVD")));
	menuItems.push_back(MenuItem("CD-ROM", _("Install from a set of CD/DVD")));
	menuItems.push_back(MenuItem("HDD", _("Install from hard drive")));
	menuItems.push_back(MenuItem("Network", _("Install from network repository (FTP, HTTP)")));
	menuItems.push_back(MenuItem("ISO", _("Install from ISO image on HDD")));
	menuItems.push_back(MenuItem("Samba", _("Install from Samba (Windows network)")));
	menuItems.push_back(MenuItem(_("Back"), _("Back to main menu")));
	if (predefinedSource.empty()) ret = ncInterface.showMenu2(_("Select a package source to install system from it:"), menuItems);
	else {
		if (predefinedSource=="cdrom") ret = "CD-ROM";
		if (predefinedSource=="dvd") ret = "DVD-ROM";
		if (predefinedSource=="iso") ret = "ISO";
		if (predefinedSource.find("file://")==0) ret="HDD";
		if (predefinedSource.find("smb://")==0) ret = "Samba";
		if (ret.empty() && predefinedSource.find("://")!=std::string::npos) ret="Network";
		if (ret.empty()) {
			ncInterface.showMsgBox(_("Incorrect source selected: ") + predefinedSource);
			return -1;
		}
	}
	if (ret == "HDD") setHDDSource(predefinedSource);
	if (ret == "Network") setNetworkSource(predefinedSource);
	if (ret == "CD-ROM") setCDSource(predefinedSource);
	if (ret == "DVD-ROM") setCDSource("dvd");
	if (ret == "ISO") setCDSource("iso");
	if (ret == "Samba") if (setSambaSource(predefinedSource)) goto pkgSrcSelect;
	return 0;
}
	
int diskPartitioningMenu(bool simple)
{
	ncInterface.setSubtitle(_("Disk partitioning"));
	vector<MenuItem> menuItems;
	vector<TagPair> devList;
	vector<MenuItem> mDevList;
part_menu_init:
	devList = getDevList();
	mDevList.clear();
	for (size_t i=0; i<devList.size(); i++) {
		mDevList.push_back(MenuItem(devList[i].tag, devList[i].value));
	}
	mDevList.push_back(MenuItem("RAID", _("Create RAID array")));
	mDevList.push_back(MenuItem("OK", _("Continue")));
	string ret;
	string disk_name;
	int r = 0;
	if (devList.empty()) {
		ncInterface.showMsgBox(_("No hard drive detected in your computer"));
		return -1;
	}
part_menu:
	menuItems.clear();
	if (!simple) {
		menuItems.push_back(MenuItem("cfdisk", _("Simple menu")));
		menuItems.push_back(MenuItem("mparted", _("User-friendly frontend to parted. Experimental")));
		menuItems.push_back(MenuItem("fdisk", _("Command line based partitioning (expert mode)")));
		menuItems.push_back(MenuItem("parted", _("Advanced command line based partitioning (expert mode)")));
		menuItems.push_back(MenuItem("OK", _("Back to main menu")));
		ret = ncInterface.showMenu2(_("Choose partitioning method:"), menuItems);
		if (ret == "OK" || ret.empty()) return 0;
	}
	else ret = "cfdisk";
	disk_name = ncInterface.showMenu2(_("To install AgiliaLinux, you should have at least one Linux partition. Also, it is strongly recommended to have a swap partition.\nIf you didn't do this before, you can create it now.\nWhich drive do you want to edit?"), mDevList);
	if (disk_name.find("/dev/")!=0) {
		//if (!simple || devList.size()>1) goto part_menu;
		//else {
		if (disk_name == "RAID") {
			runRaidTool();
			deviceCacheActual = false;
			partitionCacheActual = false;
			goto part_menu_init;
		}
		if (disk_name == "OK") return 0;
			else return -1;
		//}
	}
	// Disabling ncurses: will interfere with console
	ncInterface.uninit();
	if (ret == "mparted") r = system("mparted");
	if (ret == "cfdisk") r = system("cfdisk " + disk_name);
	if (ret == "fdisk") r = system("fdisk " + disk_name);
	if (ret == "parted") r = system("parted " + disk_name);
	// Reset cache
	deviceCacheActual = false;
	partitionCacheActual = false;
	//if (r==0) return 0;
	/*else*/ goto part_menu;
}
/*
bool restoreGrub() {

}
bool restoreLilo(string device) {
	system("mkdir /tmp/a 2>/dev/tty4 >/dev/tty4");
       	system("mount " + device + " /tmp/a 2>/dev/tty4 >/dev/tty4");
	system("chroot /tmp/a mount -a 2>/dev/tty4 >/dev/tty4");
	system("chroot /tmp/a lilo 2>/dev/tty4 >/dev/tty4");
	system("chroot /tmp/a umount -a 2>/dev/tty4 >/dev/tty4");
	system("umount /tmp/a 2>/dev/tty4 >/dev/tty4");
	system("rmdir /tmp/a 2>/dev/tty4 >/dev/tty4");
}*/
bool restoreDefaultBootloader() {
	ncInterface.setSubtitle("Restoring boot loader");
	vector<MenuItem> menuItems;
	vector<TagPair> devList = getDevList();
	if (devList.empty()) {
		ncInterface.showMsgBox(_("No hard drive detected in your computer"));
		return false;
	}
	for (size_t i=0; i<devList.size(); i++) {
		menuItems.push_back(MenuItem(devList[i].tag, devList[i].value));
	}
	string disk_name;
	disk_name = ncInterface.showMenu2(_("Choose hard drive on which you want to restore a standard MBR boot loader:"), menuItems);
	if (!disk_name.empty() && ncInterface.showYesNo(_("Are you really want to restore standard boot loader on drive ") + disk_name + "?")) {
		if (msdosconfig(disk_name)) {
			ncInterface.showMsgBox(_("MBR boot loader successfully restored on ") + disk_name);
			return true;
		}
		else {
			ncInterface.showMsgBox(_("Failed to restore standard MBR boot loader on drive ") + disk_name);
			return false;
		}
	}
	return false;
	
}

string doFormatString(bool input)
{
	if (input) return _("YES");
	else return _("no");
}
void loadAltDescriptions(vector<MenuItem> *altMenu) {
	for (size_t i=0; i<altMenu->size(); ++i) {
		if (altMenu->at(i).tag=="bfs") altMenu->at(i).value=_("BFS kernel");
		else if (altMenu->at(i).tag=="cleartype") altMenu->at(i).value=_("Cleartype-patched fonts");
		else if (altMenu->at(i).tag=="sun") altMenu->at(i).value=_("Original OpenOffice build from Sun");
	}
}

int driverSelectionMenu(const vector<string>& alternatives, vector<string> * appliedAlternatives) {
	// First, detect hardware for which we have to install drivers.
	string tmp_hw = get_tmp_file();
	system("lspci > " + tmp_hw);
	vector<string> hw = ReadFileStrings(tmp_hw);
	// Check for VirtualBox
	bool hasVirtualBox=false;
	int hasNvidia=-1;
	int hasRadeon=-1;
	for(size_t i=0; i<hw.size(); ++i) {
		if (hw[i].find("VirtualBox")!=std::string::npos) hasVirtualBox = true;
		if (hw[i].find("VGA compatible controller")!=std::string::npos && hw[i].find("nVidia")!=std::string::npos) hasNvidia = i;
		if (hw[i].find("VGA compatible controller")!=std::string::npos && hw[i].find("Radeon")!=std::string::npos) hasRadeon = i;
	}
	if (hasNvidia!=-1) {
		vector<MenuItem> menuItems;
		if (i_availablePackages.getPackageNumberByName("nvidia-driver")!=-1 && i_availablePackages.getPackageNumberByName("nvidia-kernel")!=-1) {
			menuItems.push_back(MenuItem(_("Latest"), _("Latest GeForce (series 6xxx and later)")));
			for (size_t i=0; i<alternatives.size(); ++i) {
				if (alternatives[i]=="beta") {
					menuItems.push_back(MenuItem("Beta", _("Latest GeForce (series 6xxx and later), beta driver")));
					continue;
				}
				if (alternatives[i]=="legacy173") {
					menuItems.push_back(MenuItem("173", _("GeForce FX series")));
					continue;
				}
				if (alternatives[i]=="legacy96") {
					menuItems.push_back(MenuItem("96", _("GeForce 4 series")));
					continue;
				}
			}
		}

		menuItems.push_back(MenuItem(_("Skip"), _("Do not install driver")));

		string ret = ncInterface.showMenu2(_("Videocard detected: ") + hw[hasNvidia] + _("\nChoose driver for it:"), menuItems);
		if (!ret.empty() && ret != _("Skip")) {
			int drvnum = i_availablePackages.getPackageNumberByName("nvidia-driver");
			int kernelnum = i_availablePackages.getPackageNumberByName("nvidia-kernel");
			if (drvnum!=-1) i_availablePackages.get_package_ptr(drvnum)->set_action(ST_INSTALL, "nvidia-select");
			if (kernelnum!=-1) i_availablePackages.get_package_ptr(kernelnum)->set_action(ST_INSTALL, "nvidia-select");
			if (ret=="173") {
				appliedAlternatives->push_back("legacy173");
			}
			else if (ret=="96") {
				appliedAlternatives->push_back("legacy96");
			}
			else if (ret=="Beta") {
				appliedAlternatives->push_back("beta");
			}
		}
	}

	return 0;
}



int switchAlternativesMenu() {
	vector<string> alternatives = i_availablePackages.getAlternativeList(true);
	if (alternatives.empty()) return 0;
	
	// Let's do some filtering.
	// First, let's filter out NVIDIA drivers alternative - let them to be selected separately.
	// Second, load descriptions for alternatives (for nasty look, will implement later).
	vector<MenuItem> altMenu, altNVidiaMenu;
	for (size_t i=0; i<alternatives.size(); ++i) {
		if (alternatives[i]=="legacy96" || alternatives[i]=="legacy173" || alternatives[i]=="beta" || alternatives[i]=="ru") continue;
		altMenu.push_back(MenuItem(alternatives[i], ""));
	}
	loadAltDescriptions(&altMenu);
	if (ncInterface.showExMenu(_("There are some package alternatives. Please, mark ones which you want to use."), altMenu)==-1) return 1;
	vector<string> appliedAlternatives;
	string __applAlt;
	for (size_t i=0; i<altMenu.size(); ++i) {
		if (altMenu[i].flag) {
			appliedAlternatives.push_back(altMenu[i].tag);
			__applAlt+=altMenu[i].tag + "\n";
		}
	}
	// debug
	vector<string> depLog;
	for (size_t i=0; i<i_availablePackages.size(); ++i) {
		if (i_availablePackages[i].action()==ST_INSTALL) depLog.push_back(i_availablePackages[i].get_name());
	}
	WriteFileStrings("/tmp/i_availablePackages1.log", depLog);

	
	if (driverSelectionMenu(alternatives, &appliedAlternatives)) return 1;
	//ncInterface.showMsgBox("Applied alternative flags:\n" + __applAlt);
	// debug
	depLog.clear();
	for (size_t i=0; i<i_availablePackages.size(); ++i) {
		if (i_availablePackages[i].action()==ST_INSTALL) depLog.push_back(i_availablePackages[i].get_name());
	}
	WriteFileStrings("/tmp/i_availablePackages2.log", depLog);

	i_availablePackages.switchAlternatives(appliedAlternatives);

	// debug
	depLog.clear();
	for (size_t i=0; i<i_availablePackages.size(); ++i) {
		if (i_availablePackages[i].action()==ST_INSTALL) depLog.push_back(i_availablePackages[i].get_name());
	}
	WriteFileStrings("/tmp/i_availablePackages3.log", depLog);
	// enddebug

	//setupListCheck(i_availablePackages);
	// Let's show what we did selected:
	string altList;
	for (size_t i=0; i<i_availablePackages.size(); ++i) {
		if (i_availablePackages[i].action()!=ST_INSTALL) continue;
		if (i_availablePackages[i].getAlternative().empty()) continue;
		altList += i_availablePackages[i].get_name() + "\n";
	}
	/*if (!altList.empty()) {
		ncInterface.showMsgBox(_("Alternative packages will be used:\n") + altList);
	}*/
	return 0;

}
bool runSimpleInstall() {
	int ret=0;
	for (int i=0; i<9; ++i) {
		switch(i) {
			case 0: ret = diskPartitioningMenu(true);
				if (ret) return false;
				break;
			case 1:
				ret = setSwapSpace();
				break;
			case 2:
				ret = setRootPartition("","",true);
				break;
			case 3:
				ret = setOtherPartitions(true);
				break;
			case 4:
				ret = packageSourceSelectionMenu();
				break;
			case 5:
				ret = packageSelectionMenu("", true);
				break;
			case 6:
				ret = switchAlternativesMenu();
				break;
			case 7:
				ret = setupBootloaderOptions(true);
				break;
			case 8:
				if (!forceInInstallMD5Check) {
					forceInInstallMD5Check = ncInterface.showYesNo(_("Check package integrity before installation?"));
				}
				ret = commit(true);
				if (ret==0) return true;
				if (ret==2) {
					deleteCore();
					unlockDatabase();
					ncInterface.uninit();
					// FIXME: Why eject when there are no CD? Should eject only if CD/DVD has used.
					system("eject " + systemConfig.cdromDevice + " 2>/dev/tty4 >/dev/tty4");
					system("chroot /mnt umount /proc");
					system("chroot /mnt umount /sys");
					system("chroot /mnt umount -a");
					//system("umount /mnt");
					system("/usr/lib/setup/reboot_script &");
					return true;
				}
				return false;
				break;
		}
		if (ret) i-=2;
	}
	return true;
}
string selectLanguage() {
	vector<MenuItem> menuItems;
	menuItems.push_back(MenuItem("en_US.UTF-8", "English language"));
	menuItems.push_back(MenuItem("ru_RU.UTF-8", "Русский язык"));
	menuItems.push_back(MenuItem("uk_UA.UTF-8", "Українська мова"));
	CursesInterface lselect(false);
//	lselect._BGF=" ";
#ifndef X86_64
	lselect.setTitle("AgiliaLinux " + string(DISTRO_VERSION) + " installer");
#else
	lselect.setTitle("AgiliaLinux64 " + string(DISTRO_VERSION) + " installer");
#endif
	lselect.setSubtitle("AgiliaLinux installer: language selection");
	string lng = lselect.showMenu2("Please, select language:\nПожалуйста, выберите язык:\nВибір мови:\n", menuItems);
	lselect.uninit();
	return lng;
}
int main(int argc, char *argv[])
{
	setupMode=true;
	ncInterface.initLocale=false;
//	bindtextdomain( "mpkg", "/usr/share/locale");
//	textdomain("mpkg");
	systemConfig.tryPrelink = false; // Let prelink NOT be default option
	systemConfig.tmpfs_tmp = false; // Do not use tmpfs by default
	simulate=false;
	forceSkipLinkMD5Checks=true;
	forceInInstallMD5Check = false;
	bool valid_opt=true;
	int commit_ret=0;
	string optstr;
	// We don't care of database integrity in case of installation failure half-way, so we can use fast mode. It has beed tested enough, and I think it is useful.
	//  Warp drive active!
	_cmdOptions["ramwarp"]="yes";
	_cmdOptions["warpmode"]="yes";

	// Let's parse cmd options
	if (argc>=2)
	{
		for (int i=1; i<argc; i++)
		{
			valid_opt=false;
			/*if (strcmp(argv[i], "--simulate")==0)
			{
				simulate=true;
				say("Simulation mode!\n");
				valid_opt=true;
			}*/
			if (strcmp(argv[i], "--check")==0)
			{
				valid_opt=true;
				//forceSkipLinkMD5Checks=false;
				forceInInstallMD5Check = true;
			}
			if (strcmp(argv[i], "--no-eject")==0)
			{
				valid_opt=true;
				noEject=true;
			}
			if (strcmp(argv[i], "--no-deps")==0) {
				valid_opt = true;
				ignoreDeps = true;
			}
			
			if (strcmp(argv[i], "--no-warpmode")==0) {
				valid_opt = true;
				_cmdOptions["warpmode"]="no";
				_cmdOptions["ramwarp"]="no";
			}
			if (strcmp(argv[i], "--no-bootloader")==0) {
				valid_opt = true;
				_cmdOptions["no_bootloader"]="yes";
			}
			// Unattended install options
			if (strcmp(argv[i], "--root")==0 && i<argc+1) { // Done
				sc.sysroot = argv[i+1];
				valid_opt=true;
				unattended = true;
				i++;
			}
			if (strcmp(argv[i], "--root_format")==0 && i<argc+1) { // Done
				sc.sysroot_format = argv[i+1];
				valid_opt = true;
				unattended = true;
				i++;
			}
			if (strcmp(argv[i], "--pkgsource")==0 && i<argc+1) { // Done
				sc.pkgsource = argv[i+1];
				unattended = true;
				valid_opt=true;
				i++;
			}
			if (strcmp(argv[i], "--bootloader")==0 && i<argc+1) { //TODO
				sc.bootloader = argv[i+1];
				valid_opt=true;
				unattended = true;
				i++;
			}
			if (strcmp(argv[i], "--bootloader_root")==0 && i<argc+1) { //TODO
				sc.bootloader_root = argv[i+1];
				valid_opt=true;
				unattended = true;
				i++;
			}
			if (strcmp(argv[i], "--install_type")==0 && i<argc+1) { //TODO
				sc.install_type = argv[i+1];
				valid_opt=true;
				unattended = true;
				i++;
			}
			if (strcmp(argv[i], "--swap")==0 && i<argc+1) { // Done
				sc.swap_partition = argv[i+1];
				valid_opt=true;
				unattended = true;
				i++;
			}
			if (strcmp(argv[i], "--accept-license")==0) {
				licenseAccepted=true;
				valid_opt = true;
				unattended = true;
			}
			if (strcmp(argv[i], "--enable-addons")==0) {
				enable_addons=true;
				valid_opt = true;
			}
			if (strcmp(argv[i], "--enable-debug")==0) {
				enable_setup_debug=true;
				valid_opt = true;
			}


			if (strcmp(argv[i], "--enable-contrib")==0) {
				enable_contrib=true;
				valid_opt = true;
			}

			if (strcmp(argv[i], "--language")==0 && i<argc+1) {
				systemConfig.lang=argv[i+1];
				valid_opt=true;
				i++;
			}
			if (strcmp(argv[i], "--ask-for-kdeversion")==0) {
				askForKDE = true;
				valid_opt = true;
			}

			if (strcmp(argv[i], "--skip-doc")==0) {
				_cmdOptions["skip_doc_installation"]="true";
				valid_opt=true;
			}
			if (strcmp(argv[i], "--skip-dev")==0) {
				_cmdOptions["skip_dev_installation"]="true";
				valid_opt=true;
			}
			if (strcmp(argv[i], "--skip-man")==0) {
				_cmdOptions["skip_man_installation"]="true";
				valid_opt=true;
			}
			if (strcmp(argv[i], "--skip-static")==0) {
				_cmdOptions["skip_static_a_installation"]="true";
				valid_opt=true;
			}

			if (strcmp(argv[i], "--cache")==0) {
				FORCE_CDROM_CACHE = true;
				valid_opt = true;
			}
			if (strcmp(argv[i], "--no-prelink")==0) {
				systemConfig.tryPrelink = false;
				valid_opt=true;
			}
			if (strcmp(argv[i], "-v")==0 || strcmp(argv[i], "--verbose")==0) {
				verbose = true;
				valid_opt = true;
			}
			if (!valid_opt) {
				printf(_("Error: unknown option %s\n"), argv[i] );
			}
			if (strcmp(argv[i], "--help")==0 || !valid_opt) 
			{
				bindtextdomain( "mpkg", "/usr/share/locale");
				textdomain("mpkg");
				printf(_("AgiliaLinux %s setup (build %s)\n"), DISTRO_VERSION, MPKG_BUILD);
				printf(_("Syntax:\n"));
				printf(_("\tsetup [ OPTIONS ]\nAvailable options:\n"));
	
				printf(_("\t--help                 Show this help\n"));
				printf(_("\t--сheck                Check package integrity before installation\n"));
				printf(_("\t--сache                Copy all packages to hard drive before installation\n"));
				printf(_("\t--no-eject             Do not eject CD/DVD after installation (useful when installing inside virtual machine)\n"));
//				printf(_("\t--autoeject            Automatically eject CD to change media\n"));
				printf(_("\t--no-deps              Disable dependencies\n"));
				printf(_("\t--root PARTITION_NAME  Set root partition name (for example, /dev/sda1)\n"));
				printf(_("\t--root_format FSTYPE   Set root filesystem type (for example, ext3\n"));
				printf(_("\t--swap PARTITION_NAME  Set swap partition name (for example, /dev/sda2, or auto to use first available)\n"));
				printf(_("\t--pkgsource SOURCE     Set package source (cdrom, dvd, iso, or network URL)\n"));
				printf(_("\t--accept-license       Automatically accept license terms\n"));
				printf(_("\t--no-warpmode          Disable fast installation mode (in case of failure, it ensures that you will have correct database)\n"));
				// Not actual:
				//printf(_("\t--ask-for-kdeversion   Ask for KDE version (if you have hybrid disk)\n"));
				//printf(_("\t--enable-addons        Enable addons installation (NOT RECOMMENDED)\n"));
				//printf(_("\t--enable-contrib       Enable contrib installation\n"));
				//printf(_("\t--install_type         Set package set (minimal | server | full | expert)\n"));
				printf(_("\t--skip-doc             Do not install /usr/doc part of packages (to save space)\n"));
				printf(_("\t--skip-man             Do not install /usr/man part of packages (to save space)\n"));
				printf(_("\t--skip-dev             Do not install /usr/include part of packages (to save space)\n"));
				printf(_("\t--skip-static          Do not install *.a static libs, to save space\n"));
				exit(0);
			}
		}
	}
	ext4_supported=checkForExt4();
	dialogMode=true;
	if (systemConfig.lang.empty()) {
		setlocale(LC_ALL, "");
		systemConfig.lang = selectLanguage();
		string optstr;
		for (int i=1; i<argc; ++i) {
			optstr += string(argv[i]) + " ";
		}
		ncInterface.uninit();
		return system("setup --language " + systemConfig.lang + " " + optstr);

	}
	setlocale(LC_ALL, systemConfig.lang.c_str());
	//char *clo = setlocale(LC_ALL, NULL);
	bindtextdomain( "mpkg", "/usr/share/locale");
	textdomain("mpkg");
	ncInterface.setStrings();
	ncInterface.cancelStr=_("BACK");
	if (systemConfig.lang=="ru_RU.UTF-8") i_menuHead = "Установка AgiliaLinux " + (string) string(DISTRO_VERSION);
	if (systemConfig.lang=="uk_UA.UTF-8") i_menuHead = "Встановлення AgiliaLinux " + (string) string(DISTRO_VERSION);
	ncInterface.setTitle(i_menuHead);

	if (!isDatabaseLocked()) {
		lockDatabase();
	}
	else {
		mError(_("Database is locked by another process"));//"База данных заблокирована другим процессом.");
		abort();
	}
	cdromList=getCdromList();
	unlink("/var/log/mpkg-lasterror.log");
	unlink("/var/log/mkfs.log");
	system("killall tail 2> /dev/null");
	system("tail -f /var/log/mkfs.log --retry >> /dev/tty4 2>/dev/null &");
	system("tail -f /var/log/mpkg-lasterror.log --retry >> /dev/tty4 2>/dev/null &");
showLicense:
	if (!unattended) showGreeting();
	if (!showLicense()) {
		
		ncInterface.showMsgBox(_("You cannot use this software without accepting the license.\nInstallation terminated"));//Без принятия условий данной лицензии использование данного программного обеспечения невозможно.\nУстановка прекращена.");
		return -1;
	}

	systemConfig.rootMountPoint="/mnt";
	string ret;
	string next_item;
	vector<MenuItem> menuItems;

	initDatabaseStructure();
	// Unattended options setup
	if (!sc.sysroot.empty() && sc.sysroot_format.empty()) setRootPartition(sc.sysroot);
	if (!sc.sysroot.empty() && !sc.sysroot_format.empty()) setRootPartition(sc.sysroot, sc.sysroot_format);
       	if (!sc.swap_partition.empty()) setSwapSpace(sc.swap_partition);
	if (!sc.pkgsource.empty()) packageSourceSelectionMenu(sc.pkgsource);
	if (!sc.install_type.empty()) {
		packageSelectionMenu(sc.install_type);
	}
	/* Boot loader unattended setup is not implemented now, because of dependencies from full partitioning model, need to implement it first
	if (!sc.sysroot.empty() && !sc.bootloader.empty()) setupBootloaderOptions(sc.bootloader);
	if (!sc.sysroot.empty() && !sc.bootloader_root.empty() && !sc.bootloader.empty()) setupBootloaderOptions(sc.bootloader, sc.bootloader_root);
	*/
	// Unattended end
selectInstallType:
	ncInterface.setSubtitle(i_menuHead + string(": ") + _("Installation mode"));// Тип установки
	menuItems.clear();
	menuItems.push_back(MenuItem(_("Standard"), _("Standard installation mode"), _("Simple installation step-by-step mode, recommended for most users. Some rarely-used questions are omitted (using defaults)")));
	menuItems.push_back(MenuItem(_("Advanced"), _("Advanced setup mode"), _("Full-featured menu-based installation mode. Use it if you need advanced options")));
	ret = ncInterface.showMenu2(_("Choose installation mode. In case of standard mode, setup will continue in step-by-step manner with minimum of questions. If you want full control over installation process - choose advanced mode"), menuItems);

	if (ret == _("Standard")) {
		if (runSimpleInstall()) return 0;
		else goto selectInstallType;
	}
	if (ret.empty()) goto showLicense;
	ret.clear();


main_menu:

	ncInterface.setSubtitle(i_menuHead + string(": ") + _("main menu"));
	menuItems.clear();
	menuItems.push_back(MenuItem("0",_("Perform hard drive partitioning"), _("Paritition table management: create, delete or modify partitions on your hard drive to prepare space for system installation.\n)")));
	if (systemConfig.swapPartition.empty()) menuItems.push_back(MenuItem("1", _("Choose swap partition"), _("Choose partition to use as a swap space.\nYou need it if you want to use suspend-to-disk feature, and it is strongly recommended if you have less than 1Gb RAM. Swap partition size should be equal or greater than RAM size")));
	else menuItems.push_back(MenuItem("1", _("Swap partition: ") + systemConfig.swapPartition));
	if (systemConfig.rootPartition.empty()) 
		menuItems.push_back(MenuItem("2",_("Choose root partition"), _("Root partition is a place on which Linux will be installed. Also, it is a root for filesystem tree.")));
	else menuItems.push_back(MenuItem("2",_("Root partition: ") + systemConfig.rootPartition + \
				" (" + systemConfig.rootPartitionType + _(", format: ") + doFormatString(systemConfig.rootPartitionFormat) + ")"));
	
	menuItems.push_back(MenuItem("3",_("Additional partitions configuration"), _("If you wish to attach additional partitions to your root tree at boot time (e.g. /boot, /home, or windows partitions), you can configure it here.")));
	if (systemConfig.sourceName.empty()) menuItems.push_back(MenuItem("4",_("Choose package source"), _("Choose where to get packages. It can be one of: DVD, a CD set, network repository, ISO image on HDD or available via Samba network, directory on hard drive or any other mounted filesystem).")));
	else menuItems.push_back(MenuItem("4", "Источник пакетов: " + systemConfig.sourceName));
	
	if (systemConfig.totalQueuedPackages==0) menuItems.push_back(MenuItem("5",_("Choose package installation set"), _("Choose a set of packages to be installed.")));
	else menuItems.push_back(MenuItem("5", _("Choose package set (selected: ") + systemConfig.setupMode + _(", total: ") + \
				IntToStr(systemConfig.totalQueuedPackages) + _(" packages)")));
	
	if (bootConfig.loaderType.empty()) menuItems.push_back(MenuItem("6", _("Boot loader configuration"),_("Boot loader is required to start your operating system. Without it, it will be unable to boot.")));
	else menuItems.push_back(MenuItem("6", _("Boot loader: ") + bootConfig.loaderType + _(", will be installed in ") + bootConfig.bootDevice, _("Boot loader is required to start your operating system. Without it, it will be unable to boot.")));
	menuItems.push_back(MenuItem("7",_("Install system"),_("Perform actual installation.\nWARNING: check all parameters (especially - partition names and formatting options): after confirmation, there will be no way to get back data from formatted partitions!")));
//	menuItems.push_back(MenuItem("8", "Восстановление загрузчика MBR", "Восстановление стандартного загрузчика MBR"));
	
	//menuItems.push_back(MenuItem("8", _("Edit repository list (optional)"), _("If you want to add some additional repositories or remove unneeded - go here")));
	//menuItems.push_back(MenuItem("9", _("Proprietary drivers installation")));
	menuItems.push_back(MenuItem("EXIT",_("Exit"), _("Exit from setup.")));
	
	ret = ncInterface.showMenu2(_("Choose required actions\nNavigation: UP/DOWN, TAB\nChoose: Enter key"), menuItems, next_item);
	if (ret == "0") if (diskPartitioningMenu()==0) next_item="1";
	if (ret == "1") if (setSwapSpace()==0) next_item="2";
	if (ret == "2") if (setRootPartition()==0) next_item="3";
	if (ret == "3") if (setOtherPartitions()==0) next_item="4";
	if (ret == "4") if (packageSourceSelectionMenu()==0) next_item="5";
	if (ret == "5") {
		if (packageSelectionMenu()==0 && switchAlternativesMenu()) next_item="6";
	}
	if (ret == "6") if (setupBootloaderOptions()==0) next_item="7";
	if (ret == "7") {
		if (!forceInInstallMD5Check) {
			forceInInstallMD5Check = ncInterface.showYesNo(_("Check package integrity before installation?"));
		}
		commit_ret = commit();
		if (commit_ret==0) return 0;
		if (commit_ret==2) {
			unlockDatabase();
			ncInterface.uninit();
			// FIXME: should be called ONLY if installing from CD/DVD!
			system("eject " + systemConfig.cdromDevice + " 2>/dev/tty4 >/dev/tty4");
	       		system("/usr/lib/setup/reboot_script &");
			return 0;
		}
	}
	/*if (ret == "8") {
		mpkg *core = new mpkg;
		manageRepositories(*core);
		if (ncInterface.showYesNo(_("Update data?"))) {
			core->update_repository_data();
			getCustomSetupVariants(core->get_repositorylist());
		}
		delete core;
		next_item="8";
	}
	if (ret == "9") {
		installAdditionalDrivers();
		next_item = "9";
	}*/
	if (ret == "EXIT" || ret.empty()) 
	{
		if (ncInterface.showYesNo(_("Do you really want to exit setup program?")))
		{
			unlockDatabase();
			return 0;
		}
	}
	goto main_menu;
}

int setupBootloaderOptions(bool simple)
{
	if (_cmdOptions["no_bootloader"]=="yes") {
		return 0;
	}
	vector<TagPair> devList = getDevList();
	vector<string> gp;
	vector<pEntry> pList = getGoodPartitions(gp);

	string bootLoader, loaderInstallMode, bootDevice;
	string bootRootFs, kernelOptions, vgaMode;
	ncInterface.setSubtitle(_("Boot loader configuration"));
	vector<MenuItem> menuItems;

	// Selecting boot loader type
selectBootLoaderType:
	menuItems.clear();
	menuItems.push_back(MenuItem("GRUB2", _("GRand Unified Bootloader 2 (default)")));
	//menuItems.push_back(MenuItem("GRUB", _("GRand Unified Bootloader (default)")));	
	menuItems.push_back(MenuItem("LILO", "LInux LOader"));
	menuItems.push_back(MenuItem("MSDOS", _("Restore standard Windows/DOS MBR boot loader")));
	menuItems.push_back(MenuItem("None", _("Do not install boot loader")));
	if (simple) bootLoader="GRUB2";
	else bootLoader = ncInterface.showMenu2(_("Choose boot loader type.\nIf unsure - choose default:"), menuItems);
	if (bootLoader.empty()) return -1;
	if (bootLoader=="None") return 0;
	if (bootLoader=="MSDOS") return restoreDefaultBootloader();

selectBootLoaderInstallMode:

	menuItems.clear();
	// Selecting partition/mbr mode
	menuItems.push_back(MenuItem("MBR", _("Master boot record")));
	menuItems.push_back(MenuItem("Partition", _("Hard drive partition")));
	if (bootLoader!="MSDOS" && !simple) {
		loaderInstallMode = ncInterface.showMenu2(_("Choose where do you want to install boot loader"),menuItems, "MBR");
	}
	else loaderInstallMode = "MBR";
	if (loaderInstallMode.empty()) goto selectBootLoaderType;

//selectBootLoaderInstallPartitionDetails:
	menuItems.clear();
	// Selecting drive/partition to install
	if (loaderInstallMode == "MBR") {
		if (devList.size()==0) {
			ncInterface.showMsgBox(_("Error: no hard drive detected"));
			return -1;
		}

		if (devList.size()==1 || simple) bootDevice = devList[0].tag;
		if (devList.size()>1) { 
			for (size_t i=0; i<devList.size(); i++) {
				menuItems.push_back(MenuItem(devList[i].tag, devList[i].value));
			}
			bootDevice = ncInterface.showMenu2(_("Choose hard drive to install boot loader ") + bootLoader + _(" on it's MBR. Select first HDD if unsure:"), menuItems);
			if (bootDevice.empty()) goto selectBootLoaderInstallMode;
		}
	}
	else {
		string tmp_mp;
		for (size_t i=0; i<pList.size(); i++) {
			tmp_mp.clear();
			for (size_t t=0; t<systemConfig.otherMounts.size(); t++) {
				if (systemConfig.otherMounts[t].tag == pList[i].devname) {
					tmp_mp = " [" +systemConfig.otherMounts[t].value+ "]";
				}
			}
			if (pList[i].devname==systemConfig.rootPartition) tmp_mp = " [/]";
			

			if (pList[i].devname!=systemConfig.rootPartition) menuItems.push_back(MenuItem(pList[i].devname, pList[i].fstype + tmp_mp + " (" + pList[i].size + "Mb)"));
			else menuItems.push_back(MenuItem(pList[i].devname,  pList[i].fstype + tmp_mp + " (" + pList[i].size + _("Mb, root, default)")));
		}
		bootDevice = ncInterface.showMenu2(_("Choose partition on hard drive, in which you want to install boot loader ") + bootLoader,menuItems,systemConfig.rootPartition);
		if (bootDevice.empty()) goto selectBootLoaderInstallMode;
	}

	// Set additional options for GRUB
	if (bootLoader == "GRUB" || bootLoader=="GRUB2") {
		// We will not ask user for that, just detect it.
		for (size_t i=0; i<systemConfig.otherMounts.size(); i++) {
			if (systemConfig.otherMounts[i].value=="/boot") bootRootFs = systemConfig.otherMounts[i].tag;
		}
		if (bootRootFs.empty()) bootRootFs = systemConfig.rootPartition;
	}
	if (bootLoader == "MSDOS") goto finish_bootloader_config;
selectFramebufferMode:
	// Select framebuffer mode
	menuItems.clear();
	menuItems.push_back(MenuItem("normal", _("Generic 80x25 text console")));
	if (!simple) {
		menuItems.push_back(MenuItem("257", _("640x480, 256 colors")));
		menuItems.push_back(MenuItem("259", _("800x600, 256 colors")));
		menuItems.push_back(MenuItem("261", _("1024x768, 256 colors")));
		menuItems.push_back(MenuItem("263", _("1280x1024, 256 colors")));
	
		menuItems.push_back(MenuItem("273", _("640x480, 16-bit color")));
		menuItems.push_back(MenuItem("276", _("800x600, 16-bit color")));
		menuItems.push_back(MenuItem("279", _("1024x768, 16-bit color")));
		menuItems.push_back(MenuItem("282", _("1280x1024, 16-bit color")));
	}
	menuItems.push_back(MenuItem("274", _("640x480, 24-bit color")));
	menuItems.push_back(MenuItem("277", _("800x600, 24-bit color")));
	menuItems.push_back(MenuItem("280", _("1024x768, 24-bit color (default)")));
	menuItems.push_back(MenuItem("283", _("1280x1024, 24-bit color")));
	
	menuItems.push_back(MenuItem(_("Other"), _("Define custom framebuffer mode")));
	vgaMode = ncInterface.showMenu2(_("Choose framebuffer graphics mode\nTo enable splash screen, you should choose 16-bit or 24-bit color mode\n"),menuItems, "280");
	if (vgaMode == _("Other")) {
		vgaMode = ncInterface.showInputBox(_("Enter VESA graphics mode (decimal):"));
		if (vgaMode.empty()) goto selectFramebufferMode;
	}
	if (vgaMode.empty()) return -1;
	

//selectKernelOptions:

	if (simple) kernelOptions = "acpi=force quiet splash";
	else kernelOptions = ncInterface.showInputBox(_("Check kernel boot options (separated by space, for example: acpi=force noapic quiet).\nIf unsure, leave default values"), "acpi=force quiet splash");

finish_bootloader_config:
	// So, we have determined that:
	bootConfig.rootFs = bootRootFs;
	bootConfig.kernelOptions = kernelOptions;
	bootConfig.loaderType = bootLoader;
	bootConfig.bootDevice = bootDevice;
	if (enable_setup_debug) ncInterface.showMsgBox("BOOTDEV: " + bootConfig.bootDevice);
	bootConfig.videoModeNumber = atoi(vgaMode.c_str());
	for (size_t i=0; i<menuItems.size(); i++) {
		if (menuItems[i].tag==vgaMode) {
			bootConfig.videoModeName = menuItems[i].value;
			break;
		}
	}
	return 0;
}

string getGfxPayload(string vgaMode) {
	if (vgaMode=="normal") return "";
	if (vgaMode=="257") return "640x480x8;640x480";
	if (vgaMode=="259") return "800x600x8;800x600";
	if (vgaMode=="261") return "1024x768x8;1024x768";
	if (vgaMode=="263") return "1280x1024x8;1280x1024";
	
	if (vgaMode=="273") return "640x480x16;640x480";
	if (vgaMode=="276") return "800x600x16;800x600";
	if (vgaMode=="279") return "1024x768x16;1024x768";
	if (vgaMode=="282") return "1280x1024x16;1280x1024";
	
	if (vgaMode=="274") return "640x480x24;640x480";
	if (vgaMode=="277") return "800x600x24;800x600";
	if (vgaMode=="280") return "1024x768x24;1024x768";
	if (vgaMode=="283") return "1280x1024x24;1280x1024";

	return "";
	
}
bool liloconfig()
{
	string vgaMode = IntToStr(bootConfig.videoModeNumber + 512);
	if (vgaMode == "512") vgaMode="normal";
	string initrdstring;
	string kernelstring = "/boot/vmlinuz";
	if (use_initrd) initrdstring = "initrd=/boot/initrd.gz\n";
	string bootPartition = systemConfig.rootPartition;
	/*for (size_t i=0; i<systemConfig.otherMounts.size(); i++) {
		if (systemConfig.otherMounts[i].value == "/boot") {
			bootPartition = systemConfig.otherMounts[i].tag;
			if (initrdstring.find("/boot")!=std::string::npos) initrdstring = "initrd=/initrd.gz";
			if (kernelstring.find("/boot")==0) kernelstring = "/vmlinuz";
			break;
		}
	}*/

	// Check if /boot is on separate device
	string liloConfig = \
"# LILO configuration file (created by AgiliaLinux " + string(DISTRO_VERSION) + " setup)\n\
# Global LILO options section\n\
lba32 # Allows to boot from cylinders over 1024\n\
append = \"" + bootConfig.kernelOptions + "\" # Kernel options\n\
boot = " + bootConfig.bootDevice + "\n\
install = menu\n\
menu-scheme = Wg:kw:Wg:Yg\n\
menu-title = \" SELECT SYSTEM TO BOOT \"\n\
prompt\n\
timeout = 300\n\
# Redefine harmful settings, which can rewrite partition table sometimes\n\
change-rules\n\
reset\n\
\n\
# VESA graphics console " + bootConfig.videoModeName + "\n\
vga = " + vgaMode + "\n\
\n\
# Linux boot partitions\n\
image=/boot/vmlinuz\n\
root=\"UUID=" + getUUID(systemConfig.rootPartition) + "\"\n" + \
initrdstring + \
"read-only\n\
label=AgiliaLinux\n\
\n";
// Let's add safe mode here
	liloConfig += "image=/boot/vmlinuz\n\
root=" + systemConfig.rootPartition + "\n" + \
"read-only\n\
append=\"init=/bin/sh\"\n\
label=SafeMode\n\
\n";
	vector<OsRecord> osList = getOsList();

	string os_id;
	/*for (size_t i=0; i<osList.size(); i++) {
		if (osList.size()>1) os_id="-"+IntToStr(i);
		if (osList[i].type == "linux") {
			liloConfig = liloConfig + "image="+osList[i].kernel+"\nroot=\"UUID="+getUUID(osList[i].root)+"\"\nread-only\nlabel="+osList[i].label+os_id + "\n\n";
		}
	}*/
	liloConfig = liloConfig + "# Секция загрузочных разделов Windows\n";
	for (size_t i=0; i<osList.size(); i++) {
		if (osList.size()>1) os_id="-"+IntToStr(i);
		if (osList[i].type == "other") {
			liloConfig = liloConfig + "other=" + getUUID(osList[i].root) + "\nlabel="+osList[i].label+os_id+"\n\n";
		}
	}


	WriteFile(systemConfig.rootMountPoint + "/etc/lilo.conf", liloConfig);
	int ret = system("chroot " + systemConfig.rootMountPoint + " lilo > /dev/null 2>/tmp/liloinstall_logfile");
	if (ret != 0) {
		return false;
	}
	return true;
}
StringMap getDeviceMap(string mapFile) {
	StringMap devMap;
	vector<string> devMapData = ReadFileStrings(mapFile);
	string grub_hd, linux_hd;
	for (size_t i=0; i<devMapData.size(); i++) {
		// Validate string
		if (devMapData[i].find("(")!=std::string::npos && devMapData[i].find(")")!=std::string::npos && devMapData[i].find("/")!=std::string::npos) {
			grub_hd = devMapData[i].substr(1, devMapData[i].find(")")-1);
			linux_hd = devMapData[i].substr(devMapData[i].find_first_of("/"));
			devMap.setValue(linux_hd,grub_hd);
		}
	}
	return devMap;
}

void remapHdd(StringMap& devMap, const string& root_mbr) {
	system("echo remapHdd > /dev/tty4"); //DEBUG
	// We need to swap MBR device and first device
	string mbr_num = devMap.getValue(root_mbr);
	int num = -1;
	for (size_t i=0; i<devMap.size(); ++i) {
		if (devMap.getValue(i)=="hd0") {
			num = i;
			break;
		}
	}
	if (num==-1) {
		system("echo No remapping possible > /dev/tty4");
		return;
	}
	string first_dev = devMap.getKeyName(num);
	devMap.setValue(root_mbr, string("hd0"));
	devMap.setValue(first_dev, mbr_num);
	
	system("echo remapHdd: boot from " + root_mbr + " is OK, replaced was: " + first_dev + " as " + mbr_num + " > /dev/tty4"); //DEBUG
}

string mapPart(StringMap devMap, string partName, int isGrubLegacy) {
	//system("echo mapPart > /dev/tty4"); //DEBUG
	for (size_t i=0; i<devMap.size(); i++) {
		if (partName.find(devMap.getKeyName(i))==0) {
			if (partName == devMap.getKeyName(i)) {
			//	system("echo mapPart 1 OK > /dev/tty4"); //DEBUG
				return devMap.getValue(i);
			}
			//system("echo mapPart 2 OK > /dev/tty4"); // DEBUG
			return devMap.getValue(i)+","+IntToStr(atoi(partName.substr(devMap.getKeyName(i).length()).c_str())-isGrubLegacy);
		}
	}

//	system("echo mapPart 3 OK > /dev/tty4"); // DEBUG
	return "";

}
bool msdosconfig(string device) {
	if (device.empty()) device = bootConfig.bootDevice;
	if (device.empty()) return false;
	if (system("dd ibs=440 count=1 if=/msdos_mbr of="+device + " 2>/dev/tty4 >/dev/tty4")==0) return true;
	return false;
}

bool grubconfig()
{
	/* Data needed:
	 systemConfig.rootMountPoint
	 bootConfig.videoModeNumber
	 systemConfig.rootPartition
	 systemConfig.otherMounts
	 systemConfig.otherMountFormat
	 bootConfig.kernelOptions
	 
	 */

	ncInterface.setSubtitle(_("Installing GRUB to ") + bootConfig.bootDevice);
	// Create a grub directory
	system("mkdir -p " + systemConfig.rootMountPoint + "/boot/grub/");
	// Copy image files
	system("cp " + systemConfig.rootMountPoint + "/usr/lib/grub/i386-pc/* " + systemConfig.rootMountPoint + "/boot/grub/");
	// Generate device map
	system("chroot " + systemConfig.rootMountPoint + " grub --batch --no-floppy --device-map=/boot/grub/device.map > /dev/tty4 2>/dev/tty4 << EOF\nquit");
	// Get the device map
	StringMap devMap = getDeviceMap(systemConfig.rootMountPoint + "/boot/grub/device.map");
	remapHdd(devMap, bootConfig.bootDevice);
	string vgaMode = IntToStr(bootConfig.videoModeNumber+512);
	if (vgaMode == "512") vgaMode="normal";

	// Check if /boot is located on partition other than root
	string bootPartition = systemConfig.rootPartition;
	string initrdstring;
	string kernelstring = "/boot/vmlinuz";
	if (use_initrd) initrdstring = "initrd /boot/initrd.gz\n";

	for (size_t i=0; i<systemConfig.otherMounts.size(); i++) {
		if (systemConfig.otherMounts[i].value == "/boot") {
			bootPartition = systemConfig.otherMounts[i].tag;
			if (initrdstring.find("/boot")!=std::string::npos) initrdstring = "initrd /initrd.gz";
			if (kernelstring.find("/boot")==0) kernelstring = "/vmlinuz";
			break;
		}
	}
	string grubBootPartition = systemConfig.rootPartition;
	for (size_t i=0; i<systemConfig.otherMounts.size(); ++i) {
		if (systemConfig.otherMounts[i].value == "/boot") {
			grubBootPartition = systemConfig.otherMounts[i].tag;
			break;
		}
	}
	system("echo GEN_MENU > /dev/tty4"); //DEBUG
	string grubConfig = "# GRUB configuration, generated by AgiliaLinux " + string(DISTRO_VERSION) + " setup\n\
timeout 30\n\
color white/green black/light-gray\n\
# End GRUB global section\n\
# Linux bootable partition config begins\n\
title AgiliaLinux " + string(DISTRO_VERSION) + " on " + systemConfig.rootPartition + "\n\
root ("+ mapPart(devMap, grubBootPartition) +")\n\
kernel " + kernelstring + " root=UUID=" + getUUID(systemConfig.rootPartition) + " ro vga=" + vgaMode+ " " + bootConfig.kernelOptions+"\n" + initrdstring + "\n";
// Add safe mode:
	grubConfig += "title AgiliaLinux " + string(DISTRO_VERSION) + " (safe mode) on " + systemConfig.rootPartition + "\n\
root ("+ mapPart(devMap, grubBootPartition) +")\n\
kernel " + kernelstring + " root=" + systemConfig.rootPartition + " ro vga=" + vgaMode+ " " + bootConfig.kernelOptions+" init=/bin/sh\n";

	vector<OsRecord> osList = getOsList();
	if (osList.size()<1) strReplace(&grubConfig, "timeout 30", "timeout 3");
	/*for (size_t i=0; i<osList.size(); i++) {
		if (osList[i].type == "linux") {
			grubConfig = grubConfig + "title " + osList[i].label + " on " + osList[i].root+"\nroot ("+mapPart(devMap, osList[i].root)+")\nkernel /boot/vmlinuz root="+getUUID(osList[i].root) + " ro vga=" + vgaMode + "\n\n";
		}
	}*/
	grubConfig = grubConfig + "# Other bootable partition config begins\n";
	for (size_t i=0; i<osList.size(); i++) {
		if (osList[i].type == "other") {
			grubConfig = grubConfig + "title " + osList[i].label + " on " + osList[i].root+"\nrootnoverify ("+mapPart(devMap, osList[i].root)+")\nmakeactive\nchainloader +1\n\n";
		}
	}
	grubConfig = grubConfig + "title --- For help press 'c', type: 'help'\n\
root ("+devMap.getValue(bootConfig.bootDevice)+")\n\
title --- For usage examples, type: 'cat /boot/grub/grub.txt'\n\
root ("+devMap.getValue(bootConfig.bootDevice)+")\n\n";
	WriteFile(systemConfig.rootMountPoint+"/boot/grub/menu.lst", grubConfig);

	// Generate default file and locate boot partition
	string root_dir;
	string grub_prefix="/boot/grub";
	string grubdir = "/boot/grub";
	if (bootPartition != systemConfig.rootPartition) {
		root_dir = " --root-directory=/boot ";
		grub_prefix="/grub";
	}
	system("chroot " + systemConfig.rootMountPoint + " grub-set-default " + root_dir + " default");

	string execGrubString = "#!/bin/sh\nchroot " + systemConfig.rootMountPoint + " grub --batch --device-map=/boot/grub/device.map << EOF > /tmp/grubinstall-logfile\nroot (" + mapPart(devMap, bootPartition) + ")\nsetup --prefix=" + grub_prefix + " (" + mapPart(devMap,bootConfig.bootDevice) + ")\nquit\nEOF\n";
	string tmpScript = get_tmp_file();
	WriteFile(tmpScript, execGrubString);

	system("sh " + tmpScript);

	if (!FileExists("/tmp/grubinstall-logfile") || ReadFile("/tmp/grubinstall-logfile").find("Error ")!=std::string::npos) {
		return false;
	}
	else {
		ncInterface.showInfoBox(_("GRUB boot loader installed successfully"));
	}

	return true;
}

bool grub2config()
{
	/* Data needed:
	 systemConfig.rootMountPoint
	 bootConfig.videoModeNumber
	 systemConfig.rootPartition
	 systemConfig.otherMounts
	 systemConfig.otherMountFormat
	 bootConfig.kernelOptions
	 
	 */

	ncInterface.setSubtitle(_("Installing GRUB2 to ") + bootConfig.bootDevice);
	string grubcmd = "chroot " + systemConfig.rootMountPoint + " grub-install --no-floppy " + bootConfig.bootDevice + " 2>/dev/tty4 >/tmp/grubinstall-logfile";
	if (enable_setup_debug) {
		while(!ncInterface.showYesNo("Check GRUB cmd:\n" + grubcmd)) {
			grubcmd = ncInterface.showInputBox("Edit grub cmd:", grubcmd);
		}
		ncInterface.uninit();
	}
	system(grubcmd);
	// Get the device map
	StringMap devMap = getDeviceMap(systemConfig.rootMountPoint + "/boot/grub/device.map");
	remapHdd(devMap, bootConfig.bootDevice);

	//string vgaMode = IntToStr(bootConfig.videoModeNumber+512);
	//if (vgaMode == "512") vgaMode="normal";
	string gfxPayload = getGfxPayload(IntToStr(bootConfig.videoModeNumber));
	if (!gfxPayload.empty()) gfxPayload = "\tset gfxpayload=\"" + gfxPayload + "\"\n";
	// Check if /boot is located on partition other than root
	string bootPartition = systemConfig.rootPartition;
	string initrdstring;
	string kernelstring = "/boot/vmlinuz";
	if (use_initrd) initrdstring = "initrd /boot/initrd.gz\n";

	string fontpath = "/boot/grub/unifont.pf2";
	string pngpath = "/boot/grub/grub640.png";

	for (size_t i=0; i<systemConfig.otherMounts.size(); i++) {
		if (systemConfig.otherMounts[i].value == "/boot") {
			bootPartition = systemConfig.otherMounts[i].tag;
			if (initrdstring.find("/boot")!=std::string::npos) initrdstring = "initrd /initrd.gz";
			if (kernelstring.find("/boot")==0) kernelstring = "/vmlinuz";
			fontpath = "/grub/unifont.pf2";
			pngpath = "/grub/grub640.png";
			break;
		}
	}
	string grubBootPartition = systemConfig.rootPartition;
	for (size_t i=0; i<systemConfig.otherMounts.size(); ++i) {
		if (systemConfig.otherMounts[i].value == "/boot") {
			grubBootPartition = systemConfig.otherMounts[i].tag;
			break;
		}
	}
	string grubConfig = "# GRUB2 configuration, generated by AgiliaLinux " + string(DISTRO_VERSION) + " setup\n\
set timeout=10\n\
set default=0\n\
set root=("+ mapPart(devMap, grubBootPartition, 0) +")\n\
insmod video\n\
insmod vbe\n\
insmod font\n\
loadfont " + fontpath + "\n\
insmod gfxterm\n\
set gfxmode=\"640x480x24;640x480\"\n\
terminal_output gfxterm\n\
insmod png\n\
background_image " + pngpath + "\n\
set menu_color_normal=black/black\n\
set menu_color_highlight=white/dark-gray\n\
# End GRUB global section\n\
# Linux bootable partition config begins\n\
menuentry \"" + string(_("AgiliaLinux ") + string(DISTRO_VERSION) + _(" on ")) + systemConfig.rootPartition + "\" {\n\
\tset root=(" + mapPart(devMap, grubBootPartition, 0) + ")\n" + gfxPayload + \
"\tlinux " + kernelstring + " root=UUID=" + getUUID(systemConfig.rootPartition) + " ro " + bootConfig.kernelOptions+"\n\t" + initrdstring + "\n}\n\n";
// Add safe mode
	grubConfig += "menuentry \"" + string(_("AgiliaLinux ") + string(DISTRO_VERSION) + _(" (recovery mode) on ")) + systemConfig.rootPartition + "\" {\n" + gfxPayload + \
"\tlinux ("+ mapPart(devMap, grubBootPartition, 0) +")" + kernelstring + " root=" + systemConfig.rootPartition + " ro " + bootConfig.kernelOptions+" single\n}\n\n";
	vector<OsRecord> osList = getOsList();
	if (osList.size()<1) strReplace(&grubConfig, "timeout=10", "timeout=3");
	grubConfig = grubConfig + "# Other bootable partition config begins\n";
	for (size_t i=0; i<osList.size(); i++) {
		if (osList[i].type == "other") {
			grubConfig = grubConfig + "menuentry \" " + osList[i].label + " on " + osList[i].root+"\" {\n\tset root=("+mapPart(devMap, osList[i].root, 0)+")\n\tchainloader +1\n}\n\n";
		}
	}
	WriteFile(systemConfig.rootMountPoint+"/boot/grub/grub.cfg", grubConfig);

	if (!FileExists("/tmp/grubinstall-logfile") || ReadFile("/tmp/grubinstall-logfile").find("Error ")!=std::string::npos) {
		return false;
	}
	else {
		ncInterface.showInfoBox(_("GRUB2 boot loader installed successfully"));
	}

	return true;
}

vector<OsRecord> getOsList()
{
	vector<OsRecord> ret;
	OsRecord item;
	vector<string> gp;
	vector<pEntry> pList = getGoodPartitions(gp);
	bool cleanFs;
	for (size_t i=0; i<pList.size(); i++) {
		if (pList[i].devname == systemConfig.rootPartition) {
			continue;

		}
		cleanFs=false;
		for (size_t t=0; t<systemConfig.otherMounts.size(); t++) {
			if (pList[i].devname == systemConfig.otherMounts[t].tag && systemConfig.otherMountFormat[t]) {
				cleanFs=true;
				break;
			}
		}
		if (!cleanFs) {
			// Not a new clean FS, add as OS by FS type
			
			//----------------windows-----------------
			if (pList[i].fstype == "ntfs" || pList[i].fstype.find("fat")!=std::string::npos) {
				// This can be windows
				if (pList[i].devname.find_first_of("0123456789")!=std::string::npos && atoi(pList[i].devname.substr(pList[i].devname.find_first_of("0123456789")).c_str())<=4) {
					item.label="Windows";
					item.kernel.clear();
					item.type="other";
					item.root=pList[i].devname;
					ret.push_back(item);
				}
			}

			//----------------linux-------------------
			if (pList[i].fstype == "ext4" || pList[i].fstype == "ext3" || pList[i].fstype == "ext2" || pList[i].fstype == "xfs" || pList[i].fstype == "jfs" || pList[i].fstype == "reiserfs" || pList[i].fstype == "btrfs" || pList[i].fstype == "nilfs2") {
				item.label="Linux";
				item.kernel="/boot/vmlinuz";
				item.type="linux";
				item.root=pList[i].devname;
				ret.push_back(item);
			}


		}
	}
	return ret;
}

string ncGetMountPartition(string header) {
	// Selects the partition
	vector<string> gp;
	vector<pEntry> pList;
	pList= getGoodPartitions(gp);
	ncInterface.setSubtitle(header);
	vector<MenuItem> menuItems;
	for (size_t i=0; i<pList.size(); i++)
	{
		menuItems.push_back(MenuItem(pList[i].devname, pList[i].fstype + " (" + pList[i].size + "Mb)"));
	}
	int menu_ret=0;
       while (menu_ret != -1) {
		menu_ret = ncInterface.showMenu(_("Choose partition to mount:"), menuItems);
		if (menu_ret == -1) return "";
		string mountcmd = "mount";
		// Creating directory
		string mPoint = "/tmp/mpkgmount/" + string(pList[menu_ret].devname);
		if (pList[menu_ret].fstype=="ntfs") mountcmd = "ntfs-3g";
		system("umount " + mPoint + " 2>/dev/null >/dev/null");
		system("mkdir -p " + mPoint);
		if (system(mountcmd + " " + string(pList[menu_ret].devname) + " " + mPoint + " > /dev/null 2>/dev/tty4")==0) {
			systemConfig.tmpMounts.push_back(TagPair(pList[menu_ret].devname, mPoint));
			return mPoint;
		}
		else if (!ncInterface.showYesNo(_("Failed to mount partition ") + pList[menu_ret].devname + _(". Try another?"))) return "";
       }
       return "";
}


int buildInitrd() {
	// RAID, LVM, LuKS encrypted volumes, USB drives and so on - all this will be supported now!
	//ncInterface.uninit();
	if (ncInterface.showYesNo(_("Do you need a delay to initialize your boot disk?\nIf you're installing system on USB drive, say [YES], otherwise you have a chance to get unbootable system.\n\nIf unsure - say YES, in worst case it just will boot 10 seconds longer."))) rootdev_wait = "10";

	system("chroot /mnt mount -t proc none /proc 2>/dev/null >/dev/null");
	string rootdev;
        if (systemConfig.rootPartitionType!="btrfs") rootdev = "UUID=" + getUUID(systemConfig.rootPartition);
	else rootdev = systemConfig.rootPartition; // Mounting by UUID doesn't work with btrfs, I don't know why.
	string use_swap, use_raid, use_lvm;
	if (!systemConfig.swapPartition.empty()) use_swap = "-h " + systemConfig.swapPartition;
	if (systemConfig.rootPartition.find("/dev/md")==0) use_raid = " -R ";
	if (systemConfig.rootPartition.find("/dev/vg")==0) use_lvm = " -L ";
	string kernelversion = systemConfig.kernelversion;
	bool retry = false;
	string additional_modules;
	do {
		retry = false;
		additional_modules = ncInterface.showInputBox(_("If you need additional modules to boot, specify it here separating by semicolon (':'), otherwise leave this field blank."));
		if (!additional_modules.empty()) {
			if (additional_modules.find_first_of(" <>|\n\t'\"`#")!=std::string::npos) {
				if (ncInterface.showYesNo(_("Incorrect characters in input, retry?"))) retry = true;
			}
			else additional_modules = " -m " + additional_modules;
		}
	} while (retry);

	ncInterface.showInfoBox(_("Creating initrd..."));
	system("chroot /mnt mkinitrd -c -r " + rootdev + " -f " + systemConfig.rootPartitionType + " -w " + rootdev_wait + " -k " + kernelversion + " " + " " + use_swap + " " + use_raid + " " + use_lvm + " " + additional_modules + " 2>/dev/tty4 >/dev/tty4");
	
	return 0;
}

