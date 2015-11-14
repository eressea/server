#include <platform.h>
#include <stdlib.h>
#include "move.h"

#include "guard.h"
#include "keyword.h"

#include <kernel/config.h>
#include <kernel/ally.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/item.h>
#include <kernel/unit.h>
#include <kernel/race.h>
#include <kernel/order.h>

#include <util/attrib.h>
#include <util/language.h>
#include <util/message.h>
#include <util/base36.h>
#include <util/parser.h>

#include <CuTest.h>
#include <tests.h>
#include <assert.h>

static void test_ship_not_allowed_in_coast(CuTest * tc)
{
    region *r1, *r2;
    ship * sh;
    terrain_type *ttype, *otype;
    ship_type *stype;

    test_cleanup();
    test_create_world();

    ttype = test_create_terrain("glacier", LAND_REGION | ARCTIC_REGION | WALK_INTO | SAIL_INTO);
    otype = test_create_terrain("ocean", SEA_REGION | SAIL_INTO);
    stype = test_create_shiptype("derp");
    stype->coasts = (struct terrain_type **)calloc(2, sizeof(struct terrain_type *));

    r1 = test_create_region(0, 0, ttype);
    r2 = test_create_region(1, 0, otype);
    sh = test_create_ship(0, stype);

    CuAssertIntEquals(tc, SA_COAST, check_ship_allowed(sh, r2));
    CuAssertIntEquals(tc, SA_NO_COAST, check_ship_allowed(sh, r1));
    stype->coasts[0] = ttype;
    CuAssertIntEquals(tc, SA_COAST, check_ship_allowed(sh, r1));
}

typedef struct move_fixture {
    region *r;
    ship *sh;
    building * b;
    unit *u;
} move_fixture;

static void setup_harbor(move_fixture *mf) {
    region *r;
    ship * sh;
    terrain_type * ttype;
    building_type * btype;
    building * b;
    unit *u;

    test_cleanup();
    test_create_world();

    ttype = test_create_terrain("glacier", LAND_REGION | ARCTIC_REGION | WALK_INTO | SAIL_INTO);
    btype = test_create_buildingtype("harbour");

    sh = test_create_ship(0, 0);
    r = test_create_region(0, 0, ttype);

    b = test_create_building(r, btype);
    b->flags |= BLD_WORKING;

    u = test_create_unit(test_create_faction(0), r);
    u->ship = sh;
    ship_set_owner(u);

    mf->r = r;
    mf->u = u;
    mf->sh = sh;
    mf->b = b;
}

static void test_ship_allowed_without_harbormaster(CuTest * tc)
{
    move_fixture mf;

    setup_harbor(&mf);

    CuAssertIntEquals(tc, SA_HARBOUR, check_ship_allowed(mf.sh, mf.r));
}

static void test_ship_blocked_by_harbormaster(CuTest * tc) {
    unit *u;
    move_fixture mf;

    setup_harbor(&mf);

    u = test_create_unit(test_create_faction(0), mf.r);
    u->building = mf.b;
    building_set_owner(u);

    CuAssertIntEquals_Msg(tc, "harbor master must contact ship", SA_NO_COAST, check_ship_allowed(mf.sh, mf.r));
    test_cleanup();
}

static void test_ship_has_harbormaster_contact(CuTest * tc) {
    unit *u;
    move_fixture mf;

    setup_harbor(&mf);

    u = test_create_unit(test_create_faction(0), mf.r);
    u->building = mf.b;
    building_set_owner(u);
    usetcontact(mf.b->_owner, mf.sh->_owner);

    CuAssertIntEquals(tc, SA_HARBOUR, check_ship_allowed(mf.sh, mf.r));
    test_cleanup();
}

static void test_ship_has_harbormaster_same_faction(CuTest * tc) {
    unit *u;
    move_fixture mf;

    setup_harbor(&mf);

    u = test_create_unit(mf.u->faction, mf.r);
    u->building = mf.b;
    building_set_owner(u);

    CuAssertIntEquals(tc, SA_HARBOUR, check_ship_allowed(mf.sh, mf.r));
    test_cleanup();
}

static void test_ship_has_harbormaster_ally(CuTest * tc) {
    unit *u;
    move_fixture mf;
    ally *al;

    setup_harbor(&mf);

    u = test_create_unit(test_create_faction(0), mf.r);
    u->building = mf.b;
    building_set_owner(u);
    al = ally_add(&u->faction->allies, mf.u->faction);
    al->status = HELP_GUARD;

    CuAssertIntEquals(tc, SA_HARBOUR, check_ship_allowed(mf.sh, mf.r));
    test_cleanup();
}

