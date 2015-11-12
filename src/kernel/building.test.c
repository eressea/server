#include <platform.h>

#include <kernel/config.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/building.h>
#include <kernel/unit.h>

#include <CuTest.h>
#include <tests.h>
#include <stdlib.h>
#include <string.h>

static void test_register_building(CuTest * tc)
{
    building_type *btype;

    test_cleanup();

    btype = (building_type *)calloc(sizeof(building_type), 1);
    btype->_name = _strdup("herp");
    bt_register(btype);

    CuAssertPtrNotNull(tc, bt_find("herp"));
    //    free(btype->_name);
    //    free(btype);
    test_cleanup();
}

static void test_building_set_owner(CuTest * tc)
{
    struct region *r;
    struct building *bld;
    struct unit *u1, *u2;
    struct faction *f;
    const struct building_type *btype;
    const struct race *human;

    test_cleanup();
    test_create_world();

    human = rc_find("human");
    btype = bt_find("castle");
    f = test_create_faction(human);
    r = findregion(0, 0);

    bld = test_create_building(r, btype);
    u1 = test_create_unit(f, r);
    u_set_building(u1, bld);
    CuAssertPtrEquals(tc, u1, building_owner(bld));

    u2 = test_create_unit(f, r);
    u_set_building(u2, bld);
    CuAssertPtrEquals(tc, u1, building_owner(bld));
    building_set_owner(u2);
    CuAssertPtrEquals(tc, u2, building_owner(bld));
    test_cleanup();
}

static void test_buildingowner_goes_to_next_when_empty(CuTest * tc)
{
    struct region *r;
    struct building *bld;
    struct unit *u, *u2;
    struct faction *f;
    const struct building_type *btype;
    const struct race *human;

    test_cleanup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    btype = bt_find("castle");
    CuAssertPtrNotNull(tc, btype);

    f = test_create_faction(human);
    r = findregion(0, 0);

    bld = test_create_building(r, btype);
    CuAssertPtrNotNull(tc, bld);

    u = test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);
    u_set_building(u, bld);
    u_set_building(u2, bld);
    CuAssertPtrEquals(tc, u, building_owner(bld));
    u->number = 0;
    CuAssertPtrEquals(tc, u2, building_owner(bld));
    test_cleanup();
}

static void test_buildingowner_goes_to_other_when_empty(CuTest * tc)
{
    struct region *r;
    struct building *bld;
    struct unit *u, *u2;
    struct faction *f;
    const struct building_type *btype;
    const struct race *human;

    test_cleanup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    btype = bt_find("castle");
    CuAssertPtrNotNull(tc, btype);

    f = test_create_faction(human);
    r = findregion(0, 0);

    bld = test_create_building(r, btype);
    CuAssertPtrNotNull(tc, bld);

    u2 = test_create_unit(f, r);
    u = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);
    u_set_building(u, bld);
    CuAssertPtrEquals(tc, u, building_owner(bld));
    u_set_building(u2, bld);
    CuAssertPtrEquals(tc, u, building_owner(bld));
    u->number = 0;
    CuAssertPtrEquals(tc, u2, building_owner(bld));
    test_cleanup();
}

static void test_buildingowner_goes_to_same_faction_when_empty(CuTest * tc)
{
    struct region *r;
    struct building *bld;
    struct unit *u, *u2, *u3;
    struct faction *f1, *f2;
    const struct building_type *btype;
    const struct race *human;

    test_cleanup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    btype = bt_find("castle");
    CuAssertPtrNotNull(tc, btype);

    f1 = test_create_faction(human);
    f2 = test_create_faction(human);
    r = findregion(0, 0);

    bld = test_create_building(r, btype);
    CuAssertPtrNotNull(tc, bld);

    u2 = test_create_unit(f2, r);
    u3 = test_create_unit(f1, r);
    u = test_create_unit(f1, r);
    CuAssertPtrNotNull(tc, u);
    u_set_building(u, bld);
    u_set_building(u2, bld);
    u_set_building(u3, bld);
    CuAssertPtrEquals(tc, u, building_owner(bld));
    u->number = 0;
    CuAssertPtrEquals(tc, u3, building_owner(bld));
    u3->number = 0;
    CuAssertPtrEquals(tc, u2, building_owner(bld));
    test_cleanup();
}

