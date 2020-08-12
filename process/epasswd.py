#!/usr/bin/python

import bcrypt
import collections
import sqlite3


class _Faction(collections.namedtuple('Faction', 'no email passwd')):
    def check_passwd(self, passwd):
        if self.passwd.startswith(('$2a$', '$2y$')):
            try:
                return bcrypt.checkpw(passwd.encode('utf8'), self.passwd.encode('utf8'))
            except UnicodeEncodeError:
                return False

        return passwd == self.passwd


class EPasswd:
    def __init__(self, generator):
        self._data = {row[0]: _Faction(*row) for row in generator}

    @classmethod
    def load_database(cls, filename):
        conn = sqlite3.connect('file:' + filename + '?mode=ro', uri=True)
        try:
            cls(conn.execute('SELECT no, email, password FROM faction'))
        finally:
            conn.close()

    @classmethod
    def load_file(cls, filename):
        with open(filename, 'r') as f:
            return cls(
                line.strip().split(':')
                for line in f
                if line.strip() and not line.startswith('#')
            )

    @classmethod
    def load_any(cls, filename, passwd_filename=None, log=None):
        try:
            result = cls.load_database(filename)
            type_ = "db"
        except (sqlite3.OperationalError, sqlite3.DatabaseError):
            if passwd_filename is not None:
                filename = passwd_filename
            result = cls.load_file(filename)
            type_ = "file"

        if log is not None:
            log("loaded from " + type_ + " " + filename)

        return result

    def get_faction(self, no):
        return self._data.get(no.lower())
