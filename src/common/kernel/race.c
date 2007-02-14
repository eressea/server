/* vi: set ts=2:
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
#include "race.h"

#include <races/zombies.h>
#include <races/dragons.h>
#include <races/illusion.h>

#include "alchemy.h"
#include "build.h"
#include "building.h"
#include "equipment.h"
#include "faction.h"
#include "group.h"
#include "item.h"
#include "karma.h"
#include "magic.h"
#include "names.h"
#include "pathfinder.h"
#include "region.h"
#include "ship.h"
#include "skill.h"
#include "terrain.h"
#include "unit.h"

/* util includes */
#include <util/attrib.h>
#include <util/functions.h>
#include <util/rng.h>

/* attrib includes */
#include <attributes/raceprefix.h>
#include <attributes/synonym.h>

/* libc includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

/** external variables **/
race * races;

race_list * 
get_familiarraces(void)
{
  static int init = 0;
  static race_list * familiarraces;

  if (!init) {
    race * rc = races;
    for (;rc!=NULL;rc=rc->next) {
      if (rc->init_familiar!=NULL) {
        racelist_insert(&familiarraces, rc);
      }
    }
    init = false;
  }
  return familiarraces;
}

void 
racelist_clear(struct race_list **rl)
{
  while (*rl) {
    race_list * rl2 = (*rl)->next;
    free(*rl);
    *rl = rl2;
  }
}

void 
racelist_insert(struct race_list **rl, const struct race *r)
{
  race_list *rl2 = (race_list*)malloc(sizeof(race_list));

  rl2->data = r;
  rl2->next = *rl;

  *rl = rl2;
}

race *
rc_new(const char * zName)
{
  char zBuffer[80];
  race * rc = calloc(sizeof(race), 1);
  if (strchr(zName, ' ')!=NULL) {
    log_error(("race '%s' has an invalid name. remove spaces\n", zName));
    assert(strchr(zName, ' ')==NULL);
  }
  strcpy(zBuffer, zName);
  rc->_name[0] = strdup(zBuffer);
  sprintf(zBuffer, "%s_p", zName);
  rc->_name[1] = strdup(zBuffer);
  sprintf(zBuffer, "%s_d", zName);
  rc->_name[2] = strdup(zBuffer);
  sprintf(zBuffer, "%s_x", zName);
  rc->_name[3] = strdup(zBuffer);
  rc->precombatspell = NULL;

  rc->attack[0].type = AT_COMBATSPELL;
  rc->attack[1].type = AT_NONE;
  return rc;
}

race *
rc_add(race * rc)
{
  rc->next = races;
  return races = rc;
}

static const char * racealias[2][2] = {
  { "skeletton lord", "skeleton lord" },
  { NULL, NULL }
};

race *
rc_find(const char * name)
{
	const char * rname = name;
	race * rc = races;
	int i;

	for (i=0;racealias[i][0];++i) {
		if (strcmp(racealias[i][0], name)==0) {
			rname = racealias[i][1];
			break;
		}
	}
	while (rc && !strcmp(rname, rc->_name[0])==0) rc = rc->next;
  return rc;
}

/** dragon movement **/
boolean
allowed_dragon(const region * src, const region * target)
{
	if (fval(src->terrain, ARCTIC_REGION) && fval(target->terrain, SEA_REGION)) return false;
	return allowed_fly(src, target);
}

char ** race_prefixes = NULL;

extern void 
add_raceprefix(const char * prefix)
{
  static size_t size = 4;
  static unsigned int next = 0;
  if (race_prefixes==NULL) race_prefixes = malloc(size * sizeof(char*));
  if (next+1==size) {
    size *= 2;
    race_prefixes = realloc(race_prefixes, size * sizeof(char*));
  }
  race_prefixes[next++] = strdup(prefix);
  race_prefixes[next] = NULL;
}

/* Die Bezeichnungen dürfen wegen der Art des Speicherns keine
 * Leerzeichen enthalten! */

const struct race_syn race_synonyms[] = {
	{1, {"Fee", "Feen", "Feen", "Feen"}},
	{2, {"Gnoll", "Gnolle", "Gnollen", "Gnoll"}},
	{-1, {NULL, NULL, NULL, NULL}}
};

