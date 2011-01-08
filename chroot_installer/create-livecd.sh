#!/bin/bash
# 
# NOTE: You *HAVE* to define variables first!!! See examples in openbox-x86-live.sh script.
#
#


# Cleanup

rm -rf $NODE
rm -rf $INITRD_ROOT
rm -rf $LIVE_ROOT

# Let's go :)

if [ "$ARCH" = "x86" ] ; then
	BITS=32
else
	BITS=64
fi

if [ "$ARCH" = "x86" ] ; then
	REPO=${REPO:-http://core32.agilialinux.ru/}
else
	REPO=${REPO:-http://core64.agilialinux.ru/}
fi
LIST=${SUBLIST}.list

# Installation
ARCH=$ARCH LIST=$LIST NODE=$NODE REPO=$REPO ./install_virtual_machine.sh
NODE=$NODE ./add_default_services.sh

# Rip out all documentation, includes and static libs
CWD=`pwd`/live-elements
if [ "$do_minimize" != "" ] ; then
	rm -rf $NODE/usr/doc
	rm -rf $NODE/usr/include
	rm -rf $NODE/usr/share/gtk-doc
	rm -rf $NODE/usr/share/doc
fi

# Cache has to be removed anyway.
rm -rf $NODE/var/mpkg/cache

# Copy root stuff. Login as agilia with no password.
cat $CWD/shadow > $NODE/etc/shadow
cat $CWD/passwd > $NODE/etc/passwd
cat $CWD/fstab > $NODE/etc/fstab

# Copy X11 keymap
mkdir -p ${NODE}/etc/X11/xorg.conf.d
cat $CWD/10-keymap.conf > ${NODE}/etc/X11/xorg.conf.d/10-keymap.conf

# Copy skel to root dir
rsync -arvh $NODE/etc/skel/ $NODE/root/

# Custom actions. May vary for different live systems
custom_actions

OUTPUT=$LIVE_ROOT/fs32
mkdir -p $OUTPUT

# Creating sfs files
ARCH=$ARCH OUTPUT=$OUTPUT NODE=$NODE COMPRESSOR=gzip BLOCK_SIZE=65536 $CWD/make_rootfs.sh

# Now, initrd
mkdir -p $INITRD_ROOT
( cd $INITRD_ROOT && zcat $CWD/initrd$BITS.img | cpio -div )
cat $CWD/init > $INITRD_ROOT/init

# Copy kernel modules
cd $NODE/lib/modules
KERNEL_VER=`ls`
MOD_PATH=lib/modules/$KERNEL_VER
cd -
rm -rf $INITRD_ROOT/lib/modules
mkdir -p $INITRD_ROOT/$MOD_PATH/
cp $NODE/$MOD_PATH/kernel/fs/squashfs/squashfs.ko $INITRD_ROOT/$MOD_PATH/
cp $NODE/$MOD_PATH/kernel/fs/aufs/aufs.ko $INITRD_ROOT/$MOD_PATH/
rm $INITRD_ROOT/load_kernel_modules

# Copy kernel image
mkdir -p $LIVE_ROOT/boot/
cp $NODE/boot/vmlinuz-$KERNEL_VER $LIVE_ROOT/boot/vmlinuz$BITS

# Generating initrd image
mkinitrd -s $INITRD_ROOT -o $LIVE_ROOT/boot/initrd$BITS.img

# Copying isolinux configs
mkdir -p $LIVE_ROOT/isolinux
cp -a $CWD/syslinux$BITS/* $LIVE_ROOT/isolinux/

# Creating ISO
mkdir -p $ISO_OUTPUT
ISO_FILE=$ISO_OUTPUT/$ISO_FILENAME ISO_ROOT=$LIVE_ROOT $CWD/makeiso.sh

