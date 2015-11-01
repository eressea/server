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
    CuAssertIntEquals(tc, SK_MAGIC, findskill("magic"));
    CuAssertIntEquals(tc, SK_CROSSBOW, findskill("crossbow"));
    CuAssertIntEquals(tc, NOSKILL, findskill(""));
    CuAssertIntEquals(tc, NOSKILL, findskill("potato"));
}

CuSuite *get_skill_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_init_skill);
    SUITE_ADD_TEST(suite, test_init_skills);
    SUITE_ADD_TEST(suite, test_get_skill);
    return suite;
}

