/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
 
 */
#ifndef H_KRNL_PLAYER
#define H_KRNL_PLAYER
#ifdef __cplusplus
extern "C" {
#endif

struct faction;

typedef struct player {
	unsigned int id;
	char * name;
	char * email;
	char * address;
	struct vacation {
		int weeks;
		char * email;
	} * vacation;
	const struct faction * faction;

	struct player * nexthash; /* don't use! */
} player;

extern struct player * get_players(void);
extern struct player * get_player(unsigned int id);
extern struct player * make_player(const struct faction * f);
extern struct player * next_player(struct player * p);

#ifdef __cplusplus
}
#endif
#endif
