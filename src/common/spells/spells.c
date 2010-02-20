/* vi: set ts=2:
 *
 *
 * Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <platform.h>
#include <kernel/eressea.h>
#include "spells.h"

#include "buildingcurse.h"
#include "regioncurse.h"
#include "unitcurse.h"
#include "shipcurse.h"
#include "alp.h"
#include "combatspells.h"

/* kernel includes */
#include <kernel/curse.h>
#include <kernel/battle.h> /* für lovar */
#include <kernel/connection.h>
#include <kernel/building.h>
#include <kernel/curse.h>
#include <kernel/spellid.h>
#include <kernel/faction.h>
#include <kernel/reports.h>
#include <kernel/item.h>
#include <kernel/karma.h>
#include <kernel/magic.h>
#include <kernel/message.h>
#include <kernel/objtypes.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/save.h>
#include <kernel/ship.h>
#include <kernel/skill.h>
#include <kernel/spell.h>
#include <kernel/teleport.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>
#include <kernel/version.h>

/* spells includes */
#include <spells/alp.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/language.h>
#include <util/message.h>
#include <util/parser.h>
#include <util/umlaut.h>
#include <util/functions.h>
#include <util/lists.h>
#include <util/rand.h>
#include <util/log.h>
#include <util/nrmessage.h>
#include <util/bsdstring.h>
#include <util/variant.h>
#include <util/goodies.h>
#include <util/resolve.h>
#include <util/rng.h>
#include <util/storage.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

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

static double zero_effect = 0.0;

attrib_type at_unitdissolve = {
  "unitdissolve", NULL, NULL, NULL, a_writechars, a_readchars
};

attrib_type at_wdwpyramid = {
  "wdwpyramid", NULL, NULL, NULL, a_writevoid, a_readvoid
};

/* ----------------------------------------------------------------------- */

static void
report_spell(unit * mage, region * r, message * msg)
{
  r_addmessage(r, NULL, msg);
  if (mage && mage->region!=r) {
    add_message(&mage->faction->msgs, msg);
  }
}

static void
report_failure(unit * mage, struct order * ord) {
  /* Fehler: "Der Zauber schlägt fehl" */
  cmistake(mage, ord, 180, MSG_MAGIC);
}

/* ------------------------------------------------------------- */
/* Spruchanalyse - Ausgabe von curse->info und curse->name       */
/* ------------------------------------------------------------- */

static double
curse_chance(const struct curse * c, double force)
{
  return 1.0 + (force - c->vigour) * 0.1;
}

static void
magicanalyse_region(region *r, unit *mage, double force)
{
  attrib *a;
  boolean found = false;

  for (a=r->attribs;a;a=a->next) {
    curse * c = (curse*)a->data.v;
    double probability;
    int mon;

    if (!fval(a->type, ATF_CURSE)) continue;

    /* ist der curse schwächer als der Analysezauber, so ergibt sich
     * mehr als 100% probability und damit immer ein Erfolg. */
    probability = curse_chance(c, force);
    mon = c->duration + (rng_int()%10) - 5;
    mon = MAX(1, mon);
    found = true;

    if (chance(probability)) { /* Analyse geglückt */
      if (c_flags(c) & CURSE_NOAGE) {
        ADDMSG(&mage->faction->msgs, msg_message(
          "analyse_region_noage", "mage region curse",
          mage, r, c->type));
      } else {
        ADDMSG(&mage->faction->msgs, msg_message(
          "analyse_region_age", "mage region curse months",
          mage, r, c->type, mon));
      }
    } else {
      ADDMSG(&mage->faction->msgs, msg_message(
        "analyse_region_fail", "mage region", mage, r));
    }
  }
  if (!found) {
    ADDMSG(&mage->faction->msgs, msg_message(
      "analyse_region_nospell", "mage region", mage, r));
  }
}

static void
magicanalyse_unit(unit *u, unit *mage, double force)
{
  attrib *a;
  boolean found = false;

  for (a=u->attribs;a;a=a->next) {
    curse * c;
    double probability;
    int mon;
    if (!fval(a->type, ATF_CURSE)) continue;

    c = (curse*)a->data.v;
    /* ist der curse schwächer als der Analysezauber, so ergibt sich
     * mehr als 100% probability und damit immer ein Erfolg. */
    probability = curse_chance(c, force);
    mon = c->duration + (rng_int()%10) - 5;
    mon = MAX(1,mon);

    if (chance(probability)) { /* Analyse geglückt */
      if (c_flags(c) & CURSE_NOAGE) {
        ADDMSG(&mage->faction->msgs, msg_message(
          "analyse_unit_noage", "mage unit curse",
          mage, u, c->type));
      }else{
        ADDMSG(&mage->faction->msgs, msg_message(
          "analyse_unit_age", "mage unit curse months",
          mage, u, c->type, mon));
      }
    } else {
      ADDMSG(&mage->faction->msgs, msg_message(
        "analyse_unit_fail", "mage unit", mage, u));
    }
  }
  if (!found) {
    ADDMSG(&mage->faction->msgs, msg_message(
      "analyse_unit_nospell", "mage target", mage, u));
  }
}

static void
magicanalyse_building(building *b, unit *mage, double force)
{
  attrib *a;
  boolean found = false;

  for (a=b->attribs;a;a=a->next) {
    curse * c;
    double probability;
    int mon;

    if (!fval(a->type, ATF_CURSE)) continue;

    c = (curse*)a->data.v;
    /* ist der curse schwächer als der Analysezauber, so ergibt sich
     * mehr als 100% probability und damit immer ein Erfolg. */
    probability = curse_chance(c, force);
    mon = c->duration + (rng_int()%10) - 5;
    mon = MAX(1,mon);

    if (chance(probability)) { /* Analyse geglückt */
      if (c_flags(c) & CURSE_NOAGE) {
        ADDMSG(&mage->faction->msgs, msg_message(
          "analyse_building_age", "mage building curse",
          mage, b, c->type));
      }else{
        ADDMSG(&mage->faction->msgs, msg_message(
          "analyse_building_age", "mage building curse months",
          mage, b, c->type, mon));
      }
    } else {
      ADDMSG(&mage->faction->msgs, msg_message(
        "analyse_building_fail", "mage building", mage, b));
    }
  }
  if (!found) {
    ADDMSG(&mage->faction->msgs, msg_message(
      "analyse_building_nospell", "mage building", mage, b));
  }

}

static void
magicanalyse_ship(ship *sh, unit *mage, double force)
{
  attrib *a;
  boolean found = false;

  for (a=sh->attribs;a;a=a->next) {
    curse * c;
    double probability;
    int mon;
    if (!fval(a->type, ATF_CURSE)) continue;

    c = (curse*)a->data.v;
    /* ist der curse schwächer als der Analysezauber, so ergibt sich
     * mehr als 100% probability und damit immer ein Erfolg. */
    probability = curse_chance(c, force);
    mon = c->duration + (rng_int()%10) - 5;
    mon = MAX(1,mon);

    if (chance(probability)) { /* Analyse geglückt */
      if (c_flags(c) & CURSE_NOAGE) {
        ADDMSG(&mage->faction->msgs, msg_message(
          "analyse_ship_noage", "mage ship curse",
          mage, sh, c->type));
      }else{
        ADDMSG(&mage->faction->msgs, msg_message(
          "analyse_ship_age", "mage ship curse months",
          mage, sh, c->type, mon));
      }
    } else {
      ADDMSG(&mage->faction->msgs, msg_message(
        "analyse_ship_fail", "mage ship", mage, sh));

    }
  }
  if (!found) {
    ADDMSG(&mage->faction->msgs, msg_message(
      "analyse_ship_nospell", "mage ship", mage, sh));
  }

}

/* ------------------------------------------------------------- */
/* Antimagie - curse auflösen */
/* ------------------------------------------------------------- */

/* Wenn der Curse schwächer ist als der cast_level, dann wird er
 * aufgelöst, bzw seine Kraft (vigour) auf 0 gesetzt.
 * Ist der cast_level zu gering, hat die Antimagie nur mit einer Chance
 * von 100-20*Stufenunterschied % eine Wirkung auf den Curse. Dann wird
 * die Kraft des Curse um die halbe Stärke der Antimagie reduziert.
 * Zurückgegeben wird der noch unverbrauchte Rest von force.
 */
double
destr_curse(curse* c, int cast_level, double force)
{
  if (cast_level < c->vigour) { /* Zauber ist nicht stark genug */
    double probability = 0.1 + (cast_level - c->vigour)*0.2;
    /* pro Stufe Unterschied -20% */
    if (chance(probability)) {
      force -= c->vigour;
      if (c->type->change_vigour) {
        c->type->change_vigour(c, -(cast_level+1/2));
      } else {
        c->vigour -= cast_level+1/2;
      }
    }
  } else { /* Zauber ist stärker als curse */
    if (force >= c->vigour) { /* reicht die Kraft noch aus? */
      force -= c->vigour;
      if (c->type->change_vigour) {
        c->type->change_vigour(c, -c->vigour);
      } else {
        c->vigour = 0;
      }

    }
  }
  return force;
}

int
break_curse(attrib **alist, int cast_level, double force, curse * c)
{
  int succ = 0;
/*  attrib **a = a_find(*ap, &at_curse); */
  attrib ** ap = alist;

  while (*ap && force > 0) {
    curse * c1;
    attrib * a = *ap;
    if (!fval(a->type, ATF_CURSE)) {
      do { ap = &(*ap)->next; } while (*ap && a->type==(*ap)->type);
      continue;
    }
    c1 = (curse*)a->data.v;

    /* Immunität prüfen */
    if (c_flags(c1) & CURSE_IMMUNE) {
      do { ap = &(*ap)->next; } while (*ap && a->type==(*ap)->type);
      continue;
    }

    /* Wenn kein spezieller cursetyp angegeben ist, soll die Antimagie
     * auf alle Verzauberungen wirken. Ansonsten prüfe, ob der Curse vom
     * richtigen Typ ist. */
    if (!c || c==c1) {
      double remain = destr_curse(c1, cast_level, force);
      if (remain < force) {
        succ = cast_level;
        force = remain;
      }
      if (c1->vigour <= 0) {
        a_remove(alist, a);
      }
    }
    if (*ap==a) ap = &a->next;
  }
  return succ;
}

/* ------------------------------------------------------------- */
/* Report a spell's effect to the units in the region.
*/

