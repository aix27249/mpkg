#!/bin/sh
ARCH=x86
SUBLIST=OPENBOX
NODE=/nodes/$ARCH/$SUBLIST
INITRD_ROOT=/nodes/$ARCH/$SUBLIST/boot/initrd-tree
LIVE_ROOT=/nodes/live/$ARCH/$SUBLIST
ISO_OUTPUT=/nodes/iso/
ISO_FILENAME=AgiliaLinux-8.0-LiveOPENBOX-$ARCH-snapshot.iso

do_minimize=1 # Removes documentation and includes from tree


# Custom actions to perform with liveCD.
custom_actions() {
	# Add installer run to openbox
	cat $CWD/autostart.sh > $NODE/root/.config/openbox/autostart.sh
	cat $CWD/obmenugen.schema > $NODE/root/.config/obmenugen/obmenugen.schema

	# Copy gdm config for autologin
	cat $CWD/custom.conf > $NODE/etc/gdm/custom.conf
	cat $CWD/.dmrc > $NODE/root/.dmrc

}

# -------------------------------------- YOU SHOULD NOT EDIT ANYTHING BELOW THIS LINE UNLESS YOU REALLY NEED IT -----------------------------------------------------
# --------------------------------- ВЫ НЕ ДОЛЖНЫ НИЧЕГО МЕНЯТЬ НИЖЕ ЭТОЙ СТРОКИ ЕСЛИ В ЭТОМ НЕТ КРАЙНЕЙ НЕОБХОДИМОСТИ -----------------------------------------------


. ./create-livecd.sh
