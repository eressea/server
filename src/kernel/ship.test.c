#include <platform.h>

#include <kernel/config.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/unit.h>

#include <CuTest.h>
#include <tests.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void test_register_ship(CuTest * tc)
{
    ship_type *stype;

    test_cleanup();

    stype = st_get_or_create("herp");
    CuAssertPtrNotNull(tc, stype);
    CuAssertPtrEquals(tc, stype, (void *)st_find("herp"));
}

static void test_ship_set_owner(CuTest * tc)
{
    struct region *r;
    struct ship *sh;
    struct unit *u1, *u2;
    struct faction *f;
    const struct ship_type *stype;
    const struct race *human;

    test_cleanup();
    test_create_world();

    human = rc_find("human");
    stype = st_find("boat");
    f = test_create_faction(human);
    r = findregion(0, 0);

    sh = test_create_ship(r, stype);
    u1 = test_create_unit(f, r);
    u_set_ship(u1, sh);
    CuAssertPtrEquals(tc, u1, ship_owner(sh));

    u2 = test_create_unit(f, r);
    u_set_ship(u2, sh);
    CuAssertPtrEquals(tc, u1, ship_owner(sh));
    ship_set_owner(u2);
    CuAssertPtrEquals(tc, u2, ship_owner(sh));
}

static void test_shipowner_goes_to_next_when_empty(CuTest * tc)
{
    struct region *r;
    struct ship *sh;
    struct unit *u, *u2;
    struct faction *f;
    const struct ship_type *stype;
    const struct race *human;

    test_cleanup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    stype = st_find("boat");
    CuAssertPtrNotNull(tc, stype);

    f = test_create_faction(human);
    r = findregion(0, 0);

    sh = test_create_ship(r, stype);
    CuAssertPtrNotNull(tc, sh);

    u = test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);
    u_set_ship(u, sh);
    u_set_ship(u2, sh);
    CuAssertPtrEquals(tc, u, ship_owner(sh));
    u->number = 0;
    CuAssertPtrEquals(tc, u2, ship_owner(sh));
}

static void test_shipowner_goes_to_other_when_empty(CuTest * tc)
{
    struct region *r;
    struct ship *sh;
    struct unit *u, *u2;
    struct faction *f;
    const struct ship_type *stype;
    const struct race *human;

    test_cleanup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    stype = st_find("boat");
    CuAssertPtrNotNull(tc, stype);

    f = test_create_faction(human);
    r = findregion(0, 0);

    sh = test_create_ship(r, stype);
    CuAssertPtrNotNull(tc, sh);

    u2 = test_create_unit(f, r);
    u = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);
    u_set_ship(u, sh);
    u_set_ship(u2, sh);
    CuAssertPtrEquals(tc, u, ship_owner(sh));
    u->number = 0;
    CuAssertPtrEquals(tc, u2, ship_owner(sh));
}

static void test_shipowner_goes_to_same_faction_when_empty(CuTest * tc)
{
    struct region *r;
    struct ship *sh;
    struct unit *u, *u2, *u3;
    struct faction *f1, *f2;
    const struct ship_type *stype;
    const struct race *human;

    test_cleanup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    stype = st_find("boat");
    CuAssertPtrNotNull(tc, stype);

    f1 = test_create_faction(human);
    f2 = test_create_faction(human);
    r = findregion(0, 0);

    sh = test_create_ship(r, stype);
    CuAssertPtrNotNull(tc, sh);

    u2 = test_create_unit(f2, r);
    u3 = test_create_unit(f1, r);
    u = test_create_unit(f1, r);
    CuAssertPtrNotNull(tc, u);
    u_set_ship(u, sh);
    u_set_ship(u2, sh);
    u_set_ship(u3, sh);
    CuAssertPtrEquals(tc, u, ship_owner(sh));
    u->number = 0;
    CuAssertPtrEquals(tc, u3, ship_owner(sh));
    u3->number = 0;
    CuAssertPtrEquals(tc, u2, ship_owner(sh));
}

