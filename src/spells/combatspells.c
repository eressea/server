/*
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
 */
#include <platform.h>
#include "combatspells.h"

/* kernel includes */
#include <kernel/build.h>
#include <kernel/building.h>
#include <kernel/curse.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/spell.h>
#include <kernel/race.h>
#include <kernel/terrain.h>

#include <guard.h>
#include <battle.h>
#include <move.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/log.h>
#include <util/rand.h>
#include <util/rng.h>

#include <selist.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define EFFECT_HEALING_SPELL     5

/* ------------------------------------------------------------------ */
/* Kampfzauberfunktionen */

/* COMBAT */

static const char *spell_damage(int sp)
{
    switch (sp) {
    case 0:
        /* meist t�dlich 20-65 HP */
        return "5d10+15";
    case 1:
        /* sehr variabel 4-48 HP */
        return "4d12";
    case 2:
        /* leicht verwundet 4-18 HP */
        return "2d8+2";
    case 3:
        /* fast immer t�dlich 30-50 HP */
        return "5d5+25";
    case 4:
        /* verwundet 11-26 HP */
        return "3d6+8";
    case 5:
        /* leichter Schaden */
        return "2d4";
    default:
        /* schwer verwundet 14-34 HP */
        return "4d6+10";
    }
}

static double get_force(double power, int formel)
{
    switch (formel) {
    case 0:
        /* (4,8,12,16,20,24,28,32,36,40,44,..) */
        return (power * 4);
    case 1:
        /* (15,30,45,60,75,90,105,120,135,150,165,..) */
        return (power * 15);
    case 2:
        /* (40,80,120,160,200,240,280,320,360,400,440,..) */
        return (power * 40);
    case 3:
        /* (2,8,18,32,50,72,98,128,162,200,242,..) */
        return (power * power * 2);
    case 4:
        /* (4,16,36,64,100,144,196,256,324,400,484,..) */
        return (power * power * 4);
    case 5:
        /* (10,40,90,160,250,360,490,640,810,1000,1210,1440,..) */
        return (power * power * 10);
    case 6:
        /* (6,24,54,96,150,216,294,384,486,600,726,864) */
        return (power * power * 6);
    default:
        return power;
    }
}

/* Generischer Kampfzauber */
int damage_spell(struct castorder * co, int dmg, int strength)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    const spell * sp = co->sp;
    double power = co->force;
    battle *b = fi->side->battle;
    troop at, dt;
    message *m;
    /* Immer aus der ersten Reihe nehmen */
    int enemies, killed = 0;
    int force = lovar(get_force(power, strength));
    const char *damage = spell_damage(dmg);

    at.fighter = fi;
    at.index = 0;

    enemies = count_enemies(b, fi, FIGHT_ROW, BEHIND_ROW - 1, SELECT_ADVANCE);
    if (enemies == 0) {
        message *m =
            msg_message("battle::out_of_range", "mage spell", fi->unit, sp);
        message_all(b, m);
        msg_release(m);
        return 0;
    }

    while (force > 0 && killed < enemies) {
        dt = select_enemy(fi, FIGHT_ROW, BEHIND_ROW - 1, SELECT_ADVANCE);
        assert(dt.fighter);
        --force;
        killed += terminate(dt, at, AT_COMBATSPELL, damage, false);
    }

    m = msg_message("battle::combatspell", "mage spell dead",
        fi->unit, sp, killed);
    message_all(b, m);
    msg_release(m);

    return level;
}

/* Versteinern */
int sp_petrify(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    double power = co->force;
    const spell * sp = co->sp;
    battle *b = fi->side->battle;
    unit *mage = fi->unit;
    /* Wirkt auf erste und zweite Reihe */
    int force, enemies;
    int stoned = 0;
    message *m;

    force = lovar(get_force(power, 0));

    enemies = count_enemies(b, fi, FIGHT_ROW, BEHIND_ROW, SELECT_ADVANCE);
    if (!enemies) {
        message *m =
            msg_message("battle::out_of_range", "mage spell", fi->unit, sp);
        message_all(b, m);
        msg_release(m);
        return 0;
    }

    while (force && stoned < enemies) {
        troop dt = select_enemy(fi, FIGHT_ROW, BEHIND_ROW, SELECT_ADVANCE);
        unit *du = dt.fighter->unit;
        if (!is_magic_resistant(mage, du, 0)) {
            /* person ans ende hinter die lebenden schieben */
            remove_troop(dt);
            ++stoned;
        }
        --force;
    }

    m =
        msg_message("cast_petrify_effect", "mage spell amount", fi->unit, sp,
        stoned);
    message_all(b, m);
    msg_release(m);
    return level;
}

/* Benommenheit: eine Runde kein Angriff */
int sp_stun(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    double power = co->force;
    const spell * sp = co->sp;
    battle *b = fi->side->battle;
    unit *mage = fi->unit;
    message *m;
    /* Aus beiden Reihen nehmen */
    int force = 0, enemies;
    int stunned;

    if (power <= 0)
        return 0;

    force = lovar(get_force(power, 1));

    enemies = count_enemies(b, fi, FIGHT_ROW, BEHIND_ROW, SELECT_ADVANCE);
    if (!enemies) {
        message *m =
            msg_message("battle::out_of_range", "mage spell", fi->unit, sp);
        message_all(b, m);
        msg_release(m);
        return 0;
    }

    stunned = 0;
    while (force && stunned < enemies) {
        troop dt = select_enemy(fi, FIGHT_ROW, BEHIND_ROW, SELECT_ADVANCE);
        fighter *df = dt.fighter;
        unit *du = df->unit;

        --force;
        if (!is_magic_resistant(mage, du, 0)) {
            df->person[dt.index].flags |= FL_STUNNED;
            ++stunned;
        }
    }

    m = msg_message("cast_stun_effect", "mage spell amount", fi->unit, sp, stunned);
    message_all(b, m);
    msg_release(m);
    return level;
}

