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
#include "build.h"

#ifdef OLD_TRIGGER
#include <old/trigger.h>
#endif

/* kernel includes */
#include "alchemy.h"
#include "border.h"
#include "building.h"
#include "curse.h"
#include "faction.h"
#include "group.h"
#include "item.h"
#include "magic.h"
#include "message.h"
#include "movement.h"
#include "pool.h"
#include "race.h"
#include "region.h"
#include "ship.h"
#include "skill.h"
#include "unit.h"

/* from libutil */
#include <attrib.h>
#include <base36.h>
#include <event.h>
#include <goodies.h>
#include <resolve.h>

/* from libc */
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* attributes inclues */
#include <attributes/matmod.h>

#define NEW_TAVERN
#define STONERECYCLE 50
/* Name, MaxGroesse, MinBauTalent, Kapazitaet, {Eisen, Holz, Stein, BauSilber,
 * Laen, Mallorn}, UnterSilber, UnterSpezialTyp, UnterSpezial */


/* ------------------------------------------------------------- */

static int
slipthru(region * r, unit * u, const building * b)
{
	unit *u2;
	int n, o;

	/* b ist die burg, in die man hinein oder aus der man heraus will. */

	if (!b) {
		return 1;
	}
	if (b->besieged < b->size * SIEGEFACTOR) {
		return 1;
	}
	/* u wird am hinein- oder herausschluepfen gehindert, wenn STEALTH <=
	 * OBSERVATION +2 der belagerer u2 ist */

	n = eff_skill(u, SK_STEALTH, r);

	for (u2 = r->units; u2; u2 = u2->next)
		if (usiege(u2) == b) {
			if (get_item(u, I_RING_OF_INVISIBILITY) &&
				get_item(u, I_RING_OF_INVISIBILITY) >= u->number &&
				!get_item(u2, I_AMULET_OF_TRUE_SEEING))
				continue;

			o = eff_skill(u2, SK_OBSERVATION, r);

			if (o + 2 >= n)
				return 0;		/* entdeckt! */
		}
	return 1;
}

boolean
can_contact(region * r, unit * u, unit * u2)
{

	/* hier geht es nur um die belagerung von burgen */

	if (u->building == u2->building)
		return true;

	/* unit u is trying to contact u2 - unasked for contact. wenn u oder u2
	 * nicht in einer burg ist, oder die burg nicht belagert ist, ist
	 * slipthru () == 1. ansonsten ist es nur 1, wenn man die belagerer */

	if (slipthru(u->region, u, u->building)
			&& slipthru(u->region, u2, u2->building))
		return true;

	if (allied(u, u2->faction, HELP_GIVE))
		return true;

	return false;
}


static void
set_contact(region * r, unit * u, char try)
{

	/* unit u kontaktiert unit u2. Dies setzt den contact einfach auf 1 -
	 * ein richtiger toggle ist (noch?) nicht noetig. die region als
	 * parameter ist nur deswegen wichtig, weil er an getunit ()
	 * weitergegeben wird. dies wird fuer das auffinden von tempunits in
	 * getnewunit () verwendet! */

	unit *u2 = getunit(r, u);

	if (u2) {

		if (!can_contact(r, u, u2)) {
			if(try) cmistake(u, findorder(u, u->thisorder), 23, MSG_EVENT);
			return;
		}
		usetcontact(u, u2);
	}
}
/* ------------------------------------------------------------- */

/* ------------------------------------------------------------- */


/* ------------------------------------------------------------- */

struct building *
getbuilding(const struct region * r)
{
	building * b = findbuilding(getid());
	if (b==NULL || r!=b->region) return NULL;
	return b;
}

ship *
getship(const struct region * r)
{
	ship *sh, *sx = findship(getshipid());
	for (sh = r->ships; sh; sh = sh->next) {
		if (sh == sx) return sh;
	}
	return NULL;
}

/* ------------------------------------------------------------- */

