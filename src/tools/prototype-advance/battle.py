#!/usr/bin/python
# vi:set ts=4:

# Algorithmus zur Berechnung von regionsübergreifenden Schlachten.
# Testprogramm.

# 1) Graph aufbauen aus alle Attackierern und allen direkt attackierten

# 2) Helfer in den Graphen aufnehmen

# 3) Disjunkte Teilgraphen ermitteln

# 4) Innerhalb der Teilgraphen die 'Wer-schlägt-wen-Beziehung' aufbauen

import re
from Graph import *

class heer:
	def __init__(self, rx, ry, f, o):
		self.rx = rx
		self.ry = ry
		self.f  = f
		self.o  = o
		self.attacked = []

	def toString(self):
		return "("+self.f+","+str(self.rx)+","+str(self.ry)+")"


def dist(x1, y1, x2, y2):
	dx = x1 - x2
	dy = y1 - y2
	if dy < 0:
		dy = -dy
		dx = -dx
	if dx >= 0:
		return dx + dy;
	if -dx >= dy:
		return -dx
	return dy


def is_neighbour(h, r1, r2):
	x1 = r1[0]
	y1 = r1[1]
	x2 = r2[0]
	y2 = r2[1]

	if dist(h.rx, h.ry, x1, y1) <= 1 and dist(h.rx, h.ry, x2, y2) <= 1:
		return 1

	return 0

heere = [
	Node(heer(0,-2, 'D', 'A->C:1,-3')),
	Node(heer(1,-3, 'D', '')),
	Node(heer(1,-2, 'D', '')),
	Node(heer(2,-3, 'D', 'A->C:1,-3')),
	Node(heer(1,-2, 'C', 'L->D')),
	Node(heer(1,-3, 'C', 'A->D:1,-2')),
	Node(heer(0, 0, 'A', '')),
	Node(heer(0, 0, 'B', 'L->A'))
]

def find_heer_node(x, y, f):
	for node in heere:
		h = node.obj
		if h.rx == x and h.ry == y and h.f == f:
			return node
	return None

def allied(f1, f2):
	if f1 == f2:
		return 1
	return 0

g1 = Graph()

a_re = re.compile('A->(\w):(-?\d+),(-?\d+)')
l_re = re.compile('L->(\w)')

# Schritt 1

for node in heere:
	f = ''
	h = node.obj

	mo = a_re.match(h.o)
	if mo != None:
		f = mo.group(1)
		x = int(mo.group(2))
		y = int(mo.group(3))
		
	mo = l_re.match(h.o)
	if mo != None:
		f = mo.group(1)
		x = h.rx
		y = h.ry

	if f != '':
		tnode = find_heer_node(x,y,f)
		if node not in g1.nodes:
			g1.nodes.append(node)
		if tnode not in g1.nodes:
			g1.nodes.append(tnode)
		g1.vertices.append(Vertex(node,tnode))
		tnode.obj.attacked.append((x,y,node))

# Schritt 2

for node1 in g1.nodes:
	heer1 = node1.obj
	for attack in heer1.attacked:
		attacker_node = attack[2]
		for node2 in heere:
			heer2 = node2.obj
			# heer 1 ist der angegriffene
			# heer 2 der potentielle Helfer
			if heer1 != heer2 and allied(heer1.f,heer2.f) and not allied(heer2.f, attacker_node.obj.f):
				if is_neighbour(heer2, (heer1.rx, heer1.ry), attack):
					if node2 not in g1.nodes:
						g1.nodes.append(node2)
					g1.vertices.append(Vertex(attacker_node,node2))

# Schritt 3

battles = g1.dgraphs()

count = 1

for battle in battles:
	print "Schlacht "+str(count),
	count = count + 1
	for node in battle.nodes:
		heer = node.obj
		print heer.toString()+",",
	print

	# Schritt 4

	for node in battle.nodes:
		heer = node.obj
		print "\tHeer "+str(heer.toString())+" attackiert gegen",
		for enemy in battle.neighbours(node):
			print str(enemy.obj.toString())+",",
		print

