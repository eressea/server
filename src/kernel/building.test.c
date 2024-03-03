#include "building.h"

#include "build.h"  // for construction
#include "calendar.h"
#include "config.h"
#include "magic.h"  // for make_icastle
#include "race.h"
#include "region.h"
#include "unit.h"

#include "util/variant.h"  // for variant

#include <strings.h>

#include <stb_ds.h>

#include <CuTest.h>
#include <tests.h>
#include <stdbool.h>       // for false, true
#include <stdlib.h>
#include <assert.h>

static void test_register_building(CuTest * tc)
{
    building_type *btype;
    int cache = 0;

    test_setup();

    CuAssertIntEquals(tc, true, bt_changed(&cache));
    CuAssertIntEquals(tc, false, bt_changed(&cache));

    btype = bt_get_or_create("herp");
    CuAssertIntEquals(tc, true, bt_changed(&cache));
    CuAssertPtrEquals(tc, btype, (void *)bt_find("herp"));

    free_buildingtypes();
    CuAssertIntEquals(tc, true, bt_changed(&cache));
    test_teardown();
}

static void test_building_set_owner(CuTest * tc)
{
    struct region *r;
    struct building *bld;
    struct unit *u1, *u2;
    struct faction *f;

    test_setup();

    f = test_create_faction();
    r = test_create_plain(0, 0);

    bld = test_create_building(r, NULL);
    u1 = test_create_unit(f, r);
    u_set_building(u1, bld);
    CuAssertPtrEquals(tc, u1, building_owner(bld));

    u2 = test_create_unit(f, r);
    u_set_building(u2, bld);
    CuAssertPtrEquals(tc, u1, building_owner(bld));
    building_set_owner(u2);
    CuAssertPtrEquals(tc, u2, building_owner(bld));
    test_teardown();
}

static void test_buildingowner_goes_to_next_when_empty(CuTest * tc)
{
    struct region *r;
    struct building *bld;
    struct unit *u, *u2;
    struct faction *f;

    test_setup();

    f = test_create_faction();
    r = test_create_plain(0, 0);

    bld = test_create_building(r, NULL);
    CuAssertPtrNotNull(tc, bld);

    u = test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);
    u_set_building(u, bld);
    u_set_building(u2, bld);
    CuAssertPtrEquals(tc, u, building_owner(bld));
    u->number = 0;
    CuAssertPtrEquals(tc, u2, building_owner(bld));
    test_teardown();
}

static void test_buildingowner_goes_to_other_when_empty(CuTest * tc)
{
    struct region *r;
    struct building *bld;
    struct unit *u, *u2;
    struct faction *f;

    test_setup();

    f = test_create_faction();
    r = test_create_plain(0, 0);

    bld = test_create_building(r, NULL);
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
    test_teardown();
}

static void test_buildingowner_goes_to_same_faction_when_empty(CuTest * tc)
{
    struct region *r;
    struct building *bld;
    struct unit *u, *u2, *u3;
    struct faction *f1, *f2;

    test_setup();

    f1 = test_create_faction();
    f2 = test_create_faction();
    r = test_create_plain(0, 0);

    bld = test_create_building(r, NULL);
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
    test_teardown();
}

static void test_buildingowner_goes_to_next_after_leave(CuTest * tc)
{
    struct region *r;
    struct building *bld;
    struct unit *u, *u2;
    struct faction *f;

    test_setup();
    f = test_create_faction();
    r = test_create_plain(0, 0);

    bld = test_create_building(r, NULL);
    CuAssertPtrNotNull(tc, bld);

    u = test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);
    u_set_building(u, bld);
    u_set_building(u2, bld);
    CuAssertPtrEquals(tc, u, building_owner(bld));
    leave_building(u);
    CuAssertPtrEquals(tc, u2, building_owner(bld));
    test_teardown();
}

