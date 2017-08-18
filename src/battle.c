/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
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
#include "battle.h"
#include "alchemy.h"
#include "chaos.h"
#include "guard.h"
#include "laws.h"
#include "monsters.h"
#include "move.h"
#include "skill.h"
#include "study.h"

#include <kernel/ally.h>
#include <kernel/alliance.h>
#include <kernel/build.h>
#include <kernel/building.h>
#include <kernel/curse.h>
#include <kernel/equipment.h>
#include <kernel/faction.h>
#include <kernel/group.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>
#include <kernel/spell.h>

#include <reports.h>

/* attributes includes */
#include <attributes/key.h>
#include <attributes/fleechance.h>
#include <attributes/racename.h>
#include <attributes/otherfaction.h>
#include <attributes/moved.h>
#include <spells/buildingcurse.h>

/* util includes */
#include <util/assert.h>
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/parser.h>
#include <selist.h>
#include <util/rand.h>
#include <util/rng.h>

/* libc includes */
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#define TACTICS_BONUS 1         /* when undefined, we have a tactics round. else this is the bonus tactics give */
#define TACTICS_MODIFIER 1      /* modifier for generals in the front/rear */

#define CATAPULT_INITIAL_RELOAD 4       /* erster schuss in runde 1 + rng_int() % INITIAL */
#define CATAPULT_STRUCTURAL_DAMAGE

#define BASE_CHANCE    70       /* 70% Basis-�berlebenschance */
#define TDIFF_CHANGE    5       /* 5% h�her pro Stufe */
#define DAMAGE_QUOTIENT 2       /* damage += skilldiff/DAMAGE_QUOTIENT */

#define DEBUG_SELECT            /* should be disabled if select_enemy works */

typedef enum combatmagic {
    DO_PRECOMBATSPELL,
    DO_POSTCOMBATSPELL
} combatmagic_t;

#define MINSPELLRANGE 1
#define MAXSPELLRANGE 7

#ifndef ROW_FACTOR
# define ROW_FACTOR 10
#endif
#define EFFECT_PANIC_SPELL 0.25
#define TROLL_REGENERATION 0.10

/* Nach dem alten System: */
static int missile_range[2] = { FIGHT_ROW, BEHIND_ROW };
static int melee_range[2] = { FIGHT_ROW, FIGHT_ROW };

static message *msg_separator;

const troop no_troop = { 0, 0 };

#define FORMULA_ORIG 0
#define FORMULA_NEW 1

#define LOOT_MONSTERS      (1<<0)
#define LOOT_SELF          (1<<1)       /* code is mutually exclusive with LOOT_OTHERS */
#define LOOT_OTHERS        (1<<2)
#define LOOT_KEEPLOOT      (1<<4)

#define DAMAGE_CRITICAL      (1<<0)
#define DAMAGE_MELEE_BONUS   (1<<1)
#define DAMAGE_MISSILE_BONUS (1<<2)
#define DAMAGE_SKILL_BONUS   (1<<4)

static int max_turns;
static int rule_damage;
static int rule_loot;
static int skill_formula;
static int rule_cavalry_skill;
static int rule_population_damage;
static int rule_hero_speed;
static bool rule_anon_battle;
static int rule_goblin_bonus;
static int rule_tactics_formula;
static int rule_nat_armor;
static int rule_cavalry_mode;
static int rule_vampire;

static const curse_type *peace_ct, *slave_ct, *calm_ct;

/** initialize rules from configuration.
 */
static void init_rules(void)
{
    peace_ct = ct_find("peacezone");
    slave_ct = ct_find("slavery");
    calm_ct = ct_find("calmmonster");
    rule_nat_armor = config_get_int("rules.combat.nat_armor", 0);
    rule_tactics_formula = config_get_int("rules.tactics.formula", 0);
    rule_goblin_bonus = config_get_int("rules.combat.goblinbonus", 10);
    rule_hero_speed = config_get_int("rules.combat.herospeed", 10);
    rule_population_damage = config_get_int("rules.combat.populationdamage", 20);
    rule_anon_battle = config_get_int("rules.stealth.anon_battle", 1) != 0;
    rule_cavalry_mode = config_get_int("rules.cavalry.mode", 1);
    rule_cavalry_skill = config_get_int("rules.cavalry.skill", 2);
    rule_vampire = config_get_int("rules.combat.demon_vampire", 0);
    rule_loot = config_get_int("rules.combat.loot",
        LOOT_MONSTERS | LOOT_OTHERS | LOOT_KEEPLOOT);
    /* new formula to calculate to-hit-chance */
    skill_formula = config_get_int("rules.combat.skill_formula",
        FORMULA_ORIG);
    /* maximum number of combat turns */
    max_turns = config_get_int("rules.combat.turns", COMBAT_TURNS);
    /* damage calculation */
    if (config_get_int("rules.combat.critical", 1)) {
        rule_damage |= DAMAGE_CRITICAL;
    }
    if (config_get_int("rules.combat.melee_bonus", 1)) {
        rule_damage |= DAMAGE_MELEE_BONUS;
    }
    if (config_get_int("rules.combat.missile_bonus", 1)) {
        rule_damage |= DAMAGE_MISSILE_BONUS;
    }
    if (config_get_int("rules.combat.skill_bonus", 1)) {
        rule_damage |= DAMAGE_SKILL_BONUS;
    }
}

static int army_index(side * s)
{
    return s->index;
}

static char *sidename(side * s)
{
#define SIDENAMEBUFLEN 256
    static int bufno;             /* STATIC_XCALL: used across calls */
    static char sidename_buf[4][SIDENAMEBUFLEN];  /* STATIC_RESULT: used for return, not across calls */

    bufno = bufno % 4;
    strlcpy(sidename_buf[bufno], factionname(s->stealthfaction ? s->stealthfaction : s->faction), SIDENAMEBUFLEN);
    return sidename_buf[bufno++];
}

static const char *sideabkz(side * s, bool truename)
{
    static char sideabkz_buf[8];  /* STATIC_RESULT: used for return, not across calls */
    const faction *f = (s->stealthfaction
        && !truename) ? s->stealthfaction : s->faction;

    strlcpy(sideabkz_buf, itoa36(f->no), sizeof(sideabkz_buf));
    return sideabkz_buf;
}

static void message_faction(battle * b, faction * f, struct message *m)
{
    region *r = b->region;

    assert(f);
    if (f->battles == NULL || f->battles->r != r) {
        struct bmsg *bm = (struct bmsg *)calloc(1, sizeof(struct bmsg));
        assert_alloc(bm);
        bm->next = f->battles;
        f->battles = bm;
        bm->r = r;
    }
    add_message(&f->battles->msgs, m);
}

void message_all(battle * b, message * m)
{
    bfaction *bf;

    for (bf = b->factions; bf; bf = bf->next) {
        assert(bf->faction);
        message_faction(b, bf->faction, m);
    }
}

static void fbattlerecord(battle * b, faction * f, const char *s)
{
    message *m = msg_message("battle_msg", "string", s);
    message_faction(b, f, m);
    msg_release(m);
}

/* being an enemy or a friend is (and must always be!) symmetrical */
#define enemy_i(as, di) (as->relations[di]&E_ENEMY)
#define friendly_i(as, di) (as->relations[di]&E_FRIEND)
#define enemy(as, ds) (as->relations[ds->index]&E_ENEMY)
#define friendly(as, ds) (as->relations[ds->index]&E_FRIEND)

static bool set_enemy(side * as, side * ds, bool attacking)
{
    int i;
    assert(as && ds);
    for (i = 0; i != MAXSIDES; ++i) {
        if (ds->enemies[i] == NULL)
            ds->enemies[i] = as;
        if (ds->enemies[i] == as)
            break;
    }
    for (i = 0; i != MAXSIDES; ++i) {
        if (as->enemies[i] == NULL)
            as->enemies[i] = ds;
        if (as->enemies[i] == ds)
            break;
    }
    assert(i != MAXSIDES);
    if (attacking)
        as->relations[ds->index] |= E_ATTACKING;
    if ((ds->relations[as->index] & E_ENEMY) == 0) {
        /* enemy-relation are always symmetrical */
        assert((as->relations[ds->index] & (E_ENEMY | E_FRIEND)) == 0);
        ds->relations[as->index] |= E_ENEMY;
        as->relations[ds->index] |= E_ENEMY;
        return true;
    }
    return false;
}

static void set_friendly(side * as, side * ds)
{
    assert((as->relations[ds->index] & E_ENEMY) == 0);
    ds->relations[as->index] |= E_FRIEND;
    as->relations[ds->index] |= E_FRIEND;
}

static int allysfm(const side * s, const faction * f, int mode)
{
    if (s->faction == f)
        return mode;
    if (s->group) {
        return alliedgroup(s->battle->plane, s->faction, f, s->group->allies, mode);
    }
    return alliedfaction(s->battle->plane, s->faction, f, mode);
}

static int allysf(const side * s, const faction * f)
{
    return allysfm(s, f, HELP_FIGHT);
}

static int dead_fighters(const fighter * df)
{
    return df->unit->number - df->alive - df->run.number;
}

fighter *select_corpse(battle * b, fighter * af)
/* W�hlt eine Leiche aus, der af hilft. casualties ist die Anzahl der
 * Toten auf allen Seiten (im Array). Wenn af == NULL, wird die
 * Parteizugeh�rigkeit ignoriert, und irgendeine Leiche genommen.
 *
 * Untote werden nicht ausgew�hlt (casualties, not dead) */
{
    int si, maxcasualties = 0;
    fighter *df;

    for (si = 0; si != b->nsides; ++si) {
        side *s = b->sides + si;
        if (af == NULL || (!enemy_i(af->side, si) && allysf(af->side, s->faction))) {
            maxcasualties += s->casualties;
        }
    }
    if (maxcasualties > 0) {
        int di = (int)(rng_int() % maxcasualties);
        side *s;
        for (s = b->sides; s != b->sides + b->nsides; ++s) {
            for (df = s->fighters; df; df = df->next) {
                /* Geflohene haben auch 0 hp, d�rfen hier aber nicht ausgew�hlt
                 * werden! */
                int dead = dead_fighters(df);
                if (!playerrace(u_race(df->unit)))
                    continue;

                if (af && !helping(af->side, df->side))
                    continue;
                if (di < dead) {
                    return df;
                }
                di -= dead;
            }
        }
    }
    return NULL;
}

bool helping(const side * as, const side * ds)
{
    if (as->faction == ds->faction)
        return true;
    return (bool)(!enemy(as, ds) && allysf(as, ds->faction));
}

int statusrow(int status)
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

static double hpflee(int status)
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

static int get_row(const side * s, int row, const side * vs)
{
    bool counted[MAXSIDES];
    int enemyfront = 0;
    int line, result;
    int retreat = 0;
    int size[NUMROWS];
    int front = 0;
    battle *b = s->battle;

    memset(counted, 0, sizeof(counted));
    memset(size, 0, sizeof(size));
    for (line = FIRST_ROW; line != NUMROWS; ++line) {
        int si, sa_i;
        /* how many enemies are there in the first row? */
        for (si = 0; s->enemies[si]; ++si) {
            side *se = s->enemies[si];
            if (se->size[line] > 0) {
                enemyfront += se->size[line];
                /* - s->nonblockers[line] (nicht, weil angreifer) */
            }
        }
        for (sa_i = 0; sa_i != b->nsides; ++sa_i) {
            side *sa = b->sides + sa_i;
            /* count people that like me, but don't like my enemy */
            if (friendly_i(s, sa_i) && enemy_i(vs, sa_i)) {
                if (!counted[sa_i]) {
                    int i;

                    for (i = 0; i != NUMROWS; ++i) {
                        size[i] += sa->size[i] - sa->nonblockers[i];
                    }
                    counted[sa_i] = true;
                }
            }
        }
        if (enemyfront)
            break;
    }
    if (enemyfront) {
        for (line = FIRST_ROW; line != NUMROWS; ++line) {
            front += size[line];
            if (!front || front < enemyfront / ROW_FACTOR)
                ++retreat;
            else if (front)
                break;
        }
    }

    /* every entry in the size[] array means someone trying to defend us.
     * 'retreat' is the number of rows falling.
     */
    result = MAX(FIRST_ROW, row - retreat);

    return result;
}

int get_unitrow(const fighter * af, const side * vs)
{
    int row = statusrow(af->status);
    if (vs == NULL) {
        int i;
        for (i = FIGHT_ROW; i != row; ++i)
            if (af->side->size[i])
                break;
        return FIGHT_ROW + (row - i);
    }
    else {
        battle *b = vs->battle;
        if (row != b->rowcache.row || b->alive != b->rowcache.alive
            || af->side != b->rowcache.as || vs != b->rowcache.vs) {
            b->rowcache.alive = b->alive;
            b->rowcache.as = af->side;
            b->rowcache.vs = vs;
            b->rowcache.row = row;
            b->rowcache.result = get_row(af->side, row, vs);
            return b->rowcache.result;
        }
        return b->rowcache.result;
    }
}

static void reportcasualties(battle * b, fighter * fig, int dead)
{
    struct message *m;
    region *r = NULL;
    if (fig->alive == fig->unit->number)
        return;
    m = msg_message("casualties", "unit runto run alive fallen",
        fig->unit, r, fig->run.number, fig->alive, dead);
    message_all(b, m);
    msg_release(m);
}

static int
contest_classic(int skilldiff, const armor_type * ar, const armor_type * sh)
{
    int p, vw = BASE_CHANCE - TDIFF_CHANGE * skilldiff;
    double mod = 1.0;

    if (ar != NULL)
        mod *= (1 + ar->penalty);
    if (sh != NULL)
        mod *= (1 + sh->penalty);
    vw = (int)(100 - ((100 - vw) * mod));

    do {
        p = (int)(rng_int() % 100);
        vw -= p;
    } while (vw >= 0 && p >= 90);
    return (vw <= 0);
}

/** new rule for Eressea 1.5
 * \param skilldiff - the attack skill with every modifier applied
 */
static int
contest_new(int skilldiff, const troop dt, const armor_type * ar,
const armor_type * sh)
{
    double tohit = 0.5 + skilldiff * 0.1;

    UNUSED_ARG(sh);
    UNUSED_ARG(ar);
    if (tohit < 0.5)
        tohit = 0.5;
    if (chance(tohit)) {
        int defense = effskill(dt.fighter->unit, SK_STAMINA, dt.fighter->unit->region);
        double tosave = defense * 0.05;
        return !chance(tosave);
    }
    return 0;
}

static int
contest(int skdiff, const troop dt, const armor_type * ar,
const armor_type * sh)
{
    if (skill_formula == FORMULA_ORIG) {
        return contest_classic(skdiff, ar, sh);
    }
    else {
        return contest_new(skdiff, dt, ar, sh);
    }
}

static bool is_riding(const troop t)
{
    if (t.fighter->building != NULL)
        return false;
    if (t.fighter->horses + t.fighter->elvenhorses > t.index)
        return true;
    return false;
}

