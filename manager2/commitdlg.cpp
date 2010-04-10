#include "commitdlg.h"
#include "ui_commitdlg.h"
#include <mpkg/libmpkg.h>
CommitDialog::CommitDialog(const PACKAGE_LIST& _i, const PACKAGE_LIST& r, QWidget *parent) : QDialog(parent), ui(new Ui::CommitDialogClass) {
	ui->setupUi(this);
	installQueue = _i;
	removeQueue = r;
	// Rendering upgrade queue
	for (unsigned int i=0; i<installQueue.size(); ++i) {
		for (unsigned int t=0; t<removeQueue.size(); ++t) {
			if (installQueue[i].get_name()==removeQueue[t].get_name()) {
				upgradeQueue_new.push_back(installQueue.get_package_ptr(i));
				upgradeQueue_old.push_back(removeQueue.get_package_ptr(t));
			}
		}
	}
	for (unsigned int i=0; i<installQueue.size(); ++i) {
		ui->installListWidget->addItem(QString::fromStdString(installQueue[i].get_name() + " " + installQueue[i].get_fullversion()));
	}
	for (unsigned int i=0; i<removeQueue.size(); ++i) {
		ui->removeListWidget->addItem(QString::fromStdString(removeQueue[i].get_name() + " " + removeQueue[i].get_fullversion()));
	}
	for (int i=0; i<upgradeQueue_old.size() && i<upgradeQueue_new.size(); ++i) {
		ui->upgradeListWidget->addItem(QString::fromStdString(upgradeQueue_old[i]->get_name() + ": " + upgradeQueue_old[i]->get_fullversion() + " ==> " + upgradeQueue_new[i]->get_fullversion()));
	}
	connect(ui->installListWidget, SIGNAL(currentRowChanged(int)), this, SLOT(showInstallPackageInfo(int)));
	connect(ui->removeListWidget, SIGNAL(currentRowChanged(int)), this, SLOT(showRemovePackageInfo(int)));
	connect(ui->upgradeListWidget, SIGNAL(currentRowChanged(int)), this, SLOT(showUpgradePackageInfo(int)));
	connect(ui->buttonBox, SIGNAL(accepted()), this, SIGNAL(commit()));
	double dl_size=0, ins_size=0, rm_size=0;
	for (unsigned int i=0; i<installQueue.size(); ++i) {
		ins_size += strtod(installQueue[i].get_installed_size().c_str(), NULL);
		dl_size += strtod(installQueue[i].get_compressed_size().c_str(), NULL);
	}
	for (unsigned int i=0; i<removeQueue.size(); ++i) {
		rm_size += strtod(removeQueue[i].get_installed_size().c_str(), NULL);
	}
	QString diskSpaceDiff;
	if (ins_size==rm_size) diskSpaceDiff=tr("<b>Disk space will be unchanged</b>");
	if (ins_size>rm_size) diskSpaceDiff=tr("<b>Disk space will be occupied: </b>%1").arg(QString::fromStdString(humanizeSize(ins_size-rm_size)));
	if (ins_size<rm_size) diskSpaceDiff=tr("<b>Disk space will be freed: </b>%1").arg(QString::fromStdString(humanizeSize(rm_size-ins_size)));

	QString totalDownloadSize = QString::fromStdString(humanizeSize(dl_size));
	ui->actionSummaryLabel->setText(tr("<b>To be installed: </b>%1 packages<br><b>To be removed: </b>%2 packages<br><b>To be upgraded: </b>%3<br><b>Download size: </b>%4<br>%5<br>").arg(installQueue.size()).arg(removeQueue.size()).arg(upgradeQueue_new.size()).arg(totalDownloadSize).arg(diskSpaceDiff));
}

CommitDialog::~CommitDialog() {
}

void CommitDialog::showInstallPackageInfo(int row) {
	showPackageInfo(&installQueue[row]);
}
void CommitDialog::showRemovePackageInfo(int row) {
	showPackageInfo(&removeQueue[row]);
}
void CommitDialog::showUpgradePackageInfo(int row) {
	showPackageInfo(upgradeQueue_new[row]);
}

void CommitDialog::showPackageInfo(const PACKAGE *pkg) {
	ui->packageInfoLabel->setText(QString("<b>%1 %2</b><br>%3<br><b>md5: </b>%4<br>").arg(QString::fromStdString(pkg->get_name())).arg(QString::fromStdString(pkg->get_fullversion())).arg(QString::fromStdString(pkg->get_short_description())).arg(QString::fromStdString(pkg->get_md5())));
	//ui->packageInfoLabel->setText(QString::fromStdString("<b>" + pkg->get_name() + " " + pkg->get_fullversion() + "</b><br>" + pkg->get_short_description()));
}
