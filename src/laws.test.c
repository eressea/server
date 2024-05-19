#include "laws.h"
#include "battle.h"
#include "contact.h"
#include "eressea.h"
#include "guard.h"
#include "magic.h"                   // for create_mage

#include <kernel/ally.h>
#include <kernel/alliance.h>
#include <kernel/attrib.h>
#include <kernel/calendar.h>
#include <kernel/config.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/group.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include "kernel/skill.h"            // for SK_MELEE, SK_MAGIC, SK_STEALTH
#include "kernel/status.h"           // for ST_FIGHT, ST_FLEE
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include "kernel/types.h"            // for M_GWYRRD, M_DRAIG
#include <kernel/unit.h>

#include <util/base36.h>
#include <util/keyword.h>      // for K_NAME, K_QUIT, K_MAKETEMP, K_EXPEL
#include <util/language.h>
#include <util/message.h>
#include <util/param.h>
#include <util/rand.h>
#include <util/variant.h>  // for variant, frac_make, frac_zero

#include <CuTest.h>
#include <tests.h>

#include <assert.h>
#include <stdbool.h>                 // for false, true, bool
#include <stdio.h>
#include <string.h>

static void test_new_building_can_be_renamed(CuTest * tc)
{
    region *r;
    building *b;

    test_setup();
    test_create_locale();
    r = test_create_plain(0, 0);

    b = test_create_building(r, NULL);
    CuAssertTrue(tc, !renamed_building(b));
    test_teardown();
}

static void rename_building(unit* u, building* b, const char* name)
{
    const struct locale* lang = u->faction->locale;
    order* ord = create_order(K_NAME, lang, "%s \"%s\"", param_name(P_BUILDING, lang), name);
    name_cmd(u, ord);
    free_order(ord);
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
    btype = test_create_castle();
    r = test_create_plain(0, 0);
    b = new_building(btype, r, default_locale, 1);
    f = test_create_faction();
    u = test_create_unit(f, r);
    u_set_building(u, b);

    rename_building(u, b, "Villa Nagel");
    CuAssertStrEquals(tc, "Villa Nagel", b->name);
    CuAssertTrue(tc, renamed_building(b));
    test_teardown();
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
    btype = test_create_castle();
    r = test_create_plain(0, 0);
    b = new_building(btype, r, default_locale, 1);
    f = test_create_faction();
    u = test_create_unit(f, r);
    u_set_building(u, b);

    rename_building(u, b, "Villa Nagel");
    CuAssertStrEquals(tc, "Villa Nagel", b->name);

    rename_building(u, b, "Villa Kunterbunt");
    CuAssertStrEquals(tc, "Villa Kunterbunt", b->name);
    test_teardown();
}

static void test_enter_building(CuTest * tc)
{
    unit *u;
    region *r;
    building *b;
    race * rc;

    test_setup();
    test_create_locale();
    r = test_create_plain(0, 0);
    rc = test_create_race("human");
    u = test_create_unit(test_create_faction_ex(rc, NULL), r);
    b = test_create_building(r, test_create_castle());

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
    CuAssertPtrEquals(tc, NULL, u->building);
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);

    CuAssertIntEquals(tc, 0, enter_building(u, NULL, b->no, true));
    CuAssertPtrNotNull(tc, u->faction->msgs);

    test_teardown();
}

static void test_enter_ship(CuTest * tc)
{
    unit *u;
    region *r;
    ship *sh;
    race * rc;

    test_setup();
    r = test_create_plain(0, 0);
    rc = test_create_race("smurf");
    u = test_create_unit(test_create_faction_ex(rc, NULL), r);
    sh = test_create_ship(r, NULL);

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
    CuAssertPtrEquals(tc, NULL, u->ship);
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);

    CuAssertIntEquals(tc, 0, enter_ship(u, NULL, sh->no, true));
    CuAssertPtrNotNull(tc, u->faction->msgs);

    test_teardown();
}

static void test_expel_building(CuTest *tc) {
    unit *u1, *u2;
    order *ord;
    building *b;

    test_setup();
    u1 = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u2 = test_create_unit(test_create_faction(), u1->region);
    ord = create_order(K_EXPEL, u2->faction->locale, itoa36(u1->no));

    test_clear_messages(u2->faction);
    expel_cmd(u2, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u2->faction->msgs, "feedback_not_inside"));
    test_clear_messages(u2->faction);

    b = u2->building = u1->building = test_create_building(u1->region, NULL);
    CuAssertPtrEquals(tc, u1, building_owner(b));

    test_clear_messages(u2->faction);
    expel_cmd(u2, ord);
    /* Nothing happened: */
    CuAssertPtrNotNull(tc, test_find_messagetype(u2->faction->msgs, "error5"));
    CuAssertPtrEquals(tc, u1, building_owner(b));
    CuAssertPtrEquals(tc, b, u1->building);
    CuAssertPtrEquals(tc, b, u2->building);
    free_order(ord);

    test_clear_messages(u1->faction);
    test_clear_messages(u2->faction);
    ord = create_order(K_EXPEL, u1->faction->locale, itoa36(u2->no));
    expel_cmd(u1, ord);
    /* owner has expelled u2: */
    CuAssertPtrNotNull(tc, test_find_messagetype(u1->faction->msgs, "force_leave_building"));
    CuAssertPtrNotNull(tc, test_find_messagetype(u2->faction->msgs, "force_leave_building"));
    CuAssertPtrEquals(tc, NULL, u2->building);
    free_order(ord);

    /* unit in another region, not found */
    u2 = test_create_unit(u2->faction, test_create_plain(0, 1));
    ord = create_order(K_EXPEL, u1->faction->locale, itoa36(u2->no));
    expel_cmd(u1, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u1->faction->msgs, "feedback_unit_not_found"));

    test_teardown();
}

static void test_expel_ship(CuTest *tc) {
    unit *u1, *u2;
    order *ord;
    ship *sh;

    test_setup();
    u1 = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u2 = test_create_unit(test_create_faction(), u1->region);
    ord = create_order(K_EXPEL, u2->faction->locale, itoa36(u1->no));
    test_clear_messages(u2->faction);
    expel_cmd(u2, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u2->faction->msgs, "feedback_not_inside"));
    test_clear_messages(u2->faction);

    sh = u2->ship = u1->ship = test_create_ship(u1->region, NULL);
    CuAssertPtrEquals(tc, u1, ship_owner(sh));

    test_clear_messages(u2->faction);
    expel_cmd(u2, ord);
    /* Nothing happened: */
    CuAssertPtrEquals(tc, u1, ship_owner(sh));
    CuAssertPtrEquals(tc, sh, u1->ship);
    CuAssertPtrEquals(tc, sh, u2->ship);
    CuAssertPtrNotNull(tc, test_find_messagetype(u2->faction->msgs, "error146"));
    test_clear_messages(u2->faction);
    free_order(ord);

    ord = create_order(K_EXPEL, u1->faction->locale, itoa36(u2->no));
    test_clear_messages(u1->faction);
    test_clear_messages(u2->faction);
    expel_cmd(u1, ord);
    /* owner has expelled u2: */
    CuAssertPtrEquals(tc, NULL, u2->ship);
    CuAssertPtrEquals(tc, sh, leftship(u2));
    CuAssertPtrNotNull(tc, test_find_messagetype(u1->faction->msgs, "force_leave_ship"));
    CuAssertPtrNotNull(tc, test_find_messagetype(u2->faction->msgs, "force_leave_ship"));
    test_clear_messages(u1->faction);
    free_order(ord);

    /* unit in another region, not found */
    u2 = test_create_unit(u2->faction, test_create_plain(0, 1));
    ord = create_order(K_EXPEL, u1->faction->locale, itoa36(u2->no));
    test_clear_messages(u1->faction);
    expel_cmd(u1, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u1->faction->msgs, "feedback_unit_not_found"));

    test_teardown();
}

static void test_expel_ship_at_sea(CuTest *tc) {
    unit *u1, *u2;
    order *ord;
    ship *sh;

    test_setup();
    u1 = test_create_unit(test_create_faction(), test_create_ocean(0, 0));
    u2 = test_create_unit(test_create_faction(), u1->region);
    sh = u2->ship = u1->ship = test_create_ship(u1->region, NULL);
    CuAssertPtrEquals(tc, u1, ship_owner(sh));

    ord = create_order(K_EXPEL, u1->faction->locale, itoa36(u2->no));
    expel_cmd(u1, ord);
    /* owner has not expelled u2: */
    CuAssertPtrNotNull(tc, test_find_messagetype(u1->faction->msgs, "error_onlandonly"));
    CuAssertPtrEquals(tc, sh, u2->ship);
    test_teardown();
}

static void test_display_cmd(CuTest *tc) {
    unit *u;
    faction *f;
    region *r;
    order *ord;

    test_setup();
    r = test_create_region(0, 0, test_create_terrain("plain", LAND_REGION));
    f = test_create_faction();
    assert(r && f);
    u = test_create_unit(f, r);
    assert(u);

    ord = create_order(K_DISPLAY, f->locale, "%s Hodor", param_name(P_UNIT, f->locale));
    CuAssertIntEquals(tc, 0, display_cmd(u, ord));
    CuAssertStrEquals(tc, "Hodor", unit_getinfo(u));
    free_order(ord);

    ord = create_order(K_DISPLAY, f->locale, "%s ' Klabautermann '", param_name(P_UNIT, f->locale));
    CuAssertIntEquals(tc, 0, display_cmd(u, ord));
    CuAssertStrEquals(tc, "Klabautermann", unit_getinfo(u));
    free_order(ord);

    ord = create_order(K_DISPLAY, f->locale, "%s Hodor", param_name(P_PRIVAT, f->locale));
    CuAssertIntEquals(tc, 0, display_cmd(u, ord));
    CuAssertStrEquals(tc, "Hodor", uprivate(u));
    free_order(ord);

    ord = create_order(K_DISPLAY, f->locale, "%s ' Klabautermann '", param_name(P_PRIVAT, f->locale));
    CuAssertIntEquals(tc, 0, display_cmd(u, ord));
    CuAssertStrEquals(tc, "Klabautermann", uprivate(u));
    free_order(ord);

    ord = create_order(K_DISPLAY, f->locale, param_name(P_UNIT, f->locale));
    CuAssertIntEquals(tc, 0, display_cmd(u, ord));
    CuAssertPtrEquals(tc, NULL, (void *)unit_getinfo(u));
    free_order(ord);

    ord = create_order(K_DISPLAY, f->locale, "%s Hodor", param_name(P_REGION, f->locale));
    CuAssertIntEquals(tc, 0, display_cmd(u, ord));
    CuAssertPtrEquals(tc, NULL, r->land->display);
    free_order(ord);

    test_teardown();
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
    test_teardown();
}

