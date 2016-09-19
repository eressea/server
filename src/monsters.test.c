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
#include "monsters.h"
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
    fset(rc, RCF_UNARMEDGUARD|RCF_NPC|RCF_DRAGON);
    fset(*monsters, FFL_NOIDLEOUT);
    assert(fval(*monsters, FFL_NPC) && fval((*monsters)->race, RCF_UNARMEDGUARD) && fval((*monsters)->race, RCF_NPC) && fval(*monsters, FFL_NOIDLEOUT));

    test_create_region(-1, 0, test_create_terrain("ocean", SEA_REGION | SWIM_INTO | FLY_INTO));
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

extern void random_growl(const unit *u, region *tr, int rand);

static void test_dragon_moves(CuTest * tc)
{
    faction *f, *f2;
    region *r;
    unit *u, *m;
    struct message *msg;

    create_monsters(&f, &f2, &u, &m);
    rsetmoney(findregion(1, 0), 1000);
    r = findregion(0, 0); // plain
    rsetpeasants(r, 0);
    rsetmoney(r, 0);

    set_level(m, SK_WEAPONLESS, 10);
    config_set("rules.monsters.attack_chance", ".0");
    plan_monsters(f2);

    CuAssertPtrNotNull(tc, find_order("move east", m));

    mt_register(mt_new_va("dragon_growl", "dragon:unit", "number:int", "target:region", "growl:string", 0));

    random_growl(m, findregion(1, 0), 3);

    msg = test_get_last_message(r->msgs);
    assert_message(tc, msg, "dragon_growl", 4);
    assert_pointer_parameter(tc, msg, 0, m);
    assert_int_parameter(tc, msg, 1, 1);
    assert_pointer_parameter(tc, msg, 2, findregion(1,0));
    assert_string_parameter(tc, msg, 3, "growl3");

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

static void test_spawn_seaserpent(CuTest *tc) {
    region *r;
    unit *u;
    faction *f;
    race *rc;
    test_cleanup();
    rc = test_create_race("seaserpent");
    rc->flags |= RCF_NPC;
    r = test_create_region(0, 0, 0);
    f = test_create_faction(0);
    u = spawn_seaserpent(r, f);
    CuAssertPtrNotNull(tc, u);
    CuAssertPtrEquals(tc, 0, u->_name);
    test_cleanup();
}

CuSuite *get_monsters_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_monsters_attack);
    SUITE_ADD_TEST(suite, test_spawn_seaserpent);
    SUITE_ADD_TEST(suite, test_monsters_attack_ocean);
    SUITE_ADD_TEST(suite, test_seaserpent_piracy);
    SUITE_ADD_TEST(suite, test_monsters_waiting);
    SUITE_ADD_TEST(suite, test_monsters_attack_not);
    SUITE_ADD_TEST(suite, test_dragon_attacks_the_rich);
    SUITE_ADD_TEST(suite, test_dragon_moves);
    SUITE_ADD_TEST(suite, test_monsters_learn_exp);
    return suite;
}
