#!/usr/bin/env python

import MySQLdb
import sys
import smtplib

From='accounts@vinyambar.de'
dbname=sys.argv[1]

server=smtplib.SMTP('localhost')
db=MySQLdb.connect(db=dbname)
cursor=db.cursor()

i=cursor.execute('SELECT email, balance, firstname, lastname FROM users WHERE balance>0.0')

while i>0:
    email, balance, firstname, lastname = cursor.fetchone()
    print 'Balance for '+firstname+' '+lastname+' is '+str(balance)
    i=i-1
