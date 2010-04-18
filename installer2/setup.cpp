/* New MPKG installer: actually run setup
 */

#include <mpkg/libmpkg.h>
#include "default_paths.h"
#include <mpkg-parted/mpkg-parted.h>
#include "setup.h"
int activateSwapSpace(const string& swapPartition) {
	// activates the swap space
	if (swapPartition.empty()) return 0;
	// Disconnect swap if it is connected, recreate and connect.
	system("swapoff " + swapPartition + " 2>/dev/null >/dev/null");
	if (system("mkswap " + swapPartition + " 2>/var/log/mpkg-lasterror.log >/dev/null")) {
		ncInterface.showMsgBox(_("Error creating swap on ") + swapPartition + _(", reason: ") + ReadFile("/var/log/mpkg-lasterror.log"));
		return -1;
	}
	if (system("swapon " + swapPartition + " 2>/dev/null >/dev/null")!=0) {
		ncInterface.showMsgBox(_("An error occured while enabling swap space: ") + ReadFile("/var/log/mpkg-lasterror.log"));
		return -2;
	}
	return 0;
}

bool formatPartition(const string& devname, const string& fstype) {
	string fs_options;
	if (fstype=="jfs") fs_options="-q";
	if (fstype=="xfs") fs_options="-f -q";
	if (fstype=="reiserfs") fs_options="-q";
	if (system("umount -l " + devname +  " 2>/var/log/mpkg-lasterror.log ; mkfs -t " + fstype + " " + fs_options + " " + devname + " 2>> /var/log/mpkg-lasterror.log 1>>/var/log/mkfs.log")==0) return true;
	else return false;
}

int formatPartitions(const vector<string>& formatting) {
	ncInterface.setSubtitle(_("Formatting partitions"));
	ncInterface.showInfoBox(_("Formatting filesystems..."));
	// Read list and format, lol :)
	string dev, fs;
	for (size_t i=1; i<formatting.size(); i=i+2) {
		dev = formatting[i-1];
		fs = formatting[i];
		if (dev.empty() || fs.empty()) {
			ncInterface.showMsgBox(_("Warning: malformed data in formatting on line ") + IntToStr(i));
			continue;
		}
		if (!formatPartition(dev, fs)) {
			ncInterface.showMsgBox(_("An error occured while formatting partition ") + dev + ":\n" + ReadFile("/var/log/mpkg-lasterror.log"));
			return 1;
		}
	}
	return 0;
}

int mountPartitions(const vector<TagPair>& otherMounts, const vector<string> otherMountFSTypes, string root="") {
	// Mount all HDD partitions
	ncInterface.setSubtitle(_("Mounting partitions"));
	string mount_cmd;
	string mkdir_cmd;
	ncInterface.showInfoBox(_("Mounting root partition (") + root + ")");

	mkdir_cmd = "mkdir -p /mnt 2>/var/log/mpkg-lasterror.log >/dev/null";
	mount_cmd = "mount " + root + " /mnt 2>>/var/log/mpkg-lasterror.log >/dev/null";
	if (system(mkdir_cmd) !=0 || system(mount_cmd)!=0) {
		ncInterface.showMsgBox(_("An error occured while mounting root filesystem:\n") + ReadFile("/var/log/mpkg-lasterror.log"));
		return 1;
	}
	else mDebug("root mkdir and mount OK");

	// Sorting mount points
	vector<int> mountOrder, mountPriority;
	for (size_t i=0; i<otherMounts.size(); i++) {
		mountPriority.push_back(0);
	}

	for (size_t i=0; i<otherMounts.size(); i++) {
		for (size_t t=0; t<otherMounts.size(); t++) {
			if (otherMounts[i].value.find(otherMounts[t].value)==0) mountPriority[i]++;
			if (otherMounts[i].value[0]!='/') mountPriority[i]=-1; // FIXME: WTF is this?
		}
	}

	for (size_t i=0; i<mountPriority.size(); i++) {
		for (size_t t=0; t<mountPriority.size(); t++) {
			if (mountPriority[t]==(int) i) mountOrder.push_back(t);
		}
	}
	// Mounting others...

	for (size_t i=0; i<mountOrder.size(); i++) {
		ncInterface.showInfoBox(_("Mounting other partitions: ") + otherMounts[mountOrder[i]].tag + " ["+ \
				otherMounts[mountOrder[i]].value+"]");
		mkdir_cmd = "mkdir -p /mnt" + otherMounts[mountOrder[i]].value + " 2>/var/log/mpkg-lasterror.log >/dev/null";
		if (otherMountFSTypes[mountOrder[i]]=="ntfs")  mount_cmd = "ntfs-3g -o force " + otherMounts[mountOrder[i]].tag + " /mnt" + otherMounts[mountOrder[i]].value + " 2>>/var/log/mpkg-lasterror.log >/dev/null";
		else mount_cmd = "mount " + otherMounts[mountOrder[i]].tag + " /mnt" + otherMounts[mountOrder[i]].value + " 2>>/var/log/mpkg-lasterror.log >/dev/null";
		if (otherMountFSTypes[mountOrder[i]]=="jfs") mount_cmd = "fsck " + otherMounts[mountOrder[i]].tag + " 2>>/var/log/mpkg-lasterror.log >/dev/null && " + mount_cmd;

		if (system(mkdir_cmd)!=0 || system(mount_cmd)!=0) {
			ncInterface.showMsgBox(_("An error occured while mounting partition ") + \
					otherMounts[mountOrder[i]].tag + "\n:" + ReadFile("/var/log/mpkg-lasterror.log"));
			return -1;
		}
	}
	return 0;
}

