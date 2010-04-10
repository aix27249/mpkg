/*******************************************************************
 * MOPSLinux packaging system
 * Package manager - main code
 * $Id: mainwindow.cpp,v 1.132 2007/11/25 15:24:11 i27249 Exp $
 *
 ****************************************************************/
#define REALTIME_DEPTRACKER
#include <QTextCodec>
#include <QtGui>
#include "mainwindow.h"
#include <QDir>
#include <QFileDialog>
#include "aboutbox.h"
#include "preferencesbox.h"
#include "loading.h"
#include "db.h"
#include <stdio.h>
#include "tablelabel.h"
#include <unistd.h>

PrevState prev;
MainWindow::MainWindow(QMainWindow *parent)
{
	if (isDatabaseLocked()) 
	{
		QMessageBox::critical(this, tr("MOPSLinux package manager"),
				tr("Database is locked.\nPlease close another application that uses it"),
				QMessageBox::Ok, QMessageBox::Ok);
		abort();
	}
	mDebug("initializing");
	totalInstalledSize=0;
	totalAvailableSize=0;
	totalAvailableCount = 0;
	installedCount = 0;
	installQueueCount = 0;
	removeQueueCount = 0;
	willBeFreed = 0;
	willBeOccupied = 0;
	lockPackageList(false);
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("utf-8"));
	QTextCodec::setCodecForTr(QTextCodec::codecForName("utf-8"));

	initializeOk=false;
	consoleMode=false; // Setting event tracking to GUI mode
	currentCategoryID=1;
	qRegisterMetaType<QMessageBox::StandardButton>("QMessageBox::StandardButton");
	qRegisterMetaType<QMessageBox::StandardButtons>("QMessageBox::StandardButtons");

	ErrorBus = new errorBus;
	QObject::connect(this, SIGNAL(startErrorBus()), ErrorBus, SLOT(Start()), Qt::DirectConnection);
	QObject::connect(this, SIGNAL(sendUserReply(QMessageBox::StandardButton)), ErrorBus, SLOT(receiveUserReply(QMessageBox::StandardButton)));
	QObject::connect(ErrorBus, SIGNAL(sendErrorMessage(QString, QString, QMessageBox::StandardButtons, QMessageBox::StandardButton)), \
				this, SLOT(showErrorMessage(QString, QString, QMessageBox::StandardButtons, QMessageBox::StandardButton)));


	emit startErrorBus();
	if (getuid()!=0)
	{
		QMessageBox::critical(this, tr("MOPSLinux package manager"),
                   tr("You need to be root to run package manager"),
                   QMessageBox::Ok,
                   QMessageBox::Ok);
		exit(0);

	}

	if (parent == 0) ui.setupUi(this);
	else ui.setupUi(parent);
	QToolBar *mainToolBar = addToolBar("maintoolbar");
	applyChangesAction = mainToolBar->addAction(QIcon("/usr/share/mpkg/icons/installed.png"),tr("Apply changes"));
	resetChangesAction = mainToolBar->addAction(QIcon("/usr/share/mpkg/icons/rotate.png"), tr("Reset changes"));
	mainToolBar->addSeparator();
	showConfigAction = mainToolBar->addAction(QIcon("/usr/share/mpkg/icons/icons/default.kde/32x32/apps/kcmprocessor.png"), tr("Show core settings"));
	mainToolBar->addSeparator();
	showRepositoryConfigAction = mainToolBar->addAction(QIcon("/usr/share/mpkg/icons/icons/taskbar.png"), tr("Add/remove repositories"));
	updateRepositoryDataAction = mainToolBar->addAction(QIcon("/usr/share/mpkg/icons/reload.png"), tr("Update data"));
	QObject::connect(applyChangesAction, SIGNAL(triggered()), ui.actionCommit_changes, SIGNAL(triggered()));
	QObject::connect(ui.actionCommit_changes, SIGNAL(triggered()), ui.applyButton, SIGNAL(clicked()));
	QObject::connect(resetChangesAction, SIGNAL(triggered()), ui.actionReset_changes, SIGNAL(triggered()));
	QObject::connect(showConfigAction, SIGNAL(triggered()), ui.actionCore_settings, SIGNAL(triggered()));
	QObject::connect(showRepositoryConfigAction, SIGNAL(triggered()), ui.actionAdd_remove_repositories, SIGNAL(triggered()));
	QObject::connect(updateRepositoryDataAction, SIGNAL(triggered()), ui.actionUpdate_data, SIGNAL(triggered()));


	//updateRepositoryDataAction;
	//applyInstalledFilterAction;
	//applyAvailableFilterAction;
	//applyNotPurgedFilterAction;
	//applyDeprecatedFilterAction;
	//applyUnavailableFilterAction;

	ui.applyButton->setEnabled(false);
	ui.quickPackageSearchEdit->hide();
	ui.quickSearchLabel->hide();
	ui.clearSearchButton->hide();
	ui.packageTable->hide();
	movie = new QMovie("/usr/share/mpkg/icons/indicator.mng");
	ui.indicatorLabel->setMovie(movie);
	movie->start();
	ui.progressTable->setColumnWidth(0,190);
	ui.progressTable->setColumnWidth(1,350);
	ui.progressTable->setColumnHidden(2,true);

	clearForm();
	disableProgressBar(); disableProgressBar2();
	
	tableMenu = new QMenu;
	installPackageAction = tableMenu->addAction(tr("Install"));
	removePackageAction = tableMenu->addAction(tr("Remove"));
	purgePackageAction = tableMenu->addAction(tr("Purge"));

	qRegisterMetaType<string>("string");
	qRegisterMetaType<PACKAGE_LIST>("PACKAGE_LIST");
	qRegisterMetaType< vector<int> >("vector<int>");
	qRegisterMetaType< vector<string> >("vector<string>");
	StatusThread = new statusThread;
	thread = new coreThread; // Creating core thread
	packagelist = new PACKAGE_LIST;
	ui.progressTable->hide();
	// Building thread connections
	QObject::connect(this, SIGNAL(imReady()), thread, SLOT(recvReadyFlag()));
	QObject::connect(this, SIGNAL(fillReady()), thread, SLOT(recvFillReady()));

	QObject::connect(StatusThread, SIGNAL(showProgressWindow(bool)), this, SLOT(showProgressWindow(bool)));
	QObject::connect(StatusThread, SIGNAL(setSkipButton(bool)), this, SLOT(setSkipButton(bool)));
	QObject::connect(StatusThread, SIGNAL(setIdleButtons(bool)), this, SLOT(setIdleButtons(bool)));
	QObject::connect(ui.abortButton, SIGNAL(clicked()), this, SLOT(abortActions()));
	QObject::connect(ui.skipButton, SIGNAL(clicked()), this, SLOT(skipAction()));
	QObject::connect(this, SIGNAL(redrawReady(bool)), StatusThread, SLOT(recvRedrawReady(bool)));
	QObject::connect(StatusThread, SIGNAL(loadProgressData()), this, SLOT(updateProgressData()));
	QObject::connect(ui.clearSearchButton, SIGNAL(clicked()), ui.quickPackageSearchEdit, SLOT(clear()));
	QObject::connect(thread, SIGNAL(applyFilters()), this, SLOT(applyPackageFilter()));
	QObject::connect(thread, SIGNAL(initState(bool)), this, SLOT(setInitOk(bool)));
	QObject::connect(StatusThread, SIGNAL(setStatus(QString)), this, SLOT(setStatus(QString)));
	QObject::connect(this, SIGNAL(backsync(vector<int>)), thread, SLOT(recvBacksync(vector<int>)));
	QObject::connect(thread, SIGNAL(errorLoadingDatabase()), this, SLOT(errorLoadingDatabase()));
	QObject::connect(thread, SIGNAL(sqlQueryBegin()), this, SLOT(sqlQueryBegin()));
	QObject::connect(thread, SIGNAL(sqlQueryEnd()), this, SLOT(sqlQueryEnd()));
	QObject::connect(thread, SIGNAL(loadingStarted()), this, SLOT(loadingStarted()));
	QObject::connect(thread, SIGNAL(loadingFinished()), this, SLOT(loadingFinished()));
	QObject::connect(thread, SIGNAL(enableProgressBar()), this, SLOT(enableProgressBar()));
	QObject::connect(thread, SIGNAL(disableProgressBar()), this, SLOT(disableProgressBar()));
	QObject::connect(thread, SIGNAL(resetProgressBar()), this, SLOT(resetProgressBar()));
	QObject::connect(thread, SIGNAL(setProgressBarValue(unsigned int)), this, SLOT(setProgressBarValue(unsigned int)));
	QObject::connect(thread, SIGNAL(clearTable()), this, SLOT(clearTable()));
	QObject::connect(thread, SIGNAL(setTableSize(unsigned int)), this, SLOT(setTableSize(unsigned int)));
	QObject::connect(thread, SIGNAL(setTableItem(unsigned int, int, bool, string)), this, SLOT(setTableItem(unsigned int, int, bool, string)));
	QObject::connect(thread, SIGNAL(setTableItemVisible(unsigned int, bool)), this, SLOT(setTableItemVisible(unsigned int, bool)));
	QObject::connect(this, SIGNAL(loadPackageDatabase()), thread, SLOT(loadPackageDatabase()));
	QObject::connect(this, SIGNAL(startThread()), thread, SLOT(start()), Qt::DirectConnection);
	QObject::connect(this, SIGNAL(startStatusThread()), StatusThread, SLOT(start()), Qt::DirectConnection);
	QObject::connect(this, SIGNAL(requestPackages(vector<bool>)), thread, SLOT(loadItems(vector<bool>)));
	QObject::connect(this, SIGNAL(syncData()), thread, SLOT(getPackageList()));
	QObject::connect(thread, SIGNAL(initProgressBar(unsigned int)), this, SLOT(initProgressBar(unsigned int)));
	QObject::connect(thread, SIGNAL(sendPackageList(PACKAGE_LIST, vector<int>)), this, SLOT(receivePackageList(PACKAGE_LIST, vector<int>)));
	QObject::connect(thread, SIGNAL(loadData()), this, SLOT(loadData()), Qt::DirectConnection);
	QObject::connect(this, SIGNAL(updateDatabase()), thread, SLOT(updatePackageDatabase()));
	QObject::connect(this, SIGNAL(quitThread()), thread, SLOT(callQuit()));
	QObject::connect(this, SIGNAL(commit(vector<int>)), thread, SLOT(commitQueue(vector<int>)));
	QObject::connect(thread, SIGNAL(setStatus(QString)), this, SLOT(setStatus(QString)));
	QObject::connect(StatusThread, SIGNAL(enableProgressBar()), this, SLOT(enableProgressBar()));
	QObject::connect(StatusThread, SIGNAL(disableProgressBar()), this, SLOT(disableProgressBar()));
	QObject::connect(StatusThread, SIGNAL(setBarValue(unsigned int)), this, SLOT(setProgressBarValue(unsigned int)));
	QObject::connect(StatusThread, SIGNAL(initProgressBar(unsigned int)), this, SLOT(initProgressBar(unsigned int)));
	QObject::connect(this, SIGNAL(getAvailableTags()), thread, SLOT(getAvailableTags()));
	QObject::connect(thread, SIGNAL(sendAvailableTags(vector<string>)), this, SLOT(receiveAvailableTags(vector<string>)), Qt::QueuedConnection);
	QObject::connect(StatusThread, SIGNAL(enableProgressBar2()), this, SLOT(enableProgressBar2()));
	QObject::connect(StatusThread, SIGNAL(disableProgressBar2()), this, SLOT(disableProgressBar2()));
	QObject::connect(StatusThread, SIGNAL(setBarValue2(unsigned int)), this, SLOT(setProgressBarValue2(unsigned int)));
	QObject::connect(StatusThread, SIGNAL(initProgressBar2(unsigned int)), this, SLOT(initProgressBar2(unsigned int)));
	QObject::connect(this, SIGNAL(callCleanCache()), thread, SLOT(cleanCache()));
	QObject::connect(thread, SIGNAL(showMessageBox(QString, QString)), this, SLOT(showMessageBox(QString, QString)));
	// Startup initialization
	emit startThread(); // Starting thread (does nothing imho....)
	emit startStatusThread();
	prefBox = new PreferencesBox(mDb);
	QObject::connect(prefBox, SIGNAL(updatePackageData()), thread, SLOT(updatePackageDatabase()));
	QObject::connect(prefBox, SIGNAL(getCdromName()), thread, SLOT(getCdromName()));
	QObject::connect(thread, SIGNAL(sendCdromName(string)), prefBox, SLOT(recvCdromVolname(string))); 

