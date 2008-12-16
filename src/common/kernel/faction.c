/* vi: set ts=2:
 *
 *
 * Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include <kernel/eressea.h>
#include "faction.h"

#include "equipment.h"
#include "group.h"
#include "item.h"
#include "message.h"
#include "plane.h"
#include "race.h"
#include "region.h"
#include "terrain.h"
#include "unit.h"
#include "version.h"

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/goodies.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/resolve.h>
#include <util/rng.h>
#include <util/storage.h>
#include <util/sql.h>
#include <util/variant.h>
#include <util/unicode.h>
#include <attributes/otherfaction.h>

/* libc includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

faction *
get_monsters(void)
{
  static faction * monsters;
  static int gamecookie = -1;
  if (gamecookie!=global.cookie) {
    monsters = NULL;
    gamecookie = global.cookie;
  }

  if (!monsters) {
    faction * f;
    for (f=factions;f;f=f->next) {
      if (f->flags&FFL_NPC) {
        return monsters=f;
      }
    }
  }
  return monsters;
}

const unit *
random_unit_in_faction(const faction *f)
{
  unit *u;
  int c = 0, u_nr;

  for(u = f->units; u; u=u->next) c++;

  u_nr = rng_int()%c;
  c = 0;

  for(u = f->units; u; u=u->next)
    if(u_nr == c) return u;

  /* Hier sollte er nie ankommen */
  return NULL;
}

const char *
factionname(const faction * f)
{
  typedef char name[OBJECTIDSIZE+1];
  static name idbuf[8];
  static int nextbuf = 0;

  char *ibuf = idbuf[(++nextbuf) % 8];

  if (f && f->name) {
    snprintf(ibuf, sizeof(name), "%s (%s)", f->name, itoa36(f->no));
    ibuf[sizeof(name)-1] = 0;
  } else {
    strcpy(ibuf, "Unbekannte Partei (?)");
  }
  return ibuf;
}

int
resolve_faction(variant id, void * address) {
  faction * f = NULL;
  if (id.i!=0) {
    f = findfaction(id.i);
    if (f==NULL) {
      return -1;
    }
  }
  *(faction**)address = f;
  return 0;
}

#define MAX_FACTION_ID (36*36*36*36)

static int
unused_faction_id(void)
{
  int id = rng_int()%MAX_FACTION_ID;

  while (!faction_id_is_unused(id)) {
    id++; if(id == MAX_FACTION_ID) id = 0;
  }

  return id;
}

faction *
addfaction(const char *email, const char * password,
           const struct race * frace, const struct locale *loc,
           int subscription)
{
  faction * f = calloc(sizeof(faction), 1);
  char buf[128];

  assert(frace && frace != new_race[RC_ORC]);

  if (set_email(&f->email, email)!=0) {
    log_error(("Invalid email address for faction %s: %s\n", itoa36(f->no), email));
  }

  f->override = strdup(itoa36(rng_int()));
  faction_setpassword(f, password);

  f->lastorders = turn;
  f->alive = 1;
  f->age = 0;
  f->race = frace;
  f->magiegebiet = 0;
  f->locale = loc;
  f->subscription = subscription;

  f->options = want(O_REPORT) | want(O_ZUGVORLAGE) | want(O_COMPUTER) | want(O_COMPRESS) | want(O_ADRESSEN) | want(O_STATISTICS);

  f->no = unused_faction_id();
  addlist(&factions, f);
  fhash(f);

  snprintf(buf, sizeof(buf), "%s %s", LOC(loc, "factiondefault"), factionid(f));
  f->name = strdup(buf);

  return f;
}

unit *
addplayer(region *r, faction * f)
{
  unit *u;

  assert(f->units==NULL);
  set_ursprung(f, 0, r->x, r->y);
  u = createunit(r, f, 1, f->race);
  equip_items(&u->faction->items, get_equipment("new_faction"));
  equip_unit(u, get_equipment("first_unit"));
  equip_unit(u, get_equipment(u->race->_name[0]));
  u->hp = unit_max_hp(u) * u->number;
  fset(u, UFL_ISNEW);
  if (f->race == new_race[RC_DAEMON]) {
    race_t urc;
    do {
      urc = (race_t)(rng_int() % MAXRACES);
    } while (new_race[urc]==NULL || urc == RC_DAEMON || !playerrace(new_race[urc]));
    u->irace = new_race[urc];
  }

  return u;
}

boolean
checkpasswd(const faction * f, const char * passwd, boolean shortp)
{
  if (unicode_utf8_strcasecmp(f->passw, passwd)==0) return true;
  if (unicode_utf8_strcasecmp(f->override, passwd)==0) return true;
  return false;
}


variant
read_faction_reference(struct storage * store)
{
  variant id;
  id.i = store->r_id(store);
  return id;
}

void
write_faction_reference(const faction * f, struct storage * store)
{
  store->w_id(store, f?f->no:0);
}

