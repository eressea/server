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
#include "magic.h"

#include "item.h"
#include "pool.h"
#include "message.h"
#include "faction.h"
#include "race.h"
#include "spell.h"
#include "ship.h"
#include "curse.h"
#include "building.h"
#include "region.h"
#include "goodies.h"
#include "plane.h"
#include "objtypes.h"
#include "unit.h"
#include "skill.h"
#include "pathfinder.h"
#include "karma.h"

#ifdef OLD_TRIGGER
#include "old/trigger.h"
#endif

/* util includes */
#include <resolve.h>
#include <base36.h>
#include <event.h>
#include <triggers/timeout.h>
#include <triggers/shock.h>
#include <triggers/killunit.h>
#include <triggers/giveitem.h>
#include <triggers/changerace.h>
#include <triggers/clonedied.h>

/* libc includes */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <math.h>

/* ------------------------------------------------------------- */

const char *magietypen[MAXMAGIETYP] =
{
	"Kein Magiegebiet",
	"Illaun",
	"Tybied",
	"Cerddor",
	"Gwyrrd",
	"Draig"
};

attrib_type at_reportspell = {
	"reportspell", NULL, NULL, NULL, NO_WRITE, NO_READ
};

/**
 ** at_icastle
 ** TODO: separate castle-appearance from illusion-effects
 **/

#ifdef FUZZY_BASE36
int fuzzy_hits;
extern boolean enable_fuzzy;
#endif /* FUZZY_BASE36 */

static ship *
findshipr(const region *r, int n)
	/* Ein Schiff in einer bestimmten Region finden: */
{
	ship * sh;

	for (sh = r->ships; sh; sh = sh->next) {
		if (sh->no == n) {
			assert(sh->region == r);
			return sh;
		}
	}
#ifdef FUZZY_BASE36
	if(enable_fuzzy) {
		n = atoi(itoa36(n));
		if (n) for (sh = r->ships; sh; sh = sh->next) {
			if (sh->no == n) {
				++fuzzy_hits;
				return sh;
			}
		}
	}
#endif

	return 0;
}

static building *
findbuildingr(const region *r, int n)
	/* Ein Gebäude in einer bestimmten Region finden: */
{
	building *b;

	for (b = rbuildings(r); b; b = b->next) {
		if (b->no == n) {
			return b;
		}
	}
#ifdef FUZZY_BASE36
	if(enable_fuzzy) {
		n = atoi(itoa36(n));
		if (n) for (b = rbuildings(r); b; b = b->next) {
			if (b->no == n) {
				++fuzzy_hits;
				return b;
			}
		}
	}
#endif

	return 0;
}

static int
a_readicastle(attrib * a, FILE * f)
{
	icastle_data * data = (icastle_data*)a->data.v;
	if (global.data_version<TYPES_VERSION) {
		union {
			int i;
			short sa[2];
		} old;
		int t;
		fscanf(f, "%d", &old.i);
		data->time = old.sa[1];
		t = old.sa[0];
		if (t<0 || t >= MAXBUILDINGS) t = 0;
		data->type = oldbuildings[t];
	} else {
		int bno;
		fscanf(f, "%s %d %d", buf, &bno, &data->time);
		data->building = findbuilding(bno);
		if (!data->building) {
			/* this shouldn't happen, but just in case it does: */
			ur_add((void*)bno, (void**)&data->building, resolve_building);
		}
		data->type = bt_find(buf);
	}
	return 1;
}

static void
a_writeicastle(const attrib * a, FILE * f)
{
	icastle_data * data = (icastle_data*)a->data.v;
	fprintf(f, "%s %d %d", data->type->_name, data->building->no, data->time);
}

static int
a_ageicastle(struct attrib * a)
{
	icastle_data * data = (icastle_data*)a->data.v;
	if (data->time<=0) {
		building * b = data->building;
		region * r = b->region;
		sprintf(buf, "Plötzlich löst sich %s in kleine Traumwolken auf.", buildingname(b));
		addmessage(r, 0, buf, MSG_EVENT, ML_INFO);
		/* destroy_building lets units leave the building */
		destroy_building(b);
		return 0;
	}
	else data->time--;
	return 1;
}

static void
a_initicastle(struct attrib * a)
{
	a->data.v = calloc(sizeof(icastle_data), 1);
}

static void
a_finalizeicastle(struct attrib * a)
{
	free(a->data.v);
}

attrib_type at_icastle = {
	"zauber_icastle",
	a_initicastle,
	a_finalizeicastle,
	a_ageicastle,
	a_writeicastle,
	a_readicastle
};

/* ------------------------------------------------------------- */

extern int dice(int count, int value);

/* ------------------------------------------------------------- */
/* aus dem alten System übriggebliegene Funktionen, die bei der
 * Umwandlung von alt nach neu gebraucht werden */
/* ------------------------------------------------------------- */

static void
init_mage(attrib * a) {
	a->data.v = calloc(sizeof(sc_mage), 1);
}

static void
free_mage(attrib * a)
{
	sc_mage * mage = (sc_mage*)a->data.v;
	freelist(mage->spellptr);
	free(mage);
}

static int
read_mage(attrib * a, FILE * F)
{
	int i;
	sc_mage * mage = (sc_mage*)a->data.v;
	spell_ptr ** sp = &mage->spellptr;
	fscanf(F, "%d %d %d", &mage->magietyp, &mage->spellpoints, &mage->spchange);
	for (i=0;i!=MAXCOMBATSPELLS;++i) {
		fscanf (F, "%d %d", &mage->combatspell[i], &mage->combatspelllevel[i]);
	}
	for (;;) {
		fscanf (F, "%d", &i);
		if (i < 0) break;
		*sp = calloc (sizeof(spell_ptr), 1);
		(*sp)->spellid = (spellid_t)i;
		sp = &(*sp)->next;
	}
	return 1;
}

static void
write_mage(const attrib * a, FILE * F) {
	int i;
	sc_mage *mage = (sc_mage*)a->data.v;
	spell_ptr *sp = mage->spellptr;
	fprintf (F, "%d %d %d ",
		mage->magietyp, mage->spellpoints, mage->spchange);
	for (i=0;i!=MAXCOMBATSPELLS;++i) {
		fprintf (F, "%d %d ", mage->combatspell[i], mage->combatspelllevel[i]);
	}
	while (sp) {
		fprintf (F, "%d ", sp->spellid);
		sp = sp->next;
	}
	fprintf (F, "-1\n");
}

attrib_type at_mage = {
	"mage",
	init_mage,
	free_mage,
	NULL,
	write_mage,
	read_mage,
	ATF_UNIQUE
};

boolean
is_mage(const unit * u)
{
	return i2b(get_mage(u) != NULL);
}

sc_mage *
get_mage(const unit * u)
{
	if (get_skill(u, SK_MAGIC) != 0) {
		attrib * a = a_find(u->attribs, &at_mage);
		if (a) return a->data.v;
	}
	return (sc_mage *) NULL;
}

magic_t
find_magetype(const unit * u)
{
	sc_mage *m;

	/* Null abfangen! */
	m = get_mage(u);

	if (!m)
		return 0;

	return m->magietyp;
}

/* ------------------------------------------------------------- */
/* Erzeugen eines neuen Magiers */

sc_mage *
create_mage(unit * u, magic_t mtyp)
{
	sc_mage *mage;
	attrib *a;
	int i;

	mage = calloc(1, sizeof(sc_mage));

	mage->magietyp = mtyp;
	mage->spellpoints = 0;
	mage->spchange = 0;
	mage->spellcount = 0;
	for (i=0;i<MAXCOMBATSPELLS;i++) {
		mage->combatspell[i] = SPL_NOSPELL;
	}
	mage->spellptr = NULL;
	a = a_add(&u->attribs, a_new(&at_mage));
	a->data.v = mage;
	createspelllist(u, mtyp);
	return mage;
}

/* ------------------------------------------------------------- */
/* ein 'neuer' Magier bekommt von Anfang an alle Sprüche seines
 * Magiegebietes mit einer Stufe kleiner oder gleich seinem Magietalent
 * in seine List-of-known-spells. Ausgenommen mtyp 0 (M_GRAU) */

void
createspelllist(unit *u, magic_t mtyp)
{
	int sk, i;
	if (mtyp == M_GRAU)
		return;

	sk = effskill(u, SK_MAGIC);
	if (sk == 0)
		return;

	for (i = 0; spelldaten[i].id != SPL_NOSPELL; i++) {
		if (spelldaten[i].magietyp == mtyp
				&& spelldaten[i].level <= sk)
		{
			if (!getspell(u, spelldaten[i].id))
				addspell(u, spelldaten[i].id);
		}
	}
}

/* ------------------------------------------------------------- */
/* Ausgabe der Spruchbeschreibungen
 * Anzeige des Spruchs nur, wenn die Stufe des besten Magiers vorher
 * kleiner war (u->faction->seenspells). Ansonsten muss nur geprüft
 * werden, ob dieser Magier den Spruch schon kennt, und andernfalls der
 * Spruch zu seiner List-of-known-spells hinzugefügt werden.
 */

attrib_type at_seenspell = {
	"seenspell", NULL, NULL, NULL, DEFAULT_WRITE, DEFAULT_READ
};

static boolean
already_seen(const faction * f, spellid_t id)
{
	attrib *a;

	for (a = a_find(f->attribs, &at_seenspell); a; a=a->nexttype)
		if (a->data.i==id) return true;
	return false;
}

void
updatespelllist(unit * u)
{
	int i, sk = eff_skill(u, SK_MAGIC, u->region);
	magic_t gebiet;
	int max = eff_skill(u, SK_MAGIC, u->region);

	gebiet = find_magetype(u);

	/* Magier mit keinem bzw M_GRAU bekommen weder Sprüche angezeigt noch
	 * neue Sprüche in ihre List-of-known-spells. Das sind zb alle alten
	 * Drachen, die noch den Skill Magie haben */
	if (gebiet == M_GRAU) return;

	for (i = 0; spelldaten[i].id != SPL_NOSPELL; i++) {
		boolean know = getspell(u, spelldaten[i].id);
		if (know || (spelldaten[i].magietyp == gebiet && spelldaten[i].level <= sk)) {
			if (!know) addspell(u, spelldaten[i].id);

			if (!already_seen(u->faction, spelldaten[i].id)) {
				a_add(&u->faction->attribs,
						a_new(&at_reportspell))->data.i = spelldaten[i].id;
				a_add(&u->faction->attribs,
						a_new(&at_seenspell))->data.i = spelldaten[i].id;
			}
		}
	}
	/* Nur Orkmagier bekommen den Keuschheitsamulettzauber */
	if (old_race(u->race) == RC_ORC
			&& !getspell(u, SPL_ARTEFAKT_CHASTITYBELT)
			&& find_spellbyid(SPL_ARTEFAKT_CHASTITYBELT)->level <= max)
	{
		addspell(u,SPL_ARTEFAKT_CHASTITYBELT);

		if (!already_seen(u->faction, spelldaten[i].id)) {
			a_add(&u->faction->attribs,
					a_new(&at_reportspell))->data.i = SPL_ARTEFAKT_CHASTITYBELT;
			a_add(&u->faction->attribs,
					a_new(&at_seenspell))->data.i = SPL_ARTEFAKT_CHASTITYBELT;
		}
	}

	/* Nur Wyrm-Magier bekommen den Wyrmtransformationszauber */
	if (fspecial(u->faction, FS_WYRM)
			&& !getspell(u, SPL_BECOMEWYRM)
			&& find_spellbyid(SPL_BECOMEWYRM)->level <= max)
	{
		addspell(u, SPL_BECOMEWYRM);
		if (!already_seen(u->faction, spelldaten[i].id)) {
			a_add(&u->faction->attribs,
					a_new(&at_reportspell))->data.i = SPL_BECOMEWYRM;
			a_add(&u->faction->attribs,
					a_new(&at_seenspell))->data.i = SPL_BECOMEWYRM;
		}
	}

	/* Transformierte Wyrm-Magier bekommen Drachenodem */
	if (dragonrace(u->race)) {
		race_t urc = old_race(u->race);
		switch (urc) {
		/* keine breaks! Wyrme sollen alle drei Zauber können.*/
			case RC_WYRM:
			{
				if(!getspell(u, SPL_WYRMODEM) &&
						find_spellbyid(SPL_WYRMODEM)->level <= max) {

					addspell(u, SPL_WYRMODEM);
					if (!already_seen(u->faction, spelldaten[i].id)) {
						a_add(&u->faction->attribs,
								a_new(&at_reportspell))->data.i = SPL_WYRMODEM;
						a_add(&u->faction->attribs,
								a_new(&at_seenspell))->data.i = SPL_WYRMODEM;
					}
				}
			}
			case RC_DRAGON:
			{
				if(!getspell(u, SPL_DRAGONODEM) &&
						find_spellbyid(SPL_DRAGONODEM)->level <= max) {

					addspell(u, SPL_DRAGONODEM);
					if (!already_seen(u->faction, spelldaten[i].id)) {
						a_add(&u->faction->attribs,
								a_new(&at_reportspell))->data.i = SPL_DRAGONODEM;
						a_add(&u->faction->attribs,
								a_new(&at_seenspell))->data.i = SPL_DRAGONODEM;
					}
				}
			}
			case RC_FIREDRAGON:
			{
				if(!getspell(u, SPL_FIREDRAGONODEM) &&
						find_spellbyid(SPL_FIREDRAGONODEM)->level <= max) {

					addspell(u, SPL_FIREDRAGONODEM);
					if (!already_seen(u->faction, spelldaten[i].id)) {
						a_add(&u->faction->attribs,
								a_new(&at_reportspell))->data.i = SPL_FIREDRAGONODEM;
						a_add(&u->faction->attribs,
								a_new(&at_seenspell))->data.i = SPL_FIREDRAGONODEM;
					}
				}
			}
		}
	}

	return;
}

