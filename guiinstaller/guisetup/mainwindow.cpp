#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QCheckBox>
#include <QMessageBox>
#include <QSettings>
#include <QListWidgetItem>
#include "help.h"
MainWindow *guiObject;


MpkgErrorReturn qtErrorHandler(ErrorDescription err, const string& details) {
	return guiObject->errorHandler(err, details);
}

MpkgErrorReturn MainWindow::errorHandler(ErrorDescription err, const string& details) {
	QMessageBox box(this);
	QVector<QPushButton *> buttons;
	if (err.action.size()>1) {
		for (size_t i=0; i<err.action.size(); ++i) {
			buttons.push_back(box.addButton(err.action[i].text.c_str(), QMessageBox::AcceptRole));
		}
	}
	box.setWindowTitle(tr("Error"));
	box.setText(err.text.c_str());
	box.setInformativeText(details.c_str());
	box.exec();
	if (err.action.size()==1) return err.action[0].ret;
	for (int i=0; i<buttons.size(); ++i) {
		if (box.clickedButton()==buttons[i]) return err.action[i].ret;
	}
	return MPKG_RETURN_ABORT;
}

MountOptions::MountOptions(QTreeWidgetItem *_item, QString _partition, QString _size, QString _currentfs, QString _mountpoint, bool _format, QString _newfs) {
	itemPtr = _item;
	partition = _partition;
	size = _size;
	currentfs = _currentfs;
	mountpoint = _mountpoint;
	format = _format;
	newfs = _newfs;
	printf("Creating object with data: %s %s %d %s\n", partition.toStdString().c_str(), mountpoint.toStdString().c_str(), format, newfs.toStdString().c_str());
	setupMode = true;
}

MountOptions::~MountOptions() {
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindowClass) {
	hasNvidia = -1;
	mpkgErrorHandler.registerErrorHandler(qtErrorHandler);
	guiObject = this;
	ui->setupUi(this);
	ui->releaseNotesTextBrowser->hide();
	ui->sendStatCheckBox->hide();
	setWindowState(Qt::WindowMaximized);
	connect(ui->nextButton, SIGNAL(clicked()), this, SLOT(nextButtonClick()));
	connect(ui->backButton, SIGNAL(clicked()), this, SLOT(backButtonClick()));
	connect(ui->mountPointsTreeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT(updateMountsGUI(QTreeWidgetItem *, QTreeWidgetItem *)));
	connect(ui->installButton, SIGNAL(clicked()), this, SLOT(runInstaller()));

	connect(ui->mountDoFormatRadioButton, SIGNAL(toggled(bool)), this, SLOT(updateMountItemUI()));
	connect(ui->mountDontUseRadioButton, SIGNAL(toggled(bool)), this, SLOT(updateMountItemUI()));
	connect(ui->mountRootRadioButton, SIGNAL(toggled(bool)), this, SLOT(updateMountItemUI()));
	connect(ui->mountSwapRadioButton, SIGNAL(toggled(bool)), this, SLOT(updateMountItemUI()));
	connect(ui->mountNoFormatRadioButton, SIGNAL(toggled(bool)), this, SLOT(updateMountItemUI()));
	connect(ui->mountCustomRadioButton, SIGNAL(toggled(bool)), this, SLOT(updateMountItemUI()));
	connect(ui->mountFormatOptionsComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateMountItemUI()));
	connect(ui->mountPointEdit, SIGNAL(textEdited(const QString &)), this, SLOT(updateMountItemUI()));
	connect(ui->releaseNotesButton, SIGNAL(clicked()), this, SLOT(showHideReleaseNotes()));
	connect(ui->helpButton, SIGNAL(clicked()), this, SLOT(showHelp()));
	lockUIUpdate = false;
	connect(ui->timezoneSearchEdit, SIGNAL(textEdited(const QString &)), this, SLOT(timezoneSearch(const QString &)));
	ui->stackedWidget->setCurrentIndex(0);
	settings = new QSettings("guiinstaller");
	loadSetupVariantsThread = new LoadSetupVariantsThread;
	connect(loadSetupVariantsThread, SIGNAL(finished(bool)), this, SLOT(receiveLoadSetupVariants(bool)));
	connect(ui->quitButton, SIGNAL(clicked()), this, SLOT(askQuit()));

}

