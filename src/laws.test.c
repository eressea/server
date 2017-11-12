#include <platform.h>
#include "laws.h"
#include "battle.h"
#include "guard.h"
#include "monsters.h"

#include <kernel/ally.h>
#include <kernel/alliance.h>
#include <kernel/config.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>

#include <util/attrib.h>
#include <util/base36.h>
#include <util/language.h>
#include <util/message.h>
#include <util/rand.h>

#include <CuTest.h>
#include <tests.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_new_building_can_be_renamed(CuTest * tc)
{
    region *r;
    building *b;

    test_setup();
    test_create_locale();
    r = test_create_region(0, 0, 0);

    b = test_create_building(r, NULL);
    CuAssertTrue(tc, !renamed_building(b));
    test_cleanup();
}

static void test_rename_building(CuTest * tc)
{
    region *r;
    building *b;
    unit *u;
    faction *f;
    building_type *btype;

    test_setup();
    test_create_locale();
    btype = test_create_buildingtype("castle");
    r = test_create_region(0, 0, 0);
    b = new_building(btype, r, default_locale);
    f = test_create_faction(NULL);
    u = test_create_unit(f, r);
    u_set_building(u, b);

    rename_building(u, NULL, b, "Villa Nagel");
    CuAssertStrEquals(tc, "Villa Nagel", b->name);
    CuAssertTrue(tc, renamed_building(b));
    test_cleanup();
}

static void test_rename_building_twice(CuTest * tc)
{
    region *r;
    unit *u;
    faction *f;
    building *b;
    building_type *btype;

    test_setup();
    test_create_locale();
    btype = test_create_buildingtype("castle");
    r = test_create_region(0, 0, 0);
    b = new_building(btype, r, default_locale);
    f = test_create_faction(NULL);
    u = test_create_unit(f, r);
    u_set_building(u, b);

    rename_building(u, NULL, b, "Villa Nagel");
    CuAssertStrEquals(tc, "Villa Nagel", b->name);

    rename_building(u, NULL, b, "Villa Kunterbunt");
    CuAssertStrEquals(tc, "Villa Kunterbunt", b->name);
    test_cleanup();
}

static void test_contact(CuTest * tc)
{
    region *r;
    unit *u1, *u2, *u3;
    building *b;
    building_type *btype;
    ally *al;

    test_setup();
    test_create_locale();
    btype = test_create_buildingtype("castle");
    r = test_create_region(0, 0, 0);
    b = new_building(btype, r, default_locale);
    u1 = test_create_unit(test_create_faction(0), r);
    u2 = test_create_unit(test_create_faction(0), r);
    u3 = test_create_unit(test_create_faction(0), r);
    set_level(u3, SK_PERCEPTION, 2);
    usetsiege(u3, b);
    b->besieged = 1;
    CuAssertIntEquals(tc, 1, can_contact(r, u1, u2));

    u_set_building(u1, b);
    CuAssertIntEquals(tc, 0, can_contact(r, u1, u2));
    al = ally_add(&u1->faction->allies, u2->faction);
    al->status = HELP_ALL;
    CuAssertIntEquals(tc, HELP_GIVE, can_contact(r, u1, u2));
    u_set_building(u2, b);
    CuAssertIntEquals(tc, 1, can_contact(r, u1, u2));
    test_cleanup();
}

static void test_enter_building(CuTest * tc)
{
    unit *u;
    region *r;
    building *b;
    race * rc;

    test_setup();
    test_create_locale();
    r = test_create_region(0, 0, 0);
    rc = test_create_race("human");
    u = test_create_unit(test_create_faction(rc), r);
    b = test_create_building(r, test_create_buildingtype("castle"));

    rc->flags = RCF_WALK;
    u->building = 0;
    CuAssertIntEquals(tc, 1, enter_building(u, NULL, b->no, false));
    CuAssertPtrEquals(tc, b, u->building);

    rc->flags = RCF_FLY;
    u->building = 0;
    CuAssertIntEquals(tc, 1, enter_building(u, NULL, b->no, false));
    CuAssertPtrEquals(tc, b, u->building);

    rc->flags = RCF_SWIM;
    u->building = 0;
    CuAssertIntEquals(tc, 0, enter_building(u, NULL, b->no, false));
    CuAssertPtrEquals(tc, 0, u->building);
    CuAssertPtrEquals(tc, 0, u->faction->msgs);

    CuAssertIntEquals(tc, 0, enter_building(u, NULL, b->no, true));
    CuAssertPtrNotNull(tc, u->faction->msgs);

    test_cleanup();
}

static void test_enter_ship(CuTest * tc)
{
    unit *u;
    region *r;
    ship *sh;
    race * rc;

    test_cleanup();
    test_create_world();

    r = findregion(0, 0);
    rc = rc_get_or_create("human");
    u = test_create_unit(test_create_faction(rc), r);
    sh = test_create_ship(r, st_get_or_create("boat"));

    rc->flags = RCF_WALK;
    u->ship = 0;
    CuAssertIntEquals(tc, 1, enter_ship(u, NULL, sh->no, false));
    CuAssertPtrEquals(tc, sh, u->ship);

    rc->flags = RCF_FLY;
    u->ship = 0;
    CuAssertIntEquals(tc, 1, enter_ship(u, NULL, sh->no, false));
    CuAssertPtrEquals(tc, sh, u->ship);

    rc->flags = RCF_CANSAIL;
    u->ship = 0;
    CuAssertIntEquals(tc, 1, enter_ship(u, NULL, sh->no, false));
    CuAssertPtrEquals(tc, sh, u->ship);

    rc->flags = RCF_SWIM;
    u->ship = 0;
    CuAssertIntEquals(tc, 0, enter_ship(u, NULL, sh->no, false));
    CuAssertPtrEquals(tc, 0, u->ship);
    CuAssertPtrEquals(tc, 0, u->faction->msgs);

    CuAssertIntEquals(tc, 0, enter_ship(u, NULL, sh->no, true));
    CuAssertPtrNotNull(tc, u->faction->msgs);

    test_cleanup();
}

static void test_display_cmd(CuTest *tc) {
    unit *u;
    faction *f;
    region *r;
    order *ord;

    test_cleanup();
    r = test_create_region(0, 0, test_create_terrain("plain", LAND_REGION));
    f = test_create_faction(0);
    assert(r && f);
    u = test_create_unit(f, r);
    assert(u);

    ord = create_order(K_DISPLAY, f->locale, "%s Hodor", LOC(f->locale, parameters[P_UNIT]));
    CuAssertIntEquals(tc, 0, display_cmd(u, ord));
    CuAssertStrEquals(tc, "Hodor", u->display);
    free_order(ord);

    ord = create_order(K_DISPLAY, f->locale, LOC(f->locale, parameters[P_UNIT]));
    CuAssertIntEquals(tc, 0, display_cmd(u, ord));
    CuAssertPtrEquals(tc, NULL, u->display);
    free_order(ord);

    ord = create_order(K_DISPLAY, f->locale, "%s Hodor", LOC(f->locale, parameters[P_REGION]));
    CuAssertIntEquals(tc, 0, display_cmd(u, ord));
    CuAssertPtrEquals(tc, NULL, r->land->display);
    free_order(ord);

    test_cleanup();
}

