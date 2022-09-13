#include "reports.h"

#include "guard.h"
#include "lighthouse.h"
#include "laws.h"
#include "spy.h"
#include "travelthru.h"

#include "kernel/ally.h"
#include "kernel/calendar.h"
#include "kernel/config.h"
#include "kernel/building.h"
#include "kernel/faction.h"
#include "kernel/item.h"
#include "kernel/messages.h"
#include "kernel/race.h"
#include "kernel/region.h"
#include "kernel/ship.h"
#include "kernel/skill.h"
#include "kernel/terrain.h"
#include "kernel/unit.h"

#include "util/language.h"
#include "util/lists.h"
#include "util/message.h"
#include "util/nrmessage.h"
#include "util/variant.h"

#include "attributes/attributes.h"

#include <selist.h>

#include <CuTest.h>
#include <tests.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static void test_reorder_units(CuTest * tc)
{
    region *r;
    building *b;
    ship * s;
    unit *u0, *u1, *u2, *u3, *u4;
    struct faction * f;

    test_setup();
    r = test_create_plain(0, 0);
    b = test_create_building(r, NULL);
    s = test_create_ship(r, NULL);
    f = test_create_faction();

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
    CuAssertPtrEquals(tc, NULL, u0->next);
    test_teardown();
}

static void test_regionid(CuTest * tc) {
    size_t len;
    const struct terrain_type * plain;
    struct region * r;
    char buffer[64];

    test_setup();
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
    test_teardown();
}

static void test_seen_faction(CuTest *tc) {
    faction *f1, *f2;
    race *rc;

    test_setup();
    rc = test_create_race("human");
    f1 = test_create_faction_ex(rc, NULL);
    f2 = test_create_faction_ex(rc, NULL);
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
    test_teardown();
}

static void test_sparagraph(CuTest *tc) {
    strlist *sp = 0;

    split_paragraph(&sp, "Hello World", 0, 16, 0);
    CuAssertPtrNotNull(tc, sp);
    CuAssertStrEquals(tc, "Hello World", sp->s);
    CuAssertPtrEquals(tc, NULL, sp->next);
    freestrlist(sp);

    split_paragraph(&sp, "Hello World", 4, 16, 0);
    CuAssertPtrNotNull(tc, sp);
    CuAssertStrEquals(tc, "    Hello World", sp->s);
    CuAssertPtrEquals(tc, NULL, sp->next);
    freestrlist(sp);

    split_paragraph(&sp, "Hello World", 4, 16, '*');
    CuAssertPtrNotNull(tc, sp);
    CuAssertStrEquals(tc, "  * Hello World", sp->s);
    CuAssertPtrEquals(tc, NULL, sp->next);
    freestrlist(sp);

    split_paragraph(&sp, "12345678 90 12345678", 0, 8, '*');
    CuAssertPtrNotNull(tc, sp);
    CuAssertStrEquals(tc, "12345678", sp->s);
    CuAssertPtrNotNull(tc, sp->next);
    CuAssertStrEquals(tc, "90", sp->next->s);
    CuAssertPtrNotNull(tc, sp->next->next);
    CuAssertStrEquals(tc, "12345678", sp->next->next->s);
    CuAssertPtrEquals(tc, NULL, sp->next->next->next);
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
    char buf[256];
    struct locale *lang;

    test_setup();
    lang = get_or_create_locale("de");
    locale_setstring(lang, "status_aggressive", "aggressive");
    locale_setstring(lang, "anonymous", "anonymous");
    locale_setstring(lang, "disguised_as", "disguised as");
    f1 = test_create_faction();
    f1->locale = lang;
    f2 = test_create_faction();
    f2->locale = lang;
    r = test_create_plain(0, 0);
    u = test_create_unit(f1, r);
    faction_setname(f1, "UFO");
    renumber_faction(f1, 1);
    faction_setname(f2, "TWW");
    renumber_faction(f2, 2);
    unit_setname(u, "Hodor");
    unit_setid(u, 1);

    /* report to ourselves */
    bufunit_depr(f1, u, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), 1 human, aggressive.", buf);

    /* ... also when we are anonymous */
    u->flags |= UFL_ANON_FACTION;
    bufunit_depr(f1, u, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), anonymous, 1 human, aggressive.", buf);
    u->flags &= ~UFL_ANON_FACTION;

    /* we see that our unit is cloaked */
    set_factionstealth(u, f2);
    CuAssertPtrNotNull(tc, u->attribs);
    bufunit_depr(f1, u, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), disguised as TWW (2), 1 human, aggressive.", buf);

    /* ... also see that we are anonymous */
    u->flags |= UFL_ANON_FACTION;
    bufunit_depr(f1, u, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), disguised as TWW (2), anonymous, 1 human, aggressive.", buf);
    u->flags &= ~UFL_ANON_FACTION;

    /* we can see that someone is presenting as us */
    bufunit_depr(f2, u, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), disguised as TWW (2), 1 human.", buf);

    /* ... but not if they are anonymous */
    u->flags |= UFL_ANON_FACTION;
    bufunit_depr(f2, u, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), anonymous, 1 human.", buf);
    u->flags &= ~UFL_ANON_FACTION;

    /* we see the same thing as them when we are an ally */
    ally_set(&f1->allies, f2, HELP_FSTEALTH);
    bufunit_depr(f2, u, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), UFO (1), disguised as TWW (2), 1 human.", buf);

    /* ... also when they are anonymous */
    u->flags |= UFL_ANON_FACTION;
    bufunit_depr(f2, u, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), UFO (1), anonymous, 1 human.", buf);
    u->flags &= ~UFL_ANON_FACTION;

    /* fstealth has no influence when we are allies, same results again */
    set_factionstealth(u, NULL);
    bufunit_depr(f2, u, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), UFO (1), 1 human.", buf);

    u->flags |= UFL_ANON_FACTION;
    bufunit_depr(f2, u, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), UFO (1), anonymous, 1 human.", buf);
    u->flags &= ~UFL_ANON_FACTION;

    test_teardown();
}

