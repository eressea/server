#include "automate.h"

#include "kernel/config.h"
#include "kernel/faction.h"
#include "kernel/order.h"
#include "kernel/race.h"
#include "kernel/region.h"
#include "kernel/skill.h"
#include "kernel/unit.h"

#include "util/keyword.h"
#include "util/message.h"

#include "tests.h"

#include <CuTest.h>

#include <stddef.h>

static void test_autostudy_init(CuTest *tc) {
    scholar scholars[4];
    unit *u1, *u2, *u3, *u4, *u5, *ulist;
    faction *f;
    region *r;
    message *msg;
    skill_t skill = NOSKILL;

    test_setup();
    mt_create_error(77);
    mt_create_error(771);

    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    u1->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    u2->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    set_level(u2, SK_ENTERTAINMENT, 2);
    u4 = test_create_unit(f, r);
    u4->thisorder = create_order(K_AUTOSTUDY, f->locale, "Dudelidu");
    u3 = test_create_unit(f, r);
    u3->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_PERCEPTION]);
    u5 = test_create_unit(test_create_faction(), r);
    u5->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_PERCEPTION]);
    scholars[2].u = NULL;

    ulist = r->units;
    CuAssertIntEquals(tc, 2, autostudy_init(scholars, 4, &ulist, &skill));
    CuAssertIntEquals(tc, SK_ENTERTAINMENT, skill);
    CuAssertPtrEquals(tc, u2, scholars[0].u);
    CuAssertIntEquals(tc, 2, scholars[0].level);
    CuAssertIntEquals(tc, 0, scholars[0].learn);
    CuAssertPtrEquals(tc, u1, scholars[1].u);
    CuAssertIntEquals(tc, 0, scholars[1].level);
    CuAssertIntEquals(tc, 0, scholars[1].learn);
    CuAssertPtrEquals(tc, NULL, scholars[2].u);
    CuAssertPtrEquals(tc, NULL, ulist);

    ulist = u3;
    CuAssertIntEquals(tc, 1, autostudy_init(scholars, 4, &ulist, &skill));
    CuAssertIntEquals(tc, SK_PERCEPTION, skill);
    CuAssertPtrEquals(tc, u3, scholars[0].u);
    CuAssertIntEquals(tc, 0, scholars[0].level);
    CuAssertIntEquals(tc, 0, scholars[0].learn);
    CuAssertPtrEquals(tc, NULL, ulist);

    CuAssertPtrNotNull(tc, msg = test_find_messagetype(f->msgs, "error77"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype_ex(f->msgs, "error77", msg));
    test_teardown();
}

static void test_autostudy_init_fallback(CuTest *tc) {
    scholar scholar;
    unit *u, *ulist;
    faction *f;
    region *r;
    race *rc;
    skill_t skill = NOSKILL;

    test_setup();
    rc = test_create_race("tunnelworm");
    rc->flags |= RCF_NOTEACH;
    r = test_create_plain(0, 0);
    f = test_create_faction();
    ulist = u = test_create_unit(f, r);
    u_setrace(u, rc);
    u->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    CuAssertIntEquals(tc, 0, autostudy_init(&scholar, 1, &ulist, &skill));
    CuAssertIntEquals(tc, K_STUDY, getkeyword(u->thisorder));
}

/**
 * Reproduce Bug 2520
 */
static void test_autostudy_run_twoteachers(CuTest *tc) {
    scholar scholars[4];
    int nscholars;
    unit *u1, *u2, *u3, *u4, *ulist;
    faction *f;
    region *r;
    skill_t skill;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    set_level(u1, SK_ENTERTAINMENT, 2);
    u1->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    u2 = test_create_unit(f, r);
    set_level(u2, SK_ENTERTAINMENT, 2);
    u2->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);

    u3 = test_create_unit(f, r);
    u3->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    set_number(u3, 8);
    u4 = test_create_unit(f, r);
    u4->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    set_number(u4, 12);

    ulist = r->units;
    CuAssertIntEquals(tc, 4, nscholars = autostudy_init(scholars, 4, &ulist, &skill));
    CuAssertPtrEquals(tc, NULL, ulist);
    autostudy_run(scholars, nscholars);
    CuAssertIntEquals(tc, SK_ENTERTAINMENT, skill);
    CuAssertIntEquals(tc, 0, scholars[0].learn);
    CuAssertIntEquals(tc, 0, scholars[1].learn);
    CuAssertIntEquals(tc, scholars[2].u->number * 2, scholars[2].learn);
    CuAssertIntEquals(tc, scholars[3].u->number * 2, scholars[3].learn);

    test_teardown();
}

