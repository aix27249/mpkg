#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_
#include <QtGui>
#include <mpkg/libmpkg.h>
#include <mpkg-parted/mpkg-parted.h>
#include <mpkg/errorhandler.h>
#include "thread.h"
class QListWidgetItem;

namespace Ui {
	class MainWindowClass;
}
class QSettings;
class QTimer;


class MainWindow: public QMainWindow {
	Q_OBJECT
	public:
		MainWindow(QWidget *parent = 0);
		~MainWindow();
	private:
		Ui::MainWindowClass *ui;
		SetupThread thread;

		int currentPhoto;
		QTimer *timer;
	public slots:
		void showError(const QString &);
		void finish();
		void changePhoto();
		void reboot();
		void minimizeWindow();
		void maximizeWindow();
		void closeEvent(QCloseEvent *event);
		void showLog();


		void errorHandler(ErrorDescription err, const QString& details);

	signals:
		void sendErrorResponce(MpkgErrorReturn);

	
};
#endif // MAINWINDOW_H_
