#!/usr/bin/env python

import MySQLdb
import re
import sys

dbname=sys.argv[1]
file=sys.argv[2]
game=int(sys.argv[3])
infile=open('update.log', 'r')
db=MySQLdb.connect(db=dbname)
cursor=db.cursor()

matchrenum=re.compile(
  r"""renum\s(.*)\s(.*)""",
  re.IGNORECASE | re.DOTALL | re.VERBOSE)

matchdropout=re.compile(
  r"""dropout\s(.*)""",
  re.IGNORECASE | re.DOTALL | re.VERBOSE)

for line in infile.readlines():
	match=matchrenum.match(line)
	if match!=None:
		fold=match.group(1)
		fnew=match.group(2)
		i=cursor.execute("update subscriptions set faction='"+fnew+"' where game="+str(game)+" and faction='"+fold+"')
		if i!=1:
			print "could not renum faction "+fold+" to new id "+fnew
