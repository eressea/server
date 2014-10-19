#include <platform.h>
#include <kernel/types.h>
#include "laws.h"

#include <kernel/config.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

#include <util/language.h>

#include <CuTest.h>
#include <tests.h>

#include <assert.h>

static void test_new_building_can_be_renamed(CuTest * tc)
{
    region *r;
    building *b;
    building_type *btype;

    test_cleanup();
    test_create_world();

    btype = bt_get_or_create("castle");
    r = findregion(-1, 0);

    b = new_building(btype, r, default_locale);
    CuAssertTrue(tc, !renamed_building(b));
}

static void test_rename_building(CuTest * tc)
{
    region *r;
    building *b;
    unit *u;
    faction *f;
    building_type *btype;

    test_cleanup();
    test_create_world();

    btype = bt_get_or_create("castle");

    r = findregion(-1, 0);
    b = new_building(btype, r, default_locale);
    f = test_create_faction(NULL);
    u = test_create_unit(f, r);
    u_set_building(u, b);

    rename_building(u, NULL, b, "Villa Nagel");
    CuAssertStrEquals(tc, "Villa Nagel", b->name);
}

static void test_rename_building_twice(CuTest * tc)
{
    region *r;
    building *b;
    unit *u;
    faction *f;
    building_type *btype;

    test_cleanup();
    test_create_world();

    btype = bt_get_or_create("castle");

    r = findregion(-1, 0);
    b = new_building(btype, r, default_locale);
    f = test_create_faction(NULL);
    u = test_create_unit(f, r);
    u_set_building(u, b);

    rename_building(u, NULL, b, "Villa Nagel");
    CuAssertStrEquals(tc, "Villa Nagel", b->name);

    rename_building(u, NULL, b, "Villa Kunterbunt");
    CuAssertStrEquals(tc, "Villa Kunterbunt", b->name);
}

static void test_fishing_feeds_2_people(CuTest * tc)
{
    const resource_type *rtype;
    region *r;
    faction *f;
    unit *u;
    ship *sh;

    test_cleanup();
    test_create_world();
    r = findregion(-1, 0);
    CuAssertStrEquals(tc, "ocean", r->terrain->_name);    /* test_create_world needs coverage */
    f = test_create_faction(NULL);
    u = test_create_unit(f, r);
    sh = new_ship(st_find("boat"), r, 0);
    u_set_ship(u, sh);
    rtype = get_resourcetype(R_SILVER);
    i_change(&u->items, rtype->itype, 42);

    scale_number(u, 1);
    sh->flags |= SF_FISHING;
    get_food(r);
    CuAssertIntEquals(tc, 42, i_get(u->items, rtype->itype));

    scale_number(u, 2);
    sh->flags |= SF_FISHING;
    get_food(r);
    CuAssertIntEquals(tc, 42, i_get(u->items, rtype->itype));

    scale_number(u, 3);
    sh->flags |= SF_FISHING;
    get_food(r);
    CuAssertIntEquals(tc, 32, i_get(u->items, rtype->itype));
}

static int not_so_hungry(const unit * u)
{
    return 6 * u->number;
}

static void test_fishing_does_not_give_goblins_money(CuTest * tc)
{
    const resource_type *rtype;
    region *r;
    faction *f;
    unit *u;
    ship *sh;

    test_cleanup();
    test_create_world();
    rtype = get_resourcetype(R_SILVER);

    r = findregion(-1, 0);
    CuAssertStrEquals(tc, "ocean", r->terrain->_name);    /* test_create_world needs coverage */
    f = test_create_faction(NULL);
    u = test_create_unit(f, r);
    sh = new_ship(st_find("boat"), r, 0);
    u_set_ship(u, sh);
    i_change(&u->items, rtype->itype, 42);

    global.functions.maintenance = not_so_hungry;
    scale_number(u, 2);
    sh->flags |= SF_FISHING;
    get_food(r);
    CuAssertIntEquals(tc, 42, i_get(u->items, rtype->itype));
}

