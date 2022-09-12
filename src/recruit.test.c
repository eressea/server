#include "recruit.h"

#include "kernel/config.h"
#include "kernel/faction.h"
#include "kernel/item.h"
#include "kernel/order.h"
#include "kernel/race.h"
#include "kernel/region.h"
#include "kernel/skills.h"
#include "kernel/unit.h"

#include "util/keyword.h"    // for K_RECRUIT
#include "util/message.h"
#include "util/variant.h"    // for variant

#include "stb_ds.h"

#include <tests.h>
#include <CuTest.h>

#include <stddef.h>          // for NULL

static void setup_recruit(void)
{
    init_resources();
    mt_create_va(mt_new("recruit", NULL),
        "unit:unit", "region:region", "amount:int", "want:int", MT_NEW_END);
}

static void test_stb(CuTest* tc)
{
    void** varr;
    int* arr = NULL;
    arrpush(arr, 42);
    CuAssertIntEquals(tc, 1, (int)arrlen(arr));
    arrpush(arr, 47);
    CuAssertIntEquals(tc, 42, arr[0]);
    CuAssertIntEquals(tc, 47, arr[1]);
    varr = NULL;
    arrpush(varr, NULL);
    CuAssertIntEquals(tc, 1, (int)arrlen(varr));
}

static void test_recruit(CuTest* tc)
{
    region* r;
    unit* u;
    faction* f;
    item_type* it_money;
    message* msg;

    test_setup();
    setup_recruit();
    it_money = it_find("money");
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    unit_addorder(u, create_order(K_RECRUIT, f->locale, "%d", 1));

    /* happy case: */
    i_change(&u->items, it_money, 100);
    r->land->peasants = RECRUIT_FRACTION;
    recruit(r);
    CuAssertIntEquals(tc, 2, u->number);
    CuAssertIntEquals(tc, 0, i_get(u->items, it_money));
    CuAssertIntEquals(tc, RECRUIT_FRACTION - 1, r->land->peasants);
    CuAssertPtrEquals(tc, NULL, f->msgs);

    /* not enough money */
    i_change(&u->items, it_money, 99);
    r->land->peasants = RECRUIT_FRACTION;
    recruit(r);
    CuAssertIntEquals(tc, 2, u->number);
    CuAssertIntEquals(tc, 99, i_get(u->items, it_money));
    CuAssertIntEquals(tc, RECRUIT_FRACTION, r->land->peasants);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error142"));
    test_clear_messages(f);

    /* not enough peasants */
    i_change(&u->items, it_money, 1);
    r->land->peasants = RECRUIT_FRACTION - 1;
    recruit(r);
    CuAssertIntEquals(tc, 2, u->number);
    CuAssertIntEquals(tc, 100, i_get(u->items, it_money));
    CuAssertIntEquals(tc, RECRUIT_FRACTION - 1, r->land->peasants);
    CuAssertPtrNotNull(tc, msg = test_find_messagetype(f->msgs, "recruit"));
    CuAssertPtrEquals(tc, u, (unit*)msg->parameters[0].v);
    CuAssertPtrEquals(tc, r, (region*)msg->parameters[1].v);
    CuAssertIntEquals(tc, 0, msg->parameters[2].i);
    CuAssertIntEquals(tc, 1, msg->parameters[3].i);
    test_clear_messages(f);

    test_teardown();
}

