#include "economy.h"

#include "contact.h"
#include "give.h"
#include "recruit.h"
#include "study.h"

#include <spells/buildingcurse.h>

#include <kernel/building.h>
#include <kernel/calendar.h>
#include <kernel/config.h>
#include <kernel/curse.h>
#include <kernel/faction.h>
#include "kernel/direction.h"         // for D_EAST, directions
#include <kernel/item.h>
#include <kernel/order.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/resources.h>
#include <kernel/ship.h>
#include <kernel/skills.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>
#include <kernel/build.h>      // for construction, requirement
#include <kernel/skill.h>      // for SK_ROAD_BUILDING, SK_WEAPONSMITH, SK_A...

#include <util/language.h>
#include <util/macros.h>
#include <util/message.h>
#include <util/param.h>
#include <util/keyword.h>      // for K_BUY, K_DESTROY, K_RECRUIT, K_SELL
#include <util/variant.h>      // for variant, frac_make, frac_zero

#include <CuTest.h>
#include <tests.h>

#include <stb_ds.h>

#include <assert.h>
#include <stdbool.h>           // for false, true
#include <stdlib.h>            // for NULL, calloc, free, malloc

static void test_give_unit_cmd(CuTest * tc)
{
    unit *u1, *u2;
    region *r;
    faction* f;
    order *ord;

    test_setup();
    r = test_create_plain(0, 0);
    u1 = test_create_unit(f = test_create_faction(), r);
    u2 = test_create_unit(test_create_faction(), test_create_plain(1, 1));
    contact_unit(u2, u1);
    ord = create_order(K_GIVE, u1->faction->locale, "%i %s",
        u2->no, param_name(P_UNIT, u1->faction->locale));

    test_clear_messages(u1->faction);
    test_clear_messages(u2->faction);
    give_unit_cmd(u1, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "feedback_unit_not_found"));
    CuAssertPtrEquals(tc, NULL, u2->faction->msgs);
    CuAssertPtrEquals(tc, f, u1->faction);

    move_unit(u2, u1->region, NULL);
    test_clear_messages(u1->faction);
    test_clear_messages(u2->faction);
    give_unit_cmd(u1, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "give_person"));
    CuAssertPtrNotNull(tc, test_find_messagetype(u2->faction->msgs, "receive_person"));
    CuAssertPtrEquals(tc, u2->faction, u1->faction);

    test_teardown();
}

static void test_give_control_cmd(CuTest * tc)
{
    unit *u1, *u2;
    building *b;
    region *r;
    order *ord;

    test_setup();
    r = test_create_plain(0, 0);
    b = test_create_building(r, NULL);
    u1 = test_create_unit(test_create_faction(), r);
    u2 = test_create_unit(test_create_faction(), test_create_plain(1, 1));
    u_set_building(u1, b);
    CuAssertPtrEquals(tc, u1, building_owner(b));
    ord = create_order(K_GIVE, u1->faction->locale, "%i %s",
        u2->no, param_name(P_CONTROL, u1->faction->locale));

    test_clear_messages(u1->faction);
    test_clear_messages(u2->faction);
    give_control_cmd(u1, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u1->faction->msgs, "feedback_unit_not_found"));
    CuAssertPtrEquals(tc, u1, building_owner(b));
    CuAssertPtrEquals(tc, NULL, u2->faction->msgs);

    move_unit(u2, u1->region, NULL);
    test_clear_messages(u1->faction);
    test_clear_messages(u2->faction);
    give_control_cmd(u1, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u1->faction->msgs, "error33"));
    CuAssertPtrEquals(tc, NULL, u2->faction->msgs);
    CuAssertPtrEquals(tc, u1, building_owner(b));

    u_set_building(u2, b);
    CuAssertPtrEquals(tc, u1, building_owner(b));
    test_clear_messages(u1->faction);
    test_clear_messages(u2->faction);
    give_control_cmd(u1, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u1->faction->msgs, "givecommand"));
    CuAssertPtrNotNull(tc, test_find_messagetype(u2->faction->msgs, "givecommand"));
    CuAssertPtrEquals(tc, u2, building_owner(b));

    test_teardown();
}

static void test_give_control_building(CuTest * tc)
{
    unit *u1, *u2;
    building *b;
    struct faction *f;
    region *r;

    test_setup();
    f = test_create_faction();
    r = test_create_plain(0, 0);
    b = test_create_building(r, NULL);
    u1 = test_create_unit(f, r);
    u_set_building(u1, b);
    u2 = test_create_unit(f, r);
    u_set_building(u2, b);
    CuAssertPtrEquals(tc, u1, building_owner(b));
    give_control(u1, u2);
    CuAssertPtrEquals(tc, u2, building_owner(b));
    test_teardown();
}

static void test_give_control_ship(CuTest * tc)
{
    unit *u1, *u2;
    ship *sh;
    struct faction *f;
    region *r;

    test_setup();
    f = test_create_faction();
    r = test_create_plain(0, 0);
    sh = test_create_ship(r, NULL);
    u1 = test_create_unit(f, r);
    u_set_ship(u1, sh);
    u2 = test_create_unit(f, r);
    u_set_ship(u2, sh);
    CuAssertPtrEquals(tc, u1, ship_owner(sh));
    give_control(u1, u2);
    CuAssertPtrEquals(tc, u2, ship_owner(sh));
    test_teardown();
}

struct steal {
    struct unit *u;
    struct region *r;
    struct faction *f;
};

static void setup_steal(struct steal *env, struct terrain_type *ter, struct race *rc) {
    env->r = test_create_region(0, 0, ter);
    env->f = test_create_faction_ex(rc, NULL);
    env->u = test_create_unit(env->f, env->r);
}

static void test_steal_okay(CuTest * tc) {
    struct steal env;
    race *rc;
    struct terrain_type *ter;
    message *msg;

    test_setup();
    ter = test_create_terrain("plain", LAND_REGION);
    rc = test_create_race("human");
    rc->flags = 0;
    setup_steal(&env, ter, rc);
    CuAssertPtrEquals(tc, NULL, msg = steal_message(env.u, 0));
    assert(!msg);
    test_teardown();
}

static void test_steal_nosteal(CuTest * tc) {
    struct steal env;
    race *rc;
    terrain_type *ter;
    message *msg;

    test_setup();
    ter = test_create_terrain("plain", LAND_REGION);
    rc = test_create_race("human");
    rc->flags = RCF_NOSTEAL;
    setup_steal(&env, ter, rc);
    CuAssertPtrNotNull(tc, msg = steal_message(env.u, 0));
    msg_release(msg);
    test_teardown();
}

static void test_steal_ocean(CuTest * tc) {
    struct steal env;
    race *rc;
    terrain_type *ter;
    message *msg;

    test_setup();
    ter = test_create_terrain("ocean", SEA_REGION);
    rc = test_create_race("human");
    setup_steal(&env, ter, rc);
    CuAssertPtrNotNull(tc, msg = steal_message(env.u, 0));
    msg_release(msg);
    test_teardown();
}

static struct unit *create_recruiter(void) {
    region *r;
    faction *f;
    unit *u;
    const resource_type* rtype;

    r=test_create_plain(0, 0);
    rsetpeasants(r, 999);
    f = test_create_faction();
    u = test_create_unit(f, r);
    rtype = get_resourcetype(R_SILVER);
    change_resource(u, rtype, 1000);
    return u;
}

static void setup_production(void) {
    init_resources();
    config_set_int("study.produceexp", 0);
    mt_create_feedback("error_cannotmake");
    mt_create_va(mt_new("produce", NULL), "unit:unit", "region:region", "amount:int", "wanted:int", "resource:resource", MT_NEW_END);
    mt_create_va(mt_new("income", NULL), "unit:unit", "region:region", "amount:int", "wanted:int", "mode:int", MT_NEW_END);
    mt_create_va(mt_new("buy", NULL), "unit:unit", "money:int", MT_NEW_END);
    mt_create_va(mt_new("buyamount", NULL), "unit:unit", "amount:int", "resource:resource", MT_NEW_END);
    mt_create_va(mt_new("sellamount", NULL), "unit:unit", "amount:int", "resource:resource", MT_NEW_END);
}

static void test_heroes_dont_recruit(CuTest * tc) {
    unit *u;

    test_setup();
    setup_production();
    u = create_recruiter();

    fset(u, UFL_HERO);
    unit_addorder(u, create_order(K_RECRUIT, default_locale, "1"));

    recruit(u->region);

    CuAssertIntEquals(tc, 1, u->number);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error_herorecruit"));

    test_teardown();
}