static void test_building_type_exists(CuTest * tc)
{
    region *r;
    building *b;
    building_type *btype, *btype2;

    test_cleanup();
    test_create_world();

    btype2 = bt_get_or_create("lighthouse");
    btype = bt_get_or_create("castle");

    r = findregion(-1, 0);
    b = new_building(btype, r, default_locale);

    CuAssertPtrNotNull(tc, b);
    CuAssertTrue(tc, !buildingtype_exists(r, NULL, false));
    CuAssertTrue(tc, buildingtype_exists(r, btype, false));
    CuAssertTrue(tc, !buildingtype_exists(r, btype2, false));
}

static void test_walkingcapacity(CuTest *tc) {
    region *r;
    unit *u;
    int cap;
    const struct item_type *itype;

    test_cleanup();
    test_create_world();

    r = findregion(0, 0);
    u = test_create_unit(test_create_faction(0), r);
    cap = u->number * (u->_race->capacity + u->_race->weight);
    CuAssertIntEquals(tc, cap, walkingcapacity(u));
    scale_number(u, 2);
    cap = u->number * (u->_race->capacity + u->_race->weight);
    CuAssertIntEquals(tc, cap, walkingcapacity(u));

    itype = it_find("horse");
    assert(itype);
    i_change(&u->items, itype, 1);
    cap += itype->capacity;
    CuAssertIntEquals(tc, cap, walkingcapacity(u));
    i_change(&u->items, itype, 1);
    cap += itype->capacity;
    CuAssertIntEquals(tc, cap, walkingcapacity(u));

    itype = it_find("cart");
    assert(itype);
    i_change(&u->items, itype, 1);
    CuAssertIntEquals(tc, cap, walkingcapacity(u));
    set_level(u, SK_RIDING, 1);
    cap += itype->capacity;
    CuAssertIntEquals(tc, cap, walkingcapacity(u));

    itype = test_create_itemtype("trollbelt");
    assert(itype);
    i_change(&u->items, itype, 1);
    CuAssertIntEquals(tc, cap + (STRENGTHMULTIPLIER-1) * u->_race->capacity, walkingcapacity(u));
    set_param(&global.parameters, "rules.trollbelt.multiplier", "5");
    CuAssertIntEquals(tc, cap + 4 * u->_race->capacity, walkingcapacity(u));

    test_cleanup();
}

static void test_is_guarded(CuTest *tc) {
    unit *u1, *u2;
    region *r;
    race *rc;

    test_cleanup();
    rc = rc_get_or_create("dragon");
    rc->flags |= RCF_UNARMEDGUARD;
    r = test_create_region(0, 0, 0);
    u1 = test_create_unit(test_create_faction(0), r);
    u2 = test_create_unit(test_create_faction(rc), r);
    CuAssertPtrEquals(tc, 0, is_guarded(r, u1, GUARD_TRAVELTHRU));
    CuAssertPtrEquals(tc, 0, is_guarded(r, u1, GUARD_PRODUCE));
    CuAssertPtrEquals(tc, 0, is_guarded(r, u1, GUARD_TREES));
    CuAssertPtrEquals(tc, 0, is_guarded(r, u1, GUARD_MINING));
    guard(u2, GUARD_MINING | GUARD_PRODUCE);
    CuAssertIntEquals(tc, GUARD_CREWS | GUARD_LANDING | GUARD_TRAVELTHRU | GUARD_TAX | GUARD_PRODUCE | GUARD_RECRUIT, guard_flags(u2));
    CuAssertPtrEquals(tc, 0, is_guarded(r, u1, GUARD_TRAVELTHRU));
    CuAssertPtrEquals(tc, 0, is_guarded(r, u1, GUARD_TREES));
    CuAssertPtrEquals(tc, 0, is_guarded(r, u1, GUARD_MINING));
    CuAssertPtrEquals(tc, u2, is_guarded(r, u1, GUARD_PRODUCE));
    test_cleanup();
}

