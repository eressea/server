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
Errors = ""

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


def SendTransfer(userid, factionid, game):
    db = MySQLdb.connect(db=dbname)
    cursor=db.cursor()
    cursor.execute("select email, firstname from users where id="+str(userid))
    email, firstname = cursor.fetchone()
    Msg="From: "+From+"\nTo: "+email+"\nSubject: Vinambar Passwort\n\n"
    Msg=Msg+"Hallo,  "+firstname+"\n"
    Msg=Msg+"Ein Spieler hat Dir seine Partei " + factionid + " im Spiel " + game + "\n"
    Msg=Msg+"übertragen. Um die Partei zu übernehmen, gehe bitte auf die Webseite \n"
    Msg=Msg+"http://www.vinyambar.de/accounts.shtml, und akzeptiere dort den Transfer.\n"
    server=smtplib.SMTP(smtpserver)
    server.sendmail(From, email, Msg)
    server.close()
    db.close()
    return


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

	
def ShowInfo(custid, Password):
    global Errors
    db = MySQLdb.connect(db=dbname)
    cursor = db.cursor()
    query=("select firstname, lastname, email, address, city, country, phone, status "+
      "from users "+
      "where id="+str(custid)+" and password='"+Password+"' ")

    results = cursor.execute(query)
    if results != 0:
	firstname, lastname, email, address, city, country, phone, status = cursor.fetchone()
	if status=='WAITING':
	    cursor.execute("update users set status='CONFIRMED' where id="+str(custid))
	cursor.execute("select max(date), max(id) from transactions")
	lastdate, id = cursor.fetchone()

	nraces = cursor.execute("select distinct race, name from races where locale='de'")
	races=[('', 'Keine Anmeldung')]
	while nraces>0:
	    nraces = nraces - 1
	    races.append(cursor.fetchone())

        output = '<div align=center>Letzter Buchungstag: '+str(lastdate)[0:10]+'</div><form action="'+scripturl+'" method=post><div align=center><table  bgcolor="#e0e0e0" width=80% border>\n'
	
	query = "SELECT sum(balance) from transactions where user="+str(custid)
	transactions = cursor.execute(query)
	balance = 0.00
	if transactions != 0:
	    balance = cursor.fetchone()[0]
	    if balance == None:
		balance=0.00

	line = "<font color=red>"+Errors+"</font><tr>"
	line = line + "<tr><th height=30>Vorname</th><td><input size=40 name=firstname value=\""+firstname+"\"></td></tr>\n"
	line = line + "<tr><th height=30>Nachname</th><td><input size=40 name=lastname value=\""+lastname+"\"></td></tr>\n"
	if email==None:
	    email=""
	line = line + "<tr><th height=30>EMail Adresse</th><td><input size=40 name=email value=\""+email+"\"></td></tr>\n"
	if address==None:
	    address=""
	line = line + "<tr><th height=30>Adresse</th><td><input size=40 name=address value=\""+address+"\"></td></tr>\n"
	if city==None:
	    city=""
	line = line + "<tr><th height=30>Wohnort</th><td><input size=40 name=city value=\""+city+"\"></td></tr>\n"
	if phone==None:
	    phone=""
	line = line + "<tr><th height=30>Telefon</th><td><input size=40 name=phone value=\""+phone+"\"></td></tr>\n"
	line = line + "<tr><th height=30>Kontostand</th><td>"+str(balance)+" EUR</td></tr>\n"
	line = line + "<tr><th height=30>Status</th><td>"+status+"</td></tr>\n"
	output = output + line;

        output=output+"</table></div>"

	output=output+"<div align=center><h3>Partien</h3>\n"
	games = cursor.execute("select id, name, status, info from games order by id")
	while games>0:
	    games=games-1
	    gid, game, status, info = cursor.fetchone()
	    cself = db.cursor();
	    sub = cself.execute("select s.race from subscriptions s, users u where game="+str(int(gid))+" and u.id=s.user and u.id="+str(int(custid)))
	    prev=""
	    if sub>0:
		prev=cself.fetchone()[0]
		if prev==None:
		    prev=""
	    line = '<table  bgcolor="#e0e0e0" width=80% border>\n<tr><th align=center><em>' + game + '</em>: ' + info + '</th></tr>'
	    if status=='WAITING':
		line = line+'<tr><td>'
		line = line + 'Ich möchte an diesem Spiel teilnehmen, und bevorzuge folgende Rasse:<br>\n'
		nraces=len(races)
		line = line + '<select name="race_'+str(int(gid))+'" size="1">'
		while nraces>0:
		    nraces=nraces-1
		    race=races[nraces]
		    if prev == race[0]:
			line = line + '<OPTION selected value="'+race[0]+'">'+race[1]+'\n'
		    else:
			line = line + '<OPTION value="'+race[0]+'">'+race[1]+'\n'
		line = line + '</select>'
		line = line+'</td></tr>'
	    elif status=='RUNNING':
		query = ("select games.name, races.name, s.status, s.faction "+
		  "from races, games, subscriptions s "+
		  "where s.race=races.race and s.game="+str(int(gid))+" and s.game=games.id "+
		  "and s.user="+str(custid)+" ")
	    
		fcursor = db.cursor()
		results = fcursor.execute(query)
		if results>0:
		    while results>0:
			results = results - 1
			game, race, status, faction = fcursor.fetchone()
			line = line + '<tr><td><em>Partei ' + faction + ', ' + race + ", " + status + "</em></td></tr>"
			line = line + "<tr><td>"
			if status=='ACTIVE':
			    line = line + 'Ich möchte diese Partei aufgeben: <input type="checkbox" name="cancel_' + faction + '"><br>\n'
			    line = line + 'Ich möchte die Partei an Spieler #<input size=4 name="transfer_' + faction + '"> übergeben.\n'
			elif status=='CANCELLED':
			    line = line + 'Reaktivieren: <input type="checkbox" name="activate_' + faction + '">\n'
			elif status=='TRANSFERED':
			    line = line + 'Transfer akzeptieren: <input type="checkbox" name="accept_' + faction + '">\n'
			line = line+'</td></tr>'
		else:
		    continue
	    else:
		continue
	    output=output+line+'</table>\n<p>\n'
	output=output+"</div>"
	
	query="select date, balance, text from transactions, descriptions where descriptions.handle=transactions.description and user="+str(custid)+" ORDER BY date"
        results = cursor.execute(query);

        if results>0:
	    output=output+'<div align=center>\n<h3>Transaktionen</h3>\n<table width=80% bgcolor="#e0e0e0" border>\n'
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
	    
        output=output+'<div align=center><p><input name="save" type="submit" value="Speichern"></div>'
        output=output+'<input type="hidden" name="user" value="'+str(custid)+'"></div>'
        output=output+'<input type="hidden" name="pass" value="'+Password+'"></div>'
        output=output+"</form>"
    else:
        output = "Die Kundennummer oder das angegebene Passwort sind nicht korrekt."
    db.close()
    Display(output, "Kundendaten #"+str(custid))
    Errors = ""

