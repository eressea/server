#include <platform.h>
#include "move.h"

#include <kernel/faction.h>
#include <kernel/order.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/unit.h>

#include <util/base36.h>
#include <util/language.h>

#include <CuTest.h>
#include <tests.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


static void setup_fleet(void) {
    ship_type* ftype;
    struct locale* lang;

    lang = get_or_create_locale("de");
    locale_setstring(lang, "fleet", "Flotte");
    test_create_world();
    init_locales();

    ftype = st_get_or_create("fleet");
    ftype->cptskill = 6;
}


static void test_fleet_create(CuTest *tc) {
    faction *f1;
    region *r;
    const ship_type *stype1;
    ship *sh1;
    unit *u11;

    struct order *ord;

    test_cleanup();
    setup_fleet();

    f1 = test_create_faction(NULL);
    r = findregion(0, 0);

    stype1 = st_find("boat");
    sh1 = test_create_ship(r, stype1);
    u11 = test_create_unit(f1, r);

    u_set_ship(u11, sh1);

    ord = create_order(K_FLEET, f1->locale, NULL);
    unit_addorder(u11, ord);
    fleet_cmd(r);

    CuAssertPtrNotNull(tc, sh1->fleet);

    test_cleanup();
}


/*
// TODO

// NEW
// test command variants
// test unhappy paths (captain valid, ship owner contacts...)
// test fleet ship properties
*/


CuSuite *get_fleets_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_fleet_create);
    return suite;
}