static void test_buildingowner_goes_to_other_after_leave(CuTest * tc)
{
    struct region *r;
    struct building *bld;
    struct unit *u, *u2;
    struct faction *f;

    test_setup();

    f = test_create_faction();
    r = test_create_plain(0, 0);

    bld = test_create_building(r, NULL);
    CuAssertPtrNotNull(tc, bld);

    u2 = test_create_unit(f, r);
    u = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);
    u_set_building(u, bld);
    u_set_building(u2, bld);
    CuAssertPtrEquals(tc, u, building_owner(bld));
    leave_building(u);
    CuAssertPtrEquals(tc, u2, building_owner(bld));
    test_teardown();
}

static void test_buildingowner_goes_to_same_faction_after_leave(CuTest * tc)
{
    struct region *r;
    struct building *bld;
    struct unit *u, *u2, *u3;
    struct faction *f1, *f2;

    test_setup();

    f1 = test_create_faction();
    f2 = test_create_faction();
    r = test_create_plain(0, 0);

    bld = test_create_building(r, NULL);
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
    CuAssertPtrEquals(tc, NULL, building_owner(bld));
    test_teardown();
}

static void test_buildingowner_resets_when_empty(CuTest * tc)
{
    struct region *r;
    struct building *bld;
    struct unit *u;
    struct faction *f;

    test_setup();

    f = test_create_faction();
    r = test_create_plain(0, 0);

    bld = test_create_building(r, NULL);
    CuAssertPtrNotNull(tc, bld);

    u = test_create_unit(f, r);
    CuAssertPtrNotNull(tc, u);
    u_set_building(u, bld);
    CuAssertPtrEquals(tc, u, building_owner(bld));
    u->number = 0;
    CuAssertPtrEquals(tc, NULL, building_owner(bld));
    u->number = 1;
    CuAssertPtrEquals(tc, u, building_owner(bld));
    test_teardown();
}

void test_buildingowner_goes_to_empty_unit_after_leave(CuTest * tc)
{
    struct region *r;
    struct building *bld;
    struct unit *u1, *u2, *u3;
    struct faction *f1;

    test_setup();

    f1 = test_create_faction();
    r = test_create_plain(0, 0);

    bld = test_create_building(r, NULL);
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
    CuAssertPtrEquals(tc, NULL, building_owner(bld));
    u2->number = 1;
    CuAssertPtrEquals(tc, u2, building_owner(bld));
    test_teardown();
}

static void test_btype_defaults(CuTest *tc) {
    building_type * btype;

    test_setup();

    btype = bt_get_or_create("hodor");
    CuAssertPtrNotNull(tc, btype);
    CuAssertStrEquals(tc, "hodor", btype->_name);
    CuAssertPtrEquals(tc, NULL, btype->maintenance);
    CuAssertPtrEquals(tc, NULL, btype->a_stages);
    CuAssertDblEquals(tc, 1.0, btype->auraregen, 0.0);
    CuAssertIntEquals(tc, 0, btype->taxes);
    CuAssertIntEquals(tc, -1, btype->maxsize);
    CuAssertIntEquals(tc, 1, btype->capacity);
    CuAssertIntEquals(tc, -1, btype->maxcapacity);
    CuAssertIntEquals(tc, 0, btype->magres.sa[0]);
    CuAssertIntEquals(tc, 0, btype->magresbonus);
    CuAssertIntEquals(tc, 0, btype->fumblebonus);
    CuAssertIntEquals(tc, BTF_DEFAULT, btype->flags);
    test_teardown();
}

static void test_buildingtype_exists(CuTest * tc)
{
    region *r;
    building *b;
    building_type *btype, *btype2;

    test_setup();

    btype2 = test_create_castle();
    assert(btype2);
    btype = test_create_buildingtype("lighhouse");
    btype->maxsize = 10;

    r = test_create_plain(0, 0);
    b = test_create_building(r, btype);
    CuAssertPtrNotNull(tc, b);
    b->size = 10;

    CuAssertTrue(tc, !buildingtype_exists(r, NULL, false));
    CuAssertTrue(tc, !buildingtype_exists(r, btype2, false));

    CuAssertTrue(tc, buildingtype_exists(r, btype, false));
    b->size = 9;
    CuAssertTrue(tc, !buildingtype_exists(r, btype, false));
    btype->maxsize = 0;
    fset(b, BLD_UNMAINTAINED);
    CuAssertTrue(tc, buildingtype_exists(r, btype, false));
    btype->maxsize = 10;
    b->size = 10;

    freset(b, BLD_UNMAINTAINED);
    CuAssertTrue(tc, buildingtype_exists(r, btype, true));
    fset(b, BLD_UNMAINTAINED);
    CuAssertTrue(tc, !buildingtype_exists(r, btype, true));
    test_teardown();
}

