#include <platform.h>
#include "piracy.h"

#include <kernel/config.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/faction.h>
#include <kernel/order.h>

#include <util/base36.h>
#include <util/language.h>

#include <CuTest.h>
#include <tests.h>
#include <assert.h>

static void setup_piracy(void) {
    struct locale *lang;
    terrain_type *t_ocean;
    ship_type *st_boat;

    test_cleanup();
    set_param(&global.parameters, "rules.ship.storms", "0");
    lang = get_or_create_locale("de");
    locale_setstring(lang, directions[D_EAST], "OSTEN");
    init_directions(lang);
    t_ocean = test_create_terrain("ocean", SAIL_INTO | SEA_REGION);
    st_boat = test_create_shiptype("boat");
    st_boat->cargo = 1000;
}

static void test_piracy_cmd(CuTest * tc) {
    faction *f;
    region *r;
    unit *u, *u2;
    order *ord;
    terrain_type *t_ocean;
    ship_type *st_boat;

    setup_piracy();
    t_ocean = get_or_create_terrain("ocean");
    st_boat = st_get_or_create("boat");
    u2 = test_create_unit(test_create_faction(0), test_create_region(1, 0, t_ocean));
    u_set_ship(u2, test_create_ship(u2->region, st_boat));
    assert(u2);
    u = test_create_unit(f = test_create_faction(0), r = test_create_region(0, 0, t_ocean));
    set_level(u, SK_SAILING, st_boat->sumskill);
    u_set_ship(u, test_create_ship(u->region, st_boat));
    assert(f && u);
    f->locale = get_or_create_locale("de");
    ord = create_order(K_PIRACY, f->locale, "%s", itoa36(u2->faction->no));
    assert(ord);

    piracy_cmd(u, ord);
    CuAssertPtrEquals(tc, 0, u->thisorder);
    CuAssertTrue(tc, u->region != r);
    CuAssertPtrEquals(tc, u2->region, u->region);
    CuAssertPtrEquals(tc, u2->region, u->ship->region);
    CuAssertPtrNotNullMsg(tc, "successful PIRACY sets attribute", r->attribs); // FIXME: this is testing implementation, not interface
    CuAssertPtrNotNullMsg(tc, "successful PIRACY message", test_find_messagetype(f->msgs, "piratesawvictim"));
    CuAssertPtrNotNullMsg(tc, "successful PIRACY movement", test_find_messagetype(f->msgs, "shipsail"));

    test_cleanup();
}

static void test_piracy_cmd_errors(CuTest * tc) {
    faction *f;
    unit *u, *u2;
    order *ord;
    ship_type *st_boat;

    setup_piracy();
    st_boat = st_get_or_create("boat");
    u = test_create_unit(f = test_create_faction(0), test_create_region(0, 0, get_or_create_terrain("ocean")));
    f->locale = get_or_create_locale("de");
    ord = create_order(K_PIRACY, f->locale, "");
    assert(u && ord);

    piracy_cmd(u, ord);
    CuAssertPtrNotNullMsg(tc, "must be on a ship for PIRACY", test_find_messagetype(f->msgs, "error144"));

    u_set_ship(u, test_create_ship(u->region, st_boat));

    u2 = test_create_unit(u->faction, u->region);
    u_set_ship(u2, u->ship);

    test_clear_messages(f);
    piracy_cmd(u2, ord);
    CuAssertPtrNotNullMsg(tc, "must be owner for PIRACY", test_find_messagetype(f->msgs, "error146"));

    test_clear_messages(f);
    piracy_cmd(u, ord);
    CuAssertPtrNotNullMsg(tc, "must specify target for PIRACY", test_find_messagetype(f->msgs, "piratenovictim"));

    test_cleanup();
}


CuSuite *get_piracy_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_piracy_cmd_errors);
    SUITE_ADD_TEST(suite, test_piracy_cmd);
    return suite;
}
