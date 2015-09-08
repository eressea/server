#include <platform.h>
#include <config.h>
#include "seen.h"
#include "reports.h"

#include <kernel/region.h>

#include <CuTest.h>
#include <tests.h>

static void test_seen_region(CuTest *tc) {
    seen_region **seen, *sr;
    region *r;
    int dir;

    test_cleanup();
    for (dir=0;dir!=MAXDIRECTIONS;++dir) {
        region *rn = test_create_region(delta_x[dir], delta_y[dir], 0);
    }
    r = test_create_region(0, 0, 0);
    seen = seen_init();
    add_seen(seen, r, see_unit, true);
    sr = find_seen(seen,  r);
    CuAssertPtrEquals(tc, r, sr->r);
    seen_done(seen);
    test_cleanup();
}

static void test_seen_interval_backward(CuTest *tc) {
    region *r, *first, *last;
    seen_region **seen, *sr;
    int dir;

    test_cleanup();
    r = test_create_region(0, 0, 0);
    for (dir=0;dir!=MAXDIRECTIONS;++dir) {
        region *rn = test_create_region(delta_x[dir], delta_y[dir], 0);
    }
    seen = seen_init();
    add_seen(seen, r, see_unit, true);
    view_default(seen, r, 0);
    first = r;
    last = 0;
    get_seen_interval(seen, &first, &last);
    CuAssertPtrEquals(tc, regions, first);
    CuAssertPtrEquals(tc, 0, last);
    test_cleanup();
}

static void test_seen_interval_forward(CuTest *tc) {
    region *r, *first, *last;
    seen_region **seen, *sr;
    int dir;

    test_cleanup();
    for (dir=0;dir!=MAXDIRECTIONS;++dir) {
        region *rn = test_create_region(delta_x[dir], delta_y[dir], 0);
    }
    r = test_create_region(0, 0, 0);
    seen = seen_init();
    add_seen(seen, r, see_unit, true);
    view_default(seen, r, 0);
    first = r;
    last = 0;
    get_seen_interval(seen, &first, &last);
    CuAssertPtrEquals(tc, regions, first);
    CuAssertPtrEquals(tc, 0, last);
    test_cleanup();
}

CuSuite *get_seen_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_seen_region);
    SUITE_ADD_TEST(suite, test_seen_interval_backward);
    SUITE_ADD_TEST(suite, test_seen_interval_forward);
    return suite;
}