static void test_active_building(CuTest *tc) {
    building *b;
    region *r;
    unit *u;
    building_type *btype;

    test_setup();

    btype = test_create_castle();
    assert(btype && btype->maxsize == -1);
    b = test_create_building(r = test_create_plain(0, 0), btype);
    u = test_create_unit(test_create_faction(), r);
    CuAssertIntEquals(tc, true, building_is_active(b));
    CuAssertPtrEquals(tc, NULL, active_building(u, btype));
    u_set_building(u, b);
    CuAssertIntEquals(tc, true, building_is_active(b));
    CuAssertPtrNotNull(tc, active_building(u, btype) );
    btype->maxsize = 10;
    b->size = btype->maxsize;
    CuAssertIntEquals(tc, true, building_is_active(b));
    CuAssertPtrNotNull(tc, active_building(u, btype) );
    b->size = 9;
    CuAssertIntEquals(tc, false, building_is_active(b));
    CuAssertPtrEquals(tc, NULL, active_building(u, btype));
    btype->maxsize = -1;
    b->flags |= BLD_UNMAINTAINED;
    CuAssertIntEquals(tc, false, building_is_active(b));
    CuAssertPtrEquals(tc, NULL, active_building(u, btype));
    test_teardown();
}

static void test_safe_building(CuTest *tc) {
    building_type *btype;
    unit *u1, *u2;

    test_setup();
    btype = test_create_buildingtype("house");
    u1 = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u2 = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    CuAssertTrue(tc, !in_safe_building(u1, u2));
    u1->building = test_create_building(u1->region, btype);
    CuAssertTrue(tc, !in_safe_building(u1, u2));
    btype->flags |= BTF_FORTIFICATION;
    CuAssertTrue(tc, in_safe_building(u1, u2));
    u2->building = u1->building;
    CuAssertTrue(tc, in_safe_building(u1, u2));
    u1->number = 2;
    CuAssertTrue(tc, !in_safe_building(u1, u2));
    u1->building->size = 3;
    CuAssertTrue(tc, !in_safe_building(u1, u2));
    test_teardown();
}

static void test_visible_building(CuTest *tc) {
    building_type *bt_castle, *bt_light;
    building *b;

    test_setup();
    bt_light = test_create_buildingtype("lighthouse");
    bt_castle = test_create_castle();

    b = test_create_building(test_create_plain(0, 0), bt_light);
    CuAssertPtrEquals(tc, bt_light, (void *)visible_building(b));
    make_icastle(b, bt_castle, 1);
    CuAssertPtrEquals(tc, bt_castle, (void *)visible_building(b));

    test_teardown();
}

static void test_building_type(CuTest *tc) {
    building_type *btype;
    test_setup();
    btype = test_create_buildingtype("house");
    CuAssertIntEquals(tc, true, is_building_type(btype, "house"));
    CuAssertIntEquals(tc, false, is_building_type(btype, "castle"));
    test_teardown();
}

static void test_cmp_castle_size(CuTest *tc) {
    region *r;
    building *b1, *b2;
    unit *u1, *u2;

    test_setup();
    r = test_create_plain(0, 0);
    b1 = test_create_building(r, test_create_castle());
    b2 = test_create_building(r, test_create_castle());
    u1 = test_create_unit(test_create_faction(), r);
    u_set_building(u1, b1);
    u2 = test_create_unit(test_create_faction(), r);
    u_set_building(u2, b2);
    b1->size = 5;
    b2->size = 10;
    CuAssertTrue(tc, cmp_castle_size(b1, b2) < 0);
    CuAssertTrue(tc, cmp_castle_size(b2, b1) > 0);
    CuAssertTrue(tc, cmp_castle_size(b1, b1) == 0);
    test_teardown();
}

