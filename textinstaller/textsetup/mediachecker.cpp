#include "mediachecker.h"
MediaChecker::MediaChecker() {
}

MediaChecker::~MediaChecker() {
}

vector<CustomPkgSet> MediaChecker::getCustomPkgSetList(const string &_pkgsource, char *_locale, string *result_pkgsource, string *r_volname, string *r_rep_location) {
	pkgsource = _pkgsource;
	if (_locale) locale = (string) _locale;

	if (pkgsource=="dvd" || pkgsource.find("iso:///")==0) system("umount /var/log/mount");
	dvdDevice.clear();
	vector<string> rList, dlist;
	if (pkgsource=="dvd" && FileExists("/bootmedia/.volume_id")) {
	       rList.push_back("file:///bootmedia/repository/");
	       pkgsource="file:///bootmedia/repository/";
	}	
	else if (pkgsource=="dvd" || pkgsource.find("iso:///")==0) {
		if (pkgsource.find("iso:///")==0) {
			printf("Checking for ISO\n");
			if (detectDVDDevice(pkgsource.substr(6).c_str())=="") {
				return customPkgSetList;
			}
		}
		else {
			printf("Checking for DVD drive\n");
			if (detectDVDDevice()=="") {
				return customPkgSetList;
			}
		}
		rList.push_back("cdrom://"+volname+"/"+rep_location);

	}
	else rList.push_back(pkgsource);

	mpkg *core = new mpkg;
	core->set_repositorylist(rList, dlist);

	if (pkgsource=="dvd" || pkgsource.find("iso:///")) {
		//ncInterface.showProgressBar(_("Processing repository"), _("Mounting media"), 100, 5);
		mountDVD();
	}

	//ncInterface.showProgressBar(_("Processing repository"), _("Receiving data"), 100, 5);

	core->update_repository_data();
	PACKAGE_LIST pkglist;
	SQLRecord record;
	core->get_packagelist(record, &pkglist, true);
	delete core;
	if (pkglist.IsEmpty()) {
		return customPkgSetList;
	}
	
	printf("Got %d repositories\n", (int) rList.size());
	for (size_t i=0; i<rList.size(); ++i) {
		printf("Repo %d: %s\n", (int) i, rList[i].c_str());
	}
	getCustomSetupVariants(rList);
	if (pkgsource=="dvd" || pkgsource.find("iso:///")) {
		system("umount /var/log/mount");
	}
	// Returning pkgsource, which may be modified during DVD detection. Note that all previous returns in this function means fail, so we don't need to return this stuff earlier.
	*result_pkgsource = pkgsource;
	*r_volname = volname;
	*r_rep_location = rep_location;
	
	printf("Repo detecting complete, sending %d items in pkgSetList\n", (int) customPkgSetList.size());
	return customPkgSetList;

}

string MediaChecker::detectDVDDevice(string isofile) {
	if (!dvdDevice.empty()) return dvdDevice;
	printf("%s\n", __func__);
	system("umount /var/log/mount");
	string cdlist = get_tmp_file();
	vector<string> ret, mOptions;
	if (isofile.empty()) {
		system("getcdromlist.sh " + cdlist + " 2>/dev/null >/dev/null");
		ret = ReadFileStrings(cdlist);
		for (size_t i=0; i<ret.size(); ++i) {
			ret[i] = "/dev/" + ret[i];
			printf("Found drive: /dev/%s\n", ret[i].c_str());
			mOptions.push_back("");
		}
		unlink(cdlist.c_str());
	}
	else ret.push_back(isofile);
	// Let's add fallback devices
	// First: /bootmedia as bind from /media
	ret.push_back("/bootmedia");
	mOptions.push_back("-o bind");
	// Second: ISO image in case of USB FLASH installation (let it be in /bootmedia/iso)
	ret.push_back("/bootmedia/iso/*.iso");
	mOptions.push_back("-o loop");

	// In case of in-ram boot from USB flash, you will need to mount USB drive manually and use ISO image instead of DVD
	// Maybe in future I will implement some sort of logic to detect this case


	string testdev, testoptions;
	while (dvdDevice.empty()) {
		for (size_t i=0; i<ret.size(); ++i) {
			if (isofile.empty()) {
				testdev = ret[i].c_str();
				testoptions = mOptions[i].c_str();
			}
			else {
				testdev = isofile;
				testoptions = "";
			}
			// first of all, umount
			system("umount /var/log/mount 2>/dev/null");
			// Try to mount, check if device exists
			printf("Checking device %s\n", testdev.c_str());
			if (mountDVD(testdev, testoptions, !isofile.empty())) {
				if (FileExists("/var/log/mount/.volume_id")) {
					printf("Found volume_id on device %s\n", testdev.c_str());
					CDROM_MOUNTPOINT="/var/log/mount/";
					CDROM_DEVICE=testdev;
					volname = getCdromVolname(&rep_location);
					printf("Detected volname: %s, rep_location: %s\n", volname.c_str(), rep_location.c_str());
					system("rm -f /dev/cdrom 2>/dev/tty4 >/dev/tty4; ln -s " + testdev + " /dev/cdrom 2>/dev/tty4 >/dev/tty4");

					mpkg *core = new mpkg;
					core->set_cdromdevice(testdev);
					core->set_cdrommountpoint("/var/log/mount/");
					mConfig.setValue("cdrom_mountoptions", testoptions);
					delete core;

					umountDVD();
					dvdDevice = testdev;
					return dvdDevice;
				}
				
				printf("NOT FOUND volume_id on device %s\n", testdev.c_str());
				umountDVD();
			}
		}
		if (dvdDevice.empty()) {
			return "";
		}
	}
	return dvdDevice;
	
}
bool MediaChecker::mountDVD(string dev, string mountOptions, bool iso) {
	if (dev.empty()) dev = dvdDevice;
	if (dev.empty()) {
		printf("%s: Empty device name, returning\n", __func__);
		return false;
	}
	if (!iso) {
		printf("Mounting real DVD drive [%s]\n", dev.c_str());
		system("mkdir -p /var/log/mount");
		if (system("mount " + mountOptions + " " + dev + " /var/log/mount")==0) {
			printf("Real DVD mount of [%s] successful, returning true\n", dev.c_str());
			return true;
		}
	}
	else {
		printf("Mounting ISO image %s\n", dev.c_str());
		system("mkdir -p /var/log/mount");
		if (system("mount -o loop " + dev + " /var/log/mount")==0) {
			printf("ISO image mount of [%s] successful, returning true\n", dev.c_str());
			return true;
		}
	}
	printf("Failed to mount %s\n", dev.c_str());
	return false;
}

