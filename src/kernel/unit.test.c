#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "unit.h"

#include "ally.h"
#include "attrib.h"
#include "building.h"
#include "config.h"
#include "faction.h"
#include "item.h"
#include "magic.h"
#include "race.h"
#include "region.h"
#include "skill.h"
#include "skills.h"
#include "terrain.h"

#include <util/base36.h>
#include <util/language.h>
#include <util/macros.h>
#include <util/rng.h>

#include <tests.h>

#include <stb_ds.h>
#include <CuTest.h>

#include <stdbool.h>
#include <stdio.h>

static void test_remove_empty_units(CuTest *tc) {
    unit *u;
    int uid;

    test_setup();
    test_create_world();

    u = test_create_unit(test_create_faction(), findregion(0, 0));
    uid = u->no;
    remove_empty_units();
    CuAssertPtrNotNull(tc, findunit(uid));
    u->number = 0;
    remove_empty_units();
    CuAssertPtrEquals(tc, NULL, findunit(uid));
    test_teardown();
}

static void test_remove_empty_units_in_region(CuTest *tc) {
    unit *u;
    int uid;

    test_setup();
    test_create_world();

    u = test_create_unit(test_create_faction(), findregion(0, 0));
    u = test_create_unit(u->faction, u->region);
    CuAssertPtrNotNull(tc, u->nextF);
    uid = u->no;
    remove_empty_units_in_region(u->region);
    CuAssertPtrNotNull(tc, findunit(uid));
    u->number = 0;
    remove_empty_units_in_region(u->region);
    CuAssertPtrEquals(tc, NULL, findunit(uid));
    CuAssertPtrEquals(tc, NULL, u->nextF);
    CuAssertPtrEquals(tc, NULL, u->region);
    test_teardown();
}

static void test_remove_units_without_faction(CuTest *tc) {
    unit *u;
    int uid;

    test_setup();
    test_create_world();

    u = test_create_unit(test_create_faction(), findregion(0, 0));
    uid = u->no;
    u_setfaction(u, 0);
    remove_empty_units_in_region(u->region);
    CuAssertPtrEquals(tc, NULL, findunit(uid));
    CuAssertIntEquals(tc, 0, u->number);
    test_teardown();
}

static void test_remove_units_with_dead_faction(CuTest *tc) {
    unit *u;
    int uid;

    test_setup();
    test_create_world();

    u = test_create_unit(test_create_faction(), findregion(0, 0));
    uid = u->no;
    u->faction->_alive = false;
    remove_empty_units_in_region(u->region);
    CuAssertPtrEquals(tc, NULL, findunit(uid));
    CuAssertIntEquals(tc, 0, u->number);
    test_teardown();
}

static void test_scale_number(CuTest *tc) {
    unit *u;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u->hp = 35;
    CuAssertIntEquals(tc, 1, u->number);
    CuAssertIntEquals(tc, 35, u->hp);
    scale_number(u, 2);
    CuAssertIntEquals(tc, 2, u->number);
    CuAssertIntEquals(tc, 35 * u->number, u->hp);
    scale_number(u, 8237);
    CuAssertIntEquals(tc, 8237, u->number);
    CuAssertIntEquals(tc, 35 * u->number, u->hp);
    scale_number(u, 8100);
    CuAssertIntEquals(tc, 8100, u->number);
    CuAssertIntEquals(tc, 35 * u->number, u->hp);
    set_level(u, SK_ALCHEMY, 1);
    scale_number(u, 0);
    CuAssertIntEquals(tc, 0, get_level(u, SK_ALCHEMY));
    test_teardown();
}

static void test_unit_name(CuTest *tc) {
    unit *u;

    test_setup();
    test_create_world();
    u = test_create_unit(test_create_faction(), findregion(0, 0));
    renumber_unit(u, 666);
    unit_setname(u, "Hodor");
    CuAssertStrEquals(tc, "Hodor (ii)", unitname(u));
    test_teardown();
}