/* ------------------------------------------------------------- */
/* Funktionen für die Bearbeitung der List-of-known-spells */

void
addspell(unit *u, spellid_t spellid)
{
	sc_mage *m;
	spell_ptr *newsp;

	m = get_mage(u);
	if (!m) {
		return;
	}
	newsp = calloc(1, sizeof(spell_ptr));
	newsp->spellid = spellid;

	addlist(&m->spellptr, newsp);
	return;
}

void
removespell(unit *u, spellid_t spellid)
{
	sc_mage *m;
	spell_ptr *spt;

	m = get_mage(u);
	if (!m) {
		return;
	}
	for (spt = m->spellptr; spt; spt = spt->next) {
		if (spt->spellid == spellid) {
			removelist(&m->spellptr, spt);
		}
	}

	return;
}

boolean
getspell(const unit *u, spellid_t spellid)
{
	sc_mage *m;
	spell_ptr *spt;

  m = get_mage(u);
  if (!m) {
		return false;
	}
	for (spt = m->spellptr; spt; spt = spt->next) {
		if (spt->spellid == spellid) {
			return true;
		}
	}
	return false;
}

/* ------------------------------------------------------------- */
/* Spruch identifizieren */

#include "umlaut.h"

#define SPELLNAMES_HACK 1
#if SPELLNAMES_HACK
struct tnode spellnames;

static void init_spellnames(void)
{
	int i;
	for (i=0; spelldaten[i].id != SPL_NOSPELL; i++)
		addtoken(&spellnames, spelldaten[i].name, (void*)i);
}
#endif

spell *
find_spellbyname(unit *u, char *s)
{
	sc_mage *m;
	spell_ptr *spt;
	spell *sp;

	m = get_mage(u);
	if (!m) {
		return (spell *) NULL;
	}

	for (spt = m->spellptr; spt; spt = spt->next) {
		sp = find_spellbyid(spt->spellid);
		if (!strncasecmp(sp->name, s, strlen(s))) {
			return sp;
		}
	}
#if SPELLNAMES_HACK /* enno: geht vorerst nicht mit locales */
	{
		static int init = 0;
		int i;
		if (!init) {
			init=1;
			init_spellnames();
		}
		if (findtoken(&spellnames, s, (void**)&i)==0 && getspell(u, spelldaten[i].id))
			return &spelldaten[i];
	}
#endif
	return (spell *) NULL;
}

spell *
find_spellbyid(spellid_t id)
{
	int i;

	for (i = 0; spelldaten[i].id != SPL_NOSPELL; i++) {
		if (spelldaten[i].id == id) {
			return &spelldaten[i];
		}
	}
	return (spell *) NULL;
}

/* ------------------------------------------------------------- */
/* Eingestellte Kampfzauberstufe ermitteln */

int
get_combatspelllevel(const unit *u, int nr)
{
	sc_mage *m;

	m = get_mage(u);
	if (!m || !(nr < MAXCOMBATSPELLS)) {
		return -1;
	}

	return min(m->combatspelllevel[nr], eff_skill(u, SK_MAGIC, u->region));
}

/* ------------------------------------------------------------- */
/* Kampfzauber ermitteln, setzen oder löschen */

spell*
get_combatspell(const unit *u, int nr)
{
	sc_mage *m;

	m = get_mage(u);
	if (!m || !(nr < MAXCOMBATSPELLS)) {
		return (spell *)NULL;
	}

	return find_spellbyid(m->combatspell[nr]);
}

void
set_combatspell(unit *u, spell *sp, const char * cmd, int level)
{
	sc_mage *m;

	m = get_mage(u);
	if (!m) return;

	/* knowsspell prüft auf ist_magier, ist_spruch, kennt_spruch */
	if (knowsspell(u->region, u, sp) == false){
		/* Fehler 'Spell not found' */
		cmistake(u, cmd, 173, MSG_MAGIC);
		return;
	}
	if (getspell(u, sp->id) == false) {
		/* Diesen Zauber kennt die Einheit nicht */
		cmistake(u, cmd, 169, MSG_MAGIC);
		return;
	}
	if (!(sp->sptyp & ISCOMBATSPELL)) {
		/* Diesen Kampfzauber gibt es nicht */
		cmistake(u, cmd, 171, MSG_MAGIC);
		return;
	}

	if (sp->sptyp & PRECOMBATSPELL) {
		m->combatspell[0] = sp->id;
		m->combatspelllevel[0] = level;
	} else if (sp->sptyp & COMBATSPELL) {
		m->combatspell[1] = sp->id;
		m->combatspelllevel[1] = level;
	} else if (sp->sptyp & POSTCOMBATSPELL) {
		m->combatspell[2] = sp->id;
		m->combatspelllevel[2] = level;
	}
	return;
}

void
unset_combatspell(unit *u, spell *sp)
{
	sc_mage *m;
	int nr = 0;
	int i;

	m = get_mage(u);
	if (!m) return;

	/* Kampfzauber löschen */
	if (!sp) {
		for (i=0;i<MAXCOMBATSPELLS;i++) {
			m->combatspell[i] = SPL_NOSPELL;
		}
	}
	else if (sp->sptyp & PRECOMBATSPELL) {
		if (sp != get_combatspell(u,0))
			return;
	} else if (sp->sptyp & COMBATSPELL) {
		if (sp != get_combatspell(u,1)) {
			return;
		} else {
			nr = 1;
		}
	} else if (sp->sptyp & POSTCOMBATSPELL) {
		if (sp != get_combatspell(u,2)) {
			return;
		} else {
			nr = 2;
		}
	}
	m->combatspell[nr] = SPL_NOSPELL;
	m->combatspelllevel[nr] = 0;
	return;
}

/* ------------------------------------------------------------- */
/* Gibt die aktuelle Anzahl der Magiepunkte der Einheit zurück */
int
get_spellpoints(const unit * u)
{
	sc_mage *m;

	m = get_mage(u);
	if (!m) return 0;

	return m->spellpoints;
}

void
set_spellpoints(unit * u, int sp)
{
	sc_mage *m;

	m = get_mage(u);
	if (!m) return;

	m->spellpoints = sp;

	return;
}

/*
 * verändert die Anzahl der Magiepunkte der Einheit um +mp
 */
int
change_spellpoints(unit * u, int mp)
{
	sc_mage *m;
	int sp;

	m = get_mage(u);
	if (!m) return 0;

	/* verhindere negative Magiepunkte */
	sp = max(m->spellpoints + mp, 0);
	m->spellpoints = sp;

	return sp;
}

/* bietet die Möglichkeit, die maximale Anzahl der Magiepunkte mit
 * Regionszaubern oder Attributen zu beinflussen
 */
int
get_spchange(const unit * u)
{
	sc_mage *m;

	m = get_mage(u);
	if (!m) return 0;

	return m->spchange;
}

/* ein Magier kann normalerweise maximal Stufe^2.1/1.2+1 Magiepunkte
 * haben.
 * Manche Rassen haben einen zusätzlichen Multiplikator
 * Durch Talentverlust (zB Insekten im Berg) können negative Werte
 * entstehen
 */

/* Artefakt der Stärke
 * Ermöglicht dem Magier mehr Magiepunkte zu 'speichern'
 */
/** TODO: at_skillmod daraus machen */
static int
use_item_aura(const region * r, const unit * u)
{
	int sk, n;

	sk = eff_skill(u, SK_MAGIC, r);
	n = (int)(sk * sk * u->race->maxaura / 4);

	return n;
}

int
max_spellpoints(const region * r, const unit * u)
{
	int sk, n;
	double msp;
	double potenz = 2.1;
	double divisor = 1.2;

	sk = eff_skill(u, SK_MAGIC, r);
	msp = u->race->maxaura*(pow(sk, potenz)/divisor+1) + get_spchange(u);

	if (get_item(u,I_AURAKULUM) > 0) {
		msp += use_item_aura(r, u);
	}
	n = get_curseeffect(u->attribs, C_AURA, 0);
	if (n>0) msp = (msp*n)/100;

	return max((int)msp, 0);
}

int
change_maxspellpoints(unit * u, int csp)
{
	sc_mage *m;

	m = get_mage(u);
	if (!m) return 0;

	m->spchange += csp;
	return max_spellpoints(u->region, u);
}

/* ------------------------------------------------------------- */
/* Counter für die bereits gezauberte Anzahl Sprüche pro Runde.
 * Um nur die Zahl der bereits gezauberten Sprüche zu ermitteln mit
 * step = 0 aufrufen.
 */
int
countspells(unit *u, int step)
{
	sc_mage *m;
	int count;

	m = get_mage(u);
	if (!m)
		return 0;

	if (step == 0)
		return m->spellcount;

	count = m->spellcount + step;

	/* negative Werte abfangen. */
	m->spellcount = max(0,count);

	return m->spellcount;
}

/* ------------------------------------------------------------- */
/* Die für den Spruch benötigte Aura pro Stufe.
 * Die Grundkosten pro Stufe werden hier um 2^count erhöht. Der
 * Parameter count ist dabei die Anzahl der bereits gezauberten Sprüche
 */
int
spellcost(unit *u, spell * sp)
{
	int k, aura = 0;
	int count;

	count = countspells(u,0);

	for (k = 0; k < MAXINGREDIENT; k++) {
		if (sp->komponenten[k][0] == R_AURA) {
			aura = sp->komponenten[k][1];
		}
	}
	aura *= (int)pow(2.0,(double)count);
	return aura;
}

/* ------------------------------------------------------------- */
/* SPC_LINEAR ist am höchstwertigen, dann müssen Komponenten für die
 * Stufe des Magiers vorhanden sein.
 * SPC_LINEAR hat die gewünschte Stufe als multiplikator,
 * nur SPC_FIX muss nur einmal vorhanden sein, ist also am
 * niedrigstwertigen und sollte von den beiden anderen Typen
 * überschrieben werden */
int
spl_costtyp(spell * sp)
{
	int k;
	int costtyp = SPC_FIX;

	for (k = 0; k < MAXINGREDIENT; k++) {
		if (costtyp == SPC_LINEAR) return SPC_LINEAR;

		if (sp->komponenten[k][2] == SPC_LINEAR) {
			return SPC_LINEAR;
		}

		/* wenn keine Fixkosten, Typ übernehmen */
		if (sp->komponenten[k][2] != SPC_FIX) {
			costtyp = sp->komponenten[k][2];
		}
	}
	return costtyp;
}

/* ------------------------------------------------------------- */
/* durch Komponenten und cast_level begrenzter maximal möglicher
 * Level
 * Da die Funktion nicht alle Komponenten durchprobiert sondern beim
 * ersten Fehler abbricht, muss die Fehlermeldung später mit cancast()
 * generiert werden.
 * */
