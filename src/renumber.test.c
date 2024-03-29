#include "renumber.h"

#include <tests.h>
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/building.h>
#include <kernel/direction.h>
#include <kernel/ship.h>
#include <kernel/order.h>
#include <util/base36.h>
#include "util/keyword.h"     // for K_NUMBER
#include <util/message.h>
#include <util/param.h>

#include <stddef.h>
#include <CuTest.h>

static void setup_renumber(CuTest *tc) {
    test_setup_ex(tc);
    mt_create_error(114);
    mt_create_error(115);
    mt_create_error(116);
}

static void test_renumber_faction(CuTest *tc) {
    unit *u;
    int uno, no;
    const struct locale *lang;

    setup_renumber(tc);
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    no = u->faction->no;
    uno = (no > 1) ? no - 1 : no + 1;
    lang = u->faction->locale;
    u->thisorder = create_order(K_NUMBER, lang, "%s %s", param_name(P_FACTION, lang), itoa36(uno));
    renumber_cmd(u, u->thisorder);
    renumber_factions();
    CuAssertIntEquals(tc, uno, u->faction->no);
    test_teardown();
}

static void test_renumber_faction_duplicate(CuTest *tc) {
    unit *u;
    faction *f, *f2;
    int no;
    const struct locale *lang;

    setup_renumber(tc);
    mt_create_va(mt_new("renumber_inuse", NULL), "id:int", MT_NEW_END);
    f2 = test_create_faction();
    u = test_create_unit(f = test_create_faction(), test_create_plain(0, 0));
    no = f->no;
    lang = f->locale;
    u->thisorder = create_order(K_NUMBER, lang, "%s %s", param_name(P_FACTION, lang), itoa36(f2->no));
    renumber_cmd(u, u->thisorder);
    renumber_factions();
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "renumber_inuse"));
    CuAssertIntEquals(tc, no, u->faction->no);
    test_teardown();
}

static void test_renumber_faction_invalid(CuTest *tc) {
    unit *u;
    faction *f;
    int no;
    const struct locale *lang;

    setup_renumber(tc);
    u = test_create_unit(f = test_create_faction(), test_create_region(0, 0, 0));
    no = f->no;
    lang = f->locale;
    u->thisorder = create_order(K_NUMBER, lang, "%s [halima]", param_name(P_FACTION, lang));
    renumber_cmd(u, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error114"));
    renumber_factions();
    CuAssertIntEquals(tc, no, f->no);

    test_clear_messages(f);
    free_order(u->thisorder);
    u->thisorder = create_order(K_NUMBER, lang, "%s 10000", param_name(P_FACTION, lang));
    renumber_cmd(u, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error114"));

    test_clear_messages(f);
    free_order(u->thisorder);
    u->thisorder = create_order(K_NUMBER, lang, "%s 0", param_name(P_FACTION, lang));
    renumber_cmd(u, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error114"));

    test_teardown();
}

static void test_renumber_building(CuTest *tc) {
    unit *u;
    int uno, no;
    const struct locale *lang;

    setup_renumber(tc);
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u->building = test_create_building(u->region, NULL);
    no = u->building->no;
    uno = (no > 1) ? no - 1 : no + 1;
    lang = u->faction->locale;
    u->thisorder = create_order(K_NUMBER, lang, param_name(P_BUILDING, lang));
    renumber_cmd(u, u->thisorder);
    CuAssertTrue(tc, no != u->building->no);
    free_order(u->thisorder);

    u->thisorder = create_order(K_NUMBER, lang, "%s %i", param_name(P_BUILDING, lang), uno);
    renumber_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, uno, u->building->no);
    test_teardown();
}

static void test_renumber_building_duplicate(CuTest *tc) {
    unit *u;
    faction *f;
    int uno, no;
    const struct locale *lang;

    setup_renumber(tc);
    u = test_create_unit(f = test_create_faction(), test_create_plain(0, 0));
    u->building = test_create_building(u->region, NULL);
    uno = u->building->no;
    u->building = test_create_building(u->region, NULL);
    no = u->building->no;
    lang = f->locale;
    u->thisorder = create_order(K_NUMBER, lang, "%s %i", param_name(P_BUILDING, lang), uno);
    renumber_cmd(u, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error115"));
    CuAssertIntEquals(tc, no, u->building->no);
    test_teardown();
}

static void test_renumber_ship(CuTest* tc) {
    unit* u;
    int uno, no;
    const struct locale* lang;

    setup_renumber(tc);
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u->ship = test_create_ship(u->region, NULL);
    no = u->ship->no;
    uno = (no > 1) ? no - 1 : no + 1;
    lang = u->faction->locale;
    u->thisorder = create_order(K_NUMBER, lang, "%s %s", param_name(P_SHIP, lang), itoa36(uno));
    renumber_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, uno, u->ship->no);
    test_teardown();
}

static void test_renumber_ship_not_owner(CuTest *tc) {
    unit *u, *u2;
    int uno, no;
    const struct locale *lang;

    setup_renumber(tc);
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u2 = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u->ship = test_create_ship(u->region, NULL);
    no = u->ship->no;
    uno = (no > 1) ? no - 1 : no + 1;
    lang = u->faction->locale;
    u2->thisorder = create_order(K_NUMBER, lang, "%s %s", param_name(P_SHIP, lang), itoa36(uno));
    renumber_cmd(u2, u2->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(u2->faction->msgs, "error144"));
    CuAssertIntEquals(tc, no, u->ship->no);
    u2->ship = u->ship;
    renumber_cmd(u2, u2->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(u2->faction->msgs, "error146"));
    CuAssertIntEquals(tc, no, u->ship->no);
    test_teardown();
}

static void test_renumber_moved_ship(CuTest* tc) {
    unit* u;
    int uno, no;
    const struct locale* lang;

    setup_renumber(tc);
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u->ship = test_create_ship(u->region, NULL);
    u->ship->coast = D_EAST;
    no = u->ship->no;
    uno = (no > 1) ? no - 1 : no + 1;
    lang = u->faction->locale;
    u->thisorder = create_order(K_NUMBER, lang, "%s %s", param_name(P_SHIP, lang), itoa36(uno));
    renumber_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, uno, u->ship->no);
    test_teardown();
}

static void test_renumber_ship_twice(CuTest *tc) {
    unit *u;
    int uno, no;
    const struct locale *lang;

    setup_renumber(tc);
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u->ship = test_create_ship(u->region, NULL);
    no = u->ship->no;
    uno = (no > 1) ? no - 1 : no + 1;
    lang = u->faction->locale;
    u->thisorder = create_order(K_NUMBER, lang, "%s %s", param_name(P_SHIP, lang), itoa36(uno));
    renumber_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, uno, u->ship->no);
    free_order(u->thisorder);
    u->thisorder = create_order(K_NUMBER, lang, "%s %s", param_name(P_SHIP, lang), itoa36(no));
    renumber_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, no, u->ship->no);
    test_teardown();
}

