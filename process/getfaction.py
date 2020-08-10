#!/usr/bin/env python

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

pw_data = EPasswd.load_any(filename, log=log)
faction = pw_data.get_faction(myfaction)

if not faction:
    log("faction missing: " + myfaction)
    sys.exit(-1)

print(faction.email)
log("faction found: " + myfaction)
