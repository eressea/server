/* vi: set ts=2:
 *
 *	$Id: study.c,v 1.5 2001/02/04 13:20:12 corwin Exp $
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

#include "alchemy.h"
#include "item.h"
#include "faction.h"
#include "magic.h"
#include "building.h"
#include "race.h"
#include "pool.h"
#include "region.h"
#include "unit.h"
#include "skill.h"
#include "message.h"
#include "plane.h"
#include "karma.h"

/* util includes */
#include <base36.h>

/* libc includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <math.h>

#define TEACHNUMBER 10

/* ------------------------------------------------------------- */
static skill_t
getskill(void)
{
	return findskill(getstrtoken());
}

static magic_t
findmagicskill(const char *s)
{
	return (char) findstr(magietypen, s, MAXMAGIETYP);
}

magic_t
getmagicskill(void)
{
	return findmagicskill(getstrtoken());
}

/* ------------------------------------------------------------- */

static int
study_cost(unit *u, int talent)
{
	int stufe, k = 50;

	switch (talent) {
		case SK_SPY:
			return 100;
			break;
		case SK_TACTICS:
		case SK_HERBALISM:
		case SK_ALCHEMY:
			return 200;
			break;
		case SK_MAGIC:	/* Die Magiekosten betragen 50+Summe(50*Stufe) */
				/* 'Stufe' ist dabei die nächste zu erreichende Stufe */
			stufe = 1 + pure_skill(u, SK_MAGIC, u->region);
			return k*(1+((stufe+1)*stufe/2));
			break;
	}
	return 0;
}

/* ------------------------------------------------------------- */

static const attrib_type at_learning = {
	"learning",
	NULL, NULL, NULL, NULL, NULL,
	ATF_UNIQUE
};

