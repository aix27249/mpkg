#include "parted_tools.h"
#include "raidtool.h"
#include <fstream>
// Partition table and devices cache
vector<TagPair> deviceCache;
bool deviceCacheActual=false;
vector<pEntry> partitionCache;
bool partitionCacheActual=false;
vector<string> cdromList;

vector<string> getCdromList() {
	string cdlist = get_tmp_file();
	system("getcdromlist.sh " + cdlist + " 2>/dev/null >/dev/null");
	vector<string> ret = ReadFileStrings(cdlist);
	for (unsigned int i=0; i<ret.size(); i++)
	{
		ret[i]="/dev/"+ret[i];
	}
	unlink(cdlist.c_str());
	return ret;
}
bool checkIfCd(string devname) {
	for (unsigned int i=0; i<cdromList.size(); i++) {
		if (cdromList[i]==devname)	return true;
	}
	return false;
}
void ped_device_probe_all_workaround() {
	ncInterface.uninit();
	std::ifstream proc("/proc/partitions");
	if (!proc) return;
	string line;
	size_t p;
	size_t i=0;
	while (getline(proc, line)) {
		if (i<2) {
			i++;
			continue;
		}
		p = line.find_last_of(" \t");
		if (p==std::string::npos || p>=line.length()) continue;
		else line = "/dev/" + line.substr(p+1);
		if (line.find_first_of("0123456789")==std::string::npos) ped_device_get(line.c_str());
	}
	proc.close();
}

vector<TagPair> getDevList()
{
	// Using cache if it has actual state
	if (deviceCacheActual) return deviceCache;
	ncInterface.showInfoBox(_("Looking for hard drives and partitions..."));
	vector<TagPair> ret;
	// FIXME: This is a workaround function
	// When libparted bug 194 is fixed, remove code to read:
	//           /proc/partitions
	//        This was a problem with no floppy drive yet BIOS indicated one existed.
	//        http://parted.alioth.debian.org/cgi-bin/trac.cgi/ticket/194
	
	// ped_device_probe_all();
	ped_device_probe_all_workaround();

	PedDevice *tmpDevice=NULL;
	bool enumFinished=false;
	string description;
	vector<RaidArray> raidArrays = getActiveRaidArrays();
	bool skipThisRaid = false;
	


	while(!enumFinished)
	{
		tmpDevice = ped_device_get_next(tmpDevice);
		if (tmpDevice==NULL) enumFinished=true;
		else
		{
			if (tmpDevice->length!=0 && !checkIfCd(tmpDevice->path)) {
				description = tmpDevice->model;
				if (description == "Unknown") {
					for (unsigned int i=0; i<raidArrays.size(); ++i) {
						skipThisRaid = false;
						if (raidArrays[i].md_dev == tmpDevice->path) {
							skipThisRaid = true;
							description = raidArrays[i].extra_description;
							/*description = "RAID-массив " + raidArrays[i].level + " [";
							for (unsigned int d=0; d<raidArrays[i].devices.size(); ++d) {
								if (raidArrays[i].devices[d].length()>strlen("/dev/")) description += raidArrays[i].devices[d].substr(strlen("/dev/"));
								else description += raidArrays[i].devices[d];
								if (d != raidArrays[i].devices.size()-1) description += "|";
							}
							description += "]";*/
						}
					}
				}
				if (!skipThisRaid) ret.push_back(TagPair(tmpDevice->path, \
							description + (string) ", " + \
							IntToStr((tmpDevice->length * (double) (tmpDevice->sector_size/(double) 1048576))/1024) + \
							" Gb"));
			}

		}
	}
	// Cache management
	deviceCache = ret;
	deviceCacheActual = true;
	// returning data
	return ret;
}


