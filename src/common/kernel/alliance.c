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
#include <eressea.h>
#include "alliance.h"
#include "command.h"

/* kernel includes */
#include <faction.h>
#include <message.h>
#include <region.h>
#include <unit.h>

/* util includes */
#include <umlaut.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>

alliance * alliances = NULL;

alliance *
makealliance(int id, const char * name)
{
	alliance * al = calloc(1, sizeof(alliance));
	al->id=id;
	al->name=strdup(name);
	al->next=alliances;
	return alliances=al;
}

alliance *
findalliance(int id)
{
	alliance * al;
	for (al=alliances;al;al=al->next) {
		if (al->id==id) return al;
	}
	return NULL;
}

void destroy_kick(struct attrib * a)
{
	faction_list * flist = (faction_list*)a->data.v;
	freelist(flist);
}

const attrib_type at_kick = { "kick", 
	DEFAULT_INIT, destroy_kick
};

static attrib *
make_kick(void)
{
	return a_new(&at_kick);
}

void
add_kick(attrib * a, const faction * f)
{
	faction_list * flist = (faction_list*)a->data.v;
	while (flist && flist->data!=f) flist = flist->next;
	if (flist) return;
	flist = calloc(1, sizeof(faction_list));
	flist->data = (void*)f;
	flist->next = (faction_list*)a->data.v;
	a->data.v = flist;
}

static void
alliance_kick(const char * str, void * data, const char * cmd)
{
	unit * u = (unit*)data;
	faction * f = findfaction(atoi36(igetstrtoken(str)));
	attrib * a;

	if (f==NULL || f->alliance!=u->faction->alliance) {
		/* does not belong to our alliance */
		return;
	}
	a = a_find(f->attribs, &at_kick);
	if (a==NULL) a = a_add(&f->attribs, make_kick());
	add_kick(a, f);
}

static void
alliance_join(const char * str, void * data, const char * cmd)
{
	unit * u = (unit*)data;
	alliance * al = findalliance(atoi36(igetstrtoken(str)));
	
	if (u->faction->alliance!=NULL || al==NULL) {
		/* not found */
		return;
	}
	u->faction->alliance = al;
	/* inform the rest? */
}

static tnode * g_keys;
static void
alliance_command(const char * str, void * data, const char * cmd)
{
	do_command(g_keys, data, str);
}

static void
execute(tnode * root)
{
	region ** rp = &regions;
	while (*rp) {
		region * r = *rp;
		unit **up = &r->units;
		while (*up) {
			unit * u = *up;
			strlist * order;
			for (order = u->orders; order; order = order->next) {
				if (igetkeyword(order->s, u->faction->locale) == K_ALLIANCE) {
					do_command(root, u, order->s);
				}
			}
			if (u==*up) up = &u->next;
		}
		if (*rp==r) rp = &r->next;
	}
}

void
alliancejoin(void)
{
	tnode root;
	add_command(&root, "alliance", &alliance_command);
	g_keys = calloc(1, sizeof(tnode));
	add_command(g_keys, "join", &alliance_join);
	execute(&root);
	free(g_keys);
}

void
alliancekick(void)
{
	tnode root;
	add_command(&root, "alliance", &alliance_command);
	g_keys = calloc(1, sizeof(tnode));
	add_command(g_keys, "kick", &alliance_kick);
	execute(&root);
	free(g_keys);
}

