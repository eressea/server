/* vi: set ts=2:
 *
 *  Eressea PB(E)M host Copyright (C) 1998-2003
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

/* modules includes */
#include <modules/xecmd.h>

/* attributes includes */
#include <attributes/reduceproduction.h>
#include <attributes/otherfaction.h>
#include <attributes/racename.h>
#include <attributes/gm.h>

/* kernel includes */
#include "alliance.h"
#include "alchemy.h"
#include "battle.h"
#include "border.h"
#include "building.h"
#include "calendar.h"
#include "curse.h"
#include "faction.h"
#include "group.h"
#include "item.h"
#include "karma.h"
#include "magic.h"
#include "message.h"
#include "movement.h"
#include "names.h"
#include "objtypes.h"
#include "order.h"
#include "plane.h"
#include "pool.h"
#include "race.h"
#include "region.h"
#include "save.h"
#include "ship.h"
#include "skill.h"
#include "terrain.h"
#include "unit.h"

/* util includes */
#include <util/base36.h>
#include <util/crmessage.h>
#include <util/event.h>
#include <util/functions.h>
#include <util/log.h>
#include <util/rng.h>
#include <util/sql.h>
#include <util/translation.h>
#include <util/umlaut.h>
#include <util/xml.h>

/* libxml includes */
#include <libxml/tree.h>
#include <libxml/xpath.h>

/* libc includes */
#include <stdio.h>
#include <message.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <errno.h>

/* exported variables */
region  *regions;
faction *factions;
settings global;
char     buf[BUFSIZE + 1];
FILE    *logfile;
FILE    *updatelog;
const struct race * new_race[MAXRACES];
boolean sqlpatch = false;
int turn;

char *
strnzcpy(char * dst, const char *src, size_t len)
{
  strncpy(dst, src, len);
  dst[len]=0;
  return dst;
}

static attrib_type at_creator = {
  "creator"
  /* Rest ist NULL; temporäres, nicht alterndes Attribut */
};

int
NewbieImmunity(void) {
  static int value = -1;
  if (value<0) {
    const char * str = get_param(global.parameters, "NewbieImmunity");
    value = str?atoi(str):IMMUN_GEGEN_ANGRIFF;
  }
  return value;
}

static int
MaxAge(void) {
  static int value = -1;
  if (value<0) {
    const char * str = get_param(global.parameters, "MaxAge");
    value = str?atoi(str):0;
  }
  return value;
}


static int
ally_flag(const char * s)
{
  if (strcmp(s, "money")==0) return HELP_MONEY;
  if (strcmp(s, "fight")==0) return HELP_FIGHT;
  if (strcmp(s, "observe")==0) return HELP_OBSERVE;
  if (strcmp(s, "give")==0) return HELP_GIVE;
  if (strcmp(s, "guard")==0) return HELP_GUARD;
  if (strcmp(s, "stealth")==0) return HELP_FSTEALTH;
  if (strcmp(s, "travel")==0) return HELP_TRAVEL;
  return 0;
}

boolean
ExpensiveMigrants(void)
{
  int value = -1;
  if (value<0) {
    const char * str = get_param(global.parameters, "study.expensivemigrants");
    value = str?atoi(str):0;
  }
  return value;
}

int
AllianceAuto(void)
{
  static int value = -1;
  if (value<0) {
    const char * str = get_param(global.parameters, "alliance.auto");
    value = 0;
    if (str!=NULL) {
    char * sstr = strdup(str);
      char * tok = strtok(sstr, " ");
      while (tok) {
        value |= ally_flag(tok);
        tok = strtok(NULL, " ");
      }
      free(sstr);
    }
  }
  return value;
}

int
AllianceRestricted(void)
{
  static int value = -1;
  if (value<0) {
    const char * str = get_param(global.parameters, "alliance.restricted");
    value = 0;
    if (str!=NULL) {
    char * sstr = strdup(str);
      char * tok = strtok(sstr, " ");
      while (tok) {
        value |= ally_flag(tok);
        tok = strtok(NULL, " ");
      }
      free(sstr);
    }
  }
  return value;
}

int
LongHunger(const struct unit * u) {
  static int value = -1;
  if (u!=NULL) {
    if (!fval(u, UFL_HUNGER)) return false;
#ifdef NEW_DAEMONHUNGER_RULE
    if (u->race==new_race[RC_DAEMON]) return false;
#endif
  }
  if (value<0) {
    const char * str = get_param(global.parameters, "hunger.long");
    value = str?atoi(str):0;
  }
  return value;
}

int
SkillCap(skill_t sk) {
  static int value = -1;
  if (sk==SK_MAGIC) return 0; /* no caps on magic */
  if (value<0) {
    const char * str = get_param(global.parameters, "skill.maxlevel");
    value = str?atoi(str):0;
  }
  return value;
}

boolean
TradeDisabled(void) {
  static int value = -1;
  if (value<0) {
    const char * str = get_param(global.parameters, "trade.disabled");
    value = str?atoi(str):0;
  }
  return value;
}

int
NMRTimeout(void) {
  static int value = -1;
  if (value<0) {
    const char * str = get_param(global.parameters, "nmr.timeout");
    value = str?atoi(str):0;
  }
  return value;
}

race_t
old_race(const struct race * rc)
{
  race_t i;
  for (i=0;i!=MAXRACES;++i) {
    if (new_race[i]==rc) return i;
  }
  return NORACE;
}

const char *directions[MAXDIRECTIONS+2] =
{
  "northwest",
  "northeast",
  "east",
  "southeast",
  "southwest",
  "west",
  "",
  "pause"
};

const char *
dbrace(const struct race * rc)
{
  static char zText[32];
  char * zPtr = zText;
  strcpy(zText, LOC(find_locale("en"), rc_name(rc, 0)));
  while (*zPtr) {
    *zPtr = (char)(toupper(*zPtr));
    ++zPtr;
  }
  return zText;
}

const char *parameters[MAXPARAMS] =
{
  "LOCALE",
  "ALLES",
  "BAUERN",
  "BURG",
  "EINHEIT",
  "PRIVAT",
  "HINTEN",
  "KOMMANDO",
  "KRAEUTER",
  "NICHT",
  "NAECHSTER",
  "PARTEI",
  "ERESSEA",
  "PERSONEN",
  "REGION",
  "SCHIFF",
  "SILBER",
  "STRASSEN",
  "TEMPORAERE",
  "FLIEHE",
  "GEBAEUDE",
  "GIB",      /* Für HELFE */
  "KAEMPFE",
  "DURCHREISE",
  "BEWACHE",
  "ZAUBER",
  "PAUSE",
  "VORNE",
  "AGGRESSIV",
  "DEFENSIV",
  "STUFE",
  "HELFE",
  "FREMDES",
  "AURA",
  "UM",
  "BEISTAND",
  "GNADE",
  "HINTER",
  "VOR",
  "ANZAHL",
  "GEGENSTAENDE",
  "TRAENKE",
  "GRUPPE",
  "PARTEITARNUNG",
  "BAEUME",
  "XEPOTION",
  "XEBALLOON",
  "XELAEN"
};


const char *keywords[MAXKEYWORDS] =
{
  "//",
  "BANNER",
  "ARBEITEN",
  "ATTACKIEREN",
  "BEKLAUEN",
  "BELAGERE",
  "BENENNEN",
  "BENUTZEN",
  "BESCHREIBEN",
  "BETRETEN",
  "BEWACHEN",
  "BOTSCHAFT",
  "ENDE",
  "FAHREN",
  "NUMMER",
  "KRIEG",
  "FRIEDEN",
  "FOLGEN",
  "FORSCHEN",
  "GIB",
  "HELFEN",
  "KAEMPFEN",
  "KAMPFZAUBER",
  "KAUFEN",
  "KONTAKTIEREN",
  "LEHREN",
  "LERNEN",
  "LIEFERE",
  "MACHEN",
  "NACH",
  "PASSWORT",
  "REKRUTIEREN",
  "RESERVIEREN",
  "ROUTE",
  "SABOTIEREN",
  "OPTION",
  "SPIONIEREN",
  "STIRB",
  "TARNEN",
  "TRANSPORTIEREN",
  "TREIBEN",
  "UNTERHALTEN",
  "VERKAUFEN",
  "VERLASSEN",
  "VERGESSEN",
  "ZAUBERE",
  "ZEIGEN",
  "ZERSTOEREN",
  "ZUECHTEN",
  "DEFAULT",
  "URSPRUNG",
  "EMAIL",
  "MEINUNG",
  "MAGIEGEBIET",
  "PIRATERIE",
  "NEUSTART",
  "GRUPPE",
  "OPFERE",
  "BETEN",
  "SORTIEREN",
  "JIHAD",
  "GM",
  "INFO",
  "PRAEFIX",
  "SYNONYM",
  "PFLANZEN",
  "WERWESEN",
  "XONTORMIA",
  "ALLIANZ",
  "BEANSPRUCHEN",
#ifdef HEROES
  "PROMOTION",
#endif
};

const char *report_options[MAX_MSG] =
{
  "Kampf",
  "Ereignisse",
  "Bewegung",
  "Einkommen",
  "Handel",
  "Produktion",
  "Orkvermehrung",
  "Zauber",
  "",
  ""
};

const char *message_levels[ML_MAX] =
{
  "Wichtig",
  "Debug",
  "Fehler",
  "Warnungen",
  "Infos"
};

const char *options[MAXOPTIONS] =
{
  "AUSWERTUNG",
  "COMPUTER",
  "ZUGVORLAGE",
  "SILBERPOOL",
  "STATISTIK",
  "DEBUG",
  "ZIPPED",
  "ZEITUNG",        /* Option hat Sonderbehandlung! */
  "MATERIALPOOL",
  "ADRESSEN",
  "BZIP2",
  "PUNKTE",
  "SHOWSKCHANGE",
  "XML"
};

static int
allied_skillcount(const faction * f, skill_t sk)
{
  int num = 0;
  alliance * a = f->alliance;
  faction_list * members = a->members;
  while (members!=NULL) {
    num += count_skill(members->data, sk);
    members=members->next;
  }
  return num;
}

static int
allied_skilllimit(const faction * f, skill_t sk)
{
  static int value = -1;
  if (value<0) {
    const char * str = get_param(global.parameters, "alliance.skilllimit");
    value = str?atoi(str):0;
  }
  return value;
}

