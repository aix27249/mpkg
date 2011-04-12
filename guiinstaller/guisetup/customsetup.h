#ifndef CUSTOMSETUP_H_
#define CUSTOMSETUP_H_
#include <QtGui>
namespace Ui {
	class CustomSetupDialog;
};

class CustomSetupWidget: public QDialog {
	Q_OBJECT
	public:
		CustomSetupWidget(QWidget *parent = 0);
		~CustomSetupWidget();
		bool doMerge;
	private:
		Ui::CustomSetupDialog *ui;
		
	public slots:
		void chooseFile();
		QString customURL();

};

#endif