static void
report_effect(region * r, unit * mage, message * seen, message * unseen)
{
#if 0
  unit * u;

  /* melden, 1x pro Partei */
  freset(mage->faction, FFL_SELECT);
  for (u = r->units; u; u = u->next ) freset(u->faction, FFL_SELECT);
  for (u = r->units; u; u = u->next ) {
    if (!fval(u->faction, FFL_SELECT) ) {
      fset(u->faction, FFL_SELECT);

      /* Bei Fernzaubern sieht nur die eigene Partei den Magier */
      if (u->faction != mage->faction) {
        if (r == mage->region) {
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
  if (!fval(mage->faction, FFL_SELECT)) {
    add_message(&mage->faction->msgs, seen);
  }
#else
  int err = report_action(r, mage, seen, ACTION_RESET|ACTION_CANSEE);
  if (err) {
    report_action(r, mage, seen, ACTION_CANNOTSEE);
  }
#endif
}

/* ------------------------------------------------------------- */
/* Die Spruchfunktionen */
/* ------------------------------------------------------------- */
/* Meldungen:
 *
 * Fehlermeldungen sollten als MSG_MAGIC, level ML_MISTAKE oder
 * ML_WARN ausgegeben werden. (stehen im Kopf der Auswertung unter
 * Zauberwirkungen)

  sprintf(buf, "%s in %s: 'ZAUBER %s': [hier die Fehlermeldung].",
    unitname(mage), regionname(mage->region, mage->faction), sa->strings[0]);
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
/* Name:    Vertrauter
 * Stufe:   10
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
 *       Magieg.  Rasse Besonderheiten
 * Eule   Tybied  -/-   fliegt, Auraregeneration
 * Rabe  Ilaun  -/-   fliegt
 * Imp    Draig -/-   Magieresistenz?
 * Fuchs  Gwyrrd  -/-   Wahrnehmung
 * ????   Cerddor -/-   ???? (Singvogel?, Papagei?)
 * Adler  -/-   -/-   fliegt, +Wahrnehmung, =^=Adlerauge-Spruch?
 * Krähe  -/-   -/-   fliegt, +Tarnung (weil unauffällig)
 * Delphin  -/-   Meerm.  schwimmt
 * Wolf   -/-   Ork
 * Hund   -/-   Mensch  kann evtl BEWACHE ausführen
 * Ratte  -/-   Goblin
 * Albatros -/-   -/-   fliegt, kann auf Ozean "landen"
 * Affe   -/-   -/-   kann evtl BEKLAUE ausführen
 * Goblin -/-   !Goblin normale Einheit
 * Katze  -/-   !Katze  normale Einheit
 * Dämon  -/-   !Dämon  normale Einheit
 *
 * Spezielle V. für Katzen, Trolle, Elfen, Dämonen, Insekten, Zwerge?
 */

static const race *
select_familiar(const race * magerace, magic_t magiegebiet)
{
  const race * retval = NULL;
  int rnd = rng_int()%100;

  assert(magerace->familiars[0]);
  if (rnd < 3) {
    race_list * familiarraces = get_familiarraces();
    unsigned int maxlen = listlen(familiarraces);
    if (maxlen>0) {
      race_list * rclist = familiarraces;
      int index = rng_int()%maxlen;
      while (index-->0) {
        rclist = rclist->next;
      }
      retval = rclist->data;
    }
  } else if (rnd < 70) {
    retval = magerace->familiars[0];
  } else {
    retval = magerace->familiars[magiegebiet];
  }

  return retval;
}

/* ------------------------------------------------------------- */
/* der Vertraue des Magiers */

static void
make_familiar(unit *familiar, unit *mage)
{
  /* skills and spells: */
  if (familiar->race->init_familiar!=NULL) {
    familiar->race->init_familiar(familiar);
  } else {
    log_error(("could not perform initialization for familiar %s.\n",
      familiar->faction->race->_name[0]));
  }

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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  const race * rc;
  skill_t sk;
  int dh, dh1, bytes;
  direction_t d;
  message * msg;
  char zText[2048], * bufp = zText;
  size_t size = sizeof(zText) - 1;

  if (get_familiar(mage) != NULL ) {
    cmistake(mage, co->order, 199, MSG_MAGIC);
    return 0;
  }
  rc = select_familiar(mage->faction->race, mage->faction->magiegebiet);
  if (rc==NULL) {
    log_error(("could not find suitable familiar for %s.\n",
      mage->faction->race->_name[0]));
    return 0;
  }

  if (fval(rc, RCF_SWIM) && !fval(rc, RCF_WALK)) {
    int coasts = is_coastregion(r);

    if (coasts == 0) {
      cmistake(mage, co->order, 229, MSG_MAGIC);
      return 0;
    }

    /* In welcher benachbarten Ozeanregion soll der Familiar erscheinen? */
    coasts = rng_int()%coasts;
    dh = -1;
    for(d=0; d<MAXDIRECTIONS; d++) {
      region * rn = rconnect(r,d);
      if (rn && fval(rn->terrain, SEA_REGION)) {
        dh++;
        if (dh == coasts) break;
      }
    }
    target_region = rconnect(r,d);
  }

  msg = msg_message("familiar_name", "unit", mage);
  nr_render(msg, mage->faction->locale, zText, sizeof(zText), mage->faction);
  msg_release(msg);
  familiar = create_unit(target_region, mage->faction, 1, rc, 0, zText, mage);
  setstatus(familiar, ST_FLEE);
  fset(familiar, UFL_LOCKED);
  make_familiar(familiar, mage);

  dh = 0;
  dh1 = 0;
  for (sk=0;sk<MAXSKILLS;sk++) {
    if (rc->bonus[sk] > -5) dh++;
  }

  for (sk=0;sk<MAXSKILLS;sk++) {
    if (rc->bonus[sk] > -5) {
      dh--;
      if (dh1 == 0) {
        dh1 = 1;
      } else {
        if (dh == 0) {
          bytes = (int)strlcpy(bufp, (const char*)LOC(mage->faction->locale, "list_and"), size);
        } else {
          bytes = (int)strlcpy(bufp, (const char*)", ", size);
        }
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      }
      bytes = (int)strlcpy(bufp, (const char*)skillname(sk, mage->faction->locale), size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    }
  }
  ADDMSG(&mage->faction->msgs, msg_message("familiar_describe",
    "mage race skills", mage, rc, zText));
  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Zerstöre Magie
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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  spellparameter *pa = co->par;
  curse * c = NULL;
  char ts[80];
  attrib **ap;
  int obj;
  int succ;

  /* da jeder Zauber force verbraucht und der Zauber auf alles und nicht
   * nur einen Spruch wirken soll, wird die Wirkung hier verstärkt */
  force *= 4;

  /* Objekt ermitteln */
  obj = pa->param[0]->typ;

  switch(obj) {
    case SPP_REGION:
    {
      /* region *tr = pa->param[0]->data.r; -- farcasting! */
      region *tr = co->rt;
      ap = &tr->attribs;
      write_regionname(tr, mage->faction, ts, sizeof(ts));
      break;
    }
    case SPP_TEMP:
    case SPP_UNIT:
    {
      unit *u;
      u = pa->param[0]->data.u;
      ap = &u->attribs;
      write_unitname(u, ts, sizeof(ts));
      break;
    }
    case SPP_BUILDING:
    {
      building *b;
      b = pa->param[0]->data.b;
      ap = &b->attribs;
      write_buildingname(b, ts, sizeof(ts));
      break;
    }
    case SPP_SHIP:
    {
      ship *sh;
      sh = pa->param[0]->data.sh;
      ap = &sh->attribs;
      write_shipname(sh, ts, sizeof(ts));
      break;
    }
    default:
      return 0;
  }

  succ = break_curse(ap, cast_level, force, c);

  if (succ) {
    ADDMSG(&mage->faction->msgs, msg_message(
      "destroy_magic_effect", "unit region command succ target",
      mage, mage->region, co->order, succ, ts));
  } else {
    ADDMSG(&mage->faction->msgs, msg_message(
      "destroy_magic_noeffect", "unit region command",
      mage, mage->region, co->order));
  }

  return MAX(succ, 1);
}

/* ------------------------------------------------------------- */
/* Name:    Transferiere Aura
 * Stufe:  variabel
 * Gebiet:  alle
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
 *  (UNITSPELL|ONSHIPCAST)
 * */

static int
sp_transferaura(castorder *co)
{
  int aura, gain, multi = 2;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  spellparameter *pa = co->par;
  unit * u;
  sc_mage * scm_dst, * scm_src = get_mage(mage);

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (pa->param[0]->flag == TARGET_NOTFOUND) return 0;

  /* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
   * abbrechen aber kosten lassen */
  if (pa->param[0]->flag == TARGET_RESISTS) return cast_level;

  /* Wieviel Transferieren? */
  aura = pa->param[1]->data.i;
  u = pa->param[0]->data.u;
  scm_dst = get_mage(u);

  if (scm_dst==NULL) {
    /* "Zu dieser Einheit kann ich keine Aura übertragen." */
    cmistake(mage, co->order, 207, MSG_MAGIC);
    return 0;
  } else if (scm_src->magietyp==M_TYBIED) {
    if (scm_src->magietyp != scm_dst->magietyp) multi = 3;
  } else if (scm_src->magietyp==M_GRAY) {
    if (scm_src->magietyp != scm_dst->magietyp) multi = 4;
  } else if (scm_dst->magietyp!=scm_src->magietyp) {
    /* "Zu dieser Einheit kann ich keine Aura übertragen." */
    cmistake(mage, co->order, 207, MSG_MAGIC);
    return 0;
  }

  if (aura < multi) {
    /* "Auraangabe fehlerhaft." */
    cmistake(mage, co->order, 208, MSG_MAGIC);
    return 0;
  }

  gain = MIN(aura, scm_src->spellpoints) / multi;
  scm_src->spellpoints -= gain*multi;
  scm_dst->spellpoints += gain;

/*  sprintf(buf, "%s transferiert %d Aura auf %s", unitname(mage),
      gain, unitname(u)); */
  ADDMSG(&mage->faction->msgs, msg_message(
    "auratransfer_success", "unit target aura", mage, u, gain));
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
 * (SHIPSPELL|ONSHIPCAST|SPELLLEVEL|TESTRESISTANCE)
 */

static int
sp_goodwinds(castorder *co)
{
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;
  int duration = cast_level+1;
  spellparameter *pa = co->par;
  message * m;
  ship *sh;
  unit *u;

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (pa->param[0]->flag == TARGET_NOTFOUND) return 0;

  sh = pa->param[0]->data.sh;

  /* keine Probleme mit C_SHIP_SPEEDUP und C_SHIP_FLYING */
  /* NODRIFT bewirkt auch +1 Geschwindigkeit */
  create_curse(mage, &sh->attribs, ct_find("nodrift"), power, duration, zero_effect, 0);

  /* melden, 1x pro Partei */
  freset(mage->faction, FFL_SELECT);
  for(u = r->units; u; u = u->next ) freset(u->faction, FFL_SELECT);
  m = msg_message("wind_effect", "mage ship", mage, sh);
  for(u = r->units; u; u = u->next ) {
    if (u->ship != sh )    /* nur den Schiffsbesatzungen! */
      continue;
    if (!fval(u->faction, FFL_SELECT) ) {
      r_addmessage(r, u->faction, m);
      fset(u->faction, FFL_SELECT);
    }
  }
  if (!fval(mage->faction, FFL_SELECT)) {
    r_addmessage(r, mage->faction, m);
  }
  msg_release(m);

  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Magischer Pfad
 * Stufe:      4
 * Gebiet:     Gwyrrd
 * Wirkung:
 *   für Stufe Runden wird eine (magische) Strasse erzeugt, die wie eine
 *   normale Strasse wirkt.
 *   Im Ozean schlägt der Spruch fehl
 *
 * Flags:
 * (FARCASTING|SPELLLEVEL|REGIONSPELL|ONSHIPCAST|TESTRESISTANCE)
 */
static int
sp_magicstreet(castorder *co)
{
  region *r = co->rt;
  unit *mage = co->magician.u;

  if (!fval(r->terrain, LAND_REGION)) {
    cmistake(mage, co->order, 186, MSG_MAGIC);
    return 0;
  }

  /* wirkt schon in der Zauberrunde! */
  create_curse(mage, &r->attribs, ct_find("magicstreet"), co->force, co->level+1, zero_effect, 0);

  /* melden, 1x pro Partei */
  {
    message * seen = msg_message("path_effect", "mage region", mage, r);
    message * unseen = msg_message("path_effect", "mage region", NULL, r);
    report_effect(r, mage, seen, unseen);
    msg_release(seen);
    msg_release(unseen);
  }

  return co->level;
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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;
  unit *u;
  attrib *a;
  int ents;

  if (rtrees(r,2) == 0) {
    cmistake(mage, co->order, 204, MSG_EVENT);
    /* nicht ohne bäume */
    return 0;
  }

  ents = (int)MIN(power*power, rtrees(r,2));

  u = create_unit(r, mage->faction, ents, new_race[RC_TREEMAN], 0, NULL, mage);

  a = a_new(&at_unitdissolve);
  a->data.ca[0] = 2;  /* An r->trees. */
  a->data.ca[1] = 5;  /* 5% */
  a_add(&u->attribs, a);
  fset(u, UFL_LOCKED);

  rsettrees(r, 2, rtrees(r,2) - ents);

  /* melden, 1x pro Partei */
  {
    message * seen = msg_message("ent_effect", "mage amount", mage, ents);
    message * unseen = msg_message("ent_effect", "mage amount", NULL, ents);
    report_effect(r, mage, seen, unseen);
    msg_release(unseen);
    msg_release(seen);
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
 * (BUILDINGSPELL)
 *
 */
static int
sp_blessstonecircle(castorder *co)
{
  building *b;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  spellparameter *p = co->par;
  message * msg;

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (p->param[0]->flag == TARGET_NOTFOUND) return 0;

  b = p->param[0]->data.b;

  if (b->type != bt_find("stonecircle")) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "error_notstonecircle", "building", b));
    return 0;
  }

  if (b->size < b->type->maxsize) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "error_notcomplete", "building", b));
    return 0;
  }

  b->type = bt_find("blessedstonecircle");

  msg = msg_message("blessedstonecircle_effect", "mage building", mage, b);
  add_message(&r->msgs, msg);
  msg_release(msg);

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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  curse * c;
  double power = co->force;
  double effect = co->force;
  int duration = (int)power+1;

  if (!fval(r->terrain, SEA_REGION)) {
    cmistake(mage, co->order, 205, MSG_MAGIC);
    /* nur auf ozean */
    return 0;
  }

  /* Attribut auf Region.
   * Existiert schon ein curse, so wird dieser verstärkt
   * (Max(Dauer), Max(Stärke))*/
  c = create_curse(mage, &r->attribs, ct_find("maelstrom"), power, duration, effect, 0);

  /* melden, 1x pro Partei */
  {
    message * seen = msg_message("maelstrom_effect", "mage", mage);
    message * unseen = msg_message("maelstrom_effect", "mage", NULL);
    report_effect(r, mage, seen, unseen);
    msg_release(seen);
    msg_release(unseen);
  }

  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Wurzeln der Magie
 * Stufe:      16
 * Kategorie:  Region, neutral
 * Gebiet:     Gwyrrd
 * Wirkung:
 *   Wandelt einen Wald permanent in eine Mallornregion
 *
 * Flags:
 * (FARCASTING | REGIONSPELL | TESTRESISTANCE)
 */
static int
sp_mallorn(castorder *co)
{
  region *r = co->rt;
  int cast_level = co->level;
  unit *mage = co->magician.u;

  if (!fval(r->terrain, LAND_REGION)) {
    cmistake(mage, co->order, 290, MSG_MAGIC);
    return 0;
  }
  if (fval(r, RF_MALLORN)) {
    cmistake(mage, co->order, 291, MSG_MAGIC);
    return 0;
  }

  /* half the trees will die */
  rsettrees(r, 2, rtrees(r,2)/2);
  rsettrees(r, 1, rtrees(r,1)/2);
  rsettrees(r, 0, rtrees(r,0)/2);
  fset(r, RF_MALLORN);

  /* melden, 1x pro Partei */
  {
    message * seen = msg_message("mallorn_effect", "mage", mage);
    message * unseen = msg_message("mallorn_effect", "mage", NULL);
    report_effect(r, mage, seen, unseen);
    msg_release(seen);
    msg_release(unseen);
  }

  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Segen der Erde / Regentanz
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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;
  int duration = (int)power+1;
  /* Attribut auf Region.
   * Existiert schon ein curse, so wird dieser verstärkt
   * (Max(Dauer), Max(Stärke))*/

  if (create_curse(mage, &r->attribs, ct_find("blessedharvest"), power, duration, 1.0, 0)) {
     message * seen = msg_message("harvest_effect", "mage", mage);
     message * unseen = msg_message("harvest_effect", "mage", NULL);
     report_effect(r, mage, seen, unseen);
     msg_release(seen);
     msg_release(unseen);
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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;

  if (!r->land) {
    cmistake(mage, co->order, 296, MSG_MAGIC);
    return 0;
  }
  if (fval(r, RF_MALLORN)) {
    cmistake(mage, co->order, 92, MSG_MAGIC);
    return 0;
  }

  trees = lovar((int)(force * 10 * RESOURCE_QUANTITY)) + (int)force;
  rsettrees(r, 1, rtrees(r,1) + trees);

  /* melden, 1x pro Partei */
  {
    message * seen = msg_message("growtree_effect", "mage amount", mage, trees);
    message * unseen = msg_message("growtree_effect", "mage amount", NULL, trees);
    report_effect(r, mage, seen, unseen);
    msg_release(seen);
    msg_release(unseen);
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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;

  if (!r->land) {
    cmistake(mage, co->order, 296, MSG_MAGIC);
    return 0;
  }
  if (!fval(r, RF_MALLORN)) {
    cmistake(mage, co->order, 91, MSG_MAGIC);
    return 0;
  }

  trees = lovar((int)(force * 10 * RESOURCE_QUANTITY)) + (int)force;
  rsettrees(r, 1, rtrees(r,1) + trees);

  /* melden, 1x pro Partei */
  {
    message * seen = msg_message("growtree_effect", "mage amount", mage, trees);
    message * unseen = msg_message("growtree_effect", "mage amount", NULL, trees);
    report_effect(r, mage, seen, unseen);
    msg_release(seen);
    msg_release(unseen);
  }

  return cast_level;
}

void
patzer_ents(castorder *co)
{
  int ents;
  unit *u;
  region *r = co->rt;
  unit *mage = co->magician.u;
  /* int cast_level = co->level; */
  double force = co->force;

  if (!r->land) {
    cmistake(mage, co->order, 296, MSG_MAGIC);
    return;
  }

  ents = (int)(force*10);
  u = create_unit(r, get_monsters(), ents, new_race[RC_TREEMAN], 0, NULL, NULL);

  /* 'Erfolg' melden */
  ADDMSG(&mage->faction->msgs, msg_message(
        "regionmagic_patzer", "unit region command", mage,
        mage->region, co->order));

  /* melden, 1x pro Partei */
  {
    message * unseen = msg_message("entrise", "region", r);
    report_effect(r, mage, unseen, unseen);
    msg_release(unseen);
  }
}

/* ------------------------------------------------------------- */
/* Name:       Rosthauch
 * Stufe:      3
 * Kategorie:  Einheit, negativ
 * Gebiet:     Gwyrrd
 * Wirkung:
 *  Zerstört zwischen Stufe und Stufe*10 Eisenwaffen
 *
 * Flag:
 * (FARCASTING | SPELLLEVEL | UNITSPELL | TESTCANSEE | TESTRESISTANCE)
 */
/* Syntax: ZAUBER [REGION x y] [STUFE 2] "Rosthauch" 1111 2222 3333 */

typedef struct iron_weapon {
  const struct item_type * type;
  const struct item_type * rusty;
  float chance;
  struct iron_weapon * next;
} iron_weapon;

static iron_weapon * ironweapons = NULL;

void
add_ironweapon(const struct item_type * type, const struct item_type * rusty, float chance)
{
  iron_weapon * iweapon = malloc(sizeof(iron_weapon));
  iweapon->type = type;
  iweapon->rusty = rusty;
  iweapon->chance = chance;
  iweapon->next = ironweapons;
  ironweapons = iweapon;
}

static int
sp_rosthauch(castorder *co)
{
  int n;
  int success = 0;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  int force = (int)co->force;
  spellparameter *pa = co->par;

  if (ironweapons==NULL) {
    add_ironweapon(it_find("sword"), it_find("rustysword"), 1.0);
    add_ironweapon(it_find("axe"), it_find("rustyaxe"), 1.0);
    add_ironweapon(it_find("greatsword"), it_find("rustygreatsword"), 1.0);
    add_ironweapon(it_find("halberd"), it_find("rustyhalberd"), 0.5f);
#ifndef NO_RUSTY_ARMOR
    add_ironweapon(it_find("shield"), it_find("rustyshield"), 0.5f);
    add_ironweapon(it_find("chainmail"), it_find("rustychainmail"), 0.2f);
#endif
  }

  if (force>0) {
    force = rng_int()%((int)(force * 10)) + force;
  }
  /* fuer jede Einheit */
  for (n = 0; n < pa->length; n++) {
    unit *u = pa->param[n]->data.u;
    int ironweapon = 0;
    iron_weapon * iweapon = ironweapons;

    if (force<=0) break;
    if (pa->param[n]->flag & (TARGET_RESISTS|TARGET_NOTFOUND)) continue;

    for (;iweapon!=NULL;iweapon=iweapon->next) {
      item ** ip = i_find(&u->items, iweapon->type);
      if (*ip) {
        int i = MIN((*ip)->number, force);
        if (iweapon->chance<1.0) {
          i = (int)(i*iweapon->chance);
        }
        if (i>0) {
          force -= i;
          ironweapon += i;
          i_change(ip, iweapon->type, -i);
          if (iweapon->rusty) {
            i_change(&u->items, iweapon->rusty, i);
          }
        }
      }
      if (force<=0) break;
    }

    if (ironweapon>0) {
      /* {$mage mage} legt einen Rosthauch auf {target}. {amount} Waffen
       * wurden vom Rost zerfressen */
      ADDMSG(&mage->faction->msgs, msg_message("rust_effect",
        "mage target amount", mage, u, ironweapon));
      ADDMSG(&u->faction->msgs, msg_message("rust_effect",
        "mage target amount",
        cansee(u->faction, r, mage, 0) ? mage:NULL, u, ironweapon));
      success += ironweapon;
    } else {
      /* {$mage mage} legt einen Rosthauch auf {target}, doch der
       * Rosthauch fand keine Nahrung */
      ADDMSG(&mage->faction->msgs, msg_message(
        "rust_fail", "mage target", mage, u));
    }
  }
  /* in success stehen nun die insgesamt zerstörten Waffen. Im
   * ungünstigsten Fall kann pro Stufe nur eine Waffe verzaubert werden,
   * darum wird hier nur für alle Fälle in denen noch weniger Waffen
   * betroffen wurden ein Kostennachlass gegeben */
  return MIN(success, cast_level);
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
 *  ganz normal alles machen. Die Wirkung hält Stufe Wochen an
 *  Insekten haben in Gletschern den selben Malus wie in Bergen. Zu
 *  lange drin, nicht mehr ändern
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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  int duration = MAX(cast_level, (int)force) + 1;
  spellparameter *pa = co->par;
  double effect;

  force*=10;  /* 10 Personen pro Force-Punkt */

  /* für jede Einheit in der Kommandozeile */
  for (n = 0; n < pa->length; n++) {
    if (force < 1)
      break;

    if (pa->param[n]->flag == TARGET_RESISTS
        || pa->param[n]->flag == TARGET_NOTFOUND)
      continue;

    u = pa->param[n]->data.u;

    if (force < u->number) {
      men = (int)force;
    } else {
      men = u->number;
    }

    effect = 1;
    create_curse(mage, &u->attribs, ct_find("insectfur"), cast_level,
        duration, effect, men);

    force -= u->number;
    ADDMSG(&mage->faction->msgs, msg_message(
      "heat_effect", "mage target", mage, u));
    if (u->faction!=mage->faction) ADDMSG(&u->faction->msgs, msg_message(
      "heat_effect", "mage target",
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
 * (UNITSPELL | TESTCANSEE | SPELLLEVEL)
 */
/* Syntax: ZAUBER "Funkenregen" eh1 */

static int
sp_sparkle(castorder *co)
{
  unit *u;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  spellparameter *pa = co->par;
  int duration = cast_level+1;
  double effect;

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (pa->param[0]->flag == TARGET_NOTFOUND) return 0;

  /* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
   * abbrechen aber kosten lassen */
  if (pa->param[0]->flag == TARGET_RESISTS) return cast_level;

  u = pa->param[0]->data.u;
  effect = rng_int() % 0xffffff;
  create_curse(mage, &u->attribs, ct_find("sparkle"), cast_level,
      duration, effect, u->number);

  ADDMSG(&mage->faction->msgs, msg_message(
    "sparkle_effect", "mage target", mage, u));
  if (u->faction!=mage->faction) {
    ADDMSG(&u->faction->msgs, msg_message(
    "sparkle_effect", "mage target", mage, u));
  }

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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  int number = lovar(force*8*RESOURCE_QUANTITY);
  if (number<1) number = 1;

  if (r->terrain == newterrain(T_SWAMP)) {
    cmistake(mage, co->order, 188, MSG_MAGIC);
    return 0;
  }

  u2 = create_unit(r, mage->faction, number, new_race[RC_IRONGOLEM], 0, NULL, mage);

  set_level(u2, SK_ARMORER, 1);
  set_level(u2, SK_WEAPONSMITH, 1);

  a = a_new(&at_unitdissolve);
  a->data.ca[0] = 0;
  a->data.ca[1] = IRONGOLEM_CRUMBLE;
  a_add(&u2->attribs, a);

  ADDMSG(&mage->faction->msgs,
    msg_message("magiccreate_effect", "region command unit amount object",
    mage->region, co->order, mage, number,
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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  int number = lovar(co->force*5*RESOURCE_QUANTITY);
  if (number<1) number = 1;

  if (r->terrain == newterrain(T_SWAMP)) {
    cmistake(mage, co->order, 188, MSG_MAGIC);
    return 0;
  }

  u2 = create_unit(r, mage->faction, number, new_race[RC_STONEGOLEM], 0, NULL, mage);
  set_level(u2, SK_ROAD_BUILDING, 1);
  set_level(u2, SK_BUILDING, 1);

  a = a_new(&at_unitdissolve);
  a->data.ca[0] = 0;
  a->data.ca[1] = STONEGOLEM_CRUMBLE;
  a_add(&u2->attribs, a);

  ADDMSG(&mage->faction->msgs,
    msg_message("magiccreate_effect", "region command unit amount object",
    mage->region, co->order, mage, number,
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

static void
destroy_all_roads(region *r)
{
  int i;

  for(i = 0; i < MAXDIRECTIONS; i++) {
    rsetroad(r,(direction_t)i, 0);
  }
}

static int
sp_great_drought(castorder *co)
{
  unit *u;
  boolean terraform = false;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  int duration = 2;
  double effect;

  if (fval(r->terrain, SEA_REGION) ) {
    cmistake(mage, co->order, 189, MSG_MAGIC);
    /* TODO: vielleicht einen netten Patzer hier? */
    return 0;
  }

  /* sterben */
  rsetpeasants(r, rpeasants(r)/2); /* evtl wuerfeln */
  rsettrees(r, 2, rtrees(r,2)/2);
  rsettrees(r, 1, rtrees(r,1)/2);
  rsettrees(r, 0, rtrees(r,0)/2);
  rsethorses(r, rhorses(r)/2);

  /* Arbeitslohn = 1/4 */
  effect = 4.0; /* curses: higher is stronger */
  create_curse(mage, &r->attribs, ct_find("drought"), force, duration, effect, 0);

  /* terraforming */
  if (rng_int() % 100 < 25) {
    terraform = true;

    switch(rterrain(r)) {
      case T_PLAIN:
        /* rsetterrain(r, T_GRASSLAND); */
        destroy_all_roads(r);
        break;

      case T_SWAMP:
        /* rsetterrain(r, T_GRASSLAND); */
        destroy_all_roads(r);
        break;
/*
      case T_GRASSLAND:
        rsetterrain(r, T_DESERT);
        destroy_all_roads(r);
        break;
*/
      case T_GLACIER:
        if (rng_int() % 100 < 50) {
          rsetterrain(r, T_SWAMP);
          destroy_all_roads(r);
        } else {   /* Ozean */
          destroy_all_roads(r);
          rsetterrain(r, T_OCEAN);
          /* Einheiten dürfen hier auf keinen Fall gelöscht werden! */
          for (u = r->units; u; u = u->next) {
            if (u->race != new_race[RC_SPELL] && u->ship == 0) {
              set_number(u, 0);
            }
          }
          while (r->buildings) {
            remove_building(&r->buildings, r->buildings);
          }
        }
        break;

      default:
        terraform = false;
        break;
    }
  }

  if (!fval(r->terrain, SEA_REGION)) {
    /* not destroying the region, so it should be safe to make this a local 
     * message */
    message * msg;
    const char * mtype;
    if (r->terrain == newterrain(T_SWAMP) && terraform) {
      mtype = "drought_effect_1";
    } else if (!terraform) {
      mtype = "drought_effect_2";
    } else {
      mtype = "drought_effect_3";
    }
    msg = msg_message(mtype, "mage region", mage, r);
    add_message(&r->msgs, msg);
    msg_release(msg);
  } else {
    /* possible that all units here get killed so better to inform with a global
     * message */
    message * msg = msg_message("drought_effect_4", "mage region", mage, r);
    for (u = r->units; u; u = u->next) freset(u->faction, FFL_SELECT);
    for (u = r->units; u; u = u->next) {
      if (!fval(u->faction, FFL_SELECT)) {
        fset(u->faction, FFL_SELECT);
        add_message(&u->faction->msgs, msg);
      }
    }
    if (!fval(mage->faction, FFL_SELECT)) {
      add_message(&mage->faction->msgs, msg);
    }
    msg_release(msg);
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
  unit *mage = co->magician.u;
  spellparameter *pa = co->par;
  double power = co->force;
  int cast_level = co->level;
  region *rt;
  int remaining_cap;
  int n;
  int erfolg = 0;

  if (getplane(r) != 0) {
    cmistake(mage, co->order, 190, MSG_MAGIC);
    return 0;
  }

  if (!r_isforest(r)) {
    cmistake(mage, co->order, 191, MSG_MAGIC);
    return 0;
  }

  rt = r_standard_to_astral(r);
  if (rt==NULL || is_cursed(rt->attribs, C_ASTRALBLOCK, 0) || fval(rt->terrain, FORBIDDEN_REGION)) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "spellfail_astralblock", ""));
    return 0;
  }

  remaining_cap = (int)(power * 500);

  /* fuer jede Einheit */
  for (n = 0; n < pa->length; n++) {
    unit * u = pa->param[n]->data.u;
    spllprm * param = pa->param[n];

    if (param->flag & (TARGET_RESISTS|TARGET_NOTFOUND)) {
      continue;
    }

    if (!ucontact(u, mage)) {
      ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "feedback_no_contact", "target", u));
    } else {
      int w;
      message * m;
      unit * u2;

      if (!can_survive(u, rt)) {
        cmistake(mage, co->order, 231, MSG_MAGIC);
        continue;
      }

      w = weight(u);
      if (remaining_cap - w < 0) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "fail_tooheavy",
          "target", u));
        continue;
      }
      remaining_cap = remaining_cap - w;
      move_unit(u, rt, NULL);
      erfolg = cast_level;

      /* Meldungen in der Ausgangsregion */
      for (u2 = r->units; u2; u2 = u2->next) freset(u2->faction, FFL_SELECT);
      m = NULL;
      for (u2 = r->units; u2; u2 = u2->next ) {
        if (!fval(u2->faction, FFL_SELECT)) {
          if (cansee(u2->faction, r, u, 0)) {
            fset(u2->faction, FFL_SELECT);
            if (!m) m = msg_message("astral_disappear", "unit", u);
            r_addmessage(r, u2->faction, m);
          }
        }
      }
      if (m) msg_release(m);

      /* Meldungen in der Zielregion */
      for (u2 = rt->units; u2; u2 = u2->next) freset(u2->faction, FFL_SELECT);
      m = NULL;
      for (u2 = rt->units; u2; u2 = u2->next ) {
        if (!fval(u2->faction, FFL_SELECT)) {
          if (cansee(u2->faction, rt, u, 0)) {
            fset(u2->faction, FFL_SELECT);
            if (!m) m = msg_message("astral_appear", "unit", u);
            r_addmessage(rt, u2->faction, m);
          }
        }
      }
      if (m) msg_release(m);
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
  region_list *rl, *rl2;
  int tax, tay;
  unit *u, *u2;
  int remaining_cap;
  int n;
  int erfolg = 0;
  region *r = co->rt;
  unit *mage = co->magician.u;
  double power = co->force;
  spellparameter *pa = co->par;
  int cast_level = co->level;

  if (!is_astral(r)) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "spellfail_astralonly", ""));
    return 0;
  }
  if (is_cursed(r->attribs, C_ASTRALBLOCK, 0)) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "spellfail_astralblock", ""));
    return 0;
  }

  remaining_cap = (int)(power * 500);

  if (pa->param[0]->typ != SPP_REGION) {
    report_failure(mage, co->order);
    return 0;
  }

  /* Koordinaten setzen und Region löschen für Überprüfung auf
  * Gültigkeit */
  rt  = pa->param[0]->data.r;
  tax = rt->x;
  tay = rt->y;
  rt  = NULL;

  rl  = astralregions(r, inhabitable);
  rt  = 0;

  rl2 = rl;
  while(rl2) {
    if (rl2->data->x == tax && rl2->data->y == tay) {
      rt = rl2->data;
      break;
    }
    rl2 = rl2->next;
  }
  free_regionlist(rl);

  if (!rt) {
    cmistake(mage, co->order, 195, MSG_MAGIC);
    return 0;
  }

  if (!r_isforest(rt)) {
    cmistake(mage, co->order, 196, MSG_MAGIC);
    return 0;
  }

  /* für jede Einheit in der Kommandozeile */
  for (n = 1; n < pa->length; n++) {
    if (pa->param[n]->flag == TARGET_RESISTS
      || pa->param[n]->flag == TARGET_NOTFOUND)
      continue;

    u = pa->param[n]->data.u;

    if (!ucontact(u, mage)) {
      ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "feedback_no_contact", "target", u));
    } else {
      int w = weight(u);
      if (!can_survive(u, rt)) {
        cmistake(mage, co->order, 231, MSG_MAGIC);
      } else if (remaining_cap - w < 0) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "fail_tooheavy", "target", u));
      } else {
        message * m;

        remaining_cap = remaining_cap - w;
        move_unit(u, rt, NULL);
        erfolg = cast_level;

        /* Meldungen in der Ausgangsregion */

        for (u2 = r->units; u2; u2 = u2->next) freset(u2->faction, FFL_SELECT);
        m = NULL;
        for(u2 = r->units; u2; u2 = u2->next ) {
          if (!fval(u2->faction, FFL_SELECT)) {
            if (cansee(u2->faction, r, u, 0)) {
              fset(u2->faction, FFL_SELECT);
              if (!m) m = msg_message("astral_disappear", "unit", u);
              r_addmessage(r, u2->faction, m);
            }
          }
        }
        if (m) msg_release(m);

        /* Meldungen in der Zielregion */

        for (u2 = rt->units; u2; u2 = u2->next) freset(u2->faction, FFL_SELECT);
        m = NULL;
        for (u2 = rt->units; u2; u2 = u2->next ) {
          if (!fval(u2->faction, FFL_SELECT)) {
            if (cansee(u2->faction, rt, u, 0)) {
              fset(u2->faction, FFL_SELECT);
              if (!m) m = msg_message("astral_appear", "unit", u);
              r_addmessage(rt, u2->faction, m);
            }
          }
        }
        if (m) msg_release(m);
      }
    }
  }
  return erfolg;
}