static void
init_maxmagicians(struct attrib *a)
{
  a->data.i = MAXMAGICIANS;
}

static attrib_type at_maxmagicians = {
  "maxmagicians",
  init_maxmagicians,
  NULL,
  NULL,
  a_writeint,
  a_readint,
  ATF_UNIQUE
};

static void
init_npcfaction(struct attrib *a)
{
  a->data.i = 1;
}


static attrib_type at_npcfaction = {
  "npcfaction",
  init_npcfaction,
  NULL,
  NULL,
  a_writeint,
  a_readint,
  ATF_UNIQUE
};

int
max_magicians(const faction * f)
{
  int m = MAXMAGICIANS;
  attrib * a;
  
  if ((a = a_find(f->attribs, &at_maxmagicians)) != NULL) {
    m = a->data.i;
  }
  if (f->race == new_race[RC_ELF]) ++m;
#ifdef KARMA_MODULE
  m += fspecial(f, FS_MAGOCRACY) * 2;
#endif /* KARMA_MODULE */
  return m;
}

int
max_skill(faction * f, skill_t sk)
{
  int     m = INT_MAX;

  if (allied_skilllimit(f, sk)) {
    if (sk!=SK_ALCHEMY && sk!=SK_MAGIC) return INT_MAX;
    if (f->alliance!=NULL) {
      int ac = listlen(f->alliance->members); /* number of factions */
      int al = allied_skilllimit(f, sk); /* limit per alliance */
      int fl = (al+ac-1)/ac; /* faction limit */
      /* the following ist _very_ weird, please examine */
      int sc = al - allied_skillcount(f, sk);
      if (sc==0) return count_skill(f, sk);
      return fl;
    }
  }
  switch (sk) {
  case SK_MAGIC:
    m = max_magicians(f);
    break;
  case SK_ALCHEMY:
    m = MAXALCHEMISTS;
    break;
  }
  return m;
}

char * g_basedir;
char * g_resourcedir;

const char *
basepath(void)
{
  if (g_basedir) return g_basedir;
  return ".";
}

const char *
resourcepath(void)
{
  static char zText[MAX_PATH];
  if (g_resourcedir) return g_resourcedir;
  return strcat(strcpy(zText, basepath()), "/res");
}

int
count_skill(faction * f, skill_t sk)
{
  int n = 0;
  unit *u;

  for (u = f->units; u; u = u->nextF) {
    if (has_skill(u, sk)) {
      if (!is_familiar(u)) n += u->number;
    }
  }
  return n;
}

int quiet = 0;

FILE *debug;

int
shipspeed (const ship * sh, const unit * u)
{
  int k = sh->type->range;
  static const curse_type * stormwind_ct, * nodrift_ct;
  static boolean init;
  attrib *a;
  curse  *c;

  if (!init) {
    init = true;
    stormwind_ct = ct_find("stormwind");
    nodrift_ct = ct_find("nodrift");
  }

  assert(u->ship==sh);
  assert(sh->type->construction->improvement==NULL); /* sonst ist construction::size nicht ship_type::maxsize */
  if (sh->size!=sh->type->construction->maxsize) return 0;

  if( curse_active(get_curse(sh->attribs, stormwind_ct)))
      k *= 2;
  if( curse_active(get_curse(sh->attribs, nodrift_ct)))
      k += 1;

  if (u->faction->race == new_race[RC_AQUARIAN]
      && u->race == new_race[RC_AQUARIAN]) {
    k += 1;
  }

  a = a_find(sh->attribs, &at_speedup);
  while (a != NULL && a->type==&at_speedup) {
    k += a->data.sa[0];
    a = a->next;
  }

  c = get_curse(sh->attribs, ct_find("shipspeedup"));
  while(c) {
    k += curse_geteffect(c);
    c  = c->nexthash;
  }

#ifdef SHIPSPEED
  k *= SHIPSPEED;
#endif

#ifdef SHIPDAMAGE
  if (sh->damage) k = (k * (sh->size * DAMAGE_SCALE - sh->damage) + sh->size * DAMAGE_SCALE- 1) / (sh->size*DAMAGE_SCALE);
#endif

  return k;
}

/* erwartete Anzahl Einheiten x 2 */
#define UMAXHASH 199999
unit *unithash[UMAXHASH];

void
uhash (unit * u)
{
  assert(!u->nexthash || !"unit ist bereits gehasht");
  u->nexthash = unithash[u->no % UMAXHASH];
  unithash[u->no % UMAXHASH] = u;
}

void
uunhash (unit * u)
{
  unit ** x = &(unithash[u->no % UMAXHASH]);
  while (*x && *x!=u) x = &(*x)->nexthash;
  assert(*x || !"unit nicht gefunden");
  *x = u->nexthash;
  u->nexthash=NULL;
}

unit *
ufindhash (int i)
{
  unit * u = unithash[i % UMAXHASH];
  while (u && u->no!=i) u = u->nexthash;
  return u;
}

#define FMAXHASH 2039
faction * factionhash[FMAXHASH];

void
fhash(faction * f)
{
  int index = f->no % FMAXHASH;
  f->nexthash = factionhash[index];
  factionhash[index] = f;
}

void
funhash(faction * f)
{
  int index = f->no % FMAXHASH;
  faction ** fp = factionhash+index;
  while (*fp && (*fp)!=f) fp = &(*fp)->nexthash;
  *fp = f->nexthash;
}

static faction *
ffindhash(int no)
{
  int index = no % FMAXHASH;
  faction * f = factionhash[index];
  while (f && f->no!=no) f = f->nexthash;
  return f;
}
/* ----------------------------------------------------------------------- */

void
stripfaction (faction * f)
{
  /* TODO: inhalt auch löschen */
  if (f->msgs) free_messagelist(f->msgs);
  while (f->battles) {
    struct bmsg * bm = f->battles;
    f->battles = bm->next;
    if (bm->msgs) free_messagelist(bm->msgs);
    free(bm);
  }

  freelist(f->allies);

  free(f->email);
  free(f->banner);
  free(f->passw);
  free(f->override);
  free(f->name);

  while (f->attribs) a_remove (&f->attribs, f->attribs);

  i_freeall(&f->items);

  freelist(f->ursprung);
  funhash(f);
}

void
verify_data(void)
{
#ifndef NDEBUG
  int lf = -1;
  faction *f;
  unit *u;
  int mage, alchemist;

  puts(" - Überprüfe Daten auf Korrektheit...");

  list_foreach(faction, factions, f) {
    mage = 0;
    alchemist = 0;
    for (u=f->units;u;u=u->nextF) {
      if (eff_skill(u, SK_MAGIC, u->region)) {
        mage += u->number;
      }
      if (eff_skill(u, SK_ALCHEMY, u->region))
        alchemist += u->number;
      if (u->number > UNIT_MAXSIZE) {
        if (lf != f->no) {
          lf = f->no;
          printf("Partei %s:\n", factionid(f));
        }
        log_warning(("Einheit %s hat %d Personen\n", unitid(u), u->number));
      }
    }
    if (f->no != 0 && ((mage > 3 && f->race != new_race[RC_ELF]) || mage > 4))
      log_error(("Partei %s hat %d Magier.\n", factionid(f), mage));
    if (alchemist > 3)
      log_error(("Partei %s hat %d Alchemisten.\n", factionid(f), alchemist));
  }
  list_next(f);
#endif
}

int
distribute(int old, int new_value, int n)
{
  int i;
  int t;
  assert(new_value <= old);

  if (old == 0)
    return 0;

  t = (n / old) * new_value;
  for (i = (n % old); i; i--)
    if (rng_int() % old < new_value)
      t++;

  return t;
}

int
change_hitpoints (unit * u, int value)
{
  int hp = u->hp;

  hp += value;

  /* Jede Person benötigt mindestens 1 HP */
  if (hp < u->number){
    if (hp < 0){ /* Einheit tot */
      hp = 0;
    }
    scale_number(u, hp);
  }
  u->hp = hp;
  return hp;
}

int
atoip(const char *s)
{
  int n;

  n = atoi (s);

  if (n < 0)
    n = 0;

  return n;
}

void
scat (const char *s)
{
  strncat (buf, s, BUFSIZE - strlen (buf));
}

void
icat (int n)
{
  char s[12];

  sprintf (s, "%d", n);
  scat (s);
}


region *
findunitregion (const unit * su)
{
#ifndef SLOW_REGION
  return su->region;
#else
  region *r;
  const unit *u;

  for (r = regions; r; r = r->next) {
    for (u = r->units; u; u = u->next) {
      if (su == u) {
  return r;
      }
    }
  }

  /* This should never happen */
  assert (!"Die unit wurde nicht gefunden");

  return (region *) NULL;
#endif
}

int
effskill(const unit * u, skill_t sk)
{
  return eff_skill(u, sk, u->region);
}

int
eff_stealth(const unit * u, const region * r)
{
  int e;

  /* Auf Schiffen keine Tarnung! */
  if (u->ship) return 0;

  e = eff_skill (u, SK_STEALTH, r);

  if (fval(u, UFL_STEALTH)) {
    int es = u_geteffstealth(u);
    if (es >=0 && es < e) return es;
  }
  return e;
}

void
scale_number (unit * u, int n)
{
  skill_t sk;
  const attrib * a;
  int remain;

  if (n == u->number) return;
  if (n && u->number) {
    int full;
    remain = ((u->hp%u->number) * (n % u->number)) % u->number;

    full = u->hp/u->number; /* wieviel kriegt jede person mindestens */
    u->hp = full * n + (u->hp-full*u->number) * n / u->number;
    assert(u->hp>=0);
    if ((rng_int() % u->number) < remain)
      ++u->hp;  /* Nachkommastellen */
  } else {
    remain = 0;
    u->hp = 0;
  }
  for (a = a_find(u->attribs, &at_effect);a && a->type==&at_effect;a=a->next) {
    effect_data * data = (effect_data *)a->data.v;
    int snew = data->value / u->number * n;
    if (n) {
      remain = data->value - snew / n * u->number;
      snew += remain * n / u->number;
      remain = (remain * n) % u->number;
      if ((rng_int() % u->number) < remain)
        ++snew; /* Nachkommastellen */
    }
    data->value = snew;
  }
  if (u->number==0 || n==0) {
    for (sk = 0; sk < MAXSKILLS; sk++) {
      remove_skill(u, sk);
    }
  }

  set_number(u, n);
}

