#!/bin/bash

# Configuration variables. I think it's names and content shows everything.
NODE=${NODE:-/tmp/new_chroot}
REPO=${REPO:-http://packages.agilialinux.ru/}
LIST=minimal.list

echo Node will be installed in $NODE
echo Repository: $REPO

# Creating directory structure
CWD=$(pwd)
rm -rf $NODE
mkdir -p ${NODE}/{etc,tmp}
mkdir -p ${NODE}/var/mpkg/{packages,scripts,configs,backup}
mkdir -p ${NODE}/root

# Copying 
cp $CWD/mpkg.xml.system ${NODE}/etc/mpkg.xml.system
cat $CWD/mpkg.xml | sed -e s#NODE#$NODE#g | sed -e s#REPOSITORY#$REPO#g> ${NODE}/etc/mpkg.xml
cp packages.db ${NODE}/var/mpkg/

# Installing
mpkg-update --conf=${NODE}/etc/mpkg.xml --sysroot=${NODE}
mpkg-installfromlist --conf=${NODE}/etc/mpkg.xml --sysroot=${NODE}  $CWD/${LIST}

# Moving config
mv ${NODE}/etc/mpkg.xml.system ${NODE}/etc/mpkg.xml
echo "Installation complete"
