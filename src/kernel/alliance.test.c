#include <platform.h>
#include <kernel/config.h>
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/order.h>
#include <util/base36.h>
#include "alliance.h"
#include <CuTest.h>
#include <tests.h>
#include <selist.h>

#include <assert.h>

typedef struct alliance_fixture {
    struct race * rc;
    struct faction *f1, *f2;
} alliance_fixture;

static void setup_alliance(alliance_fixture *fix) {
    test_setup();
    fix->rc = test_create_race("human");
    fix->f1 = test_create_faction(fix->rc);
    fix->f2 = test_create_faction(fix->rc);
    assert(fix->rc && fix->f1 && fix->f2);
}

static void test_alliance_make(CuTest *tc) {
    alliance * al;

    test_setup();
    assert(!alliances);
    al = makealliance(1, "Hodor");
    CuAssertPtrNotNull(tc, al);
    CuAssertStrEquals(tc, "Hodor", al->name);
    CuAssertIntEquals(tc, 1, al->id);
    CuAssertIntEquals(tc, 0, al->flags);
    CuAssertPtrEquals(tc, NULL, al->members);
    CuAssertPtrEquals(tc, NULL, al->_leader);
    CuAssertPtrEquals(tc, NULL, al->allies);
    CuAssertPtrEquals(tc, al, findalliance(1));
    CuAssertPtrEquals(tc, al, alliances);
    free_alliances();
    CuAssertPtrEquals(tc, NULL, findalliance(1));
    CuAssertPtrEquals(tc, NULL, alliances);
    test_teardown();
}

static void test_alliance_join(CuTest *tc) {
    alliance_fixture fix;
    alliance * al;

    setup_alliance(&fix);
    CuAssertPtrEquals(tc, NULL, fix.f1->alliance);
    CuAssertPtrEquals(tc, NULL, fix.f2->alliance);
    al = makealliance(1, "Hodor");
    setalliance(fix.f1, al);
    CuAssertPtrEquals(tc, fix.f1, alliance_get_leader(al));
    setalliance(fix.f2, al);
    CuAssertPtrEquals(tc, fix.f1, alliance_get_leader(al));
    CuAssertTrue(tc, is_allied(fix.f1, fix.f2));
    setalliance(fix.f1, 0);
    CuAssertPtrEquals(tc, fix.f2, alliance_get_leader(al));
    CuAssertTrue(tc, !is_allied(fix.f1, fix.f2));
    test_teardown();
}

static void test_alliance_dead_faction(CuTest *tc) {
    faction *f, *f2;
    alliance *al;

    test_setup();
    f = test_create_faction(NULL);
    f2 = test_create_faction(NULL);
    al = makealliance(42, "Hodor");
    setalliance(f, al);
    setalliance(f2, al);
    CuAssertPtrEquals(tc, f, alliance_get_leader(al));
    CuAssertIntEquals(tc, 2, selist_length(al->members));
    CuAssertPtrEquals(tc, al, f->alliance);
    destroyfaction(&factions);
    CuAssertIntEquals(tc, 1, selist_length(al->members));
    CuAssertPtrEquals(tc, f2, alliance_get_leader(al));
    CuAssertPtrEquals(tc, NULL, f->alliance);
    CuAssertTrue(tc, !f->_alive);
    test_teardown();
}

static void test_alliance_cmd(CuTest *tc) {
    unit *u1, *u2;
    struct region *r;
    struct alliance *al;

    test_setup();
    r = test_create_region(0, 0, NULL);
    u1 = test_create_unit(test_create_faction(NULL), r);
    u2 = test_create_unit(test_create_faction(NULL), r);
    unit_addorder(u1, create_order(K_ALLIANCE, u1->faction->locale, "%s %s", alliance_kwd[ALLIANCE_NEW], itoa36(42)));
    unit_addorder(u1, create_order(K_ALLIANCE, u1->faction->locale, "%s %s", alliance_kwd[ALLIANCE_INVITE], itoa36(u2->faction->no)));
    unit_addorder(u2, create_order(K_ALLIANCE, u1->faction->locale, "%s %s", alliance_kwd[ALLIANCE_JOIN], itoa36(42)));
    CuAssertTrue(tc, is_allied(u1->faction, u1->faction));
    CuAssertTrue(tc, !is_allied(u1->faction, u2->faction));
    CuAssertPtrEquals(tc, NULL, f_get_alliance(u1->faction));
    alliance_cmd();
    al = f_get_alliance(u1->faction);
    CuAssertPtrNotNull(tc, al);
    CuAssertIntEquals(tc, 42, al->id);
    CuAssertPtrNotNull(tc, al->members);
    CuAssertPtrEquals(tc, u1->faction, alliance_get_leader(al));
    CuAssertPtrEquals(tc, al, findalliance(42));
    CuAssertTrue(tc, is_allied(u1->faction, u1->faction));
    CuAssertPtrEquals(tc, al, u2->faction->alliance);
    test_teardown();
}