static weapon *preferred_weapon(const troop t, bool attacking)
{
    weapon *missile = t.fighter->person[t.index].missile;
    weapon *melee = t.fighter->person[t.index].melee;
    if (attacking) {
        if (melee == NULL || (missile && missile->attackskill > melee->attackskill)) {
            return missile;
        }
    }
    else {
        if (melee == NULL || (missile
            && missile->defenseskill > melee->defenseskill)) {
            return missile;
        }
    }
    return melee;
}

static weapon *select_weapon(const troop t, bool attacking,
    bool ismissile)
    /* select the primary weapon for this trooper */
{
    if (attacking) {
        if (ismissile) {
            /* from the back rows, have to use your missile weapon */
            return t.fighter->person[t.index].missile;
        }
    }
    else {
        if (!ismissile) {
            /* have to use your melee weapon if it's melee */
            return t.fighter->person[t.index].melee;
        }
    }
    return preferred_weapon(t, attacking);
}

static bool i_canuse(const unit * u, const item_type * itype)
{
    return rc_can_use(u_race(u), itype);
}

static int
weapon_skill(const weapon_type * wtype, const unit * u, bool attacking)
/* the 'pure' skill when using this weapon to attack or defend.
 * only undiscriminate modifiers (not affected by troops or enemies)
 * are taken into account, e.g. no horses, magic, etc. */
{
    int skill;

    if (wtype == NULL) {
        skill = effskill(u, SK_WEAPONLESS, 0);
        if (skill <= 0) {
            /* wenn kein waffenloser kampf, dann den rassen-defaultwert */
            if (u_race(u) == get_race(RC_ORC)) {
                int sword = effskill(u, SK_MELEE, 0);
                int spear = effskill(u, SK_SPEAR, 0);
                skill = MAX(sword, spear) - 3;
                if (attacking) {
                    skill = MAX(skill, u_race(u)->at_default);
                }
                else {
                    skill = MAX(skill, u_race(u)->df_default);
                }
            }
            else {
                if (attacking) {
                    skill = u_race(u)->at_default;
                }
                else {
                    skill = u_race(u)->df_default;
                }
            }
        }
        else {
            /* der rassen-defaultwert kann h�her sein als der Talentwert von
             * waffenloser kampf */
            if (attacking) {
                if (skill < u_race(u)->at_default)
                    skill = u_race(u)->at_default;
            }
            else {
                if (skill < u_race(u)->df_default)
                    skill = u_race(u)->df_default;
            }
        }
        if (attacking) {
            skill += u_race(u)->at_bonus;
            if (fval(u->region->terrain, SEA_REGION) && u->ship)
                skill += u->ship->type->at_bonus;
        }
        else {
            skill += u_race(u)->df_bonus;
            if (fval(u->region->terrain, SEA_REGION) && u->ship)
                skill += u->ship->type->df_bonus;
        }
    }
    else {
        /* changed: if we own a weapon, we have at least a skill of 0 */
        if (!i_canuse(u, wtype->itype))
            return -1;
        skill = effskill(u, wtype->skill, 0);
        if (skill < wtype->minskill)
            skill = 0;
        if (skill > 0) {
            if (attacking) {
                skill += u_race(u)->at_bonus;
            }
            else {
                skill += u_race(u)->df_bonus;
            }
        }
        if (attacking) {
            skill += wtype->offmod;
        }
        else {
            skill += wtype->defmod;
        }
    }

    return skill;
}

static int CavalrySkill(void)
{
    return rule_cavalry_skill;
}

#define BONUS_SKILL 1
#define BONUS_DAMAGE 2
static int CavalryBonus(const unit * u, troop enemy, int type)
{
    if (rule_cavalry_mode == 0) {
        /* old rule, Eressea 1.0 compat */
        return (type == BONUS_SKILL) ? 2 : 0;
    }
    else {
        /* new rule, chargers in Eressea 1.1 */
        int skl = effskill(u, SK_RIDING, 0);
        /* only half against trolls */
        if (skl > 0) {
            if (type == BONUS_SKILL) {
                int dmg = MIN(skl, 8);
                if (u_race(enemy.fighter->unit) == get_race(RC_TROLL)) {
                    dmg = dmg / 4;
                }
                else {
                    dmg = dmg / 2;
                }
                return dmg;
            }
            else {
                skl = skl / 2;
                return MIN(skl, 4);
            }
        }
    }
    return 0;
}

static int
weapon_effskill(troop t, troop enemy, const weapon * w, bool attacking,
bool missile)
/* effektiver Waffenskill w�hrend des Kampfes */
{
    /* In dieser Runde alle die Modifier berechnen, die fig durch die
     * Waffen bekommt. */
    fighter *tf = t.fighter;
    unit *tu = t.fighter->unit;
    int skill;
    const weapon_type *wtype = w ? w->type : NULL;

    if (wtype == NULL) {
        /* Ohne Waffe: Waffenlose Angriffe */
        skill = weapon_skill(NULL, tu, attacking);
    }
    else {
        if (attacking) {
            skill = w->attackskill;
        }
        else {
            skill = w->defenseskill;
        }
        if (wtype->modifiers != NULL) {
            /* Pferdebonus, Lanzenbonus, usw. */
            int m;
            unsigned int flags =
                WMF_SKILL | (attacking ? WMF_OFFENSIVE : WMF_DEFENSIVE);

            if (is_riding(t))
                flags |= WMF_RIDING;
            else
                flags |= WMF_WALKING;
            if (is_riding(enemy))
                flags |= WMF_AGAINST_RIDING;
            else
                flags |= WMF_AGAINST_WALKING;

            for (m = 0; wtype->modifiers[m].value; ++m) {
                if ((wtype->modifiers[m].flags & flags) == flags) {
                    race_list *rlist = wtype->modifiers[m].races;
                    if (rlist != NULL) {
                        while (rlist) {
                            if (rlist->data == u_race(tu))
                                break;
                            rlist = rlist->next;
                        }
                        if (rlist == NULL)
                            continue;
                    }
                    skill += wtype->modifiers[m].value;
                }
            }
        }
    }

    /* Burgenbonus, Pferdebonus */
    if (is_riding(t) && (wtype == NULL || (fval(wtype, WTF_HORSEBONUS)
        && !fval(wtype, WTF_MISSILE)))) {
        skill += CavalryBonus(tu, enemy, BONUS_SKILL);
    }

    if (t.index < tf->elvenhorses) {
        /* Elfenpferde: Helfen dem Reiter, egal ob und welche Waffe. Das ist
         * eleganter, und vor allem einfacher, sonst mu� man noch ein
         * WMF_ELVENHORSE einbauen. */
        skill += 2;
    }

    if (skill > 0 && !attacking && missile) {
        /*
         * Wenn ich verteidige, und nicht direkt meinem Feind gegen�berstehe,
         * halbiert sich mein Skill: (z.B. gegen Fernk�mpfer. Nahk�mpfer
         * k�nnen mich eh nicht treffen)
         */
        skill /= 2;
    }
    return skill;
}

static const armor_type *select_armor(troop t, bool shield)
{
    unsigned int type = shield ? ATF_SHIELD : 0;
    unit *u = t.fighter->unit;
    const armor *a = t.fighter->armors;
    int geschuetzt = 0;

    /* some monsters should not use armor (dragons in chainmail? ha!) */
    if (!(u_race(u)->battle_flags & BF_EQUIPMENT))
        return NULL;

    /* ... neither do werewolves */
    if (fval(u, UFL_WERE)) {
        return NULL;
    }

    for (; a; a = a->next) {
        if ((a->atype->flags & ATF_SHIELD) == type) {
            geschuetzt += a->count;
            if (geschuetzt > t.index) {
                /* unser Kandidat wird geschuetzt */
                return a->atype;
            }
        }
    }
    return NULL;
}

/* Hier ist zu beachten, ob und wie sich Zauber und Artefakte, die
 * R�stungschutz geben, addieren.
 * - Artefakt "trollbelt" gibt R�stung +1
 * - Zauber Rindenhaut gibt R�stung +3
 */
static int trollbelts(const unit *u) {
    const struct resource_type *belt = rt_find("trollbelt");
    return belt ? i_get(u->items, belt->itype) : 0;
}

int select_magicarmor(troop t)
{
    unit *u = t.fighter->unit;
    int ma = 0;

    if (trollbelts(u) > t.index)     /* unser Kandidat wird geschuetzt */
        ma += 1;

    return ma;
}

/* Sind side ds und Magier des meffect verb�ndet, dann return 1*/
bool meffect_protection(battle * b, meffect * s, side * ds)
{
    UNUSED_ARG(b);
    if (!s->magician->alive)
        return false;
    if (s->duration <= 0)
        return false;
    if (enemy(s->magician->side, ds))
        return false;
    if (allysf(s->magician->side, ds->faction))
        return true;
    return false;
}

/* Sind side as und Magier des meffect verfeindet, dann return 1*/
bool meffect_blocked(battle * b, meffect * s, side * as)
{
    UNUSED_ARG(b);
    if (!s->magician->alive)
        return false;
    if (s->duration <= 0)
        return false;
    if (enemy(s->magician->side, as))
        return true;
    return false;
}

/* rmfighter wird schon im PRAECOMBAT gebraucht, da gibt es noch keine
 * troops */
void rmfighter(fighter * df, int i)
{
    side *ds = df->side;

    /* nicht mehr personen abziehen, als in der Einheit am Leben sind */
    assert(df->alive >= i);
    assert(df->alive <= df->unit->number);

    /* erst ziehen wir die Anzahl der Personen von den K�mpfern in der
     * Schlacht, dann von denen auf dieser Seite ab*/
    df->side->alive -= i;
    df->side->battle->alive -= i;

    /* Dann die Kampfreihen aktualisieren */
    ds->size[SUM_ROW] -= i;
    ds->size[statusrow(df->status)] -= i;

    /* Spezialwirkungen, z.B. Schattenritter */
    if (u_race(df->unit)->battle_flags & BF_NOBLOCK) {
        ds->nonblockers[SUM_ROW] -= i;
        ds->nonblockers[statusrow(df->status)] -= i;
    }

    /* und die Einheit selbst aktualisieren */
    df->alive -= i;
}

static void rmtroop(troop dt)
{
    fighter *df = dt.fighter;

    /* troop ist immer eine einzele Person */
    rmfighter(df, 1);

    assert(dt.index >= 0 && dt.index < df->unit->number);
    if (dt.index!=df->alive-df->removed) {
        df->person[dt.index] = df->person[df->alive - df->removed];
    }
    if (df->removed) {
        df->person[df->alive - df->removed] = df->person[df->alive];
    }
    df->person[df->alive].hp = 0;
}

void remove_troop(troop dt)
{
    fighter *df = dt.fighter;
    struct person p = df->person[dt.index];
    battle *b = df->side->battle;
    b->fast.alive = -1;           /* invalidate cached value */
    b->rowcache.alive = -1;       /* invalidate cached value */
    ++df->removed;
    ++df->side->removed;
    df->person[dt.index] = df->person[df->alive - df->removed];
    df->person[df->alive - df->removed] = p;
}

void kill_troop(troop dt)
{
    fighter *df = dt.fighter;
    unit *du = df->unit;

    rmtroop(dt);
    if (!df->alive) {
        char eqname[64];
        const struct equipment *eq;
        const race *rc = u_race(du);
        item *drops = item_spoil(rc, du->number - df->run.number);
        if (drops != NULL) {
            i_merge(&du->items, &drops);
        }
        sprintf(eqname, "%s_spoils", rc->_name);
        eq = get_equipment(eqname);
        if (eq != NULL) {
            equip_items(&du->items, eq);
        }
    }
}

/** reduces the target's exp by an equivalent of n points learning
 * 30 points = 1 week
 */
void drain_exp(struct unit *u, int n)
{
    skill_t sk = (skill_t)(rng_int() % MAXSKILLS);
    skill_t ssk;

    ssk = sk;

    while (get_level(u, sk) == 0) {
        sk++;
        if (sk == MAXSKILLS)
            sk = 0;
        if (sk == ssk) {
            sk = NOSKILL;
            break;
        }
    }
    if (sk != NOSKILL) {
        reduce_skill_days(u, sk, n);
    }
}

const char *rel_dam(int dam, int hp)
{
    double q = (double)dam / (double)hp;

    if (q > 0.75) {
        return "eine klaffende Wunde";
    }
    else if (q > 0.5) {
        return "eine schwere Wunde";
    }
    else if (q > 0.25) {
        return "eine Wunde";
    }
    return "eine kleine Wunde";
}

static void vampirism(troop at, int damage)
{
    if (rule_vampire > 0) {
        int gain = damage / rule_vampire;
        int chance = damage - rule_vampire * gain;
        if (chance > 0 && (rng_int() % rule_vampire < chance))
            ++gain;
        if (gain > 0) {
            int maxhp = unit_max_hp(at.fighter->unit);
            at.fighter->person[at.index].hp =
                MIN(gain + at.fighter->person[at.index].hp, maxhp);
        }
    }
}

#define MAXRACES 128

int natural_armor(unit * du)
{
    const race *rc = u_race(du);
    int an;

    assert(rc);
    an = rc_armor_bonus(rc);
    if (an > 0) {
        int sk = effskill(du, SK_STAMINA, 0);
        return rc->armor + sk / an;
    }
    return rc->armor;
}

static int rc_specialdamage(const unit *au, const unit *du, const struct weapon_type *wtype)
{
    const race *ar = u_race(au);
    int m, modifier = 0;
    if (wtype != NULL) {
        if (fval(u_race(du), RCF_DRAGON)) {
            static int cache;
            static const race *rc_halfling;
            if (rc_changed(&cache)) {
                rc_halfling = get_race(RC_HALFLING);
            }
            if (ar == rc_halfling) {
                modifier += 5;
            }
        }
        if (wtype->modifiers != NULL) {
            for (m = 0; wtype->modifiers[m].value; ++m) {
                /* weapon damage for this weapon, possibly by race */
                if (wtype->modifiers[m].flags & WMF_DAMAGE) {
                    race_list *rlist = wtype->modifiers[m].races;
                    if (rlist != NULL) {
                        while (rlist) {
                            if (rlist->data == ar)
                                break;
                            rlist = rlist->next;
                        }
                        if (rlist == NULL)
                            continue;
                    }
                    modifier += wtype->modifiers[m].value;
                }
            }
        }
    }
    return modifier;
}

