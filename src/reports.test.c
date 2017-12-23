#include <platform.h>
#include "reports.h"

#include "calendar.h"
#include "keyword.h"
#include "lighthouse.h"
#include "laws.h"
#include "move.h"
#include "spells.h"
#include "spy.h"
#include "travelthru.h"

#include <kernel/ally.h>
#include <kernel/config.h>
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

#include <util/attrib.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/message.h>

#include <attributes/attributes.h>
#include <attributes/key.h>
#include <attributes/otherfaction.h>

#include <selist.h>
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

    test_setup();
    r = test_create_region(0, 0, NULL);
    b = test_create_building(r, NULL);
    s = test_create_ship(r, NULL);
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
    CuAssertPtrEquals(tc, f2, selist_get(f1->seen_factions, 0));
    CuAssertIntEquals(tc, 1, selist_length(f1->seen_factions));
    add_seen_faction(f1, f2);
    CuAssertIntEquals(tc, 1, selist_length(f1->seen_factions));
    add_seen_faction(f1, f1);
    CuAssertIntEquals(tc, 2, selist_length(f1->seen_factions));
    f2 = (faction *)selist_get(f1->seen_factions, 1);
    f1 = (faction *)selist_get(f1->seen_factions, 0);
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

static void test_sparagraph_long(CuTest *tc) {
    strlist *sp = 0;

    split_paragraph(&sp, "HelloWorld HelloWorld", 0, 16, 0);
    CuAssertPtrNotNull(tc, sp);
    CuAssertStrEquals(tc, "HelloWorld", sp->s);
    CuAssertStrEquals(tc, "HelloWorld", sp->next->s);
    CuAssertPtrEquals(tc, NULL, sp->next->next);
    freestrlist(sp);

    split_paragraph(&sp, "HelloWorldHelloWorld", 0, 16, 0);
    CuAssertPtrNotNull(tc, sp);
    CuAssertStrEquals(tc, "HelloWorldHelloWorld", sp->s);
    CuAssertPtrEquals(tc, NULL, sp->next);
    freestrlist(sp);
}

static void test_bufunit_fstealth(CuTest *tc) {
    faction *f1, *f2;
    region *r;
    unit *u;
    ally *al;
    char buf[256];
    struct locale *lang;

    test_setup();
    lang = get_or_create_locale("de");
    locale_setstring(lang, "status_aggressive", "aggressive");
    locale_setstring(lang, "anonymous", "anonymous");
    f1 = test_create_faction(0);
    f1->locale = lang;
    f2 = test_create_faction(0);
    f2->locale = lang;
    r = test_create_region(0, 0, 0);
    u = test_create_unit(f1, r);
    faction_setname(f1, "UFO");
    renumber_faction(f1, 1);
    faction_setname(f2, "TWW");
    renumber_faction(f2, 2);
    unit_setname(u, "Hodor");
    unit_setid(u, 1);
    key_set(&u->attribs, 42, 42);

    /* report to ourselves */
    bufunit(f1, u, 0, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), 1 human, aggressive.", buf);

    /* ... also when we are anonymous */
    u->flags |= UFL_ANON_FACTION;
    bufunit(f1, u, 0, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), anonymous, 1 human, aggressive.", buf);
    u->flags &= ~UFL_ANON_FACTION;

    /* we see that our unit is cloaked */
    set_factionstealth(u, f2);
    CuAssertPtrNotNull(tc, u->attribs);
    bufunit(f1, u, 0, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), TWW (2), 1 human, aggressive.", buf);

    /* ... also when we are anonymous */
    u->flags |= UFL_ANON_FACTION;
    bufunit(f1, u, 0, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), anonymous, 1 human, aggressive.", buf);
    u->flags &= ~UFL_ANON_FACTION;

    /* we can see that someone is presenting as us */
    bufunit(f2, u, 0, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), TWW (2), 1 human.", buf);

    /* ... but not if they are anonymous */
    u->flags |= UFL_ANON_FACTION;
    bufunit(f2, u, 0, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), anonymous, 1 human.", buf);
    u->flags &= ~UFL_ANON_FACTION;

    /* we see the same thing as them when we are an ally */
    al = ally_add(&f1->allies, f2);
    al->status = HELP_FSTEALTH;
    bufunit(f2, u, 0, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), TWW (2) (UFO (1)), 1 human.", buf);

    /* ... also when they are anonymous */
    u->flags |= UFL_ANON_FACTION;
    bufunit(f2, u, 0, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), anonymous, 1 human.", buf);
    u->flags &= ~UFL_ANON_FACTION;

    /* fstealth has no influence when we are allies, same results again */
    set_factionstealth(u, NULL);
    bufunit(f2, u, 0, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), UFO (1), 1 human.", buf);

    u->flags |= UFL_ANON_FACTION;
    bufunit(f2, u, 0, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), anonymous, 1 human.", buf);
    u->flags &= ~UFL_ANON_FACTION;

    test_cleanup();
}

