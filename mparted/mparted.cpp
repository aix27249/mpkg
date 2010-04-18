/*
 * mparted: libparted-based partition table editor with ncurses-based interface.
 * This is FREE SOFTWARE, licensed under terms of GPLv3+.
 * Copyleft: aix27249 <i27249@gmail.com>
 */
#include "mparted.h"
#include <mpkg/libmpkg.h>
#include "filesystems.h"
#include <fstream>
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

MParted::MParted() {
	// Some little routines: check for supported filesystems
	// First, collect all KNOWN filesystems here
	vector<FSHandler> kfs;
}

MParted::~MParted() {
}

bool MParted::createMenu(PedPartition *partition) {
	// First of all: if we have MBR-like partition table, we should check for primary/extended partition
	vector<MenuItem> typeMenu;
	
	PedPartitionType partition_type = PED_PARTITION_NORMAL;
	if (partition->type&PED_PARTITION_LOGICAL) {
		partition_type = PED_PARTITION_LOGICAL;
	}
	else {
		typeMenu.push_back(MenuItem("PRIMARY", _("Primary partition")));
		typeMenu.push_back(MenuItem("EXTENDED", _("Extended partition")));
	}
	string ptype = "PRIMARY";
	if (ped_disk_type_check_feature(ped_disk_probe(partition->disk->dev), PED_DISK_TYPE_EXTENDED) && !typeMenu.empty()) {
		ptype = ncInterface.showMenu2(_("Select partition type:"), typeMenu);
		if (ptype == "PRIMARY") partition_type = PED_PARTITION_NORMAL;
		if (ptype == "EXTENDED") partition_type = PED_PARTITION_EXTENDED;
		if (ptype == "LOGICAL") partition_type = PED_PARTITION_LOGICAL;
	}

	// Next: ask for fstype. Supported filesystem types may vary
	vector<MenuItem> fstypeMenu;
	vector<PedFileSystemType *> fstypes;
	PedFileSystemType *fs_type = ped_file_system_type_get_next(NULL);
	while (fs_type) {
		fstypes.push_back(fs_type);
		fstypeMenu.push_back(MenuItem(fs_type->name, ""));
		fs_type = ped_file_system_type_get_next(fs_type);
	}
	int fstypeSelect = 0;
       	if (!(partition_type&PED_PARTITION_EXTENDED)) fstypeSelect = ncInterface.showMenu(_("Select filesystem type: (note that not all filesystem types can be created)"), fstypeMenu);
	if (fstypeSelect==-1) return false;
	fs_type = fstypes[fstypeSelect];
	
	// And the last: we have four variants to create partition:
	// 1. Use all this space
	// 2. Use some amount at beginning of this space
	// 3. Use some abount at end of this space
	// 4. Use some abount in the middle of this space
	
	// So, let's ask user what he wants
	vector<MenuItem> modeMenu;
	modeMenu.push_back(MenuItem("ALL", _("Use all available space")));
	modeMenu.push_back(MenuItem("BEGIN", _("Use part of free space at it's start")));
	modeMenu.push_back(MenuItem("END", _("Use part of free space at it's end")));
	modeMenu.push_back(MenuItem("CUSTOM", _("Use custom size and position")));
	modeMenu.push_back(MenuItem("CANCEL", _("Go back")));
	long long sector_size = partition->disk->dev->sector_size;

	PedSector start, end;
	string mode;
	PedPartition *newPartition; // Placeholder for new partition
	while (mode.empty()) {
		mode = ncInterface.showMenu2(_("Free space: ") + humanizeSize(partition->geom.length*sector_size) + \
				_("\nStart at: ")+ humanizeSize(partition->geom.start*sector_size) + \
				_("\nEnd at: ") + humanizeSize(partition->geom.end*sector_size) + \
				_("\nPlease select, how do you want to create new partition:"), modeMenu);

		if (mode=="CANCEL") return false;
		if (mode=="ALL") {
			start = partition->geom.start;
			end = partition->geom.end;
		}
		if (mode=="BEGIN") {
			start = partition->geom.start;
			end = start + (PedSector) atol(ncInterface.showInputBox(_("Specify size of partition (in megabytes):")).c_str()) * (PedSector) 1048576/sector_size;
		}
		if (mode=="END") {
			end = partition->geom.end;
			start = end - (PedSector) atol(ncInterface.showInputBox(_("Specify size of partition (in megabytes):")).c_str()) * (PedSector) 1048576/sector_size;
		}
		if (mode=="CUSTOM") {
			start = (PedSector) atol(ncInterface.showInputBox(_("Enter start position between ") + humanizeSize(partition->geom.start*sector_size) + _(" and ") + humanizeSize(partition->geom.end*sector_size) + _(" (in megabytes):")).c_str()) * (PedSector) 1048576/sector_size;
			end = (PedSector) atol(ncInterface.showInputBox(_("Enter end position between ") + humanizeSize(partition->geom.start*sector_size) + _(" and ") + humanizeSize(partition->geom.end*sector_size) + _(" (in megabytes):")).c_str()) * (PedSector) 1048576/sector_size;
		}

		// Clear mode for loop if failure
		mode.clear();
		// Check for malformed sizes:
		if (start>=end) {
			ncInterface.showMsgBox(_("Starting position cannot be after end"));
			continue;
		}
		if (start<partition->geom.start) {
			if (!ncInterface.showYesNo(_("Starting position is outside of free space. Align it to fit free space?"))) continue;
		}
		if (end>partition->geom.end) {
			if (!ncInterface.showYesNo(_("Ending position is outside of free space. Align it to fit free space?"))) continue;
		}
		// If everything is OK, try to create partition:
		newPartition = ped_partition_new(partition->disk, partition_type, fs_type, start, end);
		if (!newPartition) {
			ncInterface.showMsgBox(_("Failed to create partition"));
			return false;
		}
		if (!ped_disk_add_partition(partition->disk, newPartition, ped_constraint_any(partition->disk->dev))) {
			ncInterface.showMsgBox(_("Failed to add partition to disk"));
			return false;
		}
		return true;
	}
	
	return true;
}

