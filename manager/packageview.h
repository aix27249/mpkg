#ifndef TEST_H_
#define TEST_H_

#include <QtGui>
#include <vector>
using namespace std;
class PackageView: public QWidget
{
	Q_OBJECT

	public:
	PackageView();
	~PackageView();
	QPushButton *btn;
	QScrollArea *area;
	QWidget *list;
	QHeaderView *view;
	QGridLayout *itemLayout, *topLayout, *viewLayout;
	vector<QLabel *> item;
	vector<QCheckBox *> box;
//	vector<bool> rowVisible;
	// Real implementation
	
	public slots:
	void clear();
	void clearContents();
	void setRowHidden(unsigned int row, bool hidden=true);
	unsigned int rowCount();
	void setRowCount(unsigned int size);
	bool isRowHidden(unsigned int row);
	void setCBox(QCheckBox * boxWidget, unsigned int row);
	void setLItem(QLabel * labelWidget, unsigned int row);
	//void setRowHeight(unsigned int height);
	//void setColumnWidth(unsigned int width);
	QLabel * lCellWidget(unsigned int row);

	void test();
	//unsigned int frameSize();
	
	// unsigned int currentRow(); // 
	

};

#endif