#ifdef REALTIME_DEPTRACKER_OLD
	QObject::connect(this, SIGNAL(getRequiredPackages(unsigned int)), thread, SLOT(getRequiredPackages(unsigned int)));
	QObject::connect(this, SIGNAL(getDependantPackages(unsigned int)), thread, SLOT(getDependantPackages(unsigned int)));
	QObject::connect(thread, SIGNAL(sendRequiredPackages(unsigned int, PACKAGE_LIST)), this, SLOT(receiveRequiredPackages(unsigned int, PACKAGE_LIST)));
	QObject::connect(thread, SIGNAL(sendDependantPackages(unsigned int, PACKAGE_LIST)), this, SLOT(receiveDependantPackages(unsigned int, PACKAGE_LIST)));
#endif
	QObject::connect(thread, SIGNAL(loadDefaultData()), this, SLOT(loadDefaultData()));
	
	lockDatabase();
	mDebug("init ok, let's show the UI");
	this->show();
	// Wait threads to start
	while (!StatusThread->isRunning() && !thread->isRunning() && !ErrorBus->isRunning())
	{
		say("Waiting for threads to start...\n");
		usleep(1);
	}
	mDebug("requesting data");
	emit getAvailableTags();

	//printf("constructor finished\n");
	mDebug("constructor complete");
}

MainWindow::~MainWindow()
{
	ui.packageTable->clearContents();
	thread->callQuit();
	StatusThread->halt();
	ErrorBus->Stop();
	thread->wait();
	StatusThread->wait();
	ErrorBus->wait();
	
	if (isMounted(CDROM_MOUNTPOINT)) system("umount " + CDROM_MOUNTPOINT + " 2> /dev/null");
	//delete thread;
	//delete StatusThread;
	//delete ErrorBus;
}


void MainWindow::setSkipButton(bool flag)
{
	ui.skipButton->setVisible(flag);
}

void MainWindow::setIdleButtons(bool flag)
{
	ui.applyButton->setVisible(flag);
	ui.quitButton->setVisible(flag);
	ui.abortButton->setVisible(!flag);
}

void MainWindow::errorLoadingDatabase()
{
	QMessageBox::critical(this, tr("MOPSLinux package manager"),
				tr("Database initialization error!\nCheck your settings"),
				QMessageBox::Ok, QMessageBox::Ok);

}

void MainWindow::showErrorMessage(QString headerText, QString bodyText, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
	emit sendUserReply(QMessageBox::warning(this, headerText, bodyText, buttons, defaultButton));
}

void MainWindow::showMessageBox(QString header, QString body)
{
	QMessageBox::information(this, header, body, QMessageBox::Ok, QMessageBox::Ok);
}
void MainWindow::sqlQueryBegin()
{
	currentStatus=tr("Loading database...").toStdString();
}

void MainWindow::sqlQueryEnd()
{
	currentStatus=tr("Data loaded, rendering visuals").toStdString();
}

