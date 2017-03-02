#!/usr/bin/env python
# -*- coding: iso-8859-1 -*-

from email.Utils import parseaddr
from email.Parser import Parser
import os
import os.path
import ConfigParser
from re import compile, IGNORECASE
from stat import ST_MTIME
from string import upper, split, replace
import logging
import sys
from sys import stdin
from time import ctime, sleep, time
from socket import gethostname
from rfc822 import parsedate_tz, mktime_tz

if 'ERESSEA' in os.environ:
    dir = os.environ['ERESSEA']
elif 'HOME' in os.environ:
    dir = os.path.join(os.environ['HOME'], '/eressea')
else: # WTF? No HOME?
    dir = "/home/eressea/eressea"
if not os.path.isdir(dir):
    print "please set the ERESSEA environment variable to the install path"
    sys.exit(1)
rootdir = dir

game = int(sys.argv[1])
gamedir = os.path.join(rootdir, "game-%d" % (game, ))
frommail = 'eressea-server@kn-bremen.de'
gamename = 'Eressea'
sender = '%s Server <%s>' % (gamename, frommail)

inifile = os.path.join(gamedir, 'eressea.ini')
if not os.path.exists(inifile):
    print "no such file: " . inifile
else:
    config = ConfigParser.ConfigParser()
    config.read(inifile)
    if config.has_option('game', 'email'):
        frommail = config.get('game', 'email')
    if config.has_option('game', 'name'):
        gamename = config.get('game', 'name')
    if config.has_option('game', 'sender'):
        sender = config.get('game', 'sender')
    else:
        sender = "%s Server <%s>" % (gamename, frommail)
    config = None
prefix = 'turn-'
hostname = gethostname()
orderbase = "orders.dir"
sendmail = True
# maximum number of reports per sender:
maxfiles = 20
# write headers to file?
writeheaders = True
# reject all html email?
rejecthtml = True

def unlock_file(filename):
    try:
        os.unlink(filename+".lock")
    except:
        print "could not unlock %s.lock, file not found" % filename

def lock_file(filename):
    i = 0
    wait = 1
    if not os.path.exists(filename):
        file=open(filename, "w")
        file.close()
    while True:
        try:
            os.symlink(filename, filename+".lock")
            return
        except:
            i = i+1
            if i == 5: unlock_file(filename)
            sleep(wait)
            wait = wait*2

messages = {
        "multipart-en" :
                "ERROR: The orders you sent contain no plaintext. " \
                "The Eressea server cannot process orders containing HTML " \
                "or invalid attachments, which are the reasons why this " \
                "usually happens. Please change the settings of your mail " \
                "software and re-send the orders.",

        "multipart-de" :
                "FEHLER: Die von dir eingeschickte Mail enth�lt keinen " \
                "Text. Evtl. hast Du den Zug als HTML oder als anderweitig " \
                "ung�ltig formatierte Mail ingeschickt. Wir k�nnen ihn " \
                "deshalb nicht ber�cksichtigen. Schicke den Zug nochmals " \
                "als reinen Text ohne Formatierungen ein.",

        "maildate-de":
                "Es erreichte uns bereits ein Zug mit einem sp�teren " \
                "Absendedatum (%s > %s). Entweder ist deine " \
                "Systemzeit verstellt, oder ein Zug hat einen anderen Zug von " \
                "dir auf dem Transportweg �berholt. Entscheidend f�r die " \
                "Auswertungsreihenfolge ist das Absendedatum, d.h. der Date:-Header " \
                "deiner Mail.",

        "maildate-en":
                "The server already received an order file that was sent at a later " \
                "date (%s > %s). Either your system clock is wrong, or two messages have " \
                "overtaken each other on the way to the server. The order of " \
                "execution on the server is always according to the Date: header in " \
                "your mail.",

        "nodate-en":
                "Your message did not contain a valid Date: header in accordance with RFC2822.",

        "nodate-de":
                "Deine Nachricht enthielt keinen gueltigen Date: header nach RFC2822.",

        "error-de":
                "Fehler",

        "error-en":
                "Error",

        "warning-de":
                "Warnung",

        "warning-en":
                "Warning",

        "subject-de":
                "Befehle angekommen",

        "subject-en":
                "orders received"
}