static void test_fishing_gets_reset(CuTest * tc)
{
    const resource_type *rtype;
    region *r;
    faction *f;
    unit *u;
    ship *sh;

    test_cleanup();
    test_create_world();
    rtype = get_resourcetype(R_SILVER);
    r = findregion(-1, 0);
    CuAssertStrEquals(tc, "ocean", r->terrain->_name);    /* test_create_world needs coverage */
    f = test_create_faction(NULL);
    u = test_create_unit(f, r);
    sh = new_ship(st_find("boat"), r, 0);
    u_set_ship(u, sh);
    i_change(&u->items, rtype->itype, 42);

    scale_number(u, 1);
    sh->flags |= SF_FISHING;
    get_food(r);
    CuAssertIntEquals(tc, 42, i_get(u->items, rtype->itype));

    scale_number(u, 1);
    get_food(r);
    CuAssertIntEquals(tc, 32, i_get(u->items, rtype->itype));
}

static void test_unit_limit(CuTest * tc)
{
    set_param(&global.parameters, "rules.limit.faction", "250");
    CuAssertIntEquals(tc, 250, rule_faction_limit());

    set_param(&global.parameters, "rules.limit.faction", "200");
    CuAssertIntEquals(tc, 200, rule_faction_limit());

    set_param(&global.parameters, "rules.limit.alliance", "250");
    CuAssertIntEquals(tc, 250, rule_alliance_limit());

}

extern int checkunitnumber(const faction * f, int add);
static void test_cannot_create_unit_above_limit(CuTest * tc)
{
    faction *f;

    test_cleanup();
    test_create_world();
    f = test_create_faction(NULL);
    set_param(&global.parameters, "rules.limit.faction", "4");

    CuAssertIntEquals(tc, 0, checkunitnumber(f, 4));
    CuAssertIntEquals(tc, 2, checkunitnumber(f, 5));

    set_param(&global.parameters, "rules.limit.alliance", "3");
    CuAssertIntEquals(tc, 0, checkunitnumber(f, 3));
    CuAssertIntEquals(tc, 1, checkunitnumber(f, 4));
}

static void test_reserve_cmd(CuTest *tc) {
    unit *u1, *u2;
    faction *f;
    region *r;
    order *ord;
    const resource_type *rtype;
    const struct locale *loc;

    test_cleanup();
    test_create_world();

    rtype = get_resourcetype(R_SILVER);
    assert(rtype && rtype->itype);
    f = test_create_faction(NULL);
    r = findregion(0, 0);
    assert(r && f);
    u1 = test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    assert(u1 && u2);
    loc = get_locale("de");
    assert(loc);
    ord = create_order(K_RESERVE, loc, "200 SILBER");
    assert(ord);
    i_change(&u1->items, rtype->itype, 100);
    i_change(&u2->items, rtype->itype, 100);
    CuAssertIntEquals(tc, 200, reserve_cmd(u1, ord));
    CuAssertIntEquals(tc, 200, i_get(u1->items, rtype->itype));
    CuAssertIntEquals(tc, 0, i_get(u2->items, rtype->itype));
    test_cleanup();
}

static void test_new_units(CuTest *tc) {
    unit *u;
    faction *f;
    region *r;
    order *ord;
    const struct locale *loc;
    test_cleanup();
    test_create_world();
    f = test_create_faction(NULL);
    r = findregion(0, 0);
    assert(r && f);
    u = test_create_unit(f, r);
    assert(u && !u->next);
    loc = get_locale("de");
    assert(loc);
    ord = create_order(K_MAKETEMP, loc, "hurr");
    assert(ord);
    u->orders = ord;
    new_units();
    CuAssertPtrNotNull(tc, u->next);
    test_cleanup();
}

typedef struct guard_fixture {
    unit * u;
} guard_fixture;

void setup_guard(guard_fixture *fix, bool armed) {
    region *r;
    faction *f;
    unit * u;

    test_cleanup();
    test_create_world();

    f = test_create_faction(NULL);
    r = findregion(0, 0);
    assert(r && f);
    u = test_create_unit(f, r);
    fset(u, UFL_GUARD);
    u->status = ST_FIGHT;

    if (armed) {
        item_type *itype;
        itype = it_get_or_create(rt_get_or_create("sword"));
        new_weapontype(itype, 0, 0.0, NULL, 0, 0, 0, SK_MELEE, 2);
        i_change(&u->items, itype, 1);
        set_level(u, SK_MELEE, 2);
    }
    fix->u = u;
}