int
eff_spelllevel(unit *u, spell * sp, int cast_level, int range)
{
	int k;
	int maxlevel;
	int needplevel;
	int costtyp = SPC_FIX;

	for (k = 0; k < MAXINGREDIENT; k++) {
		if (cast_level == 0)
			return 0;

		if (sp->komponenten[k][1] > 0) {
			/* Die Kosten für Aura sind auch von der Zahl der bereits
			 * gezauberten Sprüche abhängig */
			if (sp->komponenten[k][0] == R_AURA) {
				needplevel = spellcost(u, sp) * range;
			} else {
				needplevel = sp->komponenten[k][1] * range;
			}
			maxlevel = get_pooled(u, u->region, sp->komponenten[k][0])/needplevel;

			/* sind die Kosten fix, so muss die Komponente nur einmal vorhanden
			 * sein und der cast_level ändert sich nicht */
			if (sp->komponenten[k][2] == SPC_FIX) {
				if (maxlevel < 1) cast_level = 0;
			/* ansonsten wird das Minimum aus maximal möglicher Stufe und der
			 * gewünschten gebildet */
			} else if (sp->komponenten[k][2] == SPC_LEVEL) {
				costtyp = SPC_LEVEL;
				cast_level = min(cast_level, maxlevel);
			/* bei Typ Linear müssen die Kosten in Höhe der Stufe vorhanden
			 * sein, ansonsten schlägt der Spruch fehl */
			} else if (sp->komponenten[k][2] == SPC_LINEAR) {
				costtyp = SPC_LINEAR;
				if (maxlevel < cast_level) cast_level = 0;
			}
		}
	}
	/* Ein Spruch mit Fixkosten wird immer mit der Stufe des Spruchs und
	 * nicht auf der Stufe des Magiers gezaubert */
	if (costtyp == SPC_FIX) {
		cast_level = min(cast_level, sp->level);
	}

	return cast_level;
}

/* ------------------------------------------------------------- */
/* Die Spruchgrundkosten werden mit der Entfernung (Farcasting)
 * multipliziert, wobei die Aurakosten ein Sonderfall sind, da sie sich
 * auch durch die Menge der bereits gezauberten Sprüche erhöht.
 * Je nach Kostenart werden dann die Komponenten noch mit cast_level
 * multipliziert.
 */
void
pay_spell(unit * u, spell * sp, int cast_level, int range)
{
	int k;
	int resuse;

	for (k = 0; k != MAXINGREDIENT; k++) {
		if (sp->komponenten[k][0] == R_AURA) {
			resuse = spellcost(u, sp) * range;
		} else {
			resuse = sp->komponenten[k][1] * range;
		}

		if (sp->komponenten[k][2] == SPC_LINEAR
				|| sp->komponenten[k][2] == SPC_LEVEL)
		{
			resuse *= cast_level;
		}

		use_pooled(u, u->region, sp->komponenten[k][0], resuse);
	}
}


/* ------------------------------------------------------------- */
/* Ein Magier kennt den Spruch und kann sich die Beschreibung anzeigen
 * lassen, wenn diese in seiner Spruchliste steht. Zaubern muss er ihn
 * aber dann immer noch nicht können, vieleicht ist seine Stufe derzeit
 * nicht ausreichend oder die Komponenten fehlen.
 */
boolean
knowsspell(const region * r, const unit * u, const spell * sp)
{
	/* Ist überhaupt ein gültiger Spruch angegeben? */
	if (!sp || (sp->id == SPL_NOSPELL)) {
		return false;
	}
	/* Magier? */
	if (get_mage(u) == NULL) {
		log_warning(("%s ist kein Magier, versucht aber zu zaubern.\n",
				unitname(u)));
		return false;
	}
	/* steht der Spruch in der Spruchliste? */
	if (getspell(u, sp->id) == false){
		/* ist der Spruch aus einem anderen Magiegebiet? */
		if (find_magetype(u) != sp->magietyp){
			return false;
		}
		if (eff_skill(u, SK_MAGIC, u->region) >= sp->level){
			log_warning(("%s ist hat die erforderliche Stufe, kennt aber %s nicht.\n",
				unitname(u), sp->name));
		}
		return false;
	}

	/* hier sollten alle potentiellen Fehler abgefangen sein */
	return true;
}

/* Um einen Spruch zu beherrschen, muss der Magier die Stufe des
 * Spruchs besitzen, nicht nur wissen, das es ihn gibt (also den Spruch
 * in seiner Spruchliste haben).
 * Kosten für einen Spruch können Magiepunkte, Silber, Kraeuter
 * und sonstige Gegenstaende sein.
 */

boolean
cancast(unit * u, spell * sp, int level, int range, char * cmd)
{
	int k;
	resource_t res;
	int itemanz;
	boolean b = true;

	if (knowsspell(u->region, u, sp) == false) {
		/* Diesen Zauber kennt die Einheit nicht */
		cmistake(u, strdup(cmd), 173, MSG_MAGIC);
		return false;
	}
	/* reicht die Stufe aus? */
	if (eff_skill(u, SK_MAGIC, u->region) < sp->level) {
		log_warning(("Zauber von %s schlug fehl: %s braucht Stufe %d.\n",
				unitname(u), sp->name, sp->level));
		/* die Einheit ist nicht erfahren genug für diesen Zauber */
		cmistake(u, strdup(cmd), 169, MSG_MAGIC);
		return false;
	}

	for (k = 0; k < MAXINGREDIENT; k++) {
		if (sp->komponenten[k][1] > 0) {
			res = sp->komponenten[k][0];

			/* Die Kosten für Aura sind auch von der Zahl der bereits
			 * gezauberten Sprüche abhängig */
			if (sp->komponenten[k][0] == R_AURA) {
				itemanz = spellcost(u, sp) * range;
			} else {
				itemanz = sp->komponenten[k][1] * range;
			}

			/* sind die Kosten stufenabhängig, so muss itemanz noch mit dem
			 * level multipliziert werden */
			switch(sp->komponenten[k][2]) {
				case SPC_LEVEL:
				case SPC_LINEAR:
					itemanz *= level;
					break;
				case SPC_FIX:
				default:
					break;
			}

			if (get_pooled(u, u->region, res) < itemanz) {
				if (b == false) {
					/* es fehlte schon eine andere Komponente, wir basteln die
					 * Meldung weiter zusammen */
					scat(", ");
					icat(itemanz);
					scat(locale_string(u->faction->locale,
							resname(res, (itemanz == 1 ? 0 : 1))));
				} else {
					/* Noch fehlte keine Komponente, wir generieren den Anfang der
					 * Fehlermeldung */
					sprintf(buf, "%s in %s: 'ZAUBER %s' Für diesen Zauber fehlen "
							"noch %d ", unitname(u), regionid(u->region), sp->name,
							itemanz);
					scat(locale_string(u->faction->locale,
								resname(res, (itemanz == 1 ? 0 : 1))));
					b = false;
				}
			}
		}
	}
	if (b == false) {
		scat(".");
		addmessage(0, u->faction, buf, MSG_MAGIC, ML_MISTAKE);
		return false;
	}

	return true;
}

/* ------------------------------------------------------------- */
/* generische Spruchstaerke,
 *
 * setzt sich derzeit aus der Stufe des Magiers, Magieturmeffekt,
 * Spruchitems und Antimagiefeldern zusammen. Es koennen noch die
 * Stufe des Spruchs und Magiekosten mit einfliessen.
 *
 * Die effektive Spruchstärke und ihre Auswirkungen werden in der
 * Spruchfunktionsroutine ermittelt.
 */

int
spellpower(region * r, unit * u, spell * sp, int cast_level)
{
	int force = cast_level;
	if (sp==NULL) return 0;
	else {
		/* Bonus durch Magieturm und gesegneten Steinkreis */
		struct building * b = inside_building(u);
		const struct building_type * btype = b?b->type:NULL;
		if (btype == &bt_magictower) {
			force++;
		} else
		if (btype == &bt_blessedstonecircle){
			force++;
		}
	}
	if (get_item(u, I_RING_OF_POWER) > 0) {
		force++;
	}

	/* Antimagie in der Zielregion */
	if (is_spell_active(r, C_ANTIMAGICZONE)) {
		force -= get_curseeffect(r->attribs, C_ANTIMAGICZONE, 0);
		change_cursevigour(&r->attribs, C_ANTIMAGICZONE, 0, -cast_level);
		cmistake(u, findorder(u, u->thisorder), 185, MSG_MAGIC);
	}

	/* Patzerfluch-Effekt: */
	if (is_cursed(u->attribs, C_FUMBLE, 0) == true) {
		force -= get_curseeffect(u->attribs, C_FUMBLE, 0);
		change_cursevigour(&u->attribs, C_FUMBLE, 0, -1);
		cmistake(u, findorder(u, u->thisorder), 185, MSG_MAGIC);
	}

	return max(force, 0);
}

/* ------------------------------------------------------------- */
/* farcasting() == 1 -> gleiche Region, da man mit Null nicht vernünfigt
 * rechnen kann */
int
farcasting(unit *magician, region *r)
{
	int dist;
	int mult;

	if (!r) {
		return 1025;
	}

	dist = koor_distance(r->x, r->y, magician->region->x, magician->region->y);

	if (dist > 24) return 1025;

	mult = (int)pow(2.0,(double)dist);
	if (dist > 1) {
#ifdef NEW_PATH
		if (!path_exists(magician->region, r, dist*2, allowed_fly)) mult = 1025;
#else
		region *rn;
		for (rn=regions;rn;rn=rn->next) freset(rn, FL_DH);
		if (step(magician->region,r,FLY,dist*2) == false) {
			mult = 1025;
		}
#endif
	}

	return mult;
}


/* ------------------------------------------------------------- */
/* Antimagie - Magieresistenz */
/* ------------------------------------------------------------- */

/* allgemeine Magieresistenz einer Einheit,
 * reduziert magischen Schaden */
int
magic_resistance(unit *target)
{
	attrib * a;
	int chance;
	int n;

	/* Bonus durch Rassenmagieresistenz */
	chance = (int)(target->race->magres * 100);

	/* Auswirkungen von Zaubern auf der Einheit */
	if (is_cursed(target->attribs, C_MAGICRESISTANCE, 0)) {
		chance += get_curseeffect(target->attribs, C_MAGICRESISTANCE, 0) *
			get_cursedmen(target->attribs, C_MAGICRESISTANCE, 0) / target->number;
	}

	/* Unicorn +10 */
	n = get_item(target, I_UNICORN);
	if (n) chance += (10*n)/target->number;

	/* Auswirkungen von Zaubern auf der Region */
	a = a_find(target->region->attribs, &at_curse);
	while (a) {
		curse *c = (curse*)a->data.v;
		unit *mage = c->magician;

		if (c->cspellid == C_SONG_GOODMR) {
			if (mage != NULL) {
				if (allied(mage, target->faction, HELP_GUARD)) {
					chance += c->effect;
					break;
				}
			} else {
				a = a->nexttype;
				continue;
			}
		}
		a = a->nexttype;
	}
	a = a_find(target->region->attribs, &at_curse);
	while (a) {
		curse *c = (curse*)a->data.v;
		unit *mage = c->magician;

		if (c->cspellid == C_SONG_BADMR) {
			if (mage != NULL) {
				if (allied(mage, target->faction, HELP_GUARD)) {
					a = a->nexttype;
					continue;
				} else {
					chance -= c->effect;
					break;
				}
			}
		}
		a = a->nexttype;
	}

	/* Bonus durch Artefakte */
	/* TODO (noch gibs keine)*/

	/* Bonus durch Gebäude */
	{
		struct building * b = inside_building(target);
		const struct building_type * btype = b?b->type:NULL;
		/* gesegneter Steinkreis gibt 30% dazu */
		if (btype == &bt_blessedstonecircle){
			chance += 30;
		}
	}
	return chance;
}

/* ------------------------------------------------------------- */
/* Prüft, ob das Objekt dem Zauber widerstehen kann.
 * Objekte können Regionen, Units, Gebäude oder Schiffe sein.
 * TYP_UNIT:
 * Das höchste Talent des Ziels ist sein 'Magieresistenz-Talent', Magier
 * bekommen einen Bonus. Grundchance ist 50%, für jede Stufe
 * Unterschied gibt es 5%, minimalchance ist 5% für jeden (5-95%)
 * Scheitert der Spruch an der Magieresistenz, so gibt die Funktion
 * true zurück
 */

