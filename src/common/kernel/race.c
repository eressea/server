/* vi: set ts=2:
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2000
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

race *
rc_new(const char * zName)
{
	char zBuffer[80];
	race * rc = calloc(sizeof(race), 1);
	sprintf(zBuffer, "%s", zName);
	rc->_name[0] = strdup(zBuffer);
	sprintf(zBuffer, "%s_p", zName);
	rc->_name[1] = strdup(zBuffer);
	sprintf(zBuffer, "%s_d", zName);
	rc->_name[2] = strdup(zBuffer);
	sprintf(zBuffer, "%s_x", zName);
	rc->_name[3] = strdup(zBuffer);
	rc->precombatspell = NO_SPELL;
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

/* required for old_race, do not change order! */
static const char * oldracenames[MAXRACES] = {
	"dwarf", "elf", "orc", "goblin", "human", "troll", "demon", "insect", "halfling", "cat", "aquarian", "uruk", "snotling",
	"undead", "illusion",
	"young dragon", "dragon", "wyrm", "ent", "catdragon", "dracoid",
	"special", "spell",
	"irongolem", "stone golem", "shadowdemon", "shadowmaster", "mountainguard", "alp",
	"toad",
	"braineater", "peasant",
	"wolf", "lynx", "tunnelworm", "eagle", "rat", "songdragon", "nymph", "unicorn",
	"direwolf", "ghost",
	"imp", "dreamcat", "fairy", "owl", "hellcat", "tiger", "dolphin", "giantturtle", "kraken", "sea serpent",
	"shadow knight",
	"centaur",
	"skeleton", "skeleton lord", "zombie", "juju-zombie", "ghoul", "ghast", "museumghost", "gnome",
	"template",
	"clone", "shadowdragon", "shadowbat", "nightmare", "vampunicorn"
};

/* magres, {3 namen},
 * heimat, rekr.kost, unterhalt, splitsize, weight,
 * hitpoints, damage, armor, at_default, df_default, at_bonus, df_bonus
 * Alc,Arm,Ber,Bog, Bur,Han,Hol,Kat, Kräu,Mag,Pfer,Rei, Rüs,Sbau,Hie,Seg,
 * Sta,Spi,Ste,Str, Tak,Tar,Unt,Waf, Wag,Wahr,Steu,Aus, WlK
 * nonplayer,use_armor,
 * flags,
 * battle_flags,
 * generate_name
 * */

void
set_show_item(faction *f, item_t i)
{
	attrib *a = a_add(&f->attribs, a_new(&at_showitem));
	a->data.v = (void*)olditemtype[i];
}

void
give_starting_equipment(struct region *r, struct unit *u)

{
	set_item(u, I_WOOD, 30);
	set_item(u, I_STONE, 30);

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
			fset(u, FL_OWNER);
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
			fset(u, FL_OWNER);
		}
		set_level(u, SK_SAILING, 1);
		break;
	case RC_CENTAUR:
		rsethorses(r, 250+rand()%51+rand()%51);
		break;
	}

	set_money(u, 2000 + turn * 10);
}

