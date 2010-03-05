#!/usr/bin/env python

# duplicates.py
# this script will find users that registered from the same IP,
# where at least one of them is currenly ACTIVE.

import MySQLdb
import sys

dbname=sys.argv[1]
db = MySQLdb.connect(db=dbname)
cursor = db.cursor()
dupes = cursor.execute("select count(*) sum, ip from users,userips where users.id=userips.user and status!='EXPIRED' group by ip having sum>1")

while dupes:
    dupes=dupes-1
    sum, ip = cursor.fetchone()
    c = db.cursor()
    c.execute("select count(*) from users, userips where users.id=userips.user and status='CONFIRMED' and ip='"+ip+"'")
    (active,) = c.fetchone()
    if active:
	users = c.execute("select id, email, firstname, lastname, status from users, userips where userips.user=users.id and ip='"+ip+"'")
	if users:
	    print ip
	    while users:
		users=users-1
		uid, email, firstname, lastname, status = c.fetchone()
		print "\t"+str(int(uid)) +"("+status+")\t"+firstname+" "+lastname+" <"+email+">"
	    print "\n"
    