static void test_normals_recruit(CuTest * tc) {
    unit *u;

    test_setup();
    setup_production();
    u = create_recruiter();
    unit_addorder(u, create_order(K_RECRUIT, default_locale, "1"));

    recruit(u->region);

    CuAssertIntEquals(tc, 2, u->number);

    test_teardown();
}

/** 
 * Create any terrain types that are used by the trade rules.
 * 
 * This should prevent newterrain from returning NULL.
 */
static void setup_terrains(CuTest *tc) {
    test_create_terrain("plain", LAND_REGION | FOREST_REGION | WALK_INTO | CAVALRY_REGION | FLY_INTO);
    test_create_terrain("ocean", SEA_REGION | SWIM_INTO | FLY_INTO);
    test_create_terrain("swamp", LAND_REGION | WALK_INTO | FLY_INTO);
    test_create_terrain("desert", LAND_REGION | WALK_INTO | FLY_INTO);
    test_create_terrain("mountain", LAND_REGION | WALK_INTO | FLY_INTO);
    init_terrains();
    CuAssertPtrNotNull(tc, newterrain(T_MOUNTAIN));
    CuAssertPtrNotNull(tc, newterrain(T_OCEAN));
    CuAssertPtrNotNull(tc, newterrain(T_PLAIN));
    CuAssertPtrNotNull(tc, newterrain(T_SWAMP));
    CuAssertPtrNotNull(tc, newterrain(T_DESERT));
}

static region *setup_trade_region(CuTest *tc, const struct terrain_type *terrain) {
    region *r;
    item_type *it_luxury;
    struct locale * lang = test_create_locale();

    new_luxurytype(it_luxury = test_create_itemtype("oil"), 5);
    locale_setstring(lang, it_luxury->rtype->_name, it_luxury->rtype->_name);
    CuAssertStrEquals(tc, it_luxury->rtype->_name, LOC(lang, resourcename(it_luxury->rtype, 0)));

    new_luxurytype(it_luxury = test_create_itemtype("balm"), 5);
    locale_setstring(lang, it_luxury->rtype->_name, it_luxury->rtype->_name);
    CuAssertStrEquals(tc, it_luxury->rtype->_name, LOC(lang, resourcename(it_luxury->rtype, 0)));

    new_luxurytype(it_luxury = test_create_itemtype("jewel"), 5);
    locale_setstring(lang, it_luxury->rtype->_name, it_luxury->rtype->_name);
    CuAssertStrEquals(tc, it_luxury->rtype->_name, LOC(lang, resourcename(it_luxury->rtype, 0)));

    r = test_create_region(0, 0, terrain);
    setluxuries(r, it_luxury->rtype->ltype);
    return r;
}

static unit *setup_trade_unit(CuTest *tc, region *r, const struct race *rc) {
    unit *u;

    UNUSED_ARG(tc);
    u = test_create_unit(test_create_faction_ex(rc, NULL), r);
    test_set_skill(u, SK_TRADE, 2, 1);
    return u;
}

static void test_sell_over_demand(CuTest* tc) {
    region* r;
    unit* u;
    building* b;
    const item_type* it_luxury;
    const luxury_type* ltype;
    int max_products;

    test_setup();
    setup_production();
    r = setup_trade_region(tc, NULL);
    it_luxury = r_luxury(r);
    ltype = it_luxury->rtype->ltype;
    rsetpeasants(r, TRADE_FRACTION * 10);
    max_products = rpeasants(r) / TRADE_FRACTION;
    r_setdemand(r, ltype, 2);
    b = test_create_building(r, test_create_castle());
    b->size = 2;
    u = test_create_unit(test_create_faction(), r);
    set_level(u, SK_TRADE, 10);
    i_change(&u->items, it_luxury, max_products + 1);
    unit_addorder(u, create_order(K_SELL, u->faction->locale, "%d %s",
        max_products + 1,
        LOC(u->faction->locale, resourcename(it_luxury->rtype, 0))));
    produce(r);
    CuAssertIntEquals(tc, 0, i_get(u->items, it_luxury));
    CuAssertIntEquals(tc, 1, r_demand(r, ltype));
    CuAssertIntEquals(tc, max_products * 2 * ltype->price + ltype->price, i_get(u->items, it_find("money")));
    test_teardown();
}

static void test_sell_all(CuTest* tc) {
    region* r;
    unit* u;
    building* b;
    const item_type* it_luxury;
    const luxury_type* ltype;
    int max_products;
    message *m;

    test_setup();
    setup_production();
    r = setup_trade_region(tc, NULL);
    it_luxury = r_luxury(r);
    ltype = it_luxury->rtype->ltype;
    rsetpeasants(r, TRADE_FRACTION * 10);
    max_products = rpeasants(r) / TRADE_FRACTION;
    r_setdemand(r, ltype, 2);
    b = test_create_building(r, test_create_castle());
    b->size = 2;
    u = test_create_unit(test_create_faction(), r);
    set_level(u, SK_TRADE, 10);
    i_change(&u->items, it_luxury, max_products);
    unit_addorder(u, create_order(K_SELL, u->faction->locale, "%s %s",
        param_name(P_ANY, u->faction->locale),
        LOC(u->faction->locale, resourcename(it_luxury->rtype, 0))));
    produce(r);
    CuAssertIntEquals(tc, 0, i_get(u->items, it_luxury));
    CuAssertIntEquals(tc, 2, r_demand(r, ltype));
    CuAssertIntEquals(tc, max_products * 2 * ltype->price, i_get(u->items, it_find("money")));
    CuAssertPtrNotNull(tc, m = test_find_faction_message(u->faction, "income"));
    CuAssertIntEquals(tc, max_products * 2 * ltype->price, m->parameters[2].i);
    test_teardown();
}

static void test_sales_taxes(CuTest *tc) {
    region *r;
    unit *u, *ub;
    building *b;
    const item_type *it_luxury, *it_money;
    const luxury_type *ltype;
    int max_products, revenue;
    message *m;

    test_setup();
    setup_production();
    r = setup_trade_region(tc, NULL);
    it_luxury = r_luxury(r);
    it_money = it_find("money");
    ltype = it_luxury->rtype->ltype;
    rsetpeasants(r, TRADE_FRACTION * 100);
    max_products = rpeasants(r) / TRADE_FRACTION;
    r_setdemand(r, ltype, 2);
    b = test_create_building(r, test_create_castle());
    b->size = 10; // 12% sales tax
    CuAssertIntEquals(tc, 2, buildingeffsize(b, false));

    ub = test_create_unit(test_create_faction(), r);
    u_set_building(ub, b);

    u = test_create_unit(test_create_faction(), r);
    set_level(u, SK_TRADE, 10);
    i_change(&u->items, it_luxury, max_products);
    unit_addorder(u, create_order(K_SELL, u->faction->locale, "%d %s",
        max_products,
        LOC(u->faction->locale, resourcename(it_luxury->rtype, 0))));

    produce(r);
    CuAssertIntEquals(tc, 0, i_get(u->items, it_luxury));
    CuAssertIntEquals(tc, 2, r_demand(r, ltype));
    revenue = max_products * 2 * ltype->price;
    CuAssertIntEquals(tc, revenue * 88 / 100, i_get(u->items, it_money));
    CuAssertPtrNotNull(tc, m = test_find_faction_message(u->faction, "income"));
    CuAssertIntEquals(tc, revenue * 88 / 100, m->parameters[2].i);
    CuAssertIntEquals(tc, revenue * 12 / 100, i_get(ub->items, it_money));
    CuAssertPtrNotNull(tc, m = test_find_faction_message(ub->faction, "income"));
    CuAssertIntEquals(tc, revenue * 12 / 100, m->parameters[2].i);
    test_teardown();
}

static void test_import_taxes(CuTest *tc) {
    region *r;
    unit *u, *ub;
    building *b;
    const item_type *it_luxury, *it_money;
    const luxury_type *ltype;
    int max_products, revenue;
    message *m;

    test_setup();
    setup_production();
    r = setup_trade_region(tc, NULL);
    it_luxury = r_luxury(r);
    it_money = it_find("money");
    ltype = it_luxury->rtype->ltype;
    rsetpeasants(r, TRADE_FRACTION * 100);
    max_products = rpeasants(r) / TRADE_FRACTION;
    r_setdemand(r, ltype, 2);
    b = test_create_building(r, test_create_castle());
    b->size = 2;
    b = test_create_building(r, test_create_buildingtype("harbour"));
    b->size = b->type->maxsize;

    ub = test_create_unit(test_create_faction(), r);
    u_set_building(ub, b);

    u = test_create_unit(test_create_faction(), r);
    set_level(u, SK_TRADE, 10);
    i_change(&u->items, it_luxury, max_products);
    unit_addorder(u, create_order(K_SELL, u->faction->locale, "%d %s",
        max_products,
        LOC(u->faction->locale, resourcename(it_luxury->rtype, 0))));

    produce(r);
    CuAssertIntEquals(tc, 0, i_get(u->items, it_luxury));
    CuAssertIntEquals(tc, 2, r_demand(r, ltype));
    revenue = max_products * 2 * ltype->price;
    CuAssertIntEquals(tc, revenue * 90 / 100, i_get(u->items, it_money));
    CuAssertPtrNotNull(tc, m = test_find_faction_message(u->faction, "income"));
    CuAssertIntEquals(tc, revenue * 90 / 100, m->parameters[2].i);
    CuAssertIntEquals(tc, revenue * 10 / 100, i_get(ub->items, it_money));
    CuAssertPtrNotNull(tc, m = test_find_faction_message(ub->faction, "income"));
    CuAssertIntEquals(tc, revenue * 10 / 100, m->parameters[2].i);
    test_teardown();
}