static void test_rule_force_leave(CuTest *tc) {
    test_setup();
    config_set("rules.owners.force_leave", "0");
    CuAssertIntEquals(tc, false, rule_force_leave(FORCE_LEAVE_ALL));
    CuAssertIntEquals(tc, false, rule_force_leave(FORCE_LEAVE_POSTCOMBAT));
    config_set("rules.owners.force_leave", "1");
    CuAssertIntEquals(tc, false, rule_force_leave(FORCE_LEAVE_ALL));
    CuAssertIntEquals(tc, true, rule_force_leave(FORCE_LEAVE_POSTCOMBAT));
    config_set("rules.owners.force_leave", "2");
    CuAssertIntEquals(tc, true, rule_force_leave(FORCE_LEAVE_ALL));
    CuAssertIntEquals(tc, false, rule_force_leave(FORCE_LEAVE_POSTCOMBAT));
    config_set("rules.owners.force_leave", "3");
    CuAssertIntEquals(tc, true, rule_force_leave(FORCE_LEAVE_ALL));
    CuAssertIntEquals(tc, true, rule_force_leave(FORCE_LEAVE_POSTCOMBAT));
    test_cleanup();
}

static void test_force_leave_buildings(CuTest *tc) {
    ally *al;
    region *r;
    unit *u1, *u2, *u3;
    building * b;
    message *msg;

    test_setup();
    r = test_create_region(0, 0, test_create_terrain("plain", LAND_REGION));
    u1 = test_create_unit(test_create_faction(NULL), r);
    u2 = test_create_unit(u1->faction, r);
    u3 = test_create_unit(test_create_faction(NULL), r);
    b = test_create_building(r, NULL);
    u_set_building(u1, b);
    building_set_owner(u1);
    u_set_building(u2, b);
    u_set_building(u3, b);
    force_leave(r, NULL);
    CuAssertPtrEquals_Msg(tc, "owner should not be forced to leave", b, u1->building);
    CuAssertPtrEquals_Msg(tc, "same faction should not be forced to leave", b, u2->building);
    CuAssertPtrEquals_Msg(tc, "non-allies should be forced to leave", NULL, u3->building);
    msg = test_get_last_message(u3->faction->msgs);
    CuAssertStrEquals(tc, "force_leave_building", test_get_messagetype(msg));

    u_set_building(u3, b);
    al = ally_add(&u1->faction->allies, u3->faction);
    al->status = HELP_GUARD;
    force_leave(r, NULL);
    CuAssertPtrEquals_Msg(tc, "allies should not be forced to leave", b, u3->building);
    test_cleanup();
}

static void test_force_leave_ships(CuTest *tc) {
    region *r;
    unit *u1, *u2;
    ship *sh;
    message *msg;

    test_setup();
    r = test_create_region(0, 0, test_create_terrain("plain", LAND_REGION));
    u1 = test_create_unit(test_create_faction(NULL), r);
    u2 = test_create_unit(test_create_faction(NULL), r);
    sh = test_create_ship(r, NULL);
    u_set_ship(u1, sh);
    u_set_ship(u2, sh);
    ship_set_owner(u1);
    force_leave(r, NULL);
    CuAssertPtrEquals_Msg(tc, "non-allies should be forced to leave", NULL, u2->ship);
    msg = test_get_last_message(u2->faction->msgs);
    CuAssertStrEquals(tc, "force_leave_ship", test_get_messagetype(msg));
    test_cleanup();
}

static void test_force_leave_ships_on_ocean(CuTest *tc) {
    region *r;
    unit *u1, *u2;
    ship *sh;

    test_setup();
    r = test_create_region(0, 0, test_create_terrain("ocean", SEA_REGION));
    u1 = test_create_unit(test_create_faction(NULL), r);
    u2 = test_create_unit(test_create_faction(NULL), r);
    sh = test_create_ship(r, NULL);
    u_set_ship(u1, sh);
    u_set_ship(u2, sh);
    ship_set_owner(u1);
    force_leave(r, NULL);
    CuAssertPtrEquals_Msg(tc, "no forcing out of ships on oceans", sh, u2->ship);
    test_cleanup();
}

static void test_fishing_feeds_2_people(CuTest * tc)
{
    const resource_type *rtype;
    region *r;
    faction *f;
    unit *u;
    ship *sh;

    test_setup();
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
    test_cleanup();
}

static void test_fishing_does_not_give_goblins_money(CuTest * tc)
{
    const resource_type *rtype;
    region *r;
    faction *f;
    unit *u;
    ship *sh;

    test_setup();
    test_create_world();
    rtype = get_resourcetype(R_SILVER);

    r = findregion(-1, 0);
    CuAssertStrEquals(tc, "ocean", r->terrain->_name);    /* test_create_world needs coverage */
    f = test_create_faction(NULL);
    u = test_create_unit(f, r);
    sh = new_ship(st_find("boat"), r, 0);
    u_set_ship(u, sh);
    i_change(&u->items, rtype->itype, 42);

    scale_number(u, 2);
    sh->flags |= SF_FISHING;
    get_food(r);
    CuAssertIntEquals(tc, 42, i_get(u->items, rtype->itype));
    test_cleanup();
}

static void test_fishing_gets_reset(CuTest * tc)
{
    const resource_type *rtype;
    region *r;
    faction *f;
    unit *u;
    ship *sh;

    test_setup();
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
    test_cleanup();
}

static void test_unit_limit(CuTest * tc)
{
    test_setup();
    config_set("rules.limit.faction", "250");
    CuAssertIntEquals(tc, 250, rule_faction_limit());

    config_set("rules.limit.faction", "200");
    CuAssertIntEquals(tc, 200, rule_faction_limit());

    config_set("rules.limit.alliance", "250");
    CuAssertIntEquals(tc, 250, rule_alliance_limit());
    test_cleanup();
}

static void test_maketemp(CuTest * tc)
{
    faction *f;
    unit *u, *u2;

    test_setup();
    f = test_create_faction(NULL);
    u = test_create_unit(f, test_create_region(0, 0, NULL));

    u->orders = create_order(K_MAKETEMP, f->locale, "1");
    u->orders->next = create_order(K_ENTERTAIN, f->locale, NULL);
    u->orders->next->next = create_order(K_END, f->locale, NULL);
    u->orders->next->next->next = create_order(K_TAX, f->locale, NULL);

    new_units();
    CuAssertIntEquals(tc, 2, f->num_units);
    CuAssertPtrNotNull(tc, u2 = u->next);
    CuAssertPtrNotNull(tc, u2->orders);
    CuAssertPtrEquals(tc, NULL, u2->orders->next);
    CuAssertIntEquals(tc, K_ENTERTAIN, getkeyword(u2->orders));

    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrEquals(tc, NULL, u->orders->next);
    CuAssertIntEquals(tc, K_TAX, getkeyword(u->orders));
    test_cleanup();
}

static void test_maketemp_default_order(CuTest * tc)
{
    faction *f;
    unit *u, *u2;

    test_setup();
    config_set("orders.default", "work");
    f = test_create_faction(NULL);
    u = test_create_unit(f, test_create_region(0, 0, NULL));

    new_units();
    CuAssertIntEquals(tc, 1, f->num_units);

    u->orders = create_order(K_MAKETEMP, f->locale, "1");
    u->orders->next = create_order(K_END, f->locale, NULL);
    u->orders->next->next = create_order(K_TAX, f->locale, NULL);

    new_units();
    CuAssertIntEquals(tc, 2, f->num_units);
    CuAssertPtrNotNull(tc, u2 = u->next);
    CuAssertPtrNotNull(tc, u2->orders);
    CuAssertPtrEquals(tc, NULL, u2->orders->next);
    CuAssertIntEquals(tc, K_WORK, getkeyword(u2->orders));

    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrEquals(tc, NULL, u->orders->next);
    CuAssertIntEquals(tc, K_TAX, getkeyword(u->orders));
    test_cleanup();
}

