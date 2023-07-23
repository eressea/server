#include "spells.h"

#include "contact.h"
#include "magic.h"
#include "teleport.h"

#include <kernel/curse.h>
#include <kernel/event.h>
#include <kernel/faction.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include "kernel/ship.h"            // for SK_MELEE
#include "kernel/skill.h"            // for SK_MELEE
#include <kernel/terrain.h>
#include <kernel/unit.h>
#include <kernel/order.h>
#include <kernel/attrib.h>
#include <util/message.h>
#include "util/variant.h"  // for variant

#include <spells/regioncurse.h>
#include <spells/unitcurse.h>
#include <spells/shipcurse.h>
#include <spells/flyingship.h>
#include <attributes/attributes.h>

#include <triggers/changerace.h>
#include <triggers/timeout.h>

#include <CuTest.h>
#include <tests.h>

#include <stdbool.h>                 // for false
#include <stdio.h>

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
    test_teardown();
}

static void test_dreams(CuTest *tc) {
    struct region *r;
    struct faction *f1, *f2;
    unit *u1, *u2;
    castorder co;

    test_setup();
    r = test_create_plain(0, 0);
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

    test_teardown();
}

static void test_change_race(CuTest* tc) {
    unit* u;
    race* rctoad, * rcsmurf;
    trigger** tp, * tr;
    timeout_data* td;
    changerace_data* crd;

    test_setup();
    rctoad = test_create_race("toad");
    rcsmurf = test_create_race("smurf");
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    CuAssertPtrEquals(tc, (void*)u->faction->race, (void*)u->_race);
    CuAssertPtrNotNull(tc, tr = change_race(u, 2, rctoad, NULL));
    CuAssertPtrEquals(tc, (void*)rctoad, (void*)u->_race);
    CuAssertPtrEquals(tc, NULL, (void*)u->irace);
    CuAssertPtrEquals(tc, &tt_timeout, tr->type);
    CuAssertPtrNotNull(tc, u->attribs);
    CuAssertPtrEquals(tc, NULL, u->attribs->next);
    tp = get_triggers(u->attribs, "timer");
    CuAssertPtrNotNull(tc, tp);
    CuAssertPtrEquals(tc, tr, *tp);
    CuAssertPtrEquals(tc, NULL, tr->next);
    td = (timeout_data*)tr->data.v;
    CuAssertPtrNotNull(tc, td);
    CuAssertIntEquals(tc, 2, td->timer);
    CuAssertPtrNotNull(tc, td->triggers);
    CuAssertPtrEquals(tc, &tt_changerace, td->triggers->type);
    CuAssertPtrEquals(tc, NULL, td->triggers->next);
    crd = (changerace_data*)td->triggers->data.v;
    CuAssertPtrEquals(tc, (void*)u->faction->race, (void*)crd->race);
    CuAssertPtrEquals(tc, NULL, (void*)crd->irace);

    /* change race, but do not add a second change_race trigger */
    CuAssertPtrEquals(tc, tr, change_race(u, 2, rcsmurf, NULL));
    CuAssertPtrNotNull(tc, u->attribs);
    CuAssertPtrEquals(tc, NULL, u->attribs->next);
    CuAssertPtrEquals(tc, NULL, tr->next);
    CuAssertPtrEquals(tc, (void*)rcsmurf, (void*)u->_race);
    CuAssertPtrEquals(tc, NULL, (void*)u->irace);
    td = (timeout_data*)tr->data.v;
    CuAssertPtrNotNull(tc, td);
    CuAssertIntEquals(tc, 2, td->timer);
    CuAssertPtrNotNull(tc, td->triggers);
    CuAssertPtrEquals(tc, &tt_changerace, td->triggers->type);
    CuAssertPtrEquals(tc, NULL, td->triggers->next);
    crd = (changerace_data*)td->triggers->data.v;
    CuAssertPtrEquals(tc, (void*)u->faction->race, (void*)crd->race);
    CuAssertPtrEquals(tc, NULL, (void*)crd->irace);

    test_teardown();
}