static void test_shipowner_goes_to_next_after_leave(CuTest * tc)
{
    struct region *r;
    struct ship *sh;
    struct unit *u, *u2;
    struct faction *f;
    const struct ship_type *stype;
    const struct race *human;

    test_cleanup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    stype = st_find("boat");
    CuAssertPtrNotNull(tc, stype);

    f = test_create_faction(human);
    r = findregion(0, 0);

    sh = test_create_ship(r, stype);
    CuAssertPtrNotNull(tc, sh);

    u = test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);
    u_set_ship(u, sh);
    u_set_ship(u2, sh);
    CuAssertPtrEquals(tc, u, ship_owner(sh));
    leave_ship(u);
    CuAssertPtrEquals(tc, u2, ship_owner(sh));
}

static void test_shipowner_goes_to_other_after_leave(CuTest * tc)
{
    struct region *r;
    struct ship *sh;
    struct unit *u, *u2;
    struct faction *f;
    const struct ship_type *stype;
    const struct race *human;

    test_cleanup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    stype = st_find("boat");
    CuAssertPtrNotNull(tc, stype);

    f = test_create_faction(human);
    r = findregion(0, 0);

    sh = test_create_ship(r, stype);
    CuAssertPtrNotNull(tc, sh);

    u2 = test_create_unit(f, r);
    u = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);
    u_set_ship(u, sh);
    u_set_ship(u2, sh);
    CuAssertPtrEquals(tc, u, ship_owner(sh));
    leave_ship(u);
    CuAssertPtrEquals(tc, u2, ship_owner(sh));
}

static void test_shipowner_goes_to_same_faction_after_leave(CuTest * tc)
{
    struct region *r;
    struct ship *sh;
    struct unit *u, *u2, *u3;
    struct faction *f1, *f2;
    const struct ship_type *stype;
    const struct race *human;

    test_cleanup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    stype = st_find("boat");
    CuAssertPtrNotNull(tc, stype);

    f1 = test_create_faction(human);
    f2 = test_create_faction(human);
    r = findregion(0, 0);

    sh = test_create_ship(r, stype);
    CuAssertPtrNotNull(tc, sh);

    u2 = test_create_unit(f2, r);
    u3 = test_create_unit(f1, r);
    u = test_create_unit(f1, r);
    CuAssertPtrNotNull(tc, u);
    u_set_ship(u, sh);
    u_set_ship(u2, sh);
    u_set_ship(u3, sh);
    CuAssertPtrEquals(tc, u, ship_owner(sh));
    leave_ship(u);
    CuAssertPtrEquals(tc, u3, ship_owner(sh));
    leave_ship(u3);
    CuAssertPtrEquals(tc, u2, ship_owner(sh));
    leave_ship(u2);
    CuAssertPtrEquals(tc, 0, ship_owner(sh));
}

static void test_shipowner_resets_when_empty(CuTest * tc)
{
    struct region *r;
    struct ship *sh;
    struct unit *u;
    struct faction *f;
    const struct ship_type *stype;
    const struct race *human;

    test_cleanup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    stype = st_find("boat");
    CuAssertPtrNotNull(tc, stype);

    f = test_create_faction(human);
    r = findregion(0, 0);

    sh = test_create_ship(r, stype);
    CuAssertPtrNotNull(tc, sh);

    u = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);
    u_set_ship(u, sh);
    CuAssertPtrEquals(tc, u, ship_owner(sh));
    u->number = 0;
    CuAssertPtrEquals(tc, 0, ship_owner(sh));
    u->number = 1;
    CuAssertPtrEquals(tc, u, ship_owner(sh));
}

void test_shipowner_goes_to_empty_unit_after_leave(CuTest * tc)
{
    struct region *r;
    struct ship *sh;
    struct unit *u1, *u2, *u3;
    struct faction *f1;
    const struct ship_type *stype;
    const struct race *human;

    test_cleanup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    stype = st_find("boat");
    CuAssertPtrNotNull(tc, stype);

    f1 = test_create_faction(human);
    r = findregion(0, 0);

    sh = test_create_ship(r, stype);
    CuAssertPtrNotNull(tc, sh);

    u1 = test_create_unit(f1, r);
    u2 = test_create_unit(f1, r);
    u3 = test_create_unit(f1, r);
    u_set_ship(u1, sh);
    u_set_ship(u2, sh);
    u_set_ship(u3, sh);

    CuAssertPtrEquals(tc, u1, ship_owner(sh));
    u2->number = 0;
    leave_ship(u1);
    CuAssertPtrEquals(tc, u3, ship_owner(sh));
    leave_ship(u3);
    CuAssertPtrEquals(tc, 0, ship_owner(sh));
    u2->number = 1;
    CuAssertPtrEquals(tc, u2, ship_owner(sh));
}

