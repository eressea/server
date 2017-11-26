#include <platform.h>
#include <kernel/config.h>
#include <kernel/curse.h>
#include <kernel/item.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/spell.h>
#include <kernel/terrain.h>
#include <util/attrib.h>
#include <util/base36.h>
#include <util/language.h>
#include <util/rng.h>
#include <spells/regioncurse.h>
#include <alchemy.h>
#include <laws.h>
#include <spells.h>
#include "unit.h"

#include <CuTest.h>
#include <tests.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void test_remove_empty_units(CuTest *tc) {
    unit *u;
    int uid;

    test_setup();
    test_create_world();

    u = test_create_unit(test_create_faction(test_create_race("human")), findregion(0, 0));
    uid = u->no;
    remove_empty_units();
    CuAssertPtrNotNull(tc, findunit(uid));
    u->number = 0;
    remove_empty_units();
    CuAssertPtrEquals(tc, 0, findunit(uid));
    test_cleanup();
}

static void test_remove_empty_units_in_region(CuTest *tc) {
    unit *u;
    int uid;

    test_setup();
    test_create_world();

    u = test_create_unit(test_create_faction(test_create_race("human")), findregion(0, 0));
    u = test_create_unit(u->faction, u->region);
    CuAssertPtrNotNull(tc, u->nextF);
    uid = u->no;
    remove_empty_units_in_region(u->region);
    CuAssertPtrNotNull(tc, findunit(uid));
    u->number = 0;
    remove_empty_units_in_region(u->region);
    CuAssertPtrEquals(tc, 0, findunit(uid));
    CuAssertPtrEquals(tc, 0, u->nextF);
    CuAssertPtrEquals(tc, 0, u->region);
    test_cleanup();
}

static void test_remove_units_without_faction(CuTest *tc) {
    unit *u;
    int uid;

    test_setup();
    test_create_world();

    u = test_create_unit(test_create_faction(test_create_race("human")), findregion(0, 0));
    uid = u->no;
    u_setfaction(u, 0);
    remove_empty_units_in_region(u->region);
    CuAssertPtrEquals(tc, 0, findunit(uid));
    CuAssertIntEquals(tc, 0, u->number);
    test_cleanup();
}

static void test_remove_units_with_dead_faction(CuTest *tc) {
    unit *u;
    int uid;

    test_setup();
    test_create_world();

    u = test_create_unit(test_create_faction(test_create_race("human")), findregion(0, 0));
    uid = u->no;
    u->faction->_alive = false;
    remove_empty_units_in_region(u->region);
    CuAssertPtrEquals(tc, 0, findunit(uid));
    CuAssertIntEquals(tc, 0, u->number);
    test_cleanup();
}

static void test_scale_number(CuTest *tc) {
    unit *u;
    const struct potion_type *ptype;

    test_setup();
    test_create_world();
    ptype = new_potiontype(it_get_or_create(rt_get_or_create("hodor")), 1);
    u = test_create_unit(test_create_faction(test_create_race("human")), findregion(0, 0));
    change_effect(u, ptype, 1);
    u->hp = 35;
    CuAssertIntEquals(tc, 1, u->number);
    CuAssertIntEquals(tc, 35, u->hp);
    CuAssertIntEquals(tc, 1, get_effect(u, ptype));
    scale_number(u, 2);
    CuAssertIntEquals(tc, 2, u->number);
    CuAssertIntEquals(tc, 35 * u->number, u->hp);
    CuAssertIntEquals(tc, u->number, get_effect(u, ptype));
    scale_number(u, 8237);
    CuAssertIntEquals(tc, 8237, u->number);
    CuAssertIntEquals(tc, 35 * u->number, u->hp);
    CuAssertIntEquals(tc, u->number, get_effect(u, ptype));
    scale_number(u, 8100);
    CuAssertIntEquals(tc, 8100, u->number);
    CuAssertIntEquals(tc, 35 * u->number, u->hp);
    CuAssertIntEquals(tc, u->number, get_effect(u, ptype));
    set_level(u, SK_ALCHEMY, 1);
    scale_number(u, 0);
    CuAssertIntEquals(tc, 0, get_level(u, SK_ALCHEMY));
    test_cleanup();
}