static void test_speed2(CuTest *tc) {
    struct region *r;
    struct faction *f;
    unit *u1, *u2;
    castorder co;
    curse* c;
    spellparameter args;
    spllprm param;
    spllprm *params = &param;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    scale_number(u2, 30);

    args.length = 1;
    args.param = &params;
    param.flag = 0;
    param.typ = SPP_UNIT;
    param.data.u = u2;
    
    /* force 4, kann bis zu 32 Personen verzaubern */
    test_create_castorder(&co, u1, 3, 4., 0, &args);
    CuAssertIntEquals(tc, co.level, sp_speed2(&co));
    CuAssertPtrNotNull(tc, c = get_curse(u2->attribs, &ct_speed));
    CuAssertDblEquals(tc, 2.0, c->effect, 0.01);
    CuAssertIntEquals(tc, (1 + co.level) / 2, c->duration);
    CuAssertIntEquals(tc, u2->number, c->data.i);
    CuAssertDblEquals(tc, co.force, c->vigour, 0.01);
    a_removeall(&u2->attribs, NULL);

    /* force 3, kann nur bis zu 18 Personen verzaubern */
    test_create_castorder(&co, u1, 1, 3., 0, &args);
    CuAssertIntEquals(tc, co.level, sp_speed2(&co));
    CuAssertPtrNotNull(tc, c = get_curse(u2->attribs, &ct_speed));
    CuAssertDblEquals(tc, 2.0, c->effect, 0.01);
    CuAssertIntEquals(tc, (1 + co.level) / 2, c->duration);
    CuAssertIntEquals(tc, 18, c->data.i);
    CuAssertDblEquals(tc, co.force, c->vigour, 0.01);
    a_removeall(&u2->attribs, NULL);

    /* if target not found, no costs, no effect */
    param.flag = TARGET_NOTFOUND;
    CuAssertIntEquals(tc, 0, sp_speed2(&co));
    CuAssertPtrEquals(tc, NULL, u2->attribs);

    /* if target resists, pay in full, no effect */
    param.flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_speed2(&co));
    CuAssertPtrEquals(tc, NULL, u2->attribs);

    test_teardown();
}

static void test_goodwinds(CuTest *tc) {
    struct region *r;
    struct faction *f;
    unit *u;
    ship* sh;
    castorder co;
    curse* c;
    spellparameter args;
    spllprm param;
    spllprm *params = &param;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    sh = test_create_ship(r, NULL);

    args.length = 1;
    args.param = &params;
    param.flag = 0;
    param.typ = SPP_SHIP;
    param.data.sh = sh;
    
    test_create_castorder(&co, u, 3, 4., 0, &args);
    CuAssertIntEquals(tc, co.level, sp_goodwinds(&co));
    CuAssertPtrNotNull(tc, c = get_curse(sh->attribs, &ct_nodrift));
    CuAssertIntEquals(tc, 1 + co.level, c->duration);
    CuAssertDblEquals(tc, co.force, c->vigour, 0.01);
    a_removeall(&sh->attribs, NULL);

    /* if target not found, no costs, no effect */
    param.flag = TARGET_NOTFOUND;
    CuAssertIntEquals(tc, 0, sp_goodwinds(&co));
    CuAssertPtrEquals(tc, NULL, sh->attribs);

    /* if target resists, pay in full, no effect */
    param.flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_goodwinds(&co));
    CuAssertPtrEquals(tc, NULL, sh->attribs);

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

    test_teardown();
}

static void test_view_reality(CuTest *tc) {
    region *r, *ra, *rx;
    faction *f;
    unit *u;
    castorder co;
    curse *c;

    test_setup();
    test_use_astral();
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

    test_clear_messagelist(&f->msgs);
    ra = test_create_region(real2tp(0), real2tp(0), NULL);
    ra->_plane = get_astralplane();
    move_unit(u, ra, NULL);

    /* there is no connection from ra to rx */
    test_create_castorder(&co, u, 10, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_viewreality(&co));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error216"));
    CuAssertIntEquals(tc, -1, get_observer(rx, f));

    test_clear_messagelist(&f->msgs);
    r = test_create_plain(0, 0);

    test_clear_messagelist(&f->msgs);

    /* units exist, r can be seen, but rx is out of range */
    test_create_castorder(&co, u, 9, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 9, sp_viewreality(&co));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "viewreality_effect"));
    CuAssertIntEquals(tc, 5, get_observer(r, f));
    CuAssertIntEquals(tc, -1, get_observer(rx, f));

    set_observer(r, f, -1, 0);
    CuAssertIntEquals(tc, -1, get_observer(r, f));

    /* target region r exists, but astral space is blocked */
    c = create_curse(u, &ra->attribs, &ct_astralblock, 50.0, 1, 50, 0);
    test_create_castorder(&co, u, 10, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_viewreality(&co));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error216"));
    CuAssertIntEquals(tc, -1, get_observer(r, f));
    remove_curse(&ra->attribs, c);

    /* target region r exists, but astral interference is blocked */
    c = create_curse(u, &r->attribs, &ct_astralblock, 50.0, 1, 50, 0);
    test_create_castorder(&co, u, 10, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_viewreality(&co));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error216"));
    CuAssertIntEquals(tc, -1, get_observer(r, f));

    test_teardown();
}

