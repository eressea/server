#include <platform.h>
#include "kernel/types.h"
#include "direction.h"
#include "util/language.h"
#include "tests.h"

#include <CuTest.h>

void test_init_directions(CuTest *tc) {
    struct locale *lang;

    test_cleanup();
    lang = make_locale("en");
    locale_setstring(lang, "dir_nw", "NW");
    init_directions(lang);
    CuAssertIntEquals(tc, D_NORTHWEST, finddirection("nw", lang));
    test_cleanup();
}

void test_init_direction(CuTest *tc) {
    struct locale *lang;
    test_cleanup();

    lang = make_locale("de");
    init_direction(lang, D_NORTHWEST, "NW");
    init_direction(lang, D_EAST, "OST");
    CuAssertIntEquals(tc, D_NORTHWEST, finddirection("nw", lang));
    CuAssertIntEquals(tc, D_EAST, finddirection("ost", lang));
    CuAssertIntEquals(tc, NODIRECTION, finddirection("east", lang));
    test_cleanup();
}

void test_finddirection_default(CuTest *tc) {
    struct locale *lang;
    test_cleanup();
    lang = make_locale("en");
    CuAssertIntEquals(tc, NODIRECTION, finddirection("potato", lang));
    CuAssertIntEquals(tc, D_SOUTHWEST, finddirection("southwest", lang));
    CuAssertIntEquals(tc, D_SOUTHEAST, finddirection("southeast", lang));
    CuAssertIntEquals(tc, D_NORTHWEST, finddirection("northwest", lang));
    CuAssertIntEquals(tc, D_NORTHEAST, finddirection("northeast", lang));
    CuAssertIntEquals(tc, D_WEST, finddirection("west", lang));
    CuAssertIntEquals(tc, D_EAST, finddirection("east", lang));
}

CuSuite *get_direction_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_init_direction);
  SUITE_ADD_TEST(suite, test_init_directions);
  SUITE_ADD_TEST(suite, test_finddirection_default);
  return suite;
}

