#!/usr/bin/env python

import sys
import MySQLdb
import cgi
import re

# specify the filename of the template file
TemplateFile = "vinyambar.html"
DefaultTitle = "Eressea Umfrage 2001"
dbname = "eressea"
myurl="http://eressea.upb.de/cgi-bin/eressea/poll2001.py"

# define a new function called Display
# it takes one parameter - a string to Display
print "Content-Type: text/html\n\n"
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

    print SubResult[0]

def ReadForm(Form):
    output=""
    
    if (Form.has_key('faction') & Form.has_key('password'))==0:
	Display('Partei oder Passwort nicht angegeben')
	sys.exit()
    faction=Form['faction'].value
    password=Form['password'].value
    db = MySQLdb.connect(db=dbname)
    cursor = db.cursor()
    num = cursor.execute('SELECT password from factions where id=\''+faction+'\' and password=\''+password+'\'')
    if num==0:
	Display('Partei oder Passwort ungültig')
	sys.exit()

    fields="faction"
    values="'"+faction+"'"
    booleans=['otherpbem', 'party','othertools','crtools','magellan','emap','ehmv','echeck','vorlage','esurvey','mercator', 'madmax', 'slow', 'fast']
    for field in booleans:
	if Form.has_key(field):
	    fields=fields+", "+field
	    values=values+", 1"
    ints=['age','country','turn','starts','socializer','killer','explorer','achiever']
    for field in ints:
	if Form.has_key(field):
	    fields=fields+", "+field
	    values=values+", "+Form[field].value
    strings=['referer','refererother','fun','toolsother', 'freetext']
    for field in strings:
	if Form.has_key(field):
	    fields=fields+", "+field
	    values=values+", '"+Form[field].value+"'"
    query="REPLACE poll ("+fields+") VALUES ("+values+")"
    # print query
    cursor.execute(query)

    Display('Danke für die Teilnahme an der Umfrage.\n<p><a href="'+myurl+'">Ergebnisse ansehen</a>\n')

def ShowResult():
    db = MySQLdb.connect(db=dbname)
    cursor = db.cursor()
    results=cursor.execute('SELECT factions.email, poll.freetext, poll.age, countries.name FROM factions, poll, countries WHERE factions.id=poll.faction AND countries.id=poll.country')
    answers=int(results)
    ages = {}
    countries = {}
    comments = "<h2>Kommentare</h2>\n"
    while results > 0:
	results=results-1
	email, freetext, age, country=cursor.fetchone()
	
	if freetext!=None:
	    name=""
#	    if email==None:
#		email="unknown"
#	    name = " ("+re.subn('@', " at ", email)[0]+")"
	    comments=comments+"<p>"+freetext+name+"\n"
	if age!=None:
	    if (ages.has_key(age)):
		ages[age]=ages[age]+1
	    else:
		ages[age]=1;
	if country!=None:
	    if (countries.has_key(country)):
		countries[country]=countries[country]+1
	    else:
		countries[country]=1;

    personaltext="<h2>Spielerprofil</h2>\n"
    personals={ 'age': 'Alter', 'turn':'Startrunde', 'starts':'Anzahl Starts'}
    for key in personals.keys():
	cursor.execute("SELECT AVG("+key+"), MIN("+key+"), MAX("+key+") FROM poll")
	kavg, kmin, kmax = cursor.fetchone()
	personaltext=personaltext+personals[key]+": "+str(int(kmin))+" bis "+str(int(kmax))+" (Durchschnitt "+str(kavg)+")<br>\n"
    cursor.execute("select count(*) from poll where party=1")
    value = cursor.fetchone()[0]
    personaltext=personaltext+str(int(value))+" der "+str(answers)+" Spieler haben schon einmal wegen Eressea eine Party frühzeitig verlassen."
    
    funtypes= { 
      'quitting':'Ich höre auf',
      'okay':'Geht so', 
      'greatfun':'Macht großen Spaß',
      'bestgame':'Bestes Spiel der Welt'
     }
    funtext ="<h2>Spielspaß</h2>\n<table>\n"
    for key in funtypes.keys():
	cursor.execute("SELECT COUNT(*) FROM poll WHERE fun='"+key+"'")
	value = int(cursor.fetchone()[0])
	funtext=funtext+"<tr><td>"+funtypes[key]+"</td><td>"+str(value)+"</td></tr>\n"
    funtext=funtext+"</table>\n"

    playertypes = { 'killer': 'Strategiespiel', 'explorer':'Erkundung', 'achiever':'Aufbau und Entwicklung', 'socializer':'Rollenspiel' }
    typetext ="<h2>Spielaspekte</h2>\n<table>\n"
    for key in playertypes.keys():
	cursor.execute("SELECT AVG("+key+") FROM poll")
	value = cursor.fetchone()[0]
	typetext=typetext+"<tr><td>"+playertypes[key]+"</td><td>"+str(value)+"</td></tr>\n"
    typetext=typetext+"</table>\n"

    toolnames=('magellan', 'crtools','ehmv', 'echeck', 'mercator', 'emap', 'esurvey', 'vorlage', 'othertools')
    tooltext="<h2>Tools</h2>\n<table>\n"	
    for tool in toolnames:
	results=cursor.execute('SELECT count(*) from poll where '+tool+'=1')
	if results==1:
	    num=int(cursor.fetchone()[0]);
	    tooltext=tooltext+"<tr><td>"+tool+'</td><td>'+str(num)+' ('+str(int(num*100.0/answers))+'%)</td></tr>\n'
    tooltext=tooltext+"</table><h3>...andere</h3>\n"
    num=cursor.execute("SELECT DISTINCT toolsother FROM poll where toolsother is not null order by toolsother")
    while num!=0:
	num=num-1
	text=str(cursor.fetchone()[0])
	tooltext=tooltext+text+"<br>\n"

    referernames= {'friends':'Freunde &amp; Familie','pbemdirect':'PBEM Webseiten','other':'Andere', 'otherpbem':'Andere PBEM', 'searchengi':'Suchmaschinen','press':'Presse','yahoo':'Yahoo &amp; Co,' }
    referertext="<h2>Wie bist Du zu Eressea gekommen?</h2>\n<table>\n"
    for referer in referernames.keys():
	results=cursor.execute("SELECT count(*) from poll where referer='"+referer+"'")
	if results==1:
	    num=int(cursor.fetchone()[0]);
	    referertext=referertext+"<tr><td>"+referernames[referer]+'</td><td>'+str(num)+"</td></tr>\n"
    referertext=referertext+"</table><h3>...andere</h3>\n"
    num=cursor.execute("SELECT DISTINCT refererother FROM poll where refererother is not null order by refererother")
    while num!=0:
	num=num-1
	text=str(cursor.fetchone()[0])
	referertext=referertext+text+"<br>\n"

    scenarionames={'fast':'Zweimal pro Woche','slow':'Alle zwei Wochen','madmax':'Endzeit-Eressea'}
    scenariotext="<h2>Kommerzielle Szenarios</h2>\n<table>\n"
    for scenario in scenarionames.keys():
	results=cursor.execute('SELECT count(*) from poll where '+scenario+'=1')
	if results==1:
	    scenariotext=scenariotext+"<tr><td>"+scenarionames[scenario]+'</td><td>'+str(int(cursor.fetchone()[0]))+'</td>\n'
    scenariotext=scenariotext+"</table>\n"
