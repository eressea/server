#!/usr/bin/env python

import sys
import MySQLdb
import cgi
import re
import smtplib

# specify the filename of the template file
scripturl="http://eressea.upb.de/~enno/cgi-bin/info.py"
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

    for key in Form.keys():
       Content=Content+"<br>"+str(key)+"="+str(Form[key])

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

        output = "<div align=center>Letzte Aktualisierung: "+str(lastdate)[0:10]+"</div><form action=\""+scripturl+"\" method=post><div align=left><table width=80% border>\n"
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
            line = line + "<tr><th height=30>Kontostand</th><td>"+str(row[8])+" EUR</td></tr>\n"
            line = line + "<tr><th height=30>Status</th><td>"+row[9]+"</td></tr>\n"
            output = output + line;

        output=output+"</table></div>"

        query = ("select games.name, races.name, s.status, s.faction "+
          "from races, games, subscriptions s "+
          "where s.race=races.race and s.game=games.id "+
          "and s.user="+str(custid)+" ");

        results = cursor.execute(query);

        output=output+"<h3>Anmeldungen</h3>\n<div align=left><table width=80% border>\n"
        output=output+"<tr><th>Spiel</th><th>Rasse</th><th>Status</th><th>Partei</th><th>An-/Abmelden</th></tr>\n"
        while results>0:
            results = results - 1
            row = cursor.fetchone()
            line = "<tr>"
            line = line + '<td align="left">'+row[0]+'</td>\n'
            line = line + '<td align="left">'+row[1]+'</td>\n'
            line = line + '<td align="left">'+row[2]+'</td>\n'
            line = line + '<td align="left">'+row[3]+'</td>\n'
            line = line + '<td align="center">'
	    if row[2]=='ACTIVE':
		line = line + '<input type="checkbox" name="cancel_' + row[3] + '">'
	    if row[2]=='CANCELLED':
		line = line + '<input type="checkbox" name="activate_' + row[3] + '">'
	    line = line + '</td>\n'
            line = line + '</tr>\n'
            output=output+line

        output=output+"</table></div>"

	query="select date, balance, text from transactions, descriptions where descriptions.handle=transactions.description and user="+str(custid)+" ORDER BY date"
        results = cursor.execute(query);

        output=output+"<h3>Transaktionen</h3>\n<div align=left><table width=80% border>\n"
        output=output+"<tr><th>Datum</th><th>Betrag</th><th>Verwendung</th></tr>\n"
        while results>0:
            results = results - 1
            row = cursor.fetchone()
            line = "<tr>"
            line = line + "<td align=left>"+str(row[0])[0:10]+"</td>\n"
            line = line + "<td align=right>"+str(row[1])+" EUR</td>\n"
            line = line + "<td align=left>"+row[2]+"</td>\n"
            line = line + "</tr>\n"
            output=output+line

        output=output+"</table></div>"
        output=output+'<div align=left><input name="save" type="submit" value="Speichern"></div>'
        output=output+'<input type="hidden" name="user" value="'+str(custid)+'"></div>'
        output=output+'<input type="hidden" name="pass" value="'+Password+'"></div>'
        output=output+"</form>"
    else:
        output = "Die Kundennummer oder das angegebene Passwort sind nicht korrekt."
    db.close()
    Display(output, "Kundendaten #"+str(custid))

def Save(custid, Password):
    validkeys=['email','address','lastname','firstname','city','password','phone']
    values='id='+str(custid)
    for key in Form.keys():
        if key in validkeys:
            values=values+", "+key+"='"+Form[key].value+"'"
    db = MySQLdb.connect(db=dbname)
    cursor=db.cursor()
    cursor.execute('UPDATE users SET '+values+' where id='+str(custid))

    nfactions = cursor.execute("select g.name, s.id, faction from games g, subscriptions s where s.status='ACTIVE' and s.user="+str(custid) + " and s.game=g.id")
    while nfactions > 0:
        game, sid, faction = cursor.fetchone()
        if Form.has_key("cancel_"+faction):
            update = db.cursor()
	    update.execute("UPDATE subscriptions set status='CANCELLED' where id="+str(sid))
        nfactions = nfactions - 1

    nfactions = cursor.execute("select g.name, s.id, faction from games g, subscriptions s where s.status='CANCELLED' and s.user="+str(custid) + " and s.game=g.id")
    while nfactions > 0:
        game, sid, faction = cursor.fetchone()
        if Form.has_key("activate_"+faction):
            update = db.cursor()
	    update.execute("UPDATE subscriptions set status='ACTIVE' where id="+str(sid))
        nfactions = nfactions - 1

    db.close()
    ShowInfo(custid, Password)
#    Display("Noch nicht implementiert", "Daten speichern für Kunde #"+str(custid))

def SendPass(email):
    try:
        db = MySQLdb.connect(db=dbname)
        cursor=db.cursor()
#       print custid
        cursor.execute("select id, email, password from users where email='"+email+"'")
        custid, email, password = cursor.fetchone()
        Msg="From: "+From+"\nTo: "+email+"\nSubject: Vinambar Passwort\n\n"
        Msg=Msg+"Deine Kundennummer ist: "+str(int(custid))+"\n"
        Msg=Msg+"Dein Vinyambar-Passwort lautet: "+password+"\n"
        Msg=Msg+"\nDiese Mail wurde an Dich versandt, weil Du (oder jemand anders) \n"
        Msg=Msg+"es im Formular auf http://www.vinyambar.de/accounts.shtml angefordert hat.\n"
        server=smtplib.SMTP(smtpserver)
        server.sendmail(From, email, Msg)
        server.close()
        db.close()
        Display('<div align="center">Das Passwort wurde verschickt</div>', 'Kundendaten #'+str(custid))
    except:
        Display('<div align="center">Beim Versenden des Passwortes ist ein Fehler aufgetreten.<br>Eventuell ist die email-Adresse unbekannt</div>', 'Kundendaten für '+email)

Form = cgi.FieldStorage()

if Form.has_key("user"):
    custid = int(Form["user"].value)
else:
    custid = 0

if Form.has_key("pass"):
    Password = Form["pass"].value
else:
    Password=""

if Form.has_key("sendpass"):
    if Form.has_key("email"):
        Email = Form["email"].value
    else:
        Email=""
    SendPass(Email)
elif Form.has_key("save"):
    Save(custid, Password)
else:
    ShowInfo(custid, Password)
