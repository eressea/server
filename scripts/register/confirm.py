#!/usr/bin/env python

import sys
import MySQLdb
import smtplib

dbname=sys.argv[1]
db = MySQLdb.connect(db=dbname)

From="accounts@vinyambar.de"
server=smtplib.SMTP('localhost')

cursor=db.cursor()
records=cursor.execute("SELECT users.id, users.password, users.email, "+
    "races.name, "+
    "games.name, games.info, subscriptions.id "+
    "from users, games, subscriptions, races "+
    "where users.id = subscriptions.user AND games.id = subscriptions.game "+
    "AND races.race = subscriptions.race AND races.locale='de' "+
    "AND subscriptions.status='NEW'")

while (records>0):
    row=cursor.fetchone()
    records = records - 1
    customerid=row[0]
    passwd=row[1]
    email=row[2]
    race=row[3]
    gamename=row[4]
    gameinfo=row[5]
    subscription=row[6]

    print "Sent confirmation to "+email+" for "+gamename+"."
    Msg = ("From: "+From+"\nTo: "+email+"\nSubject: Vinyambar Anmeldung angenommen.\n\n"+
        "Deine Anmeldung für '"+gamename+"' wurde akzeptiert.\n"
        "\n"+
        gameinfo +"\n"+
        "Kundennummer:   " + str(int(customerid)) + "\n"+
        "Auftragsnummer: " + str(int(subscription)) + "\n"+
        "Passwort:       " + passwd + "\n" +
        "Rasse:          " + race + "\n\n"+
        "Bitte bewahre diese Mail sorgfältig auf, da Du deine Kundennummerund das\n"+
        "Passwort für das Spiel benötigst. Solltest Du noch Fragen zu Deiner \n"+
        "Anmeldung haben, wende Dich bitte an accounts@vinyambar.de.\n\n"+
        "Das Vinyambar-Team")
    try:
        server.sendmail(From, email, Msg)
        update=db.cursor()
        update.execute("UPDATE subscriptions set status='CONFIRMED' WHERE id="+
            str(int(subscription)));
    except:
        print "Could not send Error to "+To
        print "Reason was: '"+Reason+"'"
        print "Exception is:", sys.exc_type, ":", sys.exc_value
