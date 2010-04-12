#include "engine.h"

EngineWindow *guiObject;

MpkgErrorReturn qtErrorHandler(ErrorDescription err, const string& details) {
	return guiObject->errorHandler(err, details);
}

MpkgErrorReturn EngineWindow::errorHandler(ErrorDescription err, const string& details) {
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

EngineWindow::EngineWindow() {
	mpkgErrorHandler.registerErrorHandler(qtErrorHandler);
	guiObject = this;

	ui.setupUi(this);
	ui.continueButton->setEnabled(false);
	connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(cancel()));
	connect(ui.continueButton, SIGNAL(clicked()), this, SLOT(commit()));
	consoleMode = false;
	
	db = new mpkg;
	threadObject = new Thread(db);
	progressWindow = new ProgressWindow;
	connect(threadObject, SIGNAL(workFinished(bool)), this, SLOT(finish(bool)));
}
EngineWindow::~EngineWindow() {
	threadObject->terminate();
	threadObject->wait();
	delete db;
}

void EngineWindow::prepareData(int argc, char **argv) {
	vector<string> fname, errorList;
	QString text = tr("Errors detected:\n");
	for (int i=1; i<argc; i++) {
		fname.push_back((string) argv[i]);
	}
	int ret_install = db->install(fname, NULL, NULL, &errorList);
	int ret_dep = db->DepTracker->renderData();
	if (ret_install!=0) {
		text += tr("some packages not found or already installed\n");
	}
	if (ret_dep!=0) {
		text += tr("some packages have dependency errors\n");
	}
	PACKAGE_LIST iList = db->DepTracker->get_install_list();
	PACKAGE_LIST rList = db->DepTracker->get_remove_list();
	PACKAGE_LIST fList = db->DepTracker->get_failure_list();
	filterDupes(&iList, false);
	filterDupes(&rList, false);
	filterDupes(&fList, false);
	ui.listWidget->clear();
	string tmp;
	for (unsigned int i=0; i<iList.size(); i++) {
		tmp = iList[i].get_name() + "-" + iList[i].get_fullversion();
		ui.listWidget->addItem(new QListWidgetItem(QIcon("/usr/share/mpkg/icons/install.png"), tmp.c_str()));
	}
	for (unsigned int i=0; i<rList.size(); i++) {
		tmp = rList[i].get_name() + "-" + rList[i].get_fullversion();
		ui.listWidget->addItem(new QListWidgetItem(QIcon("/usr/share/mpkg/icons/purge.png"), tmp.c_str()));
	}
	for (unsigned int i=0; i<fList.size(); i++) {
		tmp = fList[i].get_name() + "-" + fList[i].get_fullversion();
		ui.listWidget->addItem(new QListWidgetItem(QIcon("/usr/share/mpkg/icons/warning.png"), tmp.c_str()));
	}
	for (unsigned int i=0; i<errorList.size(); i++) {
		ui.listWidget->addItem(new QListWidgetItem(QIcon("/usr/share/mpkg/icons/unknown.png"), errorList[i].c_str()));
	}
	if (iList.size()>0 && errorList.empty() && fList.size()==0) {
		ui.continueButton->setEnabled(true); 
		ui.label->setText(tr("Action summary:"));
	}
	else ui.label->setText(text);

}

void EngineWindow::finish(bool success) {
	progressWindow->hide();
	if (success) QMessageBox::information(this, tr("Completed"), tr("All operations completed successfully"));
	else QMessageBox::critical(this, tr("Error"), tr("An error occured while processing operations"));
	qApp->quit();

}

void EngineWindow::cancel() {
	threadObject->wait();
	qApp->quit();
}

void EngineWindow::commit() {
	this->hide();
	progressWindow->timer->start();
	progressWindow->show();
	
	threadObject->start();
}
