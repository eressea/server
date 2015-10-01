#!/bin/bash

##
## Prepare the report

if [ -z $ERESSEA ]; then
  echo "You have to define the \$ERESSEA environment variable to run $0"
  exit -2
fi
source $HOME/bin/functions.sh
source $ERESSEA/etc/eressea.conf

if [ ! -z $1 ]; then
  GAME=$ERESSEA/game-$1
else
  GAME=$ERESSEA
fi

cd $GAME/reports || abort "could not chdir to reports directory"
for REPORT in *.sh
do
  echo -n "Sending "
  basename $REPORT .sh
  bash $REPORT
done
cd -

if [ -e $GAME/ages.sh ]; then
  cd $GAME
  ./ages.sh
  cd -
fi

