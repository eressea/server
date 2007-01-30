/* vi: set ts=2:
 *
 *
 *  Eressea PB(E)M host Copyright (C) 1998-2003
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
#include "battle.h"

#include "alchemy.h"
#include "alliance.h"
#include "build.h"
#include "building.h"
#include "curse.h"
#include "equipment.h"
#include "faction.h"
#include "goodies.h"
#include "group.h"
#include "item.h"
#include "karma.h"
#include "magic.h"
#include "message.h"
#include "movement.h"
#include "names.h"
#include "order.h"
#include "plane.h"
#include "race.h"
#include "region.h"
#include "reports.h"
#include "ship.h"
#include "skill.h"
#include "terrain.h"
#include "unit.h"

/* attributes includes */
#include <attributes/key.h>
#include <attributes/fleechance.h>
#include <attributes/racename.h>
#include <attributes/otherfaction.h>
#include <attributes/moved.h>

/* items includes */
#include <items/demonseye.h>

/* util includes */
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/cvector.h>
#include <util/rand.h>
#include <util/rng.h>
#include <util/log.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>

#define dbgprintf(a) fprintf a;
static FILE *bdebug;
#undef DELAYED_OFFENSE /* non-guarding factions cannot attack after moving */

#define TACTICS_RANDOM 5 /* define this as 1 to deactivate */
#define CATAPULT_INITIAL_RELOAD 4 /* erster schuss in runde 1 + rng_int() % INITIAL */
#define CATAPULT_STRUCTURAL_DAMAGE

#define BASE_CHANCE    70 /* 70% Basis-Überlebenschance */
#ifdef NEW_COMBATSKILLS_RULE
#define TDIFF_CHANGE    5 /* 5% höher pro Stufe */
#define DAMAGE_QUOTIENT 2 /* damage += skilldiff/DAMAGE_QUOTIENT */
#else
#define TDIFF_CHANGE 10
# define DAMAGE_QUOTIENT 1 /* damage += skilldiff/DAMAGE_QUOTIENT */
#endif

typedef enum combatmagic {
  DO_PRECOMBATSPELL,
  DO_POSTCOMBATSPELL
} combatmagic_t;

/* external variables */
boolean nobattledebug = false;

/* globals */
static int obs_count = 0;

#define TACTICS_MALUS 1
#undef  MAGIC_TURNS

#define MINSPELLRANGE 1
#define MAXSPELLRANGE 7

#ifndef ROW_FACTOR
# define ROW_FACTOR 10
#endif
static const double EFFECT_PANIC_SPELL = 0.25;
static const double TROLL_REGENERATION = 0.10;

extern weapon_type * oldweapontype[];

/* Nach dem alten System: */
static int missile_range[2] = {FIGHT_ROW, BEHIND_ROW};
static int melee_range[2] = {FIGHT_ROW, FIGHT_ROW};

const troop no_troop = {0, 0};

static int
army_index(side * s)
{
  return s->battle->nsides - s->index - 1;
}

region *
fleeregion(const unit * u)
{
  region *r = u->region;
  region *neighbours[MAXDIRECTIONS];
  int c = 0;
  direction_t i;

  if (u->ship && !fval(r->terrain, SEA_REGION))
    return NULL;

  if (u->ship &&
      !(u->race->flags & RCF_SWIM) &&
      !(u->race->flags & RCF_FLY)) {
    return NULL;
  }

  for (i = 0; i != MAXDIRECTIONS; ++i) {
    region * r2 = rconnect(r, i);
    if (r2) {
      if (can_survive(u,r2) && !move_blocked(u, r, r2))
        neighbours[c++] = r2;
    }
  }

  if (!c)
    return NULL;
  return neighbours[rng_int() % c];
}

static char *
sidename(side * s, boolean truename)
{
#define SIDENAMEBUFLEN 256
  static char sidename_buf[SIDENAMEBUFLEN];

  if(s->stealthfaction && truename == false) {
    snprintf(sidename_buf, SIDENAMEBUFLEN,
      "%s", factionname(s->stealthfaction));
  } else {
    snprintf(sidename_buf, SIDENAMEBUFLEN,
      "%s", factionname(s->bf->faction));
  }
  return sidename_buf;
}

static const char *
sideabkz(side *s, boolean truename)
{
  static char sideabkz_buf[4];

  if(s->stealthfaction && truename == false) {
    snprintf(sideabkz_buf, 4, "%s", abkz(s->stealthfaction->name, 3));
  } else {
    snprintf(sideabkz_buf, 4, "%s", abkz(s->bf->faction->name, 3));
  }
  return sideabkz_buf;
}

void
message_faction(battle * b, faction * f, struct message * m)
{
  region * r = b->region;

  if (f->battles==NULL || f->battles->r!=r) {
    struct bmsg * bm = calloc(1, sizeof(struct bmsg));
    bm->next = f->battles;
    f->battles = bm;
    bm->r = r;
  }
  add_message(&f->battles->msgs, m);
}

int
armedmen(const unit * u)
{
  item * itm;
  int n = 0;
  if (!(urace(u)->flags & RCF_NOWEAPONS)) {
    if ((urace(u)->ec_flags & CANGUARD) || effskill(u, SK_WEAPONLESS)>=1) {
      /* kann ohne waffen bewachen: fuer drachen */
      n = u->number;
    } else {
      /* alle Waffen werden gezaehlt, und dann wird auf die Anzahl
       * Personen minimiert */
      for (itm=u->items;itm;itm=itm->next) {
        const weapon_type * wtype = resource2weapon(itm->type->rtype);
        if (wtype==NULL) continue;
        if (effskill(u, wtype->skill) >= 1) n += itm->number;
        /* if (effskill(u, wtype->skill) >= wtype->minskill) n += itm->number; */
        if (n>u->number) break;
      }
      n = min(n, u->number);
    }
  }
  return n;
}

static void
battledebug(const char *s)
{
#if SHOW_DEBUG
  puts(s);
  putc('\n');
#endif
  if (bdebug) {
    dbgprintf((bdebug, "%s\n", s));
  }
}

void
message_all(battle * b, message * m)
{
  bfaction * bf;
  plane * p = rplane(b->region);
  watcher * w;

  for (bf = b->factions;bf;bf=bf->next) {
    message_faction(b, bf->faction, m);
  }
  if (p) for (w=p->watchers;w;w=w->next) {
    for (bf = b->factions;bf;bf=bf->next) if (bf->faction==w->faction) break;
    if (bf==NULL) message_faction(b, w->faction, m);
  }
}

void
battlerecord(battle * b, const char *s)
{
  struct message * m = msg_message("msg_battle", "string", s);
  message_all(b, m);
  msg_release(m);
}

void
battlemsg(battle * b, unit * u, const char * s)
{
  bfaction * bf;
  struct message * m;
  plane * p = rplane(b->region);
  watcher * w;

  sprintf(buf, "%s %s", unitname(u), s);
  m = msg_message("msg_battle", "string", buf);
  for (bf=b->factions;bf;bf=bf->next) {
    message_faction(b, bf->faction, m);
  }
  if (p) for (w=p->watchers;w;w=w->next) {
    for (bf = b->factions;bf;bf=bf->next) if (bf->faction==w->faction) break;
    if (bf==NULL) message_faction(b, w->faction, m);
  }
  msg_release(m);
}

static void
fbattlerecord(battle * b, faction * f, const char *s)
{
  message * m = msg_message("msg_battle", "string", s);
  message_faction(b, f, m);
  msg_release(m);
}

#define enemy(as, ds) (as->relations[ds->index]&E_ENEMY)
#define friendly(as, ds) (as->bf->faction==ds->bf->faction || (as->relations[ds->index]&E_FRIEND))

static void
set_enemy(side * as, side * ds, boolean attacking)
{
  int i;
  for (i=0;i!=128;++i) {
    if (ds->enemies[i]==NULL) ds->enemies[i]=as;
    if (ds->enemies[i]==as) break;
  }
  for (i=0;i!=128;++i) {
    if (as->enemies[i]==NULL) as->enemies[i]=ds;
    if (as->enemies[i]==ds) break;
  }
  ds->relations[as->index] |= E_ENEMY;
  as->relations[ds->index] |= E_ENEMY;
  if (attacking) as->relations[ds->index] |= E_ATTACKING;
}

static void
set_friendly(side * as, side * ds)
{
  ds->relations[as->index] |= E_FRIEND;
  as->relations[ds->index] |= E_FRIEND;
}

static int
allysfm(const side * s, const faction * f, int mode)
{
  if (s->bf->faction==f) return mode;
  if (s->group) {
    return alliedgroup(s->battle->plane, s->bf->faction, f, s->group->allies, mode);
  }
  return alliedfaction(s->battle->plane, s->bf->faction, f, mode);
}

static int
allysf(const side * s, const faction * f)
{
  return allysfm(s, f, HELP_FIGHT);
}

fighter *
select_corpse(battle * b, fighter * af)
/* Wählt eine Leiche aus, der af hilft. casualties ist die Anzahl der
 * Toten auf allen Seiten (im Array). Wenn af == NULL, wird die
 * Parteizugehörigkeit ignoriert, und irgendeine Leiche genommen.
 *
 * Untote werden nicht ausgewählt (casualties, not dead) */
{
  int di, maxcasualties = 0;
  fighter *df;
  side *s;

  for (s=b->sides;s;s=s->next) {
    if (af==NULL || (!enemy(af->side, s) && allysf(af->side, s->bf->faction))) {
      maxcasualties += s->casualties;
    }
  }
  di = rng_int() % maxcasualties;
  for (s=b->sides;s;s=s->next) {
    for (df=s->fighters;df;df=df->next) {
      /* Geflohene haben auch 0 hp, dürfen hier aber nicht ausgewählt
      * werden! */
      int dead = df->unit->number - (df->alive + df->run.number);
      if (!playerrace(df->unit->race)) continue;

      if (af && !helping(af->side, df->side))
        continue;
      if (di < dead) {
        return df;
      }
      di -= dead;
    }
  }

  return NULL;
}

boolean
helping(const side * as, const side * ds)
{
  if (as->bf->faction==ds->bf->faction) return true;
  return (boolean)(!enemy(as, ds) && allysf(as, ds->bf->faction));
}

int
statusrow(int status)
{
  switch (status) {
  case ST_AGGRO:
  case ST_FIGHT:
    return FIGHT_ROW;
  case ST_BEHIND:
  case ST_CHICKEN:
    return BEHIND_ROW;
  case ST_AVOID:
    return AVOID_ROW;
  case ST_FLEE:
    return FLEE_ROW;
  default:
    assert(!"unknown combatrow");
  }
  return FIGHT_ROW;
}

static double
hpflee(int status)
  /* if hp drop below this percentage, run away */
{
  switch (status) {
  case ST_AGGRO:
    return 0.0;
  case ST_FIGHT:
  case ST_BEHIND:
    return 0.2;
  case ST_CHICKEN:
  case ST_AVOID:
    return 0.9;
  case ST_FLEE:
    return 1.0;
  default:
    assert(!"unknown combatrow");
  }
  return 0.0;
}

static int
get_row(const side * s, int row, const side * vs)
{
  boolean counted[MAXSIDES];
  int enemyfront = 0;
  int line, result;
  int retreat = 0;
  int size[NUMROWS];
  int front = 0;

  memset(counted, 0, sizeof(counted));
  memset(size, 0, sizeof(size));
  for (line=FIRST_ROW;line!=NUMROWS;++line) {
    int si;
    side *sa;
    /* how many enemies are there in the first row? */
    for (si=0;s->enemies[si];++si) {
      side *se = s->enemies[si];
      if (se->size[line]>0) {
        enemyfront += se->size[line]; 
        /* - s->nonblockers[line] (nicht, weil angreifer) */
      }
    }
    for (sa = s->battle->sides; sa; sa=sa->next) {
      /* count people that like me, but don't like my enemy */
      if (friendly(sa, s) && enemy(sa, vs)) {
        if (!counted[sa->index]) {
          int i;
          
          for (i=0;i!=NUMROWS;++i) {
            size[i] += sa->size[i] - sa->nonblockers[i];
          }
          counted[sa->index] = true;
        }
      }
    }
    if (enemyfront) break;
  }
  if (enemyfront) {
    for (line=FIRST_ROW;line!=NUMROWS;++line) {
      front += size[line];
      if (!front || front<enemyfront/ROW_FACTOR) ++retreat;
      else if (front) break;
    }
  }

  /* every entry in the size[] array means someone trying to defend us.
  * 'retreat' is the number of rows falling.
  */
  result = max(FIRST_ROW, row - retreat);

  return result;
}

int
get_unitrow(const fighter * af, const side * vs)
{
  int row = statusrow(af->status);
  if (vs==NULL) {
    int i;
    for (i=FIGHT_ROW;i!=row;++i) if (af->side->size[i]) break;
    return FIGHT_ROW+(row-i);
  }
  return get_row(af->side, row, vs);
}

static void
reportcasualties(battle * b, fighter * fig, int dead)
{
  struct message * m;
  if (fig->alive == fig->unit->number) return;
  if (fig->run.region == NULL) {
    fig->run.region = fleeregion(fig->unit);
    if (fig->run.region == NULL) fig->run.region = b->region;
  }
  m = msg_message("casualties", "unit runto run alive fallen",
    fig->unit, fig->run.region, fig->run.number, fig->alive, dead);
  message_all(b, m);
  msg_release(m);
}


static int
contest(int skilldiff, const armor_type * ar, const armor_type * sh)
{
  int p, vw = BASE_CHANCE - TDIFF_CHANGE * skilldiff;
  double mod = 1.0;

  if (ar != NULL)
    mod *= (1 + ar->penalty);
  if (sh != NULL)
    mod *= (1 + sh->penalty);
  vw = (int)(100 - ((100 - vw) * mod));

  do {
    p = rng_int() % 100;
    vw -= p;
  }
  while (vw >= 0 && p >= 90);
  return (vw <= 0);
}

static boolean
riding(const troop t) {
  if (t.fighter->building!=NULL) return false;
  if (t.fighter->horses + t.fighter->elvenhorses > t.index) return true;
  return false;
}

static weapon *
preferred_weapon(const troop t, boolean attacking)
{
  weapon * missile = t.fighter->person[t.index].missile;
  weapon * melee = t.fighter->person[t.index].melee;
  if (attacking) {
    if (melee==NULL || (missile && missile->attackskill>melee->attackskill)) {
      return missile;
    }
  } else {
    if (melee==NULL || (missile && missile->defenseskill>melee->defenseskill)) {
      return missile;
    }
  }
  return melee;
}

static weapon *
select_weapon(const troop t, boolean attacking, boolean ismissile)
  /* select the primary weapon for this trooper */
{
  if (attacking) {
    if (ismissile) {
      /* from the back rows, have to use your missile weapon */
      return t.fighter->person[t.index].missile;
    }
  } else {
    if (!ismissile) {
      /* have to use your melee weapon if it's melee */
      return t.fighter->person[t.index].melee;
    }
  }
  return preferred_weapon(t, attacking);
}