static void test_bufunit_help_stealth(CuTest *tc) {
    faction *f1, *f2;
    region *r;
    unit *u;
    char buf[256];
    struct locale *lang;

    test_setup();
    lang = get_or_create_locale("de");
    locale_setstring(lang, "status_aggressive", "aggressive");
    locale_setstring(lang, "anonymous", "anonymous");
    locale_setstring(lang, "disguised_as", "disguised as");
    f1 = test_create_faction();
    f1->locale = lang;
    f2 = test_create_faction();
    f2->locale = lang;
    r = test_create_plain(0, 0);
    u = test_create_unit(f2, r);
    faction_setname(f1, "UFO");
    renumber_faction(f1, 1);
    faction_setname(f2, "TWW");
    renumber_faction(f2, 2);
    unit_setname(u, "Hodor");
    unit_setid(u, 1);

    /* report non-stealthed unit */
    bufunit_depr(f1, u, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), TWW (2), 1 human.", buf);

    /* anonymous unit */
    u->flags |= UFL_ANON_FACTION;
    bufunit_depr(f1, u, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), anonymous, 1 human.", buf);
    u->flags &= ~UFL_ANON_FACTION;

    /* stealthed unit */
    set_factionstealth(u, f1);
    CuAssertPtrNotNull(tc, u->attribs);
    bufunit_depr(f1, u, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), disguised as UFO (1), 1 human.", buf);

    /* allies let us see their true faction */
    ally_set(&f2->allies, f1, HELP_FSTEALTH);
    bufunit_depr(f1, u, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), TWW (2), disguised as UFO (1), 1 human.", buf);

    /* anonymous unit */
    set_factionstealth(u, NULL);
    u->flags |= UFL_ANON_FACTION;
    /* allies let us see their true faction */
    bufunit_depr(f1, u, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), TWW (2), anonymous, 1 human.", buf);
    /* non-allies do not show their faction when anonymous*/
    ally_set(&f2->allies, f1, 0);
    bufunit_depr(f1, u, seen_unit, buf, sizeof(buf));
    CuAssertStrEquals(tc, "Hodor (1), anonymous, 1 human.", buf);

    test_teardown();
}

static void test_bufunit_bullets(CuTest* tc) {
    unit* u;
    faction* f, *f2;

    test_setup();
    f = test_create_faction();
    u = test_create_unit(f2 = test_create_faction(), test_create_plain(0, 0));
    CuAssertIntEquals(tc, '-', bufunit_bullet(f, u, NULL, false));
    ally_set(&f->allies, f2, HELP_GIVE);
    CuAssertIntEquals(tc, '+', bufunit_bullet(f, u, NULL, false));
    test_teardown();
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
    u = test_create_unit(test_create_faction_ex(rc, NULL), test_create_plain(0, 0));
    u->faction->locale = lang;
    faction_setname(u->faction, "UFO");
    renumber_faction(u->faction, 1);
    unit_setname(u, "Hodor");
    unit_setid(u, 1);

    bufunit_depr(u->faction, u, 0, buffer, sizeof(buffer));
    CuAssertStrEquals(tc, "Hodor (1), 1 human, aggressiv.", buffer);

    set_level(u, SK_ALCHEMY, 1);
    bufunit_depr(u->faction, u, 0, buffer, sizeof(buffer));
    CuAssertStrEquals(tc, "Hodor (1), 1 human, aggressiv, Talente: Alchemie 2.", buffer);

    set_level(u, SK_SAILING, 1);
    bufunit_depr(u->faction, u, 0, buffer, sizeof(buffer));
    CuAssertStrEquals(tc, "Hodor (1), 1 human, aggressiv, Talente: Alchemie 2, Segeln 1.", buffer);

    f = test_create_faction();
    f->locale = get_or_create_locale("de");
    bufunit_depr(f, u, 0, buffer, sizeof(buffer));
    CuAssertStrEquals(tc, "Hodor (1), UFO (1), 1 human.", buffer);

    test_teardown();
}