static void test_unit_name_from_race(CuTest *tc) {
    unit *u;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    renumber_unit(u, 666);
    unit_setname(u, NULL);

    CuAssertStrEquals(tc, "human (ii)", unitname(u));
    CuAssertStrEquals(tc, "human", unit_getname(u));

    u->number = 2;
    CuAssertStrEquals(tc, "human_p (ii)", unitname(u));
    CuAssertStrEquals(tc, "human_p", unit_getname(u));

    test_teardown();
}

static void test_update_monster_name(CuTest *tc) {
    unit *u;
    race *rc;

    test_setup();
    rc = test_create_race("human");
    u = test_create_unit(test_create_faction_ex(rc, NULL), test_create_plain(0, 0));

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

    test_teardown();
}

static void test_names(CuTest *tc) {
    unit *u;

    test_setup();
    test_create_world();
    u = test_create_unit(test_create_faction(), findregion(0, 0));

    unit_setname(u, "Hodor");
    unit_setid(u, 5);
    CuAssertStrEquals(tc, "Hodor", unit_getname(u));
    CuAssertStrEquals(tc, "Hodor (5)", unitname(u));
    test_teardown();
}

static void test_default_name(CuTest *tc) {
    unit *u;
    struct locale* lang;
    char buf[32], compare[32];

    test_setup();

    lang = get_or_create_locale(__FUNCTION__);
    locale_setstring(lang, "unitdefault", "Zweiheit");

    u = test_create_unit(test_create_faction_ex(NULL, lang), test_create_plain(0, 0));

    default_name(u, buf, sizeof(buf));

    sprintf(compare, "Zweiheit %s", itoa36(u->no));
    CuAssertStrEquals(tc, compare, buf);

    test_teardown();
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

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    set_level(u, SK_ARMORER, 5);
    CuAssertIntEquals(tc, 5, effskill(u, SK_ARMORER, NULL));

    a_add(&u->attribs, a = make_skillmod(SK_ARMORER, 0, 2.0, 0));
    CuAssertIntEquals(tc, 10, effskill(u, SK_ARMORER, NULL));
    a_remove(&u->attribs, a);

    a_add(&u->attribs, a = make_skillmod(NOSKILL, 0, 2.0, 0)); /* NOSKILL means any skill */
    CuAssertIntEquals(tc, 10, effskill(u, SK_ARMORER, NULL));
    a_remove(&u->attribs, a);

    a_add(&u->attribs, a = make_skillmod(SK_ARMORER, 0, 0, 2));
    CuAssertIntEquals(tc, 7, effskill(u, SK_ARMORER, NULL));
    a_remove(&u->attribs, a);

    a_add(&u->attribs, a = make_skillmod(SK_ARMORER, cb_skillmod, 0, 0));
    CuAssertIntEquals(tc, 8, effskill(u, SK_ARMORER, NULL));
    a_remove(&u->attribs, a);

    test_teardown();
}

static void test_skill_hunger(CuTest *tc) {
    unit *u;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    set_level(u, SK_ARMORER, 6);
    set_level(u, SK_SAILING, 6);
    fset(u, UFL_HUNGER);
    CuAssertIntEquals(tc, 3, effskill(u, SK_ARMORER, NULL));
    CuAssertIntEquals(tc, 5, effskill(u, SK_SAILING, NULL));
    set_level(u, SK_SAILING, 2);
    CuAssertIntEquals(tc, 1, effskill(u, SK_SAILING, NULL));
    test_teardown();
}

