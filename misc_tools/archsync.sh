#!/bin/sh
BRANCH=$1
echo "Syncing branch $BRANCH"

SYNCLIST=`wget 'http://packages.agilialinux.ru/archsync.php?b=core&human_readable=false&compact&group&to_arch=x86_64&tag='$BRANCH -O -`
for i in $SYNCLIST ; do
	echo $i
	mkpkg -i -bt $i

done

