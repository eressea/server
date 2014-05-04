/*
Copyright (c) 1998-2010, Enno Rehling <enno@eressea.de>
                         Katja Zedel <katze@felidae.kn-bremen.de
                         Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#include <platform.h>
#include <kernel/config.h>

/* attributes includes */
#include <attributes/reduceproduction.h>
#include <attributes/gm.h>

/* kernel includes */
#include "alliance.h"
#include "ally.h"
#include "alchemy.h"
#include "battle.h"
#include "connection.h"
#include "building.h"
#include "calendar.h"
#include "curse.h"
#include "faction.h"
#include "group.h"
#include "item.h"
#include "magic.h"
#include "message.h"
#include "move.h"
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
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <critbit.h>
#include <util/crmessage.h>
#include <util/event.h>
#include <util/functions.h>
#include <util/language.h>
#include <util/log.h>
#include <util/lists.h>
#include <util/parser.h>
#include <quicklist.h>
#include <util/rand.h>
#include <util/rng.h>
#include <util/sql.h>
#include <util/translation.h>
#include <util/unicode.h>
#include <util/umlaut.h>
#include <util/xml.h>

/* libxml includes */
#include <libxml/tree.h>
#include <libxml/xpath.h>

/* external libraries */
#include <iniparser.h>

/* libc includes */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <errno.h>

struct settings global = {
  "Eressea",                    /* gamename */
};

bool lomem = false;
FILE *logfile;
FILE *updatelog;
const struct race *new_race[MAXRACES];
bool sqlpatch = false;
bool battledebug = false;
int turn = -1;

int NewbieImmunity(void)
{
  static int value = -1;
  static int gamecookie = -1;
  if (value < 0 || gamecookie != global.cookie) {
    gamecookie = global.cookie;
    value = get_param_int(global.parameters, "NewbieImmunity", 0);
  }
  return value;
}

bool IsImmune(const faction * f)
{
  return !fval(f, FFL_NPC) && f->age < NewbieImmunity();
}

static int MaxAge(void)
{
  static int value = -1;
  static int gamecookie = -1;
  if (value < 0 || gamecookie != global.cookie) {
    gamecookie = global.cookie;
    value = get_param_int(global.parameters, "MaxAge", 0);
  }
  return value;
}

static int ally_flag(const char *s, int help_mask)
{
  if ((help_mask & HELP_MONEY) && strcmp(s, "money") == 0)
    return HELP_MONEY;
  if ((help_mask & HELP_FIGHT) && strcmp(s, "fight") == 0)
    return HELP_FIGHT;
  if ((help_mask & HELP_GIVE) && strcmp(s, "give") == 0)
    return HELP_GIVE;
  if ((help_mask & HELP_GUARD) && strcmp(s, "guard") == 0)
    return HELP_GUARD;
  if ((help_mask & HELP_FSTEALTH) && strcmp(s, "stealth") == 0)
    return HELP_FSTEALTH;
  if ((help_mask & HELP_TRAVEL) && strcmp(s, "travel") == 0)
    return HELP_TRAVEL;
  return 0;
}

bool ExpensiveMigrants(void)
{
  static int value = -1;
  static int gamecookie = -1;
  if (value < 0 || gamecookie != global.cookie) {
    gamecookie = global.cookie;
    value = get_param_int(global.parameters, "study.expensivemigrants", 0);
  }
  return value;
}

/** Specifies automatic alliance modes.
 * If this returns a value then the bits set are immutable between alliance
 * partners (faction::alliance) and cannot be changed with the HELP command.
 */
int AllianceAuto(void)
{
  static int value = -1;
  static int gamecookie = -1;
  if (value < 0 || gamecookie != global.cookie) {
    const char *str = get_param(global.parameters, "alliance.auto");
    gamecookie = global.cookie;
    value = 0;
    if (str != NULL) {
      char *sstr = _strdup(str);
      char *tok = strtok(sstr, " ");
      while (tok) {
        value |= ally_flag(tok, -1);
        tok = strtok(NULL, " ");
      }
      free(sstr);
    }
  }
  return value & HelpMask();
}

/** Limits the available help modes
 * The bitfield returned by this function specifies the available help modes
 * in this game (so you can, for example, disable HELP GIVE globally).
 * Disabling a status will disable the command sequence entirely (order parsing
 * uses this function).
 */
int HelpMask(void)
{
  static int rule = -1;
  static int gamecookie = -1;
  if (rule < 0 || gamecookie != global.cookie) {
    const char *str = get_param(global.parameters, "rules.help.mask");
    gamecookie = global.cookie;
    rule = 0;
    if (str != NULL) {
      char *sstr = _strdup(str);
      char *tok = strtok(sstr, " ");
      while (tok) {
        rule |= ally_flag(tok, -1);
        tok = strtok(NULL, " ");
      }
      free(sstr);
    } else {
      rule = HELP_ALL;
    }
  }
  return rule;
}

int AllianceRestricted(void)
{
  static int rule = -1;
  static int gamecookie = -1;
  if (rule < 0 || gamecookie != global.cookie) {
    const char *str = get_param(global.parameters, "alliance.restricted");
    gamecookie = global.cookie;
    rule = 0;
    if (str != NULL) {
      char *sstr = _strdup(str);
      char *tok = strtok(sstr, " ");
      while (tok) {
        rule |= ally_flag(tok, -1);
        tok = strtok(NULL, " ");
      }
      free(sstr);
    }
    rule &= HelpMask();
  }
  return rule;
}

int LongHunger(const struct unit *u)
{
  static int gamecookie = -1;
  static int rule = -1;
  if (u != NULL) {
    if (!fval(u, UFL_HUNGER))
      return false;
#ifdef NEW_DAEMONHUNGER_RULE
    if (u_race(u) == new_race[RC_DAEMON])
      return false;
#endif
  }
  if (rule < 0 || gamecookie != global.cookie) {
    gamecookie = global.cookie;
    rule = get_param_int(global.parameters, "hunger.long", 0);
  }
  return rule;
}

int SkillCap(skill_t sk)
{
  static int gamecookie = -1;
  static int rule = -1;
  if (sk == SK_MAGIC)
    return 0;                   /* no caps on magic */
  if (rule < 0 || gamecookie != global.cookie) {
    gamecookie = global.cookie;
    rule = get_param_int(global.parameters, "skill.maxlevel", 0);
  }
  return rule;
}

int NMRTimeout(void)
{
  static int gamecookie = -1;
  static int rule = -1;
  if (rule < 0 || gamecookie != global.cookie) {
    gamecookie = global.cookie;
    rule = get_param_int(global.parameters, "nmr.timeout", 0);
  }
  return rule;
}

race_t old_race(const struct race * rc)
{
  race_t i;
  for (i = 0; i != MAXRACES; ++i) {
    if (new_race[i] == rc)
      return i;
  }
  return NORACE;
}

helpmode helpmodes[] = {
  {"all", HELP_ALL}
  ,
  {"money", HELP_MONEY}
  ,
  {"fight", HELP_FIGHT}
  ,
  {"observe", HELP_OBSERVE}
  ,
  {"give", HELP_GIVE}
  ,
  {"guard", HELP_GUARD}
  ,
  {"stealth", HELP_FSTEALTH}
  ,
  {"travel", HELP_TRAVEL}
  ,
  {NULL, 0}
};

const char *directions[MAXDIRECTIONS + 2] = {
  "northwest",
  "northeast",
  "east",
  "southeast",
  "southwest",
  "west",
  "",
  "pause"
};

/** Returns the English name of the race, which is what the database uses.
 */
const char *dbrace(const struct race *rc)
{
  static char zText[32];
  char *zPtr = zText;

  /* the english names are all in ASCII, so we don't need to worry about UTF8 */
  strcpy(zText, (const char *)LOC(find_locale("en"), rc_name(rc, 0)));
  while (*zPtr) {
    *zPtr = (char)(toupper(*zPtr));
    ++zPtr;
  }
  return zText;
}

const char *parameters[MAXPARAMS] = {
  "LOCALE",
  "ALLES",
  "JEDEM",
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
  "GIB",                        /* Für HELFE */
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
  "XELAEN",
  "ALLIANZ"
};

const char *keywords[MAXKEYWORDS] = {
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
  "PIRATERIE",
  "GRUPPE",
  "SORTIEREN",
  "GM",
  "INFO",
  "PRAEFIX",
  "PFLANZEN",
  "ALLIANZ",
  "BEANSPRUCHEN",
  "PROMOTION",
  "BEZAHLEN",
};

