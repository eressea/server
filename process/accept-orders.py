#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import os.path
import io
from configparser import ConfigParser
import codecs
import logging
import sys
import subprocess
import time
import socket
from stat import ST_MTIME
from email.utils import parseaddr, parsedate_tz, mktime_tz
from email.parser import Parser

if 'ERESSEA' in os.environ:
    dir = os.environ['ERESSEA']
elif 'HOME' in os.environ:
    dir = os.path.join(os.environ['HOME'], 'eressea')
else: # WTF? No HOME?
    dir = "/home/eressea/eressea"
if not os.path.isdir(dir):
    print("please set the ERESSEA environment variable to the install path")
    sys.exit(1)
rootdir = dir

game = int(sys.argv[1])
gamedir = os.path.join(rootdir, "game-%d" % (game, ))
frommail = 'eressea-server@kn-bremen.de'
gamename = 'Eressea'
sender = '%s Server <%s>' % (gamename, frommail)

inifile = os.path.join(gamedir, 'eressea.ini')
if not os.path.exists(inifile):
    print("no such file: " . inifile)
else:
    config = ConfigParser()
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
hostname = socket.gethostname()
orderbase = "orders.dir"
sendmail = True
# maximum number of reports per sender:
maxfiles = 50
# write headers to file?
writeheaders = True
# reject all html email?
rejecthtml = True

messages = {
        "multipart-en" :
                u"ERROR: The orders you sent contain no plaintext. " \
                u"The Eressea server cannot process orders containing HTML " \
                u"or invalid attachments, which are the reasons why this " \
                u"usually happens. Please change the settings of your mail " \
                u"software and re-send the orders.",

        "multipart-de" :
                u"FEHLER: Die von dir eingeschickte Mail enthält keinen " \
                u"Text. Evtl. hast Du den Zug als HTML oder als anderweitig " \
                u"ungültig formatierte Mail ingeschickt. Wir können ihn " \
                u"deshalb nicht berücksichtigen. Schicke den Zug nochmals " \
                u"als reinen Text ohne Formatierungen ein.",

        "maildate-de":
                u"Es erreichte uns bereits ein Zug mit einem späteren " \
                u"Absendedatum (%s > %s). Entweder ist deine " \
                u"Systemzeit verstellt, oder ein Zug hat einen anderen Zug von " \
                u"dir auf dem Transportweg überholt. Entscheidend für die " \
                u"Auswertungsreihenfolge ist das Absendedatum, d.h. der Date:-Header " \
                u"deiner Mail.",

        "maildate-en":
                u"The server already received an order file that was sent at a later " \
                u"date (%s > %s). Either your system clock is wrong, or two messages have " \
                u"overtaken each other on the way to the server. The order of " \
                u"execution on the server is always according to the Date: header in " \
                u"your mail.",

        "nodate-en":
                u"Your message did not contain a valid Date: header in accordance with RFC2822.",

        "nodate-de":
                u"Deine Nachricht enthielt keinen gueltigen Date: header nach RFC2822.",

        "error-de":
                u"Fehler",

        "error-en":
                u"Error",

        "warning-de":
                u"Warnung",

        "warning-en":
                u"Warning",

        "subject-de":
                u"Befehle angekommen",

        "subject-en":
                u"orders received"
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
    words = string.split()
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
                res = res + u"\n"+" "*indent+word
                ll = len(word) + indent
            else:
                res = res+" "+word
                ll = ll + len(word) + 1

    return res+"\n"

def store_message(message, filename):
    outfile = io.open(filename, "wb")
    outfile.write(message.as_bytes())
    outfile.close()
    return

def write_part(outfile, part, sender):
    content_type = part.get_content_type()
    logger.debug("found content type %s for %s" % (content_type, sender))
    if content_type != "text/plain":
        return False
    charset = part.get_content_charset()
    payload = part.get_payload(decode=True)
    if payload.startswith(codecs.BOM_UTF8):
        payload = payload[3:]

    if charset is None:
        charsets = [ 'utf8', 'latin1' ]
    else:
        charsets = [ charset, 'utf8', 'latin1']
    for cs in charsets:
        try:
            msg = payload.decode(cs)
            break
        except:
            logger.debug("could not decode %s mesage from %s" % (cs, sender))
    if not msg:
        msg = payload.decode('utf8', 'ignore')

    try:
        utf8 = msg.encode("utf8", "ignore")
        outfile.write(utf8)
    except:
        outfile.write(msg)
        return False
    outfile.write("\n".encode('ascii'));
    return True

def copy_orders(message, filename, sender, mtime):
    # print the header first
    dirname, basename = os.path.split(filename)
    if writeheaders:
        header_dir = dirname + '/headers'
        if not os.path.exists(header_dir): os.mkdir(header_dir)
        outfile = io.open(header_dir + '/' + basename, "wb")
        for name, value in message.items():
            outfile.write((name + ": " + value + "\n").encode('utf8', 'ignore'))
        outfile.close()

    found = False
    outfile = io.open(filename, "wb")
    if message.is_multipart():
        for part in message.get_payload():
            if write_part(outfile, part, sender):
                found = True
    else:
        if write_part(outfile, message, sender):
            found = True
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
    maxdate, filename = available_file(savedir, prefix + email)
    if filename is None:
        logger.warning("more than " + str(maxfiles) + " orders from " + email)
        return -1
    # copy the orders to the file
    
    turndate = None
    maildate = message.get("Date")
    if maildate is None:
        turndate = time.time()
    else:
        turndate = mktime_tz(parsedate_tz(maildate))

    text_ok = copy_orders(message, filename, email, turndate)

    warning, msg, fail = None, "", False
    if not maildate is None:
        os.utime(filename, (turndate, turndate))
        logger.debug("mail date is '%s' (%d)" % (maildate, turndate))
        if turndate < maxdate:
            logger.warning("inconsistent message date " + email)
            warning = " (" + messages["warning-" + locale] + ")"
            msg = msg + formatpar(messages["maildate-" + locale] % (time.ctime(maxdate), time.ctime(turndate)), 76, 2) + "\n"
    else:
        logger.warning("missing message date " + email)
        warning = " (" + messages["warning-" + locale] + ")"
        msg = msg + formatpar(messages["nodate-" + locale], 76, 2) + "\n"

    print('ACCEPT_MAIL=' + email)
    print('ACCEPT_FILE="' + filename + '"')

    if not text_ok:
        warning = " (" + messages["error-" + locale] + ")"
        msg = msg + formatpar(messages["multipart-" + locale], 76, 2) + "\n"
        logger.warning("rejected - no text/plain in orders from " + email)
        os.unlink(filename)
        savedir = savedir + "/rejected"
        if not os.path.exists(savedir): os.mkdir(savedir)
        maxdate, filename = available_file(savedir, prefix + email)
        if filename is None:
            logger.error("too many failed attempts")
        else:
            store_message(message, filename)
        fail = True

    if sendmail and warning is not None:
        logger.warning(warning)
        subject = gamename + " " + messages["subject-"+locale] + warning
        ps = subprocess.Popen(['mutt', '-s', subject, email], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        output = ps.communicate(msg.encode("utf8", "ignore"))
        if output[0] != '':
            logger.warning(output[0])

    if not sendmail:
        print(text_ok, fail, email)
        print(filename)

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
delay = None # TODO: parse the turn delay
locale = sys.argv[2]
infile = sys.stdin
if len(sys.argv)>3:
    infile = io.open(sys.argv[3], "rt")
retval = accept(game, locale, infile, delay)
if infile!=sys.stdin:
    infile.close()
sys.exit(retval)
