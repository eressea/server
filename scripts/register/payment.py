#!/usr/bin/env python

import MySQLdb
import sys
import smtplib

From='accounts@vinyambar.de'
dbname=sys.argv[1]
userid=sys.argv[2]

db=MySQLdb.connect(db=dbname)
cursor=db.cursor()
locale="de"

i=cursor.execute('SELECT email, firstname, lastname FROM users, transactions WHERE users.id='+str(userid))

if i==0:
    print "Unknown user "+str(userid)
    sys.exit()

email, firstname, lastname = cursor.fetchone()
i=cursor.execute('SELECT sum(balance) from transactions WHERE user='+str(userid))
balance=cursor.fetchone()[0]
if balance==None:
    balance=0.0
print 'Balance for '+firstname+' '+lastname+' is '+str(balance)+' DEM'

if len(sys.argv)>4:
    cash=float(sys.argv[3])
    date=sys.argv[4]
    reason='PAYMENT'
    if len(sys.argv)>5:
	reason=sys.argv[5]

    cursor.execute("UPDATE users SET status='PAYING' WHERE id="+str(userid));
    
    cursor.execute('INSERT transactions (user, balance, description, date) VALUES ('+str(userid)+', '+str(cash)+', \''+reason+'\', \''+date+'\')')
    cursor.execute('SELECT LAST_INSERT_ID() FROM transactions WHERE user='+str(userid));
    lastid = int(cursor.fetchone()[0])
    result = cursor.execute('SELECT text FROM descriptions WHERE locale=\''+locale+'\' AND handle=\''+reason+'\'')
    if result!=0:
	reason = cursor.fetchone()[0]

    print 'Transaction #'+str(lastid)+', new balance is '+str(balance+cash)

    Msg = ("From: Vinyambar Buchhaltung <"+From+">\nTo: "+email+"\nSubject: Vinyambar Zahlungseingang.\n\n"+
      "Kundennummer:       "+str(userid)+"\n"+
      "Eingangsdatum:      "+date+"\n"+
      "Transaktionsnummer: "+str(lastid)+"\n"+
      "Alter Kontostand:   "+str(balance)+" DEM\n"+
      "Zahlungseingang:    "+str(cash)+" DEM\n"+
      "Neuer Kontostand:   "+str(balance+cash)+" DEM\n"+
      "Verwendungszweck:   "+reason+"\n"+
      "\n"+
      "Deine Zahlung ist eingegangen und wurde auf dein Spielerkonto verbucht.\n")

    try:
	server=smtplib.SMTP('localhost')
	server.sendmail(From, email, Msg)
    except:
	print "Could not send confirmation to "+email
	print "Exception is:", sys.exc_type, ":", sys.exc_value

cursor.execute("select count(*) from users u, transactions t where u.id=t.user group by u.id having sum(t.balance)!=0.0")
count = cursor.fetchone()[0]
cursor.execute("select sum(transactions.balance) from transactions")
balance = cursor.fetchone()[0]
print str(balance)+ " DEM (" + str(balance/1.955830)+ " EUR) in " + str(int(count)) + " Konten"