bool MediaChecker::umountDVD() {
	if (system("umount /var/log/mount")==0) return true;
	return false;
}


CustomPkgSet MediaChecker::getCustomPkgSet(const string& name) {
	vector<string> data = ReadFileStrings("/tmp/setup_variants/" + name + ".desc");
	CustomPkgSet ret;
	ret.hasX11 = false;
	ret.hasDM = false;
	ret.name = name;
	printf("Processing %s\n", name.c_str());
	string c_locale = locale;
	if (c_locale.size()>2) c_locale = "[" + c_locale.substr(0,2) + "]";
	else c_locale = "";
	string gendesc, genfull;
	for (size_t i=0; i<data.size(); ++i) {
		if (data[i].find("desc" + c_locale + ": ")==0) ret.desc = getParseValue("desc" + c_locale + ": ", data[i], true);
		else if (data[i].find("desc: ")==0) gendesc = getParseValue("desc: ", data[i], true);
		else if (data[i].find("full" + c_locale + ": ")==0) ret.full = getParseValue("full" + c_locale + ": ", data[i], true);
		else if (data[i].find("full: ")==0) genfull = getParseValue("full: ", data[i], true);
		else if (data[i].find("hasX11")==0) ret.hasX11 = true;
		else if (data[i].find("hasDM")==0) ret.hasDM = true;
		else if (data[i].find("hardware" + c_locale + ": ")==0) ret.hw = getParseValue("hardware" + c_locale + ": ", data[i], true);
		else if (data[i].find("hardware: ")==0) if (ret.hw.empty()) ret.hw = getParseValue("hardware: ", data[i], true);
	}
	if (ret.desc.empty()) ret.desc = gendesc;
	if (ret.full.empty()) ret.full = genfull;
	calculatePkgSetSize(ret);
	return ret;
}

void MediaChecker::calculatePkgSetSize(CustomPkgSet &set) {
	vector<string> list = ReadFileStrings("/tmp/setup_variants/" + set.name + ".list");
	PACKAGE_LIST pkgList;
	SQLRecord record;
	mpkg *core = new mpkg;
	core->get_packagelist(record, &pkgList, true);
	delete core;
	int64_t csize = 0, isize = 0;
	size_t count = 0;
	vector<string> was;
	bool pkgWas;
	for (size_t i=0; i<list.size(); ++i) {
		if (list[i].find("#")!=std::string::npos) continue;
		if (cutSpaces(list[i]).empty()) continue;
		pkgWas = false;
		for (size_t w=0; !pkgWas && w<was.size(); ++w) {
			if (was[w]==cutSpaces(list[i])) pkgWas = true;
		}
		if (pkgWas) continue;
		for (size_t t=0; t<pkgList.size(); ++t) {
			if (pkgList[t].get_name()!=cutSpaces(list[i])) continue;
			was.push_back(cutSpaces(list[i]));
			csize += atol(pkgList[t].get_compressed_size().c_str());
			isize += atol(pkgList[t].get_installed_size().c_str());
			count++;
		}
	}
	set.isize = isize;
	set.csize = csize;
	set.count = count;

}

// Get setup variants
void MediaChecker::getCustomSetupVariants(const vector<string>& rep_list) {
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

		//ncInterface.showProgressBar(_("Processing repository"), _("Receiving setup variants"), 100, 5+z*3);

		downloadQueue.clear();
		path = rep_list[z];
		CommonGetFile(path + "/setup_variants.list", tmpfile);
		CommonGetFile(path + "/setup_variants.tar.xz", "/tmp/setup_variants.tar.xz");
		vector<string> list = ReadFileStrings(tmpfile);
		printf("Received %d setup variants\n", (int) list.size());
		vector<CustomPkgSet> ret;
		if (!FileExists("/tmp/setup_variants.tar.xz")) {
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
		}
		else {
			system("( cd /tmp && tar xf setup_variants.tar.xz )");
		}

		printf("Starting importing package lists\n");
		for (size_t i=0; i<list.size(); ++i) {
			//ncInterface.showProgressBar(_("Processing repository"), _("Processing setup variants"), 100, (10+(z+1)*3+( (double)((100-(10+(z+1)*3))/(double) list.size())*(i+1) )));

			printf("Processing %d of %d\n", (int) i+1, (int) list.size());
			customPkgSetList.push_back(getCustomPkgSet(list[i]));
		}
	}
}

