/***************************************************************************
 * MOPSLinux packaging system - package manager - preferences
 * $Id: preferencesbox.cpp,v 1.21 2007/08/24 11:36:13 i27249 Exp $
 * ************************************************************************/

#include "preferencesbox.h"
#include <QFileDialog>
//#include "mainwindow.h"
PreferencesBox::PreferencesBox(mpkg *mDB)
{
	repositoryChangesMade=false;
	mDb=mDB;
	ui.setupUi(this);
	ui.addModifyRepositoryFrame->hide();
	//ui.tabWidget->removeTab(0);
	//ui.tabWidget->removeTab(2);	
	//ui.tabWidget->removeTab(2);
	ui.fcheckRemove->hide();
	QObject::connect(ui.urlEdit, SIGNAL(textEdited(const QString &)), this, SLOT(changeRepositoryType()));
	QObject::connect(ui.repTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeRepositoryType()));
	QObject::connect(ui.authCheckBox, SIGNAL(stateChanged(int)), this, SLOT(switchAuthMode()));
	QObject::connect(ui.applyRepChangesButton, SIGNAL(clicked()), this, SLOT(applyRepositoryChanges()));
	QObject::connect(ui.cancelRepChangesButton, SIGNAL(clicked()), this, SLOT(cancelRepositoryEdit()));
	QObject::connect(ui.applyButton, SIGNAL(clicked()), this, SLOT(applyConfig()));
	QObject::connect(ui.addRepositoryButton, SIGNAL(clicked()), this, SLOT(addRepositoryShow()));
	QObject::connect(ui.editRepositoryButton, SIGNAL(clicked()), this, SLOT(editRepositoryShow()));
	QObject::connect(ui.delRepositoryButton, SIGNAL(clicked()), this, SLOT(delRepository()));
	QObject::connect(ui.okButton, SIGNAL(clicked()), this, SLOT(okProcess()));
	QObject::connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(cancelProcess()));	
	QObject::connect(ui.urlHelpSearch, SIGNAL(clicked()), this, SLOT(urlHelpSearchShow()));
	QObject::connect(ui.sysrootHelpBtn, SIGNAL(clicked()), this, SLOT(sysRootHelpShow()));
	QObject::connect(ui.syscacheHelpBtn, SIGNAL(clicked()), this, SLOT(sysCacheHelpShow()));
	QObject::connect(ui.dbfileHelpBtn, SIGNAL(clicked()), this, SLOT(sysDBHelpShow()));
	QObject::connect(ui.scriptsfolderHelpBtn, SIGNAL(clicked()), this, SLOT(sysScriptsHelpShow()));
	QObject::connect(ui.volNameDetectButton, SIGNAL(clicked()), this, SLOT(detectCdromVolnameCall()));
	QObject::connect(ui.enablePrelinkCheckBox, SIGNAL(stateChanged(int)), this, SLOT(switchPrelinkRandomization()));
}

void PreferencesBox::delRepository()
{
	int remIndex = ui.repositoryTable->currentRow();
	if (remIndex>=0)
	{
		ui.repositoryTable->removeRow(remIndex);
		vector<bool> tmpRepStatus;
		for (unsigned int i=0; i<repStatus.size(); i++)
		{
			if (i == (unsigned int) remIndex) i++;
			tmpRepStatus.push_back(repStatus[i]);
		}
		repStatus = tmpRepStatus;
	}
}

void PreferencesBox::okProcess()
{
	applyConfig();
	this->close();
}

void PreferencesBox::cancelProcess()
{
	this->close();
}

void PreferencesBox::addRepositoryShow()
{
	editMode = REP_ADDMODE;

	ui.applyRepChangesButton->setText(tr("Add"));
	ui.addRepositoryButton->setEnabled(false);
	ui.delRepositoryButton->setEnabled(false);
	ui.editRepositoryButton->setEnabled(false);
	ui.repTypeComboBox->setCurrentIndex(0);
	changeRepositoryType();
	ui.urlEdit->clear();
	ui.loginEdit->clear();
	ui.passwdEdit->clear();
	ui.volNameEdit->clear();
	ui.authCheckBox->setCheckState(Qt::Unchecked);
	ui.addModifyRepositoryFrame->show();
}

