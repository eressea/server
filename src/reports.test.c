#include <platform.h>
#include <config.h>
#include "reports.h"

#include "move.h"
#include "lighthouse.h"
#include "travelthru.h"
#include "keyword.h"

#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>
#include <kernel/spell.h>
#include <kernel/spellbook.h>
#include <kernel/terrain.h>

#include <util/language.h>
#include <util/lists.h>
#include <util/message.h>

#include <quicklist.h>
#include <stream.h>
#include <memstream.h>

#include <CuTest.h>
#include <tests.h>

#include <string.h>

static void test_reorder_units(CuTest * tc)
{
    region *r;
    building *b;
    ship * s;
    unit *u0, *u1, *u2, *u3, *u4;
    struct faction * f;
    const building_type *btype;
    const ship_type *stype;

    test_cleanup();
    test_create_world();

    btype = bt_find("castle");
    stype = st_find("boat");

    r = findregion(-1, 0);
    b = test_create_building(r, btype);
    s = test_create_ship(r, stype);
    f = test_create_faction(0);

    u0 = test_create_unit(f, r);
    u_set_ship(u0, s);
    u1 = test_create_unit(f, r);
    u_set_ship(u1, s);
    ship_set_owner(u1);
    u2 = test_create_unit(f, r);
    u3 = test_create_unit(f, r);
    u_set_building(u3, b);
    u4 = test_create_unit(f, r);
    u_set_building(u4, b);
    building_set_owner(u4);

    reorder_units(r);

    CuAssertPtrEquals(tc, u4, r->units);
    CuAssertPtrEquals(tc, u3, u4->next);
    CuAssertPtrEquals(tc, u2, u3->next);
    CuAssertPtrEquals(tc, u1, u2->next);
    CuAssertPtrEquals(tc, u0, u1->next);
    CuAssertPtrEquals(tc, 0, u0->next);
    test_cleanup();
}

static void test_regionid(CuTest * tc) {
    size_t len;
    const struct terrain_type * plain;
    struct region * r;
    char buffer[64];

    test_cleanup();
    plain = test_create_terrain("plain", 0);
    r = test_create_region(0, 0, plain);

    memset(buffer, 0x7d, sizeof(buffer));
    len = f_regionid(r, 0, buffer, sizeof(buffer));
    CuAssertIntEquals(tc, 11, (int)len);
    CuAssertStrEquals(tc, "plain (0,0)", buffer);

    memset(buffer, 0x7d, sizeof(buffer));
    len = f_regionid(r, 0, buffer, 11);
    CuAssertIntEquals(tc, 10, (int)len);
    CuAssertStrEquals(tc, "plain (0,0", buffer);
    CuAssertIntEquals(tc, 0x7d, buffer[11]);
    test_cleanup();
}

static void test_seen_faction(CuTest *tc) {
    faction *f1, *f2;
    race *rc;

    test_cleanup();
    rc = test_create_race("human");
    f1 = test_create_faction(rc);
    f2 = test_create_faction(rc);
    add_seen_faction(f1, f2);
    CuAssertPtrEquals(tc, f2, ql_get(f1->seen_factions, 0));
    CuAssertIntEquals(tc, 1, ql_length(f1->seen_factions));
    add_seen_faction(f1, f2);
    CuAssertIntEquals(tc, 1, ql_length(f1->seen_factions));
    add_seen_faction(f1, f1);
    CuAssertIntEquals(tc, 2, ql_length(f1->seen_factions));
    f2 = (faction *)ql_get(f1->seen_factions, 1);
    f1 = (faction *)ql_get(f1->seen_factions, 0);
    CuAssertTrue(tc, f1->no < f2->no);
    test_cleanup();
}

static void test_sparagraph(CuTest *tc) {
    strlist *sp = 0;

    split_paragraph(&sp, "Hello World", 0, 16, 0);
    CuAssertPtrNotNull(tc, sp);
    CuAssertStrEquals(tc, "Hello World", sp->s);
    CuAssertPtrEquals(tc, 0, sp->next);
    freestrlist(sp);

    split_paragraph(&sp, "Hello World", 4, 16, 0);
    CuAssertPtrNotNull(tc, sp);
    CuAssertStrEquals(tc, "    Hello World", sp->s);
    CuAssertPtrEquals(tc, 0, sp->next);
    freestrlist(sp);

    split_paragraph(&sp, "Hello World", 4, 16, '*');
    CuAssertPtrNotNull(tc, sp);
    CuAssertStrEquals(tc, "  * Hello World", sp->s);
    CuAssertPtrEquals(tc, 0, sp->next);
    freestrlist(sp);

    split_paragraph(&sp, "12345678 90 12345678", 0, 8, '*');
    CuAssertPtrNotNull(tc, sp);
    CuAssertStrEquals(tc, "12345678", sp->s);
    CuAssertPtrNotNull(tc, sp->next);
    CuAssertStrEquals(tc, "90", sp->next->s);
    CuAssertPtrNotNull(tc, sp->next->next);
    CuAssertStrEquals(tc, "12345678", sp->next->next->s);
    CuAssertPtrEquals(tc, 0, sp->next->next->next);
    freestrlist(sp);
}

