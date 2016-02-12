#include <platform.h>

#include <kernel/ally.h>
#include <kernel/alliance.h>
#include <kernel/faction.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/plane.h>
#include <kernel/config.h>
#include <util/language.h>
#include <util/password.h>

#include "monster.h"
#include <CuTest.h>
#include <tests.h>

#include <assert.h>
#include <stdio.h>

static void test_destroyfaction_allies(CuTest *tc) {
    faction *f1, *f2;
    region *r;
    ally *al;

    test_cleanup();
    r = test_create_region(0, 0, 0);
    f1 = test_create_faction(0);
    test_create_unit(f1, r);
    f2 = test_create_faction(0);
    al = ally_add(&f1->allies, f2);
    al->status = HELP_FIGHT;
    CuAssertIntEquals(tc, HELP_FIGHT, alliedgroup(0, f1, f2, f1->allies, HELP_ALL));
    CuAssertPtrEquals(tc, f2, f1->next);
    destroyfaction(&f1->next);
    CuAssertIntEquals(tc, false, faction_alive(f2));
    CuAssertIntEquals(tc, 0, alliedgroup(0, f1, f2, f1->allies, HELP_ALL));
    test_cleanup();
}

static void test_remove_empty_factions_alliance(CuTest *tc) {
    faction *f;
    struct alliance *al;

    test_cleanup();
    f = test_create_faction(0);
    al = makealliance(0, "Hodor");
    setalliance(f, al);
    CuAssertPtrEquals(tc, f, alliance_get_leader(al));
    remove_empty_factions();
    CuAssertPtrEquals(tc, 0, al->_leader);
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
    fm->units = 0;
    f->_alive = false;
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
    CuAssertTrue(tc, checkpasswd(f, "hurrdurr"));
    CuAssertPtrEquals(tc, (void *)lang, (void *)f->locale);
    CuAssertIntEquals(tc, 1234, f->subscription);
    CuAssertIntEquals(tc, 0, f->flags);
    CuAssertIntEquals(tc, 0, f->age);
    CuAssertTrue(tc, faction_alive(f));
    CuAssertIntEquals(tc, M_GRAY, f->magiegebiet);
    CuAssertIntEquals(tc, turn, f->lastorders);
    CuAssertPtrEquals(tc, f, findfaction(f->no));
    test_cleanup();
}

static void test_check_passwd(CuTest *tc) {
    faction *f;
    
    f = test_create_faction(0);
    faction_setpassword(f, password_hash("password", 0, PASSWORD_DEFAULT));
    CuAssertTrue(tc, checkpasswd(f, "password"));
    CuAssertTrue(tc, !checkpasswd(f, "assword"));
    CuAssertTrue(tc, !checkpasswd(f, "PASSWORD"));
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

static void test_set_origin(CuTest *tc) {
    faction *f;
    int x = 0, y = 0;
    plane *pl;

    test_cleanup();
    test_create_world();
    pl = create_new_plane(0, "", 0, 19, 0, 19, 0);
    f = test_create_faction(0);
    CuAssertPtrEquals(tc, 0, f->ursprung);
    faction_setorigin(f, 0, 1, 1);
    CuAssertIntEquals(tc, 0, f->ursprung->id);
    CuAssertIntEquals(tc, 1, f->ursprung->x);
    CuAssertIntEquals(tc, 1, f->ursprung->y);
    faction_getorigin(f, 0, &x, &y);
    CuAssertIntEquals(tc, 1, x);
    CuAssertIntEquals(tc, 1, y);
    adjust_coordinates(f, &x, &y, pl);
    CuAssertIntEquals(tc, -9, x);
    CuAssertIntEquals(tc, -9, y);
    adjust_coordinates(f, &x, &y, 0);
    CuAssertIntEquals(tc, -10, x);
    CuAssertIntEquals(tc, -10, y);
    test_cleanup();
}

static void test_set_origin_bug(CuTest *tc) {
    faction *f;
    plane *pl;
    int x = 17, y = 10;

    test_cleanup();
    test_create_world();
    pl = create_new_plane(0, "", 0, 19, 0, 19, 0);
    f = test_create_faction(0);
    faction_setorigin(f, 0, -10, 3);
    faction_setorigin(f, 0, -13, -4);
    adjust_coordinates(f, &x, &y, pl);
    CuAssertIntEquals(tc, 0, f->ursprung->id);
    CuAssertIntEquals(tc, -9, x);
    CuAssertIntEquals(tc, 2, y);
    test_cleanup();
}

CuSuite *get_faction_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_addfaction);
    SUITE_ADD_TEST(suite, test_remove_empty_factions);
    SUITE_ADD_TEST(suite, test_destroyfaction_allies);
    SUITE_ADD_TEST(suite, test_remove_empty_factions_alliance);
    SUITE_ADD_TEST(suite, test_remove_dead_factions);
    SUITE_ADD_TEST(suite, test_get_monsters);
    SUITE_ADD_TEST(suite, test_set_origin);
    SUITE_ADD_TEST(suite, test_set_origin_bug);
    SUITE_ADD_TEST(suite, test_check_passwd);
    return suite;
}