boolean
target_resists_magic(unit *magician, void *obj, int objtyp, int t_bonus)
{
	int chance = 0;

	if (magician == NULL) return true;
	if (obj == NULL) return true;

	switch(objtyp) {
		case TYP_UNIT:
			{
				int at, pa = 0;
				skill_t i;

				if(fspecial(((unit *)obj)->faction, FS_MAGICIMMUNE)) return true;

				at = effskill(magician, SK_MAGIC);

				for (i=0;i<MAXSKILLS;i++) {
					int sk = effskill((unit *)obj, i);
					if (pa < sk) pa = sk;
				}
				/* Magier haben einen Resistenzbonus vom Magietalent/2 */
				if (pa) pa += effskill((unit *)obj, SK_MAGIC)/2;

				/* Contest */
				chance = 5*(pa+10 - at);

				chance += magic_resistance((unit *)obj);
				break;
			}

		case TYP_REGION:
			/* Bonus durch Zauber */
			chance += get_curseeffect(((region *)obj)->attribs, C_RESIST_MAGIC, 0);
			break;

		case TYP_BUILDING:
			/* Bonus durch Zauber */
			chance += get_curseeffect(((building *)obj)->attribs, C_RESIST_MAGIC, 0);

			/* Bonus durch Typ */
			if (((building *)obj)->type == &bt_magictower)
				chance += 40;
			if (((building *)obj)->type == &bt_blessedstonecircle)
				chance += 60;


			break;

		case TYP_SHIP:
			/* Bonus durch Zauber */
			chance += get_curseeffect(((ship *)obj)->attribs, C_RESIST_MAGIC, 0);
			break;

		case TYP_FACTION:
		default:
			chance = 0;
	}

	chance = max(2, chance + t_bonus);
	chance = min(98, chance);

	/* gibt true, wenn die Zufallszahl kleiner als die chance ist und
	 * false, wenn sie gleich oder größer ist, dh je größer die
	 * Magieresistenz (chance) desto eher gibt die Funktion true zurück */
	if (rand()%100 < chance) {
		return true;
	}
	return false;
}

/* ------------------------------------------------------------- */

boolean
is_magic_resistant(unit *magician, unit *target, int resist_bonus)
{
	return (boolean)target_resists_magic(magician, target, TYP_UNIT, resist_bonus);
}

/* ------------------------------------------------------------- */
/* Patzer */
/* ------------------------------------------------------------- */
/* allgemeine Zauberpatzer:
 * Treten auf, wenn die Stufe des Magieres zu niedrig ist.
 * Abhaengig von Staerke des Spruchs.
 *
 * Die Warscheinlichkeit reicht von 20% (beherrscht den Spruch gerade
 * eben) bis zu etwa 1% bei doppelt so gut wie notwendig
 */

boolean
fumble(region * r, unit * u, spell * sp, int cast_grade)
{
/* X ergibt Zahl zwischen 1 und 0, je kleiner, desto besser der Magier.
 * 0,5*40-20=0, dh wenn der Magier doppelt so gut ist, wie der Spruch
 * benötigt, gelingt er immer, ist er gleich gut, gelingt der Spruch mit
 * 20% Warscheinlichkeit nicht
 * */

	int rnd = 0;
	double x = (double) cast_grade / (double) eff_skill(u, SK_MAGIC, r);
	int patzer = (int) (((double) x * 40.0) - 20.0);
	struct building * b = inside_building(u);
	const struct building_type * btype = b?b->type:NULL;

	if (btype==&bt_magictower) {
		patzer -= 10;
	}
	/* CHAOSPATZERCHANCE 10 : +10% Chance zu Patzern */
	if (sp->magietyp == M_CHAOS) {
		patzer += CHAOSPATZERCHANCE;
	}
	if (is_cursed(u->attribs, C_MBOOST, 0) == true) {
		patzer += CHAOSPATZERCHANCE;
	}
	if (is_cursed(u->attribs, C_FUMBLE, 0) == true) {
		patzer += CHAOSPATZERCHANCE;
	}

	/* wenn die Chance kleiner als 0 ist, können wir gleich false
	 * zurückgeben */
	if (patzer <= 0) {
#ifdef PATZERDEBUG
		printf("%s: Zauber Stufe %d, Talent %d, Patzerchance %d\n",
			unitname(u), cast_grade, eff_skill(u, SK_MAGIC, r), patzer);
#endif
		return false;
	}
	rnd = rand()%100;

#ifdef PATZERDEBUG
	printf("%s: Zauber Stufe %d, Talent %d, Patzerchance %d, rand: %d\n",
			unitname(u), cast_grade, eff_skill(u, SK_MAGIC, r), patzer, rnd);
#endif

	if (rnd > patzer) {
		/* Glück gehabt, kein Patzer */
		return false;
	}
	return true;
}

/* Die normalen Spruchkosten müssen immer bezahlt werden, hier noch
 * alle weiteren Folgen eines Patzers
 */

void
do_fumble(castorder *co)
{
	region * r = co->rt;
	unit * u = (unit*)co->magician;
	spell * sp = co->sp;
	int level = co->level;
	int duration;

	switch (rand() % 10) {
	/* wenn vorhanden spezieller Patzer, ansonsten nix */
	case 0:
		sp->patzer(co);
		break;
	case 1:
	/* Kröte */
		duration = rand()%level/2;
		if (duration<2) duration = 2;
#ifdef NEW_TRIGGER
		{
			/* one or two things will happen: the toad changes her race back,
			 * and may or may not get toadslime.
			 * The list of things to happen are attached to a timeout
			 * trigger and that's added to the triggerlit of the mage gone toad.
			 */
			trigger * trestore = trigger_changerace(u, u->race, u->irace);
			if (rand()%10>2) t_add(&trestore, trigger_giveitem(u, olditemtype[I_TOADSLIME], 1));
			add_trigger(&u->attribs, "timer", trigger_timeout(duration, trestore));
		}
#else
		{
			action *a1;
			timeout *t1;
			t1 = create_timeout(duration);
			a1 = action_changerace(u, TYP_UNIT, SPREAD_ALWAYS, u->race, u->irace);
			link_action_timeout(a1, t1); /* converted */
		}
#endif
		u->race = new_race[RC_TOAD];
		u->irace = new_race[RC_TOAD];
		sprintf(buf, "Eine Botschaft von %s: 'Ups! Quack, Quack!'", unitname(u));
		addmessage(r, 0, buf, MSG_MAGIC, ML_MISTAKE);
		break;
	case 2:
	/* temporärer Stufenverlust */
		duration = max(rand()%level/2, 2);
		create_curse(u, &u->attribs, C_SKILL, SK_MAGIC, level, duration,
				-(level/2), 1);
		add_message(&u->faction->msgs,
				new_message(u->faction, "patzer2%u:unit%r:region", u, r));
		break;
	case 3:
	case 4:
	/* Spruch schlägt fehl, alle Magiepunkte weg */
		set_spellpoints(u, 0);
		add_message(&u->faction->msgs, new_message(u->faction,
					"patzer3%u:unit%r:region%s:command",
					u, r, sp->name));
		break;

	case 5:
	case 6:
	/* Spruch gelingt, aber alle Magiepunkte weg */
		((nspell_f)sp->sp_function)(co);
		set_spellpoints(u, 0);
		sprintf(buf, "Als %s versucht, '%s' zu zaubern erhebt sich "
				"plötzlich ein dunkler Wind. Bizarre geisterhafte "
				"Gestalten kreisen um den Magier und scheinen sich von "
				"den magischen Energien des Zaubers zu ernähren. Mit letzter "
				"Kraft gelingt es %s dennoch den Spruch zu zaubern.",
				unitname(u), sp->name, unitname(u));
		addmessage(0, u->faction, buf, MSG_MAGIC, ML_WARN);
		break;

	case 7:
	case 8:
	case 9:
	default:
		/* Spruch gelingt, alle nachfolgenden Sprüche werden 2^4 so teuer */
		((nspell_f)sp->sp_function)(co);
		sprintf(buf, "%s fühlt sich nach dem Zaubern von %s viel erschöpfter "
				"als sonst und hat das Gefühl, dass alle weiteren Zauber deutlich "
				"mehr Kraft als normalerweise kosten werden.", unitname(u), sp->name);
		addmessage(0, u->faction, buf, MSG_MAGIC, ML_WARN);
		countspells(u,3);
	}

	return;
}

/* ------------------------------------------------------------- */
/* Regeneration von Aura */
/* ------------------------------------------------------------- */

/* Ein Magier regeneriert pro Woche W(Stufe^1.5/2+1), mindestens 1
 * Zwerge nur die Hälfte
 */
int
regeneration(unit * u)
{
	int sk, aura, d;
	double potenz = 1.5;
	double divisor = 2.0;

	if (fspecial(u->faction, FS_MAGICIMMUNE)) return 0;

	sk = effskill(u, SK_MAGIC);
	/* Rassenbonus/-malus */
	d = (int)(pow(sk, potenz) * u->race->regaura / divisor);
	d++;

	/* Einfluss von Artefakten */
	/* TODO (noch gibs keine)*/

	/* Würfeln */
	aura = (rand() % d + rand() % d)/2 + 1;

	return aura;
}

void
regeneration_magiepunkte(void)
{
	region *r;
	unit *u;
	int aura, auramax;
	double reg_aura;
	int n;

	for (r = regions; r; r = r->next) {
		for (u = r->units; u; u = u->next) {
			if (is_mage(u)) {
				aura = get_spellpoints(u);
				auramax = max_spellpoints(r, u);
				if (aura < auramax) {
					struct building * b = inside_building(u);
					const struct building_type * btype = b?b->type:NULL;
					reg_aura = (double)regeneration(u);

					/* Magierturm erhöht die Regeneration um 75% */
					if (btype == &bt_magictower) {
						reg_aura *= 1.75;
					}
					/* gesegnerter Steinkreis erhöht die Regeneration um 50% */
					if (btype == &bt_blessedstonecircle) {
						reg_aura *= 1.50;
					}

					/* Bonus/Malus durch Zauber */
					n = get_curseeffect(u->attribs, C_AURA, 0);
					if (n>0) {
						reg_aura = (reg_aura*n)/100;
					}

					/* Einfluss von Artefakten */
					/* TODO (noch gibs keine)*/

					/* maximal Differenz bis Maximale-Aura regenerieren
					 * mindestens 1 Aura pro Monat */
					reg_aura = max(1,reg_aura);
					reg_aura = min((auramax - aura), reg_aura);

					aura += (int)reg_aura;
					add_message(&u->faction->msgs, msg_message(
						"regenaura", "unit region amount", 
						u, r, (int)reg_aura));
				}
				set_spellpoints(u, min(aura, auramax));

				/* Zum letzten Mal Spruchliste aktualisieren */
				updatespelllist(u);
			}
		}
	}
}

/* ------------------------------------------------------------- */
/* ------------------------------------------------------------- */
/* Zuerst wird versucht alle noch nicht gefundenen Objekte zu finden
 * oder zu prüfen, ob das gefundene Objekt wirklich hätte gefunden
 * werden dürfen (nicht alle Zauber wirken global). Dabei zählen wir die
 * Misserfolge (failed).
 * Dann folgen die Tests der gefundenen Objekte auf Magieresistenz und
 * Sichtbarkeit. Dabei zählen wir die magieresistenten (resists)
 * Objekte. Alle anderen werten wir als Erfolge (success) */

/* gibt bei Misserfolg 0 zurück, bei Magieresistenz zumindeste eines
 * Objektes 1 und bei Erfolg auf ganzer Linie 2 */
