#include "variant.h"

#include <CuTest.h>

static void test_fractions(CuTest *tc) {
    variant a, b;
    a = frac_make(120, 12000);
    CuAssertIntEquals(tc, 1, a.sa[0]);
    CuAssertIntEquals(tc, 100, a.sa[1]);
    b = frac_make(23, 2300);
    a = frac_add(a, b);
    CuAssertIntEquals(tc, 1, a.sa[0]);
    CuAssertIntEquals(tc, 50, a.sa[1]);
    a = frac_mul(a, b);
    CuAssertIntEquals(tc, 1, a.sa[0]);
    CuAssertIntEquals(tc, 5000, a.sa[1]);
    a = frac_div(b, b);
    CuAssertIntEquals(tc, 1, a.sa[0]);
    CuAssertIntEquals(tc, 1, a.sa[1]);
    a = frac_sub(a, a);
    CuAssertIntEquals(tc, 0, a.sa[0]);
    a = frac_sub(frac_one, a);
    CuAssertIntEquals(tc, 1, a.sa[0]);
    CuAssertIntEquals(tc, 1, a.sa[1]);
    a = frac_mul(a, frac_zero);
    CuAssertIntEquals(tc, 0, a.sa[0]);
    CuAssertIntEquals(tc, 1, frac_sign(frac_make(-1, -1)));
    CuAssertIntEquals(tc, 1, frac_sign(frac_make(1, 1)));
    CuAssertIntEquals(tc, -1, frac_sign(frac_make(-1, 1)));
    CuAssertIntEquals(tc, -1, frac_sign(frac_make(1, -1)));
    CuAssertIntEquals(tc, 0, frac_sign(frac_make(0, 1)));

    /* we reduce large integers by calculating the gcd */
    a = frac_make(480000, 3000);
    CuAssertIntEquals(tc, 160, a.sa[0]);
    CuAssertIntEquals(tc, 1, a.sa[1]);

    /* if num is too big for a short, and the gcd is 1, we cheat: */
    a = frac_make(480001, 3000);
    CuAssertIntEquals(tc, 32000, a.sa[0]);
    CuAssertIntEquals(tc, 200, a.sa[1]);
    
    a = frac_make(0, 0);
    b = frac_make(4, 1);
    a = frac_add(b, a);
    CuAssertIntEquals(tc, 4, a.sa[0]);
    CuAssertIntEquals(tc, 1, a.sa[1]);
    
    a = frac_make(0, 0);
    b = frac_make(4, 1);
    a = frac_add(a, b);
    CuAssertIntEquals(tc, 4, a.sa[0]);
    CuAssertIntEquals(tc, 1, a.sa[1]);
}

CuSuite *get_variant_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_fractions);
    return suite;
}
