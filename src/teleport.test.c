#include "teleport.h"

#include <kernel/config.h>
#include <kernel/plane.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

#include "reports.h"
#include "tests.h"

#include <CuTest.h>

static void test_update_teleport(CuTest *tc) {
    int n = 0;
    region* r1, *r2, *ra;
    struct plane* aplane;
    const struct terrain_type *t_dense, *t_fog, *t_firewall;

    test_setup();
    t_dense = test_create_terrain("fog", FORBIDDEN_REGION);
    t_fog = test_create_terrain("thickfog", LAND_REGION);
    t_firewall = test_create_terrain("firewall", FORBIDDEN_REGION);
    r1 = test_create_plain(0, 0);
    r2 = test_create_region(1, 0, t_firewall);
    ra = r_standard_to_astral(r1);
    config_set_int("modules.astralspace", 1);
    CuAssertPtrEquals(tc, NULL, ra);
    aplane = get_astralplane();
    update_teleport_plane(r1, aplane, t_fog, t_dense);
    ra = r_standard_to_astral(r1);
    CuAssertPtrNotNull(tc, ra);
    CuAssertPtrEquals(tc, (void *)t_fog, (void*)ra->terrain);
    update_teleport_plane(r2, aplane, t_fog, t_dense);
    CuAssertPtrEquals(tc, (void*)ra, (void*)r_standard_to_astral(r2));
    CuAssertPtrEquals(tc, (void*)t_dense, (void*)ra->terrain);

    test_teardown();
}

CuSuite *get_teleport_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_update_teleport);
    return suite;
}
