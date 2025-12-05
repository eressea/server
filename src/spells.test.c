#include "spells.h"

#include "contact.h"
#include "magic.h"
#include "teleport.h"

#include <kernel/attrib.h>
#include <kernel/building.h>
#include <kernel/curse.h>
#include <kernel/config.h>
#include "kernel/connection.h"
#include "kernel/direction.h"
#include "kernel/event.h"
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include "kernel/ship.h"
#include "kernel/skill.h"
#include "kernel/spell.h"
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include "kernel/types.h"
#include <kernel/unit.h>

#include "util/keyword.h"
#include "util/language.h"
#include "util/message.h"
#include "util/rand.h"
#include "util/variant.h"  // for variant

#include <spells/borders.h>
#include <spells/regioncurse.h>
#include <spells/unitcurse.h>
#include <spells/shipcurse.h>
#include <spells/buildingcurse.h>
#include <spells/firewall.h>
#include <spells/flyingship.h>
#include <attributes/attributes.h>
#include <attributes/otherfaction.h>

#include <triggers/changerace.h>
#include <triggers/timeout.h>

#include <strings.h>
#include <tests.h>

#include <CuTest.h>
#include <stb_ds.h>

#include <stdbool.h>                 // for false
#include <stdio.h>

static void setup_terrains(CuTest *tc) {
    test_create_terrain("glacier", LAND_REGION | ARCTIC_REGION | WALK_INTO | FLY_INTO);
    test_create_terrain("ocean", SEA_REGION | SWIM_INTO | FLY_INTO);
    test_create_terrain("plain", LAND_REGION | FOREST_REGION | WALK_INTO | CAVALRY_REGION | FLY_INTO);
    test_create_terrain("swamp", LAND_REGION | WALK_INTO | FLY_INTO);
    test_create_terrain("desert", LAND_REGION | WALK_INTO | FLY_INTO);
    init_terrains();
    CuAssertPtrNotNull(tc, newterrain(T_GLACIER));
    CuAssertPtrNotNull(tc, newterrain(T_OCEAN));
    CuAssertPtrNotNull(tc, newterrain(T_PLAIN));
    CuAssertPtrNotNull(tc, newterrain(T_SWAMP));
    CuAssertPtrNotNull(tc, newterrain(T_DESERT));
}

static void test_good_dreams(CuTest *tc) {
    struct region *r;
    struct faction *f1, *f2;
    unit *u1, *u2;
    int level;
    castorder co;
    curse *curse;
    
    test_setup();
    r = test_create_plain(0, 0);
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

    /* change race, but do not add a second change_race timeout */
    CuAssertPtrEquals(tc, tr, change_race(u, 2, rcsmurf, NULL));
    CuAssertPtrEquals(tc, (void *)rcsmurf, (void *)u->_race);
    CuAssertPtrEquals(tc, NULL, (void *)u->irace);
    CuAssertPtrNotNull(tc, u->attribs);
    CuAssertPtrEquals(tc, NULL, u->attribs->next);
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

    test_teardown();
}

static void test_break_curse(CuTest *tc) {
    struct region *r;
    struct faction *f;
    unit *u1, *u2;
    castorder co;
    spellparameter param, *args = NULL;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    scale_number(u2, 30);

    param.flag = TARGET_RESISTS;
    param.typ = SPP_BUILDING;
    param.data.b = NULL;
    arrput(args, param);

    param.flag = TARGET_OK;
    param.typ = SPP_STRING;
    param.data.xs = NULL;
    arrput(args, param);

    test_create_castorder(&co, u1, 3, 4., 0, args);
    CuAssertIntEquals(tc, co.level, sp_break_curse(&co));
    CuAssertIntEquals(tc, 0, test_count_messagetype(f->msgs, NULL));
    test_teardown();
}

static void test_magicrunes(CuTest *tc) {
    struct region *r;
    struct faction *f;
    unit *u1, *u2;
    castorder co;
    spellparameter param, *args = NULL;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    scale_number(u2, 30);

    param.flag = TARGET_RESISTS;
    param.typ = SPP_BUILDING;
    param.data.b = NULL;
    arrput(args, param);

    test_create_castorder(&co, u1, 3, 4., 0, args);
    CuAssertIntEquals(tc, co.level, sp_magicrunes(&co));
    CuAssertIntEquals(tc, 0, test_count_messagetype(f->msgs, NULL));
    test_teardown();
}

static void test_speed2(CuTest *tc) {
    struct region *r;
    struct faction *f;
    unit *u1, *u2;
    castorder co;
    curse* c;
    spellparameter param, * args = NULL;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    scale_number(u2, 30);

    param.flag = TARGET_OK;
    param.typ = SPP_UNIT;
    param.data.u = u2;
    arrput(args, param);
    
    /* force 4, kann bis zu 32 Personen verzaubern */
    test_create_castorder(&co, u1, 3, 4., 0, args);
    CuAssertIntEquals(tc, co.level, sp_speed2(&co));
    CuAssertPtrNotNull(tc, c = get_curse(u2->attribs, &ct_speed));
    CuAssertDblEquals(tc, 2.0, c->effect, 0.01);
    CuAssertIntEquals(tc, (1 + co.level) / 2, c->duration);
    CuAssertIntEquals(tc, u2->number, c->data.i);
    CuAssertDblEquals(tc, co.force, c->vigour, 0.01);
    a_removeall(&u2->attribs, NULL);

    /* force 3, kann nur bis zu 18 Personen verzaubern */
    test_create_castorder(&co, u1, 1, 3., 0, args);
    CuAssertIntEquals(tc, co.level, sp_speed2(&co));
    CuAssertPtrNotNull(tc, c = get_curse(u2->attribs, &ct_speed));
    CuAssertDblEquals(tc, 2.0, c->effect, 0.01);
    CuAssertIntEquals(tc, (1 + co.level) / 2, c->duration);
    CuAssertIntEquals(tc, 18, c->data.i);
    CuAssertDblEquals(tc, co.force, c->vigour, 0.01);
    a_removeall(&u2->attribs, NULL);

    /* if target not found, no costs, no effect */
    co.a_params[0].flag = TARGET_NOTFOUND;
    CuAssertIntEquals(tc, 0, sp_speed2(&co));
    CuAssertPtrEquals(tc, NULL, u2->attribs);

    /* if target resists, pay in full, no effect */
    co.a_params[0].flag = TARGET_RESISTS;
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
    spellparameter param, * args = NULL;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    sh = test_create_ship(r, NULL);

    param.flag = TARGET_OK;
    param.typ = SPP_SHIP;
    param.data.sh = sh;
    arrput(args, param);

    test_create_castorder(&co, u, 3, 4., 0, args);
    CuAssertIntEquals(tc, co.level, sp_goodwinds(&co));
    CuAssertPtrNotNull(tc, c = get_curse(sh->attribs, &ct_nodrift));
    CuAssertIntEquals(tc, 1 + co.level, c->duration);
    CuAssertDblEquals(tc, co.force, c->vigour, 0.01);
    a_removeall(&sh->attribs, NULL);

    /* if target not found, no costs, no effect */
    co.a_params[0].flag = TARGET_NOTFOUND;
    CuAssertIntEquals(tc, 0, sp_goodwinds(&co));
    CuAssertPtrEquals(tc, NULL, sh->attribs);

    /* if target resists, pay in full, no effect */
    co.a_params[0].flag = TARGET_RESISTS;
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
    r = test_create_plain(0, 0);
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
    mt_create_va(mt_new("summon_effect", NULL),
        "mage:unit", "amount:int", "race:race", MT_NEW_END);
    mt_create_va(mt_new("summonshadow_effect", NULL),
        "mage:unit", "number:int", MT_NEW_END);
    rx = test_create_region(0, TP_RADIUS + 1, NULL);
    f = test_create_faction();
    u = test_create_unit(f, rx);

    /* can only cast in astral space */
    test_create_castorder(&co, u, 10, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_viewreality(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "spell_astral_only"));

    test_clear_messages(f);
    ra = test_create_region(real2tp(0), real2tp(0), NULL);
    ra->_plane = get_astralplane();
    move_unit(u, ra, NULL);

    /* there is no connection from ra to rx */
    test_create_castorder(&co, u, 10, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_viewreality(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "error216"));
    CuAssertIntEquals(tc, -1, get_observer(rx, f));

    test_clear_messages(f);
    r = test_create_plain(0, 0);

    test_clear_messages(f);

    /* units exist, r can be seen, but rx is out of range */
    test_create_castorder(&co, u, 9, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 9, sp_viewreality(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "viewreality_effect"));
    CuAssertIntEquals(tc, 5, get_observer(r, f));
    CuAssertIntEquals(tc, -1, get_observer(rx, f));

    set_observer(r, f, -1, 0);
    CuAssertIntEquals(tc, -1, get_observer(r, f));

    /* target region r exists, but astral space is blocked */
    c = create_curse(u, &ra->attribs, &ct_astralblock, 50.0, 1, 50, 0);
    test_create_castorder(&co, u, 10, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_viewreality(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "error216"));
    CuAssertIntEquals(tc, -1, get_observer(r, f));
    remove_curse(&ra->attribs, c);

    /* target region r exists, but astral interference is blocked */
    c = create_curse(u, &r->attribs, &ct_astralblock, 50.0, 1, 50, 0);
    test_create_castorder(&co, u, 10, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_viewreality(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "error216"));
    CuAssertIntEquals(tc, -1, get_observer(r, f));

    test_teardown();
}