bool MParted::resizeMenu(PedPartition *partition) {
	// Partition resizing is a complex operation which consists of next parts:
	// 1. Check if partition has filesystem in it
	// 2. If filesystem exists:
	// 	2.1 If user wants to grow partition, then first increase partition geometry, and then - resize filesystem
	// 	2.2 If user wants to shrink partition, then first resize filesystem, and then partition geometry
	// It is still unknown when these actions will actually performed
	// TODO: PedTimer to show resize progress (it may be VERY long...)
	// Also we need to check filesystem capatibilities to resize. For example, many filesystems doesn't support shrink.
	
	PedConstraint *fs_resize_constraint = NULL;
	bool no_start_move = false;
	PedSector min_size = 0;
	PedSector max_size = 0;
	PedFileSystem *filesystem = NULL;
	// First, let's detect filesystem limitations if it exists. Partitions without filesystem has no specific limitations.
	if (partition->fs_type) {
		
		/* All this code is unusable now, because parted doesn't support anything at practice now... :(
		 * Sad, but we have to use external utilities manually
		// Trying to open filesystem
		filesystem = ped_file_system_open(&partition->geom);
		if (!filesystem) {
			ncInterface.showMsgBox(_("Sorry, but filesystem cannot be opened. It can be damaged or doesn't exist"));
			return false;
		}
		fs_resize_constraint = ped_file_system_get_resize_constraint(filesystem);
		if (!fs_resize_constraint) {
			ncInterface.showMsgBox(_("Sorry, but this filesystem cannot be resized"));
			return false;
		}
		// Check if start cannot be moved
		if (fs_resize_constraint->start_align->grain_size == 0 || fs_resize_constraint->start_range->length==1) {
			no_start_move = true;
		}
		// Check minimum filesystem size
		min_size = fs_resize_constraint->min_size;
		*
		*/ // End of unusable code
	}
	// Determine available space to grow
	PedConstraint * grow_constraint = ped_constraint_new_from_min(&partition->geom);
	if (!grow_constraint) {
		ncInterface.showMsgBox(_("Cannot determine size constrains for partition"));
		return false;
	}
	max_size = grow_constraint->max_size;
	PedSector usersize=atol(ncInterface.showInputBox(_("Current partition size: ") + humanizeSize(partition->geom.length) + \
				_("\nMinimum size: ") + humanizeSize(min_size) + \
				_("\nMaximum size: ") + humanizeSize(max_size) + \
				_("\n\nPlease, enter new size:")).c_str());
	
	if (usersize==0) {
		ncInterface.showMsgBox(_("Size cannot be null"));
		return false;
	}
	return true;
}

