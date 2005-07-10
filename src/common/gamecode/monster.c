/* vi: set ts=2:
 *
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#include <config.h>
#include "eressea.h"
#include "monster.h"

/* gamecode includes */
#include "economy.h"

/* triggers includes */
#include <triggers/removecurse.h>

/* attributes includes */
#include <attributes/targetregion.h>
#include <attributes/hate.h>

/* spezialmonster */
#include <spells/alp.h>

/* kernel includes */
#include <kernel/build.h>
#include <kernel/faction.h>
#include <kernel/give.h>
#include <kernel/item.h>
#include <kernel/message.h>
#include <kernel/movement.h>
#include <kernel/names.h>
#include <kernel/order.h>
#include <kernel/pathfinder.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/reports.h>
#include <kernel/skill.h>
#include <kernel/unit.h>

/* util includes */
#include <attrib.h>
#include <base36.h>
#include <event.h>
#include <rand.h>

/* libc includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define UNDEAD_REPRODUCTION         0 	/* vermehrung */
#define MOVECHANCE                  25	/* chance fuer bewegung */

#define MAXILLUSION_TEXTS   3

static boolean
is_waiting(const unit * u)
{
	if (fval(u, UFL_ISNEW|UFL_MOVED)) return true;
  return false;
}

static order *
monster_attack(unit * u, const unit * target)
{
	char zText[20];

	if (u->region!=target->region) return NULL;
  if (u->faction==target->faction) return NULL;
	if (!cansee(u->faction, u->region, target, 0)) return NULL;
	if (is_waiting(u)) return NULL;

	sprintf(zText, "%s %s",
		locale_string(u->faction->locale, keywords[K_ATTACK]), unitid(target));
	return parse_order(zText, u->faction->locale);
}


static order *
get_money_for_dragon(region * r, unit * u, int wanted)
{
	unit *u2;
	int n;

	/* attackiere bewachende einheiten */
  for (u2 = r->units; u2; u2 = u2->next) {
    if (u2 != u && getguard(u2)&GUARD_TAX) {
      order * ord = monster_attack(u, u2);
      if (ord) addlist(&u->orders, ord);
    }
  }

	/* falls genug geld in der region ist, treiben wir steuern ein. */
  if (rmoney(r) >= wanted) {
    /* 5% chance, dass der drache aus einer laune raus attackiert */
    if (chance(1.0-u->race->aggression)) return parse_order(keywords[K_TAX], default_locale);
  }

  /* falls der drache launisch ist, oder das regionssilber knapp, greift er alle an */
	n = 0;
  for (u2 = r->units; u2; u2 = u2->next) {
    if (u2->faction != u->faction && cansee(u->faction, r, u2, 0)) {
      int m = get_money(u2);
      if (m==0 || (getguard(u2) & GUARD_TAX)) continue;
      else {
        order * ord = monster_attack(u, u2);
        if (ord) {
          addlist(&u->orders, ord);
          n += m;
        }
      }
    }
  }

  /* falls die einnahmen erreicht werden, bleibt das monster noch eine
	 * runde hier. */
  if (n + rmoney(r) >= wanted) {
    return parse_order(keywords[K_TAX], default_locale);
  }

  /* wenn wir NULL zurückliefern, macht der drache was anderes, z.b. weggehen */
  return NULL;
}

static int
all_money(region * r, faction * f)
{
	unit *u;
	int m;

	m = rmoney(r);
  for (u = r->units; u; u = u->next) {
    if (f!=u->faction) {
      m += get_money(u);
    }
	}
	return m;
}