/** randomly shuffle an array
 * for correctness, see Donald E. Knuth, The Art of Computer Programming
 */
static void scramble_fighters(selist * ql)
{
    int qi, qlen = selist_length(ql);

    for (qi = 0; qi != qlen; ++qi) {
        int qj = qi + (rng_int() % (qlen - qi));
        void *a = selist_get(ql, qi);
        void *b = selist_replace(ql, qj, a);
        selist_replace(ql, qi, b);
    }
}

static bool select_armed(const side *vs, const fighter *fig, void *cbdata)
{
    int row = get_unitrow(fig, vs);

    UNUSED_ARG(cbdata);
    if (row >= FIGHT_ROW && row < BEHIND_ROW) {
        return fig->alive > 0 && fig->weapons;
    }
    return false;
}

/* Rosthauch */
int sp_combatrosthauch(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    double power = co->force;
    battle *b = fi->side->battle;
    selist *ql, *fgs;
    int force = lovar(power * 15);
    int qi, k = 0;

    if (!count_enemies(b, fi, FIGHT_ROW, BEHIND_ROW - 1,
        SELECT_ADVANCE | SELECT_FIND)) {
        message *msg = msg_message("rust_effect_0", "mage", fi->unit);
        message_all(b, msg);
        msg_release(msg);
        return 0;
    }

    fgs = select_fighters(b, fi->side, FS_ENEMY, select_armed, NULL);
    scramble_fighters(fgs);

    for (qi = 0, ql = fgs; force>0 && ql; selist_advance(&ql, &qi, 1)) {
        fighter *df = (fighter *)selist_get(ql, qi);
        int w;

        for (w = 0; df->weapons[w].type != NULL; ++w) {
            weapon *wp = df->weapons;
            int n = MIN(force, wp->used);
            if (n) {
                requirement *mat = wp->type->itype->construction->materials;
                bool iron = false;
                while (mat && mat->number > 0) {
                    if (mat->rtype == get_resourcetype(R_IRON)) {
                        iron = true;
                        break;
                    }
                    mat++;
                }
                if (iron) {
                    int p;
                    force -= n;
                    wp->used -= n;
                    k += n;
                    i_change(&df->unit->items, wp->type->itype, -n);
                    for (p = 0; n && p != df->unit->number; ++p) {
                        if (df->person[p].missile == wp) {
                            df->person[p].missile = NULL;
                            --n;
                        }
                    }
                    for (p = 0; n && p != df->unit->number; ++p) {
                        if (df->person[p].melee == wp) {
                            df->person[p].melee = NULL;
                            --n;
                        }
                    }
                }
            }
        }
    }
    selist_free(fgs);

    if (k == 0) {
        /* keine Waffen mehr da, die zerst�rt werden k�nnten */
        message *msg = msg_message("rust_effect_1", "mage", fi->unit);
        message_all(b, msg);
        msg_release(msg);
        fi->magic = 0;              /* k�mpft nichtmagisch weiter */
        level = 0;
    }
    else {
        message *msg = msg_message("rust_effect_2", "mage", fi->unit);
        message_all(b, msg);
        msg_release(msg);
    }
    return level;
}

int sp_sleep(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    const spell * sp = co->sp;
    battle *b = fi->side->battle;
    unit *mage = fi->unit;
    unit *du;
    troop dt;
    int force, enemies;
    int k = 0;
    message *m;
    /* Immer aus der ersten Reihe nehmen */

    force = lovar(co->force * 25);
    enemies = count_enemies(b, fi, FIGHT_ROW, BEHIND_ROW, SELECT_ADVANCE);

    if (!enemies) {
        m = msg_message("battle::out_of_range", "mage spell", fi->unit, sp);
        message_all(b, m);
        msg_release(m);
        return 0;
    }
    while (force && enemies) {
        dt = select_enemy(fi, FIGHT_ROW, BEHIND_ROW, SELECT_ADVANCE);
        assert(dt.fighter);
        du = dt.fighter->unit;
        if (!is_magic_resistant(mage, du, 0)) {
            dt.fighter->person[dt.index].flags |= FL_SLEEPING;
            ++k;
            --enemies;
        }
        --force;
    }

    m = msg_message("cast_sleep_effect", "mage spell amount", fi->unit, sp, k);
    message_all(b, m);
    msg_release(m);
    return level;
}

int sp_speed(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    double power = co->force;
    const spell * sp = co->sp;
    battle *b = fi->side->battle;
    int force;
    int allies;
    int targets = 0;
    message *m;

    force = lovar(power * power * 5);

    allies =
        count_allies(fi->side, FIGHT_ROW, BEHIND_ROW, SELECT_ADVANCE, ALLY_ANY);
    /* maximal 2*allies Versuche ein Opfer zu finden, ansonsten best�nde
     * die Gefahr eine Endlosschleife*/
    allies *= 2;

    while (force && allies) {
        troop dt = select_ally(fi, FIGHT_ROW, BEHIND_ROW, ALLY_ANY);
        fighter *df = dt.fighter;
        --allies;

        if (df) {
            if (df->person[dt.index].speed == 1) {
                df->person[dt.index].speed++;
                targets++;
                --force;
            }
        }
    }

    m =
        msg_message("cast_speed_effect", "mage spell amount", fi->unit, sp,
        targets);
    message_all(b, m);
    msg_release(m);
    return 1;
}

static skill_t random_skill(unit * u, bool weighted)
{
    int n = 0;
    skill *sv;

    for (sv = u->skills; sv != u->skills + u->skill_size; ++sv) {
        if (sv->level > 0) {
            if (weighted)
                n += sv->level;
            else
                ++n;
        }
    }

    if (n == 0)
        return NOSKILL;

    n = rng_int() % n;

    for (sv = u->skills; sv != u->skills + u->skill_size; ++sv) {
        if (sv->level > 0) {
            if (weighted) {
                if (n < (int)sv->level)
                    return sv->id;
                n -= sv->level;
            }
            else {
                if (n == 0)
                    return sv->id;
                --n;
            }
        }
    }

    assert(0 == 1);               /* Hier sollte er niemals ankommen. */
    return NOSKILL;
}

