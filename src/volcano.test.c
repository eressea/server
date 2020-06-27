#include <platform.h>
#include <tests.h>
#include "volcano.h"

#include <kernel/faction.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>
#include <kernel/messages.h>

#include <kernel/attrib.h>

#include <attributes/reduceproduction.h>

#include <CuTest.h>

static void test_volcano_update(CuTest *tc) {
    region *r;
    message *m;
    const struct terrain_type *t_volcano, *t_active;
    
    test_setup();
    mt_create_va(mt_new("volcanostopsmoke", NULL),
        "region:region", MT_NEW_END);
    t_volcano = test_create_terrain("volcano", LAND_REGION);
    t_active = test_create_terrain("activevolcano", LAND_REGION);
    r = test_create_region(0, 0, t_active);
    a_add(&r->attribs, make_reduceproduction(25, 10));

    volcano_update();
    CuAssertPtrNotNull(tc, m = test_find_messagetype(r->msgs, "volcanostopsmoke"));
    CuAssertPtrEquals(tc, r, m->parameters[0].v);
    CuAssertPtrEquals(tc, (void *)t_volcano, (void *)r->terrain);
    
    test_teardown();
}

static void test_volcano_outbreak(CuTest *tc) {
    region *r, *rn;
    unit *u1, *u2;
    faction *f;
    message *m;
    const struct terrain_type *t_volcano, *t_active;
    
    test_setup();
    mt_create_va(mt_new("volcanooutbreak", NULL), 
        "regionv:region", "regionn:region", MT_NEW_END);
    mt_create_va(mt_new("volcanooutbreaknn", NULL),
        "region:region", MT_NEW_END);
    mt_create_va(mt_new("volcano_dead", NULL),
        "unit:unit", "region:region", "dead:int", MT_NEW_END);
    t_volcano = test_create_terrain("volcano", LAND_REGION);
    t_active = test_create_terrain("activevolcano", LAND_REGION);
    r = test_create_region(0, 0, t_active);
    rn = test_create_region(1, 0, t_volcano);
    f = test_create_faction(NULL);
    u1 = test_create_unit(f, r);
    u1->hp = u1->number;
    u2 = test_create_unit(f, rn);
    u2->hp = u2->number;

    volcano_outbreak(r, rn);
    CuAssertPtrEquals(tc, (void *)t_active, (void *)r->terrain);
    CuAssertIntEquals(tc, 0, rtrees(r, 0));
    CuAssertIntEquals(tc, 0, rtrees(r, 1));
    CuAssertIntEquals(tc, 0, rtrees(r, 2));
    CuAssertPtrNotNull(tc, a_find(r->attribs, &at_reduceproduction));
    CuAssertPtrNotNull(tc, a_find(rn->attribs, &at_reduceproduction));

    CuAssertPtrNotNull(tc, m = test_find_messagetype(rn->msgs, "volcanooutbreak"));
    CuAssertPtrEquals(tc, r, m->parameters[0].v);
    CuAssertPtrEquals(tc, rn, m->parameters[1].v);

    CuAssertPtrNotNull(tc, m = test_find_messagetype(f->msgs, "volcanooutbreaknn"));
    CuAssertPtrEquals(tc, r, m->parameters[0].v);

    CuAssertPtrNotNull(tc, m = test_find_messagetype_ex(f->msgs, "volcano_dead", NULL));
    CuAssertPtrEquals(tc, u1, m->parameters[0].v);
    CuAssertPtrEquals(tc, r, m->parameters[1].v);
    CuAssertIntEquals(tc, 1, m->parameters[2].i);
    CuAssertPtrNotNull(tc, m = test_find_messagetype_ex(f->msgs, "volcano_dead", m));
    CuAssertPtrEquals(tc, u2, m->parameters[0].v);
    CuAssertPtrEquals(tc, r, m->parameters[1].v);
    CuAssertIntEquals(tc, 1, m->parameters[2].i);
    test_teardown();
}

CuSuite *get_volcano_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_volcano_update);
    SUITE_ADD_TEST(suite, test_volcano_outbreak);
    return suite;
}
