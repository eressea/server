#include "move.h"

#include "contact.h"
#include "lighthouse.h"
#include "attributes/follow.h"
#include "spells/regioncurse.h"

#include "kernel/attrib.h"
#include "kernel/ally.h"
#include "kernel/building.h"
#include "kernel/config.h"
#include "kernel/curse.h"
#include "kernel/faction.h"
#include "kernel/direction.h"          // for D_WEST, shortdirections, D_EAST, dire...
#include "kernel/region.h"
#include "kernel/ship.h"
#include "kernel/skill.h"       // for SK_SAILING, SK_RIDING
#include "kernel/terrain.h"
#include "kernel/item.h"
#include "kernel/unit.h"
#include "kernel/race.h"
#include "kernel/order.h"

#include "util/keyword.h"
#include "util/language.h"
#include "util/message.h"
#include "util/base36.h"
#include "util/param.h"
#include "util/parser.h"
#include "util/variant.h"       // for variant

#include <stb_ds.h>
#include <tests.h>
#include <CuTest.h>

#include <assert.h>
#include <stdlib.h>

static void setup_move(void) {
    mt_create_va(mt_new("travel", NULL),
        "unit:unit", "start:region", "end:region", "mode:int", "regions:regions", MT_NEW_END);
    mt_create_va(mt_new("moveblocked", NULL),
        "unit:unit", "direction:int", MT_NEW_END);
    mt_create_va(mt_new("enterfail", NULL),
        "unit:unit", "region:region", MT_NEW_END);
}

static void test_ship_not_allowed_in_coast(CuTest * tc)
{
    region *r1, *r2;
    ship * sh;
    terrain_type *ttype;
    ship_type *stype;

    test_setup();
    ttype = test_create_terrain("glacier", LAND_REGION | ARCTIC_REGION | WALK_INTO);
    stype = test_create_shiptype("derp");
    arrsetlen(stype->coasts, 1);

    r1 = test_create_region(0, 0, ttype);
    r2 = test_create_ocean(1, 0);
    sh = test_create_ship(0, stype);

    CuAssertIntEquals(tc, SA_ALLOWED, check_ship_allowed(sh, r2, NULL));
    CuAssertIntEquals(tc, SA_DENIED, check_ship_allowed(sh, r1, NULL));
    stype->coasts[0] = ttype;
    CuAssertIntEquals(tc, SA_ALLOWED, check_ship_allowed(sh, r1, NULL));
    test_teardown();
}

typedef struct move_fixture {
    struct region *r;
    struct ship *sh;
    struct terrain_type* ttype;
    struct ship_type* stype;
    struct building * b;
    struct unit *u;
} move_fixture;

static void setup_harbor(move_fixture *mf, region *r_ship) {
    region *r;
    ship * sh;
    building_type * btype;
    building * b;
    unit *u;

    mf->ttype = test_create_terrain("glacier", LAND_REGION | ARCTIC_REGION | WALK_INTO);
    btype = test_create_buildingtype("harbour");

    r = test_create_region(0, 0, mf->ttype);
    b = test_create_building(r, btype);

    mf->stype = test_create_shiptype("kayak");
    if (!r_ship) {
        r_ship = r;
    }
    sh = test_create_ship(r_ship, mf->stype);
    u = test_create_unit(test_create_faction(), r_ship);
    set_level(u, SK_SAILING, sh->type->sumskill);
    u->ship = sh;
    ship_set_owner(u);

    mf->r = r;
    mf->u = u;
    mf->sh = sh;
    mf->b = b;
}

static void test_ship_allowed_coast_ignores_harbor(CuTest* tc)
{
    move_fixture mf;
    unit* u;

    test_setup();
    setup_harbor(&mf, NULL);

    /* ship cannot sail into a glacier, so the harbor gets used: */
    CuAssertIntEquals(tc, SA_HARBOUR_ALLOWED, check_ship_allowed(mf.sh, mf.r, mf.b));

    /* now the harbot belongs to someone who is not our ally: */
    u = test_create_unit(test_create_faction(), mf.r);
    u->building = mf.b;
    building_set_owner(u);

    /* ship cannot sail in becasue of the harbor: */
    CuAssertIntEquals(tc, SA_HARBOUR_DENIED, check_ship_allowed(mf.sh, mf.r, mf.b));

    /* make it so our ship can enter glaciers: */
    mf.stype->coasts[0] = mf.ttype;
    /* ship cannot sail in becasue of the harbor: */
    CuAssertIntEquals(tc, SA_ALLOWED, check_ship_allowed(mf.sh, mf.r, mf.b));

    test_teardown();
}

static void test_ship_allowed_without_harbormaster(CuTest * tc)
{
    move_fixture mf;

    test_setup();
    setup_harbor(&mf, NULL);

    CuAssertIntEquals(tc, SA_HARBOUR_ALLOWED, check_ship_allowed(mf.sh, mf.r, mf.b));
    test_teardown();
}

static void test_ship_blocked_by_harbormaster(CuTest * tc) {
    unit *u;
    move_fixture mf;

    test_setup();
    setup_harbor(&mf, NULL);

    u = test_create_unit(test_create_faction(), mf.r);
    u->building = mf.b;
    building_set_owner(u);

    CuAssertIntEquals_Msg(tc, "harbor master must contact ship", SA_HARBOUR_DENIED, check_ship_allowed(mf.sh, mf.r, mf.b));
    test_teardown();
}

static void test_ship_blocked_by_unpaid_harbor(CuTest * tc) {
    unit *u;
    move_fixture mf;

    test_setup();
    setup_harbor(&mf, NULL);

    u = test_create_unit(test_create_faction(), mf.r);
    u->building = mf.b;
    u->building->flags |= BLD_UNMAINTAINED;
    building_set_owner(u);

    CuAssertIntEquals_Msg(tc, "unmaintained harbor", SA_HARBOUR_DISABLED, check_ship_allowed(mf.sh, mf.r, mf.b));
    test_teardown();
}

static void test_ship_has_harbormaster_contact(CuTest * tc) {
    unit *u;
    move_fixture mf;

    test_setup();
    setup_harbor(&mf, NULL);

    u = test_create_unit(test_create_faction(), mf.r);
    u->building = mf.b;
    building_set_owner(u);
    contact_unit(mf.b->_owner, mf.sh->_owner);

    CuAssertIntEquals(tc, SA_HARBOUR_ALLOWED, check_ship_allowed(mf.sh, mf.r, mf.b));
    test_teardown();
}

static void test_ship_has_harbormaster_same_faction(CuTest * tc) {
    unit *u;
    move_fixture mf;

    test_setup();
    setup_harbor(&mf, NULL);

    u = test_create_unit(mf.u->faction, mf.r);
    u->building = mf.b;
    building_set_owner(u);

    CuAssertIntEquals(tc, SA_HARBOUR_ALLOWED, check_ship_allowed(mf.sh, mf.r, mf.b));
    test_teardown();
}

static void test_ship_has_harbormaster_ally(CuTest * tc) {
    unit *u;
    move_fixture mf;

    test_setup();
    setup_harbor(&mf, NULL);

    u = test_create_unit(test_create_faction(), mf.r);
    u->building = mf.b;
    building_set_owner(u);
    ally_set(&u->faction->allies, mf.u->faction, HELP_GUARD);

    CuAssertIntEquals(tc, SA_HARBOUR_ALLOWED, check_ship_allowed(mf.sh, mf.r, mf.b));
    test_teardown();
}