static void test_sell_nothing_message(CuTest *tc) {
    region* r;
    unit* u, *u2;
    building* b;
    const item_type* it_luxury;

    test_setup();
    setup_production();
    r = setup_trade_region(tc, NULL);
    it_luxury = r_luxury(r);
    rsetpeasants(r, TRADE_FRACTION * 10);
    b = test_create_building(r, test_create_castle());
    b->size = 2;
    u = test_create_unit(test_create_faction(), r);
    u2 = test_create_unit(test_create_faction(), r);
    set_level(u, SK_TRADE, 10);
    set_level(u2, SK_TRADE, 10);
    i_change(&u->items, it_find("money"), 500);
    unit_addorder(u, create_order(K_BUY, u->faction->locale, "5 %s",
        LOC(u->faction->locale, resourcename(it_luxury->rtype, 0))));
    it_luxury = it_find("balm");
    unit_addorder(u, create_order(K_SELL, u->faction->locale, "10 %s",
        LOC(u->faction->locale, resourcename(it_luxury->rtype, 0))));
    i_change(&u2->items, it_luxury, 50);
    unit_addorder(u2, create_order(K_SELL, u->faction->locale, "10 %s",
        LOC(u->faction->locale, resourcename(it_luxury->rtype, 0))));
    produce(r);
    CuAssertPtrNotNull(tc, test_find_messagetype(u2->faction->msgs, "income"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(u->faction->msgs, "income"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(u->faction->msgs, "sellamount"));
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error264"));
    test_teardown();
}

static void test_trade_limits(CuTest *tc) {
    region *r;
    unit *u;
    building *b;
    const item_type *it_jewel, *it_balm;

    test_setup();
    setup_production();
    r = setup_trade_region(tc, NULL);
    b = test_create_building(r, test_create_castle());
    b->size = 2;
    rsetpeasants(r, TRADE_FRACTION * 20);
    it_jewel = it_find("jewel");
    u = test_create_unit(test_create_faction(), r);
    set_level(u, SK_TRADE, 1);
    i_change(&u->items, it_find("money"), 500);
    unit_addorder(u, create_order(K_BUY, u->faction->locale, "5 %s",
        LOC(u->faction->locale, resourcename(it_jewel->rtype, 0))));
    it_balm = it_find("balm");
    i_change(&u->items, it_balm, 10);
    unit_addorder(u, create_order(K_SELL, u->faction->locale, "10 %s",
        LOC(u->faction->locale, resourcename(it_balm->rtype, 0))));
    produce(r);
    CuAssertIntEquals(tc, 5, i_get(u->items, it_jewel));
    CuAssertIntEquals(tc, 5, i_get(u->items, it_balm));
    test_teardown();
}

static void test_trade_produceexp(CuTest *tc) {
    region *r;
    unit *u;
    building *b;
    skill *sv;
    const item_type *it_jewel, *it_balm;

    test_setup();
    setup_production();
    config_set_int("study.produceexp", STUDYDAYS);
    r = setup_trade_region(tc, NULL);
    b = test_create_building(r, test_create_castle());
    b->size = 2;
    rsetpeasants(r, TRADE_FRACTION * 20);
    it_jewel = it_find("jewel");
    u = test_create_unit(test_create_faction(), r);
    sv = test_set_skill(u, SK_TRADE, 2, 1);
    i_change(&u->items, it_find("money"), 500);
    unit_addorder(u, create_order(K_BUY, u->faction->locale, "10 %s",
        LOC(u->faction->locale, resourcename(it_jewel->rtype, 0))));
    it_balm = it_find("balm");
    i_change(&u->items, it_balm, 20);
    unit_addorder(u, create_order(K_SELL, u->faction->locale, "10 %s",
        LOC(u->faction->locale, resourcename(it_balm->rtype, 0))));
    produce(r);
    CuAssertIntEquals(tc, 10, i_get(u->items, it_jewel));
    CuAssertIntEquals(tc, 10, i_get(u->items, it_balm));
    CuAssertIntEquals(tc, 3, sv->level);
    test_teardown();
}

static void test_buy_limits(CuTest *tc) {
    region *r;
    unit *u;
    building *b;
    const item_type *it_jewel;

    test_setup();
    setup_production();
    r = setup_trade_region(tc, NULL);
    b = test_create_building(r, test_create_castle());
    b->size = 2;
    rsetpeasants(r, TRADE_FRACTION * 20);
    it_jewel = it_find("jewel");
    u = test_create_unit(test_create_faction(), r);
    set_level(u, SK_TRADE, 1);
    i_change(&u->items, it_find("money"), 5000);
    unit_addorder(u, create_order(K_BUY, u->faction->locale, "30 %s",
        LOC(u->faction->locale, resourcename(it_jewel->rtype, 0))));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, "20 %s",
        LOC(u->faction->locale, resourcename(it_jewel->rtype, 0))));
    produce(r);
    CuAssertIntEquals(tc, 10, i_get(u->items, it_jewel));
    test_teardown();
}

static void test_buy_limited_funds(CuTest *tc) {
    region *r;
    unit *u1, *u2;
    building *b;
    const item_type *it_jewel;

    test_setup();
    setup_production();
    r = setup_trade_region(tc, NULL);
    b = test_create_building(r, test_create_castle());
    b->size = 2;
    rsetpeasants(r, TRADE_FRACTION * 20);
    it_jewel = it_find("jewel");
    u1 = test_create_unit(test_create_faction(), r);
    scale_number(u1, 10);
    set_level(u1, SK_TRADE, 1);
    i_change(&u1->items, it_find("money"), 5000);
    unit_addorder(u1, create_order(K_BUY, u1->faction->locale, "100 %s",
        LOC(u1->faction->locale, resourcename(it_jewel->rtype, 0))));
    u2 = test_create_unit(test_create_faction(), r);
    scale_number(u2, 100);
    set_level(u2, SK_TRADE, 1);
    unit_addorder(u2, create_order(K_BUY, u2->faction->locale, "1000 %s",
        LOC(u2->faction->locale, resourcename(it_jewel->rtype, 0))));
    produce(r);
    CuAssertIntEquals(tc, 0, i_get(u2->items, it_jewel));
    CuAssertIntEquals(tc, 100, i_get(u1->items, it_jewel));
    test_teardown();
}

static void test_trade_needs_castle(CuTest *tc) {
    /* Handeln ist nur in Regionen mit Burgen m�glich. */
    race *rc;
    region *r;
    unit *u;
    building *b;
    const terrain_type *t_swamp, *t_desert, *t_plain;
    const item_type *it_luxury;

    test_setup();
    setup_production();
    setup_terrains(tc);
    init_terrains();
    test_create_locale();
    t_swamp = get_terrain("swamp");
    t_desert = get_terrain("desert");
    t_plain = get_terrain("plain");
    r = setup_trade_region(tc, t_plain);
    it_luxury = r_luxury(r);

    rc = test_create_race(NULL);
    CuAssertTrue(tc, trade_needs_castle(t_swamp, rc));
    CuAssertTrue(tc, trade_needs_castle(t_desert, rc));
    CuAssertTrue(tc, trade_needs_castle(t_plain, rc));

    u = test_create_unit(test_create_faction_ex(rc, NULL), r);
    unit_addorder(u, create_order(K_BUY, u->faction->locale, "1 %s",
        LOC(u->faction->locale, resourcename(it_luxury->rtype, 0))));
    unit_addorder(u, create_order(K_SELL, u->faction->locale, "1 %s",
        LOC(u->faction->locale, resourcename(it_luxury->rtype, 0))));
    produce(r);
    CuAssertIntEquals(tc, 2, test_count_messagetype(u->faction->msgs, "error119"));

    test_clear_messages(u->faction);
    freset(u, UFL_LONGACTION);
    b = test_create_building(r, test_create_castle());
    b->size = 1;
    produce(r);
    CuAssertIntEquals(tc, 2, test_count_messagetype(u->faction->msgs, "error119"));

    test_clear_messages(u->faction);
    freset(u, UFL_LONGACTION);
    b->size = 2;
    test_clear_messages(u->faction);
    produce(r);
    CuAssertIntEquals(tc, 0, test_count_messagetype(u->faction->msgs, "error119"));

    rc = test_create_race("insect");
    CuAssertTrue(tc, !trade_needs_castle(t_swamp, rc));
    CuAssertTrue(tc, !trade_needs_castle(t_desert, rc));
    CuAssertTrue(tc, trade_needs_castle(t_plain, rc));
    test_teardown();
}

