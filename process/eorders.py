#!/usr/bin/env python

import os, sys
import ConfigParser

from time import ctime, sleep, time

def get_eressea_dir():
    if 'ERESSEA' in os.environ:
        dir = os.environ['ERESSEA']
    elif 'HOME' in os.environ:
        dir = os.path.join(os.environ['HOME'], '/eressea')
    else: # WTF? No HOME?
        dir = "/home/eressea/eressea"
    if not os.path.isdir(dir):
        print "please set the ERESSEA environment variable to the install path"
        sys.exit(1)
    return dir

def split_filename(filename):
    return os.path.split(filename)

def unlock_file(filename):
    try:
        os.unlink(filename+".lock")
    except:
        print "could not unlock %s.lock, file not found" % filename

def lock_file(filename):
    i = 0
    wait = 1
    if not os.path.exists(filename):
        file=open(filename, "w")
        file.close()
    while True:
        try:
            os.symlink(filename, filename+".lock")
            return
        except:
            i = i+1
            if i == 5: unlock_file(filename)
            sleep(wait)
            wait = wait*2

def unlock_file(filename):
    try:
        os.unlink(filename+".lock")
    except:
        print "could not unlock %s.lock, file not found" % filename
        raise
    
def lock_file(filename):
    i = 0
    wait = 1
    if not os.path.exists(filename):
        file=open(filename, "w")
        file.close()
    while True:
        try:
            os.symlink(filename, filename+".lock")
            return
        except:
            i = i+1
            if i == 5:
                raise
            sleep(wait)
            wait = wait*2

class EConfig:

    def __init__(self, filename):
        self.config = ConfigParser.ConfigParser()
        self.config.read(filename)

  
    def get(self, section, key, abort=None, default=None):
        if self.config.has_option(section, key):
            return self.config.get(section, key)
        elif abort is not None:
            print "missing %s.%s" % (section, key)
            sys.exit(abort)
        return default

    
