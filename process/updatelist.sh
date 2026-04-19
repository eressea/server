#!/bin/sh

TURN=$1
LIST=eressea-announce@kn-bremen.de
awk '{ if ($2 =="'$TURN'") print $5 }' deadlog.txt |\
    mailman delmembers -f- -l$LIST
tail reports/reports.txt | cut -d: -f2 | sed -e 's/email=//' | \
    mailman addmembers - $LIST

