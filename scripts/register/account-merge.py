#!/usr/bin/env python

import MySQLdb
import sys

dbname=sys.argv[1]
userid=int(sys.argv[2])
oldid=int(sys.argv[3])

db=MySQLdb.connect(db=dbname)
cursor=db.cursor()

i=cursor.execute("select distinct email from users where id="+str(oldid)+" or id="+str(userid))

if i==0:
    print "Could not find specified usernames"
    sys.exit()

if i>1:
    print "EMail addresses do not match"
    i=cursor.execute("select id, email from users where id="+str(oldid)+" or id="+str(userid))
    while i>0:
	i=i-1
	id, email = cursor.fetchone()
	print "  "+str(int(id))+"  "+email
    sys.exit()

i=cursor.execute("select id, email, balance from users where id="+str(oldid)+" or id="+str(userid))
if i!=2:
    print "Could not find both customer ids"
    while i>0:
	i=i-1
	id, email, balance = cursor.fetchone()
	print "  "+str(int(id))+"  "+email
    sys.exit()

bal=0.0
while i>0:
    i=i-1
    id, email, balance = cursor.fetchone()
    bal=bal+balance

cursor.execute("update users set balance="+str(bal)+" where id="+str(userid))
cursor.execute("delete from users where id="+str(oldid))
cursor.execute("update transactions set user="+str(userid)+" where user="+str(oldid))
cursor.execute("update subscriptions set user="+str(userid)+" where user="+str(oldid))
print "Customer records have been merged"
