# Introduction

Eressea is a play-by-email strategy role-playing game that simulates a
fantasy world. Unlike most video games, there are are no fancy
graphics, but everything that happens is communicated by text.

As a player, you are in control of a number of characters, which are
referred to as units, which combine to make a faction. Your faction
competes with other factions over the resources of the world that you
explore together. Sometimes this leads to war, and sometimes to
friendship between factions, and eventually it all adds up to a rich
history of the world, with its own heroes and legends.

Eressea is different from most games in many respects, but most of all
in that there is no declared winner. There is not even a goal, other
than to enjoy yourself, and set your own goals. Do you want to
monopolize a natural resource? Become the strongest military force?
Create a democratic government for the factions in your part of the
world? Collect trophies from slain dragons? Map the entire globe?
Those are all valid goals to set for yourself, but you might find your
own.

### How to play

Eressea is played in weekly turns. Every weekend, you receive a
textual report containing your units, the part of the world that they
can observe, and any events that happened since the last report. 

Based on that report, players send an email to the game's server that
contains orders for all of their units. A week later, the server
evaluates the orders from all players, updates the state of the world,
and sends out new reports.

A (slightly simplified) report may look like this:

	Report for Eressea, Sunday, 13. April 2014, 23:25

	It is the first week of the month of harvest moon in
	the 1. year of the fourth age. It is summer.

	Guardians (xin8), elves/draig (guardian@gmx.de)

	Your faction has 4 people in 1 unit.

	Vetkan (10,1), highland, 1012 peasants, 13156 silver,
	17 horses. To the northwest lies the forests of
	Buviken (9,2), to the northeast the mountains of Rebus
	(10,2), to the east the plain of Setutvul (-9,1), to
	the southeast the mountains of Vithil (-9,0), to the
	southwest the forests of Posotmetid (10,0) and to the
	west the forests of Cesartapun (9,1)

	The local market offers silk and mandrakes.

	Statistics for Vetkan (10,1):

	  Peasant wages: 11 silver
	  Recruits: 25 peasants

	  Pelenth (opw4), size 10, fortification.

	    * Fighters of Pelenth (r6b0), 4 elves,
	      aggressive, skills: melee 5, has: platemail,
	      sword, 500 silver.

	  + Osswid the Destroyer (wtmj), Vikings (atnf), 1
	    dwarf, has: 1 iron.

What you see here are two units, Osswid (wtmj) and the Fighters of
Pelenth (r6b0). The fighters are our own unit, but Osswid belongs to
the faction Vikings (atnf). Every unit, faction or other object in the
game is identified by a four-letter code, and it is common to append
it to the name in parentheses. The fort of Pelenth (opw4) is a
building, and our fighters are inside, which causes them to be
indented a little more, and gives them some benefits including
additional protection in combat.

While Osswid is a single dwarf, our unit of fighters consists of
several elves. Units can any number of men, which are always of the
same race. Each unit can learn any number of skills, and our Fighters
are skilled in melee combat at a respectable level 5. They own a sword
and one platemail, which means that one of them will be well equipped
when going into battle, while the other three will fight with their
bare hands. The unit also has 500 pieces of silver, the currency of
the game.

Since he isn't of our own faction, we cannot tell what skills Osswid
has, but we can tell that he's unarmed.

We see all this because one of our units is in the region, the
highland of Vetkan (10,1). Regions do not have a four-letter
identifier, but are instead identified by their coordinates on the
map, which makes it easier to visualize the world.

The region of Vetkan is surrounded by a number of other regions on six
sides, since Eressea's Map uses a hexagonal grid. Every region has a
terrain, and we can see highland, mountain, plain and forest regions
in our vicinity. Each terrain has different qualities: For example,
mountains are good for mining iron ore and quarries, while plains are
fertile and forests produce lumber. Each region is inhabited by a
peasant population, and you can see that Vetkan has a population of
1012. These peasants are the basis of the region's economy, and they
own a combined 13,156 silver that we could try to acquire through
taxation or other means.

Example orders for this faction may look like this:

	ERESSEA xin8 "password"
	UNIT r6b0
		STUDY perception
		GIVE wtmj 100 silver
		GIVE TEMP 1 150 silver
		GIVE TEMP 2 150 silver
	MAKE TEMP 1
		RECRUIT 1
		MOVE W
	END
	MAKE TEMP 2
		RECRUIT 1
		MOVE E
	END

Every set of orders begins with the faction's id and password, and is
followed by orders for one or more units. A unit's orders begin with
the word UNIT followed by the unit's id, and one or more orders. In
this case, we are giving the unit an order to STUDY the perception
skill, and to GIVE silver to three other units. STUDY is a standard
action, and each unit can perform exactly one per turn. If you do not
specify a standard action, then the unit will usually repeat the
action from the previous week. GIVE is a free action, and all units
can perform as many free actions in a week as they like.

The MAKE TEMP order is special: It creates a new unit with a temporary
id (in this case, we're calling them TEMP 1 and TEMP 2). The server
will replace this with a unique id by the time it sends the next
report, but until then, we can address the new unit by this temporary
identifier, for example to give it the silver that it requires for
recruiting new members. RECRUIT is another example of a free action,
while MOVE is a standard action. The MAKE TEMP order is technically
given by the unit whose orders preceded it, and the new unit's orders
are finished with the END keyword.