/** The mind blast spell for regular folks.
* This spell temporarily reduces the skill of the victims
*/
int sp_mindblast_temp(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    double power = co->force;
    const spell * sp = co->sp;
    battle *b = fi->side->battle;
    unit *mage = fi->unit;
    int k = 0, reset = 0, maxloss = (level + 2) / 3;
    message *m;
    int force = lovar(power * 25);
    int enemies = count_enemies(b, fi, FIGHT_ROW, BEHIND_ROW, SELECT_ADVANCE);

    if (!enemies) {
        m = msg_message("battle::out_of_range", "mage spell", fi->unit, sp);
        message_all(b, m);
        msg_release(m);
        return 0;
    }

    while (force > 0 && enemies > 0) {
        unit *du;
        troop dt = select_enemy(fi, FIGHT_ROW, BEHIND_ROW, SELECT_ADVANCE);

        assert(dt.fighter);
        du = dt.fighter->unit;
        if (du->flags & UFL_MARK) {
            /* not this one again */
            continue;
        }

        if (humanoidrace(u_race(du)) && force >= du->number) {
            if (!is_magic_resistant(mage, du, 0)) {
                skill_t sk = random_skill(du, true);
                if (sk != NOSKILL) {
                    int n = 1 + rng_int() % maxloss;
                    attrib *a = make_skillmod(sk, NULL, 0.0, n);
                    /* neat: you can add a whole lot of these to a unit, they stack */
                    a_add(&du->attribs, a);
                }
                k += du->number;
            }
            force -= du->number;
        }
        du->flags |= UFL_MARK;
        reset = 1;
        enemies -= du->number;
    }

    if (reset) {
        unit *u;
        for (u = b->region->units; u; u = u->next) {
            u->flags &= ~UFL_MARK;
        }
    }

    m = msg_message("sp_mindblast_temp_effect", "mage spell amount", mage, sp, k);
    message_all(b, m);
    msg_release(m);
    return level;
}

/** A mind blast spell for monsters.
 * This spell PERMANENTLY reduces the skill of the victims or kills them
 * when they have no skills left. Not currently in use.
 */
int sp_mindblast(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    double power = co->force;
    const spell * sp = co->sp;
    battle *b = fi->side->battle;
    unit *mage = fi->unit;
    int killed = 0, k = 0, reset = 0;
    message *m;
    int force = lovar(power * 25);
    int enemies = count_enemies(b, fi, FIGHT_ROW, BEHIND_ROW, SELECT_ADVANCE);

    if (!enemies) {
        m = msg_message("battle::out_of_range", "mage spell", fi->unit, sp);
        message_all(b, m);
        msg_release(m);
        return 0;
    }

    while (enemies > 0 && force > 0) {
        unit *du;
        troop dt = select_enemy(fi, FIGHT_ROW, BEHIND_ROW, SELECT_ADVANCE);

        assert(dt.fighter);
        du = dt.fighter->unit;
        if (du->flags & UFL_MARK) {
            /* not this one again */
            continue;
        }

        if (humanoidrace(u_race(du)) && force >= du->number) {
            if (!is_magic_resistant(mage, du, 0)) {
                skill_t sk = random_skill(du, false);
                if (sk != NOSKILL) {
                    skill *sv = unit_skill(du, sk);
                    if (sv) {
                        int n = 1 + rng_int() % 3;

                        reduce_skill(du, sv, n);
                        k += du->number;
                    }
                }
                else {
                    /* unit has no skill. kill it. */
                    kill_troop(dt);
                    ++killed;
                }
            }
            force -= du->number;
        }
        else {
            /* only works against humanoids, don't try others. but do remove them
             * from 'force' once or we may never terminate. */
            du->flags |= UFL_MARK;
            reset = 1;
        }
        enemies -= du->number;
    }

    if (reset) {
        unit *u;
        for (u = b->region->units; u; u = u->next) {
            u->flags &= ~UFL_MARK;
        }
    }

    m =
        msg_message("sp_mindblast_effect", "mage spell amount dead", mage, sp, k,
        killed);
    message_all(b, m);
    msg_release(m);
    return level;
}

int sp_dragonodem(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    double power = co->force;
    const spell * sp = co->sp;
    battle *b = fi->side->battle;
    troop dt;
    troop at;
    int force, enemies;
    int killed = 0;
    const char *damage;

    /* 11-26 HP */
    damage = spell_damage(4);
    /* Jungdrache 3->54, Drache 6->216, Wyrm 12->864 Treffer */
    force = lovar(get_force(power, 6));

    enemies = count_enemies(b, fi, FIGHT_ROW, BEHIND_ROW - 1, SELECT_ADVANCE);

    if (!enemies) {
        struct message *m =
            msg_message("battle::out_of_range", "mage spell", fi->unit, sp);
        message_all(b, m);
        msg_release(m);
        return 0;
    }
    else {
        struct message *m;

        at.fighter = fi;
        at.index = 0;

        while (force && killed < enemies) {
            dt = select_enemy(fi, FIGHT_ROW, BEHIND_ROW - 1, SELECT_ADVANCE);
            assert(dt.fighter);
            --force;
            killed += terminate(dt, at, AT_COMBATSPELL, damage, false);
        }

        m =
            msg_message("battle::combatspell", "mage spell dead", fi->unit, sp,
            killed);
        message_all(b, m);
        msg_release(m);
    }
    return level;
}

/* Feuersturm: Betrifft sehr viele Gegner (in der Regel alle),
 * macht nur vergleichsweise geringen Schaden */
