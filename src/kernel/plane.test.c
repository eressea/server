#include <platform.h>
#include <kernel/config.h>
#include "plane.h"
#include "faction.h"
#include <tests.h>
#include <CuTest.h>

static void test_plane(CuTest *tc) {
    struct region *r;
    plane *pl;

    test_cleanup();
    r = test_create_region(0, 0, 0);
    CuAssertPtrEquals(tc, 0, findplane(0, 0));
    CuAssertPtrEquals(tc, 0, getplane(r));
    CuAssertIntEquals(tc, 0, getplaneid(r));
    CuAssertPtrEquals(tc, 0, getplanebyid(0));
    CuAssertIntEquals(tc, 0, plane_center_x(0));
    CuAssertIntEquals(tc, 0, plane_center_y(0));
    CuAssertIntEquals(tc, 0, plane_width(0));
    CuAssertIntEquals(tc, 0, plane_height(0));
    CuAssertPtrEquals(tc, 0, get_homeplane());

    pl = create_new_plane(1, "Hell", 4, 8, 40, 80, 15);
    r = test_create_region(4, 40, 0);
    CuAssertIntEquals(tc, 15, pl->flags);
    CuAssertIntEquals(tc, 4, pl->minx);
    CuAssertIntEquals(tc, 8, pl->maxx);
    CuAssertIntEquals(tc, 40, pl->miny);
    CuAssertIntEquals(tc, 80, pl->maxy);
    CuAssertPtrEquals(tc, 0, pl->attribs);
    CuAssertStrEquals(tc, "Hell", pl->name);
    CuAssertPtrEquals(tc, pl, findplane(4, 40));
    CuAssertPtrEquals(tc, pl, getplane(r));
    CuAssertPtrEquals(tc, pl, getplanebyid(1));
    CuAssertIntEquals(tc, 1, getplaneid(r));
    CuAssertIntEquals(tc, 6, plane_center_x(pl));
    CuAssertIntEquals(tc, 60, plane_center_y(pl));
    CuAssertIntEquals(tc, 5, plane_width(pl));
    CuAssertIntEquals(tc, 41, plane_height(pl));
}

static void test_origin(CuTest *tc) {
    struct faction *f;
    int x, y;

    test_cleanup();
    f = test_create_faction(0);
    x = 0;
    y = 0;
    adjust_coordinates(f, &x, &y, 0);
    CuAssertIntEquals(tc, 0, x);
    CuAssertIntEquals(tc, 0, y);
    faction_setorigin(f, 0, 10, 20);
    adjust_coordinates(f, &x, &y, 0);
    CuAssertIntEquals(tc, -10, x);
    CuAssertIntEquals(tc, -20, y);
    test_cleanup();
}

CuSuite *get_plane_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_plane);
    SUITE_ADD_TEST(suite, test_origin);
    return suite;
}