static int
weapon_skill(const weapon_type * wtype, const unit * u, boolean attacking)
  /* the 'pure' skill when using this weapon to attack or defend.
   * only undiscriminate modifiers (not affected by troops or enemies)
   * are taken into account, e.g. no horses, magic, etc. */
{
  int skill;

  if (wtype==NULL) {
    skill = effskill(u, SK_WEAPONLESS);
    if (skill<=0) {
      /* wenn kein waffenloser kampf, dann den rassen-defaultwert */
      if(u->race == new_race[RC_URUK]) {
        int sword = effskill(u, SK_MELEE);
        int spear = effskill(u, SK_SPEAR);
        skill = max(sword, spear) - 3;
        if (attacking) {
          skill = max(skill, u->race->at_default);
        } else {
          skill = max(skill, u->race->df_default);
        }
      } else {
        if (attacking) {
          skill = u->race->at_default;
        } else {
          skill = u->race->df_default;
        }
      }
    } else {
      /* der rassen-defaultwert kann höher sein als der Talentwert von
       * waffenloser kampf */
      if (attacking) {
        if (skill < u->race->at_default) skill = u->race->at_default;
      } else {
        if (skill < u->race->df_default) skill = u->race->df_default;
      }
    }
    if (attacking) {
      skill += u->race->at_bonus;
    } else {
      skill += u->race->df_bonus;
    }
  } else {
    /* changed: if we own a weapon, we have at least a skill of 0 */
    skill = effskill(u, wtype->skill);
    if (skill < wtype->minskill) skill = 0;
    if (skill > 0) {
      if(attacking) {
        skill += u->race->at_bonus;
      } else {
        skill += u->race->df_bonus;
      }
    }
    if (attacking) {
      skill += wtype->offmod;
    } else {
      skill += wtype->defmod;
    }
  }

  return skill;
}

static int
weapon_effskill(troop t, troop enemy, const weapon * w, boolean attacking, boolean missile)
  /* effektiver Waffenskill während des Kampfes */
{
  /* In dieser Runde alle die Modifier berechnen, die fig durch die
   * Waffen bekommt. */
  fighter * tf = t.fighter;
  unit * tu = t.fighter->unit;
  int skill;
  const weapon_type * wtype = w?w->type:NULL;

  if (wtype==NULL) {
    /* Ohne Waffe: Waffenlose Angriffe */
    skill = weapon_skill(NULL, tu, attacking);
  } else {
    if (attacking) {
      skill = w->attackskill;
    } else {
      skill = w->defenseskill;
    }
    if (wtype->modifiers!=NULL) {
      /* Pferdebonus, Lanzenbonus, usw. */
      int m;
      unsigned int flags = WMF_SKILL|(attacking?WMF_OFFENSIVE:WMF_DEFENSIVE);

      if (riding(t)) flags |= WMF_RIDING;
      else flags |= WMF_WALKING;
      if (riding(enemy)) flags |= WMF_AGAINST_RIDING;
      else flags |= WMF_AGAINST_WALKING;

      for (m=0;wtype->modifiers[m].value;++m) {
        if ((wtype->modifiers[m].flags & flags) == flags) {
          race_list * rlist = wtype->modifiers[m].races;
          if (rlist!=NULL) {
            while (rlist) {
              if (rlist->data == tu->race) break;
              rlist = rlist->next;
            }
            if (rlist==NULL) continue;
          }
          skill += wtype->modifiers[m].value;
        }
      }
    }
  }

  /* Burgenbonus, Pferdebonus */
  if (riding(t) && (wtype==NULL || !fval(wtype, WTF_MISSILE))) {
    skill += 2;
    if (wtype) skill = skillmod(urace(tu)->attribs, tu, tu->region, wtype->skill, skill, SMF_RIDING);
  }

  if (t.index<tf->elvenhorses) {
    /* Elfenpferde: Helfen dem Reiter, egal ob und welche Waffe. Das ist
     * eleganter, und vor allem einfacher, sonst muß man noch ein
     * WMF_ELVENHORSE einbauen. */
    skill += 2;
  }

  if (skill>0 && !attacking && missile) {
    /*
     * Wenn ich verteidige, und nicht direkt meinem Feind gegenüberstehe,
     * halbiert sich mein Skill: (z.B. gegen Fernkämpfer. Nahkämpfer
     * können mich eh nicht treffen)
     */
    skill /= 2;
  }
  return skill;
}

static const armor_type *
select_armor(troop t, boolean shield)
{
  unit * u = t.fighter->unit;
  const armor * a = t.fighter->armors;
  int geschuetzt = 0;

  /* Drachen benutzen keine Rüstungen */
  if (!(u->race->battle_flags & BF_EQUIPMENT))
    return NULL;

  /* ... und Werwölfe auch nicht */
  if (fval(u, UFL_WERE)) {
    return NULL;
  }

  for (;a;a=a->next) {
    if (a->atype->flags & ATF_SHIELD) {
      geschuetzt += a->count;
      if (geschuetzt > t.index) /* unser Kandidat wird geschuetzt */
        return a->atype;
    }
  }

  return NULL;
}


/* Hier ist zu beachten, ob und wie sich Zauber und Artefakte, die
 * Rüstungschutz geben, addieren.
 * - Artefakt I_TROLLBELT gibt Rüstung +1
 * - Zauber Rindenhaut gibt Rüstung +3
 */
int
select_magicarmor(troop t)
{
  unit *u = t.fighter->unit;
  int geschuetzt = 0;
  int ma = 0;

  geschuetzt = min(get_item(u, I_TROLLBELT), u->number);

  if (geschuetzt > t.index) /* unser Kandidat wird geschuetzt */
    ma += 1;

  return ma;
}

/* Sind side ds und Magier des meffect verbündet, dann return 1*/
boolean
meffect_protection(battle * b, meffect * s, side * ds)
{
  if (!s->magician->alive) return false;
  if (s->duration <= 0) return false;
  if (enemy(s->magician->side, ds)) return false;
  if (allysf(s->magician->side, ds->bf->faction)) return true;
  return false;
}

/* Sind side as und Magier des meffect verfeindet, dann return 1*/
boolean
meffect_blocked(battle *b, meffect *s, side *as)
{
  if (!s->magician->alive) return false;
  if (s->duration <= 0) return false;
  if (enemy(s->magician->side, as)) return true;
  return false;
}

/* rmfighter wird schon im PRAECOMBAT gebraucht, da gibt es noch keine
 * troops */
void
rmfighter(fighter *df, int i)
{
  side *ds = df->side;

  /* nicht mehr personen abziehen, als in der Einheit am Leben sind */
  assert(df->alive >= i);
  assert(df->alive <= df->unit->number);

  /* erst ziehen wir die Anzahl der Personen von den Kämpfern in der
  * Schlacht, dann von denen auf dieser Seite ab*/
  df->side->alive -= i;
  df->side->battle->alive -= i;

  /* Dann die Kampfreihen aktualisieren */
  ds->size[SUM_ROW] -= i;
  ds->size[statusrow(df->status)] -= i;

  /* Spezialwirkungen, z.B. Schattenritter */
  if (df->unit->race->battle_flags & BF_NOBLOCK) {
    ds->nonblockers[SUM_ROW] -= i;
    ds->nonblockers[statusrow(df->status)] -= i;
  }

  /* und die Einheit selbst aktualisieren */
  df->alive -= i;
}


void
rmtroop(troop dt)
{
  fighter *df = dt.fighter;

  /* troop ist immer eine einzele Person */
  rmfighter(df, 1);

  assert(dt.index >= 0 && dt.index < df->unit->number);
  df->person[dt.index] = df->person[df->alive - df->removed];
  if (df->removed) {
    df->person[df->alive - df->removed] = df->person[df->alive];
  }
  df->person[df->alive].hp = 0;
}

void
remove_troop(troop dt)
{
  fighter * df = dt.fighter;
  unit * du = df->unit;

  rmtroop(dt);
  if (!df->alive) {
    char eqname[64];
    const struct equipment * eq;
    if (du->race->itemdrop) {
      item * drops = du->race->itemdrop(du->race, du->number-df->run.number);

      if (drops != NULL){
        i_merge(&du->items, &drops);
      }
    }
    sprintf(eqname, "%s_spoils", du->race->_name[0]);
    eq = get_equipment(eqname);
    if (eq!=NULL) {
      equip_items(&du->items, eq);
    }
  }
}

/** reduces the target's exp by an equivalent of n points learning
 * 30 points = 1 week
 */
void
drain_exp(struct unit *u, int n)
{
  skill_t sk = (skill_t)(rng_int() % MAXSKILLS);
  skill_t ssk;

  ssk = sk;

  while (get_level(u, sk)==0) {
    sk++;
    if (sk == MAXSKILLS)
      sk = 0;
    if (sk == ssk) {
      sk = NOSKILL;
      break;
    }
  }
  if (sk != NOSKILL) {
    skill * sv = get_skill(u, sk);
    while (n>0) {
      if (n>=30*u->number) {
        reduce_skill(u, sv, 1);
        n-=30;
      } else {
        if (rng_int()%(30*u->number)<n) reduce_skill(u, sv, 1);
        n = 0;
      }
    }
  }
}

const char *
rel_dam(int dam, int hp)
{
  double q = (double)dam/(double)hp;

  if (q > 0.75) {
    return "eine klaffende Wunde";
  } else if (q > 0.5) {
    return "eine schwere Wunde";
  } else if (q > 0.25) {
    return "eine Wunde";
  }
  return "eine kleine Wunde";
}

boolean
terminate(troop dt, troop at, int type, const char *damage, boolean missile)
{
#ifdef SMALL_BATTLE_MESSAGES
  char smallbuf[512];
#endif
  item ** pitm;
  fighter *df = dt.fighter;
  fighter *af = at.fighter;
  unit *au = af->unit;
  unit *du = df->unit;
  battle *b = df->side->battle;
  int heiltrank = 0;
  char debugbuf[512];

  /* Schild */
  void **si;
  side *ds = df->side;
  int hp;

  int ar = 0, an, am;
  const armor_type * armor = select_armor(dt, true);
  const armor_type * shield = select_armor(dt, false);

  const weapon_type *dwtype = NULL;
  const weapon_type *awtype = NULL;
  const weapon * weapon;

  int rda, sk = 0, sd;
  boolean magic = false;
  int da = dice_rand(damage);

  assert(du->number>0);
#ifdef SHOW_KILLS
  ++at.fighter->hits;
#endif

#ifdef KARMA_MODULE
  if (fval(au, UFL_WERE)) {
    int level = fspecial(du->faction, FS_LYCANTROPE);
    da += level;
  }
#endif /* KARMA_MODULE */

  switch (type) {
  case AT_STANDARD:
    weapon = select_weapon(at, true, missile);
    sk = weapon_effskill(at, dt, weapon, true, missile);
    if (weapon) awtype = weapon->type;
    if (awtype && fval(awtype, WTF_MAGICAL)) magic = true;
    break;
  case AT_NATURAL:
    sk = weapon_effskill(at, dt, NULL, true, missile);
    break;
  case AT_SPELL:
  case AT_COMBATSPELL:
    magic = true;
    break;
  default:
    break;
  }
  weapon = select_weapon(dt, false, true); /* missile=true to get the unmodified best weapon she has */
  sd = weapon_effskill(dt, at, weapon, false, false);
  if (weapon!=NULL) dwtype=weapon->type;

  if (armor) ar += armor->prot;
  if (shield) ar += shield->prot;

  /* natürliche Rüstung */
  an = du->race->armor;

  /* magische Rüstung durch Artefakte oder Sprüche */
  /* Momentan nur Trollgürtel und Werwolf-Eigenschaft */
  am = select_magicarmor(dt);

#ifdef KARMA_MODULE
  if(fval(du, UFL_WERE)) {
    /* this counts as magical armor */
    int level = fspecial(du->faction, FS_LYCANTROPE);
    am += level;
  }
#endif /* KARMA_MODULE */

#if CHANGED_CROSSBOWS == 1
  if(awtype && fval(awtype,WTF_ARMORPIERCING)) {
    /* crossbows */
    ar /= 2;
    an /= 2;
  }
#endif

  /* natürliche Rüstung ist halbkumulativ */
  if (ar>0) {
    ar += an/2;
  } else {
    ar = an;
  }
  ar += am;

  if (type!=AT_COMBATSPELL && type!=AT_SPELL) {
    /* Kein Zauber, normaler Waffenschaden */
    double kritchance = (sk * 3 - sd) / 200.0;
#ifdef KARMA_MODULE
    int faerie_level = fspecial(du->faction, FS_FAERIE);

    da += jihad(au->faction, du->race);
    if (type == AT_STANDARD && faerie_level) {
      int c;

      for (c=0;weapon->type->itype->construction->materials[c].number; c++) {
        if(weapon->type->itype->construction->materials[c].rtype == oldresourcetype[R_IRON]) {
          da += faerie_level;
          break;
        }
      }
    }
#endif /* KARMA_MODULE */

    kritchance = max(kritchance, 0.005);
    kritchance = min(0.9, kritchance);

    while (chance(kritchance)) {
      sprintf(debugbuf,
        "%s/%d landet einen kritischen Treffer", unitid(au), at.index);
      battledebug(debugbuf);
      da += dice_rand(damage);
    }

    da += rc_specialdamage(au->race, du->race, awtype);

    if (awtype!=NULL && fval(awtype, WTF_MISSILE)) {
      /* Fernkampfschadenbonus */
      da += af->person[at.index].damage_rear;
    } else if (awtype==NULL) {
      /* Waffenloser kampf, bonus von talentwert*/
      da += effskill(au, SK_WEAPONLESS);
    } else {
      /* Nahkampfschadensbonus */
      da += af->person[at.index].damage;
    }

    /* Skilldifferenzbonus */
    da += max(0, (sk-sd)/DAMAGE_QUOTIENT);
  }


  if (magic) {
    /* Magischer Schaden durch Spruch oder magische Waffe */
    double res = 1.0;

    /* magic_resistance gib x% Resistenzbonus zurück */
    res -= magic_resistance(du)*3.0;

    if (du->race->battle_flags & BF_EQUIPMENT) {
#ifdef TODO_RUNESWORD
      /* Runenschwert gibt im Kampf 80% Resistenzbonus */
      if (dwp == WP_RUNESWORD) res -= 0.80;
#endif
      /* der Effekt von Laen steigt nicht linear */
      if (armor && fval(armor, ATF_LAEN)) res *= (1-armor->magres);
      if (shield && fval(shield, ATF_LAEN)) res *= (1-shield->magres);
      if (dwtype) res *= (1-dwtype->magres);
    }

    if (res > 0) {
      da = (int) (max(da * res, 0));
    }
    /* gegen Magie wirkt nur natürliche und magische Rüstung */
    ar = an+am;
  }

  rda = max(da - ar,0);

  if ((du->race->battle_flags & BF_INV_NONMAGIC) && !magic) rda = 0;
  else {
    unsigned int i = 0;
    if (du->race->battle_flags & BF_RES_PIERCE) i |= WTF_PIERCE;
    if (du->race->battle_flags & BF_RES_CUT) i |= WTF_CUT;
    if (du->race->battle_flags & BF_RES_BASH) i |= WTF_BLUNT;

    if (i && awtype && fval(awtype, i)) rda /= 2;

    /* Schilde */
    for (si = b->meffects.begin; si != b->meffects.end; ++si) {
      meffect *meffect = *si;
      if (meffect_protection(b, meffect, ds) != 0) {
        assert(0 <= rda); /* rda sollte hier immer mindestens 0 sein */
        /* jeder Schaden wird um effect% reduziert bis der Schild duration
        * Trefferpunkte aufgefangen hat */
        if (meffect->typ == SHIELD_REDUCE) {
          hp = rda * (meffect->effect/100);
          rda -= hp;
          meffect->duration -= hp;
        }
        /* gibt Rüstung +effect für duration Treffer */
        if (meffect->typ == SHIELD_ARMOR) {
          rda = max(rda - meffect->effect, 0);
          meffect->duration--;
        }
      }
    }
  }

  sprintf(debugbuf, "Verursacht %dTP, Rüstung %d: %d -> %d HP",
    da, ar, df->person[dt.index].hp, df->person[dt.index].hp - rda);

#ifdef SMALL_BATTLE_MESSAGES
  if (b->small) {
    if (rda > 0) {
      sprintf(smallbuf, "Der Treffer verursacht %s",
        rel_dam(rda, df->person[dt.index].hp));
    } else {
      sprintf(smallbuf, "Der Treffer verursacht keinen Schaden");
    }
  }
#endif

  assert(dt.index<du->number);
  df->person[dt.index].hp -= rda;

  if (df->person[dt.index].hp > 0) {  /* Hat überlebt */
    battledebug(debugbuf);
    if (au->race == new_race[RC_DAEMON]) {
#ifdef TODO_RUNESWORD
      if (select_weapon(dt, 0, -1) == WP_RUNESWORD) continue;
#endif
      if (!(df->person[dt.index].flags & (FL_COURAGE|FL_DAZZLED))) {
        df->person[dt.index].flags |= FL_DAZZLED;
        df->person[dt.index].defence--;
      }
    }
    df->person[dt.index].flags = (df->person[dt.index].flags & ~FL_SLEEPING);
#ifdef SMALL_BATTLE_MESSAGES
    if (b->small) {
      strcat(smallbuf, ".");
      battlerecord(b, smallbuf);
    }
#endif
    return false;
  }
#ifdef SHOW_KILLS
  ++at.fighter->kills;
#endif

#ifdef SMALL_BATTLE_MESSAGES
  if (b->small) {
    strcat(smallbuf, ", die tödlich ist");
  }
#endif

  /* Sieben Leben */
  if (du->race == new_race[RC_CAT] && (chance(1.0 / 7))) {
#ifdef SMALL_BATTLE_MESSAGES
    if (b->small) {
      strcat(smallbuf, ", doch die Katzengöttin ist gnädig");
      battlerecord(b, smallbuf);
    }
#endif
    assert(dt.index>=0 && dt.index<du->number);
    df->person[dt.index].hp = unit_max_hp(du);
    return false;
  }

  /* Heiltrank schluerfen und hoffen */
  if (get_effect(du, oldpotiontype[P_HEAL]) > 0) {
    change_effect(du, oldpotiontype[P_HEAL], -1);
    heiltrank = 1;
  } else if (i_get(du->items, oldpotiontype[P_HEAL]->itype) > 0) {
    i_change(&du->items, oldpotiontype[P_HEAL]->itype, -1);
    change_effect(du, oldpotiontype[P_HEAL], 3);
    heiltrank = 1;
  }
  if (heiltrank && (chance(0.50))) {
#ifdef SMALL_BATTLE_MESSAGES
    if (b->small) {
      strcat(smallbuf, ", doch ein Heiltrank bringt Rettung");
      battlerecord(b, smallbuf);
    } else
#endif
    {
      message * m = msg_message("battle::potionsave", "unit", du);
      message_faction(b, du->faction, m);
      msg_release(m);
    }
    assert(dt.index>=0 && dt.index<du->number);
    df->person[dt.index].hp = du->race->hitpoints;
    return false;
  }

  strcat(debugbuf, ", tot");
  battledebug(debugbuf);
#ifdef SMALL_BATTLE_MESSAGES
  if (b->small) {
    strcat(smallbuf, ".");
    battlerecord(b, smallbuf);
  }
#endif
  for (pitm=&du->items; *pitm; pitm=&(*pitm)->next) {
    const item_type * itype = (*pitm)->type;
    if (!itype->flags & ITF_CURSED && dt.index < (*pitm)->number) {
      /* 25% Grundchance, das ein Item kaputtgeht. */
      if (rng_int() % 4 < 1) i_change(pitm, itype, -1);
    }
  }
  remove_troop(dt);

  return true;
}

