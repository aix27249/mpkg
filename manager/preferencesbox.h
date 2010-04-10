/**************************************************************************
 * MOPSLinux packaging system - package manager - preferences
 * $Id: preferencesbox.h,v 1.12 2007/05/30 14:29:08 i27249 Exp $
 * **********************************************************************/
#ifndef PREF_BOX_H
#define PREF_BOX_H

#include "ui_preferencesbox.h"
#include <QtGui>
#include <mpkg/libmpkg.h>
#define REP_ADDMODE 0
#define REP_EDITMODE 1
class PreferencesBox: public QWidget
{
	Q_OBJECT
	public:
		PreferencesBox(mpkg *mDB);
		Ui::preferencesBox ui;

		vector<bool>repStatus;
		vector<bool>oldRepStatus;
	public slots:
		void repTableChanged();
		void openAccounts();
		void openCore();
		//void openUpdates();
		void openRepositories();
		//void openInterface();
		void loadData();
		void applyConfig();
		
		void addRepositoryShow();
		void editRepositoryShow();
		void applyRepositoryChanges();
		void cancelRepositoryEdit();
		void changeRepositoryType();
		void switchAuthMode();
		void delRepository();
		void okProcess();
		void cancelProcess();
		void urlHelpSearchShow();
		void sysCacheHelpShow();
		void sysDBHelpShow();
		void sysScriptsHelpShow();
		void sysRootHelpShow();
		void detectCdromVolnameCall();
		void recvCdromVolname(string volname);
		void switchPrelinkRandomization();

	signals:
		void getCdromName();
		void updatePackageData();
		
	private:
		mpkg *mDb;
		unsigned int editMode;
		unsigned int editingRepository;
		bool repositoryChangesMade;
};

class RCheckBox: public QCheckBox
{
	Q_OBJECT
	public:
		RCheckBox(PreferencesBox *parent, int rowNum);
	public slots:
		void markChanges();
		void setRow(int rowNum);
		int getRow();
	private:
		int row;
		PreferencesBox *mw;

	
};


#endif
