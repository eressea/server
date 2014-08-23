#include <platform.h>
#include <kernel/types.h>
#include <kernel/race.h>
#include <kernel/config.h>
#include <util/language.h>
#include "faction.h"
#include <CuTest.h>
#include <stdio.h>

static void test_addfaction(CuTest *tc) {
    faction *f = 0;
    const struct race *rc = rc_get_or_create("human");
    const struct locale *lang = get_or_create_locale("en");

    f = addfaction("test@eressea.de", "hurrdurr", rc, lang, 1234);
    CuAssertPtrNotNull(tc, f);
    CuAssertPtrNotNull(tc, f->name);
    CuAssertPtrEquals(tc, NULL, (void *)f->units);
    CuAssertPtrEquals(tc, NULL, (void *)f->next);
    CuAssertPtrEquals(tc, NULL, (void *)f->banner);
    CuAssertPtrEquals(tc, NULL, (void *)f->spellbook);
    CuAssertPtrEquals(tc, NULL, (void *)f->ursprung);
    CuAssertPtrEquals(tc, (void *)factions, (void *)f);
    CuAssertStrEquals(tc, "test@eressea.de", f->email);
    CuAssertStrEquals(tc, "hurrdurr", f->passw);
    CuAssertPtrEquals(tc, (void *)lang, (void *)f->locale);
    CuAssertIntEquals(tc, 1234, f->subscription);
    CuAssertIntEquals(tc, 0, f->flags);
    CuAssertIntEquals(tc, 0, f->age);
    CuAssertIntEquals(tc, 1, f->alive);
    CuAssertIntEquals(tc, M_GRAY, f->magiegebiet);
    CuAssertIntEquals(tc, turn, f->lastorders);
    CuAssertPtrEquals(tc, f, findfaction(f->no));
}

static void test_get_monsters(CuTest *tc) {
    faction *f;
    CuAssertPtrEquals(tc, NULL, get_monsters());
    f = get_or_create_monsters();
    CuAssertPtrEquals(tc, f, get_monsters());
    CuAssertIntEquals(tc, 666, f->no);
    CuAssertStrEquals(tc, "Monster", f->name);
    free_gamedata();
    CuAssertPtrEquals(tc, NULL, get_monsters());
    f = get_or_create_monsters();
    CuAssertPtrEquals(tc, f, get_monsters());
    CuAssertIntEquals(tc, 666, f->no);
}

CuSuite *get_faction_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_addfaction);
    SUITE_ADD_TEST(suite, test_get_monsters);
    return suite;
}