static int
count_side(const side * s, const side * vs, int minrow, int maxrow, int select)
{
  fighter * fig;
  int people = 0;
  int unitrow[NUMROWS];

  if (maxrow<FIGHT_ROW) return 0;
  if (select&SELECT_ADVANCE) {
    memset(unitrow, -1, sizeof(unitrow));
  }

  for (fig = s->fighters; fig; fig = fig->next) {
    if (fig->alive - fig->removed > 0) {
      int row = statusrow(fig->status);
      if (select&SELECT_ADVANCE) {
        if (unitrow[row] == -1) {
          unitrow[row] = get_unitrow(fig, vs);
        }
        row = unitrow[row];
      }
      if (row >= minrow && row <= maxrow) {
        people += fig->alive - fig->removed;
        if (people>0 && (select&SELECT_FIND)) break;
      }
    }
  }
  return people;
}

/* return the number of live allies warning: this function only considers
* troops that are still alive, not those that are still fighting although
* dead. */
int
count_allies(const side * as, int minrow, int maxrow, int select)
{
  battle *b = as->battle;
  side *s;
  int count = 0;

  for (s=b->sides;s;s=s->next) {
    if (!helping(as, s)) continue;
    count += count_side(s, NULL, minrow, maxrow, select);
    if (count>0 && (select&SELECT_FIND)) break;
  }
  return count;
}

int
count_enemies(battle * b, const fighter * af, int minrow, int maxrow, int select)
{
  int i = 0;
  side *es, *as = af->side;

  if (maxrow<FIRST_ROW) return 0;
  for (es = b->sides; es; es = es->next) {
    if (as==NULL || enemy(es, as)) {
      int offset = 0;
      if (select&SELECT_DISTANCE) {
        offset = get_unitrow(af, es) - FIGHT_ROW;
      }
      i += count_side(es, as, minrow-offset, maxrow-offset, select);
      if (i>0 && (select&SELECT_FIND)) break;
    }
  }
  return i;
}

troop
select_enemy(fighter * af, int minrow, int maxrow, int select)
{
  side *as = af->side;
  battle * b = as->battle;
  int si;
  int enemies;

  if (af->unit->race->flags & RCF_FLY) {
    /* flying races ignore min- and maxrow and can attack anyone fighting
    * them */
    minrow = FIGHT_ROW;
    maxrow = BEHIND_ROW;
  }
  minrow = max(minrow, FIGHT_ROW);

  enemies = count_enemies(b, af, minrow, maxrow, select);

  /* Niemand ist in der angegebenen Entfernung? */
  if (enemies<=0) return no_troop;

  enemies = rng_int() % enemies;
  for (si=0;as->enemies[si];++si) {
    side *ds = as->enemies[si];
    fighter * df;
    int unitrow[NUMROWS];
    int offset = 0;

    if (select&SELECT_DISTANCE) offset = get_unitrow(af, ds) - FIGHT_ROW;

    if (select&SELECT_ADVANCE) {
      int ui;
      for (ui=0;ui!=NUMROWS;++ui) unitrow[ui] = -1;
    }

    for (df=ds->fighters; df ; df = df->next) {
      int dr;

      dr = statusrow(df->status);
      if (select&SELECT_ADVANCE) {
        if (unitrow[dr]<0) {
          unitrow[dr] = get_unitrow(df, as);
        }
        dr = unitrow[dr];
      }

      if (select&SELECT_DISTANCE) dr += offset;
      if (dr < minrow || dr > maxrow) continue;
      if (df->alive - df->removed > enemies) {
        troop dt;
        dt.index = enemies;
        dt.fighter = df;
        return dt;
      }
      else enemies -= (df->alive - df->removed);
    }
  }
  assert(!enemies);
  return no_troop;
}

static troop
select_opponent(battle * b, troop at, int mindist, int maxdist)
{
  fighter * af = at.fighter;
  troop dt;

  if (af->unit->race->flags & RCF_FLY) {
    /* flying races ignore min- and maxrow and can attack anyone fighting
    * them */
    dt = select_enemy(at.fighter, FIGHT_ROW, BEHIND_ROW, SELECT_ADVANCE);
  } else {
    mindist = max(mindist, FIGHT_ROW);
    dt = select_enemy(at.fighter, mindist, maxdist, SELECT_ADVANCE);
  }

  return dt;
}

/*
 * Abfrage mit
 *
 * cvector *fgs=fighters(b,af,FIGHT_ROW,BEHIND_ROW, FS_HELP|FS_ENEMY);
 * fighter *fig;
 *
 * Optional: Verwirbeln. Vorsicht: Aufwändig!
 * v_scramble(fgs->begin, fgs->end);
 *
 * for (fig = fgs->begin; fig != fgs->end; ++fig) {
 *     fighter *df = *fig;
 *
 * }
 *
 * cv_kill(fgs); free(fgs); Nicht vergessen
 */

cvector *
fighters(battle *b, const side * vs, int minrow, int maxrow, int mask)
{
  side * s;
  cvector *fightervp = malloc(sizeof(cvector));

  assert(vs!=NULL);
  cv_init(fightervp);

  for (s = b->sides; s; s = s->next) {
    fighter *fig;
    for (fig = s->fighters; fig; fig = fig->next) {
      int row = get_unitrow(fig, vs);
      if (row >= minrow && row <= maxrow) {
        switch (mask) {
        case FS_ENEMY:
          if (enemy(fig->side, vs)) cv_pushback(fightervp, fig);
          break;
        case FS_HELP:
          if (!enemy(fig->side, vs) && allysf(fig->side, vs->bf->faction))
            cv_pushback(fightervp, fig);
          break;
        case FS_HELP|FS_ENEMY:
          cv_pushback(fightervp, fig);
          break;
        default:
          assert(0 || !"Ungültiger Allianzstatus in fighters()");
        }
      }
    }
  }

  return fightervp;
}

static void
report_failed_spell(battle * b, unit * mage, const spell * sp)
{
  message * m = msg_message("battle::spell_failed", "unit spell", mage, sp);
  message_all(b, m);
  msg_release(m);
}

void
do_combatmagic(battle *b, combatmagic_t was)
{
  side * s;
  region *r = b->region;
  castorder *co;
  castorder *cll[MAX_SPELLRANK];
  int level;
  int spellrank;
  int sl;

  for (spellrank = 0; spellrank < MAX_SPELLRANK; spellrank++) {
    cll[spellrank] = (castorder*)NULL;
  }

  for (s = b->sides; s; s = s->next) {
    fighter * fig;
    for (fig = s->fighters; fig; fig = fig->next) {
      unit * mage = fig->unit;

      if (fig->alive <= 0) continue; /* fighter kann im Kampf getötet worden sein */

      level = eff_skill(mage, SK_MAGIC, r);
      if (level > 0) {
        double power;
        const spell *sp;
        const struct locale * lang = mage->faction->locale;
        char cmd[128];
        order * ord;

        switch(was) {
        case DO_PRECOMBATSPELL:
          sp = get_combatspell(mage, 0);
          sl = get_combatspelllevel(mage, 0);
          break;
        case DO_POSTCOMBATSPELL:
          sp = get_combatspell(mage, 2);
          sl = get_combatspelllevel(mage, 2);
          break;
        default:
          /* Fehler! */
          return;
        }
        if (sp == NULL)
          continue;

        snprintf(cmd, 128, "%s \"%s\"",
          LOC(lang, keywords[K_CAST]), spell_name(sp, lang));

        ord = parse_order(cmd, lang);
        if (cancast(mage, sp, 1, 1, ord) == false) {
          free_order(ord);
          continue;
        }

        level = eff_spelllevel(mage, sp, level, 1);
        if (sl > 0) level = min(sl, level);
        if (level < 0) {
          report_failed_spell(b, mage, sp);
          free_order(ord);
          continue;
        }

        power = spellpower(r, mage, sp, level, ord);
        free_order(ord);
        if (power <= 0) { /* Effekt von Antimagie */
          report_failed_spell(b, mage, sp);
          pay_spell(mage, sp, level, 1);
        } else if (fumble(r, mage, sp, sp->level) == true) {
          report_failed_spell(b, mage, sp);
          pay_spell(mage, sp, level, 1);
        } else {
          co = new_castorder(fig, 0, sp, r, level, power, 0, 0, 0);
          add_castorder(&cll[(int)(sp->rank)], co);
        }
      }
    }
  }
  for (spellrank = 0; spellrank < MAX_SPELLRANK; spellrank++) {
    for (co = cll[spellrank]; co; co = co->next) {
      fighter * fig = co->magician.fig;
      const spell * sp = co->sp;
      int level = co->level;
      double power = co->force;

      if (sp->sp_function==NULL) {
        log_error(("spell '%s' has no function.\n", sp->sname));
      } else {
        level = ((cspell_f)sp->sp_function)(fig, level, power, sp);
        if (level > 0) {
          pay_spell(fig->unit, sp, level, 1);
        }
      }
    }
  }
  for (spellrank = 0; spellrank < MAX_SPELLRANK; spellrank++) {
    free_castorders(cll[spellrank]);
  }
}


static void
do_combatspell(troop at)
{
  const spell *sp;
  fighter *fi = at.fighter;
  unit *mage = fi->unit;
  battle *b = fi->side->battle;
  region *r = b->region;
  int level;
  double power;
  int fumblechance = 0;
  void **mg;
  order * ord;
  int sl;
  char cmd[128];
  const struct locale * lang = mage->faction->locale;

  sp = get_combatspell(mage, 1);
  if (sp == NULL) {
    fi->magic = 0; /* Hat keinen Kampfzauber, kämpft nichtmagisch weiter */
    return;
  }
  snprintf(cmd, 128, "%s \"%s\"",
    LOC(lang, keywords[K_CAST]), spell_name(sp, lang));
  ord = parse_order(cmd, lang);
  if (cancast(mage, sp, 1, 1, ord) == false) {
    fi->magic = 0; /* Kann nicht mehr Zaubern, kämpft nichtmagisch weiter */
    return;
  }

  level = eff_spelllevel(mage, sp, fi->magic, 1);
  if ((sl = get_combatspelllevel(mage, 1)) > 0) level = min(level, sl);

  if (fumble(r, mage, sp, sp->level) == true) {
    report_failed_spell(b, mage, sp);
    pay_spell(mage, sp, level, 1);
    return;
  }

  for (mg = b->meffects.begin; mg != b->meffects.end; ++mg) {
    meffect *mblock = *mg;
    if (mblock->typ == SHIELD_BLOCK) {
      if (meffect_blocked(b, mblock, fi->side) != 0) {
        fumblechance += mblock->duration;
        mblock->duration -= mblock->effect;
      }
    }
  }

  /* Antimagie die Fehlschlag erhöht */
  if (rng_int()%100 < fumblechance) {
    report_failed_spell(b, mage, sp);
    pay_spell(mage, sp, level, 1);
    free_order(ord);
    return;
  }
  power = spellpower(r, mage, sp, level, ord);
  free_order(ord);
  if (power <= 0) { /* Effekt von Antimagie */
    report_failed_spell(b, mage, sp);
    pay_spell(mage, sp, level, 1);
    return;
  }

  if (sp->sp_function==NULL) {
    log_error(("spell '%s' has no function.\n", sp->sname));
  } else {
    level = ((cspell_f)sp->sp_function)(fi, level, power, sp);
    if (level > 0) {
      pay_spell(mage, sp, level, 1);
#ifndef SIMPLE_COMBAT
      at.fighter->action_counter++;
      at.fighter->side->bf->lastturn = b->turn;
#endif
    }
  }
}


/* Sonderattacken: Monster patzern nicht und zahlen auch keine
 * Spruchkosten. Da die Spruchstärke direkt durch den Level bestimmt
 * wird, wirkt auch keine Antimagie (wird sonst in spellpower
 * gemacht) */

static void
do_extra_spell(troop at, const att *a)
{
  const spell *sp = a->data.sp;
  fighter *fi = at.fighter;
  unit *au = fi->unit;
  int power;

  /* nur bei Monstern können mehrere 'Magier' in einer Einheit sein */
  power = sp->level * au->number;
  if (sp->sp_function==NULL) {
    log_error(("spell '%s' has no function.\n", sp->sname));
  } else {
    ((cspell_f)sp->sp_function)(fi, sp->level, power, sp);
  }
}