static void test_write_unit(CuTest *tc) {
    unit *u;
    faction *f;
    race *rc;
    struct locale *lang;
    char buffer[1024];

    test_cleanup();
    rc = rc_get_or_create("human");
    rc->bonus[SK_ALCHEMY] = 1;
    lang = get_or_create_locale("de");
    locale_setstring(lang, "nr_skills", "Talente");
    locale_setstring(lang, "skill::sailing", "Segeln");
    locale_setstring(lang, "skill::alchemy", "Alchemie");
    locale_setstring(lang, "status_aggressive", "aggressiv");
    init_skills(lang);
    u = test_create_unit(test_create_faction(rc), test_create_region(0, 0, 0));
    u->faction->locale = lang;
    faction_setname(u->faction, "UFO");
    renumber_faction(u->faction, 1);
    unit_setname(u, "Hodor");
    unit_setid(u, 1);

    bufunit(u->faction, u, 0, 0, buffer, sizeof(buffer));
    CuAssertStrEquals(tc, "Hodor (1), 1 human, aggressiv.", buffer);

    set_level(u, SK_SAILING, 1);
    bufunit(u->faction, u, 0, 0, buffer, sizeof(buffer));
    CuAssertStrEquals(tc, "Hodor (1), 1 human, aggressiv, Talente: Segeln 1.", buffer);

    set_level(u, SK_ALCHEMY, 1);
    bufunit(u->faction, u, 0, 0, buffer, sizeof(buffer));
    CuAssertStrEquals(tc, "Hodor (1), 1 human, aggressiv, Talente: Segeln 1, Alchemie 2.", buffer);

    f = test_create_faction(0);
    f->locale = get_or_create_locale("de");
    bufunit(f, u, 0, 0, buffer, sizeof(buffer));
    CuAssertStrEquals(tc, "Hodor (1), UFO (1), 1 human.", buffer);
    test_cleanup();
}

static void test_arg_resources(CuTest *tc) {
    variant v1, v2;
    arg_type *atype;
    resource *res;
    item_type *itype;

    test_setup();
    itype = test_create_itemtype("stone");
    v1.v = res = malloc(sizeof(resource)*2);
    res[0].number = 10;
    res[0].type = itype->rtype;
    res[0].next = &res[1];
    res[1].number = 5;
    res[1].type = itype->rtype;
    res[1].next = NULL;

    register_reports();
    atype = find_argtype("resources");
    CuAssertPtrNotNull(tc, atype);
    v2 = atype->copy(v1);
    free(v1.v);
    CuAssertPtrNotNull(tc, v2.v);
    res = (resource *)v2.v;
    CuAssertPtrEquals(tc, itype->rtype, (void *)res->type);
    CuAssertIntEquals(tc, 10, res->number);
    CuAssertPtrNotNull(tc, res = res->next);
    CuAssertPtrEquals(tc, itype->rtype, (void *)res->type);
    CuAssertIntEquals(tc, 5, res->number);
    CuAssertPtrEquals(tc, 0, res->next);
    atype->release(v2);
    test_cleanup();
}

static void test_prepare_travelthru(CuTest *tc) {
    report_context ctx;
    faction *f, *f2;
    region *r1, *r2, *r3;
    unit *u;

    test_setup();
    f = test_create_faction(0);
    f2 = test_create_faction(0);
    r1 = test_create_region(0, 0, 0);
    r2 = test_create_region(1, 0, 0);
    r3 = test_create_region(3, 0, 0);
    test_create_unit(f2, r1);
    test_create_unit(f2, r3);
    u = test_create_unit(f, r1);
    travelthru_add(r2, u);
    prepare_report(&ctx, f);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, r3, ctx.last);
    CuAssertPtrEquals(tc, f, ctx.f);
    CuAssertIntEquals(tc, seen_unit, r1->seen.mode);
    CuAssertIntEquals(tc, seen_travel, r2->seen.mode);
    CuAssertIntEquals(tc, seen_none, r3->seen.mode);
    finish_reports(&ctx);
    CuAssertIntEquals(tc, seen_none, r2->seen.mode);

    prepare_report(&ctx, f2);
    CuAssertIntEquals(tc, seen_unit, r1->seen.mode);
    CuAssertIntEquals(tc, seen_neighbour, r2->seen.mode);
    CuAssertIntEquals(tc, seen_unit, r3->seen.mode);
    CuAssertPtrEquals(tc, f2, ctx.f);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, NULL, ctx.last);
    test_cleanup();
}

