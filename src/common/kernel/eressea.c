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

/* attributes includes */
#include <attributes/reduceproduction.h>
#include <attributes/otherfaction.h>
#include <attributes/racename.h>
#include <attributes/gm.h>

/* kernel includes */
#include "message.h"
#include "spell.h"
#include "names.h"
#include "faction.h"
#include "item.h"
#include "border.h"
#include "plane.h"
#include "alchemy.h"
#include "unit.h"
#include "save.h"
#include "magic.h"
#include "building.h"
#include "battle.h"
#include "race.h"
#include "pool.h"
#include "region.h"
#include "unit.h"
#include "skill.h"
#include "objtypes.h"
#include "ship.h"
#include "karma.h"
#include "group.h"

/* util includes */
#include <base36.h>
#include <event.h>
#include <umlaut.h>
#include <translation.h>
#include <crmessage.h>
#include <sql.h>
#include <xml.h>

/* libc includes */
#include <stdio.h>
#include <stdlib.h>
#include <message.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <errno.h>

/* exported variables */
region  *regions;
faction *factions;
settings global;
char     buf[BUFSIZE + 1];
FILE    *logfile;
const struct race * new_race[MAXRACES];

race_t 
old_race(const struct race * rc)
{
	race_t i;
	for (i=0;i!=MAXRACES;++i) {
		if (new_race[i]==rc) return i;
	}
	return NORACE;
}

const char *directions[MAXDIRECTIONS+2] =
{
	"northwest",
	"northeast",
	"east",
	"southeast",
	"southwest",
	"west",
	"",
	"pause"
};

const char *gr_prefix[3] = {
	"einem",
	"einer",
	"einem"
};

const char *parameters[MAXPARAMS] =
{
	"LOCALE",
	"ALLES",
	"BAUERN",
	"BURG",
	"EINHEIT",
	"PRIVAT",
	"HINTEN",
	"KOMMANDO",
	"KRÄUTER",
	"NICHT",
	"NÄCHSTER",
	"PARTEI",
	"ERESSEA",
	"PERSONEN",
	"REGION",
	"SCHIFF",
	"SILBER",
	"STRAßEN",
	"TEMPORÄRE",
	"FLIEHE",
	"GEBÄUDE",
	"GIB",			/* Für HELFE */
	"KÄMPFE",
	"BEWACHE",
	"ZAUBER",
	"PAUSE",
	"VORNE",
	"AGGRESSIV",
	"DEFENSIV",
	"STUFE",
	"HELFE",
	"FREMDES",
	"AURA",
	"UM",
	"BEISTAND",
	"GNADE",
	"HINTER",
	"VOR",
	"ANZAHL",
	"GEGENSTÄNDE",
	"TRÄNKE",
	"GRUPPE",
	"PARTEITARNUNG",
	"BÄUME"
};


const char *keywords[MAXKEYWORDS] =
{
	"//",
	"BANNER",
	"ARBEITEN",
	"ATTACKIEREN",
	"BIETEN",
	"BEKLAUEN",
	"BELAGERE",
	"BENENNEN",
	"BENUTZEN",
	"BESCHREIBE",
	"BETRETEN",
	"BEWACHEN",
	"BOTSCHAFT",
	"ENDE",
	"FAHREN",
	"NUMMER",
	"FOLGEN",
	"FORSCHEN",
	"GIB",
	"HELFEN",
	"KÄMPFEN",
	"KAMPFZAUBER",
	"KAUFEN",
	"KONTAKTIERE",
	"LEHREN",
	"LERNEN",
	"LIEFERE",
	"MACHEN",
	"NACH",
	"PASSWORT",
	"REGION",
	"REKRUTIEREN",
	"RESERVIEREN",
	"ROUTE",
	"SABOTIEREN",
	"OPTION",
	"SPIONIEREN",
	"STIRB",
	"TARNEN",
	"TRANSPORTIEREN",
	"TREIBEN",
	"UNTERHALTEN",
	"VERKAUFE",
	"VERLASSE",
	"VERGESSE",
	"ZAUBERE",
	"ZEIGEN",
	"ZERSTÖREN",
	"ZÜCHTEN",
	"DEFAULT",
	"REPORT",
	"URSPRUNG",
	"EMAIL",
	"MEINUNG",
	"MAGIEGEBIET",
	"PIRATERIE",
	"NEUSTART",
	"GRUPPE",
	"OPFERE",
	"BETEN",
	"SORTIEREN",
	"JIHAD",
	"GM",
	"INFO",
#ifdef USE_UGROUPS
	"JOINVERBAND",
	"LEAVEVERBAND",
#endif
	"PRÄFIX",
	"SYNONYM",
#if GROWING_TREES
	"PFLANZEN",
#endif
};

const char *report_options[MAX_MSG] =
{
	"Kampf",
	"Ereignisse",
	"Bewegung",
	"Einkommen",
	"Handel",
	"Produktion",
	"Orkvermehrung",
	"Zauber",
	"",
	""
};

const char *message_levels[ML_MAX] =
{
	"Wichtig",
	"Debug",
	"Fehler",
	"Warnungen",
	"Infos"
};

const char *options[MAXOPTIONS] =
{
	"AUSWERTUNG",
	"COMPUTER",
	"ZUGVORLAGE",
	"SILBERPOOL",
	"STATISTIK",
	"DEBUG",
	"ZIPPED",
	"ZEITUNG",				/* Option hat Sonderbehandlung! */
	"MATERIALPOOL",
	"ADRESSEN",
	"BZIP2",
	"PUNKTE"
};

int
max_skill(faction * f, skill_t skill)
{
	int m = INT_MAX;

	switch (skill) {
	case SK_MAGIC:
		m = MAXMAGICIANS;
		if (old_race(f->race) == RC_ELF) m += 1;
		m += fspecial(f, FS_MAGOCRACY) * 2;
		break;
	case SK_ALCHEMY:
		m = MAXALCHEMISTS;
		break;
	}

	return m;
}

char * g_basedir;
char * g_resourcedir;

const char *
basepath(void)
{
	if (g_basedir) return g_basedir;
	return ".";
}

const char *
resourcepath(void)
{
	static char zText[MAX_PATH];
	if (g_resourcedir) return g_resourcedir;
	return strcat(strcpy(zText, basepath()), "/res");
}

int
count_all_money(const region * r)
{
	const unit *u;
	int m = rmoney(r);

	for (u = r->units; u; u = u->next)
		m += get_money(u);

	return m;
}

int
count_skill(faction * f, skill_t skill)
{
	int n = 0;
	region *r;
	unit *u;
	region *last = lastregion(f);

	for (r = firstregion(f); r != last; r = r->next)
		for (u = r->units; u; u = u->next)
			if (u->faction == f && get_skill(u, skill) > 0)
				if(!is_familiar(u))
					n += u->number;

	return n;
}

int
shipcapacity (const ship * sh)
{
	int i;

	/* sonst ist construction:: size nicht ship_type::maxsize */
	assert(!sh->type->construction || sh->type->construction->improvement==NULL);

	if (sh->type->construction && sh->size!=sh->type->construction->maxsize)
		return 0;

#ifdef SHIPDAMAGE
	i = ((sh->size * DAMAGE_SCALE - sh->damage) / DAMAGE_SCALE)
		* sh->type->cargo / sh->size;
	i += ((sh->size * DAMAGE_SCALE - sh->damage) % DAMAGE_SCALE)
		* sh->type->cargo / (sh->size*DAMAGE_SCALE);
#else
	i = sh->type->cargo;
#endif
	return i;
}

int max_unique_id;
int quiet = 0;

FILE *debug;

int
shipspeed (ship * sh, const unit * u)
{
	int k = sh->type->range;
	assert(u->ship==sh);
	assert(sh->type->construction->improvement==NULL); /* sonst ist construction::size nicht ship_type::maxsize */
	if (sh->size!=sh->type->construction->maxsize) return 0;

	if( is_cursed(sh->attribs, C_SHIP_SPEEDUP, 0) )
			k *= 2;
	if( is_cursed(sh->attribs, C_SHIP_NODRIFT, 0) )
			k += 1;

	if (old_race(u->faction->race) == RC_AQUARIAN
			&& old_race(u->race) == RC_AQUARIAN)
		k += 1;
#ifdef SHIPSPEED
	k *= SHIPSPEED;
#endif

#ifdef SHIPDAMAGE
	if (sh->damage) k = (k * (sh->size * DAMAGE_SCALE - sh->damage) + sh->size * DAMAGE_SCALE- 1) / (sh->size*DAMAGE_SCALE);
#endif

	return min (k, MAXSPEED);
}

/* erwartete Anzahl Einheiten x 2 */
#define UMAXHASH 199999
unit *unithash[UMAXHASH];

void
uhash (unit * u)
{
	assert(!u->nexthash || !"unit ist bereits gehasht");
	u->nexthash = unithash[u->no % UMAXHASH];
	unithash[u->no % UMAXHASH] = u;
}

void
uunhash (unit * u)
{
	unit ** x = &(unithash[u->no % UMAXHASH]);
	while (*x && *x!=u) x = &(*x)->nexthash;
	assert(*x || !"unit nicht gefunden");
	*x = u->nexthash;
	u->nexthash=NULL;
}

unit *
ufindhash (int i)
{
	unit * u = unithash[i % UMAXHASH];
	while (u && u->no!=i) u = u->nexthash;
	return u;
}

#define FMAXHASH 2047
faction * factionhash[FMAXHASH];

static void
fhash(faction * f)
{
	int index = f->no % FMAXHASH;
	f->nexthash = factionhash[index];
	factionhash[index] = f;
}

static void
funhash(faction * f)
{
	int index = f->no % FMAXHASH;
	faction ** fp = factionhash+index;
	while (*fp && (*fp)!=f) fp = &(*fp)->nexthash;
	*fp = f->nexthash;
}

static faction *
ffindhash(int no)
{
	int index = no % FMAXHASH;
	faction * f = factionhash[index];
	while (f && f->no!=no) f = f->nexthash;
	return f;
}
/* ----------------------------------------------------------------------- */

