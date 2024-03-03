#include "build.h"

#include "alchemy.h"
#include "building.h"
#include "config.h"
#include "faction.h"
#include "item.h"
#include "order.h"
#include "race.h"
#include "region.h"
#include "ship.h"
#include "skill.h"  // for SK_BUILDING, SK_ARMORER
#include "study.h"  // for STUDYDAYS
#include "unit.h"

#include <kernel/skills.h>

#include <util/keyword.h>  // for K_MAKE

#include <CuTest.h>
#include <tests.h>

#include <assert.h>
#include <stdlib.h>
#include <limits.h>

typedef struct build_fixture {
    faction *f;
    unit *u;
    region *r;
    race *rc;
    construction cons;
    building_type *btype;
    ship_type *stype;
} build_fixture;

static unit * setup_build(build_fixture *bf) {
    test_setup();
    config_set_int("study.produceexp", STUDYDAYS);
    init_resources();

    test_create_itemtype("stone");
    test_create_itemtype("log");
    bf->btype = test_create_castle();
    bf->stype = test_create_shiptype("boat");
    bf->rc = test_create_race("human");
    bf->r = test_create_plain(0, 0);
    bf->f = test_create_faction_ex(bf->rc, NULL);
    assert(bf->rc && bf->f && bf->r);
    bf->u = test_create_unit(bf->f, bf->r);
    assert(bf->u);

    bf->r = test_create_plain(0, 0);
    bf->f = test_create_faction_ex(bf->rc, NULL);
    assert(bf->rc && bf->f && bf->r);
    bf->u = test_create_unit(bf->f, bf->r);
    assert(bf->u);

    bf->cons.materials = calloc(2, sizeof(requirement));
    assert(bf->cons.materials);
    bf->cons.materials[0].number = 1;
    bf->cons.materials[0].rtype = get_resourcetype(R_SILVER);
    bf->cons.skill = SK_ARMORER;
    bf->cons.minskill = 2;
    bf->cons.maxsize = -1;
    bf->cons.reqsize = 1;
    return bf->u;
}

static void test_build_building_stages(CuTest *tc) {
    building_type *btype;
    item_type *it_stone;
    unit *u;

    test_setup();
    init_resources();
    it_stone = test_create_itemtype("stone");
    btype = test_create_castle();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u->building = test_create_building(u->region, btype);
    u->building->size = 1;
    set_level(u, SK_BUILDING, 2);
    i_change(&u->items, it_stone, 4);
    build_building(u, btype, -1, INT_MAX, NULL);
    CuAssertPtrNotNull(tc, u->building);
    CuAssertIntEquals(tc, 3, u->building->size);
    CuAssertIntEquals(tc, 2, i_get(u->items, it_stone));

    test_teardown();
}

static void test_build_building_stage_continue(CuTest *tc) {
    building_type *btype;
    item_type *it_stone;
    unit *u;

    test_setup();
    init_resources();
    it_stone = test_create_itemtype("stone");
    btype = test_create_castle();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    set_level(u, SK_BUILDING, 2);
    i_change(&u->items, it_stone, 4);
    build_building(u, btype, -1, INT_MAX, NULL);
    CuAssertPtrNotNull(tc, u->building);
    CuAssertIntEquals(tc, 2, u->building->size);
    CuAssertIntEquals(tc, 2, i_get(u->items, it_stone));

    test_teardown();
}

static void teardown_build(build_fixture *bf) {
    free(bf->cons.materials);
    test_teardown();
}

static void test_build_requires_materials(CuTest *tc) {
    build_fixture bf = { 0 };
    unit *u;
    const struct item_type *itype;

    u = setup_build(&bf);
    assert(bf.cons.materials);
    itype = bf.cons.materials[0].rtype->itype;
    set_level(u, SK_ARMORER, 2);
    CuAssertIntEquals(tc, ENOMATERIALS, build(u, 1, &bf.cons, 0, 1, 0));
    i_change(&u->items, itype, 2);
    CuAssertIntEquals(tc, 1, build(u, 1, &bf.cons, 0, 1, 0));
    CuAssertIntEquals(tc, 1, i_get(u->items, itype));
    teardown_build(&bf);
}