static void test_bufunit(CuTest *tc) {
    unit *u;
    faction *f;
    race *rc;
    struct locale *lang;
    char buffer[256];

    test_setup();
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

    set_level(u, SK_ALCHEMY, 1);
    bufunit(u->faction, u, 0, 0, buffer, sizeof(buffer));
    CuAssertStrEquals(tc, "Hodor (1), 1 human, aggressiv, Talente: Alchemie 2.", buffer);

    set_level(u, SK_SAILING, 1);
    bufunit(u->faction, u, 0, 0, buffer, sizeof(buffer));
    CuAssertStrEquals(tc, "Hodor (1), 1 human, aggressiv, Talente: Alchemie 2, Segeln 1.", buffer);

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

static void test_newbie_password_message(CuTest *tc) {
    report_context ctx;
    faction *f;
    test_setup();
    f = test_create_faction(0);
    f->age = 5;
    f->flags = 0;
    prepare_report(&ctx, f);
    CuAssertIntEquals(tc, 0, f->flags&FFL_PWMSG);
    CuAssertPtrEquals(tc, 0, test_find_messagetype(f->msgs, "changepasswd"));
    f->age=2;
    prepare_report(&ctx, f);
    CuAssertIntEquals(tc, FFL_PWMSG, f->flags&FFL_PWMSG);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "changepasswd"));
    finish_reports(&ctx);
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
    finish_reports(&ctx);
    test_cleanup();
}

static void test_get_addresses(CuTest *tc) {
    report_context ctx;
    faction *f, *f2, *f1;
    region *r;

    test_setup();
    f = test_create_faction(0);
    f1 = test_create_faction(0);
    f2 = test_create_faction(0);
    r = test_create_region(0, 0, 0);
    test_create_unit(f, r);
    test_create_unit(f1, r);
    test_create_unit(f2, r);
    prepare_report(&ctx, f);
    CuAssertPtrEquals(tc, r, ctx.first);
    CuAssertPtrEquals(tc, NULL, ctx.last);
    get_addresses(&ctx);
    CuAssertPtrNotNull(tc, ctx.addresses);
    CuAssertTrue(tc, selist_contains(ctx.addresses, f, NULL));
    CuAssertTrue(tc, selist_contains(ctx.addresses, f1, NULL));
    CuAssertTrue(tc, selist_contains(ctx.addresses, f2, NULL));
    CuAssertIntEquals(tc, 3, selist_length(ctx.addresses));
    finish_reports(&ctx);
    test_cleanup();
}

static void test_get_addresses_fstealth(CuTest *tc) {
    report_context ctx;
    faction *f, *f2, *f1;
    region *r;
    unit *u;

    test_setup();
    f = test_create_faction(0);
    f1 = test_create_faction(0);
    f2 = test_create_faction(0);
    r = test_create_region(0, 0, 0);
    test_create_unit(f, r);
    u = test_create_unit(f1, r);
    set_factionstealth(u, f2);

    prepare_report(&ctx, f);
    CuAssertPtrEquals(tc, r, ctx.first);
    CuAssertPtrEquals(tc, NULL, ctx.last);
    get_addresses(&ctx);
    CuAssertPtrNotNull(tc, ctx.addresses);
    CuAssertTrue(tc, selist_contains(ctx.addresses, f, NULL));
    CuAssertTrue(tc, !selist_contains(ctx.addresses, f1, NULL));
    CuAssertTrue(tc, selist_contains(ctx.addresses, f2, NULL));
    CuAssertIntEquals(tc, 2, selist_length(ctx.addresses));
    finish_reports(&ctx);
    test_cleanup();
}

static void test_get_addresses_travelthru(CuTest *tc) {
    report_context ctx;
    faction *f, *f2, *f1;
    region *r1, *r2;
    unit *u;

    test_setup();
    f = test_create_faction(0);
    f1 = test_create_faction(0);
    f2 = test_create_faction(0);
    r1 = test_create_region(0, 0, 0);
    r2 = test_create_region(1, 0, 0);
    u = test_create_unit(f, r2);
    travelthru_add(r1, u);
    u = test_create_unit(f1, r1);
    set_factionstealth(u, f2);
    u->building = test_create_building(u->region, test_create_buildingtype("tower"));

    prepare_report(&ctx, f);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, NULL, ctx.last);
    get_addresses(&ctx);
    CuAssertPtrNotNull(tc, ctx.addresses);
    CuAssertTrue(tc, selist_contains(ctx.addresses, f, NULL));
    CuAssertTrue(tc, !selist_contains(ctx.addresses, f1, NULL));
    CuAssertTrue(tc, selist_contains(ctx.addresses, f2, NULL));
    CuAssertIntEquals(tc, 2, selist_length(ctx.addresses));
    finish_reports(&ctx);
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
    CuAssertIntEquals(tc, 1, lighthouse_range(b, u1->faction, u1));
    CuAssertPtrEquals(tc, b, inside_building(u1));
    u2 = test_create_unit(f, r1);
    u2->building = b;
    set_level(u2, SK_PERCEPTION, 3);
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

    /* lighthouse capacity is # of units, not people: */
    config_set_int("rules.lighthouse.unit_capacity", 1);
    prepare_report(&ctx, u2->faction);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, 0, ctx.last);
    CuAssertIntEquals(tc, seen_unit, r1->seen.mode);
    CuAssertIntEquals(tc, seen_lighthouse, r2->seen.mode);
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
    finish_reports(&ctx);
    test_cleanup();
}