void
stripfaction (faction * f)
{
#ifdef OLD_MESSAGES
	free_messages(f->msgs);
#else
	/* TODO */
#endif
	/* TODO: free msgs */
	freestrlist(f->mistakes);
	freelist(f->allies);
	free(f->email);
	free(f->banner);
	free(f->passw);
	free(f->name);
#ifndef FAST_REGION
	vset_destroy(&f->regions);
#endif
	while (f->attribs) a_remove (&f->attribs, f->attribs);
	freelist(f->ursprung);
	funhash(f);
}

void
stripunit(unit * u)
{
	free(u->name);
	free(u->display);
	free(u->thisorder);
	free(u->lastorder);
	freestrlist(u->orders);
	freestrlist(u->botschaften);
	if(u->skills) free(u->skills);
	while (u->items) {
		item * it = u->items->next;
		u->items->next = NULL;
		i_free(u->items);
		u->items = it;
	}
#ifdef OLD_TRIGGER
	change_all_pointers(u, TYP_UNIT, NULL);	/* vor Zerstoeren der Attribs! */
#endif
	while (u->attribs) a_remove (&u->attribs, u->attribs);
}

void
verify_data (void)
{
#ifndef NDEBUG
	int lf = -1;
	faction *f;
	unit *u;
	int mage, alchemist;

	puts(" - Überprüfe Daten auf Korrektheit...");

	list_foreach(faction, factions, f) {
		mage = 0;
		alchemist = 0;
		for (u=f->units;u;u=u->nextF) {
			if (eff_skill(u, SK_MAGIC, u->region)) {
				mage += u->number;
			}
			if (eff_skill(u, SK_ALCHEMY, u->region))
				alchemist += u->number;
			if (u->number > 50000 || u->number < 0) {
				if (lf != f->no) {
					lf = f->no;
					printf("Partei %s:\n", factionid(f));
				}
				log_warning(("Einheit %s hat %d Personen\n", unitid(u), u->number));
			}
		}
		if (f->no != 0 && ((mage > 3 && old_race(f->race) != RC_ELF) || mage > 4))
			log_error(("Partei %s hat %d Magier.\n", factionid(f), mage));
		if (alchemist > 3)
			log_error(("Partei %s hat %d Alchemisten.\n", factionid(f), alchemist));
	}
	list_next(f);
#endif
}

int
get_skill (const unit * u, skill_t id)
{
	skillvalue *i = u->skills;

	for (; i != u->skills + u->skill_size; ++i)
		if (i->id == id)
			return i->value;
	return 0;
}

int
distribute (int old, int new_value, int n)
{
	int i;
	int t;
	assert(new_value <= old);

	if (old == 0)
		return 0;

	t = (n / old) * new_value;
	for (i = (n % old); i; i--)
		if (rand() % old < new_value)
			t++;

	return t;
}

int
change_hitpoints (unit * u, int value)
{
	int hp = u->hp;

	hp += value;

	/* Jede Person benötigt mindestens 1 HP */
	if (hp < u->number){
		if (hp < 0){ /* Einheit tot */
			hp = 0;
		}
		scale_number(u, hp);
	}
	u->hp = hp;
	return hp;
}

int
atoip (const char *s)
{
	int n;

	n = atoi (s);

	if (n < 0)
		n = 0;

	return n;
}

void
scat (const char *s)
{
	strncat (buf, s, BUFSIZE - strlen (buf));
}

void
icat (int n)
{
	char s[12];

	sprintf (s, "%d", n);
	scat (s);
}


region *
findunitregion (const unit * su)
{
#ifndef SLOW_REGION
	return su->region;
#else
	region *r;
	const unit *u;

	for (r = regions; r; r = r->next) {
		for (u = r->units; u; u = u->next) {
			if (su == u) {
	return r;
			}
		}
	}

	/* This should never happen */
	assert (!"Die unit wurde nicht gefunden");

	return (region *) NULL;
#endif
}

int
effskill(const unit * u, skill_t sk)
{
	return eff_skill(u, sk, u->region);
}

int
effstealth(const unit * u)
{
	int e, es;

	/* Auf dem Ozean keine Tarnung! */
	if (u->region->terrain == T_OCEAN) return 0;

	e = effskill(u, SK_STEALTH);

	es = u_geteffstealth(u);
	if (es >=0 && es < e) return es;
	return e;
}

int
eff_stealth (const unit * u, const region * r)
{
	int e, es;

	e = eff_skill (u, SK_STEALTH, r);

	es = u_geteffstealth(u);
	if (es >=0 && es < e) return es;
	return e;
}

void
scale_number (unit * u, int n)
{
	skill_t skill;
	const attrib * a;
	int remain;

	if (n == u->number) return;
	if (n && u->number) {
		int full;
		remain = ((u->hp%u->number) * (n % u->number)) % u->number;

		full = u->hp/u->number; /* wieviel kriegt jede person mindestens */
		u->hp = full * n + (u->hp-full*u->number) * n / u->number;
		assert(u->hp>=0);
		if ((rand() % u->number) < remain)
			++u->hp;	/* Nachkommastellen */
	} else {
		remain = 0;
		u->hp = 0;
	}
	for (a = a_find(u->attribs, &at_effect);a;a=a->nexttype) {
		effect_data * data = (effect_data *)a->data.v;
		int snew = data->value / u->number * n;
		if (n) {
			remain = data->value - snew / n * u->number;
			snew += remain * n / u->number;
			remain = (remain * n) % u->number;
			if ((rand() % u->number) < remain)
				++snew;	/* Nachkommastellen */
		}
		data->value = snew;
	}
	for (skill = 0; skill < MAXSKILLS; skill++) {
		if (n==0 || u->number == 0) {
			set_skill(u, skill, 0);
		} else {
			int sval = get_skill(u, skill);
			int snew = sval / u->number * n;
			remain = sval - snew / n * u->number;
			snew += remain * n / u->number;
			remain = (remain * n) % u->number;
			if ((rand() % u->number) < remain)
				++snew;	/* Nachkommastellen */
			set_skill(u, skill, snew);
		}
	}

	set_number(u, n);
}

boolean
unit_has_cursed_item(unit *u)
{
	item * itm = u->items;
	while (itm) {
		if (fval(itm->type, ITF_CURSED) && itm->number>0) return true;
		itm=itm->next;
	}
	return false;
}

/* f hat zu f2 HELFE mode gesetzt */
int
isallied(const plane * pl, const faction * f, const faction * f2, int mode)
{
	ally *sf;
	attrib * a;

	if (f == f2) return mode;
	if (f2==NULL) return 0;

	a = a_find(f->attribs, &at_gm);
	while (a) {
		plane * p = (plane*)a->data.v;
		if (p==pl) return mode;
		a=a->next;
	}

	if (pl && pl->flags & PFL_FRIENDLY) return mode;
	if (mode != HELP_GIVE && pl && (pl->flags & PFL_NOALLIANCES)) return 0;
	for (sf = f->allies; sf; sf = sf->next)
		if (sf->faction == f2)
			return (sf->status & mode);

	return 0;
}

int
alliance(const ally * sf, const faction * f, int mode)
{
	while (sf) {
		if (sf->faction == f)
			return sf->status & mode;
		sf = sf->next;
	}
	return 0;
}

/* Die Gruppe von Einheit u hat helfe zu f2 gesetzt. */
int
allied(const unit * u, const faction * f2, int mode)
{
	ally * sf;
	const attrib * a;
	plane * pl;

	if (u->faction == f2) return mode;
	if (u->faction == NULL || f2==NULL) return 0;

	sf = u->faction->allies;
	pl = getplane(u->region);

	if (pl && pl->flags & PFL_FRIENDLY) return mode;

	/* if f2 is a gm in this plane, everyone has an auto-help to it */
	a = a_find(f2->attribs, &at_gm);
	while (a) {
		plane * p = (plane*)a->data.v;
		if (p==pl) return mode;
		a=a->next;
	}

	if (mode != HELP_GIVE && pl && (pl->flags & PFL_NOALLIANCES))
		return 0;
	a = a_find(u->attribs, &at_group);
	if (a) sf = ((group*)a->data.v)->allies;
	return alliance(sf, f2, mode);
}

boolean
seefaction(const faction * f, const region * r, const unit * u, int modifier)
{
	if (((f == u->faction) || !fval(u, FL_PARTEITARNUNG)) && cansee(f, r, u, modifier))
		return true;
	return false;
}

boolean
cansee(const faction * f, const region * r, const unit * u, int modifier)
	/* r kann != u->region sein, wenn es um durchreisen geht */
	/* und es muss niemand aus f in der region sein, wenn sie vom Turm
	 * erblickt wird */
{
	int n;
	boolean cansee = false;
	unit *u2;
	if (u->faction == f || omniscient(f)) cansee = true;
	else if (old_race(u->race) == RC_SPELL || u->number == 0) return false;
	else {
		n = eff_stealth(u, r) - modifier;
		for (u2 = r->units; u2; u2 = u2->next) {
			if (u2->faction == f) {
				int o;

				if (getguard(u) || usiege(u) || u->building || u->ship) {
					cansee = true;
					break;
				}
				if (get_item(u, I_RING_OF_INVISIBILITY) >= u->number
					&& !get_item(u2, I_AMULET_OF_TRUE_SEEING))
					continue;

				o = eff_skill(u2, SK_OBSERVATION, r);
#if NIGHTEYES
				if (u2->enchanted == SP_NIGHT_EYES && o < NIGHT_EYE_TALENT)
					o = NIGHT_EYE_TALENT;
#endif
				if (o >= n) {
					cansee = true;
					break;
				}
			}
		}
	}
	return cansee;
}