static void test_wage(CuTest *tc) {
    region *r;
    building *b;
    building_type *btype;
    race *rc_orc, *rc_elf;
    test_setup();
    rc_orc = test_create_race("orc");
    rc_elf = test_create_race("elf");
    rc_elf->maintenance = 13;
    btype = test_create_castle();
    r = test_create_plain(0, 0);
    CuAssertIntEquals(tc, 10, wage(r, rc_elf));
    CuAssertIntEquals(tc, 10, wage(r, rc_orc));
    CuAssertIntEquals(tc, 11, peasant_wage(r, false));
    CuAssertIntEquals(tc, 10, peasant_wage(r, true));

    b = test_create_building(r, btype);
    b->size = 1;
    CuAssertIntEquals(tc, 0, buildingeffsize(b, false));
    CuAssertIntEquals(tc, 10, wage(r, rc_elf));
    CuAssertIntEquals(tc, 10, wage(r, rc_orc));
    CuAssertIntEquals(tc, 11, peasant_wage(r, false));
    CuAssertIntEquals(tc, 10, peasant_wage(r, true));
    b->size = 2;
    CuAssertIntEquals(tc, 1, buildingeffsize(b, false));
    b->size = 9;
    CuAssertIntEquals(tc, 1, buildingeffsize(b, false));
    CuAssertIntEquals(tc, 10, wage(r, rc_elf));
    CuAssertIntEquals(tc, 10, wage(r, rc_orc));
    CuAssertIntEquals(tc, 11, peasant_wage(r, false));
    CuAssertIntEquals(tc, 10, peasant_wage(r, true));
    b->size = 10;
    CuAssertIntEquals(tc, 2, buildingeffsize(b, false));
    b->size = 49;
    CuAssertIntEquals(tc, 2, buildingeffsize(b, false));
    CuAssertIntEquals(tc, 11, wage(r, rc_elf));
    CuAssertIntEquals(tc, 11, wage(r, rc_orc));
    CuAssertIntEquals(tc, 12, peasant_wage(r, false));
    CuAssertIntEquals(tc, 10, peasant_wage(r, true));
    b->size = 50;
    CuAssertIntEquals(tc, 3, buildingeffsize(b, false));
    b->size = 249;
    CuAssertIntEquals(tc, 3, buildingeffsize(b, false));
    CuAssertIntEquals(tc, 12, wage(r, rc_elf));
    CuAssertIntEquals(tc, 11, wage(r, rc_orc));
    CuAssertIntEquals(tc, 13, peasant_wage(r, false));
    CuAssertIntEquals(tc, 10, peasant_wage(r, true));
    b->size = 250;
    CuAssertIntEquals(tc, 4, buildingeffsize(b, false));
    CuAssertIntEquals(tc, 13, wage(r, rc_elf));
    CuAssertIntEquals(tc, 12, wage(r, rc_orc));
    CuAssertIntEquals(tc, 14, peasant_wage(r, false));
    CuAssertIntEquals(tc, 10, peasant_wage(r, true));
    config_set_int("rules.wage.function", 1);
    CuAssertIntEquals(tc, 13, wage(r, rc_elf));
    config_set_int("rules.wage.function", 0);
    CuAssertIntEquals(tc, 0, wage(r, rc_elf));
    config_set_int("rules.wage.function", 2);
    CuAssertIntEquals(tc, rc_elf->maintenance, wage(r, rc_elf));
    test_teardown();
}

