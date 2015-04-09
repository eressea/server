#include <platform.h>

#include <kernel/ally.h>
#include <kernel/faction.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/config.h>
#include <util/language.h>

#include "monster.h"
#include <CuTest.h>
#include <tests.h>

#include <assert.h>
#include <stdio.h>

static void test_remove_empty_factions_allies(CuTest *tc) {
    faction *f1, *f2;
    region *r;

    test_cleanup();
    r = test_create_region(0, 0, 0);
    f1 = test_create_faction(0);
    test_create_unit(f1, r);
    f2 = test_create_faction(0);
    ally_add(&f1->allies, f2);
    remove_empty_factions();
    CuAssertPtrEquals(tc, 0, f1->allies);
    test_cleanup();
}

static void test_remove_empty_factions(CuTest *tc) {
    faction *f, *fm;
    int fno;

    test_cleanup();
    fm = get_or_create_monsters();
    assert(fm);
    f = test_create_faction(0);
    fno = f->no;
    remove_empty_factions();
    CuAssertPtrEquals(tc, 0, findfaction(fno));
    CuAssertPtrEquals(tc, fm, get_monsters());
    test_cleanup();
}

static void test_remove_dead_factions(CuTest *tc) {
    faction *f, *fm;
    region *r;
    int fno;

    test_cleanup();
    r = test_create_region(0, 0, 0);
    fm = get_or_create_monsters();
    f = test_create_faction(0);
    assert(fm && r && f);
    test_create_unit(f, r);
    test_create_unit(fm, r);
    remove_empty_factions();
    CuAssertPtrEquals(tc, f, findfaction(f->no));
    CuAssertPtrNotNull(tc, get_monsters());
    fm->alive = 0;
    f->alive = 0;
    fno = f->no;
    remove_empty_factions();
    CuAssertPtrEquals(tc, 0, findfaction(fno));
    CuAssertPtrEquals(tc, fm, get_monsters());
    test_cleanup();
}

static void test_addfaction(CuTest *tc) {
    faction *f = 0;
    const struct race *rc;
    const struct locale *lang;

    test_cleanup();
    rc = rc_get_or_create("human");
    lang = get_or_create_locale("en");
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
    test_cleanup();
}

static void test_get_monsters(CuTest *tc) {
    faction *f;

    free_gamedata();
    CuAssertPtrNotNull(tc, (f = get_monsters()));
    CuAssertPtrEquals(tc, f, get_monsters());
    CuAssertIntEquals(tc, 666, f->no);
    CuAssertStrEquals(tc, "Monster", f->name);
    test_cleanup();
}

CuSuite *get_faction_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_addfaction);
    SUITE_ADD_TEST(suite, test_remove_empty_factions);
    SUITE_ADD_TEST(suite, test_remove_empty_factions_allies);
    SUITE_ADD_TEST(suite, test_remove_dead_factions);
    SUITE_ADD_TEST(suite, test_get_monsters);
    return suite;
}