static void test_unit_name(CuTest *tc) {
    unit *u;

    test_setup();
    test_create_world();
    u = test_create_unit(test_create_faction(test_create_race("human")), findregion(0, 0));
    renumber_unit(u, 666);
    unit_setname(u, "Hodor");
    CuAssertStrEquals(tc, "Hodor (ii)", unitname(u));
    test_cleanup();
}

static void test_unit_name_from_race(CuTest *tc) {
    unit *u;

    test_setup();
    u = test_create_unit(test_create_faction(test_create_race("human")), test_create_region(0, 0, NULL));
    renumber_unit(u, 666);
    unit_setname(u, NULL);

    CuAssertStrEquals(tc, "human (ii)", unitname(u));
    CuAssertStrEquals(tc, "human", unit_getname(u));

    u->number = 2;
    CuAssertStrEquals(tc, "human_p (ii)", unitname(u));
    CuAssertStrEquals(tc, "human_p", unit_getname(u));

    test_cleanup();
}

static void test_update_monster_name(CuTest *tc) {
    unit *u;
    race *rc;

    test_setup();
    rc = test_create_race("human");
    u = test_create_unit(test_create_faction(rc), test_create_region(0, 0, NULL));

    unit_setname(u, "Hodor");
    CuAssertTrue(tc, !unit_name_equals_race(u));

    unit_setname(u, "humanitarian");
    CuAssertTrue(tc, !unit_name_equals_race(u));

    unit_setname(u, "huma");
    CuAssertTrue(tc, !unit_name_equals_race(u));

    unit_setname(u, rc_name_s(u->_race, NAME_SINGULAR));
    CuAssertTrue(tc, unit_name_equals_race(u));

    unit_setname(u, rc_name_s(u->_race, NAME_PLURAL));
    CuAssertTrue(tc, unit_name_equals_race(u));

    test_cleanup();
}

static void test_names(CuTest *tc) {
    unit *u;

    test_cleanup();
    test_create_world();
    u = test_create_unit(test_create_faction(test_create_race("human")), findregion(0, 0));

    unit_setname(u, "Hodor");
    unit_setid(u, 5);
    CuAssertStrEquals(tc, "Hodor", unit_getname(u));
    CuAssertStrEquals(tc, "Hodor (5)", unitname(u));
    test_cleanup();
}

static void test_default_name(CuTest *tc) {
    unit *u;
    struct locale* lang;
    char buf[32], compare[32];

    test_setup();

    lang = test_create_locale();
    locale_setstring(lang, "unitdefault", "Zweiheit");

    u = test_create_unit(test_create_faction(NULL), test_create_plain(0, 0));

    default_name(u, buf, sizeof(buf));

    sprintf(compare, "Zweiheit %s", itoa36(u->no));
    CuAssertStrEquals(tc, compare, buf);

    test_cleanup();
}

static int cb_skillmod(const unit *u, const region *r, skill_t sk, int level) {
    UNUSED_ARG(u);
    UNUSED_ARG(r);
    UNUSED_ARG(sk);
    return level + 3;
}

static void test_skillmod(CuTest *tc) {
    unit *u;
    attrib *a;

    test_cleanup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    set_level(u, SK_ARMORER, 5);
    CuAssertIntEquals(tc, 5, effskill(u, SK_ARMORER, 0));

    a_add(&u->attribs, a = make_skillmod(SK_ARMORER, 0, 2.0, 0));
    CuAssertIntEquals(tc, 10, effskill(u, SK_ARMORER, 0));
    a_remove(&u->attribs, a);

    a_add(&u->attribs, a = make_skillmod(NOSKILL, 0, 2.0, 0)); /* NOSKILL means any skill */
    CuAssertIntEquals(tc, 10, effskill(u, SK_ARMORER, 0));
    a_remove(&u->attribs, a);

    a_add(&u->attribs, a = make_skillmod(SK_ARMORER, 0, 0, 2));
    CuAssertIntEquals(tc, 7, effskill(u, SK_ARMORER, 0));
    a_remove(&u->attribs, a);

    a_add(&u->attribs, a = make_skillmod(SK_ARMORER, cb_skillmod, 0, 0));
    CuAssertIntEquals(tc, 8, effskill(u, SK_ARMORER, 0));
    a_remove(&u->attribs, a);

    test_cleanup();
}