static void test_skill_familiar(CuTest *tc) {
    unit *mag, *fam;
    region *r;

    test_setup();

    /* setup two units */
    mag = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    fam = test_create_unit(mag->faction, test_create_plain(0, 0));
    set_level(fam, SK_PERCEPTION, 6);
    CuAssertIntEquals(tc, 6, effskill(fam, SK_PERCEPTION, NULL));
    set_level(mag, SK_PERCEPTION, 6);
    CuAssertIntEquals(tc, 6, effskill(mag, SK_PERCEPTION, NULL));

    /* make them mage and familiar to each other */
    create_newfamiliar(mag, fam);

    /* when they are in the same region, the mage gets half their skill as a bonus */
    CuAssertIntEquals(tc, 6, effskill(fam, SK_PERCEPTION, NULL));
    CuAssertIntEquals(tc, 9, effskill(mag, SK_PERCEPTION, NULL));

    /* when they are further apart, divide bonus by distance */
    r = test_create_region(3, 0, 0);
    move_unit(fam, r, &r->units);
    CuAssertIntEquals(tc, 7, effskill(mag, SK_PERCEPTION, NULL));
    test_teardown();
}

static void test_inside_building(CuTest *tc) {
    unit *u;
    building *b;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    b = test_create_building(u->region, NULL);

    b->size = 1;
    scale_number(u, 1);
    CuAssertPtrEquals(tc, NULL, inside_building(u));
    u->building = b;
    CuAssertPtrEquals(tc, b, inside_building(u));
    scale_number(u, 2);
    CuAssertPtrEquals(tc, NULL, inside_building(u));
    b->size = 2;
    CuAssertPtrEquals(tc, b, inside_building(u));
    u = test_create_unit(u->faction, u->region);
    u->building = b;
    CuAssertPtrEquals(tc, NULL, inside_building(u));
    b->size = 3;
    CuAssertPtrEquals(tc, b, inside_building(u));
    test_teardown();
}

static void test_skills(CuTest *tc) {
    unit *u;
    skill *sv;
    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    sv = add_skill(u, SK_ALCHEMY);
    CuAssertPtrNotNull(tc, sv);
    CuAssertPtrEquals(tc, sv, u->skills);
    CuAssertIntEquals(tc, 1, (int)arrlen(u->skills));
    CuAssertIntEquals(tc, SK_ALCHEMY, sv->id);
    CuAssertIntEquals(tc, 0, sv->level);
    CuAssertIntEquals(tc, 1, sv->weeks);
    CuAssertIntEquals(tc, 0, sv->old);
    sv = add_skill(u, SK_BUILDING);
    CuAssertPtrNotNull(tc, sv);
    CuAssertIntEquals(tc, 2, (int)arrlen(u->skills));
    CuAssertIntEquals(tc, SK_ALCHEMY, u->skills[0].id);
    CuAssertIntEquals(tc, SK_BUILDING, u->skills[1].id);
    sv = add_skill(u, SK_LONGBOW);
    CuAssertPtrNotNull(tc, sv);
    CuAssertPtrEquals(tc, sv, unit_skill(u, SK_LONGBOW));
    CuAssertIntEquals(tc, 3, (int)arrlen(u->skills));
    CuAssertIntEquals(tc, SK_ALCHEMY, u->skills[0].id);
    CuAssertIntEquals(tc, SK_LONGBOW, u->skills[1].id);
    CuAssertIntEquals(tc, SK_BUILDING, u->skills[2].id);
    CuAssertTrue(tc, !has_skill(u, SK_LONGBOW));
    set_level(u, SK_LONGBOW, 1);
    CuAssertTrue(tc, has_skill(u, SK_LONGBOW));
    remove_skill(u, SK_LONGBOW);
    CuAssertIntEquals(tc, SK_BUILDING, u->skills[1].id);
    CuAssertIntEquals(tc, 2, (int)arrlen(u->skills));
    remove_skill(u, SK_LONGBOW);
    CuAssertIntEquals(tc, SK_BUILDING, u->skills[1].id);
    CuAssertIntEquals(tc, 2, (int)arrlen(u->skills));
    remove_skill(u, SK_BUILDING);
    CuAssertIntEquals(tc, SK_ALCHEMY, u->skills[0].id);
    CuAssertIntEquals(tc, 1, (int)arrlen(u->skills));
    CuAssertTrue(tc, !has_skill(u, SK_LONGBOW));
    test_teardown();
}

