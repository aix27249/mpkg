rm rootfs.sfs
rm initrd.new/*.sfs
mksquashfs /mnt/hd/initrd.openbox rootfs.sfs -e bin lib lib64 sbin usr
cd initrd.new
mksquashfs usr/local local.sfs
cd -
chmod 644 *.sfs
chmod 644 initrd.new/*.sfs