const char *report_options[MAX_MSG] = {
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

const char *message_levels[ML_MAX] = {
  "Wichtig",
  "Debug",
  "Fehler",
  "Warnungen",
  "Infos"
};

const char *options[MAXOPTIONS] = {
  "AUSWERTUNG",
  "COMPUTER",
  "ZUGVORLAGE",
  NULL,
  "STATISTIK",
  "DEBUG",
  "ZIPPED",
  "ZEITUNG",                    /* Option hat Sonderbehandlung! */
  NULL,
  "ADRESSEN",
  "BZIP2",
  "PUNKTE",
  "SHOWSKCHANGE",
  "XML"
};

static int allied_skillcount(const faction * f, skill_t sk)
{
  int num = 0;
  alliance *a = f_get_alliance(f);
  quicklist *members = a->members;
  int qi;

  for (qi = 0; members; ql_advance(&members, &qi, 1)) {
    faction *m = (faction *) ql_get(members, qi);
    num += count_skill(m, sk);
  }
  return num;
}

static int allied_skilllimit(const faction * f, skill_t sk)
{
  static int value = -1;
  if (value < 0) {
    value = get_param_int(global.parameters, "alliance.skilllimit", 0);
  }
  return value;
}

static void init_maxmagicians(struct attrib *a)
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

static void init_npcfaction(struct attrib *a)
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

int max_magicians(const faction * f)
{
  int m =
    get_param_int(global.parameters, "rules.maxskills.magic", MAXMAGICIANS);
  attrib *a;

  if ((a = a_find(f->attribs, &at_maxmagicians)) != NULL) {
    m = a->data.i;
  }
  if (f->race == new_race[RC_ELF])
    ++m;
  return m;
}

int skill_limit(faction * f, skill_t sk)
{
  int m = INT_MAX;
  int al = allied_skilllimit(f, sk);
  if (al > 0) {
    if (sk != SK_ALCHEMY && sk != SK_MAGIC)
      return INT_MAX;
    if (f_get_alliance(f)) {
      int ac = listlen(f->alliance->members);   /* number of factions */
      int fl = (al + ac - 1) / ac;      /* faction limit, rounded up */
      /* the faction limit may not be achievable because it would break the alliance-limit */
      int sc = al - allied_skillcount(f, sk);
      if (sc <= 0)
        return 0;
      return fl;
    }
  }
  if (sk == SK_MAGIC) {
    m = max_magicians(f);
  } else if (sk == SK_ALCHEMY) {
    m = get_param_int(global.parameters, "rules.maxskills.alchemy",
      MAXALCHEMISTS);
  }
  return m;
}

int count_skill(faction * f, skill_t sk)
{
  int n = 0;
  unit *u;

  for (u = f->units; u; u = u->nextF) {
    if (has_skill(u, sk)) {
      if (!is_familiar(u)) {
        n += u->number;
      }
    }
  }
  return n;
}

int verbosity = 1;

FILE *debug;

static int ShipSpeedBonus(const unit * u)
{
  static int level = -1;
  if (level == -1) {
    level =
      get_param_int(global.parameters, "movement.shipspeed.skillbonus", 0);
  }
  if (level > 0) {
    ship *sh = u->ship;
    int skl = effskill(u, SK_SAILING);
    int minsk = (sh->type->cptskill + 1) / 2;
    return (skl - minsk) / level;
  }
  return 0;
}

int shipspeed(const ship * sh, const unit * u)
{
  double k = sh->type->range;
  static const curse_type *stormwind_ct, *nodrift_ct;
  static bool init;
  attrib *a;
  curse *c;

  if (!init) {
    init = true;
    stormwind_ct = ct_find("stormwind");
    nodrift_ct = ct_find("nodrift");
  }

  assert(u->ship == sh);
  assert(sh->type->construction->improvement == NULL);  /* sonst ist construction::size nicht ship_type::maxsize */
  if (sh->size != sh->type->construction->maxsize)
    return 0;

  if (curse_active(get_curse(sh->attribs, stormwind_ct)))
    k *= 2;
  if (curse_active(get_curse(sh->attribs, nodrift_ct)))
    k += 1;

  if (u->faction->race == u_race(u)) {
    /* race bonus for this faction? */
    if (fval(u_race(u), RCF_SHIPSPEED)) {
      k += 1;
    }
  }

  k += ShipSpeedBonus(u);

  a = a_find(sh->attribs, &at_speedup);
  while (a != NULL && a->type == &at_speedup) {
    k += a->data.sa[0];
    a = a->next;
  }

  c = get_curse(sh->attribs, ct_find("shipspeedup"));
  while (c) {
    k += curse_geteffect(c);
    c = c->nexthash;
  }

#ifdef SHIPSPEED
  k *= SHIPSPEED;
#endif

#ifdef SHIPDAMAGE
  if (sh->damage)
    k =
      (k * (sh->size * DAMAGE_SCALE - sh->damage) + sh->size * DAMAGE_SCALE -
      1) / (sh->size * DAMAGE_SCALE);
#endif

  return (int)k;
}

#define FMAXHASH 2039
faction *factionhash[FMAXHASH];

void fhash(faction * f)
{
  int index = f->no % FMAXHASH;
  f->nexthash = factionhash[index];
  factionhash[index] = f;
}

void funhash(faction * f)
{
  int index = f->no % FMAXHASH;
  faction **fp = factionhash + index;
  while (*fp && (*fp) != f)
    fp = &(*fp)->nexthash;
  *fp = f->nexthash;
}

static faction *ffindhash(int no)
{
  int index = no % FMAXHASH;
  faction *f = factionhash[index];
  while (f && f->no != no)
    f = f->nexthash;
  return f;
}

/* ----------------------------------------------------------------------- */

void verify_data(void)
{
#ifndef NDEBUG
  int lf = -1;
  faction *f;
  unit *u;
  int mage, alchemist;

  if (verbosity >= 1)
    puts(" - Überprüfe Daten auf Korrektheit...");

  for (f = factions; f; f = f->next) {
    mage = 0;
    alchemist = 0;
    for (u = f->units; u; u = u->nextF) {
      if (eff_skill(u, SK_MAGIC, u->region)) {
        mage += u->number;
      }
      if (eff_skill(u, SK_ALCHEMY, u->region))
        alchemist += u->number;
      if (u->number > UNIT_MAXSIZE) {
        if (lf != f->no) {
          lf = f->no;
          log_printf(stdout, "Partei %s:\n", factionid(f));
        }
        log_warning("Einheit %s hat %d Personen\n", unitid(u), u->number);
      }
    }
    if (f->no != 0 && ((mage > 3 && f->race != new_race[RC_ELF]) || mage > 4))
      log_error("Partei %s hat %d Magier.\n", factionid(f), mage);
    if (alchemist > 3)
      log_error("Partei %s hat %d Alchemisten.\n", factionid(f), alchemist);
  }
#endif
}

int distribute(int old, int new_value, int n)
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

int change_hitpoints(unit * u, int value)
{
  int hp = u->hp;

  hp += value;

  /* Jede Person benötigt mindestens 1 HP */
  if (hp < u->number) {
    if (hp < 0) {               /* Einheit tot */
      hp = 0;
    }
    scale_number(u, hp);
  }
  u->hp = hp;
  return hp;
}

unsigned int atoip(const char *s)
{
  int n;

  n = atoi(s);

  if (n < 0)
    n = 0;

  return n;
}

region *findunitregion(const unit * su)
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
  assert(!"Die unit wurde nicht gefunden");

  return (region *) NULL;
#endif
}

int effskill(const unit * u, skill_t sk)
{
  return eff_skill(u, sk, u->region);
}

int eff_stealth(const unit * u, const region * r)
{
  int e = 0;

  /* Auf Schiffen keine Tarnung! */
  if (!u->ship && skill_enabled[SK_STEALTH]) {
    e = eff_skill(u, SK_STEALTH, r);

    if (fval(u, UFL_STEALTH)) {
      int es = u_geteffstealth(u);
      if (es >= 0 && es < e)
        return es;
    }
  }
  return e;
}

bool unit_has_cursed_item(unit * u)
{
  item *itm = u->items;
  while (itm) {
    if (fval(itm->type, ITF_CURSED) && itm->number > 0)
      return true;
    itm = itm->next;
  }
  return false;
}

static void init_gms(void)
{
  faction *f;

  for (f = factions; f; f = f->next) {
    const attrib *a = a_findc(f->attribs, &at_gm);

    if (a != NULL)
      fset(f, FFL_GM);
  }
}

static int
autoalliance(const plane * pl, const faction * sf, const faction * f2)
{
  static bool init = false;
  if (!init) {
    init_gms();
    init = true;
  }
  if (pl && (pl->flags & PFL_FRIENDLY))
    return HELP_ALL;
  /* if f2 is a gm in this plane, everyone has an auto-help to it */
  if (fval(f2, FFL_GM)) {
    attrib *a = a_find(f2->attribs, &at_gm);

    while (a) {
      const plane *p = (const plane *)a->data.v;
      if (p == pl)
        return HELP_ALL;
      a = a->next;
    }
  }

  if (f_get_alliance(sf) != NULL && AllianceAuto()) {
    if (sf->alliance == f2->alliance)
      return AllianceAuto();
  }

  return 0;
}

static int ally_mode(const ally * sf, int mode)
{
  if (sf == NULL)
    return 0;
  return sf->status & mode;
}

int
alliedgroup(const struct plane *pl, const struct faction *f,
  const struct faction *f2, const struct ally *sf, int mode)
{
  while (sf && sf->faction != f2)
    sf = sf->next;
  if (sf == NULL) {
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
    if (f->alliance != f2->alliance) {
      mode &= ~AllianceRestricted();
    }
  }
  return mode;
}

int
alliedfaction(const struct plane *pl, const struct faction *f,
  const struct faction *f2, int mode)
{
  return alliedgroup(pl, f, f2, f->allies, mode);
}

