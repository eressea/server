/* vi: set ts=2:
 *
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

#include "magic.h"
#include "item.h"
#include "region.h"
#include <faction.h>
#include <building.h>
#include "alliance.h"
#include "race.h"
#include "unit.h"
#include "skill.h"
#include "goodies.h"
#include "ship.h"

/* util includes */
#include <base36.h>

/* libc includes */
#include <math.h>

static attrib_type a_score = {
	"score"
};

void
init_scores(void)
{
	int i;

	for (i = 0;olditemtype[i];i++) {
		const luxury_type * ltype;
		item_type * itype = olditemtype[i];
		attrib * a = a_add(&itype->rtype->attribs, a_new(&a_score));
		switch (i) {
		case I_KEKS:
		case I_APFEL:
		case I_NUSS:
		case I_MANDELKERN:
			break;
		case I_IRON:
		case I_WOOD:
		case I_STONE:
		case I_HORSE:
			a->data.i = 10;
			break;
		case I_MALLORN:
			a->data.i = 30;
			break;
		case I_LAEN:
			a->data.i = 100;
			break;
		case I_WAGON:
			a->data.i = 60;
			break;
		case I_CATAPULT:
			a->data.i = 200;
			break;
		case I_SWORD:
		case I_SPEAR:
		case I_CROSSBOW:
		case I_LONGBOW:
		case I_LANCE:
		case I_HALBERD:
		case I_GREATSWORD:
		case I_AXE:
		case I_SHIELD:
			a->data.i = 20;
			break;
		case I_GREATBOW:
			a->data.i = 50;
			break;
		case I_LAENSHIELD:
		case I_LAENSWORD:
			a->data.i = 400;
			break;
		case I_LAENCHAIN:
			a->data.i = 1000;
			break;
		case I_CHAIN_MAIL:
			a->data.i = 40;
			break;
		case I_PLATE_ARMOR:
			a->data.i = 60;
			break;
		case I_BALM:
		case I_SPICES:
		case I_JEWELERY:
		case I_MYRRH:
		case I_OIL:
		case I_SILK:
		case I_INCENSE:
			ltype = resource2luxury(itype->rtype);
			if (ltype) a->data.i = ltype->price / 5;
			break;
#ifdef COMPATIBILITY
		case I_AMULET_OF_DARKNESS:
		case I_AMULET_OF_DEATH:
		case I_SHIELDSTONE:
		case I_STAFF_OF_FIRE:
		case I_STAFF_OF_LIGHTNING:
		case I_WAND_OF_TELEPORTATION:
		case I_CLOAK_OF_INVULNERABILITY:
#endif
		case I_AMULET_OF_HEALING:
		case I_AMULET_OF_TRUE_SEEING:
		case I_RING_OF_INVISIBILITY:
		case I_RING_OF_POWER:
		case I_RUNESWORD:
		case I_CHASTITY_BELT:
		case I_FIRESWORD:
		case I_TROLLBELT:
		case I_RING_OF_NIMBLEFINGER:
		case I_FEENSTIEFEL:
			a->data.i = 6000;
			break;
		case I_ANTIMAGICCRYSTAL:
			a->data.i = 2000;
			break;
		}
	}
}

int
average_score_of_age(int age, int a)
{
	faction *f;
	int sum = 0, count = 0;

	for (f = factions; f; f = f->next) {
		if (f->no != MONSTER_FACTION && f->race != new_race[RC_TEMPLATE] && f->age <= age + a && f->age >= age - a) {
			sum += f->score;
			count++;
		}
	}

	if (count == 0) {
		return 0;
	}
	return sum / count;
}