static void test_limit_new_units(CuTest * tc)
{
    faction *f;
    unit *u;
    alliance *al;

    test_setup();
    al = makealliance(1, "Hodor");
    f = test_create_faction(NULL);
    u = test_create_unit(f, test_create_region(0, 0, NULL));
    CuAssertIntEquals(tc, 1, f->num_units);
    CuAssertIntEquals(tc, 1, f->num_people);
    scale_number(u, 10);
    CuAssertIntEquals(tc, 10, f->num_people);
    config_set("rules.limit.faction", "2");

    u->orders = create_order(K_MAKETEMP, f->locale, "1");
    new_units();
    CuAssertPtrNotNull(tc, u->next);
    CuAssertIntEquals(tc, 2, f->num_units);

    CuAssertPtrEquals(tc, NULL, u->orders);
    u->orders = create_order(K_MAKETEMP, f->locale, "1");
    new_units();
    CuAssertIntEquals(tc, 2, f->num_units);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "too_many_units_in_faction"));

    setalliance(f, al);

    config_set("rules.limit.faction", "3");
    config_set("rules.limit.alliance", "2");

    CuAssertPtrEquals(tc, NULL, u->orders);
    u->orders = create_order(K_MAKETEMP, f->locale, "1");
    new_units();
    CuAssertIntEquals(tc, 2, f->num_units);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "too_many_units_in_alliance"));

    config_set("rules.limit.alliance", "3");
    u = test_create_unit(test_create_faction(NULL), u->region);
    setalliance(u->faction, al);

    CuAssertPtrEquals(tc, NULL, u->orders);
    u->orders = create_order(K_MAKETEMP, f->locale, "1");
    new_units();
    CuAssertIntEquals(tc, 2, f->num_units);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "too_many_units_in_alliance"));

    test_cleanup();
}

static void test_cannot_create_unit_above_limit(CuTest * tc)
{
    faction *f;

    test_setup();
    test_create_world();
    f = test_create_faction(NULL);
    config_set("rules.limit.faction", "4");

    CuAssertIntEquals(tc, 0, checkunitnumber(f, 4));
    CuAssertIntEquals(tc, 2, checkunitnumber(f, 5));

    config_set("rules.limit.alliance", "3");
    CuAssertIntEquals(tc, 0, checkunitnumber(f, 3));
    CuAssertIntEquals(tc, 1, checkunitnumber(f, 4));
    test_cleanup();
}

static void test_reserve_cmd(CuTest *tc) {
    unit *u1, *u2;
    faction *f;
    region *r;
    order *ord;
    const resource_type *rtype;

    test_setup();
    test_create_world();

    rtype = get_resourcetype(R_SILVER);
    assert(rtype && rtype->itype);
    f = test_create_faction(NULL);
    r = findregion(0, 0);
    assert(r && f);
    u1 = test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    assert(u1 && u2);
    ord = create_order(K_RESERVE, f->locale, "200 SILBER");
    assert(ord);
    i_change(&u1->items, rtype->itype, 100);
    i_change(&u2->items, rtype->itype, 100);
    CuAssertIntEquals(tc, 200, reserve_cmd(u1, ord));
    CuAssertIntEquals(tc, 200, i_get(u1->items, rtype->itype));
    CuAssertIntEquals(tc, 0, i_get(u2->items, rtype->itype));
    free_order(ord);
    test_cleanup();
}

struct pay_fixture {
    unit *u1;
    unit *u2;
};

static void setup_pay_cmd(struct pay_fixture *fix) {
    faction *f;
    region *r;
    building *b;
    building_type *btcastle;

    test_create_world();
    f = test_create_faction(NULL);
    r = findregion(0, 0);
    assert(r && f);
    btcastle = test_create_buildingtype("castle");
    btcastle->taxes = 100;
    b = test_create_building(r, btcastle);
    assert(b);
    fix->u1 = test_create_unit(f, r);
    fix->u2 = test_create_unit(f, r);
    assert(fix->u1 && fix->u2);
    u_set_building(fix->u1, b);
    u_set_building(fix->u2, b);
    assert(building_owner(b) == fix->u1);
    test_translate_param(f->locale, P_NOT, "NOT");
}

static void test_pay_cmd(CuTest *tc) {
    struct pay_fixture fix;
    order *ord;
    faction *f;
    building *b;

    test_setup();
    setup_pay_cmd(&fix);
    b = fix.u1->building;
    f = fix.u1->faction;

    ord = create_order(K_PAY, f->locale, "NOT");
    assert(ord);
    CuAssertIntEquals(tc, 0, pay_cmd(fix.u1, ord));
    CuAssertIntEquals(tc, BLD_DONTPAY, b->flags&BLD_DONTPAY);
    free_order(ord);
    test_cleanup();
}

static void test_pay_cmd_other_building(CuTest *tc) {
    struct pay_fixture fix;
    order *ord;
    faction *f;
    building *b;

    test_setup();
    setup_pay_cmd(&fix);
    f = fix.u1->faction;
    /* lighthouse is not in the test locale, so we're using castle */
    b = test_create_building(fix.u1->region, test_create_buildingtype("lighthouse"));
    config_set("rules.region_owners", "1");
    config_set("rules.region_owner_pay_building", "lighthouse");
    update_owners(b->region);

    ord = create_order(K_PAY, f->locale, "NOT %s", itoa36(b->no));
    assert(ord);
    CuAssertPtrEquals(tc, fix.u1, building_owner(b));
    CuAssertIntEquals(tc, 0, pay_cmd(fix.u1, ord));
    CuAssertIntEquals(tc, BLD_DONTPAY, b->flags&BLD_DONTPAY);
    free_order(ord);
    test_cleanup();
}

static void test_pay_cmd_must_be_owner(CuTest *tc) {
    struct pay_fixture fix;
    order *ord;
    faction *f;
    building *b;

    test_setup();
    setup_pay_cmd(&fix);
    b = fix.u1->building;
    f = fix.u1->faction;

    ord = create_order(K_PAY, f->locale, "NOT");
    assert(ord);
    CuAssertIntEquals(tc, 0, pay_cmd(fix.u2, ord));
    CuAssertIntEquals(tc, 0, b->flags&BLD_DONTPAY);
    free_order(ord);
    test_cleanup();
}

static void test_new_units(CuTest *tc) {
    unit *u;
    faction *f;
    region *r;
    const struct locale *loc;

    test_setup();
    f = test_create_faction(NULL);
    r = test_create_region(0, 0, NULL);
    assert(r && f);
    u = test_create_unit(f, r);
    assert(u && !u->next);
    loc = test_create_locale();
    assert(loc);
    u->orders = create_order(K_MAKETEMP, loc, "hurr");
    new_units();
    CuAssertPtrNotNull(tc, u = u->next);
    CuAssertIntEquals(tc, UFL_ISNEW, fval(u, UFL_ISNEW));
    CuAssertIntEquals(tc, 0, u->number);
    CuAssertIntEquals(tc, 0, u->age);
    CuAssertPtrEquals(tc, f, u->faction);
    CuAssertStrEquals(tc, "EINHEIT hurr", u->_name);
    test_cleanup();
}

typedef struct guard_fixture {
    unit * u;
} guard_fixture;