int sp_immolation(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    const spell * sp = co->sp;
    battle *b = fi->side->battle;
    troop at;
    int force, qi, killed = 0;
    const char *damage;
    selist *fgs, *ql;
    message *m;

    /* 2d4 HP */
    damage = spell_damage(5);
    /* Betrifft alle Gegner */
    force = 99999;

    if (!count_enemies(b, fi, FIGHT_ROW, AVOID_ROW, SELECT_ADVANCE | SELECT_FIND)) {
        message *m =
            msg_message("battle::out_of_range", "mage spell", fi->unit, sp);
        message_all(b, m);
        msg_release(m);
        return 0;
    }

    at.fighter = fi;
    at.index = 0;

    fgs = fighters(b, fi->side, FIGHT_ROW, AVOID_ROW, FS_ENEMY);
    for (qi = 0, ql = fgs; ql; selist_advance(&ql, &qi, 1)) {
        fighter *df = (fighter *)selist_get(ql, qi);
        int n = df->alive - df->removed;
        troop dt;

        dt.fighter = df;
        while (n != 0) {
            dt.index = --n;
            killed += terminate(dt, at, AT_COMBATSPELL, damage, false);
            if (--force == 0)
                break;
        }
        if (force == 0)
            break;
    }
    selist_free(fgs);

    m =
        msg_message("battle::combatspell", "mage spell killed", fi->unit, sp,
        killed);
    message_all(b, m);
    msg_release(m);
    return level;
}

int sp_drainodem(fighter * fi, int level, double power, spell * sp)
{
    battle *b = fi->side->battle;
    troop dt;
    troop at;
    int force, enemies;
    int drained = 0;
    int killed = 0;
    const char *damage;
    message *m;

    /* 11-26 HP */
    damage = spell_damage(4);
    /* Jungdrache 3->54, Drache 6->216, Wyrm 12->864 Treffer */
    force = lovar(get_force(power, 6));

    enemies = count_enemies(b, fi, FIGHT_ROW, BEHIND_ROW - 1, SELECT_ADVANCE);

    if (!enemies) {
        m = msg_message("battle::out_of_range", "mage spell", fi->unit, sp);
        message_all(b, m);
        msg_release(m);
        return 0;
    }

    at.fighter = fi;
    at.index = 0;

    while (force && drained < enemies) {
        dt = select_enemy(fi, FIGHT_ROW, BEHIND_ROW - 1, SELECT_ADVANCE);
        assert(dt.fighter);
        if (hits(at, dt, NULL)) {
            drain_exp(dt.fighter->unit, 90);
            ++drained;
            killed += terminate(dt, at, AT_COMBATSPELL, damage, false);
        }
        --force;
    }

    m =
        msg_message("cast_drainlife_effect", "mage spell amount", fi->unit, sp,
        drained);
    message_all(b, m);
    msg_release(m);
    return level;
}

/* ------------------------------------------------------------- */
/* PRECOMBAT */

static fighter *summon_allies(const fighter *fi, const race *rc, int number) {
    attrib *a;
    unit *mage = fi->unit;
    side *si = fi->side;
    battle *b = si->battle;
    region *r = b->region;
    message *msg;
    unit *u =
        create_unit(r, mage->faction, number, rc, 0, NULL, mage);
    leave(u, true);
    setstatus(u, ST_FIGHT);
    
    u->hp = u->number * unit_max_hp(u);
    
    if (mage->flags & UFL_ANON_FACTION) {
        u->flags |= UFL_ANON_FACTION;
    }
    
    a = a_new(&at_unitdissolve);
    a->data.ca[0] = 0;
    a->data.ca[1] = 100;
    a_add(&u->attribs, a);
    
    msg = msg_message("sp_wolfhowl_effect", "mage amount race", mage, u->number, rc);
    message_all(b, msg);
    msg_release(msg);

    return make_fighter(b, u, si, is_attacker(fi));
}

int sp_igjarjuk(castorder *co) {
    unit *u;
    fighter *fm = co->magician.fig, *fi;
    const race *rc = get_race(RC_WYRM);
    fi = summon_allies(fm, rc, 1);
    u = fi->unit;
    unit_setname(u, "Igjarjuk");
    log_info("%s summons Igjarjuk in %s", unitname(fm->unit), regionname(u->region, 0));
    return co->level;
}

int sp_wolfhowl(castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    double power = co->force;
    int force = (int)(get_force(power, 3) / 2);
    const race * rc = get_race(RC_WOLF);
    if (force > 0) {
        unit *u;
        int skills = (int)(power/3);
        fi = summon_allies(fi, rc, force);
        u = fi->unit;
        set_level(u, SK_WEAPONLESS, skills);
        set_level(u, SK_STAMINA, skills);
        u->hp = u->number * unit_max_hp(u);
    }
    return level;
}

int sp_shadowknights(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    double power = co->force;
    unit *u;
    battle *b = fi->side->battle;
    region *r = b->region;
    unit *mage = fi->unit;
    attrib *a;
    int force = MAX(1, (int)get_force(power, 3));
    message *msg;

    u =
        create_unit(r, mage->faction, force, get_race(RC_SHADOWKNIGHT), 0, NULL,
        mage);
    setstatus(u, ST_FIGHT);

    u->hp = u->number * unit_max_hp(u);

    if (mage->flags & UFL_ANON_FACTION) {
        u->flags |= UFL_ANON_FACTION;
    }

    a = a_new(&at_unitdissolve);
    a->data.ca[0] = 0;
    a->data.ca[1] = 100;
    a_add(&u->attribs, a);

    make_fighter(b, u, fi->side, is_attacker(fi));

    msg = msg_message("sp_shadowknights_effect", "mage", mage);
    message_all(b, msg);
    msg_release(msg);

    return level;
}

int sp_strong_wall(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    double power = co->force;
    battle *b = fi->side->battle;
    unit *mage = fi->unit;
    building *burg;
    double effect;
    message *msg;

    if (!mage->building) {
        return 0;
    }
    burg = mage->building;

    effect = power / 4;
    create_curse(mage, &burg->attribs, ct_find("strongwall"), power, 1, effect, 0);

    msg =
        msg_message("sp_strongwalls_effect", "mage building", mage, mage->building);
    message_all(b, msg);
    msg_release(msg);

    return level;
}

