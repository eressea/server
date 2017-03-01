#include <platform.h>
#include "creport.h"
#include "move.h"
#include "spy.h"
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

static int cr_get_int(stream *strm, const char *match, int def)
{
    char line[1024];

    strm->api->rewind(strm->handle);
    while (strm->api->readln(strm->handle, line, sizeof(line))==0) {
        if (strstr(line, match)) {
            return atoi(line);
        }
    }
    return def;
}

static void test_cr_factionstealth(CuTest *tc) {
    stream strm;
    faction *f1, *f2;
    region *r;
    unit *u;

    test_setup();
    mstream_init(&strm);
    f1 = test_create_faction(0);
    f2 = test_create_faction(0);
    r = test_create_region(0, 0, 0);
    u = test_create_unit(f1, r);

    cr_output_unit(&strm, u->region, f1, u, seen_unit);
    CuAssertIntEquals(tc, f1->no, cr_get_int(&strm, ";Partei", -1));
    CuAssertIntEquals(tc, -1, cr_get_int(&strm, ";Anderepartei", -1));
    CuAssertIntEquals(tc, -1, cr_get_int(&strm, ";Verraeter", -1));

    set_factionstealth(u, f2);
    CuAssertPtrNotNull(tc, u->attribs);

    cr_output_unit(&strm, u->region, f1, u, seen_unit);
    CuAssertIntEquals(tc, f1->no, cr_get_int(&strm, ";Partei", -1));
    CuAssertIntEquals(tc, f2->no, cr_get_int(&strm, ";Anderepartei", -1));
    CuAssertIntEquals(tc, -1, cr_get_int(&strm, ";Verraeter", -1));

    cr_output_unit(&strm, u->region, f2, u, seen_unit);
    CuAssertIntEquals(tc, f1->no, cr_get_int(&strm, ";Partei", -1));
    CuAssertIntEquals(tc, f2->no, cr_get_int(&strm, ";Anderepartei", -1));
    CuAssertIntEquals(tc, 1, cr_get_int(&strm, ";Verraeter", -1));

    mstream_done(&strm);
    test_cleanup();
}

CuSuite *get_creport_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_cr_unit);
    SUITE_ADD_TEST(suite, test_cr_factionstealth);
    return suite;
}
