#!/usr/bin/env python

## this cgi script adds a new user to the eressea DB,
## as well as a new subscription entry. it logs IP addresses for registrations.

import sys
import MySQLdb
import os
import cgi
import re
import string
import smtplib
from whrandom import choice

# specify the filename of the template file
HTMLTemplate = "eressea.html"
MailTemplate="register.mail"
DefaultTitle = "Eressea Anmeldung"
dbname = "eressea"
From = "accounts@eressea-pbem.de"
locale="de"
smtpserver = 'localhost'
db=None
game_id=0  # Eressea Main game
tutorial_id=None # 1 to active, None to disable tutorials

# define a new function called Display
# it takes one parameter - a string to Display
def Display(Content, Title=DefaultTitle):
    TemplateHandle = open(HTMLTemplate, "r")  # open in read only mode
    # read the entire file as a string
    TemplateInput = TemplateHandle.read()
    TemplateHandle.close()                    # close the file

    # this defines an exception string in case our
    # template file is messed up
    BadTemplateException = "There was a problem with the HTML template."

    SubResult = re.subn("<!-- INSERT TITLE HERE -->", Title, TemplateInput)
    SubResult = re.subn("<!-- INSERT CONTENT HERE -->", Content, SubResult[0])
    if SubResult[1] == 0:
        raise BadTemplateException

    print "Content-Type: text/html\n\n"
    print SubResult[0]
    return


def Send(email, custid, sid, firstname, password, game):
    cursor=db.cursor()
    cursor.execute("select count(*), g.name from games g, subscriptions s where g.id="+str(game)+" AND (s.status='WAITING' or s.status='CONFIRMED') AND g.id=s.game GROUP BY g.id")
    position, name = cursor.fetchone()
    TemplateHandle = open(MailTemplate+"."+string.lower(name)+"."+locale, "r")  # open in read only mode
    # read the entire file as a string
    TemplateInput = TemplateHandle.read()
    TemplateHandle.close()                    # close the file

    SubResult = re.subn("<FIRSTNAME>", firstname, TemplateInput)
    SubResult = re.subn("<PASSWORD>", password, SubResult[0])
    SubResult = re.subn("<POSITION>", str(int(position)), SubResult[0])
    SubResult = re.subn("<GAME>", name, SubResult[0])
    SubResult = re.subn("<CUSTID>", str(int(custid)), SubResult[0])
    subject={"de":"Anmeldung","en":"Registration"}
    Msg="From: "+From+"\nTo: "+email+"\nSubject: "+name+" "+subject[locale]+"\n\n"
    Msg=Msg+SubResult[0]
    server=smtplib.SMTP(smtpserver)
    server.sendmail(From, email, Msg)
    server.close()
    return

def GetKey(Form, key):
    if Form.has_key(key):
	value=Form[key].value
	if value!="":
	    return value
    return None

def ValidEmail(email):
    if string.find(email, "@")==-1:
	return 0
    elif string.find(email, " ")!=-1:
	return 0
    return 1

def genpasswd():
    newpasswd=""
    chars = string.letters + string.digits
    for i in range(8):
	newpasswd = newpasswd + choice(chars)
    return newpasswd

#Display("Derzeit ist wegen einer technischen Umstellung keine Anmeldung möglich")
#sys.exit(0)

Form = cgi.FieldStorage()

email=GetKey(Form, "email")
firstname=GetKey(Form, "firstname")
lastname=GetKey(Form, "lastname")
#info=GetKey(Form, "info")
address=GetKey(Form, "address")
city=GetKey(Form, "city")
country=GetKey(Form, "country")
phone=GetKey(Form, "phone")
race=GetKey(Form, "race")
locale=GetKey(Form, "locale")

referrer=GetKey(Form, "referrer")
firsttime=GetKey(Form, "firsttime")
bonus=GetKey(Form, "bonus")

if (locale==None) or (lastname==None) or (race==None) or (firstname==None) or (address==None) or (city==None):
    output="<p>Um Dich zu Eressea anzumelden musst Du das Formular vollständig ausfüllen.\n "
    for key in Form.keys():
	output=output+"<br>"+key+": "+Form[key].value+"\n"
    Display(output)
