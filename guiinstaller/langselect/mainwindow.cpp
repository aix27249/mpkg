#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSettings>
MainWindow::MainWindow(QWidget *parent) : QDialog(parent), ui(new Ui::MainWindowClass) {
	ui->setupUi(this);
	connect(ui->okButton, SIGNAL(clicked()), this, SLOT(ok()));
	connect(ui->cancelButton, SIGNAL(clicked()), qApp, SLOT(quit()));

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
