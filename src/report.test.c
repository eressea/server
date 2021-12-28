#include <platform.h>
#include "report.h"
#include "move.h"
#include "travelthru.h"

#include <kernel/ally.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/resources.h>
#include <kernel/ship.h>
#include <kernel/unit.h>
#include <kernel/spell.h>
#include <kernel/spellbook.h>

#include "util/keyword.h"
#include "util/param.h"
#include <util/language.h>
#include <util/lists.h>
#include <util/message.h>
#include <util/strings.h>

#include <stream.h>
#include <memstream.h>

#include <CuTest.h>
#include <tests.h>

#include <string.h>

static void test_write_spaces(CuTest *tc) {
    stream out = { 0 };
    char buf[5];

    CuAssertIntEquals(tc, 0, mstream_init(&out));
    write_spaces(&out, 4);
    out.api->rewind(out.handle);
    CuAssertIntEquals(tc, EOF, out.api->read(out.handle, buf, sizeof(buf)));
    buf[4] = '\0';
    CuAssertStrEquals(tc, "    ", buf);
    mstream_done(&out);
}

static void test_write_many_spaces(CuTest *tc) {
    stream out = { 0 };
    char buf[1024];

    CuAssertIntEquals(tc, 0, mstream_init(&out));
    write_spaces(&out, 100);
    out.api->rewind(out.handle);
    CuAssertIntEquals(tc, EOF, out.api->read(out.handle, buf, sizeof(buf)));
    CuAssertIntEquals(tc, ' ', buf[99]);
    mstream_done(&out);
}

static void test_report_region(CuTest *tc) {
    char buf[1024];
    region *r;
    faction *f;
    unit *u;
    stream out = { 0 };
    struct locale *lang;
    struct resource_type *rt_stone;
    construction *cons;

    test_setup();
    init_resources();
    rt_stone = rt_get_or_create("stone");
    rt_stone->itype = it_get_or_create(rt_stone);
    cons = rt_stone->itype->construction = calloc(1, sizeof(construction));
    cons->minskill = 1;
    cons->skill = SK_QUARRYING;
    rmt_create(rt_stone);

    lang = get_or_create_locale("de"); /* CR tags are translated from this */
    locale_setstring(lang, "money", "Silber");
    locale_setstring(lang, "money_p", "Silber");
    locale_setstring(lang, "horse", "Pferd");
    locale_setstring(lang, "horse_p", "Pferde");
    locale_setstring(lang, "peasant", "Bauer");
    locale_setstring(lang, "peasant_p", "Bauern");
    locale_setstring(lang, "tree", "Blume");
    locale_setstring(lang, "tree_p", "Blumen");
    locale_setstring(lang, "stone", "Stein");
    locale_setstring(lang, "stone_p", "Steine");
    locale_setstring(lang, "sapling", "Schoessling");
    locale_setstring(lang, "sapling_p", "Schoesslinge");
    locale_setstring(lang, "plain", "Ebene");
    locale_setstring(lang, "see_travel", "durchgereist");

    CuAssertIntEquals(tc, 0, mstream_init(&out));
    r = test_create_region(0, 0, NULL);
    add_resource(r, 1, 135, 10, rt_stone);
    CuAssertIntEquals(tc, 1, r->resources->level);
    r->land->peasants = 5;
    r->land->horses = 7;
    r->land->money = 2;
    rsettrees(r, 0, 1);
    rsettrees(r, 1, 2);
    rsettrees(r, 2, 3);
    f = test_create_faction_ex(NULL, lang);
    u = test_create_unit(f, r);
    set_level(u, SK_QUARRYING, 1);

    region_setname(r, "1234567890123456789012345678901234567890");
    r->seen.mode = seen_travel;
    report_region(&out, r, f);
    out.api->write(out.handle, "", 1);
    out.api->rewind(out.handle);
    CuAssertIntEquals(tc, EOF, out.api->read(out.handle, buf, sizeof(buf)));
    CuAssertStrEquals(tc, "1234567890123456789012345678901234567890 (0,0) (durchgereist), Ebene, 3/2\nBlumen, 5 Bauern, 2 Silber, 7 Pferde.\n", buf);

    out.api->rewind(out.handle);
    region_setname(r, "12345678901234567890123456789012345678901234567890123456789012345678901234567890");
    r->seen.mode = seen_travel;
    report_region(&out, r, f);
    out.api->write(out.handle, "", 1);
    out.api->rewind(out.handle);
    CuAssertIntEquals(tc, EOF, out.api->read(out.handle, buf, sizeof(buf)));
    CuAssertStrEquals(tc, "12345678901234567890123456789012345678901234567890123456789012345678901234567890\n(0,0) (durchgereist), Ebene, 3/2 Blumen, 5 Bauern, 2 Silber, 7 Pferde.\n", buf);

    out.api->rewind(out.handle);
    region_setname(r, "Hodor");
    r->seen.mode = seen_travel;
    report_region(&out, r, f);
    out.api->write(out.handle, "", 1);
    out.api->rewind(out.handle);
    CuAssertIntEquals(tc, EOF, out.api->read(out.handle, buf, sizeof(buf)));
    CuAssertStrEquals(tc, "Hodor (0,0) (durchgereist), Ebene, 3/2 Blumen, 5 Bauern, 2 Silber, 7 Pferde.\n", buf);

    out.api->rewind(out.handle);
    r->seen.mode = seen_unit;
    report_region(&out, r, f);
    out.api->write(out.handle, "", 1);
    out.api->rewind(out.handle);
    CuAssertIntEquals(tc, EOF, out.api->read(out.handle, buf, sizeof(buf)));
    CuAssertStrEquals(tc, "Hodor (0,0), Ebene, 3/2 Blumen, 135 Steine/1, 5 Bauern, 2 Silber, 7 Pferde.\n", buf);

    out.api->rewind(out.handle);
    r->resources->amount = 1;
    r->land->peasants = 1;
    r->land->horses = 1;
    r->land->money = 1;
    r->seen.mode = seen_unit;
    report_region(&out, r, f);
    out.api->write(out.handle, "", 1);
    out.api->rewind(out.handle);
    CuAssertIntEquals(tc, EOF, out.api->read(out.handle, buf, sizeof(buf)));
    CuAssertStrEquals(tc, "Hodor (0,0), Ebene, 3/2 Blumen, 1 Stein/1, 1 Bauer, 1 Silber, 1 Pferd.\n", buf);

    r->land->peasants = 0;
    r->land->horses = 0;
    r->land->money = 0;
    rsettrees(r, 0, 0);
    rsettrees(r, 1, 0);
    rsettrees(r, 2, 0);

    out.api->rewind(out.handle);
    report_region(&out, r, f);
    out.api->write(out.handle, "", 1);
    out.api->rewind(out.handle);
    CuAssertIntEquals(tc, EOF, out.api->read(out.handle, buf, sizeof(buf)));
    CuAssertStrEquals(tc, "Hodor (0,0), Ebene, 1 Stein/1.\n", buf);

    mstream_done(&out);
    test_teardown();
}