static void
siege(region * r, unit * u)
{
	unit *u2;
	building *b;
	int d;
	int bewaffnete, katapultiere = 0;

	/* gibt es ueberhaupt Burgen? */

	b = getbuilding(r);

	if (!b) {
		cmistake(u, u->thisorder, 31, MSG_BATTLE);
		return;
	}

	if (race[u->race].nonplayer) {
		/* keine Drachen, Illusionen, Untote etc */
		cmistake(u, u->thisorder, 166, MSG_BATTLE);
		return;
	}
	/* schaden durch katapulte */

	d = min(u->number, get_item(u, I_CATAPULT));
	if (eff_skill(u, SK_CATAPULT, r) >= 1) {
		katapultiere = d;
		d *= eff_skill(u, SK_CATAPULT, r);
	} else {
		d = 0;
	}


	if ((bewaffnete = armedmen(u)) == 0 && d == 0) {
		/* abbruch, falls unbewaffnet oder unfaehig, katapulte zu benutzen */
		cmistake(u, findorder(u, u->thisorder), 80, MSG_EVENT);
		return;
	}

	if (!(getguard(u) & GUARD_TRAVELTHRU)) {
		/* abbruch, wenn die einheit nicht vorher die region bewacht - als
		 * warnung fuer alle anderen! */
		cmistake(u, findorder(u, u->thisorder), 81, MSG_EVENT);
		return;
	}
	/* einheit und burg markieren - spart zeit beim behandeln der einheiten
	 * in der burg, falls die burg auch markiert ist und nicht alle
	 * einheiten wieder abgesucht werden muessen! */

	usetsiege(u, b);
	b->besieged += max(bewaffnete, katapultiere);

	/* definitiver schaden eingeschraenkt */

	d = min(d, b->size - 1);

	/* meldung, schaden anrichten */
	if (d && !(is_cursed(b->attribs, C_MAGICSTONE, 0))) {
		b->size -= d;
		d = 100 * d / b->size;
	} else d = 0;

	/* meldung fuer belagerer */
	add_message(&u->faction->msgs,
		new_message(u->faction, "siege%u:unit%b:building%i:destruction",
		u, b, d));

	for (u2 = r->units; u2; u2 = u2->next) freset(u2->faction, FL_DH);
	fset(u->faction, FL_DH);

	/* Meldung fuer Burginsassen */
	for (u2 = r->units; u2; u2 = u2->next) {
		if (u2->building == b && !fval(u2->faction, FL_DH)) {
			fset(u2->faction, FL_DH);
			add_message(&u2->faction->msgs,
				new_message(u2->faction, "siege%u:unit%b:building%i:destruction",
				u, b, d));
		}
	}
}

void
do_siege(void)
{
	region *r;

	for (r = regions; r; r = r->next) {
		if (rterrain(r) != T_OCEAN) {
			unit *u;

			for (u = r->units; u; u = u->next) {
				if (igetkeyword(u->thisorder, u->faction->locale) == K_BESIEGE)
					siege(r, u);
			}
		}
	}
}
/* ------------------------------------------------------------- */

void
destroy_road(unit *u, int n, const char *cmd)
{
	direction_t d = getdirection();
	unit *u2;
	region *r = u->region;

	for (u2=r->units;u2;u2=u2->next) {
		if (u2->faction!=u->faction && getguard(u2)&GUARD_TAX &&
			 	!allied(u, u2->faction, HELP_GUARD)) {
			cmistake(u, cmd, 70, MSG_EVENT);
			return;
		}
	}

	if (d==NODIRECTION) {
		cmistake(u, cmd, 71, MSG_PRODUCE);
	} else {
#if 0
		int salvage, divy = 2;
#endif
		int willdo = min(n, (1+get_skill(u, SK_ROAD_BUILDING))*u->number);
		int road = rroad(r, d);
		region * r2 = rconnect(r,d);
		willdo = min(willdo, road);
#if 0
		salvage = willdo / divy;
		change_item(u, I_STONE, salvage);
#endif
		rsetroad(r, d, road - willdo);
		add_message(&u->faction->msgs, new_message(
			u->faction, "destroy_road%u:unit%r:from%r:to", u, r, r2));
	}
}