void PreferencesBox::switchAuthMode()
{
	if (ui.authCheckBox->checkState()==Qt::Unchecked)
	{
		ui.loginLabel->hide();
		ui.loginEdit->hide();
		ui.passwdlabel->hide();
		ui.passwdEdit->hide();
	}
	else
	{
		ui.loginLabel->show();
		ui.loginEdit->show();
		ui.passwdlabel->show();
		ui.passwdEdit->show();
	}
}


void PreferencesBox::changeRepositoryType()
{
	switch(ui.repTypeComboBox->currentIndex())
	{
		case 0:
			ui.urlLabel->setText(tr("URL:"));
			ui.urlHelpSearch->hide();
			ui.authCheckBox->show();
			ui.volNameEdit->hide();
			ui.volNameLabel->hide();
			ui.volNameDetectButton->hide();
			switchAuthMode();
			break;
		case 1:
			ui.urlLabel->setText(tr("Path:"));
			ui.urlHelpSearch->show();
			ui.authCheckBox->hide();
			ui.loginLabel->hide();
			ui.loginEdit->hide();
			ui.passwdlabel->hide();
			ui.passwdEdit->hide();
			ui.volNameEdit->hide();
			ui.volNameLabel->hide();
			ui.volNameDetectButton->hide();

			break;
		case 2:
			ui.urlLabel->setText(tr("Relative path:"));
			ui.urlHelpSearch->show();
			ui.authCheckBox->hide();
			ui.loginLabel->hide();
			ui.loginEdit->hide();
			ui.passwdlabel->hide();
			ui.passwdEdit->hide();
			ui.volNameLabel->show();
			ui.volNameEdit->show();
			ui.volNameDetectButton->show();
			break;
	}
}		

void PreferencesBox::detectCdromVolnameCall()
{
	//mpkgErrorReturn errRet;
	ui.volNameDetectButton->setEnabled(false);
	ui.volNameDetectButton->setText(tr("Detecting..."));
	emit getCdromName();
}

void PreferencesBox::repTableChanged()
{
	repositoryChangesMade=true;
}

void PreferencesBox::recvCdromVolname(string volname)
{
	if (volname.empty())
	{
		mDebug("no volname");
		ui.volNameDetectButton->setText(tr("Detect"));
		ui.volNameDetectButton->setEnabled(true);
		return;
	}
	ui.volNameDetectButton->setText(tr("Detect"));
	ui.volNameDetectButton->setEnabled(true);
	ui.volNameEdit->setText(volname.c_str());
}
void PreferencesBox::urlHelpSearchShow()
{

	QFileDialog fd;
	fd.setOption(QFileDialog::DontUseNativeDialog);
	QString url = fd.getExistingDirectory(this, tr("Select a repository folder"), ui.urlEdit->text());
	if (!url.isEmpty()) ui.urlEdit->setText(url);
}

void PreferencesBox::sysCacheHelpShow()
{
	string current = ui.syscacheEdit->text().toStdString();
	if (current[current.length()-1] == '/') current = current.substr(0,current.length()-1);

	QFileDialog fd;
	fd.setOption(QFileDialog::DontUseNativeDialog);
	QString sroot = fd.getExistingDirectory(this, tr("Select package cache folder"), current.c_str());
	if (!sroot.isEmpty()) ui.syscacheEdit->setText(sroot);
}

void PreferencesBox::sysDBHelpShow()
{
	string current = ui.dbfileEdit->text().toStdString();
	current = current.substr(current.find("://")+3);

	QFileDialog fd;
	fd.setOption(QFileDialog::DontUseNativeDialog);
	QString sroot = fd.getOpenFileName(this, tr("Select a database file"), current.c_str());
	if (!sroot.isEmpty()) ui.dbfileEdit->setText(sroot);
}

void PreferencesBox::sysScriptsHelpShow()
{
	string current = ui.scriptsfolderEdit->text().toStdString();
	if (current[current.length()-1] == '/') current = current.substr(0,current.length()-1);

	QFileDialog fd;
	fd.setOption(QFileDialog::DontUseNativeDialog);
	QString sroot = fd.getExistingDirectory(this, tr("Select script storage folder"), current.c_str());
	if (!sroot.isEmpty()) ui.scriptsfolderEdit->setText(sroot);
}