static void test_limited_skills(CuTest *tc) {
    unit *u;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
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
    test_teardown();
}

static void test_unit_description(CuTest *tc) {
    race *rc;
    unit *u;
    struct locale *lang;

    test_setup();
    lang = test_create_locale();
    rc = test_create_race("hodor");
    u = test_create_unit(test_create_faction_ex(rc, NULL), test_create_plain(0, 0));

    CuAssertStrEquals(tc, NULL, unit_getinfo(u));
    CuAssertStrEquals(tc, NULL, u_description(u, lang));
    unit_setinfo(u, "Hodor");
    CuAssertStrEquals(tc, "Hodor", u_description(u, NULL));
    CuAssertStrEquals(tc, "Hodor", u_description(u, lang));

    unit_setinfo(u, NULL);
    locale_setstring(lang, "describe_hodor", "HODOR");
    CuAssertStrEquals(tc, "HODOR", u_description(u, lang));

    test_teardown();
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
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u2 = test_create_unit(f, r);
    u1 = test_create_unit(f, r);
    CuAssertPtrEquals(tc, u1, f->units);
    CuAssertPtrEquals(tc, u2, u1->nextF);
    CuAssertPtrEquals(tc, u1, u2->prevF);
    CuAssertPtrEquals(tc, NULL, u2->nextF);
    uno = u1->no;
    region_setresource(r, rtype, 0);
    i_change(&u1->items, rtype->itype, 100);
    remove_unit(&r->units, u1);
    CuAssertIntEquals(tc, 0, u1->number);
    CuAssertPtrEquals(tc, NULL, u1->region);
    /* money is given to a survivor: */
    CuAssertPtrEquals(tc, NULL, u1->items);
    CuAssertIntEquals(tc, 0, region_getresource(r, rtype));
    CuAssertIntEquals(tc, 100, i_get(u2->items, rtype->itype));

    /* unit is removed from f->units: */
    CuAssertPtrEquals(tc, NULL, u1->nextF);
    CuAssertPtrEquals(tc, u2, f->units);
    CuAssertPtrEquals(tc, NULL, u2->nextF);
    CuAssertPtrEquals(tc, NULL, u2->prevF);
    /* unit is no longer in r->units: */
    CuAssertPtrEquals(tc, u2, r->units);
    CuAssertPtrEquals(tc, NULL, u2->next);

    /* unit is in deleted_units: */
    CuAssertPtrEquals(tc, NULL, findunit(uno));
    CuAssertPtrEquals(tc, f, dfindhash(uno));

    remove_unit(&r->units, u2);
    /* no survivor, give money to peasants: */
    CuAssertIntEquals(tc, 100, region_getresource(r, rtype));
    /* there are now no more units: */
    CuAssertPtrEquals(tc, NULL, r->units);
    CuAssertPtrEquals(tc, NULL, f->units);
    test_teardown();
}

static void test_renumber_unit(CuTest *tc) {
    unit *u1, *u2;

    test_setup();
    u1 = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u2 = test_create_unit(u1->faction, u1->region);
    rng_init(0);
    renumber_unit(u1, 0);
    rng_init(0);
    renumber_unit(u2, 0);
    CuAssertTrue(tc, u1->no != u2->no);
    test_teardown();
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
    u = test_create_unit(test_create_faction_ex(rc, NULL), test_create_plain(0, 0));
    rc->name_unit = gen_name;
    name_unit(u);
    CuAssertStrEquals(tc, "Hodor", unit_getname(u));
    test_teardown();
}

static void test_heal_factor(CuTest *tc) {
    unit * u;
    region *r;
    race *rc;
    terrain_type *t_plain;

    test_setup();
    t_plain = test_create_terrain("plain", LAND_REGION|FOREST_REGION);
    rc = rc_get_or_create("human");
    u = test_create_unit(test_create_faction_ex(rc, NULL), r = test_create_region(0, 0, t_plain));
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
    test_teardown();
}

