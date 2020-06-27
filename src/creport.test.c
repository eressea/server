#include <platform.h>
#include "creport.h"
#include "move.h"
#include "spy.h"
#include "travelthru.h"

#include "attributes/racename.h"

#include "util/keyword.h"
#include <util/language.h>
#include <util/lists.h>
#include <util/message.h>

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

#include <stream.h>
#include <memstream.h>

#include <CuTest.h>
#include <tests.h>

#include <string.h>

static void test_cr_unit(CuTest *tc) {
    stream strm;
    char line[1024];
    faction *f;
    region *r;
    unit *u;

    test_setup();
    f = test_create_faction(NULL);
    r = test_create_region(0, 0, NULL);
    u = test_create_unit(f, r);
    renumber_unit(u, 1234);

    mstream_init(&strm);
    cr_output_unit(&strm, f, u, seen_unit);
    strm.api->rewind(strm.handle);
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, line, "EINHEIT 1234");
    mstream_done(&strm);
    test_teardown();
}

static void setup_resources(void) {
    struct locale *lang;
    item_type *itype;

    test_setup();
    itype = it_get_or_create(rt_get_or_create("stone"));
    itype->rtype->flags = RTF_LIMITED | RTF_POOLED;
    itype->construction = calloc(1, sizeof(construction));
    itype->construction->skill = SK_QUARRYING;
    itype->construction->minskill = 1;
    rmt_create(itype->rtype);
    init_resources();
    lang = get_or_create_locale("de"); /* CR tags are translated from this */
    locale_setstring(lang, "money", "Silber");
    locale_setstring(lang, "money_p", "Silber");
    locale_setstring(lang, "horse", "Pferd");
    locale_setstring(lang, "horse_p", "Pferde");
    locale_setstring(lang, "peasant", "Bauer");
    locale_setstring(lang, "peasant_p", "Bauern");
    locale_setstring(lang, "stone", "Stein");
    locale_setstring(lang, "stone_p", "Steine");
    locale_setstring(lang, "tree", "Blume");
    locale_setstring(lang, "tree_p", "Blumen");
    locale_setstring(lang, "sapling", "Schoessling");
    locale_setstring(lang, "sapling_p", "Schoesslinge");
    locale_setstring(lang, "mallornsapling", "Mallornschoessling");
    locale_setstring(lang, "mallornsapling_p", "Mallornschoesslinge");
    locale_setstring(lang, "mallorntree", "Mallorn");
    locale_setstring(lang, "mallorntree_p", "Mallorn");
}

static void test_cr_resources(CuTest *tc) {
    stream strm;
    char line[1024];
    faction *f;
    region *r;
    unit *u;

    setup_resources();

    f = test_create_faction(NULL);
    r = test_create_region(0, 0, NULL);
    u = test_create_unit(f, r);
    set_level(u, SK_QUARRYING, 1);
    r->land->horses = 1;
    r->land->peasants = 200;
    r->land->money = 300;
    rsettrees(r, 0, 1);
    rsettrees(r, 1, 2);
    rsettrees(r, 2, 3);
    region_setresource(r, get_resourcetype(R_STONE), 1);

    mstream_init(&strm);
    cr_output_resources(&strm, f, r, true);
    strm.api->rewind(strm.handle);
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "3;Baeume", line);
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "2;Schoesslinge", line);
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "1;Steine", line);

    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertIntEquals(tc, 0, memcmp(line, "RESOURCE ", 9));
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "\"Schoesslinge\";type", line);
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "2;number", line);

    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertIntEquals(tc, 0, memcmp(line, "RESOURCE ", 9));
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "\"Blumen\";type", line);
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "3;number", line);

    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertIntEquals(tc, 0, memcmp(line, "RESOURCE ", 9));
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "\"Silber\";type", line);
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "300;number", line);

    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertIntEquals(tc, 0, memcmp(line, "RESOURCE ", 9));
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "\"Bauern\";type", line);
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "200;number", line);

    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertIntEquals(tc, 0, memcmp(line, "RESOURCE ", 9));
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "\"Pferde\";type", line);
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "1;number", line);

    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertIntEquals(tc, 0, memcmp(line, "RESOURCE ", 9));
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "\"Steine\";type", line);
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "1;skill", line);
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "1;number", line);

    mstream_done(&strm);
    test_teardown();
}

