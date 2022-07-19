#include "defaults.h"

#include "contact.h"
#include "eressea.h"
#include "guard.h"
#include "reports.h"
#include "magic.h"                   // for create_mage

#include <kernel/ally.h>
#include <kernel/alliance.h>
#include <kernel/attrib.h>
#include <kernel/calendar.h>
#include <kernel/config.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include "kernel/skill.h"            // for SK_MELEE, SK_MAGIC, SK_STEALTH
#include "kernel/status.h"           // for ST_FIGHT, ST_FLEE
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include "kernel/types.h"            // for M_GWYRRD, M_DRAIG
#include <kernel/unit.h>

#include <util/base36.h>
#include <util/keyword.h>            // for K_NAME, K_WORK, K_MAKETEMP, K_BUY
#include <util/language.h>
#include <util/lists.h>
#include <util/message.h>
#include <util/param.h>
#include <util/rand.h>
#include "util/variant.h"  // for variant, frac_make, frac_zero

#include <CuTest.h>
#include <tests.h>

#include <assert.h>
#include <stdbool.h>                 // for false, true, bool
#include <stdio.h>
#include <string.h>

static void test_defaultorders(CuTest* tc)
{
    unit* u;
    order* ord;
    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));

    ord = u->orders = create_order(K_CAST, u->faction->locale, "STUFE 9 Sturmwind");
    ord->next = create_order(K_ENTERTAIN, u->faction->locale, NULL);
    unit_addorder(u, create_order(K_DEFAULT, u->faction->locale, keywords[K_WORK]));
    unit_addorder(u, create_order(K_DEFAULT, u->faction->locale, "@%s", keywords[K_GUARD]));
    defaultorders();
    CuAssertIntEquals(tc, K_CAST, getkeyword(ord = u->orders));
    CuAssertIntEquals(tc, K_ENTERTAIN, getkeyword(ord = ord->next));
    CuAssertPtrEquals(tc, NULL, ord->next);
    CuAssertIntEquals(tc, K_WORK, getkeyword(ord = u->defaults));
    CuAssertIntEquals(tc, K_GUARD, getkeyword(ord = ord->next));
    CuAssertPtrEquals(tc, NULL, ord->next);

    free_orders(&u->orders);
    ord = u->orders = create_order(K_SELL, u->faction->locale, "1 Juwel");
    ord->next = create_order(K_SELL, u->faction->locale, "1 Balsam");
    unit_addorder(u, create_order(K_DEFAULT, u->faction->locale, "%s 1 Weihrauch", keywords[K_BUY]));
    defaultorders();
    CuAssertIntEquals(tc, K_BUY, getkeyword(ord = u->defaults));
    CuAssertPtrEquals(tc, NULL, ord->next);
    CuAssertPtrNotNull(tc, u->orders);

    test_teardown();
}

static void test_defaultorders_clear(CuTest* tc)
{
    unit* u;
    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));

    /* empty DEFAULT clears all defaults: */
    u->orders = create_order(K_ENTERTAIN, u->faction->locale, NULL);
    unit_addorder(u, create_order(K_DEFAULT, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_DEFAULT, u->faction->locale, NULL));
    defaultorders();
    CuAssertPtrEquals(tc, NULL, u->orders);
    CuAssertPtrEquals(tc, NULL, u->defaults);

    /* New repeating DEFAULT replaces long order in defaults: */
    u->orders = create_order(K_TAX, u->faction->locale, NULL);
    unit_addorder(u, create_order(K_ENTERTAIN, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_DEFAULT, u->faction->locale, keywords[K_WORK]));
    defaultorders();
    CuAssertPtrNotNull(tc, u->defaults);
    CuAssertIntEquals(tc, K_WORK, getkeyword(u->defaults));
    CuAssertPtrEquals(tc, NULL, u->defaults->next);
    CuAssertPtrEquals(tc, NULL, u->orders);

    /* Bug 2843: empty DEFAULT clears repeated orders in template: */
    unit_addorder(u, create_order(K_CAST, u->faction->locale, "Sturmwind"));
    unit_addorder(u, create_order(K_CAST, u->faction->locale, "Beulenpest"));
    unit_addorder(u, create_order(K_DEFAULT, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_WORK, u->faction->locale, NULL));
    defaultorders();
    CuAssertPtrEquals(tc, NULL, u->orders);
    CuAssertPtrNotNull(tc, u->defaults);
    CuAssertIntEquals(tc, K_WORK, getkeyword(u->defaults));
    update_defaults(u->faction);
    CuAssertIntEquals(tc, K_WORK, getkeyword(u->orders));
    CuAssertPtrEquals(tc, NULL, u->orders->next);
    CuAssertPtrEquals(tc, NULL, u->defaults);

    test_teardown();
}

