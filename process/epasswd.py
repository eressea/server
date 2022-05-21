#!/usr/bin/python

import bcrypt
import sqlite3

class EPasswd:
    def __init__(self):
        self.data = {}

    def set_data(self, no, email, passwd):
        lc_id = lower(no)
        self.data[lc_id] = {}
        self.data[lc_id]["id"] = no
        self.data[lc_id]["email"] = email
        self.data[lc_id]["passwd"] = passwd

    def load_database(self, file):
        conn = sqlite3.connect(file)
        c = conn.cursor()
        for row in c.execute('SELECT `no`, `email`, `password` FROM `faction`'):
            (no, email, passwd) = row
            self.set_data(no, email, passwd)
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
                line = line.strip()
                [id, email, passwd] = line.split(":")[0:3]
                self.set_data(id, email, passwd)
            fp.close()

    def check(self, id, passwd):
        pw = self.get_passwd(id)
        if pw[0:4]=='$2a$' or pw[0:4]=='$2y$':
            try:
                uhash = pw.encode('utf8')
                upass = passwd.encode('utf8')
                return bcrypt.checkpw(upass, uhash)
            except:
                return False
        return pw == passwd

    def get_passwd(self, id):
        return self.data[id.lower()]["passwd"]
    
    def get_email(self, id):
        return self.data[id.lower()]["email"]
    
    def get_canon_id(self, id):
        return self.data[id.lower()]["id"]

    def fac_exists(self, id):
        return self.data.has_key(id.lower())

