#include <platform.h>
#include <config.h>
#include "seen.h"
#include "reports.h"
#include "travelthru.h"

#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/faction.h>

#include <CuTest.h>
#include <tests.h>

static void setup_seen(int x, int y) {
    int dir;

    for (dir = 0; dir != MAXDIRECTIONS; ++dir) {
        test_create_region(x+delta_x[dir], y+delta_y[dir], 0);
    }
}

static void test_prepare_seen(CuTest *tc) {
    region *r;
    faction *f;
    unit *u;

    test_cleanup();
    f = test_create_faction(0);
    r = test_create_region(0, 0, 0);
    u = test_create_unit(f, r);
    f->seen = seen_init();
    add_seen(f->seen, r, see_unit, false);
    setup_seen(0, 0);
    r = test_create_region(2, 2, 0);
    setup_seen(2, 2);
    travelthru_add(r, u);

    init_reports();
    prepare_seen(f);
    CuAssertPtrEquals(tc, regions, f->first);
    CuAssertPtrEquals(tc, 0, f->last);
    seen_done(f->seen);
    test_cleanup();
}

static void test_seen_travelthru(CuTest *tc) {
    seen_region *sr;
    region *r;
    faction *f;
    unit *u;

    test_cleanup();
    setup_seen(0, 0);
    r = test_create_region(0, 0, 0);
    f = test_create_faction(0);
    u = test_create_unit(f, 0);
    travelthru_add(r, u);
    init_reports();
    view_default(f->seen, r, f);
    get_seen_interval(f->seen, &f->first, &f->last);
    link_seen(f->seen, f->first, f->last);
    CuAssertPtrEquals(tc, regions, f->first);
    CuAssertPtrEquals(tc, 0, f->last);
    sr = find_seen(f->seen, regions);
    CuAssertPtrEquals(tc, regions, sr->r);
    CuAssertIntEquals(tc, see_neighbour, sr->mode);
    sr = find_seen(f->seen, r);
    CuAssertPtrEquals(tc, r, sr->r);
    CuAssertIntEquals(tc, see_travel, sr->mode);
    test_cleanup();
}

static void test_seen_region(CuTest *tc) {
    seen_region **seen, *sr;
    region *r;

    test_cleanup();
    setup_seen(0, 0);
    r = test_create_region(0, 0, 0);
    seen = seen_init();
    add_seen(seen, r, see_unit, false);
    sr = find_seen(seen,  r);
    CuAssertPtrEquals(tc, r, sr->r);
    seen_done(seen);
    test_cleanup();
}

static void test_seen_interval_backward(CuTest *tc) {
    region *r, *first, *last;
    seen_region **seen;

    test_cleanup();
    r = test_create_region(0, 0, 0);
    setup_seen(0, 0);
    seen = seen_init();
    add_seen(seen, r, see_unit, false);
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
    seen_region **seen;

    test_cleanup();
    setup_seen(0, 0);
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

static void cb_testmap(seen_region *sr, void *cbdata) {
    int *ip = (int *)cbdata;
    *ip += sr->r->y;
}

static void test_seenhash_map(CuTest *tc) {
    region *r;
    seen_region **seen;
    int i = 0;

    test_cleanup();
    seen = seen_init();
    r = test_create_region(1, 1, 0);
    add_seen(seen, r, see_unit, false);
    r = test_create_region(2, 2, 0);
    add_seen(seen, r, see_unit, false);
    seenhash_map(seen, cb_testmap, &i);
    CuAssertIntEquals(tc, 3, i);
    seen_done(seen);
    test_cleanup();
}

CuSuite *get_seen_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_prepare_seen);
    SUITE_ADD_TEST(suite, test_seen_travelthru);
    SUITE_ADD_TEST(suite, test_seen_region);
    SUITE_ADD_TEST(suite, test_seen_interval_backward);
    SUITE_ADD_TEST(suite, test_seen_interval_forward);
    SUITE_ADD_TEST(suite, test_seenhash_map);
    return suite;
}