static void test_ship_trails(CuTest *tc) {
    ship *sh;
    region *r1, *r2, *r3;
    terrain_type *otype;
    region_list *route = 0;

    test_cleanup();
    otype = test_create_terrain("ocean", SEA_REGION | SAIL_INTO);
    r1 = test_create_region(0, 0, otype);
    r2 = test_create_region(1, 0, otype);
    r3 = test_create_region(2, 0, otype);
    sh = test_create_ship(r1, 0);
    move_ship(sh, r1, r3, 0);
    CuAssertPtrEquals(tc, r3, sh->region);
    CuAssertPtrEquals(tc, sh, r3->ships);
    CuAssertPtrEquals(tc, 0, r1->ships);
    CuAssertPtrEquals(tc, 0, a_find(r1->attribs, &at_shiptrail));
    CuAssertPtrEquals(tc, 0, a_find(r3->attribs, &at_shiptrail));
    add_regionlist(&route, r3);
    add_regionlist(&route, r2);
    move_ship(sh, r3, r1, route);
    CuAssertPtrEquals(tc, r1, sh->region);
    CuAssertPtrEquals(tc, sh, r1->ships);
    CuAssertPtrEquals(tc, 0, r3->ships);
    CuAssertPtrEquals(tc, 0, a_find(r1->attribs, &at_shiptrail));
    CuAssertPtrNotNull(tc, a_find(r2->attribs, &at_shiptrail));
    CuAssertPtrNotNull(tc, a_find(r3->attribs, &at_shiptrail));
    free_regionlist(route);
    test_cleanup();
}

static void test_age_trails(CuTest *tc) {
    region_list *route = 0;
    region *r1, *r2;
    ship *sh;

    test_cleanup();
    r1 = test_create_region(0, 0, 0);
    r2 = test_create_region(1, 0, 0);
    sh = test_create_ship(r1, 0);
    add_regionlist(&route, r1);
    add_regionlist(&route, r2);
    move_ship(sh, r1, r2, route);

    CuAssertPtrNotNull(tc, r1->attribs);
    a_age(&r1->attribs);
    CuAssertPtrNotNull(tc, r1->attribs);
    a_age(&r1->attribs);
    CuAssertPtrEquals(tc, 0, r1->attribs);
    free_regionlist(route);
    test_cleanup();
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
    item_type *silver;

    traveldir *td = NULL;
    attrib *a;

    test_cleanup();
    test_create_world();
    f = test_create_faction(0);
    r = findregion(0, 0);

    stype = st_find("boat");
    sh = test_create_ship(r, stype);
    u = test_create_unit(f, r);
    u->ship = sh;
    ship_set_owner(u);
    set_level(u, SK_SAILING, 10);

    ord = create_order(K_FOLLOW, f->locale, "SHIP 2");
    assert(ord);
    unit_addorder(u, ord);

    silver = get_resourcetype(R_SILVER)->itype;
    i_change(&u->items, silver, 999999);

    a = a_add(&(r->attribs), a_new(&at_shiptrail));
    td = (traveldir *)a->data.v;
    td->no = 2;
    td->dir = D_NORTHWEST;
    td->age = 2;

    locale_setstring(default_locale, "northwest", "Nordwesten");
    locale_setstring(default_locale, "keyword::move", "NACH");
    init_locale(default_locale);

    mt_register(mt_new_va("error18", "unit:unit", "region:region", "command:order"));

    init_order(ord);
    getstrtoken();

    follow_ship(u, ord);

    CuAssertPtrNotNull(tc, msg = test_find_messagetype(u->faction->msgs, "error18"));
    void *p = msg->parameters[2].v;
    CuAssertPtrNotNull(tc, p);
    CuAssertIntEquals(tc, K_FOLLOW, getkeyword((order *)p));

    test_cleanup();
}

CuSuite *get_move_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_walkingcapacity);
    SUITE_ADD_TEST(suite, test_building_type_exists);
    SUITE_ADD_TEST(suite, test_ship_not_allowed_in_coast);
    SUITE_ADD_TEST(suite, test_ship_allowed_without_harbormaster);
    SUITE_ADD_TEST(suite, test_ship_blocked_by_harbormaster);
    SUITE_ADD_TEST(suite, test_ship_has_harbormaster_contact);
    SUITE_ADD_TEST(suite, test_ship_has_harbormaster_ally);
    SUITE_ADD_TEST(suite, test_ship_has_harbormaster_same_faction);
    SUITE_ADD_TEST(suite, test_is_guarded);
    SUITE_ADD_TEST(suite, test_ship_trails);
    SUITE_ADD_TEST(suite, test_age_trails);
    SUITE_ADD_TEST(suite, test_follow_ship_msg);
    return suite;
}