static void test_cr_mallorn(CuTest *tc) {
    stream strm;
    char line[1024];
    faction *f;
    region *r;
    
    setup_resources();

    f = test_create_faction(NULL);
    r = test_create_region(0, 0, NULL);
    r->land->horses = 1;
    r->land->peasants = 200;
    r->land->money = 300;
    r->flags |= RF_MALLORN;
    rsettrees(r, 0, 1);
    rsettrees(r, 1, 2);
    rsettrees(r, 2, 3);

    mstream_init(&strm);
    cr_output_resources(&strm, f, r, false);
    strm.api->rewind(strm.handle);
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "3;Baeume", line);
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "2;Schoesslinge", line);
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "1;Mallorn", line);

    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertIntEquals(tc, 0, memcmp(line, "RESOURCE ", 9));
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "\"Mallornschoesslinge\";type", line);
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "2;number", line);

    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertIntEquals(tc, 0, memcmp(line, "RESOURCE ", 9));
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "\"Mallorn\";type", line);
    CuAssertIntEquals(tc, 0, strm.api->readln(strm.handle, line, sizeof(line)));
    CuAssertStrEquals(tc, "3;number", line);

    mstream_done(&strm);
    test_teardown();
}

static int cr_get_int(stream *strm, const char *match, int def)
{
    char line[1024];
    size_t len = strlen(match);

    strm->api->rewind(strm->handle);
    while (strm->api->readln(strm->handle, line, sizeof(line))==0) {
        const char * pos = strstr(line, match);
        if (pos && pos[len]=='\0') {
            return atoi(line);
        }
    }
    return def;
}

static bool cr_find_string(stream *strm, const char *match, const char *value)
{
    char line[1024];
    size_t len = strlen(match);

    strm->api->rewind(strm->handle);
    while (strm->api->readln(strm->handle, line, sizeof(line))==0) {
        if (line[0] == '\"') {
            const char * pos = strstr(line + 1, match);
            if (pos && pos[len] == '\0') {
                size_t vlen = strlen(value);
                if (value != NULL && strncmp(line + 1, value, vlen) == 0) {
                    return (line[vlen + 1] == '\"');
                }
                return false;
            }
        }
    }
    return value == NULL;
}