# return 1 if addr is a valid email address
def valid_email(addr):
    rfc822_specials = '/()<>@,;:\\"[]'
    # First we validate the name portion (name@domain)
    c = 0
    while c < len(addr):
        if addr[c] == '"' and (not c or addr[c - 1] == '.' or addr[c - 1] == '"'):
            c = c + 1
            while c < len(addr):
                if addr[c] == '"': break
                if addr[c] == '\\' and addr[c + 1] == ' ':
                    c = c + 2
                    continue
                if ord(addr[c]) < 32 or ord(addr[c]) >= 127: return 0
                c = c + 1
            else: return 0
            if addr[c] == '@': break
            if addr[c] != '.': return 0
            c = c + 1
            continue
        if addr[c] == '@': break
        if ord(addr[c]) <= 32 or ord(addr[c]) >= 127: return 0
        if addr[c] in rfc822_specials: return 0
        c = c + 1
    if not c or addr[c - 1] == '.': return 0

    # Next we validate the domain portion (name@domain)
    domain = c = c + 1
    if domain >= len(addr): return 0
    count = 0
    while c < len(addr):
        if addr[c] == '.':
            if c == domain or addr[c - 1] == '.': return 0
            count = count + 1
        if ord(addr[c]) <= 32 or ord(addr[c]) >= 127: return 0
        if addr[c] in rfc822_specials: return 0
        c = c + 1
    return count >= 1

# return the replyto or from address in the header
def get_sender(header):
    replyto = header.get("Reply-To")
    if replyto is None:
        replyto = header.get("From")
        if replyto is None: return None
    x = parseaddr(replyto)
    return x[1]

# return first available filename basename,[0-9]+
def available_file(dirname, basename):
    ver = 0
    maxdate = 0
    filename = "%s/%s,%s,%d" % (dirname, basename, hostname, ver)
    while os.path.exists(filename):
        maxdate = max(os.stat(filename)[ST_MTIME], maxdate)
        ver = ver + 1
        filename = "%s/%s,%s,%d" % (dirname, basename, hostname, ver)
    if ver >= maxfiles:
        return None, None
    return maxdate, filename

def formatpar(string, l=76, indent=2):
    words = split(string)
    res = ""
    ll = 0
    first = 1

    for word in words:
        if first == 1:
            res = word
            first = 0
            ll = len(word)
        else:
            if ll + len(word) > l:
                res = res + "\n"+" "*indent+word
                ll = len(word) + indent
            else:
                res = res+" "+word
                ll = ll + len(word) + 1

    return res+"\n"

def store_message(message, filename):
    outfile = open(filename, "w")
    outfile.write(message.as_string())
    outfile.close()
    return

def write_part(outfile, part):
    charset = part.get_content_charset()
    payload = part.get_payload(decode=True)

    if charset is None:
        charset = "latin1"
    try:
        msg = payload.decode(charset, "ignore")
    except:
        msg = payload
        charset = None
    try:
        utf8 = msg.encode("utf-8", "ignore")
        outfile.write(utf8)
    except:
        outfile.write(msg)
        return False
    outfile.write("\n");
    return True

def copy_orders(message, filename, sender):
        # print the header first
    if writeheaders:
        from os.path import split
        dirname, basename = split(filename)
        dirname = dirname + '/headers'
        if not os.path.exists(dirname): os.mkdir(dirname)
        outfile = open(dirname + '/' + basename, "w")
        for name, value in message.items():
            outfile.write(name + ": " + value + "\n")
        outfile.close()

    found = False
    outfile = open(filename, "w")
    if message.is_multipart():
        for part in message.get_payload():
            content_type = part.get_content_type()
            logger.debug("found content type %s for %s" % (content_type, sender))
            if content_type=="text/plain":
                if write_part(outfile, part):
                    found = True
                else:
                    charset = part.get_content_charset()
                    logger.error("could not write text/plain part (charset=%s) for %s" % (charset, sender))

    else:
        if write_part(outfile, message):
            found = True
        else:
            charset = message.get_content_charset()
            logger.error("could not write text/plain message (charset=%s) for %s" % (charset, sender))
    outfile.close()
    return found