int
verify_targets(castorder *co)
{
	unit *mage = (unit *)co->magician;
	spell *sp = co->sp;
	region *target_r = co->rt;
	spellparameter *sa = co->par;
	spllprm *spobj;
	int failed = 0;
	int resists = 0;
	int success = 0;
	int i;

	if (sa) {
		/* zuerst versuchen wir vorher nicht gefundene Objekte zu finden.
		 * Wurde ein Objekt durch globalsuche gefunden, obwohl der Zauber
		 * gar nicht global hätte suchen dürften, setzen wir das Objekt
		 * zurück. */
		for (i=0;i<sa->length;i++) {
			spobj = sa->param[i];

			switch(spobj->typ) {
				case SPP_UNIT_ID:
				{
					unit *u;
					if (sp->sptyp & SEARCHGLOBAL) {
						u = findunitg(spobj->data.i, target_r);
					} else {
						u = findunitr(target_r, spobj->data.i);
					}
					if (!u){ /* Einheit nicht gefunden */
						spobj->flag = TARGET_NOTFOUND;
						failed++;
						add_message(&mage->faction->msgs, new_message(mage->faction,
							"spellunitnotfound%u:unit%r:region%s:command%s:id",
							mage, mage->region, strdup(co->order),
							strdup(itoa36(spobj->data.i))));
						break;
					} else { /* Einheit wurde nun gefunden, pointer umsetzen */
						spobj->flag = 0;
						spobj->typ = SPP_UNIT;
						spobj->data.u = u;
					}
					break;
				}
				/* TEMP-Einheit */
				case SPP_TUNIT_ID:
				{
					unit *u;
						/* Versuch 1 : Region der Zauberwirkung */
					u = findnewunit(target_r, mage->faction, spobj->data.i);
					if (!u){ 
						/* Versuch 2 : Region des Magiers */
						u = findnewunit(mage->region, mage->faction, spobj->data.i);
					}
					if (!u && sp->sptyp & SEARCHGLOBAL) {
						/* Fehler: TEMP-Einheiten kann man nicht global suchen. der
						 * TEMP-Name gilt immer nur jeweils für die Region */
					}
					if (!u){ /* Einheit nicht gefunden */
						char tbuf[1024];
						spobj->flag = TARGET_NOTFOUND;
						failed++;
						/* Meldung zusammenbauen, sonst steht dort anstatt
						 * "TEMP <nummer>" nur "<nummer>" */
						sprintf(tbuf, "%s %s", LOC(mage->faction->locale,
									parameters[P_TEMP]), itoa36(spobj->data.i));
						add_message(&mage->faction->msgs, new_message(mage->faction,
							"spellunitnotfound%u:unit%r:region%s:command%s:id",
							mage, mage->region, strdup(co->order),
							strdup(tbuf)));
						break;
					} else {
						/* Einheit wurde nun gefunden, pointer umsetzen */
						spobj->flag = 0;
						spobj->typ = SPP_UNIT;
						spobj->data.u = u;
					}
					break;
				}
				case SPP_UNIT:
				{
					unit *u = spobj->data.u;
					if (!(sp->sptyp & SEARCHGLOBAL)
							&& !findunitr(target_r, u->no))
					{ /* die Einheit befindet sich nicht in der Zielregion */
						spobj->typ = SPP_UNIT_ID;
						spobj->data.i = u->no;
						spobj->flag = TARGET_NOTFOUND;
						failed++;
						add_message(&mage->faction->msgs, new_message(mage->faction,
							"spellunitnotfound%u:unit%r:region%s:command%s:id",
							mage, mage->region, strdup(co->order),
							strdup(itoa36(spobj->data.i))));
						break;
					}
					break;
				}
				case SPP_BUILDING_ID:
				{
					building *b;

					if (sp->sptyp & SEARCHGLOBAL) {
						b = findbuilding(spobj->data.i);
					} else {
						b = findbuildingr(target_r, spobj->data.i);
					}
					if (!b) { /* Burg nicht gefunden */
						spobj->flag = TARGET_NOTFOUND;
						failed++;
						add_message(&mage->faction->msgs, new_message(mage->faction,
							"spellbuildingnotfound%u:unit%r:region%s:command%i:id",
							mage, mage->region, strdup(co->order), spobj->data.i));
						break;
					} else {
						spobj->typ = SPP_BUILDING;
						spobj->flag = 0;
						spobj->data.b = b;
					}
					break;
				}
				case SPP_BUILDING:
				{
					building *b;
					b = spobj->data.b;

					if (!(sp->sptyp & SEARCHGLOBAL)
							&& !findbuildingr(target_r, b->no))
					{ /* die Burg befindet sich nicht in der Zielregion */
						spobj->typ = SPP_BUILDING_ID;
						spobj->data.i = b->no;
						spobj->flag = TARGET_NOTFOUND;
						failed++;
						add_message(&mage->faction->msgs, new_message(mage->faction,
							"spellbuildingnotfound%u:unit%r:region%s:command%i:id",
							mage, mage->region, strdup(co->order), spobj->data.i));
						break;
					}
					break;
				}
				case SPP_SHIP_ID:
				{
					ship *sh;

					if (sp->sptyp & SEARCHGLOBAL) {
						sh = findship(spobj->data.i);
					} else {
						sh = findshipr(target_r, spobj->data.i);
					}
					if (!sh) { /* Schiff nicht gefunden */
						spobj->flag = TARGET_NOTFOUND;
						failed++;
						add_message(&mage->faction->msgs, new_message(mage->faction,
							"spellshipnotfound%u:unit%r:region%s:command%i:id",
							mage, mage->region, strdup(co->order), spobj->data.i));
						break;
					} else {
						spobj->typ = SPP_SHIP;
						spobj->flag = 0;
						spobj->data.sh = sh;
					}
					break;
				}
				case SPP_SHIP:
				{
					ship *sh;
					sh = spobj->data.sh;

					if (!(sp->sptyp & SEARCHGLOBAL)
							&& !findshipr(target_r, sh->no))
					{ /* das Schiff befindet sich nicht in der Zielregion */
						spobj->typ = SPP_SHIP_ID;
						spobj->data.i = sh->no;
						spobj->flag = TARGET_NOTFOUND;
						failed++;
						add_message(&mage->faction->msgs, new_message(mage->faction,
							"spellshipnotfound%u:unit%r:region%s:command%i:id",
							mage, mage->region, strdup(co->order), spobj->data.i));
						break;
					}
					break;
				}
				case SPP_REGION:
				case SPP_INT:
				case SPP_STRING:
				default:
					break;
			}
		}
		/* Nun folgen die Tests auf cansee und Magieresistenz */
		for (i=0;i<sa->length;i++) {
			spobj = sa->param[i];

			switch(spobj->typ) {
				case SPP_UNIT:
				{
					unit *u;
					u = spobj->data.u;

					/* Wenn auf Sichtbarkeit geprüft werden soll und die Einheit
					 * nicht gesehen wird und auch nicht kontaktiert, dann melde
					 * 'Einheit nicht gefunden' */
					if ((sp->sptyp & TESTCANSEE)
							&& !cansee(mage->faction, target_r, u, 0)
							&& !ucontact(u, mage))
					{
						spobj->typ = SPP_UNIT_ID;
						spobj->data.i = u->no;
						spobj->flag = TARGET_NOTFOUND;
						add_message(&mage->faction->msgs, new_message(mage->faction,
							"spellunitnotfound%u:unit%r:region%s:command%s:id",
							mage, mage->region, strdup(co->order),
							strdup(itoa36(spobj->data.i))));
						failed++;
						break;
					}
					
					if ((sp->sptyp & TESTRESISTANCE)
							&& target_resists_magic(mage, u, TYP_UNIT, 0))
					{ /* Fehlermeldung */
						spobj->typ = SPP_UNIT_ID;
						spobj->data.i = u->no;
						spobj->flag = TARGET_RESISTS;
						resists++;
						add_message(&mage->faction->msgs, new_message(mage->faction,
							"spellunitresists%u:unit%r:region%s:command%s:id",
							mage, mage->region, strdup(co->order),
							strdup(itoa36(spobj->data.i))));
						break;
					}

					/* TODO: Test auf Parteieigenschaft Magieresistsenz */
					
					success++;
					break;
				}
				case SPP_BUILDING:
				{
					building *b;
					b = spobj->data.b;

					if ((sp->sptyp & TESTRESISTANCE)
								&& target_resists_magic(mage, b, TYP_BUILDING, 0))
					{ /* Fehlermeldung */
						spobj->typ = SPP_BUILDING_ID;
						spobj->data.i = b->no;
						spobj->flag = TARGET_RESISTS;
						resists++;
						add_message(&mage->faction->msgs, new_message(mage->faction,
							"spellbuildingresists%u:unit%r:region%s:command%i:id",
							mage, mage->region, strdup(co->order), spobj->data.i));
						break;
					}
					success++;
					break;
				}
				case SPP_SHIP:
				{
					ship *sh;
					sh = spobj->data.sh;

					if ((sp->sptyp & TESTRESISTANCE)
								&& target_resists_magic(mage, sh, TYP_SHIP, 0))
					{ /* Fehlermeldung */
						spobj->typ = SPP_SHIP_ID;
						spobj->data.i = sh->no;
						spobj->flag = TARGET_RESISTS;
						resists++;
						add_message(&mage->faction->msgs, new_message(mage->faction,
							"spellshipresists%u:unit%r:region%s:command%i:id",
							mage, mage->region, strdup(co->order), spobj->data.i));
						break;
					}
					success++;
					break;
				}
				case SPP_REGION:
				{ /* haben wir ein Regionsobjekt, dann wird auch dieses und
						 nicht target_r überprüft. */
					region *tr;
					tr = spobj->data.r;

					if (!tr) { /* Fehlermeldung */
						failed++;
						add_message(&mage->faction->msgs, new_message(mage->faction,
							"spellregionresists%u:unit%r:region%s:command",
							mage, mage->region, strdup(co->order)));
						break;
					}
					if ((sp->sptyp & TESTRESISTANCE)
							&& target_resists_magic(mage, tr, TYP_REGION, 0))
					{ /* Fehlermeldung */
						spobj->flag = TARGET_RESISTS;
						resists++;
						add_message(&mage->faction->msgs, new_message(mage->faction,
							"spellregionresists%u:unit%r:region%s:command",
							mage, mage->region, strdup(co->order)));
						break;
					}
					success++;
					break;
				}
				case SPP_INT:
				case SPP_STRING:
					success++;
					break;

				case SPP_SHIP_ID:
				case SPP_TUNIT_ID:
				case SPP_UNIT_ID:
				case SPP_BUILDING_ID:
				default:
					break;
			}
		}
	} else {
		/* der Zauber hat keine expliziten Parameter/Ziele, es kann sich
		 * aber um einen Regionszauber handeln. Wenn notwendig hier die
		 * Magieresistenz der Region prüfen. */
		if ((sp->sptyp & REGIONSPELL)) {
			/* Zielobjekt Region anlegen */
			sa = calloc(1, sizeof(spellparameter));
			sa->length = 1;
			sa->param = calloc(sa->length, sizeof(spllprm *));
			spobj = calloc(1, sizeof(spllprm));
			spobj->typ = SPP_REGION;
			spobj->data.r = target_r;
			sa->param[0] = spobj;
			co->par = sa;

			if ((sp->sptyp & TESTRESISTANCE)) {
				if (target_resists_magic(mage, target_r, TYP_REGION, 0)) {
					/* Fehlermeldung */
					add_message(&mage->faction->msgs, new_message(mage->faction,
						"spellregionresists%u:unit%r:region%s:command",
						mage, mage->region, strdup(co->order)));
					spobj->flag = TARGET_RESISTS;
					resists++;
				} else {
					success++;
				}
			} else {
				success++;
			}
		}
	}
	if (failed > 0) {
		/* mindestens ein Ziel wurde nicht gefunden */
		if (sa->length <= failed) {
			return 0;
		}
		if (success == 0) {
			/* kein Ziel war ein Erfolg */
			if (resists > 0){
				/* aber zumindest ein Ziel wurde gefunden, hat nur wiederstanden */
				return 1;
			} else {
				/* nur Fehlschläge */
				return 0;
			}
		}
	} /* kein Fehlschlag gefunden */
	if (resists > 0){
		return 1;
	}
	return 2;
}

/* ------------------------------------------------------------- */
/* Hilfsstrukturen für ZAUBERE */
/* ------------------------------------------------------------- */

void
free_spellparameter(spellparameter *pa)
{
	int i;

	/* Elemente free'en */
	for (i=0; i < pa->length; i++) {

		switch(pa->param[i]->typ) {
			case SPP_STRING:
				free(pa->param[i]->data.s);
				break;
			default:
				break;
		}
		free(pa->param[i]);
	}

	if (pa->param) free(pa->param);
	/* struct free'en */
	free(pa);
}