void setup_guard(guard_fixture *fix, bool armed) {
    region *r;
    faction *f;
    unit * u;

    test_setup();

    f = test_create_faction(NULL);
    r = test_create_region(0, 0, NULL);
    assert(r && f);
    u = test_create_unit(f, r);
    fset(u, UFL_GUARD);
    u->status = ST_FIGHT;

    if (armed) {
        item_type *itype;
        itype = it_get_or_create(rt_get_or_create("sword"));
        new_weapontype(itype, 0, frac_zero, NULL, 0, 0, 0, SK_MELEE, 2);
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
    config_set("NewbieImmunity", "4");
    CuAssertTrue(tc, IsImmune(fix.u->faction));
    CuAssertIntEquals(tc, E_GUARD_NEWBIE, can_start_guarding(fix.u));
    update_guards();
    CuAssertTrue(tc, !fval(fix.u, UFL_GUARD));
    test_cleanup();
}

static void test_unarmed_cannot_guard(CuTest *tc) {
    guard_fixture fix;

    setup_guard(&fix, false);
    CuAssertIntEquals(tc, E_GUARD_UNARMED, can_start_guarding(fix.u));
    update_guards();
    CuAssertTrue(tc, !fval(fix.u, UFL_GUARD));
    test_cleanup();
}

static void test_unarmed_races_can_guard(CuTest *tc) {
    guard_fixture fix;
    race * rc;

    setup_guard(&fix, false);
    rc = rc_get_or_create(fix.u->_race->_name);
    fset(rc, RCF_UNARMEDGUARD);
    CuAssertIntEquals(tc, E_GUARD_OK, can_start_guarding(fix.u));
    update_guards();
    CuAssertTrue(tc, fval(fix.u, UFL_GUARD));
    test_cleanup();
}

static void test_monsters_can_guard(CuTest *tc) {
    guard_fixture fix;

    setup_guard(&fix, false);
    u_setfaction(fix.u, get_or_create_monsters());
    CuAssertIntEquals(tc, E_GUARD_OK, can_start_guarding(fix.u));
    update_guards();
    CuAssertTrue(tc, fval(fix.u, UFL_GUARD));
    test_cleanup();
}

static void test_low_skill_cannot_guard(CuTest *tc) {
    guard_fixture fix;

    setup_guard(&fix, true);
    set_level(fix.u, SK_MELEE, 1);
    CuAssertIntEquals(tc, E_GUARD_UNARMED, can_start_guarding(fix.u));
    update_guards();
    CuAssertTrue(tc, !fval(fix.u, UFL_GUARD));
    test_cleanup();
}

static void test_fleeing_cannot_guard(CuTest *tc) {
    guard_fixture fix;

    setup_guard(&fix, true);
    fix.u->status = ST_FLEE;
    CuAssertIntEquals(tc, E_GUARD_FLEEING, can_start_guarding(fix.u));
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

    test_setup();
    init_resources();

    rtype = get_resourcetype(R_SILVER);
    assert(rtype && rtype->itype);
    f = test_create_faction(NULL);
    r = test_create_region(0, 0, 0);
    assert(r && f);
    u1 = test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    assert(u1 && u2);
    loc = test_create_locale();
    assert(loc);
    ord = create_order(K_RESERVE, loc, "200 SILBER");
    assert(ord);
    i_change(&u1->items, rtype->itype, 100);
    i_change(&u2->items, rtype->itype, 100);
    CuAssertIntEquals(tc, 100, reserve_self(u1, ord));
    CuAssertIntEquals(tc, 100, i_get(u1->items, rtype->itype));
    CuAssertIntEquals(tc, 100, i_get(u2->items, rtype->itype));
    free_order(ord);
    test_cleanup();
}

static void statistic_test(CuTest *tc, int peasants, int luck, int maxp,
    double variance, int min_value, int max_value) {
    int effect;

    effect = peasant_luck_effect(peasants, luck, maxp, variance);
    CuAssertTrue(tc, min_value <= effect);
    CuAssertTrue(tc, max_value >= effect);
}

static void test_peasant_luck_effect(CuTest *tc) {
    test_setup();

    config_set("rules.peasants.peasantluck.factor", "10");
    config_set("rules.peasants.growth.factor", "0.001");

    statistic_test(tc, 100, 0, 1000, 0, 0, 0);
    statistic_test(tc, 100, 2, 1000, 0, 1, 1);
    statistic_test(tc, 1000, 400, 1000, 0, 3, 3);
    statistic_test(tc, 1000, 1000, 2000, .5, 1, 501);

    config_set("rules.peasants.growth.factor", "1");
    statistic_test(tc, 1000, 1000, 1000, 0, 501, 501);
    test_cleanup();
}

/**
* Create any terrain types that are used by demographics.
*
* This should prevent newterrain from returning NULL.
*/
static void setup_terrains(CuTest *tc) {
    test_create_terrain("volcano", SEA_REGION | SWIM_INTO | FLY_INTO);
    test_create_terrain("activevolcano", LAND_REGION | WALK_INTO | FLY_INTO);
    init_terrains();
    CuAssertPtrNotNull(tc, newterrain(T_VOLCANO));
    CuAssertPtrNotNull(tc, newterrain(T_VOLCANO_SMOKING));
}

static void test_luck_message(CuTest *tc) {
    region* r;
    attrib *a;

    test_setup();
    setup_terrains(tc);
    r = test_create_region(0, 0, NULL);
    rsetpeasants(r, 1);

    demographics();

    CuAssertPtrEquals_Msg(tc, "unexpected message", (void *)NULL, r->msgs);

    a = (attrib *)a_find(r->attribs, &at_peasantluck);
    if (!a)
        a = a_add(&r->attribs, a_new(&at_peasantluck));
    a->data.i += 10;

    demographics();

    CuAssertPtrNotNull(tc, test_find_messagetype(r->msgs, "peasantluck_success"));

    test_cleanup();
}

static unit * setup_name_cmd(void) {
    faction *f;

    test_setup();
    f = test_create_faction(0);
    return test_create_unit(f, test_create_region(0, 0, 0));
}

static void test_name_unit(CuTest *tc) {
    unit *u;
    faction *f;
    order *ord;

    u = setup_name_cmd();
    f = u->faction;
    ord = create_order(K_NAME, f->locale, "%s Hodor", LOC(f->locale, parameters[P_UNIT]));
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", u->_name);
    free_order(ord);

    ord = create_order(K_NAME, f->locale, LOC(f->locale, parameters[P_UNIT]));
    name_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error84"));
    CuAssertStrEquals(tc, "Hodor", u->_name);
    free_order(ord);

    test_cleanup();
}

static void test_name_region(CuTest *tc) {
    unit *u;
    faction *f;
    order *ord;

    u = setup_name_cmd();
    f = u->faction;

    ord = create_order(K_NAME, f->locale, "%s Hodor", LOC(f->locale, parameters[P_REGION]));
    u_set_building(u, test_create_building(u->region, 0));
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", u->region->land->name);
    free_order(ord);

    ord = create_order(K_NAME, f->locale, LOC(f->locale, parameters[P_REGION]));
    name_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error84"));
    CuAssertStrEquals(tc, "Hodor", u->region->land->name);
    free_order(ord);

    test_cleanup();
}

static void test_name_building(CuTest *tc) {
    unit *u;
    faction *f;
    order *ord;

    u = setup_name_cmd();
    f = u->faction;

    ord = create_order(K_NAME, f->locale, "%s Hodor", LOC(f->locale, parameters[P_BUILDING]));
    name_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error145"));

    u->building = test_create_building(u->region, 0);
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", u->building->name);
    free_order(ord);

    ord = create_order(K_NAME, f->locale, LOC(f->locale, parameters[P_BUILDING]));
    name_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error84"));
    CuAssertStrEquals(tc, "Hodor", u->building->name);
    free_order(ord);

    /* TODO: test BTF_NAMECHANGE:
    btype->flags |= BTF_NAMECHANGE;
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error278"));
    test_clear_messages(u->faction);
    name_cmd(u, ord); */
    test_cleanup();
}

static void test_name_ship(CuTest *tc) {
    unit *uo, *u, *ux;
    faction *f;

    u = setup_name_cmd();
    u->ship = test_create_ship(u->region, 0);
    f = u->faction;
    uo = test_create_unit(test_create_faction(NULL), test_create_region(0, 0, NULL));
    u_set_ship(uo, u->ship);
    ux = test_create_unit(f, test_create_region(0, 0, NULL));
    u_set_ship(ux, u->ship);

    u->thisorder = create_order(K_NAME, f->locale, "%s Hodor", LOC(f->locale, parameters[P_SHIP]));

    ship_set_owner(uo);
    name_cmd(u, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error12"));
    test_clear_messages(f);

    ship_set_owner(u);
    name_cmd(u, u->thisorder);
    CuAssertStrEquals(tc, "Hodor", u->ship->name);

    ship_setname(u->ship, "Titanic");
    ship_set_owner(ux);
    name_cmd(u, u->thisorder);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "error12"));
    CuAssertStrEquals(tc, "Hodor", u->ship->name);

    test_clear_messages(f);
    free_order(u->thisorder);
    u->thisorder = create_order(K_NAME, f->locale, LOC(f->locale, parameters[P_SHIP]));
    name_cmd(u, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error84"));
    CuAssertStrEquals(tc, "Hodor", u->ship->name);

    test_cleanup();
}

static void test_long_order_normal(CuTest *tc) {
    /* TODO: write more tests */
    unit *u;
    order *ord;

    test_setup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    fset(u, UFL_MOVED);
    fset(u, UFL_LONGACTION);
    unit_addorder(u, ord = create_order(K_MOVE, u->faction->locale, 0));
    update_long_order(u);
    CuAssertPtrEquals(tc, ord->data, u->thisorder->data);
    CuAssertIntEquals(tc, 0, fval(u, UFL_MOVED));
    CuAssertIntEquals(tc, 0, fval(u, UFL_LONGACTION));
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrEquals(tc, 0, u->faction->msgs);
    CuAssertPtrEquals(tc, 0, u->old_orders);
    test_cleanup();
}

static void test_long_order_none(CuTest *tc) {
    /* TODO: write more tests */
    unit *u;
    test_setup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    update_long_order(u);
    CuAssertPtrEquals(tc, 0, u->thisorder);
    CuAssertPtrEquals(tc, 0, u->orders);
    CuAssertPtrEquals(tc, 0, u->faction->msgs);
    test_cleanup();
}

static void test_long_order_cast(CuTest *tc) {
    /* TODO: write more tests */
    unit *u;
    test_setup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    unit_addorder(u, create_order(K_CAST, u->faction->locale, 0));
    unit_addorder(u, create_order(K_CAST, u->faction->locale, 0));
    update_long_order(u);
    CuAssertPtrEquals(tc, 0, u->thisorder);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrEquals(tc, 0, u->faction->msgs);
    test_cleanup();
}

static void test_long_order_buy_sell(CuTest *tc) {
    /* TODO: write more tests */
    unit *u;
    test_setup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, 0));
    unit_addorder(u, create_order(K_SELL, u->faction->locale, 0));
    unit_addorder(u, create_order(K_SELL, u->faction->locale, 0));
    update_long_order(u);
    CuAssertPtrEquals(tc, 0, u->thisorder);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrEquals(tc, 0, u->faction->msgs);
    test_cleanup();
}