void
destroy(region * r, unit * u, const char * cmd)
{
	ship *sh;
	unit *u2;
#if 0
	const construction * con = NULL;
	int size = 0;
#endif
	char *s;
	int n = INT_MAX;

	if (u->number < 1)
		return;

	s = getstrtoken();

	if (findparam(s, u->faction->locale)==P_ROAD) {
		destroy_road(u, INT_MAX, cmd);
		return;
	}

	if (!fval(u, FL_OWNER)) {
		cmistake(u, cmd, 138, MSG_PRODUCE);
		return;
	}

	if(s && *s) {
		n = atoi(s);
		if(n <= 0) {
			cmistake(u, cmd, 288, MSG_PRODUCE);
			return;
		}
	}

	if(getparam(u->faction->locale) == P_ROAD) {
		destroy_road(u, n, cmd);
		return;
	}

	if (u->building) {
		building *b = u->building;
#if 0
		con = b->type->construction;
		size = b->size;
#endif

		if(n >= b->size) {
			/* destroy completly */
			/* all units leave the building */
			for (u2 = r->units; u2; u2 = u2->next)
				if (u2->building == b) {
					u2->building = 0;
					freset(u2, FL_OWNER);
				}
			add_message(&u->faction->msgs, new_message(
				u->faction, "destroy%b:building%u:unit", b, u));
			destroy_building(b);
		} else {
			/* partial destroy */
			b->size -= n;
			add_message(&u->faction->msgs, new_message(
				u->faction, "destroy_partial%b:building%u:unit", b, u));
		}
	} else if (u->ship) {
		sh = u->ship;
#if 0
		con = sh->type->construction;
		size = (sh->size * DAMAGE_SCALE - sh->damage) / DAMAGE_SCALE;
#endif

		if (rterrain(r) == T_OCEAN) {
			cmistake(u, cmd, 14, MSG_EVENT);
			return;
		}

		if(n >= (sh->size*100)/sh->type->construction->maxsize) {
			/* destroy completly */
			/* all units leave the ship */
			for (u2 = r->units; u2; u2 = u2->next)
				if (u2->ship == sh) {
					u2->ship = 0;
					freset(u2, FL_OWNER);
				}
			add_message(&u->faction->msgs, new_message(
				u->faction, "shipdestroy%u:unit%r:region%h:ship", u, r, sh));
			destroy_ship(sh, r);
		} else {
			/* partial destroy */
			sh->size -= (sh->type->construction->maxsize * n)/100;
			add_message(&u->faction->msgs, new_message(
				u->faction, "shipdestroy_partial%u:unit%r:region%h:ship", u, r, sh));
		}
	} else
		printf("* Fehler im Program! Die Einheit %s von %s\n"
			   "  (Spieler: %s) war owner eines objects,\n"
			   "  war aber weder in einer Burg noch in einem Schiff.\n",
			   unitname(u), u->faction->name, u->faction->email);

#if 0
	/* Achtung: Nicht an ZERSTÖRE mit Punktangabe angepaßt! */
	if (con) {
		/* Man sollte alle Materialien zurückkriegen können: */
		int c;
		for (c=0;con->materials[c].number;++c) {
			const requirement * rq = con->materials+c;
			int recycle = (int)(rq->recycle * rq->number * size/con->reqsize);
			if (recycle)
			  change_resource(u, rq->type, recycle);
		}
	}
#endif
}
/* ------------------------------------------------------------- */

void
build_road(region * r, unit * u, int size, direction_t d)
{
	int n, dm, left;

	if (!eff_skill(u, SK_ROAD_BUILDING, r)) {
		cmistake(u, findorder(u, u->thisorder), 103, MSG_PRODUCE);
		return;
	}
	if (besieged(u)) {
		cmistake(u, findorder(u, u->thisorder), 60, MSG_PRODUCE);
		return;
	}

	if (terrain[rterrain(r)].roadreq < 0) {
		cmistake(u, findorder(u, u->thisorder), 94, MSG_PRODUCE);
		return;
	}

	if (rterrain(r) == T_SWAMP)
		/* wenn kein Damm existiert */
		if (!buildingtype_exists(r, &bt_dam)) {
			cmistake(u, findorder(u, u->thisorder), 132, MSG_PRODUCE);
			return;
		}
	if (rterrain(r) == T_DESERT)
		/* wenn keine Karawanserei existiert */
		if (!buildingtype_exists(r, &bt_caravan)) {
			cmistake(u, findorder(u, u->thisorder), 133, MSG_PRODUCE);
			return;
		}
	if (rterrain(r) == T_GLACIER)
		/* wenn kein Tunnel existiert */
		if (!buildingtype_exists(r, &bt_tunnel)) {
			cmistake(u, findorder(u, u->thisorder), 131, MSG_PRODUCE);
			return;
		}
	if (!get_pooled(u, r, R_STONE) && u->race != RC_STONEGOLEM) {
		cmistake(u, findorder(u, u->thisorder), 151, MSG_PRODUCE);
		return;
	}

	/* left kann man noch bauen */
	left = terrain[rterrain(r)].roadreq - rroad(r, d);
	if (size > 0) {
		n = min(size, left);
	} else {
		n = left;
	}

	/* hoffentlich ist r->road <= terrain[rterrain(r)].roadreq, n also >= 0 */

	if (n <= 0) {
		sprintf(buf, "In %s gibt es keine Brücken und Straßen "
			"mehr zu bauen", regionid(r));
		mistake(u, u->thisorder, buf, MSG_PRODUCE);
		return;
	}

	/* max Bauten anhand des Talentes */
	n = min(n, u->number * eff_skill(u, SK_ROAD_BUILDING, r));

	if ((dm = get_effect(u, oldpotiontype[P_DOMORE])) != 0) {
		dm = min(dm, u->number);
		change_effect(u, oldpotiontype[P_DOMORE], -dm);
		n += dm * eff_skill(u, SK_ROAD_BUILDING, r);
	}							/* Auswirkung Schaffenstrunk */

	/* und anhand der rohstoffe */
	if (u->race == RC_STONEGOLEM){
		n = min(n, u->number * GOLEM_STONE);
	} else {
		n = use_pooled(u, r, R_STONE, n);
	}

	/* n is now modified by several special effects, so we have to
	 * minimize it again to make sure the road will not grow beyond
	 * maximum. */
	rsetroad(r, d, rroad(r, d) + min(n, left));

	if (u->race == RC_STONEGOLEM){
		int golemsused = n / GOLEM_STONE;
		if (n%GOLEM_STONE != 0){
			++golemsused;
		}
		scale_number(u,u->number - golemsused);
	} else {
		/* Nur soviel PRODUCEEXP wie auch tatsaechlich gemacht wurde */
		change_skill(u, SK_ROAD_BUILDING, min(n, u->number) * PRODUCEEXP);
	}
	add_message(&u->faction->msgs, new_message(
		u->faction, "buildroad%r:region%u:unit%i:size", r, u, n));
}
/* ------------------------------------------------------------- */

