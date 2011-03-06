#include "thread.h"
SetupThread *progressWidgetPtr;
void updateProgressData(ItemState a) {
	progressWidgetPtr->updateData(a);
}

MpkgErrorReturn SetupThread::errorHandler(ErrorDescription err, const string& details) {
	QString qdetails = QString::fromStdString(details);
	emit sendErrorHandler(err, qdetails);
	errCode=MPKG_RETURN_WAIT;
	while (errCode==MPKG_RETURN_WAIT) {
		printf("Waiting for errCode\n");
		sleep(1);
	}
	if (errCode!=MPKG_RETURN_ACCEPT && errCode!=MPKG_RETURN_SKIP && errCode!=MPKG_RETURN_RETRY && errCode!=MPKG_RETURN_CONTINUE && errCode!=MPKG_RETURN_IGNORE && errCode!=MPKG_RETURN_OK) {
		emit reportError(tr("An error occured during package installation. Setup will exit now."));
	}
	return errCode;
}
void SetupThread::receiveErrorResponce(MpkgErrorReturn ret) {
	printf("Received errorCode %d\n", ret);
	errCode = ret;
	
}
void SetupThread::updateData(const ItemState& a) {
	emit setDetailsText(QString::fromStdString(a.name + ": " + a.currentAction));
	//if (a.progress>=0 && a.progress<=100) ui->currentProgressBar->setValue(a.progress);
	if (a.totalProgress>=0 && a.totalProgress<=100) emit setProgress(a.totalProgress);
}

void SetupThread::setDetailsTextCallback(const string& msg) {
	emit setDetailsText(QString::fromStdString(msg));
}

void SetupThread::sendReportError(const string& text) {
	emit reportError(QString::fromStdString(text));
}


void SetupThread::getCustomSetupVariants(const vector<string>& rep_list) {
	emit setSummaryText(tr("Retrieving setup variants"));
	emit setDetailsText(tr("Retrieving list..."));
	agiliaSetup.getCustomSetupVariants(rep_list, this);
}

void SetupThread::skipMD5() {
	forceSkipLinkMD5Checks = true;
	forceInInstallMD5Check = false;
	emit enableMD5Button(false);
}


bool SetupThread::validateConfig() {

	emit setSummaryText(tr("Validating config"));
	emit setDetailsText("");
	if (settings->value("bootloader").toString().isEmpty()) {
		emit reportError(tr("Bootloader partition not specified, how you wish to boot, Luke?"));
		return false;
	}
	if (settings->value("setup_variant").toString().isEmpty()) {
		emit reportError(tr("No setup variant selected, cannot continue"));
		return false;
	}
	if (settings->value("pkgsource").toString().isEmpty()) {
		emit reportError(tr("No package source specified, cannot continue"));
		return false;
	}
	else {
		if (settings->value("pkgsource").toString()=="dvd") {
			if (settings->value("volname").toString().isEmpty()) {
				emit reportError(tr("No volume name specified, cannot continue"));
				return false;
			}
			if (settings->value("rep_location").toString().isEmpty()) {
				emit reportError(tr("Repository location not specified, cannot continue"));
				return false;
			}
		}
	}
	if (settings->value("timezone").toString().isEmpty()) {
		emit reportError(tr("No timezone specified, cannot continue"));
		return false;
	}
	return true;
}

bool SetupThread::setMpkgConfig() {
	emit setSummaryText(tr("Setting MPKG config"));
	emit setDetailsText("");

	vector<string> additional_repositories;
	int add_url_size = settings->beginReadArray("additional_repositories");
	for (int i=0; i<add_url_size; ++i) {
		settings->setArrayIndex(i);
		additional_repositories.push_back(settings->value("url").toString().toStdString());
	}
	settings->endArray();

	agiliaSetup.setMpkgConfig(settings->value("pkgsource").toString().toStdString(), settings->value("volname").toString().toStdString(), settings->value("rep_location").toString().toStdString(), additional_repositories);
	return true;
}

bool SetupThread::getRepositoryData() {
	emit setSummaryText(tr("Updating repository data"));
	emit setDetailsText(tr("Retrieving indices from repository"));
	return agiliaSetup.getRepositoryData(this);
}

bool SetupThread::prepareInstallQueue() {
	emit setSummaryText(tr("Preparing install queue"));
	emit setDetailsText("");
	return agiliaSetup.prepareInstallQueue(settings->value("setup_variant").toString().toStdString(), settings->value("netman").toString().toStdString(),settings->value("nvidia-driver").toString().toStdString(), this);
}

