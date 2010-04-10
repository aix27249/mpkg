#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "engine.h"
#include "commitdlg.h"
#include "settings.h"
#include "progresswidget.h"
#include <QCheckBox>
#include <QSettings>
#include <QMessageBox>
#include <QListWidgetItem>
#include <mpkg/menu.h>
#define ICON_PREFIX QString(INSTALL_PREFIX) + QString("/share/mpkg/icons/")
void MainWindow::loadPixmapList() {
	pixmapList.push_back(QPixmap(ICON_PREFIX+"unknown.png"));
	pixmapList.push_back(QPixmap(ICON_PREFIX+"installed.png"));
	pixmapList.push_back(QPixmap(ICON_PREFIX+"install.png"));
	pixmapList.push_back(QPixmap(ICON_PREFIX+"available.png"));
	pixmapList.push_back(QPixmap(ICON_PREFIX+"remove.png"));
	pixmapList.push_back(QPixmap(ICON_PREFIX+"deprecated_installed.png"));
	pixmapList.push_back(QPixmap(ICON_PREFIX+"deprecated_available.png"));
	pixmapList.push_back(QPixmap(ICON_PREFIX+"update.png"));

}
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindowClass) {
	slotsConnected=false;
	ui->setupUi(this);
	loadSettings();
	if (!lockDatabase()) {
		QMessageBox::critical(this, tr("Error locking database"), tr("Cannot lock database because it is locked by another process"), QMessageBox::Ok);
		qApp->quit();
	}
	core = new mpkg;
	loadTagsThread = new LoadTags(core);
	loadPackagesThread = new LoadPackages(core);
	ui->packagesListWidget->setEnabled(false);
	ui->tagsListWidget->setEnabled(false);
	commitActionsThread=NULL;
	if (pixmapList.empty()) loadPixmapList();
	connect(ui->actionReset_queue, SIGNAL(triggered()), this, SLOT(resetQueue()));
	connect(ui->actionClean_cache, SIGNAL(triggered()), this, SLOT(cleanCache()));
	connect(ui->clearSearchButton, SIGNAL(clicked()), ui->searchLineEdit, SLOT(clear()));
	connect(ui->actionHide_deprecated, SIGNAL(triggered()), this, SLOT(applySearchFilter()));
	connect(ui->actionHide_installed, SIGNAL(triggered()), this, SLOT(applySearchFilter()));
	connect(ui->actionHide_not_installed, SIGNAL(triggered()), this, SLOT(applySearchFilter()));
	connect(ui->actionCommit_actions, SIGNAL(triggered()), this, SLOT(requestCommit()));
	connect(ui->actionGet_repository_list, SIGNAL(triggered()), this, SLOT(getRepositoryList()));
	connectThreadSlots();
	loadTagsThread->start();
	progressWidgetPtr = new ProgressWidget;
	pData.registerEventHandler(&updateProgressData);


}

MainWindow::~MainWindow() {
	saveSettings();
	delete ui;
	delete core;
	unlockDatabase();
}

void MainWindow::getRepositoryList() {
 	actGetRepositorylist();
	updateRepositoryData();
}

void MainWindow::quit() {
	printf("Quit\n");
	qApp->quit();
}
void MainWindow::saveSettings() {
	QSettings settings("RpuNet", "mpkgmanager");
	settings.setValue("state", this->saveState());
	settings.setValue("geometry", this->saveGeometry());
}

void MainWindow::loadSettings() {
	QSettings settings("RpuNet", "mpkgmanager");
	this->restoreGeometry(settings.value("geometry").toByteArray());
	this->restoreState(settings.value("state").toByteArray());
}

void MainWindow::showSettings() {
	settingsDialog = new SettingsDialog(this);
	settingsDialog->show();
}

void MainWindow::connectThreadSlots() {
	connect(loadTagsThread, SIGNAL(done()), this, SLOT(tagsLoadingComplete()));
	connect(loadPackagesThread, SIGNAL(done()), this, SLOT(packagesLoadingComplete()));
}
void MainWindow::connectSlots() {
	if (slotsConnected) return;
	connect(ui->tagsListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(receiveCategoryFilter(QListWidgetItem *, QListWidgetItem *)));
	connect(ui->actionUpdate_data, SIGNAL(triggered()), this, SLOT(updateRepositoryData()));
	connect(ui->commitButton, SIGNAL(clicked()), this, SLOT(requestCommit()));
	connect(ui->searchLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(applySearchFilter()));
	connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(quit()));
	connect(ui->actionReload_package_list, SIGNAL(triggered()), this, SLOT(loadPackageList()));
	connect(ui->actionSettings, SIGNAL(triggered()), this, SLOT(showSettings()));
	connect(ui->packagesListWidget, SIGNAL(currentRowChanged(int)), this, SLOT(showPackageInfo(int)));
	connect(ui->markAllVisibleButton, SIGNAL(clicked()), this, SLOT(markAllVisible()));
	connect(ui->unmarkAllVisibleButton, SIGNAL(clicked()), this, SLOT(unmarkAllVisible()));
	slotsConnected=true;
}