MainWindow::~MainWindow() {
	delete settings;
	delete loadSetupVariantsThread;
}

void MainWindow::showHelp() {
	QString text = ReadFile("/usr/share/setup/help/" + IntToStr(ui->stackedWidget->currentIndex()) + ".html").c_str();
	if (text.isEmpty()) {
		QMessageBox::information(this, tr("No help available"), tr("Sorry, no help available for this part"));
		return;
	}
	HelpForm *helpForm = new HelpForm;
	helpForm->loadText(text);
	helpForm->show();

}

void MainWindow::askQuit() {
	if (QMessageBox::question(this, tr("Really cancel installation?"), tr("Installation is not yet complete. Do you really want to cancel?"), QMessageBox::Yes|QMessageBox::No)==QMessageBox::Yes) qApp->quit();
}
void MainWindow::runInstaller() {
	if (!ui->confirmInstallCheckBox->isChecked()) {
		QMessageBox::critical(this, tr("Please confirm settings"), tr("You chould confirm that settings are OK. Check the appropriate check box."));
		return;
	}
	if (ui->sendStatCheckBox->isChecked()) {
		settings->setValue("anonstat", true);
	}
	else settings->setValue("anonstat", false);
	system("LC_ALL=" + settings->value("language").toString().toStdString() + " nohup guisetup_exec 2>&1 >/var/log/guisetup_exec.log &");
	qApp->quit();
}

void MainWindow::nextButtonClick() {
	if (!validatePageSettings(ui->stackedWidget->currentIndex())) return;
	int shift = 1;
	while (!checkLoad(ui->stackedWidget->currentIndex()+shift)) shift++;
	storePageSettings(ui->stackedWidget->currentIndex());
	if (ui->stackedWidget->currentIndex()==ui->stackedWidget->count()-1-shift) ui->nextButton->setEnabled(false);
	else if (ui->stackedWidget->currentIndex()==0) ui->backButton->setEnabled(true);
	ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex()+shift);
	ui->statusbar->showMessage(QString("Step %1 of %2").arg(ui->stackedWidget->currentIndex()).arg(ui->stackedWidget->count()-1));
	updatePageData(ui->stackedWidget->currentIndex());
}

void MainWindow::backButtonClick() {
	if (ui->stackedWidget->currentIndex()==1) ui->backButton->setEnabled(false);
	else if (ui->stackedWidget->currentIndex()==ui->stackedWidget->count()-1) ui->nextButton->setEnabled(true);
	int shift = 1;
	if (ui->stackedWidget->currentIndex()==PAGE_INSTALLTYPE) shift=2;
	while (!checkLoad(ui->stackedWidget->currentIndex()-shift)) shift--;
	ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex()-shift);
	ui->statusbar->showMessage(QString("Step %1 of %2").arg(ui->stackedWidget->currentIndex()).arg(ui->stackedWidget->count()-1));
	//updatePageData(ui->stackedWidget->currentIndex());

}

bool MainWindow::validatePageSettings(int index) {
	switch(index) {
		case PAGE_LICENSE:
			if (!ui->licenseAcceptCheckBox->isChecked()) {
				QMessageBox::critical(this, tr("License not accepted"), tr("Without accepting license, you cannot proceed."));
				return false;
			}
			break;
		case PAGE_PARTITIONING:
			if (ui->partitioningManualRadioButton->isChecked()) runPartitioningTool();
			updatePartitionLists();
			if (drives.empty()) {
				QMessageBox::critical(this, tr("No hard drives detected"), tr("Installer cannot detect any hard drives in your computer. It may be caused by:\n - You really didn't have any hard drives,\n - Sort of hardware problems (check cables first),\n - Invalid BIOS configuration (check that your drive controllers is enabled),\n - currently unsupported hardware (in this case, please report to developers).\n\nInstallation cannot continue and will exit now."));
				qApp->quit();
			}
			if (partitions.empty()) {
				QMessageBox::critical(this, tr("No partitions detected"), tr("You have no partitions on your hard drive. Please create it first."));
				return false;
			}
			break;
		case PAGE_MOUNTPOINTS:
			if (!validateMountPoints()) return false;
			break;
		case PAGE_BOOTLOADER:
			if (!validateBootloaderSettings()) return false;
			break;
		case PAGE_PKGSOURCE:
			if (!validatePkgSources()) return false;
			break;
		case PAGE_ROOTPASSWORD:
			if (!validateRootPassword()) return false;
			break;
		case PAGE_USERS:
			if (!validateUserPasswords()) return false;
			break;
		case PAGE_TIMEZONE:
			if (!ui->timezoneListWidget->currentItem()) {
				QMessageBox::critical(this, tr("Timezone not selected"), tr("Please select timezone"));
				return false;
			}
			break;
		default:
			break;
	}
	return true;


}