static void test_trade_insect(CuTest *tc) {
    /* Insekten koennen in Wuesten und Suempfen auch ohne Burgen handeln. */
    unit *u;
    region *r;
    race *rc;
    const terrain_type *t_swamp;
    const item_type *it_luxury;

    test_setup();
    setup_production();
    test_create_locale();
    setup_terrains(tc);
    init_terrains();
    t_swamp = get_terrain("swamp");
    rc = test_create_race("insect");

    r = setup_trade_region(tc, t_swamp);
    it_luxury = r_luxury(r);
    CuAssertTrue(tc, !trade_needs_castle(t_swamp, rc));
    CuAssertPtrNotNull(tc, it_luxury);
    u = setup_trade_unit(tc, r, rc);
    unit_addorder(u, create_order(K_BUY, u->faction->locale, "1 %s",
        LOC(u->faction->locale, resourcename(it_luxury->rtype, 0))));
    unit_addorder(u, create_order(K_SELL, u->faction->locale, "1 %s",
        LOC(u->faction->locale, resourcename(it_luxury->rtype, 0))));
    produce(u->region);
    CuAssertIntEquals(tc, 0, test_count_messagetype(u->faction->msgs, "error119"));
    test_teardown();
}

static void test_buy_prices(CuTest* tc) {
    region* r;
    unit* u;
    building* b;
    const resource_type* rt_silver;
    const item_type* it_luxury;
    int sold, costs;

    test_setup();
    setup_production();
    test_create_locale();
    r = setup_trade_region(tc, test_create_terrain("swamp", LAND_REGION));
    it_luxury = r_luxury(r);
    CuAssertIntEquals(tc, 0, r_demand(r, it_luxury->rtype->ltype));
    CuAssertPtrNotNull(tc, it_luxury);
    rt_silver = get_resourcetype(R_SILVER);
    CuAssertPtrNotNull(tc, rt_silver);
    CuAssertPtrNotNull(tc, rt_silver->itype);

    b = test_create_building(r, test_create_castle());
    b->size = 2;
    sold = max_luxuries_sold(r);
    costs = sold * it_luxury->rtype->ltype->price;
    u = test_create_unit(test_create_faction(), r);
    set_level(u, SK_TRADE, (1 + sold) / 5); /* could buy twice what is for sale here */
    test_set_item(u, rt_silver->itype, costs * 2);
    unit_addorder(u, create_order(K_BUY, u->faction->locale, "%d %s", sold, LOC(u->faction->locale, resourcename(it_luxury->rtype, 0))));

    produce(r);

    CuAssertIntEquals(tc, sold, get_item(u, it_luxury));
    CuAssertIntEquals(tc, costs, get_item(u, rt_silver->itype));
    test_teardown();
}

static void test_buy_prices_split(CuTest* tc) {
    region* r;
    unit* u, *u2;
    building* b;
    const resource_type* rt_silver;
    const item_type* it_luxury;
    int sold, costs;

    test_setup();
    setup_production();
    test_create_locale();
    r = setup_trade_region(tc, test_create_terrain("swamp", LAND_REGION));
    it_luxury = r_luxury(r);
    CuAssertIntEquals(tc, 0, r_demand(r, it_luxury->rtype->ltype));
    CuAssertPtrNotNull(tc, it_luxury);
    rt_silver = get_resourcetype(R_SILVER);
    CuAssertPtrNotNull(tc, rt_silver);
    CuAssertPtrNotNull(tc, rt_silver->itype);

    b = test_create_building(r, test_create_castle());
    b->size = 2;
    sold = max_luxuries_sold(r);
    costs = sold * it_luxury->rtype->ltype->price;
    u = test_create_unit(test_create_faction(), r);
    set_level(u, SK_TRADE, (1 + sold) / 5); /* could buy twice what is for sale here */
    test_set_item(u, rt_silver->itype, costs * 2);
    unit_addorder(u, create_order(K_BUY, u->faction->locale, "%d %s", sold, LOC(u->faction->locale, resourcename(it_luxury->rtype, 0))));
    u2 = test_create_unit(test_create_faction(), r);
    set_level(u2, SK_TRADE, (1 + sold) / 5); /* could buy twice what is for sale here */
    test_set_item(u2, rt_silver->itype, costs * 2);
    unit_addorder(u2, create_order(K_BUY, u->faction->locale, "%d %s", sold, LOC(u->faction->locale, resourcename(it_luxury->rtype, 0))));

    produce(r);

    CuAssertIntEquals(tc, sold, get_item(u, it_luxury));
    CuAssertIntEquals(tc, sold, get_item(u2, it_luxury));
    CuAssertIntEquals(tc, costs, get_item(u, rt_silver->itype) + get_item(u2, rt_silver->itype));
    test_teardown();
}

static void test_buy_prices_rising(CuTest* tc) {
    region* r;
    unit* u;
    building* b;
    const resource_type* rt_silver;
    const item_type* it_luxury;
    int sold, costs, price;

    test_setup();
    setup_production();
    test_create_locale();
    r = setup_trade_region(tc, test_create_terrain("swamp", LAND_REGION));
    it_luxury = r_luxury(r);
    CuAssertIntEquals(tc, 0, r_demand(r, it_luxury->rtype->ltype));
    CuAssertPtrNotNull(tc, it_luxury);
    rt_silver = get_resourcetype(R_SILVER);
    CuAssertPtrNotNull(tc, rt_silver);
    CuAssertPtrNotNull(tc, rt_silver->itype);

    b = test_create_building(r, test_create_castle());
    b->size = 2;
    u = test_create_unit(test_create_faction(), r);
    sold = max_luxuries_sold(r);
    set_level(u, SK_TRADE, (1 + sold) / 5); /* could buy twice what is for sale here */
    price = it_luxury->rtype->ltype->price;
    costs = sold * price;
    test_set_item(u, rt_silver->itype, costs * 2);
    unit_addorder(u, create_order(K_BUY, u->faction->locale, "%d %s", sold + 1, LOC(u->faction->locale, resourcename(it_luxury->rtype, 0))));

    produce(r);

    CuAssertIntEquals(tc, sold + 1, get_item(u, it_luxury));
    CuAssertIntEquals(tc, costs - 2 * price, get_item(u, rt_silver->itype));
    test_teardown();
}

static void test_trade_is_long_action(CuTest *tc) {
    unit *u;
    region *r;
    const item_type *it_luxury, *it_sold;
    const resource_type *rt_silver;
    test_setup();
    setup_production();
    r = setup_trade_region(tc, NULL);
    test_create_building(r, test_create_castle())->size = 2;
    rt_silver = get_resourcetype(R_SILVER);
    CuAssertPtrNotNull(tc, rt_silver);
    CuAssertPtrNotNull(tc, rt_silver->itype);
    it_luxury = r_luxury(r);
    it_sold = it_find("balm");
    CuAssertTrue(tc, it_sold != it_luxury);
    CuAssertPtrNotNull(tc, it_luxury);
    u = test_create_unit(test_create_faction(), r);
    test_set_item(u, it_sold, 100);
    test_set_item(u, rt_silver->itype, 1000);
    set_level(u, SK_TRADE, 1);
    unit_addorder(u, create_order(K_BUY, u->faction->locale, "1 %s", LOC(u->faction->locale, resourcename(it_luxury->rtype, 0))));
    unit_addorder(u, create_order(K_SELL, u->faction->locale, "1 %s", LOC(u->faction->locale, resourcename(it_sold->rtype, 0))));
    produce(u->region);
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "error52"));
    CuAssertIntEquals(tc, UFL_NOTMOVING | UFL_LONGACTION, u->flags & (UFL_NOTMOVING | UFL_LONGACTION));
    CuAssertIntEquals(tc, 1, i_get(u->items, it_luxury));
    CuAssertIntEquals(tc, 99, i_get(u->items, it_sold));

    produce(u->region);
    // error, but message is created in update_long_order!
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "error52"));
    CuAssertIntEquals(tc, 1, i_get(u->items, it_luxury));
    CuAssertIntEquals(tc, 99, i_get(u->items, it_sold));
    test_teardown();
}

