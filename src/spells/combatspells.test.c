#include "combatspells.h"

#include <tests.h>

#include <battle.h>
#include <magic.h>

#include <kernel/unit.h>
#include <kernel/region.h>

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

CuSuite *get_combatspells_suite(void)
{
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, test_immolation);

    return suite;
}
