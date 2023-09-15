#include "defaults.h"

#include <kernel/config.h>
#include <kernel/faction.h>
#include <kernel/order.h>
#include <kernel/unit.h>

#include <util/keyword.h>            // for K_NAME, K_WORK, K_MAKETEMP, K_BUY
#include <util/lists.h>
#include <util/message.h>

#include <CuTest.h>
#include <tests.h>

#include <stdio.h>

static void test_defaultorders(CuTest* tc)
{
    unit* u;
    order* ord;
    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));

    ord = u->orders = create_order(K_CAST, u->faction->locale, "STUFE 9 Sturmwind");
    ord->next = create_order(K_ENTERTAIN, u->faction->locale, NULL);
    unit_addorder(u, create_order(K_DEFAULT, u->faction->locale,
        keyword_name(K_WORK, u->faction->locale)));
    unit_addorder(u, create_order(K_DEFAULT, u->faction->locale, "@%s", 
        keyword_name(K_GUARD, u->faction->locale)));
    defaultorders();
    CuAssertIntEquals(tc, K_CAST, getkeyword(ord = u->orders));
    CuAssertIntEquals(tc, K_ENTERTAIN, getkeyword(ord = ord->next));
    CuAssertPtrEquals(tc, NULL, ord->next);
    CuAssertIntEquals(tc, K_WORK, getkeyword(ord = u->defaults));
    CuAssertIntEquals(tc, K_GUARD, getkeyword(ord = ord->next));
    CuAssertPtrEquals(tc, NULL, ord->next);

    free_orders(&u->orders);
    free_orders(&u->defaults);
    ord = u->orders = create_order(K_SELL, u->faction->locale, "1 Juwel");
    ord->next = create_order(K_SELL, u->faction->locale, "1 Balsam");
    unit_addorder(u, create_order(K_DEFAULT, u->faction->locale, "%s 1 Weihrauch",
        keyword_name(K_BUY, u->faction->locale)));
    defaultorders();
    CuAssertIntEquals(tc, K_BUY, getkeyword(ord = u->defaults));
    CuAssertPtrEquals(tc, NULL, ord->next);
    CuAssertPtrNotNull(tc, u->orders);

    /* if we already have default orders (because K_MOVE didn't clear them),
     * then K_DEFAULT will append extra ones to the list.
     */
    free_orders(&u->orders);
    free_orders(&u->defaults);
    ord = u->defaults = create_order(K_BUY, u->faction->locale, "1 Weihrauch");
    ord = ord->next = create_order(K_SELL, u->faction->locale, "1 Juwel");

    unit_addorder(u, create_order(K_KOMMENTAR, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_MOVE, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_DEFAULT, u->faction->locale, "%s",
        keyword_name(K_WORK, u->faction->locale)));
    defaultorders();
    CuAssertPtrNotNull(tc, ord = u->orders);
    CuAssertIntEquals(tc, K_KOMMENTAR, getkeyword(ord));
    CuAssertIntEquals(tc, K_MOVE, getkeyword(ord = ord->next));
    CuAssertPtrEquals(tc, NULL, ord->next);

    CuAssertPtrNotNull(tc, ord = u->defaults);
    CuAssertIntEquals(tc, K_WORK, getkeyword(ord));
    CuAssertPtrEquals(tc, NULL, ord->next);

    update_defaults();
    CuAssertPtrEquals(tc, NULL, u->defaults);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertIntEquals(tc, K_WORK, getkeyword(ord = u->orders));
    CuAssertIntEquals(tc, K_KOMMENTAR, getkeyword(ord = ord->next));
    CuAssertPtrEquals(tc, NULL, ord->next);

    test_teardown();
}

static void test_long_order_normal(CuTest* tc) {

    unit* u;
    order* ord;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    fset(u, UFL_MOVED);
    fset(u, UFL_LONGACTION);
    unit_addorder(u, ord = create_order(K_ENTERTAIN, u->faction->locale, NULL));
    update_long_orders();
    CuAssertIntEquals(tc, ord->id, u->thisorder->id);
    CuAssertIntEquals(tc, 0, fval(u, UFL_MOVED));
    CuAssertIntEquals(tc, 0, fval(u, UFL_LONGACTION));
    CuAssertPtrEquals(tc, ord, u->orders);
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);
    CuAssertPtrEquals(tc, NULL, u->defaults);
    test_teardown();
}

