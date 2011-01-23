#!/bin/bash
# This script should be run as root or fakeroot.
set -e
export PATH=${scriptdir}/bin:$PATH
# Set alias for chroot to fakechroot. Let's see if it can work such way

# Loading global config
if [ -r "/etc/mklivecd.conf" ] ; then
	. /etc/mklivecd.conf
fi

# Loading local user config
if [ -r "${REAL_HOME}/.mklivecd.conf" ] ; then
	. ${REAL_HOME}/.mklivecd.conf
fi

scriptdir=${scriptdir:-/usr/share/mklivecd/scripts} # This should be defined in global config at package build time in case if you use another paths

# Helper environments for ISOBUILD
filedir=${startdir}/files
plugindir=${scriptdir}/plugins

# Loading ISOBUILD
. ${startdir}/ISOBUILD

# Defining variables
ARCH=$arch
NODE="${BUILD_ROOT}/${iso_name}-${ARCH}"
INITRD_ROOT="${NODE}/boot/initrd-tree"
LIVE_ROOT="${LIVE_BUILD_ROOT}/${iso_name}-${ARCH}"
ISO_FILENAME=${ISO_FILENAME:-${iso_name}-${ARCH}.iso}

# Cleanup
if [ "$skip_stage1" = "" ] ; then
	rm -rf "$NODE"
fi
if [ "$skip_stage2" = "" ] ; then
	rm -rf "$INITRD_ROOT"
	rm -rf "$LIVE_ROOT"
fi

# Let's go :)

if [ "$ARCH" = "x86" ] ; then
	BITS=32
	LIBDIRSUFFIX=
else
	BITS=64
	LIBDIRSUFFIX=64