void PreferencesBox::sysRootHelpShow()
{
	string current = ui.sysrootEdit->text().toStdString();
	if (current[current.length()-1] == '/') current = current.substr(0,current.length()-1);

	QFileDialog fd;
	fd.setOption(QFileDialog::DontUseNativeDialog);
	QString sroot = fd.getExistingDirectory(this, tr("Select system root folder"), current.c_str());
	if (!sroot.isEmpty()) ui.sysrootEdit->setText(sroot);
}




void PreferencesBox::editRepositoryShow()
{
	ui.addModifyRepositoryFrame->show();
	ui.addRepositoryButton->setEnabled(false);
	ui.delRepositoryButton->setEnabled(false);
	ui.editRepositoryButton->setEnabled(false);

	string url = ui.repositoryTable->item(ui.repositoryTable->currentRow(), 1)->text().toStdString();
	editingRepository = ui.repositoryTable->currentRow();
	editMode = REP_EDITMODE;
	
	ui.applyRepChangesButton->setText(tr("Apply"));
	//string url_type;
	string login;
	string password;
	string urlbody;
	string volname;
	string url_type = url.substr(0,url.find("://")+3);
	if (url_type == "file://")
	{
		ui.repTypeComboBox->setCurrentIndex(1);
		changeRepositoryType();
		ui.urlEdit->setText(url.substr(url.find("://")+3).c_str());
	}
	else if (url_type == "cdrom://")
	{
		ui.repTypeComboBox->setCurrentIndex(2);
		changeRepositoryType();
		urlbody = url.substr(url.find("://")+3);
		volname = urlbody.substr(0, urlbody.find("/"));
		ui.urlEdit->setText(urlbody.substr(urlbody.find("/")-1).c_str());
		ui.volNameEdit->setText(volname.c_str());
	}
	else
	{
		ui.repTypeComboBox->setCurrentIndex(0);
		changeRepositoryType();
		if (url.find("@")!=std::string::npos)
		{
			ui.authCheckBox->setCheckState(Qt::Checked);
			switchAuthMode();
			//url_type = url.substr(0,url.find("://")+3);
			url = url.substr(url.find("://")+3);
			if (url.find(":")!=std::string::npos)
			{
				login=url.substr(0,url.find(":"));
				url = url.substr(url.find(":")+1);

				password = url.substr(0,url.find("@"));
				url = url.substr(url.find("@")+1);
			}
			else
			{
			       login = url.substr(0,url.find("@"));
			       url = url.substr(url.find("@")+1);
			}
			url = url_type + url;
			ui.loginEdit->setText(login.c_str());
			ui.passwdEdit->setText(password.c_str());
		}
		ui.urlEdit->setText(url.c_str());
	}
}

void PreferencesBox::openAccounts()
{
	loadData();
	show();
}

void PreferencesBox::openRepositories()
{
//	for (unsigned int i=0; i<1000000; i++)
//	{
		loadData();
//	}
	ui.tabWidget->setCurrentIndex(2);
	show();
}

/*void PreferencesBox::openInterface()
{
	return;
	loadData();
	ui.tabWidget->setCurrentIndex(0);
	show();
}*/

