#!/bin/sh
[ -z $ERESSEA ] && ERESSEA=$HOME/eressea

branch=master
if [ ! -n "$1" ]; then
branch="$1"
fi
SRC=$ERESSEA/git
$SRC/s/preview build $branch
$SRC/s/preview version
for game in 2 3 4 ; do
	$SRC/s/preview -g $game run && \
	$SRC/s/preview -g $game send
done