static void test_long_order_multi_long(CuTest *tc) {
    /* TODO: write more tests */
    unit *u;
    test_setup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    unit_addorder(u, create_order(K_MOVE, u->faction->locale, 0));
    unit_addorder(u, create_order(K_DESTROY, u->faction->locale, 0));
    update_long_order(u);
    CuAssertPtrNotNull(tc, u->thisorder);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error52"));
    test_cleanup();
}

static void test_long_order_multi_buy(CuTest *tc) {
    /* TODO: write more tests */
    unit *u;
    test_setup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, 0));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, 0));
    update_long_order(u);
    CuAssertPtrEquals(tc, 0, u->thisorder);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error52"));
    test_cleanup();
}

static void test_long_order_multi_sell(CuTest *tc) {
    /* TODO: write more tests */
    unit *u;
    test_setup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    unit_addorder(u, create_order(K_SELL, u->faction->locale, 0));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, 0));
    unit_addorder(u, create_order(K_SELL, u->faction->locale, 0));
    update_long_order(u);
    CuAssertPtrEquals(tc, 0, u->thisorder);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrEquals(tc, 0, u->faction->msgs);
    test_cleanup();
}

static void test_long_order_buy_cast(CuTest *tc) {
    /* TODO: write more tests */
    unit *u;
    test_setup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, 0));
    unit_addorder(u, create_order(K_CAST, u->faction->locale, 0));
    update_long_order(u);
    CuAssertPtrEquals(tc, 0, u->thisorder);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error52"));
    test_cleanup();
}

static void test_long_order_hungry(CuTest *tc) {
    unit *u;
    test_setup();
    config_set("hunger.long", "1");
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    fset(u, UFL_HUNGER);
    unit_addorder(u, create_order(K_MOVE, u->faction->locale, 0));
    unit_addorder(u, create_order(K_DESTROY, u->faction->locale, 0));
    config_set("orders.default", "work");
    update_long_order(u);
    CuAssertIntEquals(tc, K_WORK, getkeyword(u->thisorder));
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrEquals(tc, 0, u->faction->msgs);
    test_cleanup();
}

static void test_ally_cmd_errors(CuTest *tc) {
    unit *u;
    int fid;
    order *ord;

    test_setup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    fid = u->faction->no + 1;
    CuAssertPtrEquals(tc, 0, findfaction(fid));

    ord = create_order(K_ALLY, u->faction->locale, itoa36(fid));
    ally_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error66"));
    free_order(ord);

    test_cleanup();
}

static void test_name_cmd(CuTest *tc) {
    unit *u;
    faction *f;
    alliance *al;
    order *ord;

    test_setup();
    u = test_create_unit(f = test_create_faction(0), test_create_region(0, 0, 0));
    setalliance(f, al = makealliance(42, ""));

    ord = create_order(K_NAME, f->locale, "%s '  Ho\tdor  '", LOC(f->locale, parameters[P_UNIT]));
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", u->_name);
    free_order(ord);

    ord = create_order(K_NAME, f->locale, "%s '  Ho\tdor  '", LOC(f->locale, parameters[P_FACTION]));
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", f->name);
    free_order(ord);

    ord = create_order(K_NAME, f->locale, "%s '  Ho\tdor  '", LOC(f->locale, parameters[P_SHIP]));
    u->ship = test_create_ship(u->region, 0);
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", u->ship->name);
    free_order(ord);
    
    ord = create_order(K_NAME, f->locale, "%s '  Ho\tdor  '", LOC(f->locale, parameters[P_BUILDING]));
    u_set_building(u, test_create_building(u->region, 0));
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", u->building->name);
    free_order(ord);
    
    ord = create_order(K_NAME, f->locale, "%s '  Ho\tdor  '", LOC(f->locale, parameters[P_REGION]));
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", u->region->land->name);
    free_order(ord);

    ord = create_order(K_NAME, f->locale, "%s '  Ho\tdor  '", LOC(f->locale, parameters[P_ALLIANCE]));
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", al->name);
    free_order(ord);

    test_cleanup();
}

static void test_name_cmd_2274(CuTest *tc) {
    unit *u1, *u2, *u3;
    faction *f;
    region *r;

    test_setup();
    r = test_create_region(0, 0, 0);
    u1 = test_create_unit(test_create_faction(0), r);
    u2 = test_create_unit(test_create_faction(0), r);
    u3 = test_create_unit(u2->faction, r);
    u_set_building(u1, test_create_building(r, NULL));
    u1->building->size = 10;
    u_set_building(u2, test_create_building(r, NULL));
    u2->building->size = 20;

    f = u2->faction;
    u2->thisorder = create_order(K_NAME, f->locale, "%s Heimat", LOC(f->locale, parameters[P_REGION]));
    name_cmd(u2, u2->thisorder);
    CuAssertStrEquals(tc, "Heimat", r->land->name);
    f = u3->faction;
    u3->thisorder = create_order(K_NAME, f->locale, "%s Hodor", LOC(f->locale, parameters[P_REGION]));
    name_cmd(u3, u3->thisorder);
    CuAssertStrEquals(tc, "Hodor", r->land->name);
    f = u1->faction;
    u1->thisorder = create_order(K_NAME, f->locale, "%s notallowed", LOC(f->locale, parameters[P_REGION]));
    name_cmd(u1, u1->thisorder);
    CuAssertStrEquals(tc, "Hodor", r->land->name);

    test_cleanup();
}

