#ifndef MOUNTWIDGET_H_
#define MOUNTWIDGET_H_
#include <QtGui>
#include <mpkg/libmpkg.h>
#include <mpkg/errorhandler.h>
#include <mpkg-parted/mpkg-parted.h>
namespace Ui {
	class MountWidgetClass;
};

class MountWidget: public QDialog {
	Q_OBJECT
	public:
		MountWidget(QWidget *parent = 0);
		~MountWidget();
		vector<pEntry> *partitions;
		QString selectedMountPoint;
	private:
		Ui::MountWidgetClass *ui;
		
	public slots:
		void ok();
		void cancel();
		void init(vector<pEntry> *);

};

#endif // MAINWINDOW_H_