/* ** ** ** ** ** ** *
 *  new build rules  *
 * ** ** ** ** ** ** */

static int
required(int size, int msize, int maxneed)
	/* um size von msize Punkten zu bauen,
	 * braucht man required von maxneed resourcen */
{
	int used;

	used = size * maxneed / msize;
	if (size * maxneed % msize)
	  ++used;
	return used;
}

static int
matmod(const attrib * a, const unit * u, const resource_type * material, int value)
{
	for (a=a_find((attrib*)a, &at_matmod);a;a=a->nexttype) {
		mm_fun fun = (mm_fun)a->data.f;
		value = fun(u, material, value);
		if (value<0) return value; /* pass errors to caller */
	}
	return value;
}

int
build(unit * u, const construction * ctype, int completed, int want)
	/* Use up resources for building an object. returns the actual size
	 * being built
	 * Build up to 'size' points of 'type', where 'completed'
	 * of the first object have already been finished. return the
	 * actual size that could be built.
	 */
{
	const construction * type = ctype;
	int skills; /* number of skill points remainig */
	int dm = get_effect(u, oldpotiontype[P_DOMORE]);
	int made = 0;
	int basesk, effsk;

	if (want<=0) return 0;
	if (type==NULL) return 0;
	if (type->improvement==NULL && completed==type->maxsize)
		return ECOMPLETE;

	basesk = effskill(u, type->skill);
	if (basesk==0) return ENEEDSKILL;

	effsk = basesk;
	if (u->building) {
		effsk = skillmod(u->building->type->attribs, u, u->region, type->skill, effsk, SMF_PRODUCTION);
	}
	effsk = skillmod(type->attribs, u, u->region, type->skill, effsk, SMF_PRODUCTION);
	if (effsk<0) return effsk; /* pass errors to caller */
	if (effsk==0) return ENEEDSKILL;

	skills = effsk * u->number;

	/* technically, nimblefinge and domore should be in a global set of "game"-attributes,
	 * (as at_skillmod) but for a while, we're leaving them in here. */

	if (dm != 0) {
		/* Auswirkung Schaffenstrunk */
		dm = min(dm, u->number);
		change_effect(u, oldpotiontype[P_DOMORE], -dm);
		skills += dm * effsk;
	}
	for (;want>0 && skills>0;) {
		int c, n, i = 0;
		item * itm;

		/* skip over everything that's already been done:
		 * type->improvement==NULL means no more improvements, but no size limits
		 * type->improvement==type means build another object of the same time while material lasts
		 * type->improvement==x means build x when type is finished
		 */
		while (type->improvement!=NULL &&
			   type->improvement!=type &&
			   type->maxsize>0 &&
			   type->maxsize<=completed)
		{
			completed -= type->maxsize;
			type = type->improvement;
		}
		if (type==NULL) {
			if (made==0) return ECOMPLETE;
			break; /* completed */
		}

		/* ??? Was soll das (Stefan) ???
		 *	Hier ist entweder maxsize == -1, oder completed < maxsize.
		 *	Andernfalls ist das Datenfile oder sonstwas kaputt...
		 *	(enno): Nein, das ist für Dinge, bei denen die nächste Ausbaustufe
		 *  die gleiche wie die vorherige ist. z.b. gegenstände.
		 */
		if (type->maxsize>1) {
			completed = completed % type->maxsize;
		}
		else {
			completed = 0; assert(type->reqsize==1);
		}

		if (basesk < type->minskill) {
			if (made==0) return ELOWSKILL; /* not good enough to go on */
		}

		/* n = maximum buildable size */
		n = skills / type->minskill;
		itm = *i_find(&u->items, olditemtype[I_RING_OF_NIMBLEFINGER]);
		if (itm!=NULL) i = itm->number;
		if (i>0) {
			int rings = min(u->number, i);
			n = n * (9*rings+u->number) / u->number;
		}

		if (want>0)
			n = min(want, n);
		if (type->maxsize>0) {
			n = min(type->maxsize-completed, n);
		}

		if (n<=0) break; /* can't build any more */

		if (type->materials) for (c=0;n && type->materials[c].number;c++) {
			resource_t rtype = type->materials[c].type;
			int need;
			int have = get_pooled(u, NULL, rtype);
			int prebuilt;
			int canuse = have;
			if (u->building)
				canuse = matmod(u->building->type->attribs, u, oldresourcetype[rtype], canuse);
			if (canuse<0) return canuse; /* pass errors to caller */
			canuse = matmod(type->attribs, u, oldresourcetype[rtype], canuse);
			if (type->reqsize>1) {
				prebuilt = required(completed, type->reqsize, type->materials[c].number);
				for (;n;) {
					need = required(completed + n, type->reqsize, type->materials[c].number);
					if (need-prebuilt<=canuse) break;
					--n; /* TODO: optimieren? */
				}
			} else {
				int maxn = canuse / type->materials[c].number;
				if (maxn < n) n = maxn;
			}
		}
		if (n<=0) {
			if (made==0) return ENOMATERIALS;
			else break;
		}
		if (type->materials) for (c=0;type->materials[c].number;c++) {
			resource_t rtype = type->materials[c].type;
			int prebuilt = required(completed, type->reqsize, type->materials[c].number);
			int need = required(completed + n, type->reqsize, type->materials[c].number);
			int multi = 1;
			int canuse = 100; /* normalization */
			if (u->building) canuse = matmod(u->building->type->attribs, u, oldresourcetype[rtype], canuse);
			if (canuse<0) return canuse; /* pass errors to caller */
			canuse = matmod(type->attribs, u, oldresourcetype[rtype], canuse);

			assert(canuse % 100 == 0 || !"only constant multipliers are implemented in build()");
			multi = canuse/100;
			if (canuse<0) return canuse; /* pass errors to caller */

			use_pooled(u, NULL, rtype, (need-prebuilt+multi-1)/multi);
		}
		made += n;
		skills -= n * type->minskill;
		want -= n;
		completed = completed + n;
	}
	/* Nur soviel PRODUCEEXP wie auch tatsaechlich gemacht wurde */
	change_skill(u, ctype->skill, min(made, u->number) * PRODUCEEXP);

	return made;
}