void PreferencesBox::loadData()
{
	ui.sysrootEdit->setText(mDb->get_sysroot().c_str());
	ui.syscacheEdit->setText(mDb->get_syscache().c_str());
	ui.dbfileEdit->setText(mDb->get_dburl().c_str());
	ui.runscriptsCheckBox->setChecked(mDb->get_runscripts());
	ui.cdromDeviceEdit->setText(mDb->get_cdromdevice().c_str());
	ui.mountPointEdit->setText(mDb->get_cdrommountpoint().c_str());
	ui.volNameLabel->setTextFormat(Qt::RichText);
	ui.urlLabel->setTextFormat(Qt::RichText);
	switch(mDb->get_checkFiles())
	{
		case CHECKFILES_PREINSTALL:
			ui.fcheckInstallation->setChecked(true);
			ui.fcheckRemove->setChecked(false);
			ui.fcheckDisable->setChecked(false);

			break;
		case CHECKFILES_POSTINSTALL:
			ui.fcheckInstallation->setChecked(false);
			ui.fcheckRemove->setChecked(true);
			ui.fcheckDisable->setChecked(false);

			break;
		case CHECKFILES_DISABLE:
			ui.fcheckDisable->setChecked(true);
			ui.fcheckInstallation->setChecked(false);
			ui.fcheckRemove->setChecked(false);

			break;
	}
	if (mConfig.getValue("disable_dependencies")=="yes") ui.disableDependenciesCheckBox->setChecked(true);
	else ui.disableDependenciesCheckBox->setChecked(false);
	if (mConfig.getValue("enable_download_resume")=="yes") ui.enableDownloadResumeCheckbox->setChecked(true);
	else ui.enableDownloadResumeCheckbox->setChecked(false);
	if (mConfig.getValue("enable_prelink")=="yes") ui.enablePrelinkCheckBox->setChecked(true);
	else ui.enablePrelinkCheckBox->setChecked(false);
	if (mConfig.getValue("enable_prelink_randomization")=="yes") ui.enablePrelinkRandomizationCheckBox->setChecked(true);
	else ui.enablePrelinkRandomizationCheckBox->setChecked(false);
	if (mConfig.getValue("enable_delta")=="yes") ui.enableBDeltaCheckBox->setChecked(true);
	else ui.enableBDeltaCheckBox->setChecked(false);
	ui.scriptsfolderEdit->setText(mDb->get_scriptsdir().c_str());
	vector<string> rList;
	rList = mDb->get_repositorylist();
	vector<string> drList;
        drList = mDb->get_disabled_repositorylist();
	ui.repositoryTable->clearContents();
	ui.repositoryTable->setRowCount(rList.size()+drList.size());
	oldRepStatus.clear();
	oldRepStatus.resize(rList.size()+drList.size());
	repStatus.clear();
	repStatus.resize(drList.size()+rList.size());
	vector<RCheckBox*> rCheckBoxes;
	rCheckBoxes.resize(rList.size() + drList.size());
	for (unsigned int i=0; i < rList.size(); i++)
	{
		repStatus.push_back(true);
		oldRepStatus.push_back(true);
		rCheckBoxes[i] = new RCheckBox(this, i);
		rCheckBoxes[i]->setRow(i);
		ui.repositoryTable->setCellWidget(i,0,rCheckBoxes[i]);
		ui.repositoryTable->setItem(i, 1, new QTableWidgetItem(rList[i].c_str()));
		rCheckBoxes[i]->setCheckState(Qt::Checked);
	}
	for (unsigned int i=0; i < drList.size(); i++)
	{
		repStatus.push_back(true);
		oldRepStatus.push_back(true);
		rCheckBoxes[i+rList.size()] = new RCheckBox(this, i+rList.size());
		rCheckBoxes[i+rList.size()]->setCheckState(Qt::Unchecked);
		rCheckBoxes[i+rList.size()]->setRow(i+rList.size());
		ui.repositoryTable->setCellWidget(i+rList.size(),0,rCheckBoxes[i+rList.size()]);
		ui.repositoryTable->setItem(i+rList.size(), 1, new QTableWidgetItem(drList[i].c_str()));
	}
	ui.repositoryTable->setColumnWidth(0, 32);
 	ui.repositoryTable->setColumnWidth(1, 500);
	repositoryChangesMade = false;
	int z=ui.marchComboBox->findText(mConfig.getValue("march").c_str());
	if (z!=-1) ui.marchComboBox->setCurrentIndex(z);
	else ui.marchComboBox->addItem(mConfig.getValue("march").c_str());
	
	z=ui.mtuneComboBox->findText(mConfig.getValue("mtune").c_str());
	if (z!=-1) ui.mtuneComboBox->setCurrentIndex(z);
	else ui.mtuneComboBox->addItem(mConfig.getValue("mtune").c_str());

	z=ui.olevelComboBox->findText(mConfig.getValue("olevel").c_str());
	if (z!=-1) ui.olevelComboBox->setCurrentIndex(z);
	else ui.olevelComboBox->addItem(mConfig.getValue("olevel").c_str());
	ui.numjobsEdit->setText(mConfig.getValue("numjobs").c_str());
	ui.packageOutputEdit->setText(mConfig.getValue("package_output").c_str());
	ui.sourceCacheEdit->setText(mConfig.getValue("source_cache_dir").c_str());
	ui.maintainerMailEdit->setText(mConfig.getValue("maintainer_email").c_str());
	ui.maintainerNameEdit->setText(mConfig.getValue("maintainer_name").c_str());
	switchPrelinkRandomization();
	
}