static void test_ship_allowed_insect(CuTest * tc)
{
    region *rg, *ro;
    ship * sh;
    terrain_type *gtype, *otype;
    ship_type *stype;
    faction *fi, *fh;
    unit *ui, *uh;
    building_type *btype;
    building *b;

    test_setup();
    gtype = test_create_terrain("glacier", LAND_REGION | ARCTIC_REGION | WALK_INTO);
    otype = test_create_terrain("ocean", SEA_REGION);
    stype = test_create_shiptype("derp");

    rg = test_create_region(1, 0, gtype);
    ro = test_create_region(4, 0, otype);
    sh = test_create_ship(0, stype);
    sh->region = ro;

    fi = test_create_faction_ex(test_create_race("insect"), NULL);
    ui = test_create_unit(fi, ro);
    ui->ship = sh;
    ship_set_owner(ui);

    /* coast takes precedence over insect */
    CuAssertIntEquals(tc, SA_ALLOWED, check_ship_allowed(sh, ro, NULL));
    CuAssertIntEquals_Msg(tc, "no-coast trumps insect as reason", SA_DENIED, check_ship_allowed(sh, rg, NULL));
    stype->coasts[0] = gtype;
    CuAssertIntEquals_Msg(tc, "insect", SA_INSECT_DENIED, check_ship_allowed(sh, rg, NULL));

    /* harbour does not beat insect */
    btype = test_create_buildingtype("harbour");
    b = test_create_building(rg, btype);
    uh = test_create_unit(fi, rg);
    uh->building = b;
    building_set_owner(uh);

    CuAssertIntEquals(tc, SA_INSECT_DENIED, check_ship_allowed(sh, rg, b));

    /* insect passenger can enter */
    fh = test_create_faction_ex(test_create_race("human"), NULL);
    uh = test_create_unit(fh, test_create_plain(0, 0));

    uh->ship = sh;
    ship_set_owner(uh);

    CuAssertIntEquals_Msg(tc, "insect passenger okay", SA_ALLOWED, check_ship_allowed(sh, rg, b));
    test_teardown();
}


static void test_walkingcapacity(CuTest *tc) {
    unit *u;
    int capacity;
    const struct item_type *itype;
    capacities cap;

    test_setup();
    init_resources();

    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    capacity = u->number * (u->_race->capacity + u->_race->weight);
    CuAssertIntEquals(tc, capacity, walkingcapacity(u, NULL));
    scale_number(u, 2);
    capacity = u->number * (u->_race->capacity + u->_race->weight);
    CuAssertIntEquals(tc, capacity, walkingcapacity(u, NULL));

    itype = it_find("horse");
    assert(itype);
    i_change(&u->items, itype, 1);
    capacity += itype->capacity;
    get_transporters(u->items, &cap);
    CuAssertIntEquals(tc, capacity, walkingcapacity(u, &cap));
    i_change(&u->items, itype, 1);
    capacity += itype->capacity;
    get_transporters(u->items, &cap);
    CuAssertIntEquals(tc, capacity, walkingcapacity(u, &cap));

    itype = test_create_itemtype("cart");
    assert(itype);
    i_change(&u->items, itype, 1);
    get_transporters(u->items, &cap);
    CuAssertIntEquals(tc, capacity, walkingcapacity(u, &cap));
    set_level(u, SK_RIDING, 1);
    capacity += itype->capacity;
    CuAssertIntEquals(tc, capacity, walkingcapacity(u, &cap));

    itype = test_create_itemtype("trollbelt");
    assert(itype);
    i_change(&u->items, itype, 1);
    get_transporters(u->items, &cap);
    CuAssertIntEquals(tc, capacity + (STRENGTHMULTIPLIER-1) * u->_race->capacity, walkingcapacity(u, &cap));
    config_set("rules.trollbelt.multiplier", "5");
    CuAssertIntEquals(tc, capacity + 4 * u->_race->capacity, walkingcapacity(u, &cap));

    test_teardown();
}

static void test_horses_pull_carts(CuTest *tc)
{
    unit *u;
    region *r1, *r2, *r3;
    const struct item_type *it_horse;
    const struct item_type *it_cart;
    const struct item_type *it_silver;
    int capacity = 0;
    const char *dir;

    test_setup();
    it_horse = test_create_horse();
    it_cart = test_create_cart();
    it_silver = test_create_silver();
    r1 = test_create_plain(0, 0);
    r2 = test_create_plain(1, 0);
    r3 = test_create_plain(2, 0);
    u = test_create_unit(test_create_faction(), r1);
    scale_number(u, 20);
    set_level(u, SK_RIDING, 1);
    i_change(&u->items, it_horse, 20);
    capacity += (it_horse->capacity - it_horse->weight) * 20;
    i_change(&u->items, it_cart, 10);
    capacity += (it_cart->capacity - it_cart->weight) * 10;
    capacity -= u->number * u->_race->weight;
    i_change(&u->items, it_silver, capacity);

    dir = LOC(u->faction->locale, directions[D_EAST]);
    u->thisorder = create_order(K_MOVE, u->faction->locale, "%s %s", dir, dir);
    movement();
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "travel"));
    CuAssertPtrEquals(tc, r3, u->region);
    test_clear_messages(u->faction);

    u->flags = 0;
    u->thisorder = create_order(K_MOVE, u->faction->locale,
        LOC(u->faction->locale, directions[D_WEST]));
    // make too heavy to ride:
    i_change(&u->items, it_silver, (u->_race->weight + u->_race->capacity) * u->number);
    movement();
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "travel"));
    CuAssertPtrEquals(tc, r2, u->region);
    test_clear_messages(u->faction);

    u->flags = 0;
    u->thisorder = create_order(K_MOVE, u->faction->locale,
        LOC(u->faction->locale, directions[D_WEST]));
    // make too heavy for walking:
    i_change(&u->items, it_silver, 1);
    movement();
    CuAssertPtrEquals(tc, r2, u->region);
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "error57"));
    test_teardown();
}

static void test_trolls_pull_carts(CuTest *tc)
{
    unit *u;
    race *rc;
    region *r1, *r2;
    const struct item_type *it_cart;
    const struct item_type *it_silver;
    int capacity = 0;

    test_setup();
    rc = test_create_race("troll");
    rc->capacity = 1080;
    rc->weight = 2000;
    it_cart = test_create_cart();
    it_silver = test_create_silver();
    r1 = test_create_plain(0, 0);
    r2 = test_create_plain(1, 0);
    u = test_create_unit(test_create_faction_ex(rc, NULL), r1);
    scale_number(u, 20);
    capacity = u->_race->capacity * u->number;
    i_change(&u->items, it_cart, 5);
    capacity += (it_cart->capacity - it_cart->weight) * 5;
    i_change(&u->items, it_silver, capacity);

    u->thisorder = create_order(K_MOVE, u->faction->locale,
        LOC(u->faction->locale, directions[D_EAST]));
    movement();
    CuAssertPtrEquals(tc, r2, u->region);
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "travel"));

    u->flags = 0;
    free_order(u->thisorder);
    u->thisorder = create_order(K_MOVE, u->faction->locale,
        LOC(u->faction->locale, directions[D_WEST]));
    i_change(&u->items, it_silver, 1); // too heavy, cannot move
    movement();
    CuAssertPtrEquals(tc, r2, u->region);
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "error57"));
    test_teardown();
}

