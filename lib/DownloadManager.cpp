#include <iostream>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "debug.h"
#include "DownloadManager.h"
#include "HttpDownload.h"
void *dlhandler=NULL;

DownloadResults CommonGetFile(std::string url, std::string output)//, void *callback)
{
	HttpDownload *g_pCurrentMethod = new HttpDownload;//InitializeDownloadObjects(g_pDownloadFactory);

	assert( g_pCurrentMethod );
	mDebug("load file " + url + " to " + output);

	
	DownloadResults ret = g_pCurrentMethod->getFile(url, output); 
	return ret;
}	

DownloadResults CommonGetFileEx(DownloadsList &list, std::string *itemname)//, ActionBus *aaBus, ProgressData *prData)
{
	HttpDownload *g_pCurrentMethod = new HttpDownload;// InitializeDownloadObjects(g_pDownloadFactory);
	assert( g_pCurrentMethod );
	DownloadResults ret = g_pCurrentMethod->getFile(list, itemname); 
	return ret;
}	


