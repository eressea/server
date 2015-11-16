#include <platform.h>
#include "laws.h"
#include "battle.h"
#include "monster.h"

#include <kernel/ally.h>
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
    CuAssertTrue(tc, renamed_building(b));
}

static void test_rename_building_twice(CuTest * tc)
{
    region *r;
    unit *u;
    faction *f;
    building *b;
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

static void test_contact(CuTest * tc)
{
    region *r;
    unit *u1, *u2, *u3;
    building *b;
    building_type *btype;
    ally *al;

    test_cleanup();
    test_create_world();

    btype = bt_get_or_create("castle");
    r = findregion(0, 0);
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
}

static void test_enter_building(CuTest * tc)
{
    unit *u;
    region *r;
    building *b;
    race * rc;

    test_cleanup();
    test_create_world();

    r = findregion(0, 0);
    rc = rc_get_or_create("human");
    u = test_create_unit(test_create_faction(rc), r);
    b = test_create_building(r, bt_get_or_create("castle"));

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
    f->locale = get_or_create_locale("de");
    assert(r && f);
    test_translate_param(f->locale, P_UNIT, "EINHEIT");
    u = test_create_unit(f, r);
    assert(u);

    ord = create_order(K_DISPLAY, f->locale, "EINHEIT Hodor");
    CuAssertIntEquals(tc, 0, display_cmd(u, ord));
    CuAssertStrEquals(tc, "Hodor", u->display);
    free_order(ord);

    ord = create_order(K_DISPLAY, f->locale, "EINHEIT");
    CuAssertIntEquals(tc, 0, display_cmd(u, ord));
    CuAssertPtrEquals(tc, NULL, u->display);
    free_order(ord);

    test_cleanup();
}

static void test_rule_force_leave(CuTest *tc) {
    set_param(&global.parameters, "rules.owners.force_leave", "0");
    CuAssertIntEquals(tc, false, rule_force_leave(FORCE_LEAVE_ALL));
    CuAssertIntEquals(tc, false, rule_force_leave(FORCE_LEAVE_POSTCOMBAT));
    set_param(&global.parameters, "rules.owners.force_leave", "1");
    CuAssertIntEquals(tc, false, rule_force_leave(FORCE_LEAVE_ALL));
    CuAssertIntEquals(tc, true, rule_force_leave(FORCE_LEAVE_POSTCOMBAT));
    set_param(&global.parameters, "rules.owners.force_leave", "2");
    CuAssertIntEquals(tc, true, rule_force_leave(FORCE_LEAVE_ALL));
    CuAssertIntEquals(tc, false, rule_force_leave(FORCE_LEAVE_POSTCOMBAT));
    set_param(&global.parameters, "rules.owners.force_leave", "3");
    CuAssertIntEquals(tc, true, rule_force_leave(FORCE_LEAVE_ALL));
    CuAssertIntEquals(tc, true, rule_force_leave(FORCE_LEAVE_POSTCOMBAT));
}

static void test_force_leave_buildings(CuTest *tc) {
    ally *al;
    region *r;
    unit *u1, *u2, *u3;
    building * b;
    message *msg;
    test_cleanup();
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
    test_cleanup();
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
    test_cleanup();
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

static double level_taxes(const building * b, int level) {
    return b->size * level * 2.0;
}

static void setup_pay_cmd(struct pay_fixture *fix) {
    faction *f;
    region *r;
    building *b;
    building_type *btcastle;

    test_create_world();
    f = test_create_faction(NULL);
    r = findregion(0, 0);
    assert(r && f);
    btcastle = bt_get_or_create("castle");
    btcastle->taxes = level_taxes;
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

    test_cleanup();
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
    char cmd[32];

    test_cleanup();
    setup_pay_cmd(&fix);
    f = fix.u1->faction;
    b = test_create_building(fix.u1->region, bt_get_or_create("lighthouse"));
    set_param(&global.parameters, "rules.region_owners", "1");
    set_param(&global.parameters, "rules.region_owner_pay_building", "lighthouse");
    update_owners(b->region);

    _snprintf(cmd, sizeof(cmd), "NOT %s", itoa36(b->no));
    ord = create_order(K_PAY, f->locale, cmd);
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

    test_cleanup();
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

    test_cleanup();
    test_create_world();
    f = test_create_faction(NULL);
    r = findregion(0, 0);
    assert(r && f);
    u = test_create_unit(f, r);
    assert(u && !u->next);
    loc = get_locale("de");
    assert(loc);
    u->orders = create_order(K_MAKETEMP, loc, "hurr");
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
    rc = rc_get_or_create(fix.u->_race->_name);
    fset(rc, RCF_UNARMEDGUARD);
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
    test_cleanup();

    set_param(&global.parameters, "rules.peasants.peasantluck.factor", "10");
    set_param(&global.parameters, "rules.peasants.growth.factor", "0.001");

    statistic_test(tc, 100, 0, 1000, 0, 0, 0);
    statistic_test(tc, 100, 2, 1000, 0, 1, 1);
    statistic_test(tc, 1000, 400, 1000, 0, 3, 3);
    statistic_test(tc, 1000, 1000, 2000, .5, 1, 501);

    set_param(&global.parameters, "rules.peasants.growth.factor", "1");
    statistic_test(tc, 1000, 1000, 1000, 0, 501, 501);
    test_cleanup();
}

static void test_luck_message(CuTest *tc) {
    region* r;

    test_cleanup();
    r = test_create_region(0, 0, NULL);
    rsetpeasants(r, 1);

    demographics();

    CuAssertPtrEquals_Msg(tc, "unexpected message", (void *)NULL, r->msgs);

    attrib *a = (attrib *)a_find(r->attribs, &at_peasantluck);
    if (!a)
        a = a_add(&r->attribs, a_new(&at_peasantluck));
    a->data.i += 10;

    demographics();

    CuAssertPtrNotNull(tc, test_find_messagetype(r->msgs, "peasantluck_success"));

    test_cleanup();
}

static unit * setup_name_cmd(void) {
    faction *f;
    struct locale *lang;

    test_cleanup();
    f = test_create_faction(0);
    f->locale = lang = get_or_create_locale("en");
    locale_setstring(lang, parameters[P_UNIT], "UNIT");
    locale_setstring(lang, parameters[P_REGION], "REGION");
    locale_setstring(lang, parameters[P_FACTION], "FACTION");
    locale_setstring(lang, parameters[P_BUILDING], "BUILDING");
    locale_setstring(lang, parameters[P_SHIP], "SHIP");
    init_parameters(lang);
    return test_create_unit(f, test_create_region(0, 0, 0));
}

static void test_name_unit(CuTest *tc) {
    unit *u;
    order *ord;

    u = setup_name_cmd();

    ord = create_order(K_NAME, u->faction->locale, "UNIT Hodor");
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", u->_name);
    free_order(ord);

    ord = create_order(K_NAME, u->faction->locale, "UNIT");
    name_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error84"));
    CuAssertStrEquals(tc, "Hodor", u->_name);
    free_order(ord);

    test_cleanup();
}

static void test_name_region(CuTest *tc) {
    unit *u;
    order *ord;

    u = setup_name_cmd();

    ord = create_order(K_NAME, u->faction->locale, "REGION Hodor");
    name_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error145"));

    u->building = test_create_building(u->region, 0);
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", u->region->land->name);
    free_order(ord);

    ord = create_order(K_NAME, u->faction->locale, "REGION");
    name_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error84"));
    CuAssertStrEquals(tc, "Hodor", u->region->land->name);
    free_order(ord);

    test_cleanup();
}

static void test_name_building(CuTest *tc) {
    unit *u;
    order *ord;

    u = setup_name_cmd();

    ord = create_order(K_NAME, u->faction->locale, "BUILDING Hodor");
    name_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error145"));

    u->building = test_create_building(u->region, 0);
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", u->building->name);
    free_order(ord);

    ord = create_order(K_NAME, u->faction->locale, "BUILDING");
    name_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error84"));
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
    unit *u;
    order *ord;

    u = setup_name_cmd();
    u->ship = test_create_ship(u->region, 0);

    ord = create_order(K_NAME, u->faction->locale, "SHIP Hodor");
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", u->ship->name);
    free_order(ord);

    ord = create_order(K_NAME, u->faction->locale, "SHIP");
    name_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error84"));
    CuAssertStrEquals(tc, "Hodor", u->ship->name);
    free_order(ord);

    test_cleanup();
}

static void test_long_order_normal(CuTest *tc) {
    // TODO: write more tests
    unit *u;
    order *ord;

    test_cleanup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    fset(u, UFL_MOVED);
    fset(u, UFL_LONGACTION);
    u->faction->locale = get_or_create_locale("de");
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
    // TODO: write more tests
    unit *u;
    test_cleanup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    u->faction->locale = get_or_create_locale("de");
    update_long_order(u);
    CuAssertPtrEquals(tc, 0, u->thisorder);
    CuAssertPtrEquals(tc, 0, u->orders);
    CuAssertPtrEquals(tc, 0, u->faction->msgs);
    test_cleanup();
}

static void test_long_order_cast(CuTest *tc) {
    // TODO: write more tests
    unit *u;
    test_cleanup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    u->faction->locale = get_or_create_locale("de");
    unit_addorder(u, create_order(K_CAST, u->faction->locale, 0));
    unit_addorder(u, create_order(K_CAST, u->faction->locale, 0));
    update_long_order(u);
    CuAssertPtrEquals(tc, 0, u->thisorder);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrEquals(tc, 0, u->faction->msgs);
    test_cleanup();
}

static void test_long_order_buy_sell(CuTest *tc) {
    // TODO: write more tests
    unit *u;
    test_cleanup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    u->faction->locale = get_or_create_locale("de");
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
    // TODO: write more tests
    unit *u;
    test_cleanup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    u->faction->locale = get_or_create_locale("de");
    unit_addorder(u, create_order(K_MOVE, u->faction->locale, 0));
    unit_addorder(u, create_order(K_DESTROY, u->faction->locale, 0));
    update_long_order(u);
    CuAssertPtrNotNull(tc, u->thisorder);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error52"));
    test_cleanup();
}

static void test_long_order_multi_buy(CuTest *tc) {
    // TODO: write more tests
    unit *u;
    test_cleanup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    u->faction->locale = get_or_create_locale("de");
    unit_addorder(u, create_order(K_BUY, u->faction->locale, 0));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, 0));
    update_long_order(u);
    CuAssertPtrEquals(tc, 0, u->thisorder);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error52"));
    test_cleanup();
}

