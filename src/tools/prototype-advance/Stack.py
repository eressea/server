#!/usr/bin/python

class Stack:
	def __init__(self):
		self.data = []

	def push(self, obj):
		self.data.append(obj)

	def pop(self):
		return self.data.pop()

	def empty(self):
		if self.data == []:
			return 1
		return 0