static void test_force_leave_buildings(CuTest *tc) {
    region *r;
    unit *u1, *u2, *u3;
    building * b;

    test_setup();
    mt_create_va(mt_new("force_leave_building", NULL), "unit:unit", "owner:unit", "building:building", MT_NEW_END);
    r = test_create_region(0, 0, test_create_terrain("plain", LAND_REGION));
    u1 = test_create_unit(test_create_faction(), r);
    u2 = test_create_unit(u1->faction, r);
    u3 = test_create_unit(test_create_faction(), r);
    b = test_create_building(r, NULL);
    u_set_building(u1, b);
    building_set_owner(u1);
    u_set_building(u2, b);
    u_set_building(u3, b);
    force_leave(r, NULL);
    CuAssertPtrEquals_Msg(tc, "owner should not be forced to leave", b, u1->building);
    CuAssertPtrEquals_Msg(tc, "same faction should not be forced to leave", b, u2->building);
    CuAssertPtrEquals_Msg(tc, "non-allies should be forced to leave", NULL, u3->building);
    CuAssertPtrNotNull(tc, test_find_messagetype(u3->faction->msgs, "force_leave_building"));

    u_set_building(u3, b);
    ally_set(&u1->faction->allies, u3->faction, HELP_GUARD);
    force_leave(r, NULL);
    CuAssertPtrEquals_Msg(tc, "allies should not be forced to leave", b, u3->building);
    test_teardown();
}

static void test_force_leave_ships(CuTest *tc) {
    region *r;
    unit *u1, *u2;
    ship *sh;

    test_setup();
    mt_create_va(mt_new("force_leave_ship", NULL), "unit:unit", "owner:unit", "ship:ship", MT_NEW_END);
    r = test_create_region(0, 0, test_create_terrain("plain", LAND_REGION));
    u1 = test_create_unit(test_create_faction(), r);
    u2 = test_create_unit(test_create_faction(), r);
    sh = test_create_ship(r, NULL);
    u_set_ship(u1, sh);
    u_set_ship(u2, sh);
    ship_set_owner(u1);
    force_leave(r, NULL);
    CuAssertPtrEquals_Msg(tc, "non-allies should be forced to leave", NULL, u2->ship);
    CuAssertPtrNotNull(tc, test_find_messagetype(u2->faction->msgs, "force_leave_ship"));
    test_teardown();
}

static void test_force_leave_ships_on_ocean(CuTest *tc) {
    region *r;
    unit *u1, *u2;
    ship *sh;

    test_setup();
    r = test_create_ocean(0, 0);
    u1 = test_create_unit(test_create_faction(), r);
    u2 = test_create_unit(test_create_faction(), r);
    sh = test_create_ship(r, NULL);
    u_set_ship(u1, sh);
    u_set_ship(u2, sh);
    ship_set_owner(u1);
    force_leave(r, NULL);
    CuAssertPtrEquals_Msg(tc, "no forcing out of ships on oceans", sh, u2->ship);
    test_teardown();
}

static void test_fishing_feeds_2_people(CuTest * tc)
{
    const item_type *itype;
    region *r;
    faction *f;
    unit *u;
    ship *sh;

    test_setup();
    r = test_create_ocean(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    sh = test_create_ship(r, NULL);
    u_set_ship(u, sh);
    itype = test_create_silver();
    i_change(&u->items, itype, 42);

    scale_number(u, 1);
    sh->flags |= SF_FISHING;
    get_food(r);
    CuAssertIntEquals(tc, 42, i_get(u->items, itype));

    scale_number(u, 2);
    sh->flags |= SF_FISHING;
    get_food(r);
    CuAssertIntEquals(tc, 42, i_get(u->items, itype));

    scale_number(u, 3);
    sh->flags |= SF_FISHING;
    get_food(r);
    CuAssertIntEquals(tc, 32, i_get(u->items, itype));
    test_teardown();
}

static void test_fishing_does_not_give_goblins_money(CuTest * tc)
{
    const item_type *itype;
    region *r;
    faction *f;
    unit *u;
    ship *sh;

    test_setup();
    itype = test_create_silver();

    r = test_create_ocean(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    sh = test_create_ship(r, NULL);
    u_set_ship(u, sh);
    i_change(&u->items, itype, 42);

    scale_number(u, 2);
    sh->flags |= SF_FISHING;
    get_food(r);
    CuAssertIntEquals(tc, 42, i_get(u->items, itype));
    test_teardown();
}

static void test_fishing_gets_reset(CuTest * tc)
{
    const item_type *itype;
    region *r;
    faction *f;
    unit *u;
    ship *sh;

    test_setup();
    itype = test_create_silver();

    r = test_create_ocean(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    sh = test_create_ship(r, NULL);
    u_set_ship(u, sh);
    i_change(&u->items, itype, 42);

    scale_number(u, 1);
    sh->flags |= SF_FISHING;
    get_food(r);
    CuAssertIntEquals(tc, 42, i_get(u->items, itype));

    scale_number(u, 1);
    get_food(r);
    CuAssertIntEquals(tc, 32, i_get(u->items, itype));
    test_teardown();
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
    test_teardown();
}

static void test_findparam_ex(CuTest *tc)
{
    struct locale *lang;

    test_setup();
    lang = test_create_locale();
    locale_setstring(lang, "temple", "TEMPEL");
    test_create_buildingtype("temple");

    CuAssertIntEquals(tc, P_GEBAEUDE, findparam_ex("TEMPEL", lang));
    CuAssertIntEquals(tc, P_GEBAEUDE, findparam_ex(
        param_name(P_BUILDING, lang), lang));
    CuAssertIntEquals(tc, P_SHIP, findparam_ex(
        param_name(P_SHIP, lang), lang));
    CuAssertIntEquals(tc, P_FACTION, findparam_ex(
        param_name(P_FACTION, lang), lang));
    CuAssertIntEquals(tc, P_UNIT, findparam_ex(
        param_name(P_UNIT, lang), lang));
    test_teardown();
}

static void test_maketemp(CuTest * tc)
{
    faction *f;
    unit *u, *u2;

    test_setup();
    f = test_create_faction();
    u = test_create_unit(f, test_create_plain(0, 0));

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
    CuAssertPtrEquals(tc, NULL, u2->thisorder);

    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrEquals(tc, NULL, u->orders->next);
    CuAssertIntEquals(tc, K_TAX, getkeyword(u->orders));
    test_teardown();
}

static void test_maketemp_default_order(CuTest * tc)
{
    faction *f;
    unit *u, *u2;

    test_setup();
    config_set("orders.default", "work");
    f = test_create_faction();
    u = test_create_unit(f, test_create_plain(0, 0));

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
    test_teardown();
}

static void test_limit_new_units(CuTest * tc)
{
    faction *f;
    unit *u;
    alliance *al;

    test_setup();
    mt_create_va(mt_new("too_many_units_in_faction", NULL), "unit:unit",
        "region:region", "command:order", "allowed:int", MT_NEW_END);
    mt_create_va(mt_new("too_many_units_in_alliance", NULL), "unit:unit",
        "region:region", "command:order", "allowed:int", MT_NEW_END);
    al = makealliance(1, "Hodor");
    f = test_create_faction();
    u = test_create_unit(f, test_create_plain(0, 0));
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
    u = test_create_unit(test_create_faction(), u->region);
    setalliance(u->faction, al);

    CuAssertPtrEquals(tc, NULL, u->orders);
    u->orders = create_order(K_MAKETEMP, f->locale, "1");
    new_units();
    CuAssertIntEquals(tc, 2, f->num_units);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "too_many_units_in_alliance"));

    test_teardown();
}

static void test_cannot_create_unit_above_limit(CuTest * tc)
{
    faction *f;

    test_setup();
    f = test_create_faction();
    config_set("rules.limit.faction", "4");

    CuAssertIntEquals(tc, 0, checkunitnumber(f, 4));
    CuAssertIntEquals(tc, 2, checkunitnumber(f, 5));

    config_set("rules.limit.alliance", "3");
    CuAssertIntEquals(tc, 0, checkunitnumber(f, 3));
    CuAssertIntEquals(tc, 1, checkunitnumber(f, 4));
    test_teardown();
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

    test_setup();
    f = test_create_faction();
    r = test_create_plain(0, 0);
    btcastle = test_create_castle();
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

static void test_promotion_cmd(CuTest* tc)
{
    unit* u, *u2;
    const item_type* itype;

    test_setup();
    mt_create_va(mt_new("heroes_maxed", NULL),
        "unit:unit", "region:region", "command:order", 
        "max:int", "count:int", MT_NEW_END);
    mt_create_va(mt_new("heroes_cost", NULL), 
        "unit:unit", "region:region", "command:order",
        "cost:int", "have:int", MT_NEW_END);
    mt_create_va(mt_new("hero_promotion", NULL), "unit:unit", "cost:int", MT_NEW_END);
    itype = test_create_silver();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u->thisorder = create_order(K_PROMOTION, u->faction->locale, NULL);
    u2 = test_create_unit(u->faction, u->region);

    /* not enough people to allow promotion: */
    i_change(&u->items, itype, 57);
    scale_number(u2, 55);
    CuAssertIntEquals(tc, 56, u->faction->num_people);
    promotion_cmd(u, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "heroes_maxed"));
    CuAssertIntEquals(tc, 0, u->flags & UFL_HERO);
    CuAssertIntEquals(tc, 57, i_get(u->items, itype));
    test_clear_messages(u->faction);

    /* enough people and money for promotion: */
    scale_number(u2, 56);
    CuAssertIntEquals(tc, 57, u->faction->num_people);
    promotion_cmd(u, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "hero_promotion"));
    CuAssertIntEquals(tc, UFL_HERO, u->flags & UFL_HERO);
    CuAssertIntEquals(tc, 0, i_get(u->items, itype));
    u->flags -= UFL_HERO;
    test_clear_messages(u->faction);

    /* not enough money (in pool) for promotion: */
    i_change(&u2->items, itype, 56);
    promotion_cmd(u, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "heroes_cost"));
    CuAssertIntEquals(tc, 0, u->flags & UFL_HERO);
    CuAssertIntEquals(tc, 56, i_get(u2->items, itype));
    test_clear_messages(u->faction);

    /* enough people and money (using pool) for promotion: */
    i_change(&u->items, itype, 1);
    promotion_cmd(u, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "hero_promotion"));
    CuAssertIntEquals(tc, UFL_HERO, u->flags & UFL_HERO);
    CuAssertIntEquals(tc, 0, i_get(u->items, itype));
    CuAssertIntEquals(tc, 0, i_get(u2->items, itype));

    /* cannot promote a unit twice: */
    i_change(&u2->items, itype, 57);
    promotion_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, 57, i_get(u2->items, itype));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(u->faction->msgs, "heroes_cost"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(u->faction->msgs, "heroes_promotion"));
    test_clear_messages(u->faction);
    u->flags -= UFL_HERO;
    test_clear_messages(u->faction);

    /* not using reserved money from pool for promotion: */
    set_resvalue(u2, itype, 1); /* RESERVIERE 1 Silber */
    promotion_cmd(u, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "heroes_cost"));
    CuAssertIntEquals(tc, 0, u->flags & UFL_HERO);
    CuAssertIntEquals(tc, 57, i_get(u2->items, itype));
    test_clear_messages(u->faction);

    test_teardown();
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
    test_teardown();
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
    update_region_owners(b->region);

    ord = create_order(K_PAY, f->locale, "NOT %s", itoa36(b->no));
    assert(ord);
    CuAssertPtrEquals(tc, fix.u1, building_owner(b));
    CuAssertIntEquals(tc, 0, pay_cmd(fix.u1, ord));
    CuAssertIntEquals(tc, BLD_DONTPAY, b->flags&BLD_DONTPAY);
    free_order(ord);
    test_teardown();
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
    test_teardown();
}

