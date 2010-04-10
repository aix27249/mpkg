#ifndef PARTED_TOOLS_H__
#define PARTED_TOOLS_H__

#include <parted/parted.h>
#include <mpkg/libmpkg.h>

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
vector<pEntry> getGoodPartitions(vector<string> goodFSTypes, bool includeRaidComponents=false);

vector<string> getCdromList();
bool checkIfCd(string devname);


// Partition table and devices cache
extern vector<TagPair> deviceCache;
extern bool deviceCacheActual;
extern vector<pEntry> partitionCache;
extern bool partitionCacheActual;

extern vector<string> cdromList;


#endif //PARTED_TOOLS_H__

