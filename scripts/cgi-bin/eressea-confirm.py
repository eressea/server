#!/usr/bin/env python

import sys
import MySQLdb
import cgi
import os
import re

# specify the filename of the template file
scripturl="http://eressea.upb.de/~enno/cgi-bin/eressea-confirm.py"
HTMLTemplate = "eressea.html"
DefaultTitle = "Eressea Anmeldung"
dbname = "eressea"
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

custid=GetKey(Form, "custid")
password=GetKey(Form, "password")

if (password==None) or (custid==None):
    output="<p>Um Deine Anmeldung zu bestätigen musst Du das Formular vollständig ausfüllen.\n "
    for key in Form.keys():
	output=output+"<br>"+str(key)+"="+str(Form[key])
    Display(output)
else:
    db=MySQLdb.connect(db=dbname)
    cursor=db.cursor()
    exist=cursor.execute("select id from users where id="+custid+" and password='"+password+"'")
    if exist==0:
	Display('<p>Kundennummer oder Schlüssel falsch. Bitte beachte, dass Du beim Schlüssel auf Groß- und Kleinschreibung achten mußt.')
    else:
	if os.environ.has_key('REMOTE_ADDR'):
	    ip=os.environ['REMOTE_ADDR']
	    cursor.execute("REPLACE userips (ip, user) VALUES ('"+ip+"', "+str(int(custid))+")")
	cursor.execute("update users set status='CONFIRMED' where password='"+password+"' and id="+custid)
	Display("<p>Deine Anmeldung wurde bestätigt.");
    db.close()
