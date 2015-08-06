#include <platform.h>
#include <kernel/config.h>
#include <kernel/curse.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/order.h>
#include <kernel/region.h>
#include <kernel/spell.h>
#include <kernel/unit.h>
#include <util/message.h>
#include <util/language.h>
#include <util/attrib.h>
#include <spells/regioncurse.h>
#include "spells.h"

#include <CuTest.h>
#include <tests.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


static void test_magicresistance_unit(CuTest *tc) {
    struct region *r;
    struct faction *f1, *f2;
    unit *u1, *u2;
    message *msg;
    curse *c;

    test_cleanup();
    test_create_world();
    r=findregion(0, 0);
    f1 = test_create_faction(test_create_race("human"));
    u1 = test_create_unit(f1, r);

    f2 = test_create_faction(test_create_race("human"));
    u2 = test_create_unit(f2, r);

    c = create_curse(u1, &u2->attribs, ct_find("magicresistance"), 10, 20, 30, u2->number);
    CuAssertPtrNotNull(tc, u2->attribs);
    CuAssertPtrEquals(tc, (void *)&at_curse, (void *)u2->attribs->type);
    msg = c->type->curseinfo(u2, TYP_UNIT, c, 1);
    CuAssertPtrNotNull(tc, msg);
    CuAssertStrEquals(tc, "curseinfo::magicresistance", test_get_messagetype(msg));

    test_cleanup();
}

static void test_magicresistance_building(CuTest *tc) {
    struct region *r;
    struct faction *f1;
    unit *u1;
    building *b1;
    message *msg;
    curse *c;

    test_cleanup();
    test_create_world();
    r = findregion(0, 0);
    f1 = test_create_faction(test_create_race("human"));
    u1 = test_create_unit(f1, r);

    b1 = test_create_building(r, test_create_buildingtype("castle"));

    c = create_curse(u1, &b1->attribs, ct_find("magicresistance"), 10, 20, 30, 0);
    CuAssertPtrNotNull(tc, b1->attribs);
    CuAssertPtrEquals(tc, (void *)&at_curse, (void *)b1->attribs->type);
    msg = c->type->curseinfo(b1, TYP_BUILDING, c, 1);
    CuAssertPtrNotNull(tc, msg);
    CuAssertStrEquals(tc, "curseinfo::homestone", test_get_messagetype(msg));

    test_cleanup();
}

CuSuite *get_magicresistance_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_magicresistance_unit);
    SUITE_ADD_TEST(suite, test_magicresistance_building);
    return suite;
}
