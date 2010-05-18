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
void SetupThread::getCustomSetupVariants(const vector<string>& rep_list) {
	emit setSummaryText(tr("Retrieving setup variants"));
	emit setDetailsText(tr("Retrieving list..."));
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

			emit setDetailsText(tr("Receiving %1 of %2: %3").arg(i+1).arg(list.size()).arg(list[i].c_str()));
			CommonGetFile(path + "/setup_variants/" + list[i] + ".desc", "/tmp/setup_variants/" + list[i] + ".desc");
			CommonGetFile(path + "/setup_variants/" + list[i] + ".list", "/tmp/setup_variants/" + list[i] + ".list");
			customPkgSetList.push_back(getCustomPkgSet(list[i]));
		}
	}
}

void SetupThread::skipMD5() {
	forceSkipLinkMD5Checks=true;
	forceInInstallMD5Check = false;
	emit enableMD5Button(false);
}
CustomPkgSet SetupThread::getCustomPkgSet(const string& name) {
	vector<string> data = ReadFileStrings("/tmp/setup_variants/" + name + ".desc");
	CustomPkgSet ret;
	ret.name = name;
	string locale = settings->value("language").toString().toStdString();
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

bool SetupThread::validateConfig() {

	emit setSummaryText(tr("Validating config"));
	emit setDetailsText("");
	if (!settings->value("license_accepted").toBool()) {
		emit reportError(tr("License not accepted, cannot continue"));
		return false;
	}
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
	// Should set repository URLs
	vector<string> rList, dlist;

	if (settings->value("pkgsource").toString()=="dvd") {
		rList.push_back("cdrom://"+settings->value("volname").toString().toStdString()+"/"+settings->value("rep_location").toString().toStdString());
	}
	else if (settings->value("pkgsource").toString().toStdString().find("iso:///")==0) {
		system("mount -o loop " + settings->value("pkgsource").toString().toStdString().substr(6) + " /var/log/mount");
		rList.push_back("cdrom://"+settings->value("volname").toString().toStdString()+"/"+settings->value("rep_location").toString().toStdString());
	}
	else rList.push_back(settings->value("pkgsource").toString().toStdString());
	// TODO: respect options other than DVD, please
	core = new mpkg;
	core->set_repositorylist(rList, dlist);
	delete core;
	return true;
}

bool SetupThread::getRepositoryData() {
	emit setSummaryText(tr("Updating repository data"));
	emit setDetailsText(tr("Retrieving indices from repository"));
	core = new mpkg;
	if (core->update_repository_data()!=0) {
		emit reportError(tr("Failed to retreive repository data, cannot continue"));
		delete core;
		return false;
	}
	emit setDetailsText(tr("Caching setup variants"));
	getCustomSetupVariants(core->get_repositorylist());
	delete core;
	return true;
}

bool SetupThread::prepareInstallQueue() {
	emit setSummaryText(tr("Preparing install queue"));
	emit setDetailsText("");
	string installset_filename = "/tmp/setup_variants/" + settings->value("setup_variant").toString().toStdString() + ".list";

	vector<string> installset_contains;
	if (!installset_filename.empty() && FileExists(installset_filename)) {
		vector<string> versionz; // It'll be lost after, but we can't carry about it here: only one version is available.
		parseInstallList(ReadFileStrings(installset_filename), installset_contains, versionz);
		if (installset_contains.empty()) {
			reportError(tr("Package set contains no packages, installation failed."));
			return false;
		}
	}
	vector<string> errorList;
	core = new mpkg;
	core->install(installset_contains, NULL, NULL, &errorList);
	core->commit(true);
	PACKAGE_LIST commitList;
	SQLRecord sqlSearch;
	core->get_packagelist(sqlSearch, &commitList);
	vector<string> queryLog;
	for (size_t i=0; i<commitList.size(); ++i) {
		queryLog.push_back(commitList[i].get_name() + " " + commitList[i].get_fullversion() + " " + boolToStr(commitList[i].action()==ST_INSTALL));
	}
	WriteFileStrings("/var/log/comlist_before_alterswitch.log", queryLog);
	queryLog.clear();

	vector<string> alternatives;
	if (settings->value("alternatives/bfs").toBool()) alternatives.push_back("bfs");
	if (settings->value("alternatives/cleartype").toBool()) alternatives.push_back("cleartype");
	
	if (settings->value("nvidia-driver")=="190" || settings->value("nvidia-driver")=="173" || settings->value("nvidia-driver")=="96") {
		for (size_t i=0; i<commitList.size(); ++i) {
			if (commitList[i].get_name()=="nvidia-driver" || commitList[i].get_name()=="nvidia-kernel") commitList.get_package_ptr(i)->set_action(ST_INSTALL, "nvidia-select");
		}
	}
	else if (settings->value("nvidia-driver")=="173") {
		alternatives.push_back("legacy173");
	}
	else if (settings->value("nvidia-driver")=="96") {
		alternatives.push_back("legacy96");
	}
	for (size_t i=0; i<alternatives.size(); ++i) {
		printf("USED ALT: %s\n", alternatives[i].c_str());
	}
	commitList.switchAlternatives(alternatives);
	for (size_t i=0; i<commitList.size(); ++i) {
		queryLog.push_back(commitList[i].get_name() + " " + commitList[i].get_fullversion()+ " " + boolToStr(commitList[i].action()==ST_INSTALL));
	}
	WriteFileStrings("/var/log/comlist_after_alterswitch.log", queryLog);
	queryLog.clear();

	PACKAGE_LIST commitListFinal;
	for (size_t i=0; i<commitList.size(); ++i) {
		if (commitList[i].action()==ST_INSTALL) {
			commitListFinal.add(commitList[i]);
			queryLog.push_back(commitList[i].get_name() + " " + commitList[i].get_fullversion());
		}

	}
	WriteFileStrings("/var/log/final_setup_query.log", queryLog);
	core->clean_queue();
	core->install(&commitListFinal);
	core->commit(true);
	delete core;
	return true;

}

bool SetupThread::validateQueue() {
	emit setSummaryText(tr("Validating queue"));
	emit setDetailsText("");
	core = new mpkg;
	PACKAGE_LIST queue;
	SQLRecord sqlSearch;
	sqlSearch.addField("package_action", ST_INSTALL);
	core->get_packagelist(sqlSearch, &queue);
	if (queue.IsEmpty()) {
		emit reportError(tr("Commit failed: probably dependency errors"));
		return false;
	}
	return true;
}

bool SetupThread::formatPartition(PartConfig pConfig) {

	emit setDetailsText(tr("Formatting /dev/%2").arg(pConfig.partition.c_str()));
	string fs_options;
	if (pConfig.fs=="jfs") fs_options="-q";
	else if (pConfig.fs=="xfs") fs_options="-f -q";
	else if (pConfig.fs=="reiserfs") fs_options="-q";
	if (system("umount -l /dev/" + pConfig.partition +  " ; mkfs -t " + pConfig.fs + " " + fs_options + " /dev/" + pConfig.partition)==0) return true;
	else return false;
}

bool SetupThread::makeSwap(PartConfig pConfig) {
	emit setSummaryText(tr("Creating swapspace"));
	emit setDetailsText(tr("Creating swap in %1").arg(pConfig.partition.c_str()));
	system("swapoff /dev/" + pConfig.partition);
	if (system("mkswap /dev/" + pConfig.partition)==0) return false;
	return true;
}

bool SetupThread::activateSwap(PartConfig pConfig) {

	emit setSummaryText(tr("Activating swap"));
	emit setDetailsText(tr("Activating swap in %1").arg(pConfig.partition.c_str()));
	if (system("swapon /dev/" + pConfig.partition)!=0) return false;
	return true;
}

bool SetupThread::formatPartitions() {
	emit setSummaryText(tr("Formatting partitions"));
	emit setDetailsText("");
	vector<PartConfig> partConfigs;
	PartConfig *pConfig;

	settings->beginGroup("mount_options");
	QStringList partitions = settings->childGroups();
	for (int i=0; i<partitions.size(); ++i) {
		settings->beginGroup(partitions[i]);
		pConfig = new PartConfig;
		pConfig->partition = partitions[i].toStdString();
		pConfig->mountpoint = settings->value("mountpoint").toString().toStdString();
		pConfig->format = settings->value("format").toBool();
		pConfig->fs = settings->value("fs").toString().toStdString();
		settings->endGroup();
		partConfigs.push_back(*pConfig);
		if (pConfig->format) {
			formatPartition(*pConfig);
		}
		else if (pConfig->mountpoint == "swap") {
			makeSwap(*pConfig);
			activateSwap(*pConfig);
		}
		delete pConfig;
	}
	settings->endGroup();
	return true;
}

bool SetupThread::mountPartitions() {
	emit setSummaryText(tr("Mounting partitions"));
	emit setDetailsText("");

	settings->beginGroup("mount_options");
	QStringList partitions = settings->childGroups();

	vector<PartConfig> partConfigs;
	PartConfig *pConfig;
	string rootPartition;

	for (int i=0; i<partitions.size(); ++i) {
		settings->beginGroup(partitions[i]);
		pConfig = new PartConfig;
		pConfig->partition = partitions[i].toStdString();
		pConfig->mountpoint = settings->value("mountpoint").toString().toStdString();
		pConfig->format = settings->value("format").toBool();
		pConfig->fs = settings->value("fs").toString().toStdString();
		settings->endGroup();
		partConfigs.push_back(*pConfig);
		if (pConfig->mountpoint=="/") {
			rootPartition = "/dev/" + pConfig->partition;
		}
		else if (pConfig->mountpoint == "swap") continue;
		delete pConfig;
	}
	settings->endGroup();

	string mount_cmd;
	string mkdir_cmd;

	mkdir_cmd = "mkdir -p /mnt";
	mount_cmd = "mount " + rootPartition + " /mnt";
	if (system(mkdir_cmd) !=0 || system(mount_cmd)!=0) return false;

	// Sorting mount points
	vector<int> mountOrder, mountPriority;
	for (size_t i=0; i<partConfigs.size(); i++) mountPriority.push_back(0);
	for (size_t i=0; i<partConfigs.size(); i++) {
		for (size_t t=0; t<partConfigs.size(); t++) {
			if (partConfigs[i].mountpoint.find(partConfigs[t].mountpoint)==0) mountPriority[i]++;
			if (partConfigs[i].mountpoint[0]!='/') mountPriority[i]=-1; // Dunno how it can be, but hz...
		}
	}
	for (size_t i=0; i<mountPriority.size(); i++) {
		for (size_t t=0; t<mountPriority.size(); t++) {
			if (mountPriority[t]==(int) i) mountOrder.push_back(t);
		}
	}
	if (mountPriority.size()!=partConfigs.size()) {
		return false;
	}

	// Mounting others...
	
	for (size_t i=0; i<mountOrder.size(); i++) {
		if (partConfigs[mountOrder[i]].mountpoint=="/") continue;
		mkdir_cmd = "mkdir -p /mnt" + partConfigs[mountOrder[i]].mountpoint;
		if (partConfigs[mountOrder[i]].fs=="ntfs")  mount_cmd = "ntfs-3g -o force /dev/" + partConfigs[mountOrder[i]].partition + " /mnt" + partConfigs[mountOrder[i]].mountpoint;
		else mount_cmd = "mount /dev/" + partConfigs[mountOrder[i]].partition + " /mnt" + partConfigs[mountOrder[i]].mountpoint;
		if (partConfigs[mountOrder[i]].fs=="jfs") mount_cmd = "fsck /dev/" + partConfigs[mountOrder[i]].partition + "  && " + mount_cmd;

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
	if (system("rm -rf /mnt/var/mpkg ; mkdir -p /mnt/var/log ; cp -R /var/mpkg /mnt/var/ && rm -rf /var/mpkg && ln -s /mnt/var/mpkg /var/mpkg")!=0) {
		emit reportError(tr("An error occured while moving database to hard drive"));
		return false;
	}
	if (_cmdOptions["ramwarp"]=="yes") {
		system("mkdir -p /mnt/.installer && mount -t tmpfs none /mnt/.installer && mv /mnt/var/mpkg/packages.db /mnt/.installer/packages.db && ln -s /mnt/.installer/packages.db /mnt/var/mpkg/packages.db");
	}
	return true;

}

bool SetupThread::processInstall() {
	emit setSummaryText(tr("Installing packages"));
	emit setDetailsText(tr("Preparing to installation"));
	core = new mpkg;
	if (FileExists("/var/log/mount/cache")) system("ln -s /var/log/mount/cache /var/mpkg/cache/.fcache");
	if (core->commit()!=0) {
		delete core;
		return false;
	}
	emit setSummaryText(tr("Finishing installation"));
	emit setDetailsText(tr("Exporting pkgtools-compatible database"));
	core->exportBase("/mnt/var/log/packages");

	delete core;
	return true;
}
void SetupThread::xorgSetLangHALEx() {
	string lang, varstr;
	vector<MenuItem> langmenu;
	langmenu.push_back(MenuItem("us", "", "", true));
	sysconf_lang = "en_US.UTF-8";
	if (settings->value("language").toString()=="ru_RU.UTF-8") {
	       langmenu.push_back(MenuItem("ru", "winkeys", "", true));
	       sysconf_lang = "ru_RU.UTF-8";
	}
	if (settings->value("language").toString()=="uk_UA.UTF-8") {
		langmenu.push_back(MenuItem("ua", "winkeys", "", true));
		sysconf_lang = "uk_UA.UTF-8";
	}
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
	system("mkdir -p /mnt/etc/hal/fdi/policy ");
	WriteFile("/mnt/etc/hal/fdi/policy/10-x11-input.fdi", fdi);
}

void SetupThread::generateIssue() {
	if (!FileExists("/mnt/etc/issue_" + sysconf_lang)) {
		return;
	}
	system("( cd /mnt/etc ; rm issue ; ln -s issue_" + sysconf_lang + " issue )");
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
	settings->beginGroup("mount_options");
	QStringList partitions = settings->childGroups();

	vector<PartConfig> partConfigs;
	PartConfig *pConfig;

	for (int i=0; i<partitions.size(); ++i) {
		settings->beginGroup(partitions[i]);
		pConfig = new PartConfig;
		pConfig->partition = partitions[i].toStdString();
		pConfig->mountpoint = settings->value("mountpoint").toString().toStdString();
		pConfig->format = settings->value("format").toBool();
		pConfig->fs = settings->value("fs").toString().toStdString();
		settings->endGroup();
		partConfigs.push_back(*pConfig);
		if (pConfig->mountpoint=="/") {
			rootPartition = "/dev/" + pConfig->partition;
			rootPartitionType = pConfig->fs;
		}
		else if (pConfig->mountpoint == "swap") swapPartition = "/dev/" + pConfig->partition;
		delete pConfig;
	}
	settings->endGroup();

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

	data+= "# " + rootPartition + "\n" + rootUUID + "\t/\t" + rootPartitionType + "\tdefaults\t1 1\n";
	
	string options="defaults";
	string fstype="auto";

	string fsUUID;	
	// Storing data
	for (size_t i=0; i<partConfigs.size(); i++)
	{
		if (partConfigs[i].mountpoint == "/" || partConfigs[i].mountpoint == "swap") continue;
		options = "defaults";
		fstype = partConfigs[i].fs;
		if (fstype=="hfs+") fstype = "hfsplus";
		if (partConfigs[i].fs.find("fat")!=std::string::npos) {
			options="rw,codepage=866,iocharset=utf8,umask=000,showexec,quiet";
			fstype="vfat";
		}
		if (partConfigs[i].fs.find("ntfs")!=std::string::npos) {
			options="locale=ru_RU.utf8,umask=000";
			fstype="ntfs-3g";
		}
		fsUUID = getUUID("/dev/" + partConfigs[i].partition);
		if (fsUUID.empty()) fsUUID = "/dev/" + partConfigs[i].partition;
		else fsUUID="UUID=" + fsUUID;
		data += "# " + partConfigs[i].partition + "\n"+ fsUUID + "\t" + partConfigs[i].mountpoint + "\t"+fstype+"\t" + options + "\t1 1\n";
	}
	data += "\n# Required for X shared memory\nnone\t/dev/shm\ttmpfs\tdefaults\t0 0\n";
	if (settings->value("tmpfs_tmp").toBool()) data += "\n# /tmp as tmpfs\nnone\t/tmp\ttmpfs\tdefaults\t0 0\n";
	
	WriteFile("/mnt/etc/fstab", data);
}
void SetupThread::buildInitrd() {
	// RAID, LVM, LuKS encrypted volumes, USB drives and so on - all this will be supported now!
	string rootdev_wait = "0";
	if (settings->value("initrd_delay").toBool()) rootdev_wait = "10";

	system("chroot /mnt mount -t proc none /proc ");
	string rootdev;
	string rootUUID = getUUID(rootPartition);
	if (rootUUID.empty()) rootUUID=rootPartition;
	else rootUUID="UUID=" + rootUUID;
        if (rootPartitionType!="btrfs") rootdev = rootUUID;
	else rootdev = rootPartition; // Mounting by UUID doesn't work with btrfs, I don't know why.
	string use_swap, use_raid, use_lvm;
	if (!swapPartition.empty()) use_swap = "-h " + swapPartition;
	if (rootPartition.find("/dev/md")==0) use_raid = " -R ";
	if (rootPartition.find("/dev/vg")==0) use_lvm = " -L ";
	string additional_modules;
	additional_modules = settings->value("initrd_modules").toString().toStdString();
	if (!additional_modules.empty()) additional_modules = " -m " + additional_modules;

	emit setSummaryText(tr("Creating initrd"));
	emit setDetailsText("");
	system("chroot /mnt mkinitrd -c -r " + rootdev + " -f " + rootPartitionType + " -w " + rootdev_wait + " -k " + kernelversion + " " + " " + use_swap + " " + use_raid + " " + use_lvm + " " + additional_modules);
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
	emit setDetailsText(tr("Installing GRUB2 to %1").arg(bootDevice.c_str()));
	string grubcmd = "chroot /mnt grub-install --no-floppy " + bootDevice;
	system(grubcmd);
	// Get the device map
	StringMap devMap = getDeviceMap("/mnt/boot/grub/device.map");
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

	settings->beginGroup("mount_options");
	QStringList partitions = settings->childGroups();

	vector<PartConfig> partConfigs;
	PartConfig *pConfig;

	for (int i=0; i<partitions.size(); ++i) {
		settings->beginGroup(partitions[i]);
		pConfig = new PartConfig;
		pConfig->partition = partitions[i].toStdString();
		pConfig->mountpoint = settings->value("mountpoint").toString().toStdString();
		pConfig->format = settings->value("format").toBool();
		pConfig->fs = settings->value("fs").toString().toStdString();
		settings->endGroup();
		partConfigs.push_back(*pConfig);
		if (pConfig->mountpoint=="/boot") {
			grubBootPartition = "/dev/" + pConfig->partition;
			if (initrdstring.find("/boot")!=std::string::npos) initrdstring = "initrd /initrd.gz";
			if (kernelstring.find("/boot")==0) kernelstring = "/vmlinuz";
			fontpath = "/grub/unifont.pf2";
			pngpath = "/grub/grub640.png";
		}
		delete pConfig;
	}
	settings->endGroup();
	string rootUUID = getUUID(rootPartition);
	if (rootUUID.empty()) rootUUID=rootPartition;
	else rootUUID = "UUID=" + rootUUID;

	string grubConfig = "# GRUB2 configuration, generated by AgiliaLinux 10.4 setup\n\
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
menuentry \"" + string(_("AgiliaLinux 10.4 on ")) + rootPartition + "\" {\n\
\tset root=(" + mapPart(devMap, grubBootPartition, 0) + ")\n" + gfxPayload + \
"\tlinux " + kernelstring + " root=" + rootUUID + " ro " + settings->value("kernel_options").toString().toStdString()+"\n\t" + initrdstring + "\n}\n\n";
// Add safe mode
	grubConfig += "menuentry \"" + string(_("AgiliaLinux 10.4 (recovery mode) on ")) + rootPartition + "\" {\n" + gfxPayload + \
"\tlinux ("+ mapPart(devMap, grubBootPartition, 0) +")" + kernelstring + " root=" + rootPartition + " ro " + settings->value("kernel_options").toString().toStdString()+" single\n}\n\n";

	// Other OS disabled for now, i'm too lazy to implement this right now.
	/*	vector<OsRecord> osList = getOsList();
	if (osList.size()<1) strReplace(&grubConfig, "timeout=10", "timeout=3");
	grubConfig = grubConfig + "# Other bootable partition config begins\n";
	for (size_t i=0; i<osList.size(); i++) {
		if (osList[i].type == "other") {
			grubConfig = grubConfig + "menuentry \" " + osList[i].label + " on " + osList[i].root+"\" {\n\tset root=("+mapPart(devMap, osList[i].root, 0)+")\n\tchainloader +1\n}\n\n";
		}
	}*/
	WriteFile("/mnt/boot/grub/grub.cfg", grubConfig);

	return true;
}
bool SetupThread::setHostname() {
	string hostname = settings->value("hostname").toString().toStdString();
	if (hostname.empty()) return false;
	string netname = settings->value("netname").toString().toStdString();
	if (netname.empty()) netname = "example.net";
	WriteFile("/mnt/etc/HOSTNAME", hostname + "." + netname + "\n");
	string hosts = ReadFile("/mnt/etc/hosts");
	strReplace(&hosts, "darkstar", hostname);
	strReplace(&hosts, "example.net", netname);
	WriteFile("/mnt/etc/hosts", hosts);

	return true;
}
void SetupThread::setDefaultRunlevel(const string& lvl) {
	// Can change runlevels 3 and 4 to lvl
	string data = ReadFile("/mnt/etc/inittab");
	strReplace(&data, "id:4:initdefault", "id:" + lvl + ":initdefault");
	strReplace(&data, "id:3:initdefault", "id:" + lvl + ":initdefault");
	WriteFile("/mnt/etc/inittab", data);
}
void SetupThread::enablePlymouth(bool enable) {
	if (enable) system("chroot /mnt rc-update add plymouth default");
	else system("chroot /mnt rc-update del plymouth plymouth");
}

void SetupThread::generateFontIndex() {
	emit setDetailsText(tr("Generating font index"));
	if (FileExists("/mnt/usr/sbin/setup_mkfontdir")) {
		system("chroot /mnt /usr/sbin/setup_mkfontdir");
	}
	emit setDetailsText(tr("Generating font cache"));
	if (FileExists("/mnt/usr/sbin/setup_fontconfig")) {
		system("chroot /mnt /usr/sbin/setup_fontconfig");
	}
}

void setWM(string xinitrc) {
	if (!FileExists("/mnt/etc/X11/xinit")) return;
	system("( cd /mnt/etc/X11/xinit ; rm -f xinitrc ; ln -sf xinitrc." + xinitrc + " xinitrc )");
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

bool SetupThread::postInstallActions() {
	emit setSummaryText(tr("Install complete, running post-install actions"));
	emit setDetailsText("");

	if (settings->value("language")=="en_US.UTF-8") generateLangSh("en_US.UTF-8");
	else if (settings->value("language")=="ru_RU.UTF-8") generateLangSh("ru_RU.UTF-8");
	else if (settings->value("language")=="uk_UA.UTF-8") generateLangSh("uk_UA.UTF-8");

	// Do installed kernel version check
	vector<string> dirList = getDirectoryList("/mnt/lib/modules");
	for (size_t i=0; i<dirList.size(); ++i) {
		if (dirList[i]!="." && dirList[i]!="..") {
			kernelversion = dirList[i];
			break;
		}
	}
	if (_cmdOptions["ramwarp"]=="yes") {
		system("rm /mnt/var/mpkg/packages.db ; mv /mnt/.installer/packages.db /mnt/var/mpkg/packages.db ; umount /mnt/.installer ; rmdir /mnt/.installer");
	}

	// Post-install configuration
	setRootPassword();
	createUsers();

	xorgSetLangHALEx();
	generateIssue();
	writeFstab();
	system("chroot /mnt depmod -a " + kernelversion);
	buildInitrd();
	grub2config();
	setupNetwork();
	setTimezone();
	if (FileExists("/mnt/usr/bin/X")) {
		if (FileExists("/mnt/usr/bin/kdm") ||
		    FileExists("/mnt/usr/bin/xdm") ||
    		    FileExists("/mnt/usr/sbin/gdm") ||
		    FileExists("/mnt/usr/bin/slim")) {
				setDefaultRunlevel("4");
				enablePlymouth(true);
		}
	}

	generateFontIndex();
	setXwmConfig();

	copyMPKGConfig();

	setDefaultRunlevels();
	setDefaultXDM();

	umountFilesystems();

	return true;
}


void SetupThread::run() {
	progressWidgetPtr = this;
	pData.registerEventHandler(&updateProgressData);
	string errors;
	setupMode = true;
	settings = new QSettings("guiinstaller");
	rootPassword = settings->value("rootpasswd").toString();
	settings->setValue("rootpasswd", "");
	settings->beginGroup("users");

	QStringList usernames = settings->childKeys();
	for (int i=0; i<usernames.size(); ++i) {
		users.push_back(TagPair(usernames[i].toStdString(), settings->value(usernames[i]).toString().toStdString()));
		settings->setValue(usernames[i], "");
	}
	settings->endGroup();
	settings->sync();


	emit setProgress(0);
	system("rm -rf /var/mpkg && mkdir -p /var/mpkg && cp /packages.db /var/mpkg/"); // BE AWARE OF RUNNING THIS ON REAL SYSTEM!!!
	if (!validateConfig()) return;
	if (!setMpkgConfig()) return;
	if (!getRepositoryData()) return;
	if (!prepareInstallQueue()) return;
	if (!validateQueue()) return;
	if (!formatPartitions()) return;
	if (!mountPartitions()) return;
	if (!moveDatabase()) return;
	if (!processInstall()) return;
	if (!postInstallActions()) return;
	delete settings;
	emit reportFinish();
}

bool setPasswd(string username, string passwd) {
	string tmp_file = "/mnt/tmp/wtf";
	string data = passwd + "\n" + passwd + "\n";
	WriteFile(tmp_file, data);
	string passwd_cmd = "#!/bin/sh\ncat /tmp/wtf | passwd " + username+" \n";
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
	system("chroot /mnt /usr/sbin/useradd -d /home/" + username + " -m -g users -G " + extgroup + " -s /bin/bash " + username);
	system("chroot /mnt chown -R " + username+":users /home/"+username);
	system("chmod 711 /mnt/home/" + username);
	return true;
}


void SetupThread::setRootPassword() {
	setPasswd("root", rootPassword.toStdString());
}

void SetupThread::createUsers() {
	for (size_t i=0; i<users.size(); ++i) {
		addUser(users[i].tag);
		setPasswd(users[i].tag, users[i].value);
	}
}

void SetupThread::umountFilesystems() {
	emit setDetailsText(tr("Unmounting filesystems and syncing disks"));
	system("chroot /mnt umount /proc");
	system("chroot /mnt umount /sys");
	system("chroot /mnt umount -a");
	system("sync");

}

void SetupThread::setTimezone() {
	if (settings->value("time_utc").toBool()) {
		WriteFile("/mnt/etc/hardwareclock", "# Tells how the hardware clock time is stored\n#\nUTC\n");
		WriteFile("/mnt/etc/conf.d/hwclock", "# Set clock to \"UTC\" if your hardware clock stores time in GMT, or \"local\" if your clock stores local time.\n\nclock=\"UTC\"\n#If you want to sync hardware clock with your system clock at shutdown, set clock_synctohc to YES.\nclock_synctohc=\"YES\"\n\n# You can specify special arguments to hwclock during bootup\nclock_args=\"\"\n");
	}
	else {
		WriteFile("/mnt/etc/hardwareclock", "# Tells how the hardware clock time is stored\n#\nlocaltime\n");
		WriteFile("/mnt/etc/conf.d/hwclock", "# Set clock to \"UTC\" if your hardware clock stores time in GMT, or \"local\" if your clock stores local time.\n\nclock=\"local\"\n#If you want to sync hardware clock with your system clock at shutdown, set clock_synctohc to YES.\nclock_synctohc=\"YES\"\n\n# You can specify special arguments to hwclock during bootup\nclock_args=\"\"\n");
	}

	if (!settings->value("timezone").toString().isEmpty()) {
		system("( cd /mnt/etc ; ln -sf /usr/share/zoneinfo/" + settings->value("timezone").toString().toStdString() + " localtime-copied-from )");
		unlink("/mnt/etc/localtime");
		system("chroot /mnt cp etc/localtime-copied-from etc/localtime");
	}
}

void SetupThread::setupNetwork() {
	setHostname();
	if (settings->value("netman").toString()=="wicd") {
		system("chmod +x /etc/rc.d/rc.wicd");	
		system("chroot /mnt rc-update add wicd default");
	}
	else if (settings->value("netman").toString()=="networkmanager") {
		system("chmod +x /etc/rc.d/rc.networkmanager");	
		system("chroot /mnt rc-update add networkmanager default");
	}

}

void SetupThread::copyMPKGConfig() {
	mpkg *core = new mpkg(true);
	system("cp /etc/mpkg.xml /etc/mpkg.xml.backup");
	core->set_sysroot("/");
	
	vector<string> repList;
#ifdef X86_64
	repList.push_back("http://core64.agilialinux.org/");
	repList.push_back("http://userland64.agilialinux.org/");
#else
	repList.push_back("http://core32.agilialinux.org/");
	repList.push_back("http://userland32.agilialinux.org/");
#endif

	vector<string> disabled_rep_list = core->get_repositorylist();
	mpkgconfig::set_repositorylist(repList, disabled_rep_list);
	system("cp /etc/mpkg.xml /mnt/etc/");
	system("cp /etc/mpkg.xml.backup /etc/mpkg.xml");
	delete core;
}

void SetupThread::setDefaultRunlevels() {
// We don't know which of them are in system in real, but let's try them all
	system("chroot /mnt rc-update add sysfs sysinit");
	system("chroot /mnt rc-update add udev sysinit");
	system("chroot /mnt rc-update add consolefont default");
	system("chroot /mnt rc-update add hald default");
	system("chroot /mnt rc-update add sysklogd default");
	system("chroot /mnt rc-update add dbus default");
	system("chroot /mnt rc-update add sshd default");
	system("chroot /mnt rc-update add alsasound default");
	system("chroot /mnt rc-update add acpid default");
	system("chroot /mnt rc-update add cups default");

	
}

void SetupThread::setDefaultXDM() {
	if (FileExists("/mnt/etc/rc.d/init.d/kdm")) system("chroot /mnt rc-update add kdm X11");
	else if (FileExists("/mnt/etc/rc.d/init.d/gdm")) system("chroot /mnt rc-update add gdm X11");
	else if (FileExists("/mnt/etc/rc.d/init.d/xdm")) system("chroot /mnt rc-update add xdm X11");
}
