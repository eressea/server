#include <platform.h>
#include <stdlib.h>

#include <kernel/config.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

#include "monsters.h"
#include "guard.h"
#include "reports.h"
#include "skill.h"
#include "study.h"

#include <util/attrib.h>
#include <util/language.h>
#include <util/message.h>
#include <util/nrmessage.h>

#include <attributes/hate.h>

#include <CuTest.h>
#include <tests.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

extern void plan_monsters(struct faction *f);
extern int monster_attacks(unit * monster, bool rich_only);

static order *find_order(const char *expected, const unit *u)
{
    char cmd[32];
    order *ord;
    for (ord = u->orders; ord; ord = ord->next) {
        if (strcmp(expected, get_command(ord, u->faction->locale, cmd, sizeof(cmd))) == 0) {
            return ord;
        }
    }
    return NULL;
}

static void create_monsters(unit **up, unit **um) {
    race* rc;
    region *r;
    faction *fp, *fm;

    mt_create_va(mt_new("dragon_growl", NULL),
        "dragon:unit", "number:int", "target:region", "growl:string", MT_NEW_END);
    test_create_horse();
    default_locale = test_create_locale();
    fp = test_create_faction(NULL);

    fm = get_or_create_monsters();
    fset(fm, FFL_NOIDLEOUT);
    assert(fval(fm, FFL_NPC));
    assert(fval(fm, FFL_NOIDLEOUT));

    assert(rc_find(fm->race->_name));
    rc = rc_get_or_create(fm->race->_name);
    fset(rc, RCF_UNARMEDGUARD|RCF_DRAGON);
    assert(!fval(fm->race, RCF_PLAYABLE));

    test_create_region(-1, 0, test_create_terrain("ocean", SEA_REGION | SWIM_INTO | FLY_INTO));
    test_create_region(1, 0, 0);
    r = test_create_region(0, 0, NULL);

    *up = test_create_unit(fp, r);
    unit_setid(*up, 1);
    *um = test_create_unit(fm, r);
    unit_setstatus(*um, ST_FIGHT);
}

static void test_monsters_attack(CuTest * tc)
{
    unit *u, *m;

    test_setup();
    create_monsters(&u, &m);
    setguard(m, true);

    config_set("rules.monsters.attack_chance", "1");

    plan_monsters(m->faction);

    CuAssertPtrNotNull(tc, find_order("attack 1", m));
    test_teardown();
}

static void test_monsters_attack_ocean(CuTest * tc)
{
    region *r;
    unit *u, *m;

    test_setup();
    create_monsters(&u, &m);
    r = findregion(-1, 0); /* ocean */
    u = test_create_unit(u->faction, r);
    unit_setid(u, 2);
    m = test_create_unit(m->faction, r);
    assert(!m->region->land);

    config_set("rules.monsters.attack_chance", "1");

    plan_monsters(m->faction);

    CuAssertPtrNotNull(tc, find_order("attack 2", m));
    test_teardown();
}

static void test_monsters_waiting(CuTest * tc)
{
    unit *u, *m;

    test_setup();
    create_monsters(&u, &m);
    setguard(m, true);
    fset(m, UFL_ISNEW);
    monster_attacks(m, false);
    CuAssertPtrEquals(tc, 0, find_order("attack 1", m));
    test_teardown();
}

static void test_seaserpent_piracy(CuTest * tc)
{
    region *r;
    unit *u, *m;
    race *rc;
    ship_type * stype;

    test_setup();
    create_monsters(&u, &m);
    stype = test_create_shiptype("yacht");
    r = findregion(-1, 0); /* ocean */
    u = test_create_unit(u->faction, r);
    u->ship = test_create_ship(r, stype);
    unit_setid(u, 2);
    m = test_create_unit(m->faction, r);
    u_setrace(m, rc = test_create_race("seaserpent"));
    assert(!m->region->land);
    fset(m, UFL_MOVED);
    fset(rc, RCF_ATTACK_MOVED);

    config_set("rules.monsters.attack_chance", "1");

    stype->cargo = 50000;
    plan_monsters(m->faction);
    CuAssertPtrNotNull(tc, find_order("piracy", m));
    CuAssertPtrEquals(tc, NULL, find_order("attack 2", m));
    stype->cargo = 50001;
    plan_monsters(m->faction);
    CuAssertPtrNotNull(tc, find_order("attack 2", m));
    test_teardown();
}

