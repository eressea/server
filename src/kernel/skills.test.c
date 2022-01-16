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
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
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

static void test_skills_merge(CuTest* tc)
{
    skill src, dst, result;
    test_setup();
    config_set_int("study.random_progress", 0);

    /* 1xT2 merges with 2xT0 to become 3xT1 */
    dst.level = 2;
    dst.weeks = 3;
    CuAssertIntEquals(tc, 1, merge_skill(NULL, &dst, &result, 1, 2));
    CuAssertIntEquals(tc, 1, result.level);
    CuAssertIntEquals(tc, 2, result.weeks);

    /* 1xT2 merges with 3xT0, skill goes poof */
    CuAssertIntEquals(tc, 0, merge_skill(NULL, &dst, &result, 1, 3));
    CuAssertIntEquals(tc, 0, result.level);
    CuAssertIntEquals(tc, 1, result.weeks);

    /* two units with the same skills are merged: no change */
    src.level = 2;
    src.weeks = 3;
    CuAssertIntEquals(tc, 2, merge_skill(&src, &dst, &result, 1, 1));
    CuAssertIntEquals(tc, 2, result.level);
    CuAssertIntEquals(tc, 3, result.weeks);

    /* T4 + T6 always makes T5 */
    src.level = 4;
    dst.level = 6;

    /* close as possible to next level: (T4, 1) + (T6, 1) = (T5, 1) */
    src.weeks = 1;
    dst.weeks = 1;
    CuAssertIntEquals(tc, 5, merge_skill(&src, &dst, &result, 1, 1));
    CuAssertIntEquals(tc, 5, result.level);
    CuAssertIntEquals(tc, 1, result.weeks);

    /* max. far from next level: (T4, 10) + (T6, 14) = (T5, 11) */
    src.weeks = src.level * 2 + 2;
    dst.weeks = dst.level * 2 + 2;
    CuAssertIntEquals(tc, 5, merge_skill(&src, &dst, &result, 1, 1));
    CuAssertIntEquals(tc, 5, result.level);
    CuAssertIntEquals(tc, 11, result.weeks);

    /* extreme values: (T99, 1) + (T1, 1) = (T70, 30) */
    src.level = 99;
    dst.level = 1;
    src.weeks = 1;
    dst.weeks = 1;
    CuAssertIntEquals(tc, 70, merge_skill(&src, &dst, &result, 1, 1));
    CuAssertIntEquals(tc, 70, result.level);
    CuAssertIntEquals(tc, 30, result.weeks);

    /* extreme values: (T99, 200) + (T1, 4) = (T69, 1) */
    src.weeks = src.level * 2 + 2;
    dst.weeks = dst.level * 2 + 2;
    CuAssertIntEquals(tc, 69, merge_skill(&src, &dst, &result, 1, 1));
    CuAssertIntEquals(tc, 69, result.level);
    CuAssertIntEquals(tc, 61, result.weeks);

    test_teardown();
}

CuSuite *get_skills_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_skills);
    SUITE_ADD_TEST(suite, test_skills_merge);
    return suite;
}