static void test_skill_hunger(CuTest *tc) {
    unit *u;

    test_cleanup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    set_level(u, SK_ARMORER, 6);
    set_level(u, SK_SAILING, 6);
    fset(u, UFL_HUNGER);
    CuAssertIntEquals(tc, 3, effskill(u, SK_ARMORER, 0));
    CuAssertIntEquals(tc, 5, effskill(u, SK_SAILING, 0));
    set_level(u, SK_SAILING, 2);
    CuAssertIntEquals(tc, 1, effskill(u, SK_SAILING, 0));
    test_cleanup();
}

static void test_skill_familiar(CuTest *tc) {
    unit *mag, *fam;
    region *r;

    test_cleanup();

    /* setup two units */
    mag = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    fam = test_create_unit(mag->faction, test_create_region(0, 0, 0));
    set_level(fam, SK_PERCEPTION, 6);
    CuAssertIntEquals(tc, 6, effskill(fam, SK_PERCEPTION, 0));
    set_level(mag, SK_PERCEPTION, 6);
    CuAssertIntEquals(tc, 6, effskill(mag, SK_PERCEPTION, 0));

    /* make them mage and familiar to each other */
    create_newfamiliar(mag, fam);

    /* when they are in the same region, the mage gets half their skill as a bonus */
    CuAssertIntEquals(tc, 6, effskill(fam, SK_PERCEPTION, 0));
    CuAssertIntEquals(tc, 9, effskill(mag, SK_PERCEPTION, 0));

    /* when they are further apart, divide bonus by distance */
    r = test_create_region(3, 0, 0);
    move_unit(fam, r, &r->units);
    CuAssertIntEquals(tc, 7, effskill(mag, SK_PERCEPTION, 0));
    test_cleanup();
}

static void test_inside_building(CuTest *tc) {
    unit *u;
    building *b;

    test_setup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    b = test_create_building(u->region, 0);

    b->size = 1;
    scale_number(u, 1);
    CuAssertPtrEquals(tc, 0, inside_building(u));
    u->building = b;
    CuAssertPtrEquals(tc, b, inside_building(u));
    scale_number(u, 2);
    CuAssertPtrEquals(tc, 0, inside_building(u));
    b->size = 2;
    CuAssertPtrEquals(tc, b, inside_building(u));
    u = test_create_unit(u->faction, u->region);
    u->building = b;
    CuAssertPtrEquals(tc, 0, inside_building(u));
    b->size = 3;
    CuAssertPtrEquals(tc, b, inside_building(u));
    test_cleanup();
}

static void test_skills(CuTest *tc) {
    unit *u;
    skill *sv;
    test_setup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    sv = add_skill(u, SK_ALCHEMY);
    CuAssertPtrNotNull(tc, sv);
    CuAssertPtrEquals(tc, sv, u->skills);
    CuAssertIntEquals(tc, 1, u->skill_size);
    CuAssertIntEquals(tc, SK_ALCHEMY, sv->id);
    CuAssertIntEquals(tc, 0, sv->level);
    CuAssertIntEquals(tc, 1, sv->weeks);
    CuAssertIntEquals(tc, 0, sv->old);
    sv = add_skill(u, SK_BUILDING);
    CuAssertPtrNotNull(tc, sv);
    CuAssertIntEquals(tc, 2, u->skill_size);
    CuAssertIntEquals(tc, SK_ALCHEMY, u->skills[0].id);
    CuAssertIntEquals(tc, SK_BUILDING, u->skills[1].id);
    sv = add_skill(u, SK_LONGBOW);
    CuAssertPtrNotNull(tc, sv);
    CuAssertPtrEquals(tc, sv, unit_skill(u, SK_LONGBOW));
    CuAssertIntEquals(tc, 3, u->skill_size);
    CuAssertIntEquals(tc, SK_ALCHEMY, u->skills[0].id);
    CuAssertIntEquals(tc, SK_LONGBOW, u->skills[1].id);
    CuAssertIntEquals(tc, SK_BUILDING, u->skills[2].id);
    CuAssertTrue(tc, !has_skill(u, SK_LONGBOW));
    set_level(u, SK_LONGBOW, 1);
    CuAssertTrue(tc, has_skill(u, SK_LONGBOW));
    remove_skill(u, SK_LONGBOW);
    CuAssertIntEquals(tc, SK_BUILDING, u->skills[1].id);
    CuAssertIntEquals(tc, 2, u->skill_size);
    remove_skill(u, SK_LONGBOW);
    CuAssertIntEquals(tc, SK_BUILDING, u->skills[1].id);
    CuAssertIntEquals(tc, 2, u->skill_size);
    remove_skill(u, SK_BUILDING);
    CuAssertIntEquals(tc, SK_ALCHEMY, u->skills[0].id);
    CuAssertIntEquals(tc, 1, u->skill_size);
    CuAssertTrue(tc, !has_skill(u, SK_LONGBOW));
    test_cleanup();
}