void
destroyfaction(faction * f)
{
  region *rc;
  unit *u = f->units;
  faction *ff;

  if (!f->alive) return;

  while (f->battles) {
    struct bmsg * bm = f->battles;
    f->battles = bm->next;
    if (bm->msgs) free_messagelist(bm->msgs);
    free(bm);
  }

  while (u) {
    /* give away your stuff, make zombies if you cannot (quest items) */
    int result = gift_items(u, GIFT_FRIENDS|GIFT_PEASANTS);
    if (result!=0) {
      unit * zombie = u;
      u = u->nextF;
      make_zombie(zombie);
    } else {
      region * r = u->region;

      if (!fval(r->terrain, SEA_REGION) && !!playerrace(u->race)) {
        const race * rc = u->race;
        int m = rmoney(r);

        if ((rc->ec_flags & ECF_REC_ETHEREAL)==0) {
          int p = rpeasants(u->region);
          int h = rhorses(u->region);

          /* Personen gehen nur an die Bauern, wenn sie auch von dort
           * stammen */
          if (rc->ec_flags & ECF_REC_HORSES) { /* Zentauren an die Pferde */
            h += u->number;
          } else if (rc == new_race[RC_URUK]){ /* Orks zählen nur zur Hälfte */
            p += u->number/2;
          } else {
            p += u->number;
          }
          h += get_item(u, I_HORSE);
          rsetpeasants(r, p);
          rsethorses(r, h);
        }
        m += get_money(u);
        rsetmoney(r, m);
      }
      set_number(u, 0);
      u=u->nextF;
    }
  }
  f->alive = 0;
/* no way!  f->units = NULL; */
  handle_event(f->attribs, "destroy", f);
  for (ff = factions; ff; ff = ff->next) {
    group *g;
    ally *sf, *sfn;

    /* Alle HELFE für die Partei löschen */
    for (sf = ff->allies; sf; sf = sf->next) {
      if (sf->faction == f) {
        removelist(&ff->allies, sf);
        break;
      }
    }
    for(g=ff->groups; g; g=g->next) {
      for (sf = g->allies; sf;) {
        sfn = sf->next;
        if (sf->faction == f) {
          removelist(&g->allies, sf);
          break;
        }
        sf = sfn;
      }
    }
  }

  /* units of other factions that were disguised as this faction
   * have their disguise replaced by ordinary faction hiding. */
  for (rc=regions; rc; rc=rc->next) {
    for(u=rc->units; u; u=u->next) {
      attrib *a = a_find(u->attribs, &at_otherfaction);
      if(!a) continue;
      if (get_otherfaction(a) == f) {
        a_removeall(&u->attribs, &at_otherfaction);
        fset(u, UFL_PARTEITARNUNG);
      }
    }
  }
}

int
get_alliance(const faction * a, const faction * b)
{
  const ally * sf = a->allies;
  for (;sf!=NULL;sf=sf->next) {
    if (sf->faction==b) {
      return sf->status;
    }
  }
  return 0;
}

void
set_alliance(faction * a, faction * b, int status)
{
  ally ** sfp;
  sfp = &a->allies;
  while (*sfp) {
    ally * sf = *sfp;
    if (sf->faction==b) break;
    sfp = &sf->next;
  }
  if (*sfp==NULL) {
    ally * sf = *sfp = malloc(sizeof(ally));
    sf->next = NULL;
    sf->status = status;
    sf->faction = b;
    return;
  }
  (*sfp)->status |= status;
}

void
renumber_faction(faction * f, int no)
{
  if (f->subscription) {
    sql_print(("UPDATE subscriptions set faction='%s' where id=%u;\n",
      itoa36(no), f->subscription));
  }
  funhash(f);
  f->no = no;
  fhash(f);
  fset(f, FFL_NEWID);
}

#ifdef SMART_INTERVALS
void
update_interval(struct faction * f, struct region * r)
{
  if (r==NULL || f==NULL) return;
  if (f->first==NULL || f->first->index>r->index) {
    f->first = r;
  }
  if (f->last==NULL || f->last->index<=r->index) {
    f->last = r;
  }
}
#endif

const char * faction_getname(const faction * self)
{
  return self->name;
}

void faction_setname(faction * self, const char * name)
{
  free(self->name);
  if (name) self->name = strdup(name);
}

const char * faction_getemail(const faction * self)
{
  return self->email;
}

void faction_setemail(faction * self, const char * email)
{
  free(self->email);
  if (email) self->email = strdup(email);
}

const char * faction_getbanner(const faction * self)
{
  return self->banner;
}

void faction_setbanner(faction * self, const char * banner)
{
  free(self->banner);
  if (banner) self->banner = strdup(banner);
}

void
faction_setpassword(faction * f, const char * passw)
{
  free(f->passw);
  if (passw) f->passw = strdup(passw);
  else f->passw = strdup(itoa36(rng_int()));
}

const char *
faction_getpassword(const faction * f)
{
  return f->passw;
}