/**
 * Reproduce Bug 2640
 */
static void test_autostudy_run_bigunit(CuTest *tc) {
    scholar scholars[4];
    int nscholars;
    unit *u1, *u2, *ulist;
    faction *f;
    region *r;
    skill_t skill;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    set_number(u1, 20);
    set_level(u1, SK_ENTERTAINMENT, 16);
    u1->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    u2 = test_create_unit(f, r);
    set_number(u2, 1000);
    set_level(u2, SK_ENTERTAINMENT, 10);
    u2->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);

    ulist = r->units;
    CuAssertIntEquals(tc, 2, nscholars = autostudy_init(scholars, 4, &ulist, &skill));
    CuAssertPtrEquals(tc, NULL, ulist);
    autostudy_run(scholars, nscholars);
    CuAssertIntEquals(tc, SK_ENTERTAINMENT, skill);
    CuAssertIntEquals(tc, 0, scholars[0].learn);
    CuAssertIntEquals(tc, 1200, scholars[1].learn);

    test_teardown();
}

static void test_autostudy_run_few_teachers(CuTest *tc) {
    scholar scholars[4];
    int nscholars;
    unit *u1, *u2, *u3, *ulist;
    faction *f;
    region *r;
    skill_t skill;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    set_number(u1, 20);
    set_level(u1, SK_ENTERTAINMENT, 16);
    u1->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    u2 = test_create_unit(f, r);
    set_number(u2, 500);
    set_level(u2, SK_ENTERTAINMENT, 10);
    u2->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    u3 = test_create_unit(f, r);
    set_number(u3, 100);
    set_level(u3, SK_ENTERTAINMENT, 9);
    u3->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);

    ulist = r->units;
    CuAssertIntEquals(tc, 3, nscholars = autostudy_init(scholars, 4, &ulist, &skill));
    CuAssertPtrEquals(tc, NULL, ulist);
    autostudy_run(scholars, nscholars);
    CuAssertIntEquals(tc, SK_ENTERTAINMENT, skill);
    CuAssertIntEquals(tc, 0, scholars[0].learn);
    CuAssertIntEquals(tc, 700, scholars[1].learn);
    CuAssertIntEquals(tc, 100, scholars[2].learn);

    test_teardown();
}

static void test_autostudy_run_few_teachers_reverse(CuTest *tc) {
    scholar scholars[4];
    int nscholars;
    unit *u1, *u2, *u3, *ulist;
    faction *f;
    region *r;
    skill_t skill;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    set_number(u1, 20);
    set_level(u1, SK_ENTERTAINMENT, 16);
    u1->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    u2 = test_create_unit(f, r);
    set_number(u2, 100);
    set_level(u2, SK_ENTERTAINMENT, 10);
    u2->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    u3 = test_create_unit(f, r);
    set_number(u3, 500);
    set_level(u3, SK_ENTERTAINMENT, 9);
    u3->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);

    ulist = r->units;
    CuAssertIntEquals(tc, 3, nscholars = autostudy_init(scholars, 4, &ulist, &skill));
    CuAssertPtrEquals(tc, NULL, ulist);
    autostudy_run(scholars, nscholars);
    CuAssertIntEquals(tc, SK_ENTERTAINMENT, skill);
    CuAssertIntEquals(tc, 0, scholars[0].learn);
    CuAssertIntEquals(tc, 800, scholars[1].learn + scholars[2].learn);

    test_teardown();
}