int
unit_max_hp(const unit * u)
{
	int h;
	double p;
	h = u->race->hitpoints;

	p = pow(effskill(u, SK_AUSDAUER) / 2.0, 1.5) * 0.2;
	h += (int) (h * p + 0.5);

	if(fspecial(u->faction, FS_UNDEAD)) {
		h *= 2;
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
	if (rterrain(r)==T_GLACIER || rterrain(r)==T_ICEBERG_SLEEP
			|| rterrain(r)==T_ICEBERG)
		return true;

	return false;
}

const char *
rc_name(const race * rc, int n)
{
	return mkname("race", rc->_name[n]);
}

const char *
racename(const locale *loc, const unit *u, const race * rc)
{
	static char lbuf[80];
	attrib *a, *a2;


	a = a_find(u->attribs, &at_group);
	if(a) {
		a2 = a_find(((group *)(a->data.v))->attribs, &at_raceprefix);
	} else {
		a2 = a_find(u->faction->attribs, &at_raceprefix);
	}

	a = a_find(u->faction->attribs, &at_synonym);

	if(a2) {
		char s[32];

		strcpy(lbuf, locale_string(loc, (char *)a2->data.v));
		if(a) {
			strcpy(s, locale_string(loc,
				((frace_synonyms *)(a->data.v))->synonyms[u->number != 1]));
		} else {
			strcpy(s, LOC(loc, rc_name(rc, u->number != 1)));
		}
		s[0] = (char)tolower(s[0]);
		strcat(lbuf, s);
		return lbuf;
	}
	if(a) {
		return(locale_string(loc,
			((frace_synonyms *)(a->data.v))->synonyms[u->number != 1]));
	}
	return LOC(loc, rc_name(rc, u->number != 1));
}

static void
oldfamiliars(unit * familiar)
{
	sc_mage * m;
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
			addspell(familiar, SPL_FLEE);
			addspell(familiar, SPL_SLEEP);
			addspell(familiar, SPL_FRIGHTEN);
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
			addspell(familiar, SPL_SEDUCE);
			addspell(familiar, SPL_CALM_MONSTER);
			addspell(familiar, SPL_SONG_OF_CONFUSION);
			addspell(familiar, SPL_DENYATTACK);
			m->combatspell[0] = SPL_SONG_OF_CONFUSION;
			break;
		case RC_UNICORN:
			/* Mag+2, Spi, Tak, Tar+4, Wahr+4, Aus */
			set_level(familiar, SK_MAGIC, 1);
			set_level(familiar, SK_STEALTH, 1);
			set_level(familiar, SK_OBSERVATION, 1);
			m = create_mage(familiar, M_GRAU);
			addspell(familiar, SPL_RESISTMAGICBONUS);
			addspell(familiar, SPL_SONG_OF_PEACE);
			addspell(familiar, SPL_CALM_MONSTER);
			addspell(familiar, SPL_HERO);
			addspell(familiar, SPL_HEALINGSONG);
			addspell(familiar, SPL_DENYATTACK);
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
			addspell(familiar, SPL_STEALAURA);
			addspell(familiar, SPL_FRIGHTEN);
			addspell(familiar, SPL_SUMMONUNDEAD);
			break;
		case RC_IMP:
			/* Mag+1,Rei-1,Hie,Sta,Spi+1,Tar+1,Wahr+1,Steu+1,Aus*/
			set_level(familiar, SK_MAGIC, 1);
			set_level(familiar, SK_SPY, 1);
			set_level(familiar, SK_STEALTH, 1);
			set_level(familiar, SK_OBSERVATION, 1);
			set_level(familiar, SK_TAXING, 1);
			m = create_mage(familiar, M_GRAU);
			addspell(familiar, SPL_STEALAURA);
			break;
		case RC_DREAMCAT:
			/* Mag+1,Hie,Sta,Spi+1,Tar+1,Wahr+1,Steu+1,Aus*/
			set_level(familiar, SK_MAGIC, 1);
			set_level(familiar, SK_SPY, 1);
			set_level(familiar, SK_STEALTH, 1);
			set_level(familiar, SK_OBSERVATION, 1);
			set_level(familiar, SK_TAXING, 1);
			m = create_mage(familiar, M_GRAU);
			addspell(familiar, SPL_ILL_SHAPESHIFT);
			addspell(familiar, SPL_TRANSFERAURA_TRAUM);
			break;
		case RC_FEY:
			/* Mag+1,Rei-1,Hie-1,Sta-1,Spi+2,Tar+5,Wahr+2,Aus */
			set_level(familiar, SK_MAGIC, 1);
			m = create_mage(familiar,M_GRAU);
			addspell(familiar, SPL_DENYATTACK);
			addspell(familiar, SPL_CALM_MONSTER);
			addspell(familiar, SPL_SEDUCE);
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
	switch (art) {
	case RC_ELF:
		if (wtype!=NULL && fval(wtype, WTF_BOW)) {
			return 1;
		}
		break;
	case RC_HALFLING:
		if (wtype!=NULL && dragonrace(dr)) {
			return 5;
		}
		break;
	}
	return 0;
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

#include <xml.h>
#include <log.h>
/* callbacks */

typedef struct xml_state {
	struct race * race;
	int nextfamiliar;
	int nextattack;
} xml_state;

static int
tagbegin(struct xml_stack * stack)
{
	const xml_tag * tag = stack->tag;
	if (strcmp(tag->name, "races")==0) {
		stack->state = calloc(sizeof(xml_state), 1);
	} else {
		xml_state * state = (xml_state*)stack->state;
		if (strcmp(tag->name, "race")==0) {
			const char * zName = xml_value(tag, "name");
			race * rc;

			state->nextattack = 0;
			state->nextfamiliar = 0;

			if (zName) {
				rc = rc_find(zName);
				if (rc==NULL) {
					rc = rc_add(rc_new(zName));
				}
			} else {
				log_error(("missing required tag 'name'\n"));
				return XML_USERERROR;
			}
			rc->magres = xml_fvalue(tag, "magres");
			rc->maxaura = xml_fvalue(tag, "maxaura");
			rc->regaura = xml_fvalue(tag, "regaura");
			rc->recruitcost = xml_ivalue(tag, "recruitcost");
			rc->maintenance = xml_ivalue(tag, "maintenance");
			rc->weight = xml_ivalue(tag, "weight");
#if RACE_CAPACITY
			rc->capacity = xml_ivalue(tag, "capacity");
#endif
			rc->speed = xml_fvalue(tag, "speed");
			rc->hitpoints = xml_ivalue(tag, "hp");
			rc->def_damage = strdup(xml_value(tag, "damage"));
			rc->armor = (char)xml_ivalue(tag, "ac");

			rc->at_default = (char)xml_ivalue(tag, "unarmedattack");
			rc->df_default = (char)xml_ivalue(tag, "unarmeddefense");
			rc->at_bonus = (char)xml_ivalue(tag, "attackmodifier");
			rc->df_bonus = (char)xml_ivalue(tag, "defensemodifier");

			if (xml_bvalue(tag, "playerrace")) rc->flags |= RCF_PLAYERRACE;
			if (xml_bvalue(tag, "scarepeasants")) rc->flags |= RCF_SCAREPEASANTS;
			if (xml_bvalue(tag, "cannotmove")) rc->flags |= RCF_CANNOTMOVE;
			if (xml_bvalue(tag, "fly")) rc->flags |= RCF_FLY;
			if (xml_bvalue(tag, "swim")) rc->flags |= RCF_SWIM;
			if (xml_bvalue(tag, "walk")) rc->flags |= RCF_WALK;
			if (xml_bvalue(tag, "nolearn")) rc->flags |= RCF_NOLEARN;
			if (xml_bvalue(tag, "noteach")) rc->flags |= RCF_NOTEACH;
			if (xml_bvalue(tag, "horse")) rc->flags |= RCF_HORSE;
			if (xml_bvalue(tag, "desert")) rc->flags |= RCF_DESERT;
			if (xml_bvalue(tag, "absorbpeasants")) rc->flags |= RCF_ABSORBPEASANTS;
			if (xml_bvalue(tag, "noheal")) rc->flags |= RCF_NOHEAL;
			if (xml_bvalue(tag, "noweapons")) rc->flags |= RCF_NOWEAPONS;
			if (xml_bvalue(tag, "shapeshift")) rc->flags |= RCF_SHAPESHIFT;
			if (xml_bvalue(tag, "shapeshiftany")) rc->flags |= RCF_SHAPESHIFTANY;
			if (xml_bvalue(tag, "illusionary")) rc->flags |= RCF_ILLUSIONARY;
			if (xml_bvalue(tag, "undead")) rc->flags |= RCF_UNDEAD;
			if (xml_bvalue(tag, "dragon")) rc->flags |= RCF_DRAGON;

			if (xml_bvalue(tag, "nogive")) rc->ec_flags |= NOGIVE;
			if (xml_bvalue(tag, "giveitem")) rc->ec_flags |= GIVEITEM;
			if (xml_bvalue(tag, "giveperson")) rc->ec_flags |= GIVEPERSON;
			if (xml_bvalue(tag, "giveunit")) rc->ec_flags |= GIVEUNIT;
			if (xml_bvalue(tag, "getitem")) rc->ec_flags |= GETITEM;
			if (xml_bvalue(tag, "canguard")) rc->ec_flags |= CANGUARD;
			if (xml_bvalue(tag, "recruithorses")) rc->ec_flags |= ECF_REC_HORSES;
			if (xml_bvalue(tag, "recruitethereal")) rc->ec_flags |= ECF_REC_ETHEREAL;
			if (xml_bvalue(tag, "recruitunlimited")) rc->ec_flags |= ECF_REC_UNLIMITED;

			if (xml_bvalue(tag, "equipment")) rc->battle_flags |= BF_EQUIPMENT;
			if (xml_bvalue(tag, "noblock")) rc->battle_flags |= BF_NOBLOCK;
			if (xml_bvalue(tag, "invinciblenonmagic")) rc->battle_flags |= BF_INV_NONMAGIC;
			if (xml_bvalue(tag, "resistbash")) rc->battle_flags |= BF_RES_BASH;
			if (xml_bvalue(tag, "resistcut")) rc->battle_flags |= BF_RES_CUT;
			if (xml_bvalue(tag, "resistpierce")) rc->battle_flags |= BF_RES_PIERCE;
			

			state->race = rc;
		} else if (strcmp(tag->name, "ai")==0) {
			race * rc = state->race;
			rc->splitsize = xml_ivalue(tag, "splitsize");

			if (xml_bvalue(tag, "killpeasants")) rc->flags |= RCF_KILLPEASANTS;
			if (xml_bvalue(tag, "attackrandom")) rc->flags |= RCF_ATTACKRANDOM;
			if (xml_bvalue(tag, "moverandom")) rc->flags |= RCF_MOVERANDOM;
			if (xml_bvalue(tag, "learn")) rc->flags |= RCF_LEARN;

		} else if (strcmp(tag->name, "skill")==0) {
			race * rc = state->race;
			const char * name = xml_value(tag, "name");
			if (name) {
				int mod = xml_ivalue(tag, "modifier");
				if (mod!=0) {
					skill_t sk = sk_find(name);
					if (sk!=NOSKILL) {
						rc->bonus[sk] = (char)mod;
					} else {
						log_error(("unknown skill '%s'\n", name));
					}
				}
			} else {
				log_error(("missing required tag 'name'\n"));
				return XML_USERERROR;
			}
		} else if (strcmp(tag->name, "attack")==0) {
			race * rc = state->race;
			const char * damage = xml_value(tag, "damage");
			struct att * a = &rc->attack[state->nextattack++];
			if (damage) {
				a->data.dice = strdup(damage);
			} else {
				a->data.iparam = xml_ivalue(tag, "spell");
			}
			a->type = xml_ivalue(tag, "type");
			a->flags = xml_ivalue(tag, "flags");
		} else if (strcmp(tag->name, "precombatspell") == 0) {
			race * rc = state->race;
			rc->precombatspell = xml_ivalue(tag, "spell");
		} else if (strcmp(tag->name, "function")==0) {
			race * rc = state->race;
			const char * name = xml_value(tag, "name");
			const char * value = xml_value(tag, "value");
			if (name && value) {
				pf_generic fun = get_function(value);
				if (fun==NULL) {
					log_error(("unknown function value '%s=%s' for race %s\n", name, value, rc->_name[0]));
				} else {
					if (strcmp(name, "name")==0) {
						rc->generate_name = (const char* (*)(const struct unit*))fun;
					} else if (strcmp(name, "age")==0) {
						rc->age = (void(*)(struct unit*))fun;
					} else if (strcmp(name, "move")==0) {
						rc->move_allowed = (boolean(*)(const struct region *, const struct region *))fun;
					} else if (strcmp(name, "itemdrop")==0) {
						rc->itemdrop = (struct item *(*)(const struct race *, int))fun;
					} else if (strcmp(name, "initfamiliar")==0) {
						rc->init_familiar = (void(*)(struct unit *))fun;
					} else {
						log_error(("unknown function type '%s=%s' for race %s\n", name, value, rc->_name[0]));
					}
				}
			}
		} else if (strcmp(tag->name, "familiar")==0) {
			race * rc = state->race;
			const char * zRace = xml_value(tag, "race");
			if (zRace && rc) {
				race * frc = rc_find(zRace);
				if (frc == NULL) {
					frc = rc_add(rc_new(zRace));
				}
				if (xml_bvalue(tag, "default")) {
					rc->familiars[0] = frc;
				} else {
					rc->familiars[++state->nextfamiliar] = frc;
				}
			} else {
				log_error(("missing required tag 'race'\n"));
				return XML_USERERROR;
			}
		}
	}
	return XML_OK;
}

static int
tagend(struct xml_stack * stack)
{
	const xml_tag * tag = stack->tag;
	if (strcmp(tag->name, "races")==0) {
		int i;
		for (i=0;i!=MAXRACES;++i) {
			race * rc = rc_find(oldracenames[i]);
			if (rc) {
				new_race[i] = rc;
				if (rc == new_race[RC_TROLL]) {
					a_add(&rc->attribs, make_skillmod(NOSKILL, SMF_RIDING, NULL, 0.0, -1));
				}
			}
		}
	} else if (strcmp(tag->name, "race")==0) {
		xml_state * state = (xml_state*)stack->state;
		state->race = NULL;
		state->nextfamiliar = 0;
		state->nextattack = 0;
	}
	return XML_OK;
}

static xml_callbacks xml_races = {
	tagbegin, tagend, NULL
};

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
	xml_register(&xml_races, "eressea races", 0);
}
