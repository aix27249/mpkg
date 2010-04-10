#include "settings.h"
#include "ui_settings.h"
#include <QInputDialog>
#include <mpkg/menu.h>
SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent), ui(new Ui::SettingsDialogClass) {
	ui->setupUi(this);
	connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(save()));
	connect(ui->addUpdateBlacklistButton, SIGNAL(clicked()), this, SLOT(addUpdateBlacklist()));
	connect(ui->addRemoveProtectedButton, SIGNAL(clicked()), this, SLOT(addRemoveProtected()));
	connect(ui->removeUpdateBlacklistButton, SIGNAL(clicked()), this, SLOT(removeUpdateBlacklist()));
	connect(ui->removeRemoveProtectedButton, SIGNAL(clicked()), this, SLOT(removeRemoveProtected()));
	connect(ui->addRepositoryButton, SIGNAL(clicked()), this, SLOT(addRepository()));
	connect(ui->removeRepositoryButton, SIGNAL(clicked()), this, SLOT(deleteRepository()));
	connect(ui->loadProfileButton, SIGNAL(clicked()), this, SLOT(loadProfile()));
	connect(ui->saveProfileButton, SIGNAL(clicked()), this, SLOT(saveProfile()));
	connect(ui->deleteProfileButton, SIGNAL(clicked()), this, SLOT(deleteProfile()));
	connect(ui->getRepositoryListButton, SIGNAL(clicked()), this, SLOT(getRepositoryList()));
	load();
}

SettingsDialog::~SettingsDialog() {
	printf("%s\n", __func__);
}
void SettingsDialog::addUpdateBlacklist() {
	if (ui->addUpdateBlacklistLineEdit->text().isEmpty()) return;
	ui->updateBlacklistListWidget->addItem(ui->addUpdateBlacklistLineEdit->text());
}

void SettingsDialog::addRemoveProtected() {
	if (ui->addRemoveProtectedLineEdit->text().isEmpty()) return;
	ui->removeProtectedListWidget->addItem(ui->addRemoveProtectedLineEdit->text());
}

void SettingsDialog::removeUpdateBlacklist() {
	int index = ui->updateBlacklistListWidget->currentRow();
	if (index < 0 || index > ui->updateBlacklistListWidget->count()) return;
	ui->updateBlacklistListWidget->takeItem(index);
}

void SettingsDialog::removeRemoveProtected() {
	int index = ui->removeProtectedListWidget->currentRow();
	if (index < 0 || index > ui->removeProtectedListWidget->count()) return;
	ui->removeProtectedListWidget->takeItem(index);
}

