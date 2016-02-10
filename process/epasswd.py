#!/usr/bin/python

from string import split
from string import strip
from string import lower

class EPasswd:
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
				[id, email, passwd, overri] = split(line, ":")[0:4]
				lc_id = lower(id)
				self.data[lc_id] = {}
				self.data[lc_id]["id"] = id
				self.data[lc_id]["email"] = email
				self.data[lc_id]["passwd"] = passwd
				self.data[lc_id]["overri"] = overri
			fp.close()

	def check(self, id, passwd):
		pw = self.get_passwd(id)
		if pw[0:6]=='$apr1$':
			# htpasswd hashes, cannot check, assume correct
			return 1
		if lower(pw) == lower(passwd):
			return 1
		if lower(self.get_overri(id)) == lower(passwd):
			return 1
		return 0

	def get_passwd(self, id):
		return self.data[lower(id)]["passwd"]
	
	def get_overri(self, id):
		return self.data[lower(id)]["overri"]

	def get_email(self, id):
		return self.data[lower(id)]["email"]
	
	def get_canon_id(self, id):
		return self.data[lower(id)]["id"]

	def fac_exists(self, id):
		if self.data.has_key(lower(id)):
			return 1
		return 0