static void test_report_allies(CuTest *tc) {
    stream out = { 0 };
    char buf[1024];
    char exp[1024];
    size_t linebreak = 72;
    struct locale *lang;
    faction *f, *f1, *f2, *f3;

    test_setup();
    lang = test_create_locale();
    locale_setstring(lang, "list_and", " und ");
    CuAssertIntEquals(tc, 0, mstream_init(&out));
    f = test_create_faction_ex(NULL, lang);
    f1 = test_create_faction_ex(NULL, lang);
    f2 = test_create_faction_ex(NULL, lang);
    f3 = test_create_faction_ex(NULL, lang);
    snprintf(exp, sizeof(exp), "Wir helfen %s (%s).\n\n",
        factionname(f1),
        LOC(lang, parameters[P_GUARD]));
    ally_set(&f->allies, f1, HELP_GUARD);
    report_allies(&out, linebreak, f, f->allies, "Wir helfen ");
    out.api->write(out.handle, "", 1);
    out.api->rewind(out.handle);
    CuAssertIntEquals(tc, EOF, out.api->read(out.handle, buf, sizeof(buf)));
    CuAssertStrEquals(tc, exp, buf);

    out.api->rewind(out.handle);
    ally_set(&f->allies, f2, HELP_GIVE);
    ally_set(&f->allies, f3, HELP_ALL);
    snprintf(exp, sizeof(exp), "Wir helfen %s (%s), %s (%s)",
        factionname(f1),
        LOC(lang, parameters[P_GUARD]),
        factionname(f2),
        LOC(lang, parameters[P_GIVE]));
    linebreak = strlen(exp);
    snprintf(exp, sizeof(exp), "Wir helfen %s (%s), %s (%s)\nund %s (%s).\n\n",
        factionname(f1),
        LOC(lang, parameters[P_GUARD]),
        factionname(f2),
        LOC(lang, parameters[P_GIVE]),
        factionname(f3),
        LOC(lang, parameters[P_ANY]));
    report_allies(&out, linebreak, f, f->allies, "Wir helfen ");
    out.api->write(out.handle, "", 1);
    out.api->rewind(out.handle);
    CuAssertIntEquals(tc, EOF, out.api->read(out.handle, buf, sizeof(buf)));
    CuAssertStrEquals(tc, exp, buf);

    test_teardown();
}

