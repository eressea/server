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
    sk_set_level(NULL, &value, 2);
    CuAssertIntEquals(tc, 1, value.old);
    CuAssertIntEquals(tc, 2, value.level);
    CuAssertIntEquals(tc, 3 * SKILL_DAYS_PER_WEEK, value.days);
}

static int days_effort(const skill *sv)
{
    int n = sv->level + 1;
    int weeks = (n + 1) * n / 2;
    return weeks * SKILL_DAYS_PER_WEEK - sv->days;
}

static void test_skill_change(CuTest *tc)
{
    unit *u;
    skill *sv;

    test_setup();
    config_set_int("study.random_progress", 0);
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    set_number(u, 2); // number should have no effect on skill values
    change_skill_days(u, SK_CROSSBOW, SKILL_DAYS_PER_WEEK);
    CuAssertPtrNotNull(tc, sv = unit_skill(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 1 * SKILL_DAYS_PER_WEEK, days_effort(sv));
    CuAssertIntEquals(tc, 1, skill_level(u, SK_CROSSBOW));
    /* no random progress, so it will take 2 weeks of learning to next level: */
    CuAssertIntEquals(tc, 2 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
    change_skill_days(u, SK_CROSSBOW, SKILL_DAYS_PER_WEEK);
    CuAssertIntEquals(tc, 2 * SKILL_DAYS_PER_WEEK, days_effort(sv));
    CuAssertIntEquals(tc, 1, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 1 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
    change_skill_days(u, SK_CROSSBOW, SKILL_DAYS_PER_WEEK);
    CuAssertIntEquals(tc, 3 * SKILL_DAYS_PER_WEEK, days_effort(sv));
    CuAssertIntEquals(tc, 2, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 3 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
    change_skill_days(u, SK_CROSSBOW, 4 * SKILL_DAYS_PER_WEEK);
    CuAssertIntEquals(tc, 7 * SKILL_DAYS_PER_WEEK, days_effort(sv));
    CuAssertIntEquals(tc, 3, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 3 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
    change_skill_days(u, SK_CROSSBOW, -SKILL_DAYS_PER_WEEK);
    CuAssertIntEquals(tc, 6 * SKILL_DAYS_PER_WEEK, days_effort(sv));
    CuAssertIntEquals(tc, 3, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 4 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
    change_skill_days(u, SK_CROSSBOW, -SKILL_DAYS_PER_WEEK);
    CuAssertIntEquals(tc, 5 * SKILL_DAYS_PER_WEEK, days_effort(sv));
    CuAssertIntEquals(tc, 3, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 5 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
    change_skill_days(u, SK_CROSSBOW, -SKILL_DAYS_PER_WEEK * 2);
    CuAssertIntEquals(tc, 3 * SKILL_DAYS_PER_WEEK, days_effort(sv));
    CuAssertIntEquals(tc, 3, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 7 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
    change_skill_days(u, SK_CROSSBOW, -SKILL_DAYS_PER_WEEK);
    CuAssertIntEquals(tc, 2 * SKILL_DAYS_PER_WEEK, days_effort(sv));
    CuAssertIntEquals(tc, 2, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 4 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));

    sv->level = 10;
    sv->days = SKILL_DAYS_PER_WEEK;
    CuAssertIntEquals(tc, 65 * SKILL_DAYS_PER_WEEK, days_effort(sv));

    change_skill_days(u, SK_CROSSBOW, -25 * SKILL_DAYS_PER_WEEK);
    CuAssertIntEquals(tc, 40 * SKILL_DAYS_PER_WEEK, days_effort(sv));
    // 40 = (8 * 9) / 2 + 4
    CuAssertIntEquals(tc, 9, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 15 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
}

static void test_reduce_skill(CuTest *tc)
{
    unit *u;

    test_setup();
    config_set_int("study.random_progress", 0);
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    set_number(u, 2); // number should have no effect on skill values
    change_skill_days(u, SK_CROSSBOW, SKILL_DAYS_PER_WEEK);
    CuAssertIntEquals(tc, 1, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 2 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
    change_skill_days(u, SK_CROSSBOW, -SKILL_DAYS_PER_WEEK);
    CuAssertIntEquals(tc, 1, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 3 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
    change_skill_days(u, SK_CROSSBOW, -1);
    CuAssertPtrEquals(tc, NULL, u->skills);

    change_skill_days(u, SK_CROSSBOW, 3 * SKILL_DAYS_PER_WEEK);
    CuAssertIntEquals(tc, 2, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 3 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
    change_skill_days(u, SK_CROSSBOW, - 2 * SKILL_DAYS_PER_WEEK);
    CuAssertIntEquals(tc, SKILL_DAYS_PER_WEEK, days_effort(u->skills));
    CuAssertIntEquals(tc, 2, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 5 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
    change_skill_days(u, SK_CROSSBOW, -1);
    CuAssertIntEquals(tc, SKILL_DAYS_PER_WEEK - 1, days_effort(u->skills));
    CuAssertIntEquals(tc, 1, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 2 * SKILL_DAYS_PER_WEEK + 1, skill_days(u, SK_CROSSBOW));
    change_skill_days(u, SK_CROSSBOW, 2 - SKILL_DAYS_PER_WEEK);
    CuAssertIntEquals(tc, 1, days_effort(u->skills));
    change_skill_days(u, SK_CROSSBOW, -1);
    CuAssertIntEquals(tc, 1, skill_level(u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 3 * SKILL_DAYS_PER_WEEK, skill_days(u, SK_CROSSBOW));
    change_skill_days(u, SK_CROSSBOW, -1);
    CuAssertPtrEquals(tc, NULL, u->skills);
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
    CuAssertIntEquals(tc, 61 * SKILL_DAYS_PER_WEEK / 2, result.days);

    /* average values: (T99, 100) + (T1, 2) = (T69, 9.5) */
    src.days = (src.level + 1) * SKILL_DAYS_PER_WEEK;
    dst.days = (dst.level + 1) * SKILL_DAYS_PER_WEEK;
    CuAssertIntEquals(tc, 69, merge_skill(&src, &dst, &result, 1, 1));
    CuAssertIntEquals(tc, 69, result.level);
    CuAssertIntEquals(tc, 95 * SKILL_DAYS_PER_WEEK / 10, result.days);

    /* already made progress: (T99, 99) + (T1, 1) = (T69, 8.5) */
    src.days = src.level * SKILL_DAYS_PER_WEEK;
    dst.days = dst.level* SKILL_DAYS_PER_WEEK;
    CuAssertIntEquals(tc, 69, merge_skill(&src, &dst, &result, 1, 1));
    CuAssertIntEquals(tc, 69, result.level);
    CuAssertIntEquals(tc, 85 * SKILL_DAYS_PER_WEEK / 10, result.days);

    /* extreme values: (T99, 199) + (T1, 3) = (T69, 59.5) */
    src.days = (src.level * 2 + 1) * SKILL_DAYS_PER_WEEK;
    dst.days = (dst.level * 2 + 1) * SKILL_DAYS_PER_WEEK;
    CuAssertIntEquals(tc, 69, merge_skill(&src, &dst, &result, 1, 1));
    CuAssertIntEquals(tc, 69, result.level);
    CuAssertIntEquals(tc, (140 - 121 + 100) * SKILL_DAYS_PER_WEEK / 2, result.days);

    test_teardown();
}

CuSuite *get_skills_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_reduce_skill);
    SUITE_ADD_TEST(suite, test_skill_set);
    SUITE_ADD_TEST(suite, test_skill_change);
    SUITE_ADD_TEST(suite, test_set_level);
    SUITE_ADD_TEST(suite, test_skills_merge);
    return suite;
}

