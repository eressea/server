#!/usr/bin/env python

import os
import stat

def trylock(file):
  try:
    os.symlink(file, file+'.lock')
  except OSError:
    return 1
  return 0

def lock(file):
  locked=1
  while locked:
    try:
      locked=0
      os.symlink(file, file+'.lock')
    except:
      update=os.stat(file+'.lock')[stat.ST_MTIME]
      now=time.time()
      if (now > update + 60):
        locked=0
        print "removing stale lockfile "+file+".lock"
        os.unlink(file+'.lock')
      else:
        locked=1
        print "Waiting for lock on "+file+".lock"
        time.sleep(20)

def unlock(file):
  os.unlink(file+'.lock')

