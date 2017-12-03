#!/bin/bash

##
## Prepare the report

if [ -z "$ERESSEA" ]; then
  echo "You have to define the \$ERESSEA environment variable to run $0"
  exit -2
fi

if [ ! -z "$1" ]; then
  GAME="$ERESSEA/game-$1"
else
  GAME=$ERESSEA
fi

cd "$GAME/reports" || exit
for REPORT in *.sh
do
  echo -n "Sending "
  basename "$REPORT" .sh
  bash "$REPORT"
done
cd - || exit