static direction_t
richest_neighbour(region * r, faction * f, int absolut)
{

	/* m - maximum an Geld, d - Richtung, i - index, t = Geld hier */

	double m;
	double t;
	direction_t d = NODIRECTION, i;

	if (absolut == 1 || rpeasants(r) == 0) {
		m = (double) all_money(r, f);
	} else {
		m = (double) all_money(r, f) / (double) rpeasants(r);
	}

	/* finde die region mit dem meisten geld */

	for (i = 0; i != MAXDIRECTIONS; i++)
		if (rconnect(r, i) && rterrain(rconnect(r, i)) != T_OCEAN) {
			if (absolut == 1 || rpeasants(r) == 0) {
				t = (double) all_money(rconnect(r, i), f);
			} else {
				t = (double) all_money(rconnect(r, i), f) / (double) rpeasants(r);
			}

			if (t > m) {
				m = t;
				d = i;
			}
		}
	return d;
}

static boolean
room_for_race_in_region(region *r, const race * rc)
{
	unit *u;
	int  c = 0;

	for(u=r->units;u;u=u->next) {
		if(u->race == rc) c += u->number;
	}

	if(c > (rc->splitsize*2))
		return false;

	return true;
}

static direction_t
random_neighbour(region * r, unit *u)
{
	direction_t i;
	region *rc;
	int rr, c = 0, c2 = 0;

	/* Nachsehen, wieviele Regionen in Frage kommen */

	for (i = 0; i != MAXDIRECTIONS; i++) {
		rc = rconnect(r, i);
		if (rc && can_survive(u, rc)) {
			if(room_for_race_in_region(rc, u->race)) {
				c++;
			}
			c2++;
		}
	}

	if (c == 0) {
		if(c2 == 0) {
			return NODIRECTION;
		} else {
			c  = c2;
			c2 = 0;		/* c2 == 0 -> room_for_race nicht beachten */
		}
	}

	/* Zufällig eine auswählen */

	rr = rand() % c;

	/* Durchzählen */

	c = -1;
	for (i = 0; i != MAXDIRECTIONS; i++) {
		rc = rconnect(r, i);
		if (rc && can_survive(u, rc)) {
			if(c2 == 0) {
				c++;
			} else if(room_for_race_in_region(rc, u->race)) {
				c++;
			}
			if (c == rr) return i;
		}
	}

	assert(1 == 0);				/* Bis hierhin sollte er niemals kommen. */
	return NODIRECTION;
}

static direction_t
treeman_neighbour(region * r)
{
	direction_t i;
	int rr;
	int c = 0;

	/* Nachsehen, wieviele Regionen in Frage kommen */

	for (i = 0; i != MAXDIRECTIONS; i++) {
		if (rconnect(r, i)
			&& rterrain(rconnect(r, i)) != T_OCEAN
			&& rterrain(rconnect(r, i)) != T_GLACIER
			&& rterrain(rconnect(r, i)) != T_DESERT) {
			c++;
		}
	}

	if (c == 0) {
		return NODIRECTION;
	}
	/* Zufällig eine auswählen */

	rr = rand() % c;

	/* Durchzählen */

	c = -1;
	for (i = 0; i != MAXDIRECTIONS; i++) {
		if (rconnect(r, i)
			&& rterrain(rconnect(r, i)) != T_OCEAN
			&& rterrain(rconnect(r, i)) != T_GLACIER
			&& rterrain(rconnect(r, i)) != T_DESERT) {
			c++;
			if (c == rr) {
				return i;
			}
		}
	}

	assert(1 == 0);				/* Bis hierhin sollte er niemals kommen. */
	return NODIRECTION;
}

static order *
monster_move(region * r, unit * u)
{
  direction_t d = NODIRECTION;

  switch(old_race(u->race)) {
    case RC_FIREDRAGON:
    case RC_DRAGON:
    case RC_WYRM:
      d = richest_neighbour(r, u->faction, 1);
      break;
    case RC_TREEMAN:
      d = treeman_neighbour(r);
      break;
    default:
      d = random_neighbour(r,u);
      break;
  }

  /* falls kein geld gefunden wird, zufaellig verreisen, aber nicht in
  * den ozean */

  if (d == NODIRECTION)
    return NULL;

  sprintf(buf, "%s %s", locale_string(u->faction->locale, keywords[K_MOVE]), locale_string(u->faction->locale, directions[d]));

  return parse_order(buf, u->faction->locale);
}

