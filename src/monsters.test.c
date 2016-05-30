#include <platform.h>
#include <stdlib.h>

#include <kernel/config.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

#include "monster.h"
#include "guard.h"
#include "reports.h"
#include "skill.h"
#include "study.h"

#include <util/language.h>
#include <util/message.h>
#include <util/nrmessage.h>

#include <CuTest.h>
#include <tests.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

extern void plan_monsters(struct faction *f);
extern int monster_attacks(unit * monster, bool respect_buildings, bool rich_only);

static order *find_order(const char *expected, const unit *unit)
{
    char cmd[32];
    order *ord;
    for (ord = unit->orders; ord; ord = ord->next) {
        if (strcmp(expected, get_command(ord, cmd, sizeof(cmd))) == 0) {
            return ord;
        }
    }
    return NULL;
}

static void create_monsters(faction **player, faction **monsters, unit **u, unit **m) {
    race* rc;
    region *r;

    test_cleanup();

    test_create_horse();
    default_locale = test_create_locale();
    *player = test_create_faction(NULL);
    *monsters = get_or_create_monsters();
    assert(rc_find((*monsters)->race->_name));
    rc = rc_get_or_create((*monsters)->race->_name);
    fset(rc, RCF_UNARMEDGUARD);
    fset(rc, RCF_NPC);
    fset(*monsters, FFL_NOIDLEOUT);
    assert(fval(*monsters, FFL_NPC) && fval((*monsters)->race, RCF_UNARMEDGUARD) && fval((*monsters)->race, RCF_NPC) && fval(*monsters, FFL_NOIDLEOUT));

    test_create_region(-1, 0, test_create_terrain("ocean", SEA_REGION | SAIL_INTO | SWIM_INTO | FLY_INTO));
    test_create_region(1, 0, 0);
    r = test_create_region(0, 0, 0);

    *u = test_create_unit(*player, r);
    unit_setid(*u, 1);
    *m = test_create_unit(*monsters, r);
}

static void test_monsters_attack(CuTest * tc)
{
    faction *f, *f2;
    unit *u, *m;

    create_monsters(&f, &f2, &u, &m);

    guard(m, GUARD_TAX);

    config_set("rules.monsters.attack_chance", "1");

    plan_monsters(f2);

    CuAssertPtrNotNull(tc, find_order("attack 1", m));
    test_cleanup();
}

static void test_monsters_attack_ocean(CuTest * tc)
{
    faction *f, *f2;
    region *r;
    unit *u, *m;

    create_monsters(&f, &f2, &u, &m);
    r = findregion(-1, 0); // ocean
    u = test_create_unit(u->faction, r);
    unit_setid(u, 2);
    m = test_create_unit(m->faction, r);
    assert(!m->region->land);

    config_set("rules.monsters.attack_chance", "1");

    plan_monsters(f2);

    CuAssertPtrNotNull(tc, find_order("attack 2", m));
    test_cleanup();
}

static void test_monsters_waiting(CuTest * tc)
{
    faction *f, *f2;
    unit *u, *m;

    create_monsters(&f, &f2, &u, &m);
    guard(m, GUARD_TAX);
    fset(m, UFL_ISNEW);
    monster_attacks(m, false, false);
    CuAssertPtrEquals(tc, 0, find_order("attack 1", m));
    test_cleanup();
}

static void test_seaserpent_piracy(CuTest * tc)
{
    faction *f, *f2;
    region *r;
    unit *u, *m;
    race *rc;

    create_monsters(&f, &f2, &u, &m);
    r = findregion(-1, 0); // ocean
    u = test_create_unit(u->faction, r);
    unit_setid(u, 2);
    m = test_create_unit(m->faction, r);
    u_setrace(m, rc = test_create_race("seaserpent"));
    assert(!m->region->land);
    fset(m, UFL_MOVED);
    fset(rc, RCF_ATTACK_MOVED);

    config_set("rules.monsters.attack_chance", "1");

    plan_monsters(f2);
    CuAssertPtrNotNull(tc, find_order("piracy", m));
    CuAssertPtrNotNull(tc, find_order("attack 2", m));
    test_cleanup();
}

static void test_monsters_attack_not(CuTest * tc)
{
    faction *f, *f2;
    unit *u, *m;

    create_monsters(&f, &f2, &u, &m);

    guard(m, GUARD_TAX);
    guard(u, GUARD_TAX);

    config_set("rules.monsters.attack_chance", "0");

    plan_monsters(f2);

    CuAssertPtrEquals(tc, 0, find_order("attack 1", m));
    test_cleanup();
}

static void test_dragon_attacks_the_rich(CuTest * tc)
{
    faction *f, *f2;
    unit *u, *m;
    const item_type *i_silver;

    create_monsters(&f, &f2, &u, &m);
    init_resources();

    guard(m, GUARD_TAX);
    set_level(m, SK_WEAPONLESS, 10);

    rsetmoney(findregion(0, 0), 1);
    rsetmoney(findregion(1, 0), 0);
    i_silver = it_find("money");
    assert(i_silver);
    i_change(&u->items, i_silver, 5000);

    config_set("rules.monsters.attack_chance", "0.00001");

    plan_monsters(f2);

    CuAssertPtrNotNull(tc, find_order("attack 1", m));
    CuAssertPtrNotNull(tc, find_order("loot", m));
    test_cleanup();
}