static void test_alliance_limits(CuTest *tc) {
    unit *u1, *u2;
    struct region *r;

    test_setup();
    r = test_create_region(0, 0, NULL);
    u1 = test_create_unit(test_create_faction(NULL), r);
    u2 = test_create_unit(test_create_faction(NULL), r);

    config_set("rules.limit.alliance", "1");
    unit_addorder(u1, create_order(K_ALLIANCE, u1->faction->locale, "%s %s", alliance_kwd[ALLIANCE_NEW], itoa36(42)));
    unit_addorder(u1, create_order(K_ALLIANCE, u1->faction->locale, "%s %s", alliance_kwd[ALLIANCE_INVITE], itoa36(u2->faction->no)));
    unit_addorder(u2, create_order(K_ALLIANCE, u1->faction->locale, "%s %s", alliance_kwd[ALLIANCE_JOIN], itoa36(42)));
    CuAssertTrue(tc, !is_allied(u1->faction, u2->faction));
    CuAssertPtrEquals(tc, NULL, f_get_alliance(u1->faction));
    alliance_cmd();
    CuAssertPtrNotNull(tc, f_get_alliance(u1->faction));
    CuAssertPtrEquals(tc, NULL, f_get_alliance(u2->faction));
    CuAssertTrue(tc, !is_allied(u1->faction, u2->faction));
    CuAssertPtrNotNull(tc, test_find_messagetype(u2->faction->msgs, "too_many_units_in_alliance"));
    test_teardown();
}

static void test_alliance_cmd_kick(CuTest *tc) {
    unit *u1, *u2;
    struct region *r;
    struct alliance *al;

    test_setup();
    al = makealliance(42, "Hodor");
    r = test_create_region(0, 0, NULL);
    u1 = test_create_unit(test_create_faction(NULL), r);
    u2 = test_create_unit(test_create_faction(NULL), r);
    setalliance(u1->faction, al);
    setalliance(u2->faction, al);

    unit_addorder(u1, create_order(K_ALLIANCE, u1->faction->locale, "%s %s", alliance_kwd[ALLIANCE_KICK], itoa36(u2->faction->no)));
    CuAssertTrue(tc, is_allied(u1->faction, u2->faction));
    alliance_cmd();
    CuAssertTrue(tc, !is_allied(u1->faction, u2->faction));
    CuAssertPtrEquals(tc, NULL, f_get_alliance(u2->faction));
    test_teardown();
}

static void test_alliance_cmd_no_invite(CuTest *tc) {
    unit *u1, *u2;
    struct region *r;

    test_setup();
    r = test_create_region(0, 0, NULL);
    u1 = test_create_unit(test_create_faction(NULL), r);
    u2 = test_create_unit(test_create_faction(NULL), r);
    unit_addorder(u1, create_order(K_ALLIANCE, u1->faction->locale, "%s %s", alliance_kwd[ALLIANCE_NEW], itoa36(42)));
    unit_addorder(u2, create_order(K_ALLIANCE, u1->faction->locale, "%s %s", alliance_kwd[ALLIANCE_JOIN], itoa36(42)));
    CuAssertTrue(tc, is_allied(u1->faction, u1->faction));
    CuAssertTrue(tc, !is_allied(u1->faction, u2->faction));
    CuAssertPtrEquals(tc, NULL, f_get_alliance(u1->faction));
    alliance_cmd();
    CuAssertPtrNotNull(tc, f_get_alliance(u1->faction));
    CuAssertPtrEquals(tc, NULL, f_get_alliance(u2->faction));
    CuAssertTrue(tc, !is_allied(u1->faction, u2->faction));
    test_teardown();
}

static void test_alliance_cmd_leave(CuTest *tc) {
    unit *u1, *u2;
    struct region *r;
    struct alliance *al;

    test_setup();
    al = makealliance(42, "Hodor");
    r = test_create_region(0, 0, NULL);
    u1 = test_create_unit(test_create_faction(NULL), r);
    u2 = test_create_unit(test_create_faction(NULL), r);
    setalliance(u1->faction, al);
    setalliance(u2->faction, al);

    unit_addorder(u1, create_order(K_ALLIANCE, u1->faction->locale, "%s", alliance_kwd[ALLIANCE_LEAVE]));
    CuAssertTrue(tc, is_allied(u1->faction, u2->faction));
    alliance_cmd();
    CuAssertTrue(tc, !is_allied(u1->faction, u2->faction));
    CuAssertPtrEquals(tc, NULL, f_get_alliance(u1->faction));
    test_teardown();
}

static void test_alliance_cmd_transfer(CuTest *tc) {
    unit *u1, *u2;
    struct region *r;
    struct alliance *al;

    test_setup();
    al = makealliance(42, "Hodor");
    r = test_create_region(0, 0, NULL);
    u1 = test_create_unit(test_create_faction(NULL), r);
    u2 = test_create_unit(test_create_faction(NULL), r);
    setalliance(u1->faction, al);
    setalliance(u2->faction, al);
    CuAssertPtrEquals(tc, u1->faction, alliance_get_leader(al));
    unit_addorder(u1, create_order(K_ALLIANCE, u1->faction->locale, "%s %s", alliance_kwd[ALLIANCE_TRANSFER], itoa36(u2->faction->no)));
    alliance_cmd();
    CuAssertPtrEquals(tc, u2->faction, alliance_get_leader(al));
    test_teardown();
}

CuSuite *get_alliance_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_alliance_dead_faction);
    SUITE_ADD_TEST(suite, test_alliance_make);
    SUITE_ADD_TEST(suite, test_alliance_join);
    SUITE_ADD_TEST(suite, test_alliance_limits);
    SUITE_ADD_TEST(suite, test_alliance_cmd);
    SUITE_ADD_TEST(suite, test_alliance_cmd_no_invite);
    SUITE_ADD_TEST(suite, test_alliance_cmd_kick);
    SUITE_ADD_TEST(suite, test_alliance_cmd_leave);
    SUITE_ADD_TEST(suite, test_alliance_cmd_transfer);
    return suite;
}