static void test_build_failure_missing_skill(CuTest *tc) {
    build_fixture bf = { 0 };
    unit *u;
    const struct resource_type *rtype;

    u = setup_build(&bf);
    assert(bf.cons.materials);
    rtype = bf.cons.materials[0].rtype;
    i_change(&u->items, rtype->itype, 1);
    CuAssertIntEquals(tc, ENEEDSKILL, build(u, 1, &bf.cons, 1, 1, 0));
    teardown_build(&bf);
}

static void test_build_failure_low_skill(CuTest *tc) {
    build_fixture bf = { 0 };
    unit *u;
    const struct resource_type *rtype;

    u = setup_build(&bf);
    assert(bf.cons.materials);
    rtype = bf.cons.materials[0].rtype;
    i_change(&u->items, rtype->itype, 1);
    set_level(u, SK_ARMORER, bf.cons.minskill - 1);
    CuAssertIntEquals(tc, ELOWSKILL, build(u, 1, &bf.cons, 0, 10, 0));
    teardown_build(&bf);
}

static void test_build_failure_completed(CuTest *tc) {
    build_fixture bf = { 0 };
    unit *u;
    const struct resource_type *rtype;

    u = setup_build(&bf);
    assert(bf.cons.materials);
    rtype = bf.cons.materials[0].rtype;
    i_change(&u->items, rtype->itype, 1);
    set_level(u, SK_ARMORER, bf.cons.minskill);
    bf.cons.maxsize = 1;
    CuAssertIntEquals(tc, ECOMPLETE, build(u, 1, &bf.cons, bf.cons.maxsize, 10, 0));
    CuAssertIntEquals(tc, 1, i_get(u->items, rtype->itype));
    teardown_build(&bf);
}

static void test_build_skill(CuTest* tc) {
    unit* u;
    const item_type* it_ring;

    test_setup();
    u = test_create_unit(NULL, NULL);
    it_ring = it_get_or_create(rt_get_or_create("roqf"));
    oldpotiontype[P_DOMORE] = it_get_or_create(rt_get_or_create("p3"));
    scale_number(u, 3);
    CuAssertIntEquals(tc, 3, build_skill(u, 1, 0, SK_BUILDING));
    CuAssertIntEquals(tc, 6, build_skill(u, 1, 1, SK_BUILDING));

    /* Ringe werden nicht verbraucht: */
    i_change(&u->items, it_ring, 3);
    CuAssertIntEquals(tc, 30, build_skill(u, 1, 0, SK_BUILDING));
    CuAssertIntEquals(tc, 3, i_get(u->items, it_ring));
    i_change(&u->items, it_ring, -1);
    CuAssertIntEquals(tc, 21, build_skill(u, 1, 0, SK_BUILDING));
    CuAssertIntEquals(tc, 2, i_get(u->items, it_ring));
    i_change(&u->items, it_ring, -2);

    /* Schaffenstrunk-Effekt wird verbraucht: */
    change_effect(u, oldpotiontype[P_DOMORE], 5);
    CuAssertIntEquals(tc, 6, build_skill(u, 1, 0, SK_BUILDING));
    CuAssertIntEquals(tc, 2, get_effect(u, oldpotiontype[P_DOMORE]));

    CuAssertIntEquals(tc, 10, build_skill(u, 1, 1, SK_BUILDING));
    CuAssertIntEquals(tc, 0, get_effect(u, oldpotiontype[P_DOMORE]));

    /* ring and potion combined is factor 11: */
    i_change(&u->items, it_ring, 3);
    change_effect(u, oldpotiontype[P_DOMORE], 6);
    CuAssertIntEquals(tc, 33, build_skill(u, 1, 0, SK_BUILDING));
    CuAssertIntEquals(tc, 66, build_skill(u, 1, 1, SK_BUILDING));
    CuAssertIntEquals(tc, 0, get_effect(u, oldpotiontype[P_DOMORE]));

    test_teardown();
}

