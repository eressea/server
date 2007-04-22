/* vi: set ts=2:
 *
 * Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include "eressea.h"
#include "unitcurse.h"

/* kernel includes */
#include <kernel/message.h>
#include <kernel/race.h>
#include <kernel/skill.h>
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/objtypes.h>
#include <kernel/curse.h>

/* util includes */
#include <util/nrmessage.h>
#include <util/message.h>
#include <util/base36.h>
#include <util/functions.h>

/* libc includes */
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* ------------------------------------------------------------- */
/*
 * C_AURA
 */
/* erhöht/senkt regeneration und maxaura um effect% */
static message *
cinfo_auraboost(const void * obj, typ_t typ, const curse *c, int self)
{
  struct unit *u;
  unused(typ);
  assert(typ == TYP_UNIT);
  u = (struct unit *)obj;

  if (self != 0){
    if (curse_geteffect(c) > 100){
      sprintf(buf, "%s fühlt sich von starken magischen Energien "
        "durchströmt", u->name);
    } else {
      sprintf(buf, "%s hat Schwierigkeiten seine magischen Energien "
          "zu sammeln", u->name);
    }
    return msg_message("curseinfo::info_str", "text id", buf, c->no);
  }
  return NULL;
}
static struct curse_type ct_auraboost = {
  "auraboost",
  CURSETYP_NORM, CURSE_SPREADMODULO, (NO_MERGE),
  "Dieser Zauber greift irgendwie in die Verbindung zwischen Magier "
  "und Magischer Essenz ein. Mit positiver Ausrichtung kann er wohl "
  "wie ein Fokus für Aura wirken, jedoch genauso für das Gegenteil "
  "benutzt werden.",
  cinfo_auraboost
};

/* Magic Boost - Gabe des Chaos */
static struct curse_type ct_magicboost = {
  "magicboost",
  CURSETYP_UNIT, CURSE_SPREADMODULO|CURSE_IMMUNE, M_MEN,
  "",
  NULL
};

/* ------------------------------------------------------------- */
/*
 * C_SLAVE
 */
static message *
cinfo_slave(const void * obj, typ_t typ, const curse *c, int self)
{
  unit *u;
  unused(typ);

  assert(typ == TYP_UNIT);
  u = (unit *)obj;

  if (self != 0){
    return msg_message("curseinfo::slave_1", "unit duration id", u, c->duration, c->no);
  }
  return NULL;
}
static struct curse_type ct_slavery = { "slavery",
  CURSETYP_NORM, 0, NO_MERGE,
  "Dieser mächtige Bann scheint die Einheit ihres freien Willens "
  "zu berauben. Solange der Zauber wirkt, wird sie nur den Befehlen "
  "ihres neuen Herrn gehorchen.",
  cinfo_slave
};

/* ------------------------------------------------------------- */
/*
 * C_CALM
 */
static message *
cinfo_calm(const void * obj, typ_t typ, const curse *c, int self)
{
  unused(typ);
  assert(typ == TYP_UNIT);
  
  if (c->magician && c->magician->faction) {
    faction *f = c->magician->faction;
    unit *u = (unit *)obj;

    if (f==NULL || self == 0) {
      const struct race * rc = c->magician->irace;
      return msg_message("curseinfo::calm_0", "unit race id", u, rc, c->no);
    }
    return msg_message("curseinfo::calm_1", "unit faction id", u, f, c->no);
  }
  return NULL;
}

static struct curse_type ct_calmmonster = {
  "calmmonster",
  CURSETYP_NORM, CURSE_SPREADNEVER|CURSE_ONLYONE, NO_MERGE,
  "Dieser Beeinflussungszauber scheint die Einheit einem ganz "
  "bestimmten Volk wohlgesonnen zu machen.",
  cinfo_calm
};

/* ------------------------------------------------------------- */
/*
 * C_SPEED
 */
static message *
cinfo_speed(const void * obj, typ_t typ, const curse *c, int self)
{
  unused(typ);
  assert(typ == TYP_UNIT);

  if (self != 0){
    unit *u = (unit *)obj;
    curse_unit * cu = (curse_unit *)c->data.v;
    return msg_message("curseinfo::speed_1", "unit number duration id", u, cu->cursedmen, c->duration, c->no);
  }
  return NULL;
}
static struct curse_type ct_speed = {
  "speed",
  CURSETYP_UNIT, CURSE_SPREADNEVER, M_MEN,
  "Diese Einheit bewegt sich doppelt so schnell.",
  cinfo_speed
};

/* ------------------------------------------------------------- */
/*
 * C_ORC
 */
message *
cinfo_unit(const void * obj, typ_t typ, const curse *c, int self)
{
  unused(typ);
  assert(typ == TYP_UNIT);

  if (self != 0){
    unit * u = (unit *)obj;
    return msg_message(mkname("curseinfo", c->type->cname), "unit id", u, c->no);
  }
  return NULL;
}

static struct curse_type ct_orcish = {
  "orcish",
  CURSETYP_UNIT, CURSE_SPREADMODULO|CURSE_ISNEW, M_MEN,
  "Dieser Zauber scheint die Einheit zu 'orkisieren'. Wie bei Orks "
  "ist eine deutliche Neigung zur Fortpflanzung zu beobachten.",
  cinfo_unit
};

/* ------------------------------------------------------------- */
/*
 * C_KAELTESCHUTZ
 */
