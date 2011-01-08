#!/bin/bash
ISO_FILE="${ISO_FILE:-~/AgiliaLinux-8.0_beta1-x86_64.iso}"
ISO_ROOT="${ISO_ROOT:-.}"
mkisofs -o "${ISO_FILE}" \
-b isolinux/isolinux.bin -c isolinux/boot.cat \
-no-emul-boot \
-boot-load-size 4 \
-boot-info-table \
-hide-rr-moved \
-iso-level 3 \
-R -J -v -d -N  \
-publisher "AgiliaLinux / http://www.agilialinux.ru" \
-V "AGILIA" \
-A "AgiliaLinux LiveCD" "$ISO_ROOT"

isohybrid "${ISO_FILE}"
