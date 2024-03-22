#include "skills.h"

#include "config.h"
#include "skill.h"
#include "unit.h"

#include <tests.h>

#include <stb_ds.h>
#include <CuTest.h>

#include <stddef.h>

static void test_skill_set(CuTest *tc)
{
    skill value = { .id = SK_ALCHEMY, .old = 1, .level = 1 };

    test_setup();
    config_set_int("study.random_progress", 0);
    sk_set_level(&value, 2);
    CuAssertIntEquals(tc, 1, value.old);
    CuAssertIntEquals(tc, 2, value.level);
    CuAssertIntEquals(tc, 3 * SKILL_DAYS_PER_WEEK, value.days);
}

static void test_skill_change(CuTest *tc)
{
    unit *u;
    skill *sv;

    test_setup();
    config_set_int("study.random_progress", 0);
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    increase_skill_weeks(u, SK_CROSSBOW, 1);
    sv = unit_skill(u, SK_CROSSBOW);
    CuAssertIntEquals(tc, 1, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 1, sv->level);
    CuAssertIntEquals(tc, 2 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 2 * SKILL_DAYS_PER_WEEK, sv->days);
    increase_skill_weeks(u, SK_CROSSBOW, 1);
    CuAssertIntEquals(tc, 1, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 1, sv->level);
    CuAssertIntEquals(tc, 1 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 1 * SKILL_DAYS_PER_WEEK, sv->days);
    increase_skill_weeks(u, SK_CROSSBOW, 1);
    CuAssertIntEquals(tc, 2, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 2, sv->level);
    CuAssertIntEquals(tc, 3 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 3 * SKILL_DAYS_PER_WEEK, sv->days);
    increase_skill_weeks(u, SK_CROSSBOW, 4);
    CuAssertIntEquals(tc, 3, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 3, sv->level);
    CuAssertIntEquals(tc, 3 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 3 * SKILL_DAYS_PER_WEEK, sv->days);

    reduce_skill_weeks(u, sv, 1);
    CuAssertIntEquals(tc, 3, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 3, sv->level);
    CuAssertIntEquals(tc, 4 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 4 * SKILL_DAYS_PER_WEEK, sv->days);
    reduce_skill_weeks(u, sv, 1);
    CuAssertIntEquals(tc, 3, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 3, sv->level);
    CuAssertIntEquals(tc, 5 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 5 * SKILL_DAYS_PER_WEEK, sv->days);
    reduce_skill_weeks(u, sv, 2);
    CuAssertIntEquals(tc, 3, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 3, sv->level);
    CuAssertIntEquals(tc, 7 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 7 * SKILL_DAYS_PER_WEEK, sv->days);
    reduce_skill_weeks(u, sv, 1);
    CuAssertIntEquals(tc, 2, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 2, sv->level);
    CuAssertIntEquals(tc, 5 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 5 * SKILL_DAYS_PER_WEEK, sv->days);
    sv->level = 10;
    reduce_skill_weeks(u, sv, 25);
    CuAssertIntEquals(tc, 8, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 8, sv->level);
    CuAssertIntEquals(tc, 11 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 11 * SKILL_DAYS_PER_WEEK, sv->days);
}

static void test_set_level(CuTest * tc)
{
    unit *u;

    test_setup();
    config_set_int("study.random_progress", 0);
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    CuAssertPtrEquals(tc, NULL, u->skills);
    CuAssertIntEquals(tc, 0, get_level(u, SK_CROSSBOW));
    set_level(u, SK_CROSSBOW, 1);
    CuAssertPtrNotNull(tc, u->skills);
    CuAssertIntEquals(tc, 1, (int)arrlen(u->skills));
    CuAssertIntEquals(tc, SK_CROSSBOW, u->skills->id);
    CuAssertIntEquals(tc, 1, u->skills->level);
    CuAssertIntEquals(tc, 2 * SKILL_DAYS_PER_WEEK, u->skills->days);
    CuAssertIntEquals(tc, 1, get_level(u, SK_CROSSBOW));
    set_level(u, SK_CROSSBOW, 0);
    CuAssertPtrEquals(tc, NULL, u->skills);
    test_teardown();
}

