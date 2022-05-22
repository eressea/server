#!/usr/bin/env python

from sys import argv, exit
import os
import os.path

gamename='Eressea'

if(len(argv) >= 3):
  gamename=argv[2]

template="""#!/bin/bash
#PATH=$PATH:$HOME/bin

addr=%(email)s
[ $# -ge 1 ] && addr=$1
[ -z $addr ] || $ERESSEA/server/bin/send-%(compression)s-report $addr '%(gamename)s Report #%(turn)s' %(files)s
"""

turn = argv[1]
try:
    infile = file("reports.txt", "r")
except:
    print("%s: reports.txt file does not exist" % (argv[0], ))
    exit(0)

extras = []
stats = '../parteien'
if os.path.isfile(stats):
    extra = 'wochenbericht-%s.txt' % turn
    os.symlink(stats, extra)
    extras.append(extra)
express='../express-%s.txt' % turn
if os.path.isfile(express):
    extras.append(express)

for line in infile.readlines():
    settings = line[:-1].split(":")
    options = { "turn" : turn}
    options["gamename"] = gamename
    for setting in settings:
        try:
            key, value = setting.split("=")
            options[key] = value
        except:
            print("Invalid input line", line)
    if "reports" not in options:
        continue
    reports = options["reports"].split(",")
    prefix = "%(turn)s-%(faction)s." % options
    if options["compression"]=="zip":
        output = prefix+"zip"
        files = [output]
        if not os.path.isfile(output):
            parameters = []
            for extension in reports:
                filename = "%s%s" % (prefix, extension)
                if os.path.isfile(filename):
                    parameters = parameters + [ filename ]
            os.system("zip %s -q -m -j %s" % (output, parameters.join(" ")))
    else:
        files = []
        for extension in reports:
            if extension!='':
                filename = "%s%s" % (prefix, extension)
                output = "%s%s.bz2" % (prefix, extension)
                files = files+[output]
                if os.path.isfile(filename):
                    if os.path.isfile(output):
                        continue
                    os.system("bzip2 %s" % filename)
    for extra in extras:
        if os.path.isfile(extra):
            files = files + [extra]
    options["files"] = files.join(" ")
    batch = file("%s.sh" % options["faction"], "w")
    batch.write(template % options)
    batch.close()
infile.close()
