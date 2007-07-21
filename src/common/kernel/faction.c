/* vi: set ts=2:
 *
 *
 * Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include "eressea.h"
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
#include <util/resolve.h>
#include <util/rng.h>
#include <util/variant.h>

#include <attributes/otherfaction.h>

/* libc includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

void *
resolve_faction(variant id) {
   return findfaction(id.i);
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
  const char * pass = itoa36(rng_int());
  char buf[128];

  assert(frace && frace != new_race[RC_ORC]);

  if (set_email(&f->email, email)!=0) {
    log_error(("Invalid email address for faction %s: %s\n", itoa36(f->no), email));
  }

  f->override = strdup(pass);
  if (password) {
    f->passw = strdup(password);
  } else {
    pass = itoa36(rng_int());
    f->passw = strdup(pass);
  }

  f->lastorders = turn;
  f->alive = 1;
  f->age = 0;
  f->race = frace;
  f->magiegebiet = 0;
  f->locale = loc;
  f->subscription = subscription;

  f->options = want(O_REPORT) | want(O_ZUGVORLAGE) | want(O_SILBERPOOL) | want(O_COMPUTER) | want(O_COMPRESS) | want(O_ADRESSEN) | want(O_STATISTICS);

  f->no = unused_faction_id();
  addlist(&factions, f);
  fhash(f);

  snprintf(buf, sizeof(buf), "%s %s", LOC(loc, "factiondefault"), factionid(f));
  f->name = xstrdup(buf);

  return f;
}

unit *
addplayer(region *r, faction * f)
{
  unit *u;

  assert(f->units==NULL);
  set_ursprung(f, 0, r->x, r->y);
  u = createunit(r, f, 1, f->race);
  equip_unit(u, get_equipment("new_faction"));
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
#ifdef SHORTPWDS
  shortpwd * slist = f->shortpwds;
  if (shortp) while (slist) {
    if (xstrcmp(slist->pwd, passwd)==0) {
      slist->used = true;
      return true;
    }
    slist = slist->next;
  }
#endif
  if (strcmp(f->passw, passwd)==0) return true;
  if (strcmp(f->override, passwd)==0) return true;
  return false;
}


int
read_faction_reference(faction ** f, FILE * F)
{
  variant id;
  if (global.data_version >= BASE36IDS_VERSION) {
    char zText[16];
    fscanf(F, "%s ", zText);
    id.i = atoi36(zText);
  } else {
    fscanf(F, "%d ", &id.i);
  }
  if (id.i<0) {
    *f = NULL;
    return AT_READ_FAIL;
  }
  *f = findfaction(id.i);
  if (*f==NULL) ur_add(id, (void**)f, resolve_faction);
  return AT_READ_OK;
}

void
write_faction_reference(const faction * f, FILE * F)
{
  fprintf(F, "%s ", itoa36(f->no));
}

void
destroyfaction(faction * f)
{
  region *rc;
  unit *u;
  faction *ff;

  if (!f->alive) return;

  while (f->battles) {
    struct bmsg * bm = f->battles;
    f->battles = bm->next;
    if (bm->msgs) free_messagelist(bm->msgs);
    free(bm);
  }

  for (u=f->units;u;u=u->nextF) {
    region * r = u->region;
    distribute_items(u);
    if (!fval(r->terrain, SEA_REGION) && !!playerrace(u->race)) {
      const race * rc = u->race;
      int p = rpeasants(u->region);
      int m = rmoney(u->region);
      int h = rhorses(u->region);

      /* Personen gehen nur an die Bauern, wenn sie auch von dort
       * stammen */
      if ((rc->ec_flags & ECF_REC_UNLIMITED)==0) {
        if (rc->ec_flags & ECF_REC_HORSES) { /* Zentauren an die Pferde */
          h += u->number;
        } else if (rc == new_race[RC_URUK]){ /* Orks zählen nur zur Hälfte */
          p += u->number/2;
        } else {
          p += u->number;
        }
      }
      m += get_money(u);
      h += get_item(u, I_HORSE);
      rsetpeasants(r, p);
      rsethorses(r, h);
      rsetmoney(r, m);

    }
    set_number(u, 0);
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

#ifdef ENEMIES
boolean
is_enemy(const struct faction * f, const struct faction * enemy)
{
  struct faction_list * flist = f->enemies;
  for (;flist!=NULL;flist=flist->next) {
    if (flist->data==enemy) return true;
  }
  return false;
}

static void
add_enemy_i(struct faction * f, struct faction * enemy)
{
  if (!is_enemy(f, enemy)) {
    struct faction_list * flist = malloc(sizeof(faction_list));
    flist->next = f->enemies;
    flist->data = enemy;
    f->enemies = flist;
  }
}

void
add_enemy(struct faction * f, struct faction * enemy)
{
  add_enemy_i(f, enemy);
/*  add_enemy_i(enemy, f); */
}

static void
remove_enemy_i(struct faction * f, const struct faction * enemy)
{
  struct faction_list **pflist = &f->enemies;
  while (*pflist!=NULL) {
    struct faction_list * flist = *pflist;
    if (flist->data==enemy) {
      *pflist = flist->next;
      free(flist);
    } else {
      pflist = &flist->next;
    }
  }
}

void
remove_enemy(struct faction * f, struct faction * enemy)
{
  remove_enemy_i(f, enemy);
/*  remove_enemy_i(enemy, f); */
}

#endif
