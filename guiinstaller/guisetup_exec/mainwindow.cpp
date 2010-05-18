#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QCheckBox>
#include <QMessageBox>
#include <QSettings>
#include <QListWidgetItem>
#include <QTimer>
//#include <QtSvg/QSvgWidget>
SetupThread *threadPtr;

MpkgErrorReturn qtErrorHandler(ErrorDescription err, const string& details) {
	printf("Error handler receiver: %s %s\n", err.text.c_str(), details.c_str());
	return threadPtr->errorHandler(err, details);
}

void MainWindow::errorHandler(ErrorDescription err, const QString& details) {
	QMessageBox box(this);
	QVector<QPushButton *> buttons;
	if (err.action.size()>1) {
		for (size_t i=0; i<err.action.size(); ++i) {
			buttons.push_back(box.addButton(err.action[i].text.c_str(), QMessageBox::AcceptRole));
		}
	}
	box.setWindowTitle(tr("Error"));
	box.setText(err.text.c_str());
	box.setInformativeText(details);
	box.exec();
	if (err.action.size()==1) {
		emit sendErrorResponce(err.action[0].ret);
		return;
	}
	for (int i=0; i<buttons.size(); ++i) {
		if (box.clickedButton()==buttons[i]) {
			emit sendErrorResponce(err.action[i].ret);
			return;
		}
	}
	emit sendErrorResponce(MPKG_RETURN_ABORT);
}



MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindowClass) {
	threadPtr=&thread;
	mpkgErrorHandler.registerErrorHandler(qtErrorHandler);
	ui->setupUi(this);
	//setWindowState(Qt::WindowMaximized);
	show();
	//changePhoto();
	qRegisterMetaType<ErrorDescription> ("ErrorDescription");
	qRegisterMetaType<MpkgErrorReturn> ("MpkgErrorReturn");
	connect(&thread, SIGNAL(setSummaryText(const QString &)), ui->currentSummaryLabel, SLOT(setText(const QString &)), Qt::QueuedConnection);
	connect(&thread, SIGNAL(setDetailsText(const QString &)), ui->currentDetailsLabel, SLOT(setText(const QString &)), Qt::QueuedConnection);
	connect(&thread, SIGNAL(setProgress(int)), ui->progressBar, SLOT(setValue(int)), Qt::QueuedConnection);
	connect(&thread, SIGNAL(setProgressMax(int)), ui->progressBar, SLOT(setMaximum(int)), Qt::QueuedConnection);
	connect(&thread, SIGNAL(reportError(const QString &)), this, SLOT(showError(const QString &)), Qt::QueuedConnection);
	connect(&thread, SIGNAL(reportFinish()), this, SLOT(finish()));
	//connect(&thread, SIGNAL(minimizeWindow()), this, SLOT(minimizeWindow()));
	//connect(&thread, SIGNAL(maximizeWindow()), this, SLOT(maximizeWindow()));

	connect(ui->rebootNowButton, SIGNAL(clicked()), this, SLOT(reboot()));
	connect(ui->rebootLaterButton, SIGNAL(clicked()), qApp, SLOT(quit()));

	connect(ui->showLogButton, SIGNAL(clicked()), this, SLOT(showLog()));
	connect(ui->skipMD5Button, SIGNAL(clicked()), &thread, SLOT(skipMD5()));
	connect(&thread, SIGNAL(showMD5Button(bool)), ui->skipMD5Button, SLOT(setVisible(bool)));
	connect(&thread, SIGNAL(enableMD5Button(bool)), ui->skipMD5Button, SLOT(setEnabled(bool)));
	connect(this, SIGNAL(sendErrorResponce(MpkgErrorReturn)), &thread, SLOT(receiveErrorResponce(MpkgErrorReturn)));
	connect(&thread, SIGNAL(sendErrorHandler(ErrorDescription, const QString &)), this, SLOT(errorHandler(ErrorDescription, const QString &)));
			

	// GTFO mounted cd :)
	system("umount /var/log/mount");

	currentPhoto = 0;
	//timer = new QTimer;
	//timer->setInterval(20000);
	//connect(timer, SIGNAL(timeout()), this, SLOT(changePhoto()));
	//timer->start(QThread::HighestPriority);
	thread.start();

}

MainWindow::~MainWindow() {
}

void MainWindow::showError(const QString &err) {
	QMessageBox::critical(this, tr("Fatal error"), err);
	QMessageBox::critical(this, tr("Setup failed"), tr("Setup failed and will exit now. You can look in /var/log/guisetup_exec.log to see what can cause failure."));
	qApp->quit();
}

void MainWindow::finish() {
	ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::closeEvent(QCloseEvent *event) {
	if (QMessageBox::question(this, tr("Really cancel installation?"), 
				tr("Installation is not yet complete. If you interrupt it at this point, you will probably get your system completely unusable. Are you sure?"), 
				QMessageBox::Yes|QMessageBox::No)==QMessageBox::Yes) {
	
		thread.terminate();
		event->accept();
	}
	else event->ignore();
}

void MainWindow::changePhoto() {
	return;
	if (FileExists("/usr/local/share/setup/images/" + IntToStr(currentPhoto+1) + ".png")) {
		currentPhoto++;
	}
	else {
		if (currentPhoto>0) currentPhoto=0;
		else return;
	}
	//ui->svgWidget->load(QString::fromStdString("/usr/share/setup/images/" + IntToStr(currentPhoto) + ".svg"));
	//ui->imageLabel->setPixmap(QPixmap(QString::fromStdString("/usr/local/share/setup/images/" + IntToStr(currentPhoto) + ".png")).scaledToHeight(300));
}

void MainWindow::reboot() {
	system("sync && reboot &");
	qApp->quit();
}

void MainWindow::minimizeWindow() {
	setWindowState(Qt::WindowMinimized);
}

void MainWindow::maximizeWindow() {
	setWindowState(Qt::WindowMaximized);
}

void MainWindow::showLog() {
	system("xterm -T 'Installation log' -e tail -f /var/log/guisetup_exec.log &");
}
