#include "rng.h"
#include "rand.h"
#include "tests.h"

#include <CuTest.h>
#include <ctype.h>

static void test_rng_round(CuTest * tc)
{
    double f, r;

    test_setup();
    f = rng_double();
    r = RAND_ROUND(f);
    CuAssertTrue(tc, f >= 0);
    CuAssertTrue(tc, r <= f + 1);
    CuAssertTrue(tc, r >= f);
    CuAssertTrue(tc, r == r);
    CuAssertTrue(tc, r == RAND_ROUND(r));
}

static void test_dice_rand(CuTest* tc)
{
    CuAssertIntEquals(tc, 9, dice_rand("3*(2+1)"));
    CuAssertIntEquals(tc, 0, dice_rand("0"));
    CuAssertIntEquals(tc, -5, dice_rand("-5"));
    CuAssertIntEquals(tc, 1, dice_rand("d1"));
    CuAssertIntEquals(tc, 2, dice_rand("2d1"));
    CuAssertIntEquals(tc, 3, dice_rand("1+2"));
    CuAssertIntEquals(tc, 6, dice_rand("3*2"));
    CuAssertIntEquals(tc, -6, dice_rand("-3*2"));
}

CuSuite *get_rng_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_rng_round);
    SUITE_ADD_TEST(suite, test_dice_rand);
    return suite;
}
