#!/bin/sh
# This script creates an update package for MOPSLinux
# You may need to (re)define next variables to fit your needs

# Output directory, should be absolute path
OUTPUT="/tmp/mopslinux-6.2.2-update1"
# Enumerate directories on CD
DIRS="packages addons"
# ISO mountpoint, it can be just directory contained DIRS with packages.xml.gz
ISO_MOUNT=$1
# Main repository URLs
REPOSITORIES="http://mopspackages.ru/branches/mopslinux-6.2/branches/kde4/ \
	http://mopspackages.ru/branches/mopslinux-6.2/contrib/kde4/ \
	http://mopspackages.ru/branches/mopslinux-6.2/addons/ \
	ftp://ftp.rpunet.ru/mopslinux-6.2.2/packages/ \
	ftp://ftp.rpunet.ru/mopslinux-6.2.2/addons/"

# Let's go!
rm -rf $OUTPUT
mkdir -p $OUTPUT

# Creating common index
echo '<?xml version="1.0" encoding="UTF-8"?><repository>' > $OUTPUT/packages.xml
for i in $DIRS ; do 
	zcat $ISO_MOUNT/$i/packages.xml.gz | sed -e 's/<?xml version=\"1.0\" encoding=\"UTF-8\"?>//g' | sed -e "s/<repository>//g" | sed -e "s/<\/repository>//g" >> $OUTPUT/packages.xml
done
echo '</repository>' >> $OUTPUT/packages.xml
gzip $OUTPUT/packages.xml

# Creating syncmap
for i in $REPOSITORIES ; do
	echo $i $OUTPUT/ >> $OUTPUT/syncmap
done
echo "NO_REBUILD_AFTER_SYNC" >> $OUTPUT/syncmap
# Calling sync
mpkg -vyK sync $OUTPUT/syncmap

# Create new index
mpkg index $OUTPUT



