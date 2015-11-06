#include <platform.h>
#include <kernel/config.h>
#include "alchemy.h"
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
    construction cons;
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

    bf->cons.materials = calloc(2, sizeof(requirement));
    bf->cons.materials[0].number = 1;
    bf->cons.materials[0].rtype = get_resourcetype(R_SILVER);
    bf->cons.skill = SK_ARMORER;
    bf->cons.minskill = 2;
    bf->cons.maxsize = -1;
    bf->cons.reqsize = 1;
    return bf->u;
}

static void teardown_build(build_fixture *bf) {
    free(bf->cons.materials);
    test_cleanup();
}

static void test_build_requires_materials(CuTest *tc) {
    build_fixture bf = { 0 };
    unit *u;
    const struct item_type *itype;

    u = setup_build(&bf);
    set_level(u, SK_ARMORER, 2);
    CuAssertIntEquals(tc, ENOMATERIALS, build(u, &bf.cons, 0, 1));
    itype = bf.cons.materials[0].rtype->itype;
    i_change(&u->items, itype, 2);
    CuAssertIntEquals(tc, 1, build(u, &bf.cons, 0, 1));
    CuAssertIntEquals(tc, 1, i_get(u->items, itype));
    teardown_build(&bf);
}

static void test_build_requires_building(CuTest *tc) {
    build_fixture bf = { 0 };
    unit *u;
    const struct resource_type *rtype;
    building_type *btype;

    u = setup_build(&bf);
    rtype = bf.cons.materials[0].rtype;
    i_change(&u->items, rtype->itype, 1);
    set_level(u, SK_ARMORER, 2);
    bf.cons.btype = btype = bt_get_or_create("hodor");
    btype->maxcapacity = 1;
    btype->capacity = 1;
    CuAssertIntEquals_Msg(tc, "must be inside a production building", EBUILDINGREQ, build(u, &bf.cons, 0, 1));
    u->building = test_create_building(u->region, btype);
    fset(u->building, BLD_WORKING);
    CuAssertIntEquals(tc, 1, build(u, &bf.cons, 0, 1));
    btype->maxcapacity = 0;
    CuAssertIntEquals_Msg(tc, "cannot build when production building capacity exceeded", EBUILDINGREQ, build(u, &bf.cons, 0, 1));
    teardown_build(&bf);
}

static void test_build_failure_missing_skill(CuTest *tc) {
    build_fixture bf = { 0 };
    unit *u;
    const struct resource_type *rtype;

    u = setup_build(&bf);
    rtype = bf.cons.materials[0].rtype;
    i_change(&u->items, rtype->itype, 1);
    CuAssertIntEquals(tc, ENEEDSKILL, build(u, &bf.cons, 1, 1));
    teardown_build(&bf);
}

static void test_build_failure_low_skill(CuTest *tc) {
    build_fixture bf = { 0 };
    unit *u;
    const struct resource_type *rtype;

    u = setup_build(&bf);
    rtype = bf.cons.materials[0].rtype;
    i_change(&u->items, rtype->itype, 1);
    set_level(u, SK_ARMORER, bf.cons.minskill - 1);
    CuAssertIntEquals(tc, ELOWSKILL, build(u, &bf.cons, 0, 10));
    teardown_build(&bf);
}

static void test_build_failure_completed(CuTest *tc) {
    build_fixture bf = { 0 };
    unit *u;
    const struct resource_type *rtype;

    u = setup_build(&bf);
    rtype = bf.cons.materials[0].rtype;
    i_change(&u->items, rtype->itype, 1);
    set_level(u, SK_ARMORER, bf.cons.minskill);
    bf.cons.maxsize = 1;
    CuAssertIntEquals(tc, ECOMPLETE, build(u, &bf.cons, bf.cons.maxsize, 10));
    CuAssertIntEquals(tc, 1, i_get(u->items, rtype->itype));
    teardown_build(&bf);
}

static void test_build_limits(CuTest *tc) {
    build_fixture bf = { 0 };
    unit *u;
    const struct resource_type *rtype;

    u = setup_build(&bf);
    rtype = bf.cons.materials[0].rtype;
    i_change(&u->items, rtype->itype, 1);
    set_level(u, SK_ARMORER, bf.cons.minskill);
    CuAssertIntEquals(tc, 1, build(u, &bf.cons, 0, 10));
    CuAssertIntEquals(tc, 0, i_get(u->items, rtype->itype));

    scale_number(u, 2);
    set_level(u, SK_ARMORER, bf.cons.minskill);
    i_change(&u->items, rtype->itype, 2);
    CuAssertIntEquals(tc, 2, build(u, &bf.cons, 0, 10));
    CuAssertIntEquals(tc, 0, i_get(u->items, rtype->itype));

    scale_number(u, 2);
    set_level(u, SK_ARMORER, bf.cons.minskill * 2);
    i_change(&u->items, rtype->itype, 4);
    CuAssertIntEquals(tc, 4, build(u, &bf.cons, 0, 10));
    CuAssertIntEquals(tc, 0, i_get(u->items, rtype->itype));
    teardown_build(&bf);
}

static void test_build_with_ring(CuTest *tc) {
    build_fixture bf = { 0 };
    unit *u;
    item_type *ring;
    const struct resource_type *rtype;

    u = setup_build(&bf);
    rtype = bf.cons.materials[0].rtype;
    ring = it_get_or_create(rt_get_or_create("roqf"));
    assert(rtype && ring);

    set_level(u, SK_ARMORER, bf.cons.minskill);
    i_change(&u->items, rtype->itype, 20);
    i_change(&u->items, ring, 1);
    CuAssertIntEquals(tc, 10, build(u, &bf.cons, 0, 20));
    CuAssertIntEquals(tc, 10, i_get(u->items, rtype->itype));
    teardown_build(&bf);
}

