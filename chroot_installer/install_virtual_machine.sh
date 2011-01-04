#!/bin/bash

# Configuration variables. I think it's names and content shows everything.
NODE=${NODE:-/nodes/x86/gnome}
REPO=${REPO:-file:///core32-mirror/repository/}
ARCH=${ARCH:-x86}
LIST=${LIST:-GNOME.list}

echo Node: $NODE
echo Repo: $REPO
echo List: $LIST
echo Arch: $ARCH

sleep 5


# Creating directory structure
CWD=$(pwd)
rm -rf $NODE
mkdir -p ${NODE}/{etc,tmp}
mkdir -p ${NODE}/var/mpkg/{packages,scripts,configs,backup}
mkdir -p ${NODE}/{root,home,proc,sys,dev}

# Copying 
cp $CWD/mpkg.xml.system ${NODE}/etc/mpkg.xml.system
cat $CWD/mpkg.xml | sed -e s#NODE#$NODE#g | sed -e s#REPOSITORY#$REPO#g | sed -e s#ARCH#$ARCH#g > ${NODE}/etc/mpkg.xml
cp packages.db ${NODE}/var/mpkg/

# Installing
mpkg-update --conf=${NODE}/etc/mpkg.xml --sysroot=${NODE}
mpkg-installfromlist --conf=${NODE}/etc/mpkg.xml --sysroot=${NODE}  $CWD/${LIST}

# Moving config
mv ${NODE}/etc/mpkg.xml.system ${NODE}/etc/mpkg.xml
cat /etc/resolv.conf > ${NODE}/etc/resolv.conf
echo "Installation complete"