static void test_ship_trails(CuTest *tc) {
    ship *sh;
    region *r1, *r2, *r3;
    terrain_type *otype;
    region_list *route = NULL;

    test_setup();
    otype = test_create_terrain("ocean", SEA_REGION);
    r1 = test_create_region(0, 0, otype);
    r2 = test_create_region(1, 0, otype);
    r3 = test_create_region(2, 0, otype);
    sh = test_create_ship(r1, NULL);
    move_ship(sh, r1, r3, NULL);
    CuAssertPtrEquals(tc, r3, sh->region);
    CuAssertPtrEquals(tc, sh, r3->ships);
    CuAssertPtrEquals(tc, NULL, r1->ships);
    CuAssertPtrEquals(tc, NULL, a_find(r1->attribs, &at_shiptrail));
    CuAssertPtrEquals(tc, NULL, a_find(r3->attribs, &at_shiptrail));
    add_regionlist(&route, r3);
    add_regionlist(&route, r2);
    move_ship(sh, r3, r1, route);
    CuAssertPtrEquals(tc, r1, sh->region);
    CuAssertPtrEquals(tc, sh, r1->ships);
    CuAssertPtrEquals(tc, NULL, r3->ships);
    CuAssertPtrEquals(tc, NULL, a_find(r1->attribs, &at_shiptrail));
    CuAssertPtrNotNull(tc, a_find(r2->attribs, &at_shiptrail));
    CuAssertPtrNotNull(tc, a_find(r3->attribs, &at_shiptrail));
    free_regionlist(route);
    test_teardown();
}

static void test_age_trails(CuTest *tc) {
    region_list *route = NULL;
    region *r1, *r2;
    ship *sh;

    test_setup();
    r1 = test_create_plain(0, 0);
    r2 = test_create_region(1, 0, NULL);
    sh = test_create_ship(r1, NULL);
    add_regionlist(&route, r1);
    add_regionlist(&route, r2);
    move_ship(sh, r1, r2, route);

    CuAssertPtrNotNull(tc, r1->attribs);
    a_age(&r1->attribs, r1);
    CuAssertPtrNotNull(tc, r1->attribs);
    a_age(&r1->attribs, r1);
    CuAssertPtrEquals(tc, NULL, r1->attribs);
    free_regionlist(route);
    test_teardown();
}

struct drift_fixture {
    faction *f;
    region *r;
    unit *u;
    terrain_type *t_ocean;
    ship_type *st_boat;
    ship *sh;

};

void setup_drift (struct drift_fixture *fix) {
    test_create_locale();
    config_set_int("rules.ship.storms", 0);

    fix->st_boat = test_create_shiptype("boat");
    fix->st_boat->cabins = 20000;

    test_create_ocean(0, 0);
    fix->u = test_create_unit(fix->f = test_create_faction(), fix->r = test_create_ocean(-1, 0));
    assert(fix->r && fix->u && fix->f);
    set_level(fix->u, SK_SAILING, fix->st_boat->sumskill);
    u_set_ship(fix->u, fix->sh = test_create_ship(fix->u->region, fix->st_boat));
    assert(fix->sh);

    mt_create_va(mt_new("sink_msg", NULL),
        "ship:ship", "region:region", MT_NEW_END);

    mt_create_va(mt_new("ship_drift", NULL),
        "ship:ship", "dir:int", MT_NEW_END);
    mt_create_va(mt_new("ship_drift_nocrew", NULL),
        "ship:ship", "dir:int", MT_NEW_END);
    mt_create_va(mt_new("ship_drift_overload", NULL),
        "ship:ship", "dir:int", MT_NEW_END);
    mt_create_va(mt_new("shipsink", NULL),
        "ship:ship", MT_NEW_END);
    mt_create_va(mt_new("massive_overload", NULL),
        "ship:ship", MT_NEW_END);
}

static void test_ship_no_overload(CuTest *tc) {
    struct drift_fixture fix;

    test_setup();
    setup_drift(&fix);

    fix.u->number = 2;
    movement();
    CuAssertPtrEquals(tc, fix.u->region, findregion(-1,0));
    CuAssertIntEquals(tc, 0, fix.sh->damage);

    test_teardown();
}

static void test_ship_empty(CuTest *tc) {
    struct drift_fixture fix;

    test_setup();
    setup_drift(&fix);
    fix.u->ship = NULL;
    ship_update_owner(fix.sh);

    movement();
    CuAssertPtrEquals(tc, fix.sh->region, findregion(0, 0));
    CuAssertIntEquals(tc, SF_DRIFTED, fix.sh->flags & SF_DRIFTED);
    CuAssertIntEquals(tc, 2, ship_damage_percent(fix.sh));

    test_teardown();
}

static void test_ship_no_crew(CuTest *tc) {
    struct drift_fixture fix;

    test_setup();
    setup_drift(&fix);
    set_level(fix.u, SK_SAILING, 0);

    movement();
    CuAssertPtrEquals(tc, fix.sh->region, findregion(0, 0));
    CuAssertIntEquals(tc, SF_DRIFTED, fix.sh->flags & SF_DRIFTED);
    CuAssertIntEquals(tc, 2, ship_damage_percent(fix.sh));
    CuAssertPtrNotNull(tc, test_find_messagetype(fix.f->msgs, "ship_drift_nocrew"));

    test_teardown();
}

static void test_no_drift_damage(CuTest *tc) {
    struct drift_fixture fix;

    test_setup();
    setup_drift(&fix);
    fix.u->ship = NULL;
    ship_update_owner(fix.sh);

    config_set("rules.ship.damage_drift", "0.0");
    movement();
    CuAssertPtrEquals(tc, fix.sh->region, findregion(0, 0));
    CuAssertIntEquals(tc, SF_DRIFTED, fix.sh->flags & SF_DRIFTED);
    CuAssertIntEquals(tc, 0, ship_damage_percent(fix.sh));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(fix.f->msgs, "ship_drift"));

    test_teardown();
}

static void test_ship_normal_overload(CuTest *tc) {
    struct drift_fixture fix;

    test_setup();
    setup_drift(&fix);

    fix.u->number = 21;
    movement();
    CuAssertPtrEquals(tc, fix.u->region, findregion(0, 0));
    CuAssertIntEquals(tc, SF_DRIFTED, fix.sh->flags & SF_DRIFTED);
    CuAssertIntEquals(tc, 2, ship_damage_percent(fix.sh));
    CuAssertPtrNotNull(tc, test_find_messagetype(fix.f->msgs, "ship_drift_overload"));

    test_teardown();
}

static void test_ship_big_overload(CuTest *tc) {
    struct drift_fixture fix;

    test_setup();
    setup_drift(&fix);

    fix.u->number = 22;
    movement();
    CuAssertPtrEquals(tc, fix.u->region, findregion(-1, 0));
    CuAssertIntEquals(tc, SF_DRIFTED, fix.sh->flags & SF_DRIFTED);
    CuAssertIntEquals(tc, 5, ship_damage_percent(fix.sh));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(fix.f->msgs, "ship_drift_overload"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(fix.f->msgs, "ship_drift"));
    CuAssertPtrNotNull(tc, test_find_messagetype(fix.f->msgs, "massive_overload"));

    test_teardown();
}

