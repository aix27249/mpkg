#include "thread.h"

void LoadSetupVariantsThread::run() {
	mpkg *core = new mpkg;

	vector<string> rList, dlist;
	if (pkgsource=="dvd" || pkgsource.toStdString().find("iso:///")==0) {
		if (pkgsource.toStdString().find("iso:///")==0) {
			if (detectDVDDevice(pkgsource.toStdString().substr(6).c_str())=="") {
				emit finished(false);
				return;
			}
		}
		else {
			if (detectDVDDevice()=="") {
				emit finished(false);
				return;
			}
		}
		rList.push_back("cdrom://"+volname+"/"+rep_location);

	}
	else rList.push_back(pkgsource.toStdString());
	// TODO: respect options other than DVD, please
	if (pkgsource=="dvd") {
		mountDVD();
		cacheCdromIndex(volname, rep_location);
	}
	core->set_repositorylist(rList, dlist);
	core->update_repository_data();
	PACKAGE_LIST pkglist;
	SQLRecord record;
	core->get_packagelist(record, &pkglist, true);
	if (pkglist.IsEmpty()) {
		emit finished(false);
	}
	delete core;
	emit finished(true);

}

QString LoadSetupVariantsThread::detectDVDDevice(QString isofile) {
	if (!dvdDevice.isEmpty()) return dvdDevice;
	string cdlist = get_tmp_file();
	vector<string> ret, mOptions;
	if (isofile.isEmpty()) {
		system("getcdromlist.sh " + cdlist + " 2>/dev/null >/dev/null");
		ret = ReadFileStrings(cdlist);
		mOptions.resize(ret.size());
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


	mpkg *core;
	QString testdev, testoptions;
	while (dvdDevice.isEmpty()) {
		for (size_t i=0; i<ret.size(); ++i) {
			if (isofile.isEmpty()) {
				testdev = QString("/dev/") + ret[i].c_str();
				testoptions = QString("/dev/") + mOptions[i].c_str();
			}
			else {
				testdev = isofile;
				testoptions = "";
			}
			// first of all, umount
			system("umount /var/log/mount 2>/dev/null");
			// Try to mount, check if device exists
			if (mountDVD(testdev, testoptions, !isofile.isEmpty())) {
				if (FileExists("/var/log/mount/.volume_id")) {
					volname = getCdromVolname(&rep_location);
					system("rm -f /dev/cdrom 2>/dev/tty4 >/dev/tty4; ln -s " + testdev.toStdString() + " /dev/cdrom 2>/dev/tty4 >/dev/tty4");
					core = new mpkg;
					core->set_cdromdevice(testdev.toStdString());
					core->set_cdrommountpoint("/var/log/mount/");
					mConfig.setValue("cdrom_mountoptions", testoptions.toStdString());
					delete core;

					umountDVD();
					dvdDevice = testdev;
					return dvdDevice;
				}
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
	if (!iso) {
		printf("Mounting real DVD drive %s\n", dev.toStdString().c_str());
		if (system("mount " + mountOptions.toStdString() + " " + dev.toStdString() + " /var/log/mount")==0) return true;
	}
	else {
		printf("Mounting ISO image %s\n", dev.toStdString().c_str());
		if (system("mount -o loop " + dev.toStdString() + " /var/log/mount")==0) return true;
	}
	printf("Failed to mount %s\n", dev.toStdString().c_str());
	return false;
}

bool LoadSetupVariantsThread::umountDVD() {
	if (system("umount /var/log/mount")==0) return true;
	return false;
}