spellparameter *
add_spellparameter(region *target_r, unit *u, const char *syntax,
		char *s, int skip)
{
	int i = 1;
	int c = 0;
	int l = 0;
	char *tbuf, *token;
	spellparameter *par;
	spllprm *spobj;

	par = calloc(1, sizeof(spellparameter));

	/* Temporären Puffer initialisieren */
	tbuf = strdup(s);

	/* Tokens zählen */
	token = strtok(tbuf, " ");
	while(token) {
		par->length++;
		token = strtok(NULL, " ");
	}
	/* length sollte nun nur noch die Anzahl der für den Zauber relevanten
	 * Elemente enthalten */
	par->length -= skip; /* Anzahl der Elemente ('temp 123' sind zwei!) */

	/* mindestens ein Ziel (Ziellose Zauber werden nicht
	 * geparst) */
	if (par->length < 1) {
		/* Fehler: Ziel vergessen */
		cmistake(u, strdup(s), 203, MSG_MAGIC);
		/* Aufräumen */
		free(tbuf);
		free(par);
		return 0;
	}

	/* Pointer allozieren */
	par->param = calloc(par->length, sizeof(spllprm *));

	/* Tokens zuweisen */
	strcpy(tbuf, s);
	token = strtok (tbuf, " ");
	while(token && syntax[c] != 0) {
		if (i > skip) {
			if (syntax[c] == '+') {
				/* das vorhergehende Element kommt ein oder mehrmals vor, wir
				 * springen zum key zurück */
				c--;
			}
			if (syntax[c] == '?') {
				/* optionales Element vom Typ des nachfolgenden Zeichen, wir
				 * gehen ein Element weiter */
				c++;
			}
			spobj = calloc(1, sizeof(spllprm));
			switch(syntax[c]) {
				case 'u':
				{ /* Parameter ist eine Unitid */
					unit *ut;
					int unitname;

					/* Zuerst feststellen, ob es sich um eine Temp-Einheit
					 * handelt. Steht dort das Schlüsselwort EINHEIT, so hat
					 * garantiert jemand die Syntax falsch verstanden und die
					 * Einheitennummer kommt dahinter. In beiden Fällen wird der
					 * Befehl um ein token weiter eingelesen und es muß geprüft
					 * werden, ob der Befehlsstring überhaupt lang genug wäre. */
					switch (findparam(token, u->faction->locale)) {
						case P_TEMP:
							i++;
							if (par->length < i - skip) {
								/* Fehler: Ziel vergessen */
								cmistake(u, strdup(s), 203, MSG_MAGIC);
								/* Aufräumen */
								free(tbuf);
								free(spobj);
								par->length = l;
								free_spellparameter(par);
								return 0;
							}
							token = strtok(NULL, " ");
							unitname = atoi36(token);
							spobj->typ = SPP_TUNIT_ID;
							ut = findnewunit(u->region, u->faction, unitname);
							break;

						case P_UNIT:
							i++;
							if (par->length < i - skip) {
								/* Fehler: Ziel vergessen */
								cmistake(u, strdup(s), 203, MSG_MAGIC);
								/* Aufräumen */
								free(tbuf);
								free(spobj);
								par->length = l;
								free_spellparameter(par);
								return 0;
							}
							token = strtok(NULL, " ");
						default:
							unitname = atoi36(token);
							ut = findunitr(target_r, unitname);
					}

					if (ut) {
						spobj->typ = SPP_UNIT;
						spobj->data.u = ut;
					} else if (spobj->typ && spobj->typ == SPP_TUNIT_ID){
						/* die TEMP Einheit wurde nicht gefunden */
						spobj->data.i = unitname;
					} else {
						/* eine Zieleinheit wurde nicht gefunden, wir merken uns die
						 * Unitid für später. */
						spobj->typ = SPP_UNIT_ID;
						spobj->data.i = unitname;
					}
					break;
				}
				case 'r':
				{ /* Parameter sind zwei Regionskoordinaten */
					region *rt;
					int x, y, tx, ty;

					i++;
					if (par->length < i - skip) {
						/* Fehler: Zielregion vergessen */
						cmistake(u, strdup(s), 194, MSG_MAGIC);
						/* Aufräumen */
						free(tbuf);
						free(spobj);
						par->length = l;
						free_spellparameter(par);
						return 0;
					}
					tx = atoi(token);
					token = strtok(NULL, " ");
					ty = atoi(token);

					/* die relativen Koordinaten ins absolute Koordinatensystem
					 * umrechnen */
					x = rel_to_abs(0,u->faction,tx,0);
					y = rel_to_abs(0,u->faction,ty,1);
					rt = findregion(x,y);

					if (rt) {
						spobj->typ = SPP_REGION;
						spobj->data.r = rt;
					} else {
						/* Fehler: Zielregion vergessen */
						cmistake(u, strdup(s), 194, MSG_MAGIC);
						/* Aufräumen */
						free(tbuf);
						free(spobj);
						par->length = l;
						free_spellparameter(par);
						return 0;
					}
					break;
				}
				case 'b':
				{ /* Parameter ist eine Burgnummer */
					building *b;
					int b_id;

#ifdef FULL_BASE36
					b_id = atoi36(token);
#else
					b_id = atoi(token);
#endif
					b = findbuilding(b_id);

					if (b) {
						spobj->typ = SPP_BUILDING;
						spobj->data.b = b;
					} else {
						/* eine Burg mit der Nummer b_id wurde in dieser Region
						 * nicht gefunden, wir merken uns die b_id für später. */
						spobj->typ = SPP_BUILDING_ID;
						spobj->data.i = b_id;
					}
					break;
				}
				case 's':
				{ /* Parameter ist eine Schiffsnummer */
					ship *sh;
					int shid;

#ifdef FULL_BASE36
					shid = atoi36(token);
#else
					shid = atoi(token);
#endif
					sh = findshipr(target_r, shid);

					if (sh) {
						spobj->typ = SPP_SHIP;
						spobj->data.sh = sh;
					} else {
						/* ein Schiff mit der Nummer shipname wurde in dieser Region
						 * nicht gefunden, wir merken uns die shipname für später. */
						spobj->typ = SPP_SHIP_ID;
						spobj->data.i = shid;
					}
					break;
				}
				case 'c': /* Text, wird im Spruch ausgewertet */
					spobj->typ = SPP_STRING;
					spobj->data.s = strdup(token);
					break;
				case 'i': /* Zahl */
					spobj->typ = SPP_INT;
					spobj->data.i = atoi(token);
					break;
				case 'k':
				{ /* keyword, dieses Element beschreibt was für ein Typ nachfolgt */
					c++; /* das nächste Zeichen ist immer ein c für einen
									variablen Stringparameter */
					switch (findparam(token, u->faction->locale)) {
						case P_REGION:
						{
							spobj->typ = SPP_REGION;
							spobj->data.r = target_r;
							break;
						}
						case P_UNIT:
						{
							unit *ut;
							int unitname;
							i++;
							if (par->length < i - skip) {
								/* Fehler: Ziel vergessen */
								cmistake(u, strdup(s), 203, MSG_MAGIC);
								/* Aufräumen */
								free(tbuf);
								free(spobj);
								par->length = l;
								free_spellparameter(par);
								return 0;
							}
							token = strtok(NULL, " ");
							if (findparam(token, u->faction->locale) == P_TEMP) {
								i++;
								if (par->length < i - skip) {
									/* Fehler: Ziel vergessen */
									cmistake(u, strdup(s), 203, MSG_MAGIC);
									/* Aufräumen */
									free(tbuf);
									free(spobj);
									par->length = l;
									free_spellparameter(par);
									return 0;
								}
								token = strtok(NULL, " ");
								unitname = atoi36(token);
								ut = findnewunit(u->region, u->faction, unitname);
							} else {
								unitname = atoi36(token);
								ut = findunitr(target_r, unitname);
							}
							if (ut) {
								spobj->typ = SPP_UNIT;
								spobj->data.u = ut;
							} else {
								/* eine Zieleinheit wurde nicht gefunden, wir merken uns die
								 * Unitid für später. */
								spobj->typ = SPP_UNIT_ID;
								spobj->data.i = unitname;
							}
							break;
						}
						case P_BUILDING:
						case P_GEBAEUDE:
						{
							building *b;
							int b_id;

							i++;
							if (par->length < i - skip) {
								/* Fehler: Ziel vergessen */
								cmistake(u, strdup(s), 203, MSG_MAGIC);
								/* Aufräumen */
								free(tbuf);
								free(spobj);
								par->length = l;
								free_spellparameter(par);
								return 0;
							}
							token = strtok(NULL, " ");

#ifdef FULL_BASE36
							b_id = atoi36(token);
#else
							b_id = atoi(token);
#endif
							b = findbuilding(b_id);

							if (b) {
								spobj->typ = SPP_BUILDING;
								spobj->data.b = b;
							} else {
								/* eine Burg mit der Nummer b_id wurde in dieser Region
								 * nicht gefunden, wir merken uns die b_id für später. */
								spobj->typ = SPP_BUILDING_ID;
								spobj->data.i = b_id;
							}
							break;
						}
						case P_SHIP:
						{
							ship *sh;
							int shid;
							i++;
							if (par->length < i - skip) {
								/* Fehler: Ziel vergessen */
								cmistake(u, strdup(s), 203, MSG_MAGIC);
								/* Aufräumen */
								free(tbuf);
								free(spobj);
								par->length = l;
								free_spellparameter(par);
								return 0;
							}
							token = strtok(NULL, " ");
#ifdef FULL_BASE36
							shid = atoi36(token);
#else
							shid = atoi(token);
#endif
							sh = findshipr(target_r, shid);

							if (sh) {
								spobj->typ = SPP_SHIP;
								spobj->data.sh = sh;
							} else {
								/* ein Schiff mit der Nummer shipname wurde in dieser Region
								 * nicht gefunden, wir merken uns die shipname für später. */
								spobj->typ = SPP_SHIP_ID;
								spobj->data.i = shid;
							}
							break;
						}
						default:
							/* Syntax Error. */
							cmistake(u, strdup(s), 209, MSG_MAGIC);
							/* Aufräumen */
							free(tbuf);
							free(spobj);
							par->length = l;
							free_spellparameter(par);
							return 0;
					}
					break;
				}
				default:
					/* Syntax Error. */
					cmistake(u, strdup(s), 209, MSG_MAGIC);
					/* Aufräumen */
					free(tbuf);
					free(spobj);
					par->length = l;
					free_spellparameter(par);
					return 0;
			}
			par->param[l] = spobj;
			l++;
			c++;
		}
		i++;
		token = strtok(NULL, " ");
	}

	/* im Endeffekt waren es nur l parameter */
	par->length = l;

	/* das letzte Zeichen des Syntaxstrings sollte 0 oder ein +
	 * sein, ansonsten fehlte ein Parameter */
	if (syntax[c] != 0 && syntax[c] != '+' && syntax[c] != '?') {
		/* Syntax Error. */
		cmistake(u, strdup(s), 209, MSG_MAGIC);
		/* Aufräumen */
		free(tbuf);
		free_spellparameter(par);
		return 0;
	}

	/* Aufräumen */
	free(tbuf);

	return par;
}

/* ------------------------------------------------------------- */


strarray *
add_arglist(char *s, int skip)
{
	int i = 0;
	char *tbuf, *token;
	strarray *sa;

	sa = calloc(1, sizeof(strarray));

	/* Temporären Puffer initialisieren */
	tbuf = strdup(s);

	/* Tokens zählen */
	token = strtok (tbuf, " ");
	while(token) {
		sa->length++;
		token = strtok(NULL, " ");
	}
	sa->length -= skip;

	/* Pointer allozieren */
	sa->strings = calloc(sa->length, sizeof(char *));

	/* Tokens zuweisen */
	strcpy(tbuf, s);
	token = strtok (tbuf, " ");
	while(token) {
		if (i >= skip)
			sa->strings[i-skip] = strdup(token);
		i++;
		token = strtok(NULL, " ");
	}

	/* Aufräumen */
	free(tbuf);

	return sa;
}

void
freestrarray(strarray *sa)
{
	int l = sa->length;
	int i;

	/* Elemente free'en */
	for (i=0; i < l; i++) {
		free(sa->strings[i]);
	}

	/* struct free'en */
	free(sa);
}

/* ------------------------------------------------------------- */

castorder *
new_castorder(void *u, unit *u2, spell *sp, region *r, int lev,
		int force, int range, char *cmd, spellparameter *p)
{
	castorder *corder;

	corder = calloc(1, sizeof(castorder));
	corder->magician = u;
	corder->familiar = u2;
	corder->sp = sp;
	corder->level = lev;
	corder->force = force;
	corder->rt = r;
	corder->distance = range;
	corder->order = cmd;
	corder->par = p;

	return corder;
}

/* Hänge c-order co an die letze c-order von cll an */
void
add_castorder(castorder **cll, castorder *co)
{
	castorder *co2;

	/* Anfang der Liste? */
	if (*cll == NULL) {
		*cll = co;
		return;
	}

	/* suche letztes element */
	for (co2 = *cll; co2->next != NULL; co2 = co2->next) {
	}
	co2->next = co;
	return;
}

void
free_castorders(castorder *co)
{
	castorder *co2;

	while(co) {
		co2 = co;
		co = co->next;
		if (co2->par) {
			free_spellparameter(co2->par);
		}
		if (co2->order) free(co2->order);
		free(co2);
	}
	return;
}

/* ------------------------------------------------------------- */
/***
 ** at_familiarmage
 **/