static void test_ally_cmd(CuTest *tc) {
    unit *u;
    faction * f;
    order *ord;

    test_setup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    f = test_create_faction(0);

    ord = create_order(K_ALLY, f->locale, "%s", itoa36(f->no));
    ally_cmd(u, ord);
    CuAssertPtrEquals(tc, 0, u->faction->msgs);
    CuAssertIntEquals(tc, HELP_ALL, alliedfaction(0, u->faction, f, HELP_ALL));
    free_order(ord);

    ord = create_order(K_ALLY, f->locale, "%s %s", itoa36(f->no), LOC(f->locale, parameters[P_NOT]));
    ally_cmd(u, ord);
    CuAssertPtrEquals(tc, 0, u->faction->msgs);
    CuAssertIntEquals(tc, 0, alliedfaction(0, u->faction, f, HELP_ALL));
    free_order(ord);

    ord = create_order(K_ALLY, f->locale, "%s %s", itoa36(f->no), LOC(f->locale, parameters[P_GUARD]));
    ally_cmd(u, ord);
    CuAssertPtrEquals(tc, 0, u->faction->msgs);
    CuAssertIntEquals(tc, HELP_GUARD, alliedfaction(0, u->faction, f, HELP_ALL));
    free_order(ord);

    ord = create_order(K_ALLY, f->locale, "%s %s %s", itoa36(f->no), LOC(f->locale, parameters[P_GUARD]), LOC(f->locale, parameters[P_NOT]));
    ally_cmd(u, ord);
    CuAssertPtrEquals(tc, 0, u->faction->msgs);
    CuAssertIntEquals(tc, 0, alliedfaction(0, u->faction, f, HELP_ALL));
    free_order(ord);

    test_cleanup();
}

static void test_nmr_warnings(CuTest *tc) {
    faction *f1, *f2;
    test_setup();
    config_set("nmr.timeout", "3");
    f1 = test_create_faction(0);
    f2 = test_create_faction(0);
    f2->age = 2;
    f2->lastorders = 1;
    turn = 3;
    CuAssertIntEquals(tc, 0, f1->age);
    nmr_warnings();
    CuAssertPtrNotNull(tc, f1->msgs);
    CuAssertPtrNotNull(tc, test_find_messagetype(f1->msgs, "nmr_warning"));
    CuAssertPtrNotNull(tc, f2->msgs);
    CuAssertPtrNotNull(tc, f2->msgs->begin);
    CuAssertPtrNotNull(tc, test_find_messagetype(f2->msgs, "nmr_warning"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f2->msgs, "nmr_warning_final"));
    test_cleanup();
}

static unit * setup_mail_cmd(void) {
    faction *f;
    
    test_setup();
    f = test_create_faction(0);
    return test_create_unit(f, test_create_region(0, 0, 0));
}

static void test_mail_unit(CuTest *tc) {
    order *ord;
    unit *u;
    faction *f;
    
    u = setup_mail_cmd();
    f = u->faction;
    ord = create_order(K_MAIL, f->locale, "%s %s 'Hodor!'", LOC(f->locale, parameters[P_UNIT]), itoa36(u->no));
    mail_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "unitmessage"));
    free_order(ord);
    test_cleanup();
}

static void test_mail_faction(CuTest *tc) {
    order *ord;
    unit *u;
    faction *f;
    
    u = setup_mail_cmd();
    f = u->faction;
    ord = create_order(K_MAIL, f->locale, "%s %s 'Hodor!'", LOC(f->locale, parameters[P_FACTION]), itoa36(u->faction->no));
    mail_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "regionmessage"));
    free_order(ord);
    test_cleanup();
}

static void test_mail_region(CuTest *tc) {
    order *ord;
    unit *u;
    faction *f;
    
    u = setup_mail_cmd();
    f = u->faction;
    ord = create_order(K_MAIL, f->locale, "%s 'Hodor!'", LOC(f->locale, parameters[P_REGION]), itoa36(u->no));
    mail_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->region->msgs, "mail_result"));
    free_order(ord);
    test_cleanup();
}

static void test_mail_unit_no_msg(CuTest *tc) {
    unit *u;
    faction *f;
    order *ord;
    
    u = setup_mail_cmd();
    f = u->faction;
    ord = create_order(K_MAIL, f->locale, "%s %s", LOC(f->locale, parameters[P_UNIT]), itoa36(u->no));
    mail_cmd(u, ord);
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "unitmessage"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error30"));
    free_order(ord);
    test_cleanup();
}

static void test_mail_faction_no_msg(CuTest *tc) {
    unit *u;
    faction *f;
    order *ord;
    
    u = setup_mail_cmd();
    f = u->faction;
    ord = create_order(K_MAIL, f->locale, "%s %s", LOC(f->locale, parameters[P_FACTION]), itoa36(f->no));
    mail_cmd(u, ord);
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "regionmessage"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error30"));
    free_order(ord);
    test_cleanup();
}

static void test_mail_faction_no_target(CuTest *tc) {
    unit *u;
    faction *f;
    order *ord;
    
    u = setup_mail_cmd();
    f = u->faction;
    ord = create_order(K_MAIL, f->locale, "%s %s", LOC(f->locale, parameters[P_FACTION]), itoa36(f->no+1));
    mail_cmd(u, ord);
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "regionmessage"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error66"));
    free_order(ord);
    test_cleanup();
}

static void test_mail_region_no_msg(CuTest *tc) {
    unit *u;
    faction *f;
    order *ord;
    
    u = setup_mail_cmd();
    f = u->faction;
    ord = create_order(K_MAIL, f->locale, LOC(f->locale, parameters[P_REGION]));
    mail_cmd(u, ord);
    CuAssertPtrEquals(tc, 0, test_find_messagetype(u->region->msgs, "mail_result"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error30"));
    free_order(ord);
    test_cleanup();
}

static void test_show_without_item(CuTest *tc)
{
    region *r;
    faction *f;
    unit *u;
    order *ord;
    item_type *itype;
    struct locale *loc;

    test_setup();

    loc = get_or_create_locale("de");
    locale_setstring(loc, parameters[P_ANY], "ALLE");
    init_parameters(loc);

    r = test_create_region(0, 0, test_create_terrain("testregion", LAND_REGION));
    f = test_create_faction(test_create_race("human"));
    u = test_create_unit(f, r);

    itype = it_get_or_create(rt_get_or_create("testitem"));

    ord = create_order(K_RESHOW, f->locale, "testname");

    reshow_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error21"));
    test_clear_messages(f);

    locale_setstring(loc, "testitem", "testname");
    locale_setstring(loc, "iteminfo::testitem", "testdescription");

    reshow_cmd(u, ord);
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "error21"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error36"));
    test_clear_messages(f);

    i_add(&(u->items), i_new(itype, 1));
    reshow_cmd(u, ord);
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "error21"));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "error36"));
    test_clear_messages(f);

    free_order(ord);
    test_cleanup();
}

