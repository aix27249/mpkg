#!/bin/sh
sync

while [ "`ps ax | grep -v grep | grep guisetup_exec | head -n 1`" != "" ] ; do
	echo "Waiting setup to close..."
	sleep 1
done

echo "Setup complete, rebooting"
/sbin/reboot

