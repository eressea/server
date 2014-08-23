#!/bin/bash
[ -z $ERESSEA ] && ERESSEA=$HOME/eressea
SRC=$ERESSEA/git
$SRC/s/preview build master
$SRC/s/preview version
for game in 2 3 4 ; do
	$SRC/s/preview -g $game run && \
	$SRC/s/preview -g $game send
done
