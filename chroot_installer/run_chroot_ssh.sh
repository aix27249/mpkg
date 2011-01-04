#!/bin/sh
for i in /var/agilia/nodes/x86/* /var/agilia/nodes/x86_64/* ; do
	chroot $i /usr/sbin/sshd
done
