#!/bin/sh
# Script that asks user what nvidia driver he wants to use.
# It will be used in LiveDVD environment in fallback stuff

# Prerequirements: 
#   nvidia-*.txz in /var/mpkg/cache
#   working mpkg environment

# Script should be run BEFORE any X11 environments.


# Define a card class we want to detect
CARD_CLASS=nVidia

# I use this for testing on my macbook, so do not mention it
#CARD_CLASS=Intel


HAS_NVIDIA=`lspci | grep $CARD_CLASS | grep 'VGA compatible controller'`
if [ "$HAS_NVIDIA" != "" ] ; then
	dialog --menu "You have NVIDIA card: $HAS_NVIDIA. Please choose driver for it:" 25 80 70 256 "nvidia-driver-256.52" 173 "nvidia-driver-legacy-173" 96 "nvidia-driver-legacy96" "nv" "Generic nv" "nouveau" "New KMS-driven nouveau. Probably you don't want it." 2> /tmp/nvcheck
	CHOOSEN_CARD=`cat /tmp/nvcheck`
	rm /tmp/nvcheck

	case $CHOOSEN_CARD in 
		256)
			echo "Blob v.256 selected"
			rmmod nouveau
			mpkg-install -y /var/mpkg/cache/nvidia-driver-256*.txz /var/mpkg/cache/nvidia-kernel-256*.txz
			;;
		173)
			echo "Legacy blob v.173 selected"
			rmmod nouveau
			mpkg-install -y /var/mpkg/cache/nvidia-driver-legacy173*.txz /var/mpkg/cache/nvidia-kernel-legacy173*.txz
			;;
		96)
			echo "Legacy old blob v.96 selected"
			rmmod nouveau
			mpkg-install -y /var/mpkg/cache/nvidia-driver-legacy96*.txz /var/mpkg/cache/nvidia-kernel-legacy96*.txz
			;;
		nv)
			echo "Simple nv driver selected"
			rmmod nouveau
			cp /etc/X11/10-nv.conf /etc/X11/xorg.conf.d/
			;;
		nouveau)
			echo "Nouveau selected"
			;;
		*)
			echo "Nouveau selected"
			;;
	esac
fi