static int
skilldiff(troop at, troop dt, int dist)
{
  fighter *af = at.fighter, *df = dt.fighter;
  unit *au = af->unit, *du = df->unit;
  int is_protected = 0, skdiff = 0, sk;
  weapon * awp = select_weapon(at, true, dist>1);
  weapon * dwp = select_weapon(dt, false, dist>1);

  if (df->person[dt.index].flags & FL_SLEEPING)
    skdiff += 2;

  /* Effekte durch Rassen */
  if (awp!=NULL && au->race == new_race[RC_HALFLING] && dragonrace(du->race)) {
    skdiff += 5;
  }

#ifdef KARMA_MODULE
  /* Werwolf */
  if(fval(au, UFL_WERE)) {
    skdiff += fspecial(au->faction, FS_LYCANTROPE);
  }
#endif /* KARMA_MODULE */

  if (au->race == new_race[RC_GOBLIN] &&
      af->side->size[SUM_ROW] >= df->side->size[SUM_ROW] * 10)
    skdiff += 1;

#ifdef KARMA_MODULE
  /* TODO this should be a skillmod */
  skdiff += jihad(au->faction, du->race);
#endif
  skdiff += af->person[at.index].attack;
  skdiff -= df->person[dt.index].defence;

  if (df->building) {
    boolean init = false;
    static const curse_type * strongwall_ct, * magicwalls_ct;
    if (!init) {
      strongwall_ct = ct_find("strongwall");
      magicwalls_ct = ct_find("magicwalls");
      init=true;
    }
    if (df->building->type->flags & BTF_PROTECTION) {
      int beff = buildingeffsize(df->building, false)-1;
      /* -1 because the tradepost has no protection value */

#ifdef KARMA_MODULE
      if (fspecial(au->faction, FS_SAPPER)) {
        /* Halbe Schutzwirkung, aufgerundet */
        beff = (beff+1)/2;
      }
#endif /* KARMA_MODULE */
      skdiff -= beff;
      is_protected = 2;
    }
    if (strongwall_ct) {
      curse * c = get_curse(df->building->attribs, strongwall_ct);
      if (curse_active(c)) {
        /* wirkt auf alle Gebäude */
        skdiff -= curse_geteffect(c);
        is_protected = 2;
      }
    }
    if (magicwalls_ct && curse_active(get_curse(df->building->attribs, magicwalls_ct))) {
      /* Verdoppelt Burgenbonus */
      skdiff -= buildingeffsize(df->building, false);
    }
  }
  /* Goblin-Verteidigung
   * ist direkt in der Rassentabelle als df_default
  */

  /* Effekte der Waffen */
  skdiff += weapon_effskill(at, dt, awp, true, dist>1);
  if (awp && fval(awp->type, WTF_MISSILE)) {
    skdiff -= is_protected;
    if (awp->type->modifiers) {
      int w;
      for (w=0;awp->type->modifiers[w].value!=0;++w) {
        if (awp->type->modifiers[w].flags & WMF_MISSILE_TARGET) {
                                  /* skill decreases by targeting difficulty (bow -2, catapult -4) */
          skdiff -= awp->type->modifiers[w].value;
          break;
        }
      }
    }
  }
  sk = weapon_effskill(dt, at, dwp, false, dist>1);
  skdiff -= sk;
  return skdiff;
}

static int
setreload(troop at)
{
  fighter * af = at.fighter;
  const weapon_type * wtype = af->person[at.index].missile->type;
  if (wtype->reload == 0) return 0;
  return af->person[at.index].reload = wtype->reload;
}

int
getreload(troop at)
{
  return at.fighter->person[at.index].reload;
}

#ifdef SMALL_BATTLE_MESSAGES
static char *
attack_message(const troop at, const troop dt, const weapon * wp, int dist)
{
  static char smallbuf[512];
  char a_unit[NAMESIZE+8], d_unit[NAMESIZE+8];
        const char *noweap_string[4] = {"schlägt nach",
          "tritt nach",
          "beißt nach",
          "kratzt nach"};

  if (at.fighter->unit->number > 1)
    sprintf(a_unit, "%s/%d", unitname(at.fighter->unit), at.index);
  else
    sprintf(a_unit, "%s", unitname(at.fighter->unit));

  if (dt.fighter->unit->number > 1)
    sprintf(d_unit, "%s/%d", unitname(dt.fighter->unit), dt.index);
  else
    sprintf(d_unit, "%s", unitname(dt.fighter->unit));

  if (wp == NULL) {
    sprintf(smallbuf, "%s %s %s",
      a_unit, noweap_string[rng_int()%4], d_unit);
    return smallbuf;
  }

  if (dist > 1 || wp->type->missile) {
    sprintf(smallbuf, "%s schießt mit %s auf %s",
      a_unit,
      locale_string(default_locale, resourcename(wp->type->itype->rtype, GR_INDEFINITE_ARTICLE)), d_unit);
  } else {
    sprintf(smallbuf, "%s schlägt mit %s nach %s",
      a_unit,
      locale_string(default_locale, resourcename(wp->type->itype->rtype, GR_INDEFINITE_ARTICLE)), d_unit);
  }

  return smallbuf;
}
#endif

int
hits(troop at, troop dt, weapon * awp)
{
#ifdef SMALL_BATTLE_MESSAGES
  char * smallbuf = NULL;
  battle * b = at.fighter->side->battle;
#endif
  fighter *af = at.fighter, *df = dt.fighter;
  unit *au = af->unit, *du = df->unit;
  char debugbuf[512];
  const armor_type * armor, * shield;
  int skdiff = 0;
  int dist = get_unitrow(af, df->side) + get_unitrow(df, af->side) - 1;
  weapon * dwp = select_weapon(dt, false, dist>1);

  if (!df->alive) return 0;
  if (getreload(at)) return 0;
  if (dist>1 && (awp == NULL || !fval(awp->type, WTF_MISSILE))) return 0;

  /* mark this person as hit. */
  df->person[dt.index].flags |= FL_HIT;

  if (af->person[at.index].flags & FL_STUNNED) {
      af->person[at.index].flags &= ~FL_STUNNED;
    return 0;
  }
  if ((af->person[at.index].flags & FL_TIRED && rng_int()%100 < 50)
      || (af->person[at.index].flags & FL_SLEEPING))
    return 0;

  /* effect of sp_reeling_arrows combatspell */
  if (af->side->battle->reelarrow && awp && fval(awp->type, WTF_MISSILE) && rng_double() < 0.5) {
    return 0;
  }

  skdiff = skilldiff(at, dt, dist);

  /* Verteidiger bekommt eine Rüstung */
  armor = select_armor(dt, true);
  shield = select_armor(dt, false);
  sprintf(debugbuf, "%.4s/%d [%6s/%d] attackiert %.4s/%d [%6s/%d] mit %d dist %d",
      unitid(au), at.index,
      (awp != NULL) ?
        locale_string(default_locale, resourcename(awp->type->itype->rtype, 0)) : "unbewaffnet",
      weapon_effskill(at, dt, awp, true, dist>1),
      unitid(du), dt.index,
      (dwp != NULL) ?
        locale_string(default_locale, resourcename(dwp->type->itype->rtype, 0)) : "unbewaffnet",
      weapon_effskill(dt, at, dwp, false, dist>1),
      skdiff, dist);
#ifdef SMALL_BATTLE_MESSAGES
  if (b->small) {
    smallbuf = attack_message(at, dt, awp, dist);
  }
#endif
  if (contest(skdiff, armor, shield)) {
    strcat(debugbuf, " und trifft.");
    battledebug(debugbuf);
#ifdef SMALL_BATTLE_MESSAGES
    if (b->small) {
      strcat(smallbuf, " und trifft.");
      battlerecord(b,smallbuf);
    }
#endif
    return 1;
  }
  strcat(debugbuf, ".");
  battledebug(debugbuf);
#ifdef SMALL_BATTLE_MESSAGES
  if (b->small) {
    strcat(smallbuf, ".");
    battlerecord(b,smallbuf);
  }
#endif
  return 0;
}

void
dazzle(battle *b, troop *td)
{
#ifdef SMALL_BATTLE_MESSAGES
  char smallbuf[512];
#endif

  /* Nicht kumulativ ! */
  if(td->fighter->person[td->index].flags & FL_DAZZLED) return;

#ifdef TODO_RUNESWORD
  if (td->fighter->weapon[WP_RUNESWORD].count > td->index) {
#ifdef SMALL_BATTLE_MESSAGES
    if (b->small) {
      strcpy(smallbuf, "Das Runenschwert glüht kurz auf.");
      battlerecord(b,smallbuf);
    }
#endif
    return;
  }
#endif
  if (td->fighter->person[td->index].flags & FL_COURAGE) {
#ifdef SMALL_BATTLE_MESSAGES
    if (b->small) {
      sprintf(smallbuf, "Eine kurze Schwäche erfaßt %s/%d, vergeht jedoch "
        "schnell wieder.", unitname(td->fighter->unit), td->index);
      battlerecord(b,smallbuf);
    }
#endif
                return;
  }

  if (td->fighter->person[td->index].flags & FL_DAZZLED) {
#ifdef SMALL_BATTLE_MESSAGES
    if (b->small) {
      sprintf(smallbuf, "Eine kurze Schwäche erfaßt %s/%d, vergeht jedoch "
        "schnell wieder.", unitname(td->fighter->unit), td->index);
      battlerecord(b,smallbuf);
    }
#endif
    return;
  }

#ifdef SMALL_BATTLE_MESSAGES
  if (b->small) {
    sprintf(smallbuf, "%s/%d fühlt sich, als würde Kraft entzogen.",
      unitname(td->fighter->unit), td->index);
    battlerecord(b,smallbuf);
  }
#endif
  td->fighter->person[td->index].flags |= FL_DAZZLED;
  td->fighter->person[td->index].defence--;
}

/* TODO: Gebäude/Schiffe sollten auch zerstörbar sein. Schwierig im Kampf,
 * besonders bei Schiffen. */

void
damage_building(battle *b, building *bldg, int damage_abs)
{
  bldg->size = max(1, bldg->size-damage_abs);

  /* Wenn Burg, dann gucken, ob die Leute alle noch in das Gebäude passen. */

  if (bldg->type->flags & BTF_PROTECTION) {
    side * s;

    bldg->sizeleft = bldg->size;

    for (s=b->sides;s;s=s->next) {
      fighter * fig;
      for (fig=s->fighters;fig;fig=fig->next) {
        if (fig->building == bldg) {
          if (bldg->sizeleft >= fig->unit->number) {
            fig->building = bldg;
            bldg->sizeleft -= fig->unit->number;
          } else {
            fig->building = NULL;
          }
        }
      }
    }
  }
}

static int
attacks_per_round(troop t)
{
  return t.fighter->person[t.index].speed;
}


#ifdef HEROES
#define HERO_SPEED 10
static void
make_heroes(battle * b)
{
  side * s;
  for (s=b->sides;s;s=s->next) {
    fighter * fig;
    for (fig=s->fighters;fig;fig=fig->next) {
      unit * u = fig->unit;
      if (fval(u, UFL_HERO)) {
        int i;
        assert(playerrace(u->race));
        for (i=0;i!=u->number;++i) {
          fig->person[i].speed += (HERO_SPEED-1);
        }
      }
    }
  }
}
#endif

static void
attack(battle *b, troop ta, const att *a, int numattack)
{
  fighter *af = ta.fighter;
  troop td;
  unit *au = af->unit;

  switch(a->type) {
  case AT_COMBATSPELL:
    /* Magier versuchen immer erstmal zu zaubern, erst wenn das
    * fehlschlägt, wird af->magic == 0 und  der Magier kämpft
    * konventionell weiter */
    if (numattack==0 && af->magic > 0) {
      /* wenn der magier in die potenzielle Reichweite von Attacken des 
       * Feindes kommt, beginnt er auch bei einem Status von KAEMPFE NICHT,
       * Kampfzauber zu schleudern: */
      if (count_enemies(b, af, melee_range[0], missile_range[1], SELECT_ADVANCE|SELECT_DISTANCE|SELECT_FIND)) {
        do_combatspell(ta);
      }
    }
    break;
  case AT_STANDARD:   /* Waffen, mag. Gegenstände, Kampfzauber */
    if (numattack > 0 || af->magic <= 0) {
      weapon * wp = ta.fighter->person[ta.index].missile;
      int melee = count_enemies(b, af, melee_range[0], melee_range[1], SELECT_ADVANCE|SELECT_DISTANCE|SELECT_FIND);
      if (melee) wp = preferred_weapon(ta, true);
      /* Sonderbehandlungen */

      if (getreload(ta)) {
        ta.fighter->person[ta.index].reload--;
      } else {
        boolean standard_attack = true;
        boolean reload = false;
        /* spezialattacken der waffe nur, wenn erste attacke in der runde.
         * sonst helden mit feuerschwertern zu mächtig */
        if (numattack==0 && wp && wp->type->attack) {
          int dead = 0;
          standard_attack = wp->type->attack(&ta, wp->type, &dead);
          if (!standard_attack) reload = true;
          af->catmsg += dead;
          if (!standard_attack && af->person[ta.index].last_action < b->turn) {
            af->person[ta.index].last_action = b->turn;
#ifndef SIMPLE_COMBAT
            af->action_counter++;
            af->side->bf->lastturn = b->turn;
#endif
          }
        }
        if (standard_attack) {
          boolean missile = false;
          if (wp && fval(wp->type, WTF_MISSILE)) missile = true;
          if (missile) {
            td = select_opponent(b, ta, missile_range[0], missile_range[1]);
          }
          else {
            td = select_opponent(b, ta, melee_range[0], melee_range[1]);
          }
          if (!td.fighter) return;
          if (ta.fighter->person[ta.index].last_action < b->turn) {
            ta.fighter->person[ta.index].last_action = b->turn;
#ifndef SIMPLE_COMBAT
            ta.fighter->action_counter++;
            ta.fighter->side->bf->lastturn = b->turn;
#endif
          }
          reload = true;
          if (hits(ta, td, wp)) {
            const char * d;
            if (wp == NULL) d = au->race->def_damage;
            else if (riding(ta)) d = wp->type->damage[1];
            else d = wp->type->damage[0];
            terminate(td, ta, a->type, d, missile);
          }
        }
        if (reload && wp && wp->type->reload && !getreload(ta)) {
          int i = setreload(ta);
          sprintf(buf, " Nachladen gesetzt: %d", i);
          battledebug(buf);
        }
      }
    }
    break;
  case AT_SPELL:  /* Extra-Sprüche. Kampfzauber in AT_COMBATSPELL! */
    do_extra_spell(ta, a);
    break;
  case AT_NATURAL:
    td = select_opponent(b, ta, melee_range[0], melee_range[1]);
    if (!td.fighter) return;
    if(ta.fighter->person[ta.index].last_action < b->turn) {
      ta.fighter->person[ta.index].last_action = b->turn;
#ifndef SIMPLE_COMBAT
      ta.fighter->action_counter++;
      ta.fighter->side->bf->lastturn = b->turn;
#endif
    }
    if (hits(ta, td, NULL)) {
      terminate(td, ta, a->type, a->data.dice, false);
    }
    break;
  case AT_DRAIN_ST:
    td = select_opponent(b, ta, melee_range[0], melee_range[1]);
    if (!td.fighter) return;
    if(ta.fighter->person[ta.index].last_action < b->turn) {
      ta.fighter->person[ta.index].last_action = b->turn;
#ifndef SIMPLE_COMBAT
      ta.fighter->action_counter++;
      ta.fighter->side->bf->lastturn = b->turn;
#endif
    }
    if (hits(ta, td, NULL)) {
      int c = dice_rand(a->data.dice);
      while(c > 0) {
        if (rng_int()%2) {
          td.fighter->person[td.index].attack -= 1;
        } else {
          td.fighter->person[td.index].defence -= 1;
        }
        c--;
      }
    }
    break;
  case AT_DRAIN_EXP:
    td = select_opponent(b, ta, melee_range[0], melee_range[1]);
    if (!td.fighter) return;
    if(ta.fighter->person[ta.index].last_action < b->turn) {
      ta.fighter->person[ta.index].last_action = b->turn;
#ifndef SIMPLE_COMBAT
      ta.fighter->action_counter++;
      ta.fighter->side->bf->lastturn = b->turn;
#endif
    }
    if (hits(ta, td, NULL)) {
      drain_exp(td.fighter->unit, dice_rand(a->data.dice));
    }
    break;
  case AT_DAZZLE:
    td = select_opponent(b, ta, melee_range[0], melee_range[1]);
    if (!td.fighter) return;
    if(ta.fighter->person[ta.index].last_action < b->turn) {
      ta.fighter->person[ta.index].last_action = b->turn;
#ifndef SIMPLE_COMBAT
      ta.fighter->action_counter++;
      ta.fighter->side->bf->lastturn = b->turn;
#endif
    }
    if (hits(ta, td, NULL)) {
      dazzle(b, &td);
    }
    break;
  case AT_STRUCTURAL:
    td = select_opponent(b, ta, melee_range[0], melee_range[1]);
    if (!td.fighter) return;
    if(ta.fighter->person[ta.index].last_action < b->turn) {
      ta.fighter->person[ta.index].last_action = b->turn;
#ifndef SIMPLE_COMBAT
      ta.fighter->action_counter++;
      ta.fighter->side->bf->lastturn = b->turn;
#endif
    }
    if (td.fighter->unit->ship) {
      td.fighter->unit->ship->damage += DAMAGE_SCALE * dice_rand(a->data.dice);
    } else if (td.fighter->unit->building) {
      damage_building(b, td.fighter->unit->building, dice_rand(a->data.dice));
    }
  }
}