static void test_long_order_multi_sell(CuTest *tc) {
    // TODO: write more tests
    unit *u;
    test_cleanup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    u->faction->locale = get_or_create_locale("de");
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
    // TODO: write more tests
    unit *u;
    test_cleanup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    u->faction->locale = get_or_create_locale("de");
    unit_addorder(u, create_order(K_BUY, u->faction->locale, 0));
    unit_addorder(u, create_order(K_CAST, u->faction->locale, 0));
    update_long_order(u);
    CuAssertPtrEquals(tc, 0, u->thisorder);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error52"));
    test_cleanup();
}

static void test_long_order_hungry(CuTest *tc) {
    // FIXME: set_default_order is a test-only function, this is a bad test.
    // see also default_order
    unit *u;
    test_cleanup();
    set_param(&global.parameters, "hunger.long", "1");
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    fset(u, UFL_HUNGER);
    u->faction->locale = get_or_create_locale("de");
    unit_addorder(u, create_order(K_MOVE, u->faction->locale, 0));
    unit_addorder(u, create_order(K_DESTROY, u->faction->locale, 0));
    set_default_order(K_WORK);
    update_long_order(u);
    CuAssertIntEquals(tc, K_WORK, getkeyword(u->thisorder));
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrEquals(tc, 0, u->faction->msgs);
    set_default_order(NOKEYWORD);
    test_cleanup();
}

