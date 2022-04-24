#include "rand.h"
#include "tests.h"

#include <CuTest.h>

static void test_dice_rand(CuTest* tc)
{
    test_setup();

    random_source_inject_constants(0.0, 0);
    CuAssertIntEquals(tc, 1, dice_rand("1d10"));
    CuAssertIntEquals(tc, 1, dice_rand("d20"));
    CuAssertIntEquals(tc, 2, dice_rand("2d4"));

    CuAssertIntEquals(tc, 9, dice_rand("3*(2+1)"));
    CuAssertIntEquals(tc, 0, dice_rand("0"));
    CuAssertIntEquals(tc, -5, dice_rand("-5"));
    CuAssertIntEquals(tc, 1, dice_rand("d1"));
    CuAssertIntEquals(tc, 2, dice_rand("2d1"));
    CuAssertIntEquals(tc, 3, dice_rand("1+2"));
    CuAssertIntEquals(tc, 6, dice_rand("3*2"));
    CuAssertIntEquals(tc, -6, dice_rand("-3*2"));
}

CuSuite *get_rand_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_dice_rand);
    return suite;
}