/* Wir machen das mal autoconf-style: */
#ifndef HAVE_DRAND48
#define drand48() (((double)rand()) / RAND_MAX)
#endif

static int
dragon_affinity_value(region *r, unit *u)
{
	int m = all_money(r, u->faction);

	if(u->race == new_race[RC_FIREDRAGON]) {
		return (int)(normalvariate(m,m/2));
	} else {
		return (int)(normalvariate(m,m/4));
	}
}

static attrib *
set_new_dragon_target(unit * u, region * r, int range)
{
	int max_affinity = 0;
	region *max_region = NULL;

#if 1
  region_list * rptr, * rlist = regions_in_range(r, range, allowed_dragon);
  for (rptr=rlist;rptr;rptr=rptr->next) {
    region * r2 = rptr->data;
    int affinity = dragon_affinity_value(r2, u);
    if (affinity > max_affinity) {
      max_affinity = affinity;
      max_region = r2;
    }
  }

  free_regionlist(rlist);
#else
  int x, y;
  for (x = r->x - range; x < r->x + range; x++) {
		for (y = r->y - range; y < r->y + range; y++) {
      region * r2 = findregion(x, y);
      if (r2!=NULL) {
        int affinity = dragon_affinity_value(r2, u);
        if (affinity > max_affinity) {
          if (koor_distance (r->x, r->y, x, y) <= range && path_exists(r, r2, range, allowed_dragon)) 
          {
            max_affinity = affinity;
            max_region = r2;
          }
				}
			}
		}
	}
#endif
	if (max_region && max_region != r) {
		attrib * a = a_find(u->attribs, &at_targetregion);
		if (!a) {
			a = a_add(&u->attribs, make_targetregion(max_region));
		} else {
			a->data.v = max_region;
		}
		sprintf(buf, "Kommt aus: %s, Will nach: %s", 
      regionname(r, u->faction), regionname(max_region, u->faction));
		usetprivate(u, buf);
		return a;
	}
	return NULL;
}

static order *
make_movement_order(unit * u, const region * target, int moves, boolean (*allowed)(const region *, const region *))
{
	region * r = u->region;
	region ** plan = path_find(r, target, DRAGON_RANGE*5, allowed);
	int position = 0;
	char * c;

	if (plan==NULL) {
		return NULL;
	}

	strcpy(buf, locale_string(u->faction->locale, keywords[K_MOVE]));
	c = buf + strlen(buf);

	while (position!=moves && plan[position+1]) {
		region * prev = plan[position];
		region * next = plan[++position];
		direction_t dir = reldirection(prev, next);
		assert(dir!=NODIRECTION && dir!=D_SPECIAL);
		*c++ = ' ';
		strcpy(c, locale_string(u->faction->locale, directions[dir]));
		c += strlen(c);
	}

	return parse_order(buf, u->faction->locale);
}

static order *
monster_seeks_target(region *r, unit *u)
{
	direction_t d;
	unit *target = NULL;
	int dist, dist2;
	direction_t i;
	region *nr;

	/* Das Monster sucht ein bestimmtes Opfer. Welches, steht
	 * in einer Referenz/attribut
	 * derzeit gibt es nur den alp
	 */

	switch( old_race(u->race) ) {
		case RC_ALP:
			target = alp_target(u);
			break;
		default:
			assert(!"Seeker-Monster gibt kein Ziel an");
	}

	/* TODO: prüfen, ob target überhaupt noch existiert... */
  if (!target) {
    log_error(("Monster '%s' hat kein Ziel!\n", unitname(u)));
    return NULL; /* this is a bug workaround! remove!! */
  }

	if(r == target->region ) { /* Wir haben ihn! */
		if (u->race == new_race[RC_ALP]) {
			alp_findet_opfer(u, r);
		}
		else {
			assert(!"Seeker-Monster hat keine Aktion fuer Ziel");
		}
		return NULL;
	}

	/* Simpler Ansatz: Nachbarregion mit gerinster Distanz suchen.
	 * Sinnvoll momentan nur bei Monstern, die sich nicht um das
	 * Terrain kümmern.  Nebelwände & Co machen derzeit auch nix...
	 */
	dist2 = distance(r, target->region);
	d = NODIRECTION;
	for( i = 0; i < MAXDIRECTIONS; i++ ) {
		nr = rconnect(r, i);
		assert(nr);
		dist = distance(nr, target->region);
		if( dist < dist2 ) {
			dist2 = dist;
			d = i;
		}
	}
	assert(d != NODIRECTION );

	sprintf(buf, "%s %s", locale_string(u->faction->locale, keywords[K_MOVE]), locale_string(u->faction->locale, directions[d]));
	return parse_order(buf, u->faction->locale);
}