boolean
unit_has_cursed_item(unit *u)
{
  item * itm = u->items;
  while (itm) {
    if (fval(itm->type, ITF_CURSED) && itm->number>0) return true;
    itm=itm->next;
  }
  return false;
}

static void
init_gms(void)
{
  faction * f;

  for (f=factions;f;f=f->next) {
    const attrib * a = a_findc(f->attribs, &at_gm);

    if (a!=NULL) fset(f, FFL_GM);
  }
}

static int
autoalliance(const plane * pl, const faction * sf, const faction * f2)
{
  static boolean init = false;
  if (!init) {
    init_gms();
    init = true;
  }
  if (pl && (pl->flags & PFL_FRIENDLY)) return HELP_ALL;
  /* if f2 is a gm in this plane, everyone has an auto-help to it */
  if (fval(f2, FFL_GM)) {
    attrib * a = a_find(f2->attribs, &at_gm);

    while (a) {
      const plane * p = (const plane*)a->data.v;
      if (p==pl) return HELP_ALL;
      a=a->next;
    }
  }

  if (sf->alliance!=NULL && AllianceAuto()) {
    if (sf->alliance==f2->alliance) return AllianceAuto();
  }

  return 0;
}

static int
ally_mode(const ally * sf, int mode)
{
  if (sf==NULL) return 0;
  return sf->status & mode;
}

int
alliedgroup(const struct plane * pl, const struct faction * f,
            const struct faction * f2, const struct ally * sf, int mode)
{
  while (sf && sf->faction!=f2) sf=sf->next;
  if (sf==NULL) {
    mode = mode & autoalliance(pl, f, f2);
  }
  mode = ally_mode(sf, mode) | (mode & autoalliance(pl, f, f2));
  if (AllianceRestricted()) {
    if (a_findc(f->attribs, &at_npcfaction)) {
      return mode;
    }
    if (a_findc(f2->attribs, &at_npcfaction)) {
      return mode;
    }
    if (f->alliance!=f2->alliance) {
      mode &= ~AllianceRestricted();
    }
  }
  return mode;
}

int
alliedfaction(const struct plane * pl, const struct faction * f,
              const struct faction * f2, int mode)
{
  return alliedgroup(pl, f, f2, f->allies, mode);
}

/* Die Gruppe von Einheit u hat helfe zu f2 gesetzt. */
int
alliedunit(const unit * u, const faction * f2, int mode)
{
  ally * sf;
  const plane * pl = getplane(u->region);
  int automode;

  if (u->faction == f2) return mode;
  if (u->faction == NULL || f2==NULL) return 0;

  automode = mode & autoalliance(pl, u->faction, f2);

  if (pl!=NULL && (pl->flags & PFL_NOALLIANCES))
    mode = (mode & automode) | (mode & HELP_GIVE);

  sf = u->faction->allies;
  if (fval(u, UFL_GROUP)) {
    const attrib * a = a_findc(u->attribs, &at_group);
    if (a!=NULL) sf = ((group*)a->data.v)->allies;
  }
  return alliedgroup(pl, u->faction, f2, sf, mode);
}

boolean
seefaction(const faction * f, const region * r, const unit * u, int modifier)
{
  if (((f == u->faction) || !fval(u, UFL_PARTEITARNUNG)) && cansee(f, r, u, modifier))
    return true;
  return false;
}

boolean
cansee(const faction * f, const region * r, const unit * u, int modifier)
  /* r kann != u->region sein, wenn es um durchreisen geht */
  /* und es muss niemand aus f in der region sein, wenn sie vom Turm
   * erblickt wird */
{
  int stealth, rings;
  unit *u2 = r->units;
  static const item_type * itype_grail;
  static boolean init;

  if (!init) {
    init = true;
    itype_grail = it_find("grail");
  }

  if (u->faction == f || omniscient(f)) {
    return true;
  } else if (u->race == new_race[RC_SPELL]) {
    return false;
  } else if (u->number == 0) {
    attrib *a = a_find(u->attribs, &at_creator);
    if (a) {  /* u is an empty temporary unit. In this special case
               we look at the creating unit. */
      u = (unit *)a->data.v;
    } else {
      return false;
    }
  }

  if (leftship(u)) return true;
  if (itype_grail!=NULL && i_get(u->items, itype_grail)) return true;

  while (u2 && u2->faction != f) u2 = u2->next;
  if (u2==NULL) return false;

  /* simple visibility, just gotta have a unit in the region to see 'em */
  if (getguard(u) || usiege(u) || u->building || u->ship) {
    return true;
  }

  stealth = eff_stealth(u, r) - modifier;

  rings = invisible(u, NULL);
  while (u2) {
    if (rings<u->number || invisible(u, u2) < u->number) {
      int observation = eff_skill(u2, SK_OBSERVATION, r);

#ifdef NIGHTEYES
      if (u2->enchanted == SP_NIGHT_EYES && o < NIGHT_EYE_TALENT)
        observation = NIGHT_EYE_TALENT;
#endif
      if (observation >= stealth) {
        return true;
      }
    }

    /* find next unit in our faction */
    do {
      u2=u2->next;
    } while (u2 && u2->faction != f);
  }
  return false;
}


boolean
cansee_durchgezogen(const faction * f, const region * r, const unit * u, int modifier)
/* r kann != u->region sein, wenn es um durchreisen geht */
/* und es muss niemand aus f in der region sein, wenn sie vom Turm
 * erblickt wird */
{
  int n;
  boolean cansee = false;
  unit *u2;
  if (u->race == new_race[RC_SPELL] || u->number == 0) return false;
  else if (u->faction == f) cansee = true;
  else {

    if (getguard(u) || usiege(u) || u->building || u->ship) {
      cansee = true;
    }

    n = eff_stealth(u, r) - modifier;
    if (n<=0) {
      cansee = true;
    }

    for (u2 = r->units; !cansee && u2; u2 = u2->next){
      if (u2->faction == f) {
        int o;

        if (invisible(u, u2) >= u->number) continue;

        o = eff_skill(u2, SK_OBSERVATION, r);

#ifdef NIGHTEYES
        if (u2->enchanted == SP_NIGHT_EYES && o < NIGHT_EYE_TALENT)
          o = NIGHT_EYE_TALENT;
#endif
        if (o >= n) {
          cansee = true;
        }
      }
    }
    if (getguard(u) || usiege(u) || u->building || u->ship) {
      cansee = true;
    }
  }
  return cansee;
}

#ifndef NDEBUG
const char *
strcheck (const char *s, size_t maxlen)
{
  static char buffer[16 * 1024];
  if (strlen(s) > maxlen) {
    assert(maxlen < 16 * 1024);
    log_warning(("[strcheck] String wurde auf %d Zeichen verkürzt:\n%s\n",
        (int)maxlen, s));
    strnzcpy(buffer, s, maxlen);
    return buffer;
  }
  return s;
}
#endif

static attrib_type at_lighthouse = {
  "lighthouse"
  /* Rest ist NULL; temporäres, nicht alterndes Attribut */
};

/* update_lighthouse: call this function whenever the size of a lighthouse changes
 * it adds temporary markers to the surrounding regions.
 * The existence of markers says nothing about the quality of the observer in
 * the lighthouse, for this may change more frequently.
 */
void
update_lighthouse(building * lh)
{
  static boolean init_lighthouse = false;
  static const struct building_type * bt_lighthouse = 0;

  if (!init_lighthouse) {
    bt_lighthouse = bt_find("lighthouse");
    if (bt_lighthouse==NULL) return;
    init_lighthouse = true;
  }

  if (lh->type==bt_lighthouse) {
    region * r = lh->region;
    short d = (short)log10(lh->size) + 1;
    short x;

    for (x=-d;x<=d;++x) {
      short y;
      for (y=-d;y<=d;++y) {
        attrib * a;
        region * r2 = findregion(x+r->x, y+r->y);
        if (r2==NULL) continue;
        if (!fval(r2->terrain, SEA_REGION)) continue;
        if (distance(r, r2) > d) continue;
        a = a_find(r2->attribs, &at_lighthouse);
        while (a && a->type==&at_lighthouse) {
          building * b = (building*)a->data.v;
          if (b==lh) break;
          a = a->next;
        }
        if (!a) {
          a = a_add(&r2->attribs, a_new(&at_lighthouse));
          a->data.v = (void*)lh;
        }
      }
    }
  }

}

int
count_all(const faction * f)
{
#ifndef NDEBUG
  int n = 0;
  unit *u;
  for (u=f->units;u;u=u->nextF) {
    if (playerrace(u->race)) {
      n += u->number;
      assert(f==u->faction);
    }
  }
  if (f->num_people != n) {
    log_error(("Anzahl Personen für (%s) ist != num_people: %d statt %d.\n",
      factionid(f), f->num_people, n));
    return n;
  }
#endif
  return f->num_people;
}

int
count_migrants (const faction * f)
{
  unit *u = f->units;
  int n = 0;
  while (u) {
    assert(u->faction == f);
    if (u->race != f->race && u->race != new_race[RC_ILLUSION] && u->race != new_race[RC_SPELL]
    && !!playerrace(u->race) && !(is_cursed(u->attribs, C_SLAVE, 0)))
    {
      n += u->number;
    }
    u = u->nextF;
  }
  return n;
}

int
count_maxmigrants(const faction * f)
{
  int x = 0;
  if (f->race == new_race[RC_HUMAN]) {
    int nsize = count_all(f);
    if (nsize>0) {
      x = (int)(log10(nsize / 50.0) * 20);
      if (x < 0) x = 0;
    }
  }
  return x;
}

/*------------------------------------------------------------------*/

/* GET STR, I zur Eingabe von Daten liest diese aus dem Buffer, der beim ersten
 * Aufruf inititialisiert wird? */

typedef struct parser_state {
  const unsigned char *current_token;
  char * current_cmd;
  struct parser_state * next;
} parser_state;

static parser_state * state;

void
init_tokens_str(const char * initstr)
{
  if (state==NULL) {
    state = malloc(sizeof(parser_state));
    state->current_cmd = NULL;
  }
  state->current_token = (const unsigned char *)initstr;
}