static void test_disruptastral(CuTest *tc) {
    unit *u;
    faction *f;
    region *r, *ra, *rx;
    castorder co;

    test_setup();
    test_use_astral();
    config_set_int("astralspace.auto", 0);
    mt_create_error(216);

    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);

    /* need a matching astral region: */
    test_create_castorder(&co, u, 9, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_disruptastral(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "error216"));
    CuAssertPtrEquals(tc, NULL, r->attribs);

    /* create directly targeted origin astralspace region */
    ra = test_create_region(real2tp(0), real2tp(0), NULL);
    /* region at range 2 from origin, needs force >= 10 */
    rx = test_create_region(real2tp(0) + 2, real2tp(0), NULL);

    /* casts a curse on all astral regions in range: */
    CuAssertPtrEquals(tc, get_astralplane(), ra->_plane);
    test_clear_messages(f);
    CuAssertIntEquals(tc, co.level, sp_disruptastral(&co));
    CuAssertPtrEquals(tc, NULL, r->attribs);
    CuAssertTrue(tc, is_cursed(ra->attribs, &ct_astralblock));
    a_removeall(&r->attribs, NULL);
    CuAssertTrue(tc, is_cursed(rx->attribs, &ct_astralblock));
    a_removeall(&rx->attribs, NULL);

    /* force 9 only has a range of 1, rx is unchanged */
    co.force = 9.0;
    test_clear_messages(f);
    CuAssertIntEquals(tc, co.level, sp_disruptastral(&co));
    CuAssertPtrEquals(tc, NULL, rx->attribs);
    CuAssertTrue(tc, is_cursed(ra->attribs, &ct_astralblock));
    a_removeall(&r->attribs, NULL);

    /* can also be cast from astral space: */
    move_unit(u, ra, NULL);
    test_clear_messages(f);
    CuAssertIntEquals(tc, co.level, sp_disruptastral(&co));
    CuAssertPtrEquals(tc, NULL, r->attribs);
    CuAssertTrue(tc, is_cursed(ra->attribs, &ct_astralblock));

    test_teardown();
}

static void test_eternizewall(CuTest *tc) {
    unit *u, *u2, *u3;
    faction *f;
    region *r;
    building *b;
    curse *c;
    castorder co;
    message *m;
    spellparameter param, *args = NULL;

    test_setup();
    mt_create_error(206);
    mt_create_va(mt_new("eternizewall_effect", NULL),
        "mage:unit", "region:region", "building:building", MT_NEW_END);
    u = test_create_unit(f = test_create_faction(), r = test_create_plain(0, 0));
    u2 = test_create_unit(test_create_faction(), r);
    u3 = test_create_unit(test_create_faction(), r);
    param.flag = TARGET_OK;
    param.typ = SPP_BUILDING;
    param.data.b = b = test_create_building(r, test_create_castle());
    b->size = 24;
    u2->building = b;
    arrput(args, param);
    test_create_castorder(&co, u, 5, 12.0, 0, args);

    /* target not found, no cost */
    co.a_params[0].flag = TARGET_NOTFOUND;
    CuAssertIntEquals(tc, 0, sp_eternizewall(&co));
    CuAssertPtrEquals(tc, NULL, b->attribs);

    /* target resists */
    co.a_params[0].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, 0, sp_eternizewall(&co));
    CuAssertPtrEquals(tc, NULL, b->attribs);

    co.a_params[0].flag = TARGET_OK;
    CuAssertIntEquals(tc, co.level, sp_eternizewall(&co));
    CuAssertPtrNotNull(tc, c = get_curse(b->attribs, &ct_nocostbuilding));
    CuAssertDblEquals(tc, co.force * co.force, c->vigour, 0.01);
    CuAssertIntEquals(tc, 1, c->duration);
    CuAssertDblEquals(tc, 0.0, c->effect, 0.01);
    /* mage and factions with units in the building get a message: */
    CuAssertPtrNotNull(tc, m = test_find_region_message(r, "eternizewall_effect", f));
    CuAssertPtrEquals(tc, u, m->parameters[0].v);
    CuAssertPtrEquals(tc, r, m->parameters[1].v);
    CuAssertPtrEquals(tc, b, m->parameters[2].v);
    CuAssertPtrNotNull(tc, test_find_region_message(r, "eternizewall_effect", u2->faction));
    CuAssertPtrEquals(tc, NULL, test_find_region_message(r, "eternizewall_effect", u3->faction));
    test_clear_region_messages(r);

    /* target is already cursed: */
    CuAssertIntEquals(tc, 0, sp_eternizewall(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "error206"));
    CuAssertPtrNotNull(tc, c = get_curse(b->attribs, &ct_nocostbuilding));
    CuAssertDblEquals(tc, co.force * co.force, c->vigour, 0.01);
    CuAssertIntEquals(tc, 1, c->duration);
    CuAssertDblEquals(tc, 0.0, c->effect, 0.01);
    CuAssertPtrEquals(tc, NULL, test_find_region_message(r, "eternizewall_effect", f));

    a_removeall(&b->attribs, NULL);

    test_teardown();
}

static void test_permtransfer(CuTest *tc) {
    unit *u, *u2;
    region *r;
    faction *f;
    castorder co;
    spellparameter param, *args = NULL;
    spell *sp;
    message *m;
    int maxaura;

    test_setup();
    mt_create_error(214);
    mt_create_va(mt_new("permtransfer_effect", NULL),
        "mage:unit", "target:unit", "amount:int", MT_NEW_END);
    init_resources();
    u = test_create_unit(f = test_create_faction(), r = test_create_plain(0, 0));
    set_level(u, SK_MAGIC, 10);
    mage_set_spellpoints(create_mage(u, M_TYBIED), 100);
    maxaura = max_spellpoints(u, r);

    param.flag = TARGET_OK;
    param.typ = SPP_UNIT;
    param.data.u = u2 = test_create_unit(f, r);
    arrput(args, param);
    param.typ = SPP_INT;
    param.data.i = 12;
    arrput(args, param);

    test_create_castorder(&co, u, 5, 12.0, 0, args);
    co.sp = sp = test_create_spell();
    CuAssertIntEquals(tc, 1, auracost(u, sp));
    co.a_params[0].flag = TARGET_NOTFOUND;
    CuAssertIntEquals(tc, 0, sp_permtransfer(&co));
    co.a_params[0].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_permtransfer(&co));

    co.a_params[0].flag = TARGET_OK;
    /* still does nothing if target is not a mage: */
    CuAssertIntEquals(tc, 0, sp_permtransfer(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "error214"));
    test_clear_messages(f);

    set_level(u2, SK_MAGIC, 10);
    /* target is also a Tybied mage, gets half the aura: */
    mage_set_spellpoints(create_mage(u2, M_TYBIED), 100);
    CuAssertIntEquals(tc, co.level, sp_permtransfer(&co));
    CuAssertIntEquals(tc, 100 - 12, mage_get_spellpoints(get_mage(u)));
    CuAssertIntEquals(tc, maxaura - 12, max_spellpoints(u, r));
    CuAssertIntEquals(tc, maxaura + 6, max_spellpoints(u2, r));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "permtransfer_effect"));
    CuAssertPtrNotNull(tc, m = test_find_faction_message(u2->faction, "permtransfer_effect"));
    CuAssertPtrEquals(tc, u, m->parameters[0].v);
    CuAssertPtrEquals(tc, u2, m->parameters[1].v);
    CuAssertIntEquals(tc, 6, m->parameters[2].i);

    test_teardown();
}

static void test_movecastle(CuTest *tc) {
    unit *u;
    region *r, *r2;
    building *b;
    faction *f;
    castorder co;
    spellparameter param, *args = NULL;
    struct locale *lang;

    test_setup();
    random_source_inject_constants(0.0, 0);
    lang = test_create_locale();
    locale_setstring(lang, "dir_e", "E");
    init_directions(lang);
    u = test_create_unit(f = test_create_faction_ex(NULL, lang), r = test_create_plain(0, 0));
    r2 = test_create_plain(1, 0);

    param.flag = TARGET_OK;
    param.typ = SPP_BUILDING;
    param.data.b = b = test_create_building(r, test_create_castle());
    b->size = 24;
    arrput(args, param);
    param.typ = SPP_STRING;
    param.data.xs = str_strdup("E");
    arrput(args, param);
    test_create_castorder(&co, u, 5, 12.0, 0, args);

    /* target not found, no cost */
    co.a_params[0].flag = TARGET_NOTFOUND;
    CuAssertIntEquals(tc, 0, sp_movecastle(&co));
    CuAssertPtrEquals(tc, r, b->region);

    /* target resists */
    co.a_params[0].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_movecastle(&co));
    CuAssertPtrEquals(tc, r, b->region);

    /* target is okay, but not enough force was used */
    co.a_params[0].flag = TARGET_OK;
    CuAssertIntEquals(tc, co.level, sp_movecastle(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "sp_movecastle_fail_0"));
    CuAssertPtrEquals(tc, r, b->region);
    test_clear_messages(f);

    /* target is okay, and force is big enough */
    co.force = 12.1;
    CuAssertIntEquals(tc, co.level, sp_movecastle(&co));
    CuAssertPtrEquals(tc, r2, b->region);
    CuAssertPtrEquals(tc, NULL, r->buildings);
    CuAssertIntEquals(tc, 22, b->size);
    CuAssertPtrNotNull(tc, test_find_region_message(r, "movecastle_effect", NULL));

    /** TODO: invalid target regions (ocean, firewall) */
    /** TODO: special buildings (dam, caravan, tunnel) destroy roads */
    /** TODO: Insassen reisen mit */

    test_teardown();
}

static void test_auraleak(CuTest *tc) {
    unit *u, *u2;
    region *r;
    faction *f;
    castorder co;

    test_setup();
    u = test_create_unit(f = test_create_faction(), r = test_create_plain(0, 0));
    u2 = test_create_unit(f = test_create_faction(), r);

    mage_set_spellpoints(create_mage(u, M_DRAIG), 100);
    mage_set_spellpoints(create_mage(u2, M_GWYRRD), 100);
    test_create_castorder(&co, u, 5, 10.0, 0, NULL);
    CuAssertIntEquals(tc, co.level, sp_auraleak(&co));
    CuAssertIntEquals(tc, 50, mage_get_spellpoints(get_mage(u)));
    CuAssertIntEquals(tc, 50, mage_get_spellpoints(get_mage(u2)));
    CuAssertPtrNotNull(tc, test_find_region_message(r, "cast_auraleak_effect", NULL));

    mage_set_spellpoints(create_mage(u, M_DRAIG), 100);
    mage_set_spellpoints(create_mage(u2, M_GWYRRD), 100);
    co.force = 20.0;
    CuAssertIntEquals(tc, co.level, sp_auraleak(&co));
    CuAssertIntEquals(tc, 5, mage_get_spellpoints(get_mage(u)));
    CuAssertIntEquals(tc, 5, mage_get_spellpoints(get_mage(u2)));

    test_teardown();
}

