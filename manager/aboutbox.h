/*
 *  aboutbox.h
 *  manager
 *
 *  Created by aix 27249 on 2/16/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef ABOX_H
#define ABOX_H
#include "ui_aboutbox.h"

class AboutBox: public QWidget
{
	Q_OBJECT
public:
	AboutBox ();
	
public:
	Ui::aboutBox ui;
};
#endif


