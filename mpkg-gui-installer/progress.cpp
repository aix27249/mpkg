#include "progress.h"
#include <mpkg/libmpkg.h>
ProgressWindow::ProgressWindow() {
	ui.setupUi(this);
	timer = new QTimer(this);
	timer->setInterval(100);
	connect(timer, SIGNAL(timeout()), this, SLOT(readData()));
}
ProgressWindow::~ProgressWindow() {
}

void ProgressWindow::readData() {
	setProgressMaximum((int) pData.getTotalProgressMax());
	QString text = QString::fromStdString(currentStatus) + ": " + QString::fromStdString(currentItem);
	setData(text, (int) pData.getTotalProgress());
}

void ProgressWindow::setData(const QString &text, int progress) {
	ui.label->setText(text);
	ui.progressBar->setValue(progress);
}
void ProgressWindow::setProgressMaximum(int max) {
	ui.progressBar->setMaximum(max);
}
