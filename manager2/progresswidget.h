#ifndef PROGRESSWIDGET_H__
#define PROGRESSWIDGET_H__

#include <QtGui/QWidget>
struct ItemState;
namespace Ui {
	class ProgressWidgetClass;
}

class ProgressWidget: public QWidget {
	Q_OBJECT
	public:
		ProgressWidget(QWidget *parent = 0);
		~ProgressWidget();
	private:
		Ui::ProgressWidgetClass *ui;
	/*	short slowdownCounter;
		short slowdownMultiplier;
		bool checkSlowdown(); // Returns true if cycle should be skipped*/
	public slots:
		void updateData(const ItemState &);
		void updateDataProcessing(const ItemState&);
		void cancelActions();
	signals:
		void callUpdateData(const ItemState&);
};

extern ProgressWidget *progressWidgetPtr;
void updateProgressData(ItemState);
#endif // PROGRESSWIDGET_H__