static void test_cr_hiderace(CuTest *tc) {
    stream strm;
    faction *f1, *f2;
    region *r;
    unit *u;
    struct locale * lang;

    test_setup();
    default_locale = test_create_locale();
    locale_setstring(default_locale, "race::elf_p", "Elfen");
    locale_setstring(default_locale, "race::elf", "elf");
    locale_setstring(default_locale, "race::human_p", "Menschen");
    locale_setstring(default_locale, "race::human", "Mensch");
    lang = get_or_create_locale("en");
    locale_setstring(lang, "race::human_p", "humans");
    locale_setstring(lang, "race::human", "human");
    locale_setstring(lang, "race::elf_p", "elves");
    locale_setstring(lang, "race::elf", "elf");
    f1 = test_create_faction(NULL);
    f2 = test_create_faction(NULL);
    r = test_create_region(0, 0, NULL);
    u = test_create_unit(f1, r);

    mstream_init(&strm);
    CuAssertPtrEquals(tc, default_locale, (struct locale *)f1->locale);
    CuAssertPtrEquals(tc, default_locale, (struct locale *)f2->locale);
    cr_output_unit(&strm, f1, u, seen_unit);
    CuAssertTrue(tc, cr_find_string(&strm, ";Typ", "Menschen"));
    CuAssertTrue(tc, cr_find_string(&strm, ";wahrerTyp", NULL));
    mstream_done(&strm);
    mstream_init(&strm);
    CuAssertPtrEquals(tc, default_locale, (struct locale *)f1->locale);
    CuAssertPtrEquals(tc, default_locale, (struct locale *)f2->locale);
    cr_output_unit(&strm, f2, u, seen_unit);
    CuAssertTrue(tc, cr_find_string(&strm, ";Typ", "Menschen"));
    CuAssertTrue(tc, cr_find_string(&strm, ";wahrerTyp", NULL));
    mstream_done(&strm);

    /* when we use irace, the owner can see the true race */
    f1->locale = lang;
    u->irace = test_create_race("elf");
    mstream_init(&strm);
    cr_output_unit(&strm, f1, u, seen_unit);
    CuAssertTrue(tc, cr_find_string(&strm, ";Typ", "Elfen"));
    CuAssertTrue(tc, cr_find_string(&strm, ";wahrerTyp", "Menschen"));
    mstream_done(&strm);
    mstream_init(&strm);
    cr_output_unit(&strm, f2, u, seen_unit);
    CuAssertTrue(tc, cr_find_string(&strm, ";Typ", "Elfen"));
    CuAssertTrue(tc, cr_find_string(&strm, ";wahrerTyp", NULL));
    mstream_done(&strm);

    /* when we use racename, nobody can tell it's not the real deal */
    u->irace = NULL;
    set_racename(&u->attribs, "Zwerge");
    mstream_init(&strm);
    cr_output_unit(&strm, f1, u, seen_unit);
    CuAssertTrue(tc, cr_find_string(&strm, ";Typ", "Zwerge"));
    CuAssertTrue(tc, cr_find_string(&strm, ";wahrerTyp", NULL));
    mstream_done(&strm);
    mstream_init(&strm);
    cr_output_unit(&strm, f2, u, seen_unit);
    CuAssertTrue(tc, cr_find_string(&strm, ";Typ", "Zwerge"));
    CuAssertTrue(tc, cr_find_string(&strm, ";wahrerTyp", NULL));
    mstream_done(&strm);

    test_teardown();
}