static void test_ally_cmd_errors(CuTest *tc) {
    unit *u;
    int fid;
    order *ord;

    test_cleanup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    u->faction->locale = get_or_create_locale("de");
    fid = u->faction->no + 1;
    CuAssertPtrEquals(tc, 0, findfaction(fid));

    ord = create_order(K_ALLY, u->faction->locale, itoa36(fid));
    ally_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error66"));
    free_order(ord);

    test_cleanup();
}

static void test_ally_cmd(CuTest *tc) {
    unit *u;
    faction * f;
    order *ord;
    struct locale *lang;

    test_cleanup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    f = test_create_faction(0);
    u->faction->locale = lang = get_or_create_locale("de");
    locale_setstring(lang, parameters[P_NOT], "NICHT");
    locale_setstring(lang, parameters[P_GUARD], "BEWACHE");
    init_parameters(lang);

    ord = create_order(K_ALLY, lang, "%s", itoa36(f->no));
    ally_cmd(u, ord);
    CuAssertPtrEquals(tc, 0, u->faction->msgs);
    CuAssertIntEquals(tc, HELP_ALL, alliedfaction(0, u->faction, f, HELP_ALL));
    free_order(ord);

    ord = create_order(K_ALLY, lang, "%s NICHT", itoa36(f->no));
    ally_cmd(u, ord);
    CuAssertPtrEquals(tc, 0, u->faction->msgs);
    CuAssertIntEquals(tc, 0, alliedfaction(0, u->faction, f, HELP_ALL));
    free_order(ord);

    ord = create_order(K_ALLY, lang, "%s BEWACHE", itoa36(f->no));
    ally_cmd(u, ord);
    CuAssertPtrEquals(tc, 0, u->faction->msgs);
    CuAssertIntEquals(tc, HELP_GUARD, alliedfaction(0, u->faction, f, HELP_ALL));
    free_order(ord);

    ord = create_order(K_ALLY, lang, "%s BEWACHE NICHT", itoa36(f->no));
    ally_cmd(u, ord);
    CuAssertPtrEquals(tc, 0, u->faction->msgs);
    CuAssertIntEquals(tc, 0, alliedfaction(0, u->faction, f, HELP_ALL));
    free_order(ord);

    test_cleanup();
}

