#include "agiliasetup.h"

AgiliaSetup::AgiliaSetup() {
}

AgiliaSetup::~AgiliaSetup() {
}

void AgiliaSetup::setDefaultXDM() {
	if (FileExists("/tmp/new_sysroot/etc/init.d/kdm")) system("chroot /tmp/new_sysroot rc-update add kdm X11");
	else if (FileExists("/tmp/new_sysroot/etc/init.d/gdm")) system("chroot /tmp/new_sysroot rc-update add gdm X11");
	else if (FileExists("/tmp/new_sysroot/etc/init.d/lxdm")) system("chroot /tmp/new_sysroot rc-update add lxdm X11");
	else if (FileExists("/tmp/new_sysroot/etc/init.d/slim")) system("chroot /tmp/new_sysroot rc-update add slim X11");
	else if (FileExists("/tmp/new_sysroot/etc/init.d/xdm")) system("chroot /tmp/new_sysroot rc-update add xdm X11");

}

void AgiliaSetup::setDefaultRunlevels() {
	system("chroot /tmp/new_sysroot rc-update add mdadm boot");
	system("chroot /tmp/new_sysroot rc-update add lvm boot");
	system("chroot /tmp/new_sysroot rc-update add sysfs sysinit");
	system("chroot /tmp/new_sysroot rc-update add udev sysinit");
	system("chroot /tmp/new_sysroot rc-update add consolefont default");
	system("chroot /tmp/new_sysroot rc-update add hald default");
	system("chroot /tmp/new_sysroot rc-update add sysklogd default");
	system("chroot /tmp/new_sysroot rc-update add dbus default");
	system("chroot /tmp/new_sysroot rc-update add sshd default");
	system("chroot /tmp/new_sysroot rc-update add alsasound default");
	system("chroot /tmp/new_sysroot rc-update add acpid default");
	system("chroot /tmp/new_sysroot rc-update add cupsd default");
	system("chroot /tmp/new_sysroot rc-update add cron default");

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
		system("chroot /tmp/new_sysroot rc-update add wicd default");
	}
	else if (netman=="networkmanager") {
		system("chroot /tmp/new_sysroot rc-update add networkmanager default");
	}
	else if (netman=="netconfig") {
		system("chroot /tmp/new_sysroot rc-update add network default");
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
	system("chroot /tmp/new_sysroot umount /proc");
	system("chroot /tmp/new_sysroot umount /sys");
	system("chroot /tmp/new_sysroot umount -a");
	system("sync");

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

void AgiliaSetup::getCustomSetupVariants(const vector<string>& rep_list, StatusNotifier *notifier) {
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
			if (notifier) notifier->setDetailsTextCallback(_("Importing ") + IntToStr(i+1) + _(" of ") + IntToStr(list.size()) + ": " + list[i]);
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

bool AgiliaSetup::getRepositoryData(StatusNotifier *notifier) {
	mpkg *core = new mpkg;
	if (core->update_repository_data()!=0) {
		if (notifier) notifier->sendReportError(_("Failed to retreive repository data, cannot continue"));
		delete core;
		return false;
	}
	if (notifier) notifier->setDetailsTextCallback(_("Caching setup variants"));
	getCustomSetupVariants(core->get_repositorylist());
	delete core;
	return true;
}



bool AgiliaSetup::prepareInstallQueue(const string& setup_variant, const string& netman, const string& nvidia_driver, StatusNotifier *notifier) {
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

bool AgiliaSetup::validateQueue(StatusNotifier *notifier) {
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

bool AgiliaSetup::formatPartition(PartConfig pConfig, StatusNotifier *notifier) {

	if (notifier) notifier->setDetailsTextCallback(_("Formatting /dev/") + pConfig.partition);
	printf("Formatting /dev/%s\n", pConfig.partition.c_str());
	string fs_options;
	if (pConfig.fs=="jfs") fs_options="-q";
	else if (pConfig.fs=="xfs") fs_options="-f -q";
	else if (pConfig.fs=="reiserfs") fs_options="-q";
	if (system("umount -l /dev/" + pConfig.partition +  " ; mkfs -t " + pConfig.fs + " " + fs_options + " /dev/" + pConfig.partition)==0) return true;
	else return false;
}

bool AgiliaSetup::makeSwap(PartConfig pConfig, StatusNotifier *notifier) {
	if (notifier) notifier->setDetailsTextCallback(_("Creating swap in ") + pConfig.partition);
	system("swapoff /dev/" + pConfig.partition);
	if (system("mkswap /dev/" + pConfig.partition)==0) return false;
	return true;
}

bool AgiliaSetup::activateSwap(PartConfig pConfig) {
	if (system("swapon /dev/" + pConfig.partition)!=0) return false;
	return true;
}



