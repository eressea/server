#include <platform.h>
#include <kernel/curse.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/order.h>
#include <kernel/region.h>
#include <kernel/spell.h>
#include <kernel/unit.h>
#include <util/message.h>
#include <util/language.h>
#include <kernel/attrib.h>
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

    test_setup();
    mt_create_va(mt_new("magicresistance_unit", NULL),
        "unit:unit", "id:int", MT_NEW_END);
    r = test_create_plain(0, 0);
    f1 = test_create_faction(NULL);
    u1 = test_create_unit(f1, r);

    f2 = test_create_faction(NULL);
    u2 = test_create_unit(f2, r);

    c = create_curse(u1, &u2->attribs, &ct_magicresistance, 10, 20, 30, u2->number);
    CuAssertPtrNotNull(tc, u2->attribs);
    CuAssertPtrEquals(tc, (void *)&at_curse, (void *)u2->attribs->type);
    msg = c->type->curseinfo(u2, TYP_UNIT, c, 1);
    CuAssertPtrNotNull(tc, msg);
    CuAssertStrEquals(tc, "magicresistance_unit", test_get_messagetype(msg));
    msg_release(msg);

    test_teardown();
}

static void test_magicresistance_building(CuTest *tc) {
    struct region *r;
    struct faction *f1;
    unit *u1;
    building *b1;
    message *msg;
    curse *c;

    test_setup();
    mt_create_va(mt_new("magicresistance_building", NULL),
        "building:building", "id:int", MT_NEW_END);
    r = test_create_plain(0, 0);
    f1 = test_create_faction(NULL);
    u1 = test_create_unit(f1, r);

    b1 = test_create_building(r, NULL);

    c = create_curse(u1, &b1->attribs, &ct_magicresistance, 10, 20, 30, 0);
    CuAssertPtrNotNull(tc, b1->attribs);
    CuAssertPtrEquals(tc, (void *)&at_curse, (void *)b1->attribs->type);
    msg = c->type->curseinfo(b1, TYP_BUILDING, c, 1);
    CuAssertPtrNotNull(tc, msg);
    CuAssertStrEquals(tc, "magicresistance_building", test_get_messagetype(msg));
    msg_release(msg);
    test_teardown();
}

CuSuite *get_magicresistance_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_magicresistance_unit);
    SUITE_ADD_TEST(suite, test_magicresistance_building);
    return suite;
}