bool MParted::deleteMenu(PedPartition *partition) {
	if (ncInterface.showYesNo(_("Really delete partition ") + string(ped_partition_get_path(partition)) + "?")) {
		ped_disk_delete_partition(partition->disk, partition);
		return true;
	}
	return false;
}

bool MParted::formatMenu(PedPartition *partition) {
	vector<MenuItem> fstypeMenu;
	vector<PedFileSystemType *> fstypes;
	PedFileSystemType *fs_type = ped_file_system_type_get_next(NULL);
	while (fs_type) {
		fstypes.push_back(fs_type);
		fstypeMenu.push_back(MenuItem(fs_type->name, ""));
		fs_type = ped_file_system_type_get_next(fs_type);
	}
	int fstypeSelect = ncInterface.showMenu(_("Select filesystem type: (note that not all filesystem types can be created)"), fstypeMenu);
	if (fstypeSelect==-1) return false;
	fs_type = fstypes[fstypeSelect];
	
	if (ncInterface.showYesNo(_("Really format partition ") + string(ped_partition_get_path(partition)) + _("?\nWARNING: This action will be performed IMMEDIATELY!"))) {
		if (ped_file_system_create(&partition->geom, fs_type, NULL)) return true;
	}
	return false;
}

void MParted::actionsMenu(PedPartition *partition) {
	// Check partition type and fill menu with appropriate options
	vector<MenuItem> menu;
	if (partition->type&PED_PARTITION_FREESPACE) {
		ncInterface.setSubtitle(_("Free space"));
		menu.push_back(MenuItem("CREATE", _("Create new partition")));
	}
	else {
		ncInterface.setSubtitle(ped_partition_get_path(partition));
		menu.push_back(MenuItem("DELETE", _("Delete partition")));
		//menu.push_back(MenuItem("RESIZE", _("Resize partition"))); // Not implemented yet
		if (!(partition->type&PED_PARTITION_EXTENDED)) menu.push_back(MenuItem("FORMAT", _("Format partition")));
	}
	// Add option to cancel
	menu.push_back(MenuItem("CANCEL", _("Back to partitions menu")));

	string action;
	while (action!="CANCEL") {
		action = ncInterface.showMenu2(_("Select action:"), menu);

		// Let's see which action user wants and call appropriate sub-menu
		if (action == "CANCEL") return;
		if (action == "CREATE") {
			if (createMenu(partition)) return;
		}
		if (action == "DELETE") {
			if (deleteMenu(partition)) return;
		}
		if (action == "FORMAT") {
			if (formatMenu(partition)) return;
		}
		if (action == "RESIZE") {
			if (resizeMenu(partition)) return;
		}
	}
}

