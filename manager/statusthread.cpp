/****************************************************************************
 * MOPSLinux packaging system
 * Package manager - status messages thread
 * $Id: statusthread.cpp,v 1.1 2007/05/30 12:51:19 i27249 Exp $
 * *************************************************************************/
#include "corethread.h"

void statusThread::recvRedrawReady(bool flag)
{
	redrawReady=flag;
}

void statusThread::setPDataActive(bool flag)
{
	pDataActive=flag;
}

statusThread::statusThread()
{
	TIMER_RES = 400;
	idleTime=0;
	idleThreshold=40;
	enabledBar = false;
	enabledBar2 = false;
	show();
	pDataActive=false;
	redrawReady=true;
}

void statusThread::run()
{
	int tmp_c, tmp_c2;
	double dtmp, dtmp2;
	string dlStatus;
	forever 
	{
		switch(action)
		{
			case STT_Run:
				if (pData.size()>0 && redrawReady)
				{
					emit showProgressWindow(true);
					emit loadProgressData();
				}
				if (pData.size()==0) emit showProgressWindow(false);
			
				emit setStatus((QString) currentStatus.c_str());
				if (!actionBus.idle())
				{
					emit setIdleButtons(false);
					emit setSkipButton(actionBus.skippable(actionBus.currentProcessingID()));
					if (pData.size()>0) TIMER_RES = 200;
					else TIMER_RES=50;
					dtmp = 100 * (actionBus.progress()/actionBus.progressMaximum());
					tmp_c = (int) dtmp;
					
					if (!enabledBar)
					{
						emit enableProgressBar();
						if (actionBus.progressMaximum()==0)
						{
							emit initProgressBar(0);
						}
						else
						{	emit initProgressBar(100);
							enabledBar = true;
						}
					}
					else
					{
						if (actionBus.progress()>=0) emit setBarValue(tmp_c);
					}
				}
				else
				{
					emit setSkipButton(false);
					emit setIdleButtons(true);
					TIMER_RES=400;
					//if (enabledBar)
					//{
						emit disableProgressBar();
						enabledBar = false;
					//}
				}
				
				if (pData.size()>0)
				{
					dtmp2 = 100 * (pData.getTotalProgress()/pData.getTotalProgressMax());
					tmp_c2 = (int) dtmp2;
					if (!enabledBar2)
					{
						emit initProgressBar2(100);
						emit enableProgressBar2();
						enabledBar2 = true;
					}
					else
					{
						emit setBarValue2(tmp_c2);
					}
				}
				else
				{
					//if (enabledBar)
					//{
						emit disableProgressBar2();
						enabledBar2 = false;
					//}
				}

				break;
			case STT_Pause:
				break;
			case STT_Stop:
				return;
		}
		msleep(TIMER_RES);
	}
}

void statusThread::show()
{
	action = STT_Run;
}

void statusThread::hide()
{
	action = STT_Pause;
}

void statusThread::halt()
{
	action = STT_Stop;
}