static void test_forget_cmd(CuTest *tc) {
    unit *u;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    test_set_skill(u, SK_CATAPULT, 3, 4);
    u->thisorder = create_order(K_FORGET, u->faction->locale, skillname(SK_CATAPULT, u->faction->locale));
    forget_cmd(u, u->thisorder);
    CuAssertPtrEquals(tc, NULL, u->skills);
    test_teardown();
}

static void test_buy_cmd(CuTest *tc) {
    region * r;
    unit *u;
    building *b;
    const resource_type *rt_silver;
    const item_type *it_luxury;
    test_setup();
    setup_production();
    test_create_locale();
    r = setup_trade_region(tc, test_create_terrain("swamp", LAND_REGION));
    config_set_int("study.produceexp", 0);

    it_luxury = r_luxury(r);
    CuAssertPtrNotNull(tc, it_luxury);
    rt_silver = get_resourcetype(R_SILVER);
    CuAssertPtrNotNull(tc, rt_silver);
    CuAssertPtrNotNull(tc, rt_silver->itype);

    u = test_create_unit(test_create_faction(), r);
    unit_addorder(u, create_order(K_BUY, u->faction->locale, "1 %s", LOC(u->faction->locale, resourcename(it_luxury->rtype, 0))));
    test_set_item(u, rt_silver->itype, 1000);

    produce(r);
    CuAssertPtrNotNullMsg(tc, "trading requires a castle", test_find_messagetype(u->faction->msgs, "error119"));
    test_clear_messages(u->faction);
    freset(u, UFL_LONGACTION);

    b = test_create_building(r, test_create_castle());
    produce(r);
    CuAssertPtrNotNullMsg(tc, "castle must have size >=2", test_find_messagetype(u->faction->msgs, "error119"));
    test_clear_messages(u->faction);
    freset(u, UFL_LONGACTION);

    b->size = 2;
    produce(r);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(u->faction->msgs, "error119"));
    CuAssertPtrNotNullMsg(tc, "traders need SK_TRADE skill", test_find_messagetype(u->faction->msgs, "skill_needed"));
    test_clear_messages(u->faction);
    freset(u, UFL_LONGACTION);

    /* at last, the happy case: */
    set_level(u, SK_TRADE, 1);
    produce(r);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "buy"));
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "buyamount"));
    CuAssertIntEquals(tc, 1, get_item(u, it_luxury));
    CuAssertIntEquals(tc, 995, get_item(u, rt_silver->itype));
    test_teardown();
}

static void test_buy_before_sell(CuTest* tc) {
    region* r;
    unit* u;
    building* b;
    const resource_type* rt_silver;
    const item_type* it_luxury, * it_other;

    test_setup();
    setup_production();
    test_create_locale();
    r = setup_trade_region(tc, test_create_terrain("swamp", LAND_REGION));

    it_luxury = r_luxury(r);
    CuAssertPtrNotNull(tc, it_luxury);
    rt_silver = get_resourcetype(R_SILVER);
    CuAssertPtrNotNull(tc, rt_silver);
    CuAssertPtrNotNull(tc, rt_silver->itype);
    it_other = it_find("balm");
    CuAssertTrue(tc, it_other != it_luxury);

    u = test_create_unit(test_create_faction(), r);
    set_number(u, 2);
    unit_addorder(u, create_order(K_SELL, u->faction->locale, "%s %s",
        param_name(P_ANY, u->faction->locale),
        LOC(u->faction->locale, resourcename(it_other->rtype, 0))));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, "80 %s",
        LOC(u->faction->locale, resourcename(it_luxury->rtype, 0))));
    set_level(u, SK_TRADE, 4);
    test_set_item(u, rt_silver->itype, 5000);
    test_set_item(u, it_other, 1000);
    b = test_create_building(r, test_create_castle());
    b->size = 2;
    produce(r);
    CuAssertIntEquals(tc, 80, get_item(u, it_luxury));
    CuAssertIntEquals(tc, 1000, get_item(u, it_other));
    test_teardown();
}

static void test_buy_twice(CuTest *tc) {
    region * r;
    unit *u;
    building *b;
    int price, sold;
    const resource_type *rt_silver;
    const item_type *it_luxury;

    test_setup();
    setup_production();
    test_create_locale();
    r = setup_trade_region(tc, test_create_terrain("swamp", LAND_REGION));

    it_luxury = r_luxury(r);
    price = it_luxury->rtype->ltype->price;
    sold = max_luxuries_sold(r);
    CuAssertPtrNotNull(tc, it_luxury);
    rt_silver = get_resourcetype(R_SILVER);
    CuAssertPtrNotNull(tc, rt_silver);
    CuAssertPtrNotNull(tc, rt_silver->itype);

    u = test_create_unit(test_create_faction(), r);
    unit_addorder(u, create_order(K_BUY, u->faction->locale, "%d %s", sold, LOC(u->faction->locale, resourcename(it_luxury->rtype, 0))));
    unit_addorder(u, create_order(K_BUY, u->faction->locale, "%d %s", sold, LOC(u->faction->locale, resourcename(it_luxury->rtype, 0))));
    set_level(u, SK_TRADE, (1 + sold) / 5);
    test_set_item(u, rt_silver->itype, 1000);
    b = test_create_building(r, test_create_castle());
    b->size = 2;
    produce(r);
    CuAssertIntEquals(tc, 2 * sold, get_item(u, it_luxury));
    CuAssertIntEquals(tc, 1000 - (price * sold * 3), get_item(u, rt_silver->itype));
    test_teardown();
}

static void arm_unit(unit *u) {
    item_type *it_sword;

    it_sword = test_create_itemtype("sword");
    new_weapontype(it_sword, 0, frac_zero, NULL, 0, 0, 0, SK_MELEE);
    i_change(&u->items, it_sword, u->number);
    set_level(u, SK_MELEE, 1);
}

static void test_tax_cmd(CuTest *tc) {
    order *ord;
    faction *f;
    region *r;
    unit *u;
    item_type *silver;
    econ_request *taxorders = NULL;

    test_setup();
    setup_production();
    config_set("taxing.perlevel", "20");
    f = test_create_faction();
    r = test_create_plain(0, 0);
    assert(r && f);
    u = test_create_unit(f, r);

    ord = create_order(K_TAX, f->locale, "");
    assert(ord);

    tax_cmd(u, ord, &taxorders);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error48"));
    test_clear_messages(u->faction);

    silver = get_resourcetype(R_SILVER)->itype;

    arm_unit(u);

    tax_cmd(u, ord, &taxorders);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error_no_tax_skill"));
    test_clear_messages(u->faction);

    set_level(u, SK_TAXING, 1);
    tax_cmd(u, ord, &taxorders);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(u->faction->msgs, "error_no_tax_skill"));
    CuAssertPtrNotNull(tc, taxorders);

    rsetmoney(r, 11);
    expandtax(r, taxorders);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "income"));
    /* taxing is in multiples of 10 */
    CuAssertIntEquals(tc, 10, i_get(u->items, silver));
    test_clear_messages(u->faction);
    i_change(&u->items, silver, -i_get(u->items, silver));

    rsetmoney(r, 1000);
    taxorders = 0;
    tax_cmd(u, ord, &taxorders);
    expandtax(r, taxorders);
    CuAssertIntEquals(tc, 20, i_get(u->items, silver));
    test_clear_messages(u->faction);

    free_order(ord);
    test_teardown();
}

static void setup_economy(void) {
    mt_create_va(mt_new("recruit", NULL), "unit:unit", "region:region", "amount:int", "want:int", MT_NEW_END);
    mt_create_va(mt_new("maintenance", NULL), "unit:unit", "building:building", MT_NEW_END);
    mt_create_va(mt_new("maintenancefail", NULL), "unit:unit", "building:building", MT_NEW_END);
    mt_create_va(mt_new("maintenance_nowork", NULL), "building:building", MT_NEW_END);
    mt_create_va(mt_new("maintenance_noowner", NULL), "building:building", MT_NEW_END);
}

/** 
 * see https://bugs.eressea.de/view.php?id=2234
 */