/* Die Gruppe von Einheit u hat helfe zu f2 gesetzt. */
int alliedunit(const unit * u, const faction * f2, int mode)
{
  ally *sf;
  int automode;

  assert(u->region);            /* the unit should be in a region, but it's possible that u->number==0 (TEMP units) */
  if (u->faction == f2)
    return mode;
  if (u->faction != NULL && f2 != NULL) {
    plane *pl;

    if (mode & HELP_FIGHT) {
      if ((u->flags & UFL_DEFENDER) || (u->faction->flags & FFL_DEFENDER)) {
        faction *owner = region_get_owner(u->region);
        /* helps the owner of the region */
        if (owner == f2) {
          return HELP_FIGHT;
        }
      }
    }

    pl = rplane(u->region);
    automode = mode & autoalliance(pl, u->faction, f2);

    if (pl != NULL && (pl->flags & PFL_NOALLIANCES))
      mode = (mode & automode) | (mode & HELP_GIVE);

    sf = u->faction->allies;
    if (fval(u, UFL_GROUP)) {
      const attrib *a = a_findc(u->attribs, &at_group);
      if (a != NULL)
        sf = ((group *) a->data.v)->allies;
    }
    return alliedgroup(pl, u->faction, f2, sf, mode);
  }
  return 0;
}

bool
seefaction(const faction * f, const region * r, const unit * u, int modifier)
{
  if (((f == u->faction) || !fval(u, UFL_ANON_FACTION))
    && cansee(f, r, u, modifier))
    return true;
  return false;
}

bool
cansee(const faction * f, const region * r, const unit * u, int modifier)
  /* r kann != u->region sein, wenn es um durchreisen geht */
  /* und es muss niemand aus f in der region sein, wenn sie vom Turm
   * erblickt wird */
{
  int stealth, rings;
  unit *u2 = r->units;
  static const item_type *itype_grail;
  static bool init;

  if (!init) {
    init = true;
    itype_grail = it_find("grail");
  }

  if (u->faction == f || omniscient(f)) {
    return true;
  } else if (fval(u_race(u), RCF_INVISIBLE)) {
    return false;
  } else if (u->number == 0) {
    attrib *a = a_find(u->attribs, &at_creator);
    if (a) {                    /* u is an empty temporary unit. In this special case
                                   we look at the creating unit. */
      u = (unit *) a->data.v;
    } else {
      return false;
    }
  }

  if (leftship(u))
    return true;
  if (itype_grail != NULL && i_get(u->items, itype_grail))
    return true;

  while (u2 && u2->faction != f)
    u2 = u2->next;
  if (u2 == NULL)
    return false;

  /* simple visibility, just gotta have a unit in the region to see 'em */
  if (is_guard(u, GUARD_ALL) != 0 || usiege(u) || u->building || u->ship) {
    return true;
  }

  rings = invisible(u, NULL);
  stealth = eff_stealth(u, r) - modifier;

  while (u2) {
    if (rings < u->number || invisible(u, u2) < u->number) {
      if (skill_enabled[SK_PERCEPTION]) {
        int observation = eff_skill(u2, SK_PERCEPTION, r);

        if (observation >= stealth) {
          return true;
        }
      } else {
        return true;
      }
    }

    /* find next unit in our faction */
    do {
      u2 = u2->next;
    } while (u2 && u2->faction != f);
  }
  return false;
}

bool cansee_unit(const unit * u, const unit * target, int modifier)
/* target->region kann != u->region sein, wenn es um durchreisen geht */
{
  if (fval(u_race(target), RCF_INVISIBLE) || target->number == 0)
    return false;
  else if (target->faction == u->faction)
    return true;
  else {
    int n, rings, o;

    if (is_guard(target, GUARD_ALL) != 0 || usiege(target) || target->building
      || target->ship) {
      return true;
    }

    n = eff_stealth(target, target->region) - modifier;
    rings = invisible(target, NULL);
    if (rings == 0 && n <= 0) {
      return true;
    }

    if (rings && invisible(target, u) >= target->number) {
      return false;
    }
    if (skill_enabled[SK_PERCEPTION]) {
      o = eff_skill(u, SK_PERCEPTION, target->region);
      if (o >= n) {
        return true;
      }
    } else {
      return true;
    }
  }
  return false;
}

bool
cansee_durchgezogen(const faction * f, const region * r, const unit * u,
  int modifier)
/* r kann != u->region sein, wenn es um durchreisen geht */
/* und es muss niemand aus f in der region sein, wenn sie vom Turm
 * erblickt wird */
{
  int n;
  unit *u2;

  if (fval(u_race(u), RCF_INVISIBLE) || u->number == 0)
    return false;
  else if (u->faction == f)
    return true;
  else {
    int rings;

    if (is_guard(u, GUARD_ALL) != 0 || usiege(u) || u->building || u->ship) {
      return true;
    }

    n = eff_stealth(u, r) - modifier;
    rings = invisible(u, NULL);
    if (rings == 0 && n <= 0) {
      return true;
    }

    for (u2 = r->units; u2; u2 = u2->next) {
      if (u2->faction == f) {
        int o;

        if (rings && invisible(u, u2) >= u->number)
          continue;

        o = eff_skill(u2, SK_PERCEPTION, r);

        if (o >= n) {
          return true;
        }
      }
    }
  }
  return false;
}

