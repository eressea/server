#include <platform.h>
#include "types.h"
#include "ally.h"
#include "group.h"
#include "faction.h"
#include "unit.h"
#include "region.h"
#include "save.h"
#include "version.h"

#include <util/gamedata.h>
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
    int i;
    gamedata *data;

    test_cleanup();
    test_create_world();
    data = gamedata_open("test.dat", "wb", RELEASE_VERSION);
    f = test_create_faction(0);
    g = new_group(f, "NW", 42);
    g = new_group(f, "Egoisten", 43);
    key_set(&g->attribs, 44);
    al = ally_add(&g->allies, f);
    al->status = HELP_GIVE;
    write_groups(data->store, f);
    WRITE_INT(data->store, 47);
    binstore_done(data->store);
    gamedata_close(data);

    f->groups = 0;
    free_group(g);
    data = gamedata_open("test.dat", "rb", RELEASE_VERSION);
    read_groups(data, f);
    READ_INT(data->store, &i);
    gamedata_close(data);

    CuAssertIntEquals(tc, 47, i);
    CuAssertPtrNotNull(tc, f->groups);
    CuAssertIntEquals(tc, 42, f->groups->gid);
    CuAssertStrEquals(tc, "NW", f->groups->name);
    CuAssertPtrNotNull(tc, f->groups->next);
    CuAssertIntEquals(tc, 43, f->groups->next->gid);
    CuAssertStrEquals(tc, "Egoisten", f->groups->next->name);
    CuAssertPtrEquals(tc, 0, f->groups->allies);
    g = f->groups->next;
    CuAssertTrue(tc, key_get(g->attribs, 44));
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

