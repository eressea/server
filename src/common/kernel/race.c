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
#include "faction.h"
#include "item.h"
#include "magic.h"
#include "region.h"
#include "spell.h"
#include "unit.h"
#include "names.h"
#include "pathfinder.h"
#include "ship.h"
#include "skill.h"
#include "karma.h"
#include "group.h"

/* item includes */
#include <items/racespoils.h>


/* util includes */
#include <attrib.h>
#include <functions.h>

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
	strcpy(zBuffer, zName);
	rc->_name[0] = strdup(zBuffer);
	sprintf(zBuffer, "%s_p", zName);
	rc->_name[1] = strdup(zBuffer);
	sprintf(zBuffer, "%s_d", zName);
	rc->_name[2] = strdup(zBuffer);
	sprintf(zBuffer, "%s_x", zName);
	rc->_name[3] = strdup(zBuffer);
	rc->precombatspell = SPL_NOSPELL;
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
	if (src->terrain==T_GLACIER && target->terrain == T_OCEAN) return false;
	return allowed_fly(src, target);
}

const char *race_prefixes[] = {
	"prefix_Dunkel",
	"prefix_Licht",
	"prefix_Klein",
	"prefix_Hoch",
	"prefix_Huegel",
	"prefix_Berg",
	"prefix_Wald",
	"prefix_Sumpf",
	"prefix_Schnee",
	"prefix_Sonnen",
	"prefix_Mond",
	"prefix_See",
	"prefix_Tal",
	"prefix_Schatten",
	"prefix_Hoehlen",
	"prefix_Blut",
	"prefix_Wild",
	"prefix_Chaos",
	"prefix_Nacht",
	"prefix_Nebel",
	"prefix_Grau",
	"prefix_Frost",
	"prefix_Finster",
	"prefix_Duester",
	"prefix_flame",
	"prefix_ice",
	NULL
};

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

static item * equipment;
void add_equipment(const item_type * itype, int number)
{
  if (itype!=NULL) i_change(&equipment, itype, number);
}

void
give_starting_equipment(struct region *r, struct unit *u)
{
#ifdef NEW_STARTEQUIPMENT
  item * itm = equipment;
  while (itm!=NULL) {
    i_add(&u->items, i_new(itm->type, itm->number));
    itm=itm->next;
  }
#else
  const item_type * token = it_find("conquesttoken");
  if (token!=NULL) {
    i_add(&u->items, i_new(token, 1));
  }
  set_item(u, I_WOOD, 30);
  set_item(u, I_STONE, 30);
  set_money(u, 2000 + turn * 10);
#endif

	switch(old_race(u->race)) {
	case RC_DWARF:
		set_level(u, SK_SWORD, 1);
		set_item(u, I_AXE, 1);
		set_item(u, I_CHAIN_MAIL, 1);
		break;
	case RC_ELF:
		set_item(u, I_FEENSTIEFEL, 1);
		set_show_item(u->faction, I_FEENSTIEFEL);
		break;
	case RC_ORC:
	case RC_URUK:
		set_level(u, SK_SPEAR, 4);
		set_level(u, SK_SWORD, 4);
		set_level(u, SK_CROSSBOW, 4);
		set_level(u, SK_LONGBOW, 4);
		set_level(u, SK_CATAPULT, 4);
		break;
	case RC_GOBLIN:
		set_item(u, I_RING_OF_INVISIBILITY, 1);
		set_show_item(u->faction, I_RING_OF_INVISIBILITY);
		scale_number(u, 10);
		break;
	case RC_HUMAN:
		{
			building *b = new_building(bt_find("castle"), r, u->faction->locale);
			b->size = 10;
			u->building = b;
			fset(u, UFL_OWNER);
		}
		break;
	case RC_TROLL:
		set_level(u, SK_BUILDING, 1);
		set_level(u, SK_OBSERVATION, 3);
		set_item(u, I_STONE, 50);
		break;
	case RC_DAEMON:
		set_level(u, SK_AUSDAUER, 15);
		u->hp = unit_max_hp(u);
		break;
	case RC_INSECT:
	  /* TODO: Potion-Beschreibung ausgeben */
		i_change(&u->items, oldpotiontype[P_WARMTH]->itype, 9);
		break;
	case RC_HALFLING:
		set_level(u, SK_TRADE, 1);
		set_level(u, SK_RIDING, 2);
		set_item(u, I_HORSE, 2);
		set_item(u, I_WAGON, 1);
		set_item(u, I_BALM, 5);
		set_item(u, I_SPICES, 5);
		set_item(u, I_JEWELERY, 5);
		set_item(u, I_MYRRH, 5);
		set_item(u, I_OIL, 5);
		set_item(u, I_SILK, 5);
		set_item(u, I_INCENSE, 5);
		break;
	case RC_CAT:
		set_item(u, I_RING_OF_INVISIBILITY, 1);
		set_show_item(u->faction, I_RING_OF_INVISIBILITY);
		break;
	case RC_AQUARIAN:
		{
			ship *sh = new_ship(st_find("boat"), r);
			sh->size = sh->type->construction->maxsize;
			addlist(&r->ships, sh);
			u->ship = sh;
			fset(u, UFL_OWNER);
		}
		set_level(u, SK_SAILING, 1);
		break;
	case RC_CENTAUR:
		rsethorses(r, 250+rand()%51+rand()%51);
		break;
	}
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

	if(fspecial(u->faction, FS_UNDEAD)) {
		h *= 2;
	}

	/* der healing curse verändert die maximalen hp */
	if (heal_ct) {
		curse *c = get_curse(u->region->attribs, heal_ct);
		if (c) {
			h = (int) (h * (1.0+(curse_geteffect(c)/100)));
		}
	}

	return h;
}
/*
boolean is_undead(const unit *u)
{
	return u->race == RC_UNDEAD || u->race == RC_SKELETON
		|| u->race == RC_SKELETON_LORD || u->race == RC_ZOMBIE
		|| u->race == RC_ZOMBIE_LORD || u->race == RC_GHOUL
		|| u->race == RC_GHOUL_LORD;
}
*/
boolean
r_insectstalled(const region * r)
{
  switch (rterrain(r)) {
    case T_GLACIER:
    case T_ICEBERG_SLEEP:
    case T_ICEBERG:
  		return true;
    default:
      break;
  }
	return false;
}