static void test_build_limits(CuTest *tc) {
    build_fixture bf = { 0 };
    unit *u;
    const struct resource_type *rtype;

    u = setup_build(&bf);
    assert(bf.cons.materials);
    rtype = bf.cons.materials[0].rtype;
    assert(rtype);
    i_change(&u->items, rtype->itype, 1);
    set_level(u, SK_ARMORER, bf.cons.minskill);
    CuAssertIntEquals(tc, 1, build(u, 1, &bf.cons, 0, 10, 0));
    CuAssertIntEquals(tc, 0, i_get(u->items, rtype->itype));

    scale_number(u, 2);
    set_level(u, SK_ARMORER, bf.cons.minskill);
    i_change(&u->items, rtype->itype, 2);
    CuAssertIntEquals(tc, 2, build(u, 1, &bf.cons, 0, 10, 0));
    CuAssertIntEquals(tc, 0, i_get(u->items, rtype->itype));

    scale_number(u, 2);
    set_level(u, SK_ARMORER, bf.cons.minskill * 2);
    i_change(&u->items, rtype->itype, 4);
    CuAssertIntEquals(tc, 4, build(u, 1, &bf.cons, 0, 10, 0));
    CuAssertIntEquals(tc, 0, i_get(u->items, rtype->itype));
    teardown_build(&bf);
}

static void test_build_with_ring(CuTest *tc) {
    build_fixture bf = { 0 };
    unit *u;
    item_type *ring;
    const struct resource_type *rtype;

    u = setup_build(&bf);
    assert(bf.cons.materials);
    rtype = bf.cons.materials[0].rtype;
    ring = it_get_or_create(rt_get_or_create("roqf"));
    assert(rtype && ring);

    set_level(u, SK_ARMORER, bf.cons.minskill);
    i_change(&u->items, rtype->itype, 20);
    i_change(&u->items, ring, 1);
    CuAssertIntEquals(tc, 10, build(u, 1, &bf.cons, 0, 20, 0));
    CuAssertIntEquals(tc, 10, i_get(u->items, rtype->itype));
    teardown_build(&bf);
}

static void test_build_with_potion(CuTest *tc)
{
    build_fixture bf = { 0 };
    unit *u;
    const item_type *ptype;
    const struct resource_type *rtype;

    u = setup_build(&bf);
    assert(bf.cons.materials);
    rtype = bf.cons.materials[0].rtype;
    oldpotiontype[P_DOMORE] = ptype = it_get_or_create(rt_get_or_create("p3"));
    assert(rtype && ptype);

    i_change(&u->items, rtype->itype, 20);
    change_effect(u, ptype, 4);
    set_level(u, SK_ARMORER, bf.cons.minskill);
    CuAssertIntEquals(tc, 2, build(u, 1, &bf.cons, 0, 20, 0));
    CuAssertIntEquals(tc, 18, i_get(u->items, rtype->itype));
    CuAssertIntEquals(tc, 3, get_effect(u, ptype));
    set_level(u, SK_ARMORER, bf.cons.minskill * 2);
    CuAssertIntEquals(tc, 4, build(u, 1, &bf.cons, 0, 20, 0));
    CuAssertIntEquals(tc, 2, get_effect(u, ptype));
    set_level(u, SK_ARMORER, bf.cons.minskill);
    scale_number(u, 2); /* OBS: this scales the effects, too: */
    CuAssertIntEquals(tc, 4, get_effect(u, ptype));
    CuAssertIntEquals(tc, 4, build(u, 1, &bf.cons, 0, 20, 0));
    CuAssertIntEquals(tc, 2, get_effect(u, ptype));
    teardown_build(&bf);
}

static void test_build_with_potion_and_ring(CuTest *tc)
{
    build_fixture bf = { 0 };
    unit *u;
    const item_type *ptype, *ring;
    const struct resource_type *rtype;

    u = setup_build(&bf);
    ring = it_get_or_create(rt_get_or_create("roqf"));
    assert(bf.cons.materials);
    rtype = bf.cons.materials[0].rtype;
    oldpotiontype[P_DOMORE] = ptype = it_get_or_create(rt_get_or_create("hodor"));
    assert(rtype && ptype);

    i_change(&u->items, rtype->itype, 200);
    set_level(u, bf.cons.skill, bf.cons.minskill);
    CuAssertIntEquals(tc, 1, build(u, 1, &bf.cons, 0, 200, 0));

    i_change(&u->items, ring, 1);
    change_effect(u, ptype, 4);
    CuAssertIntEquals(tc, 11, build(u, 1, &bf.cons, 0, 200, 0));
    CuAssertIntEquals(tc, 3, get_effect(u, ptype));

    teardown_build(&bf);
}