static void
teach(region * r, unit * u)
{
	/* Parameter r gebraucht, um kontrollieren zu können, daß die Ziel-Unit auch
	 * in der selben Region ist (getunit). Lehren vor lernen. */
	static char order[BUFSIZE];
	int teaching, n, i, j, count;
	unit *u2;
	char *s;
	skill_t sk;

	if ((race[u->race].flags & RCF_NOTEACH)) {
		cmistake(u, u->thisorder, 274, MSG_EVENT);
		return;
	}

	if (r->planep && fval(r->planep, PFL_NOTEACH)) {
		cmistake(u, u->thisorder, 273, MSG_EVENT);
		return;
	}

	teaching = u->number * 30 * TEACHNUMBER;

	if ((i = get_effect(u, oldpotiontype[P_FOOL])) > 0) {	/* Trank "Dumpfbackenbrot" */
		i = min(i, u->number * TEACHNUMBER);
		/* Trank wirkt pro Schüler, nicht pro Lehrer */
		teaching -= i * 30;
		change_effect(u, oldpotiontype[P_FOOL], -i);
		j = teaching / 30;
		add_message(&u->faction->msgs, new_message(u->faction,
				"teachdumb%u:teacher%i:amount", u, j));
	}
	if (teaching == 0)
		return;

	strcpy(order, keywords[K_TEACH]);

	u2 = 0;
	count = 0;
	for (;;) {
		attrib * a;
		/* Da später in der Schleife igetkeyword (u2->thisorder) verwendet wird,
		 * muß hier wieder von vorne gelesen werden. Also merken wir uns, an
		 * welcher Stelle wir hier waren...
		 *
		 * Beispiel count = 1: LEHRE 101 102 103
		 *
		 * LEHRE und 101 wird gelesen (und ignoriert), und dann wird
		 * getunit die einheit 102 zurück liefern. */

		igetkeyword(u->thisorder);
		for (j = count; j; j--)
			getstrtoken();

		u2 = getunit(r, u);

		/* Falls keine Unit gefunden, abbrechen - außer es gibt überhaupt keine
		 * Unit, dann gibt es zusätzlich noch einen Fehler */

		if (!u2) {

			/* Finde den string, der den Fehler verursacht hat */

			igetkeyword(u->thisorder);
			for (j = count; j; j--)
				getstrtoken();

			s = getstrtoken();

			/* Falls es keinen String gibt, ist die Liste der Einheiten zuende */

			if (!s[0])
				return;

			/* Beginne die Fehlermeldung */

			strcpy(buf, "Die Einheit '");

			if (findparam(s) == P_TEMP) {
				/* Für: "Die Einheit 'TEMP ZET' wurde nicht gefunden" oder "Die Einheit
				 * 'TEMP' wurde nicht gefunden" */

				scat(s);
				s = getstrtoken();
				if (s[0])
					scat(" ");

				/* Um nachher weiter einlesen zu koennen */
				count++;
			}
			scat(s);
			scat("' wurde nicht gefunden");
			mistake(u, u->thisorder, buf, MSG_EVENT);

			count++;
			continue;
		}
		/* Defaultorder zusammenbauen. TEMP-Einheiten werden automatisch in
		 * ihre neuen Nummern übersetzt. */
		strcat(order, " ");
		strcat(order, unitid(u2));
		set_string(&u->lastorder, order);

		/* Wir müssen nun hochzählen, wieviele Einheiten wir schon abgearbeitet
		 * haben, damit mit getstrtoken() die richtige Einheit geholt werden kann.
		 * Falls u2 ein Alias hat, ist sie neu, und es wurde ein TEMP verwendet, um
		 * sie zu beschreiben. */

		count++;
		if (ualias(u2))
			count++;

		if (!ucontact(u2, u)) {
			sprintf(buf, "Einheit %s hat keinen Kontakt mit uns aufgenommen",
					unitid(u2));
			mistake(u, u->thisorder, buf, MSG_EVENT);
			continue;
		}
		i = igetkeyword(u2->thisorder);

		/* Input ist nun von u2->thisorder !! */

		if (i != K_STUDY || ((sk = getskill()) == NOSKILL)) {
			sprintf(buf, "%s lernt nicht", unitname(u2));
			mistake(u, u->thisorder, buf, MSG_EVENT);
			continue;
		}
		if (eff_skill(u, sk, r) <= eff_skill(u2, sk, r)) {
			sprintf(buf, "%s ist mindestens gleich gut wie wir", unitname(u2));
			mistake(u, u->thisorder, buf, MSG_EVENT);
			continue;
		}
		if (sk == SK_MAGIC) {
			/* ist der Magier schon spezialisiert, so versteht er nur noch
			 * Lehrer seines Gebietes */
			if (find_magetype(u2) != 0
				&& find_magetype(u) != find_magetype(u2))
			{
				sprintf(buf, "%s versteht unsere Art von Magie nicht", unitname(u2));
				mistake(u, u->thisorder, buf, MSG_EVENT);
				continue;
			}
		}
		/* learning sind die Tage, die sie schon durch andere Lehrer zugute
		 * geschrieben bekommen haben. Total darf dies nicht über 30 Tage pro Mann
		 * steigen.
		 *
		 * n ist die Anzahl zusätzlich gelernter Tage. n darf max. die Differenz
		 * von schon gelernten Tagen zum max(30 Tage pro Mann) betragen. */

		n = (u2->number * 30);;
		a = a_find(u2->attribs, &at_learning);
		if (a!=NULL) n -= a->data.i;

		n = min(n, teaching);

		if (n != 0) {
			struct building * b = inside_building(u);
			const struct building_type * btype = b?b->type:NULL;

			if (a==NULL) a = a_add(&u2->attribs, a_new(&at_learning));
			a->data.i += n;

			if (btype == &bt_academy && u2->building==u->building && inside_building(u2)!=NULL) {
				j = study_cost(u, sk);
				j = max(50, j * 2);
				if (get_pooled(u, r, R_SILVER) >= j) {		/* kann Einheit das zahlen? */
					/* Jeder Schüler zusätzlich +10 Tage wenn in Uni. */
					a->data.i += (n / 30) * 10; /* learning erhöhen */
					/* Lehrer zusätzlich +1 Tag pro Schüler. */
					set_skill(u, sk, get_skill(u, sk) + (n / 30));
				}	/* sonst nehmen sie nicht am Unterricht teil */
			}
			/* Teaching ist die Anzahl Leute, denen man noch was beibringen kann. Da
			 * hier nicht n verwendet wird, werden die Leute gezählt und nicht die
			 * effektiv gelernten Tage.
			 *
			 * Eine Einheit A von 11 Mann mit Talent 0 profitiert vom ersten Lehrer B
			 * also 10x30=300 tage, und der zweite Lehrer C lehrt für nur noch 1x30=30
			 * Tage (damit das Maximum von 11x30=330 nicht überschritten wird).
			 *
			 * Damit es aber in der Ausführung nicht auf die Reihenfolge drauf ankommt,
			 * darf der zweite Lehrer C keine weiteren Einheiten D mehr lehren. Also
			 * wird u2 30 Tage gutgeschrieben, aber teaching sinkt auf 0 (300-11x30 <=
			 * 0).
			 *
			 * Sonst träte dies auf:
			 *
			 * A: lernt B: lehrt A C: lehrt A D D: lernt
			 *
			 * Wenn B vor C dran ist, lehrt C nur 30 Tage an A (wie oben) und
			 * 270 Tage an D.
			 *
			 * Ist C aber vor B dran, lehrt C 300 tage an A, und 0 tage an D,
			 * und B lehrt auch 0 tage an A.
			 *
			 * Deswegen darf C D nie lehren dürfen.
			 */

			teaching = max(0, teaching - u2->number * 30);

			if (u->faction != u2->faction) {
				add_message(&u2->faction->msgs, new_message(u2->faction,
					"teach%u:teacher%u:student%t:skill", u, u2, sk));
				add_message(&u->faction->msgs, new_message(u->faction,
					"teach%u:teacher%u:student%t:skill", u, u2, sk));
			}
		}
	}
}
/* ------------------------------------------------------------- */

