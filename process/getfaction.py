#!/usr/bin/env python3

import sys, re
from epasswd import EPasswd

if len(sys.argv)<3:
    sys.exit(-2)

filename=sys.argv[1]
myfaction=sys.argv[2]
quiet=len(sys.argv)<=3

def log(str):
    if not quiet:
        print(str)

pw_data = EPasswd()
try:
    pw_data.load_database(filename)
    log("loaded from db " + filename)
except:
    pw_data.load_file(filename)
    log("loaded from file " + filename)

if pw_data.fac_exists(myfaction):
    print(pw_data.get_email(myfaction))
    log("faction found: " + myfaction)
    sys.exit(0)
else:
    log("faction missing: " + myfaction)

sys.exit(-1)
