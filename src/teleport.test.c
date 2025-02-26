#include "teleport.h"

#include <kernel/config.h>
#include <kernel/region.h>
#include <kernel/terrain.h>

#include "tests.h"

#include <CuTest.h>

#include <stddef.h>          // for NULL

static void test_update_teleport(CuTest *tc) {
    region* r1, *r2, *ra;
    struct plane* aplane;
    const struct terrain_type *t_dense, *t_fog, *t_firewall;

    test_setup();
    t_fog = test_create_terrain("fog", LAND_REGION);
    t_dense = test_create_terrain("thickfog", FORBIDDEN_REGION);
    t_firewall = test_create_terrain("firewall", FORBIDDEN_REGION);
    /* astral space is off by default: */
    CuAssertPtrEquals(tc, NULL, get_astralplane());
    r1 = test_create_plain(0, 0);
    CuAssertPtrEquals(tc, NULL, r_standard_to_astral(r1));
    r2 = test_create_region(1, 0, t_firewall);
    CuAssertPtrEquals(tc, NULL, r_standard_to_astral(r2));

    /* enabling astral space and updating existing regions creates new ones: */
    config_set_int("modules.astralspace", 1);
    CuAssertPtrNotNull(tc, aplane = get_astralplane());
    update_teleport_plane(r1, aplane, t_fog, t_dense);
    ra = r_standard_to_astral(r1);
    CuAssertPtrNotNull(tc, ra);
    CuAssertPtrEquals(tc, (void *)t_fog, (void*)ra->terrain);

    /* any firewall found under the astral region will cause a block: */
    update_teleport_plane(r2, aplane, t_fog, t_dense);
    CuAssertPtrEquals(tc, (void*)ra, (void*)r_standard_to_astral(r2));
    CuAssertPtrEquals(tc, (void*)t_dense, (void*)ra->terrain);

    /* finding a non-blocking region does not unblock astral space: */
    update_teleport_plane(r1, aplane, t_fog, t_dense);
    CuAssertPtrEquals(tc, (void *)ra, (void *)r_standard_to_astral(r2));
    CuAssertPtrEquals(tc, (void *)t_dense, (void *)ra->terrain);

    test_teardown();
}

static void test_terraform_updates_teleport(CuTest *tc) {
    region *r1, *r2, *ra;
    struct plane *aplane;
    const struct terrain_type *t_dense, *t_fog, *t_firewall, *t_plain;

    test_setup();
    t_fog = test_create_terrain("fog", LAND_REGION);
    t_dense = test_create_terrain("thickfog", FORBIDDEN_REGION);
    t_firewall = test_create_terrain("firewall", FORBIDDEN_REGION);
    t_plain = test_create_terrain("plain", LAND_REGION);
    /* astral space is off by default: */
    CuAssertPtrEquals(tc, NULL, get_astralplane());
    r1 = test_create_region(0, 0, t_plain);
    CuAssertPtrEquals(tc, NULL, r_standard_to_astral(r1));
    r2 = test_create_region(1, 0, t_plain);
    CuAssertPtrEquals(tc, NULL, r_standard_to_astral(r2));
    config_set_int("modules.astralspace", 1);
    CuAssertPtrNotNull(tc, aplane = get_astralplane());

    config_set_int("modules.astralspace", 1);
    /* single blocked region creates dense fog: */
    terraform_region(r1, t_firewall);
    CuAssertPtrNotNull(tc, ra = r_standard_to_astral(r1));
    CuAssertPtrEquals(tc, ra, r_standard_to_astral(r2));
    CuAssertPtrEquals(tc, (void *)t_dense, (void *)ra->terrain);
    /* additional blocked regions, no change: */
    terraform_region(r2, t_firewall);
    CuAssertPtrNotNull(tc, ra = r_standard_to_astral(r1));
    CuAssertPtrEquals(tc, (void *)t_dense, (void *)ra->terrain);

    /* unblocking only one region does nothing: */
    terraform_region(r1, t_plain);
    CuAssertPtrNotNull(tc, ra = r_standard_to_astral(r1));
    CuAssertPtrEquals(tc, (void *)t_dense, (void *)ra->terrain);

    /* unblocking both unblocks astral space: */
    terraform_region(r2, t_plain);
    CuAssertPtrNotNull(tc, ra = r_standard_to_astral(r1));
    CuAssertPtrEquals(tc, (void *)t_fog, (void *)ra->terrain);

    test_teardown();
}


CuSuite *get_teleport_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_update_teleport);
    SUITE_ADD_TEST(suite, test_terraform_updates_teleport);
    return suite;
}
