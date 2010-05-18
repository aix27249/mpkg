#include "mount.h"
#include "ui_mount.h"
//#include <mpkgsupport/mpkgsupport.h>
MountWidget::MountWidget(QWidget *parent) : QDialog(parent), ui(new Ui::MountWidgetClass) {
	ui->setupUi(this);
	connect(ui->okButton, SIGNAL(clicked()), this, SLOT(ok()));
	connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(cancel()));
}

MountWidget::~MountWidget() {
}


void MountWidget::ok() {
	int menu_ret = ui->listWidget->currentRow();
	if (menu_ret==-1) return;
	string mPoint = "/tmp/mpkgmount/" + partitions->at(menu_ret).devname;
	string mountcmd = "mount";
	if (partitions->at(menu_ret).fstype=="ntfs") mountcmd = "ntfs-3g";
	system("umount " + mPoint);
	system("mkdir -p " + mPoint);
	if (system(mountcmd + " " + string(partitions->at(menu_ret).devname) + " " + mPoint)==0) {
		selectedMountPoint = mPoint.c_str();
		accept();
	}
	else QMessageBox::critical(this, tr("Failed to mount partition"), tr("An error occured while mounting partition %1. Try another one.").arg(partitions->at(menu_ret).devname.c_str()));
	

}
void MountWidget::cancel() {
	reject();
}

void MountWidget::init(vector<pEntry> *_partitions) {
	partitions = _partitions;
	selectedMountPoint.clear();
	ui->listWidget->clear();
	QString label;
	for (size_t i=0; i<partitions->size(); ++i) {
		if (partitions->at(i).fslabel.empty()) label = tr("no label");
		else label = tr("label: %1").arg(partitions->at(i).fslabel.c_str());

		new QListWidgetItem(QString("%1 (%2, %3), %4").arg(partitions->at(i).devname.c_str()).arg(partitions->at(i).fstype.c_str()).arg(humanizeSize((uint64_t) atol(partitions->at(i).size.c_str())*(uint64_t) 1048576).c_str()).arg(label), ui->listWidget);
	}
	
}
