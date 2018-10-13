#!/bin/bash


if [ -z "$ERESSEA" ]; then
  echo "You have to define the \$ERESSEA environment variable to run $0"
  exit -2
fi

function abort() {
  if [ $# -gt 0 ]; then
    echo "$@"
  fi
  exit -1
}

GAME=$1
EMAIL=$2
FACTION=$3
PASSWD=$4
#echo "$GAME $EMAIL $FACTION $PASSWD" >> /tmp/report.log

function reply() {
  echo "$@" | mutt -s "Reportnachforderung Partei ${FACTION}" "$EMAIL"
  abort "$@"
}

LOCKFILE="$ERESSEA/.report.lock"
[ -e "$LOCKFILE" ] && reply "lockfile exists. wait for mail delivery to finish."

echo "$(date):report:$GAME:$EMAIL:$FACTION:$PASSWD" >> "$ERESSEA/request.log"

cd "$ERESSEA" || exit
PWFILE="game-$GAME/eressea.db"
if [ ! -e "$PWFILE" ]; then
  PWFILE="game-$GAME/passwd"
fi
checkpasswd.py "$PWFILE" "$FACTION" "$PASSWD" || reply "Das Passwort fuer die Partei $FACTION ist ungueltig"

cd "$ERESSEA/game-$GAME/reports" || exit
if [ ! -e "${FACTION}.sh" ]; then
  echo "Der Report für Partei $FACTION kann wegen technischer Probleme leider nicht nachgefordert werden: No such file ${FACTION}.sh" \
  | mutt -s "Reportnachforderung Partei ${FACTION}" "$EMAIL"
  exit
fi

bash "${FACTION}.sh" "$EMAIL" || reply "Unbekannte Partei $FACTION"

OWNER=$(getfaction.py "$PWFILE" "$FACTION")
if [ ! -z "$OWNER" ]; then
  echo "Der Report Deiner Partei wurde an ${EMAIL} gesandt." \
  | mutt -s "Reportnachforderung Partei ${FACTION}" "$OWNER"
fi
