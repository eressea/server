#!/usr/bin/env python

import sys
import MySQLdb
import cgi
import re
import string
import smtplib
from whrandom import choice

# specify the filename of the template file
scripturl="http://eressea.upb.de/~enno/cgi-bin/eressea-register.py"
HTMLTemplate = "eressea.html"
MailTemplate="register.mail"
DefaultTitle = "Eressea Anmeldung"
dbname = "eressea"
From = "accounts@eressea-pbem.de"
smtpserver = 'localhost'
db=None

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


def Send(email, custid, firstname, password, position):
    TemplateHandle = open(MailTemplate, "r")  # open in read only mode
    # read the entire file as a string
    TemplateInput = TemplateHandle.read()
    TemplateHandle.close()                    # close the file

    SubResult = re.subn("<FIRSTNAME>", firstname, TemplateInput)
    SubResult = re.subn("<PASSWORD>", password, SubResult[0])
    SubResult = re.subn("<POSITION>", str(int(position)), SubResult[0])
    SubResult = re.subn("<CUSTID>", str(int(custid)), SubResult[0])

    Msg="From: "+From+"\nTo: "+email+"\nSubject: Vinambar Passwort\n\n"
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


def genpasswd():                                                                               
    newpasswd=""                                                                                 
    chars = string.letters + string.digits
    for i in range(8):                                                                           
	newpasswd = newpasswd + choice(chars)                                                      
    return newpasswd                                                                             


Form = cgi.FieldStorage()

email=GetKey(Form, "email")
firstname=GetKey(Form, "firstname")
lastname=GetKey(Form, "lastname")
info=GetKey(Form, "info")
address=GetKey(Form, "address")
city=GetKey(Form, "city")
country=GetKey(Form, "country")
phone=GetKey(Form, "phone")
race=GetKey(Form, "race")

if (lastname==None) or (race==None) or (firstname==None) or (address==None) or (city==None):
    output="<p>Um Dich zu Eressea anzumelden musst Du das Formular vollständig ausfüllen.\n "
    for key in Form.keys():
	output=output+"<br>"+str(key)+"="+str(Form[key])
    Display(output)
else:
    db=MySQLdb.connect(db=dbname)
    cursor=db.cursor()
    exist=cursor.execute("select id from users where email='"+email+"' and (status='WAITING' or status='PENDING')")
    if exist>0:
	Display('<p>Du stehst bereits auf der Warteliste')
    else:
	password=genpasswd()
	fields = "firstname, lastname, email, address, city, status, password"
	values = "'"+firstname+"', '"+lastname+"', '"+email+"', '"+address+"', '"+city+"', 'WAITING', '"+password+"'"
	if phone!=None:
	    fields=fields+", phone"
	    values=values+", '"+phone+"'"
	if info!=None:
	    fields=fields+", info"
	    values=values+", '"+info+"'"
	if country!=None:
	    fields=fields+", country"
	    values=values+", "+country+""
	cursor.execute("insert into users ("+fields+") VALUES ("+values+")")
	cursor.execute("SELECT LAST_INSERT_ID() from dual")
	custid=cursor.fetchone()[0]
	cursor.execute("insert into subscriptions (user, race, game, status) VALUES ("+str(int(custid))+", '"+race+"', 0, 'PENDING')")
	cursor.execute("select count(*) from users")
	Send(email, custid, firstname, password, cursor.fetchone()[0])
	Display("<p>Deine Anmeldung wurde bearbeitet. Eine EMail mit Hinweisen ist unterwegs zu Dir.")
    db.close()