static void test_maintain_buildings(CuTest* tc) {
    region *r;
    building *b;
    building_type *btype;
    unit *u;
    faction *f;
    maintenance *req;
    item_type *itype;

    test_setup();
    setup_economy();
    btype = test_create_buildingtype("Hort");
    btype->maxsize = 10;
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    b = test_create_building(r, btype);
    itype = test_create_itemtype("money");
    b->size = btype->maxsize;
    u_set_building(u, b);

    /* this building has no upkeep, it just works: */
    b->flags = 0;
    maintain_buildings(r);
    CuAssertIntEquals(tc, 0, fval(b, BLD_UNMAINTAINED));
    CuAssertPtrEquals(tc, NULL, f->msgs);
    CuAssertPtrEquals(tc, NULL, r->msgs);

    req = calloc(2, sizeof(maintenance));
    req[0].number = 100;
    req[0].rtype = itype->rtype;
    btype->maintenance = req;

    /* we cannot afford to pay: */
    b->flags = 0;
    maintain_buildings(r);
    CuAssertIntEquals(tc, BLD_UNMAINTAINED, fval(b, BLD_UNMAINTAINED));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "maintenancefail"));
    CuAssertPtrNotNull(tc, test_find_messagetype(r->msgs, "maintenance_nowork"));
    test_clear_messagelist(&f->msgs);
    test_clear_messagelist(&r->msgs);
    
    /* we can afford to pay: */
    i_change(&u->items, itype, 100);
    /* but we don't want to: */
    b->flags = BLD_DONTPAY;
    maintain_buildings(r);
    CuAssertIntEquals(tc, BLD_UNMAINTAINED, fval(b, BLD_UNMAINTAINED));
    CuAssertIntEquals(tc, 100, i_get(u->items, itype));
    CuAssertPtrNotNull(tc, test_find_messagetype(r->msgs, "maintenance_nowork"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "maintenance"));
    test_clear_messagelist(&f->msgs);
    test_clear_messagelist(&r->msgs);

    /* if we want to, items get used: */
    b->flags = 0;
    maintain_buildings(r);
    CuAssertIntEquals(tc, 0, fval(b, BLD_UNMAINTAINED));
    CuAssertIntEquals(tc, 0, i_get(u->items, itype));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(r->msgs, "maintenance_nowork"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "maintenance"));
    test_clear_messagelist(&f->msgs);

    /* this building has no owner, it doesn't work: */
    u_set_building(u, NULL);
    b->flags = 0;
    maintain_buildings(r);
    CuAssertIntEquals(tc, BLD_UNMAINTAINED, fval(b, BLD_UNMAINTAINED));
    CuAssertPtrEquals(tc, NULL, f->msgs);
    CuAssertPtrEquals(tc, NULL, r->msgs);
    test_clear_messagelist(&r->msgs);

    test_teardown();
}

static void test_maintainer_oversized(CuTest* tc) {
    region *r;
    building *b;
    building_type *btype;
    unit *u;
    faction *f;
    maintenance *req;
    item_type *itype;

    test_setup();
    setup_economy();
    btype = test_create_buildingtype("Hort");
    btype->maxsize = 10;
    btype->maxcapacity = 10;
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    set_number(u, btype->maxcapacity + 1);
    b = test_create_building(r, btype);
    itype = test_create_itemtype("money");
    b->size = btype->maxsize;
    u_set_building(u, b);

    req = calloc(2, sizeof(maintenance));
    req[0].number = 100;
    req[0].rtype = itype->rtype;
    btype->maintenance = req;

    /* we can afford to pay: */
    i_change(&u->items, itype, 100);
    b->flags = 0;
    maintain_buildings(r);
    CuAssertIntEquals(tc, 0, fval(b, BLD_UNMAINTAINED));
    CuAssertIntEquals(tc, 0, i_get(u->items, itype));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(r->msgs, "maintenance_nowork"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "maintenance"));
    test_clear_messagelist(&f->msgs);

    test_teardown();
}

static void test_maintain_buildings_curse(CuTest* tc) {
    region* r;
    building* b;
    building_type* btype;
    unit* u;
    faction* f;
    maintenance* req;
    item_type* itype;

    test_setup();
    setup_economy();
    btype = test_create_buildingtype("Hort");
    btype->maxsize = 10;
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    b = test_create_building(r, btype);
    itype = test_create_itemtype("money");
    b->size = btype->maxsize;
    u_set_building(u, b);
    req = calloc(2, sizeof(maintenance));
    req[0].number = 100;
    req[0].rtype = itype->rtype;
    i_change(&u->items, itype, 100);
    btype->maintenance = req;
    create_curse(u, &b->attribs, &ct_nocostbuilding,
        1, 1, .0, 0);

    /* this building magically needs no upkeep: */
    b->flags = 0;
    maintain_buildings(r);
    CuAssertIntEquals(tc, 0, fval(b, BLD_UNMAINTAINED));
    CuAssertIntEquals(tc, 100, i_get(u->items, itype));
    CuAssertPtrEquals(tc, NULL, f->msgs);
    CuAssertPtrEquals(tc, NULL, r->msgs);

    test_teardown();
}

static void test_recruit(CuTest *tc) {
    unit *u;
    faction *f;

    test_setup();
    setup_economy();
    f = test_create_faction();
    u = test_create_unit(f, test_create_plain(0, 0));
    CuAssertIntEquals(tc, 1, u->number);
    CuAssertIntEquals(tc, 1, f->num_people);
    CuAssertIntEquals(tc, 1, f->num_units);
    add_recruits(u, 1, 1, 1);
    CuAssertIntEquals(tc, 2, u->number);
    CuAssertIntEquals(tc, 2, f->num_people);
    CuAssertIntEquals(tc, 1, f->num_units);
    CuAssertPtrEquals(tc, u, f->units);
    CuAssertPtrEquals(tc, NULL, u->nextF);
    CuAssertPtrEquals(tc, NULL, u->prevF);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "recruit"));
    add_recruits(u, 1, 2, 2);
    CuAssertIntEquals(tc, 3, u->number);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "recruit"));
    test_clear_messages(f);
    add_recruits(u, 1, 1, 2);
    CuAssertIntEquals(tc, 4, u->number);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "recruit"));
    test_teardown();
}

static void test_recruit_insect(CuTest *tc) {
    unit *u;
    faction *f;
    message * msg;

    test_setup();
    test_create_calendar();
    test_create_terrain("desert", -1);
    f = test_create_faction_ex(test_create_race("insect"), NULL);
    u = test_create_unit(f, test_create_plain(0, 0));
    u->thisorder = create_order(K_RECRUIT, f->locale, "%d", 1);

    CuAssertIntEquals(tc, SEASON_AUTUMN, calendar_season(1083));
    msg = can_recruit(u, f->race, u->thisorder, 1083); /* Autumn */
    CuAssertPtrEquals(tc, NULL, msg);

    msg = can_recruit(u, f->race, u->thisorder, 1084); /* Insects, Winter */
    CuAssertPtrNotNull(tc, msg);
    msg_release(msg);

    u->flags |= UFL_WARMTH;
    msg = can_recruit(u, f->race, u->thisorder, 1084); /* Insects, potion, Winter */
    CuAssertPtrEquals(tc, NULL, msg);

    u->flags = 0;
    msg = can_recruit(u, NULL, u->thisorder, 1084); /* Other races, Winter */
    CuAssertPtrEquals(tc, NULL, msg);

    test_teardown();
}

static void test_income(CuTest *tc)
{
    race *rc;
    unit *u;
    test_setup();
    rc = test_create_race("nerd");
    u = test_create_unit(test_create_faction_ex(rc, NULL), test_create_plain(0, 0));
    CuAssertIntEquals(tc, 20, income(u));
    u->number = 5;
    CuAssertIntEquals(tc, 100, income(u));
    test_teardown();
}

static void test_modify_material(CuTest *tc) {
    unit *u;
    struct item_type *itype;
    resource_type *rtype;
    resource_mod *mod;

    test_setup();
    setup_production();

    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    set_level(u, SK_WEAPONSMITH, 1);

    /* the unit's race gets 2x savings on iron used to produce goods */
    itype = test_create_itemtype("iron");
    rtype = itype->rtype;
    mod = rtype->modifiers = calloc(2, sizeof(resource_mod));
    mod[0].type = RMT_USE_SAVE;
    mod[0].value = frac_make(2, 1);
    mod[0].race_mask = rc_mask(u_race(u));

    itype = test_create_itemtype("sword");
    make_item(u, itype, 1);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error_cannotmake"));
    CuAssertIntEquals(tc, 0, get_item(u, itype));
    test_clear_messages(u->faction);
    itype->construction = calloc(1, sizeof(construction));
    itype->construction->skill = SK_WEAPONSMITH;
    itype->construction->minskill = 1;
    itype->construction->maxsize = 1;
    itype->construction->reqsize = 1;
    itype->construction->materials = calloc(2, sizeof(requirement));
    itype->construction->materials[0].rtype = rtype;
    itype->construction->materials[0].number = 2;

    test_set_item(u, rtype->itype, 1); /* 1 iron should get us 1 sword */
    make_item(u, itype, 1);
    CuAssertIntEquals(tc, 1, get_item(u, itype));
    CuAssertIntEquals(tc, 0, get_item(u, rtype->itype));

    u_setrace(u, test_create_race("smurf"));
    test_set_item(u, rtype->itype, 2); /* 2 iron should be required now */
    make_item(u, itype, 1);
    CuAssertIntEquals(tc, 2, get_item(u, itype));
    CuAssertIntEquals(tc, 0, get_item(u, rtype->itype));

    test_teardown();
}

