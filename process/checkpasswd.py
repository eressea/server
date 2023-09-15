#!/usr/bin/env python3

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

pw_data = EPasswd()
pw_data.load_database(filename)
log("loaded from db " + filename)
#except:
#    pw_data.load_file(filename)
#    log("loaded from file " + filename)

if pw_data.fac_exists(myfaction):
    if pw_data.check(myfaction, mypasswd):
        log("password match: " + myfaction)
        sys.exit(0)
    log("password mismatch: " + myfaction)
else:
    log("faction missing: " + myfaction)

sys.exit(-1)
