#!/usr/bin/python

import sys
import MySQLdb
import cgi
import re
import smtplib

# specify the filename of the template file
TemplateFile = "vinyambar.html"
DefaultTitle = "Vinyambar Datenbank"
dbname = "vinyambar"
From = "accounts@vinyambar.de"
smtpserver = 'localhost'

# define a new function called Display
# it takes one parameter - a string to Display
def Display(Content, Title=DefaultTitle):
    TemplateHandle = open(TemplateFile, "r")  # open in read only mode
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


def ShowInfo(custid, Password):
    db = MySQLdb.connect(db=dbname)
    cursor = db.cursor()
    cursor.execute("select max(date), max(id) from transactions")
    lastdate, id = cursor.fetchone()

    query=("select info, firstname, lastname, email, address, city, country, phone, sum(t.balance), status "+
      "from users u, transactions t "+
      "where u.id=t.user and u.id="+str(custid)+" and u.password='"+Password+"' "+
      "GROUP BY u.id")

    #print query
    results = cursor.execute(query);
    if results > 0:
	
	output = "<div align=center>Letzte Aktualisierung: "+str(lastdate)[0:10]+"</div><form action=\"update.cgi\" method=post><div align=left><table width=80% border>\n"
	while results>0:
	    results = results - 1
	    row = cursor.fetchone()
	    line = "<tr>"
	    line = line + "<tr><th height=30>Vorname</th><td><input size=40 name=firstname value=\""+row[1]+"\"></td></tr>\n"
	    line = line + "<tr><th height=30>Nachname</th><td><input size=40 name=lastname value=\""+row[2]+"\"></td></tr>\n"
	    line = line + "<tr><th height=30>EMail Adresse</th><td><input size=40 name=email value=\""+row[3]+"\"></td></tr>\n"
	    line = line + "<tr><th height=30>Adresse</th><td><input size=40 name=address value=\""+row[4]+"\"></td></tr>\n"
	    line = line + "<tr><th height=30>Wohnort</th><td><input size=40 name=city value=\""+row[5]+"\"></td></tr>\n"
	    line = line + "<tr><th height=30>Telefon</th><td><input size=40 name=phone value=\""+row[7]+"\"></td></tr>\n"
	    line = line + "<tr><th height=30>Kontostand</th><td>"+str(row[8])+" DM</td></tr>\n"
	    line = line + "<tr><th height=30>Status</th><td>"+row[9]+"</td></tr>\n"
	    output = output + line;

	output=output+"</table></div>"

	query = ("select games.name, races.name, subscriptions.status, subscriptions.faction "+
	  "from races, games, subscriptions "+
	  "where subscriptions.race=races.race and subscriptions.game=games.id "+
	  "and subscriptions.user="+str(custid)+" ");

	results = cursor.execute(query);

	output=output+"<h3>Anmeldungen</h3>\n<div align=left><table width=80% border>\n"
	output=output+"<tr><th>Spiel</th><th>Rasse</th><th>Status</th><th>Partei</th></tr>\n"
	while results>0:
	    results = results - 1
	    row = cursor.fetchone()
	    line = "<tr>"
	    line = line + "<td align=left>"+row[0]+"</td>\n"
	    line = line + "<td align=left>"+row[1]+"</td>\n"
	    line = line + "<td align=left>"+row[2]+"</td>\n"
	    line = line + "<td align=left>"+row[3]+"</td>\n"
	    line = line + "</tr>\n"
	    output=output+line

	output=output+"</table></div>"
	query="select date, balance, text from transactions, descriptions where descriptions.handle=transactions.description and user="+str(custid)
	
	results = cursor.execute(query);

	output=output+"<h3>Transaktionen</h3>\n<div align=left><table width=80% border>\n"
	output=output+"<tr><th>Datum</th><th>Betrag</th><th>Verwendung</th></tr>\n"
	while results>0:
	    results = results - 1
	    row = cursor.fetchone()
	    line = "<tr>"
	    line = line + "<td align=left>"+str(row[0])[0:10]+"</td>\n"
	    line = line + "<td align=left>"+str(row[1])+"</td>\n"
	    line = line + "<td align=left>"+row[2]+"</td>\n"
	    line = line + "</tr>\n"
	    output=output+line

	output=output+"</table></div>"
#	output=output+"<div align=left><input type=submit value=\"Speichern\"></div>"
	output=output+"</form>"
    else:
	output = "Die Kundennummer oder das angegebene Passwort sind nicht korrekt."
	
    Display(output, "Kundendaten #"+str(custid))

def SendPass(custid):
    try:
	db = MySQLdb.connect(db=dbname)
	cursor=db.cursor()
#	print custid
	cursor.execute('select email, password from users where id='+str(custid))
	email, password = cursor.fetchone()
	Msg="From: "+From+"\nTo: "+email+"\nSubject: Vinambar Passwort\n\nDein Vinyambar-Passwort lautet: "+password+"\n"
	Msg=Msg+"\nDiese Mail wurde an Dich versandt, weil Du (oder jemand anders) \n"
	Msg=Msg+"es im Formular auf http://www.vinyambar.de/accounts.shtml angefordert hat.\n"
	server=smtplib.SMTP(smtpserver)
	server.sendmail(From, email, Msg)
	server.close()
	Display('<div align="center">Das Passwort wurde verschickt</div>', 'Kundendaten #'+str(custid))
    except:
	Display('<div align="center">Beim Versenden des Passwortes ist ein Fehler aufgetreten</div>', 'Kundendaten #'+str(custid))

Form = cgi.FieldStorage()

if Form.has_key("user"):
    custid = int(Form["user"].value)
else:
    custid = 0
if Form.has_key("pass"):
    Password = Form["pass"].value
else:
    Password=""

if Password!="":
    ShowInfo(custid, Password)
else:
    SendPass(custid)
