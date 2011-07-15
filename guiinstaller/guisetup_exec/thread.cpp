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
	if (a.itemOrder == ITEMORDER_FIRST) { // This piece of code can be named as OMGWTF KOSTYLI, so we need a more elegant solution here.
		emit showMD5Button(true);
	}
	else if (a.itemOrder == ITEMORDER_LAST) {
		emit showMD5Button(false);
	}
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

	printf("MERGE_IN: %s\n", strSettings["merge_setup_variant"].c_str());
	agiliaSetup.run(strSettings, users, partConfigs, additional_repositories, &updateProgressData);
	emit reportFinish();
}