/* ------------------------------------------------------------- */
/* Name:       Heiliger Boden
 * Stufe:      9
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
  static const curse_type * ctype = NULL;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;
  curse * c;
  message * msg = msg_message("sp_holyground_effect", "mage region", mage, r);
  report_spell(mage, r, msg);
  msg_release(msg);

  if (!ctype) ctype = ct_find("holyground");
  c = create_curse(mage, &r->attribs, ctype, power*power, 1, zero_effect, 0);

  a_removeall(&r->attribs, &at_deathcount);

  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Heimstein
 * Stufe:      7
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
  curse * c;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  double effect;
  message * msg;
  if (!mage->building || mage->building->type != bt_find("castle")) {
    cmistake(mage, co->order, 197, MSG_MAGIC);
    return 0;
  }

  c = create_curse(mage, &mage->building->attribs, ct_find("magicwalls"),
      force*force, 1, zero_effect, 0);

  if (c==NULL) {
    cmistake(mage, co->order, 206, MSG_MAGIC);
    return 0;
  }
  c_setflag(c, CURSE_NOAGE|CURSE_ONLYONE);

  /* Magieresistenz der Burg erhöht sich um 50% */
  effect = 50;
  c = create_curse(mage,  &mage->building->attribs,
    ct_find("magicresistance"), force*force, 1, effect, 0);
  c_setflag(c, CURSE_NOAGE);

  /* melden, 1x pro Partei in der Burg */
  for (u = r->units; u; u = u->next) freset(u->faction, FFL_SELECT);
  msg = msg_message("homestone_effect", "mage building", mage, mage->building);
  for (u = r->units; u; u = u->next) {
    if (!fval(u->faction, FFL_SELECT)) {
      fset(u->faction, FFL_SELECT);
      if (u->building == mage->building) {
        r_addmessage(r, u->faction, msg);
      }
    }
  }
  msg_release(msg);
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
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;
  int duration = (int)power+1;
  message * msg;

  if (fval(r->terrain, SEA_REGION) ) {
    cmistake(mage, co->order, 189, MSG_MAGIC);
    /* TODO: vielleicht einen netten Patzer hier? */
    return 0;
  }

  /* melden, 1x pro Partei */
  msg = msg_message("sp_drought_effect", "mage region", mage, r);
  report_spell(mage, r, msg);
  msg_release(msg);

  /* Wenn schon Duerre herrscht, dann setzen wir nur den Power-Level
   * hoch (evtl dauert dann die Duerre laenger).  Ansonsten volle
   * Auswirkungen.
   */
  c = get_curse(r->attribs, ct_find("drought"));
  if (c) {
    c->vigour = MAX(c->vigour, power);
    c->duration = MAX(c->duration, (int)power);
  } else {
    double effect;
    /* Baeume und Pferde sterben */
    rsettrees(r, 2, rtrees(r,2)/2);
    rsettrees(r, 1, rtrees(r,1)/2);
    rsettrees(r, 0, rtrees(r,0)/2);
    rsethorses(r, rhorses(r)/2);

    effect = 4.0;
    create_curse(mage, &r->attribs, ct_find("drought"), power, duration, effect, 0);
  }
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
 *   4 Rüstung (=PP)
 * Flags:
 * (FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE)
 */
static int
sp_ironkeeper(castorder *co)
{
  unit *keeper;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  message * msg;

  if (r->terrain != newterrain(T_MOUNTAIN) && r->terrain != newterrain(T_GLACIER)) {
    report_failure(mage, co->order);
    return 0;
  }

  keeper = create_unit(r, mage->faction, 1, new_race[RC_IRONKEEPER], 0, NULL, mage);

  /*keeper->age = cast_level + 2;*/
  setstatus(keeper, ST_AVOID); /* kaempft nicht */
  guard(keeper, GUARD_MINING);
  fset(keeper, UFL_ISNEW);
  /* Parteitarnen, damit man nicht sofort weiß, wer dahinter steckt */
  if (rule_stealth_faction()) {
    fset(keeper, UFL_ANON_FACTION);
  }

  {
    trigger * tkill = trigger_killunit(keeper);
    add_trigger(&keeper->attribs, "timer", trigger_timeout(cast_level+2, tkill));
  }

  msg = msg_message("summon_effect", "mage amount race", mage, 1, keeper->race);
  r_addmessage(r, NULL, msg);
  msg_release(msg);

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
  ship *sh;
  unit *u;
  int erfolg = 0;
  region *r = co->rt;
  unit *mage = co->magician.u;
  double power = co->force;
  spellparameter *pa = co->par;
  int n, force = (int)power;
  message * m = NULL;

  /* melden vorbereiten */
  freset(mage->faction, FFL_SELECT);
  for (u = r->units; u; u = u->next) freset(u->faction, FFL_SELECT);

  for (n = 0; n < pa->length; n++) {
    if (force<=0) break;

    if (pa->param[n]->flag == TARGET_RESISTS
        || pa->param[n]->flag == TARGET_NOTFOUND)
      continue;

    sh = pa->param[n]->data.sh;

    /* mit C_SHIP_NODRIFT haben wir kein Problem */
    if (is_cursed(sh->attribs, C_SHIP_FLYING, 0) ) {
      ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "error_spell_on_flying_ship", "ship", sh))
      continue;
    }
    if (is_cursed(sh->attribs, C_SHIP_SPEEDUP, 0) ) {
      ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "error_spell_on_ship_already", "ship", sh))
      continue;
    }

    /* Duration = 1, nur diese Runde */
    create_curse(mage, &sh->attribs, ct_find("stormwind"), power, 1, zero_effect, 0);
    /* Da der Spruch nur diese Runde wirkt wird er nie im Report
     * erscheinen */
    erfolg++;
    force--;

    /* melden vorbereiten: */
    for(u = r->units; u; u = u->next ) {
      if (u->ship == sh ) {
        /* nur den Schiffsbesatzungen! */
        fset(u->faction, FFL_SELECT);
      }
    }
  }
  if (erfolg<pa->length) {
    ADDMSG(&mage->faction->msgs, msg_message("stormwinds_reduced", 
      "unit ships maxships", mage, erfolg, pa->length));
  }
  /* melden, 1x pro Partei auf Schiff und für den Magier */
  fset(mage->faction, FFL_SELECT);
  for (u = r->units; u; u = u->next ) {
    if (fval(u->faction, FFL_SELECT)) {
      freset(u->faction, FFL_SELECT);
      if (erfolg > 0) {
        if (!m) {
          m = msg_message("stormwinds_effect", "unit", mage);
        }
        r_addmessage(r, u->faction, m);
      }
    }
  }
  if (m) msg_release(m);
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
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  message * msg;
  building **blist = &r->buildings;

  while (*blist) {
    building * burg = *blist;

    if (burg->size != 0 && !is_cursed(burg->attribs, C_MAGICWALLS, 0)) {
      /* Magieresistenz */
      if (!target_resists_magic(mage, burg, TYP_BUILDING, 0)) {
        kaputt = MIN(10 * cast_level, burg->size / 4);
        kaputt = MAX(kaputt, 1);
        burg->size -= kaputt;
        if (burg->size == 0) {
          /* TODO: sollten die Insassen nicht Schaden nehmen? */
          remove_building(blist, burg);
        }
      }
    }
    if (*blist==burg) blist=&burg->next;
  }

  /* melden, 1x pro Partei */
  msg = msg_message("earthquake_effect", "mage region", mage, r);
  r_addmessage(r, NULL, msg);
  msg_release(msg);
  return cast_level;
}