void
do_attack(fighter * af)
{
  troop ta;
  unit *au = af->unit;
  side *side = af->side;
  battle *b = side->battle;

  ta.fighter = af;

  assert(au && au->number);
  /* Da das Zuschlagen auf Einheiten und nicht auf den einzelnen
  * Kämpfern beruht, darf die Reihenfolge und Größe der Einheit keine
  * Rolle spielen, Das tut sie nur dann, wenn jeder, der am Anfang der
  * Runde lebte, auch zuschlagen darf. Ansonsten ist der, der zufällig
  * mit einer großen Einheit zuerst drankommt, extrem bevorteilt. */
  ta.index = af->fighting;

  while (ta.index--) {
    /* Wir suchen eine beliebige Feind-Einheit aus. An der können
    * wir feststellen, ob noch jemand da ist. */
    int apr, attacks = attacks_per_round(ta);
    if (!count_enemies(b, af, FIGHT_ROW, LAST_ROW, SELECT_FIND)) break;

    for (apr=0;apr!=attacks;++apr) {
      int a;
      for (a=0; a!=10 && au->race->attack[a].type!=AT_NONE; ++a) {
        if (apr>0) {
          /* Wenn die Waffe nachladen muss, oder es sich nicht um einen
          * Waffen-Angriff handelt, dann gilt der Speed nicht. */
          if (au->race->attack[a].type!=AT_STANDARD) continue;
          else {
            weapon * wp = preferred_weapon(ta, true);
            if (wp!=NULL && wp->type->reload) continue;
          }
        }
        attack(b, ta, &(au->race->attack[a]), apr);
      }
    }
  }
  /* Der letzte Katapultschütze setzt die
  * Ladezeit neu und generiert die Meldung. */
  if (af->catmsg>=0) {
    struct message * m = msg_message("battle::killed", "unit dead", au, af->catmsg);
    message_all(b, m);
    msg_release(m);
    af->catmsg = -1;
  }
}

void
do_regenerate(fighter *af)
{
  troop ta;
  unit *au = af->unit;

  ta.fighter = af;
  ta.index = af->fighting;

  while(ta.index--) {
    af->person[ta.index].hp += effskill(au, SK_AUSDAUER);
    af->person[ta.index].hp = min(unit_max_hp(au), af->person[ta.index].hp);
  }
}

static void
add_tactics(tactics * ta, fighter * fig, int value)
{
  if (value == 0 || value < ta->value)
    return;
  if (value > ta->value)
    cv_kill(&ta->fighters);
  cv_pushback(&ta->fighters, fig);
  cv_pushback(&fig->side->battle->leaders, fig);
  ta->value = value;
}

double
fleechance(unit * u)
{
  double c = 0.20;      /* Fluchtwahrscheinlichkeit in % */
  region * r = u->region;
  attrib * a = a_find(u->attribs, &at_fleechance);
  /* Einheit u versucht, dem Getümmel zu entkommen */

  c += (eff_skill(u, SK_STEALTH, r) * 0.05);

  if (get_item(u, I_UNICORN) >= u->number && eff_skill(u, SK_RIDING, r) >= 5)
    c += 0.30;
  else if (get_item(u, I_HORSE) >= u->number && eff_skill(u, SK_RIDING, r) >= 1)
    c += 0.10;

  if (u->race == new_race[RC_HALFLING]) {
    c += 0.20;
    c = min(c, 0.90);
  } else {
    c = min(c, 0.75);
  }

  if (a!=NULL) c += a->data.flt;

  return c;
}

static int nextside = 0;

/** add a new army to the conflict
 * beware: armies need to be added _at the beginning_ of the list because 
 * otherwise join_allies() will get into trouble */
side *
make_side(battle * b, const faction * f, const group * g, unsigned int flags, const faction *stealthfaction)
{
  side *s1 = calloc(sizeof(struct side), 1);
  bfaction * bf;

  s1->battle = b;
  s1->group = g;
  s1->flags = flags;
  s1->stealthfaction = stealthfaction;
  s1->next = b->sides;
  b->sides = s1;
  for (bf = b->factions;bf;bf=bf->next) {
    faction * f2 = bf->faction;

    if (f2 == f) {
      s1->bf = bf;
      s1->index = nextside++;
      s1->nextF = bf->sides;
      bf->sides = s1;
      ++b->nsides;
      break;
    }
  }
  assert(bf);
  return s1;
}

void
loot_items(fighter * corpse)
{
  unit * u = corpse->unit;
  item * itm = u->items;
  battle * b = corpse->side->battle;
  u->items = NULL;

  while (itm) {
    int i;
    if (itm->number) {
      for (i = 10; i != 0; --i) {
        int loot = itm->number / i;
        /* Looten tun hier immer nur die Gegner. Das ist als Ausgleich für die
        * neue Loot-regel (nur ganz tote Einheiten) fair.
        * zusätzlich looten auch geflohene, aber nach anderen Regeln.
        */
        if (loot>0) {
          int maxrow = BEHIND_ROW;
          int lootchance = 50 + b->keeploot;

          if (itm->type->flags & (ITF_CURSED|ITF_NOTLOST)) maxrow = LAST_ROW;
          itm->number -= loot;

          if (maxrow == LAST_ROW || rng_int() % 100 < lootchance) {
            fighter *fig = select_enemy(corpse, FIGHT_ROW, maxrow, 0).fighter;
            if (fig) {
              item * l = fig->loot;
              while (l && l->type!=itm->type) l=l->next;
              if (!l) {
                l = calloc(sizeof(item), 1);
                l->next = fig->loot;
                fig->loot = l;
                l->type = itm->type;
              }
              l->number += loot;
            }
          }
        }
      }
    }
    itm = itm->next;
  }
}

static void
loot_fleeing(fighter* fig, unit* runner)
{
  /* TODO: Vernünftig fixen */
  runner->items = NULL;
  assert(runner->items == NULL);
  runner->items = fig->run.items;
  fig->run.items = NULL;
}

static void
merge_fleeloot(fighter* fig, unit* u)
{
  i_merge(&u->items, &fig->run.items);
}

static boolean
seematrix(const faction * f, const side * s)
{
  if (f==s->bf->faction) return true;
  if (s->flags & SIDE_STEALTH) return false;
  return true;
}

static double
PopulationDamage(void)
{
  static double value = -1.0;
  if (value<0) {
    const char * str = get_param(global.parameters, "battle.populationdamage");
    value = str?atof(str):(BATTLE_KILLS_PEASANTS/100.0);
  }
  return value;
}


static void
battle_effects(battle * b, int dead_players)
{
  region * r = b->region;
  int dead_peasants = min(rpeasants(r), (int)(dead_players*PopulationDamage()));
  deathcounts(r, dead_peasants + dead_players);
  chaoscounts(r, dead_peasants / 2);
  rsetpeasants(r, rpeasants(r) - dead_peasants);
}

static void
aftermath(battle * b)
{
  int i;
  region *r = b->region;
  ship *sh;
  side *s;
  int dead_players = 0;
  bfaction * bf;
  boolean battle_was_relevant = (boolean)(b->turn+(b->has_tactics_turn?1:0)>2);

#ifdef TROLLSAVE
  int *trollsave = calloc(2 * cv_size(&b->factions), sizeof(int));
#endif

  for (s=b->sides; s; s=s->next) {
    fighter * df;
    for (df = s->fighters; df; df=df->next) {
      unit *du = df->unit;
      int dead = du->number - df->alive - df->run.number;
      int pr_mercy = 0;
#ifdef KARMA_MODULE
      const attrib *a= a_find(du->attribs, &at_prayer_effect);

      while (a && a->type==&at_prayer_effect) {
        if (a->data.sa[0] == PR_MERCY) {
          pr_mercy = a->data.sa[1];
        }
        a = a->next;
      }
#endif /* KARMA_MODULE */

#ifdef TROLLSAVE
      /* Trolle können regenerieren */
      if (df->alive > 0 && dead>0 && du->race == new_race[RC_TROLL]) {
        for (i = 0; i != dead; ++i) {
          if (chance(TROLL_REGENERATION)) {
            ++df->alive;
            ++s->alive;
            ++s->battle->alive;
            ++trollsave[s->index];
            /* do not change dead here, or loop will not terminate! recalculate later */
          }
        }
        dead = du->number - df->alive - df->run.number;
      }
#endif
      /* Regeneration durch PR_MERCY */
      if (dead>0 && pr_mercy) {
        for (i = 0; i != dead; ++i) {
          if (rng_int()%100 < pr_mercy) {
            ++df->alive;
            ++s->alive;
            ++s->battle->alive;
            /* do not change dead here, or loop will not terminate! recalculate later */
          }
        }
        dead = du->number - df->alive - df->run.number;
      }

      /* tote insgesamt: */
      s->dead += dead;
      /* Tote, die wiederbelebt werde können: */
      if (playerrace(df->unit->race)) {
        s->casualties += dead;
      }
#ifdef SHOW_KILLS
      if (df->hits + df->kills) {
        struct message * m = msg_message("killsandhits", "unit hits kills", du, df->hits, df->kills);
        message_faction(b, du->faction, m);
        msg_release(m);
      }
#endif
    }
  }

  /* POSTCOMBAT */
  do_combatmagic(b, DO_POSTCOMBATSPELL);

  for (s=b->sides;s;s=s->next) {
    int snumber = 0;
    fighter *df;
#ifndef SIMPLE_COMBAT
    boolean relevant = false; /* Kampf relevant für diese Partei? */

    if (s->bf->lastturn>1) {
      relevant = true;
    } else if (s->bf->lastturn==1 && b->has_tactics_turn) {
      side * stac;
      for (stac=b->sides; stac; stac=stac->next) {
        if (stac->leader.value == b->max_tactics && helping(stac, s)) {
          relevant = true;
          break;
        }
      }
    }
#endif
    s->flee = 0;

    for (df=s->fighters;df;df=df->next) {
      unit *du = df->unit;
      int dead = du->number - df->alive - df->run.number;
      int sum_hp = 0;
      int n;

      snumber += du->number;
#ifdef SIMPLE_COMBAT
      if (battle_was_relevant) {
        ship * sh = du->ship?du->ship:leftship(du);
        if (sh) fset(sh, SF_DAMAGED);
      }
#else
      if (relevant) {
        fset(du, UFL_NOTMOVING); /* unit cannot move this round */
        if (df->action_counter >= du->number) {
          ship * sh = du->ship?du->ship:leftship(du);
          if (sh) fset(sh, SF_DAMAGED);
          fset(du, UFL_LONGACTION);
        }
      }
#endif
      for (n = 0; n != df->alive; ++n) {
        if (df->person[n].hp > 0) {
          sum_hp += df->person[n].hp;
        }
      }

      if (df->alive == du->number) {
        du->hp = sum_hp;
        continue; /* nichts passiert */
      } else if (df->run.hp) {
        if (df->alive == 0) {
          /* Report the casualties */
          reportcasualties(b, df, dead);

          /* Zuerst dürfen die Feinde plündern, die mitgenommenen Items
          * stehen in fig->run.items. Dann werden die Fliehenden auf
          * die leere (tote) alte Einheit gemapt */
          if (fval(df, FIG_NOLOOT)){
            merge_fleeloot(df, du);
          } else {
            loot_items(df);
            loot_fleeing(df, du);
          }
          scale_number(du, df->run.number);
          du->hp = df->run.hp;
          set_order(&du->thisorder, NULL);
          setguard(du, GUARD_NONE);
          fset(du, UFL_LONGACTION);
          leave(du->region, du);
          if (df->run.region) {
            run_to(du, df->run.region);
            df->run.region = du->region;
          }
        } else {
          /* nur teilweise geflohene Einheiten mergen sich wieder */
          df->alive += df->run.number;
          s->size[0] += df->run.number;
          s->size[statusrow(df->status)] += df->run.number;
          s->alive += df->run.number;
          sum_hp += df->run.hp;
          merge_fleeloot(df, du);
          df->run.number = 0;
          df->run.hp = 0;
          /* df->run.region = NULL;*/

          reportcasualties(b, df, dead);

          scale_number(du, df->alive);
          du->hp = sum_hp;
        }
      } else {
        if (df->alive==0) {
          /* alle sind tot, niemand geflohen. Einheit auflösen */
          df->run.number = 0;
          df->run.hp = 0;
          df->run.region = NULL;

          /* Report the casualties */
          reportcasualties(b, df, dead);

          setguard(du, GUARD_NONE);
          scale_number(du, 0);
          /* Distribute Loot */
          loot_items(df);
        } else {
          df->run.number = 0;
          df->run.hp = 0;

          reportcasualties(b, df, dead);

          scale_number(du, df->alive);
          du->hp = sum_hp;
        }
      }
      s->flee += df->run.number;

      if (playerrace(du->race)) {
        /* tote im kampf werden zu regionsuntoten:
        * for each of them, a peasant will die as well */
        dead_players += dead;
      }
      if (du->hp < du->number) {
        log_error(("%s has less hitpoints (%u) than people (%u)\n",
                   itoa36(du->no), du->hp, du->number));
        du->hp = du->number;
      }
    }
    s->alive+=s->healed;
    assert(snumber==s->flee+s->alive+s->dead);
  }

  battle_effects(b, dead_players);

  for (s=b->sides;s;s=s->next) {
    message * seen = msg_message("battle::army_report",
      "index abbrev dead fled survived",
      army_index(s), sideabkz(s, false), s->dead, s->flee, s->alive);
    message * unseen = msg_message("battle::army_report",
      "index abbrev dead fled survived",
      army_index(s), "-?-", s->dead, s->flee, s->alive);

    for (bf=b->factions;bf;bf=bf->next) {
      faction * f = bf->faction;
      message * m = seematrix(f, s)?seen:unseen;

      message_faction(b, f, m);
    }

    msg_release(seen);
    msg_release(unseen);
  }
  /* Wir benutzen drifted, um uns zu merken, ob ein Schiff
  * schonmal Schaden genommen hat. (moved und drifted
  * sollten in flags überführt werden */

  for (s=b->sides; s; s=s->next) {
    fighter *df;
    for (df=s->fighters; df; df=df->next) {
      unit *du = df->unit;
      item * l;

      for (l=df->loot; l; l=l->next) {
        const item_type * itype = l->type;
        sprintf(buf, "%s erbeute%s %d %s.", unitname(du), du->number==1?"t":"n",
          l->number, locale_string(default_locale, resourcename(itype->rtype, l->number!=1)));
        fbattlerecord(b, du->faction, buf);
        i_change(&du->items, itype, l->number);
      }

      /* Wenn sich die Einheit auf einem Schiff befindet, wird
      * dieses Schiff beschädigt. Andernfalls ein Schiff, welches
      * evt. zuvor verlassen wurde. */

      if (du->ship) sh = du->ship; else sh = leftship(du);

      if (sh && fval(sh, SF_DAMAGED) && b->turn+(b->has_tactics_turn?1:0)>2) {
        damage_ship(sh, 0.20);
        freset(sh, SF_DAMAGED);
      }
    }
  }

  if (battle_was_relevant) {
    ship **sp = &r->ships;
    while (*sp) {
      ship * sh = *sp;
      freset(sh, SF_DAMAGED);
      if (sh->damage >= sh->size * DAMAGE_SCALE) {
        destroy_ship(sh);
      }
      if (*sp==sh) sp=&sh->next;
    }
  }
#ifdef TROLLSAVE
  free(trollsave);
#endif

  sprintf(buf, "The battle lasted %d turns, %s and %s.\n",
    b->turn,
    b->has_tactics_turn==true?"had a tactic turn":"had no tactic turn",
    battle_was_relevant==true?"was relevant":"was not relevant.");
  battledebug(buf);
}