static void test_autostudy_run(CuTest *tc) {
    scholar scholars[4];
    int nscholars;
    unit *u1, *u2, *u3, *ulist;
    faction *f;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    u1->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    set_number(u1, 2);
    set_level(u1, SK_ENTERTAINMENT, 2);
    u2 = test_create_unit(f, r);
    u2->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    set_number(u2, 10);
    u3 = test_create_unit(f, r);
    u3->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_PERCEPTION]);
    set_number(u3, 15);

    scholars[2].u = NULL;
    ulist = r->units;
    CuAssertIntEquals(tc, 2, nscholars = autostudy_init(scholars, 4, &ulist, NULL));
    CuAssertIntEquals(tc, UFL_MARK, u1->flags & UFL_MARK);
    CuAssertIntEquals(tc, UFL_MARK, u2->flags & UFL_MARK);
    CuAssertIntEquals(tc, 0, u3->flags & UFL_MARK);
    CuAssertPtrEquals(tc, NULL, ulist);
    autostudy_run(scholars, nscholars);
    CuAssertIntEquals(tc, 1, scholars[0].learn);
    CuAssertIntEquals(tc, 20, scholars[1].learn);
    CuAssertPtrEquals(tc, NULL, scholars[2].u);

    scholars[1].u = NULL;
    ulist = u3;
    CuAssertIntEquals(tc, 1, nscholars = autostudy_init(scholars, 4, &ulist, NULL));
    CuAssertPtrEquals(tc, NULL, ulist);
    autostudy_run(scholars, nscholars);
    CuAssertIntEquals(tc, 15, scholars[0].learn);
    CuAssertPtrEquals(tc, NULL, scholars[1].u);

    test_teardown();
}

static void test_autostudy_run_noteachers(CuTest *tc) {
    scholar scholars[4];
    int nscholars;
    unit *u1, *u2, *ulist;
    faction *f;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    u1->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    set_number(u1, 5);
    set_level(u1, SK_ENTERTAINMENT, 2);

    u2 = test_create_unit(f, r);
    u2->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    set_number(u2, 7);
    set_level(u2, SK_ENTERTAINMENT, 2);

    scholars[2].u = NULL;
    ulist = r->units;
    CuAssertIntEquals(tc, 2, nscholars = autostudy_init(scholars, 4, &ulist, NULL));
    CuAssertPtrEquals(tc, NULL, ulist);
    autostudy_run(scholars, nscholars);
    /* stupid qsort is unstable: */
    CuAssertIntEquals(tc, 12, scholars[0].learn + scholars[1].learn);
    CuAssertIntEquals(tc, 35, scholars[0].learn * scholars[1].learn);
    CuAssertPtrEquals(tc, NULL, scholars[2].u);
    test_teardown();
}

static void test_autostudy_run_can_teach(CuTest *tc)
{
    scholar scholars[4];
    int nscholars;
    unit *u1, *u2, *ulist;
    faction *f;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    u1->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    set_number(u1, 1);
    set_level(u1, SK_ENTERTAINMENT, 2);

    u2 = test_create_unit(f, r);
    u2->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    set_number(u2, 10);

    scholars[2].u = NULL;
    ulist = r->units;
    CuAssertIntEquals(tc, 2, nscholars = autostudy_init(scholars, 4, &ulist, NULL));
    CuAssertPtrEquals(tc, NULL, ulist);
    autostudy_run(scholars, nscholars);
    CuAssertIntEquals(tc, 0, scholars[0].learn);
    CuAssertPtrEquals(tc, u1, scholars[0].u);
    CuAssertIntEquals(tc, 20, scholars[1].learn);
    CuAssertPtrEquals(tc, u2, scholars[1].u);
    CuAssertPtrEquals(tc, NULL, scholars[2].u);
    test_teardown();
}

