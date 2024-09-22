#include "combatspells.h"

#include <tests.h>

#include <battle.h>
#include <magic.h>

#include <kernel/ally.h>
#include <kernel/faction.h>
#include <kernel/region.h>
#include <kernel/unit.h>

#include <util/rand.h>

#include <CuTest.h>

#include <stddef.h>          // for NULL

static void test_immolation(CuTest *tc)
{
    castorder co;
    unit *du, *au;
    region *r;
    battle *b;
    side *ds, *as;
    fighter *df, *af;

    test_setup();
    random_source_inject_constants(0.0, 3);
    r = test_create_plain(0, 0);
    du = test_create_unit(test_create_faction(), r);
    unit_setstatus(du, ST_FIGHT);
    au = test_create_unit(test_create_faction(), r);
    unit_setstatus(au, ST_BEHIND);
    b = make_battle(r);
    ds = make_side(b, du->faction, 0, 0, 0);
    df = make_fighter(b, du, ds, false);
    as = make_side(b, au->faction, 0, 0, 0);
    af = make_fighter(b, au, as, true);
    set_enemy(as, ds, true);
    test_create_castorder(&co, au, 10, 10.0, 0, NULL);
    co.magician.fig = af;

    CuAssertIntEquals(tc, 10, sp_immolation(&co));
    // up to 2d4 lost (we injected 4)
    CuAssertIntEquals(tc, du->hp - 8, df->person[0].hp);
    CuAssertIntEquals(tc, au->hp, af->person[0].hp);
    test_teardown();
}

static void test_healing(CuTest *tc)
{
    castorder co;
    unit *du, *au, *u;
    region *r;
    battle *b;
    side *ds, *as;
    fighter *df, *af, *uf;

    test_setup();
    r = test_create_plain(0, 0);
    u = test_create_unit(test_create_faction(), r);
    unit_setstatus(u, ST_FIGHT);
    du = test_create_unit(test_create_faction(), r);
    ally_set(&u->faction->allies, du->faction, HELP_FIGHT);
    unit_setstatus(du, ST_FIGHT);
    au = test_create_unit(test_create_faction(), r);
    unit_setstatus(au, ST_BEHIND);

    b = make_battle(r);
    ds = make_side(b, u->faction, 0, 0, 0);
    uf = make_fighter(b, u, ds, false);
    df = make_fighter(b, du, ds, false);
    as = make_side(b, au->faction, 0, 0, 0);
    af = make_fighter(b, au, as, true);
    set_enemy(as, ds, true);
    test_create_castorder(&co, u, 10, 10.0, 0, NULL);
    co.magician.fig = uf;

    uf->person[0].hp = 1;
    df->person[0].hp = 1;
    af->person[0].hp = 1;
    CuAssertIntEquals(tc, 10, sp_healing(&co));
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->battles->msgs, "healing_effect_0"));
    CuAssertPtrNotNull(tc, test_find_messagetype(au->faction->battles->msgs, "healing_effect_0"));
    CuAssertPtrNotNull(tc, test_find_messagetype(du->faction->battles->msgs, "healing_effect_0"));
    CuAssertIntEquals(tc, u->hp, uf->person[0].hp);
    CuAssertIntEquals(tc, du->hp, df->person[0].hp);
    CuAssertIntEquals(tc, 1, af->person[0].hp);
    test_teardown();
}

CuSuite *get_combatspells_suite(void)
{
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, test_immolation);
    SUITE_ADD_TEST(suite, test_healing);

    return suite;
}