/* ------------------------------------------------------------- */
/* CHAOS / M_DRAIG / Draig */
/* ------------------------------------------------------------- */
void
patzer_peasantmob(castorder *co)
{
  int anteil = 6, n;
  unit *u;
  attrib *a;
  region *r;
  unit *mage = co->magician.u;

  if (mage->region->land) {
    r = mage->region;
  } else {
    r = co->rt;
  }

  if (r->land) {
    faction * f = get_monsters();
    const struct locale * lang = f->locale;
    message * msg;

    anteil += rng_int() % 4;
    n = rpeasants(r) * anteil / 10;
    rsetpeasants(r, rpeasants(r) - n);
    assert(rpeasants(r) >= 0);

    u = create_unit(r, f, n, new_race[RC_PEASANT], 0, LOC(f->locale, "angry_mob"), NULL);
    fset(u, UFL_ISNEW);
    /* guard(u, GUARD_ALL);  hier zu früh! Befehl BEWACHE setzten */
    addlist(&u->orders, create_order(K_GUARD, lang, NULL));
    set_order(&u->thisorder, default_order(lang));
    a = a_new(&at_unitdissolve);
    a->data.ca[0] = 1;  /* An rpeasants(r). */
    a->data.ca[1] = 10; /* 10% */
    a_add(&u->attribs, a);
    a_add(&u->attribs, make_hate(mage));

    msg = msg_message("mob_warning", "");
    r_addmessage(r, NULL, msg);
    msg_release(msg);
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
  direction_t i;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double probability;
  double percentage = (rng_int() % 8 + 1) * 0.1;  /* 10 - 80% */
  message * msg;

  int vernichtet_schoesslinge = (int)(rtrees(r, 1) * percentage);
  int destroyed = (int)(rtrees(r, 2) * percentage);

  if (destroyed<1) {
    cmistake(mage, co->order, 198, MSG_MAGIC);
    return 0;
  }

  rsettrees(r, 2, rtrees(r,2) - destroyed);
  rsettrees(r, 1, rtrees(r,1) - vernichtet_schoesslinge);
  probability = destroyed * 0.001;  /* Chance, dass es sich ausbreitet */

  /* melden, 1x pro Partei */
  for (u = r->units; u; u = u->next) freset(u->faction, FFL_SELECT);
  msg = msg_message("forestfire_effect", "mage region amount", mage, r, destroyed+vernichtet_schoesslinge);
  r_addmessage(r, NULL, msg);
  add_message(&mage->faction->msgs, msg);
  msg_release(msg);

  for (i = 0; i < MAXDIRECTIONS; i++ ) {
    nr = rconnect(r, i);
    assert(nr);
    destroyed = 0;
    vernichtet_schoesslinge = 0;

    if (rtrees(nr,2) + rtrees(nr,1) >= 800) {
      if (chance(probability)) {
        destroyed = (int)(rtrees(nr,2) * percentage/2);
        vernichtet_schoesslinge = (int)(rtrees(nr,1) * percentage/2);
      }
    } else if (rtrees(nr,2) + rtrees(nr,1) >= 600) {
      if (chance(probability/2)) {
        destroyed = (int)(rtrees(nr,2) * percentage/4);
        vernichtet_schoesslinge = (int)(rtrees(nr,1) * percentage/4);
      }
    }

    if (destroyed > 0  || vernichtet_schoesslinge > 0) {
      message * m = msg_message("forestfire_spread", "region next trees",
        r, nr, destroyed+vernichtet_schoesslinge);

      add_message(&r->msgs, m);
      add_message(&mage->faction->msgs, m);
      msg_release(m);

      rsettrees(nr, 2, rtrees(nr,2) - destroyed);
      rsettrees(nr, 1, rtrees(nr,1) - vernichtet_schoesslinge);
    }
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
 *  Wirkt MAX(Stufe(Magier) - Stufe(Ziel), rand(3)) Wochen
 * Patzer:
 *  Magier wird selbst betroffen
 *
 * Flags:
 *  (UNITSPELL | SPELLLEVEL | TESTCANSEE | TESTRESISTANCE)
 *
 */
static int
sp_fumblecurse(castorder *co)
{
  unit *target;
  int rx, sx;
  int duration;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  double effect;
  curse * c;
  spellparameter *pa = co->par;

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (pa->param[0]->flag == TARGET_NOTFOUND) return 0;

  target = pa->param[0]->data.u;

  rx = rng_int()%3;
  sx = cast_level - effskill(target, SK_MAGIC);
  duration = MAX(sx, rx) + 1;

  effect = force*0.5;
  c = create_curse(mage, &target->attribs, ct_find("fumble"),
    force, duration, effect, 0);
  if (c == NULL) {
    report_failure(mage, co->order);
    return 0;
  }

  ADDMSG(&target->faction->msgs, msg_message(
    "fumblecurse", "unit region", target, target->region));

  return cast_level;
}

void
patzer_fumblecurse(castorder *co)
{
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  int duration = (cast_level/2)+1;
  double effect;
  curse * c;

  effect = force*0.5;
  c = create_curse(mage, &mage->attribs, ct_find("fumble"), force,
        duration, effect, 0);
  if (c!=NULL) {
    ADDMSG(&mage->faction->msgs, msg_message(
      "magic_fumble", "unit region command",
      mage, mage->region, co->order));
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
  unit *mage = co->magician.u;
  unit *u;
  int cast_level = co->level;
  double power = co->force;
  region_list *rl,*rl2;
  faction *f;
  int time;
  int number;
  const race * race;

  f = get_monsters();

  if (r->terrain != newterrain(T_SWAMP) && r->terrain != newterrain(T_DESERT)
      && r->terrain != newterrain(T_GLACIER)) {
    report_failure(mage, co->order);
    return 0;
  }

  for(time = 1; time < 7; time++) {
    if (rng_int()%100 < 25) {
      switch(rng_int()%3) {
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

  rl = all_in_range(r, (short)power, NULL);

  for(rl2 = rl; rl2; rl2 = rl2->next) {
    region * r2 = rl2->data;
    for(u = r2->units; u; u = u->next) {
      if (u->race == new_race[RC_WYRM] || u->race == new_race[RC_DRAGON]) {
        attrib * a = a_find(u->attribs, &at_targetregion);
        if (!a) {
          a = a_add(&u->attribs, make_targetregion(co->rt));
        } else {
          a->data.v = co->rt;
        }
      }
    }
  }

  ADDMSG(&mage->faction->msgs, msg_message(
        "summondragon", "unit region command target",
        mage, mage->region, co->order, co->rt));

  free_regionlist(rl);
  return cast_level;
}

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
  connection * wall;
} wallcurse;

void
wall_vigour(curse* c, double delta)
{
  wallcurse * wc = (wallcurse*)c->data.v;
  assert(wc->buddy->vigour==c->vigour);
  wc->buddy->vigour += delta;
  if (wc->buddy->vigour<=0) {
    erase_border(wc->wall);
    wc->wall = NULL;
    ((wallcurse*)wc->buddy->data.v)->wall = NULL;
  }
}

const curse_type ct_firewall = {
  "Feuerwand",
  CURSETYP_NORM, 0, (M_DURATION | M_VIGOUR | NO_MERGE),
  NULL, /* curseinfo */
  wall_vigour /* change_vigour */
};

void
cw_init(attrib * a) {
  curse * c;
  curse_init(a);
  c = (curse*)a->data.v;
  c->data.v = calloc(sizeof(wallcurse), 1);
}

void
cw_write(const attrib * a, storage * store) {
  connection * b = ((wallcurse*)((curse*)a->data.v)->data.v)->wall;
  curse_write(a, store);
  store->w_int(store, b->id);
}

typedef struct bresvole {
  unsigned int id;
  curse * self;
} bresolve;

static int resolve_buddy(variant data, void * addr);

static int
cw_read(attrib * a, storage * store)
{
  bresolve * br = calloc(sizeof(bresolve), 1);
  curse * c = (curse*)a->data.v;
  wallcurse * wc = (wallcurse*)c->data.v;
  variant var;

  curse_read(a, store);
  br->self = c;
  br->id = store->r_int(store);

  var.i = br->id;
  ur_add(var, &wc->wall, resolve_borderid);

  var.v = br;
  ur_add(var, &wc->buddy, resolve_buddy);
  return AT_READ_OK;
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

static int
resolve_buddy(variant data, void * addr)
{
  curse * result = NULL;
  bresolve * br = (bresolve*)data.v;

  if (br->id>=0) {
    connection * b = find_border(br->id);

    if (b && b->from && b->to) {
      attrib * a = a_find(b->from->attribs, &at_cursewall);
      while (a && a->data.v!=br->self) {
        curse * c = (curse*)a->data.v;
        wallcurse * wc = (wallcurse*)c->data.v;
        if (wc->wall->id==br->id) break;
        a = a->next;
      }
      if (!a || a->type!=&at_cursewall) {
        a = a_find(b->to->attribs, &at_cursewall);
        while (a && a->type==&at_cursewall && a->data.v!=br->self) {
          curse * c = (curse*)a->data.v;
          wallcurse * wc = (wallcurse*)c->data.v;
          if (wc->wall->id==br->id) break;
          a = a->next;
        }
      }
      if (a && a->type==&at_cursewall) {
        curse * c = (curse*)a->data.v;
        free(br);
        result = c;
      }
    } else {
      /* fail, object does not exist (but if you're still loading then 
      * you may want to try again later) */
      *(curse**)addr = NULL;
      return -1;
    }
  }
  *(curse**)addr = result;
  return 0;
}

static const char *
b_namefirewall(const connection * b, const region * r, const faction * f, int gflags)
{
  const char * bname;
  unused(f);
  unused(r);
  unused(b);
  if (gflags & GF_ARTICLE) bname = "a_firewall";
  else bname = "firewall";

  if (gflags & GF_PURE) return bname;
  return LOC(f->locale, mkname("border", bname));
}

static void
wall_init(connection * b)
{
  wall_data * fd = (wall_data*)calloc(sizeof(wall_data), 1);
  fd->countdown = -1; /* infinite */
  b->data.v = fd;
}

static void
wall_destroy(connection * b)
{
  free(b->data.v);
}

static void
wall_read(connection * b, storage * store)
{
  wall_data * fd = (wall_data*)b->data.v;
  variant mno;
  assert(fd);
  if (store->version<STORAGE_VERSION) {
    mno.i = store->r_int(store);
    fd->mage = findunit(mno.i);
    if (!fd->mage) {
      ur_add(mno, &fd->mage, resolve_unit);
    }
  } else {
    read_reference(&fd->mage, store, read_unit_reference, resolve_unit);
  }
  fd->force = store->r_int(store);
  if (store->version>=NOBORDERATTRIBS_VERSION) {
    fd->countdown = store->r_int(store);
  }
  fd->active = true;
}

static void
wall_write(const connection * b, storage * store)
{
  wall_data * fd = (wall_data*)b->data.v;
  write_unit_reference(fd->mage, store);
  store->w_int(store, fd->force);
  store->w_int(store, fd->countdown);
}

static int
wall_age(connection * b)
{
  wall_data * fd = (wall_data*)b->data.v;
  --fd->countdown;
  return (fd->countdown>0)?AT_AGE_KEEP:AT_AGE_REMOVE;
}

static region *
wall_move(const connection * b, struct unit * u, struct region * from, struct region * to, boolean routing)
{
  wall_data * fd = (wall_data*)b->data.v;
  if (!routing && fd->active) {
    int hp = dice(3, fd->force) * u->number;
    hp = MIN(u->hp, hp);
    u->hp -= hp;
    if (u->hp) {
      ADDMSG(&u->faction->msgs, msg_message("firewall_damage",
        "region unit", from, u));
    }
    else ADDMSG(&u->faction->msgs, msg_message("firewall_death", "region unit", from, u));
    if (u->number>u->hp) {
      scale_number(u, u->hp);
      u->hp = u->number;
    }
  }
  return to;
}

border_type bt_firewall = {
  "firewall", VAR_VOIDPTR,
  b_transparent, /* transparent */
  wall_init, /* init */
  wall_destroy, /* destroy */
  wall_read, /* read */
  wall_write, /* write */
  b_blocknone, /* block */
  b_namefirewall, /* name */
  b_rvisible, /* rvisible */
  b_finvisible, /* fvisible */
  b_uinvisible, /* uvisible */
  NULL,
  wall_move,
  wall_age
};

static int
sp_firewall(castorder *co)
{
  connection * b;
  wall_data * fd;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  spellparameter *pa = co->par;
  direction_t dir;
  region * r2;

  dir = finddirection(pa->param[0]->data.xs, mage->faction->locale);
  if (dir<MAXDIRECTIONS && dir!=NODIRECTION) {
    r2 = rconnect(r, dir);
  } else {
    report_failure(mage, co->order);
    return 0;
  }

  if (!r2 || r2==r) {
    report_failure(mage, co->order);
    return 0;
  }

  b = get_borders(r, r2);
  while (b!=NULL) {
    if (b->type == &bt_firewall) break;
    b = b->next;
  }
  if (b==NULL) {
    b = new_border(&bt_firewall, r, r2);
    fd = (wall_data*)b->data.v;
    fd->force = (int)(force/2+0.5);
    fd->mage = mage;
    fd->active = false;
    fd->countdown = cast_level+1;
  } else {
    fd = (wall_data*)b->data.v;
    fd->force = (int)MAX(fd->force, force/2+0.5);
    fd->countdown = MAX(fd->countdown, cast_level+1);
  }

  /* melden, 1x pro Partei */
  {
    message * seen = msg_message("firewall_effect", "mage region", mage, r);
    message * unseen = msg_message("firewall_effect", "mage region", NULL, r);
    report_effect(r, mage, seen, unseen);
    msg_release(seen);
    msg_release(unseen);
  }

  return cast_level;
}

/* ------------------------------------------------------------- */

static const char *
wisps_name(const connection * b, const region * r, const faction * f, int gflags)
{
  const char * bname;
  unused(f);
  unused(r);
  unused(b);
  if (gflags & GF_ARTICLE) {
    bname = "a_wisps";
  } else {
    bname = "wisps";
  }
  if (gflags & GF_PURE) return bname;
  return LOC(f->locale, mkname("border", bname));
}

typedef struct wisps_data {
  wall_data wall;
  int rnd;
} wisps_data;

static region *
wisps_move(const connection * b, struct unit * u, struct region * from, struct region * next, boolean routing)
{
  direction_t reldir = reldirection(from, next);
  wisps_data * wd = (wisps_data*)b->data.v;
  assert(reldir!=D_SPECIAL);

  if (routing && wd->wall.active) {
    region * rl = rconnect(from, (direction_t)((reldir+MAXDIRECTIONS-1)%MAXDIRECTIONS));
    region * rr = rconnect(from, (direction_t)((reldir+1)%MAXDIRECTIONS));
    /* pick left and right region: */
    if (wd->rnd<0) {
      wd->rnd = rng_int() % 3;
    }

    if (wd->rnd == 1 && rl && fval(rl->terrain, LAND_REGION)==fval(next, LAND_REGION)) return rl;
    if (wd->rnd == 2 && rr && fval(rr->terrain, LAND_REGION)==fval(next, LAND_REGION)) return rr;
  }
  return next;
}

static void
wisps_init(connection * b)
{
  wisps_data * wd = (wisps_data*)calloc(sizeof(wisps_data), 1);

  b->data.v = wd;
  wd->rnd = -1;
}

border_type bt_wisps = {
  "wisps", VAR_VOIDPTR,
  b_transparent, /* transparent */
  wisps_init, /* init */
  wall_destroy, /* destroy */
  wall_read, /* read */
  wall_write, /* write */
  b_blocknone, /* block */
  wisps_name, /* name */
  b_rvisible, /* rvisible */
  b_fvisible, /* fvisible */
  b_uvisible, /* uvisible */
  NULL, /* visible */
  wisps_move
};

static int
sp_wisps(castorder *co)
{
  connection * b;
  wall_data * fd;
  region * r2;
  direction_t dir;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  spellparameter *pa = co->par;

  dir = finddirection(pa->param[0]->data.xs, mage->faction->locale);
  r2 = rconnect(r, dir);

  if (!r2) {
    report_failure(mage, co->order);
    return 0;
  }

  b = new_border(&bt_wisps, r, r2);
  fd = (wall_data*)b->data.v;
  fd->force = (int)(force/2+0.5);
  fd->mage = mage;
  fd->active = false;
  fd->countdown = cast_level+1;

  /* melden, 1x pro Partei */
  {
    message * seen = msg_message("wisps_effect", "mage region", mage, r);
    message * unseen = msg_message("wisps_effect", "mage region", NULL, r);
    report_effect(r, mage, seen, unseen);
    msg_release(seen);
    msg_release(unseen);
  }

  return cast_level;
}

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
 *   (SPELLLEVEL | TESTCANSEE)
 */

static int
sp_unholypower(castorder *co)
{
  unit *mage = co->magician.u;
  int cast_level = co->level;
  spellparameter *pa = co->par;
  int i;
  int n;
  int wounds;

  n = dice((int)co->force, 10);

  for (i = 0; i < pa->length && n > 0; i++) {
    const race * target_race;
    unit *u;

    if (pa->param[i]->flag == TARGET_RESISTS
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
      cmistake(mage, co->order, 284, MSG_MAGIC);
      continue;
    }
    /* Untote heilen nicht, darum den neuen Untoten maximale hp geben
     * und vorhandene Wunden abziehen */
    wounds = unit_max_hp(u)*u->number - u->hp;

    if (u->number <= n) {
      n -= u->number;
      u->irace = NULL;
      u->race = target_race;
      u->hp = unit_max_hp(u)*u->number - wounds;
      ADDMSG(&co->rt->msgs, msg_message("unholypower_effect",
        "mage target race", mage, u, target_race));
    } else {
      unit *un;

      /* Wird hoffentlich niemals vorkommen. Es gibt im Source
       * vermutlich eine ganze Reihe von Stellen, wo das nicht
       * korrekt abgefangen wird. Besser (aber nicht gerade einfach)
       * wäre es, eine solche Konstruktion irgendwie zu kapseln. */
      if (fval(u, UFL_LOCKED) || fval(u, UFL_HUNGER)
          || is_cursed(u->attribs, C_SLAVE, 0)) {
        cmistake(mage, co->order, 74, MSG_MAGIC);
        continue;
      }
      /* Verletzungsanteil der transferierten Personen berechnen */
      wounds = wounds*n/u->number;

      un = create_unit(co->rt, u->faction, 0, target_race, 0, NULL, u);
      transfermen(u, un, n);
      un->hp = unit_max_hp(un)*n - wounds;
      ADDMSG(&co->rt->msgs, msg_message("unholypower_limitedeffect",
        "mage target race amount",
        mage, u, target_race, n));
      n = 0;
    }
  }

  return cast_level;
}

static int
dc_age(struct curse * c)
/* age returns 0 if the attribute needs to be removed, !=0 otherwise */
{
  region * r = (region*)c->data.v;
  unit **up;
  unit * mage = c->magician;
  
  if (r==NULL || mage==NULL || mage->number==0) {
    /* if the mage disappears, so does the spell. */
    return AT_AGE_REMOVE;
  }

  up = &r->units;
  if (curse_active(c)) while (*up!=NULL) {
    unit * u = *up;
    double damage = c->effect * u->number;

    freset(u->faction, FFL_SELECT);
    if (u->number<=0 || target_resists_magic(mage, u, TYP_UNIT, 0)) {
      up=&u->next;
      continue;
    }

    /* Reduziert durch Magieresistenz */
    damage *= (1.0 - magic_resistance(u));
    change_hitpoints(u, -(int)damage);

    if (*up==u) up=&u->next;
  }

  return AT_AGE_KEEP;
}

static struct curse_type ct_deathcloud = {
  "deathcloud", CURSETYP_REGION, 0, NO_MERGE, cinfo_simple, NULL, NULL, NULL, NULL, dc_age
};

static curse *
mk_deathcloud(unit * mage, region * r, double force, int duration)
{
  double effect;
  curse * c;

  effect = force * 0.5;
  c = create_curse(mage, &r->attribs, &ct_deathcloud, force, duration, effect, 0);
  c->data.v = r;
  return c;
}

#define COMPAT_DEATHCLOUD
#ifdef COMPAT_DEATHCLOUD
static int
dc_read_compat(struct attrib * a, storage * store)
/* return AT_READ_OK on success, AT_READ_FAIL if attrib needs removal */
{
  region * r = NULL;
  unit * u;
  variant var;
  int duration = store->r_int(store);
  double strength = store->r_flt(store);
  short rx, ry;

  var.i = store->r_id(store);
  u = findunit(var.i);

  /* this only affects really old data. no need to change: */
  rx = (short)store->r_int(store);
  ry = (short)store->r_int(store);
  r = findregion(rx, ry);

  if (r!=NULL) {
    double effect;
    curse * c;

    effect = strength;
    c = create_curse(u, &r->attribs, &ct_deathcloud, strength * 2, duration, effect, 0);
    c->data.v = r;
    if (u==NULL) {
      ur_add(var, &c->magician, resolve_unit);
    }
  }
  return AT_READ_FAIL; /* we don't care for the attribute. */
}


attrib_type at_deathcloud_compat = {
  "zauber_todeswolke", NULL, NULL, NULL, NULL, dc_read_compat
};
#endif

/* ------------------------------------------------------------- */
/* Name:       Todeswolke
* Stufe:      11
* Gebiet:     Draig
* Kategorie:  Region, negativ
*
* Wirkung:
*   Personen in der Region verlieren stufe/2 Trefferpunkte pro Runde.
*   Dauer force/2
*   Wirkt gegen MR
*   Rüstung wirkt nicht
* Patzer:
*   Magier gerät in den Staub und verliert zufällige Zahl von HP bis
*   auf MAX(hp,2)
* Besonderheiten:
*   Nicht als curse implementiert, was schlecht ist - man kann dadurch
*   kein dispell machen. Wegen fix unter Zeitdruck erstmal nicht zu
*   ändern...
* Missbrauchsmöglichkeit:
*   Hat der Magier mehr HP als Rasse des Feindes (extrem: Dämon/Goblin)
*   so kann er per Farcasting durch mehrmaliges Zaubern eine
*   Nachbarregion auslöschen. Darum sollte dieser Spruch nur einmal auf
*   eine Region gelegt werden können.
*
* Flag:
*   (FARCASTING | REGIONSPELL | TESTRESISTANCE)
*/

static int
sp_deathcloud(castorder *co)
{
  region *r = co->rt;
  unit *mage = co->magician.u;
  attrib *a = r->attribs;
  unit * u;

  while (a) {
    if ((a->type->flags & ATF_CURSE)) {
      curse * c = a->data.v;
      if (c->type==&ct_deathcloud) {
        report_failure(mage, co->order);
        return 0;
      }
      a = a->next;
    }
    else a = a->nexttype;
  }

  mk_deathcloud(mage, r, co->force, co->level);

  /* melden, 1x pro Partei */
  for (u = r->units; u; u = u->next) freset(u->faction, FFL_SELECT);
  for (u = r->units; u; u = u->next ) {
    if (!fval(u->faction, FFL_SELECT) ) {
      fset(u->faction, FFL_SELECT);
      ADDMSG(&u->faction->msgs, msg_message("deathcloud_effect",
        "mage region", cansee(u->faction, r, mage, 0) ? mage : NULL, r));
    }
  }

  if (!fval(mage->faction, FFL_SELECT)) {
    ADDMSG(&mage->faction->msgs, msg_message("deathcloud_effect",
      "mage region", mage, r));
  }

  return co->level;
}

void
patzer_deathcloud(castorder *co)
{
  unit *mage = co->magician.u;
  int hp = (mage->hp - 2);

  change_hitpoints(mage, -rng_int()%hp);

  ADDMSG(&mage->faction->msgs, msg_message(
    "magic_fumble", "unit region command",
    mage, mage->region, co->order));

  return;
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
  unit *mage = co->magician.u;
  int cast_level = co->level;

  plagues(r, true);

  ADDMSG(&mage->faction->msgs, msg_message("plague_spell",
    "region mage", r, mage));
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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  unit *u;
  int val, number = (int)(force*force);

  u = create_unit(r, mage->faction, number, new_race[RC_SHADOW], 0, NULL, mage);

  /* Bekommen Tarnung = (Magie+Tarnung)/2 und Wahrnehmung 1. */
  val = get_level(mage, SK_MAGIC) + get_level(mage, SK_STEALTH);

  set_level(u, SK_STEALTH, val);
  set_level(u, SK_PERCEPTION, 1);

  ADDMSG(&mage->faction->msgs, msg_message("summonshadow_effect",
    "mage number", mage, number));

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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  int amount = (int)(force*force);

  u = create_unit(r, mage->faction, amount, new_race[RC_SHADOWLORD], 0, NULL, mage);

  /* Bekommen Tarnung = Magie und Wahrnehmung 5. */
  set_level(u, SK_STEALTH, get_level(mage, SK_MAGIC));
  set_level(u, SK_PERCEPTION, 5);

  ADDMSG(&mage->faction->msgs, msg_message("summon_effect", "mage amount race", mage, amount, u->race));
  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Chaossog
 * Stufe:      14
 * Gebiet:     Draig
 * Kategorie:  Teleport
 * Wirkung:
 *  Durch das Opfern von 200 Bauern kann der Chaosmagier ein Tor zur
 *  astralen Welt öffnen. Das Tor kann im Folgemonat verwendet werden,
 *  es löst sich am Ende des Folgemonats auf.
 *
 * Flag:  (0)
 */
static int
sp_chaossuction(castorder *co)
{
  region *rt;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;

  if (getplane(r)!=get_normalplane()) {
    /* Der Zauber funktioniert nur in der materiellen Welt. */
    cmistake(mage, co->order, 190, MSG_MAGIC);
    return 0;
  }

  rt  = r_standard_to_astral(r);

  if (rt==NULL || fval(rt->terrain, FORBIDDEN_REGION)) {
    /* Hier gibt es keine Verbindung zur astralen Welt.*/
    cmistake(mage, co->order, 216, MSG_MAGIC);
    return 0;
  } else if (is_cursed(rt->attribs, C_ASTRALBLOCK, 0)) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "spellfail_astralblock", ""));
    return 0;
  }

  /* TODO: implement with a building */
  create_special_direction(r, rt, 2, "vortex_desc", "vortex");
  create_special_direction(rt, r, 2, "vortex_desc", "vortex");
  new_border(&bt_chaosgate, r, rt);

  add_message(&r->msgs, msg_message("chaosgate_effect_1", "mage", mage));
  add_message(&rt->msgs, msg_message("chaosgate_effect_2", ""));
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
  curse * c;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;
  double effect;
  trigger * tsummon;
  static const curse_type * ct_auraboost;
  static const curse_type * ct_magicboost;

  if (!ct_auraboost) {
    ct_auraboost = ct_find("auraboost");
    ct_magicboost = ct_find("magicboost");
    assert(ct_auraboost!=NULL);
    assert(ct_magicboost!=NULL);
  }
  /* fehler, wenn schon ein boost */
  if (is_cursed(mage->attribs, C_MBOOST, 0) == true) {
    report_failure(mage, co->order);
    return 0;
  }

  effect = 6;
  c = create_curse(mage, &mage->attribs, ct_magicboost, power, 10, effect, 1);

  /* one aura boost with 200% aura now: */
  effect = 200;
  c = create_curse(mage, &mage->attribs, ct_auraboost, power, 4, effect, 1);

  /* and one aura boost with 50% aura in 5 weeks: */
  tsummon = trigger_createcurse(mage, mage, ct_auraboost, power, 6, 50, 1);
  add_trigger(&mage->attribs, "timer", trigger_timeout(5, tsummon));

  ADDMSG(&mage->faction->msgs, msg_message("magicboost_effect",
    "unit region command", mage, mage->region, co->order));

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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  int aura;
  int skill = eff_skill(mage, SK_MAGIC, mage->region);
  int hp = (int)(co->force*8);

  if (hp <= 0) {
    report_failure(mage, co->order);
    return 0;
  }

  aura = lovar(hp);

  if (skill < 8) {
    aura /= 4;
  } else if (skill < 12) {
    aura /=  3;
  } else if (skill < 15) {
    aura /= 2;
  /* von 15 bis 17 ist hp = aura */
  } else if (skill > 17) {
    aura *= 2;
  }

  if (aura <= 0) {
    report_failure(mage, co->order);
    return 0;
  }

  /* sicherheitshalber gibs hier einen HP gratis. sonst schaffen es
   * garantiert ne ganze reihe von leuten ihren Magier damit umzubringen */
  mage->hp++;
  change_spellpoints(mage, aura);
  ADDMSG(&mage->faction->msgs,
    msg_message("sp_bloodsacrifice_effect",
    "unit region command amount",
    mage, mage->region, co->order, aura));
  return cast_level;
}

/** gives a summoned undead unit some base skills.
 */
static void
skill_summoned(unit * u, int level)
{
  if (level>0) {
    const race * rc = u->race;
    skill_t sk;
    for (sk=0;sk!=MAXSKILLS;++sk) {
      if (rc->bonus[sk]>0) {
        set_level(u, sk, level);
      }
    }
  }
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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  int force = (int)(co->force*10);
  const race * race = new_race[RC_SKELETON];

  if (!r->land || deathcount(r) == 0) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "error_nograves", "target", r));
    return 0;
  }

  undead = MIN(deathcount(r), 2 + lovar(force));

  if (cast_level <= 8) {
    race = new_race[RC_SKELETON];
  } else if (cast_level <= 12) {
    race = new_race[RC_ZOMBIE];
  } else {
    race = new_race[RC_GHOUL];
  }

  u = create_unit(r, mage->faction, undead, race, 0, NULL, mage);
  make_undead_unit(u);
  skill_summoned(u, cast_level/2);

  ADDMSG(&mage->faction->msgs, msg_message("summonundead_effect_1", "mage region amount", mage, r, undead));
  ADDMSG(&r->msgs, msg_message("summonundead_effect_2", "mage region", mage, r));
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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  message * msg;
  
  lost = MIN(0.95, cast_level * 0.05);

  for (u = r->units; u; u = u->next) {
    if (is_mage(u)) {
      /* Magieresistenz Einheit?  Bei gegenerischen Magiern nur sehr
       * geringe Chance auf Erfolg wg erhöhter MR, würde Spruch sinnlos
       * machen */
      lost_aura = (int)(get_spellpoints(u)*lost);
      change_spellpoints(u, -lost_aura);
    }
  }
  msg = msg_message("cast_auraleak_effect", "mage region", mage, r);
  r_addmessage(r, NULL, msg);
  msg_release(msg);
  return cast_level;
}

/* ------------------------------------------------------------- */
/* BARDE  - CERDDOR*/
/* ------------------------------------------------------------- */
/* ------------------------------------------------------------- */
/* Name:       Magie analysieren - Gebäude, Schiffe, Region
 * Name:       Lied des Ortes analysieren
 * Stufe:      8
 * Gebiet:     Cerddor
 *
 * Wirkung:
 *  Zeigt die Verzauberungen eines Objekts an (curse->name,
 *  curse::info). Aus der Differenz Spruchstärke und Curse->vigour
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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  spellparameter *pa = co->par;

  obj = pa->param[0]->typ;

  switch(obj) {
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
/* Name:     Gesang des Lebens analysieren
 * Name:     Magie analysieren - Unit
 * Stufe:   5
 * Gebiet:   Cerddor
 * Wirkung:
 *  Zeigt die Verzauberungen eines Objekts an (curse->name,
 *  curse::info). Aus der Differenz Spruchstärke und Curse->vigour
 *  ergibt sich die Chance den Spruch zu identifizieren ((force -
 *  c->vigour)*10 + 100 %).
 *
 * Flag:
 *  (UNITSPELL|ONSHIPCAST|TESTCANSEE)
 */
static int
sp_analysesong_unit(castorder *co)
{
  unit *u;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  spellparameter *pa = co->par;

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (pa->param[0]->flag == TARGET_NOTFOUND) return 0;

  /* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
   * abbrechen aber kosten lassen */
  if (pa->param[0]->flag == TARGET_RESISTS) return cast_level;

  u = pa->param[0]->data.u;

  magicanalyse_unit(u, mage, force);

  return cast_level;
}

static boolean
can_charm(const unit * u, int maxlevel)
{
  const skill_t expskills[] = { SK_ALCHEMY, SK_HERBALISM, SK_MAGIC, SK_SPY, SK_TACTICS, NOSKILL };
  skill * sv = u->skills;

  if (fval(u, UFL_HERO)) return false;

  for (;sv!=u->skills+u->skill_size;++sv) {
    int l = 0, h = 5;
    skill_t sk = sv->id;
    assert(expskills[h]==NOSKILL);
    while (l<h) {
      int m = (l+h)/2;
      if (sk==expskills[m]) {
        if (skill_limit(u->faction, sk)!=INT_MAX) {
          return false;
        } else if ((int)sv->level>maxlevel) {
          return false;
        }
        break;
      }
      else if (sk>expskills[m]) l=m+1;
      else h=m;
    }
  }
  return true;
}
/* ------------------------------------------------------------- */
/* Name:     Charming
 * Stufe:   13
 * Gebiet:   Cerddor
 * Flag:     UNITSPELL
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
 *   (UNITSPELL | TESTCANSEE)
 */
static int
sp_charmingsong(castorder *co)
{
  unit *target;
  int duration;
  skill_t i;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  spellparameter *pa = co->par;
  int resist_bonus = 0;
  int tb = 0;

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (pa->param[0]->flag == TARGET_NOTFOUND) return 0;

  target = pa->param[0]->data.u;

  /* Auf eigene Einheiten versucht zu zaubern? Garantiert Tippfehler */
  if (target->faction == mage->faction) {
    /* Die Einheit ist eine der unsrigen */
    cmistake(mage, co->order, 45, MSG_MAGIC);
  }
  /* niemand mit teurem Talent */
  if (!can_charm(target, cast_level/2)) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, 
      "spellfail_noexpensives", "target", target));
    return 0;
  }

  /* Magieresistensbonus für mehr als Stufe Personen */
  if (target->number > force) {
    resist_bonus += (int)((target->number - force) * 10);
  }
  /* Magieresistensbonus für höhere Talentwerte */
  for(i = 0; i < MAXSKILLS; i++) {
    int sk = effskill(target, i);
    if (tb < sk) tb = sk;
  }
  tb -= effskill(mage, SK_MAGIC);
  if (tb > 0) {
    resist_bonus += tb * 15;
  }
  /* Magieresistenz */
  if (target_resists_magic(mage, target, TYP_UNIT, resist_bonus)) {
    report_failure(mage, co->order);
#if 0
    sprintf(buf, "%s fühlt sich einen Moment lang benommen und desorientiert.",
        unitname(target));
    addmessage(target->region, target->faction, buf, MSG_EVENT, ML_WARN);
#endif
    return 0;
  }

  duration = 3 + rng_int()%(int)force;
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
  create_curse(mage, &target->attribs, ct_find("slavery"), force, duration, zero_effect, 0);

  /* setze Partei um und lösche langen Befehl aus Sicherheitsgründen */
  u_setfaction(target, mage->faction);
  set_order(&target->thisorder, NULL);

  /* setze Parteitarnung, damit nicht sofort klar ist, wer dahinter
   * steckt */
  if (rule_stealth_faction()) {
    fset(target, UFL_ANON_FACTION);
  }

  ADDMSG(&mage->faction->msgs, msg_message("charming_effect", "mage unit duration", mage, target, duration));

  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Gesang des wachen Geistes
 * Stufe:   10
 * Gebiet:   Cerddor
 * Kosten:   SPC_LEVEL
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
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  int duration = (int)force+1;

  create_curse(mage, &r->attribs, ct_find("goodmagicresistancezone"),
      force, duration, 15, 0);

  /* Erfolg melden */
  ADDMSG(&mage->faction->msgs, msg_message(
        "regionmagic_effect", "unit region command", mage,
        mage->region, co->order));

  return cast_level;
}
/* ------------------------------------------------------------- */
/* Name:     Gesang des schwachen Geistes
 * Stufe:   12
 * Gebiet:   Cerddor
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
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  int duration = (int)force+1;

  create_curse(mage, &r->attribs, ct_find("badmagicresistancezone"),
      force, duration, 15, 0);

  ADDMSG(&mage->faction->msgs, msg_message(
        "regionmagic_effect", "unit region command", mage,
        mage->region, co->order));

  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Aufruhr beschwichtigen
 * Stufe:   15
 * Gebiet:   Cerddor
 * Flag:     FARCASTING
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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  message * msg;
  curse * c;

  for (u = r->units; u; u = un) {
    un = u->next;
    if (is_monsters(u->faction) && u->race == new_race[RC_PEASANT]) {
      rsetpeasants(r, rpeasants(r) + u->number);
      rsetmoney(r, rmoney(r) + get_money(u));
      set_money(u, 0);
      setguard(u, GUARD_NONE);
      set_number(u, 0);
      erfolg = cast_level;
    }
  }

  c = get_curse(r->attribs, ct_find(oldcursename(C_RIOT)));
  if (c!=NULL) {
    remove_curse(&r->attribs, c);
  }

  msg = msg_message("cast_rally_effect", "mage region", mage, r);
  r_addmessage(r, NULL, msg);
  msg_release(msg);
  return erfolg;
}

/* ------------------------------------------------------------- */
/* Name:     Aufruhr verursachen
 * Stufe:   16
 * Gebiet:   Cerddor
 * Wirkung:
 *  Wiegelt 60% bis 90% der Bauern einer Region auf.  Bauern werden ein
 *  großer Mob, der zur Monsterpartei gehört und die Region bewacht.
 *  Regionssilber sollte auch nicht durch Unterhaltung gewonnen werden
 *  können.
 *
 *   Fehlt: Triggeraktion: löste Bauernmob auf und gib alles an Region,
 *   dann können die Bauernmobs ihr Silber mitnehmen und bleiben x
 *   Wochen bestehen
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
  int anteil;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  int duration = (int)force+1;
  faction * monsters = get_monsters();
  message * msg;

  anteil = 6 + (rng_int()%4);

  n = rpeasants(r) * anteil / 10;
  n = MAX(0, n);
  n = MIN(n, rpeasants(r));

  if (n <= 0) {
    report_failure(mage, co->order);
    return 0;
  }

  rsetpeasants(r, rpeasants(r) - n);
  assert(rpeasants(r) >= 0);

  u = create_unit(r, monsters, n, new_race[RC_PEASANT], 0, LOC(monsters->locale, "furious_mob"), NULL);
  fset(u, UFL_ISNEW);
  guard(u, GUARD_ALL);
  a = a_new(&at_unitdissolve);
  a->data.ca[0] = 1;  /* An rpeasants(r). */
  a->data.ca[1] = 15; /* 15% */
  a_add(&u->attribs, a);

  create_curse(mage, &r->attribs, ct_find("riotzone"), cast_level, duration, (double)anteil, 0);

  msg = msg_message("sp_raisepeasantmob_effect", "mage region", mage, r);
  report_spell(mage, r, msg);
  msg_release(msg);

  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Ritual der Aufnahme / Migrantenwerben
 * Stufe:   9
 * Gebiet:   Cerddor
 * Wirkung:
 *  Bis zu Stufe Personen fremder Rasse können angeworben werden. Die
 *  angeworbene Einheit muss kontaktieren. Keine teuren Talente
 *
 * Flag:
 *  (UNITSPELL | SPELLLEVEL | TESTCANSEE)
 */