static void test_nmr_warnings(CuTest *tc) {
    faction *f1, *f2;
    test_cleanup();
    set_param(&global.parameters, "nmr.timeout", "3");
    f1 = test_create_faction(0);
    f2 = test_create_faction(0);
    f2->age = 2;
    f2->lastorders = 1;
    turn = 3;
    CuAssertIntEquals(tc, 0, f1->age);
    nmr_warnings();
    CuAssertPtrNotNull(tc, f1->msgs);
    CuAssertPtrNotNull(tc, test_find_messagetype(f1->msgs, "changepasswd"));
    CuAssertPtrNotNull(tc, f2->msgs);
    CuAssertPtrNotNull(tc, test_find_messagetype(f2->msgs, "nmr_warning"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f2->msgs, "nmr_warning_final"));
    test_cleanup();
}

static unit * setup_mail_cmd(void) {
    faction *f;
    struct locale *lang;
    
    test_cleanup();
    f = test_create_faction(0);
    f->locale = lang = get_or_create_locale("de");
    locale_setstring(lang, parameters[P_UNIT], "EINHEIT");
    locale_setstring(lang, parameters[P_REGION], "REGION");
    locale_setstring(lang, parameters[P_FACTION], "PARTEI");
    init_parameters(lang);
    return test_create_unit(f, test_create_region(0, 0, 0));
}

static void test_mail_unit(CuTest *tc) {
    order *ord;
    unit *u;
    
    u = setup_mail_cmd();
    ord = create_order(K_MAIL, u->faction->locale, "EINHEIT %s 'Hodor!'", itoa36(u->no));
    mail_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "unitmessage"));
    free_order(ord);
    test_cleanup();
}

static void test_mail_faction(CuTest *tc) {
    order *ord;
    unit *u;
    
    u = setup_mail_cmd();
    ord = create_order(K_MAIL, u->faction->locale, "PARTEI %s 'Hodor!'", itoa36(u->faction->no));
    mail_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "regionmessage"));
    free_order(ord);
    test_cleanup();
}

