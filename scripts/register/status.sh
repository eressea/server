#!/bin/sh

if [ $# -gt 0 ]; then
  db="$1"
else
  db="vinyambar"
fi
echo "Vinyambar I"
echo
mysql --table $db -e "source subscriptions-1.sql"
echo
echo

echo "Vinyambar II"
echo
mysql --table $db -e "source subscriptions-2.sql"
echo
echo

echo "Rassenverteilung"
echo
mysql --table $db -e "source races.sql"
echo
echo

echo "Ausgemusterte Parteien"
echo
mysql --table $db -e "source noowner.sql"
echo
echo

echo "Überweisung erforderlich"
echo
mysql --table $db -e "source unpaid.sql"
echo
echo

echo "Parteienverteilung"
echo
mysql --table $db -e "source summary.sql"
echo
echo