#ifndef NDEBUG
const char *strcheck(const char *s, size_t maxlen)
{
  static char buffer[16 * 1024];
  if (strlen(s) > maxlen) {
    assert(maxlen < 16 * 1024);
    log_warning("[strcheck] String wurde auf %d Zeichen verkürzt:\n%s\n", (int)maxlen, s);
    strlcpy(buffer, s, maxlen);
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
void update_lighthouse(building * lh)
{
  static bool init_lighthouse = false;
  static const struct building_type *bt_lighthouse = 0;

  if (!init_lighthouse) {
    bt_lighthouse = bt_find("lighthouse");
    if (bt_lighthouse == NULL)
      return;
    init_lighthouse = true;
  }

  if (lh->type == bt_lighthouse) {
    region *r = lh->region;
    int d = (int)log10(lh->size) + 1;
    int x;

    if (lh->size > 0) {
      r->flags |= RF_LIGHTHOUSE;
    }

    for (x = -d; x <= d; ++x) {
      int y;
      for (y = -d; y <= d; ++y) {
        attrib *a;
        region *r2;
        int px = r->x + x, py = r->y + y;
        pnormalize(&px, &py, rplane(r));
        r2 = findregion(px, py);
        if (r2 == NULL)
          continue;
        if (!fval(r2->terrain, SEA_REGION))
          continue;
        if (distance(r, r2) > d)
          continue;
        a = a_find(r2->attribs, &at_lighthouse);
        while (a && a->type == &at_lighthouse) {
          building *b = (building *) a->data.v;
          if (b == lh)
            break;
          a = a->next;
        }
        if (!a) {
          a = a_add(&r2->attribs, a_new(&at_lighthouse));
          a->data.v = (void *)lh;
        }
      }
    }
  }
}

int count_all(const faction * f)
{
#ifndef NDEBUG
    unit *u;
    int np = 0, n = 0;
    for (u = f->units; u; u = u->nextF) {
        assert(f == u->faction);
        n += u->number;
        if (playerrace(u_race(u))) {
            np += u->number;
        }
    }
    if (f->num_people != np) {
        log_error("# of people in %s is != num_people: %d should be %d.\n", factionid(f), f->num_people, np);
    }
    if (f->num_total != n) {
        log_error("# of men in %s is != num_total: %d should be %d.\n", factionid(f), f->num_total, n);
    }
#endif
    return (f->flags & FFL_NPC) ? f->num_total : f->num_people;
}

int count_units(const faction * f)
{
#ifndef NDEBUG
    unit *u;
    int n = 0, np = 0;
    for (u = f->units; u; u = u->nextF) {
        ++n;
        if (playerrace(u_race(u))) ++np;
    }
    n = (f->flags & FFL_NPC) ? n : np;
    if (f->no_units && n != f->no_units) {
        log_warning("# of units in %s is != no_units: %d should be %d.\n", factionid(f), f->no_units, n);
    }
#endif
    return n;
}

int count_migrants(const faction * f)
{
  unit *u = f->units;
  int n = 0;
  while (u) {
    assert(u->faction == f);
    if (u_race(u) != f->race && u_race(u) != new_race[RC_ILLUSION]
      && u_race(u) != new_race[RC_SPELL]
      && !!playerrace(u_race(u)) && !(is_cursed(u->attribs, C_SLAVE, 0))) {
      n += u->number;
    }
    u = u->nextF;
  }
  return n;
}

int count_maxmigrants(const faction * f)
{
  static int migrants = -1;

  if (migrants < 0) {
    migrants = get_param_int(global.parameters, "rules.migrants", INT_MAX);
  }
  if (migrants == INT_MAX) {
    int x = 0;
    if (f->race == new_race[RC_HUMAN]) {
      int nsize = count_all(f);
      if (nsize > 0) {
        x = (int)(log10(nsize / 50.0) * 20);
        if (x < 0)
          x = 0;
      }
    }
    return x;
  }
  return migrants;
}

void
parse(keyword_t kword, int (*dofun) (unit *, struct order *), bool thisorder)
{
  region *r;

  for (r = regions; r; r = r->next) {
    unit **up = &r->units;
    while (*up) {
      unit *u = *up;
      order **ordp = &u->orders;
      if (thisorder)
        ordp = &u->thisorder;
      while (*ordp) {
        order *ord = *ordp;
        if (get_keyword(ord) == kword) {
          if (dofun(u, ord) != 0)
            break;
          if (u->orders == NULL)
            break;
        }
        if (thisorder)
          break;
        if (*ordp == ord)
          ordp = &ord->next;
      }
      if (*up == u)
        up = &u->next;
    }
  }
}

const char *igetstrtoken(const char *initstr)
{
  if (initstr != NULL) {
    init_tokens_str(initstr, NULL);
  }

  return getstrtoken();
}

unsigned int getuint(void)
{
  return atoip((const char *)getstrtoken());
}

int getint(void)
{
  return atoi((const char *)getstrtoken());
}

const struct race *findrace(const char *s, const struct locale *lang)
{
  void **tokens = get_translations(lang, UT_RACES);
  variant token;

  assert(lang);
  if (tokens && findtoken(*tokens, s, &token) == E_TOK_SUCCESS) {
    return (const struct race *)token.v;
  }
  return NULL;
}

int findoption(const char *s, const struct locale *lang)
{
  void **tokens = get_translations(lang, UT_OPTIONS);
  variant token;

  if (findtoken(*tokens, s, &token) == E_TOK_SUCCESS) {
    return (direction_t) token.i;
  }
  return NODIRECTION;
}

skill_t findskill(const char *s, const struct locale * lang)
{
  param_t result = NOSKILL;
  char buffer[64];
  char * str = transliterate(buffer, sizeof(buffer)-sizeof(int), s);

  if (str) {
    int i;
    const void * match;
    void **tokens = get_translations(lang, UT_SKILLS);
    critbit_tree *cb = (critbit_tree *)*tokens;
    if (cb_find_prefix(cb, str, strlen(str), &match, 1, 0)) {
      cb_get_kv(match, &i, sizeof(int));
      result = (skill_t)i;
    }
  }
  return result;
}

keyword_t findkeyword(const char *s, const struct locale * lang)
{
  keyword_t result = NOKEYWORD;
  char buffer[64];

  while (*s == '@') ++s;

  if (*s) {
    char * str = transliterate(buffer, sizeof(buffer)-sizeof(int), s);

    if (str) {
      int i;
      const void * match;
      void **tokens = get_translations(lang, UT_KEYWORDS);
      critbit_tree *cb = (critbit_tree *)*tokens;
      if (cb_find_prefix(cb, str, strlen(str), &match, 1, 0)) {
        cb_get_kv(match, &i, sizeof(int));
        result = (keyword_t)i;
        return global.disabled[result] ? NOKEYWORD : result;
      }
    }
  }
  return NOKEYWORD;
}

param_t findparam(const char *s, const struct locale * lang)
{
  param_t result = NOPARAM;
  char buffer[64];
  char * str = transliterate(buffer, sizeof(buffer)-sizeof(int), s);

  if (str && *str) {
    int i;
    const void * match;
    void **tokens = get_translations(lang, UT_PARAMS);
    critbit_tree *cb = (critbit_tree *)*tokens;
    if (cb_find_prefix(cb, str, strlen(str), &match, 1, 0)) {
      cb_get_kv(match, &i, sizeof(int));
      result = (param_t)i;
    }
  }
  return result;
}

param_t findparam_ex(const char *s, const struct locale * lang)
{
  param_t result = findparam(s, lang);

  if (result==NOPARAM) {
    const building_type *btype = findbuildingtype(s, lang);
    if (btype != NULL)
      return P_GEBAEUDE;
  }
  return (result == P_BUILDING) ? P_GEBAEUDE : result;
}

bool isparam(const char *s, const struct locale * lang, param_t param)
{
  if (s[0]>'@') {
    param_t p = (param==P_GEBAEUDE) ? findparam_ex(s, lang) : findparam(s, lang);
    return p==param;
  }
  return false;
}

param_t getparam(const struct locale * lang)
{
  return findparam(getstrtoken(), lang);
}

faction *findfaction(int n)
{
  faction *f = ffindhash(n);
  return f;
}

faction *getfaction(void)
{
  return findfaction(getid());
}

unit *findunitr(const region * r, int n)
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
  if (n <= 0) {
    return NULL;
  }
  return ufindhash(n);
}

unit *findunitg(int n, const region * hint)
{

  /* Abfangen von Syntaxfehlern. */
  if (n <= 0)
    return NULL;

  /* findunit global! */
  hint = 0;
  return ufindhash(n);
}

unit *getnewunit(const region * r, const faction * f)
{
  int n;
  n = getid();

  return findnewunit(r, f, n);
}

static int read_newunitid(const faction * f, const region * r)
{
  int n;
  unit *u2;
  n = getid();
  if (n == 0)
    return -1;

  u2 = findnewunit(r, f, n);
  if (u2)
    return u2->no;

  return -1;
}

int read_unitid(const faction * f, const region * r)
{
  const char *s = getstrtoken();

  /* Da s nun nur einen string enthaelt, suchen wir ihn direkt in der
   * paramliste. machen wir das nicht, dann wird getnewunit in s nach der
   * nummer suchen, doch dort steht bei temp-units nur "temp" drinnen! */

  if (!s || *s == 0) {
    return -1;
  }
  if (isparam(s, f->locale, P_TEMP)) {
    return read_newunitid(f, r);
  }
  return atoi36((const char *)s);
}

/* exported symbol */
bool getunitpeasants;
unit *getunitg(const region * r, const faction * f)
{
  int n = read_unitid(f, r);

  if (n == 0) {
    getunitpeasants = 1;
    return NULL;
  }
  getunitpeasants = 0;
  if (n < 0)
    return 0;

  return findunit(n);
}

unit *getunit(const region * r, const faction * f)
{
  int n = read_unitid(f, r);
  unit *u2;

  if (n == 0) {
    getunitpeasants = 1;
    return NULL;
  }
  getunitpeasants = 0;
  if (n < 0)
    return 0;

  u2 = findunit(n);
  if (u2 != NULL && u2->region == r) {
    /* there used to be a 'u2->flags & UFL_ISNEW || u2->number>0' condition
     * here, but it got removed because of a bug that made units disappear:
     * http://eressea.upb.de/mantis/bug_view_page.php?bug_id=0000172
     */
    return u2;
  }

  return NULL;
}

/* - String Listen --------------------------------------------- */
void addstrlist(strlist ** SP, const char *s)
{
  strlist *slist = malloc(sizeof(strlist));
  slist->next = NULL;
  slist->s = _strdup(s);
  addlist(SP, slist);
}

void freestrlist(strlist * s)
{
  strlist *q, *p = s;
  while (p) {
    q = p->next;
    free(p->s);
    free(p);
    p = q;
  }
}

/* - Namen der Strukturen -------------------------------------- */
typedef char name[OBJECTIDSIZE + 1];
static name idbuf[8];
static int nextbuf = 0;

char *estring_i(char *ibuf)
{
  char *p = ibuf;

  while (*p) {
    if (isxspace(*(unsigned *)p) == ' ') {
      *p = '~';
    }
    ++p;
  }
  return ibuf;
}

char *estring(const char *s)
{
  char *ibuf = idbuf[(++nextbuf) % 8];

  strlcpy(ibuf, s, sizeof(name));
  return estring_i(ibuf);
}

char *cstring_i(char *ibuf)
{
  char *p = ibuf;

  while (*p) {
    if (*p == '~') {
      *p = ' ';
    }
    ++p;
  }
  return ibuf;
}

char *cstring(const char *s)
{
  char *ibuf = idbuf[(++nextbuf) % 8];

  strlcpy(ibuf, s, sizeof(name));
  return cstring_i(ibuf);
}

building *largestbuilding(const region * r, cmp_building_cb cmp_gt,
  bool imaginary)
{
  building *b, *best = NULL;

  for (b = rbuildings(r); b; b = b->next) {
    if (cmp_gt(b, best) <= 0)
      continue;
    if (!imaginary) {
      const attrib *a = a_find(b->attribs, &at_icastle);
      if (a)
        continue;
    }
    best = b;
  }
  return best;
}

char *write_unitname(const unit * u, char *buffer, size_t size)
{
  slprintf(buffer, size, "%s (%s)", (const char *)u->name, itoa36(u->no));
  buffer[size - 1] = 0;
  return buffer;
}

const char *unitname(const unit * u)
{
  char *ubuf = idbuf[(++nextbuf) % 8];
  return write_unitname(u, ubuf, sizeof(name));
}

/* -- Erschaffung neuer Einheiten ------------------------------ */

extern faction *dfindhash(int i);

static const char *forbidden[] = { "t", "te", "tem", "temp", NULL };

int forbiddenid(int id)
{
  static int *forbid = NULL;
  static size_t len;
  size_t i;
  if (id <= 0)
    return 1;
  if (!forbid) {
    while (forbidden[len])
      ++len;
    forbid = calloc(len, sizeof(int));
    for (i = 0; i != len; ++i) {
      forbid[i] = strtol(forbidden[i], NULL, 36);
    }
  }
  for (i = 0; i != len; ++i)
    if (id == forbid[i])
      return 1;
  return 0;
}

/* ID's für Einheiten und Zauber */
int newunitid(void)
{
  int random_unit_no;
  int start_random_no;
  random_unit_no = 1 + (rng_int() % MAX_UNIT_NR);
  start_random_no = random_unit_no;

  while (ufindhash(random_unit_no) || dfindhash(random_unit_no)
    || cfindhash(random_unit_no)
    || forbiddenid(random_unit_no)) {
    random_unit_no++;
    if (random_unit_no == MAX_UNIT_NR + 1) {
      random_unit_no = 1;
    }
    if (random_unit_no == start_random_no) {
      random_unit_no = (int)MAX_UNIT_NR + 1;
    }
  }
  return random_unit_no;
}

int newcontainerid(void)
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
      random_no = (int)MAX_CONTAINER_NR + 1;
    }
  }
  return random_no;
}