unit *
random_unit(const region * r)
{
	int c = 0;
	int n;
	unit *u;

	for (u = r->units; u; u = u->next) {
		if (u->race != new_race[RC_SPELL]) {
			c += u->number;
		}
	}

	if (c == 0) {
		return NULL;
	}
	n = rand() % c;
	c = 0;
	u = r->units;

	while (u && c < n) {
		if (u->race != new_race[RC_SPELL]) {
			c += u->number;
		}
		u = u->next;
	}

	return u;
}

static void
monster_attacks(unit * u)
{
  region * r = u->region;
  unit * u2;

  for (u2=r->units;u2;u2=u2->next) {
    if (chance(0.75)) {
      order * ord = monster_attack(u, u2);
      if (ord) addlist(&u->orders, ord);
    }
  }
}

static void
eaten_by_monster(unit * u)
{
	int n = 0;
	int horse = 0;

	switch (old_race(u->race)) {
	case RC_FIREDRAGON:
		n = rand()%80 * u->number;
		horse = get_item(u, I_HORSE);
		break;
	case RC_DRAGON:
		n = rand()%200 * u->number;
		horse = get_item(u, I_HORSE);
		break;
	case RC_WYRM:
		n = rand()%500 * u->number;
		horse = get_item(u, I_HORSE);
		break;
	default:
		n = rand()%(u->number/20+1);
	}

	if (n > 0) {
		n = lovar(n);
		n = min(rpeasants(u->region), n);

		if (n > 0) {
			deathcounts(u->region, n);
			rsetpeasants(u->region, rpeasants(u->region) - n);
			ADDMSG(&u->region->msgs, msg_message("eatpeasants", "unit amount", u, n));
		}
	}
	if (horse > 0) {
		set_item(u, I_HORSE, 0);
    ADDMSG(&u->region->msgs, msg_message("eathorse", "unit amount", u, horse));
	}
}

static void
absorbed_by_monster(unit * u)
{
	int n;

	switch (old_race(u->race)) {
	default:
		n = rand()%(u->number/20+1);
	}

	if(n > 0) {
		n = lovar(n);
		n = min(rpeasants(u->region), n);
		if (n > 0){
			rsetpeasants(u->region, rpeasants(u->region) - n);
			scale_number(u, u->number + n);
			ADDMSG(&u->region->msgs, msg_message("absorbpeasants", 
        "unit race amount", u, u->race, n));
		}
	}
}

static int
scareaway(region * r, int anzahl)
{
	int n, p, diff = 0, emigrants[MAXDIRECTIONS];
	direction_t d;

	anzahl = min(max(1, anzahl),rpeasants(r));

	/* Wandern am Ende der Woche (normal) oder wegen Monster. Die
	 * Wanderung wird erst am Ende von demographics () ausgefuehrt.
	 * emigrants[] ist local, weil r->newpeasants durch die Monster
	 * vielleicht schon hochgezaehlt worden ist. */

	for (d = 0; d != MAXDIRECTIONS; d++)
		emigrants[d] = 0;

	p = rpeasants(r);
	assert(p >= 0 && anzahl >= 0);
	for (n = min(p, anzahl); n; n--) {
		direction_t dir = (direction_t)(rand() % MAXDIRECTIONS);
		region * c = rconnect(r, dir);

		if (c && landregion(rterrain(c))) {
			++diff;
			c->land->newpeasants++;
			emigrants[dir]++;
		}
	}
	rsetpeasants(r, p-diff);
	assert(p >= diff);
	return diff;
}