/**
 * In E3, region owners get the view range benefit of 
 * any lighthouse in the region.
 */
static void test_prepare_lighthouse_owners(CuTest *tc)
{
    report_context ctx;
    faction *f;
    region *r1, *r2, *r3;
    unit *u;
    building *b;
    building_type *btype;
    const struct terrain_type *t_ocean, *t_plain;

    test_setup();
    enable_skill(SK_PERCEPTION, false);
    config_set("rules.region_owner_pay_building", "lighthouse");
    config_set("rules.region_owners", "1");
    t_ocean = test_create_terrain("ocean", SEA_REGION);
    t_plain = test_create_terrain("plain", LAND_REGION);
    f = test_create_faction(0);
    r1 = test_create_region(0, 0, t_plain);
    r2 = test_create_region(1, 0, t_ocean);
    r3 = test_create_region(2, 0, t_ocean);
    r3 = test_create_region(3, 0, t_ocean);
    btype = test_create_buildingtype("lighthouse");
    b = test_create_building(r1, btype);
    b->flags |= BLD_MAINTAINED;
    b->size = 10;
    update_lighthouse(b);
    u = test_create_unit(f, r1);
    u = test_create_unit(test_create_faction(0), r1);
    u->building = b;
    region_set_owner(b->region, f, 0);
    CuAssertIntEquals(tc, 2, lighthouse_range(b, NULL, NULL));
    prepare_report(&ctx, f);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, NULL, ctx.last);
    CuAssertIntEquals(tc, seen_unit, r1->seen.mode);
    CuAssertIntEquals(tc, seen_lighthouse, r2->seen.mode);
    CuAssertIntEquals(tc, seen_neighbour, r3->seen.mode);
    finish_reports(&ctx);
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

    r = test_create_region(2, 0, 0);
    CuAssertPtrEquals(tc, r, regions->next);
    prepare_report(&ctx, f);
    CuAssertPtrEquals(tc, regions, ctx.first);
    CuAssertPtrEquals(tc, r, ctx.last);
    CuAssertIntEquals(tc, seen_none, r->seen.mode);
    finish_reports(&ctx);
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

static void test_region_distance_max(CuTest *tc) {
    region *r;
    region *result[64];
    int x, y;
    test_setup();
    r = test_create_region(0, 0, 0);
    for (x=-3;x<=3;++x) {
        for (y = -3; y <= 3; ++y) {
            if (x != 0 || y != 0) {
                test_create_region(x, y, 0);
            }
        }
    }
    CuAssertIntEquals(tc, 1, get_regions_distance_arr(r, 0, result, 64));
    CuAssertIntEquals(tc, 7, get_regions_distance_arr(r, 1, result, 64));
    CuAssertIntEquals(tc, 19, get_regions_distance_arr(r, 2, result, 64));
    CuAssertIntEquals(tc, 37, get_regions_distance_arr(r, 3, result, 64));
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
    selist *ql;
    test_setup();
    r = test_create_region(0, 0, 0);
    ql = get_regions_distance(r, 0);
    CuAssertIntEquals(tc, 1, selist_length(ql));
    CuAssertPtrEquals(tc, r, selist_get(ql, 0));
    selist_free(ql);
    test_create_region(1, 0, 0);
    test_create_region(0, 1, 0);
    ql = get_regions_distance(r, 1);
    CuAssertIntEquals(tc, 3, selist_length(ql));
    selist_free(ql);
    ql = get_regions_distance(r, 2);
    CuAssertIntEquals(tc, 3, selist_length(ql));
    selist_free(ql);
    test_cleanup();
}

static void test_report_far_vision(CuTest *tc) {
    faction *f;
    region *r1, *r2;
    test_setup();
    f = test_create_faction(0);
    r1 = test_create_region(0, 0, 0);
    test_create_unit(f, r1);
    r2 = test_create_region(10, 0, 0);
    set_observer(r2, f, 10, 2);
    CuAssertPtrEquals(tc, r1, f->first);
    CuAssertPtrEquals(tc, r2, f->last);
    report_context ctx;
    prepare_report(&ctx, f);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, 0, ctx.last);
    CuAssertIntEquals(tc, seen_unit, r1->seen.mode);
    CuAssertIntEquals(tc, seen_spell, r2->seen.mode);
    finish_reports(&ctx);
    test_cleanup();
}