unit *createunit(region * r, faction * f, int number, const struct race * rc)
{
  assert(rc);
  return create_unit(r, f, number, rc, 0, NULL, NULL);
}

bool idle(faction * f)
{
  return (bool) (f ? false : true);
}

int maxworkingpeasants(const struct region *r)
{
  int i = production(r) * MAXPEASANTS_PER_AREA
    - ((rtrees(r, 2) + rtrees(r, 1) / 2) * TREESIZE);
  return _max(i, 0);
}

int lighthouse_range(const building * b, const faction * f)
{
  int d = 0;
  if (fval(b, BLD_WORKING) && b->size >= 10) {
    int maxd = (int)log10(b->size) + 1;

    if (skill_enabled[SK_PERCEPTION]) {
      region *r = b->region;
      int c = 0;
      unit *u;
      for (u = r->units; u; u = u->next) {
        if (u->building == b) {
          c += u->number;
          if (c > buildingcapacity(b))
            break;
          if (f == NULL || u->faction == f) {
            int sk = eff_skill(u, SK_PERCEPTION, r) / 3;
            d = _max(d, sk);
            d = _min(maxd, d);
            if (d == maxd)
              break;
          }
        } else if (c)
          break;                /* first unit that's no longer in the house ends the search */
      }
    } else {
      /* E3A rule: no perception req'd */
      return maxd;
    }
  }
  return d;
}

bool check_leuchtturm(region * r, faction * f)
{
  attrib *a;

  if (!fval(r->terrain, SEA_REGION))
    return false;

  for (a = a_find(r->attribs, &at_lighthouse); a && a->type == &at_lighthouse;
    a = a->next) {
    building *b = (building *) a->data.v;

    assert(b->type == bt_find("lighthouse"));
    if (fval(b, BLD_WORKING) && b->size >= 10) {
      int maxd = (int)log10(b->size) + 1;

      if (skill_enabled[SK_PERCEPTION]) {
        region *r2 = b->region;
        unit *u;
        int c = 0;
        int d = 0;

        for (u = r2->units; u; u = u->next) {
          if (u->building == b) {
            c += u->number;
            if (c > buildingcapacity(b))
              break;
            if (f == NULL || u->faction == f) {
              if (!d)
                d = distance(r, r2);
              if (maxd < d)
                break;
              if (eff_skill(u, SK_PERCEPTION, r) >= d * 3)
                return true;
            }
          } else if (c)
            break;              /* first unit that's no longer in the house ends the search */
        }
      } else {
        /* E3A rule: no perception req'd */
        return maxd;
      }
    }
  }

  return false;
}

