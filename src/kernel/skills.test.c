#ifdef _MSC_VER
#include <platform.h>
#endif
#include "skills.h"

#include "config.h"
#include "unit.h"

#include <CuTest.h>
#include <tests.h>

static void test_skills(CuTest * tc)
{
    unit *u;

    test_setup();
    config_set_int("study.random_progress", 0);
    u = test_create_unit(test_create_faction(NULL), test_create_plain(0, 0));
    CuAssertPtrEquals(tc, NULL, u->skills);
    CuAssertIntEquals(tc, 0, u->skill_size);
    CuAssertIntEquals(tc, 0, get_level(u, SK_CROSSBOW));
    set_level(u, SK_CROSSBOW, 1);
    CuAssertPtrNotNull(tc, u->skills);
    CuAssertIntEquals(tc, 1, u->skill_size);
    CuAssertIntEquals(tc, SK_CROSSBOW, u->skills->id);
    CuAssertIntEquals(tc, 1, u->skills->level);
    CuAssertIntEquals(tc, 2, u->skills->weeks);
    CuAssertIntEquals(tc, 1, get_level(u, SK_CROSSBOW));
    set_level(u, SK_CROSSBOW, 0);
    CuAssertPtrEquals(tc, NULL, u->skills);
    CuAssertIntEquals(tc, 0, u->skill_size);
    test_teardown();
}

CuSuite *get_skills_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_skills);
    return suite;
}

