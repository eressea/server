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

for line in sys.stdin.readlines():
  match=matchfrom.match(line)
  if (match!=None):
    email=match.group(1)
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

if email==None:
    error("enno@eressea.upb.de",
    "Es wurde keine Emailadresse angegeben: "+firstname+" "+lastname)
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
	
    cursor=db.cursor()
    cursor.execute("INSERT INTO users (firstname, lastname, email, address, city, phone, country, password) "+
      "VALUES ('"+firstname+"', '"+lastname+"', '"+email+"', '"+address+"', '"+city+"', '"+phone+"', "+country+", '"+genpasswd()+"')")
    
      cursor.execute("SELECT LAST_INSERT_ID() from dual")
      userid=str(int(cursor.fetchone()[0]))

    cursor.execute("INSERT INTO subscriptions (user, game, status) "+
      "VALUES ("+str(userid)+", 3, 'WAITING')")

errors.close()
unlock(sys.argv[1]+".err")