static void test_buildingowner_goes_to_next_after_leave(CuTest * tc)
{
    struct region *r;
    struct building *bld;
    struct unit *u, *u2;
    struct faction *f;
    const struct building_type *btype;
    const struct race *human;

    test_cleanup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    btype = bt_find("castle");
    CuAssertPtrNotNull(tc, btype);

    f = test_create_faction(human);
    r = findregion(0, 0);

    bld = test_create_building(r, btype);
    CuAssertPtrNotNull(tc, bld);

    u = test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);
    u_set_building(u, bld);
    u_set_building(u2, bld);
    CuAssertPtrEquals(tc, u, building_owner(bld));
    leave_building(u);
    CuAssertPtrEquals(tc, u2, building_owner(bld));
    test_cleanup();
}

static void test_buildingowner_goes_to_other_after_leave(CuTest * tc)
{
    struct region *r;
    struct building *bld;
    struct unit *u, *u2;
    struct faction *f;
    const struct building_type *btype;
    const struct race *human;

    test_cleanup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    btype = bt_find("castle");
    CuAssertPtrNotNull(tc, btype);

    f = test_create_faction(human);
    r = findregion(0, 0);

    bld = test_create_building(r, btype);
    CuAssertPtrNotNull(tc, bld);

    u2 = test_create_unit(f, r);
    u = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);
    u_set_building(u, bld);
    u_set_building(u2, bld);
    CuAssertPtrEquals(tc, u, building_owner(bld));
    leave_building(u);
    CuAssertPtrEquals(tc, u2, building_owner(bld));
    test_cleanup();
}

static void test_buildingowner_goes_to_same_faction_after_leave(CuTest * tc)
{
    struct region *r;
    struct building *bld;
    struct unit *u, *u2, *u3;
    struct faction *f1, *f2;
    const struct building_type *btype;
    const struct race *human;

    test_cleanup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    btype = bt_find("castle");
    CuAssertPtrNotNull(tc, btype);

    f1 = test_create_faction(human);
    f2 = test_create_faction(human);
    r = findregion(0, 0);

    bld = test_create_building(r, btype);
    CuAssertPtrNotNull(tc, bld);

    u2 = test_create_unit(f2, r);
    u3 = test_create_unit(f1, r);
    u = test_create_unit(f1, r);
    CuAssertPtrNotNull(tc, u);
    u_set_building(u, bld);
    u_set_building(u2, bld);
    u_set_building(u3, bld);
    CuAssertPtrEquals(tc, u, building_owner(bld));
    leave_building(u);
    CuAssertPtrEquals(tc, u3, building_owner(bld));
    leave_building(u3);
    CuAssertPtrEquals(tc, u2, building_owner(bld));
    leave_building(u2);
    CuAssertPtrEquals(tc, 0, building_owner(bld));
    test_cleanup();
}

static void test_buildingowner_resets_when_empty(CuTest * tc)
{
    struct region *r;
    struct building *bld;
    struct unit *u;
    struct faction *f;
    const struct building_type *btype;
    const struct race *human;

    test_cleanup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    btype = bt_find("castle");
    CuAssertPtrNotNull(tc, btype);

    f = test_create_faction(human);
    r = findregion(0, 0);

    bld = test_create_building(r, btype);
    CuAssertPtrNotNull(tc, bld);

    u = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);
    u_set_building(u, bld);
    CuAssertPtrEquals(tc, u, building_owner(bld));
    u->number = 0;
    CuAssertPtrEquals(tc, 0, building_owner(bld));
    u->number = 1;
    CuAssertPtrEquals(tc, u, building_owner(bld));
    test_cleanup();
}