static int
sp_migranten(castorder *co)
{
  unit *target;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  spellparameter *pa = co->par;

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (pa->param[0]->flag == TARGET_NOTFOUND) return 0;

  target = pa->param[0]->data.u; /* Zieleinheit */

  /* Personen unserer Rasse können problemlos normal übergeben werden */
  if (target->race == mage->faction->race) {
    /* u ist von unserer Art, das Ritual wäre verschwendete Aura. */
    ADDMSG(&mage->faction->msgs, msg_message(
      "sp_migranten_fail1", "unit region command target", mage,
      mage->region, co->order, target));
  }
  /* Auf eigene Einheiten versucht zu zaubern? Garantiert Tippfehler */
  if (target->faction == mage->faction) {
    cmistake(mage, co->order, 45, MSG_MAGIC);
  }

  /* Keine Monstereinheiten */
  if (!playerrace(target->race)) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, 
      "spellfail_nomonsters", ""));
    return 0;
  }
  /* niemand mit teurem Talent */
  if (has_limited_skills(target)) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, 
      "spellfail_noexpensives", "target", target));
    return 0;
  }
  /* maximal Stufe Personen */
  if (target->number > cast_level
    || target->number > max_spellpoints(r, mage))
  {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, 
      "spellfail_toomanytargets", ""));
    return 0;
  }

  /* Kontakt prüfen (aus alter Teleportroutine übernommen) */
  if (!ucontact(target, mage)) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, 
      "spellfail::contact", "target", target));
    return 0;
  }
  u_setfaction(target,mage->faction);
  set_order(&target->thisorder, NULL);

  /* Erfolg melden */
  ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "sp_migranten",
    "target", target));

  return target->number;
}

/* ------------------------------------------------------------- */
/* Name:     Gesang der Friedfertigkeit
 * Stufe:   12
 * Gebiet:   Cerddor
 * Wirkung:
 *   verhindert jede Attacke für lovar(Stufe/2) Runden
 */

static int
sp_song_of_peace(castorder *co)
{
  unit *u;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  int duration = 2 + lovar(force/2);
  message * msg[2] = { NULL, NULL };

  create_curse(mage, &r->attribs, ct_find("peacezone"), force, duration, zero_effect, 0);

  for (u = r->units; u; u = u->next) freset(u->faction, FFL_SELECT);
  for (u = r->units; u; u = u->next ) {
    if (!fval(u->faction, FFL_SELECT) ) {
      message * m = NULL;
      fset(u->faction, FFL_SELECT);
      if (cansee(u->faction, r, mage, 0)) {
        if (msg[0]==NULL) msg[0] = msg_message("song_of_peace_effect_0", "mage", mage);
        m = msg[0];
      } else {
        if (msg[1]==NULL) msg[1] = msg_message("song_of_peace_effect_1", "");
        m = msg[1];
      }
      r_addmessage(r, u->faction, m);
    }
  }
  if (msg[0]) msg_release(msg[0]);
  if (msg[1]) msg_release(msg[1]);
  return cast_level;

}

/* ------------------------------------------------------------- */
/* Name:     Hohes Lied der Gaukelei
 * Stufe:   2
 * Gebiet:   Cerddor
 * Wirkung:
 *   Das Unterhaltungsmaximum steigt von 20% auf 40% des
 *   Regionsvermögens. Der Spruch hält Stufe Wochen an
 */

static int
sp_generous(castorder *co)
{
  unit *u;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  int duration = (int)force+1;
  double effect;
  message * msg[2] = { NULL, NULL };

  if (is_cursed(r->attribs, C_DEPRESSION, 0)) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "spellfail_generous", ""));
    return 0;
  }

  effect = 2;
  create_curse(mage,&r->attribs, ct_find("generous"), force, duration, effect, 0);

  for (u = r->units; u; u = u->next) freset(u->faction, FFL_SELECT);
  for (u = r->units; u; u = u->next ) {
    if (!fval(u->faction, FFL_SELECT) ) {
      message * m = NULL;
      fset(u->faction, FFL_SELECT);
      if (cansee(u->faction, r, mage, 0)) {
        if (msg[0]==NULL) msg[0] = msg_message("generous_effect_0", "mage", mage);
        m = msg[0];
      } else {
        if (msg[1]==NULL) msg[1] = msg_message("generous_effect_1", "");
        m = msg[1];
      }
      r_addmessage(r, u->faction, m);
    }
  }
  if (msg[0]) msg_release(msg[0]);
  if (msg[1]) msg_release(msg[1]);
  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Anwerbung
 * Stufe:   4
 * Gebiet:   Cerddor
 * Wirkung:
 *   Bauern schliessen sich der eigenen Partei an
 *   ist zusätzlich zur Rekrutierungsmenge in der Region
 * */

static int
sp_recruit(castorder *co)
{
  unit *u;
  region *r = co->rt;
  int num, maxp = rpeasants(r);
  double n;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  faction *f = mage->faction;
  const struct race * rc = f->race;

  if (maxp == 0) {
    report_failure(mage, co->order);
    return 0;
  }
  /* Immer noch zuviel auf niedrigen Stufen. Deshalb die Rekrutierungskosten
   * mit einfliessen lassen und dafür den Exponenten etwas größer.
   * Wenn die Rekrutierungskosten deutlich höher sind als der Faktor,
   * ist das Verhältniss von ausgegebene Aura pro Bauer bei Stufe 2
   * ein mehrfaches von Stufe 1, denn in beiden Fällen gibt es nur 1
   * Bauer, nur die Kosten steigen. */
  n = (pow(force, 1.6) * 100)/f->race->recruitcost;
  if (rc->recruit_multi!=0) {
    double multp = maxp/rc->recruit_multi;
    n = MIN(multp, n);
    n = MAX(n, 1);
    rsetpeasants(r, maxp - (int)(n * rc->recruit_multi));
  } else {
    n = MIN(maxp, n);
    n = MAX(n, 1);
    rsetpeasants(r, maxp - (int)n);
  }

  num = (int)n;
  u = create_unit(r, f, num, f->race, 0, LOC(f->locale,(num == 1 ? "peasant" : "peasant_p")), mage);
  set_order(&u->thisorder, default_order(f->locale));

  ADDMSG(&mage->faction->msgs, msg_message("recruit_effect", "mage amount", mage, num));

  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:    Wanderprediger - Große Anwerbung
 * Stufe:   14
 * Gebiet:  Cerddor
 * Wirkung:
 *   Bauern schliessen sich der eigenen Partei an
 *   ist zusätzlich zur Rekrutierungsmenge in der Region
 * */

static int
sp_bigrecruit(castorder *co)
{
  unit *u;
  region *r = co->rt;
  int n, maxp = rpeasants(r);
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  faction *f = mage->faction;
  message * msg;

  if (maxp <= 0) {
    report_failure(mage, co->order);
    return 0;
  }
  /* Für vergleichbare Erfolge bei unterschiedlichen Rassen die
   * Rekrutierungskosten mit einfliessen lassen. */

  n = (int)force + lovar((force * force * 1000)/f->race->recruitcost);
  if (f->race==new_race[RC_URUK]) {
    n = MIN(2*maxp, n);
    n = MAX(n, 1);
    rsetpeasants(r, maxp - (n+1) / 2);
  } else {
    n = MIN(maxp, n);
    n = MAX(n, 1);
    rsetpeasants(r, maxp - n);
  }

  u = create_unit(r, f, n, f->race, 0, LOC(f->locale,(n == 1 ? "peasant" : "peasant_p")), mage);
  set_order(&u->thisorder, default_order(f->locale));

  msg = msg_message("recruit_effect", "mage amount", mage, n);
  r_addmessage(r, mage->faction, msg);
  msg_release(msg);

  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Aushorchen
 * Stufe:   7
 * Gebiet:   Cerddor
 * Wirkung:
 *  Erliegt die Einheit dem Zauber, so wird sie dem Magier alles
 *  erzählen, was sie über die gefragte Region weiß. Ist in der Region
 *  niemand ihrer Partei, so weiß sie nichts zu berichten.  Auch kann
 *  sie nur das erzählen, was sie selber sehen könnte.
 * Flags:
 *   (UNITSPELL | TESTCANSEE)
 */

/* restistenz der einheit prüfen */
static int
sp_pump(castorder *co)
{
  unit *u, *target;
  region *rt;
  boolean see = false;
  unit *mage = co->magician.u;
  spellparameter *pa = co->par;
  int cast_level = co->level;

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (pa->param[0]->flag == TARGET_NOTFOUND) return 0;

  target = pa->param[0]->data.u; /* Zieleinheit */

  if (fval(target->race, RCF_UNDEAD)) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "error_not_on_undead", ""));
    return 0;
  }
  if (is_magic_resistant(mage, target, 0) || is_monsters(target->faction)) {
    report_failure(mage, co->order);
    return 0;
  }

  rt  = pa->param[1]->data.r;

  for (u = rt->units; u; u = u->next) {
    if (u->faction == target->faction)
      see = true;
  }

  if (see == false) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "spellfail_pump", "target tregion", target, rt));
    return cast_level/2;
  } else {
    ADDMSG(&mage->faction->msgs, msg_message("pump_effect", "mage unit tregion", mage, target, rt));
  }

  u = create_unit(rt, mage->faction, RS_FARVISION, new_race[RC_SPELL], 0, "spell/pump", NULL);
  u->age = 2;
  set_level(u, SK_PERCEPTION, eff_skill(target, SK_PERCEPTION, u->region));

  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Verführung
 * Stufe:   6
 * Gebiet:   Cerddor
 * Wirkung:
 *  Betört eine Einheit, so das sie ihm den größten Teil ihres Bargelds
 *  und 50% ihres Besitzes schenkt. Sie behält jedoch immer soviel, wie
 *  sie zum überleben braucht. Wirkt gegen Magieresistenz.
 *  MIN(Stufe*1000$, u->money - maintenace)
 *  Von jedem Item wird 50% abgerundet ermittelt und übergeben. Dazu
 *  kommt Itemzahl%2 mit 50% chance
 *
 * Flags:
 * (UNITSPELL | TESTRESISTANCE | TESTCANSEE)
 */
static int
sp_seduce(castorder *co)
{
  item *items = NULL;
  unit *target;
  item **itmp;
  unit *mage = co->magician.u;
  spellparameter *pa = co->par;
  int cast_level = co->level;
  double force = co->force;

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (pa->param[0]->flag == TARGET_NOTFOUND) return 0;

  target = pa->param[0]->data.u; /* Zieleinheit */

  if (fval(target->race, RCF_UNDEAD)) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, 
      "spellfail_noundead", ""));
    return 0;
  }

  /* Erfolgsmeldung */

  itmp=&target->items;
  while (*itmp) {
    item * itm = *itmp;
    int loot;
    if (itm->type==i_silver) {
      loot = MIN(cast_level * 1000, get_money(target) - (maintenance_cost(target)));
      loot = MAX(loot, 0);
    } else {
      loot = itm->number/2;
      if (itm->number % 2) {
        loot += rng_int() % 2;
      }
      if (loot > 0) {
        loot = (int)MIN(loot, force * 5);
      }
    }
    if (loot>0) {
      i_change(&mage->items, itm->type, loot);
      i_change(&items, itm->type, loot);
      i_change(itmp, itm->type, -loot);
    }
    if (*itmp==itm) itmp=&itm->next;
  }

  ADDMSG(&mage->faction->msgs, msg_message("seduce_effect_0", "mage unit items", mage, target, items));
  i_freeall(&items);
  ADDMSG(&target->faction->msgs, msg_message("seduce_effect_1", "unit", target));

  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Monster friedlich stimmen
 * Stufe:   6
 * Gebiet:   Cerddor
 * Wirkung:
 *  verhindert Angriffe des bezauberten Monsters auf die Partei des
 *  Barden für Stufe Wochen. Nicht übertragbar, dh Verbündete werden vom
 *  Monster natürlich noch angegriffen. Wirkt nicht gegen Untote
 *  Jede Einheit kann maximal unter einem Beherrschungszauber dieser Art
 *  stehen, dh wird auf die selbe Einheit dieser Zauber von einem
 *  anderen Magier nochmal gezaubert, schlägt der Zauber fehl.
 *
 * Flags:
 * (UNITSPELL | ONSHIPCAST | TESTRESISTANCE | TESTCANSEE)
 */

static int
sp_calm_monster(castorder *co)
{
  curse * c;
  unit *target;
  unit *mage = co->magician.u;
  spellparameter *pa = co->par;
  int cast_level = co->level;
  double force = co->force;
  double effect;
  message * msg;

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (pa->param[0]->flag == TARGET_NOTFOUND) return 0;

  target = pa->param[0]->data.u; /* Zieleinheit */

  if (fval(target->race, RCF_UNDEAD)) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, 
      "spellfail_noundead", ""));
    return 0;
  }

  effect = mage->faction->subscription;
  c = create_curse(mage, &target->attribs, ct_find("calmmonster"), force,
    (int)force, effect, 0);
  if (c==NULL) {
    report_failure(mage, co->order);
    return 0;
  }

  msg = msg_message("calm_effect", "mage unit", mage, target);
  r_addmessage(mage->region, mage->faction, msg);
  msg_release(msg);
  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     schaler Wein
 * Stufe:   7
 * Gebiet:   Cerddor
 * Wirkung:
 *  wird gegen Magieresistenz gezaubert Das Opfer vergisst bis zu
 *  Talenttage seines höchsten Talentes und tut die Woche nix.
 *  Nachfolgende Zauber sind erschwert.
 *  Wirkt auf bis zu 10 Personen in der Einheit
 *
 * Flags:
 * (UNITSPELL | TESTRESISTANCE | TESTCANSEE)
 */

static int
sp_headache(castorder *co)
{
  skill * smax = NULL;
  int i;
  unit *target;
  unit *mage = co->magician.u;
  spellparameter *pa = co->par;
  int cast_level = co->level;
  message * msg;

  /* Macht alle nachfolgenden Zauber doppelt so teuer */
  countspells(mage, 1);

  target = pa->param[0]->data.u; /* Zieleinheit */

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (target->number==0 || pa->param[0]->flag == TARGET_NOTFOUND) return 0;

  /* finde das größte Talent: */
  for (i=0;i!=target->skill_size;++i) {
    skill * sv = target->skills+i;
    if (smax==NULL || skill_compare(sv, smax)>0) {
      smax = sv;
    }
  }
  if (smax!=NULL) {
    /* wirkt auf maximal 10 Personen */
    int change = MIN(10, target->number) * (rng_int()%2+1) / target->number;
    reduce_skill(target, smax, change);
  }
  set_order(&target->thisorder, NULL);

  msg = msg_message("hangover_effect_0", "mage unit", mage, target);
  r_addmessage(mage->region, mage->faction, msg);
  msg_release(msg);

  msg = msg_message("hangover_effect_1", "unit", target);
  r_addmessage(target->region, target->faction, msg);
  msg_release(msg);

  return cast_level;
}


/* ------------------------------------------------------------- */
/* Name:     Mob
 * Stufe:   10
 * Gebiet:   Cerddor
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
  unit *u2;
  attrib *a;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;
  message * msg;

  if (rpeasants(r) == 0) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "error_nopeasants", ""));
    return 0;
  }
  bauern = (int)MIN(rpeasants(r), power*250);
  rsetpeasants(r, rpeasants(r) - bauern);

  u2 = create_unit(r, mage->faction, bauern, new_race[RC_PEASANT], 0, LOC(mage->faction->locale, "furious_mob"), mage);

  fset(u2, UFL_LOCKED);
  if (rule_stealth_faction()) {
    fset(u2, UFL_ANON_FACTION);
  }

  a = a_new(&at_unitdissolve);
  a->data.ca[0] = 1;  /* An rpeasants(r). */
  a->data.ca[1] = 15; /* 15% */
  a_add(&u2->attribs, a);

  msg = msg_message("sp_raisepeasants_effect", "mage region amount", mage, r, u2->number);
  r_addmessage(r, NULL, msg);
  if (mage->region!=r) {
    add_message(&mage->faction->msgs, msg);
  }
  msg_release(msg);

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
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  int duration = (int)force+1;
  message * msg;

  create_curse(mage, &r->attribs, ct_find("depression"), force, duration, zero_effect, 0);

  msg = msg_message("sp_depression_effect", "mage region", mage, r);
  r_addmessage(r, NULL, msg);
  if (mage->region!=r) {
    add_message(&mage->faction->msgs, msg);
  }
  msg_release(msg);
  return cast_level;
}

#if 0
/* ------------------------------------------------------------- */
/* Name:       Hoher Gesang der Drachen
 * Stufe:      14
 * Gebiet:     Cerddor
 * Kategorie:  Monster, Beschwörung, positiv
 *
 * Wirkung:
 *  Erhöht HP-Regeneration in der Region und lockt drachenartige (Wyrm,
 *  Drache, Jungdrache, Seeschlange, ...) aus der Umgebung an
 *
 * Flag:
 *  (FARCASTING | REGIONSPELL | TESTRESISTANCE)
 */
/* TODO zur Aktivierung in Zauberliste aufnehmen*/
static int
sp_dragonsong(castorder *co)
{
  region *r = co->rt; /* Zauberregion */
  unit *mage = co->magician.u;
  unit *u;
  int cast_level = co->level;
  double power = co->force;
  region_list *rl,*rl2;
  faction *f;

  /* TODO HP-Effekt */

  f = get_monsters();

  rl = all_in_range(r, (int)power);

  for(rl2 = rl; rl2; rl2 = rl2->next) {
    region * r2 = rl2->data;
    for(u = r2->units; u; u = u->next) {
      if (u->race->flags & RCF_DRAGON) {
        attrib * a = a_find(u->attribs, &at_targetregion);
        if (!a) {
          a = a_add(&u->attribs, make_targetregion(r));
        } else {
          a->data.v = r;
        }
      }
    }
  }

  ADDMSG(&mage->faction->msgs, msg_message(
        "summondragon", "unit region command target",
        mage, mage->region, co->order, co->rt));

  free_regionlist(rl);
  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Hoher Gesang der Verlockung
 * Stufe:      17
 * Gebiet:     Cerddor
 * Kategorie:  Monster, Beschwörung, positiv
 *
 * Wirkung:
 *  Lockt Bauern aus den umliegenden Regionen her
 *
 * Flag:
 *  (FARCASTING | REGIONSPELL | TESTRESISTANCE)
 */
/* TODO zur Aktivierung in Zauberliste aufnehmen*/

static int
sp_songofAttraction(castorder *co)
{
  region *r = co->rt; /* Zauberregion */
  unit *mage = co->magician.u;
  int cast_level = co->level;
  /* double power = co->force; */

  /* TODO Wander Effekt */

  ADDMSG(&mage->faction->msgs, msg_message(
        "summon", "unit region command region",
        mage, mage->region, co->order, r));

  return cast_level;
}

#endif

/* ------------------------------------------------------------- */
/* TRAUM - Illaun */
/* ------------------------------------------------------------- */

/* Name:       Seelenfrieden
 * Stufe:      2
 * Kategorie:  Region, positiv
 * Gebiet:     Illaun
 * Wirkung:
 *   Reduziert Untotencounter
 * Flag: (0)
 */

int
sp_puttorest(castorder *co)
{
  region *r = co->rt;
  unit *mage = co->magician.u;
  int dead = deathcount(r);
  int laid_to_rest = dice((int)(co->force * 2), 100);
  message * seen = msg_message("puttorest", "mage", mage);
  message * unseen = msg_message("puttorest", "mage", NULL);

  laid_to_rest = MAX(laid_to_rest, dead);

  deathcounts(r, -laid_to_rest);

  report_effect(r, mage, seen, unseen);
  msg_release(seen);
  msg_release(unseen);
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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;
  spellparameter *pa = co->par;
  icastle_data * data;
  const char * bname;
  message * msg;
  static const building_type * bt_illusion;

  if (bt_illusion==NULL) bt_illusion = bt_find("illusioncastle");
  if (bt_illusion==NULL) {
    return 0;
  }

  if ((type=findbuildingtype(pa->param[0]->data.xs, mage->faction->locale)) == NULL) {
    type = bt_find("castle");
  }

  b = new_building(bt_illusion, r, mage->faction->locale);

  /* Größe festlegen. */
  if (type == bt_illusion) {
    b->size = (rng_int()%(int)((power*power)+1)*10);
  } else if (type->maxsize == -1) {
    b->size = ((rng_int()%(int)(power))+1)*5;
  } else {
    b->size = type->maxsize;
  }

  if (type->name==NULL) {
    bname = LOC(mage->faction->locale, type->_name);
  } else {
    bname = LOC(mage->faction->locale, buildingtype(type, b, 0));
  }
  building_setname(b, bname);

  /* TODO: Auf timeout und action_destroy umstellen */
  a = a_add(&b->attribs, a_new(&at_icastle));
  data = (icastle_data*)a->data.v;
  data->type = type;
  data->building = b;
  data->time = 2+(rng_int()%(int)(power)+1)*(rng_int()%(int)(power)+1);

  if (mage->region == r) {
    if (leave(mage, false)) {
      mage->building = b;
    }
  }

  ADDMSG(&mage->faction->msgs, msg_message(
    "icastle_create", "unit region command", mage, mage->region,
    co->order));

  msg = msg_message("sp_icastle_effect", "region", r);
  report_spell(mage, r, msg);
  msg_release(msg);

  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:    Gestaltwandlung
 * Stufe:   3
 * Gebiet:  Illaun
 * Wirkung:
 *  Zieleinheit erscheint für (Stufe) Wochen als eine andere Gestalt
 *  (wie bei dämonischer Rassetarnung).
 * Syntax: ZAUBERE "Gestaltwandlung"  <einheit>     <rasse>
 * Flags:
 *  (UNITSPELL | SPELLLEVEL)
 */

int
sp_illusionary_shapeshift(castorder *co)
{
  unit *u;
  const race * rc;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;
  spellparameter *pa = co->par;
  const race * irace;

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (pa->param[0]->flag == TARGET_NOTFOUND) return 0;

  /* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
   * abbrechen aber kosten lassen */
  if (pa->param[0]->flag == TARGET_RESISTS) return cast_level;

  u = pa->param[0]->data.u;

  rc = findrace(pa->param[1]->data.xs, mage->faction->locale);
  if (rc == NULL) {
    cmistake(mage, co->order, 202, MSG_MAGIC);
    return 0;
  }

  /* ähnlich wie in laws.c:setealth() */
  if (!playerrace(rc)) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "sp_shapeshift_fail", "target race", u, rc));
    return 0;
  }
  irace = u_irace(u);
  if (irace == u->race) {
    trigger * trestore = trigger_changerace(u, NULL, irace);
    add_trigger(&u->attribs, "timer", trigger_timeout((int)power+2, trestore));
    u->irace = rc;
  }

  ADDMSG(&mage->faction->msgs, msg_message("shapeshift_effect", "mage target race", mage, u, rc));

  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Regionstraum analysieren
 * Stufe:   9
 * Aura:     18
 * Kosten:   SPC_FIX
 * Wirkung:
 *  Zeigt die Verzauberungen eines Objekts an (curse->name,
 *  curse::info). Aus der Differenz Spruchstärke und Curse->vigour
 *  ergibt sich die Chance den Spruch zu identifizieren ((force -
 *  c->vigour)*10 + 100 %).
 */
int
sp_analyseregionsdream(castorder *co)
{
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;

  magicanalyse_region(r, mage, cast_level);

  return cast_level;
}


/* ------------------------------------------------------------- */
/* Name:     Traumbilder erkennen
 * Stufe:   5
 * Aura:     12
 * Kosten:   SPC_FIX
 * Wirkung:
 *  Zeigt die Verzauberungen eines Objekts an (curse->name,
 *  curse::info). Aus der Differenz Spruchstärke und Curse->vigour
 *  ergibt sich die Chance den Spruch zu identifizieren ((force -
 *  c->vigour)*10 + 100 %).
 */
