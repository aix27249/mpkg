#ifndef MECHANICS_H__
#define MECHANICS_H__
#include <mpkg/libmpkg.h>
#include <mpkg-parted/mpkg-parted.h>
#include <mpkg-parted/raidtool.h>
#include <mpkg-parted/lvmtool.h>


class TextSetupMechanics {
	public:
		TextSetupMechanics();
		~TextSetupMechanics();

		vector<CustomPkgSet> getCustomPkgSetList(const string &pkgsource, string *result_pkgsource, string *volname, string *rep_location);

		bool checkNvidiaLoad();
		
		void updatePartitionLists();
		string getLABEL(const string& dev);
		
		vector<TagPair> drives;
		vector<pEntry> partitions;
		vector<LVM_VG> lvm_groups;


};


#endif
