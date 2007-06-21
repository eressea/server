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
#include "spellid.h"
#include "terrain.h"
#include "unit.h"
#include "version.h"

#include <triggers/timeout.h>
#include <triggers/shock.h>
#include <triggers/killunit.h>
#include <triggers/giveitem.h>
#include <triggers/changerace.h>
#include <triggers/clonedied.h>

/* util includes */
#include <util/attrib.h>
#include <util/lists.h>
#include <util/parser.h>
#include <util/resolve.h>
#include <util/rand.h>
#include <util/rng.h>
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
	"gray",
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
		variant bno;
		fscanf(f, "%s %d %d", buf, &bno.i, &data->time);
		data->building = findbuilding(bno.i);
		if (!data->building) {
			/* this shouldn't happen, but just in case it does: */
			ur_add(bno, (void**)&data->building, resolve_building);
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
    ADDMSG(&r->msgs, msg_message("icastle_dissolve", "building", b));
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
  freelist(mage->spells);
  free(mage);
}

static int
read_mage(attrib * a, FILE * F)
{
  int i, mtype;
  sc_mage * mage = (sc_mage*)a->data.v;
  char spname[64];

  fscanf(F, "%d %d %d", &mtype, &mage->spellpoints, &mage->spchange);
  mage->magietyp = (magic_t)mtype;
  for (i=0;i!=MAXCOMBATSPELLS;++i) {
    if (global.data_version<SPELLNAME_VERSION) {
      int spid;
      fscanf (F, "%d %d", &spid, &mage->combatspells[i].level);
      if (spid>=0) {
        mage->combatspells[i].sp = find_spellbyid(mage->magietyp, (spellid_t)spid);
      }
    } else {
      fscanf (F, "%s %d", spname, &mage->combatspells[i].level);
      if (strcmp("none", spname)!=0) {
        mage->combatspells[i].sp = find_spell(mage->magietyp, spname);
      }
    }
  }

  for (;;) {
    spell * sp;

    if (global.data_version<SPELLNAME_VERSION) {
      fscanf (F, "%d", &i);
      if (i < 0) break;
      sp = find_spellbyid(mage->magietyp, (spellid_t)i);
    } else {
      fscanf(F, "%s", spname);
      if (strcmp(spname, "end")==0) break;
      sp = find_spell(mage->magietyp, spname);
    }
    if (sp==NULL) {
      /* flag upstream to show that updatespellist() is necessary (hackish) */
      mage->spellcount = -1;
      continue;
    }
    add_spell(mage, sp);
  }
  return AT_READ_OK;
}