static void test_ship_small_overload(CuTest *tc) {
    struct drift_fixture fix;

    test_setup();
    setup_drift(&fix);

    fix.u->number = 21;
    damage_ship(fix.sh, .80);
    movement();
    CuAssertPtrEquals(tc, fix.u->region, findregion(0, 0));
    CuAssertIntEquals(tc, SF_DRIFTED, fix.sh->flags & SF_DRIFTED);
    CuAssertIntEquals(tc, 82, ship_damage_percent(fix.sh));
    CuAssertPtrNotNull(tc, test_find_messagetype(fix.f->msgs, "ship_drift_overload"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(fix.f->msgs, "massive_overload"));

    test_teardown();
}

static void test_ship_ridiculous_overload(CuTest *tc) {
    struct drift_fixture fix;

    test_setup();
    setup_drift(&fix);

    fix.u->number = 500;
    movement();
    CuAssertPtrEquals(tc, fix.u->region, findregion(-1, 0));
    CuAssertIntEquals(tc, SF_DRIFTED, fix.sh->flags & SF_DRIFTED);
    CuAssertIntEquals(tc, 37, ship_damage_percent(fix.sh));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(fix.f->msgs, "ship_drift_overload"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(fix.f->msgs, "ship_drift"));
    CuAssertPtrNotNull(tc, test_find_messagetype(fix.f->msgs, "massive_overload"));

    test_teardown();
}

static void test_ship_ridiculous_overload_no_captain(CuTest *tc) {
    struct drift_fixture fix;

    test_setup();
    setup_drift(&fix);
    set_level(fix.u, SK_SAILING, 0);

    fix.u->number = 500;
    movement();
    CuAssertIntEquals(tc, 37, ship_damage_percent(fix.sh));
    CuAssertPtrNotNull(tc, test_find_messagetype(fix.f->msgs, "massive_overload"));

    test_teardown();
}

static void test_ship_ridiculous_overload_bad(CuTest *tc) {
    struct drift_fixture fix;

    test_setup();
    setup_drift(&fix);

    fix.u->number = 500;
    config_set("rules.ship.overload.damage.max", "1");
    movement();
    CuAssertTrue(tc, ship_damage_percent(fix.sh) > 99);
    CuAssertPtrNotNull(tc, test_find_messagetype(fix.f->msgs, "massive_overload"));
    CuAssertPtrEquals(tc, NULL, fix.sh->region);
    CuAssertPtrNotNull(tc, test_find_messagetype(fix.f->msgs, "shipsink"));
    test_teardown();
}

extern double damage_overload(double overload, double damage_max);

static void test_ship_damage_overload(CuTest *tc) {
    double damage_max = 0.37;
    CuAssertDblEquals(tc, 0.0, damage_overload(1, damage_max), ASSERT_DBL_DELTA);
    CuAssertDblEquals(tc, 0.05, damage_overload(1.1, damage_max), ASSERT_DBL_DELTA);
    CuAssertDblEquals(tc, 0.05, damage_overload(1.5, damage_max), ASSERT_DBL_DELTA);
    CuAssertDblEquals(tc, 0.21, damage_overload(3.25, damage_max), ASSERT_DBL_DELTA);
    CuAssertDblEquals(tc, 0.37, damage_overload(5, damage_max), ASSERT_DBL_DELTA);
    CuAssertDblEquals(tc, 0.0, damage_overload(5, 0.0), ASSERT_DBL_DELTA);
}

static void test_follow_bad_target(CuTest* tc) {
    unit* u, * u2;
    faction* f;

    test_setup();
    mt_create_va(mt_new("error330", NULL),
        "unit:unit", "region:region", "command:order", MT_NEW_END);
    mt_create_va(mt_new("error146", NULL),
        "unit:unit", "region:region", "command:order", MT_NEW_END);

    f = test_create_faction();
    u = test_create_unit(f, test_create_ocean(0, 0));
    u2 = test_create_unit(f, test_create_ocean(0, 0));
    u->ship = test_create_ship(u->region, NULL);
    unit_addorder(u, create_order(K_FOLLOW, f->locale, "%s %s", param_name(P_UNIT, f->locale), itoa36(u2->no)));
    unit_addorder(u2, create_order(K_FOLLOW, f->locale, "%s %s", param_name(P_SHIP, f->locale), itoa36(u->ship->no)));
    follow_cmds(u);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error330"));
    CuAssertIntEquals(tc, 0, fval(u, UFL_NOTMOVING | UFL_LONGACTION));
    test_clear_messages(f);

    follow_cmds(u2);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error146"));
    CuAssertIntEquals(tc, 0, fval(u2, UFL_NOTMOVING | UFL_LONGACTION));
    test_clear_messages(f);

    test_teardown();
}

static void test_follow_unit(CuTest* tc) {
    unit *u, *u2;
    order *ord;
    faction *f;
    region *r;

    test_setup();

    f = test_create_faction();
    u = test_create_unit(f, test_create_plain(0, 0));
    r = test_create_plain(1, 0);
    u2 = test_create_unit(test_create_faction(), u->region);
    ord = create_order(K_MOVE, f->locale, shortdirections[D_EAST] + 4);
    unit_addorder(u2, ord);
    u2->thisorder = copy_order(ord);
    ord = create_order(K_FOLLOW, f->locale, "%s %s",
        param_name(P_UNIT, f->locale), itoa36(u2->no));
    unit_addorder(u, ord);
    u->thisorder = copy_order(ord);

    /* Verfolger m�ssen ihre Ziele finden, ehe diese sich bewegen */
    follow_cmds(u);
    CuAssertPtrEquals(tc, &at_follow, (void *)u->attribs->type);
    CuAssertPtrEquals(tc, u2, u->attribs->data.v);
    CuAssertPtrEquals(tc, NULL, u->thisorder);

    movement();
    CuAssertPtrEquals(tc, r, u2->region);
    CuAssertIntEquals(tc, UFL_NOTMOVING | UFL_LONGACTION, fval(u2, UFL_NOTMOVING | UFL_LONGACTION));

    CuAssertPtrEquals(tc, r, u->region);
    CuAssertPtrNotNull(tc, test_find_messagetype(u2->faction->msgs, "followdetect"));
    CuAssertIntEquals(tc, UFL_FOLLOWING | UFL_LONGACTION, fval(u, UFL_FOLLOWING | UFL_LONGACTION));

    test_teardown();
}

static void test_follow_unit_self(CuTest *tc) {
    unit *u;
    order *ord;
    faction *f;

    test_setup();
    mt_create_va(mt_new("followfail", NULL),
        "unit:unit", "follower:unit", MT_NEW_END);

    f = test_create_faction();
    u = test_create_unit(f, test_create_plain(0, 0));
    ord = create_order(K_FOLLOW, f->locale, "%s %s",
        param_name(P_UNIT, f->locale), itoa36(u->no));
    unit_addorder(u, ord);
    follow_cmds(u);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "followfail"));
    CuAssertIntEquals(tc, 0, fval(u, UFL_FOLLOWING | UFL_LONGACTION));
    test_teardown();
}

typedef struct traveldir {
    int no;
    direction_t dir;
    int age;
} traveldir;

static void test_follow_ship_msg(CuTest * tc) {
    region *r;
    ship * sh;
    faction *f;
    unit *u;
    const ship_type *stype;
    message *msg;
    order *ord;
    traveldir *td = NULL;
    attrib *a;
    void *p;
    
    test_setup();
    init_resources();

    f = test_create_faction();
    r = test_create_plain(0, 0);
    test_create_ocean(-1, 1); /* D_NORTHWEST */

    stype = st_find("boat");
    sh = test_create_ship(r, stype);
    u = test_create_unit(f, r);
    u->ship = sh;
    ship_set_owner(u);
    set_level(u, SK_SAILING, 10);

    ord = create_order(K_FOLLOW, f->locale, "SHIP 2");
    assert(ord);
    unit_addorder(u, ord);

    set_money(u, 999999); /* overloaded ship */

    a = a_add(&(r->attribs), a_new(&at_shiptrail));
    td = (traveldir *)a->data.v;
    td->no = 2;
    td->dir = D_NORTHWEST;
    td->age = 2;

    mt_create_va(mt_new("error18", NULL),
        "unit:unit", "region:region", "command:order", MT_NEW_END);

    init_order(ord, NULL);
    skip_token();

    follow_ship(u, ord);

    CuAssertPtrEquals(tc, r, u->region);
    CuAssertPtrNotNull(tc, msg = test_find_messagetype(u->faction->msgs, "error18"));
    p = msg->parameters[2].v;
    CuAssertPtrNotNull(tc, p);
    CuAssertIntEquals(tc, K_FOLLOW, getkeyword((order *)p));

    test_teardown();
}