static void test_bufunit_racename_plural(CuTest *tc) {
    unit *u;
    faction *f;
    race *rc, *rct;
    struct locale *lang;
    char buffer[256];

    test_setup();
    rc = rc_get_or_create("human");
    rct = rc_get_or_create("orc");
    rc->bonus[SK_ALCHEMY] = 1;
    lang = get_or_create_locale("de");
    locale_setstring(lang, "race::orc", "Ork");
    locale_setstring(lang, "race::orc_p", "Orks");
    locale_setstring(lang, "race::human", "Mensch");
    locale_setstring(lang, "race::human_p", "Menschen");
    locale_setstring(lang, "status_aggressive", "aggressiv");
    init_skills(lang);
    f = test_create_faction();
    u = test_create_unit(test_create_faction_ex(rc, NULL), test_create_plain(0, 0));
    u->faction->locale = f->locale = lang;
    renumber_faction(u->faction, 7);
    faction_setname(u->faction, "UFO");
    unit_setname(u, "Hodor");
    unit_setid(u, 1);

    bufunit_depr(f, u, 0, buffer, sizeof(buffer));
    CuAssertStrEquals(tc, "Hodor (1), UFO (7), 1 Mensch.", buffer);

    scale_number(u, 2);
    bufunit_depr(f, u, 0, buffer, sizeof(buffer));
    CuAssertStrEquals(tc, "Hodor (1), UFO (7), 2 Menschen.", buffer);

    unit_convert_race(u, rct, "human");
    bufunit_depr(f, u, 0, buffer, sizeof(buffer));
    CuAssertStrEquals(tc, "Hodor (1), UFO (7), 2 Menschen.", buffer);

    bufunit_depr(u->faction, u, 0, buffer, sizeof(buffer));
    CuAssertStrEquals(tc, "Hodor (1), 2 Menschen (Orks), aggressiv.", buffer);

    scale_number(u, 1);
    bufunit_depr(f, u, 0, buffer, sizeof(buffer));
    CuAssertStrEquals(tc, "Hodor (1), UFO (7), 1 Mensch.", buffer);

    bufunit_depr(u->faction, u, 0, buffer, sizeof(buffer));
    CuAssertStrEquals(tc, "Hodor (1), 1 Mensch (Ork), aggressiv.", buffer);

    test_teardown();
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
    CuAssertPtrEquals(tc, NULL, res->next);
    atype->release(v2);
    test_teardown();
}

static void test_newbie_password_message(CuTest *tc) {
    report_context ctx;
    faction *f;
    test_setup();
    f = test_create_faction();
    f->age = 5;
    f->flags = 0;
    prepare_report(&ctx, f, NULL);
    CuAssertIntEquals(tc, 0, f->flags&FFL_PWMSG);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "changepasswd"));
    f->age=2;
    prepare_report(&ctx, f, NULL);
    CuAssertIntEquals(tc, FFL_PWMSG, f->flags&FFL_PWMSG);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "changepasswd"));
    finish_reports(&ctx);
    test_teardown();
}

static void test_prepare_travelthru(CuTest *tc) {
    report_context ctx;
    faction *f, *f2;
    region *r1, *r2, *r3;
    unit *u;

    test_setup();
    f = test_create_faction();
    f2 = test_create_faction();
    r1 = test_create_plain(0, 0);
    r2 = test_create_region(1, 0, 0);
    r3 = test_create_region(3, 0, 0);
    test_create_unit(f2, r1);
    test_create_unit(f2, r3);
    u = test_create_unit(f, r1);
    travelthru_add(r2, u);
    prepare_report(&ctx, f, NULL);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, r3, ctx.last);
    CuAssertPtrEquals(tc, f, ctx.f);
    CuAssertIntEquals(tc, seen_unit, r1->seen.mode);
    CuAssertIntEquals(tc, seen_travel, r2->seen.mode);
    CuAssertIntEquals(tc, seen_none, r3->seen.mode);
    finish_reports(&ctx);
    CuAssertIntEquals(tc, seen_none, r2->seen.mode);

    prepare_report(&ctx, f2, NULL);
    CuAssertIntEquals(tc, seen_unit, r1->seen.mode);
    CuAssertIntEquals(tc, seen_neighbour, r2->seen.mode);
    CuAssertIntEquals(tc, seen_unit, r3->seen.mode);
    CuAssertPtrEquals(tc, f2, ctx.f);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, NULL, ctx.last);
    finish_reports(&ctx);
    test_teardown();
}

