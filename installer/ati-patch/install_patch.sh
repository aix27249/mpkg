#!/bin/bash

PATCHPATH=/usr/src/drivers/ati-patch
KERNV=`uname -r`
MODPATH=/lib/modules
KERNPATH=/usr/src/linux

echo -e "Installing patch for ATI fglrx driver..."

cd $MODPATH/fglrx
cp $KERNPATH/drivers/acpi/acpica/{acconfig.h,aclocal.h,acobject.h} build_mod/

patch -Np0 < $PATCHPATH/2.6.29.x_fglrx-9.3.patch

cd $MODPATH/fglrx/build_mod
sh make.sh
cd ..
sh make_install.sh