static void test_autostudy_run_cannot_teach(CuTest *tc)
{
    scholar scholars[4];
    int nscholars;
    unit *u1, *u2, *ulist;
    faction *f;
    region *r;
    race* rc;

    test_setup();
    rc = test_create_race("tunnelworm");
    rc->flags |= RCF_NOTEACH;
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    u_setrace(u1, rc);
    u1->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    set_number(u1, 1);
    set_level(u1, SK_ENTERTAINMENT, 2);

    u2 = test_create_unit(f, r);
    u2->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    set_number(u2, 10);

    scholars[1].u = NULL;
    ulist = r->units;
    CuAssertIntEquals(tc, 1, nscholars = autostudy_init(scholars, 4, &ulist, NULL));
    CuAssertPtrNotNull(tc, test_find_messagetype(u1->faction->msgs, "error274"));
    CuAssertPtrEquals(tc, NULL, ulist);
    autostudy_run(scholars, nscholars);
    CuAssertIntEquals(tc, 10, scholars[0].learn);
    CuAssertPtrEquals(tc, u2, scholars[0].u);
    CuAssertPtrEquals(tc, NULL, scholars[1].u);
    test_teardown();
}

/**
 * If a teacher unit doesn't have enough students, the remaining members study.
 */
static void test_autostudy_run_teachers_learn(CuTest *tc) {
    scholar scholars[4];
    int nscholars;
    unit *u1, *u2, *ulist;
    faction *f;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    u1->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    set_number(u1, 3);
    set_level(u1, SK_ENTERTAINMENT, 2);
    u2 = test_create_unit(f, r);
    u2->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    set_number(u2, 10);
    ulist = r->units;
    CuAssertIntEquals(tc, 2, nscholars = autostudy_init(scholars, 4, &ulist, NULL));
    CuAssertPtrEquals(tc, NULL, ulist);
    autostudy_run(scholars, nscholars);
    CuAssertIntEquals(tc, 2, scholars[0].learn);
    CuAssertIntEquals(tc, 20, scholars[1].learn);
    test_teardown();
}

/**
 * If a teacher unit doesn't have enough students, the remaining members study.
 */
static void test_autostudy_run_teachers_learn_roundup(CuTest *tc) {
    scholar scholars[4];
    int nscholars;
    unit *u1, *u2, *ulist;
    faction *f;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    u1->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    set_number(u1, 3);
    set_level(u1, SK_ENTERTAINMENT, 2);
    u2 = test_create_unit(f, r);
    u2->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    set_number(u2, 9);
    ulist = r->units;
    CuAssertIntEquals(tc, 2, nscholars = autostudy_init(scholars, 4, &ulist, NULL));
    CuAssertPtrEquals(tc, NULL, ulist);
    autostudy_run(scholars, nscholars);
    CuAssertIntEquals(tc, 2, scholars[0].learn);
    CuAssertIntEquals(tc, 18, scholars[1].learn);
    test_teardown();
}

/**
 * If a teacher unit doesn't have enough students, the remaining members study.
 */
static void test_autostudy_run_teachers_learn_roundup2(CuTest *tc) {
    scholar scholars[4];
    int nscholars;
    unit *u1, *u2, *ulist;
    faction *f;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    u1->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    set_number(u1, 3);
    set_level(u1, SK_ENTERTAINMENT, 2);
    u2 = test_create_unit(f, r);
    u2->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    set_number(u2, 11);
    ulist = r->units;
    CuAssertIntEquals(tc, 2, nscholars = autostudy_init(scholars, 4, &ulist, NULL));
    CuAssertPtrEquals(tc, NULL, ulist);
    autostudy_run(scholars, nscholars);
    CuAssertIntEquals(tc, 1, scholars[0].learn);
    CuAssertIntEquals(tc, 22, scholars[1].learn);
    test_teardown();
}

/**
 * Reproduce Bug 2514
 */
static void test_autostudy_run_skilldiff(CuTest *tc) {
    scholar scholars[4];
    int nscholars;
    unit *u1, *u2, *u3, *ulist;
    faction *f;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    u1->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_PERCEPTION]);
    set_number(u1, 1);
    set_level(u1, SK_PERCEPTION, 2);
    u2 = test_create_unit(f, r);
    u2->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_PERCEPTION]);
    set_number(u2, 10);
    set_level(u2, SK_PERCEPTION, 1);
    u3 = test_create_unit(f, r);
    u3->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_PERCEPTION]);
    set_number(u3, 10);
    scholars[3].u = NULL;
    ulist = r->units;
    CuAssertIntEquals(tc, 3, nscholars = autostudy_init(scholars, 4, &ulist, NULL));
    CuAssertPtrEquals(tc, NULL, ulist);
    autostudy_run(scholars, nscholars);
    CuAssertIntEquals(tc, 0, scholars[0].learn);
    CuAssertIntEquals(tc, 20, scholars[2].learn);
    CuAssertIntEquals(tc, 10, scholars[1].learn);
    test_teardown();
}