#if 0
boolean
cansee(faction * f, region * r, unit * u, int modifier)
/* r kann != u->region sein, wenn es um durchreisen geht */
/* und es muss niemand aus f in der region sein, wenn sie vom Turm
 * erblickt wird */
{
	boolean cansee = false;
	unit *u2;
#if FAST_CANSEE
	static region * lastr = NULL;
	static faction * lastf = NULL;
	static int maxskill = INT_MIN;
#endif
	int n = 0;
	boolean ring = false;
	if (u->faction == f || omniscient(f)) return true;
#if FAST_CANSEE /* buggy */
	if (lastr==r && lastf==f) {
		n = eff_stealth(u, r) - modifier;
		if (n<=maxskill) return true;
	}
	else {
		lastf=f;
		lastr=r;
		maxskill=INT_MIN;
	}
#endif
	if (old_race(u->race) == RC_SPELL || u->number == 0) return false;
	else {
		boolean xcheck = false;
		int o = INT_MIN;
		ring = (boolean)(ring || (get_item(u, I_RING_OF_INVISIBILITY) >= u->number));
		n = n || eff_stealth(u, r) - modifier;
		for (u2 = r->units; u2; u2 = u2->next) {
			if (u2->faction == f) {

				if (!xcheck && (getguard(u) || usiege(u) || u->building || u->ship)) {
					cansee = true;
					break;
				}
				else
					xcheck = true;
				if (ring && !get_item(u2, I_AMULET_OF_TRUE_SEEING))
					continue;

				o = eff_skill(u2, SK_OBSERVATION, r);
#if NIGHTEYES
				if (u2->enchanted == SP_NIGHT_EYES && o < NIGHT_EYE_TALENT)
					o = NIGHT_EYE_TALENT;
#endif
				if (o >= n) {
					cansee = true;
					break;
				}
			}
		}
#if FAST_CANSEE
		maxskill = o;
#endif
	}
	return cansee;
}
#endif

boolean
cansee_durchgezogen(const faction * f, const region * r, const unit * u, int modifier)
/* r kann != u->region sein, wenn es um durchreisen geht */
/* und es muss niemand aus f in der region sein, wenn sie vom Turm
 * erblickt wird */
{
	int n;
	boolean cansee = false;
	unit *u2;
	if (old_race(u->race) == RC_SPELL || u->number == 0) return false;
	else if (u->faction == f) cansee = true;
	else {

		n = eff_stealth(u, r) - modifier;

		for (u2 = r->units; u2; u2 = u2->next){
			if (u2->faction == f) {
				int o;

				if (getguard(u) || usiege(u) || u->building || u->ship) {
					cansee = true;
					break;
				}
				if (get_item(u, I_RING_OF_INVISIBILITY) >= u->number
						&& !get_item(u2, I_AMULET_OF_TRUE_SEEING))
					continue;

				o = eff_skill(u2, SK_OBSERVATION, r);

#if NIGHTEYES
				if (u2->enchanted == SP_NIGHT_EYES && o < NIGHT_EYE_TALENT)
					o = NIGHT_EYE_TALENT;
#endif
				if (o >= n) {
					cansee = true;
					break;
				}
			}
		}
		if (getguard(u) || usiege(u) || u->building || u->ship) {
			cansee = true;
		}
	}
	return cansee;
}

#ifndef NDEBUG
const char *
strcheck (const char *s, size_t maxlen)
{
	static char buffer[16 * 1024];
	if (strlen(s) > maxlen) {
		assert(maxlen < 16 * 1024);
		log_warning(("[strcheck] String wurde auf %d Zeichen verkürzt:\n%s\n",
				(int)maxlen, s));
		strncpy(buffer, s, maxlen);
		buffer[maxlen] = 0;
		return buffer;
	}
	return s;
}
#endif
/* Zaehlfunktionen fuer Typen ------------------------------------- */

int
teure_talente (unit * u)
{
	if (get_skill (u, SK_MAGIC) > 0 || get_skill (u, SK_ALCHEMY) > 0 ||
			get_skill (u, SK_TACTICS) > 0 || get_skill (u, SK_HERBALISM) > 0 ||
			get_skill (u, SK_SPY) > 0) {
		return 0;
	} else {
		return 1;
	}
}

attrib_type at_lighthouse = {
	"lighthouse"
	/* Rest ist NULL; temporäres, nicht alterndes Attribut */
};

/* update_lighthouse: call this function whenever the size of a lighthouse changes
 * it adds temporary markers to the surrounding regions.
 * The existence of markers says nothing about the quality of the observer in
 * the lighthouse, for this may change more frequently.
 */
void
update_lighthouse(building * lh)
{
	region * r = lh->region;
	int d = (int)log10(lh->size) + 1;
	int x, y;

	for (x=-d;x<=d;++x) {
		for (y=-d;y<=d;++y) {
			attrib * a;
			region * r2 = findregion(x+r->x, y+r->y);
			if (rterrain(r2)!=T_OCEAN) continue;
			if (!r2 || distance(r, r2) > d) continue;
			a = a_find(r2->attribs, &at_lighthouse);
			while (a) {
				building * b = (building*)a->data.v;
				if (b==lh) break;
				a=a->nexttype;
			}
			if (!a) {
				a = a_add(&r2->attribs, a_new(&at_lighthouse));
				a->data.v = (void*)lh;
			}
		}
	}
}

int
count_all(const faction * f)
{
	int n = 0;
	unit *u;
	for (u=f->units;u;u=u->nextF)
		if (playerrace(u->race)) {
			n += u->number;
			assert(f==u->faction);
		}
	return n;
}

int
count_maxmigrants(const faction * f)
{
	int x = 0;
	if (old_race(f->race) == RC_HUMAN) {
		x = (int)(log10(count_all(f) / 50.0) * 20);
		if (x < 0) x = 0;
	}
	return x;
}

/*------------------------------------------------------------------*/

/* GET STR, I zur Eingabe von Daten liest diese aus dem Buffer, der beim ersten
 * Aufruf inititialisiert wird? */

char *
igetstrtoken (const char *s1)
{
	int i;
	static const char *s;
	static char lbuf[DISPLAYSIZE + 1];

	if (s1) s = s1;
	while (*s == ' ')
		s++;
	i = 0;

	while (*s && *s != ' ' && i < DISPLAYSIZE) {
		lbuf[i] = (*s);

		/* Hier wird space_replacement wieder in space zurueck
		 * verwandelt, ausser wenn es nach einem escape_char kommt. Im
		 * letzteren Fall wird escape_char durch space_replacement
		 * ersetzt, statt den aktuellen char einfach dran zu haengen. */

		if (*s == SPACE_REPLACEMENT) {
			if (i > 0 && lbuf[i - 1] == ESCAPE_CHAR)
				lbuf[--i] = SPACE_REPLACEMENT;
			else
				lbuf[i] = SPACE;
		}
		i++;
		s++;
	}

	lbuf[i] = 0;
	return lbuf;
}

char *
getstrtoken (void)
{
	return igetstrtoken (0);
}

int
geti (void)
{
	return atoip (getstrtoken ());
}

/* GET KEYWORD, SKILL, ITEM, SPELL benutzen FINDSTR - welche Item um Item eine
 * Liste durchsucht.
 *
 * FIND wird immer von GET aufgerufen. GET braucht keine Parameter, IGET braucht
 * einen String aus dem gelesen wird. In FIND stehen dann listen etc drinnen.
 * FIND kann man auch allein verwenden, wenn der string _nur_ noch das gesuchte
 * object enthaelt. Steht noch weitere info darin, sollte man GET verwenden,
 * bzw. GETI wenn die info am Anfang eines neuen Stringes steht. */

int
findstr(const char **v, const char *s, unsigned char n)
{
	int i;
	size_t ss = strlen(s);
	if (!ss)
		return -1;
	for (i = 0; i != n; i++)
		if (!strncasecmp(s, v[i], ss))
			return i;
	return -1;
}

enum {
	UT_NONE,
	UT_PARAM,
	UT_ITEM,
	UT_BUILDING,
	UT_HERB,
	UT_POTION,
	UT_MAX
};

static struct lstr {
	const struct locale * lang;
	struct tnode tokens[UT_MAX];
	struct tnode skillnames;
	struct tnode keywords;
	struct tnode races;
	struct tnode directions;
	struct tnode options;
	struct lstr * next;
} * lstrs;

static struct lstr *
get_lnames(const struct locale * lang)
{
	static struct lstr * lnames = NULL;
	static const struct locale * lastlang = NULL;

	if (lastlang!=lang || lnames==NULL) {
		lnames = lstrs;
		while (lnames && lnames->lang!=lang) lnames = lnames->next;
		if (lnames==NULL) {
			lnames = calloc(sizeof(struct lstr), 1);
			lnames->lang = lang;
			lnames->next = lstrs;
			lstrs = lnames;
		}
	}
	return lnames;
}

const struct race * 
findrace(const char * s, const struct locale * lang)
{
	struct lstr * lnames = get_lnames(lang);
	const struct race * rc;

	if (findtoken(&lnames->races, s, (void **)&rc)==E_TOK_SUCCESS) {
		return rc;
	}
	return NULL;
}

int
findoption(const char *s, const struct locale * lang)
{
	struct lstr * lnames = get_lnames(lang);
	int dir;

	if (findtoken(&lnames->options, s, (void**)&dir)==E_TOK_SUCCESS) {
		return (direction_t)dir;
	}
	return NODIRECTION;
}

skill_t
findskill(const char *s, const struct locale * lang)
{
	struct lstr * lnames = get_lnames(lang);
	int i;

	if (findtoken(&lnames->skillnames, s, (void**)&i)==E_TOK_NOMATCH) return NOSKILL;
	return (skill_t)i;
}

keyword_t
findkeyword(const char *s, const struct locale * lang)
{
	struct lstr * lnames = get_lnames(lang);
	int i;
#ifdef AT_PERSISTENT
	if(*s == '@') s++;
#endif
	if (findtoken(&lnames->keywords, s, (void**)&i)==E_TOK_NOMATCH) return NOKEYWORD;
	if (global.disabled[i]) return NOKEYWORD;
	return (keyword_t) i;
}

keyword_t
igetkeyword (const char *s, const struct locale * lang)
{
	return findkeyword (igetstrtoken (s), lang);
}

keyword_t
getkeyword (const struct locale * lang)
{
	return findkeyword (getstrtoken (), lang);
}

