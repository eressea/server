#!/usr/bin/python

from string import split
from string import strip
from string import lower
import subprocess

class EPasswd:
	def _check_apr1(self, pwhash, pw):
		spl = split(pwhash, '$')
		salt = spl[2]
		hash = subprocess.check_output(['openssl', 'passwd', '-apr1', '-salt', salt, pw]).decode('utf-8').strip()
		return hash==pwhash

	def __init__(self, file):
		self.data = {}
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
				lc_id = lower(id)
				self.data[lc_id] = {}
				self.data[lc_id]["id"] = id
				self.data[lc_id]["email"] = email
				self.data[lc_id]["passwd"] = passwd
			fp.close()

	def check(self, id, passwd):
		pw = self.get_passwd(id)
		if pw[0:6]=='$apr1$':
			return self._check_apr1(pw, passwd)
		return pw == passwd

	def get_passwd(self, id):
		return self.data[lower(id)]["passwd"]
	
	def get_email(self, id):
		return self.data[lower(id)]["email"]
	
	def get_canon_id(self, id):
		return self.data[lower(id)]["id"]

	def fac_exists(self, id):
		return self.data.has_key(lower(id))