static void test_report_travelthru(CuTest *tc) {
    stream out = { 0 };
    char buf[1024];
    region *r;
    faction *f;
    unit *u;
    struct locale *lang;

    test_setup();
    lang = get_or_create_locale("de");
    locale_setstring(lang, "travelthru_header", "Durchreise: ");
    locale_setstring(lang, "list_and", " und ");
    CuAssertIntEquals(tc, 0, mstream_init(&out));
    r = test_create_region(0, 0, NULL);
    r->flags |= RF_TRAVELUNIT;
    f = test_create_faction();
    f->locale = lang;
    u = test_create_unit(f, test_create_region(0, 1, NULL));
    unit_setname(u, "Hodor");
    unit_setid(u, 1);

    buf[0] = '\0';
    report_travelthru(&out, r, f);
    out.api->write(out.handle, "", 1);
    out.api->rewind(out.handle);
    CuAssertIntEquals(tc, EOF, out.api->read(out.handle, buf, sizeof(buf)));
    CuAssertStrEquals_Msg(tc, "no travelers, no report", "", buf);
    mstream_done(&out);

    CuAssertIntEquals(tc, 0, mstream_init(&out));
    travelthru_add(r, u);
    report_travelthru(&out, r, f);
    out.api->write(out.handle, "", 1);
    out.api->rewind(out.handle);
    CuAssertIntEquals(tc, EOF, out.api->read(out.handle, buf, sizeof(buf)));
    CuAssertStrEquals_Msg(tc, "list one unit", "\nDurchreise: Hodor (1).\n", buf);
    mstream_done(&out);
    buf[0] = '\0';

    CuAssertIntEquals(tc, 0, mstream_init(&out));
    travelthru_add(r, u);
    travelthru_add(r, u);
    report_travelthru(&out, r, f);
    out.api->write(out.handle, "", 1);
    out.api->rewind(out.handle);
    CuAssertIntEquals(tc, EOF, out.api->read(out.handle, buf, sizeof(buf)));
    CuAssertStrEquals_Msg(tc, "list three units", "\nDurchreise: Hodor (1), Hodor (1) und Hodor (1).\n", buf);
    mstream_done(&out);
    buf[0] = '\0';

    CuAssertIntEquals(tc, 0, mstream_init(&out));
    move_unit(u, r, NULL);
    report_travelthru(&out, r, f);
    out.api->write(out.handle, "", 1);
    out.api->rewind(out.handle);
    CuAssertIntEquals(tc, EOF, out.api->read(out.handle, buf, sizeof(buf)));
    CuAssertStrEquals_Msg(tc, "do not list units that stopped in the region", "", buf);

    mstream_done(&out);
    test_teardown();
}

typedef struct {
    struct locale *lang;
    spell *sp;
    spellbook *spb;
    spellbook_entry * sbe;
} spell_fixture;

static void setup_spell_fixture(spell_fixture * spf) {
    spf->lang = get_or_create_locale(__FUNCTION__);
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
}

static void set_parameter(spell_fixture spell, char *value) {
    free(spell.sp->parameter);
    spell.sp->parameter = str_strdup(value);
}

static void check_spell_syntax(CuTest *tc, char *msg, spell_fixture *spell, char *syntax) {
    char buf[1024];

    nr_spell_syntax(buf, sizeof(buf), spell->sbe, spell->lang);

    CuAssertStrEquals_Msg(tc, msg, syntax, buf);
}

