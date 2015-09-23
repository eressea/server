#include <platform.h>
#include <kernel/config.h>
#include <kernel/curse.h>
#include <kernel/item.h>
#include <kernel/faction.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/spell.h>
#include <util/base36.h>
#include <util/language.h>
#include <util/attrib.h>
#include <spells/regioncurse.h>
#include <alchemy.h>
#include <laws.h>
#include <spells.h>
#include "unit.h"

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

static void test_names(CuTest *tc) {
    unit *u;

    test_cleanup();
    test_create_world();
    u = test_create_unit(test_create_faction(test_create_race("human")), findregion(0, 0));

    unit_setname(u, "Hodor");
    unit_setid(u, 5);
    CuAssertStrEquals(tc, "Hodor", unit_getname(u));
    CuAssertStrEquals(tc, "Hodor (5)", unitname(u));
    test_cleanup();
}

static void test_default_name(CuTest *tc) {
    unit *u;
    struct locale* lang;
    char buf[32], compare[32];

    test_cleanup();
    test_create_world();
    lang = get_or_create_locale("de");
    /* FIXME this has no real effect: default_name uses a static buffer that is initialized in some other test. This sucks. */
    locale_setstring(lang, "unitdefault", "Einheit");

    u = test_create_unit(test_create_faction(test_create_race("human")), findregion(0, 0));

    default_name(u, buf, sizeof(buf));

    sprintf(compare, "Einheit %s", itoa36(u->no));
    CuAssertStrEquals(tc, compare, buf);

    test_cleanup();
}

static void test_skill_hunger(CuTest *tc) {
    unit *u;

    test_cleanup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    set_level(u, SK_ARMORER, 6);
    set_level(u, SK_SAILING, 6);
    fset(u, UFL_HUNGER);

    set_param(&global.parameters, "rules.hunger.reduces_skill", "0");
    CuAssertIntEquals(tc, 6, effskill(u, SK_ARMORER));
    CuAssertIntEquals(tc, 6, effskill(u, SK_SAILING));

    set_param(&global.parameters, "rules.hunger.reduces_skill", "1");
    CuAssertIntEquals(tc, 3, effskill(u, SK_ARMORER));
    CuAssertIntEquals(tc, 3, effskill(u, SK_SAILING));

    set_param(&global.parameters, "rules.hunger.reduces_skill", "2");
    CuAssertIntEquals(tc, 3, effskill(u, SK_ARMORER));
    CuAssertIntEquals(tc, 5, effskill(u, SK_SAILING));
    set_level(u, SK_SAILING, 2);
    CuAssertIntEquals(tc, 1, effskill(u, SK_SAILING));
    test_cleanup();
}

static void test_skill_familiar(CuTest *tc) {
    unit *mag, *fam;
    region *r;

    test_cleanup();

    // setup two units
    mag = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    fam = test_create_unit(mag->faction, test_create_region(0, 0, 0));
    set_level(fam, SK_PERCEPTION, 6);
    CuAssertIntEquals(tc, 6, effskill(fam, SK_PERCEPTION));
    set_level(mag, SK_PERCEPTION, 6);
    CuAssertIntEquals(tc, 6, effskill(mag, SK_PERCEPTION));

    // make them mage and familiar to each other
    CuAssertIntEquals(tc, true, create_newfamiliar(mag, fam));

    // when they are in the same region, the mage gets half their skill as a bonus
    CuAssertIntEquals(tc, 6, effskill(fam, SK_PERCEPTION));
    CuAssertIntEquals(tc, 9, effskill(mag, SK_PERCEPTION));

    // when they are further apart, divide bonus by distance
    r = test_create_region(3, 0, 0);
    move_unit(fam, r, &r->units);
    CuAssertIntEquals(tc, 7, effskill(mag, SK_PERCEPTION));
    test_cleanup();
}

static void test_age_familiar(CuTest *tc) {
    unit *mag, *fam;

    test_cleanup();

    // setup two units
    mag = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    fam = test_create_unit(mag->faction, test_create_region(0, 0, 0));
    CuAssertPtrEquals(tc, 0, get_familiar(mag));
    CuAssertPtrEquals(tc, 0, get_familiar_mage(fam));
    CuAssertIntEquals(tc, true, create_newfamiliar(mag, fam));
    CuAssertPtrEquals(tc, fam, get_familiar(mag));
    CuAssertPtrEquals(tc, mag, get_familiar_mage(fam));
    a_age(&fam->attribs);
    a_age(&mag->attribs);
    CuAssertPtrEquals(tc, fam, get_familiar(mag));
    CuAssertPtrEquals(tc, mag, get_familiar_mage(fam));
    set_number(fam, 0);
    a_age(&mag->attribs);
    CuAssertPtrEquals(tc, 0, get_familiar(mag));
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
    SUITE_ADD_TEST(suite, test_names);
    SUITE_ADD_TEST(suite, test_default_name);
    SUITE_ADD_TEST(suite, test_skill_hunger);
    SUITE_ADD_TEST(suite, test_skill_familiar);
    SUITE_ADD_TEST(suite, test_age_familiar);
    return suite;
}
