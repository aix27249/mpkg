#ifndef MPARTED_FILESYSTEMS_H__
#define MPARTED_FILESYSTEMS_H__
#include <mpkg/libmpkg.h>
#include <parted/parted.h>

#include "support.h"

// Base class for filesystem utilities handler
class FSHandler {
	public: 
		virtual ~FSHandler() {}

		virtual bool resize(PedPartition *partition, PedSector size, string *results=NULL){ return false; }
		virtual bool create(PedPartition *partition, string *results=NULL){ return false; }
		virtual bool check(PedPartition *partition, string *results=NULL){ return false; }
		virtual bool diskusage(PedPartition *partition, PedSector *used){ return false; }
		virtual bool get_label(PedPartition *partition, string *label){ return false; }
		virtual bool set_label(PedPartition *partition, const string& label){ return false; }

		bool can_resize, can_create, can_check, can_diskusage, can_getlabel, can_setlabel; // These flags indicates what features can be used
		virtual bool check_support() { return false; } // Checks if filesystem support available. Returns true if at least one of the features can be used
};

#endif