static bool select_alive(const side *vs, const fighter *fig, void *cbdata)
{
    UNUSED_ARG(vs);
    UNUSED_ARG(cbdata);
    return fig->alive > 0;
}

/** Spells: chaosrow / song of confusion.
 * German Title: 'Gesang der Verwirrung'
 */
int sp_chaosrow(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    double power = co->force;
    const spell * sp = co->sp;
    battle *b = fi->side->battle;
    unit *mage = fi->unit;
    selist *fgs, *ql;
    message *m;
    const char *mtype;
    int qi, k = 0;
    bool chaosrow = strcmp(sp->sname, "chaosrow") == 0;

    if (!count_enemies(b, fi, FIGHT_ROW, NUMROWS, SELECT_ADVANCE | SELECT_FIND)) {
        m = msg_message("battle::out_of_range", "mage spell", fi->unit, sp);
        message_all(b, m);
        msg_release(m);
        return 0;
    }

    power = chaosrow ? (power * 40) : get_force(power, 5);

    fgs = select_fighters(b, fi->side, FS_ENEMY, select_alive, NULL);
    scramble_fighters(fgs);

    for (qi = 0, ql = fgs; ql; selist_advance(&ql, &qi, 1)) {
        fighter *df = (fighter *)selist_get(ql, qi);
        int n = df->unit->number;

        if (df->alive == 0)
            continue;
        if (power <= 0.0)
            break;
        /* force sollte wegen des MAX(0,x) nicht unter 0 fallen k�nnen */

        if (is_magic_resistant(mage, df->unit, 0))
            continue;

        if (chance(power / n)) {
            int row = statusrow(df->status);
            df->side->size[row] -= df->alive;
            if (u_race(df->unit)->battle_flags & BF_NOBLOCK) {
                df->side->nonblockers[row] -= df->alive;
            }
            row = FIRST_ROW + (rng_int() % (NUMROWS - FIRST_ROW));
            switch (row) {
            case FIGHT_ROW:
                df->status = ST_FIGHT;
                break;
            case BEHIND_ROW:
                df->status = ST_CHICKEN;
                break;
            case AVOID_ROW:
                df->status = ST_AVOID;
                break;
            case FLEE_ROW:
                df->status = ST_FLEE;
                break;
            default:
                assert(!"unknown combatrow");
            }
            assert(statusrow(df->status) == row);
            df->side->size[row] += df->alive;
            if (u_race(df->unit)->battle_flags & BF_NOBLOCK) {
                df->side->nonblockers[row] += df->alive;
            }
            k += df->alive;
        }
        power = MAX(0, power - n);
    }
    selist_free(fgs);

    if (chaosrow) {
        mtype = (k > 0) ? "sp_chaosrow_effect_1" : "sp_chaosrow_effect_0";
    }
    else {
        mtype = (k > 0) ? "sp_confusion_effect_1" : "sp_confusion_effect_0";
    }
    m = msg_message(mtype, "mage", mage);
    message_all(b, m);
    msg_release(m);
    return level;
}

static bool select_afraid(const side *vs, const fighter *fig, void *cbdata)
{
    int row = get_unitrow(fig, vs);
    UNUSED_ARG(cbdata);
    if (row >= FIGHT_ROW && row <= AVOID_ROW) {
        return fig->alive + fig->run.number < fig->unit->number;
    }
    return false;
}

/* Gesang der Furcht (Kampfzauber) */
/* Panik (Pr�kampfzauber) */
int flee_spell(struct castorder * co, int strength)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    const spell * sp = co->sp;
    battle *b = fi->side->battle;
    unit *mage = fi->unit;
    selist *fgs, *ql;
    int n, qi, panik = 0;
    message *msg;
    double power = co->force;
    int force;

    force = (int)get_force(power, strength);
    if (force<=0 || !count_enemies(b, fi, FIGHT_ROW, AVOID_ROW, SELECT_ADVANCE | SELECT_FIND)) {
        msg = msg_message("sp_flee_effect_0", "mage spell", mage, sp);
        message_all(b, msg);
        msg_release(msg);
        return 0;
    }

    fgs = select_fighters(b, fi->side, FS_ENEMY, select_afraid, NULL);
    scramble_fighters(fgs);

    for (qi = 0, ql = fgs; force > 0 && ql; selist_advance(&ql, &qi, 1)) {
        fighter *df = (fighter *)selist_get(ql, qi);

        for (n = 0; force > 0 && n != df->alive; ++n) {
            if (df->person[n].flags & FL_PANICED) {   /* bei SPL_SONG_OF_FEAR m�glich */
                df->person[n].attack -= 1;
                --force;
                ++panik;
            }
            else if (!(df->person[n].flags & FL_COURAGE)
                || !(u_race(df->unit)->flags & RCF_UNDEAD)) {
                if (!is_magic_resistant(mage, df->unit, 0)) {
                    df->person[n].flags |= FL_PANICED;
                    ++panik;
                }
                --force;
            }
        }
    }
    selist_free(fgs);

    msg = msg_message("sp_flee_effect_1", "mage spell amount", mage, sp, panik);
    message_all(b, msg);
    msg_release(msg);

    return level;
}