void MainWindow::disconnectSlots() {
	return;
	disconnect(ui->tagsListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)));
	disconnect(ui->actionShow_queue, SIGNAL(triggered()));
	disconnect(ui->actionUpdate_data, SIGNAL(triggered()));
	disconnect(ui->commitButton, SIGNAL(clicked()));
}


void MainWindow::tagsLoadingComplete() {
	//printf(">>>>>>>>>>>>>>>>>>>> %s <<<<<<<<<<<<<<<<<<<<\n", __func__);
	ui->tagsListWidget->clear();
	ui->tagsListWidget->addItem(QString("all"));
	ui->tagsListWidget->addItem(QString("updates"));
	ui->tagsListWidget->addItem(QString("not installed"));
	ui->tagsListWidget->addItems(loadTagsThread->getTagList());
	disconnect(loadTagsThread, SIGNAL(done()));
	loadTagsThread->wait();
	delete loadTagsThread;
	ui->statusbar->showMessage(tr("Fetching package list"));
	//printf("PKGLIST FETCH\n");
	loadPackagesThread->start();
	//printf("GOTO!\n");
}
int MainWindow::getIconState(const PACKAGE& p) {
	int lastVersionIndex, installedVersionIndex;
	const PACKAGE* lastVersionPtr;
       	const PACKAGE* installedVersionPtr;

	lastVersionPtr = NULL;
	installedVersionPtr = NULL;
	lastVersionIndex=globalPkgList.getMaxVersionNumber(p.get_name());
	installedVersionIndex=globalPkgList.getInstalledVersionNumber(p.get_name());
	if (lastVersionIndex!=MPKGERROR_NOPACKAGE) lastVersionPtr = &globalPkgList[lastVersionIndex];
	if (installedVersionIndex!=MPKGERROR_NOPACKAGE) installedVersionPtr = &globalPkgList[installedVersionIndex];

	int state;
	if (p.installed()) {
		if (!lastVersionPtr || lastVersionPtr==&p) {
			state = ICONSTATE_INSTALLED;
		}
		else {
			state = ICONSTATE_INSTALLED_DEPRECATED;
		}
	}
	else if (p.available()) {
		if (!lastVersionPtr || lastVersionPtr==&p) {
			if (installedVersionPtr) state=ICONSTATE_UPDATE;
			else state = ICONSTATE_AVAILABLE;
		}
		else {
			state = ICONSTATE_AVAILABLE_DEPRECATED;
		}
	}
	else {
		state = ICONSTATE_UNKNOWN;
	}
	return state;

}
QListWidgetItem * MainWindow::createItemForPkg(const PACKAGE& p) {
	QListWidgetItem *item = new QListWidgetItem(QIcon(pixmapList[getIconState(p)]), QString::fromStdString(p.get_name() + " " + p.get_fullversion() + ": " + p.get_short_description()));
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
	if (p.installed()) item->setCheckState(Qt::Checked);
	else item->setCheckState(Qt::Unchecked);

	return item;
}
void MainWindow::packagesLoadingComplete() {
	disconnect(loadPackagesThread, SIGNAL(done()));
	globalPkgList=loadPackagesThread->getPackageList();
	loadPackagesThread->wait();
	delete loadPackagesThread;

	ui->statusbar->showMessage(tr("Loading package table"));
	ui->packagesListWidget->clear();

	for (unsigned int i=0; i<globalPkgList.size(); ++i) {
		ui->packagesListWidget->addItem(createItemForPkg(globalPkgList[i]));
	}
	ui->statusbar->clearMessage();
	ui->packagesListWidget->setEnabled(true);
	ui->tagsListWidget->setEnabled(true);
	connectSlots();
	applySearchFilter();
}
void MainWindow::showPackageInfo(int row) {
	if (row < 0 || row >= ui->packagesListWidget->count()) return;
	const PACKAGE *p = &globalPkgList[row];
	QString linkList;
	for (unsigned int i=0; i<p->get_locations().size(); ++i) {
		linkList += "<a href=\"" + QString::fromStdString(p->get_locations().at(i).get_full_url() + p->get_filename()) + "\">" + QString::fromStdString(p->get_filename()) + "</a><br>";
	}
	QString tagList;
	for (unsigned int i=0; i<p->get_tags().size(); ++i) {
		tagList += QString::fromStdString(p->get_tags().at(i));
	       	if (i<p->get_tags().size()-1) tagList += "<br>";
	}
	ui->dependencyListWidget->clear();
	for (unsigned int i=0; i<p->get_dependencies().size(); ++i) {
		ui->dependencyListWidget->addItem(QString::fromStdString(p->get_dependencies().at(i).getDepInfo()));
	}



	if (!tagList.isEmpty()) tagList = "<hr><big><b>" + tr("Tags") + ":</b></big><br>" + tagList;
	ui->packageInfoLabel->setText(QString::fromStdString("<b><big>" + p->get_name() + " " + p->get_fullversion() + "</b></big><br>" + p->get_short_description() + "<hr>" + p->get_description() + \
				"<hr><b>") + tr("Package size: ") + "</b>" + QString::fromStdString(humanizeSize(p->get_compressed_size())) + \
			"<br><b>" + tr("Installed size: ") + "</b>" + QString::fromStdString(humanizeSize(p->get_installed_size())) + \
			"<br><b>" + tr("Branch: ") + "</b>" + QString::fromStdString(p->get_repository_tags()) + \
			"<br><b>" + tr("Distrib: ") + "</b>" + QString::fromStdString(p->package_distro_version) + \
			"<br><b>" + tr("File name: ") + "</b>" + QString::fromStdString(p->get_filename()) + \
			"<br><b>MD5: </b>" + QString::fromStdString(p->get_md5()) + 
			tagList + "<hr><big><b>" + tr("Download links:") + "</b></big><br>" + linkList);
}

