#include "magic.h"

#include "spells/regioncurse.h"

#include <kernel/ally.h>
#include <kernel/attrib.h>
#include <kernel/building.h>
#include <kernel/curse.h>
#include <kernel/faction.h>
#include "kernel/objtypes.h"     // for TYP_BUILDING, TYP_UNIT
#include <kernel/region.h>
#include <kernel/unit.h>

#include "util/message.h"
#include "util/variant.h"        // for frac_make, frac_sub, variant, frac_add

#include <CuTest.h>
#include <tests.h>

#include <stdio.h>

/* TODO:
 * - ct_magicresistance on buildings
 * - stone circles
 */

static void test_magicresistance_curse_effects(CuTest *tc) {
    struct region *r;
    unit *u1, *u2;
    variant var;
    curse *c;

    test_setup();
    r = test_create_plain(0, 0);

    u1 = test_create_unit(test_create_faction(), r);
    u2 = test_create_unit(test_create_faction(), r);

    /* increase someone's magic resistance by 10 percent */
    c = create_curse(u1, &u2->attribs, &ct_magicresistance, 1, 1, 10, 1);
    var = magic_resistance(u2);
    var = frac_sub(var, frac_make(1, 10));
    CuAssertIntEquals(tc, 0, var.sa[0]);

    /* curse only works for 1 person, is reduced for more */
    scale_number(u2, 2);
    var = magic_resistance(u2);
    var = frac_sub(var, frac_make(1, 20));
    CuAssertIntEquals(tc, 0, var.sa[0]);
    remove_curse(&u2->attribs, c);

    /* create an increase of magic resistance by 10 percent for our friends */
    c = create_curse(u1, &r->attribs, &ct_goodmagicresistancezone, 1, 1, 10, 0);
    /* curse increases our own resistance: */
    var = magic_resistance(u1);
    var = frac_sub(var, frac_make(1, 10));
    CuAssertIntEquals(tc, 0, var.sa[0]);
    /* no effect on non-allied units: */
    var = magic_resistance(u2);
    CuAssertIntEquals(tc, 0, var.sa[0]);
    /* allied units also get the bonus: */
    ally_set(&u1->faction->allies, u2->faction, HELP_GUARD);
    var = magic_resistance(u2);
    var = frac_sub(var, frac_make(1, 10));
    CuAssertIntEquals(tc, 0, var.sa[0]);
    ally_set(&u1->faction->allies, u2->faction, 0);
    remove_curse(&r->attribs, c);

    /* reduce magic resistance for non-friendly units */
    c = create_curse(u1, &r->attribs, &ct_badmagicresistancezone, 1, 1, 10, 0);
    /* we do not affect ourselves: */
    var = magic_resistance(u1);
    CuAssertIntEquals(tc, 0, var.sa[0]);
    /* non-allied units get the effect, even if it takes them into negative: */
    var = magic_resistance(u2);
    var = frac_add(var, frac_make(1, 10));
    CuAssertIntEquals(tc, 0, var.sa[0]);
    /* no effect on allied units: */
    ally_set(&u1->faction->allies, u2->faction, HELP_GUARD);
    var = magic_resistance(u2);
    CuAssertIntEquals(tc, 0, var.sa[0]);
    ally_set(&u1->faction->allies, u2->faction, 0);
    remove_curse(&r->attribs, c);

    test_teardown();
}

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
    f1 = test_create_faction();
    u1 = test_create_unit(f1, r);

    f2 = test_create_faction();
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

/** 
 * Test for spell "Heimstein"
 */
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
    f1 = test_create_faction();
    u1 = test_create_unit(f1, r);

    b1 = test_create_building(r, NULL);
    u1->building = b1;
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
    SUITE_ADD_TEST(suite, test_magicresistance_curse_effects);
    SUITE_ADD_TEST(suite, test_magicresistance_unit);
    SUITE_ADD_TEST(suite, test_magicresistance_building);
    return suite;
}