static void test_recruit_orcs(CuTest* tc)
{
    region* r;
    unit* u;
    faction* f;
    race* rc;
    message* msg;
    item_type* it_money;

    test_setup();
    setup_recruit();
    it_money = it_find("money");
    r = test_create_plain(0, 0);
    f = test_create_faction();
    f->race = rc = test_create_race("orc");
    rc->recruit_multi = 2;
    rc->recruitcost = 50;
    u = test_create_unit(f, r);
    unit_addorder(u, create_order(K_RECRUIT, f->locale, "%d", 2));

    /* happy case, two new recruits for one peasant */
    r->land->peasants = RECRUIT_FRACTION * 2;
    i_change(&u->items, it_money, 100);
    recruit(r);
    CuAssertIntEquals(tc, 3, u->number);
    CuAssertIntEquals(tc, 0, i_get(u->items, it_money));
    CuAssertIntEquals(tc, RECRUIT_FRACTION * 2 - 1, r->land->peasants);
    CuAssertPtrEquals(tc, NULL, f->msgs);

    /* not enough money for any recruits */
    i_change(&u->items, it_money, 49);
    r->land->peasants = RECRUIT_FRACTION * 2;
    recruit(r);
    CuAssertIntEquals(tc, 3, u->number);
    CuAssertIntEquals(tc, 49, i_get(u->items, it_money));
    CuAssertIntEquals(tc, RECRUIT_FRACTION * 2, r->land->peasants);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error142"));
    test_clear_messages(f);

    /* only enough money for one recruits */
    i_change(&u->items, it_money, 50);
    recruit(r);
    CuAssertIntEquals(tc, 4, u->number);
    CuAssertIntEquals(tc, 49, i_get(u->items, it_money));
    CuAssertIntEquals(tc, RECRUIT_FRACTION * 2 - 1, r->land->peasants);
    CuAssertPtrNotNull(tc, msg = test_find_messagetype(f->msgs, "recruit"));
    CuAssertPtrEquals(tc, u, (unit*)msg->parameters[0].v);
    CuAssertPtrEquals(tc, r, (region*)msg->parameters[1].v);
    CuAssertIntEquals(tc, 1, msg->parameters[2].i);
    CuAssertIntEquals(tc, 2, msg->parameters[3].i);
    test_clear_messages(f);

    /* Bug: region must have >= 40 peasants to recruit */
    i_change(&u->items, it_money, 51);
    r->land->peasants = RECRUIT_FRACTION / 2;
    recruit(r);
    CuAssertIntEquals(tc, 4, u->number);
    CuAssertIntEquals(tc, 100, i_get(u->items, it_money));
    CuAssertIntEquals(tc, RECRUIT_FRACTION / 2, r->land->peasants);
    CuAssertPtrNotNull(tc, msg = test_find_messagetype(f->msgs, "recruit"));
    CuAssertPtrEquals(tc, u, (unit*)msg->parameters[0].v);
    CuAssertPtrEquals(tc, r, (region*)msg->parameters[1].v);
    CuAssertIntEquals(tc, 0, msg->parameters[2].i);
    CuAssertIntEquals(tc, 2, msg->parameters[3].i);
    test_clear_messages(f);

    test_teardown();
}

static void test_recruit_split(CuTest* tc)
{
    region* r;
    unit* u1, * u2;
    faction* f1, * f2;
    item_type* it_money;

    test_setup();
    setup_recruit();
    it_money = it_find("money");
    r = test_create_plain(0, 0);
    f1 = test_create_faction();
    f2 = test_create_faction();
    u1 = test_create_unit(f1, r);
    u2 = test_create_unit(f2, r);

    /* happy case: */
    unit_addorder(u1, create_order(K_RECRUIT, f1->locale, "%d", 1));
    unit_addorder(u2, create_order(K_RECRUIT, f2->locale, "%d", 1));
    i_change(&u1->items, it_money, 100);
    i_change(&u2->items, it_money, 100);
    r->land->peasants = RECRUIT_FRACTION * 2;
    recruit(r);
    CuAssertIntEquals(tc, 2, u1->number);
    CuAssertIntEquals(tc, 2, u2->number);
    CuAssertIntEquals(tc, 0, i_get(u1->items, it_money));
    CuAssertIntEquals(tc, 0, i_get(u2->items, it_money));
    CuAssertIntEquals(tc, RECRUIT_FRACTION * 2 - 2, r->land->peasants);
    free_orders(&u1->orders);
    free_orders(&u2->orders);

    /* not enough peasants */
    unit_addorder(u1, create_order(K_RECRUIT, f1->locale, "%d", 2));
    unit_addorder(u2, create_order(K_RECRUIT, f2->locale, "%d", 2));
    i_change(&u1->items, it_money, 200);
    i_change(&u2->items, it_money, 200);
    r->land->peasants = RECRUIT_FRACTION * 2;
    recruit(r);
    CuAssertIntEquals(tc, 3, u1->number);
    CuAssertIntEquals(tc, 3, u2->number);
    CuAssertIntEquals(tc, RECRUIT_FRACTION * 2 - 2, r->land->peasants);

    test_teardown();
}