region *lastregion(faction * f)
{
#ifdef SMART_INTERVALS
  unit *u = f->units;
  region *r = f->last;

  if (u == NULL)
    return NULL;
  if (r != NULL)
    return r->next;

  /* it is safe to start in the region of the first unit. */
  f->last = u->region;
  /* if regions have indices, we can skip ahead: */
  for (u = u->nextF; u != NULL; u = u->nextF) {
    r = u->region;
    if (r->index > f->last->index)
      f->last = r;
  }

  /* we continue from the best region and look for travelthru etc. */
  for (r = f->last->next; r; r = r->next) {
    plane *p = rplane(r);

    /* search the region for travelthru-attributes: */
    if (fval(r, RF_TRAVELUNIT)) {
      attrib *ru = a_find(r->attribs, &at_travelunit);
      while (ru && ru->type == &at_travelunit) {
        u = (unit *) ru->data.v;
        if (u->faction == f) {
          f->last = r;
          break;
        }
        ru = ru->next;
      }
    }
    if (f->last == r)
      continue;
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

region *firstregion(faction * f)
{
#ifdef SMART_INTERVALS
  region *r = f->first;

  if (f->units == NULL)
    return NULL;
  if (r != NULL)
    return r;

  return f->first = regions;
#else
  return regions;
#endif
}

void **blk_list[1024];
int list_index;
int blk_index;

static void gc_done(void)
{
  int i, k;
  for (i = 0; i != list_index; ++i) {
    for (k = 0; k != 1024; ++k)
      free(blk_list[i][k]);
    free(blk_list[i]);
  }
  for (k = 0; k != blk_index; ++k)
    free(blk_list[list_index][k]);
  free(blk_list[list_index]);

}

void *gc_add(void *p)
{
  if (blk_index == 0) {
    blk_list[list_index] = (void **)malloc(1024 * sizeof(void *));
  }
  blk_list[list_index][blk_index] = p;
  blk_index = (blk_index + 1) % 1024;
  if (!blk_index)
    ++list_index;
  return p;
}

static void init_directions(void ** root, const struct locale *lang)
{
  /* mit dieser routine kann man mehrere namen für eine direction geben,
   * das ist für die hexes ideal. */
  const struct {
    const char *name;
    int direction;
  } dirs[] = {
    {
    "dir_ne", D_NORTHEAST}, {
    "dir_nw", D_NORTHWEST}, {
    "dir_se", D_SOUTHEAST}, {
    "dir_sw", D_SOUTHWEST}, {
    "dir_east", D_EAST}, {
    "dir_west", D_WEST}, {
    "northeast", D_NORTHEAST}, {
    "northwest", D_NORTHWEST}, {
    "southeast", D_SOUTHEAST}, {
    "southwest", D_SOUTHWEST}, {
    "east", D_EAST}, {
    "west", D_WEST}, {
    "PAUSE", D_PAUSE}, {
    NULL, NODIRECTION}
  };
  int i;
  void **tokens = get_translations(lang, UT_DIRECTIONS);

  for (i = 0; dirs[i].direction != NODIRECTION; ++i) {
    variant token;
    token.i = dirs[i].direction;
    addtoken(tokens, LOC(lang, dirs[i].name), token);
  }
}

direction_t finddirection(const char *s, const struct locale *lang)
{
  void **tokens = get_translations(lang, UT_DIRECTIONS);
  variant token;

  if (findtoken(*tokens, s, &token) == E_TOK_SUCCESS) {
    return (direction_t) token.i;
  }
  return NODIRECTION;
}

direction_t getdirection(const struct locale * lang)
{
  return finddirection(getstrtoken(), lang);
}

static void init_translations(const struct locale *lang, int ut, const char * (*string_cb)(int i), int maxstrings)
{
  char buffer[256];
  void **tokens;
  int i;

  assert(string_cb);
  assert(maxstrings>0);
  tokens = get_translations(lang, ut);
  for (i = 0; i != maxstrings; ++i) {
    const char * s = string_cb(i);
    const char * key = s ? locale_string(lang, s) : 0;
    if (key) {
      char * str = transliterate(buffer, sizeof(buffer)-sizeof(int), key);
      if (str) {
        critbit_tree * cb = (critbit_tree *)*tokens;
		size_t len = strlen(str);
		if (!cb) {
          *tokens = cb = (critbit_tree *)calloc(1, sizeof(critbit_tree *));
        }
        len = cb_new_kv(str, len, &i, sizeof(int), buffer);
        cb_insert(cb, buffer, len);
      } else {
        log_error("could not transliterate '%s'\n", key);
      }
    }
  }
}

static const char * keyword_key(int i)
{
  assert(i<MAXKEYWORDS&& i>=0);
  return keywords[i];
}

static const char * parameter_key(int i)
{
  assert(i<MAXPARAMS && i>=0);
  return parameters[i];
}

static const char * skill_key(int sk)
{
  assert(sk<MAXPARAMS && sk>=0);
  return skill_enabled[sk] ? mkname("skill", skillnames[sk]) : 0;
}

static void init_locale(const struct locale *lang)
{
  variant var;
  int i;
  const struct race *rc;
  const terrain_type *terrain;
  void **tokens;

  tokens = get_translations(lang, UT_MAGIC);
  if (tokens) {
    const char *str = get_param(global.parameters, "rules.magic.playerschools");
    char *sstr, *tok;
    if (str == NULL) {
      str = "gwyrrd illaun draig cerddor tybied";
    }

    sstr = _strdup(str);
    tok = strtok(sstr, " ");
    while (tok) {
      for (i = 0; i != MAXMAGIETYP; ++i) {
        if (strcmp(tok, magic_school[i]) == 0)
          break;
      }
      assert(i != MAXMAGIETYP);
      var.i = i;
      addtoken(tokens, LOC(lang, mkname("school", tok)), var);
      tok = strtok(NULL, " ");
    }
    free(sstr);
  }

  tokens = get_translations(lang, UT_DIRECTIONS);
  init_directions(tokens, lang);

  tokens = get_translations(lang, UT_RACES);
  for (rc = races; rc; rc = rc->next) {
    var.v = (void *)rc;
    addtoken(tokens, LOC(lang, rc_name(rc, 1)), var);
    addtoken(tokens, LOC(lang, rc_name(rc, 0)), var);
  }

  init_translations(lang, UT_PARAMS, parameter_key, MAXPARAMS);
  init_translations(lang, UT_SKILLS, skill_key, MAXSKILLS);
  init_translations(lang, UT_KEYWORDS, keyword_key, MAXKEYWORDS);

  tokens = get_translations(lang, UT_OPTIONS);
  for (i = 0; i != MAXOPTIONS; ++i) {
    var.i = i;
    if (options[i])
      addtoken(tokens, LOC(lang, options[i]), var);
  }

  tokens = get_translations(lang, UT_TERRAINS);
  for (terrain = terrains(); terrain != NULL; terrain = terrain->next) {
    var.v = (void *)terrain;
    addtoken(tokens, LOC(lang, terrain->_name), var);
  }
}

typedef struct param {
  struct param *next;
  char *name;
  char *data;
} param;

int getid(void)
{
  const char *str = (const char *)getstrtoken();
  int i = atoi36(str);
  if (i < 0) {
    return -1;
  }
  return i;
}

const char *get_param(const struct param *p, const char *key)
{
  while (p != NULL) {
    if (strcmp(p->name, key) == 0)
      return p->data;
    p = p->next;
  }
  return NULL;
}

int get_param_int(const struct param *p, const char *key, int def)
{
  while (p != NULL) {
    if (strcmp(p->name, key) == 0)
      return atoi(p->data);
    p = p->next;
  }
  return def;
}

static const char *g_datadir;
const char *datapath(void)
{
  static char zText[MAX_PATH];
  if (g_datadir)
    return g_datadir;
  return strcat(strcpy(zText, basepath()), "/data");
}

void set_datapath(const char *path)
{
  g_datadir = path;
}

static const char *g_reportdir;
const char *reportpath(void)
{
  static char zText[MAX_PATH];
  if (g_reportdir)
    return g_reportdir;
  return strcat(strcpy(zText, basepath()), "/reports");
}

void set_reportpath(const char *path)
{
  g_reportdir = path;
}

static const char *g_basedir;
const char *basepath(void)
{
  if (g_basedir)
    return g_basedir;
  return ".";
}

void set_basepath(const char *path)
{
  g_basedir = path;
}

float get_param_flt(const struct param *p, const char *key, float def)
{
  while (p != NULL) {
    if (strcmp(p->name, key) == 0)
      return (float)atof(p->data);
    p = p->next;
  }
  return def;
}

void set_param(struct param **p, const char *key, const char *data)
{
  ++global.cookie;
  while (*p != NULL) {
    if (strcmp((*p)->name, key) == 0) {
      free((*p)->data);
      (*p)->data = _strdup(data);
      return;
    }
    p = &(*p)->next;
  }
  *p = malloc(sizeof(param));
  (*p)->name = _strdup(key);
  (*p)->data = _strdup(data);
  (*p)->next = NULL;
}

void kernel_done(void)
{
  /* calling this function releases memory assigned to static variables, etc.
   * calling it is optional, e.g. a release server will most likely not do it.
   */
  translation_done();
  gc_done();
  sql_done();
}

const char *localenames[] = {
  "de", "en",
  NULL
};

void init_locales(void)
{
  int l;
  for (l = 0; localenames[l]; ++l) {
    const struct locale *lang = find_locale(localenames[l]);
    if (lang)
      init_locale(lang);
  }
}

/* TODO: soll hier weg */
extern struct attrib_type at_shiptrail;

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

void setstatus(struct unit *u, int status)
{
  assert(status >= ST_AGGRO && status <= ST_FLEE);
  if (u->status != status) {
    u->status = (status_t) status;
  }
}

void setguard(unit * u, unsigned int flags)
{
  /* setzt die guard-flags der Einheit */
  attrib *a = NULL;
  assert(flags == 0 || !fval(u, UFL_MOVED));
  assert(flags == 0 || u->status < ST_FLEE);
  if (fval(u, UFL_GUARD)) {
    a = a_find(u->attribs, &at_guard);
  }
  if (flags == GUARD_NONE) {
    freset(u, UFL_GUARD);
    if (a)
      a_remove(&u->attribs, a);
    return;
  }
  fset(u, UFL_GUARD);
  fset(u->region, RF_GUARDED);
  if ((int)flags == guard_flags(u)) {
    if (a)
      a_remove(&u->attribs, a);
  } else {
    if (!a)
      a = a_add(&u->attribs, a_new(&at_guard));
    a->data.i = (int)flags;
  }
}

unsigned int getguard(const unit * u)
{
  attrib *a;

  assert(fval(u, UFL_GUARD) || (u->building && u==building_owner(u->building))
    || !"you're doing it wrong! check is_guard first");
  a = a_find(u->attribs, &at_guard);
  if (a) {
    return (unsigned int)a->data.i;
  }
  return guard_flags(u);
}

#ifndef HAVE_STRDUP
char *_strdup(const char *s)
{
  return strcpy((char *)malloc(sizeof(char) * (strlen(s) + 1)), s);
}
#endif

void remove_empty_factions(void)
{
  faction **fp, *f3;

  for (fp = &factions; *fp;) {
    faction *f = *fp;
    /* monster (0) werden nicht entfernt. alive kann beim readgame
     * () auf 0 gesetzt werden, wenn monsters keine einheiten mehr
     * haben. */
    if ((f->units == NULL || f->alive == 0) && !is_monsters(f)) {
      ursprung *ur = f->ursprung;
      while (ur && ur->id != 0)
        ur = ur->next;
      if (verbosity >= 2)
        log_printf(stdout, "\t%s\n", factionname(f));

      /* Einfach in eine Datei schreiben und später vermailen */

      if (updatelog)
        fprintf(updatelog, "dropout %s\n", itoa36(f->no));

      for (f3 = factions; f3; f3 = f3->next) {
        ally *sf;
        group *g;
        ally **sfp = &f3->allies;
        while (*sfp) {
          sf = *sfp;
          if (sf->faction == f || sf->faction == NULL) {
            *sfp = sf->next;
            free(sf);
          } else
            sfp = &(*sfp)->next;
        }
        for (g = f3->groups; g; g = g->next) {
          sfp = &g->allies;
          while (*sfp) {
            sf = *sfp;
            if (sf->faction == f || sf->faction == NULL) {
              *sfp = sf->next;
              free(sf);
            } else
              sfp = &(*sfp)->next;
          }
        }
      }
      if (f->subscription) {
        sql_print(("UPDATE subscriptions set status='DEAD' where id=%u;\n",
            f->subscription));
      }

      *fp = f->next;
      funhash(f);
      free_faction(f);
      free(f);
    } else
      fp = &(*fp)->next;
  }
}

void remove_empty_units_in_region(region * r)
{
  unit **up = &r->units;

  while (*up) {
    unit *u = *up;

    if (u->number) {
      faction *f = u->faction;
      if (f == NULL || !f->alive) {
        set_number(u, 0);
      }
      if (MaxAge() > 0) {
        if ((!fval(f, FFL_NOTIMEOUT) && f->age > MaxAge())) {
          set_number(u, 0);
        }
      }
    }
    if ((u->number == 0 && u_race(u) != new_race[RC_SPELL]) || (u->age <= 0
        && u_race(u) == new_race[RC_SPELL])) {
      remove_unit(up, u);
    }
    if (*up == u)
      up = &u->next;
  }
}

void remove_empty_units(void)
{
  region *r;

  for (r = regions; r; r = r->next) {
    remove_empty_units_in_region(r);
  }
}

bool faction_id_is_unused(int id)
{
  return findfaction(id) == NULL;
}

int weight(const unit * u)
{
  int w, n = 0, in_bag = 0;

  item *itm;
  for (itm = u->items; itm; itm = itm->next) {
    w = itm->type->weight * itm->number;
    n += w;
    if (!fval(itm->type, ITF_BIG))
      in_bag += w;
  }

  n += u->number * u_race(u)->weight;

  w = get_item(u, I_BAG_OF_HOLDING) * BAGCAPACITY;
  if (w > in_bag)
    w = in_bag;
  n -= w;

  return n;
}

void make_undead_unit(unit * u)
{
  free_orders(&u->orders);
  name_unit(u);
  fset(u, UFL_ISNEW);
}

unsigned int guard_flags(const unit * u)
{
  unsigned int flags =
    GUARD_CREWS | GUARD_LANDING | GUARD_TRAVELTHRU | GUARD_TAX;
#if GUARD_DISABLES_PRODUCTION == 1
  flags |= GUARD_PRODUCE;
#endif
#if GUARD_DISABLES_RECRUIT == 1
  flags |= GUARD_RECRUIT;
#endif
  switch (old_race(u_race(u))) {
  case RC_ELF:
    if (u->faction->race != u_race(u))
      break;
    /* else fallthrough */
  case RC_TREEMAN:
    flags |= GUARD_TREES;
    break;
  case RC_IRONKEEPER:
    flags = GUARD_MINING;
    break;
  default:
    /* TODO: This should be configuration variables, all of it */
    break;
  }
  return flags;
}

void guard(unit * u, unsigned int mask)
{
  unsigned int flags = guard_flags(u);
  setguard(u, flags & mask);
}

int besieged(const unit * u)
{
  /* belagert kann man in schiffen und burgen werden */
  return (u && !global.disabled[K_BESIEGE]
    && u->building && u->building->besieged
    && u->building->besieged >= u->building->size * SIEGEFACTOR);
}

int lifestyle(const unit * u)
{
  int need;
  plane *pl;
  static int gamecookie = -1;
  if (gamecookie != global.cookie) {
    gamecookie = global.cookie;
  }

  if (is_monsters(u->faction))
    return 0;

  need = maintenance_cost(u);

  pl = rplane(u->region);
  if (pl && fval(pl, PFL_NOFEED))
    return 0;

  return need;
}

bool has_horses(const struct unit * u)
{
  item *itm = u->items;
  for (; itm; itm = itm->next) {
    if (itm->type->flags & ITF_ANIMAL)
      return true;
  }
  return false;
}

bool hunger(int number, unit * u)
{
  region *r = u->region;
  int dead = 0, hpsub = 0;
  int hp = u->hp / u->number;
  static const char *damage = 0;
  static const char *rcdamage = 0;
  static const race *rc = 0;

  if (!damage) {
    damage = get_param(global.parameters, "hunger.damage");
    if (damage == NULL)
      damage = "1d12+12";
  }
  if (rc != u_race(u)) {
    rcdamage = get_param(u_race(u)->parameters, "hunger.damage");
    rc = u_race(u);
  }

  while (number--) {
    int dam = dice_rand(rcdamage ? rcdamage : damage);
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
        "unit region dead live", u, r, dead, u->number - dead));

    scale_number(u, u->number - dead);
    deathcounts(r, dead);
  }
  if (hpsub > 0) {
    /* Jetzt die Schäden der nicht gestorbenen abziehen. */
    u->hp -= hpsub;
    /* Meldung nur, wenn noch keine für Tote generiert. */
    if (dead == 0) {
      /* Durch unzureichende Ernährung wird %s geschwächt */
      ADDMSG(&u->faction->msgs, msg_message("malnourish", "unit region", u, r));
    }
  }
  return (dead || hpsub);
}