static void test_show_astral(CuTest *tc) {
    region *r, *ra, *rx;
    faction *f;
    unit *u;
    castorder co;
    curse * c;

    test_setup();
    r = test_create_plain(0, 0);
    test_use_astral();
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

    test_clear_messagelist(&f->msgs);
    move_unit(u, r, NULL);

    /* error: no target region */
    test_create_castorder(&co, u, 9, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_showastral(&co));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error216"));
    CuAssertIntEquals(tc, -1, get_observer(ra, f));

    rx = test_create_region(real2tp(r->x), real2tp(r->y), NULL);
    rx->_plane = ra->_plane;

    /* rx is in range, but empty */
    test_create_castorder(&co, u, 9, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_showastral(&co));
    CuAssertIntEquals(tc, -1, get_observer(rx, f));
    CuAssertIntEquals(tc, -1, get_observer(ra, f));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error220"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "showastral_effect"));

    test_create_unit(f, ra);
    test_create_unit(f, rx);
    /* rx is in range, but ra is not */
    test_create_castorder(&co, u, 9, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 9, sp_showastral(&co));
    CuAssertIntEquals(tc, 5, get_observer(rx, f));
    CuAssertIntEquals(tc, -1, get_observer(ra, f));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "showastral_effect"));

    /* astral block on r */
    c = create_curse(u, &r->attribs, &ct_astralblock, 50.0, 1, 50, 0);
    test_create_castorder(&co, u, 9, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_showastral(&co));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error216"));
    remove_curse(&r->attribs, c);

    /* astral block on rx */
    c = create_curse(u, &rx->attribs, &ct_astralblock, 50.0, 1, 50, 0);
    test_create_castorder(&co, u, 9, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_showastral(&co));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error220"));
    remove_curse(&rx->attribs, c);

    test_teardown();
}

static void test_watch_region(CuTest *tc) {
    region *r;
    faction *f;
    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    CuAssertIntEquals(tc, -1, get_observer(r, f));
    set_observer(r, f, 0, 2);
    CuAssertIntEquals(tc, 0, get_observer(r, f));
    set_observer(r, f, 10, 2);
    CuAssertIntEquals(tc, 10, get_observer(r, f));
    CuAssertIntEquals(tc, RF_OBSERVER, (r->flags & RF_OBSERVER));
    CuAssertPtrNotNull(tc, r->attribs);
    test_teardown();
}

static void test_summonent(CuTest *tc) {
    unit *u;
    region* r;
    castorder co;
    struct race* rc;

    test_setup();
    rc = test_create_race("ent");
    u = test_create_unit(test_create_faction(), r = test_create_plain(0, 0));
    test_create_castorder(&co, u, 3, 4.0, 0, NULL);

    /* keine Bäume, keine Kosten */
    rsettrees(r, 2, 0);
    CuAssertIntEquals(tc, 0, sp_summonent(&co));
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error204"));
    CuAssertPtrEquals(tc, NULL, u->next);
    test_clear_messages(u->faction);

    /* create 10 ents */
    rsettrees(r, 2, 10);
    CuAssertIntEquals(tc, co.level, sp_summonent(&co));
    CuAssertIntEquals(tc, 0, rtrees(r, 2));
    CuAssertPtrNotNull(tc, test_find_region_message(r, "ent_effect", u->faction));

    CuAssertPtrNotNull(tc, u = u->next);
    CuAssertIntEquals(tc, 10, u->number);
    CuAssertPtrEquals(tc, rc, (void*)u_race(u));
    CuAssertPtrNotNull(tc, a_find(u->attribs, &at_unitdissolve));
    CuAssertPtrNotNull(tc, a_find(u->attribs, &at_creator));
    CuAssertIntEquals(tc, UFL_LOCKED, u->flags & UFL_LOCKED);
    test_clear_region_messages(r);

    /* create 16 ents (force limited) */
    rsettrees(r, 2, 32);
    CuAssertIntEquals(tc, co.level, sp_summonent(&co));
    CuAssertPtrNotNull(tc, test_find_region_message(r, "ent_effect", u->faction));
    CuAssertIntEquals(tc, 16, rtrees(r, 2));
    CuAssertPtrNotNull(tc, u = u->next);
    CuAssertIntEquals(tc, 16, u->number);
    test_clear_region_messages(r);

    test_teardown();
}

