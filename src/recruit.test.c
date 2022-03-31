#include "recruit.h"

#include "kernel/faction.h"
#include "kernel/item.h"
#include "kernel/order.h"
#include "kernel/race.h"
#include "kernel/region.h"
#include "kernel/unit.h"

#include "util/message.h"

#include <tests.h>
#include <CuTest.h>

static void setup_recruit(void)
{
    init_resources();
    mt_create_va(mt_new("recruit", NULL),
        "unit:unit", "region:region", "amount:int", "want:int", MT_NEW_END);
}

static void test_recruit(CuTest* tc)
{
    region* r;
    unit* u;
    faction* f;
    race* rc;
    item_type* it_money;
    message* msg;

    test_setup();
    setup_recruit();
    it_money = it_find("money");
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    u_setrace(u, rc = test_create_race("human"));
    rc->recruitcost = 100;
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
    rc->recruit_multi = 0.5;
    u = test_create_unit(f, r);
    rc->recruitcost = 50;
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

    /* enough peasants for just one orc recruit */
    i_change(&u->items, it_money, 51);
    r->land->peasants = RECRUIT_FRACTION / 2;
    recruit(r);
    CuAssertIntEquals(tc, 5, u->number);
    CuAssertIntEquals(tc, 50, i_get(u->items, it_money));
    CuAssertIntEquals(tc, RECRUIT_FRACTION / 2 - 1, r->land->peasants);
    CuAssertPtrNotNull(tc, msg = test_find_messagetype(f->msgs, "recruit"));
    CuAssertPtrEquals(tc, u, (unit*)msg->parameters[0].v);
    CuAssertPtrEquals(tc, r, (region*)msg->parameters[1].v);
    CuAssertIntEquals(tc, 1, msg->parameters[2].i);
    CuAssertIntEquals(tc, 2, msg->parameters[3].i);
    test_clear_messages(f);

    test_teardown();
}

CuSuite* get_recruit_suite(void)
{
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_recruit);
    SUITE_ADD_TEST(suite, test_recruit_orcs);
    return suite;
}