void MainWindow::receiveCategoryFilter(QListWidgetItem *current, QListWidgetItem *) {
	if (!current) return;
	//if (!loadPackagesThread) return;
	ui->statusbar->showMessage(current->text());
	bool showAll=false;
	if (current->text()==QString("all")) showAll=true;
	bool showThis;
	bool onlyAvailable=false;
	bool onlyUpdates=false;
	vector<string> blacklist = ReadFileStrings("/etc/mpkg-update-blacklist");
	if (current->text()==QString("not installed")) onlyAvailable=true;
	if (current->text()==QString("updates")) onlyUpdates=true;
	for (unsigned int i=0; i<globalPkgList.size(); ++i) {
		showThis=false;
		if (showAll) showThis=true;
		else if (onlyAvailable) {
			if (!globalPkgList[i].installed() && globalPkgList[i].available() && globalPkgList.getInstalledVersionNumber(globalPkgList[i].get_name())==MPKGERROR_NOPACKAGE) showThis=true;
		}
		else if (onlyUpdates) {
			if (globalPkgList.getMaxVersionNumber(globalPkgList[i].get_name())==(int) i && globalPkgList.getInstalledVersionNumber(globalPkgList[i].get_name())!=MPKGERROR_NOPACKAGE && globalPkgList.getInstalledVersionNumber(globalPkgList[i].get_name()) != (int) i) showThis=true;
			for (unsigned int t=0; t<blacklist.size(); ++t) {
				if (blacklist[t]==globalPkgList[i].get_name()) {
					showThis=false;
					break;
				}
			}
		}
		else if (globalPkgList[i].isTaggedBy(current->text().toStdString())) showThis=true;
		if (!checkSearchFilter(QString::fromStdString(globalPkgList[i].get_name()))) showThis=false;
		if (ui->actionHide_deprecated->isChecked() && globalPkgList[i].deprecated() && !globalPkgList[i].installed()) showThis=false;
		if (ui->actionHide_installed->isChecked() && globalPkgList[i].installed()) showThis=false;
		if (ui->actionHide_not_installed->isChecked() && !globalPkgList[i].installed()) showThis=false;

		ui->packagesListWidget->item(i)->setHidden(!showThis);
	}
}

void MainWindow::updateRepositoryData() {
	ui->statusbar->showMessage(tr("Updating repository data"));
	updateRepositoryDataThread = new LoadUpdateRepositoryData(core);
	connect(updateRepositoryDataThread, SIGNAL(done()), this, SLOT(doneUpdateRepositoryData()));
	actionBus.registerEventHandler(&updateProgressData);
	progressWidgetPtr->show();
	updateRepositoryDataThread->start();
}
void MainWindow::doneUpdateRepositoryData() {
	updateRepositoryDataThread->wait();
	delete updateRepositoryDataThread;
	progressWidgetPtr->hide();
	actionBus.unregisterEventHandler();
	loadPackageList();
}