#    agefile=open('poll2001-age.dat', 'w')
#    countryfile=open('poll2001-country.dat', 'w')
#    for age in ages.keys():
#	agefile.write(str(int(age)) + '\t' + str(ages[age]) + '\n')
    countrytext = "<h2>Heimatländer</h2>\n<table>"
    for country in countries.keys():
#	countryfile.write(country + '\t' + str(countries[country]) + '\n')
	countrytext=countrytext+"<tr><td>"+country + '</td><td>' + str(countries[country]) + '</td></tr>\n'
    countrytext=countrytext+"</table>\n"
#    agefile.close()
#    countryfile.close()
    Display(personaltext+countrytext+typetext+funtext+scenariotext+referertext+tooltext+comments)

Form = cgi.FieldStorage()
if Form.has_key('faction'):
    ReadForm(Form)
else:
    ShowResult()

#+--------------+-------------+------+-----+---------+-------+
#| Field        | Type        | Null | Key | Default | Extra |
#+--------------+-------------+------+-----+---------+-------+
#| faction      | varchar(6)  | YES  |     | NULL    |       |
#| age          | int(11)     | YES  |     | NULL    |       |
#| country      | int(11)     | YES  |     | NULL    |       |
#| turn         | int(11)     | YES  |     | NULL    |       |
#| starts       | int(11)     | YES  |     | NULL    |       |
#| otherpbem    | tinyint(1)  |      |     | 0       |       |
#| party        | tinyint(1)  |      |     | 0       |       |
#| referer      | varchar(10) | YES  |     | NULL    |       |
#| refererother | varchar(64) | YES  |     | NULL    |       |
#| fun          | varchar(10) | YES  |     | NULL    |       |
#| socializer   | int(11)     | YES  |     | NULL    |       |
#| killer       | int(11)     | YES  |     | NULL    |       |
#| explorer     | int(11)     | YES  |     | NULL    |       |
#| achiever     | int(11)     | YES  |     | NULL    |       |
#| scenarios    | varchar(10) | YES  |     | NULL    |       |
#| magellan     | tinyint(1)  |      |     | 0       |       |
#| emap         | tinyint(1)  |      |     | 0       |       |
#| ehmv         | tinyint(1)  |      |     | 0       |       |
#| vorlage      | tinyint(1)  |      |     | 0       |       |
#| esurvey      | tinyint(1)  |      |     | 0       |       |
#| mercator     | tinyint(1)  |      |     | 0       |       |
#| echeck       | tinyint(1)  |      |     | 0       |       |
#| toolsother   | varchar(64) | YES  |     | NULL    |       |
#+--------------+-------------+------+-----+---------+-------+