bool SetupThread::validateQueue() {
	emit setSummaryText(tr("Validating queue"));
	emit setDetailsText("");
	return agiliaSetup.validateQueue(this);
}

bool SetupThread::formatPartition(PartConfig pConfig) {
	return agiliaSetup.formatPartition(pConfig, this);
}

bool SetupThread::makeSwap(PartConfig pConfig) {
	return agiliaSetup.makeSwap(pConfig, this);
}

bool SetupThread::activateSwap(PartConfig pConfig) {
	return agiliaSetup.activateSwap(pConfig);
}
bool SetupThread::fillPartConfigs() {
	PartConfig *pConfig;

	settings->beginGroup("mount_options");
	QStringList partitions = settings->childGroups();
	QStringList subgroups;

	// Searching for deep partitions (LVM, RAID, etc)
	for (int i=0; i<partitions.size(); ++i) {
		settings->beginGroup(partitions[i]);
		subgroups = settings->childGroups();
		printf("Scanning group %s, size = %d\n", partitions[i].toStdString().c_str(), subgroups.size());
		for (int s=0; s<subgroups.size(); ++s) {
			partitions.push_back(partitions[i]+"/"+subgroups[s]);
		}
		settings->endGroup();
	}
	for (int i=0; i<partitions.size(); ++i) {

		printf("Found config for partition %s\n", partitions[i].toStdString().c_str());
		settings->beginGroup(partitions[i]);
		pConfig = new PartConfig;
		pConfig->partition = partitions[i].toStdString();
		pConfig->mountpoint = settings->value("mountpoint").toString().toStdString();
		pConfig->format = settings->value("format").toBool();
		pConfig->fs = settings->value("fs").toString().toStdString();
		pConfig->mount_options = settings->value("options").toString().toStdString();
		settings->endGroup();
		if (!pConfig->mountpoint.empty()) partConfigs.push_back(*pConfig); // Skip subvolume groups from list
		delete pConfig;
	}
	settings->endGroup();
	if (partConfigs.empty()) return false;
	return true;
}
bool SetupThread::formatPartitions() {
	emit setSummaryText(tr("Formatting partitions"));
	emit setDetailsText("");
	
	for (size_t i=0; i<partConfigs.size(); ++i) {
		if (partConfigs[i].format) {
			formatPartition(partConfigs[i]);
		}
		else if (partConfigs[i].mountpoint == "swap") {
			makeSwap(partConfigs[i]);
			activateSwap(partConfigs[i]);
		}
	}
	return true;
}

