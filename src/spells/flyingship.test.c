#include "flyingship.h"

#include <kernel/build.h>
#include <kernel/faction.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/ship.h>

#include <util/message.h>

#include <magic.h>
#include <tests.h>

#include <stb_ds.h>
#include <CuTest.h>

#include <stddef.h>          // for NULL

static void test_flyingship(CuTest * tc)
{
    castorder co;
    spellparameter par_data, *par = NULL;
    region *r;
    faction *f;
    unit *u;
    ship_type *shipType1, *shipType2;
    ship *sh1, *sh2;

    test_setup();
    mt_create_va(mt_new("flying_ship_result", NULL),
        "mage:unit", "ship:ship", MT_NEW_END);

    par_data.typ = SPP_SHIP;
    par_data.flag = 0;

    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);

    shipType1 = test_create_shiptype("boot");
    shipType1->construction->maxsize = 50;

    shipType2 = test_create_shiptype("schiff");
    shipType2->construction->maxsize = 51;

    sh1 = test_create_ship(r, shipType1);
    par_data.data.sh = sh1;

    arrput(par, par_data);

    test_create_castorder(&co, u, 10, 10.0, 0, par);
    CuAssertTrue(tc, !flying_ship(sh1));
    CuAssertIntEquals(tc, 10, sp_flying_ship(&co));
    CuAssertTrue(tc, flying_ship(sh1));

    sh2 = test_create_ship(r, shipType2);
    par_data.data.sh = sh2;

    test_create_castorder(&co, u, 10, 10.0, 0, par);
    CuAssertTrue(tc, !flying_ship(sh2));
    CuAssertIntEquals(tc, 0, sp_flying_ship(&co));
    CuAssertTrue(tc, !flying_ship(sh2));

    test_teardown();
}

CuSuite *get_flyingship_suite(void)
{
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, test_flyingship);

    return suite;
}
