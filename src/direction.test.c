#include <platform.h>
#include <kernel/types.h>

#include "direction.h"
#include "util/language.h"
#include "tests.h"

#include <kernel/config.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/race.h>
#include <kernel/terrain.h>

#include <CuTest.h>

static void test_init_directions(CuTest *tc) {
    struct locale *lang;

    test_cleanup();
    lang = get_or_create_locale("en");
    locale_setstring(lang, "dir_nw", "NW");
    init_directions(lang);
    CuAssertIntEquals(tc, D_NORTHWEST, get_direction("nw", lang));
    test_cleanup();
}

static void test_init_direction(CuTest *tc) {
    struct locale *lang;
    test_cleanup();

    lang = get_or_create_locale("de");
    init_direction(lang, D_NORTHWEST, "NW");
    init_direction(lang, D_EAST, "OST");
    CuAssertIntEquals(tc, D_NORTHWEST, get_direction("nw", lang));
    CuAssertIntEquals(tc, D_EAST, get_direction("ost", lang));
    CuAssertIntEquals(tc, NODIRECTION, get_direction("east", lang));
    test_cleanup();
}

static void test_finddirection(CuTest *tc) {
    test_cleanup();
    CuAssertIntEquals(tc, D_SOUTHWEST, finddirection("southwest"));
    CuAssertIntEquals(tc, D_SOUTHEAST, finddirection("southeast"));
    CuAssertIntEquals(tc, D_NORTHWEST, finddirection("northwest"));
    CuAssertIntEquals(tc, D_NORTHEAST, finddirection("northeast"));
    CuAssertIntEquals(tc, D_WEST, finddirection("west"));
    CuAssertIntEquals(tc, D_EAST, finddirection("east"));
    CuAssertIntEquals(tc, D_PAUSE, finddirection("pause"));
    CuAssertIntEquals(tc, NODIRECTION, finddirection(""));
    CuAssertIntEquals(tc, NODIRECTION, finddirection("potato"));
}

static void test_get_direction_default(CuTest *tc) {
    struct locale *lang;
    test_cleanup();
    lang = get_or_create_locale("en");
    CuAssertIntEquals(tc, NODIRECTION, get_direction("potato", lang));
    CuAssertIntEquals(tc, D_SOUTHWEST, get_direction("southwest", lang));
    CuAssertIntEquals(tc, D_SOUTHEAST, get_direction("southeast", lang));
    CuAssertIntEquals(tc, D_NORTHWEST, get_direction("northwest", lang));
    CuAssertIntEquals(tc, D_NORTHEAST, get_direction("northeast", lang));
    CuAssertIntEquals(tc, D_WEST, get_direction("west", lang));
    CuAssertIntEquals(tc, D_EAST, get_direction("east", lang));
}

static void test_move_to_vortex(CuTest *tc) {
    region *r1, *r2, *r = 0;
    terrain_type *t_plain;
    unit *u;
    struct locale *lang;

    test_cleanup();
    lang = get_or_create_locale("en");
    locale_setstring(lang, "vortex", "wirbel");
    init_locale(lang);
    register_special_direction("vortex");
    t_plain = test_create_terrain("plain", LAND_REGION | FOREST_REGION | WALK_INTO | CAVALRY_REGION);
    r1 = test_create_region(0, 0, t_plain);
    r2 = test_create_region(5, 0, t_plain);
    CuAssertPtrNotNull(tc, create_special_direction(r1, r2, 10, "", "vortex", true));
    u = test_create_unit(test_create_faction(rc_get_or_create("hodor")), r1);
    CuAssertIntEquals(tc, E_MOVE_NOREGION, movewhere(u, "barf", r1, &r));
    CuAssertIntEquals(tc, E_MOVE_OK, movewhere(u, "wirbel", r1, &r));
    CuAssertPtrEquals(tc, r2, r);
}

#define SUITE_DISABLE_TEST(suite, test) (void)test

CuSuite *get_direction_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_init_direction);
  SUITE_ADD_TEST(suite, test_init_directions);
  SUITE_ADD_TEST(suite, test_finddirection);
  SUITE_ADD_TEST(suite, test_move_to_vortex);
  SUITE_DISABLE_TEST(suite, test_get_direction_default);
  return suite;
}

