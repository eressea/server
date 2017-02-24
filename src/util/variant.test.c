#include <platform.h>
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
}

CuSuite *get_variant_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_fractions);
    return suite;
}