void
init_tokens(const struct order * ord)
{
  if (state==NULL) state = malloc(sizeof(parser_state));
  else if (state->current_cmd!=NULL) free(state->current_cmd);
  state->current_cmd = getcommand(ord);
  init_tokens_str(state->current_cmd);
}

void
parser_pushstate(void)
{
  parser_state * new_state = malloc(sizeof(parser_state));
  new_state->current_cmd = NULL;
  new_state->current_token = NULL;
  new_state->next = state;
  state = new_state;
}

void
parser_popstate(void)
{
  parser_state * new_state = state->next;
  if (state->current_cmd!=NULL) free(state->current_cmd);
  free(state);
  state = new_state;
}

boolean
parser_end(void)
{
  while (isspace(*state->current_token)) ++state->current_token;
  return *state->current_token == 0;
}

void
skip_token(void)
{
  char quotechar = 0;

  while (isspace(*state->current_token)) ++state->current_token;
  while (*state->current_token) {
    if (isspace(*state->current_token) && quotechar==0) {
      return;
    } else {
      switch(*state->current_token) {
        case '"':
        case '\'':
          if (*state->current_token==quotechar) return;
          quotechar = *state->current_token;
          break;
        case ESCAPE_CHAR:
          ++state->current_token;
          break;
      }
    }
    ++state->current_token;
  }
}

void
parse(keyword_t kword, int (*dofun)(unit *, struct order *), boolean thisorder)
{
  region *r;

  for (r = regions; r; r = r->next) {
    unit **up = &r->units;
    while (*up) {
      unit * u = *up;
      order ** ordp = &u->orders;
      if (thisorder) ordp = &u->thisorder;
      while (*ordp) {
        order * ord = *ordp;
        if (get_keyword(ord) == kword) {
          if (dofun(u, ord)!=0) break;
          if (u->orders==NULL) break;
        }
        if (thisorder) break;
        if (*ordp==ord) ordp=&ord->next;
      }
      if (*up==u) up=&u->next;
    }
  }
}

const char *
parse_token(const char ** str)
{
  static char lbuf[DISPLAYSIZE + 1];
  char * cursor = lbuf;
  char quotechar = 0;
  boolean escape = false;
  const unsigned char * ctoken = (const unsigned char*)*str;

  assert(ctoken);

  while (isspace(*ctoken)) ++ctoken;
  while (*ctoken && cursor-lbuf < DISPLAYSIZE) {
    if (escape) {
      *cursor++ = *ctoken++;
    } else if (isspace(*ctoken)) {
      if (quotechar==0) break;
      *cursor++ = *ctoken++;
    } else if (*ctoken=='"' || *ctoken=='\'') {
      if (*ctoken==quotechar) {
        ++ctoken;
        break;
      } else if (quotechar==0) {
        quotechar = *ctoken;
        ++ctoken;
      } else {
        *cursor++ = *ctoken++;
      }
    } else if (*ctoken==SPACE_REPLACEMENT) {
      *cursor++ = ' ';
      ++ctoken;
    } else if (*ctoken==ESCAPE_CHAR) {
      escape = true;
      ++ctoken;
    } else {
      *cursor++ = *ctoken++;
    }
  }

  *cursor = '\0';
  *str = (const char *)ctoken;
  return lbuf;
}

const char *
igetstrtoken(const char * initstr)
{
  if (initstr!=NULL) {
    init_tokens_str(initstr);
  }

  return parse_token((const char**)&state->current_token);
}

const char *
getstrtoken(void)
{
  return parse_token((const char**)&state->current_token);
}

int
geti (void)
{
  return atoip (getstrtoken ());
}

/* GET KEYWORD, SKILL, ITEM, SPELL benutzen FINDSTR - welche Item um Item eine
 * Liste durchsucht.
 *
 * FIND wird immer von GET aufgerufen. GET braucht keine Parameter, IGET braucht
 * einen String aus dem gelesen wird. In FIND stehen dann listen etc drinnen.
 * FIND kann man auch allein verwenden, wenn der string _nur_ noch das gesuchte
 * object enthaelt. Steht noch weitere info darin, sollte man GET verwenden,
 * bzw. GETI wenn die info am Anfang eines neuen Stringes steht. */

int
findstr(const char **v, const char *s, unsigned char n)
{
  int i;
  size_t ss = strlen(s);
  if (!ss)
    return -1;
  for (i = 0; i != n; i++)
    if (!strncasecmp(s, v[i], ss))
      return i;
  return -1;
}

enum {
  UT_NONE,
  UT_PARAM,
  UT_ITEM,
  UT_BUILDING,
  UT_HERB,
  UT_POTION,
  UT_MAX
};

static struct lstr {
  const struct locale * lang;
  struct tnode tokens[UT_MAX];
  struct tnode skillnames;
  struct tnode keywords;
  struct tnode races;
  struct tnode directions;
  struct tnode options;
  struct lstr * next;
} * lstrs;

static struct lstr *
get_lnames(const struct locale * lang)
{
  static struct lstr * lnames = NULL;
  static const struct locale * lastlang = NULL;

  if (lastlang!=lang || lnames==NULL) {
    lnames = lstrs;
    while (lnames && lnames->lang!=lang) lnames = lnames->next;
    if (lnames==NULL) {
      lnames = calloc(sizeof(struct lstr), 1);
      lnames->lang = lang;
      lnames->next = lstrs;
      lstrs = lnames;
    }
  }
  return lnames;
}

const struct race *
findrace(const char * s, const struct locale * lang)
{
  struct lstr * lnames = get_lnames(lang);
  variant token;

  if (findtoken(&lnames->races, s, &token)==E_TOK_SUCCESS) {
    return (const struct race *)token.v;
  }
  return NULL;
}

int
findoption(const char *s, const struct locale * lang)
{
  struct lstr * lnames = get_lnames(lang);
  variant token;

  if (findtoken(&lnames->options, s, &token)==E_TOK_SUCCESS) {
    return (direction_t)token.i;
  }
  return NODIRECTION;
}

skill_t
findskill(const char *s, const struct locale * lang)
{
  struct lstr * lnames = get_lnames(lang);
  variant token;

  if (findtoken(&lnames->skillnames, s, &token)==E_TOK_NOMATCH) return NOSKILL;
  return (skill_t)token.i;
}

keyword_t
findkeyword(const char *s, const struct locale * lang)
{
  struct lstr * lnames = get_lnames(lang);
  variant token;
#ifdef AT_PERSISTENT
  if (*s == '@') s++;
#endif
  if (findtoken(&lnames->keywords, s, &token)==E_TOK_NOMATCH) return NOKEYWORD;
  if (global.disabled[token.i]) return NOKEYWORD;
  return (keyword_t) token.i;
}

param_t
findparam(const char *s, const struct locale * lang)
{
  struct lstr * lnames = get_lnames(lang);
  const building_type * btype;
  variant token;

  if (findtoken(&lnames->tokens[UT_PARAM], s, &token)==E_TOK_NOMATCH) {
    btype = findbuildingtype(s, lang);
    if (btype!=NULL) return (param_t) P_GEBAEUDE;
    return NOPARAM;
  }
  if (token.i==P_BUILDING) return P_GEBAEUDE;
  return (param_t)token.i;
}

param_t
getparam (const struct locale * lang)
{
  return findparam (getstrtoken (), lang);
}

#ifdef FUZZY_BASE36
extern int fuzzy_hits;
boolean enable_fuzzy = false;
#endif /* FUZZY_BASE36 */

faction *
findfaction (int n)
{
  faction * f = ffindhash(n);
  return f;
#ifdef FUZZY_BASE36
  if(enable_fuzzy) {
    n = atoi(itoa36(n));
    if (n) {
      f = ffindhash(n);
      if (f) return f;
      for (f = factions; f; f = f->next) {
        if (f->no == n) {
          fhash(f);
          return (f);
        }
      }
    }
  }
  return NULL;
#endif /* FUZZY_BASE36 */
}

faction *
getfaction (void)
{
  return findfaction (getid());
}

unit *
findunitr (const region * r, int n)
{
  unit *u;

  /* findunit regional! */

  for (u = r->units; u; u = u->next)
    if (u->no == n)
      return u;

  return 0;
}

unit *findunit(int n)
{
  return findunitg(n, NULL);
}

unit *
findunitg (int n, const region * hint)
{

  /* Abfangen von Syntaxfehlern. */
  if(n <= 0)
    return NULL;

  /* findunit global! */
  hint = 0;
  return ufindhash (n);
}

unit *
getnewunit (const region * r, const faction * f)
{
  int n;
  n = getid();

  return findnewunit (r, f, n);
}

static int
read_newunitid (const faction * f, const region * r)
{
  int n;
  unit *u2;
  n = getid();
  if (n == 0)
    return -1;

  u2 = findnewunit(r, f, n);
  if (u2) return u2->no;

  return -1;
}

int
read_unitid (const faction * f, const region * r)
{
  const char * s = getstrtoken ();

  /* Da s nun nur einen string enthaelt, suchen wir ihn direkt in der
   * paramliste. machen wir das nicht, dann wird getnewunit in s nach der
   * nummer suchen, doch dort steht bei temp-units nur "temp" drinnen! */

  switch (findparam (s, f->locale)) {
  case P_TEMP:
    return read_newunitid(f, r);
  }
  if (!s || *s == 0)
    return -1;
  return atoi36(s);
}

/* exported symbol */
boolean getunitpeasants;
unit *
getunitg(const region * r, const faction * f)
{
  int n = read_unitid(f, r);

  if (n == 0) {
    getunitpeasants = 1;
    return NULL;
  }
  getunitpeasants = 0;
  if (n < 0) return 0;

  return findunit(n);
}

unit *
getunit(const region * r, const faction * f)
{
  int n = read_unitid(f, r);
  unit *u2;

  if (n == 0) {
    getunitpeasants = 1;
    return NULL;
  }
  getunitpeasants = 0;
  if (n < 0) return 0;

  u2 = findunit(n);
  if (u2!=NULL && u2->region==r) {
    /* there used to be a 'u2->flags & UFL_ISNEW || u2->number>0' condition
    * here, but it got removed because of a bug that made units disappear:
    * http://eressea.upb.de/mantis/bug_view_page.php?bug_id=0000172
    */
    return u2;
  }

  return NULL;
}