/* Heldenmut */
int sp_hero(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    double power = co->force;
    const spell * sp = co->sp;
    battle *b = fi->side->battle;
    int df_bonus = 0;
    int force = 0;
    int allies;
    int targets = 0;
    message *m;

    df_bonus = (int)(power / 5);
    force = MAX(1, lovar(get_force(power, 4)));

    allies =
        count_allies(fi->side, FIGHT_ROW, BEHIND_ROW, SELECT_ADVANCE, ALLY_ANY);
    /* maximal 2*allies Versuche ein Opfer zu finden, ansonsten best�nde
     * die Gefahr eine Endlosschleife*/
    allies *= 2;

    while (force && allies) {
        troop dt = select_ally(fi, FIGHT_ROW, BEHIND_ROW, ALLY_ANY);
        fighter *df = dt.fighter;
        --allies;

        if (df) {
            if (!(df->person[dt.index].flags & FL_COURAGE)) {
                df->person[dt.index].defence += df_bonus;
                df->person[dt.index].flags = df->person[dt.index].flags | FL_COURAGE;
                targets++;
                --force;
            }
        }
    }

    m =
        msg_message("cast_hero_effect", "mage spell amount", fi->unit, sp, targets);
    message_all(b, m);
    msg_release(m);

    return level;
}

int sp_berserk(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    double power = co->force;
    const spell * sp = co->sp;
    battle *b = fi->side->battle;
    int at_bonus = 0;
    int df_malus = 0;
    int force = 0;
    int allies = 0;
    int targets = 0;
    message *m;

    at_bonus = MAX(1, level / 3);
    df_malus = 2;
    force = (int)get_force(power, 2);

    allies =
        count_allies(fi->side, FIGHT_ROW, BEHIND_ROW - 1, SELECT_ADVANCE, ALLY_ANY);
    /* maximal 2*allies Versuche ein Opfer zu finden, ansonsten best�nde
     * die Gefahr eine Endlosschleife*/
    allies *= 2;

    while (force && allies) {
        troop dt = select_ally(fi, FIGHT_ROW, BEHIND_ROW - 1, ALLY_ANY);
        fighter *df = dt.fighter;
        --allies;

        if (df) {
            if (!(df->person[dt.index].flags & FL_COURAGE)) {
                df->person[dt.index].attack += at_bonus;
                df->person[dt.index].defence -= df_malus;
                df->person[dt.index].flags = df->person[dt.index].flags | FL_COURAGE;
                targets++;
                --force;
            }
        }
    }

    m =
        msg_message("cast_berserk_effect", "mage spell amount", fi->unit, sp,
        targets);
    message_all(b, m);
    msg_release(m);
    return level;
}

int sp_frighten(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    double power = co->force;
    const spell * sp = co->sp;
    battle *b = fi->side->battle;
    unit *mage = fi->unit;
    int at_malus = 0;
    int df_malus = 0;
    int force = 0;
    int enemies = 0;
    int targets = 0;
    message *m;

    at_malus = MAX(1, level - 4);
    df_malus = 2;
    force = (int)get_force(power, 2);

    enemies = count_enemies(b, fi, FIGHT_ROW, BEHIND_ROW - 1, SELECT_ADVANCE);
    if (!enemies) {
        message *m =
            msg_message("battle::out_of_range", "mage spell", fi->unit, sp);
        message_all(b, m);
        msg_release(m);
        return 0;
    }

    while (force && enemies) {
        troop dt = select_enemy(fi, FIGHT_ROW, BEHIND_ROW - 1, SELECT_ADVANCE);
        fighter *df = dt.fighter;
        --enemies;

        if (!df)
            break;

        assert(!helping(fi->side, df->side));

        if (df->person[dt.index].flags & FL_COURAGE) {
            df->person[dt.index].flags &= ~(FL_COURAGE);
        }
        if (!is_magic_resistant(mage, df->unit, 0)) {
            df->person[dt.index].attack -= at_malus;
            df->person[dt.index].defence -= df_malus;
            targets++;
        }
        --force;
    }

    m =
        msg_message("cast_frighten_effect", "mage spell amount", fi->unit, sp,
        targets);
    message_all(b, m);
    msg_release(m);
    return level;
}

int sp_tiredsoldiers(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    double power = co->force;
    const spell * sp = co->sp;
    battle *b = fi->side->battle;
    unit *mage = fi->unit;
    int n = 0;
    int force = (int)(power * power * 4);
    message *m;

    if (!count_enemies(b, fi, FIGHT_ROW, BEHIND_ROW,
        SELECT_ADVANCE | SELECT_FIND)) {
        message *m =
            msg_message("battle::out_of_range", "mage spell", fi->unit, sp);
        message_all(b, m);
        msg_release(m);
        return 0;
    }

    while (force) {
        troop t = select_enemy(fi, FIGHT_ROW, BEHIND_ROW, SELECT_ADVANCE);
        fighter *df = t.fighter;

        if (!df)
            break;

        assert(!helping(fi->side, df->side));
        if (!(df->person[t.index].flags & FL_TIRED)) {
            if (!is_magic_resistant(mage, df->unit, 0)) {
                df->person[t.index].flags = df->person[t.index].flags | FL_TIRED;
                df->person[t.index].defence -= 2;
                ++n;
            }
        }
        --force;
    }

    m = msg_message("cast_tired_effect", "mage spell amount", fi->unit, sp, n);
    message_all(b, m);
    msg_release(m);
    return level;
}

int sp_windshield(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    double power = co->force;
    const spell * sp = co->sp;
    battle *b = fi->side->battle;
    int force, at_malus;
    int enemies;
    message *m;

    force = (int)get_force(power, 4);
    at_malus = level / 4;

    enemies = count_enemies(b, fi, BEHIND_ROW, BEHIND_ROW, SELECT_ADVANCE);
    if (!enemies) {
        m = msg_message("battle::out_of_range", "mage spell", fi->unit, sp);
        message_all(b, m);
        msg_release(m);
        return 0;
    }

    while (force && enemies) {
        troop dt = select_enemy(fi, BEHIND_ROW, BEHIND_ROW, SELECT_ADVANCE);
        fighter *df = dt.fighter;
        --enemies;

        if (!df)
            break;
        assert(!helping(fi->side, df->side));

        if (df->person[dt.index].missile) {
            /* this suxx... affects your melee weapon as well. */
            df->person[dt.index].attack -= at_malus;
            --force;
        }
    }

    m = msg_message("cast_storm_effect", "mage spell", fi->unit, sp);
    message_all(b, m);
    msg_release(m);
    return level;
}

