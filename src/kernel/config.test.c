#include <platform.h>
#include <stdlib.h>

#include <kernel/config.h>

#include <CuTest.h>
#include <tests.h>

static void test_get_set_param(CuTest * tc)
{
    struct param *par = 0;
    test_cleanup();
    CuAssertStrEquals(tc, 0, get_param(par, "foo"));
    set_param(&par, "foo", "bar");
    set_param(&par, "bar", "foo");
    CuAssertStrEquals(tc, "bar", get_param(par, "foo"));
    CuAssertStrEquals(tc, "foo", get_param(par, "bar"));
}

static void test_param_int(CuTest * tc)
{
    struct param *par = 0;
    test_cleanup();
    CuAssertIntEquals(tc, 13, get_param_int(par, "foo", 13));
    set_param(&par, "foo", "23");
    set_param(&par, "bar", "42");
    CuAssertIntEquals(tc, 23, get_param_int(par, "foo", 0));
    CuAssertIntEquals(tc, 42, get_param_int(par, "bar", 0));
}

static void test_param_flt(CuTest * tc)
{
    struct param *par = 0;
    test_cleanup();
    CuAssertDblEquals(tc, 13, get_param_flt(par, "foo", 13), 0.01);
    set_param(&par, "foo", "23.0");
    set_param(&par, "bar", "42.0");
    CuAssertDblEquals(tc, 23.0, get_param_flt(par, "foo", 0.0), 0.01);
    CuAssertDblEquals(tc, 42.0, get_param_flt(par, "bar", 0.0), 0.01);
}

CuSuite *get_config_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_get_set_param);
  SUITE_ADD_TEST(suite, test_param_int);
  SUITE_ADD_TEST(suite, test_param_flt);
  return suite;
}