void MainWindow::storePageSettings(int index) {
	switch(index) {
		case PAGE_LICENSE:
			settings->setValue("license_accepted", true);
			break;
		case PAGE_PARTITIONING:
			break;
		case PAGE_MOUNTPOINTS:
			saveMountSettings();
			break;
		case PAGE_BOOTLOADER:
			saveBootloaderSettings();
			break;
		case PAGE_PKGSOURCE:
			savePkgsourceSettings();
			break;
		case PAGE_INSTALLTYPE:
			saveSetupVariant();
			break;
		case PAGE_ALTERNATIVES:
			saveAlternatives();
			break;
		case PAGE_NVIDIA:
			saveNvidia();
			break;
		case PAGE_TIMEZONE:
			saveTimezone();
			break;
		case PAGE_ROOTPASSWORD:
			saveRootPassword();
			break;
		case PAGE_NETWORKING:
			saveNetworking();
			break;
		case PAGE_USERS:
			saveUsers();
			break;
		default:
			break;
	}
	settings->sync();

}

void MainWindow::updatePageData(int index) {
	switch(index) {
		case PAGE_LICENSE:
			loadLicense();
			break;
		case PAGE_PARTITIONING:
			loadPartitioningDriveList();
			break;
		case PAGE_MOUNTPOINTS:
			loadMountsTree();
			break;
		case PAGE_BOOTLOADER:
			loadBootloaderTree();
			break;
		case PAGE_PKGSOURCE:
			break;
		case PAGE_WAITPKGSOURCE:
			loadSetupVariants();
			break;
		case PAGE_TIMEZONE:
			loadTimezones();
			break;
		case PAGE_CONFIRMATION:
			loadConfirmationData();
		default:
			break;
	}
	settings->sync();
}

void MainWindow::loadLicense() {
	QString lang = settings->value("language").toString();
	QString license = ReadFile("licenses/license." + lang.toStdString()).c_str();
	ui->licenseBrowser->setPlainText(license);
}

void MainWindow::updatePartitionLists() {
	drives = getDevList();
	partitions = getPartitionList();
}

void MainWindow::loadPartitioningDriveList() {
	updatePartitionLists();
	ui->autoPartitionDriveComboBox->clear();
	for (size_t i=0; i<drives.size(); ++i) {
		ui->autoPartitionDriveComboBox->addItem(QString("%1 (%2)").arg(QString::fromStdString(drives[i].tag)).arg(QString::fromStdString(drives[i].value)));
	}

}

void MainWindow::runPartitioningTool() {
	Qt::WindowStates winstate = windowState();
	setWindowState(Qt::WindowMinimized);
	system("gparted");
	setWindowState(Qt::WindowMaximized);
	deviceCacheActual = false;
	partitionCacheActual = false;
	updatePartitionLists();
}

void MainWindow::loadMountsTree() {
	updatePartitionLists();
	ui->mountPointsTreeWidget->clear();
	mountOptions.clear();
	for (size_t i=0; i<drives.size(); ++i) {
		QTreeWidgetItem *coreItem = new QTreeWidgetItem(ui->mountPointsTreeWidget, QStringList(QString("%1 (%2)").arg(QString::fromStdString(drives[i].tag)).arg(QString::fromStdString(drives[i].value))));
		coreItem->setExpanded(true);
		for (size_t p=0; p<partitions.size(); ++p) {
			if (partitions[p].devname.find(drives[i].tag)!=0) continue;
			QTreeWidgetItem *partitionItem = new QTreeWidgetItem(coreItem, QStringList(QString("%1 (%2, %3)").arg(QString::fromStdString(partitions[p].devname)).arg(QString::fromStdString(humanizeSize(atol(partitions[p].size.c_str())*1048576))).arg(QString::fromStdString(partitions[p].fstype))));
			mountOptions.push_back(MountOptions(partitionItem, QString::fromStdString(partitions[p].devname), QString::fromStdString(humanizeSize(atol(partitions[p].size.c_str())*1048576)),QString::fromStdString(partitions[p].fstype)));
		}
	}

}