static void test_leaveastral(CuTest *tc) {
    region *r, *ra;
    faction *f;
    unit *u, *u2;
    castorder co;
    spellparameter param, *args = NULL;

    test_setup();
    test_use_astral();
    mt_create_error(216);
    mt_create_error(231);
    r = test_create_plain(0, 0);
    ra = test_create_region(real2tp(0), real2tp(0), NULL);
    ra->_plane = get_astralplane();
    CuAssertPtrEquals(tc, ra, r_standard_to_astral(r));
    f = test_create_faction();
    u = test_create_unit(f, ra);
    u2 = test_create_unit(f, ra);

    param.flag = TARGET_RESISTS;
    param.typ = SPP_REGION;
    param.data.r = r;
    arrput(args, param);
    param.flag = TARGET_OK;
    param.typ = SPP_UNIT;
    param.data.u = u2;
    arrput(args, param);
    test_create_castorder(&co, u, 10, 11., 0, args);
    CuAssertIntEquals(tc, co.level, sp_leaveastral(&co));
    CuAssertPtrEquals(tc, ra, u2->region);
    CuAssertPtrEquals(tc, NULL, f->msgs);

    args[0].flag = TARGET_OK;
    args[1].flag = TARGET_RESISTS;

    CuAssertIntEquals(tc, co.level, sp_leaveastral(&co));
    CuAssertPtrEquals(tc, ra, u2->region);
    CuAssertPtrEquals(tc, NULL, f->msgs);

    // Success:
    args[1].flag = TARGET_OK;
    CuAssertIntEquals(tc, co.level, sp_leaveastral(&co));
    CuAssertPtrEquals(tc, r, u2->region);
    CuAssertPtrEquals(tc, NULL, f->msgs);
    CuAssertPtrNotNull(tc, test_find_region_message(ra, "astral_disappear", u->faction));
    CuAssertPtrNotNull(tc, test_find_region_message(r, "astral_appear", u->faction));


    test_use_astral();
}

static void test_show_astral(CuTest *tc) {
    region *r, *ra, *rx;
    faction *f;
    unit *u;
    castorder co;
    curse * c;

    test_setup();
    // create r before creating astral plane, so astral region won't get auto-created.
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
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "spell_astral_forbidden"));

    test_clear_messages(f);
    move_unit(u, r, NULL);

    /* error: no target region */
    test_create_castorder(&co, u, 9, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_showastral(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "error216"));
    CuAssertIntEquals(tc, -1, get_observer(ra, f));

    rx = test_create_region(real2tp(r->x), real2tp(r->y), NULL);
    rx->_plane = ra->_plane;

    /* rx is in range, but empty */
    test_create_castorder(&co, u, 9, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_showastral(&co));
    CuAssertIntEquals(tc, -1, get_observer(rx, f));
    CuAssertIntEquals(tc, -1, get_observer(ra, f));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "error220"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(f, "showastral_effect"));

    test_create_unit(f, ra);
    test_create_unit(f, rx);
    /* rx is in range, but ra is not */
    test_create_castorder(&co, u, 9, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 9, sp_showastral(&co));
    CuAssertIntEquals(tc, 5, get_observer(rx, f));
    CuAssertIntEquals(tc, -1, get_observer(ra, f));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "showastral_effect"));

    /* astral block on r */
    c = create_curse(u, &r->attribs, &ct_astralblock, 50.0, 1, 50, 0);
    test_create_castorder(&co, u, 9, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_showastral(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "error216"));
    remove_curse(&r->attribs, c);

    /* astral block on rx */
    c = create_curse(u, &rx->attribs, &ct_astralblock, 50.0, 1, 50, 0);
    test_create_castorder(&co, u, 9, 10.0, 0, NULL);
    CuAssertIntEquals(tc, 0, sp_showastral(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "error220"));
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
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "error204"));
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

static void test_summonundead(CuTest *tc) {
    unit *u, *us;
    region *r;
    castorder co;
    struct race *rc;
    skill_t sk;
    int level = 8; /* <=8: Skeletons */

    test_setup();
    rc = test_create_race(racenames[RC_SKELETON]);
    deathcounts(r = test_create_plain(0, 0), 1000);
    u = test_create_unit(test_create_faction(), r);
    test_create_castorder(&co, u, level, (float)level + 1.0f, 0, NULL);

    CuAssertIntEquals(tc, 8, sp_summonundead(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "summonundead_effect_1"));
    CuAssertPtrNotNull(tc, test_find_region_message(r, "summonundead_effect_2", NULL));
    CuAssertPtrNotNull(tc, us = u->next);
    CuAssertPtrEquals(tc, u->faction, us->faction);
    CuAssertPtrEquals(tc, rc, (race *)u_race(us));
    for (sk = 0; sk != MAXSKILLS; ++sk) {
        if (rc->bonus[sk] > 0) {
            CuAssertIntEquals(tc, level, get_level(us, sk));
        }
    }
    if (rc->bonus[SK_STAMINA]) {
        CuAssertIntEquals(tc, unit_max_hp(us) * us->number, us->hp);
    }
    test_clear_messages(u->faction);

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
    spellparameter param, * args = NULL;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    param.flag = TARGET_RESISTS;
    param.typ = SPP_UNIT;
    param.data.u = u2 = test_create_unit(test_create_faction(), u->region);
    arrput(args, param);
    test_create_castorder(&co, u, 4, 5.0, 0, args);
    CuAssertIntEquals(tc, co.level, sp_kaelteschutz(&co));
    test_teardown();
}

static void test_treewalkenter(CuTest *tc) {
    unit *u, *u2;
    region *r, *ra;
    castorder co;
    spellparameter param, * args = NULL;

    test_setup();
    test_use_astral();
    u = test_create_unit(test_create_faction(), r = test_create_plain(0, 0));
    rsettrees(r, 2, r->terrain->size / 2);
    ra = test_create_region(real2tp(0), real2tp(0), NULL);
    ra->_plane = get_astralplane();
    param.flag = TARGET_NOTFOUND;
    param.typ = SPP_UNIT;
    param.data.u = u2 = test_create_unit(test_create_faction(), u->region);
    arrput(args, param);
    test_create_castorder(&co, u, 4, 5.0, 0, args);

    /* spell fails because target is not found */
    CuAssertIntEquals(tc, 0, sp_treewalkenter(&co));

    /* spell fails because target units resist it, but is paid in full */
    co.a_params[0].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_treewalkenter(&co));
    CuAssertPtrEquals(tc, r, u2->region);

    co.a_params[0].flag = TARGET_OK;
    CuAssertIntEquals(tc, 0, sp_treewalkenter(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "feedback_no_contact"));
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
    race *rc;
    castorder co;
    spellparameter param, * args = NULL;
    const struct item_type *it_silver;
    curse *c;

    test_setup();
    rc = test_create_race("undead");
    rc->flags |= RCF_UNDEAD;
    init_resources();
    it_silver = get_resourcetype(R_SILVER)->itype;
    test_use_astral();
    r = test_create_plain(0, 0);
    ra = test_create_region(real2tp(0), real2tp(0), NULL);
    ra->_plane = get_astralplane();
    u = test_create_unit(test_create_faction(), ra);
    param.flag = TARGET_OK;
    param.typ = SPP_REGION;
    param.data.r = r;
    arrput(args, param);
    param.flag = TARGET_OK;
    param.typ = SPP_UNIT;
    param.data.u = u2 = test_create_unit(test_create_faction_ex(rc, NULL), u->region);
    arrput(args, param);
    test_create_castorder(&co, u, 4, 5.0, 0, args);

    /* target region is not a forest */
    rsettrees(r, 2, 0);
    CuAssertTrue(tc, !r_isforest(r));
    CuAssertIntEquals(tc, 0, sp_treewalkexit(&co));
    CuAssertPtrEquals(tc, ra, u2->region);
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "error196"));
    test_clear_messages(u->faction);
    rsettrees(r, 2, r->terrain->size / 2);
    CuAssertTrue(tc, r_isforest(r));

    /* spell fails because the region resisted it: */
    co.a_params[0].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_treewalkexit(&co));
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);

    /* spell fails because the region was not found: */
    co.a_params[0].flag = TARGET_NOTFOUND;
    CuAssertIntEquals(tc, 0, sp_treewalkexit(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "error195"));
    test_clear_messages(u->faction);
    co.a_params[0].flag = TARGET_OK;

    /* spell fails because target unit was not found*/
    co.a_params[1].flag = TARGET_NOTFOUND;
    CuAssertIntEquals(tc, 0, sp_treewalkexit(&co));
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);

    /* spell fails because target units resist it, but is paid in full */
    co.a_params[1].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_treewalkexit(&co));
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);

    /* no contact */
    co.a_params[1].flag = TARGET_OK;
    CuAssertIntEquals(tc, 0, sp_treewalkexit(&co));
    CuAssertPtrEquals(tc, ra, u2->region);
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "feedback_no_contact"));
    test_clear_messages(u->faction);

    contact_unit(u2, u);
    /* target unit is too heavy */
    /* at level 5, weight is capped at 2500: */
    i_change(&u2->items, it_silver, 1501);
    CuAssertIntEquals(tc, 2501, weight(u2));
    CuAssertIntEquals(tc, 0, sp_treewalkexit(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "fail_tooheavy"));
    test_clear_messages(u->faction);
    i_change(&u2->items, it_silver, -1);
    CuAssertIntEquals(tc, 2500, weight(u2));

    /* target region is inhospitable */
    c = create_curse(u, &r->attribs, &ct_holyground, 1.0, 1, 1.0, 0);
    CuAssertTrue(tc, !can_survive(u2, r));
    CuAssertIntEquals(tc, 0, sp_treewalkexit(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "error231"));
    test_clear_messages(u->faction);
    remove_curse(&r->attribs, c);
    CuAssertTrue(tc, can_survive(u2, r));

    /* success */
    CuAssertIntEquals(tc, co.level, sp_treewalkexit(&co));
    CuAssertPtrEquals(tc, r, u2->region);
    CuAssertPtrEquals(tc, NULL, u->faction->msgs);
    CuAssertPtrEquals(tc, NULL, u->region->msgs);
    CuAssertPtrNotNull(tc, u->region->individual_messages);
    CuAssertPtrNotNull(tc, test_find_messagetype(r->individual_messages->msgs, "astral_appear"));
    CuAssertPtrNotNull(tc, test_find_messagetype(ra->individual_messages->msgs, "astral_disappear"));

    test_teardown();
}