static void test_maelstrom(CuTest *tc) {
    unit *u;
    region *r;
    castorder co;
    curse *c;

    test_setup();
    u = test_create_unit(test_create_faction(), r = test_create_ocean(0, 0));
    test_create_castorder(&co, u, 4, 5.0, 0, NULL);
    CuAssertIntEquals(tc, co.level, sp_maelstrom(&co));
    CuAssertPtrNotNull(tc, c = get_curse(r->attribs, &ct_maelstrom));
    CuAssertDblEquals(tc, co.force, c->vigour, 0.01);
    CuAssertDblEquals(tc, co.force, c->effect, 0.01);
    CuAssertIntEquals(tc, co.level + 1, c->duration);
    CuAssertPtrNotNull(tc, test_find_region_message(r, "maelstrom_effect", u->faction));
    test_teardown();
}

static void test_blessedharvest(CuTest *tc) {
    unit *u;
    region *r;
    curse *c;
    castorder co;

    test_setup();
    u = test_create_unit(test_create_faction(), r = test_create_plain(0, 0));
    test_create_castorder(&co, u, 4, 5.0, 0, NULL);
    CuAssertIntEquals(tc, co.level, sp_blessedharvest(&co));
    CuAssertPtrNotNull(tc, c = get_curse(r->attribs, &ct_blessedharvest));
    CuAssertDblEquals(tc, co.force, c->vigour, 0.01);
    CuAssertIntEquals(tc, 1, curse_geteffect_int(c));
    CuAssertIntEquals(tc, co.level + 1, c->duration);
    CuAssertPtrNotNull(tc, test_find_region_message(r, "harvest_effect", u->faction));
    test_teardown();
}

static void test_kaelteschutz(CuTest *tc) {
    unit *u, *u2;
    castorder co;
    spellparameter args;
    spllprm param;
    spllprm* params = &param;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    args.length = 1;
    args.param = &params;
    param.flag = TARGET_RESISTS;
    param.typ = SPP_UNIT;
    param.data.u = u2 = test_create_unit(test_create_faction(), u->region);
    test_create_castorder(&co, u, 4, 5.0, 0, &args);
    CuAssertIntEquals(tc, co.level, sp_kaelteschutz(&co));
    test_teardown();
}

static void test_treewalkenter(CuTest *tc) {
    unit *u, *u2;
    region *r, *ra;
    castorder co;
    spellparameter args;
    spllprm param;
    spllprm *params = &param;

    test_setup();
    test_use_astral();
    u = test_create_unit(test_create_faction(), r = test_create_plain(0, 0));
    rsettrees(r, 2, r->terrain->size / 2);
    ra = test_create_region(real2tp(0), real2tp(0), NULL);
    ra->_plane = get_astralplane();
    args.length = 1;
    args.param = &params;
    param.flag = TARGET_RESISTS;
    param.typ = SPP_UNIT;
    param.data.u = u2 = test_create_unit(test_create_faction(), u->region);
    test_create_castorder(&co, u, 4, 5.0, 0, &args);
    CuAssertIntEquals(tc, 0, sp_treewalkenter(&co));
    CuAssertPtrEquals(tc, r, u2->region);

    param.flag = 0;
    CuAssertIntEquals(tc, 0, sp_treewalkenter(&co));
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "feedback_no_contact"));
    test_clear_messages(u->faction);

    contact_unit(u2, u);
    CuAssertIntEquals(tc, co.level, sp_treewalkenter(&co));
    CuAssertPtrEquals(tc, ra, u2->region);
    CuAssertPtrNotNull(tc, test_find_region_message(r, "astral_disappear", u->faction));
    CuAssertPtrNotNull(tc, test_find_region_message(ra, "astral_appear", u2->faction));
    test_teardown();
}

