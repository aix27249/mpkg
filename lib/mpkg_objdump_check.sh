#!/bin/sh
LDDLIST=`cat $1`
OUTLDDDIR=$2
FILELIST=`cat $3`
OUTFILEDIR=$4
TMPDIR=$5
echo "outfiledir = $OUTFILEDIR"
echo "outldddir = $OUTLDDDIR"
mkdir -p $OUTFILEDIR
mkdir -p $OUTLDDDIR
let i=0
echo "objdump: searching from $i"
for index in $LDDLIST; do
	LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:`dirname $TMPDIR/$index` objdump -p $TMPDIR/$index 2>/dev/null | grep NEEDED > $OUTLDDDIR/$i
	#if [ "`cat $OUTLDDDIR/$i`" != "" ] ; then
	#	cat $OUTLDDDIR/$i
	#fi
	let i=$i+1
done
echo "objdump: searched for $i files"
let i=0
echo "files searching from $i"
for index in $FILELIST; do
	#echo $i
	file $TMPDIR/$index > $OUTFILEDIR/$i 2>/dev/null
	let i=$i+1
done
echo "file searched for $i files"