bool SetupThread::mountPartitions() {
	emit setSummaryText(tr("Mounting partitions"));
	emit setDetailsText("");

	for (size_t i=0; i<partConfigs.size(); ++i) {
		if (partConfigs[i].mountpoint=="/") {
			rootPartition = "/dev/" + partConfigs[i].partition;
			rootPartitionType = partConfigs[i].fs;
			rootPartitionMountOptions=partConfigs[i].mount_options;
		}
		else if (partConfigs[i].mountpoint == "swap") swapPartition = "/dev/" + partConfigs[i].partition;
	}

	string mount_cmd;
	string mkdir_cmd;
	string mount_options;

	mkdir_cmd = "mkdir -p /tmp/new_sysroot";
	if (!rootPartitionMountOptions.empty()) mount_options = "-o " + rootPartitionMountOptions;
	mount_cmd = "mount " + mount_options + " " + rootPartition + " /tmp/new_sysroot";
	if (system(mkdir_cmd) !=0 || system(mount_cmd)!=0) return false;

	// Sorting mount points
	
	// Mount order: setting priority 0 to all partitions.
	vector<int> mountOrder, mountPriority;
	for (size_t i=0; i<partConfigs.size(); i++) {
		mountPriority.push_back(0);
	}

	int max_priority=-1;
	// Increase priority if some partition should be inside other
	for (size_t i=0; i<partConfigs.size(); i++) {
		for (size_t t=0; t<partConfigs.size(); t++) {
			if (partConfigs[i].mountpoint.find(partConfigs[t].mountpoint)==0) {
				mountPriority[i]++;
				if (mountPriority[i]>max_priority) max_priority=mountPriority[i];
			}
			if (partConfigs[i].mountpoint[0]!='/') mountPriority[i]=-1; // In case of incorrectly specified mount point or swap one
		}
	}
	// Building mount order, according to mount priority
	for (int i=0; i<=max_priority; i++) {
		for (size_t t=0; t<mountPriority.size(); t++) {
			if (mountPriority[t]==i) mountOrder.push_back(t);
		}
	}
	if (mountPriority.size()!=partConfigs.size()) {
		printf("Mount priority: size mismatch: %d vs %d\nTHIS IS A CRITICAL BUG, ABORTING\n", (int) mountPriority.size(), (int) partConfigs.size());
		abort();
	}

	// Mounting partitions, skipping root one

	for (size_t i=0; i<mountOrder.size(); i++) {
		printf("Checking to mount: %s\n", partConfigs[mountOrder[i]].mountpoint.c_str());
		if (partConfigs[mountOrder[i]].mountpoint=="/") continue;
		if (partConfigs[mountOrder[i]].mountpoint=="swap") continue;
		if (partConfigs[mountOrder[i]].mount_options.empty()) mount_options.clear();
		else mount_options = "-o " + partConfigs[mountOrder[i]].mount_options;
		mkdir_cmd = "mkdir -p /tmp/new_sysroot" + partConfigs[mountOrder[i]].mountpoint;
		if (partConfigs[mountOrder[i]].fs=="ntfs")  {
			if (mount_options.empty()) mount_options="-o force";
			else mount_options+=",force";
			mount_cmd = "ntfs-3g " + mount_options + " /dev/" + partConfigs[mountOrder[i]].partition + " /tmp/new_sysroot" + partConfigs[mountOrder[i]].mountpoint;
		}
		else mount_cmd = "mount " + mount_options + " /dev/" + partConfigs[mountOrder[i]].partition + " /tmp/new_sysroot" + partConfigs[mountOrder[i]].mountpoint;
		if (partConfigs[mountOrder[i]].fs=="jfs") mount_cmd = "fsck /dev/" + partConfigs[mountOrder[i]].partition + "  && " + mount_cmd;

		printf("Mounting partition: %s\n", mount_cmd.c_str());
		if (system(mkdir_cmd)!=0 || system(mount_cmd)!=0) {
			emit reportError(tr("Failed to mount partition %1").arg(partConfigs[mountOrder[i]].partition.c_str()));
			return false;
		}
	}
	return true;

}

bool SetupThread::moveDatabase() {
	emit setSummaryText(tr("Moving database"));
	emit setDetailsText("");
	system("mkdir -p /tmp/new_sysroot/var/mpkg/cache");
	system("mkdir -p /tmp/new_sysroot/var/log");
	return true;

}

bool SetupThread::processInstall() {
	emit setSummaryText(tr("Installing packages"));
	emit setDetailsText(tr("Preparing to installation"));
	core = new mpkg;
	if ((settings->value("pkgsource")=="dvd" || settings->value("pkgsource").toString().toStdString().find("iso://")==0) && FileExists("/var/log/mount/cache")) {
		system("ln -sf /var/log/mount/cache /tmp/new_sysroot/var/mpkg/cache/.fcache");
	}
	else if (settings->value("pkgsource")=="file:///bootmedia/repository/" && FileExists("/bootmedia/cache")) {
		system("ln -sf /bootmedia/cache /tmp/new_sysroot/var/mpkg/cache/.fcache");
	}
	printf("Committing\n");
	if (core->commit()!=0) {
		delete core;
		printf("Commit failed");
		return false;
	}
	printf("\n\n\nCommit OK, going to post-install\n\n\n\n");
	emit setSummaryText(tr("Finishing installation"));
	emit setDetailsText("");
//	core->exportBase("/tmp/new_sysroot/var/log/packages"); // Not used anymore

	delete core;
	return true;
}
void SetupThread::xorgSetLangConf() {
	string lang = "us";
	sysconf_lang = "en_US.UTF-8";
	if (settings->value("language").toString()=="ru_RU.UTF-8") {
	       lang +=",ru";
	       sysconf_lang = "ru_RU.UTF-8";
	}
	if (settings->value("language").toString()=="uk_UA.UTF-8") {
	       	lang +=",ua";
		sysconf_lang = "uk_UA.UTF-8";
	}

	string keymap = "# Keyboard settings\n\
Section \"InputClass\"\n\
\tIdentifier \"KeymapSettings\"\n\
\tMatchIsKeyboard \"on\"\n\
\tOption      \"AutoRepeat\"  \"250 30\"\n\
\tOption      \"XkbRules\"    \"xorg\"\n\
\tOption      \"XkbModel\"    \"pc105\"\n\
\tOption      \"XkbLayout\"   \"" + lang + "\"\n\
\tOption      \"XkbOptions\"  \"grp:alt_shift_toggle,grp_led:scroll\"\n\
EndSection\n";

	system("mkdir -p /tmp/new_sysroot/etc/X11/xorg.conf.d");
	WriteFile("/tmp/new_sysroot/etc/X11/xorg.conf.d/10-keymap.conf", keymap);

}


