#include "direction.h"
#include "tests.h"

#include <util/language.h>

#include <CuTest.h>

static void test_init_directions(CuTest *tc) {
    struct locale *lang;

    test_setup();
    lang = get_or_create_locale("de");
    locale_setstring(lang, "dir_nw", "NW");
    init_directions(lang);
    CuAssertIntEquals(tc, D_NORTHWEST, get_direction("nw", lang));
    test_teardown();
}

static void test_init_direction(CuTest *tc) {
    struct locale *lang;
    test_setup();

    lang = get_or_create_locale("de");
    init_direction(lang, D_NORTHWEST, "NW");
    init_direction(lang, D_NORTHWEST, "northwest");
    init_direction(lang, D_EAST, "OST");
    init_direction(lang, D_EAST, "O");
    CuAssertIntEquals(tc, D_NORTHWEST, get_direction("nw", lang));
    CuAssertIntEquals(tc, D_NORTHWEST, get_direction("northwest", lang));
    CuAssertIntEquals(tc, D_EAST, get_direction("ost", lang));
    CuAssertIntEquals(tc, D_EAST, get_direction("O", lang));
    CuAssertIntEquals(tc, NODIRECTION, get_direction("east", lang));
    test_teardown();
}

static void test_finddirection(CuTest *tc) {
    test_setup();
    CuAssertIntEquals(tc, D_SOUTHWEST, finddirection("southwest"));
    CuAssertIntEquals(tc, D_SOUTHEAST, finddirection("southeast"));
    CuAssertIntEquals(tc, D_NORTHWEST, finddirection("northwest"));
    CuAssertIntEquals(tc, D_NORTHEAST, finddirection("northeast"));
    CuAssertIntEquals(tc, D_WEST, finddirection("west"));
    CuAssertIntEquals(tc, D_EAST, finddirection("east"));
    CuAssertIntEquals(tc, D_PAUSE, finddirection("pause"));
    CuAssertIntEquals(tc, NODIRECTION, finddirection(""));
    CuAssertIntEquals(tc, NODIRECTION, finddirection("potato"));
    test_teardown();
}

static void test_direction_reverse(CuTest *tc) {
    test_setup();
    CuAssertIntEquals(tc, D_SOUTHWEST, d_reverse(D_NORTHEAST));
    CuAssertIntEquals(tc, D_SOUTHEAST, d_reverse(D_NORTHWEST));
    CuAssertIntEquals(tc, D_NORTHEAST, d_reverse(D_SOUTHWEST));
    CuAssertIntEquals(tc, D_NORTHWEST, d_reverse(D_SOUTHEAST));
    CuAssertIntEquals(tc, D_EAST, d_reverse(D_WEST));
    CuAssertIntEquals(tc, D_WEST, d_reverse(D_EAST));
    test_teardown();
}

CuSuite *get_direction_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_init_direction);
    SUITE_ADD_TEST(suite, test_init_directions);
    SUITE_ADD_TEST(suite, test_finddirection);
    SUITE_ADD_TEST(suite, test_direction_reverse);
    return suite;
}

