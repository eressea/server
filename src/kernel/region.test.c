#include "region.h"

#include "direction.h"     // for D_NORTHWEST
#include "build.h"  // for construction
#include "building.h"
#include "calendar.h"
#include "config.h"
#include "connection.h"
#include "resources.h"
#include "unit.h"
#include "terrain.h"
#include "terrainid.h"
#include "item.h"


#include <CuTest.h>
#include <tests.h>
#include <stdlib.h>

void test_newterrain(CuTest *tc) {
    terrain_type *t_plain;

    test_setup();
    init_terrains();
    t_plain = get_or_create_terrain("plain");
    CuAssertPtrEquals(tc, t_plain, (void *)newterrain(T_PLAIN));
}

void test_terraform(CuTest *tc) {
    region *r;
    terrain_type *t_plain, *t_ocean;
    item_type *itype;

    test_setup();
    itype = test_create_itemtype("oil");
    itype->rtype->flags |= (RTF_ITEM | RTF_POOLED);
    new_luxurytype(itype, 0);

    t_plain = test_create_terrain("plain", LAND_REGION);
    t_ocean = test_create_terrain("ocean", SEA_REGION);
    r = test_create_region(0, 0, t_ocean);
    CuAssertPtrEquals(tc, NULL, r->land);
    terraform_region(r, t_plain);
    CuAssertPtrNotNull(tc, r->land);
    CuAssertTrue(tc, r_has_demands(r));
    CuAssertPtrEquals(tc, itype, (item_type *)r_luxury(r));
    CuAssertIntEquals(tc, 0, r_demand(r, itype->rtype->ltype));
    rsetroad(r, D_NORTHWEST, 10);
    terraform_region(r, t_ocean);
    CuAssertIntEquals(tc, 0, rroad(r, D_NORTHWEST));
    CuAssertPtrEquals(tc, NULL, r->land);
    test_teardown();
}

static void test_region_get_owner(CuTest *tc) {
    region *r;
    building *b1, *b2;
    unit *u1, *u2;

    test_setup();
    r = test_create_plain(0, 0);
    b1 = test_create_building(r, test_create_castle());
    b2 = test_create_building(r, test_create_castle());
    b1->size = 5;
    b2->size = 10;
    u1 = test_create_unit(test_create_faction(), r);
    u_set_building(u1, b1);
    u2 = test_create_unit(test_create_faction(), r);
    u_set_building(u2, b2);
    CuAssertPtrEquals(tc, u2->faction, region_get_owner(r));
    test_teardown();
}

static void test_region_getset_resource(CuTest *tc) {
    region *r;
    item_type *itype;

    test_setup();
    init_resources();
    itype = test_create_itemtype("iron");
    itype->construction = calloc(1, sizeof(construction));
    rmt_create(itype->rtype);
    r = test_create_plain(0, 0);

    region_setresource(r, itype->rtype, 50);
    CuAssertIntEquals(tc, 50, region_getresource(r, itype->rtype));

    region_setresource(r, get_resourcetype(R_HORSE), 10);
    CuAssertIntEquals(tc, 10, region_getresource(r, get_resourcetype(R_HORSE)));
    CuAssertIntEquals(tc, 10, rhorses(r));

    region_setresource(r, get_resourcetype(R_PEASANT), 10);
    CuAssertIntEquals(tc, 10, region_getresource(r, get_resourcetype(R_PEASANT)));
    CuAssertIntEquals(tc, 10, rpeasants(r));

    test_teardown();
}

static void test_roads(CuTest *tc) {
    region *r, *r2;

    test_setup();
    r = test_create_plain(0, 0);
    r2 = test_create_plain(1, 0);
    rsetroad(r, D_EAST, 100);
    rsetroad(r2, D_WEST, 50);
    CuAssertIntEquals(tc, 100, rroad(r, D_EAST));
    CuAssertIntEquals(tc, 50, rroad(r2, D_WEST));
    test_teardown();
}

static void test_borders(CuTest *tc) {
    region *r, *r2;
    connection *c;

    test_setup();
    r = test_create_plain(0, 0);
    r2 = test_create_plain(1, 0);
    CuAssertPtrEquals(tc, NULL, get_borders(r, r2));
    c = create_border(&bt_road, r, r2);
    CuAssertPtrEquals(tc, c, get_borders(r, r2));
    CuAssertPtrEquals(tc, c, get_borders(r2, r));
    test_teardown();
}

static void test_trees(CuTest *tc) {
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    rsettrees(r, 0, 1000);
    rsettrees(r, 1, 2000);
    rsettrees(r, 2, 3000);
    CuAssertIntEquals(tc, 1000, rtrees(r, 0));
    CuAssertIntEquals(tc, 2000, rtrees(r, 1));
    CuAssertIntEquals(tc, 3000, rtrees(r, 2));
    rsettrees(r, 0, MAXTREES);
    CuAssertIntEquals(tc, MAXTREES, rtrees(r, 0));
    rsettrees(r, 0, MAXTREES+100);
    CuAssertIntEquals(tc, MAXTREES, rtrees(r, 0));
    test_teardown();
}

static void test_mourning(CuTest *tc) {
    struct region * r;
    struct faction * f1, * f2;

    test_setup();
    config_set_int("rules.region_owners", 1);
    r = test_create_plain(0, 0);
    f1 = test_create_faction();
    f2 = test_create_faction();
    CuAssertTrue(tc, !is_mourning(r, turn));
    region_set_owner(r, f1, turn);
    CuAssertTrue(tc, !is_mourning(r, turn));
    CuAssertTrue(tc, !is_mourning(r, turn + 1));
    region_set_owner(r, f2, turn);
    CuAssertTrue(tc, !is_mourning(r, turn));
    CuAssertTrue(tc, is_mourning(r, turn + 1));
    CuAssertTrue(tc, !is_mourning(r, turn + 2));
    test_teardown();
}

CuSuite *get_region_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_newterrain);
    SUITE_ADD_TEST(suite, test_terraform);
    SUITE_ADD_TEST(suite, test_roads);
    SUITE_ADD_TEST(suite, test_borders);
    SUITE_ADD_TEST(suite, test_trees);
    SUITE_ADD_TEST(suite, test_mourning);
    SUITE_ADD_TEST(suite, test_region_getset_resource);
    SUITE_ADD_TEST(suite, test_region_get_owner);
    return suite;
}