int MParted::applyActions(PedDisk *disk) {
	if (ncInterface.showYesNo(_("Are you sure you want to apply actions?"))) {
		int ret = ped_disk_commit_to_dev(disk);
		ped_disk_commit_to_os(disk);
		return ret;
	}
	else return 2;
}
void MParted::showDevMenu(PedDevice *device) {
	// Note: all changes are to be applied within one disk editing session.
	// Probe for partition table on device
	PedDiskType *diskType = ped_disk_probe(device);
	PedDisk *disk;
	// If no partition table detected - ask to create it
	if (!diskType) {
		if (ncInterface.showYesNo(_("No partition table detected on drive ") + string(device->path) + _(". Do you want to create a new one?\nWARNING: Partition table will be immediately written to drive!"))) {
			// Let's enumerate all known partition table types
			vector<PedDiskType *> types;
			vector<MenuItem> typeMenu;
			PedDiskType* type = ped_disk_type_get_next(NULL);
			while (type) {
				types.push_back(type);
				typeMenu.push_back(MenuItem(type->name, ""));
				type = ped_disk_type_get_next(type);
			}
			type = ped_disk_type_get(ncInterface.showMenu2(_("Select partition table type:"), typeMenu).c_str());
			disk = ped_disk_new_fresh(device, type);
			if (!disk) {
				ncInterface.showMsgBox(_("Failed to create in-memory partition table"));
				return;
			}
			ped_disk_commit_to_dev(disk);
			ped_disk_commit_to_os(disk);
		}
		else return;
	}
	else disk = ped_disk_new(device); // Otherwise, read it from device.

	// Let's collect partitions from this disk and fill the menu. Note that it should be a loop
	vector<MenuItem> menu;
	string ppath;
	vector<PedPartition *> partitions;
	PedPartition *tmpPart;
	string fstype;
	ncInterface.setSubtitle(_("Main menu: ") + string(device->path));
	int part=0;
	while (part!=-1) {
		menu.clear();
		partitions.clear();
		tmpPart = ped_disk_next_partition(disk, NULL);
		while (tmpPart) {
			ppath.clear();
			if (!(tmpPart->type&PED_PARTITION_FREESPACE) && !(tmpPart->type&PED_PARTITION_METADATA)) { 
				ppath = ped_partition_get_path(tmpPart);
			}
			if (tmpPart->fs_type) {
				fstype=tmpPart->fs_type->name;
			}
			else {
				if (tmpPart->type&PED_PARTITION_FREESPACE) fstype="[Free space]";
				else if (tmpPart->type&PED_PARTITION_METADATA) fstype="[Metadata]";
				else if (tmpPart->type&PED_PARTITION_EXTENDED) fstype="[Extended]";
				else if (tmpPart->type&PED_PARTITION_LOGICAL) fstype="[Logical]";
				else if (tmpPart->type&PED_PARTITION_PROTECTED) fstype="[Protected]";
				else fstype="[WTF: " + string(ped_partition_type_get_name(tmpPart->type)) + ", ID: " + IntToStr(tmpPart->type) + "]";
			}
			// Let's filter out metadata "partitions":
			if ((tmpPart->type&PED_PARTITION_METADATA)==0) {
				// Add partition to vector and menu
				partitions.push_back(tmpPart);
				menu.push_back(MenuItem(ppath, fstype + ", " + humanizeSize(tmpPart->geom.start*disk->dev->sector_size) + " - " + humanizeSize(tmpPart->geom.end*disk->dev->sector_size) + _(", size: ") + humanizeSize(tmpPart->geom.length*disk->dev->sector_size)));
			}
			// Finally, check for next partition
			tmpPart = ped_disk_next_partition(disk, tmpPart);
		}
	
		// Add items for exit this menu
		menu.push_back(MenuItem("", ""));
		menu.push_back(MenuItem("APPLY", _("Write changes to drive")));
		menu.push_back(MenuItem("CANCEL", _("Abandon changes and return to main menu")));
		part = ncInterface.showMenu(_("Select partition or free space to perform some action:"), menu, part);
		if (part==(int) menu.size()-3) continue;
		if (part==(int) menu.size()-2) {
			if (applyActions(disk)==0) {
				ncInterface.showMsgBox(_("Changes applied successfully"));
			}
			return;
		}
		if (part==(int) menu.size()-1) {
			return;
		}
		actionsMenu(partitions[part]);
	}
	return;
}
int MParted::showMainMenu() {
	// NCurses initialization
	dialogMode = true;
	ncInterface.setTitle(_("AgiliaLinux partition manager") + string(" 0.1"));
	ncInterface.setSubtitle(_("Initialization"));

	// Probing all devices
	ncInterface.showInfoBox(_("Probing all drives, please wait..."));
	//ped_device_probe_all();
	ped_device_probe_all_workaround();

	// The only one is static: physical devices. Let's enumerate it now and collect main (device selection) menu.
	vector<MenuItem> menu;
	ncInterface.setSubtitle(_("Drive selection"));
	vector<PedDevice *> devices;
	PedDevice *tmpDev = ped_device_get_next(NULL);	
	while (tmpDev) {
		devices.push_back(tmpDev);
		menu.push_back(MenuItem(tmpDev->path, tmpDev->model));
		tmpDev = ped_device_get_next(tmpDev);
	}

	// Check if no devices found: notify user and exit
	if (devices.empty()) {
		ncInterface.showMsgBox(_("No drives found. Check your hardware"));
		ped_device_free_all();
		return 0;
	}

	// Adding items to exit program to menu vector
	menu.push_back(MenuItem("", ""));
	menu.push_back(MenuItem("QUIT", _("Exit partition table editor")));

	// Starting the main menu loop
	int dev=0;
	while (dev!=-1) {
		dev = ncInterface.showMenu(_("Select drive to work with:"), menu, dev);
		if (dev==(int) menu.size()-2) continue;
		if (dev==(int) menu.size()-1) return 0;
		showDevMenu(devices[dev]);
	}
	return 0;
}

