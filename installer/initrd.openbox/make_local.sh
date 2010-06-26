rm rootfs.sfs
mksquashfs /mnt/hd/initrd.openbox rootfs.sfs -e bin lib lib64 sbin usr
cd initrd.new
rm local.sfs
mksquashfs usr/local local.sfs
cd -