int
maxbuild(const unit * u, const construction * cons)
		/* calculate maximum size that can be built from available material */
		/* !! ignores maximum objectsize and improvements...*/
{
	int c;
	int maximum = INT_MAX;
	for (c=0;cons->materials[c].number;c++) {
		resource_t rtype = cons->materials[c].type;
		int have = get_pooled(u, NULL, rtype);
		int need = required(1, cons->reqsize, cons->materials[c].number);
		if (have<need) {
			cmistake(u, findorder(u, u->thisorder), 88, MSG_PRODUCE);
			return 0;
		}
		else maximum = min(maximum, have/need);
	}
	return maximum;
}

/** old build routines */

void
build_building(unit * u, const building_type * btype, int want)
{
	region * r = u->region;
	boolean newbuilding = false;
	int c, built = 0;
	building * b = getbuilding(r);
	/* einmalige Korrektur */
	static char buffer[8 + IDSIZE + 1 + NAMESIZE + 1];
	const char *string2;

	if (eff_skill(u, SK_BUILDING, r) == 0) {
		cmistake(u, findorder(u, u->thisorder), 101, MSG_PRODUCE);
		return;
	}

	/* Falls eine Nummer angegeben worden ist, und ein Gebaeude mit der
	 * betreffenden Nummer existiert, ist b nun gueltig. Wenn keine Burg
	 * gefunden wurde, dann wird einfach eine neue erbaut, falls man in
	 * keiner burg ist. Ansonsten baut man an der eigenen burg weiter. */

	if (!b && u->building && u->building->type==btype) b = u->building;
	if (b) btype = b->type;

	if (fval(btype, BTF_UNIQUE) && buildingtype_exists(r, btype)) {
		/* only one of these per region */
		cmistake(u, findorder(u, u->thisorder), 93, MSG_PRODUCE);
		return;
	}
	if (besieged(u)) {
		/* units under siege can not build */
		cmistake(u, findorder(u, u->thisorder), 60, MSG_PRODUCE);
		return;
	}
	if (btype->flags & BTF_NOBUILD) {
		/* special building, cannot be built */
		cmistake(u, findorder(u, u->thisorder), 221, MSG_PRODUCE);
		return;
	}

	if (b) built = b->size;
	if (want<=0 || want == INT_MAX) {
		if(b == NULL) {
			if(btype->maxsize > 0) {
				want = btype->maxsize - built;
			} else {
				want = INT_MAX;
			}
		} else {
			if(b->type->maxsize > 0) {
				want = b->type->maxsize - built;
			} else {
				want = INT_MAX;
			}
		}
	}
	built = build(u, btype->construction, built, want);

	switch (built) {
	case ECOMPLETE:
		/* the building is already complete */
		cmistake(u, findorder(u, u->thisorder), 4, MSG_PRODUCE);
		return;
	case ENOMATERIALS: {
		/* something missing from the list of materials */
		const construction * cons = btype->construction;
		char * ch;
		assert(cons);
		strcpy(buf, "Dafür braucht man mindestens:");
		ch = buf+strlen(buf);
		for (c=0;cons->materials[c].number; c++) {
			int n;
			if (c!=0) strcat(ch++, ",");
			n = cons->materials[c].number / cons->reqsize;
			sprintf(ch, " %d %s", n?n:1, resname(cons->materials[c].type, cons->materials[c].number==1));
			ch = ch+strlen(ch);
		}
		strcat(ch,".");
		mistake(u, u->thisorder, buf, MSG_PRODUCE);
		return;
	}
	case ELOWSKILL:
	case ENEEDSKILL:
		/* no skill, or not enough skill points to build */
		cmistake(u, findorder(u, u->thisorder), 50, MSG_PRODUCE);
		return;
	}


	/* at this point, the building size is increased. */
	if (b==NULL) {
		/* build a new building */
		b = new_building(btype, r, u->faction->locale);
		b->type = btype;
		fset(b, BLD_MAINTAINED);

		/* Die Einheit befindet sich automatisch im Inneren der neuen Burg. */
		leave(r, u);
		u->building = b;
		fset(u, FL_OWNER);

		newbuilding = 1;
	}

	if(b->type==&bt_castle) {
			string2 = locale_string(u->faction->locale, bt_castle._name);
	} else {
			string2 = buildingtype(b, b->size, u->faction->locale);
			if( newbuilding && b->type->maxsize != -1 )
				want = b->type->maxsize - b->size;
	}

	if( want == INT_MAX )
		sprintf(buffer, "%s %s %s", locale_string(u->faction->locale, keywords[K_MAKE]), string2, buildingid(b));
	else if( want-built <= 0 )
		strcpy(buffer, locale_string(u->faction->locale, keywords[K_WORK]));
	else
		sprintf(buffer, "%s %d %s %s", locale_string(u->faction->locale, keywords[K_MAKE]), want-built, string2, buildingid(b));
	set_string(&u->lastorder, buffer);

	b->size += built;
	if (b->type == &bt_lighthouse)
	  update_lighthouse(b);

	add_message(&u->faction->msgs, new_message(
		u->faction, "buildbuilding%b:building%u:unit%i:size", b, u, built));
}