void SetupThread::generateIssue() {
	if (!FileExists("/tmp/new_sysroot/etc/issue_" + sysconf_lang)) {
		return;
	}
	system("( cd /tmp/new_sysroot/etc ; rm issue ; ln -s issue_" + sysconf_lang + " issue )");
}

string getUUID(const string& dev) {
	string tmp_file = get_tmp_file();
	string data;
	int try_count = 0;
	while (try_count<2 && data.empty()) {
		system("blkid -s UUID " + dev + " > " + tmp_file);
		data = ReadFile(tmp_file);
		try_count++;
	}
	if (data.empty()) return "";
	size_t a = data.find_first_of("\"");
	if (a==std::string::npos || a==data.size()-1) return "";
	data.erase(data.begin(), data.begin()+a+1);
	a = data.find_first_of("\"");
	if (a==std::string::npos || a==0) return "";
	return data.substr(0, a);
}

void SetupThread::writeFstab() {

	for (size_t i=0; i<partConfigs.size(); ++i) {
		if (partConfigs[i].mountpoint == "swap") swapPartition = "/dev/" + partConfigs[i].partition;
	}

	string data;
	if (!swapPartition.empty()) {
		string swapUUID = getUUID(swapPartition);
		if (swapUUID.empty()) swapUUID = swapPartition;
		else swapUUID="UUID=" + swapUUID;
		data = "# " + swapPartition + "\n" + swapUUID + "\tswap\tswap\tdefaults\t0 0\n";
	}

	string rootUUID = getUUID(rootPartition);
	if (rootUUID.empty()) rootUUID = rootPartition;
	else rootUUID = "UUID=" + rootUUID;
	string f_rootPartitionMountOptions = cutSpaces(rootPartitionMountOptions);
	if (f_rootPartitionMountOptions.empty()) f_rootPartitionMountOptions = "defaults";
	data+= "# " + rootPartition + "\n" + rootUUID + "\t/\t" + rootPartitionType + "\t" + f_rootPartitionMountOptions + "\t1 1\n";
	
	string options="defaults";
	string fstype="auto";

	string fsUUID;	
	// Storing data
	for (size_t i=0; i<partConfigs.size(); i++)
	{
		if (partConfigs[i].mountpoint == "/" || partConfigs[i].mountpoint == "swap") continue;
		partConfigs[i].mount_options=cutSpaces(partConfigs[i].mount_options);
		options = "defaults";
		if (!partConfigs[i].mount_options.empty()) options=partConfigs[i].mount_options;
		fstype = partConfigs[i].fs;
		if (fstype=="hfs+") fstype = "hfsplus";
		if (partConfigs[i].fs.find("fat")!=std::string::npos) {
			options="rw,codepage=866,iocharset=utf8,umask=000,showexec,quiet";
			if (!partConfigs[i].mount_options.empty()) options+="," + partConfigs[i].mount_options;
			fstype="vfat";
		}
		if (partConfigs[i].fs.find("ntfs")!=std::string::npos) {
			options="locale=ru_RU.utf8,umask=000";
			if (!partConfigs[i].mount_options.empty()) options+="," + partConfigs[i].mount_options;
			fstype="ntfs-3g";
		}
		fsUUID = getUUID("/dev/" + partConfigs[i].partition);
		if (fsUUID.empty()) fsUUID = "/dev/" + partConfigs[i].partition;
		else fsUUID="UUID=" + fsUUID;
		data += "# " + partConfigs[i].partition + "\n"+ fsUUID + "\t" + partConfigs[i].mountpoint + "\t"+fstype+"\t" + options + "\t1 1\n";
	}
	data += "\n# Required for X shared memory\nnone\t/dev/shm\ttmpfs\tdefaults\t0 0\n";
	if (settings->value("tmpfs_tmp").toBool()) data += "\n# /tmp as tmpfs\nnone\t/tmp\ttmpfs\tdefaults\t0 0\n";
	
	WriteFile("/tmp/new_sysroot/etc/fstab", data);
}
void SetupThread::buildInitrd() {
	// RAID, LVM, LuKS encrypted volumes, USB drives and so on - all this will be supported now!
	string rootdev_wait = "0";
	if (settings->value("initrd_delay").toBool()) rootdev_wait = "10";

	string rootdev;
	string rootUUID = getUUID(rootPartition);
	if (rootUUID.empty()) rootUUID=rootPartition;
	else rootUUID="UUID=" + rootUUID;
	string use_swap, use_raid, use_lvm;
	if (!swapPartition.empty()) use_swap = "-h " + swapPartition;
	// Temporary (or permanent?) workaround: always add RAID modules to initrd, since we don't know how to detect it in case of RAID+LVM. See bug http://trac.agilialinux.ru/ticket/1292 for details.
	use_raid = " -R ";
	// Maybe it is ok to force LVM too, but I leave it alone for a while.
	if (rootPartition.size()>strlen("/dev/") && rootPartition.substr(strlen("/dev/")).find("/")!=std::string::npos) use_lvm = " -L ";

	if (rootPartitionType!="btrfs" && use_lvm.empty()) rootdev = rootUUID;
	else rootdev = rootPartition; // Mounting by UUID doesn't work with btrfs, I don't know why.

	string additional_modules;
	additional_modules = settings->value("initrd_modules").toString().toStdString();
	if (!additional_modules.empty()) additional_modules = " -m " + additional_modules;

	emit setSummaryText(tr("Creating initrd"));
	emit setDetailsText("");
	int ret = system("chroot /tmp/new_sysroot mkinitrd -c -r " + rootdev + " -f " + rootPartitionType + " -w " + rootdev_wait + " -k " + kernelversion + " " + " " + use_swap + " " + use_raid + " " + use_lvm + " " + additional_modules);
	// In case if first run fails (happened only twice, but happened)
	if (ret) system("chroot /tmp/new_sysroot mkinitrd");
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
		return;
	}
	string first_dev = devMap.getKeyName(num);
	devMap.setValue(root_mbr, string("hd0"));
	devMap.setValue(first_dev, mbr_num);
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