static void test_get_addresses(CuTest *tc) {
    report_context ctx;
    faction *f, *f2, *f1;
    region *r;

    test_setup();
    f = test_create_faction();
    f1 = test_create_faction();
    f2 = test_create_faction();
    r = test_create_plain(0, 0);
    test_create_unit(f, r);
    test_create_unit(f1, r);
    test_create_unit(f2, r);
    prepare_report(&ctx, f, NULL);
    CuAssertPtrEquals(tc, r, ctx.first);
    CuAssertPtrEquals(tc, NULL, ctx.last);
    get_addresses(&ctx);
    CuAssertPtrNotNull(tc, ctx.addresses);
    CuAssertTrue(tc, selist_contains(ctx.addresses, f, NULL));
    CuAssertTrue(tc, selist_contains(ctx.addresses, f1, NULL));
    CuAssertTrue(tc, selist_contains(ctx.addresses, f2, NULL));
    CuAssertIntEquals(tc, 3, selist_length(ctx.addresses));
    finish_reports(&ctx);
    test_teardown();
}

static void test_get_addresses_fstealth(CuTest *tc) {
    report_context ctx;
    faction *f, *f2, *f1;
    region *r;
    unit *u;

    test_setup();
    f = test_create_faction();
    f1 = test_create_faction();
    f2 = test_create_faction();
    r = test_create_plain(0, 0);
    test_create_unit(f, r);
    u = test_create_unit(f1, r);
    set_factionstealth(u, f2);

    prepare_report(&ctx, f, NULL);
    CuAssertPtrEquals(tc, r, ctx.first);
    CuAssertPtrEquals(tc, NULL, ctx.last);
    get_addresses(&ctx);
    CuAssertPtrNotNull(tc, ctx.addresses);
    CuAssertTrue(tc, selist_contains(ctx.addresses, f, NULL));
    CuAssertTrue(tc, !selist_contains(ctx.addresses, f1, NULL));
    CuAssertTrue(tc, selist_contains(ctx.addresses, f2, NULL));
    CuAssertIntEquals(tc, 2, selist_length(ctx.addresses));
    finish_reports(&ctx);
    test_teardown();
}

static void test_get_addresses_travelthru(CuTest *tc) {
    report_context ctx;
    faction *f, *f4, *f3, *f2, *f1;
    region *r1, *r2;
    unit *u;
    race *rc;

    test_setup();
    f = test_create_faction();
    r1 = test_create_plain(0, 0);
    r2 = test_create_region(1, 0, 0);
    u = test_create_unit(f, r2);
    travelthru_add(r1, u);

    f1 = test_create_faction();
    u = test_create_unit(f1, r1);
    f2 = test_create_faction();
    set_factionstealth(u, f2);
    u->building = test_create_building(u->region, test_create_buildingtype("tower"));

    rc = rc_get_or_create("dragon");
    rc->flags |= RCF_UNARMEDGUARD;
    f3 = test_create_faction_ex(rc, NULL);
    u = test_create_unit(f3, r1);
    setguard(u, true);

    f4 = test_create_faction();
    u = test_create_unit(f4, r1);
    set_level(u, SK_STEALTH, 1);

    prepare_report(&ctx, f, NULL);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, NULL, ctx.last);
    get_addresses(&ctx);
    CuAssertPtrNotNull(tc, ctx.addresses);
    CuAssertTrue(tc, selist_contains(ctx.addresses, f, NULL));
    CuAssertTrue(tc, !selist_contains(ctx.addresses, f1, NULL));
    CuAssertTrue(tc, selist_contains(ctx.addresses, f2, NULL));
    CuAssertTrue(tc, selist_contains(ctx.addresses, f3, NULL));
    CuAssertTrue(tc, !selist_contains(ctx.addresses, f4, NULL));
    CuAssertIntEquals(tc, 3, selist_length(ctx.addresses));
    finish_reports(&ctx);
    test_teardown();
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
    f = test_create_faction();
    t_ocean = test_create_terrain("ocean", SEA_REGION);
    t_plain = test_create_terrain("plain", LAND_REGION);
    btype = test_create_buildingtype("lighthouse");
    btype->maxcapacity = 4;
    r1 = test_create_region(0, 0, t_plain);
    r2 = test_create_region(1, 0, t_ocean);
    b = test_create_building(r1, btype);
    b->size = 10;
    update_lighthouse(b);
    u1 = test_create_unit(NULL, r1);
    u1->number = 4;
    u1->building = b;
    set_level(u1, SK_PERCEPTION, 3);
    CuAssertIntEquals(tc, 1, lighthouse_view_distance(b, u1));
    CuAssertPtrEquals(tc, b, inside_building(u1));
    u2 = test_create_unit(f, r1);
    u2->building = b;
    set_level(u2, SK_PERCEPTION, 3);
    CuAssertPtrEquals(tc, NULL, inside_building(u2));
    prepare_report(&ctx, u1->faction, NULL);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, NULL, ctx.last);
    CuAssertIntEquals(tc, seen_unit, r1->seen.mode);
    CuAssertIntEquals(tc, seen_lighthouse, r2->seen.mode);
    finish_reports(&ctx);

    prepare_report(&ctx, u2->faction, NULL);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, NULL, ctx.last);
    CuAssertIntEquals(tc, seen_unit, r1->seen.mode);
    CuAssertIntEquals(tc, seen_neighbour, r2->seen.mode);
    finish_reports(&ctx);

    /* lighthouse capacity is # of units, not people: */
    config_set_int("rules.lighthouse.unit_capacity", 1);
    prepare_report(&ctx, u2->faction, NULL);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, NULL, ctx.last);
    CuAssertIntEquals(tc, seen_unit, r1->seen.mode);
    CuAssertIntEquals(tc, seen_lighthouse, r2->seen.mode);
    finish_reports(&ctx);

    test_teardown();
}