/*                      "den Zwergen", "Halblingsparteien" */

void
set_show_item(faction *f, item_t i)
{
	attrib *a = a_add(&f->attribs, a_new(&at_showitem));
	a->data.v = (void*)olditemtype[i];
}

int
unit_max_hp(const unit * u)
{
	int h;
	double p;
	static const curse_type * heal_ct = NULL;
	h = u->race->hitpoints;
	if (heal_ct==NULL) heal_ct = ct_find("healingzone");


	p = pow(effskill(u, SK_AUSDAUER) / 2.0, 1.5) * 0.2;
	h += (int) (h * p + 0.5);

#ifdef KARMA_MODULE
	if (fspecial(u->faction, FS_UNDEAD)) {
		h *= 2;
	}
#endif /* KARMA_MODULE */

	/* der healing curse verändert die maximalen hp */
	if (heal_ct) {
		curse *c = get_curse(u->region->attribs, heal_ct);
		if (c) {
			h = (int) (h * (1.0+(curse_geteffect(c)/100)));
		}
	}

	return h;
}

void
give_starting_equipment(struct unit *u)
{
  struct region *r = u->region;

  switch(old_race(u->race)) {
  case RC_ELF:
    set_show_item(u->faction, I_FEENSTIEFEL);
    break;
  case RC_GOBLIN:
    set_show_item(u->faction, I_RING_OF_INVISIBILITY);
    set_number(u, 10);
    break;
  case RC_HUMAN:
    {
      const building_type * btype = bt_find("castle");
      if (btype!=NULL) {
        building *b = new_building(btype, r, u->faction->locale);
        b->size = 10;
        u->building = b;
        fset(u, UFL_OWNER);
      }
    }
    break;
  case RC_CAT:
    set_show_item(u->faction, I_RING_OF_INVISIBILITY);
    break;
  case RC_AQUARIAN:
    {
      ship *sh = new_ship(st_find("boat"), u->faction->locale, r);
      sh->size = sh->type->construction->maxsize;
      u->ship = sh;
      fset(u, UFL_OWNER);
    }
    break;
  case RC_CENTAUR:
    rsethorses(r, 250+rng_int()%51+rng_int()%51);
    break;
  }
  u->hp = unit_max_hp(u) * u->number;
}

boolean
r_insectstalled(const region * r)
{
	return fval(r->terrain, ARCTIC_REGION);
}

const char *
rc_name(const race * rc, int n)
{
  return mkname("race", rc->_name[n]);
}

const char *
raceprefix(const unit *u)
{
  const attrib * asource = u->faction->attribs;

  if (fval(u, UFL_GROUP)) {
    const attrib * agroup = agroup = a_findc(u->attribs, &at_group);
    if (agroup!=NULL) asource = ((const group *)(agroup->data.v))->attribs;
  }
  return get_prefix(asource);
}

const char *
racename(const struct locale *loc, const unit *u, const race * rc)
{
  const char * prefix = raceprefix(u);
  attrib * asyn = a_find(u->faction->attribs, &at_synonym);

  if (prefix!=NULL) {
    static char lbuf[80];
    char * s = lbuf;
    strcpy(lbuf, locale_string(loc, mkname("prefix", prefix)));
    s += strlen(lbuf);
    if (asyn!=NULL) {
      strcpy(s, LOC(loc, ((frace_synonyms *)(asyn->data.v))->synonyms[u->number != 1]));
    } else {
      strcpy(s, LOC(loc, rc_name(rc, u->number != 1)));
    }
    s[0] = (char)tolower(s[0]);
    return lbuf;
  } else if (asyn!=NULL) {
    return(LOC(loc, ((frace_synonyms *)(asyn->data.v))->synonyms[u->number != 1]));
  }
  return LOC(loc, rc_name(rc, u->number != 1));
}

static void
oldfamiliars(unit * u)
{
  char fname[64];
  /* these familiars have no special skills.
   */
  snprintf(fname, sizeof(fname), "%s_familiar", u->race->_name[0]);
  create_mage(u, M_GRAU);
  equip_unit(u, get_equipment(fname));
}

