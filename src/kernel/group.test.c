#include <platform.h>
#include "types.h"
#include "ally.h"
#include "group.h"
#include "faction.h"
#include "unit.h"
#include "region.h"
#include <util/attrib.h>
#include <attributes/key.h>
#include <stream.h>
#include <filestream.h>
#include <storage.h>
#include <binarystore.h>

#include <CuTest.h>
#include <tests.h>
#include <assert.h>

static void test_group_readwrite(CuTest * tc)
{
    faction * f;
    group *g;
    ally *al;
    storage store;
    FILE *F;
    stream strm;
    int i;

    F = fopen("test.dat", "wb");
    fstream_init(&strm, F);
    binstore_init(&store, &strm);
    test_cleanup();
    test_create_world();
    f = test_create_faction(0);
    g = new_group(f, "NW", 42);
    g = new_group(f, "Egoisten", 43);
    a_add(&g->attribs, make_key(44));
    al = ally_add(&g->allies, f);
    al->status = HELP_GIVE;
    write_groups(&store, f);
    WRITE_INT(&store, 47);
    binstore_done(&store);
    fstream_done(&strm);

    F = fopen("test.dat", "rb");
    fstream_init(&strm, F);
    binstore_init(&store, &strm);
    f->groups = 0;
    read_groups(&store, f);
    READ_INT(&store, &i);
    binstore_done(&store);
    fstream_done(&strm);

    CuAssertIntEquals(tc, 47, i);
    CuAssertPtrNotNull(tc, f->groups);
    CuAssertIntEquals(tc, 42, f->groups->gid);
    CuAssertStrEquals(tc, "NW", f->groups->name);
    CuAssertPtrNotNull(tc, f->groups->next);
    CuAssertIntEquals(tc, 43, f->groups->next->gid);
    CuAssertStrEquals(tc, "Egoisten", f->groups->next->name);
    CuAssertPtrEquals(tc, 0, f->groups->allies);
    g = f->groups->next;
    CuAssertPtrNotNull(tc, find_key(g->attribs, 44));
    CuAssertPtrNotNull(tc, g->allies);
    CuAssertPtrEquals(tc, 0, g->allies->next);
    CuAssertPtrEquals(tc, f, g->allies->faction);
    CuAssertIntEquals(tc, HELP_GIVE, g->allies->status);
    remove("test.dat");
    test_cleanup();
}

static void test_group(CuTest * tc)
{
    unit *u;
    region *r;
    faction *f;
    group *g;

    test_cleanup();
    test_create_world();
    r = findregion(0, 0);
    f = test_create_faction(0);
    assert(r && f);
    u = test_create_unit(f, r);
    assert(u);
    CuAssertPtrNotNull(tc, (g = join_group(u, "hodor")));
    CuAssertPtrEquals(tc, g, get_group(u));
    CuAssertStrEquals(tc, "hodor", g->name);
    CuAssertIntEquals(tc, 1, g->members);
    set_group(u, 0);
    CuAssertIntEquals(tc, 0, g->members);
    CuAssertPtrEquals(tc, 0, get_group(u));
    set_group(u, g);
    CuAssertIntEquals(tc, 1, g->members);
    CuAssertPtrEquals(tc, g, get_group(u));
    test_cleanup();
}

CuSuite *get_group_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_group);
    SUITE_ADD_TEST(suite, test_group_readwrite);
    return suite;
}