void plagues(region * r, bool ismagic)
{
  int peasants;
  int i;
  int dead = 0;

  /* Seuchenwahrscheinlichkeit in % */

  if (!ismagic) {
    double mwp = _max(maxworkingpeasants(r), 1);
    double prob =
      pow(rpeasants(r) / (mwp * wage(r, NULL, NULL, turn) * 0.13), 4.0)
      * PLAGUE_CHANCE;

    if (rng_double() >= prob)
      return;
  }

  peasants = rpeasants(r);
  dead = (int)(0.5F + PLAGUE_VICTIMS * peasants);
  for (i = dead; i != 0; i--) {
    if (rng_double() < PLAGUE_HEALCHANCE && rmoney(r) >= PLAGUE_HEALCOST) {
      rsetmoney(r, rmoney(r) - PLAGUE_HEALCOST);
      --dead;
    }
  }

  if (dead > 0) {
    message *msg = add_message(&r->msgs, msg_message("pest", "dead", dead));
    msg_release(msg);
    deathcounts(r, dead);
    rsetpeasants(r, peasants - dead);
  }
}

/* Lohn bei den einzelnen Burgstufen für Normale Typen, Orks, Bauern,
 * Modifikation für Städter. */

static const int wagetable[7][4] = {
  {10, 10, 11, -7},             /* Baustelle */
  {10, 10, 11, -5},             /* Handelsposten */
  {11, 11, 12, -3},             /* Befestigung */
  {12, 11, 13, -1},             /* Turm */
  {13, 12, 14, 0},              /* Burg */
  {14, 12, 15, 1},              /* Festung */
  {15, 13, 16, 2}               /* Zitadelle */
};

int cmp_wage(const struct building *b, const building * a)
{
  static const struct building_type *bt_castle;
  if (!bt_castle)
    bt_castle = bt_find("castle");
  if (b->type == bt_castle) {
    if (!a)
      return 1;
    if (b->size > a->size)
      return 1;
    if (b->size == a->size)
      return 0;
  }
  return -1;
}

bool is_owner_building(const struct building * b)
{
  region *r = b->region;
  if (b->type->taxes && r->land && r->land->ownership) {
    unit *u = building_owner(b);
    return u && u->faction == r->land->ownership->owner;
  }
  return false;
}

int cmp_taxes(const building * b, const building * a)
{
  faction *f = region_get_owner(b->region);
  if (b->type->taxes) {
    unit *u = building_owner(b);
    if (!u) {
      return -1;
    } else if (a) {
      int newsize = buildingeffsize(b, false);
      double newtaxes = b->type->taxes(b, newsize);
      int oldsize = buildingeffsize(a, false);
      double oldtaxes = a->type->taxes(a, oldsize);

      if (newtaxes < oldtaxes)
        return -1;
      else if (newtaxes > oldtaxes)
        return 1;
      else if (b->size < a->size)
        return -1;
      else if (b->size > a->size)
        return 1;
      else {
        if (u && u->faction == f) {
          u = building_owner(a);
          if (u && u->faction == f)
            return -1;
          return 1;
        }
      }
    } else {
      return 1;
    }
  }
  return -1;
}

int cmp_current_owner(const building * b, const building * a)
{
  faction *f = region_get_owner(b->region);

  assert(rule_region_owners());
  if (f && b->type->taxes) {
    unit *u = building_owner(b);
    if (!u || u->faction != f)
      return -1;
    if (a) {
      int newsize = buildingeffsize(b, false);
      double newtaxes = b->type->taxes(b, newsize);
      int oldsize = buildingeffsize(a, false);
      double oldtaxes = a->type->taxes(a, oldsize);

      if (newtaxes != oldtaxes)
        return (newtaxes > oldtaxes) ? 1 : -1;
      if (newsize != oldsize)
        return newsize - oldsize;
      return (b->size - a->size);
    } else {
      return 1;
    }
  }
  return -1;
}

int rule_stealth_faction(void)
{
  static int gamecookie = -1;
  static int rule = -1;
  if (rule < 0 || gamecookie != global.cookie) {
    rule = get_param_int(global.parameters, "rules.stealth.faction", 0xFF);
    gamecookie = global.cookie;
    assert(rule >= 0);
  }
  return rule;
}

int rule_region_owners(void)
{
  static int gamecookie = -1;
  static int rule = -1;
  if (rule < 0 || gamecookie != global.cookie) {
    rule = get_param_int(global.parameters, "rules.region_owners", 0);
    gamecookie = global.cookie;
    assert(rule >= 0);
  }
  return rule;
}

int rule_auto_taxation(void)
{
  static int gamecookie = -1;
  static int rule = -1;
  if (rule < 0 || gamecookie != global.cookie) {
    rule =
      get_param_int(global.parameters, "rules.economy.taxation", TAX_ORDER);
    gamecookie = global.cookie;
    assert(rule >= 0);
  }
  return rule;
}

int rule_blessed_harvest(void)
{
  static int gamecookie = -1;
  static int rule = -1;
  if (rule < 0 || gamecookie != global.cookie) {
    rule =
      get_param_int(global.parameters, "rules.magic.blessed_harvest",
      HARVEST_WORK);
    gamecookie = global.cookie;
    assert(rule >= 0);
  }
  return rule;
}

int rule_alliance_limit(void)
{
  static int gamecookie = -1;
  static int rule = -1;
  if (rule < 0 || gamecookie != global.cookie) {
    rule = get_param_int(global.parameters, "rules.limit.alliance", 0);
    gamecookie = global.cookie;
    assert(rule >= 0);
  }
  return rule;
}

int rule_faction_limit(void)
{
  static int gamecookie = -1;
  static int rule = -1;
  if (rule < 0 || gamecookie != global.cookie) {
    rule = get_param_int(global.parameters, "rules.limit.faction", 0);
    gamecookie = global.cookie;
    assert(rule >= 0);
  }
  return rule;
}

int rule_transfermen(void)
{
  static int gamecookie = -1;
  static int rule = -1;
  if (rule < 0 || gamecookie != global.cookie) {
    rule = get_param_int(global.parameters, "rules.transfermen", 1);
    gamecookie = global.cookie;
    assert(rule >= 0);
  }
  return rule;
}

