/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2001   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
 
 */
#include <config.h>
#include "player.h"

#include <goodies.h>

#include <string.h>
#include <stdlib.h>
#include <limits.h>

#define PMAXHASH 1023

typedef struct player_hash {
	struct player * entries;
} player_hash;

static player_hash * players[PMAXHASH];

player *
make_player(void)
{
	player * p = calloc(sizeof(player), 1);
	unsigned int hash;
	
	for (p->id = rand();;p->id++) {
		/* if there is a hashing conflict, resolve it */
		player * pi = get_player(p->id);
		if (pi) p->id++;
		else break;
	}
	hash = p->id % PMAXHASH;
	p->nexthash = players[hash]->entries;
	players[hash]->entries = p;

	return p;
}

player *
next_player(player * p)
{
	if (p->nexthash) return p->nexthash;
	else {
		unsigned int hash = p->id % PMAXHASH;
		p = NULL;
		while (++hash!=PMAXHASH) {
			if (players[hash]->entries!=NULL) {
				p = players[hash]->entries;
				break;
			}
		}
		return p;
	}
}

player *
get_player(unsigned int id)
{
	unsigned int hash = id % PMAXHASH;
	struct player * p = players[hash]->entries;

	while (p && p->id!=id) p = p->nexthash;
	return p;
}

player *
get_players(void)
{
	struct player * p = NULL;
	unsigned int hash = 0;

	while (p!=NULL && hash!=PMAXHASH) {
		p = players[hash++]->entries;
	}
	return p;
}

void
players_done(void)
{
	int i;
	for (i=0;i!=PMAXHASH;++i) {
		player * p = players[i]->entries;
		players[i]->entries = p->nexthash;
		free(p->name);
		if (p->email) free(p->email);
		if (p->info) free(p->info);
		free(p);
	}
}