param_t
findparam(const char *s, const struct locale * lang)
{
	struct lstr * lnames = get_lnames(lang);
	const building_type * btype;

	int i;
	if (findtoken(&lnames->tokens[UT_PARAM], s, (void**)&i)==E_TOK_NOMATCH) {
		btype = findbuildingtype(s, lang);
		if (btype!=NULL) return (param_t) P_BUILDING;
		return NOPARAM;
	}
	return (param_t)i;
}

param_t
igetparam (const char *s, const struct locale *lang)
{
	return findparam (igetstrtoken (s), lang);
}
param_t
getparam (const struct locale * lang)
{
	return findparam (getstrtoken (), lang);
}

#ifdef FUZZY_BASE36
extern int fuzzy_hits;
boolean enable_fuzzy = false;
#endif /* FUZZY_BASE36 */

faction *
findfaction (int n)
{
	faction * f;

	f = ffindhash(n);
	if (f) return f;
	for (f = factions; f; f = f->next)
		if (f->no == n) {
			fhash(f);
			return f;
		}
#ifdef FUZZY_BASE36
	if(enable_fuzzy) {
		n = atoi(itoa36(n));
		if (n) {
			f = ffindhash(n);
			if (f) return f;
			for (f = factions; f; f = f->next) {
				if (f->no == n) {
					fhash(f);
					return (f);
				}
			}
		}
	}
#endif /* FUZZY_BASE36 */
	/* Gibt komische Seiteneffekte hier! */
	/* if (n==MONSTER_FACTION) return makemonsters(); */
	return NULL;
}

faction *
getfaction (void)
{
	return findfaction (getid());
}

faction *
findfaction_unique_id (int unique_id)
{
	faction *f;

	for (f = factions; f; f = f->next)
		if (f->unique_id == unique_id) {
			return f;
		}
	return NULL;
}

unit *
findunitr (const region * r, int n)
{
	unit *u;

	/* findunit regional! */

	for (u = r->units; u; u = u->next)
		if (u->no == n)
			return u;

	return 0;
}

unit *findunit(int n)
{
	return findunitg(n, NULL);
}

unit *
findunitg (int n, const region * hint)
{

	/* Abfangen von Syntaxfehlern. */
	if(n <= 0)
		return NULL;

	/* findunit global! */
	hint = 0;
	return ufindhash (n);
}

unit *
getnewunit (const region * r, const faction * f)
{
	int n;
	n = getid();

	return findnewunit (r, f, n);
}

int
read_newunitid (const faction * f, const region * r)
{
	int n;
	unit *u2;
	n = getid();
	if (n == 0)
		return -1;

	u2 = findnewunit(r, f, n);
	if (u2) return u2->no;

	return -1;
}

int
read_unitid (const faction * f, const region * r)
{
	char *s;

	s = getstrtoken ();

	/* Da s nun nur einen string enthaelt, suchen wir ihn direkt in der
	 * paramliste. machen wir das nicht, dann wird getnewunit in s nach der
	 * nummer suchen, doch dort steht bei temp-units nur "temp" drinnen! */

	switch (findparam (s, f->locale)) {
	case P_TEMP:
		return read_newunitid(f, r);
	}
	if (!s || *s == 0)
		return -1;
	return atoi36(s);
}

/* exported symbol */
boolean getunitpeasants;
unit *
getunitg(const region * r, const faction * f)
{
	int n;
	getunitpeasants = 0;

	n = read_unitid(f, r);

	if (n == 0) {
		getunitpeasants = 1;
		return NULL;
	}

	if (n < 0) return 0;

	return findunit(n);
}

unit *
getunit(const region * r, const faction * f)
{
	int n;
	unit *u2;
	getunitpeasants = 0;

	n = read_unitid(f, r);

	if (n == 0) {
		getunitpeasants = 1;
		return NULL;
	}

	if (n < 0) return 0;

	for (u2 = r->units; u2; u2 = u2->next) {
		if (u2->no == n && !fval(u2, FL_ISNEW)) {
			return u2;
		}
	}

	return 0;
}
/* - String Listen --------------------------------------------- */

strlist *
makestrlist (const char *s)
{
	strlist *S;
	S = malloc(sizeof(strlist));
	S->s = strdup(s);
	S->next = NULL;
	return S;
}

void
addstrlist (strlist ** SP, const char *s)
{
	addlist(SP, makestrlist(s));
}

void
freestrlist (strlist * s)
{
	strlist *q, *p = s;
	while (p) {
		q = p->next;
		free(p->s);
		free(p);
		p = q;
	}
}

/* - Meldungen und Fehler ------------------------------------------------- */

boolean nomsg = false;

/* - Namen der Strukturen -------------------------------------- */
typedef char name[OBJECTIDSIZE + 1];
static name idbuf[8];
static int nextbuf = 0;

char *
estring(const char *s)
{
	char *buf = idbuf[(++nextbuf) % 8];
	char *r;

	strcpy(buf,s);
	r = buf;

	while(*buf) {
		if(*buf == ' ') {
			*buf = '~';
		}
		buf++;
	}
	return r;
}

char *
cstring(const char *s)
{
	char *buf = idbuf[(++nextbuf) % 8];
	char *r;

	strcpy(buf,s);
	r = buf;

	while(*buf) {
		if(*buf == '~') {
			*buf = ' ';
		}
		buf++;
	}
	return r;
}

const char *
regionid(const region * r)
{
	char	*buf = idbuf[(++nextbuf) % 8];

	if (!r) {
		strcpy(buf, "(Chaos)");
	} else {
		sprintf(buf, "\\r(%d,%d)", r->x, r->y);
	}

	return buf;
}

const char *
buildingname (const building * b)
{
	char *buf = idbuf[(++nextbuf) % 8];

	sprintf(buf, "%s (%s)", strcheck(b->name, NAMESIZE), itoa36(b->no));
	return buf;
}

building *
largestbuilding (const region * r, boolean img)
{
	const building_type * btype = &bt_castle; /* TODO: parameter der funktion? */
	building *b, *best = NULL;
	/* durch die verw. von '>' statt '>=' werden die aelteren burgen
	 * bevorzugt. */

	for (b = rbuildings(r); b; b = b->next) {
		if (b->type!=btype) {
			if (img) {
				const attrib * a = a_find(b->attribs, &at_icastle);
				if (!a) continue;
				if (a->data.v != btype) continue;
			} else continue;
		}
		if (best==NULL || b->size > best->size)
			best = b;
	}
	return best;
}

const char *
unitname(const unit * u)
{
	char *ubuf = idbuf[(++nextbuf) % 8];
	sprintf(ubuf, "%s (%s)", strcheck(u->name, NAMESIZE), itoa36(u->no));
	return ubuf;
}

char *
xunitid(const unit *u)
{
	char *buf = idbuf[(++nextbuf) % 8];
	sprintf(buf, "%s in %s", unitname(u), regionid(u->region));
	return buf;
}

/* -- Erschaffung neuer Einheiten ------------------------------ */

extern faction * dfindhash(int i);

const char* forbidden[] = { "t", "te", "tem", "temp", NULL };

int
forbiddenid(int id)
{
	static int * forbid = NULL;
	static size_t len;
	size_t i;
	if (id<=0) return 1;
	if (!forbid) {
		while (forbidden[len]) ++len;
		forbid = calloc(len, sizeof(int));
		for (i=0;i!=len;++i) {
			forbid[i] = strtol(forbidden[i], NULL, 36);
		}
	}
	for (i=0;i!=len;++i) if (id==forbid[i]) return 1;
	return 0;
}

/* ID's für Einheiten und Zauber */
int
newunitid(void)
{
	int random_unit_no;
	int start_random_no;
	random_unit_no = 1 + (rand() % MAX_UNIT_NR);
	start_random_no = random_unit_no;

	while (ufindhash(random_unit_no) || dfindhash(random_unit_no)
			|| cfindhash(random_unit_no)
			|| forbiddenid(random_unit_no))
	{
		random_unit_no++;
		if (random_unit_no == MAX_UNIT_NR + 1) {
			random_unit_no = 1;
		}
		if (random_unit_no == start_random_no) {
			random_unit_no = (int) MAX_UNIT_NR + 1;
		}
	}
	return random_unit_no;
}

int
newcontainerid(void)
{
	int random_no;
	int start_random_no;

	random_no = 1 + (rand() % MAX_CONTAINER_NR);
	start_random_no = random_no;

	while (findship(random_no) || findbuilding(random_no)) {
		random_no++;
		if (random_no == MAX_CONTAINER_NR + 1) {
			random_no = 1;
		}
		if (random_no == start_random_no) {
			random_no = (int) MAX_CONTAINER_NR + 1;
		}
	}
	return random_no;
}

void
createunitid(unit *u, int id)
{
	if (id<=0 || id > MAX_UNIT_NR || ufindhash(id) || dfindhash(id) || forbiddenid(id))
		u->no = newunitid();
	else
		u->no = id;
	uhash(u);
}

unit *
createunit(region * r, faction * f, int number, const struct race * rc)
{
	return create_unit(r, f, number, rc, 0, NULL, NULL);
}

