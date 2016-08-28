#include <platform.h>

#include "names.h"

#include <kernel/race.h>
#include <kernel/unit.h>
#include <util/language.h>
#include <util/functions.h>

#include <CuTest.h>
#include "tests.h"

static void test_names(CuTest * tc)
{
    race_name_func foo;
    unit *u;
    race *rc;
    test_cleanup();
    register_names();
    default_locale = test_create_locale();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    rc = test_create_race("undead");
    locale_setstring(default_locale, "undead_name_0", "Graue");
    locale_setstring(default_locale, "undead_postfix_0", "Kobolde");
    CuAssertPtrNotNull(tc, foo = (race_name_func)get_function("nameundead"));
    rc->generate_name = foo;
    race_namegen(rc, u);
    CuAssertStrEquals(tc, "Graue Kobolde", u->_name);
    CuAssertPtrNotNull(tc, get_function("nameskeleton"));
    CuAssertPtrNotNull(tc, get_function("namezombie"));
    CuAssertPtrNotNull(tc, get_function("nameghoul"));
    CuAssertPtrNotNull(tc, get_function("namedragon"));
    CuAssertPtrNotNull(tc, get_function("namedracoid"));
    CuAssertPtrNotNull(tc, get_function("namegeneric"));
    CuAssertPtrNotNull(tc, get_function("describe_race"));
    test_cleanup();
}

CuSuite *get_names_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_names);
    return suite;
}
