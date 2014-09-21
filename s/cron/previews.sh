#!/bin/sh

cd $ERESSEA/git
s/preview build
s/preview version
for game in 2 3 4 ; do
	s/preview -g $game run && \
	s/preview -g $game send
done