static void test_long_order_normal(CuTest* tc) {
    /* TODO: write more tests */
    unit* u;
    order* ord;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    fset(u, UFL_MOVED);
    fset(u, UFL_LONGACTION);
    unit_addorder(u, ord = create_order(K_ENTERTAIN, u->faction->locale, NULL));
    update_long_order(u);
    CuAssertIntEquals(tc, ord->id, u->thisorder->id);
    CuAssertIntEquals(tc, 0, fval(u, UFL_MOVED));
    CuAssertIntEquals(tc, 0, fval(u, UFL_LONGACTION));
    CuAssertPtrEquals(tc, ord, u->orders);
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);
    CuAssertPtrEquals(tc, NULL, u->defaults);
    test_teardown();
}

static void test_long_order_none(CuTest* tc) {
    /* TODO: write more tests */
    unit* u;
    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    update_long_order(u);
    CuAssertPtrEquals(tc, NULL, u->thisorder);
    CuAssertPtrEquals(tc, NULL, u->orders);
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);
    test_teardown();
}

static void test_long_order_cast(CuTest* tc) {
    /* TODO: write more tests */
    unit* u;
    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    unit_addorder(u, create_order(K_CAST, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_CAST, u->faction->locale, NULL));
    update_long_order(u);
    CuAssertPtrEquals(tc, NULL, u->thisorder);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);
    test_teardown();
}

static void test_long_order_buy_sell(CuTest* tc) {
    /* TODO: write more tests */
    unit* u;
    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_SELL, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_SELL, u->faction->locale, NULL));
    update_long_order(u);
    CuAssertPtrEquals(tc, NULL, u->thisorder);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);
    test_teardown();
}

static void test_long_order_multi_long(CuTest* tc) {
    /* TODO: write more tests */
    unit* u;
    test_setup();
    mt_create_error(52);
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    unit_addorder(u, create_order(K_MOVE, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_DESTROY, u->faction->locale, NULL));
    update_long_order(u);
    CuAssertPtrNotNull(tc, u->thisorder);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error52"));
    test_teardown();
}

static void test_long_order_multi_buy(CuTest* tc) {
    /* TODO: write more tests */
    unit* u;
    test_setup();
    mt_create_error(52);
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, 0));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, 0));
    update_long_order(u);
    CuAssertPtrEquals(tc, NULL, u->thisorder);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error52"));
    test_teardown();
}

static void test_long_order_multi_sell(CuTest* tc) {
    /* TODO: write more tests */
    unit* u;
    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    unit_addorder(u, create_order(K_SELL, u->faction->locale, 0));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, 0));
    unit_addorder(u, create_order(K_SELL, u->faction->locale, 0));
    update_long_order(u);
    CuAssertPtrEquals(tc, NULL, u->thisorder);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);
    test_teardown();
}

static void test_long_order_buy_cast(CuTest* tc) {
    /* TODO: write more tests */
    unit* u;
    test_setup();
    mt_create_error(52);
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, 0));
    unit_addorder(u, create_order(K_CAST, u->faction->locale, 0));
    update_long_order(u);
    CuAssertPtrEquals(tc, NULL, u->thisorder);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error52"));
    test_teardown();
}

