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
	if (a.totalProgress>=0 && a.totalProgress<=100) emit setProgress(a.totalProgress);
}

void SetupThread::setDetailsTextCall(const string& msg) {
	emit setDetailsText(QString::fromStdString(msg));
}

void SetupThread::setSummaryTextCall(const string& msg) {
	emit setSummaryText(QString::fromStdString(msg));
}
void SetupThread::sendReportError(const string& text) {
	emit reportError(QString::fromStdString(text));
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
	return agiliaSetup.getRepositoryData();
}

bool SetupThread::prepareInstallQueue() {
	emit setSummaryText(tr("Preparing install queue"));
	emit setDetailsText("");
	return agiliaSetup.prepareInstallQueue(settings->value("setup_variant").toString().toStdString(), settings->value("netman").toString().toStdString(),settings->value("nvidia-driver").toString().toStdString());
}

bool SetupThread::validateQueue() {
	emit setSummaryText(tr("Validating queue"));
	emit setDetailsText("");
	return agiliaSetup.validateQueue();
}



bool SetupThread::fillPartConfigs() {
	// Parser-dependant. In general, this is bad idea, so maybe we should move to some generic parser instead of QSettings.
	PartConfig *pConfig;
	vector<PartConfig> partConfigs;
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
	agiliaSetup.setPartConfigs(partConfigs);
	if (partConfigs.empty()) return false;
	return true;
}
bool SetupThread::formatPartitions() {
	emit setSummaryText(tr("Formatting partitions"));
	emit setDetailsText("");
	return agiliaSetup.formatPartitions();
}

bool SetupThread::mountPartitions() {
	emit setSummaryText(tr("Mounting partitions"));
	emit setDetailsText("");
	return agiliaSetup.mountPartitions();
}

bool SetupThread::moveDatabase() {
	emit setSummaryText(tr("Moving database"));
	emit setDetailsText("");
	return agiliaSetup.moveDatabase();

}

bool SetupThread::processInstall() {
	emit setSummaryText(tr("Installing packages"));
	emit setDetailsText(tr("Preparing to installation"));
	return agiliaSetup.processInstall(settings->value("pkgsource").toString().toStdString());
}


bool SetupThread::postInstallActions() {
	return agiliaSetup.postInstallActions(settings->value("language").toString().toStdString(), settings->value("setup_variant").toString().toStdString(), settings->value("tmpfs_tmp").toBool(), settings->value("initrd_delay").toBool(), settings->value("initrd_modules").toString().toStdString(), settings->value("bootloader").toString().toStdString(), settings->value("fbmode").toString().toStdString(), settings->value("kernel_options").toString().toStdString(), settings->value("netman").toString().toStdString(), settings->value("hostname").toString().toStdString(), settings->value("netname").toString().toStdString(), settings->value("time_utc").toBool(), settings->value("timezone").toString().toStdString(), settings->value("nvidia-driver").toString().toStdString());
}


void SetupThread::run() {
	verbose=true;
	progressWidgetPtr = this;
	string errors;
	setupMode = true;

	settings = new QSettings("guiinstaller");
	agiliaSetup.setLocale(settings->value("language").toString().toStdString());
	agiliaSetup.registerStatusNotifier(this);
	agiliaSetup.rootPassword = settings->value("rootpasswd").toString().toStdString();
	settings->beginGroup("users");

	QStringList usernames = settings->childKeys();
	for (int i=0; i<usernames.size(); ++i) {
		agiliaSetup.users.push_back(TagPair(usernames[i].toStdString(), settings->value(usernames[i]).toString().toStdString()));
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



