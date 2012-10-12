#!/bin/sh
LDDLIST="$1"
OUTLDDDIR="$2"
FILELIST="$3"
OUTFILEDIR="$4"
TMPDIR="$5"
echo "outfiledir = $OUTFILEDIR"
echo "outldddir = $OUTLDDDIR"
echo "TMPDIR=$TMPDIR"

mkdir -p $OUTFILEDIR
mkdir -p $OUTLDDDIR

let i=0
echo "Counting files..."
LDDCOUNT=$(cat $LDDLIST | wc -l)
FILECOUNT=$(cat $FILELIST | wc -l)
echo "Counting complete"
# Process ldd list
echo "ldd: processing $LDDCOUNT files"
while read index ; do
	ldd "$TMPDIR/$index" > $OUTLDDDIR/$i 2>/dev/null
	let i=$i+1
done < $LDDLIST

echo "ldd processing complete, processed $i files"

# Process file list
let i=0
echo "file: processing $FILECOUNT files"
while read index ; do
	file "$TMPDIR/$index" > $OUTFILEDIR/$i 2>/dev/null
	let i=$i+1
done < $FILELIST

echo "file processing complete, processed $i files"

