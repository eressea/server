#include <platform.h>
#include <config.h>
#include "reports.h"
#include "creport.h"
#include "move.h"
#include "travelthru.h"
#include "keyword.h"

#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/unit.h>
#include <kernel/spell.h>
#include <kernel/spellbook.h>

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

static void test_cr_unit(CuTest *tc) {
    stream strm;
    char line[1024];
    faction *f;
    region *r;
    unit *u;

    test_cleanup();
    f = test_create_faction(0);
    r = test_create_region(0, 0, 0);
    u = test_create_unit(f, r);
    renumber_unit(u, 1234);

    mstream_init(&strm);
    cr_output_unit(&strm, r, f, u, seen_unit);
    strm.api->rewind(strm.handle);
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, line, "EINHEIT 1234");
    mstream_done(&strm);
    test_cleanup();
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

CuSuite *get_reports_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_cr_unit);
    SUITE_ADD_TEST(suite, test_reorder_units);
    SUITE_ADD_TEST(suite, test_seen_faction);
    SUITE_ADD_TEST(suite, test_regionid);
    SUITE_ADD_TEST(suite, test_sparagraph);
    SUITE_ADD_TEST(suite, test_write_unit);
    SUITE_ADD_TEST(suite, test_arg_resources);
    return suite;
}