static void test_treewalkexit(CuTest *tc) {
    unit *u, *u2;
    region *r, *ra;
    castorder co;
    spellparameter args;
    spllprm param[2];
    spllprm *params[2] = { param, param + 1 };

    test_setup();
    test_use_astral();
    r = test_create_plain(0, 0);
    rsettrees(r, 2, r->terrain->size / 2);
    ra = test_create_region(real2tp(0), real2tp(0), NULL);
    ra->_plane = get_astralplane();
    u = test_create_unit(test_create_faction(), ra);
    args.length = 2;
    args.param = params;
    param[0].flag = TARGET_RESISTS;
    param[0].typ = SPP_REGION;
    param[0].data.r = r;
    param[1].flag = 0;
    param[1].typ = SPP_UNIT;
    param[1].data.u = u2 = test_create_unit(test_create_faction(), u->region);
    test_create_castorder(&co, u, 4, 5.0, 0, &args);
    CuAssertIntEquals(tc, co.level, sp_treewalkexit(&co));

    param[0].flag = TARGET_NOTFOUND;
    CuAssertIntEquals(tc, 0, sp_treewalkexit(&co));

    param[0].flag = 0;
    param[1].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_treewalkexit(&co));

    param[0].flag = 0;
    param[1].flag = TARGET_NOTFOUND;
    CuAssertIntEquals(tc, 0, sp_treewalkexit(&co));

    test_teardown();
}

static void test_holyground(CuTest *tc) {
    unit *u;
    castorder co;
    curse *c;
    region *r;

    test_setup();
    u = test_create_unit(test_create_faction(), r = test_create_plain(0, 0));
    deathcounts(r, 100);
    test_create_castorder(&co, u, 4, 5.0, 0, NULL);
    CuAssertIntEquals(tc, co.level, sp_holyground(&co));
    CuAssertPtrNotNull(tc, c = get_curse(r->attribs, &ct_holyground));
    CuAssertDblEquals(tc, co.force * co.force, c->vigour, 0.01);
    CuAssertDblEquals(tc, 0.0, c->effect, 0.01);
    CuAssertIntEquals(tc, 0, deathcount(r));
    test_teardown();
}

static void test_drought(CuTest *tc) {
    unit *u;
    castorder co;
    curse *c;
    region *r;

    test_setup();
    u = test_create_unit(test_create_faction(), r = test_create_plain(0, 0));
    rsettrees(r, 2, 200);
    rsettrees(r, 1, 200);
    rsettrees(r, 0, 200);
    rsethorses(r, 200);
    test_create_castorder(&co, u, 4, 5.0, 0, NULL);
    CuAssertIntEquals(tc, co.level, sp_drought(&co));
    /* Meldung nur bei Fernzaubern: */
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(u->faction->msgs, "drought_effect"));
    CuAssertPtrNotNull(tc, c = get_curse(r->attribs, &ct_drought));
    CuAssertIntEquals(tc, co.level, c->duration);
    CuAssertDblEquals(tc, co.force, c->vigour, 0.01);
    CuAssertDblEquals(tc, 4.0, c->effect, 0.01);
    CuAssertIntEquals(tc, 100, rtrees(r, 2));
    CuAssertIntEquals(tc, 100, rtrees(r, 1));
    CuAssertIntEquals(tc, 100, rtrees(r, 0));
    CuAssertIntEquals(tc, 100, rhorses(r));

    co.level++;
    co.force += 1.0;
    CuAssertIntEquals(tc, co.level, sp_drought(&co));
    CuAssertPtrEquals(tc, c, get_curse(r->attribs, &ct_drought));
    CuAssertIntEquals(tc, co.level, c->duration);
    CuAssertDblEquals(tc, co.force, c->vigour, 0.01);
    CuAssertDblEquals(tc, 4.0, c->effect, 0.01);
    CuAssertIntEquals(tc, 100, rtrees(r, 2));
    CuAssertIntEquals(tc, 100, rtrees(r, 1));
    CuAssertIntEquals(tc, 100, rtrees(r, 0));
    CuAssertIntEquals(tc, 100, rhorses(r));

    test_teardown();
}