void MainWindow::updateMountItemUI() {
	if (lockUIUpdate) return;
	QTreeWidgetItem *item = ui->mountPointsTreeWidget->currentItem();
	if (!item) return;
	MountOptions *mountPtr = NULL;
	for (size_t i=0; i<mountOptions.size(); ++i) {
		if (item == mountOptions[i].itemPtr) {
			mountPtr = &mountOptions[i];
			break;
		}
	}
	if (!mountPtr) return;

	QString newdata, formatdata;
	processMountEdit(item);
	newdata.clear();
	formatdata.clear();
	if (!mountPtr->mountpoint.isEmpty()) newdata = ": " + mountPtr->mountpoint;
	if (mountPtr->format) formatdata = tr(", format to: %1").arg(mountPtr->newfs);
	item->setText(0, QString("%1 (%2, %3)%4%5").arg(mountPtr->partition).arg(mountPtr->size).arg(mountPtr->currentfs).arg(newdata).arg(formatdata));
	if (mountPtr->mountpoint=="/") item->setBackground(0, QBrush(QColor(186,15,88,127)));
	else if (mountPtr->mountpoint=="swap") item->setBackground(0, QBrush(QColor(215,91,24,127)));
	else if (mountPtr->mountpoint.isEmpty()) item->setBackground(0, QBrush(QColor(255,255,255,0)));
	else if (mountPtr->format) item->setBackground(0, QBrush(QColor(59,74,215,127)));
	else item->setBackground(0, QBrush(QColor(255,0,0,127)));

}

void MainWindow::updateMountsGUI(QTreeWidgetItem *nextItem, QTreeWidgetItem *prevItem) {
	lockUIUpdate = true;
	// Let's debug and print whole table
	MountOptions *prevMountPtr = NULL, *nextMountPtr = NULL;
	for (size_t i=0; i<mountOptions.size(); ++i) {
		if (nextItem == mountOptions[i].itemPtr) {
			nextMountPtr = &mountOptions[i];
			break;
		}
	}
	for (size_t i=0; i<mountOptions.size(); ++i) {
		if (prevItem == mountOptions[i].itemPtr) {
			prevMountPtr = &mountOptions[i];
			break;
		}
	}

	QString newdata, formatdata;
	if (prevMountPtr && prevItem) {
		processMountEdit(prevItem);
		newdata.clear();
		formatdata.clear();
		if (!prevMountPtr->mountpoint.isEmpty()) newdata = ": " + prevMountPtr->mountpoint;
		if (prevMountPtr->format) formatdata = tr(", format to: %1").arg(prevMountPtr->newfs);

		prevItem->setText(0, QString("%1 (%2, %3)%4%5").arg(prevMountPtr->partition).arg(prevMountPtr->size).arg(prevMountPtr->currentfs).arg(newdata).arg(formatdata));
	}
	
	if (nextItem && nextMountPtr) {
		ui->mountPointEdit->setText("");
		if (nextMountPtr->mountpoint=="/") ui->mountRootRadioButton->setChecked(true);
		else if (nextMountPtr->mountpoint.isEmpty()) ui->mountDontUseRadioButton->setChecked(true);
		else if (nextMountPtr->mountpoint=="swap") ui->mountSwapRadioButton->setChecked(true);
		else {
			ui->mountCustomRadioButton->setChecked(true);
			ui->mountPointEdit->setText(nextMountPtr->mountpoint);
		}


		int itemID;
		if (nextMountPtr->format) {
			ui->mountDoFormatRadioButton->setChecked(true);
			itemID = ui->mountFormatOptionsComboBox->findText(nextMountPtr->newfs);
			if (itemID!=-1) ui->mountFormatOptionsComboBox->setCurrentIndex(itemID);
		}
		else {
			ui->mountNoFormatRadioButton->setChecked(true);
		}
	}
	lockUIUpdate = false;
}

