#include <platform.h>
#include <kernel/config.h>
#include "types.h"
#include "build.h"
#include "order.h"
#include "unit.h"
#include "building.h"
#include "faction.h"
#include "region.h"
#include "race.h"
#include "item.h"
#include <util/language.h>
#include <CuTest.h>
#include <tests.h>

#include <assert.h>

static void test_build_building_no_materials(CuTest *tc) {
    unit *u;
    region *r;
    faction *f;
    race *rc;
    const building_type *btype;

    test_cleanup();
    test_create_world();

    rc = test_create_race("human");
    r = findregion(0, 0);
    f = test_create_faction(rc);
    assert(rc && f && r);
    u  = test_create_unit(f, r);
    btype = bt_find("castle");
    assert(u && btype);
    set_level(u, SK_BUILDING, 1);
    CuAssertIntEquals(tc, ENOMATERIALS, build_building(u, btype, 0, 4, 0));
    CuAssertPtrEquals(tc, 0, r->buildings);
    CuAssertPtrEquals(tc, 0, u->building);
}

static void test_build_building_with_golem(CuTest *tc) {
    unit *u;
    region *r;
    faction *f;
    race *rc;
    const building_type *btype;

    test_cleanup();
    test_create_world();

    rc = test_create_race("stonegolem");
    rc->flags |= RCF_STONEGOLEM;
    btype = bt_find("castle");
    assert(btype && rc);
    assert(btype->construction);
    r = findregion(0, 0);
    assert(!r->buildings);
    f = test_create_faction(rc);
    assert(r && f);
    u  = test_create_unit(f, r);
    assert(u);
    set_level(u, SK_BUILDING, 1);
    CuAssertIntEquals(tc, 1, build_building(u, btype, 0, 4, 0));
    CuAssertPtrNotNull(tc, r->buildings);
    CuAssertIntEquals(tc, 1, r->buildings->size);
    CuAssertIntEquals(tc, 0, u->number);
}

static void test_build_building_success(CuTest *tc) {
    unit *u;
    region *r;
    faction *f;
    race *rc;
    const building_type *btype;
    const resource_type *rtype;

    test_cleanup();
    test_create_world();

    rc = test_create_race("human");
    rtype = get_resourcetype(R_STONE);
    btype = bt_find("castle");
    assert(btype && rc && rtype && rtype->itype);
    assert(btype->construction);
    r = findregion(0, 0);
    assert(!r->buildings);
    f = test_create_faction(rc);
    assert(r && f);
    u  = test_create_unit(f, r);
    assert(u);
    i_change(&u->items, rtype->itype, 1);
    set_level(u, SK_BUILDING, 1);
    CuAssertIntEquals(tc, 1, build_building(u, btype, 0, 4, 0));
    CuAssertPtrNotNull(tc, r->buildings);
    CuAssertPtrEquals(tc, r->buildings, u->building);
    CuAssertIntEquals(tc, 1, u->building->size);
    CuAssertIntEquals(tc, 0, i_get(u->items, rtype->itype));
}

CuSuite *get_build_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_build_building_success);
    SUITE_ADD_TEST(suite, test_build_building_with_golem);
    SUITE_ADD_TEST(suite, test_build_building_no_materials);
    return suite;
}

