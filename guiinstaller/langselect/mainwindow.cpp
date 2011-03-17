#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <mpkgsupport/mpkgsupport.h>
#include <agiliasetup.h>
MainWindow::MainWindow(QWidget *parent) : QDialog(parent), ui(new Ui::MainWindowClass) {
	if (FileExists("/var/run/guisetup_exec.pid")) {
		string pid_locked = ReadFile("/var/run/guisetup_exec.pid").c_str();
		if (isProcessRunning(pid_locked)) {
			fprintf(stderr, "Another setup process %s is alrealy running.\n", pid_locked.c_str());
			QMessageBox::critical(this, tr("Setup is already running"), tr("Setup is already running. If not, remove lock file /var/run/guisetup_exec.pid"));
			qApp->quit();
			exit(1);
		}
	}
	if (FileExists("/var/run/guisetup.pid")) {
		string pid_locked = ReadFile("/var/run/guisetup.pid").c_str();
		if (isProcessRunning(pid_locked)) {
			fprintf(stderr, "Another setup process %s is alrealy running.\n", pid_locked.c_str());
			QMessageBox::critical(this, tr("Setup is already running"), tr("Setup is already running. If not, remove lock file /var/run/guisetup.pid"));
			qApp->quit();
			exit(1);
		}
	}
	ui->setupUi(this);
	connect(ui->okButton, SIGNAL(clicked()), this, SLOT(ok()));
	connect(ui->cancelButton, SIGNAL(clicked()), qApp, SLOT(quit()));

}

MainWindow::~MainWindow() {
}

void MainWindow::ok() {
	string clang;
	if (ui->listWidget->currentItem()->text()=="English") clang="en_US.UTF-8";
	else if (ui->listWidget->currentItem()->text()=="Russian") clang="ru_RU.UTF-8";
	else if (ui->listWidget->currentItem()->text()=="Ukrainian") clang="uk_UA.UTF-8";
	
	map<string, string> settings;
	map<string, map<string, string> > partitions;
	vector<string> repositories;
	string home = getenv("HOME");
	if (!FileExists(home + "/.config")) system("mkdir -p " + home + "/.config");
	loadSettings(home + "/.config/agilia_installer.conf", settings, repositories, partitions);
	settings["language"] = clang;
	saveSettings(home + "/.config/agilia_installer.conf", settings, repositories, partitions);

	this->hide();
	system(std::string("LC_ALL=" + clang + " guisetup").c_str());
	qApp->quit();
}