string mapPart(StringMap devMap, string partName, int isGrubLegacy) {
	for (size_t i=0; i<devMap.size(); i++) {
		if (partName.find(devMap.getKeyName(i))==0) {
			if (partName == devMap.getKeyName(i)) {
				return devMap.getValue(i);
			}
			return devMap.getValue(i)+","+IntToStr(atoi(partName.substr(devMap.getKeyName(i).length()).c_str())-isGrubLegacy);
		}
	}
	return "";

}

vector<OsRecord> SetupThread::getOsList() {
	vector<OsRecord> ret;
	// This is a stub: will read from settings what stuff to boot.
	return ret;
}

bool SetupThread::grub2_install() {
	string bootDevice = settings->value("bootloader").toString().toStdString();
	if (bootDevice=="NONE") return true;

	emit setSummaryText(tr("Installing GRUB2 to %1").arg(bootDevice.c_str()));
	emit setDetailsText("");

	// Installing GRUB into device
	int ret = system("chroot /tmp/new_sysroot grub-install --no-floppy --force " + bootDevice);
	if (ret!=0) {
		emit setSummaryText(tr("Failed to install GRUB2 to %1").arg(bootDevice.c_str()));
		emit setDetailsText(tr("grub-install returned %1, see log for details").arg(ret));

		printf("FATAL: Failed to install grub via grub-install!\n");
		return false;
	}
	else {
		emit setSummaryText(tr("Generating GRUB2 menu"));
		emit setDetailsText(tr(""));

		printf("GRUB2 installed successfully, generating config\n");
		bool c_ret = grub2_mkconfig();
		if (!c_ret) c_ret = grub2config();
		if (!c_ret) {
			emit setSummaryText(tr("Failed to generate GRUB2 configuration, expecting boot failure.").arg(bootDevice.c_str()));
			emit setDetailsText(tr("You have to create config manually."));

			printf("FATAL: Failed to create grub.cfg using both methods!\n");
			return false;
		}
		return true;
	}
	


}

bool SetupThread::grub2_mkconfig() {
	string bootDevice = settings->value("bootloader").toString().toStdString();
	if (bootDevice=="NONE") return true;

	// Fixing /etc/default/grub
	string grub_default = ReadFile("/tmp/new_sysroot/etc/default/grub");
	strReplace(&grub_default, "#GRUB_GFXPAYLOAD_LINUX=keep", "GRUB_GFXPAYLOAD_LINUX=\"" + settings->value("fbmode").toString().toStdString()+"\"");
	strReplace(&grub_default, "GRUB_CMDLINE_LINUX_DEFAULT=\"quiet\"", "GRUB_CMDLINE_LINUX_DEFAULT=\"quiet " + settings->value("kernel_options").toString().toStdString() + "\"");

	WriteFile("/tmp/new_sysroot/etc/default/grub", grub_default);
	grub_default.clear();

	// Generating configuration:
	if (system("chroot /tmp/new_sysroot grub-mkconfig -o /boot/grub/grub.cfg")!=0) {
		printf("Failed to generate GRUB menu via grub-mkconfig.\n");
		return false;
	}
	printf("GRUB menu successfully generated via grub-mkconfig.\n");
	return true;
}

