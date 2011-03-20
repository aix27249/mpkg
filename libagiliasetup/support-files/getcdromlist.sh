#!/bin/sh
PART=3
while [ 0 ]; do
	LINE=`cat /proc/sys/dev/cdrom/info | grep "drive name:"`
	DRIVE=`echo $LINE | cut -f $PART -d ' '`
	if [ "$DRIVE" = "" ]; then
		break
	fi
	PART=`expr $PART + 1`
	echo $DRIVE >> $1
done

