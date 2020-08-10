#!/usr/bin/env python
# -*- coding: utf-8 -*-

from os import unlink, symlink, rename, popen, tmpfile
import sys
import os
import os.path
import ConfigParser
import subprocess
from re import compile, IGNORECASE
from string import split, join, upper, strip
from sys import argv, exit
from time import sleep, time, ctime
from syslog import openlog, closelog, syslog

from epasswd import EPasswd

def pwd_get_email(faction, pwd, pwdfile=None):
    return None

def split_filename(filename):
    return os.path.split(filename)


messages = {
"subject-de": "Befehle angekommen",
"subject-en": "orders received",

"validate-en": "Validating",
"validate-de": "Verarbeite",

"noorders-en": "The email contained no recognizable orders.",
"noorders-de": "Es konnten keine Befehle gefunden werden.",

"faction-en": "Faction",
"faction-de": "Partei",

"unknown-de": "WARNUNG: Die Partei ist nicht bekannt, oder das Passwort falsch!",
"unknown-en": "WARNING: This faction is unknown, or the password is incorrect!",

"warning-de": "Warnung",
"warning-en": "Warning",

"error-de": "Fehler",
"error-en": "Error",
}

game = int(sys.argv[1])
echeck_cmd = "/home/eressea/echeck/echeck.sh"
maxlines = 25

# base directory for all your games:
install_dir = "/home/eressea/eressea"
if 'ERESSEA' in os.environ:
    install_dir = os.environ['ERESSEA']
elif 'HOME' in os.environ:
    install_dir = os.path.join(os.environ['HOME'], 'eressea')
if not os.path.isdir(install_dir):
    print "please set the ERESSEA environment variable to the install path"
    sys.exit(1)

game_dir = os.path.join(install_dir, "game-%d" % (game, ))
gamename = 'Eressea'

inifile = os.path.join(game_dir, 'eressea.ini')
queue_file = os.path.join(game_dir, "orders.queue")
if not os.path.exists(queue_file):
    exit(0)

# regular expression that finds the start of a faction
fact_re = compile("^\s*(eressea|partei|faction)\s+([a-zA-Z0-9]+)\s+\"?([^\"]*)\"?", IGNORECASE)

def check_pwd(filename, email, pw_data):
    results = []
    try:
        file = open(filename, "r")
    except:
        print "could not open file", filename
        return results
    for line in file.readlines():
        mo = fact_re.search(strip(line))
        if mo != None:
            fact_nr = str(mo.group(2))
            fact_pw = str(mo.group(3))
            faction = pw_data.get_faction(fact_nr)

            if not faction:
                results.append((fact_nr, None, False, fact_pw))
            else:
                results.append((fact_nr, faction.email, faction.check_passwd(fact_pw), fact_pw))

    return results

def echeck(filename, locale, rules):
    dirname, filename = split_filename(filename)
    stream = popen("%s %s %s %s %s" % (echeck_cmd, locale, filename, dirname, rules), 'r')
    lines = stream.readlines()
    if len(lines)==0:
        stream.close()
        return None
    if len(lines)>maxlines:
        mail = join(lines[:maxlines-3] + ["...", "\n"] + lines[-3:], '')
    else:
        mail = join(lines[:maxlines], '')
    stream.close()
    return mail

#print "reading password file..."
pw_data = EPasswd.load_any(
    os.path.join(game_dir, "eressea.db"),
    os.path.join(game_dir, "passwd"),
)

#print "reading orders.queue..."
# move the queue file to a safe space while locking it:
queuefile = open(queue_file, "r")
lines = queuefile.readlines()
queuefile.close()

# copy to a temp file

tname="/tmp/orders.queue.%s" % str(time())
tmpfile=open(tname, "w")
for line in lines:
    tmpfile.write(line)
tmpfile.close()

openlog("orders")
unlink(queue_file)

for line in lines:
    tokens = split(line[:-1], ' ')
    dict = {}
    for token in tokens:
        name, value = split(token, '=')
        dict[name] = value

    email = dict["email"]
    locale = dict["locale"]
    game = int(dict["game"])
    infile = dict["file"]
    gamename='[E%d]' % game
    rules='e%d' % game
    warning = ""
    failed = True
    results = check_pwd(infile, email, pw_data)
    logfile = open(os.path.join(game_dir, "zug.log"), "a")
    dirname, filename = split_filename(infile)
    msg = messages["validate-"+locale] + " " + infile + "\n\n"
    if len(results)==0:
        msg = msg + messages["noorders-"+locale]
    for faction, game_email, success, pwd in results:
        msg = msg + messages["faction-"+locale] + " " + faction + "\n"
        if success: failed = False
        else: msg = msg + messages["unknown-"+locale] + "\n"
        msg = msg + "\n"
        logfile.write("%s:%s:%s:%s:%s\n" % (ctime(time()), email, game_email, faction, success))
    logfile.close()

    if failed:
        warning = " (" + messages["warning-" + locale] + ")"
        syslog("failed - no valid password in " + infile)
    else:
        result = None
        if os.path.exists(echeck_cmd):
            result = echeck(infile, locale, rules)
        if result is None:
            # echeck did not finish
            msg = msg + "Echeck is broken. Your turn was accepted, but could not be verified.\n"
            warning = " (" + messages["warning-" + locale] + ")"
            syslog("process - echeck broken, " + infile)
        else:
            msg = msg + result
            syslog("process - checked orders in " + infile)

    subject = gamename + " " + messages["subject-" + locale] + warning
    try:
        sp = subprocess.Popen(['mutt', '-s', subject, email], stdin=subprocess.PIPE)
        sp.communicate(msg)
    except:
        syslog("failed - cannot send to " + email)

closelog()
unlink(tname)