static void
scared_by_monster(unit * u)
{
	int n;

	switch (old_race(u->race)) {
	case RC_FIREDRAGON:
		n = rand()%160 * u->number;
		break;
	case RC_DRAGON:
		n = rand()%400 * u->number;
		break;
	case RC_WYRM:
		n = rand()%1000 * u->number;
		break;
	default:
		n = rand()%(u->number/4+1);
	}

	if(n > 0) {
		n = lovar(n);
		n = min(rpeasants(u->region), n);
		if(n > 0) {
			n = scareaway(u->region, n);
			if(n > 0) {
        ADDMSG(&u->region->msgs, msg_message("fleescared", 
          "amount unit", n, u));
			}
		}
	}
}

static const char *
random_growl(void)
{
	switch(rand()%5) {
	case 0:
		return "Groammm";
	case 1:
		return "Roaaarrrr";
	case 2:
		return "Chhhhhhhhhh";
	case 3:
		return "Tschrrrkk";
	case 4:
		return "Schhhh";
	}
	return "";
}

extern attrib_type at_direction;

static order *
monster_learn(unit *u)
{
	int c = 0;
	int n;
	skill * sv;
  const struct locale * lang = u->faction->locale;

	/* Monster lernt ein zufälliges Talent aus allen, in denen es schon
	 * Lerntage hat. */

  for (sv = u->skills; sv != u->skills + u->skill_size; ++sv) {
    if (sv->level>0) ++c;
  }

	if(c == 0) return NULL;

	n = rand()%c + 1;
	c = 0;

  for (sv = u->skills; sv != u->skills + u->skill_size; ++sv) {
		if (sv->level>0) {
			if (++c == n) {
				sprintf(buf, "%s %s", locale_string(lang, keywords[K_STUDY]),
					skillname(sv->id, lang));
				return parse_order(buf, lang);
			}
		}
	}
  return NULL;
}

void
monsters_kill_peasants(void)
{
  region *r;
  unit *u;

  for (r = regions; r; r = r->next) {
    for (u = r->units; u; u = u->next) if(!fval(u, UFL_MOVED)) {
      if (u->race->flags & RCF_SCAREPEASANTS) {
        scared_by_monster(u);
      }
      if (u->race->flags & RCF_KILLPEASANTS) {
        eaten_by_monster(u);
      }
      if (u->race->flags & RCF_ABSORBPEASANTS) {
        absorbed_by_monster(u);
      }
    }
  }
}

static boolean
check_overpopulated(unit *u)
{
  unit *u2;
  int c = 0;

  for(u2 = u->region->units; u2; u2 = u2->next) {
    if(u2->race == u->race && u != u2) c += u2->number;
  }

  if(c > u->race->splitsize * 2) return true;

  return false;
}

static void
recruit_dracoids(unit * dragon, int size)
{
  faction * f = dragon->faction;
  region * r = dragon->region;
  const struct item_type * weapon = NULL;
  order * new_order;
  unit *un = createunit(r, f, size, new_race[RC_DRACOID]);

  fset(un, UFL_ISNEW|UFL_MOVED);

  name_unit(un);
  change_money(dragon, -un->number * 50);

  set_level(un, SK_SPEAR, (3 + rand() % 4));
  set_level(un, SK_SWORD, (3 + rand() % 4));
  set_level(un, SK_LONGBOW, (2 + rand() % 3));

  switch (rand() % 3) {
    case 0:
      weapon = olditemtype[I_LONGBOW];
      break;
    case 1:
      weapon = olditemtype[I_SWORD];
      break;
    default:
      weapon = olditemtype[I_SPEAR];
      break;
  }
  i_change(&un->items, weapon, un->number);
  if (weapon->rtype->wtype->flags & WTF_MISSILE) un->status = ST_BEHIND;
  else un->status = ST_FIGHT;

  sprintf(buf, "%s \"%s\"", keywords[K_STUDY], skillname(weapon->rtype->wtype->skill, f->locale));
  new_order = parse_order(buf, default_locale);
#ifdef LASTORDER
  set_order(&un->lastorder, new_order);
#else
  addlist(&un->orders, new_order);
#endif
}

