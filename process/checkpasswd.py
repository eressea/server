#!/usr/bin/env python

import sys, re
from epasswd import EPasswd

if len(sys.argv)<4:
    sys.exit(-2)

filename=sys.argv[1]
myfaction=sys.argv[2]
mypasswd=sys.argv[3]

if mypasswd[0] == '"':
    mypasswd = mypasswd.strip('"')

pw_data = EPasswd()
try:
    pw_data.load_database(filename)
except:
    pw_data.load_file(filename)

if pw_data.fac_exists(myfaction):
    if pw_data.check(myfaction, mypasswd):
        sys.exit(0)
sys.exit(-1)