void generateLangSh(string lang, string dir="/mnt/etc/profile.d/") {
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
	strReplace(&lang_sh, "$L", lang);
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
	ret.push_back(MenuItem("uk", _("Ukrainian"), "winkeys"));
	ret.push_back(MenuItem("uk", _("Ukrainian (UNIX)"), ""));
	ret.push_back(MenuItem("il", _("Hebrew"), ""));
	ret.push_back(MenuItem("il", _("Hebrew (phonetic)"), "phonetic"));
	return ret;
}
vector<MenuItem> askXkbLayout(string lang) {
	vector<MenuItem> al = getAvailableXkbLayouts();
	for (size_t i=0; i<al.size(); ++i) {
		if (al[i].tag=="us") al[i].flag = true;
		if (al[i].tag=="ru" && al[i].description=="winkeys" && lang.find("ru")==0) al[i].flag=true;
		if (al[i].tag=="uk" && al[i].description=="winkeys" && lang.find("uk")==0) al[i].flag=true;
	}
	ncInterface.showExMenu(_("Select X11 keyboard layouts. If not sure, leave this as is"), al);
	return al;
	

}
void xorgSetLangHALEx(string language) {
	string lang, varstr;
	vector<MenuItem> langmenu = askXkbLayout(language);
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
	
/*	if (systemConfig.lang.find("ru")==0) lang="us,ru";
	if (systemConfig.lang.find("uk")==0) lang="us,uk";
	if (systemConfig.lang.find("en")==0) lang="us";*/
	string fdi = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><!-- -*- SGML -*- -->\n\
<match key=\"info.capabilities\" contains=\"input.keyboard\">\n\
  <merge key=\"input.xkb.layout\" type=\"string\">" + lang + "</merge>\n" + variant + \
"  <merge key=\"input.xkb.options\" type=\"string\">terminate:ctrl_alt_bksp,grp:ctrl_shift_toggle,grp_led:scroll</merge>\n\
</match>\n";
	system("mkdir -p /mnt/etc/hal/fdi/policy 2>/dev/null >/dev/null");
	WriteFile("/mnt/etc/hal/fdi/policy/10-x11-input.fdi", fdi);
}

