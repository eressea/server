#!/usr/bin/env python

import MySQLdb
import sys
import smtplib

From='accounts@vinyambar.de'
#!/usr/bin/env python

import re
import MySQLdb
import sys

def pay(db, userid, email, cash, date, reason='PAYMENT'):
    cursor=db.cursor()
    locale="de"
    cursor.execute("UPDATE users SET status='PAYING' WHERE id="+str(userid));
    
    cursor.execute('INSERT transactions (user, balance, description, date) VALUES ('+str(userid)+', '+str(cash)+', \''+reason+'\', \''+date+'\')')
    cursor.execute('SELECT LAST_INSERT_ID() FROM transactions WHERE user='+str(userid));
    lastid = int(cursor.fetchone()[0])
    result = cursor.execute('SELECT text FROM descriptions WHERE locale=\''+locale+'\' AND handle=\''+reason+'\'')
    if result!=0:
	reason = cursor.fetchone()[0]

    i=cursor.execute('SELECT sum(balance) from transactions WHERE user='+str(userid))
    balance=cursor.fetchone()[0]
    if balance==None:
	balance=0.0
    Msg = ("From: Vinyambar Buchhaltung <"+From+">\nTo: "+email+"\nSubject: Vinyambar Zahlungseingang.\n\n"+
      "Kundennummer:       "+str(userid)+"\n"+
      "Eingangsdatum:      "+date+"\n"+
      "Transaktionsnummer: "+str(lastid)+"\n"+
      "Alter Kontostand:   "+str(balance)+" EUR\n"+
      "Zahlungseingang:    "+str(cash)+" EUR\n"+
      "Neuer Kontostand:   "+str(balance+cash)+" EUR\n"+
      "Verwendungszweck:   "+reason+"\n"+
      "\n"+
      "Deine Zahlung ist eingegangen und wurde auf dein Spielerkonto verbucht.\n")

    try:
	server=smtplib.SMTP('localhost')
	server.sendmail(From, email, Msg)
    except:
	print "Could not send confirmation to "+email
	print "Exception is:", sys.exc_type, ":", sys.exc_value
    return 

def charge(ids, balance, kto, blz, date):
    if len(ids):
	custids = []
	db=MySQLdb.connect(db=dbname)
	cursor = db.cursor()
	for custid in ids:
	    k = cursor.execute('SELECT firstname, lastname FROM users WHERE id='+str(custid))
	    if k:
		custids.append(custid)
	if len(custids)==1:
	    k = cursor.execute('SELECT balance from transactions where USER='+str(custid)+" and description='PAYMENT' and date='"+date+"'")
	    if k:
		print "user already had a transaction today, not adding it for safety reasons"
	    else:
		cursor.execute('SELECT email FROM users WHERE users.id='+str(custid))
		email = cursor.fetchone()[0]
		pay(db, custid, email, balance, date)
		return 0
	else:
	    print "zero or more than one possible customerid found:", custids
    return -1

def eumel(dbname):
    balance=None
    kto=None
    blz=None
    date=None
    zweck=[]
    ids = []
    rv=0
    for line in sys.stdin.readlines():
	match=re.match('Buchung/Wert.* / ([0-9][0-9][0-9][0-9])([0-9][0-9])([0-9][0-9]).*H (.*),(.*) EUR', line)
	if match!=None:
	    if (balance!=None):
		r = charge(ids, balance, kto, blz, date)
		if r!=0:
		    print "FAILED", balance, kto, blz, date, zweck
		    rv=r
		balance=None
		kto=None
		blz=None
		date=None
		ids = []
	    date=match.groups()[0]+'-'+match.groups()[1]+'-'+match.groups()[2]
	    balance=float(match.groups()[3]+'.'+match.groups()[4])
	    continue
	match=re.match(' KTO/BLZ\s*([0-9]*) / ([0-9]*)', line)
	if match!=None:
	    kto, blz = match.groups()
	    continue
	match=re.match(' VZweck [0-9] *(.*)', line)
	if match!=None:
	    zweck.append(line)
	    line=match.groups()[0]
	    while len(line):
		match = re.match('(.*[^ ]) +(.+)', line)
		if (match!=None):
		    line, value = match.groups()
		else:
		    value = line
		    line=''
		try:
		    custid = int(value)
		    ids.append(custid)
		except ValueError:
		    continue
    if (balance):
	r = charge(ids, balance, kto, blz, date)
	if r!=0:
	    print "FAILED", balance, kto, blz, date, zweck
	    rv=r
    return rv

def manual(dbname):
    userid=sys.argv[2]

    db=MySQLdb.connect(db=dbname)
    cursor=db.cursor()

    i=cursor.execute('SELECT email, firstname, lastname FROM users, transactions WHERE users.id='+str(userid))
    if i==0:
	print "Unknown user "+str(userid)
	sys.exit()

    email, firstname, lastname = cursor.fetchone()
    i=cursor.execute('SELECT sum(balance) from transactions WHERE user='+str(userid))
    balance=cursor.fetchone()[0]
    if balance==None:
	balance=0.0

    print 'Balance for '+firstname+' '+lastname+' is '+str(balance)+' EUR'

    if len(sys.argv)>4:
	cash=float(sys.argv[3])
	date=sys.argv[4]
	reason='PAYMENT'
	if len(sys.argv)>5:
	    reason=sys.argv[5]

	pay(db, int(userid), email, cash, date, reason)
	print 'New balance is '+str(balance+cash)+' EUR'
    return

dbname=sys.argv[1]
if sys.argv[2]=='--eumel':
    r = eumel(dbname)
    sys.exit(r)
else:
    manual(dbname)
