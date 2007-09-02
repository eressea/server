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

#include <config.h>
#include "eressea.h"
#include "alliance.h"

#include <attributes/key.h>

/* kernel includes */
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/message.h>
#include <kernel/order.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/item.h>
#include <kernel/command.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/lists.h>
#include <util/parser.h>
#include <util/umlaut.h>

/* libc includes */
#include <assert.h>
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
alliance_kick(const tnode * tnext, void * data, struct order * ord)
{
	unit * u = (unit*)data;
	faction * f = findfaction(getid());
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
alliance_join(const tnode * tnext, void * data, struct order * ord)
{
	unit * u = (unit*)data;
	alliance * al = findalliance(getid());
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
			order * ord;
			for (ord = u->orders; ord; ord = ord->next) {
				if (get_keyword(ord) == K_ALLIANCE) {
					do_command(root, u, ord);
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

  char *ibuf = idbuf[(++nextbuf) % 8];

  if (al && al->name) {
    snprintf(ibuf, sizeof(name), "%s (%s)", al->name, itoa36(al->id));
    ibuf[sizeof(name)-1] = 0;
  } else {
    return NULL;
  }
  return ibuf;
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
					fset(u->faction->alliance, FFL_MARK);
				}
			}
			b = b->next;
		}
		r=r->next;
	}
	while (al!=NULL) {
		if (!fval(al, FFL_MARK)) {
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
			freset(al, FFL_MARK);
		}
		al = al->next;
	}
}

int
victorycondition(const alliance * al, const char * name)
{
  const char * gems[] = { "opal", "diamond", "zaphire", "topaz", "beryl", "agate", "garnet", "emerald", NULL };
  if (strcmp(name, "gems")==0) {
    const char ** igem = gems;

    for (;*igem;++igem) {
      const struct item_type * itype = it_find(*igem);
      faction_list * flist = al->members;
      boolean found = false;

      assert(itype!=NULL);
      for (;flist && !found;flist=flist->next) {
        unit * u = flist->data->units;

        for (;u;u=u->nextF) {
          if (i_get(u->items, itype)>0) {
            found = true;
            break;
          }
        }
      }
      if (!found) return 0;
    }
    return 1;

  } else if (strcmp(name, "phoenix")==0) {
    faction_list * flist = al->members;
    for (;flist;flist=flist->next) {
      faction * f = flist->data;
      if (find_key(f->attribs, atoi36("phnx"))) {
        return 1;
      }
    }
    return 0;

  } else if (strcmp(name, "pyramid")==0) {

    /* Logik:
     * - if (pyr > last_passed_size && pyr > all_others) {
     *     pyr->passed->counter++;
     *     for(all_other_pyrs) {
     *       pyr->passed->counter=0;
     *     }
     *
     *     if(pyr->passed->counter >= 3) {
     *       set(pyr, passed);
     *       pyr->owner->set_attrib(pyra);
     *     }
     *     last_passed_size = pyr->size;
     *   }
     */

    faction_list * flist = al->members;
    for (;flist;flist=flist->next) {
      faction * f = flist->data;
      if (find_key(f->attribs, atoi36("pyra"))) {
        return 1;
      }
    }
    return 0;
  }
  return -1;
}