unit *
create_unit(region * r, faction * f, int number, const struct race *urace, int id, const char * dname, unit *creator)
{
	unit * u = calloc(1, sizeof(unit));

	assert(f->alive);
	u_setfaction(u, f);
	set_string(&u->thisorder, "");
	set_string(&u->lastorder, LOC(u->faction->locale, "defaultorder"));
	u_seteffstealth(u, -1);
	u->race = urace;
	u->irace = urace;

	set_number(u, number);

	/* die nummer der neuen einheit muss vor name_unit generiert werden,
	 * da der default name immer noch 'Nummer u->no' ist */
	createunitid(u, id);

	/* zuerst in die Region setzen, da zb Drachennamen den Regionsnamen
	 * enthalten */
	move_unit(u, r, NULL);

	/* u->race muss bereits gesetzt sein, wird für default-hp gebraucht */
	/* u->region auch */
	u->hp = unit_max_hp(u) * number;

	if (!dname) {
		name_unit(u);
	}
	else set_string(&u->name, dname);
	set_string(&u->display, "");

	/* Nicht zu der Einheitenzahl zählen sollten auch alle Monster. Da
	 * aber auf die MAXUNITS nur in MACHE TEMP geprüft wird, ist es egal */
	if(!fval(u->race, RCF_UNDEAD)) {
		f->no_units++;
	}

	if (creator){
		attrib * a;

		/* erbt Kampfstatus */
		u->status = creator->status;

		/* erbt Gebäude/Schiff*/
		if (creator->region==r) {
			u->building = creator->building;
			u->ship = creator->ship;
		}

		/* Temps von parteigetarnten Einheiten sind wieder parteigetarnt */
		if (fval(creator, FL_PARTEITARNUNG))
			fset(u, FL_PARTEITARNUNG);

		/* Daemonentarnung */
		set_racename(&u->attribs, get_racename(creator->attribs));
		if (fval(creator->race, RCF_SHAPESHIFT)) {
			u->irace = creator->irace;
		}

		/* Gruppen */
		a = a_find(creator->attribs, &at_group);
		if (a) {
			group * g = (group*)a->data.v;
			a_add(&u->attribs, a_new(&at_group))->data.v = g;
		}
		a = a_find(creator->attribs, &at_otherfaction);
		if (a){
			attrib *an = a_add(&u->attribs, a_new(&at_otherfaction));
			an->data.i = a->data.i;
		}
	}
	/* Monster sind grundsätzlich parteigetarnt */
	if(f->no <= 0) fset(u, FL_PARTEITARNUNG);

	return u;
}

/* Setzt Default Befehle -------------------------------------- */

boolean
idle (faction * f)
{
	return (boolean) (f ? false : true);
}


int
maxworkingpeasants(const struct region * r)
{
#if GROWING_TREES
	int i = production(r) * MAXPEASANTS_PER_AREA
		- ((rtrees(r,2)+rtrees(r,1)/2) * TREESIZE);
#else
	int i = production(r) * MAXPEASANTS_PER_AREA - rtrees(r) * TREESIZE;
#endif
	return max(i, 0);
}

boolean
check_leuchtturm(region * r, faction * f)
{
	attrib * a;

	if (rterrain(r) != T_OCEAN) return false;

	for (a = a_find(r->attribs, &at_lighthouse);a;a=a->nexttype) {
		building *b = (building *)a->data.v;
		region *r2 = b->region;

		assert(b->type == &bt_lighthouse);
		if (fval(b, BLD_WORKING) && b->size >= 10) {
			int c = 0;
			unit *u;
			int d = 0;
			int maxd = (int)log10(b->size) + 1;

			for (u = r2->units; u; u = u->next) {
				if (u->building == b) {
					c += u->number;
					if (c > buildingcapacity(b)) break;
					if (f==NULL || u->faction == f) {
						if (!d) d = distance(r, r2);
						if (maxd < d) break;
						if (eff_skill(u, SK_OBSERVATION, r) >= d * 3) return true;
					}
				} else if (c) break; /* first unit that's no longer in the house ends the search */
			}
		}
	}

	return false;
}

region *
lastregion (faction * f)
{
	region *r = f->last;
	if (!r && f->units) {
		for (r = firstregion(f); r; r = r->next) {
			unit *u;
			attrib *ru;
			for (u = r->units; u; u = u->next) {
				if (u->faction == f) {
					f->last = r->next;
					break;
				}
			}
			if (f->last == r->next)
				continue;
			for (ru = a_find(r->attribs, &at_travelunit); ru; ru = ru->nexttype) {
				u = (unit*)ru->data.v;
				if (u->faction == f) {
					f->last = r->next;
					break;
				}
			}
			if (f->last == r->next)
				continue;
			if (check_leuchtturm(r, f))
				f->last = r->next;
		}
	}
	return f->last;
}

region *
firstregion (faction * f)
{
	region *r;
	if (f->first || !f->units)
		return f->first;

	for (r = regions; r; r = r->next) {
		attrib *ru;
		unit *u;
		for (u = r->units; u; u = u->next) {
			if (u->faction == f) {
				f->first = r;
				return r;
			}
		}
		if (f->first == r->next)
			continue;
		for (ru = a_find(r->attribs, &at_travelunit); ru; ru = ru->nexttype) {
			u = (unit*)ru->data.v;
			if (u->faction == f) {
				f->first = r;
				return r;
			}
		}
		if (check_leuchtturm(r, f)) {
			f->first = r;
			return r;
		}
	}
	f->first = regions;
	return regions;
}

void ** blk_list[1024];
int list_index;
int blk_index;

void
gc_done(void)
{
	int i, k;
	for (i=0;i!=list_index;++i)
	{
		for (k=0;k!=1024;++k) free(blk_list[i][k]);
		free(blk_list[i]);
	}
	for (k=0;k!=blk_index;++k) free(blk_list[list_index][k]);
	free(blk_list[list_index]);

}

void *
gc_add(void * p)
{
	if (blk_index==0) {
		blk_list[list_index] = (void**)malloc(1024 * sizeof(void*));
	}
	blk_list[list_index][blk_index] = p;
	blk_index = (blk_index+1) % 1024;
	if (!blk_index) ++ list_index;
	return p;
}

const char *
findorder(const unit * u, const char * cmd)
{
	strlist * o;
	char * c;
	for (o=u->orders;o;o=o->next) {
		if (!strcmp(cmd, o->s)) {
			if (o->s==cmd)
				return cmd;
			return o->s;
		}
	}
	c = strdup(cmd);
	gc_add(c);
	return c;
}

void
use_birthdayamulet(region * r, unit * magician, strlist * cmdstrings)
{
	region *tr;
	direction_t d;

	unused(cmdstrings);
	unused(magician);

	for(d=0;d<MAXDIRECTIONS;d++) {
		tr = rconnect(r, d);
		if(tr) addmessage(tr, 0, "Miiauuuuuu...", MSG_MESSAGE, ML_IMPORTANT);
	}

	tr = r;
	addmessage(r, 0, "Miiauuuuuu...", MSG_MESSAGE, ML_IMPORTANT);
}

typedef struct t_umlaut {
	const char *txt;
	int id;
	int typ;
} t_umlaut;

/* Hier sind zum einen Umlaut-Versionen von Schlüsselworten, aber auch
 * oftmals Umlaut-umschriebene Worte, weil im "normalen" Source die
 * Umlaut-Version steht, damit die im Report erscheint.
 * WICHTIG: "setenv LANG en_US" sonst ist ä != Ä
 */

#if 0
static const t_umlaut umlaut[] = {
/* Parameter */
	{ "Straßen", P_ROAD, UT_PARAM },
/* Gegenstände - alternative Namen */
	{ "Eisenbarren", I_IRON, UT_ITEM },
	{ "Holzstamm", I_WOOD, UT_ITEM },
	{ "Holzstämme", I_WOOD, UT_ITEM },
	{ "Stämme", I_WOOD, UT_ITEM },
	{ "Stamm", I_WOOD, UT_ITEM },
	{ "Quader", I_STONE, UT_ITEM },
	{ "Steinquader", I_STONE, UT_ITEM },
	{ "Langbogen", I_LONGBOW, UT_ITEM },
	{ "Langbögen", I_LONGBOW, UT_ITEM },
	{ "Hemden", I_CHAIN_MAIL, UT_ITEM },
	{ "Panzer", I_PLATE_ARMOR, UT_ITEM },
	{ "Gewürze", I_SPICES, UT_ITEM },
	{ "Öle", I_OIL, UT_ITEM },
	{ "Sehens", I_AMULET_OF_TRUE_SEEING, UT_ITEM },
	{ "Heilung", I_AMULET_OF_HEALING, UT_ITEM },
	{ "Unsichtbarkeit", I_RING_OF_INVISIBILITY, UT_ITEM },
	{ "Macht", I_RING_OF_POWER, UT_ITEM },
#ifdef COMPATIBILITY
	{ "Einhornaugen", I_EYE_OF_HORAX, UT_ITEM },
	{ "Reisekristall", I_TELEPORTCRYSTAL, UT_ITEM },
	{ "Dunkelheit", I_AMULET_OF_DARKNESS, UT_ITEM },
#endif
	{ "Kopf", I_DRAGONHEAD, UT_ITEM },
	{ "Köpfe", I_DRAGONHEAD, UT_ITEM },
	{ "Keuschheitsamulett", I_CHASTITY_BELT, UT_ITEM },
	{ "Zweihänder", I_GREATSWORD, UT_ITEM },
	{ "Axt", I_AXE, UT_ITEM },
	{ "Äxte", I_AXE, UT_ITEM },
	{ "Treffens", I_AMULETT_DES_TREFFENS, UT_ITEM },
	{ "Flinkfingerring", I_RING_OF_NIMBLEFINGER, UT_ITEM },
	{ NULL, 0, 0 }
};
#endif

static void
init_directions(tnode * root, const struct locale * lang)
{
	/* mit dieser routine kann man mehrere namen für eine direction geben,
	 * das ist für die hexes ideal. */
	const struct {
		const char* name;
		int direction;
	} dirs [] = {
		{ "dir_ne", D_NORTHEAST},
		{ "dir_nw", D_NORTHWEST},
		{ "dir_se", D_SOUTHEAST},
		{ "dir_sw", D_SOUTHWEST},
		{ "dir_east", D_EAST},
		{ "dir_west", D_WEST},
		{ "northeast", D_NORTHEAST},
		{ "northwest", D_NORTHWEST},
		{ "southeast", D_SOUTHEAST},
		{ "southwest", D_SOUTHWEST},
		{ "east", D_EAST },
		{ "west",D_WEST },
		{ "PAUSE", D_PAUSE },
		{ NULL, NODIRECTION}
	};
	int i;
	struct lstr * lnames = get_lnames(lang);
	for (i=0; dirs[i].direction!=NODIRECTION;++i) {
		addtoken(&lnames->directions, LOC(lang, dirs[i].name), (void*)dirs[i].direction);
	}
}

