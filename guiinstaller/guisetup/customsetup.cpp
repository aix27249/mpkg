#include "customsetup.h"
#include "ui_customsetup.h"
CustomSetupWidget::CustomSetupWidget(QWidget *parent) : QDialog(parent), ui(new Ui::CustomSetupDialog) {
	ui->setupUi(this);
	connect(ui->fileChooseButton, SIGNAL(clicked()), this, SLOT(chooseFile()));
}

CustomSetupWidget::~CustomSetupWidget() {

}

void CustomSetupWidget::chooseFile() {
	QString filename = QFileDialog::getOpenFileName(this, tr("Choose package list file"), "", tr(""), 0);
	if (filename.isEmpty()) return;
	ui->fileURLEdit->setText(filename);
}

QString CustomSetupWidget::customURL() {
	return ui->fileURLEdit->text();
}

bool CustomSetupWidget::isMerge() {
	return ui->mergeCheckBox->isChecked();
}

void CustomSetupWidget::setMerge(bool m) {
	if (m) ui->mergeCheckBox->setChecked(Qt::Checked);
	else ui->mergeCheckBox->setChecked(Qt::Unchecked);
}