static order *
plan_dragon(unit * u)
{
  attrib * ta = a_find(u->attribs, &at_targetregion);
  region * r = u->region;
  region * tr = NULL;
  int horses = get_resource(u,R_HORSE);
  int capacity = walkingcapacity(u);
  item ** itmp = &u->items;
  boolean move = false;
  order * long_order = NULL;

  if (horses > 0) {
    change_resource(u, R_HORSE, - min(horses,(u->number*2)));
  }
  while (capacity>0 && *itmp!=NULL) {
    item * itm = *itmp;
    if (itm->type->capacity<itm->type->weight) {
      int weight = itm->number*itm->type->capacity;
      if (weight > capacity) {
        int error = give_item(itm->number, itm->type, u, NULL, NULL);
        if (error!=0) break;
      }
    }
    if (*itmp==itm) itmp=&itm->next;
  }

  if (ta==NULL) {
    move |= (r->land==0 || r->land->peasants==0); /* when no peasants, move */
    move |= (r->land==0 || r->land->money==0); /* when no money, move */
  }
  move |= chance(0.04); /* 4% chance to change your mind */
  if (move) {
    /* dragon gets bored and looks for a different place to go */
    ta = set_new_dragon_target(u, u->region, DRAGON_RANGE);
  }
  else ta = a_find(u->attribs, &at_targetregion);
  if (ta!=NULL) {
    tr = (region *) ta->data.v;
    if (tr==NULL || !path_exists(u->region, tr, DRAGON_RANGE, allowed_dragon)) {
      ta = set_new_dragon_target(u, u->region, DRAGON_RANGE);
      if (ta) tr = findregion(ta->data.sa[0], ta->data.sa[1]);
    }
  }
  if (tr!=NULL) {
    assert(long_order==NULL);
    switch(old_race(u->race)) {
    case RC_FIREDRAGON:
      long_order = make_movement_order(u, tr, 4, allowed_dragon);
      break;
    case RC_DRAGON:
      long_order = make_movement_order(u, tr, 3, allowed_dragon);
      break;
    case RC_WYRM:
      long_order = make_movement_order(u, tr, 1, allowed_dragon);
      break;
    }
    if (rand()%100 < 15) {
      /* do a growl */
      if (rname(tr, u->faction->locale)) {
        sprintf(buf,
          "botschaft an region %s~...~%s~etwas~in~%s.",
          estring(random_growl()), u->number==1?"Ich~rieche":"Wir~riechen",
          estring(rname(tr, u->faction->locale)));
        addlist(&u->orders, parse_order(buf, u->faction->locale));
      }
    }
  } else {
    /* we have no target. do we like it here, then? */
    long_order = get_money_for_dragon(u->region, u, income(u));
    if (long_order==NULL) {
      /* money is gone, need a new target */
      set_new_dragon_target(u, u->region, DRAGON_RANGE);
    }
    else if (u->race != new_race[RC_FIREDRAGON]) {
      /* neue dracoiden! */
      terrain_t t = rterrain(r);
      if (r->land && !(terrain[t].flags & FORBIDDEN_LAND)) {
        int ra = 20 + rand() % 100;
        if (get_money(u) > ra * 50 + 100 && rand() % 100 < 50) {
          recruit_dracoids(u, ra);
        }
      }
    }
  }
  if (long_order==NULL) {
    sprintf(buf, "%s \"%s\"", keywords[K_STUDY], skillname(SK_OBSERVATION, u->faction->locale));
    long_order = parse_order(buf, default_locale);
  }
  return long_order;
}

