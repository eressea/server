#include <platform.h>

#include "direction.h"
#include "tests.h"

#include <util/language.h>

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

#define SUITE_DISABLE_TEST(suite, test) (void)test

CuSuite *get_direction_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_init_direction);
    SUITE_ADD_TEST(suite, test_init_directions);
    SUITE_ADD_TEST(suite, test_finddirection);
    SUITE_DISABLE_TEST(suite, test_get_direction_default);
    return suite;
}