static void test_limited_skills(CuTest *tc) {
    unit *u;
    test_setup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    CuAssertIntEquals(tc, false, has_limited_skills(u));
    set_level(u, SK_ENTERTAINMENT, 1);
    CuAssertIntEquals(tc, false, has_limited_skills(u));
    u->skills->id = SK_ALCHEMY;
    CuAssertIntEquals(tc, true, has_limited_skills(u));
    u->skills->id = SK_MAGIC;
    CuAssertIntEquals(tc, true, has_limited_skills(u));
    u->skills->id = SK_TACTICS;
    CuAssertIntEquals(tc, true, has_limited_skills(u));
    u->skills->id = SK_HERBALISM;
    CuAssertIntEquals(tc, true, has_limited_skills(u));
    u->skills->id = SK_SPY;
    CuAssertIntEquals(tc, true, has_limited_skills(u));
    u->skills->id = SK_TAXING;
    CuAssertIntEquals(tc, false, has_limited_skills(u));
    test_cleanup();
}

static void test_unit_description(CuTest *tc) {
    race *rc;
    unit *u;
    struct locale *lang;

    test_setup();
    lang = test_create_locale();
    rc = test_create_race("hodor");
    u = test_create_unit(test_create_faction(rc), test_create_region(0,0,0));

    CuAssertPtrEquals(tc, 0, u->display);
    CuAssertStrEquals(tc, 0, u_description(u, lang));
    u->display = strdup("Hodor");
    CuAssertStrEquals(tc, "Hodor", u_description(u, NULL));
    CuAssertStrEquals(tc, "Hodor", u_description(u, lang));

    free(u->display);
    u->display = NULL;
    locale_setstring(lang, "describe_hodor", "HODOR");
    CuAssertStrEquals(tc, "HODOR", u_description(u, lang));

    test_cleanup();
}

static void test_remove_unit(CuTest *tc) {
    region *r;
    unit *u1, *u2;
    faction *f;
    int uno;
    const resource_type *rtype;

    test_setup();
    init_resources();
    rtype = get_resourcetype(R_SILVER);
    r = test_create_region(0, 0, 0);
    f = test_create_faction(0);
    u2 = test_create_unit(f, r);
    u1 = test_create_unit(f, r);
    CuAssertPtrEquals(tc, u1, f->units);
    CuAssertPtrEquals(tc, u2, u1->nextF);
    CuAssertPtrEquals(tc, u1, u2->prevF);
    CuAssertPtrEquals(tc, 0, u2->nextF);
    uno = u1->no;
    region_setresource(r, rtype, 0);
    i_change(&u1->items, rtype->itype, 100);
    remove_unit(&r->units, u1);
    CuAssertIntEquals(tc, 0, u1->number);
    CuAssertPtrEquals(tc, 0, u1->region);
    /* money is given to a survivor: */
    CuAssertPtrEquals(tc, 0, u1->items);
    CuAssertIntEquals(tc, 0, region_getresource(r, rtype));
    CuAssertIntEquals(tc, 100, i_get(u2->items, rtype->itype));

    /* unit is removed from f->units: */
    CuAssertPtrEquals(tc, 0, u1->nextF);
    CuAssertPtrEquals(tc, u2, f->units);
    CuAssertPtrEquals(tc, 0, u2->nextF);
    CuAssertPtrEquals(tc, 0, u2->prevF);
    /* unit is no longer in r->units: */
    CuAssertPtrEquals(tc, u2, r->units);
    CuAssertPtrEquals(tc, 0, u2->next);

    /* unit is in deleted_units: */
    CuAssertPtrEquals(tc, 0, findunit(uno));
    CuAssertPtrEquals(tc, f, dfindhash(uno));

    remove_unit(&r->units, u2);
    /* no survivor, give money to peasants: */
    CuAssertIntEquals(tc, 100, region_getresource(r, rtype));
    /* there are now no more units: */
    CuAssertPtrEquals(tc, 0, r->units);
    CuAssertPtrEquals(tc, 0, f->units);
    test_cleanup();
}

