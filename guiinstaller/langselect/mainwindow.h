#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_
#include <QtGui>
namespace Ui {
	class MainWindowClass;
}

class MainWindow: public QDialog {
	Q_OBJECT
	public:
		MainWindow(QWidget *parent = 0);
		~MainWindow();
	private:
		Ui::MainWindowClass *ui;
		
	public slots:
		void ok();
};

#endif // MAINWINDOW_H_