int
sp_analysedream(castorder *co)
{
  unit *u;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  spellparameter *pa = co->par;

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (pa->param[0]->flag == TARGET_NOTFOUND) return 0;

  /* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
   * abbrechen aber kosten lassen */
  if (pa->param[0]->flag == TARGET_RESISTS) return cast_level;

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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;
  region *r = co->rt;
  curse * c;
  double effect;

  /* wirkt erst in der Folgerunde, soll mindestens eine Runde wirken,
   * also duration+2 */
  duration = (int)MAX(1, power/2); /* Stufe 1 macht sonst mist */
  duration = 2 + rng_int()%duration;

  /* Nichts machen als ein entsprechendes Attribut in die Region legen. */
  effect = -1;
  c = create_curse(mage, &r->attribs, ct_find("gbdream"), power, duration, effect, 0);

  /* Erfolg melden*/
  ADDMSG(&mage->faction->msgs, msg_message(
        "regionmagic_effect", "unit region command", mage,
        mage->region, co->order));

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
  curse * c;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;
  double effect;

  /* wirkt erst in der Folgerunde, soll mindestens eine Runde wirken,
   * also duration+2 */
  duration = (int)MAX(1, power/2); /* Stufe 1 macht sonst mist */
  duration = 2 + rng_int()%duration;
  effect = 1;
  c = create_curse(mage, &r->attribs, ct_find("gbdream"), power, duration, effect, 0);

  /* Erfolg melden*/
  ADDMSG(&mage->faction->msgs, msg_message(
        "regionmagic_effect", "unit region command", mage,
        mage->region, co->order));

  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:
 * Stufe:      9
 * Kategorie:
 * Wirkung:
 *   Es wird eine Kloneinheit erzeugt, die nichts kann. Stirbt der
 *   Magier, wird er mit einer Wahrscheinlichkeit von 90% in den Klon
 *   transferiert.
 * Flags:
 * (NOTFAMILARCAST)
 */
int
sp_clonecopy(castorder *co)
{
  unit *clone;
  region *r = co->rt;
  region *target_region = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  message * msg;
  char name[NAMESIZE];

  if (get_clone(mage) != NULL ) {
    cmistake(mage, co->order, 298, MSG_MAGIC);
    return 0;
  }

  snprintf(name, sizeof(name), (const char*)LOC(mage->faction->locale, "clone_of"), unitname(mage));
  clone = create_unit(target_region, mage->faction, 1, new_race[RC_CLONE], 0, name, mage);
  setstatus(clone, ST_FLEE);
  fset(clone, UFL_LOCKED);

  create_newclone(mage, clone);

  msg = msg_message("sp_clone_effet", "mage", mage);
  r_addmessage(r, mage->faction, msg);
  msg_release(msg);

  return cast_level;
}

/* ------------------------------------------------------------- */
int
sp_dreamreading(castorder *co)
{
  unit *u,*u2;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  spellparameter *pa = co->par;
  double power = co->force;
  message * msg;

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (pa->param[0]->flag == TARGET_NOTFOUND) return 0;

  /* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
   * abbrechen aber kosten lassen */
  if (pa->param[0]->flag == TARGET_RESISTS) return cast_level;

  u = pa->param[0]->data.u;

  /* Illusionen und Untote abfangen. */
  if (fval(u->race, RCF_UNDEAD|RCF_ILLUSIONARY)) {
    ADDMSG(&mage->faction->msgs, msg_unitnotfound(mage, co->order, pa->param[0]));
    return 0;
  }

  /* Entfernung */
  if (distance(mage->region, u->region) > power) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "spellfail_distance", ""));
    return 0;
  }

  u2 = create_unit(u->region,mage->faction, RS_FARVISION, new_race[RC_SPELL], 0, "spell/dreamreading", NULL);
  set_number(u2, 1);
  u2->age = 2;   /* Nur für diese Runde. */
  set_level(u2, SK_PERCEPTION, eff_skill(u, SK_PERCEPTION, u2->region));

  msg = msg_message("sp_dreamreading_effect", "mage unit region", mage, u, u->region);
  r_addmessage(r, mage->faction, msg);
  msg_release(msg);
  return cast_level;
}

/* ------------------------------------------------------------- */
/* Wirkt power/2 Runden auf bis zu power^2 Personen
 * mit einer Chance von 5% vermehren sie sich  */
int
sp_sweetdreams(castorder *co)
{
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;
  spellparameter *pa = co->par;
  int men, n;
  int duration = (int)(power/2)+1;
  int opfer = (int)(power*power);

  /* Schleife über alle angegebenen Einheiten */
  for (n = 0; n < pa->length; n++) {
    curse * c;
    unit *u;
    double effect;
    message * msg;
    /* sollte nie negativ werden */
    if (opfer < 1) break;

    if (pa->param[n]->flag == TARGET_RESISTS ||
      pa->param[n]->flag == TARGET_NOTFOUND)
      continue;

    /* Zieleinheit */
    u = pa->param[n]->data.u;

    if (!ucontact(u, mage)) {
      cmistake(mage, co->order, 40, MSG_EVENT);
      continue;
    }
    men = MIN(opfer, u->number);
    opfer -= men;

    /* Nichts machen als ein entsprechendes Attribut an die Einheit legen. */
    effect = 0.05;
    c = create_curse(mage,&u->attribs, ct_find("orcish"), power, duration, effect, men);

    msg = msg_message("sp_sweetdreams_effect", "mage unit region", mage, u, r);
    r_addmessage(r, mage->faction, msg);
    if (u->faction!=mage->faction) {
      r_addmessage(r, u->faction, msg);
    }
    msg_release(msg);
  }
  return cast_level;
}

/* ------------------------------------------------------------- */
int
sp_disturbingdreams(castorder *co)
{
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;
  int duration = 1 + (int)(power/6);
  double effect;
  curse * c;

  effect = 10;
  c = create_curse(mage, &r->attribs, ct_find("badlearn"), power, duration, effect, 0);

  ADDMSG(&mage->faction->msgs, msg_message("sp_disturbingdreams_effect", "mage region", mage, r));
  return cast_level;
}

/* ------------------------------------------------------------- */
/* ASTRAL / THEORIE / M_TYBIED */
/* ------------------------------------------------------------- */
/* Name:     Magie analysieren
 * Stufe:   1
 * Aura:     1
 * Kosten:   SPC_LINEAR
 * Komponenten:
 *
 * Wirkung:
 *  Zeigt die Verzauberungen eines Objekts an (curse->name,
 *  curse::info). Aus der Differenz Spruchstärke und Curse->vigour
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
  unit *mage = co->magician.u;
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
    case SPP_TEMP:
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
  unit *mage = co->magician.u;
  spellparameter *pa = co->par;
  int cast_level = co->level;
  double power = co->force;
  int duration = (int)MAX(2.0, power+1); /* works in the report, and ageing this round would kill it if it's <=1 */

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (pa->param[0]->flag == TARGET_NOTFOUND) return 0;

  /* Zieleinheit */
  target = pa->param[0]->data.u;

  create_curse(mage,&target->attribs, ct_find("itemcloak"), power, duration, zero_effect, 0);
  ADDMSG(&mage->faction->msgs, msg_message(
    "itemcloak", "mage target", mage, target));

  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Magieresistenz erhöhen
 * Stufe:   3
 * Aura:     5 MP
 * Kosten:   SPC_LEVEL
 * Komponenten:
 *
 * Wirkung:
 *   erhöht die Magierestistenz der Personen um 20 Punkte für 6 Wochen
 *   Wirkt auf Stufe*5 Personen kann auf mehrere Einheiten gezaubert
 *   werden, bis die Zahl der möglichen Personen erschöpft ist
 *
 * Flags:
 *   UNITSPELL
 */
int
sp_resist_magic_bonus(castorder *co)
{
  unit *u;
  int n, m;
  int duration = 6;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;
  spellparameter *pa = co->par;
  /* Pro Stufe können bis zu 5 Personen verzaubert werden */
  double maxvictims = power * 5;
  int victims = (int)maxvictims;

  /* Schleife über alle angegebenen Einheiten */
  for (n = 0; n < pa->length; n++) {
    message * msg;
    /* sollte nie negativ werden */
    if (victims < 1)
      break;

    if (pa->param[n]->flag == TARGET_RESISTS
        || pa->param[n]->flag == TARGET_NOTFOUND)
      continue;

    u = pa->param[n]->data.u;

    /* Ist die Einheit schon verzaubert, wirkt sich dies nur auf die
     * Menge der Verzauberten Personen aus.
    if (is_cursed(u->attribs, C_MAGICRESISTANCE, 0))
      continue;
    */

    m = MIN(u->number,victims);
    victims -= m;

    create_curse(mage, &u->attribs, ct_find("magicresistance"),
      power, duration, 20, m);

    msg = msg_message("magicresistance_effect", "unit", u);
    add_message(&u->faction->msgs, msg);

    /* und noch einmal dem Magier melden */
    if (u->faction != mage->faction) {
      add_message(&mage->faction->msgs, msg);
    }
    msg_release(msg);
  }

  cast_level = MIN(cast_level, (int)(cast_level*(victims+4)/maxvictims));
  return MAX(cast_level, 1);
}

/** spell 'Astraler Weg'.
 * Syntax "ZAUBERE [STUFE n] 'Astraler Weg' <Einheit-Nr> [<Einheit-Nr> ...]",
 *
 * Parameter:
 * pa->param[0]->data.xs
*/
int
sp_enterastral(castorder *co)
{
  region *rt, *ro;
  unit *u, *u2;
  int remaining_cap;
  int n, w;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;
  spellparameter *pa = co->par;

  switch(getplaneid(r)) {
  case 0:
    rt = r_standard_to_astral(r);
    ro = r;
    break;
  default:
    cmistake(mage, co->order, 190, MSG_MAGIC);
    return 0;
  }

  if (!rt || fval(rt->terrain, FORBIDDEN_REGION)) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "feedback_no_astralregion", ""));
    return 0;
  }

  if (is_cursed(rt->attribs, C_ASTRALBLOCK, 0) || is_cursed(ro->attribs, C_ASTRALBLOCK, 0)) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "spellfail_astralblock", ""));
    return 0;
  }

  remaining_cap = (int)((power-3) * 1500);

  /* für jede Einheit in der Kommandozeile */
  for (n = 0; n < pa->length; n++) {
    if (pa->param[n]->flag == TARGET_NOTFOUND) continue;
    u = pa->param[n]->data.u;

    if (!ucontact(u, mage)) {
      if (power > 10 && !is_magic_resistant(mage, u, 0) && can_survive(u, rt)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "feedback_no_contact_no_resist", "target", u));
        ADDMSG(&u->faction->msgs, msg_message("send_astral", "unit target", mage, u));
      } else {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "feedback_no_contact_resist", "target", u));
        ADDMSG(&u->faction->msgs, msg_message("try_astral", "unit target", mage, u));
        continue;
      }
    }

    w = weight(u);
    if (!can_survive(u, rt)) {
      cmistake(mage, co->order, 231, MSG_MAGIC);
    } else if (remaining_cap - w < 0) {
      ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "fail_tooheavy", "target", u));
    } else {
      message * m;
      remaining_cap = remaining_cap - w;
      move_unit(u, rt, NULL);

      /* Meldungen in der Ausgangsregion */

      for (u2 = r->units; u2; u2 = u2->next) freset(u2->faction, FFL_SELECT);
      m = NULL;
      for (u2 = r->units; u2; u2 = u2->next ) {
        if (!fval(u2->faction, FFL_SELECT)) {
          if (cansee(u2->faction, r, u, 0)) {
            fset(u2->faction, FFL_SELECT);
            if (!m) m = msg_message("astral_disappear", "unit", u);
            r_addmessage(r, u2->faction, m);
          }
        }
      }
      if (m) msg_release(m);

      /* Meldungen in der Zielregion */

      for (u2 = rt->units; u2; u2 = u2->next) freset(u2->faction, FFL_SELECT);
      m = NULL;
      for (u2 = rt->units; u2; u2 = u2->next ) {
        if (!fval(u2->faction, FFL_SELECT)) {
          if (cansee(u2->faction, rt, u, 0)) {
            fset(u2->faction, FFL_SELECT);
            if (!m) m = msg_message("astral_appear", "unit", u);
            r_addmessage(rt, u2->faction, m);
          }
        }
      }
      if (m) msg_release(m);
    }
  }
  return cast_level;
}

/** Spell 'Astraler Ruf' / 'Astral Call'.
 */
int
sp_pullastral(castorder *co)
{
  region *rt, *ro;
  unit *u, *u2;
  region_list *rl, *rl2;
  int remaining_cap;
  int n, w;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;
  spellparameter *pa = co->par;

  switch (getplaneid(r)) {
    case 1:
      rt = r;
      ro = pa->param[0]->data.r;
      rl = astralregions(r, NULL);
      rl2 = rl;
      while (rl2!=NULL) {
        region * r2 = rl2->data;
        if (r2->x == ro->x && r2->y == ro->y) {
          ro = r2;
          break;
        }
        rl2 = rl2->next;
      }
      if (!rl2) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "spellfail::nocontact",
          "target", rt));
        free_regionlist(rl);
        return 0;
      }
      free_regionlist(rl);
      break;
    default:
      ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "spellfail_astralonly", ""));
      return 0;
  }

  if (is_cursed(rt->attribs, C_ASTRALBLOCK, 0) || is_cursed(ro->attribs, C_ASTRALBLOCK, 0)) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "spellfail_astralblock", ""));
    return 0;
  }

  remaining_cap = (int)((power-3) * 1500);

  /* für jede Einheit in der Kommandozeile */
  for (n = 1; n < pa->length; n++) {
    spllprm * spobj = pa->param[n];
    if (spobj->flag == TARGET_NOTFOUND) continue;

    u = spobj->data.u;

    if (u->region!=ro) {
      /* Report this as unit not found */
      if (spobj->typ == SPP_UNIT) {
        spobj->data.i = u->no;
      } else {
        spobj->data.i = ualias(u);
      }
      spobj->flag = TARGET_NOTFOUND;
      ADDMSG(&mage->faction->msgs, msg_unitnotfound(mage, co->order, spobj));
      return false;
    }

    if (!ucontact(u, mage)) {
      if (power > 12 && spobj->flag != TARGET_RESISTS && can_survive(u, rt)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "feedback_no_contact_no_resist", "target", u));
      } else {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "feedback_no_contact_resist", "target", u));
        ADDMSG(&u->faction->msgs, msg_message("try_astral", "unit target", mage, u));
        continue;
      }
    }

    w = weight(u);

    if (!can_survive(u, rt)) {
      cmistake(mage, co->order, 231, MSG_MAGIC);
    } else if (remaining_cap - w < 0) {
      ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "fail_tooheavy", "target", u));
    } else {
      message * m;

      ADDMSG(&u->faction->msgs, msg_message("send_astral", "unit target", mage, u));
      remaining_cap = remaining_cap - w;
      move_unit(u, rt, NULL);

      /* Meldungen in der Ausgangsregion */

      for (u2 = r->units; u2; u2 = u2->next) freset(u2->faction, FFL_SELECT);
      m = NULL;
      for (u2 = r->units; u2; u2 = u2->next ) {
        if (!fval(u2->faction, FFL_SELECT)) {
          if (cansee(u2->faction, r, u, 0)) {
            fset(u2->faction, FFL_SELECT);
            if (!m) m = msg_message("astral_disappear", "unit", u);
            r_addmessage(r, u2->faction, m);
          }
        }
      }
      if (m) msg_release(m);

      /* Meldungen in der Zielregion */

      for (u2 = rt->units; u2; u2 = u2->next) freset(u2->faction, FFL_SELECT);
      m = NULL;
      for (u2 = rt->units; u2; u2 = u2->next ) {
        if (!fval(u2->faction, FFL_SELECT)) {
          if (cansee(u2->faction, rt, u, 0)) {
            fset(u2->faction, FFL_SELECT);
            if (!m) m = msg_message("astral_appear", "unit", u);
            r_addmessage(rt, u2->faction, m);
          }
        }
      }
      if (m) msg_release(m);
    }
  }
  return cast_level;
}

int
sp_leaveastral(castorder *co)
{
  region *rt, *ro;
  region_list *rl, *rl2;
  unit *u, *u2;
  int remaining_cap;
  int n, w;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;
  spellparameter *pa = co->par;

  switch(getplaneid(r)) {
  case 1:
    ro = r;
    rt = pa->param[0]->data.r;
    if (!rt) {
      ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "spellfail::noway", ""));
      return 0;
    }
    rl  = astralregions(r, inhabitable);
    rl2 = rl;
    while (rl2!=NULL) {
      if (rl2->data == rt) break;
      rl2 = rl2->next;
    }
    if (rl2==NULL) {
      ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "spellfail::noway", ""));
      free_regionlist(rl);
      return 0;
    }
    free_regionlist(rl);
    break;
  default:
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "spell_astral_only", ""));
    return 0;
  }

  if (ro==NULL || is_cursed(ro->attribs, C_ASTRALBLOCK, 0) || is_cursed(rt->attribs, C_ASTRALBLOCK, 0)) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "spellfail_astralblock", ""));
    return 0;
  }

  remaining_cap = (int)((power-3) * 1500);

  /* für jede Einheit in der Kommandozeile */
  for (n = 1; n < pa->length; n++) {
    if (pa->param[n]->flag == TARGET_NOTFOUND) continue;

    u = pa->param[n]->data.u;

    if (!ucontact(u, mage)) {
      if (power > 10 && !pa->param[n]->flag == TARGET_RESISTS && can_survive(u, rt)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "feedback_no_contact_no_resist", "target", u));
        ADDMSG(&u->faction->msgs, msg_message("send_astral", "unit target", mage, u));
      } else {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "feedback_no_contact_resist", "target", u));
        ADDMSG(&u->faction->msgs, msg_message("try_astral", "unit target", mage, u));
        continue;
      }
    }

    w = weight(u);

    if (!can_survive(u, rt)) {
      cmistake(mage, co->order, 231, MSG_MAGIC);
    } else if (remaining_cap - w < 0) {
      ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "fail_tooheavy", "target", u));
    } else {
      message * m;

      remaining_cap = remaining_cap - w;
      move_unit(u, rt, NULL);

      /* Meldungen in der Ausgangsregion */

      for (u2 = r->units; u2; u2 = u2->next) freset(u2->faction, FFL_SELECT);
      m = NULL;
      for (u2 = r->units; u2; u2 = u2->next ) {
        if (!fval(u2->faction, FFL_SELECT)) {
          if (cansee(u2->faction, r, u, 0)) {
            fset(u2->faction, FFL_SELECT);
            if (!m) m = msg_message("astral_disappear", "unit", u);
            r_addmessage(r, u2->faction, m);
          }
        }
      }
      if (m) msg_release(m);

      /* Meldungen in der Zielregion */

      for (u2 = rt->units; u2; u2 = u2->next) freset(u2->faction, FFL_SELECT);
      m = NULL;
      for (u2 = rt->units; u2; u2 = u2->next ) {
        if (!fval(u2->faction, FFL_SELECT)) {
          if (cansee(u2->faction, rt, u, 0)) {
            fset(u2->faction, FFL_SELECT);
            if (!m) m = msg_message("astral_appear", "unit", u);
            r_addmessage(rt, u2->faction, m);
          }
        }
      }
      if (m) msg_release(m);
    }
  }
  return cast_level;
}

int
sp_fetchastral(castorder *co)
{
  int n;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  spellparameter *pa = co->par;
  double power = co->force;
  int remaining_cap = (int)((power-3) * 1500);
  region_list * rtl = NULL;
  region * rt = co->rt; /* region to which we are fetching */
  region * ro = NULL; /* region in which the target is */

  if (rplane(rt)!=get_normalplane()) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "error190", ""));
    return 0;
  }

  /* für jede Einheit in der Kommandozeile */
  for (n=0; n!=pa->length; ++n) {
    unit * u2, * u = pa->param[n]->data.u;
    int w;
    message * m;

    if (pa->param[n]->flag & TARGET_NOTFOUND) continue;

    if (u->region!=ro) {
      /* this can happen several times if the units are from different astral
       * regions. Only possible on the intersections of schemes */
      region_list * rfind;
      if (!is_astral(u->region)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "spellfail_astralonly", ""));
        continue;
      }
      if (rtl!=NULL) free_regionlist(rtl);
      rtl = astralregions(u->region, NULL);
      for (rfind=rtl;rfind!=NULL;rfind=rfind->next) {
        if (rfind->data==mage->region) break;
      }
      if (rfind==NULL) {
        /* the region r is not in the schemes of rt */
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
          "spellfail_distance", "target", u));
        continue;
      }
      ro = u->region;
    }

    if (is_cursed(ro->attribs, C_ASTRALBLOCK, 0)) {
      ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
        "spellfail_astralblock", ""));
      continue;
    }

    if (!can_survive(u, rt)) {
      cmistake(mage, co->order, 231, MSG_MAGIC);
      continue;
    }

    w = weight(u);
    if (remaining_cap - w < 0) {
      ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
        "fail_tooheavy", "target", u));
      continue;
    }

    if (!ucontact(u, mage)) {
      if (power>12 && !(pa->param[n]->flag & TARGET_RESISTS)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "feedback_no_contact_no_resist", "target", u));
        ADDMSG(&u->faction->msgs, msg_message("send_astral", "unit target", mage, u));
      } else {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "feedback_no_contact_resist", "target", u));
        ADDMSG(&u->faction->msgs, msg_message("try_astral", "unit target", mage, u));
        continue;
      }
    }

    remaining_cap -= w;
    move_unit(u, rt, NULL);

    /* Meldungen in der Ausgangsregion */
    for (u2 = ro->units; u2; u2 = u2->next) freset(u2->faction, FFL_SELECT);
    m = NULL;
    for (u2 = ro->units; u2; u2 = u2->next ) {
      if (!fval(u2->faction, FFL_SELECT)) {
        if (cansee(u2->faction, ro, u, 0)) {
          fset(u2->faction, FFL_SELECT);
          if (!m) m = msg_message("astral_disappear", "unit", u);
          r_addmessage(ro, u2->faction, m);
        }
      }
    }
    if (m) msg_release(m);

    /* Meldungen in der Zielregion */
    for (u2 = rt->units; u2; u2 = u2->next) freset(u2->faction, FFL_SELECT);
    m = NULL;
    for (u2 = rt->units; u2; u2 = u2->next ) {
      if (!fval(u2->faction, FFL_SELECT)) {
        if (cansee(u2->faction, rt, u, 0)) {
          fset(u2->faction, FFL_SELECT);
          if (!m) m = msg_message("astral_appear", "unit", u);
          r_addmessage(rt, u2->faction, m);
        }
      }
    }
    if (m) msg_release(m);
  }
  if (rtl!=NULL) free_regionlist(rtl);
  return cast_level;
}