static void test_new_units(CuTest *tc) {
    unit *u;
    faction *f;
    region *r;
    const struct locale *loc;

    test_setup();
    f = test_create_faction();
    r = test_create_plain(0, 0);
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
    test_teardown();
}

typedef struct guard_fixture {
    unit * u;
} guard_fixture;

void setup_guard(guard_fixture *fix, bool armed) {
    region *r;
    faction *f;
    unit * u;

    test_setup();

    f = test_create_faction();
    r = test_create_plain(0, 0);
    assert(r && f);
    u = test_create_unit(f, r);
    fset(u, UFL_GUARD);
    u->status = ST_FIGHT;

    if (armed) {
        item_type *itype;
        itype = it_get_or_create(rt_get_or_create("sword"));
        new_weapontype(itype, 0, frac_zero, NULL, 0, 0, 0, SK_MELEE);
        i_change(&u->items, itype, 1);
        set_level(u, SK_MELEE, 1);
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
    test_teardown();
}

static void test_newbie_cannot_guard(CuTest *tc) {
    guard_fixture fix;

    setup_guard(&fix, true);
    config_set("NewbieImmunity", "4");
    faction_set_age(fix.u->faction, 3);
    CuAssertIntEquals(tc, E_GUARD_OK, can_start_guarding(fix.u));
    faction_set_age(fix.u->faction, 2);
    CuAssertIntEquals(tc, E_GUARD_NEWBIE, can_start_guarding(fix.u));
    update_guards();
    CuAssertTrue(tc, !fval(fix.u, UFL_GUARD));
    test_teardown();
}

static void test_unarmed_cannot_guard(CuTest *tc) {
    guard_fixture fix;

    setup_guard(&fix, false);
    CuAssertIntEquals(tc, E_GUARD_UNARMED, can_start_guarding(fix.u));
    update_guards();
    CuAssertTrue(tc, !fval(fix.u, UFL_GUARD));
    test_teardown();
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
    test_teardown();
}

static void test_monsters_can_guard(CuTest *tc) {
    guard_fixture fix;

    setup_guard(&fix, false);
    u_setfaction(fix.u, get_or_create_monsters());
    CuAssertIntEquals(tc, E_GUARD_OK, can_start_guarding(fix.u));
    update_guards();
    CuAssertTrue(tc, fval(fix.u, UFL_GUARD));
    test_teardown();
}

static void test_unskilled_cannot_guard(CuTest *tc) {
    guard_fixture fix;

    setup_guard(&fix, true);
    set_level(fix.u, SK_MELEE, 0);
    CuAssertIntEquals(tc, E_GUARD_UNARMED, can_start_guarding(fix.u));
    update_guards();
    CuAssertTrue(tc, !fval(fix.u, UFL_GUARD));
    test_teardown();
}

static void test_fleeing_cannot_guard(CuTest *tc) {
    guard_fixture fix;

    setup_guard(&fix, true);
    fix.u->status = ST_FLEE;
    CuAssertIntEquals(tc, E_GUARD_FLEEING, can_start_guarding(fix.u));
    update_guards();
    CuAssertTrue(tc, !fval(fix.u, UFL_GUARD));
    test_teardown();
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
    test_teardown();
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

static void test_demographics_demand(CuTest *tc) {
    region *r;
    int d;
    const struct luxury_type *lt_balm, *lt_jewel;
    const struct building_type *btype;
    test_setup();
    btype = test_create_buildingtype("harbour");
    random_source_inject_constants(DMRISE, 0);
    lt_balm = new_luxurytype(test_create_itemtype("balm"), 5);
    lt_jewel = new_luxurytype(test_create_itemtype("jewel"), 7);
    CuAssertIntEquals(tc, 2, get_maxluxuries());
    r = test_create_plain(0, 0);
    setluxuries(r, lt_jewel);
    CuAssertPtrEquals(tc, (struct item_type *)lt_jewel->itype, (struct item_type *)r_luxury(r));
    CuAssertIntEquals(tc, 0, r_demand(r, lt_jewel));
    CuAssertTrue(tc, 0 != (d = r_demand(r, lt_balm)));
    CuAssertTrue(tc, r_has_demands(r));
    demographics();
    CuAssertTrue(tc, r_has_demands(r));
    CuAssertIntEquals(tc, 0, r_demand(r, lt_jewel));
    CuAssertIntEquals(tc, d + 1, r_demand(r, lt_balm));
    random_source_inject_constants(DMRISEHAFEN, 0);

    // no harbour, no further increases:
    demographics();
    CuAssertIntEquals(tc, d + 1, r_demand(r, lt_balm));
    // new increase with harbour:
    test_create_building(r, btype);
    demographics();
    CuAssertIntEquals(tc, d + 2, r_demand(r, lt_balm));
}

static void test_luck_message(CuTest *tc) {
    region* r;
    attrib *a;

    test_setup();
    mt_create_va(mt_new("peasantluck_success", NULL), "births:int", MT_NEW_END);
    setup_terrains(tc);
    r = test_create_plain(0, 0);
    rsetpeasants(r, 1);

    demographics();

    CuAssertPtrEquals_Msg(tc, "unexpected message", NULL, r->msgs);

    a = (attrib *)a_find(r->attribs, &at_peasantluck);
    if (!a)
        a = a_add(&r->attribs, a_new(&at_peasantluck));
    a->data.i += 10;

    demographics();

    CuAssertPtrNotNull(tc, test_find_messagetype(r->msgs, "peasantluck_success"));

    test_teardown();
}

static unit * setup_name_cmd(void) {
    faction *f;

    test_setup();
    mt_create_error(84);
    mt_create_error(148);
    mt_create_error(332);
    mt_create_error(12);
    mt_create_va(mt_new("renamed_building_seen", NULL), "renamer:unit", "region:region", "building:building", MT_NEW_END);
    mt_create_va(mt_new("renamed_building_notseen", NULL), "region:region", "building:building", MT_NEW_END);
    f = test_create_faction();
    return test_create_unit(f, test_create_plain(0, 0));
}

static void test_name_foreign_unit(CuTest* tc) {
    unit* u1, * u2;
    order* ord;
    faction* f;

    u1 = setup_name_cmd();
    f = u1->faction;
    u2 = test_create_unit(test_create_faction(), test_create_plain(1, 1));
    ord = create_order(K_NAME, f->locale, "%s %s %i Dopefish",
        param_name(P_FOREIGN, f->locale),
        param_name(P_UNIT, f->locale), u2->no);

    name_cmd(u1, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "feedback_unit_not_found"));
    CuAssertPtrEquals(tc, NULL, u2->faction->msgs);

    test_clear_messages(f);
    move_unit(u2, u1->region, NULL);
    name_cmd(u1, ord);
    CuAssertStrEquals(tc, "Dopefish", u2->_name);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "feedback_unit_not_found"));
    CuAssertPtrNotNull(tc, test_find_messagetype(u2->faction->msgs, "renamed_seen"));
    test_teardown();
}

static void test_name_unit(CuTest *tc) {
    unit *u;
    faction *f;
    order *ord;

    u = setup_name_cmd();
    f = u->faction;

    ord = create_order(K_NAME, f->locale, "%s ' Klabauterfrau '", param_name(P_UNIT, f->locale));
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Klabauterfrau", u->_name);
    free_order(ord);

    ord = create_order(K_NAME, f->locale, "%s Hodor", param_name(P_UNIT, f->locale));
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", u->_name);
    free_order(ord);

    ord = create_order(K_NAME, f->locale, param_name(P_UNIT, f->locale));
    name_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error84"));
    CuAssertStrEquals(tc, "Hodor", u->_name);
    free_order(ord);

    test_teardown();
}

static void test_name_region(CuTest *tc) {
    unit *u;
    faction *f;
    order *ord;

    u = setup_name_cmd();
    u_set_building(u, test_create_building(u->region, test_create_castle()));
    f = u->faction;

    ord = create_order(K_NAME, f->locale, "%s ' Hodor Hodor '", param_name(P_REGION, f->locale));
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor Hodor", u->region->land->name);
    free_order(ord);

    ord = create_order(K_NAME, f->locale, "%s Hodor", param_name(P_REGION, f->locale));
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", u->region->land->name);
    free_order(ord);

    ord = create_order(K_NAME, f->locale, param_name(P_REGION, f->locale));
    name_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error84"));
    CuAssertStrEquals(tc, "Hodor", u->region->land->name);
    free_order(ord);

    test_teardown();
}

static void test_name_building(CuTest *tc) {
    unit *uo, *u, *ux;
    faction *f;
    order *ord;

    u = setup_name_cmd();
    u->building = test_create_building(u->region, NULL);
    f = u->faction;
    uo = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u_set_building(uo, u->building);
    ux = test_create_unit(f, test_create_plain(0, 0));
    u_set_building(ux, u->building);

    ord = create_order(K_NAME, f->locale, "%s ' Hodor Hodor '", param_name(P_BUILDING, f->locale));
    building_set_owner(uo);
    name_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error148"));
    test_clear_messages(f);
    building_set_owner(u);
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor Hodor", u->building->name);
    free_order(ord);

    ord = create_order(K_NAME, f->locale, "%s Hodor", param_name(P_BUILDING, f->locale));
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", u->building->name);

    building_setname(u->building, "Home");
    building_set_owner(ux);
    name_cmd(u, ord);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "error148"));
    CuAssertStrEquals(tc, "Hodor", u->building->name);
    test_clear_messages(f);
    free_order(ord);

    ord = create_order(K_NAME, f->locale, param_name(P_BUILDING, f->locale));
    name_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error84"));
    CuAssertStrEquals(tc, "Hodor", u->building->name);
    free_order(ord);

    /* TODO: test BTF_NAMECHANGE:
    btype->flags |= BTF_NAMECHANGE;
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error278"));
    test_clear_messages(u->faction);
    name_cmd(u, ord); */
    test_teardown();
}

