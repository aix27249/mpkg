#include "thread.h"

void LoadSetupVariantsThread::run() {
	if (pkgsource=="dvd" || pkgsource.toStdString().find("iso:///")==0) system("umount /var/log/mount");
	dvdDevice.clear();
	vector<string> rList, dlist;
	if (pkgsource=="dvd" && FileExists("/bootmedia/.volume_id")) {
	       rList.push_back("file:///bootmedia/repository/");
	       pkgsource="file:///bootmedia/repository/";
	}
	
	else if (pkgsource=="dvd" || pkgsource.toStdString().find("iso:///")==0) {
		if (pkgsource.toStdString().find("iso:///")==0) {
			printf("Checking for ISO\n");
			if (detectDVDDevice(pkgsource.toStdString().substr(6).c_str())=="") {
				emit finished(false, customPkgSetList);
				return;
			}
		}
		else {
			printf("Checking for DVD drive\n");
			if (detectDVDDevice()=="") {
				emit finished(false, customPkgSetList);
				return;
			}
		}
		rList.push_back("cdrom://"+volname+"/"+rep_location);

	}
	else rList.push_back(pkgsource.toStdString());

	mpkg *core = new mpkg;
	core->set_repositorylist(rList, dlist);

	if (pkgsource=="dvd" || pkgsource.toStdString().find("iso:///")) {
		emit sendLoadText(tr("Mounting media"));
		emit sendLoadProgress(5);
		mountDVD();
	}

	emit sendLoadText(tr("Receiving repository data"));
	emit sendLoadProgress(10);

	core->update_repository_data();
	PACKAGE_LIST pkglist;
	SQLRecord record;
	core->get_packagelist(record, &pkglist, true, false);
	delete core;
	if (pkglist.IsEmpty()) {
		emit finished(false, customPkgSetList);
		return;
	}
	
	printf("Got %d repositories\n", (int) rList.size());
	for (size_t i=0; i<rList.size(); ++i) {
		printf("Repo %d: %s\n", (int) i, rList[i].c_str());
	}
	getCustomSetupVariants(rList);
	if (pkgsource=="dvd" || pkgsource.toStdString().find("iso:///")) {
		system("umount /var/log/mount");
	}
	
	printf("Repo detecting complete, sending %d items in pkgSetList\n", (int) customPkgSetList.size());
	emit finished(true, customPkgSetList);

}

QString LoadSetupVariantsThread::detectDVDDevice(QString isofile) {
	if (!dvdDevice.isEmpty()) return dvdDevice;
	printf("%s\n", __func__);
	system("umount /var/log/mount");
	string cdlist = get_tmp_file();
	vector<string> ret, mOptions;
	if (isofile.isEmpty()) {
		system("getcdromlist.sh " + cdlist + " 2>/dev/null >/dev/null");
		ret = ReadFileStrings(cdlist);
		for (size_t i=0; i<ret.size(); ++i) {
			ret[i] = "/dev/" + ret[i];
			printf("Found drive: /dev/%s\n", ret[i].c_str());
			mOptions.push_back("");
		}
		unlink(cdlist.c_str());
	}
	else ret.push_back(isofile.toStdString());
	// Let's add fallback devices
	// First: /bootmedia as bind from /media
	ret.push_back("/bootmedia");
	mOptions.push_back("-o bind");
	// Second: ISO image in case of USB FLASH installation (let it be in /bootmedia/iso)
	ret.push_back("/bootmedia/iso/*.iso");
	mOptions.push_back("-o loop");

	// In case of in-ram boot from USB flash, you will need to mount USB drive manually and use ISO image instead of DVD
	// Maybe in future I will implement some sort of logic to detect this case


	QString testdev, testoptions;
	while (dvdDevice.isEmpty()) {
		for (size_t i=0; i<ret.size(); ++i) {
			if (isofile.isEmpty()) {
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
			printf("Checking device %s\n", testdev.toStdString().c_str());
			if (mountDVD(testdev, testoptions, !isofile.isEmpty())) {
				if (FileExists("/var/log/mount/.volume_id")) {
					printf("Found volume_id on device %s\n", testdev.toStdString().c_str());
					CDROM_MOUNTPOINT="/var/log/mount/";
					CDROM_DEVICE=testdev.toStdString();
					volname = getCdromVolname(&rep_location);
					printf("Detected volname: %s, rep_location: %s\n", volname.c_str(), rep_location.c_str());
					system("rm -f /dev/cdrom 2>/dev/tty4 >/dev/tty4; ln -s " + testdev.toStdString() + " /dev/cdrom 2>/dev/tty4 >/dev/tty4");

					mpkg *core = new mpkg;
					core->set_cdromdevice(testdev.toStdString());
					core->set_cdrommountpoint("/var/log/mount/");
					mConfig.setValue("cdrom_mountoptions", testoptions.toStdString());
					delete core;

					umountDVD();
					dvdDevice = testdev;
					return dvdDevice;
				}
				
				printf("NOT FOUND volume_id on device %s\n", testdev.toStdString().c_str());
				umountDVD();
			}
		}
		if (dvdDevice.isEmpty()) {
			return "";
		}
	}
	return dvdDevice;
	
}
bool LoadSetupVariantsThread::mountDVD(QString dev, QString mountOptions, bool iso) {
	if (dev.isEmpty()) dev = dvdDevice;
	if (dev.isEmpty()) {
		printf("%s: Empty device name, returning\n", __func__);
		return false;
	}
	if (!iso) {
		printf("Mounting real DVD drive [%s]\n", dev.toStdString().c_str());
		system("mkdir -p /var/log/mount");
		if (system("mount " + mountOptions.toStdString() + " " + dev.toStdString() + " /var/log/mount")==0) {
			printf("Real DVD mount of [%s] successful, returning true\n", dev.toStdString().c_str());
			return true;
		}
	}
	else {
		printf("Mounting ISO image %s\n", dev.toStdString().c_str());
		system("mkdir -p /var/log/mount");
		if (system("mount -o loop " + dev.toStdString() + " /var/log/mount")==0) {
			printf("ISO image mount of [%s] successful, returning true\n", dev.toStdString().c_str());
			return true;
		}
	}
	printf("Failed to mount %s\n", dev.toStdString().c_str());
	return false;
}

bool LoadSetupVariantsThread::umountDVD() {
	if (system("umount /var/log/mount")==0) return true;
	return false;
}



// Get setup variants
void LoadSetupVariantsThread::getCustomSetupVariants(const vector<string>& rep_list) {
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
		emit sendLoadText(tr("Receiving setup variants"));
		emit sendLoadProgress(5+z*3);

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
			emit sendLoadText(tr("Processing setup variants"));
			emit sendLoadProgress(10+(z+1)*3+( (double)((100-(10+(z+1)*3))/(double) list.size())*(i+1) ));

			printf("Processing %d of %d\n", (int) i+1, (int) list.size());
			customPkgSetList.push_back(getCustomPkgSet(list[i], locale));
		}
	}
}

