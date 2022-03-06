#include "volcano.h"
#include "alchemy.h"

#include "attributes/reduceproduction.h"

#include "kernel/attrib.h"
#include "kernel/building.h"
#include "kernel/faction.h"
#include "kernel/item.h"
#include "kernel/messages.h"
#include "kernel/region.h"
#include "kernel/terrain.h"
#include "kernel/unit.h"

#include "util/message.h"
#include "util/rand.h"
#include "util/rng.h"

#include <tests.h>
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

static void test_volcano_damage(CuTest* tc) {
    unit* u;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    scale_number(u, 100);
    u->hp = u->number * 20;
    CuAssertIntEquals(tc, 0, volcano_damage(u, "0"));
    CuAssertIntEquals(tc, u->number * 20, u->hp);
    CuAssertIntEquals(tc, 0, volcano_damage(u, "10"));
    CuAssertIntEquals(tc, u->number * 10, u->hp);
    CuAssertIntEquals(tc, 0, volcano_damage(u, "d9"));

    /* 10 people have 11 HP, the rest has 10 and dies */
    u->hp = 1010;
    CuAssertIntEquals(tc, 90, volcano_damage(u, "10"));
    CuAssertIntEquals(tc, 10, u->number);
    CuAssertIntEquals(tc, 10, u->hp);

    test_teardown();
}

static void test_volcano_damage_armor(CuTest* tc) {
    unit* u;
    item_type* itype;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    scale_number(u, 100);
    itype = test_create_itemtype("plate");
    new_armortype(itype, 0.0, frac_zero, 1, 0);
    i_change(&u->items, itype, 50);
    u->hp = u->number * 10;
    CuAssertIntEquals(tc, 50, volcano_damage(u, "10"));
    CuAssertIntEquals(tc, u->number, u->hp);

    test_teardown();
}

static void test_volcano_damage_buildings(CuTest* tc) {
    unit* u;
    building_type* btype;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    scale_number(u, 100);
    btype = test_create_castle();
    u->building = test_create_building(u->region, btype);
    u->building->size = 100; /* Turm, 2 Punkte Bonus */
    u->hp = u->number * 10;
    CuAssertIntEquals(tc, 0, volcano_damage(u, "10"));
    CuAssertIntEquals(tc, 3 * u->number, u->hp);

    scale_number(u, 40);
    u->hp = u->number * 10;
    u->building->size = 40; /* Befestigung, 1 Punkt Bonus */
    CuAssertIntEquals(tc, 0, volcano_damage(u, "10"));
    CuAssertIntEquals(tc, 40, u->number);
    CuAssertIntEquals(tc, 2 * u->number, u->hp);

    btype->flags -= BTF_FORTIFICATION; /* regular buildings provide just 1 point total */
    u->hp = u->number * 10;
    CuAssertIntEquals(tc, 0, volcano_damage(u, "10"));
    CuAssertIntEquals(tc, u->number, u->hp);

    scale_number(u, 100); /* unit is to big for the building, everyone gets hurt */
    u->hp = u->number * 10;
    CuAssertIntEquals(tc, 100, volcano_damage(u, "10"));
    CuAssertIntEquals(tc, 0, u->number);
    CuAssertIntEquals(tc, 0, u->hp);

    test_teardown();
}

static void test_volcano_damage_cats(CuTest* tc) {
    unit* u;
    struct race* rc_cat;

    test_setup();
    rc_cat = test_create_race("cat");
    u = test_create_unit(test_create_faction_ex(rc_cat, NULL), test_create_plain(0, 0));
    scale_number(u, 100);

    random_source_inject_constants(0.0, 0); /* cats are always lucky */
    u->hp = u->number * 10;
    CuAssertIntEquals(tc, 0, volcano_damage(u, "10"));
    CuAssertIntEquals(tc, 100, u->number);
    CuAssertIntEquals(tc, 10 * u->number, u->hp);

    random_source_inject_constants(0.0, 1); /* cats are never lucky */
    CuAssertIntEquals(tc, 100, volcano_damage(u, "10"));
    CuAssertIntEquals(tc, 0, u->number);
    CuAssertIntEquals(tc, 0, u->hp);

    test_teardown();
}

static void test_volcano_damage_healing_potions(CuTest* tc) {
    unit* u;
    item_type* itype;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    scale_number(u, 100);
    itype = test_create_itemtype("healing");
    new_potiontype(itype, 1);
    oldpotiontype[P_HEAL] = itype;
    i_change(&u->items, itype, 50); /* saves up to 4 dead people each */

    random_source_inject_constants(0.0, 1); /* potions always work */
    u->hp = u->number * 10;
    CuAssertIntEquals(tc, 0, volcano_damage(u, "10"));
    CuAssertIntEquals(tc, 100, u->number);
    CuAssertIntEquals(tc, 10 * u->number, u->hp);
    CuAssertIntEquals(tc, 25, i_get(u->items, itype));

    random_source_inject_constants(0.0, 0); /* potions never work, everyone dies */
    u->hp = u->number * 10;
    CuAssertIntEquals(tc, 100, volcano_damage(u, "10"));
    CuAssertIntEquals(tc, 0, u->number);
    CuAssertIntEquals(tc, 0, u->hp);
    CuAssertIntEquals(tc, 0, i_get(u->items, itype));

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
    f = test_create_faction();
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
    SUITE_ADD_TEST(suite, test_volcano_damage);
    SUITE_ADD_TEST(suite, test_volcano_damage_healing_potions);
    SUITE_ADD_TEST(suite, test_volcano_damage_armor);
    SUITE_ADD_TEST(suite, test_volcano_damage_buildings);
    SUITE_ADD_TEST(suite, test_volcano_damage_cats);
    SUITE_ADD_TEST(suite, test_volcano_outbreak);
    return suite;
}
