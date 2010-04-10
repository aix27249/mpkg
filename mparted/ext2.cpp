#include "ext2.h"
bool Ext2::resize(PedPartition *partition, PedSector size, string *results) {
	if (!can_resize) return false;
	string output = get_tmp_file();
	return (system("resize2fs " + string(ped_partition_get_path(partition)) + IntToStr((long long) (size*partition->disk->dev->sector_size)/1024) + "K")==0);

}
bool Ext2::create(PedPartition *partition, string *results) {
	if (!can_create) return false;
}
bool Ext2::check(PedPartition *partition, string *results) {
	if (!can_check) return false;
}
bool Ext2::diskusage(PedPartition *partition, PedSector *used) {
	if (!can_diskusage) return false;
}
bool Ext2::get_label(PedPartition *partition, string *label) {
	if (!can_getlabel) return false;
}
bool Ext2::set_label(PedPartition *partition, const string& label) {
	if (!can_setlabel) return false;
}
bool Ext2::check_support() {
	can_create = !find_program("mkfs.ext2").empty();
	can_resize = !find_program("resize2fs").empty();
	can_check = !find_program("e2fsck").empty();
	can_diskusage = !find_program("dumpe2fs").empty();
	can_getlabel = !find_program("e2label").empty();
	can_setlabel = can_getlabel;
}
