#!/usr/bin/env python
import MySQLdb
import sys
import os

dbname=sys.argv[1]
gid=int(sys.argv[2])
date=sys.argv[3]
price=1.25
db=MySQLdb.connect(db=dbname)
patchdir='/home/eressea/vinyambar-rsync/vin'+str(gid)+'/data'

cursor=db.cursor()

def SetUser(cursor, num, newuser):
    update = db.cursor()
    while num:
	num = num-1
	uid, faction = cursor.fetchone()
	update.execute("insert into transfers (faction, game, src,dst) values ('"+faction+"', "+str(gid)+", "+str(int(uid))+", 0)")
	update.execute("update subscriptions set user="+str(newuser)+" where game="+str(gid)+" and faction='"+faction+"'")


k = cursor.execute('select name, patch from games where id='+str(gid))
if k==0:
    print "Unbekanntes Spiel"

name, patch = cursor.fetchone()

os.system("mysqldump vinyambar > backup-vinyambar-"+str(int(patch))+".sql")

k = cursor.execute("SELECT max(lastturn) from subscriptions")
lastturn = int(cursor.fetchone()[0])

print "Auswertung für " + name + " Patch Level " + str(int(patch)) + ", Runde "+str(lastturn)
while os.access(patchdir+'/patch-'+str(int(patch+1))+'.sql', os.F_OK):
    patch=patch+1
    print "  Patching to level "+str(patch)
    os.system('mysql ' + dbname + ' < ' + patchdir+'/patch-'+str(int(patch))+'.sql')
    cursor.execute('update games set patch='+str(int(patch))+' where id='+str(gid))

k = cursor.execute("select user, faction from subscriptions where game="+str(gid)+" and status='TRANSFERED' and user!=0 and updated<'"+date+"'")
print "Removing "+str(int(k))+" transfered subscriptions."
SetUser(cursor, int(k), 0)

k = cursor.execute("select user, faction from subscriptions where game="+str(gid)+" and status='CANCELLED' and user!=0 and updated<'"+date+"'")
print "Removing "+str(int(k))+" cancelled subscriptions."
SetUser(cursor, int(k), 0)

k = cursor.execute("select user, faction from subscriptions where game="+str(gid)+" and user!=0 and status='DEAD'")
print "Removing "+str(int(k))+" dead subscriptions."

k = cursor.execute("UPDATE subscriptions SET status='CANCELLED' where game="+str(gid)+" and status='ACTIVE' and lastturn+3<="+str(lastturn))
if k:
    print "Cancelling subscriptions with 3+ NMRs."

cursor.execute("SELECT count(*) from transactions where date='"+date+"' and description='ZAT-"+str(gid)+"'")
k = cursor.fetchone()[0]
if k==0:
    k = cursor.execute("SELECT users.id FROM users, subscriptions WHERE users.id=subscriptions.user and subscriptions.status='ACTIVE' and subscriptions.game="+str(gid))
    print "Billing "+str(int(k))+" users."
    while k!=0:
	k=k-1
	user = int(cursor.fetchone()[0])
	update=db.cursor()
	update.execute("INSERT INTO transactions (user,date,balance,description) VALUES ("+str(user)+", '"+date+"', -"+str(price)+", 'ZAT-"+str(gid)+"')")
else:
    print "ERROR: ZAT for game "+str(gid)+" has already been done."
