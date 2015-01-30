#include <platform.h>
#include "skill.h"
#include "util/language.h"
#include "tests.h"

#include <CuTest.h>

static void test_init_skills(CuTest *tc) {
    struct locale *lang;

    test_cleanup();
    lang = get_or_create_locale("de");
    init_skill(lang, SK_ALCHEMY, "Alchemie");
    CuAssertIntEquals(tc, SK_ALCHEMY, get_skill("alchemie", lang));
    test_cleanup();
}

static void test_init_skill(CuTest *tc) {
    struct locale *lang;
    test_cleanup();

    lang = get_or_create_locale("de");
    init_skill(lang, SK_ALCHEMY, "Alchemie");
    CuAssertIntEquals(tc, SK_ALCHEMY, get_skill("alchemie", lang));
    CuAssertIntEquals(tc, NOSKILL, get_skill("east", lang));
    test_cleanup();
}

static void test_get_skill(CuTest *tc) {
    test_cleanup();
    CuAssertIntEquals(tc, SK_ALCHEMY, findskill("alchemy"));
    CuAssertIntEquals(tc, SK_CROSSBOW, findskill("crossbow"));
    CuAssertIntEquals(tc, NOSKILL, findskill(""));
    CuAssertIntEquals(tc, NOSKILL, findskill("potato"));
}

static void test_get_skill_default(CuTest *tc) {
    struct locale *lang;
    test_cleanup();
    lang = get_or_create_locale("en");
    CuAssertIntEquals(tc, NOSKILL, get_skill("potato", lang));
    CuAssertIntEquals(tc, SK_ALCHEMY, get_skill("alchemy", lang));
    CuAssertIntEquals(tc, SK_CROSSBOW, get_skill("crossbow", lang));
}

#define SUITE_DISABLE_TEST(suite, test) (void)test

CuSuite *get_skill_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_init_skill);
    SUITE_ADD_TEST(suite, test_init_skills);
    SUITE_ADD_TEST(suite, test_get_skill);
    SUITE_DISABLE_TEST(suite, test_get_skill_default);
    return suite;
}

