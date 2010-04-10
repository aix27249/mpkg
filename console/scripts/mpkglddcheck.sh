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
echo "ldd searching from $i"
for index in $LDDLIST; do
	#echo $i
	ldd $TMPDIR/$index > $OUTLDDDIR/$i 2>/dev/null
	let i=$i+1
done
echo "ldd searched for $i files"
let i=0
echo "files searching from $i"
for index in $FILELIST; do
	#echo $i
	file $TMPDIR/$index > $OUTFILEDIR/$i 2>/dev/null
	let i=$i+1
done
echo "file searched for $i files"

