#!/usr/bin/env python

import MySQLdb
import sys
import string
from whrandom import choice

dbname=sys.argv[1]
maxnum=int(sys.argv[2])
game_id=0 # eressea is game 0, tutorial is 1

def genpasswd():
    newpasswd=""
    chars = string.letters + string.digits
    for i in range(8):
	newpasswd = newpasswd + choice(chars)
    return newpasswd

query = "select distinct u.email, s.id, s.password, r.name, u.locale, s.bonus from users u, races r, subscriptions s left join userips i on u.id=i.user left join bannedips b on i.ip=b.ip where s.user=u.id and b.ip is NULL and s.status='CONFIRMED' and r.race=s.race and s.game="+str(game_id)+" and r.locale='de' order by s.id"
db=MySQLdb.connect(db=dbname)
cursor = db.cursor()
c = db.cursor()
num=cursor.execute(query)
if num>maxnum:
    num=maxnum
while num:
    num=num-1
    email, sid, password, race, locale, bonus = cursor.fetchone()
    if bonus==None:
	bonus=0
    if password==None:
	password=genpasswd()
	c.execute("UPDATE subscriptions set password='"+password+"' where id="+str(int(sid)))

    print email+" "+race+" "+locale+" "+str(int(bonus))+" "+password