int sp_reeling_arrows(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    const spell * sp = co->sp;
    battle *b = fi->side->battle;
    message *m;

    b->reelarrow = true;
    m = msg_message("cast_spell_effect", "mage spell", fi->unit, sp);
    message_all(b, m);
    msg_release(m);
    return level;
}

/* Magier weicht dem Kampf aus. Wenn er sich bewegen kann, zieht er in
 * eine Nachbarregion, wobei ein NACH ber�cksichtigt wird. Ansonsten
 * bleibt er stehen und nimmt nicht weiter am Kampf teil. */
int sp_denyattack(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;

    const spell * sp = co->sp;
    battle *b = fi->side->battle;
    unit *mage = fi->unit;
    region *r = b->region;
    message *m;

    /* Fliehende Einheiten verlassen auf jeden Fall Geb�ude und Schiffe. */
    if (!(r->terrain->flags & SEA_REGION)) {
        leave(mage, false);
    }
    /* und bewachen nicht */
    setguard(mage, false);
    /* irgendwie den langen befehl sperren */

    /* wir tun so, als w�re die Person geflohen */
    fi->flags |= FIG_NOLOOT;
    fi->run.hp = mage->hp;
    fi->run.number = mage->number;
    /* fighter leeren */
    rmfighter(fi, mage->number);

    m = msg_message("cast_escape_effect", "mage spell", fi->unit, sp);
    message_all(b, m);
    msg_release(m);

    return level;
}

static void do_meffect(fighter * af, int typ, int effect, int duration)
{
    battle *b = af->side->battle;
    meffect *me = (meffect *)malloc(sizeof(struct meffect));
    selist_push(&b->meffects, me);
    me->magician = af;
    me->typ = typ;
    me->effect = effect;
    me->duration = duration;
}

int armor_spell(struct castorder * co, int per_level, int time_multi)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    double power = co->force;
    const spell * sp = co->sp;
    int effect;
    int duration;
    battle *b = fi->side->battle;
    message *m = msg_message("cast_spell_effect", "mage spell", fi->unit, sp);

    message_all(b, m);
    msg_release(m);

    /* gibt R�stung +effect f�r duration Treffer */

    effect = level / per_level;
    duration = (int)(time_multi * power * power);
    do_meffect(fi, SHIELD_ARMOR, effect, duration);
    return level;
}

int sp_reduceshield(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    double power = co->force;
    const spell * sp = co->sp;
    int effect;
    int duration;
    battle *b = fi->side->battle;
    message *m = msg_message("cast_spell_effect", "mage spell", fi->unit, sp);
    message_all(b, m);
    msg_release(m);

    /* jeder Schaden wird um effect% reduziert bis der Schild duration
     * Trefferpunkte aufgefangen hat */

    effect = 50;
    duration = (int)(50 * power * power);

    do_meffect(fi, SHIELD_REDUCE, effect, duration);
    return level;
}

int sp_fumbleshield(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    const spell * sp = co->sp;
    int effect;
    int duration;
    battle *b = fi->side->battle;
    message *m = msg_message("cast_spell_effect", "mage spell", fi->unit, sp);

    message_all(b, m);
    msg_release(m);

    /* der erste Zauber schl�gt mit 100% fehl  */
    duration = 100;
    effect = MAX(1, 25 - level);

    do_meffect(fi, SHIELD_BLOCK, effect, duration);
    return level;
}

/* ------------------------------------------------------------- */
/* POSTCOMBAT */

static int count_healable(battle * b, fighter * df)
{
    side *s;
    int healable = 0;

    for (s = b->sides; s != b->sides + b->nsides; ++s) {
        if (helping(df->side, s)) {
            healable += s->casualties;
        }
    }
    return healable;
}
static bool has_ao_healing(const unit *u) {
    item *const* iter = i_findc(&u->items, it_find("ao_healing"));
    return (*iter && (*iter)->number > 0);
}

/* wiederbeleben */
int sp_reanimate(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    double power = co->force;
    battle *b = fi->side->battle;
    unit *mage = fi->unit;
    int healable, j = 0;
    double c = 0.50 + 0.02 * power;
    double k = EFFECT_HEALING_SPELL * power;
    bool use_item = has_ao_healing(mage);
    message *msg;

    if (use_item) {
        k *= 2;
        c += 0.10;
    }

    healable = count_healable(b, fi);
    healable = (int)MIN(k, healable);
    while (healable--) {
        fighter *tf = select_corpse(b, fi);
        if (tf != NULL && tf->side->casualties > 0
            && u_race(tf->unit) != get_race(RC_DAEMON)
            && (chance(c))) {
            assert(tf->alive < tf->unit->number);
            /* t.fighter->person[].hp beginnt mit t.index = 0 zu z�hlen,
             * t.fighter->alive ist jedoch die Anzahl lebender in der Einheit,
             * also sind die hp von t.fighter->alive
             * t.fighter->hitpoints[t.fighter->alive-1] und der erste Tote
             * oder weggelaufene ist t.fighter->hitpoints[tf->alive] */
            tf->person[tf->alive].hp = 2;
            ++tf->alive;
            ++tf->side->size[SUM_ROW];
            ++tf->side->size[tf->unit->status + 1];
            ++tf->side->healed;
            --tf->side->casualties;
            assert(tf->side->casualties >= 0);
            --tf->side->dead;
            assert(tf->side->dead >= 0);
            ++j;
        }
    }
    if (j <= 0) {
        level = j;
    }
    if (use_item) {
        msg =
            msg_message("reanimate_effect_1", "mage amount item", mage, j,
            get_resourcetype(R_AMULET_OF_HEALING));
    }
    else {
        msg = msg_message("reanimate_effect_0", "mage amount", mage, j);
    }
    message_all(b, msg);
    msg_release(msg);

    return level;
}