void MainWindow::processMountEdit(QTreeWidgetItem *item) {
	if (!item) {
		printf("processMountEdit: NULL ITEM!\n");
		return;
	}
	MountOptions *mountPtr = NULL;
	for (size_t i=0; i<mountOptions.size(); ++i) {
		if (item == mountOptions[i].itemPtr) {
			mountPtr = &mountOptions[i];
			break;
		}
	}
	if (mountPtr==NULL) return;

	if (ui->mountRootRadioButton->isChecked()) mountPtr->mountpoint = "/";
	else if (ui->mountSwapRadioButton->isChecked()) mountPtr->mountpoint = "swap";
	else if (ui->mountDontUseRadioButton->isChecked()) mountPtr->mountpoint.clear();
	else if (ui->mountCustomRadioButton->isChecked()) mountPtr->mountpoint=ui->mountPointEdit->text();

	if (ui->mountNoFormatRadioButton->isChecked()) {
		mountPtr->format=false;
		mountPtr->newfs.clear();
	}
	else {
		mountPtr->format=true;
		mountPtr->newfs=ui->mountFormatOptionsComboBox->currentText();
	}
}

void MainWindow::saveMountSettings() {
	processMountEdit(ui->mountPointsTreeWidget->currentItem());
	settings->beginGroup("mount_options");
	settings->remove("");
	for (size_t i=0; i<mountOptions.size(); ++i) {
		if (mountOptions[i].mountpoint.isEmpty() && !mountOptions[i].format) continue;
		settings->beginGroup(mountOptions[i].partition.toStdString().substr(5).c_str());
		settings->setValue("mountpoint", mountOptions[i].mountpoint);
		settings->setValue("format", mountOptions[i].format);
		if (mountOptions[i].format) settings->setValue("fs", mountOptions[i].newfs);
		else settings->setValue("fs", mountOptions[i].currentfs);
		settings->endGroup();
	}
	settings->endGroup();
}

void MainWindow::loadBootloaderTree() {
	updatePartitionLists();
	ui->bootLoaderComboBox->clear();
	for (size_t i=0; i<drives.size(); ++i) {
		ui->bootLoaderComboBox->addItem(QString("%1 (%2)").arg(QString::fromStdString(drives[i].tag)).arg(QString::fromStdString(drives[i].value)));
	}
}

bool MainWindow::validateBootloaderSettings() {
	if (ui->bootLoaderComboBox->currentIndex()==-1) {
		QMessageBox::critical(this, tr("No bootloader disk selected"), tr("You should specify drive or partition where bootloader will be installed."));
		return false;
	}
	return true;
}

void MainWindow::saveBootloaderSettings() {
	QString fbmode = ui->fbResolutionComboBox->currentText();
	if (fbmode=="KMS" || fbmode == "Kernel modesetting") fbmode = "text";
	if (fbmode!="text") fbmode += "x" + ui->fbColorComboBox->currentText();
	settings->setValue("bootloader", drives[ui->bootLoaderComboBox->currentIndex()].tag.c_str());
	settings->setValue("fbmode", fbmode);
	settings->setValue("initrd_delay", ui->initrdDelayCheckBox->isChecked());
	settings->setValue("kernel_options", ui->kernelOptionsLineEdit->text());
	settings->setValue("initrd_modules", ui->initrdModulesLineEdit->text());
	
}

bool MainWindow::validatePkgSources() {
	if (ui->pkgSourceISORadioButton->isChecked()) {
		isopath = QFileDialog::getOpenFileName(this, tr("Specify ISO image"), "/", tr("ISO image (*.iso)"), 0, QFileDialog::DontUseNativeDialog);
		if (isopath.isEmpty() || !FileExists(isopath.toStdString())) {
			return false;
		}
	}
	printf("Got ISO image in %s\n", isopath.toStdString().c_str());
	return true;
}
void MainWindow::savePkgsourceSettings() {
	if (ui->pkgSourceDVDRadioButton->isChecked()) {
		settings->setValue("pkgsource", "dvd");
	}
	else if (ui->pkgSourceISORadioButton->isChecked()) {
		settings->setValue("pkgsource", QString("iso://%1").arg(isopath));
	}
	else if (ui->pkgSourceNetworkRadioButton->isChecked()) settings->setValue("pkgsource", "http://core64.mopspackages.ru/");
	// TODO: Don't forget about pkgSourceCustomRadioButton!
}