static item *
default_spoil(const struct race * rc, int size)
{
  item * itm = NULL;

  if (rng_int()%100 < RACESPOILCHANCE) {
    char spoilname[32];
    const item_type * itype;

    sprintf(spoilname,  "%sspoil", rc->_name[0]);
    itype = it_find(spoilname);
    if (itype!=NULL) {
      i_add(&itm, i_new(itype, size));
    }
	}
	return itm;
}

int
rc_specialdamage(const race * ar, const race * dr, const struct weapon_type * wtype)
{
  race_t art = old_race(ar);
  int m, modifier = 0;

  if (wtype!=NULL && wtype->modifiers!=NULL) for (m=0;wtype->modifiers[m].value;++m) {
    /* weapon damage for this weapon, possibly by race */
    if (wtype->modifiers[m].flags & WMF_DAMAGE) {
      race_list * rlist = wtype->modifiers[m].races;
      if (rlist!=NULL) {
        while (rlist) {
          if (rlist->data == ar) break;
          rlist = rlist->next;
        }
        if (rlist==NULL) continue;
      }
      modifier += wtype->modifiers[m].value;
    }
  }
  switch (art) {
    case RC_HALFLING:
      if (wtype!=NULL && dragonrace(dr)) {
        modifier += 5;
      }
      break;
    default:
      break;
  }
  return modifier;
}

void
write_race_reference(const race * rc, FILE * F)
{
	fprintf(F, "%s ", rc?rc->_name[0]:"none");
}

int
read_race_reference(const struct race ** rp, FILE * F)
{
	char zName[20];
	if (global.data_version<NEWRACE_VERSION) {
		int i;
		fscanf(F, "%d", &i);
		if (i>=0) {
			*rp = new_race[i];
		} else {
			*rp = NULL;
			return AT_READ_FAIL;
		}
	} else {
		fscanf(F, "%s", zName);
		if (strcmp(zName, "none")==0) {
			*rp = NULL;
			return AT_READ_OK;
		}
		*rp = rc_find(zName);
		assert(*rp!=NULL);
	}
	return AT_READ_OK;
}

/* Die Funktionen werden über den hier registrierten Namen in races.xml
 * in die jeweilige Rassendefiniton eingebunden */
void
register_races(void)
{
	char zBuffer[MAX_PATH];
	/* function initfamiliar */
	register_function((pf_generic)oldfamiliars, "oldfamiliars");

	/* function move
	 * into which regiontyp moving is allowed for this race
	 * race->move_allowed() */
	register_function((pf_generic)allowed_swim, "moveswimming");
	register_function((pf_generic)allowed_walk, "movewalking");
	register_function((pf_generic)allowed_fly, "moveflying");
	register_function((pf_generic)allowed_dragon, "movedragon");

	/* function name 
	 * generate a name for a nonplayerunit
	 * race->generate_name() */
	register_function((pf_generic)untoten_name, "nameundead");
	register_function((pf_generic)skeleton_name, "nameskeleton");
	register_function((pf_generic)zombie_name, "namezombie");
	register_function((pf_generic)ghoul_name, "nameghoul");
	register_function((pf_generic)drachen_name, "namedragon");
	register_function((pf_generic)dracoid_name, "namedracoid");
	register_function((pf_generic)shadow_name, "nameshadow");

	/* function age 
	 * race->age() */
	register_function((pf_generic)age_undead, "ageundead");
	register_function((pf_generic)age_illusion, "ageillusion");
	register_function((pf_generic)age_skeleton, "ageskeleton");
	register_function((pf_generic)age_zombie, "agezombie");
	register_function((pf_generic)age_ghoul, "ageghoul");
	register_function((pf_generic)age_dragon, "agedragon");
	register_function((pf_generic)age_firedragon, "agefiredragon");

	/* function itemdrop
	 * to generate battle spoils
	 * race->itemdrop() */
  register_function((pf_generic)default_spoil, "defaultdrops");

	sprintf(zBuffer, "%s/races.xml", resourcepath());
}

void 
init_races(void)
{
}