static void test_skills_merge(CuTest* tc)
{
    skill src, dst, result;
    test_setup();
    config_set_int("study.random_progress", 0);

    /* simple case:
     * two units with the same skills merge.
     */
    dst.level = 2;
    dst.days = 3 * SKILL_DAYS_PER_WEEK;
    CuAssertIntEquals(tc, 2, merge_skill(&dst, &dst, &result, 1, 2));
    CuAssertIntEquals(tc, 2, result.level);
    CuAssertIntEquals(tc, 3 * SKILL_DAYS_PER_WEEK, result.days);

    /* two-person lucky unit gets merged in*/
    src.level = 2;
    src.days = 2 * SKILL_DAYS_PER_WEEK;
    CuAssertIntEquals(tc, 2, merge_skill(&src, &dst, &result, 1, 2));
    CuAssertIntEquals(tc, 2, result.level);
    CuAssertIntEquals(tc, 7 * SKILL_DAYS_PER_WEEK / 3, result.days);

    /* unlucky unit gets merged */
    src.level = 2;
    src.days = 4 * SKILL_DAYS_PER_WEEK;
    CuAssertIntEquals(tc, 2, merge_skill(&src, &dst, &result, 1, 1));
    CuAssertIntEquals(tc, 2, result.level);
    CuAssertIntEquals(tc, 7 * SKILL_DAYS_PER_WEEK / 2, result.days);

    /* good and bad luck cancel out */
    dst.level = 2;
    dst.days = 2 * SKILL_DAYS_PER_WEEK;
    CuAssertIntEquals(tc, 2, merge_skill(&src, &dst, &result, 100, 100));
    CuAssertIntEquals(tc, 2, result.level);
    CuAssertIntEquals(tc, 3 * SKILL_DAYS_PER_WEEK, result.days);

    /* 1xT2 merges with 2xT0 to become 3xT1 */
    dst.level = 2;
    dst.days = 3 * SKILL_DAYS_PER_WEEK;
    CuAssertIntEquals(tc, 1, merge_skill(NULL, &dst, &result, 1, 2));
    CuAssertIntEquals(tc, 1, result.level);
    CuAssertIntEquals(tc, 2 * SKILL_DAYS_PER_WEEK, result.days);

    /* 1xT2 merges with 3xT0, skill goes poof */
    CuAssertIntEquals(tc, 0, merge_skill(NULL, &dst, &result, 1, 3));
    CuAssertIntEquals(tc, 0, result.level);
    CuAssertIntEquals(tc, SKILL_DAYS_PER_WEEK - 3 * SKILL_DAYS_PER_WEEK / 4, result.days);

    /* two units with the same skills are merged: no change */
    src.level = 2;
    src.days = 3 * SKILL_DAYS_PER_WEEK;
    CuAssertIntEquals(tc, 2, merge_skill(&src, &dst, &result, 1, 1));
    CuAssertIntEquals(tc, 2, result.level);
    CuAssertIntEquals(tc, 3 * SKILL_DAYS_PER_WEEK, result.days);

    /* T4 + T6 always makes T5 */
    src.level = 4;
    dst.level = 6;

    /* close as possible to next level: (T4, 1) + (T6, 1) = (T5, 1) */
    src.days = 1 * SKILL_DAYS_PER_WEEK;
    dst.days = 1 * SKILL_DAYS_PER_WEEK;
    CuAssertIntEquals(tc, 5, merge_skill(&src, &dst, &result, 1, 1));
    CuAssertIntEquals(tc, 5, result.level);
    CuAssertIntEquals(tc, SKILL_DAYS_PER_WEEK / 2, result.days);

    /* max. far from next level: (T4, 9) + (T6, 13) = (T5, 11) */
    src.days = MAX_DAYS_TO_NEXT_LEVEL(src.level);
    dst.days = MAX_DAYS_TO_NEXT_LEVEL(dst.level);
    CuAssertIntEquals(tc, 5, merge_skill(&src, &dst, &result, 1, 1));
    CuAssertIntEquals(tc, 5, result.level);
    CuAssertIntEquals(tc, 21 * SKILL_DAYS_PER_WEEK / 2, result.days);

    /* extreme values: (T99, 1) + (T1, 1) = (T70, 30) */
    src.level = 99;
    dst.level = 1;
    src.days = 1 * SKILL_DAYS_PER_WEEK;
    dst.days = 1 * SKILL_DAYS_PER_WEEK;
    CuAssertIntEquals(tc, 70, merge_skill(&src, &dst, &result, 1, 1));
    CuAssertIntEquals(tc, 70, result.level);
    CuAssertIntEquals(tc, 23 * SKILL_DAYS_PER_WEEK / 2, result.days);

    /* extreme values: (T99, 199) + (T1, 3) = (T69, 60) */
    src.days = (src.level * 2 + 1) * SKILL_DAYS_PER_WEEK;
    dst.days = (dst.level * 2 + 1) * SKILL_DAYS_PER_WEEK;
    CuAssertIntEquals(tc, 69, merge_skill(&src, &dst, &result, 1, 1));
    CuAssertIntEquals(tc, 69, result.level);
    CuAssertIntEquals(tc, 61 * SKILL_DAYS_PER_WEEK, result.days);

    test_teardown();
}

CuSuite *get_skills_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_skill_set);
    SUITE_ADD_TEST(suite, test_skill_change);
    SUITE_ADD_TEST(suite, test_set_level);
    SUITE_ADD_TEST(suite, test_skills_merge);
    return suite;
}