/* - String Listen --------------------------------------------- */
void
addstrlist (strlist ** SP, const char *s)
{
  strlist * slist = malloc(sizeof(strlist));
  slist->next = NULL;
  slist->s = strdup(s);
  addlist(SP, slist);
}

void
freestrlist (strlist * s)
{
  strlist *q, *p = s;
  while (p) {
    q = p->next;
    free(p->s);
    free(p);
    p = q;
  }
}

/* - Meldungen und Fehler ------------------------------------------------- */

boolean lomem = false;

/* - Namen der Strukturen -------------------------------------- */
typedef char name[OBJECTIDSIZE + 1];
static name idbuf[8];
static int nextbuf = 0;

char *
estring(const char *s)
{
  char *buf = idbuf[(++nextbuf) % 8];
  char *r;

  strcpy(buf,s);
  r = buf;

  while(*buf) {
    if(*buf == ' ') {
      *buf = '~';
    }
    buf++;
  }
  return r;
}

char *
cstring(const char *s)
{
  char *buf = idbuf[(++nextbuf) % 8];
  char *r;

  strcpy(buf,s);
  r = buf;

  while(*buf) {
    if(*buf == '~') {
      *buf = ' ';
    }
    buf++;
  }
  return r;
}

const char *
buildingname (const building * b)
{
  char *buf = idbuf[(++nextbuf) % 8];

  sprintf(buf, "%s (%s)", strcheck(b->name, NAMESIZE), itoa36(b->no));
  return buf;
}

building *
largestbuilding (const region * r, boolean img)
{
  static const building_type * btype = NULL;
  building *b, *best = NULL;
  if (!btype) btype = bt_find("castle"); /* TODO: parameter der funktion? */
  /* durch die verw. von '>' statt '>=' werden die aelteren burgen
   * bevorzugt. */

  for (b = rbuildings(r); b; b = b->next) {
    if (b->type!=btype) {
      if (img) {
        const attrib * a = a_find(b->attribs, &at_icastle);
        if (!a) continue;
        if (a->data.v != btype) continue;
      } else continue;
    }
    if (best==NULL || b->size > best->size)
      best = b;
  }
  return best;
}

const char *
unitname(const unit * u)
{
  char *ubuf = idbuf[(++nextbuf) % 8];
  sprintf(ubuf, "%s (%s)", strcheck(u->name, NAMESIZE), itoa36(u->no));
  return ubuf;
}

/* -- Erschaffung neuer Einheiten ------------------------------ */

extern faction * dfindhash(int i);

static const char* forbidden[] = { "t", "te", "tem", "temp", NULL };

int
forbiddenid(int id)
{
  static int * forbid = NULL;
  static size_t len;
  size_t i;
  if (id<=0) return 1;
  if (!forbid) {
    while (forbidden[len]) ++len;
    forbid = calloc(len, sizeof(int));
    for (i=0;i!=len;++i) {
      forbid[i] = strtol(forbidden[i], NULL, 36);
    }
  }
  for (i=0;i!=len;++i) if (id==forbid[i]) return 1;
  return 0;
}

/* ID's für Einheiten und Zauber */
int
newunitid(void)
{
  int random_unit_no;
  int start_random_no;
  random_unit_no = 1 + (rng_int() % MAX_UNIT_NR);
  start_random_no = random_unit_no;

  while (ufindhash(random_unit_no) || dfindhash(random_unit_no)
      || cfindhash(random_unit_no)
      || forbiddenid(random_unit_no))
  {
    random_unit_no++;
    if (random_unit_no == MAX_UNIT_NR + 1) {
      random_unit_no = 1;
    }
    if (random_unit_no == start_random_no) {
      random_unit_no = (int) MAX_UNIT_NR + 1;
    }
  }
  return random_unit_no;
}

int
newcontainerid(void)
{
  int random_no;
  int start_random_no;

  random_no = 1 + (rng_int() % MAX_CONTAINER_NR);
  start_random_no = random_no;

  while (findship(random_no) || findbuilding(random_no)) {
    random_no++;
    if (random_no == MAX_CONTAINER_NR + 1) {
      random_no = 1;
    }
    if (random_no == start_random_no) {
      random_no = (int) MAX_CONTAINER_NR + 1;
    }
  }
  return random_no;
}

static void
createunitid(unit *u, int id)
{
  if (id<=0 || id > MAX_UNIT_NR || ufindhash(id) || dfindhash(id) || forbiddenid(id))
    u->no = newunitid();
  else
    u->no = id;
  uhash(u);
}

unit *
createunit(region * r, faction * f, int number, const struct race * rc)
{
  assert(rc);
  return create_unit(r, f, number, rc, 0, NULL, NULL);
}

unit *
create_unit(region * r, faction * f, int number, const struct race *urace, int id, const char * dname, unit *creator)
{
  unit * u = calloc(1, sizeof(unit));
  order * deford = default_order(f->locale);

  assert(urace);
  assert(deford);
  assert(f->alive);
  u_setfaction(u, f);
  set_order(&u->thisorder, NULL);
#ifdef LASTORDER
  set_order(&u->lastorder, deford);
#else
  addlist(&u->orders, deford);
#endif
  u_seteffstealth(u, -1);
  u->race = urace;
  u->irace = urace;

  set_number(u, number);

  /* die nummer der neuen einheit muss vor name_unit generiert werden,
   * da der default name immer noch 'Nummer u->no' ist */
  createunitid(u, id);

  /* zuerst in die Region setzen, da zb Drachennamen den Regionsnamen
   * enthalten */
  move_unit(u, r, NULL);

  /* u->race muss bereits gesetzt sein, wird für default-hp gebraucht */
  /* u->region auch */
  u->hp = unit_max_hp(u) * number;

  if (!dname) {
    name_unit(u);
  }
  else set_string(&u->name, dname);
  set_string(&u->display, "");

  /* Nicht zu der Einheitenzahl zählen sollten auch alle Monster. Da
   * aber auf die MAXUNITS nur in MACHE TEMP geprüft wird, ist es egal */
  if (playerrace(u->race)) {
    f->no_units++;
  }

  if (creator) {
    attrib * a;

    /* erbt Kampfstatus */
    u->status = creator->status;

    /* erbt Gebäude/Schiff*/
    if (creator->region==r) {
      u->building = creator->building;
      u->ship = creator->ship;
    }

    /* Temps von parteigetarnten Einheiten sind wieder parteigetarnt */
    if (fval(creator, UFL_PARTEITARNUNG))
      fset(u, UFL_PARTEITARNUNG);

    /* Daemonentarnung */
    set_racename(&u->attribs, get_racename(creator->attribs));
    if (fval(u->race, RCF_SHAPESHIFT) && fval(creator->race, RCF_SHAPESHIFT)) {
      u->irace = creator->irace;
    }

    /* Gruppen */
    if (fval(creator, UFL_GROUP)) {
      a = a_find(creator->attribs, &at_group);
      if (a) {
        group * g = (group*)a->data.v;
        a_add(&u->attribs, a_new(&at_group))->data.v = g;
        fset(u, UFL_GROUP);
      }
    }
    a = a_find(creator->attribs, &at_otherfaction);
    if (a) {
      a_add(&u->attribs, make_otherfaction(get_otherfaction(a)));
    }

    a = a_add(&u->attribs, a_new(&at_creator));
    a->data.v = creator;
  }
  /* Monster sind grundsätzlich parteigetarnt */
  if(f->no <= 0) fset(u, UFL_PARTEITARNUNG);

  return u;
}

/* Setzt Default Befehle -------------------------------------- */

boolean
idle (faction * f)
{
  return (boolean) (f ? false : true);
}


int
maxworkingpeasants(const struct region * r)
{
  int i = production(r) * MAXPEASANTS_PER_AREA
    - ((rtrees(r,2)+rtrees(r,1)/2) * TREESIZE);
  return max(i, 0);
}

unit_list *
get_lighthouses(const region * r)
{
  attrib * a;
  unit_list * ulist = NULL;

  if (!fval(r->terrain, SEA_REGION)) return NULL;

  for (a = a_find(r->attribs, &at_lighthouse);a && a->type==&at_lighthouse;a=a->next) {
    building *b = (building *)a->data.v;
    region *r2 = b->region;

    if (fval(b, BLD_WORKING) && b->size >= 10) {
      boolean c = false;
      unit *u;
      int d = distance(r, r2);
      int maxd = (int)log10(b->size) + 1;

      if (maxd < d) break;

      for (u = r2->units; u; u = u->next) {
        if (u->building == b) {
          c = true;
          if (c > buildingcapacity(b)) break;
          if (eff_skill(u, SK_OBSERVATION, r) >= d * 3) {
            unitlist_insert(&ulist, u);
          }
        } else if (c) break; /* first unit that's no longer in the house ends the search */
      }
    }
  }
  return ulist;
}

int
lighthouse_range(const building * b, const faction * f)
{
  int d = 0;
  if (fval(b, BLD_WORKING) && b->size >= 10) {
    region * r = b->region;
    unit *u;
    int maxd = (int)log10(b->size) + 1;
    int c = 0;

    for (u = r->units; u; u = u->next) {
      if (u->building == b) {
        c += u->number;
        if (c > buildingcapacity(b)) break;
        if (f==NULL || u->faction == f) {
          int sk = eff_skill(u, SK_OBSERVATION, r) / 3;
          d = max(d, sk);
          d = min(maxd, d);
          if (d==maxd) break;
        }
      } else if (c) break; /* first unit that's no longer in the house ends the search */
    }
  }
  return d;
}

boolean
check_leuchtturm(region * r, faction * f)
{
  attrib * a;

  if (!fval(r->terrain, SEA_REGION)) return false;

  for (a = a_find(r->attribs, &at_lighthouse);a && a->type==&at_lighthouse;a=a->next) {
    building *b = (building *)a->data.v;
    region *r2 = b->region;

    assert(b->type == bt_find("lighthouse"));
    if (fval(b, BLD_WORKING) && b->size >= 10) {
      int c = 0;
      unit *u;
      int d = 0;
      int maxd = (int)log10(b->size) + 1;

      for (u = r2->units; u; u = u->next) {
        if (u->building == b) {
          c += u->number;
          if (c > buildingcapacity(b)) break;
          if (f==NULL || u->faction == f) {
            if (!d) d = distance(r, r2);
            if (maxd < d) break;
            if (eff_skill(u, SK_OBSERVATION, r) >= d * 3) return true;
          }
        } else if (c) break; /* first unit that's no longer in the house ends the search */
      }
    }
  }

  return false;
}