static void
battle_punit(unit * u, battle * b)
{
  bfaction * bf;
  strlist *S, *x;

  for (bf = b->factions;bf;bf=bf->next) {
    faction *f = bf->faction;

    S = 0;
    spunit(&S, f, u, 4, see_battle);
    for (x = S; x; x = x->next) {
      fbattlerecord(b, f, x->s);
      if (u->faction == f)
        battledebug(x->s);
    }
    if (S)
      freestrlist(S);
  }
}

static void
print_fighters(battle * b, const side * s)
{
  fighter *df;
  int row;

  for (row=1;row!=NUMROWS;++row) {
    message * m = NULL;

    for (df=s->fighters; df; df=df->next) {
      unit *du = df->unit;
      int thisrow = statusrow(df->unit->status);

      if (row == thisrow) {
        if (m==NULL) {
          m = msg_message("battle::row_header", "row", row);
          message_all(b, m);
        }
        battle_punit(du, b);
      }
    }
    if (m!=NULL) msg_release(m);
  }
}

boolean
is_attacker(const fighter * fig)
{
  return fval(fig, FIG_ATTACKER)!=0;
}

static void
set_attacker(fighter * fig)
{
  fset(fig, FIG_ATTACKER);
}

static void
print_header(battle * b)
{
  bfaction * bf;

  for (bf=b->factions;bf;bf=bf->next) {
    faction * f = bf->faction;
    const char * lastf = NULL;
    boolean first = false;
    side * s;

    strcpy(buf, "Der Kampf wurde ausgelöst von ");
    for (s=b->sides; s; s=s->next) {
      fighter *df;
      for (df=s->fighters;df;df=df->next) {
        if (is_attacker(df)) {
          if (first) strcat(buf, ", ");
          if (lastf) {
            strcat(buf, lastf);
            first = true;
          }
          if (seematrix(f, s) == true)
            lastf = sidename(s, false);
          else
            lastf = "einer unbekannten Partei";
          break;
        }
      }
    }
    if (first) strcat(buf, " und ");
    if (lastf) strcat(buf, lastf);
    strcat(buf, ".");
    fbattlerecord(b, f, buf);
  }
}

static void
print_stats(battle * b)
{
  side *s2;
  side *s;
  int i = 0;
  for (s=b->sides;s;s=s->next) {
    bfaction *bf;
    char *k;

    ++i;

    for (bf=b->factions;bf;bf=bf->next) {
      faction * f = bf->faction;
      const char * loc_army = LOC(f->locale, "battle_army");
      char * bufp;
      const char * header;
      size_t rsize, size;
      int komma;

      fbattlerecord(b, f, " ");

      slprintf(buf, sizeof(buf), "%s %d: %s", loc_army, army_index(s),
        seematrix(f, s) ? sidename(s, false) : LOC(f->locale, "unknown_faction"));
      fbattlerecord(b, f, buf);

      bufp = buf;
      size = sizeof(buf);
      komma = 0;
      header = LOC(f->locale, "battle_opponents");

      for (s2=b->sides;s2;s2=s2->next) {
        if (enemy(s2, s)) {
          const char * abbrev = seematrix(f, s2)?sideabkz(s2, false):"-?-";
          rsize = slprintf(bufp, size, "%s %s %d(%s)",
            komma++ ? "," : header, loc_army, army_index(s2), abbrev);
          if (rsize>size) rsize = size-1;
          size -= rsize;
          bufp += rsize;
        }
      }
      if (komma) fbattlerecord(b, f, buf);

      bufp = buf;
      size = sizeof(buf);
      komma = 0;
      header = LOC(f->locale, "battle_helpers");

      for (s2=b->sides;s2;s2=s2->next) {
        if (friendly(s2, s)) {
          const char * abbrev = seematrix(f, s2)?sideabkz(s2, false):"-?-";
          rsize = slprintf(bufp, size, "%s %s %d(%s)",
            komma++ ? "," : header, loc_army, army_index(s2), abbrev);
          if (rsize>size) rsize = size-1;
          size -= rsize;
          bufp += rsize;
        }
      }
      if (komma) fbattlerecord(b, f, buf);

      bufp = buf;
      size = sizeof(buf);
      komma = 0;
      header = LOC(f->locale, "battle_attack");

      for (s2=b->sides;s2;s2=s2->next) {
        if (s->relations[s2->index] & E_ATTACKING) {
          const char * abbrev = seematrix(f, s2)?sideabkz(s2, false):"-?-";
          rsize = slprintf(bufp, size, "%s %s %d(%s)", komma++ ? "," : header, loc_army,
            army_index(s2), abbrev);
          if (rsize>size) rsize = size-1;
          size -= rsize;
          bufp += rsize;
        }
      }
      if (komma) fbattlerecord(b, f, buf);
    }
    buf[77] = (char)0;
    for (k = buf; *k; ++k) *k = '-';
    battlerecord(b, buf);
    if (s->bf->faction) {
      if (s->bf->faction->alliance) {
        slprintf(buf, sizeof(buf), "##### %s (%s/%d)", s->bf->faction->name, itoa36(s->bf->faction->no),
          s->bf->faction->alliance?s->bf->faction->alliance->id:0);
      } else {
        slprintf(buf, sizeof(buf), "##### %s (%s)", s->bf->faction->name, itoa36(s->bf->faction->no));
      }
      battledebug(buf);
    }
    print_fighters(b, s);
  }

  battlerecord(b, " ");

  /* Besten Taktiker ermitteln */

  b->max_tactics = 0;

  for (s = b->sides; s; s = s->next) {
    if (cv_size(&s->leader.fighters)) {
      b->max_tactics = max(b->max_tactics, s->leader.value);
    }
  }

  if (b->max_tactics > 0) {
    for (s = b->sides; s; s = s->next) {
      if (s->leader.value == b->max_tactics) {
        fighter *tf;
        cv_foreach(tf, s->leader.fighters) {
          unit *u = tf->unit;
          message * m = NULL;
          if (!is_attacker(tf)) {
            m = msg_message("battle::tactics_lost", "unit", u);
          } else {
            m = msg_message("battle::tactics_won", "unit", u);
          }
          message_all(b, m);
          msg_release(m);
        } cv_next(tf);
      }
    }
  }
}

static int
weapon_weight(const weapon * w, boolean missile)
{
  if (missile == i2b(fval(w->type, WTF_MISSILE))) {
    return w->attackskill + w->defenseskill;
  }
  return 0;
}

fighter *
make_fighter(battle * b, unit * u, side * s1, boolean attack)
{
#define WMAX 20
  weapon weapons[WMAX];
  int owp[WMAX];
  int dwp[WMAX];
  int w = 0;
  region *r = b->region;
  item * itm;
  fighter *fig = NULL;
  int i, t = eff_skill(u, SK_TACTICS, r);
  side *s2;
  int h;
  int berserk;
  int strongmen;
  int speeded = 0, speed = 1;
  boolean pr_aid = false;
  int rest;
  const group * g = NULL;
  const attrib *a = a_find(u->attribs, &at_otherfaction);
  const faction *stealthfaction = a?get_otherfaction(a):NULL;
  static const struct item_type * it_demonseye;
  static boolean init = false;
  unsigned int flags = 0;

  assert(u->number);
  if (fval(u, UFL_PARTEITARNUNG)!=0) flags |= SIDE_STEALTH;
#ifdef SIMPLE_COMBAT
  if (attack) flags |= SIDE_HASGUARDS;
#endif
  if (!init) {
    it_demonseye = it_find("demonseye");
    init=true;
  }

  if (fval(u, UFL_GROUP)) {
    const attrib * agroup = a_find(u->attribs, &at_group);
    if (agroup!=NULL) g = (const group*)agroup->data.v;
  }

  /* Illusionen und Zauber kaempfen nicht */
  if (fval(u->race, RCF_ILLUSIONARY) || idle(u->faction) || u->number==0)
    return NULL;

  if (s1==NULL) {
    for (s2 = b->sides; s2; s2 = s2->next) {
      if (s2->bf->faction == u->faction && s2->group==g) {
        if (s2->flags==flags && s2->stealthfaction==stealthfaction) {
          s1 = s2;
          break;
        }
      }
    }

    /* aliances are moved out of make_fighter and will be handled later */
    if (!s1) s1 = make_side(b, u->faction, g, flags, stealthfaction);
    /* Zu diesem Zeitpunkt ist attacked noch 0, da die Einheit für noch
    * keinen Kampf ausgewählt wurde (sonst würde ein fighter existieren) */
  }
  fig = calloc(1, sizeof(struct fighter));

  fig->next = s1->fighters;
  s1->fighters = fig;

  fig->unit = u;
  /* In einer Burg muß man a) nicht Angreifer sein, und b) drin sein, und
   * c) noch Platz finden. d) menschanähnlich sein */
  if (attack) {
    set_attacker(fig);
  } else {
    building * b = u->building;
    if (b && b->sizeleft>=u->number && playerrace(u->race)) {
      fig->building = b;
      fig->building->sizeleft -= u->number;
    }
  }
  fig->status = u->status;
  fig->side = s1;
  fig->alive = u->number;
  fig->side->alive += u->number;
  fig->side->battle->alive += u->number;
  fig->catmsg = -1;

  /* Freigeben nicht vergessen! */
  fig->person = calloc(fig->alive, sizeof(struct person));

  h = u->hp / u->number;
  assert(h);
  rest = u->hp % u->number;

  /* Effekte von Sprüchen */

  {
    static const curse_type * speed_ct;
    speed_ct = ct_find("speed");
    if (speed_ct) {
      curse *c = get_curse(u->attribs, speed_ct);
      if (c) {
        speeded = get_cursedmen(u, c);
        speed   = curse_geteffect(c);
      }
    }
  }

  /* Effekte von Alchemie */
  berserk = get_effect(u, oldpotiontype[P_BERSERK]);
  /* change_effect wird in ageing gemacht */

  /* Effekte von Artefakten */
  strongmen = min(fig->unit->number, get_item(u, I_TROLLBELT));

#ifdef KARMA_MODULE
  for (a = a_find(u->attribs, &at_prayer_effect); a && a->type==&at_prayer_effect; a = a->next) {
    if (a->data.sa[0] == PR_AID) {
      pr_aid = true;
      break;
    }
  }
#endif /* KARMA_MODULE */

  /* Hitpoints, Attack- und Defence-Boni für alle Personen */
  for (i = 0; i < fig->alive; i++) {
    assert(i < fig->unit->number);
    fig->person[i].hp = h;
    if (i < rest)
      fig->person[i].hp++;

    if (i < speeded)
      fig->person[i].speed = speed;
    else
      fig->person[i].speed = 1;

    if (i < berserk) {
      fig->person[i].attack++;
    }
    /* Leute mit einem Aid-Prayer bekommen +1 auf fast alles. */
    if (pr_aid) {
      fig->person[i].attack++;
      fig->person[i].defence++;
      fig->person[i].damage++;
      fig->person[i].damage_rear++;
        fig->person[i].flags |= FL_COURAGE;
    }
    /* Leute mit Kraftzauber machen +2 Schaden im Nahkampf. */
    if (i < strongmen) {
      fig->person[i].damage += 2;
    }
  }

  /* Für alle Waffengattungne wird bestimmt, wie viele der Personen mit
   * ihr kämpfen könnten, und was ihr Wert darin ist. */
  if (u->race->battle_flags & BF_EQUIPMENT) {
    int oi=0, di=0;
    for (itm=u->items;itm && w!=WMAX;itm=itm->next) {
      const weapon_type * wtype = resource2weapon(itm->type->rtype);
      if (wtype==NULL || itm->number==0) continue;
      weapons[w].attackskill = weapon_skill(wtype, u, true);
      weapons[w].defenseskill = weapon_skill(wtype, u, false);
      weapons[w].type = wtype;
      weapons[w].used = 0;
      weapons[w].count = itm->number;
      ++w;
      assert(w!=WMAX);
    }
    fig->weapons = calloc(sizeof(weapon), w+1);
    memcpy(fig->weapons, weapons, w*sizeof(weapon));

    for (i=0; i!=w; ++i) {
      int j, o=0, d=0;
      for (j=0; j!=i; ++j) {
        if (weapon_weight(fig->weapons+j, true)>=weapon_weight(fig->weapons+i, true)) ++d;
        if (weapon_weight(fig->weapons+j, false)>=weapon_weight(fig->weapons+i, false)) ++o;
      }
      for (j=i+1; j!=w; ++j) {
        if (weapon_weight(fig->weapons+j, true)>weapon_weight(fig->weapons+i, true)) ++d;
        if (weapon_weight(fig->weapons+j, false)>weapon_weight(fig->weapons+i, false)) ++o;
      }
      owp[o] = i;
      dwp[d] = i;
    }
    /* jetzt enthalten owp und dwp eine absteigend schlechter werdende Liste der Waffen
     * oi and di are the current index to the sorted owp/dwp arrays
     * owp, dwp contain indices to the figther::weapons array */

    /* hand out melee weapons: */
    for (i=0; i!=fig->alive; ++i) {
      int wpless = weapon_skill(NULL, u, true);
      while (oi!=w && (fig->weapons[owp[oi]].used==fig->weapons[owp[oi]].count || fval(fig->weapons[owp[oi]].type, WTF_MISSILE))) {
        ++oi;
      }
      if (oi==w) break; /* no more weapons available */
      if (weapon_weight(fig->weapons+owp[oi], false)<=wpless) {
        continue; /* we fight better with bare hands */
      }
      fig->person[i].melee = &fig->weapons[owp[oi]];
      ++fig->weapons[owp[oi]].used;
    }
    /* hand out missile weapons (from back to front, in case of mixed troops). */
    for (di=0, i=fig->alive; i--!=0;) {
      while (di!=w && (fig->weapons[dwp[di]].used==fig->weapons[dwp[di]].count || !fval(fig->weapons[dwp[di]].type, WTF_MISSILE))) {
        ++di;
      }
      if (di==w) break; /* no more weapons available */
      if (weapon_weight(fig->weapons+dwp[di], true)>0) {
        fig->person[i].missile = &fig->weapons[dwp[di]];
        ++fig->weapons[dwp[di]].used;
      }
    }
  }

  if (it_demonseye && i_get(u->items, it_demonseye)) {
    char lbuf[80];
    const char * s = LOC(default_locale, rc_name(u->race, 3));
    char * c = lbuf;
    while (*s) *c++ = (char)toupper(*s++);
    *c = 0;
    fig->person[0].hp = unit_max_hp(u) * 3;
    slprintf(buf, sizeof(buf), "Eine Stimme ertönt über dem Schlachtfeld. 'DIESES %sKIND IST MEIN. IHR SOLLT ES NICHT HABEN.'. Eine leuchtende Aura umgibt %s", lbuf, unitname(u));
    battlerecord(b, buf);
  }

  s1->size[statusrow(fig->status)] += u->number;
  s1->size[SUM_ROW] += u->number;
  if (u->race->battle_flags & BF_NOBLOCK) {
    s1->nonblockers[statusrow(fig->status)] += u->number;
  }

  if (fig->unit->race->flags & RCF_HORSE) {
    fig->horses = fig->unit->number;
    fig->elvenhorses = 0;
  } else {
    fig->horses = get_item(u, I_HORSE);
    fig->elvenhorses = get_item(u, I_UNICORN);
  }

  if (u->race->battle_flags & BF_EQUIPMENT) {
    for (itm=u->items; itm; itm=itm->next) {
      if (itm->type->rtype->atype) {
        struct armor * adata = malloc(sizeof(armor)), **aptr;
        adata->atype = itm->type->rtype->atype;
        adata->count = itm->number;
        for (aptr=&fig->armors;*aptr;aptr=&(*aptr)->next) {
          if (adata->atype->prot > (*aptr)->atype->prot) break;
        }
        adata->next = *aptr;
        *aptr = adata;
      }
    }
  }


  /* Jetzt muß noch geschaut werden, wo die Einheit die jeweils besten
   * Werte hat, das kommt aber erst irgendwo später. Ich entscheide
   * wärend des Kampfes, welche ich nehme, je nach Gegner. Deswegen auch
   * keine addierten boni. */

  /* Zuerst mal die Spezialbehandlung gewisser Sonderfälle. */
  fig->magic = eff_skill(u, SK_MAGIC, r);

  if (fig->horses) {
    if (!fval(r->terrain, CAVALRY_REGION) || r_isforest(r)
      || eff_skill(u, SK_RIDING, r) < 2 || u->race == new_race[RC_TROLL] || fval(u, UFL_WERE))
      fig->horses = 0;
  }

  if (fig->elvenhorses) {
    if (eff_skill(u, SK_RIDING, r) < 5 || u->race == new_race[RC_TROLL] || fval(u, UFL_WERE))
      fig->elvenhorses = 0;
  }

  /* Schauen, wie gut wir in Taktik sind. */
  if (t > 0 && u->race == new_race[RC_INSECT])
    t -= 1 - (int) log10(fig->side->size[SUM_ROW]);
  if (t > 0 && statusrow(fig->status) == FIGHT_ROW)
    t += 1;
#ifdef TACTICS_MALUS
  if (t > 0 && statusrow(fig->status) > BEHIND_ROW) {
    t -= TACTICS_MALUS;
  }
#endif
#ifdef TACTICS_RANDOM
  if (t > 0) {
    int bonus = 0;

    for (i = 0; i < fig->alive; i++) {
      int p_bonus = 0;
      int rnd;

      do {
        rnd = rng_int()%100;
        if (rnd >= 40 && rnd <= 69)
          p_bonus += 1;
        else if (rnd <= 89)
          p_bonus += 2;
        else
          p_bonus += 3;
      } while(rnd >= 97);
      bonus = max(p_bonus, bonus);
    }
    t += bonus;
  }
#endif
  add_tactics(&fig->side->leader, fig, t);
  ++b->nfighters;
  return fig;
}