direction_t
finddirection(const char *s, const struct locale * lang)
{
	struct lstr * lnames = get_lnames(lang);
	int dir;

	if (findtoken(&lnames->directions, s, (void**)&dir)==E_TOK_SUCCESS) {
		return (direction_t)dir;
	}
	return NODIRECTION;
}

static void
init_tokens(const struct locale * lang)
{
	struct lstr * lnames = get_lnames(lang);
	int i;
	const struct race * rc;

	init_directions(&lnames->directions, lang);
	for (rc=races;rc;rc=rc->next) {
		addtoken(&lnames->races, LOC(lang, rc_name(rc, 1)), (void*)rc);
	}
	for (i=0;i!=MAXPARAMS;++i)
		addtoken(&lnames->tokens[UT_PARAM], LOC(lang, parameters[i]), (void*)i);
	for (i=0;i!=MAXSKILLS;++i)
		addtoken(&lnames->skillnames, skillname(i, lang), (void*)i);
	for (i=0;i!=MAXKEYWORDS;++i)
		addtoken(&lnames->keywords, LOC(lang, keywords[i]), (void*)i);
	for (i=0;i!=MAXOPTIONS;++i)
		addtoken(&lnames->options, LOC(lang, options[i]), (void*)i);
#if 0
	for (i=0;umlaut[i].txt;++i)
		addtoken(&lnames->tokens[umlaut[i].typ], umlaut[i].txt, (void*)umlaut[i].id);
#endif
}

void
kernel_done(void)
{
	/* calling this function releases memory assigned to static variables, etc.
	 * calling it is optional, e.g. a release server will most likely not do it.
	 */
	translation_done();
	skill_done();
	gc_done();
	sql_done();
}

static void
read_strings(FILE * F)
{
	char rbuf[8192];
	while (fgets(rbuf, sizeof(rbuf), F)) {
		char * b = rbuf;
		locale * lang;
		char * key = b;
		char * language;
		const char * k;

		if (rbuf[0]=='#') continue;
		rbuf[strlen(rbuf)-1] = 0; /* \n weg */
		while (*b && *b!=';') ++b;
		if (!*b) continue;
		*b++ = 0;
		language = b;
		while (*b && *b!=';') ++b;
		*b++ = 0;
		lang = find_locale(language);
		if (!lang) lang = make_locale(language);
		k = locale_getstring(lang, key);
		if (k) {
			log_warning(("Trying to register %s[%s]=\"%s\", already have \"%s\"\n", key, language, k, b));
		} else locale_setstring(lang, key, b);
	}
}

const char * messages[] = {
	"%s/%s/strings.xml",
	"%s/%s/messages.xml",
	NULL
};

const char * strings[] = {
	"%s/%s/strings.txt",
	NULL
};

const char * locales[] = {
	"de", "en",
	NULL
};

static int read_xml(const char * filename);

static int 
parse_tagbegin(struct xml_stack *stack, void *data)
{
	const xml_tag * tag = stack->tag;
	if (strcmp(tag->name, "include")==0) {
		const char * filename = xml_value(tag, "file");
		if (filename) {
			return read_xml(filename);
		} else {
			log_printf("required tag 'file' missing from include");
			return XML_USERERROR;
		}
	} else if (strcmp(tag->name, "game")==0) {
		const char * welcome = xml_value(tag, "welcome");
		const char * name = xml_value(tag, "name");
		int maxunits = xml_ivalue(tag, "units");
		if (welcome!=NULL) {
			global.welcomepath = strdup(welcome);
		}
		if (name!=NULL) {
			global.gamename = strdup(name);
		}
		if (maxunits!=0) {
			global.maxunits = maxunits;
		}
	} else if (strcmp(tag->name, "game")==0) {
	} else if (strcmp(tag->name, "order")==0) {
		const char * name = xml_value(tag, "name");
		if (xml_bvalue(tag, "disable")) {
			int k;
			for (k=0;k!=MAXKEYWORDS;++k) {
				if (strncmp(keywords[k], name, strlen(name))==0) {
					global.disabled[k]=0;
					break;
				}
			}
		}
	} else if (strcmp(tag->name, "resource")==0) {
		xml_readresource(stack->stream, stack);
	} else if (strcmp(tag->name, "races")==0) {
		read_races(stack->stream, stack);
	} else if (strcmp(tag->name, "strings")==0) {
		return read_messages(stack->stream, stack);
	} else if (strcmp(tag->name, "messages")==0) {
		return read_messages(stack->stream, stack);
	}
	return XML_OK;
}

static xml_callbacks msgcallback = {
	NULL,
	parse_tagbegin,
	NULL,
	NULL
};

static int
read_xml(const char * filename)
{
	char zText[80];
	FILE * F;
	int i;
	sprintf(zText, "%s/%s", resourcepath(), filename);
	F = fopen(zText, "r+");
	if (F==NULL) {
		log_printf("could not open %s: %s\n", zText, strerror(errno));
		return XML_USERERROR;
	}

	i = xml_parse(F, &msgcallback, NULL, NULL);
	fclose(F);
	return i;
}

int
init_data(const char * filename)
{
	int l = read_xml(filename);

	if (l) return l;

	/* old stuff, for removal: */
	for (l=0;locales[l];++l) {
		char zText[MAX_PATH];
		int i;
		for (i=0;strings[i];++i) {
			FILE * F;
			sprintf(zText, strings[i], resourcepath(), locales[l]);
			F = fopen(zText, "r+");
			if (F) {
				read_strings(F);
				fclose(F);
			} else {
				sprintf(buf, "fopen(%s): ", zText);
				perror(buf);
				return 1;
			}
		}
	}
	return 0;
}

void
init_locales(void)
{
	int l;
	for (l=0;locales[l];++l) {
		const struct locale * lang = find_locale(locales[l]);
		if (lang) init_tokens(lang);
	}
}

/* TODO: soll hier weg */
extern building_type bt_caldera;
extern attrib_type at_traveldir_new;

attrib_type at_germs = {
	"germs",
	DEFAULT_INIT,
	DEFAULT_FINALIZE,
	DEFAULT_AGE,
	DEFAULT_WRITE,
	DEFAULT_READ,
	ATF_UNIQUE
};

void
attrib_init(void)
{
	/* Gebäudetypen registrieren */
	init_buildings();
	bt_register(&bt_castle);
	bt_register(&bt_lighthouse);
	bt_register(&bt_mine);
	bt_register(&bt_quarry);
	bt_register(&bt_harbour);
	bt_register(&bt_academy);
	bt_register(&bt_magictower);
	bt_register(&bt_smithy);
	bt_register(&bt_sawmill);
	bt_register(&bt_stables);
	bt_register(&bt_monument);
	bt_register(&bt_dam);
	bt_register(&bt_caravan);
	bt_register(&bt_tunnel);
	bt_register(&bt_inn);
	bt_register(&bt_stonecircle);
	bt_register(&bt_blessedstonecircle);
	bt_register(&bt_illusion);
	bt_register(&bt_generic);
	bt_register(&bt_caldera);

	/* Schiffstypen registrieren: */
	st_register(&st_boat);
	st_register(&st_balloon);
	st_register(&st_longboat);
	st_register(&st_dragonship);
	st_register(&st_caravelle);
	st_register(&st_trireme);

	/* disable: st_register(&st_transport); */

	/* Alle speicherbaren Attribute müssen hier registriert werden */
	at_register(&at_unitdissolve);
	at_register(&at_traveldir_new);
	at_register(&at_familiar);
	at_register(&at_familiarmage);
	at_register(&at_clone);
	at_register(&at_clonemage);
	at_register(&at_eventhandler);
	at_register(&at_stealth);
	at_register(&at_mage);
	at_register(&at_bauernblut);
	at_register(&at_countdown);
	at_register(&at_showitem);
	at_register(&at_curse);
	at_register(&at_cursewall);

	at_register(&at_seenspell);
	at_register(&at_reportspell);
	at_register(&at_deathcloud);

	/* neue REGION-Attribute */
	at_register(&at_direction);
	at_register(&at_moveblock);
#if AT_SALARY
	at_register(&at_salary);
#endif
	at_register(&at_horseluck);
	at_register(&at_peasantluck);
	at_register(&at_deathcount);
	at_register(&at_chaoscount);
	at_register(&at_woodcount);
	at_register(&at_road);

	/* neue UNIT-Attribute */
	at_register(&at_alias);
	at_register(&at_siege);
	at_register(&at_target);
	at_register(&at_potion);
	at_register(&at_potionuser);
	at_register(&at_contact);
	at_register(&at_effect);
	at_register(&at_private);

#if defined(OLD_TRIGGER)
	at_register(&at_pointer_tag);
	at_register(&at_relation);
	at_register(&at_relbackref);
	at_register(&at_trigger);
	at_register(&at_action);
#endif
	at_register(&at_icastle);
	at_register(&at_guard);
	at_register(&at_lighthouse);
	at_register(&at_group);
	at_register(&at_faction_special);
	at_register(&at_prayer_timeout);
	at_register(&at_prayer_effect);
	at_register(&at_wyrm);
	at_register(&at_building_generic_type);

/* border-typen */
	register_bordertype(&bt_noway);
	register_bordertype(&bt_fogwall);
	register_bordertype(&bt_wall);
	register_bordertype(&bt_illusionwall);
	register_bordertype(&bt_firewall);
	register_bordertype(&bt_wisps);
	register_bordertype(&bt_road);

#if USE_EVENTS
	at_register(&at_events);
#endif
	at_register(&at_jihad);
	at_register(&at_skillmod);
#if GROWING_TREES
	at_register(&at_germs);
#endif
	at_register(&at_laen); /* required for old datafiles */
}

void
kernel_init(void)
{
	char zBuffer[MAX_PATH];
	skill_init();
	attrib_init();
	translation_init();

	if (!turn) turn = lastturn();
	if (turn == 0)
		srand(time((time_t *) NULL));
	else
		srand(turn);

	sprintf(zBuffer, "%s/patch-%d.sql", datapath(), turn);
	sql_init(zBuffer);
}

