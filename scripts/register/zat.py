#!/usr/bin/env python
import MySQLdb
import sys
import os

dbname=sys.argv[1]
game=int(sys.argv[2])
date=sys.argv[3]
price=1.25
db=MySQLdb.connect(db=dbname)
patchdir='/home/eressea/vinyambar-rsync/vin'+str(game)+'/data'

cursor=db.cursor()

k = cursor.execute('select name, patch from games where id='+str(game))
if k==0:
    print "Unbekanntes Spiel"

name, patch = cursor.fetchone()
print "Auswertung für " + name + " Patch Level " + str(int(patch))
while os.access(patchdir+'/patch-'+str(int(patch+1))+'.sql', os.F_OK)==0:
    patch=patch+1
    os.system('mysql --table ' + dbname + '-e ' + patchdir+'/patch-'+str(int(patch+1))+'.sql')
    cursor.execute('update games set patch='+str(int(patch))+' where game='+str(game))

k = cursor.execute("UPDATE subscriptions SET user=NULL where status='TRANSFERED' and updated<'"+date+"'"
print "Removing "+str(int(k))+" transfered subscriptions."

k = cursor.execute("UPDATE subscriptions SET user=NULL where status='CANCELLED' and updated<'"+date+"'"
print "Removing "+str(int(k))+" cancelled subscriptions."

k = cursor.execute("UPDATE subscriptions SET user=NULL where status='DEAD' and updated<'"+date+"'"
print "Removing "+str(int(k))+" dead subscriptions."

k = cursor.execute("SELECT max(lastturn) from subscriptions");
lastturn = int(cursor.fetchone()[0])
k = cursor.execute("UPDATE subscriptions SET status='CANCELLED' where lastturn+3<="+str(lastturn)
k = cursor.execute("UPDATE subscriptions SET status='CANCELLED' where NMR>2"
print "Cancelling "+str(int(k))+" subscriptions with 3+ NMRs."

k = cursor.execute("SELECT users.id FROM users, subscriptions WHERE users.id=subscriptions.user and subscriptions.status='ACTIVE' and subscriptions.game="+str(game))
while k!=0:
    k=k-1
    user = int(cursor.fetchone()[0])
    update=db.cursor()
    update.execute("INSERT INTO transactions (user,date,balance,description) VALUES ("+str(user)+", '"+date+"', -"+str(price)+", 'ZAT-"+str(game)+"')")