bool SetupThread::grub2config() {
	/* Data needed:
	 systemConfig.rootMountPoint
	 bootConfig.videoModeNumber
	 systemConfig.rootPartition
	 systemConfig.otherMounts
	 systemConfig.otherMountFormat
	 bootConfig.kernelOptions
	 
	 */
	string bootDevice = settings->value("bootloader").toString().toStdString();
	// If bootDevice=="NONE", skip bootloader installation
	if (bootDevice=="NONE") return true;

	emit setSummaryText(tr("Installing GRUB2 to %1").arg(bootDevice.c_str()));
	emit setDetailsText("");
	string grubcmd = "chroot /tmp/new_sysroot grub-install --no-floppy " + bootDevice;
	system(grubcmd);
	// Get the device map
	StringMap devMap = getDeviceMap("/tmp/new_sysroot/boot/grub/device.map");
	remapHdd(devMap, bootDevice);

	string gfxPayload = settings->value("fbmode").toString().toStdString();
	if (!gfxPayload.empty()) gfxPayload = "\tset gfxpayload=\"" + gfxPayload + "\"\n";
	// Check if /boot is located on partition other than root
	string grubBootPartition = rootPartition;
	string initrdstring;
	string kernelstring = "/boot/vmlinuz";
	initrdstring = "initrd /boot/initrd.gz\n";

	string fontpath = "/boot/grub/unifont.pf2";
	string pngpath = "/boot/grub/grub640.png";

	for (size_t i=0; i<partConfigs.size(); ++i) {
		if (partConfigs[i].mountpoint=="/boot") {
			grubBootPartition = "/dev/" + partConfigs[i].partition;
			if (initrdstring.find("/boot")!=std::string::npos) initrdstring = "initrd /initrd.gz";
			if (kernelstring.find("/boot")==0) kernelstring = "/vmlinuz";
			fontpath = "/grub/unifont.pf2";
			pngpath = "/grub/grub640.png";
		}
	}
	string rootUUID = getUUID(rootPartition);
	if (rootUUID.empty()) rootUUID=rootPartition;
	else rootUUID = "UUID=" + rootUUID;

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
menuentry \"" + string(_("AgiliaLinux ") + string(DISTRO_VERSION) + _(" on ")) + rootPartition + "\" {\n\
\tset root=(" + mapPart(devMap, grubBootPartition, 0) + ")\n" + gfxPayload + \
"\tlinux " + kernelstring + " root=" + rootUUID + " ro quiet " + settings->value("kernel_options").toString().toStdString()+"\n\t" + initrdstring + "\n}\n\n";
// Add safe mode
	grubConfig += "menuentry \"" + string(_("AgiliaLinux ") + string(DISTRO_VERSION) + _(" (recovery mode) on ")) + rootPartition + "\" {\n" + gfxPayload + \
"\tlinux ("+ mapPart(devMap, grubBootPartition, 0) +")" + kernelstring + " root=" + rootPartition + " ro " + settings->value("kernel_options").toString().toStdString()+" single\n}\n\n";

	// Other OS *may* work, but no sure that it will work fine. Let's see.
	vector<OsRecord> osList = getOsList();
	if (osList.size()<1) strReplace(&grubConfig, "timeout=10", "timeout=3");
	grubConfig = grubConfig + "# Other bootable partition config begins\n";
	for (size_t i=0; i<osList.size(); i++) {
		if (osList[i].type == "other") {
			grubConfig = grubConfig + "menuentry \" " + osList[i].label + _(" on ") + osList[i].root+"\" {\n\tset root=("+mapPart(devMap, osList[i].root, 0)+")\n\tchainloader +1\n}\n\n";
		}
	}
	WriteFile("/tmp/new_sysroot/boot/grub/grub.cfg", grubConfig);

	return true;
}