static void test_name_ship(CuTest *tc) {
    unit *uo, *u, *ux;
    faction *f;

    u = setup_name_cmd();
    u->ship = test_create_ship(u->region, NULL);
    f = u->faction;
    uo = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u_set_ship(uo, u->ship);
    ux = test_create_unit(f, test_create_plain(0, 0));
    u_set_ship(ux, u->ship);

    u->thisorder = create_order(K_NAME, f->locale, "%s Hodor", param_name(P_SHIP, f->locale));

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
    u->thisorder = create_order(K_NAME, f->locale, param_name(P_SHIP, f->locale));
    name_cmd(u, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error84"));
    CuAssertStrEquals(tc, "Hodor", u->ship->name);

    test_teardown();
}

static void test_name_group(CuTest *tc) {
    faction* f;
    unit *u1, *u2;
    group *g1, *g2;

    u1 = setup_name_cmd();
    f = u1->faction;
    set_group(u1, g1 = create_group(f, "Penner", 1));
    u2 = test_create_unit(u1->faction, u1->region);
    set_group(u2, g2 = create_group(f, "Helden", 2));

    u1->thisorder = create_order(K_NAME, f->locale, "%s Deppen", param_name(P_GROUP, f->locale));
    name_cmd(u1, u1->thisorder);
    CuAssertPtrEquals(tc, g1, get_group(u1));
    CuAssertStrEquals(tc, "Deppen", g1->name);
    CuAssertPtrEquals(tc, g2, get_group(u2));
    free_order(u1->thisorder);

    u1->thisorder = create_order(K_NAME, f->locale, "%s Helden", param_name(P_GROUP, f->locale));
    name_cmd(u1, u1->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(u1->faction->msgs, "error332"));
    CuAssertPtrEquals(tc, g1, get_group(u1));
    CuAssertStrEquals(tc, "Deppen", g1->name);
    CuAssertPtrEquals(tc, g2, get_group(u2));

    test_teardown();
}

static void test_ally_cmd_errors(CuTest *tc) {
    unit *u;
    int fid;
    order *ord;

    test_setup();
    mt_create_error(66);
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    fid = u->faction->no + 1;
    CuAssertPtrEquals(tc, NULL, findfaction(fid));

    ord = create_order(K_ALLY, u->faction->locale, itoa36(fid));
    ally_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error66"));
    free_order(ord);

    test_teardown();
}

static void test_banner_cmd(CuTest *tc) {
    unit *u;
    faction *f;
    order *ord;

    test_setup();
    mt_create_va(mt_new("changebanner", NULL), "value:string", MT_NEW_END);
    u = test_create_unit(f = test_create_faction(), test_create_plain(0, 0));

    ord = create_order(K_BANNER, f->locale, "Hodor!");
    banner_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor!", faction_getbanner(f));
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "changebanner"));
    free_order(ord);
    test_clear_messages(f);

    ord = create_order(K_BANNER, f->locale, NULL);
    banner_cmd(u, ord);
    CuAssertStrEquals(tc, NULL, faction_getbanner(f));
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "changebanner"));
    free_order(ord);
    test_clear_messages(f);

    test_teardown();
}

static void test_email_cmd(CuTest *tc) {
    unit *u;
    faction *f;
    order *ord;

    test_setup();
    mt_create_error(85);
    mt_create_va(mt_new("changemail", NULL), "value:string", MT_NEW_END);
    mt_create_va(mt_new("changemail_invalid", NULL), "value:string", MT_NEW_END);
    u = test_create_unit(f = test_create_faction(), test_create_plain(0, 0));

    ord = create_order(K_EMAIL, f->locale, "hodor@example.com");
    email_cmd(u, ord);
    CuAssertStrEquals(tc, "hodor@example.com", f->email);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "changemail"));
    free_order(ord);
    test_clear_messages(f);

    ord = create_order(K_EMAIL, f->locale, "example.com");
    email_cmd(u, ord);
    CuAssertStrEquals(tc, "hodor@example.com", f->email);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "changemail_invalid"));
    free_order(ord);
    test_clear_messages(f);

    ord = create_order(K_EMAIL, f->locale, NULL);
    email_cmd(u, ord);
    CuAssertStrEquals(tc, "hodor@example.com", f->email);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error85"));
    free_order(ord);
    test_clear_messages(f);

    test_teardown();
}

static void test_name_cmd(CuTest *tc) {
    unit *u;
    faction *f;
    alliance *al;
    order *ord;

    test_setup();
    u = test_create_unit(f = test_create_faction(), test_create_plain(0, 0));
    setalliance(f, al = makealliance(42, ""));

    ord = create_order(K_NAME, f->locale, "%s '  Ho\tdor  '", param_name(P_UNIT, f->locale));
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", u->_name);
    free_order(ord);

    ord = create_order(K_NAME, f->locale, "%s '  Ho\tdor  '", param_name(P_FACTION, f->locale));
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", f->name);
    free_order(ord);

    ord = create_order(K_NAME, f->locale, "%s '  Ho\tdor  '", param_name(P_SHIP, f->locale));
    u->ship = test_create_ship(u->region, NULL);
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", u->ship->name);
    free_order(ord);
    
    ord = create_order(K_NAME, f->locale, "%s '  Ho\tdor  '", param_name(P_BUILDING, f->locale));
    u_set_building(u, test_create_building(u->region, test_create_castle()));
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", u->building->name);
    free_order(ord);
    
    ord = create_order(K_NAME, f->locale, "%s '  Ho\tdor  '", param_name(P_REGION, f->locale));
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", u->region->land->name);
    free_order(ord);

    ord = create_order(K_NAME, f->locale, "%s '  Ho\tdor  '", param_name(P_ALLIANCE, f->locale));
    name_cmd(u, ord);
    CuAssertStrEquals(tc, "Hodor", al->name);
    free_order(ord);

    test_teardown();
}

static void test_name_foreign_cmd(CuTest *tc) {
    building *b;
    faction *f;
    region *r;
    unit *u;

    test_setup();
    u = test_create_unit(f = test_create_faction(), r = test_create_plain(0, 0));
    b = test_create_building(u->region, NULL);
    u->thisorder = create_order(K_NAME, f->locale, "%s %s %s Hodor",
        param_name(P_FOREIGN, f->locale),
        param_name(P_BUILDING, f->locale),
        itoa36(b->no));
    name_cmd(u, u->thisorder);
    CuAssertStrEquals(tc, "Hodor", b->name);
    test_teardown();
}

static void test_name_cmd_2274(CuTest *tc) {
    unit *u1, *u2, *u3;
    faction *f;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    u1 = test_create_unit(test_create_faction(), r);
    u2 = test_create_unit(test_create_faction(), r);
    u3 = test_create_unit(u2->faction, r);
    u_set_building(u1, test_create_building(r, test_create_castle()));
    u1->building->size = 10;
    u_set_building(u2, test_create_building(r, test_create_castle()));
    u2->building->size = 20;

    f = u2->faction;
    u2->thisorder = create_order(K_NAME, f->locale, "%s Heimat", param_name(P_REGION, f->locale));
    name_cmd(u2, u2->thisorder);
    CuAssertStrEquals(tc, "Heimat", r->land->name);
    f = u3->faction;
    u3->thisorder = create_order(K_NAME, f->locale, "%s Hodor", param_name(P_REGION, f->locale));
    name_cmd(u3, u3->thisorder);
    CuAssertStrEquals(tc, "Hodor", r->land->name);
    f = u1->faction;
    u1->thisorder = create_order(K_NAME, f->locale, "%s notallowed", param_name(P_REGION, f->locale));
    name_cmd(u1, u1->thisorder);
    CuAssertStrEquals(tc, "Hodor", r->land->name);

    test_teardown();
}

static void test_status_cmd(CuTest* tc) {
    unit* u;
    faction* f;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    f = test_create_faction();
    CuAssertIntEquals(tc, ST_AGGRO, u->status);

    u->thisorder = create_order(K_STATUS, f->locale, "PUPSEN");
    status_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, ST_AGGRO, u->status);

    u->thisorder = create_order(K_STATUS, f->locale, param_name(P_PAUSE, f->locale));
    status_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, ST_AGGRO, u->status);

    u->thisorder = create_order(K_STATUS, f->locale, NULL);
    status_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, ST_FIGHT, u->status);

    u->thisorder = create_order(K_STATUS, f->locale, param_name(P_NOT, f->locale));
    status_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, ST_AVOID, u->status);

    u->thisorder = create_order(K_STATUS, f->locale, param_name(P_BEHIND, f->locale));
    status_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, ST_BEHIND, u->status);

    u->thisorder = create_order(K_STATUS, f->locale, param_name(P_FLEE, f->locale));
    status_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, ST_FLEE, u->status);

    u->thisorder = create_order(K_STATUS, f->locale, param_name(P_CHICKEN, f->locale));
    status_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, ST_CHICKEN, u->status);

    u->thisorder = create_order(K_STATUS, f->locale, param_name(P_AGGRO, f->locale));
    status_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, ST_AGGRO, u->status);

    u->thisorder = create_order(K_STATUS, f->locale, param_name(P_VORNE, f->locale));
    status_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, ST_FIGHT, u->status);

    test_teardown();
}

static void test_ally_cmd(CuTest* tc) {
    unit *u;
    faction * f;
    order *ord;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    f = test_create_faction();

    ord = create_order(K_ALLY, f->locale, itoa36(f->no));
    ally_cmd(u, ord);
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);
    CuAssertIntEquals(tc, HELP_ALL, ally_get(u->faction->allies, f));
    free_order(ord);

    ord = create_order(K_ALLY, f->locale, "%s %s", itoa36(f->no), param_name(P_NOT, f->locale));
    ally_cmd(u, ord);
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);
    CuAssertIntEquals(tc, 0, ally_get(u->faction->allies, f));
    free_order(ord);

    ord = create_order(K_ALLY, f->locale, "%s %s", itoa36(f->no), param_name(P_GUARD, f->locale));
    ally_cmd(u, ord);
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);
    CuAssertIntEquals(tc, HELP_GUARD, ally_get(u->faction->allies, f));
    free_order(ord);

    ord = create_order(K_ALLY, f->locale, "%s %s %s", itoa36(f->no), param_name(P_GUARD, f->locale), param_name(P_NOT, f->locale));
    ally_cmd(u, ord);
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);
    CuAssertIntEquals(tc, 0, ally_get(u->faction->allies, f));
    free_order(ord);

    test_teardown();
}