static fighter *
join_battle(battle * b, unit * u, boolean attack)
{
  side * s;
  fighter *c = NULL;

  if (!attack) {
    attrib * a = a_find(u->attribs, &at_fleechance);
    if (a!=NULL) {
      if (rng_double()<=a->data.flt) {
        return NULL;
      }
    }
  }

  for (s=b->sides;s;s=s->next) {
    fighter *fig;
    for (fig=s->fighters;fig;fig=fig->next) {
      if (fig->unit == u) {
        c = fig;
        if (attack) {
          set_attacker(fig);
        }
        break;
      }
    }
  }
  if (!c) c = make_fighter(b, u, NULL, attack);
  return c;
}

static const char *
simplename(region * r)
{
  int i;
  static char name[17];
  const char * cp = rname(r, NULL);
  for (i=0;*cp && i!=16;++i, ++cp) {
    int c = *(unsigned char*)cp;
    while (c && !isalpha(c) && !isspace(c)) {
      ++cp;
      c = *(unsigned char*)cp;
    }
    if (isspace(c)) name[i] = '_';
    else name[i] = *cp;
    if (c==0) break;
  }
  name[i]=0;
  return name;
}

static battle *
make_battle(region * r)
{
  battle *b = calloc(1, sizeof(struct battle));
  unit *u;
  faction *lastf = NULL;
  bfaction * bf;
  static int max_fac_no = 0; /* need this only once */

  if (!nobattledebug) {
    char zText[MAX_PATH];
    char zFilename[MAX_PATH];
    sprintf(zText, "%s/battles", basepath());
    makedir(zText, 0700);
    sprintf(zFilename, "%s/battle-%d-%s.log", zText, obs_count, simplename(r));
    bdebug = fopen(zFilename, "w");
    if (!bdebug) log_error(("battles können nicht debugged werden\n"));
    else {
      dbgprintf((bdebug, "In %s findet ein Kampf statt:\n", rname(r, NULL)));
    }
    obs_count++;
  }
  nextside = 0;

  b->region = r;
  b->plane = getplane(r);
  /* Finde alle Parteien, die den Kampf beobachten können: */
  list_foreach(unit, r->units, u)
    if (u->number > 0) {
    if (lastf != u->faction) {  /* Speed optimiert, aufeinanderfolgende
                 * * gleiche nicht getestet */
      lastf = u->faction;
      for (bf=b->factions;bf;bf=bf->next)
        if (bf->faction==lastf) break;
      if (!bf) {
        bf = calloc(sizeof(bfaction), 1);
        ++b->nfactions;
        bf->faction = lastf;
        bf->next = b->factions;
        b->factions = bf;
      }
    }
  }
  list_next(u);

  for (bf=b->factions;bf;bf=bf->next) {
    faction * f = bf->faction;
    max_fac_no = max(max_fac_no, f->no);
  }
  return b;
}

static void
free_side(side * si)
{
  cv_kill(&si->leader.fighters);
}

static void
free_fighter(fighter * fig)
{
  while (fig->loot) {
    i_free(i_remove(&fig->loot, fig->loot));
  }
  while (fig->armors) {
    armor * a = fig->armors;
    fig->armors = a->next;
    free(a);
  }
  free(fig->person);
  free(fig->weapons);

}

static void
free_battle(battle * b)
{
  side *snext;
  meffect *meffect;
  int max_fac_no = 0;

  if (bdebug) {
    fclose(bdebug);
  }

  while (b->factions) {
    bfaction * bf = b->factions;
    faction * f = bf->faction;
    b->factions = bf->next;
    max_fac_no = max(max_fac_no, f->no);
    free(bf);
  }

  snext = b->sides;
  while (snext) {
    side *s = snext;
    fighter *fnext = s->fighters;
    while (fnext) {
      fighter *fig = fnext;
      fnext = fig->next;
      free_fighter(fig);
      free(fig);
    }
    snext = s->next;
    free_side(s);
    free(s);
  }
  cv_kill(&b->leaders);
  cv_foreach(meffect, b->meffects) {
    free(meffect);
  }
  cv_next(meffect);
  cv_kill(&b->meffects);
}

static int *
get_alive(side * s)
{
#if 0
  static int alive[NUMROWS];
  fighter *fig;
  memset(alive, 0, NUMROWS * sizeof(int));
  for (fig=s->fighters;fig;fig=fig->next) {
    if (fig->alive>0) {
      int row = statusrow(fig);
      alive[row] += fig->alive;
    }
  }
  return alive;
#endif
  return s->size;
}

static int
battle_report(battle * b)
{
  side *s, *s2;
  boolean cont = false;
  boolean komma;
  bfaction *bf;

  buf[0] = 0;

  for (s=b->sides; s; s=s->next) {
    for(s2=b->sides; s2; s2=s2->next) {
      if (s->alive-s->removed > 0 && s2->alive-s2->removed > 0 && enemy(s, s2)) {
        cont = true;
        break;
      }
    }
    if (cont) break;
  }

  printf(" %d", b->turn);
  fflush(stdout);

  for (bf=b->factions;bf;bf=bf->next) {
    faction * fac = bf->faction;
    char * bufp = buf;
    size_t size = sizeof(buf), rsize;
    message * m;

    fbattlerecord(b, fac, " ");

    if (cont) m = msg_message("battle::lineup", "turn", b->turn);
    else m = msg_message("battle::after", "");
    message_faction(b, fac, m);
    msg_release(m);

    komma   = false;
    for (s=b->sides; s; s=s->next) {
      if (s->alive) {
        int r, k = 0, * alive = get_alive(s);
        int l = FIGHT_ROW;
        const char * abbrev = seematrix(fac, s)?sideabkz(s, false):"-?-";
        const char * loc_army = LOC(fac->locale, "battle_army");
        char buffer[32];

        if (komma) {
          rsize = strlcpy(bufp, ", ", size);
          if (rsize>size) rsize = size-1;
          size -= rsize;
          bufp += rsize;
        }
        snprintf(buffer, sizeof(buffer), "%s %2d(%s): ",
          loc_army, army_index(s), abbrev);
        buffer[sizeof(buffer)-1] = 0;

        rsize = strlcpy(bufp, buffer, size);
        if (rsize>size) rsize = size-1;
        size -= rsize;
        bufp += rsize;

        for (r=FIGHT_ROW;r!=NUMROWS;++r) {
          if (alive[r]) {
            if (l!=FIGHT_ROW) {
              rsize = strlcpy(bufp, "+", size);
              if (rsize>size) rsize = size-1;
              size -= rsize;
              bufp += rsize;
            }
            while (k--) {
              rsize = strlcpy(bufp, "0+", size);
              if (rsize>size) rsize = size-1;
              size -= rsize;
              bufp += rsize;
            }
            sprintf(buffer, "%d", alive[r]);

            rsize = strlcpy(bufp, buffer, size);
            if (rsize>size) rsize = size-1;
            size -= rsize;
            bufp += rsize;

            k = 0;
            l = r+1;
          } else ++k;
        }

        komma = true;
      }
    }
    fbattlerecord(b, fac, buf);
  }
  return cont;
}

static void
join_allies(battle * b)
{
  region *r = b->region;
  unit *u;
  side *s, *sbegin = b->sides;
  /* make_side might be adding a new faciton, but it adds them to the beginning
   * of the list, so we're safe in our iteration here if we remember b->sides
   * up front. */
  for (u=r->units;u;u=u->next) {
    /* Was ist mit Schiffen? */
    if (u->status != ST_FLEE && u->status != ST_AVOID && !fval(u, UFL_LONGACTION|UFL_ISNEW) && u->number > 0) {
      faction * f = u->faction;
      fighter * c = NULL;

      for (s = sbegin; s; s=s->next) {
        side * se;
        /* Wenn alle attackierten noch FFL_NOAID haben, dann kämpfe nicht mit. */
        if (fval(s->bf->faction, FFL_NOAID)) continue;
        if (s->bf->faction!=f) {
          /* Wenn wir attackiert haben, kommt niemand mehr hinzu: */
          if (s->bf->attacker) continue;
          /* alliiert müssen wir schon sein, sonst ist's eh egal : */
          if (!alliedunit(u, s->bf->faction, HELP_FIGHT)) continue;
          /* wenn die partei verborgen ist, oder gar eine andere
          * vorgespiegelt wird, und er sich uns gegenüber nicht zu
          * erkennen gibt, helfen wir ihm nicht */
          if (s->stealthfaction){
            if(!allysfm(s, u->faction, HELP_FSTEALTH)) {
              continue;
            }
          }
        }
        /* einen alliierten angreifen dürfen sie nicht, es sei denn, der
        * ist mit einem alliierten verfeindet, der nicht attackiert
        * hat: */
        for (se = sbegin; se; se = se->next) {
          if (u->faction==se->bf->faction) continue;
          if (alliedunit(u, se->bf->faction, HELP_FIGHT) && !se->bf->attacker) {
            continue;
          }
          if (enemy(s, se)) break;
        }
        if (se==NULL) continue;
        /* Wenn die Einheit belagert ist, muß auch einer der Alliierten belagert sein: */
        if (besieged(u)) {
          fighter *ally;
          for (ally = s->fighters; ally; ally=ally->next) {
            if (besieged(ally->unit)) {
              break;
            }
          }
          if (ally==NULL) continue;
        }
        /* keine Einwände, also soll er mitmachen: */
        if (!c) c = join_battle(b, u, false);
        if (!c) continue;
        /* Die Feinde meiner Freunde sind meine Feinde: */
        for (se = sbegin; se; se=se->next) {
          if (se->bf->faction!=u->faction && enemy(s, se)) {
            set_enemy(se, c->side, false);
          }
        }
      }
    }
  }

  for (s=sbegin;s;s=s->next) {
    int si;
    /* Den Feinden meiner Feinde gebe ich Deckung (gegen gemeinsame Feinde): */
    for (si=0; s->enemies[si]; ++si) {
      side * se = s->enemies[si];
      int ai;
      for (ai=0; se->enemies[ai]; ++ai) {
        side * as = se->enemies[ai];
        if (as==s || !enemy(as, s)) {
          set_friendly(as, s);
        }
      }
    }
  }
}

static void
flee(const troop dt)
{
  fighter * fig = dt.fighter;
  unit * u = fig->unit;
  int carry = personcapacity(u) - u->race->weight;
  int money;
  item ** ip = &u->items;

  while (*ip) {
    item * itm = *ip;
    const item_type * itype = itm->type;
    int keep = 0;

    if (fval(itype, ITF_ANIMAL)) {
      /* Regeländerung: Man muß das Tier nicht reiten können,
       * um es vom Schlachtfeld mitzunehmen, ist ja nur
       * eine Region weit. * */
      keep = min(1, itm->number);
      /* da ist das weight des tiers mit drin */
      carry += itype->capacity - itype->weight;
    } else if (itm->type->weight <= 0) {
      /* if it doesn't weigh anything, it won't slow us down */
      keep = itm->number;
    }
    /* jeder troop nimmt seinen eigenen Teil der Sachen mit */
    if (keep>0){
      if (itm->number==keep) {
        i_add(&fig->run.items, i_remove(ip, itm));
      } else {
        item *run_itm = i_new(itype, keep);
        i_add(&fig->run.items, run_itm);
        i_change(ip, itype, -keep);
      }
    }
    if (*ip==itm) ip = &itm->next;
  }

  /* we will take money with us */
  money = get_money(u);
  /* nur ganzgeflohene/resttote Einheiten verlassen die Region */
  if (money > carry) money = carry;
  if (money > 0) {
    i_change(&u->items, i_silver, -money);
    i_change(&fig->run.items, i_silver, +money);
  }

  fig->run.hp += fig->person[dt.index].hp;
  ++fig->run.number;

  remove_troop(dt);
}