static void test_autostudy_batches(CuTest *tc) {
    scholar scholars[2];
    int nscholars;
    unit *u1, *u2, *u3, *ulist;
    faction *f;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    u1->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_PERCEPTION]);
    set_number(u1, 1);
    set_level(u1, SK_PERCEPTION, 2);
    u2 = test_create_unit(f, r);
    u2->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_PERCEPTION]);
    set_number(u2, 10);
    u3 = test_create_unit(f, r);
    u3->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_PERCEPTION]);
    set_number(u3, 10);
    scholars[1].u = NULL;
    ulist = r->units;
    config_set("automate.batchsize", "2");
    CuAssertIntEquals(tc, 2, nscholars = autostudy_init(scholars, 2, &ulist, NULL));
    CuAssertPtrEquals(tc, u3, ulist);
    autostudy_run(scholars, nscholars);
    CuAssertIntEquals(tc, 0, scholars[0].learn);
    CuAssertIntEquals(tc, 20, scholars[1].learn);
    CuAssertIntEquals(tc, 1, nscholars = autostudy_init(scholars, 2, &ulist, NULL));
    autostudy_run(scholars, nscholars);
    CuAssertIntEquals(tc, 10, scholars[0].learn);
    test_teardown();
}

static void test_do_autostudy(CuTest *tc) {
    unit *u1, *u2, *u3, *u4;
    faction *f;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    u1->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_PERCEPTION]);
    set_number(u1, 1);
    set_level(u1, SK_PERCEPTION, 2);
    u2 = test_create_unit(f, r);
    u2->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_PERCEPTION]);
    set_number(u2, 10);
    u3 = test_create_unit(f, r);
    u3->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    u4 = test_create_unit(test_create_faction(), r);
    u4->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    do_autostudy(r);
    CuAssertIntEquals(tc, 2, get_level(u1, SK_PERCEPTION));
    /* impossible to say if u2 is T1 or T2 now */
    CuAssertIntEquals(tc, 1, get_level(u3, SK_ENTERTAINMENT));
    CuAssertIntEquals(tc, 1, get_level(u4, SK_ENTERTAINMENT));
    CuAssertIntEquals(tc, 0, u1->flags & UFL_MARK);
    CuAssertIntEquals(tc, 0, u2->flags & UFL_MARK);
    CuAssertIntEquals(tc, 0, u3->flags & UFL_MARK);
    CuAssertIntEquals(tc, 0, u4->flags & UFL_MARK);
    test_teardown();
}

CuSuite *get_automate_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_autostudy_init);
    SUITE_ADD_TEST(suite, test_autostudy_init_fallback);
    SUITE_ADD_TEST(suite, test_autostudy_run);
    SUITE_ADD_TEST(suite, test_do_autostudy);
    SUITE_ADD_TEST(suite, test_autostudy_batches);
    SUITE_ADD_TEST(suite, test_autostudy_run_noteachers);
    SUITE_ADD_TEST(suite, test_autostudy_run_can_teach);
    SUITE_ADD_TEST(suite, test_autostudy_run_cannot_teach);
    SUITE_ADD_TEST(suite, test_autostudy_run_teachers_learn);
    SUITE_ADD_TEST(suite, test_autostudy_run_teachers_learn_roundup);
    SUITE_ADD_TEST(suite, test_autostudy_run_teachers_learn_roundup2);
    SUITE_ADD_TEST(suite, test_autostudy_run_twoteachers);
    SUITE_ADD_TEST(suite, test_autostudy_run_bigunit);
    SUITE_ADD_TEST(suite, test_autostudy_run_few_teachers);
    SUITE_ADD_TEST(suite, test_autostudy_run_few_teachers_reverse);
    SUITE_ADD_TEST(suite, test_autostudy_run_skilldiff);
    return suite;
}
