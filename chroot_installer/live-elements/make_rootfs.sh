mksquashfs $NODE $OUTPUT/rootfs.sfs -b $BLOCK_SIZE -comp $COMPRESSOR -e bin lib lib64 sbin usr boot lost+found
cd $NODE
mksquashfs bin $OUTPUT/bin.sfs -b $BLOCK_SIZE -comp $COMPRESSOR
mksquashfs sbin $OUTPUT/sbin.sfs -b $BLOCK_SIZE -comp $COMPRESSOR
mksquashfs lib $OUTPUT/lib.sfs -b $BLOCK_SIZE -comp $COMPRESSOR
if [ "$ARCH" = "x86_64" ] ; then
	mksquashfs lib64 $OUTPUT/lib64.sfs -b $BLOCK_SIZE -comp $COMPRESSOR
fi
mksquashfs usr $OUTPUT/usr.sfs -e local -b $BLOCK_SIZE -comp $COMPRESSOR
mksquashfs usr/local $OUTPUT/local.sfs -b $BLOCK_SIZE -comp $COMPRESSOR
cd -
