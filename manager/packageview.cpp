#include "test.h"

PackageView::PackageView()
{
	topLayout = new QGridLayout(this);
	btn = new QPushButton("Hide",this);
	area = new QScrollArea(this);
	list = new QWidget;
	area->setWidget(list);
	area->setAlignment(Qt::AlignHCenter);
	area->setWidgetResizable(true);
	topLayout->addWidget(area,0,0);
	topLayout->addWidget(btn,1,0);
	itemLayout = new QGridLayout(list);
	QString t;
	setRowCount(800);
	for (unsigned int i=0; i<800; i++)
	{

		t.setNum(i);
		setCBox(new QCheckBox,i);
		setLItem(new QLabel ("Package name " + t),i);

		/*
		item.push_back(new QLabel("Package name " + t,list));
		box.push_back(new QCheckBox);
		rowVisible.push_back(true);
		itemLayout->addWidget(box[i],i,0);
		itemLayout->addWidget(item[i],i,1);
		*/
	}
	connect(btn,SIGNAL(clicked()), this, SLOT(test()));
	show();
}

PackageView::~PackageView(){}

void PackageView::test()
{
	setRowHidden(2, !isRowHidden(2));
}
void PackageView::clear()
{
	for (unsigned int i=0; i<rowCount(); i++)
	{
		setRowHidden(i);
		delete item[i];
		delete box[i];
		item[i]=NULL;
		box[i]=NULL;
	}
}
void PackageView::clearContents()
{
	clear();
	item.clear();
	box.clear();
}

void PackageView::setRowHidden(unsigned int row, bool hidden)
{
	if (hidden)
	{
		printf("Hiding row %d\n",row);
		item[row]->hide();
		box[row]->hide();
		//rowVisible[row]=false;
	}
	else
	{
		printf("Unhiding row %d\n",row);
		item[row]->show();
		box[row]->show();
		//rowVisible[row]=true;
	}
}
unsigned int PackageView::rowCount()
{
	return item.size();
}
void PackageView::setRowCount(unsigned int size)
{
	item.resize(size);
	box.resize(size);
//	rowVisible.resize(size);
}
bool PackageView::isRowHidden(unsigned int row)
{
	if (item[row]->isHidden()) printf("Row %d is hidden\n",row);
	return item[row]->isHidden();
	//return rowVisible[row];
}
void PackageView::setCBox(QCheckBox * boxWidget, unsigned int row)
{
	if (box[row]!=NULL)
	{
		box[row]->hide();
		delete box[row];
	}
	box[row]=boxWidget;
	itemLayout->addWidget(box[row],row,0);

}

void PackageView::setLItem(QLabel * labelWidget, unsigned int row)
{
	if (item[row]!=NULL)
	{
		item[row]->hide();
		delete item[row];
	}
	item[row]=labelWidget;
	itemLayout->addWidget(item[row],row,1);
}
//void setRowHeight(unsigned int height);
//void setColumnWidth(unsigned int width);
QLabel * PackageView::lCellWidget(unsigned int row)
{
	return item[row];
}

