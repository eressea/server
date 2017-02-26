#!/bin/bash

if [ -z $ERESSEA ]; then
  echo "You need to define the \$ERESSEA environment variable to run $0"
  exit -2
fi

GAME=$ERESSEA/game-$1
GAME_NAME=$(grep -w name $GAME/eressea.ini | sed 's/.*=\s*//')

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
$ERESSEA/server/bin/compress.py $TURN "$GAME_NAME"
cd -