static void test_drifting_ships(CuTest *tc) {
    ship *sh;
    region *r;
    terrain_type *t_ocean, *t_plain;
    ship_type *st_boat;

    test_setup();
    t_ocean = test_create_terrain("ocean", SEA_REGION);
    t_plain = test_create_terrain("plain", LAND_REGION);
    r = test_create_region(0, 0, t_ocean);
    test_create_region(1, 0, t_ocean);
    st_boat = test_create_shiptype("boat");
    sh = test_create_ship(r, st_boat);
    CuAssertIntEquals(tc, D_EAST, drift_target(sh));
    test_create_region(-1, 0, t_plain);
    CuAssertIntEquals(tc, D_WEST, drift_target(sh));
    test_teardown();
}

static void test_ship_leave_trail(CuTest *tc) {
    ship *s1, *s2;
    region *r1, *r2;
    terrain_type *t_ocean;
    ship_type *st_boat;
    region_list *route = NULL;

    test_setup();
    t_ocean = test_create_terrain("ocean", SEA_REGION);
    r1 = test_create_region(0, 0, t_ocean);
    add_regionlist(&route, test_create_region(2, 0, t_ocean));
    add_regionlist(&route, r2 = test_create_region(1, 0, t_ocean));
    st_boat = test_create_shiptype("boat");
    s1 = test_create_ship(r1, st_boat);
    s2 = test_create_ship(r1, st_boat);
    leave_trail(s1, r1, route);
    a_add(&r1->attribs, a_new(&at_lighthouse));
    leave_trail(s2, r1, route);
    a_add(&r2->attribs, a_new(&at_lighthouse));
    CuAssertPtrEquals(tc, &at_shiptrail, (void *)r1->attribs->type);
    CuAssertPtrEquals(tc, &at_shiptrail, (void *)r1->attribs->next->type);
    CuAssertPtrEquals(tc, &at_lighthouse, (void *)r1->attribs->next->next->type);
    CuAssertPtrEquals(tc, &at_shiptrail, (void *)r2->attribs->type);
    CuAssertPtrEquals(tc, &at_shiptrail, (void *)r2->attribs->next->type);
    CuAssertPtrEquals(tc, &at_lighthouse, (void *)r2->attribs->next->next->type);
    free_regionlist(route);
    test_teardown();
}

static void test_is_transporting(CuTest* tc) {
    unit *u1, *u2;
    test_setup();
    u1 = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u2 = test_create_unit(u1->faction, test_create_plain(1, 1));
    unit_addorder(u1, create_order(K_TRANSPORT, u1->faction->locale, itoa36(u2->no)));
    CuAssertTrue(tc, !is_transporting(u1, u2));

    move_unit(u2, u1->region, NULL);
    CuAssertTrue(tc, is_transporting(u1, u2));
    test_teardown();
}

static void test_movement_speed(CuTest *tc) {
    unit * u;
    race * rc;
    const struct item_type *it_horse;
    const struct item_type *it_cart;
    capacities cap;

    test_setup();
    it_horse = test_create_horse();
    it_cart = test_create_cart();
    rc = test_create_race(NULL);
    u = test_create_unit(test_create_faction_ex(rc, NULL), test_create_plain(0, 0));

    rc->speed = 1.0;
    CuAssertIntEquals(tc, BP_WALKING, movement_speed(u, NULL));

    rc->speed = 2.0;
    CuAssertIntEquals(tc, 2 * BP_WALKING, movement_speed(u, NULL));

    set_level(u, SK_RIDING, 1);
    i_change(&u->items, it_horse, 1);
    get_transporters(u->items, &cap);
    CuAssertPtrEquals(tc, (void *)it_horse, (void *)cap.animals[0].type);
    CuAssertIntEquals(tc, 1, cap.animals[0].count);
    CuAssertPtrEquals(tc, NULL, (void *)cap.animals[1].type);
    CuAssertPtrEquals(tc, NULL, (void *)cap.vehicles[0].type);
    CuAssertIntEquals(tc, BP_RIDING, movement_speed(u, &cap));

    i_change(&u->items, it_horse, 1);
    i_change(&u->items, it_cart, 1);
    get_transporters(u->items, &cap);
    CuAssertPtrEquals(tc, (void *)it_horse, (void *)cap.animals[0].type);
    CuAssertIntEquals(tc, 2, cap.animals[0].count);
    CuAssertPtrEquals(tc, NULL, (void *)cap.animals[1].type);
    CuAssertPtrEquals(tc, (void *)it_cart, (void *)cap.vehicles[0].type);
    CuAssertIntEquals(tc, 1, cap.vehicles[0].count);
    CuAssertPtrEquals(tc, NULL, (void *)cap.vehicles[1].type);

    test_teardown();
}

static void test_route_cycle(CuTest *tc) {
    unit *u;
    region *r;
    struct locale *lang;
    char buffer[32];

    test_setup();
    setup_move();
    test_create_region(1, 0, NULL);
    r = test_create_region(2, 0, NULL);
    lang = test_create_locale();
    CuAssertPtrNotNull(tc, LOC(lang, shortdirections[D_WEST]));
    u = test_create_unit(test_create_faction(), r);
    u->faction->locale = lang;
    CuAssertIntEquals(tc, RCF_WALK, u->_race->flags & RCF_WALK);
    u->orders = create_order(K_ROUTE, u->faction->locale, "WEST EAST NW");
    CuAssertStrEquals(tc, "route WEST EAST NW", get_command(u->orders, lang, buffer, sizeof(buffer)));
    move_cmd(u, u->orders);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "moveblocked"));
    CuAssertIntEquals(tc, 1, u->region->x);
    CuAssertStrEquals(tc, "route east nw west", get_command(u->orders, lang, buffer, sizeof(buffer)));
    test_teardown();
}

static void test_cycle_route(CuTest *tc) {
    struct locale *lang;
    char buffer[32];
    order *ord, *onew;

    test_setup();
    lang = test_create_locale();
    CuAssertPtrNotNull(tc, LOC(lang, shortdirections[D_WEST]));

    ord = create_order(K_ROUTE, lang, "WEST EAST PAUSE NW");
    CuAssertStrEquals(tc, "route WEST EAST PAUSE NW", get_command(ord, lang, buffer, sizeof(buffer)));

    onew = cycle_route(ord, lang, 1);
    CuAssertStrEquals(tc, "route east PAUSE nw west", get_command(onew, lang, buffer, sizeof(buffer)));
    free_order(onew);

    onew = cycle_route(ord, lang, 2);
    CuAssertStrEquals(tc, "route nw west east PAUSE", get_command(onew, lang, buffer, sizeof(buffer)));
    free_order(onew);

    free_order(ord);
    test_teardown();
}

static void test_sailing_sets_coast(CuTest* tc) {
    unit* u;
    region* r1, * r2;
    ship_type * stype;

    test_setup();
    r2 = test_create_plain(0, 0);
    r1 = test_create_ocean(1, 0);
    u = test_create_unit(test_create_faction(), r1);
    u->ship = test_create_ship(r1, stype = test_create_shiptype("longboat"));
    set_level(u, SK_SAILING, stype->sumskill);
    CuAssertPtrEquals(tc, (void *)r2->terrain, stype->coasts[0]);
    u->thisorder = create_order(K_MOVE, u->faction->locale, "WEST");
    move_cmd(u, u->thisorder);
    CuAssertPtrEquals(tc, r2, u->region);
    CuAssertIntEquals(tc, D_EAST, u->ship->coast);
    test_teardown();
}