int calculate_armor(troop dt, const weapon_type *dwtype, const weapon_type *awtype, variant *magres) {
    fighter *df = dt.fighter;
    unit *du = df->unit;
    int ar = 0, an, am;
    const armor_type *armor = select_armor(dt, false);
    const armor_type *shield = select_armor(dt, true);
    bool missile = awtype && (awtype->flags&WTF_MISSILE);

    if (armor) {
        ar += armor->prot;
        if (missile && armor->projectile > 0 && chance(armor->projectile)) {
            return -1;
        }
    }
    if (shield) {
        ar += shield->prot;
        if (missile && shield->projectile > 0 && chance(shield->projectile)) {
            return -1;
        }
    }

    /* nat�rliche R�stung */
    an = natural_armor(du);

    /* magische R�stung durch Artefakte oder Spr�che */
    /* Momentan nur Trollg�rtel und Werwolf-Eigenschaft */
    am = select_magicarmor(dt);

    if (rule_nat_armor == 0) {
        /* nat�rliche R�stung ist halbkumulativ */
        if (ar > 0) {
            ar += an / 2;
        }
        else {
            ar = an;
        }
    }
    else {
        /* use the higher value, add half the other value */
        ar = (ar > an) ? (ar + an / 2) : (an + ar / 2);
    }

    if (awtype && fval(awtype, WTF_ARMORPIERCING)) {
        /* crossbows */
        ar /= 2;
    }

    ar += am;

    if (magres) {
        /* calculate damage multiplier for magical damage */
        variant res;
        
        res = frac_sub(frac_one, magic_resistance(du));

        if (u_race(du)->battle_flags & BF_EQUIPMENT) {
            /* der Effekt von Laen steigt nicht linear */
            if (armor && fval(armor, ATF_LAEN)) {
                res = frac_mul(res, frac_sub(frac_one, armor->magres));
            }
            if (shield && fval(shield, ATF_LAEN)) {
                res = frac_mul(res, frac_sub(frac_one, shield->magres));
            }
            if (dwtype) {
                res = frac_mul(res, frac_sub(frac_one, dwtype->magres));
            }
        }

        /* gegen Magie wirkt nur natuerliche und magische Ruestung */
        ar = an + am;
        if (res.sa[0] >= 0) {
            *magres = res;
        }
        else {
            *magres = frac_make(0, 1);
        }
    }

    return ar;
}