typedef struct familiar_data {
	unit * mage;
	unit * familiar;
} famililar_data;

static void
write_unit(const attrib * a, FILE * F)
{
	unit * u = (unit*)a->data.v;
	write_unit_reference(u, F);
}

static int
sm_familiar(const unit * u, const region * r, skill_t sk, int value) /* skillmod */
{
	if (sk==SK_MAGIC) return value;
	else {
		int mod;
		unit * familiar = get_familiar(u);
		if (familiar==NULL) {
			log_error(("[sm_familiar] %s has a familiar-skillmod, but no familiar\n", unitname(u)));
			return value;
		}
		mod = eff_skill(familiar, sk, r)/2;
		if (r != familiar->region) {
			mod /= distance(r, familiar->region);
		}
		return value + mod;
	}
}

void
set_familiar(unit * mage, unit * familiar)
{
	/* if the skill modifier for the mage does not yet exist, add it */
	attrib * a = a_find(mage->attribs, &at_skillmod);
	while (a) {
		skillmod_data * smd = (skillmod_data *)a->data.v;
		if (smd->special==sm_familiar) break;
		a = a->nexttype;
	}
	if (a==NULL) {
		attrib * an = a_add(&mage->attribs, a_new(&at_skillmod));
		skillmod_data * smd = (skillmod_data *)an->data.v;
		smd->special = sm_familiar;
		smd->skill=NOSKILL;
	}

	a = a_find(mage->attribs, &at_familiar);
	if (a==NULL) {
		a = a_add(&mage->attribs, a_new(&at_familiar));
		a->data.v = familiar;
	} else assert(!a->data.v || a->data.v == familiar);
	/* TODO: Diese Attribute beim Tod des Familiars entfernen: */

	a = a_find(familiar->attribs, &at_familiarmage);
	if (a==NULL) {
		a = a_add(&familiar->attribs, a_new(&at_familiarmage));
		a->data.v = mage;
	} else assert(!a->data.v || a->data.v == mage);
}

void
remove_familiar(unit *mage)
{
	attrib *a = a_find(mage->attribs, &at_familiar);
	attrib *an;
	skillmod_data *smd;

	a_remove(&mage->attribs, a);
  a = a_find(mage->attribs, &at_skillmod);
  while (a) {
		an = a->nexttype;
		smd = (skillmod_data *)a->data.v;
	  if (smd->special==sm_familiar) a_remove(&mage->attribs, a);
		a = an;
	} 
}

void
create_newfamiliar(unit * mage, unit * familiar)
{
	/* if the skill modifier for the mage does not yet exist, add it */
	attrib * a = a_find(mage->attribs, &at_skillmod);
	while (a) {
		skillmod_data * smd = (skillmod_data *)a->data.v;
		if (smd->special==sm_familiar) break;
		a = a->nexttype;
	}
	if (a==NULL) {
		attrib * an = a_add(&mage->attribs, a_new(&at_skillmod));
		skillmod_data * smd = (skillmod_data *)an->data.v;
		smd->special = sm_familiar;
		smd->skill=NOSKILL;
	}

	a = a_find(mage->attribs, &at_familiar);
	if (a==NULL) {
		a = a_add(&mage->attribs, a_new(&at_familiar));
		a->data.v = familiar;
	} else assert(!a->data.v || a->data.v == familiar);
	/* TODO: Diese Attribute beim Tod des Familiars entfernen: */

	a = a_find(familiar->attribs, &at_familiarmage);
	if (a==NULL) {
		a = a_add(&familiar->attribs, a_new(&at_familiarmage));
		a->data.v = mage;
	} else assert(!a->data.v || a->data.v == mage);

	/* Wenn der Magier stirbt, dann auch der Vertraute */
	add_trigger(&mage->attribs, "destroy", trigger_killunit(familiar));
	/* Wenn der Vertraute stirbt, dann bekommt der Magier einen Schock */
	add_trigger(&familiar->attribs, "destroy", trigger_shock(mage));

}

static void *
resolve_familiar(void * data)
{
	unit * familiar = resolve_unit(data);
	if (familiar) {
		attrib * a = a_find(familiar->attribs, &at_familiarmage);
		if (a!=NULL && a->data.v) {
			unit * mage = (unit *)a->data.v;
			set_familiar(mage, familiar);
		}
	}
	return familiar;
}

static int
read_familiar(attrib * a, FILE * F)
{
	int i;
	fscanf(F, "%s", buf);
	i = atoi36(buf);
	ur_add((void*)i, &a->data.v, resolve_familiar);
	return 1;
}

/* clones */

void
create_newclone(unit * mage, unit * clone)
{
	attrib *a;

	a = a_find(mage->attribs, &at_clone);
	if (a == NULL) {
		a = a_add(&mage->attribs, a_new(&at_clone));
		a->data.v = clone;
	} else assert(!a->data.v || a->data.v == clone);
	/* TODO: Diese Attribute beim Tod des Klons entfernen: */

	a = a_find(clone->attribs, &at_clonemage);
	if (a == NULL) {
		a = a_add(&clone->attribs, a_new(&at_clonemage));
		a->data.v = mage;
	} else assert(!a->data.v || a->data.v == mage);

	/* Wenn der Magier stirbt, wird das in destroy_unit abgefangen.
	 * Kein Trigger, zu kompliziert. */

	/* Wenn der Klon stirbt, dann bekommt der Magier einen Schock */
	add_trigger(&clone->attribs, "destroy", trigger_clonedied(mage));
}

void
set_clone(unit * mage, unit * clone)
{
	attrib *a;

	a = a_find(mage->attribs, &at_clone);
	if (a==NULL) {
		a = a_add(&mage->attribs, a_new(&at_clone));
		a->data.v = clone;
	} else assert(!a->data.v || a->data.v == clone);

	a = a_find(clone->attribs, &at_clonemage);
	if (a==NULL) {
		a = a_add(&clone->attribs, a_new(&at_clonemage));
		a->data.v = mage;
	} else assert(!a->data.v || a->data.v == mage);
}

unit *
has_clone(unit *mage)
{
	attrib *a = a_find(mage->attribs, &at_clone);
	if(a) return (unit *)a->data.v;
	return NULL;
}

static void *
resolve_clone(void * data)
{
	unit * clone = resolve_unit(data);
	if (clone) {
		attrib * a = a_find(clone->attribs, &at_clonemage);
		if (a!=NULL && a->data.v) {
			unit * mage = (unit *)a->data.v;
			set_clone(mage, clone);
		}
	}
	return clone;
}

static int
read_clone(attrib * a, FILE * F)
{
	int i;
	fscanf(F, "%s", buf);
	i = atoi36(buf);
	ur_add((void*)i, &a->data.v, resolve_clone);
	return 1;
}

/* mages */

static void *
resolve_mage(void * data)
{
	unit * mage = resolve_unit(data);
	if (mage) {
		attrib * a = a_find(mage->attribs, &at_familiar);
		if (a!=NULL && a->data.v) {
			unit * familiar = (unit *)a->data.v;
			set_familiar(mage, familiar);
		}
	}
	return mage;
}

static int
read_magician(attrib * a, FILE * F)
{
	int i;
	fscanf(F, "%s", buf);
	i = atoi36(buf);
	ur_add((void*)i, &a->data.v, resolve_mage);
	return 1;
}

static int
age_unit(attrib * a)
	/* if unit is gone or dead, remove the attribute */
{
	unit * u = (unit*)a->data.v;
	return (u!=NULL && u->number>0);
}

attrib_type at_familiarmage = {
	"familiarmage",
	NULL,
	NULL,
	age_unit,
	write_unit,
	read_magician,
	ATF_UNIQUE
};

attrib_type at_familiar = {
	"familiar",
	NULL,
	NULL,
	age_unit,
	write_unit,
	read_familiar,
	ATF_UNIQUE
};

attrib_type at_clonemage = {
	"clonemage",
	NULL,
	NULL,
	age_unit,
	write_unit,
	read_magician,
	ATF_UNIQUE
};

attrib_type at_clone = {
	"clone",
	NULL,
	NULL,
	age_unit,
	write_unit,
	read_clone,
	ATF_UNIQUE
};

unit *
get_familiar(const unit *u)
{
	attrib * a = a_find(u->attribs, &at_familiar);
	if (a!=NULL) return (unit*)a->data.v;
	return NULL;
}

unit *
get_familiar_mage(const unit *u)
{
	attrib * a = a_find(u->attribs, &at_familiarmage);
	if (a!=NULL) return (unit*)a->data.v;
	return NULL;
}

unit *
get_clone(const unit *u)
{
	attrib * a = a_find(u->attribs, &at_clone);
	if (a!=NULL) return (unit*)a->data.v;
	return NULL;
}

unit *
get_clone_mage(const unit *u)
{
	attrib * a = a_find(u->attribs, &at_clonemage);
	if (a!=NULL) return (unit*)a->data.v;
	return NULL;
}

static boolean
is_moving_ship(const region * r, const ship *sh)
{
	const unit *u;
	int todo;

	u = shipowner(r, sh);
	todo = igetkeyword(u->thisorder, u->faction->locale);
	if (todo == K_ROUTE || todo == K_MOVE || todo == K_FOLLOW){
		return true;
	}
	return false;
}

/* ------------------------------------------------------------- */
/* Damit man keine Rituale in fremden Gebiet machen kann, diese vor
 * Bewegung zaubern. Magier sind also in einem fremden Gebiet eine Runde
 * lang verletzlich, da sie es betreten, und angegriffen werden können,
 * bevor sie ein Ritual machen können.
 *
 * Syntax: ZAUBER [REGION X Y] [STUFE <stufe>] "Spruchname" [Einheit-1
 * Einheit-2 ..]
 *
 * Nach Priorität geordnet die Zauber global auswerten.
 *
 * Die Kosten für Farcasting multiplizieren sich mit der Entfernung,
 * cast_level gibt die virtuelle Stufe an, die den durch das Farcasten
 * entstandenen Spruchkosten entspricht.  Sind die Spruchkosten nicht
 * levelabhängig, so sind die Kosten nur von der Entfernung bestimmt,
 * die Stärke/Level durch den realen Skill des Magiers
 */