static void test_stype_defaults(CuTest *tc) {
    ship_type *stype;
    test_cleanup();
    stype = st_get_or_create("hodor");
    CuAssertPtrNotNull(tc, stype);
    CuAssertStrEquals(tc, "hodor", stype->_name);
    CuAssertPtrEquals(tc, 0, stype->construction);
    CuAssertPtrEquals(tc, 0, stype->coasts);
    CuAssertDblEquals(tc, 0.0, stype->damage, 0.0);
    CuAssertDblEquals(tc, 1.0, stype->storm, 0.0);
    CuAssertDblEquals(tc, 0.0, stype->tac_bonus, 0.0);
    CuAssertIntEquals(tc, 0, stype->cabins);
    CuAssertIntEquals(tc, 0, stype->cargo);
    CuAssertIntEquals(tc, 0, stype->combat);
    CuAssertIntEquals(tc, 0, stype->fishing);
    CuAssertIntEquals(tc, 0, stype->range);
    CuAssertIntEquals(tc, 0, stype->cptskill);
    CuAssertIntEquals(tc, 0, stype->minskill);
    CuAssertIntEquals(tc, 0, stype->sumskill);
    CuAssertIntEquals(tc, 0, stype->at_bonus);
    CuAssertIntEquals(tc, 0, stype->df_bonus);
    CuAssertIntEquals(tc, 0, stype->flags);
    test_cleanup();
}

static void test_crew_skill(CuTest *tc) {
    ship *sh;
    region *r;
    struct faction *f;
    int i;

    test_cleanup();
    test_create_world();
    r = findregion(0, 0);
    f = test_create_faction(0);
    assert(r && f);
    sh = test_create_ship(r, st_find("boat"));
    for (i = 0; i != 4; ++i) {
        unit * u = test_create_unit(f, r);
        set_level(u, SK_SAILING, 5);
        u->ship = sh;
    }
    CuAssertIntEquals(tc, 20, crew_skill(sh));
    test_cleanup();
}

static void test_shipspeed(CuTest *tc) {
    ship *sh;
    ship_type *stype;
    region *r;
    struct faction *f;
    unit *cap, *crew;

    test_cleanup();
    test_create_world();
    r = test_create_region(0, 0, test_create_terrain("ocean", 0));
    f = test_create_faction(0);
    assert(r && f);
    stype = test_create_shiptype("longboat");
    stype->cptskill = 1;
    stype->sumskill = 10;
    stype->minskill = 1;
    stype->range = 2;
    stype->range_max = 4;
    sh = test_create_ship(r, stype);
    cap = test_create_unit(f, r);
    crew = test_create_unit(f, r);
    cap->ship = sh;
    set_level(cap, SK_SAILING, stype->cptskill + 10);
    set_level(crew, SK_SAILING, stype->sumskill * 5);
    crew->ship = sh;
    set_param(&global.parameters, "movement.shipspeed.skillbonus", "5");
    CuAssertIntEquals(tc, 2+stype->range, shipspeed(sh, cap));
    test_cleanup();
}

CuSuite *get_ship_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_register_ship);
    SUITE_ADD_TEST(suite, test_stype_defaults);
    SUITE_ADD_TEST(suite, test_ship_set_owner);
    SUITE_ADD_TEST(suite, test_shipowner_resets_when_empty);
    SUITE_ADD_TEST(suite, test_shipowner_goes_to_next_when_empty);
    SUITE_ADD_TEST(suite, test_shipowner_goes_to_other_when_empty);
    SUITE_ADD_TEST(suite, test_shipowner_goes_to_same_faction_when_empty);
    SUITE_ADD_TEST(suite, test_shipowner_goes_to_next_after_leave);
    SUITE_ADD_TEST(suite, test_shipowner_goes_to_other_after_leave);
    SUITE_ADD_TEST(suite, test_shipowner_goes_to_same_faction_after_leave);
    SUITE_ADD_TEST(suite, test_shipowner_goes_to_empty_unit_after_leave);
    SUITE_ADD_TEST(suite, test_crew_skill);
    SUITE_ADD_TEST(suite, test_shipspeed);
    return suite;
}
