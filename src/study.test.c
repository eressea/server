#include <platform.h>

#include "study.h"

#include <kernel/config.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <util/rand.h>
#include <util/message.h>
#include <util/language.h>
#include <util/base36.h>
#include <tests.h>

#include <assert.h>

#include <CuTest.h>

typedef struct {
    unit *u;
    unit *teachers[2];
} study_fixture;

static void setup_locale(struct locale *lang) {
    int i;
    for (i = 0; i < MAXSKILLS; ++i) {
        if (!locale_getstring(lang, mkname("skill", skillnames[i])))
            locale_setstring(lang, mkname("skill", skillnames[i]), skillnames[i]);
    }
    locale_setstring(lang, parameters[P_ANY], "ALLE");
    init_parameters(lang);
    init_skills(lang);
}

static void setup_study(study_fixture *fix, skill_t sk) {
    struct region * r;
    struct faction *f;
    struct locale *lang;

    assert(fix);
    test_cleanup();
    config_set("study.random_progress", "0");
    test_create_world();
    r = findregion(0, 0);
    f = test_create_faction(0);
    lang = get_or_create_locale(locale_name(f->locale));
    setup_locale(lang);
    fix->u = test_create_unit(f, r);
    assert(fix->u);
    fix->u->thisorder = create_order(K_STUDY, f->locale, "%s", skillnames[sk]);

    fix->teachers[0] = test_create_unit(f, r);
    assert(fix->teachers[0]);
    fix->teachers[0]->thisorder = create_order(K_TEACH, f->locale, "%s", itoa36(fix->u->no));

    fix->teachers[1] = test_create_unit(f, r);
    assert(fix->teachers[1]);
    fix->teachers[1]->thisorder = create_order(K_TEACH, f->locale, "%s", itoa36(fix->u->no));
    test_clear_messages(f);
}

