#!/usr/bin/env python
import MySQLdb
import sys
import smtplib

From='accounts@vinyambar.de'
dbname=sys.argv[1]
dryrun=0
if len(sys.argv)>2:
    dryrun=1
price=1.25
warnahead=2
db=MySQLdb.connect(db=dbname)
cursor=db.cursor()

users = cursor.execute("select users.email, users.id, count(subscriptions.game) from users, subscriptions, games where users.id=subscriptions.user and subscriptions.game=games.id and users.id!=0 and games.status='RUNNING' GROUP BY users.id")
server=smtplib.SMTP('localhost')
while users > 0:
    users=users-1
    email, uid, games = cursor.fetchone()
    c2 = db.cursor()
    t = c2.execute("select sum(balance) from transactions where user="+str(int(uid)))
    if t==0:
	balance=0.0
    else:
	balance = c2.fetchone()[0]
    if balance < games*warnahead*price:
	print "Warning "+email+", uid="+str(int(uid))+", balance="+str(balance)+", games="+str(int(games))
	Msg = ("From: Vinyambar Buchhaltung <"+From+">\nTo: "+email+"\nSubject: Vinyambar Kontostand, Warnung.\n\n"+
	  "Dein Vinyambar Kontostand reicht nicht mehr aus, um die kommenden "+str(warnahead)+"\n"+
	  "Wochen zu bezahlen. Bitte mache baldmöglichst eine neue Überweisung\n"
	  "auf das Vinyambar-Konto.\n\n"+
	  "Kundennummer:   "+str(uid)+"\n"+
	  "Kontostand:     "+str(balance)+" EUR\n"+
	  "\n"+
	  "Unser Konto: Katja Zedel\n"+
	  "Kontonummer 1251 886 106\n"+
	  "BLZ 500 502 01 (Frankfurter Sparkasse)\n")
	try:
	    # print Msg
	    if dryrun==0:
		server.sendmail(From, email, Msg)
	except:
	    print "Could not send confirmation to "+email
	    print "Exception is:", sys.exc_type, ":", sys.exc_value
	