static void test_unlimited_units(CuTest *tc) {
    faction *f;
    unit *u;

    test_setup();
    f = test_create_faction();
    CuAssertIntEquals(tc, 0, f->num_units);
    CuAssertIntEquals(tc, 0, f->num_people);
    u = test_create_unit(f, test_create_plain(0, 0));
    CuAssertTrue(tc, count_unit(u));
    CuAssertIntEquals(tc, 1, f->num_units);
    CuAssertIntEquals(tc, 1, f->num_people);
    u_setfaction(u, NULL);
    CuAssertIntEquals(tc, 0, f->num_units);
    CuAssertIntEquals(tc, 0, f->num_people);
    u_setfaction(u, f);
    CuAssertIntEquals(tc, 1, f->num_units);
    CuAssertIntEquals(tc, 1, f->num_people);
    scale_number(u, 10);
    CuAssertIntEquals(tc, 1, f->num_units);
    CuAssertIntEquals(tc, 10, f->num_people);
    remove_unit(&u->region->units, u);
    CuAssertIntEquals(tc, 0, f->num_units);
    CuAssertIntEquals(tc, 0, f->num_people);
    test_teardown();
}

static void test_clone_men_bug_2386(CuTest *tc) {
    unit *u1, *u2;
    region *r;
    faction *f;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    scale_number(u1, 8237);
    u1->hp = 39 * u1->number;
    u2 = test_create_unit(f, r);
    scale_number(u2, 0);
    clone_men(u1, u2, 8100);
    CuAssertIntEquals(tc, 8100, u2->number);
    CuAssertIntEquals(tc, u2->number * 39, u2->hp);
    test_teardown();
}

static void test_clone_men(CuTest *tc) {
    unit *u1, *u2;
    region *r;
    faction *f;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
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
    test_teardown();
}

static void test_transfer_hitpoints(CuTest *tc) {
    unit *u1, *u2;
    region *r;
    faction *f;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction();
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
    test_teardown();
}

/**
 * A transfer of men between two units with the same skill 
 * does not change their skills.
 */
static void test_transfer_skills(CuTest *tc) {
    unit *u1, *u2;
    region *r;
    faction *f;
    skill *sv;

    test_setup();
    config_set_int("study.random_progress", 0);
    r = test_create_plain(0, 0);
    f = test_create_faction();
    u1 = test_create_unit(f, r);
    scale_number(u1, 200);
    set_level(u1, SK_ALCHEMY, 2);
    sv = unit_skill(u1, SK_ALCHEMY);
    CuAssertIntEquals(tc, 2, sv->level);
    CuAssertIntEquals(tc, 3, sv->weeks);
    u2 = test_create_unit(f, r);
    scale_number(u2, 100);
    set_level(u2, SK_ALCHEMY, 2);
    transfermen(u1, u2, 100);
    CuAssertIntEquals(tc, 100, u1->number);
    CuAssertIntEquals(tc, 2, effskill(u1, SK_ALCHEMY, NULL));
    CuAssertIntEquals(tc, 200, u2->number);
    CuAssertIntEquals(tc, 2, effskill(u2, SK_ALCHEMY, NULL));
    sv = unit_skill(u2, SK_ALCHEMY);
    CuAssertIntEquals(tc, 2, sv->level);
    CuAssertIntEquals(tc, 3, sv->weeks);
    test_teardown();
}

/**
 * A transfer of men between two units with different progress
 * merges their skill progress.
 */
