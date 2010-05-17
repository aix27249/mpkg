#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <mpkgsupport/mpkgsupport.h>
#include <QSettings>
MainWindow::MainWindow(QWidget *parent) : QDialog(parent), ui(new Ui::MainWindowClass) {
	ui->setupUi(this);
	connect(ui->okButton, SIGNAL(clicked()), this, SLOT(ok()));
	connect(ui->cancelButton, SIGNAL(clicked()), qApp, SLOT(quit()));
	if (FileExists("/var/run/guisetup_exec.pid")) {
		string pid_locked = ReadFile("/var/run/guisetup_exec.pid").c_str();
		if (isProcessRunning(pid_locked)) {
			fprintf(stderr, "Another setup process %s is alrealy running.\n", pid_locked.c_str());
			QMessageBox::critical(this, tr("Setup is already running"), tr("Setup is already running. If not, remove lock file /var/run/guisetup_exec.pid"));
			qApp->quit();
		}
	}
	if (FileExists("/var/run/guisetup.pid")) {
		string pid_locked = ReadFile("/var/run/guisetup.pid").c_str();
		if (isProcessRunning(pid_locked)) {
			fprintf(stderr, "Another setup process %s is alrealy running.\n", pid_locked.c_str());
			QMessageBox::critical(this, tr("Setup is already running"), tr("Setup is already running. If not, remove lock file /var/run/guisetup.pid"));
			qApp->quit();
		}
	}


}

MainWindow::~MainWindow() {
}

void MainWindow::ok() {
	QSettings settings("guiinstaller");
	QString clang;
	if (ui->listWidget->currentItem()->text()=="English") clang="en_US.UTF-8";
	else if (ui->listWidget->currentItem()->text()=="Russian") clang="ru_RU.UTF-8";
	else if (ui->listWidget->currentItem()->text()=="Ukrainian") clang="uk_UA.UTF-8";
	
	settings.setValue("language", clang);
	settings.sync();
	system(std::string("LC_ALL=" + clang.toStdString() + " nohup guisetup 2>&1 >/var/log/guisetup.log &").c_str());
	qApp->quit();
}