void
plan_monsters(void)
{
  region *r;
  faction *f = findfaction(MONSTER_FACTION);

  if (!f) return;
  f->lastorders = turn;

  for (r = regions; r; r = r->next) {
    unit *u;
    double attack_chance = MONSTERATTACK;
    boolean attacking = false;

    for (u = r->units; u; u = u->next) {
      attrib * ta;
      order * long_order = NULL;

      /* Ab hier nur noch Befehle für NPC-Einheiten. */
      if (u->faction->no != MONSTER_FACTION) continue;

      if (attack_chance>0.0) {
        if (chance(attack_chance)) attacking = true;
        attack_chance = 0.0;
      }

      if (u->status>ST_BEHIND) u->status = ST_FIGHT; /* all monsters fight */
      /* Monster bekommen jede Runde ein paar Tage Wahrnehmung dazu */
      produceexp(u, SK_OBSERVATION, u->number);

      /* Befehle müssen jede Runde neu gegeben werden: */
      free_orders(&u->orders);

      if (attacking) {
        monster_attacks(u);
      }
      /* units with a plan to kill get ATTACK orders: */
      ta = a_find(u->attribs, &at_hate);
      if (ta && !is_waiting(u)) {
        unit * tu = (unit *)ta->data.v;
        if (tu && tu->region==r) {
          sprintf(buf, "%s %s", locale_string(u->faction->locale, keywords[K_ATTACK]), itoa36(tu->no));
          addlist(&u->orders, parse_order(buf, u->faction->locale));
        } else if (tu) {
          tu = findunitg(ta->data.i, NULL);
          if (tu!=NULL) {
            long_order = make_movement_order(u, tu->region, 2, allowed_walk);
          }
        }
        else a_remove(&u->attribs, ta);
      }

      /* All monsters guard the region: */
      if (!is_waiting(u) && r->land) {
        const char * cmd = locale_string(u->faction->locale, keywords[K_GUARD]);
        addlist(&u->orders, parse_order(cmd, u->faction->locale));
      }

      /* Einheiten mit Bewegungsplan kriegen ein NACH: */
      if (long_order==NULL) {
        attrib * ta = a_find(u->attribs, &at_targetregion);
        if (ta) {
          if (u->region == (region*)ta->data.v) {
            a_remove(&u->attribs, ta);
          }
        } else if (u->race->flags & RCF_MOVERANDOM) {
          if (rand()%100<MOVECHANCE || check_overpopulated(u)) {
            long_order = monster_move(r, u);
          }
        }
      }

      if (long_order==NULL) {
        /* Einheiten, die Waffenlosen Kampf lernen könnten, lernen es um 
        * zu bewachen: */
        if (u->race->bonus[SK_WEAPONLESS] != -99) {
          if (eff_skill(u, SK_WEAPONLESS, u->region) < 1) {
            sprintf(buf, "%s %s", locale_string(f->locale, keywords[K_STUDY]),
              skillname(SK_WEAPONLESS, f->locale));
            long_order = parse_order(buf, f->locale);
          }
        }
      }

      if (long_order==NULL) {
        /* Ab hier noch nicht generalisierte Spezialbehandlungen. */

        switch (old_race(u->race)) {
          case RC_SEASERPENT:
            long_order = parse_order(keywords[K_PIRACY], default_locale);
            break;
          case RC_ALP:
            long_order = monster_seeks_target(r, u);
            break;
          case RC_FIREDRAGON:
          case RC_DRAGON:
          case RC_WYRM:
            long_order = plan_dragon(u);
            break;
          default:
            if (u->race->flags & RCF_LEARN) {
              long_order = monster_learn(u);
            }
            break;
        }
        if (long_order) {
          set_order(&u->thisorder, copy_order(long_order));
#ifdef LASTORDER
          set_order(&u->lastorder, copy_order(long_order));
#else
          addlist(&u->orders, long_order);
#endif
        }
      }
    }
  }
  pathfinder_cleanup();
}