static void test_renumber_unit(CuTest *tc) {
    unit *u1, *u2;
    test_setup();
    u1 = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    u2 = test_create_unit(u1->faction, u1->region);
    rng_init(0);
    renumber_unit(u1, 0);
    rng_init(0);
    renumber_unit(u2, 0);
    CuAssertTrue(tc, u1->no != u2->no);
    test_cleanup();
}

static void gen_name(unit *u)
{
    unit_setname(u, "Hodor");
}

static void test_name_unit(CuTest *tc) {
    race *rc;
    unit * u;

    test_setup();
    rc = test_create_race("skeleton");
    u = test_create_unit(test_create_faction(rc), test_create_region(0, 0, 0));
    rc->name_unit = gen_name;
    name_unit(u);
    CuAssertStrEquals(tc, "Hodor", unit_getname(u));
    test_cleanup();
}

static void test_heal_factor(CuTest *tc) {
    unit * u;
    region *r;
    race *rc;
    terrain_type *t_plain;

    test_setup();
    t_plain = test_create_terrain("plain", LAND_REGION|FOREST_REGION);
    rc = rc_get_or_create("human");
    u = test_create_unit(test_create_faction(rc), r = test_create_region(0, 0, t_plain));
    rsettrees(r, 1, r->terrain->size / TREESIZE);
    rsettrees(r, 2, 0);
    CuAssertTrue(tc, r_isforest(r));
    CuAssertDblEquals(tc, 1.0, u_heal_factor(u), 0.0);
    rc->healing = 200;
    CuAssertDblEquals(tc, 2.0, u_heal_factor(u), 0.0);
    rc->healing = 0;
    rc = rc_get_or_create("elf");
    CuAssertPtrEquals(tc, (void *)rc, (void *)get_race(RC_ELF));
    u_setrace(u, get_race(RC_ELF));
    CuAssertDblEquals(tc, 1.0, u_heal_factor(u), 0.0);
    config_set("healing.forest", "1.5");
    CuAssertDblEquals(tc, 1.5, u_heal_factor(u), 0.0);
    test_cleanup();
}

static void test_unlimited_units(CuTest *tc) {
    race *rc;
    faction *f;
    unit *u;

    test_setup();
    f = test_create_faction(NULL);
    rc = test_create_race("spell");
    rc->flags |= RCF_INVISIBLE;
    CuAssertIntEquals(tc, 0, f->num_units);
    CuAssertIntEquals(tc, 0, f->num_people);
    u = test_create_unit(f, test_create_region(0, 0, NULL));
    CuAssertTrue(tc, count_unit(u));
    CuAssertIntEquals(tc, 1, f->num_units);
    CuAssertIntEquals(tc, 1, f->num_people);
    u_setfaction(u, NULL);
    CuAssertIntEquals(tc, 0, f->num_units);
    CuAssertIntEquals(tc, 0, f->num_people);
    u_setfaction(u, f);
    CuAssertIntEquals(tc, 1, f->num_units);
    CuAssertIntEquals(tc, 1, f->num_people);
    u_setrace(u, rc);
    CuAssertTrue(tc, !count_unit(u));
    CuAssertIntEquals(tc, 0, f->num_units);
    CuAssertIntEquals(tc, 0, f->num_people);
    scale_number(u, 10);
    CuAssertIntEquals(tc, 0, f->num_units);
    CuAssertIntEquals(tc, 0, f->num_people);
    u_setrace(u, f->race);
    CuAssertIntEquals(tc, 1, f->num_units);
    CuAssertIntEquals(tc, 10, f->num_people);
    remove_unit(&u->region->units, u);
    CuAssertIntEquals(tc, 0, f->num_units);
    CuAssertIntEquals(tc, 0, f->num_people);
    test_cleanup();
}