void PreferencesBox::applyRepositoryChanges()
{
	string urlString, tmp_url;
	unsigned int body_pos;

	switch(ui.repTypeComboBox->currentIndex())
	{
		case 0:
			// Checking validity of URL
			tmp_url = ui.urlEdit->text().toStdString();
			body_pos = tmp_url.find("://");
			if (body_pos == std::string::npos)
			{
				ui.urlLabel->setText("<html><font color = \"#FF0000\">"+tr("URL:")+"</color></html>");
				return;
			}
			else body_pos = body_pos + 3;
			if (ui.authCheckBox->checkState()==Qt::Checked && ui.loginEdit->text().length()>0)
			{
				urlString = tmp_url.substr(0, body_pos) + ui.loginEdit->text().toStdString();
				if (ui.passwdEdit->text().length()>0)
				{
					urlString += ":" + ui.passwdEdit->text().toStdString();
				}
				urlString += "@"+tmp_url.substr(body_pos);
			}
			else
				urlString = tmp_url;
			break;
		case 1:
			urlString = "file://";
			if (ui.urlEdit->text().length()==0)
			{
				ui.urlLabel->setText("<font color=\"#FF0000\">"+tr("Path:")+"</color>");
				return;
			}

			urlString += ui.urlEdit->text().toStdString();
			break;
		case 2:
			urlString = "cdrom://";
			if (ui.urlEdit->text().length()==0)
			{
				ui.urlLabel->setText("<font color=\"#FF0000\">"+tr("Relative path:")+"</color>");
				return;
			}

			if (ui.volNameEdit->text().length()==0)
			{
				ui.volNameLabel->setText("<font color=\"#FF0000\">"+tr("Volume name:")+"</color>");
				return;
			}
			urlString += ui.volNameEdit->text().toStdString() + "/" + ui.urlEdit->text().toStdString();
			break;
	}


	int insert_pos=ui.repositoryTable->rowCount();
	if (editMode==REP_ADDMODE)
	{
		RCheckBox *rCheckBox = new RCheckBox(this, insert_pos);
		rCheckBox->setCheckState(Qt::Checked);
		insert_pos = ui.repositoryTable->rowCount();
		ui.repositoryTable->setRowCount(insert_pos+1);
		rCheckBox->setRow(insert_pos);
		//repStatus.push_back(true);
		ui.repositoryTable->setCellWidget(rCheckBox->getRow(),0,rCheckBox);
	}
	if (editMode == REP_EDITMODE)
	{
		insert_pos = editingRepository;
	}
	ui.repositoryTable->setItem(insert_pos, 1, new QTableWidgetItem(urlString.c_str()));
	ui.addModifyRepositoryFrame->hide();
	ui.addRepositoryButton->setEnabled(true);
	ui.editRepositoryButton->setEnabled(true);
	ui.delRepositoryButton->setEnabled(true);
	repTableChanged();
}

void PreferencesBox::cancelRepositoryEdit()
{
	ui.addRepositoryButton->setEnabled(true);
	ui.editRepositoryButton->setEnabled(true);
	ui.delRepositoryButton->setEnabled(true);
	ui.addModifyRepositoryFrame->hide();
}

void PreferencesBox::openCore()
{
	loadData();
	ui.tabWidget->setCurrentIndex(0);
	show();
}

/*void PreferencesBox::openUpdates()
{
	return;
	loadData();
	ui.tabWidget->setCurrentIndex(4);
	show();
}*/

