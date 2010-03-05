#!/usr/bin/env python

import MySQLdb
import sys

dbname=sys.argv[1]
db=MySQLdb.connect(db=dbname)

cursor=db.cursor()
bans = cursor.execute("select user, users.email, users.status, userips.ip from bannedips, users, userips where users.status!='BANNED' and users.id=userips.user and userips.ip=bannedips.ip")
bc = db.cursor()
while bans:
    bans=bans-1
    user, email, status, ip = cursor.fetchone()
    if status!='ACTIVE':
	bc.execute("update users set status='BANNED' where id="+str(int(user)))
    else:
	print email + " is active, and playing from banned ip "+ip