static void test_build_building_no_materials(CuTest *tc) {
    const building_type *btype;
    build_fixture bf = { 0 };
    unit *u;

    u = setup_build(&bf);
    btype = bf.btype;
    set_level(u, SK_BUILDING, 1);
    u->orders = create_order(K_MAKE, u->faction->locale, NULL);
    CuAssertIntEquals(tc, ENOMATERIALS, build_building(u, btype, 0, 4, u->orders));
    CuAssertPtrEquals(tc, NULL, u->region->buildings);
    CuAssertPtrEquals(tc, NULL, u->building);
    teardown_build(&bf);
}

static void test_build_building_with_golem(CuTest *tc) {
    unit *u;
    build_fixture bf = { 0 };
    const building_type *btype;

    u = setup_build(&bf);
    bf.rc->ec_flags |= ECF_STONEGOLEM;
    btype = bf.btype;

    set_level(bf.u, SK_BUILDING, 1);
    u->orders = create_order(K_MAKE, u->faction->locale, NULL);
    CuAssertIntEquals(tc, 1, build_building(u, btype, 0, 1, u->orders));
    CuAssertPtrNotNull(tc, u->region->buildings);
    CuAssertIntEquals(tc, 1, u->region->buildings->size);
    CuAssertIntEquals(tc, 0, u->number);
    teardown_build(&bf);
}

static void test_build_building_success(CuTest *tc)
{
    unit *u;
    skill *sv;
    build_fixture bf = { 0 };
    const building_type *btype;
    const resource_type *rtype;

    u = setup_build(&bf);

    rtype = get_resourcetype(R_STONE);
    btype = bf.btype;
    assert(btype && rtype && rtype->itype);
    assert(!u->region->buildings);

    i_change(&u->items, rtype->itype, 1);
    sv = test_set_skill(u, SK_BUILDING, 1, 2);
    u->orders = create_order(K_MAKE, u->faction->locale, NULL);
    CuAssertIntEquals(tc, 1, build_building(u, btype, 0, 4, u->orders));
    CuAssertPtrNotNull(tc, u->region->buildings);
    CuAssertPtrEquals(tc, u->region->buildings, u->building);
    CuAssertIntEquals(tc, 1, u->building->size);
    CuAssertIntEquals(tc, 0, i_get(u->items, rtype->itype));
    CuAssertIntEquals(tc, 1, sv->weeks);
    teardown_build(&bf);
}

static void test_build_building_produceexp(CuTest *tc)
{
    unit *u;
    build_fixture bf = { 0 };
    const building_type *btype;
    const resource_type *rtype;
    skill *sv;

    u = setup_build(&bf);

    rtype = get_resourcetype(R_STONE);
    btype = bf.btype;
    assert(btype && rtype && rtype->itype);
    assert(!u->region->buildings);

    i_change(&u->items, rtype->itype, 1);
    sv = test_set_skill(u, SK_BUILDING, 1, 2);
    u->orders = create_order(K_MAKE, u->faction->locale, NULL);
    CuAssertIntEquals(tc, 1, build_building(u, btype, 0, 4, u->orders));
    CuAssertPtrNotNull(tc, u->region->buildings);
    CuAssertPtrEquals(tc, u->region->buildings, u->building);
    CuAssertIntEquals(tc, 1, u->building->size);
    CuAssertIntEquals(tc, 0, i_get(u->items, rtype->itype));
    CuAssertIntEquals(tc, 1, sv->weeks);
    teardown_build(&bf);
}


static void test_build_roqf_factor(CuTest *tc) {
    test_setup();
    CuAssertIntEquals(tc, 10, roqf_factor());
    config_set("rules.economy.roqf", "50");
    CuAssertIntEquals(tc, 50, roqf_factor());
    test_teardown();
}

