#!/bin/bash

## this script takes a backup of a turn.
## usage: backup.sh <turn>

if [ -z $ERESSEA ]; then
  echo "You have to define the \$ERESSEA environment variable to run $0"
  exit -2
fi
source $HOME/bin/functions.sh
source $HOME/etc/eressea.conf

GAME=$1
EMAIL=$2
FACTION=$3
PASSWD=$4
echo "$GAME $EMAIL $FACTION $PASSWD" >> /tmp/report.log

function reply() {
  echo $@ | mutt -s "Reportnachforderung Partei ${FACTION}" $EMAIL
  abort $@
}

LOCKFILE=$ERESSEA/.report.lock
[ -e $LOCKFILE ] && reply "lockfile exists. wait for mail delivery to finish."

REPLYTO='accounts@eressea.de'

echo `date`:report:$GAME:$EMAIL:$FACTION:$PASSWD >> $ERESSEA/request.log

cd $ERESSEA
checkpasswd.py game-$GAME/passwd $FACTION $PASSWD || reply "Das Passwort fuer die Partei $FACTION ist ungueltig"

cd $ERESSEA/game-$GAME/reports
if [ ! -e ${FACTION}.sh ]; then
  echo "Der Report für Partei $FACTION kann wegen technischer Probleme leider nicht nachgefordert werden: No such file ${FACTION}.sh" \
  | mutt -s "Reportnachforderung Partei ${FACTION}" $EMAIL
  exit
fi

source ${FACTION}.sh $EMAIL || reply "Unbekannte Partei $FACTION"

if [ -e $ERESSEA/game-$GAME/eressea.db ]; then
  SQL="select email from faction f left join faction_data fd on fd.faction_id=f.id where f.game_id=$GAME AND fd.code='$FACTION' and fd.turn=(select max(turn) from faction_data fx where fx.faction_id=f.id)"
  OWNER=$(sqlite3 $ERESSEA/game-$GAME/eressea.db "$SQL")
  echo "Der Report Deiner Partei wurde an ${EMAIL} gesandt." \
  | mutt -s "Reportnachforderung Partei ${FACTION}" $OWNER
fi