void SetupThread::setDefaultRunlevel(const string& lvl) {
	// Can change runlevels 3 and 4 to lvl
	string data = ReadFile("/tmp/new_sysroot/etc/inittab");
	strReplace(&data, "id:4:initdefault", "id:" + lvl + ":initdefault");
	strReplace(&data, "id:3:initdefault", "id:" + lvl + ":initdefault");
	WriteFile("/tmp/new_sysroot/etc/inittab", data);
}
void SetupThread::enablePlymouth(bool enable) {
	if (enable) system("chroot /tmp/new_sysroot rc-update add plymouth default");
	else system("chroot /tmp/new_sysroot rc-update del plymouth plymouth");
}

void SetupThread::generateFontIndex() {
	emit setSummaryText(tr("Initializing X11 font database"));
	emit setDetailsText(tr("Generating font index"));
	if (FileExists("/tmp/new_sysroot/usr/sbin/setup_mkfontdir")) {
		system("chroot /tmp/new_sysroot /usr/sbin/setup_mkfontdir");
	}
	emit setDetailsText(tr("Generating font cache"));
	if (FileExists("/tmp/new_sysroot/usr/sbin/setup_fontconfig")) {
		system("chroot /tmp/new_sysroot /usr/sbin/setup_fontconfig");
	}
}

void setWM(string xinitrc) {
	if (!FileExists("/tmp/new_sysroot/etc/X11/xinit")) return;
	system("( cd /tmp/new_sysroot/etc/X11/xinit ; rm -f xinitrc ; ln -sf xinitrc." + xinitrc + " xinitrc )");
}

void SetupThread::setXwmConfig() {
	QString wm = settings->value("setup_variant").toString();
	if (wm=="KDE") setWM("kde");
	else if (wm=="OPENBOX") setWM("openbox");
	else if (wm=="LXDE") setWM("lxde");
	else if (wm=="XFCE") setWM("xfce");
	else if (wm=="microfluxbox" || wm=="FLUXBOX") setWM("fluxbox");
	else if (wm=="GNOME") setWM("gnome");
}

void generateLangSh(string lang, string dir="/tmp/new_sysroot/etc/profile.d/") {
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
	if (!FileExists(dir)) system("mkdir " + dir);
	strReplace(&lang_sh, "$L", lang);
	WriteFile(dir+"lang.sh", lang_sh);
	strReplace(&lang_sh, "export", "setenv");
	strReplace(&lang_sh, "/bin/sh", "/bin/csh");
	WriteFile(dir+"lang.csh", lang_sh);
}

void setConsoleKeymap(string lang) {
	if (lang.find("ru")==0 || lang.find("uk")==0) {
		string keymaps = ReadFile("/tmp/new_sysroot/etc/conf.d/keymaps");
		strReplace(&keymaps, "keymap=\"us\"", "keymap=\"ru-winkeys-uni_ct_sh\"");
		if (!FileExists("/tmp/new_sysroot/etc/conf.d")) system("mkdir /tmp/new_sysroot/etc/conf.d");
		WriteFile("/tmp/new_sysroot/etc/conf.d/keymaps", keymaps);
	}

}