#ifdef SKILLFIX_SAVE
extern void skillfix(struct unit *, skill_t, int, int, int);
#endif

void
learn(void)
{
	region *r;
	unit *u;
	int p;
	magic_t mtyp;
	int i, l;
	int warrior_skill;
	int studycost;

	/* lernen nach lehren */

	for (r = regions; r; r = r->next)
		for (u = r->units; u; u = u->next)
			if (rterrain(r) != T_OCEAN || u->race == RC_AQUARIAN)
				if (igetkeyword(u->thisorder) == K_STUDY) {
					double multi = 1.0;
					attrib * a = NULL;
					int money = 0;
					int maxalchemy = 0;
					if (rterrain(r) == T_GLACIER && u->race == RC_INSECT
							&& !is_cursed(u->attribs, C_KAELTESCHUTZ,0)){
						continue;
					}
					if (attacked(u)) {
						cmistake(u, findorder(u, u->thisorder), 52, MSG_PRODUCE);
						continue;
					}

					i = getskill();

					if (i < 0) {
						cmistake(u, findorder(u, u->thisorder), 77, MSG_EVENT);
						continue;
					}
					/* Hack: Talente mit Malus -99 können nicht gelernt werden */
					if (race[u->race].bonus[i] == -99) {
						cmistake(u, findorder(u, u->thisorder), 77, MSG_EVENT);
						continue;
					}
					if ((race[u->race].flags & RCF_NOLEARN)) {
						sprintf(buf, "%s können nichts lernen", race[u->race].name[1]);
						mistake(u, u->thisorder, buf, MSG_EVENT);
						continue;
					} else {
						struct building * b = inside_building(u);
						const struct building_type * btype = b?b->type:NULL;

						p = studycost = study_cost(u,i);
						a = a_find(u->attribs, &at_learning);
						
						if (btype == &bt_academy) {
							studycost = max(50, studycost * 2);
						}
					}
					if (i == SK_MAGIC){
						if (u->number > 1){
							cmistake(u, findorder(u, u->thisorder), 106, MSG_MAGIC);
							continue;
						}
						if (is_familiar(u)){
							/* Vertraute zählen nicht zu den Magiern einer Partei,
							 * können aber nur Graue Magie lernen */
							mtyp = M_GRAU;
							if (!get_skill(u, SK_MAGIC)){
								create_mage(u, mtyp);
							}
						} else if (!get_skill(u, SK_MAGIC)){
							/* Die Einheit ist noch kein Magier */
							if (count_skill(u->faction, SK_MAGIC) + u->number >
								max_skill(u->faction, SK_MAGIC))
							{
								sprintf(buf, "Es kann maximal %d Magier pro Partei geben",
									max_skill(u->faction, SK_MAGIC));
								mistake(u, u->thisorder, buf, MSG_EVENT);
								continue;
							}
							mtyp = getmagicskill();
							if (mtyp == M_NONE || mtyp == M_GRAU) {
								/* wurde kein Magiegebiet angegeben, wird davon
								 * ausgegangen, daß das normal gelernt werden soll */
								if(u->faction->magiegebiet != 0) {
									mtyp = u->faction->magiegebiet;
								} else {
								/* Es wurde kein Magiegebiet angegeben und die Partei
								 * hat noch keins gewählt. */
									cmistake(u, findorder(u, u->thisorder), 178, MSG_MAGIC);
									continue;
								}
							}
							if (mtyp != u->faction->magiegebiet){
								/* Es wurde versucht, ein anderes Magiegebiet zu lernen
								 * als das der Partei */
								if (u->faction->magiegebiet != 0){
									cmistake(u, findorder(u, u->thisorder), 179, MSG_MAGIC);
									continue;
								} else {
									/* Lernt zum ersten mal Magie und legt damit das
									 * Magiegebiet der Partei fest */
									u->faction->magiegebiet = mtyp;
								}
							}
							create_mage(u, mtyp);
						} else {
							/* ist schon ein Magier und kein Vertrauter */
							if(u->faction->magiegebiet == 0){
								/* die Partei hat noch kein Magiegebiet gewählt. */
								mtyp = getmagicskill();
								if (mtyp == M_NONE){
									cmistake(u, findorder(u, u->thisorder), 178, MSG_MAGIC);
									continue;
								} else {
									/* Legt damit das Magiegebiet der Partei fest */
									u->faction->magiegebiet = mtyp;
								}
							}
						}
					}
					if (i == SK_ALCHEMY) {
						maxalchemy = get_skill(u, SK_ALCHEMY);
						if (maxalchemy==0 && count_skill(u->faction, SK_ALCHEMY) + u->number >
							max_skill(u->faction, SK_ALCHEMY)) {
							sprintf(buf, "Es kann maximal %d Alchemisten pro Partei geben",
									max_skill(u->faction, SK_ALCHEMY));
							mistake(u, u->thisorder, buf, MSG_EVENT);
							continue;
						}
					}
					if (u->race != u->faction->race
						&& (i == SK_MAGIC || i == SK_ALCHEMY || i == SK_TACTICS
							|| i == SK_HERBALISM || i == SK_SPY))
					{
						if (is_familiar(u) || (nonplayer(u) && get_skill(u, i) > 0)) {
						} else {
							sprintf(buf, "Migranten können keine kostenpflichtigen Talente lernen");
							mistake(u, u->thisorder, buf, MSG_EVENT);
							continue;
						}
					}
					if (studycost) {
						money = get_pooled(u, r, R_SILVER);
						money = min(money, studycost * u->number);
					}
					if (money < studycost * u->number) {
						studycost = p;	/* Ohne Uni? */
						money = min(money, studycost);
						if (p>0 && money < studycost * u->number) {
#ifdef PARTIAL_STUDY
							cmistake(u, findorder(u, u->thisorder), 65, MSG_EVENT);
							multi = money / (double)(studycost * u->number);
#else
							cmistake(u, findorder(u, u->thisorder), 65, MSG_EVENT);
							continue;		/* nein, Silber reicht auch so nicht */
#endif
						}
					}

					if (a==NULL) a = a_add(&u->attribs, a_new(&at_learning));

					if (money>0) {
						use_pooled(u, r, R_SILVER, money);
						add_message(&u->faction->msgs, new_message(u->faction,
								"studycost%u:unit%r:region%i:cost%t:skill",
							 	u, u->region, money, i));
					}

					if (get_effect(u, oldpotiontype[P_WISE])) {
						l = min(u->number, get_effect(u, oldpotiontype[P_WISE]));
						a->data.i += l * 10;
						change_effect(u, oldpotiontype[P_WISE], -l);
					}
					if (get_effect(u, oldpotiontype[P_FOOL])) {	/* Trank "Dumpfbackenbrot" */
						l = min(u->number, get_effect(u, oldpotiontype[P_FOOL]));
						a->data.i -= l * 30;
						change_effect(u, oldpotiontype[P_FOOL], -l);
					}

					warrior_skill = fspecial(u->faction, FS_WARRIOR);
					if(warrior_skill > 0) {
						if(i == SK_CROSSBOW || i == SK_LONGBOW
								|| i == SK_CATAPULT || i == SK_SWORD || i == SK_SPEAR
								|| i == SK_AUSDAUER || i == SK_WEAPONLESS)
						{
							a->data.i += u->number * (5+warrior_skill*5);
						} else {
							a->data.i -= u->number * (5+warrior_skill*5);
							a->data.i = max(0, a->data.i);
						}
					}

					if (p != studycost) {
						/* ist_in_gebaeude(r, u, BT_UNIVERSITAET) == 1) { */
						/* p ist Kosten ohne Uni, studycost mit; wenn
						 * p!=studycost, ist die Einheit zwangsweise
						 * in einer Uni */
						a->data.i += u->number * 10;
					}
					if (is_cursed(r->attribs,C_BADLEARN,0)) {
						a->data.i -= u->number * 10;
					}
#ifdef SKILLFIX_SAVE
					if (a && a->data.i) {
						int skill = get_skill(u, (skill_t)i);
						skillfix(u, (skill_t)i, skill, 
								 (int)(u->number * 30 * multi), a->data.i);
					}
#endif

					change_skill(u, (skill_t)i, (int)((u->number * 30 + a->data.i) * multi));
					if (a) {
						a_remove(&u->attribs, a);
						a = NULL;
					}

					/* Anzeigen neuer Tränke */
					/* Spruchlistenaktualiesierung ist in Regeneration */

					if (i == SK_ALCHEMY) {
						const potion_type * ptype;
						faction * f = u->faction;
						int skill = eff_skill(u, SK_ALCHEMY, r);
						if (skill>maxalchemy) {
							for (ptype=potiontypes; ptype; ptype=ptype->next) {
								if (skill == ptype->level * 2) {
									attrib * a = a_find(f->attribs, &at_showitem);
									while (a && a->data.v != ptype) a=a->next;
									if (!a) {
										a = a_add(&f->attribs, a_new(&at_showitem));
										a->data.v = (void*) ptype->itype;
									}
								}
							}
						}
					}
				}
}



void
teaching(void)
{
	region *r;
	unit *u;
	/* das sind alles befehle, die 30 tage brauchen, und die in thisorder
	 * stehen! von allen 30-tage befehlen wird einfach der letzte verwendet
	 * (dosetdefaults).
	 *
	 * lehren vor lernen. */

	for (r = regions; r; r = r->next) {

		for (u = r->units; u; u = u->next) {

			if (u->race == RC_SPELL || fval(u, FL_HADBATTLE))
				continue;

			if (rterrain(r) == T_OCEAN && u->race != RC_AQUARIAN)
				continue;

			if (rterrain(r) == T_GLACIER && u->race == RC_INSECT
					&& !is_cursed(u->attribs, C_KAELTESCHUTZ,0))
				continue;

			switch (igetkeyword(u->thisorder)) {

			case K_TEACH:
				if (attacked(u)){
					cmistake(u, findorder(u, u->thisorder), 52, MSG_PRODUCE);
					continue;
				}
				teach(r, u);
				break;

			}
		}
	}
}
