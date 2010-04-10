/*
 *  loading.h
 *  manager
 *
 *  Created by aix 27249 on 2/16/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef LBOX_H
#define LBOX_H
#include "ui_loading.h"

class LoadingBox: public QWidget
{
	Q_OBJECT
public:
	LoadingBox ();
	
public:
	Ui::loadingBox ui;
};
#endif