static void test_long_order_none(CuTest* tc) {

    unit* u;
    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    update_long_orders();
    CuAssertPtrEquals(tc, NULL, u->thisorder);
    CuAssertPtrEquals(tc, NULL, u->orders);
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);
    test_teardown();
}

static void test_long_order_cast(CuTest* tc) {

    unit* u;
    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    unit_addorder(u, create_order(K_CAST, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_CAST, u->faction->locale, NULL));
    update_long_orders();
    CuAssertPtrEquals(tc, NULL, u->thisorder);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);
    test_teardown();
}

static void test_long_order_attack(CuTest* tc) {

    unit* u;
    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    unit_addorder(u, create_order(K_ATTACK, u->faction->locale, NULL));
    update_long_orders();
    CuAssertPtrEquals(tc, NULL, u->thisorder);

    unit_addorder(u, create_order(K_ATTACK, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_AUTOSTUDY, u->faction->locale, NULL));
    update_long_orders();
    CuAssertIntEquals(tc, K_AUTOSTUDY, getkeyword(u->thisorder));

    test_teardown();
}

static void test_long_order_buy_sell(CuTest* tc) {

    unit* u;
    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_SELL, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_SELL, u->faction->locale, NULL));
    update_long_orders();
    CuAssertPtrEquals(tc, NULL, u->thisorder);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);
    test_teardown();
}

static void test_long_order_multi_long(CuTest* tc) {

    unit* u;
    test_setup();
    mt_create_error(52);
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    unit_addorder(u, create_order(K_MOVE, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_DESTROY, u->faction->locale, NULL));
    update_long_orders();
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrNotNull(tc, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error52"));
    test_teardown();
}

static void test_long_order_multi_buy(CuTest* tc) {

    unit* u;
    test_setup();
    mt_create_error(52);
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, 0));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, 0));
    update_long_orders();
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrEquals(tc, NULL, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error52"));
    test_teardown();
}

static void test_long_order_trade_and_other(CuTest *tc) {

    unit *u;
    test_setup();
    mt_create_error(52);
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    unit_addorder(u, create_order(K_WORK, u->faction->locale, 0));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, 0));
    update_long_orders();
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertIntEquals(tc, K_WORK, getkeyword(u->thisorder));
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error52"));
    test_teardown();
}

static void test_long_order_multi_sell(CuTest* tc) {

    unit* u;
    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    unit_addorder(u, create_order(K_SELL, u->faction->locale, 0));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, 0));
    unit_addorder(u, create_order(K_SELL, u->faction->locale, 0));
    update_long_orders();
    CuAssertPtrEquals(tc, NULL, u->thisorder);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);
    test_teardown();
}

static void test_long_order_buy_cast(CuTest* tc) {

    unit* u;
    test_setup();
    mt_create_error(52);
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, 0));
    unit_addorder(u, create_order(K_CAST, u->faction->locale, 0));
    update_long_orders();
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
    update_long_orders();
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
    update_long_orders();
    CuAssertIntEquals(tc, ord->id, u->thisorder->id);
    update_defaults();
    CuAssertIntEquals(tc, ord->id, u->orders->id);
    CuAssertPtrEquals(tc, NULL, u->orders->next);
    CuAssertPtrEquals(tc, NULL, u->defaults);

    /* persistent short orders are kept */
    free_orders(&u->orders);
    unit_addorder(u, create_order(K_GIVE, u->faction->locale, NULL));
    u->orders->command |= CMD_PERSIST;
    CuAssertTrue(tc, is_persistent(u->orders));
    CuAssertIntEquals(tc, K_GIVE, getkeyword(u->orders));
    unit_addorder(u, create_order(K_GUARD, u->faction->locale, NULL));
    CuAssertTrue(tc, !is_persistent(u->orders->next));
    update_defaults();
    ord = u->orders;
    CuAssertIntEquals(tc, K_GIVE, getkeyword(ord));
    CuAssertTrue(tc, is_persistent(ord));
    CuAssertPtrEquals(tc, NULL, ord->next);
    CuAssertPtrEquals(tc, NULL, u->defaults);

    free_orders(&u->orders);
    unit_addorder(u, create_order(K_ATTACK, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_MOVE, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_SELL, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_CAST, u->faction->locale, NULL));

    /* K_SELL, K_BUY, K_CAST are stored, K_ATTACK and K_MOVE are not */
    update_defaults();
    CuAssertIntEquals(tc, K_SELL, getkeyword(u->orders));
    CuAssertIntEquals(tc, 3, listlen(u->orders));
    CuAssertPtrEquals(tc, NULL, u->defaults);

    /* if the unit has one or more defaults (from K_DEFAULT) then those are kept instead of the new ones */
    free_orders(&u->orders);
    u->defaults = create_order(K_WORK, u->faction->locale, NULL);
    u->defaults->next = create_order(K_GUARD, u->faction->locale, NULL);
    unit_addorder(u, create_order(K_ENTERTAIN, u->faction->locale, NULL));
    unit_addorder(u, create_order(K_KOMMENTAR, u->faction->locale, NULL));
    update_defaults();
    CuAssertIntEquals(tc, K_WORK, getkeyword(ord = u->orders));
    CuAssertIntEquals(tc, K_GUARD, getkeyword(ord = ord->next));
    CuAssertIntEquals(tc, K_KOMMENTAR, getkeyword(ord = ord->next));
    CuAssertPtrEquals(tc, NULL, ord->next);
    CuAssertPtrEquals(tc, NULL, u->defaults);

    test_teardown();
}

