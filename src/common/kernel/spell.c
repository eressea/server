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

/* kernel includes */
/* FIXME: brauchen wir die wirklich alle? */
#include "battle.h" /* für lovar */
#include "border.h"
#include "building.h"
#include "curse.h"
#include "faction.h"
#include "goodies.h"
#include "item.h"
#include "karma.h"
#include "magic.h"
#include "message.h"
#include "objtypes.h"
#include "plane.h"
#include "pool.h"
#include "race.h"
#include "region.h"
#include "resolve.h"
#include "ship.h"
#include "skill.h"
#include "spy.h"
#include "teleport.h"
#include "terrain.h"
#include "unit.h"

#include "spell.h"

/* spells includes */
#include <spells/alp.h>

/* util includes */
#include <base36.h>
#include <message.h>
#include <event.h>
#include <rand.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* triggers includes */
#include <triggers/changefaction.h>
#include <triggers/changerace.h>
#include <triggers/createcurse.h>
#include <triggers/createunit.h>
#include <triggers/killunit.h>
#include <triggers/timeout.h>
#include <triggers/unitmessage.h>

/* attributes includes */
#include <attributes/targetregion.h>
#include <attributes/hate.h>
/* ----------------------------------------------------------------------- */
attrib_type at_deathcloud = {
	"zauber_todeswolke", NULL, NULL, NULL, NULL, NULL
};

attrib_type at_unitdissolve = {
	"unitdissolve", NULL, NULL, NULL, a_writedefault, a_readdefault
};

/* ----------------------------------------------------------------------- */

#ifdef TODO
static const char *
spellcmd(const strarray * sa, const struct locale * lang) {
	int i;
	char * p = buf;
	strcpy(p, locale_string(lang, keywords[K_CAST]));
	p += strlen(p);
	for (i=0;i!=sa->length;++i) {
		*p++ = ' ';
		strcpy(p, sa->strings[i]);
		p += strlen(p);
	}
	return buf;
}

void
report_failure(unit * mage, const strarray * sa) {
	/* Fehler: "Der Zauber schlägt fehl" */
	cmistake(mage, strdup(spellcmd(sa, mage->faction->locale)), 180, MSG_MAGIC);
}
#else
void
report_failure(unit * mage, const char * sa) {
	/* Fehler: "Der Zauber schlägt fehl" */
	cmistake(mage, strdup(sa), 180, MSG_MAGIC);
}
#endif

/* ------------------------------------------------------------- */
/* do_shock - Schockt die Einheit, z.B. bei Verlust eines        */
/* Vertrauten.                                                   */
/* ------------------------------------------------------------- */

void
do_shock(unit *u, char *reason)
{
#if SKILLPOINTS
	skill_t sk;
#else
	int i;
#endif
	if(u->number == 0) return;

	/* HP - Verlust */
	u->hp = (unit_max_hp(u) * u->number)/10;
	u->hp = max(1, u->hp);
	/* Aura - Verlust */
	if(is_mage(u)) {
		set_spellpoints(u, max_spellpoints(u->region,u)/10);
	}

	/* Evt. Talenttageverlust */
#if SKILLPOINTS
	if(rand()%10 < 2) {
		for (sk=0; sk < MAXSKILLS; sk++) {
			int n = get_skill(u, sk);
			if (n!=0) set_skill(u, sk, (n*9)/10, false);
		}
	}
#else
	for (i=0;i!=u->skill_size;++i) if (rand()%5==0) {
		skill * sv = u->skills+i;
		int weeks = (sv->level * sv->level - sv->level) / 2;
		int change = (weeks+9) / 10;
		reduce_skill(u, sv, change);
	}
#endif

	/* Dies ist ein Hack, um das skillmod und familiar-Attribut beim Mage
	 * zu löschen wenn der Familiar getötet wird. Da sollten wir über eine
	 * saubere Implementation nachdenken. */

	if(!strcmp(reason, "trigger")) {
		remove_familiar(u);
	}

	add_message(&u->faction->msgs, new_message(u->faction,
		"shock%u:mage%s:reason", u, strdup(reason)));
}

/* ------------------------------------------------------------- */
/* Spruchanalyse - Ausgabe von curse->info und curse->name       */
/* ------------------------------------------------------------- */

void
magicanalyse_region(region *r, unit *mage, int force)
{
	attrib *a;
	boolean found = false;

	for (a=r->attribs;a;a=a->next) {
		curse * c;
		int chance;
		int mon;
		if (!fval(a->type, ATF_CURSE)) continue;

		c = (curse*)a->data.v;

		/* ist der curse schwächer als der Analysezauber, so ergibt sich
		 * mehr als 100% chance und damit immer ein Erfolg. */
		chance = (force - c->vigour)*10 + 100;
		mon = c->duration + (rand()%10) - 5;
		mon = max(1,mon);
		found = true;

		if(rand()%100 < chance){ /* Analyse geglückt */
			if(c->flag & CURSE_NOAGE){
				add_message(&mage->faction->msgs, new_message(mage->faction,
					"analyse_region_noage%u:mage%r:region%s:spell",
					mage, r, c->type->name));
			}else{
				add_message(&mage->faction->msgs, new_message(mage->faction,
					"analyse_region_age%u:mage%r:region%s:spell%i:months",
					mage, r, c->type->name, mon));
			}
		} else {
			add_message(&mage->faction->msgs, new_message(mage->faction,
				"analyse_region_fail%u:mage%r:region", mage, r));
		}
	}
	if (!found) {
		add_message(&mage->faction->msgs, new_message(mage->faction,
			"analyse_region_nospell%u:mage%r:region", mage, r));
	}
}

void
magicanalyse_unit(unit *u, unit *mage, int force)
{
	attrib *a;
	boolean found = false;

	for (a=u->attribs;a;a=a->next) {
		curse * c;
		int chance;
		int mon;
		if (!fval(a->type, ATF_CURSE)) continue;

		c = (curse*)a->data.v;
		/* ist der curse schwächer als der Analysezauber, so ergibt sich
		 * mehr als 100% chance und damit immer ein Erfolg. */
		chance = (force - c->vigour)*10 + 100;
		mon = c->duration + (rand()%10) - 5;
		mon = max(1,mon);

		if(rand()%100 < chance){ /* Analyse geglückt */
			if(c->flag & CURSE_NOAGE){
				add_message(&mage->faction->msgs, new_message(mage->faction,
					"analyse_unit_noage%u:mage%u:unit%s:spell",
					mage, u, c->type->name));
			}else{
				add_message(&mage->faction->msgs, new_message(mage->faction,
					"analyse_unit_age%u:mage%u:unit%s:spell%i:months",
					mage, u, c->type->name, mon));
			}
		} else {
			add_message(&mage->faction->msgs, new_message(mage->faction,
				"analyse_unit_fail%u:mage%u:unit", mage, u));
		}
	}
	if (!found) {
		add_message(&mage->faction->msgs, new_message(mage->faction,
			"analyse_unit_nospell%u:mage%u:target", mage, u));
	}
}

void
magicanalyse_building(building *b, unit *mage, int force)
{
	attrib *a;
	boolean found = false;

	for (a=b->attribs;a;a=a->next) {
		curse * c;
		int chance;
		int mon;
		if (!fval(a->type, ATF_CURSE)) continue;

		c = (curse*)a->data.v;
		/* ist der curse schwächer als der Analysezauber, so ergibt sich
		 * mehr als 100% chance und damit immer ein Erfolg. */
		chance = (force - c->vigour)*10 + 100;
		mon = c->duration + (rand()%10) - 5;
		mon = max(1,mon);

		if(rand()%100 < chance){ /* Analyse geglückt */
			if(c->flag & CURSE_NOAGE){
				add_message(&mage->faction->msgs, new_message(mage->faction,
					"analyse_building_age%u:mage%b:building%s:spell",
					mage, b, c->type->name));
			}else{
				add_message(&mage->faction->msgs, new_message(mage->faction,
					"analyse_building_age%u:mage%b:building%s:spell%i:months",
					mage, b, c->type->name, mon));
			}
		} else {
			add_message(&mage->faction->msgs, new_message(mage->faction,
				"analyse_building_fail%u:mage%b:building", mage, b));
		}
	}
	if (!found) {
		add_message(&mage->faction->msgs, new_message(mage->faction,
			"analyse_building_nospell%u:mage%b:building", mage, b));
	}

}

void
magicanalyse_ship(ship *sh, unit *mage, int force)
{
	attrib *a;
	boolean found = false;

	for (a=sh->attribs;a;a=a->next) {
		curse * c;
		int chance;
		int mon;
		if (!fval(a->type, ATF_CURSE)) continue;

		c = (curse*)a->data.v;
		/* ist der curse schwächer als der Analysezauber, so ergibt sich
		 * mehr als 100% chance und damit immer ein Erfolg. */
		chance = (force - c->vigour)*10 + 100;
		mon = c->duration + (rand()%10) - 5;
		mon = max(1,mon);

		if(rand()%100 < chance){ /* Analyse geglückt */
			if(c->flag & CURSE_NOAGE){
				add_message(&mage->faction->msgs, new_message(mage->faction,
					"analyse_ship_noage%u:mage%h:ship%s:spell",
					mage, sh, c->type->name));
			}else{
				add_message(&mage->faction->msgs, new_message(mage->faction,
					"analyse_ship_age%u:mage%h:ship%s:spell",
					mage, sh, c->type->name, mon));
			}
		} else {
			add_message(&mage->faction->msgs, new_message(mage->faction,
				"analyse_ship_fail%u:mage%h:ship", mage, sh));

		}
	}
	if (!found) {
		add_message(&mage->faction->msgs, new_message(mage->faction,
			"analyse_ship_nospell%u:mage%h:ship", mage, sh));
	}

}

/* ------------------------------------------------------------- */
/* Antimagie - curse auflösen */
/* ------------------------------------------------------------- */
int
destroy_curse(attrib **alist, int cast_level, int force,
		const curse_type * ctype)
{
	int succ = 0;
/*	attrib **a = a_find(*ap, &at_curse); */
	attrib ** ap = alist;

	while (*ap && force > 0) {
		curse * c;
		attrib * a = *ap;
		if (!fval(a->type, ATF_CURSE)) {
			do { ap = &(*ap)->next; } while (*ap && a->type==(*ap)->type);
			continue;
		}
		c = (curse*)a->data.v;

		/* Immunität prüfen */
		if (c->flag & CURSE_IMMUN) {
			do { ap = &(*ap)->next; } while (*ap && a->type==(*ap)->type);
			continue;
		}

		if(!ctype || c->type==ctype) { /* wirkt auf alle */
			if (cast_level < c->vigour) { /* Zauber ist nicht stark genug */
				int chance;
				/* pro Stufe Unterschied -20% */
				chance = (10 + (cast_level - c->vigour)*2);
				if(rand()%10 >= chance){
					if (c->type->change_vigour)
						c->type->change_vigour(c, -2);
					force -= c->vigour;
					c->vigour -= 2;
					if(c->vigour <= 0) {
						a_remove(alist, a);
					}
					succ = cast_level;
				}
			} else { /* Zauber ist stärker als curse */
				if (force >= c->vigour){ /* reicht die Kraft noch aus? */
					force -= c->vigour;
					if (c->type->change_vigour)
						c->type->change_vigour(c, -c->vigour);
					a_remove(alist, a);
					succ = cast_level;
				}
			}
		}
		if(*ap) ap = &(*ap)->next;
	}
	return succ;
}

/* ------------------------------------------------------------- */
/* Report a spell's effect to the units in the region.
*/
static void
report_effect(region * r, unit * mage, message * seen, message * unseen)
{
	unit * u;

	/* melden, 1x pro Partei */
	freset(mage->faction, FL_DH);
	for (u = r->units; u; u = u->next ) freset(u->faction, FL_DH);
	for (u = r->units; u; u = u->next ) {
		if (!fval(u->faction, FL_DH) ) {
			fset(u->faction, FL_DH);

			/* Bei Fernzaubern sieht nur die eigene Partei den Magier */
			if (u->faction != mage->faction){
				if (r == mage->region){
					/* kein Fernzauber, prüfe, ob der Magier überhaupt gesehen
					 * wird */
					if (cansee(u->faction, r, mage, 0)) {
						r_addmessage(r, u->faction, seen);
					} else {
						r_addmessage(r, u->faction, unseen);
					}
				} else { /* Fernzauber, fremde Partei sieht den Magier niemals */
					r_addmessage(r, u->faction, unseen);
				}
			} else { /* Partei des Magiers, sieht diesen immer */
				r_addmessage(r, u->faction, seen);
			}
		}
	}
	/* Ist niemand von der Partei des Magiers in der Region, dem Magier
	 * nochmal gesondert melden */
	if(!fval(mage->faction, FL_DH)) {
		add_message(&mage->faction->msgs, seen);
	}
	msg_release(seen);
	if (unseen) msg_release(unseen);
}

/* ------------------------------------------------------------- */
/* Die Spruchfunktionen */
/* ------------------------------------------------------------- */
/* Meldungen:
 *
 * Fehlermeldungen sollten als MSG_MAGIC, level ML_MISTAKE oder
 * ML_WARN ausgegeben werden. (stehen im Kopf der Auswertung unter
 * Zauberwirkungen)

	sprintf(buf, "%s in %s: 'ZAUBER \"%s\"': [hier die Fehlermeldung].",
		unitname(mage), regionid(mage->region), sa->strings[0]);
	add_message(0, mage->faction, buf,  MSG_MAGIC, ML_MISTAKE);

 * Allgemein sichtbare Auswirkungen in der Region sollten als
 * Regionsereignisse auch dort auftauchen.

	{
		message * seen = msg_message("harvest_effect", "mage", mage);
		message * unseen = msg_message("harvest_effect", "mage", NULL);
		report_effect(r, mage, seen, unseen);
	}

 * Meldungen an den Magier über Erfolg sollten, wenn sie nicht als
 * Regionsereigniss auftauchen, als MSG_MAGIC level ML_INFO unter
 * Zauberwirkungen gemeldet werden. Direkt dem Magier zuordnen (wie
 * Botschaft an Einheit) ist derzeit nicht möglich.
 * ACHTUNG! r muss nicht die Region des Magier sein! (FARCASTING)
 *
 * Parameter:
 * die Struct castorder *co ist in magic.h deklariert
 * die Parameterliste spellparameter *pa = co->par steht dort auch.
 *
 */

/* ------------------------------------------------------------- */
/* Name:		Vertrauter
 * Stufe:		10
 *
 * Wirkung:
 * Der Magier beschwört einen Vertrauten, ein kleines Tier, welches
 * dem Magier zu Diensten ist.  Der Magier kann durch die Augen des
 * Vertrauten sehen, und durch den Vertrauten zaubern, allerdings nur
 * mit seiner halben Stufe. Je nach Vertrautem erhält der Magier
 * evtl diverse Skillmodifikationen.  Der Typ des Vertrauten ist
 * zufällig bestimmt, wird aber durch Magiegebiet und Rasse beeinflußt.
 * "Tierische" Vertraute brauchen keinen Unterhalt.
 *
 * Ein paar Möglichkeiten:
 * 			Magieg.	Rasse	Besonderheiten
 * Eule		Tybied	-/-		fliegt, Auraregeneration
 * Rabe 	Ilaun	-/-		fliegt
 * Imp		Draig	-/-		Magieresistenz?
 * Fuchs	Gwyrrd	-/-		Wahrnehmung
 * ????		Cerddor	-/-		???? (Singvogel?, Papagei?)
 * Adler	-/-		-/-		fliegt, +Wahrnehmung, =^=Adlerauge-Spruch?
 * Krähe	-/-		-/-		fliegt, +Tarnung (weil unauffällig)
 * Delphin	-/-		Meerm.	schwimmt
 * Wolf		-/-		Ork
 * Hund		-/-		Mensch	kann evtl BEWACHE ausführen
 * Ratte	-/-		Goblin
 * Albatros	-/-		-/-		fliegt, kann auf Ozean "landen"
 * Affe		-/-		-/-		kann evtl BEKLAUE ausführen
 * Goblin	-/-		!Goblin	normale Einheit
 * Katze	-/-		!Katze	normale Einheit
 * Dämon	-/-		!Dämon	normale Einheit
 *
 * Spezielle V. für Katzen, Trolle, Elfen, Dämonen, Insekten, Zwerge?
 */

static const race *
select_familiar(const race * magerace, magic_t magiegebiet)
{
	int rnd = rand()%100;
	assert(magerace->familiars[0]);

	if (rnd < 3) {
		/* RC_KRAKEN muß letzter Vertraute sein */
		return new_race[(race_t)(RC_HOUSECAT + rand()%(RC_KRAKEN+1-RC_HOUSECAT))];
	} else if (rnd < 80) {
		return magerace->familiars[0];
	}
	return magerace->familiars[magiegebiet];
}

/* ------------------------------------------------------------- */
/* der Vertraue des Magiers */

boolean
is_familiar(const unit *u)
{
	return i2b(get_familiar_mage(u)!=NULL);
}

static void
make_familiar(unit *familiar, unit *mage)
{
	/* skills and spells: */
	familiar->race->init_familiar(familiar);

	/* triggers: */
	create_newfamiliar(mage, familiar);

	/* Hitpoints nach Talenten korrigieren, sonst starten vertraute
	 * mit Ausdauerbonus verwundet */
	familiar->hp = unit_max_hp(familiar);
}

static int
sp_summon_familiar(castorder *co)
{
	unit *familiar;
	region *r = co->rt;
	region *target_region = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	const race * rc;
	skill_t sk;
	int dh, dh1;
	direction_t d;
	if (get_familiar(mage) != NULL ) {
		cmistake(mage, strdup(co->order), 199, MSG_MAGIC);
		return 0;
	}
	rc = select_familiar(mage->faction->race, mage->faction->magiegebiet);

	if (fval(rc, RCF_SWIM) && !fval(rc, RCF_WALK)) {
		int coasts = is_coastregion(r);

		if (coasts == 0) {
			cmistake(mage, strdup(co->order), 229, MSG_MAGIC);
			return 0;
		}

		/* In welcher benachbarten Ozeanregion soll der Familiar erscheinen? */
		coasts = rand()%coasts;
		dh = -1;
		for(d=0; d<MAXDIRECTIONS; d++) {
			if(rconnect(r,d)->terrain == T_OCEAN) {
				dh++;
				if(dh == coasts) break;
			}
		}
		target_region = rconnect(r,d);
	}

	familiar = createunit(target_region, mage->faction, 1, rc);
	if (target_region==mage->region) {
		familiar->building = mage->building;
		familiar->ship = mage->ship;
	}
	familiar->status = ST_FLEE;	/* flieht */
	sprintf(buf, "Vertrauter von %s", unitname(mage));
	set_string(&familiar->name, buf);
	if (fval(mage, FL_PARTEITARNUNG)) fset(familiar, FL_PARTEITARNUNG);
	fset(familiar, FL_LOCKED);
	make_familiar(familiar, mage);

	dh = 0;
	dh1 = 0;
	sprintf(buf, "%s ruft einen Vertrauten. %s können ", 
		unitname(mage), LOC(mage->faction->locale, rc_name(rc, 1)));
	for(sk=0;sk<MAXSKILLS;sk++){
		if(rc->bonus[sk] > -5) dh++;
	}
	for(sk=0;sk<MAXSKILLS;sk++) {
		if(rc->bonus[sk] > -5){
			dh--;
			if (dh1 == 0){
				dh1 = 1;
			} else {
				if (dh == 0){
					scat(" und ");
				} else {
					scat(", ");
				}
			}
			scat(skillname(sk, mage->faction->locale));
		}
	}
	scat(" lernen.");
	scat(" ");
	scat("Der Vertraute verleiht dem Magier einen Bonus auf jedes Talent ");
	scat("(ausgenommen Magie), welches der Vertraute beherrscht.");
	scat(" ");
	scat("Das spezielle Band zu seinem Vertrauten ermöglicht dem Magier ");
	scat("auch, Sprüche durch diesen zu wirken. So gezauberte Sprüche ");
	scat("wirken auf die Region des Vertrauten und brauchen keine Fernzauber ");
	scat("zu sein. Die maximale Entfernung dafür entspricht dem Talent des ");
	scat("Magiers. Einen Spruch durch das Vertrautenband zu richten ist ");
	scat("jedoch gewissen Einschränkungen unterworfen. Die Stufe des Zaubers ");
	scat("kann nicht größer als das Magietalent des Vertrauten oder das halbe ");
	scat("Talent des Magiers sein. Auch verdoppeln sich die Kosten für den ");
	scat("Spruch. (Um einen Zauber durch den Vertrauten zu wirken, gibt ");
	scat("man statt dem Magier dem Vertrauten den Befehl ZAUBERE.)");

	addmessage(r, mage->faction, buf, MSG_MAGIC, ML_INFO);
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:           Zerstöre Magie
 * Wirkung:
 *  Zerstört alle Zauberwirkungen auf dem Objekt. Jeder gebrochene
 *  Zauber verbraucht c->vigour an Zauberkraft. Wird der Spruch auf
 *  einer geringeren Stufe gezaubert, als der Zielzauber an c->vigour
 *  hat, so schlägt die Auflösung mit einer von der Differenz abhängigen
 *  Chance fehl.  Auch dann wird force verbraucht, der Zauber jedoch nur
 *  abgeschwächt.
 *
 * Flag:
 *  (FARCASTING|SPELLLEVEL|ONSHIPCAST|TESTCANSEE)
 * */
static int
sp_destroy_magic(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;
	spellparameter *pa = co->par;
	const curse_type * ctype;
	char ts[80];
	attrib **ap;
	int obj;
	int succ;

	/* da jeder Zauber force verbraucht und der Zauber auf alles und nicht
	 * nur einen Spruch wirken soll, wird die Wirkung hier verstärkt */
	force *= 4;

	/* Objekt ermitteln */
	obj = pa->param[0]->typ;
	ctype = NULL;

	switch(obj) {
		case SPP_REGION:
		{
			region *tr = pa->param[0]->data.r;
			ap = &tr->attribs;
			strcpy(ts, regionid(tr));
			break;
		}
		case SPP_UNIT:
		{
			unit *u;
			u = pa->param[0]->data.u;
			ap = &u->attribs;
			strcpy(ts, unitname(u));
			break;
		}
		case SPP_BUILDING:
		{
			building *b;
			b = pa->param[0]->data.b;
			ap = &b->attribs;
			strcpy(ts, buildingname(b));
			break;
		}
		case SPP_SHIP:
		{
			ship *sh;
			sh = pa->param[0]->data.sh;
			ap = &sh->attribs;
			strcpy(ts, shipname(sh));
			break;
		}
		default:
			return 0;
	}

  succ = destroy_curse(ap, cast_level, force, ctype);

	if(succ) {
		add_message(&mage->faction->msgs, new_message(mage->faction,
			"destroy_magic_effect%u:unit%r:region%s:command%i:succ%s:target",
			mage, mage->region, strdup(co->order), succ, strdup(ts)));
	} else {
		add_message(&mage->faction->msgs, new_message(mage->faction,
			"destroy_magic_noeffect%u:unit%r:region%s:command",
			mage, mage->region, strdup(co->order)));
	}

	return max(succ, 1);
}

/* ------------------------------------------------------------- */
/* Name:          Transferiere Aura
 * Stufe:         variabel
 * Gebiet:        alle
 * Kategorie:     Einheit, positiv
 * Wirkung:
 *   Mit Hilfe dieses Zauber kann der Magier eigene Aura im Verhältnis
 *   2:1 auf einen anderen Magier des gleichen Magiegebietes oder (nur
 *   bei Tybied) im Verhältnis 3:1 auf einen Magier eines anderen
 *   Magiegebietes übertragen.
 *
 * Syntax:
 *  "ZAUBERE <spruchname> <Einheit-Nr> <investierte Aura>"
 *  "ui"
 * Flags:
 *  (UNITSPELL|ONSHIPCAST|ONETARGET)
 * */

static int
sp_transferaura(castorder *co)
{
	unit *u;
	int aura;
	int  cost, gain;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	spellparameter *pa = co->par;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	/* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
	 * abbrechen aber kosten lassen */
	if(pa->param[0]->flag == TARGET_RESISTS) return cast_level;

	/* Zieleinheit ermitteln */
	u = pa->param[0]->data.u;

	/* Wieviel Transferieren? */
	aura = pa->param[1]->data.i;

	if(aura < 2) {
/*		"Auraangabe fehlerhaft." */
		cmistake(mage, strdup(co->order), 208, MSG_MAGIC);
		return 0;
	}

	if(get_mage(mage)->magietyp != M_ASTRAL) {
		if (!get_mage(u) || get_mage(u)->magietyp != get_mage(mage)->magietyp) {
/*			"Zu dieser Einheit kann ich keine Aura übertragen." */
			cmistake(mage, strdup(co->order), 207, MSG_MAGIC);
			return 0;
		}
		gain = min(aura,get_mage(mage)->spellpoints)/2;
		cost = 2 * gain;
	} else {
		if (!get_mage(u)) {
/*			"Zu dieser Einheit kann ich keine Aura übertragen."); */
			cmistake(mage, strdup(co->order), 207, MSG_MAGIC);
			return 0;
		}
		if(get_mage(u)->magietyp == get_mage(mage)->magietyp) {
			gain = min(aura,get_mage(mage)->spellpoints)/2;
			cost = 2 * gain;
		} else {
			gain = min(aura,get_mage(mage)->spellpoints)/3;
			cost = 3 * gain;
		}
	}

	get_mage(u)->spellpoints += gain;
	get_mage(mage)->spellpoints -= cost;

/*	sprintf(buf, "%s transferiert %d Aura auf %s", unitname(mage),
			gain, unitname(u)); */
	add_message(&mage->faction->msgs, new_message(mage->faction,
		"auratransfer_success%u:unit%u:target%i:aura", mage, u, gain));
	return cast_level;
}

/* ------------------------------------------------------------- */
/* DRUIDE */
/* ------------------------------------------------------------- */
/* Name:       Günstige Winde
 * Stufe:      4
 * Gebiet:     Gwyrrd
 * Wirkung:
 * Schiffsbewegung +1, kein Abtreiben.  Hält (Stufe) Runden an.
 * Kombinierbar mit "Sturmwind" (das +1 wird dadurch aber nicht
 * verdoppelt), und "Luftschiff".
 *
 * Flags:
 * (SHIPSPELL|ONSHIPCAST|SPELLLEVEL|ONETARGET|TESTRESISTANCE)
 */

static int
sp_goodwinds(castorder *co)
{
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;
	spellparameter *pa = co->par;
	ship *sh;
	unit *u;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	sh = pa->param[0]->data.sh;

	/* keine Probleme mit C_SHIP_SPEEDUP und C_SHIP_FLYING */
	/* NODRIFT bewirkt auch +1 Geschwindigkeit */
	create_curse(mage, &sh->attribs, C_SHIP_NODRIFT, 0, power, cast_level, 0, 0);

	/* melden, 1x pro Partei */
	freset(mage->faction, FL_DH);
	for(u = r->units; u; u = u->next ) freset(u->faction, FL_DH);
	for(u = r->units; u; u = u->next ) {
		if(u->ship != sh )		/* nur den Schiffsbesatzungen! */
			continue;
		if(!fval(u->faction, FL_DH) ) {
			message * m = msg_message("wind_effect", "mage ship", cansee(u->faction, r, mage, 0) ? mage:NULL, sh);
			r_addmessage(r, u->faction, m);
			msg_release(m);
			fset(u->faction, FL_DH);
		}
	}
	if (!fval(mage->faction, FL_DH)) {
		message * m = msg_message("wind_effect", "mage ship", mage, sh);
		r_addmessage(r, mage->faction, m);
		msg_release(m);
	}

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Magischer Pfad
 * Stufe:      4
 * Gebiet:     Gwyrrd
 * Wirkung:
 * 	für Stufe Runden wird eine (magische) Strasse erzeugt, die wie eine
 * 	normale Strasse wirkt.
 * 	Im Ozean schlägt der Spruch fehl
 *
 * Flags:
 * (FARCASTING|SPELLLEVEL|REGIONSPELL|ONSHIPCAST|TESTRESISTANCE)
 */
static int
sp_magicstreet(castorder *co)
{
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;
	spellparameter *pa = co->par;
	direction_t dir;

	if(!(landregion(rterrain(r)))){
		cmistake(mage, strdup(co->order), 186, MSG_MAGIC);
    return 0;
	}
	dir = finddirection(pa->param[0]->data.s, mage->faction->locale);

	if (rroad(r, dir)){
		cmistake(mage, strdup(co->order), 187, MSG_MAGIC);
    return 0;
	}

	/* wirkt schon in der Zauberrunde! */
	create_curse(mage, &r->attribs, C_MAGICSTREET, 0, power, cast_level, 0, 0);

	/* melden, 1x pro Partei */
	{
		message * seen = msg_message("path_effect", "mage region", mage, r);
		message * unseen = msg_message("path_effect", "mage region", NULL, r);
		report_effect(r, mage, seen, unseen);
	}

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Erwecke Ents
 * Stufe:      10
 * Kategorie:  Beschwörung, positiv
 * Gebiet:     Gwyrrd
 * Wirkung:
 *  Verwandelt (Stufe) Bäume in eine Gruppe von Ents, die sich für Stufe
 *  Runden der Partei des Druiden anschliessen und danach wieder zu
 *  Bäumen werden
 * Patzer:
 *  Monster-Ents entstehen
 *
 * Flags:
 * (SPELLLEVEL)
 */
static int
sp_summonent(castorder *co)
{
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;
	unit *u;
	attrib *a;
	int ents;

#if GROWING_TREES
	if(rtrees(r,2) == 0) {
#else
	if(rtrees(r) == 0) {
#endif
		cmistake(mage, strdup(co->order), 204, MSG_EVENT);
		/* nicht ohne bäume */
    return 0;
	}

#if GROWING_TREES
	ents = min(power*power, rtrees(r,2));
#else
	ents = min(power*power, rtrees(r));
#endif

	u = create_unit(r, mage->faction, ents, new_race[RC_TREEMAN], 0, LOC(mage->faction->locale, rc_name(new_race[RC_TREEMAN], ents!=1)), mage);

	a = a_new(&at_unitdissolve);
	a->data.ca[0] = 2;	/* An r->trees. */
	a->data.ca[1] = 5;	/* 5% */
	a_add(&u->attribs, a);
	fset(u, FL_LOCKED);

#if GROWING_TREES
	rsettrees(r, 2, rtrees(r,2) - ents);
#else
	rsettrees(r, rtrees(r) - ents);
#endif

	/* melden, 1x pro Partei */
	{
		message * seen = msg_message("ent_effect", "mage amount", mage, ents);
		message * unseen = msg_message("ent_effect", "mage amount", NULL, ents);
		report_effect(r, mage, seen, unseen);
	}
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Segne Steinkreis
 * Stufe:      11
 * Kategorie:  Artefakt
 * Gebiet:     Gwyrrd
 * Wirkung:
 *  Es werden zwei neue Gebäude eingeführt: Steinkreis und Steinkreis
 *  (gesegnet). Ersteres kann man bauen, letzteres wird aus einem
 *  fertigen Steinkreis mittels des Zaubers erschaffen.
 *
 * Flags:
 * (BUILDINGSPELL | ONETARGET)
 *
 */
static int
sp_blessstonecircle(castorder *co)
{
	building *b;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	spellparameter *p = co->par;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(p->param[0]->flag == TARGET_NOTFOUND) return 0;

	b = p->param[0]->data.b;

	if(b->type != &bt_stonecircle) {
		sprintf(buf, "%s in %s: 'ZAUBER \"%s\"': %s ist kein Steinkreis.",
			unitname(mage), regionid(mage->region), co->order, buildingname(b));
		addmessage(0, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
		return 0;
	}

	if(b->size < b->type->maxsize) {
		sprintf(buf, "%s in %s: 'ZAUBER \"%s\"': %s muss vor der Weihe "
			"fertiggestellt sein.", unitname(mage), regionid(mage->region),
			co->order, buildingname(b));
		addmessage(0, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
		return 0;
	}

	b->type = &bt_blessedstonecircle;

	sprintf(buf, "%s weiht %s.", unitname(mage), buildingname(b));
	addmessage(r, 0, buf, MSG_MAGIC, ML_INFO);

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Mahlstrom
 * Stufe:      15
 * Kategorie:  Region, negativ
 * Gebiet:     Gwyrrd
 * Wirkung:
 *  Erzeugt auf See einen Mahlstrom für Stufe-Wochen. Jedes Schiff, das
 *  durch den Mahlstrom segelt, nimmt 0-150% Schaden. (D.h.  es hat auch
 *  eine 1/3-Chance, ohne Federlesens zu sinken.  Der Mahlstrom sollte
 *  aus den Nachbarregionen sichtbar sein.
 *
 * Flags:
 * (OCEANCASTABLE | ONSHIPCAST | REGIONSPELL | TESTRESISTANCE)
 */
static int
sp_maelstrom(castorder *co)
{
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;

	if(rterrain(r) != T_OCEAN) {
		cmistake(mage, strdup(co->order), 205, MSG_MAGIC);
		/* nur auf ozean */
		return 0;
	}

	/* Attribut auf Region.
	 * Existiert schon ein curse, so wird dieser verstärkt
	 * (Max(Dauer), Max(Stärke))*/
	create_curse(mage,&mage->attribs,C_MAELSTROM,0,power,power,power,0);
	set_curseflag(r->attribs, C_MAELSTROM, 0, CURSE_ISNEW);

	/* melden, 1x pro Partei */
	{
		message * seen = msg_message("maelstrom_effect", "mage", mage);
		message * unseen = msg_message("maelstrom_effect", "mage", NULL);
		report_effect(r, mage, seen, unseen);
	}

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Wurzeln der Magie
 * Stufe:      16
 * Kategorie:  Region, neutral
 * Gebiet:     Gwyrrd
 * Wirkung:
 * 	Wandelt einen Wald permanent in eine Mallornregion
 *
 * Flags:
 * (FARCASTING | REGIONSPELL | TESTRESISTANCE)
 */
static int
sp_mallorn(castorder *co)
{
	region *r = co->rt;
	int cast_level = co->level;
	unit *mage = (unit *)co->magician;

	if(!landregion(rterrain(r))) {
		cmistake(mage, strdup(co->order), 290, MSG_MAGIC);
		return 0;
	}
	if(fval(r, RF_MALLORN)) {
		cmistake(mage, strdup(co->order), 291, MSG_MAGIC);
		return 0;
	}

	/* half the trees will die */
#if GROWING_TREES
	rsettrees(r, 2, rtrees(r,2)/2);
	rsettrees(r, 1, rtrees(r,1)/2);
	rsettrees(r, 0, rtrees(r,0)/2);
#else
	rsettrees(r, rtrees(r)/2);
#endif
	fset(r, RF_MALLORN);

	/* melden, 1x pro Partei */
	{
		message * seen = msg_message("mallorn_effect", "mage", mage);
		message * unseen = msg_message("mallorn_effect", "mage", NULL);
		report_effect(r, mage, seen, unseen);
	}

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Segen der Erde
 * Stufe:      1
 * Kategorie:  Region, positiv
 * Gebiet:     Gwyrrd
 *
 * Wirkung:
 *  Alle Bauern verdienen Stufe-Wochen 1 Silber mehr.
 *
 * Flags:
 * (FARCASTING | SPELLLEVEL | ONSHIPCAST | REGIONSPELL)
 */
static int
sp_blessedharvest(castorder *co)
{
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;

	/* Attribut auf Region.
	 * Existiert schon ein curse, so wird dieser verstärkt
	 * (Max(Dauer), Max(Stärke))*/
	create_curse(mage,&r->attribs,C_BLESSEDHARVEST,0,power,power,1,0);

	{
		message * seen = msg_message("harvest_effect", "mage", mage);
		message * unseen = msg_message("harvest_effect", "mage", NULL);
		report_effect(r, mage, seen, unseen);
	}

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Hainzauber
 * Stufe:      2
 * Kategorie:  Region, positiv
 * Gebiet:     Gwyrrd
 * Syntax:     ZAUBER [REGION x y] [STUFE 2] "Hain"
 * Wirkung:
 *     Erschafft Stufe-10*Stufe Jungbäume
 *
 * Flag:
 * (FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE)
 */

static int
sp_hain(castorder *co)
{
	int trees;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;

	if(!r->land) {
		cmistake(mage, strdup(co->order), 296, MSG_MAGIC);
		return 0;
	}
	if (fval(r, RF_MALLORN)) {
		cmistake(mage, strdup(co->order), 92, MSG_MAGIC);
		return 0;
	}

	trees = lovar(force * 10) + force;
#if GROWING_TREES
	rsettrees(r, 1, rtrees(r,1) + trees);
#else
	rsettrees(r, rtrees(r) + trees);
#endif

	/* melden, 1x pro Partei */
	{
		message * seen = msg_message("growtree_effect", "mage amount", mage, trees);
		message * unseen = msg_message("growtree_effect", "mage amount", NULL, trees);
		report_effect(r, mage, seen, unseen);
	}

	return cast_level;
}
/* ------------------------------------------------------------- */
/* Name:       Segne Mallornstecken - Mallorn Hainzauber 
 * Stufe:      4
 * Kategorie:  Region, positiv
 * Gebiet:     Gwyrrd
 * Syntax:     ZAUBER [REGION x y] [STUFE 4] "Segne Mallornstecken"
 * Wirkung:
 *     Erschafft Stufe-10*Stufe Jungbäume
 *
 * Flag:
 * (FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE)
 */

static int
sp_mallornhain(castorder *co)
{
	int trees;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;

	if(!r->land) {
		cmistake(mage, strdup(co->order), 296, MSG_MAGIC);
		return 0;
	}
	if (!fval(r, RF_MALLORN)) {
		cmistake(mage, strdup(co->order), 91, MSG_MAGIC);
		return 0;
	}

	trees = lovar(force * 10) + force;
#if GROWING_TREES
	rsettrees(r, 1, rtrees(r,1) + trees);
#else
	rsettrees(r, rtrees(r) + trees);
#endif

	/* melden, 1x pro Partei */
	{
		message * seen = msg_message("growtree_effect", "mage amount", mage, trees);
		message * unseen = msg_message("growtree_effect", "mage amount", NULL, trees);
		report_effect(r, mage, seen, unseen);
	}

	return cast_level;
}

void
patzer_ents(castorder *co)
{
	int ents;
	unit *u;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	/* int cast_level = co->level; */
	int force = co->force;

	if(!r->land) {
		cmistake(mage, strdup(co->order), 296, MSG_MAGIC);
		return;
	}

	ents = force*10;
	u = create_unit(r, findfaction(MONSTER_FACTION), ents, new_race[RC_TREEMAN], 0, 
		LOC(default_locale, rc_name(new_race[RC_TREEMAN], ents!=1)), NULL);

	/* 'Erfolg' melden */
	add_message(&mage->faction->msgs, new_message(mage->faction,
				"regionmagic_patzer%u:unit%r:region%s:command%u:unit", mage,
				mage->region, strdup(co->order), mage));

	/* melden, 1x pro Partei */
	{
		message * unseen = msg_message("entrise", "region amount", r, ents);
		report_effect(r, mage, unseen, unseen);
	}
}

/* ------------------------------------------------------------- */
/* Name:       Rosthauch
 * Stufe:      3
 * Kategorie:  Einheit, negativ
 * Gebiet:     Gwyrrd
 * Wirkung:
 *  Zerstört zwischen Stufe und Stufe*10 Eisenwaffen
 *  Eisenwaffen: I_SWORD, I_GREATSWORD, I_AXE, I_HALBERD
 *
 * Flag:
 * (FARCASTING | SPELLLEVEL | UNITSPELL | TESTCANSEE | TESTRESISTANCE)
 */
/* Syntax: ZAUBER [REGION x y] [STUFE 2] "Rosthauch" 1111 2222 3333 */

static int
sp_rosthauch(castorder *co)
{
	unit *u;
	int ironweapon;
	int i, n;
	int success = 0;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;
	spellparameter *pa = co->par;

	force = rand()%(force * 10) + force;

	/* fuer jede Einheit */
	for (n = 0; n < pa->length; n++) {
		if (!force)
			break;

		if(pa->param[n]->flag == TARGET_RESISTS
				|| pa->param[n]->flag == TARGET_NOTFOUND)
			continue;

		u = pa->param[n]->data.u;

		/* Eisenwaffen: I_SWORD, I_GREATSWORD, I_AXE, I_HALBERD (50% Chance)*/
		ironweapon = 0;

		i = min(get_item(u, I_SWORD), force);
		if (i > 0){
			change_item(u, I_SWORD, -i);
			force -= i;
			ironweapon += i;
		}
		i = min(get_item(u, I_GREATSWORD), force);
		if (i > 0){
			change_item(u, I_GREATSWORD, -i);
			force -= i;
			ironweapon += i;
		}
		i = min(get_item(u, I_AXE), force);
		if (i > 0){
			change_item(u, I_AXE, -i);
			force -= i;
			ironweapon += i;
		}
		i = min(get_item(u, I_HALBERD), force);
		if (i > 0){
			if(rand()%100 < 50){
				change_item(u, I_HALBERD, -i);
				force -= i;
				ironweapon += i;
			}
		}

		if (ironweapon) {
			/* {$mage mage} legt einen Rosthauch auf {target}. {amount} Waffen
			 * wurden vom Rost zerfressen */
			add_message(&mage->faction->msgs, new_message(mage->faction,
				"rust_effect%u:mage%u:target%i:amount", mage, u, ironweapon));
			add_message(&u->faction->msgs, new_message(u->faction,
				"rust_effect%u:mage%u:target%i:amount",
				cansee(u->faction, r, mage, 0) ? mage:NULL, u, ironweapon));
			success += ironweapon;
		} else {
			/* {$mage mage} legt einen Rosthauch auf {target}, doch der
			 * Rosthauch fand keine Nahrung */
			add_message(&mage->faction->msgs, new_message(mage->faction,
				"rust_fail%u:mage%u:target", mage, u));
		}
	}
	/* in success stehen nun die insgesamt zerstörten Waffen. Im
	 * ungünstigsten Fall kann pro Stufe nur eine Waffe verzaubert werden,
	 * darum wird hier nur für alle Fälle in denen noch weniger Waffen
	 * betroffen wurden ein Kostennachlass gegeben */
	return min(success, cast_level);
}


/* ------------------------------------------------------------- */
/* Name:       Kälteschutz
 * Stufe:      3
 * Kategorie:  Einheit, positiv
 * Gebiet:     Gwyrrd
 *
 * Wirkung:
 *  schützt ein bis mehrere Einheiten mit bis zu Stufe*10 Insekten vor
 *  den Auswirkungen der Kälte. Sie können Gletscher betreten und dort
 *  ganz normal ohne jeden Malus alles machen Die Wirkung hält Stufe
 *  Wochen an
 *
 * Flag:
 * (UNITSPELL | SPELLLEVEL | ONSHIPCAST | TESTCANSEE)
 */
/* Syntax: ZAUBER [STUFE n] "Kälteschutz" eh1 [eh2 [eh3 [...]]] */

static int
sp_kaelteschutz(castorder *co)
{
	unit *u;
	int n, i = 0;
	int men;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;
	int duration = max(cast_level, force);
	spellparameter *pa = co->par;


	force*=10;	/* 10 Personen pro Force-Punkt */

	/* für jede Einheit in der Kommandozeile */
	for (n = 0; n < pa->length; n++) {
		if (force < 1)
			break;

		if(pa->param[n]->flag == TARGET_RESISTS
				|| pa->param[n]->flag == TARGET_NOTFOUND)
			continue;

		u = pa->param[n]->data.u;

		if (force < u->number){
			men = force;
		} else {
			men = u->number;
		}

		create_curse(mage, &u->attribs, C_KAELTESCHUTZ, 0, cast_level,
				duration, 1, men);

		force -= u->number;
		add_message(&mage->faction->msgs, new_message(mage->faction,
			"heat_effect%u:mage%u:target", mage, u));
		if (u->faction!=mage->faction) add_message(&u->faction->msgs, new_message(u->faction,
			"heat_effect%u:mage%u:target",
			cansee(u->faction, r, mage, 0) ? mage:NULL, u));
		i = cast_level;
	}
	/* Erstattung? */
	return i;
}

/* ------------------------------------------------------------- */
/* Name:       Verwünschung, Funkenregen, Naturfreund, ...
 * Stufe:      1
 * Kategorie:  Einheit, rein visuell
 * Gebiet:     Alle
 *
 * Wirkung:
 * Die Einheit wird von einem magischen Effekt heimgesucht, der in ihrer
 * Beschreibung auftaucht, aber nur visuellen Effekt hat.
 *
 * Flag:
 * (UNITSPELL | TESTCANSEE | SPELLLEVEL | ONETARGET)
 */
/* Syntax: ZAUBER "Funkenregen" eh1 */

static int
sp_sparkle(castorder *co)
{
	unit *u;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	spellparameter *pa = co->par;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	/* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
	 * abbrechen aber kosten lassen */
	if(pa->param[0]->flag == TARGET_RESISTS) return cast_level;

	u = pa->param[0]->data.u;
	create_curse(mage, &u->attribs, C_SPARKLE, 0, cast_level,
			cast_level, rand(), u->number);

	add_message(&mage->faction->msgs, new_message(mage->faction,
		"sparkle_effect%u:mage%u:target", mage, u));
	if (u->faction!=mage->faction)
		add_message(&u->faction->msgs, new_message(u->faction,
		"sparkle_effect%u:mage%u:target", mage, u));

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Eisengolem
 * Stufe:      2
 * Kategorie:  Beschwörung, positiv
 * Gebiet:     Gwyrrd
 * Wirkung:
 *   Erschafft eine Einheit Eisengolems mit Stufe*8 Golems.  Jeder Golem
 *   hat jede Runde eine Chance von 15% zu Staub zu zerfallen.  Gibt man
 *   den Golems den Befehl 'mache Schwert/Bihänder' oder 'mache
 *   Schild/Kettenhemd/Plattenpanzer', so werden pro Golem 5 Eisenbarren
 *   verbaut und der Golem löst sich auf.
 *
 *   Golems sind zu langsam um wirklich im Kampf von Nutzen zu sein.
 *   Jedoch fangen sie eine Menge Schaden auf und sollten sie zufällig
 *   treffen, so ist der Schaden fast immer tödlich.  (Eisengolem: HP
 *   50, AT 4, PA 2, Rüstung 2(KH), 2d10+4 TP, Magieresistenz 0.25)
 *
 *   Golems nehmen nix an und geben nix.  Sie bewegen sich immer nur 1
 *   Region weit und ziehen aus Strassen keinen Nutzen.  Ein Golem wiegt
 *   soviel wie ein Stein. Kann nicht im Sumpf gezaubert werden
 *
 * Flag:
 *  (SPELLLEVEL)
 *
 * #define GOLEM_IRON   4
 */

static int
sp_create_irongolem(castorder *co)
{
	unit *u2;
	attrib *a;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;

	if (rterrain(r) == T_SWAMP){
		cmistake(mage, strdup(co->order), 188, MSG_MAGIC);
		return 0;
	}

	u2 = create_unit(r, mage->faction, force*8, new_race[RC_IRONGOLEM], 0, 
		LOC(mage->faction->locale, rc_name(new_race[RC_IRONGOLEM], 1)), mage);

	set_level(u2, SK_ARMORER, 1);
	set_level(u2, SK_WEAPONSMITH, 1);

	a = a_new(&at_unitdissolve);
	a->data.ca[0] = 0;
	a->data.ca[1] = IRONGOLEM_CRUMBLE;
	a_add(&u2->attribs, a);

	add_message(&mage->faction->msgs, 
		msg_message("magiccreate_effect", "region command unit amount object",
		mage->region, strdup(co->order), mage, force*8,
		LOC(mage->faction->locale, rc_name(new_race[RC_IRONGOLEM], 1))));

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Steingolem
 * Stufe:      1
 * Kategorie:  Beschwörung, positiv
 * Gebiet:     Gwyrrd
 * Wirkung:
 *  Erschafft eine Einheit Steingolems mit Stufe*5 Golems. Jeder Golem
 *  hat jede Runde eine Chance von 10% zu Staub zu zerfallen.  Gibt man
 *  den Golems den Befehl 'mache Burg' oder 'mache Strasse', so werden
 *  pro Golem 10 Steine verbaut und der Golem löst sich auf.
 *
 *  Golems sind zu langsam um wirklich im Kampf von Nutzen zu sein.
 *  Jedoch fangen sie eine Menge Schaden auf und sollten sie zufällig
 *  treffen, so ist der Schaden fast immer tödlich.  (Steingolem: HP 60,
 *  AT 4, PA 2, Rüstung 4(PP), 2d12+6 TP)
 *
 *  Golems nehmen nix an und geben nix. Sie bewegen sich immer nur 1
 *  Region weit und ziehen aus Strassen keinen Nutzen. Ein Golem wiegt
 *  soviel wie ein Stein.
 *
 *  Kann nicht im Sumpf gezaubert werden
 *
 * Flag:
 *  (SPELLLEVEL)
 *
 * #define GOLEM_STONE 4
 */
static int
sp_create_stonegolem(castorder *co)
{
	unit *u2;
	attrib *a;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;

	if (rterrain(r) == T_SWAMP){
		cmistake(mage, strdup(co->order), 188, MSG_MAGIC);
		return 0;
	}

	u2 = create_unit(r, mage->faction, force*5, new_race[RC_STONEGOLEM], 0, 
		LOC(mage->faction->locale, rc_name(new_race[RC_STONEGOLEM], 1)), mage);
	set_level(u2, SK_ROAD_BUILDING, 1);
	set_level(u2, SK_BUILDING, 1);

	a = a_new(&at_unitdissolve);
	a->data.ca[0] = 0;
	a->data.ca[1] = STONEGOLEM_CRUMBLE;
	a_add(&u2->attribs, a);

	add_message(&mage->faction->msgs, 
		msg_message("magiccreate_effect", "region command unit amount object",
		mage->region, strdup(co->order), mage, force*5,
		LOC(mage->faction->locale, rc_name(new_race[RC_STONEGOLEM], 1))));

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Große Dürre
 * Stufe:      17
 * Kategorie:  Region, negativ
 * Gebiet:     Gwyrrd
 *
 * Wirkung:
 *   50% alle Bauern, Pferde, Bäume sterben.
 *   Zu 25% terraform: Gletscher wird mit 50% zu Sumpf, sonst Ozean,
 *   Sumpf wird zu Steppe, Ebene zur Steppe, Steppe zur Wüste.
 * Besonderheiten:
 *  neuer Terraintyp Steppe:
 *  5000 Felder, 500 Bäume, Strasse: 250 Steine. Anlegen wie in Ebene
 *  möglich
 *
 * Flags:
 * (FARCASTING | REGIONSPELL | TESTRESISTANCE)
 */

void
destroy_all_roads(region *r, int val)
{
	int i;

	for(i = 0; i < MAXDIRECTIONS; i++){
		rsetroad(r,(direction_t)i, val);
	}
}

static int
sp_great_drought(castorder *co)
{
	building *b, *b2;
	unit *u;
	boolean terraform = false;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;

	if(rterrain(r) == T_OCEAN ) {
		cmistake(mage, strdup(co->order), 189, MSG_MAGIC);
		/* TODO: vielleicht einen netten Patzer hier? */
		return 0;
	}

  /* sterben */
	rsetpeasants(r, rpeasants(r)/2); /* evtl wuerfeln */
#if GROWING_TREES
	rsettrees(r, 2, rtrees(r,2)/2);
	rsettrees(r, 1, rtrees(r,1)/2);
	rsettrees(r, 0, rtrees(r,0)/2);
#else
	rsettrees(r, rtrees(r)/2);
#endif
	rsethorses(r, rhorses(r)/2);

	/* Arbeitslohn = 1/4 */
	create_curse(mage, &r->attribs, C_DROUGHT, 0, force, 1, 4, 0);

  /* terraforming */
	if (rand() % 100 < 25){
		terraform = true;

		switch(rterrain(r)){
			case T_PLAIN:
	   		rsetterrain(r, T_GRASSLAND);
				destroy_all_roads(r, 0);
				break;

			case T_SWAMP:
	   		rsetterrain(r, T_GRASSLAND);
				destroy_all_roads(r, 0);
				break;

			case T_GRASSLAND:
				rsetterrain(r, T_DESERT);
				destroy_all_roads(r, 0);
				break;

			case T_GLACIER:
#if NEW_RESOURCEGROWTH == 0
				rsetiron(r, 0);
				rsetlaen(r, -1);
#endif
				if (rand() % 100 < 50){
					rsetterrain(r, T_SWAMP);
					destroy_all_roads(r, 0);
				} else {   /* Ozean */
					destroy_all_roads(r, 0);
					rsetterrain(r, T_OCEAN);
					/* Einheiten dürfen hier auf keinen Fall gelöscht werden! */
					for (u = r->units; u; u = u->next) {
						if (old_race(u->race) != RC_SPELL && u->ship == 0) {
							set_number(u, 0);
						}
					}
					for (b = r->buildings; b;){
						b2 = b->next;
						destroy_building(b);
						b = b2;
					}
				}
				break;

			default:
				terraform = false;
				break;
		}
	}

	/* melden, 1x pro partei */
	for (u = r->units; u; u = u->next) freset(u->faction, FL_DH);

	for (u = r->units; u; u = u->next) {
		if (!fval(u->faction, FL_DH)) {
			fset(u->faction, FL_DH);
			sprintf(buf, "%s ruft das Feuer der Sonne auf %s hinab.",
					cansee(u->faction, r, mage, 0)? unitname(mage) : "Jemand",
					regionid(r));
			if(rterrain(r) != T_OCEAN){
				if(rterrain(r) == T_SWAMP && terraform){
						scat(" Eis schmilzt und verwandelt sich in Morast. Reißende "
								"Ströme spülen die mageren Felder weg und ersäufen "
								"Mensch und Tier. Was an Bauten nicht den Fluten zum Opfer "
								"fiel, verschlingt der Morast. Die sengende Hitze verändert "
								"die Region für immer.");
				} else {
					scat(" Die Felder verdorren und Pferde verdursten. Die Hungersnot "
							"kostet vielen Bauern das Leben. Vertrocknete Bäume recken "
							"ihre kahlen Zweige in den blauen Himmel, von dem "
							"erbarmungslos die sengende Sonne brennt.");
					if(terraform){
						scat(" Die Dürre veränderte die Region für immer.");
					}
				}
				addmessage(r, u->faction, buf, MSG_EVENT, ML_WARN);
			} else { /* ist Ozean */
				scat(" Das Eis zerbricht und eine gewaltige Flutwelle verschlingt"
						"die Region.");
				/* es kann gut sein, das in der Region niemand überlebt, also
				 * besser eine Globalmeldung */
				addmessage(0, u->faction, buf, MSG_EVENT, ML_IMPORTANT);
			}
		}
	}
	if(!fval(mage->faction, FL_DH)){
		add_message(&mage->faction->msgs, new_message(mage->faction,
			"drought_effect%u:mage%r:region", mage, r));
	}
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       'Weg der Bäume'
 * Stufe:      9
 * Kategorie:  Teleport
 * Gebiet:     Gwyrrd
 * Wirkung:
 *  Der Druide kann 5*Stufe GE in die astrale Ebene schicken.
 *  Der Druide wird nicht mitteleportiert, es sei denn, er gibt sich
 *  selbst mit an.
 *  Der Zauber funktioniert nur in Wäldern.
 *
 * Syntax: Zauber "Weg der Bäume" <Einheit> ...
 *
 * Flags:
 * (UNITSPELL | SPELLLEVEL | TESTCANSEE)
 */
static int
sp_treewalkenter(castorder *co)
{
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	spellparameter *pa = co->par;
	int power = co->force;
	int cast_level = co->level;
	region *rt;
	unit *u, *u2;
	int remaining_cap;
	int n;
	int erfolg = 0 ;

	if (getplane(r) != 0) {
		cmistake(mage, strdup(co->order), 190, MSG_MAGIC);
		return 0;
	}

	if (!r_isforest(r)) {
		cmistake(mage, strdup(co->order), 191, MSG_MAGIC);
		return 0;
	}

	rt = r_standard_to_astral(r);
	if(!rt || is_cursed(rt->attribs, C_ASTRALBLOCK, 0)) {
		cmistake(mage, strdup(co->order), 192, MSG_MAGIC);
		return 0;
	}

	assert(rt != NULL);

	remaining_cap = power * 500;

	/* fuer jede Einheit */
	for (n = 0; n < pa->length; n++) {
		if(pa->param[n]->flag == TARGET_RESISTS
				|| pa->param[n]->flag == TARGET_NOTFOUND)
			continue;

		u = pa->param[n]->data.u;

		if(!ucontact(u, mage)) {
			cmistake(mage, strdup(co->order), 73, MSG_MAGIC);
		} else {
			int w = weight(u);
			if(!can_survive(u, rt)) {
				cmistake(mage, strdup(co->order), 231, MSG_MAGIC);
			} else if(remaining_cap - w < 0) {
				sprintf(buf, "%s ist zu schwer.", unitname(u));
  			addmessage(r, mage->faction, buf,  MSG_MAGIC, ML_WARN);
			} else {
				remaining_cap = remaining_cap - w;
				move_unit(u, rt, NULL);
				erfolg = cast_level;

				/* Meldungen in der Ausgangsregion */

				for (u2 = r->units; u2; u2 = u2->next) freset(u2->faction, FL_DH);

				for(u2 = r->units; u2; u2 = u2->next ) {
					if(!fval(u2->faction, FL_DH)) {
						fset(u2->faction, FL_DH);
						if(cansee(u2->faction, r, u, 0)) {
							sprintf(buf, "%s wird durchscheinend und verschwindet.",
								unitname(u));
							addmessage(r, u2->faction, buf, MSG_EVENT, ML_INFO);
						}
					}
				}

				/* Meldungen in der Zielregion */

				for (u2 = r->units; u2; u2 = u2->next) freset(u2->faction, FL_DH);
				for(u2 = rt->units; u2; u2 = u2->next ) {
					if(!fval(u2->faction, FL_DH)) {
						fset(u2->faction, FL_DH);
						if(cansee(u2->faction, rt, u, 0)) {
							sprintf(buf, "%s erscheint plötzlich.", unitname(u));
							addmessage(rt, u2->faction, buf, MSG_EVENT, ML_INFO);
						}
					}
				}
			}
		}
	}
	return erfolg;
}

/* ------------------------------------------------------------- */
/* Name:       'Sog des Lebens'
 * Stufe:      9
 * Kategorie:  Teleport
 * Gebiet:     Gwyrrd
 * Wirkung:
 *  Der Druide kann 5*Stufe GE aus die astrale Ebene schicken.  Der
 *  Druide wird nicht mitteleportiert, es sei denn, er gibt sich selbst
 *  mit an.
 *  Der Zauber funktioniert nur, wenn die Zielregion ein Wald ist.
 *
 * Syntax: Zauber "Sog des Lebens" <Ziel-X> <Ziel-Y> <Einheit> ...
 *
 * Flags:
 * (UNITSPELL|SPELLLEVEL)
 */
static int
sp_treewalkexit(castorder *co)
{
	region *rt;
	regionlist *rl, *rl2;
	int tax, tay;
	unit *u, *u2;
	int remaining_cap;
	int n;
	int erfolg = 0;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int power = co->force;
	spellparameter *pa = co->par;
	int cast_level = co->level;

	if(getplane(r) != astral_plane) {
		cmistake(mage, strdup(co->order), 193, MSG_MAGIC);
		return 0;
	}
	if(is_cursed(r->attribs, C_ASTRALBLOCK, 0)) {
		cmistake(mage, strdup(co->order), 192, MSG_MAGIC);
		return 0;
	}

	remaining_cap = power * 500;

	if(pa->param[0]->typ != SPP_REGION){
		report_failure(mage, co->order);
		return 0;
	}

	/* Koordinaten setzen und Region löschen für Überprüfung auf
	 * Gültigkeit */
	rt  = pa->param[0]->data.r;
	tax = rt->x;
	tay = rt->y;
	rt  = NULL;

	rl  = allinhab_in_range(r_astral_to_standard(r),TP_RADIUS);
	rt  = 0;

	rl2 = rl;
	while(rl2) {
		if(rl2->region->x == tax && rl2->region->y == tay) {
			rt = rl2->region;
			break;
		}
		rl2 = rl2->next;
  }
  free_regionlist(rl);

	if(!rt) {
		cmistake(mage, strdup(co->order), 195, MSG_MAGIC);
    return 0;
  }

	if (!r_isforest(rt)) {
		cmistake(mage, strdup(co->order), 196, MSG_MAGIC);
    return 0;
  }

	/* für jede Einheit in der Kommandozeile */
	for (n = 1; n < pa->length; n++) {
		if(pa->param[n]->flag == TARGET_RESISTS
				|| pa->param[n]->flag == TARGET_NOTFOUND)
			continue;

		u = pa->param[n]->data.u;

		if(!ucontact(u, mage)) {
			sprintf(buf, "%s hat uns nicht kontaktiert.", unitname(u));
  		addmessage(r, mage->faction, buf,  MSG_MAGIC, ML_MISTAKE);
		} else {
			int w = weight(u);
			if(!can_survive(u, rt)) {
				cmistake(mage, strdup(co->order), 231, MSG_MAGIC);
			} else if(remaining_cap - w < 0) {
				sprintf(buf, "%s ist zu schwer.", unitname(u));
  			addmessage(r, mage->faction, buf,  MSG_MAGIC, ML_MISTAKE);
			} else {
				remaining_cap = remaining_cap - w;
				move_unit(u, rt, NULL);
				erfolg = cast_level;

				/* Meldungen in der Ausgangsregion */

				for (u2 = r->units; u2; u2 = u2->next) freset(u2->faction, FL_DH);
				for(u2 = r->units; u2; u2 = u2->next ) {
					if(!fval(u2->faction, FL_DH)) {
						fset(u2->faction, FL_DH);
						if(cansee(u2->faction, r, u, 0)) {
							sprintf(buf, "%s wird durchscheinend und verschwindet.",
								unitname(u));
							addmessage(r, u2->faction, buf, MSG_EVENT, ML_INFO);
						}
					}
				}

				/* Meldungen in der Zielregion */

				for (u2 = r->units; u2; u2 = u2->next) freset(u2->faction, FL_DH);
				for(u2 = rt->units; u2; u2 = u2->next ) {
					if(!fval(u2->faction, FL_DH)) {
						fset(u2->faction, FL_DH);
						if(cansee(u2->faction, rt, u, 0)) {
							sprintf(buf, "%s erscheint plötzlich.", unitname(u));
							addmessage(rt, u2->faction, buf, MSG_EVENT, ML_INFO);
						}
					}
				}
			}
		}
	}
	return erfolg;
}

void
creation_message(unit * mage, item_t i)
{
	region * r = mage->region;
	sprintf(buf, "%s erschafft ein %s.", unitname(mage),
			locale_string(mage->faction->locale, resourcename(olditemtype[i]->rtype, 0)));
	addmessage(r, mage->faction, buf, MSG_MAGIC, ML_INFO);
}

static int
sp_create_sack_of_conservation(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	change_item(mage,I_SACK_OF_CONSERVATION,1);

	creation_message(mage, I_SACK_OF_CONSERVATION);

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:		   Heiliger Boden
 * Stufe:		   9
 * Kategorie:  perm. Regionszauber
 * Gebiet:     Gwyrrd
 * Wirkung:
 *   Es entstehen keine Untoten mehr, Untote betreten die Region
 *   nicht mehr.
 *
 * ZAUBER "Heiliger Boden"
 * Flags: (0)
 */
static int
sp_holyground(castorder *co)
{
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;
	message * msg = r_addmessage(r, mage->faction, msg_message("holyground", "mage", mage));
	msg_release(msg);

	create_curse(mage, &r->attribs, C_HOLYGROUND, 0,
			power*power, 1, 0, 0);

	set_curseflag(mage->region->attribs, C_HOLYGROUND, 0, CURSE_NOAGE);

	a_removeall(&r->attribs, &at_deathcount);

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:		   Heimstein
 * Stufe:		   7
 * Kategorie:  Artefakt
 * Gebiet:     Gwyrrd
 * Wirkung:
 * Die Burg kann nicht mehr durch Donnerbeben oder andere
 * Gebäudezerstörenden Sprüche kaputt gemacht werden. Auch
 * schützt der Zauber vor Belagerungskatapulten.
 *
 * ZAUBER Heimstein
 * Flags: (0)
 */
static int
sp_homestone(castorder *co)
{
	unit *u;
	curse * success;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;

	if(!mage->building || mage->building->type != &bt_castle){
		cmistake(mage, strdup(co->order), 197, MSG_MAGIC);
		return 0;
	}

	success = create_curse(mage, &mage->building->attribs, C_MAGICSTONE, 0,
			force*force, 1, 0, 0);

	if(!success) {
		cmistake(mage, strdup(co->order), 206, MSG_MAGIC);
		return 0;
	}

	set_curseflag(mage->building->attribs, C_MAGICSTONE, 0, CURSE_NOAGE);

	/* Nur ein Heimstein pro Burg möglich */
	set_curseflag(mage->building->attribs, C_MAGICSTONE, 0, CURSE_ONLYONE);

	/* Magieresistenz der Burg erhöht sich um 50% */
	create_curse(mage,  &mage->building->attribs, C_RESIST_MAGIC, 0,
		force*force, 1, 50, 0);
	set_curseflag(mage->building->attribs, C_RESIST_MAGIC, 0, CURSE_NOAGE);

	/* melden, 1x pro Partei in der Burg */
	for (u = r->units; u; u = u->next) freset(u->faction, FL_DH);
	for (u = r->units; u; u = u->next) {
		if (!fval(u->faction, FL_DH)) {
			fset(u->faction, FL_DH);
			if (u->building ==  mage->building) {
				sprintf(buf, "Mit einem Ritual bindet %s die magischen Kräfte "
						"der Erde in die Mauern von %s", unitname(mage),
						buildingname(mage->building));
				addmessage(r, u->faction, buf, MSG_EVENT, ML_INFO);
			}
		}
	}
	return cast_level;
}



/* ------------------------------------------------------------- */
/* Name:       Dürre
 * Stufe:      13
 * Kategorie:  Region, negativ
 * Gebiet:     Gwyrrd
 * Wirkung:
 *  temporär verändert sich das Baummaximum und die maximalen Felder in
 *  einer Region auf die Hälfte des normalen.
 *  Die Hälfte der Bäume verdorren und Pferde verdursten.
 *  Arbeiten bringt nur noch 1/4 des normalen Verdienstes
 *
 * Flags:
 * (FARCASTING|REGIONSPELL|TESTRESISTANCE),
 */
static int
sp_drought(castorder *co)
{
	curse *c;
	unit *u;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;

	if(rterrain(r) == T_OCEAN ) {
		cmistake(mage, strdup(co->order), 189, MSG_MAGIC);
		/* TODO: vielleicht einen netten Patzer hier? */
		return 0;
	}

	/* melden, 1x pro Partei */
	for (u = r->units; u; u = u->next) freset(u->faction, FL_DH);
	for(u = r->units; u; u = u->next ) {
		if(!fval(u->faction, FL_DH) ) {
			fset(u->faction, FL_DH);
			sprintf(buf, "%s verflucht das Land, und eine Dürreperiode beginnt.",
					cansee(u->faction, r, mage, 0) ? unitname(mage) : "Jemand");
			addmessage(r, u->faction, buf, MSG_EVENT, ML_INFO);
		}
	}
	if(!fval(mage->faction, FL_DH)){
		sprintf(buf, "%s verflucht das Land, und eine Dürreperiode beginnt.",
				unitname(mage));
  	addmessage(0, mage->faction, buf, MSG_MAGIC, ML_INFO);
	}

	/* Wenn schon Duerre herrscht, dann setzen wir nur den Power-Level
	 * hoch (evtl dauert dann die Duerre laenger).  Ansonsten volle
	 * Auswirkungen.
	 */
	c = get_curse(r->attribs, C_DROUGHT, 0);
	if(c) {
		c->vigour = max(c->vigour, power);
		c->duration = max(c->duration, power);
	} else {
		/* Baeume und Pferde sterben */
#if GROWING_TREES
		rsettrees(r, 2, rtrees(r,2)/2);
		rsettrees(r, 1, rtrees(r,1)/2);
		rsettrees(r, 0, rtrees(r,0)/2);
#else
		rsettrees(r, rtrees(r)/2);
#endif
		rsethorses(r, rhorses(r)/2);

		create_curse(mage, &r->attribs, C_DROUGHT, 0, power, power, 4, 0);
	}
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Nebel der Verwirrung
 * Stufe:      14
 * Kategorie:  Region, negativ
 * Gebiet:     Gwyrrd
 * Wirkung:
 *  Alle Regionen innerhalb eines Radius von ((Stufe-15)/2 aufgerundet)
 *  werden von einem verwirrenden Nebel bedeckt.  Innerhalb des Nebels
 *  können keine Himmelsrichtungen mehr erkannt werden, alle Bewegungen
 *  erfolgen in eine zufällige Richtung.
 *  Die Gwyrrd-Variante wirkt nur auf Wälder und Ozeanregionen
 * Flags:
 *  (FARCASTING | SPELLLEVEL)
 * */
static int
sp_fog_of_confusion(castorder *co)
{
	unit *u;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;
	int range;
	int duration;
	regionlist *rl,*rl2;

	range = (power-11)/3-1;
	duration = ((power-11)/3)*2;

	rl = all_in_range(r, range);

	for(rl2 = rl; rl2; rl2 = rl2->next) {
		if(rterrain(rl2->region) != T_OCEAN
				&& !r_isforest(rl2->region)) continue;

		/* Magieresistenz jeder Region prüfen */
		if (target_resists_magic(mage, r, TYP_REGION, 0)){
			report_failure(mage, co->order);
			continue;
		}

		create_curse(mage, &rl2->region->attribs, C_REGCONF,0,
				power, duration, cast_level*5, 0);
		/* Soll der schon in der Zauberrunde wirken? */
		set_curseflag(rl2->region->attribs, C_REGCONF, 0, CURSE_ISNEW);

		for (u = rl2->region->units; u; u = u->next) freset(u->faction, FL_DH);
		for(u = rl2->region->units; u; u = u->next ) {
			if(!fval(u->faction, FL_DH) ) {
				fset(u->faction, FL_DH);
				sprintf(buf, "%s beschwört einen Schleier der Verwirrung.",
						cansee(u->faction, r, mage, 0) ? unitname(mage) : "Jemand");
				addmessage(rl2->region, u->faction, buf, MSG_EVENT, ML_INFO);
			}
		}
		if(!fval(mage->faction, FL_DH)){
			sprintf(buf, "%s beschwört einen Schleier der Verwirrung.",
					unitname(mage));
  		addmessage(0, mage->faction, buf, MSG_MAGIC, ML_INFO);
		}
	}
	free_regionlist(rl);
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Bergwächter
 * Stufe:      9
 * Gebiet:     Gwyrrd
 * Kategorie:  Beschwörung, negativ
 *
 * Wirkung:
 * Erschafft in Bergen oder Gletschern einen Wächter, der durch bewachen
 * den Eisen/Laen-Abbau für nicht-Allierte verhindert.  Bergwächter
 * verhindern auch Abbau durch getarnte/unsichtbare Einheiten und lassen
 * sich auch durch Belagerungen nicht aufhalten.
 *
 * (Ansonsten in economic.c:manufacture() entsprechend anpassen).
 *
 * Fähigkeiten (factypes.c): 50% Magieresistenz, 25 HP, 4d4 Schaden,
 * 	4 Rüstung (=PP)
 * Flags:
 * (FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE)
 */
static int
sp_ironkeeper(castorder *co)
{
	unit *keeper;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	if(rterrain(r) != T_MOUNTAIN  &&  rterrain(r) != T_GLACIER ) {
		sprintf(buf, "%s in %s: Wächtergeister können nur in Bergen "
			"und Gletschern beschworen werden.", unitname(mage),
			regionid(mage->region));
		addmessage(r, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
		return 0;
	}
	keeper = create_unit(r, mage->faction, 1, new_race[RC_IRONKEEPER], 0, "Bergwächter", mage);

	/*keeper->age = cast_level + 2;*/
	guard(keeper, GUARD_MINING);
	fset(keeper, FL_ISNEW);
	keeper->status = ST_AVOID;	/* kaempft nicht */
	/* Parteitarnen, damit man nicht sofort weiß, wer dahinter steckt */
	fset(keeper, FL_PARTEITARNUNG);
	{
		trigger * tkill = trigger_killunit(keeper);
		add_trigger(&keeper->attribs, "timer", trigger_timeout(cast_level+2, tkill));
	}

	sprintf(buf, "%s beschwört einen Bergwächter.", unitname(mage));
	addmessage(r, mage->faction, buf, MSG_MAGIC, ML_INFO);

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Sturmwind - Beschwöre einen Sturmelementar
 * Stufe:      6
 * Gebiet:     Gwyrrd
 *
 * Wirkung:
 * Verdoppelt Geschwindigkeit aller angegebener Schiffe fuer diese
 * Runde.  Kombinierbar mit "Günstige Winde", aber nicht mit
 * "Luftschiff".
 *
 * Anstelle des alten ship->enchanted benutzen wir einen kurzfristigen
 * Curse.  Das ist zwar ein wenig aufwendiger, aber weitaus flexibler
 * und erlaubt es zB, die Dauer später problemlos zu verändern.
 *
 * Flags:
 *  (SHIPSPELL|ONSHIPCAST|OCEANCASTABLE|TESTRESISTANCE)
 */

static int
sp_stormwinds(castorder *co)
{
	faction *f;
	ship *sh;
	unit *u;
	int n, force;
	int erfolg = 0;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int power = co->force;
	spellparameter *pa = co->par;

	force = power;

	/* melden vorbereiten */
	for(f = factions; f; f = f->next ) freset(f, FL_DH);

	for (n = 0; n < pa->length; n++) {
		if (!force)
			break;

		if(pa->param[n]->flag == TARGET_RESISTS
				|| pa->param[n]->flag == TARGET_NOTFOUND)
			continue;

		sh = pa->param[n]->data.sh;

		/* mit C_SHIP_NODRIFT haben wir kein Problem */
		if(is_cursed(sh->attribs, C_SHIP_FLYING, 0) ) {
			sprintf(buf, "Es ist zu gefährlich, diesen Zauber auf ein "
					"fliegendes Schiff zu legen.");
			addmessage(r, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
			continue;
		}
		if(is_cursed(sh->attribs, C_SHIP_SPEEDUP, 0) ) {
			sprintf(buf, "Auf %s befindet sich bereits ein Zauber", shipname(sh));
			addmessage(r, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
			continue;
		}

		/* Duration = 1, nur diese Runde */
		create_curse(mage, &sh->attribs, C_SHIP_SPEEDUP, 0, power, 1, 0, 0);
		/* Da der Spruch nur diese Runde wirkt, brauchen wir kein
		 * set_cursedisplay() zu benutzten - es sieht eh niemand...  */
		erfolg++;
		force--;

		/* melden vorbereiten: */
		for(u = r->units; u; u = u->next ) {
			if(u->ship != sh )		/* nur den Schiffsbesatzungen! */
				continue;

			fset(u->faction, FL_DH);
		}

	}
	/* melden, 1x pro Partei auf Schiff und für den Magier */
	fset(mage->faction, FL_DH);
	for(u = r->units; u; u = u->next ) {
		if(fval(u->faction, FL_DH)) {
			freset(u->faction, FL_DH);
			if (erfolg > 0){
				sprintf(buf, "%s beschwört einen magischen Wind, der die Schiffe "
						"über das Wasser treibt.",
						cansee(u->faction, r, mage, 0) ? unitname(mage) : "Jemand");
				addmessage(r, u->faction, buf, MSG_EVENT, ML_INFO);
			}
		}
	}
	return erfolg;
}


/* ------------------------------------------------------------- */
/* Name:       Donnerbeben
 * Stufe:      6
 * Gebiet:     Gwyrrd
 *
 * Wirkung:
 * Zerstört Stufe*10 "Steineinheiten" aller Gebäude der Region, aber nie
 * mehr als 25% des gesamten Gebäudes (aber natürlich mindestens ein
 * Stein).
 *
 * Flags:
 *  (FARCASTING|REGIONSPELL|TESTRESISTANCE)
 */
static int
sp_earthquake(castorder *co)
{
	int kaputt;
	building *burg;
	unit *u;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	for (burg = r->buildings; burg; burg = burg->next){
		if(burg->size == 0 )
			continue;

		/* Schutzzauber */
		if(is_cursed(burg->attribs, C_MAGICSTONE, 0))
			continue;

		/* Magieresistenz */
		if (target_resists_magic(mage, burg, TYP_BUILDING, 0))
			continue;

		kaputt = min(10 * cast_level, burg->size / 4);
		kaputt = max(kaputt, 1);
		burg->size -= kaputt;
		if(burg->size == 0 ) {
			/* alle Einheiten hinausbefördern */
			for(u = r->units; u; u = u->next ) {
				if(u->building == burg ) {
					u->building = 0;
					freset(u, FL_OWNER);
				}
			}
			/* TODO: sollten die Insassen nicht Schaden nehmen? */
			destroy_building(burg);
		}
	}

	/* melden, 1x pro Partei */
	for (u = r->units; u; u = u->next) freset(u->faction, FL_DH);
	for (u = r->units; u; u = u->next ) {
		if(!fval(u->faction, FL_DH) ) {
			fset(u->faction, FL_DH);
			sprintf(buf, "%s läßt die Erde in %s erzittern.",
					cansee(u->faction, r, mage, 0) ? unitname(mage) : "Jemand",
					regionid(r));

			addmessage(r, u->faction, buf, MSG_EVENT, ML_INFO);
		}
	}
	return cast_level;
}


/* ------------------------------------------------------------- */
/* CHAOS / M_CHAOS / Draig */
/* ------------------------------------------------------------- */
void
patzer_peasantmob(castorder *co)
{
	int anteil = 6, n;
	unit *u;
	attrib *a;
	region *r;
	unit *mage = (unit *)co->magician;
	strlist *S;

	if (mage->region->land){
		r = mage->region;
	} else {
		r = co->rt;
	}

	if (r->land) {
		anteil += rand() % 4;
		n = rpeasants(r) * anteil / 10;
		rsetpeasants(r, rpeasants(r) - n);
		assert(rpeasants(r) >= 0);

		u = createunit(r, findfaction(MONSTER_FACTION), n, new_race[RC_PEASANT]);
		set_string(&u->name, "Bauernmob");
		/* guard(u, GUARD_ALL);  hier zu früh! Befehl BEWACHE setzten */
		sprintf(buf, "BEWACHE");
		S = makestrlist(buf);
		addlist(&u->orders, S);
		set_string(&u->thisorder, LOC(u->faction->locale, "defaultorder"));
		a = a_new(&at_unitdissolve);
		a->data.ca[0] = 1;  /* An rpeasants(r). */
		a->data.ca[1] = 10; /* 10% */
		a_add(&u->attribs, a);
		a_add(&u->attribs, make_hate(mage));

		sprintf(buf, "Ein Bauernmob erhebt sich und macht Jagd auf Schwarzmagier.");
		addmessage(r, 0, buf, MSG_MAGIC, ML_INFO);
	}
	return;
}


/* ------------------------------------------------------------- */
/* Name:       Waldbrand
 * Stufe:      10
 * Kategorie:  Region, negativ
 * Gebiet:     Draig
 * Wirkung:
 * Vernichtet 10-80% aller Baeume in der Region.  Kann sich auf benachbarte
 * Regionen ausbreiten, wenn diese (stark) bewaldet sind.  Für jeweils
 * 10 verbrannte Baeume in der Startregion gibts es eine 1%-Chance, dass
 * sich das Feuer auf stark bewaldete Nachbarregionen ausdehnt, auf
 * bewaldeten mit halb so hoher Wahrscheinlichkeit.  Dort verbrennen
 * dann prozentual halbsoviele bzw ein viertel soviele Baeume wie in der
 * Startregion.
 *
 * Im Extremfall: 1250 Baeume in Region, 80% davon verbrennen (1000).
 * Dann breitet es sich mit 100% Chance in stark bewaldete Regionen
 * aus, mit 50% in bewaldete.  Dort verbrennen dann 40% bzw 20% der Baeume.
 * Weiter als eine Nachbarregion breitet sich dass Feuer nicht aus.
 *
 * Sinn: Ein Feuer in einer "stark bewaldeten" Wueste hat so trotzdem kaum
 * eine Chance, sich weiter auszubreiten, waehrend ein Brand in einem Wald
 * sich fast mit Sicherheit weiter ausbreitet.
 *
 * Flags:
 * (FARCASTING | REGIONSPELL | TESTRESISTANCE)
 */
static int
sp_forest_fire(castorder *co)
{
	unit *u;
	region *nr;
	int prozent, chance, vernichtet;
#if GROWING_TREES
	int vernichtet_schoesslinge;
#endif
	direction_t i;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	prozent = (rand() % 71) + 10; 	/* 10 - 80% */
#if GROWING_TREES
	vernichtet = rtrees(r,2) * prozent / 100;
	vernichtet_schoesslinge = rtrees(r,1) * prozent / 100;
#else
	vernichtet = rtrees(r) * prozent / 100;
#endif

	if(!vernichtet ) {
		cmistake(mage, strdup(co->order), 198, MSG_MAGIC);
		return 0;
	}

#if GROWING_TREES
	rsettrees(r, 2, rtrees(r,2) - vernichtet);
	rsettrees(r, 1, rtrees(r,1) - vernichtet_schoesslinge);
#else
	rsettrees(r, rtrees(r) - vernichtet);
#endif
	chance = vernichtet / 10;	/* Chance, dass es sich ausbreitet */

	/* melden, 1x pro Partei */
	for (u = r->units; u; u = u->next) freset(u->faction, FL_DH);

	for(u = r->units; u; u = u->next ) {
		if(!fval(u->faction, FL_DH) ) {
			fset(u->faction, FL_DH);
			sprintf(buf, "%s erzeugt eine verheerende Feuersbrunst.  %d %s "
					"den Flammen zum Opfer.",
					cansee(u->faction, r, mage, 0) ? unitname(mage) : "Jemand",
					vernichtet,
					vernichtet == 1 ? "Baum fiel" : "Bäume fielen");
			addmessage(r, u->faction, buf, MSG_EVENT, ML_INFO);
		}
	}
	if(!fval(mage->faction, FL_DH)){
#if GROWING_TREES
		sprintf(buf, "%s erzeugt eine verheerende Feuersbrunst.  %d %s "
				"den Flammen zum Opfer.", unitname(mage), vernichtet+vernichtet_schoesslinge,
				vernichtet+vernichtet_schoesslinge == 1 ? "Baum fiel" : "Bäume fielen");
#else
		sprintf(buf, "%s erzeugt eine verheerende Feuersbrunst.  %d %s "
				"den Flammen zum Opfer.", unitname(mage), vernichtet,
				vernichtet == 1 ? "Baum fiel" : "Bäume fielen");
#endif
		addmessage(0, mage->faction, buf, MSG_MAGIC, ML_INFO);
	}

	for(i = 0; i < MAXDIRECTIONS; i++ ) {
		nr = rconnect(r, i);
		assert(nr);
		vernichtet = 0;

#if GROWING_TREES
		if(rtrees(nr,2) + rtrees(nr,1) >= 800) {
			if((rand() % 100) < chance ) {
				vernichtet = rtrees(nr,2) * prozent / 200;
				vernichtet_schoesslinge = rtrees(nr,1) * prozent / 200;
			}
		} else if(rtrees(nr,2) + rtrees(nr,1) >= 600) {
			if((rand() % 100) < chance / 2 ) {
				vernichtet = rtrees(nr,2) * prozent / 400;
				vernichtet_schoesslinge = rtrees(nr,1) * prozent / 400;
			}
		}
		
		if(vernichtet > 0  || vernichtet_schoesslinge > 0) {
			rsettrees(nr, 2, rtrees(nr,2) - vernichtet);
			rsettrees(nr, 1, rtrees(nr,1) - vernichtet_schoesslinge);
			sprintf(buf, "Der Waldbrand in %s griff auch auf %s "
					"über und %d %s.",
					regionid(r), regionid(nr), vernichtet+vernichtet_schoesslinge,
					vernichtet+vernichtet_schoesslinge == 1 ? "Baum verbrannte" : "Bäume verbrannten");
			for (u = nr->units; u; u = u->next) freset(u->faction, FL_DH);
			for(u = nr->units; u; u = u->next ) {
				if(!fval(u->faction, FL_DH) ) {
					fset(u->faction, FL_DH);
					addmessage(r, u->faction, buf, MSG_EVENT, ML_INFO);
				}
			}
			if(!fval(mage->faction, FL_DH)){
		  	addmessage(0, mage->faction, buf, MSG_MAGIC, ML_INFO);
			}
		}
#else
		if(rtrees(nr) >= 800) {
			if((rand() % 100) < chance ) vernichtet = rtrees(nr) * prozent / 200;
		} else if(rtrees(nr) >= 600) {
			if((rand() % 100) < chance / 2 ) vernichtet = rtrees(nr) * prozent / 400;
		}

		if(vernichtet > 0 ) {
			rsettrees(nr, rtrees(nr) - vernichtet);
			sprintf(buf, "Der Waldbrand in %s griff auch auf %s "
					"über und %d %s.",
					regionid(r), regionid(nr), vernichtet,
					vernichtet == 1 ? "Baum verbrannte" : "Bäume verbrannten");
			for (u = nr->units; u; u = u->next) freset(u->faction, FL_DH);
			for(u = nr->units; u; u = u->next ) {
				if(!fval(u->faction, FL_DH) ) {
					fset(u->faction, FL_DH);
					addmessage(r, u->faction, buf, MSG_EVENT, ML_INFO);
				}
			}
			if(!fval(mage->faction, FL_DH)){
		  	addmessage(0, mage->faction, buf, MSG_MAGIC, ML_INFO);
			}
		}
#endif
	}
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Chaosfluch
 * Stufe:      5
 * Gebiet:     Draig
 * Kategorie:  (Antimagie) Kraftreduzierer, Einheit, negativ
 * Wirkung:
 *  Auf einen Magier gezaubert verhindert/erschwert dieser Chaosfluch
 *  das Zaubern. Patzer werden warscheinlicher.
 *  Jeder Zauber muss erst gegen den Wiederstand des Fluchs gezaubert
 *  werden und schwächt dessen Antimagiewiederstand um 1.
 *  Wirkt max(Stufe(Magier) - Stufe(Ziel), rand(3)) Wochen
 * Patzer:
 *  Magier wird selbst betroffen
 *
 * Flags:
 *  (UNITSPELL | SPELLLEVEL | ONETARGET | TESTCANSEE | TESTRESISTANCE)
 *
 */
static int
sp_fumblecurse(castorder *co)
{
	unit *target;
	int rx, sx;
	int duration;
	int effect;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;
	spellparameter *pa = co->par;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	target = pa->param[0]->data.u;

	rx = rand()%3;
	sx = cast_level - effskill(target, SK_MAGIC);
	duration = max(sx, rx);

	effect = force/2;

	if (create_curse(mage, &target->attribs, C_FUMBLE, 0,
				force, duration+1, effect, 0) == NULL)
	{
		report_failure(mage, co->order);
		return 0;
	}

	set_curseflag(target->attribs, C_FUMBLE, 0, CURSE_ONLYONE);
	add_message(&target->faction->msgs, new_message(target->faction,
		"fumblecurse%u:unit%r:region", target, target->region));

	return cast_level;
}

void
patzer_fumblecurse(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;
	int effect;

	effect = force/2;

	if (create_curse(mage, &mage->attribs, C_FUMBLE, 0, force,
				(cast_level/2)+1, effect, 0))
	{
		add_message(&mage->faction->msgs, new_message(mage->faction,
			"magic_fumble%u:unit%r:region%s:command",
			mage, mage->region, strdup(co->order)));
		set_curseflag(mage->attribs, C_FUMBLE, 0, CURSE_ONLYONE);
	}
	return;
}

/* ------------------------------------------------------------- */
/* Name:       Drachenruf
 * Stufe:      11
 * Gebiet:     Draig
 * Kategorie:  Monster, Beschwörung, negativ
 *
 * Wirkung:
 *  In einer Wüste, Sumpf oder Gletscher gezaubert kann innerhalb der
 *  nächsten 6 Runden ein bis 6 Dracheneinheiten bis Größe Wyrm
 *  entstehen.
 *
 *  Mit Stufe 12-15 erscheinen Jung- oder normaler Drachen, mit Stufe
 *  16+ erscheinen normale Drachen oder Wyrme.
 *
 * Flag:
 *  (FARCASTING | REGIONSPELL | TESTRESISTANCE)
 */

static int
sp_summondragon(castorder *co)
{
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	unit *u;
	int cast_level = co->level;
	int power = co->force;
	regionlist *rl,*rl2;
	faction *f;
	int time;
	int number;
	int range;
	const race * race;

	f = findfaction(MONSTER_FACTION);

	if(rterrain(r) != T_SWAMP && rterrain(r) != T_DESERT
			&& rterrain(r) != T_GLACIER){
		report_failure(mage, co->order);
		return 0;
	}

	for(time = 1; time < 7; time++){
		if (rand()%100 < 25){
			switch(rand()%3){
			case 0:
				race = new_race[RC_WYRM];
				number = 1;
				break;

			case 1:
				race = new_race[RC_DRAGON];
				number = 2;
				break;

			case 2:
			default:
				race = new_race[RC_FIREDRAGON];
				number = 6;
				break;
			}
			{
				trigger * tsummon = trigger_createunit(r, f, race, number);
				add_trigger(&r->attribs, "timer", trigger_timeout(time, tsummon));
			}
		}
	}

	range = power;
	rl = all_in_range(r, range);

	for(rl2 = rl; rl2; rl2 = rl2->next) {
		for(u = rl2->region->units; u; u = u->next) {
			if (u->race == new_race[RC_WYRM] || u->race == new_race[RC_DRAGON]) {
				attrib * a = a_find(u->attribs, &at_targetregion);
				if (!a) {
					a = a_add(&u->attribs, make_targetregion(co->rt));
				} else {
					a->data.v = co->rt;
				}
				sprintf(buf, "Kommt aus: %s, Will nach: %s", regionid(rl2->region), regionid(co->rt));
				usetprivate(u, buf);
			}
		}
	}

	add_message(&mage->faction->msgs, new_message(mage->faction,
				"summondragon%u:unit%r:region%s:command%u:unit%r:region",
				mage, mage->region, strdup(co->order),mage, co->rt));

	free_regionlist(rl);
	return cast_level;
}

#if USE_FIREWALL
/* ------------------------------------------------------------- */
/* Name:       Feuerwand
 * Stufe:
 * Gebiet:     Draig
 * Kategorie:  Region, negativ
 * Flag:
 * Kosten:     SPC_LINEAR
 * Aura:
 * Komponenten:
 *
 * Wirkung:
 *   eine Wand aus Feuer entsteht in der angegebenen Richtung
 *
 *   Was für eine Wirkung hat die?
 */

typedef struct wallcurse {
	curse * buddy;
	border * wall;
} wallcurse;

void
wall_vigour(curse* c, int delta)
{
	wallcurse * wc = (wallcurse*)c->data;
	assert(wc->buddy->vigour==c->vigour);
	wc->buddy->vigour += delta;
	if (wc->buddy->vigour<=0) {
		erase_border(wc->wall);
		wc->wall = NULL;
		((wallcurse*)wc->buddy->data)->wall = NULL;
	}
}

const curse_type ct_firewall = {
	CURSETYP_NORM, 0, (M_DURATION | M_VIGOUR | NO_MERGE),
	"Feuerwand",
	"Eine Feuerwand blockiert die Ein- und Ausreise",
	NULL, /* curseinfo */
	wall_vigour /* change_vigour */
};

void
cw_init(attrib * a) {
	curse * c;
	curse_init(a);
	c = (curse*)a->data.v;
	c->data = calloc(sizeof(wallcurse), 1);
}

void
cw_write(const attrib * a, FILE * f) {
	border * b = ((wallcurse*)((curse*)a->data.v)->data)->wall;
	curse_write(a, f);
	fprintf(f, "%d ", b->id);
}

typedef struct bresvole {
	unsigned int id;
	curse * self;
} bresolve;

void *
resolve_buddy(void * data) {
	bresolve * br = (bresolve*)data;
	border * b = find_border(br->id);
	if (b && b->from && b->to) {
		attrib * a = a_find(b->from->attribs, &at_cursewall);
		while (a && a->data.v!=br->self) {
			curse * c = (curse*)a->data.v;
			wallcurse * wc = (wallcurse*)c->data;
			if (wc->wall->id==br->id) break;
			a = a->nexttype;
		}
		if (!a) {
			a = a_find(b->to->attribs, &at_cursewall);
			while (a && a->data.v!=br->self) {
				curse * c = (curse*)a->data.v;
				wallcurse * wc = (wallcurse*)c->data;
				if (wc->wall->id==br->id) break;
				a = a->nexttype;
			}
		}
		if (a) {
			curse * c = (curse*)a->data.v;
			free(br);
			return c;
		}
	}
	return NULL;
}

int
cw_read(attrib * a, FILE * f)
{
	bresolve * br = calloc(sizeof(bresolve), 1);
	curse * c = (curse*)a->data.v;
	wallcurse * wc = (wallcurse*)c->data;

	curse_read(a, f);
	br->self = c;
	fscanf(f, "%d ", &br->id);
	ur_add((void *)br->id, (void**)&wc->wall, resolve_borderid);
	ur_add((void *)br, (void**)&wc->buddy, resolve_buddy);
	return 1;
}

attrib_type at_cursewall =
{
	"cursewall",
	cw_init,
	curse_done,
	curse_age,
	cw_write,
	cw_read,
	ATF_CURSE
};

static const char *
fire_name(const border * b, const region * r, const faction * f, int gflags)
{
	unused(f);
	unused(r);
	unused(b);
	if (gflags & GF_ARTICLE)
		return "eine Feuerwand";
	else
		return "Feuerwand";
}

static void
wall_init(border * b)
{
	b->data = calloc(sizeof(wall_data), 1);
}

static void
wall_destroy(border * b)
{
	free(b->data);
}

static void
wall_read(border * b, FILE * f)
{
	wall_data * fd = (wall_data*)b->data;
	int mno;
	assert(fd);
	fscanf(f, "%d %d ", &mno, &fd->force);
	fd->mage = findunitg(mno, NULL);
	fd->active = true;
	if (!fd->mage)
		ur_add((void*)mno, (void**)&fd->mage, resolve_unit);
}

static void
wall_write(const border * b, FILE * f)
{
	wall_data * fd = (wall_data*)b->data;
	fprintf(f, "%d %d ", fd->mage?fd->mage->no:0, fd->force);
}

static void
wall_move(const border * b, struct unit * u, const struct region * from, const struct region * to)
{
	wall_data * fd = (wall_data*)b->data;
	int hp = dice(2, fd->force) * u->number;
	if (fd->active) {
		hp = min (u->hp, hp);
		u->hp -= hp;
		if (u->hp) add_message(&u->faction->msgs,
			new_message(u->faction, "firewall_damage%r:region%u:unit", from, u));
		else add_message(&u->faction->msgs,
			new_message(u->faction, "firewall_death%r:region%u:unit", from, u));
		if (u->number>u->hp) {
			scale_number(u, u->hp);
			u->hp = u->number;
		}
	}
	unused(from);
	unused(to);
}

border_type bt_firewall = {
	"firewall",
	b_transparent, /* transparent */
	wall_init, /* init */
	wall_destroy, /* destroy */
	wall_read, /* read */
	wall_write, /* write */
	b_blocknone, /* block */
	fire_name, /* name */
	b_rvisible, /* rvisible */
	b_finvisible, /* fvisible */
	b_uinvisible, /* uvisible */
	NULL,
	wall_move
};

static int
sp_firewall(castorder *co)
{
	unit * u;
	border * b;
	wall_data * fd;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;
	spellparameter *pa = co->par;
	direction_t dir;
	region * r2;

	dir = finddirection(pa->param[0]->data.s, mage->faction->locale);
	if (dir<MAXDIRECTIONS && dir!=NODIRECTION){
		r2 = rconnect(r, dir);
	} else {
		report_failure(mage, co->order);
		return 0;
	}

	if (!r2 || r2==r) {
		report_failure(mage, co->order);
		return 0;
	}

	b = new_border(&bt_firewall, r, r2);
	fd = (wall_data*)b->data;
	fd->force = (force+1)/2;
	fd->mage = mage;
	fd->active = false;

	a_add(&b->attribs, a_new(&at_countdown))->data.i = cast_level;

	/* melden, 1x pro Partei */
	for (u = r->units; u; u = u->next) freset(u->faction, FL_DH);

	for (u = r->units; u; u = u->next ) {
		if(!fval(u->faction, FL_DH) ) {
			fset(u->faction, FL_DH);
			add_message(&u->faction->msgs, new_message(u->faction,
				"firewall_effect%u:mage%r:region",
				cansee(u->faction, r, mage, 0) ? mage:NULL, r));
		}
	}
	if(!fval(mage->faction, FL_DH)){
		add_message(&mage->faction->msgs, new_message(mage->faction,
			"firewall_effect%u:mage%r:region", mage, r));
	}

	return cast_level;
}

/* ------------------------------------------------------------- */

static const char *
wisps_name(const border * b, const region * r, const faction * f, int gflags)
{
	unused(f);
	unused(r);
	unused(b);
	if (gflags & GF_ARTICLE)
		return "eine Gruppe von Irrlichtern";
	else
		return "Irrlichter";
}

border_type bt_wisps = {
	"wisps",
	b_transparent, /* transparent */
	wall_init, /* init */
	wall_destroy, /* destroy */
	wall_read, /* read */
	wall_write, /* write */
	b_blocknone, /* block */
	wisps_name, /* name */
	b_rvisible, /* rvisible */
	b_fvisible, /* fvisible */
	b_uvisible, /* uvisible */
};

static int
sp_wisps(castorder *co)
{
	unit * u;
	border * b;
	wall_data * fd;
	region * r2;
	direction_t dir;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;
	spellparameter *pa = co->par;

	dir = finddirection(pa->param[0]->data.s, mage->faction->locale);
	r2 = rconnect(r, dir);

	if (!r2) {
		report_failure(mage, co->order);
		return 0;
	}

	b = new_border(&bt_wisps, r, r2);
	fd = (wall_data*)b->data;
	fd->force = (force+1)/2;
	fd->mage = mage;
	fd->active = false;

	a_add(&b->attribs, a_new(&at_countdown))->data.i = cast_level;

	/* melden, 1x pro Partei */
	for (u = r->units; u; u = u->next) freset(u->faction, FL_DH);

	for(u = r->units; u; u = u->next ) {
		if(!fval(u->faction, FL_DH) ) {
			fset(u->faction, FL_DH);
			add_message(&u->faction->msgs, new_message(u->faction,
				"wisps_effect%u:mage%r:region",
				cansee(u->faction, r, mage, 0) ? mage:NULL, r));
		}
	}
	if(!fval(mage->faction, FL_DH)){
		add_message(&mage->faction->msgs, new_message(mage->faction,
			"wisps_effect%u:mage%r:region", mage, r));
	}

	return cast_level;
}
#endif

/* ------------------------------------------------------------- */
/* Name:       Unheilige Kraft
 * Stufe:      10
 * Gebiet:     Draig
 * Kategorie:  Untote Einheit, positiv
 *
 * Wirkung:
 *  transformiert (Stufe)W10 Untote in ihre stärkere Form
 *
 *
 * Flag:
 * 	(SPELLLEVEL | TESTCANSEE)
 */

static int
sp_unholypower(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	spellparameter *pa = co->par;
	int i;
	int n;

	n = dice(co->force, 10);

	for (i = 0; i < pa->length && n > 0; i++) {
		const race * target_race;
		unit *u;

		if(pa->param[i]->flag == TARGET_RESISTS
				|| pa->param[i]->flag == TARGET_NOTFOUND)
			continue;

		u = pa->param[i]->data.u;

		switch (old_race(u->race)) {
		case RC_SKELETON:
			target_race = new_race[RC_SKELETON_LORD]; 
			break;
		case RC_ZOMBIE:
			target_race = new_race[RC_ZOMBIE_LORD]; 
			break;
		case RC_GHOUL:
			target_race = new_race[RC_GHOUL_LORD]; 
			break;
		default:
			cmistake(mage, strdup(co->order), 284, MSG_MAGIC);
			continue;
		}

		if(u->number <= n) {
			n -= u->number;
			u->race = target_race;
			add_message(&co->rt->msgs, msg_message("unholypower_effect", 
				"mage target race", mage, u, target_race));
		} else {
			unit *un;

			/* Wird hoffentlich niemals vorkommen. Es gibt im Source
			 * vermutlich eine ganze Reihe von Stellen, wo das nicht
			 * korrekt abgefangen wird. Besser (aber nicht gerade einfach)
			 * wäre es, eine solche Konstruktion irgendwie zu kapseln. */
			if(fval(u, FL_LOCKED) || fval(u, FL_HUNGER)
					|| is_cursed(u->attribs, C_SLAVE, 0)) {
				cmistake(mage, strdup(co->order), 74, MSG_MAGIC);
				continue;
			}

			un = createunit(co->rt, u->faction, 0, target_race);
			if (co->rt==u->region) {
				un->building = u->building;
				un->ship = u->ship;
			}
			transfermen(u, un, n);
			add_message(&co->rt->msgs, msg_message("unholypower_limitedeffect", 
				"mage target race amount",
				mage, u, target_race, n));
			n = 0;
		}
	}

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Todeswolke
 * Stufe:      11
 * Gebiet:     Draig
 * Kategorie:  Region, negativ
 *
 * Wirkung:
 *   alle Personen in der Region verlieren 8 HP * Magieresistenz
 *   Wirkt gegen MR
 *   Rüstung wirkt nicht
 * Patzer:
 *   Magier gerät in den Staub und verliert zufällige Zahl von HP bis
 *   auf max(hp,2)
 * Besonderheiten:
 *   Magier verliert 4 HP pro Entfernung.
 * Missbrauchsmöglichkeit:
 *   Hat der Magier mehr HP als Rasse des Feindes (extrem: Dämon/Goblin)
 *   so kann er per Farcasting durch mehrmaliges Zaubern eine
 *   Nachbarregion auslöschen. Darum sollte dieser Spruch nur einmal auf
 *   eine Region gelegt werden können.
 *
 * Flag:
 * 	(FARCASTING | REGIONSPELL | TESTRESISTANCE)
 */

static int
sp_deathcloud(castorder *co)
{
	unit *u;
	double damage;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	attrib *a = a_find(r->attribs, &at_deathcloud);

	if(a){
		report_failure(mage, co->order);
		return 0;
	}

	for (u = r->units; u; u = u->next){
		if (target_resists_magic(mage, u, TYP_UNIT, 0)){
			continue;
		}
		/* Jede Person verliert 8 HP */
		damage = 8 * u->number;
		/* Reduziert durch Magieresistenz */
		damage *= (100.0 - magic_resistance(u))/100.0;
		change_hitpoints(u, -(int)damage);
	}

	a = a_add(&r->attribs, a_new(&at_deathcloud));

	/* melden, 1x pro Partei */
	for (u = r->units; u; u = u->next) freset(u->faction, FL_DH);

	for(u = r->units; u; u = u->next ) {
		if(!fval(u->faction, FL_DH) ) {
			fset(u->faction, FL_DH);
			add_message(&u->faction->msgs, new_message(u->faction,
				"deathcloud_effect%u:mage%r:region",
				cansee(u->faction, r, mage, 0) ? mage:NULL, r));
		}
	}
	if(!fval(mage->faction, FL_DH)){
		add_message(&mage->faction->msgs, new_message(mage->faction,
			"deathcloud_effect%u:mage%r:region", mage, r));
	}

	return cast_level;
}

void
patzer_deathcloud(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int hp = (mage->hp - 2);

	change_hitpoints(mage, -rand()%hp);

	add_message(&mage->faction->msgs, new_message(mage->faction,
		"magic_fumble%u:unit%r:region%s:command",
		mage, mage->region, strdup(co->order)));

	return;
}

/* ------------------------------------------------------------- */
/* Name:           Trollgürtel
 * Stufe:          9
 * Gebiet:         Draig
 * Kategorie:      Artefakt
 * Wirkung:
 *   Artefakt. *50 GE, +2 Schaden, +1 Rüstung
*/

static int
sp_create_trollbelt(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	change_item(mage,I_TROLLBELT,1);

	creation_message(mage, I_TROLLBELT);
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Erschaffe ein Flammenschwert
 * Stufe:      12
 * Gebiet:     Draig
 * Kategorie:      Artefakt
 * Wirkung:
 *   Artefakt.
 *   3d6+10 Schaden, +1 auf AT, +1 auf DF, schleudert Feuerball
 */

static int
sp_create_firesword(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	change_item(mage,I_FIRESWORD,1);
	creation_message(mage, I_FIRESWORD);
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:      Pest
 * Stufe:     7
 * Gebiet:    Draig
 * Wirkung:
 *  ruft eine Pest in der Region hervor.
 * Flags:
 *  (FARCASTING | REGIONSPELL | TESTRESISTANCE)
 * Syntax: ZAUBER [REGION x y] "Pest"
 */
static int
sp_plague(castorder *co)
{
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	plagues(r, true);

	sprintf(buf, "%s ruft eine Pest in %s hervor.", unitname(mage),
			regionid(r));
	addmessage(0, mage->faction, buf,  MSG_MAGIC, ML_INFO);
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Beschwöre Schattendämon
 * Stufe:      8
 * Gebiet:     Draig
 * Kategorie:  Beschwörung, positiv
 * Wirkung:
 *  Der Magier beschwört Stufe^2 Schattendämonen.
 *  Schattendämonen haben Tarnung = (Magie_Magier+ Tarnung_Magier)/2 und
 *  Wahrnehmung 1. Sie haben einen Attacke-Bonus von 8, einen
 *  Verteidigungsbonus von 11 und machen 2d3 Schaden.  Sie entziehen bei
 *  einem Treffer dem Getroffenen einen Attacke- oder
 *  Verteidigungspunkt.  (50% Chance.) Sie haben 25 Hitpoints und
 *  Rüstungsschutz 3.
 * Flag:
 *  (SPELLLEVEL)
 */
static int
sp_summonshadow(castorder *co)
{
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;
	unit *u;
	int val;

	u = createunit(r, mage->faction, force*force, new_race[RC_SHADOW]);
	if (r==mage->region) {
		u->building = mage->building;
		u->ship = mage->ship;
	}
	if (fval(mage, FL_PARTEITARNUNG))
		fset(u, FL_PARTEITARNUNG);

	/* Bekommen Tarnung = (Magie+Tarnung)/2 und Wahrnehmung 1. */
	val = get_level(mage, SK_MAGIC) + get_level(mage, SK_STEALTH);

	set_level(u, SK_STEALTH, val);
	set_level(u, SK_OBSERVATION, 1);

	sprintf(buf, "%s beschwört %d Dämonen aus dem Reich der Schatten.",
		unitname(mage), force*force);
	addmessage(0, mage->faction, buf, MSG_MAGIC, ML_INFO);

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Beschwöre Schattenmeister
 * Stufe:      12
 * Gebiet:     Draig
 * Kategorie:  Beschwörung, positiv
 * Wirkung:
 *  Diese höheren Schattendämonen sind erheblich gefährlicher als die
 *  einfachen Schattendämonen.  Sie haben Tarnung entsprechend dem
 *  Magietalent des Beschwörer-1 und Wahrnehmung 5, 75 HP,
 *  Rüstungsschutz 4, Attacke-Bonus 11 und Verteidigungsbonus 13, machen
 *  bei einem Treffer 2d4 Schaden, entziehen einen Stärkepunkt und
 *  entziehen 5 Talenttage in einem zufälligen Talent.
 *  Stufe^2 Dämonen.
 *
 * Flag:
 *  (SPELLLEVEL)
 * */
static int
sp_summonshadowlords(castorder *co)
{
	unit *u;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;

	u = createunit(r, mage->faction, force*force, new_race[RC_SHADOWLORD]);
	if (r==mage->region) {
		u->building = mage->building;
		u->ship = mage->ship;
	}
	if (fval(mage, FL_PARTEITARNUNG))
		fset(u, FL_PARTEITARNUNG);

	/* Bekommen Tarnung = Magie und Wahrnehmung 5. */
	set_level(u, SK_STEALTH, get_level(mage, SK_MAGIC));
	set_level(u, SK_OBSERVATION, 5);
	sprintf(buf, "%s beschwört %d Schattenmeister.",
		unitname(mage), force*force);
	addmessage(0, mage->faction, buf, MSG_MAGIC, ML_INFO);
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Chaossog
 * Stufe:      14
 * Gebiet:     Draig
 * Kategorie:  Teleport
 * Wirkung:
 *  Durch das Opfern von 500 Bauern kann der Chaosmagier ein Tor zur
 *  astralen Welt öffnen. Das Tor kann im Folgemonat verwendet werden,
 *  es löst sich am Ende des Folgemonats auf.
 *
 * Flag:  (0)
 */
static int
sp_chaossuction(castorder *co)
{
	unit *u;
	region *rt;
	faction *f;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	if(getplane(r)) {
		/* Der Zauber funktioniert nur in der materiellen Welt. */
		cmistake(mage, strdup(co->order), 190, MSG_MAGIC);
		return 0;
	}

	rt  = r_standard_to_astral(r);

	if(!rt || is_cursed(rt->attribs, C_ASTRALBLOCK, 0)) {
		/* Hier gibt es keine Verbindung zur astralen Welt.*/
		cmistake(mage, strdup(co->order), 216, MSG_MAGIC);
		return 0;
	}

	create_special_direction(r, rt->x, rt->y, 2,
			"Ein Wirbel aus reinem Chaos zieht über die Region.",
			"Wirbel");
	create_special_direction(rt, r->x, r->y, 2,
			"Ein Wirbel aus reinem Chaos zieht über die Region.",
			"Wirbel");

	for (f = factions; f; f = f->next) freset(f, FL_DH);
	for (u = r->units; u; u = u->next) {
		if (!fval(u->faction, FL_DH)) {
			fset(u->faction, FL_DH);
			sprintf(buf, "%s öffnete ein Chaostor.",
					cansee(u->faction, r, mage, 0)?unitname(mage):"Jemand");
			addmessage(r, u->faction, buf, MSG_EVENT, ML_INFO);
		}
	}
	for (u = rt->units; u; u = u->next) freset(u->faction, FL_DH);

	for (u = rt->units; u; u = u->next) {
		if (!fval(u->faction, FL_DH)) {
			fset(u->faction, FL_DH);
			addmessage(r, u->faction, "Ein Wirbel aus blendendem Licht erscheint.",
				MSG_EVENT, ML_INFO);
		}
	}
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Magic Boost - Gabe des Chaos
 * Stufe:      3
 * Gebiet:     Draig
 * Kategorie:  Einheit, positiv
 *
 * Wirkung:
 *   Erhöht die maximalen Magiepunkte und die monatliche Regeneration auf
 *   das doppelte. Dauer: 4 Wochen Danach sinkt beides auf die Hälfte des
 *   normalen ab.
 * Dauer: 6 Wochen
 * Patzer:
 *   permanenter Stufen- (Talenttage), Regenerations- oder maxMP-Verlust
 * Besonderheiten:
 *   Patzer können während der Zauberdauer häufiger auftreten derzeit
 *   +10%
 *
 * Flag:
 *  (ONSHIPCAST)
 */

static int
sp_magicboost(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;

	/* fehler, wenn schon ein boost */
	if(is_cursed(mage->attribs, C_MBOOST, 0) == true){
		report_failure(mage, co->order);
		return 0;
	}
	/* (magier, attribut, curseid, id2, power, duration, effect, men) */
	create_curse(mage, &mage->attribs, C_MBOOST, 0, power, 10, 6, 1);
	create_curse(mage, &mage->attribs, C_AURA, 0, power, 4, 200, 1);

	{
		trigger * tsummon = trigger_createcurse(mage, mage, C_AURA, 0, power, 6, 50, 1);
		add_trigger(&mage->attribs, "timer", trigger_timeout(5, tsummon));
	}

	/* kann nicht durch Antimagie beeinflusst werden */
	set_curseflag(mage->attribs, C_MBOOST, 0, CURSE_IMMUN);

	add_message(&mage->faction->msgs, new_message(mage->faction,
		"magicboost_effect%u:unit%r:region%s:command",
		mage, mage->region, strdup(co->order)));

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       kleines Blutopfer
 * Stufe:      4
 * Gebiet:     Draig
 * Kategorie:  Einheit, positiv
 *
 * Wirkung:
 *   Hitpoints to Aura:
 *   skill < 8  = 4:1
 *   skill < 12 = 3:1
 *   skill < 15 = 2:1
 *   skill < 18 = 1:2
 *   skill >    = 2:1
 * Patzer:
 *   permanenter HP verlust
 *
 * Flag:
 *  (ONSHIPCAST)
 */
static int
sp_bloodsacrifice(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int aura, damage;
	int skill = eff_skill(mage, SK_MAGIC, mage->region);
	int hp = mage->hp - 5; /* braucht noch 4 HP zum Bezahlen des
																		 Spruchs, und 1 HP zum Überleben*/

	if (hp <= 0){
		report_failure(mage, co->order);
		return 0;
	}

	damage = min(hp, dice_rand("4d12"));

	if (skill < 8) {
		aura = damage / 4;
	} else if (skill < 12){
		aura = damage / 3;
	} else if (skill < 15){
		aura = damage / 2;
	} else if (skill < 18){
		aura = damage;
	} else {
		aura = damage * 2;
	}

	if (aura <= 0){
		report_failure(mage, co->order);
		return 0;
	}

	change_spellpoints(mage, aura);
	use_pooled(mage, mage->region, R_HITPOINTS, damage);
	ADDMSG(&mage->faction->msgs,
		msg_message("sp_bloodsacrifice_effect",
		"unit region command amount",
		mage, mage->region, strdup(co->order), aura));
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Totenruf - Mächte des Todes
 * Stufe:      6
 * Gebiet:     Draig
 * Kategorie:  Beschwörung, positiv
 * Flag:       FARCASTING
 * Wirkung:
 *   Untote aus deathcounther ziehen, bis Stufe*10 Stück
 *
 * Patzer:
 *   Erzeugt Monsteruntote
 */
static int
sp_summonundead(castorder *co)
{
	int undead;
	unit *u;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;
	const race * race = new_race[RC_SKELETON];

	if (!r->land || deathcount(r) == 0) {
		sprintf(buf, "%s in %s: In %s sind keine Gräber.", unitname(mage),
				regionid(mage->region), regionid(r));
		addmessage(0, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
		return 0;
	}

	undead = min(deathcount(r), 2 + lovar(force * 10));

	if(cast_level <= 8) {
		race = new_race[RC_SKELETON];
	} else if(cast_level <= 12) {
		race = new_race[RC_ZOMBIE];
	} else {
		race = new_race[RC_GHOUL];
	}

	u = make_undead_unit(r, mage->faction, undead, race);
	if (r==mage->region) {
		u->building = mage->building;
		u->ship = mage->ship;
	}
	if (fval(mage, FL_PARTEITARNUNG))
		fset(u, FL_PARTEITARNUNG);

	sprintf(buf, "%s erweckt %d Untote aus ihren Gräbern.",
			unitname(mage), undead);
	addmessage(0, mage->faction, buf, MSG_MAGIC, ML_INFO);

	/* melden, 1x pro Partei */
	for (u = r->units; u; u = u->next) freset(u->faction, FL_DH);

	for(u = r->units; u; u = u->next ) {
		if(!fval(u->faction, FL_DH) ) {
			fset(u->faction, FL_DH);
			sprintf(buf, "%s stört die Ruhe der Toten",
				cansee(u->faction, r, mage, 0) ? unitname(mage) : "Jemand");
			addmessage(r, u->faction, buf, MSG_EVENT, ML_INFO);
		}
	}
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Astraler Sog
 * Stufe:      9
 * Gebiet:     Draig
 * Kategorie:  Region, negativ
 * Wirkung:
 *   Allen Magier in der betroffenen Region wird eine Teil ihrer
 *   Magischen Kraft in die Gefilde des Chaos entzogen Jeder Magier im
 *   Einflussbereich verliert Stufe(Zaubernden)*5% seiner Magiepunkte.
 *   Keine Regeneration in der Woche (fehlt noch)
 *
 * Flag:
 *   (REGIONSPELL | TESTRESISTANCE)
 */

static int
sp_auraleak(castorder *co)
{
	int lost_aura;
	double lost;
	unit *u;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	lost = min(0.95, cast_level * 0.05);

	for(u = r->units; u; u = u->next) {
		if (is_mage(u)){
			/* Magieresistenz Einheit?  Bei gegenerischen Magiern nur sehr
			 * geringe Chance auf Erfolg wg erhöhter MR, würde Spruch sinnlos
			 * machen */
			lost_aura = (int)(get_spellpoints(u)*lost);
			change_spellpoints(u, -lost_aura);
		}
		freset(u->faction, FL_DH);
	}
	for (u = r->units; u; u = u->next) {
		if (!fval(u->faction, FL_DH)) {
			fset(u->faction, FL_DH);
			if (cansee(u->faction, r, mage, 0)) {
				sprintf(buf, "%s rief in %s einen Riss in dem Gefüge der Magie "
						"hervor, der alle magische Kraft aus der Region riss.",
						unitname(mage), regionid(r));
			} else {
				sprintf(buf, "In %s entstand ein Riss in dem Gefüge der Magie, "
						"der alle magische Kraft aus der Region riss.",
						regionid(r));
			}
			addmessage(r, u->faction, buf, MSG_EVENT, ML_WARN);
		}
	}
	return cast_level;
}

/* ------------------------------------------------------------- */
/* BARDE  - CERDDOR*/
/* ------------------------------------------------------------- */
/* Name:       Plappermaul
 * Stufe:      4
 * Gebiet:     Cerddor
 * Kategorie:  Einheit
 *
 * Wirkung:
 *  Einheit ausspionieren. Gibt auch Zauber und Kampfstatus aus.  Wirkt
 *  gegen Magieresistenz. Ist diese zu hoch, so wird der Zauber entdeckt
 *  (Meldung) und der Zauberer erhält nur die Talente, nicht die Werte
 *  der Einheit und auch keine Zauber.
 *
 * Flag:
 *  (UNITSPELL | ONETARGET | TESTCANSEE)
 */
static int
sp_babbler(castorder *co)
{
	unit *target;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	spellparameter *pa = co->par;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	target = pa->param[0]->data.u;

	if (target->faction == mage->faction){
		/* Die Einheit ist eine der unsrigen */
		cmistake(mage, strdup(co->order), 45, MSG_MAGIC);
	}

	/* Magieresistenz Unit */
	if (target_resists_magic(mage, target, TYP_UNIT, 0)){
		spy_message(5, mage, target);
		sprintf(buf, "%s hat einen feuchtfröhlichen Abend in der Taverne "
				"verbracht. Ausser einem fürchterlichen Brummschädel ist da auch "
				"noch das dumme Gefühl %s seine ganze Lebensgeschichte "
				"erzählt zu haben.", unitname(target),
				cansee(target->faction, r, mage, 0)? "irgendjemanden":unitname(mage));
		addmessage(r, target->faction, buf, MSG_EVENT, ML_WARN);

	} else {
		spy_message(100, mage, target);
		sprintf(buf, "%s hat einen feuchtfröhlichen Abend in der Taverne "
				"verbracht. Ausser einem fürchterlichen Brummschädel ist da auch "
				"noch das dumme Gefühl die ganze Taverne mit seiner Lebensgeschichte "
				"unterhalten zu haben.", unitname(target));
		addmessage(r, target->faction, buf, MSG_EVENT, ML_WARN);
	}
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Magie analysieren - Gebäude, Schiffe, Region
 * Name:       Lied des Ortes analysieren
 * Stufe:      8
 * Gebiet:     Cerddor
 *
 * Wirkung:
 *  Zeigt die Verzauberungen eines Objekts an (curse->name,
 *  cures->info). Aus der Differenz Spruchstärke und Curse->vigour
 *  ergibt sich die Chance den Spruch zu identifizieren ((force -
 *  c->vigour)*10 + 100 %).
 *
 * Flag:
 *  (SPELLLEVEL|ONSHIPCAST)
 */
static int
sp_analysesong_obj(castorder *co)
{
	int obj;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;
	spellparameter *pa = co->par;

	obj = pa->param[0]->typ;

	switch(obj){
	case SPP_REGION:
		magicanalyse_region(r, mage, force);
		break;

	case SPP_BUILDING:
		{
			building *b = pa->param[0]->data.b;
			magicanalyse_building(b, mage, force);
			break;
		}
	case SPP_SHIP:
		{
			ship * sh = pa->param[0]->data.sh;
			magicanalyse_ship(sh, mage, force);
			break;
		}
	default:
		/* Syntax fehlerhaft */
		return 0;
	}

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:           Gesang des Lebens analysieren
 * Name:           Magie analysieren - Unit
 * Stufe:          5
 * Gebiet:         Cerddor
 * Wirkung:
 *  Zeigt die Verzauberungen eines Objekts an (curse->name,
 *  cures->info). Aus der Differenz Spruchstärke und Curse->vigour
 *  ergibt sich die Chance den Spruch zu identifizieren ((force -
 *  c->vigour)*10 + 100 %).
 *
 * Flag:
 *  (UNITSPELL|ONSHIPCAST|ONETARGET|TESTCANSEE)
 */
static int
sp_analysesong_unit(castorder *co)
{
	unit *u;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;
	spellparameter *pa = co->par;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	/* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
	 * abbrechen aber kosten lassen */
	if(pa->param[0]->flag == TARGET_RESISTS) return cast_level;

	u = pa->param[0]->data.u;

	magicanalyse_unit(u, mage, force);

	return cast_level;
}
/* ------------------------------------------------------------- */
/* Name:           Charming
 * Stufe:          13
 * Gebiet:         Cerddor
 * Flag:           UNITSPELL
 * Wirkung:
 *   bezauberte Einheit wechselt 'virtuell' die Partei und führt fremde
 *   Befehle aus.
 *   Dauer: 3 - force+2 Wochen
 *   Wirkt gegen Magieresistenz
 *
 *   wirkt auf eine Einheit mit maximal Talent Personen normal. Für jede
 *   zusätzliche Person gibt es einen Bonus auf Magieresistenz, also auf
 *   nichtgelingen, von 10%.
 *
 *   Das höchste Talent der Einheit darf maximal so hoch sein wie das
 *   Magietalent des Magiers. Für jeden Talentpunkt mehr gibt es einen
 *   Bonus auf Magieresistenz von 15%, was dazu führt, das bei +2 Stufen
 *   die Magiersistenz bei 90% liegt.
 *
 *   Migrantenzählung muss Einheit überspringen
 *
 *   Attackiere verbieten
 * Flags:
 *   (UNITSPELL | ONETARGET | TESTCANSEE)
 */
static int
sp_charmingsong(castorder *co)
{
	unit *target;
	int duration;
	skill_t i;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;
	spellparameter *pa = co->par;
	int resist_bonus = 0;
	int tb = 0;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	target = pa->param[0]->data.u;

	/* Auf eigene Einheiten versucht zu zaubern? Garantiert Tippfehler */
	if (target->faction == mage->faction){
		/* Die Einheit ist eine der unsrigen */
		cmistake(mage, strdup(co->order), 45, MSG_MAGIC);
	}

	/* Magieresistensbonus für mehr als Stufe Personen */
	if (target->number > force) {
		resist_bonus += (target->number - force) * 10;
	}
	/* Magieresistensbonus für höhere Talentwerte */
	for(i = 0; i < MAXSKILLS; i++){
		int sk = effskill(target, i);
		if (tb < sk) tb = sk;
	}
	tb -= effskill(mage, SK_MAGIC);
	if(tb > 0){
		resist_bonus += tb * 15;
	}
	/* Magieresistenz */
	if (target_resists_magic(mage, target, TYP_UNIT, resist_bonus)){
		report_failure(mage, co->order);
		sprintf(buf, "%s fühlt sich einen Moment lang benommen und desorientiert.",
				unitname(target));
		addmessage(target->region, target->faction, buf, MSG_EVENT, ML_WARN);
		return 0;
	}

	duration = 3 + rand()%force;
	{
		trigger * trestore = trigger_changefaction(target, target->faction);
		/* läuft die Dauer ab, setze Partei zurück */
		add_trigger(&target->attribs, "timer", trigger_timeout(duration, trestore));
		/* wird die alte Partei von Target aufgelöst, dann auch diese Einheit */
		add_trigger(&target->faction->attribs, "destroy", trigger_killunit(target));
		/* wird die neue Partei von Target aufgelöst, dann auch diese Einheit */
		add_trigger(&mage->faction->attribs, "destroy", trigger_killunit(target));
	}
	/* sperre ATTACKIERE, GIB PERSON und überspringe Migranten */
	create_curse(mage, &target->attribs, C_SLAVE, 0, force, duration, 0, 0);

	/* setze Partei um und lösche langen Befehl aus Sicherheitsgründen */
	u_setfaction(target,mage->faction);
	set_string(&target->thisorder, "");

	/* setze Parteitarnung, damit nicht sofort klar ist, wer dahinter
	 * steckt */
	fset(target, FL_PARTEITARNUNG);

	sprintf(buf, "%s gelingt es %s zu verzaubern. %s wird für etwa %d "
			"Wochen unseren Befehlen gehorchen.", unitname(mage),
			unitname(target), unitname(target), duration);
	addmessage(0, mage->faction, buf, MSG_MAGIC, ML_INFO);

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:           Gesang des wachen Geistes
 * Stufe:          10
 * Gebiet:         Cerddor
 * Kosten:         SPC_LEVEL
 * Wirkung:
 *  Bringt einmaligen Bonus von +15% auf Magieresistenz.  Wirkt auf alle
 *  Aliierten (HELFE BEWACHE) in der Region.
 *  Dauert Stufe Wochen an, ist nicht kumulativ.
 * Flag:
 *   (FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE)
 */
static int
sp_song_resistmagic(castorder *co)
{
	int mr_bonus = 15;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;


	create_curse(mage, &r->attribs, C_SONG_GOODMR, 0,
			force, force, mr_bonus, 0);

	/* Erfolg melden */
	add_message(&mage->faction->msgs, new_message(mage->faction,
				"regionmagic_effect%u:unit%r:region%s:command%u:unit", mage,
				mage->region, strdup(co->order), mage));

	return cast_level;
}
/* ------------------------------------------------------------- */
/* Name:           Gesang des schwachen Geistes
 * Stufe:          12
 * Gebiet:         Cerddor
 * Wirkung:
 *  Bringt einmaligen Malus von -15% auf Magieresistenz.
 *  Wirkt auf alle Nicht-Aliierten (HELFE BEWACHE) in der Region.
 *  Dauert Stufe Wochen an, ist nicht kumulativ.
 * Flag:
 *   (FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE)
 */
static int
sp_song_susceptmagic(castorder *co)
{
	int mr_malus = 15;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;

	create_curse(mage, &r->attribs, C_SONG_BADMR, 0,
			force, force, mr_malus, 0);

	add_message(&mage->faction->msgs, new_message(mage->faction,
				"regionmagic_effect%u:unit%r:region%s:command%u:unit", mage,
				mage->region, strdup(co->order), mage));

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:           Aufruhr beschwichtigen
 * Stufe:          15
 * Gebiet:         Cerddor
 * Flag:           FARCASTING
 * Wirkung:
 *   zerstreut einen Monsterbauernmob, Antimagie zu 'Aufruhr
 *   verursachen'
 */

static int
sp_rallypeasantmob(castorder *co)
{
	unit *u, *un;
	int erfolg = 0;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	/* TODO
	remove_allcurse(&r->attribs, C_RIOT, 0);
	*/

	for (u = r->units; u; u = un){
		un = u->next;
		if (u->faction->no == MONSTER_FACTION && u->race == new_race[RC_PEASANT]){
			rsetpeasants(r, rpeasants(r) + u->number);
			rsetmoney(r, rmoney(r) + get_money(u));
			set_money(u, 0);
			setguard(u, GUARD_NONE);
			set_number(u, 0);
			erfolg = cast_level;
		}
	}

	if (erfolg){
		for (u = r->units; u; u = u->next) freset(u->faction, FL_DH);
		for(u = r->units; u; u = u->next ) {
			if (!fval(u->faction, FL_DH) ) {
				fset(u->faction, FL_DH);
				sprintf(buf, "%s besänftigt den Bauernaufstand in %s.",
						cansee(u->faction, r, mage, 0) ? unitname(mage) : "Jemand",
						regionid(r));
				addmessage(r, u->faction, buf, MSG_MAGIC, ML_INFO);
			}
		}
		if (!fval(mage->faction, FL_DH)){
			sprintf(buf, "%s besänftigt den Bauernaufstand in %s.",
					unitname(mage), regionid(r));
			addmessage(r, u->faction, buf, MSG_MAGIC, ML_INFO);
		}
	} else {
		sprintf(buf, "Der Bauernaufstand in %s hatte sich bereits verlaufen.",
				regionid(r));
		addmessage(r, u->faction, buf, MSG_MAGIC, ML_INFO);
	}
	return erfolg;
}

/* ------------------------------------------------------------- */
/* Name:           Aufruhr verursachen
 * Stufe:          16
 * Gebiet:         Cerddor
 * Wirkung:
 *	Wiegelt 60% bis 90% der Bauern einer Region auf.  Bauern werden ein
 *	großer Mob, der zur Monsterpartei gehört und die Region bewacht.
 *	Regionssilber sollte auch nicht durch Unterhaltung gewonnen werden
 *	können.
 *
 * 	Fehlt: Triggeraktion: löste Bauernmob auf und gib alles an Region,
 * 	dann können die Bauernmobs ihr Silber mitnehmen und bleiben lovar(8)
 * 	Wochen bestehen
 *
 *  alternativ: Lösen sich langsam wieder auf
 * Flag:
 *   (FARCASTING | REGIONSPELL | TESTRESISTANCE)
 */
static int
sp_raisepeasantmob(castorder *co)
{
	unit *u;
	attrib *a;
	int n;
	int anteil = 6;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	anteil += rand()%4;

	n = rpeasants(r) * anteil / 10;
	n = max(0, n);
	n = min(n, rpeasants(r));

	if(n <= 0){
		report_failure(mage, co->order);
		return 0;
	}

	rsetpeasants(r, rpeasants(r) - n);
	assert(rpeasants(r) >= 0);

	u = createunit(r, findfaction(MONSTER_FACTION), n, new_race[RC_PEASANT]);
	guard(u, GUARD_ALL);
	a = a_new(&at_unitdissolve);
	a->data.ca[0] = 1;	/* An rpeasants(r). */
	a->data.ca[1] = 15;	/* 15% */
	a_add(&u->attribs, a);

	create_curse(mage, &r->attribs, C_RIOT, 0, cast_level, anteil, 0, 0);

	for (u = r->units; u; u = u->next) freset(u->faction, FL_DH);
	for (u = r->units; u; u = u->next ) {
		if (!fval(u->faction, FL_DH) ) {
			fset(u->faction, FL_DH);
			add_message(&u->faction->msgs, new_message(u->faction,
				"sp_raisepeasantmob_effect%u:mage%r:region",
				cansee(u->faction, r, mage, 0) ? mage : NULL, r ));
		}
	}
	if (!fval(mage->faction, FL_DH)){
			add_message(&mage->faction->msgs, new_message(mage->faction,
				"sp_raisepeasantmob_effect%u:mage%r:region", mage, r));
	}

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:           Ritual der Aufnahme / Migrantenwerben
 * Stufe:          9
 * Gebiet:         Cerddor
 * Wirkung:
 *	Bis zu Stufe Personen fremder Rasse können angeworben werden. Die
 *	angeworbene Einheit muss kontaktieren. Keine teuren Talente
 *
 * Flag:
 *  (UNITSPELL | SPELLLEVEL | ONETARGET | TESTCANSEE)
 */
static int
sp_migranten(castorder *co)
{
	unit *target;
	strlist *S;
	int kontaktiert = 0;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	spellparameter *pa = co->par;
	spell *sp = co->sp;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	target = pa->param[0]->data.u; /* Zieleinheit */

	/* Personen unserer Rasse können problemlos normal übergeben werden */
	if (target->race == mage->faction->race){
		/* u ist von unserer Art, das Ritual wäre verschwendete Aura. */
		add_message(&mage->faction->msgs, new_message(mage->faction,
				"sp_migranten_fail1%u:unit%r:region%s:command%u:target", mage,
				mage->region, strdup(co->order), target));
	}
	/* Auf eigene Einheiten versucht zu zaubern? Garantiert Tippfehler */
	if (target->faction == mage->faction){
		cmistake(mage, strdup(co->order), 45, MSG_MAGIC);
	}

	/* Keine Monstereinheiten */
	if (!playerrace(target->race)){
		sprintf(buf, "%s kann nicht auf Monster gezaubert werden.", sp->name);
		addmessage(0, mage->faction, buf, MSG_EVENT, ML_WARN);
		return 0;
	}
	/* niemand mit teurem Talent */
	if (teure_talente(target)) {
		sprintf(buf, "%s hat unaufkündbare Bindungen an seine alte Partei.",
				unitname(target));
		addmessage(0, mage->faction, buf, MSG_EVENT, ML_WARN);
		return 0;
	}
	/* maximal Stufe Personen */
	if (target->number > cast_level
			|| target->number > max_spellpoints(r, mage))
	{
		sprintf(buf, "%s in %s: 'ZAUBER \"%s\"': So viele Personen übersteigen "
				"meine Kräfte.", unitname(mage), regionid(mage->region),
				sp->name);
		addmessage(0, mage->faction, buf, MSG_MAGIC, ML_WARN);
	}

	/* Kontakt prüfen (aus alter Teleportroutine übernommen) */
	{
		/* Nun kommt etwas reichlich krankes, um den
		 * KONTAKTIERE-Befehl des Ziels zu überprüfen. */

		for (S = target->orders; S; S = S->next) {
			if (strncasecmp("KON", S->s, 3) == 0) {
				char *c;
				int kontakt = -1;
				/* So weit, so gut. S->s ist also ein KONTAKTIERE. Nun gilt es,
				 * herauszufinden, wer kontaktiert wird. Das ist nicht trivial.
				 * Zuerst muß der Parameter herausoperiert werden. */
				/* Leerzeichen finden */

				for (c = S->s; *c != 0; c++) {
					if (isspace((int)*c) != 0) {
						break;
					}
				}

				/* Wenn ein Leerzeichen da ist, ist *c != 0 und zeigt auf das
				 * Leerzeichen. */

				if (*c == 0) {
					continue;
				}
				kontakt = atoi36(c);

				if (kontakt == mage->no) {
					kontaktiert = 1;
					break;
				}
			}
		}
	}

	if (kontaktiert == 0){
		sprintf(buf, "%s in %s: 'ZAUBER \"%s\"': Die Einheit %s hat keinen "
				"Kontakt mit uns aufgenommen.", unitname(mage),
				regionid(mage->region), sp->name, unitname(target));
		addmessage(0, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
		return 0;
	}
	u_setfaction(target,mage->faction);
	set_string(&target->thisorder, "");

	/* Erfolg melden */
	add_message(&mage->faction->msgs, new_message(mage->faction,
				"sp_migranten%u:unit%r:region%s:command%u:target", mage,
				mage->region, strdup(co->order), target));

	return target->number;
}

/* ------------------------------------------------------------- */
/* Name:           Gesang der Friedfertigkeit
 * Stufe:          12
 * Gebiet:         Cerddor
 * Wirkung:
 *         verhindert jede Attacke für lovar(Stufe/2) Runden
 */

static int
sp_song_of_peace(castorder *co)
{
	unit *u;
	int duration;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;

	if(force < 2)
		duration = 0;
	else
		duration = lovar(force/2);

	create_curse(mage,&r->attribs,C_PEACE,0,force,duration,1,0);

	for (u = r->units; u; u = u->next) freset(u->faction, FL_DH);
	for (u = r->units; u; u = u->next ) {
		if (!fval(u->faction, FL_DH) ) {
			fset(u->faction, FL_DH);
			if (cansee(u->faction, r, mage, 0)){
				sprintf(buf, "%s's Gesangskunst begeistert die Leute. Die "
						"friedfertige Stimmung des Lieds überträgt sich auf alle "
						"Zuhörer. Einige werfen ihre Waffen weg.", unitname(mage));
			}else{
				sprintf(buf, "In der Luft liegt ein wunderschönes Lied, dessen "
						"friedfertiger Stimmung sich niemand entziehen kann. "
						"Einige Leute werfen sogar ihre Waffen weg.");
			}
			addmessage(r, u->faction, buf, MSG_EVENT, ML_INFO);
		}
	}
	return cast_level;

}

/* ------------------------------------------------------------- */
/* Name:           Hohes Lied der Gaukelei
 * Stufe:          2
 * Gebiet:         Cerddor
 * Wirkung:
 *         Das Unterhaltungsmaximum steigt von 20% auf 40% des
 *         Regionsvermögens. Der Spruch hält Stufe Wochen an
 */

static int
sp_generous(castorder *co)
{
	unit *u;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;

	if(is_cursed(r->attribs, C_DEPRESSION, 0)){
		sprintf(buf, "%s in %s: Die Stimmung in %s ist so schlecht, das "
				"niemand auf den Zauber reagiert.", unitname(mage),
				regionid(mage->region), regionid(r));
		addmessage(0, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
		return 0;
	}
	create_curse(mage,&r->attribs,C_GENEROUS,0,force,force,2,0);

	for (u = r->units; u; u = u->next) freset(u->faction, FL_DH);
	for (u = r->units; u; u = u->next ) {
		if (!fval(u->faction, FL_DH) ) {
			fset(u->faction, FL_DH);
			if (cansee(u->faction, r, mage, 0)){
				sprintf(buf, "%s's Gesangskunst begeistert die Leute. Die "
						"fröhliche und ausgelassene Stimmung der Lieder überträgt "
						"sich auf alle Zuhörer.", unitname(mage));
			}else{
				sprintf(buf, "Die Darbietungen eines fahrenden Gauklers begeistern "
						"die Leute. Die fröhliche und ausgelassene Stimmung seiner "
						"Lieder überträgt sich auf alle Zuhörer.");
			}
			addmessage(r, u->faction, buf, MSG_EVENT, ML_INFO);
		}
	}
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:           Anwerbung
 * Stufe:          4
 * Gebiet:         Cerddor
 * Wirkung:
 *         Bauern schliessen sich der eigenen Partei an
 *         ist zusätzlich zur Rekrutierungsmenge in der Region
 * */

static int
sp_recruit(castorder *co)
{
	unit *u;
	faction *f;
	int n;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;

	f = mage->faction;

	if (rpeasants(r) == 0){
		report_failure(mage, co->order);
		return 0;
	}
	/* Immer noch zuviel auf niedrigen Stufen. Deshalb die Rekrutierungskosten
	 * mit einfliessen lassen und dafür den Exponenten etwas größer. */
	n = (int)((pow(force, 1.6) * 50)/f->race->recruitcost);
	n = min(rpeasants(r),n);
	n = max(n, 1);

	if(n <= 0){
		report_failure(mage, co->order);
		return 0;
	}

	rsetpeasants(r, rpeasants(r) - n);
	u = create_unit(r, f, n, f->race, 0, (n == 1 ? "Bauer" : "Bauern"), mage);
	set_string(&u->thisorder, locale_string(u->faction->locale, "defaultorder"));

	sprintf(buf, "%s konnte %d %s anwerben", unitname(mage), n,
			n == 1 ? "Bauer" : "Bauern");
	addmessage(r, mage->faction, buf, MSG_MAGIC, ML_INFO);
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:           Aushorchen
 * Stufe:          7
 * Gebiet:         Cerddor
 * Wirkung:
 *  Erliegt die Einheit dem Zauber, so wird sie dem Magier alles
 *  erzählen, was sie über die gefragte Region weiß. Ist in der Region
 *  niemand ihrer Partei, so weiß sie nichts zu berichten.  Auch kann
 *  sie nur das erzählen, was sie selber sehen könnte.
 * Flags:
 *   (UNITSPELL | ONETARGET | TESTCANSEE)
 */

/* restistenz der einheit prüfen */
static int
sp_pump(castorder *co)
{
	unit *u, *target;
	region *rt;
	boolean see = false;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	spellparameter *pa = co->par;
	int cast_level = co->level;
	spell *sp = co->sp;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	target = pa->param[0]->data.u; /* Zieleinheit */

	if (fval(target->race, RCF_UNDEAD)) {
		sprintf(buf, "%s kann nicht auf Untote gezaubert werden.", sp->name);
		addmessage(0, mage->faction, buf, MSG_EVENT, ML_WARN);
		return 0;
	}
	if (is_magic_resistant(mage, target, 0) || target->faction->no == MONSTER_FACTION) {
		report_failure(mage, co->order);
		return 0;
	}

	rt  = pa->param[1]->data.r;

	for (u = rt->units; u; u = u->next){
		if(u->faction == target->faction)
			see = true;
	}

	if (see == false){
		sprintf(buf, "%s horcht %s über %s aus, aber %s wusste nichts zu "
				"berichten.", unitname(mage), unitname(target), regionid(rt),
				unitname(target));
		addmessage(r, mage->faction, buf, MSG_MAGIC, ML_INFO);
		return cast_level/2;
	} else {
		sprintf(buf, "%s horcht %s über %s aus.", unitname(mage),
				unitname(target), regionid(rt));
		addmessage(r, mage->faction, buf, MSG_MAGIC, ML_INFO);
	}

	u = createunit(rt, mage->faction, RS_FARVISION, new_race[RC_SPELL]);
	set_string(&u->name, "Zauber: Aushorchen");
	u->age = 2;
	set_level(u, SK_OBSERVATION, eff_skill(target, SK_OBSERVATION, u->region));

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:           Verführung
 * Stufe:          6
 * Gebiet:         Cerddor
 * Wirkung:
 *  Betört eine Einheit, so das sie ihm den größten Teil ihres Bargelds
 *  und 50% ihres Besitzes schenkt. Sie behält jedoch immer soviel, wie
 *  sie zum überleben braucht. Wirkt gegen Magieresistenz.
 *  min(Stufe*1000$, u->money - maintenace)
 *  Von jedem Item wird 50% abgerundet ermittelt und übergeben. Dazu
 *  kommt Itemzahl%2 mit 50% chance
 *
 * Flags:
 * (UNITSPELL | ONETARGET | TESTRESISTANCE | TESTCANSEE)
 */
static int
sp_seduce(castorder *co)
{
	unit *target;
	int loot;
	item **itmp;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	spellparameter *pa = co->par;
	int cast_level = co->level;
	spell *sp = co->sp;
	int force = co->force;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	target = pa->param[0]->data.u; /* Zieleinheit */

	if (fval(target->race, RCF_UNDEAD)) {
		sprintf(buf, "%s kann nicht auf Untote gezaubert werden.", sp->name);
		addmessage(0, mage->faction, buf, MSG_MAGIC, ML_WARN);
		return 0;
	}

	/* Erfolgsmeldung */
	sprintf(buf, "%s schenkt %s ", unitname(target), unitname(mage));

	loot = min(cast_level * 1000, get_money(target) - (MAINTENANCE*target->number));
	loot = max(loot, 0);
	change_money(mage, loot);
	change_money(target, -loot);

	if(loot > 0){
		icat(loot);
	} else {
		scat("kein");
	}
	scat(" Silber");
	itmp=&target->items;
	while(*itmp) {
		item * itm = *itmp;
		loot = itm->number/2;
		if (itm->number % 2) {
			loot += rand() % 2;
		}
		if (loot > 0) {
			loot = min(loot, force * 5);
			scat(", ");
			icat(loot);
			scat(" ");
			scat(locale_string(mage->faction->locale, resourcename(itm->type->rtype, (loot==1)?0:GR_PLURAL)));
			i_change(&mage->items, itm->type, loot);
			if (loot!=itm->number) itm->number-=loot;
			else i_free(i_remove(itmp, itm));
		}
		if (*itmp==itm) itmp=&itm->next;
	}
	scat(".");
	addmessage(r, mage->faction, buf, MSG_MAGIC, ML_INFO);

	sprintf(buf, "%s verfiel dem Glücksspiel und hat fast sein ganzes Hab "
			"und Gut verspielt.", unitname(target));
	addmessage(r, target->faction, buf, MSG_EVENT, ML_WARN);

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:           Miriams flinke Finger
 * Stufe:          11
 * Gebiet:         Cerddor
 * Wirkung:
 * 	Erschafft Artefakt I_RING_OF_NIMBLEFINGER, Ring der flinken Finger. Der
 * 	Ring verzehnfacht die Produktion seines Trägers (wirkt nur auf
 * 	'Handwerker') und erhöht Menge des geklauten Geldes auf 500 Silber
 * 	pro Talentpunkt Unterschied
 */

static int
sp_create_nimblefingerring(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	change_item(mage,I_RING_OF_NIMBLEFINGER,1);
	creation_message(mage, I_RING_OF_NIMBLEFINGER);
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:           Monster friedlich stimmen
 * Stufe:          6
 * Gebiet:         Cerddor
 * Wirkung:
 *  verhindert Angriffe des bezauberten Monsters auf die Partei des
 *  Barden für Stufe Wochen. Nicht übertragbar, dh Verbündete werden vom
 *  Monster natürlich noch angegriffen. Wirkt nicht gegen Untote
 *  Jede Einheit kann maximal unter einem Beherrschungszauber dieser Art
 *  stehen, dh wird auf die selbe Einheit dieser Zauber von einem
 *  anderen Magier nochmal gezaubert, schlägt der Zauber fehl.
 *
 * Flags:
 * (UNITSPELL | ONSHIPCAST | ONETARGET | TESTRESISTANCE | TESTCANSEE)
 */

static int
sp_calm_monster(castorder *co)
{
	int duration;
	unit *target;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	spellparameter *pa = co->par;
	int cast_level = co->level;
	int force = co->force;
	spell *sp = co->sp;

	duration = force;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	target = pa->param[0]->data.u; /* Zieleinheit */

	if (fval(target->race, RCF_UNDEAD)) {
		sprintf(buf, "%s kann nicht auf Untote gezaubert werden.", sp->name);
		addmessage(0, mage->faction, buf, MSG_MAGIC, ML_WARN);
		return 0;
	}

	if (!create_curse(mage, &target->attribs, C_CALM, 0, force, duration,
				mage->faction->unique_id, 0)){
		report_failure(mage, co->order);
		return 0;
	}
	/* Nur ein Beherrschungszauber pro Unit */
	set_curseflag(target->attribs, C_CALM, 0, CURSE_ONLYONE);

	sprintf(buf, "%s besänftigt %s.", unitname(mage), unitname(target));
	addmessage(r, mage->faction, buf, MSG_MAGIC, ML_INFO);
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:           schaler Wein
 * Stufe:          7
 * Gebiet:         Cerddor
 * Wirkung:
 *  wird gegen Magieresistenz gezaubert Das Opfer vergisst bis zu
 *  Talenttage seines höchsten Talentes und tut die Woche nix.
 *  Nachfolgende Zauber sind erschwert.
 *  Wirkt auf bis zu 10 Personen in der Einheit
 *
 * Flags:
 * (UNITSPELL | ONETARGET | TESTRESISTANCE | TESTCANSEE)
 */

static int
sp_headache(castorder *co)
{
#if SKILLPOINTS
	skill_t sk = 0;
	skill_t i;
	int sk_val = 0;
	int days;
#else
	skill * smax = NULL;
	int i;
#endif
	unit *target;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	spellparameter *pa = co->par;
	int cast_level = co->level;

	/* Macht alle nachfolgenden Zauber doppelt so teuer */
	countspells(mage, 1);

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	target = pa->param[0]->data.u; /* Zieleinheit */

#if SKILLPOINTS
	/* finde das größte Talent: */
	for (i=0;i<MAXSKILLS;i++){
		int l = get_level(target, i);
		if (sk_val < l) {
			sk = i;
			sk_val = l;
		}
	}
	/* wirkt auf maximal 10 Personen */
	days = min(10, target->number) * lovar(60);
	change_skill(target, sk, -days);
#else
	/* finde das größte Talent: */
	for (i=0;i!=target->skill_size;++i) {
		skill * sv = target->skills+i;
		if (smax==NULL || skill_compare(sv, smax)>0) {
			smax = sv;
		}
	}
	if (smax!=NULL) {
		/* wirkt auf maximal 10 Personen */
		int change = min(10, target->number) * (rand()%2+1) / target->number;
		reduce_skill(target, smax, change);
	}
#endif
	set_string(&target->thisorder, "");

	sprintf(buf, "%s verschafft %s einige feuchtfröhliche Stunden mit heftigen "
		"Nachwirkungen.", unitname(mage), unitname(target));
	addmessage(mage->region, mage->faction, buf, MSG_MAGIC, ML_INFO);

	sprintf(buf, "%s hat höllische Kopfschmerzen und kann sich an die "
		"vergangene Woche nicht mehr erinnern. Nur noch daran, wie alles mit "
		"einer fröhlichen Feier in irgendeiner Taverne anfing...", unitname(target));
	addmessage(r, target->faction, buf, MSG_EVENT, ML_WARN);

	return cast_level;
}


/* ------------------------------------------------------------- */
/* Name:           Mob
 * Stufe:          10
 * Gebiet:         Cerddor
 * Wirkung:
 *   Wiegelt Stufe*250 Bauern zu einem Mob auf, der sich der Partei des
 *   Magier anschliesst Pro Woche beruhigen sich etwa 15% wieder und
 *   kehren auf ihre Felder zurück
 *
 * Flags:
 * (SPELLLEVEL | REGIONSPELL | TESTRESISTANCE)
 */
static int
sp_raisepeasants(castorder *co)
{
	int bauern;
	unit *u, *u2;
	attrib *a;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;

	if(rpeasants(r) == 0) {
		addmessage(r, mage->faction, "Hier gibt es keine Bauern.",
				MSG_MAGIC, ML_MISTAKE);
		return 0;
	}
	bauern = min(rpeasants(r),power*250);
	rsetpeasants(r, rpeasants(r) - bauern);

	u2 = create_unit(r,mage->faction, bauern, new_race[RC_PEASANT], 0,"Wilder Bauernmob",mage);

	fset(u2, FL_LOCKED);
	fset(u2, FL_PARTEITARNUNG);

	a = a_new(&at_unitdissolve);
	a->data.ca[0] = 1;	/* An rpeasants(r). */
	a->data.ca[1] = 15;	/* 15% */
	a_add(&u2->attribs, a);

	for (u = r->units; u; u = u->next) freset(u->faction, FL_DH);
	for (u = r->units; u; u = u->next ) {
		if (!fval(u->faction, FL_DH) ) {
			fset(u->faction, FL_DH);
			sprintf(buf, "%s wiegelt %d Bauern auf.",
					cansee(u->faction, r, mage, 0) ? unitname(mage) : "Jemand",
					u2->number);
			addmessage(r, u->faction, buf, MSG_MAGIC, ML_INFO);
		}
	}
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Trübsal
 * Stufe:      11
 * Kategorie:  Region, negativ
 * Wirkung:
 *  in der Region kann für einige Wochen durch Unterhaltung kein Geld
 *  mehr verdient werden
 *
 * Flag:
 * (FARCASTING | REGIONSPELL | TESTRESISTANCE)
 */
static int
sp_depression(castorder *co)
{
	unit *u;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;

	create_curse(mage,&r->attribs,C_DEPRESSION,0,power,power,0,0);

	for (u = r->units; u; u = u->next) freset(u->faction, FL_DH);
	for (u = r->units; u; u = u->next ) {
		if (!fval(u->faction, FL_DH) ) {
			fset(u->faction, FL_DH);
			sprintf(buf, "%s sorgt für Trübsal unter den Bauern.",
					cansee(u->faction, r, mage, 0) ? unitname(mage) : "Jemand");
			addmessage(r, u->faction, buf, MSG_MAGIC, ML_INFO);
		}
	}
	return cast_level;
}

/* ------------------------------------------------------------- */
/* TRAUM - Illaun */
/* ------------------------------------------------------------- */

/* Name:       Seelenfrieden
 * Stufe:      2
 * Kategorie:  Region, positiv
 * Gebiet:     Illaun
 * Wirkung:
 * 	Reduziert Untotencounter
 * Flag: (0)
 */

int
sp_puttorest(castorder *co)
{
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int laid_to_rest = 0;
	laid_to_rest = dice(co->force * 2, 100);
	laid_to_rest = max(laid_to_rest, deathcount(r));

	deathcounts(r, -laid_to_rest);

	report_effect(r, mage,
		msg_message("puttorest", "mage", mage),
		msg_message("puttorest", "mage", NULL));
	return co->level;
}

/* Name:       Traumschlößchen
 * Stufe:      3
 * Kategorie:  Region, Gebäude, positiv
 * Gebiet:     Illaun
 * Wirkung:
 *  Mit Hilfe dieses Zaubers kann der Traumweber die Illusion eines
 *  beliebigen Gebäudes erzeugen. Die Illusion kann betreten werden, ist
 *  aber ansonsten funktionslos und benötigt auch keinen Unterhalt
 * Flag: (0)
 */

int
sp_icastle(castorder *co)
{
	building *b;
	const building_type * type;
	attrib *a;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;
	spellparameter *pa = co->par;
	icastle_data * data;

	if((type=bt_find(pa->param[0]->data.s)) == NOBUILDING) {
		type = &bt_castle;
	}

	b = new_building(&bt_illusion, r, mage->faction->locale);

	/* Größe festlegen. */
	if(type == &bt_illusion) {
		b->size = (rand()%(power*power)+1)*10;
	} else if (b->type->maxsize == -1) {
		b->size = ((rand()%power)+1)*5;
	} else {
		b->size = b->type->maxsize;
	}
	sprintf(buf, "%s %s", LOC(mage->faction->locale, buildingtype(b, 0)), buildingid(b));
	set_string(&b->name, buf);

	/* TODO: Auf timeout und action_destroy umstellen */
	a = a_add(&b->attribs, a_new(&at_icastle));
	data = (icastle_data*)a->data.v;
	data->type = type;
	data->building = b;
	data->time = 2+(rand()%power+1)*(rand()%power+1);

	if(mage->region == r) {
		leave(r, mage);
		mage->building = b;
	}

	add_message(&mage->faction->msgs, new_message(mage->faction,
		"icastle_create%u:unit%r:region%s:command", mage, mage->region,
		strdup(co->order)));

	addmessage(r, 0,
		"Verwundert blicken die Bauern auf ein plötzlich erschienenes Gebäude.",
		MSG_EVENT, ML_INFO);

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:		Gestaltwandlung
 * Stufe:		3
 * Gebiet:  Illaun
 * Wirkung:
 *  Zieleinheit erscheint für (Stufe) Wochen als eine andere Gestalt
 *  (wie bei dämonischer Rassetarnung).
 * Syntax: ZAUBERE "Gestaltwandlung"  <einheit>     <rasse>
 * Flags:
 *  (UNITSPELL | SPELLLEVEL | ONETARGET)
 */

int
sp_illusionary_shapeshift(castorder *co)
{
	unit *u;
	const race * rc;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;
	spellparameter *pa = co->par;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	/* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
	 * abbrechen aber kosten lassen */
	if(pa->param[0]->flag == TARGET_RESISTS) return cast_level;

	u = pa->param[0]->data.u;

	rc = findrace(pa->param[1]->data.s, mage->faction->locale);
	if (rc == NULL) {
		cmistake(mage, strdup(co->order), 202, MSG_MAGIC);
		return 0;
	}

	/* ähnlich wie in laws.c:setealth() */
	if (!playerrace(rc)) {
		sprintf(buf, "%s %s keine %s-Gestalt annehmen.",
			unitname(u),
			u->number > 1 ? "können" : "kann",
			LOC(u->faction->locale, rc_name(rc, 2)));
		addmessage(r, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
		return 0;
	}
	{
		trigger * trestore = trigger_changerace(u, NULL, u->irace);
		add_trigger(&u->attribs, "timer", trigger_timeout(power+2, trestore));
	}
	u->irace = rc;

	sprintf(buf, "%s läßt %s als %s erscheinen.",
		unitname(mage), unitname(u), LOC(u->faction->locale, rc_name(rc, u->number != 1)));
	addmessage(r, mage->faction, buf, MSG_MAGIC, ML_INFO);

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:           Traumdeuten
 * Stufe:          7
 * Kategorie:      Einheit
 *
 * Wirkung:
 *  Wirkt gegen Magieresistenz.  Spioniert die Einheit aus. Gibt alle
 *  Gegenstände, Talente mit Stufe, Zauber und Kampfstatus an.
 *
 *  Magieresistenz hier prüfen, wegen Fehlermeldung
 *
 * Flag:
 * (UNITSPELL | ONETARGET)
 */
int
sp_readmind(castorder *co)
{
	unit *target;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	spellparameter *pa = co->par;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	target = pa->param[0]->data.u;

	if (target->faction == mage->faction){
		/* Die Einheit ist eine der unsrigen */
		cmistake(mage, strdup(co->order), 45, MSG_MAGIC);
	}

	/* Magieresistenz Unit */
	if (target_resists_magic(mage, target, TYP_UNIT, 0)){
		report_failure(mage, co->order);
		/* "Fühlt sich beobachtet"*/
	  add_message(&target->faction->msgs, new_message(target->faction,
						    "stealdetect%u:unit", target));
		return 0;
	}
	spy_message(2, mage, target);

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:           Regionstraum analysieren
 * Stufe:          9
 * Aura:           18
 * Kosten:         SPC_FIX
 * Wirkung:
 *  Zeigt die Verzauberungen eines Objekts an (curse->name,
 *  cures->info). Aus der Differenz Spruchstärke und Curse->vigour
 *  ergibt sich die Chance den Spruch zu identifizieren ((force -
 *  c->vigour)*10 + 100 %).
 */
int
sp_analyseregionsdream(castorder *co)
{
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	magicanalyse_region(r, mage, cast_level);

	return cast_level;
}


/* ------------------------------------------------------------- */
/* Name:           Traumbilder erkennen
 * Stufe:          5
 * Aura:           12
 * Kosten:         SPC_FIX
 * Wirkung:
 *  Zeigt die Verzauberungen eines Objekts an (curse->name,
 *  cures->info). Aus der Differenz Spruchstärke und Curse->vigour
 *  ergibt sich die Chance den Spruch zu identifizieren ((force -
 *  c->vigour)*10 + 100 %).
 */
int
sp_analysedream(castorder *co)
{
	unit *u;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	spellparameter *pa = co->par;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	/* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
	 * abbrechen aber kosten lassen */
	if(pa->param[0]->flag == TARGET_RESISTS) return cast_level;

	u = pa->param[0]->data.u;
	magicanalyse_unit(u, mage, cast_level);

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Schlechte Träume
 * Stufe:      10
 * Kategorie:  Region, negativ
 * Wirkung:
 *   Dieser Zauber ermöglicht es dem Träumer, den Schlaf aller
 *   nichtaliierten Einheiten (HELFE BEWACHE) in der Region so starkzu
 *   stören, das sie 1 Talentstufe in allen Talenten
 *   vorübergehend verlieren. Der Zauber wirkt erst im Folgemonat.
 *
 * Flags:
 * (FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE)
 * */
int
sp_baddreams(castorder *co)
{
	int duration;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;
	region *r = co->rt;

	/* wirkt erst in der Folgerunde, soll mindestens eine Runde wirken,
	 * also duration+2 */
	duration = max(1, power/2); /* Stufe 1 macht sonst mist */
	duration = 2 + rand()%duration;

	/* Nichts machen als ein entsprechendes Attribut in die Region legen. */
	create_curse(mage, &r->attribs, C_GBDREAM, 0, power, duration, -1, 0);
	set_curseflag(r->attribs, C_GBDREAM, 0, CURSE_ISNEW);

	/* Erfolg melden*/
	add_message(&mage->faction->msgs, new_message(mage->faction,
				"regionmagic_effect%u:unit%r:region%s:command%u:unit", mage,
				mage->region, strdup(co->order), mage));

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Schöne Träume
 * Stufe:      8
 * Kategorie:
 * Wirkung:
 *   Dieser Zauber ermöglicht es dem Träumer, den Schlaf aller aliierten
 *   Einheiten in der Region so zu beeinflussen, daß sie für einige Zeit
 *   einen Bonus von 1 Talentstufe in allen Talenten
 *   bekommen. Der Zauber wirkt erst im Folgemonat.
 * Flags:
 * (FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE)
 */
int
sp_gooddreams(castorder *co)
{
	int    duration;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;

	/* wirkt erst in der Folgerunde, soll mindestens eine Runde wirken,
	 * also duration+2 */
	duration = max(1, power/2); /* Stufe 1 macht sonst mist */
	duration = 2 + rand()%duration;
	create_curse(mage, &r->attribs, C_GBDREAM, 0, power, duration, 1, 0);
	set_curseflag(r->attribs, C_GBDREAM, 0, CURSE_ISNEW);

	/* Erfolg melden*/
	add_message(&mage->faction->msgs, new_message(mage->faction,
				"regionmagic_effect%u:unit%r:region%s:command%u:unit", mage,
				mage->region, strdup(co->order), mage));

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       
 * Stufe:      9
 * Kategorie:
 * Wirkung:
 * 	Es wird eine Kloneinheit erzeugt, die nichts kann. Stirbt der
 * 	Magier, wird er mit einer Wahrscheinlichkeit von 90% in den Klon
 * 	transferiert.
 * Flags:
 * (NOTFAMILARCAST)
 */
int
sp_clonecopy(castorder *co)
{
	unit *clone;
	region *r = co->rt;
	region *target_region = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	if (get_clone(mage) != NULL ) {
		cmistake(mage, strdup(co->order), 298, MSG_MAGIC);
		return 0;
	}

	clone = createunit(target_region, mage->faction, 1, new_race[RC_CLONE]);
	if (target_region==mage->region) {
		clone->building = mage->building;
		clone->ship = mage->ship;
	}
	clone->status = ST_FLEE;
	sprintf(buf, "Klon von %s", unitname(mage));
	set_string(&clone->name, buf);
	fset(clone, FL_LOCKED);
	
	create_newclone(mage, clone);

	sprintf(buf, "%s erschafft einen Klon.", unitname(mage));
	addmessage(r, mage->faction, buf, MSG_MAGIC, ML_INFO);

	return cast_level;
}

/* ------------------------------------------------------------- */
int
sp_dreamreading(castorder *co)
{
	unit *u,*u2;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	spellparameter *pa = co->par;
	int power = co->force;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	/* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
	 * abbrechen aber kosten lassen */
	if(pa->param[0]->flag == TARGET_RESISTS) return cast_level;

	u = pa->param[0]->data.u;

	/* Illusionen und Untote abfangen. */
	if (fval(u->race, RCF_UNDEAD|RCF_ILLUSIONARY)) {
		add_message(&mage->faction->msgs, new_message(mage->faction,
			"spellunitnotfound%u:unit%r:region%s:command%s:id",
			mage, mage->region, strdup(co->order), strdup(itoa36(u->no))));
		return 0;
	}

  /* Entfernung */
  if(distance(mage->region, u->region) > power) {
			addmessage(r, mage->faction, "Die Einheit ist zu weit "
					"entfernt.", MSG_MAGIC, ML_MISTAKE);
      return 0;
  }

  u2=createunit(u->region,mage->faction, RS_FARVISION, new_race[RC_SPELL]);
  set_number(u2, 1);
  set_string(&u2->name, "sp_dreamreading");
  u2->age  = 2;   /* Nur für diese Runde. */
  set_level(u2, SK_OBSERVATION, eff_skill(u, SK_OBSERVATION, u2->region));

  sprintf(buf, "%s verliert sich in die Träume von %s und erhält einen "
      "Eindruck von %s.", unitname(mage), unitname(u), regionid(u->region));
  addmessage(r, mage->faction, buf, MSG_EVENT, ML_INFO);
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Wirkt power/2 Runden auf bis zu power^2 Personen
 * mit einer Chance von 5% vermehren sie sich  */
int
sp_sweetdreams(castorder *co)
{
	unit *u;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;
	spellparameter *pa = co->par;
	int men, n;
	int duration = power/2;
	int opfer = power*power;

	/* Schleife über alle angegebenen Einheiten */
	for (n = 0; n < pa->length; n++) {
		/* sollte nie negativ werden */
		if (opfer < 1)
			break;

		if(pa->param[n]->flag == TARGET_RESISTS
				|| pa->param[n]->flag == TARGET_NOTFOUND)
			continue;

		/* Zieleinheit */
		u = pa->param[n]->data.u;

		men = min(opfer, u->number);
		opfer -= men;

		/* Nichts machen als ein entsprechendes Attribut an die Einheit legen. */
		create_curse(mage,&u->attribs,C_ORC,0,power,duration,5,men);
		set_curseflag(u->attribs, C_ORC, 0, CURSE_ISNEW);

		sprintf(buf, "%s verschafft %s ein interessanteres Nachtleben.",
				unitname(mage), unitname(u));
		addmessage(r, mage->faction, buf, MSG_EVENT, ML_INFO);
	}
	return cast_level;
}

/* ------------------------------------------------------------- */
int
sp_create_tacticcrystal(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	change_item(mage,I_TACTICCRYSTAL,1);
	creation_message(mage, I_TACTICCRYSTAL);
	return cast_level;
}

/* ------------------------------------------------------------- */
int
sp_disturbingdreams(castorder *co)
{
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;

	create_curse(mage, &r->attribs, C_BADLEARN, 0, power, power/6, 10, 0);
	set_curseflag(r->attribs, C_BADLEARN, 0, CURSE_ISNEW);

	sprintf(buf, "%s sorgt für schlechten Schlaf in %s.",
			unitname(mage), regionid(r));
	addmessage(0, mage->faction, buf, MSG_EVENT, ML_INFO);
	return cast_level;
}

/* ------------------------------------------------------------- */
int
sp_dream_of_confusion(castorder *co)
{
	unit *u;
	regionlist *rl,*rl2;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;
	int range = (power-14)/2-1;
	int duration = ((power-14)/2)*2;

	rl = all_in_range(r, range);

	for(rl2 = rl; rl2; rl2 = rl2->next) {

		/* Magieresistenz jeder Region prüfen */
		if (target_resists_magic(mage, rl2->region, TYP_REGION, 0)){
			report_failure(mage, co->order);
			continue;
		}

		create_curse(mage, &rl2->region->attribs, C_REGCONF,0,
				power, duration, cast_level*5, 0);
		/* soll der Zauber schon in der Zauberrunde wirken? */
		set_curseflag(rl2->region->attribs, C_REGCONF, 0, CURSE_ISNEW);

		for (u = rl2->region->units; u; u = u->next) freset(u->faction, FL_DH);
		for (u = rl2->region->units; u; u = u->next ) {
			if(!fval(u->faction, FL_DH) ) {
				fset(u->faction, FL_DH);
				sprintf(buf, "%s beschwört einen Schleier der Verwirrung.",
						cansee(u->faction, r, mage, 0) ? unitname(mage) : "Jemand");
				addmessage(rl2->region, u->faction, buf, MSG_EVENT, ML_INFO);
			}
		}
		if(!fval(mage->faction, FL_DH)){
			sprintf(buf, "%s beschwört einen Schleier der Verwirrung.",
					unitname(mage));
  		addmessage(0, mage->faction, buf, MSG_MAGIC, ML_INFO);
		}
	}
	free_regionlist(rl);
	return cast_level;
}

/* ------------------------------------------------------------- */
/* ASTRAL / THEORIE / M_ASTRAL */
/* ------------------------------------------------------------- */
/* Name:           Magie analysieren
 * Stufe:          1
 * Aura:           1
 * Kosten:         SPC_LINEAR
 * Komponenten:
 *
 * Wirkung:
 *  Zeigt die Verzauberungen eines Objekts an (curse->name,
 *  cures->info). Aus der Differenz Spruchstärke und Curse->vigour
 *  ergibt sich die Chance den Spruch zu identifizieren ((force -
 *  c->vigour)*10 + 100 %).
 *
 * Flags:
 *  UNITSPELL, SHIPSPELL, BUILDINGSPELL
 */

int
sp_analysemagic(castorder *co)
{
	int obj;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	spellparameter *pa = co->par;

	/* Objekt ermitteln */
	obj = pa->param[0]->typ;

	switch(obj) {
		case SPP_REGION:
		{
			region *tr = pa->param[0]->data.r;
			magicanalyse_region(tr, mage, cast_level);
			break;
		}
		case SPP_UNIT:
		{
			unit *u;
			u = pa->param[0]->data.u;
			magicanalyse_unit(u, mage, cast_level);
			break;
		}
		case SPP_BUILDING:
		{
			building *b;
			b = pa->param[0]->data.b;
			magicanalyse_building(b, mage, cast_level);
			break;
		}
		case SPP_SHIP:
		{
			ship *sh;
			sh = pa->param[0]->data.sh;
			magicanalyse_ship(sh, mage, cast_level);
			break;
		}
		default:
		/* Fehlerhafter Parameter */
			return 0;
	}

	return cast_level;
}

/* ------------------------------------------------------------- */

int
sp_itemcloak(castorder *co)
{
	unit *target;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;
	spellparameter *pa = co->par;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	/* Zieleinheit */
	target = pa->param[0]->data.u;

	create_curse(mage,&target->attribs,C_ITEMCLOAK,0,power,power,0,0);
	add_message(&mage->faction->msgs, new_message(mage->faction,
		"itemcloak%u:mage%u:target", mage, target));

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:           Magieresistenz erhöhen
 * Stufe:          3
 * Aura:           5 MP
 * Kosten:         SPC_LEVEL
 * Komponenten:
 *
 * Wirkung:
 * 	erhöht die Magierestistenz der Personen um 20 Punkte für 6 Wochen
 * 	Wirkt auf Stufe*5 Personen kann auf mehrere Einheiten gezaubert
 * 	werden, bis die Zahl der möglichen Personen erschöpft ist
 *
 * Flags:
 * 	UNITSPELL
 */
int
sp_resist_magic_bonus(castorder *co)
{
	unit *u;
	int n, m, opfer;
	int resistbonus = 20;
	int duration = 6;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;
	spellparameter *pa = co->par;

	/* Pro Stufe können bis zu 5 Personen verzaubert werden */
	opfer = power * 5;

	/* Schleife über alle angegebenen Einheiten */
	for (n = 0; n < pa->length; n++) {
		/* sollte nie negativ werden */
		if (opfer < 1)
			break;

		if(pa->param[n]->flag == TARGET_RESISTS
				|| pa->param[n]->flag == TARGET_NOTFOUND)
			continue;

		u = pa->param[n]->data.u;

		/* Ist die Einheit schon verzaubert, wirkt sich dies nur auf die
		 * Menge der Verzauberten Personen aus.
		if(is_cursed(u->attribs, C_MAGICRESISTANCE, 0))
			continue;
		*/

		m = min(u->number,opfer);
		opfer -= m;

		create_curse(mage, &u->attribs, C_MAGICRESISTANCE, 0, power, duration,
				resistbonus, m);

		sprintf(buf, "%s wird kurz von einem magischen Licht umhüllt.",
				unitname(u));
		addmessage(0, u->faction, buf, MSG_EVENT, ML_IMPORTANT);

		/* und noch einmal dem Magier melden */
		if (u->faction != mage->faction)
			addmessage(mage->region, mage->faction, buf, MSG_MAGIC, ML_INFO);
	}
	/* pro 5 nicht verzauberte Personen kann der Level und damit die
	 * Kosten des Zaubers um 1 reduziert werden. (die Formel geht von
	 * immer abrunden da int aus) */
	cast_level -= opfer/5;

	return cast_level;
}

/* ------------------------------------------------------------- */
/* 		"ZAUBERE [STUFE n]  \"Astraler Weg\" [<Ziel-X> <Ziel-Y>]
 * 		<Einheit-Nr> [<Einheit-Nr> ...]",
 *
 * Parameter:
 * pa->param[0]->data.s
*/
int
sp_enterastral(castorder *co)
{
	region *rt, *ro;
	unit *u, *u2;
	int remaining_cap;
	int n, w;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;
	spellparameter *pa = co->par;
	spell *sp = co->sp;

	switch(getplaneid(r)) {
	case 0:
		rt = r_standard_to_astral(r);
		ro = r;
		break;
	default:
		sprintf(buf, "%s in %s: 'ZAUBER \"%s\"': Dieser Zauber funktioniert "
				"nur in der materiellen Welt.", unitname(mage),
				regionid(mage->region),sp->name);
		addmessage(r, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
		return 0;
	}

	if(!rt) {
		sprintf(buf, "%s in %s: 'ZAUBER \"%s\"': Es kann hier kein Kontakt zur "
				"Astralwelt hergestellt werden.", unitname(mage),
				regionid(mage->region),sp->name);
		addmessage(r, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
		return 0;
	}

	if(is_cursed(rt->attribs, C_ASTRALBLOCK, 0) ||
			is_cursed(ro->attribs, C_ASTRALBLOCK, 0)) {
		sprintf(buf, "%s in %s: 'ZAUBER \"%s\"': Es kann kein Kontakt zu "
				"dieser astralen Region hergestellt werden.", unitname(mage),
				regionid(mage->region),sp->name);
		addmessage(r, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
		return 0;
	}

	remaining_cap = (power-3) * 1500;

	/* für jede Einheit in der Kommandozeile */
	for (n = 0; n < pa->length; n++) {
		if(pa->param[n]->flag == TARGET_NOTFOUND) continue;
		u = pa->param[n]->data.u;

		if(!ucontact(u, mage)) {
			if(power > 10 && !is_magic_resistant(mage, u, 0)
					&& can_survive(u, rt)) {
				sprintf(buf, "%s hat uns nicht kontaktiert, widersteht dem "
						"Zauber jedoch nicht.", unitname(u));
				addmessage(r, mage->faction, buf, MSG_MAGIC, ML_INFO);
				sprintf(buf, "%s wird von %s in eine andere Welt geschleudert.",
					unitname(u),
					cansee(u->faction, r, mage, 0)?unitname(mage):"jemandem");
				addmessage(r, u->faction, buf, MSG_MAGIC, ML_WARN);
			} else {
				sprintf(buf, "%s hat uns nicht kontaktiert und widersteht dem "
						"Zauber.", unitname(u));
				addmessage(r, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
				sprintf(buf, "%s versucht, %s in eine andere Welt zu schleudern.",
					cansee(u->faction, r, mage, 0)?unitname(mage):"Jemand",
					unitname(u));
				addmessage(r, u->faction, buf, MSG_EVENT, ML_WARN);
				continue;
			}
		}

		w = weight(u);
		if(!can_survive(u, rt)) {
			cmistake(mage, strdup(co->order), 231, MSG_MAGIC);
		} else if(remaining_cap - w < 0) {
			addmessage(r, mage->faction, "Die Einheit ist zu schwer.",
					MSG_MAGIC, ML_MISTAKE);
		} else {
			remaining_cap = remaining_cap - w;
			move_unit(u, rt, NULL);

			/* Meldungen in der Ausgangsregion */

			for (u2 = r->units; u2; u2 = u2->next) freset(u2->faction, FL_DH);
			for(u2 = r->units; u2; u2 = u2->next ) {
				if(!fval(u2->faction, FL_DH)) {
					fset(u2->faction, FL_DH);
					if(cansee(u2->faction, r, u, 0)) {
						sprintf(buf, "%s wird durchscheinend und verschwindet.",
							unitname(u));
						addmessage(r, u2->faction, buf, MSG_EVENT, ML_INFO);
					}
				}
			}

			/* Meldungen in der Zielregion */

			for (u2 = rt->units; u2; u2 = u2->next) freset(u2->faction, FL_DH);
			for (u2 = rt->units; u2; u2 = u2->next ) {
				if(!fval(u2->faction, FL_DH)) {
					fset(u2->faction, FL_DH);
					if(cansee(u2->faction, rt, u, 0)) {
						sprintf(buf, "%s erscheint plötzlich.", unitname(u));
						addmessage(rt, u2->faction, buf, MSG_EVENT, ML_INFO);
					}
				}
			}
		}
	}
	return cast_level;
}

int
sp_pullastral(castorder *co)
{
	region *rt, *ro;
	unit *u, *u2;
	regionlist *rl, *rl2;
	int remaining_cap;
	int n, w;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;
	spellparameter *pa = co->par;
	spell *sp = co->sp;

	switch(getplaneid(r)) {
	case 1:
		rt = r;
		ro = pa->param[0]->data.r;
		rl = all_in_range(r_astral_to_standard(r),TP_RADIUS);
		rl2 = rl;
		while(rl2) {
			if(rl2->region->x == ro->x && rl2->region->y == ro->y) {
				ro = rl2->region;
				break;
			}
			rl2 = rl2->next;
		}
		if(!rl2) {
			sprintf(buf, "%s in %s: 'ZAUBER \"%s\"': Es kann kein Kontakt zu "
				"dieser Region hergestellt werden.", unitname(mage),
				regionid(mage->region),sp->name);
			addmessage(r, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
			free_regionlist(rl);
			return 0;
		}
		free_regionlist(rl);
		break;
	default:
		sprintf(buf, "%s in %s: 'ZAUBER \"%s\"': Dieser Zauber funktioniert "
				"nur in der astralen  Welt.", unitname(mage),
				regionid(mage->region),sp->name);
		addmessage(r, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
		return 0;
	}

	if(is_cursed(rt->attribs, C_ASTRALBLOCK, 0) ||
			is_cursed(ro->attribs, C_ASTRALBLOCK, 0)) {
		sprintf(buf, "%s in %s: 'ZAUBER \"%s\"': Es kann kein Kontakt zu "
				"dieser Region hergestellt werden.", unitname(mage),
				regionid(mage->region),sp->name);
		addmessage(r, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
		return 0;
	}

	remaining_cap = (power-3) * 1500;

	/* für jede Einheit in der Kommandozeile */
	for (n = 1; n < pa->length; n++) {
		if(pa->param[n]->flag == TARGET_NOTFOUND) continue;

		u = pa->param[n]->data.u;

		if(!ucontact(u, mage)) {
			if(power > 12 && pa->param[n]->flag == TARGET_RESISTS && can_survive(u, rt)) {
				sprintf(buf, "%s hat uns nicht kontaktiert, widersteht dem "
						"Zauber jedoch nicht.", unitname(u));
				addmessage(r, mage->faction, buf, MSG_MAGIC, ML_INFO);
				sprintf(buf, "%s wird von %s in eine andere Welt geschleudert.",
					unitname(u),
					cansee(u->faction, r, mage, 0)?unitname(mage):"jemandem");
				addmessage(r, u->faction, buf, MSG_MAGIC, ML_WARN);
			} else {
				sprintf(buf, "%s hat uns nicht kontaktiert und widersteht dem "
						"Zauber.", unitname(u));
				addmessage(r, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
				sprintf(buf, "%s versucht, %s in eine andere Welt zu schleudern.",
					cansee(u->faction, r, mage, 0)?unitname(mage):"Jemand",
					unitname(u));
				addmessage(r, u->faction, buf, MSG_EVENT, ML_WARN);
				continue;
			}
		}

		w = weight(u);

		if(!can_survive(u, rt)) {
			cmistake(mage, strdup(co->order), 231, MSG_MAGIC);
		} else if(remaining_cap - w < 0) {
			addmessage(r, mage->faction, "Die Einheit ist zu schwer.",
					MSG_MAGIC, ML_MISTAKE);
		} else {
			remaining_cap = remaining_cap - w;
			move_unit(u, rt, NULL);

			/* Meldungen in der Ausgangsregion */

			for (u2 = r->units; u2; u2 = u2->next) freset(u2->faction, FL_DH);
			for(u2 = r->units; u2; u2 = u2->next ) {
				if(!fval(u2->faction, FL_DH)) {
					fset(u2->faction, FL_DH);
					if(cansee(u2->faction, r, u, 0)) {
						sprintf(buf, "%s wird durchscheinend und verschwindet.",
							unitname(u));
						addmessage(r, u2->faction, buf, MSG_EVENT, ML_INFO);
					}
				}
			}

			/* Meldungen in der Zielregion */

			for (u2 = rt->units; u2; u2 = u2->next) freset(u2->faction, FL_DH);
			for(u2 = rt->units; u2; u2 = u2->next ) {
				if(!fval(u2->faction, FL_DH)) {
					fset(u2->faction, FL_DH);
					if(cansee(u2->faction, rt, u, 0)) {
						sprintf(buf, "%s erscheint plötzlich.", unitname(u));
						addmessage(rt, u2->faction, buf, MSG_EVENT, ML_INFO);
					}
				}
			}
		}
	}
	return cast_level;
}


int
sp_leaveastral(castorder *co)
{
	region *rt, *ro;
	regionlist *rl, *rl2;
	unit *u, *u2;
	int remaining_cap;
	int n, w;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;
	spellparameter *pa = co->par;

	switch(getplaneid(r)) {
	case 1:
		ro = r;
		rt = pa->param[0]->data.r;
		if(!rt) {
			addmessage(r, mage->faction, "Dorthin führt kein Weg.",
				MSG_MAGIC, ML_MISTAKE);
  	  return 0;
		}
		rl  = allinhab_in_range(r_astral_to_standard(r),TP_RADIUS);
		rl2 = rl;
		while(rl2) {
			if(rl2->region == rt) break;
			rl2 = rl2->next;
		}
		if(!rl2) {
			addmessage(r, mage->faction, "Dorthin führt kein Weg.",
				MSG_MAGIC, ML_MISTAKE);
			free_regionlist(rl);
  	  return 0;
		}
		free_regionlist(rl);
		break;
	default:
		sprintf(buf, "Der Zauber funktioniert nur in der astralen Welt.");
		addmessage(r, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
		return 0;
	}

	if(!ro
			|| is_cursed(ro->attribs, C_ASTRALBLOCK, 0)
			|| is_cursed(rt->attribs, C_ASTRALBLOCK, 0)) {
		sprintf(buf, "Die Wege aus dieser astralen Region sind blockiert.");
		addmessage(r, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
		return 0;
	}

	remaining_cap = (power-3) * 1500;

	/* für jede Einheit in der Kommandozeile */
	for (n = 1; n < pa->length; n++) {
		if(pa->param[n]->flag == TARGET_NOTFOUND) continue;

		u = pa->param[n]->data.u;

		if(!ucontact(u, mage)) {
			if(power > 10 && !pa->param[n]->flag == TARGET_RESISTS && can_survive(u, rt)) {
				sprintf(buf, "%s hat uns nicht kontaktiert, widersteht dem "
					"Zauber jedoch nicht.", unitname(u));
				addmessage(r, mage->faction, buf, MSG_MAGIC, ML_INFO);
				sprintf(buf, "%s wird von %s in eine andere Welt geschleudert.",
					unitname(u),
					cansee(u->faction, r, mage, 0)?unitname(mage):"jemandem");
				addmessage(r, u->faction, buf, MSG_EVENT, ML_WARN);
			} else {
				sprintf(buf, "%s hat uns nicht kontaktiert und widersteht dem "
						"Zauber.", unitname(u));
				addmessage(r, mage->faction, buf, MSG_MAGIC, ML_WARN);
				sprintf(buf, "%s versucht, %s in eine andere Welt zu schleudern.",
					cansee(u->faction, r, mage, 0)?unitname(mage):"Jemand",
					unitname(u));
				addmessage(r, u->faction, buf, MSG_EVENT, ML_WARN);
				continue;
			}
		}

		w = weight(u);

		if(!can_survive(u, rt)) {
			cmistake(mage, strdup(co->order), 231, MSG_MAGIC);
		} else if(remaining_cap - w < 0) {
			addmessage(r, mage->faction, "Die Einheit ist zu schwer.",
					MSG_MAGIC, ML_MISTAKE);
		} else {
			remaining_cap = remaining_cap - w;
			move_unit(u, rt, NULL);

			/* Meldungen in der Ausgangsregion */

			for (u2 = r->units; u2; u2 = u2->next) freset(u2->faction, FL_DH);
			for(u2 = r->units; u2; u2 = u2->next ) {
				if(!fval(u2->faction, FL_DH)) {
					fset(u2->faction, FL_DH);
					if(cansee(u2->faction, r, u, 0)) {
						sprintf(buf, "%s wird durchscheinend und verschwindet.",
							unitname(u));
						addmessage(r, u2->faction, buf, MSG_EVENT, ML_INFO);
					}
				}
			}

			/* Meldungen in der Zielregion */

			for (u2 = rt->units; u2; u2 = u2->next) freset(u2->faction, FL_DH);
			for (u2 = rt->units; u2; u2 = u2->next ) {
				if(!fval(u2->faction, FL_DH)) {
					fset(u2->faction, FL_DH);
					if(cansee(u2->faction, rt, u, 0)) {
						sprintf(buf, "%s erscheint plötzlich.", unitname(u));
						addmessage(rt, u2->faction, buf, MSG_EVENT, ML_INFO);
					}
				}
			}
		}
	}
	return cast_level;
}

int
sp_fetchastral(castorder *co)
{
	region *rt, *ro;
	unit *u, *u2;
	int remaining_cap;
	int n, w;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;
	spellparameter *pa = co->par;

	switch(getplaneid(r)) {
	case 0:
		rt = r;
		ro = r_standard_to_astral(r);
		if(!ro) {
			cmistake(mage, strdup(co->order), 216, MSG_MAGIC);
			return 0;
		}
		break;
	default:
		sprintf(buf, "Der Zauber funktioniert nur in der materiellen Welt.");
		addmessage(r, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
		return 0;
	}

	if(!ro
			|| is_cursed(ro->attribs, C_ASTRALBLOCK, 0)
			|| is_cursed(rt->attribs, C_ASTRALBLOCK, 0)) {
		sprintf(buf, "Die Wege aus dieser astralen Region sind blockiert.");
		addmessage(r, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
		return 0;
	}

	remaining_cap = (power-3) * 1500;

	/* für jede Einheit in der Kommandozeile */
	for (n = 0; n < pa->length; n++) {

		if(pa->param[n]->flag == TARGET_NOTFOUND) continue;

		u = pa->param[n]->data.u;

		if(!ucontact(u, mage)) {
			if(power > 12 && !pa->param[n]->flag == TARGET_RESISTS && can_survive(u, rt)) {
				sprintf(buf, "%s hat uns nicht kontaktiert, widersteht dem "
						"Zauber jedoch nicht.", unitname(u));
				addmessage(r, mage->faction, buf, MSG_MAGIC, ML_INFO);
				sprintf(buf, "%s wird von %s in eine andere Welt geschleudert.",
					unitname(u),
					cansee(u->faction, r, mage, 0)?unitname(mage):"jemandem");
				addmessage(r, u->faction, buf, MSG_EVENT, ML_WARN);
			} else {
				sprintf(buf, "%s hat uns nicht kontaktiert und widersteht dem "
						"Zauber.", unitname(u));
				addmessage(r, mage->faction, buf, MSG_MAGIC, ML_WARN);
				sprintf(buf, "%s versucht, %s in eine andere Welt zu schleudern.",
					cansee(u->faction, r, mage, 0)?unitname(mage):"Jemand",
					unitname(u));
				addmessage(r, u->faction, buf, MSG_EVENT, ML_WARN);
				continue;
			}
		}

		w = weight(u);
		if(!can_survive(u, rt)) {
			cmistake(mage, strdup(co->order), 231, MSG_MAGIC);
		} else if(remaining_cap - w < 0) {
			addmessage(r, mage->faction, "Die Einheit ist zu schwer.",
					MSG_MAGIC, ML_MISTAKE);
		} else {
			remaining_cap = remaining_cap - w;
			move_unit(u, rt, NULL);

			/* Meldungen in der Ausgangsregion */

			for (u2 = r->units; u2; u2 = u2->next) freset(u2->faction, FL_DH);
			for(u2 = r->units; u2; u2 = u2->next ) {
				if(!fval(u2->faction, FL_DH)) {
					fset(u2->faction, FL_DH);
					if(cansee(u2->faction, r, u, 0)) {
						sprintf(buf, "%s wird durchscheinend und verschwindet.",
							unitname(u));
						addmessage(r, u2->faction, buf, MSG_EVENT, ML_INFO);
					}
				}
			}

			/* Meldungen in der Zielregion */

			for (u2 = rt->units; u2; u2 = u2->next) freset(u2->faction, FL_DH);
			for (u2 = rt->units; u2; u2 = u2->next ) {
				if(!fval(u2->faction, FL_DH)) {
					fset(u2->faction, FL_DH);
					if(cansee(u2->faction, rt, u, 0)) {
						sprintf(buf, "%s erscheint plötzlich.", unitname(u));
						addmessage(rt, u2->faction, buf, MSG_EVENT, ML_INFO);
					}
				}
			}
		}
	}
	return cast_level;
}

/* ------------------------------------------------------------- */
int
sp_showastral(castorder *co)
{
	unit *u;
	region *rt;
	int n = 0;
	int c = 0;
	regionlist *rl, *rl2;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;

	switch(getplaneid(r)) {
	case 0:
		rt = r_standard_to_astral(r);
		if(!rt) {
			/* Hier gibt es keine Verbindung zur astralen Welt */
			cmistake(mage, strdup(co->order), 216, MSG_MAGIC);
			return 0;
		}
		break;
	case 1:
		rt = r;
		break;
	default:
		/* Hier gibt es keine Verbindung zur astralen Welt */
		cmistake(mage, strdup(co->order), 216, MSG_MAGIC);
		return 0;
	}

	rl = all_in_range(rt,power/5);

	/* Erst Einheiten zählen, für die Grammatik. */

	for(rl2=rl; rl2; rl2=rl2->next) {
		if(!is_cursed(rl2->region->attribs, C_ASTRALBLOCK, 0)) {
			for(u = rl2->region->units; u; u=u->next) {
				if (u->race != new_race[RC_SPECIAL] && u->race != new_race[RC_SPELL]) n++;
			}
		}
	}

	if(n == 0) {
		/* sprintf(buf, "%s kann niemanden im astralen Nebel entdecken.",
			unitname(mage)); */
		cmistake(mage, strdup(co->order), 220, MSG_MAGIC);
	} else {

		/* Ausgeben */

		sprintf(buf, "%s hat eine Vision der astralen Ebene. Im astralen "
			"Nebel zu erkennen sind ", unitname(mage));

		for(rl2=rl; rl2; rl2=rl2->next) {
			if(!is_cursed(rl2->region->attribs, C_ASTRALBLOCK, 0)) {
				for(u = rl2->region->units; u; u=u->next) {
					if(u->race != new_race[RC_SPECIAL] && u->race != new_race[RC_SPELL]) {
						c++;
						scat(unitname(u));
						scat(" (");
						if(!fval(u, FL_PARTEITARNUNG)) {
							scat(factionname(u->faction));
							scat(", ");
						}
						icat(u->number);
						scat(" ");
						scat(LOC(mage->faction->locale, rc_name(u->race, u->number!=1)));
						scat(", Entfernung ");
						icat(distance(rl2->region, rt));
						scat(")");
						if(c == n-1) {
							scat(" und ");
						} else if(c < n-1) {
							scat(", ");
						}
					}
				}
			}
		}
		scat(".");
		addmessage(r, mage->faction, buf, MSG_MAGIC, ML_INFO);
	}

	free_regionlist(rl);
	return cast_level;
}

/* ------------------------------------------------------------- */
int
sp_viewreality(castorder *co)
{
	regionlist *rl, *rl2;
	unit *u;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	if(getplaneid(r) != 1) {
		/* sprintf(buf, "Dieser Zauber kann nur im Astralraum gezaubert werden."); */
		cmistake(mage, strdup(co->order), 217, MSG_MAGIC);
		return 0;
	}

	if(is_cursed(r->attribs, C_ASTRALBLOCK, 0)) {
		/* sprintf(buf, "Die materielle Welt ist hier nicht sichtbar.");*/
		cmistake(mage, strdup(co->order), 218, MSG_MAGIC);
		return 0;
	}

	rl = all_in_range(r_astral_to_standard(r), TP_RADIUS);

	/* Irgendwann mal auf Curses u/o Attribut umstellen. */
	for(rl2=rl; rl2; rl2=rl2->next) {
		u = createunit(rl2->region, mage->faction, RS_FARVISION, new_race[RC_SPELL]);
		set_string(&u->name, "Zauber: Blick in die Realität");
		u->age = 2;
	}

	free_regionlist(rl);

	sprintf(buf, "%s gelingt es, durch die Nebel auf die Realität zu blicken.",
			unitname(mage));
	addmessage(r, mage->faction, buf, MSG_MAGIC, ML_INFO);
	return cast_level;
}

/* ------------------------------------------------------------- */
int
sp_disruptastral(castorder *co)
{
	regionlist *rl, *rl2;
	region *rt;
	unit *u;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;

	switch(getplaneid(r)) {
	case 0:
		rt = r_standard_to_astral(r);
		if(!rt) {
			/* "Hier gibt es keine Verbindung zur astralen Welt." */
			cmistake(mage, strdup(co->order), 216, MSG_MAGIC);
			return 0;
		}
		break;
	case 1:
		rt = r;
		break;
	default:
		/* "Von hier aus kann man die astrale Ebene nicht erreichen." */
		cmistake(mage, strdup(co->order), 215, MSG_MAGIC);
		return 0;
	}

	rl = all_in_range(rt, power/5);

	for(rl2=rl; rl; rl=rl->next) {
		attrib *a, *a2;
		spec_direction *sd;

		if(is_cursed(rl2->region->attribs, C_ASTRALBLOCK, 0))
			continue;

		/* Nicht-Permanente Tore zerstören */
		a = a_find(r->attribs, &at_direction);

		while(a) {
			a2 = a->nexttype;
			sd = (spec_direction *)(a->data.v);
			if(sd->duration != -1) a_remove(&r->attribs, a);
			a = a2;
		}

		/* Einheiten auswerfen */

		for(u=rl2->region->units;u;u=u->next) {
			if(u->race != new_race[RC_SPELL]) {
				regionlist *trl, *trl2;
				region *tr;
				int c = 0;

				/* Zufällige Zielregion suchen */
				trl = allinhab_in_range(r_astral_to_standard(rl2->region), TP_RADIUS);
				for(trl2 = trl; trl2; trl2 = trl2->next) c++;
				c = rand()%c;
				for(trl2 = trl; trl2 && c != 0; trl2 = trl2->next) c--;
				tr = trl2->region;
				free_regionlist(trl);

				if(!is_magic_resistant(mage, u, 0) && can_survive(u, tr)) {
					move_unit(u, tr, NULL);
					sprintf(buf, "%s wird aus der astralen Ebene nach %s geschleudert.",
						unitname(u), regionid(tr));
					addmessage(0, u->faction, buf, MSG_MAGIC, ML_INFO);
				}
			}
		}

		/* Kontakt unterbinden */
		create_curse(mage,&rl2->region->attribs,C_ASTRALBLOCK,0,power,power/3,
				100, 0);
		addmessage(r, 0, "Mächtige Magie verhindert den Kontakt zur Realität.",
				MSG_COMMENT, ML_IMPORTANT);
	}

	free_regionlist(rl);
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:           Erschaffe einen Beutel des Negativen Gewichts
 * Stufe:          10
 * Gebiet:         Tybied
 * Kategorie:      Artefakt
 * Wirkung:        Von transportierten Items werden bis zu 200 GE nicht auf
 *                 das Traggewicht angerechnet.
*/
int
sp_create_bag_of_holding(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	change_item(mage,I_BAG_OF_HOLDING,1);

	creation_message(mage, I_BAG_OF_HOLDING);
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:		   Mauern der Ewigkeit
 * Stufe:		   7
 * Kategorie:  Artefakt
 * Gebiet:     Tybied
 * Wirkung:
 * Das Gebäude kostet keinen Unterhalt mehr
 *
 * ZAUBER "Mauern der Ewigkeit" <gebäude-nummer>
 * Flags: (0)
 */
static int
sp_eternizewall(castorder *co)
{
	unit *u;
	curse * success;
	building *b;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;
	spellparameter *pa = co->par;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	b = pa->param[0]->data.b;
	success = create_curse(mage, &b->attribs, C_NOCOST, 0,
			power*power, 1, 0, 0);

	if(!success){	/* ist bereits verzaubert */
		cmistake(mage, strdup(co->order), 206, MSG_MAGIC);
		return 0;
	}

	set_curseflag(b->attribs, C_NOCOST, 0, CURSE_NOAGE);

	/* Nur einmal  pro Burg möglich */
	set_curseflag(b->attribs, C_NOCOST, 0, CURSE_ONLYONE);

	/* melden, 1x pro Partei in der Burg */
	for (u = r->units; u; u = u->next) freset(u->faction, FL_DH);
	for (u = r->units; u; u = u->next) {
		if (!fval(u->faction, FL_DH)) {
			fset(u->faction, FL_DH);
			if (u->building ==  b) {
				sprintf(buf, "Mit einem Ritual bindet %s die magischen Kräfte "
						"der Erde in die Mauern von %s", unitname(mage),
						buildingname(b));
				addmessage(r, u->faction, buf, MSG_EVENT, ML_INFO);
			}
		}
	}

	return cast_level;
}


/* ------------------------------------------------------------- */
/* Name:       Opfere Kraft
 * Stufe:      15
 * Gebiet:     Tybied
 * Kategorie:  Einheit, positiv
 * Wirkung:
 *   Mit Hilfe dieses Zaubers kann der Magier einen Teil seiner
 *   magischen Kraft permanent auf einen anderen Magier übertragen. Auf
 *   einen Tybied-Magier kann er die Hälfte der eingesetzten Kraft
 *   übertragen, auf einen Magier eines anderen Gebietes ein Drittel.
 *
 * Flags:
 * (UNITSPELL|ONETARGET)
 *
 * Syntax:
 * ZAUBERE \"Opfere Kraft\" <Einheit-Nr> <Aura>
 * "ui"
 */
int
sp_permtransfer(castorder *co)
{
	int aura;
	unit *tu;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	spellparameter *pa = co->par;
	spell *sp = co->sp;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	/* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
	 * abbrechen aber kosten lassen */
	if(pa->param[0]->flag == TARGET_RESISTS) return cast_level;

	tu = pa->param[0]->data.u;
	aura = pa->param[0]->data.i;

	if(!is_mage(tu)) {
/*		sprintf(buf, "%s in %s: 'ZAUBER \"%s\"': Einheit ist kein Magier."
			, unitname(mage), regionid(mage->region),sa->strings[0]); */
		cmistake(mage, strdup(co->order), 214, MSG_MAGIC);
		return 0;
	}

  aura = min(get_spellpoints(mage)-spellcost(mage, sp),aura);

	change_maxspellpoints(mage,-aura);
	change_spellpoints(mage,-aura);

	if(get_mage(tu)->magietyp == get_mage(mage)->magietyp) {
		change_maxspellpoints(tu, aura/2);
	} else {
		change_maxspellpoints(tu, aura/3);
	}

	sprintf(buf, "%s opfert %s %d Aura.", unitname(mage), unitname(tu), aura);
	addmessage(r, mage->faction, buf, MSG_MAGIC, ML_INFO);

	return cast_level;
}

/* ------------------------------------------------------------- */
/* TODO: specialdirections? */

int
sp_movecastle(castorder *co)
{
	building *b;
	direction_t dir;
	region *target_region;
	unit *u, *unext;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	spellparameter *pa = co->par;
	spell *sp = co->sp;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	b = pa->param[0]->data.b;
	dir = finddirection(pa->param[1]->data.s, mage->faction->locale);

	if(dir == NODIRECTION) {
		sprintf(buf, "%s in %s: 'ZAUBER \"%s\"': Ungültige Richtung %s.",
			unitname(mage), regionid(mage->region), sp->name,
			pa->param[1]->data.s);
		addmessage(0, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
		return 0;
	}

	if(b->size > (cast_level-12) * 250) {
		sprintf(buf, "%s in %s: 'ZAUBER \"%s\"': Der Elementar ist "
			"zu klein, um das Gebäude zu tragen.", unitname(mage),
			regionid(mage->region), sp->name);
		addmessage(0, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
		return cast_level;
	}

	target_region = rconnect(r,dir);

	if(!(terrain[target_region->terrain].flags & LAND_REGION)) {
		sprintf(buf, "%s in %s: 'ZAUBER \"%s\"': Der Erdelementar "
				"weigert sich, nach %s zu gehen.",
			unitname(mage), regionid(mage->region), sp->name,
			locale_string(mage->faction->locale, directions[dir]));
		addmessage(0, mage->faction, buf, MSG_MAGIC, ML_MISTAKE);
		return cast_level;
	}

	bunhash(b);
	translist(&r->buildings, &target_region->buildings, b);
	b->region = target_region;
	b->size -= b->size/(10-rand()%6);
	bhash(b);

	for(u=r->units;u;) {
		unext = u->next;
		if(u->building == b) {
			uunhash(u);
			translist(&r->units, &target_region->units, u);
			uhash(u);
		}
		u = unext;
	}

	sprintf(buf, "Ein Beben erschüttert %s. Viele kleine Pseudopodien "
		"erheben das Gebäude und tragen es in Richtung %s.",
		buildingname(b), locale_string(mage->faction->locale, directions[dir]));

	if((b->type==&bt_caravan || b->type==&bt_dam || b->type==&bt_tunnel)) {
		boolean damage = false;
		direction_t d;
		for (d=0;d!=MAXDIRECTIONS;++d) {
			if (rroad(r, d)) {
				rsetroad(r, d, rroad(r, d)/2);
				damage = true;
			}
		}
		if (damage) strcat(buf, " Die Straßen der Region wurden beschädigt.");
	}
	addmessage(r, 0, buf, MSG_MAGIC, ML_INFO);

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Luftschiff
 * Stufe:      6
 *
 * Wirkung:
 * Läßt ein Schiff eine Runde lang fliegen.  Wirkt nur auf Boote und
 * Langboote.
 * Kombinierbar mit "Günstige Winde", aber nicht mit "Sturmwind".
 *
 * Flag:
 *  (ONSHIPCAST | SHIPSPELL | ONETARGET | TESTRESISTANCE)
 */
int
sp_flying_ship(castorder *co)
{
	ship *sh;
	unit *u;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;
	spellparameter *pa = co->par;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	sh = pa->param[0]->data.sh;

	if(is_cursed(sh->attribs, C_SHIP_FLYING, 0) ) {
/*		sprintf(buf, "Auf dem Schiff befindet liegt bereits so ein Zauber."); */
		cmistake(mage, strdup(co->order), 211, MSG_MAGIC);
		return 0;
	}
	if(is_cursed(sh->attribs, C_SHIP_SPEEDUP, 0) ) {
/*		sprintf(buf, "Es ist zu gefährlich, ein sturmgepeitschtes Schiff "
				"fliegen zu lassen."); */
		cmistake(mage, strdup(co->order), 210, MSG_MAGIC);
		return 0;
	}
	/* mit C_SHIP_NODRIFT haben wir kein Problem */

	/* Duration = 1, nur diese Runde */
	create_curse(mage, &sh->attribs, C_SHIP_FLYING, 0, power, 1, 0, 0);
	/* Da der Spruch nur diese Runde wirkt, brauchen wir kein
	 * set_cursedisplay() zu benutzten - es sieht eh niemand...
	 */
	sh->coast = NODIRECTION;

	/* melden, 1x pro Partei */
	for (u = r->units; u; u = u->next) freset(u->faction, FL_DH);
	for(u = r->units; u; u = u->next ) {
		/* das sehen natürlich auch die Leute an Land */
		if(!fval(u->faction, FL_DH) ) {
			fset(u->faction, FL_DH);
			sprintf(buf, "%s beschwört einen Luftgeist, der die %s in "
					"die Wolken hebt.",
				cansee(u->faction, r, mage, 0) ? unitname(mage) : "Jemand",
				shipname(sh));
			addmessage(r, u->faction, buf, MSG_EVENT, ML_INFO);
		}
	}
	return cast_level;
}


/* ------------------------------------------------------------- */
/* Name:       Stehle Aura
 * Stufe:      6
 * Kategorie:  Einheit, negativ
 * Wirkung:
 *     Mit Hilfe dieses Zaubers kann der Magier einem anderen Magier
 *     seine Aura gegen dessen Willen entziehen und sich selber
 *     zuführen.
 *
 * Flags:
 * (FARCASTING | SPELLLEVEL | UNITSPELL | ONETARGET | TESTRESISTANCE |
 * TESTCANSEE)
 * */
int
sp_stealaura(castorder *co)
{
	int taura;
	unit *u;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int power = co->force;
	spellparameter *pa = co->par;

	/* wenn kein Ziel gefunden, Zauber abbrechen */
	if(pa->param[0]->flag == TARGET_NOTFOUND) return 0;

	/* Zieleinheit */
	u  = pa->param[0]->data.u;

	if(!get_mage(u)) {
		add_message(&mage->faction->msgs, new_message(mage->faction,
			"stealaura_fail%u:unit%u:target", mage, u));
		add_message(&u->faction->msgs, new_message(u->faction,
			"stealaura_fail_detect%u:unit", u));
		return 0;
	}

	taura = (get_mage(u)->spellpoints*(rand()%(3*power)+1))/100;

	if(taura > 0) {
		get_mage(u)->spellpoints -= taura;
		get_mage(mage)->spellpoints += taura;
/*		sprintf(buf, "%s entzieht %s %d Aura.", unitname(mage), unitname(u),
			taura); */
		add_message(&mage->faction->msgs, new_message(mage->faction,
			"stealaura_success%u:mage%u:target%i:aura", mage, u, taura));
/*		sprintf(buf, "%s fühlt seine magischen Kräfte schwinden und verliert %d "
			"Aura.", unitname(u), taura); */
		add_message(&u->faction->msgs, new_message(u->faction,
			"stealaura_detect%u:unit%i:aura", u, taura));
	} else {
		add_message(&mage->faction->msgs, new_message(mage->faction,
			"stealaura_fail%u:unit%u:target", mage, u));
		add_message(&u->faction->msgs, new_message(u->faction,
			"stealaura_fail_detect%u:unit", u));
	}
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Erschaffe Antimagiekristall
 * Stufe:      7
 * Kategorie:  Artefakt
 * Wirkung:
 * 	 Erzeugt Antimagiekristall
 */
int
sp_create_antimagiccrystal(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	change_item(mage,I_ANTIMAGICCRYSTAL,1);
	creation_message(mage, I_ANTIMAGICCRYSTAL);
	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Astrale Schwächezone
 * Stufe:      5
 * Kategorie:
 * Wirkung:
 *    Reduziert die Stärke jedes Spruch in der Region um Level Hält
 *    Sprüche bis zu einem Gesammtlevel von Stärke*10 aus, danach ist
 *    sie verbraucht.
 *    leibt bis zu Stärke Wochen aktiv.
 *    Ein Ring der Macht erhöht die Stärke um 1, in einem Magierturm
 *    gezaubert gibt nochmal +1 auf Stärke. (force)
 *
 *    Beispiel:
 *    Eine Antimagiezone Stufe 7 hält bis zu 7 Wochen an oder Sprüche mit
 *    einem Gesammtlevel bis zu 70 auf. Also zB 7 Stufe 10 Sprüche, 10
 *    Stufe 7 Sprüche oder 35 Stufe 2 Sprüche.  Sie reduziert die Stärke
 *    (level+boni) jedes Spruchs, der in der Region gezaubert wird, um
 *    7. Alle Sprüche mit einer Stärke kleiner als 7 schlagen fehl
 *    (power = 0).
 *
 * Flags:
 * (FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE)
 *
 */
int
sp_antimagiczone(castorder *co)
{
	int power;
	int effect;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;

	/* Hält Sprüche bis zu einem summierten Gesamtlevel von power aus.
	 * Jeder Zauber reduziert die 'Lebenskraft' (vigour) der Antimagiezone
	 * um seine Stufe */
	power = force * 10;

	/* Reduziert die Stärke jedes Spruchs um effect */
	effect = cast_level;

	create_curse(mage, &r->attribs, C_ANTIMAGICZONE, 0, power, force,
			effect, 0);

	/* Erfolg melden*/
	add_message(&mage->faction->msgs, new_message(mage->faction,
				"regionmagic_effect%u:unit%r:region%s:command%u:unit", mage,
				mage->region, strdup(co->order), mage));

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:           Schutzrunen
 * Stufe:          8
 * Kosten:         SPC_FIX
 *
 * Wirkung:
 *   Gibt Gebäuden einen Bonus auf Magieresistenz von +20%. Der Zauber
 *   dauert 3+rand()%Level Wochen an, also im Extremfall bis zu 2 Jahre
 *   bei Stufe 20
 *
 *   Es können mehrere Zauber übereinander gelegt werden, der Effekt
 *   summiert sich, jedoch wird die Dauer dadurch nicht verlängert.
 *
 * oder:
 *
 *   Gibt Schiffen einen Bonus auf Magieresistenz von +20%. Der Zauber
 *   dauert 3+rand()%Level Wochen an, also im Extremfall bis zu 2 Jahre
 *   bei Stufe 20
 *
 *   Es können mehrere Zauber übereinander gelegt werden, der Effekt
 *   summiert sich, jedoch wird die Dauer dadurch nicht verlängert.
 *
 * Flags:
 * (ONSHIPCAST | TESTRESISTANCE)
 *
 * Syntax:
 *  ZAUBERE \"Runen des Schutzes\" BURG <Burg-nr>
 *  ZAUBERE \"Runen des Schutzes\" GEBÄUDE <Gebäude-Nr>
 *  ZAUBERE \"Runen des Schutzes\" SCHIFF <Schiff-Nr>
 * "kc"
 */

int
sp_antimagicshild(castorder *co)
{
	int duration;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;
	spellparameter *pa = co->par;

	duration = 3 + rand()%cast_level;

	switch(pa->param[0]->typ){
		case SPP_BUILDING:
		{
			building *b;
			b = pa->param[0]->data.b;

			/* Magieresistenz der Burg erhöht sich um 20% */
			create_curse(mage, &b->attribs, C_RESIST_MAGIC, 0, force,
					duration, 20, 0);

			/* Erfolg melden */
			add_message(&mage->faction->msgs, new_message(mage->faction,
					"objmagic_effect%u:unit%r:region%s:command%s:target", mage,
					mage->region, strdup(co->order), buildingname(b)));
			break;
		}
		case SPP_SHIP:
		{
			ship *sh;
			sh = pa->param[0]->data.sh;
			/* Magieresistenz des Schiffs erhöht sich um 20% */
			create_curse(mage, &sh->attribs, C_RESIST_MAGIC, 0, force,
					duration, 20, 0);

			/* Erfolg melden */
			add_message(&mage->faction->msgs, new_message(mage->faction,
					"objmagic_effect%u:unit%r:region%s:command%s:target", mage,
					mage->region, strdup(co->order), shipname(sh)));
			break;
		}
		default:
		/* fehlerhafter Parameter */
		return 0;
	}

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:   Zeitdehnung
 *
 * Flags:
 * (UNITSPELL | SPELLLEVEL | ONSHIPCAST | TESTCANSEE)
 * Syntax:
 *  "u+"
 */

int
sp_speed2(castorder *co)
{
	int n, maxmen, used = 0, dur, men;
	unit *u;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;
	spellparameter *pa = co->par;

	maxmen = 2 * cast_level * cast_level;
	dur = max(1, cast_level/2);

	for (n = 0; n < pa->length; n++) {
		/* sollte nie negativ werden */
		if (maxmen < 1)
			break;

		if(pa->param[n]->flag == TARGET_RESISTS
				|| pa->param[n]->flag == TARGET_NOTFOUND)
			continue;

		u = pa->param[n]->data.u;

		men = min(maxmen,u->number);
		create_curse(mage, &u->attribs, C_SPEED, 0, force, dur, 2, men);
		maxmen -= men;
		used += men;
	}

	/* TODO: Erfolg melden*/
	/* Effektiv benötigten cast_level (mindestens 1) zurückgeben */
	used = (int)sqrt(used/2);
	return max(1, used);
}

/* ------------------------------------------------------------- */
/* Name:           Magiefresser
 * Stufe:          7
 * Kosten:         SPC_LEVEL
 *
 * Wirkung:
 *   Kann eine bestimmte Verzauberung angreifen und auflösen. Die Stärke
 *   des Zaubers muss stärker sein als die der Verzauberung.
 * Syntax:
 *  ZAUBERE \"Magiefresser\" REGION [Zaubername]
 *  ZAUBERE \"Magiefresser\" EINHEIT <Einheit-Nr> [Zaubername]
 *  ZAUBERE \"Magiefresser\" BURG <Burg-Nr> [Zaubername]
 *  ZAUBERE \"Magiefresser\" GEBÄUDE <Gebäude-Nr> [Zaubername]
 *  ZAUBERE \"Magiefresser\" SCHIFF <Schiff-Nr> [Zaubername]
 *
 *  "kc?c"
 * Flags:
 *   (FARCASTING | SPELLLEVEL | ONSHIPCAST | TESTCANSEE)
 */
/* Jeder gebrochene Zauber verbraucht c->vigour an Zauberkraft
 * (force) */
int
sp_q_antimagie(castorder *co)
{
	attrib **ap;
	int obj;
	const curse_type * ctype;
	int succ;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	int force = co->force;
	spellparameter *pa = co->par;
	char *ts;

	obj = pa->param[0]->typ;

	ctype = NULL;
	if(pa->length == 2)
		ctype = ct_find(pa->param[1]->data.s);

	switch(obj){
		case SPP_REGION:
			ap = &r->attribs;
			set_string(&ts, regionid(r));
			break;

		case SPP_UNIT:
		{
			unit *u = pa->param[0]->data.u;
			ap = &u->attribs;
			set_string(&ts, unitid(u));
			break;
		}
		case SPP_BUILDING:
		{
			building *b = pa->param[0]->data.b;
			ap = &b->attribs;
			set_string(&ts, buildingid(b));
			break;
		}
		case SPP_SHIP:
		{
			ship *sh = pa->param[0]->data.sh;
			ap = &sh->attribs;
			set_string(&ts, shipid(sh));
			break;
		}
		default:
			/* Das Zielobjekt wurde vergessen */
			cmistake(mage, strdup(co->order), 203, MSG_MAGIC);
		return 0;
	}

	succ = destroy_curse(ap, cast_level, force, ctype);

	if(succ) {
		add_message(&mage->faction->msgs, new_message(mage->faction,
			"destroy_magic_effect%u:unit%r:region%s:command%i:succ%s:target",
			mage, mage->region, strdup(co->order), succ, strdup(ts)));
	} else {
		add_message(&mage->faction->msgs, new_message(mage->faction,
			"destroy_magic_noeffect%u:unit%r:region%s:command",
			mage, mage->region, strdup(co->order)));
	}

	return max(succ, 1);
}

/* ------------------------------------------------------------- */
/* Name:           Fluch brechen
 * Stufe:          7
 * Kosten:         SPC_LEVEL
 *
 * Wirkung:
 *   Kann eine bestimmte Verzauberung angreifen und auflösen. Die Stärke
 *   des Zaubers muss stärker sein als die der Verzauberung.
 * Syntax:
 *  ZAUBERE \"Fluch brechen\" REGION <Zauber-id>
 *  ZAUBERE \"Fluch brechen\" EINHEIT <Einheit-Nr> <Zauber-id>
 *  ZAUBERE \"Fluch brechen\" BURG <Burg-Nr> <Zauber-id>
 *  ZAUBERE \"Fluch brechen\" GEBÄUDE <Gebäude-Nr> <Zauber-id>
 *  ZAUBERE \"Fluch brechen\" SCHIFF <Schiff-Nr> <Zauber-id>
 *
 *  "kcc"
 * Flags:
 *   (FARCASTING | SPELLLEVEL | ONSHIPCAST | TESTCANSEE)
 */
int
sp_destroy_curse(castorder *co)
{
	attrib **ap;
	int obj;
	curse * c;
	int succ = 0;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level; 
	int force = co->force;
	spellparameter *pa = co->par;
	char *ts;

	if(pa->length < 2){
		/* Das Zielobjekt wurde vergessen */
		cmistake(mage, strdup(co->order), 203, MSG_MAGIC);
	}

	obj = pa->param[0]->typ;

	c = findcurse(atoi36(pa->param[1]->data.s));
	if (!c){
		/* Es wurde kein Ziel gefunden */
		add_message(&mage->faction->msgs, new_message(mage->faction, 
					"spelltargetnotfound%u:unit%r:region%s:command",
					mage, mage->region, strdup(co->order)));
	}

	switch(obj){
		case SPP_REGION:
			ap = &r->attribs;
			set_string(&ts, regionname(r, mage->faction));
			break;

		case SPP_UNIT:
		{
			unit *u = pa->param[0]->data.u;
			ap = &u->attribs;
			set_string(&ts, unitid(u));
			break;
		}
		case SPP_BUILDING:
		{
			building *b = pa->param[0]->data.b;
			ap = &b->attribs;
			set_string(&ts, buildingid(b));
			break;
		}
		case SPP_SHIP:
		{
			ship *sh = pa->param[0]->data.sh;
			ap = &sh->attribs;
			set_string(&ts, shipid(sh));
			break;
		}
		default:
			/* Das Zielobjekt wurde vergessen */
			cmistake(mage, strdup(co->order), 203, MSG_MAGIC);
		return 0;
	}

	/* überprüfung, ob curse zu diesem objekt gehört */
	if (!is_cursed_with(*ap, c)){
		/* Es wurde kein Ziel gefunden */
		add_message(&mage->faction->msgs, new_message(mage->faction, 
					"spelltargetnotfound%u:unit%r:region%s:command",
					mage, mage->region, strdup(co->order)));
	}

	/* curse auflösen, wenn zauber stärker (force > vigour)*/
	succ = c->vigour - force;
	c->vigour = max(0, succ);
	
	if(succ <= 0) {
		remove_cursec(ap, c);

		add_message(&mage->faction->msgs, new_message(mage->faction,
			"destroy_magic_effect%u:unit%r:region%s:command%i:id%s:target",
			mage, mage->region, strdup(co->order), strdup(pa->param[1]->data.s),
			strdup(ts)));
	} else {
		add_message(&mage->faction->msgs, new_message(mage->faction,
			"destroy_magic_noeffect%u:unit%r:region%s:command",
			mage, mage->region, strdup(co->order)));
	}

	return cast_level;
}


/* ------------------------------------------------------------- */
int
sp_becomewyrm(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int wyrms_already_created = 0;
	int wyrms_allowed;
	attrib *a;

	wyrms_allowed = fspecial(mage->faction, FS_WYRM);
	a = a_find(mage->faction->attribs, &at_wyrm);
	if(a) wyrms_already_created = a->data.i;

	if(wyrms_already_created >= wyrms_allowed) {
		cmistake(mage, strdup(co->order), 262, MSG_MAGIC);
		return 0;
	}

	if(!a) {
		a_add(&mage->faction->attribs, a_new(&at_wyrm));
		a->data.i = 1;
	} else {
		a->data.i++;
	}

	mage->race = new_race[RC_WYRM];
	addspell(mage, SPL_WYRMODEM);

	add_message(&mage->faction->msgs, new_message(mage->faction,
		"becomewyrm%u:mage", mage));

	return co->level;
}

/* ------------------------------------------------------------- */
/* Name:       Alltagszauber, hat je nach Gebiet anderen Namen
 * Stufe:      1
 * Gebiet:     alle
 * Wirkung:		 der Magier verdient $50 pro Spruchstufe
 * Kosten:		 1 SP pro Stufe
 */
#include "../gamecode/economy.h"
/* TODO: das ist scheisse, aber spells gehören eh nicht in den kernel */
int
sp_earn_silver(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	region *r = co->rt;
	int wanted, earned;

	wanted = cast_level * 50;
	earned = min(rmoney(r), wanted);
	rsetmoney(r, rmoney(r) - earned);
	change_money(mage, earned);
	/* TODO klären: ist das Silber damit schon reserviert? */

	add_income(mage, IC_MAGIC, wanted, earned);
	return cast_level;
}


/* ------------------------------------------------------------- */
/* Dummy-Zauberpatzer, Platzhalter für speziel auf die Sprüche
 * zugeschnittene Patzer */
void
patzer(castorder *co)
{
	unit *mage = (unit *)co->magician;

	report_failure(mage, co->order);
	return;
}

/* ------------------------------------------------------------- */
/* allgemeine Artefakterschaffungszauber (Gebietsunspezifisch)   */
/* ------------------------------------------------------------- */
/* Amulett des wahren Sehens */
int
sp_createitem_trueseeing(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	change_item(mage,I_AMULET_OF_TRUE_SEEING,1);
	creation_message(mage, I_AMULET_OF_TRUE_SEEING);

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Ring der Unsichtbarkeit */
int
sp_createitem_invisibility(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	change_item(mage,I_RING_OF_INVISIBILITY,1);
	creation_message(mage, I_RING_OF_INVISIBILITY);

	return cast_level;
}
/* ------------------------------------------------------------- */
/* Keuschheitsgürtel der Orks */
int
sp_createitem_chastitybelt(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	change_item(mage,I_CHASTITY_BELT,1);
	creation_message(mage, I_CHASTITY_BELT);

	return cast_level;
}
/* ------------------------------------------------------------- */
/* Ring der Macht
 * erhöht effektive Stufe +1 */
int
sp_createitem_power(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	change_item(mage,I_RING_OF_POWER,1);
	creation_message(mage, I_RING_OF_POWER);

	return cast_level;
}
/* ------------------------------------------------------------- */
/* Runenschwert */
int
sp_createitem_runesword(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	change_item(mage,I_RUNESWORD,1);
	creation_message(mage, I_RUNESWORD);

	return cast_level;
}
/* ------------------------------------------------------------- */
/* Artefakt der Stärke
 * Ermöglicht dem Magier mehr Magiepunkte zu 'speichern'
 */
int
sp_createitem_aura(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	change_item(mage,I_AURAKULUM,1);
	creation_message(mage, I_AURAKULUM);

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Artefakt der Regeneration
 * Heilt pro Runde HP
 * noch nicht implementiert
 */
int
sp_createitem_regeneration(castorder *co)
{
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;

	change_item(mage,I_RING_OF_REGENERATION,1);
	creation_message(mage, I_RING_OF_REGENERATION);

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Dummy-Zauberpatzer, Platzhalter für speziel auf die Sprüche
 * zugeschnittene Patzer */
void
patzer_createitem(castorder *co)
{
	unit *mage = (unit *)co->magician;

	report_failure(mage, co->order);
	return;
}

/* ------------------------------------------------------------- */
/* Erläuterungen zu den Spruchdefinitionen
 *
 * Spruchstukturdefinition:
 * spell{
 *  id, name,
 *  beschreibung,
 *  syntax,
 *  parameter,
 *  magietyp,
 *  sptyp,
 *  rank,level,
 *  costtyp, aura,
 *  komponenten[5][2][faktorart],
 *  &funktion, patzer}
 *
 * id:
 * SPL_NOSPELL muss der letzte Spruch in der Liste spelldaten[] sein,
 * denn nicht auf die Reihenfolge in der Liste sondern auf die id wird
 * geprüft
 *
 * sptyp:
 * besondere Spruchtypen und Flags
 *    (Regionszauber, Kampfzauber, Farcastbar, Stufe variable, ..)
 *
 * rank:
 * gibt die Priorität und damit die Reihenfolge an, in der der Spruch
 * gezaubert wird.
 * 1: Aura übertragen
 * 2: Antimagie
 * 3: Magierverändernde Sprüche (Magic Boost, ..)
 * 4: Monster erschaffen
 * 5: Standartlevel
 * 7: Teleport
 *
 * Komponenten[Anzahl mögl. Items][Art:Anzahl:Kostentyp]
 *
 * R_AURA:
 * Grundkosten für einen Zauber. Soviel Mp müssen mindestens investiert
 * werden, um den Spruch zu wirken. Zusätzliche Mp können unterschiedliche
 * Auswirkungen haben, die in der Spruchfunktionsroutine definiert werden.
 *
 * R_PERMAURA:
 * Kosten an permantenter Aura
 *
 * Komponenten Kostentyp:
 * SPC_LEVEL == Spruch mit Levelabhängigen Magiekosten. Die angegeben
 * Kosten müssen für Stufe 1 berechnet sein.
 * SPC_FIX   == Feste Kosten
 *
 * Wenn keine spezielle Syntax angegeben ist, wird die
 * Syntaxbeschreibung aus sptyp  generiert:
 * FARCASTING: ZAUBER [REGION x y]
 * SPELLLEVEL: ZAUBER [STUFE n]
 * UNITSPELL : ZAUBER <spruchname> <Einheit-Nr> [<Einheit-Nr> ..]
 * SHIPSPELL : ZAUBER <spruchname> <Schiff-Nr> [<Schiff-Nr> ..]
 * BUILDINGSPELL: ZAUBER <spruchname> <Gebäude-Nr> [<Gebäude-Nr> ..]
 * ONETARGET : ZAUBER <spruchname> <target-nr>
 * PRECOMBATSPELL : KAMPFZAUBER [STUFE n] <spruchname>
 * COMBATSPELL    : KAMPFZAUBER [STUFE n] <spruchname>
 * POSTCOMBATSPELL: KAMPFZAUBER [STUFE n] <spruchname>
 *
 * Das Parsing
 *
 * Der String spell->parameter gibt die Syntax an, nach der die
 * Parameter des Spruches in add_spellparameter() geparst werden sollen.
 *
 * u : eine Einheitennummer
 * r : hier kommen zwei Regionskoordinaten x y
 * b : Gebäude- oder Burgnummer
 * s : Schiffsnummer
 * c : String, wird ohne Weiterverarbeitung übergeben
 * i : Zahl (int), wird ohne Weiterverarbeitung übergeben
 * k : Keywort - dieser String gibt den Paramter an, der folgt. Der
 *     Parameter wird mit findparam() identifiziert.
 *     k muss immer von einem c als Platzhalter für das Objekt gefolgt
 *     werden.
 *     Ein gutes Beispiel sind hierfür die Sprüche zur Magieanalyse.
 * + : gibt an, das der vorherige Parameter mehrfach vorkommen kann. Da
 *     ein Ende nicht definiert werden kann, muss dies immer am Schluss
 *     kommen.
 * ? : gibt an, das der folgende Parameter optional ist. Da nicht
 *     definiert werden kann, ob wirklich ein Parameter nachfolgt oder
 *     etwas anderes, muss dies immer als vorletztes Zeichen stehen und
 *     darf nicht direkt von einem + gefolgt werden.
 *
 * Flags für das Parsing:
 * TESTRESISTANCE : alle Zielobjekte, also alle Parameter vom Typ Unit,
 *                  Burg, Schiff oder Region, werden auf ihre
 *                  Magieresistenz überprüft
 * TESTCANSEE     : jedes Objekt vom Typ Einheit wird auf seine
 *                  Sichtbarkeit überprüft
 * SEARCHGLOBAL   : die Zielobjekte werden global anstelle von regional
 *                  gesucht
 * REGIONSPELL    : Ziel ist die Region, auch wenn kein Zielobjekt
 *                  angegeben wird. Ist TESTRESISTANCE gesetzt, so wird
 *                  die Magieresistenz der Region überprüft
 *
 * Bei fehlendem Ziel oder wenn dieses dem Zauber widersteht, wird die
 * Spruchfunktion nicht aufgerufen.
 * Sind zu wenig Parameter vorhanden, wird der Zauber ebenfalls nicht
 * ausgeführt.
 * Ist eins von mehreren Zielobjekten resistent, so wird das Flag
 * pa->param[n]->flag == TARGET_RESISTS
 * Ist eins von mehreren Zielobjekten nicht gefunden worden, so ist
 * pa->param[n]->flag == TARGET_NOTFOUND
 *
 */
/* ------------------------------------------------------------- */

 /* Bitte die Sprüche nach Gebieten und Stufe ordnen, denn in derselben
	* Reihenfolge wie in Spelldaten tauchen sie auch im Report auf
	*/

spell spelldaten[] =
{

/* M_DRUIDE */

	{SPL_BLESSEDHARVEST, "Segen der Erde",
	 "Dieses Ernteritual verbessert die Erträge der arbeitenden Bauern in der "
	 "Region um ein Silberstück. Je mehr Kraft der Druide investiert, desto "
	 "länger wirkt der Zauber.",
		NULL,
		NULL,
		M_DRUIDE,
		(FARCASTING | SPELLLEVEL | ONSHIPCAST | REGIONSPELL),
		5, 1,
		{
			{R_AURA, 1, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_blessedharvest,
		patzer
	},

	{SPL_GWYRRD_EARN_SILVER, "Viehheilung",
	"Die Fähigkeiten der Gwyrrd-Magier in der Viehzucht und Heilung sind bei "
	"den Bauern sehr begehrt. Grade auf Märkten sind ihre Dienste häufig sehr "
	"gefragt. Manch einer mag auch sein Talent dazu nutzen, ein Tier für einen "
	"besseren Preis zu verkaufen.  Pro Stufe kann der Magier so 50 Silber "
	"verdienen.",
		NULL,
		NULL,
		M_DRUIDE,
		(SPELLLEVEL|ONSHIPCAST),
		5, 1,
		{
			{R_AURA, 1, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_earn_silver,
		patzer
	},

	{SPL_STONEGOLEM, "Erschaffe Steingolems",
		"Man befeuchte einen kluftfreien Block aus feinkristallinen Gestein "
		"mit einer Phiole des Lebenswassers bis dieses vollständig vom Gestein "
		"aufgesogen wurde. Sodann richte man seine Kraft auf die sich bildenden "
		"feine "
		"Aura des Lebens und forme der ungebundenen Kraft ein Gehäuse. Je mehr "
		"Kraft der Magier investiert, desto mehr Golems können geschaffen "
		"werden, bevor die Aura sich verflüchtigt. "
		"Jeder Golem hat jede Runde eine Chance von 10% zu Staub zu zerfallen. "
		"Gibt man den Golems die Befehle MACHE BURG oder MACHE STRASSE, "
		"so werden pro Golem 5 Steine verbaut und der Golem löst sich auf.",
		NULL,
		NULL,
		M_DRUIDE, (SPELLLEVEL), 4, 1,
		{
		 {R_AURA, 2, SPC_LEVEL},
		 {R_STONE, 1, SPC_LEVEL},
		 {R_TREES, 1, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_create_stonegolem, patzer
	},

	{SPL_IRONGOLEM, "Erschaffe Eisengolems",
		"Je mehr Kraft der Magier investiert, desto mehr Golems können "
		"geschaffen werden. "
		"Jeder Golem hat jede Runde eine Chance von 15% zu Staub zu zerfallen. "
		"Gibt man den Golems den Befehl MACHE SCHWERT/BIHÄNDER oder "
		"MACHE SCHILD/KETTENHEMD/PLATTENPANZER, so werden pro Golem 4 "
		"Eisenbarren verbaut und der Golem löst sich auf.",
		NULL,
		NULL,
	 M_DRUIDE, (SPELLLEVEL), 4, 2,
	 {
		 {R_AURA, 2, SPC_LEVEL},
		 {R_IRON, 1, SPC_LEVEL},
		 {R_TREES, 1, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_create_irongolem, patzer
	},

	{SPL_TREEGROW, "Hainzauber",
		"Diese Ritual verstärkt die Wirkung des magischen Trankes um ein "
		"vielfaches. Wo sonst aus einem Stecken nur ein Baum spießen konnte, "
		"so treibt nun jeder Ast Wurzeln.",
		NULL,
		NULL,
	 M_DRUIDE,
	 (FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE),
	 5, 2,
	 {
		 {R_AURA, 4, SPC_LEVEL},
		 {R_WOOD, 1, SPC_LEVEL},
		 {R_TREES, 1, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_hain, patzer_ents
	},

	{SPL_RUSTWEAPON, "Rostregen",
		"Mit diesem Ritual wird eine dunkle Gewitterfront beschworen, die sich "
		"unheilverkündend über der Region auftürmt. Der magische Regen wird "
		"alles Erz rosten lassen. Seine Kraft reicht nicht aus, um Rüstungen "
		"unbrauchbar zu machen, jedoch werden Eisenwaffen so unbrauchbar "
		"schartig und rostig, dass man sie wegwerfen muss. Die Zerstörungskraft "
		"des Regens ist von der investierten Kraft des Magiers abhängig. Für "
		"jede Stufe können bis zu 10 Eisenwaffen betroffen werden. Ein Ring "
		"der Macht verstärkt die Wirkung wie eine zusätzliche Stufe.",
		NULL,
		"u+",
		M_DRUIDE,
		(FARCASTING | SPELLLEVEL | UNITSPELL | TESTCANSEE | TESTRESISTANCE),
		5, 3,
		{
		 {R_AURA, 2, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_rosthauch, patzer
	},

	{SPL_KAELTESCHUTZ, "Firuns Fell",
		"Dieser Zauber ermöglicht es dem Magier Insekten auf "
		"magische Weise vor der lähmenden Kälte der Gletscher zu bewahren. Sie "
		"können Gletscher betreten und dort normal agieren. Der Spruch wirkt "
		"auf Stufe*10 Insekten. Ein Ring der Macht erhöht die Menge der "
		"verzauberbaren Insekten zusätzlich um 10.",
		NULL,
		"u+",
		M_DRUIDE,
		(UNITSPELL | SPELLLEVEL | TESTCANSEE | ONSHIPCAST),
		5, 3,
		{
		 {R_AURA, 2, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_kaelteschutz, patzer
	},

	{SPL_HAGEL, "Hagel",
		"Im Kampf ruft der Magier die Elementargeister der Kälte an und bindet "
		"sie an sich. Sodann kann er ihnen befehlen, den Gegner mit Hagelkörnern "
		"und Eisbrocken zuzusetzen.",
		NULL,
		NULL,
		M_DRUIDE, (COMBATSPELL|SPELLLEVEL), 5, 3,
		{
			{R_AURA, 1, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_kampfzauber, patzer
	},

	{SPL_IRONKEEPER, "Bergwächter",
		"Erschafft in Bergen oder Gletschern einen Wächtergeist, der Eisen- und "
		"Laenabbau durch nichtalliierte Parteien (HELFE BEWACHE) verhindert, "
		"solange er die Region bewacht. Der Bergwächter ist an den Ort der "
		"Beschwörung gebunden.",
		NULL,
		NULL,
	 M_DRUIDE,
	 (FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE),
	 5, 3,
	 {
		 {R_AURA, 3, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_ironkeeper, patzer
	},

	{SPL_MAGICSTREET, "Magischer Pfad",
		"Durch Ausführung dieser Rituale ist der Magier in der Lage einen "
		"mächtigen Erdelementar zu beschwören. Solange dieser in den Boden "
		"gebannt ist, wird kein Regen die Wege aufweichen und kein Fluß "
		"Brücken zerstören können. Alle Reisende erhalten damit die gleichen "
		"Vorteile, die sonst nur ein ausgebautes gepflastertes Straßennetz "
		"bietet. Selbst Sümpfe und Gletscher können so verzaubert werden. "
		"Je mehr Kraft der Magier in den Bann legt, desto länger bleibt "
		"die Straße bestehen.",
		"ZAUBERE \"Magischer Pfad\" <Richtung>",
		"c",
		M_DRUIDE,
		(FARCASTING | SPELLLEVEL | REGIONSPELL | ONSHIPCAST | TESTRESISTANCE),
		5, 4,
		{
			{R_AURA, 1, SPC_LEVEL},
			{R_STONE, 1, SPC_FIX},
			{R_WOOD, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_magicstreet, patzer
	},

	{SPL_WINDSHIELD, "Windschild",
		"Die Anrufung der Elementargeister des Windes beschwört plötzliche "
		"Windböen, kleine Windhosen und Luftlöcher herauf, die die gegnerischen "
		"Schützen behindern werden.",
		NULL,
		NULL,
		M_DRUIDE, (PRECOMBATSPELL | SPELLLEVEL), 5, 4,
		{
			{R_AURA, 2, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}
		},
		(spell_f)sp_windshield, patzer
	},

	{SPL_MALLORNTREEGROW, "Segne Mallornstecken",
		"Diese Ritual verstärkt die Wirkung des magischen Trankes um ein "
		"vielfaches. Wo sonst aus einem Stecken nur ein Baum spießen konnte, "
		"so treibt nun jeder Ast Wurzeln.",
		NULL,
		NULL,
	 M_DRUIDE,
	 (FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE),
	 5, 4,
	 {
		 {R_AURA, 6, SPC_LEVEL},
		 {R_MALLORN, 2, SPC_LEVEL},
		 {R_TREES, 1, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_mallornhain, patzer_ents
	},

	{ SPL_GOODWINDS, "Beschwörung eines Wasserelementar",
		"Der Magier zwingt mit diesem Ritual die Elementargeister des Wassers "
		"in seinen Dienst und bringt sie dazu, das angegebene Schiff schneller "
		"durch das Wasser zu tragen. Zudem wird das Schiff nicht durch "
		"ungünstige Winde oder Strömungen beeinträchtigt.",
		NULL,
		"s",
		M_DRUIDE,
		(SHIPSPELL|ONSHIPCAST|SPELLLEVEL|ONETARGET|TESTRESISTANCE),
		5, 4,
		{
			{R_AURA, 1, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}
		},
	 (spell_f)sp_goodwinds, patzer
	},

	{SPL_HEALING, "Heilung",
		"Nicht nur der Feldscher kann den Verwundeten einer Schlacht helfen. "
		"Druiden vermögen mittels einer Beschwörung der Elementargeister des "
		"Lebens Wunden zu schließen, gebrochene Knochen zu richten und "
		"selbst abgetrennte Glieder wieder zu regenerieren.",
		NULL,
		NULL,
		M_DRUIDE, (POSTCOMBATSPELL | SPELLLEVEL), 5, 5,
	 {
			{R_AURA, 1, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_healing, patzer
	},

	{SPL_REELING_ARROWS, "Wirbelwind",
		"Diese Beschwörung öffnet ein Tor in die Ebene der Elementargeister "
		"des Windes. Sofort erheben sich in der Umgebung des Tors starke "
		"Winde oder gar Stürme und behindern alle Schützen einer Schlacht.",
		NULL,
		NULL,
		M_DRUIDE, (PRECOMBATSPELL | SPELLLEVEL), 5, 5,
		{
			{R_AURA, 15, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_reeling_arrows, patzer
	},

	{SPL_GWYRRD_FUMBLESHIELD, "Astralschutzgeister",
		"Dieses Ritual beschwört einige Elementargeister der Magie und "
		"schickt sie in die Reihen der feindlichen Magier. Diesen wird das "
		"Zaubern für die Dauer des Kampfes deutlich schwerer fallen.",
		NULL,
		NULL,
	 M_DRUIDE, (PRECOMBATSPELL | SPELLLEVEL), 2, 5,
	 {
			{R_AURA, 5, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_fumbleshield, patzer
	},

	{SPL_TRANSFERAURA_DRUIDE, "Meditation",
	 "Mit Hilfe dieses Zaubers kann der Magier eigene Aura im Verhältnis "
	 "2:1 auf einen anderen Magier des gleichen Magiegebietes übertragen.",
	 "ZAUBERE \"Meditation\" <Einheit-Nr> <investierte Aura>",
		"ui",
	 M_DRUIDE, (UNITSPELL|ONSHIPCAST|ONETARGET), 1, 6,
	 {
			{R_AURA, 2, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_transferaura, patzer
	},

	{SPL_EARTHQUAKE, "Beschwöre einen Erdelementar",
		"Der Druide beschwört mit diesem Ritual einen Elementargeist "
		"der Erde und bringt ihn dazu, die Erde erbeben zu lassen. "
		"Dieses Erdbeben wird alle Gebäude in der Region beschädigen.",
		NULL,
		NULL,
	 M_DRUIDE, (FARCASTING|REGIONSPELL|TESTRESISTANCE), 5, 6,
	 {
		 {R_AURA, 25, SPC_FIX},
		 {R_EOG, 2, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_earthquake, patzer
	},

	{SPL_STORMWINDS, "Beschwöre einen Sturmelementar",
		"Die Beschwörung von Elementargeistern der Stürme ist ein uraltes "
		"Ritual. Der Druide bannt die Elementare in die Segel der Schiffe, "
		"wo sie helfen, das Schiff mit hoher Geschwindigkeit über die Wellen "
		"zu tragen. Je mehr Kraft der Druide in den Zauber investiert, desto "
		"größer ist die Zahl der Elementargeister, die sich bannen lassen. "
		"Für jedes Schiff wird ein Elementargeist benötigt.",
		NULL,
		"s+",
		M_DRUIDE,
		(SHIPSPELL | ONSHIPCAST | OCEANCASTABLE | TESTRESISTANCE | SPELLLEVEL),
		5, 6,
		{
		 {R_AURA, 6, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_stormwinds, patzer
	},

	{SPL_TRUESEEING_GWYRRD, "Erschaffe ein Amulett des wahren Sehens",
		"Der Spruch ermöglicht es einem Magier, ein Amulett des Wahren Sehens "
		"zu erschaffen. Das Amulett erlaubt es dem Träger, alle Einheiten, die "
		"durch einen Ring der Unsichtbarkeit geschützt sind, zu sehen. Einheiten "
		"allerdings, die sich mit ihrem Tarnungs-Talent verstecken, bleiben "
		"weiterhin unentdeckt.",
		NULL,
		NULL,
		M_DRUIDE, (ONSHIPCAST), 5, 6,
		{
			{R_AURA, 50, SPC_FIX},
			{R_SILVER, 3000, SPC_FIX},
			{R_PERMAURA, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_createitem_trueseeing, patzer_createitem
	},

	{SPL_INVISIBILITY_GWYRRD, "Erschaffe einen Ring der Unsichtbarkeit",
		"Mit diesem Spruch kann der Zauberer einen Ring der Unsichtbarkeit "
		"erschaffen. Der Träger des Ringes wird für alle Einheiten anderer "
		"Parteien unsichtbar, egal wie gut ihre Wahrnehmung auch sein mag. In "
		"einer unsichtbaren Einheit muss jede Person einen Ring tragen.",
		NULL,
		NULL,
		M_DRUIDE, (ONSHIPCAST), 5, 6,
		{
			{R_AURA, 50, SPC_FIX},
			{R_SILVER, 3000, SPC_FIX},
			{R_PERMAURA, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_createitem_invisibility, patzer_createitem
	},

	{SPL_HOMESTONE, "Heimstein",
		"Mit dieser Formel bindet der Magier auf ewig die Kräfte der Erde in "
		"die Mauern der Burg, in der er sich gerade befindet. Weder magisch "
		"noch mit schwerem Geschütz können derartig gestärkte Mauern zerstört "
		"werden, und auch das Alter setzt ihnen weniger zu. Das Gebäude bietet "
		"sodann auch einen besseren Schutz gegen Angriffe mit dem Schwert wie "
		"mit Magie.",
		NULL,
		NULL,
		M_DRUIDE, (0), 5, 7,
		{
			{R_AURA, 50, SPC_FIX},
			{R_PERMAURA, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
	 (spell_f)sp_homestone, patzer
	},

	{SPL_WOLFHOWL, "Wolfsgeheul",
		"Nicht wenige Druiden freunden sich im Laufe ihres Lebens in der "
		"Natur mit den ältesten Freunden der großen Völker an. Sie erlernen, "
		"mit einem einzigen heulenden Ruf viele ihrer Freunde herbeizurufen, "
		"um ihnen im Kampf beizustehen.",
		NULL,
		NULL,
	 M_DRUIDE, (PRECOMBATSPELL | SPELLLEVEL ), 5, 7,
	 {
		 {R_AURA, 2, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_wolfhowl, patzer
	},

	{SPL_VERSTEINERN, "Blick des Basilisken",
		"Dieser schwierige, aber effektive Kampfzauber benutzt die "
		"Elementargeister des Steins, um eine Reihe von Gegnern für "
		"die Dauer des Kampfes in Stein zu verwandeln. Die betroffenen "
		"Personen werden nicht mehr kämpfen, können jedoch auch nicht "
		"verwundet werden.",
		NULL,
		NULL,
	 M_DRUIDE, (COMBATSPELL | SPELLLEVEL), 5, 8,
	 {
		 {R_AURA, 1, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_versteinern, patzer
	},

	{SPL_STRONG_WALL, "Starkes Tor und feste Mauer",
		"Mit dieser Formel bindet der Magier zu Beginn eines Kampfes einige "
		"Elementargeister des Fels in die Mauern des Gebäudes, in dem er sich "
		"gerade befindet. Das Gebäude bietet sodann einen besseren Schutz "
		"gegen Angriffe mit dem Schwert wie mit Magie.",
		NULL,
		NULL,
	 M_DRUIDE, (PRECOMBATSPELL | SPELLLEVEL), 5, 8,
	 {
			{R_AURA, 2, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_strong_wall, patzer
	},

	{SPL_GWYRRD_DESTROY_MAGIC, "Geister bannen",
		"Wie die alten Lehren der Druiden berichten, besteht das, was "
		"die normalen Wesen Magie nennen aus Elementargeistern. Der Magier "
		"beschwört und bannt diese in eine Form, um den gewünschten Effekt "
		"zu erzielen. Dieses Ritual nun vermag es, in diese Welt gerufene "
		"Elementargeister zu vertreiben, um so ein Objekt von Magie zu "
		"befreien.",
		"ZAUBERE [REGION x y] [STUFE n] \"Geister bannen\" REGION\n"
		"ZAUBERE [REGION x y] [STUFE n] \"Geister bannen\" EINHEIT <Einheit-Nr>\n"
		"ZAUBERE [REGION x y] [STUFE n] \"Geister bannen\" BURG <Burg-Nr>\n"
		"ZAUBERE [REGION x y] [STUFE n] \"Geister bannen\" GEBÄUDE <Gebäude-Nr>\n"
		"ZAUBERE [REGION x y] [STUFE n] \"Geister bannen\" SCHIFF <Schiff-Nr>",
		"kc",
		M_DRUIDE,
		(FARCASTING | SPELLLEVEL | ONSHIPCAST | ONETARGET | TESTCANSEE),
		2, 8,
	 {
			{R_AURA, 6, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_destroy_magic, patzer
	},

	{SPL_TREEWALKENTER, "Weg der Bäume",
	"Große Macht liegt in Orten, an denen das Leben pulsiert. Der "
	"Druide kann diese Kraft sammeln und so ein Tor in die Welt "
	"der Geistwesen erschaffen. Der Druide kann dann Stufe*5 "
	"Gewichtseinheiten durch das Tor entsenden.",
		NULL,
		"u+",
	 M_DRUIDE, (UNITSPELL | SPELLLEVEL | TESTCANSEE), 7, 9,
	 {
			{R_AURA, 3, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_treewalkenter, patzer
	},

	{SPL_TREEWALKEXIT, "Sog des Lebens",
		"Ein Druide, den es in die Welt der Geister verschlagen hat, kann "
		"mit Hilfe dieses Zaubers Stufe*5 Gewichtseinheiten in einen Wald "
		"auf der materiellen Welt zurückschicken.",
		"Zauber \"Sog des Lebens\" <Ziel-X> <Ziel-Y> <Einheit> [<Einheit> ..]",
		"ru+",
		M_DRUIDE, (UNITSPELL | SPELLLEVEL | TESTCANSEE), 7, 9,
		{
		 {R_AURA, 2, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
		(spell_f)sp_treewalkexit, patzer
	},

	{SPL_HOLYGROUND, "Heiliger Boden",
		"Dieses Ritual beschwört verschiedene Naturgeister in den Boden der "
		"Region, welche diese fortan bewachen. In einer so gesegneten Region "
		"werden niemals wieder die Toten ihre Gräber verlassen, und anderswo "
		"entstandene Untote werden sie wann immer möglich meiden.",
		NULL,
		NULL,
		M_DRUIDE, (0), 5, 9,
		{
		 {R_AURA, 80, SPC_FIX},
		 {R_PERMAURA, 3, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
		(spell_f)sp_holyground, patzer
	},

	{SPL_ARTEFAKT_SACK_OF_CONSERVATION, "Erschaffe einen magischen Kräuterbeutel",
		"Der Druide nehme etwas präpäriertes Leder, welches er in einem großen "
		"Ritual der Reinigung von allen unreinen Geistern befreie, und binde "
		"dann einige kleine Geister der Luft und des Wassers in das Material. "
		"Aus dem so vorbereiteten Leder fertige er nun ein kleines Beutelchen, "
		"welches in ihm aufbewahrte Kräuter besser zu konservieren vermag.",
		NULL,
		NULL,
		M_DRUIDE, (ONSHIPCAST), 5, 5,
		{
		 {R_AURA, 30, SPC_FIX},
		 {R_PERMAURA, 1, SPC_FIX},
		 {R_TREES, 1, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0}},
		(spell_f)sp_create_sack_of_conservation, patzer
	},

	{SPL_SUMMONENT, "Erwecke Ents",
		"Mit Hilfe dieses Zaubers weckt der Druide die in den Wälder der "
		"Region schlummernden Ents aus ihrem äonenlangen Schlaf. Die wilden "
		"Baumwesen werden sich ihm anschließen und ihm beistehen, jedoch "
		"nach einiger Zeit wieder in Schlummer verfallen.",
		NULL,
		NULL,
	 M_DRUIDE, (SPELLLEVEL), 5, 10,
	 {
		 {R_AURA, 6, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_summonent, patzer
	},

	{SPL_GWYRRD_FAMILIAR, "Vertrauten binden",
	  "Einem erfahrenen Druidem wird irgendwann auf seinen Wanderungen ein "
		"ungewöhnliches Exemplar einer Gattung begegnen, welches sich dem "
		"Druiden anschließen wird.",
		NULL,
		NULL,
		M_DRUIDE, (NOTFAMILIARCAST), 5, 10,
		{
			{R_AURA, 100, SPC_FIX},
			{R_PERMAURA, 5, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
	 (spell_f)sp_summon_familiar, patzer
	},

	{SPL_BLESSSTONECIRCLE, "Segne Steinkreis",
		"Dieses Ritual segnet einen Steinkreis, der zuvor aus Steinen und "
		"etwas Holz gebaut werden muss. Die Segnung des Druiden macht aus dem "
		"Kreis eine mächtige Stätte magischen Wirkens.",
		NULL,
		"b",
		M_DRUIDE, (BUILDINGSPELL | ONETARGET), 5, 11,
		{
			{R_AURA, 350, SPC_FIX},
			{R_PERMAURA, 5, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_blessstonecircle, patzer
	},

	{SPL_GWYRRD_ARMORSHIELD, "Rindenhaut",
		"Diese vor dem Kampf zu zaubernde Ritual gibt den eigenen Truppen "
		"einen zusätzlichen Bonus auf ihre Rüstung. Jeder Treffer "
		"reduziert die Kraft des Zaubers, so das der Schild sich irgendwann "
		"im Kampf auflösen wird.",
		NULL,
		NULL,
	 M_DRUIDE, (PRECOMBATSPELL | SPELLLEVEL), 2, 12,
	 {
			{R_AURA, 4, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_armorshield, patzer
	},

	{SPL_DROUGHT, "Beschwörung eines Hitzeelementar",
	"Dieses Ritual beschwört wütende Elementargeister der Hitze. "
	"Eine Dürre sucht das Land heim. Bäume verdorren, Tiere verenden, "
	"und die Ernte fällt aus. Für Tagelöhner gibt es kaum noch Arbeit "
	"in der Landwirtschaft zu finden.",
		NULL,
		NULL,
	 M_DRUIDE, (FARCASTING|REGIONSPELL|TESTRESISTANCE), 5, 13,
	 {
			{R_AURA, 600, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_drought, patzer
	},

	{SPL_FOG_OF_CONFUSION, "Nebel der Verwirrung",
		"Der Druide beschwört die Elementargeister des Nebels. Sie werden sich "
		"für einige Zeit in der Umgebung festsetzen und sie mit dichtem Nebel "
		"überziehen. Personen innerhalb des magischen Nebels verlieren die "
		"Orientierung und haben große Schwierigkeiten, sich in eine bestimmte "
		"Richtung zu bewegen.",
		NULL,
		NULL,
		M_DRUIDE,
		(FARCASTING|SPELLLEVEL),
		5, 14,
		{
			{R_AURA, 8, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_fog_of_confusion, patzer
	},

	{SPL_MAELSTROM, "Mahlstrom",
		"Dieses Ritual beschört einen großen Wasserelementar aus den "
		"Tiefen des Ozeans. Der Elementar erzeugt einen gewaltigen "
		"Strudel, einen Mahlstrom, welcher alle Schiffe, die ihn passieren, "
		"schwer beschädigen kann.",
		NULL,
		NULL,
		M_DRUIDE,
		(OCEANCASTABLE | ONSHIPCAST | REGIONSPELL | TESTRESISTANCE),
		5, 15,
		{
			{R_AURA, 200, SPC_FIX},
			{R_SEASERPENTHEAD, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_maelstrom, patzer
	},

	{SPL_MALLORN, "Wurzeln der Magie",
		"Mit Hilfe dieses aufwändigen Rituals läßt der Druide einen Teil seiner "
		"dauerhaft in den Boden und die Wälder der Region fliessen. Dadurch wird "
		"das Gleichgewicht der Natur in der Region für immer verändert, und in "
		"Zukunft werden nur noch die anspruchsvollen, aber kräftigen "
		"Mallorngewächse in der Region gedeihen.",
		NULL,
		NULL,
		M_DRUIDE,
		(FARCASTING | REGIONSPELL | TESTRESISTANCE),
		5, 16,
		{
			{R_AURA, 250, SPC_FIX},
			{R_PERMAURA, 10, SPC_FIX},
			{R_TOADSLIME, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_mallorn, patzer
	},

	{SPL_GREAT_DROUGHT, "Tor in die Ebene der Hitze",
		"Dieses mächtige Ritual öffnet ein Tor in die Elementarebene der "
		"Hitze. Eine grosse Dürre kommt über das Land. Bauern, Tiere und "
		"Pflanzen der Region kämpfen um das nackte Überleben, aber eine "
		"solche Dürre überlebt wohl nur die Hälfte aller Lebewesen. "
	 	"Der Landstrich kann über Jahre hinaus von den Folgen einer "
	 	"solchen Dürre betroffen sein.",
		NULL,
		NULL,
	 M_DRUIDE,
	 (FARCASTING | REGIONSPELL | TESTRESISTANCE),
	 5, 17,
	 {
			{R_AURA, 800, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_great_drought, patzer
	},

/* M_CHAOS */
	{SPL_SPARKLE_CHAOS, "Verwünschung",
		"Das Ziel des Zauberers wird von einer harmlosen Verwünschung heimgesucht.",
		NULL,
		"u",
	 M_CHAOS, (UNITSPELL | TESTCANSEE | SPELLLEVEL | ONETARGET), 5, 1,
	 {
		{R_AURA, 1, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_sparkle, patzer
	},

	{SPL_DRAIG_EARN_SILVER, "Kleine Flüche",
	"In den dunkleren Gassen gibt es sie, die Flüche und Verhexungen auf "
	"Bestellung. Aber auch Gegenzauber hat der Jünger des Draigs natürlich im "
	"Angebot. Ob nun der Sohn des Nachbarn in einen Liebesbann gezogen werden "
	"soll oder die Nebenbuhlerin Pickel und Warzen bekommen soll, niemand "
	"gibt gerne zu, zu solchen Mitteln gegriffen zu haben.  Für diese "
	"Dienstleistung streicht der Magier 50 Silber pro Stufe ein.",
	NULL,
	NULL,
	 M_CHAOS, (SPELLLEVEL|ONSHIPCAST), 5, 1,
	 {
		{R_AURA, 1, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_earn_silver, patzer
	},

	{SPL_FIREBALL, "Feuerball",
		"Der Zauberer schleudert fokussiertes Chaos in die Reihen der Gegner. "
		"Das ballförmige Chaos wird jeden verwunden, den es trifft.",
		NULL,
		NULL,
	 M_CHAOS, (COMBATSPELL | SPELLLEVEL), 5, 2,
	 {
			{R_AURA, 1, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_kampfzauber, patzer
	},

	{SPL_MAGICBOOST, "Gabe des Chaos",
		"Der Magier öffnet seinen Geist den Sphären des Chaos und wird so "
		"für einige Zeit über mehr magische Kraft verfügen. Doch die Hilfe "
		"der Herren der Sphären hat seinen Preis, und so wird die Phase der "
		"Macht abgelöst von einer Phase der Schwäche.",
		NULL,
		NULL,
		M_CHAOS, (ONSHIPCAST), 3, 3,
		{
			{R_AURA, 2, SPC_LINEAR},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_magicboost, patzer
	},

	{SPL_BLOODSACRIFICE, "kleines Blutopfer",
		"Mit diesem Ritual kann der Magier einen Teil seiner Lebensenergie "
		"opfern, um dafür an magischer Kraft zu gewinnen. Erfahrene "
		"Ritualmagier berichten, das sich das Ritual, einmal initiiert, nur "
		"schlecht steuern ließe und die Menge der so gewonnenen Kraft stark "
		"schwankt. So steht im 'Buch des Blutes' geschrieben: 'So richte Er aus "
		"das Zeichen der vier Elemente im Kreis des Werdens und Vergehens und "
		"Weihe einjedes mit einem Tropfen Blut. Sodann begebe Er in der Mitten "
		"der Ewigen Vierer sich und lasse Leben verrinnen, auf das Kraft "
		"geboren werde.'",
		NULL,
		NULL,
		M_CHAOS, (ONSHIPCAST), 1, 4,
		{
			{R_HITPOINTS, 4, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_bloodsacrifice, patzer
	},

	{SPL_BERSERK, "Blutrausch",
		"In diesem blutigen Ritual opfert der Magier vor der Schlacht ein "
		"Neugeborenes vor den Augen seiner Armee. Die so gerufenen Blutgeister "
		"werden von den Soldaten Besitz ergreifen und sie in einen Blutrausch "
		"versetzen.",
		NULL,
		NULL,
	 M_CHAOS, (PRECOMBATSPELL | SPELLLEVEL), 4, 5,
	 {
			{R_AURA, 5, SPC_LEVEL},
			{R_PEASANTS, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
	 (spell_f)sp_berserk, patzer
	},

	{SPL_FUMBLECURSE, "Chaosfluch",
		"Dieser heimtückische Fluch beeinträchtigt die magischen Fähigkeiten "
		"des Opfers erheblich. Eine chaosmagische Zone um das Opfer vermindert "
		"seine Konzentrationsfähigkeit und macht es ihm sehr schwer Zauber "
		"zu wirken.",
		NULL,
		"u",
		M_CHAOS,
		(UNITSPELL | SPELLLEVEL | ONETARGET | TESTCANSEE | TESTRESISTANCE),
		4, 5,
		{
			{R_AURA, 4, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_fumblecurse, patzer_fumblecurse
	},

	{SPL_SUMMONUNDEAD, "Mächte des Todes",
		"Nächtelang muss der Schwarzmagier durch die Friedhöfe und Gräberfelder "
		"der Region ziehen um dann die ausgegrabenen Leichen beleben zu können. "
		"Die Untoten werden ihm zu Diensten sein, doch sei der Unkundige gewarnt, "
		"dass die Beschwörung der Mächte des Todes ein zweischneidiges Schwert "
		"sein kann.",
		NULL,
		NULL,
		M_CHAOS, (SPELLLEVEL | FARCASTING | ONSHIPCAST),
		5, 6,
		{
			{R_AURA, 5, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_summonundead, patzer_peasantmob
	},
	{SPL_COMBATRUST, "Rosthauch",
		"Mit diesem Ritual wird eine dunkle Gewitterfront "
		"beschworen, die sich unheilverkündend über der Region auftürmt. "
		"Der magische Regen wird alles Erz rosten lassen und so viele "
		"Waffen des Gegners zerstören.",
		NULL,
		NULL,
		M_CHAOS, (COMBATSPELL | SPELLLEVEL), 5, 6,
		{
			{R_AURA, 2, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_combatrosthauch, patzer
	},

	{SPL_TRUESEEING_DRAIG, "Erschaffe ein Amulett des wahren Sehens",
		"Der Spruch ermöglicht es einem Magier, ein Amulett des Wahren Sehens "
		"zu erschaffen. Das Amulett erlaubt es dem Träger, alle Einheiten, die "
		"durch einen Ring der Unsichtbarkeit geschützt sind, zu sehen. Einheiten "
		"allerdings, die sich mit ihrem Tarnungs-Talent verstecken, bleiben "
		"weiterhin unentdeckt.",
		NULL,
		NULL,
		M_CHAOS, (ONSHIPCAST), 5, 6,
		{
			{R_AURA, 50, SPC_FIX},
			{R_SILVER, 3000, SPC_FIX},
			{R_PERMAURA, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_createitem_trueseeing, patzer_createitem
	},

	{SPL_INVISIBILITY_DRAIG, "Erschaffe einen Ring der Unsichtbarkeit",
		"Mit diesem Spruch kann der Zauberer einen Ring der Unsichtbarkeit "
		"erschaffen. Der Träger des Ringes wird für alle Einheiten anderer "
		"Parteien unsichtbar, egal wie gut ihre Wahrnehmung auch sein mag. In "
		"einer unsichtbaren Einheit muss jede Person einen Ring tragen.",
		NULL,
		NULL,
		M_CHAOS, (ONSHIPCAST), 5, 6,
		{
			{R_AURA, 50, SPC_FIX},
			{R_SILVER, 3000, SPC_FIX},
			{R_PERMAURA, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_createitem_invisibility, patzer_createitem
	},

	{SPL_TRANSFERAURA_CHAOS, "Machtübertragung",
	 "Mit Hilfe dieses Zaubers kann der Magier eigene Aura im Verhältnis "
	 "2:1 auf einen anderen Magier des gleichen Magiegebietes übertragen.",
	 "ZAUBERE \"Machtübertragung\" <Einheit-Nr> <investierte Aura>",
		"ui",
		M_CHAOS, (UNITSPELL|ONSHIPCAST|ONETARGET), 1, 7,
		{
			{R_AURA, 2, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_transferaura, patzer
	},

	{SPL_FIREWALL, "Feuerwand",
		"Der Zauberer erschafft eine Wand aus Feuer in der angegebenen Richtung."
		"Sie verletzt jeden, der sie durchschreitet.",
		"ZAUBERE \"Feuerwand\" <Richtung>",
		"c",
		M_CHAOS, (SPELLLEVEL | REGIONSPELL | TESTRESISTANCE), 4, 7,
		{
			{R_AURA, 6, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_firewall, patzer_peasantmob
	},

	{SPL_PLAGUE, "Fluch der Pestilenz",
		"In einem aufwendigen Ritual opfert der Schwarzmagier einige Bauern "
		"und verteilt dann die Leichen auf magische Weise in den Brunnen der "
		"Region.",
		NULL,
		NULL,
		M_CHAOS,
		(FARCASTING | REGIONSPELL | TESTRESISTANCE),
		5, 7,
		{
			{R_AURA, 30, SPC_FIX},
		  {R_PEASANTS, 250, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
	 (spell_f)sp_plague, patzer_peasantmob
	},

	{SPL_CHAOSROW, "Wahnsinn des Krieges",
		"Vor den Augen der feindlichen Soldaten opfert der Schwarzmagier die "
		"zehn Bauern in einem blutigen, grausamen Ritual und beschwört auf "
		"diese Weise Geister des Wahnsinns über die feindlichen Truppen. "
		"Diese werden im Kampf verwirrt reagieren und nicht in der Lage "
		"sein, den Anweisungen ihrer Offiziere zu folgen.",
		NULL,
		NULL,
		M_CHAOS, (PRECOMBATSPELL | SPELLLEVEL), 5, 8,
		{
			{R_AURA, 3, SPC_LEVEL},
			{R_PEASANTS, 10, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_chaosrow, patzer
	},

	{SPL_SUMMONSHADOW, "Beschwöre Schattendämonen",
	 "Mit Hilfe dunkler Rituale beschwört der Zauberer Dämonen aus "
	 "der Sphäre der Schatten. Diese gefürchteten Wesen können sich "
	 "fast unsichtbar unter den Lebenden bewegen, ihre finstere Aura "
	 "ist jedoch für jeden spürbar. Im Kampf sind Schattendämonen "
	 "gefürchtete Gegner. Sie sind schwer zu treffen und entziehen ihrem "
	 "Gegner Kraft.",
		NULL,
		NULL,
		M_CHAOS, (SPELLLEVEL), 5, 8,
		{
			{R_AURA, 3, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_summonshadow, patzer_peasantmob
	},

	{SPL_UNDEADHERO, "Untote Helden",
		"Dieses Ritual bindet die bereits entfliehenden Seelen einiger "
		"Kampfopfer an ihren toten Körper, wodurch sie zu untoten Leben "
		"wiedererweckt werden. Ob sie ehemals auf der Seite des Feindes "
		"oder der eigenen kämpften, ist für das Ritual ohne belang.",
		NULL,
		NULL,
		M_CHAOS, (POSTCOMBATSPELL | SPELLLEVEL), 5, 9,
		{
			{R_AURA, 1, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_undeadhero, patzer
	},

	{SPL_STRENGTH, "Erschaffe einen Gürtel der Trollstärke",
		"Dieses magische Artefakt verleiht dem Träger die Stärke eines "
		"ausgewachsenen Höllentrolls. Seine Tragkraft erhöht sich auf "
		"das 50fache und auch im Kampf werden sich die erhöhte Kraft und "
		"die trollisch zähe Haut positiv auswirken.",
		NULL,
		NULL,
	 M_CHAOS, (ONSHIPCAST), 5, 9,
	 {
			{R_AURA, 20, SPC_FIX},
		 {R_PERMAURA, 1, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	(spell_f)sp_create_trollbelt, patzer
	},

	{SPL_AURALEAK, "Astraler Riss",
		"Der Schwarzmagier kann mit diesem dunklen Ritual einen Riss in das "
		"Gefüge der Magie bewirken, der alle magische Kraft aus der Region "
		"reißen wird. Alle magisch begabten in der Region werden einen "
		"Großteil ihrer Aura verlieren.",
		NULL,
		NULL,
		M_CHAOS, (REGIONSPELL | TESTRESISTANCE), 3, 9,
		{
			{R_AURA, 35, SPC_FIX},
			{R_DRACHENBLUT, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_auraleak, patzer
	},

	{SPL_DRAIG_FUMBLESHIELD, "Astrales Chaos",
		"Dieses Ritual, ausgeführt vor einem Kampf, verwirbelt die astralen "
		"Energien auf dem Schlachtfeld und macht es so feindlichen Magier "
		"schwieriger, ihre Zauber zu wirken.",
		NULL,
		NULL,
	 M_CHAOS, (PRECOMBATSPELL | SPELLLEVEL), 2, 9,
	 {
			{R_AURA, 6, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_fumbleshield, patzer
	},

	{SPL_FOREST_FIRE, "Feuerteufel",
		"Diese Elementarbeschwörung ruft einen Feuerteufel herbei, ein Wesen "
		"aus den tiefsten Niederungen der Flammenhöllen. Der Feuerteufel "
		"wird sich begierig auf die Wälder der Region stürzen und sie in "
		"Flammen setzen.",
		NULL,
		NULL,
		M_CHAOS, (FARCASTING | REGIONSPELL | TESTRESISTANCE), 5, 10,
		{
			{R_AURA, 50, SPC_FIX},
			{R_OIL, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_forest_fire, patzer_peasantmob
	},

	{SPL_DRAIG_DESTROY_MAGIC, "Pentagramm",
		"Genau um Mitternacht, wenn die Kräfte der Finsternis am größten sind, "
		"kann auch ein Schwarzmagier seine Kräfte nutzen um Verzauberungen "
		"aufzuheben. Dazu zeichnet er ein Pentagramm in das verzauberte Objekt "
		"und beginnt mit einer Anrufung der Herren der Finsternis. Die Herren "
		"werden ihm beistehen, doch ob es ihm gelingt, den Zauber zu lösen, "
		"hängt allein von seiner eigenen Kraft ab.",
		"ZAUBERE [REGION x y] [STUFE n] \"Pentagramm\" REGION\n"
		"ZAUBERE [REGION x y] [STUFE n] \"Pentagramm\" EINHEIT <Einheit-Nr>\n"
		"ZAUBERE [REGION x y] [STUFE n] \"Pentagramm\" BURG <Burg-Nr>\n"
		"ZAUBERE [REGION x y] [STUFE n] \"Pentagramm\" GEBÄUDE <Gebäude-Nr>\n"
		"ZAUBERE [REGION x y] [STUFE n] \"Pentagramm\" SCHIFF <Schiff-Nr>",
		"kc",
		M_CHAOS,
		(FARCASTING | SPELLLEVEL | ONSHIPCAST | ONETARGET | TESTCANSEE),
		2, 10,
		{
			{R_AURA, 10, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_destroy_magic, patzer
	},

	{SPL_UNHOLYPOWER, "Unheilige Kraft",
		"Nur geflüstert wird dieses Ritual an den dunklen Akademien an die "
		"Adepten weitergegeben, gehört es doch zu den finstersten, die je "
		"niedergeschrieben wurden. Durch die Anrufung unheiliger Dämonen "
		"wird die Kraft der lebenden Toten verstärkt und sie verwandeln "
		"sich in untote Monster großer Kraft.",
		NULL,
		"u+",
		M_CHAOS, (UNITSPELL | SPELLLEVEL | TESTCANSEE), 5, 10,
		{
			{R_AURA, 8, SPC_LEVEL},
			{R_PEASANTS, 50, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_unholypower, patzer
	},

	{SPL_DEATHCLOUD, "Todeswolke",
		"Mit einem düsteren Ritual und unter Opferung seines eigenen Blutes "
		"beschwört der Schwarzmagier einen großen Geist von der Elementarebene "
		"der Gifte. Der Geist manifestiert sich als giftgrüner Schwaden über "
		"der Region und wird allen, die mit ihm in Kontakt kommen, Schaden "
		"zufügen.",
		NULL,
		NULL,
		M_CHAOS, (FARCASTING | REGIONSPELL | TESTRESISTANCE), 5, 11,
		{
			{R_AURA, 40, SPC_FIX},
			{R_HITPOINTS, 4, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_deathcloud, patzer_peasantmob
	},

	{SPL_SUMMONDRAGON, "Drachenruf",
		"Mit diesem dunklen Ritual erzeugt der Magier einen Köder, der "
		"für Drachen einfach unwiderstehlich riecht. Ob die Drachen aus der "
		"Umgebung oder aus der Sphäre des Chaos stammen, konnte noch nicht "
		"erforscht werden. Es soll beides bereits vorgekommen sein. "
		"Der Köder hält etwa 6 Wochen, muss aber in einem drachengenehmen "
		"Terrain plaziert werden.",
		NULL,
		NULL,
		M_CHAOS, (FARCASTING | REGIONSPELL | TESTRESISTANCE), 5, 11,
	 {
			{R_AURA, 80, SPC_FIX},
		 {R_DRAGONHEAD, 1, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_summondragon, patzer_peasantmob
	},

	{SPL_SUMMONSHADOWLORDS, "Beschwöre Schattenmeister",
	 "Mit Hilfe dunkler Rituale beschwört der Zauberer Dämonen aus "
	 "der Sphäre der Schatten. Diese gefürchteten Wesen können sich "
	 "fast unsichtbar unter den Lebenden bewegen, ihre finstere Aura "
	 "ist jedoch für jeden spürbar. Im Kampf sind Schattenmeister "
	 "gefürchtete Gegner. Sie sind schwer zu treffen und entziehen ihrem "
	 "Gegner Kraft und Leben.",
		NULL,
		NULL,
	 M_CHAOS, (SPELLLEVEL), 5, 12,
	 {
			{R_AURA, 7, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_summonshadowlords, patzer_peasantmob
	},

	{SPL_FIRESWORD, "Erschaffe ein Flammenschwert",
		"'Und so reibe das Blut eines wilden Kämpfers in den Stahl der Klinge "
		"und beginne die Anrufung der Sphären des Chaos. Und hast du alles "
		"zu ihrem Wohlgefallen getan, so werden sie einen niederen der ihren "
		"senden, das Schwert mit seiner Macht zu beseelen...'",
		NULL,
		NULL,
		M_CHAOS, (ONSHIPCAST), 5, 12,
		{
			{R_AURA, 100, SPC_FIX},
			{R_BERSERK, 1, SPC_FIX},
			{R_SWORD, 1, SPC_FIX},
			{R_PERMAURA, 1, SPC_FIX},
			{0, 0, 0}},
		(spell_f)sp_create_firesword, patzer
	},

	{SPL_DRAIG_FAMILIAR, "Vertrauten rufen",
		"Einem erfahrenen Magier wird irgendwann auf seinen Wanderungen ein "
		"ungewöhnliches Exemplar einer Gattung begegnen, welches sich dem "
		"Magier anschließen wird.",
		NULL,
		NULL,
		M_CHAOS, (NOTFAMILIARCAST), 5, 13,
		{
			{R_AURA, 100, SPC_FIX},
		 {R_PERMAURA, 5, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
		(spell_f)sp_summon_familiar, patzer
	},

	{SPL_CHAOSSUCTION, "Chaossog",
		"Durch das Opfern von 500 Bauern kann der Chaosmagier ein Tor zur "
		"astralen Welt öffnen. Das Tor kann in der Folgewoche verwendet werden, "
		"es löst sich am Ende der Folgewoche auf.",
		NULL,
		NULL,
		M_CHAOS, (0), 5, 14,
		{
			{R_AURA, 150, SPC_FIX},
		 {R_PEASANTS, 500, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_chaossuction, patzer_peasantmob
	},

/* M_TRAUM */

	{SPL_SPARKLE_DREAM, "Traumsenden",
		"Der Zauberer sendet dem Ziel des Spruches einen Traum.",
		NULL,
		"u",
		M_TRAUM,
		(UNITSPELL | TESTCANSEE | SPELLLEVEL | ONETARGET | ONSHIPCAST),
		5, 1,
	 {
		{R_AURA, 1, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_sparkle, patzer
	},

	{SPL_ILLAUN_EARN_SILVER, "Wahrsagen",
	"Niemand kann so gut die Träume deuten wie ein Magier des Illaun. Auch "
	"die Kunst der Wahrsagerrei, des Kartenlegens und des Handlesens sind ihm "
	"geläufig.  Dafür zahlen ihm die Bauern 50 Silber pro Stufe.",
	NULL,
	NULL,
	 M_TRAUM, (SPELLLEVEL|ONSHIPCAST), 5, 1,
	 {
		{R_AURA, 1, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_earn_silver, patzer
	},

	{SPL_SHADOWKNIGHTS, "Schattenritter",
		"Dieser Zauber vermag dem Gegner ein geringfügig versetztes Bild der "
		"eigenen Truppen vorzuspiegeln. Die Schattenritter haben keinen "
		"effektiven Angriff und Verwundungen im Kampf zerstören sie sofort.",
		NULL,
		NULL,
	 M_TRAUM, (PRECOMBATSPELL | SPELLLEVEL), 4, 1,
	 {
		 {R_AURA, 1, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_shadowknights, patzer
	},

	{SPL_FLEE, "Grauen der Schlacht",
		"Der Traumweber beschwört vor dem Kampf grauenerregende Trugbilder herauf, "
		"die viele Gegner in Panik versetzen. Die Betroffenen werden versuchen, "
		"vor den Trugbildern zu fliehen.",
		NULL,
		NULL,
	 M_TRAUM, (PRECOMBATSPELL | SPELLLEVEL), 5, 2,
	 {
			{R_AURA, 1, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_flee, patzer
	},

	{SPL_PUTTOREST, "Seelenfrieden",
		"Dieses magische Ritual beruhigt die gequälten Seelen der gewaltsam "
		"zu Tode gekommenen und ermöglicht es ihnen so, ihre letzte Reise in "
		"die Anderlande zu beginnen. Je Stufe des Zaubers werden ungefähr 50 "
		"Seelen ihre Ruhe finden. Der Zauber vermag nicht, bereits wieder "
		"auferstandene lebende Tote zu erlösen, da deren Bindung an diese "
		"Welt zu stark ist.",
		NULL,
		NULL,
	 M_TRAUM, (SPELLLEVEL), 5, 2,
	 {
		 {R_AURA, 3, SPC_LEVEL},
		 {R_TREES, 1, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_puttorest, patzer
	},

	{SPL_ICASTLE, "Traumschlößchen",
		"Mit Hilfe dieses Zaubers kann der Traumweber die Illusion eines "
		"beliebigen Gebäudes erzeugen. Die Illusion kann betreten werden, ist "
		"aber ansonsten funktionslos und benötigt auch keinen Unterhalt. Sie "
		"wird einige Wochen bestehen bleiben.",
		"ZAUBERE \"Traumschlößchen\" <Gebäude-Typ>",
		"c",
	 M_TRAUM, (0), 5, 3,
	 {
		 {R_AURA, 1, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_icastle, patzer
	},

	{SPL_TRANSFERAURA_TRAUM, "Traum der Magie",
	 "Mit Hilfe dieses Zaubers kann der Traumweber eigene Aura im Verhältnis "
	 "2:1 auf einen anderen Traumweber übertragen.",
		"ZAUBERE \"Traum der Magie\" <Einheit-Nr> <investierte Aura>",
		"ui",
	 M_TRAUM, (UNITSPELL|ONSHIPCAST|ONETARGET), 1, 3,
	 {
			{R_AURA, 2, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_transferaura, patzer
	},

	{SPL_ILL_SHAPESHIFT, "Gestaltwandlung",
		"Mit Hilfe dieses arkanen Rituals vermag der Traumweber die wahre "
		"Gestalt einer Gruppe zu verschleiern. Unbedarften Beobachtern "
		"erscheint sie dann als einer anderen Rasse zugehörig.",
		"ZAUBERE [STUFE n] \"Gestaltwandlung\" <Einheit-nr> <Rasse>",
		"uc",
	 M_TRAUM, (UNITSPELL|SPELLLEVEL|ONETARGET), 5, 3,
	 {
			{R_AURA, 1, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_illusionary_shapeshift, patzer
	},

	{SPL_DREAMREADING, "Traumlesen",
	 "Dieser Zauber ermöglicht es dem Traumweber, in die Träume einer Einheit "
	 "einzudringen und so einen Bericht über die Umgebung zu erhalten.",
		NULL,
		"u",
	 M_TRAUM, (FARCASTING | UNITSPELL | ONETARGET | TESTRESISTANCE), 5, 4,
	 {
			{R_AURA, 8, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_dreamreading, patzer
	},

	{SPL_TIREDSOLDIERS, "Schwere Glieder",
	 "Dieser Kampfzauber führt dazu, dass einige Gegner im Kampf unter "
	 "schwerer Müdigkeit leiden. Die Soldaten verschlafen manchmal ihren "
	 "Angriff und verteidigen sich schlechter.",
		NULL,
		NULL,
	 M_TRAUM, (PRECOMBATSPELL | SPELLLEVEL), 5, 4,
	 {
			{R_AURA, 4, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_tiredsoldiers, patzer
	},

	{SPL_REANIMATE, "Wiederbelebung",
		"Stirbt ein Krieger im Kampf so macht sich seine Seele auf die lange "
		"Wanderung zu den Sternen. Mit Hilfe eines Rituals kann ein Traumweber "
		"versuchen, die Seele wieder einzufangen und in den Körper des "
		"Verstorbenen zurückzubringen. Zwar heilt der Zauber keine "
		"körperlichen Verwundungen, doch ein Behandelter wird den Kampf "
		"überleben.",
		NULL,
		NULL,
	 M_TRAUM, (POSTCOMBATSPELL | SPELLLEVEL), 4, 5,
	 {
			{R_AURA, 1, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_reanimate, patzer
	},

	{SPL_ANALYSEDREAM, "Traumbilder analysieren",
		"Mit diesem Spruch kann der Traumweber versuchen, die Verzauberungen "
		"einer einzelnen Einheit zu erkennen. Von allen Sprüchen, "
		"die seine eigenen Fähigkeiten nicht überschreiten, wird er einen "
		"Eindruck ihres Wirkens erhalten können. Bei stärkeren Sprüchen "
		"benötigt er ein wenig Glück für eine gelungene Analyse.",
		NULL,
		"u",
	 M_TRAUM, (UNITSPELL | ONSHIPCAST | ONETARGET | TESTCANSEE), 5, 5,
	 {
			{R_AURA, 5, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_analysedream, patzer
	},

	{SPL_DISTURBINGDREAMS, "Schlechter Schlaf",
	 "Dieser Zauber führt in der betroffenen Region für einige Wochen zu "
	 "Schlaflosigkeit und Unruhe. Den Betroffenen fällt das Lernen deutlich "
	 "schwerer.",
		NULL,
		NULL,
	 M_TRAUM, (FARCASTING | REGIONSPELL | TESTRESISTANCE), 5, 6,
	 {
			{R_AURA, 18, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_disturbingdreams, patzer
	},

	{SPL_TRUESEEING_ILLAUN, "Erschaffe ein Amulett des wahren Sehens",
		"Der Spruch ermöglicht es einem Magier, ein Amulett des Wahren Sehens "
		"zu erschaffen. Das Amulett erlaubt es dem Träger, alle Einheiten, die "
		"durch einen Ring der Unsichtbarkeit geschützt sind, zu sehen. Einheiten "
		"allerdings, die sich mit ihrem Tarnungs-Talent verstecken, bleiben "
		"weiterhin unentdeckt.",
		NULL,
		NULL,
		M_TRAUM, (ONSHIPCAST), 5, 6,
		{
			{R_AURA, 50, SPC_FIX},
			{R_SILVER, 3000, SPC_FIX},
			{R_PERMAURA, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_createitem_trueseeing, patzer_createitem
	},

	{SPL_INVISIBILITY_ILLAUN, "Erschaffe einen Ring der Unsichtbarkeit",
		"Mit diesem Spruch kann der Zauberer einen Ring der Unsichtbarkeit "
		"erschaffen. Der Träger des Ringes wird für alle Einheiten anderer "
		"Parteien unsichtbar, egal wie gut ihre Wahrnehmung auch sein mag. In "
		"einer unsichtbaren Einheit muss jede Person einen Ring tragen.",
		NULL,
		NULL,
		M_TRAUM, (ONSHIPCAST), 5, 6,
		{
			{R_AURA, 50, SPC_FIX},
			{R_SILVER, 3000, SPC_FIX},
			{R_PERMAURA, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_createitem_invisibility, patzer_createitem
	},

	{SPL_SLEEP, "Schlaf",
	 "Dieser Zauber läßt einige feindliche Kämpfer einschlafen. Schlafende "
	 "Kämpfer greifen nicht an und verteidigen sich schlechter, sie wachen "
	 "jedoch auf, sobald sie im Kampf getroffen werden.",
		NULL,
		NULL,
	 M_TRAUM, (COMBATSPELL | SPELLLEVEL ), 5, 7,
	 {
			{R_AURA, 1, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_sleep, patzer
	},

	{SPL_WISPS, "Irrlichter",
		"Der Zauberer spricht eine Beschwörung über einen Teil der Region, "
		"und in der Folge entstehn dort Irrlichter. "
		"Wer durch diese Nebel wandert, wird von Visionen geplagt und "
		"in die Irre geleitet.",
		"ZAUBERE [REGION x y] [STUFE n] \"Irrlichter\" <Richtung>",
		"c",
	 M_TRAUM, (SPELLLEVEL | FARCASTING), 5, 7,
	 {
		 {R_AURA, 2, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_wisps, patzer
	},

	{SPL_READMIND, "Traumdeuten",
		"Mit diesem Zauber dringt der Traumweber in die Gedanken und Traumwelt "
		"seines Opfers ein und kann so seine intimsten Geheimnisse ausspähen. "
		"Seine Fähigkeiten, seinen Besitz und seine Parteizugehörigkeit wird "
		"nicht länger ungewiss sein.",
		NULL,
		"u",
	 M_TRAUM, (UNITSPELL | ONETARGET), 5, 7,
	 {
			{R_AURA, 20, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_readmind, patzer
	},

	{SPL_GOODDREAMS, "Schöne Träume",
	 "Dieser Zauber ermöglicht es dem Traumweber, den Schlaf aller aliierten "
	 "Einheiten in der Region so zu beeinflussen, dass sie für einige Zeit "
	 "einen Bonus in allen Talenten bekommen.",
		NULL,
		NULL,
		M_TRAUM,
		(FARCASTING | REGIONSPELL | TESTRESISTANCE),
		5, 8,
		{
			{R_AURA, 80, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_gooddreams, patzer
	},

	{SPL_ILLAUN_DESTROY_MAGIC, "Traumbilder entwirren",
		"Dieser Zauber ermöglicht es dem Traumweber die natürlichen und "
		"aufgezwungenen Traumbilder einer Person, eines Gebäudes, Schiffes "
		"oder einer Region zu unterscheiden und diese zu entwirren.",
		"ZAUBERE [REGION x y] [STUFE n] \"Traumbilder entwirren\" REGION\n"
		"ZAUBERE [REGION x y] [STUFE n] \"Traumbilder entwirren\" EINHEIT <Einheit-Nr>\n"
		"ZAUBERE [REGION x y] [STUFE n] \"Traumbilder entwirren\" BURG <Burg-Nr>\n"
		"ZAUBERE [REGION x y] [STUFE n] \"Traumbilder entwirren\" GEBÄUDE <Gebäude-Nr>\n"
		"ZAUBERE [REGION x y] [STUFE n] \"Traumbilder entwirren\" SCHIFF <Schiff-Nr>",
		"kc",
		M_TRAUM,
		(FARCASTING | SPELLLEVEL | ONSHIPCAST | ONETARGET | TESTCANSEE),
		2, 8,
		{
			{R_AURA, 6, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_destroy_magic, patzer
	},

	{SPL_ILLAUN_FAMILIAR, "Vertrauten rufen",
		"Einem erfahrenen Magier wird irgendwann auf seinen Wanderungen ein "
		"ungewöhnliches Exemplar einer Gattung begegnen, welches sich dem "
		"Magier anschließen wird.",
		NULL,
		NULL,
	 M_TRAUM, (NOTFAMILIARCAST), 5, 9,
	 {
			{R_AURA, 100, SPC_FIX},
			{R_PERMAURA, 5, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
	 (spell_f)sp_summon_familiar, patzer
	},
	
	{SPL_CLONECOPY, "Seelenkopie",
		"Dieser mächtige Zauber kann einen Magier vor dem sicheren Tod "
		"bewahren. Der Magier erschafft anhand einer kleinen Blutprobe einen "
		"Klon von sich, und legt diesen in ein Bad aus Drachenblut und verdünntem "
		"Wasser des Lebens. "
		"Anschließend transferiert er in einem aufwändigen Ritual einen Teil "
		"seiner Seele in den Klon. Stirbt der Magier, reist seine Seele in den "
		"Klon und der erschaffene Körper dient nun dem Magier als neues Gefäß. "
		"Es besteht allerdings eine geringer Wahrscheinlichkeit, dass die Seele "
		"nach dem Tod zu schwach ist, das neue Gefäß zu erreichen.",
		NULL,
		NULL,
	 M_TRAUM, (NOTFAMILIARCAST), 5, 9,
	 {
			{R_AURA, 100, SPC_FIX},
			{R_PERMAURA, 20, SPC_FIX},
			{R_DRACHENBLUT, 5, SPC_FIX},
		  {R_TREES, 5, SPC_FIX},
			{0, 0, 0}},
	 (spell_f)sp_clonecopy, patzer
	},

	{SPL_BADDREAMS, "Schlechte Träume",
	 "Dieser Zauber ermöglicht es dem Träumer, den Schlaf aller nichtaliierten "
	 "Einheiten (HELFE BEWACHE) in der Region so stark zu stören, das sie "
	 "vorübergehend einen Teil ihrer Erinnerungen verlieren.",
		NULL,
		NULL,
		M_TRAUM,
		(FARCASTING | REGIONSPELL | TESTRESISTANCE), 5, 10,
		{
			{R_AURA, 90, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
		(spell_f)sp_baddreams, patzer
	},

	{SPL_MINDBLAST, "Tod des Geistes",
	 "Mit diesem Zauber greift der Magier direkt den Geist seiner Gegner "
	 "an. Ein Schlag aus astraler und elektrischer Energie trifft die "
	 "Gegner, wird die Magieresistenz durchbrochen, verliert ein Opfer "
	 "permanent einen Teil seiner Erinnerungen. Wird es zu oft ein Opfer "
	 "dieses Zaubers kann es daran sterben.",
		NULL,
		NULL,
	 M_TRAUM, (COMBATSPELL | SPELLLEVEL), 5, 11,
	 {
			{R_AURA, 2, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_mindblast, patzer
	},

	{SPL_ORKDREAM, "Süße Träume",
		"Dieser Zauber - dessen Anwendung in den meisten Kulturen streng "
		"verboten ist - löst im Opfer ein unkontrollierbares Verlangen "
		"nach körperlicher Liebe aus. Die betroffenen Personen werden sich "
		"Hals über Kopf in ein Liebesabenteuer stürzen, zu blind vor "
		"Verlangen, um an etwas anderes zu denken. Meistens bereuen sie "
		"es einige Wochen später...",
		NULL,
		"u+",
		M_TRAUM,
		(UNITSPELL | TESTRESISTANCE | TESTCANSEE | SPELLLEVEL), 5, 12,
		{
			{R_AURA, 5, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_sweetdreams, patzer
	},

	{SPL_CREATE_TACTICCRYSTAL, "Erschaffe ein Traumauge",
		"Ein mit diesem Zauber belegtes Drachenauge, welches zum Abendmahle "
		"verzehrt wird, erlaubt es dem Benutzer, in die Träume einer anderen "
		"Person einzudringen und diese zu lesen. Lange Zeit wurde eine solche "
		"Fähigkeit für nutzlos erachtet, bis die ehemalige waldelfische "
		"Magistra für Kampfmagie, Liarana Sonnentau von der Akademie Thall, "
		"eine besondere Anwendung vorstellte: Feldherren träumen vor großen "
		"Kämpfen oft unruhig und verraten im Traum ihre Pläne. Dies kann dem "
		"Anwender einen großen Vorteil im kommenden Kampf geben. Aber Vorsicht: "
		"Die Interpretation von Träumen ist eine schwierige Angelegenheit.",
		NULL,
		NULL,
		M_TRAUM, (ONSHIPCAST), 5, 14,
		{
			{R_PERMAURA, 5, SPC_FIX},
			{R_DRAGONHEAD, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
	 (spell_f)sp_create_tacticcrystal, patzer_createitem
	},

	{SPL_SUMMON_ALP, "Alp",
		"Der Magier beschwört ein kleines Monster, einen Alp.  Dieses bewegt sich "
		"langsam auf sein Opfer zu (das sich an einem beliebigen Ort an der Welt "
		"befinden kann, der Magier oder seine Partei braucht es nicht zu sehen). "
		"Sobald das Opfer erreicht ist, wird es gnadenlos gequält, und nur durch "
		"einen starken Gegenzauber oder den Tod des beschwörenden Magiers kann "
		"das Opfer wieder Frieden finden. Bei der Beschwörung des Alps verliert "
		"der Magier einen kleinen Teil seiner Aura für immer.",
		NULL,
		"u",
		M_TRAUM,
		(UNITSPELL | ONETARGET | SEARCHGLOBAL | TESTRESISTANCE),
		5, 15,
		{
			{R_AURA, 350, SPC_FIX},
			{R_PERMAURA, 5, SPC_FIX},
			{R_SWAMP_3, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_summon_alp, patzer
	},

	{SPL_DREAM_OF_CONFUSION, "Schleier der Verwirrung",
		"Der Magier erzeugt in allen Wald- und Ozeanregionen im Umkreis von "
		"einigen Regionen einen magischen Schleier, der einige Wochen bestehen "
		"bleibt. Personen innerhalb des Schleiers verlieren "
		"leicht die Orientierung und haben große Schwierigkeiten, sich in "
		"eine bestimmte Richtung zu bewegen.",
		NULL,
		NULL,
		M_TRAUM,
		(FARCASTING | SPELLLEVEL),
		5, 16,
	 {
			{R_AURA, 7, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_dream_of_confusion, patzer
	},

/* M_BARDE */

	{SPL_DENYATTACK, "Friedenslied",
	 "Dieses Lied zähmt selbst den wildesten Ork und macht ihn friedfertig "
	 "und sanftmütig. Jeder Gedanke, dem Sänger zu schaden, wird ihm "
	 "entfallen. Unbehelligt kann der Magier in eine Nachbarregion ziehen.",
		NULL,
		NULL,
	 M_BARDE, (PRECOMBATSPELL | SPELLLEVEL ), 5, 1,
	 {
			{R_AURA, 2, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_denyattack, patzer
	},

	{SPL_CERDDOR_EARN_SILVER, "Gaukeleien",
	"Cerddormagier sind _die_ Gaukler unter den Magiern, sie lieben es das "
	"Volk zu unterhalten und im Mittelpunkt zu stehen. Schon Anfänger lernen "
	"die kleinen Kunststücke und magischen Tricks, mit denen man das Volk "
	"locken und verführen kann, den Geldbeutel ganz weit zu öffnen, und "
	"am Ende der Woche wird der Gaukler 50 Silber pro Stufe verdient haben.",
		NULL,
		NULL,
	 M_BARDE, (SPELLLEVEL|ONSHIPCAST), 5, 1,
	 {
			{R_AURA, 1, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_earn_silver, patzer
	},

	{SPL_HEALINGSONG, "Lied der Heilung",
		"Nicht nur der Feldscher kann den Verwundeten einer Schlacht helfen. "
		"Die Barden kennen verschiedene Lieder, die die Selbstheilungskräfte "
		"des Körpers unterstützen. Dieses Lied vermag Wunden zu schließen, "
		"gebrochene Knochen zu richten und selbst abgetrennte Glieder wieder "
		"zu regenerieren.",
		NULL,
		NULL,
	 M_BARDE, (POSTCOMBATSPELL | SPELLLEVEL), 5, 2,
	 {
			{R_AURA, 1, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_healing, patzer
	},

	{SPL_GENEROUS, "Hohes Lied der Gaukelei",
		"Dieser fröhliche Gesang wird sich wie ein Gerücht in der Region "
		"ausbreiten und alle Welt in Feierlaune versetzten. Überall werden "
		"Tavernen und Theater gut gefüllt sein und selbst die Bettler satt "
		"werden.",
		NULL,
		NULL,
		M_BARDE,
		(FARCASTING | SPELLLEVEL | ONSHIPCAST | REGIONSPELL | TESTRESISTANCE),
		5, 2,
	 {
			{R_AURA, 2, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_generous, patzer
	},

	{SPL_RAINDANCE, "Regentanz",
		"Dieses uralte Tanzritual ruft die Kräfte des Lebens und der "
		"Fruchtbarkeit. Die Erträge der Bauern werden für einige Wochen deutlich "
		"besser ausfallen.",
		NULL,
		NULL,
		M_BARDE,
		(FARCASTING | SPELLLEVEL | ONSHIPCAST | REGIONSPELL),
		5, 3,
	 {
			{R_AURA, 1, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_blessedharvest, patzer
	},

	{SPL_SONG_OF_FEAR, "Gesang der Furcht",
		"Ein gar machtvoller Gesang aus den Überlieferungen der Katzen, der "
		"tief in die Herzen der Feinde dringt und ihnen Mut und Hoffnung "
		"raubt. Furcht wird sie zittern lassen und Panik ihre Gedanken "
		"beherrschen. Voller Angst werden sie versuchen, den gräßlichen "
		"Gesängen zu entrinnen und fliehen.",
		NULL,
		NULL,
		M_BARDE, (COMBATSPELL | SPELLLEVEL), 5, 3,
		{
			{R_AURA, 1, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
	 (spell_f)sp_flee, patzer
	},

	{SPL_RECRUIT, "Gesang des Werbens",
		"Aus 'Die Gesänge der Alten' von Firudin dem Weisen: "
		"'Diese verführerische kleine Melodie und einige einschmeichelnde "
		"Worte überwinden das Misstrauen der Bauern im Nu. Begeistert werden "
		"sie sich Euch anschliessen und selbst Haus und Hof in Stich lassen.'",
		NULL,
		NULL,
		M_BARDE, (SPELLLEVEL), 5, 4,
		{
			{R_AURA, 2, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_recruit, patzer
	},

	{SPL_SONG_OF_CONFUSION, "Gesang der Verwirrung",
		"Aus den uralten Gesängen der Katzen entstammt dieses magisches Lied, "
		"welches vor einem Kampfe eingesetzt, einem entscheidende strategische "
		"Vorteile bringen kann. Wer unter den Einfluss dieses Gesangs gelangt, "
		"der wird seiner Umgebung nicht achtend der Melodie folgen, sein Geist "
		"wird verwirrt und sprunghaft plötzlichen Eingebungen nachgeben. So "
		"sollen schon einst wohlgeordnete Heere plötzlich ihre Schützen "
		"weit vorne und ihre Kavallerie bei den Lagerwachen kartenspielend "
		"wiedergefunden haben (oder ihren Anführer schlafend im lange "
		"verlassenen Lager, wie es in den Großen Kriegen der Alten Welt "
		"wirklich geschehen sein soll).",
		NULL,
		NULL,
		M_BARDE, (PRECOMBATSPELL | SPELLLEVEL), 5, 4,
		{
			{R_AURA, 2, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
			(spell_f)sp_chaosrow, patzer
	},

	{SPL_BABBLER, "Plappermaul",
		"Die verzauberte Einheit beginnt hemmungslos zu plappern und "
		"erzählt welche Talente sie kann, was für Gegenstände sie mit sich "
		"führt und sollte sie magisch begabt sein, sogar welche Zauber "
		"sie beherrscht. Leider beeinflußt dieser Zauber nicht das Gedächnis, "
		"und so wird sie sich im nachhinein wohl bewußt werden, dass sie "
		"zuviel erzählt hat.",
		NULL,
		"u",
	 M_BARDE, (UNITSPELL | ONETARGET | TESTCANSEE), 5, 4,
	 {
			{R_AURA, 10, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_babbler, patzer
	},

	{SPL_HERO, "Heldengesang",
		"Dieser alte Schlachtengesang hebt die Moral der eigenen Truppen "
		"und und hilft ihnen auch der angsteinflößenden Aura dämonischer und "
		"untoter Wesen zu widerstehen. Ein derartig gefestigter Krieger wird "
		"auch in schwierigen Situationen nicht die Flucht ergreifen und sein "
		"überlegtes Verhalten wird ihm manch Vorteil in der Verteidigung geben.",
		NULL,
		NULL,
	 M_BARDE, (PRECOMBATSPELL | SPELLLEVEL), 4, 5,
	 {
			{R_AURA, 2, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_hero, patzer
	},

	{SPL_TRANSFERAURA_BARDE, "Gesang des Auratransfers",
	 "Mit Hilfe dieses Zaubers kann der Magier eigene Aura im Verhältnis "
	 "2:1 auf einen anderen Magier des gleichen Magiegebietes übertragen.",
		"ZAUBERE \"Gesang des Auratransfers\" <Einheit-Nr> <investierte Aura>",
		"ui",
	 M_BARDE, (UNITSPELL|ONSHIPCAST|ONETARGET), 1, 5,
	 {
			{R_AURA, 2, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_transferaura, patzer
	},

	{SPL_UNIT_ANALYSESONG, "Gesang des Lebens analysieren",
		"Alle lebenden Wesen haben ein eigenes individuelles Lebenslied. "
		"Nicht zwei Lieder gleichen sich, auch wenn sich alle Lieder einer "
		"Art ähneln. Jeder Zauber verändert dieses Lied auf die eine oder "
		"andere Art und gibt sich damit zu erkennen. Dieser Gesang hilft, "
		"jene Veränderungen im Lebenslied einer Person zu erlauschen, "
		"welche magischer Natur sind. Alle Verzauberungen, die nicht stärker "
		"maskiert sind als Eure Fähigkeit, werdet Ihr so entschlüsseln und "
		"demaskieren können.",
		NULL,
		"u",
		M_BARDE,
		(UNITSPELL | ONSHIPCAST | ONETARGET | TESTCANSEE),
		5, 5,
	 {
			{R_AURA, 10, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_analysesong_unit, patzer
	},

	{SPL_CERRDOR_FUMBLESHIELD, "Bannlied",
		"Dieser schrille Gesang hallt über das ganze Schlachtfeld. Die besonderen "
		"Dissonanzen in den Melodien machen es Magier fast unmöglich, sich "
		"auf ihre Zauber zu konzentrieren.",
		NULL,
		NULL,
	 M_BARDE, (PRECOMBATSPELL | SPELLLEVEL), 2, 5,
	 {
			{R_AURA, 5, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_fumbleshield, patzer
	},

	{ SPL_CALM_MONSTER, "Monster friedlich stimmen",
		"Dieser einschmeichelnde Gesang kann fast jedes intelligente Monster "
		"zähmen. Es wird von Angriffen auf den Magier absehen und auch seine "
		"Begleiter nicht anrühren. Doch sollte man sich nicht täuschen, es "
		"wird dennoch ein unberechenbares Wesen bleiben.",
		NULL,
		"u",
		M_BARDE,
		(UNITSPELL | ONSHIPCAST | ONETARGET | TESTRESISTANCE | TESTCANSEE),
		5, 6,
		{
			{R_AURA, 15, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
	 (spell_f)sp_calm_monster, patzer
	},

	{ SPL_SEDUCE, "Lied der Verführung",
		"Mit diesem Lied kann eine Einheit derartig betört werden, so dass "
		"sie dem Barden den größten Teil ihres Bargelds und ihres Besitzes "
		"schenkt. Sie behält jedoch immer soviel, wie sie zum Überleben "
		"braucht.",
		NULL,
		"u",
		M_BARDE,
		(UNITSPELL | ONETARGET | TESTRESISTANCE | TESTCANSEE),
		5, 6,
		{
			{R_AURA, 12, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
	 (spell_f)sp_seduce, patzer
	},

	{SPL_TRUESEEING_CERDDOR, "Erschaffe ein Amulett des wahren Sehens",
		"Der Spruch ermöglicht es einem Magier, ein Amulett des Wahren Sehens "
		"zu erschaffen. Das Amulett erlaubt es dem Träger, alle Einheiten, die "
		"durch einen Ring der Unsichtbarkeit geschützt sind, zu sehen. Einheiten "
		"allerdings, die sich mit ihrem Tarnungs-Talent verstecken, bleiben "
		"weiterhin unentdeckt.",
		NULL,
		NULL,
		M_BARDE, (ONSHIPCAST), 5, 6,
		{
			{R_AURA, 50, SPC_FIX},
			{R_SILVER, 3000, SPC_FIX},
			{R_PERMAURA, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_createitem_trueseeing, patzer_createitem
	},

	{SPL_INVISIBILITY_CERDDOR, "Erschaffe einen Ring der Unsichtbarkeit",
		"Mit diesem Spruch kann der Zauberer einen Ring der Unsichtbarkeit "
		"erschaffen. Der Träger des Ringes wird für alle Einheiten anderer "
		"Parteien unsichtbar, egal wie gut ihre Wahrnehmung auch sein mag. In "
		"einer unsichtbaren Einheit muss jede Person einen Ring tragen.",
		NULL,
		NULL,
		M_BARDE, (ONSHIPCAST), 5, 6,
		{
			{R_AURA, 50, SPC_FIX},
			{R_SILVER, 3000, SPC_FIX},
			{R_PERMAURA, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_createitem_invisibility, patzer_createitem
	},

  {SPL_HEADACHE, "Schaler Wein",
		"Aufzeichung des Vortrags von Selen Ard'Ragorn in Bar'Glingal: "
		"'Es heiss, dieser Spruch wäre wohl in den Spelunken der Westgassen "
		"entstanden, doch es kann genausogut in jedem andern verrufenen "
		"Viertel gewesen sein. Seine wichtigste Zutat ist etwa ein Fass "
		"schlechtesten Weines, je billiger und ungesunder, desto "
		"wirkungsvoller wird die Essenz. Die Kunst, diesen Wein in pure "
		"Essenz zu destillieren, die weitaus anspruchsvoller als das einfache "
		"Rezeptmischen eines Alchemisten ist, und diese dergestalt zu binden "
		"und konservieren, das sie sich nicht gleich wieder verflüchtigt, wie "
		"es ihre Natur wäre, ja, dies ist etwas, das nur ein Meister des "
		"Cerddor vollbringen kann. Nun besitzt Ihr eine kleine Phiola mit "
		"einer rubinrotschimmernden - nun, nicht flüssig, doch auch nicht "
		"ganz Dunst - nennen wir es einfach nur Elixier. Doch nicht dies ist "
		"die wahre Herausforderung, sodann muss, da sich ihre Wirkung leicht "
		"verflüchtigt, diese innerhalb weniger Tage unbemerkt in das Getränkt "
		"des Opfers geträufelt werden. Ihr Meister der Betöhrung und "
		"Verführung, hier nun könnt Ihr Eure ganze Kunst unter Beweis "
		"stellen. Doch gebt Acht, nicht unbedacht selbst von dem Elixier zu "
		"kosten, denn wer einmal gekostet hat, der kann vom Weine nicht mehr "
		"lassen, und er säuft sicherlich eine volle Woche lang. Jedoch nicht "
		"die Verführung zum Trunke ist die wahre Gefahr, die dem Elixier "
		"innewohnt, sondern das der Trunkenheit so sicher ein gar "
		"fürchterliches Leid des Kopfes folgen wird, wie der Tag auf die "
		"Nacht folgt. Und er wird gar sicherlich von seiner besten Fähigkeit "
		"einige Tage bis hin zu den Studien zweier Wochen vergessen haben. "
		"Noch ein Wort der Warnung: Dieses ist sehr aufwendig, und so Ihr "
		"noch weitere Zauber in der selben Woche wirken wollt, so werden sie Euch "
		"schwerer fallen.'",
		NULL,
		"u",
		M_BARDE,
		(UNITSPELL | ONETARGET | TESTRESISTANCE | TESTCANSEE),
		5, 7,
		{
			{R_AURA, 4, SPC_LINEAR},
			{R_SWAMP_2, 3, SPC_FIX},
			{R_SILVER, 50, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_headache, patzer
	},

	{ SPL_PUMP, "Aushorchen",
		"Erliegt die Einheit dem Zauber, so wird sie dem Magier alles erzählen, "
		"was sie über die gefragte Region weiß. Ist in der Region niemand "
		"ihrer Partei, so weiß sie nichts zu berichten. Auch kann sie nur das "
		"erzählen, was sie selber sehen könnte.",
		"ZAUBERE \"Aushorchen\" <Einheit-Nr> <Zielregion-X> <Zielregion-Y>",
		"ur",
		M_BARDE, (UNITSPELL | ONETARGET | TESTCANSEE), 5, 7,
		{
			{R_AURA, 4, SPC_FIX},
			{R_SILVER, 100, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
	 (spell_f)sp_pump, patzer
	},

	{SPL_BLOODTHIRST, "Kriegsgesang",
		"Wie viele magischen Gesänge, so entstammt auch dieser den altem "
		"Wissen der Katzen, die schon immer um die machtvolle Wirkung der "
		"Stimme wussten. Mit diesem Lied wird die Stimmung der Krieger "
		"aufgepeitscht, sie gar in wilde Raserrei und Blutrausch versetzt. "
		"Ungeachtet eigener Schmerzen werden sie kämpfen bis zum "
		"Tode und niemals fliehen. Während ihre Attacke verstärkt ist "
		"achten sie kaum auf sich selbst.",
		NULL,
		NULL,
		M_BARDE, (PRECOMBATSPELL | SPELLLEVEL), 4, 7,
		{
			{R_AURA, 5, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
	 (spell_f)sp_berserk, patzer
	},

	{SPL_FRIGHTEN, "Gesang der Angst",
		"Dieser Kriegsgesang sät Panik in der Front der Gegner und schwächt "
		"so ihre Kampfkraft erheblich. Angst wird ihren Schwertarm schwächen "
		"und Furcht ihren Schildarm lähmen.",
		NULL,
		NULL,
	 M_BARDE, (PRECOMBATSPELL | SPELLLEVEL), 5, 8,
	 {
		{R_AURA, 5, SPC_LEVEL},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0}},
	 (spell_f)sp_frighten, patzer
	},

	{SPL_OBJ_ANALYSESONG, "Lied des Ortes analysieren",
		"Wie Lebewesen, so haben auch Schiffe und Gebäude und sogar Regionen "
		"ihr eigenes Lied, wenn auch viel schwächer und schwerer zu hören. "
		"Und so, wie wie aus dem Lebenslied einer Person erkannt werden kann, "
		"ob diese unter einem Zauber steht, so ist dies auch bei Burgen, "
		"Schiffen oder Regionen möglich.",
		"ZAUBERE [STUFE n] \"Lied des Ortes analysieren\" REGION\n"
		"ZAUBERE [STUFE n] \"Lied des Ortes analysieren\" BURG <Burg-nr>\n"
		"ZAUBERE [STUFE n] \"Lied des Ortes analysieren\" GEBÄUDE <Gebäude-nr>\n"
		"ZAUBERE [STUFE n] \"Lied des Ortes analysieren\" SCHIFF <Schiff-nr>",
		"kc",
		M_BARDE, (SPELLLEVEL|ONSHIPCAST), 5, 8,
		{
			{R_AURA, 3, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_analysesong_obj, patzer
	},

	{SPL_CERDDOR_DESTROY_MAGIC, "Lebenslied festigen",
		"Jede Verzauberung beeinflußt das Lebenslied, schwächt und verzerrt es. "
		"Der kundige Barde kann versuchen, das Lebenslied aufzufangen und zu "
		"verstärken und die Veränderungen aus dem Lied zu tilgen.",
		"ZAUBERE [REGION x y] [STUFE n] \"Lebenslied festigen\" REGION\n"
		"ZAUBERE [REGION x y] [STUFE n] \"Lebenslied festigen\" EINHEIT <Einheit-Nr>\n"
		"ZAUBERE [REGION x y] [STUFE n] \"Lebenslied festigen\" BURG <Burg-Nr>\n"
		"ZAUBERE [REGION x y] [STUFE n] \"Lebenslied festigen\" GEBÄUDE <Gebäude-Nr>\n"
		"ZAUBERE [REGION x y] [STUFE n] \"Lebenslied festigen\" SCHIFF <Schiff-Nr>",
		"kc",
		M_BARDE,
		(FARCASTING | SPELLLEVEL | ONSHIPCAST | ONETARGET | TESTCANSEE),
		2, 8,
	 {
			{R_AURA, 5, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_destroy_magic, patzer
	},

	{SPL_MIGRANT, "Ritual der Aufnahme",
		"Dieses Ritual ermöglicht es, eine Einheit, egal welcher Art, in die "
		"eigene Partei aufzunehmen. Der um Aufnahme Bittende muss dazu willig "
		"und bereit sein, seiner alten Partei abzuschwören. Dies bezeugt er "
		"durch KONTAKTIEREn des Magiers. Auch wird er die Woche über "
		"ausschliesslich mit Vorbereitungen auf das Ritual beschäftigt sein. "
		"Das Ritual wird fehlschlagen, wenn er zu stark an seine alte Partei "
		"gebunden ist, dieser etwa Dienst für seine teuere Ausbildung "
		"schuldet. Der das Ritual leitende Magier muss für die permanente "
		"Bindung des Aufnahmewilligen an seine Partei naturgemäß auch "
		"permanente Aura aufwenden. Pro Stufe und pro 1 permanente Aura kann "
		"er eine Person aufnehmen.",
		NULL,
		"u",
		M_BARDE, (UNITSPELL | SPELLLEVEL | ONETARGET | TESTCANSEE), 5, 9,
		{
			{R_AURA, 3, SPC_LEVEL},
			{R_PERMAURA, 1, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
	 (spell_f)sp_migranten, patzer
	},

	{SPL_CERDDOR_FAMILIAR, "Vertrauten rufen",
		"Einem erfahrenen Magier wird irgendwann auf seinen Wanderungen ein "
		"ungewöhnliches Exemplar einer Gattung begegnen, welches sich dem "
		"Magier anschließen wird.",
		NULL,
		NULL,
	 M_BARDE, (NOTFAMILIARCAST), 5, 9,
	 {
			{R_AURA, 100, SPC_FIX},
		 {R_PERMAURA, 5, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_summon_familiar, patzer
	},

	{SPL_RAISEPEASANTS, "Mob aufwiegeln",
	 "Mit Hilfe dieses magischen Gesangs überzeugt der Magier die Bauern "
	 "der Region, sich ihm anzuschließen. Die Bauern werden ihre Heimat jedoch "
	 "nicht verlassen, und keine ihrer Besitztümer fortgeben. Jede Woche "
	 "werden zudem einige der Bauern den Bann abwerfen und auf ihre Felder "
	 "zurückkehren. Wie viele Bauern sich dem Magier anschließen hängt von der "
	 "Kraft seines Gesangs ab.",
		NULL,
		NULL,
	 M_BARDE, (SPELLLEVEL | REGIONSPELL | TESTRESISTANCE), 5, 10,
	 {
			{R_AURA, 4, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_raisepeasants, patzer
	},

	{SPL_SONG_RESISTMAGIC, "Gesang des wachen Geistes",
		"Dieses magische Lied wird, einmal mit Inbrunst gesungen, sich in der "
		"Region fortpflanzen, von Mund zu Mund springen und eine Zeitlang "
		"überall zu vernehmen sein. Nach wie vielen Wochen der Gesang aus dem "
		"Gedächnis der Region entschwunden ist, ist von dem Geschick des Barden "
		"abhängig. Bis das Lied ganz verklungen ist, wird seine Magie allen "
		"Verbündeten des Barden (HELFE BEWACHE), und natürlich auch seinen "
		"eigenem Volk, einen einmaligen Bonus von 15% "
		"auf die natürliche Widerstandskraft gegen eine Verzauberung "
		"verleihen.",
		NULL,
		NULL,
		M_BARDE,
		(FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE),
		2, 10,
	 {
			{R_AURA, 2, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_song_resistmagic, patzer
	},

	{SPL_DEPRESSION, "Gesang der Melancholie",
	 "Mit diesem Gesang verbreitet der Barde eine melancholische, traurige "
	 "Stimmung unter den Bauern. Einige Wochen lang werden sie sich in ihre "
	 "Hütten zurückziehen und kein Silber in den Theatern und Tavernen lassen.",
		NULL,
		NULL,
	 M_BARDE, (FARCASTING | REGIONSPELL | TESTRESISTANCE), 5, 11,
	 {
			{R_AURA, 40, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_depression, patzer
	},

	{SPL_ARTEFAKT_NIMBLEFINGERRING, "Miriams flinke Finger",
		"Die berühmte Bardin Miriam bhean'Meddaf war bekannt für ihr "
		"außergewöhnliches Geschick mit der Harfe. Ihre Finger sollen sich "
		"so schnell über die Saiten bewegt haben, das sie nicht mehr erkennbar "
		"waren. Dieser Zauber, der recht einfach in einen Silberring zu bannen "
		"ist, bewirkt eine um das zehnfache verbesserte Geschicklichkeit und "
		"Gewandheit der Finger. (Das soll sie auch an anderer Stelle ausgenutzt "
		"haben, ihr Ruf als Falschspielerin war berüchtigt.) Handwerker können "
		"somit das zehnfache produzieren, und bei einigen anderen Tätigkeiten "
		"könnte dies ebenfalls von Nutzen sein.",
		NULL,
		NULL,
		M_BARDE, (ONSHIPCAST), 5, 11,
		{
			{R_AURA, 20, SPC_FIX},
			{R_PERMAURA, 1, SPC_FIX},
			{R_SILVER, 1000, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0}},
	 (spell_f)sp_create_nimblefingerring, patzer
	},

	{SPL_SONG_SUSCEPTMAGIC, "Gesang des schwachen Geistes",
			"Dieses Lied, das in die magische Essenz der Region gewoben wird, "
			"schwächt die natürliche Widerstandskraft gegen eine "
			"Verzauberung einmalig um 15%. Nur die Verbündeten des Barden "
			"(HELFE BEWACHE) sind gegen die Wirkung des Gesangs gefeit.",
		NULL,
		NULL,
		M_BARDE,
		(FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE),
		2, 12,
	 {
			{R_AURA, 2, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_song_susceptmagic, patzer
	},

	{SPL_SONG_OF_PEACE, "Gesang der Friedfertigkeit",
		"Dieser mächtige Bann verhindert jegliche Attacken. Niemand in der "
		"ganzen Region ist fähig seine Waffe gegen irgendjemanden zu erheben. "
		"Die Wirkung kann etliche Wochen andauern",
		NULL,
		NULL,
		M_BARDE, (SPELLLEVEL | REGIONSPELL | TESTRESISTANCE), 5, 12,
		{
			{R_AURA, 20, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0} },
		(spell_f)sp_song_of_peace, patzer
	},

	{SPL_SONG_OF_ENSLAVE, "Gesang der Versklavung",
		"Dieser mächtige Bann raubt dem Opfer seinen freien Willen und "
		"unterwirft sie den Befehlen des Barden. Für einige Zeit wird das Opfer "
		"sich völlig von seinen eigenen Leuten abwenden und der Partei des Barden "
		"zugehörig fühlen.",
		NULL,
		"u",
		M_BARDE, (UNITSPELL | ONETARGET | TESTCANSEE), 5, 13,
		{
			{R_AURA, 40, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0} },
		(spell_f)sp_charmingsong, patzer
	},

	{SPL_RALLYPEASANTMOB, "Aufruhr beschwichtigen",
		"Mit Hilfe dieses magischen Gesangs kann der Magier eine Region in "
		"Aufruhr wieder beruhigen. Die Bauernhorden werden sich verlaufen "
		"und wieder auf ihre Felder zurückkehren.",
		NULL,
		NULL,
	 M_BARDE, (FARCASTING), 5, 15,
	 {
			{R_AURA, 30, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_rallypeasantmob, patzer
	},

	{SPL_RAISEPEASANTMOB, "Aufruhr verursachen",
		"Mit Hilfe dieses magischen Gesangs versetzt der Magier eine ganze "
		"Region in Aufruhr. Rebellierende Bauernhorden machen jedes Besteuern "
		"unmöglich, kaum jemand wird mehr für Gaukeleien Geld spenden und "
		"es können keine neuen Leute angeworben werden. Nach einigen Wochen "
		"beruhigt sich der Mob wieder.",
		NULL,
		NULL,
	 M_BARDE, (FARCASTING | REGIONSPELL | TESTRESISTANCE), 5, 16,
	 {
			{R_AURA, 40, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_raisepeasantmob, patzer
	},

/* M_ASTRAL */

	{SPL_ANALYSEMAGIC, "Magie analysieren",
		"Mit diesem Spruch kann der Magier versuchen, die Verzauberungen "
		"eines einzelnen angegebenen Objekts zu erkennen. Von allen Sprüchen, "
		"die seine eigenen Fähigkeiten nicht überschreiten, wird er einen "
		"Eindruck ihres Wirkens erhalten können. Bei stärkeren Sprüchen "
		"benötigt er ein wenig Glück für eine gelungene Analyse.",
		"ZAUBERE [STUFE n] \"Magie analysieren\" REGION\n"
		"ZAUBERE [STUFE n] \"Magie analysieren\" EINHEIT <Einheit-Nr>\n"
		"ZAUBERE [STUFE n] \"Magie analysieren\" BURG <Burg-Nr>\n"
		"ZAUBERE [STUFE n] \"Magie analysieren\" GEBÄUDE <Gebäude-Nr>\n"
		"ZAUBERE [STUFE n] \"Magie analysieren\" SCHIFF <Schiff-Nr>",
		"kc",
		M_ASTRAL, (UNITSPELL | ONSHIPCAST | TESTCANSEE), 5, 1,
		{
			{R_AURA, 1, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_analysemagic, patzer
	},

	{SPL_ITEMCLOAK, "Schleieraura",
		"Dieser Zauber wird die gesamte Ausrüstung der Zieleinheit für "
		"einige Zeit vor den Blicken anderer verschleiern. Der Zauber "
		"schützt nicht vor Dieben und Spionen.",
		NULL,
		"u",
		M_ASTRAL, (SPELLLEVEL | UNITSPELL | ONSHIPCAST | ONETARGET), 5, 1,
		{
		 {R_AURA, 1, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
		(spell_f)sp_itemcloak, patzer
	},

	{SPL_TYBIED_EARN_SILVER, "Wunderdoktor",
	"Wenn einem der Alchemist nicht weiterhelfen kann, geht man zu dem "
	"gelehrten Tybiedmagier. Seine Tränke und Tinkturen helfen gegen alles, "
	"was man sonst nicht bekommen kann. Ob nun die kryptische Formel unter dem "
	"Holzschuh des untreuen Ehemannes wirklich geholfen hat - nun, der des "
	"Lesens nicht mächtige Bauer wird es nie wissen.  Dem Magier hilft es "
	"auf jeden Fall... beim Füllen seines Geldbeutels.  50 Silber pro Stufe "
	"lassen sich so in einer Woche verdienen.",
		NULL,
		NULL,
		M_ASTRAL, (SPELLLEVEL|ONSHIPCAST), 5, 1,
		{
		 {R_AURA, 1, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
		(spell_f)sp_earn_silver, patzer
	},

	{SPL_TYBIED_FUMBLESHIELD, "Schutz vor Magie",
		"Dieser Zauber legt ein antimagisches Feld um die Magier der "
		"Feinde und behindert ihre Zauber erheblich. Nur wenige werden "
		"die Kraft besitzen, das Feld zu durchdringen und ihren Truppen "
		"in der Schlacht zu helfen.",
		NULL,
		NULL,
	 M_ASTRAL, (PRECOMBATSPELL | SPELLLEVEL), 2, 2,
	 {
		 {R_AURA, 3, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_fumbleshield, patzer
	},

	{SPL_SHOWASTRAL, "Astraler Blick",
	"Der Magier kann kurzzeitig in die Astralebene blicken und erfährt "
	"so alle Einheiten innerhalb eines astralen Radius von Stufe/5 Regionen.",
		NULL,
		NULL,
	 M_ASTRAL, (SPELLLEVEL), 5, 2,
	 {
			{R_AURA, 1, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_showastral, patzer
	},

	{SPL_RESISTMAGICBONUS, "Schutzzauber",
		"Dieser Zauber verstärkt die natürliche Widerstandskraft gegen Magie. "
		"Eine so geschützte Einheit ist auch gegen Kampfmagie weniger "
		"empfindlich. Pro Stufe reicht die Kraft des Magiers aus, um 5 Personen "
		"zu schützen.",
		NULL,
		"u+",
		M_ASTRAL,
		(UNITSPELL | SPELLLEVEL | ONSHIPCAST | TESTCANSEE),
		2, 3,
		{
			{R_AURA, 5, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
		(spell_f)sp_resist_magic_bonus, patzer
	},

	{SPL_KEEPLOOT, "Beute bewahren",
		"Dieser Zauber verhindert, dass ein Teil der sonst im Kampf zerstörten "
		"Gegenstände beschädigt wird. Die Verluste reduzieren sich um 5% pro "
		"Stufe des Zaubers bis zu einem Minimum von 25%.",
		NULL,
		NULL,
	 M_ASTRAL, ( POSTCOMBATSPELL | SPELLLEVEL ), 5, 3,
	 {
			{R_AURA, 1, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_keeploot, patzer
	},

	{SPL_ENTERASTRAL, "Astraler Weg",
	 "Alte arkane Formeln ermöglichen es dem Magier, sich und andere in die "
	 "astrale Ebene zu bringen. Der Magier kann (Stufe-3)*15 GE durch das "
	 "kurzzeitig entstehende Tor bringen. Ist der Magier erfahren genug, "
	 "den Zauber auf Stufen von 11 oder mehr zu zaubern, kann er andere "
	 "Einheiten auch gegen ihren Willen auf die andere Ebene zwingen.",
		NULL,
		"u+",
	 M_ASTRAL, (UNITSPELL|SPELLLEVEL), 7, 4,
	 {
		 {R_AURA, 2, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_enterastral, patzer
	},

	{SPL_LEAVEASTRAL, "Astraler Ausgang",
		"Der Magier konzentriert sich auf die Struktur der Realität und kann "
		"so die astrale Ebene verlassen. Er kann insgesamt (Stufe-3)*15 GE durch "
		"das kurzzeitig entstehende Tor bringen. Ist der Magier erfahren genug, "
		"den Zauber auf Stufen von 11 oder mehr zu zaubern, kann er andere "
		"Einheiten auch gegen ihren Willen auf die andere Ebene zwingen.",
		"ZAUBER [STUFE n] \"Astraler Ausgang\" <Ziel-X> <Ziel-Y> <Einheit-Nr> "
			"[<Einheit-Nr> ...]",
		"ru+",
		M_ASTRAL, (UNITSPELL |SPELLLEVEL), 7, 4,
		{
			{R_AURA, 2, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_leaveastral, patzer
	},

	{SPL_TRANSFERAURA_ASTRAL, "Auratransfer",
	 "Mit Hilfe dieses Zauber kann der Magier eigene Aura im Verhältnis "
	 "2:1 auf einen anderen Magier des gleichen Magiegebietes oder im "
	 "Verhältnis 3:1 auf einen Magier eines anderen Magiegebietes "
	 "übertragen.",
		"ZAUBERE \"Auratransfer\" <Einheit-Nr> <investierte Aura>",
		"ui",
	 M_ASTRAL, (UNITSPELL|ONSHIPCAST|ONETARGET), 1, 5,
	 {
			{R_AURA, 1, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_transferaura, patzer
	},

	{SPL_SHOCKWAVE, "Schockwelle",
		"Dieser Zauber läßt eine Welle aus purer Kraft über die "
		"gegnerischen Reihen hinwegfegen.  Viele Kämpfer wird der Schock "
		"so benommen machen, daß sie für einen kurzen Moment nicht angreifen "
		"können.",
		NULL,
		NULL,
		M_ASTRAL, (COMBATSPELL|SPELLLEVEL), 5, 5,
		{
			{R_AURA, 1, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_stun, patzer
	},

	{SPL_ANTIMAGICZONE, "Astrale Schwächezone",
		"Mit diesem Zauber kann der Magier eine Zone der astralen Schwächung "
		"erzeugen, ein lokales Ungleichgewicht im Astralen Feld. Dieses "
		"Zone wird bestrebt sein, wieder in den Gleichgewichtszustand "
		"zu gelangen. Dazu wird sie jedem in dieser Region gesprochenen "
		"Zauber einen Teil seiner Stärke entziehen, die schwächeren gar "
		"ganz absorbieren.",
		NULL,
		NULL,
		M_ASTRAL, (FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE),
		2, 5,
	 {
			{R_AURA, 3, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_antimagiczone, patzer
	},

	{SPL_TRUESEEING_TYBIED, "Erschaffe ein Amulett des wahren Sehens",
		"Der Spruch ermöglicht es einem Magier, ein Amulett des Wahren Sehens "
		"zu erschaffen. Das Amulett erlaubt es dem Träger, alle Einheiten, die "
		"durch einen Ring der Unsichtbarkeit geschützt sind, zu sehen. Einheiten "
		"allerdings, die sich mit ihrem Tarnungs-Talent verstecken, bleiben "
		"weiterhin unentdeckt.",
		NULL,
		NULL,
		M_ASTRAL, (ONSHIPCAST), 5, 5,
		{
			{R_AURA, 50, SPC_FIX},
			{R_SILVER, 3000, SPC_FIX},
			{R_PERMAURA, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_createitem_trueseeing, patzer_createitem
	},

	{SPL_PULLASTRAL, "Astraler Ruf",
	 "Ein Magier, der sich in der astralen Ebene befindet, kann mit Hilfe "
	 "dieses Zaubers andere Einheiten zu sich holen. Der Magier kann "
	 "(Stufe-3)*15 GE durch das kurzzeitig entstehende Tor bringen. Ist der "
	 "Magier erfahren genug, den Zauber auf Stufen von 13 oder mehr zu zaubern, "
	 "kann er andere Einheiten auch gegen ihren Willen auf die andere Ebene "
	 "zwingen.",
		"ZAUBER [STUFE n] \"Astraler Ruf\" <Ziel-X> <Ziel-Y> <Einheit-Nr> "
			"[<Einheit-Nr> ...]",
	 "ru+",
	 M_ASTRAL, (UNITSPELL | SEARCHGLOBAL | SPELLLEVEL), 7, 6,
	 {
		 {R_AURA, 2, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_pullastral, patzer
	},


	{SPL_FETCHASTRAL, "Ruf der Realität",
		"Ein Magier, welcher sich in der materiellen Welt befindet, kann er mit "
		"Hilfe dieses Zaubers Einheiten aus der angrenzenden Astralwelt herbeiholen. "
		"Ist der Magier erfahren genug, den Zauber auf Stufen von 13 oder mehr zu "
		"zaubern, kann er andere Einheiten auch gegen ihren Willen in die materielle "
		"Welt zwingen.",
		NULL,
		"u+",
		M_ASTRAL, (UNITSPELL | SEARCHGLOBAL | SPELLLEVEL), 7, 6,
		{
			{R_AURA, 2, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_fetchastral, patzer
	},

	{SPL_STEALAURA, "Stehle Aura",
	 "Mit Hilfe dieses Zaubers kann der Magier einem anderen Magier seine "
	 "Aura gegen dessen Willen entziehen und sich selber zuführen.",
		NULL,
		"u",
		M_ASTRAL,
		(FARCASTING | SPELLLEVEL | UNITSPELL | ONETARGET | TESTRESISTANCE | TESTCANSEE),
		3, 6,
		{
			{R_AURA, 2, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_stealaura, patzer
	},

	{SPL_FLYING_SHIP, "Luftschiff",
		"Diese magischen Runen bringen ein Boot oder Langboot für eine Woche "
		"zum fliegen. Damit kann dann auch Land überquert werden. Die Zuladung "
		"von Langbooten ist unter der Einwirkung dieses Zaubers auf 100 "
		"Gewichtseinheiten begrenzt. Für die Farbe der Runen muss eine spezielle "
		"Tinte aus einem Windbeutel und einem Schneekristall angerührt werden.",
		NULL,
		"s",
		M_ASTRAL, (ONSHIPCAST | SHIPSPELL | ONETARGET | TESTRESISTANCE), 5, 6,
		{
			{R_AURA, 10, SPC_FIX},
			{R_HIGHLAND_1, 1, SPC_FIX},
			{R_GLACIER_3, 1, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_flying_ship, patzer
	},

	{SPL_INVISIBILITY_TYBIED, "Erschaffe einen Ring der Unsichtbarkeit",
		"Mit diesem Spruch kann der Zauberer einen Ring der Unsichtbarkeit "
		"erschaffen. Der Träger des Ringes wird für alle Einheiten anderer "
		"Parteien unsichtbar, egal wie gut ihre Wahrnehmung auch sein mag. In "
		"einer unsichtbaren Einheit muss jede Person einen Ring tragen.",
		NULL,
		NULL,
		M_ASTRAL, (ONSHIPCAST), 5, 6,
		{
			{R_AURA, 50, SPC_FIX},
			{R_SILVER, 3000, SPC_FIX},
			{R_PERMAURA, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_createitem_invisibility, patzer_createitem
	},

	{SPL_CREATE_ANTIMAGICCRYSTAL, "Erschaffe Antimagiekristall",
	 "Mit Hilfe dieses Zauber entzieht der Magier einem Quarzkristall "
	 "all seine magischen Energien. Der Kristall wird dann, wenn er zu "
	 "feinem Staub zermahlen und verteilt wird, die beim Zaubern "
	 "freigesetzten magischen Energien aufsaugen und alle Zauber, "
	 "welche in der betreffenden Woche in der Region gezaubert werden "
	 "fehlschlagen lassen.",
		NULL,
		NULL,
		M_ASTRAL, (ONSHIPCAST), 5, 7,
		{
			{R_AURA, 50, SPC_FIX},
			{R_SILVER, 3000, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
	 (spell_f)sp_create_antimagiccrystal, patzer_createitem
	},

	{SPL_Q_ANTIMAGIE, "Magiefresser",
		"Dieser Zauber ermöglicht dem Magier, gezielt eine bestimmte "
		"Verzauberung einer Einheit, eines Schiffes, Gebäudes oder auch "
		"der Region aufzulösen. Dazu muss er den Namen des Fluchs, "
		"den er aufheben will, beim Zaubern angeben. Ist der Name ihm "
		"unbekannt, so wird der Zauberspruch zufällig gegen irgendeine "
		"Verzauberung des Ziels wirken.",
		"ZAUBERE [REGION x y] [STUFE n] \"Magiefresser\" REGION [Zaubername]\n"
		"ZAUBERE [REGION x y] [STUFE n] \"Magiefresser\" EINHEIT <Einheit-Nr> [Zaubername]\n"
		"ZAUBERE [REGION x y] [STUFE n] \"Magiefresser\" BURG <Burg-Nr> [Zaubername]\n"
		"ZAUBERE [REGION x y] [STUFE n] \"Magiefresser\" GEBÄUDE <Gebäude-Nr> [Zaubername]\n"
		"ZAUBERE [REGION x y] [STUFE n] \"Magiefresser\" SCHIFF <Schiff-Nr> [Zaubername]",
		"kc?c",
	 M_ASTRAL, (FARCASTING | SPELLLEVEL | ONSHIPCAST | TESTCANSEE), 3, 7,
	 {
			{R_AURA, 3, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_q_antimagie, patzer
	},

	{SPL_ETERNIZEWALL, "Mauern der Ewigkeit",
		"Mit dieser Formel bindet der Magier auf ewig die Kräfte der Erde in "
		"die Mauern des Gebäudes. Ein solchermaßen verzaubertes Gebäude ist "
		"gegen den Zahn der Zeit geschützt und benötigt keinen "
		"Unterhalt mehr.",
		"ZAUBER \"Mauern der Ewigkeit\" <Gebäude-Nr>",
		"b",
		M_ASTRAL,
		(SPELLLEVEL | BUILDINGSPELL | ONETARGET | TESTRESISTANCE | ONSHIPCAST),
		5, 7,
		{
			{R_AURA, 50, SPC_FIX},
			{R_PERMAURA, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
	 (spell_f)sp_eternizewall, patzer
	},

	{SPL_SCHILDRUNEN, "Runen des Schutzes",
		"Zeichnet man diese Runen auf die Wände eines Gebäudes oder auf die "
		"Planken eines Schiffes, so wird es schwerer durch Zauber zu "
		"beeinflussen sein. Jedes Ritual erhöht die Widerstandskraft des "
		"Gebäudes oder Schiffes gegen Verzauberung um 20%. "
		"Werden mehrere Schutzzauber übereinander gelegt, so addiert "
		"sich ihre Wirkung, doch ein hundertprozentiger Schutz läßt sich so "
		"nicht erreichen. Der Zauber hält mindestens drei Wochen an, je nach "
		"Talent des Magiers aber auch viel länger.",
		"ZAUBERE \"Runen des Schutzes\" [BURG <Burg-nr> | GEBÄUDE <Gebäude-Nr> | "
		"SCHIFF <Schiff-Nr>]",
		"kc",
	 M_ASTRAL, (ONSHIPCAST | TESTRESISTANCE), 2, 8,
	 {
			{R_AURA, 20, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_antimagicshild, patzer
	},


	{SPL_REDUCESHIELD, "Schild des Fisches",
		"Dieser Zauber vermag dem Gegner ein geringfügig versetztes Bild der "
		"eigenen Truppen vorzuspiegeln, so wie der Fisch im Wasser auch nicht "
		"dort ist wo er zu sein scheint. Von jedem Treffer kann so die Hälfte "
		"des Schadens unschädlich abgeleitet werden. Doch hält der Schild nur "
		"einige Hundert Schwerthiebe aus, danach wird er sich auflösen. "
		"Je stärker der Magier, desto mehr Schaden hält der Schild aus.",
		NULL,
		NULL,
	 M_ASTRAL, (PRECOMBATSPELL | SPELLLEVEL), 2, 8,
	 {
			{R_AURA, 4, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_reduceshield, patzer
	},

	{SPL_SPEED, "Beschleunigung",
		"Dieser Zauber beschleunigt einige Kämpfer auf der eigenen Seite "
		"so, dass sie während des gesamten Kampfes in einer Kampfrunde zweimal "
		"angreifen können.",
		NULL,
		NULL,
	 M_ASTRAL, (PRECOMBATSPELL | SPELLLEVEL), 5, 9,
	 {
			{R_AURA, 5, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_speed, patzer
	},

	{SPL_ARTEFAKT_OF_POWER, "Erschaffe einen Ring der Macht",
		"Dieses mächtige Ritual erschafft einen Ring der Macht. Ein Ring "
		"der Macht erhöht die Stärke jedes Zaubers, den sein Träger zaubert, "
		"als wäre der Magier eine Stufe besser.",
		NULL,
		NULL,
		M_ASTRAL, (ONSHIPCAST), 5, 9,
		{
			{R_AURA, 100, SPC_FIX},
			{R_SILVER, 4000, SPC_FIX},
			{R_PERMAURA, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0}},
	 (spell_f)sp_createitem_power, patzer_createitem
	},

	{SPL_VIEWREALITY, "Blick in die Realität",
	 "Der Magier kann mit Hilfe dieses Zaubers aus der Astral- in die "
	 "materielle Ebene blicken und die Regionen und Einheiten genau "
	 "erkennen.",
		NULL,
		NULL,
	 M_ASTRAL, (0), 5, 10,
	 {
			{R_AURA, 40, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_viewreality, patzer
	},

	{SPL_BAG_OF_HOLDING, "Erschaffe einen Beutel des Negativen Gewichts",
	 "Dieser Beutel umschließt eine kleine Dimensionsfalte, in der bis "
	 "zu 200 Gewichtseinheiten transportiert werden können, ohne dass "
   "sie auf das Traggewicht angerechnet werden.  Pferde und andere "
   "Lebewesen sowie besonders sperrige Dinge (Wagen und Katapulte) können "
   "nicht in dem Beutel transportiert werden.  Auch ist es nicht möglich, "
   "einen Zauberbeutel in einem anderen zu transportieren.  Der Beutel "
   "selber wiegt 1 GE.",
		NULL,
		NULL,
	 M_ASTRAL, (ONSHIPCAST), 5, 10,
	 {
			{R_AURA, 30, SPC_FIX},
			{R_PERMAURA, 1, SPC_FIX},
		 {R_SILVER, 5000, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_create_bag_of_holding, patzer
	},

	{SPL_SPEED2, "Zeitdehnung",
		"Diese praktische Anwendung des theoretischen Wissens um Raum und Zeit "
		"ermöglicht es, den Zeitfluß für einige Personen zu verändern. Auf "
		"diese Weise veränderte Personen bekommen für einige Wochen doppelt "
		"soviele Bewegungspunkte und doppelt soviele Angriffe pro Runde.",
		NULL,
		"u+",
	 M_ASTRAL, (UNITSPELL | SPELLLEVEL | ONSHIPCAST | TESTCANSEE), 5, 11,
	 {
			{R_AURA, 5, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_speed2, patzer
	},

	{SPL_ARMORSHIELD, "Rüstschild",
		"Diese vor dem Kampf zu zaubernde Ritual gibt den eigenen Truppen "
		"einen zusätzlichen Bonus auf ihre Rüstung. Jeder Treffer "
		"reduziert die Kraft des Zaubers, so dass der Schild sich irgendwann "
		"im Kampf auflösen wird.",
		NULL,
		NULL,
	 M_ASTRAL, (PRECOMBATSPELL | SPELLLEVEL), 2, 12,
	 {
			{R_AURA, 4, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_armorshield, patzer
	},

	{SPL_TYBIED_FAMILIAR, "Vertrauten rufen",
		"Einem erfahrenen Magier wird irgendwann auf seinen Wanderungen ein "
		"ungewöhnliches Exemplar einer Gattung begegnen, welches sich dem "
		"Magier anschließen wird.",
		NULL,
		NULL,
	 M_ASTRAL, (NOTFAMILIARCAST), 5, 12,
	 {
			{R_AURA, 100, SPC_FIX},
		 {R_PERMAURA, 5, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_summon_familiar, patzer
	},

	{SPL_MOVECASTLE, "Belebtes Gestein",
		"Dieses kräftezehrende Ritual beschwört mit Hilfe einer Kugel aus "
		"konzentriertem Laen einen gewaltigen Erdelementar und bannt ihn "
		"in ein Gebäude. Dem Elementar kann dann befohlen werden, das "
		"Gebäude mitsamt aller Bewohner in eine Nachbarregion zu tragen. "
		"Die Stärke des beschworenen Elementars hängt vom Talent des "
		"Magiers ab: Der Elementar kann maximal [Stufe-12]*250 Größeneinheiten "
		"große Gebäude versetzen. Das Gebäude wird diese Prozedur nicht "
		"unbeschädigt überstehen.",
		"ZAUBER [STUFE n] \"Belebtes Gestein\" <Burg-Nr> <Richtung>",
		"bc",
		M_ASTRAL,
		(SPELLLEVEL | BUILDINGSPELL | ONETARGET | TESTRESISTANCE),
		5, 13,
		{
			{R_AURA, 10, SPC_LEVEL},
			{R_PERMAURA, 1, SPC_FIX},
			{R_EOG, 5, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0}},
	 (spell_f)sp_movecastle, patzer
	},

	{SPL_DISRUPTASTRAL, "Störe Astrale Integrität",
	 "Dieser Zauber bewirkt eine schwere Störung des Astralraums. Innerhalb "
	 "eines astralen Radius von Stufe/5 Regionen werden alle Astralwesen, "
	 "die dem Zauber nicht wiederstehen können, aus der astralen Ebene "
	 "geschleudert. Der astrale Kontakt mit allen betroffenen Regionen ist "
	 "für Stufe/3 Wochen gestört.",
		NULL,
		NULL,
	 M_ASTRAL, (REGIONSPELL), 4, 14,
	 {
			{R_AURA, 140, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_disruptastral, patzer
	},

	{SPL_PERMTRANSFER, "Opfere Kraft",
	 "Mit Hilfe dieses Zaubers kann der Magier einen Teil seiner magischen "
	 "Kraft permanent auf einen anderen Magier übertragen. Auf einen Tybied-"
	 "Magier kann er die Hälfte der eingesetzten Kraft übertragen, auf einen "
	 "Magier eines anderen Gebietes ein Drittel.",
	 "ZAUBERE \"Opfere Kraft\" <Einheit-Nr> <Aura>",
		"ui",
	 M_ASTRAL, (UNITSPELL|ONETARGET), 1, 15,
	 {
			{R_AURA, 100, SPC_FIX},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_permtransfer, patzer
	},

/* M_GRAU */
/*  Definitionen von Create_Artefaktsprüchen                    */

	{SPL_ARTEFAKT_OF_AURAPOWER, "Erschaffe einen Fokus",
	 "Der auf diesem Gegenstand liegende Zauber erleichtert es dem "
	 "Zauberers enorm größere Mengen an Aura zu beherrschen.",
		NULL,
		NULL,
	 M_GRAU, (ONSHIPCAST), 5, 9,
	 {
			{R_AURA, 100, SPC_FIX},
			{R_PERMAURA, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
	 (spell_f)sp_createitem_aura, patzer_createitem
	},

	{SPL_ARTEFAKT_OF_REGENERATION, "Regeneration",
	 "Der auf diesem Gegenstand liegende Zauber saugt die diffusen "
	 "magischen Energien des Lebens aus der Umgebung auf und läßt sie "
	 "seinem Träger zukommen.",
		NULL,
		NULL,
	 M_GRAU, (ONSHIPCAST), 5, 9,
	 {
			{R_AURA, 100, SPC_FIX},
			{R_PERMAURA, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
	 (spell_f)sp_createitem_regeneration, patzer_createitem
	},

	{SPL_ARTEFAKT_CHASTITYBELT, "Erschaffe ein Amulett der Keuschheit",
		"Dieses Amulett in Gestalt einer orkischen Matrone unterdrückt den "
		"Fortpflanzungstrieb eines einzelnen Orks sehr zuverlässig. Ein Ork "
		"mit Amulett der Keuschheit wird sich nicht mehr vermehren.",
		NULL,
		NULL,
		M_GRAU, (ONSHIPCAST), 5, 7,
		{
			{R_AURA, 50, SPC_FIX},
			{R_SILVER, 3000, SPC_FIX},
			{R_PERMAURA, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_createitem_chastitybelt, patzer_createitem
	},

	{SPL_METEORRAIN, "Meteorregen",
		"Ein Schauer von Meteoren regnet über das Schlachtfeld.",
		NULL,
		NULL,
		M_GRAU, (COMBATSPELL | SPELLLEVEL), 5, 3,
		{
			{R_AURA, 1, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_kampfzauber, patzer
	},

	{SPL_ARTEFAKT_RUNESWORD, "Erschaffe ein Runenschwert",
		"Mit diesem Spruch erzeugt man ein Runenschwert. Die Klinge des "
		"schwarzen "
		"Schwertes ist mit alten, magischen Runen verziert, und ein seltsames "
		"Eigenleben erfüllt die warme Klinge. Um es zu benutzen, muss man "
		"ein Schwertkämpfer von beachtlichem Talent (7) sein. "
		"Der Träger des Runenschwertes erhält einen Talentbonus von +4 im Kampf "
		"und wird so gut wie immun gegen alle Formen von Magie.",
		NULL,
		NULL,
		M_GRAU, (ONSHIPCAST), 5, 6,
		{
			{R_AURA, 100, SPC_FIX},
			{R_PERMAURA, 1, SPC_FIX},
			{R_SILVER, 1000, SPC_FIX},
			{R_EOGSWORD, 1, SPC_FIX},
			{0, 0, 0}},
		(spell_f)sp_createitem_runesword, patzer_createitem
	},

	{SPL_BECOMEWYRM, "Wyrmtransformation",
		"Mit Hilfe dieses Zaubers kann sich der Magier permanent in einen "
		"mächtigen Wyrm verwandeln. Der Magier behält seine Talente und "
		"Möglichkeiten, bekommt jedoch die Kampf- und Bewegungseigenschaften "
		"eines Wyrms. Der Odem des Wyrms wird sich mit steigendem Magie-Talent "
		"verbessern. Der Zauber ist sehr kraftraubend und der Wyrm wird einige "
		"Zeit brauchen, um sich zu erholen.",
		NULL,
		NULL,
		M_GRAU, 0, 5, 1,
		{
			{R_AURA, 1, SPC_FIX},
			{R_PERMAURA, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_becomewyrm, patzer
	},

	/* Monstersprüche */

	{ SPL_FIREDRAGONODEM, "Feuriger Drachenodem",
		"Verbrennt die Feinde",
		NULL,
		NULL,
		M_GRAU, (COMBATSPELL), 5, 3,
		{
			{R_AURA, 1, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_dragonodem, patzer
	},

	{ SPL_DRAGONODEM, "Eisiger Drachenodem",
		"Tötet die Feinde",
		NULL,
		NULL,
		M_GRAU, (COMBATSPELL), 5, 6,
		{
			{R_AURA, 2, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_dragonodem, patzer
	},

	{ SPL_WYRMODEM, "Großer Drachenodem",
		"Verbrennt die Feinde",
		NULL,
		NULL,
		M_GRAU, (COMBATSPELL), 5, 12,
		{
			{R_AURA, 3, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_dragonodem, patzer
	},
	
	{ SPL_DRAINODEM, "Schattenodem",
		"Entzieht Talentstufen und macht Schaden wie Großer Odem",
		NULL,
		NULL,
		M_GRAU, (COMBATSPELL), 5, 12,
		{
			{R_AURA, 4, SPC_FIX},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
		(spell_f)sp_dragonodem, patzer
	},
	
	{SPL_AURA_OF_FEAR, "Gesang der Furcht",
		"Panik",
		NULL,
		NULL,
		M_GRAU, (COMBATSPELL), 5, 12,
		{
			{R_AURA, 1, SPC_LEVEL},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			{0, 0, 0}},
	 (spell_f)sp_flee, patzer
	},
	
	{SPL_SHADOWCALL, "Schattenruf",
		"Ruft Schattenwesen.",
		NULL,
		NULL,
	 M_GRAU, (PRECOMBATSPELL), 5, 12,
	 {
		 {R_AURA, 2, SPC_LEVEL},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 0}},
	 (spell_f)sp_shadowcall, patzer
	},

/* SPL_NOSPELL  MUSS der letzte Spruch der Liste sein*/

	{SPL_NOSPELL, "Keiner", NULL, NULL, NULL, 0, 0, 0, 0,
	 {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
	 NULL, NULL
	}
};