static void test_nmr_warnings(CuTest *tc) {
    faction *f1, *f2;
    test_setup();
    mt_create_va(mt_new("nmr_warning", NULL), MT_NEW_END);
    mt_create_va(mt_new("nmr_warning_final", NULL), MT_NEW_END);
    mt_create_va(mt_new("warn_dropout", NULL), "faction:faction", "turn:int", MT_NEW_END);
    config_set("nmr.timeout", "3");
    turn = 3;
    f1 = test_create_faction();
    CuAssertIntEquals(tc, 0, faction_age(f1));
    f2 = test_create_faction();
    faction_set_age(f2, 2);
    CuAssertIntEquals(tc, 2, faction_age(f2));
    f2->lastorders = 1;
    nmr_warnings();
    CuAssertPtrNotNull(tc, f1->msgs);
    CuAssertPtrNotNull(tc, test_find_messagetype(f1->msgs, "nmr_warning"));
    CuAssertPtrNotNull(tc, f2->msgs);
    CuAssertPtrNotNull(tc, f2->msgs->begin);
    CuAssertPtrNotNull(tc, test_find_messagetype(f2->msgs, "nmr_warning"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f2->msgs, "nmr_warning_final"));
    test_teardown();
}

static unit * setup_mail_cmd(void) {
    faction *f;
    
    test_setup();
    mt_create_error(66);
    mt_create_error(30);
    mt_create_va(mt_new("regionmessage", NULL), "region:region", "sender:unit", "string:string", MT_NEW_END);
    mt_create_va(mt_new("unitmessage", NULL), "region:region", "sender:unit", "string:string", "unit:unit", MT_NEW_END);
    mt_create_va(mt_new("mail_result", NULL), "message:string", "unit:unit", MT_NEW_END);
    f = test_create_faction();
    return test_create_unit(f, test_create_plain(0, 0));
}

static void test_mail_unit(CuTest *tc) {
    order *ord;
    unit *u;
    faction *f;
    
    u = setup_mail_cmd();
    f = u->faction;
    ord = create_order(K_MAIL, f->locale, "%s %s 'Hodor!'", param_name(P_UNIT, f->locale), itoa36(u->no));
    mail_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "unitmessage"));
    free_order(ord);
    test_teardown();
}

static void test_mail_faction(CuTest *tc) {
    order *ord;
    unit *u;
    faction *f;
    
    u = setup_mail_cmd();
    f = u->faction;
    ord = create_order(K_MAIL, f->locale, "%s %s 'Hodor!'", param_name(P_FACTION, f->locale), itoa36(u->faction->no));
    mail_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "regionmessage"));
    free_order(ord);
    test_teardown();
}

static void test_mail_region(CuTest *tc) {
    order *ord;
    unit *u;
    faction *f;
    
    u = setup_mail_cmd();
    f = u->faction;
    ord = create_order(K_MAIL, f->locale, "%s 'Hodor!'", param_name(P_REGION, f->locale), itoa36(u->no));
    mail_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->region->msgs, "mail_result"));
    free_order(ord);
    test_teardown();
}

static void test_mail_unit_no_msg(CuTest *tc) {
    unit *u;
    faction *f;
    order *ord;
    
    u = setup_mail_cmd();
    f = u->faction;
    ord = create_order(K_MAIL, f->locale, "%s %s", param_name(P_UNIT, f->locale), itoa36(u->no));
    mail_cmd(u, ord);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "unitmessage"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error30"));
    free_order(ord);
    test_teardown();
}

static void test_mail_faction_no_msg(CuTest *tc) {
    unit *u;
    faction *f;
    order *ord;
    
    u = setup_mail_cmd();
    f = u->faction;
    ord = create_order(K_MAIL, f->locale, "%s %s", param_name(P_FACTION, f->locale), itoa36(f->no));
    mail_cmd(u, ord);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "regionmessage"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error30"));
    free_order(ord);
    test_teardown();
}

static void test_mail_faction_no_target(CuTest *tc) {
    unit *u;
    faction *f;
    order *ord;
    
    u = setup_mail_cmd();
    f = u->faction;
    ord = create_order(K_MAIL, f->locale, "%s %s", param_name(P_FACTION, f->locale), itoa36(f->no+1));
    mail_cmd(u, ord);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "regionmessage"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error66"));
    free_order(ord);
    test_teardown();
}

static void test_mail_region_no_msg(CuTest *tc) {
    unit *u;
    faction *f;
    order *ord;
    
    u = setup_mail_cmd();
    f = u->faction;
    ord = create_order(K_MAIL, f->locale, param_name(P_REGION, f->locale));
    mail_cmd(u, ord);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(u->region->msgs, "mail_result"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error30"));
    free_order(ord);
    test_teardown();
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
    mt_create_error(21);
    mt_create_error(36);
    mt_create_va(mt_new("displayitem", NULL), "weight:int", "item:resource", "description:string", MT_NEW_END);

    loc = get_or_create_locale("de");
    locale_setstring(loc, param_name(P_ANY, NULL), "ALLE");
    init_parameters(loc);

    r = test_create_region(0, 0, test_create_terrain("testregion", LAND_REGION));
    f = test_create_faction_ex(test_create_race("human"), NULL);
    u = test_create_unit(f, r);

    itype = it_get_or_create(rt_get_or_create("testitem"));

    ord = create_order(K_RESHOW, f->locale, "testname");

    reshow_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error21"));
    test_clear_messages(f);

    locale_setstring(loc, "testitem", "testname");
    locale_setstring(loc, "iteminfo::testitem", "testdescription");

    reshow_cmd(u, ord);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "error21"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error36"));
    test_clear_messages(f);

    i_add(&(u->items), i_new(itype, 1));
    reshow_cmd(u, ord);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "error21"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "error36"));
    test_clear_messages(f);

    free_order(ord);
    test_teardown();
}

static struct locale *setup_locale(void) {
    struct locale *loc;

    loc = get_or_create_locale("de");
    locale_setstring(loc, "elvenhorse", "Elfenpferd");
    locale_setstring(loc, "elvenhorse_p", "Elfenpferde");
    locale_setstring(loc, "iteminfo::elvenhorse", "Elfenpferd Informationen");
    locale_setstring(loc, "race::elf_p", "Elfen");
    locale_setstring(loc, "race::elf", "Elf");
    locale_setstring(loc, "race::human_p", "Menschen");
    locale_setstring(loc, "race::human", "Mensch");
    locale_setstring(loc, "stat_hitpoints", "Trefferpunkte");
    locale_setstring(loc, "stat_attack", "Angriff");
    locale_setstring(loc, "stat_attacks", "Angriffe");
    locale_setstring(loc, "stat_defense", "Verteidigung");
    locale_setstring(loc, "stat_armor", "Ruestung");
    locale_setstring(loc, "stat_equipment", "Gegenstaende");
    locale_setstring(loc, "stat_pierce", "Pieks");
    locale_setstring(loc, "stat_cut", "Schnipp");
    locale_setstring(loc, "stat_bash", "Boink");
    locale_setstring(loc, "attack_magical", "magisch");
    locale_setstring(loc, "attack_standard", "normal");
    locale_setstring(loc, "attack_natural", "natuerlich");
    locale_setstring(loc, "attack_structural", "strukturell");
    init_locale(loc);
    return loc;
}

static void test_show_race(CuTest *tc) {
    order *ord;
    race * rc;
    unit *u;
    struct locale *loc;
    message * msg;

    test_setup();
    mt_create_va(mt_new("msg_event", NULL), "string:string", MT_NEW_END);
    test_create_race("human");
    rc = test_create_race("elf");

    loc = setup_locale();
    u = test_create_unit(test_create_faction_ex(rc, NULL), test_create_plain(0, 0));
    u->faction->locale = loc;

    ord = create_order(K_RESHOW, loc, "Mensch");
    reshow_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error21"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(u->faction->msgs, "msg_event"));
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

    test_teardown();
}

static void test_show_both(CuTest *tc) {
    order *ord;
    race * rc;
    unit *u;
    struct locale *loc;
    message * msg;

    test_setup();
    mt_create_va(mt_new("msg_event", NULL), "string:string", MT_NEW_END);
    mt_create_va(mt_new("displayitem", NULL), "weight:int", "item:resource", "description:string", MT_NEW_END);
    rc = test_create_race("elf");
    test_create_itemtype("elvenhorse");

    loc = setup_locale();

    CuAssertPtrNotNull(tc, finditemtype("elf", loc));
    CuAssertPtrNotNull(tc, findrace("elf", loc));

    u = test_create_unit(test_create_faction_ex(rc, NULL), test_create_plain(0, 0));
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
    CuAssertTrue(tc, memcmp("Elfenpferd Informationen", msg->parameters[2].v, 4) == 0);
    test_clear_messages(u->faction);
    free_order(ord);
    test_teardown();
}

static void test_immigration(CuTest * tc)
{
    region *r;

    test_setup();
    r = test_create_plain(0, 0);

    rsetpeasants(r, 0);
    config_set("rules.economy.repopulate_maximum", 0);

    random_source_inject_constant(0);

    immigration();
    CuAssertIntEquals(tc, 0, rpeasants(r));

    random_source_inject_constant(1);

    immigration();
    CuAssertIntEquals(tc, 2, rpeasants(r));

    config_set("rules.wage.function", "0");
    immigration();
    CuAssertIntEquals(tc, 2, rpeasants(r));

    test_teardown();
}

static void test_demon_hunger(CuTest * tc)
{
    const resource_type *rtype;
    region *r;
    race *rc;
    faction *f;
    unit *u;

    test_setup();
    config_set("hunger.damage", NULL);
    init_resources();
    r = test_create_plain(0, 0);
    rc = test_create_race("demon");
    f = test_create_faction_ex(rc, NULL);
    u = test_create_unit(f, r);
    u->hp = 999;

    config_set("hunger.demon.peasant_tolerance", "1");

    rtype = get_resourcetype(R_SILVER);
    i_change(&u->items, rtype->itype, 30);
    scale_number(u, 1);
    rsetpeasants(r, 0);

    get_food(r);

    CuAssertIntEquals(tc, 20, i_get(u->items, rtype->itype));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "malnourish"));

    config_set("hunger.demon.peasant_tolerance", "0");

    get_food(r);

    CuAssertIntEquals(tc, 10, i_get(u->items, rtype->itype));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "malnourish"));

    test_teardown();
}

static void test_armedmen(CuTest *tc) {
    /* TODO: test RCF_NOWEAPONS and SK_WEAPONLESS */
    unit *u;
    item_type *it_sword;
    weapon_type *wtype;
    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    it_sword = test_create_itemtype("sword");
    wtype = new_weapontype(it_sword, 0, frac_make(1, 2), 0, 0, 0, 0, SK_MELEE);
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
    wtype->flags |= WTF_SIEGE;
    CuAssertIntEquals(tc, 0, armedmen(u, false));
    CuAssertIntEquals(tc, 1, armedmen(u, true));
    test_teardown();
}

