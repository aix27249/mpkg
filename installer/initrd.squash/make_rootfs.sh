mksquashfs initrd.new rootfs.sfs -e bin lib lib64 sbin usr
cd initrd.new
mksquashfs bin bin.sfs
mksquashfs sbin sbin.sfs
mksquashfs lib lib.sfs
mksquashfs lib64 lib64.sfs
mksquashfs usr usr.sfs
cd -
