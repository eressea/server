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
#include "magic.h"

#include "building.h"
#include "curse.h"
#include "faction.h"
#include "goodies.h"
#include "item.h"
#include "karma.h"
#include "message.h"
#include "objtypes.h"
#include "order.h"
#include "pathfinder.h"
#include "plane.h"
#include "pool.h"
#include "race.h"
#include "region.h"
#include "ship.h"
#include "skill.h"
#include "spell.h"
#include "unit.h"

#include <triggers/timeout.h>
#include <triggers/shock.h>
#include <triggers/killunit.h>
#include <triggers/giveitem.h>
#include <triggers/changerace.h>
#include <triggers/clonedied.h>

/* util includes */
#include <util/attrib.h>
#include <util/resolve.h>
#include <util/rand.h>
#include <util/base36.h>
#include <util/event.h>

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
	"nomagic",
	"illaun",
	"tybied",
	"cerddor",
	"gwyrrd",
	"draig"
};

attrib_type at_reportspell = {
	"reportspell", NULL, NULL, NULL, NO_WRITE, NO_READ
};

/**
 ** at_icastle
 ** TODO: separate castle-appearance from illusion-effects
 **/

static double
MagicRegeneration(void)
{
  static double value = -1.0;
  if (value<0) {
    const char * str = get_param(global.parameters, "magic.regeneration");
    value = str?atof(str):1.0;
  }
  return value;
}

static double
MagicPower(void)
{
  static double value = -1.0;
  if (value<0) {
    const char * str = get_param(global.parameters, "magic.power");
    value = str?atof(str):1.0;
  }
  return value;
}

static int
a_readicastle(attrib * a, FILE * f)
{
	icastle_data * data = (icastle_data*)a->data.v;
	if (global.data_version<TYPES_VERSION) {
		int t;
		fscanf(f, "%d", &t);
		data->time = 0;
		data->type = NULL;
		return AT_READ_FAIL;
	} else {
		int bno;
		fscanf(f, "%s %d %d", buf, &bno, &data->time);
		data->building = findbuilding(bno);
		if (!data->building) {
			/* this shouldn't happen, but just in case it does: */
			ur_add((void*)bno, (void**)&data->building, resolve_building);
		}
		data->type = bt_find(buf);
		return AT_READ_OK;
	}
}

static void
a_writeicastle(const attrib * a, FILE * f)
{
	icastle_data * data = (icastle_data*)a->data.v;
	fprintf(f, "%s %d %d ", data->type->_name, data->building->no, data->time);
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
  int i, mtype;
  sc_mage * mage = (sc_mage*)a->data.v;

  fscanf(F, "%d %d %d", &mtype, &mage->spellpoints, &mage->spchange);
  mage->magietyp = (magic_t)mtype;
  for (i=0;i!=MAXCOMBATSPELLS;++i) {
    int spid;
    fscanf (F, "%d %d", &spid, &mage->combatspelllevel[i]);
    if (spid<0) spid = SPL_NOSPELL;
    mage->combatspell[i] = (spellid_t)spid;
  }
  for (;;) {
    int i;

    fscanf (F, "%d", &i);
    if (i < 0) break;
    else {
      spellid_t spid = (spellid_t)i;

      if (find_spellbyid(spid)==NULL) continue;
      add_spell(mage, spid);
    }
  }
  return AT_READ_OK;
}

