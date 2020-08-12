#!/usr/bin/env python

import sys, re
from epasswd import EPasswd

if len(sys.argv)<4:
    sys.exit(-2)

filename=sys.argv[1]
myfaction=sys.argv[2]
mypasswd=sys.argv[3]
quiet=len(sys.argv)<=4

def log(str):
    if not quiet:
        print(str)

if mypasswd[0] == '"':
    mypasswd = mypasswd.strip('"')

pw_data = EPasswd.load_any(filename, log=log)
faction = pw_data.get_faction(myfaction)

if not faction:
    log("faction missing: " + myfaction)
elif not faction.check_passwd(mypasswd):
    log("password mismatch: " + myfaction)
else:
    log("password match: " + myfaction)
    sys.exit(0)
sys.exit(-1)
