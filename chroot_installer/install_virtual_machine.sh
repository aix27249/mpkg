#!/bin/bash
set -e
# Configuration variables. I think it's names and content shows everything.
echo Node: $NODE
echo Repositories:
i=0
while [ -n "${REPO[$i]}" ] ; do
	echo $i: ${REPO[$i]}
	i=`expr $i + 1`
done
echo List: $LIST
echo Arch: $ARCH
echo PATH: $PATH
echo Chroot: `which chroot`



# Creating directory structure
if [ -z "$scriptdir" ] ; then
	echo 'Warning: scriptdir not specified, using current path instead'
	scriptdir=${scriptdir:-$(pwd)}
fi
rm -rf $NODE
mkdir -p ${NODE}/{etc,tmp}
mkdir -p ${NODE}/var/mpkg/{packages,scripts,configs,backup}
mkdir -p ${NODE}/{root,home,proc,sys,dev,mnt,media}
mkdir -p ${NODE}/usr/local/{bin,lib,sbin,share,doc}
mkdir -p ${NODE}/etc/init.d
mkdir -p ${NODE}/etc/rc.d

# Set permissions on /tmp
chmod 1777 ${NODE}/tmp

# This is for KDE:
( cd ${NODE}/var ; ln -s ../tmp tmp )

# For malformed packages
( cd ${NODE}/etc/rc.d ; ln -s ../init.d init.d )

# Copying 
cp ${scriptdir}/mpkg.xml.system ${NODE}/etc/mpkg.xml.system
cat ${scriptdir}/mpkg.xml | sed -e s#NODE#$NODE#g | sed -e s#ARCH#$ARCH#g > ${NODE}/etc/mpkg.xml
cp ${scriptdir}/packages.db ${NODE}/var/mpkg/

# Repository specifications
for i in ${REPO} ; do
	mpkg-add_rep --conf=${NODE}/etc/mpkg.xml --sysroot=${NODE} $i
done

# Installing
mpkg-update --conf=${NODE}/etc/mpkg.xml --sysroot=${NODE} || exit 1
mpkg-installfromlist -v -y -m --conf=${NODE}/etc/mpkg.xml --sysroot=${NODE}  ${LIST} || exit 1

# Moving config
mv ${NODE}/etc/mpkg.xml.system ${NODE}/etc/mpkg.xml
cat /etc/resolv.conf > ${NODE}/etc/resolv.conf
echo "Installation complete"

