#include "equipment.h"

#include "callbacks.h"
#include "unit.h"

#include <CuTest.h>
#include <tests.h>

#include <stdbool.h>    // for true, bool

static unit *eq_unit;
static const char *eq_name;
static int eq_mask;

static bool equip_callback(unit *u, const char *eqname, int mask) {
    eq_unit = u;
    eq_name = eqname;
    eq_mask = mask;
    return true;
}

static void test_equipment(CuTest * tc)
{
    unit * u;

    test_setup();
    callbacks.equip_unit = equip_callback;

    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    CuAssertIntEquals(tc, true, equip_unit_mask(u, "hodor", EQUIP_ALL));
    CuAssertIntEquals(tc, EQUIP_ALL, eq_mask);
    CuAssertPtrEquals(tc, u, eq_unit);
    CuAssertStrEquals(tc, "hodor", eq_name);

    CuAssertIntEquals(tc, true, equip_unit_mask(u, "foobar", EQUIP_ITEMS));
    CuAssertIntEquals(tc, EQUIP_ITEMS, eq_mask);
    CuAssertPtrEquals(tc, u, eq_unit);
    CuAssertStrEquals(tc, "foobar", eq_name);

    CuAssertIntEquals(tc, true, equip_unit(u, "unknown"));
    CuAssertIntEquals(tc, EQUIP_ALL, eq_mask);
    CuAssertPtrEquals(tc, u, eq_unit);
    CuAssertStrEquals(tc, "unknown", eq_name);

    test_teardown();
}

CuSuite *get_equipment_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_equipment);
    return suite;
}
