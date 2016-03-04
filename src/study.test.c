#include <platform.h>

#include "study.h"

#include <kernel/config.h>
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/order.h>
#include <kernel/region.h>
#include <kernel/building.h>
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

CuSuite *get_study_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_study_no_teacher);
    SUITE_ADD_TEST(suite, test_study_with_teacher);
    SUITE_ADD_TEST(suite, test_study_with_bad_teacher);
    SUITE_ADD_TEST(suite, test_study_bug_2194);
    return suite;
}
