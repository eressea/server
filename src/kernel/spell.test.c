#include <platform.h>

#include <kernel/spell.h>
#include <kernel/spellbook.h>

#include <util/log.h>
#include <util/lists.h>

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
    struct log_t *log;
    strlist *sl = 0;

    test_setup();
    test_log_stderr(0);
    log = test_log_start(LOG_CPERROR, &sl);

    CuAssertPtrEquals(tc, 0, find_spell("testspell"));

    sp = create_spell("testspell", 0);
    CuAssertPtrEquals(tc, 0, create_spell("testspell", 0));
    CuAssertPtrNotNull(tc, sl);
    CuAssertStrEquals(tc, "create_spell: duplicate name '%s'", sl->s);
    CuAssertPtrEquals(tc, 0, sl->next);
    CuAssertPtrEquals(tc, sp, find_spell("testspell"));
    test_log_stop(log, sl);
    test_cleanup();
}

static void test_create_spell_with_id(CuTest * tc)
{
    spell *sp;
    struct log_t *log;
    strlist *sl = 0;

    test_setup();
    test_log_stderr(0);
    log = test_log_start(LOG_CPERROR, &sl);

    CuAssertPtrEquals(tc, 0, find_spellbyid(42));
    sp = create_spell("testspell", 42);
    CuAssertPtrEquals(tc, sp, find_spellbyid(42));
    CuAssertPtrEquals(tc, 0, create_spell("testspell", 47));
    CuAssertPtrEquals(tc, 0, find_spellbyid(47));
    CuAssertPtrNotNull(tc, sl);
    CuAssertStrEquals(tc, "create_spell: duplicate name '%s'", sl->s);
    CuAssertPtrEquals(tc, 0, sl->next);
    test_log_stop(log, sl);
    test_cleanup();
}

static void test_spellref(CuTest *tc)
{
    spellref *ref;
    spell *sp;
    test_setup();
    ref = spellref_create("hodor");
    CuAssertPtrNotNull(tc, ref);
    CuAssertPtrEquals(tc, NULL, spellref_get(ref));
    sp = create_spell("hodor", 0);
    CuAssertPtrNotNull(tc, sp);
    CuAssertPtrEquals(tc, sp, spellref_get(ref));
    CuAssertPtrEquals(tc, NULL, ref->name);
    spellref_free(ref);
    test_cleanup();
}

CuSuite *get_spell_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_spellref);
    SUITE_ADD_TEST(suite, test_create_a_spell);
    SUITE_ADD_TEST(suite, test_create_duplicate_spell);
    SUITE_ADD_TEST(suite, test_create_spell_with_id);
    return suite;
}
