#include <CuTest.h>
#include <stdio.h>
#include <string.h>
#include "functions.h"


static void test_functions(CuTest * tc)
{
    pf_generic fun;

    fun = get_function("herpderp");
    CuAssertTrue(tc, !fun);
    register_function((pf_generic)test_functions, "herpderp");
    fun = get_function("herpderp");
    CuAssertTrue(tc, fun == (pf_generic)test_functions);
}

CuSuite *get_functions_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_functions);
    return suite;
}