static void test_pullastral(CuTest *tc) {
    unit *u, *u2;
    region *r, *ra;
    castorder co;
    spellparameter param, * args = NULL;

    test_setup();
    test_use_astral();
    r = test_create_plain(0, 0);
    ra = test_create_region(real2tp(0), real2tp(0), NULL);

    CuAssertPtrEquals(tc, r_astral_to_standard(ra), r);

    u = test_create_unit(test_create_faction(), ra);
    ra->_plane = get_astralplane();

    param.flag = TARGET_RESISTS;
    param.typ = SPP_REGION;
    param.data.r = r;
    arrput(args, param);

    param.flag = TARGET_NOTFOUND;
    param.typ = SPP_UNIT;
    param.data.u = u2 = test_create_unit(test_create_faction(), r);
    test_create_unit(u2->faction, r);
    arrput(args, param);
    test_create_castorder(&co, u, 4, 5.0, 0, args);

    /* spell fails because target region resists */
    CuAssertIntEquals(tc, co.level, sp_pullastral(&co));

    /* spell fails because target unit not found */
    co.a_params[0].flag = TARGET_OK;
    CuAssertIntEquals(tc, 0, sp_pullastral(&co));

    co.a_params[1].flag = TARGET_RESISTS;
    /* spell fails because target units resist it, but is paid in full */
    test_clear_messages(u->faction);

    CuAssertIntEquals(tc, co.level, sp_pullastral(&co));

    CuAssertPtrEquals(tc, r, u2->region);

    co.level = 13;
    co.force = 13.0;
    /* spell level high enough, but target unit resists */
    test_clear_messages(u->faction);

    CuAssertIntEquals(tc, co.level, sp_pullastral(&co));

    CuAssertPtrEquals(tc, r, u2->region);
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "feedback_no_contact_resist"));

    /* spell succeeds because target can't resist */
    co.a_params[1].flag = TARGET_OK;
    test_clear_messages(u->faction);
    test_clear_messages(u2->faction);

    CuAssertIntEquals(tc, co.level, sp_pullastral(&co));

    CuAssertPtrEquals(tc, ra, u2->region);
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "feedback_no_contact_no_resist"));

    /* spell succeeds */
    move_unit(u2, r, NULL);
    test_clear_messages(u->faction);
    test_clear_messages(u2->faction);
    co.level = 4;
    co.force = 4.0;
    contact_unit(u2, u);

    CuAssertIntEquals(tc, co.level, sp_pullastral(&co));

    CuAssertPtrEquals(tc, ra, u2->region);
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "feedback_no_contact_no_resist"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "feedback_no_contact_resist"));
    CuAssertPtrNotNull(tc, test_find_region_message(r, "astral_disappear", u2->faction));
    CuAssertPtrNotNull(tc, test_find_region_message(ra, "astral_appear", u2->faction));
    CuAssertPtrNotNull(tc, test_find_region_message(ra, "astral_appear", u->faction));


    /* other visible region */
    test_clear_messages(u->faction);
    test_clear_messages(u2->faction);
    co.a_params[1].data.u = u2 = test_create_unit(u2->faction, r =test_create_plain(3,0));
    test_create_unit(u2->faction, r);
    co.a_params[0].data.r = r;
    contact_unit(u2, u);

    CuAssertIntEquals(tc, co.level, sp_pullastral(&co));

    CuAssertPtrEquals(tc, ra, u2->region);
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "feedback_no_contact_no_resist"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "feedback_no_contact_resist"));
    CuAssertPtrNotNull(tc, test_find_region_message(r, "astral_disappear", u2->faction));
    CuAssertPtrNotNull(tc, test_find_region_message(ra, "astral_appear", u2->faction));
    CuAssertPtrNotNull(tc, test_find_region_message(ra, "astral_appear", u->faction));


    /* region out of range */
    test_clear_messages(u->faction);
    test_clear_messages(u2->faction);
    co.a_params[1].data.u = u2 = test_create_unit(u2->faction, r = test_create_plain(4,0));
    test_create_unit(u2->faction, r);
    co.a_params[0].data.r = r;
    contact_unit(u2, u);

    CuAssertIntEquals(tc, 0, sp_pullastral(&co));

    CuAssertPtrEquals(tc, r, u2->region);
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "spellfail_distance"));

    /* region out of range */
    test_clear_messages(u->faction);
    test_clear_messages(u2->faction);
    co.a_params[1].data.u = u2 = test_create_unit(u2->faction, r = test_create_plain(-1,0));
    test_create_unit(u2->faction, r);
    co.a_params[0].data.r = r;
    contact_unit(u2, u);

    CuAssertIntEquals(tc, 0, sp_pullastral(&co));

    CuAssertPtrEquals(tc, r, u2->region);
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "spellfail_distance"));

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
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "drought_effect"));
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

static void test_great_drought_no_terraform(CuTest *tc) {
    unit *u;
    faction *f;
    castorder co;
    curse *c;
    region *r;
    int road_size;

    test_setup();
    setup_terrains(tc);
    u = test_create_unit(test_create_faction(), r = test_create_region(0, 0, newterrain(T_GLACIER)));
    test_create_unit(f = test_create_faction(), r); /* observer unit+faction */
    r->buildings = test_create_building(r, NULL);

    rsettrees(r, 2, 200);
    rsettrees(r, 1, 200);
    rsettrees(r, 0, 200);
    rsethorses(r, 200);

    /* build a road to the east */
    test_create_plain(1, 0);
    road_size = r->terrain->max_road;
    rsetroad(r, D_EAST, road_size);
    CuAssertIntEquals(tc, road_size, rroad(r, D_EAST));

    test_create_castorder(&co, u, 4, 5.0, 0, NULL);

    /* [50, 200) = no terraforming  */
    random_source_inject_constants(0.0, 50);
    CuAssertIntEquals(tc, co.level, sp_great_drought(&co));
    CuAssertIntEquals(tc, road_size, rroad(r, D_EAST));
    CuAssertPtrEquals(tc, (void *)newterrain(T_GLACIER), (void *)r->terrain);
    CuAssertPtrNotNull(tc, test_find_region_message(r, "drought_effect", NULL));
    /* Parteimeldung nur beim Magier (und nur Fernzauber): */
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(f, "drought_effect"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "drought_effect"));
    CuAssertPtrNotNull(tc, c = get_curse(r->attribs, &ct_drought));
    CuAssertIntEquals(tc, 2, c->duration);
    CuAssertDblEquals(tc, co.force, c->vigour, 0.01);
    CuAssertDblEquals(tc, 4.0, c->effect, 0.01);
    CuAssertIntEquals(tc, 100, rtrees(r, 2));
    CuAssertIntEquals(tc, 100, rtrees(r, 1));
    CuAssertIntEquals(tc, 100, rtrees(r, 0));
    CuAssertIntEquals(tc, 100, rhorses(r));
    CuAssertPtrNotNull(tc, r->buildings);

    test_teardown();
}

static void test_great_drought_to_swamp(CuTest *tc) {
    unit *u;
    faction *f;
    castorder co;
    region *r;

    test_setup();
    setup_terrains(tc);
    u = test_create_unit(test_create_faction(), r = test_create_region(0, 0, newterrain(T_GLACIER)));
    test_create_unit(f = test_create_faction(), r); /* observer unit+faction */
    r->buildings = test_create_building(r, NULL);

    test_create_castorder(&co, u, 4, 5.0, 0, NULL);

    /* [0, 25) = terraforming success, to swamp */
    random_source_inject_constants(0.0, 24);
    CuAssertIntEquals(tc, co.level, sp_great_drought(&co));
    CuAssertIntEquals(tc, 0, rroad(r, D_EAST));
    CuAssertPtrEquals(tc, (void *)newterrain(T_SWAMP), (void *)r->terrain);
    CuAssertPtrNotNull(tc, test_find_region_message(r, "drought_effect", NULL));
    CuAssertPtrNotNull(tc, test_find_region_message(r, "drought_glacier_to_swamp", NULL));
    CuAssertPtrNotNull(tc, r->buildings);

    test_teardown();
}

static void test_great_drought_to_ocean(CuTest *tc) {
    unit *u, *u2;
    faction *f;
    castorder co;
    region *r;

    test_setup();
    setup_terrains(tc);
    u = test_create_unit(test_create_faction(), r = test_create_region(0, 0, newterrain(T_GLACIER)));
    u->ship = test_create_ship(r, NULL);
    r->buildings = test_create_building(r, NULL);
    u2 = test_create_unit(f = test_create_faction(), r); /* observer unit */

    test_create_castorder(&co, u, 4, 5.0, 0, NULL);

    /* [25, 50) = terraforming success, to ocean */
    random_source_inject_constants(0.0, 25);
    CuAssertIntEquals(tc, co.level, sp_great_drought(&co));
    CuAssertPtrEquals(tc, (void *)newterrain(T_OCEAN), (void *)r->terrain);
    CuAssertPtrNotNull(tc, test_find_region_message(r, "drought_effect", NULL));
    CuAssertPtrEquals(tc, NULL, test_find_region_message(r, "drought_glacier_to_ocean", NULL));
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "drought_to_ocean_global"));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "drought_to_ocean_global"));
    CuAssertIntEquals(tc, 0, u2->number);
    CuAssertIntEquals(tc, 1, u->number);
    CuAssertPtrEquals(tc, NULL, r->buildings);
    CuAssertPtrNotNull(tc, r->ships);

    test_teardown();
}

