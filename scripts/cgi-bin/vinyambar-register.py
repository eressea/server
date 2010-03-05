#!/usr/bin/env python

import sys
import MySQLdb
import os
import cgi
import re
import string
import smtplib
from whrandom import choice

# specify the filename of the template file
scripturl="http://eressea.upb.de/cgi-bin/eressea/vinyambar-register.py"
HTMLTemplate = "vinyambar.html"
MailTemplate="vinyambar-register.mail"
DefaultTitle = "Vinyambar Anmeldung"
dbname = "vinyambar"
From = "accounts@vinyambar.de"
locale="de"
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
    TemplateHandle = open(MailTemplate+"."+locale, "r")  # open in read only mode
    # read the entire file as a string
    TemplateInput = TemplateHandle.read()
    TemplateHandle.close()                    # close the file

    SubResult = re.subn("<FIRSTNAME>", firstname, TemplateInput)
    SubResult = re.subn("<PASSWORD>", password, SubResult[0])
    SubResult = re.subn("<POSITION>", str(int(position)), SubResult[0])
    SubResult = re.subn("<CUSTID>", str(int(custid)), SubResult[0])

    Msg="From: "+From+"\nTo: "+email+"\nSubject: Vinyambar Anmeldung\n\n"
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


Form = cgi.FieldStorage()

email=GetKey(Form, "email")
firstname=GetKey(Form, "firstname")
lastname=GetKey(Form, "lastname")
address=GetKey(Form, "address")
city=GetKey(Form, "city")
country=GetKey(Form, "country")
phone=GetKey(Form, "phone")

if (locale==None) or (lastname==None) or (firstname==None) or (address==None) or (city==None):
    output="<p>Um Dich zu Vinyambar anzumelden musst Du das Formular vollständig ausfüllen.\n "
    for key in Form.keys():
	output=output+"<br>"+key+": "+Form[key].value+"\n"
    Display(output)
elif ValidEmail(email)==0:
    output="<p>Um Dich zu Vinyambar anzumelden musst Du eine gültige Email-Adresse angeben.\n "
    Display(output)
else:
    db=MySQLdb.connect(db=dbname)
    cursor=db.cursor()
    exist=cursor.execute("select id from users where email='"+email+"'")
    if exist>0:
	Display('<p>Du hast bereits einen Eintrag in der Datenbank.')
    else:
	password=genpasswd()
	fields = "firstname, lastname, locale, email, address, city, status, password"
	values = "'"+firstname+"', '"+lastname+"', '"+locale+"', '"+email+"', '"+address+"', '"+city+"', 'WAITING', '"+password+"'"
	if phone!=None:
	    fields=fields+", phone"
	    values=values+", '"+phone+"'"
	if country!=None:
	    fields=fields+", country"
	    values=values+", "+country+""
	cursor.execute("insert into users ("+fields+") VALUES ("+values+")")
	cursor.execute("SELECT LAST_INSERT_ID() from dual")
	custid=cursor.fetchone()[0]
	if os.environ.has_key('REMOTE_ADDR'):
	    ip=os.environ['REMOTE_ADDR']
	if ip!=None:
	    cursor.execute("REPLACE userips (ip, user) VALUES ('"+ip+"', "+str(int(custid))+")")
	cursor.execute("select count(*) from users where status='WAITING' or status='CONFIRMED'")
	waiting=cursor.fetchone()[0]
	Send(email, custid, firstname, password, waiting)
	text={"de":"Deine Anmeldung wurde bearbeitet. Eine EMail mit Hinweisen ist unterwegs zu Dir", "en":"Your application was processed. An email containing further instructions is being sent to you"}
	Display("<p>"+text[locale]+".")
    db.close()