static void test_transfer_skills_merge(CuTest *tc) {
    unit *u1, *u2, *u3;
    region *r;
    faction *f;
    skill *sv;

    test_setup();
    config_set_int("study.random_progress", 0);
    r = test_create_plain(0, 0);
    f = test_create_faction();

    u1 = test_create_unit(f, r);
    scale_number(u1, 5);

    u2 = test_create_unit(f, r);
    set_level(u2, SK_ALCHEMY, 2);
    u3 = test_create_unit(f, r);
    set_level(u2, SK_ALCHEMY, 2);

    transfermen(u1, u2, 2);
    CuAssertIntEquals(tc, 3, u1->number);
    CuAssertIntEquals(tc, 3, u2->number);
    sv = unit_skill(u2, SK_ALCHEMY);
    CuAssertIntEquals(tc, 1, sv->level);

    transfermen(u1, u3, 3);
    CuAssertIntEquals(tc, 0, u1->number);
    CuAssertIntEquals(tc, 4, u3->number);
    sv = unit_skill(u3, SK_ALCHEMY);
    CuAssertPtrEquals(tc, NULL, sv);

    test_teardown();
}

static void test_get_modifier(CuTest *tc) {
    race * rc;
    region *r;
    unit *u;
    terrain_type *t_plain, *t_desert;

    test_setup();
    t_desert = test_create_terrain("desert", -1);
    t_plain = test_create_terrain("plain", -1);
    rc = test_create_race("insect");
    rc->bonus[SK_ARMORER] = 1;
    rc->bonus[SK_TAXING] = 0;
    rc->bonus[SK_TRADE] = -1;
    u = test_create_unit(test_create_faction_ex(rc, NULL), r = test_create_region(0, 0, t_plain));

    /* no effects for insects in plains: */
    CuAssertIntEquals(tc, 0, get_modifier(u, SK_TAXING, 0, r, true));
    CuAssertIntEquals(tc, 0, get_modifier(u, SK_TAXING, 1, r, true));
    CuAssertIntEquals(tc, 0, get_modifier(u, SK_TAXING, 7, r, true));

    r = test_create_region(1, 0, t_desert);
    /* terrain effect: +1 for insects in deserts: */
    CuAssertIntEquals(tc, 1, get_modifier(u, SK_TAXING, 1, r, true));
    /* no terrain effects if no region is given: */
    CuAssertIntEquals(tc, 0, get_modifier(u, SK_TAXING, 1, NULL, true));
    /* only get terrain effect if they have at least level 1: */
    CuAssertIntEquals(tc, 0, get_modifier(u, SK_TAXING, 0, r, true));

    test_teardown();
}

static void test_maintenance_cost(CuTest *tc) {
    unit *u;
    race *rc;
    test_setup();
    CuAssertIntEquals(tc, 10, maintenance_cost(NULL));
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    CuAssertIntEquals(tc, 10, maintenance_cost(u));
    u->number = 4;
    CuAssertIntEquals(tc, 40, maintenance_cost(u));
    rc = test_create_race("smurf");
    rc->maintenance = 15;
    u_setrace(u, rc);
    CuAssertIntEquals(tc, 60, maintenance_cost(u));
    u->faction->flags |= FFL_PAUSED;
    CuAssertIntEquals(tc, 0, maintenance_cost(u));
    test_teardown();
}

