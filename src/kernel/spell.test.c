#include <platform.h>

#include <quicklist.h>
#include <kernel/spell.h>
#include <util/log.h>

#include <CuTest.h>
#include <tests.h>

#include <stdlib.h>

static void test_create_a_spell(CuTest * tc)
{
    spell * sp;

    test_setup();
    CuAssertPtrEquals(tc, 0, spells);
    CuAssertPtrEquals(tc, 0, find_spell("testspell"));

    sp = create_spell("testspell", 0);
    CuAssertPtrEquals(tc, sp, find_spell("testspell"));
    CuAssertPtrNotNull(tc, spells);
    test_cleanup();
}

static void test_create_duplicate_spell(CuTest * tc)
{
    spell *sp;
    /* FIXME: this test emits ERROR messages (duplicate spells), inject a logger to verify that */

    test_setup();
    test_log_stderr(0);
    CuAssertPtrEquals(tc, 0, find_spell("testspell"));

    sp = create_spell("testspell", 0);
    CuAssertPtrEquals(tc, 0, create_spell("testspell", 0));
    CuAssertPtrEquals(tc, sp, find_spell("testspell"));
    test_cleanup();
}

static void test_create_spell_with_id(CuTest * tc)
{
    spell *sp;
    /* FIXME: this test emits ERROR messages (duplicate spells), inject a logger to verify that */

    test_setup();
    test_log_stderr(0);
    CuAssertPtrEquals(tc, 0, find_spellbyid(42));
    sp = create_spell("testspell", 42);
    CuAssertPtrEquals(tc, sp, find_spellbyid(42));
    CuAssertPtrEquals(tc, 0, create_spell("testspell", 47));
    CuAssertPtrEquals(tc, 0, find_spellbyid(47));
    test_cleanup();
}

CuSuite *get_spell_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_create_a_spell);
    SUITE_ADD_TEST(suite, test_create_duplicate_spell);
    SUITE_ADD_TEST(suite, test_create_spell_with_id);
    return suite;
}