static void test_leave_coast_direction(CuTest *tc) {
    unit *u;
    region *r1, *r2;
    ship_type *stype;

    test_setup();
    r1 = test_create_plain(0, 0);
    u = test_create_unit(test_create_faction(), r1);
    u->ship = test_create_ship(r1, stype = test_create_shiptype("longboat"));
    set_level(u, SK_SAILING, stype->sumskill);

    r2 = test_create_ocean(1, 0);
    u->ship->coast = D_EAST;
    u->thisorder = create_order(K_MOVE, u->faction->locale, "EAST");
    move_cmd(u, u->thisorder);
    CuAssertPtrEquals(tc, r2, u->region);
    CuAssertIntEquals(tc, NODIRECTION, u->ship->coast);
    CuAssertIntEquals(tc, SF_MOVED, u->ship->flags & SF_MOVED);
    CuAssertPtrEquals(tc, NULL, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "shipsail"));
    test_clear_messages(u->faction);
    u->ship->flags -= SF_MOVED;

    move_ship(u->ship, r2, r1, NULL);
    u->ship->coast = D_NORTHEAST;
    u->thisorder = create_order(K_MOVE, u->faction->locale, "EAST");
    move_cmd(u, u->thisorder);
    CuAssertPtrEquals(tc, r2, u->region);
    CuAssertIntEquals(tc, NODIRECTION, u->ship->coast);
    CuAssertIntEquals(tc, SF_MOVED, u->ship->flags & SF_MOVED);
    CuAssertPtrEquals(tc, NULL, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "shipsail"));
    test_clear_messages(u->faction);
    u->ship->flags -= SF_MOVED;

    move_ship(u->ship, r2, r1, NULL);
    u->ship->coast = D_SOUTHEAST;
    u->thisorder = create_order(K_MOVE, u->faction->locale, "EAST");
    move_cmd(u, u->thisorder);
    CuAssertPtrEquals(tc, r2, u->region);
    CuAssertIntEquals(tc, NODIRECTION, u->ship->coast);
    CuAssertIntEquals(tc, SF_MOVED, u->ship->flags & SF_MOVED);
    CuAssertPtrEquals(tc, NULL, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "shipsail"));
    test_clear_messages(u->faction);
    u->ship->flags -= SF_MOVED;

    move_ship(u->ship, r2, r1, NULL);
    u->ship->coast = D_WEST;
    u->thisorder = create_order(K_MOVE, u->faction->locale, "EAST");
    move_cmd(u, u->thisorder);
    CuAssertPtrEquals(tc, r1, u->region);
    CuAssertIntEquals(tc, D_WEST, u->ship->coast);
    CuAssertIntEquals(tc, 0, u->ship->flags & SF_MOVED);
    CuAssertPtrEquals(tc, NULL, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "error182"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "shipsail"));
    test_clear_messages(u->faction);

    u->ship->coast = D_NORTHWEST;
    u->thisorder = create_order(K_MOVE, u->faction->locale, "EAST");
    move_cmd(u, u->thisorder);
    CuAssertPtrEquals(tc, r1, u->region);
    CuAssertIntEquals(tc, D_NORTHWEST, u->ship->coast);
    CuAssertIntEquals(tc, 0, u->ship->flags & SF_MOVED);
    CuAssertPtrEquals(tc, NULL, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "error182"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "shipsail"));
    test_clear_messages(u->faction);

    u->ship->coast = D_SOUTHWEST;
    u->thisorder = create_order(K_MOVE, u->faction->locale, "EAST");
    move_cmd(u, u->thisorder);
    CuAssertPtrEquals(tc, r1, u->region);
    CuAssertIntEquals(tc, D_SOUTHWEST, u->ship->coast);
    CuAssertIntEquals(tc, 0, u->ship->flags & SF_MOVED);
    CuAssertPtrEquals(tc, NULL, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "error182"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "shipsail"));
    test_clear_messages(u->faction);

    test_teardown();
}

static void test_sail_into_harbour(CuTest* tc) {
    unit* u, *u2;
    region* r1, *r2;
    faction* f;
    move_fixture mf;

    test_setup();
    r1 = test_create_ocean(1, 0);
    setup_harbor(&mf, r1);
    u = mf.u;
    r2 = mf.r;
    f = mf.u->faction;

    u->thisorder = create_order(K_MOVE, f->locale, "WEST");
    u2 = test_create_unit(test_create_faction(), r2);
    u2->building = mf.b;
    building_set_owner(u2);
    move_cmd(u, u->thisorder);
    CuAssertPtrNotNullMsg(tc, "entry into harbor denied", test_find_messagetype(f->msgs, "harbor_denied"));
    CuAssertIntEquals(tc, NODIRECTION, u->ship->coast);
    CuAssertIntEquals(tc, 0, u->ship->damage);
    CuAssertPtrEquals(tc, r1, u->region);
    test_clear_messages(f);

    contact_unit(u2, u);
    u->thisorder = create_order(K_MOVE, f->locale, "WEST");
    move_cmd(u, u->thisorder);
    CuAssertPtrEquals_Msg(tc, "harbor master makes contact", NULL, test_find_messagetype(f->msgs, "harbor_denied"));
    CuAssertIntEquals(tc, NODIRECTION, u->ship->coast);
    CuAssertIntEquals(tc, 0, u->ship->damage);
    CuAssertPtrEquals(tc, r2, u->region);
    test_clear_messages(f);

    test_teardown();
}

static void test_harbour_in_plain(CuTest *tc) {
    unit *u, *u2;
    region *r1, *r2;
    faction *f;
    move_fixture mf;

    test_setup();
    r1 = test_create_ocean(1, 0);
    setup_harbor(&mf, r1);
    u = mf.u;
    r2 = mf.r;
    f = mf.u->faction;
    mf.stype->coasts[0] = mf.ttype;

    u->thisorder = create_order(K_MOVE, f->locale, "WEST");
    u2 = test_create_unit(test_create_faction(), r2);
    u2->building = mf.b;
    building_set_owner(u2);
    move_cmd(u, u->thisorder);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "harbor_denied"));
    CuAssertIntEquals(tc, D_EAST, u->ship->coast);
    CuAssertIntEquals(tc, 0, u->ship->damage);
    CuAssertPtrEquals(tc, r2, u->region);
    test_clear_messages(f);

    move_ship(u->ship, r2, r1, NULL);
    contact_unit(u2, u);
    u->thisorder = create_order(K_MOVE, f->locale, "WEST");
    move_cmd(u, u->thisorder);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "harbor_denied"));
    CuAssertIntEquals(tc, NODIRECTION, u->ship->coast);
    CuAssertIntEquals(tc, 0, u->ship->damage);
    CuAssertPtrEquals(tc, r2, u->region);
    test_clear_messages(f);

    test_teardown();
}

static void test_route_pause(CuTest *tc) {
    unit *u;
    region *r;
    struct locale *lang;
    char buffer[32];

    test_setup();
    setup_move();
    test_create_region(1, 0, NULL);
    r = test_create_region(2, 0, NULL);
    lang = test_create_locale();
    CuAssertPtrNotNull(tc, LOC(lang, shortdirections[D_WEST]));
    u = test_create_unit(test_create_faction(), r);
    u->faction->locale = lang;
    CuAssertIntEquals(tc, RCF_WALK, u->_race->flags & RCF_WALK);
    u->orders = create_order(K_ROUTE, u->faction->locale, "PAUSE EAST NW");
    CuAssertStrEquals(tc, "route PAUSE EAST NW", get_command(u->orders, lang, buffer, sizeof(buffer)));
    move_cmd(u, u->orders);
    CuAssertIntEquals(tc, 2, u->region->x);
    CuAssertStrEquals(tc, "route PAUSE EAST NW", get_command(u->orders, lang, buffer, sizeof(buffer)));
    test_teardown();
}

static void test_movement_speed_dragon(CuTest *tc) {
    unit *u;
    race *rc;

    test_setup();
    rc = test_create_race("dragon");
    rc->flags |= RCF_DRAGON;
    rc->speed = 1.5;
    u = test_create_unit(test_create_faction_ex(rc, NULL), test_create_plain(0, 0));
    CuAssertIntEquals(tc, 6, movement_speed(u, NULL));
    test_teardown();
}

