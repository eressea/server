#!/usr/bin/env python

#
# reset the waiting list after ZAT.
# before running this script, be sure to apply the patchfile generated by the eressea-server to 
# mark users with new accounts as ACTIVE.
# * Will mark WAITING users as EXPIRED, removig them from the waitng list.
# * Will mark CONFIRMED users as WAITING, and send them another mail.

import MySQLdb
import re
import sys
import smtplib

dbname=sys.argv[1]
date=sys.argv[2]
db=MySQLdb.connect(db=dbname)
cursor=db.cursor()
From='accounts@eressea-pbem.de'
MailTemplate="templates/register.mail"
smtpserver='localhost'
server=smtplib.SMTP(smtpserver)
patchdir='/home/eressea/eressea-rsync'
game_id=0

def Patch():
    i = cursor.execute("select name, patch from games where id="+str(game_id))
    name, patch = cursor.fetchone()
    print "Auswertung f�r " + name + " Patch Level " + str(int(patch)) + ", Runde "+str(lastturn)
    while os.access(patchdir+'/patch-'+str(int(patch+1))+'.sql', os.F_OK):
	patch=patch+1
	print "  Patching to level "+str(patch)
	os.system('mysql ' + dbname + ' < ' + patchdir+'/patch-'+str(int(patch))+'.sql')
	cursor.execute('update games set patch='+str(int(patch))+' where id='+str(gid))
    return

def Send(email, custid, firstname, password, position, locale):
    TemplateHandle = open(MailTemplate+"."+locale, "r")  # open in read only mode
    # read the entire file as a string
    TemplateInput = TemplateHandle.read()
    TemplateHandle.close()                    # close the file

    SubResult = re.subn("<FIRSTNAME>", firstname, TemplateInput)
    SubResult = re.subn("<PASSWORD>", password, SubResult[0])
    SubResult = re.subn("<POSITION>", str(int(position)), SubResult[0])
    SubResult = re.subn("<CUSTID>", str(int(custid)), SubResult[0])

    Msg="From: "+From+"\nTo: "+email+"\nSubject: Eressea Anmeldung\n\n"
    Msg=Msg+SubResult[0]
    server.sendmail(From, email, Msg)
    return

Patch()
print "must update this. only partially done"
sys.exit(-1)

# remove all illegal and banned users:
users = cursor.execute("SELECT s.id from users u, subscriptions s where u.id=s.user and u.status in ('BANNED', 'ILLEGAL')")
while users:
    users=users-1
    sid = cursor.fetchone()[0]
    update.execute("UPDATE subscriptions set status='EXPIRED' WHERE id="+str(int(sid)))

# espire all users without confirmation. reset waiting list.
cursor.execute("update subscriptions set status='EXPIRED' where TO_DAYS(updated)<TO_DAYS('"+date+"') and status='WAITING'")
cursor.execute("update subscriptions set status='WAITING' where TO_DAYS(updated)<TO_DAYS('"+date+"') and status='CONFIRMED'")

# remind everyone who is left on the waiting list.
waiting = cursor.execute("select firstname, locale, email, u.id, s.password from users u, subscriptions s where u.id=s.users and s.status='WAITING'")
position=0
while waiting:
    waiting=waiting-1
    position=position+1
    firstname, locale, email, custid, password = cursor.fetchone()
    print "Sending reminder email to "+str(int(custid))+" "+email
    Send(email, custid, firstname, password, position, locale)

server.close()

