#!/usr/bin/env python

import sys
import MySQLdb
import smtplib

dbname=sys.argv[1]
db = MySQLdb.connect(db=dbname)

From="accounts@vinyambar.de"
server=smtplib.SMTP('localhost')

cursor=db.cursor()
records=cursor.execute("SELECT u.id, u.password, u.email "+
    "from users u "+
    "where u.status='NEW'")

while records>0:
    records = records - 1
    customerid, passwd, email = cursor.fetchone()
    
    Msg = ("From: "+From+"\nTo: "+email+"\nSubject: Vinyambar Anmeldung angenommen.\n\n"+
        "Deine Anmeldung für Vinyambar wurde akzeptiert.\n"
        "\n"+
        "Kundennummer:   " + str(int(customerid)) + "\n"+
        "Passwort:       " + passwd + "\n" +
	"\n" +
        "Bitte bewahre diese Mail sorgfältig auf, da Du deine Kundennummer und das\n"+
        "Passwort für das Spiel benötigst. Solltest Du noch Fragen zu Deiner \n"+
        "Anmeldung haben, wende Dich bitte an accounts@vinyambar.de.\n" +
	"\n"+
	"Die Kundennummer gib bitte bei der Überweisung der Spielgebühren an. \n" +
	"Unsere Kontoinformationen lauten:\n" +
	"    Katja Zedel\n"+
	"    Kontonummer 1251 886 106 \n"+
	"    BLZ 500 502 01 (Frankfurter Sparkasse)\n" +
	"\n"+
	"Zugang zu deinem Konto erhältst Du mit dem Passowrt auf der Webseite\n"+
	"    http://www.vinyambar.de/accounts.shtml\n"+
	"\n"+
        "Das Vinyambar-Team")
    try:
        server.sendmail(From, email, Msg)
	print "Sent confirmation to "+email+"."
        update=db.cursor()
        update.execute("UPDATE users set status='CONFIRMED' WHERE id="+
            str(int(customerid)));
    except:
        print "Could not send Error to "+email
        print "Reason was: '"+Reason+"'"
        print "Exception is:", sys.exc_type, ":", sys.exc_value
	sys.exit()