static void test_mail_region(CuTest *tc) {
    order *ord;
    unit *u;
    
    u = setup_mail_cmd();
    ord = create_order(K_MAIL, u->faction->locale, "REGION 'Hodor!'", itoa36(u->no));
    mail_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->region->msgs, "mail_result"));
    free_order(ord);
    test_cleanup();
}

static void test_mail_unit_no_msg(CuTest *tc) {
    unit *u;
    order *ord;
    
    u = setup_mail_cmd();
    ord = create_order(K_MAIL, u->faction->locale, "EINHEIT %s", itoa36(u->no));
    mail_cmd(u, ord);
    CuAssertPtrEquals(tc, 0, test_find_messagetype(u->faction->msgs, "unitmessage"));
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error30"));
    free_order(ord);
    test_cleanup();
}

static void test_mail_faction_no_msg(CuTest *tc) {
    unit *u;
    order *ord;
    
    u = setup_mail_cmd();
    ord = create_order(K_MAIL, u->faction->locale, "PARTEI %s", itoa36(u->faction->no));
    mail_cmd(u, ord);
    CuAssertPtrEquals(tc, 0, test_find_messagetype(u->faction->msgs, "regionmessage"));
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error30"));
    free_order(ord);
    test_cleanup();
}

static void test_mail_faction_no_target(CuTest *tc) {
    unit *u;
    order *ord;
    
    u = setup_mail_cmd();
    ord = create_order(K_MAIL, u->faction->locale, "PARTEI %s", itoa36(u->faction->no+1));
    mail_cmd(u, ord);
    CuAssertPtrEquals(tc, 0, test_find_messagetype(u->faction->msgs, "regionmessage"));
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error66"));
    free_order(ord);
    test_cleanup();
}

static void test_mail_region_no_msg(CuTest *tc) {
    unit *u;
    order *ord;
    
    u = setup_mail_cmd();
    ord = create_order(K_MAIL, u->faction->locale, "REGION");
    mail_cmd(u, ord);
    CuAssertPtrEquals(tc, 0, test_find_messagetype(u->region->msgs, "mail_result"));
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error30"));
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
    item *i;
    struct locale *loc;

    test_cleanup();

    loc = get_or_create_locale("de");

    r = test_create_region(0, 0, test_create_terrain("testregion", LAND_REGION));
    f = test_create_faction(test_create_race("human"));
    u = test_create_unit(f, r);

    ord = create_order(K_RESHOW, u->faction->locale, "testname");

    itype = it_get_or_create(rt_get_or_create("testitem"));
    i = i_new(itype, 1);

    reshow_cmd(u, ord);
    CuAssertTrue(tc, test_find_messagetype(u->faction->msgs, "error21") != NULL);
    test_clear_messages(u->faction);

    locale_setstring(loc, "testitem", "testname");
    locale_setstring(loc, "iteminfo::testitem", "testdescription");
    reshow_cmd(u, ord);
    CuAssertTrue(tc, test_find_messagetype(u->faction->msgs, "error21") == NULL);
    CuAssertTrue(tc, test_find_messagetype(u->faction->msgs, "error36") != NULL);
    test_clear_messages(u->faction);

    i_add(&(u->items), i);
    reshow_cmd(u, ord);
    CuAssertTrue(tc, test_find_messagetype(u->faction->msgs, "error21") == NULL);
    CuAssertTrue(tc, test_find_messagetype(u->faction->msgs, "error36") == NULL);
    test_clear_messages(u->faction);

    free_order(ord);
    test_cleanup();
}

CuSuite *get_laws_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_nmr_warnings);
    SUITE_ADD_TEST(suite, test_ally_cmd);
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
    SUITE_ADD_TEST(suite, test_update_guards);
    SUITE_ADD_TEST(suite, test_newbie_cannot_guard);
    SUITE_ADD_TEST(suite, test_unarmed_cannot_guard);
    SUITE_ADD_TEST(suite, test_unarmed_races_can_guard);
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
    SUITE_ADD_TEST(suite, test_name_unit);
    SUITE_ADD_TEST(suite, test_name_region);
    SUITE_ADD_TEST(suite, test_name_building);
    SUITE_ADD_TEST(suite, test_name_ship);
    SUITE_ADD_TEST(suite, test_show_without_item);

    return suite;
}