bool
terminate(troop dt, troop at, int type, const char *damage, bool missile)
{
    item **pitm;
    fighter *df = dt.fighter;
    fighter *af = at.fighter;
    unit *au = af->unit;
    unit *du = df->unit;
    battle *b = df->side->battle;

    /* Schild */
    side *ds = df->side;
    int hp, ar;

    const weapon_type *dwtype = NULL;
    const weapon_type *awtype = NULL;
    const weapon *weapon;
    variant res = frac_make(1, 1);

    int rda, sk = 0, sd;
    bool magic = false;
    int da = dice_rand(damage);

    assert(du->number > 0);
    ++at.fighter->hits;

    switch (type) {
    case AT_STANDARD:
        weapon = select_weapon(at, true, missile);
        sk = weapon_effskill(at, dt, weapon, true, missile);
        if (weapon)
            awtype = weapon->type;
        if (awtype && fval(awtype, WTF_MAGICAL))
            magic = true;
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
    weapon = select_weapon(dt, false, true);      /* missile=true to get the unmodified best weapon she has */
    sd = weapon_effskill(dt, at, weapon, false, false);
    if (weapon != NULL)
        dwtype = weapon->type;

    if (is_riding(at) && (awtype == NULL || (fval(awtype, WTF_HORSEBONUS)
        && !fval(awtype, WTF_MISSILE)))) {
        da += CavalryBonus(au, dt, BONUS_DAMAGE);
    }

    ar = calculate_armor(dt, dwtype, awtype, magic ? &res : 0);
    if (ar < 0) {
        return false;
    }

    if (magic) {
        res = frac_mul(frac_make(da, 1), res);
        da = res.sa[0] / res.sa[1];
    }

    if (type != AT_COMBATSPELL && type != AT_SPELL) {
        if (rule_damage & DAMAGE_CRITICAL) {
            double kritchance = (sk * 3 - sd) / 200.0;
            int maxk = 4;

            kritchance = MAX(kritchance, 0.005);
            kritchance = MIN(0.9, kritchance);

            while (maxk-- && chance(kritchance)) {
                da += dice_rand(damage);
            }
        }

        da += rc_specialdamage(au, du, awtype);

        if (awtype != NULL && fval(awtype, WTF_MISSILE)) {
            /* missile weapon bonus */
            if (rule_damage & DAMAGE_MISSILE_BONUS) {
                da += af->person[at.index].damage_rear;
            }
        }
        else {
            /* melee bonus */
            if (rule_damage & DAMAGE_MELEE_BONUS) {
                da += af->person[at.index].damage;
            }
        }

        /* Skilldifferenzbonus */
        if (rule_damage & DAMAGE_SKILL_BONUS) {
            da += MAX(0, (sk - sd) / DAMAGE_QUOTIENT);
        }
    }

    rda = MAX(da - ar, 0);

    if ((u_race(du)->battle_flags & BF_INV_NONMAGIC) && !magic)
        rda = 0;
    else {
        int qi;
        selist *ql;
        unsigned int i = 0;

        if (u_race(du)->battle_flags & BF_RES_PIERCE)
            i |= WTF_PIERCE;
        if (u_race(du)->battle_flags & BF_RES_CUT)
            i |= WTF_CUT;
        if (u_race(du)->battle_flags & BF_RES_BASH)
            i |= WTF_BLUNT;

        if (i && awtype && fval(awtype, i))
            rda /= 2;

        /* Schilde */
        for (qi = 0, ql = b->meffects; ql; selist_advance(&ql, &qi, 1)) {
            meffect *me = (meffect *)selist_get(ql, qi);
            if (meffect_protection(b, me, ds) != 0) {
                assert(0 <= rda);       /* rda sollte hier immer mindestens 0 sein */
                /* jeder Schaden wird um effect% reduziert bis der Schild duration
                 * Trefferpunkte aufgefangen hat */
                if (me->typ == SHIELD_REDUCE) {
                    hp = rda * (me->effect / 100);
                    rda -= hp;
                    me->duration -= hp;
                }
                /* gibt R�stung +effect f�r duration Treffer */
                if (me->typ == SHIELD_ARMOR) {
                    rda = MAX(rda - me->effect, 0);
                    me->duration--;
                }
            }
        }
    }

    assert(dt.index >= 0 && dt.index < du->number);
    if (rda>0) {
        df->person[dt.index].hp -= rda;
        if (u_race(au) == get_race(RC_DAEMON)) {
            vampirism(at, rda);
        }
        if (b->turn>1) {
            /* someone on the ship got damaged, damage the ship */
            ship *sh = du->ship ? du->ship : leftship(du);
            if (sh)
                fset(sh, SF_DAMAGED);
        }

    }
    if (df->person[dt.index].hp > 0) {    /* Hat �berlebt */
        if (u_race(au) == get_race(RC_DAEMON)) {
            if (!(df->person[dt.index].flags & (FL_COURAGE | FL_DAZZLED))) {
                df->person[dt.index].flags |= FL_DAZZLED;
                df->person[dt.index].defence--;
            }
        }
        return false;
    }

    /* Sieben Leben */
    if (u_race(du) == get_race(RC_CAT) && (chance(1.0 / 7))) {
        df->person[dt.index].hp = unit_max_hp(du);
        return false;
    }

    if (oldpotiontype[P_HEAL] && !fval(&df->person[dt.index], FL_HEALING_USED)) {
        if (i_get(du->items, oldpotiontype[P_HEAL]->itype) > 0) {
            message *m = msg_message("battle::potionsave", "unit", du);
            message_faction(b, du->faction, m);
            msg_release(m);
            i_change(&du->items, oldpotiontype[P_HEAL]->itype, -1);
            fset(&df->person[dt.index], FL_HEALING_USED);
            df->person[dt.index].hp = u_race(du)->hitpoints * 5; /* give the person a buffer */
            return false;
        }
    }
    ++at.fighter->kills;

    for (pitm = &du->items; *pitm;) {
        item *itm = *pitm;
        const item_type *itype = itm->type;
        if (!(itype->flags & ITF_CURSED) && dt.index < itm->number) {
            /* 25% Grundchance, das ein Item kaputtgeht. */
            if (rng_int() % 4 < 1) {
                i_change(pitm, itype, -1);
            }
        }
        if (*pitm == itm) {
            pitm = &itm->next;
        }
    }
    kill_troop(dt);

    return true;
}

static int
count_side(const side * s, const side * vs, int minrow, int maxrow, int select)
{
    fighter *fig;
    int people = 0;
    int unitrow[NUMROWS];

    if (maxrow < FIGHT_ROW)
        return 0;
    if (select & SELECT_ADVANCE) {
        memset(unitrow, -1, sizeof(unitrow));
    }

    for (fig = s->fighters; fig; fig = fig->next) {
        if (fig->alive - fig->removed > 0) {
            int row = statusrow(fig->status);
            if (select & SELECT_ADVANCE) {
                if (unitrow[row] == -1) {
                    unitrow[row] = get_unitrow(fig, vs);
                }
                row = unitrow[row];
            }
            if (row >= minrow && row <= maxrow) {
                people += fig->alive - fig->removed;
                if (people > 0 && (select & SELECT_FIND))
                    break;
            }
        }
    }
    return people;
}

/* return the number of live allies warning: this function only considers
* troops that are still alive, not those that are still fighting although
* dead. */
int
count_allies(const side * as, int minrow, int maxrow, int select, int allytype)
{
    battle *b = as->battle;
    side *ds;
    int count = 0;

    for (ds = b->sides; ds != b->sides + b->nsides; ++ds) {
        if ((allytype == ALLY_ANY && helping(as, ds)) || (allytype == ALLY_SELF
            && as->faction == ds->faction)) {
            count += count_side(ds, NULL, minrow, maxrow, select);
            if (count > 0 && (select & SELECT_FIND))
                break;
        }
    }
    return count;
}

static int
count_enemies_i(battle * b, const fighter * af, int minrow, int maxrow,
int select)
{
    side *es, *as = af->side;
    int i = 0;

    for (es = b->sides; es != b->sides + b->nsides; ++es) {
        if (as == NULL || enemy(es, as)) {
            int offset = 0;
            if (select & SELECT_DISTANCE) {
                offset = get_unitrow(af, es) - FIGHT_ROW;
            }
            i += count_side(es, as, minrow - offset, maxrow - offset, select);
            if (i > 0 && (select & SELECT_FIND))
                break;
        }
    }
    return i;
}

int
count_enemies(battle * b, const fighter * af, int minrow, int maxrow,
int select)
{
    int sr = statusrow(af->status);
    side *as = af->side;

    if (b->alive == b->fast.alive && as == b->fast.side && sr == b->fast.status
        && minrow == b->fast.minrow && maxrow == b->fast.maxrow) {
        if (b->fast.enemies[select] >= 0) {
            return b->fast.enemies[select];
        }
        else if (select & SELECT_FIND) {
            if (b->fast.enemies[select - SELECT_FIND] >= 0) {
                return b->fast.enemies[select - SELECT_FIND];
            }
        }
    }
    else if (select != SELECT_FIND || b->alive != b->fast.alive) {
        b->fast.side = as;
        b->fast.status = sr;
        b->fast.minrow = minrow;
        b->fast.alive = b->alive;
        b->fast.maxrow = maxrow;
        memset(b->fast.enemies, -1, sizeof(b->fast.enemies));
    }
    if (maxrow >= FIRST_ROW) {
        int i = count_enemies_i(b, af, minrow, maxrow, select);
        b->fast.enemies[select] = i;
        return i;
    }
    return 0;
}

troop select_enemy(fighter * af, int minrow, int maxrow, int select)
{
    side *as = af->side;
    battle *b = as->battle;
    int si, selected;
    int enemies;
#ifdef DEBUG_SELECT
    troop result = no_troop;
#endif
    if (u_race(af->unit)->flags & RCF_FLY) {
        /* flying races ignore min- and maxrow and can attack anyone fighting
         * them */
        minrow = FIGHT_ROW;
        maxrow = BEHIND_ROW;
    }
    minrow = MAX(minrow, FIGHT_ROW);

    enemies = count_enemies(b, af, minrow, maxrow, select);

    /* Niemand ist in der angegebenen Entfernung? */
    if (enemies <= 0)
        return no_troop;

    selected = (int)(rng_int() % enemies);
    for (si = 0; as->enemies[si]; ++si) {
        side *ds = as->enemies[si];
        fighter *df;
        int unitrow[NUMROWS];
        int offset = 0;

        if (select & SELECT_DISTANCE)
            offset = get_unitrow(af, ds) - FIGHT_ROW;

        if (select & SELECT_ADVANCE) {
            int ui;
            for (ui = 0; ui != NUMROWS; ++ui)
                unitrow[ui] = -1;
        }

        for (df = ds->fighters; df; df = df->next) {
            int dr;

            dr = statusrow(df->status);
            if (select & SELECT_ADVANCE) {
                if (unitrow[dr] < 0) {
                    unitrow[dr] = get_unitrow(df, as);
                }
                dr = unitrow[dr];
            }

            if (select & SELECT_DISTANCE)
                dr += offset;
            if (dr < minrow || dr > maxrow)
                continue;
            if (df->alive - df->removed > selected) {
#ifdef DEBUG_SELECT
                if (result.fighter == NULL) {
                    result.index = selected;
                    result.fighter = df;
                }
#else
                troop dt;
                dt.index = selected;
                dt.fighter = df;
                return dt;
#endif
            }
            selected -= (df->alive - df->removed);
            enemies -= (df->alive - df->removed);
        }
    }
    if (enemies != 0) {
        log_error("select_enemies has a bug.\n");
    }
#ifdef DEBUG_SELECT
    return result;
#else
    assert(!selected);
    return no_troop;
#endif
}

static int get_tactics(const side * as, const side * ds)
{
    battle *b = as->battle;
    side *stac;
    int result = 0;
    int defense = 0;

    if (b->max_tactics > 0) {
        for (stac = b->sides; stac != b->sides + b->nsides; ++stac) {
            if (stac->leader.value > result && helping(stac, as)) {
                assert(ds == NULL || !helping(stac, ds));
                result = stac->leader.value;
            }
            if (ds && stac->leader.value > defense && helping(stac, ds)) {
                assert(!helping(stac, as));
                defense = stac->leader.value;
            }
        }
    }
    return result - defense;
}

static troop select_opponent(battle * b, troop at, int mindist, int maxdist)
{
    fighter *af = at.fighter;
    troop dt;

    if (u_race(af->unit)->flags & RCF_FLY) {
        /* flying races ignore min- and maxrow and can attack anyone fighting
         * them */
        dt = select_enemy(at.fighter, FIGHT_ROW, BEHIND_ROW, SELECT_ADVANCE);
    }
    else {
        mindist = MAX(mindist, FIGHT_ROW);
        dt = select_enemy(at.fighter, mindist, maxdist, SELECT_ADVANCE);
    }

    if (b->turn == 0 && dt.fighter) {
        if (rule_tactics_formula == 1) {
            int tactics = get_tactics(at.fighter->side, dt.fighter->side);

            /* percentage chance to get this attack */
            if (tactics > 0) {
                double tacch = 0.1 * tactics;
                if (fval(b->region->terrain, SEA_REGION)) {
                    ship *sh = at.fighter->unit->ship;
                    if (sh)
                        tacch *= sh->type->tac_bonus;
                }
                if (!chance(tacch)) {
                    dt.fighter = NULL;
                }
            }
            else {
                dt.fighter = NULL;
            }
        }
    }

    return dt;
}

selist *select_fighters(battle * b, const side * vs, int mask, select_fun cb, void *cbdata)
{
    side *s;
    selist *fightervp = 0;

    assert(vs != NULL);

    for (s = b->sides; s != b->sides + b->nsides; ++s) {
        fighter *fig;

        if (mask == FS_ENEMY) {
            if (!enemy(s, vs))
                continue;
        }
        else if (mask == FS_HELP) {
            if (enemy(s, vs) || !allysf(s, vs->faction)) {
                continue;
            }
        }
        else {
            assert(mask == (FS_HELP | FS_ENEMY) || !"invalid alliance state");
        }
        for (fig = s->fighters; fig; fig = fig->next) {
            if (cb(vs, fig, cbdata)) {
                selist_push(&fightervp, fig);
            }
        }
    }

    return fightervp;
}

struct selector {
    int minrow;
    int maxrow;
};

static bool select_row(const side *vs, const fighter *fig, void *cbdata)
{
    struct selector *sel = (struct selector *)cbdata;
    int row = get_unitrow(fig, vs);
    return (row >= sel->minrow && row <= sel->maxrow);
}

selist *fighters(battle * b, const side * vs, int minrow, int maxrow, int mask)
{
    struct selector sel;
    sel.maxrow = maxrow;
    sel.minrow = minrow;
    return select_fighters(b, vs, mask, select_row, &sel);
}

static void report_failed_spell(struct battle * b, struct unit * mage, const struct spell *sp)
{
    message *m = msg_message("battle::spell_failed", "unit spell", mage, sp);
    message_all(b, m);
    msg_release(m);
}

static castorder * create_castorder_combat(castorder *co, fighter *fig, const spell * sp, int level, double force) {
    co = create_castorder(co, fig->unit, 0, sp, fig->unit->region, level, force, 0, 0, 0);
    co->magician.fig = fig;
    return co;
}

#ifdef FFL_CURSED
static void summon_igjarjuk(battle *b, spellrank spellranks[]) {
    side *s;
    castorder *co;

    for (s = b->sides; s != b->sides + b->nsides; ++s) {
        fighter *fig = 0;
        if (s->bf->attacker && fval(s->faction, FFL_CURSED)) {
            spell *sp = find_spell("igjarjuk");
            if (sp) {
                int si;
                for (si = 0; s->enemies[si]; ++si) {
                    side *se = s->enemies[si];
                    if (se && !fval(se->faction, FFL_NPC)) {
                        fighter *fi;
                        for (fi = se->fighters; fi; fi = fi->next) {
                            if (fi && (!fig || fig->unit->number > fi->unit->number)) {
                                fig = fi;
                                if (fig->unit->number == 1) {
                                    break;
                                }
                            }
                        }
                        if (fig && fig->unit->number == 1) {
                            break;
                        }
                    }
                }
                if (fig) {
                    co = create_castorder_combat(0, fig, sp, 10, 10);
                    co->magician.fig = fig;
                    add_castorder(&spellranks[sp->rank], co);
                    break;
                }
            }
        }
    }
}
#endif

void do_combatmagic(battle * b, combatmagic_t was)
{
    side *s;
    castorder *co;
    region *r = b->region;
    int level, rank, sl;
    spellrank spellranks[MAX_SPELLRANK];

    memset(spellranks, 0, sizeof(spellranks));

#ifdef FFL_CURSED
    if (was == DO_PRECOMBATSPELL) {
        summon_igjarjuk(b, spellranks);
    }
#endif
    for (s = b->sides; s != b->sides + b->nsides; ++s) {
        fighter *fig;
        for (fig = s->fighters; fig; fig = fig->next) {
            unit *mage = fig->unit;

            if (fig->alive <= 0)
                continue;               /* fighter kann im Kampf get�tet worden sein */

            level = effskill(mage, SK_MAGIC, r);
            if (level > 0) {
                double power;
                const spell *sp;
                const struct locale *lang = mage->faction->locale;
                order *ord;

                switch (was) {
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

                ord = create_order(K_CAST, lang, "'%s'", spell_name(sp, lang));
                if (!cancast(mage, sp, 1, 1, ord)) {
                    free_order(ord);
                    continue;
                }

                level = eff_spelllevel(mage, sp, level, 1);
                if (sl > 0)
                    level = MIN(sl, level);
                if (level < 0) {
                    report_failed_spell(b, mage, sp);
                    free_order(ord);
                    continue;
                }

                power = spellpower(r, mage, sp, level, ord);
                free_order(ord);
                if (power <= 0) {       /* Effekt von Antimagie */
                    report_failed_spell(b, mage, sp);
                    pay_spell(mage, sp, level, 1);
                }
                else if (fumble(r, mage, sp, level)) {
                    report_failed_spell(b, mage, sp);
                    pay_spell(mage, sp, level, 1);
                }
                else {
                    co = create_castorder_combat(0, fig, sp, level, power);
                    add_castorder(&spellranks[sp->rank], co);
                }
            }
        }
    }
    for (rank = 0; rank < MAX_SPELLRANK; rank++) {
        for (co = spellranks[rank].begin; co; co = co->next) {
            fighter *fig = co->magician.fig;
            const spell *sp = co->sp;

            level = cast_spell(co);
            if (level > 0) {
                pay_spell(fig->unit, sp, level, 1);
            }
        }
    }
    for (rank = 0; rank < MAX_SPELLRANK; rank++) {
        free_castorders(spellranks[rank].begin);
    }
}

static int cast_combatspell(troop at, const spell * sp, int level, double force)
{
    castorder co;

    create_castorder_combat(&co, at.fighter, sp, level, force);
    level = cast_spell(&co);
    free_castorder(&co);
    if (level > 0) {
        pay_spell(at.fighter->unit, sp, level, 1);
    }
    return level;
}

static void do_combatspell(troop at)
{
    const spell *sp;
    fighter *fi = at.fighter;
    unit *caster = fi->unit;
    battle *b = fi->side->battle;
    region *r = b->region;
    selist *ql;
    int level, qi;
    double power;
    int fumblechance = 0;
    order *ord;
    int sl;
    const struct locale *lang = caster->faction->locale;

    sp = get_combatspell(caster, 1);
    if (sp == NULL) {
        fi->magic = 0;              /* Hat keinen Kampfzauber, k�mpft nichtmagisch weiter */
        return;
    }
    ord = create_order(K_CAST, lang, "'%s'", spell_name(sp, lang));
    if (!cancast(caster, sp, 1, 1, ord)) {
        fi->magic = 0;              /* Kann nicht mehr Zaubern, k�mpft nichtmagisch weiter */
        return;
    }

    level = eff_spelllevel(caster, sp, fi->magic, 1);
    if ((sl = get_combatspelllevel(caster, 1)) > 0)
        level = MIN(level, sl);

    if (fumble(r, caster, sp, level)) {
        report_failed_spell(b, caster, sp);
        pay_spell(caster, sp, level, 1);
        return;
    }

    for (qi = 0, ql = b->meffects; ql; selist_advance(&ql, &qi, 1)) {
        meffect *mblock = (meffect *)selist_get(ql, qi);
        if (mblock->typ == SHIELD_BLOCK) {
            if (meffect_blocked(b, mblock, fi->side) != 0) {
                fumblechance += mblock->duration;
                mblock->duration -= mblock->effect;
            }
        }
    }

    /* Antimagie die Fehlschlag erh�ht */
    if (rng_int() % 100 < fumblechance) {
        report_failed_spell(b, caster, sp);
        pay_spell(caster, sp, level, 1);
        free_order(ord);
        return;
    }
    power = spellpower(r, caster, sp, level, ord);
    free_order(ord);
    if (power <= 0) {             /* Effekt von Antimagie */
        report_failed_spell(b, caster, sp);
        pay_spell(caster, sp, level, 1);
        return;
    }

    level = cast_combatspell(at, sp, level, power);
}

/* Sonderattacken: Monster patzern nicht und zahlen auch keine
 * Spruchkosten. Da die Spruchst�rke direkt durch den Level bestimmt
 * wird, wirkt auch keine Antimagie (wird sonst in spellpower
 * gemacht) */

static void do_extra_spell(troop at, const att * a)
{
    const spell *sp = spellref_get(a->data.sp);

    if (!sp) {
        log_error("no such spell: '%s'", a->data.sp->name);
    }
    else {
        assert(a->level > 0);
        cast_combatspell(at, sp, a->level, a->level);
    }
}

int skilldiff(troop at, troop dt, int dist)
{
    fighter *af = at.fighter, *df = dt.fighter;
    unit *au = af->unit, *du = df->unit;
    int is_protected = 0, skdiff = 0;
    weapon *awp = select_weapon(at, true, dist > 1);
    static int rc_cache;
    static const race *rc_halfling, *rc_goblin;

    if (rc_changed(&rc_cache)) {
        rc_halfling = get_race(RC_HALFLING);
        rc_goblin = get_race(RC_GOBLIN);
    }
    skdiff += af->person[at.index].attack;
    skdiff -= df->person[dt.index].defence;

    if (df->person[dt.index].flags & FL_SLEEPING)
        skdiff += 2;

    /* Effekte durch Rassen */
    if (awp != NULL && u_race(au) == rc_halfling && dragonrace(u_race(du))) {
        skdiff += 5;
    }
    else if (u_race(au) == rc_goblin) {
        if (af->side->size[SUM_ROW] >= df->side->size[SUM_ROW] * rule_goblin_bonus) {
            skdiff += 1;
        }
    }

    if (df->building) {
        building *b = df->building;
        if (b->attribs) {
            const curse_type *strongwall_ct = ct_find("strongwall");
            if (strongwall_ct) {
                curse *c = get_curse(b->attribs, strongwall_ct);
                if (curse_active(c)) {
                    /* wirkt auf alle Geb�ude */
                    skdiff -= curse_geteffect_int(c);
                    is_protected = 2;
                }
            }
        }
        if (b->type->flags & BTF_FORTIFICATION) {
            int stage = buildingeffsize(b, false);
            int beff = building_protection(b->type, stage);
            if (beff > 0) {
                skdiff -= beff;
                is_protected = 2;
                if (b->attribs) {
                    if (curse_active(get_curse(b->attribs, &ct_magicwalls))) {
                        /* Verdoppelt Burgenbonus */
                        skdiff -= beff;
                    }
                }
            }
        }
    }
    /* Effekte der Waffen */
    skdiff += weapon_effskill(at, dt, awp, true, dist > 1);
    if (awp && fval(awp->type, WTF_MISSILE)) {
        skdiff -= is_protected;
        if (awp->type->modifiers) {
            int w;
            for (w = 0; awp->type->modifiers[w].value != 0; ++w) {
                if (awp->type->modifiers[w].flags & WMF_MISSILE_TARGET) {
                    /* skill decreases by targeting difficulty (bow -2, catapult -4) */
                    skdiff -= awp->type->modifiers[w].value;
                    break;
                }
            }
        }
    }
    if (skill_formula == FORMULA_ORIG) {
        weapon *dwp = select_weapon(dt, false, dist > 1);
        skdiff -= weapon_effskill(dt, at, dwp, false, dist > 1);
    }
    return skdiff;
}

static int setreload(troop at)
{
    fighter *af = at.fighter;
    const weapon_type *wtype = af->person[at.index].missile->type;
    if (wtype->reload == 0)
        return 0;
    return af->person[at.index].reload = wtype->reload;
}

int getreload(troop at)
{
    return at.fighter->person[at.index].reload;
}

int hits(troop at, troop dt, weapon * awp)
{
    fighter *af = at.fighter, *df = dt.fighter;
    const armor_type *armor, *shield = 0;
    int skdiff = 0;
    int dist = get_unitrow(af, df->side) + get_unitrow(df, af->side) - 1;
    weapon *dwp = select_weapon(dt, false, dist > 1);

    if (!df->alive)
        return 0;
    if (getreload(at))
        return 0;
    if (dist > 1 && (awp == NULL || !fval(awp->type, WTF_MISSILE)))
        return 0;

    /* mark this person as hit. */
    df->person[dt.index].flags |= FL_HIT;

    if (af->person[at.index].flags & FL_STUNNED) {
        af->person[at.index].flags &= ~FL_STUNNED;
        return 0;
    }
    if ((af->person[at.index].flags & FL_TIRED && rng_int() % 100 < 50)
        || (af->person[at.index].flags & FL_SLEEPING))
        return 0;

    /* effect of sp_reeling_arrows combatspell */
    if (af->side->battle->reelarrow && awp && fval(awp->type, WTF_MISSILE)
        && rng_double() < 0.5) {
        return 0;
    }

    skdiff = skilldiff(at, dt, dist);
    /* Verteidiger bekommt eine R�stung */
    armor = select_armor(dt, true);
    if (dwp == NULL || (dwp->type->flags & WTF_USESHIELD)) {
        shield = select_armor(dt, false);
    }
    if (contest(skdiff, dt, armor, shield)) {
        return 1;
    }
    return 0;
}

void dazzle(battle * b, troop * td)
{
    UNUSED_ARG(b);
    /* Nicht kumulativ ! */
#ifdef TODO_RUNESWORD
    if (td->fighter->weapon[WP_RUNESWORD].count > td->index) {
        return;
    }
#endif
    if (td->fighter->person[td->index].flags & (FL_COURAGE|FL_DAZZLED)) {
        return;
    }

    td->fighter->person[td->index].flags |= FL_DAZZLED;
    td->fighter->person[td->index].defence--;
}

void damage_building(battle * b, building * bldg, int damage_abs)
{
    bldg->size = MAX(1, bldg->size - damage_abs);

    /* Wenn Burg, dann gucken, ob die Leute alle noch in das Geb�ude passen. */

    if (bldg->type->flags & BTF_FORTIFICATION) {
        side *s;

        bldg->sizeleft = bldg->size;

        for (s = b->sides; s != b->sides + b->nsides; ++s) {
            fighter *fig;
            for (fig = s->fighters; fig; fig = fig->next) {
                if (fig->building == bldg) {
                    if (bldg->sizeleft >= fig->unit->number) {
                        fig->building = bldg;
                        bldg->sizeleft -= fig->unit->number;
                    }
                    else {
                        fig->building = NULL;
                    }
                }
            }
        }
    }
}

static int attacks_per_round(troop t)
{
    return t.fighter->person[t.index].speed;
}

static void make_heroes(battle * b)
{
    side *s;
    for (s = b->sides; s != b->sides + b->nsides; ++s) {
        fighter *fig;
        for (fig = s->fighters; fig; fig = fig->next) {
            unit *u = fig->unit;
            if (fval(u, UFL_HERO)) {
                int i;
                if (!playerrace(u_race(u))) {
                    log_error("Hero %s is a %s.\n", unitname(u), u_race(u)->_name);
                }
                for (i = 0; i != u->number; ++i) {
                    fig->person[i].speed += (rule_hero_speed - 1);
                }
            }
        }
    }
}

static void attack(battle * b, troop ta, const att * a, int numattack)
{
    fighter *af = ta.fighter;
    troop td;
    unit *au = af->unit;

    switch (a->type) {
    case AT_COMBATSPELL:
        /* Magier versuchen immer erstmal zu zaubern, erst wenn das
         * fehlschl�gt, wird af->magic == 0 und  der Magier k�mpft
         * konventionell weiter */
        if (numattack == 0 && af->magic > 0) {
            /* wenn der magier in die potenzielle Reichweite von Attacken des
             * Feindes kommt, beginnt er auch bei einem Status von KAEMPFE NICHT,
             * Kampfzauber zu schleudern: */
            if (count_enemies(b, af, melee_range[0], missile_range[1],
                SELECT_ADVANCE | SELECT_DISTANCE | SELECT_FIND)) {
                do_combatspell(ta);
            }
        }
        break;
    case AT_STANDARD:          /* Waffen, mag. Gegenst�nde, Kampfzauber */
        if (numattack > 0 || af->magic <= 0) {
            weapon *wp = ta.fighter->person[ta.index].missile;
            int melee =
                count_enemies(b, af, melee_range[0], melee_range[1],
                SELECT_ADVANCE | SELECT_DISTANCE | SELECT_FIND);
            if (melee)
                wp = preferred_weapon(ta, true);
            /* Sonderbehandlungen */

            if (getreload(ta)) {
                ta.fighter->person[ta.index].reload--;
            }
            else {
                bool standard_attack = true;
                bool reload = false;
                /* spezialattacken der waffe nur, wenn erste attacke in der runde.
                 * sonst helden mit feuerschwertern zu m�chtig */
                if (numattack == 0 && wp && wp->type->attack) {
                    int dead = 0;
                    standard_attack = wp->type->attack(&ta, wp->type, &dead);
                    if (!standard_attack)
                        reload = true;
                    af->catmsg += dead;
                    if (!standard_attack && af->person[ta.index].last_action < b->turn) {
                        af->person[ta.index].last_action = b->turn;
                    }
                }
                if (standard_attack) {
                    bool missile = false;
                    if (wp && fval(wp->type, WTF_MISSILE))
                        missile = true;
                    if (missile) {
                        td = select_opponent(b, ta, missile_range[0], missile_range[1]);
                    }
                    else {
                        td = select_opponent(b, ta, melee_range[0], melee_range[1]);
                    }
                    if (!td.fighter)
                        return;
                    if (ta.fighter->person[ta.index].last_action < b->turn) {
                        ta.fighter->person[ta.index].last_action = b->turn;
                    }
                    reload = true;
                    if (hits(ta, td, wp)) {
                        const char *d;
                        if (wp == NULL)
                            d = u_race(au)->def_damage;
                        else if (is_riding(ta))
                            d = wp->type->damage[1];
                        else
                            d = wp->type->damage[0];
                        terminate(td, ta, a->type, d, missile);
                    }
                }
                if (reload && wp && wp->type->reload && !getreload(ta)) {
                    setreload(ta);
                }
            }
        }
        break;
    case AT_SPELL:             /* Extra-Spr�che. Kampfzauber in AT_COMBATSPELL! */
        do_extra_spell(ta, a);
        break;
    case AT_NATURAL:
        td = select_opponent(b, ta, melee_range[0], melee_range[1]);
        if (!td.fighter)
            return;
        if (ta.fighter->person[ta.index].last_action < b->turn) {
            ta.fighter->person[ta.index].last_action = b->turn;
        }
        if (hits(ta, td, NULL)) {
            terminate(td, ta, a->type, a->data.dice, false);
        }
        break;
    case AT_DRAIN_ST:
        td = select_opponent(b, ta, melee_range[0], melee_range[1]);
        if (!td.fighter)
            return;
        if (ta.fighter->person[ta.index].last_action < b->turn) {
            ta.fighter->person[ta.index].last_action = b->turn;
        }
        if (hits(ta, td, NULL)) {
            int c = dice_rand(a->data.dice);
            while (c > 0) {
                if (rng_int() % 2) {
                    td.fighter->person[td.index].attack -= 1;
                }
                else {
                    td.fighter->person[td.index].defence -= 1;
                }
                c--;
            }
        }
        break;
    case AT_DRAIN_EXP:
        td = select_opponent(b, ta, melee_range[0], melee_range[1]);
        if (!td.fighter)
            return;
        if (ta.fighter->person[ta.index].last_action < b->turn) {
            ta.fighter->person[ta.index].last_action = b->turn;
        }
        if (hits(ta, td, NULL)) {
            drain_exp(td.fighter->unit, dice_rand(a->data.dice));
        }
        break;
    case AT_DAZZLE:
        td = select_opponent(b, ta, melee_range[0], melee_range[1]);
        if (!td.fighter)
            return;
        if (ta.fighter->person[ta.index].last_action < b->turn) {
            ta.fighter->person[ta.index].last_action = b->turn;
        }
        if (hits(ta, td, NULL)) {
            dazzle(b, &td);
        }
        break;
    case AT_STRUCTURAL:
        td = select_opponent(b, ta, melee_range[0], melee_range[1]);
        if (!td.fighter)
            return;
        if (ta.fighter->person[ta.index].last_action < b->turn) {
            ta.fighter->person[ta.index].last_action = b->turn;
        }
        if (td.fighter->unit->ship) {
            int dice = dice_rand(a->data.dice);
            ship * sh = td.fighter->unit->ship;
            damage_ship(sh, dice / sh->type->damage / sh->size);
        }
        else if (td.fighter->unit->building) {
            damage_building(b, td.fighter->unit->building, dice_rand(a->data.dice));
        }
    }
}

void do_attack(fighter * af)
{
    troop ta;
    unit *au = af->unit;
    side *side = af->side;
    battle *b = side->battle;

    ta.fighter = af;

    assert(au && au->number);
    /* Da das Zuschlagen auf Einheiten und nicht auf den einzelnen
     * K�mpfern beruht, darf die Reihenfolge und Gr��e der Einheit keine
     * Rolle spielen, Das tut sie nur dann, wenn jeder, der am Anfang der
     * Runde lebte, auch zuschlagen darf. Ansonsten ist der, der zuf�llig
     * mit einer gro�en Einheit zuerst drankommt, extrem bevorteilt. */
    ta.index = af->fighting;

    while (ta.index--) {
        /* Wir suchen eine beliebige Feind-Einheit aus. An der k�nnen
         * wir feststellen, ob noch jemand da ist. */
        int apr, attacks = attacks_per_round(ta);
        if (!count_enemies(b, af, FIGHT_ROW, LAST_ROW, SELECT_FIND))
            break;

        for (apr = 0; apr != attacks; ++apr) {
            int a;
            for (a = 0; a < RACE_ATTACKS && u_race(au)->attack[a].type != AT_NONE; ++a) {
                if (apr > 0) {
                    /* Wenn die Waffe nachladen muss, oder es sich nicht um einen
                     * Waffen-Angriff handelt, dann gilt der Speed nicht. */
                    /* TODO: allow multiple AT_NATURAL attacks? */
                    if (u_race(au)->attack[a].type != AT_STANDARD) 
                        continue;
                    else {
                        weapon *wp = preferred_weapon(ta, true);
                        if (wp != NULL && wp->type->reload)
                            continue;
                    }
                }
                attack(b, ta, &(u_race(au)->attack[a]), apr);
            }
        }
    }
    /* Der letzte Katapultsch�tze setzt die
     * Ladezeit neu und generiert die Meldung. */
    if (af->catmsg >= 0) {
        struct message *m =
            msg_message("battle::killed", "unit dead", au, af->catmsg);
        message_all(b, m);
        msg_release(m);
        af->catmsg = -1;
    }
}

void do_regenerate(fighter * af)
{
    troop ta;
    unit *au = af->unit;

    ta.fighter = af;
    ta.index = af->fighting;

    while (ta.index--) {
        struct person *p = af->person + ta.index;
        p->hp += effskill(au, SK_STAMINA, 0);
        p->hp = MIN(unit_max_hp(au), p->hp);
    }
}

static void add_tactics(tactics * ta, fighter * fig, int value)
{
    if (value == 0 || value < ta->value)
        return;
    if (value > ta->value) {
        selist_free(ta->fighters);
        ta->fighters = 0;
    }
    selist_push(&ta->fighters, fig);
    selist_push(&fig->side->battle->leaders, fig);
    ta->value = value;
}

static double horse_fleeing_bonus(const unit * u)
{
    const item_type *it_horse, *it_elvenhorse, *it_charger;
    int n1 = 0, n2 = 0, n3 = 0;
    item *itm;
    int skl = effskill(u, SK_RIDING, 0);
    const resource_type *rtype;

    it_horse = ((rtype = get_resourcetype(R_HORSE)) != NULL) ? rtype->itype : 0;
    it_elvenhorse = ((rtype = get_resourcetype(R_UNICORN)) != NULL) ? rtype->itype : 0;
    it_charger = ((rtype = get_resourcetype(R_CHARGER)) != NULL) ? rtype->itype : 0;

    for (itm = u->items; itm; itm = itm->next) {
        if (itm->type->flags & ITF_ANIMAL) {
            if (itm->type == it_elvenhorse)
                n3 += itm->number;
            else if (itm->type == it_charger)
                n2 += itm->number;
            else if (itm->type == it_horse)
                n1 += itm->number;
        }
    }
    if (skl >= 5 && n3 >= u->number)
        return 0.30;
    if (skl >= 2 && n2 + n3 >= u->number)
        return 0.20;
    if (n1 + n2 + n3 >= u->number)
        return 0.10;
    return 0.0F;
}

double fleechance(unit * u)
{
    double c = 0.20;              /* Fluchtwahrscheinlichkeit in % */
    attrib *a = a_find(u->attribs, &at_fleechance);
    /* Einheit u versucht, dem Get�mmel zu entkommen */

    c += (effskill(u, SK_STEALTH, 0) * 0.05);
    c += horse_fleeing_bonus(u);

    if (u_race(u) == get_race(RC_HALFLING)) {
        c += 0.20;
        c = MIN(c, 0.90);
    }
    else {
        c = MIN(c, 0.75);
    }

    if (a != NULL)
        c += a->data.flt;

    return c;
}

/** add a new army to the conflict
 * beware: armies need to be added _at the beginning_ of the list because
 * otherwise join_allies() will get into trouble */
side *make_side(battle * b, const faction * f, const group * g,
    unsigned int flags, const faction * stealthfaction)
{
    side *s1 = b->sides + b->nsides;
    bfaction *bf;

    if (fval(b->region->terrain, SEA_REGION)) {
        /* every fight in an ocean is short */
        flags |= SIDE_HASGUARDS;
    }
    else {
        unit *u;
        for (u = b->region->units; u; u = u->next) {
            if (is_guard(u)) {
                if (alliedunit(u, f, HELP_GUARD)) {
                    flags |= SIDE_HASGUARDS;
                    break;
                }
            }
        }
    }

    s1->battle = b;
    s1->group = g;
    s1->flags = flags;
    s1->stealthfaction = stealthfaction;
    for (bf = b->factions; bf; bf = bf->next) {
        faction *f2 = bf->faction;

        if (f2 == f) {
            s1->bf = bf;
            s1->faction = f2;
            s1->index = b->nsides++;
            s1->nextF = bf->sides;
            bf->sides = s1;
            assert(b->nsides <= MAXSIDES);
            break;
        }
    }
    assert(bf);
    return s1;
}

troop select_ally(fighter * af, int minrow, int maxrow, int allytype)
{
    side *as = af->side;
    battle *b = as->battle;
    side *ds;
    int allies = count_allies(as, minrow, maxrow, SELECT_ADVANCE, allytype);

    if (!allies) {
        return no_troop;
    }
    allies = (int)(rng_int() % allies);

    for (ds = b->sides; ds != b->sides + b->nsides; ++ds) {
        if ((allytype == ALLY_ANY && helping(as, ds)) || (allytype == ALLY_SELF
            && as->faction == ds->faction)) {
            fighter *df;
            for (df = ds->fighters; df; df = df->next) {
                int dr = get_unitrow(df, NULL);
                if (dr >= minrow && dr <= maxrow) {
                    if (df->alive - df->removed > allies) {
                        troop dt;
                        assert(allies >= 0);
                        dt.index = allies;
                        dt.fighter = df;
                        return dt;
                    }
                    allies -= df->alive;
                }
            }
        }
    }
    assert(!"we should never have gotten here");
    return no_troop;
}

static int loot_quota(const unit * src, const unit * dst,
    const item_type * type, int n)
{
    UNUSED_ARG(type);
    if (dst && src && src->faction != dst->faction) {
        double divisor = config_get_flt("rules.items.loot_divisor", 1);
        assert(divisor <= 0 || divisor >= 1);
        if (divisor >= 1) {
            double r = n / divisor;
            int x = (int)r;

            r = r - x;
            if (chance(r))
                ++x;

            return x;
        }
    }
    return n;
}

static void loot_items(fighter * corpse)
{
    unit *u = corpse->unit;
    item *itm = u->items;
    battle *b = corpse->side->battle;
    int dead = dead_fighters(corpse);

    if (dead <= 0)
        return;

    while (itm) {
        float lootfactor = (float)dead / (float)u->number; /* only loot the dead! */
        int maxloot = (int)((float)itm->number * lootfactor);
        if (maxloot > 0) {
            int i = MIN(10, maxloot);
            for (; i != 0; --i) {
                int loot = maxloot / i;

                if (loot > 0) {
                    fighter *fig = NULL;
                    int looting = 0;
                    int maxrow = 0;
                    /* mustloot: we absolutely, positively must have somebody loot this thing */
                    int mustloot = itm->type->flags & (ITF_CURSED | ITF_NOTLOST);

                    itm->number -= loot;
                    maxloot -= loot;

                    if (is_monsters(u->faction) && (rule_loot & LOOT_MONSTERS)) {
                        looting = 1;
                    }
                    else if (rule_loot & LOOT_OTHERS) {
                        looting = 1;
                    }
                    else if (rule_loot & LOOT_SELF) {
                        looting = 2;
                    }
                    if (looting) {
                        if (mustloot) {
                            maxrow = LAST_ROW;
                        }
                        else if (rule_loot & LOOT_KEEPLOOT) {
                            int lootchance = 50 + b->keeploot;
                            if (rng_int() % 100 < lootchance) {
                                maxrow = BEHIND_ROW;
                            }
                        }
                        else {
                            maxrow = LAST_ROW;
                        }
                    }
                    if (maxrow > 0) {
                        if (looting == 1) {
                            /* enemies get dibs */
                            fig = select_enemy(corpse, FIGHT_ROW, maxrow, 0).fighter;
                        }
                        if (!fig) {
                            /* self and allies get second pick */
                            fig = select_ally(corpse, FIGHT_ROW, LAST_ROW, ALLY_SELF).fighter;
                        }
                    }

                    if (fig) {
                        int trueloot =
                            mustloot ? loot : loot_quota(corpse->unit, fig->unit, itm->type,
                            loot);
                        if (trueloot > 0) {
                            i_change(&fig->loot, itm->type, trueloot);
                        }
                    }
                }
            }
        }
        itm = itm->next;
    }
}

static bool seematrix(const faction * f, const side * s)
{
    if (f == s->faction)
        return true;
    if (s->flags & SIDE_STEALTH)
        return false;
    return true;
}

static double PopulationDamage(void)
{
    return rule_population_damage / 100.0;
}

static void battle_effects(battle * b, int dead_players)
{
    region *r = b->region;
    int dead_peasants =
        MIN(rpeasants(r), (int)(dead_players * PopulationDamage()));
    if (dead_peasants) {
        deathcounts(r, dead_peasants + dead_players);
        add_chaoscount(r, dead_peasants / 2);
        rsetpeasants(r, rpeasants(r) - dead_peasants);
    }
}

static void reorder_fleeing(region * r)
{
    unit **usrc = &r->units;
    unit **udst = &r->units;
    unit *ufirst = NULL;
    unit *u;

    for (; *udst; udst = &u->next) {
        u = *udst;
    }

    for (u = *usrc; u != ufirst; u = *usrc) {
        if (u->next && fval(u, UFL_FLEEING)) {
            *usrc = u->next;
            *udst = u;
            udst = &u->next;
            if (!ufirst)
                ufirst = u;
        }
        else {
            usrc = &u->next;
        }
    }
    *udst = NULL;
}

static void aftermath(battle * b)
{
    region *r = b->region;
    side *s;
    int dead_players = 0;
    bfaction *bf;
    bool ships_damaged = (bool)(b->turn + (b->has_tactics_turn ? 1 : 0) > 2);      /* only used for ship damage! */

    for (s = b->sides; s != b->sides + b->nsides; ++s) {
        fighter *df;
        s->dead = 0;

        for (df = s->fighters; df; df = df->next) {
            unit *du = df->unit;
            int dead = dead_fighters(df);

            /* tote insgesamt: */
            s->dead += dead;
            /* Tote, die wiederbelebt werde koennen: */
            if (playerrace(u_race(df->unit))) {
                s->casualties += dead;
            }
            if (df->hits + df->kills) {
                struct message *m =
                    msg_message("killsandhits", "unit hits kills", du, df->hits,
                    df->kills);
                message_faction(b, du->faction, m);
                msg_release(m);
            }
        }
    }

    /* POSTCOMBAT */
    do_combatmagic(b, DO_POSTCOMBATSPELL);

    for (s = b->sides; s != b->sides + b->nsides; ++s) {
        int snumber = 0;
        fighter *df;
        bool relevant = false;   /* Kampf relevant f�r diese Partei? */
        if (!fval(s, SIDE_HASGUARDS)) {
            relevant = true;
        }
        s->flee = 0;

        for (df = s->fighters; df; df = df->next) {
            unit *du = df->unit;
            int dead = dead_fighters(df);
            int sum_hp = 0;
            int n;
            int flags = 0;

            for (n = 0; n != df->alive; ++n) {
                if (df->person[n].hp > 0) {
                    sum_hp += df->person[n].hp;
                }
            }
            snumber += du->number;
            if (relevant) {
                flags = UFL_LONGACTION | UFL_NOTMOVING;
                if (du->status == ST_FLEE) {
                    flags -= UFL_NOTMOVING;
                }
            }
            if (df->alive == 0) {
                flags |= UFL_DEAD;
            }
            if (flags) {
                fset(du, flags);
            }
            if (df->alive && df->alive == du->number) {
                du->hp = sum_hp;
                continue;               /* nichts passiert */
            }
            else if (df->run.hp) {
                if (df->alive == 0) {
                    /* Report the casualties */
                    reportcasualties(b, df, dead);

                    /* Zuerst d�rfen die Feinde pl�ndern, die mitgenommenen Items
                     * stehen in fig->run.items. Dann werden die Fliehenden auf
                     * die leere (tote) alte Einheit gemapt */
                    if (!fval(df, FIG_NOLOOT)) {
                        loot_items(df);
                    }
                    scale_number(du, df->run.number);
                    du->hp = df->run.hp;
                    setguard(du, false);
                    /* must leave ships or buildings, or a stealthy hobbit
                     * can hold castles indefinitely */
                    if (!fval(r->terrain, SEA_REGION)) {
                        leave(du, true);    /* even region owners have to flee */
                    }
                    fset(du, UFL_FLEEING);
                }
                else {
                    /* nur teilweise geflohene Einheiten mergen sich wieder */
                    df->alive += df->run.number;
                    s->size[0] += df->run.number;
                    s->size[statusrow(df->status)] += df->run.number;
                    s->alive += df->run.number;
                    sum_hp += df->run.hp;
                    df->run.number = 0;
                    df->run.hp = 0;
                    /* df->run.region = NULL; */

                    reportcasualties(b, df, dead);

                    scale_number(du, df->alive);
                    du->hp = sum_hp;
                }
            }
            else {
                if (df->alive == 0) {
                    /* alle sind tot, niemand geflohen. Einheit aufl�sen */
                    df->run.number = 0;
                    df->run.hp = 0;

                    /* Report the casualties */
                    reportcasualties(b, df, dead);

                    /* Distribute Loot */
                    loot_items(df);

                    setguard(du, false);
                    scale_number(du, 0);
                }
                else {
                    df->run.number = 0;
                    df->run.hp = 0;

                    reportcasualties(b, df, dead);

                    scale_number(du, df->alive);
                    du->hp = sum_hp;
                }
            }
            s->flee += df->run.number;

            if (playerrace(u_race(du))) {
                /* tote im kampf werden zu regionsuntoten:
                 * for each of them, a peasant will die as well */
                dead_players += dead;
            }
            if (du->hp < du->number) {
                log_error("%s has less hitpoints (%u) than people (%u)\n", itoa36(du->no), du->hp, du->number);
                du->hp = du->number;
            }
        }
        s->alive += s->healed;
        assert(snumber == s->flee + s->alive + s->dead);
    }

    battle_effects(b, dead_players);

    for (s = b->sides; s != b->sides + b->nsides; ++s) {
        message *seen = msg_message("battle::army_report",
            "index abbrev dead fled survived",
            army_index(s), sideabkz(s, false), s->dead, s->flee, s->alive);
        message *unseen = msg_message("battle::army_report",
            "index abbrev dead fled survived",
            army_index(s), "-?-", s->dead, s->flee, s->alive);

        for (bf = b->factions; bf; bf = bf->next) {
            faction *f = bf->faction;
            message *m = seematrix(f, s) ? seen : unseen;

            message_faction(b, f, m);
        }

        msg_release(seen);
        msg_release(unseen);
    }

    /* Wir benutzen drifted, um uns zu merken, ob ein Schiff
     * schonmal Schaden genommen hat. (moved und drifted
     * sollten in flags �berf�hrt werden */

    for (s = b->sides; s != b->sides + b->nsides; ++s) {
        fighter *df;

        for (df = s->fighters; df; df = df->next) {
            unit *du = df->unit;
            item *l;

            /* Beute verteilen */
            for (l = df->loot; l; l = l->next) {
                const item_type *itype = l->type;
                message *m =
                    msg_message("battle_loot", "unit amount item", du, l->number,
                    itype->rtype);
                message_faction(b, du->faction, m);
                msg_release(m);
                i_change(&du->items, itype, l->number);
            }

            /* Wenn sich die Einheit auf einem Schiff befindet, wird
             * dieses Schiff besch�digt. Andernfalls ein Schiff, welches
             * evt. zuvor verlassen wurde. */
            if (ships_damaged) {
                ship *sh;
                if (du->ship)
                    sh = du->ship;
                else
                    sh = leftship(du);

                if (sh && fval(sh, SF_DAMAGED)) {
                    int n = b->turn - 2;
                    if (n > 0) {
                        double dmg =
                            config_get_flt("rules.ship.damage.battleround",
                            0.05F);
                        damage_ship(sh, dmg * n);
                        freset(sh, SF_DAMAGED);
                    }
                }
            }
        }
    }

    if (ships_damaged) {
        ship **sp = &r->ships;

        while (*sp) {
            ship *sh = *sp;
            freset(sh, SF_DAMAGED);
            if (sh->damage >= sh->size * DAMAGE_SCALE) {
                remove_ship(sp, sh);
            }
            if (*sp == sh)
                sp = &sh->next;
        }
    }

    reorder_fleeing(r);
}

static void battle_punit(unit * u, battle * b)
{
    bfaction *bf;

    for (bf = b->factions; bf; bf = bf->next) {
        faction *f = bf->faction;
        strlist *S = 0, *x;

        spunit(&S, f, u, 4, seen_battle);
        for (x = S; x; x = x->next) {
            fbattlerecord(b, f, x->s);
        }
        if (S)
            freestrlist(S);
    }
}

static void print_fighters(battle * b, const side * s)
{
    fighter *df;
    int row;

    for (row = 1; row != NUMROWS; ++row) {
        message *m = NULL;

        for (df = s->fighters; df; df = df->next) {
            unit *du = df->unit;
            int thisrow = statusrow(df->unit->status);

            if (row == thisrow) {
                if (m == NULL) {
                    m = msg_message("battle::row_header", "row", row);
                    message_all(b, m);
                }
                battle_punit(du, b);
            }
        }
        if (m != NULL)
            msg_release(m);
    }
}

bool is_attacker(const fighter * fig)
{
    return fval(fig, FIG_ATTACKER) != 0;
}

static void set_attacker(fighter * fig)
{
    fset(fig, FIG_ATTACKER);
}

static void print_header(battle * b)
{
    bfaction *bf;
    char zText[32 * MAXSIDES];

    for (bf = b->factions; bf; bf = bf->next) {
        message *m;
        faction *f = bf->faction;
        const char *lastf = NULL;
        bool first = false;
        side *s;
        char *bufp = zText;
        size_t size = sizeof(zText) - 1;

        for (s = b->sides; s != b->sides + b->nsides; ++s) {
            fighter *df;
            for (df = s->fighters; df; df = df->next) {
                if (is_attacker(df)) {
                    if (first) {
                        bufp = STRLCPY(bufp, ", ", size);
                    }
                    if (lastf) {
                        bufp = STRLCPY(bufp, lastf, size);
                        first = true;
                    }
                    if (seematrix(f, s))
                        lastf = sidename(s);
                    else
                        lastf = LOC(f->locale, "unknown_faction_dative");
                    break;
                }
            }
        }
        if (first) {
            bufp = STRLCPY(bufp, " ", size);
            bufp = STRLCPY(bufp, LOC(f->locale, "and"), size);
            bufp = STRLCPY(bufp, " ", size);
        }
        if (lastf) {
            bufp = STRLCPY(bufp, lastf, size);
        }

        m = msg_message("battle::starters", "factions", zText);
        message_faction(b, f, m);
        msg_release(m);
    }
}

static void print_stats(battle * b)
{
    side *s2;
    side *s;
    int i = 0;
    for (s = b->sides; s != b->sides + b->nsides; ++s) {
        bfaction *bf;

        ++i;

        for (bf = b->factions; bf; bf = bf->next) {
            faction *f = bf->faction;
            const char *loc_army = LOC(f->locale, "battle_army");
            char *bufp;
            const char *header;
            size_t rsize, size;
            int komma;
            const char *sname =
                seematrix(f, s) ? sidename(s) : LOC(f->locale, "unknown_faction");
            message *msg;
            char buf[1024];

            message_faction(b, f, msg_separator);

            msg = msg_message("battle_army", "index name", army_index(s), sname);
            message_faction(b, f, msg);
            msg_release(msg);

            bufp = buf;
            size = sizeof(buf);
            komma = 0;
            header = LOC(f->locale, "battle_opponents");

            for (s2 = b->sides; s2 != b->sides + b->nsides; ++s2) {
                if (enemy(s2, s)) {
                    const char *abbrev = seematrix(f, s2) ? sideabkz(s2, false) : "-?-";
                    rsize = slprintf(bufp, size, "%s %s %d(%s)",
                        komma++ ? "," : (const char *)header, loc_army, army_index(s2),
                        abbrev);
                    if (rsize > size)
                        rsize = size - 1;
                    size -= rsize;
                    bufp += rsize;
                }
            }
            if (komma)
                fbattlerecord(b, f, buf);

            bufp = buf;
            size = sizeof(buf);
            komma = 0;
            header = LOC(f->locale, "battle_helpers");

            for (s2 = b->sides; s2 != b->sides + b->nsides; ++s2) {
                if (friendly(s2, s)) {
                    const char *abbrev = seematrix(f, s2) ? sideabkz(s2, false) : "-?-";
                    rsize = slprintf(bufp, size, "%s %s %d(%s)",
                        komma++ ? "," : (const char *)header, loc_army, army_index(s2),
                        abbrev);
                    if (rsize > size)
                        rsize = size - 1;
                    size -= rsize;
                    bufp += rsize;
                }
            }
            if (komma)
                fbattlerecord(b, f, buf);

            bufp = buf;
            size = sizeof(buf);
            komma = 0;
            header = LOC(f->locale, "battle_attack");

            for (s2 = b->sides; s2 != b->sides + b->nsides; ++s2) {
                if (s->relations[s2->index] & E_ATTACKING) {
                    const char *abbrev = seematrix(f, s2) ? sideabkz(s2, false) : "-?-";
                    rsize =
                        slprintf(bufp, size, "%s %s %d(%s)",
                        komma++ ? "," : (const char *)header, loc_army, army_index(s2),
                        abbrev);
                    if (rsize > size)
                        rsize = size - 1;
                    size -= rsize;
                    bufp += rsize;
                }
            }
            if (komma)
                fbattlerecord(b, f, buf);
        }

        print_fighters(b, s);
    }

    message_all(b, msg_separator);

    /* Besten Taktiker ermitteln */

    b->max_tactics = 0;

    for (s = b->sides; s != b->sides + b->nsides; ++s) {
        if (!selist_empty(s->leader.fighters)) {
            b->max_tactics = MAX(b->max_tactics, s->leader.value);
        }
    }

    if (b->max_tactics > 0) {
        for (s = b->sides; s != b->sides + b->nsides; ++s) {
            if (s->leader.value == b->max_tactics) {
                selist *ql;
                int qi;

                for (qi = 0, ql = s->leader.fighters; ql; selist_advance(&ql, &qi, 1)) {
                    fighter *tf = (fighter *)selist_get(ql, qi);
                    unit *u = tf->unit;
                    message *m = NULL;
                    if (!is_attacker(tf)) {
                        m = msg_message("battle::tactics_lost", "unit", u);
                    }
                    else {
                        m = msg_message("battle::tactics_won", "unit", u);
                    }
                    message_all(b, m);
                    msg_release(m);
                }
            }
        }
    }
}

static int weapon_weight(const weapon * w, bool missile)
{
    if (missile == !!(fval(w->type, WTF_MISSILE))) {
        return w->attackskill + w->defenseskill;
    }
    return 0;
}

side * get_side(battle * b, const struct unit * u)
{
    side * s;
    for (s = b->sides; s != b->sides + b->nsides; ++s) {
        if (s->faction == u->faction) {
            fighter * fig;
            for (fig = s->fighters; fig; fig = fig->next) {
                if (fig->unit == u) {
                    return s;
                }
            }
        }
    }
    return 0;
}

side * find_side(battle * b, const faction * f, const group * g, unsigned int flags, const faction * stealthfaction)
{
    side * s;
    for (s = b->sides; s != b->sides + b->nsides; ++s) {
        if (s->faction == f && s->group == g) {
            unsigned int s1flags = flags | SIDE_HASGUARDS;
            unsigned int s2flags = s->flags | SIDE_HASGUARDS;
            if (rule_anon_battle && s->stealthfaction != stealthfaction) {
                continue;
            }
            if (s1flags == s2flags) {
                return s;
            }
        }
    }
    return 0;
}

fighter *make_fighter(battle * b, unit * u, side * s1, bool attack)
{
#define WMAX 20
    weapon weapons[WMAX];
    int owp[WMAX];
    int dwp[WMAX];
    region *r = b->region;
    item *itm;
    fighter *fig = NULL;
    int h, i, tactics = effskill(u, SK_TACTICS, 0);
    int berserk;
    int strongmen;
    int speeded = 0, speed = 1;
    int rest;
    const group *g = NULL;
    const faction *stealthfaction = get_otherfaction(u);
    unsigned int flags = 0;

    assert(u->number);
    if (fval(u, UFL_ANON_FACTION) != 0)
        flags |= SIDE_STEALTH;
    if (!(AllianceAuto() & HELP_FIGHT) && fval(u, UFL_GROUP)) {
        const attrib *agroup = a_find(u->attribs, &at_group);
        if (agroup != NULL)
            g = (const group *)agroup->data.v;
    }

    /* Illusionen und Zauber kaempfen nicht */
    if (fval(u_race(u), RCF_ILLUSIONARY) || u->number == 0) {
        return NULL;
    }
    if (s1 == NULL) {
        s1 = find_side(b, u->faction, g, flags, stealthfaction);
        /* aliances are moved out of make_fighter and will be handled later */
        if (!s1) {
            s1 = make_side(b, u->faction, g, flags, stealthfaction);
        }
        else if (!stealthfaction) {
            s1->stealthfaction = NULL;
        }
        /* Zu diesem Zeitpunkt ist attacked noch 0, da die Einheit f�r noch
         * keinen Kampf ausgew�hlt wurde (sonst w�rde ein fighter existieren) */
    }
    fig = (struct fighter*)calloc(1, sizeof(struct fighter));

    fig->next = s1->fighters;
    s1->fighters = fig;

    fig->unit = u;
    /* In einer Burg mu� man a) nicht Angreifer sein, und b) drin sein, und
     * c) noch Platz finden. d) menschan�hnlich sein */
    if (attack) {
        set_attacker(fig);
    }
    else {
        building *bld = u->building;
        if (bld && bld->sizeleft >= u->number && playerrace(u_race(u))) {
            fig->building = bld;
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
    assert(fig->alive > 0);
    fig->person = (struct person*)calloc((size_t)fig->alive, sizeof(struct person));

    h = u->hp / u->number;
    assert(h);
    rest = u->hp % u->number;

    /* Effekte von Spr�chen */

    if (u->attribs) {
        const curse_type *speed_ct;
        speed_ct = ct_find("speed");
        if (speed_ct) {
            curse *c = get_curse(u->attribs, speed_ct);
            if (c) {
                speeded = get_cursedmen(u, c);
                speed = curse_geteffect_int(c);
            }
        }
    }

    /* Effekte von Alchemie */
    berserk = get_effect(u, oldpotiontype[P_BERSERK]);
    /* change_effect wird in ageing gemacht */

    /* Effekte von Artefakten */
    strongmen = MIN(fig->unit->number, trollbelts(u));

    /* Hitpoints, Attack- und Defence-Boni f�r alle Personen */
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
        /* Leute mit Kraftzauber machen +2 Schaden im Nahkampf. */
        if (i < strongmen) {
            fig->person[i].damage += 2;
        }
    }

    /* F�r alle Waffengattungen wird bestimmt, wie viele der Personen mit
     * ihr k�mpfen k�nnten, und was ihr Wert darin ist. */
    if (u_race(u)->battle_flags & BF_EQUIPMENT) {
        int oi = 0, di = 0, w = 0;
        for (itm = u->items; itm && w != WMAX; itm = itm->next) {
            const weapon_type *wtype = resource2weapon(itm->type->rtype);
            if (wtype == NULL || itm->number == 0)
                continue;
            weapons[w].attackskill = weapon_skill(wtype, u, true);
            weapons[w].defenseskill = weapon_skill(wtype, u, false);
            if (weapons[w].attackskill >= 0 || weapons[w].defenseskill >= 0) {
                weapons[w].type = wtype;
                weapons[w].used = 0;
                weapons[w].count = itm->number;
                ++w;
            }
            assert(w != WMAX);
        }
        assert(w >= 0);
        fig->weapons = (weapon *)calloc(sizeof(weapon), (size_t)(w + 1));
        memcpy(fig->weapons, weapons, (size_t)w * sizeof(weapon));

        for (i = 0; i != w; ++i) {
            int j, o = 0, d = 0;
            for (j = 0; j != i; ++j) {
                if (weapon_weight(fig->weapons + j,
                    true) >= weapon_weight(fig->weapons + i, true))
                    ++d;
                if (weapon_weight(fig->weapons + j,
                    false) >= weapon_weight(fig->weapons + i, false))
                    ++o;
            }
            for (j = i + 1; j != w; ++j) {
                if (weapon_weight(fig->weapons + j,
                    true) > weapon_weight(fig->weapons + i, true))
                    ++d;
                if (weapon_weight(fig->weapons + j,
                    false) > weapon_weight(fig->weapons + i, false))
                    ++o;
            }
            owp[o] = i;
            dwp[d] = i;
        }
        /* jetzt enthalten owp und dwp eine absteigend schlechter werdende Liste der Waffen
         * oi and di are the current index to the sorted owp/dwp arrays
         * owp, dwp contain indices to the figther::weapons array */

        /* hand out melee weapons: */
        for (i = 0; i != fig->alive; ++i) {
            int wpless = weapon_skill(NULL, u, true);
            while (oi != w
                && (fig->weapons[owp[oi]].used == fig->weapons[owp[oi]].count
                || fval(fig->weapons[owp[oi]].type, WTF_MISSILE))) {
                ++oi;
            }
            if (oi == w)
                break;                  /* no more weapons available */
            if (weapon_weight(fig->weapons + owp[oi], false) <= wpless) {
                continue;               /* we fight better with bare hands */
            }
            fig->person[i].melee = &fig->weapons[owp[oi]];
            ++fig->weapons[owp[oi]].used;
        }
        /* hand out missile weapons (from back to front, in case of mixed troops). */
        for (di = 0, i = fig->alive; i-- != 0;) {
            while (di != w
                && (fig->weapons[dwp[di]].used == fig->weapons[dwp[di]].count
                || !fval(fig->weapons[dwp[di]].type, WTF_MISSILE))) {
                ++di;
            }
            if (di == w)
                break;                  /* no more weapons available */
            if (weapon_weight(fig->weapons + dwp[di], true) > 0) {
                fig->person[i].missile = &fig->weapons[dwp[di]];
                ++fig->weapons[dwp[di]].used;
            }
        }
    }

    s1->size[statusrow(fig->status)] += u->number;
    s1->size[SUM_ROW] += u->number;
    if (u_race(u)->battle_flags & BF_NOBLOCK) {
        s1->nonblockers[statusrow(fig->status)] += u->number;
    }

    if (u_race(fig->unit)->flags & RCF_HORSE) {
        fig->horses = fig->unit->number;
        fig->elvenhorses = 0;
    }
    else {
        const resource_type *rt_horse = 0;
        const resource_type *rt_elvenhorse = 0;
        rt_elvenhorse = get_resourcetype(R_UNICORN);
        rt_horse = get_resourcetype(R_CHARGER);
        if (!rt_horse) {
            rt_horse = get_resourcetype(R_HORSE);
        }
        fig->horses = rt_horse ? i_get(u->items, rt_horse->itype) : 0;
        fig->elvenhorses = rt_elvenhorse ? i_get(u->items, rt_elvenhorse->itype) : 0;
    }

    if (u_race(u)->battle_flags & BF_EQUIPMENT) {
        for (itm = u->items; itm; itm = itm->next) {
            if (itm->type->rtype->atype) {
                if (i_canuse(u, itm->type)) {
                    struct armor *adata = (struct armor *)malloc(sizeof(armor)), **aptr;
                    adata->atype = itm->type->rtype->atype;
                    adata->count = itm->number;
                    for (aptr = &fig->armors; *aptr; aptr = &(*aptr)->next) {
                        if (adata->atype->prot > (*aptr)->atype->prot)
                            break;
                    }
                    adata->next = *aptr;
                    *aptr = adata;
                }
            }
        }
    }

    /* Jetzt mu� noch geschaut werden, wo die Einheit die jeweils besten
     * Werte hat, das kommt aber erst irgendwo sp�ter. Ich entscheide
     * w�rend des Kampfes, welche ich nehme, je nach Gegner. Deswegen auch
     * keine addierten boni. */

    /* Zuerst mal die Spezialbehandlung gewisser Sonderf�lle. */
    fig->magic = effskill(u, SK_MAGIC, 0);

    if (fig->horses) {
        if (!fval(r->terrain, CAVALRY_REGION) || r_isforest(r)
            || effskill(u, SK_RIDING, 0) < CavalrySkill()
            || u_race(u) == get_race(RC_TROLL) || fval(u, UFL_WERE))
            fig->horses = 0;
    }

    if (fig->elvenhorses) {
        if (effskill(u, SK_RIDING, 0) < 5 || u_race(u) == get_race(RC_TROLL)
            || fval(u, UFL_WERE))
            fig->elvenhorses = 0;
    }

    /* Schauen, wie gut wir in Taktik sind. */
    if (tactics > 0 && u_race(u) == get_race(RC_INSECT))
        tactics -= 1 - (int)log10(fig->side->size[SUM_ROW]);
#ifdef TACTICS_MODIFIER
    if (tactics > 0 && statusrow(fig->status) == FIGHT_ROW)
        tactics += TACTICS_MODIFIER;
    if (tactics > 0 && statusrow(fig->status) > BEHIND_ROW) {
        tactics -= TACTICS_MODIFIER;
    }
#endif

    if (tactics > 0) {
        int bonus = 0;

        for (i = 0; i < fig->alive; i++) {
            int p_bonus = 0;
            int rnd;

            do {
                rnd = (int)(rng_int() % 100);
                if (rnd >= 40 && rnd <= 69)
                    p_bonus += 1;
                else if (rnd <= 89)
                    p_bonus += 2;
                else
                    p_bonus += 3;
            } while (rnd >= 97);
            bonus = MAX(p_bonus, bonus);
        }
        tactics += bonus;
    }

    add_tactics(&fig->side->leader, fig, tactics);
    ++b->nfighters;
    return fig;
}

fighter * get_fighter(battle * b, const struct unit * u)
{
    side * s;

    for (s = b->sides; s != b->sides + b->nsides; ++s) {
        fighter *fig;
        if (s->faction == u->faction) {
            for (fig = s->fighters; fig; fig = fig->next) {
                if (fig->unit == u) {
                    return fig;
                }
            }
        }
    }
    return 0;
}

static int join_battle(battle * b, unit * u, bool attack, fighter ** cp)
{
    side *s;
    fighter *c = NULL;

    if (!attack) {
        attrib *a = a_find(u->attribs, &at_fleechance);
        if (a != NULL) {
            if (rng_double() <= a->data.flt) {
                *cp = NULL;
                return false;
            }
        }
    }

    for (s = b->sides; s != b->sides + b->nsides; ++s) {
        fighter *fig;
        if (s->faction == u->faction) {
            for (fig = s->fighters; fig; fig = fig->next) {
                if (fig->unit == u) {
                    c = fig;
                    if (attack) {
                        set_attacker(fig);
                    }
                    break;
                }
            }
        }
    }
    if (!c) {
        *cp = make_fighter(b, u, NULL, attack);
        return *cp != NULL;
    }
    *cp = c;
    return false;
}

battle *make_battle(region * r)
{
    battle *b = (battle *)calloc(1, sizeof(battle));
    unit *u;
    bfaction *bf;
    building * bld;

    /* Alle Mann raus aus der Burg! */
    for (bld = r->buildings; bld != NULL; bld = bld->next)
        bld->sizeleft = bld->size;

    b->region = r;
    b->plane = getplane(r);
    /* Finde alle Parteien, die den Kampf beobachten k�nnen: */
    for (u = r->units; u; u = u->next) {
        if (u->number > 0) {
            if (!fval(u->faction, FFL_MARK)) {
                fset(u->faction, FFL_MARK);
                for (bf = b->factions; bf; bf = bf->next) {
                    if (bf->faction == u->faction)
                        break;
                }
                if (!bf) {
                    bf = (bfaction *)calloc(sizeof(bfaction), 1);
                    ++b->nfactions;
                    bf->faction = u->faction;
                    bf->next = b->factions;
                    b->factions = bf;
                }
            }
        }
    }

    for (bf = b->factions; bf; bf = bf->next) {
        faction *f = bf->faction;
        freset(f, FFL_MARK);
    }
    return b;
}

static void free_side(side * si)
{
    selist_free(si->leader.fighters);
}

static void free_fighter(fighter * fig)
{
    while (fig->loot) {
        i_free(i_remove(&fig->loot, fig->loot));
    }
    while (fig->armors) {
        armor *a = fig->armors;
        fig->armors = a->next;
        free(a);
    }
    free(fig->person);
    free(fig->weapons);

}

static void battle_free(battle * b) {
    side *s;

    assert(b);

    for (s = b->sides; s != b->sides + b->nsides; ++s) {
        fighter *fnext = s->fighters;
        while (fnext) {
            fighter *fig = fnext;
            fnext = fig->next;
            free_fighter(fig);
            free(fig);
        }
        free_side(s);
    }
    free(b);
}

void free_battle(battle * b)
{
    while (b->factions) {
        bfaction *bf = b->factions;
        b->factions = bf->next;
        free(bf);
    }

    selist_free(b->leaders);
    selist_foreach(b->meffects, free);
    selist_free(b->meffects);

    battle_free(b);
}

static int *get_alive(side * s)
{
    return s->size;
}

static int battle_report(battle * b)
{
    side *s, *s2;
    bool cont = false;
    bool komma;
    bfaction *bf;

    for (s = b->sides; s != b->sides + b->nsides; ++s) {
        if (s->alive - s->removed > 0) {
            for (s2 = b->sides; s2 != b->sides + b->nsides; ++s2) {
                if (s2->alive - s2->removed > 0 && enemy(s, s2)) {
                    cont = true;
                    break;
                }
            }
            if (cont)
                break;
        }
    }

    fflush(stdout);

    for (bf = b->factions; bf; bf = bf->next) {
        faction *fac = bf->faction;
        char buf[32 * MAXSIDES];
        char *bufp = buf;
        size_t size = sizeof(buf) - 1;
        message *m;

        message_faction(b, fac, msg_separator);

        if (cont)
            m = msg_message("battle::lineup", "turn", b->turn);
        else
            m = msg_message("battle::after", "");
        message_faction(b, fac, m);
        msg_release(m);

        komma = false;
        for (s = b->sides; s != b->sides + b->nsides; ++s) {
            if (s->alive) {
                int r, k = 0, *alive = get_alive(s);
                int l = FIGHT_ROW;
                const char *abbrev = seematrix(fac, s) ? sideabkz(s, false) : "-?-";
                const char *loc_army = LOC(fac->locale, "battle_army");
                char buffer[32];

                if (komma) {
                    bufp = STRLCPY(bufp, ", ", size);
                }
                slprintf(buffer, sizeof(buffer), "%s %2d(%s): ",
                    loc_army, army_index(s), abbrev);
                
                bufp = STRLCPY(bufp, buffer, size);

                for (r = FIGHT_ROW; r != NUMROWS; ++r) {
                    if (alive[r]) {
                        if (l != FIGHT_ROW) {
                            bufp = STRLCPY(bufp, "+", size);
                        }
                        while (k--) {
                            bufp = STRLCPY(bufp, "0+", size);
                        }
                        sprintf(buffer, "%d", alive[r]);
                        
                        bufp = STRLCPY(bufp, buffer, size);

                        k = 0;
                        l = r + 1;
                    }
                    else
                        ++k;
                }

                komma = true;
            }
        }
        *bufp = 0;
        fbattlerecord(b, fac, buf);
    }
    return cont;
}

static void join_allies(battle * b)
{
    region *r = b->region;
    unit *u;
    side *s, *s_end = b->sides + b->nsides;
    /* make_side might be adding a new faction, but it adds them to the end
     * of the list, so we're safe in our iteration here if we remember the end
     * up front. */
    for (u = r->units; u; u = u->next) {
        /* Was ist mit Schiffen? */
        if (u->status != ST_FLEE && u->status != ST_AVOID
            && !fval(u, UFL_LONGACTION | UFL_ISNEW) && u->number > 0) {
            faction *f = u->faction;
            fighter *c = NULL;

            for (s = b->sides; s != s_end; ++s) {
                side *se;
                /* Wenn alle attackierten noch FFL_NOAID haben, dann k�mpfe nicht mit. */
                if (fval(s->faction, FFL_NOAID))
                    continue;
                if (s->faction != f) {
                    /* Wenn wir attackiert haben, kommt niemand mehr hinzu: */
                    if (s->bf->attacker)
                        continue;
                    /* alliiert m�ssen wir schon sein, sonst ist's eh egal : */
                    if (!alliedunit(u, s->faction, HELP_FIGHT))
                        continue;
                    /* wenn die partei verborgen ist, oder gar eine andere
                     * vorgespiegelt wird, und er sich uns gegen�ber nicht zu
                     * erkennen gibt, helfen wir ihm nicht */
                    if (s->stealthfaction) {
                        if (!allysfm(s, u->faction, HELP_FSTEALTH)) {
                            continue;
                        }
                    }
                }
                /* einen alliierten angreifen d�rfen sie nicht, es sei denn, der
                 * ist mit einem alliierten verfeindet, der nicht attackiert
                 * hat: */
                for (se = b->sides; se != s_end; ++se) {
                    if (u->faction == se->faction)
                        continue;
                    if (alliedunit(u, se->faction, HELP_FIGHT) && !se->bf->attacker) {
                        continue;
                    }
                    if (enemy(s, se))
                        break;
                }
                if (se == s_end)
                    continue;
                /* Wenn die Einheit belagert ist, mu� auch einer der Alliierten belagert sein: */
                if (besieged(u)) {
                    fighter *ally;
                    for (ally = s->fighters; ally; ally = ally->next) {
                        if (besieged(ally->unit)) {
                            break;
                        }
                    }
                    if (ally == NULL)
                        continue;
                }
                /* keine Einw�nde, also soll er mitmachen: */
                if (c == NULL) {
                    if (!join_battle(b, u, false, &c)) {
                        continue;
                    }
                }

                /* the enemy of my friend is my enemy: */
                for (se = b->sides; se != s_end; ++se) {
                    if (se->faction != u->faction && enemy(s, se)) {
                        set_enemy(se, c->side, false);
                    }
                }
            }
        }
    }

    for (s = b->sides; s != b->sides + b->nsides; ++s) {
        int si;
        side *sa;
        faction *f = s->faction;

        /* Den Feinden meiner Feinde gebe ich Deckung (gegen gemeinsame Feinde): */
        for (si = 0; s->enemies[si]; ++si) {
            side *se = s->enemies[si];
            int ai;
            for (ai = 0; se->enemies[ai]; ++ai) {
                side *as = se->enemies[ai];
                if (as == s || !enemy(as, s)) {
                    set_friendly(as, s);
                }
            }
        }

        for (sa = s + 1; sa != b->sides + b->nsides; ++sa) {
            plane *pl = rplane(r);
            if (enemy(s, sa))
                continue;
            if (friendly(s, sa))
                continue;
            if (!alliedgroup(pl, f, sa->faction, f->allies, HELP_FIGHT))
                continue;
            if (!alliedgroup(pl, sa->faction, f, sa->faction->allies, HELP_FIGHT))
                continue;

            set_friendly(s, sa);
        }
    }
}

static void flee(const troop dt)
{
    fighter *fig = dt.fighter;
    unit *u = fig->unit;

    fig->run.hp += fig->person[dt.index].hp;
    ++fig->run.number;

    setguard(u, false);

    kill_troop(dt);
}

static bool start_battle(region * r, battle ** bp)
{
    battle *b = NULL;
    unit *u;
    bool fighting = false;

    for (u = r->units; u != NULL; u = u->next) {
        if (fval(u, UFL_LONGACTION))
            continue;
        if (u->number > 0) {
            order *ord;

            for (ord = u->orders; ord; ord = ord->next) {
                if (getkeyword(ord) == K_ATTACK) {
                    unit *u2;
                    fighter *c1, *c2;
                    ship *lsh = NULL;
                    plane *pl = rplane(r);

                    if (pl && fval(pl, PFL_NOATTACK)) {
                        cmistake(u, ord, 271, MSG_BATTLE);
                        continue;
                    }

                    if (u_race(u)->battle_flags & BF_NO_ATTACK) {
                        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "race_no_attack",
                            "race", u_race(u)));
                        continue;
                    }
                    /**
                    ** Fehlerbehandlung Angreifer
                    **/
                    if (LongHunger(u)) {
                        cmistake(u, ord, 225, MSG_BATTLE);
                        continue;
                    }

                    if (u->status == ST_AVOID || u->status == ST_FLEE) {
                        cmistake(u, ord, 226, MSG_BATTLE);
                        continue;
                    }

                    /* ist ein Fl�chtling aus einem andern Kampf */
                    if (fval(u, UFL_LONGACTION))
                        continue;

                    if (peace_ct && curse_active(get_curse(r->attribs, peace_ct))) {
                        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "peace_active", ""));
                        continue;
                    }

                    if (slave_ct && curse_active(get_curse(u->attribs, slave_ct))) {
                        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "slave_active", ""));
                        continue;
                    }

                    if ((u->ship != NULL && !fval(r->terrain, SEA_REGION))
                        || (lsh = leftship(u)) != NULL) {
                        if (is_guarded(r, u)) {
                            if (lsh) {
                                cmistake(u, ord, 234, MSG_BATTLE);
                            }
                            else {
                                /* Fehler: "Das Schiff mu� erst verlassen werden" */
                                cmistake(u, ord, 19, MSG_BATTLE);
                            }
                            continue;
                        }
                    }

                    /* Ende Fehlerbehandlung Angreifer */

                    init_order(ord);
                    /* attackierte Einheit ermitteln */
                    getunit(r, u->faction, &u2);

                    /* Beginn Fehlerbehandlung */
                    /* Fehler: "Die Einheit wurde nicht gefunden" */
                    if (!u2 || u2->number == 0 || !cansee(u->faction, u->region, u2, 0)) {
                        ADDMSG(&u->faction->msgs, msg_feedback(u, ord,
                            "feedback_unit_not_found", ""));
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
                    if (IsImmune(u2->faction)) {
                        add_message(&u->faction->msgs,
                            msg_feedback(u, u->thisorder, "newbie_immunity_error", "turns",
                            NewbieImmunity()));
                        continue;
                    }
                    /* Fehler: "Die Einheit ist mit uns alliert" */

                    if (calm_ct) {
                        attrib *a = a_find(u->attribs, &at_curse);
                        bool calm = false;
                        while (a && a->type == &at_curse) {
                            curse *c = (curse *)a->data.v;
                            if (c->type == calm_ct
                                && curse_geteffect_int(c) == u2->faction->subscription) {
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
                    if (b == NULL) {
                        unit *utmp;
                        for (utmp = r->units; utmp != NULL; utmp = utmp->next) {
                            fset(utmp->faction, FFL_NOAID);
                        }
                        b = make_battle(r);
                    }
                    join_battle(b, u, true, &c1);
                    join_battle(b, u2, false, &c2);

                    /* Hat die attackierte Einheit keinen Noaid-Status,
                     * wird das Flag von der Faction genommen, andere
                     * Einheiten greifen ein. */
                    if (!fval(u2, UFL_NOAID))
                        freset(u2->faction, FFL_NOAID);

                    if (c1 != NULL && c2 != NULL) {
                        /* Merken, wer Angreifer ist, f�r die R�ckzahlung der
                         * Pr�combataura bei kurzem Kampf. */
                        c1->side->bf->attacker = true;

                        set_enemy(c1->side, c2->side, true);
                        fighting = true;
                    }
                }
            }
        }
    }
    *bp = b;
    return fighting;
}

/** execute one round of attacks
 * fig->fighting is used to determine who attacks, not fig->alive, since
 * the latter may be influenced by attacks that already took place.
 */
static void battle_attacks(battle * b)
{
    side *s;

    for (s = b->sides; s != b->sides + b->nsides; ++s) {
        fighter *fig;

        if (b->turn != 0 || (b->max_tactics > 0
            && get_tactics(s, NULL) == b->max_tactics)) {
            for (fig = s->fighters; fig; fig = fig->next) {

                /* ist in dieser Einheit noch jemand handlungsf�hig? */
                if (fig->fighting <= 0)
                    continue;

                /* Handle the unit's attack on someone */
                do_attack(fig);
            }
        }
    }
}

/** updates the number of attacking troops in each fighter struct.
 * this has to be calculated _before_ the actual attacks take
 * place because otherwise dead troops would not strike in the
 * round they die. */
static void battle_update(battle * b)
{
    side *s;
    for (s = b->sides; s != b->sides + b->nsides; ++s) {
        fighter *fig;
        for (fig = s->fighters; fig; fig = fig->next) {
            fig->fighting = fig->alive - fig->removed;
        }
    }
}

/** attempt to flee from battle before the next round begins
 * there's a double attempt before the first round, but only
 * one attempt before round zero, the potential tactics round. */
static void battle_flee(battle * b)
{
    int attempt, flee_ops = 1;

    if (b->turn == 1)
        flee_ops = 2;

    for (attempt = 1; attempt <= flee_ops; ++attempt) {
        side *s;
        for (s = b->sides; s != b->sides + b->nsides; ++s) {
            fighter *fig;
            for (fig = s->fighters; fig; fig = fig->next) {
                unit *u = fig->unit;
                troop dt;
                int runners = 0;
                /* Flucht nicht bei mehr als 600 HP. Damit Wyrme t�tbar bleiben. */
                int runhp = MIN(600, (int)(0.9 + unit_max_hp(u) * hpflee(u->status)));

                if (u->ship && fval(u->region->terrain, SEA_REGION)) {
                    /* keine Flucht von Schiffen auf hoher See */
                    continue;
                }
                if (fval(u_race(u), RCF_UNDEAD) || u_race(u) == get_race(RC_SHADOWKNIGHT)) {
                    /* Untote fliehen nicht. Warum eigentlich? */
                    continue;
                }

                dt.fighter = fig;
                dt.index = fig->alive - fig->removed;
                while (s->size[SUM_ROW] && dt.index != 0) {
                    double ispaniced = 0.0;
                    --dt.index;
                    assert(dt.index >= 0 && dt.index < fig->unit->number);
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
                        if ((fig->person[dt.index].flags & FL_HIT) == 0)
                            continue;
                        if (fig->person[dt.index].hp <= runhp)
                            break;
                        if (fig->person[dt.index].flags & FL_PANICED) {
                            if ((fig->person[dt.index].flags & FL_COURAGE) == 0)
                                break;
                        }
                        continue;
                    }

                    if (fig->person[dt.index].flags & FL_PANICED) {
                        ispaniced = EFFECT_PANIC_SPELL;
                    }
                    if (chance(MIN(fleechance(u) + ispaniced, 0.90))) {
                        ++runners;
                        flee(dt);
                    }
                }
            }
        }
    }
}