static void test_stormwinds(CuTest *tc) {
    unit *u;
    ship *sh;
    region *r;
    curse *c;
    castorder co;
    spellparameter param, * args = NULL;

    test_setup();
    u = test_create_unit(test_create_faction(), r = test_create_plain(0, 0));
    param.flag = TARGET_RESISTS;
    param.typ = SPP_SHIP;
    param.data.sh = sh = test_create_ship(r, NULL);
    arrput(args, param);
    test_create_castorder(&co, u, 2, 1.0, 0, args);
    CuAssertIntEquals(tc, co.level, sp_stormwinds(&co));
    CuAssertPtrEquals(tc, NULL, get_curse(sh->attribs, &ct_stormwind));
    CuAssertPtrEquals(tc, NULL, test_find_region_message(r, "stormwinds_effect", u->faction));
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "stormwinds_reduced"));
    test_clear_messages(u->faction);

    co.a_params[0].flag = TARGET_NOTFOUND;
    CuAssertIntEquals(tc, 0, sp_stormwinds(&co));
    CuAssertPtrEquals(tc, NULL, get_curse(sh->attribs, &ct_stormwind));
    CuAssertPtrEquals(tc, NULL, test_find_region_message(r, "stormwinds_effect", u->faction));
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "stormwinds_reduced"));
    test_clear_messages(u->faction);

    co.a_params[0].flag = TARGET_OK;
    CuAssertIntEquals(tc, co.level, sp_stormwinds(&co));
    CuAssertPtrNotNull(tc, c = get_curse(sh->attribs, &ct_stormwind));
    CuAssertIntEquals(tc, 1, c->duration);
    CuAssertDblEquals(tc, co.force, c->vigour, 0.01);
    CuAssertPtrNotNull(tc, test_find_region_message(r, "stormwinds_effect", u->faction));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "stormwinds_reduced"));
    test_clear_messages(u->faction);

    /* not twice: */
    CuAssertIntEquals(tc, 0, sp_stormwinds(&co));
    CuAssertPtrEquals(tc, c, get_curse(sh->attribs, &ct_stormwind));
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "error_spell_on_ship_already"));
    test_clear_messages(u->faction);
    a_removeall(&sh->attribs, NULL);

    /* not on flying ships: */
    levitate_ship(sh, u, 1.0, 1);
    CuAssertIntEquals(tc, 0, sp_stormwinds(&co));
    CuAssertPtrEquals(tc, NULL, get_curse(sh->attribs, &ct_stormwind));
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "error_spell_on_flying_ship"));

    test_teardown();
}

static void test_fumblecurse(CuTest *tc) {
    unit *u, *u2;
    curse *c;
    castorder co;
    spellparameter param, * args = NULL;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    param.flag = TARGET_NOTFOUND;
    param.typ = SPP_UNIT;
    param.data.u = u2 = test_create_unit(test_create_faction(), u->region);
    arrput(args, param);
    set_level(u2, SK_MAGIC, 1);
    test_create_castorder(&co, u, 4, 5.0, 0, args);
    CuAssertIntEquals(tc, 0, sp_fumblecurse(&co));

    co.a_params[0].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_fumblecurse(&co));
    CuAssertPtrEquals(tc, NULL, get_curse(u2->attribs, &ct_fumble));

    co.a_params[0].flag = TARGET_OK;
    CuAssertIntEquals(tc, co.level, sp_fumblecurse(&co));
    CuAssertPtrNotNull(tc, c = get_curse(u2->attribs, &ct_fumble));
    CuAssertIntEquals(tc, co.level, c->duration);
    CuAssertDblEquals(tc, co.force * .5, c->effect, 0.01);
    CuAssertDblEquals(tc, co.force, c->vigour, 0.01);
    CuAssertPtrNotNull(tc, test_find_faction_message(u2->faction, "fumblecurse"));

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
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "deathcloud_effect"));
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
    test_teardown();
}

static void test_magicstreet(CuTest *tc) {
    unit *u;
    faction *f;
    region *r;
    castorder co;
    curse *c;

    test_setup();
    u = test_create_unit(f = test_create_faction(), r = test_create_plain(0, 0));
    test_create_castorder(&co, u, 4, 5.0, 0, NULL);
    CuAssertIntEquals(tc, co.level, sp_magicstreet(&co));
    CuAssertPtrNotNull(tc, c = get_curse(r->attribs, &ct_magicstreet));
    CuAssertIntEquals(tc, co.level + 1, c->duration);
    CuAssertDblEquals(tc, co.force, c->vigour, 0.01);
    CuAssertPtrNotNull(tc, test_find_region_message(r, "path_effect", f));
    test_teardown();
}

static void test_migrants(CuTest *tc) {
    unit *u, *u2;
    faction *f;
    castorder co;
    spellparameter param, * args = NULL;
    const struct race* rc;

    test_setup();
    u = test_create_unit(f = test_create_faction(), test_create_plain(0, 0));
    param.flag = TARGET_OK;
    param.typ = SPP_UNIT;
    param.data.u = u2 = test_create_unit(f, u->region);
    arrput(args, param);
    u2->orders = create_order(K_WORK, u2->faction->locale, NULL);
    u_setrace(u2, rc = test_create_race("hobgoblin"));
    test_create_castorder(&co, u, 4, 5.0, 0, args);

    /* bug 3019: no pay when accidentally used on own units: */
    CuAssertIntEquals(tc, 0, sp_migranten(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "error45"));
    u_setfaction(u2, f = test_create_faction());
    test_clear_messages(u->faction);

    /* only migrants are allowed, not units of our own race: */
    u_setrace(u2, u->faction->race);
    CuAssertIntEquals(tc, 0, sp_migranten(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "sp_migranten_fail1"));
    u_setrace(u2, rc);
    test_clear_messages(u->faction);

    co.a_params[0].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, u2->number, sp_migranten(&co));
    CuAssertPtrEquals(tc, f, u2->faction);
    test_clear_messages(u->faction);

    co.a_params[0].flag = TARGET_NOTFOUND;
    CuAssertIntEquals(tc, 0, sp_migranten(&co));
    CuAssertPtrEquals(tc, f, u2->faction);
    test_clear_messages(u->faction);

    /* no contact, no cost: */
    co.a_params[0].flag = TARGET_OK;
    CuAssertIntEquals(tc, 0, sp_migranten(&co));
    CuAssertPtrEquals(tc, f, u2->faction);
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "spellfail::contact"));
    test_clear_messages(u->faction);

    contact_unit(u2, u);
    CuAssertIntEquals(tc, u2->number, sp_migranten(&co));
    CuAssertPtrEquals(tc, u->faction, u2->faction);
    CuAssertPtrEquals(tc, NULL, u2->orders);
    test_clear_messages(u->faction);

    test_teardown();

}

static void test_blessstonecircle(CuTest *tc) {
    struct region *r;
    struct faction *f;
    unit *u;
    building *b;
    const struct building_type *btype;
    castorder co;
    spellparameter param, * args = NULL;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    b = test_create_building(r, test_create_buildingtype("stonecircle"));
    btype = test_create_buildingtype("blessedstonecircle");

    param.flag = TARGET_OK;
    param.typ = SPP_BUILDING;
    param.data.b = b;
    arrput(args, param);

    test_create_castorder(&co, u, 3, 4., 0, args);
    CuAssertIntEquals(tc, co.level, sp_blessstonecircle(&co));
    CuAssertPtrEquals(tc, (void *)btype, (void *)b->type);
    CuAssertPtrEquals(tc, NULL, b->attribs);

    /* if target not found, no costs, no effect */
    co.a_params[0].flag = TARGET_NOTFOUND;
    CuAssertIntEquals(tc, 0, sp_goodwinds(&co));

    /* if target resists, pay in full, no effect */
    co.a_params[0].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_goodwinds(&co));

    test_teardown();
}

