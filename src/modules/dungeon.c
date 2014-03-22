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

#include <platform.h>
#include <kernel/config.h>

#if DUNGEON_MODULE
#include "dungeon.h"
#include "gmcmd.h"

#include <triggers/gate.h>
#include <triggers/unguard.h>

/* kernel includes */
#include <building.h>
#include <item.h>
#include <plane.h>
#include <race.h>
#include <region.h>
#include <skill.h>
#include <terrain.h>
#include <unit.h>

/* util includes */
#include <event.h>
#include <xml.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef struct treasure {
  const struct item_type *itype;
  int amount;
  struct treasure *next;
} treasure;

typedef struct monster {
  const struct race *race;
  double chance;
  int maxunits;
  int avgsize;
  struct treasure *treasures;
  struct monster *next;
  struct itemtype_list *weapons;
} monster;

typedef struct skilllimit {
  skill_t skill;
  int minskill;
  int maxskill;
  struct skilllimit *next;
} skilllimit;

typedef struct dungeon {
  int level;
  int radius;
  int size;
  int maxpeople;
  struct skilllimit *limits;
  double connect;
  struct monster *boss;
  struct monster *monsters;
  struct dungeon *next;
} dungeon;

dungeon *dungeonstyles;

region *make_dungeon(const dungeon * data)
{
  int nb[2][3][2] = {
    {{-1, 0}, {0, 1}, {1, -1}},
    {{1, 0}, {-1, 1}, {0, -1}}
  };
  const struct race *bossrace = data->boss->race;
  char name[128];
  int size = data->size;
  int iterations = size * size;
  unsigned int flags = PFL_NORECRUITS;
  int n = 0;
  struct faction *fmonsters = get_monsters();
  plane *p;
  region *r, *center;
  region *rnext;
  region_list *iregion, *rlist = NULL;
  const terrain_type *terrain_hell = get_terrain("hell");

  assert(terrain_hell != NULL);
  sprintf(name, "Die Höhlen von %s", bossrace->generate_name(NULL));
  p = gm_addplane(data->radius, flags, name);

  center =
    findregion(p->minx + (p->maxx - p->minx) / 2,
    p->miny + (p->maxy - p->miny) / 2);
  assert(center);
  terraform_region(center, terrain_hell);
  add_regionlist(&rlist, center);
  rnext = r = center;
  while (size > 0 && iterations--) {
    int d, o = rng_int() % 3;
    for (d = 0; d != 3; ++d) {
      int index = (d + o) % 3;
      region *rn = findregion(r->x + nb[n][index][0], r->y + nb[n][index][1]);
      assert(r->terrain == terrain_hell);
      if (rn) {
        if (rn->terrain == terrain_hell) {
          rnext = rn;
        } else if (fval(rn->terrain, SEA_REGION)) {
          if (rng_int() % 100 < data->connect * 100) {
            terraform_region(rn, terrain_hell);
            --size;
            rnext = rn;
            add_regionlist(&rlist, rn);
          } else
            terraform(rn, T_FIREWALL);
        }
        if (size == 0)
          break;
      }
      rn =
        findregion(r->x + nb[(n + 1) % 2][index][0],
        r->y + nb[(n + 1) % 2][index][1]);
      if (rn && fval(rn->terrain, SEA_REGION)) {
        terraform(rn, T_FIREWALL);
      }
    }
    if (size == 0)
      break;
    if (r == rnext) {
      /* error */
      break;
    }
    r = rnext;
    n = (n + 1) % 2;
  }

  for (iregion = rlist; iregion; iregion = iregion->next) {
    monster *m = data->monsters;
    region *r = iregion->data;
    while (m) {
      if ((rng_int() % 100) < (m->chance * 100)) {
        /* TODO: check maxunits. */
        treasure *loot = m->treasures;
        struct itemtype_list *weapon = m->weapons;
        int size = 1 + (rng_int() % m->avgsize) + (rng_int() % m->avgsize);
        unit *u = createunit(r, fmonsters, size, m->race);
        while (weapon) {
          i_change(&u->items, weapon->type, size);
          weapon = weapon->next;
        }
        while (loot) {
          i_change(&u->items, loot->itype, loot->amount * size);
          loot = loot->next;
        }
      }
      m = m->next;
    }
  }
  return center;
}

