/*
 *  loading.h
 *  manager
 *
 *  Created by aix 27249 on 2/16/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef DBBOX_H
#define DBBOX_H
#include "ui_db.h"
#include <mpkg/libmpkg.h>

class DatabaseBox: public QWidget
{
	Q_OBJECT
public:
	DatabaseBox ();
	
public:
	Ui::databaseBox ui;
	mpkg *mDb;
	mpkg *recreateDb();
	void commit();
};
#endif