region *
lastregion (faction * f)
{
#ifdef SMART_INTERVALS
  unit * u = f->units;
  region *r = f->last;

  if (u==NULL) return NULL;
  if (r!=NULL) return r->next;

  /* it is safe to start in the region of the first unit. */
  f->last = u->region;
  /* if regions have indices, we can skip ahead: */
  for (u=u->nextF; u!=NULL; u=u->nextF) {
    r = u->region;
    if (r->index > f->last->index) f->last = r;
  }

  /* we continue from the best region and look for travelthru etc. */
  for (r = f->last->next; r; r = r->next) {
    plane * p = rplane(r);

    /* search the region for travelthru-attributes: */
    if (fval(r, RF_TRAVELUNIT)) {
      attrib * ru = a_find(r->attribs, &at_travelunit);
      while (ru && ru->type==&at_travelunit) {
        u = (unit*)ru->data.v;
        if (u->faction == f) {
          f->last = r;
          break;
        }
        ru = ru->next;
      }
    }
    if (f->last == r) continue;
    if (check_leuchtturm(r, f))
      f->last = r;
    if (p && is_watcher(p, f)) {
      f->last = r;
    }
  }
  return f->last->next;
#else
  return NULL;
#endif
}

region *
firstregion (faction * f)
{
#ifdef SMART_INTERVALS
  region *r = f->first;

  if (f->units==NULL) return NULL;
  if (r!=NULL) return r;

  return f->first = regions;
#else
  return regions;
#endif
}

void
update_intervals(void)
{
#ifdef SMART_INTERVALS
  region *r;
  for (r = regions; r; r = r->next) {
    plane * p = rplane(r);

    if (p!=NULL) {
      struct watcher * w = p->watchers;
      while (w) {
        if (w->faction!=NULL) {
          update_interval(w->faction, r);
        }
        w = w->next;
      }
    }
  }
#endif
}

void ** blk_list[1024];
int list_index;
int blk_index;

static void
gc_done(void)
{
  int i, k;
  for (i=0;i!=list_index;++i)
  {
    for (k=0;k!=1024;++k) free(blk_list[i][k]);
    free(blk_list[i]);
  }
  for (k=0;k!=blk_index;++k) free(blk_list[list_index][k]);
  free(blk_list[list_index]);

}

void *
gc_add(void * p)
{
  if (blk_index==0) {
    blk_list[list_index] = (void**)malloc(1024 * sizeof(void*));
  }
  blk_list[list_index][blk_index] = p;
  blk_index = (blk_index+1) % 1024;
  if (!blk_index) ++ list_index;
  return p;
}

void
use_birthdayamulet(region * r, unit * magician, order * ord)
{
  region *tr;
  direction_t d;

  unused(ord);
  unused(magician);

  for(d=0;d<MAXDIRECTIONS;d++) {
    tr = rconnect(r, d);
    if(tr) addmessage(tr, 0, "Miiauuuuuu...", MSG_MESSAGE, ML_IMPORTANT);
  }

  tr = r;
  addmessage(r, 0, "Miiauuuuuu...", MSG_MESSAGE, ML_IMPORTANT);
}

static void
init_directions(tnode * root, const struct locale * lang)
{
  /* mit dieser routine kann man mehrere namen für eine direction geben,
   * das ist für die hexes ideal. */
  const struct {
    const char* name;
    int direction;
  } dirs [] = {
    { "dir_ne", D_NORTHEAST},
    { "dir_nw", D_NORTHWEST},
    { "dir_se", D_SOUTHEAST},
    { "dir_sw", D_SOUTHWEST},
    { "dir_east", D_EAST},
    { "dir_west", D_WEST},
    { "northeast", D_NORTHEAST},
    { "northwest", D_NORTHWEST},
    { "southeast", D_SOUTHEAST},
    { "southwest", D_SOUTHWEST},
    { "east", D_EAST },
    { "west",D_WEST },
    { "PAUSE", D_PAUSE },
    { NULL, NODIRECTION}
  };
  int i;
  struct lstr * lnames = get_lnames(lang);
  for (i=0; dirs[i].direction!=NODIRECTION;++i) {
    variant token;
    token.i = dirs[i].direction;
    addtoken(&lnames->directions, LOC(lang, dirs[i].name), token);
  }
}

direction_t
finddirection(const char *s, const struct locale * lang)
{
  struct lstr * lnames = get_lnames(lang);
  variant token;

  if (findtoken(&lnames->directions, s, &token)==E_TOK_SUCCESS) {
    return (direction_t)token.i;
  }
  return NODIRECTION;
}

static void
init_locale(const struct locale * lang)
{
  struct lstr * lnames = get_lnames(lang);
  variant var;
  int i;
  const struct race * rc;

  init_directions(&lnames->directions, lang);
  for (rc=races;rc;rc=rc->next) {
    var.v = (void*)rc;
    addtoken(&lnames->races, LOC(lang, rc_name(rc, 1)), var);
    addtoken(&lnames->races, LOC(lang, rc_name(rc, 0)), var);
  }
  for (i=0;i!=MAXPARAMS;++i) {
    var.i = i;
    addtoken(&lnames->tokens[UT_PARAM], LOC(lang, parameters[i]), var);
  }
  for (i=0;i!=MAXSKILLS;++i) {
    if (i!=SK_TRADE || !TradeDisabled()) {
      const char * skname = skillname((skill_t)i, lang);
      if (skname!=NULL) {
        var.i = i;
        addtoken(&lnames->skillnames, skname, var);
      }
    }
  }
  for (i=0;i!=MAXKEYWORDS;++i) {
    var.i = i;
    addtoken(&lnames->keywords, LOC(lang, keywords[i]), var);
  }
  for (i=0;i!=MAXOPTIONS;++i) {
    var.i = i;
    addtoken(&lnames->options, LOC(lang, options[i]), var);
  }
}

typedef struct param {
  struct param * next;
  char * name;
  char * data;
} param;

const char *
get_param(const struct param * p, const char * key)
{
  while (p!=NULL) {
    if (strcmp(p->name, key)==0) return p->data;
    p = p->next;
  }
  return NULL;
}


void
set_param(struct param ** p, const char * key, const char * data)
{
  while (*p!=NULL) {
    if (strcmp((*p)->name, key)==0) {
      free((*p)->data);
      (*p)->data = strdup(data);
      return;
    }
    p=&(*p)->next;
  }
  *p = malloc(sizeof(param));
  (*p)->name = strdup(key);
  (*p)->data = strdup(data);
  (*p)->next = NULL;
}

void
kernel_done(void)
{
  /* calling this function releases memory assigned to static variables, etc.
   * calling it is optional, e.g. a release server will most likely not do it.
   */
  translation_done();
  skill_done();
  gc_done();
  sql_done();
}

const char * localenames[] = {
  "de", "en",
  NULL
};

int
init_data(const char * filename)
{
  int l;
  char zText[80];

  sprintf(zText, "%s/%s", resourcepath(), filename);
  l = read_xml(zText);
  if (l) return l;

  if (turn<first_turn) turn = first_turn;
  return 0;
}

void
init_locales(void)
{
  int l;
  for (l=0;localenames[l];++l) {
    const struct locale * lang = find_locale(localenames[l]);
    if (lang) init_locale(lang);
  }
}

/* TODO: soll hier weg */
extern attrib_type at_shiptrail;

attrib_type at_germs = {
  "germs",
  DEFAULT_INIT,
  DEFAULT_FINALIZE,
  DEFAULT_AGE,
  a_writeshorts,
  a_readshorts,
  ATF_UNIQUE
};

/*********************/
/*   at_guard   */
/*********************/
attrib_type at_guard = {
  "guard",
  DEFAULT_INIT,
  DEFAULT_FINALIZE,
  DEFAULT_AGE,
  a_writeint,
  a_readint,
  ATF_UNIQUE
};

void
setguard(unit * u, unsigned int flags)
{
  /* setzt die guard-flags der Einheit */
  attrib * a = NULL;
  if (fval(u, UFL_GUARD)) {
    a = a_find(u->attribs, &at_guard);
  }
  if (flags == GUARD_NONE) {
    freset(u, UFL_GUARD);
    if (a) a_remove(&u->attribs, a);
    return;
  }
  fset(u, UFL_GUARD);
  if (!a) a = a_add(&u->attribs, a_new(&at_guard));
  a->data.i = (int)flags;
}

unsigned int
getguard(const unit * u)
{

  if (!fval(u->region->terrain, LAND_REGION)) return GUARD_NONE;
  if (fval(u, UFL_GUARD)) {
    attrib * a = a_find(u->attribs, &at_guard);
    if (a) return (unsigned int)a->data.i;
  }
  return GUARD_NONE;
}

#ifndef HAVE_STRDUP
char *
strdup(const char *s)
{
  return strcpy((char*)malloc(sizeof(char)*(strlen(s)+1)), s);
}
#endif