static void
write_mage(const attrib * a, FILE * F) {
  int i;
  sc_mage *mage = (sc_mage*)a->data.v;
  spell_list *slist = mage->spells;
  fprintf(F, "%d %d %d ", mage->magietyp, mage->spellpoints, mage->spchange);
  for (i=0;i!=MAXCOMBATSPELLS;++i) {
    fputs(mage->combatspells[i].sp?mage->combatspells[i].sp->sname:"none", F);
    fprintf(F, " %d ", mage->combatspells[i].level);
  }
  while (slist!=NULL) {
    spell * sp = slist->data;
    fprintf (F, "%s ", sp->sname);
    slist = slist->next;
  }
#if RELEASE_VERSION < SPELLNAME_VERSION
  fprintf (F, "-1 ");
#else
  fputs("end ", F);
#endif
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
        add_spell(get_mage(u), sp);
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

  a = a_find(u->attribs, &at_mage);
  if (a!=NULL) {
    a_remove(&u->attribs, a);
  }
  a = a_add(&u->attribs, a_new(&at_mage));
  mage = a->data.v;

  mage->magietyp = mtyp;
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


static int
read_seenspell(attrib * a, FILE * f)
{
  int i;
  spell * sp = NULL;

  fscanf(f, "%s", buf);
  i = atoi(buf);
  if (i!=0) {
    sp = find_spellbyid(M_GRAU, (spellid_t)i);
  } else {
    int mtype;
    fscanf(f, "%d", &mtype);
    sp = find_spell((magic_t)mtype, buf);
  }
  if (sp==NULL) {
    /* log_error(("could not find seenspell '%s'\n", buf)); */
    return AT_READ_FAIL;
  }
  a->data.v = sp;
  return AT_READ_OK;
}

static void
write_seenspell(const attrib * a, FILE * f)
{
  const spell * sp = (const spell*)a->data.v;
  fprintf(f, "%s %d ", sp->sname, sp->magietyp);
}

attrib_type at_seenspell = {
	"seenspell", NULL, NULL, NULL, write_seenspell, read_seenspell
};

static boolean
already_seen(const faction * f, const spell * sp)
{
  attrib *a;

  for (a = a_find(f->attribs, &at_seenspell); a && a->type==&at_seenspell; a=a->next) {
    if (a->data.v==sp) return true;
  }
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

  /* Nur Wyrm-Magier bekommen den Wyrmtransformationszauber */
  sp = find_spellbyid(M_GRAU, SPL_BECOMEWYRM);

#ifdef KARMA_MODULE
  if (fspecial(u->faction, FS_WYRM) && !has_spell(u, sp) && sp->level<=sk) {
    add_spell(mage, find_spellbyid(M_GRAU, SPL_BECOMEWYRM));
  }
#endif /* KARMA_MODULE */

  /* Transformierte Wyrm-Magier bekommen Drachenodem */
  if (dragonrace(u->race)) {
    race_t urc = old_race(u->race);
    switch (urc) {
      /* keine breaks! Wyrme sollen alle drei Zauber können.*/
    case RC_WYRM:
      sp = find_spellbyid(M_GRAU, SPL_WYRMODEM);
      if (sp!=NULL && !has_spell(u, sp) && sp->level<=sk) {
        add_spell(mage, sp);
      }
    case RC_DRAGON:
      sp = find_spellbyid(M_GRAU, SPL_DRAGONODEM);
      if (sp!=NULL && !has_spell(u, sp) && sp->level<=sk) {
        add_spell(mage, sp);
      }
    case RC_FIREDRAGON:
      sp = find_spellbyid(M_GRAU, SPL_FIREDRAGONODEM);
      if (sp!=NULL && !has_spell(u, sp) && sp->level<=sk) {
        add_spell(mage, sp);
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

        if (!know) add_spell(mage, sp);
        if (!ismonster && !already_seen(u->faction, sp)) {
          a_add(&f->attribs, a_new(&at_reportspell))->data.v = sp;
          a_add(&f->attribs, a_new(&at_seenspell))->data.v = sp;
        }
      }
    }
  }
}

/* ------------------------------------------------------------- */
/* Funktionen für die Bearbeitung der List-of-known-spells */

void
add_spell(sc_mage* m, spell * sp)
{
  if (m==NULL) {
    log_error(("add_spell: unit is not a mage.\n"));
  } else {
    spell_list ** slist = spelllist_find(&m->spells, sp);
    if (*slist) {
      spell * psp = (*slist)->data;
      if (psp==sp) {
        log_error(("add_spell: unit already has spell '%s'.\n", sp->sname));
        return;
      }
    }
    spelllist_add(slist, sp);
  }
}

boolean
has_spell(const unit *u, const spell * sp)
{
  sc_mage * m = get_mage(u);
  if (m!=NULL) {
    spell_list * sfind = *spelllist_find(&m->spells, sp);
    return sfind!=NULL && sfind->data==sp;
  }
  return false;
}

/* ------------------------------------------------------------- */
/* Eingestellte Kampfzauberstufe ermitteln */

int
get_combatspelllevel(const unit *u, int nr)
{
	sc_mage *m = get_mage(u);

	assert(nr < MAXCOMBATSPELLS);
	if (m) {
    int level = eff_skill(u, SK_MAGIC, u->region);
    return min(m->combatspells[nr].level, level);
	}
  return -1;
}

/* ------------------------------------------------------------- */
/* Kampfzauber ermitteln, setzen oder löschen */

const spell*
get_combatspell(const unit *u, int nr)
{
	sc_mage *m;

	assert(nr < MAXCOMBATSPELLS);
	m = get_mage(u);
	if (m) {
		return m->combatspells[nr].sp;
	} else if (u->race->precombatspell != NULL) {
		return u->race->precombatspell;
	}

	return NULL;
}

void
set_combatspell(unit *u, spell *sp, struct order * ord, int level)
{
	sc_mage *m = get_mage(u);
  int i = -1;
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

	if (sp->sptyp & PRECOMBATSPELL) i = 0;
  else if (sp->sptyp & COMBATSPELL) i = 1;
  else if (sp->sptyp & POSTCOMBATSPELL) i = 2;
  assert(i>=0);
	m->combatspells[i].sp = sp;
	m->combatspells[i].level = level;
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
			m->combatspells[i].sp = NULL;
		}
	}
	else if (sp->sptyp & PRECOMBATSPELL) {
		if (sp != get_combatspell(u,0))
			return;
	} else if (sp->sptyp & COMBATSPELL) {
		if (sp != get_combatspell(u,1)) {
			return;
		}
    nr = 1;
	} else if (sp->sptyp & POSTCOMBATSPELL) {
		if (sp != get_combatspell(u,2)) {
			return;
		}
    nr = 2;
	}
	m->combatspells[nr].sp = NULL;
	m->combatspells[nr].level = 0;
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

	if (get_item(u, I_AURAKULUM) > 0) {
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
spellcost(unit *u, const spell * sp)
{
	int k, aura = 0;
	int count = countspells(u, 0);

	for (k = 0; sp->components[k].type; k++) {
		if (sp->components[k].type == r_aura) {
			aura = sp->components[k].amount;
    }
	}
	aura *= (1<<count);
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
spl_costtyp(const spell * sp)
{
	int k;
	int costtyp = SPC_FIX;

	for (k = 0; sp->components[k].type; k++) {
		if (costtyp == SPC_LINEAR) return SPC_LINEAR;

		if (sp->components[k].cost == SPC_LINEAR) {
			return SPC_LINEAR;
		}

		/* wenn keine Fixkosten, Typ übernehmen */
		if (sp->components[k].cost != SPC_FIX) {
			costtyp = sp->components[k].cost;
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
eff_spelllevel(unit *u, const spell * sp, int cast_level, int range)
{
  int k, maxlevel, needplevel;
  int costtyp = SPC_FIX;
  
  for (k = 0; sp->components[k].type; k++) {
    if (cast_level == 0)
        return 0;
    
    if (sp->components[k].amount > 0) {
      /* Die Kosten für Aura sind auch von der Zahl der bereits
       * gezauberten Sprüche abhängig */
      if (sp->components[k].type == r_aura) {
        needplevel = spellcost(u, sp) * range;
      } else {
        needplevel = sp->components[k].amount * range;
      }
      maxlevel = get_pooled(u, sp->components[k].type, GET_DEFAULT, needplevel*cast_level)/needplevel;
      
      /* sind die Kosten fix, so muss die Komponente nur einmal vorhanden
       * sein und der cast_level ändert sich nicht */
      if (sp->components[k].cost == SPC_FIX) {
        if (maxlevel < 1) cast_level = 0;
        /* ansonsten wird das Minimum aus maximal möglicher Stufe und der
         * gewünschten gebildet */
      } else if (sp->components[k].cost == SPC_LEVEL) {
        costtyp = SPC_LEVEL;
        cast_level = min(cast_level, maxlevel);
        /* bei Typ Linear müssen die Kosten in Höhe der Stufe vorhanden
         * sein, ansonsten schlägt der Spruch fehl */
      } else if (sp->components[k].cost == SPC_LINEAR) {
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
pay_spell(unit * u, const spell * sp, int cast_level, int range)
{
	int k;
	int resuse;

	for (k = 0; sp->components[k].type; k++) {
		if (sp->components[k].type == r_aura) {
			resuse = spellcost(u, sp) * range;
		} else {
			resuse = sp->components[k].amount * range;
		}

		if (sp->components[k].cost == SPC_LINEAR
				|| sp->components[k].cost == SPC_LEVEL)
		{
			resuse *= cast_level;
		}

		use_pooled(u, sp->components[k].type, GET_DEFAULT, resuse);
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
cancast(unit * u, const spell * sp, int level, int range, struct order * ord)
{
  int k;
  int itemanz;
  resource * reslist = NULL;

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
  
  for (k = 0; sp->components[k].type; ++k) {
    if (sp->components[k].amount > 0) {
      const resource_type * rtype = sp->components[k].type;
      int itemhave;

      /* Die Kosten für Aura sind auch von der Zahl der bereits
       * gezauberten Sprüche abhängig */
      if (rtype == r_aura) {
        itemanz = spellcost(u, sp) * range;
      } else {
        itemanz = sp->components[k].amount * range;
      }
      
      /* sind die Kosten stufenabhängig, so muss itemanz noch mit dem
       * level multipliziert werden */
      switch(sp->components[k].cost) {
      case SPC_LEVEL:
      case SPC_LINEAR:
        itemanz *= level;
        break;
      case SPC_FIX:
      default:
        break;
      }
      
      itemhave = get_pooled(u, rtype, GET_DEFAULT, itemanz);
      if (itemhave < itemanz) {
        resource * res = malloc(sizeof(resource));
        res->number = itemanz-itemhave;
        res->type = rtype;
        res->next = reslist;
        reslist = res;
      }
    }
  }
  if (reslist!=NULL) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "missing_components_list", "list", reslist));
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
spellpower(region * r, unit * u, const spell * sp, int cast_level, struct order * ord)
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
    if (mage!=NULL && mage->faction!=NULL) {
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
    if (mage!=NULL && mage->faction!=NULL) {
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
		return INT_MAX;
	}

	dist = koor_distance(r->x, r->y, magician->region->x, magician->region->y);

	if (dist > 24) return INT_MAX;

	mult = 1 << dist;
	if (dist > 1) {
		if (!path_exists(magician->region, r, dist*2, allowed_fly)) mult = INT_MAX;
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
  assert(target->number>0);

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
  while (a && a->type==&at_curse) {
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
          /* TODO: hier sollte doch sicher was passieren? */
          a = a->next;
          continue;
        }
      }
    }
    a = a->next;
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

#ifdef KARMA_MODULE
        if (fspecial(u->faction, FS_MAGICIMMUNE)) return true;
#endif /* KARMA_MODULE */
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
fumble(region * r, unit * u, const spell * sp, int cast_grade)
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
	rnd = rng_int()%100;

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

/* ------------------------------------------------------------- */
/* Dummy-Zauberpatzer, Platzhalter für speziel auf die Sprüche
* zugeschnittene Patzer */
static void
patzer(castorder *co)
{
  unit *mage = co->magician.u;

  cmistake(mage, co->order, 180, MSG_MAGIC);

  return;
}
/* Die normalen Spruchkosten müssen immer bezahlt werden, hier noch
 * alle weiteren Folgen eines Patzers
 */

static void
do_fumble(castorder *co)
{
  curse * c;
  region * r = co->rt;
  unit * u = co->magician.u;
  const spell *sp = co->sp;
  int level = co->level;
  int duration;
  variant effect;

  ADDMSG(&u->faction->msgs, msg_message("patzer", "unit region spell",
    u, r, sp));
  switch (rng_int() % 10) {
  case 0:
    /* wenn vorhanden spezieller Patzer, ansonsten nix */
    if (sp->patzer) sp->patzer(co);
    else patzer(co);
    break;

  case 1:
    /* Kröte */
    duration = rng_int()%level/2;
    if (duration<2) duration = 2;
    {
      /* one or two things will happen: the toad changes her race back,
      * and may or may not get toadslime.
      * The list of things to happen are attached to a timeout
      * trigger and that's added to the triggerlit of the mage gone toad.
      */
      trigger * trestore = trigger_changerace(u, u->race, u->irace);
      if (chance(0.7)) {
        const item_type * it_toadslime = it_find("toadslime");
        if (it_toadslime!=NULL) {
          t_add(&trestore, trigger_giveitem(u, it_toadslime, 1));
        }
      }
      add_trigger(&u->attribs, "timer", trigger_timeout(duration, trestore));
    }
    u->race = new_race[RC_TOAD];
    u->irace = new_race[RC_TOAD];
    ADDMSG(&r->msgs, msg_message("patzer6", "unit region spell",
      u, r, sp));
    break;

  case 2:
    /* temporärer Stufenverlust */
    duration = max(rng_int()%level/2, 2);
    effect.i = -(level/2);
    c = create_curse(u, &u->attribs, ct_find("skil"), level, duration,
      effect, 1);
    c->data.i = SK_MAGIC;
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
    if (sp->sp_function==NULL) {
      log_error(("spell '%s' has no function.\n", sp->sname));
    } else {
      ((nspell_f)sp->sp_function)(co);
    }
    set_spellpoints(u, 0);
    ADDMSG(&u->faction->msgs, msg_message("patzer4", "unit region spell",
      u, r, sp));
    break;

  case 7:
  case 8:
  case 9:
  default:
    /* Spruch gelingt, alle nachfolgenden Sprüche werden 2^4 so teuer */
    if (sp->sp_function==NULL) {
      log_error(("spell '%s' has no function.\n", sp->sname));
    } else {
      ((nspell_f)sp->sp_function)(co);
    }
    ADDMSG(&u->faction->msgs, msg_message("patzer5", "unit region spell",
      u, r, sp));
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

#ifdef KARMA_MODULE
	if (fspecial(u->faction, FS_MAGICIMMUNE)) return 0;
#endif /* KARMA_MODULE */

  sk = effskill(u, SK_MAGIC);
	/* Rassenbonus/-malus */
	d = (int)(pow(sk, potenz) * u->race->regaura / divisor);
	d++;

	/* Einfluss von Artefakten */
	/* TODO (noch gibs keine)*/

	/* Würfeln */
	aura = (rng_int() % d + rng_int() % d)/2 + 1;

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
			if (u->number && is_mage(u)) {
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

message *
msg_unitnotfound(const struct unit * mage, struct order * ord, const struct spllprm * spobj)
{
  /* Einheit nicht gefunden */
  char tbuf[20];
  const char * uid;

  if (spobj->typ==SPP_UNIT) {
    uid = itoa36(spobj->data.i);
  } else {
    sprintf(tbuf, "%s %s", LOC(mage->faction->locale,
      parameters[P_TEMP]), itoa36(spobj->data.i));
    uid = tbuf;
  }
  return msg_message("spellunitnotfound", 
    "unit region command id", mage, mage->region, ord, uid);
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
    spobj->flag = TARGET_NOTFOUND;
    ADDMSG(&mage->faction->msgs, msg_unitnotfound(mage, ord, spobj));
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
	unit *mage = co->magician.u;
	const spell *sp = co->sp;
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
            ADDMSG(&mage->faction->msgs, msg_message("spellbuildingresists",
              "unit region command id", 
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
						ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, 
              "spellshipresists", "ship", sh));
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
						ADDMSG(&mage->faction->msgs, msg_message("spellregionresists",
              "unit region command", mage, mage->region, co->order));
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
          ADDMSG(&mage->faction->msgs, msg_message("spellregionresists", 
            "unit region command", mage, mage->region, co->order));
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
    short x = rel_to_abs(0, u->faction, (short)tx, 0);
    short y = rel_to_abs(0, u->faction, (short)ty, 1);
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
  const char * c;
  spellparameter *par;
  int minlen = 0;

  for (c=syntax;*c!=0;++c) {
		/* this makes sure that:
		 *  minlen("kc?") = 0
		 *  minlen("kc+") = 1
		 *  minlen("cccc+c?") = 4
		 */
    if (*c=='?') --minlen;
    else if (*c!='+' && *c!='k') ++minlen;
  }
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
		case '?':
			/* tja. das sollte moeglichst nur am Ende passieren, 
			 * weil sonst die kacke dampft. */
			j = 0;
			++c;
			assert(*c==0);
			break;
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
new_castorder(void *u, unit *u2, const spell *sp, region *r, int lev,
              double force, int range, struct order * ord, spellparameter *p)
{
  castorder *corder;

  corder = calloc(1, sizeof(castorder));
  corder->magician.u = u;
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
add_castorder(spellrank *cll, castorder *co)
{
  if (cll->begin==NULL) {
    cll->end = &cll->begin;
  }
  
  *cll->end = co;
  cll->end = &co->next;

  return;
}

void
free_castorders(castorder *co)
{
  castorder *co2;

  while (co) {
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

boolean
is_familiar(const unit *u)
{
  attrib * a = a_find(u->attribs, &at_familiarmage);
  return i2b(a!=NULL);
}

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
	while (a && a->type==&at_skillmod) {
		skillmod_data * smd = (skillmod_data *)a->data.v;
		if (smd->special==sm_familiar) break;
		a = a->next;
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

  if (a!=NULL) {
    a_remove(&mage->attribs, a);
  }
  a = a_find(mage->attribs, &at_skillmod);
  while (a && a->type==&at_skillmod) {
		an = a->next;
		smd = (skillmod_data *)a->data.v;
	  if (smd->special==sm_familiar) a_remove(&mage->attribs, a);
		a = an;
	}
}

boolean
create_newfamiliar(unit * mage, unit * familiar)
{
  /* if the skill modifier for the mage does not yet exist, add it */
  attrib *a;
  attrib *afam = a_find(mage->attribs, &at_familiar);
  attrib *amage = a_find(familiar->attribs, &at_familiarmage);
  
  if (afam==NULL) {
    afam = a_add(&mage->attribs, a_new(&at_familiar));
  }
  afam->data.v = familiar;
  if (amage==NULL) {
    amage = a_add(&familiar->attribs, a_new(&at_familiarmage));
  }
  amage->data.v = mage;

  /* TODO: Diese Attribute beim Tod des Familiars entfernen: */
  /* Wenn der Magier stirbt, dann auch der Vertraute */
  add_trigger(&mage->attribs, "destroy", trigger_killunit(familiar));
  /* Wenn der Vertraute stirbt, dann bekommt der Magier einen Schock */
  add_trigger(&familiar->attribs, "destroy", trigger_shock(mage));
  
  a = a_find(mage->attribs, &at_skillmod);
  while (a && a->type==&at_skillmod) {
    skillmod_data * smd = (skillmod_data *)a->data.v;
    if (smd->special==sm_familiar) break;
    a = a->next;
  }
  if (a==NULL) {
    attrib * an = a_add(&mage->attribs, a_new(&at_skillmod));
    skillmod_data * smd = (skillmod_data *)an->data.v;
    smd->special = sm_familiar;
    smd->skill=NOSKILL;
  }
  return true;
}

static void *
resolve_familiar(variant data)
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
	variant id;
	fscanf(F, "%s", buf);
	id.i = atoi36(buf);
	ur_add(id, &a->data.v, resolve_familiar);
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
resolve_clone(variant data)
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
	variant id;
	fscanf(F, "%s", buf);
	id.i = atoi36(buf);
	ur_add(id, &a->data.v, resolve_clone);
	return AT_READ_OK;
}

/* mages */

static void *
resolve_mage(variant data)
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
	variant id;
	fscanf(F, "%s", buf);
	id.i = atoi36(buf);
	ur_add(id, &a->data.v, resolve_mage);
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

  if (u) switch (get_keyword(u->thisorder)) {
    case K_ROUTE:
    case K_MOVE:
    case K_FOLLOW:
      return true;
  }
  return false;
}

static castorder *
cast_cmd(unit * u, order * ord)
{
  region * r = u->region;
  region * target_r = r;
  int level, range;
  unit *familiar = NULL, *mage = u;
  const char * s;
  spell * sp;
  spellparameter *args = NULL;

  if (LongHunger(u)) {
    cmistake(u, ord, 224, MSG_MAGIC);
    return 0;
  }
  if (r->planep && fval(r->planep, PFL_NOMAGIC)) {
    cmistake(u, ord, 269, MSG_MAGIC);
    return 0;
  }
  level = eff_skill(u, SK_MAGIC, r);

  init_tokens(ord);
  skip_token();
  s = getstrtoken();
  /* für Syntax ' STUFE x REGION y z ' */
  if (findparam(s, u->faction->locale) == P_LEVEL) {
    s = getstrtoken();
    level = min(atoip(s), level);
    if (level < 1) {
      /* Fehler "Das macht wenig Sinn" */
      cmistake(u, ord, 10, MSG_MAGIC);
      return 0;
    }
    s = getstrtoken();
  }
  if (findparam(s, u->faction->locale) == P_REGION) {
    short t_x = (short)atoi(getstrtoken());
    short t_y = (short)atoi(getstrtoken());
    t_x = rel_to_abs(getplane(u->region),u->faction,t_x,0);
    t_y = rel_to_abs(getplane(u->region),u->faction,t_y,1);
    target_r = findregion(t_x, t_y);
    if (!target_r) {
      /* Fehler "Die Region konnte nicht verzaubert werden" */
      ADDMSG(&mage->faction->msgs, msg_message("spellregionresists",
        "unit region command", mage, mage->region, ord));
      return 0;
    }
    s = getstrtoken();
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
      return 0;
    }
  }
  if (!s[0] || strlen(s) == 0) {
    /* Fehler "Es wurde kein Zauber angegeben" */
    cmistake(u, ord, 172, MSG_MAGIC);
    return 0;
  }
  sp = get_spellfromtoken(u, s, u->faction->locale);

  /* Vertraute können auch Zauber sprechen, die sie selbst nicht
  * können. get_spellfromtoken findet aber nur jene Sprüche, die
  * die Einheit beherrscht. */
  if (sp == NULL && is_familiar(u)) {
    familiar = u;
    mage = get_familiar_mage(u);
    if (mage!=NULL) sp = get_spellfromtoken(mage, s, mage->faction->locale);
  }

  if (sp == NULL) {
    /* Fehler 'Spell not found' */
    cmistake(u, ord, 173, MSG_MAGIC);
    return 0;
  }
  /* um testen auf spruchnamen zu unterbinden sollte vor allen
  * fehlermeldungen die anzeigen das der magier diesen Spruch
  * nur in diese Situation nicht anwenden kann, noch eine
  * einfache Sicherheitsprüfung kommen */
  if (!knowsspell(r, u, sp)) {
    /* vorsicht! u kann der familiar sein */
    if (!familiar) {
      cmistake(u, ord, 173, MSG_MAGIC);
      return 0;
    }
  }
  if (sp->sptyp & ISCOMBATSPELL) {
    /* Fehler: "Dieser Zauber ist nur im Kampf sinnvoll" */
    cmistake(u, ord, 174, MSG_MAGIC);
    return 0;
  }
  /* Auf dem Ozean Zaubern als quasi-langer Befehl können
  * normalerweise nur Meermenschen, ausgenommen explizit als
  * OCEANCASTABLE deklarierte Sprüche */
  if (fval(r->terrain, SEA_REGION)) {
    if (u->race != new_race[RC_AQUARIAN]
    && !fval(u->race, RCF_SWIM)
      && !(sp->sptyp & OCEANCASTABLE)) {
        /* Fehlermeldung */
        ADDMSG(&u->faction->msgs, msg_message("spellfail_onocean",
          "unit region command", u, u->region, ord));
        return 0;
      }
      /* Auf bewegenden Schiffen kann man nur explizit als
      * ONSHIPCAST deklarierte Zauber sprechen */
  } else if (u->ship) {
    if (is_moving_ship(r, u->ship)) {
      if (!(sp->sptyp & ONSHIPCAST)) {
        /* Fehler: "Diesen Spruch kann man nicht auf einem sich
        * bewegenden Schiff stehend zaubern" */
        cmistake(u, ord, 175, MSG_MAGIC);
        return 0;
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
      return 0;
    }
    if (range > 1024) { /* (2^10) weiter als 10 Regionen entfernt */
      ADDMSG(&u->faction->msgs, msg_message("spellfail::nocontact",
        "mage region command target", u, u->region, ord, 
        regionname(target_r, u->faction)));
      return 0;
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
      return 0;
    }
    if (!knowsspell(r, u, sp)) { /* Magier zaubert durch Vertrauten */
      mage = get_familiar_mage(u);
      if (range > 1) { /* Fehler! Versucht zu Farcasten */
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "familiar_farcast", 
          "mage", mage));
        return 0;
      }
      if (distance(mage->region, r) > eff_skill(mage, SK_MAGIC, mage->region)) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "familiar_toofar", 
          "mage", mage));
        return 0;
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
    int p = 0, size = 2;
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
      return 0;
    }
  }
  return new_castorder(mage, familiar, sp, target_r, level, 0, range, ord, args);
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
  int rank;
  castorder *co;
  spellrank spellranks[MAX_SPELLRANK];

  memset(spellranks, 0, sizeof(spellranks));

  for (r = regions; r; r = r->next) {
    unit *u;
    for (u = r->units; u; u = u->next) {
      order * ord;

      if (u->number<=0 || u->race == new_race[RC_SPELL])
        continue;

      if (u->race == new_race[RC_INSECT] && r_insectstalled(r) &&
        !is_cursed(u->attribs, C_KAELTESCHUTZ,0))
        continue;

      if (fval(u, UFL_WERE|UFL_LONGACTION)) {
        continue;
      }

      for (ord = u->orders; ord; ord = ord->next) {
        if (get_keyword(ord) == K_CAST) {
          castorder * co = cast_cmd(u, ord);
          fset(u, UFL_LONGACTION|UFL_NOTMOVING);
          if (co) {
            const spell * sp = co->sp;
            add_castorder(&spellranks[sp->rank], co);
          }
        }
      }
    }
  }

  /* Da sich die Aura und Komponenten in der Zwischenzeit verändert
   * haben können und sich durch vorherige Sprüche das Zaubern
   * erschwert haben kann, muss beim zaubern erneut geprüft werden, ob der
   * Spruch überhaupt gezaubert werden kann.
   * (level) die effektive Stärke des Spruchs (= Stufe, auf der der
   * Spruch gezaubert wird) */

  for (rank = 0; rank < MAX_SPELLRANK; rank++) {
    for (co = spellranks[rank].begin; co; co = co->next) {
      order * ord = co->order;
      int verify, cast_level = co->level;
      boolean fumbled = false;
      unit * u = co->magician.u;
      const spell *sp = co->sp;
      region * target_r = co->rt;

      /* reichen die Komponenten nicht, wird der Level reduziert. */
      co->level = eff_spelllevel(u, sp, cast_level, co->distance);

      if (co->level < 1) {
        /* Fehlermeldung mit Komponenten generieren */
        cancast(u, sp, cast_level, co->distance, ord);
        continue;
      }

      if (cast_level > co->level) {
        /* Sprüche mit Fixkosten werden immer auf Stufe des Spruchs
        * gezaubert, co->level ist aber defaultmäßig Stufe des Magiers */
        if (spl_costtyp(sp) != SPC_FIX) {
          ADDMSG(&u->faction->msgs, msg_message("missing_components", 
            "unit spell level", u, sp, cast_level));
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
        ADDMSG(&u->faction->msgs, msg_message("missing_force", 
          "unit spell level", u, sp, co->level));
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
          ADDMSG(&u->faction->msgs, msg_message("spell_resist", "unit region spell",
            u, r, sp));
          co->force = 0;
        }
      }

      /* Auch für Patzer gibt es Erfahrung, müssen die Spruchkosten
      * bezahlt werden und die nachfolgenden Sprüche werden teurer */
      if (co->force>0 && fumble(target_r, u, sp, co->level)) {
        /* zuerst bezahlen, dann evt in do_fumble alle Aura verlieren */
        fumbled = true;
      } else if (co->force>0) {
        if (sp->sp_function==NULL) {
          log_error(("spell '%s' has no function.\n", sp->sname));
          co->level = 0;
        } else {
          co->level = ((nspell_f)sp->sp_function)(co);
        }
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
    unit *u;
    for (u = r->units; u; u = u->next) {
      if (is_mage(u) && countspells(u, 0) > 0) {
        produceexp(u, SK_MAGIC, u->number);
        /* Spruchlistenaktualiesierung ist in Regeneration */
      }
    }
  }
  for (rank = 0; rank < MAX_SPELLRANK; rank++) {
    free_castorders(spellranks[rank].begin);
  }
  remove_empty_units();
}

const char *
spell_info(const spell * sp, const struct locale * lang)
{
  return LOC(lang, mkname("spellinfo", sp->sname));
}

const char *
spell_name(const spell * sp, const struct locale * lang)
{
  return LOC(lang, mkname("spell", sp->sname));
}

void
spelllist_add(spell_list ** lspells, spell * sp)
{
  spell_list * entry;

  while (*lspells) {
    spell_list * slist = *lspells;
    if (slist->data->id==sp->id) {
      if (slist->data==sp) {
        log_error(("trying to add spell '%s' to a list twice.\n", sp->sname));
        return;
      }
    }
    if (slist->data->id>sp->id) break;
    lspells = &slist->next;
  }
  entry = malloc(sizeof(spell_list));
  entry->data = sp;
  entry->next = *lspells;
  *lspells = entry;
}

spell_list ** 
spelllist_find(spell_list ** lspells, const spell * sp)
{
  while (*lspells) {
    spell_list * slist = *lspells;
    if (slist->data->id>=sp->id) break;
    lspells = &slist->next;
  }
  return lspells;
}