static void test_cansee(CuTest *tc) {
    unit *u, *u2;
    
    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u2 = test_create_unit(test_create_faction(), u->region);
    
    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 0));

    set_level(u2, SK_STEALTH, 1);
    CuAssertTrue(tc, !cansee(u->faction, u->region, u2, 0));
    
    set_level(u, SK_PERCEPTION, 1);
    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 0));
    
    test_teardown();
}

static void test_cansee_ring(CuTest *tc) {
    unit *u, *u2;
    item_type *iring, *isee;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u2 = test_create_unit(test_create_faction(), u->region);

    iring = test_create_itemtype("roi");
    isee = test_create_itemtype("aots");
    CuAssertPtrNotNull(tc, get_resourcetype(R_RING_OF_INVISIBILITY));
    CuAssertPtrEquals(tc, iring->rtype, (void *)get_resourcetype(R_RING_OF_INVISIBILITY));
    CuAssertPtrNotNull(tc, get_resourcetype(R_AMULET_OF_TRUE_SEEING));
    CuAssertPtrEquals(tc, isee->rtype, (void *)get_resourcetype(R_AMULET_OF_TRUE_SEEING));

    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 0));

    /* a single ring hides one person, but not two: */
    i_change(&u2->items, iring, 1);
    CuAssertTrue(tc, !cansee(u->faction, u->region, u2, 0));
    scale_number(u2, 2);
    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 0));

    /* two rings can hide two people */
    i_change(&u2->items, iring, 1);
    CuAssertTrue(tc, !cansee(u->faction, u->region, u2, 0));

    /* one amulet negates one of the two rings */
    i_change(&u->items, isee, 1);
    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 0));

    /* having more rings than people doesn't help: */
    i_change(&u2->items, iring, 1);
    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 0));

    test_teardown();
}

static void test_cansee_sphere(CuTest *tc) {
    unit *u, *u2;
    item_type *itype[2];

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u2 = test_create_unit(test_create_faction(), u->region);

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

    test_teardown();
}

static void test_cansee_temp(CuTest* tc) {
    unit* u, * u2;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u2 = test_create_unit(test_create_faction(), u->region);

    u->orders = create_order(K_MAKETEMP, u->faction->locale, "1");
    new_units();
    u2 = u2->next;

    CuAssertPtrNotNull(tc, u2);
    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 0));

    test_teardown();
}

static void test_cansee_empty(CuTest *tc) {
    unit *u, *u2;
    faction *f;

    test_setup();
    u = test_create_unit(f = test_create_faction(), test_create_plain(0, 0));
    u2 = test_create_unit(test_create_faction(), u->region);

    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 0));

    u2->number = 0;
    set_level(u2, SK_STEALTH, 1);
    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 0));

    test_teardown();
}


/**
 * Hidden monsters are seen in oceans if they are big enough.
 */
static void test_cansee_monsters(CuTest *tc) {
    unit *u, *u2;
    race *rc;
    const item_type *itype;

    test_setup();
    itype = test_create_itemtype("roi");
    u = test_create_unit(test_create_faction(), test_create_ocean(0, 0));
    u2 = test_create_unit(test_create_faction(), u->region);
    rc = test_create_race("seaserpent");
    rc->weight = 4999;
    u_setrace(u2, rc);

    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 0));
    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 2));

    set_level(u2, SK_STEALTH, 1);
    CuAssertTrue(tc, !cansee(u->faction, u->region, u2, 0));
    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 1));

    rc->weight = 5000;
    /* no stealth for fatties at sea */
    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 0));
    CuAssertTrue(tc, cansee(u->faction, u->region, u2, 1));

    /* rings still work */
    i_change(&u2->items, itype, 1);
    CuAssertTrue(tc, !cansee(u->faction, u->region, u2, 0));
    CuAssertTrue(tc, !cansee(u->faction, u->region, u2, 2));

    test_teardown();
}

static void test_nmr_timeout(CuTest *tc) {
    test_setup();
    CuAssertIntEquals(tc, 0, NMRTimeout());
    config_set_int("nmr.timeout", 5);
    CuAssertIntEquals(tc, 5, NMRTimeout());
    config_set_int("game.maxnmr", 4);
    CuAssertIntEquals(tc, 4, NMRTimeout());
    config_set("nmr.timeout", NULL);
    CuAssertIntEquals(tc, 4, NMRTimeout());
    config_set("game.maxnmr", NULL);
    CuAssertIntEquals(tc, 0, NMRTimeout());
    test_teardown();
}

static void test_long_orders(CuTest *tc) {
    unit *u;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    CuAssertTrue(tc, long_order_allowed(u, true));
    u->flags |= UFL_LONGACTION;
    CuAssertTrue(tc, !long_order_allowed(u, true));
    u->flags -= UFL_LONGACTION;
    u->faction->flags |= FFL_PAUSED;
    CuAssertTrue(tc, !long_order_allowed(u, true));
    test_teardown();
}

static void test_long_order_on_ocean(CuTest *tc) {
    unit *u;
    race * rc;

    test_setup();
    rc = test_create_race("pikachu");
    u = test_create_unit(test_create_faction_ex(rc, NULL), test_create_ocean(0, 0));
    CuAssertTrue(tc, long_order_allowed(u, true));
    CuAssertTrue(tc, !long_order_allowed(u, false));
    rc->flags |= RCF_SWIM;
    CuAssertTrue(tc, long_order_allowed(u, false));
    u->faction->flags |= FFL_PAUSED;
    CuAssertTrue(tc, !long_order_allowed(u, false));
    u->faction->flags -= FFL_PAUSED;

    rc = test_create_race("aquarian");
    u = test_create_unit(test_create_faction_ex(rc, NULL), u->region);
    CuAssertTrue(tc, long_order_allowed(u, false));
    CuAssertTrue(tc, long_order_allowed(u, true));
    u->faction->flags |= FFL_PAUSED;
    CuAssertTrue(tc, !long_order_allowed(u, true));
    CuAssertTrue(tc, !long_order_allowed(u, false));
    test_teardown();
}