static void test_movement_speed_unicorns(CuTest *tc) {
    unit *u;
    item_type* it_unicorn;
    item_type const *it_horse;
    capacities cap;

    test_setup();
    init_resources();
    it_horse = it_find("horse");
    it_unicorn = test_create_itemtype("elvenhorse");
    it_unicorn->flags |= ITF_ANIMAL;
    it_unicorn->weight = it_horse->weight;
    it_unicorn->capacity = it_horse->capacity;

    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    scale_number(u, 10);
    i_change(&u->items, it_unicorn, 5);
    get_transporters(u->items, &cap);

    /* 5 animals can carry 2 people each: */
    set_level(u, SK_RIDING, 1);
    CuAssertIntEquals(tc, u->number / 2, i_get(u->items, it_unicorn));
    CuAssertIntEquals(tc, BP_RIDING, movement_speed(u, &cap));

    /* at level 1, riders can move 2 animals each */
    i_change(&u->items, it_unicorn, 15);
    get_transporters(u->items, &cap);
    CuAssertIntEquals(tc, 2 * u->number, i_get(u->items, it_unicorn));
    CuAssertIntEquals(tc, BP_RIDING, movement_speed(u, &cap));

    /* any more animals, and they walk: */
    i_change(&u->items, it_unicorn, 1);
    get_transporters(u->items, &cap);
    CuAssertIntEquals(tc, 1 + 2 * u->number, i_get(u->items, it_unicorn));
    CuAssertIntEquals(tc, BP_WALKING, movement_speed(u, &cap));

    /* up to 5 animals per person can we walked: */
    i_change(&u->items, it_unicorn, 29);
    get_transporters(u->items, &cap);
    CuAssertIntEquals(tc, 5 * u->number, i_get(u->items, it_unicorn));
    CuAssertIntEquals(tc, BP_WALKING, movement_speed(u, &cap));

    /* but no more! */
    i_change(&u->items, it_unicorn, 1);
    get_transporters(u->items, &cap);
    CuAssertIntEquals(tc, 1 + 5 * u->number, i_get(u->items, it_unicorn));
    CuAssertIntEquals(tc, 0, movement_speed(u, &cap));

    /* at level 5, unicorns give a speed boost: */
    set_level(u, SK_RIDING, 5);
    /* we can now ride at most 10 animals each: */
    i_change(&u->items, it_unicorn, 49);
    get_transporters(u->items, &cap);
    CuAssertIntEquals(tc, 10 * u->number, i_get(u->items, it_unicorn));
    CuAssertIntEquals(tc, BP_UNICORN, movement_speed(u, &cap));

    /* too many animals, we must walk: */
    i_change(&u->items, it_unicorn, 1);
    get_transporters(u->items, &cap);
    CuAssertIntEquals(tc, 1 + 10 * u->number, i_get(u->items, it_unicorn));
    CuAssertIntEquals(tc, BP_WALKING, movement_speed(u, &cap));

    /* at level 5, we can each walk 21 animals: */
    i_change(&u->items, it_unicorn, 109);
    get_transporters(u->items, &cap);
    CuAssertIntEquals(tc, 21 * u->number, i_get(u->items, it_unicorn));
    CuAssertIntEquals(tc, BP_WALKING, movement_speed(u, &cap));

    /* too many animals, we cannot move: */
    i_change(&u->items, it_unicorn, 1);
    get_transporters(u->items, &cap);
    CuAssertIntEquals(tc, 1 + 21 * u->number, i_get(u->items, it_unicorn));
    CuAssertIntEquals(tc, 0, movement_speed(u, &cap));

    /* max animals for riding, then add one (slow) horse */
    i_change(&u->items, it_unicorn, -112);
    get_transporters(u->items, &cap);
    CuAssertIntEquals(tc, 10 * u->number - 1, i_get(u->items, it_unicorn));
    CuAssertIntEquals(tc, BP_UNICORN, movement_speed(u, &cap));
    i_change(&u->items, it_horse, 1);
    get_transporters(u->items, &cap);
    CuAssertIntEquals(tc, 1, i_get(u->items, it_horse));
    CuAssertIntEquals(tc, BP_RIDING, movement_speed(u, &cap));

    test_teardown();
}

static void test_make_movement_order(CuTest *tc) {
    order *ord;
    char buffer[32];
    struct locale *lang;
    direction_t steps[] = { D_EAST, D_WEST, D_EAST, D_WEST };

    test_setup();
    lang = test_create_locale();

    ord = make_movement_order(lang, steps, 2);
    CuAssertStrEquals(tc, "move east west", get_command(ord, lang, buffer, sizeof(buffer)));
    free_order(ord);

    ord = make_movement_order(lang, steps, 4);
    CuAssertStrEquals(tc, "move east west east west", get_command(ord, lang, buffer, sizeof(buffer)));
    free_order(ord);

    test_teardown();
}

static void test_transport_unit(CuTest* tc)
{
    unit* u1, * u2;
    region *r, *r2;
    faction *f, *f2;

    test_setup();
    r = test_create_plain(0, 0);
    r2 = test_create_plain(1, 0);
    u1 = test_create_unit(f = test_create_faction(), r);
    scale_number(u1, 10);
    u2 = test_create_unit(f2 = test_create_faction(), r2);
    contact_unit(u2, u1);
    u2->thisorder = create_order(K_DRIVE, f->locale, itoa36(u1->no));
    movement();
    CuAssertIntEquals(tc, 1, test_count_messagetype(f2->msgs, "feedback_unit_not_found"));

    unit_addorder(u1, create_order(K_TRANSPORT, f->locale, itoa36(u2->no)));
    test_clear_messages(f);
    test_clear_messages(f2);
    movement();
    CuAssertIntEquals(tc, 0, test_count_messagetype(f->msgs, "feedback_unit_not_found"));
    CuAssertIntEquals(tc, 1, test_count_messagetype(f2->msgs, "feedback_unit_not_found"));

    u1->thisorder = create_order(K_MOVE, f->locale, LOC(f->locale, directions[D_EAST]));
    move_unit(u2, r, NULL);
    test_clear_messages(f);
    test_clear_messages(f2);
    movement();
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "feedback_unit_not_found"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f2->msgs, "feedback_unit_not_found"));
    CuAssertPtrEquals(tc, r2, u1->region);
    CuAssertPtrEquals(tc, r2, u2->region);
    test_teardown();
}

static void test_transport_stealthed(CuTest* tc)
{
    unit* u1, * u2;
    region *r, *r2;
    faction *f, *f2;

    test_setup();
    r = test_create_plain(0, 0);
    r2 = test_create_plain(1, 0);
    u1 = test_create_unit(f = test_create_faction(), r);
    scale_number(u1, 10);
    u2 = test_create_unit(f2 = test_create_faction(), r);
    set_level(u1, SK_STEALTH, 1);
    set_level(u2, SK_STEALTH, 1);
    u2->thisorder = create_order(K_DRIVE, f->locale, itoa36(u1->no));
    u1->thisorder = create_order(K_MOVE, f->locale, LOC(f->locale, directions[D_EAST]));
    unit_addorder(u1, create_order(K_TRANSPORT, f->locale, itoa36(u2->no)));

    movement();
    CuAssertPtrEquals(tc, r2, u1->region);
    CuAssertPtrEquals(tc, r2, u2->region);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "feedback_unit_not_found"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f2->msgs, "feedback_unit_not_found"));

    test_teardown();
}

