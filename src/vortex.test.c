#include <platform.h>
#include <kernel/types.h>

#include "vortex.h"
#include "move.h"
#include "tests.h"

#include <kernel/config.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/race.h>

#include <util/language.h>

#include <CuTest.h>

static void test_move_to_vortex(CuTest *tc) {
    region *r1, *r2, *r = 0;
    terrain_type *t_plain;
    unit *u;
    struct locale *lang;

    test_setup();
    lang = test_create_locale();
    locale_setstring(lang, "vortex", "wirbel");
    init_locale(lang);
    t_plain = test_create_terrain("plain", LAND_REGION | FOREST_REGION | WALK_INTO | CAVALRY_REGION);
    r1 = test_create_region(0, 0, t_plain);
    r2 = test_create_region(5, 0, t_plain);
    CuAssertPtrNotNull(tc, create_special_direction(r1, r2, 10, "", "vortex", true));
    u = test_create_unit(test_create_faction(rc_get_or_create("hodor")), r1);
    u->faction->locale = lang;
    CuAssertIntEquals(tc, E_MOVE_NOREGION, movewhere(u, "barf", r1, &r));
    CuAssertIntEquals(tc, E_MOVE_OK, movewhere(u, "wirbel", r1, &r));
    CuAssertPtrEquals(tc, r2, r);
    test_teardown();
}

CuSuite *get_vortex_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_move_to_vortex);
    return suite;
}