static void test_clone_men_bug_2386(CuTest *tc) {
    unit *u1, *u2;
    region *r;
    faction *f;

    test_setup();
    r = test_create_region(0, 0, NULL);
    f = test_create_faction(NULL);
    u1 = test_create_unit(f, r);
    scale_number(u1, 8237);
    u1->hp = 39 * u1->number;
    u2 = test_create_unit(f, r);
    scale_number(u2, 0);
    clone_men(u1, u2, 8100);
    CuAssertIntEquals(tc, 8100, u2->number);
    CuAssertIntEquals(tc, u2->number * 39, u2->hp);
    test_cleanup();
}

static void test_clone_men(CuTest *tc) {
    unit *u1, *u2;
    region *r;
    faction *f;
    test_setup();
    r = test_create_region(0, 0, NULL);
    f = test_create_faction(NULL);
    u1 = test_create_unit(f, r);
    scale_number(u1, 10);
    u2 = test_create_unit(f, r);
    scale_number(u2, 0);
    CuAssertIntEquals(tc, 10, u1->number);
    CuAssertIntEquals(tc, 200, u1->hp);
    CuAssertIntEquals(tc, 0, u2->number);
    CuAssertIntEquals(tc, 0, u2->hp);
    clone_men(u1, u2, 1);
    CuAssertIntEquals(tc, 10, u1->number);
    CuAssertIntEquals(tc, 200, u1->hp);
    CuAssertIntEquals(tc, 1, u2->number);
    CuAssertIntEquals(tc, 20, u2->hp);
    test_cleanup();
}

static void test_transfermen(CuTest *tc) {
    unit *u1, *u2;
    region *r;
    faction *f;
    test_setup();
    r = test_create_region(0, 0, NULL);
    f = test_create_faction(NULL);
    u1 = test_create_unit(f, r);
    scale_number(u1, 3500);
    u2 = test_create_unit(f, r);
    scale_number(u2, 3500);
    CuAssertIntEquals(tc, 70000, u1->hp);
    CuAssertIntEquals(tc, 70000, u2->hp);
    transfermen(u1, u2, u1->number);
    CuAssertIntEquals(tc, 7000, u2->number);
    CuAssertIntEquals(tc, 140000, u2->hp);
    CuAssertIntEquals(tc, 0, u1->number);
    CuAssertIntEquals(tc, 0, u1->hp);
    test_cleanup();
}

CuSuite *get_unit_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_scale_number);
    SUITE_ADD_TEST(suite, test_unit_description);
    SUITE_ADD_TEST(suite, test_unit_name);
    SUITE_ADD_TEST(suite, test_unit_name_from_race);
    SUITE_ADD_TEST(suite, test_update_monster_name);
    SUITE_ADD_TEST(suite, test_clone_men);
    SUITE_ADD_TEST(suite, test_clone_men_bug_2386);
    SUITE_ADD_TEST(suite, test_transfermen);
    SUITE_ADD_TEST(suite, test_clone_men_bug_2386);
    SUITE_ADD_TEST(suite, test_transfermen);
    SUITE_ADD_TEST(suite, test_remove_unit);
    SUITE_ADD_TEST(suite, test_remove_empty_units);
    SUITE_ADD_TEST(suite, test_remove_units_without_faction);
    SUITE_ADD_TEST(suite, test_remove_units_with_dead_faction);
    SUITE_ADD_TEST(suite, test_remove_empty_units_in_region);
    SUITE_ADD_TEST(suite, test_names);
    SUITE_ADD_TEST(suite, test_unlimited_units);
    SUITE_ADD_TEST(suite, test_default_name);
    SUITE_ADD_TEST(suite, test_skillmod);
    SUITE_ADD_TEST(suite, test_skill_hunger);
    SUITE_ADD_TEST(suite, test_skill_familiar);
    SUITE_ADD_TEST(suite, test_inside_building);
    SUITE_ADD_TEST(suite, test_skills);
    SUITE_ADD_TEST(suite, test_limited_skills);
    SUITE_ADD_TEST(suite, test_renumber_unit);
    SUITE_ADD_TEST(suite, test_name_unit);
    SUITE_ADD_TEST(suite, test_heal_factor);
    return suite;
}