static void test_stealth_modifier(CuTest *tc) {
    region *r;
    faction *f;

    test_setup();
    f = test_create_faction(NULL);
    r = test_create_region(0, 0, NULL);
    CuAssertIntEquals(tc, 0, stealth_modifier(r, f, seen_unit));
    CuAssertIntEquals(tc, -1, stealth_modifier(r, f, seen_travel));
    CuAssertIntEquals(tc, -2, stealth_modifier(r, f, seen_lighthouse));
    CuAssertIntEquals(tc, -1, stealth_modifier(r, f, seen_spell));

    set_observer(r, f, 10, 1);
    CuAssertIntEquals(tc, 10, stealth_modifier(r, f, seen_spell));
    CuAssertIntEquals(tc, -1, stealth_modifier(r, NULL, seen_spell));
    test_cleanup();
}

static void test_insect_warnings(CuTest *tc) {
    faction *f;
    gamedate gd;

    /* OBS: in unit tests, get_gamedate always returns season = 0 */
    test_setup();
    f = test_create_faction(test_create_race("insect"));

    gd.turn = 0;
    gd.season = 3;
    report_warnings(f, &gd);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "nr_insectfall"));

    gd.season = 0;
    report_warnings(f, &gd);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "nr_insectwinter"));
    test_cleanup();
}

static void test_newbie_warning(CuTest *tc) {
    faction *f;

    test_setup();
    f = test_create_faction(test_create_race("insect"));
    config_set_int("NewbieImmunity", 3);

    f->age = 2;
    report_warnings(f, NULL);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "newbieimmunity"));
    test_clear_messages(f);

    f->age = 3;
    report_warnings(f, NULL);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "newbieimmunity"));
    test_clear_messages(f);

    test_cleanup();
}

static void test_cansee_spell(CuTest *tc) {
    unit *u2;
    faction *f;

    test_setup();
    f = test_create_faction(0);
    u2 = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));

    CuAssertTrue(tc, cansee(f, u2->region, u2, 0));
    CuAssertTrue(tc, visible_unit(u2, f, 0, seen_spell));
    CuAssertTrue(tc, visible_unit(u2, f, 0, seen_battle));

    set_level(u2, SK_STEALTH, 1);
    CuAssertTrue(tc, !cansee(f, u2->region, u2, 0));
    CuAssertTrue(tc, cansee(f, u2->region, u2, 1));
    CuAssertTrue(tc, visible_unit(u2, f, 1, seen_spell));
    CuAssertTrue(tc, visible_unit(u2, f, 1, seen_battle));

    test_cleanup();
}

CuSuite *get_reports_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_region_distance);
    SUITE_ADD_TEST(suite, test_region_distance_max);
    SUITE_ADD_TEST(suite, test_region_distance_ql);
    SUITE_ADD_TEST(suite, test_newbie_password_message);
    SUITE_ADD_TEST(suite, test_prepare_report);
    SUITE_ADD_TEST(suite, test_seen_neighbours);
    SUITE_ADD_TEST(suite, test_seen_travelthru);
    SUITE_ADD_TEST(suite, test_prepare_lighthouse);
    SUITE_ADD_TEST(suite, test_prepare_lighthouse_owners);
    SUITE_ADD_TEST(suite, test_prepare_lighthouse_capacity);
    SUITE_ADD_TEST(suite, test_prepare_travelthru);
    SUITE_ADD_TEST(suite, test_get_addresses);
    SUITE_ADD_TEST(suite, test_get_addresses_fstealth);
    SUITE_ADD_TEST(suite, test_get_addresses_travelthru);
    SUITE_ADD_TEST(suite, test_report_far_vision);
    SUITE_ADD_TEST(suite, test_reorder_units);
    SUITE_ADD_TEST(suite, test_seen_faction);
    SUITE_ADD_TEST(suite, test_stealth_modifier);
    SUITE_ADD_TEST(suite, test_regionid);
    SUITE_ADD_TEST(suite, test_sparagraph);
    SUITE_ADD_TEST(suite, test_sparagraph_long);
    SUITE_ADD_TEST(suite, test_bufunit);
    SUITE_ADD_TEST(suite, test_bufunit_fstealth);
    SUITE_ADD_TEST(suite, test_arg_resources);
    SUITE_ADD_TEST(suite, test_insect_warnings);
    SUITE_ADD_TEST(suite, test_newbie_warning);
    SUITE_ADD_TEST(suite, test_cansee_spell);
    return suite;
}
