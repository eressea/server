#include <platform.h>
#include "types.h"
#include "group.h"
#include "faction.h"
#include "unit.h"
#include "region.h"

#include <CuTest.h>
#include <tests.h>
#include <assert.h>

static void test_group(CuTest * tc)
{
    unit *u;
    region *r;
    faction *f;
    group *g;

    test_cleanup();
    test_create_world();
    r = findregion(0, 0);
    f = test_create_faction(0);
    assert(r && f);
    u = test_create_unit(f, r);
    assert(u);
    CuAssertTrue(tc, join_group(u, "hodor"));
    CuAssertPtrNotNull(tc, (g = get_group(u)));
    CuAssertStrEquals(tc, "hodor", g->name);
    CuAssertIntEquals(tc, 1, g->members);
    set_group(u, 0);
    CuAssertIntEquals(tc, 0, g->members);
    CuAssertPtrEquals(tc, 0, get_group(u));
    set_group(u, g);
    CuAssertIntEquals(tc, 1, g->members);
    CuAssertPtrEquals(tc, g, get_group(u));
    test_cleanup();
}

CuSuite *get_group_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_group);
    return suite;
}