void MainWindow::loadingStarted()
{

	ui.statLabel->setText("");
	ui.loadingLabel->setText("<html><img src=\"/usr/share/mpkg/splash.png\"></img></html>");
	ui.packageListBox->setEnabled(false);
	//ui.packageInfoBox->hide();
	ui.packageTable->hide();
	ui.selectAllButton->hide();
	ui.deselectAllButton->hide();
	ui.quickPackageSearchEdit->hide();
	ui.quickSearchLabel->hide();
	ui.clearSearchButton->hide();
	ui.splashFrame->show();
}

void MainWindow::filterCloneItems()
{

}			

void MainWindow::loadingFinished()
{
	hideEntireTable();
	ui.splashFrame->hide();
	ui.packageListBox->setEnabled(true);
	ui.progressTable->hide();
	ui.quickSearchLabel->show();
	ui.quickPackageSearchEdit->show();
	ui.clearSearchButton->show();
	ui.packageTable->show();
	ui.selectAllButton->show();
	ui.deselectAllButton->show();
	setTableSize();
	emit imReady();
	currentStatus = tr("Idle").toStdString();
}

void MainWindow::enableProgressBar()
{
	ui.progressBar->show();
	ui.currentLabel->show();
}

void MainWindow::disableProgressBar()
{
	ui.progressBar->hide();
	ui.currentLabel->hide();
}
void MainWindow::resetProgressBar()
{
	ui.progressBar->reset();
}
void MainWindow::setProgressBarValue(unsigned int value)
{
	ui.progressBar->setValue(value);
}

void MainWindow::enableProgressBar2()
{
	ui.progressBar2->show();
	ui.totalLabel->show();
}

void MainWindow::disableProgressBar2()
{
	ui.progressBar2->hide();
	ui.totalLabel->hide();
}

void MainWindow::setProgressBarValue2(unsigned int value)
{
	ui.progressBar2->setValue(value);
}

void MainWindow::generateStat(vector<int> newStatusData)
{
	// TODO: use deptracker data too
	totalInstalledSize=0;
	totalAvailableSize=0;
	totalAvailableCount = 0;
	installedCount = 0;
	installQueueCount = 0;
	removeQueueCount = 0;
	updateQueueCount = 0;
	willBeFreed = 0;
	willBeOccupied = 0;
	if (!initializeOk)
	{
		return;
	}
	waitUnlock();
	if ((unsigned int) packagelist->size()!=newStatusData.size())
	{
		mError("Structure not ready");
		return;
	}
	for (unsigned int i=0; i<packagelist->size(); i++)
	{
		if (packagelist->at(i).installed())
		{
			installedCount++;
			totalInstalledSize = totalInstalledSize + atoi(packagelist->at(i).get_installed_size().c_str());
		}
		if (packagelist->at(i).available())
		{
			totalAvailableCount++;
			totalAvailableSize=totalAvailableSize + atoi(packagelist->at(i).get_compressed_size().c_str());
		}
		if (newStatusData[i]==ST_INSTALL)
		{
			installQueueCount++;
			willBeOccupied = willBeOccupied + atoi(packagelist->at(i).get_installed_size().c_str());
		}
		if (newStatusData[i]==ST_REMOVE)
		{
			removeQueueCount++;
			willBeFreed = willBeFreed + atoi(packagelist->at(i).get_installed_size().c_str());
		}
		if (newStatusData[i] == ST_UPDATE)
		{
			updateQueueCount++;
			willBeFreed = willBeFreed + atoi(packagelist->at(i).get_installed_size().c_str());
		}

		if (newStatusData[i]==ST_PURGE)
		{
			removeQueueCount++;
			if (packagelist->at(i).installed()) willBeFreed=willBeFreed + atoi(packagelist->at(i).get_installed_size().c_str());
		}
	}
	installQueueCount -= updateQueueCount;
	QString countStat = tr("Installed: ") + IntToStr(installedCount).c_str() +\
			    tr(", Available: ")+IntToStr(totalAvailableCount).c_str()+\
			    tr(", To install: ")+IntToStr(installQueueCount).c_str()+\
			    tr(", To remove: ")+IntToStr(removeQueueCount).c_str()+\
			    tr(", To update: ")+IntToStr(updateQueueCount).c_str();
	if (willBeFreed>willBeOccupied) countStat += tr(", Will be freed: ") + humanizeSize(willBeFreed - willBeOccupied).c_str();
	if (willBeFreed<willBeOccupied) countStat+= tr(", Will be occupied: ") + humanizeSize(willBeOccupied - willBeFreed).c_str();

	ui.statLabel->setText(countStat);
}

bool isSameVersionInstalled(PACKAGE_LIST *p, int num) {
	string fullversion = p->at(num).get_fullversion();
	string name = p->at(num).get_name();
	for (unsigned int i=0; i<p->size(); i++) {
		if (p->at(i).installed() && p->at(i).get_name() == name && p->at(i).get_fullversion() == fullversion) return true;
	}
	return false;
}

void MainWindow::applyPackageFilter ()
{
	if (!initializeOk || currentCategoryID<0 || currentCategoryID > ui.listWidget->count())
	{
		mDebug("uninitialized");
		return;
	}

	mDebug("init ok");
	string pkgBoxLabel = tr("Packages").toStdString();
	generateStat(newStatus);
	bool nameOk = false;
	bool statusOk = false;
	bool categoryOk = false;
	bool deprecatedOk = false;
	vector<string> tmpTagList;
	string tagvalue;
	int action;
	bool available;
	bool installed;
	bool configexist;
	bool deprecated;
	bool sameversion;
	mDebug("request to currentCategory " + IntToStr(currentCategoryID) + ", list count = " + IntToStr(ui.listWidget->count()));
	pkgBoxLabel += " - " + ui.listWidget->item(currentCategoryID)->text().toStdString();
	pkgBoxLabel += " ";
	unsigned int pkgCount = 0;
	mDebug("initializing highlight map");
	for (unsigned int a=0; a<availableTags.size(); a++)
	{
		highlightMap[availableTags[a]]=false;
	}
	waitUnlock();
	mDebug("processing items, packagelist size = " + IntToStr(packagelist->size()));
	for (unsigned int i=0; i<packagelist->size(); i++)
	{
		nameOk = false;
		statusOk = false;
		categoryOk = false;
		nameOk = nameComplain(i, ui.quickPackageSearchEdit->text());
	
		if (nameOk)
		{
			action = packagelist->at(i).action();
			available = packagelist->at(i).available();
			installed = packagelist->at(i).installed();
			configexist = packagelist->at(i).configexist();
			deprecated = packagelist->at(i).deprecated();
			sameversion = isSameVersionInstalled(packagelist, i);
			statusOk=false;

			if (ui.actionShow_installed->isChecked() && installed)
			{
				ui.packageTable->setRowHidden(i, false);
				statusOk = true;
			}
			if (ui.actionShow_available->isChecked() && available && !installed && !sameversion)
			{
				ui.packageTable->setRowHidden(i, false);
				statusOk = true;
			}
			if (ui.actionShow_configexist->isChecked() && !installed && configexist)
			{
				ui.packageTable->setRowHidden(i, false);
				statusOk = true;
			}
			if (ui.actionShow_queue->isChecked() && action!=ST_NONE)
			{
				ui.packageTable->setRowHidden(i, false);
				statusOk = true;
			}
			if (ui.actionShow_unavailable->isChecked() && !available && !installed)
			{
				ui.packageTable->setRowHidden(i,false);
				statusOk=true;
			}
		} // if(nameOk)
		if (statusOk)
		{
			if (deprecated)
			{
				if (ui.actionShow_deprecated->isChecked())
				{
					ui.packageTable->setRowHidden(i, false);
					deprecatedOk = true;
				}
				else deprecatedOk = false;
			}
			else deprecatedOk=true;

		}
		for (unsigned int a=0; a<availableTags.size(); a++)
		{
			if (isCategoryComplain(i, a) && nameOk)
			{
				highlightMap[availableTags[a]]=true;
			}
		}
		if (nameOk && statusOk && deprecatedOk && isCategoryComplain(i, currentCategoryID))
		{
			pkgCount++;
			ui.packageTable->setRowHidden(packagelist->getTableID(i), false);
		}
		else 
		{
			if (isCategoryComplain(i, currentCategoryID)) ui.packageTable->setRowHidden(packagelist->getTableID(i), true);
		}
	} // for (...)

	pkgBoxLabel += "\t\t("+IntToStr(pkgCount)+"/"+IntToStr(ui.packageTable->rowCount())\
			+tr(" packages)").toStdString();
	ui.packagesBox->setTitle(pkgBoxLabel.c_str());
	mDebug("calling highlight");
	highlightCategoryList();
}


