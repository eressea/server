#include "economy.h"

#include <CuTest.h>
#include <tests.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/order.h>
#include <kernel/region.h>
#include <kernel/unit.h>

#include <util/base36.h>
#include <util/keyword.h>
#include <util/message.h>

#include <stb_ds.h>

#include <stdlib.h>

static void setup_messages(void) {
    mt_create_va(mt_new("income", NULL), "unit:unit", "region:region", "mode:int", "wanted:int", "amount:int", MT_NEW_END);
    mt_create_va(mt_new("stealeffect", NULL), "unit:unit", "region:region", "amount:int", MT_NEW_END);
}

static void test_steal_success(CuTest * tc) {
    region *r;
    unit *u1, *u2;
    econ_request* stealorders = NULL;
    message* m;
    const struct item_type* it_money;

    test_setup();
    init_resources();
    setup_messages();
    it_money = it_find("money");
    r = test_create_plain(0, 0);
    u1 = test_create_unit(test_create_faction(), r);
    u2 = test_create_unit(test_create_faction(), r);
    i_change(&u2->items, it_money, STEALINCOME * 2);
    set_level(u1, SK_STEALTH, 1);
    u1->thisorder = create_order(K_STEAL, u1->faction->locale, itoa36(u2->no));
    steal_cmd(u1, u1->thisorder, &stealorders);
    CuAssertPtrNotNull(tc, stealorders);
    CuAssertPtrEquals(tc, u1, stealorders[0].unit);
    CuAssertIntEquals(tc, 1, stealorders[0].qty);
    CuAssertIntEquals(tc, 0, stealorders[0].unit->n);
    CuAssertIntEquals(tc, STEALINCOME, stealorders[0].unit->wants);
    expandstealing(r, stealorders);
    CuAssertIntEquals(tc, STEALINCOME, i_get(u1->items, it_money));
    CuAssertIntEquals(tc, STEALINCOME, i_get(u2->items, it_money));

    CuAssertPtrNotNull(tc, m = test_find_messagetype(u1->faction->msgs, "income"));
    CuAssertPtrEquals(tc, u1, m->parameters[0].v);
    CuAssertPtrEquals(tc, r, m->parameters[1].v);
    CuAssertIntEquals(tc, IC_STEAL, m->parameters[2].i);
    CuAssertIntEquals(tc, STEALINCOME, m->parameters[3].i);
    CuAssertIntEquals(tc, STEALINCOME, m->parameters[4].i);

    CuAssertPtrNotNull(tc, m = test_find_messagetype(u2->faction->msgs, "stealeffect"));
    CuAssertPtrEquals(tc, u2, m->parameters[0].v);
    CuAssertPtrEquals(tc, r, m->parameters[1].v);
    CuAssertIntEquals(tc, STEALINCOME, m->parameters[2].i);

    arrfree(stealorders);
    test_teardown();
}

static void test_steal_nothing(CuTest * tc) {
    region *r;
    unit* u1, * u2;
    econ_request* stealorders = NULL;
    message* m;

    test_setup();
    init_resources();
    setup_messages();
    r = test_create_plain(0, 0);
    u1 = test_create_unit(test_create_faction(), r);
    u2 = test_create_unit(test_create_faction(), r);
    set_level(u1, SK_STEALTH, 1);
    u1->thisorder = create_order(K_STEAL, u1->faction->locale, itoa36(u2->no));
    steal_cmd(u1, u1->thisorder, &stealorders);
    CuAssertPtrNotNull(tc, stealorders);
    CuAssertPtrEquals(tc, u1, stealorders[0].unit);
    CuAssertIntEquals(tc, 1, stealorders[0].qty);
    CuAssertIntEquals(tc, 0, stealorders[0].unit->n);
    CuAssertIntEquals(tc, STEALINCOME, stealorders[0].unit->wants);
    expandstealing(r, stealorders);
    CuAssertPtrNotNull(tc, m = test_find_messagetype(u1->faction->msgs, "income"));
    CuAssertPtrEquals(tc, u1, m->parameters[0].v);
    CuAssertPtrEquals(tc, r, m->parameters[1].v);
    CuAssertIntEquals(tc, IC_STEAL, m->parameters[2].i);
    CuAssertIntEquals(tc, STEALINCOME, m->parameters[3].i);
    CuAssertIntEquals(tc, 0, m->parameters[4].i);
    arrfree(stealorders);
    test_teardown();
}

CuSuite *get_steal_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_steal_success);
    SUITE_ADD_TEST(suite, test_steal_nothing);
    return suite;
}