/*********************/
/*   at_guard   */
/*********************/
attrib_type at_guard = {
	"guard",
	DEFAULT_INIT,
	DEFAULT_FINALIZE,
	DEFAULT_AGE,
	DEFAULT_WRITE,
	DEFAULT_READ,
	ATF_UNIQUE
};

void
setguard(unit * u, unsigned int flags)
{
	/* setzt die guard-flags der Einheit */
	attrib * a = a_find(u->attribs, &at_guard);
	if(flags == GUARD_NONE) {
		if(a) a_remove(&u->attribs, a);
		return;
	}
	if (!a) a = a_add(&u->attribs, a_new(&at_guard));
	a->data.i = (int)flags;
}

unsigned int
getguard(const unit * u)
{
	attrib *a;

	if(u->region->terrain == T_OCEAN) return GUARD_NONE;
	a = a_find(u->attribs, &at_guard);
	if (a) return (unsigned int)a->data.i;
	return GUARD_NONE;
}

#ifndef HAVE_STRDUP
char *
strdup(const char *s)
{
	return strcpy((char*)malloc(sizeof(char)*(strlen(s)+1)), s);
}
#endif

void
remove_empty_factions(void)
{
	faction **fp, *f3;
	FILE *dofp;
	char zText[MAX_PATH];

	sprintf(zText, "%s/dropouts", basepath());

	dofp = fopen(zText, "a");

	for (fp = &factions; *fp;) {
		faction * f = *fp;
		/* monster (0) werden nicht entfernt. alive kann beim readgame
		 * () auf 0 gesetzt werden, wenn monsters keine einheiten mehr
		 * haben. */

		if (f->alive == 0 && f->no != MONSTER_FACTION) {
			if (!quiet) printf("\t%s\n", factionname(f));

			/* Einfach in eine Datei schreiben und später vermailen */

			fprintf(dofp, "%s\n", f->email);

			for (f3 = factions; f3; f3 = f3->next) {
				ally * sf;
				group * g;
				ally ** sfp = &f3->allies;
				while (*sfp) {
					sf = *sfp;
					if (sf->faction == f || sf->faction == NULL) {
						*sfp = sf->next;
						free(sf);
					}
					else sfp = &(*sfp)->next;
				}
				for (g = f3->groups; g; g=g->next) {
					sfp = &g->allies;
					while (*sfp) {
						sf = *sfp;
						if (sf->faction == f || sf->faction == NULL) {
							*sfp = sf->next;
							free(sf);
						}
						else sfp = &(*sfp)->next;
					}
				}
			}
			stripfaction(f);
			*fp = f->next;
			free(f);
		}
		else fp = &(*fp)->next;
	}

	fclose(dofp);
}

void
remove_empty_units_in_region(region *r)
{
	unit **up = &r->units;

	while (*up) {
		unit * u = *up;

		if ((old_race(u->race) != RC_SPELL && u->number <= 0)
		 	|| (old_race(u->race) == RC_SPELL && u->age <= 0)
		 	|| u->number < 0) {
			destroy_unit(u);
		}
		if (*up==u) up=&u->next;
	}
}

void
remove_empty_units(void)
{
	region *r;

	for (r = regions; r; r = r->next) {
		remove_empty_units_in_region(r);
	}
}

int *used_faction_ids = NULL;
int no_used_faction_ids = 0;

static int
_cmp_int(const void *i1, const void *i2)
{
	if(i2==NULL)
		return(*(int*)i1);
	return (*(int*)i1 - *(int *)i2);
}

void
register_faction_id(int id)
{
	no_used_faction_ids++;
	used_faction_ids = realloc(used_faction_ids, no_used_faction_ids*sizeof(int));
	used_faction_ids[no_used_faction_ids-1] = id;
	if(no_used_faction_ids > 1)
		qsort(used_faction_ids, (size_t)no_used_faction_ids, sizeof(int), _cmp_int);
}

boolean
faction_id_is_unused(int id)
{
    if(used_faction_ids==NULL)
		return(true);
	return (boolean)(bsearch(&id, used_faction_ids, no_used_faction_ids,
			sizeof(int), _cmp_int) == NULL);
}

int
weight(const unit * u)
{
	int w, n = 0, in_bag = 0;
	int faerie_level;

	item * itm;
	for (itm=u->items;itm;itm=itm->next) {
		w = itm->type->weight * itm->number;
		n += w;
		if( !fval(itm->type, ITF_BIG))
			in_bag += w;
	}

	faerie_level = fspecial(u->faction, FS_FAERIE);
	if (faerie_level) {
		n += (u->number * u->race->weight)/(1+faerie_level);
	} else {
		n += u->number * u->race->weight;
	}

	w = get_item(u, I_BAG_OF_HOLDING) * BAGCAPACITY;
	if( w > in_bag )
		w = in_bag;
	n -= w;

	return n;
}

void
init_used_faction_ids(void)
{
	faction *f;

	no_used_faction_ids = 0;
	for(f = factions; f; f = f->next) {
		register_faction_id(f->no);
	}
}


#if defined(OLD_TRIGGER) || defined(CONVERT_TRIGGER)
# include <eressea/old/trigger.h>
# include <resolve.h>
typedef struct unresolved {
	struct unresolved * next;
	void ** ptrptr;
		/* pointer to the location where the unresolved object
		 * should be, or NULL if special handling is required */
	void * data;
		/* information on how to resolve the missing object */
	resolve_fun resolve;
		/* function to resolve the unknown object */
	typ_t typ;
	tag_t tag;
} unresolved;

static unresolved * ur_list;

void
ur_add2(int id, void ** ptrptr, typ_t typ, tag_t tag, resolve_fun fun) {
	/* skip this for the moment */
   unresolved * ur = calloc(1, sizeof(unresolved));
   ur->data = (void *)id;
   ur->resolve = fun;
   ur->ptrptr = ptrptr;
   ur->typ = typ;
   ur->tag = tag;
   ur->next = ur_list;
   ur_list = ur;
}

void
resolve2(void)
{
	while (ur_list) {
		unresolved * ur = ur_list;
		ur_list = ur->next;
		if (ur->ptrptr) *ur->ptrptr = ur->resolve(ur->data);
		else ur->resolve(ur->data);
		free(ur);
	}
}

#endif

unit *
make_undead_unit(region * r, faction * f, int n, const struct race * rc)
{
	unit *u;

	u = createunit(r, f, n, rc);
	set_string(&u->lastorder, "");
	name_unit(u);
	fset(u, FL_ISNEW);
	return u;
}

void
guard(unit * u, unsigned int mask)
{
	int flags = GUARD_CREWS | GUARD_LANDING | GUARD_TRAVELTHRU | GUARD_TAX;
	switch (old_race(u->race)) {
	case RC_ELF:
		if (u->faction->race != u->race) break;
		/* else fallthrough */
	case RC_TREEMAN:
		flags |= GUARD_TREES;
		break;
	case RC_IRONKEEPER:
		flags = GUARD_MINING;
		break;
	}
	setguard(u, flags & mask);
}

int
besieged(const unit * u)
{
	/* belagert kann man in schiffen und burgen werden */
	return (u
			&& u->building && u->building->besieged
			&& u->building->besieged >= u->building->size * SIEGEFACTOR);
}

int
lifestyle(const unit * u)
{
	static plane * astralspace = NULL;
	int need = u->number * u->race->maintenance;

	if (!astralspace) {
		astralspace = getplanebyname("Astralraum");
	}

	/* Keinen Unterhalt im Astralraum. */
	if (getplane(u->region) == astralspace)
		return 0;

	if(u->region->planep && fval(u->region->planep, PFL_NOFEED))
		return 0;

	if(fspecial(u->faction, FS_REGENERATION))
		need += 1;
	if(fspecial(u->faction, FS_ADMINISTRATOR))
		need += 1;
	if(fspecial(u->faction, FS_WYRM) && old_race(u->race) == RC_WYRM)
		need *= 500;

	return need;
}

void
hunger(unit * u, int need)
{
	region * r = u->region;
	int dead = 0, hpsub = 0;
	int hp = u->hp / u->number;
	int lspp = lifestyle(u)/u->number;

	if(lspp <= 0) return;

	while(need > 0) {
		int dam = old_race(u->race)==RC_HALFLING?15+rand()%14:(13+rand()%12);
		if(dam >= hp) {
			++dead;
		} else {
			hpsub += dam;
		}
		need -= lspp;
	}

	if (dead) {
		/* Gestorbene aus der Einheit nehmen,
		 * Sie bekommen keine Beerdingung. */
		add_message(&u->faction->msgs, new_message(u->faction,
			"starvation%u:unit%r:region%i:dead%i:live", u, r, dead, u->number-dead));

		scale_number(u, u->number - dead);
		deathcounts(r, dead);
	}
	if(hpsub > 0) {
		/* Jetzt die Schäden der nicht gestorbenen abziehen. */
		u->hp -= hpsub;
		/* Meldung nur, wenn noch keine für Tote generiert. */
		if (dead == 0) {
			/* Durch unzureichende Ernährung wird %s geschwächt */
			add_message(&u->faction->msgs, new_message(u->faction,
				"malnourish%u:unit%r:region", u, r));
		}
	}
	if(dead || hpsub) {
		fset(u, FL_HUNGER);
	}
}

void
plagues(region * r, boolean ismagic)
{
	double prob;
	int peasants;
	int i;
	int gestorben;
	/* Vermeidung von DivByZero */
	double mwp = max(maxworkingpeasants(r), 1);

	/* Seuchenwahrscheinlichkeit in % */

	prob = pow((double) rpeasants(r) /
	  (mwp * (((double)wage(r,NULL,false)) / 10.0) * 1.3), 4.0)
		* (double) SEUCHE;

	if (rand() % 100 >= (int)prob && !ismagic) return;

	peasants = rpeasants(r);
	for (i = peasants; i != 0; i--) {
		if (rand() % 100 < SEUCHENOPFER) {
			if (rand() % 100 < HEILCHANCE && rmoney(r) >= HEILKOSTEN) {
				rsetmoney(r, rmoney(r) - HEILKOSTEN);
			} else {
				peasants--;
			}
		}
	}

	gestorben = rpeasants(r) - peasants;

	if (gestorben > 0) {
		message * msg = add_message(&r->msgs, msg_message("pest", "dead", gestorben));
		msg_release(msg);
	}
	rsetpeasants(r, peasants);
}

