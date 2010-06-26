rm rootfs.sfs
mksquashfs /mnt/hd/initrd.openbox rootfs.sfs -e bin lib lib64 sbin usr
ARCH=`uname -m`

cd initrd.new
rm bin.sfs sbin.sfs lib.sfs usr.sfs local.sfs
mksquashfs bin bin.sfs
mksquashfs sbin sbin.sfs
mksquashfs lib lib.sfs
if [ "$ARCH" = "x86_64" ] ; then
	rm lib64.sfs
	mksquashfs lib64 lib64.sfs
fi
mksquashfs usr usr.sfs -e local
mksquashfs usr/local local.sfs
cd -
