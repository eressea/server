#include <platform.h>

#include "names.h"

#include <kernel/race.h>
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <util/language.h>
#include <util/functions.h>

#include <CuTest.h>
#include "tests.h"

static void test_names(CuTest * tc)
{
    unit *u;
    race *rc;
    test_cleanup();
    register_names();
    CuAssertPtrNotNull(tc, get_function("name_undead"));
    CuAssertPtrNotNull(tc, get_function("name_skeleton"));
    CuAssertPtrNotNull(tc, get_function("name_zombie"));
    CuAssertPtrNotNull(tc, get_function("name_ghoul"));
    CuAssertPtrNotNull(tc, get_function("name_dragon"));
    CuAssertPtrNotNull(tc, get_function("name_youngdragon"));
    CuAssertPtrNotNull(tc, get_function("name_wyrm"));
    CuAssertPtrNotNull(tc, get_function("name_dracoid"));
    default_locale = test_create_locale();
    rc = test_create_race("undead");
    u = test_create_unit(test_create_faction(rc), test_create_region(0, 0, 0));
    locale_setstring(default_locale, "undead_name_0", "Graue");
    locale_setstring(default_locale, "undead_postfix_0", "Kobolde");
    CuAssertPtrNotNull(tc, rc->name_unit);
    CuAssertTrue(tc, rc->name_unit == (race_func)get_function("name_undead"));
    name_unit(u);
    CuAssertStrEquals(tc, "Graue Kobolde", u->_name);
    test_cleanup();
}

static void test_monster_names(CuTest *tc) {
    unit *u;
    faction *f;
    race *rc;

    test_setup();
    register_names();
    default_locale = test_create_locale();
    locale_setstring(default_locale, "race::irongolem", "Eisengolem");
    locale_setstring(default_locale, "race::irongolem_p", "Eisengolems");
    rc = test_create_race("irongolem");
    f = test_create_faction(rc);
    f->flags |= FFL_NPC;
    u = test_create_unit(f, test_create_region(0, 0, 0));
    unit_setname(u, "Hodor");
    CuAssertPtrNotNull(tc, u->_name);
    name_unit(u);
    CuAssertPtrEquals(tc, NULL, u->_name);
    CuAssertStrEquals(tc, "Eisengolem", unit_getname(u));
    u->number = 2;
    CuAssertStrEquals(tc, "Eisengolems", unit_getname(u));
    test_cleanup();
}

CuSuite *get_names_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_names);
    SUITE_ADD_TEST(suite, test_monster_names);
    return suite;
}
