#include <platform.h>
#include <kernel/config.h>
#include "alchemy.h"
#include "unit.h"
#include "item.h"
#include "region.h"

#include <CuTest.h>
#include <tests.h>

#include <stdlib.h>
#include <assert.h>

static void test_scale_number(CuTest *tc) {
    unit *u;
    const struct potion_type *ptype;

    test_cleanup();
    test_create_world();
    ptype = new_potiontype(it_get_or_create(rt_get_or_create("hodor")), 1);
    u = test_create_unit(test_create_faction(test_create_race("human")), findregion(0, 0));
    change_effect(u, ptype, 1);
    CuAssertIntEquals(tc, 1, u->number);
    CuAssertIntEquals(tc, 1, u->hp);
    CuAssertIntEquals(tc, 1, get_effect(u, ptype));
    scale_number(u, 2);
    CuAssertIntEquals(tc, 2, u->number);
    CuAssertIntEquals(tc, 2, u->hp);
    CuAssertIntEquals(tc, 2, get_effect(u, ptype));
    set_level(u, SK_ALCHEMY, 1);
    scale_number(u, 0);
    CuAssertIntEquals(tc, 0, get_level(u, SK_ALCHEMY));
    test_cleanup();
}

CuSuite *get_unit_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_scale_number);
    return suite;
}