void SettingsDialog::load() {
	// Tab 1: core settings
	if (mConfig.getValue("disable_dependencies")=="yes") ui->disableDependenciesCheckBox->setChecked(true);
	else ui->disableDependenciesCheckBox->setChecked(false);
	if (mConfig.getValue("enable_download_resume")=="yes") ui->enableDownloadResumeCheckBox->setChecked(true);
	else ui->enableDownloadResumeCheckBox->setChecked(false);
	if (mConfig.getValue("enable_prelink")=="yes") ui->enablePrelinkCheckBox->setChecked(true);
	else ui->enablePrelinkCheckBox->setChecked(false);
	if (mConfig.getValue("enable_prelink_randomization")=="yes") ui->enablePrelinkRandomCheckBox->setChecked(true);
	else ui->enablePrelinkRandomCheckBox->setChecked(false);
	if (mConfig.getValue("enable_delta")=="yes") ui->enableBDeltaCheckBox->setChecked(true);
	else ui->enableBDeltaCheckBox->setChecked(false);
	ui->cdromDeviceEdit->setText(QString::fromStdString(mConfig.getValue("cdrom_device")));
	ui->cdromMountEdit->setText(QString::fromStdString(mConfig.getValue("cdrom_mountpoint")));

	// Tab 2: repositories 
	// -----%<-------------------------from here------------------------------------->%-----------------
	vector<string> enabledRepositories = mpkgconfig::get_repositorylist();
	vector<string> disabledRepositories = mpkgconfig::get_disabled_repositorylist();
	ui->repositoryListWidget->clear();
	for (unsigned int i=0; i<enabledRepositories.size(); ++i) {
		QListWidgetItem *item = new QListWidgetItem(QString::fromStdString(enabledRepositories[i]));
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
		item->setCheckState(Qt::Checked);
		ui->repositoryListWidget->addItem(item);
	}
	for (unsigned int i=0; i<disabledRepositories.size(); ++i) {
		QListWidgetItem *item = new QListWidgetItem(QString::fromStdString(disabledRepositories[i]));
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		item->setCheckState(Qt::Unchecked);
		ui->repositoryListWidget->addItem(item);
	}
	// ---%<-------------------------------to here------------------------------>%------------
	// Repo profiles
	
	vector<string> pList = getDirectoryList("/etc/mpkg/profiles");
	ui->repositoryProfilesComboBox->clear();
	for (unsigned int i=0; i<pList.size(); ++i) {
		ui->repositoryProfilesComboBox->addItem(QString::fromStdString(pList[i]));
	}

	// Tab 3: Update blacklist
	vector<string> updateBlacklist = ReadFileStrings("/etc/mpkg-update-blacklist");
	for (unsigned int i=0; i<updateBlacklist.size(); ++i) {
		ui->updateBlacklistListWidget->addItem(QString::fromStdString(updateBlacklist[i]));
	}

	vector<string> removeProtected = ReadFileStrings("/etc/mpkg-remove-blacklist");
	for (unsigned int i=0; i<removeProtected.size(); ++i) {
		ui->removeProtectedListWidget->addItem(QString::fromStdString(removeProtected[i]));
	}

	vector<string> unremovable = ReadFileStrings("/etc/mpkg-unremovable");
	for (unsigned int i=0; i<unremovable.size(); ++i) {
		ui->unremovableListWidget->addItem(QString::fromStdString(unremovable[i]));
	}

	// Build settings
	QString march, mtune, olevel, package_output, source_cache_dir;
	march = QString::fromStdString(mConfig.getValue("march"));
	mtune = QString::fromStdString(mConfig.getValue("mtune"));
	olevel = QString::fromStdString(mConfig.getValue("olevel"));
	if (!march.isEmpty()) ui->cpuArchComboBox->setEditText(march);
	if (!mtune.isEmpty()) ui->cpuTuneComboBox->setEditText(mtune);
	if (!olevel.isEmpty()) ui->oLevelComboBox->setEditText(olevel);
	int numjobs = atoi(mConfig.getValue("numjobs").c_str());
	if (numjobs < 1) numjobs = 2;
	ui->numjobsSpinBox->setValue(numjobs);
	ui->maintainerNameEdit->setText(QString::fromStdString(mConfig.getValue("maintainer_name")));
	ui->maintainerEmailEdit->setText(QString::fromStdString(mConfig.getValue("maintainer_email")));
	package_output = QString::fromStdString(mConfig.getValue("package_output"));
	if (!package_output.isEmpty())	ui->pkgOutputEdit->setText(package_output);
	source_cache_dir = QString::fromStdString(mConfig.getValue("source_cache_dir"));
	if (!source_cache_dir.isEmpty()) ui->sourceCacheEdit->setText(source_cache_dir);
	QString pkgOutputType = QString::fromStdString(mConfig.getValue("build_pkg_type"));
	if (pkgOutputType.isEmpty()) pkgOutputType="txz";
	ui->outputPkgTypeComboBox->setEditText(pkgOutputType);

}