void generateIssue(string lang, string dir="/mnt/etc/") {
	if (!FileExists(dir+"issue_" + lang)) {
		return;
	}
	system("(cd " + dir + " 2>/dev/tty4 >/dev/null ; rm issue > /dev/null 2>/dev/tty4 ; ln -s issue_" + lang + " issue > /dev/null 2>/dev/tty4 )");
}
void setDefaultRunlevel(const string& lvl) {
	// Can change runlevels 3 and 4 to lvl
	string data = ReadFile("/mnt/etc/inittab");
	strReplace(&data, "id:4:initdefault", "id:" + lvl + ":initdefault");
	strReplace(&data, "id:3:initdefault", "id:" + lvl + ":initdefault");
	WriteFile("/mnt/etc/inittab", data);
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

void writeFstab(string root, string rootPartitionType, string swap, const vector<TagPair>& otherMounts, const vector<string> &otherMountFSTypes) {
	string data;
	if (!swap.empty()) data = "# " + swap + "\nUUID="+getUUID(swap) + "\tswap\tswap\tdefaults\t0 0\n";

	data+= "# " + root + "\nUUID=" + getUUID(root) + "\t/\t" + rootPartitionType + "\tdefaults\t1 1\n";
	string options="defaults";
	string fstype="auto";
	
	// Sorting
	vector<int> mountOrder, mountPriority;
	for (size_t i=0; i<otherMounts.size(); i++) {
		mountPriority.push_back(0);
	}
	for (size_t i=0; i<otherMounts.size(); i++) {
		for (size_t t=0; t<otherMounts.size(); t++) {
			if (otherMounts[i].value.find(otherMounts[t].value)==0)
			{
				mountPriority[i]++;
			}
			if (otherMounts[i].value[0]!='/') mountPriority[i]=-1;
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
	// Storing data
	for (size_t i=0; i<mountOrder.size(); i++)
	{
		options="defaults";
#ifdef AUTO_FS_DETECT
		fstype="auto";
#else
		fstype=otherMountFSTypes[mountOrder[i]];
#endif
		if (fstype=="hfs+") fstype = "hfsplus";
		if (otherMountFSTypes[mountOrder[i]].find("fat")!=std::string::npos)
		{
			options="rw,codepage=866,iocharset=utf8,umask=000,showexec,quiet";
			fstype="vfat";
		}
		if (otherMountFSTypes[mountOrder[i]].find("ntfs")!=std::string::npos)
		{
			options="locale=ru_RU.utf8,umask=000";
			fstype="ntfs-3g";
		}
		data += "# " + otherMounts[mountOrder[i]].tag + "\nUUID="+getUUID(otherMounts[mountOrder[i]].tag) + "\t" + otherMounts[mountOrder[i]].value + "\t"+fstype+"\t" + options + "\t1 1\n";
	}
	data += "\n# Required for X shared memory\nnone\t/dev/shm\ttmpfs\tdefaults\t0 0\n";
	WriteFile("/mnt/etc/fstab", data);
}
int buildInitrd(string root, string rootPartitionType, string swap) {
	// RAID, LVM, LuKS encrypted volumes, USB drives and so on - all this will be supported now!
	//ncInterface.uninit();
	string rootdev_wait;
	if (ncInterface.showYesNo(_("Do you need a delay to initialize your boot disk?\nIf you're installing system on USB drive, say [YES], otherwise you have a chance to get unbootable system.\n\nIf unsure - say YES, in worst case it just will boot 10 seconds longer."))) rootdev_wait = "10";

	ncInterface.showInfoBox(_("Creating initrd..."));
	system("chroot /mnt mount -t proc none /proc 2>/dev/null >/dev/null");
	system("chroot /mnt mkinitrd 2>/dev/null >/dev/null");
	WriteFile("/mnt/boot/initrd-tree/rootdev", root);
	WriteFile("/mnt/boot/initrd-tree/rootfs", rootPartitionType);
	WriteFile("/mnt/boot/initrd-tree/initrd-name", "agilialinux-10.4-initrd");
	WriteFile("/mnt/boot/initrd-tree/wait-for-root", rootdev_wait);
	if (!swap.empty()) WriteFile("/mnt/boot/initrd-tree/resumedev", swap);
	
	// To ensure in all cases, let's copy ALL modules to initrd. TODO: remove unneeded tons of them :)
	// new TODO: add some :)))
	//system("chroot /mnt cp -R /lib/modules/`uname -r` /boot/initrd-tree/lib/modules/");
	
	system("chroot /mnt cp /sbin/mdadm /boot/initrd-tree/sbin/ 2>/dev/null >/dev/null");
	system("chroot /mnt mkinitrd >/dev/null 2>/dev/null");
	return 0;
}

/*vector<OsRecord> getOsList(const string& root)
{
	vector<OsRecord> ret;
	OsRecord item;
	vector<pEntry> pList = getPartitionList();
	bool cleanFs;
	for (size_t i=0; i<pList.size(); i++) {
		if (pList[i].devname == root) {
			continue;

		}
		cleanFs=false;
		for (size_t t=0; t<otherMounts.size(); t++) {
			if (pList[i].devname == otherMounts[t].tag && systemConfig.otherMountFormat[t]) {
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
*/
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



bool grub2config(const string& root, const vector<TagPair>& otherMounts)
{
	string kernelOptions = "acpi=force quiet splash";

	string bootDevice = ReadFile(SETUPCONFIG_BOOTLOADER_TARGET);
	string videoModeNumber = ReadFile(SETUPCONFIG_FRAMEBUFFER);
	ncInterface.setSubtitle(_("Installing GRUB2 to ") + bootDevice);
	string grubcmd = "chroot /mnt grub-install --no-floppy " + bootDevice + " 2>/dev/tty4 >/tmp/grubinstall-logfile";
	system(grubcmd);
	// Get the device map
	StringMap devMap = getDeviceMap("/mnt/boot/grub/device.map");
	remapHdd(devMap, bootDevice);

	string vgaMode = IntToStr(atoi(videoModeNumber.c_str())+512);
	if (vgaMode == "512") vgaMode="normal";

	// Check if /boot is located on partition other than root
	string bootPartition = root;
	string initrdstring;
	string kernelstring = "/boot/vmlinuz";
	initrdstring = "initrd /boot/initrd.gz\n";

	string fontpath = "/boot/grub/unifont.pf2";
	string pngpath = "/boot/grub/grub640.png";

	for (size_t i=0; i<otherMounts.size(); i++) {
		if (otherMounts[i].value == "/boot") {
			bootPartition = otherMounts[i].tag;
			if (initrdstring.find("/boot")!=std::string::npos) initrdstring = "initrd /initrd.gz";
			if (kernelstring.find("/boot")==0) kernelstring = "/vmlinuz";
			fontpath = "/grub/unifont.pf2";
			pngpath = "/grub/grub640.png";
			break;
		}
	}
	string grubBootPartition = root;
	for (size_t i=0; i<otherMounts.size(); ++i) {
		if (otherMounts[i].value == "/boot") {
			grubBootPartition = otherMounts[i].tag;
			break;
		}
	}
	string grubConfig = "# GRUB2 configuration, generated by AgiliaLinux 7.0 setup\n\
set timeout=10\n\
set default=0\n\
set root=("+ mapPart(devMap, grubBootPartition, 0) +")\n\
insmod video\n\
insmod vbe\n\
insmod font\n\
loadfont " + fontpath + "\n\
insmod gfxterm\n\
set gfxmode=\"640x480x16;640x480\"\n\
vbeinfo\n\
terminal_output gfxterm\n\
insmod png\n\
background_image " + pngpath + "\n\
# End GRUB global section\n\
# Linux bootable partition config begins\n\
menuentry \"" + string(_("AgiliaLinux 7.0 on ")) + root + "\" {\n\
\tset root=(" + mapPart(devMap, grubBootPartition, 0) + ")\n\
\tlinux " + kernelstring + " root=" + getUUID(root) + " ro vga=" + vgaMode+ " " + kernelOptions+"\n\t" + initrdstring + "\n}\n\n";
// Add safe mode
	grubConfig += "menuentry \"" + string(_("AgiliaLinux 7.0 (recovery mode) on ")) + root + "\" {\n\
\tlinux ("+ mapPart(devMap, grubBootPartition, 0) +")" + kernelstring + " root=" + root + " ro vga=" + vgaMode+ " " + kernelOptions+" single\n}\n\n";
	vector<OsRecord> osList;// = getOsList(root);
	if (osList.size()<1) strReplace(&grubConfig, "timeout=10", "timeout=3");
	for (size_t i=0; i<osList.size(); i++) {
		if (osList[i].type == "linux") {
			grubConfig = grubConfig + "menuentry \"" + osList[i].label + _(" on ") + osList[i].root+"\" {\n\tlinux ("+mapPart(devMap, osList[i].root, 0)+")/boot/vmlinuz root="+getUUID(osList[i].root) + " ro vga=" + vgaMode + "\n}\n\n";
		}
	}
	grubConfig = grubConfig + "# Other bootable partition config begins\n";
	for (size_t i=0; i<osList.size(); i++) {
		if (osList[i].type == "other") {
			grubConfig = grubConfig + "menuentry \" " + osList[i].label + " on " + osList[i].root+"\" {\n\tset root=("+mapPart(devMap, osList[i].root, 0)+")\n\tchainloader +1\n}\n\n";
		}
	}
	WriteFile("/mnt/boot/grub/grub.cfg", grubConfig);

	if (!FileExists("/tmp/grubinstall-logfile") || ReadFile("/tmp/grubinstall-logfile").find("Error ")!=std::string::npos) {
		return false;
	}
	else {
		ncInterface.showInfoBox(_("GRUB2 boot loader installed successfully"));
	}

	return true;
}

bool setHostname() {
	string hostname = ncInterface.showInputBox(_("Enter hostname (for example, agilia):"));
	if (hostname.empty()) return false;
	string netname = ncInterface.showInputBox(_("Enter network name (for example, example.net):"), "example.net");
	if (netname.empty()) return false;
	WriteFile("/mnt/etc/HOSTNAME", hostname + "." + netname + "\n");
	string hosts = ReadFile("/mnt/etc/hosts");
	strReplace(&hosts, "darkstar", hostname);
	strReplace(&hosts, "example.net", netname);
	WriteFile("/mnt/etc/hosts", hosts);
	return true;
}
bool setUserPasswordPwd(string username) {
	ncInterface.uninit();
	system("chroot /mnt passwd " + username);
	return 0;
}
bool setRootPassword() {
	return setUserPasswordPwd("root");
}

bool addUser(string username) {
	//string extgroup="audio,cdrom,disk,floppy,lp,scanner,video,wheel"; // Old default groups
	string extgroup="audio,cdrom,floppy,video,netdev,plugdev,power"; // New default groups, which conforms current guidelines
	system("chroot /mnt /usr/sbin/useradd -d /home/" + username + " -m -g users -G " + extgroup + " -s /bin/bash " + username + " 2>/dev/tty4 >/dev/tty4");
	system("chroot /mnt chown -R " + username+":users /home/"+username + " 2>/dev/tty4 >/dev/tty4");
	system("chmod 711 /mnt/home/" + username + " 2>/dev/tty4 >/dev/tty4");
	return true;
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
			addUser(username);
			//setUserPassword(username);
			setUserPasswordPwd(username);
			username.clear();
			add_else_user = _("another ");
		}
	}
	return true;
	
}

int performConfig(const vector<TagPair>& otherMounts, const vector<string>& otherMountFSTypes, const string& rootPartitionType) {
	string lang = ReadFile(SETUPCONFIG_LANGUAGE);
	string root = ReadFile(SETUPCONFIG_ROOT);
	string swap = ReadFile(SETUPCONFIG_SWAP);
	string loaderType = ReadFile(SETUPCONFIG_BOOTLOADER);
	ncInterface.setSubtitle(_("Initial system configuration"));
	generateLangSh(lang);
	xorgSetLangHALEx(lang);
	generateIssue(lang);
	
	writeFstab(root, rootPartitionType, swap, otherMounts, otherMountFSTypes);
	//buildInitrd(root, rootPartitionType, swap);
	buildInitrd(getUUID(root), rootPartitionType, swap); // Seems that root fs should be mounted using UUID even here. Need to test moar btw.
	vector<MenuItem> networkManagers;
	string selectedNetworkManager;
	if (FileExists("/mnt/usr/sbin/wicd")) {
		networkManagers.push_back(MenuItem("wicd", _("Wicd is an open source wired and wireless network manager")));
		system("chmod -x /mnt/etc/rc.d/rc.wicd >/dev/tty4 2>/dev/tty4");
	}
	if (FileExists("/mnt/sbin/netconfig")) networkManagers.push_back(MenuItem("netconfig", _("Generic netconfig, Slackware default network settings manager")));
	if (!loaderType.empty()) ncInterface.showInfoBox(_("Installing bootloader: ") + loaderType);
	if (loaderType == "GRUB2") {
		if(!grub2config(root, otherMounts)) {
		       ncInterface.showMsgBox(_("GRUB2 installation failed. Your system will NOT boot without bootloader. Now you should install it manually."));
		}
	}

	/*else if (loaderType == "LILO") {
		if (!liloconfig()) {
			ncInterface.showMsgBox(_("Installation of LILO failed. Your system will NOT boot without bootloader"));
		}
	}*/
	setenv("ROOT_DEVICE", root.c_str(), 1);
	setenv("COLOR", "on", 1);
	WriteFile("/var/log/setup/tmp/SeTT_PX", "/mnt/\n");
	WriteFile("/var/log/setup/tmp/SeTrootdev", root+"\n");
	WriteFile("/var/log/setup/tmp/SeTcolor", "on\n");
	
	if (!networkManagers.empty()) {
		ncInterface.setSubtitle(_("Network setup"));
	selectNetManagerMode:
		selectedNetworkManager = ncInterface.showMenu2(_("Please select preferred network manager:"), networkManagers);

		if (selectedNetworkManager == "wicd") {
			system("chmod +x /mnt/etc/rc.d/rc.wicd >/dev/tty4 2>/dev/tty4");
			if (!setHostname()) goto selectNetManagerMode;
		}
		if (selectedNetworkManager == "netconfig") {
			ncInterface.uninit();
			system("chroot /mnt LC_ALL=" + lang + " netconfig");
		}
	}
	ncInterface.setSubtitle(_("Initial system configuration"));
	ncInterface.showInfoBox(_("Configuring your system..."));
	// Skip some question in simple mode
	
	ncInterface.uninit(); // Disable ncurses

	system("chmod -x /mnt/etc/rc.d/rc.pcmcia 2>/dev/null >/dev/null"); // skip PCMCIA
	
		system("AUTOCONFIG=imps2 LC_ALL=" + lang + " /mnt/var/log/setup/setup.mouse /mnt"); // Set defaults for GDM
		system("chmod -x /mnt/var/log/setup/setup.mouse"); // skip GPM config in future: already configured
		system("chmod -x /mnt/var/log/setup/setup.services"); // skip services config. Defaults are ok for 90% of users, and another 10% can set this manually later or use advanced install mode
		system("chmod -x /mnt/var/log/setup/setup.80.make-bootdisk"); // We never shipped USB boot creation by default, neither will now. Let it for advanced users and ones who failed to install bootloader

	

	if (system("LC_ALL=" + lang + " /usr/lib/setup/SeTconfig")!=0){
		ncInterface.setSubtitle(_("Initial system configuration"));
		ncInterface.showMsgBox(_("An error occured while performing initial system configuration, but it is not critical. You can continue."));
	}
	setRootPassword();
	addNewUsers();
	//installAdditionalDrivers();
	if (FileExists("/mnt/usr/bin/X")) {
		if (FileExists("/mnt/usr/bin/kdm") ||
		    FileExists("/mnt/usr/bin/xdm") ||
    		    FileExists("/mnt/usr/sbin/gdm") ||
		    FileExists("/mnt/usr/bin/slim")) {
			if (ncInterface.showYesNo(_("Enable X11 login by default?"))) {
				setDefaultRunlevel("4");
			}
		}
	}
	return 0;
}

void syncFS()
{

	string cdromDevice = ReadFile(SETUPCONFIG_CDROM);
	system("sync >/dev/tty4 2>/dev/tty4");
	//if (noEject) system("umount " + cdromDevice + " >/dev/null 2>/dev/null");
	//else {
		system("eject " + cdromDevice + " 2>/dev/null >/dev/null");
		system("echo EJECTING in syncFS > /dev/tty4");
	//}
}


int dbConfig() {
	mDebug("Writing the data");
	// Goals: create an appropriate mpkg configuration for installed system
	system("cp /tmp/mpkg/mpkg.xml /mnt/etc/mpkg.xml");
	system("cp /tmp/mpkg/packages.db /mnt/var/mpkg/packages.db");
	
	CONFIG_FILE="/mnt/etc/mpkg.xml";
	mConfig.configName=CONFIG_FILE;

	mpkg *core = new mpkg;
	core->set_sysroot("/");
	mConfig.setValue("database_url", "sqlite:///var/mpkg/packages.db");
	delete core;
	return 0;
}




int main(int argc, char **argv) {
	dialogMode = true;
	setupMode = true;
	vector<string> emptyVector;

	// First, read configuration from config directory
	string language = ReadFile(SETUPCONFIG_LANGUAGE);
	string swap = ReadFile(SETUPCONFIG_SWAP);
	string root = ReadFile(SETUPCONFIG_ROOT);
	vector<string> formatting = ReadFileStrings(SETUPCONFIG_FORMATTING);
	vector<string> mountpoints = ReadFileStrings(SETUPCONFIG_MOUNT);
	vector<string> pkgsources = ReadFileStrings(SETUPCONFIG_PKGSOURCE);
	string pkgset = ReadFile(SETUPCONFIG_PKGSET);
	vector<string> tmpRepositoryList = ReadFileStrings(SETUPCONFIG_REPOSITORYLIST);
	vector<string> repositoryList;
	for (size_t i=0; i<tmpRepositoryList.size(); ++i) {
		if (cutSpaces(tmpRepositoryList[i]).empty()) continue;
		repositoryList.push_back(tmpRepositoryList[i]);
	}
	vector<string> tempMounts = ReadFileStrings(SETUPCONFIG_TEMPMOUNTS);

	// Create temporary database
	system("rm -rf /tmp/mpkg");
	system("mkdir -p /tmp/mpkg");
	system("cp /packages.db /tmp/mpkg/packages.db");
	system("cp /mpkg-temp.xml /tmp/mpkg/mpkg.xml");
	CONFIG_FILE="/tmp/mpkg/mpkg.xml";
	mConfig.configName=CONFIG_FILE;
	mConfig.setValue("database_url", "sqlite:///tmp/mpkg/packages.db");
	TEMP_SYSROOT="/mnt/";
	SYS_ROOT=TEMP_SYSROOT;
	TEMP_CACHE="/mnt/var/mpkg/cache/";
	SYS_CACHE = TEMP_CACHE;



	string summary;
	string errors;
	if (root.empty()) errors += _("You didn't specified a root partition\n");
	else if (root==swap) errors += _("Root and swap partition cannot be the same. Please fix this.\n");
	if (repositoryList.empty()) errors += _("You didn't specified a package source\n");
	if (pkgset.empty()) errors += _("No package set selected\n");

	if (!errors.empty())
	{
		ncInterface.setSubtitle(_("Some errors has been detected in configuration"));
		ncInterface.showMsgBox(errors);
		return 1;
	}

	// If everything is OK - let's go!
	
	// Create MPKG core, set repository list and update repository data
	mpkg *core = new mpkg;
	core->set_repositorylist(repositoryList, emptyVector);
	core->update_repository_data();

	// Now, get available packages from database, and according to chosen package set, mark some to installation
	// Step 1: load package list
	PACKAGE_LIST *pkgList = new PACKAGE_LIST;
	SQLRecord sqlSearch;
	core->get_packagelist(sqlSearch, pkgList);
	
	// Step 2: load package set
	vector<string> pkgSetList, pkgSetVersions;

	if (pkgset!=_("FULL")) {
		parseInstallList(ReadFileStrings(SETUP_PKGSET_CACHE_DIR+pkgset+".list"), pkgSetList, pkgSetVersions);
		if (pkgSetList.empty()) {
			ncInterface.showMsgBox(_("Empty package set!"));
			delete core;
			return 1;
		}
	}
	
	bool mark;
	string nvidia = ReadFile(SETUPCONFIG_NVIDIA);

	for (size_t i=0; i<pkgList->size(); ++i) {
		if (pkgList->at(i).get_repository_tags()=="additional-drivers") continue;
		if (nvidia.empty()) {
			if (pkgList->at(i).get_corename()=="nvidia-kernel" || pkgList->at(i).get_corename()=="nvidia-driver") continue;
		}
		mark = false;
		// TODO: Drop any alternatives here, if it doesn't specified to force
		if (pkgset=="FULL") mark=true;
		else {
			for (size_t z=0; z<pkgSetList.size(); ++z) {
				if (pkgSetList[z]==pkgList->at(i).get_name()) {
					mark = true;
					break;
				}
			}
		}
		if (mark) pkgList->get_package_ptr(i)->set_action(ST_INSTALL, "marked");
	}
	vector<string> alternatives = ReadFileStrings(SETUPCONFIG_ALTERNATIVES);
	if (!nvidia.empty() && cutSpaces(nvidia)!="generic") alternatives.push_back(cutSpaces(nvidia));
	pkgList->switchAlternatives(alternatives);
	
	
	vector<string> commitList;
	for (size_t i=0; i<pkgList->size(); i++) {
		if (pkgList->at(i).action()==ST_INSTALL) {
			commitList.push_back(pkgList->at(i).get_name());
		}
	}
	delete pkgList;
	ncInterface.showMsgBox("Prepared " + IntToStr(commitList.size()) + " packages to install");
	core->install(commitList);
	/*int depErrorCount = core->DepTracker->renderData();
	if (depErrorCount!=0) {
		ncInterface.setSubtitle(_("Error: unresolved dependencies"));
		string depFailList;
		for (unsigned int i=0; i<core->DepTracker->get_failure_list().size(); ++i) {
		       depFailList += core->DepTracker->get_failure_list().at(i).get_name() + "-" + core->DepTracker->get_failure_list().at(i).get_fullversion() + "\n";
		}
		WriteFile("/tmp/depErrTable.log", depFailList);
		ncInterface.showMsgBox(depFailList);
		return 1;
	}*/
	// If all ok - let's GO!!!!

	if (activateSwapSpace(swap)!=0) return -1;
	if (formatPartitions(formatting)!=0) return -1;

	// Parse mounts stuff
	vector<TagPair> otherMounts;
	string dev, mpoint;
	for (size_t i=1; i<mountpoints.size(); i=i+2) {
		dev = mountpoints[i-1];
		mpoint = mountpoints[i];
		if (dev.empty() || mpoint.empty()) continue;
		otherMounts.push_back(TagPair(dev, mpoint));
	}

	vector<string> otherMountFSTypes;
	vector<pEntry> pList = getPartitionList();
	string rootPartitionType;
	for (size_t i=0; i<pList.size(); ++i) {
		if (pList[i].devname==root) {
			rootPartitionType = pList[i].fstype;
			continue;
		}
		for (size_t z=0; z<otherMounts.size(); ++z) {
			if (otherMounts[z].tag!=pList[i].devname) continue;
			otherMountFSTypes.push_back(pList[i].fstype);
		}
	}

	if (mountPartitions(otherMounts, otherMountFSTypes, root)!=0) return -1;
	// Now create directory structure for mpkg:
	system("mkdir -p /mnt/tmp");
	system("mkdir -p /mnt/var/mpkg/{cache,scripts}");
	system("mkdir -p /mnt/var/log/packages");
	ncInterface.showMsgBox("Press OK to perform install");
	if (core->commit()!=0) {
		ncInterface.showMsgBox(_("Installation failed. Look and 4th console for details."));
		return 1;
	}

	if (performConfig(otherMounts, otherMountFSTypes, rootPartitionType)!=0) return -1;
	delete core;
	if (dbConfig()!=0) return -1;
	syncFS();








}