const char *
rc_name(const race * rc, int n)
{
	return mkname("race", rc->_name[n]);
}

const char *
raceprefix(const unit *u)
{
	const attrib * agroup = a_findc(u->attribs, &at_group);
	const attrib * asource = u->faction->attribs;

	if (agroup!=NULL) asource = ((const group *)(agroup->data.v))->attribs;
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
		strcpy(lbuf, locale_string(loc, prefix));
		s += strlen(lbuf);
		if (asyn!=NULL) {
			strcpy(s, locale_string(loc,
				((frace_synonyms *)(asyn->data.v))->synonyms[u->number != 1]));
		} else {
			strcpy(s, LOC(loc, rc_name(rc, u->number != 1)));
		}
		s[0] = (char)tolower(s[0]);
		return lbuf;
	} else if (asyn!=NULL) {
		return(locale_string(loc,
			((frace_synonyms *)(asyn->data.v))->synonyms[u->number != 1]));
	}
	return LOC(loc, rc_name(rc, u->number != 1));
}

static void
oldfamiliars(unit * familiar)
{
	sc_mage * m = NULL;
	race_t frt = old_race(familiar->race);

	switch(frt) {
		case RC_HOUSECAT:
			/* Kräu+1, Mag, Pfer+1, Spi+3, Tar+3, Wahr+4, Aus */
			set_level(familiar, SK_MAGIC, 1);
			set_level(familiar, SK_SPY, 1);
			set_level(familiar, SK_STEALTH, 1);
			set_level(familiar, SK_OBSERVATION, 1);
			m = create_mage(familiar, M_GRAU);
			break;
		case RC_TUNNELWORM:
			/* Ber+50,Hol+50,Sbau+50,Aus+2*/
			set_level(familiar, SK_MAGIC, 1);
			set_level(familiar, SK_MINING, 1);
			set_level(familiar, SK_LUMBERJACK, 1);
			set_level(familiar, SK_ROAD_BUILDING, 1);
			set_level(familiar, SK_AUSDAUER, 1);
			m = create_mage(familiar, M_GRAU);
			break;
		case RC_EAGLE:
			/* Spi, Wahr+2, Aus */
			set_level(familiar, SK_MAGIC, 1);
			set_level(familiar, SK_OBSERVATION, 1);
			m = create_mage(familiar, M_GRAU);
			break;
		case RC_RAT:
			/* Spionage+5, Tarnung+4, Wahrnehmung+2, Ausdauer */
			set_level(familiar, SK_MAGIC, 1);
			set_level(familiar, SK_SPY, 1);
			set_level(familiar, SK_STEALTH, 1);
			set_level(familiar, SK_OBSERVATION, 1);
			set_level(familiar, SK_AUSDAUER, 1+rand()%8);
			/* set_number(familiar, 50+rand()%500+rand()%500); */
			m = create_mage(familiar, M_GRAU);
			break;
		case RC_PSEUDODRAGON:
			/* Magie+1, Spionage, Tarnung, Wahrnehmung, Ausdauer */
			m = create_mage(familiar, M_GRAU);
			set_level(familiar, SK_MAGIC, 1);
			m->combatspell[0] = SPL_FLEE;
			m->combatspell[1] = SPL_SLEEP;
			break;
		case RC_NYMPH:
			/* Alc, Arm, Bog+2, Han-2, Kräu+4, Mag+1, Pfer+5, Rei+5,
			 * Rüs-2, Sbau, Seg-2, Sta, Spi+2, Tak-2, Tar+3, Unt+10,
			 * Waf-2, Wag-2, Wahr+2, Steu-2, Aus-1 */
			set_level(familiar, SK_MAGIC, 1);
			set_level(familiar, SK_LONGBOW, 1);
			set_level(familiar, SK_HERBALISM, 1);
			set_level(familiar, SK_HORSE_TRAINING, 1);
			set_level(familiar, SK_RIDING, 1);
			set_level(familiar, SK_SPY, 1);
			set_level(familiar, SK_STEALTH, 1);
			set_level(familiar, SK_ENTERTAINMENT, 1);
			set_level(familiar, SK_OBSERVATION, 1);
			m = create_mage(familiar, M_GRAU);
			m->combatspell[0] = SPL_SONG_OF_CONFUSION;
			break;
		case RC_UNICORN:
			/* Mag+2, Spi, Tak, Tar+4, Wahr+4, Aus */
			set_level(familiar, SK_MAGIC, 1);
			set_level(familiar, SK_STEALTH, 1);
			set_level(familiar, SK_OBSERVATION, 1);
			m = create_mage(familiar, M_GRAU);
			break;
		case RC_WARG:
			/* Spi, Tak, Tar, Wahri+2, Aus */
			set_level(familiar, SK_MAGIC, 1);
			set_level(familiar, SK_OBSERVATION, 1);
			m = create_mage(familiar, M_GRAU);
			break;
		case RC_WRAITH:
			/* Mag+1, Rei-2, Hie, Sta, Spi, Tar, Wahr, Aus */
			set_level(familiar, SK_MAGIC, 1);
			m = create_mage(familiar, M_GRAU);
			break;
		case RC_IMP:
			/* Mag+1,Rei-1,Hie,Sta,Spi+1,Tar+1,Wahr+1,Steu+1,Aus*/
			set_level(familiar, SK_MAGIC, 1);
			set_level(familiar, SK_SPY, 1);
			set_level(familiar, SK_STEALTH, 1);
			set_level(familiar, SK_OBSERVATION, 1);
			set_level(familiar, SK_TAXING, 1);
			m = create_mage(familiar, M_GRAU);
			break;
		case RC_DREAMCAT:
			/* Mag+1,Hie,Sta,Spi+1,Tar+1,Wahr+1,Steu+1,Aus*/
			set_level(familiar, SK_MAGIC, 1);
			set_level(familiar, SK_SPY, 1);
			set_level(familiar, SK_STEALTH, 1);
			set_level(familiar, SK_OBSERVATION, 1);
			set_level(familiar, SK_TAXING, 1);
			m = create_mage(familiar, M_GRAU);
			break;
		case RC_FEY:
			/* Mag+1,Rei-1,Hie-1,Sta-1,Spi+2,Tar+5,Wahr+2,Aus */
			set_level(familiar, SK_MAGIC, 1);
			m = create_mage(familiar,M_GRAU);
			break;
		case RC_OWL:
			/* Spi+1,Tar+1,Wahr+5,Aus */
			set_level(familiar, SK_MAGIC, 1);
			set_level(familiar, SK_SPY, 1);
			set_level(familiar, SK_STEALTH, 1);
			set_level(familiar, SK_OBSERVATION, 1);
			m = create_mage(familiar,M_GRAU);
			break;
		case RC_HELLCAT:
			/* Spi, Tak, Tar, Wahr+1, Aus */
			set_level(familiar, SK_MAGIC, 1);
			set_level(familiar, SK_OBSERVATION, 1);
			m = create_mage(familiar,M_GRAU);
			break;
		case RC_TIGER:
			/* Spi, Tak, Tar, Wahr+1, Aus */
			set_level(familiar, SK_MAGIC, 1);
			set_level(familiar, SK_OBSERVATION, 1);
			m = create_mage(familiar,M_GRAU);
			break;
	}
  if (m!=NULL) {
    spell_list * fspells = familiarspells(familiar->race);
    while (fspells!=NULL) {
      addspell(familiar, fspells->data->id);
      fspells=fspells->next;
    }
  }
}

