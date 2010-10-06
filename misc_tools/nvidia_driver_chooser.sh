#!/bin/sh

CARD_CLASS=nVidia


HAS_NVIDIA=`lspci | grep $CARD_CLASS | grep 'VGA compatible controller'`
if [ "$HAS_NVIDIA" != "" ] ; then
	echo "We have $CARD_CLASS card: $HAS_NVIDIA"
fi