static void
write_mage(const attrib * a, FILE * F) {
  int i;
  sc_mage *mage = (sc_mage*)a->data.v;
  spell_ptr *sp = mage->spellptr;
  fprintf(F, "%d %d %d ", mage->magietyp, mage->spellpoints, mage->spchange);
  for (i=0;i!=MAXCOMBATSPELLS;++i) {
    fprintf(F, "%d %d ", mage->combatspell[i], mage->combatspelllevel[i]);
  }
  while (sp!=NULL) {
    fprintf (F, "%d ", sp->spellid);
    sp = sp->next;
  }
  fprintf (F, "-1 ");
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
  if (has_skill(u, SK_MAGIC)) {
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
/* ein 'neuer' Magier bekommt von Anfang an alle Sprüche seines
* Magiegebietes mit einer Stufe kleiner oder gleich seinem Magietalent
* in seine List-of-known-spells. Ausgenommen mtyp 0 (M_GRAU) */

static void
createspelllist(unit *u, magic_t mtyp)
{
  spell_list * slist;
  int sk;

  if (mtyp == M_GRAU) return;

  sk = effskill(u, SK_MAGIC);
  if (sk == 0) return;

  for (slist=spells;slist!=NULL;slist=slist->next) {
    spell * sp = slist->data;
    if (sp->magietyp == mtyp && sp->level <= sk) {
      if (!has_spell(u, sp)) {
        add_spell(get_mage(u), sp->id);
      }
    }
  }
}

/* ------------------------------------------------------------- */
/* Erzeugen eines neuen Magiers */
sc_mage *
create_mage(unit * u, magic_t mtyp)
{
  sc_mage *mage;
  attrib *a;
  int i;

  a = a_find(u->attribs, &at_mage);
  if (a==NULL) {
    a = a_add(&u->attribs, a_new(&at_mage));
    mage = calloc(1, sizeof(sc_mage));
    a->data.v = mage;
  } else {
    mage = a->data.v;
  }

  mage->magietyp = mtyp;
  mage->spellpoints = 0;
  mage->spchange = 0;
  mage->spellcount = 0;
  for (i=0;i<MAXCOMBATSPELLS;i++) {
    mage->combatspell[i] = SPL_NOSPELL;
  }
  mage->spellptr = NULL;
  createspelllist(u, mtyp);
  return mage;
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
  int sk = eff_skill(u, SK_MAGIC, u->region);
  spell_list * slist;
  spell * sp;
  struct sc_mage * mage = get_mage(u);
  magic_t gebiet = find_magetype(u);
  boolean ismonster = u->faction->no==MONSTER_FACTION;

  /* Nur Orkmagier bekommen den Keuschheitsamulettzauber */
  sp = find_spellbyid(SPL_ARTEFAKT_CHASTITYBELT);
  if (old_race(u->race)==RC_ORC && !has_spell(u, sp) && sp->level<=sk) {
    add_spell(mage, SPL_ARTEFAKT_CHASTITYBELT);
  }

  /* Nur Wyrm-Magier bekommen den Wyrmtransformationszauber */
  sp = find_spellbyid(SPL_BECOMEWYRM);
  if (fspecial(u->faction, FS_WYRM) && !has_spell(u, sp) && sp->level<=sk) {
    add_spell(mage, SPL_BECOMEWYRM);
  }

  /* Transformierte Wyrm-Magier bekommen Drachenodem */
  if (dragonrace(u->race)) {
    race_t urc = old_race(u->race);
    switch (urc) {
      /* keine breaks! Wyrme sollen alle drei Zauber können.*/
    case RC_WYRM:
      sp = find_spellbyid(SPL_WYRMODEM);
      if (sp!=NULL && !has_spell(u, sp) && sp->level<=sk) {
        add_spell(mage, sp->id);
      }
    case RC_DRAGON:
      sp = find_spellbyid(SPL_DRAGONODEM);
      if (sp!=NULL && !has_spell(u, sp) && sp->level<=sk) {
        add_spell(mage, sp->id);
      }
    case RC_FIREDRAGON:
      sp = find_spellbyid(SPL_FIREDRAGONODEM);
      if (sp!=NULL && !has_spell(u, sp) && sp->level<=sk) {
        add_spell(mage, sp->id);
      }
      break;
    }
  }

  /* Magier mit keinem bzw M_GRAU bekommen weder Sprüche angezeigt noch
  * neue Sprüche in ihre List-of-known-spells. Das sind zb alle alten
  * Drachen, die noch den Skill Magie haben */

  for (slist=spells;slist!=NULL;slist=slist->next) {
    spell * sp = slist->data;
    if (sp->level<=sk) {
      boolean know = has_spell(u, sp);

      if (know || (gebiet!=M_GRAU && sp->magietyp == gebiet)) {
        faction * f = u->faction;

        if (!know) add_spell(mage, sp->id);
        if (!ismonster && !already_seen(u->faction, sp->id)) {
          a_add(&f->attribs, a_new(&at_reportspell))->data.i = sp->id;
          a_add(&f->attribs, a_new(&at_seenspell))->data.i = sp->id;
        }
      }
    }
  }
}

/* ------------------------------------------------------------- */
/* Funktionen für die Bearbeitung der List-of-known-spells */

void
add_spell(sc_mage* m, spellid_t spellid)
{
  if (m==NULL) {
    log_error(("add_spell: unit is not a mage.\n"));
  } else {
    spell_ptr *newsp;
    spell_ptr **spinsert = &m->spellptr;
    while (*spinsert && (*spinsert)->spellid<spellid) spinsert=&(*spinsert)->next;
    newsp = *spinsert;
    if (newsp && newsp->spellid==spellid) {
      log_error(("add_spell: unit already has spell %d.\n", spellid));
      return;
    }
    newsp = calloc(1, sizeof(spell_ptr));
    newsp->spellid = spellid;
    newsp->next = *spinsert;
    *spinsert = newsp;
  }
  return;
}

boolean
has_spell(const unit *u, const spell * sp)
{
  spell_ptr *spt;
  sc_mage * m = get_mage(u);

  if (m==NULL) return false;

  spt = m->spellptr;
  while (spt && spt->spellid<sp->id) spt = spt->next;
  if (spt && spt->spellid==sp->id) return true;
  return false;
}

/* ------------------------------------------------------------- */
/* Eingestellte Kampfzauberstufe ermitteln */

int
get_combatspelllevel(const unit *u, int nr)
{
	sc_mage *m;

	assert(nr < MAXCOMBATSPELLS);
	m = get_mage(u);
	if (!m) {
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

	assert(nr < MAXCOMBATSPELLS);
	m = get_mage(u);
	if (m) {
		return find_spellbyid(m->combatspell[nr]);
	} else if(u->race->precombatspell != SPL_NOSPELL) {
		return find_spellbyid(u->race->precombatspell);
	}

	return NULL;
}

void
set_combatspell(unit *u, spell *sp, struct order * ord, int level)
{
	sc_mage *m;

	m = get_mage(u);
	if (!m) return;

	/* knowsspell prüft auf ist_magier, ist_spruch, kennt_spruch */
	if (knowsspell(u->region, u, sp) == false){
		/* Fehler 'Spell not found' */
		cmistake(u, ord, 173, MSG_MAGIC);
		return;
	}
	if (!has_spell(u, sp)) {
		/* Diesen Zauber kennt die Einheit nicht */
		cmistake(u, ord, 169, MSG_MAGIC);
		return;
	}
	if (!(sp->sptyp & ISCOMBATSPELL)) {
		/* Diesen Kampfzauber gibt es nicht */
		cmistake(u, ord, 171, MSG_MAGIC);
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
static int
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
static int
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
	if (!has_spell(u, sp)) {
		/* ist der Spruch aus einem anderen Magiegebiet? */
		if (find_magetype(u) != sp->magietyp) {
			return false;
		}
		if (eff_skill(u, SK_MAGIC, u->region) >= sp->level) {
			log_warning(("%s ist hat die erforderliche Stufe, kennt aber %s nicht.\n",
				unitname(u), spell_name(sp, default_locale)));
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
cancast(unit * u, spell * sp, int level, int range, struct order * ord)
{
	int k;
	resource_t res;
	int itemanz;
	boolean b = true;

	if (knowsspell(u->region, u, sp) == false) {
		/* Diesen Zauber kennt die Einheit nicht */
		cmistake(u, ord, 173, MSG_MAGIC);
		return false;
	}
	/* reicht die Stufe aus? */
	if (eff_skill(u, SK_MAGIC, u->region) < sp->level) {
		/* die Einheit ist nicht erfahren genug für diesen Zauber */
		cmistake(u, ord, 169, MSG_MAGIC);
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
							"noch %d ", unitname(u), regionname(u->region, u->faction),
							spell_name(sp, u->faction->locale),
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

double
spellpower(region * r, unit * u, spell * sp, int cast_level, struct order * ord)
{
  curse * c;
  double force = cast_level;
  if (sp==NULL) {
    return 0;
  } else {
    /* Bonus durch Magieturm und gesegneten Steinkreis */
    struct building * b = inside_building(u);
    const struct building_type * btype = b?b->type:NULL;
    if (btype && btype->flags & BTF_MAGIC) ++force;
  }

  if (get_item(u, I_RING_OF_POWER) > 0) ++force;

  /* Antimagie in der Zielregion */
  c = get_curse(r->attribs, ct_find("antimagiczone"));
  if (curse_active(c)) {
    unit * mage = c->magician;
    force -= curse_geteffect(c);
    curse_changevigour(&r->attribs, c, -cast_level);
    cmistake(u, ord, 185, MSG_MAGIC);
    if (mage!=NULL) {
      if (force>0) {
        ADDMSG(&mage->faction->msgs, msg_message("reduce_spell", "self mage region", mage, u, r));
      } else {
        ADDMSG(&mage->faction->msgs, msg_message("block_spell", "self mage region", mage, u, r));
      }
    }
  }

  /* Patzerfluch-Effekt: */
  c = get_curse(r->attribs, ct_find("fumble"));
  if (curse_active(c)) {
    unit * mage = c->magician;
    force -= curse_geteffect(c);
    curse_changevigour(&u->attribs, c, -1);
    cmistake(u, ord, 185, MSG_MAGIC);
    if (mage!=NULL) {
      if (force>0) {
        ADDMSG(&mage->faction->msgs, msg_message("reduce_spell", "self mage region", mage, u, r));
      } else {
        ADDMSG(&mage->faction->msgs, msg_message("block_spell", "self mage region", mage, u, r));
      }
    }
  }

  force = force * MagicPower();

  return max(force, 0);
}

/* ------------------------------------------------------------- */
/* farcasting() == 1 -> gleiche Region, da man mit Null nicht vernünfigt
 * rechnen kann */
static int
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
		if (!path_exists(magician->region, r, dist*2, allowed_fly)) mult = 1025;
	}

	return mult;
}


/* ------------------------------------------------------------- */
/* Antimagie - Magieresistenz */
/* ------------------------------------------------------------- */

/* allgemeine Magieresistenz einer Einheit,
 * reduziert magischen Schaden */
double
magic_resistance(unit *target)
{
  attrib * a;
  curse *c;
  int n;

  /* Bonus durch Rassenmagieresistenz */
  double probability = target->race->magres;

  /* Magier haben einen Resistenzbonus vom Magietalent * 5%*/
  probability += effskill(target, SK_MAGIC)*0.05;

  /* Auswirkungen von Zaubern auf der Einheit */
  c = get_curse(target->attribs, ct_find("magicresistance"));
  if (c) {
    probability += 0.01 * curse_geteffect(c) * get_cursedmen(target, c);
  }

  /* Unicorn +10 */
  n = get_item(target, I_UNICORN);
  if (n) probability += n*0.1/target->number;

  /* Auswirkungen von Zaubern auf der Region */
  a = a_find(target->region->attribs, &at_curse);
  while (a) {
    curse *c = (curse*)a->data.v;
    unit *mage = c->magician;

    if (mage!=NULL) {
      if (c->type == ct_find("goodmagicresistancezone")) {
        if (alliedunit(mage, target->faction, HELP_GUARD)) {
          probability += curse_geteffect(c)*0.01;
          break;
        }
      }
      else if (c->type == ct_find("badmagicresistancezone")) {
        if (alliedunit(mage, target->faction, HELP_GUARD)) {
          a = a->nexttype;
          continue;
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
    if (btype) probability += btype->magresbonus * 0.01;
  }
  return probability;
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
	double probability = 0.0;

	if (magician == NULL) return true;
	if (obj == NULL) return true;

	switch(objtyp) {
		case TYP_UNIT:
			{
				int at, pa = 0;
				skill * sv;
        unit * u = (unit*)obj;

				if (fspecial(u->faction, FS_MAGICIMMUNE)) return true;

				at = effskill(magician, SK_MAGIC);

				for (sv = u->skills; sv != u->skills + u->skill_size; ++sv) {
					int sk = effskill(u, sv->id);
					if (pa < sk) pa = sk;
				}

				/* Contest */
				probability = 0.05 * (10 + pa - at);

				probability += magic_resistance((unit *)obj);
				break;
			}

		case TYP_REGION:
			/* Bonus durch Zauber */
			probability += 0.01 * get_curseeffect(((region *)obj)->attribs, C_RESIST_MAGIC, 0);
			break;

		case TYP_BUILDING:
			/* Bonus durch Zauber */
			probability += 0.01 * get_curseeffect(((building *)obj)->attribs, C_RESIST_MAGIC, 0);

			/* Bonus durch Typ */
			probability += 0.01 * ((building *)obj)->type->magres;

			break;

		case TYP_SHIP:
			/* Bonus durch Zauber */
			probability += 0.01 * get_curseeffect(((ship *)obj)->attribs, C_RESIST_MAGIC, 0);
			break;
	}

	probability = max(0.02, probability + t_bonus*0.01);
	probability = min(0.98, probability);

	/* gibt true, wenn die Zufallszahl kleiner als die chance ist und
	 * false, wenn sie gleich oder größer ist, dh je größer die
	 * Magieresistenz (chance) desto eher gibt die Funktion true zurück */
	return chance(probability);
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

	if (btype) patzer -= btype->fumblebonus;
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

static void
do_fumble(castorder *co)
{
  curse * c;
  region * r = co->rt;
  unit * u = (unit*)co->magician;
  spell * sp = co->sp;
  int level = co->level;
  int duration;
  const char * sp_name = spell_name(sp, u->faction->locale);

  ADDMSG(&u->faction->msgs, msg_message("patzer", "unit region spell",
    u, r, sp));
  switch (rand() % 10) {
  case 0:
    /* wenn vorhanden spezieller Patzer, ansonsten nix */
    sp->patzer(co);
    break;

  case 1:
    /* Kröte */
    duration = rand()%level/2;
    if (duration<2) duration = 2;
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
    u->race = new_race[RC_TOAD];
    u->irace = new_race[RC_TOAD];
    sprintf(buf, "Eine Botschaft von %s: 'Ups! Quack, Quack!'", unitname(u));
    addmessage(r, 0, buf, MSG_MAGIC, ML_MISTAKE);
    break;

  case 2:
    /* temporärer Stufenverlust */
    duration = max(rand()%level/2, 2);
    c = create_curse(u, &u->attribs, ct_find("skil"), level, duration,
      -(level/2), 1);
    c->data = (void*)SK_MAGIC;
    ADDMSG(&u->faction->msgs, msg_message("patzer2", "unit region", u, r));
    break;
  case 3:
  case 4:
    /* Spruch schlägt fehl, alle Magiepunkte weg */
    set_spellpoints(u, 0);
    ADDMSG(&u->faction->msgs, msg_message("patzer3", "unit region spell",
      u, r, sp));
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
      unitname(u), sp_name, unitname(u));
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
      "mehr Kraft als normalerweise kosten werden.", unitname(u), sp_name);
    addmessage(0, u->faction, buf, MSG_MAGIC, ML_WARN);
    countspells(u, 3);
  }

  return;
}

/* ------------------------------------------------------------- */
/* Regeneration von Aura */
/* ------------------------------------------------------------- */

/* Ein Magier regeneriert pro Woche W(Stufe^1.5/2+1), mindestens 1
 * Zwerge nur die Hälfte
 */
static int
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

	aura = (int)(aura * MagicRegeneration());

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
					/* Steinkreis erhöht die Regeneration um 50% */
					if (btype) reg_aura *= btype->auraregen;

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
					ADDMSG(&u->faction->msgs, msg_message(
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

static boolean
verify_ship(region * r, unit * mage, const spell * sp, spllprm * spobj, order * ord)
{
  ship *sh = findship(spobj->data.i);

  if (sh!=NULL && sh->region!=r && !(sp->sptyp & SEARCHGLOBAL)) {
    /* Burg muss in gleicher Region sein */
    sh = NULL;
  }

  if (sh==NULL) { 
    /* Burg nicht gefunden */
    spobj->flag = TARGET_NOTFOUND;
    ADDMSG(&mage->faction->msgs, msg_message("spellshipnotfound", 
      "unit region command id", mage, mage->region, ord, spobj->data.i));
    return false;
  }
  spobj->flag = 0;
  spobj->data.sh = sh;
  return true;
}

static boolean
verify_building(region * r, unit * mage, const spell * sp, spllprm * spobj, order * ord)
{
  building *b = findbuilding(spobj->data.i);

  if (b!=NULL && b->region!=r && !(sp->sptyp & SEARCHGLOBAL)) {
    /* Burg muss in gleicher Region sein */
    b = NULL;
  }

  if (b==NULL) { 
    /* Burg nicht gefunden */
    spobj->flag = TARGET_NOTFOUND;
    ADDMSG(&mage->faction->msgs, msg_message("spellbuildingnotfound", 
      "unit region command id", mage, mage->region, ord, spobj->data.i));
    return false;
  }
  spobj->flag = 0;
  spobj->data.b = b;
  return true;
}

static boolean
verify_unit(region * r, unit * mage, const spell * sp, spllprm * spobj, order * ord)
{
  unit *u = NULL;
  switch (spobj->typ) {
    case SPP_UNIT:
      u = findunit(spobj->data.i);
      break;
    case SPP_TEMP:
      u = findnewunit(r, mage->faction, spobj->data.i);
      if (u==NULL) u = findnewunit(mage->region, mage->faction, spobj->data.i);
      break;
    default:
      assert(!"shouldn't happen, this");
  }
  if (u!=NULL && (sp->sptyp & SEARCHGLOBAL) == 0) {
    if (u->region!=r) u = NULL;
    else if (sp->sptyp & TESTCANSEE) {
      if (!cansee(mage->faction, r, u, 0) && !ucontact(u, mage)) {
        u = NULL;
      }
    }
  }

  if (u==NULL) { 
    /* Einheit nicht gefunden */
    char * uid;
    spobj->flag = TARGET_NOTFOUND;

    if (spobj->typ==SPP_UNIT) {
      uid = strdup(itoa36(spobj->data.i));
    } else {
      char tbuf[20];
      sprintf(tbuf, "%s %s", LOC(mage->faction->locale,
        parameters[P_TEMP]), itoa36(spobj->data.i));
      uid = strdup(tbuf);
    }
    ADDMSG(&mage->faction->msgs, msg_message("spellunitnotfound", 
      "unit region command id", mage, mage->region, ord, uid));
    return false;
  }
  /* Einheit wurde gefunden, pointer setzen */
  spobj->flag = 0;
  spobj->data.u = u;
  return true;
}

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
static int
verify_targets(castorder *co)
{
	unit *mage = (unit *)co->magician;
	spell *sp = co->sp;
	region *target_r = co->rt;
	spellparameter *sa = co->par;
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
			spllprm * spobj = sa->param[i];

			switch(spobj->typ) {
        case SPP_TEMP:
				case SPP_UNIT:
          if (!verify_unit(target_r, mage, sp, spobj, co->order)) ++failed;
          break;
				case SPP_BUILDING:
          if (!verify_building(target_r, mage, sp, spobj, co->order)) ++failed;
          break;
				case SPP_SHIP:
          if (!verify_ship(target_r, mage, sp, spobj, co->order)) ++failed;
          break;
        default:
          break;
			}
		}

    /* Nun folgen die Tests auf cansee und Magieresistenz */
		for (i=0;i<sa->length;i++) {
			spllprm * spobj = sa->param[i];
      unit * u;
      building * b;
      ship * sh;
      region * tr;

      if (spobj->flag == TARGET_NOTFOUND) continue;
			switch(spobj->typ) {
        case SPP_TEMP:
				case SPP_UNIT:
					u = spobj->data.u;

          if ((sp->sptyp & TESTRESISTANCE)
							&& target_resists_magic(mage, u, TYP_UNIT, 0))
					{ 
            /* Fehlermeldung */
						spobj->data.i = u->no;
						spobj->flag = TARGET_RESISTS;
						++resists;
						ADDMSG(&mage->faction->msgs, msg_message("spellunitresists",
              "unit region command target",
							mage, mage->region, co->order, u));
						break;
					}

					/* TODO: Test auf Parteieigenschaft Magieresistsenz */
					++success;
					break;
				case SPP_BUILDING:
					b = spobj->data.b;

					if ((sp->sptyp & TESTRESISTANCE)
								&& target_resists_magic(mage, b, TYP_BUILDING, 0))
					{ /* Fehlermeldung */
						spobj->data.i = b->no;
						spobj->flag = TARGET_RESISTS;
						resists++;
						add_message(&mage->faction->msgs, new_message(mage->faction,
							"spellbuildingresists%u:unit%r:region%s:command%d:id",
							mage, mage->region, co->order, spobj->data.i));
						break;
					}
					success++;
					break;
				case SPP_SHIP:
					sh = spobj->data.sh;

					if ((sp->sptyp & TESTRESISTANCE)
								&& target_resists_magic(mage, sh, TYP_SHIP, 0))
					{ /* Fehlermeldung */
						spobj->data.i = sh->no;
						spobj->flag = TARGET_RESISTS;
						resists++;
						add_message(&mage->faction->msgs, new_message(mage->faction,
							"spellshipresists%u:unit%r:region%s:command%d:id",
							mage, mage->region, co->order, spobj->data.i));
						break;
					}
					success++;
					break;

        case SPP_REGION:
          /* haben wir ein Regionsobjekt, dann wird auch dieses und
						 nicht target_r überprüft. */
					tr = spobj->data.r;

					if ((sp->sptyp & TESTRESISTANCE)
							&& target_resists_magic(mage, tr, TYP_REGION, 0))
					{ /* Fehlermeldung */
						spobj->flag = TARGET_RESISTS;
						resists++;
						add_message(&mage->faction->msgs, new_message(mage->faction,
							"spellregionresists%u:unit%r:region%s:command",
							mage, mage->region, co->order));
						break;
					}
					success++;
					break;
				case SPP_INT:
				case SPP_STRING:
					success++;
					break;

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
      spllprm * spobj = malloc(sizeof(spllprm));
      spobj->flag = 0;
      spobj->typ = SPP_REGION;
      spobj->data.r = target_r;
      
      sa = calloc(1, sizeof(spellparameter));
			sa->length = 1;
			sa->param = calloc(sa->length, sizeof(spllprm *));
			sa->param[0] = spobj;
			co->par = sa;

			if ((sp->sptyp & TESTRESISTANCE)) {
				if (target_resists_magic(mage, target_r, TYP_REGION, 0)) {
					/* Fehlermeldung */
					add_message(&mage->faction->msgs, new_message(mage->faction,
						"spellregionresists%u:unit%r:region%s:command",
						mage, mage->region, co->order));
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

static void
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

static int
addparam_string(char ** param, spllprm ** spobjp)
{
  spllprm * spobj = *spobjp = malloc(sizeof(spllprm));
  assert(param[0]);

  spobj->flag = 0;
  spobj->typ = SPP_STRING;
  spobj->data.s = strdup(param[0]);
  return 1;
}

static int
addparam_int(char ** param, spllprm ** spobjp)
{
  spllprm * spobj = *spobjp = malloc(sizeof(spllprm));
  assert(param[0]);

  spobj->flag = 0;
  spobj->typ = SPP_INT;
  spobj->data.i = atoi(param[0]);
  return 1;
}

static int
addparam_ship(char ** param, spllprm ** spobjp)
{
  spllprm * spobj = *spobjp = malloc(sizeof(spllprm));
  int id = atoi36(param[0]);

  spobj->flag = 0;
  spobj->typ = SPP_SHIP;
  spobj->data.i = id;
  return 1;
}

static int
addparam_building(char ** param, spllprm ** spobjp)
{
  spllprm * spobj = *spobjp = malloc(sizeof(spllprm));
  int id = atoi36(param[0]);
  
  spobj->flag = 0;
  spobj->typ = SPP_BUILDING;
  spobj->data.i = id;
  return 1;
}

static int
addparam_region(char ** param, spllprm ** spobjp, const unit * u, order * ord)
{
  assert(param[0]);
  if (param[1]==0) {
    /* Fehler: Zielregion vergessen */
    cmistake(u, ord, 194, MSG_MAGIC);
    return -1;
  } else {
    int tx = atoi(param[0]), ty = atoi(param[1]);
    int x = rel_to_abs(0, u->faction, tx, 0);
    int y = rel_to_abs(0, u->faction, ty, 1);
    region *rt = findregion(x,y);

    if (rt!=NULL) {
      spllprm * spobj = *spobjp = malloc(sizeof(spllprm));

      spobj->flag = 0;
      spobj->typ = SPP_REGION;
      spobj->data.r = rt;
    } else {
      /* Fehler: Zielregion vergessen */
      cmistake(u, ord, 194, MSG_MAGIC);
      return -1;
    }
    return 2;
  }
}


static int
addparam_unit(char ** param, spllprm ** spobjp, const unit * u, order * ord)
{
  spllprm *spobj;
  int i = 0;
  sppobj_t otype = SPP_UNIT;

  *spobjp = NULL;
  if (findparam(param[0], u->faction->locale)==P_TEMP) {
    if (param[1]==NULL) {
      /* Fehler: Ziel vergessen */
      cmistake(u, ord, 203, MSG_MAGIC);
      return -1;
    }
    ++i;
    otype = SPP_TEMP;
  }

  spobj = *spobjp = malloc(sizeof(spllprm));
  spobj->flag = 0;
  spobj->typ = otype;
  spobj->data.i = atoi36(param[i]);

  return i+1;
}

static spellparameter *
add_spellparameter(region *target_r, unit *u, const char *syntax, char ** param, int size, struct order * ord)
{
  boolean fail = false;
	int i = 0;
  int p = 0;
	const char * c = syntax;
	spellparameter *par;
  int minlen = 0;

  while (*c!=0) if (*c++!='+') ++minlen;
  c = syntax;

	/* mindestens ein Ziel (Ziellose Zauber werden nicht
	 * geparst) */
	if (size==0) {
		/* Fehler: Ziel vergessen */
		cmistake(u, ord, 203, MSG_MAGIC);
		return 0;
	}

  par = malloc(sizeof(spellparameter));
  par->length = size;
	par->param = malloc(size * sizeof(spllprm *));

  while (!fail && *c && i<size && param[i]!=NULL) {
    spllprm * spobj = NULL;
    param_t pword;
    int j = -1;
    switch (*c) {
      case '+':
        /* das vorhergehende Element kommt ein oder mehrmals vor, wir
        * springen zum key zurück */
        j = 0;
        --c;
        break;
      case 'u':
        /* Parameter ist eine Einheit, evtl. TEMP */
        j = addparam_unit(param+i, &spobj, u, ord);
        ++c;
        break;
      case 'r':
        /* Parameter sind zwei Regionskoordinaten */
        j = addparam_region(param+i, &spobj, u, ord);
        ++c;
        break;
      case 'b':
				/* Parameter ist eine Burgnummer */
        j = addparam_building(param+i, &spobj);
        ++c;
        break;
      case 's':
        j = addparam_ship(param+i, &spobj);
        ++c;
        break;
      case 'c': 
        /* Text, wird im Spruch ausgewertet */
        j = addparam_string(param+i, &spobj);
        ++c;
        break;
      case 'i': /* Zahl */
        j = addparam_int(param+i, &spobj);
        ++c;
        break;
      case 'k':
        ++c;
        pword = findparam(param[i++], u->faction->locale);
        switch (pword) {
          case P_REGION:
            spobj = malloc(sizeof(spllprm));
            spobj->flag = 0;
            spobj->typ = SPP_REGION;
            spobj->data.r = u->region;
            j = 0;
            ++c;
            break;
          case P_UNIT:
            if (i<size) {
              j = addparam_unit(param+i, &spobj, u, ord);
              ++c;
            }
            break;
          case P_BUILDING:
          case P_GEBAEUDE:
            if (i<size) {
              j = addparam_building(param+i, &spobj);
              ++c;
            }
            break;
          case P_SHIP:
            if (i<size) {
              j = addparam_ship(param+i, &spobj);
              ++c;
            }
            break;
          default:
            j = -1;
            break;
        }
        break;
      default:
        j = -1;
        break;
    }
    if (j<0) fail = true;
    else {
      if (spobj!=NULL) par->param[p++] = spobj;
      i += j;
    }
	}

  /* im Endeffekt waren es evtl. nur p parameter (wegen TEMP) */
  par->length = p;
  if (fail || par->length<minlen) {
    cmistake(u, ord, 209, MSG_MAGIC);
    free_spellparameter(par);
    return NULL;
  }

	return par;
}

/* ------------------------------------------------------------- */

castorder *
new_castorder(void *u, unit *u2, spell *sp, region *r, int lev,
		double force, int range, struct order * ord, spellparameter *p)
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
	corder->order = copy_order(ord);
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
		if (co2->order) free_order(co2->order);
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

static void
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
	return AT_READ_OK;
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

static void
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
	return AT_READ_OK;
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
	return AT_READ_OK;
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
  const unit *u = shipowner(sh);

  switch (get_keyword(u->thisorder)) {
    case K_ROUTE:
    case K_MOVE:
    case K_FOLLOW:
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
  const char *s;
  int spellrank;
  int range, t_x, t_y;
  castorder *co;
  castorder *cll[MAX_SPELLRANK];
  spellparameter *args;

  for (spellrank = 0; spellrank < MAX_SPELLRANK; spellrank++) {
    cll[spellrank] = (castorder*)NULL;
  }

  for (r = regions; r; r = r->next) {
    for (u = r->units; u; u = u->next) {
      int level;
      boolean casted = false;
      order * ord;

      if (old_race(u->race) == RC_SPELL || fval(u, UFL_LONGACTION))
        continue;

      if (old_race(u->race) == RC_INSECT && r_insectstalled(r) &&
        !is_cursed(u->attribs, C_KAELTESCHUTZ,0))
        continue;

      if(fval(u, UFL_WERE)) {
        continue;
      }

      if (attacked(u)) {
        continue;
      }

      for (ord = u->orders; ord; ord = ord->next) {
        if (get_keyword(ord) == K_CAST) {
          if (LongHunger(u)) {
            cmistake(u, ord, 224, MSG_MAGIC);
            continue;
          }
          if (r->planep && fval(r->planep, PFL_NOMAGIC)) {
            cmistake(u, ord, 269, MSG_MAGIC);
            continue;
          }
          casted = true;
          target_r = r;
          mage = u;
          level = eff_skill(u, SK_MAGIC, r);
          familiar = NULL;
          init_tokens(ord);
          skip_token();
          s = getstrtoken();
          /* für Syntax ' STUFE x REGION y z ' */
          if (findparam(s, u->faction->locale) == P_LEVEL) {
            s = getstrtoken();
            level = min(atoip(s), level);
            s = getstrtoken();
            if (level < 1) {
              /* Fehler "Das macht wenig Sinn" */
              cmistake(u, ord, 10, MSG_MAGIC);
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
            if (!target_r) {
              /* Fehler "Es wurde kein Zauber angegeben" */
              cmistake(u, ord, 172, MSG_MAGIC);
              continue;
            }
          }
          /* für Syntax ' REGION x y STUFE z '
          * hier nach REGION nochmal auf STUFE prüfen */
          if (findparam(s, u->faction->locale) == P_LEVEL) {
            s = getstrtoken();
            level = min(atoip(s), level);
            s = getstrtoken();
            if (level < 1) {
              /* Fehler "Das macht wenig Sinn" */
              cmistake(u, ord, 10, MSG_MAGIC);
              continue;
            }
          }
          if (!s[0] || strlen(s) == 0) {
            /* Fehler "Es wurde kein Zauber angegeben" */
            cmistake(u, ord, 172, MSG_MAGIC);
            continue;
          }
          sp = find_spellbyname(u, s, u->faction->locale);

          /* Vertraute können auch Zauber sprechen, die sie selbst nicht
          * können. find_spellbyname findet aber nur jene Sprüche, die
          * die Einheit beherrscht. */
          if (sp == NULL && is_familiar(u)) {
            familiar = u;
            mage = get_familiar_mage(u);
            if (mage!=NULL) sp = find_spellbyname(mage, s, mage->faction->locale);
          }

          if (sp == NULL) {
            /* Fehler 'Spell not found' */
            cmistake(u, ord, 173, MSG_MAGIC);
            continue;
          }
          /* um testen auf spruchnamen zu unterbinden sollte vor allen
          * fehlermeldungen die anzeigen das der magier diesen Spruch
          * nur in diese Situation nicht anwenden kann, noch eine
          * einfache Sicherheitsprüfung kommen */
          if (knowsspell(r, u, sp) == false) {
            /* vorsicht! u kann der familiar sein */
            if (!familiar){
              cmistake(u, ord, 173, MSG_MAGIC);
              continue;
            }
          }
          if (sp->sptyp & ISCOMBATSPELL) {
            /* Fehler: "Dieser Zauber ist nur im Kampf sinnvoll" */
            cmistake(u, ord, 174, MSG_MAGIC);
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
                ADDMSG(&u->faction->msgs, msg_message("spellfail_onocean",
                  "unit region command", u, u->region, ord));
                continue;
              }
              /* Auf bewegenden Schiffen kann man nur explizit als
              * ONSHIPCAST deklarierte Zauber sprechen */
          } else if (u->ship) {
            if (is_moving_ship(r, u->ship)) {
              if (!(sp->sptyp & ONSHIPCAST)) {
                /* Fehler: "Diesen Spruch kann man nicht auf einem sich
                * bewegenden Schiff stehend zaubern" */
                cmistake(u, ord, 175, MSG_MAGIC);
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
              cmistake(u, ord, 176, MSG_MAGIC);
              continue;
            }
            if (range > 1024) { /* (2^10) weiter als 10 Regionen entfernt */
              ADDMSG(&u->faction->msgs, msg_message("spellfail::nocontact",
                "mage region command target", u, u->region, ord, 
                gc_add(strdup(regionname(target_r, u->faction)))));
              continue;
            }
          }
          /* Stufenangabe bei nicht Stufenvariierbaren Sprüchen abfangen */
          if (!(sp->sptyp & SPELLLEVEL)) {
            int ilevel = eff_skill(u, SK_MAGIC, u->region);
            if (ilevel!=level) {
              level = ilevel;
              ADDMSG(&u->faction->msgs, msg_message("spellfail::nolevel",
                "mage region command", u, u->region, ord));
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
              cmistake(u, ord, 177, MSG_MAGIC);
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
              set_order(&mage->thisorder, NULL);
              level = min(level, eff_skill(mage, SK_MAGIC, mage->region)/2);
              familiar = u;
            }
          }
          /* Weitere Argumente zusammenbasten */
          if (sp->parameter) {
            char ** params = malloc(2*sizeof(char*));
            size_t p = 0, size = 2;
            for (;;) {
              s = getstrtoken();
              if (*s==0) break;
              if (p+1>=size) {
                size*=2;
                params = realloc(params, sizeof(char*)*size);
              }
              params[p++] = strdup(s);
            }
            params[p] = 0;
            args = add_spellparameter(target_r, mage, sp->parameter, params, p, ord);
            for (p=0;params[p];++p) free(params[p]);
            free(params);
            if (args==NULL) {
              /* Syntax war falsch */
              continue;
            }
          } else {
            args = (spellparameter *) NULL;
          }
          co = new_castorder(mage, familiar, sp, target_r, level, 0, range,
            ord, args);
          add_castorder(&cll[(int)(sp->rank)], co);
        }
      }
      if (casted) fset(u, UFL_LONGACTION);
    }
  }

  /* Da sich die Aura und Komponenten in der Zwischenzeit verändert
  * haben können und sich durch vorherige Sprüche das Zaubern
  * erschwert haben kann, muss beim zaubern erneut geprüft werden, ob der
  * Spruch überhaupt gezaubert werden kann.
  * (level) die effektive Stärke des Spruchs (= Stufe, auf der der
  * Spruch gezaubert wird) */

  for (spellrank = 0; spellrank < MAX_SPELLRANK; spellrank++) {
    for (co = cll[spellrank]; co; co = co->next) {
      order * ord = co->order;
      int verify, cast_level = co->level;
      boolean fumbled = false;
      unit * u = (unit *)co->magician;
      spell * sp = co->sp;
      region * target_r = co->rt;

      /* reichen die Komponenten nicht, wird der Level reduziert. */
      co->level = eff_spelllevel(u, sp, cast_level, co->distance);

      if (co->level < 1) {
        /* Fehlermeldung mit Komponenten generieren */
        cancast(u, sp, co->level, co->distance, ord);
        continue;
      }

      if (cast_level > co->level) {
        /* Sprüche mit Fixkosten werden immer auf Stufe des Spruchs
        * gezaubert, co->level ist aber defaultmäßig Stufe des Magiers */
        if (spl_costtyp(sp) != SPC_FIX) {
          sprintf(buf, "%s hat nur genügend Komponenten um %s auf Stufe %d "
            "zu zaubern.", unitname(u), spell_name(sp, u->faction->locale),
            co->level);
          addmessage(0, u->faction, buf, MSG_MAGIC, ML_INFO);
        }
      }

      /* Prüfen, ob die realen Kosten für die gewünschten Stufe bezahlt
      * werden können */
      if (cancast(u, sp, co->level, co->distance, ord) == false) {
        /* die Fehlermeldung wird in cancast generiert */
        continue;
      }

      co->force = spellpower(target_r, u, sp, co->level, ord);
      /* die Stärke kann durch Antimagie auf 0 sinken */
      if (co->force <= 0) {
        co->force = 0;
        sprintf(buf, "%s schafft es nicht genügend Kraft aufzubringen "
          "um %s dennoch zu zaubern.", unitname(u),
          spell_name(sp, u->faction->locale));
        addmessage(0, u->faction, buf, MSG_MAGIC, ML_MISTAKE);
      }

      /* Ziele auf Existenz prüfen und Magieresistenz feststellen. Wurde
      * kein Ziel gefunden, so ist verify_targets=0. Scheitert der
      * Spruch an der Magieresistenz, so ist verify_targets = 1, bei
      * Erfolg auf ganzer Linie ist verify_targets= 2
      */
      verify = verify_targets(co);
      if (verify==0) {
        /* kein Ziel gefunden, Fehlermeldungen sind in verify_targets */
        /* keine kosten für den zauber */
        continue; /* äußere Schleife, nächster Zauberer */
      } else if (co->force>0 && verify==1) {
        /* einige oder alle Ziele waren magieresistent */
        spellparameter *pa = co->par;
        int n;
        for (n=0; n!=pa->length;++n) {
          if ((pa->param[n]->flag & (TARGET_RESISTS|TARGET_NOTFOUND)) ==0) {
            /* mindestens ein erfolgreicher Zauberversuch, wir machen normal weiter */
            break;
          }
        }
        if (n==pa->length) {
          co->force = 0;
          /* zwar wurde mindestens ein Ziel gefunden, das widerstand
            * jedoch dem Zauber. Kosten abziehen und abbrechen. */
          sprintf(buf, "%s gelingt es %s zu zaubern, doch der Spruch zeigt "
            "keine Wirkung.", unitname(u),
            spell_name(sp, u->faction->locale));
          addmessage(0, u->faction, buf, MSG_MAGIC, ML_MISTAKE);
          co->force = 0;
        }
      }

      /* Auch für Patzer gibt es Erfahrung, müssen die Spruchkosten
      * bezahlt werden und die nachfolgenden Sprüche werden teurer */
      if (co->force>0 && fumble(target_r, u, sp, co->level)) {
        /* zuerst bezahlen, dann evt in do_fumble alle Aura verlieren */
        fumbled = true;
      } else if (co->force>0) {
        co->level = ((nspell_f)sp->sp_function)(co);
        if (co->level <= 0) {
          /* Kosten nur für real benötige Stufe berechnen */
          continue;
        }
      }
      pay_spell(u, sp, co->level, co->distance);
      /* erst bezahlen, dann Kostenzähler erhöhen */
      if (fumbled) do_fumble(co);
      countspells(u, 1);
    }
  }

  /* Sind alle Zauber gesprochen gibts Erfahrung */
  for (r = regions; r; r = r->next) {
    for (u = r->units; u; u = u->next) {
      if (is_mage(u) && countspells(u,0) > 0) {
        produceexp(u, SK_MAGIC, u->number);
        /* Spruchlistenaktualiesierung ist in Regeneration */
      }
    }
  }
  for (spellrank = 0; spellrank < MAX_SPELLRANK; spellrank++) {
    free_castorders(cll[spellrank]);
  }
}

const char *
spell_info(const struct spell * sp, const struct locale * lang)
{
	if (sp->info==NULL) {
		return LOC(lang, mkname("spellinfo", sp->sname));
	}
	return sp->info;
}

const char *
spell_name(const struct spell * sp, const struct locale * lang)
{
	if (sp->info==NULL) {
		return LOC(lang, mkname("spell", sp->sname));
	}
	return sp->sname;
}

void
add_spelllist(spell_list ** lspells, spell * sp)
{
  spell_list * entry = malloc(sizeof(spell_list));
  entry->data = sp;
  entry->next = *lspells;
  *lspells = entry;
}
