#!/bin/bash
set -e
# Configuration variables. I think it's names and content shows everything.
NODE=${NODE:-/nodes/x86/gnome}
REPO=${REPO:-file:///core32-mirror/repository/}
ARCH=${ARCH:-x86}
LIST=${LIST:-GNOME.list}

echo Node: $NODE
echo Repo: $REPO
echo List: $LIST
echo Arch: $ARCH
echo PATH: $PATH
echo Chroot: `which chroot`



# Creating directory structure
scriptdir=${scriptdir:-$(pwd)}
rm -rf $NODE
mkdir -p ${NODE}/{etc,tmp}
mkdir -p ${NODE}/var/mpkg/{packages,scripts,configs,backup}
mkdir -p ${NODE}/{root,home,proc,sys,dev}
mkdir -p ${NODE}/usr/local/{bin,lib,sbin,share,doc}

# Copying 
cp ${scriptdir}/mpkg.xml.system ${NODE}/etc/mpkg.xml.system
cat ${scriptdir}/mpkg.xml | sed -e s#NODE#$NODE#g | sed -e s#REPOSITORY#$REPO#g | sed -e s#ARCH#$ARCH#g > ${NODE}/etc/mpkg.xml
cp ${scriptdir}/packages.db ${NODE}/var/mpkg/

# Installing
mpkg-update --conf=${NODE}/etc/mpkg.xml --sysroot=${NODE}
mpkg-installfromlist -y -m --conf=${NODE}/etc/mpkg.xml --sysroot=${NODE}  ${LIST}

# Moving config
mv ${NODE}/etc/mpkg.xml.system ${NODE}/etc/mpkg.xml
cat /etc/resolv.conf > ${NODE}/etc/resolv.conf
echo "Installation complete"

set +e
