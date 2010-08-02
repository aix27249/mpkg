cd initrd.new
rm local.sfs
mksquashfs usr/local local.sfs
cd -
chmod 644 initrd.new/*.sfs
