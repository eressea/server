#!/bin/sh
NOTIFY="eressea-design@eressea.kn-bremen.de"
#NOTIFY="enno@eressea.upb.de"
NEWFILE="/tmp/commit.source.$$"
OLDFILE="/tmp/commit.source.last"

BUILDNO=0
BUILDFILE="/home/cvs/.build/eressea-source"
if [ -e $BUILDFILE ]; then
  BUILDNO=`cat $BUILDFILE`
fi

perl -e '$i=0; while (<>) { if (/^Log Message.*$/) { $i=1; } else { if ($i==1) { print $_; }  else { if (/^.*(Tag.*)$/) { print "$1\n\n"; } } } }' >| $NEWFILE
WHO="$1"
shift

NEWMD5=`md5sum $NEWFILE | awk '{ print $1 }'`
echo "New md5sum=$NEWMD5"
if [ -e $OLDFILE ]; then
  OLDMD5=`md5sum $OLDFILE | awk '{ print $1 }'`
else
  OLDMD5="N/A"
fi
cp $NEWFILE $OLDFILE
echo "Old md5sum=$OLDMD5"
if [ $NEWMD5 != $OLDMD5 ]; then
  let BUILDNO=$BUILDNO+1
  echo $BUILDNO >| $BUILDFILE
  mailx -s "[commit $BUILDNO] eressea-source by $WHO" $NOTIFY < $NEWFILE
  echo "New log message. Sent out notification"
else
  echo "Identical log message. Notification skipped"
fi
echo $BUILDNO $@ | mailx -s "build eressea" cvs@eressea.upb.de
rm $NEWFILE
