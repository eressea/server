#include <platform.h>

#include "spells.h"
#include "teleport.h"

#include <kernel/config.h>
#include <kernel/curse.h>
#include <kernel/event.h>
#include <kernel/faction.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/spell.h>
#include <kernel/unit.h>
#include <kernel/attrib.h>
#include <util/language.h>
#include <util/message.h>
#include <spells/regioncurse.h>

#include <attributes/attributes.h>

#include <triggers/changerace.h>
#include <triggers/timeout.h>

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
    f1 = test_create_faction();
    f2 = test_create_faction();
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
    f1 = test_create_faction();
    f2 = test_create_faction();
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
    f1 = test_create_faction();
    f2 = test_create_faction();
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
    region *r, *ra, *rx;
    faction *f;
    unit *u;
    castorder co;
    curse *c;

    test_setup();
    mt_create_error(216);
    mt_create_error(220);
    mt_create_va(mt_new("spell_astral_only", NULL),
        "unit:unit", "region:region", "command:order", MT_NEW_END);
    mt_create_va(mt_new("viewreality_effect", NULL),
        "unit:unit", MT_NEW_END);
    rx = test_create_region(0, TP_RADIUS + 1, NULL);
    f = test_create_faction();
    u = test_create_unit(f, rx);

    /* can only cast in astral space */
    test_create_castorder(&co, u, 10, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_viewreality(&co));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "spell_astral_only"));
    free_castorder(&co);

    test_clear_messagelist(&f->msgs);
    ra = test_create_region(real2tp(0), real2tp(0), NULL);
    ra->_plane = get_astralplane();
    move_unit(u, ra, NULL);

    /* there is no connection from ra to rx */
    test_create_castorder(&co, u, 10, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_viewreality(&co));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error216"));
    CuAssertIntEquals(tc, -1, get_observer(rx, f));
    free_castorder(&co);

    test_clear_messagelist(&f->msgs);
    r = test_create_region(0, 0, NULL);

    test_clear_messagelist(&f->msgs);

    /* units exist, r can be seen, but rx is out of range */
    test_create_castorder(&co, u, 9, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 9, sp_viewreality(&co));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "viewreality_effect"));
    CuAssertIntEquals(tc, 5, get_observer(r, f));
    CuAssertIntEquals(tc, -1, get_observer(rx, f));
    free_castorder(&co);

    set_observer(r, f, -1, 0);
    CuAssertIntEquals(tc, -1, get_observer(r, f));

    /* target region r exists, but astral space is blocked */
    c = create_curse(u, &ra->attribs, &ct_astralblock, 50.0, 1, 50, 0);
    test_create_castorder(&co, u, 10, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_viewreality(&co));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error216"));
    CuAssertIntEquals(tc, -1, get_observer(r, f));
    free_castorder(&co);
    remove_curse(&ra->attribs, c);

    /* target region r exists, but astral interference is blocked */
    c = create_curse(u, &r->attribs, &ct_astralblock, 50.0, 1, 50, 0);
    test_create_castorder(&co, u, 10, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_viewreality(&co));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error216"));
    CuAssertIntEquals(tc, -1, get_observer(r, f));
    free_castorder(&co);

    test_teardown();
}