static void test_modify_skill(CuTest *tc) {
    unit *u;
    struct item_type *itype;
    /* building_type *btype; */
    resource_type *rtype;
    resource_mod *mod;

    test_setup();
    setup_production();

    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    set_level(u, SK_WEAPONSMITH, 1);

    itype = test_create_itemtype("iron");
    rtype = itype->rtype;

    itype = test_create_itemtype("sword");
    make_item(u, itype, 1);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error_cannotmake"));
    CuAssertIntEquals(tc, 0, get_item(u, itype));
    test_clear_messages(u->faction);
    itype->construction = calloc(1, sizeof(construction));
    itype->construction->skill = SK_WEAPONSMITH;
    itype->construction->minskill = 1;
    itype->construction->maxsize = -1;
    itype->construction->reqsize = 1;
    itype->construction->materials = malloc(2 * sizeof(requirement));
    itype->construction->materials[0].rtype = rtype;
    itype->construction->materials[0].number = 1;
    itype->construction->materials[1].number = 0;

    /* our race gets a +1 bonus to the item's production skill */
    mod = itype->rtype->modifiers = calloc(2, sizeof(resource_mod));
    mod[0].type = RMT_PROD_SKILL;
    mod[0].value.sa[0] = SK_WEAPONSMITH;
    mod[0].value.sa[1] = 1;
    mod[0].race_mask = rc_mask(u_race(u));

    test_set_item(u, rtype->itype, 2); /* 2 iron should get us 2 swords */
    make_item(u, itype, 2);
    CuAssertIntEquals(tc, 2, get_item(u, itype));
    CuAssertIntEquals(tc, 0, get_item(u, rtype->itype));

    mod[0].value.sa[0] = NOSKILL; /* match any skill */
    test_set_item(u, rtype->itype, 2);
    make_item(u, itype, 2);
    CuAssertIntEquals(tc, 4, get_item(u, itype));
    CuAssertIntEquals(tc, 0, get_item(u, rtype->itype));


    u_setrace(u, test_create_race("smurf"));
    test_set_item(u, rtype->itype, 2);
    make_item(u, itype, 1); /* only enough skill to make 1 now */
    CuAssertIntEquals(tc, 5, get_item(u, itype));
    CuAssertIntEquals(tc, 1, get_item(u, rtype->itype));

    test_teardown();
}


static void test_modify_production(CuTest *tc) {
    unit *u;
    struct item_type *itype;
    const struct resource_type *rt_silver;
    resource_type *rtype;
    double d = 0.6;

    test_setup();
    setup_production();

    /* make items from other items (turn silver to stone) */
    rt_silver = get_resourcetype(R_SILVER);
    itype = test_create_itemtype("stone");
    rtype = itype->rtype;
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    make_item(u, itype, 1);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error_cannotmake"));
    CuAssertIntEquals(tc, 0, get_item(u, itype));
    test_clear_messages(u->faction);
    itype->construction = calloc(1, sizeof(construction));
    itype->construction->skill = SK_ALCHEMY;
    itype->construction->minskill = 1;
    itype->construction->maxsize = 1;
    itype->construction->reqsize = 1;
    itype->construction->materials = malloc(2 * sizeof(requirement));
    itype->construction->materials[0].number = 1;
    itype->construction->materials[0].rtype = rt_silver;
    itype->construction->materials[1].number = 0;
    set_level(u, SK_ALCHEMY, 1);
    test_set_item(u, rt_silver->itype, 1);
    make_item(u, itype, 1);
    CuAssertIntEquals(tc, 1, get_item(u, itype));
    CuAssertIntEquals(tc, 0, get_item(u, rt_silver->itype));

    /* make level-based raw materials, no materials used in construction */
    free(itype->construction->materials);
    itype->construction->materials = NULL;
    rtype->flags |= RTF_LIMITED;
    rmt_create(rtype);
    add_resource(u->region, 1, 300, 150, rtype); /* there are 300 stones at level 1 */
    CuAssertIntEquals(tc, 300, region_getresource(u->region, rtype));
    set_level(u, SK_ALCHEMY, 10);

    make_item(u, itype, 10);
    split_allocations(u->region);
    CuAssertIntEquals(tc, 11, get_item(u, itype));
    CuAssertIntEquals(tc, 290, region_getresource(u->region, rtype)); /* used 10 stones to make 10 stones */

    rtype->modifiers = calloc(3, sizeof(resource_mod));
    rtype->modifiers[0].type = RMT_PROD_SAVE;
    rtype->modifiers[0].race_mask = rc_mask(u->_race);
    rtype->modifiers[0].value.sa[0] = (short)(0.5+100*d);
    rtype->modifiers[0].value.sa[1] = 100;
    rtype->modifiers[1].type = RMT_END;
    make_item(u, itype, 10);
    split_allocations(u->region);
    CuAssertIntEquals(tc, 21, get_item(u, itype));
    CuAssertIntEquals(tc, 284, region_getresource(u->region, rtype)); /* 60% saving = 6 stones make 10 stones */

    make_item(u, itype, 1);
    split_allocations(u->region);
    CuAssertIntEquals(tc, 22, get_item(u, itype));
    CuAssertIntEquals(tc, 283, region_getresource(u->region, rtype)); /* no free lunches */

    rtype->modifiers[0].value = frac_make(1, 2);
    make_item(u, itype, 6);
    split_allocations(u->region);
    CuAssertIntEquals(tc, 28, get_item(u, itype));
    CuAssertIntEquals(tc, 280, region_getresource(u->region, rtype)); /* 50% saving = 3 stones make 6 stones */

    rtype->modifiers[0].type = RMT_PROD_REQUIRE;
    rtype->modifiers[0].race_mask = 0;
    rtype->modifiers[0].btype = bt_get_or_create("mine");

    test_clear_messages(u->faction);
    make_item(u, itype, 10);
    CuAssertIntEquals(tc, 28, get_item(u, itype));
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "building_needed"));

    rtype->modifiers[0].type = RMT_PROD_REQUIRE;
    rtype->modifiers[0].race_mask = rc_mask(test_create_race("smurf"));
    rtype->modifiers[0].btype = NULL;

    test_clear_messages(u->faction);
    make_item(u, itype, 10);
    CuAssertIntEquals(tc, 28, get_item(u, itype));
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error117"));

    rtype->modifiers[1].type = RMT_PROD_REQUIRE;
    rtype->modifiers[1].race_mask = rc_mask(u_race(u));
    rtype->modifiers[1].btype = NULL;
    rtype->modifiers[2].type = RMT_END;

    test_clear_messages(u->faction);
    make_item(u, itype, 10);
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);
    split_allocations(u->region);
    CuAssertIntEquals(tc, 38, get_item(u, itype));

    test_teardown();
}

static void test_loot(CuTest *tc) {
    unit *u;
    faction *f;
    item_type *it_silver;

    test_setup();
    setup_production();
    mt_create_error(48); /* unit is unarmed */
    it_silver = test_create_silver();
    config_set("rules.enable_loot", "1");
    u = test_create_unit(f = test_create_faction(), test_create_plain(0, 0));
    u->thisorder = create_order(K_LOOT, f->locale, NULL);
    produce(u->region);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error48")); /* unit is unarmed */
    test_clear_messages(f);
    arm_unit(u);
    produce(u->region);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "income"));
    CuAssertIntEquals(tc, 2 * TAXFRACTION, i_get(u->items, it_silver));
    CuAssertIntEquals(tc, UFL_LONGACTION | UFL_NOTMOVING, fval(u, UFL_LONGACTION | UFL_NOTMOVING));
    test_teardown();
}

static void test_expand_production(CuTest *tc) {
    econ_request *orders = NULL;
    econ_request **results = NULL;
    region *r;
    unit *u;

    test_setup();
    (void) arraddnptr(orders, 1);
    orders->qty = 2;
    orders->unit = u = test_create_unit(test_create_faction(), r = test_create_plain(0, 0));

    u->n = 1; /* will be overwritten */
    CuAssertIntEquals(tc, 2, expand_production(r, orders, &results));
    CuAssertPtrNotNull(tc, results);
    CuAssertPtrEquals(tc, u, results[0]->unit);
    CuAssertPtrEquals(tc, u, results[1]->unit);
    CuAssertIntEquals(tc, 0, u->n);
    arrfree(orders);
    free(results);
    test_teardown();
}

