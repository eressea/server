#include <platform.h>

#include <kernel/spell.h>

#include <util/log.h>
#include <util/lists.h>

#include <quicklist.h>
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

static void log_list(void *udata, int flags, const char *module, const char *format, va_list args) {
    strlist **slp = (strlist **)udata;
    addstrlist(slp, format);
}

static void test_create_duplicate_spell(CuTest * tc)
{
    spell *sp;
    struct log_t *log;
    strlist *sl = 0;

    test_setup();
    test_log_stderr(0);
    log = log_create(LOG_CPERROR, &sl, log_list);

    CuAssertPtrEquals(tc, 0, find_spell("testspell"));

    sp = create_spell("testspell", 0);
    CuAssertPtrEquals(tc, 0, create_spell("testspell", 0));
    CuAssertPtrNotNull(tc, sl);
    CuAssertStrEquals(tc, "create_spell: duplicate name '%s'", sl->s);
    CuAssertPtrEquals(tc, 0, sl->next);
    freestrlist(sl);
    log_destroy(log);
    CuAssertPtrEquals(tc, sp, find_spell("testspell"));
    test_cleanup();
}

static void test_create_spell_with_id(CuTest * tc)
{
    spell *sp;
    struct log_t *log;
    strlist *sl = 0;

    test_setup();
    test_log_stderr(0);
    log = log_create(LOG_CPERROR, &sl, log_list);

    CuAssertPtrEquals(tc, 0, find_spellbyid(42));
    sp = create_spell("testspell", 42);
    CuAssertPtrEquals(tc, sp, find_spellbyid(42));
    CuAssertPtrEquals(tc, 0, create_spell("testspell", 47));
    CuAssertPtrNotNull(tc, sl);
    CuAssertStrEquals(tc, "create_spell: duplicate name '%s'", sl->s);
    CuAssertPtrEquals(tc, 0, sl->next);
    freestrlist(sl);
    log_destroy(log);
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
