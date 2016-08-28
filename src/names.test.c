#include <platform.h>

#include "names.h"

#include <kernel/race.h>
#include <util/language.h>
#include <util/functions.h>

#include <CuTest.h>
#include "tests.h"

static void test_names(CuTest * tc)
{
    race_name_func foo;
    test_cleanup();
    register_names();
    default_locale = test_create_locale();
    locale_setstring(default_locale, "undead_prefix_0", "Kleine");
    locale_setstring(default_locale, "undead_name_0", "Graue");
    locale_setstring(default_locale, "undead_postfix_0", "Kobolde");
    CuAssertPtrNotNull(tc, foo = (race_name_func)get_function("nameundead"));
    CuAssertStrEquals(tc, "Kleine Graue Kobolde", foo(NULL));
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
