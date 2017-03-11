#include <platform.h>
#include "upkeep.h"

#include <kernel/config.h>
#include <kernel/faction.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/item.h>

#include <CuTest.h>
#include <tests.h>

#include <assert.h>

void test_upkeep_default(CuTest * tc)
{
    region *r;
    unit *u1, *u2;
    faction *f1, *f2;
    const item_type *i_silver;

    test_setup();
    test_create_world();

    i_silver = it_find("money");
    assert(i_silver);
    r = findregion(0, 0);
    f1 = test_create_faction(test_create_race("human"));
    f2 = test_create_faction(test_create_race("human"));
    assert(f1 && f2);
    u1 = test_create_unit(f1, r);
    u2 = test_create_unit(f2, r);
    assert(r && u1 && u2);

    config_set("rules.food.flags", "0");
    i_change(&u1->items, i_silver, 20);
    get_food(r);
    /* since u1 and u2 are not allied, u1 should not help u2 with upkeep */
    CuAssertIntEquals(tc, 10, i_get(u1->items, i_silver));
    CuAssertIntEquals(tc, 0, fval(u1, UFL_HUNGER));
    CuAssertIntEquals(tc, UFL_HUNGER, fval(u2, UFL_HUNGER));

    test_cleanup();
}

void test_upkeep_hunger_damage(CuTest * tc)
{
    region *r;
    unit *u1;
    faction *f1;
    const item_type *i_silver;

    test_setup();
    test_create_world();

    i_silver = it_find("money");
    assert(i_silver);
    r = findregion(0, 0);
    f1 = test_create_faction(test_create_race("human"));
    u1 = test_create_unit(f1, r);
    assert(r && u1);

    config_set("rules.food.flags", "0");
    u1->hp = 100;
    get_food(r);
    /* since u1 and u2 are not allied, u1 should not help u2 with upkeep */
    CuAssertTrue(tc, u1->hp < 100);

    test_cleanup();
}

void test_upkeep_from_pool(CuTest * tc)
{
    region *r;
    unit *u1, *u2;
    const item_type *i_silver;

    test_setup();
    test_create_world();

    i_silver = it_find("money");
    assert(i_silver);
    r = findregion(0, 0);
    assert(r);
    u1 = test_create_unit(test_create_faction(test_create_race("human")), r);
	assert(u1);
    u2 = test_create_unit(u1->faction, r);
    assert(u2);

    config_set("rules.food.flags", "0");
    i_change(&u1->items, i_silver, 30);
    get_food(r);
    CuAssertIntEquals(tc, 10, i_get(u1->items, i_silver));
    CuAssertIntEquals(tc, 0, fval(u1, UFL_HUNGER));
    CuAssertIntEquals(tc, 0, fval(u2, UFL_HUNGER));
    get_food(r);
    CuAssertIntEquals(tc, 0, i_get(u1->items, i_silver));
    CuAssertIntEquals(tc, 0, fval(u1, UFL_HUNGER));
    CuAssertIntEquals(tc, UFL_HUNGER, fval(u2, UFL_HUNGER));

    test_cleanup();
}


void test_upkeep_from_friend(CuTest * tc)
{
    region *r;
    unit *u1, *u2;
    faction *f1, *f2;
    const item_type *i_silver;

    test_setup();
    test_create_world();

    i_silver = it_find("money");
    assert(i_silver);
    r = findregion(0, 0);
    f1 = test_create_faction(test_create_race("human"));
    f2 = test_create_faction(test_create_race("human"));
    assert(f1 && f2);
    set_alliance(f1, f2, HELP_MONEY);
    u1 = test_create_unit(f1, r);
    u2 = test_create_unit(f2, r);
    assert(r && u1 && u2);

    config_set("rules.food.flags", "0");
    i_change(&u1->items, i_silver, 30);
    get_food(r);
    CuAssertIntEquals(tc, 10, i_get(u1->items, i_silver));
    CuAssertIntEquals(tc, 0, fval(u1, UFL_HUNGER));
    CuAssertIntEquals(tc, 0, fval(u2, UFL_HUNGER));
    get_food(r);
    CuAssertIntEquals(tc, 0, i_get(u1->items, i_silver));
    CuAssertIntEquals(tc, 0, fval(u1, UFL_HUNGER));
    CuAssertIntEquals(tc, UFL_HUNGER, fval(u2, UFL_HUNGER));

    test_cleanup();
}

void test_upkeep_free(CuTest * tc)
{
    region *r;
    unit *u;
    const item_type *i_silver;

    test_setup();
    test_create_world();

    i_silver = it_find("money");
    assert(i_silver);
    r = findregion(0, 0);
    u = test_create_unit(test_create_faction(test_create_race("human")), r);
    assert(r && u);

    config_set("rules.food.flags", "4"); /* FOOD_IS_FREE */
    get_food(r);
    CuAssertIntEquals(tc, 0, i_get(u->items, i_silver));
    CuAssertIntEquals(tc, 0, fval(u, UFL_HUNGER));

    test_cleanup();
}

CuSuite *get_upkeep_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_upkeep_default);
    SUITE_ADD_TEST(suite, test_upkeep_from_pool);
    SUITE_ADD_TEST(suite, test_upkeep_from_friend);
    SUITE_ADD_TEST(suite, test_upkeep_hunger_damage);
    SUITE_ADD_TEST(suite, test_upkeep_free);
    return suite;
}
