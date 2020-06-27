#include <platform.h>

#include "kernel/config.h"
#include "util/language.h"
#include "skill.h"
#include "tests.h"

#include <CuTest.h>

static void test_init_skills(CuTest *tc) {
    struct locale *lang;

    test_setup();
    lang = get_or_create_locale("de");

    init_skill(lang, SK_ALCHEMY, "Alchemie");
    CuAssertIntEquals(tc, SK_ALCHEMY, get_skill("alchemie", lang));
    test_teardown();
}

static void test_init_skill(CuTest *tc) {
    struct locale *lang;

    test_setup();
    lang = get_or_create_locale("de");

    init_skill(lang, SK_ALCHEMY, "Alchemie");
    CuAssertIntEquals(tc, SK_ALCHEMY, get_skill("alchemie", lang));
    CuAssertIntEquals(tc, NOSKILL, get_skill("east", lang));
    test_teardown();
}

static void test_get_skill(CuTest *tc) {
    test_setup();
    CuAssertIntEquals(tc, SK_ALCHEMY, findskill("alchemy"));
    CuAssertIntEquals(tc, SK_MAGIC, findskill("magic"));
    CuAssertIntEquals(tc, SK_CROSSBOW, findskill("crossbow"));
    CuAssertIntEquals(tc, NOSKILL, findskill(""));
    CuAssertIntEquals(tc, NOSKILL, findskill("potato"));
    test_teardown();
}

static void test_skill_cost(CuTest *tc) {
    test_setup();
    CuAssertTrue(tc, expensive_skill(SK_MAGIC));
    CuAssertTrue(tc, expensive_skill(SK_TACTICS));
    CuAssertTrue(tc, expensive_skill(SK_SPY));
    CuAssertTrue(tc, expensive_skill(SK_ALCHEMY));
    CuAssertTrue(tc, expensive_skill(SK_HERBALISM));
    CuAssertTrue(tc, !expensive_skill(SK_CROSSBOW));

    CuAssertIntEquals(tc, 100, skill_cost(SK_SPY));
    CuAssertIntEquals(tc, 200, skill_cost(SK_TACTICS));
    CuAssertIntEquals(tc, 200, skill_cost(SK_ALCHEMY));
    CuAssertIntEquals(tc, 200, skill_cost(SK_HERBALISM));
    CuAssertIntEquals(tc, 0, skill_cost(SK_CROSSBOW));

    config_set_int("skills.cost.crossbow", 300);
    CuAssertIntEquals(tc, 300, skill_cost(SK_CROSSBOW));
    CuAssertTrue(tc, expensive_skill(SK_CROSSBOW));
    test_teardown();
}

CuSuite *get_skill_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_init_skill);
    SUITE_ADD_TEST(suite, test_init_skills);
    SUITE_ADD_TEST(suite, test_get_skill);
    SUITE_ADD_TEST(suite, test_skill_cost);
    return suite;
}