void test_buildingowner_goes_to_empty_unit_after_leave(CuTest * tc)
{
    struct region *r;
    struct building *bld;
    struct unit *u1, *u2, *u3;
    struct faction *f1;
    const struct building_type *btype;
    const struct race *human;

    test_cleanup();
    test_create_world();

    human = rc_find("human");
    CuAssertPtrNotNull(tc, human);

    btype = bt_find("castle");
    CuAssertPtrNotNull(tc, btype);

    f1 = test_create_faction(human);
    r = findregion(0, 0);

    bld = test_create_building(r, btype);
    CuAssertPtrNotNull(tc, bld);

    u1 = test_create_unit(f1, r);
    u2 = test_create_unit(f1, r);
    u3 = test_create_unit(f1, r);
    u_set_building(u1, bld);
    u_set_building(u2, bld);
    u_set_building(u3, bld);

    CuAssertPtrEquals(tc, u1, building_owner(bld));
    u2->number = 0;
    leave_building(u1);
    CuAssertPtrEquals(tc, u3, building_owner(bld));
    leave_building(u3);
    CuAssertPtrEquals(tc, 0, building_owner(bld));
    u2->number = 1;
    CuAssertPtrEquals(tc, u2, building_owner(bld));
    test_cleanup();
}

static void test_btype_defaults(CuTest *tc) {
    building_type * btype;
    test_cleanup();

    btype = bt_get_or_create("hodor");
    CuAssertPtrNotNull(tc, btype);
    CuAssertStrEquals(tc, "hodor", btype->_name);
    CuAssertPtrEquals(tc, 0, btype->maintenance);
    CuAssertPtrEquals(tc, 0, btype->construction);
    CuAssertTrue(tc, !btype->name);
    CuAssertTrue(tc, !btype->init);
    CuAssertTrue(tc, !btype->age);
    CuAssertTrue(tc, !btype->protection);
    CuAssertTrue(tc, !btype->taxes);
    CuAssertDblEquals(tc, 1.0, btype->auraregen, 0.0);
    CuAssertIntEquals(tc, -1, btype->maxsize);
    CuAssertIntEquals(tc, 1, btype->capacity);
    CuAssertIntEquals(tc, -1, btype->maxcapacity);
    CuAssertIntEquals(tc, 0, btype->magres);
    CuAssertIntEquals(tc, 0, btype->magresbonus);
    CuAssertIntEquals(tc, 0, btype->fumblebonus);
    CuAssertIntEquals(tc, 0, btype->flags);
    test_cleanup();
}

static void test_active_building(CuTest *tc) {
    building *b;

    test_cleanup();
    b = test_create_building(test_create_region(0, 0, 0), 0);
    CuAssertIntEquals(tc, false, building_is_active(b));
    b->flags |= BLD_WORKING;
    CuAssertIntEquals(tc, true, building_is_active(b));
    b->flags &= ~BLD_WORKING;
    CuAssertIntEquals(tc, false, building_is_active(b));
    test_cleanup();
}

static void test_safe_building(CuTest *tc) {
    building_type *btype;
    unit *u1, *u2;

    test_cleanup();
    btype = test_create_buildingtype("castle");
    u1 = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    u2 = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    CuAssertIntEquals(tc, false, in_safe_building(u1, u2));
    u1->building = test_create_building(u1->region, btype);
    CuAssertIntEquals(tc, false, in_safe_building(u1, u2));
    btype->flags |= BTF_FORTIFICATION;
    CuAssertIntEquals(tc, true, in_safe_building(u1, u2));
    u2->building = u1->building;
    CuAssertIntEquals(tc, true, in_safe_building(u1, u2));
    u1->building->size = 2;
    CuAssertIntEquals(tc, false, in_safe_building(u1, u2));
    test_cleanup();
}

CuSuite *get_building_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_register_building);
    SUITE_ADD_TEST(suite, test_btype_defaults);
    SUITE_ADD_TEST(suite, test_building_set_owner);
    SUITE_ADD_TEST(suite, test_buildingowner_resets_when_empty);
    SUITE_ADD_TEST(suite, test_buildingowner_goes_to_next_when_empty);
    SUITE_ADD_TEST(suite, test_buildingowner_goes_to_other_when_empty);
    SUITE_ADD_TEST(suite, test_buildingowner_goes_to_same_faction_when_empty);
    SUITE_ADD_TEST(suite, test_buildingowner_goes_to_next_after_leave);
    SUITE_ADD_TEST(suite, test_buildingowner_goes_to_other_after_leave);
    SUITE_ADD_TEST(suite, test_buildingowner_goes_to_same_faction_after_leave);
    SUITE_ADD_TEST(suite, test_buildingowner_goes_to_empty_unit_after_leave);
    SUITE_ADD_TEST(suite, test_active_building);
    SUITE_ADD_TEST(suite, test_safe_building);
    return suite;
}