void SettingsDialog::save() {
	// Tab 1: core settings
	if (ui->disableDependenciesCheckBox->isChecked()) mConfig.setValue("disable_dependencies", "yes");
	else mConfig.setValue("disable_dependencies", "no");
	if (ui->enableDownloadResumeCheckBox->isChecked()) mConfig.setValue("enable_download_resume", "yes");
	else mConfig.setValue("enable_download_resume", "no");
	if (ui->enablePrelinkCheckBox->isChecked()) mConfig.setValue("enable_prelink", "yes");
	else mConfig.setValue("enable_prelink", "no");
	if (ui->enablePrelinkRandomCheckBox->isChecked()) mConfig.setValue("enable_prelink_randomization", "yes");
	else mConfig.setValue("enable_prelink_randomization", "no");
	if (ui->enableBDeltaCheckBox->isChecked()) mConfig.setValue("enable_delta", "yes");
	else mConfig.setValue("enable_delta", "no");
	mConfig.setValue("cdrom_device", ui->cdromDeviceEdit->text().toStdString());
	mConfig.setValue("cdrom_mountpoint", ui->cdromMountEdit->text().toStdString());

	// Build settings
	mConfig.setValue("march", ui->cpuArchComboBox->currentText().toStdString());
	mConfig.setValue("mtune", ui->cpuTuneComboBox->currentText().toStdString());
	mConfig.setValue("olevel", ui->oLevelComboBox->currentText().toStdString());
	mConfig.setValue("numjobs", IntToStr(ui->numjobsSpinBox->value()));
	mConfig.setValue("maintainer_name", ui->maintainerNameEdit->text().toStdString());
	mConfig.setValue("maintainer_email", ui->maintainerEmailEdit->text().toStdString());
	mConfig.setValue("package_output", ui->pkgOutputEdit->text().toStdString());
	mConfig.setValue("source_cache_dir", ui->sourceCacheEdit->text().toStdString());
	mConfig.setValue("build_pkg_type", ui->outputPkgTypeComboBox->currentText().toStdString());
	// Repositories
	vector<string> enabledReps, disabledReps;
	for (int i=0; i<ui->repositoryListWidget->count(); ++i) {
		if (ui->repositoryListWidget->item(i)->checkState()==Qt::Checked) enabledReps.push_back(ui->repositoryListWidget->item(i)->text().toStdString());
		else disabledReps.push_back(ui->repositoryListWidget->item(i)->text().toStdString());
	}
	mpkgconfig::set_repositorylist(enabledReps, disabledReps);
	// Lists
	vector<string> updateBlacklist;
	for (int i=0; i<ui->updateBlacklistListWidget->count(); ++i) {
		updateBlacklist.push_back(ui->updateBlacklistListWidget->item(i)->text().toStdString());
	}
	WriteFileStrings("/etc/mpkg-update-blacklist", updateBlacklist);

	vector<string> removeProtected;
	for (int i=0; i<ui->removeProtectedListWidget->count(); ++i) {
		removeProtected.push_back(ui->removeProtectedListWidget->item(i)->text().toStdString());
	}
	WriteFileStrings("/etc/mpkg-remove-blacklist", removeProtected);
}
void SettingsDialog::addRepository() {
	if (ui->addRepositoryLineEdit->text().isEmpty()) return;
	QListWidgetItem *item = new QListWidgetItem(ui->addRepositoryLineEdit->text());
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
	item->setCheckState(Qt::Checked);
	ui->repositoryListWidget->addItem(item);
	ui->addRepositoryLineEdit->clear();
}
void SettingsDialog::deleteRepository() {
	int id = ui->repositoryListWidget->currentRow();
	if (id<0 || id>=ui->repositoryListWidget->count()) return;
	ui->repositoryListWidget->takeItem(id);
}

void SettingsDialog::getRepositoryList() {
 	actGetRepositorylist();
	load();
}


void SettingsDialog::loadProfile() {
	string pname = ui->repositoryProfilesComboBox->currentText().toStdString();
	if (pname.empty()) return;
	vector<string> repListTmp = ReadFileStrings("/etc/mpkg/profiles/" + pname);
	vector<QString> repList;
	// Validating input
	ui->repositoryListWidget->clear();
	for (unsigned int i=0; i<repListTmp.size(); ++i) {
		if (validateRepStr(repListTmp[i])) {
			QListWidgetItem *item = new QListWidgetItem(QString::fromStdString(repListTmp[i]));
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
			item->setCheckState(Qt::Checked);
			ui->repositoryListWidget->addItem(item);
		}
	}
}
void SettingsDialog::saveProfile() {
	vector<string> rList;
	for (int i=0; i<ui->repositoryListWidget->count(); ++i) {
		rList.push_back(ui->repositoryListWidget->item(i)->text().toStdString());
	}
	QString filename = QInputDialog::getText(this, tr("Save profile as..."), tr("Enter name for new profile:"));
	if (filename.isEmpty()) return;
	system("mkdir -p /etc/mpkg/profiles");
	WriteFileStrings("/etc/mpkg/profiles/"+filename.toStdString(), rList);
	vector<string> pList = getDirectoryList("/etc/mpkg/profiles");
	ui->repositoryProfilesComboBox->clear();
	for (unsigned int i=0; i<pList.size(); ++i) {
		ui->repositoryProfilesComboBox->addItem(QString::fromStdString(pList[i]));
	}

}
void SettingsDialog::deleteProfile() {
	string pname = ui->repositoryProfilesComboBox->currentText().toStdString();
	if (pname.empty()) return;
	unlink(string("/etc/mpkg/profiles/" + pname).c_str());
	vector<string> pList = getDirectoryList("/etc/mpkg/profiles");
	ui->repositoryProfilesComboBox->clear();
	for (unsigned int i=0; i<pList.size(); ++i) {
		ui->repositoryProfilesComboBox->addItem(QString::fromStdString(pList[i]));
	}


}