static void test_gift_items(CuTest *tc) {
    unit *u, *u1, *u2;
    region *r;
    const resource_type *rtype;
    test_setup();
    init_resources();
    r = test_create_plain(0, 0);
    u = test_create_unit(test_create_faction(), r);
    rtype = get_resourcetype(R_SILVER);
    region_setresource(r, rtype, 0);
    i_change(&u->items, rtype->itype, 10);
    gift_items(u, GIFT_FRIENDS | GIFT_PEASANTS | GIFT_SELF);
    CuAssertIntEquals(tc, 10, region_getresource(r, rtype));
    CuAssertIntEquals(tc, 0, i_get(u->items, rtype->itype));

    region_setresource(r, get_resourcetype(R_HORSE), 0);
    region_setresource(r, rtype, 0);
    i_change(&u->items, rtype->itype, 10);
    i_change(&u->items, get_resourcetype(R_HORSE)->itype, 20);
    u1 = test_create_unit(test_create_faction(), r);
    u2 = test_create_unit(u1->faction, r);
    gift_items(u, GIFT_FRIENDS | GIFT_PEASANTS | GIFT_SELF);
    CuAssertIntEquals(tc, 20, region_getresource(r, get_resourcetype(R_HORSE)));
    CuAssertIntEquals(tc, 10, region_getresource(r, rtype));
    CuAssertIntEquals(tc, 0, i_get(u->items, rtype->itype));
    CuAssertIntEquals(tc, 0, i_get(u1->items, rtype->itype));
    CuAssertIntEquals(tc, 0, i_get(u2->items, rtype->itype));

    region_setresource(r, rtype, 0);
    i_change(&u->items, rtype->itype, 10);
    ally_set(&u->faction->allies, u1->faction, HELP_MONEY);
    ally_set(&u1->faction->allies, u->faction, HELP_GIVE);
    gift_items(u, GIFT_FRIENDS | GIFT_PEASANTS | GIFT_SELF);
    CuAssertIntEquals(tc, 0, region_getresource(r, rtype));
    CuAssertIntEquals(tc, 0, i_get(u->items, rtype->itype));
    CuAssertIntEquals(tc, 10, i_get(u1->items, rtype->itype));
    CuAssertIntEquals(tc, 0, i_get(u2->items, rtype->itype));
    i_change(&u1->items, rtype->itype, -10);

    set_number(u1, 2);
    u_setfaction(u2, test_create_faction());
    ally_set(&u->faction->allies, u2->faction, HELP_MONEY);
    ally_set(&u2->faction->allies, u->faction, HELP_GIVE);
    region_setresource(r, rtype, 0);
    i_change(&u->items, rtype->itype, 15);
    ally_set(&u->faction->allies, u1->faction, HELP_MONEY);
    ally_set(&u1->faction->allies, u->faction, HELP_GIVE);
    gift_items(u, GIFT_FRIENDS | GIFT_PEASANTS | GIFT_SELF);
    CuAssertIntEquals(tc, 0, region_getresource(r, rtype));
    CuAssertIntEquals(tc, 0, i_get(u->items, rtype->itype));
    CuAssertIntEquals(tc, 10, i_get(u1->items, rtype->itype));
    CuAssertIntEquals(tc, 5, i_get(u2->items, rtype->itype));

    test_teardown();
}

static void test_max_heroes(CuTest* tc) {
    CuAssertIntEquals(tc, 0, max_heroes(56));
    CuAssertIntEquals(tc, 1, max_heroes(57));
    CuAssertIntEquals(tc, 1, max_heroes(62));
    CuAssertIntEquals(tc, 2, max_heroes(63));
    CuAssertIntEquals(tc, 2, max_heroes(70));
    CuAssertIntEquals(tc, 3, max_heroes(71));
    config_set_int("rules.heroes.offset", 500);
    CuAssertIntEquals(tc, 0, max_heroes(556));
    CuAssertIntEquals(tc, 1, max_heroes(557));
    CuAssertIntEquals(tc, 1, max_heroes(562));
    CuAssertIntEquals(tc, 2, max_heroes(563));
    CuAssertIntEquals(tc, 2, max_heroes(570));
    CuAssertIntEquals(tc, 3, max_heroes(571));
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
    SUITE_ADD_TEST(suite, test_transfer_hitpoints);
    SUITE_ADD_TEST(suite, test_transfer_skills);
    SUITE_ADD_TEST(suite, test_transfer_skills_merge);
    SUITE_ADD_TEST(suite, test_clone_men_bug_2386);
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
    SUITE_ADD_TEST(suite, test_get_modifier);
    SUITE_ADD_TEST(suite, test_gift_items);
    SUITE_ADD_TEST(suite, test_maintenance_cost);
    SUITE_ADD_TEST(suite, test_max_heroes);
    return suite;
}