static void test_destroy_magic_region(CuTest *tc) {
    struct region *r;
    struct faction *f;
    unit *u;
    castorder co;
    curse *c;
    spellparameter param, * args = NULL;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    param.flag = TARGET_NOTFOUND;
    param.typ = SPP_REGION;
    param.data.r = NULL;
    arrput(args, param);

    test_create_castorder(&co, u, 3, 4., 0, args);
    CuAssertIntEquals(tc, 0, sp_destroy_magic(&co));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_effect"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_noeffect"));

    co.a_params[0].data.r = r;
    co.a_params[0].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_destroy_magic(&co));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_effect"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_noeffect"));

    /* even without a curse to destroy, we still have to pay */
    co.a_params[0].flag = TARGET_OK;
    CuAssertIntEquals(tc, co.level, sp_destroy_magic(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "destroy_magic_noeffect"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_effect"));

    /* completely remove a weak curse: */
    test_clear_messages(f);
    c = create_curse(u, &r->attribs, &ct_holyground, co.level, 1, 1.0, 0);
    CuAssertIntEquals(tc, co.level, sp_destroy_magic(&co));
    CuAssertPtrEquals(tc, NULL, r->attribs);
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "destroy_magic_effect"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_noeffect"));

    /* weaken a strong curse: */
    test_clear_messages(f);
    c = create_curse(u, &r->attribs, &ct_holyground, 5.0, 1, 1.0, 0);
    CuAssertIntEquals(tc, co.level, sp_destroy_magic(&co));
    CuAssertDblEquals(tc, 5.0 - (co.level + 1) / 2, c->vigour, 0.01);
    CuAssertDblEquals(tc, 1.0, c->effect, 0.01);
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "destroy_magic_noeffect"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_effect"));

    test_teardown();
}

static void test_destroy_magic_unit(CuTest *tc) {
    struct region *r;
    struct faction *f;
    unit *u;
    castorder co;
    spellparameter param, * args = NULL;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    param.flag = TARGET_NOTFOUND;
    param.typ = SPP_UNIT;
    param.data.u = NULL;
    arrput(args, param);

    test_create_castorder(&co, u, 3, 4., 0, args);
    CuAssertIntEquals(tc, 0, sp_destroy_magic(&co));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_effect"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_noeffect"));

    co.a_params[0].data.u = test_create_unit(f, r);
    co.a_params[0].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_destroy_magic(&co));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_effect"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_noeffect"));

    /* even without a curse to destroy, we still have to pay */
    co.a_params[0].flag = TARGET_OK;
    CuAssertIntEquals(tc, co.level, sp_destroy_magic(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "destroy_magic_noeffect"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_effect"));

    test_teardown();
}

static void test_destroy_magic_building(CuTest *tc) {
    struct region *r;
    struct faction *f;
    unit *u;
    castorder co;
    spellparameter param, * args = NULL;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    param.flag = TARGET_NOTFOUND;
    param.typ = SPP_BUILDING;
    param.data.b = NULL;
    arrput(args, param);

    test_create_castorder(&co, u, 3, 4., 0, args);
    CuAssertIntEquals(tc, 0, sp_destroy_magic(&co));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_effect"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_noeffect"));

    co.a_params[0].data.b = test_create_building(r, NULL);
    co.a_params[0].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_destroy_magic(&co));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_effect"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_noeffect"));

    /* even without a curse to destroy, we still have to pay */
    co.a_params[0].flag = TARGET_OK;
    CuAssertIntEquals(tc, co.level, sp_destroy_magic(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "destroy_magic_noeffect"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_effect"));

    test_teardown();
}

static void test_destroy_magic_ship(CuTest *tc) {
    struct region *r;
    struct faction *f;
    unit *u;
    castorder co;
    spellparameter param, * args = NULL;

    test_setup();

    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    param.flag = TARGET_NOTFOUND;
    param.typ = SPP_SHIP;
    param.data.sh = NULL;
    arrput(args, param);

    test_create_castorder(&co, u, 3, 4., 0, args);
    CuAssertIntEquals(tc, 0, sp_destroy_magic(&co));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_effect"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_noeffect"));

    co.a_params[0].data.sh = test_create_ship(r, NULL);
    co.a_params[0].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_destroy_magic(&co));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_effect"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_noeffect"));

    /* even without a curse to destroy, we still have to pay */
    co.a_params[0].flag = TARGET_OK;
    CuAssertIntEquals(tc, co.level, sp_destroy_magic(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(u->faction, "destroy_magic_noeffect"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(u->faction, "destroy_magic_effect"));

    test_teardown();
}

static void test_rosthauch(CuTest *tc) {
    struct region *r;
    struct faction *f;
    unit *u, *u2, *u3;
    castorder co;
    item_type *it_sword, *it_rsword, *it_shield, *it_rshield;
    spellparameter param, * args = NULL;

    test_setup();
    it_sword = test_create_itemtype("sword");
    it_rsword = test_create_itemtype("rustysword");
    it_shield = test_create_itemtype("shield");
    it_rshield = test_create_itemtype("rustyshield");
    // must call this to register iron items:
    init_spells();

    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    param.flag = TARGET_NOTFOUND;
    param.typ = SPP_UNIT;
    param.data.u = u2 = test_create_unit(test_create_faction(), r);
    arrput(args, param);
    param.data.u = u3 = test_create_unit(u2->faction, r);
    arrput(args, param);

    random_source_inject_constants(0.0, 0);
    test_create_castorder(&co, u, 3, 4., 0, args);

    /* if target not found, no costs, no effect */
    CuAssertIntEquals(tc, 0, sp_rosthauch(&co));

    /* if target resists, pay in full, no effect */
    co.a_params[0].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_rosthauch(&co));

    /* target found, pay even if nothing happened: */
    co.a_params[0].flag = TARGET_OK;
    CuAssertIntEquals(tc, co.level, sp_rosthauch(&co));

    /* at level 4, destroy up to 24 swords: */
    i_change(&u2->items, it_sword, 40);
    CuAssertIntEquals(tc, co.level, sp_rosthauch(&co));
    CuAssertIntEquals(tc, 16, i_get(u2->items, it_sword));
    CuAssertIntEquals(tc, 24, i_get(u2->items, it_rsword));

    /* spell can work on more than one unit, shields are affected after swords, 
     * only 50% can be rusted.
     * force of 24 = 16 sword, 4 shields
     */
    co.a_params[1].flag = TARGET_OK;
    i_change(&u2->items, it_shield, 2);
    i_change(&u3->items, it_shield, 12);
    CuAssertIntEquals(tc, co.level, sp_rosthauch(&co));
    CuAssertIntEquals(tc, 0, i_get(u2->items, it_sword));
    CuAssertIntEquals(tc, 1, i_get(u2->items, it_shield));
    CuAssertIntEquals(tc, 9, i_get(u3->items, it_shield));
    CuAssertIntEquals(tc, 3, i_get(u3->items, it_rshield));

    test_teardown();
}


static void test_babbler(CuTest *tc) {
    struct region *r;
    struct faction *f, *f2;
    unit *u, *u2;
    castorder co;
    spellparameter param, * args = NULL;

    test_setup();

    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    param.typ = SPP_UNIT;
    param.data.u = u2 = test_create_unit(f2 = test_create_faction(), r);
    set_otherfaction(u2, f);
    param.flag = TARGET_NOTFOUND;
    arrput(args, param);
    create_mage(u2, M_GWYRRD);
    test_create_castorder(&co, u, 3, 4., 0, args);

    CuAssertIntEquals(tc, 0, sp_babbler(&co));

    co.a_params[0].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_babbler(&co));
    CuAssertPtrNotNull(tc, test_find_region_message(r, "babbler_resist", f2));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "spyreport"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(f, "spyreport_faction"));
    test_clear_messages(f2);

    co.a_params[0].flag = TARGET_OK;
    CuAssertIntEquals(tc, co.level, sp_babbler(&co));
    CuAssertPtrNotNull(tc, test_find_region_message(r, "babbler_effect", f2));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "spyreport"));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "spyreport_mage"));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "spyreport_faction"));
    test_clear_messages(f);

    co.a_params[0].data.u = test_create_unit(f, r);
    CuAssertIntEquals(tc, 0, sp_babbler(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "error45"));
    test_clear_messages(f);

    test_teardown();
}

static void test_sparkle(CuTest *tc) {
    struct region *r;
    struct faction *f;
    unit *u, *u2;
    castorder co;
    curse *c;
    spellparameter param, * args = NULL;

    test_setup();
    random_source_inject_constants(.0, 0); /** should make all magic resistance succeed */

    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    param.typ = SPP_UNIT;
    param.data.u = u2 = test_create_unit(test_create_faction(), r);
    param.flag = TARGET_NOTFOUND;
    arrput(args, param);
    test_create_castorder(&co, u, 3, 4., 0, args);

    CuAssertIntEquals(tc, 0, sp_sparkle(&co));

    co.a_params[0].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_sparkle(&co));

    co.a_params[0].flag = TARGET_OK;
    CuAssertIntEquals(tc, co.level, sp_sparkle(&co));
    CuAssertPtrNotNull(tc, c = get_curse(u2->attribs, &ct_sparkle));
    CuAssertIntEquals(tc, co.level + 1, c->duration);
    CuAssertDblEquals(tc, co.force, c->vigour, 0.01);
    CuAssertDblEquals(tc, 0.0, c->effect - (int)c->effect, 0.01);

    test_teardown();
}

static void test_readmind(CuTest *tc) {
    struct region *r;
    struct faction *f;
    unit *u, *u2;
    castorder co;
    spellparameter param, *args = NULL;

    test_setup();

    r = test_create_plain(0, 0);
    f = test_create_faction();
    u = test_create_unit(f, r);
    param.typ = SPP_UNIT;
    param.data.u = u2 = test_create_unit(test_create_faction(), r);
    param.flag = TARGET_NOTFOUND;
    arrput(args, param);
    test_create_castorder(&co, u, 3, 4., 0, args);

    // target not found:
    CuAssertIntEquals(tc, 0, sp_readmind(&co));
    CuAssertPtrEquals(tc, NULL, f->msgs);
    CuAssertPtrEquals(tc, NULL, u2->faction->msgs);

    // target resistts and notices:
    co.a_params[0].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_readmind(&co));
    CuAssertPtrEquals(tc, NULL, f->msgs);
    CuAssertPtrNotNull(tc, test_find_faction_message(u2->faction, "stealdetect"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(f, "spyreport"));
    test_clear_messages(u2->faction);

    // success:
    co.a_params[0].flag = TARGET_OK;
    CuAssertIntEquals(tc, co.level, sp_readmind(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "spyreport"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(f, "spyreport_faction"));
    CuAssertPtrEquals(tc, NULL, u2->faction->msgs);
    test_clear_messages(f);

    // not on your own units:
    u_setfaction(u2, f);
    CuAssertIntEquals(tc, 0, sp_readmind(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "error45"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(f, "spyreport"));

    test_teardown();
}

static void test_pump(CuTest *tc) {
    struct region *r, *r2;
    struct faction *f, *f2;
    unit *u, *u2;
    castorder co;
    spellparameter param, *args = NULL;

    test_setup();

    r = test_create_plain(0, 0);
    r2 = test_create_plain(1, 0);
    u = test_create_unit(f = test_create_faction(), r);
    param.typ = SPP_UNIT;
    param.data.u = u2 = test_create_unit(f2 = test_create_faction(), r);
    param.flag = TARGET_NOTFOUND;
    arrput(args, param);
    param.typ = SPP_REGION;
    param.data.r = r;
    param.flag = TARGET_NOTFOUND; /* does not matter */
    arrput(args, param);
    set_level(u2, SK_PERCEPTION, 4);
    test_create_castorder(&co, u, 3, 4., 0, args);

    /* fail spell if any additional resistance checks are made: */
    random_source_inject_constants(.0, 0);

    /* spell fails because target unit not found: */
    CuAssertIntEquals(tc, 0, sp_pump(&co));
    CuAssertPtrEquals(tc, NULL, f->msgs);
    co.a_params[0].flag = TARGET_OK;

    /* spell fails because target region not found: */
    CuAssertIntEquals(tc, 0, sp_pump(&co));
    CuAssertPtrEquals(tc, NULL, f->msgs);
    co.a_params[1].flag = TARGET_RESISTS; /* ignored as long as not TARGET_NOTFOUND */

    /* success: */
    CuAssertIntEquals(tc, co.level, sp_pump(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "pump_effect"));
    CuAssertIntEquals(tc, 4, get_observer(r, f));
    /* TODO: observer duration 2 */

    co.a_params[1].data.r = r2; /* no unit of f2 in the region, no effect, half price */
    CuAssertIntEquals(tc, co.level / 2, sp_pump(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "spellfail_pump"));

    /* TODO: fails against undead */
    /* TODO: fails against monsters */

    CuAssertPtrEquals(tc, NULL, f2->msgs);
}

static void test_summon_familiar(CuTest *tc) {
    struct region *r;
    struct faction *f;
    race *rc, *rc_special;
    unit *u, *u2;
    castorder co;

    test_setup();
    rc = test_create_race("human");
    rc->familiars[0] = test_create_race("goblin");
    rc->familiars[M_DRAIG] = rc_special = test_create_race("demon");
    r = test_create_plain(0, 0);
    f = test_create_faction();
    f->race = rc;
    f->magiegebiet = M_DRAIG;
    u = test_create_unit(f, r);

    /* mit force 20 bekommt man garantiert den magiegebiet-familiar: */
    random_source_inject_constants(0.0, 0);
    test_create_castorder(&co, u, 3, 20., 0, NULL);

    CuAssertIntEquals(tc, co.level, sp_summon_familiar(&co));
    CuAssertPtrNotNull(tc, u2 = get_familiar(u));
    CuAssertTrue(tc, is_familiar(u2));
    CuAssertPtrEquals(tc, u, get_familiar_mage(u2));
    CuAssertPtrEquals(tc, rc_special, (race *)u_race(u2));
    test_teardown();
}

static void test_shadowdemons(CuTest *tc) {
    struct region *r;
    struct faction *f;
    unit *u, *u2;
    race *rc;
    castorder co;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    f->magiegebiet = M_DRAIG;
    u = test_create_unit(f, r);
    rc = test_create_race("shadowdemon");
    CuAssertPtrEquals(tc, (void *)get_race(RC_SHADOW), rc);
    test_set_skill(u, SK_MAGIC, 10, 1);
    test_set_skill(u, SK_STEALTH, 6, 1);
    test_create_castorder(&co, u, 3, 10., 0, NULL);

    CuAssertIntEquals(tc, co.level, sp_shadowdemons(&co));
    CuAssertPtrNotNull(tc, u2 = u->next);
    CuAssertPtrEquals(tc, u->faction, u2->faction);
    CuAssertPtrEquals(tc, rc, (race *)u_race(u2));
    CuAssertIntEquals(tc, 100, u2->number);
    CuAssertIntEquals(tc, 8, get_level(u2, SK_STEALTH));
    CuAssertIntEquals(tc, 1, get_level(u2, SK_PERCEPTION));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "summonshadow_effect"));
    test_teardown();
}

