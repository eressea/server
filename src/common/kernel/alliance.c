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

#pragma region includes

#include <config.h>
#include <kernel/eressea.h>
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
#include <util/language.h>
#include <util/parser.h>
#include <util/rng.h>
#include <util/umlaut.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#pragma endregion

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

typedef struct alliance_transaction {
  struct alliance_transaction * next;
  unit * u;
  variant userdata;
  order * ord;
} alliance_transaction;

static faction * alliance_leader(const alliance * al)
{
  return al->leader;
}

static alliance_transaction **
get_transaction(alliance * al, const variant * userdata, int type)
{
  alliance_transaction ** tap=al->transactions+type;
  while (*tap) {
    alliance_transaction * ta = *tap;
    if (userdata==NULL || memcmp(&ta->userdata, userdata, sizeof(variant))==0) {
      return tap;
    }
    tap = &ta->next;
  }
  return NULL;
}

static void
create_transaction(alliance * al, int type, const variant * userdata, unit * u, order * ord)
{
  alliance_transaction * tr = malloc(sizeof(alliance_transaction));
  memcpy(&tr->userdata, userdata, sizeof(variant));
  tr->ord = ord;
  tr->next = al->transactions[type];
  al->transactions[type] = tr;
}

static void
cmd_kick(const tnode * tnext, void * data, struct order * ord)
{
  unit * u = (unit*)data;
  alliance * al = u->faction->alliance;
  faction * f = findfaction(getid());
  unused(tnext);

  if (f && al && u->faction==alliance_leader(al)) {
    static variant var;
    var.v = f;
    create_transaction(al, ALLIANCE_KICK, &var, u, ord);
  }
}

static void
cmd_leave(const tnode * tnext, void * data, struct order * ord)
{
  unit * u = (unit*)data;
  alliance * al = u->faction->alliance;
  unused(tnext);

  if (al) {
    create_transaction(al, ALLIANCE_LEAVE, NULL, u, ord);
  }
}

static void
cmd_transfer(const tnode * tnext, void * data, struct order * ord)
{
  unit * u = (unit*)data;
  alliance * al = u->faction->alliance;
  faction * f = findfaction(getid());
  unused(tnext);

  if (f && al && f->alliance==al && u->faction==alliance_leader(al)) {
    static variant var;
    var.v = f;
    create_transaction(al, ALLIANCE_TRANSFER, &var, u, ord);
  }
}

static void
cmd_new(const tnode * tnext, void * data, struct order * ord)
{
  unit * u = (unit*)data;
  alliance * al = u->faction->alliance;
  unused(tnext);

  if (al && u->faction==alliance_leader(al)) {
    static variant var;
    const char * str = getstrtoken();

    var.i = str?atoi36(str):0;
    create_transaction(al, ALLIANCE_NEW, &var, u, ord);
  }
}

static void
cmd_invite(const tnode * tnext, void * data, struct order * ord)
{
  unit * u = (unit*)data;
  alliance * al = u->faction->alliance;
  faction * f = findfaction(getid());
  unused(tnext);

  if (f==NULL || al==NULL) {
    /* TODO: error message */
    return;
  } else {
    static variant var;
    var.v = f;
    create_transaction(al, ALLIANCE_INVITE, &var, u, ord);
  }
}

static void
cmd_join(const tnode * tnext, void * data, struct order * ord)
{
  unit * u = (unit*)data;
  alliance * al = findalliance(getid());
  unused(tnext);

  if (u->faction->alliance==al || al==NULL) {
    /* TODO: error message */
    return;
  }

  create_transaction(al, ALLIANCE_JOIN, NULL, u, ord);
}

static void
perform_kick(alliance * al)
{
  alliance_transaction ** tap = al->transactions+ALLIANCE_KICK;
  while (*tap) {
    alliance_transaction * ta = *tap;
    faction * f = ta->u->faction;
    setalliance(f, NULL);
    *tap = ta->next;
    free(ta);
  }
}

static void
perform_new(alliance * al)
{
  alliance_transaction ** tap = al->transactions+ALLIANCE_LEAVE;
  while (*tap) {
    alliance * al;
    alliance_transaction * ta = *tap;
    faction * f = ta->u->faction;
    int id = ta->userdata.i;
    
    do {
      id = id?id:(1 + (rng_int() % MAX_UNIT_NR));
      for (al=alliances;al;al=al->next) {
        if (al->id==id) {
          id = 0;
          break;
        }
      }
    } while (id==0);

    al = calloc(1, sizeof(alliance));
    al->id = id;
    setalliance(f, al);

    *tap = ta->next;
    free(ta);
  }

}

