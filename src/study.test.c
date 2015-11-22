#include <platform.h>

#include "study.h"

#include <kernel/config.h>
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/order.h>
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

static void setup_study(study_fixture *fix, skill_t sk) {
    struct region * r;
    struct faction *f;
    struct locale *lang;

    assert(fix);
    test_cleanup();
    config_set("study.random_progress", "0");
    test_create_world();
    r = test_create_region(0, 0, 0);
    f = test_create_faction(0);
    lang = get_or_create_locale(locale_name(f->locale));
    locale_setstring(lang, mkname("skill", skillnames[sk]), skillnames[sk]);
    init_skills(lang);
    fix->u = test_create_unit(f, r);
    assert(fix->u);
    fix->u->thisorder = create_order(K_STUDY, f->locale, "%s", skillnames[sk]);

    fix->teachers[0] = test_create_unit(f, r);
    assert(fix->teachers[0]);
    fix->teachers[0]->thisorder = create_order(K_TEACH, f->locale, "%s", itoa36(fix->u->no));

    fix->teachers[1] = test_create_unit(f, r);
    assert(fix->teachers[1]);
    fix->teachers[1]->thisorder = create_order(K_TEACH, f->locale, "%s", itoa36(fix->u->no));
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
    message *msg;

    setup_study(&fix, SK_CROSSBOW);
    teach_cmd(fix.teachers[0], fix.teachers[0]->thisorder);
    CuAssertPtrNotNull(tc, msg = test_get_last_message(fix.u->faction->msgs));
    CuAssertStrEquals(tc, "teach_asgood", test_get_messagetype(msg));
    study_cmd(fix.u, fix.u->thisorder);
    CuAssertPtrNotNull(tc, sv = unit_skill(fix.u, SK_CROSSBOW));
    CuAssertIntEquals(tc, 1, sv->level);
    CuAssertIntEquals(tc, 2, sv->weeks);
    test_cleanup();
}

CuSuite *get_study_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_study_no_teacher);
    SUITE_ADD_TEST(suite, test_study_with_teacher);
    SUITE_ADD_TEST(suite, test_study_with_bad_teacher);
    return suite;
}