static void test_long_order_hungry(CuTest* tc) {
    unit* u;
    test_setup();
    config_set("hunger.long", "1");
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    fset(u, UFL_HUNGER);
    unit_addorder(u, create_order(K_MOVE, u->faction->locale, 0));
    unit_addorder(u, create_order(K_DESTROY, u->faction->locale, 0));
    config_set("orders.default", "work");
    update_long_order(u);
    CuAssertIntEquals(tc, K_WORK, getkeyword(u->thisorder));
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);
    test_teardown();
}

static void test_update_defaults(CuTest* tc) {
    unit* u;
    order* ord;

    test_setup();
    u = test_create_unit(NULL, NULL);

    /* a long order gets promoted to storage, short order does not */
    unit_addorder(u, ord = create_order(K_ENTERTAIN, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_GUARD, u->faction->locale, NULL));
    CuAssertPtrEquals(tc, ord, u->orders);
    update_long_order(u);
    CuAssertIntEquals(tc, ord->id, u->thisorder->id);
    update_defaults(u->faction);
    CuAssertIntEquals(tc, ord->id, u->orders->id);
    CuAssertPtrEquals(tc, NULL, ord->next);
    CuAssertPtrEquals(tc, NULL, u->defaults);

    /* persistent short orders are kept */
    unit_addorder(u, create_order(K_GIVE, u->faction->locale, NULL));
    u->orders->command |= CMD_PERSIST;
    CuAssertTrue(tc, is_persistent(u->orders));
    CuAssertIntEquals(tc, K_GIVE, getkeyword(u->orders));
    unit_addorder(u, create_order(K_GUARD, u->faction->locale, NULL));
    CuAssertTrue(tc, !is_persistent(u->orders->next));
    update_defaults(u->faction);
    CuAssertIntEquals(tc, ord->id, u->orders->id);
    ord = u->orders->next;
    CuAssertIntEquals(tc, K_GIVE, getkeyword(ord));
    CuAssertTrue(tc, is_persistent(ord));
    CuAssertPtrEquals(tc, NULL, ord->next);
    CuAssertPtrEquals(tc, NULL, u->defaults);

    /* K_SELL, K_BUY, K_CAST, K_ATTACK do not become thisorder */
    unit_addorder(u, create_order(K_ATTACK, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_SELL, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_CAST, u->faction->locale, NULL));
    update_long_order(u);
    CuAssertPtrEquals(tc, NULL, u->thisorder);

    /* K_SELL, K_BUY, K_CAST are stored, K_ATTACK and K_MOVE are not */
    unit_addorder(u, create_order(K_MOVE, u->faction->locale, NULL));
    update_defaults(u->faction);
    CuAssertIntEquals(tc, K_SELL, getkeyword(u->orders));
    CuAssertIntEquals(tc, 3, listlen(u->orders));
    CuAssertPtrEquals(tc, NULL, u->defaults);

    test_teardown();
}

CuSuite *get_defaults_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_defaultorders);
    SUITE_ADD_TEST(suite, test_defaultorders_clear);
    SUITE_ADD_TEST(suite, test_long_order_normal);
    SUITE_ADD_TEST(suite, test_long_order_none);
    SUITE_ADD_TEST(suite, test_long_order_cast);
    SUITE_ADD_TEST(suite, test_long_order_buy_sell);
    SUITE_ADD_TEST(suite, test_long_order_multi_long);
    SUITE_ADD_TEST(suite, test_long_order_multi_buy);
    SUITE_ADD_TEST(suite, test_long_order_multi_sell);
    SUITE_ADD_TEST(suite, test_long_order_buy_cast);
    SUITE_ADD_TEST(suite, test_long_order_hungry);
    SUITE_ADD_TEST(suite, test_update_defaults);

    return suite;
}