fi
if [ "$REPO" = "" ] ; then
	if [ "$ARCH" = "x86" ] ; then
		REPO=${REPO:-http://core32.agilialinux.ru/}
	else
		REPO=${REPO:-http://core64.agilialinux.ru/}
	fi
fi

LIST="${startdir}/pkglist"

# Getting online list if needed
if [ "`echo $package_list | grep ^http:`" != "" -o "`echo $package_list | grep ^ftp:`" != "" ] ; then
	wget "$package_list" -O "${startdir}/pkglist"
fi
# List-tuning
# Add
if [ ! -z "$add_to_list" ] ; then
	for i in $add_to_list ; do
		echo $i >> ${LIST}
	done
fi
# Remove
if [ ! -z "$remove_from_list" ] ; then
	for i in $remove_from_list ; do
		sed -i "s/^$i$//g" $LIST
	done
fi

# Installation
if [ "$skip_stage1" = "" ] ; then
	ARCH=$ARCH LIST="$LIST" NODE="$NODE" REPO="$REPO" ${scriptdir}/install_virtual_machine.sh
	NODE="$NODE" ${scriptdir}/add_default_services.sh
fi

# Rip out all documentation, includes and static libs
CWD=${scriptdir}/live-elements

if [ "$remove_docs" = "1" -o "$do_minimize" = "1" ] ; then
	rm -rf $NODE/usr/doc
	rm -rf $NODE/usr/share/gtk-doc
	rm -rf $NODE/usr/share/doc
fi

if [ "$remove_devel" = "1" -o "$do_minimize" = "1" ] ; then
	rm -rf $NODE/usr/include
	rm -rf $NODE/usr/lib/*.a
	rm -rf $NODE/usr/lib/*.la
	if [ -d $NODE/usr/lib64 ] ; then
		rm -rf $NODE/usr/lib64/*.a
		rm -rf $NODE/usr/lib64/*.la
	fi
	rm -rf $NODE/lib/*.a
	if [ -d $NODE/lib64 ] ; then
		rm -rf $NODE/lib64/*.la
	fi
fi

# Move packages if requested
if [ "$include_used_packages" = "1" ] ; then
	mv ${NODE}/var/mpkg/cache ${LIVE_ROOT}/repository
	mkdir -p ${LIVE_ROOT}/repository/setup_variants
	cat ${LIST} > ${LIVE_ROOT}/repository/setup_variants/LIVE.list
	echo "desc: Live system" > ${LIVE_ROOT}/repository/setup_variants/LIVE.desc
	echo "full: Install system like this LiveCD" >> ${LIVE_ROOT}/repository/setup_variants/LIVE.desc
	echo "/repository/" > ${LIVE_ROOT}/.repository
	echo "AGILIA_LIVE" > ${LIVE_ROOT}/.volume_id
	mpkg-index ${LIVE_ROOT}/repository
fi

# Cache has to be removed anyway.
rm -rf $NODE/var/mpkg/cache
mkdir -p ${NODE}/var/mpkg/cache

# Copy root stuff. Login as agilia with no password.
# cat $CWD/shadow > $NODE/etc/shadow
# cat $CWD/passwd > $NODE/etc/passwd
cat $CWD/fstab > $NODE/etc/fstab

# Copy X11 keymap
mkdir -p ${NODE}/etc/X11/xorg.conf.d
cat $CWD/10-keymap.conf > ${NODE}/etc/X11/xorg.conf.d/10-keymap.conf

# Remove xinitrc if any and recreate it:
if [ -d ${NODE}/etc/X11/xinit ] ; then
	rm -f ${NODE}/etc/X11/xinit/xinitrc
	( cd ${NODE}/etc/X11/xinit
		for i in xinitrc.* ; do
			ln -sf $i xinitrc
		done
	)
fi

# Copy skel to root dir
rsync -arvh $NODE/etc/skel/ $NODE/root/

# Set root password if defined by ISOBUILD
if [ "$root_password" = "" ] ; then
	if [ "$empty_root_password" = "" ] ; then
		root_password=root
	fi
fi

echo -ne "$root_password\n$root_password\n" | chroot $NODE passwd root

# Add standard user. If not specified, user will be agilia/agilia
if [ "$no_user" = "" ] ; then
	if [ "$user_name" = "" ] ; then
		user_name=agilia
	fi
	if [ "$user_password" = "" ] ; then
		if [ "$empty_user_password" = "" ] ; then
			user_password=agilia
		fi
	fi
	user_groups=${user_groups:-audio,cdrom,floppy,video,netdev,plugdev,power}
	chroot $NODE /usr/sbin/useradd -d /home/$user_name -m -g users -G $user_groups -s /bin/bash $user_name
	echo -ne "$user_password\n$user_password\n" | chroot $NODE passwd $user_name
fi




# Custom actions. May vary for different live systems
custom_actions

OUTPUT=$LIVE_ROOT/fs${BITS}
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
cp $NODE/$MOD_PATH/kernel/drivers/virtio/virtio.ko $INITRD_ROOT/$MOD_PATH/
cp $NODE/$MOD_PATH/kernel/drivers/block/virtio_blk.ko $INITRD_ROOT/$MOD_PATH/
rm $INITRD_ROOT/load_kernel_modules

# Generate load_kernel_modules script
for i in "squashfs aufs virtio virtio_blk" ; do
	echo "insmod /lib/modules/$KERNEL_VER/$i" >> $INITRD_ROOT/load_kernel_modules
done

# Copy kernel image
mkdir -p $LIVE_ROOT/boot/
cp $NODE/boot/vmlinuz-$KERNEL_VER $LIVE_ROOT/boot/vmlinuz$BITS

# Generating initrd image
mkinitrd -s $INITRD_ROOT -o $LIVE_ROOT/boot/initrd$BITS.img -k $KERNEL_VER

# Copying isolinux configs
mkdir -p $LIVE_ROOT/isolinux
cat $CWD/isolinux.cfg | sed s/@ARCH@/$BITS/g | sed "s/@ISO_TITLE@/${iso_title}/g" > $LIVE_ROOT/isolinux/isolinux.cfg

# ISOLINUX binaries
for i in linux.c32 vesamenu.c32 vesainfo.c32 isolinux.bin ; do
	cp /usr/lib${LIBDIRSUFFIX}/syslinux/$i $LIVE_ROOT/isolinux/
done

# Creating ISO
mkdir -p $ISO_OUTPUT
rm -f $ISO_OUTPUT/$ISO_FILENAME
ISO_FILE=$ISO_OUTPUT/$ISO_FILENAME ISO_ROOT=$LIVE_ROOT $CWD/makeiso.sh
set +e