static void test_study_no_teacher(CuTest *tc) {
    study_fixture fix;
    skill *sv;

    setup_study(&fix, SK_CROSSBOW);
    study_cmd(fix.u, fix.u->thisorder);
    CuAssertPtrNotNull(tc, sv = unit_skill(fix.u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 1, sv->level);
    CuAssertIntEquals(tc, 2, sv->weeks);
    CuAssertPtrEquals(tc, 0, test_get_last_message(fix.u->faction->msgs));
    test_cleanup();
}

static void test_study_with_teacher(CuTest *tc) {
    study_fixture fix;
    skill *sv;

    setup_study(&fix, SK_CROSSBOW);
    set_level(fix.teachers[0], SK_CROSSBOW, TEACHDIFFERENCE);
    teach_cmd(fix.teachers[0], fix.teachers[0]->thisorder);
    CuAssertPtrEquals(tc, 0, test_get_last_message(fix.u->faction->msgs));
    study_cmd(fix.u, fix.u->thisorder);
    CuAssertPtrNotNull(tc, sv = unit_skill(fix.u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 1, sv->level);
    CuAssertIntEquals(tc, 1, sv->weeks);
    test_cleanup();
}

static void test_study_with_bad_teacher(CuTest *tc) {
    study_fixture fix;
    skill *sv;

    setup_study(&fix, SK_CROSSBOW);
    teach_cmd(fix.teachers[0], fix.teachers[0]->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(fix.u->faction->msgs, "teach_asgood"));
    study_cmd(fix.u, fix.u->thisorder);
    CuAssertPtrNotNull(tc, sv = unit_skill(fix.u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 1, sv->level);
    CuAssertIntEquals(tc, 2, sv->weeks);
    test_cleanup();
}

static void test_study_bug_2194(CuTest *tc) {
    unit *u, *u1, *u2;
    struct locale * loc;
    building * b;

    test_cleanup();
    random_source_inject_constant(0.0);
    init_resources();
    loc = get_or_create_locale("de");
    setup_locale(loc);
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    scale_number(u, 2);
    set_level(u, SK_CROSSBOW, TEACHDIFFERENCE);
    u->faction->locale = loc;
    u1 = test_create_unit(u->faction, u->region);
    scale_number(u1, 17);
    u1->thisorder = create_order(K_STUDY, loc, skillnames[SK_CROSSBOW]);
    u2 = test_create_unit(u->faction, u->region);
    scale_number(u2, 3);
    u2->thisorder = create_order(K_STUDY, loc, skillnames[SK_MAGIC]);
    u->thisorder = create_order(K_TEACH, loc, "%s %s", itoa36(u1->no), itoa36(u2->no));
    b = test_create_building(u->region, test_create_buildingtype("academy"));
    b->size = 22;
    u_set_building(u, b);
    u_set_building(u1, b);
    u_set_building(u2, b);
    i_change(&u1->items, get_resourcetype(R_SILVER)->itype, 50);
    i_change(&u2->items, get_resourcetype(R_SILVER)->itype, 50);
    b->flags = BLD_WORKING;
    teach_cmd(u, u->thisorder);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "teach_asgood"));
    test_cleanup();
}

static CuTest *g_tc;

static bool cb_learn_one(unit *u, skill_t sk, double chance) {
    CuAssertIntEquals(g_tc, SK_ALCHEMY, sk);
    CuAssertDblEquals(g_tc, 0.5 / u->number, chance, 0.01);
    return false;
}

static bool cb_learn_two(unit *u, skill_t sk, double chance) {
    CuAssertIntEquals(g_tc, SK_ALCHEMY, sk);
    CuAssertDblEquals(g_tc, 2 * 0.5 / u->number, chance, 0.01);
    return false;
}

static void test_produceexp(CuTest *tc) {
    unit *u;

    g_tc = tc;
    test_cleanup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    config_set("study.from_use", "0.5");
    produceexp_ex(u, SK_ALCHEMY, 1, cb_learn_one);
    produceexp_ex(u, SK_ALCHEMY, 2, cb_learn_two);
    test_cleanup();
}

#define MAXLOG 4
typedef struct log_entry {
    unit *u;
    skill_t sk;
    int days;
} log_entry;

static log_entry log_learners[MAXLOG];
static int log_size;

static void log_learn(unit *u, skill_t sk, int days) {
    if (log_size < MAXLOG) {
        log_entry * entry = &log_learners[log_size++];
        entry->u = u;
        entry->sk = sk;
        entry->days = days;
    }
}

void learn_inject(void) {
    log_size = 0;
    inject_learn(log_learn);
}

void learn_reset(void) {
    inject_learn(0);
}

static void test_academy_building(CuTest *tc) {
    unit *u, *u1, *u2;
    struct locale * loc;
    building * b;
    message * msg;

    test_cleanup();
    mt_register(mt_new_va("teach_asgood", "unit:unit", "region:region", "command:order", "student:unit", 0));

    random_source_inject_constant(0.0);
    init_resources();
    loc = get_or_create_locale("de");
    setup_locale(loc);
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    scale_number(u, 2);
    set_level(u, SK_CROSSBOW, TEACHDIFFERENCE);
    u->faction->locale = loc;
    u1 = test_create_unit(u->faction, u->region);
    scale_number(u1, 15);
    u1->thisorder = create_order(K_STUDY, loc, skillnames[SK_CROSSBOW]);
    u2 = test_create_unit(u->faction, u->region);
    scale_number(u2, 5);
    u2->thisorder = create_order(K_STUDY, loc, skillnames[SK_CROSSBOW]);
    set_level(u2, SK_CROSSBOW, 1);
    u->thisorder = create_order(K_TEACH, loc, "%s %s", itoa36(u1->no), itoa36(u2->no));
    b = test_create_building(u->region, test_create_buildingtype("academy"));
    b->size = 22;
    u_set_building(u, b);
    u_set_building(u1, b);
    u_set_building(u2, b);
    i_change(&u1->items, get_resourcetype(R_SILVER)->itype, 50);
    i_change(&u2->items, get_resourcetype(R_SILVER)->itype, 50);
    b->flags = BLD_WORKING;
    learn_inject();
    teach_cmd(u, u->thisorder);
    learn_reset();
    CuAssertPtrNotNull(tc, msg = test_find_messagetype(u->faction->msgs, "teach_asgood"));
    // FIXME: new injection function
#if 0
    CuAssertPtrEquals(tc, u, (unit *)(msg)->parameters[0].v);
    CuAssertPtrEquals(tc, u2, (unit *)(msg)->parameters[3].v);
    CuAssertPtrEquals(tc, u, log_learners[0].u);
    CuAssertIntEquals(tc, SK_CROSSBOW, log_learners[0].sk);
    CuAssertIntEquals(tc, 10, log_learners[0].days);
#endif
    test_cleanup();
}

void test_learn_skill_single(CuTest *tc) {
    unit *u;
    skill *sv;
    test_cleanup();
    config_set("study.random_progress", "0");
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    learn_skill(u, SK_ALCHEMY, STUDYDAYS);
    CuAssertPtrNotNull(tc, sv = u->skills);
    CuAssertIntEquals(tc, SK_ALCHEMY, sv->id);
    CuAssertIntEquals(tc, 1, sv->level);
    CuAssertIntEquals(tc, 2, sv->weeks);
    learn_skill(u, SK_ALCHEMY, STUDYDAYS);
    CuAssertIntEquals(tc, 1, sv->weeks);
    learn_skill(u, SK_ALCHEMY, STUDYDAYS * 2);
    CuAssertIntEquals(tc, 2, sv->level);
    CuAssertIntEquals(tc, 2, sv->weeks);
    test_cleanup();
}

void test_learn_skill_multi(CuTest *tc) {
    unit *u;
    skill *sv;
    test_cleanup();
    config_set("study.random_progress", "0");
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    scale_number(u, 10);
    learn_skill(u, SK_ALCHEMY, STUDYDAYS * u->number);
    CuAssertPtrNotNull(tc, sv = u->skills);
    CuAssertIntEquals(tc, SK_ALCHEMY, sv->id);
    CuAssertIntEquals(tc, 1, sv->level);
    CuAssertIntEquals(tc, 2, sv->weeks);
    learn_skill(u, SK_ALCHEMY, STUDYDAYS * u->number);
    CuAssertIntEquals(tc, 1, sv->weeks);
    learn_skill(u, SK_ALCHEMY, STUDYDAYS  * u->number * 2);
    CuAssertIntEquals(tc, 2, sv->level);
    CuAssertIntEquals(tc, 2, sv->weeks);
    test_cleanup();
}

static void test_demon_skillchanges(CuTest *tc) {
    unit * u;
    race * rc;
    test_cleanup();
    rc = test_create_race("demon");
    CuAssertPtrEquals(tc, rc, get_race(RC_DAEMON));
    u = test_create_unit(test_create_faction(rc), 0);
    CuAssertPtrNotNull(tc, u);
    set_level(u, SK_CROSSBOW, 1);
    demon_skillchange(u);
    // TODO: sensing here
    test_cleanup();
}

static void test_study_cmd(CuTest *tc) {
    unit *u;
    test_cleanup();
    init_resources();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    u->thisorder = create_order(K_STUDY, u->faction->locale, "ALCHEMY");
    learn_inject();
    study_cmd(u, u->thisorder);
    learn_reset();
    CuAssertPtrEquals(tc, u, log_learners[0].u);
    CuAssertIntEquals(tc, SK_ALCHEMY, log_learners[0].sk);
    test_cleanup();
}

CuSuite *get_study_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_study_cmd);
    SUITE_ADD_TEST(suite, test_learn_skill_single);
    SUITE_ADD_TEST(suite, test_learn_skill_multi);
    SUITE_ADD_TEST(suite, test_study_no_teacher);
    SUITE_ADD_TEST(suite, test_study_with_teacher);
    SUITE_ADD_TEST(suite, test_study_with_bad_teacher);
    SUITE_ADD_TEST(suite, test_produceexp);
    SUITE_ADD_TEST(suite, test_academy_building);
    SUITE_ADD_TEST(suite, test_demon_skillchanges);
    DISABLE_TEST(suite, test_study_bug_2194);
    return suite;
}
