#include <platform.h>
#include <config.h>
#include "seen.h"

#include <kernel/region.h>

#include <CuTest.h>
#include <tests.h>

static void test_seen_region(CuTest *tc) {
    region *r, *first, *last;
    seen_region **seen;
    int dir;

    test_cleanup();
    r = test_create_region(0, 0, 0);
    for (dir=0;dir!=MAXDIRECTIONS;++dir) {
        region *rn = test_create_region(delta_x[dir], delta_y[dir], 0);
    }
    seen = seen_init();
    add_seen(seen, r, see_unit, true);
    first = r;
    last = 0;
    get_seen_interval(seen, &first, &last);
    CuAssertPtrEquals(tc, r, first);
    CuAssertPtrEquals(tc, 0, last);
    seen_done(seen);
    test_cleanup();    
}

CuSuite *get_seen_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_seen_region);
    return suite;
}