static void test_stormwinds(CuTest *tc) {
    unit *u;
    ship *sh;
    region *r;
    curse *c;
    castorder co;
    spellparameter args;
    spllprm param;
    spllprm* params = &param;

    test_setup();
    u = test_create_unit(test_create_faction(), r = test_create_plain(0, 0));
    args.length = 1;
    args.param = &params;
    param.flag = TARGET_RESISTS;
    param.typ = SPP_SHIP;
    param.data.sh = sh = test_create_ship(r, NULL);
    test_create_castorder(&co, u, 2, 1.0, 0, &args);
    CuAssertIntEquals(tc, co.level, sp_stormwinds(&co));
    CuAssertPtrEquals(tc, NULL, get_curse(sh->attribs, &ct_stormwind));
    CuAssertPtrEquals(tc, NULL, test_find_region_message(r, "stormwinds_effect", u->faction));
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "stormwinds_reduced"));
    test_clear_messages(u->faction);

    param.flag = TARGET_NOTFOUND;
    CuAssertIntEquals(tc, 0, sp_stormwinds(&co));
    CuAssertPtrEquals(tc, NULL, get_curse(sh->attribs, &ct_stormwind));
    CuAssertPtrEquals(tc, NULL, test_find_region_message(r, "stormwinds_effect", u->faction));
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "stormwinds_reduced"));
    test_clear_messages(u->faction);

    param.flag = 0;
    CuAssertIntEquals(tc, co.level, sp_stormwinds(&co));
    CuAssertPtrNotNull(tc, c = get_curse(sh->attribs, &ct_stormwind));
    CuAssertIntEquals(tc, 1, c->duration);
    CuAssertDblEquals(tc, co.force, c->vigour, 0.01);
    CuAssertPtrNotNull(tc, test_find_region_message(r, "stormwinds_effect", u->faction));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(u->faction->msgs, "stormwinds_reduced"));
    test_clear_messages(u->faction);

    /* not twice: */
    CuAssertIntEquals(tc, 0, sp_stormwinds(&co));
    CuAssertPtrEquals(tc, c, get_curse(sh->attribs, &ct_stormwind));
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error_spell_on_ship_already"));
    test_clear_messages(u->faction);
    a_removeall(&sh->attribs, NULL);

    /* not on flying ships: */
    levitate_ship(sh, u, 1.0, 1);
    CuAssertIntEquals(tc, 0, sp_stormwinds(&co));
    CuAssertPtrEquals(tc, NULL, get_curse(sh->attribs, &ct_stormwind));
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error_spell_on_flying_ship"));

    test_teardown();
}

static void test_fumblecurse(CuTest *tc) {
    unit *u, *u2;
    curse *c;
    castorder co;
    spellparameter args;
    spllprm param;
    spllprm* params = &param;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    args.length = 1;
    args.param = &params;
    param.flag = TARGET_NOTFOUND;
    param.typ = SPP_UNIT;
    param.data.u = u2 = test_create_unit(test_create_faction(), u->region);
    set_level(u2, SK_MAGIC, 1);
    test_create_castorder(&co, u, 4, 5.0, 0, &args);
    CuAssertIntEquals(tc, 0, sp_fumblecurse(&co));

    param.flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_fumblecurse(&co));
    CuAssertPtrEquals(tc, NULL, get_curse(u2->attribs, &ct_fumble));

    param.flag = 0;
    CuAssertIntEquals(tc, co.level, sp_fumblecurse(&co));
    CuAssertPtrNotNull(tc, c = get_curse(u2->attribs, &ct_fumble));
    CuAssertIntEquals(tc, co.level, c->duration);
    CuAssertDblEquals(tc, co.force * .5, c->effect, 0.01);
    CuAssertDblEquals(tc, co.force, c->vigour, 0.01);
    CuAssertPtrNotNull(tc, test_find_messagetype(u2->faction->msgs, "fumblecurse"));

    test_teardown();
}

