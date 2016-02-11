#!/usr/bin/env python

import sys, re
from epasswd import EPasswd

if len(sys.argv)<4:
    sys.exit(-2)

passfile=sys.argv[1]
myfaction=sys.argv[2]
mypasswd=sys.argv[3]

if mypasswd[0]=='"':
    mypasswd=mypasswd[1:len(mypasswd)-1]

pw_data=EPasswd(passfile)
if pw_data.fac_exists(myfaction):
    if pw_data.check(myfaction, mypasswd):
        sys.exit(0)
sys.exit(-1)