void MainWindow::clearTable()
{
	ui.packageTable->clearContents();
	ui.packageTable->setRowCount(0);
}

void MainWindow::setTableSize(unsigned int size)
{
	ui.packageTable->setRowCount(size);
}

void MainWindow::selectAll()
{
	for (int i = 0; i<ui.packageTable->rowCount(); i++)
	{
		if (!ui.packageTable->isRowHidden(i))
		{
			markChanges(i, Qt::Checked);
		}
	}
	applyPackageFilter();
}

void MainWindow::deselectAll()
{
	for (int i = 0; i<ui.packageTable->rowCount(); i++)
	{
		if (!ui.packageTable->isRowHidden(i)) markChanges(i, Qt::Unchecked);
	}
	applyPackageFilter();
}

string MainWindow::bool2str(bool data)
{
	if (data) return tr("true").toStdString();
	else return tr("false").toStdString();
}

void MainWindow::setTableItem(unsigned int row, int packageNum, bool checkState, string cellItemText)
{
	//waitUnlock();
	if ((unsigned int) packageNum>=packagelist->size())
	{
		mDebug("uninitialized");
		return;
	}
	if (row >= (unsigned int) ui.packageTable->rowCount())
	{
		mDebug("out of range (skipped): " + IntToStr(row));
		return;
	}
	if (nameComplain(packageNum, ui.quickPackageSearchEdit->text()))
	{
		ui.packageTable->setRowHidden(row, false);
	}
	packagelist->setTableID(packageNum, row);
	CheckBox *stat = new CheckBox(this);
	ui.packageTable->setCellWidget(row,PT_INSTALLCHECK, stat);
	if (checkState) stat->setCheckState(Qt::Checked);
	else stat->setCheckState(Qt::Unchecked);

	TableLabel *pkgName = new TableLabel(ui.packageTable);
	pkgName->setTextFormat(Qt::RichText);
	cellItemText = "<html>" + cellItemText + "</html>";
	pkgName->setText(cellItemText.c_str());
	pkgName->row = row;
	ui.packageTable->setCellWidget(row, PT_NAME, pkgName);
//	ui.packageTable->setItem(row, PT_NAME, new QTableWidgetItem(cellItemText.c_str()));
	string depData;
	if (packagelist->at(packageNum).installed() && packagelist->at(packageNum).isRemoveBlacklisted()) stat->setEnabled(false);
	string tagList = "";
	//printf("set table item, cellItemText size = %d\n", cellItemText.size());
	cellItemText+="<html><b>"+tr("Installed version:").toStdString()+" </b>" + packagelist->at(packageNum).installedVersion + \
		       "<br><b>"+tr("It is max version:").toStdString()+" </b>" + bool2str(packagelist->at(packageNum).hasMaxVersion)+\
		       "<br><b>"+tr("Max version:").toStdString()+" </b>" + packagelist->at(packageNum).maxVersion + \
			"<br>" + depData + tagList + \
			"<b>"+tr("Description:").toStdString()+" </b><br>" + \
		       adjustStringWide(packagelist->at(packageNum).get_description(), packagelist->at(packageNum).get_short_description().size())+ \
		       		       "</html>";
	stat->row = row;
	QObject::connect(stat, SIGNAL(stateChanged(int)), stat, SLOT(markChanges()));
	QObject::connect(stat, SIGNAL(stateChanged(int)), this, SLOT(applyPackageFilter()));
	ui.packageTable->setRowHeight(row, 45);
//	printf("Setting tooltip to %dx%d, table size = %dx%d\n", row, PT_NAME, ui.packageTable->rowCount(), ui.packageTable->columnCount());
	ui.packageTable->cellWidget(row, PT_NAME)->setToolTip(cellItemText.c_str());
//	pkgName->repaint();
//	printf("set\n");
	//ui.packageTable->cellWidget(row, PT_NAME)->setToolTip((QString) "VOID");
//	mDebug("Tooltip set");

	//ui.packageTable->setRowHidden(row, true);
	//thread->recvFillReady();
}


void MainWindow::setTableItemVisible(unsigned int row, bool visible)
{
	ui.packageTable->setRowHidden(row, visible);
}



