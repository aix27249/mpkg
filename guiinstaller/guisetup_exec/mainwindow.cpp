#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QCheckBox>
#include <QMessageBox>
#include <QSettings>
#include <QListWidgetItem>
#include <QTimer>
//#include <QtSvg/QSvgWidget>

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



MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindowClass) {
	mpkgErrorHandler.registerErrorHandler(qtErrorHandler);
	guiObject = this;
	ui->setupUi(this);
	setWindowState(Qt::WindowMaximized);
	show();
	changePhoto();
	connect(&thread, SIGNAL(setSummaryText(const QString &)), ui->currentSummaryLabel, SLOT(setText(const QString &)));
	connect(&thread, SIGNAL(setDetailsText(const QString &)), ui->currentDetailsLabel, SLOT(setText(const QString &)));
	connect(&thread, SIGNAL(setProgress(int)), ui->progressBar, SLOT(setValue(int)));
	connect(&thread, SIGNAL(setProgressMax(int)), ui->progressBar, SLOT(setMaximum(int)));
	connect(&thread, SIGNAL(reportError(const QString &)), this, SLOT(showError(const QString &)));
	connect(&thread, SIGNAL(reportFinish()), this, SLOT(finish()));
	connect(&thread, SIGNAL(minimizeWindow()), this, SLOT(minimizeWindow()));
	connect(&thread, SIGNAL(maximizeWindow()), this, SLOT(maximizeWindow()));

	connect(ui->rebootNowButton, SIGNAL(clicked()), this, SLOT(reboot()));
	connect(ui->rebootLaterButton, SIGNAL(clicked()), qApp, SLOT(quit()));
	currentPhoto = 0;
	timer = new QTimer;
	timer->setInterval(20000);
	connect(timer, SIGNAL(timeout()), this, SLOT(changePhoto()));
	timer->start();
	thread.start();

}

MainWindow::~MainWindow() {
}

void MainWindow::showError(const QString &err) {
	QMessageBox::critical(this, tr("Fatal error"), err);
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
	if (FileExists("/usr/share/setup/images/" + IntToStr(currentPhoto+1) + ".png")) {
		currentPhoto++;
	}
	else {
		if (currentPhoto>0) currentPhoto=0;
		else return;
	}
	//ui->svgWidget->load(QString::fromStdString("/usr/share/setup/images/" + IntToStr(currentPhoto) + ".svg"));
	ui->imageLabel->setPixmap(QPixmap(QString::fromStdString("/usr/share/setup/images/" + IntToStr(currentPhoto) + ".png")).scaledToHeight(300));
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
