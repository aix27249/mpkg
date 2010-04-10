/*
 *  aboutbox.h
 *  manager
 *
 *  Created by aix 27249 on 2/16/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef PROGRESSW_H
#define PROGRESSW_H
#include "ui_progresswindow.h"
#include <mpkg/libmpkg.h>
#include <QWidget>

class ProgressWindow: public QWidget
{
	Q_OBJECT
public:
	ProgressWindow();
	Ui::progressWindow ui;
public slots:
	void loadData();
		
private:
	vector<QProgressBar *> pBarArray;
};
#endif


