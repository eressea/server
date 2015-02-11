#include <platform.h>
#include <stdlib.h>
#include "move.h"

#include <kernel/ally.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

#include <util/language.h>

#include <CuTest.h>
#include <tests.h>

static void test_ship_not_allowed_in_coast(CuTest * tc)
{
    region *r1, *r2;
    ship * sh;
    terrain_type *ttype, *otype;
    ship_type *stype;

    test_cleanup();
    test_create_world();

    ttype = test_create_terrain("glacier", LAND_REGION | ARCTIC_REGION | WALK_INTO | SAIL_INTO);
    otype = test_create_terrain("ocean", SEA_REGION | SAIL_INTO);
    stype = test_create_shiptype("derp");
    stype->coasts = (struct terrain_type **)calloc(2, sizeof(struct terrain_type *));

    r1 = test_create_region(0, 0, ttype);
    r2 = test_create_region(1, 0, otype);
    sh = test_create_ship(0, stype);

    CuAssertIntEquals(tc, SA_COAST, check_ship_allowed(sh, r2));
    CuAssertIntEquals(tc, SA_NO_COAST, check_ship_allowed(sh, r1));
    stype->coasts[0] = ttype;
    CuAssertIntEquals(tc, SA_COAST, check_ship_allowed(sh, r1));
}

typedef struct move_fixture {
    region *r;
    ship *sh;
    building * b;
    unit *u;
} move_fixture;

static void setup_harbor(move_fixture *mf) {
    region *r;
    ship * sh;
    terrain_type * ttype;
    building_type * btype;
    building * b;
    unit *u;

    test_cleanup();
    test_create_world();

    ttype = test_create_terrain("glacier", LAND_REGION | ARCTIC_REGION | WALK_INTO | SAIL_INTO);
    btype = test_create_buildingtype("harbour");

    sh = test_create_ship(0, 0);
    r = test_create_region(0, 0, ttype);

    b = test_create_building(r, btype);
    b->flags |= BLD_WORKING;

    u = test_create_unit(test_create_faction(0), r);
    u->ship = sh;
    ship_set_owner(u);

    mf->r = r;
    mf->u = u;
    mf->sh = sh;
    mf->b = b;
}

static void test_ship_allowed_without_harbormaster(CuTest * tc)
{
    move_fixture mf;

    setup_harbor(&mf);

    CuAssertIntEquals(tc, SA_HARBOUR, check_ship_allowed(mf.sh, mf.r));
}

static void test_ship_blocked_by_harbormaster(CuTest * tc) {
    unit *u;
    move_fixture mf;

    setup_harbor(&mf);

    u = test_create_unit(test_create_faction(0), mf.r);
    u->building = mf.b;
    building_set_owner(u);

    CuAssertIntEquals_Msg(tc, "harbor master must contact ship", SA_NO_COAST, check_ship_allowed(mf.sh, mf.r));
    test_cleanup();
}

static void test_ship_has_harbormaster_contact(CuTest * tc) {
    unit *u;
    move_fixture mf;

    setup_harbor(&mf);

    u = test_create_unit(test_create_faction(0), mf.r);
    u->building = mf.b;
    building_set_owner(u);
    usetcontact(mf.b->_owner, mf.sh->_owner);

    CuAssertIntEquals(tc, SA_HARBOUR, check_ship_allowed(mf.sh, mf.r));
    test_cleanup();
}

static void test_ship_has_harbormaster_same_faction(CuTest * tc) {
    unit *u;
    move_fixture mf;

    setup_harbor(&mf);

    u = test_create_unit(mf.u->faction, mf.r);
    u->building = mf.b;
    building_set_owner(u);

    CuAssertIntEquals(tc, SA_HARBOUR, check_ship_allowed(mf.sh, mf.r));
    test_cleanup();
}

static void test_ship_has_harbormaster_ally(CuTest * tc) {
    unit *u;
    move_fixture mf;
    ally *al;

    setup_harbor(&mf);

    u = test_create_unit(test_create_faction(0), mf.r);
    u->building = mf.b;
    building_set_owner(u);
    al = ally_add(&u->faction->allies, mf.u->faction);
    al->status = HELP_GUARD;

    CuAssertIntEquals(tc, SA_HARBOUR, check_ship_allowed(mf.sh, mf.r));
    test_cleanup();
}

static void test_building_type_exists(CuTest * tc)
{
    region *r;
    building *b;
    building_type *btype, *btype2;

    test_cleanup();
    test_create_world();

    btype2 = bt_get_or_create("lighthouse");
    btype = bt_get_or_create("castle");

    r = findregion(-1, 0);
    b = new_building(btype, r, default_locale);

    CuAssertPtrNotNull(tc, b);
    CuAssertTrue(tc, !buildingtype_exists(r, NULL, false));
    CuAssertTrue(tc, buildingtype_exists(r, btype, false));
    CuAssertTrue(tc, !buildingtype_exists(r, btype2, false));
}

CuSuite *get_move_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_building_type_exists);
    SUITE_ADD_TEST(suite, test_ship_not_allowed_in_coast);
    SUITE_ADD_TEST(suite, test_ship_allowed_without_harbormaster);
    SUITE_ADD_TEST(suite, test_ship_blocked_by_harbormaster);
    SUITE_ADD_TEST(suite, test_ship_has_harbormaster_contact);
    SUITE_ADD_TEST(suite, test_ship_has_harbormaster_ally);
    SUITE_ADD_TEST(suite, test_ship_has_harbormaster_same_faction);
    return suite;
}
