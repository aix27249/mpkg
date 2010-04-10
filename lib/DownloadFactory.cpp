#include "DownloadFactory.h"

void DownloadFactory::addMethodHandler(DownloadMethods method, IDownload* handler)
{
	library[ method ] = handler;
}

IDownload* DownloadFactory::getMethodHandler(DownloadMethods method)
{
	return library[ method ];
}
