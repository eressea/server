#include <platform.h>
#include "flyingship.h"

#include <kernel/build.h>
#include <kernel/faction.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/ship.h>

#include <util/message.h>

#include <magic.h>

#include <CuTest.h>
#include <tests.h>
#include <assert.h>

static void test_flyingship(CuTest * tc)
{
    castorder co;
    spellparameter par;
    spllprm par_data;
    spllprm *par_data_ptr = &par_data;

    region *r;
    faction *f;
    unit *u;
    ship_type *shipType1, *shipType2;
    ship *sh1, *sh2;

    test_setup();
    mt_create_va(mt_new("flying_ship_result", NULL),
        "mage:unit", "ship:ship", MT_NEW_END);

    par.param = &par_data_ptr;
    par_data.typ = SPP_SHIP;
    par_data.flag = 0;

    r = test_create_region(0, 0, NULL);
    f = test_create_faction(test_create_race("human"));
    u = test_create_unit(f, r);

    shipType1 = test_create_shiptype("boot");
    shipType1->construction->maxsize = 50;

    shipType2 = test_create_shiptype("schiff");
    shipType2->construction->maxsize = 51;

    sh1 = test_create_ship(r, shipType1);
    par_data.data.sh = sh1;

    test_create_castorder(&co, u, 10, 10.0, 0, &par);
    CuAssertTrue(tc, !flying_ship(sh1));
    CuAssertIntEquals(tc, 10, sp_flying_ship(&co));
    CuAssertTrue(tc, flying_ship(sh1));
    co.par = 0;
    free_castorder(&co);

    sh2 = test_create_ship(r, shipType2);
    par_data.data.sh = sh2;

    test_create_castorder(&co, u, 10, 10.0, 0, &par);
    CuAssertTrue(tc, !flying_ship(sh2));
    CuAssertIntEquals(tc, 0, sp_flying_ship(&co));
    CuAssertTrue(tc, !flying_ship(sh2));
    co.par = 0;
    free_castorder(&co);
    test_teardown();
}

CuSuite *get_flyingship_suite(void)
{
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, test_flyingship);

    return suite;
}