static void test_update_guards(CuTest *tc) {
    guard_fixture fix;

    setup_guard(&fix, true);

    update_guards();
    CuAssertTrue(tc, fval(fix.u, UFL_GUARD));
    freset(fix.u, UFL_GUARD);
    update_guards();
    CuAssertTrue(tc, !fval(fix.u, UFL_GUARD));
    test_cleanup();
}

static void test_newbie_cannot_guard(CuTest *tc) {
    guard_fixture fix;

    setup_guard(&fix, true);
    set_param(&global.parameters, "NewbieImmunity", "4");
    CuAssertTrue(tc, IsImmune(fix.u->faction));
    update_guards();
    CuAssertTrue(tc, !fval(fix.u, UFL_GUARD));
    test_cleanup();
}

static void test_unarmed_cannot_guard(CuTest *tc) {
    guard_fixture fix;

    setup_guard(&fix, false);
    update_guards();
    CuAssertTrue(tc, !fval(fix.u, UFL_GUARD));
    test_cleanup();
}

static void test_unarmed_races_can_guard(CuTest *tc) {
    guard_fixture fix;
    race * rc;

    setup_guard(&fix, false);
    rc = rc_get_or_create(fix.u->race_->_name);
    rc->flags |= RCF_UNARMEDGUARD;
    update_guards();
    CuAssertTrue(tc, fval(fix.u, UFL_GUARD));
    test_cleanup();
}

static void test_low_skill_cannot_guard(CuTest *tc) {
    guard_fixture fix;

    setup_guard(&fix, true);
    set_level(fix.u, SK_MELEE, 1);
    fix.u->status = ST_FLEE;
    update_guards();
    CuAssertTrue(tc, !fval(fix.u, UFL_GUARD));
    test_cleanup();
}

static void test_fleeing_cannot_guard(CuTest *tc) {
    guard_fixture fix;

    setup_guard(&fix, true);
    fix.u->status = ST_FLEE;
    update_guards();
    CuAssertTrue(tc, !fval(fix.u, UFL_GUARD));
    test_cleanup();
}

static void test_reserve_self(CuTest *tc) {
    unit *u1, *u2;
    faction *f;
    region *r;
    order *ord;
    const resource_type *rtype;
    const struct locale *loc;

    test_cleanup();
    test_create_world();

    rtype = get_resourcetype(R_SILVER);
    assert(rtype && rtype->itype);
    f = test_create_faction(NULL);
    r = findregion(0, 0);
    assert(r && f);
    u1 = test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    assert(u1 && u2);
    loc = get_locale("de");
    assert(loc);
    ord = create_order(K_RESERVE, loc, "200 SILBER");
    assert(ord);
    i_change(&u1->items, rtype->itype, 100);
    i_change(&u2->items, rtype->itype, 100);
    CuAssertIntEquals(tc, 100, reserve_self(u1, ord));
    CuAssertIntEquals(tc, 100, i_get(u1->items, rtype->itype));
    CuAssertIntEquals(tc, 100, i_get(u2->items, rtype->itype));
    test_cleanup();
}

CuSuite *get_laws_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_new_building_can_be_renamed);
    SUITE_ADD_TEST(suite, test_rename_building);
    SUITE_ADD_TEST(suite, test_rename_building_twice);
    SUITE_ADD_TEST(suite, test_fishing_feeds_2_people);
    SUITE_ADD_TEST(suite, test_fishing_does_not_give_goblins_money);
    SUITE_ADD_TEST(suite, test_fishing_gets_reset);
    SUITE_ADD_TEST(suite, test_unit_limit);
    SUITE_ADD_TEST(suite, test_update_guards);
    SUITE_ADD_TEST(suite, test_newbie_cannot_guard);
    SUITE_ADD_TEST(suite, test_unarmed_cannot_guard);
    SUITE_ADD_TEST(suite, test_unarmed_races_can_guard);
    SUITE_ADD_TEST(suite, test_fleeing_cannot_guard);
    SUITE_ADD_TEST(suite, test_low_skill_cannot_guard);
    SUITE_ADD_TEST(suite, test_reserve_self);
    SUITE_ADD_TEST(suite, test_reserve_cmd);
    SUITE_ADD_TEST(suite, test_new_units);
    SUITE_ADD_TEST(suite, test_cannot_create_unit_above_limit);
    return suite;
}