static void test_cmp_wage(CuTest *tc) {
    region *r;
    building *b1, *b2;
    building_type *btype;

    test_setup();
    btype = test_create_castle();
    btype->taxes = 100;
    r = test_create_plain(0, 0);
    b1 = test_create_building(r, btype);
    b2 = test_create_building(r, btype);
    b1->size = 5;
    b2->size = 10;
    CuAssertPtrEquals(tc, b2, largestbuilding(r, cmp_wage, false));
    CuAssertTrue(tc, cmp_wage(b1, b2) < 0);
    CuAssertTrue(tc, cmp_wage(b2, b1) > 0);
    CuAssertTrue(tc, cmp_wage(b1, b1) == 0);
    test_teardown();
}

static void test_cmp_taxes(CuTest *tc) {
    region *r;
    building *b1, *b2;
    building_type *btype;
    unit *u1, *u2;

    test_setup();
    btype = test_create_castle();
    btype->taxes = 100;
    r = test_create_plain(0, 0);
    b1 = test_create_building(r, btype);
    b2 = test_create_building(r, btype);
    b1->size = 5;
    b2->size = 10;
    u1 = test_create_unit(test_create_faction(), r);
    u_set_building(u1, b1);
    u2 = test_create_unit(test_create_faction(), r);
    u_set_building(u2, b2);
    CuAssertPtrEquals(tc, b2, largestbuilding(r, cmp_taxes, false));
    CuAssertTrue(tc, cmp_taxes(b1, b2) < 0);
    CuAssertTrue(tc, cmp_taxes(b2, b1) > 0);
    CuAssertTrue(tc, cmp_taxes(b1, b1) == 0);
    test_teardown();
}

static void test_cmp_current_owner(CuTest *tc) {
    region *r;
    building *b1, *b2;
    building_type *btype;
    unit *u1, *u2;

    test_setup();
    config_set("rules.region_owners", "1");
    r = test_create_plain(0, 0);
    btype = test_create_buildingtype("watch");
    btype->a_stages[0].construction.maxsize = 1;
    btype->taxes = 200;
    b1 = test_create_building(r, btype);
    btype = test_create_castle();
    btype->a_stages[0].construction.maxsize = 1;
    btype->taxes = 100;
    b2 = test_create_building(r, btype);
    b1->size = 1;
    CuAssertIntEquals(tc, 1, buildingeffsize(b1, false));
    b2->size = 1;
    CuAssertIntEquals(tc, 1, buildingeffsize(b2, false));
    u1 = test_create_unit(test_create_faction(), r);
    u_set_building(u1, b1);
    u2 = test_create_unit(test_create_faction(), r);
    u_set_building(u2, b2);
    region_set_owner(r, u1->faction, turn);
    CuAssertPtrEquals(tc, b1, largestbuilding(r, cmp_current_owner, false));
    CuAssertTrue(tc, cmp_current_owner(b2, b1) < 0);
    CuAssertTrue(tc, cmp_current_owner(b1, b2) > 0);
    CuAssertTrue(tc, cmp_current_owner(b1, b1) == 0);
    test_teardown();
}

static void test_building_effsize(CuTest *tc) {
    building *b;
    building_type *btype;

    test_setup();
    btype = test_create_castle();
    CuAssertTrue(tc, arrlen(btype->a_stages) >= 3);
    btype->a_stages[0].construction.maxsize = 5;
    btype->a_stages[1].construction.maxsize = 5;
    btype->a_stages[2].construction.maxsize = -1;

    b = test_create_building(test_create_region(0,0,0), btype);
    b->size = 1;
    CuAssertIntEquals(tc, 0, buildingeffsize(b, false));
    b->size = 5;
    CuAssertIntEquals(tc, 1, buildingeffsize(b, false));
    b->size = 10;
    CuAssertIntEquals(tc, 2, buildingeffsize(b, false));
    b->size = 20;
    CuAssertIntEquals(tc, 2, buildingeffsize(b, false));
    test_teardown();
}

static int cmp_size(const building *lhs, const building *rhs) {
    assert(lhs);
    return rhs ? lhs->size - rhs->size : 1;
}