static void test_destroy_road(CuTest* tc)
{
    region* r, * r2;
    faction* f;
    unit* u;
    order* ord;
    message* m;

    test_setup();
    mt_create_va(mt_new("destroy_road", NULL), "unit:unit", "from:region", "to:region", MT_NEW_END);
    r2 = test_create_region(1, 0, 0);
    r = test_create_plain(0, 0);
    rsetroad(r, D_EAST, 100);
    u = test_create_unit(f = test_create_faction(), r);
    u->orders = ord = create_order(K_DESTROY, f->locale, "%s %s", param_name(P_ROAD, f->locale), LOC(f->locale, directions[D_EAST]));

    CuAssertIntEquals(tc, 0, destroy_cmd(u, ord));
    CuAssertIntEquals(tc, 100, rroad(r, D_EAST));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "destroy_road"));

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
    test_teardown();
}

static unit* test_create_guard(region* r, faction* f, race* rc) {
    unit* ug;

    if (!rc) {
        rc = test_create_race("guardian");
        rc->flags |= RCF_UNARMEDGUARD;
    }
    if (!f) {
        f = test_create_faction_ex(rc, NULL);
    }
    ug = test_create_unit(f, r);
    setguard(ug, true);

    return ug;
}

static void test_destroy_road_guard(CuTest* tc)
{
    region* r;
    faction* f;
    unit* u, * ug;
    order* ord;

    test_setup();
    test_create_region(1, 0, 0);
    r = test_create_plain(0, 0);
    rsetroad(r, D_EAST, 100);
    ug = test_create_guard(r, 0, 0);
    u = test_create_unit(f = test_create_faction(), r);
    u->orders = ord = create_order(K_DESTROY, f->locale, "%s %s", param_name(P_ROAD, f->locale), LOC(f->locale, directions[D_EAST]));

    set_level(u, SK_ROAD_BUILDING, 1);
    CuAssertIntEquals(tc, 0, destroy_cmd(u, ord));
    CuAssertIntEquals(tc, 100, rroad(r, D_EAST));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error70"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "destroy_road"));

    test_clear_messages(f);
    setguard(ug, false);

    CuAssertIntEquals(tc, 0, destroy_cmd(u, ord));
    CuAssertIntEquals(tc, 99, rroad(r, D_EAST));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "error70"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "destroy_road"));

    test_teardown();
}

static void test_destroy_road_limit(CuTest* tc)
{
    region* r;
    faction* f;
    unit* u;
    order* ord;

    test_setup();
    test_create_region(1, 0, 0);
    r = test_create_plain(0, 0);
    rsetroad(r, D_EAST, 100);
    u = test_create_unit(f = test_create_faction(), r);
    u->orders = ord = create_order(K_DESTROY, f->locale, "1 %s %s",
        param_name(P_ROAD, f->locale), LOC(f->locale, directions[D_EAST]));

    set_level(u, SK_ROAD_BUILDING, 1);
    CuAssertIntEquals(tc, 0, destroy_cmd(u, ord));
    CuAssertIntEquals(tc, 99, rroad(r, D_EAST));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "destroy_road"));

    set_level(u, SK_ROAD_BUILDING, 4);
    CuAssertIntEquals(tc, 0, destroy_cmd(u, ord));
    CuAssertIntEquals(tc, 98, rroad(r, D_EAST));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "destroy_road"));

    test_teardown();
}

static void test_destroy_cmd(CuTest* tc) {
    unit* u;
    faction* f;

    test_setup();
    mt_create_error(138);
    u = test_create_unit(f = test_create_faction(), test_create_plain(0, 0));
    u->thisorder = create_order(K_DESTROY, f->locale, NULL);
    CuAssertIntEquals(tc, 138, destroy_cmd(u, u->thisorder));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error138"));
    test_teardown();
}

static void test_make_zero(CuTest* tc) {
    unit* u;
    faction* f;

    test_setup();
    u = test_create_unit(f = test_create_faction(), test_create_plain(0, 0));
    u->thisorder = create_order(K_MAKE, f->locale, "0 BURG");
    CuAssertIntEquals(tc, 0, make_cmd(u, u->thisorder));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error_cannotmake"));
    test_teardown();
}

static void test_entertain_fair(CuTest *tc) {
    unit *u1, *u2;
    region *r;
    faction *f;
    const item_type *itype;

    test_setup();
    init_resources();
    itype = it_find("money");
    f = test_create_faction();
    r = test_create_plain(0, 0);
    rsetmoney(r, ENTERTAINFRACTION * 1800);
    u1 = test_create_unit(f, r);
    scale_number(u1, 90);
    test_set_skill(u1, SK_ENTERTAINMENT, 1, 1);
    u1->thisorder = create_order(K_ENTERTAIN, f->locale, "1000", 0);
    u2 = test_create_unit(f, r);
    scale_number(u2, 10);
    test_set_skill(u2, SK_ENTERTAINMENT, 9, 1);
    u2->thisorder = create_order(K_ENTERTAIN, f->locale, "1000", 0);
    produce(r);
    CuAssertIntEquals(tc, 1800, i_get(u1->items, itype) + i_get(u2->items, itype));
    CuAssertIntEquals(tc, 900, i_get(u2->items, itype));
    CuAssertIntEquals(tc, 900, i_get(u1->items, itype));
    test_teardown();
}

CuSuite *get_economy_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_give_unit_cmd);
    SUITE_ADD_TEST(suite, test_give_control_cmd);
    SUITE_ADD_TEST(suite, test_give_control_building);
    SUITE_ADD_TEST(suite, test_give_control_ship);
    SUITE_ADD_TEST(suite, test_income);
    SUITE_ADD_TEST(suite, test_modify_production);
    SUITE_ADD_TEST(suite, test_modify_skill);
    SUITE_ADD_TEST(suite, test_modify_material);
    SUITE_ADD_TEST(suite, test_steal_okay);
    SUITE_ADD_TEST(suite, test_steal_ocean);
    SUITE_ADD_TEST(suite, test_steal_nosteal);
    SUITE_ADD_TEST(suite, test_normals_recruit);
    SUITE_ADD_TEST(suite, test_heroes_dont_recruit);
    SUITE_ADD_TEST(suite, test_tax_cmd);
    SUITE_ADD_TEST(suite, test_trade_is_long_action);
    SUITE_ADD_TEST(suite, test_forget_cmd);
    SUITE_ADD_TEST(suite, test_buy_cmd);
    SUITE_ADD_TEST(suite, test_buy_twice);
    SUITE_ADD_TEST(suite, test_buy_prices);
    SUITE_ADD_TEST(suite, test_buy_prices_split);
    SUITE_ADD_TEST(suite, test_buy_prices_rising);
    SUITE_ADD_TEST(suite, test_buy_before_sell);
    SUITE_ADD_TEST(suite, test_sell_over_demand);
    SUITE_ADD_TEST(suite, test_sell_all);
    SUITE_ADD_TEST(suite, test_sales_taxes);
    SUITE_ADD_TEST(suite, test_import_taxes);
    SUITE_ADD_TEST(suite, test_sell_nothing_message);
    SUITE_ADD_TEST(suite, test_trade_limits);
    SUITE_ADD_TEST(suite, test_trade_produceexp);
    SUITE_ADD_TEST(suite, test_buy_limits);
    SUITE_ADD_TEST(suite, test_buy_limited_funds);
    SUITE_ADD_TEST(suite, test_trade_needs_castle);
    SUITE_ADD_TEST(suite, test_trade_insect);
    SUITE_ADD_TEST(suite, test_maintain_buildings);
    SUITE_ADD_TEST(suite, test_maintainer_oversized);
    SUITE_ADD_TEST(suite, test_maintain_buildings_curse);
    SUITE_ADD_TEST(suite, test_recruit);
    SUITE_ADD_TEST(suite, test_recruit_insect);
    SUITE_ADD_TEST(suite, test_loot);
    SUITE_ADD_TEST(suite, test_expand_production);
    SUITE_ADD_TEST(suite, test_destroy_cmd);
    SUITE_ADD_TEST(suite, test_destroy_road);
    SUITE_ADD_TEST(suite, test_destroy_road_limit);
    SUITE_ADD_TEST(suite, test_destroy_road_guard);
    SUITE_ADD_TEST(suite, test_make_zero);
    SUITE_ADD_TEST(suite, test_entertain_fair);
    return suite;
}