static void test_renumber_ship_duplicate(CuTest *tc) {
    unit *u;
    faction *f;
    int uno, no;
    const struct locale *lang;

    setup_renumber(tc);
    u = test_create_unit(f = test_create_faction(), test_create_plain(0, 0));
    u->ship = test_create_ship(u->region, NULL);
    uno = u->ship->no;
    u->ship = test_create_ship(u->region, NULL);
    no = u->ship->no;
    lang = f->locale;
    u->thisorder = create_order(K_NUMBER, lang, "%s %s", param_name(P_SHIP, lang), itoa36(uno));
    renumber_cmd(u, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error115"));
    CuAssertIntEquals(tc, no, u->ship->no);
    test_teardown();
}

static void test_renumber_unit(CuTest *tc) {
    unit *u;
    int uno, no;
    const struct locale *lang;

    setup_renumber(tc);
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    no = u->no;
    uno = (no > 1) ? no - 1 : no + 1;
    lang = u->faction->locale;
    u->thisorder = create_order(K_NUMBER, lang, "%s %s", param_name(P_UNIT, lang), itoa36(uno));
    renumber_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, uno, u->no);
    CuAssertIntEquals(tc, -no, ualias(u));
    test_teardown();
}

static void test_renumber_unit_duplicate(CuTest *tc) {
    unit *u, *u2;
    faction *f;
    int no;
    const struct locale *lang;

    setup_renumber(tc);
    u = test_create_unit(f = test_create_faction(), test_create_plain(0, 0));
    no = u->no;
    u2 = test_create_unit(f, u->region);
    lang = f->locale;
    u->thisorder = create_order(K_NUMBER, lang, "%s %s", param_name(P_UNIT, lang), itoa36(u2->no));
    renumber_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, no, u->no);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error115"));
    CuAssertIntEquals(tc, 0, ualias(u));
    test_teardown();
}

static void test_renumber_unit_limit(CuTest *tc) {
    unit *u;
    faction *f;
    int no;
    const struct locale *lang;

    setup_renumber(tc);
    u = test_create_unit(f = test_create_faction(), test_create_plain(0, 0));
    no = u->no;
    lang = f->locale;
    u->thisorder = create_order(K_NUMBER, lang, "%s 10000", param_name(P_UNIT, lang));
    renumber_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, no, u->no);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error114"));
    CuAssertIntEquals(tc, 0, ualias(u));
    test_teardown();
}

static void test_renumber_unit_invalid(CuTest *tc) {
    unit *u;
    faction *f;
    int no;
    const struct locale *lang;

    setup_renumber(tc);
    u = test_create_unit(f = test_create_faction(), test_create_plain(0, 0));
    no = u->no;
    lang = f->locale;
    u->thisorder = create_order(K_NUMBER, lang, "%s TEMP", param_name(P_UNIT, lang));
    renumber_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, no, u->no);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error116"));
    CuAssertIntEquals(tc, 0, ualias(u));
    test_teardown();
}

CuSuite *get_renumber_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_renumber_unit);
    SUITE_ADD_TEST(suite, test_renumber_unit_limit);
    SUITE_ADD_TEST(suite, test_renumber_unit_duplicate);
    SUITE_ADD_TEST(suite, test_renumber_unit_invalid);
    SUITE_ADD_TEST(suite, test_renumber_building);
    SUITE_ADD_TEST(suite, test_renumber_building_duplicate);
    SUITE_ADD_TEST(suite, test_renumber_ship);
    SUITE_ADD_TEST(suite, test_renumber_ship_not_owner);
    SUITE_ADD_TEST(suite, test_renumber_moved_ship);
    SUITE_ADD_TEST(suite, test_renumber_ship_twice);
    SUITE_ADD_TEST(suite, test_renumber_ship_duplicate);
    SUITE_ADD_TEST(suite, test_renumber_faction);
    SUITE_ADD_TEST(suite, test_renumber_faction_duplicate);
    SUITE_ADD_TEST(suite, test_renumber_faction_invalid);
    return suite;
}