static void test_recruit_split_one_peasant(CuTest* tc)
{
    region* r;
    unit* u1, * u2;
    faction* f1, * f2;
    item_type* it_money;

    test_setup();
    setup_recruit();
    it_money = it_find("money");
    r = test_create_plain(0, 0);
    f1 = test_create_faction();
    f2 = test_create_faction();
    u1 = test_create_unit(f1, r);
    u2 = test_create_unit(f2, r);

    /* only one peasant can be recruited, someone will win it: */
    unit_addorder(u1, create_order(K_RECRUIT, f1->locale, "%d", 1));
    unit_addorder(u2, create_order(K_RECRUIT, f2->locale, "%d", 1));
    i_change(&u1->items, it_money, 100);
    i_change(&u2->items, it_money, 100);
    r->land->peasants = RECRUIT_FRACTION;
    recruit(r);
    CuAssertIntEquals(tc, 3, u1->number + u2->number);
    CuAssertIntEquals(tc, RECRUIT_FRACTION - 1, r->land->peasants);

    test_teardown();
}

static void test_recruit_orcs_split_one_peasant(CuTest* tc)
{
    region* r;
    unit* u1, * u2;
    faction* f1, * f2;
    race* rc;
    item_type* it_money;

    test_setup();
    setup_recruit();
    it_money = it_find("money");
    r = test_create_plain(0, 0);
    rc = test_create_race("orc");
    rc->recruit_multi = 2;
    f1 = test_create_faction();
    f1->race = rc;
    f2 = test_create_faction();
    f2->race = rc;
    u1 = test_create_unit(f1, r);
    u2 = test_create_unit(f2, r);

    /* only one peasant can be recruited, someone will win it: */
    unit_addorder(u1, create_order(K_RECRUIT, f1->locale, "%d", 1));
    unit_addorder(u2, create_order(K_RECRUIT, f2->locale, "%d", 1));
    i_change(&u1->items, it_money, 100);
    i_change(&u2->items, it_money, 100);
    r->land->peasants = RECRUIT_FRACTION;
    recruit(r);
    CuAssertIntEquals(tc, 2, u1->number);
    CuAssertIntEquals(tc, 2, u2->number);
    CuAssertIntEquals(tc, RECRUIT_FRACTION - 1, r->land->peasants);

    test_teardown();
}

static void test_recruit_mixed_race(CuTest* tc)
{
    region* r;
    unit* u1, * u2;
    faction* f1, * f2;
    race* rc;
    item_type* it_money;

    test_setup();
    setup_recruit();
    it_money = it_find("money");
    r = test_create_plain(0, 0);
    f1 = test_create_faction();
    f2 = test_create_faction();
    rc = test_create_race("orc");
    rc->recruit_multi = 2;
    f2->race = rc;
    u1 = test_create_unit(f1, r);
    u2 = test_create_unit(f2, r);

    /* only one peasant can be recruited: */
    unit_addorder(u1, create_order(K_RECRUIT, f1->locale, "%d", 1));
    unit_addorder(u2, create_order(K_RECRUIT, f2->locale, "%d", 1));
    i_change(&u1->items, it_money, 100);
    i_change(&u2->items, it_money, 100);
    r->land->peasants = RECRUIT_FRACTION * 3 / 2;
    recruit(r);
    CuAssertIntEquals(tc, 3, u1->number + u2->number);
    CuAssertIntEquals(tc, RECRUIT_FRACTION * 3 / 2 - 1, r->land->peasants);

    test_teardown();
}

/**
 * Recruiting new men into a unit dilutes its skills
 */
static void test_recruiting_dilutes_skills(CuTest* tc) {
    unit* u;
    region* r;
    faction* f;
    skill* sv;

    test_setup();
    config_set_int("study.random_progress", 0);
    r = test_create_plain(0, 0);
    f = test_create_faction();

    u = test_create_unit(f, r);
    set_level(u, SK_ALCHEMY, 4);

    add_recruits(u, 9, 9, 9);
    CuAssertIntEquals(tc, 10, u->number);
    sv = unit_skill(u, SK_ALCHEMY);
    CuAssertIntEquals(tc, 1, sv->level);
    CuAssertIntEquals(tc, 2, sv->weeks);

    scale_number(u, 1);
    add_recruits(u, 9, 9, 9);
    CuAssertPtrEquals(tc, NULL, unit_skill(u, SK_ALCHEMY));

    test_teardown();
}

CuSuite* get_recruit_suite(void)
{
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_stb);
    SUITE_ADD_TEST(suite, test_recruit);
    SUITE_ADD_TEST(suite, test_recruit_orcs);
    SUITE_ADD_TEST(suite, test_recruit_mixed_race);
    SUITE_ADD_TEST(suite, test_recruit_split);
    SUITE_ADD_TEST(suite, test_recruit_split_one_peasant);
    SUITE_ADD_TEST(suite, test_recruit_orcs_split_one_peasant);
    SUITE_ADD_TEST(suite, test_recruiting_dilutes_skills);
    return suite;
}
