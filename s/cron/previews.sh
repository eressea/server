#!/bin/bash

s/preview build master
s/preview version
for game in 2 3 4 ; do
	s/preview -g $game run && \
	s/preview -g $game send
done