static void test_deathcloud(CuTest *tc) {
    unit *u;
    curse *c;
    region *r;
    castorder co;

    test_setup();
    u = test_create_unit(test_create_faction(), r = test_create_plain(0, 0));
    test_create_castorder(&co, u, 4, 5.0, 0, NULL);
    CuAssertIntEquals(tc, co.level, sp_deathcloud(&co));
    CuAssertPtrNotNull(tc, c = get_curse(r->attribs, &ct_deathcloud));
    CuAssertIntEquals(tc, co.level, c->duration);
    CuAssertDblEquals(tc, co.force * .5, c->effect, 0.01);
    CuAssertDblEquals(tc, co.force, c->vigour, 0.01);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "deathcloud_effect"));
    test_teardown();
}

static void test_magicboost(CuTest *tc) {
    unit *u;
    curse *c;
    castorder co;
    attrib *a;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    test_create_castorder(&co, u, 4, 5.0, 0, NULL);
    CuAssertIntEquals(tc, co.level, sp_magicboost(&co));
    CuAssertPtrNotNull(tc, c = get_curse(u->attribs, &ct_magicboost));
    CuAssertIntEquals(tc, 10, c->duration);
    CuAssertDblEquals(tc, 6.0, c->effect, 0.01);
    CuAssertDblEquals(tc, co.force, c->vigour, 0.01);

    CuAssertPtrNotNull(tc, a = a_find(u->attribs, &at_eventhandler));
    /*
    CuAssertPtrNotNull(tc, c = get_curse(u->attribs, &ct_auraboost));
    CuAssertIntEquals(tc, 4, c->duration);
    CuAssertDblEquals(tc, 200.0, c->effect, 0.01);
    CuAssertDblEquals(tc, co.force, c->vigour, 0.01);
    */
    test_teardown();
}

static void test_migrants(CuTest *tc) {
    unit *u, *u2;
    faction *f;
    castorder co;
    spellparameter args;
    spllprm param;
    spllprm *params = &param;

    test_setup();
    u = test_create_unit(f = test_create_faction(), test_create_plain(0, 0));
    args.length = 1;
    args.param = &params;
    param.flag = TARGET_RESISTS;
    param.typ = SPP_UNIT;
    param.data.u = u2 = test_create_unit(f = test_create_faction(), u->region);
    u2->orders = create_order(K_WORK, u2->faction->locale, NULL);
    u_setrace(u2, test_create_race("hobgoblin"));
    test_create_castorder(&co, u, 4, 5.0, 0, &args);
    CuAssertIntEquals(tc, u2->number, sp_migranten(&co));
    CuAssertPtrEquals(tc, f, u2->faction);

    param.flag = TARGET_NOTFOUND;
    CuAssertIntEquals(tc, 0, sp_migranten(&co));
    CuAssertPtrEquals(tc, f, u2->faction);

    /* no contact, no cost: */
    param.flag = 0;
    CuAssertIntEquals(tc, 0, sp_migranten(&co));
    CuAssertPtrEquals(tc, f, u2->faction);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "spellfail::contact"));
    test_clear_messages(u->faction);

    contact_unit(u2, u);
    CuAssertIntEquals(tc, u2->number, sp_migranten(&co));
    CuAssertPtrEquals(tc, u->faction, u2->faction);
    CuAssertPtrEquals(tc, NULL, u2->orders);
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
    SUITE_ADD_TEST(suite, test_speed2);
    SUITE_ADD_TEST(suite, test_goodwinds);
    SUITE_ADD_TEST(suite, test_change_race);
    SUITE_ADD_TEST(suite, test_summonent);
    SUITE_ADD_TEST(suite, test_maelstrom);
    SUITE_ADD_TEST(suite, test_blessedharvest);
    SUITE_ADD_TEST(suite, test_kaelteschutz);
    SUITE_ADD_TEST(suite, test_treewalkenter);
    SUITE_ADD_TEST(suite, test_treewalkexit);
    SUITE_ADD_TEST(suite, test_holyground);
    SUITE_ADD_TEST(suite, test_drought);
    SUITE_ADD_TEST(suite, test_stormwinds);
    SUITE_ADD_TEST(suite, test_fumblecurse);
    SUITE_ADD_TEST(suite, test_deathcloud);
    SUITE_ADD_TEST(suite, test_magicboost);
    SUITE_ADD_TEST(suite, test_migrants);

    return suite;
}
