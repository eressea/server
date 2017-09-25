#!/bin/bash

file=$1
locale=$2
game=$3
game_dir=$4
server_dir=$5

shift 5

# you can pass more parameters with the game:checker.parameter option in your eressea.ini

# replace this with the correct value for your installation
echeck_dir=$HOME/echeck
echeck=${echeck_dir}/echeck

if [ "$game" == "2" ]; then
  rules="e2"
fi
if [ "$game" == "3" ]; then
  rules="e3"
fi
if [  "$game" == "4" ]; then
  rules="e3"
fi

# uncomment this line if you have echeck (tested with version 4.4.1) installed
#[ -f $echeck ] && $echeck -P$echeck_dir -R$rules -w0 -x -L$locale $file

