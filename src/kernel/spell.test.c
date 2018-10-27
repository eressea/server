#include <platform.h>

#include <kernel/spell.h>
#include <kernel/spellbook.h>

#include <util/log.h>
#include <util/lists.h>
#include <util/macros.h>

#include <CuTest.h>
#include <tests.h>

#include <stdlib.h>

static void test_create_a_spell(CuTest * tc)
{
    spell * sp;

    test_setup();
    CuAssertPtrEquals(tc, NULL, spells);
    CuAssertPtrEquals(tc, NULL, find_spell("testspell"));

    sp = create_spell("testspell");
    CuAssertPtrEquals(tc, sp, find_spell("testspell"));
    CuAssertPtrNotNull(tc, spells);
    test_teardown();
}

static void test_create_duplicate_spell(CuTest * tc)
{
    spell *sp;
    struct log_t *log;
    strlist *sl = 0;

    test_setup();
    test_inject_messagetypes();
    test_log_stderr(0); /* suppress the "duplicate spell" error message */
    log = test_log_start(LOG_CPERROR, &sl);

    CuAssertPtrEquals(tc, NULL, find_spell("testspell"));

    sp = create_spell("testspell");
    CuAssertPtrEquals(tc, NULL, create_spell("testspell"));
    CuAssertPtrNotNull(tc, sl);
    CuAssertStrEquals(tc, "create_spell: duplicate name '%s'", sl->s);
    CuAssertPtrEquals(tc, NULL, sl->next);
    CuAssertPtrEquals(tc, sp, find_spell("testspell"));
    test_log_stop(log, sl);
    test_log_stderr(1); /* or teardown complains that stderr logging is off */
    test_teardown();
}

static void test_spellref(CuTest *tc)
{
    spellref *ref;
    spell *sp;
    test_setup();
    ref = spellref_create(NULL, "hodor");
    CuAssertPtrNotNull(tc, ref);
    CuAssertPtrEquals(tc, NULL, ref->sp);
    CuAssertStrEquals(tc, "hodor", ref->_name);
    CuAssertPtrEquals(tc, NULL, spellref_get(ref));
    CuAssertStrEquals(tc, "hodor", spellref_name(ref));
    sp = create_spell("hodor");
    CuAssertPtrNotNull(tc, sp);
    CuAssertPtrEquals(tc, sp, spellref_get(ref));
    spellref_free(ref);
    test_teardown();
}

void fumble_foo(const struct castorder *co) {
    UNUSED_ARG(co);
}

void fumble_bar(const struct castorder *co) {
    UNUSED_ARG(co);
}

static void test_fumbles(CuTest *tc)
{
    test_setup();
    CuAssertTrue(tc, NULL==get_fumble("foo"));
    add_fumble("foone", fumble_foo);
    add_fumble("foozle", fumble_bar);
    CuAssertTrue(tc, fumble_foo==get_fumble("foone"));
    CuAssertTrue(tc, fumble_bar==get_fumble("foozle"));
    CuAssertTrue(tc, NULL==get_fumble("foo"));
    test_teardown();
}

CuSuite *get_spell_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_spellref);
    SUITE_ADD_TEST(suite, test_fumbles);
    SUITE_ADD_TEST(suite, test_create_a_spell);
    SUITE_ADD_TEST(suite, test_create_duplicate_spell);
    return suite;
}