int sp_keeploot(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    double power = co->force;
    const spell * sp = co->sp;
    battle *b = fi->side->battle;
    message *m = msg_message("cast_spell_effect", "mage spell", fi->unit, sp);

    message_all(b, m);
    msg_release(m);

    b->keeploot = (int)MAX(25, b->keeploot + 5 * power);

    return level;
}

static int heal_fighters(selist * fgs, int *power, bool heal_monsters)
{
    int healhp = *power, healed = 0, qi;
    selist *ql;

    for (qi = 0, ql = fgs; ql; selist_advance(&ql, &qi, 1)) {
        fighter *df = (fighter *)selist_get(ql, qi);

        if (healhp <= 0)
            break;

        /* Untote kann man nicht heilen */
        if (df->unit->number == 0 || (u_race(df->unit)->flags & RCF_NOHEAL))
            continue;

        /* wir heilen erstmal keine Monster */
        if (heal_monsters || playerrace(u_race(df->unit))) {
            int n, hp = df->unit->hp / df->unit->number;
            int rest = df->unit->hp % df->unit->number;

            for (n = 0; n < df->unit->number; n++) {
                int wound = hp - df->person[n].hp;
                if (rest > n)
                    ++wound;

                if (wound > 0 && wound < hp) {
                    int heal = MIN(healhp, wound);
                    assert(heal >= 0);
                    df->person[n].hp += heal;
                    healhp = MAX(0, healhp - heal);
                    ++healed;
                    if (healhp <= 0)
                        break;
                }
            }
        }
    }

    *power = healhp;
    return healed;
}

int sp_healing(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    double power = co->force;
    battle *b = fi->side->battle;
    unit *mage = fi->unit;
    int j = 0;
    int healhp = (int)power * 200;
    selist *fgs;
    message *msg;
    bool use_item = has_ao_healing(mage);

    /* bis zu 11 Personen pro Stufe (einen HP muessen sie ja noch
     * haben, sonst waeren sie tot) koennen geheilt werden */

    if (use_item) {
        healhp *= 2;
    }

    /* gehe alle denen wir helfen der reihe nach durch, heile verwundete,
     * bis zu verteilende HP aufgebraucht sind */

    fgs = fighters(b, fi->side, FIGHT_ROW, AVOID_ROW, FS_HELP);
    scramble_fighters(fgs);
    j += heal_fighters(fgs, &healhp, false);
    j += heal_fighters(fgs, &healhp, true);
    selist_free(fgs);

    if (j <= 0) {
        level = j;
    }
    if (use_item) {
        msg =
            msg_message("healing_effect_1", "mage amount item", mage, j,
            get_resourcetype(R_AMULET_OF_HEALING));
    }
    else {
        msg = msg_message("healing_effect_0", "mage amount", mage, j);
    }
    message_all(b, msg);
    msg_release(msg);

    return level;
}

static bool select_hero(const side *vs, const fighter *fig, void *cbdata)
{
    UNUSED_ARG(cbdata);

    if (playerrace(u_race(fig->unit))) {
        int row = get_unitrow(fig, vs);
        if (row >= FIGHT_ROW && row <= AVOID_ROW) {
            return fig->alive + fig->run.number < fig->unit->number;
        }
    }
    return false;
}

int sp_undeadhero(struct castorder * co)
{
    fighter * fi = co->magician.fig;
    int level = co->level;
    double power = co->force;
    battle *b = fi->side->battle;
    unit *mage = fi->unit;
    region *r = b->region;
    selist *fgs, *ql;
    int qi, n, undead = 0;
    message *msg;
    int force = (int)get_force(power, 0);
    double c = 0.50 + 0.02 * power;

    /* Liste aus allen Kaempfern */
    fgs = select_fighters(b, fi->side, FS_ENEMY | FS_HELP, select_hero, NULL);
    scramble_fighters(fgs);

    for (qi = 0, ql = fgs; ql && force>0; selist_advance(&ql, &qi, 1)) {
        fighter *df = (fighter *)selist_get(ql, qi);
        unit *du = df->unit;
        int j = 0;

        /* Wieviele Untote koennen wir aus dieser Einheit wecken? */
        for (n = df->alive + df->run.number; force>0 && n != du->number; n++) {
            if (chance(c)) {
                ++j;
                --force;
            }
        }

        if (j > 0) {
            item **ilist;
            unit *u =
                create_unit(r, mage->faction, 0, get_race(RC_UNDEAD), 0, unit_getname(du),
                du);

            /* new units gets some stats from old unit */

            if (du->display) {
                unit_setinfo(u, du->display);
            }
            else {
                unit_setinfo(u, NULL);
            }
            setstatus(u, du->status);
            setguard(u, false);
            for (ilist = &du->items; *ilist;) {
                item *itm = *ilist;
                int loot = itm->number * j / du->number;
                if (loot != itm->number) {
                    int split = itm->number * j % du->number;
                    if (split > 0 && (rng_int() % du->number) < split) {
                        ++loot;
                    }
                }
                i_change(&u->items, itm->type, loot);
                i_change(ilist, itm->type, -loot);
                if (*ilist == itm) {
                    ilist = &itm->next;
                }
            }

            /* inherit stealth from magician */
            if (mage->flags & UFL_ANON_FACTION) {
                u->flags |= UFL_ANON_FACTION;
            }

            /* transfer dead people to new unit, set hitpoints to those of old unit */
            transfermen(du, u, j);
            u->hp = u->number * unit_max_hp(du);
            assert(j <= df->side->casualties);
            df->side->casualties -= j;
            df->side->dead -= j;

            /* counting total number of undead */
            undead += j;
        }
    }
    selist_free(fgs);

    level = MIN(level, undead);
    if (undead == 0) {
        msg =
            msg_message("summonundead_effect_0", "mage region", mage, mage->region);
    }
    else {
        msg =
            msg_message("summonundead_effect_1", "mage region amount", mage,
            mage->region, undead);
    }

    message_all(b, msg);
    msg_release(msg);
    return level;
}
