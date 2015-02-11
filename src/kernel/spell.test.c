#include <platform.h>

#include <quicklist.h>
#include <kernel/spell.h>

#include <CuTest.h>
#include <tests.h>

#include <stdlib.h>

static void test_create_spell(CuTest * tc)
{
    spell * sp;

    test_cleanup();
    CuAssertPtrEquals(tc, 0, spells);
    CuAssertPtrEquals(tc, 0, find_spell("testspell"));

    sp = create_spell("testspell", 0);
    CuAssertPtrEquals(tc, sp, find_spell("testspell"));
    CuAssertPtrNotNull(tc, spells);
}

static void test_create_duplicate_spell(CuTest * tc)
{
    spell *sp;

    test_cleanup();
    CuAssertPtrEquals(tc, 0, find_spell("testspell"));

    sp = create_spell("testspell", 0);
    CuAssertPtrEquals(tc, 0, create_spell("testspell", 0));
    CuAssertPtrEquals(tc, sp, find_spell("testspell"));
}

static void test_create_spell_with_id(CuTest * tc)
{
    spell *sp;

    test_cleanup();
    CuAssertPtrEquals(tc, 0, find_spellbyid(42));
    sp = create_spell("testspell", 42);
    CuAssertPtrEquals(tc, sp, find_spellbyid(42));
    CuAssertPtrEquals(tc, 0, create_spell("testspell", 47));
    CuAssertPtrEquals(tc, 0, find_spellbyid(47));
}

CuSuite *get_spell_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_create_spell);
    SUITE_ADD_TEST(suite, test_create_duplicate_spell);
    SUITE_ADD_TEST(suite, test_create_spell_with_id);
    return suite;
}