static void test_password_cmd(CuTest *tc) {
    unit *u;
    message *msg;
    faction * f;

    CuAssertTrue(tc, password_wellformed("PASSword"));
    CuAssertTrue(tc, password_wellformed("1234567"));
    CuAssertTrue(tc, !password_wellformed("$password"));
    CuAssertTrue(tc, !password_wellformed("no space"));

    test_setup();
    mt_create_error(283);
    mt_create_error(321);
    mt_create_va(mt_new("changepasswd", NULL), "value:string", MT_NEW_END);
    u = test_create_unit(f = test_create_faction(), test_create_plain(0, 0));
    u->thisorder = create_order(K_PASSWORD, f->locale, "password1234", NULL);
    password_cmd(u, u->thisorder);
    CuAssertTrue(tc, checkpasswd(f, "password1234"));
    CuAssertPtrNotNull(tc, msg = test_find_messagetype(f->msgs, "changepasswd"));
    free_order(u->thisorder);
    test_clear_messages(f);

    u->thisorder = create_order(K_PASSWORD, f->locale, "bad-password", NULL);
    password_cmd(u, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error283"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "changepasswd"));
    CuAssertTrue(tc, !checkpasswd(f, "password1234"));
    free_order(u->thisorder);
    test_clear_messages(f);

    u->thisorder = create_order(K_PASSWORD, f->locale, "''", NULL);
    password_cmd(u, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error283"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "changepasswd"));
    free_order(u->thisorder);
    test_clear_messages(f);

    u->thisorder = create_order(K_PASSWORD, f->locale, NULL);
    password_cmd(u, u->thisorder);
    CuAssertTrue(tc, !checkpasswd(f, "password1234"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "error283"));
    CuAssertPtrNotNull(tc, msg = test_find_messagetype(f->msgs, "changepasswd"));
    free_order(u->thisorder);
    test_clear_messages(f);

    u->thisorder = create_order(K_PASSWORD, f->locale, "1234567890123456789012345678901234567890");
    password_cmd(u, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error321"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "changepasswd"));
    CuAssertTrue(tc, checkpasswd(f, "1234567890123456789012345678901"));

    test_teardown();
}

static void test_peasant_migration(CuTest *tc) {
    region *r1, *r2;
    int rmax;

    test_setup();
    config_set("rules.economy.repopulate_maximum", "0");
    r1 = test_create_plain(0, 0);
    rsettrees(r1, 0, 0);
    rsettrees(r1, 1, 0);
    rsettrees(r1, 2, 0);
    rmax = region_production(r1);
    r2 = test_create_plain(0, 1);
    rsettrees(r2, 0, 0);
    rsettrees(r2, 1, 0);
    rsettrees(r2, 2, 0);

    rsetpeasants(r1, rmax - 90);
    rsetpeasants(r2, rmax);
    peasant_migration(r1);
    immigration();
    CuAssertIntEquals(tc, rmax - 90, rpeasants(r1));
    CuAssertIntEquals(tc, rmax, rpeasants(r2));

    rsetpeasants(r1, rmax - 90);
    rsetpeasants(r2, rmax + 60);
    peasant_migration(r1);
    immigration();
    CuAssertIntEquals(tc, rmax - 80, rpeasants(r1));
    CuAssertIntEquals(tc, rmax + 50, rpeasants(r2));

    rsetpeasants(r1, rmax - 6); /* max 4 immigrants. */
    rsetpeasants(r2, rmax + 60); /* max 10 emigrants. */
    peasant_migration(r1);
    immigration(); /* 4 peasants will move */
    CuAssertIntEquals(tc, rmax - 2, rpeasants(r1));
    CuAssertIntEquals(tc, rmax + 56, rpeasants(r2));

    rsetpeasants(r1, rmax - 6); /* max 4 immigrants. */
    rsetpeasants(r2, rmax + 6); /* max 1 emigrant. */
    peasant_migration(r1);
    immigration(); /* 4 peasants will move */
    CuAssertIntEquals(tc, rmax - 5, rpeasants(r1));
    CuAssertIntEquals(tc, rmax + 5, rpeasants(r2));

    test_teardown();
}

static void test_tree_growth_winter(CuTest* tc) {
    region* r;
    int week;

    test_setup();
    random_source_inject_constant(0); /* maximum herb growth */
    test_create_calendar();
    r = test_create_plain(0, 0);
    rsettrees(r, 0, 100);
    rsettrees(r, 1, 100);
    rsettrees(r, 2, 100);
    rsetherbs(r, 50);
    CuAssertIntEquals(tc, SEASON_WINTER, calendar_season(week = 0));
    demographics_week(week);
    CuAssertPtrEquals(tc, NULL, a_find(r->attribs, &at_germs));
    CuAssertIntEquals(tc, 100, rtrees(r, 0));
    CuAssertIntEquals(tc, 100, rtrees(r, 1));
    CuAssertIntEquals(tc, 100, rtrees(r, 2));
    CuAssertIntEquals(tc, 50, rherbs(r));

    CuAssertIntEquals(tc, SEASON_WINTER, calendar_season(week = 1));
    demographics_week(week);
    CuAssertPtrEquals(tc, NULL, a_find(r->attribs, &at_germs));
    CuAssertIntEquals(tc, 100, rtrees(r, 0));
    CuAssertIntEquals(tc, 100, rtrees(r, 1));
    CuAssertIntEquals(tc, 100, rtrees(r, 2));
    CuAssertIntEquals(tc, 50, rherbs(r));
    test_teardown();
}

static void test_tree_growth_spring(CuTest* tc) {
    region* r;
    attrib* a;
    int week;

    test_setup();
    test_create_calendar();
    r = test_create_plain(0, 0);
    rsettrees(r, 0, 100);
    rsettrees(r, 1, 100);
    rsettrees(r, 2, 100);
    CuAssertIntEquals(tc, SEASON_SPRING, calendar_season(week = 2));
    demographics_week(week);
    CuAssertPtrNotNull(tc, a = a_find(r->attribs, &at_germs));
    CuAssertIntEquals(tc, 12, a->data.sa[0]);
    CuAssertIntEquals(tc, 12, a->data.sa[1]);
    CuAssertIntEquals(tc, 100, rtrees(r, 0));
    CuAssertIntEquals(tc, 88, rtrees(r, 1));
    CuAssertIntEquals(tc, 112, rtrees(r, 2));

    a_removeall(&r->attribs, &at_germs);
    rsettrees(r, 0, 100);
    rsettrees(r, 1, 100);
    rsettrees(r, 2, 100);
    CuAssertIntEquals(tc, SEASON_SPRING, calendar_season(week = 3));
    demographics_week(week);
    CuAssertPtrNotNull(tc, a = a_find(r->attribs, &at_germs));
    CuAssertIntEquals(tc, 12, a->data.sa[0]);
    CuAssertIntEquals(tc, 12, a->data.sa[1]);
    CuAssertIntEquals(tc, 100, rtrees(r, 0));
    CuAssertIntEquals(tc, 88, rtrees(r, 1));
    CuAssertIntEquals(tc, 112, rtrees(r, 2));
    test_teardown();
}

static void test_tree_growth_summer(CuTest* tc) {
    region* r;
    unit* u;
    attrib* a;
    race* rc_elf;
    int week;

    test_setup();
    test_create_calendar();
    rc_elf = test_create_race("elf");
    config_set("rules.treeseeds.chance", "1.0");
    r = test_create_plain(0, 0);
    rsettrees(r, 0, 100);
    rsettrees(r, 1, 100);
    rsettrees(r, 2, 100);
    CuAssertIntEquals(tc, SEASON_SUMMER, calendar_season(week = 4));
    demographics_week(week);
    CuAssertPtrEquals(tc, NULL, a_find(r->attribs, &at_germs));
    CuAssertIntEquals(tc, 300, rtrees(r, 0));
    CuAssertIntEquals(tc, 100, rtrees(r, 1));
    CuAssertIntEquals(tc, 100, rtrees(r, 2));
    rsettrees(r, 0, 100);

    a_add(&r->attribs, a = a_new(&at_germs));
    a->data.sa[1] = 50;
    demographics_week(week);
    CuAssertPtrEquals(tc, NULL, a_find(r->attribs, &at_germs));
    CuAssertIntEquals(tc, 400, rtrees(r, 0));
    CuAssertIntEquals(tc, 50, rtrees(r, 1));
    CuAssertIntEquals(tc, 150, rtrees(r, 2));
    rsettrees(r, 0, 100);
    rsettrees(r, 1, 100);
    rsettrees(r, 2, 100);

    CuAssertIntEquals(tc, SEASON_SUMMER, calendar_season(week = 5));
    demographics_week(week);
    CuAssertPtrEquals(tc, NULL, a_find(r->attribs, &at_germs));
    CuAssertIntEquals(tc, 300, rtrees(r, 0));
    CuAssertIntEquals(tc, 100, rtrees(r, 1));
    CuAssertIntEquals(tc, 100, rtrees(r, 2));
    rsettrees(r, 0, 100);

    u = test_create_unit(test_create_faction(), r);
    demographics_week(week);
    CuAssertPtrEquals(tc, NULL, a_find(r->attribs, &at_germs));
    CuAssertIntEquals(tc, 200, rtrees(r, 0));
    CuAssertIntEquals(tc, 100, rtrees(r, 1));
    CuAssertIntEquals(tc, 100, rtrees(r, 2));
    rsettrees(r, 0, 100);

    u_setrace(u, rc_elf);
    demographics_week(week);
    CuAssertPtrEquals(tc, NULL, a_find(r->attribs, &at_germs));
    CuAssertIntEquals(tc, 300, rtrees(r, 0));
    CuAssertIntEquals(tc, 100, rtrees(r, 1));
    CuAssertIntEquals(tc, 100, rtrees(r, 2));
    rsettrees(r, 0, 100);

    test_teardown();
}

static void test_tree_growth_autumn(CuTest* tc) {
    region* r;
    unit* u;
    race* rc_elf;
    int week;

    test_setup();
    test_create_calendar();
    rc_elf = test_create_race("elf");
    config_set("rules.treeseeds.chance", "1.0");
    r = test_create_plain(0, 0);
    rsettrees(r, 0, 100);
    rsettrees(r, 1, 100);
    rsettrees(r, 2, 100);
    CuAssertIntEquals(tc, SEASON_AUTUMN, calendar_season(week = 6));
    demographics_week(week);
    CuAssertPtrEquals(tc, NULL, a_find(r->attribs, &at_germs));
    CuAssertIntEquals(tc, 300, rtrees(r, 0));
    CuAssertIntEquals(tc, 100, rtrees(r, 1));
    CuAssertIntEquals(tc, 100, rtrees(r, 2));
    rsettrees(r, 0, 100);

    CuAssertIntEquals(tc, SEASON_AUTUMN, calendar_season(week = 7));
    demographics_week(week);
    CuAssertPtrEquals(tc, NULL, a_find(r->attribs, &at_germs));
    CuAssertIntEquals(tc, 300, rtrees(r, 0));
    CuAssertIntEquals(tc, 100, rtrees(r, 1));
    CuAssertIntEquals(tc, 100, rtrees(r, 2));
    rsettrees(r, 0, 100);

    u = test_create_unit(test_create_faction(), r);
    demographics_week(week);
    CuAssertPtrEquals(tc, NULL, a_find(r->attribs, &at_germs));
    CuAssertIntEquals(tc, 200, rtrees(r, 0));
    CuAssertIntEquals(tc, 100, rtrees(r, 1));
    CuAssertIntEquals(tc, 100, rtrees(r, 2));
    rsettrees(r, 0, 100);

    u_setrace(u, rc_elf);
    demographics_week(week);
    CuAssertPtrEquals(tc, NULL, a_find(r->attribs, &at_germs));
    CuAssertIntEquals(tc, 300, rtrees(r, 0));
    CuAssertIntEquals(tc, 100, rtrees(r, 1));
    CuAssertIntEquals(tc, 100, rtrees(r, 2));
    rsettrees(r, 0, 100);

    test_teardown();
}

static void test_quit(CuTest *tc) {
    faction *f;
    unit *u;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    u->thisorder = create_order(K_QUIT, f->locale, "password");

    faction_setpassword(f, "passwort");
    quit_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, 0, f->flags & FFL_QUIT);

    faction_setpassword(f, "password");
    quit_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, FFL_QUIT, f->flags & FFL_QUIT);

    test_teardown();
}

#ifdef QUIT_WITH_TRANSFER
/**
 * Gifting units to another faction upon voluntary death (QUIT).
 */ 
static void test_quit_transfer(CuTest *tc) {
    faction *f1, *f2;
    unit *u1, *u2;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    f1 = test_create_faction();
    faction_setpassword(f1, "password");
    u1 = test_create_unit(f1, r);
    f2 = test_create_faction();
    u2 = test_create_unit(f2, r);
    contact_unit(u2, u1);
    u1->thisorder = create_order(K_QUIT, f1->locale, "password %s %s",
        param_name(P_FACTION, f1->locale), itoa36(f2->no));
    quit_cmd(u1, u1->thisorder);
    CuAssertIntEquals(tc, FFL_QUIT, f1->flags & FFL_QUIT);
    CuAssertPtrEquals(tc, f2, u1->faction);
    test_teardown();
}

/**
 * Gifting units requires CONTACT.
 */ 
static void test_quit_transfer_no_contact(CuTest *tc) {
    faction *f1, *f2;
    unit *u1, *u2;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    f1 = test_create_faction();
    faction_setpassword(f1, "password");
    u1 = test_create_unit(f1, r);
    f2 = test_create_faction();
    u2 = test_create_unit(f2, r);
    u1->thisorder = create_order(K_QUIT, f1->locale, "password %s %s",
        param_name(P_FACTION, f1->locale), itoa36(f2->no));
    quit_cmd(u1, u1->thisorder);
    CuAssertIntEquals(tc, 0, f1->flags & FFL_QUIT);
    CuAssertPtrEquals(tc, f1, u1->faction);
    CuAssertPtrEquals(tc, f2, u2->faction);
    CuAssertPtrNotNull(tc, test_find_messagetype(f1->msgs, "error40"));
    test_teardown();
}

/**
 * Gifting units requires CONTACT UNIT, not FACTION.
 */ 
static void test_quit_transfer_contact_faction(CuTest *tc) {
    faction *f1, *f2;
    unit *u1, *u2;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    f1 = test_create_faction();
    faction_setpassword(f1, "password");
    u1 = test_create_unit(f1, r);
    f2 = test_create_faction();
    u2 = test_create_unit(f2, r);
    contact_faction(u2, u1->faction);
    u1->thisorder = create_order(K_QUIT, f1->locale, "password %s %s",
        param_name(P_FACTION, f1->locale), itoa36(f2->no));
    quit_cmd(u1, u1->thisorder);
    CuAssertIntEquals(tc, FFL_QUIT, f1->flags & FFL_QUIT);
    CuAssertPtrEquals(tc, f2, u1->faction);
    test_teardown();
}

/**
 * Gifting units requires CONTACT in same region.
 */
static void test_quit_transfer_contact_wrong_region(CuTest* tc) {
    faction* f1, * f2;
    unit* u1, * u2;
    region* r;

    test_setup();
    r = test_create_plain(0, 0);
    f1 = test_create_faction();
    faction_setpassword(f1, "password");
    u1 = test_create_unit(f1, r);
    f2 = test_create_faction();
    u2 = test_create_unit(f2, test_create_plain(1, 1));
    contact_unit(u2, u1);
    u1->thisorder = create_order(K_QUIT, f1->locale, "password %s %s",
        param_name(P_FACTION, f1->locale), itoa36(f2->no));
    quit_cmd(u1, u1->thisorder);
    CuAssertIntEquals(tc, 0, f1->flags & FFL_QUIT);
    CuAssertPtrEquals(tc, f1, u1->faction);
    CuAssertPtrNotNull(tc, test_find_messagetype(f1->msgs, "error40"));
    test_teardown();
}

/**
 * Gifting units with limited skills to another faction.
 *
 * This is allowed only up to the limit of the target faction.
 * Units that would break the limit are not transferred.
 */
static void test_quit_transfer_limited(CuTest *tc) {
    faction *f1, *f2;
    unit *u1, *u2;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    f1 = test_create_faction();
    faction_setpassword(f1, "password");
    u1 = test_create_unit(f1, r);
    f2 = test_create_faction();
    u2 = test_create_unit(f2, r);
    contact_unit(u2, u1);
    u1->thisorder = create_order(K_QUIT, f1->locale, "password %s %s",
        param_name(P_FACTION, f1->locale), itoa36(f2->no));

    set_level(u1, SK_MAGIC, 1);
    set_level(u2, SK_MAGIC, 1);
    CuAssertIntEquals(tc, true, has_limited_skills(u1));

    config_set_int("rules.maxskills.magic", 1);
    quit_cmd(u1, u1->thisorder);
    CuAssertIntEquals(tc, FFL_QUIT, f1->flags & FFL_QUIT);
    CuAssertPtrEquals(tc, f1, u1->faction);

    f1->flags -= FFL_QUIT;
    config_set_int("rules.maxskills.magic", 2);
    quit_cmd(u1, u1->thisorder);
    CuAssertIntEquals(tc, FFL_QUIT, f1->flags & FFL_QUIT);
    CuAssertPtrEquals(tc, f2, u1->faction);

    test_teardown();
}

/**
 * Mages can be transfered within same school.
 */
static void test_quit_transfer_mages(CuTest *tc) {
    faction *f1, *f2;
    unit *u1, *u2;
    region *r;

    test_setup();
    config_set_int("rules.maxskills.magic", 2);
    r = test_create_plain(0, 0);
    f1 = test_create_faction();
    faction_setpassword(f1, "password");
    u1 = test_create_unit(f1, r);
    f2 = test_create_faction();
    u2 = test_create_unit(f2, r);
    contact_unit(u2, u1);
    u1->thisorder = create_order(K_QUIT, f1->locale, "password %s %s",
        param_name(P_FACTION, f1->locale), itoa36(f2->no));

    f1->magiegebiet = M_GWYRRD;
    set_level(u1, SK_MAGIC, 1);
    create_mage(u1, M_GWYRRD);

    f2->magiegebiet = M_GWYRRD;
    set_level(u2, SK_MAGIC, 1);
    create_mage(u2, M_GWYRRD);

    quit_cmd(u1, u1->thisorder);
    CuAssertPtrEquals(tc, f2, u1->faction);
    CuAssertPtrEquals(tc, f2, u2->faction);
    CuAssertIntEquals(tc, FFL_QUIT, f1->flags & FFL_QUIT);

    test_teardown();
}

/**
 * Mages cannot be transfered from another school.
 */
static void test_quit_transfer_different_mages(CuTest *tc) {
    faction *f1, *f2;
    unit *u1, *u2;
    region *r;

    test_setup();
    config_set_int("rules.maxskills.magic", 2);
    r = test_create_plain(0, 0);
    f1 = test_create_faction();
    faction_setpassword(f1, "password");
    u1 = test_create_unit(f1, r);
    f2 = test_create_faction();
    u2 = test_create_unit(f2, r);
    contact_unit(u2, u1);
    u1->thisorder = create_order(K_QUIT, f1->locale, "password %s %s",
        param_name(P_FACTION, f1->locale), itoa36(f2->no));

    f1->magiegebiet = M_DRAIG;
    set_level(u1, SK_MAGIC, 1);
    create_mage(u1, M_DRAIG);

    f2->magiegebiet = M_GWYRRD;
    set_level(u2, SK_MAGIC, 1);
    create_mage(u2, M_GWYRRD);

    quit_cmd(u1, u1->thisorder);
    CuAssertPtrEquals(tc, f1, u1->faction);
    CuAssertPtrEquals(tc, f2, u2->faction);
    CuAssertIntEquals(tc, FFL_QUIT, f1->flags & FFL_QUIT);

    test_teardown();
}

/**
 * Only units of the same race can be gifted to another faction.
 */
static void test_quit_transfer_migrants(CuTest *tc) {
    faction *f1, *f2;
    unit *u1, *u2;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    f1 = test_create_faction();
    faction_setpassword(f1, "password");
    u1 = test_create_unit(f1, r);
    f2 = test_create_faction();
    u2 = test_create_unit(f2, r);
    contact_unit(u2, u1);
    u1->thisorder = create_order(K_QUIT, f1->locale, "password %s %s",
        param_name(P_FACTION, f1->locale), itoa36(f2->no));

    u_setrace(u1, test_create_race("smurf"));

    quit_cmd(u1, u1->thisorder);
    CuAssertIntEquals(tc, FFL_QUIT, f1->flags & FFL_QUIT);
    CuAssertPtrEquals(tc, f1, u1->faction);

    test_teardown();
}

/**
 * A hero that is gifted to another faction loses their status.
 */
static void test_quit_transfer_hero(CuTest *tc) {
    faction *f1, *f2;
    unit *u1, *u2;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    f1 = test_create_faction();
    faction_setpassword(f1, "password");
    u1 = test_create_unit(f1, r);
    f2 = test_create_faction();
    u2 = test_create_unit(f2, r);
    contact_unit(u2, u1);
    u1->thisorder = create_order(K_QUIT, f1->locale, "password %s %s",
        param_name(P_FACTION, f1->locale), itoa36(f2->no));

    u1->flags |= UFL_HERO;

    quit_cmd(u1, u1->thisorder);
    CuAssertIntEquals(tc, FFL_QUIT, f1->flags & FFL_QUIT);
    CuAssertPtrEquals(tc, f2, u1->faction);
    CuAssertIntEquals(tc, 0, u1->flags & UFL_HERO);

    test_teardown();
}

static void test_transfer_faction(CuTest *tc) {
    faction *f1, *f2;
    unit *u1, *u2, *u3, *u4;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    f1 = test_create_faction();
    f2 = test_create_faction();
    u1 = test_create_unit(f1, r);
    u2 = test_create_unit(f1, r);
    u_setrace(u2, test_create_race("smurf"));
    u3 = test_create_unit(f2, r);
    u4 = test_create_unit(f1, r);
    transfer_faction(f1, f2);
    CuAssertPtrEquals(tc, f2, u1->faction);
    CuAssertPtrEquals(tc, f1, u2->faction);
    CuAssertPtrEquals(tc, f2, u3->faction);
    CuAssertPtrEquals(tc, f2, u4->faction);

    test_teardown();
}
#endif

CuSuite *get_laws_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_maketemp_default_order);
    SUITE_ADD_TEST(suite, test_maketemp);
    SUITE_ADD_TEST(suite, test_findparam_ex);
    SUITE_ADD_TEST(suite, test_nmr_warnings);
    SUITE_ADD_TEST(suite, test_status_cmd);
    SUITE_ADD_TEST(suite, test_ally_cmd);
    SUITE_ADD_TEST(suite, test_name_cmd);
    SUITE_ADD_TEST(suite, test_name_foreign_cmd);
    SUITE_ADD_TEST(suite, test_banner_cmd);
    SUITE_ADD_TEST(suite, test_email_cmd);
    SUITE_ADD_TEST(suite, test_name_cmd_2274);
    SUITE_ADD_TEST(suite, test_name_foreign_unit);
    SUITE_ADD_TEST(suite, test_name_unit);
    SUITE_ADD_TEST(suite, test_name_region);
    SUITE_ADD_TEST(suite, test_name_building);
    SUITE_ADD_TEST(suite, test_name_ship);
    SUITE_ADD_TEST(suite, test_name_group);
    SUITE_ADD_TEST(suite, test_ally_cmd_errors);
    SUITE_ADD_TEST(suite, test_new_building_can_be_renamed);
    SUITE_ADD_TEST(suite, test_password_cmd);
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
    SUITE_ADD_TEST(suite, test_unskilled_cannot_guard);
    SUITE_ADD_TEST(suite, test_promotion_cmd);
    SUITE_ADD_TEST(suite, test_pay_cmd);
    SUITE_ADD_TEST(suite, test_pay_cmd_other_building);
    SUITE_ADD_TEST(suite, test_pay_cmd_must_be_owner);
    SUITE_ADD_TEST(suite, test_new_units);
    SUITE_ADD_TEST(suite, test_cannot_create_unit_above_limit);
    SUITE_ADD_TEST(suite, test_enter_building);
    SUITE_ADD_TEST(suite, test_enter_ship);
    SUITE_ADD_TEST(suite, test_display_cmd);
    SUITE_ADD_TEST(suite, test_expel_building);
    SUITE_ADD_TEST(suite, test_expel_ship);
    SUITE_ADD_TEST(suite, test_expel_ship_at_sea);
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
    SUITE_ADD_TEST(suite, test_demographics_demand);
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
    SUITE_ADD_TEST(suite, test_cansee_monsters);
    SUITE_ADD_TEST(suite, test_cansee_empty);
    SUITE_ADD_TEST(suite, test_cansee_temp);
    SUITE_ADD_TEST(suite, test_nmr_timeout);
    SUITE_ADD_TEST(suite, test_long_orders);
    SUITE_ADD_TEST(suite, test_long_order_on_ocean);
    SUITE_ADD_TEST(suite, test_quit);
    SUITE_ADD_TEST(suite, test_peasant_migration);
    SUITE_ADD_TEST(suite, test_tree_growth_winter);
    SUITE_ADD_TEST(suite, test_tree_growth_spring);
    SUITE_ADD_TEST(suite, test_tree_growth_summer);
    SUITE_ADD_TEST(suite, test_tree_growth_autumn);
#ifdef QUIT_WITH_TRANSFER
    SUITE_ADD_TEST(suite, test_quit_transfer);
    SUITE_ADD_TEST(suite, test_quit_transfer_no_contact);
    SUITE_ADD_TEST(suite, test_quit_transfer_contact_faction);
    SUITE_ADD_TEST(suite, test_quit_transfer_contact_wrong_region);
    SUITE_ADD_TEST(suite, test_quit_transfer_limited);
    SUITE_ADD_TEST(suite, test_quit_transfer_migrants);
    SUITE_ADD_TEST(suite, test_quit_transfer_mages);
    SUITE_ADD_TEST(suite, test_quit_transfer_different_mages);
    SUITE_ADD_TEST(suite, test_quit_transfer_hero);
    SUITE_ADD_TEST(suite, test_transfer_faction);
#endif

    return suite;
}
