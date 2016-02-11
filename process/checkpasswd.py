#!/usr/bin/env python

import sys, re

if len(sys.argv)<4:
    sys.exit(-2)

passfile=sys.argv[1]
myfaction=sys.argv[2]
mypasswd=sys.argv[3]

if mypasswd[0]=='"':
    mypasswd=mypasswd[1:len(mypasswd)-1]
try:
    infile = open(passfile, "r")
except:
    print "failed to open " + passfile
    sys.exit(-1)

for line in infile.readlines():
    match = line.split(":")
    if match!=None:
        faction, passwd, override = match[0:3]
        if faction==myfaction:
            if passwd==mypasswd or override==mypasswd:
                sys.exit(0)
sys.exit(-1)