void MainWindow::requestCommit() {
	QVector<int> installQueue;
	QVector<int> removeQueue;

	for (unsigned int i=0; i<globalPkgList.size(); ++i) {
		if (globalPkgList[i].installed() && ui->packagesListWidget->item(i)->checkState()==Qt::Unchecked) removeQueue.push_back(i);
		if (!globalPkgList[i].installed() && ui->packagesListWidget->item(i)->checkState()==Qt::Checked) installQueue.push_back(i);
	}
	if (installQueue.isEmpty() && removeQueue.isEmpty()) {
		QMessageBox::information(this, tr("Nothing to perform"), tr("No actions to perform"));
		return;
	}

	PACKAGE_LIST tmpIQueue, tmpRQueue, failureList;
	for (int i=0; i<installQueue.size(); ++i) {
		tmpIQueue.add(globalPkgList[installQueue[i]]);
	}
	for (int i=0; i<removeQueue.size(); ++i) {
		tmpRQueue.add(globalPkgList[removeQueue[i]]);
	}
	core->DepTracker->reset();
	core->install(&tmpIQueue);
	core->uninstall(&tmpRQueue);
	core->DepTracker->renderData();
	tmpIQueue=core->DepTracker->get_install_list();
	tmpRQueue=core->DepTracker->get_remove_list();
	failureList=core->DepTracker->get_failure_list();
	//printf("Install queue: %d\n", tmpIQueue.size());
	//printf("Remove queue: %d\n", tmpRQueue.size());


	commitDialog = new CommitDialog(tmpIQueue, tmpRQueue);
	connect(commitDialog, SIGNAL(commit()), this, SLOT(commitActions()));
	commitDialog->show();

}
void MainWindow::resetQueue() {
	ui->statusbar->showMessage(tr("Cleaning queue"));
	core->clean_queue();
	loadPackageList();
	QMessageBox::information(this, tr("Queue cleaned up"), tr("Action queue was cleaned up"), QMessageBox::Ok);
}
void MainWindow::commitActions() {
	QVector<int> installQueue;
	QVector<int> removeQueue;

	for (unsigned int i=0; i<globalPkgList.size(); ++i) {
		if (globalPkgList[i].installed() && ui->packagesListWidget->item(i)->checkState()==Qt::Unchecked) removeQueue.push_back(i);
		if (!globalPkgList[i].installed() && ui->packagesListWidget->item(i)->checkState()==Qt::Checked) installQueue.push_back(i);
	}

	progressWidgetPtr->show();
	PACKAGE_LIST installList, removeList;
	for (int i=0; i<installQueue.size(); ++i) {
		installList.add(globalPkgList[installQueue[i]]);
	}
	for (int i=0; i<removeQueue.size(); ++i) {
		removeList.add(globalPkgList[removeQueue[i]]);
	}
	//core->install(&installList);
	//core->uninstall(&removeList);
	commitActionsThread = new CommitActions(core);
	connect(commitActionsThread, SIGNAL(done()), this, SLOT(commitFinished()));
	connect(commitActionsThread, SIGNAL(done()), progressWidgetPtr, SLOT(hide()));
	commitActionsThread->start();
}

void MainWindow::commitFinished() {
	//disconnectSlots();
	ui->tagsListWidget->clear();
	ui->packagesListWidget->clear();
	ui->packagesListWidget->setEnabled(false);
	ui->tagsListWidget->setEnabled(false);
	disconnect(commitActionsThread, SIGNAL(done()));
	commitActionsThread->wait();
	delete commitActionsThread;
	
	loadPackageList();
}
void MainWindow::loadPackageList() {
	ui->packagesListWidget->setEnabled(false);
	ui->tagsListWidget->setEnabled(false);
	loadTagsThread = new LoadTags(core);
	loadPackagesThread = new LoadPackages(core);
	connectThreadSlots();
	loadTagsThread->start();
}

bool MainWindow::checkSearchFilter(const QString& pkgName) {
	if (ui->searchLineEdit->text().isEmpty()) return true;
	return pkgName.contains(ui->searchLineEdit->text(), Qt::CaseInsensitive);
}

void MainWindow::applySearchFilter() {
	QListWidgetItem *item = ui->tagsListWidget->currentItem();
	if (!item) {
		if (ui->tagsListWidget->count() > 0) item=ui->tagsListWidget->item(0);
		else return;
	}
	receiveCategoryFilter(item, item);
	
}

void MainWindow::markAllVisible() {
	for (int i=0; i<ui->packagesListWidget->count(); ++i) {
		if (ui->packagesListWidget->item(i)->isHidden()) continue;
		ui->packagesListWidget->item(i)->setCheckState(Qt::Checked);
	}
}

void MainWindow::unmarkAllVisible() {
	for (int i=0; i<ui->packagesListWidget->count(); ++i) {
		if (ui->packagesListWidget->item(i)->isHidden()) continue;
		ui->packagesListWidget->item(i)->setCheckState(Qt::Unchecked);
	}

	
}

void MainWindow::cleanCache() {
	core->clean_cache();
	QMessageBox::information(this, tr("Cache cleaned up"), tr("Package cache was cleaned up"), QMessageBox::Ok);

}
