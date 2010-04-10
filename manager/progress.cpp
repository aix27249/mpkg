/*
 *  aboutbox.cpp
 *  manager
 *
 *  Created by aix 27249 on 2/16/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#include "progress.h"

ProgressWindow::ProgressWindow()
{
	ui.setupUi(this);
}

void ProgressWindow::loadData()
{
	//printf("progress: loading data... \npData: %d\npBarArray: %d\n", pData.size(), pBarArray.size());
	bool recreate=true;
	double dtmp;
	int tmp_c;

	if (pData.size()>0)
	{

		if (pData.size()!=pBarArray.size()) recreate=true;
		if (recreate)
		{
			ui.progressTable->clearContents();
			ui.progressTable->setRowCount(pData.size());
			/*for (unsigned int i=0; i<pBarArray.size(); i++)
			{
				delete pBarArray.at(i);
			}*/
			pBarArray.clear();
			pBarArray.resize(pData.size());
			for (unsigned int i=0; i<pData.size(); i++)
			{
				QProgressBar *pBar = new QProgressBar;
				pBarArray.push_back(pBar);
				dtmp = 100 * (pData.itemProgress.at(i)/pData.itemProgressMaximum.at(i));
				tmp_c = (int) dtmp;

				pBar->setMaximum(100);
				pBar->setValue(tmp_c);
				ui.progressTable->setItem(i,0,new QTableWidgetItem(pData.itemName.at(i).c_str()));
				ui.progressTable->setItem(i,1,new QTableWidgetItem(pData.itemCurrentAction.at(i).c_str()));
				ui.progressTable->setCellWidget(i,2,pBar);
			}
		}
		/*printf("Setting values using non-creative method\n");
		for (unsigned int i=0; i<pData.size(); i++)
		{
			printf("Setting values...\n");
			ui.progressTable->item(i,0)->setText(pData.itemName.at(i).c_str());
			ui.progressTable->item(i,1)->setText(pData.itemCurrentAction.at(i).c_str());
			printf("Trying to set local maximum to %d\n", pData.itemProgressMaximum.at(i));
			//ui.progressTable->cellWidget(i,2)->setMaximum(pData.itemProgressMaximum.at(i));
			printf("Trying to set local progress to %d\n", pData.itemProgress.at(i));
			//ui.progressTable->cellWidget(i,2)->setValue(pData.itemProgress.at(i));
		}*/
		dtmp = pData.getTotalProgress()/pData.getTotalProgressMax();
		tmp_c = (int) dtmp;
		ui.totalProgress->setMaximum(100);
		ui.totalProgress->setValue(tmp_c);
		string currentAct;
		if (pData.currentItem>=0) currentAct = "<b>"+pData.itemCurrentAction.at(pData.currentItem)+" "+pData.itemName.at(pData.currentItem)+"</b>";
		ui.currentLabel->setText(currentAct.c_str());
	}
}
		