# create a file, containing:
# game=0 locale=de file=/path/to/filename email=rcpt@domain.to
def accept(game, locale, stream, extend=None):
    global rootdir, orderbase, gamedir, gamename, sender
    if extend is not None:
        orderbase = orderbase + ".pre-" + extend
    savedir = os.path.join(gamedir, orderbase)
    # check if it's one of the pre-sent orders.
    # create the save-directories if they don't exist
    if not os.path.exists(gamedir): os.mkdir(gamedir)
    if not os.path.exists(savedir): os.mkdir(savedir)
    # parse message
    message = Parser().parse(stream)
    email = get_sender(message)
    logger = logging.getLogger(email)
    # write syslog
    if email is None or valid_email(email)==0:
        logger.warning("invalid email address: " + str(email))
        return -1
    logger.info("received orders from " + email)
    # get an available filename
    lock_file(gamedir + "/orders.queue")
    maxdate, filename = available_file(savedir, prefix + email)
    if filename is None:
        logger.warning("more than " + str(maxfiles) + " orders from " + email)
        return -1
    # copy the orders to the file
    text_ok = copy_orders(message, filename, email)
    unlock_file(gamedir + "/orders.queue")

    warning, msg, fail = None, "", False
    maildate = message.get("Date")
    if maildate != None:
        turndate = mktime_tz(parsedate_tz(maildate))
        os.utime(filename, (turndate, turndate))
        logger.debug("mail date is '%s' (%d)" % (maildate, turndate))
        if turndate < maxdate:
            logger.warning("inconsistent message date " + email)
            warning = " (" + messages["warning-" + locale] + ")"
            msg = msg + formatpar(messages["maildate-" + locale] % (ctime(maxdate),ctime(turndate)), 76, 2) + "\n"
    else:
        logger.warning("missing message date " + email)
        warning = " (" + messages["warning-" + locale] + ")"
        msg = msg + formatpar(messages["nodate-" + locale], 76, 2) + "\n"

    if not text_ok:
        warning = " (" + messages["error-" + locale] + ")"
        msg = msg + formatpar(messages["multipart-" + locale], 76, 2) + "\n"
        logger.warning("rejected - no text/plain in orders from " + email)
        os.unlink(filename)
        savedir = savedir + "/rejected"
        if not os.path.exists(savedir): os.mkdir(savedir)
        lock_file(gamedir + "/orders.queue")
        maxdate, filename = available_file(savedir, prefix + email)
        store_message(message, filename)
        unlock_file(gamedir + "/orders.queue")
        fail = True

    if sendmail and warning is not None:
        subject = gamename + " " + messages["subject-"+locale] + warning
        mail = "Subject: %s\nFrom: %s\nTo: %s\n\n" % (subject, sender, email) + msg
        from smtplib import SMTP
        server = SMTP("localhost")
        server.sendmail(sender, email, mail)
        server.close()

    if not sendmail:
        print text_ok, fail, email
        print filename

    if not fail:
        lock_file(gamedir + "/orders.queue")
        queue = open(gamedir + "/orders.queue", "a")
        queue.write("email=%s file=%s locale=%s game=%s\n" % (email, filename, locale, game))
        queue.close()
        unlock_file(gamedir + "/orders.queue")

    logger.info("done - accepted orders from " + email)

    return 0

# the main body of the script:
try:
    os.mkdir(os.path.join(rootdir, 'log'))
except:
    pass # already exists?
LOG_FILENAME=os.path.join(rootdir, 'log/orders.log')
logging.basicConfig(level=logging.DEBUG, filename=LOG_FILENAME)
logger = logging
delay=None # TODO: parse the turn delay
locale = sys.argv[2]
infile = stdin
if len(sys.argv)>3:
    infile = open(sys.argv[3], "r")
retval = accept(game, locale, infile, delay)
if infile!=stdin:
    infile.close()
sys.exit(retval)
