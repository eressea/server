#include <platform.h>

#include "names.h"

#include <util/functions.h>

#include <CuTest.h>
#include "tests.h"

static void test_names(CuTest * tc)
{
    test_cleanup();
    register_names();
    CuAssertPtrNotNull(tc, get_function("nameundead"));
    CuAssertPtrNotNull(tc, get_function("nameskeleton"));
    CuAssertPtrNotNull(tc, get_function("namezombie"));
    CuAssertPtrNotNull(tc, get_function("nameghoul"));
    CuAssertPtrNotNull(tc, get_function("namedragon"));
    CuAssertPtrNotNull(tc, get_function("namedracoid"));
    CuAssertPtrNotNull(tc, get_function("namegeneric"));
    CuAssertPtrNotNull(tc, get_function("describe_braineater"));
    test_cleanup();
}

CuSuite *get_names_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_names);
    return suite;
}