void make_dungeongate(region * source, region * target, const struct dungeon *d)
{
  building *bsource, *btarget;

  if (source == NULL || target == NULL || d == NULL)
    return;
  bsource = new_building(bt_find("castle"), source, default_locale);
  set_string(&bsource->name, "Pforte zur Hölle");
  bsource->size = 50;
  add_trigger(&bsource->attribs, "timer", trigger_gate(bsource, target));
  add_trigger(&bsource->attribs, "create", trigger_unguard(bsource));
  fset(bsource, BLD_UNGUARDED);

  btarget = new_building(bt_find("castle"), target, default_locale);
  set_string(&btarget->name, "Pforte zur Außenwelt");
  btarget->size = 50;
  add_trigger(&btarget->attribs, "timer", trigger_gate(btarget, source));
  add_trigger(&btarget->attribs, "create", trigger_unguard(btarget));
  fset(btarget, BLD_UNGUARDED);
}

static int tagbegin(xml_stack * stack)
{
  xml_tag *tag = stack->tag;
  if (strcmp(tag->name, "dungeon") == 0) {
    dungeon *d = (dungeon *) calloc(sizeof(dungeon), 1);
    d->maxpeople = xml_ivalue(tag, "maxpeople");
    if (d->maxpeople == 0)
      d->maxpeople = INT_MAX;
    d->level = xml_ivalue(tag, "level");
    d->radius = xml_ivalue(tag, "radius");
    d->connect = xml_fvalue(tag, "connect");
    d->size = xml_ivalue(tag, "size");
    stack->state = d;
  } else {
    dungeon *d = (dungeon *) stack->state;
    if (strcmp(tag->name, "skilllimit") == 0) {
      skill_t sk = sk_find(xml_value(tag, "name"));
      if (sk != NOSKILL) {
        skilllimit *skl = calloc(sizeof(skilllimit), 1);
        skl->skill = sk;
        if (xml_value(tag, "max") != NULL) {
          skl->maxskill = xml_ivalue(tag, "max");
        } else
          skl->maxskill = INT_MAX;
        if (xml_value(tag, "min") != NULL) {
          skl->minskill = xml_ivalue(tag, "min");
        } else
          skl->maxskill = INT_MIN;
        skl->next = d->limits;
        d->limits = skl;
      }
    } else if (strcmp(tag->name, "monster") == 0) {
      monster *m = calloc(sizeof(monster), 1);
      m->race = rc_find(xml_value(tag, "race"));
      m->chance = xml_fvalue(tag, "chance");
      m->avgsize = _max(1, xml_ivalue(tag, "size"));
      m->maxunits = _min(1, xml_ivalue(tag, "maxunits"));

      if (m->race) {
        if (xml_bvalue(tag, "boss")) {
          d->boss = m;
        } else {
          m->next = d->monsters;
          d->monsters = m;
        }
      }
    } else if (strcmp(tag->name, "weapon") == 0) {
      monster *m = d->monsters;
      itemtype_list *w = calloc(sizeof(itemtype_list), 1);
      w->type = it_find(xml_value(tag, "type"));
      if (w->type) {
        w->next = m->weapons;
        m->weapons = w;
      }
    }
  }
  return XML_OK;
}

static int tagend(xml_stack * stack)
{
  xml_tag *tag = stack->tag;
  if (strcmp(tag->name, "dungeon")) {
    dungeon *d = (dungeon *) stack->state;
    stack->state = NULL;
    d->next = dungeonstyles;
    dungeonstyles = d;
  }
  return XML_OK;
}

xml_callbacks xml_dungeon = {
  tagbegin, tagend, NULL
};

void register_dungeon(void)
{
  xml_register(&xml_dungeon, "eressea dungeon", 0);
}

#endif
