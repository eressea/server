#include <platform.h>

#include "magic.h"

#include <kernel/callbacks.h>
#include <kernel/equipment.h>
#include <kernel/item.h>
#include <kernel/unit.h>
#include <kernel/spell.h>

#include <CuTest.h>
#include <tests.h>

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
    callbacks.equip_unit = equip_callback;
    unit * u;

    test_setup();

    u = test_create_unit(test_create_faction(NULL), test_create_region(0, 0, NULL));
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