/* Lohn bei den einzelnen Burgstufen für Normale Typen, Orks, Bauern,
 * Modifikation für Städter. */

static const int wagetable[7][4] = {
	{10, 10, 11, -5},			/* Baustelle */
	{10, 10, 11, -5},			/* Handelsposten */
	{11, 11, 12, -3},			/* Befestigung */
	{12, 11, 13, -1},			/* Turm */
	{13, 12, 14,  0},			/* Burg */
	{14, 12, 15,  1},			/* Festung */
	{15, 13, 16,  2}		 	/* Zitadelle */
};

int
wage(const region *r, const unit *u, boolean img)
	/* Gibt Arbeitslohn für entsprechende Rasse zurück, oder für
	 * die Bauern wenn ra == NORACE. */
{
	building *b = largestbuilding(r, img);
	int      esize = 0;
	int      wage;
	attrib	 *a;

	if (b) esize = buildingeffsize(b, img);

	if (u) {
		wage = wagetable[esize][old_race(u->race) == RC_ORC];
		if (fspecial(u->faction, FS_URBAN)) {
			wage += wagetable[esize][3];
		}
	} else {
		if (rterrain(r) == T_OCEAN) {
			wage = 11;
		} else if (fval(r, RF_ORCIFIED)) {
			wage = wagetable[esize][1];
		} else {
			wage = wagetable[esize][2];
		}
		wage += get_curseeffect(r->attribs, C_BLESSEDHARVEST, 0);
	}

	/* Godcurse: Income -10 */
	if(is_cursed(r->attribs, C_CURSED_BY_THE_GODS, 0)) {
		wage = max(0,wage-10);
	}

	/* Bei einer Dürre verdient man nur noch ein Viertel  */
	if (is_spell_active(r, C_DROUGHT))
		wage /= get_curseeffect(r->attribs, C_DROUGHT, 0);

	a = a_find(r->attribs, &at_reduceproduction);
	if (a) wage = (wage * a->data.sa[0])/100;

	return wage;
}

int
fwage(const region *r, const faction *f, boolean img)
{
	building *b = largestbuilding(r, img);
	int      esize = 0;
	int      wage;
	attrib   *a;

	if (b) esize = buildingeffsize(b, img);

	if (f) {
		wage = wagetable[esize][old_race(f->race) == RC_ORC];
		if (fspecial(f, FS_URBAN)) {
			wage += wagetable[esize][3];
		}
	} else {
		if (rterrain(r) == T_OCEAN) {
			wage = 11;
		} else if (fval(r, RF_ORCIFIED)) {
			wage = wagetable[esize][1];
		} else {
			wage = wagetable[esize][2];
		}
		wage += get_curseeffect(r->attribs, C_BLESSEDHARVEST, 0);
	}

	/* Godcurse: Income -10 */
	if(is_cursed(r->attribs, C_CURSED_BY_THE_GODS, 0)) {
		wage = max(0,wage-10);
	}

	/* Bei einer Dürre verdient man nur noch ein Viertel  */
	if (is_spell_active(r, C_DROUGHT))
		wage /= get_curseeffect(r->attribs, C_DROUGHT, 0);

	a = a_find(r->attribs, &at_reduceproduction);
	if (a) wage = (wage * a->data.sa[0])/100;

	return wage;
}



region *
findspecialdirection(const region *r, char *token)
{
	attrib *a;
	spec_direction *d;

	for (a = a_find(r->attribs, &at_direction);a;a=a->nexttype) {
		d = (spec_direction *)(a->data.v);

		if(strncasecmp(d->keyword, token, strlen(token)) == 0) {
			return findregion(d->x, d->y);
		}
	}

	return NULL;
}

region *
movewhere(region * r, const unit *u)
{
	direction_t d;
	char *token;
	region * r2;

	token = getstrtoken();

	d = finddirection(token, u->faction->locale);
	if (d == D_PAUSE)
		return r;

	if (d == NODIRECTION)
		return findspecialdirection(r, token);

#if 0 /* NOT here! make a temporary attribute for this and move it into travel() */
	if (is_cursed(r->attribs, C_REGCONF, 0)) {
		if (rand()%100 < get_curseeffect(r->attribs, C_REGCONF, 0)) {
			if(u->wants < 0) u->wants--;
			else if(u->wants > 0) u->wants++;
			else u->wants = rand()%2==0?1:-1;
		}
	}

	if (u->ship && is_cursed(u->ship->attribs, C_DISORIENTATION, 0)) {
		if (rand()%100 < get_curseeffect(r->attribs, C_DISORIENTATION, 0)) {
			if(u->wants < 0) u->wants--;
			else if(u->wants > 0) u->wants++;
			else u->wants = rand()%20?1:-1;
		}
	}
	d = (direction_t)((d + u->wants + MAXDIRECTIONS) % MAXDIRECTIONS);
#endif

	if (!rconnect(r, d)) {
#if USE_CREATION
		makeblock(r->x + delta_x[d], r->y + delta_y[d], 1);
		printf("* Fehler! Region (%d,%d) hatte seine Nachbarn "
			   "(%d,%d) noch nicht generiert!\n", r->x, r->y,
			   r->x + delta_x[d], r->y + delta_y[d]);
#else
		add_message(&u->faction->msgs,
			msg_message("moveblocked", "unit direction", u, d));
		return NULL;
#endif
	}
	r2 = rconnect(r, d);

	if (!r2) {
		printf("* Fehler! Region (%d,%d) hatte seine Nachbarn "
			   "(%d,%d) nicht gefunden!", r->x, r->y,
			   r->x + delta_x[d], r->y + delta_y[d]);
		return 0;
	}

	if (move_blocked(u, r, d) == true) {
		add_message(&u->faction->msgs,
			msg_message("moveblocked", "unit direction", u, d));
		return NULL;
	}

	/* r2 enthält nun die existierende Zielregion - ihre Nachbarn sollen
	 * auch schon alle existieren. Dies erleichtert das Umherschauen bei
	 * den Reports! */

#if USE_CREATION
	for (d = 0; d != MAXDIRECTIONS; d++)
		if (!rconnect(r2, d))
			makeblock(r2->x + delta_x[d], r2->y + delta_y[d], 1);
#endif

	return r2;
}

boolean
move_blocked(const unit * u, const region *r, direction_t dir)
{
	region * r2 = NULL;
	border * b;
	if (dir<MAXDIRECTIONS) r2 = rconnect(r, dir);
	if (r2==NULL) return true;
	b = get_borders(r, r2);
	while (b) {
		if (b->type->block && b->type->block(b, u, r)) return true;
		b = b->next;
	}
	return false;
}

void
add_income(unit * u, int type, int want, int qty)
{
	if (want==INT_MAX) want = qty;
	add_message(&u->faction->msgs, new_message(u->faction, "income%u:unit%r:region%i:mode%i:wanted%i:amount",
		u, u->region, type, want, qty));
}

int weeks_per_month;
int months_per_year;

int
month(int offset)
{
	int t = turn - FIRST_TURN + offset;
	int year, r, month;

	if (t<0) t = turn;

	year  = t/(months_per_year * weeks_per_month) + 1;
	r     = t - (year-1) * months_per_year * weeks_per_month;
	month = r/weeks_per_month;

	return month;
}

void
reorder_owners(region * r)
{
	unit ** up=&r->units, ** useek;
	building * b=NULL;
	ship * sh=NULL;
#ifndef NDEBUG
	size_t len = listlen(r->units);
#endif
	for (b = r->buildings;b;b=b->next) {
		unit ** ubegin = up;
		unit ** uend = up;

		useek = up;
		while (*useek) {
			unit * u = *useek;
			if (u->building==b) {
				unit ** insert;
				if (fval(u, FL_OWNER)) {
					unit * nu = *ubegin;
					insert=ubegin;
					if (nu!=u && nu->building==u->building && fval(nu, FL_OWNER)) {
						log_error(("[reorder_owners] %s hat mehrere Besitzer mit FL_OWNER.\n", buildingname(nu->building)));
						freset(nu, FL_OWNER);
					}
				}
				else insert = uend;
				if (insert!=useek) {
					*useek = u->next; /* raus aus der liste */
					u->next = *insert;
					*insert = u;
				}
				if (insert==uend) uend=&u->next;
			}
			if (*useek==u) useek = &u->next;
		}
		up = uend;
	}

	useek=up;
	while (*useek) {
		unit * u = *useek;
		assert(!u->building);
		if (u->ship==NULL) {
			if (fval(u, FL_OWNER)) {
				log_warning(("[reorder_owners] Einheit %s war Besitzer von nichts.\n", unitname(u)));
				freset(u, FL_OWNER);
			}
			if (useek!=up) {
				*useek = u->next; /* raus aus der liste */
				u->next = *up;
				*up = u;
			}
			up = &u->next;
		}
		if (*useek==u) useek = &u->next;
	}

	for (sh = r->ships;sh;sh=sh->next) {
		unit ** ubegin = up;
		unit ** uend = up;

		useek = up;
		while (*useek) {
			unit * u = *useek;
			if (u->ship==sh) {
				unit ** insert;
				if (fval(u, FL_OWNER)) {
					unit * nu = *ubegin;
					insert = ubegin;
					if (nu!=u && nu->ship==u->ship && fval(nu, FL_OWNER)) {
						log_error(("[reorder_owners] %s hat mehrere Besitzer mit FL_OWNER.\n", shipname(nu->ship)));
						freset(nu, FL_OWNER);
					}
				}
				else insert = uend;
				if (insert!=useek) {
					*useek = u->next; /* raus aus der liste */
					u->next = *insert;
					*insert = u;
				}
				if (insert==uend) uend=&u->next;
			}
			if (*useek==u) useek = &u->next;
		}
		up = uend;
	}
#ifndef NDEBUG
	assert(len==listlen(r->units));
#endif
}
