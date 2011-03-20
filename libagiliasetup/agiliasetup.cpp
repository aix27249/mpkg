#include "agiliasetup.h"

AgiliaSetup::AgiliaSetup(StatusNotifier *_n) {
	notifier = _n;
}

AgiliaSetup::~AgiliaSetup() {
}

void AgiliaSetup::registerStatusNotifier(StatusNotifier *_n) {
	notifier = _n;
}

void AgiliaSetup::unregisterStatusNotifier() {
	notifier = NULL;
	
}

void AgiliaSetup::setDefaultXDM() {
	if (FileExists("/tmp/new_sysroot/etc/init.d/kdm")) system("chroot /tmp/new_sysroot rc-update add kdm X11 2>&1 >/dev/null");
	else if (FileExists("/tmp/new_sysroot/etc/init.d/gdm")) system("chroot /tmp/new_sysroot rc-update add gdm X11 2>&1 >/dev/null");
	else if (FileExists("/tmp/new_sysroot/etc/init.d/lxdm")) system("chroot /tmp/new_sysroot rc-update add lxdm X11 2>&1 >/dev/null");
	else if (FileExists("/tmp/new_sysroot/etc/init.d/slim")) system("chroot /tmp/new_sysroot rc-update add slim X11 2>&1 >/dev/null");
	else if (FileExists("/tmp/new_sysroot/etc/init.d/xdm")) system("chroot /tmp/new_sysroot rc-update add xdm X11 2>&1 >/dev/null");

}

void AgiliaSetup::setDefaultRunlevels() {
	system("chroot /tmp/new_sysroot rc-update add mdadm boot 2>&1 >/dev/null");
	system("chroot /tmp/new_sysroot rc-update add lvm boot 2>&1 >/dev/null");
	system("chroot /tmp/new_sysroot rc-update add sysfs sysinit 2>&1 >/dev/null");
	system("chroot /tmp/new_sysroot rc-update add udev sysinit 2>&1 >/dev/null");
	system("chroot /tmp/new_sysroot rc-update add consolefont default 2>&1 >/dev/null");
	system("chroot /tmp/new_sysroot rc-update add hald default 2>&1 >/dev/null");
	system("chroot /tmp/new_sysroot rc-update add sysklogd default 2>&1 >/dev/null");
	system("chroot /tmp/new_sysroot rc-update add dbus default 2>&1 >/dev/null");
	system("chroot /tmp/new_sysroot rc-update add sshd default 2>&1 >/dev/null");
	system("chroot /tmp/new_sysroot rc-update add alsasound default 2>&1 >/dev/null");
	system("chroot /tmp/new_sysroot rc-update add acpid default 2>&1 >/dev/null");
	system("chroot /tmp/new_sysroot rc-update add cupsd default 2>&1 >/dev/null");
	system("chroot /tmp/new_sysroot rc-update add cron default 2>&1 >/dev/null");

}


void AgiliaSetup::copyMPKGConfig() {
#ifdef X86_64
	system("cp /usr/share/setup/mpkg-x86_64.xml /tmp/new_sysroot/etc/mpkg.xml");
#else
	system("cp /usr/share/setup/mpkg-x86.xml /tmp/new_sysroot/etc/mpkg.xml");
#endif
	system("mv /tmp/packages.db /tmp/new_sysroot/var/mpkg/packages.db");
}

bool AgiliaSetup::setHostname(const string& hostname, const string& netname) {
	if (hostname.empty()) return false;
	WriteFile("/tmp/new_sysroot/etc/HOSTNAME", hostname + "." + netname + "\n");
	string hosts = ReadFile("/tmp/new_sysroot/etc/hosts");
	strReplace(&hosts, "darkstar", hostname);
	strReplace(&hosts, "example.net", netname);
	WriteFile("/tmp/new_sysroot/etc/hosts", hosts);

	return true;
}


void AgiliaSetup::setupNetwork(const string& netman, const string& hostname, const string& netname) {
	setHostname(hostname, netname);
	if (netman=="wicd") {
		system("chroot /tmp/new_sysroot rc-update add wicd default 2>&1 >/dev/null");
	}
	else if (netman=="networkmanager") {
		system("chroot /tmp/new_sysroot rc-update add networkmanager default 2>&1 >/dev/null");
	}
	else if (netman=="netconfig") {
		system("chroot /tmp/new_sysroot rc-update add network default 2>&1 >/dev/null");
	}

}

