#!/usr/bin/python
## This script is called when an email from the user arrives
## in reply to the registration form's confirmation email.
## It's the first time the user is added to the database.

import MySQLdb
import sys
import re
import string
from whrandom import choice

import locking
from locking import lock, unlock

dbname=sys.argv[2]
db=MySQLdb.connect(db=dbname)

lock(sys.argv[1]+".err")
errors=open(sys.argv[1]+".err", 'a')

def validrace(race):
  validraces=('GOBLIN', 'DWARF', 'ELF', 'HALFLING', 'INSECT', 'AQUARIAN', 'HUMAN', 'CAT', 'TROLL', 'ORC', 'DEMON')
  name = string.upper(str(race))
  if name in validraces:
    return name
  return None

def genpasswd():
  newpasswd=""
  chars = string.letters + string.digits
  for i in range(8):
    newpasswd = newpasswd + choice(chars)
  return newpasswd

def error(email, message):
  errors.write(email + " "+ message+"\n");

matchfrom=re.compile(
  r"""From ([^\s]*)""",
  re.IGNORECASE | re.DOTALL | re.VERBOSE)

matchfirstname=re.compile(
  r""".*vorname:\s*([^\n\r]*)""",
  re.IGNORECASE | re.DOTALL | re.VERBOSE)
firstname=None

matchlastname=re.compile(
  r""".*nachname:\s*([^\n\r]*)\s*""",
  re.IGNORECASE | re.DOTALL | re.VERBOSE)
lastname=None

matchemail=re.compile(
  r""".*email:\s*([^\n\r]*)\s*""",
  re.IGNORECASE | re.DOTALL | re.VERBOSE)
email=None

matchuserid=re.compile(
  r""".*userid:\s*([^\n\r]*)\s*""",
  re.IGNORECASE | re.DOTALL | re.VERBOSE)
userid=None

matchpassword=re.compile(
  r""".*passwort:\s*([^\n\r]*)\s*""",
  re.IGNORECASE | re.DOTALL | re.VERBOSE)
password=None

matchaddress=re.compile(
  r""".*adresse:\s*([^\n\r]*)\s*""",
  re.IGNORECASE | re.DOTALL | re.VERBOSE)
address=None

matchcity=re.compile(
  r""".*ort:\s*([^\n\r]*)\s*""",
  re.IGNORECASE | re.DOTALL | re.VERBOSE)
city=None

matchcountry=re.compile(
  r""".*land:\s*([^\n\r]*)""",
  re.IGNORECASE | re.DOTALL | re.VERBOSE)
country=None

matchphone=re.compile(
  r""".*telefon:\s*([^\n\r]*)\s*""",
  re.IGNORECASE | re.DOTALL | re.VERBOSE)
phone=None

matchnewrace=re.compile(
  r""".*neues\sspiel:\s*([^\n\r]+)\s*""",
  re.IGNORECASE | re.DOTALL | re.VERBOSE)
newrace=0

matcholdrace=re.compile(
  r""".*altes\sspiel:\s*([^\n\r]+)\s*""",
  re.IGNORECASE | re.DOTALL | re.VERBOSE)
oldrace=0

matchstandin=re.compile(
  r""".*Stand-In:\s(on)\s*""",
  re.IGNORECASE | re.DOTALL | re.VERBOSE)
standin=0

matchwaiting=re.compile(
  r""".*Warteliste:\s(on)\s*""",
  re.IGNORECASE | re.DOTALL | re.VERBOSE)
waiting=0

for line in sys.stdin.readlines():
  match=matchwaiting.match(line)
  if (match!=None):
    waiting=1
    continue
  match=matchstandin.match(line)
  if (match!=None):
    standin=1
    continue
  match=matcholdrace.match(line)
  if (match!=None):
    oldrace=match.group(1)
    continue
  match=matchnewrace.match(line)
  if (match!=None):
    newrace=match.group(1)
    continue

  match=matchfrom.match(line)
  if (match!=None):
    email=match.group(1)
    continue

  match=matchuserid.match(line)
  if (match!=None):
    userid=match.group(1)
    continue
  match=matchpassword.match(line)
  if (match!=None):
    password=match.group(1)
    continue

  match=matchfirstname.match(line)
  if (match!=None):
    firstname=match.group(1)
    continue
  match=matchlastname.match(line)
  if (match!=None):
    lastname=match.group(1)
    continue
  match=matchemail.match(line)
  if (match!=None):
    email=match.group(1)
    continue
  match=matchaddress.match(line)
  if (match!=None):
    address=match.group(1)
    continue
  match=matchcity.match(line)
  if (match!=None):
    city=match.group(1)
    continue
  match=matchcountry.match(line)
  if (match!=None):
    country=match.group(1)
    continue
  match=matchphone.match(line)
  if (match!=None):
    phone=match.group(1)
    continue

oldrace=validrace(oldrace)
newrace=validrace(newrace)

okay=0
if (password!=None) & (userid!=None):
    query="select id from users where password='" + password + "' and id="+str(userid)
    cursor=db.cursor()
    num=cursor.execute(query)
    if num==1:
	okay=1
    else:
	if email==None:
	    email="enno@eressea.upb.de"
	error(email, "Das Passwort für Kundennummer "+str(userid)+" war nicht korrekt.")

else:
    if email==None:
	error("enno@eressea.upb.de",
	"Es wurde keine Emailadresse angegeben: "+firstname+" "+lastname)
    elif (oldrace==None) & (newrace==None) & (standin==0) & (waiting==0):
	error(email, "Es wurde kein Spiel ausgewählt.")
    elif (firstname==None):
	error(email, "Es wurde kein Vorname angegeben")
    elif (lastname==None):
	error(email, "Es wurde kein Nachname angegeben")
    elif (address==None):
	error(email, "Es wurde keine Adresse angegeben")
    elif (city==None):
	error(email, "Es wurde kein Wohnort angegeben")
    elif (country==None):
	error(email, "Es wurde kein Land angegeben")
    else:
	if (phone==None):
	    phone = "NULL"
	okay=1
	cursor=db.cursor()
	cursor.execute("INSERT INTO users (firstname, lastname, email, address, city, phone, country, password) "+
	  "VALUES ('"+firstname+"', '"+lastname+"', '"+email+"', '"+address+"', '"+city+"', '"+phone+"', "+country+", '"+genpasswd()+"')")
    
	cursor.execute("SELECT LAST_INSERT_ID() from dual")
	userid=str(int(cursor.fetchone()[0]))

if okay:
    if (oldrace!=None):
	error(email, "Derzeit wird kein Spiel nach alten Regeln angeboten")
#	cursor.execute("INSERT INTO subscriptions (user, race, game) "+
#	  "VALUES ("+str(userid)+", '"+oldrace+"', 1)")
    if (newrace!=None):
	error(email, "Derzeit wird kein Spiel nach neuen Regeln angeboten")
#	cursor.execute("INSERT INTO subscriptions (user, race, game, status) "+
#	  "VALUES ("+str(userid)+", '"+newrace+"', 2, 'WAITING')")
    if waiting:
	cursor.execute("INSERT INTO subscriptions (user, game, status) "+
	  "VALUES ("+str(userid)+", 3, 'WAITING')")
    if standin:
	cursor.execute("INSERT INTO subscriptions (user, game, status) "+
	  "VALUES ("+str(userid)+", 4, 'WAITING')")

errors.close()
unlock(sys.argv[1]+".err")