void
magic(void)
{
	region *r;
	region *target_r;
	unit *u;           /* Aktuelle unit in Region */
	unit *familiar;    /* wenn u ein Vertrauter ist */
	unit *mage;        /* derjenige, der den Spruch am Ende zaubert */
	spell *sp;
	char *s;
	strlist *so;
	int spellrank;
	int level, success;
	int force, range, t_x, t_y;
	int skiptokens;
	castorder *co;
	castorder *cll[MAX_SPELLRANK];
	spellparameter *args;

	for (spellrank = 0; spellrank < MAX_SPELLRANK; spellrank++) {
		cll[spellrank] = (castorder*)NULL;
	}

	for (r = regions; r; r = r->next) {
		for (u = r->units; u; u = u->next) {
			boolean casted = false;

			if (old_race(u->race) == RC_SPELL || fval(u, FL_LONGACTION))
				continue;

			if (old_race(u->race) == RC_INSECT && r_insectstalled(r) && 
					!is_cursed(u->attribs, C_KAELTESCHUTZ,0))
				continue;

			if (attacked(u)) {
				continue;
			}

			for (so = u->orders; so; so = so->next) {
				if (igetkeyword(so->s, u->faction->locale) == K_CAST) {
#if HUNGER_DISABLES_LONGORDERS
					if (fval(u, FL_HUNGER)) {
						cmistake(u, so->s, 224, MSG_MAGIC);
						continue;
					}
#endif
					if (r->planep && fval(r->planep, PFL_NOMAGIC)) {
						cmistake(u, so->s, 269, MSG_MAGIC);
						continue;
					}
					casted = true;
					target_r = r;
					mage = u;
					level = eff_skill(u, SK_MAGIC, r);
					familiar = NULL;
					skiptokens = 1;
					s = getstrtoken();
					/* für Syntax ' STUFE x REGION y z ' */
					if (findparam(s, u->faction->locale) == P_LEVEL) {
						s = getstrtoken();
						level = min(atoip(s), level);
						s = getstrtoken();
						skiptokens += 2;
						if (level < 1) {
							/* Fehler "Das macht wenig Sinn" */
							cmistake(u, so->s, 10, MSG_MAGIC);
							continue;
						}
					}
					if (findparam(s, u->faction->locale) == P_REGION) {
						t_x = atoi(getstrtoken());
						t_x = rel_to_abs(getplane(u->region),u->faction,t_x,0);
						t_y = atoi(getstrtoken());
						t_y = rel_to_abs(getplane(u->region),u->faction,t_y,1);
						target_r = findregion(t_x, t_y);
						s = getstrtoken();
						skiptokens += 3;
						if (!target_r) {
							/* Fehler "Es wurde kein Zauber angegeben" */
							cmistake(u, so->s, 172, MSG_MAGIC);
							continue;
						}
					}
					/* für Syntax ' REGION x y STUFE z '
					 * hier nach REGION nochmal auf STUFE prüfen */
					if (findparam(s, u->faction->locale) == P_LEVEL) {
						s = getstrtoken();
						level = min(atoip(s), level);
						s = getstrtoken();
						skiptokens += 2;
						if (level < 1) {
							/* Fehler "Das macht wenig Sinn" */
							cmistake(u, so->s, 10, MSG_MAGIC);
							continue;
						}
					}
					if (!s[0] || strlen(s) == 0) {
						/* Fehler "Es wurde kein Zauber angegeben" */
						cmistake(u, so->s, 172, MSG_MAGIC);
						continue;
					}
					sp = find_spellbyname(u, s);

					/* Vertraute können auch Zauber sprechen, die sie selbst nicht
					 * können. find_spellbyname findet aber nur jene Sprüche, die
					 * die Einheit beherrscht. */
					if (sp == NULL && is_familiar(u)) {
						familiar = u;
						mage = get_familiar_mage(u);
						sp = find_spellbyname(mage, s);
					}

					if (sp == NULL) {
						/* Fehler 'Spell not found' */
						cmistake(u, so->s, 173, MSG_MAGIC);
						continue;
					}
					/* um testen auf spruchnamen zu unterbinden sollte vor allen
					 * fehlermeldungen die anzeigen das der magier diesen Spruch
					 * nur in diese Situation nicht anwenden kann, noch eine
					 * einfache Sicherheitsprüfung kommen */
					if (knowsspell(r, u, sp) == false){
						/* vorsicht! u kann der familiar sein */
						if (!familiar){
							cmistake(u, so->s, 173, MSG_MAGIC);
							continue;
						}
					}
					if (sp->sptyp & ISCOMBATSPELL) {
						/* Fehler: "Dieser Zauber ist nur im Kampf sinnvoll" */
						cmistake(u, so->s, 174, MSG_MAGIC);
						continue;
					}
					/* Auf dem Ozean Zaubern als quasi-langer Befehl können
					 * normalerweise nur Meermenschen, ausgenommen explizit als
					 * OCEANCASTABLE deklarierte Sprüche */
					if (rterrain(r) == T_OCEAN) {
						if (old_race(u->race) != RC_AQUARIAN
								&& !fval(u->race, RCF_SWIM)
								&& !(sp->sptyp & OCEANCASTABLE)) {
							/* Fehlermeldung */
								continue;
						}
					/* Auf bewegenden Schiffen kann man nur explizit als
					 * ONSHIPCAST deklarierte Zauber sprechen */
					} else if (u->ship) {
						if (is_moving_ship(r, u->ship)) {
							if (!(sp->sptyp & ONSHIPCAST)) {
								/* Fehler: "Diesen Spruch kann man nicht auf einem sich
								 * bewegenden Schiff stehend zaubern" */
								cmistake(u, so->s, 175, MSG_MAGIC);
								continue;
							}
						}
					}
					/* Farcasting bei nicht farcastbaren Sprüchen abfangen */
					range = farcasting(u, target_r);
					if (range > 1) {
						if (!(sp->sptyp & FARCASTING)) {
							/* Fehler "Diesen Spruch kann man nicht in die Ferne
							 * richten" */
							cmistake(u, so->s, 176, MSG_MAGIC);
							continue;
						}
						if (range > 1024) { /* (2^10) weiter als 10 Regionen entfernt */
							sprintf(buf, "%s in %s: 'ZAUBER %s' Zu der Region %s kann keine "
									"Verbindung hergestellt werden", unitname(u),
									regionid(u->region), sp->name, regionid(target_r));
							addmessage(0, u->faction, buf, MSG_MAGIC, ML_MISTAKE);
							continue;
						}
					}
					/* Stufenangabe bei nicht Stufenvariierbaren Sprüchen abfangen */
					if (!(sp->sptyp & SPELLLEVEL)) {
						int ilevel = eff_skill(u, SK_MAGIC, u->region);
						if (ilevel!=level) {
							level = ilevel;
							sprintf(buf, "%s in %s: 'ZAUBER %s' Dieser Zauber kann nicht "
									"mit Stufenangabe gezaubert werden.", unitname(u),
									regionid(u->region), sp->name);
							addmessage(0, u->faction, buf, MSG_MAGIC, ML_WARN);
						}
					}
					/* Vertrautenmagie */
					/* Kennt der Vertraute den Spruch, so zaubert er ganz normal.
					 * Ansonsten zaubert der Magier durch seinen Vertrauten, dh
					 * zahlt Komponenten und Aura. Dabei ist die maximale Stufe
					 * die des Vertrauten!
					 * Der Spruch wirkt dann auf die Region des Vertrauten und
					 * gilt nicht als Farcasting. */
					if (familiar || is_familiar(u)) {
						if ((sp->sptyp & NOTFAMILIARCAST)) {
							/* Fehler: "Diesen Spruch kann der Vertraute nicht zaubern" */
							cmistake(u, so->s, 177, MSG_MAGIC);
							continue;
						}
						if (!knowsspell(r, u, sp)) { /* Magier zaubert durch Vertrauten */
							mage = get_familiar_mage(u);
							if (range > 1) { /* Fehler! Versucht zu Farcasten */
								sprintf(buf, "%s kann Sprüche, die durch %s wirken, nicht "
										"zusätzlich nochmal in die Ferne richten.",
										unitname(mage), unitname(u));
								addmessage(0, u->faction, buf, MSG_MAGIC, ML_MISTAKE);
								continue;
							}
							if (distance(mage->region, r) > eff_skill(mage, SK_MAGIC, mage->region)) {
								sprintf(buf, "%s kann nicht genug Kraft aufbringen, um "
										"durch %s zu wirken", unitname(mage), unitname(u));
								addmessage(0, u->faction, buf, MSG_MAGIC, ML_MISTAKE);
								continue;
							}
							/* mage auf magier setzen, level anpassen, range für Erhöhung
							 * der Spruchkosten nutzen, langen Befehl des Magiers
							 * löschen, zaubern kann er noch */
							range *= 2;
							set_string(&mage->thisorder, "");
							level = min(level, eff_skill(mage, SK_MAGIC, mage->region)/2);
							familiar = u;
						}
					}
					/* Weitere Argumente zusammenbasten */
					if (sp->parameter) {
						++skiptokens;
						args = add_spellparameter(target_r, mage, sp->parameter,
							    so->s, skiptokens);
						if (!args) {
							/* Syntax war falsch */
							continue;
						}
					} else {
						args = (spellparameter *) NULL;
					}
					co = new_castorder(mage, familiar, sp, target_r, level, 0, range,
							strdup(so->s), args);
					add_castorder(&cll[(int)(sp->rank)], co);
				}
			}
			if (casted) fset(u, FL_LONGACTION);
		}
	}
	for (spellrank = 0; spellrank < MAX_SPELLRANK; spellrank++) {
		for (co = cll[spellrank]; co; co = co->next) {
			char *cmd = co->order;

			u = (unit *)co->magician;
			sp = co->sp;
			level = co->level; /* Talent des Magiers oder gewünschte Stufe */
			target_r = co->rt;

			/* Da sich die Aura und Komponenten in der Zwischenzeit verändert
			 * haben können und sich durch vorherige Sprüche das Zaubern
			 * erschwert haben kann, muss hier erneut geprüft werden, ob der
			 * Spruch überhaupt gezaubert werden kann.
			 * (level) die effektive Stärke des Spruchs (= Stufe, auf der der
			 * Spruch gezaubert wird) */

			/* reichen die Komponenten nicht, wird der Level reduziert. */
			level = eff_spelllevel(u, sp, level, co->distance);
			if (level < 1) {
				/* Fehlermeldung mit Komponenten generieren */
				cancast(u, sp, co->level, co->distance, cmd);
				continue;
			}
			if (level < co->level){
				/* Sprüche mit Fixkosten werden immer auf Stufe des Spruchs
				 * gezaubert, co->level ist aber defaultmäßig Stufe des Magiers */
				if (spl_costtyp(sp) != SPC_FIX) {
					sprintf(buf, "%s hat nur genügend Komponenten um %s auf Stufe %d "
							"zu zaubern.", unitname(u), sp->name, level);
					addmessage(0, u->faction, buf, MSG_MAGIC, ML_INFO);
				}
			}
			co->level = level;

			/* Prüfen, ob die realen Kosten für die gewünschten Stufe bezahlt
			 * werden können */
			if (cancast(u, sp, level, co->distance, cmd) == false) {
				/* die Fehlermeldung wird in cancast generiert */
				continue;
			}

			/* die Stärke kann durch Antimagie auf 0 sinken */
			force = spellpower(target_r, u, sp, level);
			if (force < 1) {
				sprintf(buf, "%s schafft es nicht genügend Kraft aufzubringen "
						"um %s dennoch zu zaubern.", unitname(u), sp->name);
				addmessage(0, u->faction, buf, MSG_MAGIC, ML_MISTAKE);
				continue;
			}
			co->force = force;

			/* Ziele auf Existenz prüfen und Magieresistenz feststellen. Wurde
			 * kein Ziel gefunden, so ist verify_targets=0. Scheitert der
			 * Spruch an der Magieresistenz, so ist verify_targets = 1, bei
			 * Erfolg auf ganzer Linie ist verify_targets= 2 
			 */
			switch (verify_targets(co)){
				case 0:
					/* kein Ziel gefunden, Fehlermeldungen sind in verify_targets */
					continue; /* äußere Schleife, nächster Zauberer */
				case 1:
				{ /* einige oder alle Ziele waren magieresistent */
					spellparameter *pa = co->par;
					int n;
					for (n = 0; n < pa->length; n++) {
						if(pa->param[n]->flag != TARGET_RESISTS 
								&& pa->param[n]->flag != TARGET_NOTFOUND)
						{ /* mindestens ein erfolgreicher Zauberversuch, wir machen
								 normal weiter */
							break;
						}
					} 
					/* zwar wurde mindestens ein Ziel gefunden, das widerstand
					 * jedoch dem Zauber. Kosten abziehen und abbrechen. */
					pay_spell(u, sp, level, co->distance);
					sprintf(buf, "%s gelingt es %s zu zaubern, doch der Spruch zeigt "
							"keine Wirkung.", unitname(u), sp->name);
					addmessage(0, u->faction, buf, MSG_MAGIC, ML_MISTAKE);
					continue; /* äußere Schleife, nächster Zauberer */
				}
				case 2:
				default: 
					/* Zauber war erfolgreich */
					break;
			}

			/* Auch für Patzer gibt es Erfahrung, müssen die Spruchkosten
			 * bezahlt werden und die nachfolgenden Sprüche werden teurer */
			if (fumble(target_r, u, sp, level) == true) {
				/* zuerst bezahlen, dann evt in do_fumble alle Aura verlieren */
				pay_spell(u, sp, level, co->distance);
				do_fumble(co);
				countspells(u,1);
				continue;
			}
			success = ((nspell_f)sp->sp_function)(co);
			if (success > 0) {
				/* Kosten nur für real benötige Stufe berechnen */
				pay_spell(u, sp, success, co->distance);
				/* erst bezahlen, dann Kostenzähler erhöhen */
				countspells(u,1);
			}
		}
	}

	/* Sind alle Zauber gesprochen gibts Erfahrung */
	for (r = regions; r; r = r->next) {
		for (u = r->units; u; u = u->next) {
			if (is_mage(u) && countspells(u,0) > 0) {
				change_skill(u, SK_MAGIC, PRODUCEEXP);
				/* Spruchlistenaktualiesierung ist in Regeneration */
			}
		}
	}
	for (spellrank = 0; spellrank < MAX_SPELLRANK; spellrank++) {
		free_castorders(cll[spellrank]);
	}
}

attrib *
create_special_direction(region *r, int x, int y, int duration,
						 const char *desc, const char *keyword)

{
	attrib *a = a_add(&r->attribs, a_new(&at_direction));
	spec_direction *d = (spec_direction *)(a->data.v);

	d->x = x;
	d->y = y;
	d->duration = duration;
	d->desc = strdup(desc);
	d->keyword = strdup(keyword);

	return a;
}