static void test_write_spell_syntax(CuTest *tc) {
    spell_fixture spell;

    test_setup();
    setup_spell_fixture(&spell);

    check_spell_syntax(tc, "vanilla", &spell, "ZAUBERE Testzauber");

    spell.sp->sptyp |= FARCASTING;
    check_spell_syntax(tc, "far", &spell, "ZAUBERE [REGION x y] Testzauber");

    spell.sp->sptyp |= SPELLLEVEL;
    check_spell_syntax(tc, "farlevel", &spell, "ZAUBERE [REGION x y] [STUFE n] Testzauber");
    spell.sp->sptyp = 0;

    set_parameter(spell, "kc");
    check_spell_syntax(tc, "kc", &spell, "ZAUBERE Testzauber ( REGION | EINHEIT <enr> | SCHIFF <snr> | BURG <bnr> )");

    spell.sp->sptyp |= BUILDINGSPELL;
    check_spell_syntax(tc, "kc typed", &spell, "ZAUBERE Testzauber BURG <bnr>");
    spell.sp->sptyp = 0;

    set_parameter(spell, "b");
    check_spell_syntax(tc, "b", &spell, "ZAUBERE Testzauber <bnr>");

    set_parameter(spell, "s");
    check_spell_syntax(tc, "s", &spell, "ZAUBERE Testzauber <snr>");

    set_parameter(spell, "s+");
    check_spell_syntax(tc, "s+", &spell, "ZAUBERE Testzauber <snr> [<snr> ...]");

    set_parameter(spell, "u");
    check_spell_syntax(tc, "u", &spell, "ZAUBERE Testzauber <enr>");

    set_parameter(spell, "r");
    check_spell_syntax(tc, "r", &spell, "ZAUBERE Testzauber <x> <y>");

    set_parameter(spell, "bc");
    free(spell.sp->syntax);
    spell.sp->syntax = str_strdup("hodor");
    check_spell_syntax(tc, "bc hodor", &spell, "ZAUBERE Testzauber <bnr> <Hodor>");
    free(spell.sp->syntax);
    spell.sp->syntax = 0;

    set_parameter(spell, "c?");
    free(spell.sp->syntax);
    spell.sp->syntax = str_strdup("hodor");
    check_spell_syntax(tc, "c?", &spell, "ZAUBERE Testzauber [<Hodor>]");
    free(spell.sp->syntax);
    spell.sp->syntax = 0;

    set_parameter(spell, "kc+");
    check_spell_syntax(tc, "kc+", &spell,
        "ZAUBERE Testzauber ( REGION | EINHEIT <enr> [<enr> ...] | SCHIFF <snr> [<snr> ...] | BURG <bnr> [<bnr> ...] )");

    cleanup_spell_fixture(&spell);
    test_teardown();
}

static void test_paragraph(CuTest *tc) {
    const char *toolong = "im Westen das Hochland von Geraldin (93,-303).";
    const char *expect = "im Westen das Hochland von Geraldin (93,-303).\n";
    char buf[256];
    stream out = { 0 };

    CuAssertIntEquals(tc, 0, mstream_init(&out));
    paragraph(&out, toolong, 0, 0, 0);
    out.api->write(out.handle, "", 1);
    out.api->rewind(out.handle);
    CuAssertIntEquals(tc, EOF, out.api->read(out.handle, buf, sizeof(buf)));
    CuAssertStrEquals(tc, expect, buf);
}

static void test_paragraph_break(CuTest *tc) {
    const char *toolong = "die Ebene von Godsettova (94,-304) und im Westen das Hochland von Geraldin (93,-303).";
    const char *expect = "die Ebene von Godsettova (94,-304) und im Westen das Hochland von Geraldin\n(93,-303).\n";
    char buf[256];
    stream out = { 0 };

    CuAssertIntEquals(tc, 0, mstream_init(&out));
    paragraph(&out, toolong, 0, 0, 0);
    out.api->write(out.handle, "", 1);
    out.api->rewind(out.handle);
    CuAssertIntEquals(tc, EOF, out.api->read(out.handle, buf, sizeof(buf)));
    CuAssertStrEquals(tc, expect, buf);
}

static void test_pump_paragraph_toolong(CuTest *tc) {
    const char *toolong = "die Ebene von Godsettova (94,-304) und im Westen das Hochland von Geraldin (93,-303).";
    const char *expect = "die Ebene von Godsettova (94,-304) und im Westen das Hochland von Geraldin\n(93,-303).\n";
    sbstring sbs;
    char buf[256];
    stream out = { 0 };

    CuAssertIntEquals(tc, 0, mstream_init(&out));
    sbs_init(&sbs, buf, sizeof(buf));
    sbs_strcat(&sbs, toolong);

    pump_paragraph(&sbs, &out, 78, true);
    out.api->write(out.handle, "\0", 1);
    out.api->rewind(out.handle);
    CuAssertIntEquals(tc, EOF, out.api->read(out.handle, buf, sizeof(buf)));
    CuAssertStrEquals(tc, expect, buf);
}

CuSuite *get_report_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_write_spaces);
    SUITE_ADD_TEST(suite, test_write_many_spaces);
    SUITE_ADD_TEST(suite, test_paragraph);
    SUITE_ADD_TEST(suite, test_paragraph_break);
    SUITE_ADD_TEST(suite, test_pump_paragraph_toolong);
    SUITE_ADD_TEST(suite, test_report_travelthru);
    SUITE_ADD_TEST(suite, test_report_region);
    SUITE_ADD_TEST(suite, test_report_allies);
    SUITE_ADD_TEST(suite, test_write_spell_syntax);
    return suite;
}