void test_prepare_lighthouse_capacity(CuTest *tc) {
    building *b;
    building_type *btype;
    unit *u1, *u2;
    region *r1, *r2;
    faction *f;
    const struct terrain_type *t_ocean, *t_plain;
    report_context ctx;

    test_setup();
    f = test_create_faction(0);
    t_ocean = test_create_terrain("ocean", SEA_REGION);
    t_plain = test_create_terrain("plain", LAND_REGION);
    btype = test_create_buildingtype("lighthouse");
    btype->maxcapacity = 4;
    r1 = test_create_region(0, 0, t_plain);
    r2 = test_create_region(1, 0, t_ocean);
    b = test_create_building(r1, btype);
    b->flags |= BLD_MAINTAINED;
    b->size = 10;
    update_lighthouse(b);
    u1 = test_create_unit(test_create_faction(0), r1);
    u1->number = 4;
    u1->building = b;
    set_level(u1, SK_PERCEPTION, 3);
    CuAssertIntEquals(tc, 1, lighthouse_range(b, u1->faction));
    CuAssertPtrEquals(tc, b, inside_building(u1));
    u2 = test_create_unit(f, r1);
    u2->building = b;
    set_level(u2, SK_PERCEPTION, 3);
    CuAssertIntEquals(tc, 0, lighthouse_range(b, u2->faction));
    CuAssertPtrEquals(tc, NULL, inside_building(u2));
    prepare_report(&ctx, u1->faction);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, NULL, ctx.last);
    CuAssertIntEquals(tc, seen_unit, r1->seen.mode);
    CuAssertIntEquals(tc, seen_lighthouse, r2->seen.mode);
    finish_reports(&ctx);

    prepare_report(&ctx, u2->faction);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, 0, ctx.last);
    CuAssertIntEquals(tc, seen_unit, r1->seen.mode);
    CuAssertIntEquals(tc, seen_neighbour, r2->seen.mode);
    finish_reports(&ctx);

    test_cleanup();
}

static void test_prepare_lighthouse(CuTest *tc) {
    report_context ctx;
    faction *f;
    region *r1, *r2, *r3;
    unit *u;
    building *b;
    building_type *btype;
    const struct terrain_type *t_ocean, *t_plain;

    test_setup();
    t_ocean = test_create_terrain("ocean", SEA_REGION);
    t_plain = test_create_terrain("plain", LAND_REGION);
    f = test_create_faction(0);
    r1 = test_create_region(0, 0, t_plain);
    r2 = test_create_region(1, 0, t_ocean);
    r3 = test_create_region(2, 0, t_ocean);
    btype = test_create_buildingtype("lighthouse");
    b = test_create_building(r1, btype);
    b->flags |= BLD_MAINTAINED;
    b->size = 10;
    update_lighthouse(b);
    u = test_create_unit(f, r1);
    u->building = b;
    set_level(u, SK_PERCEPTION, 3);
    prepare_report(&ctx, f);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, NULL, ctx.last);
    CuAssertIntEquals(tc, seen_unit, r1->seen.mode);
    CuAssertIntEquals(tc, seen_lighthouse, r2->seen.mode);
    CuAssertIntEquals(tc, seen_neighbour, r3->seen.mode);
    test_cleanup();
}

static void test_prepare_report(CuTest *tc) {
    report_context ctx;
    faction *f;
    region *r;

    test_setup();
    f = test_create_faction(0);
    r = test_create_region(0, 0, 0);

    prepare_report(&ctx, f);
    CuAssertPtrEquals(tc, 0, ctx.first);
    CuAssertPtrEquals(tc, 0, ctx.last);
    CuAssertIntEquals(tc, seen_none, r->seen.mode);
    finish_reports(&ctx);

    test_create_unit(f, r);
    prepare_report(&ctx, f);
    CuAssertPtrEquals(tc, r, ctx.first);
    CuAssertPtrEquals(tc, 0, ctx.last);
    CuAssertIntEquals(tc, seen_unit, r->seen.mode);
    finish_reports(&ctx);
    CuAssertIntEquals(tc, seen_none, r->seen.mode);
    finish_reports(&ctx);

    r = test_create_region(2, 0, 0);
    CuAssertPtrEquals(tc, r, regions->next);
    prepare_report(&ctx, f);
    CuAssertPtrEquals(tc, regions, ctx.first);
    CuAssertPtrEquals(tc, r, ctx.last);
    CuAssertIntEquals(tc, seen_none, r->seen.mode);
    test_cleanup();
}

