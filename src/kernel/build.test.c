#include <platform.h>
#include <kernel/config.h>

#include <kernel/messages.h>
#include "alchemy.h"
#include "types.h"
#include "build.h"
#include "guard.h"
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
    init_resources();

    test_create_itemtype("stone");
    test_create_buildingtype("castle");
    bf->rc = test_create_race("human");
    bf->r = test_create_region(0, 0, 0);
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
    fset(u->building, BLD_MAINTAINED);
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
    assert(rtype);
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

static void test_build_destroy_road(CuTest *tc)
{
    region *r, *r2;
    faction *f;
    unit *u;
    order *ord;
    message *m;

    test_cleanup();
    mt_register(mt_new_va("destroy_road", "unit:unit", "from:region", "to:region", 0));
    r2 = test_create_region(1, 0, 0);
    r = test_create_region(0, 0, 0);
    rsetroad(r, D_EAST, 100);
    u = test_create_unit(f = test_create_faction(0), r);
    u->orders = ord = create_order(K_DESTROY, f->locale, "%s %s", LOC(f->locale, parameters[P_ROAD]), LOC(f->locale, directions[D_EAST]));

    CuAssertIntEquals(tc, 0, destroy_cmd(u, ord));
    CuAssertIntEquals(tc, 100, rroad(r, D_EAST));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "destroy_road"));

    set_level(u, SK_ROAD_BUILDING, 1);
    CuAssertIntEquals(tc, 0, destroy_cmd(u, ord));
    CuAssertIntEquals(tc, 99, rroad(r, D_EAST));
    CuAssertPtrNotNull(tc, m = test_find_messagetype(f->msgs, "destroy_road"));
    CuAssertPtrEquals(tc, u, m->parameters[0].v);
    CuAssertPtrEquals(tc, r, m->parameters[1].v);
    CuAssertPtrEquals(tc, r2, m->parameters[2].v);

    set_level(u, SK_ROAD_BUILDING, 4);
    CuAssertIntEquals(tc, 0, destroy_cmd(u, ord));
    CuAssertIntEquals(tc, 95, rroad(r, D_EAST));

    scale_number(u, 4);
    set_level(u, SK_ROAD_BUILDING, 2);
    CuAssertIntEquals(tc, 0, destroy_cmd(u, ord));
    CuAssertIntEquals(tc, 87, rroad(r, D_EAST));
    test_cleanup();
}

unit *test_create_guard(region *r, faction *f, race *rc) {
    unit *ug;

    if (!rc) {
        rc = test_create_race("guardian");
        rc->flags |= RCF_UNARMEDGUARD;
    }
    if (!f) {
        f = test_create_faction(rc);
    }
    ug = test_create_unit(f, r);
    guard(ug, GUARD_TAX);

    return ug;
}

static void test_build_destroy_road_guard(CuTest *tc)
{
    region *r;
    faction *f;
    unit *u, *ug;
    order *ord;

    test_cleanup();
    test_create_region(1, 0, 0);
    r = test_create_region(0, 0, 0);
    rsetroad(r, D_EAST, 100);
    ug = test_create_guard(r, 0, 0);
    u = test_create_unit(f = test_create_faction(0), r);
    u->orders = ord = create_order(K_DESTROY, f->locale, "%s %s", LOC(f->locale, parameters[P_ROAD]), LOC(f->locale, directions[D_EAST]));

    set_level(u, SK_ROAD_BUILDING, 1);
    CuAssertIntEquals(tc, 0, destroy_cmd(u, ord));
    CuAssertIntEquals(tc, 100, rroad(r, D_EAST));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error70"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "destroy_road"));

    test_clear_messages(f);
    guard(ug, GUARD_NONE);

    CuAssertIntEquals(tc, 0, destroy_cmd(u, ord));
    CuAssertIntEquals(tc, 99, rroad(r, D_EAST));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "error70"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "destroy_road"));

    test_cleanup();
}

static void test_build_destroy_road_limit(CuTest *tc)
{
    region *r;
    faction *f;
    unit *u;
    order *ord;

    test_cleanup();
    test_create_region(1, 0, 0);
    r = test_create_region(0, 0, 0);
    rsetroad(r, D_EAST, 100);
    u = test_create_unit(f = test_create_faction(0), r);
    u->orders = ord = create_order(K_DESTROY, f->locale, "1 %s %s", LOC(f->locale, parameters[P_ROAD]), LOC(f->locale, directions[D_EAST]));

    set_level(u, SK_ROAD_BUILDING, 1);
    CuAssertIntEquals(tc, 0, destroy_cmd(u, ord));
    CuAssertIntEquals(tc, 99, rroad(r, D_EAST));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "destroy_road"));

    set_level(u, SK_ROAD_BUILDING, 4);
    CuAssertIntEquals(tc, 0, destroy_cmd(u, ord));
    CuAssertIntEquals(tc, 98, rroad(r, D_EAST));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "destroy_road"));

    test_cleanup();
}

static void test_build_destroy_cmd(CuTest *tc) {
    unit *u;
    faction *f;

    test_setup();
    u = test_create_unit(f = test_create_faction(0), test_create_region(0, 0, 0));
    u->thisorder = create_order(K_DESTROY, f->locale, NULL);
    CuAssertIntEquals(tc, 138, destroy_cmd(u, u->thisorder));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error138"));
    test_cleanup();
}

static void test_build_roqf_factor(CuTest *tc) {
    test_setup();
    CuAssertIntEquals(tc, 10, roqf_factor());
    config_set("rules.economy.roqf", "50");
    CuAssertIntEquals(tc, 50, roqf_factor());
    test_cleanup();
}

CuSuite *get_build_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_build_limits);
    SUITE_ADD_TEST(suite, test_build_roqf_factor);
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
    SUITE_ADD_TEST(suite, test_build_destroy_cmd);
    SUITE_ADD_TEST(suite, test_build_destroy_road);
    SUITE_ADD_TEST(suite, test_build_destroy_road_limit);
    SUITE_ADD_TEST(suite, test_build_destroy_road_guard);
    return suite;
}

