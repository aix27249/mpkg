/*
 * MOPSLinux Packaging system
 * Table item redefinition
 * $Id: tablelabel.h,v 1.3 2007/08/03 14:24:55 i27249 Exp $
 */
#ifndef TABLELABEL_H_
#define TABLELABEL_H_

#include <QLabel>
#include <QTableWidget>
#include <QListWidget>
class TableLabel: public QLabel
{

	Q_OBJECT
	protected:

		void mousePressEvent(QMouseEvent *event);
	public:
		TableLabel(QTableWidget *table);
		int row;
		QTableWidget *packageTable;
};

class ListLabel: public QLabel
{

	Q_OBJECT
	protected:
		void mousePressEvent(QMouseEvent *event);
	public:
		ListLabel(QListWidget *list, int rowID);
		int row;
		QListWidget *catList;
};

#endif
