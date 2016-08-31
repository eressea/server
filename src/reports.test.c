#include <platform.h>
#include <config.h>
#include "reports.h"
#include "report.h"
#include "creport.h"
#include "move.h"
#include "seen.h"
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

static void test_write_spaces(CuTest *tc) {
    stream out = { 0 };
    char buf[1024];
    size_t len;

    mstream_init(&out);
    write_spaces(&out, 4);
    out.api->rewind(out.handle);
    len = out.api->read(out.handle, buf, sizeof(buf));
    buf[len] = '\0';
    CuAssertStrEquals(tc, "    ", buf);
    CuAssertIntEquals(tc, ' ', buf[3]);
    mstream_done(&out);
}

static void test_write_many_spaces(CuTest *tc) {
    stream out = { 0 };
    char buf[1024];
    size_t len;

    mstream_init(&out);
    write_spaces(&out, 100);
    out.api->rewind(out.handle);
    len = out.api->read(out.handle, buf, sizeof(buf));
    buf[len] = '\0';
    CuAssertIntEquals(tc, 100, (int)len);
    CuAssertIntEquals(tc, ' ', buf[99]);
    mstream_done(&out);
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
    cr_output_unit(&strm, r, f, u, see_unit);
    strm.api->rewind(strm.handle);
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, line, "EINHEIT 1234");
    mstream_done(&strm);
    test_cleanup();
}

