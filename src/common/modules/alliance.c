/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
*/

#include <config.h>
#include <eressea.h>

#ifdef ALLIANCES
#include "alliance.h"
#include "command.h"

/* kernel includes */
#include <building.h>
#include <faction.h>
#include <message.h>
#include <region.h>
#include <unit.h>

/* util includes */
#include <umlaut.h>
#include <base36.h>

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
alliance_kick(const tnode * tnext, const char * str, void * data, const char * cmd)
{
	unit * u = (unit*)data;
	faction * f = findfaction(atoi36(igetstrtoken(str)));
	attrib * a;
	unused(tnext);

	if (f==NULL || f->alliance!=u->faction->alliance) {
		/* does not belong to our alliance */
		return;
	}
	a = a_find(f->attribs, &at_kick);
	if (a==NULL) a = a_add(&f->attribs, make_kick());
	add_kick(a, f);
}

static void
alliance_join(const tnode * tnext, const char * str, void * data, const char * cmd)
{
	unit * u = (unit*)data;
	alliance * al = findalliance(atoi36(igetstrtoken(str)));
	unused(tnext);

	if (u->faction->alliance!=NULL || al==NULL) {
		/* not found */
		return;
	}
	u->faction->alliance = al;
	/* inform the rest? */
}

static void
execute(const struct syntaxtree * syntax)
{
	region ** rp = &regions;
	while (*rp) {
		region * r = *rp;
		unit **up = &r->units;
		while (*up) {
			unit * u = *up;
			const struct locale * lang = u->faction->locale;
			tnode * root = stree_find(syntax, lang);
			strlist * order;
			for (order = u->orders; order; order = order->next) {
				if (igetkeyword(order->s, lang) == K_ALLIANCE) {
					do_command(root, u, order->s);
				}
			}
			if (u==*up) up = &u->next;
		}
		if (*rp==r) rp = &r->next;
	}
}

void
alliancekick(void)
{
	static syntaxtree * stree = NULL;
	faction * f = factions;
	if (stree==NULL) {
		syntaxtree * slang = stree = stree_create();
		while (slang) {
			struct tnode * root = calloc(sizeof(tnode), 1);
			struct tnode * leaf = calloc(sizeof(tnode), 1);
			add_command(root, leaf, LOC(slang->lang, "alliance"), NULL);
			add_command(leaf, NULL, LOC(slang->lang, "kick"), &alliance_kick);
			slang = slang->next;
		}
	}
	execute(stree);
	while (f) {
		attrib * a = a_find(f->attribs, &at_kick);
		if (a!=NULL) {
			faction_list * flist = (faction_list*)a->data.v;
			if (flist!=NULL) {
				unsigned int votes = listlen(flist);
				unsigned int size = listlen(f->alliance->members);
				if (size<=votes*2) {
					f->alliance = NULL;
					/* tell him he's been kicked */
					for (flist=f->alliance->members;flist;flist=flist->next) {
						ADDMSG(&flist->data->msgs, msg_message("alliance::kickedout",
							"member alliance votes", f, f->alliance, votes));
					}
				} else {
					/* warn that he's been attempted to kick */
					for (flist=f->alliance->members;flist;flist=flist->next) {
						ADDMSG(&flist->data->msgs, msg_message("alliance::kickattempt",
							"member alliance votes", f, f->alliance, votes));
					}
				}
			}
		}
		f = f->next;
	}
	/* some may have been kicked, must remove f->alliance==NULL */
}

void
alliancejoin(void)
{
	static syntaxtree * stree = NULL;
	if (stree==NULL) {
		syntaxtree * slang = stree = stree_create();
		while (slang) {
			struct tnode * root = calloc(sizeof(tnode), 1);
			struct tnode * leaf = calloc(sizeof(tnode), 1);
			add_command(root, leaf, LOC(slang->lang, "alliance"), NULL);
			add_command(leaf, NULL, LOC(slang->lang, "join"), &alliance_join);
			slang = slang->next;
		}
	}
	execute(stree);
}

void 
setalliance(struct faction * f, alliance * al)
{
	if (f->alliance==al) return;
	if (f->alliance!=NULL) {
		faction_list ** flistp = &f->alliance->members;
		while (*flistp) {
			if ((*flistp)->data==f) {
				*flistp = (*flistp)->next;
				break;
			}
			flistp = &(*flistp)->next;
		}
	}
	f->alliance = al;
	if (al!=NULL) {
		faction_list * flist = calloc(sizeof(faction_list), 1);
		flist->next = al->members;
		flist->data = f;
		al->members = flist;
	}
}

const char *
alliancename(const alliance * al)
{
	typedef char name[OBJECTIDSIZE + 1];
	static name idbuf[8];
	static int nextbuf = 0;

	char *buf = idbuf[(++nextbuf) % 8];

	if (al && al->name) {
		sprintf(buf, "%s (%s)", strcheck(al->name, NAMESIZE), itoa36(al->id));
	} else {
		return NULL;
	}
	return buf;
}

void
alliancevictory(void)
{
	const struct building_type * btype = bt_find("stronghold");
	region * r = regions;
	alliance * al = alliances;
	if (btype==NULL) return;
	while (r!=NULL) {
		building * b = r->buildings;
		while (b!=NULL) {
			if (b->type==btype) {
				unit * u = buildingowner(r, b);
				if (u) {
					fset(u->faction->alliance, FL_MARK);
				}
			}
			b = b->next;
		}
		r=r->next;
	}
	while (al!=NULL) {
		if (!fval(al, FL_MARK)) {
			faction_list * flist = al->members;
			while (flist!=0) {
				faction * f = flist->data;
				if (f->alliance==al) {
					ADDMSG(&f->msgs, msg_message("alliance::lost", 
						"alliance", al));
					destroyfaction(f);
				}
				flist = flist->next;
			}
		} else {
			freset(al, FL_MARK);
		}
		al = al->next;
	}
}
#endif