static void test_prepare_lighthouse(CuTest *tc) {
    report_context ctx;
    faction *f;
    region *r1, *r2, *r3, *r4;
    unit *u;
    building *b;
    building_type *btype;
    const struct terrain_type *t_ocean, *t_plain;

    test_setup();
    t_ocean = test_create_terrain("ocean", SEA_REGION);
    t_plain = test_create_terrain("plain", LAND_REGION);
    f = test_create_faction();
    r1 = test_create_region(0, 0, t_plain);
    r2 = test_create_region(1, 0, t_ocean);
    r3 = test_create_region(2, 0, t_ocean);
    r4 = test_create_region(0, 1, t_plain);
    btype = test_create_buildingtype("lighthouse");
    b = test_create_building(r1, btype);
    b->size = 10;
    update_lighthouse(b);
    u = test_create_unit(f, r1);
    u->building = b;
    set_level(u, SK_PERCEPTION, 3);
    prepare_report(&ctx, f, NULL);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, NULL, ctx.last);
    CuAssertIntEquals(tc, seen_unit, r1->seen.mode);
    CuAssertIntEquals(tc, seen_lighthouse, r2->seen.mode);
    CuAssertIntEquals(tc, seen_neighbour, r3->seen.mode);
    CuAssertIntEquals(tc, seen_lighthouse_land, r4->seen.mode);
    finish_reports(&ctx);
    test_teardown();
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
    f = test_create_faction();
    r1 = test_create_region(0, 0, t_plain);
    r2 = test_create_region(1, 0, t_ocean);
    test_create_region(2, 0, t_ocean);
    r3 = test_create_region(3, 0, t_ocean);
    btype = test_create_buildingtype("lighthouse");
    b = test_create_building(r1, btype);
    b->size = 10;
    update_lighthouse(b);
    test_create_unit(f, r1);
    u = test_create_unit(NULL, r1);
    u->building = b;
    region_set_owner(b->region, f, 0);
    CuAssertIntEquals(tc, 2, lighthouse_view_distance(b, NULL));
    prepare_report(&ctx, f, NULL);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, NULL, ctx.last);
    CuAssertIntEquals(tc, seen_unit, r1->seen.mode);
    CuAssertIntEquals(tc, seen_lighthouse, r2->seen.mode);
    CuAssertIntEquals(tc, seen_neighbour, r3->seen.mode);
    finish_reports(&ctx);
    test_teardown();
}

static void test_prepare_report(CuTest *tc) {
    report_context ctx;
    faction *f;
    region *r;

    test_setup();
    f = test_create_faction();
    r = test_create_plain(0, 0);

    prepare_report(&ctx, f, NULL);
    CuAssertPtrEquals(tc, NULL, ctx.first);
    CuAssertPtrEquals(tc, NULL, ctx.last);
    CuAssertIntEquals(tc, seen_none, r->seen.mode);
    finish_reports(&ctx);

    test_create_unit(f, r);
    prepare_report(&ctx, f, NULL);
    CuAssertPtrEquals(tc, r, ctx.first);
    CuAssertPtrEquals(tc, NULL, ctx.last);
    CuAssertIntEquals(tc, seen_unit, r->seen.mode);
    finish_reports(&ctx);
    CuAssertIntEquals(tc, seen_none, r->seen.mode);

    r = test_create_region(2, 0, 0);
    CuAssertPtrEquals(tc, r, regions->next);
    prepare_report(&ctx, f, NULL);
    CuAssertPtrEquals(tc, regions, ctx.first);
    CuAssertPtrEquals(tc, r, ctx.last);
    CuAssertIntEquals(tc, seen_none, r->seen.mode);
    finish_reports(&ctx);
    test_teardown();
}

