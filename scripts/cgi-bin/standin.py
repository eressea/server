#!/usr/bin/env python

import smtplib
import MySQLdb
import cgi
import re

# specify the filename of the template file
scripturl='http://eressea.upb.de/~enno/cgi-bin/standin.py'
TemplateFile = "vinyambar.html"
DefaultTitle = "Vinyambar Parteibörse"
dbname = "vinyambar"
From = "accounts@vinyambar.de"
Errors = ""
db = MySQLdb.connect(db=dbname)
smtpserver='localhost'

# define a new function called Display
# it takes one parameter - a string to Display
def Display(Content, Title=DefaultTitle):
    TemplateHandle = open(TemplateFile, "r")  # open in read only mode
    # read the entire file as a string
    TemplateInput = TemplateHandle.read()
    TemplateHandle.close()                    # close the file

#    for key in Form.keys():
#       Content=Content+"<br>"+str(key)+"="+str(Form[key])

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

def ShowPage():
    cursor=db.cursor()
    maxturn = {}
    games = cursor.execute("SELECT max(lastturn), game from subscriptions GROUP by game")
    while games>0:
	games=games-1
	lastturn, game = cursor.fetchone()
	maxturn[game] = lastturn
    output='<p>Um eine der folgenden Parteien zu übernehmen, musst du zuerst ein <a href="register.php">Spielerkonto anlegen</a>. Wenn Du eines hast, markiere die Partei, die Du übernehmen willst, und trage Kundennummer und Kundenpasswort ein.'
    query = "SELECT g.id, g.name, s.faction, s.lastturn, s.id, r.name, s.info from games g, subscriptions s, races r where s.game=g.id and s.race=r.race and s.status='CANCELLED' order by s.lastturn DESC"
    results = cursor.execute(query)
    output=output+'<div align=center><form action="'+scripturl+'" method=post><table bgcolor="#e0e0e0" width="80%" border>\n'
    output=output+'<tr><th>Rasse</th><th>Spiel</th><th>NMRs</th><th>Informationen</th><th>Markieren</th></tr>'
    while results>0:
	results=results-1
	gid, game, faction, lastturn, sid, race, info = cursor.fetchone()
	if info==None:
	    info='Keine Informationen'
	output=output+'<tr><td>'+ race + '</td><td>'+ game + '</td><td>' + str(int(maxturn[gid]-lastturn)) + '</td><td>' + info + '</td>'
	output=output+'<td><input type="checkbox" name="accept_' + str(int(sid)) + '"> übernehmen</td></tr>\n'
    
    output=output+'</table>'
    output=output+'<p><table><tr><td>Kundennummer:</td><td><input name="user" size="4"></tr><tr><td>Passwort:</td><td><input name="pass" type="password" size="40"></td>'
    output=output+'<tr><td>'
    output=output+'<input name="save" type="submit" value="Abschicken">'
    output=output+'</td></tr></table>'
#    output=output+'<p>Aus technischen Gründen wird diese Seite erst am Dienstag abend wieder benutzbar sein.'

    output=output+'</form></div>'
    Display(output)
    return

Form = cgi.FieldStorage()

custid=None
password=None
if Form.has_key("user"):
    custid = int(Form["user"].value)

if Form.has_key("pass"):
    password = Form["pass"].value

if (password!=None) & (custid!=None):
    cursor=db.cursor()
    output=""
    if cursor.execute("select email, id from users where password='"+password+"' and id="+str(int(custid)))==1:
	email, custid = cursor.fetchone()
	c = cursor.execute("SELECT id, game, password, faction from subscriptions where status='CANCELLED'")
	while c>0:
	    c=c-1
	    sid, gid, newpass, faction = cursor.fetchone()
	    if Form.has_key("accept_"+str(int(sid))):
		update = db.cursor()
		update.execute("UPDATE subscriptions set user=" + str(int(custid)) + ", status='ACTIVE' where id=" + str(int(sid)))
		output=output+"Die Partei " + faction + " wurde Dir überschrieben. Eine Email mit dem Passwort und weiteren Hinweisen ist unterwegs zu Dir.<br>"
		Msg="From: "+From+"\nTo: "+email+"\nSubject: Vinambar Parteiuebernahme\n\n"
		Msg=Msg+"Das Passwort für deine neue Vinyambar-Partei "+faction+" lautet\n"
		Msg=Msg+"  "+newpass+"\n"
		Msg=Msg+"\nUm den Report der letzten Woche zu erhalten, schicke eine Mail mit dem Betreff\n"
		Msg=Msg+"VIN"+str(int(gid))+" REPORT "+faction+" \""+newpass+"\" an die Adresse "
		Msg=Msg+"vinyambar@eressea.amber.kn-bremen.de"

		server=smtplib.SMTP(smtpserver)
		server.sendmail(From, email, Msg)
		server.close()
    else:
	output="<font color=red>Fehler in Passwort oder Kundennummer<br></font>"
    Display(output)
else:
    ShowPage()

db.close()
