#include <platform.h>
#include <kernel/config.h>
#include <kernel/terrain.h>
#include "region.h"

#include <CuTest.h>
#include <tests.h>

#include <assert.h>

static void assert_setter(CuTest *tc, int (*setter)(const region *, int),
    int (*getter)(const region *), region *land, region *ocean, bool throwaway) {
    CuAssertIntEquals(tc, 0, setter(land, 0));
    CuAssertIntEquals(tc, 0, getter(land));
    CuAssertIntEquals(tc, 42, setter(land, 42));
    CuAssertIntEquals(tc, 42, getter(land));
    CuAssertIntEquals(tc, 0, setter(ocean, 0));
    CuAssertIntEquals(tc, 0, getter(ocean));
    if (throwaway) {
        CuAssertIntEquals(tc, 0, setter(ocean, 42));
        CuAssertIntEquals(tc, 0, getter(ocean));
    }
}

static int t_settrees0(const region *r, int trees) {
    return rsettrees(r, 0, trees);
}

static int t_trees0(const region *r) {
    return rtrees(r, 0);
}

static void test_setters(CuTest *tc) {
    region *land, *ocean;

    test_cleanup();
    test_create_world();
    land = findregion(0, 0);
    ocean = findregion(-1, 0);
    assert(fval(ocean->terrain, SEA_REGION) && fval(land->terrain, LAND_REGION));

    assert_setter(tc, rsetherbs, rherbs, land, ocean, true);
    assert_setter(tc, rsethorses, rhorses, land, ocean, true);
    assert_setter(tc, rsetmoney, rmoney, land, ocean, true);
    assert_setter(tc, rsetpeasants, rpeasants, land, ocean, true);
    assert_setter(tc, t_settrees0, t_trees0, land, ocean, false);

    test_cleanup();
}

CuSuite *get_region_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_setters);
    return suite;
}
