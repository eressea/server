#include <platform.h>
#include <kernel/config.h>
#include <util/base36.h>
#include <util/language.h>
#include "alchemy.h"
#include "faction.h"
#include "unit.h"
#include "item.h"
#include "race.h"
#include "region.h"

#include <CuTest.h>
#include <tests.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void test_remove_empty_units(CuTest *tc) {
    unit *u;
    int uid;

    test_cleanup();
    test_create_world();

    u = test_create_unit(test_create_faction(test_create_race("human")), findregion(0, 0));
    uid = u->no;
    remove_empty_units();
    CuAssertPtrNotNull(tc, findunit(uid));
    u->number = 0;
    remove_empty_units();
    CuAssertPtrEquals(tc, 0, findunit(uid));
    test_cleanup();
}

static void test_remove_empty_units_in_region(CuTest *tc) {
    unit *u;
    int uid;

    test_cleanup();
    test_create_world();

    u = test_create_unit(test_create_faction(test_create_race("human")), findregion(0, 0));
    uid = u->no;
    remove_empty_units_in_region(u->region);
    CuAssertPtrNotNull(tc, findunit(uid));
    u->number = 0;
    remove_empty_units_in_region(u->region);
    CuAssertPtrEquals(tc, 0, findunit(uid));
    CuAssertPtrEquals(tc, 0, u->region);
    CuAssertPtrEquals(tc, 0, u->faction);
    test_cleanup();
}

static void test_remove_units_without_faction(CuTest *tc) {
    unit *u;
    int uid;

    test_cleanup();
    test_create_world();

    u = test_create_unit(test_create_faction(test_create_race("human")), findregion(0, 0));
    uid = u->no;
    u_setfaction(u, 0);
    remove_empty_units_in_region(u->region);
    CuAssertPtrEquals(tc, 0, findunit(uid));
    CuAssertIntEquals(tc, 0, u->number);
    test_cleanup();
}

static void test_remove_units_with_dead_faction(CuTest *tc) {
    unit *u;
    int uid;

    test_cleanup();
    test_create_world();

    u = test_create_unit(test_create_faction(test_create_race("human")), findregion(0, 0));
    uid = u->no;
    u->faction->alive = false;
    remove_empty_units_in_region(u->region);
    CuAssertPtrEquals(tc, 0, findunit(uid));
    CuAssertIntEquals(tc, 0, u->number);
    test_cleanup();
}

static void test_remove_units_ignores_spells(CuTest *tc) {
    unit *u;
    int uid;

    test_cleanup();
    test_create_world();

    u = create_unit(findregion(0, 0), test_create_faction(test_create_race("human")), 1, get_race(RC_SPELL), 0, 0, 0);
    uid = u->no;
    u->number = 0;
    u->age = 1;
    remove_empty_units_in_region(u->region);
    CuAssertPtrNotNull(tc, findunit(uid));
    CuAssertPtrNotNull(tc, u->region);
    u->age = 0;
    remove_empty_units_in_region(u->region);
    CuAssertPtrEquals(tc, 0, findunit(uid));
    test_cleanup();
}

static void test_scale_number(CuTest *tc) {
    unit *u;
    const struct potion_type *ptype;

    test_cleanup();
    test_create_world();
    ptype = new_potiontype(it_get_or_create(rt_get_or_create("hodor")), 1);
    u = test_create_unit(test_create_faction(test_create_race("human")), findregion(0, 0));
    change_effect(u, ptype, 1);
    CuAssertIntEquals(tc, 1, u->number);
    CuAssertIntEquals(tc, 1, u->hp);
    CuAssertIntEquals(tc, 1, get_effect(u, ptype));
    scale_number(u, 2);
    CuAssertIntEquals(tc, 2, u->number);
    CuAssertIntEquals(tc, 2, u->hp);
    CuAssertIntEquals(tc, 2, get_effect(u, ptype));
    set_level(u, SK_ALCHEMY, 1);
    scale_number(u, 0);
    CuAssertIntEquals(tc, 0, get_level(u, SK_ALCHEMY));
    test_cleanup();
}

static void test_unit_name(CuTest *tc) {
    unit *u;
    char name[32];

    test_cleanup();
    test_create_world();
    u = test_create_unit(test_create_faction(test_create_race("human")), findregion(0, 0));
    unit_setname(u, "Hodor");
    _snprintf(name, sizeof(name), "Hodor (%s)", itoa36(u->no));
    CuAssertStrEquals(tc, name, unitname(u));
    test_cleanup();
}

static void test_unit_name_from_race(CuTest *tc) {
    unit *u;
    char name[32];
    struct locale *lang;

    test_cleanup();
    test_create_world();
    u = test_create_unit(test_create_faction(test_create_race("human")), findregion(0, 0));
    unit_setname(u, NULL);
    lang = get_or_create_locale("de");
    locale_setstring(lang, rc_name_s(u->_race, NAME_SINGULAR), "Mensch");
    locale_setstring(lang, rc_name_s(u->_race, NAME_PLURAL), "Menschen");

    _snprintf(name, sizeof(name), "Mensch (%s)", itoa36(u->no));
    CuAssertStrEquals(tc, name, unitname(u));
    CuAssertStrEquals(tc, "Mensch", unit_getname(u));

    u->number = 2;
    _snprintf(name, sizeof(name), "Menschen (%s)", itoa36(u->no));
    CuAssertStrEquals(tc, name, unitname(u));
    CuAssertStrEquals(tc, "Menschen", unit_getname(u));

    test_cleanup();
}

static void test_update_monster_name(CuTest *tc) {
    unit *u;
    struct locale *lang;

    test_cleanup();
    test_create_world();
    u = test_create_unit(test_create_faction(test_create_race("human")), findregion(0, 0));
    lang = get_or_create_locale("de");
    locale_setstring(lang, rc_name_s(u->_race, NAME_SINGULAR), "Mensch");
    locale_setstring(lang, rc_name_s(u->_race, NAME_PLURAL), "Menschen");

    unit_setname(u, "Hodor");
    CuAssertTrue(tc, !unit_name_equals_race(u));

    unit_setname(u, "Menschling");
    CuAssertTrue(tc, !unit_name_equals_race(u));

    unit_setname(u, rc_name_s(u->_race, NAME_SINGULAR));
    CuAssertTrue(tc, unit_name_equals_race(u));

    unit_setname(u, rc_name_s(u->_race, NAME_PLURAL));
    CuAssertTrue(tc, unit_name_equals_race(u));

    unit_setname(u, "Mensch");
    CuAssertTrue(tc, unit_name_equals_race(u));

    unit_setname(u, "Menschen");
    CuAssertTrue(tc, unit_name_equals_race(u));

    test_cleanup();
}

CuSuite *get_unit_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_scale_number);
    SUITE_ADD_TEST(suite, test_unit_name);
    SUITE_ADD_TEST(suite, test_unit_name_from_race);
    SUITE_ADD_TEST(suite, test_update_monster_name);
    SUITE_ADD_TEST(suite, test_remove_empty_units);
    SUITE_ADD_TEST(suite, test_remove_units_ignores_spells);
    SUITE_ADD_TEST(suite, test_remove_units_without_faction);
    SUITE_ADD_TEST(suite, test_remove_units_with_dead_faction);
    SUITE_ADD_TEST(suite, test_remove_empty_units_in_region);
    return suite;
}