static void test_seen_neighbours(CuTest *tc) {
    report_context ctx;
    faction *f;
    region *r1, *r2;

    test_setup();
    f = test_create_faction();
    r1 = test_create_plain(0, 0);
    r2 = test_create_region(1, 0, 0);

    test_create_unit(f, r1);
    prepare_report(&ctx, f, NULL);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, NULL, ctx.last);
    CuAssertIntEquals(tc, seen_unit, r1->seen.mode);
    CuAssertIntEquals(tc, seen_neighbour, r2->seen.mode);
    finish_reports(&ctx);
    test_teardown();
}

static void test_seen_travelthru(CuTest *tc) {
    report_context ctx;
    faction *f;
    unit *u;
    region *r1, *r2, *r3;

    test_setup();
    f = test_create_faction();
    r1 = test_create_plain(0, 0);
    r2 = test_create_region(1, 0, 0);
    r3 = test_create_region(2, 0, 0);

    u = test_create_unit(f, r1);
    CuAssertPtrEquals(tc, r1, f->first);
    CuAssertPtrEquals(tc, r1, f->last);
    travelthru_add(r2, u);
    CuAssertPtrEquals(tc, r1, f->first);
    CuAssertPtrEquals(tc, r3, f->last);
    prepare_report(&ctx, f, NULL);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, NULL, ctx.last);
    CuAssertIntEquals(tc, seen_unit, r1->seen.mode);
    CuAssertIntEquals(tc, seen_travel, r2->seen.mode);
    CuAssertIntEquals(tc, seen_neighbour, r3->seen.mode);
    finish_reports(&ctx);
    test_teardown();
}

static void test_region_distance_max(CuTest *tc) {
    region *r;
    region *result[64];
    int x, y;
    test_setup();
    r = test_create_plain(0, 0);
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
    test_teardown();
}

static void test_region_distance(CuTest *tc) {
    region *r;
    region *result[8];
    test_setup();
    r = test_create_plain(0, 0);
    CuAssertIntEquals(tc, 1, get_regions_distance_arr(r, 0, result, 8));
    CuAssertPtrEquals(tc, r, result[0]);
    CuAssertIntEquals(tc, 1, get_regions_distance_arr(r, 1, result, 8));
    test_create_region(1, 0, 0);
    test_create_region(0, 1, 0);
    CuAssertIntEquals(tc, 1, get_regions_distance_arr(r, 0, result, 8));
    CuAssertIntEquals(tc, 3, get_regions_distance_arr(r, 1, result, 8));
    CuAssertIntEquals(tc, 3, get_regions_distance_arr(r, 2, result, 8));
    test_teardown();
}

static void test_region_distance_ql(CuTest *tc) {
    region *r;
    selist *ql;
    test_setup();
    r = test_create_plain(0, 0);
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
    test_teardown();
}

static void test_report_far_vision(CuTest *tc) {
    faction *f;
    region *r1, *r2;
    test_setup();
    f = test_create_faction();
    r1 = test_create_plain(0, 0);
    test_create_unit(f, r1);
    r2 = test_create_region(10, 0, 0);
    set_observer(r2, f, 10, 2);
    CuAssertPtrEquals(tc, r1, f->first);
    CuAssertPtrEquals(tc, r2, f->last);
    report_context ctx;
    prepare_report(&ctx, f, NULL);
    CuAssertPtrEquals(tc, r1, ctx.first);
    CuAssertPtrEquals(tc, NULL, ctx.last);
    CuAssertIntEquals(tc, seen_unit, r1->seen.mode);
    CuAssertIntEquals(tc, seen_spell, r2->seen.mode);
    finish_reports(&ctx);
    test_teardown();
}

static void test_stealth_modifier(CuTest *tc) {
    region *r;
    faction *f;

    test_setup();
    f = test_create_faction();
    r = test_create_plain(0, 0);
    CuAssertIntEquals(tc, 0, stealth_modifier(r, f, seen_unit));
    CuAssertIntEquals(tc, -1, stealth_modifier(r, f, seen_travel));
    CuAssertIntEquals(tc, -1, stealth_modifier(r, f, seen_lighthouse));
    CuAssertIntEquals(tc, -1, stealth_modifier(r, f, seen_spell));

    set_observer(r, f, 10, 1);
    CuAssertIntEquals(tc, 10, stealth_modifier(r, f, seen_spell));
    CuAssertIntEquals(tc, -1, stealth_modifier(r, NULL, seen_spell));
    test_teardown();
}

