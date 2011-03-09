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

void SetupThread::setProgressCall(int progress) {
	emit setProgress(progress);
}
void SetupThread::sendReportError(const string& text) {
	emit reportError(QString::fromStdString(text));
}


void SetupThread::skipMD5() {
	forceSkipLinkMD5Checks = true;
	forceInInstallMD5Check = false;
	emit enableMD5Button(false);
}

void SetupThread::run() {
	progressWidgetPtr = this;
	map<string, string> strSettings;
	vector<PartConfig> partConfigs;
	vector<TagPair> users;
	vector<string> additional_repositories;

	parseConfig(&strSettings, &users, &partConfigs, &additional_repositories);

	agiliaSetup.registerStatusNotifier(this);
	agiliaSetup.run(strSettings, users, partConfigs, additional_repositories, &updateProgressData);
	emit reportFinish();
}

void SetupThread::parseConfig(map<string, string> *_strSettings, vector<TagPair> *_users, vector<PartConfig> *_partConfigs, vector<string> *_additional_repositories) {
	map<string, string> strSettings;
	vector<PartConfig> partConfigs;
	vector<TagPair> users;
	vector<string> additional_repositories;
	
	QSettings *settings = new QSettings("guiinstaller");

	// Generic pairs
	QStringList params = settings->childKeys();
	for (int i=0; i<params.size(); ++i) {
		strSettings[params[i].toStdString()]=settings->value(params[i]).toString().toStdString();
	}

	// Users
	settings->beginGroup("users");
	QStringList usernames = settings->childKeys();
	for (int i=0; i<usernames.size(); ++i) {
		agiliaSetup.users.push_back(TagPair(usernames[i].toStdString(), settings->value(usernames[i]).toString().toStdString()));
	}
	settings->endGroup();

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
	PartConfig *pConfig;
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

	// Repos
	int add_url_size = settings->beginReadArray("additional_repositories");
	for (int i=0; i<add_url_size; ++i) {
		settings->setArrayIndex(i);
		additional_repositories.push_back(settings->value("url").toString().toStdString());
	}
	settings->endArray();


	// Save
	*_strSettings = strSettings;
	*_users = users;
	*_partConfigs = partConfigs;
	*_additional_repositories = additional_repositories;

}