static void test_shadowlords(CuTest *tc) {
    struct region *r;
    struct faction *f;
    unit *u, *u2;
    race *rc;
    castorder co;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    f->magiegebiet = M_DRAIG;
    u = test_create_unit(f, r);
    rc = test_create_race("shadowmaster");
    CuAssertPtrEquals(tc, (void *)get_race(RC_SHADOWLORD), rc);
    test_set_skill(u, SK_MAGIC, 10, 1);
    test_set_skill(u, SK_STEALTH, 6, 1);
    test_create_castorder(&co, u, 3, 10., 0, NULL);

    CuAssertIntEquals(tc, co.level, sp_shadowlords(&co));
    CuAssertPtrNotNull(tc, u2 = u->next);
    CuAssertPtrEquals(tc, u->faction, u2->faction);
    CuAssertPtrEquals(tc, rc, (race *)u_race(u2));
    CuAssertIntEquals(tc, 100, u2->number);
    CuAssertIntEquals(tc, 9, get_level(u2, SK_STEALTH));
    CuAssertIntEquals(tc, 5, get_level(u2, SK_PERCEPTION));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "summon_effect"));
    test_clear_messages(f);
    test_teardown();
}

static void test_analysemagic_unit(CuTest *tc)
{
    struct region *r;
    struct faction *f;
    unit *u, *u2;
    castorder co;
    spellparameter param, *args = NULL;
    message *m;
    curse *c;

    test_setup();
    random_source_inject_constants(0.0, 0);
    mt_create_va(mt_new("analyse_unit_age", NULL), "mage:unit", "unit:unit", "curse:curse", "months:int", MT_NEW_END);
    r = test_create_plain(0, 0);
    f = test_create_faction();
    f->magiegebiet = M_DRAIG;
    u = test_create_unit(f, r);
    u2 = test_create_unit(f, r);

    param.flag = TARGET_OK;
    param.typ = SPP_UNIT;
    param.data.u = u2;
    arrput(args, param);
    test_create_castorder(&co, u, 3, 4., 0, args);
    CuAssertIntEquals(tc, co.level, sp_analysemagic(&co));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(f, "analyse_unit_age"));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "analyse_unit_nospell"));
    test_clear_messages(f);

    c = create_curse(u, &u2->attribs, &ct_astralblock, 5.0, 20, 5, 0);
    CuAssertIntEquals(tc, co.level, sp_analysemagic(&co));
    /* curse is too strong, analysis fails */
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "analyse_unit_fail"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(f, "analyse_unit_nospell"));
    test_clear_messages(f);

    c->vigour = co.force;
    CuAssertIntEquals(tc, co.level, sp_analysemagic(&co));
    CuAssertPtrNotNull(tc, m = test_find_faction_message(f, "analyse_unit_age"));
    CuAssertPtrEquals(tc, u, m->parameters[0].v);
    CuAssertPtrEquals(tc, u2, m->parameters[1].v);
    CuAssertPtrEquals(tc, (void *)c->type, m->parameters[2].v);
    CuAssertIntEquals(tc, 15, m->parameters[3].i);
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(f, "analyse_unit_nospell"));

    test_clear_messages(f);
    co.a_params[0].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_analysemagic(&co));
    CuAssertIntEquals(tc, 0, test_count_messagetype(f->msgs, NULL));

    test_teardown();
}

static void test_analysemagic_ship(CuTest *tc)
{
    struct region *r;
    struct faction *f;
    unit *u;
    castorder co;
    spellparameter param, *args = NULL;
    message *m;
    curse *c;

    test_setup();
    random_source_inject_constants(0.0, 0);
    mt_create_va(mt_new("analyse_ship_age", NULL), "mage:unit", "ship:ship", "curse:curse", "months:int", MT_NEW_END);
    r = test_create_plain(0, 0);
    f = test_create_faction();
    f->magiegebiet = M_DRAIG;
    u = test_create_unit(f, r);
    u->ship = test_create_ship(r, NULL);

    param.flag = TARGET_OK;
    param.typ = SPP_SHIP;
    param.data.sh = u->ship;
    arrput(args, param);

    test_create_castorder(&co, u, 3, 4., 0, args);
    CuAssertIntEquals(tc, co.level, sp_analysemagic(&co));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(f, "analyse_ship_age"));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "analyse_ship_nospell"));
    test_clear_messages(f);

    c = create_curse(u, &u->ship->attribs, &ct_flyingship, 5.0, 20, 5, 0);
    CuAssertIntEquals(tc, co.level, sp_analysemagic(&co));
    /* curse is too strong, analysis fails */
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "analyse_ship_fail"));
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(f, "analyse_ship_nospell"));
    test_clear_messages(f);

    c->vigour = co.force;
    CuAssertIntEquals(tc, co.level, sp_analysemagic(&co));
    CuAssertPtrNotNull(tc, m = test_find_faction_message(f, "analyse_ship_age"));
    CuAssertPtrEquals(tc, u, m->parameters[0].v);
    CuAssertPtrEquals(tc, u->ship, m->parameters[1].v);
    CuAssertPtrEquals(tc, (void *)c->type, m->parameters[2].v);
    CuAssertIntEquals(tc, 15, m->parameters[3].i);
    CuAssertPtrEquals(tc, NULL, test_find_faction_message(f, "analyse_ship_nospell"));

    test_clear_messages(f);
    co.a_params[0].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_analysemagic(&co));
    CuAssertIntEquals(tc, 0, test_count_messagetype(f->msgs, NULL));

    test_teardown();
}

static void test_analysemagic_building(CuTest *tc)
{
    struct region *r;
    struct faction *f;
    unit *u;
    castorder co;
    spellparameter param, *args = NULL;
    message *m;
    curse *c, *c2;

    test_setup();
    random_source_inject_constants(0.0, 0);
    mt_create_va(mt_new("analyse_building_age", NULL), "mage:unit", "building:building", "curse:curse", "months:int", MT_NEW_END);
    mt_create_va(mt_new("analyse_building_noage", NULL), "mage:unit", "building:building", "curse:curse", MT_NEW_END);
    r = test_create_plain(0, 0);
    f = test_create_faction();
    f->magiegebiet = M_DRAIG;
    u = test_create_unit(f, r);
    u->building = test_create_building(r, NULL);

    param.flag = TARGET_OK;
    param.typ = SPP_BUILDING;
    param.data.b = u->building;
    arrput(args, param);

    test_create_castorder(&co, u, 3, 4., 0, args);
    CuAssertIntEquals(tc, co.level, sp_analysemagic(&co));
    CuAssertIntEquals(tc, 1, test_count_messagetype(f->msgs, "analyse_building_nospell"));
    CuAssertIntEquals(tc, 0, test_count_messagetype(f->msgs, "analyse_building_age"));
    test_clear_messages(f);

    c = create_curse(u, &u->building->attribs, &ct_strongwall, 5.0, 20, 1, 0);
    CuAssertIntEquals(tc, co.level, sp_analysemagic(&co));
    CuAssertIntEquals(tc, 0, test_count_messagetype(f->msgs, "analyse_building_nospell"));
    /* curse is too strong, analysis fails */
    CuAssertIntEquals(tc, 1, test_count_messagetype(f->msgs, "analyse_building_fail"));
    test_clear_messages(f);

    c->vigour = co.force;
    c2 = create_curse(u, &u->building->attribs, &ct_magicwalls, co.force, 20, 1, 0);
    CuAssertIntEquals(tc, co.level, sp_analysemagic(&co));
    CuAssertIntEquals(tc, 0, test_count_messagetype(f->msgs, "analyse_building_nospell"));
    CuAssertPtrNotNull(tc, m = test_find_faction_message(f, "analyse_building_age"));
    CuAssertPtrEquals(tc, u, m->parameters[0].v);
    CuAssertPtrEquals(tc, u->building, m->parameters[1].v);
    CuAssertPtrEquals(tc, (void *)c->type, m->parameters[2].v);
    CuAssertIntEquals(tc, 15, m->parameters[3].i);
    CuAssertPtrNotNull(tc, m = test_find_faction_message(f, "analyse_building_noage"));
    CuAssertPtrEquals(tc, u, m->parameters[0].v);
    CuAssertPtrEquals(tc, u->building, m->parameters[1].v);
    CuAssertPtrEquals(tc, (void *)c2->type, m->parameters[2].v);

    test_clear_messages(f);
    co.a_params[0].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_analysemagic(&co));
    CuAssertIntEquals(tc, 0, test_count_messagetype(f->msgs, NULL));

    test_teardown();
}

