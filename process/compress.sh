#!/bin/bash

if [ -z $ERESSEA ]; then
  echo "You need to define the \$ERESSEA environment variable to run $0"
  exit -2
fi

BINDIR=$ERESSEA/server/bin

GAME=$ERESSEA/game-$1
GAME_NAME="$($BINDIR/inifile $GAME/eressea.ini get game:name)"

TURN=$2
if [ -z $TURN ]
then
  TURN=`cat $GAME/turn`
fi

if [ ! -d $GAME/reports ]; then
  echo "cannot find reports directory in $GAME"
  exit -1
fi

cd $GAME/reports
$BINDIR/compress.py $TURN "$GAME_NAME"
cd -
