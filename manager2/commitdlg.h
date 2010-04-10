#ifndef COMMITDLG_H_
#define COMMITDLG_H_

#include <QtGui/QDialog>
#include <mpkg/libmpkg.h>
namespace Ui {
	class CommitDialogClass;
}

class CommitDialog: public QDialog {
	Q_OBJECT
	public:
		CommitDialog(const PACKAGE_LIST& i, const PACKAGE_LIST &r, QWidget *parent = 0);
		~CommitDialog();
	private:
		Ui::CommitDialogClass *ui;
		PACKAGE_LIST installQueue, removeQueue;
		QVector<PACKAGE *> upgradeQueue_old, upgradeQueue_new;
		void showPackageInfo(const PACKAGE*);
	private slots:
		void showInstallPackageInfo(int);
		void showRemovePackageInfo(int);
		void showUpgradePackageInfo(int);
	signals:
		void commit();
};

#endif
