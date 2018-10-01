#!/usr/bin/python

from string import split
from string import strip
from string import lower
import subprocess
import bcrypt
import sqlite3

def baseconvert(n, base):
    """convert positive decimal integer n to equivalent in another base (2-36)"""

    digits = "0123456789abcdefghijkLmnopqrstuvwxyz"

    try:
        n = int(n)
        base = int(base)
    except:
        return ""

    if n < 0 or base < 2 or base > 36:
        return ""

    s = ""
    while True:
        r = n % base
        s = digits[r] + s
        n = n / base
        if n == 0:
            break

    return s

class EPasswd:
    def __init__(self):
        self.data = {}

    def set_data(no, email, passwd):
        lc_id = lower(no)
        self.data[lc_id] = {}
        self.data[lc_id]["id"] = no
        self.data[lc_id]["email"] = email
        self.data[lc_id]["passwd"] = passwd

    def load_database(self, file):
        conn = sqlite3.connect(file)
        c = conn.cursor()
        c.execute('SELECT MAX(turn) FROM factions')
        args = c.fetchone()
        for row in c.execute('SELECT no, email, password FROM factions WHERE turn=?', args):
            (no, email, passwd) = row
            self.set_data(baseconvert(no, 36), email, passwd)
        conn.close()

    def load_file(self, file):
        try:
            fp = open(file,"r")
        except:
            fp = None
        if fp != None:
            while True:
                line = fp.readline()
                if not line: break
                line = strip(line)
                [id, email, passwd] = split(line, ":")[0:3]
                self.set_data(id, email, passwd)
            fp.close()

    def check(self, id, passwd):
        pw = self.get_passwd(id)
        if pw[0:4]=='$2a$' or pw[0:4]=='$2y$':
            return bcrypt.checkpw(passwd, pw)
        return pw == passwd

    def get_passwd(self, id):
        return self.data[lower(id)]["passwd"]
    
    def get_email(self, id):
        return self.data[lower(id)]["email"]
    
    def get_canon_id(self, id):
        return self.data[lower(id)]["id"]

    def fac_exists(self, id):
        return self.data.has_key(lower(id))