static void test_monsters_attack_not(CuTest * tc)
{
    unit *u, *m;

    test_setup();
    create_monsters(&u, &m);

    setguard(m, true);
    setguard(u, true);

    config_set("rules.monsters.attack_chance", "0");

    plan_monsters(m->faction);

    CuAssertPtrEquals(tc, 0, find_order("attack 1", m));
    test_teardown();
}

static void test_dragon_attacks_the_rich(CuTest * tc)
{
    unit *u, *m;
    const item_type *i_silver;

    test_setup();
    create_monsters(&u, &m);
    init_resources();

    setguard(m, true);
    set_level(m, SK_WEAPONLESS, 10);

    rsetmoney(findregion(0, 0), 1);
    rsetmoney(findregion(1, 0), 0);
    i_silver = it_find("money");
    assert(i_silver);
    i_change(&u->items, i_silver, 5000);

    config_set("rules.monsters.attack_chance", "0.00001");

    plan_monsters(m->faction);
    CuAssertPtrNotNull(tc, find_order("attack 1", m));
    CuAssertPtrNotNull(tc, find_order("loot", m));
    test_teardown();
}

extern void random_growl(const unit *u, region *tr, int rand);

static void test_dragon_moves(CuTest * tc)
{
    region *r;
    unit *u, *m;
    struct message *msg;

    test_setup();
    create_monsters(&u, &m);
    rsetmoney(findregion(1, 0), 1000);
    r = findregion(0, 0); /* plain */
    rsetpeasants(r, 0);
    rsetmoney(r, 0);

    set_level(m, SK_WEAPONLESS, 10);
    config_set("rules.monsters.attack_chance", ".0");

    plan_monsters(m->faction);
    CuAssertPtrNotNull(tc, find_order("move east", m));

    random_growl(m, findregion(1, 0), 3);

    msg = test_get_last_message(r->msgs);
    assert_message(tc, msg, "dragon_growl", 4);
    assert_pointer_parameter(tc, msg, 0, m);
    assert_int_parameter(tc, msg, 1, 1);
    assert_pointer_parameter(tc, msg, 2, findregion(1,0));
    assert_string_parameter(tc, msg, 3, "growl3");

    test_teardown();
}

static void test_monsters_learn_exp(CuTest * tc)
{
    unit *u, *m;
    skill* sk;

    test_setup();
    create_monsters(&u, &m);
    config_set("study.produceexp", "30");

    u_setrace(u, u_race(m));
    produceexp(u, SK_MELEE, u->number);
    sk = unit_skill(u, SK_MELEE);
    CuAssertTrue(tc, !sk);

    produceexp(m, SK_MELEE, u->number);
    sk = unit_skill(m, SK_MELEE);
    CuAssertTrue(tc, sk && (sk->level > 0 || (sk->level == 0 && sk->weeks > 0)));

    test_teardown();
}

static void test_spawn_seaserpent(CuTest *tc) {
    region *r;
    unit *u;
    faction *f;
    race *rc;
    test_setup();
    rc = test_create_race("seaserpent");
    rc->flags &= ~RCF_PLAYABLE;
    r = test_create_region(0, 0, NULL);
    f = test_create_faction(NULL);
    u = spawn_seaserpent(r, f);
    CuAssertPtrNotNull(tc, u);
    CuAssertPtrEquals(tc, NULL, u->_name);
    test_teardown();
}

static void test_monsters_hate(CuTest *tc) {
    unit *mu, *tu;
    order *ord;
    char buffer[32];
    const struct locale *lang;

    test_setup();
    tu = test_create_unit(test_create_faction(NULL), test_create_plain(1, 0));
    mu = test_create_unit(get_monsters(), test_create_plain(0, 0));
    lang = mu->faction->locale;
    a_add(&mu->attribs, make_hate(tu));
    plan_monsters(mu->faction);
    CuAssertPtrNotNull(tc, mu->orders);
    for (ord = mu->orders; ord; ord = ord->next) {
        if (K_MOVE == getkeyword(ord)) {
            break;
        }
    }
    CuAssertPtrNotNull(tc, ord);
    CuAssertIntEquals(tc, K_MOVE, getkeyword(ord));
    CuAssertStrEquals(tc, "move east", get_command(ord, lang, buffer, sizeof(buffer)));
    test_teardown();
}

CuSuite *get_monsters_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_monsters_attack);
    SUITE_ADD_TEST(suite, test_monsters_hate);
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