def TransferFaction(sid, faction, newuser, game):
    db = MySQLdb.connect(db=dbname)
    update = db.cursor()
    exist = update.execute("select id from users where id="+str(newuser))
    if exist==1:
	update.execute("UPDATE subscriptions set status='TRANSFERED', user=" + str(newuser) + " where id="+str(sid))
	SendTransfer(newuser, faction, game);
    db.close()
    return

def Save(custid, Password):
    validkeys=['email','address','lastname','firstname','city','password','phone']
    values='id='+str(custid)
    for key in Form.keys():
        if key in validkeys:
            values=values+", "+key+"='"+Form[key].value+"'"
    db = MySQLdb.connect(db=dbname)
    cursor=db.cursor()
    cursor.execute('UPDATE users SET '+values+' where id='+str(custid))

    ngames = cursor.execute("select id from games where status='WAITING'")
    while ngames > 0:
	ngames=ngames - 1
	gid = cursor.fetchone()[0]
	key="race_"+str(int(gid))
	update = db.cursor()
	if Form.has_key(key):
	    newrace=Form[key].value
	    if newrace=='':
		newrace=None
	    if newrace==None:
		update.execute('delete from subscriptions where user='+str(int(custid))+' and game='+str(int(gid)))
	    else:
		exist=update.execute('select id, race from subscriptions where game='+str(int(gid))+' and user='+str(int(custid)))
		if exist>0:
		    sid, race = update.fetchone()
		    if race!=newrace:
			update.execute("update subscriptions  set race='"+newrace+"' where id="+str(int(sid)))
		else:
		    update.execute("insert subscriptions (race, user, status, game) values ('"+newrace+"', "+str(int(custid))+", 'WAITING', "+str(int(gid))+")")
	else:
	    update.execute('delete from subscriptions where user='+str(int(custid))+' and game='+str(int(gid)))

    nfactions = cursor.execute("select g.name, s.id, faction from games g, subscriptions s where s.status='ACTIVE' and s.user="+str(custid) + " and s.game=g.id")
    while nfactions > 0:
        game, sid, faction = cursor.fetchone()
        if Form.has_key("cancel_"+faction):
            update = db.cursor()
	    update.execute("UPDATE subscriptions set status='CANCELLED' where id="+str(int(sid)))
	elif Form.has_key("transfer_"+faction):
	    newuser = int(Form["transfer_"+faction].value)
	    TransferFaction(sid, faction, newuser, game)
        nfactions = nfactions - 1

    nfactions = cursor.execute("select g.name, s.id, faction from games g, subscriptions s where s.status='TRANSFERED' and s.user="+str(custid) + " and s.game=g.id")
    while nfactions > 0:
        game, sid, faction = cursor.fetchone()
        if Form.has_key("accept_"+faction):
            update = db.cursor()
	    update.execute("UPDATE subscriptions set status='ACTIVE' where id="+str(int(sid)))
        nfactions = nfactions - 1

    nfactions = cursor.execute("select g.name, s.id, faction from games g, subscriptions s where s.status='CANCELLED' and s.user="+str(custid) + " and s.game=g.id")
    while nfactions > 0:
        game, sid, faction = cursor.fetchone()
        if Form.has_key("activate_"+faction):
            update = db.cursor()
	    update.execute("UPDATE subscriptions set status='ACTIVE' where id="+str(int(sid)))
        nfactions = nfactions - 1

    db.close()
    ShowInfo(custid, Password)
#    Display("Noch nicht implementiert", "Daten speichern für Kunde #"+str(custid))


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
