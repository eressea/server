/* vi: set ts=2:
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
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
#include "version.h"

/* util includes */
#include <util/attrib.h>
#include <util/bsdstring.h>
#include <util/functions.h>
#include <util/log.h>
#include <util/rng.h>

/* attrib includes */
#include <attributes/raceprefix.h>

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

  if (prefix!=NULL) {
    static char lbuf[80];
    char * bufp = lbuf;
    size_t size = sizeof(lbuf) - 1;
    int ch, bytes;

    bytes = (int)strlcpy(bufp, LOC(loc, mkname("prefix", prefix)), size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();

    bytes = (int)strlcpy(bufp, LOC(loc, rc_name(rc, u->number != 1)), size);
    assert(~bufp[0] & 0x80|| !"unicode/not implemented");
    ch = tolower(*(unsigned char *)bufp);
    bufp[0] = (char)ch;
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    *bufp = 0;

    return lbuf;
  }
  return LOC(loc, rc_name(rc, u->number != 1));
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