#ifdef DELAYED_OFFENSE
static boolean
guarded_by(region * r, faction * f)
{
  unit * u;
  for (u=r->units;u;u=u->next) {
    if (u->faction == f && getguard(u)) return true;
  }
  return false;
}
#endif

static boolean
init_battle(region * r, battle **bp)
{
  battle * b = NULL;
  unit * u;
  boolean fighting = false;

  /* list_foreach geht nicht, wegen flucht */
  for (u = r->units; u != NULL; u = u->next) {
    if (fval(u, UFL_LONGACTION)) continue;
    if (u->number > 0) {
      order * ord;

      for (ord=u->orders;ord;ord=ord->next) {
        static boolean init = false;
        static const curse_type * peace_ct, * slave_ct, * calm_ct;

        if (!init) {
          init = true;
          peace_ct = ct_find("peacezone");
          slave_ct = ct_find("slavery");
          calm_ct = ct_find("calmmonster");
        }
        if (get_keyword(ord) == K_ATTACK) {
          unit *u2;
          fighter *c1, *c2;

          if(r->planep && fval(r->planep, PFL_NOATTACK)) {
            cmistake(u, ord, 271, MSG_BATTLE);
            continue;
          }

          /**
          ** Fehlerbehandlung Angreifer
          **/
#ifdef DELAYED_OFFENSE
          if (get_moved(&u->attribs) && !guarded_by(r, u->faction)) {
            add_message(&u->faction->msgs,
              msg_message("no_attack_after_advance", "unit region command", u, u->region, ord));
          }
#endif
          if (LongHunger(u)) {
            cmistake(u, ord, 225, MSG_BATTLE);
            continue;
          }

          if (u->status == ST_AVOID || u->status == ST_FLEE) {
            cmistake(u, ord, 226, MSG_BATTLE);
            continue;
          }

          /* ist ein Flüchtling aus einem andern Kampf */
          if (fval(u, UFL_LONGACTION)) continue;

          if (peace_ct && curse_active(get_curse(r->attribs, peace_ct))) {
            sprintf(buf, "Hier ist es so schön friedlich, %s möchte "
              "hier niemanden angreifen.", unitname(u));
            mistake(u, ord, buf, MSG_BATTLE);
            continue;
          }

          if (slave_ct && curse_active(get_curse(u->attribs, slave_ct))) {
            sprintf(buf, "%s kämpft nicht.", unitname(u));
            mistake(u, ord, buf, MSG_BATTLE);
            continue;
          }


          if (is_guarded(r, u, GUARD_TRAVELTHRU)) {
            /* Fehler: "Das Schiff muß erst verlassen werden" */
            if (u->ship != NULL && !fval(r->terrain, SEA_REGION)) {
              cmistake(u, ord, 19, MSG_BATTLE);
              continue;
            }

            if (leftship(u)) {
              cmistake(u, ord, 234, MSG_BATTLE);
              continue;
            }
          }

          /* Ende Fehlerbehandlung Angreifer */

          init_tokens(ord);
          skip_token();
          /* attackierte Einheit ermitteln */
          u2 = getunit(r, u->faction);

          /* Beginn Fehlerbehandlung */
          /* Fehler: "Die Einheit wurde nicht gefunden" */
          if (!u2 || u2->number == 0 || !cansee(u->faction, u->region, u2, 0)) {
              cmistake(u, ord, 63, MSG_BATTLE);
              continue;
            }
            /* Fehler: "Die Einheit ist eine der unsrigen" */
            if (u2->faction == u->faction) {
              cmistake(u, ord, 45, MSG_BATTLE);
              continue;
            }
            /* Fehler: "Die Einheit ist mit uns alliert" */
            if (alliedunit(u, u2->faction, HELP_FIGHT)) {
              cmistake(u, ord, 47, MSG_BATTLE);
              continue;
            }
            /* xmas */
            if (u2->no==atoi36("xmas") && u2->irace==new_race[RC_GNOME]) {
              a_add(&u->attribs, a_new(&at_key))->data.i = atoi36("coal");
              sprintf(buf, "%s ist böse gewesen...", unitname(u));
              mistake(u, ord, buf, MSG_BATTLE);
              continue;
            } if (u2->faction->age < NewbieImmunity()) {
              add_message(&u->faction->msgs,
                msg_feedback(u, u->thisorder, "newbie_immunity_error", "turns", NewbieImmunity()));
              continue;
            }
            /* Fehler: "Die Einheit ist mit uns alliert" */

            if (calm_ct) {
              attrib * a = a_find(u->attribs, &at_curse);
              boolean calm = false;
              while (a && a->type==&at_curse) {
                curse * c = (curse *)a->data.v;
                if (c->type==calm_ct && c->effect.i==u2->faction->subscription) {
                  if (curse_active(c)) {
                    calm = true;
                    break;
                  }
                }
                a = a->next;
              }
              if (calm) {
                cmistake(u, ord, 47, MSG_BATTLE);
                continue;
              }
            }
            /* Ende Fehlerbehandlung */
#ifdef SIMPLE_COMBAT
            fset(u, UFL_LONGACTION);
#endif
            if (b==NULL) {
              unit * utmp;
              for (utmp=r->units; utmp!=NULL; utmp=utmp->next) {
                fset(utmp->faction, FFL_NOAID);
              }
              b = make_battle(r);
            }
            c1 = join_battle(b, u, true);
            c2 = join_battle(b, u2, false);

            /* Hat die attackierte Einheit keinen Noaid-Status,
            * wird das Flag von der Faction genommen, andere
            * Einheiten greifen ein. */
            if (!fval(u2, UFL_NOAID)) freset(u2->faction, FFL_NOAID);

            if (c1!=NULL && c2!=NULL) {
              /* Merken, wer Angreifer ist, für die Rückzahlung der
              * Präcombataura bei kurzem Kampf. */
              c1->side->bf->attacker = true;

              set_enemy(c1->side, c2->side, true);
              if (!enemy(c1->side, c2->side)) {
                sprintf(buf, "%s attackiert %s", sidename(c1->side, false),
                  sidename(c2->side, false));
                battledebug(buf);
              }
              fighting = true;
            }
        }
      }
    }
  }
  *bp = b;
  return fighting;
}

static void
battle_stats(FILE * F, battle * b)
{
  typedef struct stat_info {
    struct stat_info * next;
    const weapon_type * wtype;
    int level;
    int number;
  } stat_info;
  side * s;

  for (s = b->sides; s; s = s->next) {
    fighter * df;
    stat_info * stats = NULL, * stat;

    for (df = s->fighters; df; df = df->next) {
      unit *du = df->unit;
      troop dt;
      stat_info * slast = NULL;

      dt.fighter = df;
      for (dt.index=0;dt.index!=du->number;++dt.index) {
        weapon * wp = preferred_weapon(dt, true);
        int level = wp?wp->attackskill:0;
        const weapon_type * wtype = wp?wp->type:NULL;
        stat_info ** slist = &stats;

        if (slast && slast->wtype==wtype && slast->level==level) {
          ++slast->number;
          continue;
        }
        while (*slist && (*slist)->wtype!=wtype) {
          slist = &(*slist)->next;
        }
        while (*slist && (*slist)->wtype==wtype && (*slist)->level>level) {
          slist = &(*slist)->next;
        }
        stat = *slist;
        if (stat==NULL || stat->wtype!=wtype || stat->level!=level) {
          stat = calloc(1, sizeof(stat_info));
          stat->wtype = wtype;
          stat->level = level;
          stat->next = *slist;
          *slist = stat;
        }
        slast = stat;
        ++slast->number;
      }
    }

    fprintf(F, "##STATS## Heer %u - %s:\n", army_index(s), factionname(s->bf->faction));
    for (stat=stats;stat!=NULL;stat=stat->next) {
      fprintf(F, "%s %u : %u\n", stat->wtype?stat->wtype->itype->rtype->_name[0]:"none", stat->level, stat->number);
    }
    freelist(stats);
  }
}

/** execute one round of attacks
 * fig->fighting is used to determine who attacks, not fig->alive, since
 * the latter may be influenced by attacks that already took place.
 */
static void
battle_attacks(battle * b)
{
  side * s;

  for (s=b->sides;s;s=s->next) {
    fighter *fig;
    for (fig=s->fighters;fig;fig=fig->next) {

      /* ist in dieser Einheit noch jemand handlungsfähig? */
      if (fig->fighting <= 0) continue;

      /* Taktikrunde: */
      if (b->turn == 0) {
        side *stac;

        for (stac=b->sides; stac; stac=stac->next) {
          if (b->max_tactics > 0 && stac->leader.value == b->max_tactics && helping(stac, fig->side)) {
            break;
          }
        }
        if (stac==NULL) continue;
      }
      /* Handle the unit's attack on someone */
      do_attack(fig);
    }
  }
}

/** updates the number of attacking troops in each fighter struct.
 * this has to be calculated _before_ the actual attacks take
 * place because otherwise dead troops would not strike in the
 * round they die. */
static void 
battle_update(battle * b)
{
  side * s;
  for (s=b->sides;s;s=s->next) {
    fighter *fig;
    for (fig=s->fighters;fig;fig=fig->next) {
      fig->fighting = fig->alive - fig->removed;
    }
  }
}

/** attempt to flee from battle before the next round begins
 * there's a double attempt before the first round, but only
 * one attempt before round zero, the potential tactics round. */
static void
battle_flee(battle * b)
{
  int attempt, flee_ops = 1;

  if (b->turn==1)
    flee_ops = 2;

  for (attempt=1;attempt<=flee_ops;++attempt) {
    side * s;
    for (s=b->sides;s;s=s->next) {
      fighter *fig;
      for (fig=s->fighters;fig;fig=fig->next) {
        unit *u = fig->unit;
        troop dt;
        int runners = 0;
        /* Flucht nicht bei mehr als 600 HP. Damit Wyrme tötbar bleiben. */
        int runhp = min(600,(int)(0.9+unit_max_hp(u)*hpflee(u->status)));
        if (fval(u->race, RCF_UNDEAD) || u->race == new_race[RC_SHADOWKNIGHT]) continue;

        if (u->ship) continue;
        dt.fighter = fig;
        if (!fig->run.region) fig->run.region = fleeregion(u);
        if (!fig->run.region) continue;
        dt.index = fig->alive - fig->removed;
        while (s->size[SUM_ROW] && dt.index != 0) {
          double ispaniced = 0.0;
          --dt.index;
          assert(dt.index>=0 && dt.index<fig->unit->number);
          assert(fig->person[dt.index].hp > 0);

          /* Versuche zu fliehen, wenn
          * - Kampfstatus fliehe
          * - schwer verwundet und nicht erste kampfrunde
          * - in panik (Zauber)
          * aber nicht, wenn der Zaubereffekt Held auf dir liegt!
          */
          switch (u->status) {
              case ST_FLEE:
                break;
              default:
                if ((fig->person[dt.index].flags & FL_HIT) == 0) continue;
                if (b->turn<=1) continue;
                if (fig->person[dt.index].hp <= runhp) break;
                if (fig->person[dt.index].flags & FL_PANICED) {
                  if ((fig->person[dt.index].flags & FL_COURAGE)==0) break;
                }
                continue;
          }

          if (fig->person[dt.index].flags & FL_PANICED) {
            ispaniced = EFFECT_PANIC_SPELL;
          }
          if (chance(min(fleechance(u)+ispaniced, 0.90))) {
            ++runners;
            flee(dt);
#ifdef SMALL_BATTLE_MESSAGES
            if (b->small) {
              sprintf(smallbuf, "%s/%d gelingt es, vom Schlachtfeld zu entkommen.",
                unitname(fig->unit), dt.index);
              battlerecord(b, smallbuf);
            }
          } else if (b->small) {
            sprintf(smallbuf, "%s/%d versucht zu fliehen, wird jedoch aufgehalten.",
              unitname(fig->unit), dt.index);
            battlerecord(b, smallbuf);
#endif
          }
        }
        if (runners > 0) {
          char lbuf[256];
          sprintf(lbuf, "Flucht: %d aus %s", runners, itoa36(fig->unit->no));
          battledebug(lbuf);
        }
      }
    }
  }
}

void
do_battle(region * r)
{
#ifdef SMALL_BATTLE_MESSAGES
  char smallbuf[512];
#endif
  battle *b = NULL;
  boolean fighting = false;
  ship * sh;
  building *bu;

  fighting = init_battle(r, &b);

  if (b==NULL) return;

  /* Bevor wir die alliierten hineinziehen, sollten wir schauen, *
  * Ob jemand fliehen kann. Dann erübrigt sich das ganze ja
  * vielleicht schon. */
  print_header(b);
  if (!fighting) {
    /* Niemand mehr da, Kampf kann nicht stattfinden. */
    message * m = msg_message("battle::aborted", "");
    message_all(b, m);
    msg_release(m);
    free_battle(b);
    free(b);
    return;
  }
  join_allies(b);
#ifdef HEROES
  make_heroes(b);
#endif
  /* Alle Mann raus aus der Burg! */
  for (bu=r->buildings; bu!=NULL; bu=bu->next) bu->sizeleft = bu->size;

  /* make sure no ships are damaged initially */
  for (sh=r->ships; sh; sh=sh->next) freset(sh, SF_DAMAGED);

  /* Gibt es eine Taktikrunde ? */
  if (cv_size(&b->leaders)) {
    b->turn = 0;
    b->has_tactics_turn = true;
  } else {
    b->turn = 1;
    b->has_tactics_turn = false;
  }

  if (b->region->flags & RF_COMBATDEBUG) battle_stats(bdebug, b);

  /* PRECOMBATSPELLS */
  do_combatmagic(b, DO_PRECOMBATSPELL);

  print_stats(b); /* gibt die Kampfaufstellung aus */
  printf("%s (%d, %d) : ", rname(r, NULL), r->x, r->y);

#ifdef SMALL_BATTLE_MESSAGES
  if (b->nfighters <= 30) {
    b->small = true;
  } else {
    b->small = false;
  }
#endif

  for (;battle_report(b) && b->turn<=COMBAT_TURNS;++b->turn) {
    char lbuf[256];

    sprintf(lbuf, "*** Runde: %d", b->turn);
    battledebug(lbuf);

    battle_flee(b);
    battle_update(b);
    battle_attacks(b);

#ifdef KARMA_MODULE
    /* Regeneration */
    for (fi=0;fi!=b->nfighters;++fi) {
      fighter *fig = b->fighters[fi];

      if (fspecial(fig->unit->faction, FS_REGENERATION)) {
        fig->fighting = fig->alive - fig->removed;
        if (fig->fighting == 0) continue;
        do_regenerate(fig);
      }
    }
#endif /* KARMA_MODULE */
  }

  printf("\n");

  /* Auswirkungen berechnen: */
  aftermath(b);
  /* Hier ist das Gefecht beendet, und wir können die
  * Hilfsstrukturen * wieder löschen: */

  if (b) {
    free_battle(b);
    free(b);
  }
}
