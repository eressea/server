#include <platform.h>
#include <config.h>
#include "seen.h"

#include <kernel/region.h>

#include <CuTest.h>
#include <tests.h>

static void test_seen_region(CuTest *tc) {
    region *r;
    seen_region **seen;

    test_cleanup();
    r = test_create_region(0, 0, 0);
    seen = seen_init();
    seen_done(seen);
    test_cleanup();    
}

CuSuite *get_seen_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_seen_region);
    return suite;
}
