#!/usr/bin/env python
import MySQLdb
import sys

dbname=sys.argv[1]
game=int(sys.argv[2])
date=sys.argv[3]
price=2.5
db=MySQLdb.connect(db=dbname)
cursor=db.cursor()

k = cursor.execute("SELECT users.id FROM users,subscriptions WHERE users.id=subscriptions.user and subscriptions.game="+str(game))
while k!=0:
    k=k-1
    user = int(cursor.fetchone()[0])
    update=db.cursor()
    update.execute("INSERT INTO transactions (user,date,balance,description) VALUES ("+str(user)+", '"+date+"', -"+str(price)+", 'ZAT-"+str(game)+"')")