#ifdef SHOWASTRAL_NOT_BORKED
int
sp_showastral(castorder *co)
{
  unit *u;
  region *rt;
  int n = 0;
  int c = 0;
  region_list *rl, *rl2;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;

  switch(getplaneid(r)) {
  case 0:
    rt = r_standard_to_astral(r);
    if (!rt || fval(rt->terrain, FORBIDDEN_REGION)) {
      /* Hier gibt es keine Verbindung zur astralen Welt */
      cmistake(mage, co->order, 216, MSG_MAGIC);
      return 0;
    }
    break;
  case 1:
    rt = r;
    break;
  default:
    /* Hier gibt es keine Verbindung zur astralen Welt */
    cmistake(mage, co->order, 216, MSG_MAGIC);
    return 0;
  }

  rl = all_in_range(rt,power/5);

  /* Erst Einheiten zählen, für die Grammatik. */

  for (rl2=rl; rl2; rl2=rl2->next) {
    region * r2 = rl2->data;
    if (!is_cursed(r2->attribs, C_ASTRALBLOCK, 0)) {
      for(u = r2->units; u; u=u->next) {
        if (u->race != new_race[RC_SPECIAL] && u->race != new_race[RC_SPELL]) n++;
      }
    }
  }

  if (n == 0) {
    /* sprintf(buf, "%s kann niemanden im astralen Nebel entdecken.",
      unitname(mage)); */
    cmistake(mage, co->order, 220, MSG_MAGIC);
  } else {

    /* Ausgeben */

    sprintf(buf, "%s hat eine Vision der astralen Ebene. Im astralen "
      "Nebel zu erkennen sind ", unitname(mage));

    for(rl2=rl; rl2; rl2=rl2->next) {
      if (!is_cursed(rl2->data->attribs, C_ASTRALBLOCK, 0)) {
        for(u = rl2->data->units; u; u=u->next) {
          if (u->race != new_race[RC_SPECIAL] && u->race != new_race[RC_SPELL]) {
            c++;
            scat(unitname(u));
            scat(" (");
            if (!fval(u, UFL_ANON_FACTION)) {
              scat(factionname(u->faction));
              scat(", ");
            }
            icat(u->number);
            scat(" ");
            scat(LOC(mage->faction->locale, rc_name(u->race, u->number!=1)));
            scat(", Entfernung ");
            icat(distance(rl2->data, rt));
            scat(")");
            if (c == n-1) {
              scat(" und ");
            } else if (c < n-1) {
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
  unused(co);
  return 0;
}
#endif

/* ------------------------------------------------------------- */
int
sp_viewreality(castorder *co)
{
  region_list *rl, *rl2;
  unit *u;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  message * m;

  if (getplaneid(r) != 1) {
    /* sprintf(buf, "Dieser Zauber kann nur im Astralraum gezaubert werden."); */
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "spell_astral_only", ""));
    return 0;
  }

  rl = astralregions(r, NULL);

  /* Irgendwann mal auf Curses u/o Attribut umstellen. */
  for (rl2=rl; rl2; rl2=rl2->next) {
    region * rt = rl2->data;
    if (!is_cursed(rt->attribs, C_ASTRALBLOCK, 0)) {
      u = create_unit(rt, mage->faction, RS_FARVISION, new_race[RC_SPELL], 0, "spell/viewreality", NULL);
      set_level(u, SK_PERCEPTION, co->level/2);
      u->age = 2;
    }
  }

  free_regionlist(rl);

  m = msg_message("viewreality_effect", "unit", mage);
  r_addmessage(r, mage->faction, m);
  msg_release(m);

  return cast_level;
}

/* ------------------------------------------------------------- */
int
sp_disruptastral(castorder *co)
{
  region_list *rl, *rl2;
  region *rt;
  unit *u;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;
  int duration = (int)(power/3)+1;

  switch(getplaneid(r)) {
  case 0:
    rt = r_standard_to_astral(r);
    if (!rt || fval(rt->terrain, FORBIDDEN_REGION)) {
      /* "Hier gibt es keine Verbindung zur astralen Welt." */
      cmistake(mage, co->order, 216, MSG_MAGIC);
      return 0;
    }
    break;
  case 1:
    rt = r;
    break;
  default:
    /* "Von hier aus kann man die astrale Ebene nicht erreichen." */
    cmistake(mage, co->order, 215, MSG_MAGIC);
    return 0;
  }

  rl = all_in_range(rt, (short)(power/5), NULL);

  for (rl2=rl; rl2!=NULL; rl2=rl2->next) {
    attrib *a;
    double effect;
    region * r2 = rl2->data;
    spec_direction *sd;
    int inhab_regions = 0;
    region_list * trl = NULL;

    if (is_cursed(r2->attribs, C_ASTRALBLOCK, 0)) continue;

    if (r2->units!=NULL) {
      region_list * trl2;

      trl = astralregions(rl2->data, inhabitable);
      for (trl2 = trl; trl2; trl2 = trl2->next) ++inhab_regions;
    }

    /* Nicht-Permanente Tore zerstören */
    a = a_find(r->attribs, &at_direction);

    while (a!=NULL && a->type==&at_direction) {
      attrib * a2 = a->next;
      sd = (spec_direction *)(a->data.v);
      if (sd->duration != -1) a_remove(&r->attribs, a);
      a = a2;
    }

    /* Einheiten auswerfen */

    if (trl!=NULL) {
      for (u=r2->units;u;u=u->next) {
        if (u->race != new_race[RC_SPELL]) {
          region_list *trl2 = trl;
          region *tr;
          int c = rng_int() % inhab_regions;

          /* Zufällige Zielregion suchen */
          while (c--!=0) trl2 = trl2->next;
          tr = trl2->data;

          if (!is_magic_resistant(mage, u, 0) && can_survive(u, tr)) {
            message * msg = msg_message("disrupt_astral", "unit region", u, tr);
            add_message(&u->faction->msgs, msg);
            add_message(&tr->msgs, msg);
            msg_release(msg);

            move_unit(u, tr, NULL);
          }
        }
      }
      free_regionlist(trl);
    }

    /* Kontakt unterbinden */
    effect = 100;
    create_curse(mage, &rl2->data->attribs, ct_find("astralblock"),
      power, duration, effect, 0);
  }

  free_regionlist(rl);
  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Mauern der Ewigkeit
 * Stufe:      7
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
  curse * c;
  building *b;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;
  spellparameter *pa = co->par;
  message * msg;

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (pa->param[0]->flag == TARGET_NOTFOUND) return 0;

  b = pa->param[0]->data.b;
  c = create_curse(mage, &b->attribs, ct_find("nocostbuilding"),
    power*power, 1, zero_effect, 0);

  if (c==NULL) { /* ist bereits verzaubert */
    cmistake(mage, co->order, 206, MSG_MAGIC);
    return 0;
  }

  /* melden, 1x pro Partei in der Burg */
  for (u = r->units; u; u = u->next) freset(u->faction, FFL_SELECT);
  msg = msg_message("sp_eternizewall_effect", "mage building region", mage, b, r);
  for (u = r->units; u; u = u->next) {
    if (!fval(u->faction, FFL_SELECT)) {
      fset(u->faction, FFL_SELECT);
      if (u->building ==  b) {
        r_addmessage(r, u->faction, msg);
      }
    }
  }
  if (r!=mage->region) {
    add_message(&mage->faction->msgs, msg);
  } else if (mage->building!=b) {
    r_addmessage(r, mage->faction, msg);
  } 
  msg_release(msg);

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
 * (UNITSPELL)
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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  spellparameter *pa = co->par;
  const spell *sp = co->sp;
  message * msg;

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (pa->param[0]->flag == TARGET_NOTFOUND) return 0;

  /* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
   * abbrechen aber kosten lassen */
  if (pa->param[0]->flag == TARGET_RESISTS) return cast_level;

  tu = pa->param[0]->data.u;
  aura = pa->param[1]->data.i;

  if (!is_mage(tu)) {
/*    sprintf(buf, "%s in %s: 'ZAUBER %s': Einheit ist kein Magier."
      , unitname(mage), regionname(mage->region, mage->faction),sa->strings[0]); */
    cmistake(mage, co->order, 214, MSG_MAGIC);
    return 0;
  }

  aura = MIN(get_spellpoints(mage)-spellcost(mage, sp), aura);

  change_maxspellpoints(mage,-aura);
  change_spellpoints(mage,-aura);

  if (get_mage(tu)->magietyp == get_mage(mage)->magietyp) {
    change_maxspellpoints(tu, aura/2);
  } else {
    change_maxspellpoints(tu, aura/3);
  }

  msg = msg_message("sp_permtransfer_effect", "mage target amount", mage, tu, aura);
  add_message(&mage->faction->msgs, msg);
  if (tu->faction!=mage->faction) {
    add_message(&tu->faction->msgs, msg);
  }
  msg_release(msg);

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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  spellparameter *pa = co->par;
  message* msg;

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (pa->param[0]->flag == TARGET_NOTFOUND) return 0;

  b = pa->param[0]->data.b;
  dir = finddirection(pa->param[1]->data.xs, mage->faction->locale);

  if (dir == NODIRECTION) {
    /* Die Richtung wurde nicht erkannt */
    cmistake(mage, co->order, 71, MSG_PRODUCE);
    return 0;
  }

  if (b->size > (cast_level-12) * 250) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "sp_movecastle_fail_0", ""));
    return cast_level;
  }

  target_region = rconnect(r,dir);

  if (!(target_region->terrain->flags & LAND_REGION)) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "sp_movecastle_fail_1", "direction", dir));
    return cast_level;
  }

  bunhash(b);
  translist(&r->buildings, &target_region->buildings, b);
  b->region = target_region;
  b->size -= b->size/(10-rng_int()%6);
  bhash(b);

  for(u=r->units;u;) {
    unext = u->next;
    if (u->building == b) {
      uunhash(u);
      translist(&r->units, &target_region->units, u);
      uhash(u);
    }
    u = unext;
  }

  if ((b->type==bt_find("caravan") || b->type==bt_find("dam") || b->type==bt_find("tunnel"))) {
    boolean damage = false;
    direction_t d;
    for (d=0;d!=MAXDIRECTIONS;++d) {
      if (rroad(r, d)) {
        rsetroad(r, d, rroad(r, d)/2);
        damage = true;
      }
    }
/*    if (damage) strcat(buf, " Die Straßen der Region wurden beschädigt."); */
  }

  msg = msg_message("sp_movecastle_effect", "building direction", b, dir);
  r_addmessage(r, NULL, msg);
  msg_release(msg);

  return cast_level;
}

curse *
shipcurse_flyingship(ship* sh, unit * mage, double power, int duration)
{
  static const curse_type * ct_flyingship = NULL;
  if (!ct_flyingship) {
    ct_flyingship = ct_find("flyingship");
    assert(ct_flyingship);
  }
  if (curse_active(get_curse(sh->attribs, ct_flyingship))) {
    return NULL;
  } else if (is_cursed(sh->attribs, C_SHIP_SPEEDUP, 0)) {
    return NULL;
  } else {
    /* mit C_SHIP_NODRIFT haben wir kein Problem */
    return create_curse(mage, &sh->attribs, ct_flyingship, power, duration, zero_effect, 0);
  }
}

int
levitate_ship(ship * sh, unit * mage, double power, int duration)
{
  curse * c = shipcurse_flyingship(sh, mage, power, duration);
  if (c) {
    return c->no;
  }
  return 0;
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
 *  (ONSHIPCAST | SHIPSPELL | TESTRESISTANCE)
 */
int
sp_flying_ship(castorder *co)
{
  ship *sh;
  unit *u;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;
  spellparameter *pa = co->par;
  message * m = NULL;
  int cno;

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (pa->param[0]->flag == TARGET_NOTFOUND) return 0;
  sh = pa->param[0]->data.sh;
  if (sh->type->construction->maxsize>50) {
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "error_flying_ship_too_big", "ship", sh));
    return 0;
  }

  /* Duration = 1, nur diese Runde */
    
  cno = levitate_ship(sh, mage, power, 1);
  if (cno==0) {
    if (is_cursed(sh->attribs, C_SHIP_FLYING, 0) ) {
      /* Auf dem Schiff befindet liegt bereits so ein Zauber. */
      cmistake(mage, co->order, 211, MSG_MAGIC);
    } else if (is_cursed(sh->attribs, C_SHIP_SPEEDUP, 0) ) {
      /* Es ist zu gefährlich, ein sturmgepeitschtes Schiff fliegen zu lassen. */
      cmistake(mage, co->order, 210, MSG_MAGIC);
    }
    return 0;
  }
  sh->coast = NODIRECTION;

  /* melden, 1x pro Partei */
  for (u = r->units; u; u = u->next) freset(u->faction, FFL_SELECT);
  for(u = r->units; u; u = u->next ) {
    /* das sehen natürlich auch die Leute an Land */
    if (!fval(u->faction, FFL_SELECT) ) {
      fset(u->faction, FFL_SELECT);
      if (!m) m = msg_message("flying_ship_result", "mage ship", mage, sh);
      add_message(&u->faction->msgs, m);
    }
  }
  if (m) msg_release(m);
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
 * (FARCASTING | SPELLLEVEL | UNITSPELL | TESTRESISTANCE |
 * TESTCANSEE)
 * */