static void test_show_race(CuTest *tc) {
    order *ord;
    race * rc;
    unit *u;
    struct locale *loc;
    message * msg;

    test_setup();

    mt_register(mt_new_va("msg_event", "string:string", 0));
    test_create_race("human");
    rc = test_create_race("elf");

    loc = get_or_create_locale("de");
    locale_setstring(loc, "race::elf_p", "Elfen");
    locale_setstring(loc, "race::elf", "Elf");
    locale_setstring(loc, "race::human_p", "Menschen");
    locale_setstring(loc, "race::human", "Mensch");
    init_locale(loc);
    u = test_create_unit(test_create_faction(rc), test_create_region(0, 0, 0));
    u->faction->locale = loc;

    ord = create_order(K_RESHOW, loc, "Mensch");
    reshow_cmd(u, ord);
    CuAssertTrue(tc, test_find_messagetype(u->faction->msgs, "error21") != NULL);
    CuAssertTrue(tc, test_find_messagetype(u->faction->msgs, "msg_event") == NULL);
    test_clear_messages(u->faction);
    free_order(ord);

    ord = create_order(K_RESHOW, loc, "Elf");
    reshow_cmd(u, ord);
    CuAssertTrue(tc, test_find_messagetype(u->faction->msgs, "error21") == NULL);
    msg = test_find_messagetype(u->faction->msgs, "msg_event");
    CuAssertPtrNotNull(tc, msg);
    CuAssertTrue(tc, memcmp("Elf:", msg->parameters[0].v, 4) == 0);
    test_clear_messages(u->faction);
    free_order(ord);

    test_cleanup();
}

static void test_show_both(CuTest *tc) {
    order *ord;
    race * rc;
    unit *u;
    struct locale *loc;
    message * msg;

    test_cleanup();

    mt_register(mt_new_va("msg_event", "string:string", 0));
    mt_register(mt_new_va("displayitem", "weight:int", "item:resource", "description:string", 0));
    rc = test_create_race("elf");
    test_create_itemtype("elvenhorse");

    loc = get_or_create_locale("de");
    locale_setstring(loc, "elvenhorse", "Elfenpferd");
    locale_setstring(loc, "elvenhorse_p", "Elfenpferde");
    locale_setstring(loc, "iteminfo::elvenhorse", "Hiyaa!");
    locale_setstring(loc, "race::elf_p", "Elfen");
    locale_setstring(loc, "race::elf", "Elf");
    init_locale(loc);

    CuAssertPtrNotNull(tc, finditemtype("elf", loc));
    CuAssertPtrNotNull(tc, findrace("elf", loc));

    u = test_create_unit(test_create_faction(rc), test_create_region(0, 0, 0));
    u->faction->locale = loc;
    i_change(&u->items, finditemtype("elfenpferd", loc), 1);
    ord = create_order(K_RESHOW, loc, "Elf");
    reshow_cmd(u, ord);
    CuAssertTrue(tc, test_find_messagetype(u->faction->msgs, "error36") == NULL);
    msg = test_find_messagetype(u->faction->msgs, "msg_event");
    CuAssertPtrNotNull(tc, msg);
    CuAssertTrue(tc, memcmp("Elf:", msg->parameters[0].v, 4) == 0);
    msg = test_find_messagetype(u->faction->msgs, "displayitem");
    CuAssertPtrNotNull(tc, msg);
    CuAssertTrue(tc, memcmp("Hiyaa!", msg->parameters[2].v, 4) == 0);
    test_clear_messages(u->faction);
    free_order(ord);
    test_cleanup();
}

static void test_immigration(CuTest * tc)
{
    region *r;
    double inject[] = { 1 };

    test_setup();
    r = test_create_region(0, 0, 0);

    rsetpeasants(r, 0);
    config_set("rules.economy.repopulate_maximum", 0);

    random_source_inject_constant(0);

    immigration();
    CuAssertIntEquals(tc, 0, rpeasants(r));

    random_source_inject_constant(1);

    immigration();
    CuAssertIntEquals(tc, 2, rpeasants(r));

    random_source_inject_array(inject, 2);

    config_set("rules.wage.function", "0");
    immigration();
    CuAssertIntEquals(tc, 2, rpeasants(r));

    test_cleanup();
}

static void test_demon_hunger(CuTest * tc)
{
    const resource_type *rtype;
    region *r;
    race *rc;
    faction *f;
    unit *u;
    message* msg;

    test_setup();
    init_resources();
    r = test_create_region(0, 0, 0);
    rc = test_create_race("demon");
    f = test_create_faction(rc);
    u = test_create_unit(f, r);
    u->hp = 999;

    config_set("hunger.demons.peasant_tolerance", "1");

    rtype = get_resourcetype(R_SILVER);
    i_change(&u->items, rtype->itype, 30);
    scale_number(u, 1);
    rsetpeasants(r, 0);

    get_food(r);

    CuAssertIntEquals(tc, 20, i_get(u->items, rtype->itype));
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "malnourish"));

    config_set("hunger.demons.peasant_tolerance", "0");

    get_food(r);

    CuAssertIntEquals(tc, 10, i_get(u->items, rtype->itype));
    msg = test_get_last_message(u->faction->msgs);
    CuAssertStrEquals(tc, "malnourish", test_get_messagetype(msg));

    test_cleanup();
}

static void test_armedmen(CuTest *tc) {
    /* TODO: test RCF_NOWEAPONS and SK_WEAPONLESS */
    unit *u;
    item_type *it_sword;
    weapon_type *wtype;
    test_setup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    it_sword = test_create_itemtype("sword");
    wtype = new_weapontype(it_sword, 0, frac_make(1, 2), 0, 0, 0, 0, SK_MELEE, 1);
    CuAssertIntEquals(tc, 0, armedmen(u, false));
    CuAssertIntEquals(tc, 0, armedmen(u, true));
    set_level(u, SK_MELEE, 1);
    CuAssertIntEquals(tc, 0, armedmen(u, false));
    i_change(&u->items, it_sword, 1);
    CuAssertIntEquals(tc, 1, armedmen(u, false));
    i_change(&u->items, it_sword, 1);
    CuAssertIntEquals(tc, 1, armedmen(u, false));
    scale_number(u, 2);
    set_level(u, SK_MELEE, 1);
    CuAssertIntEquals(tc, 2, armedmen(u, false));
    set_level(u, SK_MELEE, 0);
    CuAssertIntEquals(tc, 0, armedmen(u, false));
    set_level(u, SK_MELEE, 1);
    i_change(&u->items, it_sword, -1);
    CuAssertIntEquals(tc, 1, armedmen(u, false));
    wtype->minskill = 2;
    CuAssertIntEquals(tc, 0, armedmen(u, false));
    set_level(u, SK_MELEE, 2);
    CuAssertIntEquals(tc, 1, armedmen(u, false));
    CuAssertIntEquals(tc, 1, armedmen(u, true));
    wtype->flags |= WTF_SIEGE;
    CuAssertIntEquals(tc, 0, armedmen(u, false));
    CuAssertIntEquals(tc, 1, armedmen(u, true));
    test_cleanup();
}

static void test_cansee(CuTest *tc) {
    unit *u, *u2;
    
    test_setup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    u2 = test_create_unit(test_create_faction(0), u->region);
    
    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 0));

    set_level(u2, SK_STEALTH, 1);
    CuAssertTrue(tc, !cansee(u->faction, u->region, u2, 0));
    
    set_level(u, SK_PERCEPTION, 1);
    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 0));
    
    test_cleanup();
}

