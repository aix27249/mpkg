#!/bin/sh
SETUP_PID=$1
sync

while ! ps -p $1 > /dev/null ; do
	echo "Waiting installer to finish... (PID: $1)"
	sleep 1
done

echo "Installation finished, rebooting right now"
/sbin/reboot

