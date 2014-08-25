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

#include <stdlib.h>
#include <assert.h>

typedef struct build_fixture {
    faction *f;
    unit *u;
    region *r;
    race *rc;
} build_fixture;

static unit * setup_build(build_fixture *bf) {
    test_cleanup();
    test_create_world();
    bf->rc = test_create_race("human");
    bf->r = findregion(0, 0);
    bf->f = test_create_faction(bf->rc);
    assert(bf->rc && bf->f && bf->r);
    bf->u = test_create_unit(bf->f, bf->r);
    assert(bf->u);
    return bf->u;
}

static void test_build(CuTest *tc) {
    build_fixture bf;
    unit *u;
    construction cons = { 0 };
    const struct resource_type *rtype;

    u = setup_build(&bf);
    rtype = get_resourcetype(R_SILVER);
    assert(rtype);

    cons.materials = calloc(2, sizeof(requirement));
    cons.materials[0].number = 1;
    cons.materials[0].rtype = rtype;
    cons.skill = SK_ARMORER;
    cons.minskill = 2;
    cons.reqsize = 1;
    CuAssertIntEquals(tc, ENEEDSKILL, build(u, &cons, 1, 1));
    set_level(u, SK_ARMORER, 1);
    CuAssertIntEquals(tc, ELOWSKILL, build(u, &cons, 1, 1));
    set_level(u, SK_ARMORER, 2);
    CuAssertIntEquals(tc, ENOMATERIALS, build(u, &cons, 1, 1));
    i_change(&u->items, rtype->itype, 1);
    CuAssertIntEquals(tc, 1, build(u, &cons, 1, 1));
    CuAssertIntEquals(tc, 0, i_get(u->items, rtype->itype));
    scale_number(u, 2);
    i_change(&u->items, rtype->itype, 2);
    CuAssertIntEquals(tc, 2, build(u, &cons, 2, 2));
    CuAssertIntEquals(tc, 0, i_get(u->items, rtype->itype));
    test_cleanup();
}

static void test_build_building_no_materials(CuTest *tc) {
    const building_type *btype;
    build_fixture bf = { 0 };
    unit *u;

    u = setup_build(&bf);
    btype = bt_find("castle");
    assert(btype);
    set_level(u, SK_BUILDING, 1);
    CuAssertIntEquals(tc, ENOMATERIALS, build_building(u, btype, 0, 4, 0));
    CuAssertPtrEquals(tc, 0, u->region->buildings);
    CuAssertPtrEquals(tc, 0, u->building);
}

static void test_build_building_with_golem(CuTest *tc) {
    unit *u;
    build_fixture bf;
    const building_type *btype;

    u = setup_build(&bf);
    bf.rc->flags |= RCF_STONEGOLEM;
    btype = bt_find("castle");
    assert(btype);
    assert(btype->construction);

    set_level(bf.u, SK_BUILDING, 1);
    CuAssertIntEquals(tc, 1, build_building(u, btype, 0, 4, 0));
    CuAssertPtrNotNull(tc, u->region->buildings);
    CuAssertIntEquals(tc, 1, u->region->buildings->size);
    CuAssertIntEquals(tc, 0, u->number);
}

static void test_build_building_success(CuTest *tc) {
    unit *u;
    build_fixture bf = { 0 };
    const building_type *btype;
    const resource_type *rtype;

    u = setup_build(&bf);

    rtype = get_resourcetype(R_STONE);
    btype = bt_find("castle");
    assert(btype && rtype && rtype->itype);
    assert(btype->construction);
    assert(!u->region->buildings);
    
    i_change(&bf.u->items, rtype->itype, 1);
    set_level(u, SK_BUILDING, 1);
    CuAssertIntEquals(tc, 1, build_building(u, btype, 0, 4, 0));
    CuAssertPtrNotNull(tc, u->region->buildings);
    CuAssertPtrEquals(tc, u->region->buildings, u->building);
    CuAssertIntEquals(tc, 1, u->building->size);
    CuAssertIntEquals(tc, 0, i_get(u->items, rtype->itype));
}

CuSuite *get_build_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_build);
    SUITE_ADD_TEST(suite, test_build_building_success);
    SUITE_ADD_TEST(suite, test_build_building_with_golem);
    SUITE_ADD_TEST(suite, test_build_building_no_materials);
    return suite;
}