static void test_insect_warnings(CuTest *tc) {
    faction *f;
    gamedate gd;

    test_setup();
    test_create_calendar();
    f = test_create_faction_ex(test_create_race("insect"), NULL);

    CuAssertIntEquals(tc, SEASON_AUTUMN, get_gamedate(1083, &gd)->season);
    report_warnings(f, gd.turn);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "nr_insectfall"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "nr_insectwinter"));
    test_clear_messages(f);

    CuAssertIntEquals(tc, SEASON_AUTUMN, get_gamedate(1082, &gd)->season);
    report_warnings(f, gd.turn);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "nr_insectfall"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "nr_insectwinter"));
    test_clear_messages(f);

    CuAssertIntEquals(tc, SEASON_WINTER, get_gamedate(1084, &gd)->season);
    report_warnings(f, gd.turn);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "nr_insectfall"));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "nr_insectwinter"));
    test_clear_messages(f);

    test_teardown();
}

static void test_newbie_warning(CuTest *tc) {
    faction *f;

    test_setup();
    f = test_create_faction();
    config_set_int("NewbieImmunity", 3);

    f->age = 0;
    report_warnings(f, 0);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "newbieimmunity"));
    test_clear_messages(f);

    f->age = 1;
    report_warnings(f, 0);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "newbieimmunityending"));
    test_clear_messages(f);

    f->age = 2;
    report_warnings(f, 0);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "newbieimmunityended"));
    test_clear_messages(f);

    f->age = 3;
    report_warnings(f, 0);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "newbieimmunityended"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "newbieimmunityending"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "newbieimmunity"));
    test_clear_messages(f);

    test_teardown();
}

static void test_visible_unit(CuTest* tc) {
    unit* u;
    faction* f;
    ship* sh;
    building* b;
    race* rc;

    test_setup();
    f = test_create_faction();
    rc = test_create_race("smurf");
    rc->flags |= RCF_UNARMEDGUARD;

    /* visibility on land */
    u = test_create_unit(test_create_faction_ex(rc, NULL), test_create_plain(0, 0));
    test_create_unit(f, u->region);
    CuAssertTrue(tc, cansee(f, u->region, u, 0));
    CuAssertTrue(tc, visible_unit(u, f, 0, seen_unit));
    CuAssertTrue(tc, visible_unit(u, f, 0, seen_spell));
    CuAssertTrue(tc, visible_unit(u, f, 0, seen_battle));
    CuAssertTrue(tc, visible_unit(u, f, 0, seen_travel));
    CuAssertTrue(tc, !visible_unit(u, f, 0, seen_none));
    CuAssertTrue(tc, !visible_unit(u, f, 0, seen_neighbour));
    CuAssertTrue(tc, !visible_unit(u, f, 0, seen_lighthouse_land));
    /* weight makes no difference on land */
    rc->weight = 5000;
    CuAssertTrue(tc, !visible_unit(u, f, 0, seen_none));
    CuAssertTrue(tc, !visible_unit(u, f, 0, seen_neighbour));
    CuAssertTrue(tc, !visible_unit(u, f, 0, seen_lighthouse_land));
    /* stealth makes you invisible */
    set_level(u, SK_STEALTH, 2);
    CuAssertTrue(tc, !visible_unit(u, f, 0, seen_unit));
    CuAssertTrue(tc, !visible_unit(u, f, 0, seen_travel));

    /* visibility of stealthed units in oceans */
    u = test_create_unit(u->faction, test_create_ocean(0, 1));
    test_create_unit(f, u->region);
    set_level(u, SK_STEALTH, 2);
    CuAssertTrue(tc, !visible_unit(u, f, 0, seen_none));
    CuAssertTrue(tc, !visible_unit(u, f, 0, seen_neighbour));
    CuAssertTrue(tc, visible_unit(u, f, 0, seen_unit));
    CuAssertTrue(tc, visible_unit(u, f, 0, seen_travel));
    CuAssertTrue(tc, visible_unit(u, f, 0, seen_lighthouse));
    rc->weight = 4999;
    CuAssertTrue(tc, !visible_unit(u, f, 0, seen_none));
    CuAssertTrue(tc, !visible_unit(u, f, 0, seen_neighbour));
    CuAssertTrue(tc, !visible_unit(u, f, 0, seen_travel));
    CuAssertTrue(tc, !visible_unit(u, f, 0, seen_lighthouse));
    CuAssertTrue(tc, !visible_unit(u, f, 0, seen_unit));
    u->ship = sh = test_create_ship(u->region, NULL);
    CuAssertTrue(tc, !visible_unit(u, f, 0, seen_none));
    CuAssertTrue(tc, !visible_unit(u, f, 0, seen_neighbour));
    CuAssertTrue(tc, visible_unit(u, f, 0, seen_travel));
    CuAssertTrue(tc, visible_unit(u, f, 0, seen_lighthouse));
    CuAssertTrue(tc, visible_unit(u, f, 0, seen_unit));
    u->ship = NULL;

    /* guards can always be seen */
    setguard(u, true);
    CuAssertTrue(tc, is_guard(u));
    CuAssertTrue(tc, visible_unit(u, f, 0, seen_unit));
    CuAssertTrue(tc, visible_unit(u, f, 0, seen_travel));
    CuAssertTrue(tc, visible_unit(u, f, 0, seen_lighthouse));
    setguard(u, false);

    u->building = b = test_create_building(u->region, NULL);
    CuAssertTrue(tc, visible_unit(u, f, 0, seen_travel));
    CuAssertTrue(tc, visible_unit(u, f, 0, seen_lighthouse));
    u->building = NULL;

    set_level(u, SK_STEALTH, 1);
    CuAssertTrue(tc, !cansee(f, u->region, u, 0));
    CuAssertTrue(tc, cansee(f, u->region, u, 1));

    u->ship = sh;
    CuAssertTrue(tc, visible_unit(u, f, -2, seen_lighthouse));
    CuAssertTrue(tc, visible_unit(u, f, -2, seen_travel));
    u->ship = NULL;

    u->building = b;
    CuAssertTrue(tc, visible_unit(u, f, -2, seen_lighthouse));
    CuAssertTrue(tc, visible_unit(u, f, -2, seen_travel));
    u->building = NULL;

    CuAssertTrue(tc, visible_unit(u, f, 1, seen_spell));
    CuAssertTrue(tc, visible_unit(u, f, 1, seen_battle));

    test_teardown();
}

