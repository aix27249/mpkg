#ifndef MPKG_PARTED_H__
#define MPKG_PARTED_H__

#include <parted/parted.h>
#include <mpkgsupport/mpkgsupport.h>

struct pEntry
{
	string devname;
	string fstype;
	string fslabel;
	string size;
	string freespace;
	int raid;
};
struct pDevList {
	string devname;
	string description;
	string size;
	string ptableType;
};
vector<TagPair> getDevList();
vector<pEntry> getPartitionList();
vector<pEntry> getGoodPartitions(vector<string> goodFSTypes, bool includeRaidComponents=false);

vector<string> getCdromList();
bool checkIfCd(string devname);

void ped_device_probe_all_workaround();

// Partition table and devices cache
extern vector<TagPair> deviceCache;
extern bool deviceCacheActual;
extern vector<pEntry> partitionCache;
extern bool partitionCacheActual;

extern vector<string> cdromList;


#endif //MPKG_PARTED_H__