typedef struct msg_info {
    const char *name;
    int numpar;
    const char **argv;
    const char **pnames, **tnames;
    variant *values;
} msg_info;

static msg_info *test_create_messagetype(const struct locale *lang, const char *name, int num_args, ...) {
    va_list vargs;
    const message_type *mtype;
    char zBuffer[128];
    int i;
    msg_info *info = malloc(sizeof(msg_info));
    info->argv = calloc(num_args + 1, sizeof(char *));
    info->pnames = calloc(num_args, sizeof(char *));
    info->tnames = calloc(num_args, sizeof(char *));
    info->values = calloc(num_args, sizeof(variant_type));

    va_start(vargs, num_args);

    info->numpar = num_args;
    info->name = name;

    for(i = 0; i<num_args; ++i) {
        info->pnames[i] = va_arg(vargs, char *);
        info->tnames[i] = va_arg(vargs, char *);
        sprintf(zBuffer, "%s:%s", info->pnames[i], info->tnames[i]);
        info->argv[i] = _strdup(zBuffer);
    }

    mtype = mt_register(mt_new(name, info->argv));
    nrt_register(mtype, lang, "mocktext", 0, "mocksection");

    for(i = 0; i<num_args; ++i) {
        if (mtype->types[i]->vtype == VAR_VOIDPTR) {
            info->values[i].v = va_arg(vargs, char *);
        } else if (mtype->types[i]->vtype == VAR_INT) {
            info->values[i].i = va_arg(vargs, int);
        } else {
            assert(!"unknown variant type");
        }
    }
    va_end(vargs);
    return info;
}

static void assert_message(CuTest * tc, message *msg, msg_info *info)
{
    const message_type *mtype = msg->type;
    assert(mtype);
    int i;

    CuAssertStrEquals(tc, info->name, mtype->name);
    CuAssertIntEquals(tc, info->numpar, mtype->nparameters);
    for (i = 0; i != mtype->nparameters; ++i) {
        CuAssertStrEquals(tc, info->pnames[i], mtype->pnames[i]);
        CuAssertStrEquals(tc, info->tnames[i], mtype->types[i]->name);
        if (mtype->types[i]->vtype == VAR_VOIDPTR) {
            CuAssertPtrEquals(tc, info->values[i].v, msg->parameters[i].v);
        } else if (mtype->types[i]->vtype == VAR_INT) {
            CuAssertIntEquals(tc, info->values[i].i, msg->parameters[i].i);
        } else {
            assert(!"unknown variant type");
        }
    }
}

extern void random_growl(const unit *u, region *tr, int rand);

static void test_dragon_moves(CuTest * tc)
{
    faction *f, *f2;
    region *r;
    unit *u, *m;
    struct message *msg;
    struct msg_info *msginfo;

    create_monsters(&f, &f2, &u, &m);
    rsetmoney(findregion(1, 0), 1000);
    r = findregion(0, 0); // plain
    rsetpeasants(r, 0);
    rsetmoney(r, 0);

    set_level(m, SK_WEAPONLESS, 10);
    config_set("rules.monsters.attack_chance", ".0");
    plan_monsters(f2);

    CuAssertPtrNotNull(tc, find_order("move east", m));

    register_reports();
    msginfo = test_create_messagetype(f->locale, "dragon_growl", 4, "dragon", "unit", "number", "int", "target", "region", "choice", "int", m, 1, findregion(1,0), 3);

    random_growl(m, findregion(1, 0), 3);

    msg = test_get_last_message(r->msgs);
    assert_message(tc, msg, msginfo);

    test_cleanup();
}

static void test_monsters_learn_exp(CuTest * tc)
{
    faction *f, *f2;
    unit *u, *m;
    skill* sk;

    create_monsters(&f, &f2, &u, &m);
    config_set("study.produceexp", "30");

    u_setrace(u, u_race(m));
    produceexp(u, SK_MELEE, u->number);
    sk = unit_skill(u, SK_MELEE);
    CuAssertTrue(tc, !sk);

    produceexp(m, SK_MELEE, u->number);
    sk = unit_skill(m, SK_MELEE);
    CuAssertTrue(tc, sk && (sk->level > 0 || (sk->level == 0 && sk->weeks > 0)));

    test_cleanup();
}

CuSuite *get_monsters_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_monsters_attack);
    SUITE_ADD_TEST(suite, test_monsters_attack_ocean);
    SUITE_ADD_TEST(suite, test_seaserpent_piracy);
    SUITE_ADD_TEST(suite, test_monsters_waiting);
    SUITE_ADD_TEST(suite, test_monsters_attack_not);
    SUITE_ADD_TEST(suite, test_dragon_attacks_the_rich);
    SUITE_ADD_TEST(suite, test_dragon_moves);
    SUITE_ADD_TEST(suite, test_monsters_learn_exp);
    return suite;
}
