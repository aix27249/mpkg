#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QCheckBox>
#include <QMessageBox>
#include <QSettings>
#include <QListWidgetItem>


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindowClass) {
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


