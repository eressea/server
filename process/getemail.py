#!/usr/bin/env python

import sys, re
from epasswd import EPasswd

if len(sys.argv)<3:
    sys.exit(-2)

filename=sys.argv[1]
myfaction=sys.argv[2]

pw_data = EPasswd()
try:
    pw_data.load_database(filename)
except:
    pw_data.load_file(filename)

if pw_data.fac_exists(myfaction):
    email = pw_data.get_email(myfaction)
    print(email)
    sys.exit(0)
sys.exit(-1)
