#include <CuTest.h>
#include <ctype.h>

#include "rng.h"

static void test_rng_round(CuTest * tc)
{
    double f;
    int i,r;
    for (i=0; i<1000; ++i) {
	f = rng_double();
	r = RAND_ROUND(f);
	CuAssertTrue(tc, f >= 0);
	CuAssertTrue(tc, r <= (int) f+1);
	CuAssertTrue(tc, r >= (int) f);
	CuAssertTrue(tc, r == (int) r);
	CuAssertTrue(tc, r == RAND_ROUND(r));
    }
}

CuSuite *get_rng_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_rng_round);
  return suite;
}
