#!/usr/bin/env python

import MySQLdb
import re
import sys
import smtplib


dbname=sys.argv[1]
template=sys.argv[2]
state=sys.argv[3]
tostate=sys.argv[3]

server=smtplib.SMTP('localhost')
db=MySQLdb.connect(db=dbname)
From="accounts@vinyambar.de"

cursor=db.cursor()
query=("select users.id, users.email, users.firstname "+
  "from users, games, subscriptions "+
  "where users.id=subscriptions.user and subscriptions.game=games.id and "+
  "users.balance=0 and users.status='"+state+"'")

users=cursor.execute(query)
print "Sending confirmation to "+str(int(users))+" users"
while users!=0:
    users=users-1
    custid, email, firstname =cursor.fetchone()

    infile=open(template,"r")
    line = infile.read()

    line = re.sub('<CUSTID>', custid, line)
    line = re.sub('<FIRSTNAME>', firstname, line)
    line = re.sub("<GAMENAME>", game, line)

    Msg = ("From: "+From+"\nTo: "+email+"\n"+
      "Subject: Vinyambar Kontoinformationen.\n\n"+
      line)

    try:
        server.sendmail(From, email, Msg)
        update=db.cursor()
        update.execute("UPDATE users set status='"+tostate+"' WHERE id="+custid)
	print "Sent '"+template+"' information to "+email

    except:
        print "Could not inform "+To
        print "Reason was: '"+Reason+"'"
        print "Exception is:", sys.exc_type, ":", sys.exc_value

    infile.close()