vector<pEntry> getGoodPartitions(vector<string> goodFSTypes, bool includeRaidComponents)
{
	vector<pEntry> ret;

	bool skipRaidComp;

	vector<RaidArray> activeArrays = getActiveRaidArrays();
	vector<PedPartition *> raidPartList;
	if (partitionCacheActual) {
		ret.clear();
		for (unsigned int i=0; i<partitionCache.size(); i++) {
			skipRaidComp = false;
			if (!includeRaidComponents) {
				for (unsigned int r=0; r<activeArrays.size() && !skipRaidComp; ++r) {
					for (unsigned int d=0; d<activeArrays[r].devices.size(); ++d) {
						if (partitionCache[i].devname==activeArrays[r].devices[d]) {
							skipRaidComp = true;
							break;
						}
					}
				}
			}
			if (goodFSTypes.empty() && !skipRaidComp) ret.push_back(partitionCache[i]);
			for (unsigned int z=0; z<goodFSTypes.size() && !skipRaidComp; z++) {
				if (partitionCache[i].fstype == goodFSTypes[z])
					ret.push_back(partitionCache[i]);
			}
		}
		// Check for goodFSTypes and filter return if needed
		/*if (goodFSTypes.empty()) return partitionCache;
		for (unsigned int i=0; i<partitionCache.size(); i++) {
			for (unsigned int z=0; z<goodFSTypes.size(); z++) {
				if (partitionCache[i].fstype == goodFSTypes[z])
					ret.push_back(partitionCache[i]);
			}
		}*/
		return ret;
	}
	ncInterface.showInfoBox(_("Looking for partitions on hard drives..."));
	mDebug("start");
	ped_device_probe_all_workaround();
	vector<PedDevice *> devList;
	vector<PedDisk *> partList;
	vector<PedDiskType *> devListPTableTypes;
	vector<PedPartition *> tmpPartList;
	vector< vector<PedPartition *> > partitionList;
	vector<PedDevice *> raidDeviceListMap;
	bool enumFinished=false;
	bool partFinished=false;
	PedDevice *tmpDevice=NULL;
	PedDisk *tmpDisk=NULL;
	PedPartition *tmpPartition=NULL;
	//int x=0;
	while (!enumFinished)
	{
		tmpDevice = ped_device_get_next(tmpDevice);
		if (tmpDevice == NULL)
		{
			enumFinished=true;
		}
		else
		{
			if (tmpDevice->length!=0 && !checkIfCd(tmpDevice->path))
			{
				devListPTableTypes.push_back(ped_disk_probe(tmpDevice));
				for (unsigned int i=0; i<activeArrays.size(); ++i) {
					if (tmpDevice->path == activeArrays[i].md_dev) {
						raidDeviceListMap.push_back(tmpDevice);
					}
				}
				//if (skipThisRaid) continue;
				tmpDisk = ped_disk_new(tmpDevice);
				if (tmpDisk != NULL) { // else, device doesn't contain any valid partition table
					devList.push_back(tmpDevice);
					partList.push_back(tmpDisk);
					partFinished=false;
					tmpPartition=NULL;
					tmpPartList.clear();
					while (!partFinished)
					{
						tmpPartition=ped_disk_next_partition(tmpDisk, tmpPartition);
						if (tmpPartition==NULL)
						{
							partFinished=true;
						}
						else
						{
							tmpPartList.push_back(tmpPartition);
						}
					}
					partitionList.push_back(tmpPartList);
				}
			}
		}
	}
	pEntry tmpEntry;
	bool isLoop;
	for (unsigned int i=0; i<devList.size(); i++) {
		if (!ped_disk_probe(devList[i])) continue;
		isLoop = false;
		if (string(ped_disk_probe(devList[i])->name) == "loop") {
			isLoop = true;
		}

		for (unsigned int t=0; t<partitionList[i].size(); t++)	{
			if (partitionList[i][t]->num>0 && partitionList[i][t]->type!=PED_PARTITION_EXTENDED) {
				if (isLoop) tmpEntry.devname = devList[i]->path;
				// Check if it's a RAID partition
				/*isRaid = false;
				for (unsigned int r=0; r<raidDeviceListMap.size(); ++r) {
					if (devList[i]==raidDeviceListMap[r]) {
						tmpEntry.devname = devList[i]->path;
						isRaid = true;
						break;
					}
				}*/

				if (partitionList[i][t]->fs_type==NULL)
				{
					// Means that no filesystem is detected on partition
					if (!isLoop) tmpEntry.devname=(string) ped_partition_get_path(partitionList[i][t]);
					else tmpEntry.devname = (string) devList[i]->path;
					tmpEntry.fstype="unformatted";
					tmpEntry.fslabel="";
					if (ped_partition_is_flag_available(partitionList[i][t], PED_PARTITION_RAID)) tmpEntry.raid = ped_partition_get_flag(partitionList[i][t], PED_PARTITION_RAID);
					else tmpEntry.raid = 0;
					tmpEntry.size=IntToStr(partitionList[i][t]->geom.length*devList[i]->sector_size/1048576);
					ret.push_back(tmpEntry);
				}

				if (partitionList[i][t]->fs_type!=NULL)
				{
					if (!isLoop) tmpEntry.devname=(string) ped_partition_get_path(partitionList[i][t]);
					else tmpEntry.devname = (string) devList[i]->path;
					tmpEntry.fstype=(string) partitionList[i][t]->fs_type->name;
					if (ped_disk_type_check_feature(ped_disk_probe(devList[i]), PED_DISK_TYPE_PARTITION_NAME))
					{
						tmpEntry.fslabel=(string) ped_partition_get_name(partitionList[i][t]);
					}
					else tmpEntry.fslabel="";
					if (ped_partition_is_flag_available(partitionList[i][t], PED_PARTITION_RAID)) tmpEntry.raid = ped_partition_get_flag(partitionList[i][t], PED_PARTITION_RAID);
					else tmpEntry.raid = 0;
					tmpEntry.size=IntToStr(partitionList[i][t]->geom.length*devList[i]->sector_size/1048576);
					ret.push_back(tmpEntry);
				}

			}
		}
	}
	// Now - raid arrays
	/*for (unsigned int i=0; i<activeArrays.size(); ++i) {
		tmpEntry.devname = activeArrays[i].md_dev;
		tmpEntry.fstype = "RAID";
		tmpEntry.fslabel = "";
		tmpEntry.size = activeArrays[i].size;
		ret.push_back(tmpEntry);
	}*/
	partitionCache = ret;
	partitionCacheActual = true;
		ret.clear();
		for (unsigned int i=0; i<partitionCache.size(); i++) {
			skipRaidComp = false;
			if (!includeRaidComponents) {
				for (unsigned int r=0; r<activeArrays.size() && !skipRaidComp; ++r) {
					for (unsigned int d=0; d<activeArrays[r].devices.size(); ++d) {
						if (partitionCache[i].devname==activeArrays[r].devices[d]) {
							skipRaidComp = true;
							break;
						}
					}
				}
			}
			if (goodFSTypes.empty() && !skipRaidComp) ret.push_back(partitionCache[i]);
			for (unsigned int z=0; z<goodFSTypes.size() && !skipRaidComp; z++) {
				if (partitionCache[i].fstype == goodFSTypes[z])
					ret.push_back(partitionCache[i]);
			}
		}
	/*string types;
	for (unsigned int i=0; i<devListPTableTypes.size(); ++i) {
		types += string(devListPTableTypes[i]->name) + "\n";
	}
	ncInterface.showMsgBox(types);*/

	return ret;
}