static message *
cinfo_kaelteschutz(const void * obj, typ_t typ, const curse *c, int self)
{
  unused(typ);
  assert(typ == TYP_UNIT);

  if (self != 0) {
    unit * u = (unit *)obj;
    curse_unit *cu = (curse_unit *)c->data.v;
    return msg_message("curseinfo::warmth_1", "unit number id", u, cu->cursedmen, c->no);
  }
  return NULL;
}
static struct curse_type ct_insectfur = {
  "insectfur",
  CURSETYP_UNIT, CURSE_SPREADMODULO, ( M_MEN | M_DURATION ),
  "Dieser Zauber schützt vor den Auswirkungen der Kälte.",
  cinfo_kaelteschutz
};

/* ------------------------------------------------------------- */
/*
 * C_SPARKLE
 */
static message *
cinfo_sparkle(const void * obj, typ_t typ, const curse *c, int self)
{
  const char * effects[] = {
    NULL, /* end grau*/
    "sparkle_1",
    "sparkle_2",
    NULL, /* end traum */
    "sparkle_3",
    "sparkle_4",
    NULL, /* end tybied */
    "sparkle_5",
    "sparkle_6",
    "sparkle_7",
    "sparkle_8",
    NULL, /* end cerrdor */
    "sparkle_9",
    "sparkle_10",
    "sparkle_11",
    "sparkle_12",
    NULL, /* end gwyrrd */
    "sparkle_13",
    "sparkle_14",
    "sparkle_15",
    "sparkle_16",
    "sparkle_17",
    "sparkle_18",
    NULL, /* end draig */
  };
  int m, begin=0, end=0;
  unit *u;
  unused(typ);

  assert(typ == TYP_UNIT);
  u = (unit *)obj;

  if (!c->magician || !c->magician->faction) return NULL;

  for (m=0;m!=c->magician->faction->magiegebiet;++m) {
    while (effects[end]!=NULL) ++end;
    begin = end+1;
    end = begin;
  }

  while (effects[end]!=NULL) ++end;
  if (end==begin) return NULL;
  else {
    int index = begin + curse_geteffect(c) % (end-begin);
    return msg_message(mkname("curseinfo", effects[index]), "text id", buf, c->no);
  }
}
static struct curse_type ct_sparkle = { "sparkle",
  CURSETYP_UNIT, CURSE_SPREADMODULO, ( M_MEN | M_DURATION ),
  "Dieser Zauber ist einer der ersten, den junge Magier in der Schule lernen.",
  cinfo_sparkle
};

/* ------------------------------------------------------------- */
/*
 * C_STRENGTH
 */
static struct curse_type ct_strength = { "strength",
  CURSETYP_UNIT, CURSE_SPREADMODULO, M_MEN,
  "Dieser Zauber vermehrt die Stärke der verzauberten Personen um ein "
  "vielfaches.",
  cinfo_simple
};

/* ------------------------------------------------------------- */
/*
 * C_ALLSKILLS (Alp)
 */
static struct curse_type ct_worse = {
  "worse",
  CURSETYP_UNIT, CURSE_SPREADMODULO|CURSE_NOAGE, M_MEN,
  "",
  cinfo_unit
};

/* ------------------------------------------------------------- */

/*
 * C_ITEMCLOAK
 */
static struct curse_type ct_itemcloak = {
  "itemcloak",
  CURSETYP_UNIT, CURSE_SPREADNEVER, M_DURATION,
  "Dieser Zauber macht die Ausrüstung unsichtbar.",
  cinfo_unit
};
/* ------------------------------------------------------------- */

static struct curse_type ct_fumble = {
  "fumble",
  CURSETYP_NORM, CURSE_SPREADNEVER|CURSE_ONLYONE, NO_MERGE,
  "Eine Wolke negativer Energie umgibt die Einheit.",
  cinfo_unit
};
/* ------------------------------------------------------------- */


static struct curse_type ct_oldrace = { "oldrace",
  CURSETYP_NORM, CURSE_SPREADALWAYS, NO_MERGE,
  "",
  NULL
};

static struct curse_type ct_magicresistance = { "magicresistance",
  CURSETYP_UNIT, CURSE_SPREADMODULO, M_MEN,
  "Dieser Zauber verstärkt die natürliche Widerstandskraft gegen eine "
  "Verzauberung.",
  cinfo_simple
};


/* ------------------------------------------------------------- */
/*
 * C_SKILL
 */

static int
read_skill(FILE * F, curse * c)
{
  int skill;
  if (global.data_version<CURSETYPE_VERSION) {
    int men;
    fscanf(F, "%d %d", &skill, &men);
  } else {
    fscanf(F, "%d", &skill);
  }
  c->data.i = skill;
  return 0;
}
static int
write_skill(FILE * F, const curse * c)
{
  fprintf(F, "%d ", c->data.i);
  return 0;
}

static message *
cinfo_skill(const void * obj, typ_t typ, const curse *c, int self)
{
  unused(typ);

  if (self != 0) {
    unit *u = (unit *)obj;
    int sk = c->data.i;
    return msg_message("curseinfo::skill_1", "unit skill id", u, sk, c->no);
  }
  return NULL;
}

static struct curse_type ct_skillmod = {
  "skillmod",
  CURSETYP_NORM, CURSE_SPREADMODULO, M_MEN,
  "",
  cinfo_skill,
  NULL,
  read_skill, write_skill
};

/* ------------------------------------------------------------- */
void
register_unitcurse(void)
{
  ct_register(&ct_auraboost);
  ct_register(&ct_magicboost);
  ct_register(&ct_slavery);
  ct_register(&ct_calmmonster);
  ct_register(&ct_speed);
  ct_register(&ct_orcish);
  ct_register(&ct_insectfur);
  ct_register(&ct_sparkle);
  ct_register(&ct_strength);
  ct_register(&ct_worse);
  ct_register(&ct_skillmod);
  ct_register(&ct_itemcloak);
  ct_register(&ct_fumble);
  ct_register(&ct_oldrace);
  ct_register(&ct_magicresistance);
}