void AgiliaSetup::setTimezone(bool time_utc, const string& timezone) {
	if (time_utc) {
		WriteFile("/tmp/new_sysroot/etc/hardwareclock", "# Tells how the hardware clock time is stored\n#\nUTC\n");
		WriteFile("/tmp/new_sysroot/etc/conf.d/hwclock", "# Set clock to \"UTC\" if your hardware clock stores time in GMT, or \"local\" if your clock stores local time.\n\nclock=\"UTC\"\n#If you want to sync hardware clock with your system clock at shutdown, set clock_synctohc to YES.\nclock_synctohc=\"YES\"\n\n# You can specify special arguments to hwclock during bootup\nclock_args=\"\"\n");
	}
	else {
		WriteFile("/tmp/new_sysroot/etc/hardwareclock", "# Tells how the hardware clock time is stored\n#\nlocaltime\n");
		WriteFile("/tmp/new_sysroot/etc/conf.d/hwclock", "# Set clock to \"UTC\" if your hardware clock stores time in GMT, or \"local\" if your clock stores local time.\n\nclock=\"local\"\n#If you want to sync hardware clock with your system clock at shutdown, set clock_synctohc to YES.\nclock_synctohc=\"YES\"\n\n# You can specify special arguments to hwclock during bootup\nclock_args=\"\"\n");
	}

	if (!timezone.empty()) {
		system("( cd /tmp/new_sysroot/etc ; ln -sf /usr/share/zoneinfo/" + timezone + " localtime-copied-from )");
		unlink("/tmp/new_sysroot/etc/localtime");
		system("chroot /tmp/new_sysroot cp etc/localtime-copied-from etc/localtime");
	}
}

void AgiliaSetup::umountFilesystems() {
	if (notifier) {
		notifier->setSummaryTextCall(_("Finishing..."));
		notifier->setDetailsTextCall(_("Unmounting filesystems and syncing disks"));
	}
	system("chroot /tmp/new_sysroot umount /proc 2>&1 >/dev/null");
	system("chroot /tmp/new_sysroot umount /sys 2>&1 >/dev/null");
	system("chroot /tmp/new_sysroot umount -a 2>&1 >/dev/null");
	system("sync 2>&1 >/dev/null");

}

bool AgiliaSetup::createUsers(const vector<TagPair> &users) {
	bool ret = true;
	for (size_t i=0; i<users.size(); ++i) {
		if (addUser(users[i].tag)) setPasswd(users[i].tag, users[i].value);
		else ret = false; // Mark failure, but continue for other users
	}
	return ret;
}

bool AgiliaSetup::setRootPassword(const string& rootPassword) {
	return setPasswd("root", rootPassword);
}

bool AgiliaSetup::addUser(const string &username) {
	string extgroup="audio,cdrom,floppy,video,netdev,plugdev,power"; // New default groups, which conforms current guidelines
	int ret = system("chroot /tmp/new_sysroot /usr/sbin/useradd -d /home/" + username + " -m -g users -G " + extgroup + " -s /bin/bash " + username);
	if (ret) return false; // Fail

	system("chroot /tmp/new_sysroot chown -R " + username+":users /home/"+username);
	system("chmod 700 /tmp/new_sysroot/home/" + username);
	return true;
}

bool AgiliaSetup::setPasswd(const string& username, const string& passwd) {
	string tmp_file = "/tmp/new_sysroot/tmp/wtf";
	string data = passwd + "\n" + passwd + "\n";
	WriteFile(tmp_file, data);
	string passwd_cmd = "#!/bin/sh\ncat /tmp/wtf | passwd " + username+" \n";
	WriteFile("/tmp/new_sysroot/tmp/run_passwd", passwd_cmd);
	int ret = system("chroot /tmp/new_sysroot sh /tmp/run_passwd");
	for (size_t i=0; i<data.size(); i++) {
		data[i]=' ';
	}
	WriteFile(tmp_file, data);
	unlink(tmp_file.c_str());
	unlink("/tmp/new_sysroot/tmp/run_passwd");
	if (ret == 0) return true;
	return false;
}


bool AgiliaSetup::createBaselayout() {
	system("mkdir -p /tmp/new_sysroot/{dev,etc,home,media,mnt,proc,root,sys,tmp}");
	system("chmod 710 /root");
	system("chmod 1777 /tmp");
	
	// Some programs except generic directories in some very weird places.
	// For example, KDE assumes that /var/tmp is a symlink to /tmp. I'm too lazy to patch this out, so I'll just create this symlink.
	system("ln -sf ../tmp /tmp/new_sysroot/var/tmp");
	return true;
}