void
score(void)
{
	FILE *scoreFP;
	region *r;
	unit *u, *u2;
	faction *f, *fbo;
	building *b;
	ship *s;
	int allscores = 0;
	int c;
	static boolean init = false;

	if (!init) {
		init=true;
		init_scores();
	}
	for (f = factions; f; f = f->next) f->score = 0;

	for (r = regions; r; r = r->next) {
		for (b = r->buildings; b; b = b->next) {
			if ((u = buildingowner(r, b)) != 0) {
				fbo = u->faction;

				if (u->number <= b->size)
					fbo->score += u->number * 5;

				fbo->score += b->size;

				c = b->size - u->number;

				for (u2 = r->units; u2 && c > 0; u2 = u2->next) {
					if (u2->building == b && u2 != u && u2->number <= c) {
						c -= u2->number;

						if (u2->faction == fbo) {
							u2->faction->score += u2->number * 5;
						} else {
							u2->faction->score += u2->number * 3;
						}
					}
				}
			}
		}
		for (s = r->ships; s; s = s->next) {
			if ((u = shipowner(r, s)) != 0) {
				u->faction->score += s->size * 2;
			}
		}
		for (u = r->units; u; u = u->next) {
			char index;
			item * itm;
			if (u->race == new_race[RC_SPELL] || u->race == new_race[RC_BIRTHDAYDRAGON])
				continue;

			f = u->faction;

			if (old_race(u->race) <= RC_AQUARIAN) {
				f->score += (u->race->recruitcost * u->number) / 50;
			}
			f->score += get_money(u) / 50;
			for (itm=u->items; itm; itm=itm->next) {
				attrib * a = a_find(itm->type->rtype->attribs, &a_score);
				if (a!=NULL) f->score += itm->number * a->data.i / 10;
			}

			for (index = 0; index != MAXSKILLS; index++) {
				switch (index) {
				case SK_MAGIC:
					f->score += u->number * ((int) pow((double) eff_skill(u, index, r),
													   (double) 4));
					break;
				case SK_TACTICS:
					f->score += u->number * ((int) pow((double) eff_skill(u, index, r),
													   (double) 3));
					break;
				case SK_SPY:
				case SK_ALCHEMY:
				case SK_HERBALISM:
					f->score += u->number * ((int) pow((double) eff_skill(u, index, r),
													   (double) 2.5));
					break;
				default:
					f->score += u->number * ((int) pow((double) eff_skill(u, index, r),
													   (double) 2.5)) / 10;
					break;
				}
			}
		}
	}

	for (f = factions; f; f = f->next) {
		f->score = f->score / 5;
		if (f->no != MONSTER_FACTION && f->race != new_race[RC_TEMPLATE])
			allscores += f->score;
	}
	if( allscores == 0 )
			allscores = 1;

	sprintf(buf, "%s/score", basepath());
	scoreFP = fopen(buf, "w");
	for (f = factions; f; f = f->next) if(f->number != 0) {
		fprintf(scoreFP, "%8d (%8d/%4.2f%%/%5.2f) %30.30s (%3.3s) %5s (%3d)\n",
			f->score, f->score - average_score_of_age(f->age, f->age / 24 + 1),
		      ((float) f->score / (float) allscores) * 100.0,
					(float) f->score / f->number,
					f->name, LOC(default_locale, rc_name(f->race, 0)), factionid(f), f->age);
	}
	fclose(scoreFP);

#ifdef ALLIANCES
	{
		alliance *a;

		sprintf(buf, "%s/score.alliances", basepath());
		scoreFP = fopen(buf, "w");
		fprintf(scoreFP, "# alliance:factions:persons:score\n");

		for (a = alliances; a; a = a->next) {
			int alliance_score = 0, alliance_number = 0, alliance_factions = 0;

			for (f = factions; f; f = f->next) {
				if(f->alliance && f->alliance->id == a->id) {
					alliance_factions++;
					alliance_score  += f->score;
					alliance_number += f->number;
				}
			}

			fprintf(scoreFP, "%d:%d:%d:%d\n", a->id, alliance_factions, alliance_number, alliance_score);
		}
		fclose(scoreFP);
	}
#endif

}