void PreferencesBox::applyConfig()
{
	mDb->set_sysroot(ui.sysrootEdit->text().toStdString());
	mDb->set_syscache(ui.syscacheEdit->text().toStdString());
	mDb->set_dburl(ui.dbfileEdit->text().toStdString());
	mDb->set_scriptsdir(ui.scriptsfolderEdit->text().toStdString());
	mDb->set_runscripts(ui.runscriptsCheckBox->checkState());
	mDb->set_cdromdevice(ui.cdromDeviceEdit->text().toStdString());
	mDb->set_cdrommountpoint(ui.mountPointEdit->text().toStdString());
	unsigned int fcheck=CHECKFILES_DISABLE;
	if (ui.fcheckInstallation->isChecked()) fcheck = CHECKFILES_PREINSTALL;
	if (ui.fcheckRemove->isChecked()) fcheck = CHECKFILES_POSTINSTALL;
	if (ui.fcheckDisable->isChecked()) fcheck = CHECKFILES_DISABLE;
	mDb->set_checkFiles(fcheck);
	vector<string>rList;
	vector<string>drList;
	for (int i=0; i < ui.repositoryTable->rowCount(); i++)
	{
		if (repStatus[i])
		{
			rList.push_back(ui.repositoryTable->item(i, 1)->text().toStdString());
		}
		else
		{
			drList.push_back(ui.repositoryTable->item(i,1)->text().toStdString());
		}
	}
	mDb->set_repositorylist(rList, drList);
	if (repositoryChangesMade)
	{
		if (QMessageBox::warning(this, tr("Repository list changed"), tr("You have modified repository list. Update package data?"), \
					QMessageBox::Yes | QMessageBox::No, \
					QMessageBox::Yes) == QMessageBox::Yes)
		{
			emit updatePackageData();
		}
	}
	repositoryChangesMade = false;
	mConfig.setValue("march", ui.marchComboBox->currentText().toStdString());
	mConfig.setValue("mtune", ui.marchComboBox->currentText().toStdString());
	mConfig.setValue("olevel", ui.olevelComboBox->currentText().toStdString());
	mConfig.setValue("numjobs", ui.numjobsEdit->text().toStdString());
	mConfig.setValue("package_output", ui.packageOutputEdit->text().toStdString());
	mConfig.setValue("source_cache_dir", ui.sourceCacheEdit->text().toStdString());
	mConfig.setValue("maintainer_email", ui.maintainerMailEdit->text().toStdString());
	mConfig.setValue("maintainer_name", ui.maintainerNameEdit->text().toStdString());
	if (ui.enableDownloadResumeCheckbox->isChecked()) mConfig.setValue("enable_download_resume", "yes");
	else mConfig.setValue("enable_download_resume", "no");
	if (ui.disableDependenciesCheckBox->isChecked()) mConfig.setValue("disable_dependencies", "yes");
	else mConfig.setValue("disable_dependencies", "no");
	if (ui.enablePrelinkCheckBox->isChecked()) mConfig.setValue("enable_prelink", "yes");
	else mConfig.setValue("enable_prelink", "no");
	if (ui.enablePrelinkRandomizationCheckBox->isChecked()) mConfig.setValue("enable_prelink_randomization", "yes");
	else mConfig.setValue("enable_prelink_randomization", "no");
	if (ui.enableBDeltaCheckBox->isChecked()) mConfig.setValue("enable_delta", "yes");
	else mConfig.setValue("enable_delta", "no");
}

void PreferencesBox::switchPrelinkRandomization() {
	if (ui.enablePrelinkCheckBox->isChecked()) ui.enablePrelinkRandomizationCheckBox->setEnabled(true);
	else ui.enablePrelinkRandomizationCheckBox->setEnabled(false);
}


RCheckBox::RCheckBox(PreferencesBox *parent, int rowNum)
{
	row=rowNum;
	mw = parent;

	QObject::connect(this, SIGNAL(stateChanged(int)), this, SLOT(markChanges()));
	QObject::connect(this, SIGNAL(stateChanged(int)), mw, SLOT(repTableChanged()));

}

void RCheckBox::markChanges()
{
	if (mw->repStatus.size()<(unsigned int) row+1) mw->repStatus.resize(row+1);
	if (checkState() == Qt::Checked) mw->repStatus[row] = true;
	else mw->repStatus[row] = false;
}
void RCheckBox::setRow(int rowNum)
{
	row = rowNum;
	if (mw->repStatus.size()<(unsigned int)row+1) mw->repStatus.resize(row+1);
	markChanges();
}
int RCheckBox::getRow()
{
	return row;
}