static void test_analysemagic_region(CuTest *tc)
{
    struct region *r;
    struct faction *f;
    unit *u;
    castorder co;
    spellparameter param, *args = NULL;
    message *m;
    curse *c, *c2;

    test_setup();
    random_source_inject_constants(0.0, 0);
    mt_create_va(mt_new("analyse_region_age", NULL), "mage:unit", "region:region", "curse:curse", "months:int", MT_NEW_END);
    mt_create_va(mt_new("analyse_region_noage", NULL), "mage:unit", "region:region", "curse:curse", MT_NEW_END);
    r = test_create_plain(0, 0);
    f = test_create_faction();
    f->magiegebiet = M_DRAIG;
    u = test_create_unit(f, r);

    param.flag = TARGET_OK;
    param.typ = SPP_REGION;
    param.data.r = u->region;
    arrput(args, param);

    test_create_castorder(&co, u, 3, 4., 0, args);
    CuAssertIntEquals(tc, co.level, sp_analysemagic(&co));
    CuAssertIntEquals(tc, 1, test_count_messagetype(f->msgs, "analyse_region_nospell"));
    CuAssertIntEquals(tc, 0, test_count_messagetype(f->msgs, "analyse_region_age"));
    test_clear_messages(f);

    c = create_curse(u, &u->region->attribs, &ct_peacezone, 5.0, 20, 1, 0);
    CuAssertIntEquals(tc, co.level, sp_analysemagic(&co));
    CuAssertIntEquals(tc, 0, test_count_messagetype(f->msgs, "analyse_region_nospell"));
    /* curse is too strong, analysis fails */
    CuAssertIntEquals(tc, 1, test_count_messagetype(f->msgs, "analyse_region_fail"));
    test_clear_messages(f);

    c->vigour = co.force;
    c2 = create_curse(u, &u->region->attribs, &ct_holyground, co.force, 20, 1, 0);
    CuAssertIntEquals(tc, co.level, sp_analysemagic(&co));
    CuAssertIntEquals(tc, 0, test_count_messagetype(f->msgs, "analyse_region_nospell"));
    CuAssertPtrNotNull(tc, m = test_find_faction_message(f, "analyse_region_age"));
    CuAssertPtrEquals(tc, u, m->parameters[0].v);
    CuAssertPtrEquals(tc, u->region, m->parameters[1].v);
    CuAssertPtrEquals(tc, (void *)c->type, m->parameters[2].v);
    CuAssertIntEquals(tc, 15, m->parameters[3].i);
    CuAssertPtrNotNull(tc, m = test_find_faction_message(f, "analyse_region_noage"));
    CuAssertPtrEquals(tc, u, m->parameters[0].v);
    CuAssertPtrEquals(tc, u->region, m->parameters[1].v);
    CuAssertPtrEquals(tc, (void *)c2->type, m->parameters[2].v);

    test_clear_messages(f);
    co.a_params[0].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_analysemagic(&co));
    CuAssertIntEquals(tc, 0, test_count_messagetype(f->msgs, NULL));

    test_teardown();
}

static void test_firewall_spell(CuTest *tc)
{
    unit *u;
    castorder co;
    spellparameter param, *args = NULL;
    region *r2;
    connection *wall;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    r2 = test_create_plain(1, 0);
    u->faction->magiegebiet = M_DRAIG;

    param.flag = TARGET_OK;
    param.typ = SPP_STRING;
    param.data.xs = str_strdup(locale_string(u->faction->locale, directions[D_EAST], true));
    arrput(args, param);

    test_create_castorder(&co, u, 3, 4., 0, args);
    CuAssertIntEquals(tc, co.level, sp_firewall(&co));
    CuAssertPtrNotNull(tc, wall = get_borders(u->region, r2));
    CuAssertPtrEquals(tc, &bt_firewall, wall->type);

    test_teardown();
}

static void test_create_firewall(CuTest *tc)
{
    unit *u;
    region *r2, *r1;
    connection *wall;
    wall_data *fd;
    curse *c;
    double force;

    test_setup();
    u = test_create_unit(test_create_faction(), r1 = test_create_plain(0, 0));
    r2 = test_create_plain(1, 0);
    create_firewall(u, r1, D_EAST, force = 3.0, 2);
    CuAssertPtrNotNull(tc, wall = get_borders(u->region, r2));
    CuAssertPtrEquals(tc, &bt_firewall, wall->type);
    CuAssertPtrNotNull(tc, fd = (wall_data *)wall->data.v);
    CuAssertTrue(tc, ! fd->active);
    CuAssertDblEquals(tc, 3.0, fd->force, 0.01);
    CuAssertIntEquals(tc, 4, fd->countdown);

    CuAssertPtrNotNull(tc, c = get_curse(r1->attribs, &ct_firewall));
    CuAssertIntEquals(tc, 3, c->duration);
    CuAssertDblEquals(tc, force, c->vigour, 0.01);
    CuAssertPtrEquals(tc, u, c->magician);
    CuAssertIntEquals(tc, D_EAST, c->data.sa[0]);

    CuAssertPtrNotNull(tc, c = get_curse(r2->attribs, &ct_firewall));
    CuAssertIntEquals(tc, 3, c->duration);
    CuAssertDblEquals(tc, force, c->vigour, 0.01);
    CuAssertPtrEquals(tc, u, c->magician);
    CuAssertIntEquals(tc, D_WEST, c->data.sa[0]);

    test_teardown();
}

static void test_destroy_firewall(CuTest *tc)
{
    unit *u;
    region *r2, *r1;
    curse *c;
    double force;

    test_setup();
    u = test_create_unit(test_create_faction(), r1 = test_create_plain(0, 0));
    r2 = test_create_plain(1, 0);
    create_firewall(u, r1, D_EAST, force = 3.0, 2);

    CuAssertPtrNotNull(tc, c = get_curse(r1->attribs, &ct_firewall));
    CuAssertDblEquals(tc, 0.0, reduce_curse(c, 4, c->vigour, r1), 0.001);

    CuAssertPtrEquals(tc, NULL, get_borders(u->region, r2));

    test_teardown();
}


CuSuite *get_spells_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_watch_region);
    SUITE_ADD_TEST(suite, test_view_reality);
    SUITE_ADD_TEST(suite, test_disruptastral);
    SUITE_ADD_TEST(suite, test_eternizewall);
    SUITE_ADD_TEST(suite, test_permtransfer);
    SUITE_ADD_TEST(suite, test_movecastle);
    SUITE_ADD_TEST(suite, test_auraleak);
    SUITE_ADD_TEST(suite, test_leaveastral);
    SUITE_ADD_TEST(suite, test_show_astral);
    SUITE_ADD_TEST(suite, test_good_dreams);
    SUITE_ADD_TEST(suite, test_bad_dreams);
    SUITE_ADD_TEST(suite, test_dreams);
    SUITE_ADD_TEST(suite, test_speed2);
    SUITE_ADD_TEST(suite, test_goodwinds);
    SUITE_ADD_TEST(suite, test_change_race);
    SUITE_ADD_TEST(suite, test_summonent);
    SUITE_ADD_TEST(suite, test_summonundead);
    SUITE_ADD_TEST(suite, test_maelstrom);
    SUITE_ADD_TEST(suite, test_blessedharvest);
    SUITE_ADD_TEST(suite, test_kaelteschutz);
    SUITE_ADD_TEST(suite, test_treewalkenter);
    SUITE_ADD_TEST(suite, test_treewalkexit);
    SUITE_ADD_TEST(suite, test_pullastral);
    SUITE_ADD_TEST(suite, test_holyground);
    SUITE_ADD_TEST(suite, test_drought);
    SUITE_ADD_TEST(suite, test_great_drought_no_terraform);
    SUITE_ADD_TEST(suite, test_great_drought_to_swamp);
    SUITE_ADD_TEST(suite, test_great_drought_to_ocean);
    SUITE_ADD_TEST(suite, test_stormwinds);
    SUITE_ADD_TEST(suite, test_fumblecurse);
    SUITE_ADD_TEST(suite, test_deathcloud);
    SUITE_ADD_TEST(suite, test_magicboost);
    SUITE_ADD_TEST(suite, test_migrants);
    SUITE_ADD_TEST(suite, test_magicstreet);
    SUITE_ADD_TEST(suite, test_blessstonecircle);
    SUITE_ADD_TEST(suite, test_destroy_magic_region);
    SUITE_ADD_TEST(suite, test_destroy_magic_unit);
    SUITE_ADD_TEST(suite, test_destroy_magic_building);
    SUITE_ADD_TEST(suite, test_destroy_magic_ship);
    SUITE_ADD_TEST(suite, test_rosthauch);
    SUITE_ADD_TEST(suite, test_babbler);
    SUITE_ADD_TEST(suite, test_sparkle);
    SUITE_ADD_TEST(suite, test_readmind);
    SUITE_ADD_TEST(suite, test_pump);
    SUITE_ADD_TEST(suite, test_summon_familiar);
    SUITE_ADD_TEST(suite, test_shadowdemons);
    SUITE_ADD_TEST(suite, test_shadowlords);
    SUITE_ADD_TEST(suite, test_analysemagic_unit);
    //SUITE_ADD_TEST(suite, test_analysemagic_temp);
    SUITE_ADD_TEST(suite, test_analysemagic_ship);
    SUITE_ADD_TEST(suite, test_analysemagic_building);
    SUITE_ADD_TEST(suite, test_analysemagic_region);
    SUITE_ADD_TEST(suite, test_firewall_spell);
    SUITE_ADD_TEST(suite, test_create_firewall);
    SUITE_ADD_TEST(suite, test_destroy_firewall);
    SUITE_ADD_TEST(suite, test_break_curse);
    SUITE_ADD_TEST(suite, test_magicrunes);

    return suite;
}