static item *
dragon_drops(const struct race * rc, int size)
{
	item * itm = NULL;
	switch (old_race(rc)) {
	case RC_FIREDRAGON:
		i_add(&itm, i_new(olditemtype[I_DRACHENBLUT], size));
		break;
	case RC_DRAGON:
		i_add(&itm, i_new(olditemtype[I_DRACHENBLUT], size * 4));
		i_add(&itm, i_new(olditemtype[I_DRAGONHEAD], size));
		break;
	case RC_WYRM:
		i_add(&itm, i_new(olditemtype[I_DRACHENBLUT], size * 10));
		i_add(&itm, i_new(olditemtype[I_DRAGONHEAD], size));
		break;
	case RC_SEASERPENT:
		i_add(&itm, i_new(olditemtype[I_DRACHENBLUT], size * 6));
		i_add(&itm, i_new(olditemtype[I_SEASERPENTHEAD], size));
		break;
	}
	return itm;
}

static item *
phoenix_drops(const struct race *rc, int size)
{
	item *itm = NULL;
	i_add(&itm, i_new(&it_phoenixfeather, size));
	return itm;
}

static item *
elf_spoil(const struct race * rc, int size)
{
	item * itm = NULL;
	if (rand()%100 < RACESPOILCHANCE){
		i_add(&itm, i_new(&it_elfspoil, size));
	}
	return itm;
}

