#!/usr/bin/python

from Stack import *

class Node:
	def __init__(self, obj):
		self.obj    = obj
		self.marker = 0

class Vertex:
	def __init__(self, node1, node2):
		self.nodes  = (node1, node2)
		self.marker = 0

class Graph:

	def __init__(self):
		self.nodes = []
		self.vertices = []

	def remove_node(self, node):
		self.nodes.remove(node)
		for v in self.vertices:
			if v.nodes[0] == node or v.nodes[1] == node:
				self.vertices.remove(v)

	def find_node(self, obj):
		for n in self.nodes:
			if n.obj == obj:
				return n

		return None

	def neighbours(self, node):
		n = []
		for v in self.vertices:
			if v.nodes[0] == node and v.nodes[1] not in n:
				n.append(v.nodes[1])
			elif v.nodes[1] == node and v.nodes[0] not in n:
				n.append(v.nodes[0])

		return n

	def reset_marker(self):
		for n in self.nodes:
			n.marker = 0
		for v in self.vertices:
			v.marker = 0

	def dsearch(self,node):
		reset_marker()
		dg = [node]
		s = Stack()
		node.marker = 1
		s.push(node)
		while not s.empty():
			n = s.pop()
			for nb in self.neighbours(n):
				if nb.marker == 0:
					nb.marker = 1
					dg.append(nb)
					s.push(nb)
		return dg

	def dgraphs(self):
		rl       = []
		nodelist = self.nodes[:]
		while nodelist != []:
			start = nodelist.pop()
			self.reset_marker()
			g = Graph()
			g.nodes.append(start)
			start.marker = 1
			s = Stack()
			s.push(start)
			while not s.empty():
				tnode = s.pop()
				for nb in self.neighbours(tnode):
					if nb.marker == 0:
						nb.marker = 1
						g.nodes.append(nb)
						nodelist.remove(nb)
						s.push(nb)

			for v in self.vertices:
				if v.nodes[0] in g.nodes:
					g.vertices.append(v)

			rl.append(g)

		return rl

# g = Graph()

# n1 = Node(1)
# n2 = Node(2)
# n3 = Node(3)
# n4 = Node(4)
# n5 = Node(5)
# n6 = Node(6)

# g.nodes.append(n1)
# g.nodes.append(n2)
# g.nodes.append(n3)
# g.nodes.append(n4)
# g.nodes.append(n5)
# g.nodes.append(n6)

# g.vertices.append(Vertex(n1,n2))
# g.vertices.append(Vertex(n1,n4))
# g.vertices.append(Vertex(n2,n6))

# for dg in g.dgraphs():
# 	for e in dg:
# 		print e.obj,
# 	print



