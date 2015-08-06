#!/usr/bin/env python

from sys import argv, exit
from string import join
from os import access, R_OK
from os import system

gamename='Eressea'

if(len(argv) >= 3):
  gamename=argv[2]

template="""#!/bin/bash
#PATH=$PATH:$HOME/bin

addr=%(email)s
[ $# -ge 1 ] && addr=$1
[ -z $addr ] || send-%(compression)s-report $addr '%(gamename)s Report #%(turn)s' %(files)s
"""

turn = argv[1]
try:
    infile = file("reports.txt", "r")
except:
    print "%s: reports.txt file does not exist" % (argv[0], )
    exit(0)
    
for line in infile.readlines():
    settings = line[:-1].split(":")
    options = { "turn" : turn}
    options["gamename"] = gamename
    for setting in settings:
        try:
            key, value = setting.split("=")
            options[key] = value
        except:
            print "Invalid input line", line
    if not options.has_key("reports"):
        continue
    reports = options["reports"].split(",")
    prefix = "%(turn)s-%(faction)s." % options
    files=[]
    times="../parteien"
    if os.path.isfile(times):
        files = files + [ times ]
    if options["compression"]=="zip":
        output = prefix+"zip"
        files = [output]
        if (access(output, R_OK)):
            pass
        else:
            parameters = []
            for extension in reports:
                filename = "%s%s" % (prefix, extension)
                if (access(filename, R_OK)):
                    parameters = parameters + [ filename ]
            system("zip %s -q -m -j -1 %s" % (output, join(parameters," ")))
    else:
        for extension in reports:
            if extension!='':
                filename = "%s%s" % (prefix, extension)
                output = "%s%s.bz2" % (prefix, extension)
                files = files+[output]
                if access(filename, R_OK):
                    if (access(output, R_OK)):
                        #print output, "exists, skipping"
                        continue
                    system("bzip2 %s" % filename)
    #print files
    options["files"] = join(files, " ")
    batch = file("%s.sh" % options["faction"], "w")
    batch.write(template % options)
    batch.close()
infile.close()