static void
perform_leave(alliance * al)
{
  alliance_transaction ** tap = al->transactions+ALLIANCE_LEAVE;
  while (*tap) {
    alliance_transaction * ta = *tap;
    faction * f = ta->u->faction;

    setalliance(f, NULL);

    *tap = ta->next;
    free(ta);
  }

}

static void
perform_transfer(alliance * al)
{
  alliance_transaction ** tap = al->transactions+ALLIANCE_LEAVE;
  while (*tap) {
    alliance_transaction * ta = *tap;
    faction * f = (faction *)ta->userdata.v;

    assert(f->alliance==al);

    al->leader = f;

    *tap = ta->next;
    free(ta);
  }

}

static void
perform_join(alliance * al)
{
  alliance_transaction ** tap = al->transactions+ALLIANCE_JOIN;
  while (*tap) {
    alliance_transaction * ta = *tap;
    faction * f = ta->u->faction;
    if (f->alliance!=al) {
      alliance_transaction ** tip = al->transactions+ALLIANCE_INVITE;
      alliance_transaction * ti = *tip;
      while (ti) {
        if (ti->userdata.v==f) break;
        tip = &ti->next;
        ti = *tip;
      }
      if (ti) {
        setalliance(f, al);
        *tip = ti->next;
        free(ti);
      } else {
        /* TODO: error message */
      }
    }
    *tap = ta->next;
    free(ta);
  }

}

static void
execute(const struct syntaxtree * syntax, keyword_t kwd)
{
  int run = 0;

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
        if (get_keyword(ord) == kwd) {
          do_command(root, u, ord);
          run = 1;
        }
      }
      if (u==*up) up = &u->next;
    }
    if (*rp==r) rp = &r->next;
  }

  if (run) {
    alliance * al;
    for (al=alliances;al;al=al->next) {
      perform_kick(al);
    }
    for (al=alliances;al;al=al->next) {
      perform_leave(al);
    }
    for (al=alliances;al;al=al->next) {
      perform_transfer(al);
    }
    for (al=alliances;al;al=al->next) {
      perform_new(al);
    }
    for (al=alliances;al;al=al->next) {
      perform_join(al);
    }
  }
}

void
alliance_cmd(void)
{
  static syntaxtree * stree = NULL;
  if (stree==NULL) {
    syntaxtree * slang = stree = stree_create();
    while (slang) {
      struct tnode * root = calloc(sizeof(tnode), 1);
      struct tnode * leaf = calloc(sizeof(tnode), 1);
      add_command(root, leaf, LOC(slang->lang, "alliance"), NULL);
      add_command(leaf, NULL, LOC(slang->lang, "new"), &cmd_new);
      add_command(leaf, NULL, LOC(slang->lang, "invite"), &cmd_invite);
      add_command(leaf, NULL, LOC(slang->lang, "join"), &cmd_join);
      add_command(leaf, NULL, LOC(slang->lang, "kick"), &cmd_kick);
      add_command(leaf, NULL, LOC(slang->lang, "leave"), &cmd_new);
      add_command(leaf, NULL, LOC(slang->lang, "command"), &cmd_transfer);
      slang = slang->next;
    }
  }
  execute(stree, K_ALLIANCE);
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
      add_command(leaf, NULL, LOC(slang->lang, "join"), &cmd_join);
      slang = slang->next;
    }
  }
  execute(stree, K_ALLIANCE);
}

void 
setalliance(struct faction * f, alliance * al)
{
  faction_list * flist = NULL;
  if (f->alliance==al) return;
  if (f->alliance!=NULL) {
    faction_list ** flistp = &f->alliance->members;
    while (*flistp) {
      faction_list * flist = *flistp;
      if ((*flistp)->data==f) {
        *flistp = flist->next;
        break;
      }
      flistp = &flist->next;
    }

    if (f->alliance->leader==f) {
      if (f->alliance->members) {
        f->alliance->leader = f->alliance->members->data;
      } else {
        f->alliance->leader = NULL;
      }
    }
  }
  f->alliance = al;
  if (al!=NULL) {
    faction_list ** flistp = &al->members;
    while (*flistp) {
      flistp = &(*flistp)->next;
    }
    if (flist==NULL) {
      flist = malloc(sizeof(faction_list));
      flist->data = f;
    }
    *flistp = flist;
    flist->next = NULL;
    if (al->leader==NULL) {
      al->leader = f;
    }
  }
  free(flist);
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

void alliance_setname(alliance * self, const char * name)
{
  free(self->name);
  if (name) self->name = strdup(name);
  else self->name = NULL;
}
