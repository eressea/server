#include <platform.h>
#include "piracy.h"

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

    test_cleanup();
    lang = get_or_create_locale("de");
    locale_setstring(lang, directions[D_EAST], "OSTEN");
    init_directions(lang);
    t_ocean = test_create_terrain("ocean", SAIL_INTO | SEA_REGION);
}

static void test_piracy_cmd(CuTest * tc) {
    faction *f;
    unit *u, *u2;
    order *ord;
    terrain_type *t_ocean;

    setup_piracy();
    t_ocean = get_or_create_terrain("ocean");
    u2 = test_create_unit(test_create_faction(0), test_create_region(1, 0, t_ocean));
    u_set_ship(u2, test_create_ship(u2->region, 0));
    assert(u2);
    u = test_create_unit(f = test_create_faction(0), test_create_region(0, 0, t_ocean));
    u_set_ship(u, test_create_ship(u->region, 0));
    assert(f && u);
    f->locale = get_or_create_locale("de");
    ord = create_order(K_PIRACY, f->locale, "%s", itoa36(u2->faction->no));
    assert(ord);

    piracy_cmd(u, ord);
    CuAssertPtrNotNullMsg(tc, "successful PIRACY", test_find_messagetype(f->msgs, "piratesawvictim"));

    test_cleanup();
}

static void test_piracy_cmd_errors(CuTest * tc) {
    faction *f;
    unit *u, *u2;
    order *ord;

    setup_piracy();
    u = test_create_unit(f = test_create_faction(0), test_create_region(0, 0, get_or_create_terrain("ocean")));
    f->locale = get_or_create_locale("de");
    ord = create_order(K_PIRACY, f->locale, "");
    assert(u && ord);

    piracy_cmd(u, ord);
    CuAssertPtrNotNullMsg(tc, "must be on a ship for PIRACY", test_find_messagetype(f->msgs, "error144"));

    u_set_ship(u, test_create_ship(u->region, 0));

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