void
remove_empty_factions(boolean writedropouts)
{
  faction **fp, *f3;
  FILE *dofp = NULL;
  char zText[MAX_PATH];
  sprintf(zText, "%s/dropouts.%d", basepath(), turn);

  if (writedropouts) dofp = fopen(zText, "w");

  for (fp = &factions; *fp;) {
    faction * f = *fp;
    /* monster (0) werden nicht entfernt. alive kann beim readgame
     * () auf 0 gesetzt werden, wenn monsters keine einheiten mehr
     * haben. */
    if ((f->units==NULL || f->alive == 0) && f->no != MONSTER_FACTION) {
      ursprung * ur = f->ursprung;
      while (ur && ur->id!=0) ur=ur->next;
      if (!quiet) printf("\t%s\n", factionname(f));

      /* Einfach in eine Datei schreiben und später vermailen */

      if (dofp) {
        fprintf(dofp, "%3d %s %s %s\n",
          f->age, LOC(default_locale, rc_name(f->race, 0)), f->email,
          factionname(f));
      }
      if (updatelog) fprintf(updatelog, "dropout %s\n", itoa36(f->no));

      for (f3 = factions; f3; f3 = f3->next) {
        ally * sf;
        group * g;
        ally ** sfp = &f3->allies;
        while (*sfp) {
          sf = *sfp;
          if (sf->faction == f || sf->faction == NULL) {
            *sfp = sf->next;
            free(sf);
          }
          else sfp = &(*sfp)->next;
        }
        for (g = f3->groups; g; g=g->next) {
          sfp = &g->allies;
          while (*sfp) {
            sf = *sfp;
            if (sf->faction == f || sf->faction == NULL) {
              *sfp = sf->next;
              free(sf);
            }
            else sfp = &(*sfp)->next;
          }
        }
      }
      if (f->subscription) {
        sql_print(("UPDATE subscriptions set status='DEAD' where id=%u\n;",
                   f->subscription));
      }

      *fp = f->next;
    }
    else fp = &(*fp)->next;
  }

  if (dofp) fclose(dofp);
}

void
remove_empty_units_in_region(region *r)
{
  unit **up = &r->units;

  while (*up) {
    unit * u = *up;
    if (MaxAge()>0) {
      faction * f = u->faction;
      if (!fval(f, FFL_NOTIMEOUT) && f->age > MaxAge()) set_number(u, 0);
    }
    if ((u->number == 0 && u->race != new_race[RC_SPELL])
      || (u->age <= 0 && u->race == new_race[RC_SPELL])) {
      destroy_unit(u);
    }
    if (*up==u) up=&u->next;
  }
}

void
remove_empty_units(void)
{
  region *r;

  for (r = regions; r; r = r->next) {
    remove_empty_units_in_region(r);
  }
}

boolean
faction_id_is_unused(int id)
{
  return findfaction(id)==NULL && id!=MONSTER_FACTION;
}

int
weight(const unit * u)
{
  int w, n = 0, in_bag = 0;
  int faerie_level = 0;

  item * itm;
  for (itm=u->items;itm;itm=itm->next) {
    w = itm->type->weight * itm->number;
    n += w;
    if( !fval(itm->type, ITF_BIG))
      in_bag += w;
  }

#ifdef KARMA_MODULE
  faerie_level = fspecial(u->faction, FS_FAERIE);
#endif /* KARMA_MODULE */

  if (faerie_level) {
    n += (u->number * u->race->weight)/(1+faerie_level);
  } else {
    n += u->number * u->race->weight;
  }

  w = get_item(u, I_BAG_OF_HOLDING) * BAGCAPACITY;
  if( w > in_bag )
    w = in_bag;
  n -= w;

  return n;
}

void
make_undead_unit(unit * u)
{
#ifdef LASTORDER
  set_order(&u->lastorder, NULL);
#endif
  free_orders(&u->orders);
  name_unit(u);
  fset(u, UFL_ISNEW);
}

void
guard(unit * u, unsigned int mask)
{
  int flags = GUARD_CREWS | GUARD_LANDING | GUARD_TRAVELTHRU | GUARD_TAX;
#if GUARD_DISABLES_PRODUCTION == 1
  flags |= GUARD_PRODUCE;
#endif
#if GUARD_DISABLES_RECRUIT == 1
  flags |= GUARD_RECRUIT;
#endif
  switch (old_race(u->race)) {
  case RC_ELF:
    if (u->faction->race != u->race) break;
    /* else fallthrough */
  case RC_TREEMAN:
    flags |= GUARD_TREES;
    break;
  case RC_IRONKEEPER:
    flags = GUARD_MINING;
    break;
  }
  setguard(u, flags & mask);
}

int
besieged(const unit * u)
{
  /* belagert kann man in schiffen und burgen werden */
  return (u
      && u->building && u->building->besieged
      && u->building->besieged >= u->building->size * SIEGEFACTOR);
}

int
lifestyle(const unit * u)
{
  static plane * astralspace = NULL;
  int need;

  if (u->faction->no == MONSTER_FACTION) return 0;

  need = maintenance_cost(u);

  if (!astralspace) {
    astralspace = getplanebyname("Astralraum");
  }

#ifndef ASTRAL_HUNGER
  /* Keinen Unterhalt im Astralraum. */
  if (getplane(u->region) == astralspace)
    return 0;
#endif
  if(u->region->planep && fval(u->region->planep, PFL_NOFEED))
    return 0;

#ifdef KARMA_MODULE
  if(fspecial(u->faction, FS_REGENERATION))
    need += 1;
  if(fspecial(u->faction, FS_ADMINISTRATOR))
    need += 1;
  if(fspecial(u->faction, FS_WYRM) && u->race == new_race[RC_WYRM])
    need *= 500;
#endif /* KARMA_MODULE */
  return need;
}

boolean
hunger(int number, unit * u)
{
  region * r = u->region;
  int dead = 0, hpsub = 0;
  int hp = u->hp / u->number;

  while (number--) {
    int dam = u->race==new_race[RC_HALFLING]?15+rng_int()%14:(13+rng_int()%12);
    if (dam >= hp) {
      ++dead;
    } else {
      hpsub += dam;
    }
  }

  if (dead) {
    /* Gestorbene aus der Einheit nehmen,
     * Sie bekommen keine Beerdingung. */
    ADDMSG(&u->faction->msgs, msg_message("starvation",
      "unit region dead live", u, r, dead, u->number-dead));

    scale_number(u, u->number - dead);
    deathcounts(r, dead);
  }
  if (hpsub > 0) {
    /* Jetzt die Schäden der nicht gestorbenen abziehen. */
    u->hp -= hpsub;
    /* Meldung nur, wenn noch keine für Tote generiert. */
    if (dead == 0) {
      /* Durch unzureichende Ernährung wird %s geschwächt */
      ADDMSG(&u->faction->msgs, msg_message("malnourish",
        "unit region", u, r));
    }
  }
  return (dead || hpsub);
}

void
plagues(region * r, boolean ismagic)
{
  int peasants;
  int i;
  int dead = 0;

  /* Seuchenwahrscheinlichkeit in % */

  if (!ismagic) {
    double mwp = max(maxworkingpeasants(r), 1);
    double prob = pow(rpeasants(r) / (mwp * wage(r, NULL, NULL) * 0.13), 4.0)
        * SEUCHE;

    if (rng_double() >= prob) return;
  }

  peasants = rpeasants(r);
  dead = (int)(0.5F + SEUCHENOPFER * peasants);
  for (i = dead; i != 0; i--) {
    if (rng_int() % 100 < HEILCHANCE && rmoney(r) >= HEILKOSTEN) {
      rsetmoney(r, rmoney(r) - HEILKOSTEN);
    } else {
      --dead;
    }
  }

  if (dead > 0) {
    message * msg = add_message(&r->msgs, msg_message("pest", "dead", dead));
    msg_release(msg);
    deathcounts(r, dead);
    rsetpeasants(r, peasants - dead);
  }
}

/* Lohn bei den einzelnen Burgstufen für Normale Typen, Orks, Bauern,
 * Modifikation für Städter. */

static const int wagetable[7][4] = {
  {10, 10, 11, -7},     /* Baustelle */
  {10, 10, 11, -5},     /* Handelsposten */
  {11, 11, 12, -3},     /* Befestigung */
  {12, 11, 13, -1},     /* Turm */
  {13, 12, 14,  0},     /* Burg */
  {14, 12, 15,  1},     /* Festung */
  {15, 13, 16,  2}      /* Zitadelle */
};

static int
default_wage(const region *r, const faction * f, const race * rc)
{
  building *b = largestbuilding(r, false);
  int      esize = 0;
  curse * c;
  int      wage;
  attrib   *a;
  const building_type *artsculpture_type = bt_find("artsculpture");
  static const curse_type * drought_ct, * blessedharvest_ct;
  static boolean init;

  if (!init) {
    init = true;
    drought_ct = ct_find("drought");
    blessedharvest_ct = ct_find("blessedharvest");
  }

  if (b!=NULL) {
    /* TODO: this reveals imaginary castles */
    esize = buildingeffsize(b, false);
  }

  if (f!=NULL) {
    int index = 0;
    if (rc==new_race[RC_ORC] || rc==new_race[RC_URUK] || rc==new_race[RC_SNOTLING]) {
      index = 1;
    }
    wage = wagetable[esize][index];
#ifdef KARMA_MODULE
    if (fspecial(f, FS_URBAN)) {
      wage += wagetable[esize][3];
    }
#endif /* KARMA_MODULE */
  } else {
    if (fval(r->terrain, SEA_REGION)) {
      wage = 11;
    } else if (fval(r, RF_ORCIFIED)) {
      wage = wagetable[esize][1];
    } else {
      wage = wagetable[esize][2];
    }
    wage += curse_geteffect(get_curse(r->attribs, blessedharvest_ct));
  }

  /* Artsculpture: Income +5 */
  for(b=r->buildings; b; b=b->next) {
    if(b->type == artsculpture_type) {
      wage += 5;
    }
  }

  /* Godcurse: Income -10 */
  if (curse_active(get_curse(r->attribs, ct_find("godcursezone")))) {
    wage = max(0,wage-10);
  }

  /* Bei einer Dürre verdient man nur noch ein Viertel  */
  if (drought_ct) {
    c = get_curse(r->attribs, drought_ct);
    if (curse_active(c)) wage /= curse_geteffect(c);
  }

  a = a_find(r->attribs, &at_reduceproduction);
  if (a) wage = (wage * a->data.sa[0])/100;

  return wage;
}

/* Gibt Arbeitslohn für entsprechende Rasse zurück, oder für
* die Bauern wenn f == NULL. */
int
wage(const region *r, const faction * f, const race * rc)
{
  if (global.functions.wage) {
    return global.functions.wage(r, f, rc);
  }
  return default_wage(r, f, rc);
}


static region *
findspecialdirection(const region *r, const char *token)
{
  attrib *a;
  spec_direction *d;

  if (strlen(token)==0) return NULL;
  for (a = a_find(r->attribs, &at_direction);a && a->type==&at_direction;a=a->next) {
    d = (spec_direction *)(a->data.v);

    if (d->active && strncasecmp(d->keyword, token, strlen(token)) == 0) {
      return findregion(d->x, d->y);
    }
  }

  return NULL;
}

