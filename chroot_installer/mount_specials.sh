#!/bin/sh
for i in /var/agilia/nodes/x86/* /var/agilia/nodes/x86_64/* ; do
	mount -o bind /proc $i/proc
	mount -o bind /sys $i/sys
	mount -o bind /dev $i/dev
	mount -o bind /dev/pts $i/dev/pts
done