static item *
demon_spoil(const struct race * rc, int size)
{
	item * itm = NULL;
	if (rand()%100 < RACESPOILCHANCE){
		i_add(&itm, i_new(&it_demonspoil, size));
	}
	return itm;
}

static item *
goblin_spoil(const struct race * rc, int size)
{
	item * itm = NULL;
	if (rand()%100 < RACESPOILCHANCE){
		i_add(&itm, i_new(&it_goblinspoil, size));
	}
	return itm;
}
static item *
dwarf_spoil(const struct race * rc, int size)
{
	item * itm = NULL;
	if (rand()%100 < RACESPOILCHANCE){
		i_add(&itm, i_new(&it_dwarfspoil, size));
	}
	return itm;
}
static item *
halfling_spoil(const struct race * rc, int size)
{
	item * itm = NULL;
	if (rand()%100 < RACESPOILCHANCE){
		i_add(&itm, i_new(&it_halflingspoil, size));
	}
	return itm;
}
static item *
human_spoil(const struct race * rc, int size)
{
	item * itm = NULL;
	if (rand()%100 < RACESPOILCHANCE){
		i_add(&itm, i_new(&it_humanspoil, size));
	}
	return itm;
}
static item *
aquarian_spoil(const struct race * rc, int size)
{
	item * itm = NULL;
	if (rand()%100 < RACESPOILCHANCE){
		i_add(&itm, i_new(&it_aquarianspoil, size));
	}
	return itm;
}
static item *
insect_spoil(const struct race * rc, int size)
{
	item * itm = NULL;
	if (rand()%100 < RACESPOILCHANCE){
		i_add(&itm, i_new(&it_insectspoil, size));
	}
	return itm;
}
static item *
cat_spoil(const struct race * rc, int size)
{
	item * itm = NULL;
	if (rand()%100 < RACESPOILCHANCE){
		i_add(&itm, i_new(&it_catspoil, size));
	}
	return itm;
}
static item *
orc_spoil(const struct race * rc, int size)
{
	item * itm = NULL;
	if (rand()%100 < RACESPOILCHANCE){
		i_add(&itm, i_new(&it_orcspoil, size));
	}
	return itm;
}
static item *
troll_spoil(const struct race * rc, int size)
{
	item * itm = NULL;
	if (rand()%100 < RACESPOILCHANCE){
		i_add(&itm, i_new(&it_trollspoil, size));
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
	register_function((pf_generic)dragon_drops, "dragondrops");
	register_function((pf_generic)phoenix_drops, "phoenixdrops");
	register_function((pf_generic)elf_spoil, "elfspoil");
	register_function((pf_generic)demon_spoil, "demonspoil");
	register_function((pf_generic)goblin_spoil, "goblinspoil");
	register_function((pf_generic)dwarf_spoil, "dwarfspoil");
	register_function((pf_generic)halfling_spoil, "halflingspoil");
	register_function((pf_generic)human_spoil, "humanspoil");
	register_function((pf_generic)aquarian_spoil, "aquarianspoil");
	register_function((pf_generic)insect_spoil, "insectspoil");
	register_function((pf_generic)cat_spoil, "catspoil");
	register_function((pf_generic)orc_spoil, "orcspoil");
	register_function((pf_generic)troll_spoil, "trollspoil");

	sprintf(zBuffer, "%s/races.xml", resourcepath());
}

/** familiars **/
typedef struct familiar_spells {
  struct familiar_spells * next;
  spell_list * spells;
  const race * familiar_race;
} familiar_spells;

static familiar_spells * racespells;

spell_list *
familiarspells(const race * rc)
{
  familiar_spells * fspells = racespells;
  while (fspells && rc!=fspells->familiar_race) {
    fspells = fspells->next;
  }
  if (fspells!=NULL) return fspells->spells;
  return NULL;
}

familiar_spells *
mkspells(const race * rc)
{
  familiar_spells * fspells;

  fspells = malloc(sizeof(familiar_spells));
  fspells->next = racespells;
  racespells = fspells;
  fspells->familiar_race = rc;
  fspells->spells = NULL;
  return fspells;
}

void 
init_familiarspells(void)
{
  familiar_spells * fspells;

  fspells = mkspells(new_race[RC_PSEUDODRAGON]);
  add_spelllist(&fspells->spells, find_spellbyid(SPL_FLEE));
  add_spelllist(&fspells->spells, find_spellbyid(SPL_SLEEP));
  add_spelllist(&fspells->spells, find_spellbyid(SPL_FRIGHTEN));

  fspells = mkspells(new_race[RC_NYMPH]);
  add_spelllist(&fspells->spells, find_spellbyid(SPL_SEDUCE));
  add_spelllist(&fspells->spells, find_spellbyid(SPL_CALM_MONSTER));
  add_spelllist(&fspells->spells, find_spellbyid(SPL_SONG_OF_CONFUSION));
  add_spelllist(&fspells->spells, find_spellbyid(SPL_DENYATTACK));

  fspells = mkspells(new_race[RC_NYMPH]);
  add_spelllist(&fspells->spells, find_spellbyid(SPL_SEDUCE));
  add_spelllist(&fspells->spells, find_spellbyid(SPL_CALM_MONSTER));
  add_spelllist(&fspells->spells, find_spellbyid(SPL_SONG_OF_CONFUSION));
  add_spelllist(&fspells->spells, find_spellbyid(SPL_DENYATTACK));
  
  fspells = mkspells(new_race[RC_UNICORN]);
  add_spelllist(&fspells->spells, find_spellbyid(SPL_RESISTMAGICBONUS));
  add_spelllist(&fspells->spells, find_spellbyid(SPL_SONG_OF_PEACE));
  add_spelllist(&fspells->spells, find_spellbyid(SPL_CALM_MONSTER));
  add_spelllist(&fspells->spells, find_spellbyid(SPL_HERO));
  add_spelllist(&fspells->spells, find_spellbyid(SPL_HEALINGSONG));
  add_spelllist(&fspells->spells, find_spellbyid(SPL_DENYATTACK));

  fspells = mkspells(new_race[RC_WRAITH]);
  add_spelllist(&fspells->spells, find_spellbyid(SPL_STEALAURA));
  add_spelllist(&fspells->spells, find_spellbyid(SPL_FRIGHTEN));
  add_spelllist(&fspells->spells, find_spellbyid(SPL_SUMMONUNDEAD));

  fspells = mkspells(new_race[RC_IMP]);
  add_spelllist(&fspells->spells, find_spellbyid(SPL_STEALAURA));

  fspells = mkspells(new_race[RC_DREAMCAT]);
  add_spelllist(&fspells->spells, find_spellbyid(SPL_ILL_SHAPESHIFT));
  add_spelllist(&fspells->spells, find_spellbyid(SPL_TRANSFERAURA_TRAUM));

  fspells = mkspells(new_race[RC_FEY]);
  add_spelllist(&fspells->spells, find_spellbyid(SPL_DENYATTACK));
  add_spelllist(&fspells->spells, find_spellbyid(SPL_CALM_MONSTER));
  add_spelllist(&fspells->spells, find_spellbyid(SPL_SEDUCE));
}

void 
init_races(void)
{
  init_familiarspells();
}