static void test_largestbuilding(CuTest *tc) {
    region *r;
    building *b1, *b2;
    test_setup();
    r = test_create_plain(0, 0);
    CuAssertPtrEquals(tc, NULL, largestbuilding(r, cmp_size, false));
    b1 = test_create_building(r, NULL);
    b2 = test_create_building(r, NULL);
    b1->size = 1;
    b2->size = 1;
    CuAssertPtrEquals(tc, b1, largestbuilding(r, cmp_size, false));
    b1->size = 2;
    CuAssertPtrEquals(tc, b1, largestbuilding(r, cmp_size, false));
    b2->size = 3;
    CuAssertPtrEquals(tc, b2, largestbuilding(r, cmp_size, false));
    test_teardown();
}

static void test_buildingtype(CuTest *tc) {
    building_type *btype;
    test_setup();

    btype = bt_get_or_create("portal");
    CuAssertPtrEquals(tc, NULL, btype->a_stages);
    CuAssertStrEquals(tc, "portal", buildingtype(btype, NULL, 1));

    btype = test_create_buildingtype("hodor");
    CuAssertPtrNotNull(tc, btype->a_stages);
    CuAssertPtrEquals(tc, NULL, btype->a_stages[0].name);
    CuAssertStrEquals(tc, "hodor", buildingtype(btype, NULL, 1));

    btype->a_stages[0].name = str_strdup("foobar");
    CuAssertStrEquals(tc, "foobar", buildingtype(btype, NULL, 1));

    test_teardown();
}

static void test_buildingcapacity(CuTest *tc) {
    building *b;
    building_type *btype;
    test_setup();
    btype = test_create_buildingtype("lighthouse");
    btype->capacity = 1;
    btype->maxcapacity = 4;
    b = test_create_building(test_create_plain(0, 0), btype);

    b->size = 1;
    CuAssertIntEquals(tc, b->size*btype->capacity, buildingcapacity(b));
    b->size = 5;
    CuAssertIntEquals(tc, btype->maxcapacity, buildingcapacity(b));

    btype->capacity = -1;
    CuAssertTrue(tc, building_finished(b));
    CuAssertIntEquals(tc, btype->maxcapacity, buildingcapacity(b));
    test_teardown();
}

CuSuite *get_building_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_buildingtype);
    SUITE_ADD_TEST(suite, test_largestbuilding);
    SUITE_ADD_TEST(suite, test_buildingcapacity);
    SUITE_ADD_TEST(suite, test_cmp_castle_size);
    SUITE_ADD_TEST(suite, test_cmp_taxes);
    SUITE_ADD_TEST(suite, test_cmp_wage);
    SUITE_ADD_TEST(suite, test_wage);
    SUITE_ADD_TEST(suite, test_cmp_current_owner);
    SUITE_ADD_TEST(suite, test_register_building);
    SUITE_ADD_TEST(suite, test_btype_defaults);
    SUITE_ADD_TEST(suite, test_building_set_owner);
    SUITE_ADD_TEST(suite, test_building_effsize);
    SUITE_ADD_TEST(suite, test_buildingowner_resets_when_empty);
    SUITE_ADD_TEST(suite, test_buildingowner_goes_to_next_when_empty);
    SUITE_ADD_TEST(suite, test_buildingowner_goes_to_other_when_empty);
    SUITE_ADD_TEST(suite, test_buildingowner_goes_to_same_faction_when_empty);
    SUITE_ADD_TEST(suite, test_buildingowner_goes_to_next_after_leave);
    SUITE_ADD_TEST(suite, test_buildingowner_goes_to_other_after_leave);
    SUITE_ADD_TEST(suite, test_buildingowner_goes_to_same_faction_after_leave);
    SUITE_ADD_TEST(suite, test_buildingowner_goes_to_empty_unit_after_leave);
    SUITE_ADD_TEST(suite, test_building_type);
    SUITE_ADD_TEST(suite, test_active_building);
    SUITE_ADD_TEST(suite, test_buildingtype_exists);
    SUITE_ADD_TEST(suite, test_visible_building);
    SUITE_ADD_TEST(suite, test_safe_building);
    return suite;
}
