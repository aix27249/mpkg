#ifndef DOWNLOADFACTORY_H_
#define DOWNLOADFACTORY_H_

#include <map>

#include "IDownload.h"

class DownloadFactory {
public:
	void addMethodHandler(DownloadMethods method, IDownload* handler);
	IDownload* getMethodHandler(DownloadMethods method);

private:
	std::map<DownloadMethods, IDownload*> library;
};

#endif