/**
 * After reading new orders, the old defaults are in u->defaults.
 * Setting u->thisorder in update_long_orders clears them, unless
 * the new long order is not persistent (like K_MOVE).
 */
static void test_long_order_overwrites_defaults(CuTest* tc) {
    unit* u;
    order* ord;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));

    /* the unit used to have a K_WORK order, which is now replaced with
     * K_ENTERTAIN. The old defaults are cleared entirely.
     */
    u->defaults = create_order(K_WORK, u->faction->locale, NULL);
    u->defaults->next = create_order(K_GUARD, u->faction->locale, NULL);
    unit_addorder(u, ord = create_order(K_ENTERTAIN, u->faction->locale, NULL));
    update_long_orders();
    CuAssertIntEquals(tc, K_ENTERTAIN, getkeyword(u->thisorder));
    CuAssertIntEquals(tc, ord->id, u->thisorder->id);

    CuAssertPtrEquals(tc, ord, u->orders);
    CuAssertPtrEquals(tc, NULL, u->orders->next);

    CuAssertPtrEquals(tc, NULL, u->defaults);
    test_teardown();
}

static void test_move_keeps_defaults(CuTest* tc) {
    unit* u;
    order* ord;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));

    /* The unit used to have a K_WORK order, which is not replaced because
     * the new order is K_MOVE. The old short defaults are dropped.
     */
    u->defaults = create_order(K_WORK, u->faction->locale, NULL);
    u->defaults->next = create_order(K_GUARD, u->faction->locale, NULL);
    unit_addorder(u, create_order(K_KOMMENTAR, u->faction->locale, NULL));
    unit_addorder(u, ord = create_order(K_MOVE, u->faction->locale, NULL));
    update_long_orders();
    CuAssertIntEquals(tc, K_MOVE, getkeyword(u->thisorder));
    CuAssertIntEquals(tc, ord->id, u->thisorder->id);

    CuAssertIntEquals(tc, K_KOMMENTAR, getkeyword(u->orders));
    CuAssertPtrEquals(tc, ord, u->orders->next);
    CuAssertIntEquals(tc, K_MOVE, getkeyword(ord));
    CuAssertPtrEquals(tc, NULL, ord->next);

    CuAssertPtrNotNull(tc, u->defaults);
    CuAssertIntEquals(tc, K_WORK, getkeyword(u->defaults));
    CuAssertPtrEquals(tc, NULL, u->defaults->next);

    test_teardown();
}

CuSuite *get_defaults_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_defaultorders);
    SUITE_ADD_TEST(suite, test_long_order_normal);
    SUITE_ADD_TEST(suite, test_long_order_none);
    SUITE_ADD_TEST(suite, test_long_order_cast);
    SUITE_ADD_TEST(suite, test_long_order_attack);
    SUITE_ADD_TEST(suite, test_long_order_buy_sell);
    SUITE_ADD_TEST(suite, test_long_order_trade_and_other);
    SUITE_ADD_TEST(suite, test_long_order_multi_long);
    SUITE_ADD_TEST(suite, test_long_order_multi_buy);
    SUITE_ADD_TEST(suite, test_long_order_multi_sell);
    SUITE_ADD_TEST(suite, test_long_order_buy_cast);
    SUITE_ADD_TEST(suite, test_long_order_hungry);
    SUITE_ADD_TEST(suite, test_update_defaults);

    SUITE_ADD_TEST(suite, test_long_order_overwrites_defaults);
    SUITE_ADD_TEST(suite, test_move_keeps_defaults);

    return suite;
}