static void test_show_astral(CuTest *tc) {
    region *r, *ra, *rx;
    faction *f;
    unit *u;
    castorder co;
    curse * c;

    test_setup();
    mt_create_error(216);
    mt_create_error(220);
    mt_create_va(mt_new("spell_astral_forbidden", NULL),
        "unit:unit", "region:region", "command:order", MT_NEW_END);
    mt_create_va(mt_new("showastral_effect", NULL),
        "unit:unit", MT_NEW_END);
    ra = test_create_region(real2tp(0), real2tp(0) + 1 + SHOWASTRAL_MAX_RADIUS, NULL);
    ra->_plane = get_astralplane();
    f = test_create_faction();
    u = test_create_unit(f, ra);

    /* error: unit is in astral space */
    test_create_castorder(&co, u, 10, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_showastral(&co));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "spell_astral_forbidden"));
    free_castorder(&co);

    test_clear_messagelist(&f->msgs);
    r = test_create_region(0, 0, NULL);
    move_unit(u, r, NULL);

    /* error: no target region */
    test_create_castorder(&co, u, 9, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_showastral(&co));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error216"));
    CuAssertIntEquals(tc, -1, get_observer(ra, f));
    free_castorder(&co);

    rx = test_create_region(real2tp(r->x), real2tp(r->y), NULL);
    rx->_plane = ra->_plane;

    /* rx is in range, but empty */
    test_create_castorder(&co, u, 9, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_showastral(&co));
    CuAssertIntEquals(tc, -1, get_observer(rx, f));
    CuAssertIntEquals(tc, -1, get_observer(ra, f));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error220"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "showastral_effect"));
    free_castorder(&co);

    test_create_unit(f, ra);
    test_create_unit(f, rx);
    /* rx is in range, but ra is not */
    test_create_castorder(&co, u, 9, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 9, sp_showastral(&co));
    CuAssertIntEquals(tc, 5, get_observer(rx, f));
    CuAssertIntEquals(tc, -1, get_observer(ra, f));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "showastral_effect"));
    free_castorder(&co);

    /* astral block on r */
    c = create_curse(u, &r->attribs, &ct_astralblock, 50.0, 1, 50, 0);
    test_create_castorder(&co, u, 9, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_showastral(&co));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error216"));
    free_castorder(&co);
    remove_curse(&r->attribs, c);

    /* astral block on rx */
    c = create_curse(u, &rx->attribs, &ct_astralblock, 50.0, 1, 50, 0);
    test_create_castorder(&co, u, 9, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_showastral(&co));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error220"));
    free_castorder(&co);
    remove_curse(&rx->attribs, c);

    test_teardown();
}

static void test_watch_region(CuTest *tc) {
    region *r;
    faction *f;
    test_setup();
    r = test_create_region(0, 0, NULL);
    f = test_create_faction();
    CuAssertIntEquals(tc, -1, get_observer(r, f));
    set_observer(r, f, 0, 2);
    CuAssertIntEquals(tc, 0, get_observer(r, f));
    set_observer(r, f, 10, 2);
    CuAssertIntEquals(tc, 10, get_observer(r, f));
    CuAssertIntEquals(tc, RF_OBSERVER, fval(r, RF_OBSERVER));
    CuAssertPtrNotNull(tc, r->attribs);
    test_teardown();
}

static void test_change_race(CuTest *tc) {
    unit *u;
    race *rctoad;
    trigger **tp, *tr;
    timeout_data *td;
    changerace_data *crd;

    test_setup();
    rctoad = test_create_race("toad");
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    CuAssertPtrEquals(tc, (void *)u->faction->race, (void *)u->_race);
    CuAssertPtrNotNull(tc, change_race(u, 2, rctoad, NULL));
    CuAssertPtrEquals(tc, (void *)rctoad, (void *)u->_race);
    CuAssertPtrEquals(tc, NULL, (void *)u->irace);
    CuAssertPtrNotNull(tc, u->attribs);
    tp = get_triggers(u->attribs, "timer");
    CuAssertPtrNotNull(tc, tp);
    CuAssertPtrNotNull(tc, tr = *tp);
    CuAssertPtrEquals(tc, &tt_timeout, tr->type);
    td = (timeout_data *)tr->data.v;
    CuAssertPtrNotNull(tc, td);
    CuAssertIntEquals(tc, 2, td->timer);
    CuAssertPtrNotNull(tc, td->triggers);
    CuAssertPtrEquals(tc, &tt_changerace, td->triggers->type);
    CuAssertPtrEquals(tc, NULL, td->triggers->next);
    crd = (changerace_data *)td->triggers->data.v;
    CuAssertPtrEquals(tc, (void *)u->faction->race, (void *)crd->race);
    CuAssertPtrEquals(tc, NULL, (void *)crd->irace);

    test_teardown();
}

CuSuite *get_spells_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_watch_region);
    SUITE_ADD_TEST(suite, test_view_reality);
    SUITE_ADD_TEST(suite, test_show_astral);
    SUITE_ADD_TEST(suite, test_good_dreams);
    SUITE_ADD_TEST(suite, test_bad_dreams);
    SUITE_ADD_TEST(suite, test_dreams);
    SUITE_ADD_TEST(suite, test_change_race);
    return suite;
}
