#ifndef IDOWNLOAD_H_
#define IDOWNLOAD_H_

#include <vector>
#include "config.h"
#define DL_STATUS_OK      1
#define DL_STATUS_WAIT   -1
#define DL_STATUS_FAILED -2
#define DL_STATUS_FILE_ERROR -3

typedef enum {
	DOWNLOAD_OK = 1,
	DOWNLOAD_ERROR,
} DownloadResults;

typedef enum {
	HTTP = 1,
	HTTPS,
	FTP,
	SFTP,
	RSYNC,
} DownloadMethods;

class DownloadItem {
public:
	string file;
	std::vector<string> url_list;
	string name;
	unsigned int priority;
	int status;
	int itemID;
	double expectedSize;
	string *usedSource;
	DownloadItem();
	~DownloadItem();
};

typedef std::vector<DownloadItem> DownloadsList;





/*class IDownload {
public:
	virtual DownloadResults getFile(string url, string file, string cdromDevice = CDROM_DEVICE, string cdromMountPoint = CDROM_MOUNTPOINT) = 0;
	virtual DownloadResults getFile(DownloadsList &list, string *itemname, string cdromDevice = CDROM_DEVICE, string cdromMountPoint = CDROM_MOUNTPOINT, ActionBus *aaBus = &actionBus, ProgressData *prData = &pData) = 0;
	virtual ~IDownload() {};
};*/

#endif