static void test_cansee_spell(CuTest *tc) {
    unit *u2;
    faction *f;

    test_setup();
    f = test_create_faction(0);
    u2 = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));

    CuAssertTrue(tc, cansee_ex(f, u2->region, u2, 0, seen_spell));
    CuAssertTrue(tc, cansee_ex(f, u2->region, u2, 0, seen_battle));

    set_level(u2, SK_STEALTH, 1);
    CuAssertTrue(tc, !cansee_ex(f, u2->region, u2, 0, seen_spell));
    CuAssertTrue(tc, cansee_ex(f, u2->region, u2, 1, seen_spell));
    CuAssertTrue(tc, cansee_ex(f, u2->region, u2, 1, seen_battle));

    test_cleanup();
}

static void test_cansee_ring(CuTest *tc) {
    unit *u, *u2;
    item_type *itype[2];

    test_setup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    u2 = test_create_unit(test_create_faction(0), u->region);
    scale_number(u2, 2);

    itype[0] = test_create_itemtype("roi");
    itype[1] = test_create_itemtype("aots");
    CuAssertPtrNotNull(tc, get_resourcetype(R_RING_OF_INVISIBILITY));
    CuAssertPtrEquals(tc, itype[0]->rtype, (void *)get_resourcetype(R_RING_OF_INVISIBILITY));
    CuAssertPtrNotNull(tc, get_resourcetype(R_AMULET_OF_TRUE_SEEING));
    CuAssertPtrEquals(tc, itype[1]->rtype, (void *)get_resourcetype(R_AMULET_OF_TRUE_SEEING));

    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 0));

    /* a single ring is not enough to hide two people */
    i_change(&u2->items, itype[0], 1);
    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 0));

    /* two rings can hide two people */
    i_change(&u2->items, itype[0], 1);
    CuAssertTrue(tc, !cansee(u->faction, u->region, u2, 0));

    /* one amulet negates one of the two rings */
    i_change(&u->items, itype[1], 1);
    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 0));

    test_cleanup();
}

static void test_cansee_sphere(CuTest *tc) {
    unit *u, *u2;
    item_type *itype[2];

    test_setup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    u2 = test_create_unit(test_create_faction(0), u->region);

    itype[0] = test_create_itemtype("sphereofinv");
    itype[1] = test_create_itemtype("aots");
    CuAssertPtrNotNull(tc, get_resourcetype(R_SPHERE_OF_INVISIBILITY));
    CuAssertPtrEquals(tc, itype[0]->rtype, (void *)get_resourcetype(R_SPHERE_OF_INVISIBILITY));
    CuAssertPtrNotNull(tc, get_resourcetype(R_AMULET_OF_TRUE_SEEING));
    CuAssertPtrEquals(tc, itype[1]->rtype, (void *)get_resourcetype(R_AMULET_OF_TRUE_SEEING));

    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 0));

    /* a single sphere can hide 100 people */
    scale_number(u2, 100);
    i_change(&u2->items, itype[0], 1);
    CuAssertTrue(tc, !cansee(u->faction, u->region, u2, 0));

    /* one single amulet negates it? */
    i_change(&u->items, itype[1], 1);
    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 0));

    /* number of people inside the sphere does not matter? */
    scale_number(u2, 99);
    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 0));

    test_cleanup();
}

CuSuite *get_laws_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_maketemp_default_order);
    SUITE_ADD_TEST(suite, test_maketemp);
    SUITE_ADD_TEST(suite, test_nmr_warnings);
    SUITE_ADD_TEST(suite, test_ally_cmd);
    SUITE_ADD_TEST(suite, test_name_cmd);
    SUITE_ADD_TEST(suite, test_name_cmd_2274);
    SUITE_ADD_TEST(suite, test_name_unit);
    SUITE_ADD_TEST(suite, test_name_region);
    SUITE_ADD_TEST(suite, test_name_building);
    SUITE_ADD_TEST(suite, test_name_ship);
    SUITE_ADD_TEST(suite, test_ally_cmd_errors);
    SUITE_ADD_TEST(suite, test_long_order_normal);
    SUITE_ADD_TEST(suite, test_long_order_none);
    SUITE_ADD_TEST(suite, test_long_order_cast);
    SUITE_ADD_TEST(suite, test_long_order_buy_sell);
    SUITE_ADD_TEST(suite, test_long_order_multi_long);
    SUITE_ADD_TEST(suite, test_long_order_multi_buy);
    SUITE_ADD_TEST(suite, test_long_order_multi_sell);
    SUITE_ADD_TEST(suite, test_long_order_buy_cast);
    SUITE_ADD_TEST(suite, test_long_order_hungry);
    SUITE_ADD_TEST(suite, test_new_building_can_be_renamed);
    SUITE_ADD_TEST(suite, test_rename_building);
    SUITE_ADD_TEST(suite, test_rename_building_twice);
    SUITE_ADD_TEST(suite, test_fishing_feeds_2_people);
    SUITE_ADD_TEST(suite, test_fishing_does_not_give_goblins_money);
    SUITE_ADD_TEST(suite, test_fishing_gets_reset);
    SUITE_ADD_TEST(suite, test_unit_limit);
    SUITE_ADD_TEST(suite, test_limit_new_units);
    SUITE_ADD_TEST(suite, test_update_guards);
    SUITE_ADD_TEST(suite, test_newbie_cannot_guard);
    SUITE_ADD_TEST(suite, test_unarmed_cannot_guard);
    SUITE_ADD_TEST(suite, test_unarmed_races_can_guard);
    SUITE_ADD_TEST(suite, test_monsters_can_guard);
    SUITE_ADD_TEST(suite, test_fleeing_cannot_guard);
    SUITE_ADD_TEST(suite, test_low_skill_cannot_guard);
    SUITE_ADD_TEST(suite, test_reserve_self);
    SUITE_ADD_TEST(suite, test_reserve_cmd);
    SUITE_ADD_TEST(suite, test_pay_cmd);
    SUITE_ADD_TEST(suite, test_pay_cmd_other_building);
    SUITE_ADD_TEST(suite, test_pay_cmd_must_be_owner);
    SUITE_ADD_TEST(suite, test_new_units);
    SUITE_ADD_TEST(suite, test_cannot_create_unit_above_limit);
    SUITE_ADD_TEST(suite, test_contact);
    SUITE_ADD_TEST(suite, test_enter_building);
    SUITE_ADD_TEST(suite, test_enter_ship);
    SUITE_ADD_TEST(suite, test_display_cmd);
    SUITE_ADD_TEST(suite, test_rule_force_leave);
    SUITE_ADD_TEST(suite, test_force_leave_buildings);
    SUITE_ADD_TEST(suite, test_force_leave_ships);
    SUITE_ADD_TEST(suite, test_force_leave_ships_on_ocean);
    SUITE_ADD_TEST(suite, test_peasant_luck_effect);
    SUITE_ADD_TEST(suite, test_mail_unit);
    SUITE_ADD_TEST(suite, test_mail_faction);
    SUITE_ADD_TEST(suite, test_mail_region);
    SUITE_ADD_TEST(suite, test_mail_unit_no_msg);
    SUITE_ADD_TEST(suite, test_mail_faction_no_msg);
    SUITE_ADD_TEST(suite, test_mail_region_no_msg);
    SUITE_ADD_TEST(suite, test_mail_faction_no_target);
    SUITE_ADD_TEST(suite, test_luck_message);
    SUITE_ADD_TEST(suite, test_show_without_item);
    SUITE_ADD_TEST(suite, test_show_race);
    SUITE_ADD_TEST(suite, test_show_both);
    SUITE_ADD_TEST(suite, test_immigration);
    SUITE_ADD_TEST(suite, test_demon_hunger);
    SUITE_ADD_TEST(suite, test_armedmen);
    SUITE_ADD_TEST(suite, test_cansee);
    SUITE_ADD_TEST(suite, test_cansee_ring);
    SUITE_ADD_TEST(suite, test_cansee_sphere);
    SUITE_ADD_TEST(suite, test_cansee_spell);

    return suite;
}