#ifdef ENABLE_FOGTRAP_CURSE
static void test_move_blocked_by_fogtrap(CuTest* tc) {
    unit* u;
    region* r, *rt;
    struct locale* lang;

    test_setup();
    setup_move();
    register_regioncurse();
    rt = test_create_region(1, 0, NULL);
    r = test_create_region(2, 0, NULL);
    lang = test_create_locale();
    u = test_create_unit(test_create_faction(), r);
    u->faction->locale = lang;
    u->thisorder = create_order(K_MOVE, lang, "WEST");
    create_curse(u, &rt->attribs, ct_find("fogtrap"), 2.0, 2, 4.0, 0);
    /* Es wird erst gar keine Route erzeugt: */
    move_cmd(u, u->thisorder);
    CuAssertPtrEquals(tc, NULL, u->thisorder);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(u->faction->msgs, "enterfail"));
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "moveblocked"));
    CuAssertPtrEquals(tc, r, u->region);
    test_teardown();
}
#endif

static void test_holyground_blocks_undead_moves(CuTest* tc) {
    unit* u;
    region* r, *rt;
    struct locale* lang;
    race* rc;
    message* msg;

    test_setup();
    setup_move();
    register_regioncurse();
    rt = test_create_region(1, 0, NULL);
    r = test_create_region(2, 0, NULL);
    lang = test_create_locale();
    u = test_create_unit(test_create_faction(), r);
    u->faction->locale = lang;
    rc = test_create_race("undead");
    rc->flags |= RCF_UNDEAD;
    u_setrace(u, rc);
    u->thisorder = create_order(K_MOVE, lang, "WEST");
    create_curse(u, &rt->attribs, ct_find("holyground"), 2.0, 2, 4.0, 0);
    /* Es wird erst gar keine Route erzeugt: */
    move_cmd(u, u->thisorder);
    CuAssertPtrEquals(tc, NULL, u->thisorder);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(u->faction->msgs, "enterfail"));
    CuAssertPtrNotNull(tc, msg = test_find_messagetype(u->faction->msgs, "moveblocked"));
    CuAssertPtrEquals(tc, u, msg->parameters[0].v);
    CuAssertIntEquals(tc, D_WEST, msg->parameters[1].i);
    CuAssertPtrEquals(tc, r, u->region);
    test_teardown();
}

static void test_holyground_blocks_undead_follow(CuTest* tc) {
    unit* u, *u2;
    region* r, *rt;
    struct locale* lang;
    race* rc;
    message* msg;

    test_setup();
    setup_move();
    register_regioncurse();
    rt = test_create_region(1, 0, NULL);
    r = test_create_region(2, 0, NULL);
    lang = test_create_locale();
    u = test_create_unit(test_create_faction(), r);
    u->faction->locale = lang;
    rc = test_create_race("undead");
    rc->flags |= RCF_UNDEAD;
    u_setrace(u, rc);
    u2 = test_create_unit(test_create_faction(), r);
    u2->thisorder = create_order(K_MOVE, lang, "WEST");
    u->orders = create_order(K_FOLLOW, lang, "%s %s",
        param_name(P_UNIT, lang), itoa36(u2->no));
    create_curse(u, &rt->attribs, ct_find("holyground"), 2.0, 2, 4.0, 0);

    follow_cmds(u);
    CuAssertIntEquals(tc, UFL_FOLLOWED, u2->flags & UFL_FOLLOWED);
    CuAssertIntEquals(tc, UFL_FOLLOWING, u->flags & UFL_FOLLOWING);
    move_cmd(u2, u2->thisorder);
    CuAssertPtrEquals(tc, rt, u2->region);
    CuAssertPtrEquals(tc, r, u->region);
    CuAssertPtrEquals(tc, NULL, u2->thisorder);
    CuAssertPtrEquals(tc, NULL, u->thisorder);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(u2->faction->msgs, "enterfail"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(u2->faction->msgs, "moveblocked"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(u->faction->msgs, "moveblocked"));
    CuAssertPtrNotNull(tc, msg = test_find_messagetype(u->faction->msgs, "enterfail"));
    CuAssertPtrEquals(tc, u, msg->parameters[0].v);
    CuAssertPtrEquals(tc, rt, msg->parameters[1].v);
    test_teardown();
}

CuSuite *get_move_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_is_transporting);
    SUITE_ADD_TEST(suite, test_movement_speed);
    SUITE_ADD_TEST(suite, test_movement_speed_dragon);
    SUITE_ADD_TEST(suite, test_movement_speed_unicorns);
    SUITE_ADD_TEST(suite, test_walkingcapacity);
    SUITE_ADD_TEST(suite, test_horses_pull_carts);
    SUITE_ADD_TEST(suite, test_trolls_pull_carts);
    SUITE_ADD_TEST(suite, test_sailing_sets_coast);
    SUITE_ADD_TEST(suite, test_leave_coast_direction);
    SUITE_ADD_TEST(suite, test_sailing_sets_coast);
    SUITE_ADD_TEST(suite, test_sail_into_harbour);
    SUITE_ADD_TEST(suite, test_harbour_in_plain);
    SUITE_ADD_TEST(suite, test_ship_not_allowed_in_coast);
    SUITE_ADD_TEST(suite, test_ship_leave_trail);
    SUITE_ADD_TEST(suite, test_ship_allowed_coast_ignores_harbor);
    SUITE_ADD_TEST(suite, test_ship_allowed_without_harbormaster);
    SUITE_ADD_TEST(suite, test_ship_blocked_by_harbormaster);
    SUITE_ADD_TEST(suite, test_ship_blocked_by_unpaid_harbor);
    SUITE_ADD_TEST(suite, test_ship_has_harbormaster_contact);
    SUITE_ADD_TEST(suite, test_ship_has_harbormaster_ally);
    SUITE_ADD_TEST(suite, test_ship_has_harbormaster_same_faction);
    SUITE_ADD_TEST(suite, test_ship_allowed_insect);
    SUITE_ADD_TEST(suite, test_ship_trails);
    SUITE_ADD_TEST(suite, test_age_trails);
    SUITE_ADD_TEST(suite, test_ship_no_overload);
    SUITE_ADD_TEST(suite, test_ship_empty);
    SUITE_ADD_TEST(suite, test_ship_no_crew);
    SUITE_ADD_TEST(suite, test_no_drift_damage);
    SUITE_ADD_TEST(suite, test_ship_normal_overload);
    SUITE_ADD_TEST(suite, test_ship_small_overload);
    SUITE_ADD_TEST(suite, test_ship_big_overload);
    SUITE_ADD_TEST(suite, test_ship_ridiculous_overload);
    SUITE_ADD_TEST(suite, test_ship_ridiculous_overload_bad);
    SUITE_ADD_TEST(suite, test_ship_ridiculous_overload_no_captain);
    SUITE_ADD_TEST(suite, test_ship_damage_overload);
    SUITE_ADD_TEST(suite, test_follow_unit);
    SUITE_ADD_TEST(suite, test_follow_bad_target);
    SUITE_ADD_TEST(suite, test_follow_unit_self);
    SUITE_ADD_TEST(suite, test_follow_ship_msg);
    SUITE_ADD_TEST(suite, test_drifting_ships);
    SUITE_ADD_TEST(suite, test_route_cycle);
    SUITE_ADD_TEST(suite, test_cycle_route);
    SUITE_ADD_TEST(suite, test_route_pause);
    SUITE_ADD_TEST(suite, test_make_movement_order);
    SUITE_ADD_TEST(suite, test_transport_unit);
    SUITE_ADD_TEST(suite, test_transport_stealthed);
#ifdef ENABLE_FOGTRAP_CURSE
    SUITE_ADD_TEST(suite, test_move_blocked_by_fogtrap);
#endif
    SUITE_ADD_TEST(suite, test_holyground_blocks_undead_moves);
    SUITE_ADD_TEST(suite, test_holyground_blocks_undead_follow);
    return suite;
}
