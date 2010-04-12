#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QCheckBox>
#include <QMessageBox>
#include <QSettings>
#include <QListWidgetItem>
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
	setWindowState(Qt::WindowFullScreen);
	connect(&thread, SIGNAL(setSummaryText(const QString &)), ui->currentSummaryLabel, SLOT(setText(const QString &)));
	connect(&thread, SIGNAL(setDetailsText(const QString &)), ui->currentDetailsLabel, SLOT(setText(const QString &)));
	connect(&thread, SIGNAL(setProgress(int)), ui->progressBar, SLOT(setValue(int)));
	connect(&thread, SIGNAL(setProgressMax(int)), ui->progressBar, SLOT(setMaximum(int)));
	connect(&thread, SIGNAL(reportError(const QString &)), this, SLOT(showError(const QString &)));
	connect(&thread, SIGNAL(reportFinish()), this, SLOT(finish()));

	thread.start();

}

MainWindow::~MainWindow() {
}

void MainWindow::showError(const QString &err) {
	QMessageBox::critical(this, tr("Fatal error"), err);
}

void MainWindow::finish() {
	QMessageBox::information(this, tr("Installation finished"), tr("Install complete"));
	qApp->quit();
}


