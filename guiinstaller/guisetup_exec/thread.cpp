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

	map<string, map<string, string> > partitions;
	vector<string> repositories;
	string home = getenv("HOME");
	loadSettings(home + "/.config/agilia_installer.conf", strSettings, repositories, partitions);

	
	agiliaSetup.users.push_back(TagPair(strSettings["username"], strSettings["password"]));

	PartConfig *pConfig;
	map<string, map<string, string> >::iterator mapit;
	for (mapit=partitions.begin(); mapit!=partitions.end(); mapit++) {
		pConfig = new PartConfig;
		pConfig->partition = mapit->first;
		pConfig->mountpoint = partitions[mapit->first]["mountpoint"];
		pConfig->format = atoi(partitions[mapit->first]["format"].c_str());
		pConfig->fs = partitions[mapit->first]["fs"];
		pConfig->mount_options = partitions[mapit->first]["options"];
		if (!pConfig->mountpoint.empty()) partConfigs.push_back(*pConfig); // Skip subvolume groups from list
		delete pConfig;
	}

	// Save
	*_strSettings = strSettings;
	*_users = users;
	*_partConfigs = partConfigs;
	*_additional_repositories = additional_repositories;

}