bool SetupThread::postInstallActions() {
	emit setSummaryText(tr("Install complete, running post-install actions"));
	emit setDetailsText("");

	// Binding pseudo-filesystems
	
	system("mount -o bind /proc /tmp/new_sysroot/proc");
	system("mount -o bind /dev /tmp/new_sysroot/dev");
	system("mount -o bind /sys /tmp/new_sysroot/sys");

	// Locale generation
	if (settings->value("language")=="en_US.UTF-8") {
		generateLangSh("en_US.UTF-8");
		setConsoleKeymap("en");
	}
	else if (settings->value("language")=="ru_RU.UTF-8") {
		generateLangSh("ru_RU.UTF-8");
		setConsoleKeymap("ru");
	}
	else if (settings->value("language")=="uk_UA.UTF-8") {
		generateLangSh("uk_UA.UTF-8");
		setConsoleKeymap("uk");
	}

	// Do installed kernel version check
	vector<string> dirList = getDirectoryList("/tmp/new_sysroot/lib/modules");
	for (size_t i=0; i<dirList.size(); ++i) {
		if (dirList[i]!="." && dirList[i]!="..") {
			kernelversion = dirList[i];
			break;
		}
	}

	// Post-install configuration
	setRootPassword();
	createUsers();

	// Copy skel to root directory
	system("rsync -arvh /tmp/new_sysroot/etc/skel/ /tmp/new_sysroot/root/");
	system("chown -R root:root /tmp/new_sysroot/root");
	xorgSetLangConf();
	generateIssue();
	writeFstab();
	system("chroot /tmp/new_sysroot depmod -a " + kernelversion);
	buildInitrd();
	grub2_install();
	setupNetwork();
	setTimezone();
	if (FileExists("/tmp/new_sysroot/usr/bin/X")) {
		if (FileExists("/tmp/new_sysroot/usr/bin/kdm") ||
		    FileExists("/tmp/new_sysroot/usr/bin/xdm") ||
    		    FileExists("/tmp/new_sysroot/usr/sbin/gdm") ||
    		    FileExists("/tmp/new_sysroot/usr/sbin/lxdm") ||
		    FileExists("/tmp/new_sysroot/usr/bin/slim")) {
				setDefaultRunlevel("4");
				enablePlymouth(true);
		}
	}

	// If nouveau is used, remove blacklist entry from /etc/modprobe.d/nouveau.conf
	if (settings->value("nvidia-driver")=="nouveau") {
		unlink("/tmp/new_sysroot/etc/modprobe.d/nouveau.conf");
	}

	generateFontIndex();
	setXwmConfig();

	copyMPKGConfig();

	setDefaultRunlevels();
	setDefaultXDM();

	// Link /dev/cdrom and /dev/mouse
	string cdlist = get_tmp_file();
	system("getcdromlist.sh " + cdlist + " 2>/dev/null >/dev/null");
	vector<string> cdlist_v = ReadFileStrings(cdlist);
	unlink(cdlist.c_str());
	if (!cdlist_v.empty()) {
		if (!FileExists("/tmp/new_sysroot/dev/cdrom")) system("chroot /tmp/new_sysroot ln -s /dev/" + cdlist_v[0] + " /dev/cdrom");
	}
	if (!FileExists("/tmp/new_sysroot/dev/mouse")) system("chroot /tmp/new_sysroot ln -s /dev/psaux /dev/mouse"); // Everybody today has this mouse device, so I don't care about COM port users

	umountFilesystems();

	return true;
}


void SetupThread::run() {
	verbose=true;
	progressWidgetPtr = this;
	string errors;
	setupMode = true;

	settings = new QSettings("guiinstaller");
	agiliaSetup.setLocale(settings->value("language").toString().toStdString());
	rootPassword = settings->value("rootpasswd").toString();
	settings->beginGroup("users");

	QStringList usernames = settings->childKeys();
	for (int i=0; i<usernames.size(); ++i) {
		users.push_back(TagPair(usernames[i].toStdString(), settings->value(usernames[i]).toString().toStdString()));
	}
	settings->endGroup();
	settings->sync();


	emit setProgress(0);
	if (!fillPartConfigs()) return;
	emit setProgress(1);
	if (!validateConfig()) return;
	emit setProgress(5);
	if (!setMpkgConfig()) return;
	emit setProgress(10);
	if (!getRepositoryData()) return;
	emit setProgress(25);
	if (!prepareInstallQueue()) return;
	emit setProgress(50);
	if (!validateQueue()) return;
	emit setProgress(60);
	if (!formatPartitions()) return;
	emit setProgress(70);
	if (!mountPartitions()) return;
	emit setProgress(85);
	if (!moveDatabase()) return;
	emit setProgress(99);
	if (!createBaselayout()) return;

	pData.registerEventHandler(&updateProgressData);
	if (!processInstall()) return;
	if (!postInstallActions()) return;
	delete settings;
	pData.unregisterEventHandler();
	emit reportFinish();
}
bool SetupThread::createBaselayout() {
	return agiliaSetup.createBaselayout();
}

void SetupThread::setRootPassword() {
	agiliaSetup.setRootPassword(rootPassword.toStdString());
}

void SetupThread::createUsers() {
	agiliaSetup.createUsers(users);
}

void SetupThread::umountFilesystems() {
	emit setSummaryText(tr("Finishing..."));
	emit setDetailsText(tr("Unmounting filesystems and syncing disks"));
	agiliaSetup.umountFilesystems();
}

void SetupThread::setTimezone() {
	agiliaSetup.setTimezone(settings->value("time_utc").toBool(), settings->value("timezone").toString().toStdString());
}

void SetupThread::setupNetwork() {
	agiliaSetup.setupNetwork(settings->value("netman").toString().toStdString(), settings->value("hostname").toString().toStdString(), settings->value("netname").toString().toStdString());
}

void SetupThread::copyMPKGConfig() {
	agiliaSetup.copyMPKGConfig();
}

void SetupThread::setDefaultRunlevels() {
	agiliaSetup.setDefaultRunlevels();
}

void SetupThread::setDefaultXDM() {
	agiliaSetup.setDefaultXDM();
}