static void
build_ship(unit * u, ship * sh, int want)
{
	const construction * construction = sh->type->construction;
	int size = (sh->size * DAMAGE_SCALE - sh->damage) / DAMAGE_SCALE;
	int can = u->number * effskill(u, SK_SHIPBUILDING) / construction->minskill;
	int n;

	if (want > 0) can = min(want, can);
	can = min(can, construction->maxsize+sh->damage); /* 100% bauen + 100% reparieren */
	can = build(u, construction, size, can);

	if((n=construction->maxsize - sh->size)>0 && can>0) {
	  if(can>=n) {
		 sh->size += n;
		 can -= n;
	  }
	  else {
		 sh->size += can;
		 n=can;
		 can = 0;
	  }
	}

	if (sh->damage && can) {
		int repair = min(sh->damage, can * DAMAGE_SCALE);
		n += repair / DAMAGE_SCALE;
		if (repair % DAMAGE_SCALE) ++n;
		sh->damage = sh->damage - repair;
	}

	if(n)
	  add_message(&u->faction->msgs, new_message(
		  u->faction, "buildship%h:ship%u:unit%i:size", sh, u, n));
}

void
create_ship(region * r, unit * u, const struct ship_type * newtype, int want)
{
	static char buffer[IDSIZE + 2 * KEYWORDSIZE + 3];
	ship *sh;
	int msize;
	const construction * cons = newtype->construction;

	if (!eff_skill(u, SK_SHIPBUILDING, r)) {
		cmistake(u, findorder(u, u->thisorder), 100, MSG_PRODUCE);
		return;
	}
	if (besieged(u)) {
		cmistake(u, findorder(u, u->thisorder), 60, MSG_PRODUCE);
		return;
	}

	/* check if skill and material for 1 size is available */
	if (eff_skill(u, cons->skill, r) < cons->minskill) {
		sprintf(buf, "Um %s zu bauen, braucht man ein Talent von "
			"mindestens %d.", newtype->name[1], cons->minskill);
		mistake(u, u->thisorder, buf, MSG_PRODUCE);
		return;
	}

	msize = maxbuild(u, cons);
	if (msize==0) {
		cmistake(u, findorder(u, u->thisorder), 88, MSG_PRODUCE);
		return;
	}
	if (want>0) want = min(want, msize);
	else want = msize;

	sh = new_ship(newtype, r);

	addlist(&r->ships, sh);

	leave(r, u);
	u->ship = sh;
	fset(u, FL_OWNER);
	sprintf(buffer, "%s %s %s",
			locale_string(u->faction->locale, keywords[K_MAKE]), locale_string(u->faction->locale, parameters[P_SHIP]), shipid(sh));
	u->lastorder = set_string(&u->lastorder, buffer);

	build_ship(u, sh, want);
}

