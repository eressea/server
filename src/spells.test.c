#include <platform.h>

#include "spells.h"
#include "teleport.h"

#include <kernel/config.h>
#include <kernel/curse.h>
#include <kernel/faction.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/spell.h>
#include <kernel/unit.h>
#include <util/attrib.h>
#include <util/language.h>
#include <util/message.h>
#include <spells/regioncurse.h>

#include <attributes/attributes.h>

#include <CuTest.h>
#include <tests.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void test_good_dreams(CuTest *tc) {
    struct region *r;
    struct faction *f1, *f2;
    unit *u1, *u2;
    int level;
    castorder co;
    curse *curse;
    
    test_setup();
    test_create_world();
    r = findregion(0, 0);
    f1 = test_create_faction(NULL);
    f2 = test_create_faction(NULL);
    u1 = test_create_unit(f1, r);
    u2 = test_create_unit(f2, r);

    test_create_castorder(&co, u1, 10, 10., 0, NULL);

    level = sp_gooddreams(&co);
    CuAssertIntEquals(tc, 10, level);
    curse = get_curse(r->attribs, &ct_gbdream);
    CuAssertTrue(tc, curse && curse->duration > 1);
    CuAssertTrue(tc, curse->effect == 1);

    a_age(&r->attribs, r);
    CuAssertIntEquals_Msg(tc, "good dreams give +1 to allies", 1, get_modifier(u1, SK_MELEE, 11, r, false));
    CuAssertIntEquals_Msg(tc, "good dreams have no effect on non-allies", 0, get_modifier(u2, SK_MELEE, 11, r, false));
    free_castorder(&co);
    test_teardown();
}

static void test_dreams(CuTest *tc) {
    struct region *r;
    struct faction *f1, *f2;
    unit *u1, *u2;
    castorder co;

    test_setup();
    r = test_create_region(0, 0, NULL);
    f1 = test_create_faction(NULL);
    f2 = test_create_faction(NULL);
    u1 = test_create_unit(f1, r);
    u2 = test_create_unit(f2, r);

    test_create_castorder(&co, u1, 10, 10., 0, NULL);

    sp_gooddreams(&co);
    sp_baddreams(&co);
    a_age(&r->attribs, r);
    CuAssertIntEquals_Msg(tc, "good dreams in same region as bad dreams", 1, get_modifier(u1, SK_MELEE, 11, r, false));
    CuAssertIntEquals_Msg(tc, "bad dreams in same region as good dreams", -1, get_modifier(u2, SK_MELEE, 11, r, false));

    free_castorder(&co);
    test_teardown();
}

static void test_bad_dreams(CuTest *tc) {
    struct region *r;
    struct faction *f1, *f2;
    unit *u1, *u2;
    int level;
    castorder co;
    curse *curse;
    
    test_setup();
    test_create_world();
    r = findregion(0, 0);
    f1 = test_create_faction(NULL);
    f2 = test_create_faction(NULL);
    u1 = test_create_unit(f1, r);
    u2 = test_create_unit(f2, r);

    test_create_castorder(&co, u1, 10, 10., 0, NULL);

    level = sp_baddreams(&co);
    CuAssertIntEquals(tc, 10, level);
    curse = get_curse(r->attribs, &ct_gbdream);
    CuAssertTrue(tc, curse && curse->duration > 1);
    CuAssertTrue(tc, curse->effect == -1);

    a_age(&r->attribs, r);
    CuAssertIntEquals_Msg(tc, "bad dreams have no effect on allies", 0, get_modifier(u1, SK_MELEE, 11, r, false));
    CuAssertIntEquals_Msg(tc, "bad dreams give -1 to non-allies", -1, get_modifier(u2, SK_MELEE, 11, r, false));

    free_castorder(&co);
    test_teardown();
}

static void test_view_reality(CuTest *tc) {
    region *r, *ra;
    faction *f;
    unit *u;
    castorder co;

    test_setup();
    mt_register(mt_new_va("spell_astral_only", "unit:unit", "region:region", "command:order", NULL));
    mt_register(mt_new_va("viewreality_effect", "unit:unit", NULL));
    r = test_create_region(0, 0, NULL);
    ra = test_create_region(real2tp(r->x), real2tp(r->y), NULL);
    ra->_plane = get_astralplane();
    f = test_create_faction(NULL);
    u = test_create_unit(f, r);

    test_create_castorder(&co, u, 10, 10., 0, NULL);
    CuAssertIntEquals(tc, -1, get_observer(r, f));
    CuAssertIntEquals(tc, 0, sp_viewreality(&co));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "spell_astral_only"));
    free_castorder(&co);

    test_clear_messagelist(&f->msgs);
    move_unit(u, ra, NULL);

    test_create_castorder(&co, u, 9, 10., 0, NULL);
    CuAssertIntEquals(tc, -1, get_observer(r, f));
    CuAssertIntEquals(tc, 9, sp_viewreality(&co));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "spell_astral_only"));
    CuAssertIntEquals(tc, 4, get_observer(r, f));
    CuAssertPtrEquals(tc, f, (void *)ra->individual_messages->viewer);
    CuAssertPtrNotNull(tc, test_find_messagetype(ra->individual_messages->msgs, "viewreality_effect"));
    free_castorder(&co);

    test_teardown();
}

static void test_watch_region(CuTest *tc) {
    region *r;
    faction *f;
    test_setup();
    r = test_create_region(0, 0, NULL);
    f = test_create_faction(NULL);
    CuAssertIntEquals(tc, -1, get_observer(r, f));
    set_observer(r, f, 0, 2);
    CuAssertIntEquals(tc, 0, get_observer(r, f));
    set_observer(r, f, 10, 2);
    CuAssertIntEquals(tc, 10, get_observer(r, f));
    CuAssertIntEquals(tc, RF_OBSERVER, fval(r, RF_OBSERVER));
    CuAssertPtrNotNull(tc, r->attribs);
    test_teardown();
}

CuSuite *get_spells_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_watch_region);
    SUITE_ADD_TEST(suite, test_view_reality);
    SUITE_ADD_TEST(suite, test_good_dreams);
    SUITE_ADD_TEST(suite, test_bad_dreams);
    SUITE_ADD_TEST(suite, test_dreams);
    return suite;
}
