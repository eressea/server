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

#define TEACH_ALL 1
#define TEACH_FRIENDS 1

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
#include "rand.h"

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
getskill(const struct locale * lang)
{
	return findskill(getstrtoken(), lang);
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
/* Vertraute und Kröten sind keine Migranten */
boolean
is_migrant(unit *u)
{
	if (u->race == u->faction->race) return false;

	if (is_familiar(u)) return false;

	if (u->race == new_race[RC_TOAD]) return false;

	return true;
}

/* ------------------------------------------------------------- */
boolean
magic_lowskill(unit *u)
{
	if (u->race == new_race[RC_TOAD]) return true;
	return false;
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
			stufe = 1 + get_level(u, SK_MAGIC);
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

static int
teach_unit(unit * teacher, unit * student, int teaching, skill_t sk, 
			  boolean report, int * academy)
{
	attrib * a;
	int n;

	/* learning sind die Tage, die sie schon durch andere Lehrer zugute
	 * geschrieben bekommen haben. Total darf dies nicht über 30 Tage pro Mann
	 * steigen.
	 *
	 * n ist die Anzahl zusätzlich gelernter Tage. n darf max. die Differenz
	 * von schon gelernten Tagen zum max(30 Tage pro Mann) betragen. */

	if (magic_lowskill(student)){
		cmistake(teacher, teacher->thisorder, 292, MSG_EVENT);
		return 0;
	}

	n = student->number * 30;
	a = a_find(student->attribs, &at_learning);
	if (a!=NULL) n -= a->data.i;

	n = min(n, teaching);

	if (n != 0) {
		struct building * b = inside_building(teacher);
		const struct building_type * btype = b?b->type:NULL;

		if (a==NULL) a = a_add(&student->attribs, a_new(&at_learning));
		a->data.i += n;

		/* Solange Akademien größenbeschränkt sind, sollte Lehrer und
		 * Student auch in unterschiedlichen Gebäuden stehen dürfen */
		if (btype == &bt_academy
/*		&& student->building==teacher->building
 *		&& inside_building(student)!=NULL) */
			&& student->building && student->building->type == &bt_academy)
		{
			int j = study_cost(student, sk);
			j = max(50, j * 2);
			/* kann Einheit das zahlen? */
			if (get_pooled(student, student->region, R_SILVER) >= j) {
				/* Jeder Schüler zusätzlich +10 Tage wenn in Uni. */
				a->data.i += (n / 30) * 10; /* learning erhöhen */
				/* Lehrer zusätzlich +1 Tag pro Schüler. */
				if (academy) *academy += n;
			}	/* sonst nehmen sie nicht am Unterricht teil */
		}
		/* Teaching ist die Anzahl Leute, denen man noch was beibringen kann. Da
		 * hier nicht n verwendet wird, werden die Leute gezählt und nicht die
		 * effektiv gelernten Tage. -> FALSCH ? (ENNO)
		 *
		 * Eine Einheit A von 11 Mann mit Talent 0 profitiert vom ersten Lehrer B
		 * also 10x30=300 tage, und der zweite Lehrer C lehrt für nur noch 1x30=30
		 * Tage (damit das Maximum von 11x30=330 nicht überschritten wird).
		 *
		 * Damit es aber in der Ausführung nicht auf die Reihenfolge drauf ankommt,
		 * darf der zweite Lehrer C keine weiteren Einheiten D mehr lehren. Also
		 * wird student 30 Tage gutgeschrieben, aber teaching sinkt auf 0 (300-11x30 <=
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
		 *
		 * -> Das ist wirr. wer hat das entworfen?
		 * Besser wäre, man macht erst vorab alle zuordnungen, und dann
		 * die Talentänderung (enno).
		 */

		teaching = max(0, teaching - student->number * 30);

		if (report || teacher->faction != student->faction) {
			add_message(&student->faction->msgs, msg_message("teach",
				"teacher student skill", teacher, student, sk));
			if (teacher->faction != student->faction) {
				add_message(&teacher->faction->msgs, msg_message("teach",
					"teacher student skill", teacher, student, sk));
			}
		}
	}
	return n;
}

static void
teach(region * r, unit * u)
{
	/* Parameter r gebraucht, um kontrollieren zu können, daß die Ziel-Unit auch
	 * in der selben Region ist (getunit). Lehren vor lernen. */
	static char order[BUFSIZE];
	int teaching, i, j, count, academy=0;
	unit *u2;
	char *s;
	skill_t sk;

	if ((u->race->flags & RCF_NOTEACH)) {
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
		add_message(&u->faction->msgs, msg_message("teachdumb",
			"teacher amount", u, j));
	}
	if (teaching == 0) return;

	strcpy(order, locale_string(u->faction->locale, keywords[K_TEACH]));

	u2 = 0;
	count = 0;
#if TEACH_ALL
	if (getparam(u->faction->locale)==P_ANY) {
		unit * student = r->units;
		skill_t teachskill[MAXSKILLS];
		int i = 0;
		do {
			sk = getskill(u->faction->locale);
			teachskill[i++]=sk;
		} while (sk!=NOSKILL);
		while (teaching && student) {
			if (student->faction == u->faction && !fval(student, FL_HUNGER)) {
				if (igetkeyword(student->thisorder, student->faction->locale) == K_STUDY) {
					/* Input ist nun von student->thisorder !! */
					sk = getskill(student->faction->locale);
					if (sk!=NOSKILL && teachskill[0]!=NOSKILL) {
						for (i=0;teachskill[i]!=NOSKILL;++i) if (sk==teachskill[i]) break;
						sk = teachskill[i];
					}
					if (sk != NOSKILL && eff_skill(u, sk, r) > eff_skill(student, sk, r)) {
						teaching -= teach_unit(u, student, teaching, sk, true, &academy);
					}
				}
			}
			student = student->next;
		}
#if TEACH_FRIENDS
		while (teaching && student) {
			if (student->faction != u->faction && !fval(student, FL_HUNGER) && allied(u, student->faction, HELP_GUARD)) {
				if (igetkeyword(student->thisorder, student->faction->locale) == K_STUDY) {
					/* Input ist nun von student->thisorder !! */
					sk = getskill(student->faction->locale);
					if (sk != NOSKILL && eff_skill(u, sk, r) >= eff_skill(student, sk, r)+TEACHDIFFERENCE) {
						teaching -= teach_unit(u, student, teaching, sk, true, &academy);
					}
				}
			}
			student = student->next;
		}
#endif
	}
	else
#endif
	for (;;) {
		/* Da später in der Schleife igetkeyword (u2->thisorder) verwendet wird,
		 * muß hier wieder von vorne gelesen werden. Also merken wir uns, an
		 * welcher Stelle wir hier waren...
		 *
		 * Beispiel count = 1: LEHRE 101 102 103
		 *
		 * LEHRE und 101 wird gelesen (und ignoriert), und dann wird
		 * getunit die einheit 102 zurück liefern. */

		igetkeyword(u->thisorder, u->faction->locale);
		for (j = count; j; j--)
			getstrtoken();

		u2 = getunit(r, u->faction);

		/* Falls keine Unit gefunden, abbrechen - außer es gibt überhaupt keine
		 * Unit, dann gibt es zusätzlich noch einen Fehler */

		if (!u2) {

			/* Finde den string, der den Fehler verursacht hat */

			igetkeyword(u->thisorder, u->faction->locale);
			for (j = count; j; j--)
				getstrtoken();

			s = getstrtoken();

			/* Falls es keinen String gibt, ist die Liste der Einheiten zuende */

			if (!s[0])
				return;

			/* Beginne die Fehlermeldung */

			strcpy(buf, "Die Einheit '");

			if (findparam(s, u->faction->locale) == P_TEMP) {
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
		i = igetkeyword(u2->thisorder, u2->faction->locale);

		/* Input ist nun von u2->thisorder !! */

		if (i != K_STUDY || ((sk = getskill(u2->faction->locale)) == NOSKILL)) {
			add_message(&u->faction->msgs,
				msg_error(u, u->thisorder, "teach_nolearn", "student", u2));
			continue;
		}
		if (eff_skill(u, sk, r) <= eff_skill(u2, sk, r)) {
			add_message(&u->faction->msgs,
				msg_error(u, u->thisorder, "teach_asgood", "student", u2));
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

		teaching -= teach_unit(u, u2, teaching, sk, false, &academy);

	}
	if (academy) {
#if SKILLPOINTS
		change_skill(u, sk, academy / 30);
#else
		academy = academy/30;
		learn_skill(u, sk, academy/30.0/TEACHNUMBER);
#endif
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

	for (r = regions; r; r = r->next) {
		for (u = r->units; u; u = u->next) {
			int days;
			if (rterrain(r) == T_OCEAN){
				/* sonderbehandlung aller die auf Ozeanen lernen können */
				if (u->race != new_race[RC_AQUARIAN]
					&& !(u->race->flags & RCF_SWIM)) {
					continue;
				}
			}
			if (igetkeyword(u->thisorder, u->faction->locale) == K_STUDY) {
				double multi = 1.0;
				attrib * a = NULL;
				int money = 0;
				int maxalchemy = 0;
				if (u->race == new_race[RC_INSECT] && r_insectstalled(r)
						&& !is_cursed(u->attribs, C_KAELTESCHUTZ,0)) {
					continue;
				}
				if (attacked(u)) {
					cmistake(u, findorder(u, u->thisorder), 52, MSG_PRODUCE);
					continue;
				}
				if ((u->race->flags & RCF_NOLEARN)) {
					sprintf(buf, "%s können nichts lernen", LOC(default_locale, rc_name(u->race, 1)));
					mistake(u, u->thisorder, buf, MSG_EVENT);
					continue;
				} 

				i = getskill(u->faction->locale);

				if (i < 0) {
					cmistake(u, findorder(u, u->thisorder), 77, MSG_EVENT);
					continue;
				}
				/* Hack: Talente mit Malus -99 können nicht gelernt werden */
				if (u->race->bonus[i] == -99) {
					cmistake(u, findorder(u, u->thisorder), 77, MSG_EVENT);
					continue;
				}
				/* snotlings können Talente nur bis T8 lernen */
				if (u->race == new_race[RC_SNOTLING]){
					if (get_level(u, (skill_t)i) >= 8){
						cmistake(u, findorder(u, u->thisorder), 308, MSG_EVENT);
						continue;
					}
				}

				p = studycost = study_cost(u,i);
				a = a_find(u->attribs, &at_learning);

				/* keine kostenpflichtigen Talente für Migranten. Vertraute sind
				 * keine Migranten, wird in is_migrant abgefangen. Vorsicht,
				 * studycost darf hier noch nicht durch Akademie erhöht sein */
				if (is_migrant(u) && studycost > 0){
					sprintf(buf, "Migranten können keine kostenpflichtigen Talente lernen");
					mistake(u, u->thisorder, buf, MSG_EVENT);
					continue;
				}

				/* Akademie: */
				{
					struct building * b = inside_building(u);
					const struct building_type * btype = b?b->type:NULL;

					if (btype == &bt_academy) {
						studycost = max(50, studycost * 2);
					}
				}

				if (i == SK_MAGIC) {
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
					maxalchemy = eff_skill(u, SK_ALCHEMY, r);
					if (get_skill(u, SK_ALCHEMY)==0
						&& count_skill(u->faction, SK_ALCHEMY) + u->number >
						max_skill(u->faction, SK_ALCHEMY)) {
						sprintf(buf, "Es kann maximal %d Alchemisten pro Partei geben",
								max_skill(u->faction, SK_ALCHEMY));
						mistake(u, u->thisorder, buf, MSG_EVENT);
						continue;
					}
				}
				if (studycost) {
					money = get_pooled(u, r, R_SILVER);
					money = min(money, studycost * u->number);
				}
				if (money < studycost * u->number) {
					studycost = p;	/* Ohne Univertreurung */
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
					add_message(&u->faction->msgs, msg_message("studycost",
						"unit region cost skill", u, u->region, money, i));
				}

				if (get_effect(u, oldpotiontype[P_WISE])) {
					l = min(u->number, get_effect(u, oldpotiontype[P_WISE]));
					a->data.i += l * 10;
					change_effect(u, oldpotiontype[P_WISE], -l);
				}
				if (get_effect(u, oldpotiontype[P_FOOL])) {
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

				days = (int)((u->number * 30 + a->data.i) * multi);
				if (fval(u, FL_HUNGER)) days = days / 2;
#if SKILLPOINTS
				change_skill(u, (skill_t)i, days);
#else
				while (days) {
					if (days>=u->number*30) {
						learn_skill(u, (skill_t)i, 1.0);
						days -= u->number*30;
					} else {
						double chance = (double)days/u->number/30;
						learn_skill(u, (skill_t)i, chance);
						days = 0;
					}
				}
#endif
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
								while (a && a->data.v != ptype) a=a->nexttype;
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

			if (u->race == new_race[RC_SPELL] || fval(u, FL_LONGACTION))
				continue;

			if (rterrain(r) == T_OCEAN
					&& u->race != new_race[RC_AQUARIAN]
					&& !(u->race->flags & RCF_SWIM))
					continue;

			if (u->race == new_race[RC_INSECT] && r_insectstalled(r)
					&& !is_cursed(u->attribs, C_KAELTESCHUTZ,0)) {
				continue;
			}

			switch (igetkeyword(u->thisorder, u->faction->locale)) {

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
