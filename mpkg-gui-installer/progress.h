#ifndef PROGRESS_H_
#define PROGRESS_H_
#include "ui_progress.h"
#include <QTimer>
class ProgressWindow: public QWidget {
	Q_OBJECT
	public:
		ProgressWindow();
		~ProgressWindow();

		QTimer *timer;
	private:
		Ui::progressWindow ui;
	public slots:
		void readData();
		void setData(const QString &, int);
		void setProgressMaximum(int);
};
#endif
