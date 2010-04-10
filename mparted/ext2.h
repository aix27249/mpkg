// mparted: ext2 filesystem support

#ifndef MPARTED_EXT2_H__
#define MPARTED_EXT2_H__

#include "filesystems.h"

class Ext2: public FSHandler {
	public:
		bool resize(PedPartition *partition, PedSector size, string *results=NULL);
		bool create(PedPartition *partition, string *results=NULL);
		bool check(PedPartition *partition, string *results=NULL);
		bool diskusage(PedPartition *partition, PedSector *used);
		bool get_label(PedPartition *partition, string *label);
		bool set_label(PedPartition *partition, const string& label);
		bool check_support();
};
#endif