void
continue_ship(region * r, unit * u, int want)
{
	const construction * cons;
	ship *sh;
	int msize;

	if (!eff_skill(u, SK_SHIPBUILDING, r)) {
		cmistake(u, findorder(u, u->thisorder), 100, MSG_PRODUCE);
		return;
	}

	/* Die Schiffsnummer bzw der Schiffstyp wird eingelesen */
	sh = getship(r);

	if (!sh) sh = u->ship;

	if (!sh) {
		cmistake(u, findorder(u, u->thisorder), 20, MSG_PRODUCE);
		return;
	}
	cons = sh->type->construction;
	assert(cons->improvement==NULL); /* sonst ist construction::size nicht ship_type::maxsize */
	if (sh->size==cons->maxsize && !sh->damage) {
		cmistake(u, findorder(u, u->thisorder), 16, MSG_PRODUCE);
		return;
	}
	if (eff_skill(u, cons->skill, r) < cons->minskill) {
		sprintf(buf, "Um %s zu bauen, braucht man ein Talent von "
				"mindestens %d.", sh->type->name[1], cons->minskill);
		mistake(u, u->thisorder, buf, MSG_PRODUCE);
		return;
	}
	msize = maxbuild(u, cons);
	if (msize==0) {
		cmistake(u, findorder(u, u->thisorder), 88, MSG_PRODUCE);
		return;
	}
	if (want > 0) want = min(want, msize);
	else want = msize;

	build_ship(u, sh, want);
}
/* ------------------------------------------------------------- */

static int
mayenter(region * r, unit * u, building * b)
{
	unit *u2;
	u2 = buildingowner(r, b);


	return (!u2
		|| ucontact(u2, u)
		|| allied(u2, u->faction, HELP_GUARD));

}

static int
mayboard(region * r, unit * u, ship * sh)
{
	unit *u2;
	u2 = shipowner(r, sh);

	return (!u2
		|| ucontact(u2, u)
		|| allied(u2, u->faction, HELP_GUARD));

}

void
remove_contacts(void)
{
	region *r;
	unit *u;
	attrib *a;

	for (r = regions; r; r = r->next) {
		for (u = r->units; u; u = u->next) {
			a = (attrib *)a_find(u->attribs, &at_contact);
			while(a != NULL) {
				attrib * ar = a;
				a = a->nexttype;
				a_remove(&u->attribs, ar);
			}
		}
	}
}

