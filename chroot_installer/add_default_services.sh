#!/bin/sh
SYS_SERVICE_LIST="sysfs udev"
DEF_SERVICE_LIST="consolefont \
	hald \
	dbus \
	sshd \
	alsasound \
	acpid \
	cupsd \
	cron \
	gdm \
	kdm \
	networkmanager"

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

