#ifndef MPARTED_H__
#define MPARTED_H__
#include <parted/parted.h>
#include "ext2.h"
class MParted {
	public: MParted();
		~MParted();

		int showMainMenu(); // You should call this for generic partition table editor

	private:
		bool createMenu(PedPartition *partition);
		bool deleteMenu(PedPartition *partition);
		bool formatMenu(PedPartition *partition);
		bool resizeMenu(PedPartition *partition);
		void actionsMenu(PedPartition *partition);
		int applyActions(PedDisk *disk);
		void showDevMenu(PedDevice *device);

		vector<FSHandler> supportedFilesystems; // list of supported filesystems and it's handlers
		int checkFilesystemSupport(); // returns number of supported filesystems
};

#endif