static void test_eval_functions(CuTest *tc)
{
    message *msg;
    message_type *mtype;
    item *items = NULL;
    char buf[1024];
    struct locale * lang;

    test_setup();
    init_resources();
    test_create_itemtype("stone");
    test_create_itemtype("iron");

    lang = test_create_locale();
    locale_setstring(lang, "nr_claims", "$resources($items)");
    register_reports();
    mtype = mt_create_va(mt_new("nr_claims", NULL), "items:items", MT_NEW_END);
    nrt_register(mtype);

    msg = msg_message("nr_claims", "items", items);
    nr_render(msg, lang, buf, sizeof(buf), NULL);
    CuAssertStrEquals(tc, "", buf);
    msg_release(msg);

    i_change(&items, get_resourcetype(R_IRON)->itype, 1);
    msg = msg_message("nr_claims", "items", items);
    nr_render(msg, lang, buf, sizeof(buf), NULL);
    CuAssertStrEquals(tc, "1 Eisen", buf);
    msg_release(msg);

    i_change(&items, get_resourcetype(R_STONE)->itype, 2);
    msg = msg_message("nr_claims", "items", items);
    nr_render(msg, lang, buf, sizeof(buf), NULL);
    CuAssertStrEquals(tc, "1 Eisen, 2 Steine", buf);
    msg_release(msg);
    i_freeall(&items);

    test_teardown();
}

static void test_reports_genpassword(CuTest *tc) {
    faction *f;
    int pwid;

    test_setup();
    mt_create_va(mt_new("changepasswd", NULL), "value:string", MT_NEW_END);
    f = test_create_faction();
    CuAssertIntEquals(tc, 0, f->lastorders);
    CuAssertIntEquals(tc, 0, f->password_id);
    f->options = 0;
    /* writing the report does not change the password */
    write_reports(f, f->options, NULL);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "changepasswd"));
    /* but the main reporting function does */
    reports();
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "changepasswd"));
    CuAssertTrue(tc, f->password_id != 0);
    test_clear_messagelist(&f->msgs);
    f->lastorders = 1;
    f->age = 2;
    pwid = f->password_id;
    reports();
    CuAssertIntEquals(tc, pwid, f->password_id);
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "changepasswd"));
    test_teardown();
}

CuSuite* get_reports_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_region_distance);
    SUITE_ADD_TEST(suite, test_region_distance_max);
    SUITE_ADD_TEST(suite, test_region_distance_ql);
    DISABLE_TEST(suite, test_newbie_password_message);
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
    SUITE_ADD_TEST(suite, test_bufunit_racename_plural);
    SUITE_ADD_TEST(suite, test_bufunit_bullets);
    SUITE_ADD_TEST(suite, test_bufunit_fstealth);
    SUITE_ADD_TEST(suite, test_bufunit_help_stealth);
    SUITE_ADD_TEST(suite, test_arg_resources);
    SUITE_ADD_TEST(suite, test_insect_warnings);
    SUITE_ADD_TEST(suite, test_newbie_warning);
    SUITE_ADD_TEST(suite, test_visible_unit);
    SUITE_ADD_TEST(suite, test_eval_functions);
    SUITE_ADD_TEST(suite, test_reports_genpassword);
    return suite;
}