static void test_write_travelthru(CuTest *tc) {
    stream out = { 0 };
    char buf[1024];
    size_t len;
    region *r;
    faction *f;
    unit *u;
    struct locale *lang;

    test_cleanup();
    lang = get_or_create_locale("de");
    locale_setstring(lang, "travelthru_header", "Durchreise: ");
    mstream_init(&out);
    r = test_create_region(0, 0, 0);
    r->flags |= RF_TRAVELUNIT;
    f = test_create_faction(0);
    f->locale = lang;
    u = test_create_unit(f, 0);
    unit_setname(u, "Hodor");
    unit_setid(u, 1);

    write_travelthru(&out, r, f);
    out.api->rewind(out.handle);
    len = out.api->read(out.handle, buf, sizeof(buf));
    CuAssertIntEquals_Msg(tc, "no travelers, no report", 0, (int)len);
    mstream_done(&out);
    
    mstream_init(&out);
    travelthru_add(r, u);
    write_travelthru(&out, r, f);
    out.api->rewind(out.handle);
    len = out.api->read(out.handle, buf, sizeof(buf));
    buf[len] = '\0';
    CuAssertStrEquals_Msg(tc, "list one unit", "Durchreise: Hodor (1).\n", buf);
    mstream_done(&out);

    mstream_init(&out);
    move_unit(u, r, 0);
    write_travelthru(&out, r, f);
    out.api->rewind(out.handle);
    len = out.api->read(out.handle, buf, sizeof(buf));
    CuAssertIntEquals_Msg(tc, "do not list units that stopped in the region", 0, (int)len);

    mstream_done(&out);
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

typedef struct {
    struct locale *lang;
    spell *sp;
    spellbook *spb;
    spellbook_entry * sbe;
} spell_fixture;

static void setup_spell_fixture(spell_fixture * spf) {
    spf->lang = get_or_create_locale("de");
    locale_setstring(spf->lang, mkname("spell", "testspell"), "Testzauber");
    locale_setstring(spf->lang, "nr_spell_type", "Typ:");
    locale_setstring(spf->lang, "sptype_normal", "Normal");
    locale_setstring(spf->lang, "nr_spell_modifiers", "Modifier:");
    locale_setstring(spf->lang, "smod_none", "Keine");
    locale_setstring(spf->lang, keyword(K_CAST), "ZAUBERE");
    locale_setstring(spf->lang, parameters[P_REGION], "REGION");
    locale_setstring(spf->lang, parameters[P_LEVEL], "STUFE");
    locale_setstring(spf->lang, "par_unit", "enr");
    locale_setstring(spf->lang, "par_ship", "snr");
    locale_setstring(spf->lang, "par_building", "bnr");
    locale_setstring(spf->lang, "spellpar::hodor", "Hodor");

    spf->spb = create_spellbook("testbook");
    spf->sp = test_create_spell();
    spellbook_add(spf->spb, spf->sp, 1);
    spf->sbe = spellbook_get(spf->spb, spf->sp);
}

static void cleanup_spell_fixture(spell_fixture *spf) {
    spellbook_clear(spf->spb);
    free(spf->spb);
    test_cleanup();
}

static void check_spell_syntax(CuTest *tc, char *msg, spell_fixture *spell, char *syntax) {
    stream strm;
    char buf[1024];
    char *linestart, *newline;
    size_t len;

    mstream_init(&strm);
    nr_spell_syntax(&strm, spell->sbe, spell->lang);
    strm.api->rewind(strm.handle);
    len = strm.api->read(strm.handle, buf, sizeof(buf));
    buf[len] = '\0';

    linestart = strtok(buf, "\n");
    while (linestart && !strstr(linestart, "ZAUBERE"))
        linestart = strtok(NULL, "\n") ;

    CuAssertPtrNotNull(tc, linestart);

    newline = strtok(NULL, "\n");
    while (newline) {
        *(newline - 1) = '\n';
        newline = strtok(NULL, "\n");
    }

    CuAssertStrEquals_Msg(tc, msg, syntax, linestart);

    mstream_done(&strm);
}

static void set_parameter(spell_fixture spell, char *value) {
    free(spell.sp->parameter);
    spell.sp->parameter = _strdup(value);
}

static void test_write_spell_syntax(CuTest *tc) {
    spell_fixture spell;

    test_cleanup();
    setup_spell_fixture(&spell);

    check_spell_syntax(tc, "vanilla",  &spell,   "  ZAUBERE \"Testzauber\"");

    spell.sp->sptyp |= FARCASTING;
    check_spell_syntax(tc, "far",  &spell,   "  ZAUBERE [REGION x y] \"Testzauber\"");

    spell.sp->sptyp |= SPELLLEVEL;
    check_spell_syntax(tc, "farlevel",  &spell,   "  ZAUBERE [REGION x y] [STUFE n] \"Testzauber\"");
    spell.sp->sptyp = 0;

    set_parameter(spell, "kc");
    check_spell_syntax(tc, "kc", &spell,   "  ZAUBERE \"Testzauber\" ( REGION | EINHEIT <enr> | SCHIFF <snr> | BURG <bnr> )");

    spell.sp->sptyp |= BUILDINGSPELL;
    check_spell_syntax(tc, "kc typed", &spell,   "  ZAUBERE \"Testzauber\" BURG <bnr>");
    spell.sp->sptyp = 0;

    set_parameter(spell, "b");
    check_spell_syntax(tc, "b", &spell,   "  ZAUBERE \"Testzauber\" <bnr>");

    set_parameter(spell, "s");
    check_spell_syntax(tc, "s", &spell,   "  ZAUBERE \"Testzauber\" <snr>");

    set_parameter(spell, "s+");
    check_spell_syntax(tc, "s+", &spell,   "  ZAUBERE \"Testzauber\" <snr> [<snr> ...]");

    set_parameter(spell, "u");
    check_spell_syntax(tc, "u", &spell,   "  ZAUBERE \"Testzauber\" <enr>");

    set_parameter(spell, "r");
    check_spell_syntax(tc, "r", &spell,   "  ZAUBERE \"Testzauber\" <x> <y>");

    set_parameter(spell, "bc");
    free(spell.sp->syntax);
    spell.sp->syntax = _strdup("hodor");
    check_spell_syntax(tc, "bc hodor", &spell,   "  ZAUBERE \"Testzauber\" <bnr> <Hodor>");
    free(spell.sp->syntax);
    spell.sp->syntax = 0;

    set_parameter(spell, "c?");
    free(spell.sp->syntax);
    spell.sp->syntax = _strdup("hodor");
    check_spell_syntax(tc, "c?", &spell,   "  ZAUBERE \"Testzauber\" [<Hodor>]");
    free(spell.sp->syntax);
    spell.sp->syntax = 0;

    set_parameter(spell, "kc+");
    check_spell_syntax(tc, "kc+", &spell,
        "  ZAUBERE \"Testzauber\" ( REGION | EINHEIT <enr> [<enr> ...] | SCHIFF <snr>\n  [<snr> ...] | BURG <bnr> [<bnr> ...] )");

    cleanup_spell_fixture(&spell);
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
    SUITE_ADD_TEST(suite, test_write_spaces);
    SUITE_ADD_TEST(suite, test_write_many_spaces);
    SUITE_ADD_TEST(suite, test_sparagraph);
    SUITE_ADD_TEST(suite, test_write_travelthru);
    SUITE_ADD_TEST(suite, test_write_unit);
    SUITE_ADD_TEST(suite, test_write_spell_syntax);
    SUITE_ADD_TEST(suite, test_arg_resources);
    return suite;
}
