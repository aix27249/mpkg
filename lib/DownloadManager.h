#ifndef _DOWNLOAD_MANAGER_H_
#define _DOWNLOAD_MANAGER_H_

#include <string>
using namespace std;

#include "IDownload.h"
//#include "DownloadFactory.h"
#include "HttpDownload.h"
//typedef HttpDownload* (*GetHandler)();


//extern HttpDownload *g_pCurrentMethod = NULL;
//extern DownloadFactory *g_pDownloadFactory = NULL;

//HttpDownload*  InitializeDownloadObjects(DownloadFactory* factory);
DownloadResults CommonGetFile( string url, string output);//, void *callback = NULL); 
DownloadResults CommonGetFileEx( DownloadsList &list, string *itemname);//, ActionBus *aaBus=&actionBus, ProgressData *prData=&pData); 

#endif