static void test_build_with_potion(CuTest *tc) {
    build_fixture bf = { 0 };
    unit *u;
    const potion_type *ptype;
    const struct resource_type *rtype;

    u = setup_build(&bf);
    rtype = bf.cons.materials[0].rtype;
    oldpotiontype[P_DOMORE] = ptype = new_potiontype(it_get_or_create(rt_get_or_create("hodor")), 1);
    assert(rtype && ptype);

    i_change(&u->items, rtype->itype, 20);
    change_effect(u, ptype, 4);
    set_level(u, SK_ARMORER, bf.cons.minskill);
    CuAssertIntEquals(tc, 2, build(u, &bf.cons, 0, 20));
    CuAssertIntEquals(tc, 18, i_get(u->items, rtype->itype));
    CuAssertIntEquals(tc, 3, get_effect(u, ptype));
    set_level(u, SK_ARMORER, bf.cons.minskill * 2);
    CuAssertIntEquals(tc, 4, build(u, &bf.cons, 0, 20));
    CuAssertIntEquals(tc, 2, get_effect(u, ptype));
    set_level(u, SK_ARMORER, bf.cons.minskill);
    scale_number(u, 2); // OBS: this scales the effects, too:
    CuAssertIntEquals(tc, 4, get_effect(u, ptype));
    CuAssertIntEquals(tc, 4, build(u, &bf.cons, 0, 20));
    CuAssertIntEquals(tc, 2, get_effect(u, ptype));
    teardown_build(&bf);
}

static void test_build_building_no_materials(CuTest *tc) {
    const building_type *btype;
    build_fixture bf = { 0 };
    unit *u;

    u = setup_build(&bf);
    btype = bt_find("castle");
    assert(btype);
    set_level(u, SK_BUILDING, 1);
    u->orders = create_order(K_MAKE, u->faction->locale, 0);
    CuAssertIntEquals(tc, ENOMATERIALS, build_building(u, btype, 0, 4, u->orders));
    CuAssertPtrEquals(tc, 0, u->region->buildings);
    CuAssertPtrEquals(tc, 0, u->building);
    teardown_build(&bf);
}

static void test_build_building_with_golem(CuTest *tc) {
    unit *u;
    build_fixture bf = { 0 };
    const building_type *btype;

    u = setup_build(&bf);
    bf.rc->flags |= RCF_STONEGOLEM;
    btype = bt_find("castle");
    assert(btype);
    assert(btype->construction);

    set_level(bf.u, SK_BUILDING, 1);
    u->orders = create_order(K_MAKE, u->faction->locale, 0);
    CuAssertIntEquals(tc, 1, build_building(u, btype, 0, 1, u->orders));
    CuAssertPtrNotNull(tc, u->region->buildings);
    CuAssertIntEquals(tc, 1, u->region->buildings->size);
    CuAssertIntEquals(tc, 0, u->number);
    teardown_build(&bf);
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
    u->orders = create_order(K_MAKE, u->faction->locale, 0);
    CuAssertIntEquals(tc, 1, build_building(u, btype, 0, 4, u->orders));
    CuAssertPtrNotNull(tc, u->region->buildings);
    CuAssertPtrEquals(tc, u->region->buildings, u->building);
    CuAssertIntEquals(tc, 1, u->building->size);
    CuAssertIntEquals(tc, 0, i_get(u->items, rtype->itype));
    teardown_build(&bf);
}

static void test_build_castle_skillcap(CuTest *tc) {
    const building_type *btype;
    build_fixture bf = { 0 };
    unit *u;
    const resource_type *rtype;

    u = setup_build(&bf);
    btype = bt_find("castle");
    assert(btype);

    set_number(u, 1000);
    set_level(u, SK_BUILDING, 1);
    rtype = get_resourcetype(R_STONE);
    i_change(&bf.u->items, rtype->itype, 1000);

    u->orders = create_order(K_MAKE, u->faction->locale, 0);

    CuAssertIntEquals(tc, 10, build_building(u, btype, 0, 100, u->orders));
    CuAssertIntEquals(tc, 10, u->building->size);

    set_level(u, SK_BUILDING, 3);

    CuAssertIntEquals(tc, 240, build_building(u, btype, 0, 200, u->orders));
    CuAssertIntEquals(tc, 250, u->building->size);

    teardown_build(&bf);
}

CuSuite *get_build_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_build_limits);
    SUITE_ADD_TEST(suite, test_build_failure_low_skill);
    SUITE_ADD_TEST(suite, test_build_failure_missing_skill);
    SUITE_ADD_TEST(suite, test_build_requires_materials);
    SUITE_ADD_TEST(suite, test_build_requires_building);
    SUITE_ADD_TEST(suite, test_build_failure_completed);
    SUITE_ADD_TEST(suite, test_build_with_ring);
    SUITE_ADD_TEST(suite, test_build_with_potion);
    SUITE_ADD_TEST(suite, test_build_building_success);
    SUITE_ADD_TEST(suite, test_build_building_with_golem);
    SUITE_ADD_TEST(suite, test_build_building_no_materials);
    SUITE_ADD_TEST(suite, test_build_castle_skillcap);
    return suite;
}