#define MAINTENANCE 10
int
maintenance_cost(const struct unit * u)
{
  if (u==NULL) return MAINTENANCE;
  if (global.functions.maintenance) {
    int retval = global.functions.maintenance(u);
    if (retval>=0) return retval;
  }
  return u->race->maintenance * u->number;
}

message *
movement_error(unit * u, const char * token, order * ord, int error_code)
{
  direction_t d;
  switch (error_code) {
    case E_MOVE_BLOCKED:
      d = finddirection(token, u->faction->locale);
      return msg_message("moveblocked", "unit direction", u, d);
    case E_MOVE_NOREGION:
      return msg_feedback(u, ord, "unknowndirection", "dirname", token);
  }
  return NULL;
}

int
movewhere(const unit *u, const char * token, region * r, region** resultp)
{
  region * r2;
  direction_t d;

  if (*token == '\0') {
    *resultp = NULL;
    return E_MOVE_OK;
  }

  d = finddirection(token, u->faction->locale);
  switch (d) {
  case D_PAUSE:
    *resultp = r;
    break;

  case NODIRECTION:
    r2 = findspecialdirection(r, token);
    if (r2==NULL) {
      return E_MOVE_NOREGION;
    }
    *resultp = r2;
    break;

  default:
    r2 = rconnect(r, d);
    if (r2==NULL || move_blocked(u, r, r2)) {
      return E_MOVE_BLOCKED;
    }
    *resultp = r2;
  }
  return E_MOVE_OK;
}

boolean
move_blocked(const unit * u, const region *r, const region *r2)
{
  border * b;
  curse * c;
  static const curse_type * fogtrap_ct = NULL;

  if (r2==NULL) return true;
  b = get_borders(r, r2);
  while (b) {
    if (b->type->block && b->type->block(b, u, r)) return true;
    b = b->next;
  }

  if (fogtrap_ct==NULL) fogtrap_ct = ct_find("fogtrap");
  c = get_curse(r->attribs, fogtrap_ct);
  if (curse_active(c)) return true;
  return false;
}

void
add_income(unit * u, int type, int want, int qty)
{
  if (want==INT_MAX) want = qty;
  ADDMSG(&u->faction->msgs, msg_message("income", "unit region mode wanted amount",
    u, u->region, type, want, qty));
}

void
reorder_owners(region * r)
{
  unit ** up=&r->units, ** useek;
  building * b=NULL;
  ship * sh=NULL;
#ifndef NDEBUG
  size_t len = listlen(r->units);
#endif
  for (b = r->buildings;b;b=b->next) {
    unit ** ubegin = up;
    unit ** uend = up;

    useek = up;
    while (*useek) {
      unit * u = *useek;
      if (u->building==b) {
        unit ** insert;
        if (fval(u, UFL_OWNER)) {
          unit * nu = *ubegin;
          insert = ubegin;
          if (nu!=u && nu->building==u->building && fval(nu, UFL_OWNER)) {
            log_error(("[reorder_owners] %s hat mehrere Besitzer mit UFL_OWNER.\n", buildingname(nu->building)));
            freset(nu, UFL_OWNER);
          }
        }
        else insert = uend;
        if (insert != useek) {
          *useek = u->next; /* raus aus der liste */
          u->next = *insert;
          *insert = u;
        }
        if (insert==uend) uend=&u->next;
      }
      if (*useek==u) useek = &u->next;
    }
    up = uend;
  }

  useek=up;
  while (*useek) {
    unit * u = *useek;
    assert(!u->building);
    if (u->ship==NULL) {
      if (fval(u, UFL_OWNER)) {
        log_warning(("[reorder_owners] Einheit %s war Besitzer von nichts.\n", unitname(u)));
        freset(u, UFL_OWNER);
      }
      if (useek!=up) {
        *useek = u->next; /* raus aus der liste */
        u->next = *up;
        *up = u;
      }
      up = &u->next;
    }
    if (*useek==u) useek = &u->next;
  }

  for (sh = r->ships;sh;sh=sh->next) {
    unit ** ubegin = up;
    unit ** uend = up;

    useek = up;
    while (*useek) {
      unit * u = *useek;
      if (u->ship==sh) {
        unit ** insert;
        if (fval(u, UFL_OWNER)) {
          unit * nu = *ubegin;
          insert = ubegin;
          if (nu!=u && nu->ship==u->ship && fval(nu, UFL_OWNER)) {
            log_error(("[reorder_owners] %s hat mehrere Besitzer mit UFL_OWNER.\n", shipname(nu->ship)));
            freset(nu, UFL_OWNER);
          }
        }
        else insert = uend;
        if (insert!=useek) {
          *useek = u->next; /* raus aus der liste */
          u->next = *insert;
          *insert = u;
        }
        if (insert==uend) uend=&u->next;
      }
      if (*useek==u) useek = &u->next;
    }
    up = uend;
  }
#ifndef NDEBUG
  assert(len==listlen(r->units));
#endif
}

int
produceexp(struct unit * u, skill_t sk, int n)
{
  if (n==0 || !playerrace(u->race)) return 0;
  learn_skill(u, sk, PRODUCEEXP/30.0);
  return 0;
}

int
lovar(double xpct_x2)
{
  int n = (int)(xpct_x2 * 500)+1;
  if (n==0) return 0;
  return (rng_int() % n + rng_int() % n)/1000;
}

boolean
teure_talente (const struct unit * u)
{
  if (has_skill(u, SK_MAGIC) || has_skill(u, SK_ALCHEMY) ||
    has_skill(u, SK_TACTICS) || has_skill(u, SK_HERBALISM) ||
    has_skill(u, SK_SPY)) {
    return true;
  } else {
    return false;
  }
}
void
attrib_init(void)
{
  /* Alle speicherbaren Attribute müssen hier registriert werden */
  at_register(&at_shiptrail);
  at_register(&at_familiar);
  at_register(&at_familiarmage);
  at_register(&at_clone);
  at_register(&at_clonemage);
  at_register(&at_eventhandler);
  at_register(&at_stealth);
  at_register(&at_mage);
  at_register(&at_countdown);
  at_register(&at_curse);

  at_register(&at_seenspell);
  at_register(&at_deathcloud);

  /* neue REGION-Attribute */
  at_register(&at_direction);
  at_register(&at_moveblock);
  at_register(&at_deathcount);
  at_register(&at_chaoscount);
  at_register(&at_woodcount);

  /* neue UNIT-Attribute */
  at_register(&at_siege);
  at_register(&at_effect);
  at_register(&at_private);

  at_register(&at_icastle);
  at_register(&at_guard);
  at_register(&at_group);

#ifdef KARMA_MODULE
  at_register(&at_faction_special);
  at_register(&at_prayer_timeout);
  at_register(&at_wyrm);
  at_register(&at_jihad);
#endif /* KARMA_MODULE */

  at_register(&at_building_generic_type);
  at_register(&at_maxmagicians);
  at_register(&at_npcfaction);

  /* border-typen */
  register_bordertype(&bt_noway);
  register_bordertype(&bt_fogwall);
  register_bordertype(&bt_wall);
  register_bordertype(&bt_illusionwall);
  register_bordertype(&bt_road);
  register_bordertype(&bt_questportal);

  at_register(&at_germs);
  at_register(&at_laen); /* required for old datafiles */
#ifdef XECMD_MODULE
  at_register(&at_xontormiaexpress); /* required for old datafiles */
#endif
  at_register(&at_speedup);
  at_register(&at_building_action);
}

void
kernel_init(void)
{
  char zBuffer[MAX_PATH];
  skill_init();
  attrib_init();
  translation_init();

  if (!turn) turn = lastturn();
  if (turn == 0)
    rng_init((int)time(0));
  else
    rng_init(turn);
  if (sqlpatch) {
    sprintf(zBuffer, "%s/patch-%d.sql", datapath(), turn);
    sql_init(zBuffer);
  }
}

order *
default_order(const struct locale * lang)
{
  return parse_order(locale_string(lang, "defaultorder"), lang);
}

int
entertainmoney(const region *r)
{
  int n;

  if (is_cursed(r->attribs, C_DEPRESSION, 0)) {
    return 0;
  }

  n = rmoney(r) / ENTERTAINFRACTION;

  if (is_cursed(r->attribs, C_GENEROUS, 0)) {
    n *= get_curseeffect(r->attribs, C_GENEROUS, 0);
  }

  return n;
}

int
freadstr(FILE * F, char * start, size_t size)
{
  char * str = start;
  boolean quote = false;
  int nread = 0;
  for (;;) {
    int c = fgetc(F);

    ++nread;
    if (isspace(c)) {
      if (str==start) {
        continue;
      }
      if (!quote) {
        *str = 0;
        return nread;
      }
    }
    switch (c) {
      case EOF:
        return EOF;
      case '"':
        if (!quote && str!=start) {
          log_error(("datafile contains a \" that isn't at the start of a string.\n"));
          assert(!"datafile contains a \" that isn't at the start of a string.\n");
        }
        if (quote) {
          *str = 0;
          return nread;
        }
        quote = true;
        break;
      case '\\':
        c = fgetc(F);
        ++nread;
        switch (c) {
          case EOF:
            return EOF;
          case 'n':
            if ((size_t)(str-start+1)<size) {
              *str++ = '\n';
            }
            break;
          default:
            if ((size_t)(str-start+1)<size) {
              *str++ = (char)c;
            }
        }
        break;
      default:
        if ((size_t)(str-start+1)<size) {
          *str++ = (char)c;
        }
    }
  }
}

/** writes a quoted string to the file
 * no trailing space, since this is used to make the creport.
 */
int
fwritestr(FILE * F, const char * str)
{
  int nwrite = 0;
  fputc('\"', F);
  while (*str) {
    int c = (int)(unsigned char)*str++;
    switch (c) {
      case '"':
      case '\\':
        fputc('\\', F);
        fputc(c, F);
        nwrite+=2;
        break;
      case '\n':
        fputc('\\', F);
        fputc('n', F);
        nwrite+=2;
        break;
      default:
        fputc(c, F);
        ++nwrite;
    }
  }
  fputc('\"', F);
  return nwrite + 2;
}