elif ValidEmail(email)==0:
    output="<p>Um Dich zu Eressea anzumelden musst Du eine gültige Email-Adresse angeben.\n "
    Display(output)
else:
    db=MySQLdb.connect(db=dbname)
    cursor=db.cursor()
    # check if he is already entered in the main game:
    exist=cursor.execute("select u.id from users u, subscriptions s where s.user=u.id AND s.game="+str(game_id)+" AND u.email='"+email+"' and (s.status='WAITING' or s.status='CONFIRMED')")
    if exist:
	text={"de":"Du stehst bereits auf der Warteliste","en":"You are already on the waiting list"}
	Display('<p>'+text[locale])
    else:
	bans=cursor.execute('select regex, reason from bannedemails')
	while bans:
	    bans=bans-1
	    regexp, reason = cursor.fetchone()
	    if (re.match(regexp, email, re.IGNORECASE))!=None:
		Display('Deine Email-Adresse ist für Eressea nicht zugelassen. '+reason)
		sys.exit(0)

	# create a new user record
        fields = "firstname, lastname, locale, email, address, city"
	values = "'"+firstname+"', '"+lastname+"', '"+locale+"', '"+email+"', '"+address+"', '"+city+"'"
	if phone!=None:
	    fields=fields+", phone"
	    values=values+", '"+phone+"'"
#	if info!=None:
#	    fields=fields+", info"
#	    values=values+", '"+info+"'"
	if country!=None:
	    fields=fields+", country"
	    values=values+", "+country+""
	if referrer!=None:
	    fields=fields+", referrer"
	    values=values+", '"+referrer+"'"
	if firsttime!=None:
	    fields=fields+", firsttime"
	    if firsttime=='yes':
		values=values+", 1"
	    else:
		values=values+", 0"
	exist=cursor.execute("select password, status, id from users where email='"+email+"'")
	custid=0
	status='NEW'
	if exist:
	    # user exists, update his data
	    password, status, custid=cursor.fetchone()
	    if status=='BANNED':
		Display('<em>Dein Account ist gesperrt.</em><br>Bitte wende Dich an <a href="mailto:accounts@eressea-pbem.de">accounts@eressea-pbem.de</a> falls Du Fragen zu dieser Meldung hast.')
		sys.exit(0)
	    fields=fields+", id, status"
	    values=values+", "+str(custid)+", '"+status+"'"
	    command="REPLACE"
	    cursor.execute("REPLACE into users ("+fields+") VALUES ("+values+")")
	else:
	    password = genpasswd()
	    fields=fields+", password, status"
	    values=values+", '"+password+"', 'NEW'"
	    cursor.execute("INSERT into users ("+fields+") VALUES ("+values+")")
	    cursor.execute("SELECT LAST_INSERT_ID() from dual")
	    custid=cursor.fetchone()[0]

	# track IP addresses
	ip=None
	if os.environ.has_key('REMOTE_ADDR'):
	    ip=os.environ['REMOTE_ADDR']
	if ip!=None:
	    cursor.execute("REPLACE userips (ip, user) VALUES ('"+ip+"', "+str(int(custid))+")")
	
	# add a subscription record
	password = genpasswd()
	values="'WAITING', '"+password+"'"
	fields="status, password"
	game = game_id
	if tutorial_id!=None and status!='ACTIVE':
	    game=tutorial_id
	if bonus!=None:
	    fields=fields+", bonus"
	    if bonus=='yes':
		values=values+", 1"
	    else:
		values=values+", 0"
	cursor.execute("insert into subscriptions (user, race, game, "+fields+") VALUES ("+str(int(custid))+", '"+race+"', "+str(game)+", "+values+")")
	cursor.execute("SELECT LAST_INSERT_ID() from dual")
	sid = cursor.fetchone()[0]
	Send(email, custid, sid, firstname, password, game)
	text={"de":"Deine Anmeldung wurde bearbeitet. Eine EMail mit Hinweisen ist unterwegs zu Dir", "en":"Your application was processed. An email containing further instructions is being sent to you"}
	Display("<p>"+text[locale]+".")
    db.close()
