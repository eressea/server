#!/usr/bin/env python

import smtplib
import time
import sys
import os
import re
import locking
from locking import trylock, unlock

From="accounts@vinyambar.de"

# lock the input file:
if (trylock(sys.argv[1]+'.err')!=0):
  print "Could not lock "+sys.argv[1]+".err"
  sys.exit()

# move input file then unlock it:
if os.access(sys.argv[1]+'.err', os.F_OK)==0:
    unlock(sys.argv[1]+'.err')
    sys.exit();

try:
    os.rename(sys.argv[1]+'.err', sys.argv[1]+'.tmp')
finally:
    unlock(sys.argv[1]+'.err')

infile=open(sys.argv[1]+".tmp", "r")
server=smtplib.SMTP('localhost')
#server.set_debuglevel(1)

matchline=re.compile(
  r"""([^\s]+)\s*([^\n\r]+)*""",
  re.IGNORECASE | re.DOTALL | re.VERBOSE)

for line in infile.readlines():
    match=matchline.match(line)
    if match!=None:
        To = match.group(1)
        Reason = match.group(2)
	print "ERROR: "+To+": "+Reason
        Msg = ("From: "+From+"\nTo: "+To+"\nSubject: Vinyambar Anmeldung fehlgeschlagen.\n\n"
            +"Deine Anmeldung konnte aus folgendem Grund nicht akzeptiert werden:\n  "+Reason+"\n")
        try:
            server.sendmail(From, To, Msg)
        except:
            print "Could not send Error to "+To
            print "Reason was: '"+Reason+"'"
            print "Exception is:", sys.exc_type, ":", sys.exc_value

os.unlink(sys.argv[1]+".tmp")
infile.close()
server.quit()
