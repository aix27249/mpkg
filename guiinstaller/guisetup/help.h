#ifndef HELPFORM_H__
#define HELPFORM_H__
#include <QWidget>
#include <QLabel>
#include <QTextBrowser>

namespace Ui {
	class HelpFormClass;
}

class HelpForm: public QWidget {
	Q_OBJECT
	public:
		HelpForm(QWidget *parent = 0);
		~HelpForm();
		void loadText(const QString &text);
	private:
		Ui::HelpFormClass *ui;
};


#endif
