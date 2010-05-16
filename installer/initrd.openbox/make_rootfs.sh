mksquashfs /mnt/hd/initrd.openbox rootfs.sfs -e bin lib lib64 sbin usr
if [ "$1" == "root" ] ; then
	exit 0
fi

ARCH=`uname -m`

cd initrd.new
mksquashfs bin bin.sfs
mksquashfs sbin sbin.sfs
mksquashfs lib lib.sfs
if [ "$ARCH" = "x86_64" ] ; then
	mksquashfs lib64 lib64.sfs
fi
mksquashfs usr usr.sfs -e local
mksquashfs usr/local local.sfs
cd -