static int
default_wage(const region * r, const faction * f, const race * rc, int in_turn)
{
  building *b = largestbuilding(r, &cmp_wage, false);
  int esize = 0;
  curse *c;
  double wage;
  attrib *a;
  const building_type *artsculpture_type = bt_find("artsculpture");
  static const curse_type *drought_ct, *blessedharvest_ct;
  static bool init;

  if (!init) {
    init = true;
    drought_ct = ct_find("drought");
    blessedharvest_ct = ct_find("blessedharvest");
  }

  if (b != NULL) {
    /* TODO: this reveals imaginary castles */
    esize = buildingeffsize(b, false);
  }

  if (f != NULL) {
    int index = 0;
    if (rc == new_race[RC_ORC] || rc == new_race[RC_SNOTLING]) {
      index = 1;
    }
    wage = wagetable[esize][index];
  } else {
    if (is_mourning(r, in_turn)) {
      wage = 10;
    } else if (fval(r->terrain, SEA_REGION)) {
      wage = 11;
    } else if (fval(r, RF_ORCIFIED)) {
      wage = wagetable[esize][1];
    } else {
      wage = wagetable[esize][2];
    }
    if (rule_blessed_harvest() == HARVEST_WORK) {
      /* E1 rules */
      wage += curse_geteffect(get_curse(r->attribs, blessedharvest_ct));
    }
  }

  /* Artsculpture: Income +5 */
  for (b = r->buildings; b; b = b->next) {
    if (b->type == artsculpture_type) {
      wage += 5;
    }
  }

  /* Godcurse: Income -10 */
  if (curse_active(get_curse(r->attribs, ct_find("godcursezone")))) {
    wage = _max(0, wage - 10);
  }

  /* Bei einer Dürre verdient man nur noch ein Viertel  */
  if (drought_ct) {
    c = get_curse(r->attribs, drought_ct);
    if (curse_active(c))
      wage /= curse_geteffect(c);
  }

  a = a_find(r->attribs, &at_reduceproduction);
  if (a)
    wage = (wage * a->data.sa[0]) / 100;

  return (int)wage;
}

static int
minimum_wage(const region * r, const faction * f, const race * rc, int in_turn)
{
  if (f && rc) {
    return rc->maintenance;
  }
  return default_wage(r, f, rc, in_turn);
}

/* Gibt Arbeitslohn für entsprechende Rasse zurück, oder für
* die Bauern wenn f == NULL. */
int wage(const region * r, const faction * f, const race * rc, int in_turn)
{
  if (global.functions.wage) {
    return global.functions.wage(r, f, rc, in_turn);
  }
  return default_wage(r, f, rc, in_turn);
}

#define MAINTENANCE 10
int maintenance_cost(const struct unit *u)
{
  if (u == NULL)
    return MAINTENANCE;
  if (global.functions.maintenance) {
    int retval = global.functions.maintenance(u);
    if (retval >= 0)
      return retval;
  }
  return u_race(u)->maintenance * u->number;
}

message *movement_error(unit * u, const char *token, order * ord,
  int error_code)
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

int movewhere(const unit * u, const char *token, region * r, region ** resultp)
{
  region *r2;
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
    r2 = find_special_direction(r, token, u->faction->locale);
    if (r2 == NULL) {
      return E_MOVE_NOREGION;
    }
    *resultp = r2;
    break;

  default:
    r2 = rconnect(r, d);
    if (r2 == NULL || move_blocked(u, r, r2)) {
      return E_MOVE_BLOCKED;
    }
    *resultp = r2;
  }
  return E_MOVE_OK;
}

bool move_blocked(const unit * u, const region * r, const region * r2)
{
  connection *b;
  curse *c;
  static const curse_type *fogtrap_ct = NULL;

  if (r2 == NULL)
    return true;
  b = get_borders(r, r2);
  while (b) {
    if (b->type->block && b->type->block(b, u, r))
      return true;
    b = b->next;
  }

  if (fogtrap_ct == NULL)
    fogtrap_ct = ct_find("fogtrap");
  c = get_curse(r->attribs, fogtrap_ct);
  if (curse_active(c))
    return true;
  return false;
}

void add_income(unit * u, int type, int want, int qty)
{
  if (want == INT_MAX)
    want = qty;
  ADDMSG(&u->faction->msgs, msg_message("income",
      "unit region mode wanted amount", u, u->region, type, want, qty));
}

int produceexp(struct unit *u, skill_t sk, int n)
{
  if (global.producexpchance > 0.0F) {
    if (n == 0 || !playerrace(u_race(u)))
      return 0;
    learn_skill(u, sk, global.producexpchance);
  }
  return 0;
}

int lovar(double xpct_x2)
{
  int n = (int)(xpct_x2 * 500) + 1;
  if (n == 0)
    return 0;
  return (rng_int() % n + rng_int() % n) / 1000;
}

bool has_limited_skills(const struct unit * u)
{
  if (has_skill(u, SK_MAGIC) || has_skill(u, SK_ALCHEMY) ||
    has_skill(u, SK_TACTICS) || has_skill(u, SK_HERBALISM) ||
    has_skill(u, SK_SPY)) {
    return true;
  } else {
    return false;
  }
}

void attrib_init(void)
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

  at_register(&at_building_generic_type);
  at_register(&at_maxmagicians);
  at_register(&at_npcfaction);

  /* connection-typen */
  register_bordertype(&bt_noway);
  register_bordertype(&bt_fogwall);
  register_bordertype(&bt_wall);
  register_bordertype(&bt_illusionwall);
  register_bordertype(&bt_road);
  register_bordertype(&bt_questportal);

  register_function((pf_generic) & minimum_wage, "minimum_wage");

  at_register(&at_germs);

  at_deprecate("xontormiaexpress", a_readint);    /* required for old datafiles */
  at_register(&at_speedup);
  at_register(&at_building_action);
}

void kernel_init(void)
{
  char zBuffer[MAX_PATH];
  attrib_init();
  translation_init();

  if (sqlpatch) {
    sprintf(zBuffer, "%s/patch-%d.sql", datapath(), turn);
    sql_init(zBuffer);
  }
}

static order * defaults[MAXLOCALES];

order *default_order(const struct locale *lang)
{
  int i = locale_index(lang);
  order *result = 0;
  assert(i<MAXLOCALES);
  result = defaults[i];
  if (!result) {
    const char * str = locale_string(lang, "defaultorder");
    if (str) {
      result = defaults[i] = parse_order(str, lang);
    }
  }
  return result ? copy_order(result) : 0;
}

int entertainmoney(const region * r)
{
  double n;

  if (is_cursed(r->attribs, C_DEPRESSION, 0)) {
    return 0;
  }

  n = rmoney(r) / ENTERTAINFRACTION;

  if (is_cursed(r->attribs, C_GENEROUS, 0)) {
    n *= get_curseeffect(r->attribs, C_GENEROUS, 0);
  }

  return (int)n;
}

int rule_give(void)
{
  static int value = -1;
  if (value < 0) {
    value = get_param_int(global.parameters, "rules.give", GIVE_DEFAULT);
  }
  return value;
}

int markets_module(void)
{
  static int value = -1;
  if (value < 0) {
    value = get_param_int(global.parameters, "modules.markets", 0);
  }
  return value;
}

/** releases all memory associated with the game state.
 * call this function before calling read_game() to load a new game
 * if you have a previously loaded state in memory.
 */
void free_gamedata(void)
{
  int i;
  free_units();
  free_regions();
  free_borders();

  for (i=0;i!=MAXLOCALES;++i) {
    if (defaults[i]) {
      free_order(defaults[i]);
      defaults[i] = 0;
    }
  }
  while (alliances) {
    alliance *al = alliances;
    alliances = al->next;
    free_alliance(al);
  }
  while (factions) {
    faction *f = factions;
    factions = f->next;
    funhash(f);
    free_faction(f);
    free(f);
  }

  while (planes) {
    plane *pl = planes;
    planes = planes->next;
    free(pl->name);
    free(pl);
  }

  while (global.attribs) {
    a_remove(&global.attribs, global.attribs);
  }
  ++global.cookie;              /* readgame() already does this, but sjust in case */
}

void load_inifile(dictionary * d)
{
  const char *reportdir = reportpath();
  const char *datadir = datapath();
  const char *basedir = basepath();
  const char *str;

  assert(d);

  str = iniparser_getstring(d, "eressea:base", basedir);
  if (str != basedir)
    set_basepath(str);
  str = iniparser_getstring(d, "eressea:report", reportdir);
  if (str != reportdir)
    set_reportpath(str);
  str = iniparser_getstring(d, "eressea:data", datadir);
  if (str != datadir)
    set_datapath(str);

  lomem = iniparser_getint(d, "eressea:lomem", lomem) ? 1 : 0;

  str = iniparser_getstring(d, "eressea:encoding", NULL);
  if (str)
    enc_gamedata = xmlParseCharEncoding(str);

  verbosity = iniparser_getint(d, "eressea:verbose", 2);
  sqlpatch = iniparser_getint(d, "eressea:sqlpatch", false);
  battledebug = iniparser_getint(d, "eressea:debug", battledebug) ? 1 : 0;

  str = iniparser_getstring(d, "eressea:locales", "de,en");
  make_locales(str);

  /* excerpt from [config] (the rest is used in bindings.c) */
  global.game_id = iniparser_getint(d, "config:game_id", 0);
  global.inifile = d;
}