int
sp_stealaura(castorder *co)
{
  int taura;
  unit *u;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double power = co->force;
  spellparameter *pa = co->par;

  /* wenn kein Ziel gefunden, Zauber abbrechen */
  if (pa->param[0]->flag == TARGET_NOTFOUND) return 0;

  /* Zieleinheit */
  u  = pa->param[0]->data.u;

  if (!get_mage(u)) {
    ADDMSG(&mage->faction->msgs, msg_message(
      "stealaura_fail", "unit target", mage, u));
    ADDMSG(&u->faction->msgs, msg_message(
      "stealaura_fail_detect", "unit", u));
    return 0;
  }

  taura = (get_mage(u)->spellpoints*(rng_int()%(int)(3*power)+1))/100;

  if (taura > 0) {
    get_mage(u)->spellpoints -= taura;
    get_mage(mage)->spellpoints += taura;
/*    sprintf(buf, "%s entzieht %s %d Aura.", unitname(mage), unitname(u),
      taura); */
    ADDMSG(&mage->faction->msgs, msg_message(
      "stealaura_success", "mage target aura", mage, u, taura));
/*    sprintf(buf, "%s fühlt seine magischen Kräfte schwinden und verliert %d "
      "Aura.", unitname(u), taura); */
    ADDMSG(&u->faction->msgs, msg_message(
      "stealaura_detect", "unit aura", u, taura));
  } else {
    ADDMSG(&mage->faction->msgs, msg_message(
      "stealaura_fail", "unit target", mage, u));
    ADDMSG(&u->faction->msgs, msg_message(
      "stealaura_fail_detect", "unit", u));
  }
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
  double power;
  double effect;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  int duration = (int)force+1;

  /* Hält Sprüche bis zu einem summierten Gesamtlevel von power aus.
   * Jeder Zauber reduziert die 'Lebenskraft' (vigour) der Antimagiezone
   * um seine Stufe */
  power = force * 10;

  /* Reduziert die Stärke jedes Spruchs um effect */
  effect = cast_level;

  create_curse(mage, &r->attribs, ct_find("antimagiczone"), power, duration,
      effect, 0);

  /* Erfolg melden*/
  ADDMSG(&mage->faction->msgs, msg_message(
        "regionmagic_effect", "unit region command", mage,
        mage->region, co->order));

  return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Schutzrunen
 * Stufe:   8
 * Kosten:   SPC_FIX
 *
 * Wirkung:
 *   Gibt Gebäuden einen Bonus auf Magieresistenz von +20%. Der Zauber
 *   dauert 3+rng_int()%Level Wochen an, also im Extremfall bis zu 2 Jahre
 *   bei Stufe 20
 *
 *   Es können mehrere Zauber übereinander gelegt werden, der Effekt
 *   summiert sich, jedoch wird die Dauer dadurch nicht verlängert.
 *
 * oder:
 *
 *   Gibt Schiffen einen Bonus auf Magieresistenz von +20%. Der Zauber
 *   dauert 3+rng_int()%Level Wochen an, also im Extremfall bis zu 2 Jahre
 *   bei Stufe 20
 *
 *   Es können mehrere Zauber übereinander gelegt werden, der Effekt
 *   summiert sich, jedoch wird die Dauer dadurch nicht verlängert.
 *
 * Flags:
 * (ONSHIPCAST | TESTRESISTANCE)
 *
 * Syntax:
 *  ZAUBERE \"Runen des Schutzes\" GEBAEUDE <Gebaeude-Nr>
 *  ZAUBERE \"Runen des Schutzes\" SCHIFF <Schiff-Nr>
 * "kc"
 */

static int
sp_magicrunes(castorder *co)
{
  int duration;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  spellparameter *pa = co->par;
  double effect;

  duration = 3 + rng_int()%cast_level;
  effect = 20;

  switch(pa->param[0]->typ) {
    case SPP_BUILDING:
    {
      building *b;
      b = pa->param[0]->data.b;

      /* Magieresistenz der Burg erhöht sich um 20% */
      create_curse(mage, &b->attribs, ct_find("magicrunes"), force,
          duration, effect, 0);

      /* Erfolg melden */
      ADDMSG(&mage->faction->msgs, msg_message(
          "objmagic_effect", "unit region command target", mage,
          mage->region, co->order, buildingname(b)));
      break;
    }
    case SPP_SHIP:
    {
      ship *sh;
      sh = pa->param[0]->data.sh;
      /* Magieresistenz des Schiffs erhöht sich um 20% */
      create_curse(mage, &sh->attribs, ct_find("magicrunes"), force,
          duration, effect, 0);

      /* Erfolg melden */
      ADDMSG(&mage->faction->msgs, msg_message(
          "objmagic_effect", "unit region command target", mage,
          mage->region, co->order, shipname(sh)));
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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  spellparameter *pa = co->par;

  maxmen = 2 * cast_level * cast_level;
  dur = MAX(1, cast_level/2);

  for (n = 0; n < pa->length; n++) {
    double effect;
    /* sollte nie negativ werden */
    if (maxmen < 1)
      break;

    if (pa->param[n]->flag == TARGET_RESISTS
        || pa->param[n]->flag == TARGET_NOTFOUND)
      continue;

    u = pa->param[n]->data.u;

    men = MIN(maxmen,u->number);
    effect = 2;
    create_curse(mage, &u->attribs, ct_find("speed"), force, dur, effect, men);
    maxmen -= men;
    used += men;
  }

  ADDMSG(&mage->faction->msgs, msg_message("speed_time_effect", "unit region amount", mage, mage->region, used));
  /* Effektiv benötigten cast_level (mindestens 1) zurückgeben */
  used = (int)sqrt(used/2);
  return MAX(1, used);
}

/* ------------------------------------------------------------- */
/* Name:     Magiefresser
 * Stufe:   7
 * Kosten:   SPC_LEVEL
 *
 * Wirkung:
 *   Kann eine bestimmte Verzauberung angreifen und auflösen. Die Stärke
 *   des Zaubers muss stärker sein als die der Verzauberung.
 * Syntax:
 *  ZAUBERE \"Magiefresser\" REGION
 *  ZAUBERE \"Magiefresser\" EINHEIT <Einheit-Nr>
 *  ZAUBERE \"Magiefresser\" GEBAEUDE <Gebaeude-Nr>
 *  ZAUBERE \"Magiefresser\" SCHIFF <Schiff-Nr>
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
  curse * c = NULL;
  int succ;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  spellparameter *pa = co->par;
  const char *ts = NULL;

  obj = pa->param[0]->typ;

  switch(obj) {
    case SPP_REGION:
      ap = &r->attribs;
      ts = regionname(r, mage->faction);
      break;

    case SPP_TEMP:
    case SPP_UNIT:
    {
      unit *u = pa->param[0]->data.u;
      ap = &u->attribs;
      ts =  unitid(u);
      break;
    }
    case SPP_BUILDING:
    {
      building *b = pa->param[0]->data.b;
      ap = &b->attribs;
      ts = buildingid(b);
      break;
    }
    case SPP_SHIP:
    {
      ship *sh = pa->param[0]->data.sh;
      ap = &sh->attribs;
      ts = shipid(sh);
      break;
    }
    default:
      /* Das Zielobjekt wurde vergessen */
      cmistake(mage, co->order, 203, MSG_MAGIC);
    return 0;
  }

  succ = break_curse(ap, cast_level, force, c);

  if (succ) {
    ADDMSG(&mage->faction->msgs, msg_message(
      "destroy_magic_effect", "unit region command succ target",
      mage, mage->region, co->order, succ, ts));
  } else {
    ADDMSG(&mage->faction->msgs, msg_message(
      "destroy_magic_noeffect", "unit region command",
      mage, mage->region, co->order));
  }
  return MAX(succ, 1);
}

/* ------------------------------------------------------------- */
/* Name:     Fluch brechen
 * Stufe:   7
 * Kosten:   SPC_LEVEL
 *
 * Wirkung:
 *   Kann eine bestimmte Verzauberung angreifen und auflösen. Die Stärke
 *   des Zaubers muss stärker sein als die der Verzauberung.
 * Syntax:
 *  ZAUBERE \"Fluch brechen\" REGION <Zauber-id>
 *  ZAUBERE \"Fluch brechen\" EINHEIT <Einheit-Nr> <Zauber-id>
 *  ZAUBERE \"Fluch brechen\" GEBAEUDE <Gebaeude-Nr> <Zauber-id>
 *  ZAUBERE \"Fluch brechen\" SCHIFF <Schiff-Nr> <Zauber-id>
 *
 *  "kcc"
 * Flags:
 *   (FARCASTING | SPELLLEVEL | ONSHIPCAST | TESTCANSEE)
 */
int
sp_break_curse(castorder *co)
{
  attrib **ap;
  int obj;
  curse * c;
  region *r = co->rt;
  unit *mage = co->magician.u;
  int cast_level = co->level;
  double force = co->force;
  spellparameter *pa = co->par;
  const char *ts = NULL;

  if (pa->length < 2) {
    /* Das Zielobjekt wurde vergessen */
    cmistake(mage, co->order, 203, MSG_MAGIC);
  }

  obj = pa->param[0]->typ;

  c = findcurse(atoi36(pa->param[1]->data.s));
  if (!c) {
    /* Es wurde kein Ziel gefunden */
    ADDMSG(&mage->faction->msgs, msg_message(
          "spelltargetnotfound", "unit region command",
          mage, mage->region, co->order));
  } else {
    switch(obj) {
    case SPP_REGION:
      ap = &r->attribs;
      ts = regionname(r, mage->faction);
      break;

    case SPP_TEMP:
    case SPP_UNIT:
    {
      unit *u = pa->param[0]->data.u;
      ap = &u->attribs;
      ts = unitid(u);
      break;
    }
    case SPP_BUILDING:
    {
      building *b = pa->param[0]->data.b;
      ap = &b->attribs;
      ts = buildingid(b);
      break;
    }
    case SPP_SHIP:
    {
      ship *sh = pa->param[0]->data.sh;
      ap = &sh->attribs;
      ts = shipid(sh);
      break;
    }
    default:
      /* Das Zielobjekt wurde vergessen */
      cmistake(mage, co->order, 203, MSG_MAGIC);
      return 0;
    }

    /* überprüfung, ob curse zu diesem objekt gehört */
    if (!is_cursed_with(*ap, c)) {
      /* Es wurde kein Ziel gefunden */
      ADDMSG(&mage->faction->msgs,
            msg_message(
                  "spelltargetnotfound", "unit region command",
                  mage, mage->region, co->order));
    }

    /* curse auflösen, wenn zauber stärker (force > vigour)*/
    c->vigour -= force;

    if (c->vigour <= 0.0) {
      remove_curse(ap, c);

      ADDMSG(&mage->faction->msgs, msg_message(
        "destroy_curse_effect", "unit region command id target",
        mage, mage->region, co->order, pa->param[1]->data.xs, ts));
    } else {
      ADDMSG(&mage->faction->msgs, msg_message(
        "destroy_curse_noeffect", "unit region command id target",
        mage, mage->region, co->order, pa->param[1]->data.xs, ts));
    }
  }

  return cast_level;
}


/* ------------------------------------------------------------- */
int
sp_becomewyrm(castorder *co)
{
#if KARMA_MODULE
  unit *u = co->magician.u;
  int wyrms_already_created = 0;
  int wyrms_allowed = 0;
  attrib *a;

  wyrms_allowed = fspecial(u->faction, FS_WYRM);
  a = a_find(u->faction->attribs, &at_wyrm);
  if (a) wyrms_already_created = a->data.i;

  if (wyrms_already_created >= wyrms_allowed) {
    cmistake(u, co->order, 262, MSG_MAGIC);
    return 0;
  }

  if (!a) {
    a_add(&u->faction->attribs, a_new(&at_wyrm));
    a->data.i = 1;
  } else {
    a->data.i++;
  }

  u->race = new_race[RC_WYRM];
  add_spell(get_mage(u), find_spellbyid(M_GRAY, SPL_WYRMODEM));

  ADDMSG(&u->faction->msgs, msg_message("becomewyrm", "u", u));

  return co->level;
#else
  return 0;
#endif /* KARMA_MODULE */
}

/* ------------------------------------------------------------- */
/* Name:       WDW-Pyramidenfindezauber
 * Stufe:      unterschiedlich
 * Gebiet:     alle
 * Wirkung:
 *             gibt die ungefaehre Entfernung zur naechstgelegenen Pyramiden-
 *             region an.
 *
 * Flags:
 */
static int
sp_wdwpyramid(castorder *co)
{
  region  *r          = co->rt;
  unit    *mage       = co->magician.u;
  int     cast_level  = co->level;

  if (a_find(r->attribs, &at_wdwpyramid) != NULL) {
    ADDMSG(&mage->faction->msgs, msg_message("wdw_pyramidspell_found",
      "unit region command", mage, r, co->order));
  } else {
    region *r2;
    int    mindist = INT_MAX;
    int    minshowdist;
    int    maxshowdist;

    for(r2 = regions; r2; r2 = r2->next) {
      if (a_find(r2->attribs, &at_wdwpyramid) != NULL) {
        int dist = distance(mage->region, r2);
        if (dist < mindist) {
          mindist = dist;
        }
      }
    }

    assert(mindist >= 1);

    minshowdist = mindist - rng_int()%5;
    maxshowdist = minshowdist + 4;

    ADDMSG(&mage->faction->msgs, msg_message("wdw_pyramidspell_notfound",
      "unit region command mindist maxdist", mage, r, co->order,
      MAX(1, minshowdist), maxshowdist));
  }

  return cast_level;
}

typedef struct spelldata {
  spellid_t id;
  const char *sname;
  spell_f sp_function;
  void (*patzer) (castorder*);
} spelldata;

static spelldata spelldaten[] =
{
  /* M_GWYRRD */
  {
    SPL_STONEGOLEM, "stonegolem",
    (spell_f)sp_create_stonegolem, NULL
  },
  {
    SPL_IRONGOLEM, "irongolem",
    (spell_f)sp_create_irongolem, NULL
  },
  {
    SPL_TREEGROW, "treegrow",
    (spell_f)sp_hain, patzer_ents
  },
  {
    SPL_RUSTWEAPON, "rustweapon",
    (spell_f)sp_rosthauch, NULL
  },
  {
    SPL_KAELTESCHUTZ, "cold_protection",
    (spell_f)sp_kaelteschutz, NULL
  },
  {
    SPL_HAGEL, "hail",
    (spell_f)sp_kampfzauber, NULL
  },
  {
    SPL_IRONKEEPER, "ironkeeper",
    (spell_f)sp_ironkeeper, NULL
  },
  {
    SPL_MAGICSTREET, "magicstreet",
    (spell_f)sp_magicstreet, NULL
  },
  {
    SPL_WINDSHIELD, "windshield",
    (spell_f)sp_windshield, NULL
  },
  {
    SPL_MALLORNTREEGROW, "mallorntreegrow", 
    (spell_f)sp_mallornhain, patzer_ents
  },
  { SPL_GOODWINDS, "goodwinds",
    (spell_f)sp_goodwinds, NULL
  },
  {
    SPL_HEALING, "healing", 
    (spell_f)sp_healing, NULL
  },
  {
    SPL_REELING_ARROWS, "reelingarrows", 
    (spell_f)sp_reeling_arrows, NULL
  },
  {
    SPL_GWYRRD_FUMBLESHIELD, "gwyrrdfumbleshield", 
    (spell_f)sp_fumbleshield, NULL
  },
  {
    SPL_TRANSFERAURA_DRUIDE, "transferauradruide",
    (spell_f)sp_transferaura, NULL
  },
  {
    SPL_EARTHQUAKE, "earthquake", 
    (spell_f)sp_earthquake, NULL
  },
  {
    SPL_STORMWINDS, "stormwinds", 
    (spell_f)sp_stormwinds, NULL
  },
  {
    SPL_HOMESTONE, "homestone", 
    (spell_f)sp_homestone, NULL
  },
  {
    SPL_WOLFHOWL, "wolfhowl", 
    (spell_f)sp_wolfhowl, NULL
  },
  {
    SPL_VERSTEINERN, "versteinern", 
    (spell_f)sp_petrify, NULL
  },
  {
    SPL_STRONG_WALL, "strongwall", 
    (spell_f)sp_strong_wall, NULL
  },
  {
    SPL_GWYRRD_DESTROY_MAGIC, "gwyrrddestroymagic", 
    (spell_f)sp_destroy_magic, NULL
  },
  {
    SPL_TREEWALKENTER, "treewalkenter",
    (spell_f)sp_treewalkenter, NULL
  },
  {
    SPL_TREEWALKEXIT, "treewalkexit",
    (spell_f)sp_treewalkexit, NULL
  },
  {
    SPL_HOLYGROUND, "holyground",
    (spell_f)sp_holyground, NULL
  },
  {
    SPL_SUMMONENT, "summonent", 
    (spell_f)sp_summonent, NULL
  },
  {
    SPL_GWYRRD_FAMILIAR, "gwyrrdfamiliar", 
    (spell_f)sp_summon_familiar, NULL
  },
  {
    SPL_BLESSSTONECIRCLE, "blessstonecircle", 
    (spell_f)sp_blessstonecircle, NULL
  },
  {
    SPL_GWYRRD_ARMORSHIELD, "barkskin",
    (spell_f)sp_armorshield, NULL
  },
  {
    SPL_DROUGHT, "summonfireelemental", 
    (spell_f)sp_drought, NULL
  },
  {
    SPL_MAELSTROM, "maelstrom",
    (spell_f)sp_maelstrom, NULL
  },
  {
    SPL_MALLORN, "magic_roots",
    (spell_f)sp_mallorn, NULL
  },
  {
    SPL_GREAT_DROUGHT, "great_drought", 
    (spell_f)sp_great_drought, NULL
  },
  /* M_DRAIG */
  {
    SPL_SPARKLE_CHAOS, "sparklechaos", 
    (spell_f)sp_sparkle, NULL
  },
  {
    SPL_FIREBALL, "fireball", 
    (spell_f)sp_kampfzauber, NULL
  },
  {
    SPL_MAGICBOOST, "magicboost", 
    (spell_f)sp_magicboost, NULL
  },
  {
    SPL_BLOODSACRIFICE, "bloodsacrifice",
    (spell_f)sp_bloodsacrifice, NULL
  },
  {
    SPL_BERSERK, "berserk", 
    (spell_f)sp_berserk, NULL
  },
  {
    SPL_FUMBLECURSE, "fumblecurse",
    (spell_f)sp_fumblecurse, patzer_fumblecurse
  },
  {
    SPL_SUMMONUNDEAD, "summonundead", 
    (spell_f)sp_summonundead, patzer_peasantmob
  },
  {
    SPL_COMBATRUST, "combatrust", 
    (spell_f)sp_combatrosthauch, NULL
  },
  {
    SPL_TRANSFERAURA_CHAOS, "transferaurachaos", 
    (spell_f)sp_transferaura, NULL
  },
  {
    SPL_FIREWALL, "firewall",
    (spell_f)sp_firewall, patzer_peasantmob
  },
  {
    SPL_PLAGUE, "plague",
    (spell_f)sp_plague, patzer_peasantmob
  },
  {
    SPL_CHAOSROW, "chaosrow",
    (spell_f)sp_chaosrow, NULL
  },
  {
    SPL_SUMMONSHADOW, "summonshadow",
    (spell_f)sp_summonshadow, patzer_peasantmob
  },
  {
    SPL_UNDEADHERO, "undeadhero",
    (spell_f)sp_undeadhero, NULL
  },
  {
    SPL_AURALEAK, "auraleak", 
    (spell_f)sp_auraleak, NULL
  },
  {
    SPL_DRAIG_FUMBLESHIELD, "draigfumbleshield",
    (spell_f)sp_fumbleshield, NULL
  },
  {
    SPL_FOREST_FIRE, "forestfire",
    (spell_f)sp_forest_fire, patzer_peasantmob
  },
  {
    SPL_DRAIG_DESTROY_MAGIC, "draigdestroymagic",
    (spell_f)sp_destroy_magic, NULL
  },
  {
    SPL_UNHOLYPOWER, "unholypower",
    (spell_f)sp_unholypower, NULL
  },
  {
    SPL_DEATHCLOUD, "deathcloud",
    (spell_f)sp_deathcloud, patzer_peasantmob
  },
  {
    SPL_SUMMONDRAGON, "summondragon",
    (spell_f)sp_summondragon, patzer_peasantmob
  },
  {
    SPL_SUMMONSHADOWLORDS, "summonshadowlords",
    (spell_f)sp_summonshadowlords, patzer_peasantmob
  },
  {
    SPL_DRAIG_FAMILIAR, "draigfamiliar",
    (spell_f)sp_summon_familiar, NULL
  },
  {
    SPL_CHAOSSUCTION, "chaossuction",
    (spell_f)sp_chaossuction, patzer_peasantmob
  },
  /* M_ILLAUN */
  {
    SPL_SPARKLE_DREAM, "sparkledream",
    (spell_f)sp_sparkle, NULL
  },
  {
    SPL_SHADOWKNIGHTS, "shadowknights",
    (spell_f)sp_shadowknights, NULL
  },
  {
    SPL_FLEE, "flee",
    (spell_f)sp_flee, NULL
  },
  {
    SPL_PUTTOREST, "puttorest",
    (spell_f)sp_puttorest, NULL
  },
  {
    SPL_ICASTLE, "icastle",
    (spell_f)sp_icastle, NULL
  },
  {
    SPL_TRANSFERAURA_TRAUM, "transferauratraum",
    (spell_f)sp_transferaura, NULL
  },
  {
    SPL_ILL_SHAPESHIFT, "shapeshift",
    (spell_f)sp_illusionary_shapeshift, NULL
  },
  {
    SPL_DREAMREADING, "dreamreading",
    (spell_f)sp_dreamreading, NULL
  },
  {
    SPL_TIREDSOLDIERS, "tiredsoldiers",
    (spell_f)sp_tiredsoldiers, NULL
  },
  {
    SPL_REANIMATE, "reanimate",
    (spell_f)sp_reanimate, NULL
  },
  {
    SPL_ANALYSEDREAM, "analysedream",
    (spell_f)sp_analysedream, NULL
  },
  {
    SPL_DISTURBINGDREAMS, "disturbingdreams",
    (spell_f)sp_disturbingdreams, NULL
  },
  {
    SPL_SLEEP, "sleep",
    (spell_f)sp_sleep, NULL
  },
  {
    SPL_WISPS, "wisps",
    (spell_f)sp_wisps, NULL
  },
  {
    SPL_GOODDREAMS, "gooddreams",
    (spell_f)sp_gooddreams, NULL
  },
  {
    SPL_ILLAUN_DESTROY_MAGIC, "illaundestroymagic",
    (spell_f)sp_destroy_magic, NULL
  },
  {
    SPL_ILLAUN_FAMILIAR, "illaunfamiliar",
    (spell_f)sp_summon_familiar, NULL
  },
  {
    SPL_CLONECOPY, "clone",
    (spell_f)sp_clonecopy, NULL
  },
  {
    SPL_BADDREAMS, "bad_dreams",
    (spell_f)sp_baddreams, NULL
  },
  {
    SPL_MINDBLAST, "mindblast",
    (spell_f)sp_mindblast_temp, NULL
  },
  {
    SPL_ORKDREAM, "orkdream",
    (spell_f)sp_sweetdreams, NULL
  },
  {
    SPL_SUMMON_ALP, "summon_alp",
    (spell_f)sp_summon_alp, NULL
  },
  /* M_CERDDOR */
  {
    SPL_DENYATTACK, "appeasement",
    (spell_f)sp_denyattack, NULL
  },
  {
    SPL_HEALINGSONG, "song_of_healing",
    (spell_f)sp_healing, NULL
  },
  {
    SPL_GENEROUS, "generous",
    (spell_f)sp_generous, NULL
  },
  {
    SPL_SONG_OF_FEAR, "song_of_fear",
    (spell_f)sp_flee, NULL
  },
  {
    SPL_RECRUIT, "courting",
    (spell_f)sp_recruit, NULL
  },
  {
    SPL_SONG_OF_CONFUSION, "song_of_confusion",
    (spell_f)sp_chaosrow, NULL
  },
  {
    SPL_HERO, "heroic_song",
    (spell_f)sp_hero, NULL
  },
  {
    SPL_TRANSFERAURA_BARDE, "transfer_aura_song",
    (spell_f)sp_transferaura, NULL
  },
  {
    SPL_UNIT_ANALYSESONG, "analysesong_unit",
    (spell_f)sp_analysesong_unit, NULL
  },
  {
    SPL_CERRDOR_FUMBLESHIELD, "cerrdorfumbleshield",
    (spell_f)sp_fumbleshield, NULL
  },
  { SPL_CALM_MONSTER, "calm_monster",
    (spell_f)sp_calm_monster, NULL
  },
  { SPL_SEDUCE, "seduction",
    (spell_f)sp_seduce, NULL
  },
  {
    SPL_HEADACHE, "headache",
    (spell_f)sp_headache, NULL
  },
  { SPL_PUMP, "sound_out",
    (spell_f)sp_pump, NULL
  },
  {
    SPL_BLOODTHIRST, "bloodthirst",
    (spell_f)sp_berserk, NULL
  },
  {
    SPL_FRIGHTEN, "frighten",
    (spell_f)sp_frighten, NULL
  },
  {
    SPL_OBJ_ANALYSESONG, "analyse_object",
    (spell_f)sp_analysesong_obj, NULL
  },
  {
    SPL_CERDDOR_DESTROY_MAGIC, "cerddor_destroymagic",
    (spell_f)sp_destroy_magic, NULL
  },
  {
    SPL_MIGRANT, "migration",
    (spell_f)sp_migranten, NULL
  },
  {
    SPL_CERDDOR_FAMILIAR, "summon_familiar",
    (spell_f)sp_summon_familiar, NULL
  },
  {
    SPL_RAISEPEASANTS, "raise_mob",
    (spell_f)sp_raisepeasants, NULL
  },
  {
    SPL_SONG_RESISTMAGIC, "song_resist_magic",
    (spell_f)sp_song_resistmagic, NULL
  },
  {
    SPL_DEPRESSION, "melancholy",
    (spell_f)sp_depression, NULL
  },
  {
    SPL_SONG_SUSCEPTMAGIC, "song_suscept_magic",
    (spell_f)sp_song_susceptmagic, NULL
  },
  {
    SPL_SONG_OF_PEACE, "song_of_peace",
    (spell_f)sp_song_of_peace, NULL
  },
  {
    SPL_SONG_OF_ENSLAVE, "song_of_slavery",
    (spell_f)sp_charmingsong, NULL
  },
  {
    SPL_BIGRECRUIT, "big_recruit",
    (spell_f)sp_bigrecruit, NULL
  },
  {
    SPL_RALLYPEASANTMOB, "calm_riot",
    (spell_f)sp_rallypeasantmob, NULL
  },
  {
    SPL_RAISEPEASANTMOB, "incite_riot",
        (spell_f)sp_raisepeasantmob, NULL
  },
  /* M_TYBIED */
  {
    SPL_ANALYSEMAGIC, "analyze_magic",
    (spell_f)sp_analysemagic, NULL
  },
  {
    SPL_ITEMCLOAK, "concealing_aura",
    (spell_f)sp_itemcloak, NULL
  },
  {
    SPL_TYBIED_FUMBLESHIELD, "tybiedfumbleshield",
    (spell_f)sp_fumbleshield, NULL
  },
#ifdef SHOWASTRAL_NOT_BORKED
  {
    SPL_SHOWASTRAL, "show_astral",
        (spell_f)sp_showastral, NULL
  },
#endif
  {
    SPL_RESISTMAGICBONUS, "resist_magic",
    (spell_f)sp_resist_magic_bonus, NULL
  },
  {
    SPL_KEEPLOOT, "keeploot",
    (spell_f)sp_keeploot, NULL
  },
  {
    SPL_ENTERASTRAL, "enterastral",
    (spell_f)sp_enterastral, NULL
  },
  {
    SPL_LEAVEASTRAL, "leaveastral",
    (spell_f)sp_leaveastral, NULL
  },
  {
    SPL_TRANSFERAURA_ASTRAL, "auratransfer",
    (spell_f)sp_transferaura, NULL
  },
  {
    SPL_SHOCKWAVE, "shockwave",
    (spell_f)sp_stun, NULL
  },
  {
    SPL_ANTIMAGICZONE, "antimagiczone",
    (spell_f)sp_antimagiczone, NULL
  },
  {
    SPL_TYBIED_DESTROY_MAGIC, "destroy_magic",
    (spell_f)sp_destroy_magic, NULL
  },
  {
    SPL_PULLASTRAL, "pull_astral",
    (spell_f)sp_pullastral, NULL
  },

  {
    SPL_FETCHASTRAL, "fetch_astral",
    (spell_f)sp_fetchastral, NULL
  },
  {
    SPL_STEALAURA, "steal_aura",
    (spell_f)sp_stealaura, NULL
  },
  {
    SPL_FLYING_SHIP, "airship",
    (spell_f)sp_flying_ship, NULL
  },
  {
    SPL_DESTROY_MAGIC, "break_curse",
    (spell_f)sp_break_curse, NULL
  },
  {
    SPL_ETERNIZEWALL, "eternal_walls",
    (spell_f)sp_eternizewall, NULL
  },
  {
    SPL_SCHILDRUNEN, "protective_runes",
    (spell_f)sp_magicrunes, NULL
  },

  {
    SPL_REDUCESHIELD, "fish_shield",
    (spell_f)sp_reduceshield, NULL
  },
  {
    SPL_SPEED, "combat_speed",
    (spell_f)sp_speed, NULL
  },
  {
    SPL_VIEWREALITY, "view_reality",
    (spell_f)sp_viewreality, NULL
  },
  {
    SPL_SPEED2, "double_time",
    (spell_f)sp_speed2, NULL
  },
  {
    SPL_ARMORSHIELD, "armor_shield",
        (spell_f)sp_armorshield, NULL
  },
  {
    SPL_TYBIED_FAMILIAR, "summon_familiar",
        (spell_f)sp_summon_familiar, NULL
  },
  {
    SPL_MOVECASTLE, "living_rock",
    (spell_f)sp_movecastle, NULL
  },
  {
    SPL_DISRUPTASTRAL, "astral_disruption",
    (spell_f)sp_disruptastral, NULL
  },
  {
    SPL_PERMTRANSFER, "sacrifice_strength",
    (spell_f)sp_permtransfer, NULL
  },
  /* M_GRAY */
  /*  Definitionen von Create_Artefaktsprüchen    */
  {
    SPL_METEORRAIN, "meteor_rain",
    (spell_f)sp_kampfzauber, NULL
  },
  {
    SPL_BECOMEWYRM, "wyrm_transformation",
    (spell_f)sp_becomewyrm, NULL
  },
  /* Monstersprüche */
  { SPL_FIREDRAGONODEM, "fiery_dragonbreath",
    (spell_f)sp_dragonodem, NULL
  },
  { SPL_DRAGONODEM, "icy_dragonbreath",
    (spell_f)sp_dragonodem, NULL
  },
  { SPL_WYRMODEM, "powerful_dragonbreath",
    (spell_f)sp_dragonodem, NULL
  },
  { SPL_DRAINODEM, "drain_skills",
    (spell_f)sp_dragonodem, NULL
  },
  {
    SPL_AURA_OF_FEAR, "aura_of_fear",
    (spell_f)sp_flee, NULL
  },
  {
    SPL_SHADOWCALL, "shadowcall",
    (spell_f)sp_shadowcall, NULL
  },
  {
    SPL_IMMOLATION, "immolation",
    (spell_f)sp_immolation, NULL
  },
  { SPL_FIREODEM, "firestorm",
    (spell_f)sp_immolation, NULL
  },
  { SPL_ICEODEM, "coldfront",
    (spell_f)sp_immolation, NULL
  },
  { SPL_ACIDODEM, "acidrain",
    (spell_f)sp_immolation, NULL
  },
  /* SPL_NOSPELL  MUSS der letzte Spruch der Liste sein*/
  {
    SPL_NOSPELL, "no spell",
    NULL, NULL
  }
};

static boolean
chaosgate_valid(const connection * b)
{
  const attrib * a = a_findc(b->from->attribs, &at_direction);
  if (!a) a = a_findc(b->to->attribs, &at_direction);
  if (!a) return false;
  return true;
}

struct region *
chaosgate_move(const connection * b, struct unit * u, struct region * from, struct region * to, boolean routing)
{
  if (!routing) {
    int maxhp = u->hp / 4;
    if (maxhp<u->number) maxhp = u->number;
    u->hp = maxhp;
  }
  return to;
}

border_type bt_chaosgate = {
  "chaosgate", VAR_NONE,
  b_transparent, /* transparent */
  NULL, /* init */
  NULL, /* destroy */
  NULL, /* read */
  NULL, /* write */
  b_blocknone, /* block */
  NULL, /* name */
  b_rinvisible, /* rvisible */
  b_finvisible, /* fvisible */
  b_uinvisible, /* uvisible */
  chaosgate_valid,
  chaosgate_move
};

static void
set_spelldata_i(spell * sp, spelldata * data)
{
  sp->sp_function = data->sp_function;
  sp->patzer = data->patzer;
}

void
set_spelldata(spell * sp)
{
  int i;
  for (i=0;spelldaten[i].id!=SPL_NOSPELL;++i) {
    spelldata * data = spelldaten+i;
    if (strcmp(data->sname, sp->sname)==0) {
      set_spelldata_i(sp, data);
      return;
    }
  }
  log_error(("unknown spell %s.\n", sp->sname));
  assert(!"there is no spell by that name");
}

void
register_spells(void)
{
  at_register(&at_cursewall);
  at_register(&at_unitdissolve);
  at_register(&at_wdwpyramid);

  register_bordertype(&bt_firewall);
  register_bordertype(&bt_wisps);
  register_bordertype(&bt_chaosgate);
  
  /* sp_summon_alp */
  register_alp();
  /* init_firewall(); */
  ct_register(&ct_firewall);
  ct_register(&ct_deathcloud);

  at_register(&at_deathcloud_compat);
  register_unitcurse();
  register_regioncurse();
  register_shipcurse();
  register_buildingcurse();
  register_function((pf_generic)&sp_blessedharvest, "cast_blessedharvest");
  register_function((pf_generic)&sp_wdwpyramid, "wdwpyramid");
  register_function((pf_generic)&sp_summon_familiar, "cast_familiar");
}