void AgiliaSetup::getCustomSetupVariants(const vector<string>& rep_list) {
	if (notifier) {
		notifier->setSummaryTextCall(_("Retrieving setup variants"));
		notifier->setDetailsTextCall(_("Retrieving list..."));
	}

	string tmpfile = get_tmp_file();
	system("rm -rf /tmp/setup_variants");
	system("mkdir -p /tmp/setup_variants");
	string path;
	customPkgSetList.clear();

	DownloadsList downloadQueue;
	DownloadItem tmpDownloadItem;
	vector<string> itemLocations;
	tmpDownloadItem.priority = 0;
	tmpDownloadItem.status = DL_STATUS_WAIT;
	string itemname;

	for (size_t z=0; z<rep_list.size(); ++z) {
		downloadQueue.clear();
		path = rep_list[z];
		CommonGetFile(path + "/setup_variants.list", tmpfile);
		vector<string> list = ReadFileStrings(tmpfile);
		printf("Received %d setup variants\n", (int) list.size());
		vector<CustomPkgSet> ret;
		for (size_t i=0; i<list.size(); ++i) {
			itemLocations.clear();
			tmpDownloadItem.name=list[i];
			tmpDownloadItem.file="/tmp/setup_variants/" + list[i] + ".desc";
			itemLocations.push_back(path + "/setup_variants/" + list[i] + ".desc");
			tmpDownloadItem.url_list = itemLocations;
			downloadQueue.push_back(tmpDownloadItem);

			itemLocations.clear();
			tmpDownloadItem.name=list[i];
			tmpDownloadItem.file="/tmp/setup_variants/" + list[i] + ".list";
			itemLocations.push_back(path + "/setup_variants/" + list[i] + ".list");
			tmpDownloadItem.url_list = itemLocations;
			downloadQueue.push_back(tmpDownloadItem);
		}
		CommonGetFileEx(downloadQueue, &itemname);

		for (size_t i=0; i<list.size(); ++i) {
			if (notifier) notifier->setDetailsTextCall(_("Importing ") + IntToStr(i+1) + _(" of ") + IntToStr(list.size()) + ": " + list[i]);
			customPkgSetList.push_back(getCustomPkgSet(list[i]));
		}
	}
}

