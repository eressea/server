#!/usr/bin/env python

import MySQLdb
import sys

dbname=sys.argv[1]
maxnum=int(sys.argv[2])

query = "select distinct u.email, s.race, u.locale from users u, subscriptions s left join userips i on u.id=i.user left join bannedips b on i.ip=b.ip where s.user=u.id and b.ip is NULL and u.status='CONFIRMED' order by u.id"
db=MySQLdb.connect(db=dbname)
cursor = db.cursor()
num=cursor.execute(query)
if num>maxnum:
    num=maxnum
while num:
    num=num-1
    email, race, locale = cursor.fetchone()
    print email+" "+race+" "+locale+" 0"