void MainWindow::abortActions()
{
	if (QMessageBox::warning(this, tr("Please confirm abort"), tr("Are you sure you want to abort current operations?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No)==QMessageBox::Yes)
	{
		actionBus.abortActions();
	}
}

void MainWindow::skipAction()
{
	actionBus.skipAction(actionBus.currentProcessingID());
}

void MainWindow::setInitOk(bool flag)
{
	initializeOk=flag;
}
void MainWindow::showProgressWindow(bool flag)
{
	if (flag)
	{
		ui.progressTable->show();
		ui.listWidget->setEnabled(false);
		if (ui.packageTable->isVisible())
		{
			ui.packageTable->hide();
			ui.selectAllButton->hide();
			ui.quickSearchLabel->hide();
			ui.deselectAllButton->hide();
			ui.quickPackageSearchEdit->hide();
			ui.clearSearchButton->hide();
		}
	}
	else
	{
		if (ui.progressTable->isVisible() && !ui.splashFrame->isVisible())
		{
			ui.packageTable->show();
			ui.selectAllButton->show();
			ui.deselectAllButton->show();
			ui.quickPackageSearchEdit->show();
			ui.quickSearchLabel->show();
			ui.clearSearchButton->show();
		}
		ui.listWidget->setEnabled(true);
		ui.progressTable->hide();
	}
}
PrevState::PrevState(){}
PrevState::~PrevState(){}
void PrevState::resize(unsigned int newSize) {
	label.resize(newSize);
	bar.resize(newSize);
	state.resize(newSize);
}
unsigned int PrevState::size() {
	if (label.size()!=bar.size() || label.size()!=state.size()) printf("OMG!!!!!!\n");
	return label.size();
}
void MainWindow::updateProgressData()
{
	ui.packagesBox->setTitle(pData.getCurrentAction().c_str());
	emit redrawReady(false);
	double dtmp=0;
	int tmp_c;
	ui.progressTable->clearContents();
	int totalCount=pData.size();
	int totalVisible=0;
	for (unsigned int i=0; i<pData.size(); i++)
	{
		if (pData.getItemState(i)!=ITEMSTATE_WAIT ) totalVisible++;
	}
	ui.progressTable->setRowCount(totalCount);

	int tablePos = 0;
	bool showIt;
	bool updateIt;
	bool keepCount;
	if (pData.size()>0)
	{
		for (unsigned int i=0; i<pData.size(); i++)
		{
			tablePos=i;
			showIt=false;
			updateIt=false;
			keepCount=false;
			if (pData.getItemState(i)==ITEMSTATE_FINISHED && totalVisible>20)
			{
				keepCount=true;
				totalVisible--;
			}
			if (pData.getItemState(i)!=ITEMSTATE_WAIT && !keepCount) showIt=true;

			if (pData.getItemState(i)==ITEMSTATE_FINISHED) pData.increaseIdleTime(i); 
			
			if (showIt)
			{
				ui.progressTable->setRowHidden(tablePos, false);
				pData.setItemUnchanged(i);
				
				QTableWidgetItem *__name = new QTableWidgetItem;
				__name->setText(pData.getItemName(i).c_str());
				
				switch (pData.getItemState(i))
				{
					case ITEMSTATE_INPROGRESS:
						__name->setIcon(QIcon(QString::fromUtf8("/usr/share/mpkg/icons/kget.png")));
						break;
					case ITEMSTATE_FINISHED:
						__name->setIcon(QIcon(QString::fromUtf8("/usr/share/mpkg/icons/installed.png")));
						break;
					case ITEMSTATE_FAILED:
						__name->setIcon(QIcon(QString::fromUtf8("/usr/share/mpkg/icons/remove.png")));
						break;
				}
				
				ui.progressTable->setItem(tablePos,0, __name);
				if (pData.getItemState(i)==ITEMSTATE_INPROGRESS)
				{
					//QProgressBar *pBar = new QProgressBar;
					//QTableWidgetItem pText = new QTableWidgetItem;
					//pBar->setFormat(pData.getItemCurrentAction(i).c_str());
					QString txt = pData.getItemCurrentAction(i).c_str();
					if (pData.size()>0) dtmp = 100 * (pData.getItemProgress(i)/pData.getItemProgressMaximum(i));
					tmp_c = (int) dtmp;

					//pBar->setMaximum(100);
					//pBar->setValue(tmp_c);
					//txt += " (" + (QString) IntToStr(tmp_c).c_str()+ ")";
					ui.progressTable->setItem(tablePos,1,new QTableWidgetItem(txt));
				}
				else
				{
					ui.progressTable->setItem(tablePos,1,new QTableWidgetItem(pData.getItemCurrentAction(i).c_str()));
				}
				ui.progressTable->setRowHeight(tablePos,20);
			}
			else ui.progressTable->setRowHidden(tablePos, true);

			tablePos++;
		}
	}
	emit redrawReady(true);

}

void MainWindow::updateProgressData_new(){} // Placebo
/*void MainWindow::updateProgressData()
{
	//printf("update begin\n");
	ProgressData current = pData;

	//printf("[1] current size: %d, prev.size: %d, prev.data.size: %d\n", current.size(), prev.size(), prev.data.size());
	emit redrawReady(false);
	if (prev.data.getCurrentAction()!=current.getCurrentAction()) {
	//	printf("Updating title\n");
		ui.packagesBox->setTitle(current.getCurrentAction().c_str());
	//	printf("done\n");
	}
	double dtmp=0;
	int tmp_c;
	if (current.size() != ui.progressTable->rowCount()) ui.progressTable->setRowCount(current.size());

	QTableWidgetItem *__name;
//	QProgressBar *__pBar;
	QTableWidgetItem *__tBar;
	QTableWidgetItem *__state;

	for (unsigned int i=0; i<current.size(); i++)
	{
		if (i>=prev.size()) {
			__name = new QTableWidgetItem;
			__tBar = new QTableWidgetItem;
			__state = new QTableWidgetItem;	
			//__pBar->setMaximum(100);
			//ui.progressTable->setCellWidget(i,1,__pBar);
			prev.label.push_back(__name);
			prev.bar.push_back(__tBar);
			prev.state.push_back(__state);
			ui.progressTable->setItem(i, 0, __name);
			ui.progressTable->setItem(i, 1, __state);
			ui.progressTable->setItem(i, 2, __tBar);
		}
		__name = prev.label[i];
		__tBar = prev.bar[i];
		__state = prev.state[i];

		if (i>=prev.data.size() || prev.data.getItemName(i)!=current.getItemName(i)) __name->setText(current.getItemName(i).c_str());
		
		if (i>=prev.data.size() || current.getItemState(i)!=prev.data.getItemState(i)) {
			switch (current.getItemState(i))
			{
				case ITEMSTATE_INPROGRESS:
					__name->setIcon(QIcon(QString::fromUtf8("/usr/share/mpkg/icons/kget.png")));
					break;
				case ITEMSTATE_FINISHED:
					__name->setIcon(QIcon(QString::fromUtf8("/usr/share/mpkg/icons/installed.png")));
					break;
				case ITEMSTATE_FAILED:
					__name->setIcon(QIcon(QString::fromUtf8("/usr/share/mpkg/icons/remove.png")));
					break;
			}
		}
			
		if (i>=prev.data.size() || current.getItemCurrentAction(i)!=prev.data.getItemCurrentAction(i)) __tBar->setText(current.getItemCurrentAction(i).c_str());
		if (i>=prev.data.size() || current.getItemProgress(i)!=prev.data.getItemProgress(i) || current.getItemProgressMaximum(i)!=prev.data.getItemProgressMaximum(i)) {
			dtmp = 100 * (current.getItemProgress(i)/current.getItemProgressMaximum(i));
			tmp_c = (int) dtmp;
			__tBar->setText(__tBar->text() + (QString) "(" + (QString) IntToStr(tmp_c).c_str()+(QString) "%)");
		}
		ui.progressTable->setRowHeight(i,20);
	}
	prev.data = current; // Storing data
	prev.resize(current.size());
	emit redrawReady(true);
}
*/
void MainWindow::cleanCache()
{
	emit callCleanCache();
}	

void MainWindow::receiveAvailableTags(vector<string> tags)
{
	lockPackageList(true);
	availableTags.clear();
	availableTags.push_back("_all_");
	availableTags.push_back("_updates_");
	for (unsigned int i=0; i<tags.size(); i++)
	{
		availableTags.push_back(tags[i]);
	}
	availableTags.push_back("_misc_");

	lockPackageList(false);
	initCategories(true);
}

void MainWindow::initCategories(bool initial)
{
	if (!FileExists("/etc/mpkg-groups.xml")) return;
	XMLResults xmlErrCode;
	_categories = XMLNode::parseFile("/etc/mpkg-groups.xml", "groups", &xmlErrCode);
	if (xmlErrCode.error != eXMLErrorNone)
	{
		return;
	}
	ui.listWidget->clear();

	bool named = false;
	for (unsigned int i=0; i<availableTags.size(); i++)
	{
		highlightMap[availableTags[i]]=false;

		named=false;
		for (int t = 0; t< _categories.nChildNode("group");t++)
		{
			if (availableTags[i]==(string) _categories.getChildNode("group",t).getAttribute("tag"))
			{
				named=true;
				QListWidgetItem *__item = new QListWidgetItem(ui.listWidget);
				//ListLabel *L_item = new ListLabel(ui.listWidget, i);
				__item->setText((QString) _categories.getChildNode("group",t).getAttribute("name"));
		    		__item->setIcon(QIcon(QString::fromUtf8(_categories.getChildNode("group", t).getAttribute("icon"))));
				//ui.listWidget->setItemWidget(__item, L_item);
			}
		}
		if (!named)
		{
			//ListLabel *L_item = new ListLabel(ui.listWidget, i);
			QListWidgetItem *__item = new QListWidgetItem(ui.listWidget);
			__item->setText(availableTags[i].c_str());
	    		__item->setIcon(QIcon("/usr/share/mpkg/icons/icons/taskbar.png"));
			//ui.listWidget->setItemWidget(__item, L_item);


		}
	}

	
	if (initial)
	{
		ui.listWidget->setCurrentRow(1);
		ui.listWidget->scrollToItem(ui.listWidget->item(0));

		QObject::connect(ui.listWidget, SIGNAL(currentRowChanged(int)), this, SLOT(filterCategory(int)));

		emit loadPackageDatabase(); // Calling loadPackageDatabase from thread
	}
	subsysOk = true;
	//printf("categories init ok\n");
}

void MainWindow::loadDefaultData()
{
	//printf("loading default data\n");
	filterCategory(ui.listWidget->currentRow());
}

void MainWindow::hideEntireTable()
{
	for (int i=0; i<ui.packageTable->rowCount(); i++)
	{
		ui.packageTable->setRowHidden(i, true);
	}
}
bool MainWindow::isCategoryComplain(int package_num, int category_id)
{
	waitUnlock();
	if (packagelist->size()<=(unsigned int) package_num)
	{
		mDebug("uninitialized");
		return false;
	}
	string tagvalue;
	bool ret=false;
	if (category_id>=0 && availableTags.size()>(unsigned int) category_id)
	{
		tagvalue = availableTags[category_id];
	}
	else tagvalue = "";
	if (tagvalue == "_updates_")
	{
		if (packagelist->at(package_num).isUpdate()) ret = true;
		else ret = false;
	}
	else
	{
		if (tagvalue == "_all_")
		{
			ret = true;
		}
		else
		{
			ret = false;
			for (unsigned int t = 0; t < packagelist->at(package_num).get_tags().size(); t++)
			{
				if (packagelist->at(package_num).get_tags().at(t) == tagvalue)
				{
					ret = true;
				}
			} // for (... tmpTagList ...)	
			if (packagelist->at(package_num).get_tags().empty() && tagvalue != "_misc_")
			{
				ret = false;
			}
			else
			{
				if (packagelist->at(package_num).get_tags().empty() && tagvalue == "_misc_") ret = true;
			}
		}
	}
	return ret;
}

void MainWindow::filterCategory(int category_id)
{
	//printf("filtering categories\n");

	waitUnlock();
	mDebug("filterCategory");
	if (!actionBus.idle()) actionBus.abortActions();
	currentCategoryID = category_id;
	vector<bool> request;
	vector<string> tmpTagList;
	string tagvalue;
	request.resize(packagelist->size());
	for (unsigned int i=0; i<packagelist->size(); i++)
	{
		request[i]=isCategoryComplain(i, category_id);

	}
	if (actionBus._abortActions)
	{
		while (!actionBus._abortComplete)
		{
			usleep(10);
		}
	}
	emit requestPackages(request);
	applyPackageFilter();
}

void MainWindow::setStatus(QString status)
{
	ui.statusbar->showMessage(status);
}

void MainWindow::lockPackageList(bool state)
{
	__pkgLock=state;
}

void MainWindow::waitUnlock()
{
	//while (__pkgLock)
	//{
	//	usleep(1);
	//}
}


void MainWindow::receivePackageList(PACKAGE_LIST pkgList, vector<int> nStatus)
{
	*packagelist=pkgList;
	newStatus = nStatus;

}

void MainWindow::quickPackageSearch()
{
	waitUnlock();
	mDebug("searching");
	QString tmp;
	for (int i=0; i<ui.packageTable->rowCount(); i++)
	{
		tmp = tmp.fromStdString(packagelist->getPackageByTableID(i)->get_name());
		if (!tmp.contains(ui.quickPackageSearchEdit->text(), Qt::CaseInsensitive))
		{
			ui.packageTable->setRowHidden(i, true);
		}
		else
		{
			ui.packageTable->setRowHidden(i, false);
		}
	}
}

void MainWindow::resetQueue()
{
	for (unsigned int i=0; i<newStatus.size(); i++)
	{
		newStatus.at(i)=ST_NONE;
	}
	commitChanges();
}

void MainWindow::showPackageInfo()
{
	waitUnlock();
	mDebug("");

	long id = ui.packageTable->currentRow();
	PACKAGE *pkg = packagelist->getPackageByTableID(id);
	string info = "<html><h1>"+pkg->get_name()+" "+pkg->get_version()+"</h1><p><b>"+\
		       tr("Architecture:").toStdString()+"</b> "+pkg->get_arch()+"<br><b>" + \
		       tr("Build:").toStdString()+"</b> "+pkg->get_build();

	if (pkg->get_type()==PKGTYPE_SOURCE) info+="<br><b>" + tr("Type:").toStdString() + "</b>" + tr(" source").toStdString();
	if (pkg->get_type()==PKGTYPE_BINARY) info+="<br><b>" + tr("Type:").toStdString() + "</b>" + tr(" binary").toStdString();
	info += "<br><b>" + tr("Description:").toStdString()+" </b><br>"+pkg->get_description()+"</p></html>";
	mstring taglist;
	for (unsigned int i=0; i< pkg->get_tags().size(); i++)
	{
		taglist+="<br>";
		taglist+=pkg->get_tags().at(i);
	}
	string extendedInfo = (string) "<html>" \
			       + (string) "<h2>" + pkg->get_name() + (string) "</h2>" \
			       + (string) "<br><b>"+tr("Version:").toStdString()+" </b>" + pkg->get_version() \
			       + (string) "<br><b>"+tr("Beta release:").toStdString()+"</b>" + pkg->get_betarelease() \
			       + (string) "<br><b>"+tr("Arch:").toStdString()+" </b>"+pkg->get_arch() \
			       + (string) "<br><b>"+tr("Build:").toStdString()+" </b>"+pkg->get_build() \
			       + (string) "<br><b>"+tr("Package size:").toStdString()+" </b>" + humanizeSize(pkg->get_compressed_size()) \
			       + (string) "<br><b>"+tr("Installed size:").toStdString()+" </b>" + humanizeSize(pkg->get_installed_size()) \
			       + (string) "<br><b>"+tr("Filename:").toStdString()+" </b>" + pkg->get_filename() \
			       + (string) "<br><b>"+tr("MD5 sum:").toStdString()+" </b>"+pkg->get_md5() \
			       + (string) "<br><b>"+tr("Maintainer:").toStdString()+" </b>"+pkg->get_packager() \
			       + (string) " (" + pkg->get_packager_email() + (string)")" \
			       + (string) "<br><b>"+tr("Status:").toStdString()+" </b>" + pkg->get_vstatus() \
			       + (string) "<br><br><b>"+tr("Tags:").toStdString()+"</b> " \
			       + taglist.s_str() \
			       + (string) "</html>";

	// Dependencies data
	string depData;
	for (unsigned int i=0; i<pkg->get_dependencies().size(); i++)
	{
		depData += pkg->get_dependencies().at(i).getDepInfo()+"<br>";
	}
	ui.depTextBrowser->setHtml(depData.c_str());
	ui.detailedEdit->setHtml(extendedInfo.c_str());
	ui.overviewEdit->setHtml(info.c_str());
}

void MainWindow::execMenu()
{
	
	tableMenu->exec(QCursor::pos());
}
void MainWindow::setTableSize()
{
	ui.packageTable->setColumnWidth(PT_INSTALLCHECK, 25);
 	ui.packageTable->setColumnWidth(PT_NAME, ui.packageTable->frameSize().width()-80);
}
void MainWindow::initPackageTable()
{

    if (ui.packageTable->columnCount() < 2)
    	ui.packageTable->setColumnCount(2);
    QTableWidgetItem *__colItem0 = new QTableWidgetItem();
    __colItem0->setText(QApplication::translate("MainWindow", "", 0, QApplication::UnicodeUTF8));
    ui.packageTable->setHorizontalHeaderItem(PT_INSTALLCHECK, __colItem0);

    QTableWidgetItem *__colItem1 = new QTableWidgetItem();
    __colItem1->setText(QApplication::translate("MainWindow", "Name", 0, QApplication::UnicodeUTF8));
    ui.packageTable->setHorizontalHeaderItem(PT_NAME, __colItem1);

   setTableSize();
}


void MainWindow::showPreferences()
{
	showCoreSettings();
//	prefBox->loadData();
//	prefBox->openInterface();
}

void MainWindow::showAbout()
{
	QMessageBox::information(this, tr("About mpkg package manager"), tr("GUI package manager (part of mpkg) ") + (QString) mpkgVersion + tr(" (build ") + (QString) mpkgBuild + tr(")\n\n(c) RPU NET (www.rpunet.ru)\nLicensed under GPL"), QMessageBox::Ok, QMessageBox::Ok);
}

void MainWindow::clearForm()
{
	ui.packageTable->clear();
	ui.packageTable->setRowCount(0);
	initPackageTable();
}

void MainWindow::updateData()
{
	clearForm();
	emit updateDatabase();
}
	
void MainWindow::quitApp()
{
	if (!actionBus.idle())
	{
		if (QMessageBox::warning(this, \
					tr("Some actions doesn't completed"), \
					tr("Some actions hasn't completed yet. Abort and exit?"), \
					QMessageBox::Yes | QMessageBox::No, \
					QMessageBox::No) \
				== QMessageBox::Yes)
		{
			actionBus.abortActions();
			currentStatus = tr("Aborting actions...").toStdString();
		}
		else
		{
			return;
		}
	}
	else currentStatus = tr("Exiting...").toStdString();
	thread->callQuit();
	StatusThread->halt();
	ErrorBus->Stop();
	thread->wait();
	StatusThread->wait();
	ErrorBus->wait();

	this->hide();
	unlockDatabase();
	qApp->quit();

}
void MainWindow::showCoreSettings()
{
	prefBox->openCore();
}
void MainWindow::commitChanges()
{
	waitUnlock();

	if (willBeOccupied - willBeFreed - get_disk_freespace() > 0)
	{
		if (QMessageBox::warning(this, \
					tr("Insufficient disk space"),
					tr("It seems that only ") + humanizeSize(get_disk_freespace()).c_str() +\
				       	tr(" is available on /, but we need ") + humanizeSize(willBeOccupied - willBeFreed).c_str() + \
					tr(", please free additional ") + humanizeSize(willBeOccupied - willBeFreed - get_disk_freespace()).c_str() + \
					tr(" to perform all requested operations. Are you sure you want to continue anyway?"), \
					QMessageBox::Yes | QMessageBox::No, QMessageBox::No)==QMessageBox::No)
		{
			return;
		}
	}
	QString installList;
	QString removeList;
	if (newStatus.size()!=(unsigned int) packagelist->size())
	{
		mDebug("uninitialized");
		return;
	}
	vector<int> newStatusCopy = newStatus;
#ifdef REALTIME_DEPTRACKER
	PACKAGE_LIST pkgListCopy = *packagelist;
	for (unsigned int i=0; i<pkgListCopy.size(); i++)
	{
		if ((unsigned int) i>=newStatus.size()) {
			mError("desync in newStatus and pkgListCopy");
			break;
		}
		pkgListCopy.get_package_ptr(i)->set_action(newStatus[i]);

	}
	thread->renderDepTree(&pkgListCopy);
	for (unsigned int i=0; i<pkgListCopy.size(); i++)
	{
		newStatusCopy[i] = pkgListCopy.get_package_ptr(i)->action();
	}
	generateStat(newStatusCopy);
#endif
	
	for (unsigned int i=0; i<newStatusCopy.size(); i++)
	{
		if (newStatusCopy[i]==ST_INSTALL)
		{
			installList+= "\n" + (QString) packagelist->at(i).get_name().c_str() + " " +\
				      (QString) packagelist->at(i).get_fullversion().c_str();
		}
		if (newStatusCopy[i]==ST_REMOVE || newStatusCopy[i] == ST_PURGE || newStatusCopy[i] == ST_UPDATE)
		{
			removeList+= "\n" + (QString) packagelist->at(i).get_name().c_str() + " " +\
				      (QString) packagelist->at(i).get_fullversion().c_str();
		}
	}

	QMessageBox msgBox;
	msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
	QString details;
	
	if (installList.length()>0) details += tr("Next packages will be installed:") + installList + "\n";
	if (removeList.length()>0) details += tr("Next packages will be removed:") + removeList + "\n";
	
	msgBox.setDetailedText(details);
	
	QString text = 	tr("Will be installed: ")+IntToStr(installQueueCount).c_str() + tr(" packages\n") + \
			tr("Will be removed: ") + IntToStr(removeQueueCount).c_str() + tr(" packages\n") + \
			tr("Will be updated: ") + IntToStr(updateQueueCount).c_str() + tr(" packages\n");
	
	if (willBeOccupied > willBeFreed)
	{
		text+= tr("Disk space will be occupied: ") + humanizeSize(willBeOccupied - willBeFreed).c_str();
	}
	else
	{
		text+=tr("Disk space will be freed: ") + humanizeSize(willBeFreed - willBeOccupied).c_str();
	}

	msgBox.setWindowTitle(tr("Action summary"));
	
	msgBox.setText(text);
	
	if (msgBox.exec() == QMessageBox::Cancel) return;

	emit commit(newStatus);
	//ui.listWidget->setEnabled(false);
}
void MainWindow::resetChanges()
{
	emit loadData();
}
void MainWindow::showAddRemoveRepositories(){
	prefBox->openRepositories();
}

void MainWindow::receiveRequiredPackages(unsigned int package_num, PACKAGE_LIST req)
{
	waitUnlock();
	if (package_num>(unsigned int) packagelist->size())
	{
		mDebug("uninitialized");
		return;
	}
	if (req.size()==1) return;
	int this_id=packagelist->at(package_num).get_id();
	for (unsigned int i=0; i<packagelist->size(); i++)
	{
		for (unsigned int t=0; t<req.size(); t++)
		{
			if (packagelist->at(i).get_id()!=this_id && packagelist->at(i).get_id()==req[t].get_id())
			{
				markChanges(i, Qt::Checked);
			}
		}		
	}
	generateStat(newStatus);
}

void MainWindow::receiveDependantPackages(unsigned int package_num, PACKAGE_LIST dep)
{
	waitUnlock();
	if (package_num>=(unsigned int) packagelist->size())
	{
		mDebug("uninitialized");
		return;
	}

	if (dep.size()==1) return;
	int this_id=packagelist->at(package_num).get_id();
	for (unsigned int i=0; i<packagelist->size(); i++)
	{
		for (unsigned int t=0; t<dep.size(); t++)
		{
			if (packagelist->at(i).get_id()!=this_id && packagelist->at(i).get_id()==dep[t].get_id())
			{
				markChanges(i, Qt::Unchecked);
			}
		}		
	}
	generateStat(newStatus);

}

void MainWindow::markChanges(int x, Qt::CheckState state, int force_state)
{
waitUnlock();

	generateStat(newStatus);
	if (state == Qt::Checked)
	{
		emit getRequiredPackages(x);
	}
	else
	{
		emit getDependantPackages(x);
	}
	//unsigned long i = x;
	PACKAGE *_p = packagelist->getPackageByTableID(x);
	unsigned long i = packagelist->getRealNum(x);
	if (i >= newStatus.size())
	{
		mError ("overflow");
		return;
	}


	if (force_state>=0)
	{
		if (force_state == ST_NONE)
		{
			newStatus[i]=force_state;
		}
		if (!_p->installed() && !_p->installedVersion.empty() && force_state == ST_INSTALL)
		{
			for (unsigned int t=0; t<packagelist->size(); t++)
			{
				if (packagelist->at(t).get_name()==_p->get_name())
				{
					newStatus[t]=ST_UPDATE;
					//markChanges(packagelist->getTableID(t), Qt::Unchecked, ST_UPDATE);
					break;
				}
			}
		}

		if (_p->installed() && force_state == ST_UPDATE)
		{
			newStatus[i]=force_state;
			currentStatus=tr("Package queued to update").toStdString();
			state = Qt::Unchecked;
		}

		if (_p->installed() && force_state == ST_REMOVE)
		{
			newStatus[i]=force_state;
			currentStatus=tr("Package queued to remove").toStdString();
			state = Qt::Unchecked;
		}
		if (_p->configexist() && force_state == ST_PURGE)
		{
			newStatus[i] = force_state;
			currentStatus = tr("Package queued to purge").toStdString();
			state = Qt::Unchecked;
		}
		if (!_p->installed() && force_state == ST_INSTALL)
		{
			newStatus[i] = force_state;
			currentStatus = tr("Package queued to install").toStdString();
			state = Qt::Checked;
		}
	}
	else
	{
		if (_p->installed())
		{

			switch(_p->action())
			{
				case ST_NONE:
					if (state == Qt::Checked)
					{
						newStatus[i]=ST_NONE;
						currentStatus=tr("Package keeped in system").toStdString();
					}
					else
					{
						newStatus[i]=ST_REMOVE;
						currentStatus=tr("Package queued to remove").toStdString();
					}
					break;
				case ST_UPDATE:
				case ST_REMOVE:
					if (state == Qt::Checked)
					{
						newStatus[i]=ST_NONE;
						currentStatus=tr("Package removed from remove queue").toStdString();
					}
					else
					{
						newStatus[i]=ST_REMOVE;
						currentStatus=tr("Package queued to remove").toStdString();
					}
					break;
				case ST_PURGE:
					if (state == Qt::Checked)
					{
						newStatus[i]=ST_NONE;
						currentStatus=tr("Package removed from purge queue").toStdString();
					}
					else
					{
						newStatus[i]=ST_PURGE;
						currentStatus=tr("Package queued to purge").toStdString();
					}
					break;
				default:
					currentStatus=tr("Unknown condition").toStdString();
			}
		} // if(_p->installed())
		else
		{
			switch(_p->action())
			{
				case ST_INSTALL:
				case ST_NONE:
					if (!_p->installed() && !_p->installedVersion.empty())
					{
						for (unsigned int t=0; t<packagelist->size(); t++)
						{
							if (packagelist->at(t).get_name() == _p->get_name() && \
									packagelist->at(t).installed() && \
									newStatus[t]==ST_NONE)
							{
								newStatus[t]=ST_UPDATE;
								//markChanges(t, Qt::Unchecked, ST_UPDATE);
								break;
							}
						}
					}


					if (state==Qt::Checked)
					{
						newStatus[i]=ST_INSTALL;
						currentStatus=tr("Package queued to install").toStdString();
					}
					else
					{
						newStatus[i]=ST_NONE;
						for (unsigned int t=0; t<newStatus.size(); t++)
						{
							if (newStatus[t]==ST_UPDATE && packagelist->at(t).get_name() == _p->get_name())
							{
								newStatus[t]=ST_NONE;
								markChanges(t, Qt::Checked, ST_NONE);
								break;
							}
						}
							
						currentStatus=tr("Package unqueued").toStdString();

					}
					break;
				case ST_PURGE:
					if (state==Qt::Checked)
					{
						newStatus[i]=ST_NONE;
						currentStatus=tr("Package queued to install").toStdString();
					}
					else
					{
						newStatus[i]=ST_PURGE;
						currentStatus=tr("Package queued to purge").toStdString();
					}
					break;
				default:
					currentStatus=tr("Unknown condition").toStdString();
			}
		} // else
	} // else (force_state)
	string package_icon;
	switch (newStatus[i])
	{
		case ST_NONE:
			if (_p->installed()) package_icon = "installed.png";
			else
			{
				if (_p->available()){
				       if (_p->isUpdate()) package_icon = "update.png";
				       else {
					       package_icon="available.png";
					       if (_p->configexist()) package_icon="removed_available.png";
				       }
				}
				else package_icon="unknown.png";
			}
			break;
		case ST_INSTALL:
			package_icon="install.png";
			break;
		case ST_UPDATE:
		case ST_REMOVE:
			package_icon="remove.png";
			break;
		case ST_PURGE:
			package_icon="purge.png";
			break;
	}
	string cloneHeader;
	if (_p->isUpdate()) cloneHeader = "<b><font color=\"red\">["+tr("update").toStdString()+"]</font></b>";
	if (_p->get_type()==PKGTYPE_SOURCE) cloneHeader += "<b><font color=\"blue\">["+tr("source").toStdString()+"]</font></b>";

	if (_p->deprecated()) package_icon = (string) "deprecated_" + package_icon;

	if (!FileExists("/usr/share/mpkg/icons/"+package_icon)) printf("Requested icon %s doesn't exist\n", package_icon.c_str());
	string pName = "<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\" ><tbody><tr><td><img src = \"/usr/share/mpkg/icons/"+package_icon+"\"></img></td><td><b>"+_p->get_name()+"</b> "\
		+_p->get_fullversion()\
		+" <font color=\"green\"> \t["+humanizeSize(_p->get_compressed_size()) + "]     </font>" + cloneHeader+\
	       	+ "<br>"+_p->get_short_description()+"</td></tr></tbody></table>";

	setTableItem(x, i, state, pName);		
	ui.applyButton->setEnabled(true);
	emit backsync(newStatus);
}

void MainWindow::loadData()
{
	ui.statLabel->setText("");
	emit loadPackageDatabase();

}

void MainWindow::initProgressBar(unsigned int stepCount)
{
	ui.progressBar->setValue(0);
	ui.progressBar->setMaximum(stepCount);
}

void MainWindow::initProgressBar2(unsigned int stepCount)
{
	ui.progressBar2->setValue(0);
	ui.progressBar2->setMaximum(stepCount);
}



void MainWindow::setBarValue(QProgressBar *Bar, int stepValue)
{
	Bar->setValue(stepValue);
}

bool MainWindow::nameComplain(int package_num, QString text)
{
waitUnlock();
	if ((unsigned int) package_num>=packagelist->size())
	{
		mDebug("uninitialized");
		return false;
	}

	QString nameMask;	
	nameMask = nameMask.fromStdString(packagelist->at(package_num).get_name());
	return nameMask.contains(text, Qt::CaseInsensitive);
}
void MainWindow::highlightCategoryList()
{
	mDebug("start");
	if (highlightState.size()!=availableTags.size())
	{
		highlightState.clear();
		highlightState.resize(availableTags.size());
		for (unsigned int i=0; i<highlightState.size(); i++)
		{
			highlightState[i]=false;
		}
	}
	QBrush hlBrush(QColor(255,255,0,150));
	QBrush noHlBrush(Qt::NoBrush);
	QFont fonttmp;
	for (unsigned int i=0; i<availableTags.size(); i++)
	{
		if (ui.quickPackageSearchEdit->text().isEmpty())
		{
			ui.listWidget->item(i)->setBackground(noHlBrush);
			fonttmp = ui.listWidget->item(i)->font();
			fonttmp.setBold(false);
			ui.listWidget->item(i)->setForeground(QBrush(QColor(0,0,0)));

			ui.listWidget->item(i)->setFont(fonttmp);
		}

		else
		
		{
			if (highlightMap[availableTags[i]])
			{
					ui.listWidget->item(i)->setBackground(hlBrush);
					fonttmp = ui.listWidget->item(i)->font();
					fonttmp.setBold(true);
					ui.listWidget->item(i)->setForeground(QBrush(QColor(0,0,0)));

					ui.listWidget->item(i)->setFont(fonttmp);
			}
			else
			{
					ui.listWidget->item(i)->setBackground(noHlBrush);
					fonttmp = ui.listWidget->item(i)->font();
					fonttmp.setBold(false);
					ui.listWidget->item(i)->setFont(fonttmp);
					ui.listWidget->item(i)->setForeground(QBrush(QColor(25,25,25,75)));

			}
		}
	}
	ui.packageTable->repaint();

	mDebug("end");

}

void CheckBox::markChanges()
{
	mw->markChanges(row, checkState());
}

CheckBox::CheckBox(MainWindow *parent)
{
	mw = parent;
}

