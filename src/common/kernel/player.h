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
#ifndef PLAYER_H
#define PLAYER_H

struct faction;

typedef struct player {
	unsigned int id;
	char * name;
	char * email;
	char * info;
	struct faction * faction;

	struct player * nexthash; /* don't use! */
} player;

extern struct player * get_players(void);
extern struct player * get_player(unsigned int id);
extern struct player * make_player(void);
extern struct player * next_player(struct player * p);

#endif