CustomPkgSet MainWindow::getCustomPkgSet(const string& name) {
	vector<string> data = ReadFileStrings("/tmp/setup_variants/" + name + ".desc");
	CustomPkgSet ret;
	ret.name = name;
	string locale = settings->value("language").toString().toStdString();
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

// Get setup variants
void MainWindow::getCustomSetupVariants(const vector<string>& rep_list) {
	string tmpfile = get_tmp_file();
	system("rm -rf /tmp/setup_variants");
	system("mkdir -p /tmp/setup_variants");
	string path;
	customPkgSetList.clear();
	for (size_t z=0; z<rep_list.size(); ++z) {
		path = rep_list[z];
		CommonGetFile(path + "/setup_variants.list", tmpfile);
		vector<string> list = ReadFileStrings(tmpfile);
		vector<CustomPkgSet> ret;
		for (size_t i=0; i<list.size(); ++i) {
			CommonGetFile(path + "/setup_variants/" + list[i] + ".desc", "/tmp/setup_variants/" + list[i] + ".desc");
			CommonGetFile(path + "/setup_variants/" + list[i] + ".list", "/tmp/setup_variants/" + list[i] + ".list");
			customPkgSetList.push_back(getCustomPkgSet(list[i]));
		}
	}
}


void MainWindow::loadSetupVariants() {
	// Let's write down repo list
	if (loadSetupVariantsThread->isRunning()) {
		if (!loadSetupVariantsThread->wait(5000)) loadSetupVariantsThread->terminate();
	}
	loadSetupVariantsThread->pkgsource = settings->value("pkgsource").toString();
	//loadSetupVariantsThread->volname = volname;
	//loadSetupVariantsThread->rep_location = rep_location;
	loadSetupVariantsThread->start();
	ui->nextButton->setEnabled(false);
}

void MainWindow::receiveLoadSetupVariants(bool success) {
	if (!success) {
		if (settings->value("pkgsource")=="dvd") {
			if (QMessageBox::question(this, tr("DVD detection failed"), tr("Failed to detect DVD drive. Be sure you inserted installation DVD into this. Retry?"), QMessageBox::Yes|QMessageBox::No)==QMessageBox::Yes) {
				loadSetupVariants();
				return;
			}
			else qApp->quit();
		}
		else {
			if (QMessageBox::question(this, tr("Repository connection failed"), tr("Failed to connect to repository. If you trying to access network repository, check your network settings. Retry?"), QMessageBox::Yes|QMessageBox::No)==QMessageBox::Yes) {
				loadSetupVariants();
				return;
			}
			else qApp->quit();


		}
	}
	settings->setValue("volname", loadSetupVariantsThread->volname.c_str());
	settings->setValue("rep_location", loadSetupVariantsThread->rep_location.c_str());

	mpkg *core = new mpkg(false);
	vector<string> rList = core->get_repositorylist();
	getCustomSetupVariants(rList);
	ui->setupVariantsListWidget->clear();
	QListWidgetItem *item;
	for (size_t i=0; i<customPkgSetList.size(); ++i) {
		item = new QListWidgetItem(customPkgSetList[i].desc.c_str(), ui->setupVariantsListWidget);
	}
	delete core;
	ui->nextButton->setEnabled(true);
	nextButtonClick();
}

void MainWindow::loadTimezones() {
	string tmpfile = get_tmp_file();
	system("( cd /usr/share/zoneinfo && find . -type f | sed -e 's/\\.\\///g') > " + tmpfile);
	vector<string> tz = ReadFileStrings(tmpfile);
	unlink(tmpfile.c_str());
	ui->timezoneListWidget->clear();
	QListWidgetItem *item;
	for (size_t i=0; i<tz.size(); ++i) {
		item = new QListWidgetItem(tz[i].c_str(), ui->timezoneListWidget);
	}
}

void MainWindow::saveSetupVariant() {
	int index = ui->setupVariantsListWidget->currentRow();
	if (index!=-1) settings->setValue("setup_variant", customPkgSetList[index].name.c_str());
}

void MainWindow::saveTimezone() {
	settings->setValue("timezone", ui->timezoneListWidget->currentItem()->text());
	if (ui->utcCheckBox->isChecked()) settings->setValue("time_utc", true);
	else settings->setValue("time_utc", false);
}

void MainWindow::loadConfirmationData() {
	QString newdata, formatdata;
	
	QString formatted, notFormatted;
	MountOptions *mountPtr = NULL;
	for (size_t i=0; i<mountOptions.size(); ++i) {
		mountPtr = &mountOptions[i];
		newdata.clear();
		formatdata.clear();
		if (!mountPtr->mountpoint.isEmpty()) newdata = ": " + mountPtr->mountpoint;
		if (mountPtr->format) {
			formatdata = tr(", format to: %1").arg(mountPtr->newfs);
			formatted += QString("<li><u>%1</u> (%2, %3)%4%5</li>\n").arg(mountPtr->partition).arg(mountPtr->size).arg(mountPtr->currentfs).arg(newdata).arg(formatdata);
		}
		else notFormatted += QString("<li><u>%1</u> (%2, %3)%4%5</li>\n").arg(mountPtr->partition).arg(mountPtr->size).arg(mountPtr->currentfs).arg(newdata).arg(formatdata);
	}
	ui->confirmationTextBrowser->setText(tr("<p><b>Package source:</b> %1</p>\
				<p><b>Installation type:</b> %2</p>\
				<p><b>Partitions that will be FORMATTED:</b><ul>%3</ul></p>\
				<p><b>Partitions that will NOT be formatted but used:</b><ul>%4</ul></p>\
				<p><b>Boot loader will be installed to:</b> %5</p>").\
			arg(settings->value("pkgsource").toString()).\
			arg(settings->value("setup_variant").toString()).\
			arg(formatted).arg(notFormatted).\
			arg(settings->value("bootloader").toString()));
}

void MainWindow::saveRootPassword() {
	settings->setValue("rootpasswd", ui->rootPasswordEdit->text());
}

void MainWindow::saveUsers() {
	settings->beginGroup("users");
	settings->setValue(ui->usernameEdit->text(), ui->userPasswordEdit->text());
	settings->endGroup();
}

bool MainWindow::validateRootPassword() {
	if (ui->rootPasswordEdit->text()!=ui->rootPasswordVerifyEdit->text()) {
		QMessageBox::information(this, tr("Verification failed"), tr("Passwords doesn't match. Please enter it more carefully."));
		ui->rootPasswordVerifyEdit->clear();
		ui->rootPasswordEdit->clear();
		return false;
	}
	if (ui->rootPasswordEdit->text().isEmpty()) {
		QMessageBox::information(this, tr("Empty password"), tr("Root password cannot be empty, it is very insecure!"));
		return false;
	}
	return true;

}

bool MainWindow::validateUserPasswords() {
	if (ui->usernameEdit->text().isEmpty()) {
		QMessageBox::information(this, tr("Username not specified"), tr("Please, enter username"));
		return false;
	}
	if (ui->usernameEdit->text().toStdString().find_first_of("0123456789")==0) {
		QMessageBox::information(this, tr("Invalid username"), tr("Username cannot start from number. Please specify correct username."));
		return false;
	}
	if (ui->usernameEdit->text()=="root") {
		QMessageBox::information(this, tr("Invalid username"), tr("User root is an administrative account and already exists, please specify another name"));
		return false;
	}
	if (ui->usernameEdit->text().toStdString().find_first_not_of("abcdefghigklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_-")==0) {
		QMessageBox::information(this, tr("Invalid username"), tr("Username cannot start from number. Please specify correct username."));
		return false;
	}
	if (ui->userPasswordEdit->text()!=ui->userPasswordVerifyEdit->text()) {
		QMessageBox::information(this, tr("Verification failed"), tr("Passwords doesn't match. Please enter it more carefully."));
		ui->userPasswordVerifyEdit->clear();
		ui->userPasswordEdit->clear();
		return false;
	}
	if (ui->userPasswordEdit->text().isEmpty()) {
		QMessageBox::information(this, tr("Empty password"), tr("You cannot create user with no password. If you still want this, you can change password after installation."));
		return false;
	}
	return true;
}



void MainWindow::timezoneSearch(const QString &search) {
	for (int i=0; i<ui->timezoneListWidget->count(); ++i) {
		if (search.isEmpty()) {
			ui->timezoneListWidget->item(i)->setHidden(false);
			continue;
		}
		if (ui->timezoneListWidget->item(i)->text().contains(search, Qt::CaseInsensitive)) ui->timezoneListWidget->item(i)->setHidden(false);
		else ui->timezoneListWidget->item(i)->setHidden(true);
	}
}

bool MainWindow::validateMountPoints() {
	bool hasRoot = false, hasSwap = false;
	// Check for nessecary partitions
	for (size_t i=0; i<mountOptions.size(); ++i) {
		if (mountOptions[i].mountpoint == "swap") hasSwap = true;
		if (mountOptions[i].mountpoint == "/") hasRoot = true;
	}
	if (!hasSwap && QMessageBox::question(this, tr("No swap partition"), tr("You didn't specified swap partition. It is OK for systems with lots of RAM (2Gb or more), but you will be unable to use suspend-to-disk. Are you sure?"), QMessageBox::Yes|QMessageBox::No)!=QMessageBox::Yes) return false;
	if (!hasRoot) {
		QMessageBox::warning(this, tr("No root partition"), tr("You didn't specified root partition. Without this, system cannot be installed."));
		return false;
	}

	// Check for dupes
	bool dupesFound = false;
	for (size_t i=0; i<mountOptions.size(); ++i) {
		for (size_t t=0; t<mountOptions.size(); ++t) {
			if (i==t) continue;
			if (mountOptions[i].mountpoint.isEmpty()) continue;
			if (mountOptions[i].mountpoint == mountOptions[t].mountpoint) {
				QMessageBox::warning(this, tr("Duplicate mount points found"), tr("Duplicate mount points found: %1").arg(mountOptions[i].mountpoint));
				dupesFound = true;
			}
		}
	}
	if (dupesFound) return false;

	return true;
}

bool MainWindow::validateNetworking() {
	if (ui->hostnameEdit->text().isEmpty()) {
		QMessageBox::information(this, tr("Network settings error"), tr("Please specify hostname"));
		return false;
	}

	if (ui->netnameEdit->text().isEmpty()) {
		QMessageBox::information(this, tr("Network settings error"), tr("Please specify network name"));
		return false;
	}
	return true;
}

void MainWindow::saveNetworking() {
	settings->setValue("hostname", ui->hostnameEdit->text());
	settings->setValue("netname", ui->netnameEdit->text());
	if (ui->netNMRadioButton->isChecked()) settings->setValue("netman", "networkmanager");
	else if (ui->netWicdRadioButton->isChecked()) settings->setValue("netman", "wicd");
	else if (ui->netNetconfigRadioButton->isChecked()) settings->setValue("netman", "netconfig");
}

bool MainWindow::checkLoad(int page) {
	switch(page) {
		case PAGE_NVIDIA:
			return checkNvidiaLoad();
			break;
		default: 
			return true;
	}
}

bool MainWindow::checkNvidiaLoad() {
	if (hasNvidia==-1) {
		string tmp_hw = get_tmp_file();
		system("lspci > " + tmp_hw);
		vector<string> hw = ReadFileStrings(tmp_hw);
		for(size_t i=0; i<hw.size(); ++i) {
			if (hw[i].find("VGA compatible controller")!=std::string::npos && hw[i].find("nVidia")!=std::string::npos) hasNvidia = i;
		}
		if (hasNvidia == -1) hasNvidia = 0;
	}
	if (!hasNvidia) {
		return false;
	}
	else {
		return true;
	}
}

void MainWindow::saveNvidia() {
	if (ui->nvidia190RadioButton->isChecked()) settings->setValue("nvidia-driver", "190");
	else if (ui->nvidia173RadioButton->isChecked()) settings->setValue("nvidia-driver", "173");
	else if (ui->nvidia96RadioButton->isChecked()) settings->setValue("nvidia-driver", "96");
	else if (ui->nvidiaNVRadioButton->isChecked()) settings->setValue("nvidia-driver", "nv");
}

void MainWindow::saveAlternatives() {
	QString alternatives;
	settings->remove("alternatives");
	if (ui->altBFSCheckBox->isChecked()) settings->setValue("alternatives/bfs", true);
	if (ui->altCleartypeCheckBox->isChecked()) settings->setValue("alternatives/cleartype", true);
}

void MainWindow::showHideReleaseNotes() {
	if (ui->releaseNotesTextBrowser->isVisible()) {
		ui->releaseNotesButton->setText(tr("Show release notes"));
		ui->releaseNotesTextBrowser->hide();
	}
	else {
		ui->releaseNotesButton->setText(tr("Hide release notes"));
		ui->releaseNotesTextBrowser->show();
	}

}