static bool is_enemy(battle *b, unit *u1, unit *u2) {
    if (u1->faction != u2->faction) {
        if (b) {
            side *es, *s1 = 0, *s2 = 0;
            for (es = b->sides; es != b->sides + b->nsides; ++es) {
                if (!s1 && es->faction == u1->faction) s1 = es;
                else if (!s2 && es->faction == u2->faction) s2 = es;
                if (s1 && s2) {
                    return enemy(s1, s2);
                }
            }
        }
        else {
            return !help_enter(u1, u2);
        }
    }
    return false;
}

void force_leave(region *r, battle *b) {
    unit *u;

    for (u = r->units; u; u = u->next) {
        unit *uo = NULL;
        if (u->building) {
            uo = building_owner(u->building);
        }
        if (u->ship && r->land) {
            uo = ship_owner(u->ship);
        }
        if (uo && is_enemy(b, uo, u)) {
            message *msg = NULL;
            if (u->building) {
                msg = msg_message("force_leave_building", "unit owner building", u, uo, u->building);
            }
            else {
                msg = msg_message("force_leave_ship", "unit owner ship", u, uo, u->ship);
            }
            if (msg) {
                ADDMSG(&u->faction->msgs, msg);
            }
            leave(u, false);
        }
    }
}


void do_battle(region * r)
{
    battle *b = NULL;
    bool fighting = false;
    ship *sh;
    if (msg_separator == NULL) {
        msg_separator = msg_message("battle::section", "");
    }

    fighting = start_battle(r, &b);

    if (b == NULL)
        return;

    /* Bevor wir die alliierten hineinziehen, sollten wir schauen, *
     * Ob jemand fliehen kann. Dann er�brigt sich das ganze ja
     * vielleicht schon. */
    print_header(b);
    if (!fighting) {
        /* Niemand mehr da, Kampf kann nicht stattfinden. */
        message *m = msg_message("battle::aborted", "");
        message_all(b, m);
        msg_release(m);
        free_battle(b);
        return;
    }
    join_allies(b);
    make_heroes(b);

    /* make sure no ships are damaged initially */
    for (sh = r->ships; sh; sh = sh->next)
        freset(sh, SF_DAMAGED);

    /* Gibt es eine Taktikrunde ? */
    if (!selist_empty(b->leaders)) {
        b->turn = 0;
        b->has_tactics_turn = true;
    }
    else {
        b->turn = 1;
        b->has_tactics_turn = false;
    }

    /* PRECOMBATSPELLS */
    do_combatmagic(b, DO_PRECOMBATSPELL);

    print_stats(b);               /* gibt die Kampfaufstellung aus */
    log_debug("battle in %s (%d, %d) : ", regionname(r, 0), r->x, r->y);

    for (; battle_report(b) && b->turn <= max_turns; ++b->turn) {
        battle_flee(b);
        battle_update(b);
        battle_attacks(b);

    }

    /* Auswirkungen berechnen: */
    aftermath(b);
    if (rule_force_leave(FORCE_LEAVE_POSTCOMBAT)) {
        force_leave(b->region, b);
    }
    /* Hier ist das Gefecht beendet, und wir k�nnen die
     * Hilfsstrukturen * wieder l�schen: */

    free_battle(b);
}

void do_battles(void) {
    region *r;
    init_rules();
    for (r = regions; r; r = r->next) {
        do_battle(r);
    }
}
