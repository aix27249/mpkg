#!/bin/sh
sync
PID=$1
while [ "`ps ax | grep -v grep | grep "^$PID" | head -n 1`" != "" ] ; do
	echo "Waiting setup to close..."
	sleep 1
done

echo "Setup complete, rebooting"
/sbin/reboot

