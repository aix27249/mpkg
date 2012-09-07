#!/bin/sh
BOOT_SERVICE_LIST="mdadm lvm videoswitch langswitch"
SYS_SERVICE_LIST="sysfs udev"
DEF_SERVICE_LIST="consolefont \
	udev-postmount \
	dbus \
	sysklogd \
	alsasound \
	cupsd \
	gdm \
	cron \
	networkmanager"
X11_SERVICE_LIST="xdm \
	kdm \
	gdm \
	lxdm \
	slim \
	xdm"
	

for i in $BOOT_SERVICE_LIST ; do
	if [ -f "$NODE/etc/init.d/$i" ] ; then
		( cd ${NODE}/etc/runlevels/boot ; ln -s /etc/init.d/$i $i )
	fi
done


for i in $SYS_SERVICE_LIST ; do
	if [ -f "$NODE/etc/init.d/$i" ] ; then
		( cd ${NODE}/etc/runlevels/sysinit ; ln -s /etc/init.d/$i $i )
	fi
done

for i in $DEF_SERVICE_LIST ; do
	if [ -f "$NODE/etc/init.d/$i" ] ; then
		( cd ${NODE}/etc/runlevels/default ; ln -s /etc/init.d/$i $i )
	fi
done

for i in $X11_SERVICE_LIST ; do
	if [ -f "$NODE/etc/init.d/$i" ] ; then
		( cd ${NODE}/etc/runlevels/X11 ; ln -s /etc/init.d/$i $i )
		break
	fi
done

chroot $NODE rc-update -u