void
do_leave(void)
{
	region *r;
	unit *u;
	strlist *S;

	for (r = regions; r; r = r->next) {
		for (u = r->units; u; u = u->next) {
			for (S = u->orders; S; S = S->next) {
				if(igetkeyword(S->s, u->faction->locale) == K_LEAVE) {
					if (r->terrain == T_OCEAN && u->ship) {
						if(!(race[u->race].flags & RCF_SWIM)) {
							cmistake(u, S->s, 11, MSG_MOVE);
							break;
						}
						if(get_item(u, I_HORSE)) {
							cmistake(u, S->s, 231, MSG_MOVE);
							break;
						}
					}
					if (!slipthru(r, u, u->building)) {
						sprintf(buf, "%s wird belagert.", buildingname(u->building));
						mistake(u, S->s, buf, MSG_MOVE);
						break;
					}
					leave(r, u);
					break;
				}
			}
		}
	}
}

void
do_misc(char try)
{
	region *r;
	strlist *S, *Snext;
	ship *sh;
	building *b;

	/* try: Fehler nur im zweiten Versuch melden. Sonst konfus. */

	for (r = regions; r; r = r->next) {
		unit *u;

		for (u = r->units; u; u = u->next) {
			for (S = u->orders; S; S = S->next) {
				switch (igetkeyword(S->s, u->faction->locale)) {
				case K_CONTACT:
					set_contact(r, u, try);
					break;
				}
			}
		}

		for (u = r->units; u; u = u->next) {
			for (S = u->orders; S;) {
				Snext = S->next;

				switch (igetkeyword(S->s, u->faction->locale)) {
				case K_ENTER:

					switch (getparam(u->faction->locale)) {
					case P_BUILDING:
					case P_GEBAEUDE:

						/* Sollte momentan nicht vorkommen, da Schwimmer nicht
						 * an Land können, und es keine Gebäude auf See gibt. */

						if( !(race[u->race].flags & RCF_WALK) &&
								!(race[u->race].flags & RCF_FLY)) {
							if (try) cmistake(u, S->s, 232, MSG_MOVE);
							S = Snext;
							continue;
						}

						b = getbuilding(r);

						if (!b) {
							if(try) cmistake(u, S->s, 6, MSG_MOVE);
							break;
						}
						if (!mayenter(r, u, b)) {
							if(try) {
								sprintf(buf, "Der Eintritt in %s wurde verwehrt",
										buildingname(b));
								mistake(u, S->s, buf, MSG_MOVE);
							}
							break;
						}
						if (!slipthru(r, u, b)) {
							if(try) {
								sprintf(buf, "%s wird belagert", buildingname(b));
								mistake(u, S->s, buf, MSG_MOVE);
							}
							break;
						}

						/* Wenn wir hier angekommen sind, war der Befehl
						 * erfolgreich und wir löschen ihn, damit er im
						 * zweiten Versuch nicht nochmal ausgeführt wird. */
						removelist(&u->orders, S);

						leave(r, u);
						u->building = b;
						if (buildingowner(r, b) == 0) {
							fset(u, FL_OWNER);
						}
						break;

					case P_SHIP:

						/* Muß abgefangen werden, sonst könnten Schwimmer an
						 * Bord von Schiffen an Land gelangen. */

						if( !(race[u->race].flags & RCF_WALK) &&
								!(race[u->race].flags & RCF_FLY)) {
							cmistake(u, S->s, 233, MSG_MOVE);
							S = Snext;
							continue;
						}

						sh = getship(r);

						if (!sh) {
							if(try) cmistake(u, S->s, 20, MSG_MOVE);
							break;
						}
						if (!mayboard(r, u, sh)) {
							if(try) cmistake(u, S->s, 34, MSG_MOVE);
							break;
						}

						/* Wenn wir hier angekommen sind, war der Befehl
						 * erfolgreich und wir löschen ihn, damit er im
						 * zweiten Versuch nicht nochmal ausgeführt wird. */
						removelist(&u->orders, S);

						leave(r, u);
						u->ship = sh;
						/* Wozu sollte das gut sein? */
						/* freset(u, FL_LEFTSHIP); */
						if (shipowner(r, sh) == 0) {
							fset(u, FL_OWNER);
						}
						break;

					default:
						if(try) cmistake(u, S->s, 79, MSG_MOVE);

					}
				}

				S = Snext;
			}
		}
	}
}