static void test_seen_neighbours(CuTest *tc) {
    report_context ctx;
    faction *f;
    region *r1, *r2;

    test_setup();
    f = test_create_faction(0);
    r1 = test_create_region(0, 0, 0);
    r2 = test_create_region(1, 0, 0);

    test_create_unit(f, r1);
    prepare_report(&ctx, f);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, 0, ctx.last);
    CuAssertIntEquals(tc, seen_unit, r1->seen.mode);
    CuAssertIntEquals(tc, seen_neighbour, r2->seen.mode);
    finish_reports(&ctx);
    test_cleanup();
}

static void test_seen_travelthru(CuTest *tc) {
    report_context ctx;
    faction *f;
    unit *u;
    region *r1, *r2, *r3;

    test_setup();
    f = test_create_faction(0);
    r1 = test_create_region(0, 0, 0);
    r2 = test_create_region(1, 0, 0);
    r3 = test_create_region(2, 0, 0);

    u = test_create_unit(f, r1);
    CuAssertPtrEquals(tc, r1, f->first);
    CuAssertPtrEquals(tc, r1, f->last);
    travelthru_add(r2, u);
    CuAssertPtrEquals(tc, r1, f->first);
    CuAssertPtrEquals(tc, r3, f->last);
    prepare_report(&ctx, f);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, 0, ctx.last);
    CuAssertIntEquals(tc, seen_unit, r1->seen.mode);
    CuAssertIntEquals(tc, seen_travel, r2->seen.mode);
    CuAssertIntEquals(tc, seen_neighbour, r3->seen.mode);
    finish_reports(&ctx);
    test_cleanup();
}

static void test_region_distance(CuTest *tc) {
    region *r;
    region *result[8];
    test_setup();
    r = test_create_region(0, 0, 0);
    CuAssertIntEquals(tc, 1, get_regions_distance_arr(r, 0, result, 8));
    CuAssertPtrEquals(tc, r, result[0]);
    CuAssertIntEquals(tc, 1, get_regions_distance_arr(r, 1, result, 8));
    test_create_region(1, 0, 0);
    test_create_region(0, 1, 0);
    CuAssertIntEquals(tc, 1, get_regions_distance_arr(r, 0, result, 8));
    CuAssertIntEquals(tc, 3, get_regions_distance_arr(r, 1, result, 8));
    CuAssertIntEquals(tc, 3, get_regions_distance_arr(r, 2, result, 8));
    test_cleanup();
}

static void test_region_distance_ql(CuTest *tc) {
    region *r;
    quicklist *ql;
    test_setup();
    r = test_create_region(0, 0, 0);
    ql = get_regions_distance(r, 0);
    CuAssertIntEquals(tc, 1, ql_length(ql));
    CuAssertPtrEquals(tc, r, ql_get(ql, 0));
    ql_free(ql);
    test_create_region(1, 0, 0);
    test_create_region(0, 1, 0);
    ql = get_regions_distance(r, 1);
    CuAssertIntEquals(tc, 3, ql_length(ql));
    ql_free(ql);
    ql = get_regions_distance(r, 2);
    CuAssertIntEquals(tc, 3, ql_length(ql));
    ql_free(ql);
    test_cleanup();
}

CuSuite *get_reports_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_region_distance);
    SUITE_ADD_TEST(suite, test_region_distance_ql);
    SUITE_ADD_TEST(suite, test_prepare_report);
    SUITE_ADD_TEST(suite, test_seen_neighbours);
    SUITE_ADD_TEST(suite, test_seen_travelthru);
    SUITE_ADD_TEST(suite, test_prepare_lighthouse);
    SUITE_ADD_TEST(suite, test_prepare_lighthouse_capacity);
    SUITE_ADD_TEST(suite, test_prepare_travelthru);
    SUITE_ADD_TEST(suite, test_reorder_units);
    SUITE_ADD_TEST(suite, test_seen_faction);
    SUITE_ADD_TEST(suite, test_regionid);
    SUITE_ADD_TEST(suite, test_sparagraph);
    SUITE_ADD_TEST(suite, test_write_unit);
    SUITE_ADD_TEST(suite, test_arg_resources);
    return suite;
}