static void test_build_building_nobuild_fail(CuTest *tc) {
    unit *u;
    build_fixture bf = { 0 };
    building_type *btype;
    const resource_type *rtype;

    u = setup_build(&bf);

    rtype = get_resourcetype(R_STONE);
    btype = bf.btype;
    assert(btype && rtype && rtype->itype);
    assert(!u->region->buildings);
    btype->flags |= BTF_NOBUILD;

    i_change(&u->items, rtype->itype, 1);
    set_level(u, SK_BUILDING, 1);
    u->orders = create_order(K_MAKE, u->faction->locale, NULL);
    CuAssertIntEquals(tc, 0, build_building(u, btype, 0, 4, u->orders));
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "error221"));
    CuAssertPtrEquals(tc, NULL, u->region->buildings);
    CuAssertPtrEquals(tc, NULL, u->building);
    CuAssertIntEquals(tc, 1, i_get(u->items, rtype->itype));
    teardown_build(&bf);
}

static void test_build_building_unique(CuTest *tc) {
    unit *u;
    build_fixture bf = { 0 };
    building_type *btype;
    const resource_type *rtype;

    u = setup_build(&bf);

    rtype = get_resourcetype(R_STONE);
    btype = bf.btype;
    assert(btype && rtype && rtype->itype);
    i_change(&u->items, rtype->itype, 2);
    set_level(u, SK_BUILDING, 1);
    u->orders = create_order(K_MAKE, u->faction->locale, NULL);

    btype->flags |= BTF_UNIQUE;
    CuAssertIntEquals(tc, 1, build_building(u, btype, 0, 4, u->orders));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "error93"));
    CuAssertIntEquals(tc, 1, i_get(u->items, rtype->itype));
    CuAssertPtrNotNull(tc, u->building);
    leave_building(u);
    CuAssertIntEquals(tc, 0, build_building(u, btype, 0, 4, u->orders));
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "error93"));
    CuAssertIntEquals(tc, 1, i_get(u->items, rtype->itype));
    test_clear_messages(u->faction);

    /* NOBUILD before UNIQUE*/
    btype->flags |= BTF_UNIQUE|BTF_NOBUILD;
    CuAssertIntEquals(tc, 0, build_building(u, btype, 0, 4, u->orders));
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "error221"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "error93"));

    teardown_build(&bf);
}

static void test_build_ship_success(CuTest *tc)
{
    unit *u;
    skill *sv;
    build_fixture bf = { 0 };
    const ship_type *stype;
    const item_type *itype;

    u = setup_build(&bf);

    itype = it_find("log");
    stype = bf.stype;
    assert(stype && itype);
    assert(!u->region->ships);

    i_change(&u->items, itype, 1);
    sv = test_set_skill(u, SK_SHIPBUILDING, 1, 2);
    u->orders = create_order(K_MAKE, u->faction->locale, NULL);
    create_ship(u, stype, 4, u->orders);
    CuAssertPtrNotNull(tc, u->region->ships);
    CuAssertPtrEquals(tc, u->region->ships, u->ship);
    CuAssertIntEquals(tc, 1, u->ship->size);
    CuAssertIntEquals(tc, 0, i_get(u->items, itype));
    CuAssertIntEquals(tc, 1, sv->weeks);
    teardown_build(&bf);
}

CuSuite *get_build_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_build_skill);
    SUITE_ADD_TEST(suite, test_build_limits);
    SUITE_ADD_TEST(suite, test_build_roqf_factor);
    SUITE_ADD_TEST(suite, test_build_failure_low_skill);
    SUITE_ADD_TEST(suite, test_build_failure_missing_skill);
    SUITE_ADD_TEST(suite, test_build_requires_materials);
    SUITE_ADD_TEST(suite, test_build_failure_completed);
    SUITE_ADD_TEST(suite, test_build_with_ring);
    SUITE_ADD_TEST(suite, test_build_with_potion);
    SUITE_ADD_TEST(suite, test_build_with_potion_and_ring);
    SUITE_ADD_TEST(suite, test_build_building_success);
    SUITE_ADD_TEST(suite, test_build_building_stages);
    SUITE_ADD_TEST(suite, test_build_building_stage_continue);
    SUITE_ADD_TEST(suite, test_build_building_with_golem);
    SUITE_ADD_TEST(suite, test_build_building_no_materials);
    SUITE_ADD_TEST(suite, test_build_building_nobuild_fail);
    SUITE_ADD_TEST(suite, test_build_building_unique);
    SUITE_ADD_TEST(suite, test_build_building_produceexp);
    SUITE_ADD_TEST(suite, test_build_ship_success);
    return suite;
}