static void test_cr_factionstealth(CuTest *tc) {
    stream strm;
    faction *f1, *f2;
    region *r;
    unit *u;

    test_setup();
    f1 = test_create_faction(NULL);
    f2 = test_create_faction(NULL);
    r = test_create_region(0, 0, NULL);
    u = test_create_unit(f1, r);

    /* report to ourselves */
    mstream_init(&strm);
    cr_output_unit(&strm, f1, u, seen_unit);
    CuAssertIntEquals(tc, f1->no, cr_get_int(&strm, ";Partei", -1));
    CuAssertIntEquals(tc, -1, cr_get_int(&strm, ";Anderepartei", -1));
    CuAssertIntEquals(tc, -1, cr_get_int(&strm, ";Verraeter", -1));
    CuAssertIntEquals(tc, -1, cr_get_int(&strm, ";Parteitarnung", -1));
    mstream_done(&strm);

    /* ... also when we are anonymous */
    u->flags |= UFL_ANON_FACTION;
    mstream_init(&strm);
    cr_output_unit(&strm, f1, u, seen_unit);
    CuAssertIntEquals(tc, f1->no, cr_get_int(&strm, ";Partei", -1));
    CuAssertIntEquals(tc, -1, cr_get_int(&strm, ";Anderepartei", -1));
    CuAssertIntEquals(tc, -1, cr_get_int(&strm, ";Verraeter", -1));
    CuAssertIntEquals(tc, 1, cr_get_int(&strm, ";Parteitarnung", -1));
    u->flags &= ~UFL_ANON_FACTION;
    mstream_done(&strm);

    /* we see that our unit is cloaked */
    set_factionstealth(u, f2);
    CuAssertPtrNotNull(tc, u->attribs);
    mstream_init(&strm);
    cr_output_unit(&strm, f1, u, seen_unit);
    CuAssertIntEquals(tc, f1->no, cr_get_int(&strm, ";Partei", -1));
    CuAssertIntEquals(tc, f2->no, cr_get_int(&strm, ";Anderepartei", -1));
    CuAssertIntEquals(tc, -1, cr_get_int(&strm, ";Verraeter", -1));
    CuAssertIntEquals(tc, -1, cr_get_int(&strm, ";Parteitarnung", -1));
    mstream_done(&strm);

    /* ... also when we are anonymous */
    u->flags |= UFL_ANON_FACTION;
    mstream_init(&strm);
    cr_output_unit(&strm, f1, u, seen_unit);
    CuAssertIntEquals(tc, f1->no, cr_get_int(&strm, ";Partei", -1));
    CuAssertIntEquals(tc, f2->no, cr_get_int(&strm, ";Anderepartei", -1));
    CuAssertIntEquals(tc, -1, cr_get_int(&strm, ";Verraeter", -1));
    CuAssertIntEquals(tc, 1, cr_get_int(&strm, ";Parteitarnung", -1));
    u->flags &= ~UFL_ANON_FACTION;
    mstream_done(&strm);

    /* we can tell that someone is presenting as us */
    mstream_init(&strm);
    cr_output_unit(&strm, f2, u, seen_unit);
    CuAssertIntEquals(tc, f2->no, cr_get_int(&strm, ";Partei", -1));
    CuAssertIntEquals(tc, -1, cr_get_int(&strm, ";Anderepartei", -1));
    CuAssertIntEquals(tc, 1, cr_get_int(&strm, ";Verraeter", -1));
    CuAssertIntEquals(tc, -1, cr_get_int(&strm, ";Parteitarnung", -1));
    mstream_done(&strm);

    /* ... but not if they are anonymous */
    u->flags |= UFL_ANON_FACTION;
    mstream_init(&strm);
    cr_output_unit(&strm, f2, u, seen_unit);
    CuAssertIntEquals(tc, -1, cr_get_int(&strm, ";Partei", -1));
    CuAssertIntEquals(tc, -1, cr_get_int(&strm, ";Anderepartei", -1));
    CuAssertIntEquals(tc, -1, cr_get_int(&strm, ";Verraeter", -1));
    CuAssertIntEquals(tc, 1, cr_get_int(&strm, ";Parteitarnung", -1));
    u->flags &= ~UFL_ANON_FACTION;
    mstream_done(&strm);

    /* we see the same thing as them when we are an ally */
    ally_set(&f1->allies, f2, HELP_FSTEALTH);
    mstream_init(&strm);
    cr_output_unit(&strm, f2, u, seen_unit);
    CuAssertIntEquals(tc, f1->no, cr_get_int(&strm, ";Partei", -1));
    CuAssertIntEquals(tc, f2->no, cr_get_int(&strm, ";Anderepartei", -1));
    CuAssertIntEquals(tc, -1, cr_get_int(&strm, ";Verraeter", -1));
    CuAssertIntEquals(tc, -1, cr_get_int(&strm, ";Parteitarnung", -1));
    mstream_done(&strm);

    /* ... also when they are anonymous */
    u->flags |= UFL_ANON_FACTION;
    mstream_init(&strm);
    cr_output_unit(&strm, f2, u, seen_unit);
    CuAssertIntEquals(tc, f1->no, cr_get_int(&strm, ";Partei", -1));
    CuAssertIntEquals(tc, f2->no, cr_get_int(&strm, ";Anderepartei", -1));
    CuAssertIntEquals(tc, -1, cr_get_int(&strm, ";Verraeter", -1));
    CuAssertIntEquals(tc, 1, cr_get_int(&strm, ";Parteitarnung", -1));
    u->flags &= ~UFL_ANON_FACTION;
    mstream_done(&strm);

    test_teardown();
}

CuSuite *get_creport_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_cr_unit);
    SUITE_ADD_TEST(suite, test_cr_resources);
    SUITE_ADD_TEST(suite, test_cr_mallorn);
    SUITE_ADD_TEST(suite, test_cr_hiderace);
    SUITE_ADD_TEST(suite, test_cr_factionstealth);
    return suite;
}