CustomPkgSet AgiliaSetup::getCustomPkgSet(const string& name) {
	vector<string> data = ReadFileStrings("/tmp/setup_variants/" + name + ".desc");
	CustomPkgSet ret;
	ret.name = name;
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


void AgiliaSetup::setLocale(const string& lang) {
	locale = lang;
}

void AgiliaSetup::setMpkgConfig(string pkgsource, const string& volname, const string& rep_location, const vector<string> additional_repositories) {
	if (notifier) notifier->setSummaryTextCall(_("Setting MPKG config"));
	if (notifier) notifier->setDetailsTextCall("");

	vector<string> rList, dlist;

	// First, primary repository
	if (pkgsource=="dvd") {
		if (FileExists("/bootmedia/.volume_id")) {
		       rList.push_back("file:///bootmedia/repository/");
		       pkgsource = "file:///bootmedia/repository/";
		}
		else rList.push_back("cdrom://" + volname + "/" + rep_location);
	}
	else if (pkgsource.find("iso:///")==0) {
		system("mount -o loop " + pkgsource.substr(6) + " /var/log/mount");
		rList.push_back("cdrom://" + volname + "/" + rep_location);
	}
	else rList.push_back(pkgsource);

	// Now, alternate package sources: for updates and so on
	for (size_t i=0; i<additional_repositories.size(); ++i) {
		rList.push_back(additional_repositories[i]);
	}

	mpkg *core = new mpkg;
	core->set_repositorylist(rList, dlist);
	delete core;
}

bool AgiliaSetup::getRepositoryData() {
	if (notifier) notifier->setSummaryTextCall(_("Updating repository data"));
	if (notifier) notifier->setDetailsTextCall(_("Retrieving indices from repository"));

	mpkg *core = new mpkg;
	if (core->update_repository_data()!=0) {
		if (notifier) notifier->sendReportError(_("Failed to retreive repository data, cannot continue"));
		delete core;
		return false;
	}
	if (notifier) notifier->setDetailsTextCall(_("Caching setup variants"));
	getCustomSetupVariants(core->get_repositorylist());
	delete core;
	return true;
}



bool AgiliaSetup::prepareInstallQueue(const string& setup_variant, const string& netman, const string& nvidia_driver) {
	if (notifier) notifier->setSummaryTextCall(_("Preparing install queue"));
	if (notifier) notifier->setDetailsTextCall("");

	string installset_filename = "/tmp/setup_variants/" + setup_variant + ".list";

	vector<string> installset_contains;
	if (!installset_filename.empty() && FileExists(installset_filename)) {
		vector<string> versionz; // It'll be lost after, but we can't carry about it here: only one version is available.
		vector<string> pkgListStrings = ReadFileStrings(installset_filename);
		if (netman=="networkmanager") pkgListStrings.push_back("NetworkManager");
		else if (netman=="wicd") pkgListStrings.push_back("wicd");
		parseInstallList(pkgListStrings, installset_contains, versionz);
		pkgListStrings.clear();
		if (installset_contains.empty()) {
			if (notifier) notifier->sendReportError(_("Package set contains no packages, installation failed."));
			return false;
		}
	}
	vector<string> errorList;
	mpkg *core = new mpkg;
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
	// This block is disabled, since it is not used anymore. But I leave it commented out rather than deleted: maybe, we get back this feature.
/*	if (settings->value("alternatives/bfs").toBool()) alternatives.push_back("bfs");
	if (settings->value("alternatives/cleartype").toBool()) alternatives.push_back("cleartype"); */
	
	if (nvidia_driver=="latest" || nvidia_driver=="173" || nvidia_driver=="96") {
		alternatives.push_back("nvidia");
		for (size_t i=0; i<commitList.size(); ++i) {
			if (commitList[i].get_name()=="nvidia-driver" || commitList[i].get_name()=="nvidia-kernel") commitList.get_package_ptr(i)->set_action(ST_INSTALL, "nvidia-select");
		}
	}
	if (nvidia_driver=="173") {
		alternatives.push_back("legacy173");
	}
	if (nvidia_driver=="96") {
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

bool AgiliaSetup::validateQueue() {
	if (notifier) notifier->setSummaryTextCall(_("Validating queue"));
	if (notifier) notifier->setDetailsTextCall("");

	mpkg *core = new mpkg;
	PACKAGE_LIST queue;
	SQLRecord sqlSearch;
	sqlSearch.addField("package_action", ST_INSTALL);
	core->get_packagelist(sqlSearch, &queue);
	delete core;
	if (queue.IsEmpty()) {
		if (notifier) notifier->sendReportError(_("Commit failed: probably dependency errors"));
		return false;
	}
	return true;
}

bool AgiliaSetup::formatPartition(PartConfig pConfig) {
	if (notifier) notifier->setDetailsTextCall(_("Formatting ") + pConfig.partition);
	printf("Formatting %s\n", pConfig.partition.c_str());
	string fs_options;
	if (pConfig.fs=="jfs") fs_options="-q";
	else if (pConfig.fs=="xfs") fs_options="-f -q";
	else if (pConfig.fs=="reiserfs") fs_options="-q";
	if (system("umount -l " + pConfig.partition +  " ; mkfs -t " + pConfig.fs + " " + fs_options + " " + pConfig.partition)==0) return true;
	else return false;
}

bool AgiliaSetup::makeSwap(PartConfig pConfig) {
	if (notifier) notifier->setDetailsTextCall(_("Creating swap in ") + pConfig.partition);
	system("swapoff " + pConfig.partition);
	if (system("mkswap " + pConfig.partition)==0) return false;
	return true;
}

bool AgiliaSetup::activateSwap(PartConfig pConfig) {
	if (system("swapon " + pConfig.partition)!=0) return false;
	return true;
}


bool AgiliaSetup::formatPartitions() {
	if (notifier) notifier->setSummaryTextCall(_("Formatting partitions"));
	if (notifier) notifier->setDetailsTextCall("");

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

bool AgiliaSetup::mountPartitions() {
	if (notifier) notifier->setSummaryTextCall(_("Mounting partitions"));
	if (notifier) notifier->setDetailsTextCall("");

	for (size_t i=0; i<partConfigs.size(); ++i) {
		if (partConfigs[i].mountpoint=="/") {
			rootPartition = partConfigs[i].partition;
			rootPartitionType = partConfigs[i].fs;
			rootPartitionMountOptions=partConfigs[i].mount_options;
		}
		else if (partConfigs[i].mountpoint == "swap") swapPartition = partConfigs[i].partition;
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
			mount_cmd = "ntfs-3g " + mount_options + " " + partConfigs[mountOrder[i]].partition + " /tmp/new_sysroot" + partConfigs[mountOrder[i]].mountpoint;
		}
		else mount_cmd = "mount " + mount_options + " " + partConfigs[mountOrder[i]].partition + " /tmp/new_sysroot" + partConfigs[mountOrder[i]].mountpoint;
		if (partConfigs[mountOrder[i]].fs=="jfs") mount_cmd = "fsck " + partConfigs[mountOrder[i]].partition + "  && " + mount_cmd;

		printf("Mounting partition: %s\n", mount_cmd.c_str());
		if (system(mkdir_cmd)!=0 || system(mount_cmd)!=0) {
			if (notifier) notifier->sendReportError(_("Failed to mount partition ") + partConfigs[mountOrder[i]].partition);
			return false;
		}
	}
	return true;

}

void AgiliaSetup::setPartConfigs(const vector<PartConfig> &_partConfigs) {
	partConfigs = _partConfigs;
}
bool AgiliaSetup::moveDatabase() {
	if (notifier) notifier->setSummaryTextCall(_("Moving database"));
	if (notifier) notifier->setDetailsTextCall("");

	system("mkdir -p /tmp/new_sysroot/var/mpkg/cache");
	system("mkdir -p /tmp/new_sysroot/var/log");
	return true;

}

bool AgiliaSetup::processInstall(const string &pkgsource) {
	if (notifier) notifier->setSummaryTextCall(_("Installing packages"));
	if (notifier) notifier->setDetailsTextCall(_("Preparing to installation"));

	mpkg *core = new mpkg;
	if ((pkgsource=="dvd" || pkgsource.find("iso://")==0) && FileExists("/var/log/mount/cache")) {
		system("ln -sf /var/log/mount/cache /tmp/new_sysroot/var/mpkg/cache/.fcache");
	}
	else if (pkgsource=="file:///bootmedia/repository/" && FileExists("/bootmedia/cache")) {
		system("ln -sf /bootmedia/cache /tmp/new_sysroot/var/mpkg/cache/.fcache");
	}
	printf("Committing\n");
	if (core->commit()!=0) {
		delete core;
		printf("Commit failed");
		return false;
	}
	printf("\n\n\nCommit OK, going to post-install\n\n\n\n");
	if (notifier) notifier->setSummaryTextCall(_("Finishing installation"));

	delete core;
	return true;
}

void AgiliaSetup::xorgSetLangConf(const string& language) {
	string lang = "us";
	sysconf_lang = "en_US.UTF-8";
	if (language=="ru_RU.UTF-8") {
	       lang +=",ru";
	       sysconf_lang = "ru_RU.UTF-8";
	}
	if (language=="uk_UA.UTF-8") {
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

void AgiliaSetup::generateIssue() {
	if (!FileExists("/tmp/new_sysroot/etc/issue_" + sysconf_lang)) {
		return;
	}
	system("( cd /tmp/new_sysroot/etc ; rm issue ; ln -s issue_" + sysconf_lang + " issue )");
}

string AgiliaSetup::getUUID(const string& dev) {
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

void AgiliaSetup::writeFstab(bool tmpfs_tmp) {

	for (size_t i=0; i<partConfigs.size(); ++i) {
		if (partConfigs[i].mountpoint == "swap") swapPartition = partConfigs[i].partition;
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
		fsUUID = getUUID(partConfigs[i].partition);
		if (fsUUID.empty()) fsUUID = partConfigs[i].partition;
		else fsUUID="UUID=" + fsUUID;
		data += "# " + partConfigs[i].partition + "\n"+ fsUUID + "\t" + partConfigs[i].mountpoint + "\t"+fstype+"\t" + options + "\t1 1\n";
	}
	data += "\n# Required for X shared memory\nnone\t/dev/shm\ttmpfs\tdefaults\t0 0\n";
	if (tmpfs_tmp) data += "\n# /tmp as tmpfs\nnone\t/tmp\ttmpfs\tdefaults\t0 0\n";
	
	WriteFile("/tmp/new_sysroot/etc/fstab", data);
}
void AgiliaSetup::buildInitrd(bool initrd_delay, const string& initrd_modules) {
	// RAID, LVM, LuKS encrypted volumes, USB drives and so on - all this will be supported now!
	string rootdev_wait = "0";
	if (initrd_delay) rootdev_wait = "10";

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
	additional_modules = initrd_modules;
	if (!additional_modules.empty()) additional_modules = " -m " + additional_modules;

	if (notifier) notifier->setSummaryTextCall(_("Creating initrd"));
	int ret = system("chroot /tmp/new_sysroot mkinitrd -c -r " + rootdev + " -f " + rootPartitionType + " -w " + rootdev_wait + " -k " + kernelversion + " " + " " + use_swap + " " + use_raid + " " + use_lvm + " " + additional_modules);
	// In case if first run fails (happened only twice, but happened)
	if (ret) system("chroot /tmp/new_sysroot mkinitrd");
}

StringMap AgiliaSetup::getDeviceMap(const string& mapFile) {
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

void AgiliaSetup::remapHdd(StringMap& devMap, const string& root_mbr) {
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

string AgiliaSetup::getGfxPayload(const string& vgaMode) {
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

string AgiliaSetup::mapPart(const StringMap& devMap, const string& partName, int isGrubLegacy) {
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
vector<OsRecord> AgiliaSetup::getOsList() {
	vector<OsRecord> ret;
	// This is a stub
	return ret;
}

bool AgiliaSetup::grub2_install(const string& bootloader, const string& fbmode, const string& kernel_options) {
	string bootDevice = bootloader;
	if (bootDevice=="NONE") return true;
	if (notifier) {
		notifier->setSummaryTextCall(_("Installing GRUB2 to ") + bootDevice);
		notifier->setDetailsTextCall("");
	}

	// Installing GRUB into device
	int ret = system("chroot /tmp/new_sysroot grub-install --no-floppy --force " + bootDevice);
	if (ret!=0) {
		if (notifier) {
			notifier->setSummaryTextCall(_("Failed to install GRUB2 to ") + bootDevice);
			notifier->setDetailsTextCall(_("grub-install returned ") + IntToStr(ret) + _(", see log for details"));
		}

		printf("FATAL: Failed to install grub via grub-install!\n");
		return false;
	}
	else {
		if (notifier) {
			notifier->setDetailsTextCall(_("Generating GRUB2 menu"));
		}

		printf("GRUB2 installed successfully, generating config\n");
		bool c_ret = grub2_mkconfig(bootloader, fbmode, kernel_options);
		if (!c_ret) c_ret = grub2config(bootloader, fbmode, kernel_options);
		if (!c_ret) {
			if (notifier) {
				notifier->setSummaryTextCall(_("Failed to generate GRUB2 configuration, expecting boot failure."));
				notifier->setDetailsTextCall(_("You have to create config manually."));
			}

			printf("FATAL: Failed to create grub.cfg using both methods!\n");
			return false;
		}
		return true;
	}
}

bool AgiliaSetup::grub2_mkconfig(const string& bootloader, const string& fbmode, const string& kernel_options) {
	string bootDevice = bootloader;
	if (bootDevice=="NONE") return true;

	// Fixing /etc/default/grub
	string grub_default = ReadFile("/tmp/new_sysroot/etc/default/grub");
	strReplace(&grub_default, "#GRUB_GFXPAYLOAD_LINUX=keep", "GRUB_GFXPAYLOAD_LINUX=\"" + fbmode + "\"");
	strReplace(&grub_default, "GRUB_CMDLINE_LINUX_DEFAULT=\"quiet\"", "GRUB_CMDLINE_LINUX_DEFAULT=\"quiet " + kernel_options + "\"");

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

bool AgiliaSetup::grub2config(const string& bootloader, const string& fbmode, const string& kernel_options) {
	/* Data needed:
	 systemConfig.rootMountPoint
	 bootConfig.videoModeNumber
	 systemConfig.rootPartition
	 systemConfig.otherMounts
	 systemConfig.otherMountFormat
	 bootConfig.kernelOptions
	 
	 */
	string bootDevice = bootloader;
	// If bootDevice=="NONE", skip bootloader installation
	if (bootDevice=="NONE") return true;

	if (notifier) {
	       	notifier->setSummaryTextCall(_("Installing GRUB2 to ") + bootDevice);
		notifier->setDetailsTextCall("");
	}
	string grubcmd = "chroot /tmp/new_sysroot grub-install --no-floppy " + bootDevice;
	system(grubcmd);
	// Get the device map
	StringMap devMap = getDeviceMap("/tmp/new_sysroot/boot/grub/device.map");
	remapHdd(devMap, bootDevice);

	string gfxPayload = fbmode;
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
			grubBootPartition = partConfigs[i].partition;
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
"\tlinux " + kernelstring + " root=" + rootUUID + " ro quiet " + kernel_options+"\n\t" + initrdstring + "\n}\n\n";
// Add safe mode
	grubConfig += "menuentry \"" + string(_("AgiliaLinux ") + string(DISTRO_VERSION) + _(" (recovery mode) on ")) + rootPartition + "\" {\n" + gfxPayload + \
"\tlinux ("+ mapPart(devMap, grubBootPartition, 0) +")" + kernelstring + " root=" + rootPartition + " ro " + kernel_options + " single\n}\n\n";

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

bool AgiliaSetup::postInstallActions(const string& language, const string& setup_variant, bool tmpfs_tmp, bool initrd_delay, const string& initrd_modules, const string& bootloader, const string& fbmode, const string& kernel_options, const string& netman, const string& hostname, const string& netname, bool time_utc, const string& timezone, const string& nvidia_driver) {
	if (notifier) {
		notifier->setSummaryTextCall(_("Install complete, running post-install actions"));
		notifier->setDetailsTextCall("");
	}

	// Binding pseudo-filesystems
	
	system("mount -o bind /proc /tmp/new_sysroot/proc");
	system("mount -o bind /dev /tmp/new_sysroot/dev");
	system("mount -o bind /sys /tmp/new_sysroot/sys");

	// Locale generation
	if (language=="en_US.UTF-8") {
		generateLangSh("en_US.UTF-8");
		setConsoleKeymap("en");
	}
	else if (language=="ru_RU.UTF-8") {
		generateLangSh("ru_RU.UTF-8");
		setConsoleKeymap("ru");
	}
	else if (language=="uk_UA.UTF-8") {
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
	setRootPassword(rootPassword);
	createUsers(users);

	// Copy skel to root directory
	system("rsync -arvh /tmp/new_sysroot/etc/skel/ /tmp/new_sysroot/root/");
	system("chown -R root:root /tmp/new_sysroot/root");
	xorgSetLangConf(language);
	generateIssue();
	writeFstab(tmpfs_tmp);
	system("chroot /tmp/new_sysroot depmod -a " + kernelversion);
	buildInitrd(initrd_delay, initrd_modules);
	grub2_install(bootloader, fbmode, kernel_options);
	setupNetwork(netman, hostname, netname);
	setTimezone(time_utc, timezone);
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
	if (nvidia_driver=="nouveau") {
		unlink("/tmp/new_sysroot/etc/modprobe.d/nouveau.conf");
	}

	generateFontIndex();
	setXwmConfig(setup_variant);

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

void AgiliaSetup::setDefaultRunlevel(const string& lvl) {
	// Can change runlevels 3 and 4 to lvl
	string data = ReadFile("/tmp/new_sysroot/etc/inittab");
	strReplace(&data, "id:4:initdefault", "id:" + lvl + ":initdefault");
	strReplace(&data, "id:3:initdefault", "id:" + lvl + ":initdefault");
	WriteFile("/tmp/new_sysroot/etc/inittab", data);
}
void AgiliaSetup::enablePlymouth(bool enable) {
	if (enable) system("chroot /tmp/new_sysroot rc-update add plymouth default");
	else system("chroot /tmp/new_sysroot rc-update del plymouth plymouth");
}

void AgiliaSetup::generateFontIndex() {
	if (notifier) {
		notifier->setSummaryTextCall(_("Initializing X11 font database"));
		notifier->setDetailsTextCall(_("Generating font index"));
	}
	if (FileExists("/tmp/new_sysroot/usr/sbin/setup_mkfontdir")) {
		system("chroot /tmp/new_sysroot /usr/sbin/setup_mkfontdir");
	}
	if (notifier) notifier->setDetailsTextCall(_("Generating font cache"));
	if (FileExists("/tmp/new_sysroot/usr/sbin/setup_fontconfig")) {
		system("chroot /tmp/new_sysroot /usr/sbin/setup_fontconfig");
	}
}

void AgiliaSetup::setWM(const string& xinitrc) {
	if (!FileExists("/tmp/new_sysroot/etc/X11/xinit")) return;
	system("( cd /tmp/new_sysroot/etc/X11/xinit ; rm -f xinitrc ; ln -sf xinitrc." + xinitrc + " xinitrc )");
}

void AgiliaSetup::setXwmConfig(const string& setup_variant) {
	string wm = setup_variant;
	if (wm=="KDE") setWM("kde");
	else if (wm=="OPENBOX") setWM("openbox");
	else if (wm=="LXDE") setWM("lxde");
	else if (wm=="XFCE") setWM("xfce");
	else if (wm=="microfluxbox" || wm=="FLUXBOX") setWM("fluxbox");
	else if (wm=="GNOME") setWM("gnome");
}

void AgiliaSetup::generateLangSh(const string& lang, const string& dir) {
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

void AgiliaSetup::setConsoleKeymap(const string& lang) {
	if (lang.find("ru")==0 || lang.find("uk")==0) {
		string keymaps = ReadFile("/tmp/new_sysroot/etc/conf.d/keymaps");
		strReplace(&keymaps, "keymap=\"us\"", "keymap=\"ru-winkeys-uni_ct_sh\"");
		if (!FileExists("/tmp/new_sysroot/etc/conf.d")) system("mkdir /tmp/new_sysroot/etc/conf.d");
		WriteFile("/tmp/new_sysroot/etc/conf.d/keymaps", keymaps);
	}

}

bool AgiliaSetup::validateConfig() {
	if (notifier) notifier->setSummaryTextCall(_("Validating config"));
	if (notifier) notifier->setDetailsTextCall("");
	if (settings["bootloader"].empty()) {
		if (notifier) notifier->sendReportError(_("Bootloader partition not specified, how you wish to boot, Luke?"));
		return false;
	}
	if (settings["setup_variant"].empty()) {
		if (notifier) notifier->sendReportError(_("No setup variant selected, cannot continue"));
		return false;
	}
	if (settings["pkgsource"].empty()) {
		if (notifier) notifier->sendReportError(_("No package source specified, cannot continue"));
		return false;
	}
	else {
		if (settings["pkgsource"]=="dvd") {
			if (settings["volname"].empty()) {
				if (notifier) notifier->sendReportError(_("No volume name specified, cannot continue"));
				return false;
			}
			if (settings["rep_location"].empty()) {
				if (notifier) notifier->sendReportError(_("Repository location not specified, cannot continue"));
				return false;
			}
		}
	}
	if (settings["timezone"].empty()) {
		if (notifier) notifier->sendReportError(_("No timezone specified, cannot continue"));
		return false;
	}
	return true;
}



void AgiliaSetup::run(const map<string, string>& _settings, const vector<TagPair> &_users, const vector<PartConfig> &_partConfigs, const vector<string> &_additional_repositories, void (*updateProgressData) (ItemState)) {
	setupMode = true;

	settings = _settings;
	users = _users;
	partConfigs = _partConfigs;
	additional_repositories = _additional_repositories;

	setLocale(settings["language"]);
	rootPassword = settings["rootpasswd"];

	if (notifier) notifier->setProgressCall(1);
	if (!validateConfig()) return;
	if (notifier) notifier->setProgressCall(5);
	setMpkgConfig(settings["pkgsource"], settings["volname"], settings["rep_location"], additional_repositories);
	if (notifier) notifier->setProgressCall(10);
	if (!getRepositoryData()) return;
	if (notifier) notifier->setProgressCall(25);
	if (!prepareInstallQueue(settings["setup_variant"], settings["netman"], settings["nvidia-driver"])) return;
	if (notifier) notifier->setProgressCall(50);
	if (!validateQueue()) return;
	if (notifier) notifier->setProgressCall(60);
	if (!formatPartitions()) return;
	if (notifier) notifier->setProgressCall(70);
	if (!mountPartitions()) return;
	if (notifier) notifier->setProgressCall(85);
	if (!moveDatabase()) return;
	if (notifier) notifier->setProgressCall(99);
	if (!createBaselayout()) return;

	pData.registerEventHandler(updateProgressData);
	if (!processInstall(settings["pkgsource"])) return;
	if (!postInstallActions(settings["language"], settings["setup_variant"], strToBool(settings["tmpfs_tmp"]), strToBool(settings["initrd_delay"]), settings["initrd_modules"], settings["bootloader"], settings["fbmode"], settings["kernel_options"], settings["netman"], settings["hostname"], settings["netname"], strToBool(settings["time_utc"]), settings["timezone"], settings["nvidia-driver"])) return;
	pData.unregisterEventHandler();
}

